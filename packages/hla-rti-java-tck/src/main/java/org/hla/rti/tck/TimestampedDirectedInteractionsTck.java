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
import hla.rti1516_2025.ParameterHandle;
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
import java.util.concurrent.CopyOnWriteArrayList;

/** Combined portable coverage for timestamped directed recipient selection and retraction. */
final class TimestampedDirectedInteractionsTck {
   private TimestampedDirectedInteractionsTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         String parameterName, CallbackModel model) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador targeted = factory.getRtiAmbassador();
      RTIambassador universal = factory.getRtiAmbassador();
      RTIambassador nonOwner = factory.getRtiAmbassador();
      Recorder publisherEvents = new Recorder();
      Recorder targetedEvents = new Recorder();
      Recorder universalEvents = new Recorder();
      Recorder nonOwnerEvents = new Recorder();
      String federation = "java-tck-timestamped-directed-combined-" + UUID.randomUUID();
      boolean publisherConnected = false, targetedConnected = false;
      boolean universalConnected = false, nonOwnerConnected = false, created = false;
      boolean publisherJoined = false, targetedJoined = false;
      boolean universalJoined = false, nonOwnerJoined = false;
      boolean publisherRegulating = false, universalConstrained = false;
      boolean targetedConstrained = false;
      try {
         publisher.connect(publisherEvents.proxy(), model);
         publisherConnected = true;
         targeted.connect(targetedEvents.proxy(), model);
         targetedConnected = true;
         universal.connect(universalEvents.proxy(), model);
         universalConnected = true;
         nonOwner.connect(nonOwnerEvents.proxy(), model);
         nonOwnerConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-timestamped-directed-publisher", federation);
         publisherJoined = true;
         targeted.joinFederationExecution("java-tck-timestamped-directed-target", federation);
         targetedJoined = true;
         universal.joinFederationExecution("java-tck-timestamped-directed-universal", federation);
         universalJoined = true;
         nonOwner.joinFederationExecution("java-tck-timestamped-directed-non-owner", federation);
         nonOwnerJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle targetedClass = targeted.getObjectClassHandle(objectClassName);
         ObjectClassHandle universalClass = universal.getObjectClassHandle(objectClassName);
         ObjectClassHandle nonOwnerClass = nonOwner.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         AttributeHandle targetedAttribute = targeted.getAttributeHandle(
            targetedClass, attributeName);
         AttributeHandle universalAttribute = universal.getAttributeHandle(
            universalClass, attributeName);
         AttributeHandle nonOwnerAttribute = nonOwner.getAttributeHandle(
            nonOwnerClass, attributeName);
         InteractionClassHandle publisherInteraction =
            publisher.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle targetedInteraction =
            targeted.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle universalInteraction =
            universal.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle nonOwnerInteraction =
            nonOwner.getInteractionClassHandle(interactionClassName);
         ParameterHandle publisherParameter =
            publisher.getParameterHandle(publisherInteraction, parameterName);
         ParameterHandle targetedParameter =
            targeted.getParameterHandle(targetedInteraction, parameterName);
         ParameterHandle universalParameter =
            universal.getParameterHandle(universalInteraction, parameterName);
         JavaTckSupport.check(publisherClass.equals(targetedClass)
               && publisherClass.equals(universalClass) && publisherClass.equals(nonOwnerClass)
               && publisherAttribute.equals(targetedAttribute)
               && publisherAttribute.equals(universalAttribute)
               && publisherAttribute.equals(nonOwnerAttribute)
               && publisherInteraction.equals(targetedInteraction)
               && publisherInteraction.equals(universalInteraction)
               && publisherInteraction.equals(nonOwnerInteraction)
               && publisherParameter.equals(targetedParameter)
               && publisherParameter.equals(universalParameter),
            "adapter FOM returned inconsistent timestamped directed handles");

         publisher.publishObjectClassAttributes(publisherClass,
            JavaTckSupport.attributeSet(publisher, publisherAttribute));
         targeted.publishObjectClassAttributes(targetedClass,
            JavaTckSupport.attributeSet(targeted, targetedAttribute));
         publisher.subscribeObjectClassAttributes(publisherClass,
            JavaTckSupport.attributeSet(publisher, publisherAttribute));
         targeted.subscribeObjectClassAttributes(targetedClass,
            JavaTckSupport.attributeSet(targeted, targetedAttribute));
         universal.subscribeObjectClassAttributes(universalClass,
            JavaTckSupport.attributeSet(universal, universalAttribute));
         nonOwner.subscribeObjectClassAttributes(nonOwnerClass,
            JavaTckSupport.attributeSet(nonOwner, nonOwnerAttribute));

         publisher.publishObjectClassDirectedInteractions(publisherClass,
            JavaTckSupport.interactionSet(publisher, publisherInteraction));
         targeted.subscribeObjectClassDirectedInteractions(targetedClass,
            JavaTckSupport.interactionSet(targeted, targetedInteraction));
         universal.subscribeObjectClassDirectedInteractionsUniversally(universalClass,
            JavaTckSupport.interactionSet(universal, universalInteraction));
         nonOwner.subscribeObjectClassDirectedInteractions(nonOwnerClass,
            JavaTckSupport.interactionSet(nonOwner, nonOwnerInteraction));
         publisher.changeInteractionOrderType(publisherInteraction, OrderType.TIMESTAMP);

         ObjectInstanceHandle target = targeted.registerObjectInstance(targetedClass);
         drainAll(model, publisher, targeted, universal, nonOwner);
         JavaTckSupport.check(publisherEvents.discoveries.contains(target)
               && universalEvents.discoveries.contains(target)
               && nonOwnerEvents.discoveries.contains(target),
            "ordinary attribute subscriptions did not establish target discoverability");

         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         String timeFactoryName = timeFactory.getName();
         LogicalTimeFactory<?, ?> targetedTimeFactory = targeted.getTimeFactory();
         LogicalTimeFactory<?, ?> universalTimeFactory = universal.getTimeFactory();
         JavaTckSupport.check(timeFactoryName.equals(targetedTimeFactory.getName())
               && timeFactoryName.equals(universalTimeFactory.getName()),
            "timestamped directed members selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         universal.enableTimeConstrained();
         universalConstrained = true;
         publisher.enableTimeRegulation(epsilon);
         publisherRegulating = true;
         await(model, publisher, objectClassName,
            () -> publisherEvents.timeRegulationEnabled != null, "publisher regulation");
         await(model, universal, objectClassName,
            () -> universalEvents.timeConstrainedEnabled != null, "universal time constraint");

         LogicalTime publisherInitial = publisher.queryLogicalTime();
         LogicalTime targetedInitial = targeted.queryLogicalTime();
         LogicalTime universalInitial = universal.queryLogicalTime();
         JavaTckSupport.check(publisherInitial.equals(targetedInitial)
               && publisherInitial.equals(universalInitial),
            "timestamped directed members began at different logical times");
         LogicalTime firstTime = offset(publisherInitial, epsilon, 3);
         LogicalTime firstTargetedTime = offset(targetedInitial, epsilon, 3);
         LogicalTime firstUniversalTime = offset(universalInitial, epsilon, 3);
         byte[] firstValue = new byte[] {0x41, 0x42, 0x43};
         byte[] firstTag = new byte[] {0x51, 0x52};

         MessageRetractionHandle invalidRetraction =
            JavaTckSupport.invalidHandle(MessageRetractionHandle.class);
         JavaTckSupport.expectFailure(() -> publisher.retract(invalidRetraction),
            "InvalidMessageRetractionHandle", "invalid directed retraction handle");
         InteractionClassHandle invalidInteraction =
            JavaTckSupport.invalidHandle(InteractionClassHandle.class);
         LogicalTime invalidSendTime = offset(publisherInitial, epsilon, 3);
         JavaTckSupport.expectFailure(() -> publisher.sendDirectedInteraction(
               invalidInteraction, target,
               publisher.getParameterHandleValueMapFactory().create(0), new byte[0],
               invalidSendTime),
            "InteractionClassNotDefined", "timestamped directed send with invalid class");
         ObjectInstanceHandle invalidTarget =
            JavaTckSupport.invalidHandle(ObjectInstanceHandle.class);
         JavaTckSupport.expectFailure(() -> publisher.sendDirectedInteraction(
               publisherInteraction, invalidTarget,
               publisher.getParameterHandleValueMapFactory().create(0), new byte[0],
               invalidSendTime),
            "ObjectInstanceNotKnown", "timestamped directed send with unknown target");

         MessageRetractionReturn first = send(publisher, publisherInteraction,
            publisherParameter, target, firstValue, firstTag, firstTime);
         JavaTckSupport.check(first != null && first.retractionHandleIsValid && first.handle != null,
            "first timestamped directed send returned no valid retraction handle");
         await(model, targeted, objectClassName,
            () -> targetedEvents.interactions.size() >= 1,
            "non-constrained directed target receive-order delivery");
         JavaTckSupport.check(targetedEvents.interactions.size() == 1,
            "target received duplicate receive-order directed delivery");
         verify(targetedEvents.interactions.get(0), targetedInteraction, targetedParameter,
            target, firstValue, firstTag, producer, firstTime, first.handle,
            OrderType.RECEIVE, targeted, "non-constrained target");
         JavaTckSupport.check(universalEvents.interactions.isEmpty()
               && nonOwnerEvents.interactions.isEmpty(),
            "first directed send reached a constrained universal or non-owner recipient early");

         publisher.retract(first.handle);
         await(model, targeted, objectClassName,
            () -> targetedEvents.retractions.size() >= 1,
            "target retraction callback");
         JavaTckSupport.check(targetedEvents.retractions.size() == 1
               && first.handle.equals(targetedEvents.retractions.get(0))
               && universalEvents.retractions.isEmpty(),
            "first retraction did not reach only the already-delivered target copy");

         targeted.enableTimeConstrained();
         targetedConstrained = true;
         await(model, targeted, objectClassName,
            () -> targetedEvents.timeConstrainedEnabled != null,
            "target time constraint");
         targeted.timeAdvanceRequest(firstTargetedTime);
         universal.timeAdvanceRequest(firstUniversalTime);
         publisher.timeAdvanceRequest(firstTime);
         awaitPair(model, publisher, targeted, universal, objectClassName,
            () -> publisherEvents.grants.size() >= 1 && targetedEvents.grants.size() >= 1
               && universalEvents.grants.size() >= 1,
            "first directed time-advance grants");
         JavaTckSupport.check(firstTargetedTime.equals(targetedEvents.grants.get(0))
               && firstUniversalTime.equals(universalEvents.grants.get(0))
               && firstTime.equals(publisherEvents.grants.get(0))
               && targetedEvents.interactions.size() == 1
               && universalEvents.interactions.isEmpty()
               && targetedEvents.retractions.size() == 1
               && universalEvents.retractions.isEmpty(),
            "retracted first directed copy was delivered or changed recipient state at grant");

         LogicalTime secondTime = offset(firstTime, epsilon, 3);
         LogicalTime secondTargetedTime = offset(firstTargetedTime, epsilon, 3);
         LogicalTime secondUniversalTime = offset(firstUniversalTime, epsilon, 3);
         byte[] secondValue = new byte[] {0x61, 0x62, 0x63};
         byte[] secondTag = new byte[] {0x71, 0x72};
         MessageRetractionReturn second = send(publisher, publisherInteraction,
            publisherParameter, target, secondValue, secondTag, secondTime);
         JavaTckSupport.check(second != null && second.retractionHandleIsValid
               && second.handle != null, "second timestamped directed send returned no valid handle");
         targeted.timeAdvanceRequest(secondTargetedTime);
         universal.timeAdvanceRequest(secondUniversalTime);
         publisher.timeAdvanceRequest(secondTime);
         awaitPair(model, publisher, targeted, universal, objectClassName,
            () -> publisherEvents.grants.size() >= 2 && targetedEvents.grants.size() >= 2
               && universalEvents.grants.size() >= 2 && targetedEvents.interactions.size() >= 2
               && universalEvents.interactions.size() >= 1,
            "second directed delivery and time-advance grants");
         JavaTckSupport.check(nonOwnerEvents.interactions.isEmpty()
               && targetedEvents.interactions.size() == 2
               && universalEvents.interactions.size() == 1,
            "directed send did not select exactly the owner and universal subscribers");
         verify(targetedEvents.interactions.get(1), targetedInteraction, targetedParameter,
            target, secondValue, secondTag, producer, secondTime, second.handle,
            OrderType.TIMESTAMP, targeted, "targeted recipient");
         verify(universalEvents.interactions.get(0), universalInteraction, universalParameter,
            target, secondValue, secondTag, producer, secondTime, second.handle,
            OrderType.TIMESTAMP, universal, "universal recipient");
         JavaTckSupport.expectFailure(() -> publisher.retract(second.handle),
            "MessageCanNoLongerBeRetracted", "retraction after directed delivery");
         JavaTckSupport.check(targetedEvents.retractions.size() == 1
               && universalEvents.retractions.isEmpty(),
            "second delivered directed message changed prior retraction callback state");

         targeted.disableTimeConstrained();
         targetedConstrained = false;
         universal.disableTimeConstrained();
         universalConstrained = false;
         publisher.disableTimeRegulation();
         publisherRegulating = false;
         targeted.unsubscribeObjectClassDirectedInteractions(targetedClass,
            JavaTckSupport.interactionSet(targeted, targetedInteraction));
         universal.unsubscribeObjectClassDirectedInteractions(universalClass,
            JavaTckSupport.interactionSet(universal, universalInteraction));
         nonOwner.unsubscribeObjectClassDirectedInteractions(nonOwnerClass,
            JavaTckSupport.interactionSet(nonOwner, nonOwnerInteraction));
         publisher.unpublishObjectClassDirectedInteractions(publisherClass,
            JavaTckSupport.interactionSet(publisher, publisherInteraction));
         publisher.unsubscribeObjectClass(publisherClass);
         targeted.unsubscribeObjectClass(targetedClass);
         universal.unsubscribeObjectClass(universalClass);
         nonOwner.unsubscribeObjectClass(nonOwnerClass);
         publisher.unpublishObjectClassAttributes(publisherClass,
            JavaTckSupport.attributeSet(publisher, publisherAttribute));
         targeted.unpublishObjectClassAttributes(targetedClass,
            JavaTckSupport.attributeSet(targeted, targetedAttribute));

         universal.resignFederationExecution(ResignAction.NO_ACTION);
         universalJoined = false;
         nonOwner.resignFederationExecution(ResignAction.NO_ACTION);
         nonOwnerJoined = false;
         targeted.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         targetedJoined = false;
         publisher.resignFederationExecution(ResignAction.NO_ACTION);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (targetedConstrained && targetedJoined) {
            try { targeted.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (universalConstrained && universalJoined) {
            try { universal.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (publisherRegulating && publisherJoined) {
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (universalJoined) resign(universal);
         if (nonOwnerJoined) resign(nonOwner);
         if (targetedJoined) resignDelete(targeted);
         if (publisherJoined) resign(publisher);
         if (created) {
            try { publisher.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (nonOwnerConnected) disconnect(nonOwner);
         if (universalConnected) disconnect(universal);
         if (targetedConnected) disconnect(targeted);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static MessageRetractionReturn send(RTIambassador publisher,
         InteractionClassHandle interaction, ParameterHandle parameter,
         ObjectInstanceHandle target, byte[] value, byte[] tag, LogicalTime time)
         throws Exception {
      ParameterHandleValueMap parameters = publisher.getParameterHandleValueMapFactory().create(1);
      parameters.put(parameter, value);
      return publisher.sendDirectedInteraction(interaction, target, parameters, tag, time);
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(epsilon);
      return result;
   }

   private static void verify(Directed event, InteractionClassHandle interaction,
         ParameterHandle parameter, ObjectInstanceHandle target, byte[] value, byte[] tag,
         FederateHandle producer, LogicalTime time, MessageRetractionHandle retraction,
         OrderType receivedOrder, RTIambassador inspector, String recipient) throws Exception {
      JavaTckSupport.check(interaction.equals(event.interaction) && target.equals(event.target)
            && event.parameters.size() == 1 && event.parameters.containsKey(parameter)
            && Arrays.equals(value, event.parameters.get(parameter))
            && Arrays.equals(tag, event.tag) && producer.equals(event.producer)
            && time.equals(event.time) && event.sentOrder == OrderType.TIMESTAMP
            && event.receivedOrder == receivedOrder && retraction.equals(event.retraction)
            && event.transportation != null
            && !inspector.getTransportationTypeName(event.transportation).isEmpty(),
         recipient + " timestamped directed callback lost standard delivery metadata");
   }

   private static void drainAll(CallbackModel model, RTIambassador... ambassadors)
         throws Exception {
      if (model != CallbackModel.HLA_EVOKED) return;
      for (RTIambassador ambassador : ambassadors) JavaTckSupport.drain(ambassador);
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

   private static void awaitPair(CallbackModel model, RTIambassador publisher,
         RTIambassador targeted, RTIambassador universal, String objectClassName,
         Condition condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline) {
         service(publisher, model, objectClassName);
         service(targeted, model, objectClassName);
         service(universal, model, objectClassName);
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
      ObjectInstanceHandle target;
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
         target = (ObjectInstanceHandle) args[1];
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
      final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      final List<Directed> interactions = new CopyOnWriteArrayList<>();
      final List<MessageRetractionHandle> retractions = new CopyOnWriteArrayList<>();
      final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
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
                  Directed event = new Directed(a);
                  interactions.add(event);
               } else if ("requestRetraction".equals(name) && a.length == 1) {
                  retractions.add((MessageRetractionHandle) a[0]);
               } else if ("timeAdvanceGrant".equals(name) && a.length == 1) {
                  grants.add((LogicalTime<?, ?>) a[0]);
               } else if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK timestamped directed callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == a[0];
               }
               return null;
            });
      }
   }
}
