package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;

/** Checks that disabled callbacks gate both evoke services until re-enabled. */
final class DisableCallbacksGatesEvokeServicesTck {
   private DisableCallbacksGatesEvokeServicesTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "disabled-callback evoke-service testing requires HLA_EVOKED");

      RTIambassador ambassador = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      String federation = "java-tck-disable-callbacks-evoke-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         ambassador.connect(recorder.proxy(), callbackModel);
         connected = true;
         ambassador.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         ambassador.joinFederationExecution(
            "disabled-evoke-federate", "disabled-evoke-type", federation);
         joined = true;

         ambassador.disableCallbacks();
         ambassador.listFederationExecutions();
         boolean pendingWhileDisabled = awaitPendingWithMultipleCallbacks(ambassador);
         JavaTckSupport.check(pendingWhileDisabled,
            "the federation-list callback did not become pending while callbacks were disabled");

         ambassador.evokeCallback(0.0);
         boolean pendingAfterSingleEvoke = ambassador.evokeMultipleCallbacks(0.0, 0.0);
         JavaTckSupport.check(recorder.reportCount.get() == 0 && pendingAfterSingleEvoke,
            "evokeCallback processed or discarded a pending callback while callbacks were disabled");

         ambassador.evokeMultipleCallbacks(0.0, 0.0);
         JavaTckSupport.check(recorder.reportCount.get() == 0,
            "evokeMultipleCallbacks processed a callback while callbacks were disabled");

         ambassador.enableCallbacks();
         JavaTckSupport.check(awaitReportWithSingleCallback(ambassador, recorder, 1),
            "evokeCallback did not process a pending callback after callbacks were enabled");

         ambassador.listFederationExecutions();
         JavaTckSupport.check(awaitReportWithMultipleCallbacks(ambassador, recorder, 2),
            "evokeMultipleCallbacks did not process a pending callback after callbacks were enabled");

         verifyConcurrentDisableWaitsForCallback(ambassador, recorder);
      } finally {
         if (joined) {
            try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { ambassador.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) {
            try { ambassador.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static boolean awaitPendingWithMultipleCallbacks(RTIambassador ambassador)
         throws Exception {
      long deadline = System.nanoTime() + TimeUnit.SECONDS.toNanos(1);
      while (System.nanoTime() < deadline) {
         if (ambassador.evokeMultipleCallbacks(0.0, 0.0)) return true;
         Thread.sleep(1);
      }
      return false;
   }

   private static boolean awaitReportWithSingleCallback(RTIambassador ambassador,
         Recorder recorder, int expectedCount) throws Exception {
      long deadline = System.nanoTime() + TimeUnit.SECONDS.toNanos(1);
      while (recorder.reportCount.get() < expectedCount && System.nanoTime() < deadline) {
         ambassador.evokeCallback(0.0);
         Thread.sleep(1);
      }
      return recorder.reportCount.get() == expectedCount;
   }

   private static boolean awaitReportWithMultipleCallbacks(RTIambassador ambassador,
         Recorder recorder, int expectedCount) throws Exception {
      long deadline = System.nanoTime() + TimeUnit.SECONDS.toNanos(1);
      while (recorder.reportCount.get() < expectedCount && System.nanoTime() < deadline) {
         ambassador.evokeMultipleCallbacks(0.0, 0.0);
         Thread.sleep(1);
      }
      return recorder.reportCount.get() == expectedCount;
   }

   private static void verifyConcurrentDisableWaitsForCallback(
         RTIambassador ambassador, Recorder recorder) throws Exception {
      CountDownLatch callbackEntered = new CountDownLatch(1);
      CountDownLatch releaseCallback = new CountDownLatch(1);
      recorder.beginConcurrentPhase(callbackEntered, releaseCallback);
      ambassador.listFederationExecutions();
      ambassador.listFederationExecutions();

      AtomicReference<Thread> evokeThread = new AtomicReference<>();
      AtomicBoolean pendingAfterConcurrentDisable = new AtomicBoolean();
      AtomicReference<Throwable> evokeFailure = new AtomicReference<>();
      Thread evoker = new Thread(() -> {
         evokeThread.set(Thread.currentThread());
         try {
            pendingAfterConcurrentDisable.set(ambassador.evokeMultipleCallbacks(0.0, 1.0));
         } catch (Throwable failure) {
            evokeFailure.set(failure);
         }
      }, "java-tck-disable-callbacks-evoker");
      evoker.setDaemon(true);

      CountDownLatch disableStarted = new CountDownLatch(1);
      CountDownLatch disableReturned = new CountDownLatch(1);
      AtomicReference<Throwable> disableFailure = new AtomicReference<>();
      Thread disabler = new Thread(() -> {
         disableStarted.countDown();
         try {
            ambassador.disableCallbacks();
         } catch (Throwable failure) {
            disableFailure.set(failure);
         } finally {
            disableReturned.countDown();
         }
      }, "java-tck-disable-callbacks-disabler");
      disabler.setDaemon(true);

      boolean callbackEnteredInTime = false;
      boolean disableStartedInTime = false;
      boolean disableReturnedWhileCallbackWasHeld = false;
      evoker.start();
      try {
         callbackEnteredInTime = callbackEntered.await(2, TimeUnit.SECONDS);
         if (callbackEnteredInTime) {
            disabler.start();
            disableStartedInTime = disableStarted.await(2, TimeUnit.SECONDS);
            if (disableStartedInTime) {
               disableReturnedWhileCallbackWasHeld =
                  disableReturned.await(500, TimeUnit.MILLISECONDS);
            }
         }
      } finally {
         releaseCallback.countDown();
         evoker.join(TimeUnit.SECONDS.toMillis(3));
         disabler.join(TimeUnit.SECONDS.toMillis(3));
         recorder.endConcurrentPhase();
      }

      JavaTckSupport.check(callbackEnteredInTime,
         "Evoke Multiple Callbacks did not enter the queued callback on its caller thread");
      JavaTckSupport.check(disableStartedInTime,
         "the concurrent Disable Callbacks call did not start");
      JavaTckSupport.check(!disableReturnedWhileCallbackWasHeld,
         "Disable Callbacks returned while the in-flight callback was held open");
      JavaTckSupport.check(!evoker.isAlive() && !disabler.isAlive(),
         "the concurrent evoke/disable threads did not finish after releasing the callback");
      JavaTckSupport.check(evokeFailure.get() == null,
         "the concurrent Evoke Multiple Callbacks call failed: " + evokeFailure.get());
      JavaTckSupport.check(disableFailure.get() == null,
         "the concurrent Disable Callbacks call failed: " + disableFailure.get());
      JavaTckSupport.check(recorder.firstConcurrentReportThread.get() == evokeThread.get(),
         "the HLA_EVOKED callback did not use the Evoke Multiple Callbacks thread context");
      JavaTckSupport.check(pendingAfterConcurrentDisable.get() && recorder.reportCount.get() == 1,
         "concurrent Disable Callbacks did not stop the next callback and leave it pending");

      ambassador.enableCallbacks();
      boolean pendingAfterFinalDrain = ambassador.evokeMultipleCallbacks(0.0, 0.0);
      JavaTckSupport.check(!pendingAfterFinalDrain && recorder.reportCount.get() == 2,
         "the callback retained by concurrent Disable Callbacks was not delivered after re-enabling");
   }

   private static final class Recorder {
      private final AtomicInteger reportCount = new AtomicInteger();
      private final AtomicReference<Thread> firstConcurrentReportThread = new AtomicReference<>();
      private volatile CountDownLatch concurrentCallbackEntered;
      private volatile CountDownLatch releaseConcurrentCallback;

      private void beginConcurrentPhase(CountDownLatch callbackEntered, CountDownLatch releaseCallback) {
         reportCount.set(0);
         firstConcurrentReportThread.set(null);
         concurrentCallbackEntered = callbackEntered;
         releaseConcurrentCallback = releaseCallback;
      }

      private void endConcurrentPhase() {
         CountDownLatch release = releaseConcurrentCallback;
         if (release != null) release.countDown();
         concurrentCallbackEntered = null;
         releaseConcurrentCallback = null;
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("reportFederationExecutions".equals(name)) {
                  int callbackCount = reportCount.incrementAndGet();
                  CountDownLatch entered = concurrentCallbackEntered;
                  CountDownLatch release = releaseConcurrentCallback;
                  if (callbackCount == 1 && entered != null && release != null) {
                     firstConcurrentReportThread.set(Thread.currentThread());
                     entered.countDown();
                     if (!release.await(3, TimeUnit.SECONDS)) {
                        throw new IllegalStateException(
                           "the concurrent callback was not released within its bound");
                     }
                  }
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK evoke-service gating recorder";
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
