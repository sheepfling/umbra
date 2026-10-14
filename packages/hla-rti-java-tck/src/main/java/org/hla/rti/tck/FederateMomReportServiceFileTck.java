package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAunicodeString;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.ConcurrentHashMap;
import java.util.function.BooleanSupplier;

/** Checks the standard Federate MOM HLAreportServiceFile value via public reflection. */
final class FederateMomReportServiceFileTck {
   private static final String MOM_CLASS = "HLAobjectRoot.HLAmanager.HLAfederate";
   private static final String FEDERATE_HANDLE = "HLAfederateHandle";
   private static final String FEDERATE_NAME = "HLAfederateName";
   private static final String REPORT_FILE = "HLAreportServiceFile";
   private static final long TIMEOUT_NANOS = 10_000_000_000L;

   private FederateMomReportServiceFileTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador observer = factory.getRtiAmbassador();
      RTIambassador subject = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder recorder = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-federate-mom-report-file-" + suffix;
      String subjectName = "java-tck-report-file-subject-" + suffix;
      boolean observerConnected = false, subjectConnected = false, created = false;
      boolean observerJoined = false, subjectJoined = false, subscribed = false;
      ObjectClassHandle momClass = null;
      AttributeHandleSet attributes = null;
      try {
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         subject.connect(callbacks(), callbackModel);
         subjectConnected = true;
         observer.createFederationExecutionWithMIM(federation, new String[] {fom}, mim,
            timeImplementation);
         created = true;
         observer.joinFederationExecution("java-tck-report-file-observer", "java-tck", federation);
         observerJoined = true;

         momClass = observer.getObjectClassHandle(MOM_CLASS);
         AttributeHandle handleAttribute = observer.getAttributeHandle(momClass, FEDERATE_HANDLE);
         AttributeHandle nameAttribute = observer.getAttributeHandle(momClass, FEDERATE_NAME);
         AttributeHandle reportFileAttribute = observer.getAttributeHandle(momClass, REPORT_FILE);
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(MOM_CLASS.equals(observer.getObjectClassName(momClass))
               && FEDERATE_HANDLE.equals(observer.getAttributeName(momClass, handleAttribute))
               && FEDERATE_NAME.equals(observer.getAttributeName(momClass, nameAttribute))
               && REPORT_FILE.equals(observer.getAttributeName(momClass, reportFileAttribute))
               && "HLAreliable".equals(observer.getTransportationTypeName(reliable)),
            "Federate MOM report-file lookup did not round-trip standard MIM names");
         attributes = JavaTckSupport.attributeSet(observer, handleAttribute, nameAttribute,
            reportFileAttribute);
         observer.subscribeObjectClassAttributes(momClass, attributes, "");
         subscribed = true;

         FederateHandle subjectHandle = subject.joinFederationExecution(
            subjectName, "java-tck", federation);
         subjectJoined = true;
         byte[] encodedSubject = encode(subjectHandle);
         await(observer, callbackModel, () -> recorder.hasInitialValues(
            handleAttribute, nameAttribute, reportFileAttribute, encodedSubject),
            "joined-federate report-file MOM reflection");
         ObjectInstanceHandle momObject = recorder.objectForFederate(
            handleAttribute, encodedSubject);
         Map<AttributeHandle, byte[]> initialValues = recorder.valuesForObject(momObject);
         JavaTckSupport.check(momObject != null
               && momClass.equals(recorder.discoveredObjects.get(momObject))
               && momClass.equals(observer.getKnownObjectClassHandle(momObject))
               && recorder.allReliable(momObject, reliable)
               && initialValues.size() == 3
               && Arrays.equals(encodedSubject, initialValues.get(handleAttribute))
               && subjectName.equals(decode(encoder, initialValues.get(nameAttribute))),
            "initial Federate MOM report-file reflection had unexpected identity or delivery");
         String reportFile = decode(encoder, initialValues.get(reportFileAttribute));
         JavaTckSupport.check(reportFile != null && !reportFile.trim().isEmpty(),
            "HLAreportServiceFile did not expose a non-empty standard Unicode value");

         int reflectionStart = recorder.reflections.size();
         observer.requestAttributeValueUpdate(momObject,
            JavaTckSupport.attributeSet(observer, reportFileAttribute), new byte[0]);
         await(observer, callbackModel, () -> recorder.valueReflectionSince(
            reflectionStart, momObject, reportFileAttribute) != null,
            "requested HLAreportServiceFile MOM reflection");
         Reflection requested = recorder.valueReflectionSince(
            reflectionStart, momObject, reportFileAttribute);
         JavaTckSupport.check(requested != null && requested.values.size() == 1
               && reliable.equals(requested.transportation)
               && reportFile.equals(decode(encoder, requested.values.get(reportFileAttribute))),
            "requested HLAreportServiceFile value changed or used non-reliable delivery");
      } finally {
         if (subscribed && observerJoined && momClass != null && attributes != null) {
            try { observer.unsubscribeObjectClassAttributes(momClass, attributes); }
            catch (Exception ignored) { }
         }
         if (subjectJoined) resign(subject);
         if (observerJoined) resign(observer);
         if (created) {
            try { observer.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (subjectConnected) disconnect(subject);
         if (observerConnected) disconnect(observer);
      }
   }

   private static String decode(EncoderFactory encoder, byte[] bytes) throws Exception {
      JavaTckSupport.check(bytes != null, "HLAreportServiceFile reflection omitted its value");
      HLAunicodeString value = encoder.createHLAunicodeString();
      value.decode(bytes);
      return value.getValue();
   }

   private static byte[] encode(FederateHandle handle) throws Exception {
      byte[] result = new byte[handle.encodedLength()];
      handle.encode(result, 0);
      return result;
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (!condition.getAsBoolean()) {
         if (System.nanoTime() >= deadline) {
            throw new AssertionError("timed out waiting for " + description);
         }
         if (model == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.01);
         else Thread.sleep(1L);
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> objectMethod(proxy, method.getName(), arguments));
   }

   private static Object objectMethod(Object proxy, String method, Object[] arguments) {
      if ("toString".equals(method)) return "Java TCK Federate MOM report-file callbacks";
      if ("hashCode".equals(method)) return System.identityHashCode(proxy);
      if ("equals".equals(method)) return proxy == (arguments == null ? null : arguments[0]);
      return null;
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final Map<AttributeHandle, byte[]> values;
      private final TransportationTypeHandle transportation;

      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap source,
            TransportationTypeHandle transportation) {
         this.object = object;
         this.values = new LinkedHashMap<>();
         for (AttributeHandle attribute : source.keySet()) {
            values.put(attribute, source.get(attribute).clone());
         }
         this.transportation = transportation;
      }
   }

