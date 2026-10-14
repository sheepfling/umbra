package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;

/** Re-evaluate timestamped directed subscriptions at the time-grant boundary. */
final class DelaySubscriptionEvaluationTimestampedDirectedInteractionTck {
   private DelaySubscriptionEvaluationTimestampedDirectedInteractionTck() { }

   static void run(RtiFactory factory, String fom, String switchesFom,
         String timeImplementation, String objectClassName, String attributeName,
         String interactionClassName, CallbackModel model) throws Exception {
      runConfiguration(factory, fom, switchesFom, timeImplementation, objectClassName,
         attributeName, interactionClassName, model, false);
      runConfiguration(factory, fom, switchesFom, timeImplementation, objectClassName,
         attributeName, interactionClassName, model, true);
   }

   private static void runConfiguration(RtiFactory factory, String fom, String switchesFom,
         String timeImplementation, String objectClassName, String attributeName,
         String interactionClassName, CallbackModel model, boolean enabled) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder ownerEvents = new Recorder();
      Recorder receiverEvents = new Recorder();
      String federation = "java-tck-delay-tso-directed-" + enabled + "-" + UUID.randomUUID();
      boolean ownerConnected = false, receiverConnected = false, created = false;
      boolean ownerJoined = false, receiverJoined = false;
      try {
         owner.connect(ownerEvents.proxy(), model);
         ownerConnected = true;
         receiver.connect(receiverEvents.proxy(), model);
         receiverConnected = true;
         String[] modules = enabled ? new String[] {fom, switchesFom} : new String[] {fom};
         owner.createFederationExecution(federation, modules, timeImplementation);
         created = true;
         FederateHandle producer = owner.joinFederationExecution(
            "java-tck-delay-tso-directed-owner", federation);
         ownerJoined = true;
         receiver.joinFederationExecution("java-tck-delay-tso-directed-receiver", federation);
         receiverJoined = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle receiverAttribute = receiver.getAttributeHandle(receiverClass, attributeName);
         InteractionClassHandle ownerInteraction =
            owner.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle receiverInteraction =
            receiver.getInteractionClassHandle(interactionClassName);
         JavaTckSupport.check(ownerClass.equals(receiverClass)
               && ownerAttribute.equals(receiverAttribute)
               && ownerInteraction.equals(receiverInteraction),
            "adapter FOM returned inconsistent timestamped directed handles");
         JavaTckSupport.check(owner.getDelaySubscriptionEvaluationSwitch() == enabled
               && receiver.getDelaySubscriptionEvaluationSwitch() == enabled,
            "Delay Subscription Evaluation switch did not match the composed FOM");

         owner.publishObjectClassAttributes(ownerClass,
            JavaTckSupport.attributeSet(owner, ownerAttribute));
         receiver.subscribeObjectClassAttributes(receiverClass,
            JavaTckSupport.attributeSet(receiver, receiverAttribute));
         owner.publishObjectClassDirectedInteractions(ownerClass,
            JavaTckSupport.interactionSet(owner, ownerInteraction));
         owner.changeInteractionOrderType(ownerInteraction, OrderType.TIMESTAMP);
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverEvents.discoveries.contains(object)
               && receiverClass.equals(receiver.getKnownObjectClassHandle(object)),
            "receiver did not discover the directed target before time management");

         LogicalTimeFactory<?, ?> timeFactory = owner.getTimeFactory();
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         owner.enableTimeRegulation(epsilon);
         await(model, owner, objectClassName,
            () -> ownerEvents.timeRegulationEnabled != null, "time regulation");
         receiver.enableTimeConstrained();
         await(model, receiver, objectClassName,
            () -> receiverEvents.timeConstrainedEnabled != null, "time constraint");

         LogicalTime firstTime = add(owner.queryLogicalTime(), intervalOffset(epsilon, 2));
         LogicalTime firstReceiverTime = add(
            receiver.queryLogicalTime(), intervalOffset(epsilon, 2));
         JavaTckSupport.check(firstTime.equals(firstReceiverTime),
            "federates derived different first directed grant times");
         ParameterHandleValueMap empty = owner.getParameterHandleValueMapFactory().create(0);
         MessageRetractionReturn firstRetraction = owner.sendDirectedInteraction(
            ownerInteraction, object, empty, new byte[0], firstTime);
         JavaTckSupport.check(!enabled || (firstRetraction != null
               && firstRetraction.retractionHandleIsValid && firstRetraction.handle != null),
            "enabled timestamped directed send returned an invalid retraction handle");
         receiver.subscribeObjectClassDirectedInteractions(receiverClass,
            JavaTckSupport.interactionSet(receiver, receiverInteraction));
         receiver.timeAdvanceRequest(firstReceiverTime);
         owner.timeAdvanceRequest(firstTime);
         awaitPair(model, owner, receiver, objectClassName,
            () -> ownerEvents.grants.size() >= 1 && receiverEvents.grants.size() >= 1
               && receiverEvents.interactions.size() >= (enabled ? 1 : 0),
            "first timestamped directed grant");

         int firstDeliveryCount = receiverEvents.interactions.size();
         JavaTckSupport.check(enabled ? firstDeliveryCount == 1 : firstDeliveryCount <= 1,
            "unexpected first timestamped directed-interaction count");
         if (firstDeliveryCount != 0) {
            JavaTckSupport.check(firstRetraction != null
                  && firstRetraction.retractionHandleIsValid && firstRetraction.handle != null,
               "delivered directed interaction has no matching retraction designator");
            verify(receiverEvents.interactions.get(0), receiverInteraction, object,
               producer, firstTime, firstRetraction.handle, receiver, "first grant");
         }

