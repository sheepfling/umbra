package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
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
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.encoding.HLAunicodeString;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.function.BooleanSupplier;

/** Verifies standard HLAreportMOMexception after malformed standard MIM parameters. */
final class FederateMomExceptionReportDeliveryTck {
   private static final String REPORT_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportMOMexception";
   private static final String SET_SWITCHES =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches";
   private static final String SYNCHRONIZATION_STATUS_REQUEST =
      "HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestSynchronizationPointStatus";
   private static final String SYNCHRONIZATION_STATUS_REPORT =
      "HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportSynchronizationPointStatus";
   private static final String FOM_MODULE_DATA_REQUEST =
      "HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestFOMmoduleData";
   private static final String FOM_MODULE_DATA_REPORT =
      "HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportFOMmoduleData";
   private static final String SERVICE_INVOCATION_REPORT =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
   private static final List<String> TABLE7_S_PARAMETER_REQUIRED_LEAVES = List.of(
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAmodifyAttributeState",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestDirectedInteractionsReceived",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestDirectedInteractionsSent",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestFOMmoduleData",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestInteractionsReceived",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestInteractionsSent",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstanceInformation",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstancesReflected",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstancesThatCanBeDeleted",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstancesUpdated",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestPublications",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestReflectionsReceived",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestSubscriptions",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestUpdatesSent",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAchangeAttributeOrderType",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAchangeInteractionOrderType",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAdeleteObjectInstance",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAdisableAsynchronousDelivery",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAenableAsynchronousDelivery",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAdisableTimeConstrained",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAdisableTimeRegulation",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAenableTimeConstrained",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAenableTimeRegulation",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAfederateRestoreComplete",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAfederateSaveComplete",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAflushQueueRequest",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAlocalDeleteObjectInstance",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAmodifyLookahead",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAnextMessageRequest",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAnextMessageRequestAvailable",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLApublishInteractionClass",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLApublishObjectClassAttributes",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLApublishObjectClassDirectedInteractions",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLArequestAttributeTransportationTypeChange",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLArequestInteractionTransportationTypeChange",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAresignFederationExecution",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAsubscribeInteractionClass",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAsubscribeObjectClassAttributes",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAsubscribeObjectClassDirectedInteractions",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAsynchronizationPointAchieved",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAtimeAdvanceRequest",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAtimeAdvanceRequestAvailable",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAunconditionalAttributeOwnershipDivestiture",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAunpublishInteractionClass",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAunpublishObjectClassAttributes",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAunpublishObjectClassDirectedInteractions",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAunsubscribeInteractionClass",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAunsubscribeObjectClassAttributes",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAunsubscribeObjectClassDirectedInteractions",
      "HLAinteractionRoot.HLAmanager.HLAfederation.HLAadjust.HLAsetSwitches",
      "HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestFOMmoduleData",
      "HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestSynchronizationPointStatus");
   private static final String REQUEST_MIM_DATA =
      "HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestMIMdata";
   private static final String REPORT_MIM_DATA =
      "HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportMIMdata";
   private static final String REQUEST_SYNCHRONIZATION_POINTS =
      "HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestSynchronizationPoints";
   private static final String REPORT_SYNCHRONIZATION_POINTS =
      "HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportSynchronizationPoints";
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private FederateMomExceptionReportDeliveryTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(timeImplementation != null && !timeImplementation.isEmpty(),
         "MOM exception-report testing requires adapter-supplied logical-time configuration");
      RTIambassador rti = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder recorder = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-mom-exception-report-" + suffix;
      String federateName = "java-tck-mom-exception-federate-" + suffix;
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      boolean subscribed = false;
      InteractionClassHandle reportClass = null;
      try {
         rti.connect(recorder.proxy(), callbackModel);
         connected = true;
         rti.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         rti.joinFederationExecution(federateName, "java-tck", federation);
         joined = true;

         reportClass = rti.getInteractionClassHandle(REPORT_CLASS);
         ParameterHandle serviceParameter = rti.getParameterHandle(reportClass, "HLAservice");
         ParameterHandle exceptionParameter = rti.getParameterHandle(reportClass, "HLAexception");
         ParameterHandle parameterErrorParameter = rti.getParameterHandle(
            reportClass, "HLAparameterError");
         ParameterHandle federateParameter = rti.getParameterHandle(reportClass, "HLAfederate");
         TransportationTypeHandle reliable = rti.getTransportationTypeHandle("HLAreliable");
         FederateHandle joinedFederate = rti.getFederateHandle(federateName);
         JavaTckSupport.check(reportClass != null && serviceParameter != null
               && exceptionParameter != null && parameterErrorParameter != null
               && federateParameter != null && reliable != null && joinedFederate != null,
            "standard MIM HLAreportMOMexception lookup returned an invalid handle");
         byte[] expectedFederate = encode(joinedFederate);
         rti.subscribeInteractionClass(reportClass);
         subscribed = true;

         InteractionClassHandle setSwitches = rti.getInteractionClassHandle(SET_SWITCHES);
         ParameterHandle resignActionParameter = rti.getParameterHandle(
            setSwitches, "HLAautomaticResignAction");
         ParameterHandle serviceReportingParameter = rti.getParameterHandle(
            setSwitches, "HLAserviceReporting");
         JavaTckSupport.check(setSwitches != null && resignActionParameter != null
               && serviceReportingParameter != null,
            "standard HLAsetSwitches malformed-value lookup returned an invalid handle");
         ResignAction initialResignAction = rti.getAutomaticResignDirective();
         boolean initialServiceReporting = rti.getServiceReportingSwitch();

         sendMalformedValue(rti, setSwitches, resignActionParameter, 6, recorder,
            callbackModel, encoder, "out-of-domain HLAresignAction value");
         JavaTckSupport.check(recorder.events.size() == 1,
            "malformed HLAresignAction did not deliver exactly one HLAreportMOMexception");
         sendMalformedValue(rti, setSwitches, serviceReportingParameter, 2, recorder,
            callbackModel, encoder, "out-of-domain HLAswitch value");
         JavaTckSupport.check(recorder.events.size() == 2,
            "malformed HLAserviceReporting did not deliver exactly two cumulative reports");

         for (Report report : recorder.events) {
            verifyReport(rti, report, reportClass, serviceParameter, exceptionParameter,
               parameterErrorParameter, federateParameter, reliable, expectedFederate,
               SET_SWITCHES, encoder);
         }
         JavaTckSupport.check(rti.getAutomaticResignDirective() == initialResignAction,
            "malformed HLAautomaticResignAction changed the joined federate's directive");
         JavaTckSupport.check(rti.getServiceReportingSwitch() == initialServiceReporting,
            "malformed HLAserviceReporting changed the joined federate's switch");
      } finally {
         if (subscribed && reportClass != null) {
            try { rti.unsubscribeInteractionClass(reportClass); }
            catch (Exception ignored) { }
         }
         if (joined) {
            try { rti.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { rti.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) {
            try { rti.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   static void runMissingParameter(RtiFactory factory, String fom, String mim,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      runMissingParameter(factory, fom, mim, timeImplementation, callbackModel,
         SYNCHRONIZATION_STATUS_REQUEST, SYNCHRONIZATION_STATUS_REPORT,
         "synchronization-status request", "HLAsyncPointName");
   }

   static void runMissingFomModuleIndicator(RtiFactory factory, String fom, String mim,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      runMissingParameter(factory, fom, mim, timeImplementation, callbackModel,
         FOM_MODULE_DATA_REQUEST, FOM_MODULE_DATA_REPORT,
         "FOM-module data request", "HLAFOMmoduleIndicator");
   }

   static void runTable7SLeafSubscription(RtiFactory factory, String fom, String mim,
         String timeImplementation, String objectClassName, String attributeName,
         String interactionClassName, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(timeImplementation != null && !timeImplementation.isEmpty(),
         "Table 7 S-leaf testing requires adapter-supplied logical-time configuration");
      JavaTckSupport.check(TABLE7_S_PARAMETER_REQUIRED_LEAVES.size() == 54,
         "the pinned Table 7 S-leaf inventory must contain 54 parameter-required requests");

      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder recorder = new Recorder();
      AtomicBoolean synchronizationPointRegistered = new AtomicBoolean();
      AtomicBoolean synchronizationPointRegistrationFailed = new AtomicBoolean();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-table7-s-leaf-" + suffix;
      String subjectName = "java-tck-table7-s-leaf-subject-" + suffix;
      String observerName = "java-tck-table7-s-leaf-observer-" + suffix;
      String synchronizationLabel = "java-tck-table7-s-leaf-sync-" + suffix;
      boolean subjectConnected = false;
      boolean observerConnected = false;
      boolean created = false;
      boolean subjectJoined = false;
      boolean observerJoined = false;
      boolean exceptionSubscribed = false;
      boolean mimReportSubscribed = false;
      boolean synchronizationReportSubscribed = false;
      boolean deleteObjectClassPublished = false;
      boolean deleteObjectInstanceRemoved = false;
      InteractionClassHandle exceptionReportClass = null;
      InteractionClassHandle mimReportClass = null;
      InteractionClassHandle synchronizationReportClass = null;
      ObjectClassHandle deleteObjectClass = null;
      AttributeHandle deleteObjectAttribute = null;
      ObjectInstanceHandle deleteObjectInstance = null;
      List<InteractionClassHandle> published = new ArrayList<>();
      try {
         subject.connect(registrationCallbacks(
            synchronizationPointRegistered, synchronizationPointRegistrationFailed),
            callbackModel);
         subjectConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         subject.joinFederationExecution(subjectName, "java-tck", federation);
         subjectJoined = true;
         observer.joinFederationExecution(observerName, "java-tck", federation);
         observerJoined = true;

         exceptionReportClass = observer.getInteractionClassHandle(REPORT_CLASS);
         ParameterHandle exceptionFederate = observer.getParameterHandle(
            exceptionReportClass, "HLAfederate");
         ParameterHandle exceptionService = observer.getParameterHandle(
            exceptionReportClass, "HLAservice");
         ParameterHandle exceptionText = observer.getParameterHandle(
            exceptionReportClass, "HLAexception");
         ParameterHandle parameterError = observer.getParameterHandle(
            exceptionReportClass, "HLAparameterError");
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         FederateHandle subjectHandle = observer.getFederateHandle(subjectName);
         JavaTckSupport.check(exceptionReportClass != null && exceptionFederate != null
               && exceptionService != null && exceptionText != null && parameterError != null
               && reliable != null && subjectHandle != null,
            "Table 7 S-leaf scenario could not resolve standard MOM report handles");
         byte[] encodedSubject = encode(subjectHandle);
         observer.subscribeInteractionClass(exceptionReportClass);
         exceptionSubscribed = true;

         ParameterHandleValueMap noParameters =
            subject.getParameterHandleValueMapFactory().create(0);
         for (String service : TABLE7_S_PARAMETER_REQUIRED_LEAVES) {
            InteractionClassHandle request = subject.getInteractionClassHandle(service);
            JavaTckSupport.check(request != null,
               "standard MIM omitted Table 7 S interaction " + service);
            subject.publishInteractionClass(request);
            published.add(request);
            boolean rejected = false;
            try {
               subject.sendInteraction(request, noParameters, new byte[0]);
            } catch (Exception expected) {
               rejected = true;
            }
            JavaTckSupport.check(rejected,
               "parameter-required Table 7 S request was not rejected without parameters: "
                  + service);
         }

         Set<String> expectedServices = new LinkedHashSet<>(TABLE7_S_PARAMETER_REQUIRED_LEAVES);
         await(observer, callbackModel, () -> recorder.events.size() >= expectedServices.size(),
            "54 HLAreportMOMexception callbacks for Table 7 S-leaf requests");
         settleCallbacks(observer, callbackModel);
         JavaTckSupport.check(recorder.events.size() == expectedServices.size(),
            "parameter-required Table 7 S requests did not produce exactly 54 MOM reports");
         Set<String> reportedServices = new LinkedHashSet<>();
         for (Report report : recorder.events) {
            HLAunicodeString serviceValue = encoder.createHLAunicodeString();
            serviceValue.decode(report.parameters.get(exceptionService));
            String service = serviceValue.getValue();
            JavaTckSupport.check(expectedServices.contains(service)
                  && reportedServices.add(service),
               "HLAreportMOMexception did not uniquely identify an expected S-leaf service");
            verifyReport(observer, report, exceptionReportClass, exceptionService,
               exceptionText, parameterError, exceptionFederate, reliable, encodedSubject,
               service, encoder);
            JavaTckSupport.check(report.tag.length == 0,
               "HLAreportMOMexception did not preserve the empty tag");
         }
         JavaTckSupport.check(reportedServices.equals(expectedServices),
            "MOM exception reports did not cover every parameter-required Table 7 S leaf");

          deleteObjectClass = subject.getObjectClassHandle(objectClassName);
          JavaTckSupport.check(deleteObjectClass != null,
             "HLAdeleteObjectInstance effect probe could not resolve its FOM class");
          deleteObjectAttribute = subject.getAttributeHandle(deleteObjectClass, attributeName);
          JavaTckSupport.check(deleteObjectAttribute != null,
             "HLAdeleteObjectInstance effect probe could not resolve its FOM attribute");
          AttributeHandleSet deleteObjectAttributes =
             JavaTckSupport.attributeSet(subject, deleteObjectAttribute);
          subject.publishObjectClassAttributes(deleteObjectClass, deleteObjectAttributes);
          deleteObjectClassPublished = true;
          ObjectInstanceHandle objectToDelete = subject.registerObjectInstance(deleteObjectClass);
          JavaTckSupport.check(objectToDelete != null,
             "HLAdeleteObjectInstance effect probe could not register its target object");
          deleteObjectInstance = objectToDelete;
          InteractionClassHandle deleteRequest = subject.getInteractionClassHandle(
             "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAdeleteObjectInstance");
          JavaTckSupport.check(deleteRequest != null,
             "standard MIM omitted the HLAdeleteObjectInstance request");
          ParameterHandle objectInstanceParameter =
             subject.getParameterHandle(deleteRequest, "HLAobjectInstance");
          ParameterHandle deleteTagParameter = subject.getParameterHandle(deleteRequest, "HLAtag");
          JavaTckSupport.check(objectInstanceParameter != null && deleteTagParameter != null,
             "standard MIM omitted HLAdeleteObjectInstance request parameters");
          subject.publishInteractionClass(deleteRequest);
          published.add(deleteRequest);
          byte[] encodedObject = new byte[objectToDelete.encodedLength()];
          objectToDelete.encode(encodedObject, 0);
          ParameterHandleValueMap deleteParameters =
             subject.getParameterHandleValueMapFactory().create(2);
          deleteParameters.put(objectInstanceParameter, encodedObject);
          deleteParameters.put(deleteTagParameter, new byte[0]);
          subject.sendInteraction(deleteRequest, deleteParameters, new byte[0]);
          JavaTckSupport.expectFailure(
             () -> subject.getObjectInstanceName(objectToDelete), "ObjectInstanceNotKnown",
             "valid HLAdeleteObjectInstance request did not remove its target");
          deleteObjectInstanceRemoved = true;
          subject.unpublishObjectClassAttributes(deleteObjectClass, deleteObjectAttributes);
          deleteObjectClassPublished = false;

         mimReportClass = observer.getInteractionClassHandle(REPORT_MIM_DATA);
         InteractionClassHandle mimRequest = subject.getInteractionClassHandle(REQUEST_MIM_DATA);
         JavaTckSupport.check(mimReportClass != null && mimRequest != null,
            "parameterless MIM-data request/report handles were invalid");
         observer.subscribeInteractionClass(mimReportClass);
         mimReportSubscribed = true;
         subject.publishInteractionClass(mimRequest);
         published.add(mimRequest);
         recorder.events.clear();
         subject.sendInteraction(mimRequest, noParameters, new byte[0]);
         await(observer, callbackModel, () -> !recorder.events.isEmpty(),
            "HLAreportMIMdata for the parameterless HLArequestMIMdata request");
         settleCallbacks(observer, callbackModel);
         JavaTckSupport.check(recorder.events.size() == 1,
            "parameterless HLArequestMIMdata produced an unexpected report count");
         verifyReportMetadata(observer, recorder.events.get(0), mimReportClass,
            reliable, "HLAreportMIMdata");

         synchronizationReportClass =
            observer.getInteractionClassHandle(REPORT_SYNCHRONIZATION_POINTS);
         InteractionClassHandle synchronizationRequest =
            subject.getInteractionClassHandle(REQUEST_SYNCHRONIZATION_POINTS);
         JavaTckSupport.check(synchronizationReportClass != null
               && synchronizationRequest != null,
            "parameterless synchronization-point request/report handles were invalid");
         observer.subscribeInteractionClass(synchronizationReportClass);
         synchronizationReportSubscribed = true;
         subject.publishInteractionClass(synchronizationRequest);
         published.add(synchronizationRequest);
         subject.registerFederationSynchronizationPoint(synchronizationLabel, new byte[0]);
         await(subject, callbackModel,
            () -> synchronizationPointRegistered.get()
               || synchronizationPointRegistrationFailed.get(),
            "synchronization-point registration for the parameterless S-leaf query");
         JavaTckSupport.check(synchronizationPointRegistered.get(),
            "synchronization-point registration failed for the parameterless S-leaf query");
         recorder.events.clear();
         subject.sendInteraction(synchronizationRequest, noParameters, new byte[0]);
         await(observer, callbackModel, () -> !recorder.events.isEmpty(),
            "HLAreportSynchronizationPoints for the parameterless request");
         settleCallbacks(observer, callbackModel);
         JavaTckSupport.check(recorder.events.size() == 1,
            "parameterless HLArequestSynchronizationPoints produced an unexpected report count");
         verifyReportMetadata(observer, recorder.events.get(0), synchronizationReportClass,
            reliable, "HLAreportSynchronizationPoints");
      } finally {
          if (deleteObjectInstance != null && !deleteObjectInstanceRemoved && subjectJoined) {
             try { subject.deleteObjectInstance(deleteObjectInstance, new byte[0]); }
             catch (Exception ignored) { }
          }
          if (deleteObjectClassPublished && deleteObjectClass != null
                && deleteObjectAttribute != null && subjectJoined) {
             try { subject.unpublishObjectClassAttributes(deleteObjectClass,
                JavaTckSupport.attributeSet(subject, deleteObjectAttribute)); }
             catch (Exception ignored) { }
          }
         for (InteractionClassHandle interaction : published) {
            try { subject.unpublishInteractionClass(interaction); } catch (Exception ignored) { }
         }
         if (synchronizationReportSubscribed && synchronizationReportClass != null) {
            try { observer.unsubscribeInteractionClass(synchronizationReportClass); }
            catch (Exception ignored) { }
         }
         if (mimReportSubscribed && mimReportClass != null) {
            try { observer.unsubscribeInteractionClass(mimReportClass); }
            catch (Exception ignored) { }
         }
         if (exceptionSubscribed && exceptionReportClass != null) {
            try { observer.unsubscribeInteractionClass(exceptionReportClass); }
            catch (Exception ignored) { }
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
       Table7DirectedInteractionPublicationTck.run(
          factory, mim, timeImplementation, callbackModel);
       Table7DirectedInteractionUnpublicationTck.run(
          factory, mim, timeImplementation, callbackModel);
       Table7SynchronizationPointAchievementTck.run(
          factory, fom, mim, timeImplementation, callbackModel);
       Table7AsynchronousDeliveryControlTck.run(
          factory, mim, timeImplementation, callbackModel);
       Table7TimeConstrainedControlTck.run(
          factory, fom, mim, timeImplementation, callbackModel);
       Table7TimeRegulationControlTck.run(
          factory, fom, mim, timeImplementation, callbackModel);
       Table7FlushQueueRequestTck.run(
          factory, fom, mim, timeImplementation, callbackModel);
       Table7LocalDeleteObjectInstanceTck.run(
          factory, fom, mim, timeImplementation, objectClassName, attributeName, callbackModel);
       Table7ModifyLookaheadTck.run(
          factory, fom, mim, timeImplementation, callbackModel);
       Table7NextMessageRequestTck.run(
          factory, fom, mim, timeImplementation, callbackModel);
       Table7TimeAdvanceRequestMomTck.run(
          factory, fom, mim, timeImplementation, callbackModel);
       Table7UnconditionalAttributeOwnershipDivestitureTck.run(
          factory, fom, mim, timeImplementation, objectClassName, attributeName, callbackModel);
       Table7PublishInteractionClassTck.run(
          factory, fom, mim, timeImplementation, interactionClassName, callbackModel);
      Table7UnpublishInteractionClassTck.run(
         factory, fom, mim, timeImplementation, interactionClassName, callbackModel);
      Table7PublishObjectClassAttributesTck.run(
         factory, fom, mim, timeImplementation, objectClassName, attributeName, callbackModel);
      Table7UnpublishObjectClassAttributesTck.run(
         factory, fom, mim, timeImplementation, objectClassName, attributeName, callbackModel);
   }

   private static void runMissingParameter(RtiFactory factory, String fom, String mim,
         String timeImplementation, CallbackModel callbackModel, String requestInteraction,
         String relatedReportInteraction, String requestDescription, String missingParameter)
         throws Exception {
      JavaTckSupport.check(timeImplementation != null && !timeImplementation.isEmpty(),
         "missing-parameter MOM exception reporting requires adapter-supplied logical-time configuration");
      RTIambassador rti = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder recorder = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-mom-exception-missing-parameter-" + suffix;
      String federateName = "java-tck-mom-exception-missing-parameter-federate-" + suffix;
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      boolean reportSubscribed = false;
      boolean statusSubscribed = false;
      InteractionClassHandle reportClass = null;
      InteractionClassHandle statusReportClass = null;
      try {
         rti.connect(recorder.proxy(), callbackModel);
         connected = true;
         rti.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         rti.joinFederationExecution(federateName, "java-tck", federation);
         joined = true;

         reportClass = rti.getInteractionClassHandle(REPORT_CLASS);
         ParameterHandle serviceParameter = rti.getParameterHandle(reportClass, "HLAservice");
         ParameterHandle exceptionParameter = rti.getParameterHandle(reportClass, "HLAexception");
         ParameterHandle parameterErrorParameter = rti.getParameterHandle(
            reportClass, "HLAparameterError");
         ParameterHandle federateParameter = rti.getParameterHandle(reportClass, "HLAfederate");
         TransportationTypeHandle reliable = rti.getTransportationTypeHandle("HLAreliable");
         FederateHandle joinedFederate = rti.getFederateHandle(federateName);
         InteractionClassHandle requestClass = rti.getInteractionClassHandle(requestInteraction);
         statusReportClass = rti.getInteractionClassHandle(relatedReportInteraction);
         JavaTckSupport.check(reportClass != null && serviceParameter != null
               && exceptionParameter != null && parameterErrorParameter != null
               && federateParameter != null && reliable != null && joinedFederate != null
               && requestClass != null && statusReportClass != null,
            "standard MIM " + requestDescription
               + " exception-report lookup returned an invalid handle");
         byte[] expectedFederate = encode(joinedFederate);

         rti.subscribeInteractionClass(reportClass);
         reportSubscribed = true;
         rti.subscribeInteractionClass(statusReportClass);
         statusSubscribed = true;

         ParameterHandleValueMap noParameters =
            rti.getParameterHandleValueMapFactory().create(0);
         try {
            rti.sendInteraction(requestClass, noParameters, new byte[0]);
         } catch (Exception allowedSynchronousFailure) {
            // The standard HLAreportMOMexception callback is the portable result.
         }
         await(rti, callbackModel, () -> !recorder.events.isEmpty(),
            "HLAreportMOMexception for " + requestDescription
               + " missing " + missingParameter);
         settleCallbacks(rti, callbackModel);
         JavaTckSupport.check(recorder.events.size() == 1,
            requestDescription + " missing " + missingParameter
               + " did not deliver exactly one MOM exception report");
         verifyReport(rti, recorder.events.get(0), reportClass, serviceParameter,
            exceptionParameter,
            parameterErrorParameter, federateParameter, reliable, expectedFederate,
            requestInteraction, encoder);
      } finally {
         if (statusSubscribed && statusReportClass != null) {
            try { rti.unsubscribeInteractionClass(statusReportClass); }
            catch (Exception ignored) { }
         }
         if (reportSubscribed && reportClass != null) {
            try { rti.unsubscribeInteractionClass(reportClass); }
            catch (Exception ignored) { }
         }
         if (joined) {
            try { rti.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { rti.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) {
            try { rti.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   static void runServiceReportingPrecondition(RtiFactory factory, String fom, String mim,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(timeImplementation != null && !timeImplementation.isEmpty(),
         "service-reporting precondition testing requires adapter-supplied logical-time configuration");
      RTIambassador rti = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder recorder = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-mom-exception-report-precondition-" + suffix;
      String federateName = "java-tck-mom-exception-report-precondition-federate-" + suffix;
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      boolean exceptionReportSubscribed = false;
      boolean serviceReportSubscribed = false;
      InteractionClassHandle exceptionReportClass = null;
      InteractionClassHandle serviceReportClass = null;
      try {
         rti.connect(recorder.proxy(), callbackModel);
         connected = true;
         rti.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         rti.joinFederationExecution(federateName, "java-tck", federation);
         joined = true;

         exceptionReportClass = rti.getInteractionClassHandle(REPORT_CLASS);
         ParameterHandle serviceParameter =
            rti.getParameterHandle(exceptionReportClass, "HLAservice");
         ParameterHandle exceptionParameter =
            rti.getParameterHandle(exceptionReportClass, "HLAexception");
         ParameterHandle parameterErrorParameter =
            rti.getParameterHandle(exceptionReportClass, "HLAparameterError");
         ParameterHandle federateParameter =
            rti.getParameterHandle(exceptionReportClass, "HLAfederate");
         TransportationTypeHandle reliable = rti.getTransportationTypeHandle("HLAreliable");
         FederateHandle joinedFederate = rti.getFederateHandle(federateName);
         serviceReportClass = rti.getInteractionClassHandle(SERVICE_INVOCATION_REPORT);
         InteractionClassHandle setSwitchesClass = rti.getInteractionClassHandle(SET_SWITCHES);
         ParameterHandle serviceReportingParameter =
            rti.getParameterHandle(setSwitchesClass, "HLAserviceReporting");
         JavaTckSupport.check(exceptionReportClass != null && serviceParameter != null
               && exceptionParameter != null && parameterErrorParameter != null
               && federateParameter != null && reliable != null && joinedFederate != null
               && serviceReportClass != null && setSwitchesClass != null
               && serviceReportingParameter != null,
            "standard MIM service-reporting precondition lookup returned an invalid handle");
         byte[] expectedFederate = encode(joinedFederate);

         rti.subscribeInteractionClass(exceptionReportClass);
         exceptionReportSubscribed = true;
         rti.subscribeInteractionClass(serviceReportClass);
         serviceReportSubscribed = true;
         JavaTckSupport.check(!rti.getServiceReportingSwitch(),
            "standard Service Reporting switch did not begin disabled");

         try {
            sendServiceReportingValue(rti, setSwitchesClass, serviceReportingParameter,
               encoder, 1);
         } catch (Exception allowedSynchronousFailure) {
            // The standard MOM report, not an exception class, is the portable oracle.
         }
         await(rti, callbackModel, () -> !recorder.events.isEmpty(),
            "HLAreportMOMexception for the Service Reporting subscription precondition");
         settleCallbacks(rti, callbackModel);
         JavaTckSupport.check(recorder.events.size() == 1,
            "rejected Service Reporting enablement did not produce exactly one MOM exception report");
         verifyReport(rti, recorder.events.get(0), exceptionReportClass, serviceParameter,
            exceptionParameter, parameterErrorParameter, federateParameter, reliable,
            expectedFederate,
            "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches",
            false, encoder);
         JavaTckSupport.check(!rti.getServiceReportingSwitch(),
            "rejected Service Reporting enablement changed the switch state");

         rti.unsubscribeInteractionClass(serviceReportClass);
         serviceReportSubscribed = false;
         sendServiceReportingValue(rti, setSwitchesClass, serviceReportingParameter, encoder, 1);
         JavaTckSupport.check(rti.getServiceReportingSwitch(),
            "Service Reporting did not enable after removing its report-subscription precondition");
         settleCallbacks(rti, callbackModel);
         JavaTckSupport.check(recorder.events.size() == 1
               && recorder.events.get(0).interaction.equals(exceptionReportClass),
            "successful Service Reporting enablement delivered an unexpected subscribed report");

         sendServiceReportingValue(rti, setSwitchesClass, serviceReportingParameter, encoder, 0);
         JavaTckSupport.check(!rti.getServiceReportingSwitch(),
            "Service Reporting did not return to its initial disabled value");
      } finally {
         if (serviceReportSubscribed && serviceReportClass != null) {
            try { rti.unsubscribeInteractionClass(serviceReportClass); }
            catch (Exception ignored) { }
         }
         if (exceptionReportSubscribed && exceptionReportClass != null) {
            try { rti.unsubscribeInteractionClass(exceptionReportClass); }
            catch (Exception ignored) { }
         }
         if (joined) {
            try { rti.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { rti.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) {
            try { rti.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static void sendServiceReportingValue(RTIambassador rti,
         InteractionClassHandle setSwitchesClass, ParameterHandle serviceReportingParameter,
         EncoderFactory encoder, int value) throws Exception {
      HLAinteger32BE encoded = encoder.createHLAinteger32BE(value);
      ParameterHandleValueMap parameters = rti.getParameterHandleValueMapFactory().create(1);
      parameters.put(serviceReportingParameter, encoded.toByteArray());
      rti.sendInteraction(setSwitchesClass, parameters, new byte[0]);
   }

   private static void sendMalformedValue(RTIambassador rti,
         InteractionClassHandle setSwitches, ParameterHandle parameter, int value,
         Recorder recorder, CallbackModel callbackModel, EncoderFactory encoder,
         String description) throws Exception {
      HLAinteger32BE encoded = encoder.createHLAinteger32BE(value);
      ParameterHandleValueMap parameters = rti.getParameterHandleValueMapFactory().create(1);
      parameters.put(parameter, encoded.toByteArray());
      int before = recorder.events.size();
      try {
         rti.sendInteraction(setSwitches, parameters, new byte[0]);
      } catch (Exception allowedSynchronousFailure) {
         // The standard MOM callback remains the portable result for malformed data.
      }
      await(rti, callbackModel, () -> recorder.events.size() > before,
         "HLAreportMOMexception for " + description);
      JavaTckSupport.check(recorder.events.size() == before + 1,
         description + " delivered an unexpected number of MOM exception reports");
   }

   private static void verifyReport(RTIambassador rti, Report report,
         InteractionClassHandle reportClass,
         ParameterHandle serviceParameter, ParameterHandle exceptionParameter,
         ParameterHandle parameterErrorParameter, ParameterHandle federateParameter,
         TransportationTypeHandle reliable, byte[] expectedFederate, String expectedService,
         EncoderFactory encoder) throws Exception {
      verifyReport(rti, report, reportClass, serviceParameter, exceptionParameter,
         parameterErrorParameter, federateParameter, reliable, expectedFederate,
         expectedService, true, encoder);
   }

   private static void verifyReport(RTIambassador rti, Report report,
         InteractionClassHandle reportClass,
         ParameterHandle serviceParameter, ParameterHandle exceptionParameter,
         ParameterHandle parameterErrorParameter, ParameterHandle federateParameter,
         TransportationTypeHandle reliable, byte[] expectedFederate, String expectedService,
         boolean expectedParameterError, EncoderFactory encoder) throws Exception {
      JavaTckSupport.check(report.interaction.equals(reportClass)
            && report.parameters.size() == 4
            && report.parameters.containsKey(serviceParameter)
            && report.parameters.containsKey(exceptionParameter)
            && report.parameters.containsKey(parameterErrorParameter)
            && report.parameters.containsKey(federateParameter)
            && reliable.equals(report.transportation) && report.regionsEmpty,
         "HLAreportMOMexception callback metadata or parameter set was not standard");
      JavaTckSupport.requireNoProducingFederate(rti, report.producer,
         "HLAreportMOMexception");
      HLAunicodeString service = encoder.createHLAunicodeString();
      service.decode(report.parameters.get(serviceParameter));
      HLAunicodeString exception = encoder.createHLAunicodeString();
      exception.decode(report.parameters.get(exceptionParameter));
      HLAboolean parameterError = encoder.createHLAboolean();
      parameterError.decode(report.parameters.get(parameterErrorParameter));
      JavaTckSupport.check(expectedService.equals(service.getValue()),
         "HLAreportMOMexception did not identify the expected fully qualified standard interaction");
      JavaTckSupport.check(!exception.getValue().isEmpty(),
         "HLAreportMOMexception did not provide a textual problem description");
      JavaTckSupport.check(parameterError.getValue() == expectedParameterError,
         expectedParameterError
            ? "HLAreportMOMexception did not mark the malformed parameter"
            : "HLAreportMOMexception incorrectly marked a service precondition failure as a parameter error");
      JavaTckSupport.check(Arrays.equals(expectedFederate,
            report.parameters.get(federateParameter)),
         "HLAreportMOMexception identified a different joined federate");
   }

   private static void verifyReportMetadata(RTIambassador rti, Report report,
         InteractionClassHandle expectedClass, TransportationTypeHandle reliable,
         String description) throws Exception {
      JavaTckSupport.check(expectedClass.equals(report.interaction)
            && reliable.equals(report.transportation) && report.tag.length == 0
            && report.regionsEmpty,
         description + " did not preserve its class, empty tag, reliable transport, and no regions");
      JavaTckSupport.requireNoProducingFederate(rti, report.producer, description);
   }

   private static byte[] encode(FederateHandle federate) throws Exception {
      byte[] result = new byte[federate.encodedLength()];
      federate.encode(result, 0);
      return result;
   }

   private static void await(RTIambassador rti, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (!condition.getAsBoolean()) {
         if (System.nanoTime() >= deadline) {
            throw new AssertionError("timed out waiting for " + description);
         }
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(rti);
         else Thread.yield();
         Thread.sleep(1L);
      }
   }

   private static void settleCallbacks(RTIambassador rti, CallbackModel callbackModel)
         throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(rti);
      else Thread.sleep(25L);
   }

   private static FederateAmbassador callbackProxy() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, args) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK MOM exception callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) return proxy == (args == null ? null : args[0]);
            }
            return null;
         });
   }

   private static FederateAmbassador registrationCallbacks(AtomicBoolean succeeded,
         AtomicBoolean failed) {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, args) -> {
            if ("synchronizationPointRegistrationSucceeded".equals(method.getName())) {
               succeeded.set(true);
            } else if ("synchronizationPointRegistrationFailed".equals(method.getName())) {
               failed.set(true);
            }
            return callbackObjectMethod(proxy, method.getName(), args);
         });
   }

   private static final class Report {
      private InteractionClassHandle interaction;
      private Map<ParameterHandle, byte[]> parameters;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private boolean regionsEmpty;
   }

   private static final class Recorder {
      private final List<Report> events = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, args) -> {
               if ("receiveInteraction".equals(method.getName()) && args != null
                     && args.length == 6 && args[1] instanceof ParameterHandleValueMap) {
                  Report report = new Report();
                  report.interaction = (InteractionClassHandle) args[0];
                  ParameterHandleValueMap source = (ParameterHandleValueMap) args[1];
                  report.parameters = new LinkedHashMap<>();
                  for (ParameterHandle handle : source.keySet()) {
                     report.parameters.put(handle, source.get(handle).clone());
                  }
                  report.tag = args[2] instanceof byte[] ? ((byte[]) args[2]).clone() : new byte[0];
                  report.transportation = (TransportationTypeHandle) args[3];
                  report.producer = (FederateHandle) args[4];
                  report.regionsEmpty = args[5] == null || ((RegionHandleSet) args[5]).isEmpty();
                  events.add(report);
               }
               return callbackObjectMethod(proxy, method.getName(), args);
            });
      }
   }

   private static Object callbackObjectMethod(Object proxy, String name, Object[] args) {
      if ("toString".equals(name)) return "Java TCK MOM exception recorder";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == (args == null ? null : args[0]);
      return null;
   }
}
