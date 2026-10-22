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
import hla.rti1516_2025.time.LogicalTimeInterval;
import hla.rti1516_2025.time.LogicalTimeFactory;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies periodic standard federate-MOM HLAlookahead values. */
final class FederateLookaheadMomPeriodicTck {
   private static final String MOM_CLASS = "HLAobjectRoot.HLAmanager.HLAfederate";
   private static final String MOM_FEDERATE_HANDLE = "HLAfederateHandle";
   private static final String MOM_LOOKAHEAD = "HLAlookahead";
   private static final String SET_TIMING =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming";
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private FederateLookaheadMomPeriodicTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador monitor = factory.getRtiAmbassador();
      RTIambassador subject = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder monitorRecorder = new Recorder();
      Recorder subjectRecorder = new Recorder();
      String federation = "java-tck-federate-lookahead-mom-" + UUID.randomUUID();
      boolean monitorConnected = false, subjectConnected = false, created = false;
      boolean monitorJoined = false, subjectJoined = false, subscribed = false;
      boolean regulating = false;
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
         monitor.joinFederationExecution("lookahead-mom-monitor", "java-tck", federation);
         monitorJoined = true;

         momClass = monitor.getObjectClassHandle(MOM_CLASS);
         AttributeHandle federateHandleAttribute = monitor.getAttributeHandle(momClass,
            MOM_FEDERATE_HANDLE);
         AttributeHandle lookaheadAttribute = monitor.getAttributeHandle(momClass, MOM_LOOKAHEAD);
         TransportationTypeHandle reliable = monitor.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(MOM_CLASS.equals(monitor.getObjectClassName(momClass))
               && MOM_FEDERATE_HANDLE.equals(monitor.getAttributeName(momClass,
                  federateHandleAttribute))
               && MOM_LOOKAHEAD.equals(monitor.getAttributeName(momClass, lookaheadAttribute))
               && "HLAreliable".equals(monitor.getTransportationTypeName(reliable)),
            "HLAlookahead MOM lookup did not round-trip standard MIM handles");
         subscription = JavaTckSupport.attributeSet(monitor, federateHandleAttribute,
            lookaheadAttribute);
         monitor.subscribeObjectClassAttributes(momClass, subscription, "");
         subscribed = true;

         FederateHandle subjectFederate = subject.joinFederationExecution(
            "lookahead-mom-subject", "java-tck", federation);
         subjectJoined = true;
         LogicalTimeFactory<?, ?> timeFactory = subject.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(monitor.getTimeFactory().getName()),
            "HLAlookahead MOM members selected different standard time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();

         byte[] encodedSubject = new byte[subjectFederate.encodedLength()];
         subjectFederate.encode(encodedSubject, 0);
         await(monitor, callbackModel, () -> monitorRecorder.momObject(
            federateHandleAttribute, encodedSubject) != null,
            "HLAlookahead subject MOM discovery");
         ObjectInstanceHandle momObject = monitorRecorder.momObject(
            federateHandleAttribute, encodedSubject);
         JavaTckSupport.check(momObject != null
               && momClass.equals(monitor.getKnownObjectClassHandle(momObject)),
            "HLAlookahead discovery returned the wrong standard MOM object");

         int regulatingBefore = subjectRecorder.timeRegulationEnabled;
         subject.enableTimeRegulation(epsilon);
         regulating = true;
         await(subject, callbackModel,
            () -> subjectRecorder.timeRegulationEnabled >= regulatingBefore + 1,
            "timeRegulationEnabled callback for HLAlookahead MOM subject");
         JavaTckSupport.check(subjectRecorder.timeRegulationEnabled == regulatingBefore + 1,
            "Enable Time Regulation delivered an unexpected number of callbacks");
         LogicalTimeInterval initialLookahead = subject.queryLookahead();
         JavaTckSupport.check(epsilon.equals(initialLookahead),
            "initial Query Lookahead did not match the enabled epsilon");

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
         awaitLookaheadReflection(monitor, monitorRecorder, callbackModel, momObject,
            lookaheadAttribute, reliable, timeFactory, initialLookahead,
            "periodic HLAlookahead reflection after HLAsetTiming", beforeInitialReport);

