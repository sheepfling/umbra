package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectClassHandleFactory;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAfixedRecord;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.encoding.HLAoctet;
import hla.rti1516_2025.encoding.HLAvariableArray;
import java.lang.reflect.Proxy;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies distinct updated-object counts in the standard class-scoped MOM report. */
final class FederateMomUpdatedObjectReportTck {
   private static final String REQUEST_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstancesUpdated";
   private static final String REPORT_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectInstancesUpdated";
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private FederateMomUpdatedObjectReportTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador requester = factory.getRtiAmbassador();
      RTIambassador subject = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder requesterRecorder = new Recorder();
      Recorder subjectRecorder = new Recorder();
      String federation = "java-tck-federate-mom-updated-object-report-" + UUID.randomUUID();
      boolean requesterConnected = false, subjectConnected = false, created = false;
      boolean requesterJoined = false, subjectJoined = false, subscribed = false, published = false;
      InteractionClassHandle requestClass = null, reportClass = null;
      ObjectClassHandle subjectObjectClass = null;
      AttributeHandle subjectAttribute = null;
      try {
         requester.connect(requesterRecorder.proxy(), callbackModel);
         requesterConnected = true;
         subject.connect(subjectRecorder.proxy(), callbackModel);
         subjectConnected = true;
         subject.createFederationExecutionWithMIM(federation, new String[] {fom}, mim,
            timeImplementation);
         created = true;
         FederateHandle subjectFederate = subject.joinFederationExecution(
            "mom-updated-subject", "java-tck", federation);
         subjectJoined = true;
         requester.joinFederationExecution("mom-updated-requester", "java-tck", federation);
         requesterJoined = true;

         requestClass = requester.getInteractionClassHandle(REQUEST_CLASS);
         ParameterHandle requestFederate = requester.getParameterHandle(requestClass, "HLAfederate");
         reportClass = requester.getInteractionClassHandle(REPORT_CLASS);
         ParameterHandle reportCounts = requester.getParameterHandle(reportClass,
            "HLAobjectInstanceCounts");
         TransportationTypeHandle reliable = requester.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(REQUEST_CLASS.equals(requester.getInteractionClassName(requestClass))
               && "HLAfederate".equals(requester.getParameterName(requestClass, requestFederate))
               && REPORT_CLASS.equals(requester.getInteractionClassName(reportClass))
               && "HLAobjectInstanceCounts".equals(requester.getParameterName(reportClass,
                  reportCounts))
               && "HLAreliable".equals(requester.getTransportationTypeName(reliable)),
            "standard updated-object MOM request/report handles did not round-trip");
         InteractionClassHandle resolvedReportClass = reportClass;
         requester.subscribeInteractionClass(reportClass);
         subscribed = true;

         subjectObjectClass = subject.getObjectClassHandle(objectClassName);
         subjectAttribute = subject.getAttributeHandle(subjectObjectClass, attributeName);
         ObjectClassHandle requesterObjectClass = requester.getObjectClassHandle(objectClassName);
         JavaTckSupport.check(objectClassName.equals(subject.getObjectClassName(subjectObjectClass))
               && objectClassName.equals(requester.getObjectClassName(requesterObjectClass))
               && attributeName.equals(subject.getAttributeName(subjectObjectClass, subjectAttribute)),
            "adapter-supplied object-class and attribute lookup did not round-trip");
         subject.publishObjectClassAttributes(subjectObjectClass,
            JavaTckSupport.attributeSet(subject, subjectAttribute));
         published = true;

         ObjectInstanceHandle firstObject = subject.registerObjectInstance(subjectObjectClass);
         ObjectInstanceHandle secondObject = subject.registerObjectInstance(subjectObjectClass);
         JavaTckSupport.check(firstObject != null && secondObject != null
               && !firstObject.equals(secondObject),
            "updated-object registration returned invalid or duplicate handles");
         update(subject, firstObject, subjectAttribute, encoder, (byte) 1);
         update(subject, firstObject, subjectAttribute, encoder, (byte) 2);
         update(subject, secondObject, subjectAttribute, encoder, (byte) 3);

         int firstReport = requesterRecorder.interactions.size();
         ParameterHandleValueMap requestParameters =
            requester.getParameterHandleValueMapFactory().create(1);
         requestParameters.put(requestFederate, encode(subjectFederate));
         requester.sendInteraction(requestClass, requestParameters, new byte[0]);
         await(requester, callbackModel,
            () -> findReport(requesterRecorder, firstReport, resolvedReportClass, reportCounts)
               != null,
            "joined-federate MOM updated-object report");
         List<Interaction> reports = reportsSince(requesterRecorder, firstReport,
            resolvedReportClass);
         JavaTckSupport.check(reports.size() == 1,
            "updated-object request did not deliver exactly one report");
         Interaction report = reports.get(0);
         JavaTckSupport.check(report.present && report.parameters.size() == 1
               && report.parameters.containsKey(reportCounts)
               && reliable.equals(report.transportation) && report.tag.length == 0
               && report.producer == null && report.regionsEmpty,
            "updated-object report had a non-standard reliable one-parameter response shape");
         List<ObjectClassCount> counts = decodeObjectClassCounts(encoder,
            requester.getObjectClassHandleFactory(), report.parameters.get(reportCounts));
         JavaTckSupport.check(counts.size() == 1
               && requesterObjectClass.equals(counts.get(0).objectClass)
               && counts.get(0).count == 2,
            "updated-object report did not count two distinct instances once by registered class");

         requester.unsubscribeInteractionClass(reportClass);
         subscribed = false;
         subject.deleteObjectInstance(firstObject, new byte[0]);
         subject.deleteObjectInstance(secondObject, new byte[0]);
         subject.unpublishObjectClassAttributes(subjectObjectClass,
            JavaTckSupport.attributeSet(subject, subjectAttribute));
         published = false;
         requester.resignFederationExecution(ResignAction.NO_ACTION);
         requesterJoined = false;
         subject.resignFederationExecution(ResignAction.NO_ACTION);
         subjectJoined = false;
         subject.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (subscribed && requesterJoined && reportClass != null) try {
            requester.unsubscribeInteractionClass(reportClass);
         } catch (Exception ignored) { }
         if (published && subjectJoined && subjectObjectClass != null && subjectAttribute != null) try {
            subject.unpublishObjectClassAttributes(subjectObjectClass,
               JavaTckSupport.attributeSet(subject, subjectAttribute));
         } catch (Exception ignored) { }
         if (requesterJoined) resign(requester);
         if (subjectJoined) resign(subject);
         if (created) try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (requesterConnected) disconnect(requester);
         if (subjectConnected) disconnect(subject);
      }
   }

   private static void update(RTIambassador subject, ObjectInstanceHandle object,
         AttributeHandle attribute, EncoderFactory encoder, byte value) throws Exception {
      AttributeHandleValueMap values = subject.getAttributeHandleValueMapFactory().create(1);
      values.put(attribute, encoder.createHLAbyte(value).toByteArray());
      subject.updateAttributeValues(object, values, new byte[0]);
   }

   private static List<ObjectClassCount> decodeObjectClassCounts(EncoderFactory encoder,
         ObjectClassHandleFactory classFactory, byte[] encoded) throws Exception {
      HLAvariableArray<HLAoctet> handlePrototype = encoder.createHLAvariableArray(
         index -> encoder.createHLAoctet(), encoder.createHLAoctet());
      HLAfixedRecord recordPrototype = encoder.createHLAfixedRecord();
      recordPrototype.add(handlePrototype);
      recordPrototype.add(encoder.createHLAinteger32BE());
      HLAvariableArray<HLAfixedRecord> records = encoder.createHLAvariableArray(
         index -> {
            HLAfixedRecord record = encoder.createHLAfixedRecord();
            record.add(encoder.createHLAvariableArray(
               inner -> encoder.createHLAoctet(), encoder.createHLAoctet()));
            record.add(encoder.createHLAinteger32BE());
            return record;
         }, recordPrototype);
      records.decode(encoded);
      List<ObjectClassCount> result = new java.util.ArrayList<>();
      for (HLAfixedRecord record : records) {
         @SuppressWarnings("unchecked")
         HLAvariableArray<HLAoctet> handle = (HLAvariableArray<HLAoctet>) record.get(0);
         HLAinteger32BE count = (HLAinteger32BE) record.get(1);
         result.add(new ObjectClassCount(classFactory.decode(handle.toByteArray(), 0),
            count.getValue()));
      }
      return result;
   }

   private static Interaction findReport(Recorder recorder, int first,
         InteractionClassHandle reportClass, ParameterHandle counts) {
      for (int i = Math.min(first, recorder.interactions.size()); i < recorder.interactions.size(); ++i) {
         Interaction interaction = recorder.interactions.get(i);
         if (interaction.present && reportClass.equals(interaction.interaction)
               && interaction.parameters.containsKey(counts)) return interaction;
      }
      return null;
   }

   private static List<Interaction> reportsSince(Recorder recorder, int first,
         InteractionClassHandle reportClass) {
      List<Interaction> reports = new java.util.ArrayList<>();
      for (int i = Math.min(first, recorder.interactions.size()); i < recorder.interactions.size(); ++i) {
         Interaction interaction = recorder.interactions.get(i);
         if (reportClass.equals(interaction.interaction)) reports.add(interaction);
      }
      return reports;
   }

   private static byte[] encode(FederateHandle handle) throws Exception {
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

   private static final class ObjectClassCount {
      private final ObjectClassHandle objectClass;
      private final int count;
      private ObjectClassCount(ObjectClassHandle objectClass, int count) {
         this.objectClass = objectClass;
         this.count = count;
      }
   }

   private static final class Interaction {
      private final InteractionClassHandle interaction;
      private final Map<ParameterHandle, byte[]> parameters = new LinkedHashMap<>();
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final boolean regionsEmpty;
      private final boolean present = true;
      private Interaction(InteractionClassHandle interaction, ParameterHandleValueMap values,
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

   private static final class Recorder {
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();
      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 6) {
                  interactions.add(new Interaction((InteractionClassHandle) arguments[0],
                     (ParameterHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     (RegionHandleSet) arguments[5]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK updated-object MOM reports";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
