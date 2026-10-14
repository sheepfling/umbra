package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;

/** Callback enable/disable behavior for standard directed-interaction delivery. */
final class CallbackControlsDirectedInteractionTck {
   private CallbackControlsDirectedInteractionTck() {
   }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String attributeName, String interactionClassName, String parameterName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador target = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder targetRecorder = new Recorder();
      String federation = "java-tck-callback-directed-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean targetConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean targetJoined = false;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         target.connect(targetRecorder.proxy(), callbackModel);
         targetConnected = true;
         publisher.createFederationExecution(federation, fom);
         created = true;
         FederateHandle publisherFederate = publisher.joinFederationExecution(
            "java-tck-callback-directed-publisher", federation);
         publisherJoined = true;
         target.joinFederationExecution("java-tck-callback-directed-target", federation);
         targetJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle targetClass = target.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(publisherClass,
            attributeName);
         AttributeHandle targetAttribute = target.getAttributeHandle(targetClass, attributeName);
         InteractionClassHandle publisherInteraction =
            publisher.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle targetInteraction =
            target.getInteractionClassHandle(interactionClassName);
         ParameterHandle publisherParameter = publisher.getParameterHandle(
            publisherInteraction, parameterName);
         ParameterHandle targetParameter = target.getParameterHandle(targetInteraction,
            parameterName);
         JavaTckSupport.check(publisherClass.equals(targetClass)
               && publisherAttribute.equals(targetAttribute)
               && publisherInteraction.equals(targetInteraction)
               && publisherParameter.equals(targetParameter),
            "standard FOM lookup returned mismatched directed-interaction handles");

         AttributeHandleSet publisherAttributes =
            JavaTckSupport.attributeSet(publisher, publisherAttribute);
         AttributeHandleSet targetAttributes =
            JavaTckSupport.attributeSet(target, targetAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         target.publishObjectClassAttributes(targetClass, targetAttributes);
         publisher.subscribeObjectClassAttributes(publisherClass, publisherAttributes);
         target.subscribeObjectClassAttributes(targetClass, targetAttributes);
         publisher.publishObjectClassDirectedInteractions(publisherClass,
            JavaTckSupport.interactionSet(publisher, publisherInteraction));
         target.subscribeObjectClassDirectedInteractions(targetClass,
            JavaTckSupport.interactionSet(target, targetInteraction));

         ObjectInstanceHandle targetObject = target.registerObjectInstance(targetClass);
         await(publisher, callbackModel, objectClassName,
            () -> publisherRecorder.discoveries.contains(targetObject),
            "publisher discovery of directed-interaction target");

         byte[] firstValue = new byte[] {0x44, 0x49, 0x31};
         byte[] firstTag = new byte[] {0x44, 0x54, 0x31};
         publisher.sendDirectedInteraction(publisherInteraction, targetObject,
            parameters(publisher, publisherParameter, firstValue), firstTag);
         await(target, callbackModel, objectClassName,
            () -> targetRecorder.directed.size() == 1,
            "baseline directed interaction");
         JavaTckSupport.check(targetRecorder.directed.size() == 1,
            "baseline directed interaction was delivered more than once");
         checkDirected(target, targetRecorder.directed.get(0), targetObject,
            targetInteraction, targetParameter, firstValue, firstTag, publisherFederate);

         target.disableCallbacks();
         byte[] secondValue = new byte[] {0x44, 0x49, 0x32};
         byte[] secondTag = new byte[] {0x44, 0x54, 0x32};
         publisher.sendDirectedInteraction(publisherInteraction, targetObject,
            parameters(publisher, publisherParameter, secondValue), secondTag);
         service(target, callbackModel, objectClassName);
         JavaTckSupport.check(targetRecorder.directed.size() == 1,
            "disabled callbacks exposed a directed-interaction callback");

         target.enableCallbacks();
         await(target, callbackModel, objectClassName,
            () -> targetRecorder.directed.size() == 2,
            "re-enabled directed interaction");
         JavaTckSupport.check(targetRecorder.directed.size() == 2,
            "re-enabled directed interaction was delivered more than once");
         checkDirected(target, targetRecorder.directed.get(1), targetObject,
            targetInteraction, targetParameter, secondValue, secondTag, publisherFederate);
      } finally {
         if (targetJoined) {
            try {
               target.unsubscribeObjectClassDirectedInteractions(
                  target.getObjectClassHandle(objectClassName),
                  JavaTckSupport.interactionSet(target,
                     target.getInteractionClassHandle(interactionClassName)));
            } catch (Exception ignored) { }
            try {
               target.unsubscribeObjectClassAttributes(
                  target.getObjectClassHandle(objectClassName),
                  JavaTckSupport.attributeSet(target,
                     target.getAttributeHandle(target.getObjectClassHandle(objectClassName),
                        attributeName)));
            } catch (Exception ignored) { }
            try {
               target.unpublishObjectClassAttributes(
                  target.getObjectClassHandle(objectClassName),
                  JavaTckSupport.attributeSet(target,
                     target.getAttributeHandle(target.getObjectClassHandle(objectClassName),
                        attributeName)));
            } catch (Exception ignored) { }
            resign(target, ResignAction.DELETE_OBJECTS);
         }
         if (publisherJoined) {
            try {
               publisher.unpublishObjectClassDirectedInteractions(
                  publisher.getObjectClassHandle(objectClassName),
                  JavaTckSupport.interactionSet(publisher,
                     publisher.getInteractionClassHandle(interactionClassName)));
            } catch (Exception ignored) { }
            try {
               publisher.unsubscribeObjectClassAttributes(
                  publisher.getObjectClassHandle(objectClassName),
                  JavaTckSupport.attributeSet(publisher,
                     publisher.getAttributeHandle(publisher.getObjectClassHandle(objectClassName),
                        attributeName)));
            } catch (Exception ignored) { }
            try {
               publisher.unpublishObjectClassAttributes(
                  publisher.getObjectClassHandle(objectClassName),
                  JavaTckSupport.attributeSet(publisher,
                     publisher.getAttributeHandle(publisher.getObjectClassHandle(objectClassName),
                        attributeName)));
            } catch (Exception ignored) { }
            resign(publisher, ResignAction.NO_ACTION);
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (targetConnected) disconnect(target);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static ParameterHandleValueMap parameters(RTIambassador ambassador,
         ParameterHandle parameter, byte[] value) throws Exception {
      ParameterHandleValueMap values =
         ambassador.getParameterHandleValueMapFactory().create(1);
      values.put(parameter, value);
      return values;
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         String objectClassName, Condition condition, String description) throws Exception {
      for (int attempt = 0; attempt < 100 && !condition.isTrue(); attempt++) {
         service(ambassador, model, objectClassName);
         Thread.sleep(10);
      }
      JavaTckSupport.check(condition.isTrue(), "timed out waiting for " + description);
   }

   private static void service(RTIambassador ambassador, CallbackModel model,
         String objectClassName) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) {
         ambassador.evokeMultipleCallbacks(0.0, 0.01);
      } else {
         ambassador.getObjectClassHandle(objectClassName);
      }
   }

