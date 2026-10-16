package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.FederateHandleSet;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.SynchronizationPointFailureReason;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** P2.7 multi-member synchronization registration and barrier behavior. */
final class SynchronizationTck {
   private SynchronizationTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador peer = factory.getRtiAmbassador();
      RTIambassador late = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder peerRecorder = new Recorder();
      Recorder lateRecorder = new Recorder();
      String federation = "java-tck-sync-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean peerConnected = false;
      boolean lateConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean peerJoined = false;
      boolean lateJoined = false;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         peer.connect(peerRecorder.proxy(), callbackModel);
         peerConnected = true;
         late.connect(lateRecorder.proxy(), callbackModel);
         lateConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle ownerHandle = owner.joinFederationExecution(
            "java-tck-sync-owner", federation);
         ownerJoined = true;
         FederateHandle peerHandle = peer.joinFederationExecution(
            "java-tck-sync-peer", federation);
         peerJoined = true;

         String label = "java-tck-sync-all-" + UUID.randomUUID();
         byte[] tag = new byte[] {0x01, 0x02, 0x03};
         Announcement expectedAnnouncement = new Announcement(label, tag);
         owner.registerFederationSynchronizationPoint(label, tag);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(ownerRecorder.registrations.isEmpty()
                  && ownerRecorder.announcements.isEmpty()
                  && peerRecorder.announcements.isEmpty(),
               "evoked synchronization callbacks ran before callback servicing");
         }
         awaitCallbacks(callbackModel, () -> ownerRecorder.registrations.contains(label)
               && ownerRecorder.announcements.contains(expectedAnnouncement)
               && peerRecorder.announcements.contains(expectedAnnouncement), owner, peer);
         JavaTckSupport.check(ownerRecorder.registrations.contains(label),
            "synchronization registration did not succeed for the registering member");
         JavaTckSupport.check(ownerRecorder.announcements.contains(new Announcement(label, tag))
               && peerRecorder.announcements.contains(new Announcement(label, tag)),
            "current synchronization members did not receive the announcement");
         JavaTckSupport.check(peerRecorder.registrations.isEmpty(),
            "non-registering member received a registration-success callback");

         owner.registerFederationSynchronizationPoint(label, new byte[] {0x09});
         awaitCallbacks(callbackModel, () -> ownerRecorder.failures.contains(new RegistrationFailure(
               label, SynchronizationPointFailureReason.SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE)),
            owner, peer);
         JavaTckSupport.check(ownerRecorder.failures.contains(new RegistrationFailure(label,
               SynchronizationPointFailureReason.SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE)),
            "duplicate synchronization registration did not preserve its failure reason");

         // A default synchronization set is the current federation membership
         // and a member joining after registration receives the pending point.
         late.joinFederationExecution("java-tck-sync-late", federation);
         lateJoined = true;
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(!lateRecorder.announcements.contains(expectedAnnouncement),
               "evoked late-join synchronization callback ran before callback servicing");
         }
         awaitCallbacks(callbackModel,
            () -> lateRecorder.announcements.contains(expectedAnnouncement), late);
         JavaTckSupport.check(lateRecorder.announcements.contains(new Announcement(label, tag)),
            "late synchronization member did not receive the pending announcement");

         owner.synchronizationPointAchieved(label, true);
         peer.synchronizationPointAchieved(label, false);
         late.synchronizationPointAchieved(label, true);
         FederateHandleSet expectedFailure = set(owner, peerHandle);
         awaitCallbacks(callbackModel, () -> hasSynchronization(ownerRecorder, label,
               expectedFailure) && hasSynchronization(peerRecorder, label, expectedFailure)
               && hasSynchronization(lateRecorder, label, expectedFailure), owner, peer, late);
         JavaTckSupport.check(hasSynchronization(ownerRecorder, label, expectedFailure)
               && hasSynchronization(peerRecorder, label, expectedFailure)
               && hasSynchronization(lateRecorder, label, expectedFailure),
            "synchronization completion did not report the failed member set");

         String explicitLabel = "java-tck-sync-explicit-" + UUID.randomUUID();
         byte[] explicitTag = new byte[] {0x04, 0x05};
         FederateHandleSet invalidMembers = owner.getFederateHandleSetFactory().create();
         invalidMembers.add(JavaTckSupport.invalidHandle(FederateHandle.class));
         JavaTckSupport.expectFailure(() -> owner.registerFederationSynchronizationPoint(
               "java-tck-sync-invalid-explicit-" + UUID.randomUUID(), explicitTag,
               invalidMembers),
            "InvalidFederateHandle", "explicit synchronization set with an invalid handle");
         FederateHandleSet explicitMembers = owner.getFederateHandleSetFactory().create();
         explicitMembers.add(ownerHandle);
         explicitMembers.add(peerHandle);
         owner.registerFederationSynchronizationPoint(explicitLabel, explicitTag,
            explicitMembers);
         Announcement expectedExplicitAnnouncement = new Announcement(explicitLabel, explicitTag);
         awaitCallbacks(callbackModel, () -> ownerRecorder.announcements.contains(
               expectedExplicitAnnouncement) && peerRecorder.announcements.contains(
               expectedExplicitAnnouncement), owner, peer, late);
         JavaTckSupport.check(ownerRecorder.announcements.contains(
               new Announcement(explicitLabel, explicitTag))
               && peerRecorder.announcements.contains(new Announcement(explicitLabel, explicitTag)),
            "explicit synchronization members did not receive the announcement");
         int lateExplicitAnnouncements = countAnnouncements(lateRecorder, explicitLabel);
         JavaTckSupport.check(lateExplicitAnnouncements == 0,
            "excluded synchronization member received an explicit-set announcement");
         owner.synchronizationPointAchieved(explicitLabel);
         peer.synchronizationPointAchieved(explicitLabel);
         awaitCallbacks(callbackModel, () -> hasSynchronization(ownerRecorder, explicitLabel,
               emptySet(owner)) && hasSynchronization(peerRecorder, explicitLabel,
               emptySet(peer)), owner, peer, late);
         JavaTckSupport.check(hasSynchronization(ownerRecorder, explicitLabel,
               emptySet(owner)),
            "explicit synchronization barrier did not complete for the owner");
         JavaTckSupport.check(hasSynchronization(peerRecorder, explicitLabel,
               emptySet(peer)),
            "explicit synchronization barrier did not complete for the peer");

         JavaTckSupport.expectFailure(
            () -> owner.synchronizationPointAchieved("java-tck-sync-never-announced"),
            "SynchronizationPointLabelNotAnnounced",
            "achievement for an unknown synchronization label was accepted");

         peer.resignFederationExecution(ResignAction.NO_ACTION);
         peerJoined = false;
         String departedLabel = "java-tck-sync-departed-" + UUID.randomUUID();
         FederateHandleSet departedMembers = owner.getFederateHandleSetFactory().create();
         departedMembers.add(ownerHandle);
         departedMembers.add(peerHandle);
         owner.registerFederationSynchronizationPoint(departedLabel,
            new byte[] {0x06}, departedMembers);
         awaitCallbacks(callbackModel, () -> ownerRecorder.failures.contains(new RegistrationFailure(
               departedLabel, SynchronizationPointFailureReason.SYNCHRONIZATION_SET_MEMBER_NOT_JOINED)),
            owner, late);
         JavaTckSupport.check(ownerRecorder.failures.contains(new RegistrationFailure(departedLabel,
               SynchronizationPointFailureReason.SYNCHRONIZATION_SET_MEMBER_NOT_JOINED)),
            "explicit synchronization set did not reject the departed member");
      } finally {
         if (lateJoined) resign(late);
         if (peerJoined) resign(peer);
         if (ownerJoined) resign(owner);
         if (created) {
            try {
               owner.destroyFederationExecution(federation);
            } catch (Exception ignored) {
            }
         }
         if (lateConnected) disconnect(late);
         if (peerConnected) disconnect(peer);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static FederateHandleSet set(RTIambassador ambassador, FederateHandle failed)
         throws Exception {
      FederateHandleSet result = ambassador.getFederateHandleSetFactory().create();
      result.add(failed);
      return result;
   }

   private static FederateHandleSet emptySet(RTIambassador ambassador) throws Exception {
      return ambassador.getFederateHandleSetFactory().create();
   }

   static void runExplicitSetLateJoin(RtiFactory factory, String fom,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      RTIambassador registrar = factory.getRtiAmbassador();
      RTIambassador late = factory.getRtiAmbassador();
      Recorder registrarRecorder = new Recorder();
      Recorder lateRecorder = new Recorder();
      String federation = "java-tck-sync-explicit-late-" + UUID.randomUUID();
      boolean registrarConnected = false;
      boolean lateConnected = false;
      boolean created = false;
      boolean registrarJoined = false;
      boolean lateJoined = false;
      try {
         registrar.connect(registrarRecorder.proxy(), callbackModel);
         registrarConnected = true;
         late.connect(lateRecorder.proxy(), callbackModel);
         lateConnected = true;
         registrar.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle registrarHandle = registrar.joinFederationExecution(
            "java-tck-sync-explicit-registrar", federation);
         registrarJoined = true;

         String label = "java-tck-sync-explicit-late-" + UUID.randomUUID();
         byte[] tag = new byte[] {0x53, 0x45, 0x54};
         FederateHandleSet explicitSet = registrar.getFederateHandleSetFactory().create();
         explicitSet.add(registrarHandle);
         registrar.registerFederationSynchronizationPoint(label, tag, explicitSet);
         Announcement expectedAnnouncement = new Announcement(label, tag);
         awaitCallbacks(callbackModel, () -> registrarRecorder.registrations.contains(label)
               && registrarRecorder.announcements.contains(expectedAnnouncement), registrar);
         JavaTckSupport.check(registrarRecorder.registrations.contains(label)
               && registrarRecorder.announcements.contains(expectedAnnouncement),
            "explicit synchronization registration did not succeed for its sole member");

         late.joinFederationExecution("java-tck-sync-explicit-late-member", federation);
         lateJoined = true;
         JavaTckSupport.drain(late);
         JavaTckSupport.check(countAnnouncements(lateRecorder, label) == 0,
            "late joiner outside an explicit synchronization set received an announcement");
         JavaTckSupport.expectFailure(() -> late.synchronizationPointAchieved(label),
            "SynchronizationPointLabelNotAnnounced",
            "late joiner outside an explicit synchronization set achieved its label");

         registrar.synchronizationPointAchieved(label);
         awaitCallbacks(callbackModel,
            () -> hasSynchronization(registrarRecorder, label, emptySet(registrar)), registrar);
         JavaTckSupport.drain(late);
         JavaTckSupport.check(hasSynchronization(registrarRecorder, label, emptySet(registrar)),
            "single-member explicit synchronization point did not complete successfully");
         JavaTckSupport.check(countAnnouncements(lateRecorder, label) == 0
               && !hasSynchronizationLabel(lateRecorder, label),
            "excluded late joiner received explicit synchronization callbacks");
      } finally {
         if (lateJoined) resign(late);
         if (registrarJoined) resign(registrar);
         if (created) {
            try {
               registrar.destroyFederationExecution(federation);
            } catch (Exception ignored) {
            }
         }
         if (lateConnected) disconnect(late);
         if (registrarConnected) disconnect(registrar);
      }
   }

   private static boolean hasSynchronizationLabel(Recorder recorder, String label) {
      for (SynchronizationResult result : recorder.synchronizations) {
         if (label.equals(result.label)) return true;
      }
      return false;
   }

   private static boolean hasSynchronization(Recorder recorder, String label,
         FederateHandleSet failed) {
      for (SynchronizationResult result : recorder.synchronizations) {
         if (label.equals(result.label) && failed.equals(result.failed)) return true;
      }
      return false;
   }

   private static int countAnnouncements(Recorder recorder, String label) {
      int count = 0;
      for (Announcement announcement : recorder.announcements) {
         if (label.equals(announcement.label)) count++;
      }
      return count;
   }

   static void runCallbackControls(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, CallbackModel callbackModel) throws Exception {
      RTIambassador registrar = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      Recorder registrarRecorder = new Recorder();
      Recorder memberRecorder = new Recorder();
      String federation = "java-tck-callback-sync-" + UUID.randomUUID();
      boolean registrarConnected = false;
      boolean memberConnected = false;
      boolean created = false;
      boolean registrarJoined = false;
      boolean memberJoined = false;
      try {
         registrar.connect(registrarRecorder.proxy(), callbackModel);
         registrarConnected = true;
         member.connect(memberRecorder.proxy(), callbackModel);
         memberConnected = true;
         registrar.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         registrar.joinFederationExecution("java-tck-callback-sync-registrar", federation);
         registrarJoined = true;
         member.joinFederationExecution("java-tck-callback-sync-member", federation);
         memberJoined = true;

         String label = "java-tck-callback-sync-" + UUID.randomUUID();
         byte[] tag = new byte[] {0x53, 0x59, 0x4e, 0x43};
         Announcement expectedAnnouncement = new Announcement(label, tag);

         member.disableCallbacks();
         registrar.registerFederationSynchronizationPoint(label, tag);
         awaitCallbacks(callbackModel, () -> registrarRecorder.registrations.contains(label)
               && registrarRecorder.announcements.contains(expectedAnnouncement), registrar);
         JavaTckSupport.check(registrarRecorder.registrations.size() == 1
               && label.equals(registrarRecorder.registrations.get(0))
               && registrarRecorder.failures.isEmpty(),
            "synchronization registration did not produce exactly one successful result");
         JavaTckSupport.check(countAnnouncements(registrarRecorder, label) == 1,
            "registrar received an unexpected synchronization announcement count");
         serviceCallbackWork(member, callbackModel, objectClassName);
         JavaTckSupport.check(countAnnouncements(memberRecorder, label) == 0,
            "disabled callbacks exposed a synchronization announcement");

         member.enableCallbacks();
         serviceCallbackWork(member, callbackModel, objectClassName);
         awaitCallbacks(callbackModel,
            () -> memberRecorder.announcements.contains(expectedAnnouncement), member);
         JavaTckSupport.check(countAnnouncements(memberRecorder, label) == 1,
            "re-enabled callbacks did not release exactly one synchronization announcement");

         member.disableCallbacks();
         registrar.synchronizationPointAchieved(label, true);
         member.synchronizationPointAchieved(label, true);
         awaitCallbacks(callbackModel,
            () -> hasSynchronizationLabel(registrarRecorder, label), registrar);
         serviceCallbackWork(member, callbackModel, objectClassName);
         JavaTckSupport.check(!hasSynchronizationLabel(memberRecorder, label),
            "disabled callbacks exposed a federation-synchronized callback");

         member.enableCallbacks();
         serviceCallbackWork(member, callbackModel, objectClassName);
         awaitCallbacks(callbackModel,
            () -> hasSynchronizationLabel(memberRecorder, label), member);
         JavaTckSupport.check(registrarRecorder.synchronizations.size() == 1
               && memberRecorder.synchronizations.size() == 1
               && hasSynchronization(registrarRecorder, label, emptySet(registrar))
               && hasSynchronization(memberRecorder, label, emptySet(member)),
            "re-enabled synchronization completion returned the wrong label or failed set");
      } finally {
         if (memberJoined) resign(member);
         if (registrarJoined) resign(registrar);
         if (created) {
            try {
               registrar.destroyFederationExecution(federation);
            } catch (Exception ignored) {
            }
         }
         if (memberConnected) disconnect(member);
         if (registrarConnected) disconnect(registrar);
      }
   }

   private static void serviceCallbackWork(RTIambassador ambassador,
         CallbackModel callbackModel, String objectClassName) throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) {
         ambassador.evokeMultipleCallbacks(0.0, 1.0);
      } else {
         ambassador.getObjectClassHandle(objectClassName);
      }
   }

   private static void awaitCallbacks(CallbackModel callbackModel, Condition condition,
         RTIambassador... ambassadors) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            for (RTIambassador ambassador : ambassadors) ambassador.evokeCallback(0.0);
         }
         if (condition.isMet()) return;
         Thread.yield();
      }
      throw new AssertionError("synchronization callbacks did not arrive before deadline");
   }

   @FunctionalInterface
   private interface Condition {
      boolean isMet() throws Exception;
   }

   private static void resign(RTIambassador ambassador) {
      try {
         ambassador.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
      } catch (Exception ignored) {
      }
   }

   private static void disconnect(RTIambassador ambassador) {
      try {
         ambassador.disconnect();
      } catch (Exception ignored) {
      }
   }

   private static final class Announcement {
      private final String label;
      private final byte[] tag;

      private Announcement(String label, byte[] tag) {
         this.label = label;
         this.tag = tag.clone();
      }

      @Override
      public boolean equals(Object other) {
         if (!(other instanceof Announcement)) return false;
         Announcement that = (Announcement) other;
         return label.equals(that.label) && Arrays.equals(tag, that.tag);
      }

      @Override
      public int hashCode() {
         return 31 * label.hashCode() + Arrays.hashCode(tag);
      }
   }

   private static final class RegistrationFailure {
      private final String label;
      private final SynchronizationPointFailureReason reason;

      private RegistrationFailure(String label, SynchronizationPointFailureReason reason) {
         this.label = label;
         this.reason = reason;
      }

      @Override
      public boolean equals(Object other) {
         if (!(other instanceof RegistrationFailure)) return false;
         RegistrationFailure that = (RegistrationFailure) other;
         return label.equals(that.label) && reason == that.reason;
      }

      @Override
      public int hashCode() {
         return 31 * label.hashCode() + reason.hashCode();
      }
   }

   private static final class SynchronizationResult {
      private final String label;
      private final FederateHandleSet failed;

      private SynchronizationResult(String label, FederateHandleSet failed) {
         this.label = label;
         this.failed = failed;
      }
   }

   private static final class Recorder {
      private final List<String> registrations = new CopyOnWriteArrayList<>();
      private final List<Announcement> announcements = new CopyOnWriteArrayList<>();
      private final List<RegistrationFailure> failures = new CopyOnWriteArrayList<>();
      private final List<SynchronizationResult> synchronizations = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("synchronizationPointRegistrationSucceeded".equals(name)) {
                  registrations.add((String) arguments[0]);
               } else if ("synchronizationPointRegistrationFailed".equals(name)) {
                  failures.add(new RegistrationFailure((String) arguments[0],
                     (SynchronizationPointFailureReason) arguments[1]));
               } else if ("announceSynchronizationPoint".equals(name)) {
                  announcements.add(new Announcement((String) arguments[0],
                     ((byte[]) arguments[1]).clone()));
               } else if ("federationSynchronized".equals(name)) {
                  FederateHandleSet failed =
                     ((FederateHandleSet) arguments[1]).clone();
                  synchronizations.add(new SynchronizationResult((String) arguments[0], failed));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK synchronization callbacks";
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
