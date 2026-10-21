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

/** Portable checks for denying an ordinary attribute ownership request. */
final class OwnershipReleaseDeniedTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private OwnershipReleaseDeniedTck() { }

   static void runIdle(RtiFactory factory, String fom, String objectClassName,
         String attributeName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      String federation = "java-tck-ownership-denial-idle-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean requesterConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean requesterJoined = false;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         requester.connect(requesterRecorder.proxy(), callbackModel);
         requesterConnected = true;
         owner.createFederationExecution(federation, fom);
         created = true;
         owner.joinFederationExecution("java-tck-idle-denial-owner", federation);
         ownerJoined = true;
         requester.joinFederationExecution("java-tck-idle-denial-requester", federation);
         requesterJoined = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle requesterClass = requester.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle requesterAttribute = requester.getAttributeHandle(
            requesterClass, attributeName);
         JavaTckSupport.check(ownerClass.equals(requesterClass)
               && ownerAttribute.equals(requesterAttribute),
            "idle denial fixture did not resolve matching standard handles");
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet requesterAttributes = JavaTckSupport.attributeSet(
            requester, requesterAttribute);
         requester.subscribeObjectClassAttributes(requesterClass, requesterAttributes);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         await(requester, callbackModel,
            () -> requesterRecorder.discoveries.contains(object),
            "idle ownership-denial discovery");
         requester.publishObjectClassAttributes(requesterClass, requesterAttributes);

         byte[] acquisitionTag = new byte[] {(byte) 0xC2, 0x11, 0x6D, 0x09};
         byte[] denialTag = new byte[] {(byte) 0xDA, 0x7A, 0x01};
         ownerRecorder.clearOwnershipEvents();
         requesterRecorder.clearOwnershipEvents();
         requester.attributeOwnershipAcquisition(object, requesterAttributes, acquisitionTag);
         // With immediate callbacks, await observes only a thread-safe list.
         await(owner, callbackModel,
            () -> !ownerRecorder.releaseRequests.isEmpty(),
            "idle ownership-denial owner release request");
         JavaTckSupport.check(ownerRecorder.releaseRequests.size() == 1,
            "idle ownership denial produced duplicate release requests");
         checkEvent(ownerRecorder.releaseRequests.get(0), object, ownerAttributes,
            acquisitionTag, "idle ownership-denial release request");

         owner.attributeOwnershipReleaseDenied(object, ownerAttributes, denialTag);
         await(requester, callbackModel,
            () -> !requesterRecorder.unavailable.isEmpty(),
            "idle ownership-denial unavailable callback");
         JavaTckSupport.check(requesterRecorder.unavailable.size() == 1
               && requesterRecorder.acquisitions.isEmpty(),
            "idle ownership denial duplicated unavailability or acquired ownership");
         checkEvent(requesterRecorder.unavailable.get(0), object, requesterAttributes,
            denialTag, "idle ownership-denial unavailable callback");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && !requester.isAttributeOwnedByFederate(object, requesterAttribute),
            "idle ownership denial changed the owner");
      } finally {
         if (requesterJoined) {
            try { requester.resignFederationExecution(
               ResignAction.CANCEL_PENDING_OWNERSHIP_ACQUISITIONS); }
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

   static void run(RtiFactory factory, String fom, String objectClassName,
         String attributeName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      String federation = "java-tck-ownership-release-denied-" + UUID.randomUUID();
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
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         requester.connect(requesterRecorder.proxy(), callbackModel);
         requesterConnected = true;
         owner.createFederationExecution(federation, fom);
         created = true;
         owner.joinFederationExecution("java-tck-release-denied-owner", federation);
         ownerJoined = true;
         requester.joinFederationExecution("java-tck-release-denied-requester", federation);
         requesterJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         requesterClass = requester.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle requesterAttribute = requester.getAttributeHandle(
            requesterClass, attributeName);
         JavaTckSupport.check(ownerClass != null && requesterClass != null
               && ownerAttribute != null && requesterAttribute != null,
            "adapter FOM did not resolve the ownership release-denial class and attribute");
         AttributeHandleSet ownerAttributeSet = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet requesterAttributeSet = JavaTckSupport.attributeSet(
            requester, requesterAttribute);
         ownerAttributes = ownerAttributeSet;
         requesterAttributes = requesterAttributeSet;

         owner.publishObjectClassAttributes(ownerClass, ownerAttributeSet);
         ownerPublished = true;
         requester.publishObjectClassAttributes(requesterClass, requesterAttributeSet);
         requesterPublished = true;
         requester.subscribeObjectClassAttributes(requesterClass, requesterAttributeSet);
         requesterSubscribed = true;
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "ownership release-denial registration returned a null object handle");
         await(requester, callbackModel, () -> requesterRecorder.discoveries.contains(object),
            "ownership release-denial object discovery");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && !requester.isAttributeOwnedByFederate(object, requesterAttribute),
            "initial ownership was not assigned exclusively to the registering federate");

         byte[] acquisitionTag = new byte[] {(byte) 0xA5, 0x01};
         byte[] denialTag = new byte[] {(byte) 0xD3, 0x02};
         ownerRecorder.clearOwnershipEvents();
         requesterRecorder.clearOwnershipEvents();
         requester.attributeOwnershipAcquisition(object, requesterAttributeSet, acquisitionTag);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(ownerRecorder.releaseRequests.isEmpty(),
               "evoked ownership-release callback ran before callback servicing");
         }
         await(owner, callbackModel, () -> !ownerRecorder.releaseRequests.isEmpty(),
            "ownership release request");
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(owner);
            JavaTckSupport.drain(requester);
         }
         JavaTckSupport.check(ownerRecorder.releaseRequests.size() == 1,
            "ownership acquisition produced duplicate release requests");
         OwnershipEvent releaseRequest = ownerRecorder.releaseRequests.get(0);
         checkEvent(releaseRequest, object, ownerAttributeSet, acquisitionTag,
            "ownership release request");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && !requester.isAttributeOwnedByFederate(object, requesterAttribute)
               && requesterRecorder.acquisitions.isEmpty()
               && requesterRecorder.unavailable.isEmpty(),
            "pending release request changed ownership or delivered a terminal callback");

         owner.attributeOwnershipReleaseDenied(object, ownerAttributeSet, denialTag);
         await(requester, callbackModel, () -> !requesterRecorder.unavailable.isEmpty(),
            "ownership unavailable callback after release denial");
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(requester);
         }
         JavaTckSupport.check(requesterRecorder.unavailable.size() == 1,
            "release denial produced duplicate unavailable callbacks");
         checkEvent(requesterRecorder.unavailable.get(0), object, requesterAttributeSet,
            denialTag, "ownership unavailable callback");
         JavaTckSupport.check(requesterRecorder.acquisitions.isEmpty()
               && owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && !requester.isAttributeOwnedByFederate(object, requesterAttribute),
            "release denial transferred ownership or delivered an acquisition");

         RTIambassador secondRequester = factory.getRtiAmbassador();
         Recorder secondRecorder = new Recorder();
         boolean secondConnected = false;
         boolean secondJoined = false;
         boolean secondPublished = false;
         boolean secondSubscribed = false;
         ObjectClassHandle secondClass = null;
         AttributeHandleSet secondAttributes = null;
         try {
            secondRequester.connect(secondRecorder.proxy(), callbackModel);
            secondConnected = true;
            secondRequester.joinFederationExecution("java-tck-release-denied-second", federation);
            secondJoined = true;
            secondClass = secondRequester.getObjectClassHandle(objectClassName);
            AttributeHandle secondAttribute = secondRequester.getAttributeHandle(
               secondClass, attributeName);
            JavaTckSupport.check(secondClass != null && secondAttribute != null,
               "second requester could not resolve the adapter ownership attribute");
            secondAttributes = JavaTckSupport.attributeSet(secondRequester, secondAttribute);
            secondRequester.subscribeObjectClassAttributes(secondClass, secondAttributes);
            secondSubscribed = true;
            secondRequester.publishObjectClassAttributes(secondClass, secondAttributes);
            secondPublished = true;
            await(secondRequester, callbackModel,
               () -> secondRecorder.discoveries.contains(object),
               "second ownership-release-denial requester discovery");

            ownerRecorder.clearOwnershipEvents();
            requesterRecorder.clearOwnershipEvents();
            secondRecorder.clearOwnershipEvents();
            byte[] firstFanoutTag = new byte[] {(byte) 0xA5, 0x11};
            byte[] secondFanoutTag = new byte[] {(byte) 0xA5, 0x22};
            byte[] fanoutDenialTag = new byte[] {(byte) 0xD3, 0x44};
            requester.attributeOwnershipAcquisition(
               object, requesterAttributeSet, firstFanoutTag);
            secondRequester.attributeOwnershipAcquisition(
               object, secondAttributes, secondFanoutTag);
            await(owner, callbackModel, () -> !ownerRecorder.releaseRequests.isEmpty(),
               "multi-acquirer ownership release request");
            owner.attributeOwnershipReleaseDenied(object, ownerAttributeSet, fanoutDenialTag);
            await(requester, callbackModel, () -> !requesterRecorder.unavailable.isEmpty(),
               "first multi-acquirer unavailable callback");
            await(secondRequester, callbackModel, () -> !secondRecorder.unavailable.isEmpty(),
               "second multi-acquirer unavailable callback");
            if (callbackModel == CallbackModel.HLA_EVOKED) {
               JavaTckSupport.drain(requester);
               JavaTckSupport.drain(secondRequester);
            }
            JavaTckSupport.check(requesterRecorder.unavailable.size() == 1
                  && secondRecorder.unavailable.size() == 1,
               "release denial delivered duplicate multi-acquirer unavailable callbacks");
            checkEvent(requesterRecorder.unavailable.get(0), object, requesterAttributeSet,
               fanoutDenialTag, "first multi-acquirer unavailable callback");
            checkEvent(secondRecorder.unavailable.get(0), object, secondAttributes,
               fanoutDenialTag, "second multi-acquirer unavailable callback");
            JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute)
                  && !requester.isAttributeOwnedByFederate(object, requesterAttribute)
                  && !secondRequester.isAttributeOwnedByFederate(object, secondAttribute)
                  && requesterRecorder.acquisitions.isEmpty()
                  && secondRecorder.acquisitions.isEmpty(),
               "multi-acquirer release denial changed ownership or delivered acquisition");
            requester.unpublishObjectClassAttributes(requesterClass, requesterAttributeSet);
            requesterPublished = false;
            secondRequester.unpublishObjectClassAttributes(secondClass, secondAttributes);
            secondPublished = false;
         } finally {
            if (secondJoined && secondSubscribed && secondClass != null
                  && secondAttributes != null) {
               try { secondRequester.unsubscribeObjectClassAttributes(
                  secondClass, secondAttributes); } catch (Exception ignored) { }
            }
            if (secondJoined && secondPublished && secondClass != null
                  && secondAttributes != null) {
               try { secondRequester.unpublishObjectClassAttributes(
                  secondClass, secondAttributes); } catch (Exception ignored) { }
            }
            if (secondJoined) {
               try { secondRequester.resignFederationExecution(
                  ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
               catch (Exception ignored) { }
            }
            if (secondConnected) {
               try { secondRequester.disconnect(); } catch (Exception ignored) { }
            }
         }
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
         if (ownerJoined && ownerPublished && ownerClass != null && ownerAttributes != null) {
            try { owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes); }
            catch (Exception ignored) { }
         }
         if (requesterJoined) {
            try { requester.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
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

   private static void checkEvent(OwnershipEvent event, ObjectInstanceHandle object,
         AttributeHandleSet attributes, byte[] tag, String description) {
      JavaTckSupport.check(object.equals(event.object) && attributes.equals(event.attributes)
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
      private final List<OwnershipEvent> unavailable = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> acquisitions = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if (("requestAttributeOwnershipRelease".equals(name)
                     || "attributeOwnershipUnavailable".equals(name)
                     || "attributeOwnershipAcquisitionNotification".equals(name))
                     && arguments != null && arguments.length >= 3) {
                  OwnershipEvent event = new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (byte[]) arguments[2]);
                  if ("requestAttributeOwnershipRelease".equals(name)) {
                     releaseRequests.add(event);
                  } else if ("attributeOwnershipUnavailable".equals(name)) {
                     unavailable.add(event);
                  } else {
                     acquisitions.add(event);
                  }
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK ownership release callbacks";
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
         unavailable.clear();
         acquisitions.clear();
      }
   }
}
