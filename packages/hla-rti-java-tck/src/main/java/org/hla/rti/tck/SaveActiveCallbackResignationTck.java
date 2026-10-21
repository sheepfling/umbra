package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.SaveFailureReason;
import hla.rti1516_2025.exceptions.CallNotAllowedFromWithinCallback;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** A resignation inside Initiate Federate Save is rejected; one after it cancels the save. */
final class SaveActiveCallbackResignationTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private SaveActiveCallbackResignationTck() { }

   static void run(RtiFactory factory, String fom,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "active save-callback resignation is scoped to evoked callbacks");
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador held = factory.getRtiAmbassador();
      RTIambassador departing = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder(owner);
      Recorder heldCallbacks = new Recorder(held);
      Recorder departingCallbacks = new Recorder(departing);
      String federation = "java-tck-save-active-resignation-" + UUID.randomUUID();
      String firstLabel = "tck-active-callback-resignation-save";
      String secondLabel = "tck-after-active-callback-resignation-save";
      boolean ownerConnected = false;
      boolean heldConnected = false;
      boolean departingConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean heldJoined = false;
      boolean departingJoined = false;
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel);
         ownerConnected = true;
         held.connect(heldCallbacks.proxy(), callbackModel);
         heldConnected = true;
         departing.connect(departingCallbacks.proxy(), callbackModel);
         departingConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-save-owner", federation);
         ownerJoined = true;
         held.joinFederationExecution("java-tck-save-held", federation);
         heldJoined = true;
         departing.joinFederationExecution("java-tck-save-departing", federation);
         departingJoined = true;

         heldCallbacks.beginOnInitiate = true;
         departingCallbacks.beginOnInitiate = true;
         departingCallbacks.resignOnInitiate = true;
         owner.requestFederationSave(firstLabel);
         await(held, () -> heldCallbacks.labels.size() == 1,
            "held member save initiation");
         JavaTckSupport.check(heldCallbacks.order.equals(List.of("save-initiate")),
            "held member did not enter its first save callback");
         await(departing, () -> departingCallbacks.labels.size() == 1,
            "departing member save initiation");
         JavaTckSupport.check(departingCallbacks.resignationRejectedInsideCallback,
            "resignation inside Initiate Federate Save was not rejected");
         JavaTckSupport.check(departingCallbacks.order.equals(List.of("save-initiate")),
            "departing member received an unexpected callback before resignation");

         departing.resignFederationExecution(ResignAction.NO_ACTION);
         departingJoined = false;
         awaitBoth(owner, held,
            () -> ownerCallbacks.failures.size() == 1 && heldCallbacks.failures.size() == 1,
            "save failure after member resignation");
         JavaTckSupport.check(ownerCallbacks.failures.equals(
               List.of(SaveFailureReason.FEDERATE_RESIGNED_DURING_SAVE))
               && heldCallbacks.failures.equals(
                  List.of(SaveFailureReason.FEDERATE_RESIGNED_DURING_SAVE)),
            "remaining members did not receive the resignation save-failure reason");
         while (departing.evokeCallback(0.0)) { }
         JavaTckSupport.check(departingCallbacks.failures.isEmpty(),
            "resigned member received a stale save-failure callback");
         JavaTckSupport.check(ownerCallbacks.order.equals(
               List.of("save-initiate", "save-failed"))
               && heldCallbacks.order.equals(List.of("save-initiate", "save-failed")),
            "save initiation and failure callbacks arrived in the wrong order");

         heldCallbacks.beginOnInitiate = false;
         ownerCallbacks.order.clear();
         heldCallbacks.order.clear();
         owner.requestFederationSave(secondLabel);
         awaitBoth(owner, held,
            () -> ownerCallbacks.labels.size() == 2 && heldCallbacks.labels.size() == 2,
            "fresh save initiation after resignation failure");
         JavaTckSupport.check(ownerCallbacks.labels.equals(List.of(firstLabel, secondLabel))
               && heldCallbacks.labels.equals(List.of(firstLabel, secondLabel)),
            "fresh save did not initiate with the expected labels");
         owner.federateSaveBegun();
         held.federateSaveBegun();
         owner.federateSaveComplete();
         held.federateSaveComplete();
         awaitBoth(owner, held,
            () -> ownerCallbacks.savedCount == 1 && heldCallbacks.savedCount == 1,
            "fresh save completion after resignation failure");
         JavaTckSupport.check(ownerCallbacks.order.equals(
               List.of("save-initiate", "save-complete"))
               && heldCallbacks.order.equals(List.of("save-initiate", "save-complete")),
            "fresh save callbacks did not complete in order");

         held.resignFederationExecution(ResignAction.NO_ACTION);
         heldJoined = false;
         owner.resignFederationExecution(ResignAction.NO_ACTION);
         ownerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (departingJoined) resign(departing);
         if (heldJoined) resign(held);
         if (ownerJoined) resign(owner);
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (departingConnected) disconnect(departing);
         if (heldConnected) disconnect(held);
         if (ownerConnected) disconnect(owner);
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
      private final RTIambassador ambassador;
      private final List<String> labels = new CopyOnWriteArrayList<>();
      private final List<String> order = new CopyOnWriteArrayList<>();
      private final List<SaveFailureReason> failures = new CopyOnWriteArrayList<>();
      private volatile int savedCount;
      private volatile boolean beginOnInitiate;
      private volatile boolean resignOnInitiate;
      private volatile boolean resignationRejectedInsideCallback;

      private Recorder(RTIambassador ambassador) { this.ambassador = ambassador; }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("initiateFederateSave".equals(name)) {
                  labels.add((String) arguments[0]);
                  order.add("save-initiate");
                  if (beginOnInitiate) ambassador.federateSaveBegun();
                  if (resignOnInitiate) {
                     try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
                     catch (CallNotAllowedFromWithinCallback expected) {
                        resignationRejectedInsideCallback = true;
                     }
                  }
               } else if ("federationNotSaved".equals(name)) {
                  failures.add((SaveFailureReason) arguments[0]);
                  order.add("save-failed");
               } else if ("federationSaved".equals(name)) {
                  ++savedCount;
                  order.add("save-complete");
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK save callback recorder";
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