   private static final class Recorder {
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final Map<ObjectInstanceHandle, ObjectClassHandle> discoveredObjects =
         new ConcurrentHashMap<>();

      private Reflection findByFederate(AttributeHandle attribute, byte[] handle) {
         for (Reflection reflection : reflections) {
            byte[] value = reflection.values.get(attribute);
            if (value != null && Arrays.equals(handle, value)) return reflection;
         }
         return null;
      }

      private ObjectInstanceHandle objectForFederate(AttributeHandle attribute, byte[] handle) {
         Reflection reflection = findByFederate(attribute, handle);
         return reflection == null ? null : reflection.object;
      }

      private boolean hasInitialValues(AttributeHandle handleAttribute,
            AttributeHandle nameAttribute, AttributeHandle reportFileAttribute, byte[] handle) {
         ObjectInstanceHandle object = objectForFederate(handleAttribute, handle);
         if (object == null) return false;
         Map<AttributeHandle, byte[]> values = valuesForObject(object);
         return values.containsKey(handleAttribute) && values.containsKey(nameAttribute)
            && values.containsKey(reportFileAttribute);
      }

      private Map<AttributeHandle, byte[]> valuesForObject(ObjectInstanceHandle object) {
         Map<AttributeHandle, byte[]> values = new LinkedHashMap<>();
         if (object == null) return values;
         for (Reflection reflection : reflections) {
            if (object.equals(reflection.object)) values.putAll(reflection.values);
         }
         return values;
      }

      private boolean allReliable(ObjectInstanceHandle object,
            TransportationTypeHandle expectedTransportation) {
         boolean observed = false;
         for (Reflection reflection : reflections) {
            if (object.equals(reflection.object)) {
               observed = true;
               if (!expectedTransportation.equals(reflection.transportation)) return false;
            }
         }
         return observed;
      }

      private Reflection valueReflectionSince(int start, ObjectInstanceHandle object,
            AttributeHandle attribute) {
         for (int index = Math.max(0, start); index < reflections.size(); index++) {
            Reflection reflection = reflections.get(index);
            if (object.equals(reflection.object) && reflection.values.containsKey(attribute)) {
               return reflection;
            }
         }
         return null;
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("discoverObjectInstance".equals(method.getName()) && arguments != null
                     && arguments.length >= 2
                     && arguments[0] instanceof ObjectInstanceHandle
                     && arguments[1] instanceof ObjectClassHandle) {
                  discoveredObjects.put((ObjectInstanceHandle) arguments[0],
                     (ObjectClassHandle) arguments[1]);
               } else if ("reflectAttributeValues".equals(method.getName()) && arguments != null
                     && arguments.length >= 4
                     && arguments[1] instanceof AttributeHandleValueMap) {
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1],
                     (TransportationTypeHandle) arguments[3]));
               }
               return objectMethod(proxy, method.getName(), arguments);
            });
      }
   }
}
