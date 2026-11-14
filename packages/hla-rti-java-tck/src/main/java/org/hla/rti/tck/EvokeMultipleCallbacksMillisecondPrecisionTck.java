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
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Checks millisecond-scale minimum and maximum Evoke Multiple Callbacks arguments. */
final class EvokeMultipleCallbacksMillisecondPrecisionTck {
   private static final int TIMING_SAMPLES = 30;
   private static final int CALLBACK_COUNT = 120;
   private static final long CALLBACK_WORK_NANOS = 250_000L;

   private EvokeMultipleCallbacksMillisecondPrecisionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-evoke-millisecond-" + UUID.randomUUID();
      String publisherName = "java-tck-evoke-ms-publisher-" + UUID.randomUUID();
      String receiverName = "java-tck-evoke-ms-receiver-" + UUID.randomUUID();
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

         long zeroWaitTotal = 0L;
         long oneMillisecondTotal = 0L;
         long fourMillisecondTotal = 0L;
         for (int sample = 0; sample < TIMING_SAMPLES; ++sample) {
            double[] waits = sample % 2 == 0
               ? new double[] {0.0, 0.001, 0.004}
               : new double[] {0.004, 0.001, 0.0};
            for (double seconds : waits) {
               long elapsed = measureEmptyQueueCall(receiver, seconds);
               if (seconds == 0.0) {
                  zeroWaitTotal += elapsed;
               } else if (seconds == 0.001) {
                  oneMillisecondTotal += elapsed;
               } else {
                  fourMillisecondTotal += elapsed;
               }
            }
         }
         long oneMillisecondIncrement = oneMillisecondTotal - zeroWaitTotal;
         long fourMillisecondIncrement = fourMillisecondTotal - oneMillisecondTotal;
         JavaTckSupport.check(oneMillisecondIncrement >= 15_000_000L,
            "a 1 ms minimum wait was not distinguishable from a zero wait");
         JavaTckSupport.check(fourMillisecondIncrement >= 40_000_000L,
            "1-to-4 ms minimum-wait delta was only "
               + (fourMillisecondIncrement / 1_000L) + " us across " + TIMING_SAMPLES
               + " samples (zero=" + (zeroWaitTotal / 1_000L) + " us, one="
               + (oneMillisecondTotal / 1_000L) + " us, four="
               + (fourMillisecondTotal / 1_000L) + " us)");

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
            payloads[index] = new byte[] {0x50, high, low};
            tags[index] = new byte[] {0x54, high, low};
            ParameterHandleValueMap parameters =
               publisher.getParameterHandleValueMapFactory().create(1);
            parameters.put(publisherParameter, payloads[index]);
            publisher.sendInteraction(publisherInteraction, parameters, tags[index]);
         }
         Thread.sleep(25L);

         long firstMaximumStarted = System.nanoTime();
         boolean firstHasPending = receiver.evokeMultipleCallbacks(0.0, 0.001);
         long firstMaximumElapsed = System.nanoTime() - firstMaximumStarted;
         List<Delivery> afterOneMillisecondMaximum =
            new ArrayList<>(receiverRecorder.deliveries);
         JavaTckSupport.check(firstHasPending,
            "the 1 ms maximum probe did not leave the preloaded callback backlog pending");
         JavaTckSupport.check(!afterOneMillisecondMaximum.isEmpty(),
            "the 1 ms maximum probe did not invoke any queued callback");

         long secondMaximumStarted = System.nanoTime();
         receiver.evokeMultipleCallbacks(0.0, 0.004);
         long secondMaximumElapsed = System.nanoTime() - secondMaximumStarted;
         List<Delivery> afterFourMillisecondMaximum =
            new ArrayList<>(receiverRecorder.deliveries);
         int fourMillisecondCallbackCount =
            afterFourMillisecondMaximum.size() - afterOneMillisecondMaximum.size();
         JavaTckSupport.check(fourMillisecondCallbackCount >= 2,
            "increasing the maximum wait from 1 ms to 4 ms did not admit additional callbacks");
         JavaTckSupport.check(fourMillisecondCallbackCount <= 24,
            "the 4 ms maximum wait behaved like a substantially coarser timing interval");
         JavaTckSupport.check(firstMaximumElapsed < 30_000_000L
               && secondMaximumElapsed < 30_000_000L,
            "millisecond-scale maximum arguments were interpreted as a coarse wait");

         for (int index = 0; index < afterFourMillisecondMaximum.size(); ++index) {
            Delivery delivery = afterFourMillisecondMaximum.get(index);
            JavaTckSupport.check(receiverInteraction.equals(delivery.interaction)
                  && publisherHandle.equals(delivery.producer)
                  && delivery.parameters.size() == 1
                  && delivery.parameters.containsKey(receiverParameter)
                  && Arrays.equals(payloads[index], delivery.parameters.get(receiverParameter))
                  && Arrays.equals(tags[index], delivery.tag),
               "the millisecond timing probes changed queued callback order or contents");
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

   private static long measureEmptyQueueCall(RTIambassador receiver, double seconds)
         throws Exception {
      long started = System.nanoTime();
      boolean callbacksRemain = receiver.evokeMultipleCallbacks(seconds, seconds);
      long elapsed = System.nanoTime() - started;
      JavaTckSupport.check(!callbacksRemain,
         "millisecond timing probe unexpectedly found a callback");
      return elapsed;
   }

   private static FederateAmbassador emptyCallbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> objectMethod(proxy, method.getName(), arguments));
   }

   private static Object objectMethod(Object proxy, String name, Object[] arguments) {
      if ("toString".equals(name)) return "Java TCK millisecond precision publisher callbacks";
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
                  long callbackWorkDeadline = System.nanoTime() + CALLBACK_WORK_NANOS;
                  while (System.nanoTime() < callbackWorkDeadline) Thread.onSpinWait();
                  deliveries.add(new Delivery(arguments));
               }
               return objectMethod(proxy, name, arguments);
            });
      }
   }
}
