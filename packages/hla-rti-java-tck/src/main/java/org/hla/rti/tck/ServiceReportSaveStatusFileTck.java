package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.FederateHandleSaveStatusPair;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.SaveStatus;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAunicodeString;
import java.lang.reflect.Proxy;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.LinkedHashMap;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/** Checks the Table 5 SaveStatus pair in the advertised service-report file. */
final class ServiceReportSaveStatusFileTck {
   private static final String FEDERATE_MOM_CLASS =
      "HLAobjectRoot.HLAmanager.HLAfederate";
   private static final String REPORT_DIRECTORY_SETTING = "serviceReportDirectory=";
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private ServiceReportSaveStatusFileTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel, RtiConfiguration subjectConfiguration,
         String additionalSettings, String serviceReportDirectory) throws Exception {
      JavaTckSupport.check(serviceReportDirectory != null && !serviceReportDirectory.isEmpty()
            && Files.isDirectory(Path.of(serviceReportDirectory)),
         "adapter-supplied service-report directory must exist");
      String expectedDirectorySetting = REPORT_DIRECTORY_SETTING
         + Path.of(serviceReportDirectory);
      JavaTckSupport.check(subjectConfiguration != null && additionalSettings != null
            && Arrays.asList(additionalSettings.split(";")).contains(expectedDirectorySetting),
         "adapter did not configure the expected serviceReportDirectory setting");

      RTIambassador observer = factory.getRtiAmbassador();
      RTIambassador subject = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      MomRecorder momRecorder = new MomRecorder();
      SaveStatusRecorder saveStatusRecorder = new SaveStatusRecorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-service-report-save-status-" + suffix;
      String subjectName = "java-tck-service-report-save-status-subject-" + suffix;
      String observerName = "java-tck-service-report-save-status-observer-" + suffix;

      boolean observerConnected = false;
      boolean subjectConnected = false;
      boolean federationCreated = false;
      boolean observerJoined = false;
      boolean subjectJoined = false;
      boolean subscribed = false;
      ObjectClassHandle federateMomClass = null;
      hla.rti1516_2025.AttributeHandleSet federateMomAttributes = null;
      try {
         observer.connect(momRecorder.proxy(), callbackModel);
         observerConnected = true;
         subject.connect(saveStatusRecorder.proxy(), callbackModel, subjectConfiguration);
         subjectConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         federationCreated = true;
         FederateHandle subjectHandle = subject.joinFederationExecution(
            subjectName, "java-tck", federation);
         subjectJoined = true;
         FederateHandle observerHandle = observer.joinFederationExecution(
            observerName, "java-tck", federation);
         observerJoined = true;
         JavaTckSupport.check(subjectHandle != null && observerHandle != null,
            "joined federates did not return handles for SaveStatus response verification");

         federateMomClass = observer.getObjectClassHandle(FEDERATE_MOM_CLASS);
         AttributeHandle nameAttribute = observer.getAttributeHandle(
            federateMomClass, "HLAfederateName");
         AttributeHandle reportFileAttribute = observer.getAttributeHandle(
            federateMomClass, "HLAreportServiceFile");
         JavaTckSupport.check(FEDERATE_MOM_CLASS.equals(
               observer.getObjectClassName(federateMomClass))
               && "HLAfederateName".equals(
                  observer.getAttributeName(federateMomClass, nameAttribute))
               && "HLAreportServiceFile".equals(
                  observer.getAttributeName(federateMomClass, reportFileAttribute)),
            "Federate MOM report-file handles did not round-trip by public name");
         federateMomAttributes = JavaTckSupport.attributeSet(
            observer, nameAttribute, reportFileAttribute);
         observer.subscribeObjectClassAttributes(federateMomClass, federateMomAttributes, "");
         subscribed = true;

         byte[] encodedSubjectName =
            encoder.createHLAunicodeString(subjectName).toByteArray();
         await(observer, callbackModel,
            () -> momRecorder.hasAttributeForFederate(
               nameAttribute, encodedSubjectName, reportFileAttribute),
            "subject HLAreportServiceFile MOM reflection");
         ObjectInstanceHandle subjectMomObject =
            momRecorder.objectForFederate(nameAttribute, encodedSubjectName);
         byte[] reportFileValue = momRecorder.valuesForObject(subjectMomObject)
            .get(reportFileAttribute);
         String reportFileName = decode(encoder, reportFileValue);
         JavaTckSupport.check(reportFileName != null && !reportFileName.trim().isEmpty(),
            "subject HLAreportServiceFile reflection did not contain a file name");
         Path reportFile = Path.of(reportFileName);
         JavaTckSupport.check(Files.isRegularFile(reportFile),
            "subject HLAreportServiceFile did not name an existing report file");
         String initialContents = Files.readString(reportFile, StandardCharsets.UTF_8);
         JavaTckSupport.check(!initialContents.isEmpty(),
            "subject HLAreportServiceFile did not contain its initial record");

         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         subject.setSendServiceReportsToFileSwitch(true);
         subject.setServiceReportingSwitch(true);
         String beforeQuery = Files.readString(reportFile, StandardCharsets.UTF_8);
         int callbackCountBeforeQuery = saveStatusRecorder.responses.size();
         subject.queryFederationSaveStatus();
         String afterQuery = Files.readString(reportFile, StandardCharsets.UTF_8);
         String responseRecord = federationSaveStatusResponseRecord(beforeQuery, afterQuery);
         String statusPairs = federateHandleSaveStatusPairSet(responseRecord);
         JavaTckSupport.check(exactPairArray(statusPairs),
            "FederationSaveStatusResponse did not encode exactly the two joined handles "
               + "with the NO_SAVE_IN_PROGRESS SaveStatus field; observed " + statusPairs);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(saveStatusRecorder.responses.size() == callbackCountBeforeQuery,
               "Evoked Federation Save Status callback ran before service-report file inspection");
         }

         await(subject, callbackModel,
            () -> saveStatusRecorder.hasOneExpectedResponseSince(
               callbackCountBeforeQuery, subjectHandle, observerHandle),
            "NO_SAVE_IN_PROGRESS federation save-status callback");

         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         observer.unsubscribeObjectClassAttributes(federateMomClass, federateMomAttributes);
         subscribed = false;
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         observerJoined = false;
         subject.resignFederationExecution(ResignAction.NO_ACTION);
         subjectJoined = false;
         subject.destroyFederationExecution(federation);
         federationCreated = false;
         observer.disconnect();
         observerConnected = false;
         subject.disconnect();
         subjectConnected = false;
      } finally {
         if (subjectJoined) {
            try { subject.setServiceReportingSwitch(false); } catch (Exception ignored) { }
            try { subject.setSendServiceReportsToFileSwitch(false); }
            catch (Exception ignored) { }
            resign(subject);
         }
         if (subscribed && observerJoined && federateMomClass != null
               && federateMomAttributes != null) {
            try {
               observer.unsubscribeObjectClassAttributes(
                  federateMomClass, federateMomAttributes);
            } catch (Exception ignored) { }
         }
         if (observerJoined) resign(observer);
         if (federationCreated && subjectConnected) {
            try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (subjectConnected) disconnect(subject);
         if (observerConnected) disconnect(observer);
      }
   }

   private static String federationSaveStatusResponseRecord(String before, String after) {
      int servicePosition = after.indexOf(
         "\"HLAservice\":\"FederationSaveStatusResponse\"", before.length());
      JavaTckSupport.check(servicePosition >= 0,
         "HLAreportServiceFile did not append FederationSaveStatusResponse");
      int recordStart = after.lastIndexOf('{', servicePosition);
      int recordEnd = after.indexOf('\n', servicePosition);
      JavaTckSupport.check(recordStart >= 0,
         "FederationSaveStatusResponse was not contained in a JSON service-report record");
      if (recordEnd < 0) recordEnd = after.length();
      return after.substring(recordStart, recordEnd).trim();
   }

   private static String federateHandleSaveStatusPairSet(String record) {
      String marker = "\"HLAargumentType\":17,\"HLAargumentName\":"
         + "\"List of joined federates and save status for each\","
         + "\"HLAargumentValue\":[";
      int markerPosition = record.indexOf(marker);
      JavaTckSupport.check(markerPosition >= 0,
         "FederationSaveStatusResponse did not report the Table 5 "
            + "FederateHandleSaveStatusPairSet argument");
      int arrayStart = markerPosition + marker.length() - 1;
      int arrayEnd = record.indexOf(']', arrayStart);
      JavaTckSupport.check(arrayEnd >= 0,
         "FederateHandleSaveStatusPairSet was not encoded as an array");
      return record.substring(arrayStart, arrayEnd + 1);
   }

   private static boolean exactPairArray(String pairs) {
      Pattern pattern = Pattern.compile(
         "^\\[\\{\\\"handle\\\":\\\"([^\\\"]+)\\\","
            + "\\\"status\\\":\\\"NO_SAVE_IN_PROGRESS\\\"\\},"
            + "\\{\\\"handle\\\":\\\"([^\\\"]+)\\\","
            + "\\\"status\\\":\\\"NO_SAVE_IN_PROGRESS\\\"\\}\\]$");
      Matcher matcher = pattern.matcher(pairs);
      return matcher.matches() && !matcher.group(1).equals(matcher.group(2));
   }

   private static String decode(EncoderFactory encoder, byte[] bytes) throws Exception {
      JavaTckSupport.check(bytes != null,
         "HLAreportServiceFile reflection omitted the report-file value");
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

   private static Object objectMethod(Object proxy, String method, Object[] arguments) {
      if ("toString".equals(method)) return "Java TCK service-report file callbacks";
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

   private static final class MomRecorder {
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

   private static final class SaveStatusRecorder {
      private final List<FederateHandleSaveStatusPair[]> responses = new CopyOnWriteArrayList<>();

      private boolean hasOneExpectedResponseSince(int previousCount,
            FederateHandle subjectHandle, FederateHandle observerHandle) {
         if (responses.size() != previousCount + 1) return false;
         FederateHandleSaveStatusPair[] response = responses.get(previousCount);
         if (response.length != 2) return false;
         int subjectMatches = 0;
         int observerMatches = 0;
         for (FederateHandleSaveStatusPair pair : response) {
            if (subjectHandle.equals(pair.handle)) {
               subjectMatches++;
               if (pair.status != SaveStatus.NO_SAVE_IN_PROGRESS) return false;
            } else if (observerHandle.equals(pair.handle)) {
               observerMatches++;
               if (pair.status != SaveStatus.NO_SAVE_IN_PROGRESS) return false;
            }
         }
         return subjectMatches == 1 && observerMatches == 1;
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("federationSaveStatusResponse".equals(method.getName())
                     && arguments != null && arguments.length == 1) {
                  responses.add(((FederateHandleSaveStatusPair[]) arguments[0]).clone());
               }
               return objectMethod(proxy, method.getName(), arguments);
            });
      }
   }
}
