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

/** Continues negotiated divestiture with the next willing-to-acquire candidate. */
final class NegotiatedWillingToAcquireContinuationTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private NegotiatedWillingToAcquireContinuationTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName,
         String attributeName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador first = factory.getRtiAmbassador();
      RTIambassador second = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder firstRecorder = new Recorder();
      Recorder secondRecorder = new Recorder();
      String federation = "java-tck-negotiated-willing-continuation-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean firstConnected = false;
      boolean secondConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean firstJoined = false;
      boolean secondJoined = false;
      boolean ownerPublished = false;
      boolean firstPublished = false;
      boolean secondPublished = false;
      boolean firstSubscribed = false;
      boolean secondSubscribed = false;
      ObjectClassHandle ownerClass = null;
      ObjectClassHandle firstClass = null;
      ObjectClassHandle secondClass = null;
      AttributeHandleSet ownerAttributes = null;
      AttributeHandleSet firstAttributes = null;
      AttributeHandleSet secondAttributes = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         first.connect(firstRecorder.proxy(), callbackModel);
         firstConnected = true;
         second.connect(secondRecorder.proxy(), callbackModel);
         secondConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-wta-owner", federation);
         ownerJoined = true;
         first.joinFederationExecution("java-tck-wta-first", federation);
         firstJoined = true;
         second.joinFederationExecution("java-tck-wta-second", federation);
         secondJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         firstClass = first.getObjectClassHandle(objectClassName);
         secondClass = second.getObjectClassHandle(objectClassName);
         JavaTckSupport.check(ownerClass != null && firstClass != null && secondClass != null
               && ownerClass.equals(firstClass) && ownerClass.equals(secondClass),
            "adapter FOM did not resolve the same willing-to-acquire object class");
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle firstAttribute = first.getAttributeHandle(firstClass, attributeName);
         AttributeHandle secondAttribute = second.getAttributeHandle(secondClass, attributeName);
         JavaTckSupport.check(ownerClass != null && firstClass != null && secondClass != null
               && ownerAttribute != null && firstAttribute != null && secondAttribute != null
               && ownerAttribute.equals(firstAttribute) && ownerAttribute.equals(secondAttribute),
            "adapter FOM did not resolve matching willing-to-acquire handles");
         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         firstAttributes = JavaTckSupport.attributeSet(first, firstAttribute);
         secondAttributes = JavaTckSupport.attributeSet(second, secondAttribute);
         final AttributeHandleSet ownerAttributeSet = ownerAttributes;

         first.subscribeObjectClassAttributes(firstClass, firstAttributes);
         firstSubscribed = true;
         second.subscribeObjectClassAttributes(secondClass, secondAttributes);
         secondSubscribed = true;
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         ownerPublished = true;
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "object registration returned a null handle");
         await(first, callbackModel, () -> firstRecorder.discoveries.contains(object),
            "first willing-to-acquire candidate discovery");
         await(second, callbackModel, () -> secondRecorder.discoveries.contains(object),
            "second willing-to-acquire candidate discovery");
         first.publishObjectClassAttributes(firstClass, firstAttributes);
         firstPublished = true;
         second.publishObjectClassAttributes(secondClass, secondAttributes);
         secondPublished = true;

         byte[] firstWillingTag = new byte[] {(byte) 0x81, (byte) 0xa7, 0x25};
         byte[] firstRegularTag = new byte[] {(byte) 0x85, (byte) 0xa7, 0x25};
         byte[] secondWillingTag = new byte[] {(byte) 0x82, (byte) 0xa7, 0x25};
         byte[] divestitureTag = new byte[] {(byte) 0x83, (byte) 0xa7, 0x25};
         byte[] confirmationTag = new byte[] {(byte) 0x84, (byte) 0xa7, 0x25};
         ownerRecorder.clearOwnershipEvents();
         firstRecorder.clearOwnershipEvents();
         secondRecorder.clearOwnershipEvents();

         if (callbackModel == CallbackModel.HLA_IMMEDIATE) {
            cancelFirstCandidate(first, firstRecorder, object, firstAttributes,
               firstRegularTag, callbackModel);
            firstRecorder.clearOwnershipEvents();
            ownerRecorder.clearOwnershipEvents();
            second.attributeOwnershipAcquisition(object, secondAttributes, secondWillingTag);
         } else {
            first.attributeOwnershipAcquisitionIfAvailable(object, firstAttributes, firstWillingTag);
            second.attributeOwnershipAcquisitionIfAvailable(object, secondAttributes, secondWillingTag);
         }
         JavaTckSupport.check(firstRecorder.acquisitions.isEmpty()
               && secondRecorder.acquisitions.isEmpty(),
            "willing-to-acquire request transferred ownership before divestiture");

         owner.negotiatedAttributeOwnershipDivestiture(object, ownerAttributes, divestitureTag);
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && !first.isAttributeOwnedByFederate(object, firstAttribute)
               && !second.isAttributeOwnedByFederate(object, secondAttribute),
            "negotiated willing-to-acquire divestiture changed ownership before confirmation");
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(ownerRecorder.divestitureConfirmations.isEmpty()
                  && ownerRecorder.releaseRequests.isEmpty(),
               "evoked negotiated continuation crossed the callback boundary before cancellation");
            cancelFirstCandidate(first, firstRecorder, object, firstAttributes,
               firstRegularTag, callbackModel);
         }

         await(owner, callbackModel,
            () -> !ownerRecorder.divestitureConfirmations.isEmpty(),
            "willing-to-acquire continuation divestiture confirmation");
         OwnershipEvent confirmation = ownerRecorder.divestitureConfirmations.get(0);
         JavaTckSupport.check(object.equals(confirmation.object)
               && confirmation.attributes.equals(new HashSet<>(ownerAttributeSet))
               && Arrays.equals(secondWillingTag, confirmation.tag),
            "negotiated continuation selected the wrong candidate or tag");
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(ownerRecorder.releaseRequests.isEmpty()
                  && secondRecorder.acquisitions.isEmpty(),
               "stale first-candidate work produced an owner release or early acquisition callback");
         }

         owner.confirmDivestiture(object, ownerAttributes, confirmationTag);
         JavaTckSupport.check(!owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && !first.isAttributeOwnedByFederate(object, firstAttribute)
               && second.isAttributeOwnedByFederate(object, secondAttribute),
            "negotiated continuation did not transfer ownership to the second candidate");
         await(second, callbackModel, () -> !secondRecorder.acquisitions.isEmpty(),
            "second candidate ownership acquisition notification");
         OwnershipEvent acquisition = secondRecorder.acquisitions.get(0);
         JavaTckSupport.check(object.equals(acquisition.object)
               && acquisition.attributes.equals(new HashSet<>(secondAttributes))
               && Arrays.equals(confirmationTag, acquisition.tag),
            "continuation acquisition notification returned the wrong object, attributes, or tag");
      } finally {
         if (firstJoined && firstSubscribed && firstClass != null && firstAttributes != null) {
            try { first.unsubscribeObjectClassAttributes(firstClass, firstAttributes); }
            catch (Exception ignored) { }
         }
         if (secondJoined && secondSubscribed && secondClass != null && secondAttributes != null) {
            try { second.unsubscribeObjectClassAttributes(secondClass, secondAttributes); }
            catch (Exception ignored) { }
         }
         if (firstJoined && firstPublished && firstClass != null && firstAttributes != null) {
            try { first.unpublishObjectClassAttributes(firstClass, firstAttributes); }
            catch (Exception ignored) { }
         }
         if (secondJoined && secondPublished && secondClass != null && secondAttributes != null) {
            try { second.unpublishObjectClassAttributes(secondClass, secondAttributes); }
            catch (Exception ignored) { }
         }
         if (ownerJoined && ownerPublished && ownerClass != null && ownerAttributes != null) {
            try { owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(
               ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
            catch (Exception ignored) { }
         }
         if (firstJoined) {
            try { first.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (secondJoined) {
            try { second.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (secondConnected) { try { second.disconnect(); } catch (Exception ignored) { } }
         if (firstConnected) { try { first.disconnect(); } catch (Exception ignored) { } }
         if (ownerConnected) { try { owner.disconnect(); } catch (Exception ignored) { } }
      }
   }

   private static void cancelFirstCandidate(RTIambassador first, Recorder recorder,
         ObjectInstanceHandle object, AttributeHandleSet attributes, byte[] regularTag,
         CallbackModel callbackModel) throws Exception {
      first.attributeOwnershipAcquisition(object, attributes, regularTag);
      first.cancelAttributeOwnershipAcquisition(object, attributes);
      await(first, callbackModel, () -> !recorder.cancellations.isEmpty(),
         "first candidate acquisition-cancellation confirmation");
      OwnershipEvent cancellation = recorder.cancellations.get(0);
      JavaTckSupport.check(object.equals(cancellation.object)
            && cancellation.attributes.equals(new HashSet<>(attributes))
            && recorder.acquisitions.isEmpty(),
         "first candidate cancellation returned the wrong attributes or acquired ownership");
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

   private static OwnershipEvent event(Object[] arguments) {
      return new OwnershipEvent((ObjectInstanceHandle) arguments[0],
         (AttributeHandleSet) arguments[1], (byte[]) arguments[2]);
   }

   private static final class OwnershipEvent {
      private final ObjectInstanceHandle object;
      private final HashSet<AttributeHandle> attributes;
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
      private final List<OwnershipEvent> divestitureConfirmations = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> releaseRequests = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> acquisitions = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> cancellations = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("requestDivestitureConfirmation".equals(name)
                     && arguments != null && arguments.length == 3) {
                  divestitureConfirmations.add(event(arguments));
               } else if ("requestAttributeOwnershipRelease".equals(name)
                     && arguments != null && arguments.length == 3) {
                  releaseRequests.add(event(arguments));
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)
                     && arguments != null && arguments.length == 3) {
                  acquisitions.add(event(arguments));
               } else if ("confirmAttributeOwnershipAcquisitionCancellation".equals(name)
                     && arguments != null && arguments.length == 2) {
                  cancellations.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], null));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK willing-to-acquire callbacks";
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
         acquisitions.clear();
         cancellations.clear();
      }
   }
}
