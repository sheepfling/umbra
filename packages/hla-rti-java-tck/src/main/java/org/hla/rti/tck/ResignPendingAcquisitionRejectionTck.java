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
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Rejects unconditional resignation with a pending acquisition, then cancels it cleanly. */
final class ResignPendingAcquisitionRejectionTck {
   private ResignPendingAcquisitionRejectionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador requester = factory.getRtiAmbassador();
      RTIambassador owner = factory.getRtiAmbassador();
      Recorder requesterCallbacks = new Recorder();
      Recorder ownerCallbacks = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-resign-pending-acquisition-" + suffix;
      String requesterName = "java-tck-pending-requester-" + suffix;
      boolean requesterConnected = false;
      boolean ownerConnected = false;
      boolean created = false;
      boolean requesterJoined = false;
      boolean ownerJoined = false;
      try {
         requester.connect(requesterCallbacks.proxy(), callbackModel);
         requesterConnected = true;
         owner.connect(ownerCallbacks.proxy(), callbackModel);
         ownerConnected = true;
         requester.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         requester.joinFederationExecution(requesterName, federation);
         requesterJoined = true;
         owner.joinFederationExecution("java-tck-pending-owner-" + suffix, federation);
         ownerJoined = true;

         ObjectClassHandle requesterClass = requester.getObjectClassHandle(objectClassName);
         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         JavaTckSupport.check(requesterClass != null && ownerClass != null
               && requesterClass.equals(ownerClass),
            "adapter FOM did not resolve the same pending-ownership object class");
         AttributeHandle requesterAttribute = requester.getAttributeHandle(
            requesterClass, attributeName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         JavaTckSupport.check(requesterClass != null && ownerClass != null
               && requesterAttribute != null && ownerAttribute != null
               && requesterClass.equals(ownerClass)
               && requesterAttribute.equals(ownerAttribute),
            "adapter FOM did not resolve matching object and attribute handles");
         AttributeHandleSet requesterAttributes =
            JavaTckSupport.attributeSet(requester, requesterAttribute);
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);

         requester.subscribeObjectClassAttributes(requesterClass, requesterAttributes);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "object registration returned a null handle");
         await(requester, callbackModel,
            () -> requesterCallbacks.discoveries.contains(object),
            "requester did not discover the owner's object");
         requester.publishObjectClassAttributes(requesterClass, requesterAttributes);

         FederateHandle requesterHandle = requester.getFederateHandle(requesterName);
         byte[] acquisitionTag = {(byte) 0xd2, 0x25};
         ownerCallbacks.releaseRequests.clear();
         requester.attributeOwnershipAcquisition(object, requesterAttributes, acquisitionTag);
         JavaTckSupport.check(requesterCallbacks.acquisitions.isEmpty()
               && owner.isAttributeOwnedByFederate(object, ownerAttribute),
            "pending acquisition transferred ownership before resignation");

         JavaTckSupport.expectFailure(
            () -> requester.resignFederationExecution(
               ResignAction.UNCONDITIONALLY_DIVEST_ATTRIBUTES),
            "OwnershipAcquisitionPending",
            "unconditional resignation with a pending ownership acquisition");
         JavaTckSupport.check(requesterHandle.equals(requester.getFederateHandle(requesterName)),
            "requester left the federation after unconditional resignation was rejected");

         requester.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         requesterJoined = false;
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute),
            "cancelling the acquisition during resignation changed current ownership");
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(owner);
            JavaTckSupport.check(ownerCallbacks.releaseRequests.isEmpty(),
               "cancellation during resignation delivered a stale owner-release callback");
         }

         owner.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         ownerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (requesterJoined) {
            resign(requester, ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         }
         if (ownerJoined) resign(owner, ResignAction.DELETE_OBJECTS);
         if (created) {
            try { requester.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (ownerConnected) { try { owner.disconnect(); } catch (Exception ignored) { } }
         if (requesterConnected) { try { requester.disconnect(); } catch (Exception ignored) { } }
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
      private final List<ObjectInstanceHandle> acquisitions = new CopyOnWriteArrayList<>();
      private final List<ObjectInstanceHandle> releaseRequests = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)
                     && arguments != null && arguments.length >= 1) {
                  acquisitions.add((ObjectInstanceHandle) arguments[0]);
               } else if ("requestAttributeOwnershipRelease".equals(name)
                     && arguments != null && arguments.length >= 1) {
                  releaseRequests.add((ObjectInstanceHandle) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK pending-acquisition callbacks";
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
