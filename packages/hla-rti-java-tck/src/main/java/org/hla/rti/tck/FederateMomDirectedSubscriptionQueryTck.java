package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.InteractionClassHandleFactory;
import hla.rti1516_2025.InteractionClassHandleSet;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectClassHandleFactory;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAboolean;
import hla.rti1516_2025.encoding.HLAbyte;
import hla.rti1516_2025.encoding.HLAfixedRecord;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.encoding.HLAvariableArray;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies standard-MIM directed-subscription reports across two object classes. */
final class FederateMomDirectedSubscriptionQueryTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 15_000_000_000L;
   private static final String REQUEST =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestSubscriptions";
   private static final String OBJECT_REPORT =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectClassSubscription";
   private static final String INTERACTION_REPORT =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionSubscription";
   private static final String DIRECTED_REPORT =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportDirectedInteractionSubscription";

   private FederateMomDirectedSubscriptionQueryTck() { }

   static void runSingle(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-federate-mom-directed-subscription-" + UUID.randomUUID();
      String subjectName = "java-tck-directed-subscription-subject-" + UUID.randomUUID();
      String requesterName = "java-tck-directed-subscription-requester-" + UUID.randomUUID();
      boolean subjectConnected = false, requesterConnected = false, created = false;
      boolean subjectJoined = false, requesterJoined = false, directedSubscribed = false;
      boolean objectReportSubscribed = false, interactionReportSubscribed = false;
      boolean directedReportSubscribed = false;
      InteractionClassHandle objectReportClass = null;
      InteractionClassHandle interactionReportClass = null;
      InteractionClassHandle directedReportClass = null;
      try {
         subject.connect(callbacks(), callbackModel);
         subjectConnected = true;
         requester.connect(recorder.proxy(), callbackModel);
         requesterConnected = true;
         subject.createFederationExecutionWithMIM(federation, new String[] {fom}, mim,
            timeImplementation);
         created = true;
         subject.joinFederationExecution(subjectName, "java-tck", federation);
         subjectJoined = true;
         FederateHandle targetFederate = requester.joinFederationExecution(
            requesterName, "java-tck", federation);
         requesterJoined = true;

         ObjectClassHandle subjectObjectClass = subject.getObjectClassHandle(objectClassName);
         ObjectClassHandle requesterObjectClass = requester.getObjectClassHandle(objectClassName);
         AttributeHandle subjectAttribute = subject.getAttributeHandle(subjectObjectClass,
            attributeName);
         InteractionClassHandle subjectInteraction =
            subject.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle requesterInteraction =
            requester.getInteractionClassHandle(interactionClassName);
         TransportationTypeHandle reliable = requester.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(subjectObjectClass != null && requesterObjectClass != null
               && subjectAttribute != null && subjectInteraction != null
               && requesterInteraction != null && targetFederate != null && reliable != null,
            "adapter-selected directed-subscription handles were unavailable");

         subject.subscribeObjectClassDirectedInteractions(subjectObjectClass,
            JavaTckSupport.interactionSet(subject, subjectInteraction));
         directedSubscribed = true;

         InteractionClassHandle requestClass = requester.getInteractionClassHandle(REQUEST);
         ParameterHandle requestFederate = requester.getParameterHandle(requestClass, "HLAfederate");
         objectReportClass = requester.getInteractionClassHandle(OBJECT_REPORT);
         ParameterHandle objectCount = requester.getParameterHandle(
            objectReportClass, "HLAnumberOfClasses");
         ParameterHandle objectClassParameter = requester.getParameterHandle(
            objectReportClass, "HLAobjectClass");
         ParameterHandle objectActive = requester.getParameterHandle(objectReportClass, "HLAactive");
         ParameterHandle objectRate = requester.getParameterHandle(
            objectReportClass, "HLAmaxUpdateRate");
         ParameterHandle objectAttributes = requester.getParameterHandle(
            objectReportClass, "HLAattributeList");
         interactionReportClass = requester.getInteractionClassHandle(INTERACTION_REPORT);
         ParameterHandle interactionList = requester.getParameterHandle(
            interactionReportClass, "HLAinteractionClassList");
         directedReportClass = requester.getInteractionClassHandle(DIRECTED_REPORT);
         ParameterHandle directedCount = requester.getParameterHandle(
            directedReportClass, "HLAnumberOfClasses");
         ParameterHandle directedObjectClass = requester.getParameterHandle(
            directedReportClass, "HLAobjectClass");
         ParameterHandle directedList = requester.getParameterHandle(
            directedReportClass, "HLAinteractionClassList");
         JavaTckSupport.check(requestClass != null && requestFederate != null
               && objectReportClass != null && objectCount != null
               && objectClassParameter != null && objectActive != null && objectRate != null
               && objectAttributes != null && interactionReportClass != null
               && interactionList != null && directedReportClass != null
               && directedCount != null && directedObjectClass != null && directedList != null,
            "standard directed-subscription request/report lookup returned a null handle");

         requester.subscribeInteractionClass(objectReportClass);
         objectReportSubscribed = true;
         requester.subscribeInteractionClass(interactionReportClass);
         interactionReportSubscribed = true;
         requester.subscribeInteractionClass(directedReportClass);
         directedReportSubscribed = true;
         ParameterHandleValueMap request = requester.getParameterHandleValueMapFactory().create(1);
         request.put(requestFederate, encode(targetFederate));
         requester.sendInteraction(requestClass, request, new byte[0]);
         await(requester, callbackModel, () -> recorder.interactions.size() >= 3,
            "directed-subscription and ordinary NULL reports");
         JavaTckSupport.check(recorder.interactions.size() == 3,
            "directed-subscription query did not deliver exactly three standard reports");

         Interaction objectReport = onlyReport(recorder.interactions, objectReportClass,
            "object-subscription");
         Interaction interactionReport = onlyReport(recorder.interactions, interactionReportClass,
            "ordinary interaction-subscription");
         Interaction directedReport = onlyReport(recorder.interactions, directedReportClass,
            "directed-subscription");
         verifyMetadata(objectReport, reliable, "object-subscription report");
         verifyMetadata(interactionReport, reliable, "interaction-subscription report");
         verifyMetadata(directedReport, reliable, "directed-subscription report");
         verifyNullObjectReport(encoder, objectReport, objectCount, objectClassParameter,
            objectActive, objectRate, objectAttributes, "object-subscription report");
         JavaTckSupport.check(interactionReport.parameters.size() == 1
               && interactionReport.parameters.containsKey(interactionList)
               && decodeSubscriptions(encoder, requester.getInteractionClassHandleFactory(),
                  interactionReport.parameters.get(interactionList)).isEmpty(),
            "ordinary interaction-subscription report did not use its empty-list NULL shape");
         JavaTckSupport.check(directedReport.parameters.size() == 3
               && directedReport.parameters.containsKey(directedCount)
               && directedReport.parameters.containsKey(directedObjectClass)
               && directedReport.parameters.containsKey(directedList),
            "populated directed-subscription report did not contain its three standard parameters");
         HLAinteger32BE classCount = encoder.createHLAinteger32BE();
         classCount.decode(directedReport.parameters.get(directedCount));
         ObjectClassHandle reportedObjectClass = requester.getObjectClassHandleFactory().decode(
            directedReport.parameters.get(directedObjectClass), 0);
         List<Subscription> subscriptions = decodeSubscriptions(encoder,
            requester.getInteractionClassHandleFactory(),
            directedReport.parameters.get(directedList));
         JavaTckSupport.check(classCount.getValue() == 1
               && requesterObjectClass.equals(reportedObjectClass)
               && subscriptions.size() == 1
               && requesterInteraction.equals(subscriptions.get(0).interaction)
               && !subscriptions.get(0).active,
            "directed-subscription report did not identify exactly the adapter-selected passive pair");
      } finally {
         if (objectReportSubscribed && requesterJoined && objectReportClass != null) try {
            requester.unsubscribeInteractionClass(objectReportClass);
         } catch (Exception ignored) { }
         if (interactionReportSubscribed && requesterJoined && interactionReportClass != null) try {
            requester.unsubscribeInteractionClass(interactionReportClass);
         } catch (Exception ignored) { }
         if (directedReportSubscribed && requesterJoined && directedReportClass != null) try {
            requester.unsubscribeInteractionClass(directedReportClass);
         } catch (Exception ignored) { }
         if (directedSubscribed && subjectJoined) try {
            subject.unsubscribeObjectClassDirectedInteractions(
               subject.getObjectClassHandle(objectClassName));
         } catch (Exception ignored) { }
         if (requesterJoined) try { requester.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (subjectJoined) try { subject.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (created) try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (requesterConnected) try { requester.disconnect(); } catch (Exception ignored) { }
         if (subjectConnected) try { subject.disconnect(); } catch (Exception ignored) { }
      }
   }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String secondObjectClassName, String attributeName,
         String interactionClassName, String secondDirectedInteractionClassName,
         CallbackModel callbackModel) throws Exception {
      run(factory, fom, mim, timeImplementation, objectClassName, secondObjectClassName,
         attributeName, interactionClassName, secondDirectedInteractionClassName,
         callbackModel, false);
   }

   static void runAfterFullUnsubscribe(RtiFactory factory, String fom, String mim,
         String timeImplementation, String objectClassName, String secondObjectClassName,
         String attributeName, String interactionClassName,
         String secondDirectedInteractionClassName, CallbackModel callbackModel)
         throws Exception {
      run(factory, fom, mim, timeImplementation, objectClassName, secondObjectClassName,
         attributeName, interactionClassName, secondDirectedInteractionClassName,
         callbackModel, true);
   }

   private static void run(RtiFactory factory, String fom, String mim,
         String timeImplementation, String objectClassName, String secondObjectClassName,
         String attributeName, String interactionClassName,
         String secondDirectedInteractionClassName, CallbackModel callbackModel,
         boolean fullUnsubscribe) throws Exception {
      JavaTckSupport.check(secondObjectClassName != null && !secondObjectClassName.isEmpty()
            && !objectClassName.equals(secondObjectClassName)
            && secondDirectedInteractionClassName != null
            && !secondDirectedInteractionClassName.isEmpty()
            && !interactionClassName.equals(secondDirectedInteractionClassName),
         "directed-subscription query requires distinct adapter-supplied object and interaction class names");
      String federateType = System.getProperty("hla.rti.tck.federateType", "tck-type");
      JavaTckSupport.check(!federateType.isEmpty(),
         "adapter-supplied federate type must not be empty");

      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-federate-mom-multiple-directed-subscription-" + UUID.randomUUID();
      String subjectName = "java-tck-directed-subscription-subject-" + UUID.randomUUID();
      String requesterName = "java-tck-directed-subscription-requester-" + UUID.randomUUID();
      boolean subjectConnected = false, requesterConnected = false, created = false;
      boolean subjectJoined = false, requesterJoined = false;
      boolean objectSubscribed = false, interactionSubscribed = false, directedSubscribed = false;
      InteractionClassHandle objectReportClass = null;
      InteractionClassHandle interactionReportClass = null;
      InteractionClassHandle directedReportClass = null;
      try {
         subject.connect(callbacks(), callbackModel);
         subjectConnected = true;
         requester.connect(recorder.proxy(), callbackModel);
         requesterConnected = true;
         subject.createFederationExecutionWithMIM(federation, new String[] {fom}, mim,
            timeImplementation);
         created = true;
         subject.joinFederationExecution(subjectName, federateType, federation);
         subjectJoined = true;
         FederateHandle targetFederate = requester.joinFederationExecution(
            requesterName, federateType, federation);
         requesterJoined = true;

         ObjectClassHandle subjectPrimaryObjectClass = subject.getObjectClassHandle(objectClassName);
         ObjectClassHandle subjectSecondObjectClass = subject.getObjectClassHandle(secondObjectClassName);
         ObjectClassHandle requesterPrimaryObjectClass =
            requester.getObjectClassHandle(objectClassName);
         ObjectClassHandle requesterSecondObjectClass =
            requester.getObjectClassHandle(secondObjectClassName);
         InteractionClassHandle subjectInteraction =
            subject.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle subjectSecondInteraction =
            subject.getInteractionClassHandle(secondDirectedInteractionClassName);
         InteractionClassHandle requesterInteraction =
            requester.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle requesterSecondInteraction =
            requester.getInteractionClassHandle(secondDirectedInteractionClassName);
         TransportationTypeHandle reliable = requester.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(subjectPrimaryObjectClass != null
               && subjectSecondObjectClass != null
               && requesterPrimaryObjectClass != null && requesterSecondObjectClass != null
               && subjectInteraction != null && subjectSecondInteraction != null
               && requesterInteraction != null && requesterSecondInteraction != null
               && targetFederate != null && reliable != null
               && !subjectPrimaryObjectClass.equals(subjectSecondObjectClass)
               && !subjectInteraction.equals(subjectSecondInteraction)
               && subjectInteraction.equals(requesterInteraction)
               && subjectSecondInteraction.equals(requesterSecondInteraction),
            "adapter-selected directed-subscription handles were invalid, duplicated, or inconsistent across members");
         JavaTckSupport.check(subject.getAttributeHandle(subjectPrimaryObjectClass, attributeName)
               != null && requester.getAttributeHandle(requesterPrimaryObjectClass, attributeName)
                  != null,
            "adapter-selected attribute lookup was unavailable on the primary object class");

         InteractionClassHandleSet primarySet = JavaTckSupport.interactionSet(
            subject, subjectInteraction);
         InteractionClassHandleSet secondSet = JavaTckSupport.interactionSet(
            subject, subjectSecondInteraction);
         subject.subscribeObjectClassDirectedInteractions(subjectPrimaryObjectClass,
            primarySet);
         subject.subscribeObjectClassDirectedInteractions(subjectSecondObjectClass,
            secondSet);

         InteractionClassHandle requestClass = requester.getInteractionClassHandle(REQUEST);
         ParameterHandle requestFederate = requester.getParameterHandle(requestClass, "HLAfederate");
         objectReportClass = requester.getInteractionClassHandle(OBJECT_REPORT);
         ParameterHandle objectReportCount = requester.getParameterHandle(
            objectReportClass, "HLAnumberOfClasses");
         ParameterHandle objectReportClassParameter = requester.getParameterHandle(
            objectReportClass, "HLAobjectClass");
         ParameterHandle objectReportActive = requester.getParameterHandle(
            objectReportClass, "HLAactive");
         ParameterHandle objectReportRate = requester.getParameterHandle(
            objectReportClass, "HLAmaxUpdateRate");
         ParameterHandle objectReportAttributes = requester.getParameterHandle(
            objectReportClass, "HLAattributeList");
         interactionReportClass = requester.getInteractionClassHandle(INTERACTION_REPORT);
         ParameterHandle interactionReportList = requester.getParameterHandle(
            interactionReportClass, "HLAinteractionClassList");
         directedReportClass = requester.getInteractionClassHandle(DIRECTED_REPORT);
         ParameterHandle directedReportCount = requester.getParameterHandle(
            directedReportClass, "HLAnumberOfClasses");
         ParameterHandle directedReportObjectClass = requester.getParameterHandle(
            directedReportClass, "HLAobjectClass");
         ParameterHandle directedReportList = requester.getParameterHandle(
            directedReportClass, "HLAinteractionClassList");
         JavaTckSupport.check(requestClass != null && requestFederate != null
               && objectReportClass != null && objectReportCount != null
               && objectReportClassParameter != null && objectReportActive != null
               && objectReportRate != null && objectReportAttributes != null
               && interactionReportClass != null && interactionReportList != null
               && directedReportClass != null && directedReportCount != null
               && directedReportObjectClass != null && directedReportList != null,
            "standard subscription request/report lookup returned a null handle");

         requester.subscribeInteractionClass(objectReportClass);
         objectSubscribed = true;
         requester.subscribeInteractionClass(interactionReportClass);
         interactionSubscribed = true;
         requester.subscribeInteractionClass(directedReportClass);
         directedSubscribed = true;

         ParameterHandleValueMap request = requester.getParameterHandleValueMapFactory().create(1);
         request.put(requestFederate, encode(targetFederate));
         requester.sendInteraction(requestClass, request, new byte[0]);
         await(requester, callbackModel, () -> recorder.interactions.size() >= 4,
            "two directed-subscription reports and ordinary NULL reports");
         JavaTckSupport.check(recorder.interactions.size() == 4,
            "initial subscription request did not deliver exactly four standard reports");

         List<Interaction> initialReports = new ArrayList<>(recorder.interactions);
         Interaction initialObjectReport = onlyReport(initialReports, objectReportClass,
            "initial object-subscription");
         Interaction initialInteractionReport = onlyReport(initialReports, interactionReportClass,
            "initial interaction-subscription");
         verifyMetadata(initialObjectReport, reliable, "initial object-subscription report");
         verifyMetadata(initialInteractionReport, reliable,
            "initial interaction-subscription report");
         verifyNullObjectReport(encoder, initialObjectReport, objectReportCount,
            objectReportClassParameter, objectReportActive, objectReportRate,
            objectReportAttributes, "initial object-subscription report");
         JavaTckSupport.check(initialInteractionReport.parameters.size() == 1
               && initialInteractionReport.parameters.containsKey(interactionReportList)
               && decodeSubscriptions(encoder, requester.getInteractionClassHandleFactory(),
                  initialInteractionReport.parameters.get(interactionReportList)).isEmpty(),
            "initial ordinary interaction-subscription report did not use its empty-list NULL shape");
         verifyDirectedRows(encoder, requester, initialReports, directedReportClass,
            directedReportCount, directedReportObjectClass, directedReportList,
            requesterPrimaryObjectClass, requesterSecondObjectClass,
            requesterInteraction, requesterSecondInteraction, reliable, 2);

         subject.unsubscribeObjectClassDirectedInteractions(subjectPrimaryObjectClass,
            JavaTckSupport.interactionSet(subject, subjectInteraction));
         int reportOffset = recorder.interactions.size();
         requester.sendInteraction(requestClass, request, new byte[0]);
         await(requester, callbackModel,
            () -> recorder.interactions.size() >= reportOffset + 3,
            "directed-subscription report after set-based partial unsubscribe");
         JavaTckSupport.check(recorder.interactions.size() == reportOffset + 3,
            "partial directed unsubscribe did not deliver exactly three standard reports");
         List<Interaction> remainingReports = new ArrayList<>(
            recorder.interactions.subList(reportOffset, reportOffset + 3));
         Interaction remainingObjectReport = onlyReport(remainingReports, objectReportClass,
            "post-unsubscribe object-subscription");
         Interaction remainingInteractionReport = onlyReport(remainingReports,
            interactionReportClass, "post-unsubscribe interaction-subscription");
         verifyMetadata(remainingObjectReport, reliable, "post-unsubscribe object report");
         verifyMetadata(remainingInteractionReport, reliable,
            "post-unsubscribe interaction report");
         verifyNullObjectReport(encoder, remainingObjectReport, objectReportCount,
            objectReportClassParameter, objectReportActive, objectReportRate,
            objectReportAttributes, "post-unsubscribe object-subscription report");
         JavaTckSupport.check(remainingInteractionReport.parameters.size() == 1
               && remainingInteractionReport.parameters.containsKey(interactionReportList)
               && decodeSubscriptions(encoder, requester.getInteractionClassHandleFactory(),
                  remainingInteractionReport.parameters.get(interactionReportList)).isEmpty(),
            "partial directed unsubscribe changed the ordinary interaction-subscription NULL report");
         verifyDirectedRows(encoder, requester, remainingReports, directedReportClass,
            directedReportCount, directedReportObjectClass, directedReportList,
            requesterPrimaryObjectClass, requesterSecondObjectClass,
            requesterInteraction, requesterSecondInteraction, reliable, 1);

         if (fullUnsubscribe) {
            subject.unsubscribeObjectClassDirectedInteractions(subjectSecondObjectClass);
            int fullReportOffset = recorder.interactions.size();
            requester.sendInteraction(requestClass, request, new byte[0]);
            await(requester, callbackModel,
               () -> recorder.interactions.size() >= fullReportOffset + 3,
               "directed-subscription NULL report after class-scoped full unsubscribe");
            JavaTckSupport.check(recorder.interactions.size() == fullReportOffset + 3,
               "full directed unsubscribe did not deliver exactly three standard reports");
            List<Interaction> fullReports = new ArrayList<>(
               recorder.interactions.subList(fullReportOffset, fullReportOffset + 3));
            Interaction fullObjectReport = onlyReport(fullReports, objectReportClass,
               "full-unsubscribe object-subscription");
            Interaction fullInteractionReport = onlyReport(fullReports, interactionReportClass,
               "full-unsubscribe interaction-subscription");
            Interaction fullDirectedReport = onlyReport(fullReports, directedReportClass,
               "full-unsubscribe directed-subscription");
            verifyMetadata(fullObjectReport, reliable, "full-unsubscribe object report");
            verifyMetadata(fullInteractionReport, reliable,
               "full-unsubscribe interaction report");
            verifyMetadata(fullDirectedReport, reliable,
               "full-unsubscribe directed report");
            verifyNullObjectReport(encoder, fullObjectReport, objectReportCount,
               objectReportClassParameter, objectReportActive, objectReportRate,
               objectReportAttributes, "full-unsubscribe object-subscription report");
            JavaTckSupport.check(fullInteractionReport.parameters.size() == 1
                  && fullInteractionReport.parameters.containsKey(interactionReportList)
                  && decodeSubscriptions(encoder, requester.getInteractionClassHandleFactory(),
                     fullInteractionReport.parameters.get(interactionReportList)).isEmpty(),
               "full directed unsubscribe changed the ordinary interaction-subscription NULL report");
            JavaTckSupport.check(fullDirectedReport.parameters.size() == 2
                  && fullDirectedReport.parameters.containsKey(directedReportCount)
                  && !fullDirectedReport.parameters.containsKey(directedReportObjectClass)
                  && fullDirectedReport.parameters.containsKey(directedReportList),
               "full directed-subscription report did not use the exact standard NULL parameter shape");
            HLAinteger32BE directedClassCount = encoder.createHLAinteger32BE();
            directedClassCount.decode(fullDirectedReport.parameters.get(directedReportCount));
            JavaTckSupport.check(directedClassCount.getValue() == 0
                  && decodeSubscriptions(encoder, requester.getInteractionClassHandleFactory(),
                     fullDirectedReport.parameters.get(directedReportList)).isEmpty(),
               "full directed-subscription NULL report did not encode zero classes and an empty interaction list");
         }
      } finally {
         if (objectSubscribed && requesterJoined && objectReportClass != null) try {
            requester.unsubscribeInteractionClass(objectReportClass);
         } catch (Exception ignored) { }
         if (interactionSubscribed && requesterJoined && interactionReportClass != null) try {
            requester.unsubscribeInteractionClass(interactionReportClass);
         } catch (Exception ignored) { }
         if (directedSubscribed && requesterJoined && directedReportClass != null) try {
            requester.unsubscribeInteractionClass(directedReportClass);
         } catch (Exception ignored) { }
         if (subjectJoined) try {
            subject.unsubscribeObjectClassDirectedInteractions(
               subject.getObjectClassHandle(objectClassName));
         } catch (Exception ignored) { }
         if (subjectJoined) try {
            subject.unsubscribeObjectClassDirectedInteractions(
               subject.getObjectClassHandle(secondObjectClassName));
         } catch (Exception ignored) { }
         if (requesterJoined) try { requester.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (subjectJoined) try { subject.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (created) try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (requesterConnected) try { requester.disconnect(); } catch (Exception ignored) { }
         if (subjectConnected) try { subject.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static void verifyNullObjectReport(EncoderFactory encoder, Interaction report,
         ParameterHandle countParameter, ParameterHandle objectClassParameter,
         ParameterHandle activeParameter, ParameterHandle rateParameter,
         ParameterHandle attributesParameter, String description) throws Exception {
      JavaTckSupport.check(report.parameters.size() == 1
            && report.parameters.containsKey(countParameter)
            && !report.parameters.containsKey(objectClassParameter)
            && !report.parameters.containsKey(activeParameter)
            && !report.parameters.containsKey(rateParameter)
            && !report.parameters.containsKey(attributesParameter),
         description + " did not use the standard zero-class NULL response shape");
      HLAinteger32BE count = encoder.createHLAinteger32BE();
      count.decode(report.parameters.get(countParameter));
      JavaTckSupport.check(count.getValue() == 0,
         description + " did not report zero object classes");
   }

   private static void verifyDirectedRows(EncoderFactory encoder, RTIambassador requester,
         List<Interaction> reports, InteractionClassHandle directedReportClass,
         ParameterHandle countParameter, ParameterHandle objectClassParameter,
         ParameterHandle listParameter, ObjectClassHandle primaryObjectClass,
         ObjectClassHandle secondObjectClass, InteractionClassHandle primaryInteraction,
         InteractionClassHandle secondInteraction, TransportationTypeHandle reliable,
         int expectedObjectClassCount)
         throws Exception {
      List<Interaction> directedReports = new ArrayList<>();
      for (Interaction report : reports) {
         if (directedReportClass.equals(report.interaction)) directedReports.add(report);
      }
      int expectedReports = expectedObjectClassCount == 2 ? 2 : 1;
      JavaTckSupport.check(directedReports.size() == expectedReports,
         "directed-subscription query returned the wrong number of class rows");
      ObjectClassHandleFactory objectFactory = requester.getObjectClassHandleFactory();
      InteractionClassHandleFactory interactionFactory =
         requester.getInteractionClassHandleFactory();
      boolean foundPrimary = false, foundSecond = false;
      for (Interaction report : directedReports) {
         verifyMetadata(report, reliable, "directed-subscription report");
         JavaTckSupport.check(report.parameters.size() == 3
               && report.parameters.containsKey(countParameter)
               && report.parameters.containsKey(objectClassParameter)
               && report.parameters.containsKey(listParameter),
            "directed-subscription report did not contain exactly its three MIM parameters");
         HLAinteger32BE classCount = encoder.createHLAinteger32BE();
         classCount.decode(report.parameters.get(countParameter));
         ObjectClassHandle reportedObjectClass = objectFactory.decode(
            report.parameters.get(objectClassParameter), 0);
         List<Subscription> subscriptions = decodeSubscriptions(encoder, interactionFactory,
            report.parameters.get(listParameter));
         JavaTckSupport.check(classCount.getValue() == expectedObjectClassCount
               && subscriptions.size() == 1 && !subscriptions.get(0).active,
            "directed-subscription report had the wrong class count, subscription count, or active state");
         if (reportedObjectClass.equals(primaryObjectClass)) {
            JavaTckSupport.check(expectedObjectClassCount == 2 && !foundPrimary
                  && subscriptions.get(0).interaction.equals(primaryInteraction),
               "primary directed-subscription report was duplicated or identified the wrong pair");
            foundPrimary = true;
         } else if (reportedObjectClass.equals(secondObjectClass)) {
            JavaTckSupport.check(!foundSecond
                  && subscriptions.get(0).interaction.equals(secondInteraction),
               "secondary directed-subscription report was duplicated or identified the wrong pair");
            foundSecond = true;
         } else {
            JavaTckSupport.check(false,
               "directed-subscription report identified an unselected object class");
         }
      }
      JavaTckSupport.check(expectedObjectClassCount == 2
            ? foundPrimary && foundSecond
            : !foundPrimary && foundSecond,
         "directed-subscription rows did not cover the exact expected object-class groups");
   }

   private static List<Subscription> decodeSubscriptions(EncoderFactory encoder,
         InteractionClassHandleFactory interactionFactory, byte[] encoded) throws Exception {
      HLAvariableArray<HLAbyte> handlePrototype = encoder.createHLAvariableArray(
         index -> encoder.createHLAbyte(), encoder.createHLAbyte());
      HLAfixedRecord recordPrototype = encoder.createHLAfixedRecord();
      recordPrototype.add(handlePrototype);
      recordPrototype.add(encoder.createHLAboolean());
      HLAvariableArray<HLAfixedRecord> records = encoder.createHLAvariableArray(
         index -> {
            HLAfixedRecord record = encoder.createHLAfixedRecord();
            record.add(encoder.createHLAvariableArray(
               inner -> encoder.createHLAbyte(), encoder.createHLAbyte()));
            record.add(encoder.createHLAboolean());
            return record;
         }, recordPrototype);
      records.decode(encoded);
      List<Subscription> result = new ArrayList<>();
      for (HLAfixedRecord record : records) {
         @SuppressWarnings("unchecked")
         HLAvariableArray<HLAbyte> handle = (HLAvariableArray<HLAbyte>) record.get(0);
         HLAboolean active = (HLAboolean) record.get(1);
         result.add(new Subscription(interactionFactory.decode(handle.toByteArray(), 0),
            active.getValue()));
      }
      return result;
   }

   private static Interaction onlyReport(List<Interaction> reports,
         InteractionClassHandle expected, String description) {
      Interaction found = null;
      for (Interaction report : reports) {
         if (expected.equals(report.interaction)) {
            JavaTckSupport.check(found == null, "duplicate " + description + " report");
            found = report;
         }
      }
      JavaTckSupport.check(found != null, "missing " + description + " report");
      return found;
   }

   private static void verifyMetadata(Interaction report,
         TransportationTypeHandle reliable, String description) {
      JavaTckSupport.check(report.present && reliable.equals(report.transportation)
            && report.tag.length == 0 && report.regionsEmpty,
         description + " was absent or had non-standard reliable-delivery metadata");
   }

   private static byte[] encode(FederateHandle handle) throws Exception {
      byte[] encoded = new byte[handle.encodedLength()];
      handle.encode(encoded, 0);
      return encoded;
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, args) -> objectMethod(proxy, method.getName(), args));
   }

   private static Object objectMethod(Object proxy, String name, Object[] args) {
      if ("toString".equals(name)) return "Java TCK directed subscription callbacks";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == (args == null ? null : args[0]);
      return null;
   }

   private static final class Subscription {
      private final InteractionClassHandle interaction;
      private final boolean active;
      private Subscription(InteractionClassHandle interaction, boolean active) {
         this.interaction = interaction;
         this.active = active;
      }
   }

   private static final class Interaction {
      private InteractionClassHandle interaction;
      private ParameterHandleValueMap parameters;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private boolean regionsEmpty;
      private boolean present;
   }

   private static final class Recorder {
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, args) -> {
               if ("receiveInteraction".equals(method.getName()) && args != null
                     && args.length == 6) {
                  Interaction event = new Interaction();
                  event.interaction = (InteractionClassHandle) args[0];
                  event.parameters = ((ParameterHandleValueMap) args[1]).clone();
                  event.tag = ((byte[]) args[2]).clone();
                  event.transportation = (TransportationTypeHandle) args[3];
                  RegionHandleSet regions = (RegionHandleSet) args[5];
                  event.regionsEmpty = regions == null || regions.isEmpty();
                  event.present = true;
                  interactions.add(event);
               }
               return objectMethod(proxy, method.getName(), args);
            });
      }
   }
}
