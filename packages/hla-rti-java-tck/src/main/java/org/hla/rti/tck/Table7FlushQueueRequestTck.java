package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies the Table 7 MOM Flush Queue request through its public callback effect. */
final class Table7FlushQueueRequestTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final String REQUEST =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAflushQueueRequest";

   private Table7FlushQueueRequestTck() {
   }

   static void run(RtiFactory factory, String fom, String mim,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(fom != null && !fom.isEmpty()
            && mim != null && !mim.isEmpty()
            && timeImplementation != null && !timeImplementation.isEmpty(),
         "Table 7 Flush Queue testing requires adapter FOM, MIM, and time configuration");

      RTIambassador member = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-table7-flush-queue-" + suffix;
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      boolean published = false;
      InteractionClassHandle requestClass = null;
      try {
         member.connect(recorder.proxy(), callbackModel);
         connected = true;
         member.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         member.joinFederationExecution("java-tck-table7-flush-queue-" + suffix, federation);
         joined = true;

         requestClass = member.getInteractionClassHandle(REQUEST);
         JavaTckSupport.check(requestClass != null,
            "standard MIM omitted HLAflushQueueRequest");
         ParameterHandle timeStampParameter =
            member.getParameterHandle(requestClass, "HLAtimeStamp");
         JavaTckSupport.check(timeStampParameter != null,
            "standard MIM omitted the HLAflushQueueRequest HLAtimeStamp parameter");
         member.publishInteractionClass(requestClass);
         published = true;

         LogicalTimeFactory<?, ?> timeFactory = member.getTimeFactory();
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         LogicalTime<?, ?> currentTime = member.queryLogicalTime();
         JavaTckSupport.check(epsilon != null && currentTime != null,
            "Flush Queue request could not resolve its adapter time values");
         LogicalTime<?, ?> requestedTime = offset(currentTime, epsilon, 1);
         byte[] encodedTimeStamp = encode(requestedTime);

         ParameterHandleValueMap parameters =
            member.getParameterHandleValueMapFactory().create(1);
         parameters.put(timeStampParameter, encodedTimeStamp);
         member.sendInteraction(requestClass, parameters, new byte[0]);
         await(member, callbackModel,
            () -> !recorder.flushQueueGrantTimes.isEmpty(),
            "Flush Queue Grant callback after HLAflushQueueRequest");

         JavaTckSupport.check(recorder.flushQueueGrantTimes.size() == 1
               && recorder.flushQueueOptimisticTimes.size() == 1,
            "HLAflushQueueRequest did not produce exactly one Flush Queue Grant");
         JavaTckSupport.check(recorder.timeAdvanceGrantTimes.isEmpty(),
            "HLAflushQueueRequest unexpectedly produced a timeAdvanceGrant callback");
         LogicalTime<?, ?> grantTime = recorder.flushQueueGrantTimes.get(0);
         LogicalTime<?, ?> optimisticTime = recorder.flushQueueOptimisticTimes.get(0);
         JavaTckSupport.check(grantTime != null && optimisticTime != null
               && grantTime.encodedLength() > 0 && optimisticTime.encodedLength() > 0,
            "HLAflushQueueRequest returned an empty granted or optimistic time");
      } finally {
         if (published && requestClass != null) {
            try { member.unpublishInteractionClass(requestClass); }
            catch (Exception ignored) { }
         }
         if (joined) {
            try { member.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { member.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) {
            try { member.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static byte[] encode(LogicalTime<?, ?> time) throws Exception {
      byte[] encoded = new byte[time.encodedLength()];
      time.encode(encoded, 0);
      return encoded;
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime<?, ?> offset(LogicalTime<?, ?> start,
         LogicalTimeInterval interval, int steps) throws Exception {
      LogicalTime<?, ?> result = start;
      for (int i = 0; i < steps; ++i) {
         result = (LogicalTime<?, ?>) ((LogicalTime) result).add(interval);
      }
      return result;
   }

   private static void await(RTIambassador member, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean()) {
         if (System.nanoTime() >= deadline) {
            throw new AssertionError("timed out waiting for " + description);
         }
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            member.evokeCallback(0.01);
         } else {
            Thread.sleep(1L);
         }
      }
   }

   private static final class Recorder {
      private final List<LogicalTime<?, ?>> flushQueueGrantTimes = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> flushQueueOptimisticTimes =
         new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> timeAdvanceGrantTimes = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("flushQueueGrant".equals(name) && arguments.length == 2) {
                  flushQueueGrantTimes.add((LogicalTime<?, ?>) arguments[0]);
                  flushQueueOptimisticTimes.add((LogicalTime<?, ?>) arguments[1]);
               } else if ("timeAdvanceGrant".equals(name) && arguments.length == 1) {
                  timeAdvanceGrantTimes.add((LogicalTime<?, ?>) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Table 7 Flush Queue recorder";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }
   }
}
