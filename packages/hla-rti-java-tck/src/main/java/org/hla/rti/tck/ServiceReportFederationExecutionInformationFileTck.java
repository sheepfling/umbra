package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederationExecutionInformation;
import hla.rti1516_2025.FederationExecutionInformationSet;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAunicodeString;
import java.lang.reflect.Proxy;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Arrays;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks the Table 5 ListFederationExecutions record in the advertised report file. */
final class ServiceReportFederationExecutionInformationFileTck {
   private static final String FEDERATE_MOM_CLASS =
      "HLAobjectRoot.HLAmanager.HLAfederate";
   private static final String REPORT_DIRECTORY_SETTING = "serviceReportDirectory=";
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private ServiceReportFederationExecutionInformationFileTck() { }

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
      FederationReportRecorder federationRecorder = new FederationReportRecorder();
      String suffix = UUID.randomUUID().toString();
      String firstFederation = "java-tck-service-report-list-primary-" + suffix;
      String secondFederation = "java-tck-service-report-list-secondary-" + suffix;
      String observerName = "java-tck-service-report-list-observer-" + suffix;
      String subjectName = "java-tck-service-report-list-subject-" + suffix;
      Map<String, String> expectedFederations = new HashMap<>();
      expectedFederations.put(firstFederation, timeImplementation);
      expectedFederations.put(secondFederation, timeImplementation);

      boolean observerConnected = false;
      boolean subjectConnected = false;
      boolean firstFederationCreated = false;
      boolean secondFederationCreated = false;
      boolean observerJoined = false;
      boolean subjectJoined = false;
      boolean subscribed = false;
      ObjectClassHandle federateMomClass = null;
      hla.rti1516_2025.AttributeHandleSet federateMomAttributes = null;
      try {
         observer.connect(momRecorder.proxy(), callbackModel);
         observerConnected = true;
         subject.connect(federationRecorder.proxy(), callbackModel, subjectConfiguration);
         subjectConnected = true;
         subject.createFederationExecutionWithMIM(
            firstFederation, new String[] {fom}, mim, timeImplementation);
         firstFederationCreated = true;
         subject.createFederationExecutionWithMIM(
            secondFederation, new String[] {fom}, mim, timeImplementation);
         secondFederationCreated = true;
         subject.joinFederationExecution(subjectName, "java-tck", firstFederation);
         subjectJoined = true;
         observer.joinFederationExecution(observerName, "java-tck", firstFederation);
         observerJoined = true;

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
         String beforeList = Files.readString(reportFile, StandardCharsets.UTF_8);
         subject.listFederationExecutions();
         String afterList = Files.readString(reportFile, StandardCharsets.UTF_8);
         String record = listFederationExecutionsRecord(beforeList, afterList);
         String returnedValue = federationInformationSet(record);
         String expectedFirst = informationRecord(firstFederation, timeImplementation);
         String expectedSecond = informationRecord(secondFederation, timeImplementation);
         String expectedArray = "[" + expectedFirst + "," + expectedSecond + "]";
         String reversedExpectedArray = "[" + expectedSecond + "," + expectedFirst + "]";
         JavaTckSupport.check(returnedValue.equals(expectedArray)
               || returnedValue.equals(reversedExpectedArray),
            "Table 5 FederationExecutionInformationSet did not contain exactly the two "
               + "federation-name/logical-time records");

         await(subject, callbackModel,
            () -> federationRecorder.listsFederationsInSingleReport(expectedFederations),
            "complete listFederationExecutions callback report");

         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         observer.unsubscribeObjectClassAttributes(federateMomClass, federateMomAttributes);
         subscribed = false;
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         observerJoined = false;
         subject.resignFederationExecution(ResignAction.NO_ACTION);
         subjectJoined = false;
         subject.destroyFederationExecution(secondFederation);
         secondFederationCreated = false;
         subject.destroyFederationExecution(firstFederation);
         firstFederationCreated = false;
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
         if (subjectConnected) {
            if (secondFederationCreated) {
               try { subject.destroyFederationExecution(secondFederation); }
               catch (Exception ignored) { }
            }
            if (firstFederationCreated) {
               try { subject.destroyFederationExecution(firstFederation); }
               catch (Exception ignored) { }
            }
         }
         if (subjectConnected) disconnect(subject);
         if (observerConnected) disconnect(observer);
      }
   }

   private static String listFederationExecutionsRecord(String before, String after) {
      int servicePosition = after.indexOf(
         "\"HLAservice\":\"ListFederationExecutions\"", before.length());
      JavaTckSupport.check(servicePosition >= 0,
         "HLAreportServiceFile did not append ListFederationExecutions");
      int previousLineBreak = after.lastIndexOf('\n', servicePosition);
      int recordStart = previousLineBreak < 0 ? 0 : previousLineBreak + 1;
      int recordEnd = after.indexOf('\n', servicePosition);
      if (recordEnd < 0) recordEnd = after.length();
      return after.substring(recordStart, recordEnd).trim();
   }

   private static String federationInformationSet(String record) {
      String marker = "\"HLAargumentType\":22,\"HLAargumentValue\":[";
      int markerPosition = record.indexOf(marker);
      JavaTckSupport.check(markerPosition >= 0,
         "ListFederationExecutions did not use the MIM FederationExecutionInformationSet type");
      int arrayStart = markerPosition + marker.length() - 1;
      int arrayEnd = record.indexOf(']', arrayStart);
      JavaTckSupport.check(arrayEnd >= 0,
         "FederationExecutionInformationSet was not encoded as an array");
      return record.substring(arrayStart, arrayEnd + 1);
   }

   private static String informationRecord(String federation, String timeImplementation) {
      return "{\"federationExecutionName\":\"" + federation
         + "\",\"logicalTimeImplementationName\":\"" + timeImplementation + "\"}";
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

   private static final class FederationReportRecorder {
      private final List<FederationExecutionInformationSet> reports =
         new CopyOnWriteArrayList<>();

      private boolean listsFederationsInSingleReport(Map<String, String> expected) {
         for (FederationExecutionInformationSet report : reports) {
            if (report.size() != expected.size()) continue;
            Map<String, String> actual = new HashMap<>();
            for (FederationExecutionInformation information : report) {
               actual.put(information.federationExecutionName,
                  information.logicalTimeImplementationName);
            }
            if (actual.size() == report.size() && expected.equals(actual)) return true;
         }
         return false;
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("reportFederationExecutions".equals(method.getName())
                     && arguments != null && arguments.length == 1) {
                  reports.add(((FederationExecutionInformationSet) arguments[0]).clone());
               }
               return objectMethod(proxy, method.getName(), arguments);
            });
      }
   }
}
