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
import hla.rti1516_2025.encoding.HLAboolean;
import hla.rti1516_2025.encoding.HLAinteger16BE;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.encoding.HLAunicodeString;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies recipient MOM removal counts against committed receive-order callbacks. */
final class FederateMomObjectInstancesRemovedTck {
   private static final String MOM_CLASS = "HLAobjectRoot.HLAmanager.HLAfederate";
   private static final String SET_TIMING =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming";
   private static final String SERVICE_REPORT =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
   private static final String[] SERVICE_REPORT_PARAMETERS = {
      "HLAservice", "HLAserviceType", "HLAsuccessIndicator", "HLAsuppliedArguments",
      "HLAreturnedArgument", "HLAexception", "HLAserialNumber"
   };
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private FederateMomObjectInstancesRemovedTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder ownerRecorder = new Recorder();
      Recorder observerRecorder = new Recorder();
      String federation = "java-tck-federate-mom-object-instances-removed-" + UUID.randomUUID();
      boolean ownerConnected = false, observerConnected = false, created = false;
      boolean ownerJoined = false, observerJoined = false, momSubscribed = false;
      boolean appSubscribed = false, appPublished = false;
      boolean serviceReportSubscribed = false;
      ObjectClassHandle momClass = null, ownerClass = null, observerClass = null;
      AttributeHandleSet momAttributes = null, ownerAttributes = null, observerAttributes = null;
      InteractionClassHandle serviceReportClass = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         observer.connect(observerRecorder.proxy(), callbackModel);
         observerConnected = true;
         owner.createFederationExecutionWithMIM(federation, new String[] {fom}, mim,
            timeImplementation);
         created = true;
         FederateHandle ownerFederate = owner.joinFederationExecution(
            "mom-removed-owner", "java-tck", federation);
         ownerJoined = true;
         FederateHandle observerFederate = observer.joinFederationExecution(
            "mom-removed-observer", "java-tck", federation);
         observerJoined = true;
         for (RTIambassador ambassador : new RTIambassador[] {owner, observer}) {
            ambassador.setServiceReportingSwitch(false);
            ambassador.setSendServiceReportsToFileSwitch(false);
         }

         momClass = observer.getObjectClassHandle(MOM_CLASS);
         AttributeHandle federateAttribute = observer.getAttributeHandle(momClass,
            "HLAfederateHandle");
         AttributeHandle countAttribute = observer.getAttributeHandle(momClass,
            "HLAobjectInstancesRemoved");
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(MOM_CLASS.equals(observer.getObjectClassName(momClass))
               && "HLAfederateHandle".equals(observer.getAttributeName(momClass,
                  federateAttribute))
               && "HLAobjectInstancesRemoved".equals(observer.getAttributeName(momClass,
                  countAttribute))
               && "HLAreliable".equals(observer.getTransportationTypeName(reliable)),
            "standard federate MOM object-instances-removed lookup did not round-trip");
         momAttributes = JavaTckSupport.attributeSet(observer, federateAttribute, countAttribute);
         observer.subscribeObjectClassAttributes(momClass, momAttributes, "");
         momSubscribed = true;

         byte[] encodedObserver = encode(observerFederate);
         await(observer, callbackModel,
            () -> observerRecorder.momObject(federateAttribute, encodedObserver) != null,
            "observer federate MOM discovery");
         ObjectInstanceHandle observerMomObject = observerRecorder.momObject(federateAttribute,
            encodedObserver);
         JavaTckSupport.check(observerMomObject != null
               && momClass.equals(observer.getKnownObjectClassHandle(observerMomObject)),
            "observer found the wrong joined-federate MOM object");
         observerRecorder.ownedByRti.clear();
         observer.queryAttributeOwnership(observerMomObject,
            JavaTckSupport.attributeSet(observer, federateAttribute));
         await(observer, callbackModel,
            () -> observerRecorder.ownedByRti.stream().anyMatch(event ->
               observerMomObject.equals(event.object)
                  && event.attributes.contains(federateAttribute)),
            "RTI ownership response for a joined-federate MOM attribute");
         JavaTckSupport.check(observerRecorder.ownedByRti.size() == 1
               && observerMomObject.equals(observerRecorder.ownedByRti.get(0).object)
               && observerRecorder.ownedByRti.get(0).attributes.size() == 1
               && observerRecorder.ownedByRti.get(0).attributes.contains(federateAttribute),
            "MOM ownership query did not report RTI ownership exactly once");

