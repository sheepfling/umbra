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
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks transfer of HLAprivilegeToDeleteObject and resulting deletion rights. */
final class PrivilegeToDeleteTransferTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private PrivilegeToDeleteTransferTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador oldOwner = factory.getRtiAmbassador();
      RTIambassador newOwner = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder oldCallbacks = new Recorder();
      Recorder newCallbacks = new Recorder();
      Recorder observerCallbacks = new Recorder();
      String federation = "java-tck-privilege-to-delete-transfer-" + UUID.randomUUID();
      boolean[] connected = new boolean[3];
      boolean[] joined = new boolean[3];
      boolean created = false;
      try {
         oldOwner.connect(oldCallbacks.proxy(), callbackModel);
         connected[0] = true;
         newOwner.connect(newCallbacks.proxy(), callbackModel);
         connected[1] = true;
         observer.connect(observerCallbacks.proxy(), callbackModel);
         connected[2] = true;
         oldOwner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle oldOwnerHandle = oldOwner.joinFederationExecution(
            "java-tck-delete-privilege-old-owner", federation);
         joined[0] = true;
         FederateHandle newOwnerHandle = newOwner.joinFederationExecution(
            "java-tck-delete-privilege-new-owner", federation);
         joined[1] = true;
         observer.joinFederationExecution("java-tck-delete-privilege-observer", federation);
         joined[2] = true;

         ObjectClassHandle oldClass = oldOwner.getObjectClassHandle(objectClassName);
         ObjectClassHandle newClass = newOwner.getObjectClassHandle(objectClassName);
         ObjectClassHandle observerClass = observer.getObjectClassHandle(objectClassName);
         AttributeHandle oldValue = oldOwner.getAttributeHandle(oldClass, attributeName);
         AttributeHandle newValue = newOwner.getAttributeHandle(newClass, attributeName);
         AttributeHandle observerValue = observer.getAttributeHandle(observerClass, attributeName);
         AttributeHandle oldPrivilege = oldOwner.getAttributeHandle(oldClass,
            "HLAprivilegeToDeleteObject");
         AttributeHandle newPrivilege = newOwner.getAttributeHandle(newClass,
            "HLAprivilegeToDeleteObject");
         JavaTckSupport.check(objectClassName.equals(oldOwner.getObjectClassName(oldClass))
               && objectClassName.equals(newOwner.getObjectClassName(newClass))
               && attributeName.equals(oldOwner.getAttributeName(oldClass, oldValue))
               && "HLAprivilegeToDeleteObject".equals(
                  oldOwner.getAttributeName(oldClass, oldPrivilege)),
            "configured class or standard delete-privilege attribute lookup did not round-trip");

         AttributeHandleSet oldValues = JavaTckSupport.attributeSet(oldOwner, oldValue);
         AttributeHandleSet newValues = JavaTckSupport.attributeSet(newOwner, newValue);
         AttributeHandleSet observerValues = JavaTckSupport.attributeSet(observer, observerValue);
         AttributeHandleSet oldTransferred = JavaTckSupport.attributeSet(
            oldOwner, oldValue, oldPrivilege);
         AttributeHandleSet newTransferred = JavaTckSupport.attributeSet(
            newOwner, newValue, newPrivilege);
         oldOwner.publishObjectClassAttributes(oldClass, oldValues);
         newOwner.subscribeObjectClassAttributes(newClass, newValues);
         observer.subscribeObjectClassAttributes(observerClass, observerValues);

         ObjectInstanceHandle object = oldOwner.registerObjectInstance(oldClass);
         await(newOwner, callbackModel, () -> newCallbacks.discoveries.contains(object),
            "new-owner object discovery");
         await(observer, callbackModel, () -> observerCallbacks.discoveries.contains(object),
            "observer object discovery");
         JavaTckSupport.check(oldOwner.isAttributeOwnedByFederate(object, oldPrivilege)
               && !newOwner.isAttributeOwnedByFederate(object, newPrivilege),
            "delete privilege did not initially belong only to the registrant");

         newOwner.publishObjectClassAttributes(newClass, newTransferred);
         byte[] acquisitionTag = {(byte) 0xa5, 0x25};
         byte[] divestitureTag = {(byte) 0xd1, 0x25};
         newOwner.attributeOwnershipAcquisition(object, newTransferred, acquisitionTag.clone());
         AttributeHandleSet divested = oldOwner.attributeOwnershipDivestitureIfWanted(
            object, oldTransferred, divestitureTag.clone());
         JavaTckSupport.check(divested != null && divested.size() == 2
               && divested.contains(oldValue) && divested.contains(oldPrivilege),
            "Divestiture If Wanted did not return both transferred attributes");
         await(newOwner, callbackModel, () -> !newCallbacks.acquisitions.isEmpty(),
            "delete-privilege acquisition notification");
         OwnershipEvent acquisition = newCallbacks.acquisitions.get(0);
         JavaTckSupport.check(object.equals(acquisition.object)
               && acquisition.attributes.size() == 2
               && acquisition.attributes.contains(newValue)
               && acquisition.attributes.contains(newPrivilege)
               && Arrays.equals(divestitureTag, acquisition.tag),
            "delete-privilege acquisition notification returned the wrong object, attributes, or tag");
         JavaTckSupport.check(!oldOwner.isAttributeOwnedByFederate(object, oldPrivilege)
               && newOwner.isAttributeOwnedByFederate(object, newPrivilege),
            "delete privilege ownership did not transfer to the acquirer");
         JavaTckSupport.check(!oldOwner.isAttributeOwnedByFederate(object, oldValue)
               && newOwner.isAttributeOwnedByFederate(object, newValue),
            "ordinary attribute ownership did not transfer with the delete privilege");
         byte[] valueBytes = {0x25, 0x07};
         AttributeHandleValueMap oldUpdate = oldOwner.getAttributeHandleValueMapFactory().create(1);
         oldUpdate.put(oldValue, valueBytes.clone());
         AttributeHandleValueMap newUpdate = newOwner.getAttributeHandleValueMapFactory().create(1);
         newUpdate.put(newValue, valueBytes.clone());
         JavaTckSupport.expectFailure(
            () -> oldOwner.updateAttributeValues(object, oldUpdate, acquisitionTag.clone()),
            "AttributeNotOwned",
            "former ordinary-attribute owner updating after transfer");
         newOwner.updateAttributeValues(object, newUpdate, acquisitionTag.clone());

         byte[] deletionTag = {(byte) 0xd3, 0x1e, 0x25};
         JavaTckSupport.expectFailure(
            () -> oldOwner.deleteObjectInstance(object, deletionTag.clone()),
            "DeletePrivilegeNotHeld",
            "former registrant deleting after transferring delete privilege");
         JavaTckSupport.check(observerCallbacks.removals.isEmpty(),
            "failed former-owner deletion produced an object-removal callback");

         newOwner.deleteObjectInstance(object, deletionTag.clone());
         await(observer, callbackModel, () -> !observerCallbacks.removals.isEmpty(),
            "object removal after delete-privilege transfer");
         JavaTckSupport.check(observerCallbacks.removals.size() == 1,
            "delete-privilege transfer produced duplicate object removals");
         RemovalEvent removal = observerCallbacks.removals.get(0);
         JavaTckSupport.check(object.equals(removal.object)
               && Arrays.equals(deletionTag, removal.tag)
               && oldOwnerHandle.equals(removal.producer)
               && !newOwnerHandle.equals(removal.producer),
            "object removal returned the wrong object, tag, or original registrant");
      } finally {
         for (int index = 0; index < 3; ++index) {
            if (joined[index]) {
               RTIambassador ambassador = index == 0 ? oldOwner : index == 1 ? newOwner : observer;
               try {
                  ambassador.resignFederationExecution(
                     ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
               } catch (Exception ignored) { }
            }
         }
         if (created) {
            try { oldOwner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         for (int index = 0; index < 3; ++index) {
            if (connected[index]) {
               RTIambassador ambassador = index == 0 ? oldOwner : index == 1 ? newOwner : observer;
               try { ambassador.disconnect(); } catch (Exception ignored) { }
            }
         }
      }
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static final class OwnershipEvent {
      private final ObjectInstanceHandle object;
      private final AttributeHandleSet attributes;
      private final byte[] tag;

      private OwnershipEvent(ObjectInstanceHandle object, AttributeHandleSet attributes,
            byte[] tag) {
         this.object = object;
         this.attributes = attributes.clone();
         this.tag = tag.clone();
      }
   }

   private static final class RemovalEvent {
      private final ObjectInstanceHandle object;
      private final byte[] tag;
      private final FederateHandle producer;

      private RemovalEvent(ObjectInstanceHandle object, byte[] tag,
            FederateHandle producer) {
         this.object = object;
         this.tag = tag.clone();
         this.producer = producer;
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> acquisitions = new CopyOnWriteArrayList<>();
      private final List<RemovalEvent> removals = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)) {
                  acquisitions.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (byte[]) arguments[2]));
               } else if ("removeObjectInstance".equals(name) && arguments != null
                     && arguments.length == 3) {
                  removals.add(new RemovalEvent((ObjectInstanceHandle) arguments[0],
                     (byte[]) arguments[1], (FederateHandle) arguments[2]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK delete-privilege callbacks";
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
