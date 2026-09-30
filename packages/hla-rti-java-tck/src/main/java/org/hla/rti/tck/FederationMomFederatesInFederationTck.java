package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.FederateHandleFactory;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAbyte;
import hla.rti1516_2025.encoding.HLAvariableArray;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.Map;
import java.util.Set;
import java.util.UUID;
import java.util.function.BooleanSupplier;

final class FederationMomFederatesInFederationTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final String FEDERATION_CLASS = "HLAobjectRoot.HLAmanager.HLAfederation";

   private FederationMomFederatesInFederationTck() {
   }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      RTIambassador entrant = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-mom-roster-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean observerConnected = false;
      boolean entrantConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean observerJoined = false;
      boolean entrantJoined = false;
      boolean subscribed = false;
      ObjectClassHandle federationClass = null;
      AttributeHandleSet rosterAttributes = null;
      try {
         check(owner.connect(callbacks(), callbackModel) != null,
            "roster owner connect returned null ConfigurationResult");
         ownerConnected = true;
         check(observer.connect(recorder.proxy(), callbackModel) != null,
            "roster observer connect returned null ConfigurationResult");
         observerConnected = true;
         check(entrant.connect(callbacks(), callbackModel) != null,
            "roster entrant connect returned null ConfigurationResult");
         entrantConnected = true;

         owner.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle ownerHandle = owner.joinFederationExecution(
            "java-tck-roster-owner", "tck-type", federation);
         ownerJoined = true;
         FederateHandle observerHandle = observer.joinFederationExecution(
            "java-tck-roster-observer", "tck-type", federation);
         observerJoined = true;

         federationClass = observer.getObjectClassHandle(FEDERATION_CLASS);
         AttributeHandle rosterAttribute = observer.getAttributeHandle(
            federationClass, "HLAfederatesInFederation");
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         check(federationClass != null && rosterAttribute != null && reliable != null,
            "roster lookup returned a null standard handle");
         check(FEDERATION_CLASS.equals(observer.getObjectClassName(federationClass))
               && "HLAfederatesInFederation".equals(
                  observer.getAttributeName(federationClass, rosterAttribute))
               && "HLAreliable".equals(observer.getTransportationTypeName(reliable)),
            "federation roster standard handle names did not round-trip");

         rosterAttributes = JavaTckSupport.attributeSet(observer, rosterAttribute);
         observer.subscribeObjectClassAttributes(federationClass, rosterAttributes);
         subscribed = true;
         await(observer, callbackModel, () -> recorder.discoveredObject != null,
            "standard federation MOM object discovery for the roster");
         ObjectInstanceHandle federationObject = recorder.discoveredObject;
         check(federationClass.equals(recorder.discoveredClass)
               && "HLAfederation".equals(recorder.discoveredName)
               && federationClass.equals(observer.getKnownObjectClassHandle(federationObject))
               && "HLAfederation".equals(observer.getObjectInstanceName(federationObject))
               && federationObject.equals(observer.getObjectInstanceHandle("HLAfederation")),
            "federation roster discovery did not preserve standard object identity");

         FederateHandleFactory handleFactory = observer.getFederateHandleFactory();
         check(handleFactory != null, "standard federate-handle factory was null");
         Set<FederateHandle> initial = new LinkedHashSet<>(Arrays.asList(
            ownerHandle, observerHandle));
         requestRoster(observer, recorder, encoder, handleFactory, callbackModel,
            federationObject, rosterAttributes, rosterAttribute, reliable, initial,
            "requested standard federation roster");

         recorder.clearReflection();
         FederateHandle entrantHandle = entrant.joinFederationExecution(
            "java-tck-roster-entrant", "tck-type", federation);
         entrantJoined = true;
         Set<FederateHandle> joined = new LinkedHashSet<>(Arrays.asList(
            ownerHandle, observerHandle, entrantHandle));
         await(observer, callbackModel, () -> joined.equals(
            recorder.currentRoster(encoder, handleFactory, rosterAttribute)),
            "conditional federation roster update after federate join");
         verifyReflectionMetadata(recorder, federationObject, reliable,
            "join-triggered federation roster update");

         recorder.clearReflection();
         entrant.resignFederationExecution(ResignAction.NO_ACTION);
         entrantJoined = false;
         await(observer, callbackModel, () -> initial.equals(
            recorder.currentRoster(encoder, handleFactory, rosterAttribute)),
            "conditional federation roster update after federate resignation");
         verifyReflectionMetadata(recorder, federationObject, reliable,
            "resign-triggered federation roster update");
      } finally {
         if (subscribed && observerJoined && federationClass != null
               && rosterAttributes != null) {
            try {
               observer.unsubscribeObjectClassAttributes(federationClass, rosterAttributes);
            } catch (Exception ignored) {
            }
         }
         if (entrantJoined) resign(entrant);
         if (observerJoined) resign(observer);
         if (ownerJoined) resign(owner);
         if (created) {
            try {
               owner.destroyFederationExecution(federation);
            } catch (Exception ignored) {
            }
         }
         if (entrantConnected) disconnect(entrant);
         if (observerConnected) disconnect(observer);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static void requestRoster(RTIambassador observer, Recorder recorder,
         EncoderFactory encoder, FederateHandleFactory handleFactory,
         CallbackModel callbackModel, ObjectInstanceHandle federationObject,
         AttributeHandleSet attributes, AttributeHandle rosterAttribute,
         TransportationTypeHandle reliable, Set<FederateHandle> expected, String description)
         throws Exception {
      recorder.clearReflection();
      observer.requestAttributeValueUpdate(federationObject, attributes, new byte[0]);
      await(observer, callbackModel, () -> expected.equals(
         recorder.currentRoster(encoder, handleFactory, rosterAttribute)), description);
      verifyReflectionMetadata(recorder, federationObject, reliable, description);
   }

   private static void verifyReflectionMetadata(Recorder recorder,
         ObjectInstanceHandle federationObject, TransportationTypeHandle reliable,
         String description) {
      check(federationObject.equals(recorder.reflectedObject)
            && recorder.reflectedValues != null && recorder.reflectedValues.size() == 1
            && recorder.reflectionTag != null && recorder.reflectionTag.length == 0
            && reliable.equals(recorder.reflectionTransportation),
         description + " returned incomplete or non-standard reflection metadata");
   }

   private static Set<FederateHandle> decodeRoster(EncoderFactory encoder,
         FederateHandleFactory handleFactory, byte[] encoded) throws Exception {
      HLAvariableArray<HLAvariableArray<HLAbyte>> references = encoder.createHLAvariableArray(
         outerIndex -> encoder.createHLAvariableArray(innerIndex -> encoder.createHLAbyte()));
      references.decode(encoded);
      Set<FederateHandle> result = new LinkedHashSet<>();
      for (HLAvariableArray<HLAbyte> reference : references) {
         FederateHandle decoded = handleFactory.decode(reference.toByteArray(), 0);
         check(decoded != null, "HLAfederatesInFederation contained a null federate handle");
         check(result.add(decoded), "HLAfederatesInFederation contained a duplicate federate handle");
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

      private Set<FederateHandle> currentRoster(EncoderFactory encoder,
            FederateHandleFactory handleFactory, AttributeHandle attribute) {
         Map<AttributeHandle, byte[]> values = reflectedValues;
         if (values == null || !values.containsKey(attribute)) return null;
         try {
            return decodeRoster(encoder, handleFactory, values.get(attribute));
         } catch (Exception error) {
            throw new AssertionError("could not decode HLAfederatesInFederation", error);
         }
      }

      private void clearReflection() {
         reflectedObject = null;
         reflectedValues = null;
         reflectionTag = null;
         reflectionTransportation = null;
      }
   }
}
