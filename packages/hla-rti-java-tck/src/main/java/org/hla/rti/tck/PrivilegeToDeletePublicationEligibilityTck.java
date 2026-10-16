package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks the publication gate for acquiring HLAprivilegeToDeleteObject. */
final class PrivilegeToDeletePublicationEligibilityTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private PrivilegeToDeletePublicationEligibilityTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador acquirer = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder acquirerCallbacks = new Recorder();
      String federation = "java-tck-delete-privilege-publication-" + UUID.randomUUID();
      boolean[] connected = new boolean[2];
      boolean[] joined = new boolean[2];
      boolean created = false;
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel);
         connected[0] = true;
         acquirer.connect(acquirerCallbacks.proxy(), callbackModel);
         connected[1] = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-delete-privilege-owner", federation);
         joined[0] = true;
         acquirer.joinFederationExecution("java-tck-delete-privilege-acquirer", federation);
         joined[1] = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle acquirerClass = acquirer.getObjectClassHandle(objectClassName);
         AttributeHandle ownerValue = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle acquirerValue = acquirer.getAttributeHandle(acquirerClass, attributeName);
         AttributeHandle ownerPrivilege = owner.getAttributeHandle(ownerClass,
            "HLAprivilegeToDeleteObject");
         AttributeHandle acquirerPrivilege = acquirer.getAttributeHandle(acquirerClass,
            "HLAprivilegeToDeleteObject");
         JavaTckSupport.check(objectClassName.equals(owner.getObjectClassName(ownerClass))
               && attributeName.equals(owner.getAttributeName(ownerClass, ownerValue))
               && "HLAprivilegeToDeleteObject".equals(
                  owner.getAttributeName(ownerClass, ownerPrivilege)),
            "configured object class or standard delete-privilege lookup did not round-trip");

         AttributeHandleSet ownerValues = JavaTckSupport.attributeSet(owner, ownerValue);
         AttributeHandleSet acquirerValues = JavaTckSupport.attributeSet(acquirer, acquirerValue);
         AttributeHandleSet ownerPrivileges = JavaTckSupport.attributeSet(owner, ownerPrivilege);
         AttributeHandleSet acquirerPrivileges = JavaTckSupport.attributeSet(
            acquirer, acquirerPrivilege);
         owner.publishObjectClassAttributes(ownerClass, ownerValues);
         acquirer.subscribeObjectClassAttributes(acquirerClass, acquirerValues);
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         await(acquirer, callbackModel, () -> acquirerCallbacks.discoveries.contains(object),
            "object discovery before delete-privilege acquisition");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerPrivilege)
               && !acquirer.isAttributeOwnedByFederate(object, acquirerPrivilege),
            "delete privilege did not initially belong only to the registrant");

         // Publishing an ordinary attribute implicitly publishes this
         // standard privilege. Remove only the privilege to isolate the
         // attribute-level eligibility boundary.
         acquirer.publishObjectClassAttributes(acquirerClass, acquirerValues);
         acquirer.unpublishObjectClassAttributes(acquirerClass, acquirerPrivileges);
         byte[] acquisitionTag = {(byte) 0xe1, 0x25};
         acquirerCallbacks.acquisitions.clear();
         JavaTckSupport.expectFailure(
            () -> acquirer.attributeOwnershipAcquisition(
               object, acquirerPrivileges, acquisitionTag.clone()),
            "AttributeNotPublished",
            "acquiring explicitly unpublished HLAprivilegeToDeleteObject");
         JavaTckSupport.check(acquirerCallbacks.acquisitions.isEmpty()
               && owner.isAttributeOwnedByFederate(object, ownerPrivilege)
               && !acquirer.isAttributeOwnedByFederate(object, acquirerPrivilege),
            "rejected delete-privilege acquisition notified the acquirer or changed ownership");

         acquirer.publishObjectClassAttributes(acquirerClass, acquirerPrivileges);
         acquirer.attributeOwnershipAcquisition(object, acquirerPrivileges, acquisitionTag.clone());
         AttributeHandleSet divested = owner.attributeOwnershipDivestitureIfWanted(
            object, ownerPrivileges, new byte[0]);
         JavaTckSupport.check(divested != null && divested.size() == 1
               && divested.contains(ownerPrivilege),
            "eligible Divestiture If Wanted returned the wrong privilege set");
         await(acquirer, callbackModel, () -> !acquirerCallbacks.acquisitions.isEmpty(),
            "delete-privilege acquisition notification");
         OwnershipEvent acquisition = acquirerCallbacks.acquisitions.get(0);
         JavaTckSupport.check(object.equals(acquisition.object)
               && acquisition.attributes.size() == 1
               && acquisition.attributes.contains(acquirerPrivilege)
               && acquisition.tag.length == 0,
            "eligible acquisition notification returned the wrong object, set, or tag");
         JavaTckSupport.check(!owner.isAttributeOwnedByFederate(object, ownerPrivilege)
               && acquirer.isAttributeOwnedByFederate(object, acquirerPrivilege),
            "eligible delete-privilege acquisition did not transfer ownership");
         acquirer.deleteObjectInstance(object, new byte[0]);
      } finally {
         for (int index = 0; index < 2; ++index) {
            if (joined[index]) {
               RTIambassador ambassador = index == 0 ? owner : acquirer;
               try {
                  ambassador.resignFederationExecution(
                     ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
               } catch (Exception ignored) { }
            }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         for (int index = 0; index < 2; ++index) {
            if (connected[index]) {
               RTIambassador ambassador = index == 0 ? owner : acquirer;
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

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> acquisitions = new CopyOnWriteArrayList<>();

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
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK privilege publication callbacks";
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
