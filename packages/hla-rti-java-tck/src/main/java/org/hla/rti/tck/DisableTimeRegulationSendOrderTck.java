package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.UUID;
import java.util.function.BooleanSupplier;

/** Post-disable send-order observation through the standard 2025 Java API. */
final class DisableTimeRegulationSendOrderTck {
   private DisableTimeRegulationSendOrderTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionName, CallbackModel model) throws Exception {
      JavaTckSupport.check(model == CallbackModel.HLA_IMMEDIATE,
         "post-disable send-order case is scoped to HLA_IMMEDIATE");
      RTIambassador sender = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder senderCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      String federation = "java-tck-disable-regulation-order-" + UUID.randomUUID();
      boolean senderConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean senderJoined = false;
      boolean receiverJoined = false;
      try {
         sender.connect(senderCallbacks.proxy(), model);
         senderConnected = true;
         receiver.connect(receiverCallbacks.proxy(), model);
         receiverConnected = true;
         sender.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         sender.joinFederationExecution("post-disable-sender", federation);
         senderJoined = true;
         receiver.joinFederationExecution("post-disable-receiver", federation);
         receiverJoined = true;

         InteractionClassHandle sentClass = sender.getInteractionClassHandle(interactionName);
         InteractionClassHandle receivedClass = receiver.getInteractionClassHandle(interactionName);
         sender.publishInteractionClass(sentClass);
         sender.changeInteractionOrderType(sentClass, OrderType.TIMESTAMP);
         receiver.subscribeInteractionClass(receivedClass);

         LogicalTimeFactory<?, ?> timeFactory = sender.getTimeFactory();
         LogicalTimeInterval<?> epsilon = timeFactory.makeEpsilon();
         LogicalTime<?, ?> stamp = afterEpsilon(sender.queryLogicalTime(), epsilon);
         sender.enableTimeRegulation(epsilon);
         await(() -> senderCallbacks.regulationEnabled == 1,
            "Time Regulation Enabled callback");
         sender.disableTimeRegulation();

         sender.sendInteraction(sentClass,
            sender.getParameterHandleValueMapFactory().create(0), new byte[0], stamp);
         await(() -> receiverCallbacks.deliveryCount == 1,
            "post-disable interaction delivery");
         JavaTckSupport.check(receiverCallbacks.deliveryCount == 1
               && receivedClass.equals(receiverCallbacks.deliveredClass),
            "post-disable timestamped send produced the wrong interaction delivery");
         if (receiverCallbacks.timedDelivery) {
            JavaTckSupport.check(receiverCallbacks.sentOrder == OrderType.RECEIVE
                  && receiverCallbacks.receivedOrder == OrderType.RECEIVE,
               "post-disable timestamped callback did not report receive order");
         }

         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         sender.resignFederationExecution(ResignAction.NO_ACTION);
         senderJoined = false;
         sender.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (receiverJoined) {
            try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (senderJoined) {
            try { sender.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { sender.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (receiverConnected) {
            try { receiver.disconnect(); } catch (Exception ignored) { }
         }
         if (senderConnected) {
            try { sender.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime<?, ?> afterEpsilon(LogicalTime initial,
         LogicalTimeInterval epsilon) throws Exception {
      return (LogicalTime<?, ?>) initial.add(epsilon);
   }

   private static void await(BooleanSupplier ready, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (!ready.getAsBoolean() && System.nanoTime() < deadline) Thread.sleep(1L);
      JavaTckSupport.check(ready.getAsBoolean(), "timed out waiting for " + description);
   }

   private static final class Recorder {
      private volatile int regulationEnabled;
      private volatile int deliveryCount;
      private volatile InteractionClassHandle deliveredClass;
      private volatile boolean timedDelivery;
      private volatile OrderType sentOrder;
      private volatile OrderType receivedOrder;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("timeRegulationEnabled".equals(method.getName())) ++regulationEnabled;
               else if ("receiveInteraction".equals(method.getName())) {
                  deliveredClass = (InteractionClassHandle) arguments[0];
                  timedDelivery = arguments.length >= 10;
                  if (timedDelivery) {
                     sentOrder = (OrderType) arguments[7];
                     receivedOrder = (OrderType) arguments[8];
                  }
                  ++deliveryCount;
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(method.getName())) return "Java TCK post-disable order";
                  if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
                  if ("equals".equals(method.getName())) return proxy == arguments[0];
               }
               return null;
            });
      }
   }
}
