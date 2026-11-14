package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.FederateHandleFactory;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAboolean;
import hla.rti1516_2025.encoding.HLAfixedRecord;
import hla.rti1516_2025.encoding.HLAinteger16BE;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.encoding.HLAunicodeString;
import hla.rti1516_2025.encoding.HLAvariableArray;
import java.lang.reflect.Proxy;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Base64;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks public MOM report-file reflection and receive-order file routing. */
final class ServiceReportReceiveOrderFileRoutingTck {
   private static final String REPORT_CLASS_NAME =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
   private static final String FEDERATE_MOM_CLASS =
      "HLAobjectRoot.HLAmanager.HLAfederate";
   private static final String REPORT_DIRECTORY_SETTING = "serviceReportDirectory=";
   private static final String[] REPORT_PARAMETER_NAMES = {
      "HLAservice", "HLAserviceType", "HLAsuccessIndicator", "HLAsuppliedArguments",
      "HLAreturnedArgument", "HLAexception", "HLAserialNumber", "HLAfederate"
   };
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private ServiceReportReceiveOrderFileRoutingTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String interactionName, String parameterName, CallbackModel callbackModel,
         RtiConfiguration publisherConfiguration, RtiConfiguration receiverConfiguration,
         String additionalSettings, String serviceReportDirectory) throws Exception {
      JavaTckSupport.check(serviceReportDirectory != null && !serviceReportDirectory.isEmpty()
            && Files.isDirectory(Path.of(serviceReportDirectory)),
         "adapter-supplied service-report directory must exist");
      String expectedSetting = REPORT_DIRECTORY_SETTING + Path.of(serviceReportDirectory);
      JavaTckSupport.check(publisherConfiguration != null && receiverConfiguration != null
            && additionalSettings != null
            && Arrays.asList(additionalSettings.split(";")).contains(expectedSetting),
         "adapter did not configure the expected serviceReportDirectory setting");

      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      CallbackRecorder publisherRecorder = new CallbackRecorder();
      CallbackRecorder receiverRecorder = new CallbackRecorder();
      CallbackRecorder observerRecorder = new CallbackRecorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-service-report-receive-order-file-" + suffix;
      String publisherName = "java-tck-service-report-file-publisher-" + suffix;
      String receiverName = "java-tck-service-report-file-receiver-" + suffix;
      String observerName = "java-tck-service-report-file-observer-" + suffix;

      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean observerConnected = false;
      boolean federationCreated = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      boolean observerJoined = false;
      boolean publisherPublished = false;
      boolean receiverSubscribed = false;
      boolean reportSubscribed = false;
      boolean momSubscribed = false;
      InteractionClassHandle publisherClass = null;
      InteractionClassHandle receiverClass = null;
      InteractionClassHandle reportClass = null;
      ObjectClassHandle federateMomClass = null;
      AttributeHandleSet federateMomAttributes = null;
      FederateHandle publisherHandle = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel, publisherConfiguration);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel, receiverConfiguration);
         receiverConnected = true;
         observer.connect(observerRecorder.proxy(), callbackModel);
         observerConnected = true;
         publisher.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         federationCreated = true;
         publisherHandle = publisher.joinFederationExecution(
            publisherName, "java-tck", federation);
         publisherJoined = true;
         receiver.joinFederationExecution(receiverName, "java-tck", federation);
         receiverJoined = true;
         observer.joinFederationExecution(observerName, "java-tck", federation);
         observerJoined = true;

         reportClass = observer.getInteractionClassHandle(REPORT_CLASS_NAME);
         JavaTckSupport.check(reportClass != null
               && REPORT_CLASS_NAME.equals(observer.getInteractionClassName(reportClass)),
            "standard HLAreportServiceInvocation lookup/name round-trip failed");
         Map<String, ParameterHandle> reportParameters = new LinkedHashMap<>();
         for (String name : REPORT_PARAMETER_NAMES) {
            ParameterHandle handle = observer.getParameterHandle(reportClass, name);
            JavaTckSupport.check(handle != null
                  && name.equals(observer.getParameterName(reportClass, handle)),
               "standard service-report parameter lookup/name round-trip failed for " + name);
            reportParameters.put(name, handle);
         }
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(reliable != null
               && "HLAreliable".equals(observer.getTransportationTypeName(reliable)),
            "standard reliable transportation lookup/name round-trip failed");

         publisherClass = publisher.getInteractionClassHandle(interactionName);
         ParameterHandle publisherParameter = publisher.getParameterHandle(
            publisherClass, parameterName);
         receiverClass = receiver.getInteractionClassHandle(interactionName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(
            receiverClass, parameterName);
         JavaTckSupport.check(publisherClass != null && receiverClass != null
               && publisherParameter != null && receiverParameter != null
               && interactionName.equals(publisher.getInteractionClassName(publisherClass))
               && parameterName.equals(publisher.getParameterName(publisherClass,
                  publisherParameter))
               && publisherClass.equals(receiverClass)
               && publisherParameter.equals(receiverParameter),
            "adapter-supplied source interaction did not resolve consistently through the API");

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
            "standard Federate MOM report-file handles did not round-trip by public name");
         federateMomAttributes = JavaTckSupport.attributeSet(
            observer, nameAttribute, reportFileAttribute);

         publisher.setServiceReportingSwitch(false);
         publisher.setSendServiceReportsToFileSwitch(false);
         receiver.setServiceReportingSwitch(false);
         receiver.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);
         observer.subscribeObjectClassAttributes(federateMomClass, federateMomAttributes, "");
         momSubscribed = true;
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;
         publisher.publishInteractionClass(publisherClass);
         publisherPublished = true;
         receiver.subscribeInteractionClass(receiverClass);
         receiverSubscribed = true;

         byte[] publisherNameBytes = encoder.createHLAunicodeString(publisherName).toByteArray();
         byte[] receiverNameBytes = encoder.createHLAunicodeString(receiverName).toByteArray();
         await(observer, callbackModel,
            () -> observerRecorder.hasAttributeForFederate(
               nameAttribute, publisherNameBytes, reportFileAttribute)
               && observerRecorder.hasAttributeForFederate(
                  nameAttribute, receiverNameBytes, reportFileAttribute),
            "publisher and receiver HLAreportServiceFile MOM reflections");

         Path publisherReportFile = Path.of(decode(encoder, observerRecorder.valueForFederate(
            nameAttribute, publisherNameBytes, reportFileAttribute)));
         Path receiverReportFile = Path.of(decode(encoder, observerRecorder.valueForFederate(
            nameAttribute, receiverNameBytes, reportFileAttribute)));
         JavaTckSupport.check(Files.isRegularFile(publisherReportFile)
               && Files.isRegularFile(receiverReportFile),
            "HLAreportServiceFile did not name existing publisher and receiver files");
         JavaTckSupport.check(!publisherReportFile.toAbsolutePath().normalize().equals(
               receiverReportFile.toAbsolutePath().normalize())
               && !Files.isSameFile(publisherReportFile, receiverReportFile),
            "two joined federates advertised the same service-report file identity");

         String initialPublisherContents = read(publisherReportFile);
         String initialReceiverContents = read(receiverReportFile);
         JavaTckSupport.check(!initialPublisherContents.isEmpty()
               && !initialReceiverContents.isEmpty(),
            "HLAreportServiceFile omitted an initial service-report record");
         assertInitialRecord(initialPublisherContents, callbackModel, federation,
            timeImplementation);
         assertInitialRecord(initialReceiverContents, callbackModel, federation,
            timeImplementation);

         publisher.setServiceReportingSwitch(true);
         publisher.setSendServiceReportsToFileSwitch(false);
         ParameterHandleValueMap values = publisher.getParameterHandleValueMapFactory().create(1);
         byte[] payload = new byte[] {0x01, 0x02};
         byte[] tag = new byte[] {'r', 'o', 'i'};
         values.put(publisherParameter, payload);
         String beforeDisabledFileSend = read(publisherReportFile);
         publisher.sendInteraction(publisherClass, values, tag);
         awaitBoth(observer, receiver, callbackModel,
            () -> observerRecorder.interactions.size() >= 1
               && receiverRecorder.interactions.size() >= 1,
            "callback-backed SendInteraction and application interaction");
         JavaTckSupport.check(beforeDisabledFileSend.equals(read(publisherReportFile)),
            "SendInteraction was appended while file reporting was disabled");
         JavaTckSupport.check(observerRecorder.interactions.size() == 1
               && receiverRecorder.interactions.size() == 1,
            "disabled-file-switch SendInteraction delivered unexpected callback counts");
         Interaction callbackReport = observerRecorder.interactions.get(0);
         assertCallbackReport(encoder, observer, callbackReport, reportParameters,
            reportClass, publisherHandle, reliable);
         String parameterMapValue = suppliedParameterMapValue(
            encoder, callbackReport, reportParameters, tag);

         publisher.setServiceReportingSwitch(false);
         publisher.setSendServiceReportsToFileSwitch(true);
         publisher.setServiceReportingSwitch(true);
         JavaTckSupport.check(publisher.getServiceReportingSwitch()
               && publisher.getSendServiceReportsToFileSwitch(),
            "file-backed SendInteraction requires both service-report switches enabled");
         String beforeFileSends = read(publisherReportFile);
         publisher.sendInteraction(publisherClass, values, tag);
         publisher.sendInteraction(publisherClass, values, tag);
         awaitBoth(observer, receiver, callbackModel,
            () -> receiverRecorder.interactions.size() >= 3,
            "two file-backed SendInteraction application callbacks");

         String afterFileSends = read(publisherReportFile);
         JavaTckSupport.check(afterFileSends.startsWith(beforeFileSends),
            "file-backed SendInteraction changed the report-file prefix");
         List<String> appendedRecords = objectRecords(
            afterFileSends.substring(beforeFileSends.length()));
         JavaTckSupport.check(objectRecords(afterFileSends).size() == 3
               && appendedRecords.size() == 2,
            "two file-backed SendInteraction calls did not append exactly two records");
         String expectedMapArgument = "\"HLAargumentType\":40,\"HLAargumentName\":"
            + "\"Constrained set of interaction parameter designator and value pairs\","
            + "\"HLAargumentValue\":" + parameterMapValue;
         for (String record : appendedRecords) {
            JavaTckSupport.check(containsExactlyOne(record,
                  "\"HLAservice\":\"SendInteraction\"")
                  && record.contains(expectedMapArgument),
               "file-backed SendInteraction record omitted its service or type-40 map value");
         }
         JavaTckSupport.check(observerRecorder.interactions.size() == 1,
            "file-enabled SendInteraction delivered HLAreportServiceInvocation to the observer");
         verifyApplicationInteractions(receiverRecorder.interactions, receiverClass,
            receiverParameter, payload, tag, publisherHandle, reliable);

         String publisherBeforeLookup = read(publisherReportFile);
         ParameterHandle publisherReturnedParameter = publisher.getParameterHandle(
            publisherClass, parameterName);
         JavaTckSupport.check(publisherParameter.equals(publisherReturnedParameter),
            "publisher GetParameterHandle returned a different handle");
         assertLookupRecord(publisher, callbackModel, publisherReportFile, publisherBeforeLookup);

         receiver.setSendServiceReportsToFileSwitch(true);
         receiver.setServiceReportingSwitch(true);
         JavaTckSupport.check(receiver.getServiceReportingSwitch()
               && receiver.getSendServiceReportsToFileSwitch(),
            "receiver GetParameterHandle coverage requires both service-report switches enabled");
         String receiverBeforeLookup = read(receiverReportFile);
         String publisherBeforeReceiverLookup = read(publisherReportFile);
         ParameterHandle receiverReturnedParameter = receiver.getParameterHandle(
            receiverClass, parameterName);
         JavaTckSupport.check(publisherParameter.equals(receiverReturnedParameter),
            "receiver GetParameterHandle returned a different handle");
         assertLookupRecord(receiver, callbackModel, receiverReportFile, receiverBeforeLookup);
         JavaTckSupport.check(publisherBeforeReceiverLookup.equals(read(publisherReportFile)),
            "receiver GetParameterHandle was written to the publisher's HLAreportServiceFile");
         JavaTckSupport.check(observerRecorder.interactions.size() == 1,
            "file reporting delivered a service report to a subscribed federate");
      } finally {
         if (publisherJoined) {
            try { publisher.setServiceReportingSwitch(false); } catch (Exception ignored) { }
            try { publisher.setSendServiceReportsToFileSwitch(false); }
            catch (Exception ignored) { }
         }
         if (receiverJoined) {
            try { receiver.setServiceReportingSwitch(false); } catch (Exception ignored) { }
            try { receiver.setSendServiceReportsToFileSwitch(false); }
            catch (Exception ignored) { }
         }
         if (receiverJoined && receiverSubscribed && receiverClass != null) {
            try { receiver.unsubscribeInteractionClass(receiverClass); }
            catch (Exception ignored) { }
         }
         if (publisherJoined && publisherPublished && publisherClass != null) {
            try { publisher.unpublishInteractionClass(publisherClass); }
            catch (Exception ignored) { }
         }
         if (observerJoined && reportSubscribed && reportClass != null) {
            try { observer.unsubscribeInteractionClass(reportClass); }
            catch (Exception ignored) { }
         }
         if (observerJoined && momSubscribed && federateMomClass != null
               && federateMomAttributes != null) {
            try { observer.unsubscribeObjectClassAttributes(federateMomClass, federateMomAttributes); }
            catch (Exception ignored) { }
         }
         if (observerJoined) resign(observer);
         if (receiverJoined) resign(receiver);
         if (publisherJoined) resign(publisher);
         if (federationCreated && publisherConnected) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (observerConnected) disconnect(observer);
         if (receiverConnected) disconnect(receiver);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static void assertInitialRecord(String contents, CallbackModel callbackModel,
         String federation, String timeImplementation) {
      List<String> records = objectRecords(contents);
      JavaTckSupport.check(records.size() == 1,
         "initial HLAreportServiceFile content was not one Configuration record");
      String record = records.get(0);
      String model = callbackModel == CallbackModel.HLA_EVOKED
         ? "HLA_EVOKED" : "HLA_IMMEDIATE";
      JavaTckSupport.check(containsJsonString(record, "CallbackModel", model),
         "initial Configuration did not contain the selected callback-model enumeration");
      String federationAttributes = nestedObjectForMember(record, "HLAmanager.HLAfederation");
      JavaTckSupport.check(federationAttributes != null,
         "initial report omitted the HLAmanager.HLAfederation record");
      JavaTckSupport.check(containsJsonString(
            federationAttributes, "HLAfederationName", federation),
         "HLAmanager.HLAfederation reported the wrong federation name");
      JavaTckSupport.check(nonEmptyJsonString(federationAttributes, "HLARTIversion"),
         "HLAmanager.HLAfederation omitted HLARTIversion");
      JavaTckSupport.check(nonEmptyJsonString(federationAttributes, "HLAMIMDesignator"),
         "HLAmanager.HLAfederation omitted HLAMIMDesignator");
      JavaTckSupport.check(nonEmptyJsonArray(
            federationAttributes, "HLAFOMmoduleDesignatorList"),
         "HLAmanager.HLAfederation omitted its FOM module designator list");
      JavaTckSupport.check(nonEmptyJsonString(
            federationAttributes, "HLAtimeImplementationName")
            && containsJsonString(federationAttributes,
               "HLAtimeImplementationName", timeImplementation),
         "HLAmanager.HLAfederation reported the wrong time implementation name");
      JavaTckSupport.check(containsJsonBoolean(federationAttributes, "HLAautoProvide"),
         "HLAmanager.HLAfederation omitted HLAautoProvide");
   }

   private static void assertCallbackReport(EncoderFactory encoder, RTIambassador observer,
         Interaction report, Map<String, ParameterHandle> parameters,
         InteractionClassHandle reportClass, FederateHandle publisher,
         TransportationTypeHandle reliable) throws Exception {
      JavaTckSupport.check(reportClass.equals(report.interaction)
            && report.parameters.size() == REPORT_PARAMETER_NAMES.length
            && report.tag.length == 0 && report.producer == null
            && report.regionsEmpty && reliable.equals(report.transportation),
         "HLAreportServiceInvocation callback had invalid public interaction metadata");
      for (String name : REPORT_PARAMETER_NAMES) {
         JavaTckSupport.check(report.parameters.containsKey(parameters.get(name)),
            "HLAreportServiceInvocation callback omitted " + name);
      }
      HLAunicodeString service = encoder.createHLAunicodeString();
      service.decode(report.parameters.get(parameters.get("HLAservice")));
      HLAinteger16BE serviceType = encoder.createHLAinteger16BE();
      serviceType.decode(report.parameters.get(parameters.get("HLAserviceType")));
      HLAboolean success = encoder.createHLAboolean();
      success.decode(report.parameters.get(parameters.get("HLAsuccessIndicator")));
      HLAunicodeString exception = encoder.createHLAunicodeString();
      exception.decode(report.parameters.get(parameters.get("HLAexception")));
      HLAinteger32BE serial = encoder.createHLAinteger32BE();
      serial.decode(report.parameters.get(parameters.get("HLAserialNumber")));
      FederateHandleFactory handleFactory = observer.getFederateHandleFactory();
      FederateHandle reportedFederate = handleFactory.decode(
         report.parameters.get(parameters.get("HLAfederate")), 0);
      JavaTckSupport.check("SendInteraction".equals(service.getValue())
            && serviceType.getValue() == 2 && success.getValue()
            && exception.getValue().isEmpty() && serial.getValue() == 0
            && publisher.equals(reportedFederate),
         "successful SendInteraction service report returned the wrong service result");

      HLAfixedRecord returned = fixedRecordPrototype(encoder);
      returned.decode(report.parameters.get(parameters.get("HLAreturnedArgument")));
      JavaTckSupport.check(((HLAinteger32BE) returned.get(0)).getValue() == 34
            && ((HLAunicodeString) returned.get(1)).getValue().isEmpty()
            && "null".equals(((HLAunicodeString) returned.get(2)).getValue()),
         "successful SendInteraction report returned an invalid Null result argument");
   }

   private static String suppliedParameterMapValue(EncoderFactory encoder,
         Interaction report, Map<String, ParameterHandle> parameters,
         byte[] tag) throws Exception {
      HLAfixedRecord prototype = fixedRecordPrototype(encoder);
      HLAvariableArray<HLAfixedRecord> supplied = encoder.createHLAvariableArray(
         index -> fixedRecordPrototype(encoder), prototype);
      supplied.decode(report.parameters.get(parameters.get("HLAsuppliedArguments")));
      JavaTckSupport.check(supplied.size() == 4,
         "SendInteraction report did not contain its four supplied arguments");
      HLAfixedRecord classArgument = supplied.get(0);
      String classValue = ((HLAunicodeString) classArgument.get(2)).getValue();
      JavaTckSupport.check(classArgument.size() == 3
            && ((HLAinteger32BE) classArgument.get(0)).getValue() == 27
            && "Interaction class designator".equals(
               ((HLAunicodeString) classArgument.get(1)).getValue())
            && classValue.length() >= 2 && classValue.startsWith("\"")
            && classValue.endsWith("\""),
         "SendInteraction report returned an invalid interaction-class argument");
      String mapValue = mapArgumentValue(supplied.get(1));
      JavaTckSupport.check(isOneEntryMapWithValue(mapValue, "AQI="),
         "SendInteraction report encoded an invalid parameter/value map");
      verifyArgument(supplied.get(1), 40,
         "Constrained set of interaction parameter designator and value pairs", mapValue);
      verifyArgument(supplied.get(2), 60, "User-supplied tag",
         quoted(Base64.getEncoder().encodeToString(tag)));
      verifyArgument(supplied.get(3), 34, "Optional timestamp", "null");
      return mapValue;
   }

   private static String mapArgumentValue(HLAfixedRecord argument) {
      JavaTckSupport.check(argument.size() == 3
            && ((HLAinteger32BE) argument.get(0)).getValue() == 40
            && "Constrained set of interaction parameter designator and value pairs".equals(
               ((HLAunicodeString) argument.get(1)).getValue()),
         "SendInteraction report type-40 supplied argument had an invalid shape");
      return ((HLAunicodeString) argument.get(2)).getValue();
   }

   private static boolean isOneEntryMapWithValue(String json, String expectedValue) {
      if (!json.startsWith("{\"") || !json.endsWith("}")) return false;
      int separator = json.indexOf("\":\"");
      if (separator < 3 || json.indexOf("\":\"", separator + 3) >= 0
            || json.indexOf(',') >= 0) return false;
      String key = json.substring(2, separator);
      String value = json.substring(separator + 3, json.length() - 2);
      return !key.isEmpty() && expectedValue.equals(value);
   }

   private static HLAfixedRecord fixedRecordPrototype(EncoderFactory encoder) {
      HLAfixedRecord record = encoder.createHLAfixedRecord();
      record.add(encoder.createHLAinteger32BE());
      record.add(encoder.createHLAunicodeString());
      record.add(encoder.createHLAunicodeString());
      return record;
   }

   private static void verifyArgument(HLAfixedRecord argument, int type,
         String name, String value) {
      JavaTckSupport.check(argument.size() == 3
            && ((HLAinteger32BE) argument.get(0)).getValue() == type
            && name.equals(((HLAunicodeString) argument.get(1)).getValue())
            && value.equals(((HLAunicodeString) argument.get(2)).getValue()),
         "SendInteraction report returned an unexpected supplied argument");
   }

   private static void verifyApplicationInteractions(List<Interaction> interactions,
         InteractionClassHandle interactionClass, ParameterHandle parameter,
         byte[] payload, byte[] tag, FederateHandle publisher,
         TransportationTypeHandle reliable) {
      JavaTckSupport.check(interactions.size() == 3,
         "file-routing case did not deliver exactly three application interactions");
      for (Interaction interaction : interactions) {
         JavaTckSupport.check(interactionClass.equals(interaction.interaction)
               && interaction.parameters.size() == 1
               && Arrays.equals(payload, interaction.parameters.get(parameter))
               && Arrays.equals(tag, interaction.tag)
               && publisher.equals(interaction.producer)
               && reliable.equals(interaction.transportation)
               && interaction.regionsEmpty,
            "file-backed SendInteraction returned non-standard application callback data");
      }
   }

   private static void assertLookupRecord(RTIambassador ambassador, CallbackModel model,
         Path reportFile, String before) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      String after = read(reportFile);
      while (after.length() <= before.length()
            || !after.substring(before.length()).contains(
               "\"HLAservice\":\"GetParameterHandle\"")) {
         if (System.nanoTime() >= deadline) {
            throw new AssertionError("GetParameterHandle did not append a service-report record");
         }
         if (model == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.01);
         else Thread.sleep(1L);
         after = read(reportFile);
      }
      JavaTckSupport.check(after.startsWith(before),
         "GetParameterHandle changed the existing HLAreportServiceFile contents");
      String appended = after.substring(before.length());
      JavaTckSupport.check(containsExactlyOne(
            appended, "\"HLAservice\":\"GetParameterHandle\""),
         "GetParameterHandle append did not contain exactly one matching service record");
      JavaTckSupport.check(appended.contains(
            "\"HLAreturnedArgument\":[{\"HLAargumentType\":39,\"HLAargumentValue\":\""),
         "GetParameterHandle append omitted its type-39 returned-argument record");
      JavaTckSupport.check(nonEmptyJsonString(appended, "HLAargumentValue"),
         "GetParameterHandle returned an empty type-39 value");
   }

   private static String decode(EncoderFactory encoder, byte[] bytes) throws Exception {
      JavaTckSupport.check(bytes != null,
         "HLAreportServiceFile reflection omitted its value");
      HLAunicodeString value = encoder.createHLAunicodeString();
      value.decode(bytes);
      return value.getValue();
   }

   private static String read(Path path) throws Exception {
      return Files.readString(path, StandardCharsets.UTF_8);
   }

   private static List<String> objectRecords(String contents) {
      List<String> records = new ArrayList<>();
      int depth = 0;
      int start = -1;
      boolean quoted = false;
      boolean escaped = false;
      for (int index = 0; index < contents.length(); ++index) {
         char value = contents.charAt(index);
         if (quoted) {
            if (escaped) escaped = false;
            else if (value == '\\') escaped = true;
            else if (value == '"') quoted = false;
            continue;
         }
         if (value == '"') {
            quoted = true;
         } else if (value == '{') {
            if (depth == 0) start = index;
            ++depth;
         } else if (value == '}') {
            --depth;
            if (depth < 0) return List.of();
            if (depth == 0) {
               records.add(contents.substring(start, index + 1));
               start = -1;
            }
         }
      }
      return depth == 0 && !quoted && start == -1 ? records : List.of();
   }

   private static String nestedObjectForMember(String record, String name) {
      int marker = record.indexOf("\"" + name + "\"");
      if (marker < 0) return null;
      int start = record.indexOf('{', marker);
      if (start < 0) return null;
      int depth = 0;
      boolean quoted = false;
      boolean escaped = false;
      for (int index = start; index < record.length(); ++index) {
         char value = record.charAt(index);
         if (quoted) {
            if (escaped) escaped = false;
            else if (value == '\\') escaped = true;
            else if (value == '"') quoted = false;
            continue;
         }
         if (value == '"') quoted = true;
         else if (value == '{') ++depth;
         else if (value == '}' && --depth == 0) return record.substring(start, index + 1);
      }
      return null;
   }

   private static boolean containsJsonString(String json, String name, String expected) {
      return json.contains("\"" + name + "\":\"" + escapeJson(expected) + "\"");
   }

   private static boolean nonEmptyJsonString(String json, String name) {
      String marker = "\"" + name + "\":\"";
      int start = json.indexOf(marker);
      if (start < 0) return false;
      int end = json.indexOf('"', start + marker.length());
      return end > start + marker.length();
   }

   private static boolean nonEmptyJsonArray(String json, String name) {
      String marker = "\"" + name + "\":";
      int start = json.indexOf(marker);
      if (start < 0) return false;
      int arrayStart = json.indexOf('[', start + marker.length());
      int arrayEnd = arrayStart < 0 ? -1 : json.indexOf(']', arrayStart + 1);
      return arrayStart >= 0 && arrayEnd > arrayStart + 1;
   }

   private static boolean containsJsonBoolean(String json, String name) {
      String marker = "\"" + name + "\":";
      int start = json.indexOf(marker);
      if (start < 0) return false;
      String remainder = json.substring(start + marker.length());
      return remainder.startsWith("true") || remainder.startsWith("false");
   }

   private static boolean containsExactlyOne(String text, String value) {
      int first = text.indexOf(value);
      return first >= 0 && text.indexOf(value, first + value.length()) < 0;
   }

   private static String quoted(String value) {
      return "\"" + value + "\"";
   }

   private static String escapeJson(String value) {
      return value.replace("\\", "\\\\").replace("\"", "\\\"");
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (!condition.getAsBoolean()) {
         if (System.nanoTime() >= deadline) throw new AssertionError("timed out waiting for " + description);
         if (model == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.01);
         else Thread.sleep(1L);
      }
   }

   private static void awaitBoth(RTIambassador first, RTIambassador second,
         CallbackModel model, BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (!condition.getAsBoolean()) {
         if (System.nanoTime() >= deadline) throw new AssertionError("timed out waiting for " + description);
         if (model == CallbackModel.HLA_EVOKED) {
            first.evokeCallback(0.01);
            second.evokeCallback(0.01);
         } else {
            Thread.sleep(1L);
         }
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

   private static final class Interaction {
      private InteractionClassHandle interaction;
      private Map<ParameterHandle, byte[]> parameters;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private boolean regionsEmpty;
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

   private static final class CallbackRecorder {
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();

      private boolean hasAttributeForFederate(AttributeHandle nameAttribute, byte[] name,
            AttributeHandle valueAttribute) {
         ObjectInstanceHandle object = objectForFederate(nameAttribute, name);
         return valuesForObject(object).containsKey(valueAttribute);
      }

      private ObjectInstanceHandle objectForFederate(AttributeHandle nameAttribute, byte[] name) {
         for (Reflection reflection : reflections) {
            byte[] value = reflection.values.get(nameAttribute);
            if (value != null && Arrays.equals(name, value)) return reflection.object;
         }
         return null;
      }

      private byte[] valueForFederate(AttributeHandle nameAttribute, byte[] name,
            AttributeHandle valueAttribute) {
         ObjectInstanceHandle object = objectForFederate(nameAttribute, name);
         return valuesForObject(object).get(valueAttribute);
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
               } else if ("receiveInteraction".equals(method.getName()) && arguments != null
                     && arguments.length == 6) {
                  Interaction event = new Interaction();
                  event.interaction = (InteractionClassHandle) arguments[0];
                  ParameterHandleValueMap source = (ParameterHandleValueMap) arguments[1];
                  event.parameters = new LinkedHashMap<>();
                  for (ParameterHandle parameter : source.keySet()) {
                     event.parameters.put(parameter, source.get(parameter).clone());
                  }
                  event.tag = ((byte[]) arguments[2]).clone();
                  event.transportation = (TransportationTypeHandle) arguments[3];
                  event.producer = (FederateHandle) arguments[4];
                  event.regionsEmpty = arguments[5] == null
                     || ((RegionHandleSet) arguments[5]).isEmpty();
                  interactions.add(event);
               }
               return objectMethod(proxy, method.getName(), arguments);
            });
      }
   }
}
