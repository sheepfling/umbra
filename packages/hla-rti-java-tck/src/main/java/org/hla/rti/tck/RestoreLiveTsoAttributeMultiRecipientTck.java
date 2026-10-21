package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Restore one saved live timestamped attribute update for two recipients. */
final class RestoreLiveTsoAttributeMultiRecipientTck {
   private RestoreLiveTsoAttributeMultiRecipientTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel model)
         throws Exception {
      JavaTckSupport.check(model == CallbackModel.HLA_EVOKED,
         "multi-recipient attribute restore has native evidence only for evoked callbacks");
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador first = factory.getRtiAmbassador();
      RTIambassador second = factory.getRtiAmbassador();
      Recorder publisherEvents = new Recorder();
      Recorder firstEvents = new Recorder();
      Recorder secondEvents = new Recorder();
      List<RTIambassador> members = List.of(publisher, first, second);
      List<RTIambassador> connected = new ArrayList<>();
      List<RTIambassador> joined = new ArrayList<>();
      String federation = "java-tck-restore-live-tso-attribute-multi-" + UUID.randomUUID();
      String saveLabel = "tck-live-tso-attribute-multi-baseline";
      byte[] value = {'M', 'U', 'L', 'T', 'I'};
      byte[] tag = {'M', 'E', 'M', '-', 'T'};
      boolean created = false;
      try {
         publisher.connect(publisherEvents.proxy(), model);
         connected.add(publisher);
         first.connect(firstEvents.proxy(), model);
         connected.add(first);
         second.connect(secondEvents.proxy(), model);
         connected.add(second);
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle publisherHandle = publisher.joinFederationExecution(
            "live-tso-attribute-publisher", "publisher", federation);
         joined.add(publisher);
         first.joinFederationExecution("live-tso-attribute-first", "subscriber", federation);
         joined.add(first);
         second.joinFederationExecution("live-tso-attribute-second", "subscriber", federation);
         joined.add(second);

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         AttributeHandleSet publishedAttributes = JavaTckSupport.attributeSet(
            publisher, publisherAttribute);
         AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(1);
         values.put(publisherAttribute, value.clone());
         publisher.publishObjectClassAttributes(publisherClass, publishedAttributes);
         RecipientHandles firstHandles = subscribe(first, objectClassName, attributeName);
         RecipientHandles secondHandles = subscribe(second, objectClassName, attributeName);
         publisher.changeDefaultAttributeOrderType(
            publisherClass, publishedAttributes, OrderType.TIMESTAMP);
         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         await(members, () -> firstEvents.discoveries.contains(object)
               && secondEvents.discoveries.contains(object),
            "both attribute target discoveries");
         first.enableTimeConstrained();
         second.enableTimeConstrained();
         first.evokeCallback(0.0);
         second.evokeCallback(0.0);
         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         LogicalTime<?, ?> initial = publisher.queryLogicalTime();
         LogicalTimeInterval<?> epsilon = timeFactory.makeEpsilon();
         LogicalTime<?, ?> time1 = offset(initial, epsilon, 1);
         LogicalTime<?, ?> time7 = offset(initial, epsilon, 7);
         LogicalTime<?, ?> time10 = offset(initial, epsilon, 10);
         publisher.enableTimeRegulation(interval(epsilon, 5));
         await(members, () -> publisherEvents.regulationEnabled == 1,
            "publisher time-regulation enablement");

         MessageRetractionReturn saved = publisher.updateAttributeValues(
            object, values, tag.clone(), time7);
         JavaTckSupport.check(saved != null && saved.retractionHandleIsValid
               && saved.handle != null && firstEvents.reflections.isEmpty()
               && secondEvents.reflections.isEmpty(),
            "timestamp-seven update was not queued for both recipients");
         first.timeAdvanceRequest(time1);
         second.timeAdvanceRequest(time1);
         publisher.requestFederationSave(saveLabel);
         await(members, () -> publisherEvents.saveLabels.equals(List.of(saveLabel))
               && firstEvents.saveLabels.equals(List.of(saveLabel))
               && secondEvents.saveLabels.equals(List.of(saveLabel)),
            "three-member attribute save initiation");
         publisher.federateSaveBegun();
         first.federateSaveBegun();
         second.federateSaveBegun();
         publisher.federateSaveComplete();
         first.federateSaveComplete();
         second.federateSaveComplete();
         await(members, () -> publisherEvents.savedCount == 1
               && firstEvents.savedCount == 1 && secondEvents.savedCount == 1,
            "three-member attribute federation saved");

         publisher.retract(saved.handle);
         first.evokeCallback(0.0);
         second.evokeCallback(0.0);
         JavaTckSupport.check(firstEvents.reflections.isEmpty()
               && secondEvents.reflections.isEmpty(),
            "post-save retract leaked a queued reflection");
         JavaTckSupport.expectFailure(() -> publisher.retract(saved.handle),
            "MessageCanNoLongerBeRetracted", "post-save terminal update handle");
         publisher.requestFederationRestore(saveLabel);
         await(members, () -> publisherEvents.restoreLabels.equals(List.of(saveLabel))
               && firstEvents.restoreLabels.equals(List.of(saveLabel))
               && secondEvents.restoreLabels.equals(List.of(saveLabel)),
            "three-member attribute restore initiation");
         publisher.federateRestoreComplete();
         first.federateRestoreComplete();
         second.federateRestoreComplete();
         await(members, () -> publisherEvents.restoredCount == 1
               && firstEvents.restoredCount == 1 && secondEvents.restoredCount == 1,
            "three-member attribute federation restored");

         firstEvents.order.clear();
         secondEvents.order.clear();
         first.flushQueueRequest(time10);
         await(members, () -> firstEvents.reflections.size() == 1
               && firstEvents.flushGrants.size() == 1,
            "first restored reflection and FQG");
         JavaTckSupport.check(secondEvents.reflections.isEmpty()
               && secondEvents.flushGrants.isEmpty() && secondEvents.order.isEmpty(),
            "first FQR prematurely released the second reflection");
         second.flushQueueRequest(time10);
         await(members, () -> secondEvents.reflections.size() == 1
               && secondEvents.flushGrants.size() == 1,
            "second restored reflection and FQG");
         checkReflected(firstEvents, firstHandles, object, publisherHandle,
            saved.handle, time7, value, tag);
         checkReflected(secondEvents, secondHandles, object, publisherHandle,
            saved.handle, time7, value, tag);

         firstEvents.order.clear();
         secondEvents.order.clear();
         publisher.retract(saved.handle);
         await(members, () -> firstEvents.retractions.size() == 1
               && secondEvents.retractions.size() == 1,
            "both restored attribute request-retraction callbacks");
         JavaTckSupport.check(firstEvents.retractions.get(0).equals(saved.handle)
               && secondEvents.retractions.get(0).equals(saved.handle)
               && firstEvents.order.equals(List.of("request-retraction"))
               && secondEvents.order.equals(List.of("request-retraction")),
            "restored attribute retraction callbacks differed between recipients");
         JavaTckSupport.expectFailure(() -> publisher.retract(saved.handle),
            "MessageCanNoLongerBeRetracted", "restored terminal update handle");

         unsubscribe(first, firstHandles);
         unsubscribe(second, secondHandles);
         first.resignFederationExecution(ResignAction.NO_ACTION);
         joined.remove(first);
         second.resignFederationExecution(ResignAction.NO_ACTION);
         joined.remove(second);
         publisher.resignFederationExecution(ResignAction.NO_ACTION);
         joined.remove(publisher);
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         for (int index = joined.size() - 1; index >= 0; --index)
            try { joined.get(index).resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         for (int index = connected.size() - 1; index >= 0; --index)
            try { connected.get(index).disconnect(); } catch (Exception ignored) { }
      }
   }

   private static RecipientHandles subscribe(RTIambassador member,
         String objectClassName, String attributeName) throws Exception {
      RecipientHandles handles = new RecipientHandles();
      handles.objectClass = member.getObjectClassHandle(objectClassName);
      handles.attribute = member.getAttributeHandle(handles.objectClass, attributeName);
      handles.attributes = JavaTckSupport.attributeSet(member, handles.attribute);
      member.subscribeObjectClassAttributes(handles.objectClass, handles.attributes);
      return handles;
   }

   private static void unsubscribe(RTIambassador member, RecipientHandles handles)
         throws Exception {
      member.unsubscribeObjectClassAttributes(handles.objectClass, handles.attributes);
   }

   private static void checkReflected(Recorder events, RecipientHandles handles,
         ObjectInstanceHandle object, FederateHandle publisher,
         MessageRetractionHandle saved, LogicalTime<?, ?> time7,
         byte[] value, byte[] tag) {
      Reflection reflection = events.reflections.get(0);
      JavaTckSupport.check(reflection.object.equals(object)
            && reflection.values.size() == 1
            && Arrays.equals(reflection.values.get(handles.attribute), value)
            && Arrays.equals(reflection.tag, tag)
            && reflection.producer.equals(publisher)
            && (reflection.regions == null || reflection.regions.isEmpty())
            && reflection.time.equals(time7)
            && reflection.sentOrder == OrderType.TIMESTAMP
            && reflection.receivedOrder == OrderType.TIMESTAMP
            && reflection.retraction.equals(saved)
            && events.order.equals(List.of("reflect", "flush-grant")),
         "restored recipient reflection or callback ordering changed");
   }

   private static void await(List<RTIambassador> members,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline)
         for (RTIambassador member : members) member.evokeCallback(0.0);
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime offset(LogicalTime start,
         LogicalTimeInterval epsilon, int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) result = (LogicalTime) result.add(epsilon);
      return result;
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTimeInterval interval(LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTimeInterval result = epsilon;
      for (int index = 1; index < steps; ++index)
         result = (LogicalTimeInterval) result.add(epsilon);
      return result;
   }

   private static final class RecipientHandles {
      ObjectClassHandle objectClass;
      AttributeHandle attribute;
      AttributeHandleSet attributes;
   }

   private static final class Reflection {
      ObjectInstanceHandle object;
      AttributeHandleValueMap values;
      byte[] tag;
      FederateHandle producer;
      RegionHandleSet regions;
      LogicalTime<?, ?> time;
      OrderType sentOrder, receivedOrder;
      MessageRetractionHandle retraction;
   }

   private static final class Recorder {
      final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      final List<LogicalTime<?, ?>> flushGrants = new CopyOnWriteArrayList<>();
      final List<MessageRetractionHandle> retractions = new CopyOnWriteArrayList<>();
      final List<String> saveLabels = new CopyOnWriteArrayList<>();
      final List<String> restoreLabels = new CopyOnWriteArrayList<>();
      final List<String> order = new CopyOnWriteArrayList<>();
      volatile int regulationEnabled, savedCount, restoredCount;

      FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class}, (proxy, method, args) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name))
                  discoveries.add((ObjectInstanceHandle) args[0]);
               else if ("timeRegulationEnabled".equals(name)) regulationEnabled++;
               else if ("initiateFederateSave".equals(name)) saveLabels.add((String) args[0]);
               else if ("federationSaved".equals(name)) savedCount++;
               else if ("initiateFederateRestore".equals(name))
                  restoreLabels.add((String) args[0]);
               else if ("federationRestored".equals(name)) restoredCount++;
               else if ("reflectAttributeValues".equals(name) && args.length == 10) {
                  Reflection reflection = new Reflection();
                  reflection.object = (ObjectInstanceHandle) args[0];
                  reflection.values = ((AttributeHandleValueMap) args[1]).clone();
                  reflection.tag = ((byte[]) args[2]).clone();
                  reflection.producer = (FederateHandle) args[4];
                  reflection.regions = args[5] == null ? null :
                     (RegionHandleSet) ((RegionHandleSet) args[5]).clone();
                  reflection.time = (LogicalTime<?, ?>) args[6];
                  reflection.sentOrder = (OrderType) args[7];
                  reflection.receivedOrder = (OrderType) args[8];
                  reflection.retraction = (MessageRetractionHandle) args[9];
                  reflections.add(reflection);
                  order.add("reflect");
               } else if ("flushQueueGrant".equals(name)) {
                  flushGrants.add((LogicalTime<?, ?>) args[0]);
                  order.add("flush-grant");
               } else if ("requestRetraction".equals(name)) {
                  retractions.add((MessageRetractionHandle) args[0]);
                  order.add("request-retraction");
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK attribute multi-restore callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == args[0];
               }
               return null;
            });
      }
   }
}
