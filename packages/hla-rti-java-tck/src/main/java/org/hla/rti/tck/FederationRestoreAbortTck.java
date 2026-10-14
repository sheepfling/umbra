package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.FederateRestoreStatus;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RestoreFailureReason;
import hla.rti1516_2025.RestoreStatus;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.SaveFailureReason;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Abort an active restore and verify both terminal callbacks and post-abort status. */
final class FederationRestoreAbortTck {
   private FederationRestoreAbortTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador peer = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder peerRecorder = new Recorder();
      String federation = "java-tck-federation-restore-abort-" + UUID.randomUUID();
      String saveLabel = "java-tck-restore-abort-save";
      String ownerName = "java-tck-restore-abort-owner";
      String peerName = "java-tck-restore-abort-peer";
      boolean ownerConnected = false, peerConnected = false, created = false;
      boolean ownerJoined = false, peerJoined = false;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         peer.connect(peerRecorder.proxy(), callbackModel);
         peerConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle ownerHandle = owner.joinFederationExecution(ownerName, federation);
         ownerJoined = true;
         FederateHandle peerHandle = peer.joinFederationExecution(peerName, federation);
         peerJoined = true;

         owner.requestFederationSave(saveLabel);
         await(owner, peer, callbackModel, objectClassName,
            () -> ownerRecorder.saveInitiations.size() == 1
               && peerRecorder.saveInitiations.size() == 1,
            "save initiation callbacks");
         JavaTckSupport.check(saveLabel.equals(last(ownerRecorder.saveInitiations))
               && saveLabel.equals(last(peerRecorder.saveInitiations)),
            "save initiation returned the wrong label");

         owner.federateSaveBegun();
         peer.federateSaveBegun();
         owner.federateSaveComplete();
         peer.federateSaveComplete();
         await(owner, peer, callbackModel, objectClassName,
            () -> ownerRecorder.savedCount == 1 && peerRecorder.savedCount == 1,
            "successful save completion callbacks");
         JavaTckSupport.check(ownerRecorder.saveFailures.isEmpty()
               && peerRecorder.saveFailures.isEmpty(),
            "restore-abort setup reported a save failure");

         owner.requestFederationRestore(saveLabel);
         await(owner, peer, callbackModel, objectClassName,
            () -> ownerRecorder.restoreSuccesses.size() == 1
               && ownerRecorder.restoreBegunCount == 1
               && peerRecorder.restoreBegunCount == 1
               && ownerRecorder.restoreInitiations.size() == 1
               && peerRecorder.restoreInitiations.size() == 1,
            "restore initiation callbacks");
         JavaTckSupport.check(saveLabel.equals(last(ownerRecorder.restoreSuccesses))
               && ownerRecorder.restoreRequestFailures.isEmpty()
               && peerRecorder.restoreRequestFailures.isEmpty(),
            "restore request returned an unexpected result");
         checkInitiation(ownerRecorder.restoreInitiations.get(0), saveLabel, ownerName);
         checkInitiation(peerRecorder.restoreInitiations.get(0), saveLabel, peerName);
         JavaTckSupport.check(ownerHandle != null && ownerHandle.encodedLength() > 0
               && peerHandle != null && peerHandle.encodedLength() > 0
               && ownerRecorder.restoreInitiations.get(0).postRestoreHandle != null
               && ownerRecorder.restoreInitiations.get(0).postRestoreHandle.encodedLength() > 0
               && peerRecorder.restoreInitiations.get(0).postRestoreHandle != null
               && peerRecorder.restoreInitiations.get(0).postRestoreHandle.encodedLength() > 0,
            "restore initiation omitted a valid post-restore federate handle");

         owner.abortFederationRestore();
         await(owner, peer, callbackModel, objectClassName,
            () -> ownerRecorder.restoreFailures.size() == 1
               && peerRecorder.restoreFailures.size() == 1,
            "terminal restore-abort callbacks");
         JavaTckSupport.check(last(ownerRecorder.restoreFailures)
                  == RestoreFailureReason.RESTORE_ABORTED
               && last(peerRecorder.restoreFailures) == RestoreFailureReason.RESTORE_ABORTED
               && ownerRecorder.restoredCount == 0 && peerRecorder.restoredCount == 0,
            "Abort Federation Restore returned an incorrect terminal callback result");

