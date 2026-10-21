package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionReturn;
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

/** Checks the zero-lookahead timestamped-send cutoff while TAR is pending. */
final class TimestampedInteractionZeroLookaheadTarSendBoundaryTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private TimestampedInteractionZeroLookaheadTarSendBoundaryTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "zero-lookahead pending-TAR boundary requires evoked callbacks");
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      String federation = "java-tck-zero-lookahead-tar-send-boundary-"
         + UUID.randomUUID();
      boolean publisherConnected = false, receiverConnected = false;
      boolean created = false, publisherJoined = false, receiverJoined = false;
      boolean published = false, subscribed = false;
      boolean regulating = false, constrained = false;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverCallbacks.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         publisher.joinFederationExecution("java-tck-zero-tar-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-zero-tar-receiver", federation);
         receiverJoined = true;

         InteractionClassHandle publishedClass =
            publisher.getInteractionClassHandle(interactionName);
         InteractionClassHandle subscribedClass =
            receiver.getInteractionClassHandle(interactionName);
         ParameterHandle publishedParameter = publisher.getParameterHandle(
            publishedClass, parameterName);
         ParameterHandle subscribedParameter = receiver.getParameterHandle(
            subscribedClass, parameterName);
         JavaTckSupport.check(publishedClass != null && subscribedClass != null
               && publishedParameter != null && subscribedParameter != null
               && publishedClass.equals(subscribedClass)
               && publishedParameter.equals(subscribedParameter),
            "adapter FOM returned inconsistent interaction or parameter handles");
         publisher.publishInteractionClass(publishedClass);
         published = true;
         publisher.changeInteractionOrderType(publishedClass, OrderType.TIMESTAMP);
         receiver.subscribeInteractionClass(subscribedClass);
         subscribed = true;

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         JavaTckSupport.check(publisherFactory.getName().equals(receiverFactory.getName()),
            "members selected different logical-time factories");
         LogicalTime<?, ?> initial = publisher.queryLogicalTime();
         JavaTckSupport.check(initial.equals(receiver.queryLogicalTime()),
            "members began at different logical times");
         LogicalTimeInterval zero = (LogicalTimeInterval) publisherFactory.makeZero();
         LogicalTimeInterval epsilon = (LogicalTimeInterval) publisherFactory.makeEpsilon();

         receiver.enableTimeConstrained();
         constrained = true;
         await(receiver, () -> receiverCallbacks.constrainedEnabled == 1,
            "receiver Time Constrained callback");
         publisher.enableTimeRegulation(zero);
         regulating = true;
         await(publisher, () -> publisherCallbacks.regulationEnabled == 1,
            "publisher Time Regulation callback");

         LogicalTime<?, ?> requestedTime = offset(initial, epsilon, 5);
         LogicalTime<?, ?> futureTime = offset(initial, epsilon, 6);
         byte[] payload = {(byte) 0xb7, 0x05};
         byte[] tag = {(byte) 0xb7, 0x06};
         ParameterHandleValueMap values =
            publisher.getParameterHandleValueMapFactory().create(1);
         values.put(publishedParameter, payload.clone());

         publisher.timeAdvanceRequest(requestedTime);
         boolean boundaryRejected = false;
         try {
            publisher.sendInteraction(publishedClass, values, tag.clone(), requestedTime);
         } catch (InvalidLogicalTime expected) {
            boundaryRejected = true;
         }
         JavaTckSupport.check(boundaryRejected,
            "zero-lookahead pending TAR accepted a send at its requested time");
         MessageRetractionReturn future = publisher.sendInteraction(
            publishedClass, values, tag.clone(), futureTime);
         JavaTckSupport.check(future != null && future.retractionHandleIsValid
               && future.handle != null,
            "zero-lookahead pending TAR rejected a send strictly after its requested time");
         JavaTckSupport.check(publisherCallbacks.grants.isEmpty(),
            "TAR grant callback was serviced before explicit evocation");
         await(publisher, () -> publisherCallbacks.grants.size() == 1,
            "publisher TAR grant");
         JavaTckSupport.check(requestedTime.equals(publisherCallbacks.grants.get(0)),
            "publisher received the wrong TAR grant time");
         JavaTckSupport.check(receiverCallbacks.interactions == 0,
            "receiver got the future interaction before requesting its time");

         receiver.disableTimeConstrained();
         constrained = false;
         publisher.disableTimeRegulation();
         regulating = false;
         receiver.unsubscribeInteractionClass(subscribedClass);
         subscribed = false;
         publisher.unpublishInteractionClass(publishedClass);
         published = false;
         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         publisher.resignFederationExecution(ResignAction.NO_ACTION);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (constrained && receiverJoined) {
            try { receiver.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (regulating && publisherJoined) {
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (subscribed && receiverJoined) {
            try {
               receiver.unsubscribeInteractionClass(
                  receiver.getInteractionClassHandle(interactionName));
            } catch (Exception ignored) { }
         }
         if (published && publisherJoined) {
            try {
               publisher.unpublishInteractionClass(
                  publisher.getInteractionClassHandle(interactionName));
            } catch (Exception ignored) { }
         }
         if (receiverJoined) {
            try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
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
         if (receiverConnected) {
            try { receiver.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime offset(LogicalTime start,
         LogicalTimeInterval epsilon, int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) {
         result = (LogicalTime) result.add(epsilon);
      }
      return result;
   }

   private static void await(RTIambassador ambassador,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         ambassador.evokeCallback(0.0);
      }
      JavaTckSupport.check(condition.getAsBoolean(),
         "timed out waiting for " + description);
   }

   private static final class Recorder {
      private int constrainedEnabled;
      private int regulationEnabled;
      private int interactions;
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();

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
               } else if ("receiveInteraction".equals(name)) interactions++;
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK zero-lookahead TAR callbacks";
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
