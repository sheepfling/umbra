package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.FederateHandleSaveStatusPair;
import hla.rti1516_2025.FederateRestoreStatus;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RestoreStatus;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.SaveStatus;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

/** P2 callback-control checks for the standard federation save/restore lifecycle. */
final class CallbackControlsSaveRestoreTck {
   private CallbackControlsSaveRestoreTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder memberRecorder = new Recorder();
      String federation = "java-tck-callback-save-restore-" + UUID.randomUUID();
      String label = "java-tck-callback-save-restore";
      boolean ownerConnected = false;
      boolean memberConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean memberJoined = false;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         member.connect(memberRecorder.proxy(), callbackModel);
         memberConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle ownerHandle = owner.joinFederationExecution(
            "java-tck-save-restore-owner", federation);
         ownerJoined = true;
         FederateHandle memberHandle = member.joinFederationExecution(
            "java-tck-save-restore-member", federation);
         memberJoined = true;

         member.disableCallbacks();
         owner.requestFederationSave(label);
         await(callbackModel, () -> !ownerRecorder.saveInitiations.isEmpty(), owner,
            objectClassName);
         JavaTckSupport.check(label.equals(last(ownerRecorder.saveInitiations))
               && ownerRecorder.saveTimes.isEmpty(),
            "owner received incorrect untimed save initiation metadata");
         service(member, callbackModel, objectClassName);
         JavaTckSupport.check(memberRecorder.saveInitiations.isEmpty(),
            "disabled callbacks exposed a save initiation");
         member.enableCallbacks();
         await(callbackModel, () -> !memberRecorder.saveInitiations.isEmpty(), member,
            objectClassName);
         JavaTckSupport.check(label.equals(last(memberRecorder.saveInitiations))
               && memberRecorder.saveTimes.isEmpty(),
            "re-enabled member received incorrect untimed save initiation metadata");

         owner.federateSaveBegun();
         member.federateSaveBegun();
         int saveStatusBefore = ownerRecorder.saveStatuses.size();
         owner.disableCallbacks();
         owner.queryFederationSaveStatus();
         service(owner, callbackModel, objectClassName);
         JavaTckSupport.check(ownerRecorder.saveStatuses.size() == saveStatusBefore,
            "disabled callbacks exposed a save-status response");
         owner.enableCallbacks();
         await(callbackModel, () -> ownerRecorder.saveStatuses.size() > saveStatusBefore,
            owner, objectClassName);
         FederateHandleSaveStatusPair[] saveStatuses =
            ownerRecorder.saveStatuses.get(ownerRecorder.saveStatuses.size() - 1);
         JavaTckSupport.check(saveStatuses.length == 2,
            "save-status response omitted a joined federate");
         checkSaveStatus(saveStatuses, ownerHandle);
         checkSaveStatus(saveStatuses, memberHandle);

         member.disableCallbacks();
         owner.federateSaveComplete();
         member.federateSaveComplete();
         await(callbackModel, () -> ownerRecorder.savedCount > 0, owner, objectClassName);
         service(member, callbackModel, objectClassName);
         JavaTckSupport.check(memberRecorder.savedCount == 0,
            "disabled callbacks exposed federationSaved");
         member.enableCallbacks();
         await(callbackModel, () -> memberRecorder.savedCount > 0, member, objectClassName);
         JavaTckSupport.check(ownerRecorder.saveFailures.isEmpty()
               && memberRecorder.saveFailures.isEmpty(),
            "successful save reported a failure");

         int ownerRestoreSuccessBefore = ownerRecorder.restoreSuccesses.size();
         int ownerRestoreBegunBefore = ownerRecorder.restoreBegunCount;
         int ownerRestoreInitiationBefore = ownerRecorder.restoreInitiations.size();
         int memberRestoreBegunBefore = memberRecorder.restoreBegunCount;
         int memberRestoreInitiationBefore = memberRecorder.restoreInitiations.size();
         member.disableCallbacks();
         owner.requestFederationRestore(label);
         await(callbackModel, () -> ownerRecorder.restoreSuccesses.size() > ownerRestoreSuccessBefore
               && ownerRecorder.restoreBegunCount > ownerRestoreBegunBefore
               && ownerRecorder.restoreInitiations.size() > ownerRestoreInitiationBefore,
            owner, objectClassName);
         JavaTckSupport.check(label.equals(last(ownerRecorder.restoreSuccesses))
               && ownerRecorder.restoreFailures.isEmpty(),
            "restore request returned an incorrect owner result");
         service(member, callbackModel, objectClassName);
         JavaTckSupport.check(memberRecorder.restoreBegunCount == memberRestoreBegunBefore
               && memberRecorder.restoreInitiations.size() == memberRestoreInitiationBefore
               && memberRecorder.restoreSuccesses.isEmpty(),
            "disabled callbacks exposed restore initiation callbacks");
         member.enableCallbacks();
         await(callbackModel, () -> memberRecorder.restoreBegunCount > memberRestoreBegunBefore
               && memberRecorder.restoreInitiations.size() > memberRestoreInitiationBefore,
            member, objectClassName);
         checkRestore(ownerRecorder.restoreInitiations.get(
            ownerRecorder.restoreInitiations.size() - 1), label, "java-tck-save-restore-owner");
         checkRestore(memberRecorder.restoreInitiations.get(
            memberRecorder.restoreInitiations.size() - 1), label, "java-tck-save-restore-member");
         JavaTckSupport.check(memberRecorder.restoreFailures.isEmpty(),
            "member restore request reported a failure");

