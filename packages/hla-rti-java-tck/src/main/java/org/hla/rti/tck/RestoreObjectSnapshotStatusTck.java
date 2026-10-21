package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.FederateRestoreStatus;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RestoreStatus;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Restore a two-member object snapshot and inspect active and idle status descriptors. */
final class RestoreObjectSnapshotStatusTck {
   private RestoreObjectSnapshotStatusTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel model) throws Exception {
      JavaTckSupport.check(model == CallbackModel.HLA_IMMEDIATE,
         "object snapshot restore has native evidence only for immediate callbacks");
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador peer = factory.getRtiAmbassador();
      Recorder ownerEvents = new Recorder();
      Recorder peerEvents = new Recorder();
      String federation = "java-tck-restore-object-snapshot-" + UUID.randomUUID();
      String saveLabel = "java-tck-object-baseline";
      String ownerName = "java-tck-object-snapshot-owner";
      String peerName = "java-tck-object-snapshot-peer";
      boolean ownerConnected = false, peerConnected = false, created = false;
      boolean ownerJoined = false, peerJoined = false;
      try {
         owner.connect(ownerEvents.proxy(), model);
         ownerConnected = true;
         peer.connect(peerEvents.proxy(), model);
         peerConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle ownerHandle = owner.joinFederationExecution(
            ownerName, "object-snapshot-owner", federation);
         ownerJoined = true;
         FederateHandle peerHandle = peer.joinFederationExecution(
            peerName, "object-snapshot-peer", federation);
         peerJoined = true;

         owner.requestFederationSave(saveLabel);
         JavaTckSupport.check(ownerEvents.saveLabels.equals(List.of(saveLabel))
               && peerEvents.saveLabels.equals(List.of(saveLabel)),
            "immediate baseline save did not initiate at both members");
         owner.federateSaveBegun();
         peer.federateSaveBegun();
         owner.federateSaveComplete();
         peer.federateSaveComplete();
         JavaTckSupport.check(ownerEvents.savedCount == 1 && peerEvents.savedCount == 1
               && ownerEvents.notSavedCount == 0 && peerEvents.notSavedCount == 0,
            "baseline save did not finish successfully");

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         owner.publishObjectClassAttributes(
            ownerClass, JavaTckSupport.attributeSet(owner, ownerAttribute));
         ObjectClassHandle peerClass = peer.getObjectClassHandle(objectClassName);
         AttributeHandle peerAttribute = peer.getAttributeHandle(peerClass, attributeName);
         peer.subscribeObjectClassAttributes(
            peerClass, JavaTckSupport.attributeSet(peer, peerAttribute));
         ObjectInstanceHandle mutated = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(peerEvents.discoveries.contains(mutated),
            "post-save object was not discovered directly");

         owner.requestFederationRestore(saveLabel);
         JavaTckSupport.check(ownerEvents.restoreSuccesses.equals(List.of(saveLabel))
               && ownerEvents.restoreBegunCount == 1
               && peerEvents.restoreBegunCount == 1
               && ownerEvents.restoreInitiations.size() == 1
               && peerEvents.restoreInitiations.size() == 1,
            "immediate restore control callbacks did not reach both members");
         checkInitiation(ownerEvents.restoreInitiations.get(0),
            saveLabel, ownerName, ownerHandle);
         checkInitiation(peerEvents.restoreInitiations.get(0),
            saveLabel, peerName, peerHandle);

         owner.queryFederationRestoreStatus();
         JavaTckSupport.check(ownerEvents.restoreStatuses.size() == 1,
            "restoring-status response was not delivered directly");
         FederateRestoreStatus[] restoring = ownerEvents.restoreStatuses.get(0);
         JavaTckSupport.check(restoring.length == 2,
            "restoring status omitted a joined member");
         checkStatus(restoring, ownerHandle, ownerHandle, RestoreStatus.FEDERATE_RESTORING);
         checkStatus(restoring, peerHandle, peerHandle, RestoreStatus.FEDERATE_RESTORING);

         owner.federateRestoreComplete();
         JavaTckSupport.check(ownerEvents.restoredCount == 0
               && peerEvents.restoredCount == 0,
            "restore completed before the final member");
         peer.federateRestoreComplete();
         JavaTckSupport.check(ownerEvents.restoredCount == 1
               && peerEvents.restoredCount == 1
               && ownerEvents.notRestoredCount == 0
               && peerEvents.notRestoredCount == 0,
            "object snapshot restore did not finish successfully");
         JavaTckSupport.expectFailure(() -> owner.getKnownObjectClassHandle(mutated),
            "ObjectInstanceNotKnown", "post-save object after baseline restore");

         owner.queryFederationRestoreStatus();
         JavaTckSupport.check(ownerEvents.restoreStatuses.size() == 2,
            "idle restore-status response was not delivered directly");
         FederateRestoreStatus[] idle = ownerEvents.restoreStatuses.get(1);
         JavaTckSupport.check(idle.length == 2, "idle status omitted a joined member");
         checkStatus(idle, ownerHandle, null, RestoreStatus.NO_RESTORE_IN_PROGRESS);
         checkStatus(idle, peerHandle, null, RestoreStatus.NO_RESTORE_IN_PROGRESS);

         peer.resignFederationExecution(ResignAction.NO_ACTION);
         peerJoined = false;
         owner.resignFederationExecution(ResignAction.NO_ACTION);
         ownerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (peerJoined) try { peer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (ownerJoined) try { owner.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (created) try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (peerConnected) try { peer.disconnect(); } catch (Exception ignored) { }
         if (ownerConnected) try { owner.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static void checkInitiation(Initiation actual, String label,
         String name, FederateHandle handle) {
      JavaTckSupport.check(label.equals(actual.label) && name.equals(actual.name)
            && handle.equals(actual.postHandle),
         "restore initiation lost participant name or post-restore handle");
   }

   private static void checkStatus(FederateRestoreStatus[] statuses,
         FederateHandle pre, FederateHandle post, RestoreStatus expected) {
      int matches = 0;
      for (FederateRestoreStatus status : statuses) {
         if (!pre.equals(status.preRestoreHandle)) continue;
         matches++;
         JavaTckSupport.check((post == null ? status.postRestoreHandle == null
               : post.equals(status.postRestoreHandle)) && status.status == expected,
            "restore status descriptor had the wrong post handle or state");
      }
      JavaTckSupport.check(matches == 1,
         "restore status omitted or duplicated a joined member");
   }

   private static final class Initiation {
      final String label;
      final String name;
      final FederateHandle postHandle;
      Initiation(String label, String name, FederateHandle postHandle) {
         this.label = label;
         this.name = name;
         this.postHandle = postHandle;
      }
   }

   private static final class Recorder {
      final List<String> saveLabels = new CopyOnWriteArrayList<>();
      final List<String> restoreSuccesses = new CopyOnWriteArrayList<>();
      final List<Initiation> restoreInitiations = new CopyOnWriteArrayList<>();
      final List<FederateRestoreStatus[]> restoreStatuses = new CopyOnWriteArrayList<>();
      final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      volatile int savedCount, notSavedCount, restoreBegunCount;
      volatile int restoredCount, notRestoredCount;

      FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class}, (proxy, method, args) -> {
               String name = method.getName();
               if ("initiateFederateSave".equals(name)) saveLabels.add((String) args[0]);
               else if ("federationSaved".equals(name)) savedCount++;
               else if ("federationNotSaved".equals(name)) notSavedCount++;
               else if ("discoverObjectInstance".equals(name))
                  discoveries.add((ObjectInstanceHandle) args[0]);
               else if ("requestFederationRestoreSucceeded".equals(name))
                  restoreSuccesses.add((String) args[0]);
               else if ("federationRestoreBegun".equals(name)) restoreBegunCount++;
               else if ("initiateFederateRestore".equals(name))
                  restoreInitiations.add(new Initiation((String) args[0],
                     (String) args[1], (FederateHandle) args[2]));
               else if ("federationRestored".equals(name)) restoredCount++;
               else if ("federationNotRestored".equals(name)) notRestoredCount++;
               else if ("federationRestoreStatusResponse".equals(name))
                  restoreStatuses.add(((FederateRestoreStatus[]) args[0]).clone());
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK object-snapshot recorder";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (args == null ? null : args[0]);
               }
               return null;
            });
      }
   }
}
