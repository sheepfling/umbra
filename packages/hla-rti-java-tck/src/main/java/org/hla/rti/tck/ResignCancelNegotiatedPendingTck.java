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

/** Cancels pending negotiated ownership work on resignation without stale owner callbacks. */
final class ResignCancelNegotiatedPendingTck {
   private ResignCancelNegotiatedPendingTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder requesterCallbacks = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-resign-cancel-negotiated-" + suffix;
      boolean ownerConnected = false;
      boolean requesterConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean requesterJoined = false;
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel);
         ownerConnected = true;
         requester.connect(requesterCallbacks.proxy(), callbackModel);
         requesterConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-negotiated-owner-" + suffix, federation);
         ownerJoined = true;
         requester.joinFederationExecution("java-tck-negotiated-requester-" + suffix,
            federation);
         requesterJoined = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle requesterClass = requester.getObjectClassHandle(objectClassName);
         JavaTckSupport.check(ownerClass != null && requesterClass != null
               && ownerClass.equals(requesterClass),
            "adapter FOM did not resolve the same negotiated-ownership object class");
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle requesterAttribute = requester.getAttributeHandle(
            requesterClass, attributeName);
         JavaTckSupport.check(ownerClass != null && requesterClass != null
               && ownerAttribute != null && requesterAttribute != null
               && ownerClass.equals(requesterClass) && ownerAttribute.equals(requesterAttribute),
            "adapter FOM did not resolve matching object and attribute handles");
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet requesterAttributes =
            JavaTckSupport.attributeSet(requester, requesterAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         requester.publishObjectClassAttributes(requesterClass, requesterAttributes);
         requester.subscribeObjectClassAttributes(requesterClass, requesterAttributes);
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "object registration returned a null handle");
         await(requester, callbackModel, objectClassName,
            () -> requesterCallbacks.discoveries.contains(object),
            "requester did not discover the owner's object");

         ownerCallbacks.clearOwnershipEvents();
         requesterCallbacks.clearOwnershipEvents();
         byte[] acquisitionTag = {(byte) 0xe6, 0x25};
         byte[] divestitureTag = {(byte) 0xe7, 0x25};
         requester.attributeOwnershipAcquisition(object, requesterAttributes, acquisitionTag);
         owner.negotiatedAttributeOwnershipDivestiture(object, ownerAttributes, divestitureTag);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(ownerCallbacks.divestitureConfirmations.isEmpty()
                  && ownerCallbacks.releaseRequests.isEmpty(),
               "evoked negotiated resignation crossed the callback boundary before cancellation");
         }
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && !requester.isAttributeOwnedByFederate(object, requesterAttribute),
            "negotiated ownership request changed ownership before resignation cancellation");

         // Clear any result already delivered by an immediate callback model so the
         // following callback-service passes check only for stale post-resignation work.
         ownerCallbacks.clearOwnershipEvents();
         requester.resignFederationExecution(
            ResignAction.CANCEL_PENDING_OWNERSHIP_ACQUISITIONS);
         requesterJoined = false;
         serviceOwner(owner, callbackModel, objectClassName);
         JavaTckSupport.check(ownerCallbacks.divestitureConfirmations.isEmpty()
               && ownerCallbacks.releaseRequests.isEmpty(),
            "resignation cancellation delivered a stale negotiated-ownership callback");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute),
            "resignation cancellation changed the current owner");

         owner.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         ownerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (requesterJoined) {
            resign(requester, ResignAction.CANCEL_PENDING_OWNERSHIP_ACQUISITIONS);
         }
         if (ownerJoined) resign(owner, ResignAction.DELETE_OBJECTS);
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (requesterConnected) { try { requester.disconnect(); } catch (Exception ignored) { } }
         if (ownerConnected) { try { owner.disconnect(); } catch (Exception ignored) { } }
      }
   }

   private static void serviceOwner(RTIambassador owner, CallbackModel callbackModel,
         String objectClassName) throws Exception {
      for (int i = 0; i < 8; ++i) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(owner);
         } else {
            owner.getObjectClassHandle(objectClassName);
            Thread.yield();
         }
      }
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         String objectClassName, java.util.function.BooleanSupplier condition,
         String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else ambassador.getObjectClassHandle(objectClassName);
         Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void resign(RTIambassador ambassador, ResignAction action) {
      try { ambassador.resignFederationExecution(action); }
      catch (Exception ignored) { }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<ObjectInstanceHandle> divestitureConfirmations =
         new CopyOnWriteArrayList<>();
      private final List<ObjectInstanceHandle> releaseRequests = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("requestDivestitureConfirmation".equals(name)
                     && arguments != null && arguments.length >= 1) {
                  divestitureConfirmations.add((ObjectInstanceHandle) arguments[0]);
               } else if ("requestAttributeOwnershipRelease".equals(name)
                     && arguments != null && arguments.length >= 1) {
                  releaseRequests.add((ObjectInstanceHandle) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK negotiated-resign callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }

      private void clearOwnershipEvents() {
         divestitureConfirmations.clear();
         releaseRequests.clear();
      }
   }
}
