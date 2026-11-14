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
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Repeated acquisition preserves already pending and canceling attribute requests. */
final class OwnershipRepeatAcquisitionPendingTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private OwnershipRepeatAcquisitionPendingTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String firstAttributeName, String secondAttributeName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      String federation = "java-tck-repeat-acquisition-pending-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean requesterConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean requesterJoined = false;
      boolean requesterCallbacksDisabled = false;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         requester.connect(requesterRecorder.proxy(), callbackModel);
         requesterConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-repeat-pending-owner", federation);
         ownerJoined = true;
         requester.joinFederationExecution("java-tck-repeat-pending-requester", federation);
         requesterJoined = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle requesterClass = requester.getObjectClassHandle(objectClassName);
         JavaTckSupport.check(ownerClass != null && requesterClass != null
               && ownerClass.equals(requesterClass),
            "adapter multi-attribute FOM did not resolve matching repeat-acquisition classes");
         AttributeHandle ownerFirst = owner.getAttributeHandle(ownerClass, firstAttributeName);
         AttributeHandle ownerSecond = owner.getAttributeHandle(ownerClass, secondAttributeName);
         AttributeHandle requesterFirst = requester.getAttributeHandle(
            requesterClass, firstAttributeName);
         AttributeHandle requesterSecond = requester.getAttributeHandle(
            requesterClass, secondAttributeName);
         JavaTckSupport.check(ownerFirst != null && ownerSecond != null
               && requesterFirst != null && requesterSecond != null
               && !ownerFirst.equals(ownerSecond)
               && ownerFirst.equals(requesterFirst) && ownerSecond.equals(requesterSecond),
            "adapter multi-attribute FOM did not resolve two distinct matching attributes");

         AttributeHandleSet ownerBoth = JavaTckSupport.attributeSet(
            owner, ownerFirst, ownerSecond);
         AttributeHandleSet ownerFirstOnly = JavaTckSupport.attributeSet(owner, ownerFirst);
         AttributeHandleSet ownerSecondOnly = JavaTckSupport.attributeSet(owner, ownerSecond);
         AttributeHandleSet requesterBoth = JavaTckSupport.attributeSet(
            requester, requesterFirst, requesterSecond);
         AttributeHandleSet requesterFirstOnly = JavaTckSupport.attributeSet(
            requester, requesterFirst);
         AttributeHandleSet requesterSecondOnly = JavaTckSupport.attributeSet(
            requester, requesterSecond);

         owner.publishObjectClassAttributes(ownerClass, ownerBoth);
         requester.publishObjectClassAttributes(requesterClass, requesterBoth);
         requester.subscribeObjectClassAttributes(requesterClass, requesterBoth);
         ObjectInstanceHandle ownerObject = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(ownerObject != null,
            "repeat-acquisition registration returned a null object handle");
         await(requester, callbackModel, () -> !requesterRecorder.discoveries.isEmpty(),
            "repeat-acquisition object discovery");
         ObjectInstanceHandle requesterObject = requesterRecorder.discoveries.get(0);
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(ownerObject, ownerFirst)
               && owner.isAttributeOwnedByFederate(ownerObject, ownerSecond),
            "owner did not initially own both repeat-acquisition attributes");

         byte[] firstTag = new byte[] {0x31, 0x41};
         byte[] repeatedTag = new byte[] {0x59, 0x26};
         byte[] cancelingTag = new byte[] {0x70, 0x71};
         byte[] repeatedWhileCancelingTag = new byte[] {0x72, 0x73};

         requester.attributeOwnershipAcquisition(
            requesterObject, requesterFirstOnly, firstTag);
         await(owner, callbackModel, () -> !ownerRecorder.releaseRequests.isEmpty(),
            "first pending acquisition release request");
         drainIfEvoked(owner, callbackModel);
         JavaTckSupport.check(ownerRecorder.releaseRequests.size() == 1,
            "first acquisition did not produce exactly one release request");
         checkEvent(ownerRecorder.releaseRequests.get(0), ownerObject, ownerFirstOnly,
            firstTag, "first pending acquisition release request");

         requester.attributeOwnershipAcquisition(requesterObject, requesterBoth, repeatedTag);
         await(owner, callbackModel, () -> ownerRecorder.releaseRequests.size() >= 2,
            "release request for newly included attribute");
         drainIfEvoked(owner, callbackModel);
         JavaTckSupport.check(ownerRecorder.releaseRequests.size() == 2,
            "repeated acquisition issued a duplicate release request for the pending attribute");
         checkEvent(ownerRecorder.releaseRequests.get(1), ownerObject, ownerSecondOnly,
            repeatedTag, "repeated acquisition release request");
         checkOwnership(owner, requester, ownerObject, requesterObject,
            ownerFirst, ownerSecond, requesterFirst, requesterSecond,
            "repeated acquisition changed ownership before either request was resolved");

         requester.cancelAttributeOwnershipAcquisition(requesterObject, requesterBoth);
         await(requester, callbackModel, () -> !requesterRecorder.cancellations.isEmpty(),
            "cancellation confirmation for both pending attributes");
         drainIfEvoked(requester, callbackModel);
         JavaTckSupport.check(requesterRecorder.cancellations.size() == 1,
            "canceling both pending acquisitions produced duplicate confirmations");
         checkEvent(requesterRecorder.cancellations.get(0), requesterObject, requesterBoth,
            null, "cancellation confirmation for both pending attributes");
         checkOwnership(owner, requester, ownerObject, requesterObject,
            ownerFirst, ownerSecond, requesterFirst, requesterSecond,
            "canceling repeated acquisition changed ownership");

         ownerRecorder.releaseRequests.clear();
         requesterRecorder.clearOwnershipEvents();
         requester.attributeOwnershipAcquisition(
            requesterObject, requesterFirstOnly, cancelingTag);
         await(owner, callbackModel, () -> !ownerRecorder.releaseRequests.isEmpty(),
            "acquisition release request before cancellation");
         drainIfEvoked(owner, callbackModel);
         JavaTckSupport.check(ownerRecorder.releaseRequests.size() == 1,
            "pre-cancellation acquisition did not produce exactly one release request");
         checkEvent(ownerRecorder.releaseRequests.get(0), ownerObject, ownerFirstOnly,
            cancelingTag, "acquisition release request before cancellation");

         if (callbackModel == CallbackModel.HLA_IMMEDIATE) {
            requester.disableCallbacks();
            requesterCallbacksDisabled = true;
         }
         requester.cancelAttributeOwnershipAcquisition(requesterObject, requesterFirstOnly);
         requester.attributeOwnershipAcquisition(
            requesterObject, requesterBoth, repeatedWhileCancelingTag);
         await(owner, callbackModel, () -> ownerRecorder.releaseRequests.size() >= 2,
            "release request for newly included attribute while cancellation is pending");
         drainIfEvoked(owner, callbackModel);
         JavaTckSupport.check(ownerRecorder.releaseRequests.size() == 2,
            "repeated acquisition replaced the canceling request or duplicated its release request");
         checkEvent(ownerRecorder.releaseRequests.get(1), ownerObject, ownerSecondOnly,
            repeatedWhileCancelingTag,
            "new attribute release request while cancellation is pending");

         if (requesterCallbacksDisabled) {
            requester.enableCallbacks();
            requesterCallbacksDisabled = false;
         }
         await(requester, callbackModel, () -> !requesterRecorder.cancellations.isEmpty(),
            "cancellation confirmation for A after repeated acquisition");
         drainIfEvoked(requester, callbackModel);
         JavaTckSupport.check(requesterRecorder.cancellations.size() == 1,
            "repeated acquisition changed A's pending cancellation state");
         checkEvent(requesterRecorder.cancellations.get(0), requesterObject,
            requesterFirstOnly, null,
            "cancellation confirmation for A after repeated acquisition");
         checkOwnership(owner, requester, ownerObject, requesterObject,
            ownerFirst, ownerSecond, requesterFirst, requesterSecond,
            "resolving A's cancellation changed ownership");

         requester.cancelAttributeOwnershipAcquisition(
            requesterObject, requesterSecondOnly);
         await(requester, callbackModel,
            () -> requesterRecorder.cancellations.size() >= 2,
            "cancellation confirmation for newly included B");
         drainIfEvoked(requester, callbackModel);
         JavaTckSupport.check(requesterRecorder.cancellations.size() == 2,
            "newly included B did not remain independently cancelable");
         checkEvent(requesterRecorder.cancellations.get(1), requesterObject,
            requesterSecondOnly, null, "cancellation confirmation for newly included B");
         checkOwnership(owner, requester, ownerObject, requesterObject,
            ownerFirst, ownerSecond, requesterFirst, requesterSecond,
            "canceling the new acquisition changed the owner's ownership");
      } finally {
         if (requesterConnected && requesterCallbacksDisabled) {
            try { requester.enableCallbacks(); } catch (Exception ignored) { }
         }
         if (requesterJoined) {
            try { requester.resignFederationExecution(
               ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (requesterConnected) {
            try { requester.disconnect(); } catch (Exception ignored) { }
         }
         if (ownerConnected) {
            try { owner.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void checkOwnership(RTIambassador owner, RTIambassador requester,
         ObjectInstanceHandle ownerObject, ObjectInstanceHandle requesterObject,
         AttributeHandle ownerFirst, AttributeHandle ownerSecond,
         AttributeHandle requesterFirst, AttributeHandle requesterSecond, String message)
         throws Exception {
      JavaTckSupport.check(owner.isAttributeOwnedByFederate(ownerObject, ownerFirst)
            && owner.isAttributeOwnedByFederate(ownerObject, ownerSecond)
            && !requester.isAttributeOwnedByFederate(requesterObject, requesterFirst)
            && !requester.isAttributeOwnedByFederate(requesterObject, requesterSecond), message);
   }

   private static void checkEvent(OwnershipEvent event, ObjectInstanceHandle object,
         AttributeHandleSet attributes, byte[] tag, String description) {
      JavaTckSupport.check(object.equals(event.object)
            && event.attributes.equals(new HashSet<>(attributes))
            && Arrays.equals(tag, event.tag),
         description + " returned the wrong object, attributes, or tag");
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

   private static void drainIfEvoked(RTIambassador ambassador, CallbackModel callbackModel)
         throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
   }

   private static final class OwnershipEvent {
      private final ObjectInstanceHandle object;
      private final java.util.Set<AttributeHandle> attributes;
      private final byte[] tag;

      private OwnershipEvent(ObjectInstanceHandle object, AttributeHandleSet attributes,
            byte[] tag) {
         this.object = object;
         this.attributes = new HashSet<>(attributes);
         this.tag = tag == null ? null : tag.clone();
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> releaseRequests = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> cancellations = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)
                     && arguments != null && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("requestAttributeOwnershipRelease".equals(name)
                     && arguments != null && arguments.length >= 3) {
                  releaseRequests.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (byte[]) arguments[2]));
               } else if ("confirmAttributeOwnershipAcquisitionCancellation".equals(name)
                     && arguments != null && arguments.length >= 2) {
                  cancellations.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], null));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK repeated acquisition callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }

      private void clearOwnershipEvents() {
         releaseRequests.clear();
         cancellations.clear();
      }
   }
}