         int beforeChangedReport = monitorRecorder.reflections.size();
         LogicalTimeInterval changedLookahead = intervalOffset(epsilon, 3);
         subject.modifyLookahead(changedLookahead);
         LogicalTimeInterval queriedChanged = subject.queryLookahead();
         JavaTckSupport.check(changedLookahead.equals(queriedChanged),
            "Modify Lookahead did not cross the Query Lookahead boundary");
         awaitLookaheadReflection(monitor, monitorRecorder, callbackModel, momObject,
            lookaheadAttribute, reliable, timeFactory, queriedChanged,
            "periodic HLAlookahead reflection after Modify Lookahead", beforeChangedReport);

         sendReportPeriod(monitor, setTiming, targetFederate, reportPeriod, subjectFederate,
            0, encoder);
         subject.disableTimeRegulation();
         regulating = false;
         monitor.unsubscribeObjectClassAttributes(momClass, subscription);
         subscribed = false;
         subject.resignFederationExecution(ResignAction.NO_ACTION);
         subjectJoined = false;
         monitor.resignFederationExecution(ResignAction.NO_ACTION);
         monitorJoined = false;
         monitor.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (regulating && subjectJoined) try { subject.disableTimeRegulation(); }
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

   private static void awaitLookaheadReflection(RTIambassador monitor, Recorder recorder,
         CallbackModel callbackModel, ObjectInstanceHandle object, AttributeHandle attribute,
         TransportationTypeHandle reliable, LogicalTimeFactory<?, ?> timeFactory,
         LogicalTimeInterval expected, String description, int firstReflection) throws Exception {
      await(monitor, callbackModel, () -> recorder.valueReflectionsSince(
         firstReflection, object, attribute).stream().anyMatch(reflection ->
            matchesLookahead(reflection, object, attribute, timeFactory, expected)), description);
      List<Reflection> reflections = recorder.valueReflectionsSince(firstReflection,
         object, attribute);
      Reflection match = reflections.stream().filter(reflection ->
         matchesLookahead(reflection, object, attribute, timeFactory, expected))
         .findFirst().orElse(null);
      JavaTckSupport.check(match != null,
         description + " was not recorded after callback servicing");
      JavaTckSupport.check(match.values.size() == 1 && match.tag.length == 0
            && reliable.equals(match.transportation)
            && (match.regions == null || match.regions.isEmpty()) && !match.timestampOrdered,
         description + " had non-standard HLAlookahead reflection metadata");
      JavaTckSupport.requireNoProducingFederate(monitor, match.producer, description);
      JavaTckSupport.check(expected.equals(timeFactory.decodeInterval(
            match.values.get(attribute), 0)),
         description + " did not contain the encoded standard logical-time interval");
   }

   private static boolean matchesLookahead(Reflection reflection, ObjectInstanceHandle object,
         AttributeHandle attribute, LogicalTimeFactory<?, ?> timeFactory,
         LogicalTimeInterval expected) {
      if (!object.equals(reflection.object) || !reflection.values.containsKey(attribute))
         return false;
      try {
         return expected.equals(timeFactory.decodeInterval(reflection.values.get(attribute), 0));
      } catch (Exception ignored) {
         return false;
      }
   }

   private static LogicalTimeInterval intervalOffset(LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTimeInterval result = epsilon;
      for (int i = 1; i < steps; ++i) result = (LogicalTimeInterval) result.add(epsilon);
      return result;
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
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
               } else if ("timeRegulationEnabled".equals(name)) {
                  ++timeRegulationEnabled;
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK HLAlookahead MOM callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (args == null ? null : args[0]);
               }
               return null;
            });
      }
   }
}
