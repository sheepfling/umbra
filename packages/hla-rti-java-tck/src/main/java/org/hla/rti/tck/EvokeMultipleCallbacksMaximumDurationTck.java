package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Checks continued evoked callback delivery through the maximum service window. */
final class EvokeMultipleCallbacksMaximumDurationTck {
   private static final int CALLBACK_COUNT = 120;
   private static final long MAXIMUM_NANOS = 500_000_000L;
   private static final long TIMING_TOLERANCE_NANOS = 100_000_000L;

   private EvokeMultipleCallbacksMaximumDurationTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-evoke-multiple-callbacks-maximum-" + UUID.randomUUID();
      String publisherName = "java-tck-evoke-max-publisher-" + UUID.randomUUID();
      String receiverName = "java-tck-evoke-max-receiver-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      boolean published = false;
      boolean subscribed = false;
      InteractionClassHandle publisherInteraction = null;
      InteractionClassHandle receiverInteraction = null;
      try {
         publisher.connect(emptyCallbacks(), CallbackModel.HLA_EVOKED);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), CallbackModel.HLA_EVOKED);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle publisherHandle =
            publisher.joinFederationExecution(publisherName, federation);
         publisherJoined = true;
         receiver.joinFederationExecution(receiverName, federation);
         receiverJoined = true;

         publisherInteraction = publisher.getInteractionClassHandle(interactionClassName);
         receiverInteraction = receiver.getInteractionClassHandle(interactionClassName);
         ParameterHandle publisherParameter =
            publisher.getParameterHandle(publisherInteraction, parameterName);
         ParameterHandle receiverParameter =
            receiver.getParameterHandle(receiverInteraction, parameterName);
         JavaTckSupport.check(publisherInteraction.equals(receiverInteraction)
               && publisherParameter.equals(receiverParameter),
            "publisher and receiver resolved different interaction handles");

         publisher.publishInteractionClass(publisherInteraction);
         published = true;
         receiver.subscribeInteractionClass(receiverInteraction);
         subscribed = true;

         byte[][] payloads = new byte[CALLBACK_COUNT][];
         byte[][] tags = new byte[CALLBACK_COUNT][];
         for (int index = 0; index < CALLBACK_COUNT; ++index) {
            byte high = (byte) ((index >>> 8) & 0xff);
            byte low = (byte) (index & 0xff);
            payloads[index] = new byte[] {0x45, high, low};
            tags[index] = new byte[] {0x54, high, low};
            ParameterHandleValueMap parameters =
               publisher.getParameterHandleValueMapFactory().create(1);
            parameters.put(publisherParameter, payloads[index]);
            publisher.sendInteraction(publisherInteraction, parameters, tags[index]);
         }

         // Let process-boundary events reach the receiver before evoking. The slow
         // callback keeps this finite backlog non-empty through the maximum window.
         Thread.sleep(50L);
         long evokeStarted = System.nanoTime();
         receiver.evokeMultipleCallbacks(0.15, 0.50);
         long elapsedNanos = System.nanoTime() - evokeStarted;
         JavaTckSupport.check(elapsedNanos + TIMING_TOLERANCE_NANOS >= MAXIMUM_NANOS,
            "Evoke Multiple Callbacks returned materially before its maximum interval "
               + "with callbacks pending");
         JavaTckSupport.check(elapsedNanos <= MAXIMUM_NANOS + TIMING_TOLERANCE_NANOS,
            "Evoke Multiple Callbacks substantially exceeded its approximate maximum interval");

         List<Delivery> received = receiverRecorder.deliveries;
         JavaTckSupport.check(received.size() >= 20,
            "Evoke Multiple Callbacks delivered only " + received.size()
               + " callbacks in " + (elapsedNanos / 1_000_000L)
               + " ms; expected continued delivery after the minimum interval");
         JavaTckSupport.check(received.size() <= CALLBACK_COUNT,
            "Evoke Multiple Callbacks delivered more callbacks than the test queued");
         for (int index = 0; index < received.size(); ++index) {
            Delivery delivery = received.get(index);
            JavaTckSupport.check(receiverInteraction.equals(delivery.interaction)
                  && publisherHandle.equals(delivery.producer)
                  && delivery.parameters.size() == 1
                  && delivery.parameters.containsKey(receiverParameter)
                  && Arrays.equals(payloads[index], delivery.parameters.get(receiverParameter))
                  && Arrays.equals(tags[index], delivery.tag),
               "Evoke Multiple Callbacks changed the order or contents of a queued "
                  + "receive-order callback");
         }

         receiver.unsubscribeInteractionClass(receiverInteraction);
         subscribed = false;
         publisher.unpublishInteractionClass(publisherInteraction);
         published = false;
      } finally {
         if (receiverJoined && subscribed && receiverInteraction != null) {
            try { receiver.unsubscribeInteractionClass(receiverInteraction); }
            catch (Exception ignored) { }
         }
         if (publisherJoined && published && publisherInteraction != null) {
            try { publisher.unpublishInteractionClass(publisherInteraction); }
            catch (Exception ignored) { }
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
            try { receiver.disconnect(); }
            catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static FederateAmbassador emptyCallbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> objectMethod(proxy, method.getName(), arguments));
   }

   private static Object objectMethod(Object proxy, String name, Object[] arguments) {
      if ("toString".equals(name)) return "Java TCK maximum-duration publisher callbacks";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
      return null;
   }

   private static final class Delivery {
      private final InteractionClassHandle interaction;
      private final ParameterHandleValueMap parameters;
      private final byte[] tag;
      private final FederateHandle producer;

      private Delivery(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         parameters = ((ParameterHandleValueMap) arguments[1]).clone();
         for (ParameterHandle parameter : parameters.keySet()) {
            byte[] value = parameters.get(parameter);
            if (value != null) parameters.put(parameter, value.clone());
         }
         tag = ((byte[]) arguments[2]).clone();
         producer = (FederateHandle) arguments[4];
      }
   }

   private static final class Recorder {
      private final List<Delivery> deliveries = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("receiveInteraction".equals(name)) {
                  JavaTckSupport.check(arguments != null && arguments.length == 6,
                     "receiveInteraction used an unexpected Java API overload");
                  Thread.sleep(10L);
                  deliveries.add(new Delivery(arguments));
               }
               return objectMethod(proxy, name, arguments);
            });
      }
   }
}
