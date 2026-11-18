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

/** Checks that one multi-attribute negotiated acquisition produces one full-set confirmation. */
final class NegotiatedDivestitureConfirmationCoalescingTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private NegotiatedDivestitureConfirmationCoalescingTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String firstAttributeName, String secondAttributeName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      String federation = "java-tck-negotiated-confirmation-coalescing-" + UUID.randomUUID();
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
         owner.joinFederationExecution("java-tck-confirmation-coalescing-owner", federation);
         ownerJoined = true;
         requester.joinFederationExecution(
            "java-tck-confirmation-coalescing-requester", federation);
         requesterJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         requesterClass = requester.getObjectClassHandle(objectClassName);
         AttributeHandle ownerFirst = owner.getAttributeHandle(ownerClass, firstAttributeName);
         AttributeHandle ownerSecond = owner.getAttributeHandle(ownerClass, secondAttributeName);
         AttributeHandle requesterFirst = requester.getAttributeHandle(
            requesterClass, firstAttributeName);
         AttributeHandle requesterSecond = requester.getAttributeHandle(
            requesterClass, secondAttributeName);
         JavaTckSupport.check(ownerClass != null && requesterClass != null
               && ownerFirst != null && ownerSecond != null
               && requesterFirst != null && requesterSecond != null
               && !ownerFirst.equals(ownerSecond) && !requesterFirst.equals(requesterSecond),
            "adapter FOM did not resolve two distinct negotiated-ownership attributes");
         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerFirst, ownerSecond);
         requesterAttributes = JavaTckSupport.attributeSet(
            requester, requesterFirst, requesterSecond);

         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         ownerPublished = true;
         requester.publishObjectClassAttributes(requesterClass, requesterAttributes);
         requesterPublished = true;
         requester.subscribeObjectClassAttributes(requesterClass, requesterAttributes);
         requesterSubscribed = true;
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "negotiated confirmation coalescing registration returned a null object handle");
         await(requester, callbackModel,
            () -> requesterRecorder.discoveries.contains(object),
            "requester object discovery");

         byte[] offerTag = {0x41, 0x7c, 0x22};
         byte[] acquisitionTag = {0x52, (byte) 0x8d, 0x33};
         byte[] confirmationTag = {0x63, (byte) 0x9e, 0x44};
         owner.negotiatedAttributeOwnershipDivestiture(object, ownerAttributes, offerTag);
         JavaTckSupport.check(ownerRecorder.confirmations.isEmpty()
               && owner.isAttributeOwnedByFederate(object,
                  owner.getAttributeHandle(ownerClass, firstAttributeName))
               && owner.isAttributeOwnedByFederate(object,
                  owner.getAttributeHandle(ownerClass, secondAttributeName)),
            "negotiated offer transferred attributes before a new owner requested them");

         requester.attributeOwnershipAcquisition(object, requesterAttributes, acquisitionTag);
         await(owner, callbackModel, () -> !ownerRecorder.confirmations.isEmpty(),
            "coalesced request-divestiture-confirmation callback");
         drainIfEvoked(callbackModel, owner, requester);
         JavaTckSupport.check(ownerRecorder.confirmations.size() == 1,
            "one acquisition call did not produce exactly one confirmation callback");
         checkEvent(ownerRecorder.confirmations.get(0), object,
            new HashSet<>(ownerAttributes), acquisitionTag,
            "coalesced request-divestiture-confirmation callback");
         AttributeHandle requesterFirstHandle = requester.getAttributeHandle(
            requesterClass, firstAttributeName);
         AttributeHandle requesterSecondHandle = requester.getAttributeHandle(
            requesterClass, secondAttributeName);
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(
               object, owner.getAttributeHandle(ownerClass, firstAttributeName))
               && owner.isAttributeOwnedByFederate(
                  object, owner.getAttributeHandle(ownerClass, secondAttributeName))
               && !requester.isAttributeOwnedByFederate(object, requesterFirstHandle)
               && !requester.isAttributeOwnedByFederate(object, requesterSecondHandle)
               && requesterRecorder.acquisitions.isEmpty(),
            "negotiated acquisition transferred attributes before owner confirmation");

         owner.confirmDivestiture(object, ownerAttributes, confirmationTag);
         await(requester, callbackModel, () -> !requesterRecorder.acquisitions.isEmpty(),
            "acquisition callback after negotiated confirmation");
         drainIfEvoked(callbackModel, owner, requester);
         JavaTckSupport.check(requesterRecorder.acquisitions.size() == 1,
            "confirmed negotiated divestiture did not produce one acquisition callback");
         checkEvent(requesterRecorder.acquisitions.get(0), object,
            new HashSet<>(requesterAttributes), confirmationTag,
            "acquisition callback after negotiated confirmation");
         JavaTckSupport.check(!owner.isAttributeOwnedByFederate(
               object, owner.getAttributeHandle(ownerClass, firstAttributeName))
               && !owner.isAttributeOwnedByFederate(
                  object, owner.getAttributeHandle(ownerClass, secondAttributeName))
               && requester.isAttributeOwnedByFederate(object, requesterFirstHandle)
               && requester.isAttributeOwnedByFederate(object, requesterSecondHandle),
            "confirmation did not transfer the complete attribute set");
      } finally {
         if (requesterJoined && requesterSubscribed && requesterClass != null
               && requesterAttributes != null) {
            try { requester.unsubscribeObjectClassAttributes(requesterClass, requesterAttributes); }
            catch (Exception ignored) { }
         }
         unpublishIfJoined(owner, ownerJoined, ownerPublished, ownerClass, ownerAttributes);
         unpublishIfJoined(requester, requesterJoined, requesterPublished,
            requesterClass, requesterAttributes);
         if (requesterJoined) {
            try { requester.resignFederationExecution(ResignAction.UNCONDITIONALLY_DIVEST_ATTRIBUTES); }
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
         HashSet<AttributeHandle> attributes, byte[] tag, String description) {
      JavaTckSupport.check(object.equals(event.object) && attributes.equals(event.attributes)
            && Arrays.equals(tag, event.tag),
         description + " returned the wrong object, attribute set, or tag");
   }

   private static void unpublishIfJoined(RTIambassador ambassador, boolean joined,
         boolean published, ObjectClassHandle objectClass, AttributeHandleSet attributes) {
      if (joined && published && objectClass != null && attributes != null) {
         try { ambassador.unpublishObjectClassAttributes(objectClass, attributes); }
         catch (Exception ignored) { }
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

   private static void drainIfEvoked(CallbackModel model, RTIambassador... ambassadors)
         throws Exception {
      if (model == CallbackModel.HLA_EVOKED) {
         for (RTIambassador ambassador : ambassadors) JavaTckSupport.drain(ambassador);
      }
   }

   private static OwnershipEvent ownershipEvent(Object[] arguments) {
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
      private final List<OwnershipEvent> confirmations = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> acquisitions = new CopyOnWriteArrayList<>();

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
                  confirmations.add(ownershipEvent(arguments));
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)
                     && arguments != null && arguments.length == 3) {
                  acquisitions.add(ownershipEvent(arguments));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) {
                     return "Java TCK negotiated confirmation-coalescing callbacks";
                  }
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
