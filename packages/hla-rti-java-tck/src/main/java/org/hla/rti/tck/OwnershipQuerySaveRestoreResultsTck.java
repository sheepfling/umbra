package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
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
import java.util.Map;
import java.util.Optional;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Standard ownership-query result classification across federation save and restore. */
final class OwnershipQuerySaveRestoreResultsTck {
   private static final String MOM_FEDERATE_CLASS = "HLAobjectRoot.HLAmanager.HLAfederate";
   private static final String MOM_FEDERATE_HANDLE = "HLAfederateHandle";
   private static final byte[] ACQUISITION_TAG = new byte[] {0x51, 0x52};
   private static final byte[] CONFIRMATION_TAG = new byte[] {0x53, 0x54};

   private OwnershipQuerySaveRestoreResultsTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String firstAttributeName, String secondAttributeName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      String federation = "java-tck-ownership-query-save-restore-" + UUID.randomUUID();
      String ownerName = "java-tck-query-restore-owner";
      String requesterName = "java-tck-query-restore-requester";
      String saveLabel = "java-tck-ownership-query-save-restore";
      boolean ownerConnected = false;
      boolean requesterConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean requesterJoined = false;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         requester.connect(requesterRecorder.proxy(), callbackModel);
         requesterConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution(ownerName, federation);
         ownerJoined = true;
         FederateHandle requesterHandle = requester.joinFederationExecution(
            requesterName, federation);
         requesterJoined = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle requesterClass = requester.getObjectClassHandle(objectClassName);
          JavaTckSupport.check(ownerClass != null && requesterClass != null,
             "ownership-query save/restore FOM lookup returned an invalid class handle");
          AttributeHandle ownerFirst = owner.getAttributeHandle(ownerClass, firstAttributeName);
          AttributeHandle ownerSecond = owner.getAttributeHandle(ownerClass, secondAttributeName);
         AttributeHandle requesterFirst = requester.getAttributeHandle(
            requesterClass, firstAttributeName);
         AttributeHandle requesterSecond = requester.getAttributeHandle(
            requesterClass, secondAttributeName);
          JavaTckSupport.check(ownerFirst != null && ownerSecond != null
                && requesterFirst != null && requesterSecond != null
                && ownerClass.equals(requesterClass)
                && ownerFirst.equals(requesterFirst)
                && !ownerFirst.equals(ownerSecond)
                && !requesterFirst.equals(requesterSecond),
            "ownership-query save/restore FOM lookups returned inconsistent handles");

         AttributeHandleSet ownerFirstOnly = JavaTckSupport.attributeSet(owner, ownerFirst);
         AttributeHandleSet requesterFirstOnly =
            JavaTckSupport.attributeSet(requester, requesterFirst);
         AttributeHandleSet requesterQuerySet =
            JavaTckSupport.attributeSet(requester, requesterFirst, requesterSecond);
         owner.publishObjectClassAttributes(ownerClass, ownerFirstOnly);
         requester.publishObjectClassAttributes(requesterClass, requesterFirstOnly);
         requester.subscribeObjectClassAttributes(requesterClass, requesterQuerySet);
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
          JavaTckSupport.check(object != null,
             "ownership-query save/restore registration returned an invalid object handle");
         String objectName = owner.getObjectInstanceName(object);
         await(requester, callbackModel, objectClassName,
            () -> requesterRecorder.discoveries.contains(object),
            "ownership-query save/restore object discovery");

