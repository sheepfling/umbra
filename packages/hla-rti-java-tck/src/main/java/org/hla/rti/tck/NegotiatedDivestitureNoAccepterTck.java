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
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;
import java.util.function.BooleanSupplier;

/** Negotiated divestiture retains ownership until an acquirer arrives and the owner confirms. */
final class NegotiatedDivestitureNoAccepterTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private NegotiatedDivestitureNoAccepterTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      String federation = "java-tck-negotiated-no-accepter-" + UUID.randomUUID();
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
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-no-accepter-owner", federation);
         ownerJoined = true;
         requester.joinFederationExecution("java-tck-no-accepter-requester", federation);
         requesterJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         requesterClass = requester.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle requesterAttribute = requester.getAttributeHandle(
            requesterClass, attributeName);
         JavaTckSupport.check(ownerClass != null && ownerClass.equals(requesterClass)
               && ownerAttribute != null && ownerAttribute.equals(requesterAttribute),
            "adapter FOM did not resolve matching no-accepter class and attribute handles");
         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         requesterAttributes = JavaTckSupport.attributeSet(requester, requesterAttribute);
         AttributeHandleSet ownerAttributeSet = ownerAttributes;
         AttributeHandleSet requesterAttributeSet = requesterAttributes;

         owner.publishObjectClassAttributes(ownerClass, ownerAttributeSet);
         ownerPublished = true;
         requester.publishObjectClassAttributes(requesterClass, requesterAttributeSet);
         requesterPublished = true;
         requester.subscribeObjectClassAttributes(requesterClass, requesterAttributeSet);
         requesterSubscribed = true;
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "negotiated no-accepter registration returned a null object handle");
         await(requester, callbackModel,
            () -> requesterRecorder.discoveries.contains(object),
            "negotiated no-accepter discovery");

         byte[] offerTag = new byte[] {(byte) 0xd7, 0x31};
         byte[] requestTag = new byte[] {(byte) 0xa7, 0x31};
         byte[] confirmationTag = new byte[] {(byte) 0xe7, 0x31};
         ownerRecorder.clearOwnershipEvents();
         requesterRecorder.clearOwnershipEvents();
         owner.negotiatedAttributeOwnershipDivestiture(object, ownerAttributeSet, offerTag);
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(owner);
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && !requester.isAttributeOwnedByFederate(object, requesterAttribute)
               && ownerRecorder.confirmations.isEmpty(),
            "negotiated divestiture changed ownership or requested confirmation without an acquirer");

         requester.attributeOwnershipAcquisition(object, requesterAttributeSet, requestTag);
         await(owner, callbackModel, () -> !ownerRecorder.confirmations.isEmpty(),
            "negotiated divestiture confirmation after acquisition");
         OwnershipEvent request = ownerRecorder.confirmations.get(0);
         checkEvent(request, object, ownerAttributeSet, requestTag,
            "negotiated divestiture confirmation request");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && !requester.isAttributeOwnedByFederate(object, requesterAttribute),
            "negotiated divestiture transferred ownership before owner confirmation");

         owner.confirmDivestiture(object, ownerAttributeSet, confirmationTag);
         await(requester, callbackModel, () -> !requesterRecorder.acquisitions.isEmpty(),
            "negotiated no-accepter acquisition notification");
         checkEvent(requesterRecorder.acquisitions.get(0), object,
            requesterAttributeSet, confirmationTag,
            "negotiated no-accepter acquisition notification");
         JavaTckSupport.check(!owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && requester.isAttributeOwnedByFederate(object, requesterAttribute),
            "negotiated divestiture did not transfer after owner confirmation");

         requester.unpublishObjectClassAttributes(requesterClass, requesterAttributeSet);
         requesterPublished = false;
         owner.unpublishObjectClassAttributes(ownerClass, ownerAttributeSet);
         ownerPublished = false;
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
            try { requester.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
            catch (Exception ignored) { }
         }
         if (created) { try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { } }
         if (requesterConnected) { try { requester.disconnect(); } catch (Exception ignored) { } }
         if (ownerConnected) { try { owner.disconnect(); } catch (Exception ignored) { } }
      }
   }

   static void runConcurrentAssumption(RtiFactory factory, String fom,
         String timeImplementation, String objectClassName, String attributeName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      RTIambassador candidate = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      Recorder candidateRecorder = new Recorder();
      String federation = "java-tck-negotiated-concurrent-assumption-" + UUID.randomUUID();
      boolean[] connected = new boolean[3], joined = new boolean[3];
      boolean created = false;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         connected[0] = true;
         requester.connect(requesterRecorder.proxy(), callbackModel);
         connected[1] = true;
         candidate.connect(candidateRecorder.proxy(), callbackModel);
         connected[2] = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-concurrent-owner", federation);
         joined[0] = true;
         requester.joinFederationExecution("java-tck-concurrent-requester", federation);
         joined[1] = true;
         candidate.joinFederationExecution("java-tck-concurrent-candidate", federation);
         joined[2] = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle requesterClass = requester.getObjectClassHandle(objectClassName);
         ObjectClassHandle candidateClass = candidate.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle requesterAttribute = requester.getAttributeHandle(
            requesterClass, attributeName);
         AttributeHandle candidateAttribute = candidate.getAttributeHandle(
            candidateClass, attributeName);
         JavaTckSupport.check(ownerClass != null && requesterClass != null
               && candidateClass != null && ownerAttribute != null
               && requesterAttribute != null && candidateAttribute != null,
            "adapter FOM did not resolve concurrent-assumption handles");
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(
            owner, ownerAttribute);
         AttributeHandleSet requesterAttributes = JavaTckSupport.attributeSet(
            requester, requesterAttribute);
         AttributeHandleSet candidateAttributes = JavaTckSupport.attributeSet(
            candidate, candidateAttribute);
         candidate.subscribeObjectClassAttributes(candidateClass, candidateAttributes);
         candidate.publishObjectClassAttributes(candidateClass, candidateAttributes);
         requester.subscribeObjectClassAttributes(requesterClass, requesterAttributes);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "concurrent-assumption registration returned a null object handle");
         await(candidate, callbackModel, () -> candidateRecorder.discoveries.contains(object),
            "third-candidate discovery");
         await(requester, callbackModel, () -> requesterRecorder.discoveries.contains(object),
            "requester discovery");
         requester.publishObjectClassAttributes(requesterClass, requesterAttributes);

         byte[] offerTag = new byte[] {(byte) 0xd2, 0x11, 0x6d, 0x09};
         byte[] confirmationTag = new byte[] {(byte) 0xe2, 0x6a, (byte) 0xb4, 0x2d};
         requester.attributeOwnershipAcquisition(object, requesterAttributes, offerTag);
         owner.negotiatedAttributeOwnershipDivestiture(object, ownerAttributes, offerTag);
         await(owner, callbackModel, () -> ownerRecorder.confirmations.size() == 1,
            "owner confirmation request");
         await(candidate, callbackModel, () -> candidateRecorder.assumptions.size() == 1,
            "third-candidate assumption while confirmation is pending");
         JavaTckSupport.check(ownerRecorder.confirmations.size() == 1
               && candidateRecorder.assumptions.size() == 1,
            "pending confirmation emitted duplicate ownership callbacks");
         checkEvent(ownerRecorder.confirmations.get(0), object, ownerAttributes, offerTag,
            "owner confirmation request");
         checkEvent(candidateRecorder.assumptions.get(0), object, candidateAttributes,
            offerTag, "third-candidate assumption");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && !requester.isAttributeOwnedByFederate(object, requesterAttribute),
            "negotiated offer transferred ownership before confirmation");

         owner.confirmDivestiture(object, ownerAttributes, confirmationTag);
         await(requester, callbackModel, () -> requesterRecorder.acquisitions.size() == 1,
            "requester acquisition after confirmation");
         JavaTckSupport.check(requesterRecorder.acquisitions.size() == 1,
            "confirmation emitted duplicate acquisition notifications");
         checkEvent(requesterRecorder.acquisitions.get(0), object, requesterAttributes,
            confirmationTag, "requester acquisition notification");
         JavaTckSupport.check(!owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && requester.isAttributeOwnedByFederate(object, requesterAttribute),
            "Confirm Divestiture did not transfer ownership to the requester");

         requester.resignFederationExecution(ResignAction.UNCONDITIONALLY_DIVEST_ATTRIBUTES);
         joined[1] = false;
         candidate.resignFederationExecution(ResignAction.UNCONDITIONALLY_DIVEST_ATTRIBUTES);
         joined[2] = false;
         owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         joined[0] = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (joined[2]) { try { candidate.resignFederationExecution(
            ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); } catch (Exception ignored) { } }
         if (joined[1]) { try { requester.resignFederationExecution(
            ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); } catch (Exception ignored) { } }
         if (joined[0]) { try { owner.resignFederationExecution(
            ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); } catch (Exception ignored) { } }
         if (created) { try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { } }
         if (connected[2]) { try { candidate.disconnect(); } catch (Exception ignored) { } }
         if (connected[1]) { try { requester.disconnect(); } catch (Exception ignored) { } }
         if (connected[0]) { try { owner.disconnect(); } catch (Exception ignored) { } }
      }
   }

   static void runConfirmFromCallback(RtiFactory factory, String fom,
         String timeImplementation, String objectClassName, String attributeName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      String federation = "java-tck-confirm-divestiture-callback-" + UUID.randomUUID();
      boolean[] connected = new boolean[2], joined = new boolean[2];
      boolean created = false;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         connected[0] = true;
         requester.connect(requesterRecorder.proxy(), callbackModel);
         connected[1] = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-confirm-callback-owner", federation);
         joined[0] = true;
         requester.joinFederationExecution("java-tck-confirm-callback-requester", federation);
         joined[1] = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle requesterClass = requester.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle requesterAttribute = requester.getAttributeHandle(
            requesterClass, attributeName);
         JavaTckSupport.check(ownerClass != null && requesterClass != null
               && ownerAttribute != null && requesterAttribute != null,
            "adapter FOM did not resolve confirmation-callback handles");
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(
            owner, ownerAttribute);
         AttributeHandleSet requesterAttributes = JavaTckSupport.attributeSet(
            requester, requesterAttribute);
         requester.subscribeObjectClassAttributes(requesterClass, requesterAttributes);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "callback re-entry registration returned a null object handle");
         await(requester, callbackModel, () -> requesterRecorder.discoveries.contains(object),
            "requester discovery before negotiation");
         requester.publishObjectClassAttributes(requesterClass, requesterAttributes);

         byte[] negotiatedTag = new byte[] {(byte) 0xc2, 0x11, 0x6d, 0x09};
         byte[] confirmationTag = new byte[] {(byte) 0xe1, 0x6a, (byte) 0xb4, 0x2d};
         requester.attributeOwnershipAcquisition(object, requesterAttributes, negotiatedTag);
         JavaTckSupport.check(requesterRecorder.acquisitions.isEmpty()
               && ownerRecorder.confirmations.isEmpty()
               && owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && !requester.isAttributeOwnedByFederate(object, requesterAttribute),
            "acquisition transferred ownership before negotiated divestiture");

         AtomicBoolean confirmed = new AtomicBoolean();
         ownerRecorder.confirmationHook.set(() -> {
            try {
               owner.confirmDivestiture(object, ownerAttributes, confirmationTag);
               confirmed.set(true);
            } catch (Exception failure) {
               throw new IllegalStateException("Confirm Divestiture failed inside callback", failure);
            }
         });
         owner.negotiatedAttributeOwnershipDivestiture(
            object, ownerAttributes, negotiatedTag);
         await(owner, callbackModel, () -> confirmed.get()
               || ownerRecorder.callbackFailure.get() != null,
            "re-entrant confirmation callback");
         Throwable callbackFailure = ownerRecorder.callbackFailure.get();
         if (callbackFailure != null) throw new IllegalStateException(
            "re-entrant confirmation callback failed", callbackFailure);
         ownerRecorder.confirmationHook.set(null);
         JavaTckSupport.check(confirmed.get() && ownerRecorder.confirmations.size() == 1,
            "confirmation callback did not complete exactly once");
         checkEvent(ownerRecorder.confirmations.get(0), object, ownerAttributes,
            negotiatedTag, "re-entrant confirmation request");

         await(requester, callbackModel, () -> requesterRecorder.acquisitions.size() == 1,
            "acquisition after callback-local confirmation");
         JavaTckSupport.check(requesterRecorder.acquisitions.size() == 1,
            "callback-local confirmation emitted duplicate acquisitions");
         checkEvent(requesterRecorder.acquisitions.get(0), object,
            requesterAttributes, confirmationTag, "callback-local acquisition");
         JavaTckSupport.check(!owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && requester.isAttributeOwnedByFederate(object, requesterAttribute),
            "re-entrant confirmation did not transfer ownership");

         requester.resignFederationExecution(ResignAction.UNCONDITIONALLY_DIVEST_ATTRIBUTES);
         joined[1] = false;
         owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         joined[0] = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (joined[1]) { try { requester.resignFederationExecution(
            ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); } catch (Exception ignored) { } }
         if (joined[0]) { try { owner.resignFederationExecution(
            ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); } catch (Exception ignored) { } }
         if (created) { try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { } }
         if (connected[1]) { try { requester.disconnect(); } catch (Exception ignored) { } }
         if (connected[0]) { try { owner.disconnect(); } catch (Exception ignored) { } }
      }
   }

   static void runMixedSetAtomicity(RtiFactory factory, String fom,
         String timeImplementation, String objectClassName,
         String firstAttributeName, String secondAttributeName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      String federation = "java-tck-confirm-mixed-set-" + UUID.randomUUID();
      boolean[] connected = new boolean[2], joined = new boolean[2];
      boolean created = false;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         connected[0] = true;
         requester.connect(requesterRecorder.proxy(), callbackModel);
         connected[1] = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-mixed-confirm-owner", federation);
         joined[0] = true;
         requester.joinFederationExecution("java-tck-mixed-confirm-requester", federation);
         joined[1] = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle requesterClass = requester.getObjectClassHandle(objectClassName);
         AttributeHandle ownerFirst = owner.getAttributeHandle(ownerClass, firstAttributeName);
         AttributeHandle ownerSecond = owner.getAttributeHandle(ownerClass, secondAttributeName);
         AttributeHandle requesterFirst = requester.getAttributeHandle(
            requesterClass, firstAttributeName);
         JavaTckSupport.check(ownerClass != null && requesterClass != null
               && ownerFirst != null && ownerSecond != null && requesterFirst != null
               && !ownerFirst.equals(ownerSecond),
            "adapter two-attribute FOM returned invalid or duplicate handles");
         AttributeHandleSet ownerBoth = JavaTckSupport.attributeSet(
            owner, ownerFirst, ownerSecond);
         AttributeHandleSet ownerRequested = JavaTckSupport.attributeSet(owner, ownerFirst);
         AttributeHandleSet requesterRequested = JavaTckSupport.attributeSet(
            requester, requesterFirst);
         owner.publishObjectClassAttributes(ownerClass, ownerBoth);
         requester.subscribeObjectClassAttributes(requesterClass, requesterRequested);
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "mixed confirmation registration returned a null object handle");
         await(requester, callbackModel, () -> requesterRecorder.discoveries.contains(object),
            "requester discovery before mixed confirmation");
         requester.publishObjectClassAttributes(requesterClass, requesterRequested);

         byte[] negotiatedTag = new byte[] {(byte) 0xc3, 0x11, 0x6d, 0x09};
         byte[] confirmationTag = new byte[] {(byte) 0xe1, 0x6a, (byte) 0xb4, 0x2d};
         requester.attributeOwnershipAcquisition(object, requesterRequested, negotiatedTag);
         AtomicBoolean mixedAttempted = new AtomicBoolean();
         AtomicBoolean mixedRejected = new AtomicBoolean();
         ownerRecorder.confirmationHook.set(() -> {
            try {
               JavaTckSupport.expectFailure(
                  () -> owner.confirmDivestiture(object, ownerBoth, confirmationTag),
                  "AttributeDivestitureWasNotRequested", "mixed Confirm Divestiture set");
               mixedRejected.set(true);
               mixedAttempted.set(true);
            } catch (Exception failure) {
               throw new IllegalStateException("mixed confirmation callback failed", failure);
            }
         });
         owner.negotiatedAttributeOwnershipDivestiture(
            object, ownerRequested, negotiatedTag);
         await(owner, callbackModel, () -> mixedAttempted.get()
               || ownerRecorder.callbackFailure.get() != null,
            "mixed-set confirmation rejection");
         Throwable callbackFailure = ownerRecorder.callbackFailure.get();
         if (callbackFailure != null) throw new IllegalStateException(
            "mixed-set confirmation callback failed", callbackFailure);
         ownerRecorder.confirmationHook.set(null);
         JavaTckSupport.check(mixedRejected.get() && ownerRecorder.confirmations.size() == 1,
            "mixed confirmation did not reject exactly one request");
         checkEvent(ownerRecorder.confirmations.get(0), object, ownerRequested,
            negotiatedTag, "mixed-set confirmation request");
         JavaTckSupport.check(requesterRecorder.acquisitions.isEmpty()
               && owner.isAttributeOwnedByFederate(object, ownerFirst)
               && owner.isAttributeOwnedByFederate(object, ownerSecond)
               && !requester.isAttributeOwnedByFederate(object, requesterFirst),
            "rejected mixed set changed ownership or notified acquisition");

         owner.confirmDivestiture(object, ownerRequested, confirmationTag);
         await(requester, callbackModel, () -> requesterRecorder.acquisitions.size() == 1,
            "valid confirmation after mixed-set rejection");
         JavaTckSupport.check(requesterRecorder.acquisitions.size() == 1,
            "valid confirmation emitted duplicate acquisition callbacks");
         checkEvent(requesterRecorder.acquisitions.get(0), object, requesterRequested,
            confirmationTag, "valid confirmation after mixed rejection");
         JavaTckSupport.check(!owner.isAttributeOwnedByFederate(object, ownerFirst)
               && owner.isAttributeOwnedByFederate(object, ownerSecond)
               && requester.isAttributeOwnedByFederate(object, requesterFirst),
            "rejected mixed set partially consumed the valid confirmation path");

         requester.resignFederationExecution(ResignAction.UNCONDITIONALLY_DIVEST_ATTRIBUTES);
         joined[1] = false;
         owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         joined[0] = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (joined[1]) { try { requester.resignFederationExecution(
            ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); } catch (Exception ignored) { } }
         if (joined[0]) { try { owner.resignFederationExecution(
            ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); } catch (Exception ignored) { } }
         if (created) { try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { } }
         if (connected[1]) { try { requester.disconnect(); } catch (Exception ignored) { } }
         if (connected[0]) { try { owner.disconnect(); } catch (Exception ignored) { } }
      }
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

   private static final class OwnershipEvent {
      private final ObjectInstanceHandle object;
      private final java.util.Set<AttributeHandle> attributes;
      private final byte[] tag;

      private OwnershipEvent(ObjectInstanceHandle object, AttributeHandleSet attributes,
            byte[] tag) {
         this.object = object;
         this.attributes = new HashSet<>(attributes);
         this.tag = tag.clone();
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> confirmations = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> acquisitions = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> assumptions = new CopyOnWriteArrayList<>();
      private final AtomicReference<Runnable> confirmationHook = new AtomicReference<>();
      private final AtomicReference<Throwable> callbackFailure = new AtomicReference<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("requestDivestitureConfirmation".equals(name)
                     && arguments != null && arguments.length >= 3) {
                  confirmations.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (byte[]) arguments[2]));
                  Runnable hook = confirmationHook.get();
                  if (hook != null) {
                     try { hook.run(); } catch (Throwable failure) {
                        callbackFailure.set(failure);
                     }
                  }
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)
                     && arguments != null && arguments.length >= 3) {
                  acquisitions.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (byte[]) arguments[2]));
               } else if ("requestAttributeOwnershipAssumption".equals(name)
                     && arguments != null && arguments.length >= 3) {
                  assumptions.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (byte[]) arguments[2]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK negotiated no-accepter callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }

      private void clearOwnershipEvents() {
         confirmations.clear();
         acquisitions.clear();
         assumptions.clear();
      }
   }
}
