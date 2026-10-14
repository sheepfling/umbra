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
import hla.rti1516_2025.TimeQueryReturn;
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

/** Checks direct and periodic standard MOM reporting of federation GALT and LITS. */
final class FederateMomGaltLitsPeriodicTck {
   private static final String MOM_CLASS = "HLAobjectRoot.HLAmanager.HLAfederate";
   private static final String FEDERATE_HANDLE = "HLAfederateHandle";
   private static final String GALT = "HLAGALT";
   private static final String LITS = "HLALITS";
   private static final String SET_TIMING =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming";
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private FederateMomGaltLitsPeriodicTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador monitor = factory.getRtiAmbassador();
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador regulator = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder monitorRecorder = new Recorder();
      Recorder subjectRecorder = new Recorder();
      Recorder regulatorRecorder = new Recorder();
      String federation = "java-tck-federate-mom-galt-lits-" + UUID.randomUUID();
      boolean monitorConnected = false, subjectConnected = false, regulatorConnected = false;
      boolean created = false, monitorJoined = false, subjectJoined = false, regulatorJoined = false;
      boolean subscribed = false, regulating = false;
      ObjectClassHandle momClass = null;
      AttributeHandleSet subscription = null;
      try {
         monitor.connect(monitorRecorder.proxy(), callbackModel);
         monitorConnected = true;
         subject.connect(subjectRecorder.proxy(), callbackModel);
         subjectConnected = true;
         regulator.connect(regulatorRecorder.proxy(), callbackModel);
         regulatorConnected = true;
         monitor.createFederationExecutionWithMIM(federation, new String[] {fom}, mim,
            timeImplementation);
         created = true;
         monitor.joinFederationExecution("galt-lits-monitor", "java-tck", federation);
         monitorJoined = true;

         momClass = monitor.getObjectClassHandle(MOM_CLASS);
         AttributeHandle federateAttribute = monitor.getAttributeHandle(momClass, FEDERATE_HANDLE);
         AttributeHandle galtAttribute = monitor.getAttributeHandle(momClass, GALT);
         AttributeHandle litsAttribute = monitor.getAttributeHandle(momClass, LITS);
         TransportationTypeHandle reliable = monitor.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(MOM_CLASS.equals(monitor.getObjectClassName(momClass))
               && FEDERATE_HANDLE.equals(monitor.getAttributeName(momClass, federateAttribute))
               && GALT.equals(monitor.getAttributeName(momClass, galtAttribute))
               && LITS.equals(monitor.getAttributeName(momClass, litsAttribute))
               && "HLAreliable".equals(monitor.getTransportationTypeName(reliable)),
            "standard federate MOM GALT/LITS lookup did not round-trip");
         subscription = JavaTckSupport.attributeSet(monitor, federateAttribute,
            galtAttribute, litsAttribute);
         monitor.subscribeObjectClassAttributes(momClass, subscription, "");
         subscribed = true;

         regulator.joinFederationExecution("galt-lits-regulator", "java-tck", federation);
         regulatorJoined = true;
         LogicalTimeFactory<?, ?> timeFactory = regulator.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(monitor.getTimeFactory().getName()),
            "federates selected different logical-time factories");
         LogicalTime<?, ?> initial = (LogicalTime<?, ?>) timeFactory.makeInitial();
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         LogicalTimeInterval lookahead = offset(epsilon, 2);
         regulator.enableTimeRegulation(lookahead);
         regulating = true;
         await(regulator, callbackModel, () -> regulatorRecorder.timeRegulationEnabled == 1,
            "regulator timeRegulationEnabled callback");

         FederateHandle subjectHandle = subject.joinFederationExecution(
            "galt-lits-subject", "java-tck", federation);
         subjectJoined = true;
         byte[] encodedSubject = encode(subjectHandle);
         await(monitor, callbackModel,
            () -> monitorRecorder.momObject(federateAttribute, encodedSubject) != null,
            "subject federate MOM discovery");
         ObjectInstanceHandle momObject = monitorRecorder.momObject(federateAttribute,
            encodedSubject);
         JavaTckSupport.check(momObject != null
               && momClass.equals(monitor.getKnownObjectClassHandle(momObject)),
            "federate MOM discovery returned the wrong object class");

