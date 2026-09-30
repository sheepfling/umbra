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
import hla.rti1516_2025.encoding.HLAvariableArray;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.Map;
import java.util.Set;
import java.util.TreeSet;
import java.util.UUID;
import java.util.function.BooleanSupplier;

final class FederationMomFomModuleDesignatorListTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private FederationMomFomModuleDesignatorListTck() {
   }

   static void run(RtiFactory factory, String fom, String mim, String additionalFom,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      RTIambassador extension = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-fom-designators-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean observerConnected = false;
      boolean extensionConnected = false;
      boolean ownerJoined = false;
      boolean observerJoined = false;
      boolean extensionJoined = false;
      boolean created = false;
      boolean subscribed = false;
      try {
         check(owner.connect(callbacks(), callbackModel) != null,
            "FOM-designator owner connect returned null ConfigurationResult");
         ownerConnected = true;
         check(observer.connect(recorder.proxy(), callbackModel) != null,
            "FOM-designator observer connect returned null ConfigurationResult");
         observerConnected = true;
         check(extension.connect(callbacks(), callbackModel) != null,
            "FOM-designator extension connect returned null ConfigurationResult");
         extensionConnected = true;

         owner.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-designators-owner", "tck-type", federation);
         ownerJoined = true;
         observer.joinFederationExecution(
            "java-tck-designators-observer", "tck-type", federation);
         observerJoined = true;

         ObjectClassHandle federationClass = observer.getObjectClassHandle(
            "HLAobjectRoot.HLAmanager.HLAfederation");
         AttributeHandle designatorAttribute = observer.getAttributeHandle(
            federationClass, "HLAFOMmoduleDesignatorList");
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         check(federationClass != null && designatorAttribute != null && reliable != null,
            "FOM-designator lookup returned a null standard handle");
         check("HLAobjectRoot.HLAmanager.HLAfederation".equals(
               observer.getObjectClassName(federationClass))
               && "HLAFOMmoduleDesignatorList".equals(
                  observer.getAttributeName(federationClass, designatorAttribute))
               && "HLAreliable".equals(observer.getTransportationTypeName(reliable)),
            "FOM-designator standard handle names did not round-trip");

         AttributeHandleSet designatorAttributes =
            JavaTckSupport.attributeSet(observer, designatorAttribute);
         observer.subscribeObjectClassAttributes(federationClass, designatorAttributes);
         subscribed = true;
         await(observer, callbackModel, () -> recorder.discoveredObject != null,
            "federation MOM discovery for the FOM-module designator list");
         ObjectInstanceHandle federationObject = recorder.discoveredObject;
         check(federationClass.equals(recorder.discoveredClass)
               && "HLAfederation".equals(recorder.discoveredName)
               && federationClass.equals(observer.getKnownObjectClassHandle(federationObject))
               && "HLAfederation".equals(observer.getObjectInstanceName(federationObject))
               && federationObject.equals(observer.getObjectInstanceHandle("HLAfederation")),
            "FOM-designator discovery did not preserve standard federation-object identity");

         Set<String> initialExpected = canonicalize(java.util.Collections.singletonList(fom));
         Set<String> initial = requestDesignators(observer, recorder, encoder, callbackModel,
            federationObject, designatorAttribute, designatorAttributes, reliable,
            initialExpected, "initial federation FOM-module designator list");
         check(initial.size() == 1,
            "created federation FOM-module designator list had an unexpected size");

         Set<String> allExpected = canonicalize(java.util.Arrays.asList(fom, additionalFom));
         recorder.clearReflection();
         extension.joinFederationExecution("java-tck-designators-extension", "tck-type",
            federation, new String[] {additionalFom});
         extensionJoined = true;
         await(observer, callbackModel, () -> {
            Set<String> value = recorder.currentDesignators(encoder, designatorAttribute);
            return allExpected.equals(value);
         }, "conditional FOM-module designator refresh after an additional-FOM join");
         Set<String> joinedValue = recorder.currentDesignators(encoder, designatorAttribute);
         check(joinedValue != null && joinedValue.equals(allExpected)
               && joinedValue.size() == 2,
            "join-triggered designator reflection omitted or duplicated a module");
         check(recorder.reflectedValues.size() == 1
               && reliable.equals(recorder.reflectionTransportation)
               && recorder.reflectionTag != null && recorder.reflectionTag.length == 0,
            "join-triggered designator reflection returned incomplete or non-standard metadata");

         Set<String> requestedAfterJoin = requestDesignators(observer, recorder, encoder,
            callbackModel, federationObject, designatorAttribute, designatorAttributes,
            reliable, allExpected, "requested federation FOM-module designator list after composition");
         check(requestedAfterJoin.equals(joinedValue),
            "requested designator list disagreed with the join-triggered update");
      } finally {
         if (subscribed && observerJoined) {
            try {
               observer.unsubscribeObjectClassAttributes(
                  observer.getObjectClassHandle("HLAobjectRoot.HLAmanager.HLAfederation"),
                  JavaTckSupport.attributeSet(observer,
                     observer.getAttributeHandle(
                        observer.getObjectClassHandle("HLAobjectRoot.HLAmanager.HLAfederation"),
                        "HLAFOMmoduleDesignatorList")));
            } catch (Exception ignored) {
            }
         }
         if (extensionJoined) resign(extension);
         if (observerJoined) resign(observer);
         if (ownerJoined) resign(owner);
         if (created) {
            try {
               owner.destroyFederationExecution(federation);
            } catch (Exception ignored) {
            }
         }
         if (extensionConnected) disconnect(extension);
         if (observerConnected) disconnect(observer);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static Set<String> requestDesignators(RTIambassador observer, Recorder recorder,
         EncoderFactory encoder, CallbackModel callbackModel, ObjectInstanceHandle federationObject,
         AttributeHandle designatorAttribute, AttributeHandleSet designatorAttributes,
         TransportationTypeHandle reliable, Set<String> expected, String description)
         throws Exception {
      recorder.clearReflection();
      observer.requestAttributeValueUpdate(federationObject, designatorAttributes, new byte[0]);
      await(observer, callbackModel,
         () -> expected.equals(recorder.currentDesignators(encoder, designatorAttribute)),
         description);
      check(federationObject.equals(recorder.reflectedObject)
            && recorder.reflectedValues != null
            && recorder.reflectedValues.size() == 1
            && recorder.reflectionTag != null && recorder.reflectionTag.length == 0
            && reliable.equals(recorder.reflectionTransportation),
         description + " returned incomplete or non-standard reflection metadata");
      Set<String> actual = recorder.currentDesignators(encoder, designatorAttribute);
      check(expected.equals(actual), description + " returned the wrong designator set");
      return actual;
   }

   private static Set<String> decodeDesignators(EncoderFactory encoder, byte[] encoded)
         throws Exception {
      HLAvariableArray<HLAunicodeString> values = encoder.createHLAvariableArray(
         index -> encoder.createHLAunicodeString());
      values.decode(encoded);
      Set<String> result = new LinkedHashSet<>();
      for (HLAunicodeString value : values) {
         check(result.add(value.getValue()),
            "HLAFOMmoduleDesignatorList contained a duplicate designator");
      }
      return result;
   }

   private static Set<String> canonicalize(Iterable<String> values) {
      Set<String> result = new TreeSet<>();
      for (String value : values) {
         check(result.add(value), "expected FOM-module designators contained a duplicate");
      }
      return result;
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

      private void clearReflection() {
         reflectedObject = null;
         reflectedValues = null;
         reflectionTag = null;
         reflectionTransportation = null;
      }

      private Set<String> currentDesignators(EncoderFactory encoder, AttributeHandle attribute) {
         Map<AttributeHandle, byte[]> values = reflectedValues;
         if (values == null || !values.containsKey(attribute)) return null;
         try {
            return decodeDesignators(encoder, values.get(attribute));
         } catch (Exception error) {
            throw new AssertionError("could not decode HLAFOMmoduleDesignatorList", error);
         }
      }
   }
}
