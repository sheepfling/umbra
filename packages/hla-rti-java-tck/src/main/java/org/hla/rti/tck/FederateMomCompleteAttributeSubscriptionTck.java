package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleSetFactory;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.InteractionClassHandleFactory;
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
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.encoding.HLAunicodeString;
import hla.rti1516_2025.encoding.HLAvariableArray;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies a complete adapter-configured attribute-set subscription via the standard MIM. */
final class FederateMomCompleteAttributeSubscriptionTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 15_000_000_000L;
   private static final String REQUEST =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestSubscriptions";
   private static final String OBJECT_REPORT =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectClassSubscription";
   private static final String INTERACTION_REPORT =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionSubscription";
   private static final String DIRECTED_REPORT =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportDirectedInteractionSubscription";

   private FederateMomCompleteAttributeSubscriptionTck() { }

   static void run(RtiFactory factory, String fom, String rateFom, String mim,
         String timeImplementation, String objectClassName, String firstAttributeName,
         String secondAttributeName, String thirdAttributeName, String updateRateName,
         CallbackModel callbackModel) throws Exception {
      run(factory, fom, rateFom, mim, timeImplementation, objectClassName,
         firstAttributeName, secondAttributeName, thirdAttributeName, updateRateName,
         callbackModel, true, false);
   }

   static void runPassively(RtiFactory factory, String fom, String rateFom, String mim,
         String timeImplementation, String objectClassName, String firstAttributeName,
         String secondAttributeName, String thirdAttributeName, String updateRateName,
         CallbackModel callbackModel) throws Exception {
      run(factory, fom, rateFom, mim, timeImplementation, objectClassName,
         firstAttributeName, secondAttributeName, thirdAttributeName, updateRateName,
         callbackModel, false, false);
   }

   static void runAfterActivation(RtiFactory factory, String fom, String rateFom, String mim,
         String timeImplementation, String objectClassName, String firstAttributeName,
         String secondAttributeName, String thirdAttributeName, String updateRateName,
         CallbackModel callbackModel) throws Exception {
      run(factory, fom, rateFom, mim, timeImplementation, objectClassName,
         firstAttributeName, secondAttributeName, thirdAttributeName, updateRateName,
         callbackModel, false, true);
   }

   private static void run(RtiFactory factory, String fom, String rateFom, String mim,
         String timeImplementation, String objectClassName, String firstAttributeName,
         String secondAttributeName, String thirdAttributeName, String updateRateName,
         CallbackModel callbackModel, boolean expectedActive, boolean activateAfterInitialQuery)
         throws Exception {
      JavaTckSupport.check(firstAttributeName != null && secondAttributeName != null
            && thirdAttributeName != null && !firstAttributeName.equals(secondAttributeName)
            && !firstAttributeName.equals(thirdAttributeName)
            && !secondAttributeName.equals(thirdAttributeName),
         "complete object-class subscription requires three distinct adapter attributes");

      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-federate-mom-complete-attribute-subscription-" + UUID.randomUUID();
      String subjectName = "java-tck-complete-attribute-subject-" + UUID.randomUUID();
      String requesterName = "java-tck-complete-attribute-requester-" + UUID.randomUUID();
      boolean subjectConnected = false, requesterConnected = false, created = false;
      boolean subjectJoined = false, requesterJoined = false, attributesSubscribed = false;
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
         subject.createFederationExecutionWithMIM(federation, new String[] {fom, rateFom}, mim,
            timeImplementation);
         created = true;
         subject.joinFederationExecution(subjectName, "java-tck", federation);
         subjectJoined = true;
         FederateHandle targetFederate = requester.joinFederationExecution(
            requesterName, "java-tck", federation);
         requesterJoined = true;

         ObjectClassHandle subjectObjectClass = subject.getObjectClassHandle(objectClassName);
         ObjectClassHandle requesterObjectClass = requester.getObjectClassHandle(objectClassName);
         AttributeHandle subjectFirst = subject.getAttributeHandle(subjectObjectClass,
            firstAttributeName);
         AttributeHandle subjectSecond = subject.getAttributeHandle(subjectObjectClass,
            secondAttributeName);
         AttributeHandle subjectThird = subject.getAttributeHandle(subjectObjectClass,
            thirdAttributeName);
         AttributeHandle requesterFirst = requester.getAttributeHandle(requesterObjectClass,
            firstAttributeName);
         AttributeHandle requesterSecond = requester.getAttributeHandle(requesterObjectClass,
            secondAttributeName);
         AttributeHandle requesterThird = requester.getAttributeHandle(requesterObjectClass,
            thirdAttributeName);
         TransportationTypeHandle reliable = requester.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(subjectObjectClass != null && requesterObjectClass != null
               && subjectFirst != null && subjectSecond != null && subjectThird != null
               && requesterFirst != null && requesterSecond != null && requesterThird != null
               && reliable != null,
            "adapter-selected object class or complete attribute-set handle lookup failed");

         AttributeHandleSetFactory setFactory = subject.getAttributeHandleSetFactory();
         AttributeHandleSet subscribedAttributes = setFactory.create();
         subscribedAttributes.add(subjectFirst);
         subscribedAttributes.add(subjectSecond);
         subscribedAttributes.add(subjectThird);
         if (expectedActive) {
            subject.subscribeObjectClassAttributes(subjectObjectClass, subscribedAttributes,
               updateRateName);
         } else {
            subject.subscribeObjectClassAttributesPassively(subjectObjectClass,
               subscribedAttributes, updateRateName);
         }
         attributesSubscribed = true;

         InteractionClassHandle requestClass = requester.getInteractionClassHandle(REQUEST);
         ParameterHandle requestFederate = requester.getParameterHandle(requestClass, "HLAfederate");
         objectReportClass = requester.getInteractionClassHandle(OBJECT_REPORT);
         ParameterHandle objectReportCount = requester.getParameterHandle(
            objectReportClass, "HLAnumberOfClasses");
         ParameterHandle objectReportObject = requester.getParameterHandle(
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
         ParameterHandle directedReportObject = requester.getParameterHandle(
            directedReportClass, "HLAobjectClass");
         ParameterHandle directedReportList = requester.getParameterHandle(
            directedReportClass, "HLAinteractionClassList");
         JavaTckSupport.check(requestClass != null && requestFederate != null
               && objectReportClass != null && objectReportCount != null
               && objectReportObject != null && objectReportActive != null
               && objectReportRate != null && objectReportAttributes != null
               && interactionReportClass != null && interactionReportList != null
               && directedReportClass != null && directedReportCount != null
               && directedReportObject != null && directedReportList != null,
            "standard subscription request/report lookup returned a null handle");

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
            "standard subscription reports for complete object-class attributes");
         JavaTckSupport.check(recorder.interactions.size() == 3,
            "complete attribute subscription query delivered an unexpected number of reports");

         Interaction objectReport = onlyReport(recorder.interactions, objectReportClass,
            "object-subscription");
         Interaction interactionReport = onlyReport(recorder.interactions, interactionReportClass,
            "interaction-subscription");
         Interaction directedReport = onlyReport(recorder.interactions, directedReportClass,
            "directed-subscription");
         verifyMetadata(objectReport, reliable, "object-subscription report");
         verifyMetadata(interactionReport, reliable, "interaction-subscription report");
         verifyMetadata(directedReport, reliable, "directed-subscription report");
         JavaTckSupport.check(objectReport.parameters.size() == 5
               && objectReport.parameters.containsKey(objectReportCount)
               && objectReport.parameters.containsKey(objectReportObject)
               && objectReport.parameters.containsKey(objectReportActive)
               && objectReport.parameters.containsKey(objectReportRate)
               && objectReport.parameters.containsKey(objectReportAttributes),
            "populated object-subscription report did not contain its five standard parameters");
         HLAinteger32BE count = encoder.createHLAinteger32BE();
         count.decode(objectReport.parameters.get(objectReportCount));
         HLAboolean active = encoder.createHLAboolean();
         active.decode(objectReport.parameters.get(objectReportActive));
         HLAunicodeString reportedRate = encoder.createHLAunicodeString();
         reportedRate.decode(objectReport.parameters.get(objectReportRate));
         ObjectClassHandleFactory objectFactory = requester.getObjectClassHandleFactory();
         ObjectClassHandle reportedClass = objectFactory.decode(
            objectReport.parameters.get(objectReportObject), 0);
         List<AttributeHandle> reportedAttributes = decodeHandles(encoder,
            bytes -> requester.getAttributeHandleFactory().decode(bytes, 0),
            objectReport.parameters.get(objectReportAttributes));
         Set<AttributeHandle> expectedAttributes = new HashSet<>();
         expectedAttributes.add(requesterFirst);
         expectedAttributes.add(requesterSecond);
         expectedAttributes.add(requesterThird);
         JavaTckSupport.check(count.getValue() == 1 && active.getValue() == expectedActive
               && requesterObjectClass.equals(reportedClass)
               && updateRateName.equals(reportedRate.getValue())
               && reportedAttributes.size() == 3
               && new HashSet<>(reportedAttributes).equals(expectedAttributes),
            "standard MIM did not report the expected state and exact adapter-configured attribute set");
         JavaTckSupport.check(interactionReport.parameters.size() == 1
               && interactionReport.parameters.containsKey(interactionReportList)
               && decodeHandles(encoder,
                  bytes -> requester.getInteractionClassHandleFactory().decode(bytes, 0),
                  interactionReport.parameters.get(interactionReportList)).isEmpty(),
            "empty interaction-subscription report did not use the standard NULL shape");
         JavaTckSupport.check(directedReport.parameters.size() == 2
               && directedReport.parameters.containsKey(directedReportCount)
               && !directedReport.parameters.containsKey(directedReportObject)
               && directedReport.parameters.containsKey(directedReportList),
            "empty directed-subscription report did not use the standard NULL parameter shape");
         HLAinteger32BE directedCount = encoder.createHLAinteger32BE();
         directedCount.decode(directedReport.parameters.get(directedReportCount));
         JavaTckSupport.check(directedCount.getValue() == 0
               && decodeHandles(encoder,
                  bytes -> requester.getInteractionClassHandleFactory().decode(bytes, 0),
                  directedReport.parameters.get(directedReportList)).isEmpty(),
            "empty directed-subscription report did not report zero classes and an empty list");

         if (activateAfterInitialQuery) {
            subject.subscribeObjectClassAttributes(subjectObjectClass, subscribedAttributes,
               updateRateName);
            int reportOffset = recorder.interactions.size();
            requester.sendInteraction(requestClass, request, new byte[0]);
            await(requester, callbackModel, () -> recorder.interactions.size() >= reportOffset + 3,
               "standard MIM reports after complete attribute-set activation");
            JavaTckSupport.check(recorder.interactions.size() == reportOffset + 3,
               "complete-set activation did not deliver exactly three fresh MIM reports");
            List<Interaction> activatedReports = new ArrayList<>(
               recorder.interactions.subList(reportOffset, reportOffset + 3));
            Interaction activatedObjectReport = onlyReport(activatedReports, objectReportClass,
               "activated object-subscription");
            Interaction activatedInteractionReport = onlyReport(activatedReports,
               interactionReportClass, "activated interaction-subscription");
            Interaction activatedDirectedReport = onlyReport(activatedReports,
               directedReportClass, "activated directed-subscription");
            verifyMetadata(activatedObjectReport, reliable, "activated object report");
            verifyMetadata(activatedInteractionReport, reliable,
               "activated interaction report");
            verifyMetadata(activatedDirectedReport, reliable, "activated directed report");
            JavaTckSupport.check(activatedObjectReport.parameters.size() == 5
                  && activatedObjectReport.parameters.containsKey(objectReportCount)
                  && activatedObjectReport.parameters.containsKey(objectReportObject)
                  && activatedObjectReport.parameters.containsKey(objectReportActive)
                  && activatedObjectReport.parameters.containsKey(objectReportRate)
                  && activatedObjectReport.parameters.containsKey(objectReportAttributes),
               "activated object-subscription report did not contain all five standard parameters");
            HLAinteger32BE activatedCount = encoder.createHLAinteger32BE();
            activatedCount.decode(activatedObjectReport.parameters.get(objectReportCount));
            HLAboolean activatedState = encoder.createHLAboolean();
            activatedState.decode(activatedObjectReport.parameters.get(objectReportActive));
            HLAunicodeString activatedRate = encoder.createHLAunicodeString();
            activatedRate.decode(activatedObjectReport.parameters.get(objectReportRate));
            ObjectClassHandle activatedClass = requester.getObjectClassHandleFactory().decode(
               activatedObjectReport.parameters.get(objectReportObject), 0);
            List<AttributeHandle> activatedAttributes = decodeHandles(encoder,
               bytes -> requester.getAttributeHandleFactory().decode(bytes, 0),
               activatedObjectReport.parameters.get(objectReportAttributes));
            Set<AttributeHandle> activatedExpectedAttributes = new HashSet<>();
            activatedExpectedAttributes.add(requesterFirst);
            activatedExpectedAttributes.add(requesterSecond);
            activatedExpectedAttributes.add(requesterThird);
            JavaTckSupport.check(activatedCount.getValue() == 1 && activatedState.getValue()
                  && requesterObjectClass.equals(activatedClass)
                  && updateRateName.equals(activatedRate.getValue())
                  && activatedAttributes.size() == 3
                  && new HashSet<>(activatedAttributes).equals(activatedExpectedAttributes),
               "MIM did not preserve the full attribute set and rate while activating the subscription");
         }
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
         if (attributesSubscribed && subjectJoined) try {
            ObjectClassHandle cls = subject.getObjectClassHandle(objectClassName);
            AttributeHandleSet set = subject.getAttributeHandleSetFactory().create();
            set.add(subject.getAttributeHandle(cls, firstAttributeName));
            set.add(subject.getAttributeHandle(cls, secondAttributeName));
            set.add(subject.getAttributeHandle(cls, thirdAttributeName));
            subject.unsubscribeObjectClassAttributes(cls, set);
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

   private static Interaction onlyReport(List<Interaction> reports,
         InteractionClassHandle expected, String phase) {
      Interaction found = null;
      for (Interaction report : reports) {
         if (expected.equals(report.interaction)) {
            JavaTckSupport.check(found == null, "duplicate " + phase + " report");
            found = report;
         }
      }
      JavaTckSupport.check(found != null, "missing " + phase + " report");
      return found;
   }

   private static void verifyMetadata(Interaction report,
         TransportationTypeHandle reliable, String phase) {
      JavaTckSupport.check(report.present && reliable.equals(report.transportation)
            && report.tag.length == 0 && report.regionsEmpty,
         phase + " was absent or had non-standard reliable-delivery metadata");
   }

   private static byte[] encode(FederateHandle handle) throws Exception {
      byte[] encoded = new byte[handle.encodedLength()];
      handle.encode(encoded, 0);
      return encoded;
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         BooleanSupplier condition, String phase) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + phase);
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, args) -> objectMethod(proxy, method.getName(), args));
   }

   private static Object objectMethod(Object proxy, String name, Object[] args) {
      if ("toString".equals(name)) return "Java TCK complete attribute subscription callbacks";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == (args == null ? null : args[0]);
      return null;
   }

   private interface HandleDecoder<H> { H decode(byte[] encoded) throws Exception; }

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