         int restoreStatusBefore = ownerRecorder.restoreStatuses.size();
         owner.disableCallbacks();
         owner.queryFederationRestoreStatus();
         service(owner, callbackModel, objectClassName);
         JavaTckSupport.check(ownerRecorder.restoreStatuses.size() == restoreStatusBefore,
            "disabled callbacks exposed a restore-status response");
         owner.enableCallbacks();
         await(callbackModel, () -> ownerRecorder.restoreStatuses.size() > restoreStatusBefore,
            owner, objectClassName);
         FederateRestoreStatus[] restoreStatuses =
            ownerRecorder.restoreStatuses.get(ownerRecorder.restoreStatuses.size() - 1);
         JavaTckSupport.check(restoreStatuses.length == 2,
            "restore-status response omitted a joined federate");
         for (FederateRestoreStatus status : restoreStatuses) {
            JavaTckSupport.check(status.status == RestoreStatus.FEDERATE_RESTORING,
               "restore-status response returned the wrong state");
         }

         int ownerRestoredBefore = ownerRecorder.restoredCount;
         int memberRestoredBefore = memberRecorder.restoredCount;
         member.disableCallbacks();
         owner.federateRestoreComplete();
         member.federateRestoreComplete();
         await(callbackModel, () -> ownerRecorder.restoredCount > ownerRestoredBefore,
            owner, objectClassName);
         service(member, callbackModel, objectClassName);
         JavaTckSupport.check(memberRecorder.restoredCount == memberRestoredBefore,
            "disabled callbacks exposed federationRestored");
         member.enableCallbacks();
         await(callbackModel, () -> memberRecorder.restoredCount > memberRestoredBefore,
            member, objectClassName);
         JavaTckSupport.check(ownerRecorder.restoreFailures.isEmpty()
               && memberRecorder.restoreFailures.isEmpty(),
            "successful restore reported a failure");
      } finally {
         if (memberJoined) resign(member);
         if (ownerJoined) resign(owner);
         if (created) {
            try { owner.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (memberConnected) disconnect(member);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static void checkSaveStatus(FederateHandleSaveStatusPair[] statuses,
         FederateHandle handle) {
      for (FederateHandleSaveStatusPair status : statuses) {
         if (handle.equals(status.handle)) {
            JavaTckSupport.check(status.status == SaveStatus.FEDERATE_SAVING,
               "save-status response returned the wrong member state");
            return;
         }
      }
      throw new AssertionError("save-status response omitted a joined federate");
   }

   private static void checkRestore(RestoreInitiation initiation, String label,
         String expectedFederateName) {
      JavaTckSupport.check(label.equals(initiation.label)
            && expectedFederateName.equals(initiation.federateName)
            && initiation.postRestoreHandle != null,
         "restore initiation returned incorrect label, name, or federate handle");
   }

   private static <T> T last(List<T> values) {
      return values.get(values.size() - 1);
   }

   private static void service(RTIambassador ambassador, CallbackModel model,
         String objectClassName) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) ambassador.evokeMultipleCallbacks(0.0, 1.0);
      else ambassador.getObjectClassHandle(objectClassName);
   }

   private static void await(CallbackModel model, Condition condition, RTIambassador ambassador,
         String objectClassName) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.0);
         else ambassador.getObjectClassHandle(objectClassName);
         if (condition.isMet()) return;
         Thread.yield();
      }
      throw new AssertionError("save/restore callback did not arrive before deadline");
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

   private static final class RestoreInitiation {
      private final String label;
      private final String federateName;
      private final FederateHandle postRestoreHandle;

      private RestoreInitiation(String label, String federateName, FederateHandle handle) {
         this.label = label;
         this.federateName = federateName;
         this.postRestoreHandle = handle;
      }
   }

   private static final class Recorder {
      private final List<String> saveInitiations = new ArrayList<>();
      private final List<Object> saveTimes = new ArrayList<>();
      private final List<Object> saveFailures = new ArrayList<>();
      private final List<FederateHandleSaveStatusPair[]> saveStatuses = new ArrayList<>();
      private final List<String> restoreSuccesses = new ArrayList<>();
      private final List<String> restoreFailures = new ArrayList<>();
      private final List<RestoreInitiation> restoreInitiations = new ArrayList<>();
      private final List<FederateRestoreStatus[]> restoreStatuses = new ArrayList<>();
      private int restoreBegunCount;
      private int savedCount;
      private int restoredCount;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("initiateFederateSave".equals(name)) {
                  saveInitiations.add((String) arguments[0]);
                  if (arguments.length > 1) saveTimes.add(arguments[1]);
               }
               if ("federationSaved".equals(name)) savedCount++;
               if ("federationNotSaved".equals(name)) saveFailures.add(arguments[0]);
               if ("federationSaveStatusResponse".equals(name)) {
                  saveStatuses.add(((FederateHandleSaveStatusPair[]) arguments[0]).clone());
               }
               if ("requestFederationRestoreSucceeded".equals(name)) {
                  restoreSuccesses.add((String) arguments[0]);
               }
               if ("requestFederationRestoreFailed".equals(name)) {
                  restoreFailures.add((String) arguments[0]);
               }
               if ("federationRestoreBegun".equals(name)) restoreBegunCount++;
               if ("initiateFederateRestore".equals(name)) {
                  restoreInitiations.add(new RestoreInitiation((String) arguments[0],
                     (String) arguments[1], (FederateHandle) arguments[2]));
               }
               if ("federationRestoreStatusResponse".equals(name)) {
                  restoreStatuses.add(((FederateRestoreStatus[]) arguments[0]).clone());
               }
               if ("federationRestored".equals(name)) restoredCount++;
               if ("federationNotRestored".equals(name)) restoreFailures.add(name);
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK save/restore callback recorder";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == arguments[0];
               }
               return null;
            });
      }
   }
}
