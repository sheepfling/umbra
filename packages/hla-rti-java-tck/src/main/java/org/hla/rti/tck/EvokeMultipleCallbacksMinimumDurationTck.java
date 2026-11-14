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
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.atomic.AtomicReference;

/** Checks the minimum evoked callback interval and completion when no callbacks remain. */
final class EvokeMultipleCallbacksMinimumDurationTck {
   private static final long MINIMUM_NANOS = 200_000_000L;
   private static final long EARLY_RETURN_TOLERANCE_NANOS = 20_000_000L;
   private static final long COMPLETION_TOLERANCE_NANOS = 150_000_000L;

   private EvokeMultipleCallbacksMinimumDurationTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-evoke-minimum-" + UUID.randomUUID();
      String publisherName = "java-tck-evoke-min-publisher-" + UUID.randomUUID();
      String receiverName = "java-tck-evoke-min-receiver-" + UUID.randomUUID();
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

         byte[][] payloads = {
            {0x45, 0x31}, {0x45, 0x32}, {0x45, 0x33}
         };
         byte[][] tags = {
            {0x54, 0x31}, {0x54, 0x32}, {0x54, 0x33}
         };
         CountDownLatch beginSending = new CountDownLatch(1);
         AtomicReference<Throwable> sendFailure = new AtomicReference<>();
         InteractionClassHandle senderInteraction = publisherInteraction;
         Thread sender = new Thread(() -> {
            try {
               beginSending.await();
               for (int index = 0; index < payloads.length; ++index) {
                  if (index != 0) Thread.sleep(35L);
                  ParameterHandleValueMap parameters =
                     publisher.getParameterHandleValueMapFactory().create(1);
                  parameters.put(publisherParameter, payloads[index]);
                  publisher.sendInteraction(senderInteraction, parameters, tags[index]);
               }
            } catch (Throwable failure) {
               sendFailure.set(failure);
            }
         }, "java-tck-evoke-minimum-sender");
         sender.start();

         long evokeStarted = System.nanoTime();
         beginSending.countDown();
         boolean callbacksRemain = false;
         Throwable evokeFailure = null;
         long elapsedNanos = 0L;
         try {
            callbacksRemain = receiver.evokeMultipleCallbacks(0.20, 0.75);
         } catch (Throwable failure) {
            evokeFailure = failure;
         } finally {
            elapsedNanos = System.nanoTime() - evokeStarted;
            joinUninterruptibly(sender);
         }

         rethrow(sendFailure.get(), "sending the receive-order interactions failed");
         rethrow(evokeFailure, "Evoke Multiple Callbacks failed");
         JavaTckSupport.check(elapsedNanos + EARLY_RETURN_TOLERANCE_NANOS >= MINIMUM_NANOS,
            "Evoke Multiple Callbacks returned materially before its approximate minimum "
               + "wall-clock duration");
         JavaTckSupport.check(elapsedNanos <= MINIMUM_NANOS + COMPLETION_TOLERANCE_NANOS,
            "Evoke Multiple Callbacks did not complete near its minimum when no additional "
               + "callbacks remained");
         JavaTckSupport.check(!callbacksRemain,
            "Evoke Multiple Callbacks reported pending callbacks after draining the test sequence");

         List<Delivery> received = receiverRecorder.deliveries;
         JavaTckSupport.check(received.size() == payloads.length,
            "Evoke Multiple Callbacks processed " + received.size() + " of "
               + payloads.length + " offered receive-order callbacks");
         for (int index = 0; index < received.size(); ++index) {
            Delivery delivery = received.get(index);
            JavaTckSupport.check(receiverInteraction.equals(delivery.interaction)
                  && publisherHandle.equals(delivery.producer)
                  && delivery.parameters.size() == 1
                  && delivery.parameters.containsKey(receiverParameter)
                  && Arrays.equals(payloads[index], delivery.parameters.get(receiverParameter))
                  && Arrays.equals(tags[index], delivery.tag),
               "Evoke Multiple Callbacks changed the order or contents of an offered callback");
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

   private static void rethrow(Throwable failure, String message) throws Exception {
      if (failure == null) return;
      if (failure instanceof Exception) throw (Exception) failure;
      if (failure instanceof Error) throw (Error) failure;
      throw new AssertionError(message, failure);
   }

   private static void joinUninterruptibly(Thread thread) {
      boolean interrupted = false;
      while (thread.isAlive()) {
         try {
            thread.join();
         } catch (InterruptedException failure) {
            interrupted = true;
         }
      }
      if (interrupted) Thread.currentThread().interrupt();
   }

   private static FederateAmbassador emptyCallbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> objectMethod(proxy, method.getName(), arguments));
   }

   private static Object objectMethod(Object proxy, String name, Object[] arguments) {
      if ("toString".equals(name)) return "Java TCK minimum-duration publisher callbacks";
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
                  deliveries.add(new Delivery(arguments));
               }
               return objectMethod(proxy, name, arguments);
            });
      }
   }
}