         serviceReportClass = owner.getInteractionClassHandle(SERVICE_REPORT);
         InteractionClassHandle observerServiceReportClass =
            observer.getInteractionClassHandle(SERVICE_REPORT);
         TransportationTypeHandle reportReliable = owner.getTransportationTypeHandle("HLAreliable");
         Map<String, ParameterHandle> reportParameters = new LinkedHashMap<>();
         JavaTckSupport.check(serviceReportClass != null
               && serviceReportClass.equals(observerServiceReportClass)
               && SERVICE_REPORT.equals(owner.getInteractionClassName(serviceReportClass))
               && reportReliable != null
               && "HLAreliable".equals(owner.getTransportationTypeName(reportReliable)),
            "standard MOM service-report lookup did not round-trip");
         for (String name : SERVICE_REPORT_PARAMETERS) {
            ParameterHandle parameter = owner.getParameterHandle(serviceReportClass, name);
            JavaTckSupport.check(parameter != null
                  && name.equals(owner.getParameterName(serviceReportClass, parameter)),
               "standard MOM service-report parameter lookup failed for " + name);
            reportParameters.put(name, parameter);
         }
         owner.subscribeInteractionClass(serviceReportClass);
         serviceReportSubscribed = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         observerClass = observer.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle observerAttribute = observer.getAttributeHandle(observerClass,
            attributeName);
         JavaTckSupport.check(ownerClass.equals(observerClass)
               && ownerAttribute.equals(observerAttribute)
               && objectClassName.equals(owner.getObjectClassName(ownerClass))
               && objectClassName.equals(observer.getObjectClassName(observerClass))
               && attributeName.equals(owner.getAttributeName(ownerClass, ownerAttribute))
               && attributeName.equals(observer.getAttributeName(observerClass, observerAttribute)),
            "adapter-supplied object class or attribute handles did not round-trip");
         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         observerAttributes = JavaTckSupport.attributeSet(observer, observerAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         appPublished = true;
         observer.subscribeObjectClassAttributes(observerClass, observerAttributes, "");
         appSubscribed = true;

         ObjectInstanceHandle firstObject = owner.registerObjectInstance(ownerClass);
         ObjectInstanceHandle secondObject = owner.registerObjectInstance(ownerClass);
         ObjectInstanceHandle thirdObject = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(firstObject != null && secondObject != null && thirdObject != null,
            "application registration returned a null object handle");
         String firstObjectName = owner.getObjectInstanceName(firstObject);
         JavaTckSupport.check(!firstObject.equals(secondObject) && !firstObject.equals(thirdObject)
               && !secondObject.equals(thirdObject)
               && firstObjectName != null && !firstObjectName.isEmpty(),
            "application registration returned invalid or duplicate object handles");
         await(observer, callbackModel,
            () -> observerRecorder.discovered(firstObject) && observerRecorder.discovered(secondObject)
               && observerRecorder.discovered(thirdObject),
            "observer discovery of three application objects");

         int baseline = requestCount(observer, observerRecorder, callbackModel, observerMomObject,
            countAttribute, reliable, encoder, "initial removed-object count");
         JavaTckSupport.check(baseline == 0,
            "HLAobjectInstancesRemoved was nonzero before the first application removal");
         JavaTckSupport.check(baseline <= Integer.MAX_VALUE - 3,
            "initial HLAobjectInstancesRemoved count cannot safely support three removals");
         removeAndCheck(owner, observer, observerRecorder, callbackModel, ownerFederate,
            firstObject, new byte[] {(byte) 0x71}, "first receive-order object removal");
         JavaTckSupport.check(requestCount(observer, observerRecorder, callbackModel,
               observerMomObject, countAttribute, reliable, encoder,
               "count after first removal") == baseline + 1,
            "HLAobjectInstancesRemoved did not count the first committed removal callback");
         JavaTckSupport.expectFailure(
            () -> observer.getObjectInstanceHandle(firstObjectName),
            "ObjectInstanceNotKnown",
            "MOM-counted removal did not retire the application's object name");
         ownerRecorder.interactions.clear();
         observer.setServiceReportingSwitch(true);
         JavaTckSupport.check(requestCount(observer, observerRecorder, callbackModel,
               observerMomObject, countAttribute, reliable, encoder,
               "service-reported count after first removal") == baseline + 1,
            "service-reported MOM request returned the wrong removal count");
         await(owner, callbackModel, () -> !ownerRecorder.interactions.isEmpty(),
            "MOM RequestAttributeValueUpdate service report");
         verifyRequestReport(ownerRecorder.interactions, serviceReportClass, reportParameters,
            reportReliable, encoder);
         observer.setServiceReportingSwitch(false);

         InteractionClassHandle firstSetTiming = observer.getInteractionClassHandle(SET_TIMING);
         ParameterHandle firstTimingFederate = observer.getParameterHandle(
            firstSetTiming, "HLAfederate");
         ParameterHandle firstReportPeriod = observer.getParameterHandle(
            firstSetTiming, "HLAreportPeriod");
         JavaTckSupport.check(SET_TIMING.equals(observer.getInteractionClassName(firstSetTiming))
               && "HLAfederate".equals(observer.getParameterName(
                  firstSetTiming, firstTimingFederate))
               && "HLAreportPeriod".equals(observer.getParameterName(
                  firstSetTiming, firstReportPeriod)),
            "standard HLAsetTiming lookup did not round-trip");
         int firstPeriodicStart = observerRecorder.reflections.size();
         sendTiming(observer, firstSetTiming, firstTimingFederate, firstReportPeriod,
            observerFederate, 1, encoder);
         await(observer, callbackModel,
            () -> findCountAtLeast(observerRecorder, firstPeriodicStart, observerMomObject,
               countAttribute, encoder, baseline + 1) != null,
            "periodic removed-object count after first removal");
         Reflection firstPeriodic = findCountAtLeast(observerRecorder, firstPeriodicStart,
            observerMomObject, countAttribute, encoder, baseline + 1);
         JavaTckSupport.check(firstPeriodic != null
               && verifyCount(firstPeriodic, countAttribute, reliable, encoder,
                  "periodic removed-object count after first removal") == baseline + 1,
            "periodic HLAobjectInstancesRemoved did not preserve the first removal count");
         sendTiming(observer, firstSetTiming, firstTimingFederate, firstReportPeriod,
            observerFederate, 0, encoder);
         removeAndCheck(owner, observer, observerRecorder, callbackModel, ownerFederate,
            secondObject, new byte[] {(byte) 0x72}, "second receive-order object removal");
         JavaTckSupport.check(requestCount(observer, observerRecorder, callbackModel,
               observerMomObject, countAttribute, reliable, encoder,
               "count after second removal") == baseline + 2,
            "HLAobjectInstancesRemoved did not count the second committed removal callback");

         InteractionClassHandle setTiming = observer.getInteractionClassHandle(SET_TIMING);
         ParameterHandle timingFederate = observer.getParameterHandle(setTiming, "HLAfederate");
         ParameterHandle reportPeriod = observer.getParameterHandle(setTiming, "HLAreportPeriod");
         JavaTckSupport.check(SET_TIMING.equals(observer.getInteractionClassName(setTiming))
               && "HLAfederate".equals(observer.getParameterName(setTiming, timingFederate))
               && "HLAreportPeriod".equals(observer.getParameterName(setTiming, reportPeriod)),
            "standard HLAsetTiming lookup did not round-trip");
         int periodicStart = observerRecorder.reflections.size();
         sendTiming(observer, setTiming, timingFederate, reportPeriod, observerFederate, 1, encoder);
         removeAndCheck(owner, observer, observerRecorder, callbackModel, ownerFederate,
            thirdObject, new byte[] {(byte) 0x73}, "periodic-phase receive-order object removal");
         await(observer, callbackModel,
            () -> findCountAtLeast(observerRecorder, periodicStart, observerMomObject,
               countAttribute, encoder, baseline + 3) != null,
            "periodic object-instances-removed value");
         Reflection periodic = findCountAtLeast(observerRecorder, periodicStart, observerMomObject,
            countAttribute, encoder, baseline + 3);
         JavaTckSupport.check(periodic != null && verifyCount(periodic, countAttribute, reliable,
               encoder, "periodic object-instances-removed") == baseline + 3,
            "periodic HLAobjectInstancesRemoved disagreed with committed callback total");
         sendTiming(observer, setTiming, timingFederate, reportPeriod, observerFederate, 0, encoder);
         JavaTckSupport.check(requestCount(observer, observerRecorder, callbackModel,
               observerMomObject, countAttribute, reliable, encoder,
               "direct count after disabling periodic reports") == baseline + 3,
            "direct HLAobjectInstancesRemoved changed after periodic reporting was disabled");

         observer.unsubscribeObjectClassAttributes(observerClass, observerAttributes);
         appSubscribed = false;
         observer.unsubscribeObjectClassAttributes(momClass, momAttributes);
         momSubscribed = false;
         owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes);
         appPublished = false;
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         observerJoined = false;
         owner.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         ownerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (observerJoined) try { observer.setServiceReportingSwitch(false); }
            catch (Exception ignored) { }
         if (appSubscribed && observerJoined && observerClass != null && observerAttributes != null)
            try { observer.unsubscribeObjectClassAttributes(observerClass, observerAttributes); }
            catch (Exception ignored) { }
         if (momSubscribed && observerJoined && momClass != null && momAttributes != null)
            try { observer.unsubscribeObjectClassAttributes(momClass, momAttributes); }
            catch (Exception ignored) { }
         if (appPublished && ownerJoined && ownerClass != null && ownerAttributes != null)
            try { owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes); }
            catch (Exception ignored) { }
         if (serviceReportSubscribed && ownerJoined && serviceReportClass != null)
            try { owner.unsubscribeInteractionClass(serviceReportClass); }
            catch (Exception ignored) { }
         if (observerJoined) resign(observer);
         if (ownerJoined) resign(owner);
         if (created) try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (observerConnected) disconnect(observer);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static void verifyRequestReport(List<Interaction> interactions,
         InteractionClassHandle reportClass, Map<String, ParameterHandle> parameters,
         TransportationTypeHandle reliable, EncoderFactory encoder) throws Exception {
      JavaTckSupport.check(interactions.size() == 1,
         "MOM RequestAttributeValueUpdate did not produce exactly one service report");
      Interaction report = interactions.get(0);
      JavaTckSupport.check(reportClass.equals(report.interaction)
            && report.tag.length == 0 && report.producer == null
            && (report.regions == null || report.regions.isEmpty())
            && reliable.equals(report.transportation),
         "MOM RequestAttributeValueUpdate report had invalid identity or metadata");
      for (String name : SERVICE_REPORT_PARAMETERS)
         JavaTckSupport.check(report.parameters.containsKey(parameters.get(name)),
            "MOM service report omitted standard parameter " + name);
      HLAunicodeString service = encoder.createHLAunicodeString();
      service.decode(report.parameters.get(parameters.get("HLAservice")));
      HLAinteger16BE serviceType = encoder.createHLAinteger16BE();
      serviceType.decode(report.parameters.get(parameters.get("HLAserviceType")));
      HLAboolean success = encoder.createHLAboolean();
      success.decode(report.parameters.get(parameters.get("HLAsuccessIndicator")));
      HLAunicodeString exception = encoder.createHLAunicodeString();
      exception.decode(report.parameters.get(parameters.get("HLAexception")));
      HLAinteger32BE serial = encoder.createHLAinteger32BE();
      serial.decode(report.parameters.get(parameters.get("HLAserialNumber")));
      JavaTckSupport.check("RequestAttributeValueUpdate".equals(service.getValue())
            && serviceType.getValue() == 2 && success.getValue()
            && exception.getValue().isEmpty() && serial.getValue() == 0,
         "MOM service report carried an incorrect service, outcome, exception, or serial");
      interactions.clear();
   }

   private static void removeAndCheck(RTIambassador owner, RTIambassador observer,
         Recorder recorder, CallbackModel model, FederateHandle ownerFederate,
         ObjectInstanceHandle object, byte[] tag, String description) throws Exception {
      int before = recorder.removals.size();
      owner.deleteObjectInstance(object, tag);
      if (model == CallbackModel.HLA_EVOKED)
         JavaTckSupport.check(recorder.removals.size() == before,
            description + " arrived before callback servicing");
      await(observer, model, () -> recorder.removal(object, before) != null, description);
      Removal removal = recorder.removal(object, before);
      JavaTckSupport.check(recorder.removalCount(object, before) == 1
            && removal != null && Arrays.equals(tag, removal.tag)
            && ownerFederate.equals(removal.producer),
         description + " produced missing, duplicate, or incorrectly attributed removal metadata");
   }

   private static int requestCount(RTIambassador rti, Recorder recorder, CallbackModel model,
         ObjectInstanceHandle object, AttributeHandle attribute, TransportationTypeHandle reliable,
         EncoderFactory encoder, String description) throws Exception {
      int start = recorder.reflections.size();
      rti.requestAttributeValueUpdate(object, JavaTckSupport.attributeSet(rti, attribute),
         new byte[0]);
      await(rti, model, () -> findCount(recorder, start, object, attribute) != null, description);
      Reflection reflection = findCount(recorder, start, object, attribute);
      JavaTckSupport.check(reflection != null, description + " produced no MOM value");
      return verifyCount(reflection, attribute, reliable, encoder, description);
   }

   private static int verifyCount(Reflection reflection, AttributeHandle attribute,
         TransportationTypeHandle reliable, EncoderFactory encoder, String description)
         throws Exception {
      JavaTckSupport.check(reflection.values.size() == 1
            && reflection.values.containsKey(attribute) && reflection.tag.length == 0
            && reliable.equals(reflection.transportation) && reflection.producer == null
            && (reflection.regions == null || reflection.regions.isEmpty()),
         description + " returned non-standard MOM reflection metadata");
      HLAinteger32BE value = encoder.createHLAinteger32BE();
      value.decode(reflection.values.get(attribute));
      JavaTckSupport.check(value.getValue() >= 0, description + " returned a negative count");
      return value.getValue();
   }

   private static Reflection findCount(Recorder recorder, int start, ObjectInstanceHandle object,
         AttributeHandle attribute) {
      for (int i = Math.min(start, recorder.reflections.size()); i < recorder.reflections.size(); ++i) {
         Reflection reflection = recorder.reflections.get(i);
         if (object.equals(reflection.object) && reflection.values.containsKey(attribute))
            return reflection;
      }
      return null;
   }

   private static Reflection findCountAtLeast(Recorder recorder, int start,
         ObjectInstanceHandle object, AttributeHandle attribute, EncoderFactory encoder, int minimum) {
      for (int i = Math.min(start, recorder.reflections.size()); i < recorder.reflections.size(); ++i) {
         Reflection reflection = recorder.reflections.get(i);
         if (!object.equals(reflection.object) || !reflection.values.containsKey(attribute)) continue;
         HLAinteger32BE value = encoder.createHLAinteger32BE();
         try { value.decode(reflection.values.get(attribute)); }
         catch (Exception error) { throw new IllegalStateException("invalid MOM counter encoding", error); }
         if (value.getValue() >= minimum) return reflection;
      }
      return null;
   }

   private static void sendTiming(RTIambassador rti, InteractionClassHandle interaction,
         ParameterHandle federateParameter, ParameterHandle periodParameter, FederateHandle target,
         int period, EncoderFactory encoder) throws Exception {
      ParameterHandleValueMap values = rti.getParameterHandleValueMapFactory().create(2);
      values.put(federateParameter, encode(target));
      values.put(periodParameter, encoder.createHLAinteger32BE(period).toByteArray());
      rti.sendInteraction(interaction, values, new byte[0]);
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

   private static final class Removal {
      private final ObjectInstanceHandle object;
      private final byte[] tag;
      private final FederateHandle producer;
      private Removal(ObjectInstanceHandle object, byte[] tag, FederateHandle producer) {
         this.object = object;
         this.tag = tag.clone();
         this.producer = producer;
      }
   }

   private static final class OwnershipEvent {
      private final ObjectInstanceHandle object;
      private final AttributeHandleSet attributes;
      private OwnershipEvent(ObjectInstanceHandle object, AttributeHandleSet attributes) {
         this.object = object;
         this.attributes = (AttributeHandleSet) attributes.clone();
      }
   }

   private static final class Interaction {
      private final InteractionClassHandle interaction;
      private final Map<ParameterHandle, byte[]> parameters;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final RegionHandleSet regions;
      private Interaction(InteractionClassHandle interaction, ParameterHandleValueMap values,
            byte[] tag, TransportationTypeHandle transportation, FederateHandle producer,
            RegionHandleSet regions) {
         this.interaction = interaction;
         this.parameters = new LinkedHashMap<>();
         for (ParameterHandle handle : values.keySet())
            this.parameters.put(handle, values.get(handle).clone());
         this.tag = tag.clone();
         this.transportation = transportation;
         this.producer = producer;
         this.regions = regions == null ? null : (RegionHandleSet) regions.clone();
      }
   }

   private static final class Recorder {
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Removal> removals = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> ownedByRti = new CopyOnWriteArrayList<>();
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();

      private ObjectInstanceHandle momObject(AttributeHandle attribute, byte[] expected) {
         for (Reflection reflection : reflections) {
            byte[] value = reflection.values.get(attribute);
            if (value != null && Arrays.equals(expected, value)) return reflection.object;
         }
         return null;
      }
      private boolean discovered(ObjectInstanceHandle object) { return discoveries.contains(object); }
      private Removal removal(ObjectInstanceHandle object, int start) {
         for (int i = Math.min(start, removals.size()); i < removals.size(); ++i)
            if (object.equals(removals.get(i).object)) return removals.get(i);
         return null;
      }
      private int removalCount(ObjectInstanceHandle object, int start) {
         int count = 0;
         for (int i = Math.min(start, removals.size()); i < removals.size(); ++i)
            if (object.equals(removals.get(i).object)) ++count;
         return count;
      }
      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("removeObjectInstance".equals(name) && arguments != null
                     && arguments.length == 3) {
                  removals.add(new Removal((ObjectInstanceHandle) arguments[0],
                     (byte[]) arguments[1], (FederateHandle) arguments[2]));
               } else if ("attributeIsOwnedByRTI".equals(name) && arguments != null
                     && arguments.length == 2) {
                  ownedByRti.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1]));
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 6) {
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     (RegionHandleSet) arguments[5]));
               } else if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 6) {
                  interactions.add(new Interaction((InteractionClassHandle) arguments[0],
                     (ParameterHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     (RegionHandleSet) arguments[5]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK object-instances-removed MOM callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