         ObjectClassHandle momClass = requester.getObjectClassHandle(MOM_FEDERATE_CLASS);
          JavaTckSupport.check(momClass != null,
             "ownership-query save/restore MOM lookup returned an invalid class handle");
         AttributeHandle momHandleAttribute =
            requester.getAttributeHandle(momClass, MOM_FEDERATE_HANDLE);
          JavaTckSupport.check(momHandleAttribute != null,
             "ownership-query save/restore MOM lookup returned an invalid attribute handle");
         requester.subscribeObjectClassAttributes(
            momClass, JavaTckSupport.attributeSet(requester, momHandleAttribute));
         byte[] encodedRequester = new byte[requesterHandle.encodedLength()];
         requesterHandle.encode(encodedRequester, 0);
         await(requester, callbackModel, objectClassName,
            () -> requesterRecorder.momObjectForHandle(momHandleAttribute, encodedRequester)
               .isPresent(), "joined-federate MOM reflection");
         ObjectInstanceHandle momObject = requesterRecorder
            .momObjectForHandle(momHandleAttribute, encodedRequester).get();
         JavaTckSupport.check(momClass.equals(requester.getKnownObjectClassHandle(momObject)),
            "MOM reflection identified the wrong joined-federate object class");

         owner.requestFederationSave(saveLabel);
         awaitBoth(owner, requester, callbackModel, objectClassName,
            () -> !ownerRecorder.saveInitiations.isEmpty()
               && !requesterRecorder.saveInitiations.isEmpty(), "save initiation");
         JavaTckSupport.check(saveLabel.equals(last(ownerRecorder.saveInitiations))
               && saveLabel.equals(last(requesterRecorder.saveInitiations)),
            "save initiation returned the wrong label");
         owner.federateSaveBegun();
         requester.federateSaveBegun();
         owner.federateSaveComplete();
         JavaTckSupport.check(ownerRecorder.savedCount == 0 && requesterRecorder.savedCount == 0,
            "federation save completed before every federate reported completion");
         requester.federateSaveComplete();
         awaitBoth(owner, requester, callbackModel, objectClassName,
            () -> ownerRecorder.savedCount > 0 && requesterRecorder.savedCount > 0,
            "federation-saved callbacks");
         JavaTckSupport.check(ownerRecorder.saveFailures.isEmpty()
               && requesterRecorder.saveFailures.isEmpty(),
            "successful ownership-query save reported a failure");

         ownerRecorder.clearOwnershipEvents();
         requesterRecorder.clearOwnershipEvents();
         owner.negotiatedAttributeOwnershipDivestiture(object, ownerFirstOnly, new byte[0]);
         requester.attributeOwnershipAcquisition(
            object, requesterFirstOnly, ACQUISITION_TAG.clone());
         await(owner, callbackModel, objectClassName,
            () -> !ownerRecorder.divestitureConfirmations.isEmpty(),
            "post-save divestiture confirmation request");
         OwnershipEvent confirmation = last(ownerRecorder.divestitureConfirmations);
         JavaTckSupport.check(object.equals(confirmation.object)
               && new HashSet<>(ownerFirstOnly).equals(confirmation.attributes)
               && Arrays.equals(ACQUISITION_TAG, confirmation.tag),
            "post-save confirmation request returned the wrong ownership transfer");
         owner.confirmDivestiture(object, ownerFirstOnly, CONFIRMATION_TAG.clone());
         await(requester, callbackModel, objectClassName,
            () -> !requesterRecorder.acquisitions.isEmpty(),
            "post-save acquisition notification");
         OwnershipEvent acquisition = last(requesterRecorder.acquisitions);
         JavaTckSupport.check(object.equals(acquisition.object)
               && new HashSet<>(requesterFirstOnly).equals(acquisition.attributes)
               && Arrays.equals(CONFIRMATION_TAG, acquisition.tag),
            "post-save acquisition notification returned the wrong attributes or tag");
         JavaTckSupport.check(!owner.isAttributeOwnedByFederate(object, ownerFirst)
               && requester.isAttributeOwnedByFederate(object, requesterFirst),
            "post-save negotiated transfer did not change the live owner");

