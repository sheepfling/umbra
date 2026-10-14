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
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies per-transport reflection MOM counts and the empty-subject report path. */
final class FederateMomReflectionsReceivedCountsTck {
   private static final String REQUEST_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestReflectionsReceived";
   private static final String REPORT_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportReflectionsReceived";
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private FederateMomReflectionsReceivedCountsTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador receiver = factory.getRtiAmbassador();
      RTIambassador sender = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder receiverRecorder = new Recorder();
      Recorder senderRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      String federation = "java-tck-federate-mom-reflections-received-counts-"
         + UUID.randomUUID();
      boolean receiverConnected = false, senderConnected = false, requesterConnected = false;
      boolean created = false, receiverJoined = false, senderJoined = false, requesterJoined = false;
      boolean senderPublished = false, receiverSubscribed = false, reportSubscribed = false;
      ObjectClassHandle receiverClass = null, senderClass = null;
      AttributeHandle receiverAttribute = null, senderAttribute = null;
      InteractionClassHandle reportClass = null;
      try {
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         sender.connect(senderRecorder.proxy(), callbackModel);
         senderConnected = true;
         requester.connect(requesterRecorder.proxy(), callbackModel);
         requesterConnected = true;
         receiver.createFederationExecutionWithMIM(federation, new String[] {fom}, mim,
            timeImplementation);
         created = true;
         FederateHandle receiverFederate = receiver.joinFederationExecution(
            "mom-reflections-received-target", "java-tck", federation);
         receiverJoined = true;
         FederateHandle senderFederate = sender.joinFederationExecution(
            "mom-reflections-received-sender", "java-tck", federation);
         senderJoined = true;
         FederateHandle requesterFederate = requester.joinFederationExecution(
            "mom-reflections-received-requester", "java-tck", federation);
         requesterJoined = true;

         receiverClass = receiver.getObjectClassHandle(objectClassName);
         receiverAttribute = receiver.getAttributeHandle(receiverClass, attributeName);
         senderClass = sender.getObjectClassHandle(objectClassName);
         senderAttribute = sender.getAttributeHandle(senderClass, attributeName);
         TransportationTypeHandle reliable = sender.getTransportationTypeHandle("HLAreliable");
         TransportationTypeHandle bestEffort = sender.getTransportationTypeHandle("HLAbestEffort");
         JavaTckSupport.check(objectClassName.equals(receiver.getObjectClassName(receiverClass))
               && objectClassName.equals(sender.getObjectClassName(senderClass))
               && attributeName.equals(receiver.getAttributeName(receiverClass, receiverAttribute))
               && attributeName.equals(sender.getAttributeName(senderClass, senderAttribute))
               && receiverClass.equals(senderClass) && receiverAttribute.equals(senderAttribute)
               && "HLAreliable".equals(sender.getTransportationTypeName(reliable))
               && "HLAbestEffort".equals(sender.getTransportationTypeName(bestEffort)),
            "adapter object/attribute handles or standard transportation handles did not round-trip");

         sender.publishObjectClassAttributes(senderClass,
            JavaTckSupport.attributeSet(sender, senderAttribute));
         senderPublished = true;
         receiver.subscribeObjectClassAttributes(receiverClass,
            JavaTckSupport.attributeSet(receiver, receiverAttribute), "");
         receiverSubscribed = true;
         ObjectInstanceHandle object = sender.registerObjectInstance(senderClass);
         await(receiver, callbackModel, () -> receiverRecorder.discoveries.contains(object),
            "reflection target discovery");
         JavaTckSupport.check(receiverClass.equals(receiver.getKnownObjectClassHandle(object)),
            "reflection target discovery associated the wrong object class");

         update(sender, object, senderAttribute, (byte) 0x71);
         await(receiver, callbackModel, () -> receiverRecorder.reflections.size() >= 1,
            "initial reliable reflection");
         verifyReflection(receiverRecorder.reflections.get(0), object, receiverAttribute,
            senderFederate, reliable, (byte) 0x71);

         int confirmationsBefore = senderRecorder.transportConfirmations.size();
         sender.requestAttributeTransportationTypeChange(object,
            JavaTckSupport.attributeSet(sender, senderAttribute), bestEffort);
         await(sender, callbackModel,
            () -> senderRecorder.transportConfirmations.size() > confirmationsBefore,
            "best-effort attribute transportation confirmation");
         AttributeTransportConfirmation confirmation = senderRecorder.transportConfirmations.get(
            senderRecorder.transportConfirmations.size() - 1);
         JavaTckSupport.check(object.equals(confirmation.object)
               && confirmation.attributes.size() == 1
               && confirmation.attributes.contains(senderAttribute)
               && bestEffort.equals(confirmation.transportation),
            "attribute transportation change did not confirm the target and best-effort type");

         update(sender, object, senderAttribute, (byte) 0x72);
         await(receiver, callbackModel, () -> receiverRecorder.reflections.size() >= 2,
            "first best-effort reflection");
         update(sender, object, senderAttribute, (byte) 0x73);
         await(receiver, callbackModel, () -> receiverRecorder.reflections.size() >= 3,
            "second best-effort reflection");
         JavaTckSupport.check(receiverRecorder.reflections.size() == 3,
            "receiver observed an unexpected number of attribute reflections");
         verifyReflection(receiverRecorder.reflections.get(1), object, receiverAttribute,
            senderFederate, bestEffort, (byte) 0x72);
         verifyReflection(receiverRecorder.reflections.get(2), object, receiverAttribute,
            senderFederate, bestEffort, (byte) 0x73);

         reportClass = requester.getInteractionClassHandle(REPORT_CLASS);
         InteractionClassHandle requestClass = requester.getInteractionClassHandle(REQUEST_CLASS);
         ParameterHandle requestFederate = requester.getParameterHandle(requestClass, "HLAfederate");
         ParameterHandle reportTransportation = requester.getParameterHandle(reportClass,
            "HLAtransportation");
         ParameterHandle reportCounts = requester.getParameterHandle(reportClass, "HLAreflectCounts");
         TransportationTypeHandle reportReliable =
            requester.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(REPORT_CLASS.equals(requester.getInteractionClassName(reportClass))
               && REQUEST_CLASS.equals(requester.getInteractionClassName(requestClass))
               && "HLAfederate".equals(requester.getParameterName(requestClass, requestFederate))
               && "HLAtransportation".equals(requester.getParameterName(reportClass,
                  reportTransportation))
               && "HLAreflectCounts".equals(requester.getParameterName(reportClass, reportCounts))
               && "HLAreliable".equals(requester.getTransportationTypeName(reportReliable)),
            "standard reflections-received MOM request/report handles did not round-trip");
         requester.subscribeInteractionClass(reportClass);
         reportSubscribed = true;
         EncoderFactory encoder = factory.getEncoderFactory();
         List<MomInteraction> receiverReports = requestReports(requester, requesterRecorder,
            callbackModel, requestClass, requestFederate, reportClass, reportCounts,
            encode(receiverFederate), "receiver reflections-received MOM reports");
         verifyReports(receiverReports, reportClass, reportTransportation, reportCounts,
            reportReliable, reliable, bestEffort, receiverClass, encoder, 1, 2, "receiver");
         List<MomInteraction> requesterReports = requestReports(requester, requesterRecorder,
            callbackModel, requestClass, requestFederate, reportClass, reportCounts,
            encode(requesterFederate), "requester empty reflections-received MOM reports");
         verifyReports(requesterReports, reportClass, reportTransportation, reportCounts,
            reportReliable, reliable, bestEffort, receiverClass, encoder, 0, 0,
            "empty requester");

         requester.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         receiver.unsubscribeObjectClassAttributes(receiverClass,
            JavaTckSupport.attributeSet(receiver, receiverAttribute));
         receiverSubscribed = false;
         sender.unpublishObjectClassAttributes(senderClass,
            JavaTckSupport.attributeSet(sender, senderAttribute));
         senderPublished = false;
         requester.resignFederationExecution(ResignAction.NO_ACTION);
         requesterJoined = false;
         sender.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         senderJoined = false;
         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         receiver.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (reportSubscribed && requesterJoined && reportClass != null) try {
            requester.unsubscribeInteractionClass(reportClass);
         } catch (Exception ignored) { }
         if (receiverSubscribed && receiverJoined && receiverClass != null
               && receiverAttribute != null) try {
            receiver.unsubscribeObjectClassAttributes(receiverClass,
               JavaTckSupport.attributeSet(receiver, receiverAttribute));
         } catch (Exception ignored) { }
         if (senderPublished && senderJoined && senderClass != null && senderAttribute != null) try {
            sender.unpublishObjectClassAttributes(senderClass,
               JavaTckSupport.attributeSet(sender, senderAttribute));
         } catch (Exception ignored) { }
         if (requesterJoined) resign(requester, ResignAction.NO_ACTION);
         if (senderJoined) resign(sender, ResignAction.DELETE_OBJECTS);
         if (receiverJoined) resign(receiver, ResignAction.NO_ACTION);
         if (created) try { receiver.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (requesterConnected) disconnect(requester);
         if (senderConnected) disconnect(sender);
         if (receiverConnected) disconnect(receiver);
      }
   }

