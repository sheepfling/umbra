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
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAunicodeString;
import java.lang.reflect.Proxy;
import java.util.Map;
import java.util.UUID;
import java.util.function.BooleanSupplier;

/** Annex C.10 service-usage composition through the public current-FDD value. */
final class FomServiceUsageMergeTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private FomServiceUsageMergeTck() { }

   static void run(RtiFactory factory, String first, String second, String mim,
         String timeImplementation, CallbackModel model, String federateType)
         throws Exception {
      JavaTckSupport.check(!first.equals(second),
         "service-usage merge requires distinct adapter FOM modules");
      String[][] moduleSets = {{first}, {second}, {first, second}, {second, first}};
      boolean[] expectedUsage = {false, true, true, true};
      for (int index = 0; index < moduleSets.length; ++index) {
         String[] modules = moduleSets[index];
         String currentFdd = composedCurrentFdd(factory, modules, mim,
            timeImplementation, model, federateType);
         JavaTckSupport.check(
            FederationMomCurrentFddTck.disconnectServiceUsed(currentFdd)
               == expectedUsage[index],
            "current FDD did not logically OR service usage across FOM modules");
      }
   }

   static String composedCurrentFdd(RtiFactory factory, String[] modules,
         String mim, String timeImplementation, CallbackModel model,
         String federateType) throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-usage-" + UUID.randomUUID();
      boolean creatorConnected = false;
      boolean observerConnected = false;
      boolean observerJoined = false;
      boolean created = false;
      try {
         creator.connect(callbacks(), model);
         creatorConnected = true;
         observer.connect(recorder.proxy(), model);
         observerConnected = true;
         creator.createFederationExecutionWithMIM(federation, modules,
            mim, timeImplementation);
         created = true;
         observer.joinFederationExecution(federateType, federation);
         observerJoined = true;

         ObjectClassHandle federationClass = observer.getObjectClassHandle(
            "HLAobjectRoot.HLAmanager.HLAfederation");
         AttributeHandle currentFdd = observer.getAttributeHandle(
            federationClass, "HLAcurrentFDD");
         AttributeHandleSet attributes = JavaTckSupport.attributeSet(observer, currentFdd);
         observer.subscribeObjectClassAttributes(federationClass, attributes);
         await(observer, model, () -> recorder.discoveredObject != null,
            "federation MOM object discovery");
         JavaTckSupport.check(federationClass.equals(recorder.discoveredClass)
               && "HLAfederation".equals(recorder.discoveredName),
            "discovered the wrong federation MOM object");

         recorder.reflectedObject = null;
         recorder.reflectedValues = null;
         observer.requestAttributeValueUpdate(recorder.discoveredObject,
            attributes, new byte[0]);
         await(observer, model, () -> recorder.currentFdd(encoder, currentFdd) != null,
            "composed current FDD");
         JavaTckSupport.check(recorder.discoveredObject.equals(recorder.reflectedObject),
            "current FDD arrived for the wrong federation MOM object");
         return recorder.currentFdd(encoder, currentFdd);
      } finally {
         if (observerJoined) try {
            observer.resignFederationExecution(ResignAction.NO_ACTION);
         } catch (Exception ignored) { }
         if (created) try { creator.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (observerConnected) try { observer.disconnect(); }
            catch (Exception ignored) { }
         if (creatorConnected) try { creator.disconnect(); }
            catch (Exception ignored) { }
      }
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean()) {
         if (System.nanoTime() >= deadline) {
            throw new AssertionError("timed out waiting for " + description);
         }
         if (model == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.01);
         else Thread.sleep(1L);
      }
   }

   private static FederateAmbassador callbacks() {
      return new Recorder().proxy();
   }

   private static final class Recorder {
      private volatile ObjectInstanceHandle discoveredObject;
      private volatile ObjectClassHandle discoveredClass;
      private volatile String discoveredName;
      private volatile ObjectInstanceHandle reflectedObject;
      private volatile Map<AttributeHandle, byte[]> reflectedValues;

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
                     && arguments != null && arguments.length == 6
                     && arguments[1] instanceof AttributeHandleValueMap) {
                  reflectedObject = (ObjectInstanceHandle) arguments[0];
                  AttributeHandleValueMap values = (AttributeHandleValueMap) arguments[1];
                  java.util.Map<AttributeHandle, byte[]> copy = new java.util.LinkedHashMap<>();
                  for (AttributeHandle handle : values.keySet()) {
                     copy.put(handle, values.get(handle).clone());
                  }
                  reflectedValues = copy;
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(method.getName())) return "Java TCK FDD callbacks";
                  if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
                  if ("equals".equals(method.getName())) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }

      private String currentFdd(EncoderFactory encoder, AttributeHandle attribute) {
         Map<AttributeHandle, byte[]> values = reflectedValues;
         if (values == null || !values.containsKey(attribute)) return null;
         try {
            HLAunicodeString decoded = encoder.createHLAunicodeString();
            decoded.decode(values.get(attribute));
            return decoded.getValue();
         } catch (Exception error) {
            throw new AssertionError("could not decode standard HLAcurrentFDD", error);
         }
      }
   }
}
