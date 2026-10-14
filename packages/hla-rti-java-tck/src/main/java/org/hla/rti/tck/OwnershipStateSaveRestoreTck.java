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

/** Preserve a completed negotiated attribute transfer through federation save and restore. */
final class OwnershipStateSaveRestoreTck {
   private static final byte[] DIVESTITURE_TAG = new byte[] {0x44, 0x49, 0x56};
   private static final byte[] ACQUISITION_TAG = new byte[] {0x41, 0x43, 0x51};
   private static final byte[] CONFIRMATION_TAG = new byte[] {0x43, 0x4f, 0x4e};

   private OwnershipStateSaveRestoreTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador acquirer = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder acquirerRecorder = new Recorder();
      String federation = "java-tck-ownership-state-save-restore-" + UUID.randomUUID();
      String ownerName = "java-tck-state-restore-owner";
      String acquirerName = "java-tck-state-restore-acquirer";
      String saveLabel = "java-tck-ownership-state-save-restore";
      boolean ownerConnected = false;
      boolean acquirerConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean acquirerJoined = false;
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
          JavaTckSupport.check(ownerClass != null && acquirerClass != null,
             "ownership save/restore lookup returned an invalid object-class handle");
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle acquirerAttribute =
            acquirer.getAttributeHandle(acquirerClass, attributeName);
          JavaTckSupport.check(ownerAttribute != null && acquirerAttribute != null
                && ownerClass.equals(acquirerClass)
               && ownerAttribute.equals(acquirerAttribute),
            "ownership save/restore FOM lookups returned inconsistent handles");
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet acquirerAttributes =
            JavaTckSupport.attributeSet(acquirer, acquirerAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         acquirer.publishObjectClassAttributes(acquirerClass, acquirerAttributes);
         owner.subscribeObjectClassAttributes(ownerClass, ownerAttributes);
         acquirer.subscribeObjectClassAttributes(acquirerClass, acquirerAttributes);
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
          JavaTckSupport.check(object != null,
             "ownership save/restore registration returned an invalid object handle");
         String objectName = owner.getObjectInstanceName(object);
         await(acquirer, callbackModel, objectClassName,
            () -> acquirerRecorder.discoveries.contains(object), "object discovery");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && !acquirer.isAttributeOwnedByFederate(object, acquirerAttribute),
            "initial attribute ownership was incorrect");

         owner.negotiatedAttributeOwnershipDivestiture(
            object, ownerAttributes, DIVESTITURE_TAG.clone());
         acquirer.attributeOwnershipAcquisition(
            object, acquirerAttributes, ACQUISITION_TAG.clone());
         await(owner, callbackModel, objectClassName,
            () -> !ownerRecorder.divestitureRequests.isEmpty(),
            "negotiated divestiture confirmation");
         OwnershipEvent divestiture = last(ownerRecorder.divestitureRequests);
         JavaTckSupport.check(object.equals(divestiture.object)
               && new HashSet<>(ownerAttributes).equals(divestiture.attributes)
               && Arrays.equals(ACQUISITION_TAG, divestiture.tag),
            "divestiture confirmation returned the wrong object, attributes, or acquisition tag");
         owner.confirmDivestiture(object, ownerAttributes, CONFIRMATION_TAG.clone());
         await(acquirer, callbackModel, objectClassName,
            () -> !acquirerRecorder.acquisitions.isEmpty(),
            "ownership acquisition notification");
         OwnershipEvent acquisition = last(acquirerRecorder.acquisitions);
         JavaTckSupport.check(object.equals(acquisition.object)
               && new HashSet<>(acquirerAttributes).equals(acquisition.attributes)
               && Arrays.equals(CONFIRMATION_TAG, acquisition.tag),
            "acquisition notification returned the wrong object, attributes, or confirmation tag");
         JavaTckSupport.check(!owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && acquirer.isAttributeOwnedByFederate(object, acquirerAttribute),
            "negotiated transfer did not establish the acquirer as owner before save");

         owner.requestFederationSave(saveLabel);
         awaitBoth(owner, acquirer, callbackModel, objectClassName,
            () -> !ownerRecorder.saveInitiations.isEmpty()
               && !acquirerRecorder.saveInitiations.isEmpty(), "save initiation");
         JavaTckSupport.check(saveLabel.equals(last(ownerRecorder.saveInitiations))
               && saveLabel.equals(last(acquirerRecorder.saveInitiations)),
            "save initiation returned the wrong label");
         owner.federateSaveBegun();
         acquirer.federateSaveBegun();
         owner.federateSaveComplete();
         JavaTckSupport.check(ownerRecorder.savedCount == 0 && acquirerRecorder.savedCount == 0,
            "federation save completed before every federate reported completion");
         acquirer.federateSaveComplete();
         awaitBoth(owner, acquirer, callbackModel, objectClassName,
            () -> ownerRecorder.savedCount > 0 && acquirerRecorder.savedCount > 0,
            "federation-saved callbacks");
         JavaTckSupport.check(ownerRecorder.saveFailures.isEmpty()
               && acquirerRecorder.saveFailures.isEmpty(),
            "successful ownership save reported a failure");

