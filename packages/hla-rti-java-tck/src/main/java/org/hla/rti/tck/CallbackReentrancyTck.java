package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.exceptions.CallNotAllowedFromWithinCallback;
import java.lang.reflect.Proxy;
import java.util.UUID;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;

/** Checks the standard callback-servicing re-entry exception boundary. */
final class CallbackReentrancyTck {
   private CallbackReentrancyTck() { }

   static void run(RtiFactory factory, CallbackModel callbackModel, String fom,
         String timeImplementation, String objectClassName) throws Exception {
      RTIambassador ambassador = factory.getRtiAmbassador();
      Recorder recorder = new Recorder(ambassador);
      String federation = "java-tck-callback-reentrancy-" + UUID.randomUUID();
      boolean connected = false;
      boolean federationCreated = false;
      boolean joined = false;
      try {
         ambassador.connect(recorder.proxy(), callbackModel);
         connected = true;
         ambassador.listFederationExecutions();
         awaitReport(ambassador, recorder, 1, callbackModel, "unjoined");

         JavaTckSupport.check(recorder.reportCount.get() == 1,
            "callback re-entrancy test did not receive exactly one federation-list callback");
         JavaTckSupport.check(recorder.evokeRejected.get()
               && recorder.multipleRejected.get(),
            "callback re-entrancy test admitted callback-service re-entry");

         JavaTckSupport.check(objectClassName != null && !objectClassName.isEmpty(),
            "joined callback re-entrancy requires an adapter-supplied object class");
         ambassador.createFederationExecution(federation, fom, timeImplementation);
         federationCreated = true;
         ambassador.joinFederationExecution(
            "java-tck-callback-reentrancy-joined-federate", federation);
         joined = true;
         recorder.beginJoinedCallback(objectClassName);
         ambassador.listFederationExecutions();
         awaitReport(ambassador, recorder, 2, callbackModel, "joined");

         JavaTckSupport.check(recorder.reportCount.get() == 2,
            "joined callback re-entrancy did not receive its federation-list callback");
         JavaTckSupport.check(recorder.joinedObjectClassLookupSucceeded.get()
               && recorder.joinedObjectClassLookupFailure.get() == null,
            "Get Object Class Handle failed from the joined federate's callback: "
               + recorder.joinedObjectClassLookupFailure.get());
         JavaTckSupport.check(recorder.joinedEvokeRejected.get()
               && recorder.joinedMultipleRejected.get(),
            "callback-only evoke services were not rejected from the joined callback");

         ambassador.resignFederationExecution(ResignAction.NO_ACTION);
         joined = false;
         ambassador.destroyFederationExecution(federation);
         federationCreated = false;
      } finally {
         if (joined) {
            try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (federationCreated) {
            try { ambassador.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) {
            try { ambassador.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void awaitReport(RTIambassador ambassador, Recorder recorder,
         int reportNumber, CallbackModel callbackModel, String phase) throws Exception {
      CountDownLatch callback = reportNumber == 1
         ? recorder.firstReportDelivered : recorder.joinedReportDelivered;
      long deadline = System.nanoTime() + TimeUnit.SECONDS.toNanos(10);
      if (callbackModel == CallbackModel.HLA_EVOKED) {
         while (callback.getCount() != 0 && System.nanoTime() < deadline) {
            ambassador.evokeMultipleCallbacks(0.0, 0.01);
         }
      }
      boolean delivered = callback.getCount() == 0;
      long remaining = deadline - System.nanoTime();
      if (!delivered && remaining > 0) {
         delivered = callback.await(remaining, TimeUnit.NANOSECONDS);
      }
      JavaTckSupport.check(delivered,
         "callback re-entrancy test timed out waiting for the " + phase
            + " federation-list callback");
   }

   private static final class Recorder {
      private final RTIambassador ambassador;
      private final AtomicInteger reportCount = new AtomicInteger();
      private final AtomicBoolean evokeRejected = new AtomicBoolean();
      private final AtomicBoolean multipleRejected = new AtomicBoolean();
      private final AtomicBoolean joinedObjectClassLookupSucceeded = new AtomicBoolean();
      private final AtomicReference<Exception> joinedObjectClassLookupFailure =
         new AtomicReference<>();
      private final AtomicBoolean joinedEvokeRejected = new AtomicBoolean();
      private final AtomicBoolean joinedMultipleRejected = new AtomicBoolean();
      private final CountDownLatch firstReportDelivered = new CountDownLatch(1);
      private final CountDownLatch joinedReportDelivered = new CountDownLatch(1);
      private volatile boolean joinedCallbackPhase;
      private volatile String joinedObjectClassName;

      private Recorder(RTIambassador ambassador) {
         this.ambassador = ambassador;
      }

      private void beginJoinedCallback(String objectClassName) {
         joinedObjectClassName = objectClassName;
         joinedCallbackPhase = true;
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("reportFederationExecutions".equals(name)) {
                  int currentReport = reportCount.incrementAndGet();
                  boolean joinedCallback = joinedCallbackPhase;
                  try {
                     if (joinedCallback) {
                        try {
                           ambassador.getObjectClassHandle(joinedObjectClassName);
                           joinedObjectClassLookupSucceeded.set(true);
                        } catch (Exception failure) {
                           joinedObjectClassLookupFailure.compareAndSet(null, failure);
                        }
                     }
                     try {
                        ambassador.evokeCallback(0.0);
                     } catch (CallNotAllowedFromWithinCallback expected) {
                        evokeRejected.set(true);
                        if (joinedCallback) joinedEvokeRejected.set(true);
                     }
                     try {
                        ambassador.evokeMultipleCallbacks(0.0, 0.0);
                     } catch (CallNotAllowedFromWithinCallback expected) {
                        multipleRejected.set(true);
                        if (joinedCallback) joinedMultipleRejected.set(true);
                     }
                  } finally {
                     if (currentReport == 1) firstReportDelivered.countDown();
                     if (currentReport == 2) joinedReportDelivered.countDown();
                  }
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK callback-reentrancy recorder";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
