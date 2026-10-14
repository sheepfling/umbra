package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

/** P2 callback-control checks for a restore request referencing no saved federation. */
final class CallbackControlsRestoreRequestFailureTck {
   private CallbackControlsRestoreRequestFailureTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, CallbackModel callbackModel) throws Exception {
      RTIambassador requester = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder requesterRecorder = new Recorder();
      Recorder observerRecorder = new Recorder();
      String federation = "java-tck-callback-restore-request-failure-" + UUID.randomUUID();
      String missingLabel = "java-tck-callback-missing-restore";
      boolean requesterConnected = false;
      boolean observerConnected = false;
      boolean created = false;
      boolean requesterJoined = false;
      boolean observerJoined = false;
      try {
         requester.connect(requesterRecorder.proxy(), callbackModel);
         requesterConnected = true;
         observer.connect(observerRecorder.proxy(), callbackModel);
         observerConnected = true;
         requester.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         requester.joinFederationExecution("java-tck-restore-requester", federation);
         requesterJoined = true;
         observer.joinFederationExecution("java-tck-restore-observer", federation);
         observerJoined = true;

         int failuresBefore = requesterRecorder.restoreFailures.size();
         int requesterBegunBefore = requesterRecorder.restoreBegunCount;
         int observerBegunBefore = observerRecorder.restoreBegunCount;
         int requesterInitiationsBefore = requesterRecorder.restoreInitiationCount;
         int observerInitiationsBefore = observerRecorder.restoreInitiationCount;
         requester.disableCallbacks();
         requester.requestFederationRestore(missingLabel);
         service(requester, callbackModel, objectClassName);
         JavaTckSupport.check(requesterRecorder.restoreFailures.size() == failuresBefore,
            "disabled callbacks exposed a restore-request failure");
         JavaTckSupport.check(observerRecorder.restoreFailures.isEmpty(),
            "restore-request failure was delivered to a non-requesting federate");

         requester.enableCallbacks();
         await(callbackModel,
            () -> requesterRecorder.restoreFailures.size() > failuresBefore,
            requester, objectClassName);
         JavaTckSupport.check(missingLabel.equals(
               requesterRecorder.restoreFailures.get(requesterRecorder.restoreFailures.size() - 1)),
            "restore-request failure returned the wrong save label");
         JavaTckSupport.check(requesterRecorder.restoreSuccesses.isEmpty()
               && observerRecorder.restoreSuccesses.isEmpty()
               && requesterRecorder.restoreBegunCount == requesterBegunBefore
               && observerRecorder.restoreBegunCount == observerBegunBefore
               && requesterRecorder.restoreInitiationCount == requesterInitiationsBefore
               && observerRecorder.restoreInitiationCount == observerInitiationsBefore,
            "failed restore request unexpectedly began a federation restore");
      } finally {
         if (requesterJoined) resign(requester);
         if (observerJoined) resign(observer);
         if (created) {
            try { requester.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (observerConnected) disconnect(observer);
         if (requesterConnected) disconnect(requester);
      }
   }

   private static void service(RTIambassador ambassador, CallbackModel model,
         String objectClassName) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) ambassador.evokeMultipleCallbacks(0.0, 1.0);
      else ambassador.getObjectClassHandle(objectClassName);
   }

   private static void await(CallbackModel model, Condition condition,
         RTIambassador ambassador, String objectClassName) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline) {
         service(ambassador, model, objectClassName);
         if (condition.isMet()) return;
         Thread.yield();
      }
      throw new AssertionError("restore-request failure callback did not arrive before deadline");
   }

   @FunctionalInterface
   private interface Condition { boolean isMet() throws Exception; }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static final class Recorder {
      private final List<String> restoreFailures = new ArrayList<>();
      private final List<String> restoreSuccesses = new ArrayList<>();
      private int restoreBegunCount;
      private int restoreInitiationCount;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("requestFederationRestoreFailed".equals(name)) {
                  restoreFailures.add((String) arguments[0]);
               }
               if ("requestFederationRestoreSucceeded".equals(name)) {
                  restoreSuccesses.add((String) arguments[0]);
               }
               if ("federationRestoreBegun".equals(name)) restoreBegunCount++;
               if ("initiateFederateRestore".equals(name)) restoreInitiationCount++;
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK restore-request failure recorder";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == arguments[0];
               }
               return null;
            });
      }
   }
}
