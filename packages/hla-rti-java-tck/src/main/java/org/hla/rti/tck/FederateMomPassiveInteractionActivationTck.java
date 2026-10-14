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
import hla.rti1516_2025.encoding.HLAfixedRecord;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.encoding.HLAunicodeString;
import hla.rti1516_2025.encoding.HLAvariableArray;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies the standard MIM state change when a passive interaction subscription is activated. */
final class FederateMomPassiveInteractionActivationTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 15_000_000_000L;
   private static final String REQUEST =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestSubscriptions";
   private static final String OBJECT_REPORT =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectClassSubscription";
   private static final String INTERACTION_REPORT =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionSubscription";
   private static final String DIRECTED_REPORT =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportDirectedInteractionSubscription";

   private FederateMomPassiveInteractionActivationTck() { }

   static void run(RtiFactory factory, String fom, String rateFom, String mim,
         String timeImplementation, String objectClassName, String attributeName,
         String interactionClassName, String updateRateName, CallbackModel callbackModel)
         throws Exception {
      run(factory, fom, rateFom, mim, timeImplementation, objectClassName, attributeName,
         interactionClassName, updateRateName, callbackModel, true);
   }

   static void runInitialOnly(RtiFactory factory, String fom, String rateFom, String mim,
         String timeImplementation, String objectClassName, String attributeName,
         String interactionClassName, String updateRateName, CallbackModel callbackModel)
         throws Exception {
      run(factory, fom, rateFom, mim, timeImplementation, objectClassName, attributeName,
         interactionClassName, updateRateName, callbackModel, false);
   }

   private static void run(RtiFactory factory, String fom, String rateFom, String mim,
         String timeImplementation, String objectClassName, String attributeName,
         String interactionClassName, String updateRateName, CallbackModel callbackModel,
         boolean activateAfterInitialQuery) throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-federate-mom-passive-interaction-activation-" + UUID.randomUUID();
      String subjectName = "java-tck-passive-interaction-subject-" + UUID.randomUUID();
      String requesterName = "java-tck-passive-interaction-requester-" + UUID.randomUUID();
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
         InteractionClassHandle subjectInteraction =
            subject.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle requesterInteraction =
            requester.getInteractionClassHandle(interactionClassName);
         TransportationTypeHandle reliable = requester.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(subjectObjectClass != null && requesterObjectClass != null
               && subjectAttribute != null && requesterAttribute != null
               && subjectInteraction != null && requesterInteraction != null
               && subjectInteraction.equals(requesterInteraction) && targetFederate != null
               && reliable != null,
            "adapter-selected object, attribute, interaction, or federate handle was invalid");

         AttributeHandleSetFactory attributeSetFactory =
            subject.getAttributeHandleSetFactory();
         AttributeHandleSet attributes = attributeSetFactory.create();
         attributes.add(subjectAttribute);
         subject.subscribeObjectClassAttributes(subjectObjectClass, attributes, updateRateName);
         objectSubscribed = true;
         subject.subscribeInteractionClassPassively(subjectInteraction);
         interactionSubscribed = true;

         InteractionClassHandle requestClass = requester.getInteractionClassHandle(REQUEST);
         ParameterHandle requestFederate = requester.getParameterHandle(requestClass, "HLAfederate");
         objectReportClass = requester.getInteractionClassHandle(OBJECT_REPORT);
         ParameterHandle objectReportCount = requester.getParameterHandle(
            objectReportClass, "HLAnumberOfClasses");
         ParameterHandle objectReportObjectClass = requester.getParameterHandle(
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
               && objectReportObjectClass != null && objectReportActive != null
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
            "initial standard MIM subscription reports");
         verifyReports(encoder, requester, recorder.interactions.subList(0, 3), reliable,
            objectReportClass, objectReportCount, objectReportObjectClass, objectReportActive,
            objectReportRate, objectReportAttributes, interactionReportClass,
            interactionReportList, directedReportClass, directedReportCount,
            directedReportObject, directedReportList, requesterObjectClass,
            requesterAttribute, requesterInteraction, updateRateName, false,
            "initial passive interaction subscription");
         JavaTckSupport.check(recorder.interactions.size() == 3,
            "initial subscription query delivered an unexpected number of MIM reports");

         if (activateAfterInitialQuery) {
            subject.subscribeInteractionClass(subjectInteraction);
            int offset = recorder.interactions.size();
            requester.sendInteraction(requestClass, request, new byte[0]);
            await(requester, callbackModel, () -> recorder.interactions.size() >= offset + 3,
               "standard MIM reports after passive subscription activation");
            JavaTckSupport.check(recorder.interactions.size() == offset + 3,
               "passive interaction activation did not deliver exactly three fresh MIM reports");
            verifyReports(encoder, requester, recorder.interactions.subList(offset, offset + 3),
               reliable, objectReportClass, objectReportCount, objectReportObjectClass,
               objectReportActive, objectReportRate, objectReportAttributes,
               interactionReportClass, interactionReportList, directedReportClass,
               directedReportCount, directedReportObject, directedReportList,
               requesterObjectClass, requesterAttribute, requesterInteraction,
               updateRateName, true, "activated interaction subscription");
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

   private static void verifyReports(EncoderFactory encoder, RTIambassador requester,
         List<Interaction> reports, TransportationTypeHandle reliable,
         InteractionClassHandle objectReportClass, ParameterHandle objectCountParameter,
         ParameterHandle objectClassParameter, ParameterHandle objectActiveParameter,
         ParameterHandle objectRateParameter, ParameterHandle objectAttributesParameter,
         InteractionClassHandle interactionReportClass, ParameterHandle interactionListParameter,
         InteractionClassHandle directedReportClass, ParameterHandle directedCountParameter,
         ParameterHandle directedObjectParameter, ParameterHandle directedListParameter,
         ObjectClassHandle expectedObjectClass, AttributeHandle expectedAttribute,
         InteractionClassHandle expectedInteraction, String expectedRate,
         boolean expectedInteractionActive, String phase) throws Exception {
      Interaction objectReport = onlyReport(reports, objectReportClass, phase + " object");
      Interaction interactionReport = onlyReport(reports, interactionReportClass,
         phase + " interaction");
      Interaction directedReport = onlyReport(reports, directedReportClass,
         phase + " directed");
      verifyMetadata(objectReport, reliable, phase + " object report");
      verifyMetadata(interactionReport, reliable, phase + " interaction report");
      verifyMetadata(directedReport, reliable, phase + " directed report");

      JavaTckSupport.check(objectReport.parameters.size() == 5
            && objectReport.parameters.containsKey(objectCountParameter)
            && objectReport.parameters.containsKey(objectClassParameter)
            && objectReport.parameters.containsKey(objectActiveParameter)
            && objectReport.parameters.containsKey(objectRateParameter)
            && objectReport.parameters.containsKey(objectAttributesParameter),
         phase + " object-subscription report did not contain its five standard parameters");
      HLAinteger32BE objectCount = encoder.createHLAinteger32BE();
      objectCount.decode(objectReport.parameters.get(objectCountParameter));
      HLAboolean objectActive = encoder.createHLAboolean();
      objectActive.decode(objectReport.parameters.get(objectActiveParameter));
      HLAunicodeString updateRate = encoder.createHLAunicodeString();
      updateRate.decode(objectReport.parameters.get(objectRateParameter));
      ObjectClassHandle reportedObjectClass = requester.getObjectClassHandleFactory().decode(
         objectReport.parameters.get(objectClassParameter), 0);
      List<AttributeHandle> attributes = decodeHandles(encoder,
         bytes -> requester.getAttributeHandleFactory().decode(bytes, 0),
         objectReport.parameters.get(objectAttributesParameter));
      JavaTckSupport.check(objectCount.getValue() == 1 && objectActive.getValue()
            && expectedObjectClass.equals(reportedObjectClass)
            && expectedRate.equals(updateRate.getValue()) && attributes.size() == 1
            && attributes.contains(expectedAttribute),
         phase + " changed the active object-class subscription state or payload");

      JavaTckSupport.check(interactionReport.parameters.size() == 1
            && interactionReport.parameters.containsKey(interactionListParameter),
         phase + " interaction report did not contain only its standard class-list parameter");
      List<Subscription> subscriptions = decodeSubscriptions(encoder,
         requester.getInteractionClassHandleFactory(),
         interactionReport.parameters.get(interactionListParameter));
      JavaTckSupport.check(subscriptions.size() == 1
            && expectedInteraction.equals(subscriptions.get(0).interaction)
            && subscriptions.get(0).active == expectedInteractionActive,
         phase + " did not report the expected interaction-class active state");

      JavaTckSupport.check(directedReport.parameters.size() == 2
            && directedReport.parameters.containsKey(directedCountParameter)
            && !directedReport.parameters.containsKey(directedObjectParameter)
            && directedReport.parameters.containsKey(directedListParameter),
         phase + " directed-subscription report did not use its standard NULL shape");
      HLAinteger32BE directedCount = encoder.createHLAinteger32BE();
      directedCount.decode(directedReport.parameters.get(directedCountParameter));
      JavaTckSupport.check(directedCount.getValue() == 0
            && decodeSubscriptions(encoder, requester.getInteractionClassHandleFactory(),
               directedReport.parameters.get(directedListParameter)).isEmpty(),
         phase + " changed the standard empty directed-subscription report");
   }

   private static List<Subscription> decodeSubscriptions(EncoderFactory encoder,
         InteractionClassHandleFactory factory, byte[] encoded) throws Exception {
      HLAvariableArray<HLAbyte> handlePrototype = encoder.createHLAvariableArray(
         index -> encoder.createHLAbyte(), encoder.createHLAbyte());
      HLAfixedRecord prototype = encoder.createHLAfixedRecord();
      prototype.add(handlePrototype);
      prototype.add(encoder.createHLAboolean());
      HLAvariableArray<HLAfixedRecord> records = encoder.createHLAvariableArray(index -> {
         HLAfixedRecord record = encoder.createHLAfixedRecord();
         record.add(encoder.createHLAvariableArray(inner -> encoder.createHLAbyte(),
            encoder.createHLAbyte()));
         record.add(encoder.createHLAboolean());
         return record;
      }, prototype);
      records.decode(encoded);
      List<Subscription> subscriptions = new ArrayList<>();
      for (HLAfixedRecord record : records) {
         @SuppressWarnings("unchecked")
         HLAvariableArray<HLAbyte> handle = (HLAvariableArray<HLAbyte>) record.get(0);
         HLAboolean active = (HLAboolean) record.get(1);
         subscriptions.add(new Subscription(factory.decode(handle.toByteArray(), 0),
            active.getValue()));
      }
      return subscriptions;
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
      if ("toString".equals(name)) return "Java TCK passive interaction activation callbacks";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == (args == null ? null : args[0]);
      return null;
   }

   private interface HandleDecoder<H> { H decode(byte[] encoded) throws Exception; }

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
