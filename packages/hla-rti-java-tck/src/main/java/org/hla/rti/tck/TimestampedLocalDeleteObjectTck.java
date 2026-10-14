package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
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

/** Suppresses a queued timestamped object removal only for a locally deleted recipient. */
final class TimestampedLocalDeleteObjectTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private TimestampedLocalDeleteObjectTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador local = factory.getRtiAmbassador();
      RTIambassador surviving = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder localRecorder = new Recorder();
      Recorder survivingRecorder = new Recorder();
      String federation = "java-tck-timestamped-local-delete-object-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean localConnected = false;
      boolean survivingConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean localJoined = false;
      boolean survivingJoined = false;
      boolean published = false;
      boolean localSubscribed = false;
      boolean survivingSubscribed = false;
      boolean publisherRegulating = false;
      boolean localConstrained = false;
      boolean survivingConstrained = false;
      ObjectClassHandle publisherClass = null;
      ObjectClassHandle localClass = null;
      ObjectClassHandle survivingClass = null;
      AttributeHandleSet publisherAttributes = null;
      AttributeHandleSet localAttributes = null;
      AttributeHandleSet survivingAttributes = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         local.connect(localRecorder.proxy(), callbackModel);
         localConnected = true;
         surviving.connect(survivingRecorder.proxy(), callbackModel);
         survivingConnected = true;

         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-local-delete-publisher", federation);
         publisherJoined = true;
         local.joinFederationExecution("java-tck-local-delete-local", federation);
         localJoined = true;
         surviving.joinFederationExecution("java-tck-local-delete-survivor", federation);
         survivingJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         localClass = local.getObjectClassHandle(objectClassName);
         survivingClass = surviving.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         AttributeHandle localAttribute = local.getAttributeHandle(localClass, attributeName);
         AttributeHandle survivingAttribute = surviving.getAttributeHandle(
            survivingClass, attributeName);
         JavaTckSupport.check(publisherClass != null && localClass != null
               && survivingClass != null && publisherAttribute != null
               && localAttribute != null && survivingAttribute != null,
            "timestamped local object-delete FOM lookup returned a null handle");
         publisherAttributes = JavaTckSupport.attributeSet(publisher, publisherAttribute);
         localAttributes = JavaTckSupport.attributeSet(local, localAttribute);
         survivingAttributes = JavaTckSupport.attributeSet(surviving, survivingAttribute);

         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         publisher.changeDefaultAttributeOrderType(
            publisherClass, publisherAttributes, OrderType.TIMESTAMP);
         local.subscribeObjectClassAttributes(localClass, localAttributes);
         localSubscribed = true;
         surviving.subscribeObjectClassAttributes(survivingClass, survivingAttributes);
         survivingSubscribed = true;
         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(object != null,
            "timestamped local object-delete registration returned a null handle");
         String objectName = publisher.getObjectInstanceName(object);
         JavaTckSupport.check(objectName != null && !objectName.isEmpty(),
            "timestamped local object-delete registration returned an empty name");
         await(local, callbackModel, () -> localRecorder.discoveries.contains(object),
            "local recipient object discovery");
         await(surviving, callbackModel, () -> survivingRecorder.discoveries.contains(object),
            "surviving recipient object discovery");
         JavaTckSupport.check(object.equals(local.getObjectInstanceHandle(objectName))
               && object.equals(surviving.getObjectInstanceHandle(objectName)),
            "object discovery changed registered object identity");

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> localFactory = local.getTimeFactory();
         LogicalTimeFactory<?, ?> survivingFactory = surviving.getTimeFactory();
         String timeFactoryName = publisherFactory.getName();
         JavaTckSupport.check(timeFactoryName.equals(localFactory.getName())
               && timeFactoryName.equals(survivingFactory.getName()),
            "federates selected different logical-time factories");
         LogicalTime publisherInitial = publisher.queryLogicalTime();
         LogicalTime localInitial = local.queryLogicalTime();
         LogicalTime survivingInitial = surviving.queryLogicalTime();
         JavaTckSupport.check(publisherInitial.equals(localInitial)
               && publisherInitial.equals(survivingInitial),
            "federates began at different logical times");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) publisherFactory.makeEpsilon();
         LogicalTimeInterval lookahead = intervalOffset(epsilon, 5);

         local.enableTimeConstrained();
         localConstrained = true;
         await(local, callbackModel, () -> localRecorder.timeConstrainedEnabledCount == 1,
            "local recipient Time Constrained enable");
         surviving.enableTimeConstrained();
         survivingConstrained = true;
         await(surviving, callbackModel,
            () -> survivingRecorder.timeConstrainedEnabledCount == 1,
            "surviving recipient Time Constrained enable");
         publisher.enableTimeRegulation(lookahead);
         publisherRegulating = true;
         await(publisher, callbackModel, () -> publisherRecorder.timeRegulationEnabledCount == 1,
            "publisher Time Regulation enable");

         LogicalTime deletionTime = offset(publisherInitial, epsilon, 6);
         LogicalTime localTarget = offset(localInitial, epsilon, 6);
         LogicalTime survivingTarget = offset(survivingInitial, epsilon, 6);
         LogicalTime publisherTarget = offset(publisherInitial, epsilon, 2);
         byte[] tag = new byte[] {0x4c, 0x4f, 0x43, 0x41, 0x4c, 0x44, 0x45, 0x4c};
         MessageRetractionReturn sent = publisher.deleteObjectInstance(object, tag, deletionTime);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid && sent.handle != null,
            "timestamped object deletion returned no valid retraction handle");
         MessageRetractionHandle retraction = sent.handle;
         JavaTckSupport.check(localRecorder.removals.isEmpty()
               && survivingRecorder.removals.isEmpty(),
            "timestamped object removal arrived before either recipient frontier");

         local.localDeleteObjectInstance(object);
         JavaTckSupport.expectFailure(() -> local.getKnownObjectClassHandle(object),
            "ObjectInstanceNotKnown", "known-object query after local deletion");
         localRecorder.callbackOrder.clear();
         survivingRecorder.callbackOrder.clear();
         publisherRecorder.callbackOrder.clear();
         local.timeAdvanceRequest(localTarget);
         surviving.timeAdvanceRequest(survivingTarget);
         publisher.timeAdvanceRequest(publisherTarget);
         awaitSessions(new RTIambassador[] {publisher, local, surviving}, callbackModel,
            () -> publisherRecorder.grants.size() == 1 && localRecorder.grants.size() == 1
               && survivingRecorder.removals.size() == 1 && survivingRecorder.grants.size() == 1,
            "timestamped object-removal deliveries and grants");

         JavaTckSupport.check(localRecorder.removals.isEmpty() && localRecorder.grants.size() == 1
               && localTarget.equals(localRecorder.grants.get(0)),
            "local deletion did not suppress only the local queued object removal");
         JavaTckSupport.check(localRecorder.callbackOrder.equals(Arrays.asList("grant")),
            "local deletion delivered a stale local removal callback");
         Removal removal = survivingRecorder.removals.get(0);
         JavaTckSupport.check(object.equals(removal.object) && Arrays.equals(tag, removal.tag)
               && producer.equals(removal.producer),
            "surviving removal callback returned the wrong object, tag, or producer");
         JavaTckSupport.check(deletionTime.equals(removal.time)
               && removal.sentOrder == OrderType.TIMESTAMP
               && removal.receivedOrder == OrderType.TIMESTAMP
               && retraction.equals(removal.retraction),
            "surviving removal callback returned incorrect time, order, or retraction metadata");
         JavaTckSupport.check(survivingTarget.equals(survivingRecorder.grants.get(0)),
            "surviving recipient received the wrong time-advance grant");
         JavaTckSupport.check(survivingRecorder.callbackOrder.equals(
               Arrays.asList("remove", "grant")),
            "surviving removal was delivered after its matching grant");
         JavaTckSupport.expectFailure(() -> surviving.getObjectInstanceHandle(objectName),
            "ObjectInstanceNotKnown", "object lookup after timestamped removal");

         local.disableTimeConstrained();
         localConstrained = false;
         surviving.disableTimeConstrained();
         survivingConstrained = false;
         publisher.disableTimeRegulation();
         publisherRegulating = false;
      } finally {
         if (localConstrained && localJoined) {
            try { local.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (survivingConstrained && survivingJoined) {
            try { surviving.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (publisherRegulating && publisherJoined) {
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (localSubscribed && localJoined) {
            try { local.unsubscribeObjectClassAttributes(localClass, localAttributes); }
            catch (Exception ignored) { }
         }
         if (survivingSubscribed && survivingJoined) {
            try { surviving.unsubscribeObjectClassAttributes(survivingClass, survivingAttributes); }
            catch (Exception ignored) { }
         }
         if (published && publisherJoined) {
            try { publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes); }
            catch (Exception ignored) { }
         }
         if (localJoined) {
            try { local.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (survivingJoined) {
            try { surviving.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(
               ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (survivingConnected) {
            try { surviving.disconnect(); } catch (Exception ignored) { }
         }
         if (localConnected) { try { local.disconnect(); } catch (Exception ignored) { } }
         if (publisherConnected) { try { publisher.disconnect(); } catch (Exception ignored) { } }
      }
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static LogicalTimeInterval intervalOffset(LogicalTimeInterval start, int steps)
         throws Exception {
      LogicalTimeInterval result = start;
      for (int i = 1; i < steps; ++i) result = (LogicalTimeInterval) result.add(start);
      return result;
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      awaitSessions(new RTIambassador[] {ambassador}, callbackModel, condition, description);
   }

   private static void awaitSessions(RTIambassador[] ambassadors, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            for (RTIambassador ambassador : ambassadors) JavaTckSupport.drain(ambassador);
         } else {
            Thread.yield();
         }
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static final class Removal {
      private ObjectInstanceHandle object;
      private byte[] tag;
      private FederateHandle producer;
      private LogicalTime<?, ?> time;
      private OrderType sentOrder;
      private OrderType receivedOrder;
      private MessageRetractionHandle retraction;
   }

   private static final class Recorder {
      private volatile int timeRegulationEnabledCount;
      private volatile int timeConstrainedEnabledCount;
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Removal> removals = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabledCount++;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabledCount++;
               } else if ("discoverObjectInstance".equals(name)
                     && arguments != null && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("removeObjectInstance".equals(name)
                     && arguments != null && arguments.length == 7) {
                  Removal removal = new Removal();
                  removal.object = (ObjectInstanceHandle) arguments[0];
                  removal.tag = ((byte[]) arguments[1]).clone();
                  removal.producer = (FederateHandle) arguments[2];
                  removal.time = (LogicalTime<?, ?>) arguments[3];
                  removal.sentOrder = (OrderType) arguments[4];
                  removal.receivedOrder = (OrderType) arguments[5];
                  removal.retraction = (MessageRetractionHandle) arguments[6];
                  removals.add(removal);
                  callbackOrder.add("remove");
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add("grant");
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK local-delete object callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }
   }
}
