package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.InteractionClassHandleSet;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TimeQueryReturn;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAboolean;
import hla.rti1516_2025.encoding.HLAfixedRecord;
import hla.rti1516_2025.encoding.HLAinteger16BE;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.encoding.HLAunicodeString;
import hla.rti1516_2025.encoding.HLAvariableArray;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verifies an ordinary successful service invocation's standard MOM report. */
final class ServiceReportInteractionTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final String FEDERATE_MOM_CLASS_NAME =
      "HLAobjectRoot.HLAmanager.HLAfederate";
   private static final String REPORT_DIRECTORY_SETTING = "serviceReportDirectory=";
   private static final String REPORT_CLASS_NAME =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
   private static final String[] REPORT_PARAMETER_NAMES = {
      "HLAservice", "HLAserviceType", "HLAsuccessIndicator", "HLAsuppliedArguments",
      "HLAreturnedArgument", "HLAexception", "HLAserialNumber", "HLAfederate"
   };

   private ServiceReportInteractionTck() { }

   static void runQueryGalt(RtiFactory factory, String fom, String mim,
         String timeImplementation, CallbackModel callbackModel,
         RtiConfiguration subjectConfiguration, String additionalSettings,
         String serviceReportDirectory) throws Exception {
      validateServiceReportDirectory(
         subjectConfiguration, additionalSettings, serviceReportDirectory);
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador regulator = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder subjectRecorder = new Recorder();
      Recorder regulatorRecorder = new Recorder();
      Recorder observerRecorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-query-galt-" + UUID.randomUUID();
      boolean subjectConnected = false, regulatorConnected = false, observerConnected = false;
      boolean created = false;
      boolean subjectJoined = false, regulatorJoined = false, observerJoined = false;
      boolean reportSubscribed = false, federateMomSubscribed = false;
      InteractionClassHandle reportClass = null;
      ObjectClassHandle federateMomClass = null;
      AttributeHandleSet federateMomAttributes = null;
      FederateHandle subjectHandle = null;
      boolean subjectRegulating = false;
      String subjectName = "java-tck-query-galt-subject-" + UUID.randomUUID();
      try {
         subject.connect(subjectRecorder.proxy(), callbackModel, subjectConfiguration);
         subjectConnected = true;
         regulator.connect(regulatorRecorder.proxy(), callbackModel);
         regulatorConnected = true;
         observer.connect(observerRecorder.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         subjectHandle = subject.joinFederationExecution(subjectName, "tck-type", federation);
         subjectJoined = true;
         regulator.joinFederationExecution("java-tck-query-galt-regulator", "tck-type", federation);
         regulatorJoined = true;
         observer.joinFederationExecution("java-tck-query-galt-observer", "tck-type", federation);
         observerJoined = true;

         federateMomClass = observer.getObjectClassHandle(FEDERATE_MOM_CLASS_NAME);
         AttributeHandle federateNameAttribute = observer.getAttributeHandle(
            federateMomClass, "HLAfederateName");
         AttributeHandle reportFileAttribute = observer.getAttributeHandle(
            federateMomClass, "HLAreportServiceFile");
         JavaTckSupport.check(federateMomClass != null && federateNameAttribute != null
               && reportFileAttribute != null
               && FEDERATE_MOM_CLASS_NAME.equals(
                  observer.getObjectClassName(federateMomClass))
               && "HLAfederateName".equals(
                  observer.getAttributeName(federateMomClass, federateNameAttribute))
               && "HLAreportServiceFile".equals(
                  observer.getAttributeName(federateMomClass, reportFileAttribute)),
            "standard Federate MOM report-file handles did not round-trip by public name");
         federateMomAttributes = observer.getAttributeHandleSetFactory().create();
         federateMomAttributes.add(federateNameAttribute);
         federateMomAttributes.add(reportFileAttribute);
         observer.subscribeObjectClassAttributes(
            federateMomClass, federateMomAttributes, "");
         federateMomSubscribed = true;

         reportClass = observer.getInteractionClassHandle(REPORT_CLASS_NAME);
         JavaTckSupport.check(reportClass != null
               && REPORT_CLASS_NAME.equals(observer.getInteractionClassName(reportClass)),
            "standard Query GALT report interaction lookup/name round-trip failed");
         Map<String, ParameterHandle> reportParameters = new LinkedHashMap<>();
         for (String name : REPORT_PARAMETER_NAMES) {
            ParameterHandle handle = observer.getParameterHandle(reportClass, name);
            JavaTckSupport.check(handle != null
                  && name.equals(observer.getParameterName(reportClass, handle)),
               "standard Query GALT report parameter lookup/name round-trip failed for " + name);
            reportParameters.put(name, handle);
         }
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(reliable != null
               && "HLAreliable".equals(observer.getTransportationTypeName(reliable)),
            "standard Query GALT report reliable-transport lookup/name round-trip failed");

         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         regulator.setServiceReportingSwitch(false);
         regulator.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;

         long momDeadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
         Reflection subjectMom = federateMomReflection(
            observerRecorder, encoder, federateNameAttribute, reportFileAttribute, subjectName);
         while (subjectMom == null && System.nanoTime() < momDeadline) {
            if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(observer);
            else Thread.yield();
            subjectMom = federateMomReflection(
               observerRecorder, encoder, federateNameAttribute, reportFileAttribute, subjectName);
         }
         JavaTckSupport.check(subjectMom != null,
            "timed out waiting for subject HLAreportServiceFile reflection");
         String reportFileName = decodeUnicode(
            encoder, subjectMom.values.get(reportFileAttribute));
         JavaTckSupport.check(reportFileName != null && !reportFileName.trim().isEmpty(),
            "subject HLAreportServiceFile reflection did not contain a file name");
         Path reportFile = Path.of(reportFileName);
         JavaTckSupport.check(Files.isRegularFile(reportFile),
            "subject HLAreportServiceFile did not identify an existing file");
         String fileBeforeQuery = Files.readString(reportFile, StandardCharsets.UTF_8);
         JavaTckSupport.check(!fileBeforeQuery.isEmpty(),
            "subject HLAreportServiceFile did not identify a file with its initial record");

         LogicalTimeFactory<?, ?> timeFactory = regulator.getTimeFactory();
         LogicalTime<?, ?> initial = (LogicalTime<?, ?>) timeFactory.makeInitial();
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         LogicalTimeInterval lookahead = (LogicalTimeInterval) epsilon.add(epsilon);
         lookahead = (LogicalTimeInterval) lookahead.add(epsilon);
         LogicalTime<?, ?> expectedGalt = addTime(initial, lookahead);
         regulator.enableTimeRegulation(lookahead);
         awaitSingleRegulationCallback(regulator, regulatorRecorder, callbackModel);

         observerRecorder.events.clear();
         subject.setServiceReportingSwitch(true);
         TimeQueryReturn actualGalt = subject.queryGALT();
         JavaTckSupport.check(actualGalt.timeIsValid && expectedGalt.equals(actualGalt.time),
            "Query GALT did not return the active regulator's current time plus lookahead");
         Interaction event = awaitServiceReport(observer, observerRecorder, reportParameters,
            encoder, "QueryGALT", callbackModel);
         String reportedTransportation = describeTransportationType(event.transportation);
         JavaTckSupport.check(reportClass.equals(event.interaction)
               && event.parameters.size() == REPORT_PARAMETER_NAMES.length
               && event.tag.length == 0 && reliable.equals(event.transportation)
               && event.regionsEmpty,
            "Query GALT service report returned invalid interaction or callback metadata: class="
               + reportClass.equals(event.interaction) + ", parameterCount="
               + event.parameters.size() + ", tagLength=" + event.tag.length + ", transport="
               + reliable.equals(event.transportation) + " (expected "
               + describeTransportationType(reliable)
               + ", received " + reportedTransportation + "), regionsEmpty="
               + event.regionsEmpty);
         for (String name : REPORT_PARAMETER_NAMES) {
            JavaTckSupport.check(event.parameters.containsKey(reportParameters.get(name)),
               "Query GALT service report omitted standard parameter " + name);
         }
         FederateHandle reportedFederate = observer.getFederateHandleFactory().decode(
            event.parameters.get(reportParameters.get("HLAfederate")), 0);
         JavaTckSupport.check(subjectHandle.equals(reportedFederate),
            "Query GALT service report identified the wrong invoking federate");
         HLAunicodeString service = encoder.createHLAunicodeString();
         service.decode(event.parameters.get(reportParameters.get("HLAservice")));
         HLAinteger16BE serviceType = encoder.createHLAinteger16BE();
         serviceType.decode(event.parameters.get(reportParameters.get("HLAserviceType")));
         HLAboolean success = encoder.createHLAboolean();
         success.decode(event.parameters.get(reportParameters.get("HLAsuccessIndicator")));
         JavaTckSupport.check("QueryGALT".equals(service.getValue())
               && serviceType.getValue() == 4 && success.getValue(),
            "Query GALT service report returned the wrong service, type, or success value");
         HLAunicodeString exception = encoder.createHLAunicodeString();
         exception.decode(event.parameters.get(reportParameters.get("HLAexception")));
         HLAinteger32BE serial = encoder.createHLAinteger32BE();
         serial.decode(event.parameters.get(reportParameters.get("HLAserialNumber")));
         JavaTckSupport.check(exception.getValue().isEmpty() && serial.getValue() == 0,
            "successful Query GALT report returned exception [" + exception.getValue()
               + "] or wrong initial serial " + serial.getValue());

         HLAfixedRecord argumentPrototype = fixedRecordPrototype(encoder);
         HLAvariableArray<HLAfixedRecord> suppliedArguments = encoder.createHLAvariableArray(
            index -> fixedRecordPrototype(encoder), argumentPrototype);
         suppliedArguments.decode(event.parameters.get(reportParameters.get("HLAsuppliedArguments")));
         JavaTckSupport.check(suppliedArguments.size() == 0,
            "Query GALT report returned unexpected supplied arguments");
         HLAfixedRecord returnedArgument = encoder.createHLAfixedRecord();
         returnedArgument.add(encoder.createHLAinteger32BE());
         returnedArgument.add(encoder.createHLAunicodeString());
         returnedArgument.add(encoder.createHLAunicodeString());
         returnedArgument.decode(event.parameters.get(reportParameters.get("HLAreturnedArgument")));
         JavaTckSupport.check(returnedArgument.size() == 3,
            "Query GALT report returned a malformed TimeQueryReturn argument");
         String expectedGaltJson = "{\"timeIsValid\":true,\"time\":"
            + quoted(expectedGalt.toString()) + "}";
         int reportedArgumentType = ((HLAinteger32BE) returnedArgument.get(0)).getValue();
         String reportedArgumentName = ((HLAunicodeString) returnedArgument.get(1)).getValue();
         String reportedGaltJson = ((HLAunicodeString) returnedArgument.get(2)).getValue();
         JavaTckSupport.check(reportedArgumentType == 58
               && "GALT".equals(reportedArgumentName)
               && expectedGaltJson.equals(reportedGaltJson),
            "Query GALT report did not preserve its standard TimeQueryReturn fields: expected type 58, name GALT, value "
               + expectedGaltJson + "; received type " + reportedArgumentType + ", name "
               + reportedArgumentName + ", value " + reportedGaltJson);

         int interactionCount = observerRecorder.events.size();
         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(true);
         subject.setServiceReportingSwitch(true);
         JavaTckSupport.check(subject.getServiceReportingSwitch()
               && subject.getSendServiceReportsToFileSwitch(),
            "Query GALT file-reporting switches did not retain their enabled state");
         TimeQueryReturn fileReportedGalt = subject.queryGALT();
         JavaTckSupport.check(fileReportedGalt.timeIsValid
               && expectedGalt.equals(fileReportedGalt.time),
            "Query GALT returned a wrong or undefined value while file reporting was selected");
         String fileAfterQuery = Files.readString(reportFile, StandardCharsets.UTF_8);
         JavaTckSupport.check(fileAfterQuery.length() > fileBeforeQuery.length(),
            "Query GALT did not append a service-report file record");
         String appendedGaltRecord = appendedRecord(
            fileBeforeQuery, fileAfterQuery, "QueryGALT");
         JavaTckSupport.check(appendedGaltRecord.contains("\"timeIsValid\":true")
               && appendedGaltRecord.contains(
                  "\"time\":" + quoted(expectedGalt.toString())),
            "Query GALT Table 5 file record omitted its valid time-query fields");

         LogicalTimeInterval subjectLookahead = (LogicalTimeInterval) epsilon.add(epsilon);
         subjectLookahead = (LogicalTimeInterval) subjectLookahead.add(epsilon);
         subject.enableTimeRegulation(subjectLookahead);
         subjectRegulating = true;
         awaitSingleRegulationCallback(subject, subjectRecorder, callbackModel);
         String fileBeforeLookaheadQuery = Files.readString(reportFile, StandardCharsets.UTF_8);
         LogicalTimeInterval queriedLookahead = subject.queryLookahead();
         JavaTckSupport.check(subjectLookahead.equals(queriedLookahead),
            "Query Lookahead returned an interval different from the active regulator's lookahead");
         String fileAfterLookaheadQuery = Files.readString(reportFile, StandardCharsets.UTF_8);
         String appendedLookaheadRecord = appendedRecord(
            fileBeforeLookaheadQuery, fileAfterLookaheadQuery, "QueryLookahead");
         JavaTckSupport.check(appendedLookaheadRecord.contains("\"intervalIsValid\":true")
               && appendedLookaheadRecord.contains(
                  "\"interval\":" + quoted(subjectLookahead.toString())),
            "Query Lookahead Table 5 file record omitted its valid interval-query fields");
         JavaTckSupport.check(observerRecorder.events.size() == interactionCount,
            "time-query file reporting unexpectedly delivered an interaction report");
      } finally {
         if (observerJoined && reportSubscribed) {
            try { observer.unsubscribeInteractionClass(reportClass); } catch (Exception ignored) { }
         }
         if (observerJoined && federateMomSubscribed) {
            try {
               observer.unsubscribeObjectClassAttributes(federateMomClass, federateMomAttributes);
            } catch (Exception ignored) { }
         }
         if (subjectJoined) {
            try { subject.setServiceReportingSwitch(false); } catch (Exception ignored) { }
            try { subject.setSendServiceReportsToFileSwitch(false); }
            catch (Exception ignored) { }
            if (subjectRegulating) {
               try { subject.disableTimeRegulation(); } catch (Exception ignored) { }
            }
         }
         if (regulatorJoined) {
            try { regulator.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (observerJoined) {
            try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (regulatorJoined) {
            try { regulator.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (subjectJoined) {
            try { subject.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { subject.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (observerConnected) {
            try { observer.disconnect(); } catch (Exception ignored) { }
         }
         if (regulatorConnected) {
            try { regulator.disconnect(); } catch (Exception ignored) { }
         }
         if (subjectConnected) {
            try { subject.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   static void runNextMessageRequestAvailable(RtiFactory factory, String fom, String mim,
         String interactionName, String parameterName, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder subjectRecorder = new Recorder();
      Recorder observerRecorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-nmra-" + UUID.randomUUID();
      boolean publisherConnected = false, subjectConnected = false, observerConnected = false;
      boolean created = false;
      boolean publisherJoined = false, subjectJoined = false, observerJoined = false;
      boolean reportSubscribed = false, sourceSubscribed = false, published = false;
      InteractionClassHandle reportClass = null, publisherClass = null, subjectClass = null;
      FederateHandle subjectHandle = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         subject.connect(subjectRecorder.proxy(), callbackModel);
         subjectConnected = true;
         observer.connect(observerRecorder.proxy(), callbackModel);
         observerConnected = true;
         publisher.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         publisher.joinFederationExecution("java-tck-nmra-publisher", "tck-type", federation);
         publisherJoined = true;
         subjectHandle = subject.joinFederationExecution(
            "java-tck-nmra-subject", "tck-type", federation);
         subjectJoined = true;
         observer.joinFederationExecution("java-tck-nmra-observer", "tck-type", federation);
         observerJoined = true;

         publisherClass = publisher.getInteractionClassHandle(interactionName);
         ParameterHandle publisherParameter = publisher.getParameterHandle(
            publisherClass, parameterName);
         subjectClass = subject.getInteractionClassHandle(interactionName);
         ParameterHandle subjectParameter = subject.getParameterHandle(subjectClass, parameterName);
         reportClass = observer.getInteractionClassHandle(REPORT_CLASS_NAME);
         JavaTckSupport.check(publisherClass != null && publisherParameter != null
               && subjectClass != null && subjectParameter != null
               && publisherClass.equals(subjectClass) && publisherParameter.equals(subjectParameter),
            "adapter-supplied Next Message Request Available interaction did not resolve equally");
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
            "standard reliable-transport lookup/name round-trip failed");

         for (RTIambassador ambassador : List.of(publisher, subject, observer)) {
            ambassador.setServiceReportingSwitch(false);
            ambassador.setSendServiceReportsToFileSwitch(false);
         }
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;
         publisher.publishInteractionClass(publisherClass);
         published = true;
         subject.subscribeInteractionClass(subjectClass);
         sourceSubscribed = true;
         publisher.changeInteractionOrderType(publisherClass, hla.rti1516_2025.OrderType.TIMESTAMP);

         LogicalTimeFactory<?, ?> publisherTimeFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> subjectTimeFactory = subject.getTimeFactory();
         LogicalTime<?, ?> initial = (LogicalTime<?, ?>) publisherTimeFactory.makeInitial();
         LogicalTimeInterval epsilon = (LogicalTimeInterval) publisherTimeFactory.makeEpsilon();
         LogicalTimeInterval lookahead = (LogicalTimeInterval) epsilon.add(epsilon);
         lookahead = (LogicalTimeInterval) lookahead.add(epsilon);
         lookahead = (LogicalTimeInterval) lookahead.add(epsilon);
         lookahead = (LogicalTimeInterval) lookahead.add(epsilon);
         JavaTckSupport.check(publisherTimeFactory.getName().equals(subjectTimeFactory.getName()),
            "service-report members selected different logical-time factories");

         subject.enableTimeConstrained();
         awaitTimeConstrainedCallback(subject, subjectRecorder, callbackModel);
         publisher.enableTimeRegulation(lookahead);
         awaitSingleRegulationCallback(publisher, publisherRecorder, callbackModel);
         LogicalTime<?, ?> messageTime = addTime(initial,
            (LogicalTimeInterval) lookahead.add(epsilon).add(epsilon));
         LogicalTime<?, ?> requestBoundary = addTime(initial,
            (LogicalTimeInterval) lookahead.add(lookahead));

         ParameterHandleValueMap values = publisher.getParameterHandleValueMapFactory().create(1);
         byte[] payload = new byte[] {0x4e, 0x4d, 0x52};
         byte[] tag = new byte[] {0x54, 0x43, 0x4b};
         values.put(publisherParameter, payload);
         publisher.sendInteraction(publisherClass, values, tag, messageTime);
         JavaTckSupport.check(subjectRecorder.timedInteractionCount == 0,
            "timestamped interaction was delivered before the subject advanced");

         observerRecorder.events.clear();
         subject.setServiceReportingSwitch(true);
         subject.nextMessageRequestAvailable(requestBoundary);
         Interaction report = awaitServiceReport(observer, observerRecorder, reportParameters,
            encoder, "NextMessageRequestAvailable", callbackModel);
         JavaTckSupport.check(reportClass.equals(report.interaction)
               && report.parameters.size() == REPORT_PARAMETER_NAMES.length
               && report.tag.length == 0 && reliable.equals(report.transportation)
               && report.regionsEmpty,
            "Next Message Request Available report had incorrect callback metadata");
         for (String name : REPORT_PARAMETER_NAMES) {
            JavaTckSupport.check(report.parameters.containsKey(reportParameters.get(name)),
               "Next Message Request Available report omitted standard parameter " + name);
         }
         FederateHandle reportedFederate = observer.getFederateHandleFactory().decode(
            report.parameters.get(reportParameters.get("HLAfederate")), 0);
         JavaTckSupport.check(subjectHandle.equals(reportedFederate),
            "Next Message Request Available report identified the wrong invoking federate");
         HLAunicodeString service = encoder.createHLAunicodeString();
         service.decode(report.parameters.get(reportParameters.get("HLAservice")));
         HLAinteger16BE serviceType = encoder.createHLAinteger16BE();
         serviceType.decode(report.parameters.get(reportParameters.get("HLAserviceType")));
         HLAboolean success = encoder.createHLAboolean();
         success.decode(report.parameters.get(reportParameters.get("HLAsuccessIndicator")));
         JavaTckSupport.check("NextMessageRequestAvailable".equals(service.getValue())
               && serviceType.getValue() == 4 && success.getValue(),
            "Next Message Request Available report returned the wrong service, type, or result");
         HLAfixedRecord argumentPrototype = fixedRecordPrototype(encoder);
         HLAvariableArray<HLAfixedRecord> arguments = encoder.createHLAvariableArray(
            index -> fixedRecordPrototype(encoder), argumentPrototype);
         arguments.decode(report.parameters.get(reportParameters.get("HLAsuppliedArguments")));
         JavaTckSupport.check(arguments.size() == 1,
            "Next Message Request Available report returned the wrong supplied-argument count");
         HLAfixedRecord argument = (HLAfixedRecord) arguments.get(0);
         JavaTckSupport.check(((HLAinteger32BE) argument.get(0)).getValue() == 31
               && "Logical time".equals(((HLAunicodeString) argument.get(1)).getValue())
               && quoted(requestBoundary.toString()).equals(
                  ((HLAunicodeString) argument.get(2)).getValue()),
            "Next Message Request Available report did not preserve its standard boundary argument");
         HLAunicodeString exception = encoder.createHLAunicodeString();
         exception.decode(report.parameters.get(reportParameters.get("HLAexception")));
         HLAinteger32BE serial = encoder.createHLAinteger32BE();
         serial.decode(report.parameters.get(reportParameters.get("HLAserialNumber")));
         JavaTckSupport.check(exception.getValue().isEmpty() && serial.getValue() == 0,
            "successful Next Message Request Available report returned an exception or wrong serial");

         publisher.timeAdvanceRequest(addTime(initial, (LogicalTimeInterval) epsilon.add(epsilon)));
         awaitTimedDeliveryAndGrant(publisher, subject, publisherRecorder, subjectRecorder,
            callbackModel);
         JavaTckSupport.check(subjectRecorder.timedInteractionCount == 1
               && messageTime.equals(subjectRecorder.interactionTime)
               && messageTime.equals(subjectRecorder.timeAdvanceGrantTime),
            "Next Message Request Available did not deliver and grant at the queued message time");
      } finally {
         if (subjectJoined) {
            try { subject.setServiceReportingSwitch(false); } catch (Exception ignored) { }
            try { subject.disableTimeConstrained(); } catch (Exception ignored) { }
            if (sourceSubscribed) {
               try { subject.unsubscribeInteractionClass(subjectClass); } catch (Exception ignored) { }
            }
         }
         if (publisherJoined) {
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
            if (published) {
               try { publisher.unpublishInteractionClass(publisherClass); } catch (Exception ignored) { }
            }
         }
         if (observerJoined && reportSubscribed) {
            try { observer.unsubscribeInteractionClass(reportClass); } catch (Exception ignored) { }
         }
         if (observerJoined) {
            try { observer.resignFederationExecution(ResignAction.NO_ACTION); } catch (Exception ignored) { }
         }
         if (subjectJoined) {
            try { subject.resignFederationExecution(ResignAction.NO_ACTION); } catch (Exception ignored) { }
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(ResignAction.NO_ACTION); } catch (Exception ignored) { }
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (observerConnected) { try { observer.disconnect(); } catch (Exception ignored) { } }
         if (subjectConnected) { try { subject.disconnect(); } catch (Exception ignored) { } }
         if (publisherConnected) { try { publisher.disconnect(); } catch (Exception ignored) { } }
      }
   }

   static void run(RtiFactory factory, String fom, String mim, String interactionName,
         String parameterName, String timeImplementation, CallbackModel callbackModel)
         throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder receiverRecorder = new Recorder();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-" + UUID.randomUUID();
      boolean subjectConnected = false, receiverConnected = false, observerConnected = false;
      boolean created = false;
      boolean subjectJoined = false, receiverJoined = false, observerJoined = false;
      boolean sourceSubscribed = false, reportSubscribed = false, published = false;
      InteractionClassHandle reportClass = null, sourceClass = null;
      InteractionClassHandle receiverSourceClass = null;
      FederateHandle subjectHandle = null;
      try {
         subject.connect(callbacks(), callbackModel);
         subjectConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         subjectHandle = subject.joinFederationExecution(
            "java-tck-service-report-subject", "tck-type", federation);
         subjectJoined = true;
         receiver.joinFederationExecution(
            "java-tck-service-report-receiver", "tck-type", federation);
         receiverJoined = true;
         observer.joinFederationExecution(
            "java-tck-service-report-observer", "tck-type", federation);
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

         sourceClass = subject.getInteractionClassHandle(interactionName);
         ParameterHandle sourceParameter = subject.getParameterHandle(sourceClass, parameterName);
         receiverSourceClass = receiver.getInteractionClassHandle(interactionName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(
            receiverSourceClass, parameterName);
         JavaTckSupport.check(sourceClass != null && sourceParameter != null
               && receiverSourceClass != null && receiverParameter != null
               && interactionName.equals(subject.getInteractionClassName(sourceClass))
               && parameterName.equals(subject.getParameterName(sourceClass, sourceParameter))
               && sourceClass.equals(receiverSourceClass)
               && sourceParameter.equals(receiverParameter),
            "adapter-supplied source interaction declaration did not resolve through the API");

         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         receiver.setServiceReportingSwitch(false);
         receiver.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;
         subject.publishInteractionClass(sourceClass);
         published = true;
         receiver.subscribeInteractionClass(receiverSourceClass);
         sourceSubscribed = true;
         recorder.events.clear();
         receiverRecorder.events.clear();

         subject.setServiceReportingSwitch(true);
         ParameterHandleValueMap values =
            subject.getParameterHandleValueMapFactory().create(1);
         byte[] payload = new byte[] {0x01, 0x02};
         byte[] tag = new byte[] {0x72, 0x6f, 0x69};
         values.put(sourceParameter, payload);
         subject.sendInteraction(sourceClass, values, tag);

         awaitDeliveries(receiver, observer, receiverRecorder, recorder,
            callbackModel);
         JavaTckSupport.check(receiverRecorder.events.size() == 1,
            "successful send did not deliver exactly one application interaction");
         Interaction applicationEvent = receiverRecorder.events.get(0);
         JavaTckSupport.check(receiverSourceClass.equals(applicationEvent.interaction)
               && applicationEvent.parameters.size() == 1
               && java.util.Arrays.equals(payload,
                  applicationEvent.parameters.get(receiverParameter))
               && java.util.Arrays.equals(tag, applicationEvent.tag)
               && subjectHandle.equals(applicationEvent.producer)
               && reliable.equals(applicationEvent.transportation)
               && applicationEvent.regionsEmpty,
            "ordinary receive-order interaction callback had invalid class, parameter, bytes, "
               + "tag, producer, transportation, or region metadata");

         JavaTckSupport.check(recorder.events.size() == 1,
            "successful send did not produce exactly one separate MOM callback");
         Interaction event = recorder.events.get(0);
         JavaTckSupport.check(reportClass.equals(event.interaction)
               && event.parameters.size() == REPORT_PARAMETER_NAMES.length,
            "service-report callback had the wrong interaction identity or parameter count");
         for (String name : REPORT_PARAMETER_NAMES) {
            JavaTckSupport.check(event.parameters.containsKey(reportParameters.get(name)),
               "service-report callback omitted standard parameter " + name);
         }
         JavaTckSupport.check(event.tag.length == 0 && reliable.equals(event.transportation)
               && event.regionsEmpty,
            "service-report callback returned invalid tag, transport, or region metadata");
         JavaTckSupport.check(event.producer == null,
            "RTI-originated service-report callback returned a producer federate");

         FederateHandle reportedFederate = observer.getFederateHandleFactory().decode(
            event.parameters.get(reportParameters.get("HLAfederate")), 0);
         JavaTckSupport.check(subjectHandle.equals(reportedFederate),
            "service report identified the wrong joined federate");
         HLAunicodeString service = encoder.createHLAunicodeString();
         service.decode(event.parameters.get(reportParameters.get("HLAservice")));
         JavaTckSupport.check("SendInteraction".equals(service.getValue()),
            "service report returned the wrong service name");
         HLAinteger16BE serviceType = encoder.createHLAinteger16BE();
         serviceType.decode(event.parameters.get(reportParameters.get("HLAserviceType")));
         JavaTckSupport.check(serviceType.getValue() == 2,
            "service report returned the wrong interaction service type");
         HLAboolean success = encoder.createHLAboolean();
         success.decode(event.parameters.get(reportParameters.get("HLAsuccessIndicator")));
         JavaTckSupport.check(success.getValue(),
            "service report did not identify the interaction invocation as successful");
         HLAunicodeString exception = encoder.createHLAunicodeString();
         exception.decode(event.parameters.get(reportParameters.get("HLAexception")));
         JavaTckSupport.check(exception.getValue().isEmpty(),
            "successful service report contained an exception description");
         HLAinteger32BE serial = encoder.createHLAinteger32BE();
         serial.decode(event.parameters.get(reportParameters.get("HLAserialNumber")));
         JavaTckSupport.check(serial.getValue() == 0,
            "first service report did not use serial number zero");
         HLAfixedRecord argumentPrototype = fixedRecordPrototype(encoder);
         HLAvariableArray<HLAfixedRecord> suppliedArguments = encoder.createHLAvariableArray(
            index -> fixedRecordPrototype(encoder), argumentPrototype);
         suppliedArguments.decode(event.parameters.get(
            reportParameters.get("HLAsuppliedArguments")));
         JavaTckSupport.check(suppliedArguments.size() == 4,
            "successful service report omitted the four standard supplied arguments");
         verifyArgument(suppliedArguments.get(0), 27, "Interaction class designator",
            quoted(sourceClass.toString()));
         verifyArgument(suppliedArguments.get(1), 40,
            "Constrained set of interaction parameter designator and value pairs",
            "{\"" + sourceParameter.toString() + "\":\"AQI=\"}");
         verifyArgument(suppliedArguments.get(2), 60, "User-supplied tag", "\"cm9p\"");
         verifyArgument(suppliedArguments.get(3), 34, "Optional timestamp", "null");
         HLAfixedRecord returnedArgument = encoder.createHLAfixedRecord();
         returnedArgument.add(encoder.createHLAinteger32BE());
         returnedArgument.add(encoder.createHLAunicodeString());
         returnedArgument.add(encoder.createHLAunicodeString());
         returnedArgument.decode(event.parameters.get(reportParameters.get("HLAreturnedArgument")));
         JavaTckSupport.check(returnedArgument.size() == 3
               && ((HLAinteger32BE) returnedArgument.get(0)).getValue() == 34
               && ((HLAunicodeString) returnedArgument.get(1)).getValue().isEmpty()
               && "null".equals(((HLAunicodeString) returnedArgument.get(2)).getValue()),
            "successful service report returned an invalid Null result argument");
      } finally {
         if (receiverJoined && sourceSubscribed) {
            try { receiver.unsubscribeInteractionClass(receiverSourceClass); }
            catch (Exception ignored) { }
         }
         if (observerJoined && reportSubscribed) {
            try { observer.unsubscribeInteractionClass(reportClass); } catch (Exception ignored) { }
         }
         if (subjectJoined && published) {
            try { subject.unpublishInteractionClass(sourceClass); } catch (Exception ignored) { }
         }
         if (subjectJoined) {
            try { subject.setServiceReportingSwitch(false); } catch (Exception ignored) { }
         }
         if (observerJoined) {
            try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (receiverJoined) {
            try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (subjectJoined) {
            try { subject.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { subject.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (observerConnected) {
            try { observer.disconnect(); } catch (Exception ignored) { }
         }
         if (receiverConnected) {
            try { receiver.disconnect(); } catch (Exception ignored) { }
         }
         if (subjectConnected) {
            try { subject.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   static void runPublication(RtiFactory factory, String fom, String foreignInteractionFom,
         String mim, String interactionName, String timeImplementation,
         String foreignInteractionName, CallbackModel callbackModel) throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      RTIambassador foreign = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-interaction-publication-" + UUID.randomUUID();
      String foreignFederation = "java-tck-service-report-interaction-foreign-" + UUID.randomUUID();
      boolean subjectConnected = false, observerConnected = false, foreignConnected = false;
      boolean created = false, foreignCreated = false;
      boolean subjectJoined = false, observerJoined = false, foreignJoined = false;
      boolean reportSubscribed = false;
      InteractionClassHandle reportClass = null, sourceClass = null;
      FederateHandle subjectHandle = null;
      try {
         subject.connect(callbacks(), callbackModel);
         subjectConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         foreign.connect(callbacks(), callbackModel);
         foreignConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         foreign.createFederationExecution(
            foreignFederation, foreignInteractionFom, timeImplementation);
         foreignCreated = true;
         subjectHandle = subject.joinFederationExecution(
            "java-tck-interaction-publication-subject", "tck-type", federation);
         subjectJoined = true;
         observer.joinFederationExecution(
            "java-tck-interaction-publication-observer", "tck-type", federation);
         observerJoined = true;
         foreign.joinFederationExecution(
            "java-tck-interaction-publication-foreign", "tck-type", foreignFederation);
         foreignJoined = true;

         sourceClass = subject.getInteractionClassHandle(interactionName);
         JavaTckSupport.check(sourceClass != null
               && interactionName.equals(subject.getInteractionClassName(sourceClass)),
            "adapter-supplied interaction class lookup/name round-trip failed");
         InteractionClassHandle foreignClass =
            foreign.getInteractionClassHandle(foreignInteractionName);
         JavaTckSupport.check(foreignClass != null
               && foreignInteractionName.equals(foreign.getInteractionClassName(foreignClass))
               && !interactionName.equals(foreignInteractionName),
            "adapter-supplied foreign interaction class lookup failed or aliases the primary name");
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

         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);
         JavaTckSupport.expectFailure(
            () -> subject.getInteractionClassName(foreignClass), "InvalidInteractionClassHandle",
            "lookup of an interaction-class handle from a different federation execution");
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;
         subject.setServiceReportingSwitch(true);

         JavaTckSupport.expectFailure(
            () -> subject.publishInteractionClass(foreignClass), "InteractionClassNotDefined",
            "publishing an interaction class from a different federation execution");
         verifyPublicationReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 0, "PublishInteractionClass",
            foreignClass.toString(), false, "InteractionClassNotDefined", callbackModel);
         JavaTckSupport.expectFailure(
            () -> subject.unpublishInteractionClass(foreignClass), "InteractionClassNotDefined",
            "unpublishing an interaction class from a different federation execution");
         verifyPublicationReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 1, "UnpublishInteractionClass",
            foreignClass.toString(), false, "InteractionClassNotDefined", callbackModel);
         subject.publishInteractionClass(sourceClass);
         verifyPublicationReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 2, "PublishInteractionClass",
            sourceClass.toString(), true, "", callbackModel);
         subject.unpublishInteractionClass(sourceClass);
         verifyPublicationReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 3, "UnpublishInteractionClass",
            sourceClass.toString(), true, "", callbackModel);
      } finally {
         if (subjectJoined) {
            try { subject.setServiceReportingSwitch(false); } catch (Exception ignored) { }
         }
         if (observerJoined && reportSubscribed) {
            try { observer.unsubscribeInteractionClass(reportClass); } catch (Exception ignored) { }
         }
         if (observerJoined) {
            try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (foreignJoined) {
            try { foreign.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (subjectJoined) {
            try { subject.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { subject.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (foreignCreated) {
            try { foreign.destroyFederationExecution(foreignFederation); }
            catch (Exception ignored) { }
         }
         if (foreignConnected) {
            try { foreign.disconnect(); } catch (Exception ignored) { }
         }
         if (observerConnected) {
            try { observer.disconnect(); } catch (Exception ignored) { }
         }
         if (subjectConnected) {
            try { subject.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   static void runSubscription(RtiFactory factory, String fom, String foreignInteractionFom,
         String mim, String interactionName, String timeImplementation,
         String foreignInteractionName, CallbackModel callbackModel) throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      RTIambassador foreign = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-interaction-subscription-" + UUID.randomUUID();
      String foreignFederation =
         "java-tck-service-report-interaction-subscription-foreign-" + UUID.randomUUID();
      boolean subjectConnected = false, observerConnected = false, foreignConnected = false;
      boolean created = false, foreignCreated = false;
      boolean subjectJoined = false, observerJoined = false, foreignJoined = false;
      boolean reportSubscribed = false;
      InteractionClassHandle reportClass = null, sourceClass = null;
      FederateHandle subjectHandle = null;
      try {
         subject.connect(callbacks(), callbackModel);
         subjectConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         foreign.connect(callbacks(), callbackModel);
         foreignConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         foreign.createFederationExecution(
            foreignFederation, foreignInteractionFom, timeImplementation);
         foreignCreated = true;
         subjectHandle = subject.joinFederationExecution(
            "java-tck-interaction-subscription-subject", "tck-type", federation);
         subjectJoined = true;
         observer.joinFederationExecution(
            "java-tck-interaction-subscription-observer", "tck-type", federation);
         observerJoined = true;
         foreign.joinFederationExecution(
            "java-tck-interaction-subscription-foreign", "tck-type", foreignFederation);
         foreignJoined = true;

         sourceClass = subject.getInteractionClassHandle(interactionName);
         JavaTckSupport.check(sourceClass != null
               && interactionName.equals(subject.getInteractionClassName(sourceClass)),
            "adapter-supplied interaction class lookup/name round-trip failed");
         InteractionClassHandle foreignClass =
            foreign.getInteractionClassHandle(foreignInteractionName);
         JavaTckSupport.check(foreignClass != null
               && foreignInteractionName.equals(foreign.getInteractionClassName(foreignClass))
               && !interactionName.equals(foreignInteractionName),
            "adapter-supplied foreign interaction class lookup failed or aliases the primary name");
         InteractionClassHandle subjectReportClass =
            subject.getInteractionClassHandle(REPORT_CLASS_NAME);
         JavaTckSupport.check(subjectReportClass != null
               && REPORT_CLASS_NAME.equals(subject.getInteractionClassName(subjectReportClass)),
            "subject HLAreportServiceInvocation lookup/name round-trip failed");
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

         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);
         JavaTckSupport.expectFailure(
            () -> subject.getInteractionClassName(foreignClass), "InvalidInteractionClassHandle",
            "lookup of an interaction-class handle from a different federation execution");
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;
         subject.setServiceReportingSwitch(true);

         JavaTckSupport.expectFailure(
            () -> subject.subscribeInteractionClass(subjectReportClass),
            "FederateServiceInvocationsAreBeingReportedViaMOM",
            "subscribeInteractionClass for HLAreportServiceInvocation while reporting is enabled");
         verifySubscriptionReport(observer, recorder, reportClass, reportParameters, reliable,
            encoder, subjectHandle, 0, "SubscribeInteractionClass", subjectReportClass.toString(),
            false, false, "FederateServiceInvocationsAreBeingReportedViaMOM", callbackModel);

         JavaTckSupport.expectFailure(
            () -> subject.subscribeInteractionClassPassively(foreignClass),
            "InteractionClassNotDefined",
            "passive subscription to an interaction class from a different federation execution");
         verifySubscriptionReport(observer, recorder, reportClass, reportParameters, reliable,
            encoder, subjectHandle, 1, "SubscribeInteractionClass", foreignClass.toString(), true,
            false, "InteractionClassNotDefined", callbackModel);
         JavaTckSupport.expectFailure(
            () -> subject.unsubscribeInteractionClass(foreignClass), "InteractionClassNotDefined",
            "unsubscription of an interaction class from a different federation execution");
         verifySubscriptionReport(observer, recorder, reportClass, reportParameters, reliable,
            encoder, subjectHandle, 2, "UnsubscribeInteractionClass", foreignClass.toString(),
            false, false, "InteractionClassNotDefined", callbackModel);

         subject.subscribeInteractionClassPassively(sourceClass);
         verifySubscriptionReport(observer, recorder, reportClass, reportParameters, reliable,
            encoder, subjectHandle, 3, "SubscribeInteractionClass", sourceClass.toString(), true,
            true, "", callbackModel);
         subject.unsubscribeInteractionClass(sourceClass);
         verifySubscriptionReport(observer, recorder, reportClass, reportParameters, reliable,
            encoder, subjectHandle, 4, "UnsubscribeInteractionClass", sourceClass.toString(),
            false, true, "", callbackModel);

         subject.subscribeInteractionClass(sourceClass);
         verifySubscriptionReport(observer, recorder, reportClass, reportParameters, reliable,
            encoder, subjectHandle, 5, "SubscribeInteractionClass", sourceClass.toString(), false,
            true, "", callbackModel);
         subject.unsubscribeInteractionClass(sourceClass);
         verifySubscriptionReport(observer, recorder, reportClass, reportParameters, reliable,
            encoder, subjectHandle, 6, "UnsubscribeInteractionClass", sourceClass.toString(),
            false, true, "", callbackModel);
      } finally {
         if (subjectJoined) {
            try { subject.setServiceReportingSwitch(false); } catch (Exception ignored) { }
         }
         if (observerJoined && reportSubscribed) {
            try { observer.unsubscribeInteractionClass(reportClass); } catch (Exception ignored) { }
         }
         if (observerJoined) {
            try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (foreignJoined) {
            try { foreign.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (subjectJoined) {
            try { subject.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { subject.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (foreignCreated) {
            try { foreign.destroyFederationExecution(foreignFederation); }
            catch (Exception ignored) { }
         }
         if (foreignConnected) {
            try { foreign.disconnect(); } catch (Exception ignored) { }
         }
         if (observerConnected) {
            try { observer.disconnect(); } catch (Exception ignored) { }
         }
         if (subjectConnected) {
            try { subject.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   static void runDirectedDeclaration(RtiFactory factory, String fom, String foreignFom,
         String mim, String objectClassName, String interactionName,
         String foreignObjectClassName, String foreignInteractionName,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      RTIambassador foreign = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-directed-declaration-" + UUID.randomUUID();
      String foreignFederation =
         "java-tck-service-report-directed-declaration-foreign-" + UUID.randomUUID();
      boolean subjectConnected = false, observerConnected = false, foreignConnected = false;
      boolean created = false, foreignCreated = false;
      boolean subjectJoined = false, observerJoined = false, foreignJoined = false;
      boolean reportSubscribed = false;
      InteractionClassHandle reportClass = null;
      FederateHandle subjectHandle = null;
      try {
         subject.connect(callbacks(), callbackModel);
         subjectConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         foreign.connect(callbacks(), callbackModel);
         foreignConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         foreign.createFederationExecution(foreignFederation, foreignFom, timeImplementation);
         foreignCreated = true;
         subjectHandle = subject.joinFederationExecution(
            "java-tck-directed-declaration-subject", "tck-type", federation);
         subjectJoined = true;
         observer.joinFederationExecution(
            "java-tck-directed-declaration-observer", "tck-type", federation);
         observerJoined = true;
         foreign.joinFederationExecution(
            "java-tck-directed-declaration-foreign", "tck-type", foreignFederation);
         foreignJoined = true;

         ObjectClassHandle objectClass = subject.getObjectClassHandle(objectClassName);
         InteractionClassHandle interactionClass = subject.getInteractionClassHandle(interactionName);
         JavaTckSupport.check(objectClass != null && interactionClass != null
               && objectClassName.equals(subject.getObjectClassName(objectClass))
               && interactionName.equals(subject.getInteractionClassName(interactionClass)),
            "adapter-supplied directed-declaration object/interaction lookup failed");
         ObjectClassHandle foreignObjectClass =
            foreign.getObjectClassHandle(foreignObjectClassName);
         InteractionClassHandle foreignInteractionClass =
            foreign.getInteractionClassHandle(foreignInteractionName);
         JavaTckSupport.check(foreignObjectClass != null && foreignInteractionClass != null
               && foreignObjectClassName.equals(foreign.getObjectClassName(foreignObjectClass))
               && foreignInteractionName.equals(
                  foreign.getInteractionClassName(foreignInteractionClass))
               && !objectClassName.equals(foreignObjectClassName)
               && !interactionName.equals(foreignInteractionName),
            "adapter-supplied foreign object/interaction class lookup failed or aliases the primary name");
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

         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);
         JavaTckSupport.expectFailure(
            () -> subject.getObjectClassName(foreignObjectClass), "InvalidObjectClassHandle",
            "lookup of an object-class handle from a different federation execution");
         JavaTckSupport.expectFailure(
            () -> subject.getInteractionClassName(foreignInteractionClass),
            "InvalidInteractionClassHandle",
            "lookup of an interaction-class handle from a different federation execution");
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;
         subject.setServiceReportingSwitch(true);

         InteractionClassHandleSet emptySet =
            subject.getInteractionClassHandleSetFactory().create();
         InteractionClassHandleSet invalidSet =
            subject.getInteractionClassHandleSetFactory().create();
         invalidSet.add(foreignInteractionClass);
         InteractionClassHandleSet validSet =
            subject.getInteractionClassHandleSetFactory().create();
         validSet.add(interactionClass);
         String invalidSetText = "[\"" + foreignInteractionClass + "\"]";
         String validSetText = "[\"" + interactionClass + "\"]";

         JavaTckSupport.expectFailure(
            () -> subject.subscribeObjectClassDirectedInteractions(foreignObjectClass, emptySet),
            "ObjectClassNotDefined",
            "directed subscription for an object class from a different federation execution");
         verifyDirectedDeclarationReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 0, "SubscribeObjectClassDirectedInteractions",
            foreignObjectClass.toString(), "[]", false, false, "ObjectClassNotDefined",
            callbackModel);
         JavaTckSupport.expectFailure(
            () -> subject.subscribeObjectClassDirectedInteractionsUniversally(
               objectClass, invalidSet),
            "InteractionClassNotDefined",
            "directed subscription containing an interaction class from a different federation execution");
         verifyDirectedDeclarationReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 1, "SubscribeObjectClassDirectedInteractions",
            objectClass.toString(), invalidSetText, true, false, "InteractionClassNotDefined",
            callbackModel);
         JavaTckSupport.expectFailure(
            () -> subject.unsubscribeObjectClassDirectedInteractions(foreignObjectClass, emptySet),
            "ObjectClassNotDefined",
            "directed unsubscription for an object class from a different federation execution");
         verifyDirectedDeclarationReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 2, "UnsubscribeObjectClassDirectedInteractions",
            foreignObjectClass.toString(), "[]", null, false, "ObjectClassNotDefined",
            callbackModel);
         JavaTckSupport.expectFailure(
            () -> subject.unsubscribeObjectClassDirectedInteractions(objectClass, invalidSet),
            "InteractionClassNotDefined",
            "directed unsubscription containing an interaction class from a different federation execution");
         verifyDirectedDeclarationReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 3, "UnsubscribeObjectClassDirectedInteractions",
            objectClass.toString(), invalidSetText, null, false, "InteractionClassNotDefined",
            callbackModel);

         subject.subscribeObjectClassDirectedInteractions(objectClass, validSet);
         verifyDirectedDeclarationReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 4, "SubscribeObjectClassDirectedInteractions",
            objectClass.toString(), validSetText, false, true, "", callbackModel);
         subject.unsubscribeObjectClassDirectedInteractions(objectClass, validSet);
         verifyDirectedDeclarationReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 5, "UnsubscribeObjectClassDirectedInteractions",
            objectClass.toString(), validSetText, null, true, "", callbackModel);
         subject.unsubscribeObjectClassDirectedInteractions(objectClass);
         verifyDirectedDeclarationReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 6, "UnsubscribeObjectClassDirectedInteractions",
            objectClass.toString(), null, null, true, "", callbackModel);
      } finally {
         if (subjectJoined) {
            try { subject.setServiceReportingSwitch(false); } catch (Exception ignored) { }
         }
         if (observerJoined && reportSubscribed) {
            try { observer.unsubscribeInteractionClass(reportClass); } catch (Exception ignored) { }
         }
         if (observerJoined) {
            try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (foreignJoined) {
            try { foreign.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (subjectJoined) {
            try { subject.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { subject.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (foreignCreated) {
            try { foreign.destroyFederationExecution(foreignFederation); }
            catch (Exception ignored) { }
         }
         if (foreignConnected) {
            try { foreign.disconnect(); } catch (Exception ignored) { }
         }
         if (observerConnected) {
            try { observer.disconnect(); } catch (Exception ignored) { }
         }
         if (subjectConnected) {
            try { subject.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   static void runObjectAttributeDeclaration(RtiFactory factory, String fom,
         String multiAttributeFom, String foreignObjectFom, String mim,
         String objectClassName, String attributeName,
         String timeImplementation, String multiAttributeObjectClassName,
         String multiAttributeFirstName, String multiAttributeSecondName,
         String foreignObjectClassName,
         CallbackModel callbackModel) throws Exception {
      String updateRateName = JavaTckSupport.updateRateName();
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      RTIambassador foreign = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-object-attribute-declaration-"
         + UUID.randomUUID();
      String foreignFederation = "java-tck-service-report-object-attribute-foreign-"
         + UUID.randomUUID();
      boolean subjectConnected = false, observerConnected = false, foreignConnected = false;
      boolean created = false, foreignCreated = false;
      boolean subjectJoined = false, observerJoined = false, foreignJoined = false;
      boolean reportSubscribed = false;
      InteractionClassHandle reportClass = null;
      FederateHandle subjectHandle = null;
      try {
         subject.connect(callbacks(), callbackModel);
         subjectConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         foreign.connect(callbacks(), callbackModel);
         foreignConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom, multiAttributeFom}, mim, timeImplementation);
         created = true;
         foreign.createFederationExecution(
            foreignFederation, foreignObjectFom, timeImplementation);
         foreignCreated = true;
         subjectHandle = subject.joinFederationExecution(
            "java-tck-object-attribute-declaration-subject", "tck-type", federation);
         subjectJoined = true;
         observer.joinFederationExecution(
            "java-tck-object-attribute-declaration-observer", "tck-type", federation);
         observerJoined = true;
         foreign.joinFederationExecution(
            "java-tck-object-attribute-declaration-foreign", "tck-type", foreignFederation);
         foreignJoined = true;

         ObjectClassHandle objectClass = subject.getObjectClassHandle(objectClassName);
         AttributeHandle attribute = subject.getAttributeHandle(objectClass, attributeName);
         JavaTckSupport.check(objectClass != null && attribute != null
               && objectClassName.equals(subject.getObjectClassName(objectClass))
               && attributeName.equals(subject.getAttributeName(objectClass, attribute)),
            "adapter-supplied object class/attribute lookup failed");
         ObjectClassHandle publicationClass =
            subject.getObjectClassHandle(multiAttributeObjectClassName);
         AttributeHandle firstPublicationAttribute =
            subject.getAttributeHandle(publicationClass, multiAttributeFirstName);
         AttributeHandle secondPublicationAttribute =
            subject.getAttributeHandle(publicationClass, multiAttributeSecondName);
         JavaTckSupport.check(publicationClass != null && firstPublicationAttribute != null
               && secondPublicationAttribute != null
               && !firstPublicationAttribute.equals(secondPublicationAttribute)
               && !publicationClass.equals(objectClass)
               && !firstPublicationAttribute.equals(attribute)
               && multiAttributeObjectClassName.equals(
                  subject.getObjectClassName(publicationClass))
               && multiAttributeFirstName.equals(
                  subject.getAttributeName(publicationClass, firstPublicationAttribute))
               && multiAttributeSecondName.equals(
                  subject.getAttributeName(publicationClass, secondPublicationAttribute)),
            "adapter-supplied multi-attribute publication lookup failed");
         ObjectClassHandle foreignObjectClass =
            foreign.getObjectClassHandle(foreignObjectClassName);
         JavaTckSupport.check(foreignObjectClass != null
               && foreignObjectClassName.equals(
                  foreign.getObjectClassName(foreignObjectClass)),
            "adapter-supplied foreign object-class lookup failed");
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

         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);
         JavaTckSupport.expectFailure(
            () -> subject.getObjectClassName(foreignObjectClass), "InvalidObjectClassHandle",
            "lookup of an object-class handle from a different federation execution");
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;
         subject.setServiceReportingSwitch(true);

         AttributeHandleSet emptyAttributes = subject.getAttributeHandleSetFactory().create();
         AttributeHandleSet unrelatedAttributes =
            subject.getAttributeHandleSetFactory().create();
         unrelatedAttributes.add(firstPublicationAttribute);
         String emptyAttributeSetText = "[]";
         String unrelatedAttributeSetText = "[\"" + firstPublicationAttribute + "\"]";
         JavaTckSupport.expectFailure(
            () -> subject.subscribeObjectClassAttributesPassively(
               foreignObjectClass, emptyAttributes, "High"),
            "ObjectClassNotDefined",
            "passive subscription with an object class from a different federation execution");
         verifyObjectAttributeReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 0, "SubscribeObjectClassAttributes",
            foreignObjectClass.toString(), emptyAttributeSetText, true, "High", false,
            "ObjectClassNotDefined", callbackModel);
         JavaTckSupport.expectFailure(
            () -> subject.subscribeObjectClassAttributesPassively(
               objectClass, unrelatedAttributes, "High"),
            "AttributeNotDefined",
            "passive subscription with an attribute from a different object class");
         verifyObjectAttributeReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 1, "SubscribeObjectClassAttributes",
            objectClass.toString(), unrelatedAttributeSetText, true, "High", false,
            "AttributeNotDefined", callbackModel);
         JavaTckSupport.expectFailure(
            () -> subject.unsubscribeObjectClassAttributes(foreignObjectClass, emptyAttributes),
            "ObjectClassNotDefined",
            "attribute-set unsubscription with an object class from a different federation execution");
         verifyObjectAttributeReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 2, "UnsubscribeObjectClassAttributes",
            foreignObjectClass.toString(), emptyAttributeSetText, null, null, false,
            "ObjectClassNotDefined", callbackModel);
         JavaTckSupport.expectFailure(
            () -> subject.unsubscribeObjectClassAttributes(objectClass, unrelatedAttributes),
            "AttributeNotDefined",
            "attribute-set unsubscription with an attribute from a different object class");
         verifyObjectAttributeReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 3, "UnsubscribeObjectClassAttributes",
            objectClass.toString(), unrelatedAttributeSetText, null, null, false,
            "AttributeNotDefined", callbackModel);

         JavaTckSupport.expectFailure(
            () -> subject.unsubscribeObjectClass(foreignObjectClass), "ObjectClassNotDefined",
            "whole-class unsubscription with an object class from a different federation execution");
         verifyObjectAttributeReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 4, "UnsubscribeObjectClassAttributes",
            foreignObjectClass.toString(), null, null, null, false, "ObjectClassNotDefined",
            callbackModel);

         AttributeHandleSet validSet = subject.getAttributeHandleSetFactory().create();
         validSet.add(attribute);
         String objectText = objectClass.toString();
         String validSetText = "[\"" + attribute + "\"]";

         subject.subscribeObjectClassAttributesPassively(objectClass, validSet, updateRateName);
         verifyObjectAttributeReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 5, "SubscribeObjectClassAttributes",
            objectText, validSetText, true, updateRateName, true, "", callbackModel);
         subject.unsubscribeObjectClassAttributes(objectClass, validSet);
         verifyObjectAttributeReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 6, "UnsubscribeObjectClassAttributes",
            objectText, validSetText, null, null, true, "", callbackModel);
         subject.unsubscribeObjectClass(objectClass);
         verifyObjectAttributeReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 7, "UnsubscribeObjectClassAttributes",
            objectText, null, null, null, true, "", callbackModel);

         AttributeHandleSet publicationAttributes =
            subject.getAttributeHandleSetFactory().create();
         publicationAttributes.add(firstPublicationAttribute);
         publicationAttributes.add(secondPublicationAttribute);
         JavaTckSupport.check(publicationAttributes.size() == 2,
            "adapter multi-attribute FOM did not provide two distinct publication handles");
         subject.publishObjectClassAttributes(publicationClass, publicationAttributes);
         verifyObjectAttributeReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 8, "PublishObjectClassAttributes",
            publicationClass.toString(), null, null, null, true, "", callbackModel,
            firstPublicationAttribute.toString(), secondPublicationAttribute.toString());
      } finally {
         if (subjectJoined) {
            try { subject.setServiceReportingSwitch(false); } catch (Exception ignored) { }
         }
         if (observerJoined && reportSubscribed) {
            try { observer.unsubscribeInteractionClass(reportClass); } catch (Exception ignored) { }
         }
         if (observerJoined) {
            try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (foreignJoined) {
            try { foreign.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (subjectJoined) {
            try { subject.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { subject.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (foreignCreated) {
            try { foreign.destroyFederationExecution(foreignFederation); }
            catch (Exception ignored) { }
         }
         if (foreignConnected) {
            try { foreign.disconnect(); } catch (Exception ignored) { }
         }
         if (observerConnected) {
            try { observer.disconnect(); } catch (Exception ignored) { }
         }
         if (subjectConnected) {
            try { subject.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void verifyObjectAttributeReport(RTIambassador observer, Recorder recorder,
         InteractionClassHandle reportClass, Map<String, ParameterHandle> reportParameters,
         TransportationTypeHandle reliable, EncoderFactory encoder, FederateHandle subjectHandle,
         int serialNumber, String serviceName, String objectHandle, String attributeSet,
         Boolean passive, String updateRate, boolean expectedSuccess, String expectedException,
         CallbackModel callbackModel) throws Exception {
      verifyObjectAttributeReport(observer, recorder, reportClass, reportParameters,
         reliable, encoder, subjectHandle, serialNumber, serviceName, objectHandle, attributeSet,
         passive, updateRate, expectedSuccess, expectedException, callbackModel, null, null);
   }

   private static void verifyObjectAttributeReport(RTIambassador observer, Recorder recorder,
         InteractionClassHandle reportClass, Map<String, ParameterHandle> reportParameters,
         TransportationTypeHandle reliable, EncoderFactory encoder, FederateHandle subjectHandle,
         int serialNumber, String serviceName, String objectHandle, String attributeSet,
         Boolean passive, String updateRate, boolean expectedSuccess, String expectedException,
         CallbackModel callbackModel, String expectedSetMemberOne,
         String expectedSetMemberTwo) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (recorder.events.isEmpty() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(observer);
         } else {
            Thread.yield();
         }
      }
      JavaTckSupport.check(recorder.events.size() == 1,
         "object-attribute declaration did not produce exactly one MOM report");
      Interaction event = recorder.events.get(0);
      JavaTckSupport.check(reportClass.equals(event.interaction)
            && event.parameters.size() == REPORT_PARAMETER_NAMES.length
            && event.producer == null && event.tag.length == 0
            && reliable.equals(event.transportation)
            && event.regionsEmpty,
         "object-attribute declaration report had invalid identity or callback metadata");
      for (String name : REPORT_PARAMETER_NAMES) {
         JavaTckSupport.check(event.parameters.containsKey(reportParameters.get(name)),
            "service-report callback omitted standard parameter " + name);
      }
      FederateHandle reportedFederate = observer.getFederateHandleFactory().decode(
         event.parameters.get(reportParameters.get("HLAfederate")), 0);
      JavaTckSupport.check(subjectHandle.equals(reportedFederate),
         "object-attribute declaration report identified the wrong invoking federate");
      HLAunicodeString service = encoder.createHLAunicodeString();
      service.decode(event.parameters.get(reportParameters.get("HLAservice")));
      JavaTckSupport.check(serviceName.equals(service.getValue()),
         "object-attribute declaration report returned the wrong service name");
      HLAinteger16BE serviceType = encoder.createHLAinteger16BE();
      serviceType.decode(event.parameters.get(reportParameters.get("HLAserviceType")));
      JavaTckSupport.check(serviceType.getValue() == 1,
         "object-attribute declaration report returned the wrong service type");
      HLAboolean success = encoder.createHLAboolean();
      success.decode(event.parameters.get(reportParameters.get("HLAsuccessIndicator")));
      JavaTckSupport.check(success.getValue() == expectedSuccess,
         "object-attribute declaration report returned the wrong success indicator");
      HLAunicodeString exception = encoder.createHLAunicodeString();
      exception.decode(event.parameters.get(reportParameters.get("HLAexception")));
      JavaTckSupport.check(expectedException.isEmpty()
            ? exception.getValue().isEmpty()
            : exception.getValue().contains(expectedException),
         "object-attribute declaration report returned the wrong exception name");
      HLAinteger32BE serial = encoder.createHLAinteger32BE();
      serial.decode(event.parameters.get(reportParameters.get("HLAserialNumber")));
      JavaTckSupport.check(serial.getValue() == serialNumber,
         "object-attribute declaration report returned the wrong serial number");
      HLAfixedRecord argumentPrototype = fixedRecordPrototype(encoder);
      HLAvariableArray<HLAfixedRecord> suppliedArguments = encoder.createHLAvariableArray(
         index -> fixedRecordPrototype(encoder), argumentPrototype);
      suppliedArguments.decode(event.parameters.get(
         reportParameters.get("HLAsuppliedArguments")));
      boolean expectSetMembers = expectedSetMemberOne != null || expectedSetMemberTwo != null;
      JavaTckSupport.check((expectedSetMemberOne == null) == (expectedSetMemberTwo == null),
         "object-attribute declaration expected set must contain either zero or two members");
      int expectedCount = expectSetMembers || attributeSet == null || passive == null ? 2 : 4;
      JavaTckSupport.check(suppliedArguments.size() == expectedCount,
         "object-attribute declaration report returned the wrong argument count");
      verifyArgument(suppliedArguments.get(0), 36, "Object class designator",
         quoted(objectHandle));
      if (expectSetMembers) {
         verifyHandleSetArgument(suppliedArguments.get(1), expectedSetMemberOne,
            expectedSetMemberTwo);
      } else if (attributeSet == null) {
         verifyArgument(suppliedArguments.get(1), 34,
            "Optional set of attribute designators", "null");
      } else {
         verifyArgument(suppliedArguments.get(1), 1,
            passive == null ? "Optional set of attribute designators"
               : "Set of attribute designators", attributeSet);
      }
      if (passive != null) {
         verifyArgument(suppliedArguments.get(2), 6,
            "Optional passive subscription indicator", Boolean.toString(passive));
      }
      if (updateRate != null) {
         int index = passive == null ? 2 : 3;
         verifyArgument(suppliedArguments.get(index), 53,
            "Optional update rate designator", quoted(updateRate));
      }
      HLAfixedRecord returnedArgument = encoder.createHLAfixedRecord();
      returnedArgument.add(encoder.createHLAinteger32BE());
      returnedArgument.add(encoder.createHLAunicodeString());
      returnedArgument.add(encoder.createHLAunicodeString());
      returnedArgument.decode(event.parameters.get(reportParameters.get("HLAreturnedArgument")));
      JavaTckSupport.check(((HLAinteger32BE) returnedArgument.get(0)).getValue() == 34
            && ((HLAunicodeString) returnedArgument.get(1)).getValue().isEmpty()
            && "null".equals(((HLAunicodeString) returnedArgument.get(2)).getValue()),
         "object-attribute declaration report returned an invalid Null result argument");
      recorder.events.clear();
   }

   private static void verifyHandleSetArgument(HLAfixedRecord argument, String memberOne,
         String memberTwo) throws Exception {
      JavaTckSupport.check(argument.size() == 3
            && argument.get(0) instanceof HLAinteger32BE
            && argument.get(1) instanceof HLAunicodeString
            && argument.get(2) instanceof HLAunicodeString,
         "object-attribute publication set argument did not contain three typed fields");
      HLAinteger32BE type = (HLAinteger32BE) argument.get(0);
      HLAunicodeString name = (HLAunicodeString) argument.get(1);
      HLAunicodeString value = (HLAunicodeString) argument.get(2);
      String expectedForward = "[" + quoted(memberOne) + "," + quoted(memberTwo) + "]";
      String expectedReverse = "[" + quoted(memberTwo) + "," + quoted(memberOne) + "]";
      JavaTckSupport.check(type.getValue() == 1
            && "Set of attribute designators".equals(name.getValue())
            && (expectedForward.equals(value.getValue())
               || expectedReverse.equals(value.getValue())),
         "object-attribute publication report did not encode both handles in its set array");
   }

   static void runDirectedPublication(RtiFactory factory, String fom, String foreignFom,
         String mim, String objectClassName, String interactionName,
         String foreignObjectClassName, String foreignInteractionName,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      RTIambassador foreign = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-directed-publication-" + UUID.randomUUID();
      String foreignFederation =
         "java-tck-service-report-directed-publication-foreign-" + UUID.randomUUID();
      boolean subjectConnected = false, observerConnected = false, foreignConnected = false;
      boolean created = false, foreignCreated = false;
      boolean subjectJoined = false, observerJoined = false, foreignJoined = false;
      boolean reportSubscribed = false;
      InteractionClassHandle reportClass = null;
      FederateHandle subjectHandle = null;
      try {
         subject.connect(callbacks(), callbackModel);
         subjectConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         foreign.connect(callbacks(), callbackModel);
         foreignConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         foreign.createFederationExecution(foreignFederation, foreignFom, timeImplementation);
         foreignCreated = true;
         subjectHandle = subject.joinFederationExecution(
            "java-tck-directed-publication-subject", "tck-type", federation);
         subjectJoined = true;
         observer.joinFederationExecution(
            "java-tck-directed-publication-observer", "tck-type", federation);
         observerJoined = true;
         foreign.joinFederationExecution(
            "java-tck-directed-publication-foreign", "tck-type", foreignFederation);
         foreignJoined = true;

         ObjectClassHandle objectClass = subject.getObjectClassHandle(objectClassName);
         InteractionClassHandle interactionClass = subject.getInteractionClassHandle(interactionName);
         JavaTckSupport.check(objectClass != null && interactionClass != null
               && objectClassName.equals(subject.getObjectClassName(objectClass))
               && interactionName.equals(subject.getInteractionClassName(interactionClass)),
            "adapter-supplied directed-publication object/interaction lookup failed");
         ObjectClassHandle foreignObjectClass =
            foreign.getObjectClassHandle(foreignObjectClassName);
         InteractionClassHandle foreignInteractionClass =
            foreign.getInteractionClassHandle(foreignInteractionName);
         JavaTckSupport.check(foreignObjectClass != null && foreignInteractionClass != null
               && foreignObjectClassName.equals(foreign.getObjectClassName(foreignObjectClass))
               && foreignInteractionName.equals(
                  foreign.getInteractionClassName(foreignInteractionClass))
               && !objectClassName.equals(foreignObjectClassName)
               && !interactionName.equals(foreignInteractionName),
            "adapter-supplied foreign object/interaction class lookup failed or aliases the primary name");
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

         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);
         JavaTckSupport.expectFailure(
            () -> subject.getObjectClassName(foreignObjectClass), "InvalidObjectClassHandle",
            "lookup of an object-class handle from a different federation execution");
         JavaTckSupport.expectFailure(
            () -> subject.getInteractionClassName(foreignInteractionClass),
            "InvalidInteractionClassHandle",
            "lookup of an interaction-class handle from a different federation execution");
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;
         subject.setServiceReportingSwitch(true);

         InteractionClassHandleSet emptySet =
            subject.getInteractionClassHandleSetFactory().create();
         InteractionClassHandleSet invalidSet =
            subject.getInteractionClassHandleSetFactory().create();
         invalidSet.add(foreignInteractionClass);
         InteractionClassHandleSet validSet =
            subject.getInteractionClassHandleSetFactory().create();
         validSet.add(interactionClass);
         String emptySetText = "[]";
         String invalidSetText = "[\"" + foreignInteractionClass + "\"]";
         String validSetText = "[\"" + interactionClass + "\"]";

         JavaTckSupport.expectFailure(
            () -> subject.publishObjectClassDirectedInteractions(foreignObjectClass, emptySet),
            "ObjectClassNotDefined",
            "directed publish for an object class from a different federation execution");
         verifyDirectedDeclarationReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 0, "PublishObjectClassDirectedInteractions",
            foreignObjectClass.toString(), emptySetText, null, false,
            "ObjectClassNotDefined", callbackModel);
         JavaTckSupport.expectFailure(
            () -> subject.publishObjectClassDirectedInteractions(objectClass, invalidSet),
            "InteractionClassNotDefined",
            "directed publish with an interaction class from a different federation execution");
         verifyDirectedDeclarationReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 1, "PublishObjectClassDirectedInteractions",
            objectClass.toString(), invalidSetText, null, false, "InteractionClassNotDefined",
            callbackModel);
         JavaTckSupport.expectFailure(
            () -> subject.unpublishObjectClassDirectedInteractions(foreignObjectClass, emptySet),
            "ObjectClassNotDefined",
            "directed unpublish for an object class from a different federation execution");
         verifyDirectedDeclarationReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 2, "UnpublishObjectClassDirectedInteractions",
            foreignObjectClass.toString(), emptySetText, null, false,
            "ObjectClassNotDefined", callbackModel);
         JavaTckSupport.expectFailure(
            () -> subject.unpublishObjectClassDirectedInteractions(objectClass, invalidSet),
            "InteractionClassNotDefined",
            "directed unpublish with an interaction class from a different federation execution");
         verifyDirectedDeclarationReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 3, "UnpublishObjectClassDirectedInteractions",
            objectClass.toString(), invalidSetText, null, false, "InteractionClassNotDefined",
            callbackModel);

         subject.publishObjectClassDirectedInteractions(objectClass, validSet);
         verifyDirectedDeclarationReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 4, "PublishObjectClassDirectedInteractions",
            objectClass.toString(), validSetText, null, true, "", callbackModel);
         subject.unpublishObjectClassDirectedInteractions(objectClass, validSet);
         verifyDirectedDeclarationReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 5, "UnpublishObjectClassDirectedInteractions",
            objectClass.toString(), validSetText, null, true, "", callbackModel);
         subject.unpublishObjectClassDirectedInteractions(objectClass);
         verifyDirectedDeclarationReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 6, "UnpublishObjectClassDirectedInteractions",
            objectClass.toString(), null, null, true, "", callbackModel);
      } finally {
         if (subjectJoined) {
            try { subject.setServiceReportingSwitch(false); } catch (Exception ignored) { }
         }
         if (observerJoined && reportSubscribed) {
            try { observer.unsubscribeInteractionClass(reportClass); } catch (Exception ignored) { }
         }
         if (observerJoined) {
            try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (foreignJoined) {
            try { foreign.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (subjectJoined) {
            try { subject.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { subject.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (foreignCreated) {
            try { foreign.destroyFederationExecution(foreignFederation); }
            catch (Exception ignored) { }
         }
         if (foreignConnected) {
            try { foreign.disconnect(); } catch (Exception ignored) { }
         }
         if (observerConnected) {
            try { observer.disconnect(); } catch (Exception ignored) { }
         }
         if (subjectConnected) {
            try { subject.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void verifyDirectedDeclarationReport(RTIambassador observer, Recorder recorder,
         InteractionClassHandle reportClass, Map<String, ParameterHandle> reportParameters,
         TransportationTypeHandle reliable, EncoderFactory encoder, FederateHandle subjectHandle,
         int serialNumber, String serviceName, String objectHandle, String interactionSet,
         Boolean universal, boolean expectedSuccess, String expectedException,
         CallbackModel callbackModel) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (recorder.events.isEmpty() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(observer);
         } else {
            Thread.yield();
         }
      }
      JavaTckSupport.check(recorder.events.size() == 1,
         "directed-interaction declaration did not produce exactly one MOM report");
      Interaction event = recorder.events.get(0);
      JavaTckSupport.check(reportClass.equals(event.interaction)
            && event.parameters.size() == REPORT_PARAMETER_NAMES.length
            && event.producer == null
            && event.tag.length == 0 && reliable.equals(event.transportation)
            && event.regionsEmpty,
         "directed-interaction declaration report had invalid identity or callback metadata");
      for (String name : REPORT_PARAMETER_NAMES) {
         JavaTckSupport.check(event.parameters.containsKey(reportParameters.get(name)),
            "service-report callback omitted standard parameter " + name);
      }
      FederateHandle reportedFederate = observer.getFederateHandleFactory().decode(
         event.parameters.get(reportParameters.get("HLAfederate")), 0);
      JavaTckSupport.check(subjectHandle.equals(reportedFederate),
         "directed-interaction report identified the wrong invoking federate");
      HLAunicodeString service = encoder.createHLAunicodeString();
      service.decode(event.parameters.get(reportParameters.get("HLAservice")));
      JavaTckSupport.check(serviceName.equals(service.getValue()),
         "directed-interaction report returned the wrong service name");
      HLAinteger16BE serviceType = encoder.createHLAinteger16BE();
      serviceType.decode(event.parameters.get(reportParameters.get("HLAserviceType")));
      JavaTckSupport.check(serviceType.getValue() == 1,
         "directed-interaction report returned the wrong service type");
      HLAboolean success = encoder.createHLAboolean();
      success.decode(event.parameters.get(reportParameters.get("HLAsuccessIndicator")));
      JavaTckSupport.check(success.getValue() == expectedSuccess,
         "directed-interaction report returned the wrong success indicator");
      HLAunicodeString exception = encoder.createHLAunicodeString();
      exception.decode(event.parameters.get(reportParameters.get("HLAexception")));
      JavaTckSupport.check(expectedException.isEmpty()
            ? exception.getValue().isEmpty()
            : exception.getValue().contains(expectedException),
         "directed-interaction report returned the wrong exception name");
      HLAinteger32BE serial = encoder.createHLAinteger32BE();
      serial.decode(event.parameters.get(reportParameters.get("HLAserialNumber")));
      JavaTckSupport.check(serial.getValue() == serialNumber,
         "directed-interaction report returned the wrong serial number");
      HLAfixedRecord argumentPrototype = fixedRecordPrototype(encoder);
      HLAvariableArray<HLAfixedRecord> suppliedArguments = encoder.createHLAvariableArray(
         index -> fixedRecordPrototype(encoder), argumentPrototype);
      suppliedArguments.decode(event.parameters.get(
         reportParameters.get("HLAsuppliedArguments")));
      int expectedCount = interactionSet == null ? 2 : (universal == null ? 2 : 3);
      JavaTckSupport.check(suppliedArguments.size() == expectedCount,
         "directed-interaction report returned the wrong supplied-argument count");
      verifyArgument(suppliedArguments.get(0), 36, "Object class designator",
         quoted(objectHandle));
      if (interactionSet != null) {
         String setArgumentName;
         if (serviceName.startsWith("Unpublish")) {
            setArgumentName = "Optional set of interaction class designators";
         } else if (serviceName.startsWith("Unsubscribe")) {
            setArgumentName = "Optional set of directed interaction designators";
         } else if (serviceName.startsWith("Publish")) {
            setArgumentName = "Set of interaction class designators";
         } else {
            setArgumentName = "Set of directed interaction designators";
         }
         verifyArgument(suppliedArguments.get(1), 28,
            setArgumentName, interactionSet);
      } else {
         verifyArgument(suppliedArguments.get(1), 34,
            serviceName.startsWith("Unpublish")
               ? "Optional set of interaction class designators"
               : "Optional set of directed interaction designators",
            "null");
      }
      if (universal != null) {
         verifyArgument(suppliedArguments.get(2), 6,
            "Optional universal subscription indicator", Boolean.toString(universal));
      }
      HLAfixedRecord returnedArgument = encoder.createHLAfixedRecord();
      returnedArgument.add(encoder.createHLAinteger32BE());
      returnedArgument.add(encoder.createHLAunicodeString());
      returnedArgument.add(encoder.createHLAunicodeString());
      returnedArgument.decode(event.parameters.get(reportParameters.get("HLAreturnedArgument")));
      JavaTckSupport.check(((HLAinteger32BE) returnedArgument.get(0)).getValue() == 34
            && ((HLAunicodeString) returnedArgument.get(1)).getValue().isEmpty()
            && "null".equals(((HLAunicodeString) returnedArgument.get(2)).getValue()),
         "directed-interaction report returned an invalid Null result argument");
      recorder.events.clear();
   }

   private static void verifySubscriptionReport(RTIambassador observer, Recorder recorder,
         InteractionClassHandle reportClass, Map<String, ParameterHandle> reportParameters,
         TransportationTypeHandle reliable, EncoderFactory encoder, FederateHandle subjectHandle,
         int serialNumber, String serviceName, String classHandle, boolean passive,
         boolean expectedSuccess, String expectedException, CallbackModel callbackModel)
         throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (recorder.events.isEmpty() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(observer);
         } else {
            Thread.yield();
         }
      }
      JavaTckSupport.check(recorder.events.size() == 1,
         "interaction subscription service did not produce exactly one MOM report");
      Interaction event = recorder.events.get(0);
      JavaTckSupport.check(reportClass.equals(event.interaction)
            && event.parameters.size() == REPORT_PARAMETER_NAMES.length
            && event.producer == null
            && event.tag.length == 0 && reliable.equals(event.transportation)
            && event.regionsEmpty,
         "interaction subscription report had invalid identity or callback metadata");
      for (String name : REPORT_PARAMETER_NAMES) {
         JavaTckSupport.check(event.parameters.containsKey(reportParameters.get(name)),
            "service-report callback omitted standard parameter " + name);
      }
      FederateHandle reportedFederate = observer.getFederateHandleFactory().decode(
         event.parameters.get(reportParameters.get("HLAfederate")), 0);
      JavaTckSupport.check(subjectHandle.equals(reportedFederate),
         "service report identified the wrong invoking federate");
      HLAunicodeString service = encoder.createHLAunicodeString();
      service.decode(event.parameters.get(reportParameters.get("HLAservice")));
      JavaTckSupport.check(serviceName.equals(service.getValue()),
         "interaction subscription report returned the wrong service name");
      HLAinteger16BE serviceType = encoder.createHLAinteger16BE();
      serviceType.decode(event.parameters.get(reportParameters.get("HLAserviceType")));
      JavaTckSupport.check(serviceType.getValue() == 1,
         "interaction subscription report returned the wrong service type");
      HLAboolean success = encoder.createHLAboolean();
      success.decode(event.parameters.get(reportParameters.get("HLAsuccessIndicator")));
      JavaTckSupport.check(success.getValue() == expectedSuccess,
         "interaction subscription report returned the wrong success indicator");
      HLAunicodeString exception = encoder.createHLAunicodeString();
      exception.decode(event.parameters.get(reportParameters.get("HLAexception")));
      JavaTckSupport.check(expectedException.isEmpty()
            ? exception.getValue().isEmpty()
            : exception.getValue().contains(expectedException),
         "interaction subscription report returned the wrong exception name");
      HLAinteger32BE serial = encoder.createHLAinteger32BE();
      serial.decode(event.parameters.get(reportParameters.get("HLAserialNumber")));
      JavaTckSupport.check(serial.getValue() == serialNumber,
         "interaction subscription report returned the wrong serial number");
      HLAfixedRecord argumentPrototype = fixedRecordPrototype(encoder);
      HLAvariableArray<HLAfixedRecord> suppliedArguments = encoder.createHLAvariableArray(
         index -> fixedRecordPrototype(encoder), argumentPrototype);
      suppliedArguments.decode(event.parameters.get(
         reportParameters.get("HLAsuppliedArguments")));
      JavaTckSupport.check(suppliedArguments.size() == (serviceName.startsWith("Subscribe") ? 2 : 1),
         "interaction subscription report returned the wrong argument count");
      verifyArgument(suppliedArguments.get(0), 27, "Interaction class designator",
         quoted(classHandle));
      if (serviceName.startsWith("Subscribe")) {
         verifyArgument(suppliedArguments.get(1), 6,
            "Optional passive subscription indicator", Boolean.toString(passive));
      }
      HLAfixedRecord returnedArgument = encoder.createHLAfixedRecord();
      returnedArgument.add(encoder.createHLAinteger32BE());
      returnedArgument.add(encoder.createHLAunicodeString());
      returnedArgument.add(encoder.createHLAunicodeString());
      returnedArgument.decode(event.parameters.get(reportParameters.get("HLAreturnedArgument")));
      JavaTckSupport.check(((HLAinteger32BE) returnedArgument.get(0)).getValue() == 34
            && ((HLAunicodeString) returnedArgument.get(1)).getValue().isEmpty()
            && "null".equals(((HLAunicodeString) returnedArgument.get(2)).getValue()),
         "interaction subscription report returned an invalid Null result argument");
      recorder.events.clear();
   }

   private static void verifyPublicationReport(RTIambassador observer,
         Recorder recorder, InteractionClassHandle reportClass,
         Map<String, ParameterHandle> reportParameters, TransportationTypeHandle reliable,
         EncoderFactory encoder, FederateHandle subjectHandle, int serialNumber,
         String serviceName, String classHandle,
         boolean expectedSuccess, String expectedException, CallbackModel callbackModel)
         throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (recorder.events.isEmpty() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(observer);
         } else {
            Thread.yield();
         }
      }
      JavaTckSupport.check(recorder.events.size() == 1,
         "interaction publication service did not produce exactly one MOM report");
      Interaction event = recorder.events.get(0);
      JavaTckSupport.check(reportClass.equals(event.interaction)
            && event.parameters.size() == REPORT_PARAMETER_NAMES.length
            && event.producer == null
            && event.tag.length == 0 && reliable.equals(event.transportation)
            && event.regionsEmpty,
         "interaction publication report had invalid identity or callback metadata");
      for (String name : REPORT_PARAMETER_NAMES) {
         JavaTckSupport.check(event.parameters.containsKey(reportParameters.get(name)),
            "service-report callback omitted standard parameter " + name);
      }
      FederateHandle reportedFederate = observer.getFederateHandleFactory().decode(
         event.parameters.get(reportParameters.get("HLAfederate")), 0);
      JavaTckSupport.check(subjectHandle.equals(reportedFederate),
         "service report identified the wrong invoking federate");
      HLAunicodeString service = encoder.createHLAunicodeString();
      service.decode(event.parameters.get(reportParameters.get("HLAservice")));
      JavaTckSupport.check(serviceName.equals(service.getValue()),
         "interaction publication report returned the wrong service name");
      HLAinteger16BE serviceType = encoder.createHLAinteger16BE();
      serviceType.decode(event.parameters.get(reportParameters.get("HLAserviceType")));
      JavaTckSupport.check(serviceType.getValue() == 1,
         "interaction publication report returned the wrong service type");
      HLAboolean success = encoder.createHLAboolean();
      success.decode(event.parameters.get(reportParameters.get("HLAsuccessIndicator")));
      JavaTckSupport.check(success.getValue() == expectedSuccess,
         "interaction publication report returned the wrong success indicator");
      HLAunicodeString exception = encoder.createHLAunicodeString();
      exception.decode(event.parameters.get(reportParameters.get("HLAexception")));
      JavaTckSupport.check(expectedException.isEmpty()
            ? exception.getValue().isEmpty()
            : exception.getValue().contains(expectedException),
         "interaction publication report returned the wrong exception name");
      HLAinteger32BE serial = encoder.createHLAinteger32BE();
      serial.decode(event.parameters.get(reportParameters.get("HLAserialNumber")));
      JavaTckSupport.check(serial.getValue() == serialNumber,
         "interaction publication report returned the wrong serial number");
      HLAfixedRecord argumentPrototype = fixedRecordPrototype(encoder);
      HLAvariableArray<HLAfixedRecord> suppliedArguments = encoder.createHLAvariableArray(
         index -> fixedRecordPrototype(encoder), argumentPrototype);
      suppliedArguments.decode(event.parameters.get(
         reportParameters.get("HLAsuppliedArguments")));
      JavaTckSupport.check(suppliedArguments.size() == 1,
         "interaction publication report returned the wrong argument count");
      verifyArgument(suppliedArguments.get(0), 27, "Interaction class designator",
         quoted(classHandle));
      HLAfixedRecord returnedArgument = encoder.createHLAfixedRecord();
      returnedArgument.add(encoder.createHLAinteger32BE());
      returnedArgument.add(encoder.createHLAunicodeString());
      returnedArgument.add(encoder.createHLAunicodeString());
      returnedArgument.decode(event.parameters.get(reportParameters.get("HLAreturnedArgument")));
      JavaTckSupport.check(((HLAinteger32BE) returnedArgument.get(0)).getValue() == 34
            && ((HLAunicodeString) returnedArgument.get(1)).getValue().isEmpty()
            && "null".equals(((HLAunicodeString) returnedArgument.get(2)).getValue()),
         "interaction publication report returned an invalid Null result argument");
      recorder.events.clear();
   }

   private static void awaitDeliveries(RTIambassador receiver, RTIambassador observer,
         Recorder receiverRecorder, Recorder observerRecorder,
         CallbackModel callbackModel) throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) {
         JavaTckSupport.check(receiverRecorder.events.isEmpty() && observerRecorder.events.isEmpty(),
            "evoked interaction or service-report callback arrived before callback servicing");
      }
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while ((receiverRecorder.events.isEmpty() || observerRecorder.events.isEmpty())
            && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(receiver);
            JavaTckSupport.drain(observer);
         } else {
            Thread.yield();
         }
      }
      JavaTckSupport.check(!receiverRecorder.events.isEmpty()
            && !observerRecorder.events.isEmpty(),
         "timed out waiting for ordinary interaction and HLAreportServiceInvocation callbacks");
   }

   private static void awaitSingleRegulationCallback(RTIambassador regulator,
         Recorder recorder, CallbackModel callbackModel) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (recorder.timeRegulationEnabled == 0 && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(regulator);
         else Thread.yield();
      }
      JavaTckSupport.check(recorder.timeRegulationEnabled == 1,
         "time regulator did not receive exactly one timeRegulationEnabled callback");
   }

   private static void awaitTimeConstrainedCallback(RTIambassador subject, Recorder recorder,
         CallbackModel callbackModel) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (recorder.timeConstrainedEnabled == 0 && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(subject);
         else Thread.yield();
      }
      JavaTckSupport.check(recorder.timeConstrainedEnabled == 1,
         "time-constrained subject did not receive exactly one enablement callback");
   }

   private static void awaitTimedDeliveryAndGrant(RTIambassador publisher, RTIambassador subject,
         Recorder publisherRecorder, Recorder subjectRecorder, CallbackModel callbackModel)
         throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while ((publisherRecorder.timeAdvanceGrantTime == null
            || subjectRecorder.timeAdvanceGrantTime == null
            || subjectRecorder.timedInteractionCount == 0) && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(publisher);
            JavaTckSupport.drain(subject);
         } else {
            Thread.yield();
         }
      }
      JavaTckSupport.check(publisherRecorder.timeAdvanceGrantTime != null
            && subjectRecorder.timeAdvanceGrantTime != null
            && subjectRecorder.timedInteractionCount == 1,
         "queued timestamped interaction or its time-advance grant did not arrive");
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime addTime(LogicalTime time, LogicalTimeInterval interval)
         throws Exception {
      return (LogicalTime) time.add(interval);
   }

   private static void awaitReport(RTIambassador observer, Recorder recorder,
         CallbackModel callbackModel) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (recorder.events.isEmpty() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(observer);
         else Thread.yield();
      }
      JavaTckSupport.check(!recorder.events.isEmpty(),
         "Query GALT HLAreportServiceInvocation callback did not arrive");
   }

   private static Interaction awaitServiceReport(RTIambassador observer, Recorder recorder,
         Map<String, ParameterHandle> reportParameters, EncoderFactory encoder, String serviceName,
         CallbackModel callbackModel) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      Interaction event = findServiceReport(recorder, reportParameters, encoder, serviceName);
      while (event == null && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(observer);
         else Thread.yield();
         event = findServiceReport(recorder, reportParameters, encoder, serviceName);
      }
      JavaTckSupport.check(event != null,
         serviceName + " HLAreportServiceInvocation callback did not arrive");
      return event;
   }

   private static Interaction findServiceReport(Recorder recorder,
         Map<String, ParameterHandle> reportParameters, EncoderFactory encoder, String serviceName)
         throws Exception {
      ParameterHandle serviceParameter = reportParameters.get("HLAservice");
      for (Interaction event : recorder.events) {
         byte[] encodedService = event.parameters.get(serviceParameter);
         if (encodedService == null) continue;
         HLAunicodeString service = encoder.createHLAunicodeString();
         service.decode(encodedService);
         if (serviceName.equals(service.getValue())) return event;
      }
      return null;
   }

   private static void validateServiceReportDirectory(RtiConfiguration subjectConfiguration,
         String additionalSettings, String serviceReportDirectory) {
      JavaTckSupport.check(subjectConfiguration != null,
         "Query GALT service-report configuration is required");
      JavaTckSupport.check(serviceReportDirectory != null && !serviceReportDirectory.isEmpty()
            && Files.isDirectory(Path.of(serviceReportDirectory)),
         "adapter-supplied Query GALT service-report directory must exist");
      String expectedSetting = REPORT_DIRECTORY_SETTING + Path.of(serviceReportDirectory);
      JavaTckSupport.check(additionalSettings != null
            && Arrays.asList(additionalSettings.split(";")).contains(expectedSetting),
         "adapter did not configure the expected Query GALT service-report directory");
   }

   private static Reflection federateMomReflection(Recorder recorder, EncoderFactory encoder,
         AttributeHandle federateNameAttribute, AttributeHandle reportFileAttribute,
         String subjectName) throws Exception {
      ObjectInstanceHandle subjectMomObject = null;
      for (Reflection reflection : recorder.reflections) {
         byte[] encodedName = reflection.values.get(federateNameAttribute);
         if (encodedName != null && subjectName.equals(decodeUnicode(encoder, encodedName))) {
            subjectMomObject = reflection.object;
            break;
         }
      }
      if (subjectMomObject == null) return null;
      Map<AttributeHandle, byte[]> values = new LinkedHashMap<>();
      for (Reflection reflection : recorder.reflections) {
         if (subjectMomObject.equals(reflection.object)) values.putAll(reflection.values);
      }
      if (!values.containsKey(reportFileAttribute)) return null;
      Reflection result = new Reflection();
      result.object = subjectMomObject;
      result.values = values;
      return result;
   }

   private static String decodeUnicode(EncoderFactory encoder, byte[] bytes) throws Exception {
      JavaTckSupport.check(bytes != null,
         "HLAreportServiceFile reflection omitted its value");
      HLAunicodeString value = encoder.createHLAunicodeString();
      value.decode(bytes);
      return value.getValue();
   }

   private static String describeTransportationType(TransportationTypeHandle handle) {
      if (handle == null) return "<null>";
      byte[] encoded = new byte[handle.encodedLength()];
      handle.encode(encoded, 0);
      return handle.getClass().getSimpleName() + " " + Arrays.toString(encoded);
   }

   private static String appendedRecord(String before, String after, String service) {
      String marker = "\"HLAservice\":\"" + service + "\"";
      int servicePosition = after.indexOf(marker, before.length());
      JavaTckSupport.check(servicePosition >= before.length(),
         "HLAreportServiceFile did not append " + service);
      int lineBreak = after.lastIndexOf('\n', servicePosition);
      int recordStart = lineBreak < 0 ? 0 : lineBreak + 1;
      int recordEnd = after.indexOf('\n', servicePosition);
      return after.substring(recordStart, recordEnd < 0 ? after.length() : recordEnd);
   }

   private static HLAfixedRecord fixedRecordPrototype(EncoderFactory encoder) {
      HLAfixedRecord result = encoder.createHLAfixedRecord();
      result.add(encoder.createHLAinteger32BE());
      result.add(encoder.createHLAunicodeString());
      result.add(encoder.createHLAunicodeString());
      return result;
   }

   private static void verifyArgument(HLAfixedRecord argument, int type, String name,
         String value) throws Exception {
      boolean matches = argument.size() == 3
            && ((HLAinteger32BE) argument.get(0)).getValue() == type
            && name.equals(((HLAunicodeString) argument.get(1)).getValue())
            && value.equals(((HLAunicodeString) argument.get(2)).getValue());
      if (!matches) {
         String actualType = argument.size() > 0 && argument.get(0) instanceof HLAinteger32BE
            ? Integer.toString(((HLAinteger32BE) argument.get(0)).getValue()) : "<missing-or-wrong-type>";
         String actualName = argument.size() > 1 && argument.get(1) instanceof HLAunicodeString
            ? ((HLAunicodeString) argument.get(1)).getValue() : "<missing-or-wrong-type>";
         String actualValue = argument.size() > 2 && argument.get(2) instanceof HLAunicodeString
            ? ((HLAunicodeString) argument.get(2)).getValue() : "<missing-or-wrong-type>";
         JavaTckSupport.check(false,
            "service-report supplied argument mismatch: expected type=" + type + ", name=[" + name
               + "], value=[" + value + "]; actual size=" + argument.size() + ", type=" + actualType
               + ", name=[" + actualName + "], value=[" + actualValue + "]");
      }
   }

   private static String quoted(String value) {
      return "\"" + value + "\"";
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> objectMethod(proxy, method.getName(), arguments));
   }

   private static Object objectMethod(Object proxy, String name, Object[] arguments) {
      if ("toString".equals(name)) return "Java TCK service-report callbacks";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
      return null;
   }

   private static final class Interaction {
      private InteractionClassHandle interaction;
      private Map<ParameterHandle, byte[]> parameters;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private boolean regionsEmpty;
      private FederateHandle producer;
   }

   private static final class Reflection {
      private ObjectInstanceHandle object;
      private Map<AttributeHandle, byte[]> values;
   }

   private static final class Recorder {
      private final List<Interaction> events = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private volatile int timeRegulationEnabled;
      private volatile int timeConstrainedEnabled;
      private volatile int timedInteractionCount;
      private volatile LogicalTime<?, ?> interactionTime;
      private volatile LogicalTime<?, ?> timeAdvanceGrantTime;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("reflectAttributeValues".equals(method.getName())
                     && arguments != null && arguments.length >= 2
                     && arguments[0] instanceof ObjectInstanceHandle
                     && arguments[1] instanceof AttributeHandleValueMap) {
                  Reflection event = new Reflection();
                  event.object = (ObjectInstanceHandle) arguments[0];
                  AttributeHandleValueMap source = (AttributeHandleValueMap) arguments[1];
                  event.values = new LinkedHashMap<>();
                  for (AttributeHandle handle : source.keySet()) {
                     event.values.put(handle, source.get(handle).clone());
                  }
                  reflections.add(event);
               } else if ("receiveInteraction".equals(method.getName())
                     && arguments != null && arguments.length == 6) {
                  Interaction event = new Interaction();
                  event.interaction = (InteractionClassHandle) arguments[0];
                  ParameterHandleValueMap source = (ParameterHandleValueMap) arguments[1];
                  event.parameters = new LinkedHashMap<>();
                  for (ParameterHandle handle : source.keySet()) {
                     event.parameters.put(handle, source.get(handle).clone());
                  }
                  event.tag = ((byte[]) arguments[2]).clone();
                  event.transportation = (TransportationTypeHandle) arguments[3];
                  event.producer = (FederateHandle) arguments[4];
                  event.regionsEmpty = arguments[5] == null
                     || ((RegionHandleSet) arguments[5]).isEmpty();
                  events.add(event);
               } else if ("receiveInteraction".equals(method.getName())
                     && arguments != null && arguments.length == 10) {
                  ++timedInteractionCount;
                  interactionTime = (LogicalTime<?, ?>) arguments[6];
               } else if ("timeRegulationEnabled".equals(method.getName())) {
                  ++timeRegulationEnabled;
               } else if ("timeConstrainedEnabled".equals(method.getName())) {
                  ++timeConstrainedEnabled;
               } else if ("timeAdvanceGrant".equals(method.getName())
                     && arguments != null && arguments.length == 1) {
                  timeAdvanceGrantTime = (LogicalTime<?, ?>) arguments[0];
               }
               return objectMethod(proxy, method.getName(), arguments);
            });
      }
   }
}
