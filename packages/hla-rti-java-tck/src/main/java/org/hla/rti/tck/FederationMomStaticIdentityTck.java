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
import hla.rti1516_2025.encoding.HLAunicodeString;
import java.lang.reflect.Proxy;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.UUID;
import java.util.function.BooleanSupplier;

final class FederationMomStaticIdentityTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final String FEDERATION_CLASS = "HLAobjectRoot.HLAmanager.HLAfederation";

   private FederationMomStaticIdentityTck() {
   }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-mom-static-identity-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean observerConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean observerJoined = false;
      boolean subscribed = false;
      ObjectClassHandle federationClass = null;
      AttributeHandleSet identityAttributes = null;
      try {
         check(owner.connect(callbacks(), callbackModel) != null,
            "static-identity owner connect returned null ConfigurationResult");
         ownerConnected = true;
         check(observer.connect(recorder.proxy(), callbackModel) != null,
            "static-identity observer connect returned null ConfigurationResult");
         observerConnected = true;

         owner.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-static-identity-owner", federation);
         ownerJoined = true;
         observer.joinFederationExecution("java-tck-static-identity-observer", federation);
         observerJoined = true;

         federationClass = observer.getObjectClassHandle(FEDERATION_CLASS);
         AttributeHandle federationName = observer.getAttributeHandle(
            federationClass, "HLAfederationName");
         AttributeHandle rtiVersion = observer.getAttributeHandle(
            federationClass, "HLARTIversion");
         AttributeHandle mimDesignator = observer.getAttributeHandle(
            federationClass, "HLAMIMdesignator");
         AttributeHandle timeImplementationName = observer.getAttributeHandle(
            federationClass, "HLAtimeImplementationName");
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         check(federationClass != null && federationName != null && rtiVersion != null
               && mimDesignator != null && timeImplementationName != null && reliable != null,
            "static-identity lookup returned a null standard handle");
         check(FEDERATION_CLASS.equals(observer.getObjectClassName(federationClass))
               && "HLAfederationName".equals(
                  observer.getAttributeName(federationClass, federationName))
               && "HLARTIversion".equals(observer.getAttributeName(federationClass, rtiVersion))
               && "HLAMIMdesignator".equals(
                  observer.getAttributeName(federationClass, mimDesignator))
               && "HLAtimeImplementationName".equals(
                  observer.getAttributeName(federationClass, timeImplementationName))
               && "HLAreliable".equals(observer.getTransportationTypeName(reliable)),
            "static-identity handle names did not round-trip through the standard API");

         identityAttributes = JavaTckSupport.attributeSet(
            observer, federationName, rtiVersion, mimDesignator, timeImplementationName);
         observer.subscribeObjectClassAttributes(federationClass, identityAttributes);
         subscribed = true;
         await(observer, callbackModel, () -> recorder.discoveredObject != null,
            "standard federation MOM object discovery");
         ObjectInstanceHandle federationObject = recorder.discoveredObject;
         check(federationClass.equals(recorder.discoveredClass)
               && "HLAfederation".equals(recorder.discoveredName)
               && federationClass.equals(observer.getKnownObjectClassHandle(federationObject))
               && "HLAfederation".equals(observer.getObjectInstanceName(federationObject))
               && federationObject.equals(observer.getObjectInstanceHandle("HLAfederation")),
            "standard federation MOM discovery did not preserve object identity");

         requestIdentity(observer, recorder, encoder, callbackModel, federationObject,
            identityAttributes, federationName, rtiVersion, mimDesignator,
            timeImplementationName, reliable, federation, timeImplementation);
      } finally {
         if (subscribed && observerJoined && federationClass != null
               && identityAttributes != null) {
            try {
               observer.unsubscribeObjectClassAttributes(federationClass, identityAttributes);
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

   private static void requestIdentity(RTIambassador observer, Recorder recorder,
         EncoderFactory encoder, CallbackModel callbackModel, ObjectInstanceHandle federationObject,
         AttributeHandleSet attributes, AttributeHandle federationName, AttributeHandle rtiVersion,
         AttributeHandle mimDesignator, AttributeHandle timeImplementationName,
         TransportationTypeHandle reliable, String expectedFederation,
         String expectedTimeImplementation) throws Exception {
      recorder.clearReflection();
      observer.requestAttributeValueUpdate(federationObject, attributes, new byte[0]);
      await(observer, callbackModel, () -> recorder.hasIdentity(federationObject,
         federationName, rtiVersion, mimDesignator, timeImplementationName),
         "requested standard federation MOM identity values");
      check(recorder.reflectedValues.size() == 4
            && recorder.reflectionTag != null && recorder.reflectionTag.length == 0
            && reliable.equals(recorder.reflectionTransportation),
         "federation MOM identity reflection had incomplete or non-standard metadata");

      String actualFederation = decode(encoder, recorder.reflectedValues.get(federationName));
      String actualVersion = decode(encoder, recorder.reflectedValues.get(rtiVersion));
      String actualMim = decode(encoder, recorder.reflectedValues.get(mimDesignator));
      String actualTimeImplementation =
         decode(encoder, recorder.reflectedValues.get(timeImplementationName));
      check(expectedFederation.equals(actualFederation),
         "HLAfederationName did not match the created federation execution");
      check(actualVersion != null && !actualVersion.isEmpty(),
         "required static HLARTIversion value was empty");
      check("HLAstandardMIM".equals(actualMim),
         "HLAMIMdesignator did not identify the supplied standard MIM");
      check(expectedTimeImplementation.equals(actualTimeImplementation),
         "HLAtimeImplementationName did not match the create-federation argument");
   }

   private static String decode(EncoderFactory encoder, byte[] value) throws Exception {
      HLAunicodeString decoded = encoder.createHLAunicodeString();
      decoded.decode(value);
      return decoded.getValue();
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

      private boolean hasIdentity(ObjectInstanceHandle expectedObject,
            AttributeHandle federationName, AttributeHandle rtiVersion,
            AttributeHandle mimDesignator, AttributeHandle timeImplementationName) {
         Map<AttributeHandle, byte[]> values = reflectedValues;
         return expectedObject.equals(reflectedObject) && values != null && values.size() == 4
            && values.containsKey(federationName) && values.containsKey(rtiVersion)
            && values.containsKey(mimDesignator) && values.containsKey(timeImplementationName);
      }

      private void clearReflection() {
         reflectedObject = null;
         reflectedValues = null;
         reflectionTag = null;
         reflectionTransportation = null;
      }
   }
}
