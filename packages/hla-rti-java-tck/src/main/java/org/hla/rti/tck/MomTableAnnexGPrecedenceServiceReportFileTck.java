package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAunicodeString;
import java.lang.reflect.Proxy;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks the public Federate MOM report-file value as service-report switches change. */
final class MomTableAnnexGPrecedenceServiceReportFileTck {
   private static final String MOM_CLASS = "HLAobjectRoot.HLAmanager.HLAfederate";
   private static final String REPORT_DIRECTORY_SETTING = "serviceReportDirectory=";
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private MomTableAnnexGPrecedenceServiceReportFileTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel, RtiConfiguration subjectConfiguration,
         String additionalSettings, String serviceReportDirectory) throws Exception {
      Path reportDirectory = validateServiceReportDirectory(
         subjectConfiguration, additionalSettings, serviceReportDirectory);
      RTIambassador observer = factory.getRtiAmbassador();
      RTIambassador subject = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder recorder = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-mom-report-file-precedence-" + suffix;
      String observerName = "java-tck-report-file-precedence-observer-" + suffix;
      String subjectName = "java-tck-report-file-precedence-subject-" + suffix;
      boolean observerConnected = false;
      boolean subjectConnected = false;
      boolean federationCreated = false;
      boolean observerJoined = false;
      boolean subjectJoined = false;
      boolean subscribed = false;
      ObjectClassHandle momClass = null;
      AttributeHandleSet attributes = null;
      AttributeHandle reportFileAttribute = null;
      ObjectInstanceHandle subjectMomObject = null;
      try {
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         subject.connect(callbacks(), callbackModel, subjectConfiguration);
         subjectConnected = true;
         observer.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         federationCreated = true;
         observer.joinFederationExecution(observerName, "java-tck", federation);
         observerJoined = true;

         momClass = observer.getObjectClassHandle(MOM_CLASS);
         AttributeHandle nameAttribute = observer.getAttributeHandle(momClass, "HLAfederateName");
         reportFileAttribute = observer.getAttributeHandle(momClass, "HLAreportServiceFile");
         AttributeHandle serviceReportingAttribute = observer.getAttributeHandle(
            momClass, "HLAserviceReporting");
         AttributeHandle sendToFileAttribute = observer.getAttributeHandle(
            momClass, "HLAsendServiceReportsToFile");
         JavaTckSupport.check(MOM_CLASS.equals(observer.getObjectClassName(momClass))
               && "HLAfederateName".equals(observer.getAttributeName(momClass, nameAttribute))
               && "HLAreportServiceFile".equals(
                  observer.getAttributeName(momClass, reportFileAttribute))
               && "HLAserviceReporting".equals(
                  observer.getAttributeName(momClass, serviceReportingAttribute))
               && "HLAsendServiceReportsToFile".equals(
                  observer.getAttributeName(momClass, sendToFileAttribute)),
            "standard Federate MOM report-file or service-switch handles did not round-trip");
         attributes = JavaTckSupport.attributeSet(
            observer, nameAttribute, reportFileAttribute,
            serviceReportingAttribute, sendToFileAttribute);
         observer.subscribeObjectClassAttributes(momClass, attributes, "");
         subscribed = true;

         subject.joinFederationExecution(subjectName, "java-tck", federation);
         subjectJoined = true;
         JavaTckSupport.check(!subject.getServiceReportingSwitch()
               && !subject.getSendServiceReportsToFileSwitch(),
            "both service-report switches were expected to start false");

         byte[] encodedSubjectName = encoder.createHLAunicodeString(subjectName).toByteArray();
         AttributeHandle finalReportFileAttribute = reportFileAttribute;
         await(observer, callbackModel,
            () -> recorder.hasAttributeForFederate(
               nameAttribute, encodedSubjectName, finalReportFileAttribute),
            "joined-federate HLAreportServiceFile initial reflection");
         subjectMomObject = recorder.objectForFederate(nameAttribute, encodedSubjectName);
         byte[] initialReportFile = recorder.valuesForObject(subjectMomObject)
            .get(reportFileAttribute);
         String decodedReportFile = decode(encoder, initialReportFile);
         JavaTckSupport.check(decodedReportFile != null && !decodedReportFile.trim().isEmpty(),
            "initial HLAreportServiceFile reflection did not contain a non-empty value");
         Path advertisedReportFile = Path.of(decodedReportFile).toAbsolutePath().normalize();
         JavaTckSupport.check(advertisedReportFile.startsWith(reportDirectory),
            "HLAreportServiceFile did not select a file under the adapter-supplied directory");
         JavaTckSupport.check(Files.isRegularFile(advertisedReportFile),
            "HLAreportServiceFile did not name an existing regular file while both switches were false");
         JavaTckSupport.check(Files.size(advertisedReportFile) > 0L,
            "the join-initialized service-report file was empty while both switches were false");
         requireStaticReportFile(recorder, subjectMomObject, reportFileAttribute,
            initialReportFile);

         int firstReflection = recorder.reflections.size();
         subject.setServiceReportingSwitch(true);
         JavaTckSupport.check(subject.getServiceReportingSwitch(),
            "could not enable Service Reporting");
         awaitAttribute(observer, callbackModel, recorder, firstReflection,
            subjectMomObject, serviceReportingAttribute, "Service Reporting MOM switch update");
         requireStaticReportFile(recorder, subjectMomObject, reportFileAttribute,
            initialReportFile);

         firstReflection = recorder.reflections.size();
         subject.setSendServiceReportsToFileSwitch(true);
         JavaTckSupport.check(subject.getSendServiceReportsToFileSwitch(),
            "could not enable Send Service Reports To File");
         awaitAttribute(observer, callbackModel, recorder, firstReflection,
            subjectMomObject, sendToFileAttribute,
            "Send Service Reports To File MOM switch update");
         requireStaticReportFile(recorder, subjectMomObject, reportFileAttribute,
            initialReportFile);

         firstReflection = recorder.reflections.size();
         subject.setSendServiceReportsToFileSwitch(false);
         JavaTckSupport.check(!subject.getSendServiceReportsToFileSwitch(),
            "could not disable Send Service Reports To File");
         awaitAttribute(observer, callbackModel, recorder, firstReflection,
            subjectMomObject, sendToFileAttribute,
            "Send Service Reports To File disable MOM switch update");
         requireStaticReportFile(recorder, subjectMomObject, reportFileAttribute,
            initialReportFile);

         firstReflection = recorder.reflections.size();
         subject.setSendServiceReportsToFileSwitch(true);
         JavaTckSupport.check(subject.getSendServiceReportsToFileSwitch(),
            "could not re-enable Send Service Reports To File");
         awaitAttribute(observer, callbackModel, recorder, firstReflection,
            subjectMomObject, sendToFileAttribute,
            "Send Service Reports To File re-enable MOM switch update");
         requireStaticReportFile(recorder, subjectMomObject, reportFileAttribute,
            initialReportFile);
      } finally {
         if (subjectJoined) {
            try { subject.setSendServiceReportsToFileSwitch(false); }
            catch (Exception ignored) { }
            try { subject.setServiceReportingSwitch(false); }
            catch (Exception ignored) { }
            resign(subject);
         }
         if (subscribed && observerJoined && momClass != null && attributes != null) {
            try { observer.unsubscribeObjectClassAttributes(momClass, attributes); }
            catch (Exception ignored) { }
         }
         if (observerJoined) resign(observer);
         if (federationCreated) {
            try { observer.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (subjectConnected) disconnect(subject);
         if (observerConnected) disconnect(observer);
      }
   }

   private static Path validateServiceReportDirectory(RtiConfiguration subjectConfiguration,
         String additionalSettings, String serviceReportDirectory) {
      JavaTckSupport.check(serviceReportDirectory != null && !serviceReportDirectory.isEmpty(),
         "adapter-supplied service-report directory is required");
      Path reportDirectory = Path.of(serviceReportDirectory).toAbsolutePath().normalize();
      JavaTckSupport.check(Files.isDirectory(reportDirectory),
         "adapter-supplied service-report directory must exist");
      String expectedSetting = REPORT_DIRECTORY_SETTING + reportDirectory;
      JavaTckSupport.check(subjectConfiguration != null && additionalSettings != null
            && Arrays.asList(additionalSettings.split(";")).contains(expectedSetting),
         "adapter did not apply the expected serviceReportDirectory setting");
      return reportDirectory;
   }

   private static void awaitAttribute(RTIambassador observer, CallbackModel callbackModel,
         Recorder recorder, int firstReflection, ObjectInstanceHandle object,
         AttributeHandle attribute, String description) throws Exception {
      await(observer, callbackModel,
         () -> recorder.reflectedAttributeSince(firstReflection, object, attribute), description);
   }

   private static void requireStaticReportFile(Recorder recorder, ObjectInstanceHandle object,
         AttributeHandle attribute, byte[] initialValue) {
      List<byte[]> values = recorder.attributeValues(object, attribute);
      JavaTckSupport.check(values.size() == 1 && Arrays.equals(initialValue, values.get(0)),
         "service-report switch changes altered or republished static HLAreportServiceFile");
   }

   private static String decode(EncoderFactory encoder, byte[] bytes) throws Exception {
      JavaTckSupport.check(bytes != null, "HLAreportServiceFile reflection omitted its value");
      HLAunicodeString value = encoder.createHLAunicodeString();
      value.decode(bytes);
      return value.getValue();
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
      if ("toString".equals(method)) return "Java TCK MOM report-file precedence callbacks";
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

      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap source) {
         this.object = object;
         this.values = new LinkedHashMap<>();
         for (AttributeHandle attribute : source.keySet()) {
            values.put(attribute, source.get(attribute).clone());
         }
      }
   }

   private static final class Recorder {
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();

      private ObjectInstanceHandle objectForFederate(AttributeHandle nameAttribute, byte[] name) {
         for (Reflection reflection : reflections) {
            byte[] value = reflection.values.get(nameAttribute);
            if (value != null && Arrays.equals(name, value)) return reflection.object;
         }
         return null;
      }

      private boolean hasAttributeForFederate(AttributeHandle nameAttribute, byte[] name,
            AttributeHandle attribute) {
         ObjectInstanceHandle object = objectForFederate(nameAttribute, name);
         return object != null && valuesForObject(object).containsKey(attribute);
      }

      private Map<AttributeHandle, byte[]> valuesForObject(ObjectInstanceHandle object) {
         Map<AttributeHandle, byte[]> values = new LinkedHashMap<>();
         if (object == null) return values;
         for (Reflection reflection : reflections) {
            if (object.equals(reflection.object)) values.putAll(reflection.values);
         }
         return values;
      }

      private boolean reflectedAttributeSince(int firstReflection, ObjectInstanceHandle object,
            AttributeHandle attribute) {
         for (int index = Math.max(0, firstReflection); index < reflections.size(); ++index) {
            Reflection reflection = reflections.get(index);
            if (object.equals(reflection.object) && reflection.values.containsKey(attribute)) {
               return true;
            }
         }
         return false;
      }

      private List<byte[]> attributeValues(ObjectInstanceHandle object, AttributeHandle attribute) {
         List<byte[]> values = new ArrayList<>();
         if (object == null) return values;
         for (Reflection reflection : reflections) {
            if (object.equals(reflection.object) && reflection.values.containsKey(attribute)) {
               values.add(reflection.values.get(attribute));
            }
         }
         return values;
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("reflectAttributeValues".equals(method.getName()) && arguments != null
                     && arguments.length >= 2
                     && arguments[0] instanceof ObjectInstanceHandle
                     && arguments[1] instanceof AttributeHandleValueMap) {
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1]));
               }
               return objectMethod(proxy, method.getName(), arguments);
            });
      }
   }
}
