package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.UUID;

/** Focused publication gate for ordinary interaction sends. */
final class DeclarationPublishBeforeSendTck {
   private DeclarationPublishBeforeSendTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder subscriberRecorder = new Recorder();
      String federation = "java-tck-declaration-publish-before-send-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean subscriberConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean subscriberJoined = false;
      boolean published = false;
      boolean subscribed = false;

      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         subscriber.connect(subscriberRecorder.proxy(), callbackModel);
         subscriberConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         publisher.joinFederationExecution(
            "java-tck-declaration-publish-publisher", "publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution(
            "java-tck-declaration-publish-subscriber", "subscriber", federation);
         subscriberJoined = true;

         InteractionClassHandle publisherClass =
            publisher.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle subscriberClass =
            subscriber.getInteractionClassHandle(interactionClassName);
         ParameterHandle publisherParameter =
            publisher.getParameterHandle(publisherClass, parameterName);
         ParameterHandle subscriberParameter =
            subscriber.getParameterHandle(subscriberClass, parameterName);
         JavaTckSupport.check(publisherClass.equals(subscriberClass)
               && publisherParameter.equals(subscriberParameter),
            "joined federates did not resolve matching interaction and parameter handles");

         subscriber.subscribeInteractionClass(subscriberClass);
         subscribed = true;
         byte[] payload = new byte[] {0x51};
         byte[] tag = new byte[] {(byte) 0xa5, 0x5a};
         ParameterHandleValueMap values =
            publisher.getParameterHandleValueMapFactory().create(1);
         values.put(publisherParameter, payload);

         JavaTckSupport.expectFailure(
            () -> publisher.sendInteraction(publisherClass, values, tag),
            "InteractionClassNotPublished", "sendInteraction before publication");
         pump(subscriber, callbackModel);
         JavaTckSupport.check(subscriberRecorder.interactions == 0,
            "an interaction sent before publication produced a receive callback");

         publisher.publishInteractionClass(publisherClass);
         published = true;
         publisher.sendInteraction(publisherClass, values, tag);
         pump(subscriber, callbackModel);
         JavaTckSupport.check(subscriberRecorder.interactions == 1,
            "published interaction did not produce exactly one receive callback");
         JavaTckSupport.check(publisherClass.equals(subscriberRecorder.interactionClass)
               && subscriberRecorder.parameters != null
               && subscriberRecorder.parameters.size() == 1
               && subscriberRecorder.parameters.containsKey(subscriberParameter),
            "receiveInteraction did not preserve the class and parameter handle");
         JavaTckSupport.check(Arrays.equals(payload,
               subscriberRecorder.parameters.get(subscriberParameter)),
            "receiveInteraction did not preserve the parameter payload");
         JavaTckSupport.check(Arrays.equals(tag, subscriberRecorder.tag),
            "receiveInteraction did not preserve the user tag");
      } finally {
         if (subscribed && subscriberJoined) {
            try { subscriber.unsubscribeInteractionClass(
               subscriber.getInteractionClassHandle(interactionClassName)); }
            catch (Exception ignored) { }
         }
         if (published && publisherJoined) {
            try { publisher.unpublishInteractionClass(
               publisher.getInteractionClassHandle(interactionClassName)); }
            catch (Exception ignored) { }
         }
         if (subscriberJoined) {
            try { subscriber.resignFederationExecution(ResignAction.NO_ACTION); }
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
         if (subscriberConnected) {
            try { subscriber.disconnect(); }
            catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static void pump(RTIambassador ambassador, CallbackModel callbackModel)
         throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) {
         JavaTckSupport.drain(ambassador);
      }
   }

   private static final class Recorder {
      private int interactions;
      private InteractionClassHandle interactionClass;
      private ParameterHandleValueMap parameters;
      private byte[] tag;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length >= 3) {
                  interactions++;
                  interactionClass = (InteractionClassHandle) arguments[0];
                  parameters = ((ParameterHandleValueMap) arguments[1]).clone();
                  tag = ((byte[]) arguments[2]).clone();
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK publication-gate callbacks";
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
