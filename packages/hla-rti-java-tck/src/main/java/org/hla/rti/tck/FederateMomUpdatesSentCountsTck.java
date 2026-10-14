package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
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
import hla.rti1516_2025.encoding.HLAbyte;
import hla.rti1516_2025.encoding.HLAfixedRecord;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.encoding.HLAvariableArray;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies the standard class- and transportation-grouped updates-sent MOM report. */
final class FederateMomUpdatesSentCountsTck {
   private static final String REPORT_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportUpdatesSent";
   private static final String REQUEST_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestUpdatesSent";
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private FederateMomUpdatesSentCountsTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador target = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder targetRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      String federation = "java-tck-federate-mom-updates-sent-counts-" + UUID.randomUUID();
      boolean targetConnected = false, requesterConnected = false, created = false;
      boolean targetJoined = false, requesterJoined = false;
      boolean published = false, reportSubscribed = false;
      ObjectClassHandle targetClass = null;
      AttributeHandle targetAttribute = null;
      AttributeHandleSet targetAttributes = null;
      InteractionClassHandle reportClass = null;
      try {
         target.connect(targetRecorder.proxy(), callbackModel);
         targetConnected = true;
         requester.connect(requesterRecorder.proxy(), callbackModel);
         requesterConnected = true;
         if (timeImplementation == null || timeImplementation.isEmpty())
            target.createFederationExecutionWithMIM(federation, new String[] {fom}, mim);
         else
            target.createFederationExecutionWithMIM(federation, new String[] {fom}, mim,
               timeImplementation);
         created = true;
         FederateHandle targetFederate = target.joinFederationExecution(
            "updates-sent-count-target", "java-tck", federation);
         targetJoined = true;
         FederateHandle requesterFederate = requester.joinFederationExecution(
            "updates-sent-count-requester", "java-tck", federation);
         requesterJoined = true;

         targetClass = target.getObjectClassHandle(objectClassName);
         targetAttribute = target.getAttributeHandle(targetClass, attributeName);
         JavaTckSupport.check(targetClass != null && targetAttribute != null
               && objectClassName.equals(target.getObjectClassName(targetClass))
               && attributeName.equals(target.getAttributeName(targetClass, targetAttribute)),
            "adapter-supplied updates-sent object and attribute lookup did not round-trip");
         TransportationTypeHandle reliable = target.getTransportationTypeHandle("HLAreliable");
         TransportationTypeHandle bestEffort = target.getTransportationTypeHandle("HLAbestEffort");
         JavaTckSupport.check(reliable != null && bestEffort != null
               && "HLAreliable".equals(target.getTransportationTypeName(reliable))
               && "HLAbestEffort".equals(target.getTransportationTypeName(bestEffort)),
            "standard updates-sent transportation lookup did not round-trip");
         targetAttributes = JavaTckSupport.attributeSet(target, targetAttribute);
         target.publishObjectClassAttributes(targetClass, targetAttributes);
         published = true;

         ObjectInstanceHandle object = target.registerObjectInstance(targetClass);
         JavaTckSupport.check(object != null,
            "updates-sent registration returned an invalid object handle");
         sendUpdate(target, object, targetAttribute, encoder, (byte) 0x51);
         target.requestAttributeTransportationTypeChange(object, targetAttributes, bestEffort);
         await(target, callbackModel,
            () -> !targetRecorder.transportConfirmations.isEmpty(),
            "updates-sent attribute transportation confirmation");
         JavaTckSupport.check(targetRecorder.transportConfirmations.size() == 1
               && object.equals(targetRecorder.transportConfirmations.get(0).object)
               && targetRecorder.transportConfirmations.get(0).attributes.size() == 1
               && targetRecorder.transportConfirmations.get(0).attributes.contains(targetAttribute)
               && bestEffort.equals(targetRecorder.transportConfirmations.get(0).transportation),
            "updates-sent transportation change did not confirm best effort for the target attribute");
         sendUpdate(target, object, targetAttribute, encoder, (byte) 0x52);
         sendUpdate(target, object, targetAttribute, encoder, (byte) 0x53);

         reportClass = requester.getInteractionClassHandle(REPORT_CLASS);
         InteractionClassHandle requestClass = requester.getInteractionClassHandle(REQUEST_CLASS);
         ParameterHandle requestFederate = requester.getParameterHandle(requestClass, "HLAfederate");
         ParameterHandle reportTransportation = requester.getParameterHandle(
            reportClass, "HLAtransportation");
         ParameterHandle reportCounts = requester.getParameterHandle(reportClass, "HLAupdateCounts");
         JavaTckSupport.check(reportClass != null && requestClass != null
               && requestFederate != null && reportTransportation != null && reportCounts != null
               && REPORT_CLASS.equals(requester.getInteractionClassName(reportClass))
               && REQUEST_CLASS.equals(requester.getInteractionClassName(requestClass))
               && "HLAfederate".equals(requester.getParameterName(requestClass, requestFederate))
               && "HLAtransportation".equals(requester.getParameterName(
                  reportClass, reportTransportation))
               && "HLAupdateCounts".equals(requester.getParameterName(reportClass, reportCounts)),
            "standard HLArequestUpdatesSent/HLAreportUpdatesSent lookup did not round-trip");
         TransportationTypeHandle reportReliable =
            requester.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(reportReliable != null
               && "HLAreliable".equals(requester.getTransportationTypeName(reportReliable)),
            "updates-sent report reliable transportation lookup did not round-trip");
         requester.subscribeInteractionClass(reportClass);
         reportSubscribed = true;

         byte[] expectedClass = encode(targetClass);
         List<Interaction> targetReports = requestReports(requester, requesterRecorder,
            callbackModel, requestClass, requestFederate, targetFederate,
            "target updates-sent report request");
         verifyReports(encoder, targetReports, reportClass, reportTransportation, reportCounts,
            reliable, bestEffort, reportReliable, expectedClass, true, "target updates-sent report");

         List<Interaction> emptyReports = requestReports(requester, requesterRecorder,
            callbackModel, requestClass, requestFederate, requesterFederate,
            "empty updates-sent report request");
         verifyReports(encoder, emptyReports, reportClass, reportTransportation, reportCounts,
            reliable, bestEffort, reportReliable, expectedClass, false, "empty updates-sent report");

         requester.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         target.unpublishObjectClassAttributes(targetClass, targetAttributes);
         published = false;
         requester.resignFederationExecution(ResignAction.NO_ACTION);
         requesterJoined = false;
         target.resignFederationExecution(ResignAction.NO_ACTION);
         targetJoined = false;
         target.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (reportSubscribed && requesterJoined && reportClass != null) try {
            requester.unsubscribeInteractionClass(reportClass);
         } catch (Exception ignored) { }
         if (published && targetJoined && targetClass != null && targetAttributes != null) try {
            target.unpublishObjectClassAttributes(targetClass, targetAttributes);
         } catch (Exception ignored) { }
         if (requesterJoined) resign(requester);
         if (targetJoined) resign(target);
         if (created) try { target.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (requesterConnected) disconnect(requester);
         if (targetConnected) disconnect(target);
      }
   }

