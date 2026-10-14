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
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies periodic standard federate-MOM HLAlogicalTime values. */
final class FederateLogicalTimeMomPeriodicTck {
   private static final String MOM_CLASS = "HLAobjectRoot.HLAmanager.HLAfederate";
   private static final String MOM_FEDERATE_HANDLE = "HLAfederateHandle";
   private static final String MOM_LOGICAL_TIME = "HLAlogicalTime";
   private static final String SET_TIMING =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming";
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private FederateLogicalTimeMomPeriodicTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador monitor = factory.getRtiAmbassador();
      RTIambassador subject = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder monitorRecorder = new Recorder();
      Recorder subjectRecorder = new Recorder();
      String federation = "java-tck-federate-logical-time-mom-" + UUID.randomUUID();
      boolean monitorConnected = false, subjectConnected = false, created = false;
      boolean monitorJoined = false, subjectJoined = false, subscribed = false;
      boolean subjectConstrained = false, monitorRegulating = false;
      ObjectClassHandle momClass = null;
      AttributeHandleSet subscription = null;
      try {
         monitor.connect(monitorRecorder.proxy(), callbackModel);
         monitorConnected = true;
         subject.connect(subjectRecorder.proxy(), callbackModel);
         subjectConnected = true;
         monitor.createFederationExecutionWithMIM(federation, new String[] {fom}, mim,
            timeImplementation);
         created = true;
         monitor.joinFederationExecution("logical-time-mom-monitor", "java-tck", federation);
         monitorJoined = true;

         momClass = monitor.getObjectClassHandle(MOM_CLASS);
         AttributeHandle federateHandleAttribute = monitor.getAttributeHandle(momClass,
            MOM_FEDERATE_HANDLE);
         AttributeHandle logicalTimeAttribute = monitor.getAttributeHandle(momClass,
            MOM_LOGICAL_TIME);
         TransportationTypeHandle reliable = monitor.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(MOM_CLASS.equals(monitor.getObjectClassName(momClass))
               && MOM_FEDERATE_HANDLE.equals(monitor.getAttributeName(momClass,
                  federateHandleAttribute))
               && MOM_LOGICAL_TIME.equals(monitor.getAttributeName(momClass,
                  logicalTimeAttribute))
               && "HLAreliable".equals(monitor.getTransportationTypeName(reliable)),
            "HLAlogicalTime MOM lookup did not round-trip standard MIM handles");
         subscription = JavaTckSupport.attributeSet(monitor, federateHandleAttribute,
            logicalTimeAttribute);
         monitor.subscribeObjectClassAttributes(momClass, subscription, "");
         subscribed = true;

         FederateHandle subjectFederate = subject.joinFederationExecution(
            "logical-time-mom-subject", "java-tck", federation);
         subjectJoined = true;
         LogicalTimeFactory<?, ?> subjectTimeFactory = subject.getTimeFactory();
         LogicalTimeFactory<?, ?> monitorTimeFactory = monitor.getTimeFactory();
         JavaTckSupport.check(subjectTimeFactory.getName().equals(monitorTimeFactory.getName()),
            "logical-time MOM members selected different standard time factories");
         LogicalTime subjectInitial = (LogicalTime) subjectTimeFactory.makeInitial();
         LogicalTime monitorInitial = (LogicalTime) monitorTimeFactory.makeInitial();
         LogicalTimeInterval subjectEpsilon =
            (LogicalTimeInterval) subjectTimeFactory.makeEpsilon();
         LogicalTimeInterval monitorEpsilon =
            (LogicalTimeInterval) monitorTimeFactory.makeEpsilon();
         JavaTckSupport.check(subjectInitial.equals(monitorInitial)
               && subjectEpsilon.equals(monitorEpsilon),
            "logical-time MOM participants had incompatible initial/epsilon values");

         byte[] encodedSubject = new byte[subjectFederate.encodedLength()];
         subjectFederate.encode(encodedSubject, 0);
         await(monitor, callbackModel, () -> monitorRecorder.momObject(
            federateHandleAttribute, encodedSubject) != null,
            "HLAlogicalTime subject MOM discovery");
         ObjectInstanceHandle momObject = monitorRecorder.momObject(
            federateHandleAttribute, encodedSubject);
         JavaTckSupport.check(momObject != null
               && momClass.equals(monitor.getKnownObjectClassHandle(momObject)),
            "HLAlogicalTime discovery returned the wrong standard MOM object");

         int constrainedBefore = subjectRecorder.timeConstrainedEnabled;
         subject.enableTimeConstrained();
         subjectConstrained = true;
         await(subject, callbackModel,
            () -> subjectRecorder.timeConstrainedEnabled >= constrainedBefore + 1,
            "timeConstrainedEnabled callback for logical-time MOM subject");
         JavaTckSupport.check(subjectRecorder.timeConstrainedEnabled == constrainedBefore + 1,
            "Enable Time Constrained delivered an unexpected number of callbacks");
         int regulatingBefore = monitorRecorder.timeRegulationEnabled;
         monitor.enableTimeRegulation(monitorEpsilon);
         monitorRegulating = true;
         await(monitor, callbackModel,
            () -> monitorRecorder.timeRegulationEnabled >= regulatingBefore + 1,
            "timeRegulationEnabled callback for logical-time MOM monitor");
         JavaTckSupport.check(monitorRecorder.timeRegulationEnabled == regulatingBefore + 1,
            "Enable Time Regulation delivered an unexpected number of callbacks");

         LogicalTime initialQuery = subject.queryLogicalTime();
         JavaTckSupport.check(subjectInitial.equals(initialQuery),
            "initial Query Logical Time did not match the federation initial time");

         InteractionClassHandle setTiming = monitor.getInteractionClassHandle(SET_TIMING);
         ParameterHandle targetFederate = monitor.getParameterHandle(setTiming, "HLAfederate");
         ParameterHandle reportPeriod = monitor.getParameterHandle(setTiming, "HLAreportPeriod");
         JavaTckSupport.check(SET_TIMING.equals(monitor.getInteractionClassName(setTiming))
               && "HLAfederate".equals(monitor.getParameterName(setTiming, targetFederate))
               && "HLAreportPeriod".equals(monitor.getParameterName(setTiming, reportPeriod)),
            "HLAsetTiming lookup did not round-trip standard MIM handles");
         int beforeInitialReport = monitorRecorder.reflections.size();
         sendReportPeriod(monitor, setTiming, targetFederate, reportPeriod, subjectFederate,
            1, encoder);
         awaitLogicalTimeReflection(monitor, monitorRecorder, callbackModel, momObject,
            logicalTimeAttribute, reliable, subjectTimeFactory, subjectInitial,
            "periodic HLAlogicalTime reflection after HLAsetTiming", beforeInitialReport);

         LogicalTime firstTarget = offset(subjectInitial, subjectEpsilon, 2);
         LogicalTime firstMonitorTarget = offset(monitorInitial, monitorEpsilon, 2);
         JavaTckSupport.check(firstTarget.equals(firstMonitorTarget),
            "first logical-time regulator and subject targets did not agree");
         advanceAndObserve(monitor, subject, monitorRecorder, subjectRecorder, callbackModel,
            momObject, logicalTimeAttribute, reliable, subjectTimeFactory,
            firstMonitorTarget, firstTarget, "first");

         LogicalTime secondTarget = offset(subjectInitial, subjectEpsilon, 4);
         LogicalTime secondMonitorTarget = offset(monitorInitial, monitorEpsilon, 4);
         JavaTckSupport.check(secondTarget.equals(secondMonitorTarget)
               && compare(secondTarget, firstTarget) > 0,
            "second logical-time regulator and subject targets did not agree or advance");
         advanceAndObserve(monitor, subject, monitorRecorder, subjectRecorder, callbackModel,
            momObject, logicalTimeAttribute, reliable, subjectTimeFactory,
            secondMonitorTarget, secondTarget, "second");

         sendReportPeriod(monitor, setTiming, targetFederate, reportPeriod, subjectFederate,
            0, encoder);
         subject.disableTimeConstrained();
         subjectConstrained = false;
         monitor.disableTimeRegulation();
         monitorRegulating = false;
         monitor.unsubscribeObjectClassAttributes(momClass, subscription);
         subscribed = false;
         subject.resignFederationExecution(ResignAction.NO_ACTION);
         subjectJoined = false;
         monitor.resignFederationExecution(ResignAction.NO_ACTION);
         monitorJoined = false;
         monitor.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (subjectConstrained && subjectJoined) try { subject.disableTimeConstrained(); }
            catch (Exception ignored) { }
         if (monitorRegulating && monitorJoined) try { monitor.disableTimeRegulation(); }
            catch (Exception ignored) { }
         if (subscribed && monitorJoined && momClass != null && subscription != null) try {
            monitor.unsubscribeObjectClassAttributes(momClass, subscription);
         } catch (Exception ignored) { }
         if (subjectJoined) try { subject.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (monitorJoined) try { monitor.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (created) try { monitor.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (subjectConnected) try { subject.disconnect(); } catch (Exception ignored) { }
         if (monitorConnected) try { monitor.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static void sendReportPeriod(RTIambassador sender, InteractionClassHandle interaction,
         ParameterHandle targetFederate, ParameterHandle reportPeriod, FederateHandle subject,
         int seconds, EncoderFactory encoder) throws Exception {
      byte[] encodedFederate = new byte[subject.encodedLength()];
      subject.encode(encodedFederate, 0);
      HLAinteger32BE encodedPeriod = encoder.createHLAinteger32BE(seconds);
      ParameterHandleValueMap parameters = sender.getParameterHandleValueMapFactory().create(2);
      parameters.put(targetFederate, encodedFederate);
      parameters.put(reportPeriod, encodedPeriod.toByteArray());
      sender.sendInteraction(interaction, parameters, new byte[0]);
   }

   private static void advanceAndObserve(RTIambassador monitor, RTIambassador subject,
         Recorder monitorRecorder, Recorder subjectRecorder, CallbackModel callbackModel,
         ObjectInstanceHandle momObject, AttributeHandle logicalTimeAttribute,
         TransportationTypeHandle reliable, LogicalTimeFactory<?, ?> timeFactory,
         LogicalTime monitorTarget, LogicalTime subjectTarget, String label) throws Exception {
      int monitorGrantBefore = monitorRecorder.timeAdvanceGrantTimes.size();
      int subjectGrantBefore = subjectRecorder.timeAdvanceGrantTimes.size();
      int reflectionBefore = monitorRecorder.reflections.size();
      monitor.timeAdvanceRequest(monitorTarget);
      subject.timeAdvanceRequest(subjectTarget);
      awaitBoth(monitor, subject, callbackModel,
         () -> monitorRecorder.timeAdvanceGrantTimes.size() >= monitorGrantBefore + 1
            && subjectRecorder.timeAdvanceGrantTimes.size() >= subjectGrantBefore + 1,
         label + " logical-time Time Advance Grants");
      JavaTckSupport.check(monitorRecorder.timeAdvanceGrantTimes.size() == monitorGrantBefore + 1
            && monitorTarget.equals(monitorRecorder.timeAdvanceGrantTimes.get(monitorGrantBefore))
            && subjectRecorder.timeAdvanceGrantTimes.size() == subjectGrantBefore + 1
            && subjectTarget.equals(subjectRecorder.timeAdvanceGrantTimes.get(subjectGrantBefore)),
         label + " logical-time regulator or subject grant returned the wrong time");
      LogicalTime queried = subject.queryLogicalTime();
      JavaTckSupport.check(subjectTarget.equals(queried),
         "Query Logical Time did not match the " + label + " successful grant");
      awaitLogicalTimeReflection(monitor, monitorRecorder, callbackModel, momObject,
         logicalTimeAttribute, reliable, timeFactory, queried,
         "periodic HLAlogicalTime reflection after the " + label + " grant", reflectionBefore);
   }

   private static void awaitLogicalTimeReflection(RTIambassador monitor, Recorder recorder,
         CallbackModel callbackModel, ObjectInstanceHandle object, AttributeHandle attribute,
         TransportationTypeHandle reliable, LogicalTimeFactory<?, ?> timeFactory,
         LogicalTime expected, String description) throws Exception {
      awaitLogicalTimeReflection(monitor, recorder, callbackModel, object, attribute, reliable,
         timeFactory, expected, description, 0);
   }

   private static void awaitLogicalTimeReflection(RTIambassador monitor, Recorder recorder,
         CallbackModel callbackModel, ObjectInstanceHandle object, AttributeHandle attribute,
         TransportationTypeHandle reliable, LogicalTimeFactory<?, ?> timeFactory,
         LogicalTime expected, String description, int firstReflection) throws Exception {
      await(monitor, callbackModel, () -> recorder.valueReflectionsSince(
         firstReflection, object, attribute).stream().anyMatch(reflection ->
            matchesTime(reflection, object, attribute, timeFactory, expected)), description);
      List<Reflection> reflections = recorder.valueReflectionsSince(firstReflection,
         object, attribute);
      Reflection match = reflections.stream().filter(reflection ->
         matchesTime(reflection, object, attribute, timeFactory, expected)).findFirst().orElse(null);
      JavaTckSupport.check(match != null,
         description + " was not recorded after callback servicing");
      validateTimeReflection(match, object, attribute, reliable, timeFactory, expected, description);
   }

   private static boolean matchesTime(Reflection reflection, ObjectInstanceHandle object,
         AttributeHandle attribute, LogicalTimeFactory<?, ?> timeFactory, LogicalTime expected) {
      if (!reflection.object.equals(object) || !reflection.values.containsKey(attribute)) return false;
      try {
         Object actual = timeFactory.decodeTime(reflection.values.get(attribute), 0);
         return expected.equals(actual);
      } catch (Exception ignored) {
         return false;
      }
   }

   private static void validateTimeReflection(Reflection reflection, ObjectInstanceHandle object,
         AttributeHandle attribute, TransportationTypeHandle reliable,
         LogicalTimeFactory<?, ?> timeFactory, LogicalTime expected, String description)
         throws Exception {
      JavaTckSupport.check(object.equals(reflection.object) && reflection.values.size() == 1
            && reflection.values.containsKey(attribute) && reflection.tag.length == 0
            && reliable.equals(reflection.transportation) && reflection.producer == null
            && (reflection.regions == null || reflection.regions.isEmpty())
            && !reflection.timestampOrdered,
         description + " had non-standard HLAlogicalTime reflection metadata");
      Object decoded = timeFactory.decodeTime(reflection.values.get(attribute), 0);
      JavaTckSupport.check(expected.equals(decoded),
         description + " did not contain the encoded standard logical time");
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static int compare(LogicalTime left, LogicalTime right) {
      return left.compareTo(right);
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) pump(ambassador, model);
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void awaitBoth(RTIambassador first, RTIambassador second, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         pump(first, model);
         pump(second, model);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void pump(RTIambassador ambassador, CallbackModel model) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
      else Thread.yield();
      Thread.sleep(1L);
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final AttributeHandleValueMap values;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final RegionHandleSet regions;
      private final boolean timestampOrdered;
      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap values,
            byte[] tag, TransportationTypeHandle transportation, FederateHandle producer,
            RegionHandleSet regions, boolean timestampOrdered) {
         this.object = object;
         this.values = values.clone();
         this.tag = tag.clone();
         this.transportation = transportation;
         this.producer = producer;
         this.regions = regions == null ? null : (RegionHandleSet) regions.clone();
         this.timestampOrdered = timestampOrdered;
      }
   }

   private static final class Recorder {
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> timeAdvanceGrantTimes = new CopyOnWriteArrayList<>();
      private volatile int timeConstrainedEnabled;
      private volatile int timeRegulationEnabled;

      private ObjectInstanceHandle momObject(AttributeHandle handle, byte[] expected) {
         for (Reflection reflection : reflections) {
            byte[] actual = reflection.values.get(handle);
            if (actual != null && Arrays.equals(expected, actual)) return reflection.object;
         }
         return null;
      }

      private List<Reflection> valueReflectionsSince(int first, ObjectInstanceHandle object,
            AttributeHandle attribute) {
         List<Reflection> result = new ArrayList<>();
         for (int i = Math.min(first, reflections.size()); i < reflections.size(); ++i) {
            Reflection reflection = reflections.get(i);
            if (object.equals(reflection.object) && reflection.values.containsKey(attribute))
               result.add(reflection);
         }
         return result;
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class}, (proxy, method, args) -> {
               String name = method.getName();
               if ("reflectAttributeValues".equals(name) && args != null && args.length == 6) {
                  reflections.add(new Reflection((ObjectInstanceHandle) args[0],
                     (AttributeHandleValueMap) args[1], (byte[]) args[2],
                     (TransportationTypeHandle) args[3], (FederateHandle) args[4],
                     (RegionHandleSet) args[5], false));
               } else if ("timeAdvanceGrant".equals(name) && args != null
                     && args.length == 1) {
                  timeAdvanceGrantTimes.add((LogicalTime<?, ?>) args[0]);
               } else if ("timeConstrainedEnabled".equals(name)) {
                  ++timeConstrainedEnabled;
               } else if ("timeRegulationEnabled".equals(name)) {
                  ++timeRegulationEnabled;
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK logical-time MOM callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (args == null ? null : args[0]);
               }
               return null;
            });
      }
   }
}