   private static void checkDirected(RTIambassador ambassador, Directed received,
         ObjectInstanceHandle targetObject, InteractionClassHandle interaction,
         ParameterHandle parameter, byte[] value, byte[] tag, FederateHandle producer)
         throws Exception {
      TransportationTypeHandle reliable =
         ambassador.getTransportationTypeHandle("HLAreliable");
      JavaTckSupport.check(interaction.equals(received.interaction)
            && targetObject.equals(received.object)
            && received.parameters.size() == 1
            && received.parameters.containsKey(parameter)
            && Arrays.equals(value, received.parameters.get(parameter))
            && Arrays.equals(tag, received.tag)
            && received.transportation != null
            && "HLAreliable".equals(
               ambassador.getTransportationTypeName(received.transportation))
            && reliable.equals(received.transportation)
            && producer.equals(received.producer),
         "directed-interaction callback returned incorrect metadata or payload");
   }

   private static void resign(RTIambassador ambassador, ResignAction action) {
      try { ambassador.resignFederationExecution(action); } catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private interface Condition {
      boolean isTrue();
   }

   private static final class Directed {
      private final InteractionClassHandle interaction;
      private final ObjectInstanceHandle object;
      private final ParameterHandleValueMap parameters;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private Directed(InteractionClassHandle interaction, ObjectInstanceHandle object,
            ParameterHandleValueMap parameters, byte[] tag,
            TransportationTypeHandle transportation, FederateHandle producer) {
         this.interaction = interaction;
         this.object = object;
         this.parameters = parameters;
         this.tag = tag;
         this.transportation = transportation;
         this.producer = producer;
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new ArrayList<>();
      private final List<Directed> directed = new ArrayList<>();
      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("receiveDirectedInteraction".equals(name)
                     && arguments != null && arguments.length == 6) {
                  directed.add(new Directed((InteractionClassHandle) arguments[0],
                     (ObjectInstanceHandle) arguments[1],
                     ((ParameterHandleValueMap) arguments[2]).clone(),
                     ((byte[]) arguments[3]).clone(),
                     (TransportationTypeHandle) arguments[4],
                     (FederateHandle) arguments[5]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK directed callback controls";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
