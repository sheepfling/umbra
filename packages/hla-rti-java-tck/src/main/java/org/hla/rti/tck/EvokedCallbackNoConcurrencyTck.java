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
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;

/** Checks that concurrent evoked services do not overlap one federate callback. */
final class EvokedCallbackNoConcurrencyTck {
   private static final long CALLBACK_ENTRY_TIMEOUT_SECONDS = 2L;
   private static final long CALLBACK_RELEASE_TIMEOUT_SECONDS = 5L;
   private static final long SECOND_CALLBACK_OBSERVATION_MILLIS = 500L;

   private EvokedCallbackNoConcurrencyTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation)
         throws Exception {
      RTIambassador ambassador = factory.getRtiAmbassador();
      String federation = "java-tck-evoked-callback-serialization-" + UUID.randomUUID();
      String federateName = "java-tck-evoked-callback-serialization-" + UUID.randomUUID();
      AtomicInteger callbackCount = new AtomicInteger();
      AtomicInteger activeCallbacks = new AtomicInteger();
      AtomicInteger maximumActiveCallbacks = new AtomicInteger();
      CountDownLatch firstCallbackEntered = new CountDownLatch(1);
      CountDownLatch secondCallbackEntered = new CountDownLatch(1);
      CountDownLatch releaseFirstCallback = new CountDownLatch(1);
      AtomicReference<Throwable> firstEvokeFailure = new AtomicReference<>();
      AtomicReference<Throwable> secondEvokeFailure = new AtomicReference<>();
      boolean connected = false;
      boolean federationCreated = false;
      boolean joined = false;
      boolean firstCallbackWasEntered = false;
      boolean secondEvokeStartedBeforeRelease = false;
      boolean secondCallbackEnteredWhileFirstWasOpen = false;
      boolean firstEvokeFinished = true;
      boolean secondEvokeFinished = true;
      Thread firstEvoke = null;
      Thread secondEvoke = null;

      try {
         ambassador.connect(callbacks(callbackCount, activeCallbacks,
            maximumActiveCallbacks, firstCallbackEntered,
            secondCallbackEntered, releaseFirstCallback), CallbackModel.HLA_EVOKED);
         connected = true;
         ambassador.createFederationExecution(federation, fom, timeImplementation);
         federationCreated = true;
         ambassador.joinFederationExecution(federateName, federation);
         joined = true;

         ambassador.listFederationExecutions();
         ambassador.listFederationExecutions();

         firstEvoke = new Thread(() -> {
            try {
               ambassador.evokeCallback(0.25);
            } catch (Throwable failure) {
               firstEvokeFailure.set(failure);
            }
         }, "java-tck-evoked-callback-first-evoker");
         firstEvoke.setDaemon(true);
         firstEvoke.start();
         firstCallbackWasEntered = firstCallbackEntered.await(
            CALLBACK_ENTRY_TIMEOUT_SECONDS, TimeUnit.SECONDS);

         if (firstCallbackWasEntered) {
            CountDownLatch secondEvokeStarted = new CountDownLatch(1);
            secondEvoke = new Thread(() -> {
               secondEvokeStarted.countDown();
               try {
                  ambassador.evokeCallback(0.25);
               } catch (Throwable failure) {
                  secondEvokeFailure.set(failure);
               }
            }, "java-tck-evoked-callback-second-evoker");
            secondEvoke.setDaemon(true);
            secondEvoke.start();
            secondEvokeStartedBeforeRelease = secondEvokeStarted.await(
               CALLBACK_ENTRY_TIMEOUT_SECONDS, TimeUnit.SECONDS);
            if (secondEvokeStartedBeforeRelease) {
               secondCallbackEnteredWhileFirstWasOpen = secondCallbackEntered.await(
                  SECOND_CALLBACK_OBSERVATION_MILLIS, TimeUnit.MILLISECONDS);
            }
         }
      } finally {
         releaseFirstCallback.countDown();
         firstEvokeFinished = joinWithin(firstEvoke, 3_000L);
         secondEvokeFinished = joinWithin(secondEvoke, 3_000L);
         if (!firstEvokeFinished && firstEvoke != null) firstEvoke.interrupt();
         if (!secondEvokeFinished && secondEvoke != null) secondEvoke.interrupt();
         if (joined) {
            try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Throwable ignored) { }
         }
         if (federationCreated) {
            try { ambassador.destroyFederationExecution(federation); }
            catch (Throwable ignored) { }
         }
         if (connected) {
            try { ambassador.disconnect(); }
            catch (Throwable ignored) { }
         }
      }

      rethrow(firstEvokeFailure.get(), "first concurrent Evoke Callback");
      rethrow(secondEvokeFailure.get(), "second concurrent Evoke Callback");
      JavaTckSupport.check(firstEvokeFinished,
         "the first concurrent Evoke Callback call did not finish after release");
      JavaTckSupport.check(secondEvokeFinished,
         "the second concurrent Evoke Callback call did not finish after release");
      JavaTckSupport.check(firstCallbackWasEntered,
         "the first concurrent Evoke Callback did not enter the queued callback");
      JavaTckSupport.check(secondEvokeStartedBeforeRelease,
         "the second Evoke Callback call did not start while the first callback was open");
      JavaTckSupport.check(!secondCallbackEnteredWhileFirstWasOpen,
         "the RTI entered a second FederateAmbassador callback while the first was in progress");
      JavaTckSupport.check(callbackCount.get() == 2,
         "the two concurrent Evoke Callback calls did not deliver both queued callbacks");
      JavaTckSupport.check(maximumActiveCallbacks.get() == 1
            && activeCallbacks.get() == 0,
         "RTI-initiated calls overlapped on the same FederateAmbassador instance");
   }

   private static FederateAmbassador callbacks(AtomicInteger callbackCount,
         AtomicInteger activeCallbacks, AtomicInteger maximumActiveCallbacks,
         CountDownLatch firstCallbackEntered, CountDownLatch secondCallbackEntered,
         CountDownLatch releaseFirstCallback) {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            String name = method.getName();
            if ("reportFederationExecutions".equals(name)) {
               JavaTckSupport.check(arguments != null && arguments.length == 1,
                  "reportFederationExecutions used an unexpected Java API overload");
               int active = activeCallbacks.incrementAndGet();
               recordMaximum(maximumActiveCallbacks, active);
               try {
                  int ordinal = callbackCount.incrementAndGet();
                  if (ordinal == 1) {
                     firstCallbackEntered.countDown();
                     try {
                        JavaTckSupport.check(releaseFirstCallback.await(
                              CALLBACK_RELEASE_TIMEOUT_SECONDS, TimeUnit.SECONDS),
                           "the held federation-list callback was not released");
                     } catch (InterruptedException failure) {
                        Thread.currentThread().interrupt();
                        throw failure;
                     }
                  } else if (ordinal == 2) {
                     secondCallbackEntered.countDown();
                  }
               } finally {
                  activeCallbacks.decrementAndGet();
               }
            }
            return objectMethod(proxy, name, arguments);
         });
   }

   private static void recordMaximum(AtomicInteger maximum, int value) {
      int observed = maximum.get();
      while (observed < value && !maximum.compareAndSet(observed, value)) {
         observed = maximum.get();
      }
   }

   private static Object objectMethod(Object proxy, String name, Object[] arguments) {
      if ("toString".equals(name)) return "Java TCK evoked callback serialization recorder";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
      return null;
   }

   private static boolean joinWithin(Thread thread, long timeoutMillis) {
      if (thread == null) return true;
      try {
         thread.join(timeoutMillis);
         return !thread.isAlive();
      } catch (InterruptedException failure) {
         Thread.currentThread().interrupt();
         return false;
      }
   }

   private static void rethrow(Throwable failure, String service) {
      if (failure != null) throw new AssertionError(service + " failed", failure);
   }
}