         int ownerRestoreBefore = ownerRecorder.restoreInitiations.size();
         int requesterRestoreBefore = requesterRecorder.restoreInitiations.size();
         int ownerRestoredBefore = ownerRecorder.restoredCount;
         int requesterRestoredBefore = requesterRecorder.restoredCount;
         owner.requestFederationRestore(saveLabel);
         awaitBoth(owner, requester, callbackModel, objectClassName,
            () -> ownerRecorder.restoreSuccesses.size() > 0
               && ownerRecorder.restoreInitiations.size() > ownerRestoreBefore
               && requesterRecorder.restoreInitiations.size() > requesterRestoreBefore
               && ownerRecorder.restoreBegunCount > 0
               && requesterRecorder.restoreBegunCount > 0,
            "restore initiation callbacks");
         JavaTckSupport.check(saveLabel.equals(last(ownerRecorder.restoreSuccesses))
               && ownerRecorder.restoreFailures.isEmpty()
               && requesterRecorder.restoreFailures.isEmpty(),
            "restore request returned an unexpected result");
         RestoreInitiation ownerRestore = last(ownerRecorder.restoreInitiations);
         RestoreInitiation requesterRestore = last(requesterRecorder.restoreInitiations);
         JavaTckSupport.check(saveLabel.equals(ownerRestore.label)
               && ownerName.equals(ownerRestore.federateName)
               && ownerRestore.postRestoreHandle != null
               && saveLabel.equals(requesterRestore.label)
               && requesterName.equals(requesterRestore.federateName)
               && requesterRestore.postRestoreHandle != null,
            "restore initiation returned incorrect saved identity data");
         owner.federateRestoreComplete();
         requester.federateRestoreComplete();
         awaitBoth(owner, requester, callbackModel, objectClassName,
            () -> ownerRecorder.restoredCount > ownerRestoredBefore
               && requesterRecorder.restoredCount > requesterRestoredBefore,
            "federation-restored callbacks");
         JavaTckSupport.check(ownerRecorder.restoreFailures.isEmpty()
               && requesterRecorder.restoreFailures.isEmpty(),
            "successful ownership-query restore reported a failure");

         ObjectInstanceHandle restoredObject = requester.getObjectInstanceHandle(objectName);
         ObjectClassHandle restoredOwnerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle restoredRequesterClass = requester.getObjectClassHandle(objectClassName);
          JavaTckSupport.check(restoredObject != null
                && restoredOwnerClass != null && restoredRequesterClass != null,
             "ownership-query restore could not rebind standard object or class handles");
         AttributeHandle restoredOwnerFirst =
            owner.getAttributeHandle(restoredOwnerClass, firstAttributeName);
         AttributeHandle restoredRequesterFirst =
            requester.getAttributeHandle(restoredRequesterClass, firstAttributeName);
         AttributeHandle restoredRequesterSecond =
            requester.getAttributeHandle(restoredRequesterClass, secondAttributeName);
          JavaTckSupport.check(restoredOwnerFirst != null && restoredRequesterFirst != null
                && restoredRequesterSecond != null,
             "ownership-query restore could not rebind standard attribute handles");
          JavaTckSupport.check(owner.isAttributeOwnedByFederate(restoredObject, restoredOwnerFirst)
               && !requester.isAttributeOwnedByFederate(restoredObject, restoredRequesterFirst),
            "restore did not reinstate the saved federate owner");

         requesterRecorder.clearOwnershipEvents();
         requester.queryAttributeOwnership(restoredObject,
            JavaTckSupport.attributeSet(requester, restoredRequesterFirst, restoredRequesterSecond));
         await(requester, callbackModel, objectClassName,
            () -> !requesterRecorder.information.isEmpty()
               && !requesterRecorder.notOwned.isEmpty(),
            "restored mixed ownership-query result");
         OwnershipEvent information = last(requesterRecorder.information);
         OwnershipEvent notOwned = last(requesterRecorder.notOwned);
         JavaTckSupport.check(requesterRecorder.information.size() == 1
               && requesterRecorder.notOwned.size() == 1
               && restoredObject.equals(information.object)
               && new HashSet<>(List.of(restoredRequesterFirst)).equals(information.attributes)
               && ownerRestore.postRestoreHandle.equals(information.owner),
            "restored ownership query returned the wrong federate-owned subset or owner");
         JavaTckSupport.check(restoredObject.equals(notOwned.object)
               && new HashSet<>(List.of(restoredRequesterSecond)).equals(notOwned.attributes)
               && requesterRecorder.ownedByRti.isEmpty(),
            "restored ownership query did not distinguish the ordinary unowned attribute");
         JavaTckSupport.check(requesterRestore.postRestoreHandle.equals(
               requester.getFederateHandle(requesterName))
               && momClass.equals(requester.getKnownObjectClassHandle(momObject)),
            "restore did not preserve the joined-federate MOM identity");