         owner.requestFederationRestore(saveLabel);
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
         RestoreInitiation ownerRestore = last(ownerRecorder.restoreInitiations);
         RestoreInitiation acquirerRestore = last(acquirerRecorder.restoreInitiations);
         JavaTckSupport.check(saveLabel.equals(ownerRestore.label)
               && ownerName.equals(ownerRestore.federateName)
               && ownerRestore.postRestoreHandle != null
               && saveLabel.equals(acquirerRestore.label)
               && acquirerName.equals(acquirerRestore.federateName)
               && acquirerRestore.postRestoreHandle != null,
            "restore initiation returned incorrect saved identity data");
         owner.federateRestoreComplete();
         acquirer.federateRestoreComplete();
         awaitBoth(owner, acquirer, callbackModel, objectClassName,
            () -> ownerRecorder.restoredCount > 0 && acquirerRecorder.restoredCount > 0,
            "federation-restored callbacks");
         JavaTckSupport.check(ownerRecorder.restoreFailures.isEmpty()
               && acquirerRecorder.restoreFailures.isEmpty(),
            "successful ownership restore reported a failure");
         JavaTckSupport.check(ownerRestore.postRestoreHandle.equals(
               owner.getFederateHandle(ownerName))
               && acquirerRestore.postRestoreHandle.equals(acquirer.getFederateHandle(acquirerName)),
            "restore did not install the post-restore federate handles");

         ObjectInstanceHandle restoredObject = owner.getObjectInstanceHandle(objectName);
         ObjectClassHandle restoredOwnerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle restoredAcquirerClass = acquirer.getObjectClassHandle(objectClassName);
          JavaTckSupport.check(restoredObject != null
                && restoredOwnerClass != null && restoredAcquirerClass != null,
             "ownership restore could not rebind standard object or class handles");
         AttributeHandle restoredOwnerAttribute =
            owner.getAttributeHandle(restoredOwnerClass, attributeName);
         AttributeHandle restoredAcquirerAttribute =
            acquirer.getAttributeHandle(restoredAcquirerClass, attributeName);
          JavaTckSupport.check(restoredOwnerAttribute != null && restoredAcquirerAttribute != null,
             "ownership restore could not rebind standard attribute handles");
         JavaTckSupport.check(restoredObject != null
               && !owner.isAttributeOwnedByFederate(restoredObject, restoredOwnerAttribute)
               && acquirer.isAttributeOwnedByFederate(restoredObject, restoredAcquirerAttribute),
            "federation restore did not preserve the completed ownership transfer");

         acquirerRecorder.information.clear();
         acquirer.queryAttributeOwnership(
            restoredObject, JavaTckSupport.attributeSet(acquirer, restoredAcquirerAttribute));
         await(acquirer, callbackModel, objectClassName,
            () -> !acquirerRecorder.information.isEmpty(), "restored ownership query");
         JavaTckSupport.check(acquirerRecorder.information.size() == 1,
            "restored ownership query returned an unexpected number of ownership callbacks");
         OwnershipEvent ownership = last(acquirerRecorder.information);
         JavaTckSupport.check(restoredObject.equals(ownership.object)
               && new HashSet<>(List.of(restoredAcquirerAttribute)).equals(ownership.attributes)
               && acquirerRestore.postRestoreHandle.equals(ownership.owner),
            "restored query returned the wrong object, attribute, or post-restore owner");

          acquirer.resignFederationExecution(ResignAction.UNCONDITIONALLY_DIVEST_ATTRIBUTES);
         acquirerJoined = false;
          owner.resignFederationExecution(ResignAction.NO_ACTION);
         ownerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
          if (acquirerJoined) resign(acquirer, ResignAction.UNCONDITIONALLY_DIVEST_ATTRIBUTES);
          if (ownerJoined) resign(owner, ResignAction.NO_ACTION);
         if (created) {
            try { owner.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (acquirerConnected) disconnect(acquirer);
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
                  if ("toString".equals(name)) return "Java TCK ownership state restore callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }

      private static OwnershipEvent event(Object[] arguments) {
         return new OwnershipEvent((ObjectInstanceHandle) arguments[0],
            (AttributeHandleSet) arguments[1], null, (byte[]) arguments[2]);
      }
   }
}
