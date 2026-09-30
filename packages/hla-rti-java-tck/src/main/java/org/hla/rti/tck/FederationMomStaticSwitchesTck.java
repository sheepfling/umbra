package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import java.lang.reflect.Proxy;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.UUID;
import java.util.function.BooleanSupplier;

final class FederationMomStaticSwitchesTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final String FEDERATION_CLASS = "HLAobjectRoot.HLAmanager.HLAfederation";
   private static final String[] SWITCH_NAMES = {
      "HLAadvisoriesUseKnownClass",
      "HLAdelaySubscriptionEvaluation",
      "HLAnonRegulatedGrant",
      "HLAallowRelaxedDDM"
   };

   private FederationMomStaticSwitchesTck() {
   }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-mom-static-switches-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean observerConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean observerJoined = false;
      boolean subscribed = false;
      ObjectClassHandle federationClass = null;
      AttributeHandleSet switchAttributes = null;
      try {
         check(owner.connect(callbacks(), callbackModel) != null,
            "static-switch owner connect returned null ConfigurationResult");
         ownerConnected = true;
         check(observer.connect(recorder.proxy(), callbackModel) != null,
            "static-switch observer connect returned null ConfigurationResult");
         observerConnected = true;

         owner.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-static-switches-owner", federation);
         ownerJoined = true;
         observer.joinFederationExecution("java-tck-static-switches-observer", federation);
         observerJoined = true;

         federationClass = observer.getObjectClassHandle(FEDERATION_CLASS);
         AttributeHandle[] switchHandles = new AttributeHandle[SWITCH_NAMES.length];
         for (int index = 0; index < SWITCH_NAMES.length; index++) {
            switchHandles[index] = observer.getAttributeHandle(
               federationClass, SWITCH_NAMES[index]);
         }
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         check(federationClass != null && reliable != null,
            "static-switch lookup returned a null standard handle");
         for (int index = 0; index < SWITCH_NAMES.length; index++) {
            check(switchHandles[index] != null
                  && SWITCH_NAMES[index].equals(observer.getAttributeName(
                     federationClass, switchHandles[index])),
               "static-switch handle name did not round-trip: " + SWITCH_NAMES[index]);
         }
         check(FEDERATION_CLASS.equals(observer.getObjectClassName(federationClass))
               && "HLAreliable".equals(observer.getTransportationTypeName(reliable)),
            "static-switch standard class or transportation name did not round-trip");

         switchAttributes = JavaTckSupport.attributeSet(observer, switchHandles);
         observer.subscribeObjectClassAttributes(federationClass, switchAttributes);
         subscribed = true;
         await(observer, callbackModel, () -> recorder.discoveredObject != null,
            "standard federation MOM object discovery for static switches");
         ObjectInstanceHandle federationObject = recorder.discoveredObject;
         check(federationClass.equals(recorder.discoveredClass)
               && "HLAfederation".equals(recorder.discoveredName)
               && federationClass.equals(observer.getKnownObjectClassHandle(federationObject))
               && "HLAfederation".equals(observer.getObjectInstanceName(federationObject))
               && federationObject.equals(observer.getObjectInstanceHandle("HLAfederation")),
            "static-switch federation MOM discovery did not preserve object identity");

         recorder.clearReflection();
         observer.requestAttributeValueUpdate(federationObject, switchAttributes, new byte[0]);
         await(observer, callbackModel,
            () -> recorder.hasSwitchValues(federationObject, switchHandles),
            "requested standard federation MOM switch values");
         check(recorder.reflectionTag != null && recorder.reflectionTag.length == 0
               && reliable.equals(recorder.reflectionTransportation),
            "static-switch reflection had non-standard callback metadata");
         for (int index = 0; index < switchHandles.length; index++) {
            HLAinteger32BE value = encoder.createHLAinteger32BE();
            value.decode(recorder.reflectedValues.get(switchHandles[index]));
            check(value.getValue() == 0 || value.getValue() == 1,
               SWITCH_NAMES[index] + " is not a standard HLAswitch value (0 or 1)");
         }
      } finally {
         if (subscribed && observerJoined && federationClass != null
               && switchAttributes != null) {
            try {
               observer.unsubscribeObjectClassAttributes(federationClass, switchAttributes);
            } catch (Exception ignored) {
            }
         }
         if (observerJoined) resign(observer);
         if (ownerJoined) resign(owner);
         if (created) {
            try {
               owner.destroyFederationExecution(federation);
            } catch (Exception ignored) {
            }
         }
         if (observerConnected) disconnect(observer);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean()) {
         if (System.nanoTime() >= deadline) {
            throw new AssertionError("timed out waiting for " + description);
         }
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            ambassador.evokeCallback(0.01);
         } else {
            Thread.sleep(1L);
         }
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java RTI TCK callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) return proxy == arguments[0];
            }
            return null;
         });
   }

   private static void resign(RTIambassador ambassador) {
      try {
         ambassador.resignFederationExecution(ResignAction.NO_ACTION);
      } catch (Exception ignored) {
      }
   }

   private static void disconnect(RTIambassador ambassador) {
      try {
         ambassador.disconnect();
      } catch (Exception ignored) {
      }
   }

   private static void check(boolean condition, String message) {
      if (!condition) throw new AssertionError(message);
   }

   private static final class Recorder {
      private volatile ObjectInstanceHandle discoveredObject;
      private volatile ObjectClassHandle discoveredClass;
      private volatile String discoveredName;
      private volatile ObjectInstanceHandle reflectedObject;
      private volatile Map<AttributeHandle, byte[]> reflectedValues;
      private volatile byte[] reflectionTag;
      private volatile TransportationTypeHandle reflectionTransportation;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("discoverObjectInstance".equals(method.getName())
                     && arguments != null && arguments.length >= 3) {
                  discoveredObject = (ObjectInstanceHandle) arguments[0];
                  discoveredClass = (ObjectClassHandle) arguments[1];
                  discoveredName = (String) arguments[2];
               } else if ("reflectAttributeValues".equals(method.getName())
                     && arguments != null && arguments.length >= 4
                     && arguments[1] instanceof AttributeHandleValueMap) {
                  reflectedObject = (ObjectInstanceHandle) arguments[0];
                  AttributeHandleValueMap source = (AttributeHandleValueMap) arguments[1];
                  Map<AttributeHandle, byte[]> copy = new LinkedHashMap<>();
                  for (AttributeHandle handle : source.keySet()) {
                     copy.put(handle, source.get(handle).clone());
                  }
                  reflectedValues = copy;
                  reflectionTag = arguments[2] instanceof byte[]
                     ? ((byte[]) arguments[2]).clone() : null;
                  reflectionTransportation = (TransportationTypeHandle) arguments[3];
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(method.getName())) return "Java RTI TCK MOM callbacks";
                  if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
                  if ("equals".equals(method.getName())) return proxy == arguments[0];
               }
               return null;
            });
      }

      private boolean hasSwitchValues(ObjectInstanceHandle expectedObject,
            AttributeHandle[] handles) {
         Map<AttributeHandle, byte[]> values = reflectedValues;
         if (!expectedObject.equals(reflectedObject) || values == null
               || values.size() != handles.length) return false;
         for (AttributeHandle handle : handles) {
            if (!values.containsKey(handle)) return false;
         }
         return true;
      }

      private void clearReflection() {
         reflectedObject = null;
         reflectedValues = null;
         reflectionTag = null;
         reflectionTransportation = null;
      }
   }
}
