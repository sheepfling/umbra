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
import hla.rti1516_2025.encoding.HLAunicodeString;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Exercises standard MIM requests to change object-attribute and interaction transportation. */
final class MomTransportationTypeChangeRequestTck {
   private static final String ATTRIBUTE_REQUEST =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice."
         + "HLArequestAttributeTransportationTypeChange";
   private static final String INTERACTION_REQUEST =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice."
         + "HLArequestInteractionTransportationTypeChange";
   private static final String SERVICE_REPORT =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private MomTransportationTypeChangeRequestTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         String parameterName, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(fom != null && !fom.isEmpty(),
         "adapter-supplied FOM is required");
      JavaTckSupport.check(mim != null && !mim.isEmpty(),
         "adapter-supplied standard MIM is required");

      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder subjectCallbacks = new Recorder();
      Recorder observerCallbacks = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-mom-transport-change-" + UUID.randomUUID();
      boolean subjectConnected = false;
      boolean observerConnected = false;
      boolean created = false;
      boolean subjectJoined = false;
      boolean observerJoined = false;
      boolean observerReportSubscribed = false;
      ObjectClassHandle subjectClass = null;
      AttributeHandle subjectAttribute = null;
      InteractionClassHandle subjectInteraction = null;
      try {
         subject.connect(subjectCallbacks.proxy(), callbackModel);
         subjectConnected = true;
         observer.connect(observerCallbacks.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle subjectFederate = subject.joinFederationExecution(
            "mom-transport-change-subject", "java-tck", federation);
         subjectJoined = true;
         FederateHandle observerFederate = observer.joinFederationExecution(
            "mom-transport-change-observer", "java-tck", federation);
         observerJoined = true;

         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);

         subjectClass = subject.getObjectClassHandle(objectClassName);
         ObjectClassHandle observerClass = observer.getObjectClassHandle(objectClassName);
         subjectAttribute = subject.getAttributeHandle(subjectClass, attributeName);
         AttributeHandle observerAttribute = observer.getAttributeHandle(
            observerClass, attributeName);
         subjectInteraction = subject.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle observerInteraction =
            observer.getInteractionClassHandle(interactionClassName);
         ParameterHandle subjectParameter = subject.getParameterHandle(
            subjectInteraction, parameterName);
         ParameterHandle observerParameter = observer.getParameterHandle(
            observerInteraction, parameterName);
         JavaTckSupport.check(objectClassName.equals(observer.getObjectClassName(observerClass))
               && attributeName.equals(observer.getAttributeName(observerClass, observerAttribute))
               && interactionClassName.equals(
                  observer.getInteractionClassName(observerInteraction))
               && parameterName.equals(observer.getParameterName(
                  observerInteraction, observerParameter)),
            "adapter FOM declarations did not round-trip through standard lookups");

         TransportationTypeHandle reliable = subject.getTransportationTypeHandle("HLAreliable");
         TransportationTypeHandle bestEffort =
            subject.getTransportationTypeHandle("HLAbestEffort");
         JavaTckSupport.check("HLAreliable".equals(subject.getTransportationTypeName(reliable))
               && "HLAbestEffort".equals(subject.getTransportationTypeName(bestEffort)),
            "standard reliable/best-effort transportation handles did not round-trip");

         AttributeHandleSet subjectAttributes =
            JavaTckSupport.attributeSet(subject, subjectAttribute);
         AttributeHandleSet observerAttributes =
            JavaTckSupport.attributeSet(observer, observerAttribute);
         subject.publishObjectClassAttributes(subjectClass, subjectAttributes);
         observer.subscribeObjectClassAttributes(observerClass, observerAttributes);
         subject.publishInteractionClass(subjectInteraction);
         subject.subscribeInteractionClass(subjectInteraction);
         observer.publishInteractionClass(observerInteraction);
         observer.subscribeInteractionClass(observerInteraction);

         ObjectInstanceHandle object = subject.registerObjectInstance(subjectClass);
         JavaTckSupport.check(object != null, "object registration returned no handle");
         await(() -> observerCallbacks.discoveries.contains(object), observer, callbackModel,
            "observer object discovery");

         byte[] baselineValue = new byte[] {0x42, 0x41};
         byte[] baselineTag = new byte[] {0x42};
         ParameterHandleValueMap baselineParameters =
            observer.getParameterHandleValueMapFactory().create(1);
         baselineParameters.put(observerParameter, baselineValue.clone());
         subjectCallbacks.clearInteractions();
         observer.sendInteraction(observerInteraction, baselineParameters, baselineTag.clone());
         await(() -> !subjectCallbacks.interactions.isEmpty(), subject, callbackModel,
            "reliable baseline interaction");
         Interaction baseline = subjectCallbacks.lastInteraction();
         JavaTckSupport.check(baseline.interaction.equals(subjectInteraction)
               && Arrays.equals(baseline.parameters.get(subjectParameter), baselineValue)
               && baseline.parameters.size() == 1
               && Arrays.equals(baseline.tag, baselineTag)
               && reliable.equals(baseline.transportation)
               && observerFederate.equals(baseline.producer) && baseline.regionsEmpty,
            "baseline interaction did not preserve reliable-delivery metadata");

         InteractionClassHandle attributeRequestClass =
            subject.getInteractionClassHandle(ATTRIBUTE_REQUEST);
         ParameterHandle requestObject =
            subject.getParameterHandle(attributeRequestClass, "HLAobjectInstance");
         ParameterHandle requestAttributes =
            subject.getParameterHandle(attributeRequestClass, "HLAattributeList");
         ParameterHandle requestTransportation =
            subject.getParameterHandle(attributeRequestClass, "HLAtransportation");
         requireHandle(attributeRequestClass, requestObject, requestAttributes,
            requestTransportation);
         subjectCallbacks.clearTransportConfirmations();
         ParameterHandleValueMap attributeRequest =
            subject.getParameterHandleValueMapFactory().create(3);
         attributeRequest.put(requestObject, encode(object.encodedLength(), object::encode));
         attributeRequest.put(requestAttributes, encodeAttributeList(subjectAttribute));
         attributeRequest.put(requestTransportation,
            encode(bestEffort.encodedLength(), bestEffort::encode));
         subject.sendInteraction(attributeRequestClass, attributeRequest, new byte[0]);
         await(() -> !subjectCallbacks.attributeConfirmations.isEmpty(), subject, callbackModel,
            "MOM attribute-transportation confirmation");
         List<AttributeTransportationConfirmation> attributeConfirmations =
            List.copyOf(subjectCallbacks.attributeConfirmations);
         JavaTckSupport.check(attributeConfirmations.size() == 1
               && object.equals(attributeConfirmations.get(0).object)
               && attributeConfirmations.get(0).attributes.size() == 1
               && attributeConfirmations.get(0).attributes.contains(subjectAttribute)
               && bestEffort.equals(attributeConfirmations.get(0).transportation),
            "MOM attribute-transportation confirmation did not commit best effort");

         byte[] updatedValue = new byte[] {(byte) 0xa1};
         AttributeHandleValueMap update =
            subject.getAttributeHandleValueMapFactory().create(1);
         update.put(subjectAttribute, updatedValue.clone());
         observerCallbacks.clearReflections();
         subject.updateAttributeValues(object, update, new byte[0]);
         await(() -> !observerCallbacks.reflections.isEmpty(), observer, callbackModel,
            "best-effort attribute reflection");
         Reflection reflection = observerCallbacks.lastReflection();
         JavaTckSupport.check(object.equals(reflection.object)
               && Arrays.equals(reflection.values.get(observerAttribute), updatedValue)
               && reflection.values.size() == 1 && reflection.tag.length == 0
               && bestEffort.equals(reflection.transportation)
               && subjectFederate.equals(reflection.producer) && reflection.regionsEmpty,
            "MOM attribute-transportation change did not affect update delivery");

         InteractionClassHandle interactionRequestClass =
            subject.getInteractionClassHandle(INTERACTION_REQUEST);
         ParameterHandle interactionTarget =
            subject.getParameterHandle(interactionRequestClass, "HLAinteractionClass");
         ParameterHandle interactionTransportation = subject.getParameterHandle(
            interactionRequestClass, "HLAtransportation");
         requireHandle(interactionRequestClass, interactionTarget, interactionTransportation);

         subjectCallbacks.clearInteractions();
         ParameterHandleValueMap independentBaseline =
            observer.getParameterHandleValueMapFactory().create(0);
         observer.sendInteraction(observerInteraction, independentBaseline, new byte[0]);
         await(() -> !subjectCallbacks.interactions.isEmpty(), subject, callbackModel,
            "independent publisher baseline interaction");
         JavaTckSupport.check(
            reliable.equals(subjectCallbacks.lastInteraction().transportation),
            "attribute transportation request changed an independent interaction publisher");

         subjectCallbacks.clearInteractionConfirmations();
         ParameterHandleValueMap interactionRequest =
            subject.getParameterHandleValueMapFactory().create(2);
         interactionRequest.put(interactionTarget,
            encode(subjectInteraction.encodedLength(), subjectInteraction::encode));
         interactionRequest.put(interactionTransportation,
            encode(bestEffort.encodedLength(), bestEffort::encode));
         subject.sendInteraction(interactionRequestClass, interactionRequest, new byte[0]);
         await(() -> !subjectCallbacks.interactionConfirmations.isEmpty(), subject, callbackModel,
            "MOM interaction-transportation confirmation");
         List<InteractionTransportationConfirmation> interactionConfirmations =
            List.copyOf(subjectCallbacks.interactionConfirmations);
         JavaTckSupport.check(interactionConfirmations.size() == 1
               && subjectInteraction.equals(interactionConfirmations.get(0).interaction)
               && bestEffort.equals(interactionConfirmations.get(0).transportation),
            "MOM interaction-transportation confirmation did not commit best effort");

         byte[] changedTag = new byte[] {0x54};
         observerCallbacks.clearInteractions();
         subject.sendInteraction(subjectInteraction,
            subject.getParameterHandleValueMapFactory().create(0), changedTag.clone());
         await(() -> !observerCallbacks.interactions.isEmpty(), observer, callbackModel,
            "best-effort interaction delivery");
         Interaction changedInteraction = observerCallbacks.lastInteraction();
         JavaTckSupport.check(observerInteraction.equals(changedInteraction.interaction)
               && bestEffort.equals(changedInteraction.transportation)
               && Arrays.equals(changedInteraction.tag, changedTag)
               && changedInteraction.parameters.isEmpty()
               && subjectFederate.equals(changedInteraction.producer)
               && changedInteraction.regionsEmpty,
            "MOM interaction-transportation change did not affect delivery");

         subjectCallbacks.clearInteractions();
         observer.sendInteraction(observerInteraction,
            observer.getParameterHandleValueMapFactory().create(0), new byte[0]);
         await(() -> !subjectCallbacks.interactions.isEmpty(), subject, callbackModel,
            "independent publisher post-change interaction");
         JavaTckSupport.check(
            reliable.equals(subjectCallbacks.lastInteraction().transportation),
            "MOM interaction transportation request changed an independent publisher");

         InteractionClassHandle reportClass = observer.getInteractionClassHandle(SERVICE_REPORT);
         ParameterHandle reportService = observer.getParameterHandle(reportClass, "HLAservice");
         JavaTckSupport.check(reportClass != null && reportService != null,
            "standard service-invocation report handles could not be resolved");
         observer.subscribeInteractionClass(reportClass);
         observerReportSubscribed = true;
         subject.setServiceReportingSwitch(true);
         observerCallbacks.clearInteractions();
         ParameterHandleValueMap reportedRequest =
            subject.getParameterHandleValueMapFactory().create(2);
         reportedRequest.put(interactionTarget,
            encode(subjectInteraction.encodedLength(), subjectInteraction::encode));
         reportedRequest.put(interactionTransportation,
            encode(reliable.encodedLength(), reliable::encode));
         subject.sendInteraction(interactionRequestClass, reportedRequest, new byte[0]);
         await(() -> !observerCallbacks.interactions.isEmpty(), observer, callbackModel,
            "standard service-invocation report");
         Interaction report = observerCallbacks.lastInteraction();
         JavaTckSupport.check(reportClass.equals(report.interaction)
               && report.parameters.containsKey(reportService)
               && reliable.equals(report.transportation) && report.producer == null
               && report.tag.length == 0 && report.regionsEmpty,
            "service-invocation report metadata was not standard");
         HLAunicodeString reportedService = encoder.createHLAunicodeString();
         reportedService.decode(report.parameters.get(reportService));
         JavaTckSupport.check("SendInteraction".equals(reportedService.getValue()),
            "service-invocation report identified the wrong service");
         subject.setServiceReportingSwitch(false);

         if (observerReportSubscribed) {
            observer.unsubscribeInteractionClass(reportClass);
            observerReportSubscribed = false;
         }
         observer.unsubscribeInteractionClass(observerInteraction);
         subject.unsubscribeInteractionClass(subjectInteraction);
         observer.unsubscribeObjectClassAttributes(observerClass, observerAttributes);
         observer.unpublishInteractionClass(observerInteraction);
         subject.unpublishInteractionClass(subjectInteraction);
         subject.unpublishObjectClassAttributes(subjectClass, subjectAttributes);
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         observerJoined = false;
         subject.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         subjectJoined = false;
         subject.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (subjectJoined) resign(subject, ResignAction.DELETE_OBJECTS);
         if (observerJoined) resign(observer, ResignAction.NO_ACTION);
         if (created) {
            try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (observerConnected) disconnect(observer);
         if (subjectConnected) disconnect(subject);
      }
   }

