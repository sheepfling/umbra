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
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks publication fences and exact ownership subsets for overlapping requests. */
final class OwnershipAcquisitionPublicationFenceTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private OwnershipAcquisitionPublicationFenceTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String firstAttributeName, String secondAttributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      String federation = "java-tck-ownership-acquisition-publication-fence-" + UUID.randomUUID();
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
      AttributeHandleSet ownerFirstOnly = null;
      AttributeHandleSet ownerSecondOnly = null;
      AttributeHandleSet requesterFirstOnly = null;
      AttributeHandleSet requesterSecondOnly = null;
      AttributeHandleSet requesterAttributes = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         requester.connect(requesterRecorder.proxy(), callbackModel);
         requesterConnected = true;
         owner.createFederationExecution(federation, fom);
         created = true;
         owner.joinFederationExecution("java-tck-publication-fence-owner", federation);
         ownerJoined = true;
         requester.joinFederationExecution("java-tck-publication-fence-requester", federation);
         requesterJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         requesterClass = requester.getObjectClassHandle(objectClassName);
          JavaTckSupport.check(ownerClass != null && requesterClass != null,
             "adapter multi-attribute FOM returned an invalid object-class handle");
         AttributeHandle ownerFirst = owner.getAttributeHandle(ownerClass, firstAttributeName);
         AttributeHandle ownerSecond = owner.getAttributeHandle(ownerClass, secondAttributeName);
         AttributeHandle requesterFirst = requester.getAttributeHandle(
            requesterClass, firstAttributeName);
         AttributeHandle requesterSecond = requester.getAttributeHandle(
            requesterClass, secondAttributeName);
          JavaTckSupport.check(ownerFirst != null && ownerSecond != null
               && requesterFirst != null && requesterSecond != null
                && !ownerFirst.equals(ownerSecond) && !requesterFirst.equals(requesterSecond),
            "adapter multi-attribute FOM did not resolve two distinct attributes");
         ownerFirstOnly = JavaTckSupport.attributeSet(owner, ownerFirst);
         ownerSecondOnly = JavaTckSupport.attributeSet(owner, ownerSecond);
         requesterFirstOnly = JavaTckSupport.attributeSet(requester, requesterFirst);
         requesterSecondOnly = JavaTckSupport.attributeSet(requester, requesterSecond);
         requesterAttributes = JavaTckSupport.attributeSet(
            requester, requesterFirst, requesterSecond);
         final AttributeHandleSet requesterFirstSet = requesterFirstOnly;
         final AttributeHandleSet requesterSecondSet = requesterSecondOnly;
         final AttributeHandleSet ownerSecondSet = ownerSecondOnly;
         final ObjectClassHandle requesterObjectClass = requesterClass;

         owner.publishObjectClassAttributes(ownerClass, ownerFirstOnly);
         ownerPublished = true;
         requester.subscribeObjectClassAttributes(requesterClass, requesterAttributes);
         requesterSubscribed = true;
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
          JavaTckSupport.check(object != null,
             "ownership acquisition registration returned an invalid object handle");
         await(requester, callbackModel, () -> requesterRecorder.discoveries.contains(object),
            "ownership acquisition object discovery");

         byte[] initialTag = new byte[] {0x11, 0x22};
         byte[] acquisitionTag = new byte[] {0x33, 0x44, 0x55};
         byte[] denialTag = new byte[] {0x66, 0x77};
         JavaTckSupport.expectFailure(
            () -> requester.attributeOwnershipAcquisition(
               object, requesterSecondSet, initialTag),
            "ObjectClassNotPublished", "acquiring an unpublished ownership attribute");
         requester.publishObjectClassAttributes(requesterClass, requesterAttributes);
         requesterPublished = true;

         AttributeHandle invalidAttribute = JavaTckSupport.invalidHandle(AttributeHandle.class);
         AttributeHandleSet invalidAttributes = JavaTckSupport.attributeSet(
            requester, invalidAttribute);
         JavaTckSupport.expectFailure(
            () -> requester.attributeOwnershipAcquisition(
               object, invalidAttributes, acquisitionTag),
            "AttributeNotDefined", "acquiring an undefined ownership attribute");
         JavaTckSupport.expectFailure(
            () -> owner.attributeOwnershipReleaseDenied(object, ownerSecondSet, denialTag),
            "AttributeNotOwned", "denying release for an attribute not owned by the owner");

         ownerRecorder.clearOwnershipEvents();
         requesterRecorder.clearOwnershipEvents();
         requester.attributeOwnershipAcquisitionIfAvailable(object, requesterFirstOnly, initialTag);
         requester.attributeOwnershipAcquisition(object, requesterAttributes, acquisitionTag);
         JavaTckSupport.expectFailure(
            () -> requester.attributeOwnershipAcquisitionIfAvailable(
               object, requesterFirstSet, initialTag),
            "AttributeAlreadyBeingAcquired",
            "repeating If Available after regular acquisition began");

         await(owner, callbackModel, () -> !ownerRecorder.releaseRequests.isEmpty(),
            "ownership release request");
         JavaTckSupport.drain(owner);
         JavaTckSupport.check(ownerRecorder.releaseRequests.size() == 1,
            "mixed acquisition returned an unexpected release-request count");
         OwnershipEvent release = ownerRecorder.releaseRequests.get(0);
         checkEvent(release, object, ownerFirstOnly, acquisitionTag,
            "ownership release request for the actually owned subset");
         await(requester, callbackModel, () -> !requesterRecorder.acquisitions.isEmpty(),
            "direct acquisition of the unowned subset");
         JavaTckSupport.drain(requester);
         JavaTckSupport.check(requesterRecorder.acquisitions.size() == 1,
            "mixed acquisition returned an unexpected acquisition-notification count");
         OwnershipEvent acquisition = requesterRecorder.acquisitions.get(0);
         checkEvent(acquisition, object, requesterSecondOnly, acquisitionTag,
            "direct acquisition of the unowned subset");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerFirst)
               && !requester.isAttributeOwnedByFederate(object, requesterFirst)
               && requester.isAttributeOwnedByFederate(object, requesterSecond),
            "mixed acquisition established the wrong ownership split");

         requester.unpublishObjectClassAttributes(requesterClass, requesterSecondOnly);
         JavaTckSupport.expectFailure(
            () -> requester.unpublishObjectClassAttributes(
               requesterObjectClass, requesterFirstSet),
            "OwnershipAcquisitionPending",
            "unpublishing an attribute with a pending acquisition");

         requesterRecorder.unavailable.clear();
         owner.attributeOwnershipReleaseDenied(object, ownerFirstOnly, denialTag);
         await(requester, callbackModel, () -> !requesterRecorder.unavailable.isEmpty(),
            "release denial notification");
         JavaTckSupport.drain(requester);
         JavaTckSupport.check(requesterRecorder.unavailable.size() == 1,
            "release denial returned an unexpected unavailable-notification count");
         checkEvent(requesterRecorder.unavailable.get(0), object, requesterFirstOnly, denialTag,
            "release denial notification");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerFirst)
               && !requester.isAttributeOwnedByFederate(object, requesterFirst),
            "denying the request changed ownership");
         requester.unpublishObjectClassAttributes(requesterClass, requesterFirstOnly);
      } finally {
         if (requesterJoined && requesterSubscribed && requesterClass != null
               && requesterAttributes != null) {
            try { requester.unsubscribeObjectClassAttributes(requesterClass, requesterAttributes); }
            catch (Exception ignored) { }
         }
         if (requesterJoined && requesterPublished && requesterClass != null
               && requesterAttributes != null) {
            try { requester.unpublishObjectClassAttributes(requesterClass, requesterAttributes); }
            catch (Exception ignored) { }
         }
         if (ownerJoined && ownerPublished && ownerClass != null && ownerFirstOnly != null) {
            try { owner.unpublishObjectClassAttributes(ownerClass, ownerFirstOnly); }
            catch (Exception ignored) { }
         }
         if (requesterJoined) {
             try { requester.resignFederationExecution(ResignAction.NO_ACTION); }
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
         if (requesterConnected) { try { requester.disconnect(); } catch (Exception ignored) { } }
         if (ownerConnected) { try { owner.disconnect(); } catch (Exception ignored) { } }
      }
   }

   private static void checkEvent(OwnershipEvent event, ObjectInstanceHandle object,
         AttributeHandleSet attributes, byte[] tag, String description) {
      JavaTckSupport.check(object.equals(event.object) && attributes.equals(event.attributes)
            && Arrays.equals(tag, event.tag),
         description + " returned the wrong object, attribute subset, or tag");
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
         this.attributes = attributes;
         this.tag = tag == null ? null : tag.clone();
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> releaseRequests = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> acquisitions = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> unavailable = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("requestAttributeOwnershipRelease".equals(name)
                     && arguments != null && arguments.length == 3) {
                  releaseRequests.add(event(arguments));
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)
                     && arguments != null && arguments.length == 3) {
                  acquisitions.add(event(arguments));
               } else if ("attributeOwnershipUnavailable".equals(name)
                     && arguments != null && arguments.length == 3) {
                  unavailable.add(event(arguments));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK ownership publication callbacks";
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
         acquisitions.clear();
         unavailable.clear();
      }

      private static OwnershipEvent event(Object[] arguments) {
         return new OwnershipEvent((ObjectInstanceHandle) arguments[0],
            (AttributeHandleSet) arguments[1], (byte[]) arguments[2]);
      }
   }
}
