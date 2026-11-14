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
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies an uncategorized MOM interaction remains ordinary federate traffic. */
final class FederateMomCategoryCInteractionTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 15_000_000_000L;
   private static final String INTERACTION_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.TckCategoryCInteraction";
   private static final String PARAMETER = "HLAserviceReporting";

   private FederateMomCategoryCInteractionTck() { }

   static void run(RtiFactory factory, String fom, String extensionFom, String mim,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      String federateType = System.getProperty("hla.rti.tck.federateType", "tck-type");
      JavaTckSupport.check(!federateType.isEmpty(),
         "adapter-supplied federate type must not be empty");

      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-mom-category-c-interaction-" + suffix;
      String publisherName = "java-tck-mom-category-c-publisher-" + suffix;
      boolean publisherConnected = false, observerConnected = false, created = false;
      boolean publisherJoined = false, observerJoined = false;
      boolean published = false, subscribed = false;
      InteractionClassHandle publisherClass = null, observerClass = null;
      try {
         publisher.connect(callbackProxy(), callbackModel);
         publisherConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         publisher.createFederationExecutionWithMIM(
            federation, new String[] {fom, extensionFom}, mim, timeImplementation);
         created = true;
         FederateHandle publisherHandle = publisher.joinFederationExecution(
            publisherName, federateType, federation);
         publisherJoined = true;
         observer.joinFederationExecution(
            "java-tck-mom-category-c-observer-" + suffix, federateType, federation);
         observerJoined = true;

         publisherClass = publisher.getInteractionClassHandle(INTERACTION_CLASS);
         observerClass = observer.getInteractionClassHandle(INTERACTION_CLASS);
         JavaTckSupport.check(publisherClass != null && observerClass != null,
            "the FOM-added category-c interaction was unavailable");
         ParameterHandle publisherParameter = publisher.getParameterHandle(
            publisherClass, PARAMETER);
         ParameterHandle observerParameter = observer.getParameterHandle(observerClass, PARAMETER);
         JavaTckSupport.check(publisherParameter != null && observerParameter != null,
            "the FOM-added category-c switch-shaped parameter was unavailable");

         publisher.publishInteractionClass(publisherClass);
         published = true;
         observer.subscribeInteractionClass(observerClass);
         subscribed = true;
         JavaTckSupport.check(!publisher.getServiceReportingSwitch(),
            "the publisher's predefined HLAserviceReporting switch did not start false");

         HLAinteger32BE switchShapedValue = encoder.createHLAinteger32BE(1);
         ParameterHandleValueMap parameters =
            publisher.getParameterHandleValueMapFactory().create(1);
         parameters.put(publisherParameter, switchShapedValue.toByteArray());
         publisher.sendInteraction(publisherClass, parameters, new byte[0]);
         await(observer, callbackModel, () -> !recorder.interactions.isEmpty(),
            "the federate-originated category-c interaction");
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            while (observer.evokeCallback(0.0)) { }
         }

         JavaTckSupport.check(recorder.interactions.size() == 1
               && observerClass.equals(recorder.interactions.get(0).interaction),
            "the RTI did not route exactly one category-c interaction at its subscribed class");
         Interaction interaction = recorder.interactions.get(0);
         JavaTckSupport.check(publisherHandle != null
               && publisherHandle.equals(interaction.producer)
               && interaction.parameters.size() == 1
               && interaction.parameters.containsKey(observerParameter),
            "the RTI did not preserve the category-c sender and one switch-shaped parameter");
         HLAinteger32BE receivedValue = encoder.createHLAinteger32BE();
         receivedValue.decode(interaction.parameters.get(observerParameter));
         JavaTckSupport.check(receivedValue.getValue() == 1,
            "the RTI changed the category-c interaction's switch-shaped payload while routing it");
         JavaTckSupport.check(!publisher.getServiceReportingSwitch(),
            "the RTI processed the ignored category-c interaction as predefined HLAsetSwitches");
      } finally {
         if (observerJoined && subscribed && observerClass != null) {
            try { observer.unsubscribeInteractionClass(observerClass); }
            catch (Exception ignored) { }
         }
         if (publisherJoined && published && publisherClass != null) {
            try { publisher.unpublishInteractionClass(publisherClass); }
            catch (Exception ignored) { }
         }
         if (observerJoined) {
            try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
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
         if (observerConnected) {
            try { observer.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static FederateAmbassador callbackProxy() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> objectMethod(proxy, method.getName(), arguments));
   }

   private static Object objectMethod(Object proxy, String name, Object[] arguments) {
      if ("toString".equals(name)) return "Java TCK category-c publisher callbacks";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
      return null;
   }

   private static final class Interaction {
      private InteractionClassHandle interaction;
      private final Map<ParameterHandle, byte[]> parameters = new LinkedHashMap<>();
      private FederateHandle producer;
   }

   private static final class Recorder {
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("receiveInteraction".equals(method.getName()) && arguments != null
                     && arguments.length == 6) {
                  Interaction interaction = new Interaction();
                  interaction.interaction = (InteractionClassHandle) arguments[0];
                  ParameterHandleValueMap source = (ParameterHandleValueMap) arguments[1];
                  for (ParameterHandle handle : source.keySet()) {
                     interaction.parameters.put(handle, source.get(handle).clone());
                  }
                  interaction.producer = (FederateHandle) arguments[4];
                  interactions.add(interaction);
               }
               return objectMethod(proxy, method.getName(), arguments);
            });
      }
   }
}
