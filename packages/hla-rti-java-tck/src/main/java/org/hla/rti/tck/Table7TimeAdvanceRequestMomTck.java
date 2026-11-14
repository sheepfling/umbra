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

/** Verifies the Table 7 Time Advance Request MOM effects through their grants. */
final class Table7TimeAdvanceRequestMomTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final String REQUEST =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAtimeAdvanceRequest";
   private static final String REQUEST_AVAILABLE =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAtimeAdvanceRequestAvailable";

   private Table7TimeAdvanceRequestMomTck() {
   }

   static void run(RtiFactory factory, String fom, String mim,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(fom != null && !fom.isEmpty()
            && mim != null && !mim.isEmpty()
            && timeImplementation != null && !timeImplementation.isEmpty(),
         "Table 7 Time Advance Request testing requires adapter FOM, MIM, and time configuration");

      RTIambassador subject = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-table7-time-advance-request-" + suffix;
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      boolean requestPublished = false;
      boolean availablePublished = false;
      InteractionClassHandle requestClass = null;
      InteractionClassHandle availableRequestClass = null;
      try {
         subject.connect(recorder.proxy(), callbackModel);
         connected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         subject.joinFederationExecution("java-tck-table7-time-advance-request-" + suffix,
            federation);
         joined = true;

         requestClass = subject.getInteractionClassHandle(REQUEST);
         availableRequestClass = subject.getInteractionClassHandle(REQUEST_AVAILABLE);
         ParameterHandle timeStampParameter =
            subject.getParameterHandle(requestClass, "HLAtimeStamp");
         ParameterHandle availableTimeStampParameter =
            subject.getParameterHandle(availableRequestClass, "HLAtimeStamp");
         JavaTckSupport.check(requestClass != null && timeStampParameter != null
               && availableRequestClass != null && availableTimeStampParameter != null,
            "standard MIM omitted a Time Advance Request or its HLAtimeStamp parameter");
         subject.publishInteractionClass(requestClass);
         requestPublished = true;
         subject.publishInteractionClass(availableRequestClass);
         availablePublished = true;

         LogicalTimeFactory<?, ?> timeFactory = subject.getTimeFactory();
         JavaTckSupport.check(timeFactory != null,
            "Time Advance Request returned no logical-time factory");
         LogicalTime<?, ?> initialTime = timeFactory.makeInitial();
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         JavaTckSupport.check(initialTime != null && epsilon != null,
            "Time Advance Request could not allocate the adapter time context");

         LogicalTime<?, ?> requestedTime = offset(initialTime, epsilon, 10);
         sendAndCheck(subject, callbackModel, recorder, requestClass, timeStampParameter,
            requestedTime, "HLAtimeAdvanceRequest", 0);

         LogicalTime<?, ?> availableRequestedTime = offset(initialTime, epsilon, 12);
         sendAndCheck(subject, callbackModel, recorder, availableRequestClass,
            availableTimeStampParameter, availableRequestedTime,
            "HLAtimeAdvanceRequestAvailable", 1);
      } finally {
         if (availablePublished && availableRequestClass != null) {
            try { subject.unpublishInteractionClass(availableRequestClass); }
            catch (Exception ignored) { }
         }
         if (requestPublished && requestClass != null) {
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

   private static void sendAndCheck(RTIambassador subject, CallbackModel callbackModel,
         Recorder recorder, InteractionClassHandle requestClass,
         ParameterHandle timeStampParameter, LogicalTime<?, ?> requestedTime,
         String description, int expectedGrantIndex) throws Exception {
      byte[] encodedTimeStamp = encode(requestedTime);
      ParameterHandleValueMap parameters =
         subject.getParameterHandleValueMapFactory().create(1);
      parameters.put(timeStampParameter, encodedTimeStamp);
      int grantsBefore = recorder.timeAdvanceGrants.size();
      JavaTckSupport.check(grantsBefore == expectedGrantIndex,
         description + " started with an unexpected grant count");
      subject.sendInteraction(requestClass, parameters, new byte[0]);
      await(subject, callbackModel,
         () -> recorder.timeAdvanceGrants.size() >= grantsBefore + 1,
         "Time Advance Grant callback after " + description);
      JavaTckSupport.check(recorder.timeAdvanceGrants.size() == grantsBefore + 1
            && requestedTime.equals(recorder.timeAdvanceGrants.get(grantsBefore)),
         description + " did not produce exactly one grant at its requested time");
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
                  if ("toString".equals(name)) return "Table 7 Time Advance Request recorder";
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
