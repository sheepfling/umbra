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

/** Checks concurrent joined services on two distinct standard RTIambassadors. */
final class FederateInitiatedConcurrencyDifferentAmbassadorsTck {
   private static final long CALLBACK_ENTRY_TIMEOUT_SECONDS = 2L;
   private static final long CALLBACK_RELEASE_TIMEOUT_SECONDS = 5L;
   private static final long SERVICE_OVERLAP_MILLIS = 100L;

   private FederateInitiatedConcurrencyDifferentAmbassadorsTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      JavaTckSupport.check(owner != member,
         "the factory returned one RTIambassador instance for two requests");

      String federation = "java-tck-different-ambassadors-" + UUID.randomUUID();
      String ownerName = "java-tck-different-ambassadors-owner-" + UUID.randomUUID();
      String memberName = "java-tck-different-ambassadors-member-" + UUID.randomUUID();
      AtomicInteger ownerReportCount = new AtomicInteger();
      AtomicInteger memberReportCount = new AtomicInteger();
      CountDownLatch ownerCallbackEntered = new CountDownLatch(1);
      CountDownLatch releaseOwnerCallback = new CountDownLatch(1);
      AtomicReference<Throwable> ownerEvokeFailure = new AtomicReference<>();
      AtomicReference<Throwable> releaserFailure = new AtomicReference<>();
      AtomicReference<Throwable> memberListFailure = new AtomicReference<>();
      AtomicReference<Throwable> memberEvokeFailure = new AtomicReference<>();
      boolean ownerConnected = false;
      boolean memberConnected = false;
      boolean federationCreated = false;
      boolean ownerJoined = false;
      boolean memberJoined = false;
      boolean ownerCallbackWasEntered = false;
      boolean ownerEvokerFinished = true;
      boolean callbackReleaserFinished = true;
      Thread ownerEvoker = null;
      Thread callbackReleaser = null;

      try {
         owner.connect(reportCallbacks(
            ownerReportCount, ownerCallbackEntered, releaseOwnerCallback, true),
            CallbackModel.HLA_EVOKED);
         ownerConnected = true;
         member.connect(reportCallbacks(
            memberReportCount, new CountDownLatch(0), new CountDownLatch(0), false),
            CallbackModel.HLA_EVOKED);
         memberConnected = true;

         owner.createFederationExecution(federation, fom, timeImplementation);
         federationCreated = true;
         owner.joinFederationExecution(ownerName, federation);
         ownerJoined = true;
         member.joinFederationExecution(memberName, federation);
         memberJoined = true;

         owner.listFederationExecutions();
         ownerEvoker = new Thread(() -> {
            try {
               owner.evokeCallback(0.25);
            } catch (Throwable failure) {
               ownerEvokeFailure.set(failure);
            }
         }, "java-tck-different-ambassadors-owner-evoker");
         ownerEvoker.setDaemon(true);
         ownerEvoker.start();

         ownerCallbackWasEntered = ownerCallbackEntered.await(
            CALLBACK_ENTRY_TIMEOUT_SECONDS, TimeUnit.SECONDS);
         if (ownerCallbackWasEntered) {
            CountDownLatch memberServiceStarting = new CountDownLatch(1);
            callbackReleaser = new Thread(() -> {
               try {
                  if (memberServiceStarting.await(
                        CALLBACK_ENTRY_TIMEOUT_SECONDS, TimeUnit.SECONDS)) {
                     Thread.sleep(SERVICE_OVERLAP_MILLIS);
                  }
               } catch (Throwable failure) {
                  releaserFailure.set(failure);
               } finally {
                  releaseOwnerCallback.countDown();
               }
            }, "java-tck-different-ambassadors-callback-releaser");
            callbackReleaser.setDaemon(true);
            callbackReleaser.start();

            memberServiceStarting.countDown();
            try {
               member.listFederationExecutions();
            } catch (Throwable failure) {
               memberListFailure.set(failure);
            }
            callbackReleaserFinished = joinWithin(callbackReleaser, 2_000L);
            if (!callbackReleaserFinished) callbackReleaser.interrupt();

            if (memberListFailure.get() == null) {
               try {
                  member.evokeCallback(0.25);
               } catch (Throwable failure) {
                  memberEvokeFailure.set(failure);
               }
            }
         }
      } finally {
         releaseOwnerCallback.countDown();
         ownerEvokerFinished = joinWithin(ownerEvoker, 3_000L);
         if (!ownerEvokerFinished && ownerEvoker != null) ownerEvoker.interrupt();
         if (callbackReleaser != null && callbackReleaser.isAlive()) {
            callbackReleaserFinished = joinWithin(callbackReleaser, 1_000L);
            if (!callbackReleaserFinished) callbackReleaser.interrupt();
         }

         if (memberJoined) {
            try { member.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Throwable ignored) { }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Throwable ignored) { }
         }
         if (federationCreated) {
            try { owner.destroyFederationExecution(federation); }
            catch (Throwable ignored) { }
         }
         if (memberConnected) {
            try { member.disconnect(); }
            catch (Throwable ignored) { }
         }
         if (ownerConnected) {
            try { owner.disconnect(); }
            catch (Throwable ignored) { }
         }
      }

      rethrow(ownerEvokeFailure.get(), "owner Evoke Callback");
      rethrow(releaserFailure.get(), "concurrent callback releaser");
      rethrow(memberListFailure.get(), "member List Federation Executions");
      rethrow(memberEvokeFailure.get(), "member Evoke Callback");
      JavaTckSupport.check(ownerEvokerFinished,
         "the owner Evoke Callback call did not finish after its callback was released");
      JavaTckSupport.check(callbackReleaserFinished,
         "the callback releaser did not finish");
      JavaTckSupport.check(ownerCallbackWasEntered && ownerReportCount.get() == 1,
         "the owner did not complete its deliberately held federation-list callback");
      JavaTckSupport.check(memberReportCount.get() == 1,
         "the second RTIambassador did not process its concurrent joined service and callback");
   }

   private static FederateAmbassador reportCallbacks(AtomicInteger reportCount,
         CountDownLatch callbackEntered, CountDownLatch callbackRelease,
         boolean holdFirstCallback) {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            String name = method.getName();
            if ("reportFederationExecutions".equals(name)) {
               JavaTckSupport.check(arguments != null && arguments.length == 1,
                  "reportFederationExecutions used an unexpected Java API overload");
               int ordinal = reportCount.incrementAndGet();
               if (holdFirstCallback && ordinal == 1) {
                  callbackEntered.countDown();
                  try {
                     JavaTckSupport.check(callbackRelease.await(
                           CALLBACK_RELEASE_TIMEOUT_SECONDS, TimeUnit.SECONDS),
                        "the held federation-list callback was not released");
                  } catch (InterruptedException failure) {
                     Thread.currentThread().interrupt();
                     throw failure;
                  }
               }
            }
            return objectMethod(proxy, name, arguments);
         });
   }

   private static Object objectMethod(Object proxy, String name, Object[] arguments) {
      if ("toString".equals(name)) {
         return "Java TCK different-ambassador federation-list callbacks";
      }
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
