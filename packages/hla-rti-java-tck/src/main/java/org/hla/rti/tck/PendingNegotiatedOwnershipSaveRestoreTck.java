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

/** Preserve a pending negotiated acquisition and divestiture through save and restore. */
final class PendingNegotiatedOwnershipSaveRestoreTck {
   private static final byte[] DIVESTITURE_TAG = new byte[] {0x44, 0x49, 0x56};
   private static final byte[] ACQUISITION_TAG = new byte[] {0x41, 0x43, 0x51};
   private static final byte[] CONFIRMATION_TAG = new byte[] {0x43, 0x4f, 0x4e};

   private PendingNegotiatedOwnershipSaveRestoreTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador acquirer = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder acquirerRecorder = new Recorder();
      String federation = "java-tck-pending-ownership-save-restore-" + UUID.randomUUID();
      String ownerName = "java-tck-pending-restore-owner";
      String acquirerName = "java-tck-pending-restore-acquirer";
      String saveLabel = "java-tck-pending-ownership-save-restore";
      boolean ownerConnected = false;
      boolean acquirerConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean acquirerJoined = false;
      boolean ownerCallbacksDisabled = false;
      boolean acquirerCallbacksDisabled = false;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         acquirer.connect(acquirerRecorder.proxy(), callbackModel);
         acquirerConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution(ownerName, federation);
         ownerJoined = true;
         acquirer.joinFederationExecution(acquirerName, federation);
         acquirerJoined = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle acquirerClass = acquirer.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle acquirerAttribute =
            acquirer.getAttributeHandle(acquirerClass, attributeName);
         JavaTckSupport.check(ownerClass.equals(acquirerClass)
               && ownerAttribute.equals(acquirerAttribute),
            "pending ownership save/restore FOM lookups returned inconsistent handles");
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet acquirerAttributes =
            JavaTckSupport.attributeSet(acquirer, acquirerAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         acquirer.publishObjectClassAttributes(acquirerClass, acquirerAttributes);
         owner.subscribeObjectClassAttributes(ownerClass, ownerAttributes);
         acquirer.subscribeObjectClassAttributes(acquirerClass, acquirerAttributes);
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         String objectName = owner.getObjectInstanceName(object);
         await(acquirer, callbackModel, objectClassName,
            () -> acquirerRecorder.discoveries.contains(object), "object discovery");

          // Suppress immediate callbacks so the If Available request remains pending until the
          // negotiated divestiture can be confirmed, then carry that pending state through save.
          if (callbackModel == CallbackModel.HLA_IMMEDIATE) {
             owner.disableCallbacks();
             ownerCallbacksDisabled = true;
             acquirer.disableCallbacks();
             acquirerCallbacksDisabled = true;
          }
          acquirer.attributeOwnershipAcquisitionIfAvailable(
             object, acquirerAttributes, ACQUISITION_TAG.clone());
         owner.negotiatedAttributeOwnershipDivestiture(
            object, ownerAttributes, DIVESTITURE_TAG.clone());
          if (!ownerCallbacksDisabled) {
             await(owner, callbackModel, objectClassName,
                () -> !ownerRecorder.divestitureRequests.isEmpty(),
                "pending divestiture confirmation callback");
             OwnershipEvent divestiture = last(ownerRecorder.divestitureRequests);
             JavaTckSupport.check(object.equals(divestiture.object)
                   && new HashSet<>(ownerAttributes).equals(divestiture.attributes)
                   && Arrays.equals(ACQUISITION_TAG, divestiture.tag),
                "pending divestiture callback returned the wrong object, attributes, or acquisition tag");
          } else {
             JavaTckSupport.check(ownerRecorder.divestitureRequests.isEmpty()
                   && acquirerRecorder.acquisitions.isEmpty(),
                "disabled callbacks delivered ownership work before the save boundary");
          }
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && !acquirer.isAttributeOwnedByFederate(object, acquirerAttribute),
            "ownership changed before the negotiated divestiture was confirmed");

         owner.requestFederationSave(saveLabel);
          if (!ownerCallbacksDisabled) {
             awaitBoth(owner, acquirer, callbackModel, objectClassName,
                () -> !ownerRecorder.saveInitiations.isEmpty()
                   && !acquirerRecorder.saveInitiations.isEmpty(), "save initiation");
             JavaTckSupport.check(saveLabel.equals(last(ownerRecorder.saveInitiations))
                   && saveLabel.equals(last(acquirerRecorder.saveInitiations)),
                "save initiation returned the wrong label");
          }
         owner.federateSaveBegun();
         acquirer.federateSaveBegun();
         owner.federateSaveComplete();
         JavaTckSupport.check(ownerRecorder.savedCount == 0 && acquirerRecorder.savedCount == 0,
            "federation save completed before every federate reported completion");
         acquirer.federateSaveComplete();
          if (!ownerCallbacksDisabled) {
             awaitBoth(owner, acquirer, callbackModel, objectClassName,
                () -> ownerRecorder.savedCount > 0 && acquirerRecorder.savedCount > 0,
                "federation-saved callbacks");
          }
         JavaTckSupport.check(ownerRecorder.saveFailures.isEmpty()
               && acquirerRecorder.saveFailures.isEmpty(),
            "successful pending-ownership save reported a failure");

