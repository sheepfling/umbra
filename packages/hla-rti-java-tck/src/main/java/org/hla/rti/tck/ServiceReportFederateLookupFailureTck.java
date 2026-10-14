package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
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
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verifies federate and object-class lookup failure reports and recovery through MOM. */
final class ServiceReportFederateLookupFailureTck {
   private static final String REPORT_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
   private static final String[] REPORT_PARAMETERS = {
      "HLAservice", "HLAserviceType", "HLAsuccessIndicator", "HLAsuppliedArguments",
      "HLAreturnedArgument", "HLAexception", "HLAserialNumber", "HLAfederate"
   };
   private static final long TIMEOUT_NANOS = 12_000_000_000L;

   private ServiceReportFederateLookupFailureTck() { }

   static void run(RtiFactory factory, String fom, String mim, String objectClassName,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-service-report-federate-lookup-failure-" + suffix;
      String subjectName = "java-tck-federate-lookup-failure-subject-" + suffix;
      String observerName = "java-tck-federate-lookup-failure-observer-" + suffix;
      String missingFederateName = uniqueName("TckMissingFederate");
      String missingObjectClassName = uniqueName("HLAobjectRoot.TckMissingObjectClass");
      boolean subjectConnected = false, observerConnected = false, created = false;
      boolean subjectJoined = false, observerJoined = false, reportSubscribed = false;
      InteractionClassHandle reportClass = null;
      try {
         subject.connect(emptyCallbacks(), callbackModel);
         subjectConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle subjectFederate = subject.joinFederationExecution(
            subjectName, "java-tck", federation);
         subjectJoined = true;
         observer.joinFederationExecution(observerName, "java-tck", federation);
         observerJoined = true;

         reportClass = observer.getInteractionClassHandle(REPORT_CLASS);
         JavaTckSupport.check(reportClass != null
               && REPORT_CLASS.equals(observer.getInteractionClassName(reportClass)),
            "standard HLAreportServiceInvocation lookup/name round-trip failed");
         Map<String, ParameterHandle> reportParameters = new LinkedHashMap<>();
         for (String name : REPORT_PARAMETERS) {
            ParameterHandle parameter = observer.getParameterHandle(reportClass, name);
            JavaTckSupport.check(parameter != null
                  && name.equals(observer.getParameterName(reportClass, parameter)),
               "service-report parameter lookup/name round-trip failed for " + name);
            reportParameters.put(name, parameter);
         }
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(reliable != null
               && "HLAreliable".equals(observer.getTransportationTypeName(reliable)),
            "standard reliable transportation lookup did not round-trip");
         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;

         FederateHandle invalidHandle = JavaTckSupport.invalidHandle(FederateHandle.class);
         subject.setServiceReportingSwitch(true);
         JavaTckSupport.expectFailure(() -> subject.getFederateHandle(missingFederateName),
            "NameNotFound", "reported missing federate-name lookup");
         JavaTckSupport.expectFailure(() -> subject.getFederateName(invalidHandle),
            "InvalidFederateHandle", "reported invalid federate-handle lookup");
         ObjectClassHandle invalidObjectClass =
            JavaTckSupport.invalidHandle(ObjectClassHandle.class);
         JavaTckSupport.expectFailure(() -> subject.getObjectClassHandle(missingObjectClassName),
            "NameNotFound", "reported missing object-class lookup");
         JavaTckSupport.expectFailure(() -> subject.getObjectClassName(invalidObjectClass),
            "InvalidObjectClassHandle", "reported invalid object-class-handle lookup");
         FederateHandle federateByName = subject.getFederateHandle(subjectName);
         String federateName = subject.getFederateName(subjectFederate);
         ObjectClassHandle objectClass = subject.getObjectClassHandle(objectClassName);
         String resolvedObjectClassName = subject.getObjectClassName(objectClass);
         JavaTckSupport.check(subjectFederate.equals(federateByName)
               && subjectName.equals(federateName) && objectClass != null
               && objectClassName.equals(resolvedObjectClassName),
            "federate/object-class lookups did not recover after reported failures");
         await(observer, recorder, callbackModel);
         JavaTckSupport.check(recorder.events.size() == 8,
            "federate lookups produced an unexpected service-report count");

         verifyFailureReport(observer, subjectFederate, recorder.events.get(0), encoder,
            reportClass, reportParameters, reliable, "GetFederateHandle",
            new Argument[] {new Argument(53, "Federate name", quoted(missingFederateName))},
            "NameNotFound", 0);
         verifyFailureReport(observer, subjectFederate, recorder.events.get(1), encoder,
            reportClass, reportParameters, reliable, "GetFederateName",
            new Argument[] {new Argument(15, "Federate handle", quoted(invalidHandle.toString()))},
            "InvalidFederateHandle", 1);
         verifyFailureReport(observer, subjectFederate, recorder.events.get(2), encoder,
            reportClass, reportParameters, reliable, "GetObjectClassHandle",
            new Argument[] {new Argument(53, "Object class name", quoted(missingObjectClassName))},
            "NameNotFound", 2);
         verifyFailureReport(observer, subjectFederate, recorder.events.get(3), encoder,
            reportClass, reportParameters, reliable, "GetObjectClassName",
            new Argument[] {new Argument(36, "Object class handle",
               quoted(invalidObjectClass.toString()))},
            "InvalidObjectClassHandle", 3);
         verifySuccessReport(observer, subjectFederate, recorder.events.get(4), encoder,
            reportClass, reportParameters, reliable, "GetFederateHandle",
            new Argument[] {new Argument(53, "Federate name", quoted(subjectName))},
            new Argument(15, "Federate handle", quoted(federateByName.toString())), 4);
         verifySuccessReport(observer, subjectFederate, recorder.events.get(5), encoder,
            reportClass, reportParameters, reliable, "GetFederateName",
            new Argument[] {new Argument(15, "Federate handle",
               quoted(subjectFederate.toString()))},
            new Argument(53, "Federate name", quoted(federateName)), 5);
         verifySuccessReport(observer, subjectFederate, recorder.events.get(6), encoder,
            reportClass, reportParameters, reliable, "GetObjectClassHandle",
            new Argument[] {new Argument(53, "Object class name", quoted(objectClassName))},
            new Argument(36, "Object class handle", quoted(objectClass.toString())), 6);
         verifySuccessReport(observer, subjectFederate, recorder.events.get(7), encoder,
            reportClass, reportParameters, reliable, "GetObjectClassName",
            new Argument[] {new Argument(36, "Object class handle", quoted(objectClass.toString()))},
            new Argument(53, "Object class name", quoted(resolvedObjectClassName)), 7);

         observer.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         subject.setServiceReportingSwitch(false);
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         observerJoined = false;
         subject.resignFederationExecution(ResignAction.NO_ACTION);
         subjectJoined = false;
         subject.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (reportSubscribed && observerJoined && reportClass != null) {
            try { observer.unsubscribeInteractionClass(reportClass); }
            catch (Exception ignored) { }
         }
         try { subject.setServiceReportingSwitch(false); } catch (Exception ignored) { }
         if (observerJoined) resign(observer);
         if (subjectJoined) resign(subject);
         if (created) {
            try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (observerConnected) disconnect(observer);
         if (subjectConnected) disconnect(subject);
      }
   }

   static void runInteractionParameter(RtiFactory factory, String fom, String mim,
         String interactionName, String parameterName, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-service-report-interaction-lookup-failure-" + suffix;
      String subjectName = "java-tck-interaction-lookup-failure-subject-" + suffix;
      String observerName = "java-tck-interaction-lookup-failure-observer-" + suffix;
      String missingInteractionName = uniqueName("HLAinteractionRoot.TckMissingInteraction");
      String missingParameterName = uniqueName("TckMissingParameter");
      boolean subjectConnected = false, observerConnected = false, created = false;
      boolean subjectJoined = false, observerJoined = false, reportSubscribed = false;
      InteractionClassHandle reportClass = null;
      try {
         subject.connect(emptyCallbacks(), callbackModel);
         subjectConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle subjectFederate = subject.joinFederationExecution(
            subjectName, "java-tck", federation);
         subjectJoined = true;
         observer.joinFederationExecution(observerName, "java-tck", federation);
         observerJoined = true;

         reportClass = observer.getInteractionClassHandle(REPORT_CLASS);
         JavaTckSupport.check(reportClass != null
               && REPORT_CLASS.equals(observer.getInteractionClassName(reportClass)),
            "standard HLAreportServiceInvocation lookup/name round-trip failed");
         Map<String, ParameterHandle> reportParameters = new LinkedHashMap<>();
         for (String name : REPORT_PARAMETERS) {
            ParameterHandle parameter = observer.getParameterHandle(reportClass, name);
            JavaTckSupport.check(parameter != null
                  && name.equals(observer.getParameterName(reportClass, parameter)),
               "service-report parameter lookup/name round-trip failed for " + name);
            reportParameters.put(name, parameter);
         }
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(reliable != null
               && "HLAreliable".equals(observer.getTransportationTypeName(reliable)),
            "standard reliable transportation lookup did not round-trip");
         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;

         InteractionClassHandle interaction = subject.getInteractionClassHandle(interactionName);
         ParameterHandle parameter = subject.getParameterHandle(interaction, parameterName);
         JavaTckSupport.check(interaction != null && parameter != null,
            "adapter-supplied interaction/parameter setup lookup failed");
         subject.setServiceReportingSwitch(true);
         JavaTckSupport.expectFailure(() -> subject.getInteractionClassHandle(
            missingInteractionName), "NameNotFound", "reported missing interaction lookup");
         InteractionClassHandle invalidInteraction =
            JavaTckSupport.invalidHandle(InteractionClassHandle.class);
         JavaTckSupport.expectFailure(() -> subject.getInteractionClassName(invalidInteraction),
            "InvalidInteractionClassHandle", "reported invalid interaction handle lookup");
         JavaTckSupport.expectFailure(() -> subject.getParameterHandle(interaction,
            missingParameterName), "NameNotFound", "reported missing parameter lookup");
         ParameterHandle invalidParameter =
            JavaTckSupport.invalidHandle(ParameterHandle.class);
         JavaTckSupport.expectFailure(() -> subject.getParameterName(interaction, invalidParameter),
            "InvalidParameterHandle", "reported invalid parameter handle lookup");
         InteractionClassHandle interactionByName =
            subject.getInteractionClassHandle(interactionName);
         JavaTckSupport.check(interaction.equals(interactionByName),
            "interaction-class lookup did not recover after reported failures");
         long deadline = System.nanoTime() + TIMEOUT_NANOS;
         while (recorder.events.size() < 5 && System.nanoTime() < deadline) {
            if (callbackModel == CallbackModel.HLA_EVOKED) observer.evokeCallback(0.01);
            else Thread.yield();
         }
         JavaTckSupport.check(recorder.events.size() == 5,
            "interaction/parameter lookups produced an unexpected service-report count");

         verifyFailureReport(observer, subjectFederate, recorder.events.get(0), encoder,
            reportClass, reportParameters, reliable, "GetInteractionClassHandle",
            new Argument[] {new Argument(53, "Interaction class name",
               quoted(missingInteractionName))}, "NameNotFound", 0);
         verifyFailureReport(observer, subjectFederate, recorder.events.get(1), encoder,
            reportClass, reportParameters, reliable, "GetInteractionClassName",
            new Argument[] {new Argument(27, "Interaction class handle",
               quoted(invalidInteraction.toString()))}, "InvalidInteractionClassHandle", 1);
         verifyFailureReport(observer, subjectFederate, recorder.events.get(2), encoder,
            reportClass, reportParameters, reliable, "GetParameterHandle",
            new Argument[] {
               new Argument(27, "Interaction class handle", quoted(interaction.toString())),
               new Argument(53, "Parameter name", quoted(missingParameterName))
            }, "NameNotFound", 2);
         verifyFailureReport(observer, subjectFederate, recorder.events.get(3), encoder,
            reportClass, reportParameters, reliable, "GetParameterName",
            new Argument[] {
               new Argument(27, "Interaction class handle", quoted(interaction.toString())),
               new Argument(39, "Parameter handle", quoted(invalidParameter.toString()))
            }, "InvalidParameterHandle", 3);
         verifySuccessReport(observer, subjectFederate, recorder.events.get(4), encoder,
            reportClass, reportParameters, reliable, "GetInteractionClassHandle",
            new Argument[] {new Argument(53, "Interaction class name", quoted(interactionName))},
            new Argument(27, "Interaction class handle", quoted(interactionByName.toString())), 4);

         observer.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         subject.setServiceReportingSwitch(false);
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         observerJoined = false;
         subject.resignFederationExecution(ResignAction.NO_ACTION);
         subjectJoined = false;
         subject.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (reportSubscribed && observerJoined && reportClass != null) {
            try { observer.unsubscribeInteractionClass(reportClass); }
            catch (Exception ignored) { }
         }
         try { subject.setServiceReportingSwitch(false); } catch (Exception ignored) { }
         if (observerJoined) resign(observer);
         if (subjectJoined) resign(subject);
         if (created) {
            try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (observerConnected) disconnect(observer);
         if (subjectConnected) disconnect(subject);
      }
   }

   static void runObjectAttributeUpdateRate(RtiFactory factory, String fom, String mim,
         String objectClassName, String attributeName, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-service-report-object-lookup-failure-" + suffix;
      String subjectName = "java-tck-object-lookup-failure-subject-" + suffix;
      String observerName = "java-tck-object-lookup-failure-observer-" + suffix;
      String missingObjectName = uniqueName("TckMissingObjectInstance");
      String missingRateName = uniqueName("TckMissingUpdateRate");
      boolean subjectConnected = false, observerConnected = false, created = false;
      boolean subjectJoined = false, observerJoined = false, reportSubscribed = false;
      InteractionClassHandle reportClass = null;
      try {
         subject.connect(emptyCallbacks(), callbackModel);
         subjectConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle subjectFederate = subject.joinFederationExecution(
            subjectName, "java-tck", federation);
         subjectJoined = true;
         observer.joinFederationExecution(observerName, "java-tck", federation);
         observerJoined = true;

         reportClass = observer.getInteractionClassHandle(REPORT_CLASS);
         JavaTckSupport.check(reportClass != null
               && REPORT_CLASS.equals(observer.getInteractionClassName(reportClass)),
            "standard HLAreportServiceInvocation lookup/name round-trip failed");
         Map<String, ParameterHandle> reportParameters = new LinkedHashMap<>();
         for (String name : REPORT_PARAMETERS) {
            ParameterHandle parameter = observer.getParameterHandle(reportClass, name);
            JavaTckSupport.check(parameter != null
                  && name.equals(observer.getParameterName(reportClass, parameter)),
               "service-report parameter lookup/name round-trip failed for " + name);
            reportParameters.put(name, parameter);
         }
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(reliable != null
               && "HLAreliable".equals(observer.getTransportationTypeName(reliable)),
            "standard reliable transportation lookup did not round-trip");
         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;

         ObjectClassHandle objectClass = subject.getObjectClassHandle(objectClassName);
         AttributeHandle attribute = subject.getAttributeHandle(objectClass, attributeName);
         JavaTckSupport.check(objectClass != null && attribute != null,
            "adapter-supplied object class/attribute setup lookup failed");
         ObjectInstanceHandle invalidObject =
            JavaTckSupport.invalidHandle(ObjectInstanceHandle.class);
         ObjectClassHandle invalidObjectClass =
            JavaTckSupport.invalidHandle(ObjectClassHandle.class);
         AttributeHandle invalidAttribute = JavaTckSupport.invalidHandle(AttributeHandle.class);
         subject.setServiceReportingSwitch(true);
         JavaTckSupport.expectFailure(() -> subject.getKnownObjectClassHandle(invalidObject),
            "ObjectInstanceNotKnown", "reported unknown object-class lookup");
         JavaTckSupport.expectFailure(() -> subject.getObjectInstanceHandle(missingObjectName),
            "ObjectInstanceNotKnown", "reported missing object-instance lookup");
         JavaTckSupport.expectFailure(() -> subject.getObjectInstanceName(invalidObject),
            "ObjectInstanceNotKnown", "reported invalid object-name lookup");
         JavaTckSupport.expectFailure(() -> subject.getAttributeHandle(invalidObjectClass,
            attributeName), "InvalidObjectClassHandle", "reported invalid class attribute lookup");
         JavaTckSupport.expectFailure(() -> subject.getAttributeName(objectClass, invalidAttribute),
            "InvalidAttributeHandle", "reported invalid attribute-name lookup");
         JavaTckSupport.expectFailure(() -> subject.getUpdateRateValue(missingRateName),
            "InvalidUpdateRateDesignator", "reported missing update-rate lookup");
         JavaTckSupport.expectFailure(() -> subject.getUpdateRateValueForAttribute(
            invalidObject, attribute), "ObjectInstanceNotKnown",
            "reported invalid object update-rate lookup");
         ObjectClassHandle objectClassByName = subject.getObjectClassHandle(objectClassName);
         JavaTckSupport.check(objectClass.equals(objectClassByName),
            "object-class lookup did not recover after reported failures");
         long deadline = System.nanoTime() + TIMEOUT_NANOS;
         while (recorder.events.size() < 8 && System.nanoTime() < deadline) {
            if (callbackModel == CallbackModel.HLA_EVOKED) observer.evokeCallback(0.01);
            else Thread.yield();
         }
         JavaTckSupport.check(recorder.events.size() == 8,
            "object/attribute/update-rate lookups produced an unexpected service-report count");

         verifyFailureReport(observer, subjectFederate, recorder.events.get(0), encoder,
            reportClass, reportParameters, reliable, "GetKnownObjectClassHandle",
            new Argument[] {new Argument(37, "Object instance handle",
               quoted(invalidObject.toString()))}, "ObjectInstanceNotKnown", 0);
         verifyFailureReport(observer, subjectFederate, recorder.events.get(1), encoder,
            reportClass, reportParameters, reliable, "GetObjectInstanceHandle",
            new Argument[] {new Argument(53, "Object instance name",
               quoted(missingObjectName))}, "ObjectInstanceNotKnown", 1);
         verifyFailureReport(observer, subjectFederate, recorder.events.get(2), encoder,
            reportClass, reportParameters, reliable, "GetObjectInstanceName",
            new Argument[] {new Argument(37, "Object instance handle",
               quoted(invalidObject.toString()))}, "ObjectInstanceNotKnown", 2);
         verifyFailureReport(observer, subjectFederate, recorder.events.get(3), encoder,
            reportClass, reportParameters, reliable, "GetAttributeHandle",
            new Argument[] {
               new Argument(36, "Object class handle", quoted(invalidObjectClass.toString())),
               new Argument(53, "Class attribute name", quoted(attributeName))
            }, "InvalidObjectClassHandle", 3);
         verifyFailureReport(observer, subjectFederate, recorder.events.get(4), encoder,
            reportClass, reportParameters, reliable, "GetAttributeName",
            new Argument[] {
               new Argument(36, "Object class handle", quoted(objectClass.toString())),
               new Argument(0, "Class attribute handle", quoted(invalidAttribute.toString()))
            }, "InvalidAttributeHandle", 4);
         verifyFailureReport(observer, subjectFederate, recorder.events.get(5), encoder,
            reportClass, reportParameters, reliable, "GetUpdateRateValue",
            new Argument[] {new Argument(53, "Update rate name", quoted(missingRateName))},
            "InvalidUpdateRateDesignator", 5);
         verifyFailureReport(observer, subjectFederate, recorder.events.get(6), encoder,
            reportClass, reportParameters, reliable, "GetUpdateRateValueForAttribute",
            new Argument[] {
               new Argument(37, "Object instance handle", quoted(invalidObject.toString())),
               new Argument(0, "Attribute handle", quoted(attribute.toString()))
            }, "ObjectInstanceNotKnown", 6);
         verifySuccessReport(observer, subjectFederate, recorder.events.get(7), encoder,
            reportClass, reportParameters, reliable, "GetObjectClassHandle",
            new Argument[] {new Argument(53, "Object class name", quoted(objectClassName))},
            new Argument(36, "Object class handle", quoted(objectClassByName.toString())), 7);

         observer.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         subject.setServiceReportingSwitch(false);
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         observerJoined = false;
         subject.resignFederationExecution(ResignAction.NO_ACTION);
         subjectJoined = false;
         subject.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (reportSubscribed && observerJoined && reportClass != null) {
            try { observer.unsubscribeInteractionClass(reportClass); }
            catch (Exception ignored) { }
         }
         try { subject.setServiceReportingSwitch(false); } catch (Exception ignored) { }
         if (observerJoined) resign(observer);
         if (subjectJoined) resign(subject);
         if (created) {
            try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (observerConnected) disconnect(observer);
         if (subjectConnected) disconnect(subject);
      }
   }

   private static void verifyFailureReport(RTIambassador observer,
         FederateHandle expectedFederate, Interaction event, EncoderFactory encoder,
         InteractionClassHandle reportClass, Map<String, ParameterHandle> parameters,
         TransportationTypeHandle reliable, String service, Argument[] supplied,
         String expectedException, int serial) throws Exception {
      verifyCommon(observer, expectedFederate, event, encoder, reportClass, parameters,
         reliable, service, supplied, serial, false);
      HLAfixedRecord returned = decodeReturnedArgument(event, encoder, parameters);
      verifyArgument(returned, new Argument(34, "", "null"), service + " null return value");
      HLAunicodeString exception = encoder.createHLAunicodeString();
      exception.decode(event.parameters.get(parameters.get("HLAexception")));
      JavaTckSupport.check(exception.getValue().contains(expectedException),
         service + " failure report omitted " + expectedException);
   }

   private static void verifySuccessReport(RTIambassador observer,
         FederateHandle expectedFederate, Interaction event, EncoderFactory encoder,
         InteractionClassHandle reportClass, Map<String, ParameterHandle> parameters,
         TransportationTypeHandle reliable, String service, Argument[] supplied,
         Argument returned, int serial) throws Exception {
      verifyCommon(observer, expectedFederate, event, encoder, reportClass, parameters,
         reliable, service, supplied, serial, true);
      HLAfixedRecord result = decodeReturnedArgument(event, encoder, parameters);
      verifyArgument(result, returned, service + " returned argument");
      HLAunicodeString exception = encoder.createHLAunicodeString();
      exception.decode(event.parameters.get(parameters.get("HLAexception")));
      JavaTckSupport.check(exception.getValue().isEmpty(),
         service + " successful report carried an exception");
   }

   private static void verifyCommon(RTIambassador observer, FederateHandle expectedFederate,
         Interaction event, EncoderFactory encoder, InteractionClassHandle reportClass,
         Map<String, ParameterHandle> parameters, TransportationTypeHandle reliable,
         String service, Argument[] expectedSupplied, int serial, boolean successExpected)
         throws Exception {
      JavaTckSupport.check(reportClass.equals(event.interaction)
            && event.parameters.size() == REPORT_PARAMETERS.length
            && event.tag.length == 0 && event.producer == null && event.regionsEmpty
            && reliable.equals(event.transportation),
         service + " report had unexpected interaction or callback metadata");
      for (String name : REPORT_PARAMETERS) {
         JavaTckSupport.check(event.parameters.containsKey(parameters.get(name)),
            service + " report omitted standard parameter " + name);
      }
      HLAunicodeString decodedService = encoder.createHLAunicodeString();
      decodedService.decode(event.parameters.get(parameters.get("HLAservice")));
      HLAinteger16BE serviceType = encoder.createHLAinteger16BE();
      serviceType.decode(event.parameters.get(parameters.get("HLAserviceType")));
      HLAboolean success = encoder.createHLAboolean();
      success.decode(event.parameters.get(parameters.get("HLAsuccessIndicator")));
      HLAinteger32BE serialNumber = encoder.createHLAinteger32BE();
      serialNumber.decode(event.parameters.get(parameters.get("HLAserialNumber")));
      JavaTckSupport.check(service.equals(decodedService.getValue())
            && serviceType.getValue() == 6 && success.getValue() == successExpected
            && serialNumber.getValue() == serial,
         service + " report returned incorrect service status or serial number");
      FederateHandle reportedFederate = observer.getFederateHandleFactory().decode(
         event.parameters.get(parameters.get("HLAfederate")), 0);
      JavaTckSupport.check(expectedFederate.equals(reportedFederate),
         service + " report identified the wrong invoking federate");

      HLAfixedRecord prototype = argumentPrototype(encoder);
      HLAvariableArray<HLAfixedRecord> arguments = encoder.createHLAvariableArray(
         index -> argumentPrototype(encoder), prototype);
      arguments.decode(event.parameters.get(parameters.get("HLAsuppliedArguments")));
      JavaTckSupport.check(arguments.size() == expectedSupplied.length,
         service + " report returned the wrong supplied-argument count");
      for (int index = 0; index < expectedSupplied.length; index++) {
         verifyArgument(arguments.get(index), expectedSupplied[index],
            service + " supplied argument " + index);
      }
   }

   private static HLAfixedRecord decodeReturnedArgument(Interaction event, EncoderFactory encoder,
         Map<String, ParameterHandle> parameters) throws Exception {
      HLAfixedRecord result = argumentPrototype(encoder);
      result.decode(event.parameters.get(parameters.get("HLAreturnedArgument")));
      return result;
   }

   private static void verifyArgument(HLAfixedRecord record, Argument expected,
         String description) throws Exception {
      JavaTckSupport.check(record.size() == 3
            && ((HLAinteger32BE) record.get(0)).getValue() == expected.type
            && expected.name.equals(((HLAunicodeString) record.get(1)).getValue())
            && expected.value.equals(((HLAunicodeString) record.get(2)).getValue()),
         description + " had the wrong argument type, name, or value");
   }

   private static HLAfixedRecord argumentPrototype(EncoderFactory encoder) {
      HLAfixedRecord result = encoder.createHLAfixedRecord();
      result.add(encoder.createHLAinteger32BE());
      result.add(encoder.createHLAunicodeString());
      result.add(encoder.createHLAunicodeString());
      return result;
   }

   private static String uniqueName(String prefix) {
      return prefix + UUID.randomUUID().toString().replace("-", "");
   }

   private static String quoted(String value) { return "\"" + value + "\""; }

   private static void await(RTIambassador observer, Recorder recorder, CallbackModel model)
         throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (recorder.events.size() < 8 && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) observer.evokeCallback(0.01);
         else Thread.yield();
      }
      JavaTckSupport.check(recorder.events.size() >= 8,
         "timed out waiting for federate lookup failure/recovery service reports");
   }

   private static FederateAmbassador emptyCallbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> objectMethod(proxy, method.getName(), arguments));
   }

   private static Object objectMethod(Object proxy, String name, Object[] arguments) {
      if ("toString".equals(name)) return "Java TCK federate lookup failure callbacks";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == arguments[0];
      return null;
   }

   private static void resign(RTIambassador rti) {
      try { rti.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador rti) {
      try { rti.disconnect(); } catch (Exception ignored) { }
   }

   private static final class Argument {
      private final int type;
      private final String name;
      private final String value;
      private Argument(int type, String name, String value) {
         this.type = type;
         this.name = name;
         this.value = value;
      }
   }

   private static final class Interaction {
      private final InteractionClassHandle interaction;
      private final Map<ParameterHandle, byte[]> parameters;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final boolean regionsEmpty;
      private Interaction(InteractionClassHandle interaction,
            Map<ParameterHandle, byte[]> parameters, byte[] tag,
            TransportationTypeHandle transportation, FederateHandle producer,
            boolean regionsEmpty) {
         this.interaction = interaction;
         this.parameters = parameters;
         this.tag = tag;
         this.transportation = transportation;
         this.producer = producer;
         this.regionsEmpty = regionsEmpty;
      }
   }

   private static final class Recorder {
      private final List<Interaction> events = new CopyOnWriteArrayList<>();
      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("receiveInteraction".equals(method.getName()) && arguments != null
                     && arguments.length == 6) {
                  ParameterHandleValueMap source = (ParameterHandleValueMap) arguments[1];
                  Map<ParameterHandle, byte[]> values = new LinkedHashMap<>();
                  for (ParameterHandle handle : source.keySet()) {
                     values.put(handle, source.get(handle).clone());
                  }
                  RegionHandleSet regions = (RegionHandleSet) arguments[5];
                  events.add(new Interaction((InteractionClassHandle) arguments[0], values,
                     ((byte[]) arguments[2]).clone(), (TransportationTypeHandle) arguments[3],
                     (FederateHandle) arguments[4], regions == null || regions.isEmpty()));
               }
               return objectMethod(proxy, method.getName(), arguments);
            });
      }
   }
}
