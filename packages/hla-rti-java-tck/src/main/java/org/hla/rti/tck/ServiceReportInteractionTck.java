package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.InteractionClassHandleSet;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
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
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verifies an ordinary successful service invocation's standard MOM report. */
final class ServiceReportInteractionTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final String REPORT_CLASS_NAME =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
   private static final String[] REPORT_PARAMETER_NAMES = {
      "HLAservice", "HLAserviceType", "HLAsuccessIndicator", "HLAsuppliedArguments",
      "HLAreturnedArgument", "HLAexception", "HLAserialNumber", "HLAfederate"
   };

   private ServiceReportInteractionTck() { }

   static void runQueryGalt(RtiFactory factory, String fom, String mim,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador regulator = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder regulatorRecorder = new Recorder();
      Recorder observerRecorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-query-galt-" + UUID.randomUUID();
      boolean subjectConnected = false, regulatorConnected = false, observerConnected = false;
      boolean created = false;
      boolean subjectJoined = false, regulatorJoined = false, observerJoined = false;
      boolean reportSubscribed = false;
      InteractionClassHandle reportClass = null;
      FederateHandle subjectHandle = null;
      try {
         subject.connect(callbacks(), callbackModel);
         subjectConnected = true;
         regulator.connect(regulatorRecorder.proxy(), callbackModel);
         regulatorConnected = true;
         observer.connect(observerRecorder.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         subjectHandle = subject.joinFederationExecution(
            "java-tck-query-galt-subject", "tck-type", federation);
         subjectJoined = true;
         regulator.joinFederationExecution("java-tck-query-galt-regulator", "tck-type", federation);
         regulatorJoined = true;
         observer.joinFederationExecution("java-tck-query-galt-observer", "tck-type", federation);
         observerJoined = true;

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
         awaitReport(observer, observerRecorder, callbackModel);

         JavaTckSupport.check(observerRecorder.events.size() == 1,
            "successful Query GALT did not produce exactly one HLAreportServiceInvocation");
         Interaction event = observerRecorder.events.get(0);
         JavaTckSupport.check(reportClass.equals(event.interaction)
               && event.parameters.size() == REPORT_PARAMETER_NAMES.length
               && event.tag.length == 0 && reliable.equals(event.transportation)
               && event.regionsEmpty,
            "Query GALT service report returned invalid interaction or callback metadata");
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
            "successful Query GALT report returned an exception or wrong initial serial");

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
         JavaTckSupport.check(returnedArgument.size() == 3
               && ((HLAinteger32BE) returnedArgument.get(0)).getValue() == 31
               && "GALT".equals(((HLAunicodeString) returnedArgument.get(1)).getValue())
               && quoted(expectedGalt.toString()).equals(
                  ((HLAunicodeString) returnedArgument.get(2)).getValue()),
            "Query GALT report did not preserve its standard logical-time return argument");
      } finally {
         if (observerJoined && reportSubscribed) {
            try { observer.unsubscribeInteractionClass(reportClass); } catch (Exception ignored) { }
         }
         if (subjectJoined) {
            try { subject.setServiceReportingSwitch(false); } catch (Exception ignored) { }
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
         awaitReport(observer, observerRecorder, callbackModel);
         JavaTckSupport.check(observerRecorder.events.size() == 1,
            "successful Next Message Request Available did not produce exactly one MOM report");
         Interaction report = observerRecorder.events.get(0);
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
            "service-report callback did not preserve empty tag, reliable transport, and no regions");

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

   static void runPublication(RtiFactory factory, String fom, String mim,
         String interactionName, String timeImplementation, CallbackModel callbackModel)
         throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-interaction-publication-" + UUID.randomUUID();
      boolean subjectConnected = false, observerConnected = false, created = false;
      boolean subjectJoined = false, observerJoined = false, reportSubscribed = false;
      InteractionClassHandle reportClass = null, sourceClass = null;
      FederateHandle subjectHandle = null;
      try {
         subject.connect(callbacks(), callbackModel);
         subjectConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         subjectHandle = subject.joinFederationExecution(
            "java-tck-interaction-publication-subject", "tck-type", federation);
         subjectJoined = true;
         observer.joinFederationExecution(
            "java-tck-interaction-publication-observer", "tck-type", federation);
         observerJoined = true;

         sourceClass = subject.getInteractionClassHandle(interactionName);
         JavaTckSupport.check(sourceClass != null
               && interactionName.equals(subject.getInteractionClassName(sourceClass)),
            "adapter-supplied interaction class lookup/name round-trip failed");
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
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;
         subject.setServiceReportingSwitch(true);

         String invalidHandle = JavaTckSupport.invalidHandle(
            InteractionClassHandle.class).toString();
         JavaTckSupport.expectFailure(
            () -> subject.publishInteractionClass(
               JavaTckSupport.invalidHandle(InteractionClassHandle.class)),
            "InteractionClassNotDefined", "publishInteractionClass with an invalid handle");
         verifyPublicationReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 0, "PublishInteractionClass", invalidHandle, false,
            "InteractionClassNotDefined", callbackModel);

         JavaTckSupport.expectFailure(
            () -> subject.unpublishInteractionClass(
               JavaTckSupport.invalidHandle(InteractionClassHandle.class)),
            "InteractionClassNotDefined", "unpublishInteractionClass with an invalid handle");
         verifyPublicationReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 1, "UnpublishInteractionClass", invalidHandle, false,
            "InteractionClassNotDefined", callbackModel);

         subject.publishInteractionClass(sourceClass);
         verifyPublicationReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 2, "PublishInteractionClass", sourceClass.toString(), true,
            "", callbackModel);
         subject.unpublishInteractionClass(sourceClass);
         verifyPublicationReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 3, "UnpublishInteractionClass", sourceClass.toString(), true,
            "", callbackModel);
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
         if (subjectConnected) {
            try { subject.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   static void runSubscription(RtiFactory factory, String fom, String mim,
         String interactionName, String timeImplementation, CallbackModel callbackModel)
         throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-interaction-subscription-" + UUID.randomUUID();
      boolean subjectConnected = false, observerConnected = false, created = false;
      boolean subjectJoined = false, observerJoined = false, reportSubscribed = false;
      InteractionClassHandle reportClass = null, sourceClass = null;
      FederateHandle subjectHandle = null;
      try {
         subject.connect(callbacks(), callbackModel);
         subjectConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         subjectHandle = subject.joinFederationExecution(
            "java-tck-interaction-subscription-subject", "tck-type", federation);
         subjectJoined = true;
         observer.joinFederationExecution(
            "java-tck-interaction-subscription-observer", "tck-type", federation);
         observerJoined = true;

         sourceClass = subject.getInteractionClassHandle(interactionName);
         JavaTckSupport.check(sourceClass != null
               && interactionName.equals(subject.getInteractionClassName(sourceClass)),
            "adapter-supplied interaction class lookup/name round-trip failed");
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
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;
         subject.setServiceReportingSwitch(true);

         String invalidHandle = JavaTckSupport.invalidHandle(
            InteractionClassHandle.class).toString();
         JavaTckSupport.expectFailure(
            () -> subject.subscribeInteractionClassPassively(
               JavaTckSupport.invalidHandle(InteractionClassHandle.class)),
            "InteractionClassNotDefined", "passive subscribeInteractionClass with invalid handle");
         verifySubscriptionReport(observer, recorder, reportClass, reportParameters, reliable,
            encoder, subjectHandle, 0, "SubscribeInteractionClass", invalidHandle, true, false,
            "InteractionClassNotDefined", callbackModel);

         JavaTckSupport.expectFailure(
            () -> subject.unsubscribeInteractionClass(
               JavaTckSupport.invalidHandle(InteractionClassHandle.class)),
            "InteractionClassNotDefined", "unsubscribeInteractionClass with an invalid handle");
         verifySubscriptionReport(observer, recorder, reportClass, reportParameters, reliable,
            encoder, subjectHandle, 1, "UnsubscribeInteractionClass", invalidHandle, false, false,
            "InteractionClassNotDefined", callbackModel);

         subject.subscribeInteractionClass(sourceClass);
         verifySubscriptionReport(observer, recorder, reportClass, reportParameters, reliable,
            encoder, subjectHandle, 2, "SubscribeInteractionClass", sourceClass.toString(), false,
            true, "", callbackModel);
         subject.unsubscribeInteractionClass(sourceClass);
         verifySubscriptionReport(observer, recorder, reportClass, reportParameters, reliable,
            encoder, subjectHandle, 3, "UnsubscribeInteractionClass", sourceClass.toString(),
            false, false, "", callbackModel);
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
         if (subjectConnected) {
            try { subject.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   static void runDirectedDeclaration(RtiFactory factory, String fom, String mim,
         String objectClassName, String interactionName, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-directed-declaration-" + UUID.randomUUID();
      boolean subjectConnected = false, observerConnected = false, created = false;
      boolean subjectJoined = false, observerJoined = false, reportSubscribed = false;
      InteractionClassHandle reportClass = null;
      FederateHandle subjectHandle = null;
      try {
         subject.connect(callbacks(), callbackModel);
         subjectConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         subjectHandle = subject.joinFederationExecution(
            "java-tck-directed-declaration-subject", "tck-type", federation);
         subjectJoined = true;
         observer.joinFederationExecution(
            "java-tck-directed-declaration-observer", "tck-type", federation);
         observerJoined = true;

         ObjectClassHandle objectClass = subject.getObjectClassHandle(objectClassName);
         InteractionClassHandle interactionClass = subject.getInteractionClassHandle(interactionName);
         JavaTckSupport.check(objectClass != null && interactionClass != null
               && objectClassName.equals(subject.getObjectClassName(objectClass))
               && interactionName.equals(subject.getInteractionClassName(interactionClass)),
            "adapter-supplied directed-declaration object/interaction lookup failed");
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
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;
         subject.setServiceReportingSwitch(true);

         ObjectClassHandle invalidObject = JavaTckSupport.invalidHandle(ObjectClassHandle.class);
         InteractionClassHandle invalidInteraction =
            JavaTckSupport.invalidHandle(InteractionClassHandle.class);
         InteractionClassHandleSet emptySet =
            subject.getInteractionClassHandleSetFactory().create();
         InteractionClassHandleSet invalidSet =
            subject.getInteractionClassHandleSetFactory().create();
         invalidSet.add(invalidInteraction);
         InteractionClassHandleSet validSet =
            subject.getInteractionClassHandleSetFactory().create();
         validSet.add(interactionClass);
         String emptySetText = "[]";
         String invalidSetText = "[\"" + invalidInteraction + "\"]";
         String validSetText = "[\"" + interactionClass + "\"]";

         JavaTckSupport.expectFailure(
            () -> subject.subscribeObjectClassDirectedInteractions(
               invalidObject, emptySet),
            "ObjectClassNotDefined", "directed subscribe for an invalid object class");
         verifyDirectedDeclarationReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 0, "SubscribeObjectClassDirectedInteractions",
            invalidObject.toString(), emptySetText, false, false, "ObjectClassNotDefined",
            callbackModel);

         JavaTckSupport.expectFailure(
            () -> subject.subscribeObjectClassDirectedInteractionsUniversally(
               objectClass, invalidSet),
            "InteractionClassNotDefined", "universal directed subscribe with invalid interaction");
         verifyDirectedDeclarationReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 1, "SubscribeObjectClassDirectedInteractions",
            objectClass.toString(), invalidSetText, true, false, "InteractionClassNotDefined",
            callbackModel);

         JavaTckSupport.expectFailure(
            () -> subject.unsubscribeObjectClassDirectedInteractions(invalidObject, emptySet),
            "ObjectClassNotDefined", "directed unsubscribe for an invalid object class");
         verifyDirectedDeclarationReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 2, "UnsubscribeObjectClassDirectedInteractions",
            invalidObject.toString(), emptySetText, null, false, "ObjectClassNotDefined",
            callbackModel);

         JavaTckSupport.expectFailure(
            () -> subject.unsubscribeObjectClassDirectedInteractions(objectClass, invalidSet),
            "InteractionClassNotDefined", "directed unsubscribe with invalid interaction");
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
         if (subjectConnected) {
            try { subject.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   static void runObjectAttributeDeclaration(RtiFactory factory, String fom, String mim,
         String objectClassName, String attributeName, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      String updateRateName = JavaTckSupport.updateRateName();
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-object-attribute-declaration-"
         + UUID.randomUUID();
      boolean subjectConnected = false, observerConnected = false, created = false;
      boolean subjectJoined = false, observerJoined = false, reportSubscribed = false;
      InteractionClassHandle reportClass = null;
      FederateHandle subjectHandle = null;
      try {
         subject.connect(callbacks(), callbackModel);
         subjectConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         subjectHandle = subject.joinFederationExecution(
            "java-tck-object-attribute-declaration-subject", "tck-type", federation);
         subjectJoined = true;
         observer.joinFederationExecution(
            "java-tck-object-attribute-declaration-observer", "tck-type", federation);
         observerJoined = true;

         ObjectClassHandle objectClass = subject.getObjectClassHandle(objectClassName);
         AttributeHandle attribute = subject.getAttributeHandle(objectClass, attributeName);
         JavaTckSupport.check(objectClass != null && attribute != null
               && objectClassName.equals(subject.getObjectClassName(objectClass))
               && attributeName.equals(subject.getAttributeName(objectClass, attribute)),
            "adapter-supplied object class/attribute lookup failed");
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
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;
         subject.setServiceReportingSwitch(true);

         ObjectClassHandle invalidObject = JavaTckSupport.invalidHandle(ObjectClassHandle.class);
         AttributeHandle invalidAttribute = JavaTckSupport.invalidHandle(AttributeHandle.class);
         AttributeHandleSet emptySet = subject.getAttributeHandleSetFactory().create();
         AttributeHandleSet invalidSet = subject.getAttributeHandleSetFactory().create();
         invalidSet.add(invalidAttribute);
         AttributeHandleSet validSet = subject.getAttributeHandleSetFactory().create();
         validSet.add(attribute);
         String invalidObjectText = invalidObject.toString();
         String objectText = objectClass.toString();
         String invalidSetText = "[\"" + invalidAttribute + "\"]";
         String validSetText = "[\"" + attribute + "\"]";

         JavaTckSupport.expectFailure(
            () -> subject.subscribeObjectClassAttributesPassively(
               invalidObject, emptySet, updateRateName),
            "ObjectClassNotDefined", "passive attribute subscribe for invalid object class");
         verifyObjectAttributeReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 0, "SubscribeObjectClassAttributes",
            invalidObjectText, "[]", true, updateRateName, false, "ObjectClassNotDefined",
            callbackModel);
         JavaTckSupport.expectFailure(
            () -> subject.subscribeObjectClassAttributesPassively(
               objectClass, invalidSet, updateRateName),
            "AttributeNotDefined", "passive attribute subscribe with invalid attribute");
         verifyObjectAttributeReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 1, "SubscribeObjectClassAttributes",
            objectText, invalidSetText, true, updateRateName, false, "AttributeNotDefined",
            callbackModel);
         JavaTckSupport.expectFailure(
            () -> subject.unsubscribeObjectClassAttributes(invalidObject, emptySet),
            "ObjectClassNotDefined", "attribute unsubscribe for invalid object class");
         verifyObjectAttributeReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 2, "UnsubscribeObjectClassAttributes",
            invalidObjectText, "[]", null, null, false, "ObjectClassNotDefined", callbackModel);
         JavaTckSupport.expectFailure(
            () -> subject.unsubscribeObjectClassAttributes(objectClass, invalidSet),
            "AttributeNotDefined", "attribute unsubscribe with invalid attribute");
         verifyObjectAttributeReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 3, "UnsubscribeObjectClassAttributes",
            objectText, invalidSetText, null, null, false, "AttributeNotDefined", callbackModel);
         JavaTckSupport.expectFailure(
            () -> subject.unsubscribeObjectClass(invalidObject),
            "ObjectClassNotDefined", "whole object-class unsubscribe with invalid class");
         verifyObjectAttributeReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 4, "UnsubscribeObjectClassAttributes",
            invalidObjectText, null, null, null, false, "ObjectClassNotDefined", callbackModel);

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
            && event.tag.length == 0 && reliable.equals(event.transportation)
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
      int expectedCount = attributeSet == null ? 2 : (passive == null ? 2 : 4);
      JavaTckSupport.check(suppliedArguments.size() == expectedCount,
         "object-attribute declaration report returned the wrong argument count");
      verifyArgument(suppliedArguments.get(0), 36, "Object class designator",
         quoted(objectHandle));
      if (attributeSet == null) {
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

   static void runDirectedPublication(RtiFactory factory, String fom, String mim,
         String objectClassName, String interactionName, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-directed-publication-" + UUID.randomUUID();
      boolean subjectConnected = false, observerConnected = false, created = false;
      boolean subjectJoined = false, observerJoined = false, reportSubscribed = false;
      InteractionClassHandle reportClass = null;
      FederateHandle subjectHandle = null;
      try {
         subject.connect(callbacks(), callbackModel);
         subjectConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         subjectHandle = subject.joinFederationExecution(
            "java-tck-directed-publication-subject", "tck-type", federation);
         subjectJoined = true;
         observer.joinFederationExecution(
            "java-tck-directed-publication-observer", "tck-type", federation);
         observerJoined = true;

         ObjectClassHandle objectClass = subject.getObjectClassHandle(objectClassName);
         InteractionClassHandle interactionClass = subject.getInteractionClassHandle(interactionName);
         JavaTckSupport.check(objectClass != null && interactionClass != null
               && objectClassName.equals(subject.getObjectClassName(objectClass))
               && interactionName.equals(subject.getInteractionClassName(interactionClass)),
            "adapter-supplied directed-publication object/interaction lookup failed");
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
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;
         subject.setServiceReportingSwitch(true);

         ObjectClassHandle invalidObject = JavaTckSupport.invalidHandle(ObjectClassHandle.class);
         InteractionClassHandle invalidInteraction =
            JavaTckSupport.invalidHandle(InteractionClassHandle.class);
         InteractionClassHandleSet emptySet =
            subject.getInteractionClassHandleSetFactory().create();
         InteractionClassHandleSet invalidSet =
            subject.getInteractionClassHandleSetFactory().create();
         invalidSet.add(invalidInteraction);
         InteractionClassHandleSet validSet =
            subject.getInteractionClassHandleSetFactory().create();
         validSet.add(interactionClass);
         String emptySetText = "[]";
         String invalidSetText = "[\"" + invalidInteraction + "\"]";
         String validSetText = "[\"" + interactionClass + "\"]";

         JavaTckSupport.expectFailure(
            () -> subject.publishObjectClassDirectedInteractions(invalidObject, emptySet),
            "ObjectClassNotDefined", "directed publish for an invalid object class");
         verifyDirectedDeclarationReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 0, "PublishObjectClassDirectedInteractions",
            invalidObject.toString(), emptySetText, null, false, "ObjectClassNotDefined",
            callbackModel);
         JavaTckSupport.expectFailure(
            () -> subject.publishObjectClassDirectedInteractions(objectClass, invalidSet),
            "InteractionClassNotDefined", "directed publish with an invalid interaction");
         verifyDirectedDeclarationReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 1, "PublishObjectClassDirectedInteractions",
            objectClass.toString(), invalidSetText, null, false, "InteractionClassNotDefined",
            callbackModel);
         JavaTckSupport.expectFailure(
            () -> subject.unpublishObjectClassDirectedInteractions(invalidObject, emptySet),
            "ObjectClassNotDefined", "directed unpublish for an invalid object class");
         verifyDirectedDeclarationReport(observer, recorder, reportClass, reportParameters,
            reliable, encoder, subjectHandle, 2, "UnpublishObjectClassDirectedInteractions",
            invalidObject.toString(), emptySetText, null, false, "ObjectClassNotDefined",
            callbackModel);
         JavaTckSupport.expectFailure(
            () -> subject.unpublishObjectClassDirectedInteractions(objectClass, invalidSet),
            "InteractionClassNotDefined", "directed unpublish with an invalid interaction");
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

   private static HLAfixedRecord fixedRecordPrototype(EncoderFactory encoder) {
      HLAfixedRecord result = encoder.createHLAfixedRecord();
      result.add(encoder.createHLAinteger32BE());
      result.add(encoder.createHLAunicodeString());
      result.add(encoder.createHLAunicodeString());
      return result;
   }

   private static void verifyArgument(HLAfixedRecord argument, int type, String name,
         String value) throws Exception {
      JavaTckSupport.check(argument.size() == 3
            && ((HLAinteger32BE) argument.get(0)).getValue() == type
            && name.equals(((HLAunicodeString) argument.get(1)).getValue())
            && value.equals(((HLAunicodeString) argument.get(2)).getValue()),
         "service-report supplied argument did not match standard type/name/value: " + name);
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

   private static final class Recorder {
      private final List<Interaction> events = new CopyOnWriteArrayList<>();
      private volatile int timeRegulationEnabled;
      private volatile int timeConstrainedEnabled;
      private volatile int timedInteractionCount;
      private volatile LogicalTime<?, ?> interactionTime;
      private volatile LogicalTime<?, ?> timeAdvanceGrantTime;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("receiveInteraction".equals(method.getName())
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