   private static void update(RTIambassador sender, ObjectInstanceHandle object,
         AttributeHandle attribute, byte value) throws Exception {
      AttributeHandleValueMap values = sender.getAttributeHandleValueMapFactory().create(1);
      values.put(attribute, new byte[] {value});
      sender.updateAttributeValues(object, values, new byte[0]);
   }

   private static void verifyReflection(Reflection reflection, ObjectInstanceHandle object,
         AttributeHandle attribute, FederateHandle producer, TransportationTypeHandle transportation,
         byte value) {
      JavaTckSupport.check(object.equals(reflection.object) && reflection.values.size() == 1
            && reflection.values.containsKey(attribute)
            && Arrays.equals(reflection.values.get(attribute), new byte[] {value})
            && reflection.tag.length == 0 && transportation.equals(reflection.transportation)
            && producer.equals(reflection.producer)
            && (reflection.regions == null || reflection.regions.isEmpty()),
         "application reflection returned non-standard class, value, or callback metadata");
   }

   private static List<MomInteraction> requestReports(RTIambassador requester, Recorder recorder,
         CallbackModel callbackModel, InteractionClassHandle requestClass,
         ParameterHandle requestFederate, InteractionClassHandle reportClass,
         ParameterHandle reportCounts, byte[] federate, String description) throws Exception {
      recorder.reports.clear();
      ParameterHandleValueMap request = requester.getParameterHandleValueMapFactory().create(1);
      request.put(requestFederate, federate);
      requester.sendInteraction(requestClass, request, new byte[0]);
      await(requester, callbackModel,
         () -> recorder.reports.stream().filter(i -> reportClass.equals(i.interaction)
            && i.parameters.containsKey(reportCounts)).count() >= 2,
         description);
      List<MomInteraction> result = new ArrayList<>();
      for (MomInteraction interaction : recorder.reports)
         if (reportClass.equals(interaction.interaction)) result.add(interaction);
      return result;
   }

