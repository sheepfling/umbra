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

/** Ensures resigning cancels a pending If Available request without stale callbacks. */
final class ResignCancelIfAvailablePendingTck {
   private ResignCancelIfAvailablePendingTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder requesterCallbacks = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-resign-cancel-if-available-" + suffix;
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
         owner.joinFederationExecution("java-tck-if-available-owner-" + suffix, federation);
         ownerJoined = true;
         requester.joinFederationExecution("java-tck-if-available-requester-" + suffix,
            federation);
         requesterJoined = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle requesterClass = requester.getObjectClassHandle(objectClassName);
         JavaTckSupport.check(ownerClass != null && requesterClass != null
               && ownerClass.equals(requesterClass),
            "adapter FOM did not resolve the same If Available object class");
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
         requester.subscribeObjectClassAttributes(requesterClass, requesterAttributes);
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "object registration returned a null handle");
         await(requester, callbackModel,
            () -> requesterCallbacks.discoveries.contains(object),
            "requester did not discover the owner's object");

         requester.publishObjectClassAttributes(requesterClass, requesterAttributes);
         byte[] acquisitionTag = {(byte) 0xe5, 0x25};
         requesterCallbacks.clearOwnershipEvents();
         requester.attributeOwnershipAcquisitionIfAvailable(
            object, requesterAttributes, acquisitionTag);
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && !requester.isAttributeOwnedByFederate(object, requesterAttribute),
            "If Available request changed ownership before resignation cancellation");

         // An immediate model may already have reported the unavailable result. Reset the
         // recorder so the drain below specifically observes callbacks made stale by resign.
         requesterCallbacks.clearOwnershipEvents();
         requester.resignFederationExecution(
            ResignAction.CANCEL_PENDING_OWNERSHIP_ACQUISITIONS);
         requesterJoined = false;
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(requester);
         JavaTckSupport.check(requesterCallbacks.unavailable.isEmpty()
               && requesterCallbacks.acquisitions.isEmpty(),
            "resignation cancellation delivered a stale If Available result");
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

   private static void resign(RTIambassador ambassador, ResignAction action) {
      try { ambassador.resignFederationExecution(action); }
      catch (Exception ignored) { }
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         java.util.function.BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<ObjectInstanceHandle> unavailable = new CopyOnWriteArrayList<>();
      private final List<ObjectInstanceHandle> acquisitions = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("attributeOwnershipUnavailable".equals(name)
                     && arguments != null && arguments.length >= 1) {
                  unavailable.add((ObjectInstanceHandle) arguments[0]);
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)
                     && arguments != null && arguments.length >= 1) {
                  acquisitions.add((ObjectInstanceHandle) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK If Available resign callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }

      private void clearOwnershipEvents() {
         unavailable.clear();
         acquisitions.clear();
      }
   }
}
