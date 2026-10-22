package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
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

/** Observes standard MOM TimeAdvancing/TimeGranted states across all advance services. */
final class FederateTimeManagerStateMomTck {
   private static final String MOM_CLASS = "HLAobjectRoot.HLAmanager.HLAfederate";
   private static final String FEDERATE_HANDLE = "HLAfederateHandle";
   private static final String TIME_MANAGER_STATE = "HLAtimeManagerState";
   private static final long TIMEOUT_NANOS = 10_000_000_000L;

   private FederateTimeManagerStateMomTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador monitor = factory.getRtiAmbassador();
      RTIambassador subject = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder monitorRecorder = new Recorder();
      Recorder subjectRecorder = new Recorder();
      String federation = "java-tck-federate-time-manager-state-mom-" + UUID.randomUUID();
      boolean monitorConnected = false, subjectConnected = false, created = false;
      boolean monitorJoined = false, subjectJoined = false, subscribed = false;
      ObjectClassHandle momClass = null;
      AttributeHandleSet subscriptions = null;
      try {
         monitor.connect(monitorRecorder.proxy(), callbackModel);
         monitorConnected = true;
         subject.connect(subjectRecorder.proxy(), callbackModel);
         subjectConnected = true;
         monitor.createFederationExecutionWithMIM(federation, new String[] {fom}, mim,
            timeImplementation);
         created = true;
         monitor.joinFederationExecution("time-manager-state-monitor", "java-tck", federation);
         monitorJoined = true;

         momClass = monitor.getObjectClassHandle(MOM_CLASS);
         AttributeHandle federateHandleAttribute = monitor.getAttributeHandle(momClass,
            FEDERATE_HANDLE);
         AttributeHandle stateAttribute = monitor.getAttributeHandle(momClass,
            TIME_MANAGER_STATE);
         TransportationTypeHandle reliable = monitor.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(MOM_CLASS.equals(monitor.getObjectClassName(momClass))
               && FEDERATE_HANDLE.equals(monitor.getAttributeName(momClass,
                  federateHandleAttribute))
               && TIME_MANAGER_STATE.equals(monitor.getAttributeName(momClass, stateAttribute))
               && "HLAreliable".equals(monitor.getTransportationTypeName(reliable)),
            "time-manager-state MOM lookup did not round-trip standard MIM handles");
         subscriptions = JavaTckSupport.attributeSet(monitor, federateHandleAttribute,
            stateAttribute);
         monitor.subscribeObjectClassAttributes(momClass, subscriptions, "");
         subscribed = true;

         FederateHandle subjectFederate = subject.joinFederationExecution(
            "time-manager-state-subject", "java-tck", federation);
         subjectJoined = true;
         LogicalTimeFactory<?, ?> timeFactory = subject.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(monitor.getTimeFactory().getName()),
            "time-manager-state members selected different logical-time factories");
         LogicalTime initial = (LogicalTime) timeFactory.makeInitial();
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         JavaTckSupport.check(initial.isInitial() && epsilon.isEpsilon(),
            "adapter logical-time factory returned invalid initial/epsilon values");

         byte[] encodedSubject = new byte[subjectFederate.encodedLength()];
         subjectFederate.encode(encodedSubject, 0);
         await(monitor, callbackModel, () -> monitorRecorder.momObject(
            federateHandleAttribute, encodedSubject) != null,
            "subject federate MOM discovery");
         ObjectInstanceHandle momObject = monitorRecorder.momObject(
            federateHandleAttribute, encodedSubject);
         JavaTckSupport.check(momObject != null
               && momClass.equals(monitor.getKnownObjectClassHandle(momObject)),
            "time-manager-state discovery returned the wrong standard MOM object");

         for (int step = 1; step <= 5; ++step) {
            LogicalTime target = offset(initial, epsilon, step);
            Advance advance = Advance.values()[step - 1];
            applyAdvance(monitor, subject, monitorRecorder, subjectRecorder, callbackModel,
               encoder, momObject, stateAttribute, reliable, target, advance);
         }