   private static void verifyReports(List<MomInteraction> reports,
         InteractionClassHandle reportClass, ParameterHandle reportTransportation,
         ParameterHandle reportCounts, TransportationTypeHandle reportReliable,
         TransportationTypeHandle reliable, TransportationTypeHandle bestEffort,
         ObjectClassHandle expectedClass, EncoderFactory encoder, int expectedReliableCount,
         int expectedBestEffortCount, String description) throws Exception {
      JavaTckSupport.check(reports.size() == 2,
         description + " did not return one report for each standard transportation type");
      boolean sawReliable = false, sawBestEffort = false;
      for (MomInteraction report : reports) {
         JavaTckSupport.check(reportClass.equals(report.interaction) && report.parameters.size() == 2
               && report.parameters.containsKey(reportTransportation)
               && report.parameters.containsKey(reportCounts) && report.tag.length == 0
               && report.producer == null && report.regionsEmpty
               && reportReliable.equals(report.transportation),
            description + " returned non-standard MOM report metadata");
         byte[] transport = report.parameters.get(reportTransportation);
         if (Arrays.equals(transport, encode(reliable))) {
            JavaTckSupport.check(!sawReliable, description + " returned duplicate reliable buckets");
            sawReliable = true;
            verifyCountArray(encoder, report.parameters.get(reportCounts), expectedClass,
               expectedReliableCount, description + " reliable reflection count");
         } else if (Arrays.equals(transport, encode(bestEffort))) {
            JavaTckSupport.check(!sawBestEffort,
               description + " returned duplicate best-effort buckets");
            sawBestEffort = true;
            verifyCountArray(encoder, report.parameters.get(reportCounts), expectedClass,
               expectedBestEffortCount, description + " best-effort reflection count");
         } else throw new AssertionError(description + " returned an unknown transport bucket");
      }
      JavaTckSupport.check(sawReliable && sawBestEffort,
         description + " omitted one of the standard transportation buckets");
   }

