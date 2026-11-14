package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Exercises the active/passive interaction subscription delivery boundary. */
final class InteractionActivePassiveSubscriptionModesTck {
   private InteractionActivePassiveSubscriptionModesTck() { }

   static void run(RtiFactory factory, String fom, String interactionName,
         String parameterName, CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador activeReceiver = factory.getRtiAmbassador();
      RTIambassador passiveReceiver = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder activeRecorder = new Recorder();
      Recorder passiveRecorder = new Recorder();
      String federation = "java-tck-interaction-subscription-modes-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean activeConnected = false;
      boolean passiveConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean activeJoined = false;
      boolean passiveJoined = false;
      boolean published = false;
      boolean activeSubscribed = false;
      boolean passiveSubscribed = false;
      InteractionClassHandle publisherInteraction = null;
      InteractionClassHandle activeInteraction = null;
      InteractionClassHandle passiveInteraction = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         activeReceiver.connect(activeRecorder.proxy(), callbackModel);
         activeConnected = true;
         passiveReceiver.connect(passiveRecorder.proxy(), callbackModel);
         passiveConnected = true;

         publisher.createFederationExecution(federation, fom);
         created = true;
         FederateHandle publisherHandle = publisher.joinFederationExecution(
            "java-tck-interaction-subscription-publisher", federation);
         publisherJoined = true;
         activeReceiver.joinFederationExecution(
            "java-tck-interaction-subscription-active", federation);
         activeJoined = true;
         passiveReceiver.joinFederationExecution(
            "java-tck-interaction-subscription-passive", federation);
         passiveJoined = true;

         publisherInteraction = publisher.getInteractionClassHandle(interactionName);
         activeInteraction = activeReceiver.getInteractionClassHandle(interactionName);
         passiveInteraction = passiveReceiver.getInteractionClassHandle(interactionName);
         ParameterHandle publisherParameter = publisher.getParameterHandle(
            publisherInteraction, parameterName);
         ParameterHandle activeParameter = activeReceiver.getParameterHandle(
            activeInteraction, parameterName);
         ParameterHandle passiveParameter = passiveReceiver.getParameterHandle(
            passiveInteraction, parameterName);
         JavaTckSupport.check(publisherInteraction.encodedLength() > 0
               && publisherParameter.encodedLength() > 0
               && publisherInteraction.equals(activeInteraction)
               && publisherInteraction.equals(passiveInteraction)
               && publisherParameter.equals(activeParameter)
               && publisherParameter.equals(passiveParameter),
            "interaction lookup returned invalid or inconsistent standard handles");

         publisher.publishInteractionClass(publisherInteraction);
         published = true;
         send(publisher, publisherInteraction, publisherParameter,
            new byte[] {0x4e, 0x53});
         observeNoDelivery(publisher, activeReceiver, passiveReceiver, callbackModel);
         JavaTckSupport.check(publisherRecorder.interactions.isEmpty()
               && activeRecorder.interactions.isEmpty()
               && passiveRecorder.interactions.isEmpty(),
            "interaction was delivered before either joined receiver declared a subscription");

         activeReceiver.subscribeInteractionClass(activeInteraction);
         activeSubscribed = true;
         passiveReceiver.subscribeInteractionClassPassively(passiveInteraction);
         passiveSubscribed = true;

         byte[] expectedValue = new byte[] {0x41, 0x50};
         send(publisher, publisherInteraction, publisherParameter, expectedValue);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(activeRecorder.interactions.isEmpty()
                  && passiveRecorder.interactions.isEmpty(),
               "active/passive interaction callbacks arrived before evoked callback servicing");
         }
         awaitDelivery(activeReceiver, activeRecorder, callbackModel);
         JavaTckSupport.check(activeRecorder.interactions.size() == 1,
            "active interaction subscriber received an unexpected callback count");
         Interaction delivered = activeRecorder.interactions.get(0);
         JavaTckSupport.check(delivered.interaction.equals(activeInteraction)
               && delivered.parameters.size() == 1
               && delivered.parameters.containsKey(activeParameter)
               && Arrays.equals(expectedValue, delivered.parameters.get(activeParameter)),
            "active interaction subscriber received the wrong class, parameter, or payload");
         JavaTckSupport.check(publisherRecorder.interactions.isEmpty(),
            "ordinary interaction callback looped back to its publisher");
         JavaTckSupport.check(publisherHandle != null && publisherHandle.encodedLength() > 0,
            "publisher join returned an invalid standard federate handle");
      } finally {
         if (activeSubscribed && activeJoined) {
            try { activeReceiver.unsubscribeInteractionClass(activeInteraction); }
            catch (Exception ignored) { }
         }
         if (passiveSubscribed && passiveJoined) {
            try { passiveReceiver.unsubscribeInteractionClass(passiveInteraction); }
            catch (Exception ignored) { }
         }
         if (published && publisherJoined) {
            try { publisher.unpublishInteractionClass(publisherInteraction); }
            catch (Exception ignored) { }
         }
         if (passiveJoined) {
            try { passiveReceiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (activeJoined) {
            try { activeReceiver.resignFederationExecution(ResignAction.NO_ACTION); }
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
         if (passiveConnected) {
            try { passiveReceiver.disconnect(); } catch (Exception ignored) { }
         }
         if (activeConnected) {
            try { activeReceiver.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void send(RTIambassador publisher, InteractionClassHandle interaction,
         ParameterHandle parameter, byte[] value) throws Exception {
      ParameterHandleValueMap parameters = publisher.getParameterHandleValueMapFactory().create(1);
      parameters.put(parameter, value);
      publisher.sendInteraction(interaction, parameters, new byte[0]);
   }

   private static void observeNoDelivery(RTIambassador publisher, RTIambassador activeReceiver,
         RTIambassador passiveReceiver, CallbackModel callbackModel) throws Exception {
      long deadline = System.nanoTime() + 100_000_000L;
      while (System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            publisher.evokeCallback(0.0);
            activeReceiver.evokeCallback(0.0);
            passiveReceiver.evokeCallback(0.0);
         } else {
            Thread.sleep(1L);
         }
      }
   }

   private static void awaitDelivery(RTIambassador receiver, Recorder recorder,
         CallbackModel callbackModel) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (recorder.interactions.isEmpty() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) receiver.evokeCallback(0.0);
         else Thread.yield();
      }
      JavaTckSupport.check(!recorder.interactions.isEmpty(),
         "active ordinary interaction delivery did not arrive");
   }

   private static final class Interaction {
      private InteractionClassHandle interaction;
      private ParameterHandleValueMap parameters;
   }

   private static final class Recorder {
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();

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
                  interactions.add(event);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK interaction subscription modes";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
