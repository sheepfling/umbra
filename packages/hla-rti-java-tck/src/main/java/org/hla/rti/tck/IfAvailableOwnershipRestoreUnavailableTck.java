package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Restore a pending If Available request and deliver its unavailable callback exactly once. */
final class IfAvailableOwnershipRestoreUnavailableTck {
   private static final byte[] ACQUISITION_TAG = new byte[] {0x49, 0x46, 0x41, 0x56};

   private IfAvailableOwnershipRestoreUnavailableTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      String federation = "java-tck-if-available-ownership-restore-" + UUID.randomUUID();
      String ownerName = "java-tck-if-available-restore-owner";
      String requesterName = "java-tck-if-available-restore-requester";
      String saveLabel = "java-tck-if-available-ownership-restore";
      boolean ownerConnected = false;
      boolean requesterConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean requesterJoined = false;
      boolean ownerCallbacksDisabled = false;
      boolean requesterCallbacksDisabled = false;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         requester.connect(requesterRecorder.proxy(), callbackModel);
         requesterConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution(ownerName, federation);
         ownerJoined = true;
         requester.joinFederationExecution(requesterName, federation);
         requesterJoined = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle requesterClass = requester.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle requesterAttribute =
            requester.getAttributeHandle(requesterClass, attributeName);
         JavaTckSupport.check(ownerClass.equals(requesterClass)
               && ownerAttribute.equals(requesterAttribute),
            "If Available restore FOM lookups returned inconsistent handles");
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet requesterAttributes =
            JavaTckSupport.attributeSet(requester, requesterAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         requester.subscribeObjectClassAttributes(requesterClass, requesterAttributes);
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         String objectName = owner.getObjectInstanceName(object);
         await(requester, callbackModel, objectClassName,
            () -> requesterRecorder.discoveries.contains(object), "object discovery");
         requester.publishObjectClassAttributes(requesterClass, requesterAttributes);
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && !requester.isAttributeOwnedByFederate(object, requesterAttribute),
            "If Available restore started with the wrong owner");

         ownerRecorder.clearEvents();
         requesterRecorder.clearEvents();
         if (callbackModel == CallbackModel.HLA_IMMEDIATE) {
            owner.disableCallbacks();
            ownerCallbacksDisabled = true;
            requester.disableCallbacks();
            requesterCallbacksDisabled = true;
         }
         requester.attributeOwnershipAcquisitionIfAvailable(
            object, requesterAttributes, ACQUISITION_TAG.clone());

