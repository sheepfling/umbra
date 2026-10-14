package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.FederateHandleSaveStatusPair;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.SaveStatus;
import java.lang.reflect.Proxy;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verify the baseline federation save-status response for every joined federate. */
final class FederationMomSaveStatusTck {
   private FederationMomSaveStatusTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador peer = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder peerRecorder = new Recorder();
      String federation = "java-tck-federation-mom-save-status-" + UUID.randomUUID();
      String ownerName = "java-tck-save-status-owner";
      String peerName = "java-tck-save-status-peer";
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

         owner.queryFederationSaveStatus();
         await(owner, callbackModel, objectClassName,
            () -> !ownerRecorder.responses.isEmpty(), "federation save-status callback");
         FederateHandleSaveStatusPair[] statuses = ownerRecorder.responses.get(0);
         JavaTckSupport.check(statuses.length == 2,
            "federation save-status response omitted a joined federate");
         checkStatus(statuses, ownerHandle, "owner");
         checkStatus(statuses, peerHandle, "peer");
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

   private static void checkStatus(FederateHandleSaveStatusPair[] statuses,
         FederateHandle handle, String description) {
      int matches = 0;
      for (FederateHandleSaveStatusPair entry : statuses) {
         if (handle.equals(entry.handle)) {
            matches++;
            JavaTckSupport.check(entry.status == SaveStatus.NO_SAVE_IN_PROGRESS,
               "federation save-status " + description + " returned the wrong status");
         }
      }
      JavaTckSupport.check(matches == 1,
         "federation save-status response omitted or duplicated the " + description);
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         String objectClassName, Condition condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && !condition.isTrue()) {
         if (model == CallbackModel.HLA_EVOKED) ambassador.evokeMultipleCallbacks(0.0, 1.0);
         else ambassador.getObjectClassHandle(objectClassName);
         Thread.yield();
      }
      JavaTckSupport.check(condition.isTrue(), "timed out waiting for " + description);
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

   private static final class Recorder {
      private final CopyOnWriteArrayList<FederateHandleSaveStatusPair[]> responses =
         new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("federationSaveStatusResponse".equals(name)) {
                  responses.add(((FederateHandleSaveStatusPair[]) arguments[0]).clone());
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK federation save-status recorder";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
