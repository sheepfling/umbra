package org.hla.rti.tck;

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

/** Verifies class-scoped HLArequestObjectsToDelete reports using the standard encoder API. */
final class FederateMomDeletableObjectReportTck {
   private static final String REQUEST_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstancesThatCanBeDeleted";
   private static final String REPORT_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectInstancesThatCanBeDeleted";
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private FederateMomDeletableObjectReportTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador requester = factory.getRtiAmbassador();
      RTIambassador owner = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder requesterRecorder = new Recorder();
      Recorder ownerRecorder = new Recorder();
      String federation = "java-tck-federate-mom-deletable-object-report-" + UUID.randomUUID();
      boolean requesterConnected = false, ownerConnected = false, created = false;
      boolean requesterJoined = false, ownerJoined = false, subscribed = false, published = false;
      InteractionClassHandle requestClass = null, reportClass = null;
      ObjectClassHandle ownerObjectClass = null;
      hla.rti1516_2025.AttributeHandleSet ownerAttributes = null;
      try {
         requester.connect(requesterRecorder.proxy(), callbackModel);
         requesterConnected = true;
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         owner.createFederationExecutionWithMIM(federation, new String[] {fom}, mim,
            timeImplementation);
         created = true;
         requester.joinFederationExecution("mom-deletable-requester", "java-tck", federation);
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
            "standard deletable-object MOM request/report handles did not round-trip");
         requester.subscribeInteractionClass(reportClass);
         subscribed = true;

         FederateHandle ownerFederate = owner.joinFederationExecution(
            "mom-deletable-owner", "java-tck", federation);
         ownerJoined = true;
         ownerObjectClass = owner.getObjectClassHandle(objectClassName);
         hla.rti1516_2025.AttributeHandle ownerAttribute =
            owner.getAttributeHandle(ownerObjectClass, attributeName);
         JavaTckSupport.check(objectClassName.equals(owner.getObjectClassName(ownerObjectClass))
               && attributeName.equals(owner.getAttributeName(ownerObjectClass, ownerAttribute)),
            "adapter-supplied object class or attribute lookup did not round-trip");
         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         owner.publishObjectClassAttributes(ownerObjectClass, ownerAttributes);
         published = true;

         ObjectInstanceHandle firstObject = owner.registerObjectInstance(ownerObjectClass);
         ObjectInstanceHandle secondObject = owner.registerObjectInstance(ownerObjectClass);
         JavaTckSupport.check(firstObject != null && secondObject != null
               && !firstObject.equals(secondObject),
            "deletable-object registrations returned invalid or duplicate handles");
         requestCount(requester, requesterRecorder, callbackModel, requestClass, requestFederate,
            reportClass, reportCounts, reliable, ownerFederate, ownerObjectClass,
            encoder, 2, "report with two live objects");
         owner.deleteObjectInstance(firstObject, new byte[0]);
         requestCount(requester, requesterRecorder, callbackModel, requestClass, requestFederate,
            reportClass, reportCounts, reliable, ownerFederate, ownerObjectClass,
            encoder, 1, "report after one deletion");
         owner.deleteObjectInstance(secondObject, new byte[0]);
         requestCount(requester, requesterRecorder, callbackModel, requestClass, requestFederate,
            reportClass, reportCounts, reliable, ownerFederate, ownerObjectClass,
            encoder, 0, "report after all deletions");

         requester.unsubscribeInteractionClass(reportClass);
         subscribed = false;
         owner.unpublishObjectClassAttributes(ownerObjectClass, ownerAttributes);
         published = false;
         requester.resignFederationExecution(ResignAction.NO_ACTION);
         requesterJoined = false;
         owner.resignFederationExecution(ResignAction.NO_ACTION);
         ownerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (subscribed && requesterJoined && reportClass != null) try {
            requester.unsubscribeInteractionClass(reportClass);
         } catch (Exception ignored) { }
         if (published && ownerJoined && ownerObjectClass != null && ownerAttributes != null) try {
            owner.unpublishObjectClassAttributes(ownerObjectClass, ownerAttributes);
         } catch (Exception ignored) { }
         if (requesterJoined) resign(requester);
         if (ownerJoined) resign(owner);
         if (created) try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (requesterConnected) disconnect(requester);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static void requestCount(RTIambassador requester, Recorder recorder,
         CallbackModel model, InteractionClassHandle requestClass, ParameterHandle requestFederate,
         InteractionClassHandle reportClass, ParameterHandle reportCounts,
         TransportationTypeHandle reliable, FederateHandle target, ObjectClassHandle expectedClass,
         EncoderFactory encoder, int expected, String description) throws Exception {
      int first = recorder.interactions.size();
      ParameterHandleValueMap parameters = requester.getParameterHandleValueMapFactory().create(1);
      parameters.put(requestFederate, encode(target));
      requester.sendInteraction(requestClass, parameters, new byte[0]);
      await(requester, model,
         () -> findReport(recorder, first, reportClass, reportCounts) != null, description);
      Interaction report = findReport(recorder, first, reportClass, reportCounts);
      JavaTckSupport.check(report != null && report.parameters.size() == 1
            && report.parameters.containsKey(reportCounts)
            && reliable.equals(report.transportation) && report.tag.length == 0
            && report.producer == null && report.regionsEmpty,
         description + " returned a missing report or non-standard callback metadata");
      JavaTckSupport.check(decodeObjectClassCount(encoder, requester.getObjectClassHandleFactory(),
            report.parameters.get(reportCounts), expectedClass) == expected,
         description + " returned the wrong class-specific object count");
   }

   private static int decodeObjectClassCount(EncoderFactory encoder,
         ObjectClassHandleFactory classFactory, byte[] encoded, ObjectClassHandle expectedClass)
         throws Exception {
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
      for (HLAfixedRecord record : records) {
         @SuppressWarnings("unchecked")
         HLAvariableArray<HLAoctet> handle = (HLAvariableArray<HLAoctet>) record.get(0);
         HLAinteger32BE count = (HLAinteger32BE) record.get(1);
         ObjectClassHandle decodedClass = classFactory.decode(handle.toByteArray(), 0);
         if (expectedClass.equals(decodedClass)) return count.getValue();
      }
      return 0;
   }

   private static Interaction findReport(Recorder recorder, int first,
         InteractionClassHandle reportClass, ParameterHandle reportCounts) {
      for (int i = Math.min(first, recorder.interactions.size()); i < recorder.interactions.size(); ++i) {
         Interaction interaction = recorder.interactions.get(i);
         if (interaction.present && reportClass.equals(interaction.interaction)
               && interaction.parameters.containsKey(reportCounts)) return interaction;
      }
      return null;
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

   private static final class Interaction {
      private final InteractionClassHandle interaction;
      private final Map<ParameterHandle, byte[]> parameters;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final boolean regionsEmpty;
      private final boolean present;
      private Interaction(InteractionClassHandle interaction,
            ParameterHandleValueMap values, byte[] tag, TransportationTypeHandle transportation,
            FederateHandle producer, RegionHandleSet regions) {
         this.interaction = interaction;
         this.parameters = new LinkedHashMap<>();
         for (ParameterHandle parameter : values.keySet())
            this.parameters.put(parameter, values.get(parameter).clone());
         this.tag = tag.clone();
         this.transportation = transportation;
         this.producer = producer;
         this.regionsEmpty = regions == null || regions.isEmpty();
         this.present = true;
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
                  if ("toString".equals(name)) return "Java TCK deletable-object MOM reports";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
