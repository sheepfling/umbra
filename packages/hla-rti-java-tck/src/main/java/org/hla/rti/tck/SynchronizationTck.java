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
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;

/** P2.7 multi-member synchronization registration and barrier behavior. */
final class SynchronizationTck {
   private SynchronizationTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation) throws Exception {
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
         owner.connect(ownerRecorder.proxy(), CallbackModel.HLA_EVOKED);
         ownerConnected = true;
         peer.connect(peerRecorder.proxy(), CallbackModel.HLA_EVOKED);
         peerConnected = true;
         late.connect(lateRecorder.proxy(), CallbackModel.HLA_EVOKED);
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
         owner.registerFederationSynchronizationPoint(label, tag);
         drain(owner, peer);
         JavaTckSupport.check(ownerRecorder.registrations.contains(label),
            "synchronization registration did not succeed for the registering member");
         JavaTckSupport.check(ownerRecorder.announcements.contains(new Announcement(label, tag))
               && peerRecorder.announcements.contains(new Announcement(label, tag)),
            "current synchronization members did not receive the announcement");
         JavaTckSupport.check(peerRecorder.registrations.isEmpty(),
            "non-registering member received a registration-success callback");

         owner.registerFederationSynchronizationPoint(label, new byte[] {0x09});
         drain(owner, peer);
         JavaTckSupport.check(ownerRecorder.failures.contains(new RegistrationFailure(label,
               SynchronizationPointFailureReason.SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE)),
            "duplicate synchronization registration did not preserve its failure reason");

         // A default synchronization set is the current federation membership
         // and a member joining after registration receives the pending point.
         late.joinFederationExecution("java-tck-sync-late", federation);
         lateJoined = true;
         drain(late);
         JavaTckSupport.check(lateRecorder.announcements.contains(new Announcement(label, tag)),
            "late synchronization member did not receive the pending announcement");

         owner.synchronizationPointAchieved(label, true);
         peer.synchronizationPointAchieved(label, false);
         late.synchronizationPointAchieved(label, true);
         drain(owner, peer, late);
         FederateHandleSet expectedFailure = set(owner, peerHandle);
         JavaTckSupport.check(hasSynchronization(ownerRecorder, label, expectedFailure)
               && hasSynchronization(peerRecorder, label, expectedFailure)
               && hasSynchronization(lateRecorder, label, expectedFailure),
            "synchronization completion did not report the failed member set");

         String explicitLabel = "java-tck-sync-explicit-" + UUID.randomUUID();
         byte[] explicitTag = new byte[] {0x04, 0x05};
         FederateHandleSet explicitMembers = owner.getFederateHandleSetFactory().create();
         explicitMembers.add(ownerHandle);
         explicitMembers.add(peerHandle);
         owner.registerFederationSynchronizationPoint(explicitLabel, explicitTag,
            explicitMembers);
         drain(owner, peer, late);
         JavaTckSupport.check(ownerRecorder.announcements.contains(
               new Announcement(explicitLabel, explicitTag))
               && peerRecorder.announcements.contains(new Announcement(explicitLabel, explicitTag)),
            "explicit synchronization members did not receive the announcement");
         int lateExplicitAnnouncements = countAnnouncements(lateRecorder, explicitLabel);
         JavaTckSupport.check(lateExplicitAnnouncements == 0,
            "excluded synchronization member received an explicit-set announcement");
         owner.synchronizationPointAchieved(explicitLabel);
         peer.synchronizationPointAchieved(explicitLabel);
         drain(owner, peer, late);
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
         drain(owner, late);
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

   private static void drain(RTIambassador... ambassadors) throws Exception {
      for (int round = 0; round < 64; ++round) {
         for (RTIambassador ambassador : ambassadors) ambassador.evokeCallback(0.0);
      }
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
      private final List<String> registrations = new ArrayList<>();
      private final List<Announcement> announcements = new ArrayList<>();
      private final List<RegistrationFailure> failures = new ArrayList<>();
      private final List<SynchronizationResult> synchronizations = new ArrayList<>();

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
