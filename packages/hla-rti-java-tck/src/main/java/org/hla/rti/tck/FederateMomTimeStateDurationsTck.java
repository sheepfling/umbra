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
      RTIambassador observer = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder subjectRecorder = new Recorder();
      Recorder observerRecorder = new Recorder();
      String federation = "java-tck-federate-mom-time-state-durations-" + UUID.randomUUID();
      boolean subjectConnected = false, observerConnected = false, created = false;
      boolean subjectJoined = false, observerJoined = false, subscribed = false;
      ObjectClassHandle momClass = null;
      AttributeHandleSet momAttributes = null;
      try {
         subject.connect(subjectRecorder.proxy(), callbackModel);
         subjectConnected = true;
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

         int directStart = observerRecorder.reflections.size();
         observer.requestAttributeValueUpdate(momObject,
            JavaTckSupport.attributeSet(observer, timeGrantedAttribute, timeAdvancingAttribute),
            new byte[0]);
         await(observer, callbackModel,
            () -> findDurationReflection(observerRecorder, directStart, momObject,
               timeGrantedAttribute, timeAdvancingAttribute) != null,
            "direct joined-federate MOM duration reflection");
         Reflection direct = findDurationReflection(observerRecorder, directStart, momObject,
            timeGrantedAttribute, timeAdvancingAttribute);
         validateDurationReflection(direct, momObject, timeGrantedAttribute,
            timeAdvancingAttribute, reliable, encoder, "direct time-state reflection");

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
         validateDurationReflection(periodic, momObject, timeGrantedAttribute,
            timeAdvancingAttribute, reliable, encoder, "periodic time-state reflection");
         sendTiming(observer, setTiming, timingFederate, reportPeriod, subjectFederate, 0, encoder);

         observer.unsubscribeObjectClassAttributes(momClass, momAttributes);
         subscribed = false;
         subject.resignFederationExecution(ResignAction.NO_ACTION);
         subjectJoined = false;
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         observerJoined = false;
         subject.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (subscribed && observerJoined && momClass != null && momAttributes != null) try {
            observer.unsubscribeObjectClassAttributes(momClass, momAttributes);
         } catch (Exception ignored) { }
         if (subjectJoined) resign(subject);
         if (observerJoined) resign(observer);
         if (created) try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (observerConnected) disconnect(observer);
         if (subjectConnected) disconnect(subject);
      }
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

   private static void validateDurationReflection(Reflection reflection,
         ObjectInstanceHandle object, AttributeHandle granted, AttributeHandle advancing,
         TransportationTypeHandle reliable, EncoderFactory encoder, String description)
         throws Exception {
      JavaTckSupport.check(reflection != null && object.equals(reflection.object)
            && reflection.values.size() == 2 && reflection.values.containsKey(granted)
            && reflection.values.containsKey(advancing) && reflection.tag.length == 0
            && reliable.equals(reflection.transportation) && reflection.producer == null
            && (reflection.regions == null || reflection.regions.isEmpty()),
         description + " returned non-standard MOM reflection metadata or fields");
      validateDuration(reflection.values.get(granted), encoder, description + " HLAtimeGrantedTime");
      validateDuration(reflection.values.get(advancing), encoder,
         description + " HLAtimeAdvancingTime");
   }

   private static void validateDuration(byte[] encoded, EncoderFactory encoder, String description)
         throws Exception {
      JavaTckSupport.check(encoded != null && encoded.length == 4,
         description + " did not use four-octet HLAinteger32BE encoding");
      HLAinteger32BE value = encoder.createHLAinteger32BE();
      value.decode(encoded);
      JavaTckSupport.check(value.getValue() >= 0, description + " returned a negative duration");
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

   private static final class Recorder {
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
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