         owner.requestFederationRestore(saveLabel);
          RestoreInitiation ownerRestore = null;
          RestoreInitiation acquirerRestore = null;
          if (!ownerCallbacksDisabled) {
             awaitBoth(owner, acquirer, callbackModel, objectClassName,
                () -> !ownerRecorder.restoreSuccesses.isEmpty()
                   && !ownerRecorder.restoreInitiations.isEmpty()
                   && !acquirerRecorder.restoreInitiations.isEmpty()
                   && ownerRecorder.restoreBegunCount > 0
                   && acquirerRecorder.restoreBegunCount > 0,
                "restore initiation callbacks");
             JavaTckSupport.check(saveLabel.equals(last(ownerRecorder.restoreSuccesses))
                   && ownerRecorder.restoreFailures.isEmpty()
                   && acquirerRecorder.restoreFailures.isEmpty(),
                "restore request returned an unexpected result");
             ownerRestore = last(ownerRecorder.restoreInitiations);
             acquirerRestore = last(acquirerRecorder.restoreInitiations);
             checkRestoreInitiation(saveLabel, ownerName, acquirerName,
                ownerRestore, acquirerRestore);
          }
         owner.federateRestoreComplete();
         acquirer.federateRestoreComplete();
          if (ownerCallbacksDisabled) {
             owner.enableCallbacks();
             ownerCallbacksDisabled = false;
             acquirer.enableCallbacks();
             acquirerCallbacksDisabled = false;
          }
         awaitBoth(owner, acquirer, callbackModel, objectClassName,
            () -> ownerRecorder.restoredCount > 0 && acquirerRecorder.restoredCount > 0,
            "federation-restored callbacks");
         JavaTckSupport.check(ownerRecorder.restoreFailures.isEmpty()
               && acquirerRecorder.restoreFailures.isEmpty(),
            "successful pending-ownership restore reported a failure");
          if (ownerRestore == null) {
             JavaTckSupport.check(!ownerRecorder.restoreSuccesses.isEmpty()
                   && saveLabel.equals(last(ownerRecorder.restoreSuccesses)),
                "deferred restore request returned the wrong label");
             ownerRestore = last(ownerRecorder.restoreInitiations);
             acquirerRestore = last(acquirerRecorder.restoreInitiations);
             checkRestoreInitiation(saveLabel, ownerName, acquirerName,
                ownerRestore, acquirerRestore);
          }
         JavaTckSupport.check(ownerRestore.postRestoreHandle.equals(
               owner.getFederateHandle(ownerName))
               && acquirerRestore.postRestoreHandle.equals(acquirer.getFederateHandle(acquirerName)),
            "restore did not install the post-restore federate handles");

         ObjectInstanceHandle restoredObject = owner.getObjectInstanceHandle(objectName);
         ObjectClassHandle restoredOwnerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle restoredAcquirerClass = acquirer.getObjectClassHandle(objectClassName);
         AttributeHandle restoredOwnerAttribute =
            owner.getAttributeHandle(restoredOwnerClass, attributeName);
         AttributeHandle restoredAcquirerAttribute =
            acquirer.getAttributeHandle(restoredAcquirerClass, attributeName);
         AttributeHandleSet restoredOwnerAttributes =
            JavaTckSupport.attributeSet(owner, restoredOwnerAttribute);
         AttributeHandleSet restoredAcquirerAttributes =
            JavaTckSupport.attributeSet(acquirer, restoredAcquirerAttribute);
         JavaTckSupport.check(restoredObject != null
               && owner.isAttributeOwnedByFederate(restoredObject, restoredOwnerAttribute)
               && !acquirer.isAttributeOwnedByFederate(restoredObject, restoredAcquirerAttribute),
            "restore changed ownership before the saved negotiated transfer was confirmed");

