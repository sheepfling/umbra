package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
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

/** Public-API witness for queued and subsequent TSO-to-RO delivery after disabling Time Constrained. */
final class TimeConstrainedDisableQueuedInteractionTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private TimeConstrainedDisableQueuedInteractionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      String federation = "java-tck-time-constrained-disable-interaction-"
         + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverCallbacks.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         publisher.joinFederationExecution("java-tck-disable-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-disable-receiver", federation);
         receiverJoined = true;

         InteractionClassHandle publishedClass =
            publisher.getInteractionClassHandle(interactionName);
         InteractionClassHandle subscribedClass =
            receiver.getInteractionClassHandle(interactionName);
         ParameterHandle parameter = publisher.getParameterHandle(
            publishedClass, parameterName);
         JavaTckSupport.check(publishedClass != null && subscribedClass != null
               && publishedClass.equals(subscribedClass) && parameter != null,
            "Time Constrained disable adapter FOM did not resolve interaction handles");
         publisher.publishInteractionClass(publishedClass);
         publisher.changeInteractionOrderType(publishedClass, OrderType.TIMESTAMP);
         receiver.subscribeInteractionClass(subscribedClass);

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         JavaTckSupport.check(publisherFactory.getName().equals(receiverFactory.getName()),
            "Time Constrained disable members selected different time factories");
         LogicalTime<?, ?> initial = publisher.queryLogicalTime();
         LogicalTimeInterval epsilon =
            (LogicalTimeInterval) publisherFactory.makeEpsilon();
         LogicalTime<?, ?> queuedTime = offset(initial, epsilon, 5);
         LogicalTime<?, ?> laterTime = offset(initial, epsilon, 3);
         publisher.enableTimeRegulation(epsilon);
         await(publisher, callbackModel,
            () -> publisherCallbacks.regulationEnabled == 1,
            "publisher Time Regulation Enabled");
         receiver.enableTimeConstrained();
         await(receiver, callbackModel,
            () -> receiverCallbacks.constrainedEnabled == 1,
            "receiver Time Constrained Enabled");

         byte[] payload = {0x54, 0x44};
         byte[] queuedTag = {0x51};
         byte[] laterTag = {0x53};
         ParameterHandleValueMap values =
            publisher.getParameterHandleValueMapFactory().create(1);
         values.put(parameter, payload.clone());
         checkRetraction(publisher.sendInteraction(publishedClass, values,
            queuedTag.clone(), queuedTime), "queued timestamped interaction");
         if (callbackModel == CallbackModel.HLA_EVOKED) receiver.evokeCallback(0.0);
         JavaTckSupport.check(receiverCallbacks.untimed.isEmpty()
               && receiverCallbacks.timed.isEmpty(),
            "future timestamped interaction escaped before Time Constrained was disabled");

         receiver.disableTimeConstrained();
         await(receiver, callbackModel,
            () -> receiverCallbacks.untimed.size() == 1,
            "queued timestamped interaction as receive order after disable");
         Delivery queued = receiverCallbacks.untimed.get(0);
         JavaTckSupport.check(receiverCallbacks.timed.isEmpty()
               && queued.interaction.equals(subscribedClass)
               && queued.values.size() == 1
               && Arrays.equals(payload, queued.values.get(parameter))
               && Arrays.equals(queuedTag, queued.tag),
            "queued interaction did not retain its identity and payload in receive order");

         checkRetraction(publisher.sendInteraction(publishedClass, values,
            laterTag.clone(), laterTime), "subsequent timestamped interaction");
         await(receiver, callbackModel,
            () -> receiverCallbacks.timed.size() == 1,
            "subsequent timestamped interaction after disable");
         Delivery subsequent = receiverCallbacks.timed.get(0);
         JavaTckSupport.check(receiverCallbacks.untimed.size() == 1
               && subsequent.interaction.equals(subscribedClass)
               && subsequent.values.size() == 1
               && Arrays.equals(payload, subsequent.values.get(parameter))
               && Arrays.equals(laterTag, subsequent.tag)
               && laterTime.equals(subsequent.time)
               && subsequent.sentOrder == OrderType.TIMESTAMP
               && subsequent.receivedOrder == OrderType.RECEIVE,
            "subsequent timestamped interaction did not retain receive-order delivery");

         publisher.disableTimeRegulation();
         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         publisher.resignFederationExecution(ResignAction.NO_ACTION);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (receiverJoined) resign(receiver);
         if (publisherJoined) resign(publisher);
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (receiverConnected) disconnect(receiver);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static void checkRetraction(MessageRetractionReturn result, String phase) {
      JavaTckSupport.check(result != null && result.retractionHandleIsValid
            && result.handle != null, phase + " returned no valid retraction handle");
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime offset(LogicalTime start,
         LogicalTimeInterval interval, int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) {
         result = (LogicalTime) result.add(interval);
      }
      return result;
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.0);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static final class Delivery {
      private final InteractionClassHandle interaction;
      private final ParameterHandleValueMap values;
      private final byte[] tag;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;

      private Delivery(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         values = ((ParameterHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
         if (arguments.length == 10) {
            sentOrder = (OrderType) arguments[3];
            time = (LogicalTime<?, ?>) arguments[6];
            receivedOrder = (OrderType) arguments[8];
         } else {
            sentOrder = null;
            time = null;
            receivedOrder = OrderType.RECEIVE;
         }
      }
   }

   private static final class Recorder {
      private volatile int regulationEnabled;
      private volatile int constrainedEnabled;
      private final List<Delivery> untimed = new CopyOnWriteArrayList<>();
      private final List<Delivery> timed = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) regulationEnabled++;
               else if ("timeConstrainedEnabled".equals(name)) constrainedEnabled++;
               else if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length == 6) {
                  untimed.add(new Delivery(arguments));
               } else if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length == 10) {
                  timed.add(new Delivery(arguments));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK Time Constrained disable callbacks";
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
