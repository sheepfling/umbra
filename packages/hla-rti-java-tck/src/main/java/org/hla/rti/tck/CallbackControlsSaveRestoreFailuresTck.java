package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RestoreFailureReason;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.SaveFailureReason;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

/** P2 callback-control checks for federate-reported save and restore failures. */
final class CallbackControlsSaveRestoreFailuresTck {
   private CallbackControlsSaveRestoreFailuresTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador peer = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder peerRecorder = new Recorder();
      String federation = "java-tck-callback-save-restore-failures-" + UUID.randomUUID();
      String failedSaveLabel = "java-tck-callback-failed-save";
      String restoreSourceLabel = "java-tck-callback-restore-source";
      boolean ownerConnected = false;
      boolean peerConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean peerJoined = false;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         peer.connect(peerRecorder.proxy(), callbackModel);
         peerConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-failure-owner", federation);
         ownerJoined = true;
         peer.joinFederationExecution("java-tck-failure-peer", federation);
         peerJoined = true;

         peer.disableCallbacks();
         owner.requestFederationSave(failedSaveLabel);
         await(callbackModel, () -> !ownerRecorder.saveInitiations.isEmpty(), owner,
            objectClassName);
         service(peer, callbackModel, objectClassName);
         JavaTckSupport.check(peerRecorder.saveInitiations.isEmpty(),
            "disabled callbacks exposed a failed-save initiation");
         peer.enableCallbacks();
         await(callbackModel, () -> !peerRecorder.saveInitiations.isEmpty(), peer,
            objectClassName);
         JavaTckSupport.check(failedSaveLabel.equals(last(ownerRecorder.saveInitiations))
               && failedSaveLabel.equals(last(peerRecorder.saveInitiations)),
            "failed-save initiation returned the wrong label");

         owner.federateSaveBegun();
         peer.federateSaveBegun();
         int ownerSaveFailuresBefore = ownerRecorder.saveFailures.size();
         int peerSaveFailuresBefore = peerRecorder.saveFailures.size();
         peer.disableCallbacks();
         peer.federateSaveNotComplete();
         await(callbackModel,
            () -> ownerRecorder.saveFailures.size() > ownerSaveFailuresBefore,
            owner, objectClassName);
         service(peer, callbackModel, objectClassName);
         JavaTckSupport.check(peerRecorder.saveFailures.size() == peerSaveFailuresBefore,
            "disabled callbacks exposed federationNotSaved");
         peer.enableCallbacks();
         await(callbackModel, () -> peerRecorder.saveFailures.size() > peerSaveFailuresBefore,
            peer, objectClassName);
         JavaTckSupport.check(last(ownerRecorder.saveFailures)
               == SaveFailureReason.FEDERATE_REPORTED_FAILURE_DURING_SAVE
               && last(peerRecorder.saveFailures)
                  == SaveFailureReason.FEDERATE_REPORTED_FAILURE_DURING_SAVE,
            "federationNotSaved returned the wrong failure reason");

         int ownerSaveCountBefore = ownerRecorder.savedCount;
         int peerSaveCountBefore = peerRecorder.savedCount;
         owner.requestFederationSave(restoreSourceLabel);
         await(callbackModel,
            () -> ownerRecorder.saveInitiations.size() > 1
               && peerRecorder.saveInitiations.size() > 1,
            owner, peer, objectClassName);
         owner.federateSaveBegun();
         peer.federateSaveBegun();
         owner.federateSaveComplete();
         peer.federateSaveComplete();
         await(callbackModel, () -> ownerRecorder.savedCount > ownerSaveCountBefore
               && peerRecorder.savedCount > peerSaveCountBefore,
            owner, peer, objectClassName);

