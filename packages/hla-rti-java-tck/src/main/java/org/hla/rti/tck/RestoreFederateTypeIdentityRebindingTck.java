package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.atomic.AtomicInteger;

/** Rebinds saved federate identities to same-type replacement members. */
final class RestoreFederateTypeIdentityRebindingTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private RestoreFederateTypeIdentityRebindingTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String federateType, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(fom != null && !fom.isEmpty()
            && timeImplementation != null && !timeImplementation.isEmpty()
            && federateType != null && !federateType.isEmpty(),
         "restore identity rebinding requires adapter FOM, time implementation, and federate type");

      String federation = "java-tck-restore-type-identity-rebinding-" + UUID.randomUUID();
      String ownerName = "java-tck-restore-rebinding-owner";
      String peerName = "java-tck-restore-rebinding-peer";
      String replacementOwnerName = ownerName + "-same-type-replacement";
      String replacementPeerName = peerName + "-same-type-replacement";
      String saveLabel = "java-tck-restore-type-identity-rebinding";

      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador peer = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder peerRecorder = new Recorder();
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
         owner.joinFederationExecution(ownerName, federateType, federation);
         ownerJoined = true;
         peer.joinFederationExecution(peerName, federateType, federation);
         peerJoined = true;

         owner.requestFederationSave(saveLabel);
         await(owner, peer, callbackModel,
            () -> ownerRecorder.saveInitiations.size() == 1
               && peerRecorder.saveInitiations.size() == 1,
            "same-type identity-rebinding save initiation");
         JavaTckSupport.check(ownerRecorder.saveInitiations.equals(Arrays.asList(saveLabel))
               && peerRecorder.saveInitiations.equals(Arrays.asList(saveLabel)),
            "same-type save did not initiate both original federates with the requested label");
         owner.federateSaveBegun();
         peer.federateSaveBegun();
         owner.federateSaveComplete();
         peer.federateSaveComplete();
         await(owner, peer, callbackModel,
            () -> ownerRecorder.savedCount.get() == 1 && peerRecorder.savedCount.get() == 1,
            "same-type identity-rebinding save completion");
         JavaTckSupport.check(ownerRecorder.saveFailures.isEmpty()
               && peerRecorder.saveFailures.isEmpty(),
            "same-type identity-rebinding save reported failure");

         owner.resignFederationExecution(ResignAction.NO_ACTION);
         ownerJoined = false;
         peer.resignFederationExecution(ResignAction.NO_ACTION);
         peerJoined = false;
         owner.joinFederationExecution(replacementOwnerName, federateType, federation);
         ownerJoined = true;
         peer.joinFederationExecution(replacementPeerName, federateType, federation);
         peerJoined = true;

         FederateHandle replacementOwnerHandle = owner.getFederateHandle(replacementOwnerName);
         FederateHandle replacementPeerHandle = owner.getFederateHandle(replacementPeerName);
         JavaTckSupport.check(replacementOwnerHandle != null && replacementPeerHandle != null
               && !replacementOwnerHandle.equals(replacementPeerHandle),
            "same-type replacement joins did not produce two distinct members");

         owner.requestFederationRestore(saveLabel);
         await(owner, peer, callbackModel,
            () -> ownerRecorder.restoreSuccesses.size() == 1
               && ownerRecorder.restoreBegunCount.get() == 1
               && peerRecorder.restoreBegunCount.get() == 1
               && ownerRecorder.restoreInitiations.size() == 1
               && peerRecorder.restoreInitiations.size() == 1,
            "same-type identity-rebinding restore initiation");

         RestoreInitiation ownerRestore = ownerRecorder.restoreInitiations.get(0);
         RestoreInitiation peerRestore = peerRecorder.restoreInitiations.get(0);
         JavaTckSupport.check(ownerRecorder.restoreSuccesses.equals(Arrays.asList(saveLabel))
               && ownerRestore.matches(saveLabel, ownerName, replacementOwnerHandle)
               && peerRestore.matches(saveLabel, peerName, replacementPeerHandle),
            "same-type restore did not return saved identities with replacement handles");
         JavaTckSupport.check(ownerRecorder.restoreRequestFailures.isEmpty()
               && peerRecorder.restoreRequestFailures.isEmpty(),
            "same-type identity-rebinding restore unexpectedly failed");

         owner.federateRestoreComplete();
         peer.federateRestoreComplete();
         await(owner, peer, callbackModel,
            () -> ownerRecorder.restoredCount.get() == 1
               && peerRecorder.restoredCount.get() == 1,
            "same-type identity-rebinding restore completion");
         JavaTckSupport.check(ownerRecorder.restoreFailures.isEmpty()
               && peerRecorder.restoreFailures.isEmpty(),
            "same-type identity-rebinding restore reported a failure");
      } finally {
         if (ownerJoined) resign(owner);
         if (peerJoined) resign(peer);
         if (created) destroy(owner, federation);
         if (peerConnected) disconnect(peer);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static void await(RTIambassador first, RTIambassador second,
         CallbackModel callbackModel, Ready ready, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !ready.isReady()) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(first);
            JavaTckSupport.drain(second);
         } else {
            Thread.sleep(1L);
         }
      }
      JavaTckSupport.check(ready.isReady(), "timed out waiting for " + description);
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
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   @FunctionalInterface
   private interface Ready { boolean isReady(); }

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

      private boolean matches(String expectedLabel, String expectedName,
            FederateHandle expectedHandle) {
         return expectedLabel.equals(label) && expectedName.equals(federateName)
            && expectedHandle.equals(postRestoreHandle);
      }
   }

   private static final class Recorder {
      private final List<String> saveInitiations = new CopyOnWriteArrayList<>();
      private final List<Object> saveFailures = new CopyOnWriteArrayList<>();
      private final List<String> restoreSuccesses = new CopyOnWriteArrayList<>();
      private final List<String> restoreRequestFailures = new CopyOnWriteArrayList<>();
      private final List<RestoreInitiation> restoreInitiations = new CopyOnWriteArrayList<>();
      private final List<Object> restoreFailures = new CopyOnWriteArrayList<>();
      private final AtomicInteger savedCount = new AtomicInteger();
      private final AtomicInteger restoreBegunCount = new AtomicInteger();
      private final AtomicInteger restoredCount = new AtomicInteger();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("initiateFederateSave".equals(name)) {
                  saveInitiations.add((String) arguments[0]);
               } else if ("federationSaved".equals(name)) {
                  savedCount.incrementAndGet();
               } else if ("federationNotSaved".equals(name)) {
                  saveFailures.add(arguments[0]);
               } else if ("requestFederationRestoreSucceeded".equals(name)) {
                  restoreSuccesses.add((String) arguments[0]);
               } else if ("requestFederationRestoreFailed".equals(name)) {
                  restoreRequestFailures.add((String) arguments[0]);
               } else if ("federationRestoreBegun".equals(name)) {
                  restoreBegunCount.incrementAndGet();
               } else if ("initiateFederateRestore".equals(name)) {
                  restoreInitiations.add(new RestoreInitiation((String) arguments[0],
                     (String) arguments[1], (FederateHandle) arguments[2]));
               } else if ("federationRestored".equals(name)) {
                  restoredCount.incrementAndGet();
               } else if ("federationNotRestored".equals(name)) {
                  restoreFailures.add(arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK restore identity recorder";
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
