package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAunicodeString;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.lang.reflect.Proxy;

/** Checks per-join service-report files and both public reporting switches. */
final class ServiceReportFileSwitchLifecycleTck {
   private static final String MOM_CLASS = "HLAobjectRoot.HLAmanager.HLAfederate";
   private static final String DIRECTORY_SETTING = "serviceReportDirectory=";
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private ServiceReportFileSwitchLifecycleTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel, RtiConfiguration configuration, String additionalSettings,
         String serviceReportDirectory) throws Exception {
      JavaTckSupport.check(configuration != null,
         "service-report file lifecycle configuration is required");
      JavaTckSupport.check(serviceReportDirectory != null && !serviceReportDirectory.isEmpty()
            && Files.isDirectory(Path.of(serviceReportDirectory)),
         "adapter-supplied service-report directory must exist");
      String expectedDirectorySetting = DIRECTORY_SETTING + Path.of(serviceReportDirectory);
      JavaTckSupport.check(additionalSettings != null
            && Arrays.asList(additionalSettings.split(";")).contains(expectedDirectorySetting),
         "adapter did not configure the expected service-report directory");

      RTIambassador first = factory.getRtiAmbassador();
      RTIambassador second = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-service-report-file-switch-" + suffix;
      String observerName = "java-tck-service-report-observer-" + suffix;
      String firstName = "java-tck-service-report-first-" + suffix;
      String secondName = "java-tck-service-report-second-" + suffix;
      boolean firstConnected = false;
      boolean secondConnected = false;
      boolean observerConnected = false;
      boolean created = false;
      boolean firstJoined = false;
      boolean secondJoined = false;
      boolean observerJoined = false;
      boolean subscribed = false;
      ObjectClassHandle momClass = null;
      AttributeHandleSet momAttributes = null;
      try {
         first.connect(new NullFederateAmbassador(), callbackModel, configuration);
         firstConnected = true;
         second.connect(new NullFederateAmbassador(), callbackModel, configuration);
         secondConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;

         first.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         observer.joinFederationExecution(observerName, "java-tck", federation);
         observerJoined = true;

         momClass = observer.getObjectClassHandle(MOM_CLASS);
         AttributeHandle nameAttribute = observer.getAttributeHandle(momClass, "HLAfederateName");
         AttributeHandle reportFileAttribute =
            observer.getAttributeHandle(momClass, "HLAreportServiceFile");
         AttributeHandle serviceReportingAttribute =
            observer.getAttributeHandle(momClass, "HLAserviceReporting");
         AttributeHandle sendToFileAttribute =
            observer.getAttributeHandle(momClass, "HLAsendServiceReportsToFile");
         JavaTckSupport.check(MOM_CLASS.equals(observer.getObjectClassName(momClass))
               && "HLAfederateName".equals(observer.getAttributeName(momClass, nameAttribute))
               && "HLAreportServiceFile".equals(
                  observer.getAttributeName(momClass, reportFileAttribute))
               && "HLAserviceReporting".equals(
                  observer.getAttributeName(momClass, serviceReportingAttribute))
               && "HLAsendServiceReportsToFile".equals(
                  observer.getAttributeName(momClass, sendToFileAttribute)),
            "standard Federate MOM service-report handles did not round-trip");
         momAttributes = JavaTckSupport.attributeSet(observer, nameAttribute, reportFileAttribute,
            serviceReportingAttribute, sendToFileAttribute);
         observer.subscribeObjectClassAttributes(momClass, momAttributes, "");
         subscribed = true;

         int firstJoinReflection = recorder.reflections.size();
         first.joinFederationExecution(firstName, "java-tck", federation);
         firstJoined = true;
         second.joinFederationExecution(secondName, "java-tck", federation);
         secondJoined = true;
         JavaTckSupport.check(!first.getServiceReportingSwitch()
               && !first.getSendServiceReportsToFileSwitch()
               && !second.getServiceReportingSwitch()
               && !second.getSendServiceReportsToFileSwitch(),
            "service-report switches did not start disabled for each joined federate");

         byte[] firstNameBytes = encoder.createHLAunicodeString(firstName).toByteArray();
         byte[] secondNameBytes = encoder.createHLAunicodeString(secondName).toByteArray();
         Report[] reports = new Report[2];
         await(callbackModel, new RTIambassador[] {observer, first, second},
            () -> (reports[0] = findReport(recorder, encoder, nameAttribute,
                  reportFileAttribute, firstNameBytes, firstJoinReflection)) != null
               && (reports[1] = findReport(recorder, encoder, nameAttribute,
                  reportFileAttribute, secondNameBytes, firstJoinReflection)) != null,
            "joined federates' HLAreportServiceFile reflections");
         Report firstReport = reports[0];
         Report secondReport = reports[1];
         JavaTckSupport.check(!firstReport.path.equals(secondReport.path)
               && Files.isRegularFile(firstReport.path) && Files.isRegularFile(secondReport.path),
            "distinct joined federates did not receive distinct existing service-report files");
         verifyInitialRecord(firstReport.path, callbackModel, federation, firstName);
         verifyInitialRecord(secondReport.path, callbackModel, federation, secondName);
         requireStaticReportFile(recorder, firstReport, reportFileAttribute);
         requireStaticReportFile(recorder, secondReport, reportFileAttribute);

         first.setServiceReportingSwitch(true);
         first.setSendServiceReportsToFileSwitch(true);
         String firstBefore = readServiceReportFile(firstReport.path);
         first.getObjectClassHandle("HLAobjectRoot");
         String firstEnabled = readServiceReportFile(firstReport.path);
         requireAppendedService(firstBefore, firstEnabled, "first federate initial enable");

         second.setServiceReportingSwitch(true);
         second.setSendServiceReportsToFileSwitch(true);
         String secondBefore = readServiceReportFile(secondReport.path);
         second.getObjectClassHandle("HLAobjectRoot");
         String secondEnabled = readServiceReportFile(secondReport.path);
         requireAppendedService(secondBefore, secondEnabled, "second federate initial enable");
         JavaTckSupport.check(readServiceReportFile(firstReport.path).equals(firstEnabled),
            "a joined federate's service report appended to another member's file");

         first.setSendServiceReportsToFileSwitch(false);
         String fileDisabledBaseline = readServiceReportFile(firstReport.path);
         first.getObjectClassHandle("HLAobjectRoot");
         JavaTckSupport.check(readServiceReportFile(firstReport.path).equals(fileDisabledBaseline),
            "disabling file reporting appended a service report");

         first.setSendServiceReportsToFileSwitch(true);
         String fileReenabledBaseline = readServiceReportFile(firstReport.path);
         first.getObjectClassHandle("HLAobjectRoot");
         String fileReenabled = readServiceReportFile(firstReport.path);
         requireAppendedService(fileReenabledBaseline, fileReenabled,
            "first federate file reporting re-enable");

         first.setServiceReportingSwitch(false);
         String serviceDisabledBaseline = readServiceReportFile(firstReport.path);
         first.getObjectClassHandle("HLAobjectRoot");
         JavaTckSupport.check(readServiceReportFile(firstReport.path).equals(serviceDisabledBaseline),
            "disabling Service Reporting appended a file record while file reporting remained enabled");

         first.setServiceReportingSwitch(true);
         String serviceReenabledBaseline = readServiceReportFile(firstReport.path);
         first.getObjectClassHandle("HLAobjectRoot");
         String serviceReenabled = readServiceReportFile(firstReport.path);
         requireAppendedService(serviceReenabledBaseline, serviceReenabled,
            "first federate Service Reporting re-enable");
         requireStaticReportFile(recorder, firstReport, reportFileAttribute);
         requireStaticReportFile(recorder, secondReport, reportFileAttribute);

         int beforeRejoin = recorder.reflections.size();
         first.resignFederationExecution(ResignAction.NO_ACTION);
         firstJoined = false;
         String priorFileAfterResign = readServiceReportFile(firstReport.path);
         first.joinFederationExecution(firstName, "java-tck", federation);
         firstJoined = true;
         Report[] rejoined = new Report[1];
         await(callbackModel, new RTIambassador[] {observer, first, second},
            () -> (rejoined[0] = findReport(recorder, encoder, nameAttribute,
                  reportFileAttribute, firstNameBytes, beforeRejoin)) != null,
            "rejoined federate's new HLAreportServiceFile reflection");
         JavaTckSupport.check(!rejoined[0].path.equals(firstReport.path)
               && !rejoined[0].path.equals(secondReport.path)
               && Files.isRegularFile(rejoined[0].path),
            "a later join reused an existing joined-federate service-report file");
         verifyInitialRecord(rejoined[0].path, callbackModel, federation, firstName);
         JavaTckSupport.check(readServiceReportFile(firstReport.path).equals(priorFileAfterResign),
            "the prior joined-federate report file changed after resignation and rejoin");
      } finally {
         if (subscribed && observerJoined && momClass != null && momAttributes != null) {
            try { observer.unsubscribeObjectClassAttributes(momClass, momAttributes); }
            catch (Exception ignored) { }
         }
         if (firstJoined) resign(first);
         if (secondJoined) resign(second);
         if (observerJoined) resign(observer);
         if (created) {
            try { first.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (observerConnected) disconnect(observer);
         if (secondConnected) disconnect(second);
         if (firstConnected) disconnect(first);
      }
   }

   private static Report findReport(Recorder recorder, EncoderFactory encoder,
         AttributeHandle nameAttribute, AttributeHandle reportFileAttribute, byte[] name,
         int firstReflection) throws Exception {
      for (int index = Math.max(0, firstReflection); index < recorder.reflections.size(); ++index) {
         Reflection reflection = recorder.reflections.get(index);
         if (!Arrays.equals(name, reflection.values.get(nameAttribute))) continue;
         HLAunicodeString value = encoder.createHLAunicodeString();
         byte[] encodedPath = reflection.values.get(reportFileAttribute);
         JavaTckSupport.check(encodedPath != null,
            "HLAreportServiceFile reflection omitted its value");
         value.decode(encodedPath);
         String path = value.getValue();
         JavaTckSupport.check(path != null && !path.trim().isEmpty(),
            "joined Federate MOM advertised an empty HLAreportServiceFile value");
         return new Report(reflection.object, Path.of(path), encodedPath);
      }
      return null;
   }

   private static void verifyInitialRecord(Path file, CallbackModel callbackModel,
         String federation, String federate) throws Exception {
      String contents = readServiceReportFile(file);
      int lineEnd = contents.indexOf('\n');
      String firstLine = contents.substring(0, lineEnd < 0 ? contents.length() : lineEnd);
      JavaTckSupport.check(firstLine.contains("\"CallbackModel\":\"" + callbackModel.name() + "\"")
            && firstLine.contains(federation) && firstLine.contains(federate),
         "initial service-report record omitted Connect callback-model, federation, or federate identity");
   }

   private static void requireStaticReportFile(Recorder recorder, Report report,
         AttributeHandle reportFileAttribute) {
      List<byte[]> values = recorder.attributeValues(report.object, reportFileAttribute);
      JavaTckSupport.check(values.size() == 1 && Arrays.equals(report.encodedPath, values.get(0)),
         "a service-report switch change altered or republished static HLAreportServiceFile");
   }

   private static void requireAppendedService(String before, String after, String phase) {
      JavaTckSupport.check(after.length() > before.length()
            && after.indexOf("\"HLAservice\":\"GetObjectClassHandle\"", before.length()) >= 0,
         phase + " did not append the GetObjectClassHandle service-report record");
   }

   private static String readServiceReportFile(Path file) throws Exception {
      JavaTckSupport.check(Files.isRegularFile(file),
         "HLAreportServiceFile did not identify an existing file: " + file);
      return Files.readString(file, StandardCharsets.UTF_8);
   }

   private static void await(CallbackModel callbackModel, RTIambassador[] ambassadors,
         Condition condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (!condition.isMet()) {
         if (System.nanoTime() >= deadline) {
            throw new AssertionError("timed out waiting for " + description);
         }
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            for (RTIambassador ambassador : ambassadors) ambassador.evokeCallback(0.01);
         } else {
            Thread.sleep(1L);
         }
      }
   }

   @FunctionalInterface
   private interface Condition { boolean isMet() throws Exception; }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static final class Report {
      private final ObjectInstanceHandle object;
      private final Path path;
      private final byte[] encodedPath;

      private Report(ObjectInstanceHandle object, Path path, byte[] encodedPath) {
         this.object = object;
         this.path = path;
         this.encodedPath = encodedPath.clone();
      }
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final Map<AttributeHandle, byte[]> values = new LinkedHashMap<>();

      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap values) {
         this.object = object;
         values.forEach((handle, value) -> this.values.put(handle, value.clone()));
      }
   }

   private static final class Recorder {
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();

      private List<byte[]> attributeValues(ObjectInstanceHandle object, AttributeHandle attribute) {
         List<byte[]> result = new ArrayList<>();
         for (Reflection reflection : reflections) {
            if (object.equals(reflection.object) && reflection.values.containsKey(attribute)) {
               result.add(reflection.values.get(attribute));
            }
         }
         return result;
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("reflectAttributeValues".equals(method.getName()) && arguments != null
                     && arguments.length >= 2 && arguments[0] instanceof ObjectInstanceHandle
                     && arguments[1] instanceof AttributeHandleValueMap) {
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(method.getName())) return "Java TCK service-report MOM recorder";
                  if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
                  if ("equals".equals(method.getName())) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }
   }
}
