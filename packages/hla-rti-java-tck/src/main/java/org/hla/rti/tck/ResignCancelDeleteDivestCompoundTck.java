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
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies the ordered cancellation, deletion, and divestiture resignation effects. */
final class ResignCancelDeleteDivestCompoundTck {
   private ResignCancelDeleteDivestCompoundTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador departing = factory.getRtiAmbassador();
      RTIambassador successor = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder departingCallbacks = new Recorder();
      Recorder successorCallbacks = new Recorder();
      boolean[] connected = new boolean[3];
      boolean[] joined = new boolean[3];
      boolean created = false;
      String federation = "java-tck-resign-cancel-delete-divest-" + UUID.randomUUID();
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel);
         connected[0] = true;
         departing.connect(departingCallbacks.proxy(), callbackModel);
         connected[1] = true;
         successor.connect(successorCallbacks.proxy(), callbackModel);
         connected[2] = true;

         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle ownerHandle = owner.joinFederationExecution(
            "java-tck-compound-owner", federation);
         joined[0] = true;
         FederateHandle departingHandle = departing.joinFederationExecution(
            "java-tck-compound-departing", federation);
         joined[1] = true;
         successor.joinFederationExecution("java-tck-compound-next", federation);
         joined[2] = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle departingClass = departing.getObjectClassHandle(objectClassName);
         ObjectClassHandle successorClass = successor.getObjectClassHandle(objectClassName);
         JavaTckSupport.check(ownerClass != null && departingClass != null
               && successorClass != null && ownerClass.equals(departingClass)
               && ownerClass.equals(successorClass),
            "adapter FOM did not resolve the same object class in all federates");
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle departingAttribute = departing.getAttributeHandle(
            departingClass, attributeName);
         AttributeHandle successorAttribute = successor.getAttributeHandle(
            successorClass, attributeName);
         JavaTckSupport.check(ownerAttribute != null
               && departingAttribute != null && successorAttribute != null,
            "adapter FOM did not resolve the class and attribute in all federates");
         JavaTckSupport.check(ownerClass.equals(departingClass)
               && ownerClass.equals(successorClass)
               && ownerAttribute.equals(departingAttribute)
               && ownerAttribute.equals(successorAttribute),
            "class or attribute handles changed across joined federates");

         AttributeHandleSet[] attributes = {
            JavaTckSupport.attributeSet(owner, ownerAttribute),
            JavaTckSupport.attributeSet(departing, departingAttribute),
            JavaTckSupport.attributeSet(successor, successorAttribute)
         };
         owner.publishObjectClassAttributes(ownerClass, attributes[0]);
         departing.publishObjectClassAttributes(departingClass, attributes[1]);
         successor.publishObjectClassAttributes(successorClass, attributes[2]);
         owner.subscribeObjectClassAttributes(ownerClass, attributes[0]);
         departing.subscribeObjectClassAttributes(departingClass, attributes[1]);
         successor.subscribeObjectClassAttributes(successorClass, attributes[2]);

         ObjectInstanceHandle retainedObject = owner.registerObjectInstance(ownerClass);
         ObjectInstanceHandle pendingObject = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(retainedObject != null && pendingObject != null
               && !retainedObject.equals(pendingObject),
            "owner registrations did not produce distinct valid object handles");
         String retainedName = owner.getObjectInstanceName(retainedObject);
         String pendingName = owner.getObjectInstanceName(pendingObject);
         JavaTckSupport.check(retainedName != null && !retainedName.isEmpty()
               && pendingName != null && !pendingName.isEmpty()
               && !retainedName.equals(pendingName),
            "owner registrations did not produce distinct valid object names");
         await(departing, callbackModel, () -> departingCallbacks.discoveries().containsAll(
               List.of(retainedObject, pendingObject)),
            "departing federate did not discover both owner objects");
         await(successor, callbackModel, () -> successorCallbacks.discoveries().containsAll(
               List.of(retainedObject, pendingObject)),
            "successor did not discover both owner objects");

         owner.unconditionalAttributeOwnershipDivestiture(
            retainedObject, attributes[0], new byte[0]);
         byte[] retainedTag = {(byte) 0xd7, 0x25, 0x31};
         departing.attributeOwnershipAcquisitionIfAvailable(
            retainedObject, attributes[1], retainedTag);
         await(departing, callbackModel, () -> !departingCallbacks.acquisitions().isEmpty(),
            "departing federate did not acquire the first retained-object attribute");
         JavaTckSupport.check(departingCallbacks.acquisitions().get(0).matches(
               retainedObject, departingAttribute, retainedTag)
               && departing.isAttributeOwnedByFederate(retainedObject, departingAttribute),
            "retained-object setup acquisition had incorrect metadata or ownership");

         ObjectInstanceHandle deletedObject = departing.registerObjectInstance(departingClass);
         JavaTckSupport.check(deletedObject != null,
            "departing registration returned a null object handle");
         String deletedName = departing.getObjectInstanceName(deletedObject);
         await(owner, callbackModel, () -> ownerCallbacks.discoveries().contains(deletedObject),
            "owner did not discover the departing federate's object");
         await(successor, callbackModel, () -> successorCallbacks.discoveries().contains(deletedObject),
            "successor did not discover the departing federate's object");
         JavaTckSupport.check(deletedName != null && !deletedName.isEmpty()
               && !deletedObject.equals(retainedObject)
               && !deletedObject.equals(pendingObject)
               && !deletedName.equals(retainedName) && !deletedName.equals(pendingName)
               && departing.isAttributeOwnedByFederate(deletedObject, departingAttribute),
            "departing registration or its initial ownership was invalid");

         departingCallbacks.clearAcquisitions();
         byte[] pendingTag = {(byte) 0xd7, 0x25, 0x32};
         departing.attributeOwnershipAcquisition(pendingObject, attributes[1], pendingTag);
         JavaTckSupport.check(departingCallbacks.acquisitions().isEmpty()
               && owner.isAttributeOwnedByFederate(pendingObject, ownerAttribute),
            "pending request unexpectedly transferred the owner's attribute");

         ownerCallbacks.clearRemovals();
         successorCallbacks.clearRemovals();
         departing.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         joined[1] = false;
         await(owner, callbackModel, () -> ownerCallbacks.removals().stream()
               .anyMatch(removal -> removal.object.equals(deletedObject)),
            "owner did not receive removal for the departing object's deletion");
         await(successor, callbackModel, () -> successorCallbacks.removals().stream()
               .anyMatch(removal -> removal.object.equals(deletedObject)),
            "successor did not receive removal for the departing object's deletion");
         JavaTckSupport.check(ownerCallbacks.removals().size() == 1
               && ownerCallbacks.removals().get(0).matches(deletedObject, departingHandle)
               && successorCallbacks.removals().size() == 1
               && successorCallbacks.removals().get(0).matches(deletedObject, departingHandle),
            "compound resignation returned incorrect or duplicate deletion metadata");
         JavaTckSupport.check(retainedObject.equals(owner.getObjectInstanceHandle(retainedName))
               && pendingObject.equals(owner.getObjectInstanceHandle(pendingName))
               && retainedObject.equals(successor.getObjectInstanceHandle(retainedName))
               && pendingObject.equals(successor.getObjectInstanceHandle(pendingName))
               && owner.isAttributeOwnedByFederate(pendingObject, ownerAttribute)
               && !owner.isAttributeOwnedByFederate(retainedObject, ownerAttribute),
            "compound resignation changed retained object identity or ownership incorrectly");
         expectFailure(() -> successor.getObjectInstanceHandle(deletedName),
            "ObjectInstanceNotKnown", "lookup of the deleted departing object");

         byte[] successorRetainedTag = {(byte) 0xd7, 0x25, 0x33};
         successor.attributeOwnershipAcquisitionIfAvailable(
            retainedObject, attributes[2], successorRetainedTag);
         await(successor, callbackModel, () -> !successorCallbacks.acquisitions().isEmpty(),
            "successor could not acquire the divested retained-object attribute");
         JavaTckSupport.check(successorCallbacks.acquisitions().get(0).matches(
               retainedObject, successorAttribute, successorRetainedTag)
               && successor.isAttributeOwnedByFederate(retainedObject, successorAttribute),
            "retained-object successor acquisition had incorrect metadata or ownership");

         owner.unconditionalAttributeOwnershipDivestiture(
            pendingObject, attributes[0], new byte[0]);
         successorCallbacks.clearAcquisitions();
         byte[] successorPendingTag = {(byte) 0xd7, 0x25, 0x34};
         successor.attributeOwnershipAcquisitionIfAvailable(
            pendingObject, attributes[2], successorPendingTag);
         await(successor, callbackModel, () -> !successorCallbacks.acquisitions().isEmpty(),
            "successor could not acquire the pending object's attribute after divestiture");
         JavaTckSupport.check(successorCallbacks.acquisitions().get(0).matches(
               pendingObject, successorAttribute, successorPendingTag)
               && !owner.isAttributeOwnedByFederate(pendingObject, ownerAttribute)
               && successor.isAttributeOwnedByFederate(pendingObject, successorAttribute),
            "pending-object successor acquisition left stale ownership or metadata");

         successorCallbacks.clearRemovals();
         owner.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         joined[0] = false;
         await(successor, callbackModel, () -> successorCallbacks.removals().size() >= 2,
            "successor did not receive cleanup removals for both owner objects");
         JavaTckSupport.check(successorCallbacks.removals().size() == 2
               && successorCallbacks.removals().stream().allMatch(removal ->
                  removal.tag.length == 0 && ownerHandle.equals(removal.producer))
               && successorCallbacks.removals().stream().filter(removal ->
                  removal.object.equals(retainedObject)).count() == 1
               && successorCallbacks.removals().stream().filter(removal ->
                  removal.object.equals(pendingObject)).count() == 1,
            "owner cleanup returned incorrect object identities or removal producers");
         successor.resignFederationExecution(ResignAction.NO_ACTION);
         joined[2] = false;
      } finally {
         if (joined[0]) resign(owner, ResignAction.DELETE_OBJECTS);
         if (joined[1]) resign(departing, ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         if (joined[2]) resign(successor, ResignAction.DELETE_OBJECTS_THEN_DIVEST);
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         for (int i = 2; i >= 0; --i) {
            if (connected[i]) {
               try { ambassador(i, owner, departing, successor).disconnect(); }
               catch (Exception ignored) { }
            }
         }
      }
   }

   private static RTIambassador ambassador(int index, RTIambassador owner,
         RTIambassador departing, RTIambassador successor) {
      return index == 0 ? owner : index == 1 ? departing : successor;
   }

   private static void resign(RTIambassador ambassador, ResignAction action) {
      try { ambassador.resignFederationExecution(action); }
      catch (Exception ignored) { }
   }

   private static void expectFailure(CheckedCall call, String expected, String operation)
         throws Exception {
      try {
         call.run();
      } catch (Exception failure) {
         JavaTckSupport.check(expected.equals(failure.getClass().getSimpleName()),
            operation + " threw " + failure.getClass().getSimpleName()
               + " instead of " + expected);
         return;
      }
      throw new AssertionError(operation + " unexpectedly succeeded");
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         BooleanSupplier condition, String failure) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(ambassador);
         } else {
            Thread.yield();
         }
      }
      JavaTckSupport.check(condition.getAsBoolean(), failure);
   }

   @FunctionalInterface
   private interface CheckedCall { void run() throws Exception; }

   private static final class Acquisition {
      private final ObjectInstanceHandle object;
      private final AttributeHandleSet attributes;
      private final byte[] tag;
      private Acquisition(ObjectInstanceHandle object, AttributeHandleSet attributes, byte[] tag) {
         this.object = object;
         this.attributes = attributes;
         this.tag = tag == null ? new byte[0] : tag.clone();
      }
      private boolean matches(ObjectInstanceHandle expectedObject, AttributeHandle attribute,
            byte[] expectedTag) {
         return expectedObject.equals(object) && attributes.size() == 1
            && attributes.contains(attribute) && Arrays.equals(tag, expectedTag);
      }
   }

   private static final class Removal {
      private final ObjectInstanceHandle object;
      private final byte[] tag;
      private final FederateHandle producer;
      private Removal(ObjectInstanceHandle object, byte[] tag, FederateHandle producer) {
         this.object = object;
         this.tag = tag == null ? new byte[0] : tag.clone();
         this.producer = producer;
      }
      private boolean matches(ObjectInstanceHandle expectedObject, FederateHandle expectedProducer) {
         return expectedObject.equals(object) && tag.length == 0
            && expectedProducer.equals(producer);
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Removal> removals = new CopyOnWriteArrayList<>();
      private final List<Acquisition> acquisitions = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("removeObjectInstance".equals(name) && arguments != null
                     && arguments.length == 3) {
                  removals.add(new Removal((ObjectInstanceHandle) arguments[0],
                     (byte[]) arguments[1], (FederateHandle) arguments[2]));
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)
                     && arguments != null && arguments.length == 3) {
                  acquisitions.add(new Acquisition((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (byte[]) arguments[2]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK compound-resignation callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == arguments[0];
               }
               return null;
            });
      }

      private List<ObjectInstanceHandle> discoveries() { return List.copyOf(discoveries); }
      private List<Removal> removals() { return List.copyOf(removals); }
      private List<Acquisition> acquisitions() { return List.copyOf(acquisitions); }
      private void clearRemovals() { removals.clear(); }
      private void clearAcquisitions() { acquisitions.clear(); }
   }
}