          if (callbackModel == CallbackModel.HLA_IMMEDIATE) {
             await(owner, callbackModel, objectClassName,
                () -> !ownerRecorder.divestitureRequests.isEmpty(),
                "deferred post-restore divestiture confirmation");
             OwnershipEvent divestiture = last(ownerRecorder.divestitureRequests);
             JavaTckSupport.check(ownerRecorder.divestitureRequests.size() == 1
                   && restoredObject.equals(divestiture.object)
                   && new HashSet<>(restoredOwnerAttributes).equals(divestiture.attributes)
                   && Arrays.equals(ACQUISITION_TAG, divestiture.tag),
                "deferred divestiture callback returned the wrong restored ownership metadata");
          }
          ownerRecorder.clearTransferEvents();
         acquirerRecorder.clearTransferEvents();
         owner.confirmDivestiture(
            restoredObject, restoredOwnerAttributes, CONFIRMATION_TAG.clone());
         await(acquirer, callbackModel, objectClassName,
            () -> !acquirerRecorder.acquisitions.isEmpty(),
            "post-restore ownership acquisition notification");
         OwnershipEvent acquisition = last(acquirerRecorder.acquisitions);
         JavaTckSupport.check(acquirerRecorder.acquisitions.size() == 1
               && restoredObject.equals(acquisition.object)
               && new HashSet<>(restoredAcquirerAttributes).equals(acquisition.attributes)
               && Arrays.equals(CONFIRMATION_TAG, acquisition.tag),
            "post-restore acquisition callback returned the wrong object, attributes, or tag");
         JavaTckSupport.check(!owner.isAttributeOwnedByFederate(
               restoredObject, restoredOwnerAttribute)
               && acquirer.isAttributeOwnedByFederate(restoredObject, restoredAcquirerAttribute),
            "post-restore divestiture confirmation did not transfer ownership");

         acquirer.queryAttributeOwnership(restoredObject, restoredAcquirerAttributes);
         await(acquirer, callbackModel, objectClassName,
            () -> !acquirerRecorder.information.isEmpty(), "post-restore ownership query");
         JavaTckSupport.check(acquirerRecorder.information.size() == 1,
            "post-restore query returned an unexpected number of ownership callbacks");
         OwnershipEvent ownership = last(acquirerRecorder.information);
         JavaTckSupport.check(restoredObject.equals(ownership.object)
               && new HashSet<>(restoredAcquirerAttributes).equals(ownership.attributes)
               && acquirerRestore.postRestoreHandle.equals(ownership.owner),
            "post-restore query returned the wrong object, attributes, or owner handle");

         acquirer.resignFederationExecution(ResignAction.NO_ACTION);
         acquirerJoined = false;
         owner.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         ownerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (acquirerCallbacksDisabled) {
            try { acquirer.enableCallbacks(); } catch (Exception ignored) { }
         }
         if (ownerCallbacksDisabled) {
            try { owner.enableCallbacks(); } catch (Exception ignored) { }
         }
         if (acquirerJoined) resign(acquirer, ResignAction.NO_ACTION);
         if (ownerJoined) resign(owner, ResignAction.DELETE_OBJECTS);
         if (created) {
            try { owner.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (acquirerConnected) disconnect(acquirer);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static void checkRestoreInitiation(String saveLabel, String ownerName,
         String acquirerName, RestoreInitiation ownerRestore,
         RestoreInitiation acquirerRestore) {
      JavaTckSupport.check(saveLabel.equals(ownerRestore.label)
            && ownerName.equals(ownerRestore.federateName)
            && ownerRestore.postRestoreHandle != null
            && saveLabel.equals(acquirerRestore.label)
            && acquirerName.equals(acquirerRestore.federateName)
            && acquirerRestore.postRestoreHandle != null,
         "restore initiation returned incorrect saved identity data");
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
      private final byte[] tag;

      private OwnershipEvent(ObjectInstanceHandle object, AttributeHandleSet attributes,
            FederateHandle owner, byte[] tag) {
         this.object = object;
         this.attributes = new HashSet<>(attributes);
         this.owner = owner;
         this.tag = tag == null ? new byte[0] : tag.clone();
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> divestitureRequests = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> acquisitions = new CopyOnWriteArrayList<>();
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
               } else if ("requestDivestitureConfirmation".equals(name)) {
                  divestitureRequests.add(event(arguments));
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)) {
                  acquisitions.add(event(arguments));
               } else if ("informAttributeOwnership".equals(name)) {
                  information.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (FederateHandle) arguments[2], null));
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
                  if ("toString".equals(name)) return "Java TCK pending ownership restore callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }

      private void clearTransferEvents() {
         divestitureRequests.clear();
         acquisitions.clear();
         information.clear();
      }

      private static OwnershipEvent event(Object[] arguments) {
         return new OwnershipEvent((ObjectInstanceHandle) arguments[0],
            (AttributeHandleSet) arguments[1], null, (byte[]) arguments[2]);
      }
   }
}