         LogicalTime<?, ?> expectedBound = add(initial, lookahead);
         expectBound(monitor, expectedBound, true, "direct Query GALT");
         expectBound(monitor, expectedBound, false, "direct Query LITS");
         requestBounds(monitor, monitorRecorder, callbackModel, momObject, galtAttribute,
            litsAttribute, reliable, timeFactory, expectedBound, false,
            "direct GALT/LITS attribute value request");

         InteractionClassHandle setTiming = monitor.getInteractionClassHandle(SET_TIMING);
         ParameterHandle timingFederate = monitor.getParameterHandle(setTiming, "HLAfederate");
         ParameterHandle reportPeriod = monitor.getParameterHandle(setTiming, "HLAreportPeriod");
         JavaTckSupport.check(SET_TIMING.equals(monitor.getInteractionClassName(setTiming))
               && "HLAfederate".equals(monitor.getParameterName(setTiming, timingFederate))
               && "HLAreportPeriod".equals(monitor.getParameterName(setTiming, reportPeriod)),
            "standard HLAsetTiming lookup did not round-trip");
         int firstPeriodicReflection = monitorRecorder.reflections.size();
         sendTiming(monitor, setTiming, timingFederate, reportPeriod, subjectHandle, 1, encoder);
         await(monitor, callbackModel,
            () -> findBounds(monitorRecorder, firstPeriodicReflection, momObject,
               galtAttribute, litsAttribute, false) != null,
            "periodic GALT/LITS report");
         verifyBounds(findBounds(monitorRecorder, firstPeriodicReflection, momObject,
            galtAttribute, litsAttribute, false), galtAttribute, litsAttribute, reliable,
            timeFactory, expectedBound, false, "periodic GALT/LITS report");

         sendTiming(monitor, setTiming, timingFederate, reportPeriod, subjectHandle, 0, encoder);
         regulator.disableTimeRegulation();
         regulating = false;
         expectUndefined(monitor, true, "Query GALT after the last regulator disables regulation");
         expectUndefined(monitor, false, "Query LITS after the last regulator disables regulation");
         requestBounds(monitor, monitorRecorder, callbackModel, momObject, galtAttribute,
            litsAttribute, reliable, timeFactory, null, true,
            "undefined GALT/LITS attribute value request");