         LogicalTime secondTime = add(firstTime, epsilon);
         LogicalTime secondReceiverTime = add(firstReceiverTime, epsilon);
         MessageRetractionReturn secondRetraction = owner.sendDirectedInteraction(
            ownerInteraction, object, empty, new byte[0], secondTime);
         JavaTckSupport.check(secondRetraction != null
               && secondRetraction.retractionHandleIsValid && secondRetraction.handle != null,
            "second timestamped directed send returned an invalid retraction handle");
         receiver.unsubscribeObjectClassDirectedInteractions(receiverClass,
            JavaTckSupport.interactionSet(receiver, receiverInteraction));
         receiver.timeAdvanceRequest(secondReceiverTime);
         owner.timeAdvanceRequest(secondTime);
         awaitPair(model, owner, receiver, objectClassName,
            () -> ownerEvents.grants.size() >= 2 && receiverEvents.grants.size() >= 2,
            "second timestamped directed grant");
         JavaTckSupport.check(receiverEvents.interactions.size() == firstDeliveryCount,
            "timestamped directed interaction arrived after unsubscribe");

         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         owner.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         ownerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (receiverJoined) resign(receiver);
         if (ownerJoined) resignDelete(owner);
         if (created) {
            try { owner.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (receiverConnected) disconnect(receiver);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static LogicalTime add(LogicalTime start, LogicalTimeInterval interval)
         throws Exception {
      return (LogicalTime) start.add(interval);
   }

   private static LogicalTimeInterval intervalOffset(LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTimeInterval result = epsilon;
      for (int i = 1; i < steps; ++i) result = (LogicalTimeInterval) result.add(epsilon);
      return result;
   }

   private static void verify(Directed event, InteractionClassHandle interaction,
         ObjectInstanceHandle object, FederateHandle producer, LogicalTime time,
         MessageRetractionHandle retraction, RTIambassador receiver, String phase)
         throws Exception {
      JavaTckSupport.check(interaction.equals(event.interaction)
            && object.equals(event.object)
            && event.parameters.isEmpty()
            && event.tag.length == 0
            && producer.equals(event.producer)
            && time.equals(event.time)
            && event.sentOrder == OrderType.TIMESTAMP
            && event.receivedOrder == OrderType.TIMESTAMP
            && retraction.equals(event.retraction)
            && event.transportation != null
            && !receiver.getTransportationTypeName(event.transportation).isEmpty(),
         phase + " timestamped directed callback lost standard metadata");
   }

   private static void service(RTIambassador ambassador, CallbackModel model,
         String objectClassName) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) ambassador.evokeMultipleCallbacks(0.0, 1.0);
      else ambassador.getObjectClassHandle(objectClassName);
   }

   private static void await(CallbackModel model, RTIambassador ambassador,
         String objectClassName, Condition condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline) {
         service(ambassador, model, objectClassName);
         if (condition.isTrue()) return;
         Thread.yield();
      }
      throw new AssertionError("timed out waiting for " + description);
   }

   private static void awaitPair(CallbackModel model, RTIambassador owner,
         RTIambassador receiver, String objectClassName, Condition condition,
         String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline) {
         service(owner, model, objectClassName);
         service(receiver, model, objectClassName);
         if (condition.isTrue()) return;
         Thread.yield();
      }
      throw new AssertionError("timed out waiting for " + description);
   }

   @FunctionalInterface
   private interface Condition { boolean isTrue() throws Exception; }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void resignDelete(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static final class Directed {
      InteractionClassHandle interaction;
      ObjectInstanceHandle object;
      ParameterHandleValueMap parameters;
      byte[] tag;
      TransportationTypeHandle transportation;
      FederateHandle producer;
      LogicalTime<?, ?> time;
      OrderType sentOrder;
      OrderType receivedOrder;
      MessageRetractionHandle retraction;
      Directed(Object[] args) {
         interaction = (InteractionClassHandle) args[0];
         object = (ObjectInstanceHandle) args[1];
         parameters = ((ParameterHandleValueMap) args[2]).clone();
         tag = ((byte[]) args[3]).clone();
         transportation = (TransportationTypeHandle) args[4];
         producer = (FederateHandle) args[5];
         time = (LogicalTime<?, ?>) args[6];
         sentOrder = (OrderType) args[7];
         receivedOrder = (OrderType) args[8];
         retraction = (MessageRetractionHandle) args[9];
      }
   }

   private static final class Recorder {
      LogicalTime<?, ?> timeRegulationEnabled;
      LogicalTime<?, ?> timeConstrainedEnabled;
      final List<ObjectInstanceHandle> discoveries = new ArrayList<>();
      final List<Directed> interactions = new ArrayList<>();
      final List<LogicalTime<?, ?>> grants = new ArrayList<>();
      FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, args) -> {
               Object[] a = args == null ? new Object[0] : args;
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && a.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) a[0]);
               } else if ("timeRegulationEnabled".equals(name) && a.length == 1) {
                  timeRegulationEnabled = (LogicalTime<?, ?>) a[0];
               } else if ("timeConstrainedEnabled".equals(name) && a.length == 1) {
                  timeConstrainedEnabled = (LogicalTime<?, ?>) a[0];
               } else if ("receiveDirectedInteraction".equals(name) && a.length == 10) {
                  interactions.add(new Directed(a));
               } else if ("timeAdvanceGrant".equals(name) && a.length == 1) {
                  grants.add((LogicalTime<?, ?>) a[0]);
               } else if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK timestamped directed DSE callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == a[0];
               }
               return null;
            });
      }
   }
}
