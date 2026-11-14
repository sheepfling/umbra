package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.FederateHandleSet;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.atomic.AtomicInteger;

/** Restores a completed save after destroying and recreating its federation execution. */
final class RestoreSaveAfterFederationDestructionTck {
   private RestoreSaveAfterFederationDestructionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      String federation = "java-tck-restore-save-after-destruction-" + UUID.randomUUID();
      String ownerName = "java-tck-restore-save-after-destruction-owner";
      String peerName = "java-tck-restore-save-after-destruction-peer";
      String saveLabel = "java-tck-save-after-execution-destruction";
      String synchronizationLabel = "java-tck-sync-after-execution-destruction";
      byte[] tag = new byte[] {(byte) 'P', (byte) 'D'};

      RTIambassador originalOwner = factory.getRtiAmbassador();
      RTIambassador originalPeer = factory.getRtiAmbassador();
      Recorder originalOwnerRecorder = new Recorder();
      Recorder originalPeerRecorder = new Recorder();
      boolean originalOwnerConnected = false;
      boolean originalPeerConnected = false;
      boolean originalCreated = false;
      boolean originalOwnerJoined = false;
      boolean originalPeerJoined = false;

      RTIambassador restoredOwner = factory.getRtiAmbassador();
      RTIambassador restoredPeer = factory.getRtiAmbassador();
      Recorder restoredOwnerRecorder = new Recorder();
      Recorder restoredPeerRecorder = new Recorder();
      boolean restoredOwnerConnected = false;
      boolean restoredPeerConnected = false;
      boolean restoredCreated = false;
      boolean restoredOwnerJoined = false;
      boolean restoredPeerJoined = false;