         monitor.unsubscribeObjectClassAttributes(momClass, subscription);
         subscribed = false;
         subject.resignFederationExecution(ResignAction.NO_ACTION);
         subjectJoined = false;
         regulator.resignFederationExecution(ResignAction.NO_ACTION);
         regulatorJoined = false;
         monitor.resignFederationExecution(ResignAction.NO_ACTION);
         monitorJoined = false;
         monitor.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (regulating && regulatorJoined) try { regulator.disableTimeRegulation(); }
            catch (Exception ignored) { }
         if (subscribed && monitorJoined && momClass != null && subscription != null) try {
            monitor.unsubscribeObjectClassAttributes(momClass, subscription);
         } catch (Exception ignored) { }
         if (subjectJoined) resign(subject);
         if (regulatorJoined) resign(regulator);
         if (monitorJoined) resign(monitor);
         if (created) try { monitor.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (subjectConnected) disconnect(subject);
         if (regulatorConnected) disconnect(regulator);
         if (monitorConnected) disconnect(monitor);
      }
   }

   private static void requestBounds(RTIambassador monitor, Recorder recorder,
         CallbackModel callbackModel, ObjectInstanceHandle object, AttributeHandle galtAttribute,
         AttributeHandle litsAttribute, TransportationTypeHandle reliable,
         LogicalTimeFactory<?, ?> timeFactory, LogicalTime expected, boolean undefined,
         String description) throws Exception {
      int first = recorder.reflections.size();
      AttributeHandleSet requested = JavaTckSupport.attributeSet(monitor,
         galtAttribute, litsAttribute);
      monitor.requestAttributeValueUpdate(object, requested, new byte[0]);
      await(monitor, callbackModel,
         () -> findBounds(recorder, first, object, galtAttribute, litsAttribute, undefined) != null,
         description);
      Reflection reflection = findBounds(recorder, first, object, galtAttribute, litsAttribute,
         undefined);
      JavaTckSupport.check(reflection != null, description + " produced no MOM reflection");
      verifyBounds(reflection, galtAttribute, litsAttribute, reliable, timeFactory, expected,
         undefined, description);
   }

   private static void verifyBounds(Reflection reflection, AttributeHandle galtAttribute,
         AttributeHandle litsAttribute, TransportationTypeHandle reliable,
         LogicalTimeFactory<?, ?> timeFactory, LogicalTime expected, boolean undefined,
         String description) throws Exception {
      JavaTckSupport.check(reflection.values.size() == 2 && reflection.tag.length == 0
            && reliable.equals(reflection.transportation) && reflection.producer == null
            && (reflection.regions == null || reflection.regions.isEmpty())
            && !reflection.timestampOrdered,
         description + " returned non-standard MOM reflection metadata");
      byte[] galt = reflection.values.get(galtAttribute);
      byte[] lits = reflection.values.get(litsAttribute);
      JavaTckSupport.check(galt != null && lits != null,
         description + " omitted a requested GALT or LITS value");
      if (undefined) {
         JavaTckSupport.check(galt.length == 0 && lits.length == 0,
            description + " did not encode undefined bounds as empty values");
      } else {
         JavaTckSupport.check(expected.equals(timeFactory.decodeTime(galt, 0))
               && expected.equals(timeFactory.decodeTime(lits, 0)),
            description + " returned incorrect logical-time encodings");
      }
   }

   private static Reflection findBounds(Recorder recorder, int first, ObjectInstanceHandle object,
         AttributeHandle galtAttribute, AttributeHandle litsAttribute, boolean undefined) {
      List<Reflection> reflections = recorder.reflections;
      for (int i = Math.min(first, reflections.size()); i < reflections.size(); ++i) {
         Reflection reflection = reflections.get(i);
         if (!object.equals(reflection.object) || reflection.values.size() != 2
               || !reflection.values.containsKey(galtAttribute)
               || !reflection.values.containsKey(litsAttribute)) continue;
         byte[] galt = reflection.values.get(galtAttribute);
         byte[] lits = reflection.values.get(litsAttribute);
         if (galt == null || lits == null) continue;
         if (undefined ? galt.length == 0 && lits.length == 0
               : galt.length != 0 && lits.length != 0) return reflection;
      }
      return null;
   }

   private static void sendTiming(RTIambassador sender, InteractionClassHandle interaction,
         ParameterHandle federateParameter, ParameterHandle periodParameter,
         FederateHandle federate, int seconds, EncoderFactory encoder) throws Exception {
      ParameterHandleValueMap parameters = sender.getParameterHandleValueMapFactory().create(2);
      parameters.put(federateParameter, encode(federate));
      HLAinteger32BE period = encoder.createHLAinteger32BE(seconds);
      parameters.put(periodParameter, period.toByteArray());
      sender.sendInteraction(interaction, parameters, new byte[0]);
   }

   private static byte[] encode(FederateHandle handle) throws Exception {
      byte[] bytes = new byte[handle.encodedLength()];
      handle.encode(bytes, 0);
      return bytes;
   }

   private static void expectBound(RTIambassador ambassador, LogicalTime expected,
         boolean galt, String description) throws Exception {
      TimeQueryReturn result = galt ? ambassador.queryGALT() : ambassador.queryLITS();
      JavaTckSupport.check(result.timeIsValid && expected.equals(result.time),
         description + " returned an undefined or incorrect logical time");
   }

   private static void expectUndefined(RTIambassador ambassador, boolean galt,
         String description) throws Exception {
      TimeQueryReturn result = galt ? ambassador.queryGALT() : ambassador.queryLITS();
      JavaTckSupport.check(!result.timeIsValid, description + " returned a defined bound");
   }

   private static LogicalTimeInterval offset(LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTimeInterval result = epsilon;
      for (int i = 1; i < steps; ++i) result = (LogicalTimeInterval) result.add(epsilon);
      return result;
   }

   private static LogicalTime add(LogicalTime time, LogicalTimeInterval interval)
         throws Exception {
      return (LogicalTime) time.add(interval);
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

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final AttributeHandleValueMap values;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final RegionHandleSet regions;
      private final boolean timestampOrdered;

      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap values, byte[] tag,
            TransportationTypeHandle transportation, FederateHandle producer,
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
            byte[] value = reflection.values.get(handle);
            if (value != null && Arrays.equals(expected, value)) return reflection.object;
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
                     (RegionHandleSet) arguments[5], false));
               } else if ("timeRegulationEnabled".equals(name)) {
                  ++timeRegulationEnabled;
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK GALT/LITS MOM callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