         monitor.unsubscribeObjectClassAttributes(momClass, subscriptions);
         subscribed = false;
         subject.resignFederationExecution(ResignAction.NO_ACTION);
         subjectJoined = false;
         monitor.resignFederationExecution(ResignAction.NO_ACTION);
         monitorJoined = false;
         monitor.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (subscribed && monitorJoined && momClass != null && subscriptions != null) try {
            monitor.unsubscribeObjectClassAttributes(momClass, subscriptions);
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

   private static void applyAdvance(RTIambassador monitor, RTIambassador subject,
         Recorder monitorRecorder, Recorder subjectRecorder, CallbackModel callbackModel,
         EncoderFactory encoder, ObjectInstanceHandle momObject, AttributeHandle stateAttribute,
         TransportationTypeHandle reliable, LogicalTime target, Advance advance) throws Exception {
      int reflectionBefore = monitorRecorder.reflections.size();
      int timeGrantBefore = subjectRecorder.timeAdvanceGrantTimes.size();
      int flushGrantBefore = subjectRecorder.flushQueueGrantTimes.size();
      switch (advance) {
         case TAR: subject.timeAdvanceRequest(target); break;
         case TARA: subject.timeAdvanceRequestAvailable(target); break;
         case NMR: subject.nextMessageRequest(target); break;
         case NMRA: subject.nextMessageRequestAvailable(target); break;
         case FLUSH: subject.flushQueueRequest(target); break;
         default: throw new IllegalStateException("unknown time advance kind");
      }

      awaitBoth(monitor, subject, callbackModel, () ->
         (advance.flushQueueGrant
            ? subjectRecorder.flushQueueGrantTimes.size() >= flushGrantBefore + 1
            : subjectRecorder.timeAdvanceGrantTimes.size() >= timeGrantBefore + 1)
            && monitorRecorder.valueReflectionsSince(reflectionBefore, momObject,
               stateAttribute).size() >= 2,
         "HLAtimeManagerState transitions after " + advance.displayName);

      if (advance.flushQueueGrant) {
         JavaTckSupport.check(subjectRecorder.flushQueueGrantTimes.size() == flushGrantBefore + 1
               && subjectRecorder.timeAdvanceGrantTimes.size() == timeGrantBefore
               && subjectRecorder.flushQueueGrantTime != null
               && subjectRecorder.flushQueueOptimisticTime != null,
            "Flush Queue Request did not return exactly one standard Flush Queue Grant");
      } else {
         JavaTckSupport.check(subjectRecorder.timeAdvanceGrantTimes.size() == timeGrantBefore + 1
               && subjectRecorder.flushQueueGrantTimes.size() == flushGrantBefore
               && target.equals(subjectRecorder.timeAdvanceGrantTimes.get(timeGrantBefore)),
            advance.displayName + " returned the wrong Time Advance Grant");
      }

      boolean sawAdvancing = false;
      boolean sawGranted = false;
      List<Reflection> stateReflections = monitorRecorder.valueReflectionsSince(
         reflectionBefore, momObject, stateAttribute);
      for (Reflection reflection : stateReflections) {
         int state = decodeState(monitor, reflection, momObject, stateAttribute, reliable, encoder,
            advance.displayName);
         JavaTckSupport.check(state == 0 || state == 1,
            advance.displayName + " returned a value outside the standard time-state enum");
         sawAdvancing |= state == 1;
         sawGranted |= state == 0;
      }
      JavaTckSupport.check(sawAdvancing && sawGranted,
         advance.displayName + " did not report both TimeAdvancing and TimeGranted");
   }

   private static int decodeState(RTIambassador monitor, Reflection reflection,
         ObjectInstanceHandle object,
         AttributeHandle attribute, TransportationTypeHandle reliable, EncoderFactory encoder,
         String description) throws Exception {
      JavaTckSupport.check(object.equals(reflection.object) && reflection.values.size() == 1
            && Arrays.equals(new byte[0], reflection.tag)
            && reliable.equals(reflection.transportation)
            && (reflection.regions == null || reflection.regions.isEmpty())
            && !reflection.timestampOrdered,
         description + " had non-standard time-manager-state reflection metadata");
      JavaTckSupport.requireNoProducingFederate(monitor, reflection.producer, description);
      HLAinteger32BE state = encoder.createHLAinteger32BE();
      state.decode(reflection.values.get(attribute));
      return state.getValue();
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(epsilon);
      return result;
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         pump(ambassador, model);
      }
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

   private enum Advance {
      TAR("Time Advance Request", false), TARA("Time Advance Request Available", false),
      NMR("Next Message Request", false), NMRA("Next Message Request Available", false),
      FLUSH("Flush Queue Request", true);
      private final String displayName;
      private final boolean flushQueueGrant;
      Advance(String displayName, boolean flushQueueGrant) {
         this.displayName = displayName;
         this.flushQueueGrant = flushQueueGrant;
      }
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
      private final List<LogicalTime<?, ?>> flushQueueGrantTimes = new CopyOnWriteArrayList<>();
      private volatile LogicalTime<?, ?> flushQueueGrantTime;
      private volatile LogicalTime<?, ?> flushQueueOptimisticTime;

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
               } else if ("flushQueueGrant".equals(name) && args != null
                     && args.length == 2) {
                  flushQueueGrantTime = (LogicalTime<?, ?>) args[0];
                  flushQueueOptimisticTime = (LogicalTime<?, ?>) args[1];
                  flushQueueGrantTimes.add(flushQueueGrantTime);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK time-manager MOM callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (args == null ? null : args[0]);
               }
               return null;
            });
      }
   }
}
