package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.ConfigurationResult;
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
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.UUID;

/**
 * Portable ordinary attribute and interaction delivery vectors.
 *
 * <p>This helper intentionally binds only to the standard Java RTI API. It
 * covers the non-regional, non-timestamped publication/subscription and
 * callback-control surface that can be moved to another 1516.1-2025 provider.
 */
final class AttributeInteractionTck {
   private static final String HIGH_UPDATE_RATE = "High";

   private AttributeInteractionTck() {
   }

   static void ordinaryRoutes(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         String parameterName) throws Exception {
      RTIambassador disconnected = factory.getRtiAmbassador();
      RTIambassador unjoined = factory.getRtiAmbassador();
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador evokedSubscriber = factory.getRtiAmbassador();
      RTIambassador immediateSubscriber = factory.getRtiAmbassador();
      RTIambassador passiveSubscriber = factory.getRtiAmbassador();
      CallbackRecorder unjoinedRecorder = new CallbackRecorder();
      CallbackRecorder publisherRecorder = new CallbackRecorder();
      CallbackRecorder evokedRecorder = new CallbackRecorder();
      CallbackRecorder immediateRecorder = new CallbackRecorder();
      CallbackRecorder passiveRecorder = new CallbackRecorder();
      String federation = "java-tck-attribute-interaction-" + UUID.randomUUID();
      boolean unjoinedConnected = false;
      boolean publisherConnected = false;
      boolean evokedConnected = false;
      boolean immediateConnected = false;
      boolean passiveConnected = false;
      boolean publisherJoined = false;
      boolean evokedJoined = false;
      boolean immediateJoined = false;
      boolean passiveJoined = false;
      boolean created = false;

      try {
         expectFailure(
            () -> disconnected.getObjectClassHandle(objectClassName),
            "NotConnected", "object-class lookup before attribute routing connect");
         expectFailure(
            () -> disconnected.getInteractionClassHandle(interactionClassName),
            "NotConnected", "interaction-class lookup before interaction routing connect");

         ConfigurationResult unjoinedConfiguration = unjoined.connect(
            unjoinedRecorder.proxy(), CallbackModel.HLA_EVOKED);
         unjoinedConnected = true;
         check(unjoinedConfiguration != null,
            "unjoined attribute/interaction connect returned null ConfigurationResult");
         expectFailure(
            () -> unjoined.getObjectClassHandle(objectClassName),
            "FederateNotExecutionMember", "object-class lookup before attribute routing join");
         expectFailure(
            () -> unjoined.getInteractionClassHandle(interactionClassName),
            "FederateNotExecutionMember",
            "interaction-class lookup before interaction routing join");

         ConfigurationResult publisherConfiguration = publisher.connect(
            publisherRecorder.proxy(), CallbackModel.HLA_EVOKED);
         publisherConnected = true;
         check(publisherConfiguration != null,
            "attribute/interaction publisher connect returned null ConfigurationResult");
         ConfigurationResult evokedConfiguration = evokedSubscriber.connect(
            evokedRecorder.proxy(), CallbackModel.HLA_EVOKED);
         evokedConnected = true;
         check(evokedConfiguration != null,
            "evoked attribute/interaction subscriber connect returned null ConfigurationResult");
         ConfigurationResult immediateConfiguration = immediateSubscriber.connect(
            immediateRecorder.proxy(), CallbackModel.HLA_IMMEDIATE);
         immediateConnected = true;
         check(immediateConfiguration != null,
            "immediate attribute/interaction subscriber connect returned null ConfigurationResult");
         ConfigurationResult passiveConfiguration = passiveSubscriber.connect(
            passiveRecorder.proxy(), CallbackModel.HLA_EVOKED);
         passiveConnected = true;
         check(passiveConfiguration != null,
            "passive attribute/interaction subscriber connect returned null ConfigurationResult");

         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle publisherHandle = publisher.joinFederationExecution(
            "java-tck-attribute-interaction-publisher", "publisher", federation);
         publisherJoined = true;
         evokedSubscriber.joinFederationExecution(
            "java-tck-attribute-interaction-evoked", "subscriber", federation);
         evokedJoined = true;
         immediateSubscriber.joinFederationExecution(
            "java-tck-attribute-interaction-immediate", "subscriber", federation);
         immediateJoined = true;
         passiveSubscriber.joinFederationExecution(
            "java-tck-attribute-interaction-passive", "subscriber", federation);
         passiveJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         ObjectClassHandle evokedClass = evokedSubscriber.getObjectClassHandle(objectClassName);
         AttributeHandle evokedAttribute = evokedSubscriber.getAttributeHandle(
            evokedClass, attributeName);
         ObjectClassHandle immediateClass = immediateSubscriber.getObjectClassHandle(objectClassName);
         AttributeHandle immediateAttribute = immediateSubscriber.getAttributeHandle(
            immediateClass, attributeName);
         ObjectClassHandle passiveClass = passiveSubscriber.getObjectClassHandle(objectClassName);
         AttributeHandle passiveAttribute = passiveSubscriber.getAttributeHandle(
            passiveClass, attributeName);

         InteractionClassHandle publisherInteraction = publisher.getInteractionClassHandle(
            interactionClassName);
         ParameterHandle publisherParameter = publisher.getParameterHandle(
            publisherInteraction, parameterName);
         InteractionClassHandle evokedInteraction = evokedSubscriber.getInteractionClassHandle(
            interactionClassName);
         ParameterHandle evokedParameter = evokedSubscriber.getParameterHandle(
            evokedInteraction, parameterName);
         InteractionClassHandle immediateInteraction = immediateSubscriber.getInteractionClassHandle(
            interactionClassName);
         ParameterHandle immediateParameter = immediateSubscriber.getParameterHandle(
            immediateInteraction, parameterName);
         InteractionClassHandle passiveInteraction = passiveSubscriber.getInteractionClassHandle(
            interactionClassName);
         ParameterHandle passiveParameter = passiveSubscriber.getParameterHandle(
            passiveInteraction, parameterName);

         check(publisherClass.encodedLength() > 0 && publisherAttribute.encodedLength() > 0,
            "publisher attribute lookup returned an invalid standard handle");
         check(evokedClass.equals(publisherClass) && immediateClass.equals(publisherClass)
               && passiveClass.equals(publisherClass),
            "object-class handles differed between joined federates");
         check(evokedAttribute.equals(publisherAttribute)
               && immediateAttribute.equals(publisherAttribute)
               && passiveAttribute.equals(publisherAttribute),
            "attribute handles differed between joined federates");
         check(objectClassName.equals(publisher.getObjectClassName(publisherClass)),
            "object-class name did not round-trip through the standard API");
         check(attributeName.equals(publisher.getAttributeName(publisherClass, publisherAttribute)),
            "attribute name did not round-trip through the standard API");
         check(publisherInteraction.encodedLength() > 0
               && publisherParameter.encodedLength() > 0,
            "publisher interaction lookup returned an invalid standard handle");
         check(evokedInteraction.equals(publisherInteraction)
               && immediateInteraction.equals(publisherInteraction)
               && passiveInteraction.equals(publisherInteraction),
            "interaction-class handles differed between joined federates");
         check(evokedParameter.equals(publisherParameter)
               && immediateParameter.equals(publisherParameter)
               && passiveParameter.equals(publisherParameter),
            "parameter handles differed between joined federates");
         check(interactionClassName.equals(publisher.getInteractionClassName(publisherInteraction)),
            "interaction-class name did not round-trip through the standard API");
         check(parameterName.equals(publisher.getParameterName(
               publisherInteraction, publisherParameter)),
            "parameter name did not round-trip through the standard API");
         check(publisher.getUpdateRateValue(HIGH_UPDATE_RATE) > 0.0,
            "named attribute update-rate lookup did not return a positive FOM value");
         expectFailure(
            () -> publisher.getObjectClassHandle(objectClassName + ".Missing"),
            "NameNotFound", "missing object-class lookup in attribute route");
         expectFailure(
            () -> publisher.getInteractionClassHandle(interactionClassName + ".Missing"),
            "NameNotFound", "missing interaction-class lookup in interaction route");
         expectFailure(
            () -> publisher.getAttributeHandle(publisherClass, attributeName + ".Missing"),
            "NameNotFound", "missing attribute lookup in attribute route");
         expectFailure(
            () -> publisher.getParameterHandle(publisherInteraction, parameterName + ".Missing"),
            "NameNotFound", "missing parameter lookup in interaction route");

         AttributeHandleSet publisherAttributes =
            publisher.getAttributeHandleSetFactory().create();
         publisherAttributes.add(publisherAttribute);
         AttributeHandleSet evokedAttributes =
            evokedSubscriber.getAttributeHandleSetFactory().create();
         evokedAttributes.add(evokedAttribute);
         AttributeHandleSet immediateAttributes =
            immediateSubscriber.getAttributeHandleSetFactory().create();
         immediateAttributes.add(immediateAttribute);
         AttributeHandleSet passiveAttributes =
            passiveSubscriber.getAttributeHandleSetFactory().create();
         passiveAttributes.add(passiveAttribute);

         // Publication and active/passive subscription are independent
         // declarations. The named-rate forms are ordinary, non-regional
         // attribute declarations; passive declarations remain retained but
         // do not arrange discovery or reflection.
         expectFailure(
            () -> publisher.sendInteraction(
               publisherInteraction, publisher.getParameterHandleValueMapFactory().create(1),
               new byte[] {0}),
            "InteractionClassNotPublished", "interaction send before publication");
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         publisher.publishInteractionClass(publisherInteraction);
         evokedSubscriber.subscribeObjectClassAttributes(
            evokedClass, evokedAttributes);
         evokedSubscriber.subscribeObjectClassAttributes(
            evokedClass, evokedAttributes, HIGH_UPDATE_RATE);
         immediateSubscriber.subscribeObjectClassAttributes(
            immediateClass, immediateAttributes, HIGH_UPDATE_RATE);
         passiveSubscriber.subscribeObjectClassAttributesPassively(
            passiveClass, passiveAttributes);
         passiveSubscriber.subscribeObjectClassAttributesPassively(
            passiveClass, passiveAttributes, HIGH_UPDATE_RATE);
         evokedSubscriber.subscribeInteractionClass(evokedInteraction);
         immediateSubscriber.subscribeInteractionClass(immediateInteraction);
         passiveSubscriber.subscribeInteractionClassPassively(passiveInteraction);

         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         check(object != null && object.encodedLength() > 0,
            "ordinary attribute route did not return a registered object handle");
         String registeredName = publisher.getObjectInstanceName(object);
         drain(evokedSubscriber);
         drain(passiveSubscriber);
         check(evokedRecorder.discoveryReports == 1,
            "active evoked attribute subscriber did not discover the object");
         check(immediateRecorder.discoveryReports == 1,
            "active immediate attribute subscriber did not discover the object");
         check(passiveRecorder.discoveryReports == 0,
            "passive attribute subscription arranged ordinary discovery");
         check(object.equals(evokedRecorder.discoveredObject)
               && object.equals(immediateRecorder.discoveredObject),
            "discovery callback named the wrong object instance");
         check(registeredName.equals(evokedRecorder.discoveredName)
               && registeredName.equals(immediateRecorder.discoveredName),
            "discovery callback did not preserve the registered object name");
         check(publisherClass.equals(evokedRecorder.discoveredClass)
               && publisherClass.equals(immediateRecorder.discoveredClass),
            "discovery callback did not preserve the registered object class");

         AttributeHandleValueMap attributeValues =
            publisher.getAttributeHandleValueMapFactory().create(1);
         byte[] firstAttributePayload = new byte[] {1, 2, 3, (byte) 0xff};
         attributeValues.put(publisherAttribute, firstAttributePayload);
         byte[] firstAttributeTag = new byte[] {9, 8, 7};
         publisher.updateAttributeValues(object, attributeValues, firstAttributeTag);
         drain(evokedSubscriber);
         drain(passiveSubscriber);
         check(evokedRecorder.reflectionReports == 1,
            "active evoked attribute subscriber did not receive the update");
         check(immediateRecorder.reflectionReports == 1,
            "active immediate attribute subscriber did not receive the update");
         check(passiveRecorder.reflectionReports == 0,
            "passive attribute subscription arranged ordinary reflection");
         check(object.equals(evokedRecorder.reflectedObject)
               && evokedRecorder.reflectedValues != null
               && evokedRecorder.reflectedValues.size() == 1,
            "evoked reflection callback had the wrong object or attribute count");
         check(Arrays.equals(firstAttributePayload,
               evokedRecorder.reflectedValues.get(evokedAttribute)),
            "evoked reflection payload did not round-trip");
         check(Arrays.equals(firstAttributeTag, evokedRecorder.reflectedTag),
            "evoked reflection tag did not round-trip");

         ParameterHandleValueMap parameterValues =
            publisher.getParameterHandleValueMapFactory().create(1);
         byte[] firstParameterPayload = new byte[] {0x4a, 0x19};
         parameterValues.put(publisherParameter, firstParameterPayload);
         byte[] firstInteractionTag = new byte[] {0x71, 0x2a};
         publisher.sendInteraction(publisherInteraction, parameterValues, firstInteractionTag);
         drain(evokedSubscriber);
         drain(passiveSubscriber);
         check(evokedRecorder.interactionReports == 1,
            "active evoked interaction subscriber did not receive the interaction");
         check(immediateRecorder.interactionReports == 1,
            "active immediate interaction subscriber did not receive the interaction");
         check(passiveRecorder.interactionReports == 0,
            "passive interaction subscription arranged ordinary delivery");
         check(publisherInteraction.equals(evokedRecorder.receivedInteraction)
               && evokedRecorder.receivedParameters != null
               && evokedRecorder.receivedParameters.size() == 1,
            "evoked interaction callback had the wrong class or parameter count");
         check(Arrays.equals(firstParameterPayload,
               evokedRecorder.receivedParameters.get(evokedParameter)),
            "evoked interaction parameter payload did not round-trip");
         check(Arrays.equals(firstInteractionTag, evokedRecorder.receivedTag),
            "evoked interaction tag did not round-trip");
         check(publisherHandle.equals(evokedRecorder.producingFederate)
               && publisherHandle.equals(immediateRecorder.producingFederate),
            "interaction callback named the wrong producing federate");

         // Callback control is orthogonal to publication and subscription.
         // Both callback models must retain the ordinary messages while
         // disabled and deliver them after the standard re-enable call.
         int evokedReflectionsBeforeDisable = evokedRecorder.reflectionReports;
         int immediateReflectionsBeforeDisable = immediateRecorder.reflectionReports;
         int evokedInteractionsBeforeDisable = evokedRecorder.interactionReports;
         int immediateInteractionsBeforeDisable = immediateRecorder.interactionReports;
         evokedSubscriber.disableCallbacks();
         immediateSubscriber.disableCallbacks();
         byte[] disabledAttributePayload = new byte[] {4, 5, 6};
         attributeValues.clear();
         attributeValues.put(publisherAttribute, disabledAttributePayload);
         publisher.updateAttributeValues(object, attributeValues, new byte[] {6, 5, 4});
         byte[] disabledInteractionPayload = new byte[] {0x2a, 0x2b};
         parameterValues.clear();
         parameterValues.put(publisherParameter, disabledInteractionPayload);
         publisher.sendInteraction(publisherInteraction, parameterValues, new byte[] {3, 2, 1});
         check(evokedRecorder.reflectionReports == evokedReflectionsBeforeDisable
               && evokedRecorder.interactionReports == evokedInteractionsBeforeDisable,
            "evoked callbacks were delivered while callbacks were disabled");
         check(immediateRecorder.reflectionReports == immediateReflectionsBeforeDisable
               && immediateRecorder.interactionReports == immediateInteractionsBeforeDisable,
            "immediate callbacks were delivered while callbacks were disabled");
         evokedSubscriber.enableCallbacks();
         immediateSubscriber.enableCallbacks();
         drain(evokedSubscriber);
         check(evokedRecorder.reflectionReports == evokedReflectionsBeforeDisable + 1
               && evokedRecorder.interactionReports == evokedInteractionsBeforeDisable + 1,
            "re-enabling evoked callbacks did not release ordinary deliveries");
         check(immediateRecorder.reflectionReports == immediateReflectionsBeforeDisable + 1
               && immediateRecorder.interactionReports == immediateInteractionsBeforeDisable + 1,
            "re-enabling immediate callbacks did not release ordinary deliveries");

         // Replacing passive declarations with active ones promotes both
         // routes. Existing objects are discovered on attribute promotion,
         // and the next ordinary interaction is delivered after interaction
         // promotion.
         passiveSubscriber.subscribeObjectClassAttributes(
            passiveClass, passiveAttributes, HIGH_UPDATE_RATE);
         drain(passiveSubscriber);
         check(passiveRecorder.discoveryReports == 1,
            "promoted passive attribute subscription did not discover the object");

         attributeValues.clear();
         attributeValues.put(publisherAttribute, new byte[] {0x11, 0x12});
         publisher.updateAttributeValues(object, attributeValues, new byte[] {0x13});
         drain(passiveSubscriber);
         check(passiveRecorder.reflectionReports == 1,
            "promoted passive attribute subscription did not receive an update");

         passiveSubscriber.subscribeInteractionClass(passiveInteraction);
         parameterValues.clear();
         parameterValues.put(publisherParameter, new byte[] {0x21, 0x22});
         publisher.sendInteraction(publisherInteraction, parameterValues, new byte[] {0x23});
         drain(passiveSubscriber);
         check(passiveRecorder.interactionReports == 1,
            "promoted passive interaction subscription did not receive an interaction");

         passiveSubscriber.unsubscribeInteractionClass(passiveInteraction);
         passiveSubscriber.unsubscribeObjectClassAttributes(passiveClass, passiveAttributes);
         passiveSubscriber.unsubscribeObjectClass(passiveClass);
         evokedSubscriber.unsubscribeInteractionClass(evokedInteraction);
         immediateSubscriber.unsubscribeInteractionClass(immediateInteraction);
         evokedSubscriber.unsubscribeObjectClassAttributes(evokedClass, evokedAttributes);
         immediateSubscriber.unsubscribeObjectClassAttributes(
            immediateClass, immediateAttributes);
         evokedSubscriber.unsubscribeObjectClass(evokedClass);
         immediateSubscriber.unsubscribeObjectClass(immediateClass);
         publisher.unpublishInteractionClass(publisherInteraction);
         publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         publisher.unpublishObjectClass(publisherClass);
      } finally {
         if (passiveJoined) {
            try {
               passiveSubscriber.resignFederationExecution(ResignAction.NO_ACTION);
            } catch (Exception ignored) {
            }
         }
         if (immediateJoined) {
            try {
               immediateSubscriber.resignFederationExecution(ResignAction.NO_ACTION);
            } catch (Exception ignored) {
            }
         }
         if (evokedJoined) {
            try {
               evokedSubscriber.resignFederationExecution(ResignAction.NO_ACTION);
            } catch (Exception ignored) {
            }
         }
         if (publisherJoined) {
            try {
               publisher.resignFederationExecution(ResignAction.NO_ACTION);
            } catch (Exception ignored) {
            }
         }
         if (created) {
            try {
               publisher.destroyFederationExecution(federation);
            } catch (Exception ignored) {
            }
         }
         if (passiveConnected) {
            try {
               passiveSubscriber.disconnect();
            } catch (Exception ignored) {
            }
         }
         if (immediateConnected) {
            try {
               immediateSubscriber.disconnect();
            } catch (Exception ignored) {
            }
         }
         if (evokedConnected) {
            try {
               evokedSubscriber.disconnect();
            } catch (Exception ignored) {
            }
         }
         if (publisherConnected) {
            try {
               publisher.disconnect();
            } catch (Exception ignored) {
            }
         }
         if (unjoinedConnected) {
            try {
               unjoined.disconnect();
            } catch (Exception ignored) {
            }
         }
      }
   }

