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

/** Verifies the Table 7 Next Message Request through its time-advance grant. */
final class Table7NextMessageRequestTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final String REQUEST =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAnextMessageRequest";
   private static final String REQUEST_AVAILABLE =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAnextMessageRequestAvailable";

   private Table7NextMessageRequestTck() {
   }

   static void run(RtiFactory factory, String fom, String mim,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(fom != null && !fom.isEmpty()
            && mim != null && !mim.isEmpty()
            && timeImplementation != null && !timeImplementation.isEmpty(),
         "Table 7 Next Message Request testing requires adapter FOM, MIM, and time configuration");

      RTIambassador subject = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-table7-next-message-request-" + suffix;
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      boolean published = false;
      boolean availablePublished = false;
      InteractionClassHandle requestClass = null;
      InteractionClassHandle availableRequestClass = null;
      try {
         subject.connect(recorder.proxy(), callbackModel);
         connected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         subject.joinFederationExecution("java-tck-table7-next-message-request-" + suffix,
            federation);
         joined = true;

         requestClass = subject.getInteractionClassHandle(REQUEST);
         ParameterHandle timeStampParameter =
            subject.getParameterHandle(requestClass, "HLAtimeStamp");
         availableRequestClass = subject.getInteractionClassHandle(REQUEST_AVAILABLE);
         ParameterHandle availableTimeStampParameter =
            subject.getParameterHandle(availableRequestClass, "HLAtimeStamp");
         JavaTckSupport.check(requestClass != null && timeStampParameter != null
               && availableRequestClass != null && availableTimeStampParameter != null,
            "standard MIM omitted a Next Message Request or its HLAtimeStamp parameter");
         subject.publishInteractionClass(requestClass);
         published = true;
         subject.publishInteractionClass(availableRequestClass);
         availablePublished = true;

         LogicalTimeFactory<?, ?> timeFactory = subject.getTimeFactory();
         JavaTckSupport.check(timeFactory != null,
            "Next Message Request returned no logical-time factory");
         LogicalTime<?, ?> initialTime = timeFactory.makeInitial();
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         JavaTckSupport.check(initialTime != null && epsilon != null,
            "Next Message Request could not allocate the adapter time context");
         LogicalTime<?, ?> requestedTime = offset(initialTime, epsilon, 5);
         JavaTckSupport.check(requestedTime != null,
            "Next Message Request could not calculate its requested time");
         byte[] encodedTimeStamp = encode(requestedTime);
         ParameterHandleValueMap parameters =
            subject.getParameterHandleValueMapFactory().create(1);
         parameters.put(timeStampParameter, encodedTimeStamp);

         int grantsBefore = recorder.timeAdvanceGrants.size();
         subject.sendInteraction(requestClass, parameters, new byte[0]);
         await(subject, callbackModel,
            () -> recorder.timeAdvanceGrants.size() >= grantsBefore + 1,
            "Time Advance Grant callback after HLAnextMessageRequest");
         JavaTckSupport.check(recorder.timeAdvanceGrants.size() == grantsBefore + 1
               && requestedTime.equals(recorder.timeAdvanceGrants.get(grantsBefore)),
            "HLAnextMessageRequest did not produce exactly one grant at its requested time");

         LogicalTime<?, ?> availableRequestedTime = offset(initialTime, epsilon, 6);
         ParameterHandleValueMap availableParameters =
            subject.getParameterHandleValueMapFactory().create(1);
         availableParameters.put(availableTimeStampParameter, encode(availableRequestedTime));
         int availableGrantsBefore = recorder.timeAdvanceGrants.size();
         subject.sendInteraction(availableRequestClass, availableParameters, new byte[0]);
         await(subject, callbackModel,
            () -> recorder.timeAdvanceGrants.size() >= availableGrantsBefore + 1,
            "Time Advance Grant callback after HLAnextMessageRequestAvailable");
         JavaTckSupport.check(recorder.timeAdvanceGrants.size() == availableGrantsBefore + 1
               && availableRequestedTime.equals(
                  recorder.timeAdvanceGrants.get(availableGrantsBefore)),
            "HLAnextMessageRequestAvailable did not produce exactly one grant at its requested time");
      } finally {
         if (availablePublished && availableRequestClass != null) {
            try { subject.unpublishInteractionClass(availableRequestClass); }
            catch (Exception ignored) { }
         }
         if (published && requestClass != null) {
            try { subject.unpublishInteractionClass(requestClass); }
            catch (Exception ignored) { }
         }
         if (joined) {
            try { subject.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) {
            try { subject.disconnect(); }
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
      for (int index = 0; index < steps; ++index) {
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
      private final List<LogicalTime<?, ?>> timeAdvanceGrants = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeAdvanceGrant".equals(name) && arguments.length == 1) {
                  timeAdvanceGrants.add((LogicalTime<?, ?>) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Table 7 Next Message Request recorder";
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