         int ownerRestoreSuccessBefore = ownerRecorder.restoreSuccesses.size();
         int peerRestoreBegunBefore = peerRecorder.restoreBegunCount;
         int ownerRestoreBegunBefore = ownerRecorder.restoreBegunCount;
         int ownerRestoreInitiationBefore = ownerRecorder.restoreInitiations;
         int peerRestoreInitiationBefore = peerRecorder.restoreInitiations;
         owner.requestFederationRestore(restoreSourceLabel);
         await(callbackModel,
            () -> ownerRecorder.restoreSuccesses.size() > ownerRestoreSuccessBefore
               && ownerRecorder.restoreBegunCount > ownerRestoreBegunBefore
               && peerRecorder.restoreBegunCount > peerRestoreBegunBefore
               && ownerRecorder.restoreInitiations > ownerRestoreInitiationBefore
               && peerRecorder.restoreInitiations > peerRestoreInitiationBefore,
            owner, peer, objectClassName);
         JavaTckSupport.check(restoreSourceLabel.equals(last(ownerRecorder.restoreSuccesses))
               && ownerRecorder.restoreRequestFailures.isEmpty()
               && peerRecorder.restoreRequestFailures.isEmpty(),
            "restore-source request returned an incorrect result");

         owner.federateRestoreComplete();
         int ownerRestoreFailuresBefore = ownerRecorder.restoreFailures.size();
         int peerRestoreFailuresBefore = peerRecorder.restoreFailures.size();
         peer.disableCallbacks();
         peer.federateRestoreNotComplete();
         await(callbackModel,
            () -> ownerRecorder.restoreFailures.size() > ownerRestoreFailuresBefore,
            owner, objectClassName);
         service(peer, callbackModel, objectClassName);
         JavaTckSupport.check(peerRecorder.restoreFailures.size() == peerRestoreFailuresBefore,
            "disabled callbacks exposed federationNotRestored");
         peer.enableCallbacks();
         await(callbackModel,
            () -> peerRecorder.restoreFailures.size() > peerRestoreFailuresBefore,
            peer, objectClassName);
         JavaTckSupport.check(last(ownerRecorder.restoreFailures)
               == RestoreFailureReason.FEDERATE_REPORTED_FAILURE_DURING_RESTORE
               && last(peerRecorder.restoreFailures)
                  == RestoreFailureReason.FEDERATE_REPORTED_FAILURE_DURING_RESTORE,
            "federationNotRestored returned the wrong failure reason");
      } finally {
         if (peerJoined) resign(peer);
         if (ownerJoined) resign(owner);
         if (created) {
            try { owner.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (peerConnected) disconnect(peer);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static <T> T last(List<T> values) {
      return values.get(values.size() - 1);
   }

   private static void service(RTIambassador ambassador, CallbackModel model,
         String objectClassName) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) ambassador.evokeMultipleCallbacks(0.0, 1.0);
      else ambassador.getObjectClassHandle(objectClassName);
   }

   private static void await(CallbackModel model, Condition condition, RTIambassador first,
         String objectClassName) throws Exception {
      await(model, condition, first, null, objectClassName);
   }

   private static void await(CallbackModel model, Condition condition, RTIambassador first,
         RTIambassador second, String objectClassName) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline) {
         service(first, model, objectClassName);
         if (second != null) service(second, model, objectClassName);
         if (condition.isMet()) return;
         Thread.yield();
      }
      throw new AssertionError("save/restore failure callback did not arrive before deadline");
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
      private final List<String> saveInitiations = new ArrayList<>();
      private final List<SaveFailureReason> saveFailures = new ArrayList<>();
      private final List<String> restoreSuccesses = new ArrayList<>();
      private final List<String> restoreRequestFailures = new ArrayList<>();
      private final List<RestoreFailureReason> restoreFailures = new ArrayList<>();
      private int savedCount;
      private int restoreBegunCount;
      private int restoreInitiations;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("initiateFederateSave".equals(name)) saveInitiations.add((String) arguments[0]);
               if ("federationNotSaved".equals(name)) {
                  saveFailures.add((SaveFailureReason) arguments[0]);
               }
               if ("federationSaved".equals(name)) savedCount++;
               if ("requestFederationRestoreSucceeded".equals(name)) {
                  restoreSuccesses.add((String) arguments[0]);
               }
               if ("requestFederationRestoreFailed".equals(name)) {
                  restoreRequestFailures.add((String) arguments[0]);
               }
               if ("federationRestoreBegun".equals(name)) restoreBegunCount++;
               if ("initiateFederateRestore".equals(name)) restoreInitiations++;
               if ("federationNotRestored".equals(name)) {
                  restoreFailures.add((RestoreFailureReason) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK save/restore failure recorder";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == arguments[0];
               }
               return null;
            });
      }
   }
}
