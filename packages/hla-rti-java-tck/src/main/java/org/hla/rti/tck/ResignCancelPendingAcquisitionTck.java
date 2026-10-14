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
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies resignation cancels a pending ownership request without changing its object. */
final class ResignCancelPendingAcquisitionTck {
   private ResignCancelPendingAcquisitionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      RTIambassador successor = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder requesterCallbacks = new Recorder();
      Recorder successorCallbacks = new Recorder();
      boolean[] connected = new boolean[3];
      boolean[] joined = new boolean[3];
      boolean created = false;
      String federation = "java-tck-resign-cancel-acquire-" + UUID.randomUUID();
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel);
         connected[0] = true;
         requester.connect(requesterCallbacks.proxy(), callbackModel);
         connected[1] = true;
         successor.connect(successorCallbacks.proxy(), callbackModel);
         connected[2] = true;

         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-cancel-owner", federation);
         joined[0] = true;
         requester.joinFederationExecution("java-tck-cancel-requester", federation);
         joined[1] = true;
         successor.joinFederationExecution("java-tck-cancel-successor", federation);
         joined[2] = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle requesterClass = requester.getObjectClassHandle(objectClassName);
         ObjectClassHandle successorClass = successor.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle requesterAttribute = requester.getAttributeHandle(
            requesterClass, attributeName);
         AttributeHandle successorAttribute = successor.getAttributeHandle(
            successorClass, attributeName);
         JavaTckSupport.check(ownerClass != null && requesterClass != null
               && successorClass != null && ownerAttribute != null
               && requesterAttribute != null && successorAttribute != null,
            "adapter FOM did not resolve the class and attribute in all federates");
         JavaTckSupport.check(ownerClass.equals(requesterClass)
               && ownerClass.equals(successorClass)
               && ownerAttribute.equals(requesterAttribute)
               && ownerAttribute.equals(successorAttribute),
            "class or attribute handles changed across joined federates");

         AttributeHandleSet[] attributes = {
            JavaTckSupport.attributeSet(owner, ownerAttribute),
            JavaTckSupport.attributeSet(requester, requesterAttribute),
            JavaTckSupport.attributeSet(successor, successorAttribute)
         };
         owner.publishObjectClassAttributes(ownerClass, attributes[0]);
         requester.publishObjectClassAttributes(requesterClass, attributes[1]);
         successor.publishObjectClassAttributes(successorClass, attributes[2]);
         requester.subscribeObjectClassAttributes(requesterClass, attributes[1]);
         successor.subscribeObjectClassAttributes(successorClass, attributes[2]);

         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         String objectName = owner.getObjectInstanceName(object);
         await(requester, callbackModel, () -> !requesterCallbacks.discoveries().isEmpty(),
            "requester did not discover the owner's object");
         await(successor, callbackModel, () -> !successorCallbacks.discoveries().isEmpty(),
            "successor did not discover the owner's object");
         JavaTckSupport.check(requesterCallbacks.discoveries().equals(List.of(object))
               && successorCallbacks.discoveries().equals(List.of(object)),
            "object discovery had incorrect identity or multiplicity");

         byte[] pendingTag = {(byte) 0xd7, 0x25, 0x21};
         requester.attributeOwnershipAcquisition(object, attributes[1], pendingTag);
         JavaTckSupport.check(requesterCallbacks.acquisitions().isEmpty()
               && owner.isAttributeOwnedByFederate(object, ownerAttribute),
            "pending acquisition unexpectedly transferred ownership before resignation");

         ownerCallbacks.releaseRequests.clear();
         requester.resignFederationExecution(
            ResignAction.CANCEL_PENDING_OWNERSHIP_ACQUISITIONS);
         joined[1] = false;
         for (int pass = 0; pass < 8; ++pass) {
            if (callbackModel == CallbackModel.HLA_EVOKED) {
               JavaTckSupport.drain(owner);
            } else {
               owner.getObjectClassHandle(objectClassName);
               Thread.yield();
            }
         }
         JavaTckSupport.check(ownerCallbacks.releaseRequests().isEmpty(),
            "canceling pending acquisition delivered a stale owner-release callback");
         JavaTckSupport.check(objectName.equals(owner.getObjectInstanceName(object))
               && object.equals(successor.getObjectInstanceHandle(objectName))
               && owner.isAttributeOwnedByFederate(object, ownerAttribute),
            "cancel-pending resignation changed object identity or ownership");

         owner.unconditionalAttributeOwnershipDivestiture(
            object, attributes[0], new byte[0]);
         byte[] successorTag = {(byte) 0xd7, 0x25, 0x22};
         successor.attributeOwnershipAcquisitionIfAvailable(
            object, attributes[2], successorTag);
         await(successor, callbackModel, () -> !successorCallbacks.acquisitions().isEmpty(),
            "successor could not acquire after the owner divested");
         Acquisition acquisition = successorCallbacks.acquisitions().get(0);
         JavaTckSupport.check(acquisition.matches(object, successorAttribute, successorTag)
               && successorCallbacks.unavailable().isEmpty()
               && !owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && successor.isAttributeOwnedByFederate(object, successorAttribute),
            "successor acquisition had incorrect callback, unavailable result, or ownership");

         owner.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         joined[0] = false;
         await(successor, callbackModel, () -> !successorCallbacks.removals().isEmpty(),
            "successor did not receive cleanup removal for the object");
         JavaTckSupport.check(successorCallbacks.removals().equals(List.of(object)),
            "cleanup removal had incorrect object identity or multiplicity");
         successor.resignFederationExecution(ResignAction.NO_ACTION);
         joined[2] = false;
      } finally {
         if (joined[0]) resign(owner, ResignAction.DELETE_OBJECTS);
         if (joined[1]) resign(requester, ResignAction.CANCEL_PENDING_OWNERSHIP_ACQUISITIONS);
         if (joined[2]) resign(successor, ResignAction.DELETE_OBJECTS_THEN_DIVEST);
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         for (int i = 2; i >= 0; --i) {
            if (connected[i]) {
               try { ambassador(i, owner, requester, successor).disconnect(); }
               catch (Exception ignored) { }
            }
         }
      }
   }

   private static RTIambassador ambassador(int index, RTIambassador owner,
         RTIambassador requester, RTIambassador successor) {
      return index == 0 ? owner : index == 1 ? requester : successor;
   }

   private static void resign(RTIambassador ambassador, ResignAction action) {
      try { ambassador.resignFederationExecution(action); }
      catch (Exception ignored) { }
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

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<ObjectInstanceHandle> removals = new CopyOnWriteArrayList<>();
      private final List<Acquisition> acquisitions = new CopyOnWriteArrayList<>();
      private final List<ObjectInstanceHandle> releaseRequests = new CopyOnWriteArrayList<>();
      private final List<ObjectInstanceHandle> unavailable = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("removeObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  removals.add((ObjectInstanceHandle) arguments[0]);
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)
                     && arguments != null && arguments.length == 3) {
                  acquisitions.add(new Acquisition((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (byte[]) arguments[2]));
               } else if ("requestAttributeOwnershipRelease".equals(name)
                     && arguments != null && arguments.length >= 1) {
                  releaseRequests.add((ObjectInstanceHandle) arguments[0]);
               } else if ("attributeOwnershipUnavailable".equals(name)
                     && arguments != null && arguments.length >= 1) {
                  unavailable.add((ObjectInstanceHandle) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK cancel-acquisition callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == arguments[0];
               }
               return null;
            });
      }

      private List<ObjectInstanceHandle> discoveries() { return List.copyOf(discoveries); }
      private List<ObjectInstanceHandle> removals() { return List.copyOf(removals); }
      private List<Acquisition> acquisitions() { return List.copyOf(acquisitions); }
      private List<ObjectInstanceHandle> releaseRequests() { return List.copyOf(releaseRequests); }
      private List<ObjectInstanceHandle> unavailable() { return List.copyOf(unavailable); }
   }
}
