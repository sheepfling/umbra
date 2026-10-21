package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
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

/** Four-member public-API save/restore of mixed timestamped object and interaction traffic. */
final class RestoreMixedTsoFourMemberTck {
   private RestoreMixedTsoFourMemberTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName,
         String interactionClassName, String parameterName,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "four-member mixed TSO restore has independent evidence only for evoked callbacks");
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador mirror = factory.getRtiAmbassador();
      RTIambassador sender = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder ownerEvents = new Recorder();
      Recorder mirrorEvents = new Recorder();
      Recorder senderEvents = new Recorder();
      Recorder observerEvents = new Recorder();
      List<RTIambassador> members = List.of(owner, mirror, sender, observer);
      List<RTIambassador> connected = new ArrayList<>();
      List<RTIambassador> joined = new ArrayList<>();
      String federation = "java-tck-restore-mixed-tso-four-member-" + UUID.randomUUID();
      String objectName = federation + "-object";
      String saveLabel = "tck-mixed-tso-four-member-save";
      byte[] attributePayload = {'M', 'I', 'X', '-', 'A'};
      byte[] interactionPayload = {'M', 'I', 'X', '-', 'I'};
      byte[] commonTag = {'M', 'I', 'X', '-', 'T'};
      byte[] deletionTag = {'M', 'I', 'X', '-', 'D'};
      boolean created = false;
      try {
         Recorder[] recorders = {ownerEvents, mirrorEvents, senderEvents, observerEvents};
         for (int index = 0; index < members.size(); ++index) {
            members.get(index).connect(recorders[index].proxy(), callbackModel);
            connected.add(members.get(index));
         }
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle ownerHandle = owner.joinFederationExecution("mixed-member", federation);
         joined.add(owner);
         mirror.joinFederationExecution("mixed-member", federation);
         joined.add(mirror);
         FederateHandle senderHandle = sender.joinFederationExecution("mixed-member", federation);
         joined.add(sender);
         observer.joinFederationExecution("mixed-member", federation);
         joined.add(observer);

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle mirrorClass = mirror.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle mirrorAttribute = mirror.getAttributeHandle(mirrorClass, attributeName);
         AttributeHandle deletePrivilege = owner.getAttributeHandle(
            ownerClass, "HLAprivilegeToDeleteObject");
         InteractionClassHandle senderClass = sender.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle observerClass = observer.getInteractionClassHandle(interactionClassName);
         ParameterHandle senderParameter = sender.getParameterHandle(senderClass, parameterName);
         ParameterHandle observerParameter = observer.getParameterHandle(observerClass, parameterName);
         AttributeHandleSet ownerAttributes = owner.getAttributeHandleSetFactory().create();
         ownerAttributes.add(ownerAttribute);
         ownerAttributes.add(deletePrivilege);
         AttributeHandleSet mirrorAttributes = mirror.getAttributeHandleSetFactory().create();
         mirrorAttributes.add(mirrorAttribute);
         AttributeHandleSet orderedAttributes = owner.getAttributeHandleSetFactory().create();
         orderedAttributes.add(ownerAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         mirror.subscribeObjectClassAttributes(mirrorClass, mirrorAttributes);
         sender.publishInteractionClass(senderClass);
         observer.subscribeInteractionClass(observerClass);
         owner.changeDefaultAttributeOrderType(ownerClass, orderedAttributes, OrderType.TIMESTAMP);
         sender.changeInteractionOrderType(senderClass, OrderType.TIMESTAMP);

         mirror.enableTimeConstrained();
         observer.enableTimeConstrained();
         LogicalTimeFactory<?, ?> timeFactory = owner.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(mirror.getTimeFactory().getName())
               && timeFactory.getName().equals(sender.getTimeFactory().getName())
               && timeFactory.getName().equals(observer.getTimeFactory().getName()),
            "mixed TSO members selected different logical-time factories");
         LogicalTime<?, ?> initial = owner.queryLogicalTime();
         LogicalTimeInterval<?> epsilon = timeFactory.makeEpsilon();
         LogicalTime<?, ?> time1 = offset(initial, epsilon, 1);
         LogicalTime<?, ?> time7 = offset(initial, epsilon, 7);
         LogicalTime<?, ?> time8 = offset(initial, epsilon, 8);
         LogicalTime<?, ?> time10 = offset(initial, epsilon, 10);
         owner.enableTimeRegulation(epsilon);
         sender.enableTimeRegulation(epsilon);
         awaitAll(members, () -> ownerEvents.regulationEnabled == 1
               && senderEvents.regulationEnabled == 1,
            "mixed TSO time-regulation enablement");

         owner.reserveObjectInstanceName(objectName);
         awaitAll(members, () -> ownerEvents.reservedNames.contains(objectName),
            "mixed TSO object name reservation");
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass, objectName);
         awaitAll(members, () -> mirrorEvents.discoveries.size() == 1,
            "mixed TSO object discovery");
         JavaTckSupport.check(mirrorEvents.discoveries.get(0).equals(object)
               && mirror.getObjectInstanceHandle(objectName).equals(object),
            "mixed TSO named object was not discovered by the mirror");

         AttributeHandleValueMap attributes = owner.getAttributeHandleValueMapFactory().create(1);
         attributes.put(ownerAttribute, attributePayload);
         ParameterHandleValueMap parameters = sender.getParameterHandleValueMapFactory().create(1);
         parameters.put(senderParameter, interactionPayload);
         MessageRetractionReturn attributeRetraction = owner.updateAttributeValues(
            object, attributes, commonTag, time7);
         MessageRetractionReturn interactionRetraction = sender.sendInteraction(
            senderClass, parameters, commonTag, time7);
         MessageRetractionReturn deletionRetraction = owner.deleteObjectInstance(
            object, deletionTag, time8);
         JavaTckSupport.check(valid(attributeRetraction) && valid(interactionRetraction)
               && valid(deletionRetraction) && mirrorEvents.reflections.isEmpty()
               && observerEvents.interactions.isEmpty(),
            "mixed TSO submissions lacked retraction handles or arrived before grants");

         owner.requestFederationSave(saveLabel);
         for (RTIambassador member : members) member.timeAdvanceRequest(time1);
         awaitAll(members, () -> ownerEvents.saveLabels.size() == 1
               && mirrorEvents.saveLabels.size() == 1
               && senderEvents.saveLabels.size() == 1
               && observerEvents.saveLabels.size() == 1,
            "mixed TSO four-member save initiation");
         for (Recorder recorder : recorders) {
            JavaTckSupport.check(recorder.saveLabels.equals(List.of(saveLabel)),
               "mixed TSO save initiation label changed");
         }
         for (RTIambassador member : members) member.federateSaveBegun();
         for (RTIambassador member : members) member.federateSaveComplete();
         awaitAll(members, () -> allSaved(recorders), "mixed TSO federation saved");

         owner.retract(attributeRetraction.handle);
         sender.retract(interactionRetraction.handle);
         owner.retract(deletionRetraction.handle);
         JavaTckSupport.check(mirrorEvents.reflections.isEmpty()
               && observerEvents.interactions.isEmpty(),
            "mixed TSO pre-restore retraction released queued callbacks");
         terminal(owner, attributeRetraction.handle);
         terminal(sender, interactionRetraction.handle);
         terminal(owner, deletionRetraction.handle);

         owner.requestFederationRestore(saveLabel);
         awaitAll(members, () -> ownerEvents.restoreRequestSucceeded == 1
               && mirrorEvents.restoreLabels.size() == 1
               && observerEvents.restoreLabels.size() == 1,
            "mixed TSO four-member restore initiation");
         for (RTIambassador member : members) member.federateRestoreComplete();
         awaitAll(members, () -> allRestored(recorders), "mixed TSO federation restored");

         mirrorEvents.callbackOrder.clear();
         observerEvents.callbackOrder.clear();
         mirror.flushQueueRequest(time10);
         awaitAll(members, () -> mirrorEvents.reflections.size() == 1
               && mirrorEvents.removals.size() == 1 && mirrorEvents.flushGrants.size() == 1,
            "mirror restored reflection, removal, and flush grant");
         observer.flushQueueRequest(time10);
         awaitAll(members, () -> observerEvents.interactions.size() == 1
               && observerEvents.flushGrants.size() == 1,
            "observer restored interaction and flush grant");
         TimedPayload reflection = mirrorEvents.reflections.get(0);
         TimedPayload removal = mirrorEvents.removals.get(0);
         TimedPayload interaction = observerEvents.interactions.get(0);
         JavaTckSupport.check(reflection.subject.equals(object)
               && reflection.values.size() == 1
               && Arrays.equals(reflection.values.get(mirrorAttribute), attributePayload)
               && Arrays.equals(reflection.tag, commonTag)
               && reflection.producer.equals(ownerHandle)
               && reflection.time.equals(time7) && reflection.retraction != null
               && removal.subject.equals(object)
               && Arrays.equals(removal.tag, deletionTag)
               && removal.producer.equals(ownerHandle)
               && removal.time.equals(time8)
               && removal.sentOrder == OrderType.TIMESTAMP
               && removal.receivedOrder == OrderType.TIMESTAMP
               && removal.retraction != null
               && interaction.subject.equals(observerClass)
               && interaction.values.size() == 1
               && Arrays.equals(interaction.values.get(observerParameter), interactionPayload)
               && Arrays.equals(interaction.tag, commonTag)
               && interaction.producer.equals(senderHandle)
               && interaction.time.equals(time7) && interaction.retraction != null,
            "mixed TSO restore changed public reflection, removal, or interaction fields");
         JavaTckSupport.check(mirrorEvents.callbackOrder.equals(
                  List.of("reflect", "remove", "flush-grant"))
               && observerEvents.callbackOrder.equals(List.of("interaction", "flush-grant")),
            "mixed TSO callbacks did not precede each Flush Queue grant");

         owner.retract(attributeRetraction.handle);
         sender.retract(interactionRetraction.handle);
         owner.retract(deletionRetraction.handle);
         awaitAll(members, () -> mirrorEvents.retractions.size() == 2
               && observerEvents.retractions.size() == 1,
            "mixed TSO restored retraction callbacks");
         JavaTckSupport.check(mirrorEvents.retractions.stream().allMatch(x -> x != null)
               && observerEvents.retractions.get(0) != null,
            "mixed TSO restored retraction callbacks lacked handles");
         terminal(owner, attributeRetraction.handle);
         terminal(sender, interactionRetraction.handle);
         terminal(owner, deletionRetraction.handle);

         mirror.unsubscribeObjectClassAttributes(mirrorClass, mirrorAttributes);
         observer.unsubscribeInteractionClass(observerClass);
         mirror.resignFederationExecution(ResignAction.NO_ACTION);
         joined.remove(mirror);
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         joined.remove(observer);
         owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         joined.remove(owner);
         sender.resignFederationExecution(ResignAction.NO_ACTION);
         joined.remove(sender);
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         for (int index = joined.size() - 1; index >= 0; --index) {
            RTIambassador member = joined.get(index);
            try { member.resignFederationExecution(member == owner
               ? ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST : ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         for (int index = connected.size() - 1; index >= 0; --index) {
            try { connected.get(index).disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static boolean valid(MessageRetractionReturn result) {
      return result != null && result.retractionHandleIsValid && result.handle != null;
   }

   private static boolean allSaved(Recorder[] recorders) {
      for (Recorder recorder : recorders) if (recorder.savedCount != 1) return false;
      return true;
   }

   private static boolean allRestored(Recorder[] recorders) {
      for (Recorder recorder : recorders) if (recorder.restoredCount != 1) return false;
      return true;
   }

   private static void terminal(RTIambassador member, MessageRetractionHandle handle)
         throws Exception {
      try { member.retract(handle); }
      catch (hla.rti1516_2025.exceptions.MessageCanNoLongerBeRetracted expected) { return; }
      throw new IllegalStateException("retraction handle did not become terminal");
   }

   private static void awaitAll(List<RTIambassador> members,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         for (RTIambassador member : members) member.evokeCallback(0.0);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static final class TimedPayload {
      private final Object subject;
      private final java.util.Map<?, byte[]> values;
      private final byte[] tag;
      private final FederateHandle producer;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;
      private final MessageRetractionHandle retraction;

      private TimedPayload(Object[] args, String kind) {
         subject = args[0];
         values = "remove".equals(kind) ? java.util.Map.of()
            : "reflect".equals(kind)
               ? ((AttributeHandleValueMap) args[1]).clone()
               : ((ParameterHandleValueMap) args[1]).clone();
         tag = ((byte[]) args["remove".equals(kind) ? 1 : 2]).clone();
         producer = (FederateHandle) args["remove".equals(kind) ? 2 : 4];
         time = (LogicalTime<?, ?>) args["remove".equals(kind) ? 3 : 6];
         sentOrder = (OrderType) args["remove".equals(kind) ? 4 : 7];
         receivedOrder = (OrderType) args["remove".equals(kind) ? 5 : 8];
         retraction = (MessageRetractionHandle) args["remove".equals(kind) ? 6 : 9];
      }
   }

   private static final class Recorder {
      private final List<String> reservedNames = new CopyOnWriteArrayList<>();
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<String> saveLabels = new CopyOnWriteArrayList<>();
      private final List<String> restoreLabels = new CopyOnWriteArrayList<>();
      private final List<TimedPayload> reflections = new CopyOnWriteArrayList<>();
      private final List<TimedPayload> removals = new CopyOnWriteArrayList<>();
      private final List<TimedPayload> interactions = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> flushGrants = new CopyOnWriteArrayList<>();
      private final List<MessageRetractionHandle> retractions = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();
      private volatile int regulationEnabled;
      private volatile int savedCount;
      private volatile int restoreRequestSucceeded;
      private volatile int restoredCount;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, args) -> {
               String name = method.getName();
               if ("objectInstanceNameReservationSucceeded".equals(name))
                  reservedNames.add((String) args[0]);
               else if ("discoverObjectInstance".equals(name) && args.length == 4)
                  discoveries.add((ObjectInstanceHandle) args[0]);
               else if ("timeRegulationEnabled".equals(name)) ++regulationEnabled;
               else if ("initiateFederateSave".equals(name)) saveLabels.add((String) args[0]);
               else if ("federationSaved".equals(name)) ++savedCount;
               else if ("requestFederationRestoreSucceeded".equals(name))
                  ++restoreRequestSucceeded;
               else if ("initiateFederateRestore".equals(name))
                  restoreLabels.add((String) args[0]);
               else if ("federationRestored".equals(name)) ++restoredCount;
               else if ("reflectAttributeValues".equals(name) && args.length == 10) {
                  reflections.add(new TimedPayload(args, "reflect"));
                  callbackOrder.add("reflect");
               } else if ("removeObjectInstance".equals(name) && args.length == 7) {
                  removals.add(new TimedPayload(args, "remove"));
                  callbackOrder.add("remove");
               } else if ("receiveInteraction".equals(name) && args.length == 10) {
                  interactions.add(new TimedPayload(args, "interaction"));
                  callbackOrder.add("interaction");
               } else if ("flushQueueGrant".equals(name)) {
                  flushGrants.add((LogicalTime<?, ?>) args[0]);
                  callbackOrder.add("flush-grant");
               } else if ("requestRetraction".equals(name))
                  retractions.add((MessageRetractionHandle) args[0]);
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK mixed TSO restore callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == args[0];
               }
               return null;
            });
      }
   }
}
