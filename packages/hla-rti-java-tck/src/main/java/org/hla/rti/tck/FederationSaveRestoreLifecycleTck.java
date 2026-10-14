package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.FederateHandleSaveStatusPair;
import hla.rti1516_2025.FederateRestoreStatus;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RestoreFailureReason;
import hla.rti1516_2025.RestoreStatus;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.SaveFailureReason;
import hla.rti1516_2025.SaveStatus;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Standard two-federate save/restore lifecycle, status, failure, and service boundaries. */
final class FederationSaveRestoreLifecycleTck {
   private FederationSaveRestoreLifecycleTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, CallbackModel callbackModel) throws Exception {
      RTIambassador lifecycle = factory.getRtiAmbassador();
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador peer = factory.getRtiAmbassador();
      Recorder lifecycleRecorder = new Recorder();
      Recorder ownerRecorder = new Recorder();
      Recorder peerRecorder = new Recorder();
      String federation = "java-tck-save-restore-lifecycle-" + UUID.randomUUID();
      String ownerName = "java-tck-save-restore-lifecycle-owner";
      String peerName = "java-tck-save-restore-lifecycle-peer";
      boolean lifecycleConnected = false, ownerConnected = false, peerConnected = false;
      boolean created = false, ownerJoined = false, peerJoined = false;
      try {
         assertInactiveServices(lifecycle, "java-tck-before-connect", "NotConnected");

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
         LogicalTime joinedTime = initialTime(owner);
         JavaTckSupport.expectFailure(
            () -> lifecycle.requestFederationSave("java-tck-boundary-save", joinedTime),
            "NotConnected", "timestamped save before connection");
         lifecycle.connect(lifecycleRecorder.proxy(), callbackModel);
         lifecycleConnected = true;
         JavaTckSupport.expectFailure(
            () -> lifecycle.requestFederationSave("java-tck-boundary-save", joinedTime),
            "FederateNotExecutionMember", "timestamped save before federation membership");
         assertInactiveServices(lifecycle, "java-tck-before-join", "FederateNotExecutionMember");
         JavaTckSupport.expectFailure(() -> owner.federateSaveBegun(),
            "SaveNotInitiated", "federateSaveBegun before a save request");
         JavaTckSupport.expectFailure(() -> owner.federateSaveComplete(),
            "FederateHasNotBegunSave", "federateSaveComplete before save preparation");
         JavaTckSupport.expectFailure(() -> owner.federateSaveNotComplete(),
            "FederateHasNotBegunSave", "federateSaveNotComplete before save preparation");
         JavaTckSupport.expectFailure(() -> owner.abortFederationSave(),
            "SaveNotInProgress", "abortFederationSave without a save in progress");
         JavaTckSupport.expectFailure(() -> owner.federateRestoreComplete(),
            "RestoreNotRequested", "federateRestoreComplete before a restore request");
         JavaTckSupport.expectFailure(() -> owner.federateRestoreNotComplete(),
            "RestoreNotRequested", "federateRestoreNotComplete before a restore request");
         JavaTckSupport.expectFailure(() -> owner.abortFederationRestore(),
            "RestoreNotInProgress", "abortFederationRestore without a restore in progress");
         lifecycle.disconnect();
         lifecycleConnected = false;

         String saveLabel = "java-tck-save-restore-lifecycle-save";
         owner.requestFederationSave(saveLabel);
         await(owner, peer, callbackModel, objectClassName,
            () -> ownerRecorder.saveInitiations.size() == 1
               && peerRecorder.saveInitiations.size() == 1,
            "save initiation callbacks");
         verifyUntimedSave(ownerRecorder, saveLabel);
         verifyUntimedSave(peerRecorder, saveLabel);
         querySaveStatus(owner, ownerRecorder, callbackModel, objectClassName,
            ownerHandle, peerHandle, SaveStatus.FEDERATE_INSTRUCTED_TO_SAVE,
            "instructed-to-save status");

         owner.federateSaveBegun();
         peer.federateSaveBegun();
         querySaveStatus(owner, ownerRecorder, callbackModel, objectClassName,
            ownerHandle, peerHandle, SaveStatus.FEDERATE_SAVING, "saving status");
         owner.federateSaveComplete();
         JavaTckSupport.check(ownerRecorder.savedCount == 0 && peerRecorder.savedCount == 0,
            "federationSaved arrived before every member completed saving");
         peer.federateSaveComplete();
         await(owner, peer, callbackModel, objectClassName,
            () -> ownerRecorder.savedCount == 1 && peerRecorder.savedCount == 1,
            "federationSaved callbacks");
         JavaTckSupport.check(ownerRecorder.saveFailures.isEmpty()
               && peerRecorder.saveFailures.isEmpty(),
            "successful save reported a failure");

         owner.requestFederationRestore(saveLabel);
         await(owner, peer, callbackModel, objectClassName,
            () -> ownerRecorder.restoreSuccesses.size() == 1
               && ownerRecorder.restoreBegunCount == 1 && peerRecorder.restoreBegunCount == 1
               && ownerRecorder.restoreInitiations.size() == 1
               && peerRecorder.restoreInitiations.size() == 1,
            "restore initiation callbacks");
         JavaTckSupport.check(saveLabel.equals(last(ownerRecorder.restoreSuccesses))
               && ownerRecorder.restoreRequestFailures.isEmpty()
               && peerRecorder.restoreRequestFailures.isEmpty(),
            "successful restore request returned an unexpected result");
         RestoreInitiation ownerRestore = last(ownerRecorder.restoreInitiations);
         RestoreInitiation peerRestore = last(peerRecorder.restoreInitiations);
         checkRestore(ownerRestore, saveLabel, ownerName);
         checkRestore(peerRestore, saveLabel, peerName);
         queryRestoreStatus(owner, ownerRecorder, callbackModel, objectClassName,
            RestoreStatus.FEDERATE_RESTORING, "restoring status");

         owner.federateRestoreComplete();
         peer.federateRestoreComplete();
         await(owner, peer, callbackModel, objectClassName,
            () -> ownerRecorder.restoredCount == 1 && peerRecorder.restoredCount == 1,
            "federationRestored callbacks");
         JavaTckSupport.check(ownerRecorder.restoreFailures.isEmpty()
               && peerRecorder.restoreFailures.isEmpty(),
            "successful restore reported a failure");
         JavaTckSupport.check(owner.getFederateHandle(ownerName).equals(
               ownerRestore.postRestoreHandle)
               && peer.getFederateHandle(peerName).equals(peerRestore.postRestoreHandle)
               && ownerName.equals(owner.getFederateName(ownerRestore.postRestoreHandle))
               && peerName.equals(peer.getFederateName(peerRestore.postRestoreHandle)),
            "post-restore federate handles did not round-trip through name lookups");
         queryRestoreStatus(owner, ownerRecorder, callbackModel, objectClassName,
            RestoreStatus.NO_RESTORE_IN_PROGRESS, "completed restore status");

         String failedSaveLabel = "java-tck-save-restore-lifecycle-failure";
         owner.requestFederationSave(failedSaveLabel);
         await(owner, peer, callbackModel, objectClassName,
            () -> ownerRecorder.saveInitiations.size() == 2
               && peerRecorder.saveInitiations.size() == 2,
            "failure-path save initiation");
         owner.federateSaveBegun();
         peer.federateSaveBegun();
         peer.federateSaveNotComplete();
         await(owner, peer, callbackModel, objectClassName,
            () -> ownerRecorder.saveFailures.size() == 1 && peerRecorder.saveFailures.size() == 1,
            "federationNotSaved callbacks");
         JavaTckSupport.check(last(ownerRecorder.saveFailures)
                  == SaveFailureReason.FEDERATE_REPORTED_FAILURE_DURING_SAVE
               && last(peerRecorder.saveFailures)
                  == SaveFailureReason.FEDERATE_REPORTED_FAILURE_DURING_SAVE,
            "federateSaveNotComplete returned the wrong failure reason");

         String abortedSaveLabel = "java-tck-save-restore-lifecycle-abort";
         owner.requestFederationSave(abortedSaveLabel);
         await(owner, peer, callbackModel, objectClassName,
            () -> ownerRecorder.saveInitiations.size() == 3
               && peerRecorder.saveInitiations.size() == 3,
            "aborted-save initiation callbacks");
         owner.abortFederationSave();
         await(owner, peer, callbackModel, objectClassName,
            () -> ownerRecorder.saveFailures.size() == 2 && peerRecorder.saveFailures.size() == 2,
            "save-abort callbacks");
         JavaTckSupport.check(last(ownerRecorder.saveFailures) == SaveFailureReason.SAVE_ABORTED
               && last(peerRecorder.saveFailures) == SaveFailureReason.SAVE_ABORTED,
            "abortFederationSave returned the wrong failure reason");
      } finally {
         if (lifecycleConnected) disconnect(lifecycle);
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

   private static void assertInactiveServices(RTIambassador ambassador, String label,
         String exceptionName) throws Exception {
      JavaTckSupport.expectFailure(() -> ambassador.requestFederationSave(label),
         exceptionName, "requestFederationSave " + exceptionName);
      JavaTckSupport.expectFailure(() -> ambassador.federateSaveBegun(),
         exceptionName, "federateSaveBegun " + exceptionName);
      JavaTckSupport.expectFailure(() -> ambassador.federateSaveComplete(),
         exceptionName, "federateSaveComplete " + exceptionName);
      JavaTckSupport.expectFailure(() -> ambassador.federateSaveNotComplete(),
         exceptionName, "federateSaveNotComplete " + exceptionName);
      JavaTckSupport.expectFailure(() -> ambassador.abortFederationSave(),
         exceptionName, "abortFederationSave " + exceptionName);
      JavaTckSupport.expectFailure(() -> ambassador.queryFederationSaveStatus(),
         exceptionName, "queryFederationSaveStatus " + exceptionName);
      JavaTckSupport.expectFailure(() -> ambassador.requestFederationRestore(label),
         exceptionName, "requestFederationRestore " + exceptionName);
      JavaTckSupport.expectFailure(() -> ambassador.federateRestoreComplete(),
         exceptionName, "federateRestoreComplete " + exceptionName);
      JavaTckSupport.expectFailure(() -> ambassador.federateRestoreNotComplete(),
         exceptionName, "federateRestoreNotComplete " + exceptionName);
      JavaTckSupport.expectFailure(() -> ambassador.abortFederationRestore(),
         exceptionName, "abortFederationRestore " + exceptionName);
      JavaTckSupport.expectFailure(() -> ambassador.queryFederationRestoreStatus(),
         exceptionName, "queryFederationRestoreStatus " + exceptionName);
   }

   private static LogicalTime initialTime(RTIambassador ambassador) throws Exception {
      LogicalTimeFactory<?, ?> factory = ambassador.getTimeFactory();
      return (LogicalTime) factory.makeInitial();
   }

   private static void verifyUntimedSave(Recorder recorder, String label) {
      JavaTckSupport.check(label.equals(last(recorder.saveInitiations))
            && !last(recorder.timestampedSaveInitiations),
         "untimed federation save returned timestamped or incorrect initiation data");
   }

   private static void querySaveStatus(RTIambassador owner, Recorder recorder,
         CallbackModel model, String objectClassName, FederateHandle ownerHandle,
         FederateHandle peerHandle, SaveStatus expected, String phase) throws Exception {
      int before = recorder.saveStatuses.size();
      owner.queryFederationSaveStatus();
      await(owner, null, model, objectClassName,
         () -> recorder.saveStatuses.size() > before, phase + " callback");
      FederateHandleSaveStatusPair[] statuses = last(recorder.saveStatuses);
      JavaTckSupport.check(statuses.length == 2,
         phase + " omitted a joined federate");
      checkSaveStatus(statuses, ownerHandle, expected, phase);
      checkSaveStatus(statuses, peerHandle, expected, phase);
   }

   private static void checkSaveStatus(FederateHandleSaveStatusPair[] statuses,
         FederateHandle handle, SaveStatus expected, String phase) {
      for (FederateHandleSaveStatusPair status : statuses) {
         if (handle.equals(status.handle)) {
            JavaTckSupport.check(status.status == expected,
               phase + " returned the wrong state");
            return;
         }
      }
      throw new AssertionError(phase + " omitted a joined federate handle");
   }

   private static void queryRestoreStatus(RTIambassador owner, Recorder recorder,
         CallbackModel model, String objectClassName, RestoreStatus expected, String phase)
         throws Exception {
      int before = recorder.restoreStatuses.size();
      owner.queryFederationRestoreStatus();
      await(owner, null, model, objectClassName,
         () -> recorder.restoreStatuses.size() > before, phase + " callback");
      FederateRestoreStatus[] statuses = last(recorder.restoreStatuses);
      JavaTckSupport.check(statuses.length == 2,
         phase + " omitted a joined federate");
      for (FederateRestoreStatus status : statuses) {
         JavaTckSupport.check(status.status == expected,
            phase + " returned the wrong state");
      }
   }

   private static void checkRestore(RestoreInitiation initiation, String label,
         String expectedName) {
      JavaTckSupport.check(label.equals(initiation.label)
            && expectedName.equals(initiation.federateName)
            && initiation.postRestoreHandle != null
            && initiation.postRestoreHandle.encodedLength() > 0,
         "restore initiation returned incomplete metadata");
   }

   private static <T> T last(List<T> values) { return values.get(values.size() - 1); }

   private static void await(RTIambassador first, RTIambassador second, CallbackModel model,
         String objectClassName, Condition condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && !condition.isMet()) {
         service(first, model, objectClassName);
         if (second != null) service(second, model, objectClassName);
         Thread.yield();
      }
      JavaTckSupport.check(condition.isMet(), "timed out waiting for " + description);
   }

   private static void service(RTIambassador ambassador, CallbackModel model,
         String objectClassName) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) ambassador.evokeMultipleCallbacks(0.0, 1.0);
      else ambassador.getObjectClassHandle(objectClassName);
   }

   @FunctionalInterface
   private interface Condition { boolean isMet() throws Exception; }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static final class RestoreInitiation {
      private final String label;
      private final String federateName;
      private final FederateHandle postRestoreHandle;

      private RestoreInitiation(String label, String federateName, FederateHandle handle) {
         this.label = label;
         this.federateName = federateName;
         postRestoreHandle = handle;
      }
   }

   private static final class Recorder {
      private final List<String> saveInitiations = new CopyOnWriteArrayList<>();
      private final List<Boolean> timestampedSaveInitiations = new CopyOnWriteArrayList<>();
      private final List<SaveFailureReason> saveFailures = new CopyOnWriteArrayList<>();
      private final List<FederateHandleSaveStatusPair[]> saveStatuses = new CopyOnWriteArrayList<>();
      private final List<String> restoreSuccesses = new CopyOnWriteArrayList<>();
      private final List<String> restoreRequestFailures = new CopyOnWriteArrayList<>();
      private final List<RestoreInitiation> restoreInitiations = new CopyOnWriteArrayList<>();
      private final List<RestoreFailureReason> restoreFailures = new CopyOnWriteArrayList<>();
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
                  timestampedSaveInitiations.add(arguments.length > 1);
               } else if ("federationSaved".equals(name)) {
                  savedCount++;
               } else if ("federationNotSaved".equals(name)) {
                  saveFailures.add((SaveFailureReason) arguments[0]);
               } else if ("federationSaveStatusResponse".equals(name)) {
                  saveStatuses.add(((FederateHandleSaveStatusPair[]) arguments[0]).clone());
               } else if ("requestFederationRestoreSucceeded".equals(name)) {
                  restoreSuccesses.add((String) arguments[0]);
               } else if ("requestFederationRestoreFailed".equals(name)) {
                  restoreRequestFailures.add((String) arguments[0]);
               } else if ("federationRestoreBegun".equals(name)) {
                  restoreBegunCount++;
               } else if ("initiateFederateRestore".equals(name)) {
                  restoreInitiations.add(new RestoreInitiation((String) arguments[0],
                     (String) arguments[1], (FederateHandle) arguments[2]));
               } else if ("federationRestored".equals(name)) {
                  restoredCount++;
               } else if ("federationNotRestored".equals(name)) {
                  restoreFailures.add((RestoreFailureReason) arguments[0]);
               } else if ("federationRestoreStatusResponse".equals(name)) {
                  restoreStatuses.add(((FederateRestoreStatus[]) arguments[0]).clone());
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK federation save/restore recorder";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
