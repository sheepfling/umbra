package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.InteractionClassHandleFactory;
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

/** Verifies mixed active/passive interaction subscriptions through the standard MIM. */
final class FederateMomMultipleInteractionSubscriptionQueryTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 15_000_000_000L;
   private static final String REQUEST =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestSubscriptions";
   private static final String OBJECT_REPORT =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectClassSubscription";
   private static final String INTERACTION_REPORT =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionSubscription";
   private static final String DIRECTED_REPORT =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportDirectedInteractionSubscription";
   private static final String SERVICE_REPORT =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";

   private FederateMomMultipleInteractionSubscriptionQueryTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String interactionClassName, CallbackModel callbackModel) throws Exception {
      run(factory, fom, mim, timeImplementation, interactionClassName, callbackModel, false, false);
   }

   static void runAfterSecondaryUnsubscribe(RtiFactory factory, String fom, String mim,
         String timeImplementation, String interactionClassName, CallbackModel callbackModel)
         throws Exception {
      run(factory, fom, mim, timeImplementation, interactionClassName, callbackModel, true, false);
   }

   static void runAfterFullUnsubscribe(RtiFactory factory, String fom, String mim,
         String timeImplementation, String interactionClassName, CallbackModel callbackModel)
         throws Exception {
      run(factory, fom, mim, timeImplementation, interactionClassName, callbackModel, true, true);
   }

   private static void run(RtiFactory factory, String fom, String mim,
         String timeImplementation, String interactionClassName, CallbackModel callbackModel,
         boolean unsubscribePassiveClass, boolean unsubscribeRemainingActiveClass)
         throws Exception {
      String federateType = System.getProperty("hla.rti.tck.federateType", "tck-type");
      JavaTckSupport.check(!federateType.isEmpty(),
         "adapter-supplied federate type must not be empty");
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-federate-mom-multiple-interaction-subscription-" + UUID.randomUUID();
      String subjectName = "java-tck-interaction-subscription-subject-" + UUID.randomUUID();
      String requesterName = "java-tck-interaction-subscription-requester-" + UUID.randomUUID();
      boolean subjectConnected = false, requesterConnected = false, created = false;
      boolean subjectJoined = false, requesterJoined = false;
      boolean objectSubscribed = false, interactionSubscribed = false, directedSubscribed = false;
      boolean subjectApplicationSubscribed = false, subjectServiceReportSubscribed = false;
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

         InteractionClassHandle subjectApplicationClass =
            subject.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle requesterApplicationClass =
            requester.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle subjectServiceReportClass =
            subject.getInteractionClassHandle(SERVICE_REPORT);
         InteractionClassHandle requesterServiceReportClass =
            requester.getInteractionClassHandle(SERVICE_REPORT);
         TransportationTypeHandle reliable = requester.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(subjectApplicationClass != null
               && requesterApplicationClass != null && subjectServiceReportClass != null
               && requesterServiceReportClass != null && targetFederate != null
               && reliable != null && !subjectApplicationClass.equals(subjectServiceReportClass)
               && subjectApplicationClass.equals(requesterApplicationClass)
               && subjectServiceReportClass.equals(requesterServiceReportClass),
            "adapter application and standard MIM interaction handles were invalid or inconsistent");

         subject.subscribeInteractionClass(subjectApplicationClass);
         subjectApplicationSubscribed = true;
         subject.subscribeInteractionClassPassively(subjectServiceReportClass);
         subjectServiceReportSubscribed = true;

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
         await(requester, callbackModel, () -> recorder.interactions.size() >= 3,
            "three standard subscription reports for the subject");
         JavaTckSupport.check(recorder.interactions.size() == 3,
            "mixed interaction-subscription request did not deliver exactly three reports");

         List<Interaction> reports = new ArrayList<>(recorder.interactions);
         Interaction objectReport = onlyReport(reports, objectReportClass, "object-subscription");
         Interaction interactionReport = onlyReport(reports, interactionReportClass,
            "interaction-subscription");
         Interaction directedReport = onlyReport(reports, directedReportClass,
            "directed-subscription");
         verifyMetadata(objectReport, reliable, "object-subscription report");
         verifyMetadata(interactionReport, reliable, "interaction-subscription report");
         verifyMetadata(directedReport, reliable, "directed-subscription report");
         verifyNullObjectReport(encoder, objectReport, objectReportCount,
            objectReportClassParameter, objectReportActive, objectReportRate,
            objectReportAttributes);
         JavaTckSupport.check(interactionReport.parameters.size() == 1
               && interactionReport.parameters.containsKey(interactionReportList),
            "interaction-subscription report did not contain only its MIM class-list parameter");
         verifyInteractionSubscriptions(encoder, requester.getInteractionClassHandleFactory(),
            interactionReport.parameters.get(interactionReportList), requesterApplicationClass,
            requesterServiceReportClass);
         verifyNullDirectedReport(encoder, requester.getInteractionClassHandleFactory(), directedReport,
            directedReportCount, directedReportObjectClass, directedReportList);

         if (unsubscribePassiveClass) {
            subject.unsubscribeInteractionClass(subjectServiceReportClass);
            int reportOffset = recorder.interactions.size();
            requester.sendInteraction(requestClass, request, new byte[0]);
            await(requester, callbackModel,
               () -> recorder.interactions.size() >= reportOffset + 3,
               "standard subscription reports after removing the passive interaction class");
            JavaTckSupport.check(recorder.interactions.size() == reportOffset + 3,
               "secondary interaction unsubscribe did not deliver exactly three standard reports");
            List<Interaction> remainingReports = new ArrayList<>(
               recorder.interactions.subList(reportOffset, reportOffset + 3));
            Interaction remainingObjectReport = onlyReport(remainingReports, objectReportClass,
               "post-unsubscribe object-subscription");
            Interaction remainingInteractionReport = onlyReport(remainingReports,
               interactionReportClass, "post-unsubscribe interaction-subscription");
            Interaction remainingDirectedReport = onlyReport(remainingReports, directedReportClass,
               "post-unsubscribe directed-subscription");
            verifyMetadata(remainingObjectReport, reliable, "post-unsubscribe object report");
            verifyMetadata(remainingInteractionReport, reliable,
               "post-unsubscribe interaction report");
            verifyMetadata(remainingDirectedReport, reliable,
               "post-unsubscribe directed report");
            verifyNullObjectReport(encoder, remainingObjectReport, objectReportCount,
               objectReportClassParameter, objectReportActive, objectReportRate,
               objectReportAttributes);
            JavaTckSupport.check(remainingInteractionReport.parameters.size() == 1
                  && remainingInteractionReport.parameters.containsKey(interactionReportList),
               "post-unsubscribe interaction report did not contain only its class-list parameter");
            verifyOnlyActiveInteraction(encoder, requester.getInteractionClassHandleFactory(),
               remainingInteractionReport.parameters.get(interactionReportList),
               requesterApplicationClass);
            verifyNullDirectedReport(encoder, requester.getInteractionClassHandleFactory(),
               remainingDirectedReport, directedReportCount, directedReportObjectClass,
               directedReportList);
         }

         if (unsubscribeRemainingActiveClass) {
            subject.unsubscribeInteractionClass(subjectApplicationClass);
            int reportOffset = recorder.interactions.size();
            requester.sendInteraction(requestClass, request, new byte[0]);
            await(requester, callbackModel,
               () -> recorder.interactions.size() >= reportOffset + 3,
               "standard subscription reports after removing the final active interaction class");
            JavaTckSupport.check(recorder.interactions.size() == reportOffset + 3,
               "final interaction unsubscribe did not deliver exactly three standard reports");
            List<Interaction> emptyReports = new ArrayList<>(
               recorder.interactions.subList(reportOffset, reportOffset + 3));
            Interaction emptyObjectReport = onlyReport(emptyReports, objectReportClass,
               "full-unsubscribe object-subscription");
            Interaction emptyInteractionReport = onlyReport(emptyReports, interactionReportClass,
               "full-unsubscribe interaction-subscription");
            Interaction emptyDirectedReport = onlyReport(emptyReports, directedReportClass,
               "full-unsubscribe directed-subscription");
            verifyMetadata(emptyObjectReport, reliable, "full-unsubscribe object report");
            verifyMetadata(emptyInteractionReport, reliable,
               "full-unsubscribe interaction report");
            verifyMetadata(emptyDirectedReport, reliable, "full-unsubscribe directed report");
            verifyNullObjectReport(encoder, emptyObjectReport, objectReportCount,
               objectReportClassParameter, objectReportActive, objectReportRate,
               objectReportAttributes);
            JavaTckSupport.check(emptyInteractionReport.parameters.size() == 1
                  && emptyInteractionReport.parameters.containsKey(interactionReportList)
                  && decodeSubscriptions(encoder, requester.getInteractionClassHandleFactory(),
                     emptyInteractionReport.parameters.get(interactionReportList)).isEmpty(),
               "full interaction unsubscribe did not use the standard empty-list NULL response");
            verifyNullDirectedReport(encoder, requester.getInteractionClassHandleFactory(),
               emptyDirectedReport, directedReportCount, directedReportObjectClass,
               directedReportList);
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
         if (subjectApplicationSubscribed && subjectJoined) try {
            subject.unsubscribeInteractionClass(subject.getInteractionClassHandle(interactionClassName));
         } catch (Exception ignored) { }
         if (subjectServiceReportSubscribed && subjectJoined) try {
            subject.unsubscribeInteractionClass(subject.getInteractionClassHandle(SERVICE_REPORT));
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
         ParameterHandle attributesParameter) throws Exception {
      JavaTckSupport.check(report.parameters.size() == 1
            && report.parameters.containsKey(countParameter)
            && !report.parameters.containsKey(objectClassParameter)
            && !report.parameters.containsKey(activeParameter)
            && !report.parameters.containsKey(rateParameter)
            && !report.parameters.containsKey(attributesParameter),
         "object-subscription report did not use the standard zero-class NULL response shape");
      HLAinteger32BE count = encoder.createHLAinteger32BE();
      count.decode(report.parameters.get(countParameter));
      JavaTckSupport.check(count.getValue() == 0,
         "object-subscription NULL response did not report zero classes");
   }

   private static void verifyInteractionSubscriptions(EncoderFactory encoder,
         InteractionClassHandleFactory factory, byte[] encoded,
         InteractionClassHandle applicationClass, InteractionClassHandle serviceReportClass)
         throws Exception {
      List<Subscription> subscriptions = decodeSubscriptions(encoder, factory, encoded);
      JavaTckSupport.check(subscriptions.size() == 2,
         "interaction-subscription report did not contain exactly two classes");
      boolean foundApplication = false, foundServiceReport = false;
      for (Subscription subscription : subscriptions) {
         if (applicationClass.equals(subscription.interaction)) {
            JavaTckSupport.check(!foundApplication && subscription.active,
               "application interaction subscription was missing, duplicated, or passive");
            foundApplication = true;
         } else if (serviceReportClass.equals(subscription.interaction)) {
            JavaTckSupport.check(!foundServiceReport && !subscription.active,
               "standard service-report subscription was missing, duplicated, or active");
            foundServiceReport = true;
         } else {
            JavaTckSupport.check(false,
               "interaction-subscription report contained an unselected class");
         }
      }
      JavaTckSupport.check(foundApplication && foundServiceReport,
         "interaction-subscription report omitted one of the selected classes");
   }

   private static void verifyOnlyActiveInteraction(EncoderFactory encoder,
         InteractionClassHandleFactory factory, byte[] encoded,
         InteractionClassHandle expectedClass) throws Exception {
      List<Subscription> subscriptions = decodeSubscriptions(encoder, factory, encoded);
      JavaTckSupport.check(subscriptions.size() == 1
            && expectedClass.equals(subscriptions.get(0).interaction)
            && subscriptions.get(0).active,
         "post-unsubscribe report did not retain only the active application interaction class");
   }

   private static void verifyNullDirectedReport(EncoderFactory encoder,
         InteractionClassHandleFactory interactionFactory,
         Interaction report, ParameterHandle countParameter,
         ParameterHandle objectClassParameter, ParameterHandle listParameter) throws Exception {
      JavaTckSupport.check(report.parameters.size() == 2
            && report.parameters.containsKey(countParameter)
            && !report.parameters.containsKey(objectClassParameter)
            && report.parameters.containsKey(listParameter),
         "empty directed-subscription report did not use the standard NULL response shape");
      HLAinteger32BE count = encoder.createHLAinteger32BE();
      count.decode(report.parameters.get(countParameter));
      JavaTckSupport.check(count.getValue() == 0
            && decodeSubscriptions(encoder, interactionFactory,
               report.parameters.get(listParameter)).isEmpty(),
         "empty directed-subscription report did not encode zero classes and an empty list");
   }

   private static List<Subscription> decodeSubscriptions(EncoderFactory encoder,
         InteractionClassHandleFactory factory, byte[] encoded) throws Exception {
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
         result.add(new Subscription(factory.decode(handle.toByteArray(), 0), active.getValue()));
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
      if ("toString".equals(name)) return "Java TCK interaction subscription callbacks";
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
