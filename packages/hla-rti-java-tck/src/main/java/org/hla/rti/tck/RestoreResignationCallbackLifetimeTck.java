package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RestoreFailureReason;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** A resigning member loses queued evoked restore callbacks; the survivor gets failure. */
final class RestoreResignationCallbackLifetimeTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private RestoreResignationCallbackLifetimeTck() { }

   static void run(RtiFactory factory, String fom,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "restore resignation callback lifetime requires evoked callbacks");
      RTIambassador survivor = factory.getRtiAmbassador();
      RTIambassador departing = factory.getRtiAmbassador();
      Recorder survivorCallbacks = new Recorder();
      Recorder departingCallbacks = new Recorder();
      String federation = "java-tck-restore-resignation-" + UUID.randomUUID();
      String label = "tck-restore-resignation-baseline";
      boolean survivorConnected = false;
      boolean departingConnected = false;
      boolean created = false;
      boolean survivorJoined = false;
      boolean departingJoined = false;
      try {
         survivor.connect(survivorCallbacks.proxy(), callbackModel);
         survivorConnected = true;
         departing.connect(departingCallbacks.proxy(), callbackModel);
         departingConnected = true;
         survivor.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         survivor.joinFederationExecution("java-tck-restore-survivor", federation);
         survivorJoined = true;
         departing.joinFederationExecution("java-tck-restore-departing", federation);
         departingJoined = true;

         survivor.requestFederationSave(label);
         awaitBoth(survivor, departing,
            () -> survivorCallbacks.saveLabels.size() == 1
               && departingCallbacks.saveLabels.size() == 1,
            "both baseline save initiations");
         JavaTckSupport.check(survivorCallbacks.saveLabels.equals(List.of(label))
               && departingCallbacks.saveLabels.equals(List.of(label)),
            "baseline save did not initiate for both members");
         survivor.federateSaveBegun();
         departing.federateSaveBegun();
         survivor.federateSaveComplete();
         departing.federateSaveComplete();
         awaitBoth(survivor, departing,
            () -> survivorCallbacks.savedCount == 1
               && departingCallbacks.savedCount == 1,
            "both baseline federation-saved callbacks");
         JavaTckSupport.check(survivorCallbacks.saveFailures == 0
               && departingCallbacks.saveFailures == 0, "baseline save failed");
         while (survivor.evokeCallback(0.0)) { }
         while (departing.evokeCallback(0.0)) { }

         survivorCallbacks.order.clear();
         departingCallbacks.order.clear();
         survivor.requestFederationRestore(label);
         JavaTckSupport.check(survivorCallbacks.order.isEmpty()
               && departingCallbacks.order.isEmpty(),
            "evoked restore callbacks arrived before callback servicing");

         departing.resignFederationExecution(ResignAction.NO_ACTION);
         departingJoined = false;
         await(survivor, () -> survivorCallbacks.restoreFailures.size() == 1,
            "survivor restore failure after peer resignation");
         JavaTckSupport.check(survivorCallbacks.restoreSuccesses.equals(List.of(label))
               && survivorCallbacks.restoreBegunCount == 1
               && survivorCallbacks.restoreInitiations.equals(List.of(label))
               && survivorCallbacks.restoreFailures.equals(
                  List.of(RestoreFailureReason.FEDERATE_RESIGNED_DURING_RESTORE))
               && survivorCallbacks.order.equals(List.of(
                  "restore-request-succeeded", "restore-begun",
                  "restore-initiate", "restore-failed")),
            "survivor did not receive queued restore initiation before resignation failure");

         while (departing.evokeCallback(0.0)) { }
         JavaTckSupport.check(departingCallbacks.order.isEmpty()
               && departingCallbacks.restoreSuccesses.isEmpty()
               && departingCallbacks.restoreBegunCount == 0
               && departingCallbacks.restoreInitiations.isEmpty()
               && departingCallbacks.restoreFailures.isEmpty(),
            "resigned member received stale public restore callbacks");

         survivor.resignFederationExecution(ResignAction.NO_ACTION);
         survivorJoined = false;
         survivor.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (departingJoined) resign(departing);
         if (survivorJoined) resign(survivor);
         if (created) {
            try { survivor.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (departingConnected) disconnect(departing);
         if (survivorConnected) disconnect(survivor);
      }
   }

   private static void await(RTIambassador ambassador,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         ambassador.evokeCallback(0.0);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void awaitBoth(RTIambassador first, RTIambassador second,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         first.evokeCallback(0.0);
         second.evokeCallback(0.0);
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

   private static final class Recorder {
      private final List<String> saveLabels = new CopyOnWriteArrayList<>();
      private final List<String> order = new CopyOnWriteArrayList<>();
      private final List<String> restoreSuccesses = new CopyOnWriteArrayList<>();
      private final List<String> restoreInitiations = new CopyOnWriteArrayList<>();
      private final List<RestoreFailureReason> restoreFailures =
         new CopyOnWriteArrayList<>();
      private volatile int savedCount;
      private volatile int saveFailures;
      private volatile int restoreBegunCount;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("initiateFederateSave".equals(name)) {
                  saveLabels.add((String) arguments[0]);
                  order.add("save-initiate");
               } else if ("federationSaved".equals(name)) ++savedCount;
               else if ("federationNotSaved".equals(name)) ++saveFailures;
               else if ("requestFederationRestoreSucceeded".equals(name)) {
                  restoreSuccesses.add((String) arguments[0]);
                  order.add("restore-request-succeeded");
               } else if ("federationRestoreBegun".equals(name)) {
                  ++restoreBegunCount;
                  order.add("restore-begun");
               } else if ("initiateFederateRestore".equals(name)) {
                  restoreInitiations.add((String) arguments[0]);
                  order.add("restore-initiate");
               } else if ("federationNotRestored".equals(name)) {
                  restoreFailures.add((RestoreFailureReason) arguments[0]);
                  order.add("restore-failed");
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK restore resignation callbacks";
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
