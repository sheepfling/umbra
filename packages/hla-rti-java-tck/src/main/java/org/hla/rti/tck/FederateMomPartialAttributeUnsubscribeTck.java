package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleSetFactory;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
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
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies attribute-set and whole-class unsubscription through the standard MIM. */
final class FederateMomPartialAttributeUnsubscribeTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 15_000_000_000L;
   private static final String REQUEST =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestSubscriptions";
   private static final String OBJECT_REPORT =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectClassSubscription";
   private static final String INTERACTION_REPORT =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionSubscription";
   private static final String DIRECTED_REPORT =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportDirectedInteractionSubscription";

   private FederateMomPartialAttributeUnsubscribeTck() { }

   static void run(RtiFactory factory, String fom, String rateFom, String mim,
         String timeImplementation, String objectClassName, String firstAttributeName,
         String secondAttributeName, String updateRateName, CallbackModel callbackModel)
         throws Exception {
      run(factory, fom, rateFom, mim, timeImplementation, objectClassName, firstAttributeName,
         secondAttributeName, updateRateName, callbackModel, false, false);
   }

   static void runFullUnsubscribe(RtiFactory factory, String fom, String rateFom, String mim,
         String timeImplementation, String objectClassName, String firstAttributeName,
         String secondAttributeName, String updateRateName, CallbackModel callbackModel)
         throws Exception {
      run(factory, fom, rateFom, mim, timeImplementation, objectClassName, firstAttributeName,
         secondAttributeName, updateRateName, callbackModel, true, false);
   }

   static void runWholeClassUnsubscribe(RtiFactory factory, String fom, String rateFom, String mim,
         String timeImplementation, String objectClassName, String firstAttributeName,
         String secondAttributeName, String updateRateName, CallbackModel callbackModel)
         throws Exception {
      run(factory, fom, rateFom, mim, timeImplementation, objectClassName, firstAttributeName,
         secondAttributeName, updateRateName, callbackModel, false, true);
   }

   private static void run(RtiFactory factory, String fom, String rateFom, String mim,
         String timeImplementation, String objectClassName, String firstAttributeName,
         String secondAttributeName, String updateRateName, CallbackModel callbackModel,
         boolean fullAttributeUnsubscribe, boolean wholeClassUnsubscribe) throws Exception {
      JavaTckSupport.check(firstAttributeName != null && secondAttributeName != null
            && !firstAttributeName.equals(secondAttributeName),
         "partial attribute unsubscription requires two distinct adapter attributes");
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-federate-mom-partial-attribute-unsubscribe-" + UUID.randomUUID();
      String subjectName = "java-tck-partial-attribute-subject-" + UUID.randomUUID();
      String requesterName = "java-tck-partial-attribute-requester-" + UUID.randomUUID();
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

         ObjectClassHandle subjectClass = subject.getObjectClassHandle(objectClassName);
         ObjectClassHandle requesterClass = requester.getObjectClassHandle(objectClassName);
         AttributeHandle subjectFirst = subject.getAttributeHandle(subjectClass, firstAttributeName);
         AttributeHandle subjectSecond = subject.getAttributeHandle(subjectClass, secondAttributeName);
         AttributeHandle requesterFirst = requester.getAttributeHandle(requesterClass, firstAttributeName);
         AttributeHandle requesterSecond = requester.getAttributeHandle(requesterClass, secondAttributeName);
         TransportationTypeHandle reliable = requester.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(subjectClass != null && requesterClass != null
               && subjectFirst != null && subjectSecond != null
               && requesterFirst != null && requesterSecond != null && reliable != null,
            "adapter-selected object class or attribute handle lookup failed");

         AttributeHandleSetFactory setFactory = subject.getAttributeHandleSetFactory();
         AttributeHandleSet bothAttributes = setFactory.create();
         bothAttributes.add(subjectFirst);
         bothAttributes.add(subjectSecond);
         subject.subscribeObjectClassAttributes(subjectClass, bothAttributes, updateRateName);
         attributesSubscribed = true;

         InteractionClassHandle requestClass = requester.getInteractionClassHandle(REQUEST);
         ParameterHandle requestFederate = requester.getParameterHandle(requestClass, "HLAfederate");
         objectReportClass = requester.getInteractionClassHandle(OBJECT_REPORT);
         ParameterHandle objectCount = requester.getParameterHandle(
            objectReportClass, "HLAnumberOfClasses");
         ParameterHandle objectParameter = requester.getParameterHandle(
            objectReportClass, "HLAobjectClass");
         ParameterHandle activeParameter = requester.getParameterHandle(objectReportClass, "HLAactive");
         ParameterHandle rateParameter = requester.getParameterHandle(
            objectReportClass, "HLAmaxUpdateRate");
         ParameterHandle attributesParameter = requester.getParameterHandle(
            objectReportClass, "HLAattributeList");
         interactionReportClass = requester.getInteractionClassHandle(INTERACTION_REPORT);
         ParameterHandle interactionList = requester.getParameterHandle(
            interactionReportClass, "HLAinteractionClassList");
         directedReportClass = requester.getInteractionClassHandle(DIRECTED_REPORT);
         ParameterHandle directedCount = requester.getParameterHandle(
            directedReportClass, "HLAnumberOfClasses");
         ParameterHandle directedObject = requester.getParameterHandle(
            directedReportClass, "HLAobjectClass");
         ParameterHandle directedList = requester.getParameterHandle(
            directedReportClass, "HLAinteractionClassList");
         JavaTckSupport.check(requestClass != null && requestFederate != null
               && objectReportClass != null && objectCount != null && objectParameter != null
               && activeParameter != null && rateParameter != null && attributesParameter != null
               && interactionReportClass != null && interactionList != null
               && directedReportClass != null && directedCount != null
               && directedObject != null && directedList != null,
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
            "initial object-attribute subscription reports");
         JavaTckSupport.check(recorder.interactions.size() == 3,
            "initial object-attribute subscription query delivered an unexpected report count");
         verifyReports(encoder, requester, recorder.interactions, reliable, objectReportClass,
            objectCount, objectParameter, activeParameter, rateParameter, attributesParameter,
            interactionReportClass, interactionList, directedReportClass, directedCount,
            directedObject, directedList, requesterClass, updateRateName,
            Arrays.asList(requesterFirst, requesterSecond), "initial query");

         AttributeHandleSet removeAttributes = setFactory.create();
         removeAttributes.add(subjectFirst);
         List<AttributeHandle> remainingAttributes;
         if (fullAttributeUnsubscribe) {
            removeAttributes.add(subjectSecond);
         }
         if (fullAttributeUnsubscribe || wholeClassUnsubscribe) {
            remainingAttributes = List.of();
         } else {
            remainingAttributes = Arrays.asList(requesterSecond);
         }
         if (wholeClassUnsubscribe) {
            subject.unsubscribeObjectClass(subjectClass);
            attributesSubscribed = false;
         } else {
            subject.unsubscribeObjectClassAttributes(subjectClass, removeAttributes);
         }
         int offset = recorder.interactions.size();
         requester.sendInteraction(requestClass, request, new byte[0]);
         await(requester, callbackModel, () -> recorder.interactions.size() >= offset + 3,
            "subscription reports after object unsubscribe");
         JavaTckSupport.check(recorder.interactions.size() == offset + 3,
            "object unsubscribe did not deliver exactly three fresh standard reports");
         verifyReports(encoder, requester,
            recorder.interactions.subList(offset, offset + 3), reliable, objectReportClass,
            objectCount, objectParameter, activeParameter, rateParameter, attributesParameter,
            interactionReportClass, interactionList, directedReportClass, directedCount,
            directedObject, directedList, requesterClass, updateRateName,
            remainingAttributes, wholeClassUnsubscribe ? "after whole-class unsubscribe"
               : fullAttributeUnsubscribe ? "after full attribute unsubscribe"
               : "after partial attribute unsubscribe");
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
            AttributeHandleSet all = subject.getAttributeHandleSetFactory().create();
            all.add(subject.getAttributeHandle(cls, firstAttributeName));
            all.add(subject.getAttributeHandle(cls, secondAttributeName));
            subject.unsubscribeObjectClassAttributes(cls, all);
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

   private static void verifyReports(EncoderFactory encoder, RTIambassador requester,
         List<Interaction> reports, TransportationTypeHandle reliable,
         InteractionClassHandle objectReportClass, ParameterHandle countParameter,
         ParameterHandle objectParameter, ParameterHandle activeParameter,
         ParameterHandle rateParameter, ParameterHandle attributesParameter,
         InteractionClassHandle interactionReportClass, ParameterHandle interactionListParameter,
         InteractionClassHandle directedReportClass, ParameterHandle directedCountParameter,
         ParameterHandle directedObjectParameter, ParameterHandle directedListParameter,
         ObjectClassHandle expectedClass, String expectedRate, List<AttributeHandle> expectedAttributes,
         String phase) throws Exception {
      Interaction objectReport = onlyReport(reports, objectReportClass, phase + " object");
      Interaction interactionReport = onlyReport(reports, interactionReportClass,
         phase + " interaction");
      Interaction directedReport = onlyReport(reports, directedReportClass, phase + " directed");
      verifyMetadata(objectReport, reliable, phase + " object report");
      verifyMetadata(interactionReport, reliable, phase + " interaction report");
      verifyMetadata(directedReport, reliable, phase + " directed report");

      HLAinteger32BE count = encoder.createHLAinteger32BE();
      JavaTckSupport.check(objectReport.parameters.containsKey(countParameter),
         phase + " object report omitted its class count");
      count.decode(objectReport.parameters.get(countParameter));
      if (expectedAttributes.isEmpty()) {
         JavaTckSupport.check(objectReport.parameters.size() == 1 && count.getValue() == 0
               && !objectReport.parameters.containsKey(objectParameter)
               && !objectReport.parameters.containsKey(activeParameter)
               && !objectReport.parameters.containsKey(rateParameter)
               && !objectReport.parameters.containsKey(attributesParameter),
            phase + " did not use the standard zero-class object-subscription NULL response");
      } else {
         JavaTckSupport.check(objectReport.parameters.size() == 5
               && objectReport.parameters.containsKey(objectParameter)
               && objectReport.parameters.containsKey(activeParameter)
               && objectReport.parameters.containsKey(rateParameter)
               && objectReport.parameters.containsKey(attributesParameter),
            phase + " object report did not contain all five standard parameters");
         HLAboolean active = encoder.createHLAboolean();
         active.decode(objectReport.parameters.get(activeParameter));
         HLAunicodeString rate = encoder.createHLAunicodeString();
         rate.decode(objectReport.parameters.get(rateParameter));
         ObjectClassHandle reportedClass = requester.getObjectClassHandleFactory().decode(
            objectReport.parameters.get(objectParameter), 0);
         List<AttributeHandle> reportedAttributes = decodeHandles(encoder,
            bytes -> requester.getAttributeHandleFactory().decode(bytes, 0),
            objectReport.parameters.get(attributesParameter));
         JavaTckSupport.check(count.getValue() == 1 && active.getValue()
               && expectedClass.equals(reportedClass) && expectedRate.equals(rate.getValue())
               && reportedAttributes.size() == expectedAttributes.size()
               && new HashSet<>(reportedAttributes).equals(new HashSet<>(expectedAttributes)),
            phase + " report did not preserve the expected active state, rate, and exact attribute set");
      }

      JavaTckSupport.check(interactionReport.parameters.size() == 1
            && interactionReport.parameters.containsKey(interactionListParameter)
            && decodeHandles(encoder,
               bytes -> requester.getInteractionClassHandleFactory().decode(bytes, 0),
               interactionReport.parameters.get(interactionListParameter)).isEmpty(),
         phase + " interaction-subscription NULL report had the wrong shape");
      JavaTckSupport.check(directedReport.parameters.size() == 2
            && directedReport.parameters.containsKey(directedCountParameter)
            && !directedReport.parameters.containsKey(directedObjectParameter)
            && directedReport.parameters.containsKey(directedListParameter),
         phase + " directed-subscription NULL report had the wrong parameter shape");
      HLAinteger32BE directedCount = encoder.createHLAinteger32BE();
      directedCount.decode(directedReport.parameters.get(directedCountParameter));
      JavaTckSupport.check(directedCount.getValue() == 0
            && decodeHandles(encoder,
               bytes -> requester.getInteractionClassHandleFactory().decode(bytes, 0),
               directedReport.parameters.get(directedListParameter)).isEmpty(),
         phase + " directed-subscription NULL report did not encode zero and an empty list");
   }

   private static <H> List<H> decodeHandles(EncoderFactory encoder,
         HandleDecoder<H> decoder, byte[] encoded) throws Exception {
      HLAvariableArray<HLAvariableArray<HLAbyte>> refs = encoder.createHLAvariableArray(
         outer -> encoder.createHLAvariableArray(inner -> encoder.createHLAbyte()));
      refs.decode(encoded);
      List<H> result = new ArrayList<>();
      for (HLAvariableArray<HLAbyte> ref : refs) result.add(decoder.decode(ref.toByteArray()));
      return result;
   }

   private static Interaction onlyReport(List<Interaction> reports,
         InteractionClassHandle expected, String phase) {
      Interaction found = null;
      for (Interaction report : reports) if (expected.equals(report.interaction)) {
         JavaTckSupport.check(found == null, "duplicate " + phase + " report");
         found = report;
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
      if ("toString".equals(name)) return "Java TCK partial attribute unsubscribe callbacks";
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
