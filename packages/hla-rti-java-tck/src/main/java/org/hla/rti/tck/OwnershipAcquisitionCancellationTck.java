package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Portable checks for cancellation of an ordinary ownership acquisition. */
final class OwnershipAcquisitionCancellationTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private OwnershipAcquisitionCancellationTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String attributeName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      String federation = "java-tck-ownership-acquisition-cancellation-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean requesterConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean requesterJoined = false;
      boolean ownerPublished = false;
      boolean requesterPublished = false;
      boolean requesterSubscribed = false;
      ObjectClassHandle ownerClass = null;
      ObjectClassHandle requesterClass = null;
      AttributeHandleSet ownerAttributes = null;
      AttributeHandleSet requesterAttributes = null;
      ObjectInstanceHandle object = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         requester.connect(requesterRecorder.proxy(), callbackModel);
         requesterConnected = true;
         owner.createFederationExecution(federation, fom);
         created = true;
         owner.joinFederationExecution("java-tck-acquisition-cancel-owner", federation);
         ownerJoined = true;
         requester.joinFederationExecution("java-tck-acquisition-cancel-requester", federation);
         requesterJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         requesterClass = requester.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle requesterAttribute = requester.getAttributeHandle(
            requesterClass, attributeName);
         JavaTckSupport.check(ownerClass != null && requesterClass != null
               && ownerAttribute != null && requesterAttribute != null,
            "adapter FOM did not resolve the ownership cancellation class and attribute");
         AttributeHandleSet ownerAttributeSet = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet requesterAttributeSet = JavaTckSupport.attributeSet(
            requester, requesterAttribute);
         ownerAttributes = ownerAttributeSet;
         requesterAttributes = requesterAttributeSet;

         owner.publishObjectClassAttributes(ownerClass, ownerAttributeSet);
         ownerPublished = true;
         requester.subscribeObjectClassAttributes(requesterClass, requesterAttributeSet);
         requesterSubscribed = true;
         ObjectInstanceHandle registeredObject = owner.registerObjectInstance(ownerClass);
         object = registeredObject;
         JavaTckSupport.check(registeredObject != null,
            "ownership acquisition cancellation returned a null object handle");
         await(requester, callbackModel,
            () -> requesterRecorder.discoveries.contains(registeredObject),
            "ownership acquisition cancellation discovery");
         requester.publishObjectClassAttributes(requesterClass, requesterAttributeSet);
         requesterPublished = true;

         byte[] acquisitionTag = new byte[] {(byte) 0xC0, (byte) 0xDE, 0x25};
         JavaTckSupport.expectFailure(
            () -> requester.cancelAttributeOwnershipAcquisition(
               registeredObject, requesterAttributeSet),
            "AttributeAcquisitionWasNotRequested",
            "canceling ownership acquisition before requesting it");
         JavaTckSupport.expectFailure(
            () -> owner.cancelAttributeOwnershipAcquisition(registeredObject, ownerAttributeSet),
            "AttributeAlreadyOwned", "canceling acquisition for an owned attribute");

         ownerRecorder.clearOwnershipEvents();
         requesterRecorder.clearOwnershipEvents();
         requester.attributeOwnershipAcquisition(
            registeredObject, requesterAttributeSet, acquisitionTag);
         requester.cancelAttributeOwnershipAcquisition(registeredObject, requesterAttributeSet);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(requesterRecorder.cancellations.isEmpty(),
               "evoked ownership-cancellation callback ran before callback servicing");
         }
         await(requester, callbackModel, () -> !requesterRecorder.cancellations.isEmpty(),
            "ownership acquisition cancellation confirmation");
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(requester);
         }
         JavaTckSupport.check(requesterRecorder.cancellations.size() == 1,
            "ownership acquisition cancellation produced duplicate confirmations");
         OwnershipEvent cancellation = requesterRecorder.cancellations.get(0);
         JavaTckSupport.check(registeredObject.equals(cancellation.object)
               && requesterAttributeSet.equals(cancellation.attributes),
            "ownership acquisition cancellation returned the wrong object or attribute set");
         JavaTckSupport.check(requesterRecorder.acquisitions.isEmpty()
               && owner.isAttributeOwnedByFederate(registeredObject, ownerAttribute)
               && !requester.isAttributeOwnedByFederate(registeredObject, requesterAttribute),
            "canceled acquisition transferred ownership or delivered acquisition notification");
         JavaTckSupport.expectFailure(
            () -> requester.cancelAttributeOwnershipAcquisition(
               registeredObject, requesterAttributeSet),
            "AttributeAcquisitionWasNotRequested",
            "repeating ownership cancellation after its confirmation");
      } finally {
         if (requesterJoined && requesterPublished && requesterClass != null
               && requesterAttributes != null) {
            try { requester.unpublishObjectClassAttributes(requesterClass, requesterAttributes); }
            catch (Exception ignored) { }
         }
         if (requesterJoined && requesterSubscribed && requesterClass != null
               && requesterAttributes != null) {
            try { requester.unsubscribeObjectClassAttributes(requesterClass, requesterAttributes); }
            catch (Exception ignored) { }
         }
         if (ownerJoined && ownerPublished && ownerClass != null && ownerAttributes != null) {
            try { owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes); }
            catch (Exception ignored) { }
         }
         if (requesterJoined) {
            try { requester.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (requesterConnected) { try { requester.disconnect(); } catch (Exception ignored) { } }
         if (ownerConnected) { try { owner.disconnect(); } catch (Exception ignored) { } }
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

      private OwnershipEvent(ObjectInstanceHandle object, AttributeHandleSet attributes) {
         this.object = object;
         this.attributes = attributes;
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> cancellations = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> acquisitions = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("confirmAttributeOwnershipAcquisitionCancellation".equals(name)
                     && arguments != null && arguments.length >= 2) {
                  cancellations.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1]));
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)
                     && arguments != null && arguments.length >= 2) {
                  acquisitions.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK ownership cancellation callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }

      private void clearOwnershipEvents() {
         cancellations.clear();
         acquisitions.clear();
      }
   }
}