   private static void sendUpdate(RTIambassador target, ObjectInstanceHandle object,
         AttributeHandle attribute, EncoderFactory encoder, byte value) throws Exception {
      AttributeHandleValueMap values = target.getAttributeHandleValueMapFactory().create(1);
      values.put(attribute, encoder.createHLAbyte(value).toByteArray());
      target.updateAttributeValues(object, values, new byte[0]);
   }

   private static List<Interaction> requestReports(RTIambassador requester, Recorder recorder,
         CallbackModel model, InteractionClassHandle requestClass, ParameterHandle federateParameter,
         FederateHandle subject, String description) throws Exception {
      recorder.interactions.clear();
      ParameterHandleValueMap parameters = requester.getParameterHandleValueMapFactory().create(1);
      parameters.put(federateParameter, encode(subject));
      requester.sendInteraction(requestClass, parameters, new byte[0]);
      await(requester, model, () -> recorder.interactions.size() >= 2, description);
      JavaTckSupport.check(recorder.interactions.size() == 2,
         description + " did not return exactly one report per standard transportation");
      return new ArrayList<>(recorder.interactions);
   }

   private static void verifyReports(EncoderFactory encoder, List<Interaction> reports,
         InteractionClassHandle reportClass, ParameterHandle transportationParameter,
         ParameterHandle countParameter, TransportationTypeHandle reliable,
         TransportationTypeHandle bestEffort, TransportationTypeHandle reportReliable,
         byte[] expectedClass, boolean expectCounts, String description) throws Exception {
      JavaTckSupport.check(reports.size() == 2,
         description + " did not return exactly one report for each standard transportation");
      boolean sawReliable = false, sawBestEffort = false;
      for (Interaction report : reports) {
         JavaTckSupport.check(reportClass.equals(report.interaction) && report.parameters.size() == 2
               && report.parameters.containsKey(transportationParameter)
               && report.parameters.containsKey(countParameter) && report.tag.length == 0
               && report.producer == null && (report.regions == null || report.regions.isEmpty())
               && reportReliable.equals(report.transportation),
            description + " returned non-standard MOM report metadata");
         byte[] transport = report.parameters.get(transportationParameter);
         if (Arrays.equals(transport, encode(reliable))) {
            JavaTckSupport.check(!sawReliable, description + " duplicated the reliable report");
            sawReliable = true;
            verifyCountArray(encoder, report.parameters.get(countParameter), expectedClass,
               expectCounts ? 1 : 0, description + " reliable class count");
         } else if (Arrays.equals(transport, encode(bestEffort))) {
            JavaTckSupport.check(!sawBestEffort,
               description + " duplicated the best-effort report");
            sawBestEffort = true;
            verifyCountArray(encoder, report.parameters.get(countParameter), expectedClass,
               expectCounts ? 2 : 0, description + " best-effort class count");
         } else {
            throw new AssertionError(description + " reported an unknown transportation type");
         }
      }
      JavaTckSupport.check(sawReliable && sawBestEffort,
         description + " omitted a standard transportation bucket");
   }