   private static void drain(RTIambassador ambassador) throws Exception {
      for (int i = 0; i < 64; ++i) {
         ambassador.evokeCallback(0.0);
      }
   }

   private static void check(boolean condition, String message) {
      if (!condition) {
         throw new AssertionError(message);
      }
   }

   private static void expectFailure(CheckedOperation operation, String expectedType,
         String message) throws Exception {
      try {
         operation.run();
      } catch (Throwable error) {
         for (Throwable current = error; current != null; current = current.getCause()) {
            if (expectedType.equals(current.getClass().getSimpleName())) {
               return;
            }
         }
         throw new AssertionError(message + "; got " + error, error);
      }
      throw new AssertionError(message + "; operation completed without " + expectedType);
   }

   @FunctionalInterface
   private interface CheckedOperation {
      void run() throws Exception;
   }

   private static final class CallbackRecorder {
      private int discoveryReports;
      private ObjectInstanceHandle discoveredObject;
      private ObjectClassHandle discoveredClass;
      private String discoveredName;
      private int reflectionReports;
      private ObjectInstanceHandle reflectedObject;
      private AttributeHandleValueMap reflectedValues;
      private byte[] reflectedTag;
      private int interactionReports;
      private InteractionClassHandle receivedInteraction;
      private ParameterHandleValueMap receivedParameters;
      private byte[] receivedTag;
      private FederateHandle producingFederate;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)
                     && arguments != null && arguments.length >= 3) {
                  discoveryReports++;
                  discoveredObject = (ObjectInstanceHandle) arguments[0];
                  discoveredClass = (ObjectClassHandle) arguments[1];
                  discoveredName = (String) arguments[2];
               } else if ("reflectAttributeValues".equals(name)
                     && arguments != null && arguments.length >= 2) {
                  reflectionReports++;
                  reflectedObject = (ObjectInstanceHandle) arguments[0];
                  reflectedValues = ((AttributeHandleValueMap) arguments[1]).clone();
                  if (arguments.length >= 3 && arguments[2] instanceof byte[]) {
                     reflectedTag = ((byte[]) arguments[2]).clone();
                  }
               } else if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length >= 2) {
                  interactionReports++;
                  receivedInteraction = (InteractionClassHandle) arguments[0];
                  receivedParameters = ((ParameterHandleValueMap) arguments[1]).clone();
                  if (arguments.length >= 3 && arguments[2] instanceof byte[]) {
                     receivedTag = ((byte[]) arguments[2]).clone();
                  }
                  if (arguments.length >= 5 && arguments[4] instanceof FederateHandle) {
                     producingFederate = (FederateHandle) arguments[4];
                  }
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK ordinary callbacks";
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
