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
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies direct and periodic joined-federate MOM time-state duration values. */
final class FederateMomTimeStateDurationsTck {
   private static final String MOM_CLASS = "HLAobjectRoot.HLAmanager.HLAfederate";
   private static final String SET_TIMING =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming";
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private FederateMomTimeStateDurationsTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(mim != null && !mim.isEmpty() && timeImplementation != null
            && !timeImplementation.isEmpty(),
         "joined-federate MOM time-state testing requires adapter-supplied MIM and logical-time implementation");
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador regulator = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder subjectRecorder = new Recorder();
      Recorder regulatorRecorder = new Recorder();
      Recorder observerRecorder = new Recorder();
      String federation = "java-tck-federate-mom-time-state-durations-" + UUID.randomUUID();
      boolean subjectConnected = false, regulatorConnected = false, observerConnected = false;
      boolean created = false;
      boolean subjectJoined = false, regulatorJoined = false, observerJoined = false;
      boolean subscribed = false, timeConstrained = false, timeRegulating = false;
      ObjectClassHandle momClass = null;
      AttributeHandleSet momAttributes = null;
      try {
         subject.connect(subjectRecorder.proxy(), callbackModel);
         subjectConnected = true;
         regulator.connect(regulatorRecorder.proxy(), callbackModel);
         regulatorConnected = true;
         observer.connect(observerRecorder.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(federation, new String[] {fom}, mim,
            timeImplementation);
         created = true;
         observer.joinFederationExecution("mom-duration-observer", "java-tck", federation);
         observerJoined = true;

         momClass = observer.getObjectClassHandle(MOM_CLASS);
         AttributeHandle federateAttribute = observer.getAttributeHandle(momClass,
            "HLAfederateHandle");
         AttributeHandle timeGrantedAttribute = observer.getAttributeHandle(momClass,
            "HLAtimeGrantedTime");
         AttributeHandle timeAdvancingAttribute = observer.getAttributeHandle(momClass,
            "HLAtimeAdvancingTime");
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(MOM_CLASS.equals(observer.getObjectClassName(momClass))
               && "HLAfederateHandle".equals(observer.getAttributeName(momClass,
                  federateAttribute))
               && "HLAtimeGrantedTime".equals(observer.getAttributeName(momClass,
                  timeGrantedAttribute))
               && "HLAtimeAdvancingTime".equals(observer.getAttributeName(momClass,
                  timeAdvancingAttribute))
               && "HLAreliable".equals(observer.getTransportationTypeName(reliable)),
            "joined-federate MOM time-state handles did not round-trip standard MIM names");
         momAttributes = JavaTckSupport.attributeSet(observer, federateAttribute,
            timeGrantedAttribute, timeAdvancingAttribute);
         observer.subscribeObjectClassAttributes(momClass, momAttributes, "");
         subscribed = true;

         FederateHandle subjectFederate = subject.joinFederationExecution(
            "mom-duration-subject", "java-tck", federation);
         subjectJoined = true;
         regulator.joinFederationExecution("mom-duration-regulator", "java-tck", federation);
         regulatorJoined = true;
         byte[] encodedSubject = encode(subjectFederate);
         await(observer, callbackModel,
            () -> observerRecorder.momReflection(federateAttribute, encodedSubject) != null,
            "subject federate MOM discovery");
         Reflection initial = observerRecorder.momReflection(federateAttribute, encodedSubject);
         ObjectInstanceHandle momObject = initial == null ? null : initial.object;
         JavaTckSupport.check(initial != null && momObject != null
               && momClass.equals(observer.getKnownObjectClassHandle(momObject))
               && !initial.values.containsKey(timeGrantedAttribute)
               && !initial.values.containsKey(timeAdvancingAttribute),
            "initial joined-federate MOM reflection did not omit conditional time-state fields");

         DurationValues initialDurations = requestDurations(observer, observerRecorder,
            callbackModel, momObject, timeGrantedAttribute, timeAdvancingAttribute, reliable,
            encoder, "initial direct time-state reflection");
         JavaTckSupport.check(initialDurations.granted >= 0 && initialDurations.advancing >= 0,
            "initial joined-federate time-state durations were negative");

         LogicalTimeFactory<?, ?> timeFactory = regulator.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(subject.getTimeFactory().getName())
               && timeFactory.getName().equals(observer.getTimeFactory().getName()),
            "joined-federate MOM members selected different logical-time factories");
         LogicalTime<?, ?> initialTime = (LogicalTime<?, ?>) timeFactory.makeInitial();
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         LogicalTime<?, ?> targetTime = add(initialTime, intervalOffset(epsilon, 2));

         subject.enableTimeConstrained();
         timeConstrained = true;
         await(subject, callbackModel, () -> subjectRecorder.timeConstrainedEnabled == 1,
            "target timeConstrainedEnabled callback");
         regulator.enableTimeRegulation(epsilon);
         timeRegulating = true;
         await(regulator, callbackModel, () -> regulatorRecorder.timeRegulationEnabled == 1,
            "regulator timeRegulationEnabled callback");
         subject.timeAdvanceRequest(targetTime);
         Thread.sleep(60L);
         DurationValues whileAdvancing = requestDurations(observer, observerRecorder,
            callbackModel, momObject, timeGrantedAttribute, timeAdvancingAttribute, reliable,
            encoder, "time-state reflection while target is advancing");
         JavaTckSupport.check(whileAdvancing.granted >= initialDurations.granted
               && whileAdvancing.advancing >= initialDurations.advancing
               && whileAdvancing.advancing > 0,
            "time-state durations did not grow while the target was advancing");

         regulator.timeAdvanceRequest(targetTime);
         await(subject, callbackModel, () -> targetTime.equals(subjectRecorder.timeAdvanceGrant),
            "target timeAdvanceGrant callback");
         await(regulator, callbackModel,
            () -> targetTime.equals(regulatorRecorder.timeAdvanceGrant),
            "regulator timeAdvanceGrant callback");
         Thread.sleep(60L);
         DurationValues afterGrant = requestDurations(observer, observerRecorder, callbackModel,
            momObject, timeGrantedAttribute, timeAdvancingAttribute, reliable, encoder,
            "time-state reflection after target grant");
         JavaTckSupport.check(afterGrant.granted >= initialDurations.granted
               && afterGrant.granted > 0 && afterGrant.advancing >= whileAdvancing.advancing,
            "time-state durations did not retain accumulated time after the target grant");

         InteractionClassHandle setTiming = observer.getInteractionClassHandle(SET_TIMING);
         ParameterHandle timingFederate = observer.getParameterHandle(setTiming, "HLAfederate");
         ParameterHandle reportPeriod = observer.getParameterHandle(setTiming, "HLAreportPeriod");
         JavaTckSupport.check(SET_TIMING.equals(observer.getInteractionClassName(setTiming))
               && "HLAfederate".equals(observer.getParameterName(setTiming, timingFederate))
               && "HLAreportPeriod".equals(observer.getParameterName(setTiming, reportPeriod)),
            "HLAsetTiming lookup did not round-trip standard MIM names");
         int periodicStart = observerRecorder.reflections.size();
         sendTiming(observer, setTiming, timingFederate, reportPeriod, subjectFederate, 1, encoder);
         await(observer, callbackModel,
            () -> findDurationReflection(observerRecorder, periodicStart, momObject,
               timeGrantedAttribute, timeAdvancingAttribute) != null,
            "periodic joined-federate MOM duration reflection");
         Reflection periodic = findDurationReflection(observerRecorder, periodicStart, momObject,
            timeGrantedAttribute, timeAdvancingAttribute);
         DurationValues periodicDurations = validateDurationReflection(periodic, momObject,
            timeGrantedAttribute, timeAdvancingAttribute, reliable, encoder,
            "periodic time-state reflection");
         JavaTckSupport.check(periodicDurations.granted >= afterGrant.granted
               && periodicDurations.advancing >= afterGrant.advancing,
            "periodic time-state values were smaller than the preceding direct values");

         DurationValues afterPeriodic = requestDurations(observer, observerRecorder,
            callbackModel, momObject, timeGrantedAttribute, timeAdvancingAttribute, reliable,
            encoder, "direct time-state reflection after periodic reporting");
         JavaTckSupport.check(afterPeriodic.granted < periodicDurations.granted
               && afterPeriodic.advancing <= periodicDurations.advancing,
            "direct time-state request did not observe the interval consumed "
               + "by periodic reporting");
         Thread.sleep(30L);
         DurationValues secondAfterPeriodic = requestDurations(observer, observerRecorder,
            callbackModel, momObject, timeGrantedAttribute, timeAdvancingAttribute, reliable,
            encoder, "second direct time-state reflection after periodic reporting");
         JavaTckSupport.check(secondAfterPeriodic.granted >= afterPeriodic.granted
               && secondAfterPeriodic.advancing >= afterPeriodic.advancing,
            "time-state durations decreased across successive direct requests");
         sendTiming(observer, setTiming, timingFederate, reportPeriod, subjectFederate, 0, encoder);

         observer.unsubscribeObjectClassAttributes(momClass, momAttributes);
         subscribed = false;
         subject.resignFederationExecution(ResignAction.NO_ACTION);
         subjectJoined = false;
         timeConstrained = false;
         regulator.resignFederationExecution(ResignAction.NO_ACTION);
         regulatorJoined = false;
         timeRegulating = false;
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         observerJoined = false;
         subject.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (timeConstrained && subjectJoined) try { subject.disableTimeConstrained(); }
            catch (Exception ignored) { }
         if (timeRegulating && regulatorJoined) try { regulator.disableTimeRegulation(); }
            catch (Exception ignored) { }
         if (subscribed && observerJoined && momClass != null && momAttributes != null) try {
            observer.unsubscribeObjectClassAttributes(momClass, momAttributes);
         } catch (Exception ignored) { }
         if (subjectJoined) resign(subject);
         if (regulatorJoined) resign(regulator);
         if (observerJoined) resign(observer);
         if (created) try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (observerConnected) disconnect(observer);
         if (regulatorConnected) disconnect(regulator);
         if (subjectConnected) disconnect(subject);
      }
   }

   private static DurationValues requestDurations(RTIambassador observer, Recorder recorder,
         CallbackModel callbackModel, ObjectInstanceHandle object, AttributeHandle granted,
         AttributeHandle advancing, TransportationTypeHandle reliable, EncoderFactory encoder,
         String description) throws Exception {
      int first = recorder.reflections.size();
      observer.requestAttributeValueUpdate(object,
         JavaTckSupport.attributeSet(observer, granted, advancing), new byte[0]);
      await(observer, callbackModel,
         () -> findDurationReflection(recorder, first, object, granted, advancing) != null,
         description);
      Reflection reflection = findDurationReflection(recorder, first, object, granted, advancing);
      return validateDurationReflection(reflection, object, granted, advancing, reliable, encoder,
         description);
   }

   private static Reflection findDurationReflection(Recorder recorder, int first,
         ObjectInstanceHandle object, AttributeHandle granted, AttributeHandle advancing) {
      for (int i = Math.min(first, recorder.reflections.size()); i < recorder.reflections.size(); ++i) {
         Reflection reflection = recorder.reflections.get(i);
         if (object.equals(reflection.object) && reflection.values.size() == 2
               && reflection.values.containsKey(granted) && reflection.values.containsKey(advancing))
            return reflection;
      }
      return null;
   }

   private static DurationValues validateDurationReflection(Reflection reflection,
         ObjectInstanceHandle object, AttributeHandle granted, AttributeHandle advancing,
         TransportationTypeHandle reliable, EncoderFactory encoder, String description)
         throws Exception {
      JavaTckSupport.check(reflection != null && object.equals(reflection.object)
            && reflection.values.size() == 2 && reflection.values.containsKey(granted)
            && reflection.values.containsKey(advancing) && reflection.tag.length == 0
            && reliable.equals(reflection.transportation) && reflection.producer == null
            && (reflection.regions == null || reflection.regions.isEmpty()),
         description + " returned non-standard MOM reflection metadata or fields");
      int grantedValue = validateDuration(reflection.values.get(granted), encoder,
         description + " HLAtimeGrantedTime");
      int advancingValue = validateDuration(reflection.values.get(advancing), encoder,
         description + " HLAtimeAdvancingTime");
      return new DurationValues(grantedValue, advancingValue);
   }

   private static int validateDuration(byte[] encoded, EncoderFactory encoder, String description)
         throws Exception {
      JavaTckSupport.check(encoded != null && encoded.length == 4,
         description + " did not use four-octet HLAinteger32BE encoding");
      HLAinteger32BE value = encoder.createHLAinteger32BE();
      value.decode(encoded);
      JavaTckSupport.check(value.getValue() >= 0, description + " returned a negative duration");
      return value.getValue();
   }

   private static LogicalTimeInterval intervalOffset(LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTimeInterval result = epsilon;
      for (int i = 1; i < steps; ++i) result = (LogicalTimeInterval) result.add(epsilon);
      return result;
   }

   private static LogicalTime add(LogicalTime start, LogicalTimeInterval interval)
         throws Exception {
      return (LogicalTime) start.add(interval);
   }

   private static void sendTiming(RTIambassador sender, InteractionClassHandle interaction,
         ParameterHandle targetFederate, ParameterHandle reportPeriod, FederateHandle subject,
         int seconds, EncoderFactory encoder) throws Exception {
      ParameterHandleValueMap values = sender.getParameterHandleValueMapFactory().create(2);
      values.put(targetFederate, encode(subject));
      values.put(reportPeriod, encoder.createHLAinteger32BE(seconds).toByteArray());
      sender.sendInteraction(interaction, values, new byte[0]);
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

   private static final class DurationValues {
      private final int granted;
      private final int advancing;

      private DurationValues(int granted, int advancing) {
         this.granted = granted;
         this.advancing = advancing;
      }
   }

   private static final class Recorder {
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private volatile int timeConstrainedEnabled;
      private volatile int timeRegulationEnabled;
      private volatile LogicalTime<?, ?> timeAdvanceGrant;
      private Reflection momReflection(AttributeHandle attribute, byte[] expected) {
         for (Reflection reflection : reflections) {
            byte[] value = reflection.values.get(attribute);
            if (value != null && Arrays.equals(expected, value)) return reflection;
         }
         return null;
      }
      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 6) {
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     (RegionHandleSet) arguments[5]));
               } else if ("timeConstrainedEnabled".equals(name)) {
                  ++timeConstrainedEnabled;
               } else if ("timeRegulationEnabled".equals(name)) {
                  ++timeRegulationEnabled;
               } else if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  timeAdvanceGrant = (LogicalTime<?, ?>) arguments[0];
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK time-state MOM callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