   private static byte[] encode(int length, JavaTckSupport.HandleEncoder encoder) {
      return JavaTckSupport.encode(length, encoder::encode);
   }

   private static byte[] encodeAttributeList(AttributeHandle attribute) {
      byte[] encoded = encode(attribute.encodedLength(), attribute::encode);
      byte[] result = new byte[4 + encoded.length];
      result[3] = 1;
      System.arraycopy(encoded, 0, result, 4, encoded.length);
      return result;
   }

   private static void requireHandle(Object... handles) {
      for (Object handle : handles) {
         JavaTckSupport.check(handle != null, "standard MIM request handle lookup failed");
      }
   }

   private static void await(BooleanSupplier condition, RTIambassador ambassador,
         CallbackModel callbackModel, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.01);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void resign(RTIambassador ambassador, ResignAction action) {
      try { ambassador.resignFederationExecution(action); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); }
      catch (Exception ignored) { }
   }

   private static final class Interaction {
      private final InteractionClassHandle interaction;
      private final Map<ParameterHandle, byte[]> parameters = new LinkedHashMap<>();
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final boolean regionsEmpty;

      private Interaction(InteractionClassHandle interaction, ParameterHandleValueMap values,
            byte[] tag, TransportationTypeHandle transportation, FederateHandle producer,
            RegionHandleSet regions) {
         this.interaction = interaction;
         for (ParameterHandle handle : values.keySet()) {
            parameters.put(handle, values.get(handle).clone());
         }
         this.tag = tag == null ? new byte[0] : tag.clone();
         this.transportation = transportation;
         this.producer = producer;
         this.regionsEmpty = regions == null || regions.isEmpty();
      }
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final Map<AttributeHandle, byte[]> values = new LinkedHashMap<>();
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final boolean regionsEmpty;

      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap values,
            byte[] tag, TransportationTypeHandle transportation, FederateHandle producer,
            RegionHandleSet regions) {
         this.object = object;
         for (AttributeHandle handle : values.keySet()) {
            this.values.put(handle, values.get(handle).clone());
         }
         this.tag = tag == null ? new byte[0] : tag.clone();
         this.transportation = transportation;
         this.producer = producer;
         this.regionsEmpty = regions == null || regions.isEmpty();
      }
   }

   private static final class AttributeTransportationConfirmation {
      private final ObjectInstanceHandle object;
      private final AttributeHandleSet attributes;
      private final TransportationTypeHandle transportation;

      private AttributeTransportationConfirmation(ObjectInstanceHandle object,
            AttributeHandleSet attributes, TransportationTypeHandle transportation) {
         this.object = object;
         this.attributes = (AttributeHandleSet) attributes.clone();
         this.transportation = transportation;
      }
   }

   private static final class InteractionTransportationConfirmation {
      private final InteractionClassHandle interaction;
      private final TransportationTypeHandle transportation;

      private InteractionTransportationConfirmation(InteractionClassHandle interaction,
            TransportationTypeHandle transportation) {
         this.interaction = interaction;
         this.transportation = transportation;
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<AttributeTransportationConfirmation> attributeConfirmations =
         new CopyOnWriteArrayList<>();
      private final List<InteractionTransportationConfirmation> interactionConfirmations =
         new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length == 4) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 6) {
                  interactions.add(new Interaction((InteractionClassHandle) arguments[0],
                     (ParameterHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     (RegionHandleSet) arguments[5]));
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 6) {
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     (RegionHandleSet) arguments[5]));
               } else if ("confirmAttributeTransportationTypeChange".equals(name)
                     && arguments != null && arguments.length == 3) {
                  attributeConfirmations.add(new AttributeTransportationConfirmation(
                     (ObjectInstanceHandle) arguments[0], (AttributeHandleSet) arguments[1],
                     (TransportationTypeHandle) arguments[2]));
               } else if ("confirmInteractionTransportationTypeChange".equals(name)
                     && arguments != null && arguments.length == 2) {
                  interactionConfirmations.add(new InteractionTransportationConfirmation(
                     (InteractionClassHandle) arguments[0],
                     (TransportationTypeHandle) arguments[1]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK MOM transport callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == arguments[0];
               }
               return null;
            });
      }

      private Interaction lastInteraction() {
         return interactions.get(interactions.size() - 1);
      }

      private Reflection lastReflection() {
         return reflections.get(reflections.size() - 1);
      }

      private void clearInteractions() { interactions.clear(); }
      private void clearReflections() { reflections.clear(); }
      private void clearTransportConfirmations() { attributeConfirmations.clear(); }
      private void clearInteractionConfirmations() { interactionConfirmations.clear(); }
   }
}
