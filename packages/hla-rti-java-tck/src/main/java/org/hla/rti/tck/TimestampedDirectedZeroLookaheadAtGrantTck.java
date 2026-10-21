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
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks a directed timestamped send at a zero-lookahead sender's just-granted time. */
final class TimestampedDirectedZeroLookaheadAtGrantTck {
   private TimestampedDirectedZeroLookaheadAtGrantTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionName,
         String parameterName, CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador owner = factory.getRtiAmbassador();
      Recorder publisherEvents = new Recorder();
      Recorder ownerEvents = new Recorder();
      String federation = "java-tck-directed-zero-grant-" + UUID.randomUUID();
      boolean publisherConnected = false, ownerConnected = false;
      boolean publisherJoined = false, ownerJoined = false, created = false;
      try {
         publisher.connect(publisherEvents.proxy(), callbackModel);
         publisherConnected = true;
         owner.connect(ownerEvents.proxy(), callbackModel);
         ownerConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle publisherHandle = publisher.joinFederationExecution(
            "java-tck-directed-zero-grant-sender", federation);
         publisherJoined = true;
         owner.joinFederationExecution("java-tck-directed-zero-grant-owner", federation);
         ownerJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute =
            publisher.getAttributeHandle(publisherClass, attributeName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         InteractionClassHandle publisherInteraction =
            publisher.getInteractionClassHandle(interactionName);
         InteractionClassHandle ownerInteraction = owner.getInteractionClassHandle(interactionName);
         ParameterHandle publisherParameter =
            publisher.getParameterHandle(publisherInteraction, parameterName);
         ParameterHandle ownerParameter =
            owner.getParameterHandle(ownerInteraction, parameterName);
         JavaTckSupport.check(publisherClass != null && ownerClass != null
               && publisherAttribute != null && ownerAttribute != null
               && publisherInteraction != null && ownerInteraction != null
               && publisherParameter != null && ownerParameter != null
               && publisherInteraction.equals(ownerInteraction)
               && publisherParameter.equals(ownerParameter),
            "Directed zero-lookahead adapter FOM returned inconsistent handles");

         owner.publishObjectClassAttributes(
            ownerClass, JavaTckSupport.attributeSet(owner, ownerAttribute));
         publisher.subscribeObjectClassAttributes(
            publisherClass, JavaTckSupport.attributeSet(publisher, publisherAttribute));
         publisher.publishObjectClassDirectedInteractions(publisherClass,
            JavaTckSupport.interactionSet(publisher, publisherInteraction));
         owner.subscribeObjectClassDirectedInteractions(ownerClass,
            JavaTckSupport.interactionSet(owner, ownerInteraction));
         publisher.subscribeObjectClassDirectedInteractions(publisherClass,
            JavaTckSupport.interactionSet(publisher, publisherInteraction));
         publisher.changeInteractionOrderType(publisherInteraction, OrderType.TIMESTAMP);
         ObjectInstanceHandle target = owner.registerObjectInstance(ownerClass);
         await(publisher, callbackModel,
            () -> publisherEvents.discoveries.contains(target), "directed target discovery");

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> ownerFactory = owner.getTimeFactory();
         LogicalTime<?, ?> initial = publisher.queryLogicalTime();
         JavaTckSupport.check(publisherFactory.getName().equals(ownerFactory.getName())
               && initial.equals(owner.queryLogicalTime()),
            "Directed zero-lookahead members selected inconsistent time contexts");
         LogicalTimeInterval zero = (LogicalTimeInterval) publisherFactory.makeZero();
         LogicalTimeInterval epsilon = (LogicalTimeInterval) publisherFactory.makeEpsilon();

         owner.enableTimeConstrained();
         await(owner, callbackModel, () -> ownerEvents.constrainedEnabled == 1,
            "directed receiver constrained callback");
         publisher.enableTimeRegulation(zero);
         await(publisher, callbackModel, () -> publisherEvents.regulationEnabled == 1,
            "directed sender regulation callback");
         JavaTckSupport.check(zero.equals(publisher.queryLookahead()),
            "Directed sender did not retain zero actual lookahead");

         LogicalTime<?, ?> grantedTime = offset(initial, epsilon, 5);
         LogicalTime<?, ?> nextTime = offset(initial, epsilon, 6);
         publisher.timeAdvanceRequest(grantedTime);
         await(publisher, callbackModel, () -> publisherEvents.grants.size() == 1,
            "directed sender initial grant");
         JavaTckSupport.check(grantedTime.equals(publisherEvents.grants.get(0))
               && grantedTime.equals(publisher.queryLogicalTime()),
            "Directed sender did not reach requested time");

         byte[] payload = {(byte) 0xd7, 0x15};
         byte[] tag = {(byte) 0xd7, 0x16};
         ParameterHandleValueMap values =
            publisher.getParameterHandleValueMapFactory().create(1);
         values.put(publisherParameter, payload.clone());
         MessageRetractionReturn retraction = publisher.sendDirectedInteraction(
            publisherInteraction, target, values, tag.clone(), grantedTime);
         JavaTckSupport.check(retraction != null && retraction.retractionHandleIsValid
               && retraction.handle != null,
            "Directed zero-lookahead send at just-granted time was rejected");
         MessageRetractionReturn secondRetraction = publisher.sendDirectedInteraction(
            publisherInteraction, target, values, tag.clone(), nextTime);
         JavaTckSupport.check(secondRetraction != null
               && secondRetraction.retractionHandleIsValid
               && secondRetraction.handle != null
               && !retraction.handle.equals(secondRetraction.handle),
            "Concurrent directed sends did not receive distinct federation retraction designators");
         publisher.retract(secondRetraction.handle);
         JavaTckSupport.check(ownerEvents.interactions.isEmpty(),
            "Directed TSO arrived before receiver advanced");

         publisher.timeAdvanceRequest(nextTime);
         await(publisher, callbackModel, () -> publisherEvents.grants.size() == 2,
            "directed sender forward grant");
         owner.timeAdvanceRequest(grantedTime);
         await(owner, callbackModel,
            () -> ownerEvents.interactions.size() == 1 && ownerEvents.grants.size() == 1,
            "directed receiver matching grant");
         Directed event = ownerEvents.interactions.get(0);
         JavaTckSupport.check(ownerEvents.receiveOrderOverloadCount == 0
               && ownerEvents.callbackOrder.equals(Arrays.asList("directed", "grant"))
               && ownerInteraction.equals(event.interaction)
               && target.equals(event.target)
               && publisherHandle.equals(event.producer)
               && event.parameters.size() == 1
               && Arrays.equals(payload, event.parameters.get(ownerParameter))
               && Arrays.equals(tag, event.tag)
               && grantedTime.equals(event.time)
               && event.sentOrder == OrderType.TIMESTAMP
               && event.receivedOrder == OrderType.TIMESTAMP
               && retraction.handle.equals(event.retraction)
               && grantedTime.equals(ownerEvents.grants.get(0)),
            "Directed zero-lookahead callback changed target, payload, metadata, or order");
         if (callbackModel == CallbackModel.HLA_EVOKED) publisher.evokeCallback(0.0);
         JavaTckSupport.check(publisherEvents.receiveOrderOverloadCount == 0
               && publisherEvents.interactions.isEmpty(),
            "Directed zero-lookahead sender received its own interaction");

         owner.disableTimeConstrained();
         publisher.disableTimeRegulation();
         owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         ownerJoined = false;
         publisher.resignFederationExecution(ResignAction.NO_ACTION);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
            catch (Exception ignored) { }
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (ownerConnected) {
            try { owner.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime<?, ?> offset(LogicalTime start,
         LogicalTimeInterval interval, int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.0);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static final class Directed {
      private final InteractionClassHandle interaction;
      private final ObjectInstanceHandle target;
      private final ParameterHandleValueMap parameters;
      private final byte[] tag;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;
      private final FederateHandle producer;
      private final MessageRetractionHandle retraction;

      private Directed(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         target = (ObjectInstanceHandle) arguments[1];
         parameters = ((ParameterHandleValueMap) arguments[2]).clone();
         tag = ((byte[]) arguments[3]).clone();
         time = (LogicalTime<?, ?>) arguments[6];
         sentOrder = (OrderType) arguments[7];
         receivedOrder = (OrderType) arguments[8];
         producer = (FederateHandle) arguments[5];
         retraction = (MessageRetractionHandle) arguments[9];
      }
   }

   private static final class Recorder {
      private volatile int constrainedEnabled;
      private volatile int regulationEnabled;
      private volatile int receiveOrderOverloadCount;
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Directed> interactions = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeConstrainedEnabled".equals(name)) constrainedEnabled++;
               else if ("timeRegulationEnabled".equals(name)) regulationEnabled++;
               else if ("discoverObjectInstance".equals(name)
                     && arguments != null && arguments.length > 0) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  callbackOrder.add("grant");
                  grants.add((LogicalTime<?, ?>) arguments[0]);
               } else if ("receiveDirectedInteraction".equals(name)
                     && arguments != null && arguments.length == 6) {
                  receiveOrderOverloadCount++;
               } else if ("receiveDirectedInteraction".equals(name)
                     && arguments != null && arguments.length == 10) {
                  Directed event = new Directed(arguments);
                  callbackOrder.add("directed");
                  interactions.add(event);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK directed zero-grant callbacks";
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
