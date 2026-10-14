package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
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
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies active object and interaction subscriptions through the standard MIM. */
final class FederateMomSubscriptionQueryTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 15_000_000_000L;
   private static final String REQUEST =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestSubscriptions";
   private static final String OBJECT_REPORT =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectClassSubscription";
   private static final String INTERACTION_REPORT =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionSubscription";
   private static final String DIRECTED_REPORT =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportDirectedInteractionSubscription";

   private FederateMomSubscriptionQueryTck() { }

   static void run(RtiFactory factory, String fom, String rateFom, String mim,
         String timeImplementation, String objectClassName, String attributeName,
         String interactionClassName, String updateRateName, CallbackModel callbackModel)
         throws Exception {
      run(factory, fom, rateFom, mim, timeImplementation, objectClassName, attributeName,
         interactionClassName, updateRateName, callbackModel, true, true, true);
   }

   static void runPassively(RtiFactory factory, String fom, String rateFom, String mim,
         String timeImplementation, String objectClassName, String attributeName,
         String interactionClassName, String updateRateName, CallbackModel callbackModel)
         throws Exception {
      run(factory, fom, rateFom, mim, timeImplementation, objectClassName, attributeName,
         interactionClassName, updateRateName, callbackModel, true, false, true);
   }

   static void runEmptySubscriptions(RtiFactory factory, String fom, String rateFom, String mim,
         String timeImplementation, String objectClassName, String attributeName,
         String interactionClassName, String updateRateName, CallbackModel callbackModel)
         throws Exception {
      run(factory, fom, rateFom, mim, timeImplementation, objectClassName, attributeName,
         interactionClassName, updateRateName, callbackModel, false, false, false);
   }

   private static void run(RtiFactory factory, String fom, String rateFom, String mim,
         String timeImplementation, String objectClassName, String attributeName,
         String interactionClassName, String updateRateName, CallbackModel callbackModel,
         boolean includeObjectSubscription, boolean activeObjectSubscription,
         boolean includeInteractionSubscription) throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-federate-mom-subscription-query-" + UUID.randomUUID();
      String subjectName = "java-tck-subscription-subject-" + UUID.randomUUID();
      String requesterName = "java-tck-subscription-requester-" + UUID.randomUUID();
      boolean subjectConnected = false, requesterConnected = false, created = false;
      boolean subjectJoined = false, requesterJoined = false;
      boolean objectSubscribed = false, interactionSubscribed = false;
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
         AttributeHandle subjectAttribute = subject.getAttributeHandle(subjectObjectClass,
            attributeName);
         AttributeHandle requesterAttribute = requester.getAttributeHandle(requesterObjectClass,
            attributeName);
         InteractionClassHandle subjectInteraction = subject.getInteractionClassHandle(
            interactionClassName);
         InteractionClassHandle requesterInteraction = requester.getInteractionClassHandle(
            interactionClassName);
         TransportationTypeHandle reliable = requester.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(subjectObjectClass != null && requesterObjectClass != null
               && subjectAttribute != null && requesterAttribute != null
               && subjectInteraction != null && requesterInteraction != null && reliable != null,
            "adapter-selected object, attribute, interaction, or transportation lookup failed");

         if (includeObjectSubscription) {
            AttributeHandleSet attributes = subject.getAttributeHandleSetFactory().create();
            attributes.add(subjectAttribute);
            if (activeObjectSubscription) {
               subject.subscribeObjectClassAttributes(subjectObjectClass, attributes, updateRateName);
            } else {
               subject.subscribeObjectClassAttributesPassively(subjectObjectClass, attributes,
                  updateRateName);
            }
            objectSubscribed = true;
         }
         if (includeInteractionSubscription) {
            subject.subscribeInteractionClass(subjectInteraction);
            interactionSubscribed = true;
         }

         InteractionClassHandle requestClass = requester.getInteractionClassHandle(REQUEST);
         ParameterHandle requestFederate = requester.getParameterHandle(requestClass, "HLAfederate");
         objectReportClass = requester.getInteractionClassHandle(OBJECT_REPORT);
         ParameterHandle objectCount = requester.getParameterHandle(
            objectReportClass, "HLAnumberOfClasses");
         ParameterHandle objectParameter = requester.getParameterHandle(
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
         ParameterHandle directedObject = requester.getParameterHandle(
            directedReportClass, "HLAobjectClass");
         ParameterHandle directedList = requester.getParameterHandle(
            directedReportClass, "HLAinteractionClassList");
         JavaTckSupport.check(requestClass != null && requestFederate != null
               && objectReportClass != null && objectCount != null && objectParameter != null
               && objectActive != null && objectRate != null && objectAttributes != null
               && interactionReportClass != null && interactionList != null
               && directedReportClass != null && directedCount != null
               && directedObject != null && directedList != null,
            "standard MIM subscription request/report lookup returned a null handle");

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
            "object, interaction, and directed-subscription reports");
         JavaTckSupport.check(recorder.interactions.size() == 3,
            "subscription query delivered an unexpected number of standard MIM reports");

         Interaction objectReport = onlyReport(recorder.interactions, objectReportClass, "object");
         Interaction interactionReport = onlyReport(recorder.interactions, interactionReportClass,
            "interaction");
         Interaction directedReport = onlyReport(recorder.interactions, directedReportClass,
            "directed interaction");
         verifyMetadata(objectReport, reliable, "object-subscription report");
         verifyMetadata(interactionReport, reliable, "interaction-subscription report");
         verifyMetadata(directedReport, reliable, "directed-subscription report");

         JavaTckSupport.check(objectReport.parameters.containsKey(objectCount),
            "object-subscription report omitted its class count");
         HLAinteger32BE count = encoder.createHLAinteger32BE();
         count.decode(objectReport.parameters.get(objectCount));
         if (includeObjectSubscription) {
            JavaTckSupport.check(objectReport.parameters.size() == 5
               && objectReport.parameters.containsKey(objectParameter)
               && objectReport.parameters.containsKey(objectActive)
               && objectReport.parameters.containsKey(objectRate)
               && objectReport.parameters.containsKey(objectAttributes),
               "populated object-subscription report did not contain its five standard parameters");
            HLAboolean active = encoder.createHLAboolean();
            active.decode(objectReport.parameters.get(objectActive));
            HLAunicodeString rate = encoder.createHLAunicodeString();
            rate.decode(objectReport.parameters.get(objectRate));
            ObjectClassHandle reportedObjectClass = requester.getObjectClassHandleFactory().decode(
               objectReport.parameters.get(objectParameter), 0);
            List<AttributeHandle> reportedAttributes = decodeHandles(encoder,
               bytes -> requester.getAttributeHandleFactory().decode(bytes, 0),
               objectReport.parameters.get(objectAttributes));
            JavaTckSupport.check(count.getValue() == 1
                  && active.getValue() == activeObjectSubscription
                  && requesterObjectClass.equals(reportedObjectClass)
                  && updateRateName.equals(rate.getValue()) && reportedAttributes.size() == 1
                  && requesterAttribute.equals(reportedAttributes.get(0)),
               "MIM object report did not preserve the active state, configured rate, and exact attribute");
         } else {
            JavaTckSupport.check(objectReport.parameters.size() == 1 && count.getValue() == 0
                  && !objectReport.parameters.containsKey(objectParameter)
                  && !objectReport.parameters.containsKey(objectActive)
                  && !objectReport.parameters.containsKey(objectRate)
                  && !objectReport.parameters.containsKey(objectAttributes),
               "empty object-subscription report did not use the standard zero-class NULL response");
         }

         JavaTckSupport.check(interactionReport.parameters.size() == 1
               && interactionReport.parameters.containsKey(interactionList),
            "interaction-subscription report did not have its single standard parameter");
         List<InteractionClassHandle> reportedInteractions = decodeHandles(encoder,
            bytes -> requester.getInteractionClassHandleFactory().decode(bytes, 0),
            interactionReport.parameters.get(interactionList));
         JavaTckSupport.check(includeInteractionSubscription
               ? reportedInteractions.size() == 1
                  && requesterInteraction.equals(reportedInteractions.get(0))
               : reportedInteractions.isEmpty(),
            includeInteractionSubscription
               ? "MIM interaction report did not contain exactly the active adapter interaction"
               : "empty interaction-subscription report did not use the standard NULL list");

         JavaTckSupport.check(directedReport.parameters.size() == 2
               && directedReport.parameters.containsKey(directedCount)
               && !directedReport.parameters.containsKey(directedObject)
               && directedReport.parameters.containsKey(directedList),
            "empty directed-subscription report did not use the standard NULL parameter shape");
         HLAinteger32BE directedClassCount = encoder.createHLAinteger32BE();
         directedClassCount.decode(directedReport.parameters.get(directedCount));
         JavaTckSupport.check(directedClassCount.getValue() == 0
               && decodeHandles(encoder,
                  bytes -> requester.getInteractionClassHandleFactory().decode(bytes, 0),
                  directedReport.parameters.get(directedList)).isEmpty(),
            "empty directed-subscription report did not encode zero and an empty interaction list");
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
         if (interactionSubscribed && subjectJoined) try {
            subject.unsubscribeInteractionClass(subject.getInteractionClassHandle(interactionClassName));
         } catch (Exception ignored) { }
         if (objectSubscribed && subjectJoined) try {
            ObjectClassHandle cls = subject.getObjectClassHandle(objectClassName);
            AttributeHandleSet set = subject.getAttributeHandleSetFactory().create();
            set.add(subject.getAttributeHandle(cls, attributeName));
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

   private static <H> List<H> decodeHandles(EncoderFactory encoder, HandleDecoder<H> decoder,
         byte[] encoded) throws Exception {
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
      if ("toString".equals(name)) return "Java TCK active subscription callbacks";
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
