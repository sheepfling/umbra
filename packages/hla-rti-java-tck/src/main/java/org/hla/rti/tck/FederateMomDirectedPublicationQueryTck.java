package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
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

/** Verifies standard-MIM directed-publication reports for two object classes. */
final class FederateMomDirectedPublicationQueryTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 15_000_000_000L;
   private static final String REQUEST =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestPublications";
   private static final String OBJECT_REPORT =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectClassPublication";
   private static final String INTERACTION_REPORT =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionPublication";
   private static final String DIRECTED_REPORT =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportDirectedInteractionPublication";

   private FederateMomDirectedPublicationQueryTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String secondObjectClassName, String attributeName,
         String interactionClassName, String secondDirectedInteractionClassName,
         CallbackModel callbackModel) throws Exception {
      run(factory, fom, mim, timeImplementation, objectClassName, secondObjectClassName,
         attributeName, interactionClassName, secondDirectedInteractionClassName,
         callbackModel, 0);
   }

   static void runAfterPartialUnpublication(RtiFactory factory, String fom, String mim,
         String timeImplementation, String objectClassName, String secondObjectClassName,
         String attributeName, String interactionClassName,
         String secondDirectedInteractionClassName, CallbackModel callbackModel) throws Exception {
      run(factory, fom, mim, timeImplementation, objectClassName, secondObjectClassName,
         attributeName, interactionClassName, secondDirectedInteractionClassName,
         callbackModel, 1);
   }

   static void runAfterFullUnpublication(RtiFactory factory, String fom, String mim,
         String timeImplementation, String objectClassName, String secondObjectClassName,
         String attributeName, String interactionClassName,
         String secondDirectedInteractionClassName, CallbackModel callbackModel) throws Exception {
      run(factory, fom, mim, timeImplementation, objectClassName, secondObjectClassName,
         attributeName, interactionClassName, secondDirectedInteractionClassName,
         callbackModel, 2);
   }

   private static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String secondObjectClassName, String attributeName,
         String interactionClassName, String secondDirectedInteractionClassName,
         CallbackModel callbackModel, int directedUnpublicationMode) throws Exception {
      JavaTckSupport.check(secondObjectClassName != null && !secondObjectClassName.isEmpty()
            && !objectClassName.equals(secondObjectClassName)
            && secondDirectedInteractionClassName != null
            && !secondDirectedInteractionClassName.isEmpty()
            && !interactionClassName.equals(secondDirectedInteractionClassName),
         "multiple directed publication query requires distinct adapter-supplied object and interaction class names");

      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-federate-mom-multiple-directed-publication-" + UUID.randomUUID();
      String subjectName = "java-tck-directed-publication-subject-" + UUID.randomUUID();
      String requesterName = "java-tck-directed-publication-requester-" + UUID.randomUUID();
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
         subject.joinFederationExecution(subjectName, "java-tck", federation);
         subjectJoined = true;
         FederateHandle targetFederate = requester.joinFederationExecution(
            requesterName, "java-tck", federation);
         requesterJoined = true;

         // The first class carries both the primary and class-specific directed
         // interactions; the second carries the primary interaction only.
         ObjectClassHandle subjectPrimaryObjectClass =
            subject.getObjectClassHandle(secondObjectClassName);
         ObjectClassHandle subjectSecondObjectClass = subject.getObjectClassHandle(objectClassName);
         ObjectClassHandle requesterPrimaryObjectClass =
            requester.getObjectClassHandle(secondObjectClassName);
         ObjectClassHandle requesterSecondObjectClass =
            requester.getObjectClassHandle(objectClassName);
         AttributeHandle subjectAttribute = subject.getAttributeHandle(
            subjectPrimaryObjectClass, attributeName);
         AttributeHandle requesterAttribute = requester.getAttributeHandle(
            requesterPrimaryObjectClass, attributeName);
         InteractionClassHandle subjectInteraction =
            subject.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle subjectSecondDirectedInteraction =
            subject.getInteractionClassHandle(secondDirectedInteractionClassName);
         InteractionClassHandle requesterInteraction =
            requester.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle requesterSecondDirectedInteraction =
            requester.getInteractionClassHandle(secondDirectedInteractionClassName);
         TransportationTypeHandle reliable = requester.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(subjectAttribute != null && requesterAttribute != null
               && subjectInteraction != null && subjectSecondDirectedInteraction != null
               && requesterInteraction != null && requesterSecondDirectedInteraction != null
               && targetFederate != null && reliable != null
               && !subjectPrimaryObjectClass.equals(subjectSecondObjectClass)
               && !subjectInteraction.equals(subjectSecondDirectedInteraction)
               && subjectInteraction.equals(requesterInteraction)
               && subjectSecondDirectedInteraction.equals(requesterSecondDirectedInteraction)
               && subjectAttribute.equals(requesterAttribute),
            "adapter-selected directed-publication handles were invalid, duplicated, or inconsistent across members");

         AttributeHandleSet attributes = JavaTckSupport.attributeSet(subject, subjectAttribute);
         subject.publishObjectClassAttributes(subjectPrimaryObjectClass, attributes);
         subject.publishInteractionClass(subjectInteraction);
         InteractionClassHandleSet primaryDirectedSet = JavaTckSupport.interactionSet(
            subject, subjectInteraction, subjectSecondDirectedInteraction);
         InteractionClassHandleSet secondDirectedSet = JavaTckSupport.interactionSet(
            subject, subjectInteraction);
         subject.publishObjectClassDirectedInteractions(subjectPrimaryObjectClass,
            primaryDirectedSet);
         subject.publishObjectClassDirectedInteractions(subjectSecondObjectClass,
            secondDirectedSet);

         InteractionClassHandle requestClass = requester.getInteractionClassHandle(REQUEST);
         ParameterHandle requestFederate = requester.getParameterHandle(requestClass, "HLAfederate");
         objectReportClass = requester.getInteractionClassHandle(OBJECT_REPORT);
         ParameterHandle objectReportCount = requester.getParameterHandle(
            objectReportClass, "HLAnumberOfClasses");
         ParameterHandle objectReportObjectClass = requester.getParameterHandle(
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
               && objectReportObjectClass != null && objectReportAttributes != null
               && interactionReportClass != null && interactionReportList != null
               && directedReportClass != null && directedReportCount != null
               && directedReportObjectClass != null && directedReportList != null,
            "standard publication request/report lookup returned a null handle");
         JavaTckSupport.check("HLAreliable".equals(requester.getTransportationTypeName(reliable)),
            "HLAreliable transportation handle did not round-trip by name");

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
            "the object, ordinary interaction, and two directed publication reports", recorder);
         JavaTckSupport.check(recorder.interactions.size() == 4,
            "one publication request did not deliver exactly four reports for the selected declarations");

         Interaction objectReport = onlyReport(recorder.interactions, objectReportClass,
            "object-class publication");
         Interaction interactionReport = onlyReport(recorder.interactions, interactionReportClass,
            "ordinary interaction publication");
         verifyMetadata(objectReport, reliable, "object-class publication report");
         verifyMetadata(interactionReport, reliable, "ordinary interaction publication report");
         JavaTckSupport.check(objectReport.parameters.size() == 3
               && objectReport.parameters.containsKey(objectReportCount)
               && objectReport.parameters.containsKey(objectReportObjectClass)
               && objectReport.parameters.containsKey(objectReportAttributes),
            "object publication report did not contain exactly its three standard parameters");
         HLAinteger32BE objectCount = encoder.createHLAinteger32BE();
         objectCount.decode(objectReport.parameters.get(objectReportCount));
         ObjectClassHandleFactory objectFactory = requester.getObjectClassHandleFactory();
         List<AttributeHandle> publishedAttributes = decodeHandles(encoder,
            bytes -> requester.getAttributeHandleFactory().decode(bytes, 0),
            objectReport.parameters.get(objectReportAttributes));
         JavaTckSupport.check(objectCount.getValue() == 1
               && requesterPrimaryObjectClass.equals(objectFactory.decode(
                  objectReport.parameters.get(objectReportObjectClass), 0))
               && publishedAttributes.contains(requesterAttribute),
            "object publication report did not identify the selected object class and attribute");

         JavaTckSupport.check(interactionReport.parameters.size() == 1
               && interactionReport.parameters.containsKey(interactionReportList),
            "ordinary interaction publication report did not contain its standard class list");
         List<InteractionClassHandle> ordinaryInteractions = decodeHandles(encoder,
            bytes -> requester.getInteractionClassHandleFactory().decode(bytes, 0),
            interactionReport.parameters.get(interactionReportList));
         JavaTckSupport.check(ordinaryInteractions.size() == 1
               && ordinaryInteractions.contains(requesterInteraction),
            "ordinary interaction report did not contain exactly the separately published class");

         List<Interaction> directedReports = new ArrayList<>();
         for (Interaction report : recorder.interactions) {
            if (directedReportClass.equals(report.interaction)) directedReports.add(report);
         }
         JavaTckSupport.check(directedReports.size() == 2,
            "standard MIM did not report both object classes with directed publications");
         boolean foundPrimaryClass = false, foundSecondClass = false;
         InteractionClassHandleFactory interactionFactory =
            requester.getInteractionClassHandleFactory();
         for (Interaction report : directedReports) {
            verifyMetadata(report, reliable, "directed-interaction publication report");
            JavaTckSupport.check(report.parameters.size() == 3
                  && report.parameters.containsKey(directedReportCount)
                  && report.parameters.containsKey(directedReportObjectClass)
                  && report.parameters.containsKey(directedReportList),
               "populated directed-publication report did not contain exactly its three MIM parameters");
            HLAinteger32BE classCount = encoder.createHLAinteger32BE();
            classCount.decode(report.parameters.get(directedReportCount));
            ObjectClassHandle reportedClass = objectFactory.decode(
               report.parameters.get(directedReportObjectClass), 0);
            List<InteractionClassHandle> directedInteractions = decodeHandles(encoder,
               bytes -> interactionFactory.decode(bytes, 0),
               report.parameters.get(directedReportList));
            JavaTckSupport.check(classCount.getValue() == 2
                  && (reportedClass.equals(requesterPrimaryObjectClass)
                     ? directedInteractions.size() == 2
                     : reportedClass.equals(requesterSecondObjectClass)
                        && directedInteractions.size() == 1),
               "directed-publication report had an unexpected class count or interaction-list size");
            if (reportedClass.equals(requesterPrimaryObjectClass)) {
               JavaTckSupport.check(!foundPrimaryClass
                     && directedInteractions.contains(requesterInteraction)
                     && directedInteractions.contains(requesterSecondDirectedInteraction),
                  "primary directed-publication report duplicated its object class or omitted a published interaction");
               foundPrimaryClass = true;
            } else if (reportedClass.equals(requesterSecondObjectClass)) {
               JavaTckSupport.check(!foundSecondClass
                     && directedInteractions.contains(requesterInteraction),
                  "second directed-publication report duplicated its object class or omitted the primary interaction");
               foundSecondClass = true;
            } else {
               JavaTckSupport.check(false,
                  "directed-publication report identified an unselected object class");
            }
         }
         JavaTckSupport.check(foundPrimaryClass && foundSecondClass,
            "directed-publication reports did not cover both adapter-selected object classes");

         if (directedUnpublicationMode >= 1) {
            subject.unpublishObjectClassDirectedInteractions(subjectPrimaryObjectClass,
               JavaTckSupport.interactionSet(subject, subjectInteraction));
            int reportOffset = recorder.interactions.size();
            requester.sendInteraction(requestClass, request, new byte[0]);
            await(requester, callbackModel,
               () -> recorder.interactions.size() >= reportOffset + 4,
               "publication reports after partial directed-interaction unpublication", recorder);
            JavaTckSupport.check(recorder.interactions.size() == reportOffset + 4,
               "partial-unpublication request did not deliver exactly four standard reports");
            List<Interaction> updatedReports = new ArrayList<>(
               recorder.interactions.subList(reportOffset, reportOffset + 4));
            Interaction updatedObjectReport = onlyReport(updatedReports, objectReportClass,
               "post-unpublication object-class publication");
            Interaction updatedInteractionReport = onlyReport(updatedReports,
               interactionReportClass, "post-unpublication ordinary interaction publication");
            List<Interaction> updatedDirectedReports = new ArrayList<>();
            for (Interaction report : updatedReports) {
               if (directedReportClass.equals(report.interaction)) {
                  updatedDirectedReports.add(report);
               }
            }
            JavaTckSupport.check(updatedDirectedReports.size() == 2,
               "post-unpublication query omitted one of the two remaining directed reports");
            verifyMetadata(updatedObjectReport, reliable,
               "post-unpublication object-class publication report");
            verifyMetadata(updatedInteractionReport, reliable,
               "post-unpublication ordinary interaction publication report");

            JavaTckSupport.check(updatedObjectReport.parameters.size() == 3
                  && updatedObjectReport.parameters.containsKey(objectReportCount)
                  && updatedObjectReport.parameters.containsKey(objectReportObjectClass)
                  && updatedObjectReport.parameters.containsKey(objectReportAttributes),
               "post-unpublication object publication report did not contain its three standard parameters");
            HLAinteger32BE updatedObjectCount = encoder.createHLAinteger32BE();
            updatedObjectCount.decode(updatedObjectReport.parameters.get(objectReportCount));
            List<AttributeHandle> updatedAttributes = decodeHandles(encoder,
               bytes -> requester.getAttributeHandleFactory().decode(bytes, 0),
               updatedObjectReport.parameters.get(objectReportAttributes));
            JavaTckSupport.check(updatedObjectCount.getValue() == 1
                  && requesterPrimaryObjectClass.equals(objectFactory.decode(
                     updatedObjectReport.parameters.get(objectReportObjectClass), 0))
                  && publishedAttributes.size() == updatedAttributes.size()
                  && new HashSet<>(publishedAttributes).equals(new HashSet<>(updatedAttributes)),
               "partial directed unpublication changed the independent ordinary object-class publication");

            JavaTckSupport.check(updatedInteractionReport.parameters.size() == 1
                  && updatedInteractionReport.parameters.containsKey(interactionReportList),
               "post-unpublication ordinary interaction report did not contain its standard class-list parameter");
            List<InteractionClassHandle> updatedOrdinaryInteractions = decodeHandles(encoder,
               bytes -> requester.getInteractionClassHandleFactory().decode(bytes, 0),
               updatedInteractionReport.parameters.get(interactionReportList));
            JavaTckSupport.check(updatedOrdinaryInteractions.size() == 1
                  && updatedOrdinaryInteractions.contains(requesterInteraction),
               "partial directed unpublication changed the independent ordinary interaction publication");

            boolean retainedDerivedPublication = false, retainedBasePublication = false;
            for (Interaction report : updatedDirectedReports) {
               verifyMetadata(report, reliable, "post-unpublication directed-publication report");
               JavaTckSupport.check(report.parameters.size() == 3
                     && report.parameters.containsKey(directedReportCount)
                     && report.parameters.containsKey(directedReportObjectClass)
                     && report.parameters.containsKey(directedReportList),
                  "remaining directed report did not contain its three standard parameters");
               HLAinteger32BE updatedClassCount = encoder.createHLAinteger32BE();
               updatedClassCount.decode(report.parameters.get(directedReportCount));
               ObjectClassHandle reportObjectClass = objectFactory.decode(
                  report.parameters.get(directedReportObjectClass), 0);
               List<InteractionClassHandle> remaining = decodeHandles(encoder,
                  bytes -> interactionFactory.decode(bytes, 0),
                  report.parameters.get(directedReportList));
               JavaTckSupport.check(updatedClassCount.getValue() == 2 && remaining.size() == 1,
                  "remaining directed report did not report two classes and exactly one interaction");
               if (reportObjectClass.equals(requesterPrimaryObjectClass)) {
                  JavaTckSupport.check(!retainedDerivedPublication
                        && remaining.contains(requesterSecondDirectedInteraction),
                     "derived-class report did not retain only its class-specific interaction");
                  retainedDerivedPublication = true;
               } else if (reportObjectClass.equals(requesterSecondObjectClass)) {
                  JavaTckSupport.check(!retainedBasePublication
                        && remaining.contains(requesterInteraction),
                     "base-class report did not retain its inherited interaction");
                  retainedBasePublication = true;
               } else {
                  JavaTckSupport.check(false,
                     "post-unpublication directed report identified an unselected object class");
               }
            }
            JavaTckSupport.check(retainedDerivedPublication && retainedBasePublication,
               "partial unpublication removed the class-specific derived or independent base publication");
         }

         if (directedUnpublicationMode == 2) {
            subject.unpublishObjectClassDirectedInteractions(subjectPrimaryObjectClass);
            subject.unpublishObjectClassDirectedInteractions(subjectSecondObjectClass);
            int reportOffset = recorder.interactions.size();
            requester.sendInteraction(requestClass, request, new byte[0]);
            await(requester, callbackModel,
               () -> recorder.interactions.size() >= reportOffset + 3,
               "publication reports after all directed interactions are unpublished", recorder);
            JavaTckSupport.check(recorder.interactions.size() == reportOffset + 3,
               "full directed-unpublication request did not deliver exactly three standard reports");
            List<Interaction> updatedReports = new ArrayList<>(
               recorder.interactions.subList(reportOffset, reportOffset + 3));
            Interaction updatedObjectReport = onlyReport(updatedReports, objectReportClass,
               "complete-unpublication object-class publication");
            Interaction updatedInteractionReport = onlyReport(updatedReports,
               interactionReportClass, "complete-unpublication ordinary interaction publication");
            Interaction directedNullReport = onlyReport(updatedReports, directedReportClass,
               "complete-unpublication directed NULL response");
            verifyMetadata(updatedObjectReport, reliable,
               "complete-unpublication object-class publication report");
            verifyMetadata(updatedInteractionReport, reliable,
               "complete-unpublication ordinary interaction publication report");
            verifyMetadata(directedNullReport, reliable,
               "complete-unpublication directed NULL response");

            JavaTckSupport.check(updatedObjectReport.parameters.size() == 3
                  && updatedObjectReport.parameters.containsKey(objectReportCount)
                  && updatedObjectReport.parameters.containsKey(objectReportObjectClass)
                  && updatedObjectReport.parameters.containsKey(objectReportAttributes),
               "complete directed unpublication changed the ordinary object report shape");
            HLAinteger32BE updatedObjectCount = encoder.createHLAinteger32BE();
            updatedObjectCount.decode(updatedObjectReport.parameters.get(objectReportCount));
            List<AttributeHandle> updatedAttributes = decodeHandles(encoder,
               bytes -> requester.getAttributeHandleFactory().decode(bytes, 0),
               updatedObjectReport.parameters.get(objectReportAttributes));
            JavaTckSupport.check(updatedObjectCount.getValue() == 1
                  && requesterPrimaryObjectClass.equals(objectFactory.decode(
                     updatedObjectReport.parameters.get(objectReportObjectClass), 0))
                  && publishedAttributes.size() == updatedAttributes.size()
                  && new HashSet<>(publishedAttributes).equals(new HashSet<>(updatedAttributes)),
               "complete directed unpublication changed the independent object-attribute publication");

            JavaTckSupport.check(updatedInteractionReport.parameters.size() == 1
                  && updatedInteractionReport.parameters.containsKey(interactionReportList),
               "complete-unpublication ordinary interaction report did not contain its standard class list");
            List<InteractionClassHandle> updatedOrdinaryInteractions = decodeHandles(encoder,
               bytes -> requester.getInteractionClassHandleFactory().decode(bytes, 0),
               updatedInteractionReport.parameters.get(interactionReportList));
            JavaTckSupport.check(updatedOrdinaryInteractions.size() == 1
                  && updatedOrdinaryInteractions.contains(requesterInteraction),
               "complete directed unpublication changed the ordinary interaction publication");

            JavaTckSupport.check(directedNullReport.parameters.size() == 2
                  && directedNullReport.parameters.containsKey(directedReportCount)
                  && !directedNullReport.parameters.containsKey(directedReportObjectClass)
                  && directedNullReport.parameters.containsKey(directedReportList),
               "complete-unpublication directed report did not use the standard MIM NULL parameter shape");
            HLAinteger32BE updatedDirectedCount = encoder.createHLAinteger32BE();
            updatedDirectedCount.decode(directedNullReport.parameters.get(directedReportCount));
            JavaTckSupport.check(updatedDirectedCount.getValue() == 0
                  && decodeHandles(encoder,
                     bytes -> requester.getInteractionClassHandleFactory().decode(bytes, 0),
                     directedNullReport.parameters.get(directedReportList)).isEmpty(),
               "complete-unpublication directed report did not encode zero classes and an empty list");
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
         BooleanSupplier condition, String description, Recorder recorder) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
         Thread.sleep(1L);
      }
      if (!condition.getAsBoolean()) {
         List<String> received = new ArrayList<>();
         for (Interaction event : recorder.interactions) {
            received.add(ambassador.getInteractionClassName(event.interaction));
         }
         throw new AssertionError("timed out waiting for " + description
            + "; received " + received.size() + " reports: " + received);
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         FederateMomDirectedPublicationQueryTck::objectMethod);
   }

   private static Object objectMethod(Object proxy, java.lang.reflect.Method method,
         Object[] args) {
      return objectMethod(proxy, method.getName(), args);
   }

   private static Object objectMethod(Object proxy, String name, Object[] args) {
      if ("toString".equals(name)) return "Java TCK directed publication callbacks";
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
