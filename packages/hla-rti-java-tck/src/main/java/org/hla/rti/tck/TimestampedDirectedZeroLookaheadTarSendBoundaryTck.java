package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.exceptions.InvalidLogicalTime;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks the directed-send timestamp cutoff while a zero-lookahead TAR is pending. */
final class TimestampedDirectedZeroLookaheadTarSendBoundaryTck {
   private TimestampedDirectedZeroLookaheadTarSendBoundaryTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         String parameterName, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "directed zero-lookahead TAR boundary requires evoked callbacks");
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador owner = factory.getRtiAmbassador();
      Recorder publisherEvents = new Recorder();
      Recorder ownerEvents = new Recorder();
      String federation = "java-tck-directed-zero-lookahead-tar-" + UUID.randomUUID();
      boolean publisherConnected = false, ownerConnected = false;
      boolean publisherJoined = false, ownerJoined = false, created = false;
      boolean regulating = false, constrained = false;
      try {
         publisher.connect(publisherEvents.proxy(), callbackModel);
         publisherConnected = true;
         owner.connect(ownerEvents.proxy(), callbackModel);
         ownerConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         publisher.joinFederationExecution("java-tck-directed-zero-tar-sender", federation);
         publisherJoined = true;
         owner.joinFederationExecution("java-tck-directed-zero-tar-owner", federation);
         ownerJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute =
            publisher.getAttributeHandle(publisherClass, attributeName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         InteractionClassHandle publisherInteraction =
            publisher.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle ownerInteraction =
            owner.getInteractionClassHandle(interactionClassName);
         ParameterHandle publisherParameter =
            publisher.getParameterHandle(publisherInteraction, parameterName);
         ParameterHandle ownerParameter =
            owner.getParameterHandle(ownerInteraction, parameterName);
         JavaTckSupport.check(publisherClass != null && ownerClass != null
               && publisherAttribute != null && ownerAttribute != null
               && publisherInteraction != null && ownerInteraction != null
               && publisherParameter != null && ownerParameter != null,
            "directed zero-lookahead TAR adapter FOM returned null handles");

         owner.publishObjectClassAttributes(
            ownerClass, JavaTckSupport.attributeSet(owner, ownerAttribute));
         publisher.subscribeObjectClassAttributes(
            publisherClass, JavaTckSupport.attributeSet(publisher, publisherAttribute));
         publisher.publishObjectClassDirectedInteractions(publisherClass,
            JavaTckSupport.interactionSet(publisher, publisherInteraction));
         owner.subscribeObjectClassDirectedInteractions(ownerClass,
            JavaTckSupport.interactionSet(owner, ownerInteraction));
         publisher.changeInteractionOrderType(publisherInteraction, OrderType.TIMESTAMP);
         ObjectInstanceHandle target = owner.registerObjectInstance(ownerClass);
         await(publisher, () -> publisherEvents.discoveries.contains(target),
            "directed zero-lookahead TAR sender target discovery");

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> ownerFactory = owner.getTimeFactory();
         LogicalTime<?, ?> initial = publisher.queryLogicalTime();
         JavaTckSupport.check(publisherFactory.getName().equals(ownerFactory.getName())
               && initial.equals(owner.queryLogicalTime()),
            "directed zero-lookahead TAR members selected inconsistent time contexts");
         LogicalTimeInterval<?> zero =
            (LogicalTimeInterval<?>) publisherFactory.makeZero();
         LogicalTimeInterval<?> epsilon =
            (LogicalTimeInterval<?>) publisherFactory.makeEpsilon();
         owner.enableTimeConstrained();
         constrained = true;
         await(owner, () -> ownerEvents.constrainedEnabled == 1,
            "directed zero-lookahead TAR owner constrained callback");
         publisher.enableTimeRegulation(zero);
         regulating = true;
         await(publisher, () -> publisherEvents.regulationEnabled == 1,
            "directed zero-lookahead TAR sender regulation callback");

         LogicalTime<?, ?> requestedTime = offset(initial, epsilon, 5);
         LogicalTime<?, ?> futureTime = offset(initial, epsilon, 6);
         ParameterHandleValueMap values =
            publisher.getParameterHandleValueMapFactory().create(1);
         values.put(publisherParameter, new byte[] {(byte) 0xD7, 0x05});
         byte[] tag = {(byte) 0xD7, 0x06};
         publisher.timeAdvanceRequest(requestedTime);
         boolean boundaryRejected = false;
         try {
            publisher.sendDirectedInteraction(
               publisherInteraction, target, values, tag, requestedTime);
         } catch (InvalidLogicalTime expected) {
            boundaryRejected = true;
         }
         JavaTckSupport.check(boundaryRejected,
            "directed zero-lookahead TAR accepted a send at requested time");
         MessageRetractionReturn future = publisher.sendDirectedInteraction(
            publisherInteraction, target, values, tag, futureTime);
         JavaTckSupport.check(future != null && future.retractionHandleIsValid
               && future.handle != null,
            "directed zero-lookahead TAR rejected a send after requested time");
         JavaTckSupport.check(publisherEvents.grants.isEmpty(),
            "directed zero-lookahead TAR grant was serviced before explicit evocation");
         await(publisher, () -> publisherEvents.grants.size() == 1,
            "directed zero-lookahead TAR sender grant");
         JavaTckSupport.check(requestedTime.equals(publisherEvents.grants.get(0))
               && ownerEvents.directedInteractions == 0,
            "directed zero-lookahead TAR grant or pre-advance delivery was incorrect");

         owner.disableTimeConstrained();
         constrained = false;
         publisher.disableTimeRegulation();
         regulating = false;
         publisher.resignFederationExecution(ResignAction.NO_ACTION);
         publisherJoined = false;
         owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         ownerJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (constrained && ownerJoined) {
            try { owner.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (regulating && publisherJoined) {
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try {
               owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
            } catch (Exception ignored) { }
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
   private static LogicalTime<?, ?> offset(LogicalTime<?, ?> start,
         LogicalTimeInterval<?> epsilon, int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) result = result.add(epsilon);
      return result;
   }

   private static void await(RTIambassador ambassador,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         ambassador.evokeCallback(0.0);
      }
      JavaTckSupport.check(condition.getAsBoolean(),
         "timed out waiting for " + description);
   }

   private static final class Recorder {
      private int constrainedEnabled;
      private int regulationEnabled;
      private int directedInteractions;
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeConstrainedEnabled".equals(name)) constrainedEnabled++;
               else if ("timeRegulationEnabled".equals(name)) regulationEnabled++;
               else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
               } else if ("discoverObjectInstance".equals(name)
                     && arguments != null && arguments.length > 0) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("receiveDirectedInteraction".equals(name)) {
                  directedInteractions++;
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK directed zero-TAR callbacks";
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
