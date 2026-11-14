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

/** Rejects restore after a saved federation's per-type member counts change. */
final class RestoreFederateTypeCountMismatchTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private RestoreFederateTypeCountMismatchTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String federateType, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(fom != null && !fom.isEmpty()
            && timeImplementation != null && !timeImplementation.isEmpty()
            && federateType != null && !federateType.isEmpty(),
         "restore type-count mismatch requires adapter FOM, time implementation, and federate type");

      String federation = "java-tck-restore-type-count-mismatch-" + UUID.randomUUID();
      String ownerName = "java-tck-restore-type-count-owner";
      String peerName = "java-tck-restore-type-peer";
      String mismatchType = federateType + "-restore-mismatch";
      String saveLabel = "java-tck-restore-type-count-mismatch";

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

         FederateHandle originalOwnerHandle = owner.getFederateHandle(ownerName);
         FederateHandle peerHandle = owner.getFederateHandle(peerName);
         JavaTckSupport.check(originalOwnerHandle != null && peerHandle != null
               && !originalOwnerHandle.equals(peerHandle),
            "same-type save fixture did not contain two distinct joined federates");

         owner.requestFederationSave(saveLabel);
         await(owner, peer, callbackModel,
            () -> ownerRecorder.saveInitiations.size() == 1
               && peerRecorder.saveInitiations.size() == 1,
            "same-type federation save initiation");
         JavaTckSupport.check(ownerRecorder.saveInitiations.equals(Arrays.asList(saveLabel))
               && peerRecorder.saveInitiations.equals(Arrays.asList(saveLabel)),
            "same-type save did not initiate both joined federates with the requested label");
         owner.federateSaveBegun();
         peer.federateSaveBegun();
         owner.federateSaveComplete();
         peer.federateSaveComplete();
         await(owner, peer, callbackModel,
            () -> ownerRecorder.savedCount.get() == 1 && peerRecorder.savedCount.get() == 1,
            "same-type federation save completion");
         JavaTckSupport.check(ownerRecorder.saveFailures.isEmpty()
               && peerRecorder.saveFailures.isEmpty(),
            "same-type federation save reported failure");

         owner.resignFederationExecution(ResignAction.NO_ACTION);
         ownerJoined = false;
         owner.joinFederationExecution(ownerName, mismatchType, federation);
         ownerJoined = true;
         FederateHandle replacementOwnerHandle = owner.getFederateHandle(ownerName);
         peerHandle = owner.getFederateHandle(peerName);
         JavaTckSupport.check(replacementOwnerHandle != null && peerHandle != null
               && !replacementOwnerHandle.equals(peerHandle),
            "type-count mismatch fixture did not retain two distinct joined federates");

         peer.requestFederationRestore(saveLabel);
         await(owner, peer, callbackModel,
            () -> peerRecorder.restoreRequestFailures.size() == 1,
            "restore rejection for a changed per-type membership count");
         JavaTckSupport.check(peerRecorder.restoreRequestFailures.equals(Arrays.asList(saveLabel)),
            "restore failure did not identify the save whose federate-type counts changed");
         JavaTckSupport.check(ownerRecorder.restoreRequestFailures.isEmpty()
               && ownerRecorder.restoreSuccesses.isEmpty()
               && peerRecorder.restoreSuccesses.isEmpty()
               && ownerRecorder.restoreBegunCount.get() == 0
               && peerRecorder.restoreBegunCount.get() == 0
               && ownerRecorder.restoreInitiationCount.get() == 0
               && peerRecorder.restoreInitiationCount.get() == 0,
            "restore began despite a changed federate-type membership count");
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

   private static final class Recorder {
      private final List<String> saveInitiations = new CopyOnWriteArrayList<>();
      private final List<Object> saveFailures = new CopyOnWriteArrayList<>();
      private final List<String> restoreSuccesses = new CopyOnWriteArrayList<>();
      private final List<String> restoreRequestFailures = new CopyOnWriteArrayList<>();
      private final AtomicInteger savedCount = new AtomicInteger();
      private final AtomicInteger restoreBegunCount = new AtomicInteger();
      private final AtomicInteger restoreInitiationCount = new AtomicInteger();

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
                  restoreInitiationCount.incrementAndGet();
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK restore type-count recorder";
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