   private static void verifyCountArray(EncoderFactory encoder, byte[] encoded,
         ObjectClassHandle expectedClass, int expectedCount, String description) throws Exception {
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
            description + " encoded a non-empty list for a NULL report");
         return;
      }
      JavaTckSupport.check(counts.size() == 1,
         description + " returned the wrong number of object-class count rows");
      HLAfixedRecord row = counts.get(0);
      @SuppressWarnings("unchecked")
      HLAvariableArray<HLAbyte> classBytes = (HLAvariableArray<HLAbyte>) row.get(0);
      JavaTckSupport.check(Arrays.equals(encode(expectedClass), classBytes.toByteArray())
            && ((HLAinteger32BE) row.get(1)).getValue() == expectedCount,
         description + " returned the wrong class encoding or reflection count");
   }

   private static byte[] encode(FederateHandle handle) throws Exception {
      byte[] bytes = new byte[handle.encodedLength()];
      handle.encode(bytes, 0);
      return bytes;
   }
   private static byte[] encode(ObjectClassHandle handle) throws Exception {
      byte[] bytes = new byte[handle.encodedLength()];
      handle.encode(bytes, 0);
      return bytes;
   }
   private static byte[] encode(TransportationTypeHandle handle) throws Exception {
      byte[] bytes = new byte[handle.encodedLength()];
      handle.encode(bytes, 0);
      return bytes;
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         if (model == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.0);
         else Thread.yield();
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void resign(RTIambassador rti, ResignAction action) {
      try { rti.resignFederationExecution(action); } catch (Exception ignored) { }
   }
   private static void disconnect(RTIambassador rti) {
      try { rti.disconnect(); } catch (Exception ignored) { }
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final AttributeHandleValueMap values;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final RegionHandleSet regions;
      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap values, byte[] tag,
            TransportationTypeHandle transportation, FederateHandle producer,
            RegionHandleSet regions) {
         this.object = object;
         this.values = values.clone();
         this.tag = tag.clone();
         this.transportation = transportation;
         this.producer = producer;
         this.regions = regions == null ? null : (RegionHandleSet) regions.clone();
      }
   }
   private static final class MomInteraction {
      private final InteractionClassHandle interaction;
      private final Map<ParameterHandle, byte[]> parameters = new LinkedHashMap<>();
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final boolean regionsEmpty;
      private MomInteraction(InteractionClassHandle interaction, ParameterHandleValueMap values,
            byte[] tag, TransportationTypeHandle transportation, FederateHandle producer,
            RegionHandleSet regions) {
         this.interaction = interaction;
         for (ParameterHandle parameter : values.keySet())
            parameters.put(parameter, values.get(parameter).clone());
         this.tag = tag.clone();
         this.transportation = transportation;
         this.producer = producer;
         this.regionsEmpty = regions == null || regions.isEmpty();
      }
   }
   private static final class AttributeTransportConfirmation {
      private final ObjectInstanceHandle object;
      private final AttributeHandleSet attributes;
      private final TransportationTypeHandle transportation;
      private AttributeTransportConfirmation(ObjectInstanceHandle object,
            AttributeHandleSet attributes, TransportationTypeHandle transportation) {
         this.object = object;
         this.attributes = (AttributeHandleSet) attributes.clone();
         this.transportation = transportation;
      }
   }
   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<MomInteraction> reports = new CopyOnWriteArrayList<>();
      private final List<AttributeTransportConfirmation> transportConfirmations =
         new CopyOnWriteArrayList<>();
      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length == 4) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 6) {
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     (RegionHandleSet) arguments[5]));
               } else if ("confirmAttributeTransportationTypeChange".equals(name)
                     && arguments != null && arguments.length == 3) {
                  transportConfirmations.add(new AttributeTransportConfirmation(
                     (ObjectInstanceHandle) arguments[0], (AttributeHandleSet) arguments[1],
                     (TransportationTypeHandle) arguments[2]));
               } else if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 6) {
                  reports.add(new MomInteraction((InteractionClassHandle) arguments[0],
                     (ParameterHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     (RegionHandleSet) arguments[5]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK reflection-count callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