   private static void verifyCountArray(EncoderFactory encoder, byte[] encoded,
         byte[] expectedClass, int expectedCount, String description) throws Exception {
      HLAfixedRecord prototype = encoder.createHLAfixedRecord();
      prototype.add(encoder.createHLAvariableArray(index -> encoder.createHLAbyte(),
         encoder.createHLAbyte()));
      prototype.add(encoder.createHLAinteger32BE());
      HLAvariableArray<HLAfixedRecord> counts = encoder.createHLAvariableArray(index -> {
         HLAfixedRecord record = encoder.createHLAfixedRecord();
         record.add(encoder.createHLAvariableArray(inner -> encoder.createHLAbyte(),
            encoder.createHLAbyte()));
         record.add(encoder.createHLAinteger32BE());
         return record;
      }, prototype);
      if (encoded.length != 0) counts.decode(encoded);
      if (expectedCount == 0) {
         JavaTckSupport.check(counts.size() == 0,
            description + " encoded a non-empty list for a NULL response");
         return;
      }
      JavaTckSupport.check(counts.size() == 1,
         description + " returned the wrong number of positive class counts");
      HLAfixedRecord record = counts.get(0);
      @SuppressWarnings("unchecked")
      HLAvariableArray<HLAbyte> encodedClass = (HLAvariableArray<HLAbyte>) record.get(0);
      HLAinteger32BE count = (HLAinteger32BE) record.get(1);
      JavaTckSupport.check(Arrays.equals(expectedClass, encodedClass.toByteArray())
            && count.getValue() == expectedCount,
         description + " returned the wrong object class or update count");
   }

   private static byte[] encode(ObjectClassHandle handle) throws Exception {
      byte[] bytes = new byte[handle.encodedLength()];
      handle.encode(bytes, 0);
      return bytes;
   }

   private static byte[] encode(FederateHandle handle) throws Exception {
      byte[] bytes = new byte[handle.encodedLength()];
      handle.encode(bytes, 0);
      return bytes;
   }

   private static byte[] encode(TransportationTypeHandle handle) throws Exception {
      byte[] bytes = new byte[handle.encodedLength()];
      handle.encode(bytes, 0);
      return bytes;
   }

   private static void await(RTIambassador rti, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         if (model == CallbackModel.HLA_EVOKED) rti.evokeCallback(0.0);
         else Thread.yield();
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void resign(RTIambassador rti) {
      try { rti.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador rti) {
      try { rti.disconnect(); } catch (Exception ignored) { }
   }

   private static final class TransportConfirmation {
      private final ObjectInstanceHandle object;
      private final AttributeHandleSet attributes;
      private final TransportationTypeHandle transportation;
      private TransportConfirmation(ObjectInstanceHandle object, AttributeHandleSet attributes,
            TransportationTypeHandle transportation) {
         this.object = object;
         this.attributes = (AttributeHandleSet) attributes.clone();
         this.transportation = transportation;
      }
   }

   private static final class Interaction {
      private final InteractionClassHandle interaction;
      private final ParameterHandleValueMap parameters;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final RegionHandleSet regions;
      private Interaction(InteractionClassHandle interaction, ParameterHandleValueMap parameters,
            byte[] tag, TransportationTypeHandle transportation, FederateHandle producer,
            RegionHandleSet regions) {
         this.interaction = interaction;
         this.parameters = parameters.clone();
         this.tag = tag.clone();
         this.transportation = transportation;
         this.producer = producer;
         this.regions = regions == null ? null : (RegionHandleSet) regions.clone();
      }
   }

   private static final class Recorder {
      private final List<TransportConfirmation> transportConfirmations =
         new CopyOnWriteArrayList<>();
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();
      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("confirmAttributeTransportationTypeChange".equals(name)
                     && arguments != null && arguments.length == 3) {
                  transportConfirmations.add(new TransportConfirmation(
                     (ObjectInstanceHandle) arguments[0], (AttributeHandleSet) arguments[1],
                     (TransportationTypeHandle) arguments[2]));
               } else if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length == 6) {
                  interactions.add(new Interaction((InteractionClassHandle) arguments[0],
                     (ParameterHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     (RegionHandleSet) arguments[5]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK updates-sent count callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