         owner.requestFederationSave(saveLabel);
         owner.federateSaveBegun();
         requester.federateSaveBegun();
         owner.federateSaveComplete();
         requester.federateSaveComplete();
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            await(owner, callbackModel, objectClassName,
               () -> !ownerRecorder.saveInitiations.isEmpty() && ownerRecorder.savedCount > 0,
               "owner federation-saved callback");
         }
         JavaTckSupport.check(requesterRecorder.unavailable.isEmpty(),
            "If Available callback was delivered before the save/restore boundary");

         int restoreRequestStart = ownerRecorder.restoreSuccesses.size();
         owner.requestFederationRestore(saveLabel);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            await(owner, callbackModel, objectClassName,
               () -> ownerRecorder.restoreSuccesses.size() > restoreRequestStart
                  && ownerRecorder.restoreBegunCount > 0
                  && !ownerRecorder.restoreInitiations.isEmpty(),
               "owner restore initiation callbacks");
         }
         owner.federateRestoreComplete();
         requester.federateRestoreComplete();
         if (callbackModel == CallbackModel.HLA_IMMEDIATE) {
            owner.enableCallbacks();
            ownerCallbacksDisabled = false;
            requester.enableCallbacks();
            requesterCallbacksDisabled = false;
         } else {
            await(owner, callbackModel, objectClassName,
               () -> ownerRecorder.restoredCount > 0,
               "owner federation-restored callback");
         }

         awaitBoth(owner, requester, callbackModel, objectClassName,
            () -> ownerRecorder.savedCount > 0 && requesterRecorder.savedCount > 0
               && ownerRecorder.restoreSuccesses.size() > restoreRequestStart
               && ownerRecorder.restoredCount > 0 && requesterRecorder.restoredCount > 0
               && !requesterRecorder.unavailable.isEmpty(),
            "If Available ownership and save/restore callbacks");
         JavaTckSupport.check(ownerRecorder.saveFailures.isEmpty()
               && requesterRecorder.saveFailures.isEmpty()
               && ownerRecorder.restoreFailures.isEmpty()
               && requesterRecorder.restoreFailures.isEmpty(),
            "If Available save/restore reported a lifecycle failure");
         JavaTckSupport.check(ownerRecorder.saveInitiations.size() == 1
               && requesterRecorder.saveInitiations.size() == 1
               && saveLabel.equals(last(ownerRecorder.saveInitiations))
               && saveLabel.equals(last(requesterRecorder.saveInitiations))
               && ownerRecorder.savedCount == 1 && requesterRecorder.savedCount == 1,
            "If Available save callbacks returned the wrong label or count");
         RestoreInitiation ownerRestore = last(ownerRecorder.restoreInitiations);
         RestoreInitiation requesterRestore = last(requesterRecorder.restoreInitiations);
         JavaTckSupport.check(ownerRecorder.restoreInitiations.size() == 1
               && requesterRecorder.restoreInitiations.size() == 1
               && saveLabel.equals(ownerRestore.label)
               && saveLabel.equals(requesterRestore.label)
               && ownerName.equals(ownerRestore.federateName)
               && requesterName.equals(requesterRestore.federateName)
               && ownerRestore.postRestoreHandle != null
               && requesterRestore.postRestoreHandle != null,
            "If Available restore callbacks returned incorrect federate data");
         JavaTckSupport.check(ownerRecorder.restoreSuccesses.size() == 1
               && saveLabel.equals(last(ownerRecorder.restoreSuccesses))
               && ownerRecorder.restoreBegunCount == 1
               && requesterRecorder.restoreBegunCount == 1
               && ownerRecorder.restoredCount == 1
               && requesterRecorder.restoredCount == 1,
            "If Available restore callbacks returned the wrong label or count");
         JavaTckSupport.check(requesterRecorder.unavailable.size() == 1
               && object.equals(last(requesterRecorder.unavailable).object)
               && new HashSet<>(requesterAttributes).equals(
                  last(requesterRecorder.unavailable).attributes)
               && Arrays.equals(ACQUISITION_TAG, last(requesterRecorder.unavailable).tag),
            "restored If Available callback returned the wrong standard data");

         ObjectInstanceHandle restoredObject = owner.getObjectInstanceHandle(objectName);
         ObjectClassHandle restoredOwnerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle restoredRequesterClass = requester.getObjectClassHandle(objectClassName);
         AttributeHandle restoredOwnerAttribute =
            owner.getAttributeHandle(restoredOwnerClass, attributeName);
         AttributeHandle restoredRequesterAttribute =
            requester.getAttributeHandle(restoredRequesterClass, attributeName);
          JavaTckSupport.check(restoredObject != null
                && restoredOwnerClass != null && restoredRequesterClass != null
                && restoredOwnerAttribute != null && restoredRequesterAttribute != null,
             "If Available restore could not resolve standard handles by name");
          JavaTckSupport.check(ownerRestore.postRestoreHandle.equals(
                owner.getFederateHandle(ownerName))
               && requesterRestore.postRestoreHandle.equals(
                  requester.getFederateHandle(requesterName))
               && owner.isAttributeOwnedByFederate(restoredObject, restoredOwnerAttribute)
               && !requester.isAttributeOwnedByFederate(restoredObject, restoredRequesterAttribute),
            "unavailable request changed ownership or restore failed to rebind standard handles");

         requester.queryAttributeOwnership(
            restoredObject, JavaTckSupport.attributeSet(requester, restoredRequesterAttribute));
         await(requester, callbackModel, objectClassName,
            () -> !requesterRecorder.information.isEmpty(), "restored ownership query");
         OwnershipEvent ownership = last(requesterRecorder.information);
         JavaTckSupport.check(requesterRecorder.information.size() == 1
               && restoredObject.equals(ownership.object)
               && new HashSet<>(List.of(restoredRequesterAttribute)).equals(ownership.attributes)
               && ownerRestore.postRestoreHandle.equals(ownership.owner),
            "If Available restored query returned the wrong object, attribute, or owner");

         for (int pass = 0; pass < 8; pass++) {
            service(owner, callbackModel, objectClassName);
            service(requester, callbackModel, objectClassName);
         }
         JavaTckSupport.check(requesterRecorder.unavailable.size() == 1,
            "If Available unavailable callback was delivered more than once");

         requester.resignFederationExecution(ResignAction.NO_ACTION);
         requesterJoined = false;
         owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         ownerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (requesterCallbacksDisabled) {
            try { requester.enableCallbacks(); } catch (Exception ignored) { }
         }
         if (ownerCallbacksDisabled) {
            try { owner.enableCallbacks(); } catch (Exception ignored) { }
         }
         if (requesterJoined) resign(requester, ResignAction.NO_ACTION);
         if (ownerJoined) resign(owner, ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         if (created) {
            try { owner.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (requesterConnected) disconnect(requester);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         String objectClassName, Condition condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && !condition.isTrue()) {
         service(ambassador, model, objectClassName);
         Thread.yield();
      }
      JavaTckSupport.check(condition.isTrue(), "timed out waiting for " + description);
   }

   private static void awaitBoth(RTIambassador first, RTIambassador second, CallbackModel model,
         String objectClassName, Condition condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && !condition.isTrue()) {
         service(first, model, objectClassName);
         service(second, model, objectClassName);
         Thread.yield();
      }
      JavaTckSupport.check(condition.isTrue(), "timed out waiting for " + description);
   }

   private static void service(RTIambassador ambassador, CallbackModel model,
         String objectClassName) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) ambassador.evokeMultipleCallbacks(0.0, 1.0);
      else ambassador.getObjectClassHandle(objectClassName);
   }

   private static void resign(RTIambassador ambassador, ResignAction action) {
      try { ambassador.resignFederationExecution(action); } catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static <T> T last(List<T> values) { return values.get(values.size() - 1); }

   @FunctionalInterface
   private interface Condition { boolean isTrue() throws Exception; }

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

   private static final class OwnershipEvent {
      private final ObjectInstanceHandle object;
      private final HashSet<AttributeHandle> attributes;
      private final FederateHandle owner;

      private OwnershipEvent(ObjectInstanceHandle object, AttributeHandleSet attributes,
            FederateHandle owner) {
         this.object = object;
         this.attributes = new HashSet<>(attributes);
         this.owner = owner;
      }
   }

   private static final class UnavailableEvent {
      private final ObjectInstanceHandle object;
      private final HashSet<AttributeHandle> attributes;
      private final byte[] tag;

      private UnavailableEvent(ObjectInstanceHandle object, AttributeHandleSet attributes,
            byte[] tag) {
         this.object = object;
         this.attributes = new HashSet<>(attributes);
         this.tag = tag.clone();
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<UnavailableEvent> unavailable = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> information = new CopyOnWriteArrayList<>();
      private final List<String> saveInitiations = new CopyOnWriteArrayList<>();
      private final List<String> saveFailures = new CopyOnWriteArrayList<>();
      private final List<String> restoreSuccesses = new CopyOnWriteArrayList<>();
      private final List<String> restoreFailures = new CopyOnWriteArrayList<>();
      private final List<RestoreInitiation> restoreInitiations = new CopyOnWriteArrayList<>();
      private volatile int restoreBegunCount;
      private volatile int savedCount;
      private volatile int restoredCount;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("attributeOwnershipUnavailable".equals(name)) {
                  unavailable.add(new UnavailableEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (byte[]) arguments[2]));
               } else if ("informAttributeOwnership".equals(name)) {
                  information.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (FederateHandle) arguments[2]));
               } else if ("initiateFederateSave".equals(name)) {
                  saveInitiations.add((String) arguments[0]);
               } else if ("federationSaved".equals(name)) {
                  savedCount++;
               } else if ("federationNotSaved".equals(name)) {
                  saveFailures.add(name);
               } else if ("requestFederationRestoreSucceeded".equals(name)) {
                  restoreSuccesses.add((String) arguments[0]);
               } else if ("requestFederationRestoreFailed".equals(name)) {
                  restoreFailures.add(name);
               } else if ("federationRestoreBegun".equals(name)) {
                  restoreBegunCount++;
               } else if ("initiateFederateRestore".equals(name)) {
                  restoreInitiations.add(new RestoreInitiation((String) arguments[0],
                     (String) arguments[1], (FederateHandle) arguments[2]));
               } else if ("federationRestored".equals(name)) {
                  restoredCount++;
               } else if ("federationNotRestored".equals(name)) {
                  restoreFailures.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK If Available restore callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }

      private void clearEvents() {
         unavailable.clear();
         information.clear();
      }
   }
}