         requesterRecorder.clearOwnershipEvents();
         requester.queryAttributeOwnership(momObject,
            JavaTckSupport.attributeSet(requester, momHandleAttribute));
         await(requester, callbackModel, objectClassName,
            () -> !requesterRecorder.ownedByRti.isEmpty(), "restored RTI-owned MOM query result");
         OwnershipEvent ownedByRti = last(requesterRecorder.ownedByRti);
         JavaTckSupport.check(requesterRecorder.ownedByRti.size() == 1
               && momObject.equals(ownedByRti.object)
               && new HashSet<>(List.of(momHandleAttribute)).equals(ownedByRti.attributes)
               && requesterRecorder.information.isEmpty()
               && requesterRecorder.notOwned.isEmpty(),
            "restored MOM query did not identify exactly the RTI-owned attribute");

         requester.resignFederationExecution(ResignAction.NO_ACTION);
         requesterJoined = false;
         owner.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         ownerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (requesterJoined) resign(requester, ResignAction.NO_ACTION);
         if (ownerJoined) resign(owner, ResignAction.DELETE_OBJECTS);
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
      private final byte[] tag;

      private OwnershipEvent(ObjectInstanceHandle object, AttributeHandleSet attributes,
            FederateHandle owner, byte[] tag) {
         this.object = object;
         this.attributes = new HashSet<>(attributes);
         this.owner = owner;
         this.tag = tag == null ? new byte[0] : tag.clone();
      }
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final Map<AttributeHandle, byte[]> values;

      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap values) {
         this.object = object;
         this.values = new java.util.HashMap<>();
         for (AttributeHandle handle : values.keySet()) {
            this.values.put(handle, values.get(handle).clone());
         }
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> divestitureConfirmations = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> acquisitions = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> information = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> notOwned = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> ownedByRti = new CopyOnWriteArrayList<>();
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
               } else if ("reflectAttributeValues".equals(name)) {
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1]));
               } else if ("requestDivestitureConfirmation".equals(name)) {
                  divestitureConfirmations.add(new OwnershipEvent(
                     (ObjectInstanceHandle) arguments[0], (AttributeHandleSet) arguments[1], null,
                     (byte[]) arguments[2]));
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)) {
                  acquisitions.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], null, (byte[]) arguments[2]));
               } else if ("informAttributeOwnership".equals(name)) {
                  information.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (FederateHandle) arguments[2], null));
               } else if ("attributeIsNotOwned".equals(name)) {
                  notOwned.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], null, null));
               } else if ("attributeIsOwnedByRTI".equals(name)) {
                  ownedByRti.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], null, null));
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
                  if ("toString".equals(name)) return "Java TCK ownership restore callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == arguments[0];
               }
               return null;
            });
      }

      private Optional<ObjectInstanceHandle> momObjectForHandle(AttributeHandle attribute,
            byte[] expected) {
         return reflections.stream()
            .filter(reflection -> reflection.values.containsKey(attribute)
               && Arrays.equals(expected, reflection.values.get(attribute)))
            .map(reflection -> reflection.object)
            .findFirst();
      }

      private void clearOwnershipEvents() {
         divestitureConfirmations.clear();
         acquisitions.clear();
         information.clear();
         notOwned.clear();
         ownedByRti.clear();
      }
   }
}
