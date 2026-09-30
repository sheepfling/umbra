package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;

/** Verifies withdrawal cancels only a receive-order interaction still pending delivery. */
final class ReceiveOrderInteractionCallbackCancellationTck {
   private ReceiveOrderInteractionCallbackCancellationTck() { }

   static void run(RtiFactory factory, String fom, String interactionName,
         String parameterName, CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador active = factory.getRtiAmbassador();
      RTIambassador cancelled = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder activeRecorder = new Recorder();
      Recorder cancelledRecorder = new Recorder();
      String federation = "java-tck-receive-order-interaction-cancel-" + UUID.randomUUID();
      boolean publisherConnected = false, activeConnected = false, cancelledConnected = false;
      boolean created = false, publisherJoined = false, activeJoined = false, cancelledJoined = false;
      boolean activeSubscribed = false, cancelledSubscribed = false, published = false;
      InteractionClassHandle publisherInteraction = null;
      InteractionClassHandle activeInteraction = null;
      InteractionClassHandle cancelledInteraction = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         active.connect(activeRecorder.proxy(), callbackModel);
         activeConnected = true;
         cancelled.connect(cancelledRecorder.proxy(), callbackModel);
         cancelledConnected = true;
         publisher.createFederationExecution(federation, fom);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-interaction-cancel-publisher", federation);
         publisherJoined = true;
         active.joinFederationExecution("java-tck-interaction-cancel-active", federation);
         activeJoined = true;
         cancelled.joinFederationExecution("java-tck-interaction-cancel-pending", federation);
         cancelledJoined = true;

         publisherInteraction = publisher.getInteractionClassHandle(interactionName);
         activeInteraction = active.getInteractionClassHandle(interactionName);
         cancelledInteraction = cancelled.getInteractionClassHandle(interactionName);
         ParameterHandle publisherParameter =
            publisher.getParameterHandle(publisherInteraction, parameterName);
         ParameterHandle activeParameter =
            active.getParameterHandle(activeInteraction, parameterName);
         ParameterHandle cancelledParameter =
            cancelled.getParameterHandle(cancelledInteraction, parameterName);

         publisher.publishInteractionClass(publisherInteraction);
         published = true;
         active.subscribeInteractionClass(activeInteraction);
         activeSubscribed = true;
         cancelled.subscribeInteractionClass(cancelledInteraction);
         cancelledSubscribed = true;

         byte[] payload = new byte[] {0x49, 0x4e, 0x54};
         byte[] tag = new byte[] {0x43, 0x41, 0x4e};
         ParameterHandleValueMap parameters =
            publisher.getParameterHandleValueMapFactory().create(1);
         parameters.put(publisherParameter, payload);
         publisher.sendInteraction(publisherInteraction, parameters, tag);

         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(activeRecorder.interactions.isEmpty()
                  && cancelledRecorder.interactions.isEmpty(),
               "evoked receive-order interactions must remain pending until callback servicing");
            cancelled.unsubscribeInteractionClass(cancelledInteraction);
            cancelledSubscribed = false;
            awaitCount(active, activeRecorder, 1,
               "active evoked receive-order interaction did not arrive before deadline");
            for (int pass = 0; pass < 8; ++pass) JavaTckSupport.drain(cancelled);
            JavaTckSupport.check(cancelledRecorder.interactions.isEmpty(),
               "withdrawal did not cancel the pending evoked interaction callback");
         } else {
            awaitBoth(active, cancelled, activeRecorder, cancelledRecorder);
            cancelled.unsubscribeInteractionClass(cancelledInteraction);
            cancelledSubscribed = false;
         }

         assertInteraction(activeRecorder.interactions.get(0), activeInteraction,
            activeParameter, payload, tag, producer, active, "active subscriber");
         if (callbackModel == CallbackModel.HLA_IMMEDIATE) {
            assertInteraction(cancelledRecorder.interactions.get(0), cancelledInteraction,
               cancelledParameter, payload, tag, producer, cancelled, "immediate subscriber");
         } else {
            JavaTckSupport.check(cancelledRecorder.interactions.isEmpty(),
               "cancelled subscriber received a stale evoked interaction");
         }

         active.unsubscribeInteractionClass(activeInteraction);
         activeSubscribed = false;
         publisher.unpublishInteractionClass(publisherInteraction);
         published = false;
      } finally {
         if (cancelledSubscribed && cancelledJoined) {
            try {
               cancelled.unsubscribeInteractionClass(cancelledInteraction);
            } catch (Exception ignored) { }
         }
         if (activeSubscribed && activeJoined) {
            try { active.unsubscribeInteractionClass(activeInteraction); }
            catch (Exception ignored) { }
         }
         if (published && publisherJoined) {
            try { publisher.unpublishInteractionClass(publisherInteraction); }
            catch (Exception ignored) { }
         }
         if (cancelledJoined) {
            try { cancelled.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (activeJoined) {
            try { active.resignFederationExecution(ResignAction.NO_ACTION); }
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
         if (cancelledConnected) {
            try { cancelled.disconnect(); } catch (Exception ignored) { }
         }
         if (activeConnected) {
            try { active.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void awaitBoth(RTIambassador active, RTIambassador cancelled,
         Recorder activeRecorder, Recorder cancelledRecorder) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline
            && (activeRecorder.interactions.isEmpty()
               || cancelledRecorder.interactions.isEmpty())) {
         JavaTckSupport.drain(active);
         JavaTckSupport.drain(cancelled);
         Thread.yield();
      }
      JavaTckSupport.check(!activeRecorder.interactions.isEmpty()
            && !cancelledRecorder.interactions.isEmpty(),
         "immediate interaction callbacks did not arrive before the bounded delivery deadline");
   }

   private static void awaitCount(RTIambassador ambassador, Recorder recorder, int count,
         String message) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && recorder.interactions.size() < count) {
         JavaTckSupport.drain(ambassador);
         Thread.yield();
      }
      JavaTckSupport.check(recorder.interactions.size() >= count, message);
   }

   private static void assertInteraction(Interaction event,
         InteractionClassHandle interaction, ParameterHandle parameter, byte[] payload,
         byte[] tag, FederateHandle producer, RTIambassador inspector, String description)
         throws Exception {
      JavaTckSupport.check(interaction.equals(event.interaction)
            && event.parameters.size() == 1 && event.parameters.containsKey(parameter)
            && Arrays.equals(payload, event.parameters.get(parameter))
            && Arrays.equals(tag, event.tag) && producer.equals(event.producer)
            && event.transportation != null
            && "HLAreliable".equals(inspector.getTransportationTypeName(event.transportation))
            && event.regionsEmpty,
         description + " interaction callback lost class, parameter, payload, tag, producer, "
            + "reliable transport, or region metadata");
   }

   private static final class Interaction {
      private InteractionClassHandle interaction;
      private ParameterHandleValueMap parameters;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private boolean regionsEmpty;
   }

   private static final class Recorder {
      private final List<Interaction> interactions = new ArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length == 6) {
                  Interaction event = new Interaction();
                  event.interaction = (InteractionClassHandle) arguments[0];
                  event.parameters = ((ParameterHandleValueMap) arguments[1]).clone();
                  event.tag = ((byte[]) arguments[2]).clone();
                  event.transportation = (TransportationTypeHandle) arguments[3];
                  event.producer = (FederateHandle) arguments[4];
                  event.regionsEmpty = arguments[5] == null
                     || ((RegionHandleSet) arguments[5]).isEmpty();
                  interactions.add(event);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK receive-order interaction cancellation";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