      try {
         originalOwner.connect(originalOwnerRecorder.proxy(), callbackModel);
         originalOwnerConnected = true;
         originalPeer.connect(originalPeerRecorder.proxy(), callbackModel);
         originalPeerConnected = true;
         originalOwner.createFederationExecution(federation, fom, timeImplementation);
         originalCreated = true;
         FederateHandle originalOwnerHandle =
            originalOwner.joinFederationExecution(ownerName, federation);
         originalOwnerJoined = true;
         FederateHandle originalPeerHandle =
            originalPeer.joinFederationExecution(peerName, federation);
         originalPeerJoined = true;

         FederateHandleSet synchronizationMembers =
            originalOwner.getFederateHandleSetFactory().create();
         synchronizationMembers.add(originalOwnerHandle);
         synchronizationMembers.add(originalPeerHandle);
         originalOwner.registerFederationSynchronizationPoint(
            synchronizationLabel, tag, synchronizationMembers);
         await(originalOwner, originalPeer, callbackModel, ownerName, peerName,
            () -> originalOwnerRecorder.registrations.contains(synchronizationLabel)
               && hasAnnouncement(originalOwnerRecorder, synchronizationLabel, tag)
               && hasAnnouncement(originalPeerRecorder, synchronizationLabel, tag),
            "persistent synchronization-point registration");
         JavaTckSupport.check(originalOwnerRecorder.registrationFailures.isEmpty()
               && originalOwnerRecorder.registrations.size() == 1
               && synchronizationLabel.equals(originalOwnerRecorder.registrations.get(0))
               && originalOwnerRecorder.announcements.size() == 1
               && originalPeerRecorder.announcements.size() == 1
               && hasAnnouncement(originalOwnerRecorder, synchronizationLabel, tag)
               && hasAnnouncement(originalPeerRecorder, synchronizationLabel, tag),
            "the synchronization point was not announced before the federation save");

         originalOwner.requestFederationSave(saveLabel);
         await(originalOwner, originalPeer, callbackModel, ownerName, peerName,
            () -> originalOwnerRecorder.saveInitiations.size() == 1
               && originalPeerRecorder.saveInitiations.size() == 1,
            "persistent synchronization-point save initiation");
         JavaTckSupport.check(saveLabel.equals(originalOwnerRecorder.saveInitiations.get(0))
               && saveLabel.equals(originalPeerRecorder.saveInitiations.get(0)),
            "the persistent synchronization-point save used a different label");
         originalOwner.federateSaveBegun();
         originalPeer.federateSaveBegun();
         originalOwner.federateSaveComplete();
         originalPeer.federateSaveComplete();
         await(originalOwner, originalPeer, callbackModel, ownerName, peerName,
            () -> originalOwnerRecorder.savedCount.get() == 1
               && originalPeerRecorder.savedCount.get() == 1,
            "persistent synchronization-point federation saved");
         JavaTckSupport.check(originalOwnerRecorder.saveFailures.isEmpty()
               && originalPeerRecorder.saveFailures.isEmpty(),
            "the completed federation save reported a failure");

         originalPeer.resignFederationExecution(ResignAction.NO_ACTION);
         originalPeerJoined = false;
         originalOwner.resignFederationExecution(ResignAction.NO_ACTION);
         originalOwnerJoined = false;
         originalOwner.destroyFederationExecution(federation);
         originalCreated = false;
         originalPeer.disconnect();
         originalPeerConnected = false;
         originalOwner.disconnect();
         originalOwnerConnected = false;

         restoredOwner.connect(restoredOwnerRecorder.proxy(), callbackModel);
         restoredOwnerConnected = true;
         restoredPeer.connect(restoredPeerRecorder.proxy(), callbackModel);
         restoredPeerConnected = true;
         restoredOwner.createFederationExecution(federation, fom, timeImplementation);
         restoredCreated = true;
         restoredOwner.joinFederationExecution(ownerName, federation);
         restoredOwnerJoined = true;
         restoredPeer.joinFederationExecution(peerName, federation);
         restoredPeerJoined = true;
         restoredOwner.requestFederationRestore(saveLabel);
         await(restoredOwner, restoredPeer, callbackModel, ownerName, peerName,
            () -> !restoredOwnerRecorder.restoreRequestFailures.isEmpty()
               || (!restoredOwnerRecorder.restoreSuccesses.isEmpty()
                  && restoredOwnerRecorder.restoreInitiations.size() == 1
                  && restoredPeerRecorder.restoreInitiations.size() == 1),
            "restore request after federation execution destruction");
         JavaTckSupport.check(restoredOwnerRecorder.restoreRequestFailures.isEmpty(),
            "the post-destruction restore request was rejected");
         JavaTckSupport.check(restoredOwnerRecorder.restoreSuccesses.size() == 1
               && saveLabel.equals(restoredOwnerRecorder.restoreSuccesses.get(0))
               && restoredOwnerRecorder.restoreInitiations.size() == 1
               && restoredPeerRecorder.restoreInitiations.size() == 1
               && saveLabel.equals(restoredOwnerRecorder.restoreInitiations.get(0).label)
               && saveLabel.equals(restoredPeerRecorder.restoreInitiations.get(0).label),
            "the post-destruction restore used an unexpected save label");

         restoredOwner.federateRestoreComplete();
         restoredPeer.federateRestoreComplete();
         await(restoredOwner, restoredPeer, callbackModel, ownerName, peerName,
            () -> restoredOwnerRecorder.restoredCount.get() == 1
               && restoredPeerRecorder.restoredCount.get() == 1,
            "federation restored after execution destruction");
         JavaTckSupport.check(restoredOwnerRecorder.restoreFailures.isEmpty()
               && restoredPeerRecorder.restoreFailures.isEmpty(),
            "the saved synchronization-point state failed to restore");

         restoredOwner.synchronizationPointAchieved(synchronizationLabel);
         restoredPeer.synchronizationPointAchieved(synchronizationLabel);
         await(restoredOwner, restoredPeer, callbackModel, ownerName, peerName,
            () -> hasSuccessfulSynchronization(restoredOwnerRecorder, synchronizationLabel)
               && hasSuccessfulSynchronization(restoredPeerRecorder, synchronizationLabel),
            "saved synchronization point completed after execution recreation");
         JavaTckSupport.check(hasSuccessfulSynchronization(
               restoredOwnerRecorder, synchronizationLabel)
               && restoredOwnerRecorder.synchronizations.size() == 1
               && hasSuccessfulSynchronization(restoredPeerRecorder, synchronizationLabel)
               && restoredPeerRecorder.synchronizations.size() == 1,
            "the pre-destruction synchronization point did not survive restore");
      } finally {
         if (restoredPeerJoined) resign(restoredPeer);
         if (restoredOwnerJoined) resign(restoredOwner);
         if (restoredCreated) destroy(restoredOwner, federation);
         if (restoredPeerConnected) disconnect(restoredPeer);
         if (restoredOwnerConnected) disconnect(restoredOwner);
         if (originalPeerJoined) resign(originalPeer);
         if (originalOwnerJoined) resign(originalOwner);
         if (originalCreated) destroy(originalOwner, federation);
         if (originalPeerConnected) disconnect(originalPeer);
         if (originalOwnerConnected) disconnect(originalOwner);
      }
   }

   private static void await(RTIambassador owner, RTIambassador peer,
         CallbackModel callbackModel, String ownerName, String peerName,
         Condition condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && !condition.isMet()) {
         service(owner, callbackModel, ownerName);
         service(peer, callbackModel, peerName);
         Thread.yield();
      }
      JavaTckSupport.check(condition.isMet(), "timed out waiting for " + description);
   }

   private static void service(RTIambassador ambassador, CallbackModel callbackModel,
         String federateName) throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) {
         ambassador.evokeMultipleCallbacks(0.0, 1.0);
      } else {
         ambassador.getFederateHandle(federateName);
      }
   }

   private static boolean hasAnnouncement(Recorder recorder, String label, byte[] tag) {
      for (Announcement announcement : recorder.announcements) {
         if (label.equals(announcement.label) && Arrays.equals(tag, announcement.tag)) {
            return true;
         }
      }
      return false;
   }

   private static boolean hasSuccessfulSynchronization(Recorder recorder, String label) {
      for (Synchronization completion : recorder.synchronizations) {
         if (label.equals(completion.label) && completion.failedSetEmpty) return true;
      }
      return false;
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void destroy(RTIambassador ambassador, String federation) {
      try { ambassador.destroyFederationExecution(federation); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); }
      catch (Exception ignored) { }
   }

   @FunctionalInterface
   private interface Condition { boolean isMet() throws Exception; }

   private static final class Announcement {
      private final String label;
      private final byte[] tag;
      private Announcement(String label, byte[] tag) {
         this.label = label;
         this.tag = tag;
      }
   }

   private static final class RestoreInitiation {
      private final String label;
      private RestoreInitiation(String label) { this.label = label; }
   }

   private static final class Synchronization {
      private final String label;
      private final boolean failedSetEmpty;
      private Synchronization(String label, boolean failedSetEmpty) {
         this.label = label;
         this.failedSetEmpty = failedSetEmpty;
      }
   }

   private static final class Recorder {
      private final List<String> registrations = new CopyOnWriteArrayList<>();
      private final List<String> registrationFailures = new CopyOnWriteArrayList<>();
      private final List<Announcement> announcements = new CopyOnWriteArrayList<>();
      private final List<String> saveInitiations = new CopyOnWriteArrayList<>();
      private final List<Object> saveFailures = new CopyOnWriteArrayList<>();
      private final List<String> restoreSuccesses = new CopyOnWriteArrayList<>();
      private final List<String> restoreRequestFailures = new CopyOnWriteArrayList<>();
      private final List<RestoreInitiation> restoreInitiations = new CopyOnWriteArrayList<>();
      private final List<Object> restoreFailures = new CopyOnWriteArrayList<>();
      private final List<Synchronization> synchronizations = new CopyOnWriteArrayList<>();
      private final AtomicInteger savedCount = new AtomicInteger();
      private final AtomicInteger restoredCount = new AtomicInteger();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("synchronizationPointRegistrationSucceeded".equals(name)) {
                  registrations.add((String) arguments[0]);
               } else if ("synchronizationPointRegistrationFailed".equals(name)) {
                  registrationFailures.add((String) arguments[0]);
               } else if ("announceSynchronizationPoint".equals(name)) {
                  announcements.add(new Announcement((String) arguments[0],
                     ((byte[]) arguments[1]).clone()));
               } else if ("initiateFederateSave".equals(name)) {
                  saveInitiations.add((String) arguments[0]);
               } else if ("federationSaved".equals(name)) {
                  savedCount.incrementAndGet();
               } else if ("federationNotSaved".equals(name)) {
                  saveFailures.add(arguments[0]);
               } else if ("requestFederationRestoreSucceeded".equals(name)) {
                  restoreSuccesses.add((String) arguments[0]);
               } else if ("requestFederationRestoreFailed".equals(name)) {
                  restoreRequestFailures.add((String) arguments[0]);
               } else if ("initiateFederateRestore".equals(name)) {
                  restoreInitiations.add(new RestoreInitiation((String) arguments[0]));
               } else if ("federationRestored".equals(name)) {
                  restoredCount.incrementAndGet();
               } else if ("federationNotRestored".equals(name)) {
                  restoreFailures.add(arguments[0]);
               } else if ("federationSynchronized".equals(name)) {
                  FederateHandleSet failed = ((FederateHandleSet) arguments[1]).clone();
                  synchronizations.add(new Synchronization((String) arguments[0],
                     failed.isEmpty()));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK save/restore recorder";
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