         owner.queryFederationRestoreStatus();
         await(owner, null, callbackModel, objectClassName,
            () -> !ownerRecorder.restoreStatuses.isEmpty(),
            "terminal restore status response");
         FederateRestoreStatus[] statuses = last(ownerRecorder.restoreStatuses);
         JavaTckSupport.check(statuses.length == 2,
            "restore-status response omitted a joined federate");
         for (FederateRestoreStatus status : statuses) {
            JavaTckSupport.check(status.status == RestoreStatus.NO_RESTORE_IN_PROGRESS,
               "restore abort left a federate in a restore state");
         }
      } finally {
         if (peerJoined) resign(peer);
         if (ownerJoined) resign(owner);
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (peerConnected) disconnect(peer);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static void checkInitiation(RestoreInitiation initiation, String label,
         String federateName) throws Exception {
      JavaTckSupport.check(label.equals(initiation.label)
            && federateName.equals(initiation.federateName)
            && initiation.postRestoreHandle != null
            && initiation.postRestoreHandle.encodedLength() > 0,
         "restore initiation returned incomplete metadata");
   }

   private static <T> T last(List<T> values) {
      return values.get(values.size() - 1);
   }

   private static void await(RTIambassador first, RTIambassador second, CallbackModel model,
         String objectClassName, Condition condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && !condition.isTrue()) {
         service(first, model, objectClassName);
         if (second != null) service(second, model, objectClassName);
         Thread.yield();
      }
      JavaTckSupport.check(condition.isTrue(), "timed out waiting for " + description);
   }

   private static void service(RTIambassador ambassador, CallbackModel model,
         String objectClassName) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) ambassador.evokeMultipleCallbacks(0.0, 1.0);
      else ambassador.getObjectClassHandle(objectClassName);
   }

   @FunctionalInterface
   private interface Condition { boolean isTrue() throws Exception; }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static final class RestoreInitiation {
      private final String label;
      private final String federateName;
      private final FederateHandle postRestoreHandle;

      private RestoreInitiation(String label, String federateName,
            FederateHandle postRestoreHandle) {
         this.label = label;
         this.federateName = federateName;
         this.postRestoreHandle = postRestoreHandle;
      }
   }

   private static final class Recorder {
      private final List<String> saveInitiations = new CopyOnWriteArrayList<>();
      private final List<SaveFailureReason> saveFailures = new CopyOnWriteArrayList<>();
      private final List<String> restoreSuccesses = new CopyOnWriteArrayList<>();
      private final List<String> restoreRequestFailures = new CopyOnWriteArrayList<>();
      private final List<RestoreFailureReason> restoreFailures = new CopyOnWriteArrayList<>();
      private final List<RestoreInitiation> restoreInitiations = new CopyOnWriteArrayList<>();
      private final List<FederateRestoreStatus[]> restoreStatuses = new CopyOnWriteArrayList<>();
      private volatile int savedCount;
      private volatile int restoreBegunCount;
      private volatile int restoredCount;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("initiateFederateSave".equals(name)) {
                  saveInitiations.add((String) arguments[0]);
               } else if ("federationSaved".equals(name)) {
                  savedCount++;
               } else if ("federationNotSaved".equals(name)) {
                  saveFailures.add((SaveFailureReason) arguments[0]);
               } else if ("requestFederationRestoreSucceeded".equals(name)) {
                  restoreSuccesses.add((String) arguments[0]);
               } else if ("requestFederationRestoreFailed".equals(name)) {
                  restoreRequestFailures.add((String) arguments[0]);
               } else if ("federationRestoreBegun".equals(name)) {
                  restoreBegunCount++;
               } else if ("initiateFederateRestore".equals(name)) {
                  restoreInitiations.add(new RestoreInitiation((String) arguments[0],
                     (String) arguments[1], (FederateHandle) arguments[2]));
               } else if ("federationNotRestored".equals(name)) {
                  restoreFailures.add((RestoreFailureReason) arguments[0]);
               } else if ("federationRestored".equals(name)) {
                  restoredCount++;
               } else if ("federationRestoreStatusResponse".equals(name)) {
                  restoreStatuses.add(((FederateRestoreStatus[]) arguments[0]).clone());
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK restore-abort recorder";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
