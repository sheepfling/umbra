package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
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
import hla.rti1516_2025.encoding.HLAbyte;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.encoding.HLAvariableArray;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks standard-MIM object, interaction, and empty directed publication reports. */
final class FederateMomPublicationQueryTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 15_000_000_000L;
   private static final String REQUEST =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestPublications";
   private static final String OBJECT_REPORT =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectClassPublication";
   private static final String INTERACTION_REPORT =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionPublication";
   private static final String DIRECTED_REPORT =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportDirectedInteractionPublication";

   private FederateMomPublicationQueryTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         CallbackModel callbackModel) throws Exception {
      run(factory, fom, mim, timeImplementation, objectClassName, attributeName,
         null, null, interactionClassName, null, callbackModel,
         false, false, false, false, false, false);
   }

   static void runCompleteObjectClass(RtiFactory factory, String fom, String mim,
         String timeImplementation, String objectClassName, String attributeName,
         String secondaryAttributeName, String thirdAttributeName, String interactionClassName,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(secondaryAttributeName != null && !secondaryAttributeName.isEmpty()
            && thirdAttributeName != null && !thirdAttributeName.isEmpty(),
         "complete publication requires adapter-supplied secondary and third attribute names");
      run(factory, fom, mim, timeImplementation, objectClassName, attributeName,
         secondaryAttributeName, thirdAttributeName, interactionClassName, null, callbackModel,
         false, false, false, false, false, false);
   }

   static void runAfterPartialAttributeUnpublication(RtiFactory factory, String fom, String mim,
         String timeImplementation, String objectClassName, String attributeName,
         String secondaryAttributeName, String thirdAttributeName, String interactionClassName,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(secondaryAttributeName != null && !secondaryAttributeName.isEmpty()
            && thirdAttributeName != null && !thirdAttributeName.isEmpty(),
         "partial unpublication requires adapter-supplied secondary and third attribute names");
      run(factory, fom, mim, timeImplementation, objectClassName, attributeName,
         secondaryAttributeName, thirdAttributeName, interactionClassName, null, callbackModel,
         true, false, false, false, false, false);
   }

   static void runAfterFullAttributeUnpublication(RtiFactory factory, String fom, String mim,
         String timeImplementation, String objectClassName, String attributeName,
         String secondaryAttributeName, String thirdAttributeName, String interactionClassName,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(secondaryAttributeName != null && !secondaryAttributeName.isEmpty()
            && thirdAttributeName != null && !thirdAttributeName.isEmpty(),
         "full unpublication requires adapter-supplied secondary and third attribute names");
      run(factory, fom, mim, timeImplementation, objectClassName, attributeName,
         secondaryAttributeName, thirdAttributeName, interactionClassName, null, callbackModel,
         false, true, false, false, false, false);
   }

   static void runAfterWholeObjectClassUnpublication(RtiFactory factory, String fom, String mim,
         String timeImplementation, String objectClassName, String attributeName,
         String interactionClassName, CallbackModel callbackModel) throws Exception {
      run(factory, fom, mim, timeImplementation, objectClassName, attributeName,
         null, null, interactionClassName, null, callbackModel,
         false, false, true, false, false, false);
   }

   static void runAfterWholeInteractionClassUnpublication(RtiFactory factory, String fom,
         String mim, String timeImplementation, String objectClassName, String attributeName,
         String interactionClassName, CallbackModel callbackModel) throws Exception {
      run(factory, fom, mim, timeImplementation, objectClassName, attributeName,
         null, null, interactionClassName, null, callbackModel,
         false, false, false, true, false, false);
   }

   static void runAfterPartialInteractionClassUnpublication(RtiFactory factory, String fom,
         String mim, String timeImplementation, String objectClassName, String attributeName,
         String interactionClassName, String secondaryInteractionClassName,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(secondaryInteractionClassName != null
            && !secondaryInteractionClassName.isEmpty()
            && !interactionClassName.equals(secondaryInteractionClassName),
         "partial interaction unpublication requires two distinct adapter-supplied interaction-class names");
      run(factory, fom, mim, timeImplementation, objectClassName, attributeName,
         null, null, interactionClassName, secondaryInteractionClassName, callbackModel,
         false, false, false, false, true, false);
   }

   static void runAfterFullInteractionClassUnpublication(RtiFactory factory, String fom,
         String mim, String timeImplementation, String objectClassName, String attributeName,
         String interactionClassName, String secondaryInteractionClassName,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(secondaryInteractionClassName != null
            && !secondaryInteractionClassName.isEmpty()
            && !interactionClassName.equals(secondaryInteractionClassName),
         "full interaction unpublication requires two distinct adapter-supplied interaction-class names");
      run(factory, fom, mim, timeImplementation, objectClassName, attributeName,
         null, null, interactionClassName, secondaryInteractionClassName, callbackModel,
         false, false, false, false, false, true);
   }

   private static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String attributeName, String secondaryAttributeName,
         String thirdAttributeName, String interactionClassName, String secondaryInteractionClassName,
         CallbackModel callbackModel,
         boolean unpublishSecondaryAfterQuery, boolean unpublishAllAfterQuery,
         boolean unpublishWholeClassAfterQuery, boolean unpublishWholeInteractionAfterQuery,
         boolean unpublishSecondaryInteractionAfterQuery,
         boolean unpublishAllInteractionClassesAfterQuery)
         throws Exception {
      boolean completeAttributeSet = secondaryAttributeName != null;
      String federateType = System.getProperty("hla.rti.tck.federateType", "tck-type");
      JavaTckSupport.check(!federateType.isEmpty(),
         "adapter-supplied federate type must not be empty");
      int unpublicationModes = (unpublishSecondaryAfterQuery ? 1 : 0)
            + (unpublishAllAfterQuery ? 1 : 0)
            + (unpublishWholeClassAfterQuery ? 1 : 0)
            + (unpublishWholeInteractionAfterQuery ? 1 : 0)
            + (unpublishSecondaryInteractionAfterQuery ? 1 : 0)
            + (unpublishAllInteractionClassesAfterQuery ? 1 : 0);
      JavaTckSupport.check(unpublicationModes <= 1
            && (!(unpublishSecondaryAfterQuery || unpublishAllAfterQuery) || completeAttributeSet)
            && ((unpublishSecondaryInteractionAfterQuery
               || unpublishAllInteractionClassesAfterQuery)
               == (secondaryInteractionClassName != null)),
         "unpublication requires one supported mode and a complete initial set when removing attributes");
      boolean completeInteractionSet = secondaryInteractionClassName != null;
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-federate-mom-publication-query-" + UUID.randomUUID();
      String subjectName = "java-tck-publication-subject-" + UUID.randomUUID();
      String requesterName = "java-tck-publication-requester-" + UUID.randomUUID();
      boolean subjectConnected = false, requesterConnected = false, created = false;
      boolean subjectJoined = false, requesterJoined = false;
      boolean objectReportSubscribed = false, interactionSubscribed = false;
      boolean directedSubscribed = false;
      InteractionClassHandle objectReportClass = null, interactionReportClass = null;
      InteractionClassHandle directedReportClass = null;
      ObjectClassHandle requesterObjectClass = null;
      AttributeHandleSet objectSubscription = null;
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
         FederateHandle requesterFederate = requester.joinFederationExecution(
            requesterName, federateType, federation);
         requesterJoined = true;

         ObjectClassHandle subjectObjectClass = subject.getObjectClassHandle(objectClassName);
         AttributeHandle subjectAttribute = subject.getAttributeHandle(
            subjectObjectClass, attributeName);
         AttributeHandle subjectSecondaryAttribute = completeAttributeSet
            ? subject.getAttributeHandle(subjectObjectClass, secondaryAttributeName) : null;
         AttributeHandle subjectThirdAttribute = completeAttributeSet
            ? subject.getAttributeHandle(subjectObjectClass, thirdAttributeName) : null;
         InteractionClassHandle subjectInteraction =
            subject.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle subjectSecondaryInteraction = completeInteractionSet
            ? subject.getInteractionClassHandle(secondaryInteractionClassName) : null;
         requesterObjectClass = requester.getObjectClassHandle(objectClassName);
         AttributeHandle requesterAttribute = requester.getAttributeHandle(
            requesterObjectClass, attributeName);
         AttributeHandle requesterSecondaryAttribute = completeAttributeSet
            ? requester.getAttributeHandle(requesterObjectClass, secondaryAttributeName) : null;
         AttributeHandle requesterThirdAttribute = completeAttributeSet
            ? requester.getAttributeHandle(requesterObjectClass, thirdAttributeName) : null;
         InteractionClassHandle requesterInteraction =
            requester.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle requesterSecondaryInteraction = completeInteractionSet
            ? requester.getInteractionClassHandle(secondaryInteractionClassName) : null;
         FederateHandle targetFederate = requester.getFederateHandle(subjectName);
         TransportationTypeHandle reliable = requester.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(subjectObjectClass != null && subjectAttribute != null
               && subjectInteraction != null && requesterObjectClass != null
               && requesterAttribute != null && requesterInteraction != null
               && (!completeInteractionSet || (subjectSecondaryInteraction != null
                  && requesterSecondaryInteraction != null))
               && targetFederate != null
               && reliable != null,
            "adapter publication handles or standard target/reliable handles were invalid");
         JavaTckSupport.check(!completeInteractionSet
               || (!subjectInteraction.equals(subjectSecondaryInteraction)
                  && requesterInteraction.equals(subjectInteraction)
                  && requesterSecondaryInteraction.equals(subjectSecondaryInteraction)
                  && interactionClassName.equals(
                     requester.getInteractionClassName(requesterInteraction))
                  && secondaryInteractionClassName.equals(
                     requester.getInteractionClassName(requesterSecondaryInteraction))),
            "adapter-selected ordinary interaction classes were invalid, duplicated, or inconsistent across members");
         JavaTckSupport.check(!completeAttributeSet
               || (subjectSecondaryAttribute != null && subjectThirdAttribute != null
                  && requesterSecondaryAttribute != null && requesterThirdAttribute != null
                  && !subjectAttribute.equals(subjectSecondaryAttribute)
                  && !subjectAttribute.equals(subjectThirdAttribute)
                  && !subjectSecondaryAttribute.equals(subjectThirdAttribute)
                  && subjectAttribute.equals(requesterAttribute)
                  && subjectSecondaryAttribute.equals(requesterSecondaryAttribute)
                  && subjectThirdAttribute.equals(requesterThirdAttribute)),
            "complete object-class publication attributes were invalid, duplicated, or inconsistent across members");
         JavaTckSupport.check(objectClassName.equals(requester.getObjectClassName(requesterObjectClass))
               && attributeName.equals(requester.getAttributeName(
                  requesterObjectClass, requesterAttribute))
               && interactionClassName.equals(
                  requester.getInteractionClassName(requesterInteraction))
               && "HLAreliable".equals(requester.getTransportationTypeName(reliable)),
            "adapter-selected publication names or HLAreliable did not round-trip");

         AttributeHandleSet publishedAttributeSet = completeAttributeSet
            ? JavaTckSupport.attributeSet(subject, subjectAttribute, subjectSecondaryAttribute,
               subjectThirdAttribute)
            : JavaTckSupport.attributeSet(subject, subjectAttribute);
         subject.publishObjectClassAttributes(subjectObjectClass, publishedAttributeSet);
         subject.publishInteractionClass(subjectInteraction);
         if (completeInteractionSet) {
            subject.publishInteractionClass(subjectSecondaryInteraction);
         }

         InteractionClassHandle requestClass = requester.getInteractionClassHandle(REQUEST);
         ParameterHandle requestFederate = requester.getParameterHandle(requestClass, "HLAfederate");
         objectReportClass = requester.getInteractionClassHandle(OBJECT_REPORT);
         ParameterHandle objectReportCount = requester.getParameterHandle(
            objectReportClass, "HLAnumberOfClasses");
         ParameterHandle objectReportClassParameter = requester.getParameterHandle(
            objectReportClass, "HLAobjectClass");
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
               && objectReportClassParameter != null && objectReportAttributes != null
               && interactionReportClass != null && interactionReportList != null
               && directedReportClass != null && directedReportCount != null
               && directedReportObjectClass != null && directedReportList != null,
            "standard publication request/report lookup returned a null handle");
         JavaTckSupport.check(REQUEST.equals(requester.getInteractionClassName(requestClass))
               && "HLAfederate".equals(requester.getParameterName(requestClass, requestFederate))
               && OBJECT_REPORT.equals(requester.getInteractionClassName(objectReportClass))
               && "HLAnumberOfClasses".equals(
                  requester.getParameterName(objectReportClass, objectReportCount))
               && "HLAobjectClass".equals(
                  requester.getParameterName(objectReportClass, objectReportClassParameter))
               && "HLAattributeList".equals(
                  requester.getParameterName(objectReportClass, objectReportAttributes))
               && INTERACTION_REPORT.equals(
                  requester.getInteractionClassName(interactionReportClass))
               && "HLAinteractionClassList".equals(
                  requester.getParameterName(interactionReportClass, interactionReportList))
               && DIRECTED_REPORT.equals(requester.getInteractionClassName(directedReportClass))
               && "HLAnumberOfClasses".equals(
                  requester.getParameterName(directedReportClass, directedReportCount))
               && "HLAobjectClass".equals(
                  requester.getParameterName(directedReportClass, directedReportObjectClass))
               && "HLAinteractionClassList".equals(
                  requester.getParameterName(directedReportClass, directedReportList)),
            "standard publication request/report parameter names did not round-trip");

         requester.subscribeInteractionClass(objectReportClass);
         objectReportSubscribed = true;
         requester.subscribeInteractionClass(interactionReportClass);
         interactionSubscribed = true;
         requester.subscribeInteractionClass(directedReportClass);
         directedSubscribed = true;

         ParameterHandleValueMap request = requester.getParameterHandleValueMapFactory().create(1);
         request.put(requestFederate, encode(targetFederate));
         requester.sendInteraction(requestClass, request, new byte[0]);
         await(requester, callbackModel, () -> recorder.interactions.size() >= 3,
            "the three standard federate MOM publication reports");
         JavaTckSupport.check(recorder.interactions.size() == 3,
            "one publication request did not deliver exactly three standard reports");

         Interaction objectReport = onlyReport(recorder, objectReportClass,
            "object-class publication");
         Interaction interactionReport = onlyReport(recorder, interactionReportClass,
            "interaction publication");
         Interaction directedReport = onlyReport(recorder, directedReportClass,
            "directed-interaction publication");
         verifyReportMetadata(objectReport, reliable, "object-class publication report");
         verifyReportMetadata(interactionReport, reliable, "interaction publication report");
         verifyReportMetadata(directedReport, reliable, "directed publication report");

         JavaTckSupport.check(objectReport.parameters.size() == 3
               && objectReport.parameters.containsKey(objectReportCount)
               && objectReport.parameters.containsKey(objectReportClassParameter)
               && objectReport.parameters.containsKey(objectReportAttributes),
            "object-class publication report did not contain exactly its three standard parameters");
         HLAinteger32BE objectCount = encoder.createHLAinteger32BE();
         objectCount.decode(objectReport.parameters.get(objectReportCount));
         ObjectClassHandleFactory objectClassFactory = requester.getObjectClassHandleFactory();
         JavaTckSupport.check(objectCount.getValue() == 1
               && requesterObjectClass.equals(objectClassFactory.decode(
                  objectReport.parameters.get(objectReportClassParameter), 0)),
            "object-class publication report identified a different publication set");
         List<AttributeHandle> attributes = decodeHandles(encoder,
            bytes -> requester.getAttributeHandleFactory().decode(bytes, 0),
            objectReport.parameters.get(objectReportAttributes));
         if (completeAttributeSet) {
            AttributeHandle privilegeToDelete = requester.getAttributeHandle(
               requesterObjectClass, "HLAprivilegeToDeleteObject");
            JavaTckSupport.check(privilegeToDelete != null
                  && !privilegeToDelete.equals(requesterAttribute)
                  && !privilegeToDelete.equals(requesterSecondaryAttribute)
                  && !privilegeToDelete.equals(requesterThirdAttribute)
                  && attributes.size() == 4
                  && attributes.contains(privilegeToDelete)
                  && attributes.contains(requesterAttribute)
                  && attributes.contains(requesterSecondaryAttribute)
                  && attributes.contains(requesterThirdAttribute),
               "complete object-class publication report omitted an application attribute or inherited HLAprivilegeToDeleteObject");
         } else {
            JavaTckSupport.check(attributes.contains(requesterAttribute),
               "object-class publication report omitted the selected published attribute");
         }

         JavaTckSupport.check(interactionReport.parameters.size() == 1
               && interactionReport.parameters.containsKey(interactionReportList),
            "interaction publication report did not contain its standard class list");
         List<InteractionClassHandle> interactions = decodeHandles(encoder,
            bytes -> requester.getInteractionClassHandleFactory().decode(bytes, 0),
            interactionReport.parameters.get(interactionReportList));
         JavaTckSupport.check(interactions.size() == (completeInteractionSet ? 2 : 1)
               && interactions.contains(requesterInteraction)
               && (!completeInteractionSet || interactions.contains(requesterSecondaryInteraction)),
            "interaction publication report did not contain exactly the selected ordinary interactions");

         JavaTckSupport.check(directedReport.parameters.size() == 2
               && directedReport.parameters.containsKey(directedReportCount)
               && !directedReport.parameters.containsKey(directedReportObjectClass)
               && directedReport.parameters.containsKey(directedReportList),
            "empty directed-publication report did not use the standard NULL response shape");
         HLAinteger32BE directedCount = encoder.createHLAinteger32BE();
         directedCount.decode(directedReport.parameters.get(directedReportCount));
         JavaTckSupport.check(directedCount.getValue() == 0
               && decodeHandles(encoder,
                  bytes -> requester.getInteractionClassHandleFactory().decode(bytes, 0),
                  directedReport.parameters.get(directedReportList)).isEmpty(),
            "empty directed-publication report did not return zero classes and an empty list");

         if (unpublishSecondaryAfterQuery) {
            subject.unpublishObjectClassAttributes(subjectObjectClass,
               JavaTckSupport.attributeSet(subject, subjectSecondaryAttribute));
            int reportOffset = recorder.interactions.size();
            requester.sendInteraction(requestClass, request, new byte[0]);
            await(requester, callbackModel,
               () -> recorder.interactions.size() >= reportOffset + 3,
               "publication reports after ordinary attribute unpublication");
            JavaTckSupport.check(recorder.interactions.size() == reportOffset + 3,
               "post-unpublication request did not deliver exactly three standard reports");
            List<Interaction> updatedReports = new ArrayList<>(
               recorder.interactions.subList(reportOffset, reportOffset + 3));
            Interaction updatedObjectReport = onlyReport(updatedReports, objectReportClass,
               "post-unpublication object-class publication");
            Interaction updatedInteractionReport = onlyReport(updatedReports,
               interactionReportClass, "post-unpublication interaction publication");
            Interaction updatedDirectedReport = onlyReport(updatedReports, directedReportClass,
               "post-unpublication directed-interaction publication");
            verifyReportMetadata(updatedObjectReport, reliable,
               "post-unpublication object-class publication report");
            verifyReportMetadata(updatedInteractionReport, reliable,
               "post-unpublication interaction publication report");
            verifyReportMetadata(updatedDirectedReport, reliable,
               "post-unpublication directed publication report");
            JavaTckSupport.check(updatedObjectReport.parameters.size() == 3
                  && updatedObjectReport.parameters.containsKey(objectReportCount)
                  && updatedObjectReport.parameters.containsKey(objectReportClassParameter)
                  && updatedObjectReport.parameters.containsKey(objectReportAttributes),
               "post-unpublication object report did not contain exactly its three standard parameters");
            HLAinteger32BE updatedObjectCount = encoder.createHLAinteger32BE();
            updatedObjectCount.decode(updatedObjectReport.parameters.get(objectReportCount));
            JavaTckSupport.check(updatedObjectCount.getValue() == 1
                  && requesterObjectClass.equals(objectClassFactory.decode(
                     updatedObjectReport.parameters.get(objectReportClassParameter), 0)),
               "post-unpublication object report identified a different class/count");
            List<AttributeHandle> remainingAttributes = decodeHandles(encoder,
               bytes -> requester.getAttributeHandleFactory().decode(bytes, 0),
               updatedObjectReport.parameters.get(objectReportAttributes));
            AttributeHandle privilegeToDelete = requester.getAttributeHandle(
               requesterObjectClass, "HLAprivilegeToDeleteObject");
            JavaTckSupport.check(privilegeToDelete != null && remainingAttributes.size() == 3
                  && remainingAttributes.contains(requesterAttribute)
                  && remainingAttributes.contains(requesterThirdAttribute)
                  && remainingAttributes.contains(privilegeToDelete)
                  && !remainingAttributes.contains(requesterSecondaryAttribute),
               "post-unpublication report did not contain exactly the remaining application attributes and inherited HLAprivilegeToDeleteObject");
         }

         if (unpublishAllAfterQuery || unpublishWholeClassAfterQuery) {
            if (unpublishWholeClassAfterQuery) {
               subject.unpublishObjectClass(subjectObjectClass);
            } else {
               subject.unpublishObjectClassAttributes(subjectObjectClass, publishedAttributeSet);
            }
            int reportOffset = recorder.interactions.size();
            requester.sendInteraction(requestClass, request, new byte[0]);
            await(requester, callbackModel,
               () -> recorder.interactions.size() >= reportOffset + 3,
               unpublishWholeClassAfterQuery
                  ? "publication reports after whole object-class unpublication"
                  : "publication reports after all application attributes are unpublished");
            JavaTckSupport.check(recorder.interactions.size() == reportOffset + 3,
               "post-unpublication request did not deliver exactly three standard reports");
            List<Interaction> updatedReports = new ArrayList<>(
               recorder.interactions.subList(reportOffset, reportOffset + 3));
            Interaction updatedObjectReport = onlyReport(updatedReports, objectReportClass,
               "post-unpublication object-class publication");
            Interaction updatedInteractionReport = onlyReport(updatedReports,
               interactionReportClass, "post-unpublication interaction publication");
            Interaction updatedDirectedReport = onlyReport(updatedReports, directedReportClass,
               "post-unpublication directed-interaction publication");
            verifyReportMetadata(updatedObjectReport, reliable,
               "post-unpublication object-class publication report");
            verifyReportMetadata(updatedInteractionReport, reliable,
               "post-unpublication interaction publication report");
            verifyReportMetadata(updatedDirectedReport, reliable,
               "post-unpublication directed publication report");
            JavaTckSupport.check(updatedObjectReport.parameters.size() == 1
                  && updatedObjectReport.parameters.containsKey(objectReportCount)
                  && !updatedObjectReport.parameters.containsKey(objectReportClassParameter)
                  && !updatedObjectReport.parameters.containsKey(objectReportAttributes),
               "post-unpublication object report did not use the standard NULL response parameter shape");
            HLAinteger32BE updatedObjectCount = encoder.createHLAinteger32BE();
            updatedObjectCount.decode(updatedObjectReport.parameters.get(objectReportCount));
            JavaTckSupport.check(updatedObjectCount.getValue() == 0,
               "post-unpublication MIM NULL response did not report zero published object classes");
         }

         if (unpublishWholeInteractionAfterQuery) {
            subject.unpublishInteractionClass(subjectInteraction);
            int reportOffset = recorder.interactions.size();
            requester.sendInteraction(requestClass, request, new byte[0]);
            await(requester, callbackModel,
               () -> recorder.interactions.size() >= reportOffset + 3,
               "publication reports after whole interaction-class unpublication");
            JavaTckSupport.check(recorder.interactions.size() == reportOffset + 3,
               "post-unpublication request did not deliver exactly three standard reports");
            List<Interaction> updatedReports = new ArrayList<>(
               recorder.interactions.subList(reportOffset, reportOffset + 3));
            Interaction updatedObjectReport = onlyReport(updatedReports, objectReportClass,
               "post-interaction-unpublication object-class publication");
            Interaction updatedInteractionReport = onlyReport(updatedReports,
               interactionReportClass, "post-interaction-unpublication publication");
            Interaction updatedDirectedReport = onlyReport(updatedReports, directedReportClass,
               "post-interaction-unpublication directed-interaction publication");
            verifyReportMetadata(updatedObjectReport, reliable,
               "post-interaction-unpublication object-class publication report");
            verifyReportMetadata(updatedInteractionReport, reliable,
               "post-interaction-unpublication interaction publication report");
            verifyReportMetadata(updatedDirectedReport, reliable,
               "post-interaction-unpublication directed publication report");

            JavaTckSupport.check(updatedObjectReport.parameters.size() == 3
                  && updatedObjectReport.parameters.containsKey(objectReportCount)
                  && updatedObjectReport.parameters.containsKey(objectReportClassParameter)
                  && updatedObjectReport.parameters.containsKey(objectReportAttributes),
               "ordinary interaction unpublication changed the standard object publication report shape");
            HLAinteger32BE updatedObjectCount = encoder.createHLAinteger32BE();
            updatedObjectCount.decode(updatedObjectReport.parameters.get(objectReportCount));
            List<AttributeHandle> updatedAttributes = decodeHandles(encoder,
               bytes -> requester.getAttributeHandleFactory().decode(bytes, 0),
               updatedObjectReport.parameters.get(objectReportAttributes));
            JavaTckSupport.check(updatedObjectCount.getValue() == 1
                  && requesterObjectClass.equals(objectClassFactory.decode(
                     updatedObjectReport.parameters.get(objectReportClassParameter), 0))
                  && attributes.size() == updatedAttributes.size()
                  && new HashSet<>(attributes).equals(new HashSet<>(updatedAttributes)),
               "ordinary interaction unpublication changed the independent object-class publication");

            JavaTckSupport.check(updatedInteractionReport.parameters.size() == 1
                  && updatedInteractionReport.parameters.containsKey(interactionReportList)
                  && decodeHandles(encoder,
                     bytes -> requester.getInteractionClassHandleFactory().decode(bytes, 0),
                     updatedInteractionReport.parameters.get(interactionReportList)).isEmpty(),
               "whole interaction-class unpublication did not produce the exact MIM NULL report with an empty class list");

            JavaTckSupport.check(updatedDirectedReport.parameters.size() == 2
                  && updatedDirectedReport.parameters.containsKey(directedReportCount)
                  && !updatedDirectedReport.parameters.containsKey(directedReportObjectClass)
                  && updatedDirectedReport.parameters.containsKey(directedReportList),
               "unchanged NULL directed-publication report did not use the standard MIM parameter shape");
            HLAinteger32BE updatedDirectedCount = encoder.createHLAinteger32BE();
            updatedDirectedCount.decode(updatedDirectedReport.parameters.get(directedReportCount));
            JavaTckSupport.check(updatedDirectedCount.getValue() == 0
                  && decodeHandles(encoder,
                     bytes -> requester.getInteractionClassHandleFactory().decode(bytes, 0),
                     updatedDirectedReport.parameters.get(directedReportList)).isEmpty(),
               "ordinary interaction unpublication changed the independent NULL directed-publication report");
         }

         if (unpublishSecondaryInteractionAfterQuery) {
            subject.unpublishInteractionClass(subjectSecondaryInteraction);
            int reportOffset = recorder.interactions.size();
            requester.sendInteraction(requestClass, request, new byte[0]);
            await(requester, callbackModel,
               () -> recorder.interactions.size() >= reportOffset + 3,
               "publication reports after partial interaction-class unpublication");
            JavaTckSupport.check(recorder.interactions.size() == reportOffset + 3,
               "partial-unpublication request did not deliver exactly three standard reports");
            List<Interaction> updatedReports = new ArrayList<>(
               recorder.interactions.subList(reportOffset, reportOffset + 3));
            Interaction updatedObjectReport = onlyReport(updatedReports, objectReportClass,
               "partial-interaction-unpublication object-class publication");
            Interaction updatedInteractionReport = onlyReport(updatedReports,
               interactionReportClass, "partial-interaction-unpublication publication");
            Interaction updatedDirectedReport = onlyReport(updatedReports, directedReportClass,
               "partial-interaction-unpublication directed-interaction publication");
            verifyReportMetadata(updatedObjectReport, reliable,
               "partial-interaction-unpublication object-class publication report");
            verifyReportMetadata(updatedInteractionReport, reliable,
               "partial-interaction-unpublication interaction publication report");
            verifyReportMetadata(updatedDirectedReport, reliable,
               "partial-interaction-unpublication directed publication report");

            JavaTckSupport.check(updatedInteractionReport.parameters.size() == 1
                  && updatedInteractionReport.parameters.containsKey(interactionReportList),
               "partial-unpublication interaction report did not contain exactly its standard class-list parameter");
            List<InteractionClassHandle> remainingInteractions = decodeHandles(encoder,
               bytes -> requester.getInteractionClassHandleFactory().decode(bytes, 0),
               updatedInteractionReport.parameters.get(interactionReportList));
            JavaTckSupport.check(remainingInteractions.size() == 1
                  && remainingInteractions.contains(requesterInteraction)
                  && !remainingInteractions.contains(requesterSecondaryInteraction),
               "partial interaction unpublication did not report exactly the remaining primary class");

            JavaTckSupport.check(updatedObjectReport.parameters.size() == 3
                  && updatedObjectReport.parameters.containsKey(objectReportCount)
                  && updatedObjectReport.parameters.containsKey(objectReportClassParameter)
                  && updatedObjectReport.parameters.containsKey(objectReportAttributes),
               "partial interaction unpublication changed the independent object report shape");
            HLAinteger32BE updatedObjectCount = encoder.createHLAinteger32BE();
            updatedObjectCount.decode(updatedObjectReport.parameters.get(objectReportCount));
            List<AttributeHandle> updatedAttributes = decodeHandles(encoder,
               bytes -> requester.getAttributeHandleFactory().decode(bytes, 0),
               updatedObjectReport.parameters.get(objectReportAttributes));
            JavaTckSupport.check(updatedObjectCount.getValue() == 1
                  && requesterObjectClass.equals(objectClassFactory.decode(
                     updatedObjectReport.parameters.get(objectReportClassParameter), 0))
                  && attributes.size() == updatedAttributes.size()
                  && new HashSet<>(attributes).equals(new HashSet<>(updatedAttributes)),
               "partial interaction unpublication changed the independent object-class publication");

            JavaTckSupport.check(updatedDirectedReport.parameters.size() == 2
                  && updatedDirectedReport.parameters.containsKey(directedReportCount)
                  && !updatedDirectedReport.parameters.containsKey(directedReportObjectClass)
                  && updatedDirectedReport.parameters.containsKey(directedReportList),
               "unchanged NULL directed-publication report did not use the standard MIM parameter shape");
            HLAinteger32BE updatedDirectedCount = encoder.createHLAinteger32BE();
            updatedDirectedCount.decode(updatedDirectedReport.parameters.get(directedReportCount));
            JavaTckSupport.check(updatedDirectedCount.getValue() == 0
                  && decodeHandles(encoder,
                     bytes -> requester.getInteractionClassHandleFactory().decode(bytes, 0),
                     updatedDirectedReport.parameters.get(directedReportList)).isEmpty(),
               "partial interaction unpublication changed the independent NULL directed-publication report");
         }

         if (unpublishAllInteractionClassesAfterQuery) {
            subject.unpublishInteractionClass(subjectSecondaryInteraction);
            subject.unpublishInteractionClass(subjectInteraction);
            int reportOffset = recorder.interactions.size();
            requester.sendInteraction(requestClass, request, new byte[0]);
            await(requester, callbackModel,
               () -> recorder.interactions.size() >= reportOffset + 3,
               "publication reports after both ordinary interaction classes are unpublished");
            JavaTckSupport.check(recorder.interactions.size() == reportOffset + 3,
               "full interaction-unpublication request did not deliver exactly three standard reports");
            List<Interaction> updatedReports = new ArrayList<>(
               recorder.interactions.subList(reportOffset, reportOffset + 3));
            Interaction updatedObjectReport = onlyReport(updatedReports, objectReportClass,
               "full-interaction-unpublication object-class publication");
            Interaction updatedInteractionReport = onlyReport(updatedReports,
               interactionReportClass, "full-interaction-unpublication publication");
            Interaction updatedDirectedReport = onlyReport(updatedReports, directedReportClass,
               "full-interaction-unpublication directed-interaction publication");
            verifyReportMetadata(updatedObjectReport, reliable,
               "full-interaction-unpublication object-class publication report");
            verifyReportMetadata(updatedInteractionReport, reliable,
               "full-interaction-unpublication interaction publication report");
            verifyReportMetadata(updatedDirectedReport, reliable,
               "full-interaction-unpublication directed publication report");

            JavaTckSupport.check(updatedInteractionReport.parameters.size() == 1
                  && updatedInteractionReport.parameters.containsKey(interactionReportList)
                  && decodeHandles(encoder,
                     bytes -> requester.getInteractionClassHandleFactory().decode(bytes, 0),
                     updatedInteractionReport.parameters.get(interactionReportList)).isEmpty(),
               "full interaction unpublication did not produce the exact MIM NULL report with an empty class list");

            JavaTckSupport.check(updatedObjectReport.parameters.size() == 3
                  && updatedObjectReport.parameters.containsKey(objectReportCount)
                  && updatedObjectReport.parameters.containsKey(objectReportClassParameter)
                  && updatedObjectReport.parameters.containsKey(objectReportAttributes),
               "full interaction unpublication changed the independent object report shape");
            HLAinteger32BE updatedObjectCount = encoder.createHLAinteger32BE();
            updatedObjectCount.decode(updatedObjectReport.parameters.get(objectReportCount));
            List<AttributeHandle> updatedAttributes = decodeHandles(encoder,
               bytes -> requester.getAttributeHandleFactory().decode(bytes, 0),
               updatedObjectReport.parameters.get(objectReportAttributes));
            JavaTckSupport.check(updatedObjectCount.getValue() == 1
                  && requesterObjectClass.equals(objectClassFactory.decode(
                     updatedObjectReport.parameters.get(objectReportClassParameter), 0))
                  && attributes.size() == updatedAttributes.size()
                  && new HashSet<>(attributes).equals(new HashSet<>(updatedAttributes)),
               "full interaction unpublication changed the independent object-class publication");

            JavaTckSupport.check(updatedDirectedReport.parameters.size() == 2
                  && updatedDirectedReport.parameters.containsKey(directedReportCount)
                  && !updatedDirectedReport.parameters.containsKey(directedReportObjectClass)
                  && updatedDirectedReport.parameters.containsKey(directedReportList),
               "unchanged NULL directed-publication report did not use the standard MIM parameter shape");
            HLAinteger32BE updatedDirectedCount = encoder.createHLAinteger32BE();
            updatedDirectedCount.decode(updatedDirectedReport.parameters.get(directedReportCount));
            JavaTckSupport.check(updatedDirectedCount.getValue() == 0
                  && decodeHandles(encoder,
                     bytes -> requester.getInteractionClassHandleFactory().decode(bytes, 0),
                     updatedDirectedReport.parameters.get(directedReportList)).isEmpty(),
               "full interaction unpublication changed the independent NULL directed-publication report");
         }
      } finally {
         if (objectReportSubscribed && requesterJoined && objectReportClass != null) try {
            requester.unsubscribeInteractionClass(objectReportClass);
         } catch (Exception ignored) { }
         if (directedSubscribed && requesterJoined && directedReportClass != null) try {
            requester.unsubscribeInteractionClass(directedReportClass);
         } catch (Exception ignored) { }
         if (interactionSubscribed && requesterJoined && interactionReportClass != null) try {
            requester.unsubscribeInteractionClass(interactionReportClass);
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

   private static Interaction onlyReport(Recorder recorder, InteractionClassHandle expected,
         String description) {
      return onlyReport(recorder.interactions, expected, description);
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

   private static void verifyReportMetadata(Interaction report,
         TransportationTypeHandle reliable, String description) {
      JavaTckSupport.check(report.present && reliable.equals(report.transportation)
            && report.tag.length == 0 && report.regionsEmpty,
         description + " was absent or had non-standard reliable-delivery metadata");
   }

   private static <H> List<H> decodeHandles(EncoderFactory encoder,
         HandleDecoder<H> decoder, byte[] encoded) throws Exception {
      HLAvariableArray<HLAvariableArray<HLAbyte>> references = encoder.createHLAvariableArray(
         outer -> encoder.createHLAvariableArray(inner -> encoder.createHLAbyte()));
      references.decode(encoded);
      List<H> result = new ArrayList<>();
      for (HLAvariableArray<HLAbyte> reference : references) {
         result.add(decoder.decode(reference.toByteArray()));
      }
      return result;
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
      if ("toString".equals(name)) return "Java TCK publication-query callbacks";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == (args == null ? null : args[0]);
      return null;
   }

   @FunctionalInterface
   private interface HandleDecoder<H> {
      H decode(byte[] encoded) throws Exception;
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
