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

/** Cancels one acquisition member during negotiated divestiture and transfers only the rest. */
final class NegotiatedDivestiturePartialAcquisitionCancellationTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private NegotiatedDivestiturePartialAcquisitionCancellationTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName,
         String firstAttributeName, String secondAttributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      String federation = "java-tck-negotiated-partial-cancel-" + UUID.randomUUID();
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
         owner.joinFederationExecution("java-tck-negotiated-partial-owner", federation);
         ownerJoined = true;
         requester.joinFederationExecution("java-tck-negotiated-partial-requester", federation);
         requesterJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         requesterClass = requester.getObjectClassHandle(objectClassName);
         JavaTckSupport.check(ownerClass != null && requesterClass != null
               && ownerClass.equals(requesterClass),
            "adapter multi-attribute FOM did not resolve matching negotiated classes");
         AttributeHandle ownerFirst = owner.getAttributeHandle(ownerClass, firstAttributeName);
         AttributeHandle ownerSecond = owner.getAttributeHandle(ownerClass, secondAttributeName);
         AttributeHandle requesterFirst = requester.getAttributeHandle(
            requesterClass, firstAttributeName);
         AttributeHandle requesterSecond = requester.getAttributeHandle(
            requesterClass, secondAttributeName);
         JavaTckSupport.check(ownerClass != null && requesterClass != null
               && ownerFirst != null && ownerSecond != null
               && requesterFirst != null && requesterSecond != null
               && !ownerFirst.equals(ownerSecond)
               && ownerFirst.equals(requesterFirst) && ownerSecond.equals(requesterSecond),
            "adapter multi-attribute FOM did not resolve distinct negotiated attributes");
         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerFirst, ownerSecond);
         requesterAttributes = JavaTckSupport.attributeSet(requester, requesterFirst, requesterSecond);
         AttributeHandleSet ownerCanceled = JavaTckSupport.attributeSet(owner, ownerFirst);
         AttributeHandleSet ownerRetained = JavaTckSupport.attributeSet(owner, ownerSecond);
         AttributeHandleSet requesterCanceled = JavaTckSupport.attributeSet(requester, requesterFirst);
         AttributeHandleSet requesterRetained = JavaTckSupport.attributeSet(
            requester, requesterSecond);
         final AttributeHandleSet ownerRetainedSet = ownerRetained;

         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         ownerPublished = true;
         requester.publishObjectClassAttributes(requesterClass, requesterAttributes);
         requesterPublished = true;
         requester.subscribeObjectClassAttributes(requesterClass, requesterAttributes);
         requesterSubscribed = true;
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "object registration returned a null handle");
         await(requester, callbackModel,
            () -> requesterRecorder.discoveries.contains(object),
            "negotiated partial-divestiture object discovery");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerFirst)
               && owner.isAttributeOwnedByFederate(object, ownerSecond),
            "owner did not initially own both negotiated attributes");

         byte[] acquisitionTag = new byte[] {(byte) 0xc3, 0x25};
         byte[] divestitureTag = new byte[] {(byte) 0xd3, 0x25};
         byte[] confirmationTag = new byte[] {(byte) 0xe3, 0x25};
         ownerRecorder.clearOwnershipEvents();
         requesterRecorder.clearOwnershipEvents();
         requester.attributeOwnershipAcquisition(object, requesterAttributes, acquisitionTag);
         owner.negotiatedAttributeOwnershipDivestiture(object, ownerAttributes, divestitureTag);
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerFirst)
               && owner.isAttributeOwnedByFederate(object, ownerSecond),
            "negotiated divestiture changed ownership before confirmation");

         requester.cancelAttributeOwnershipAcquisition(object, requesterCanceled);
         await(requester, callbackModel,
            () -> !requesterRecorder.cancellations.isEmpty(),
            "partial acquisition-cancellation confirmation");
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(requester);
         }
         JavaTckSupport.check(requesterRecorder.cancellations.size() == 1,
            "partial acquisition cancellation produced duplicate confirmations");
         checkEvent(requesterRecorder.cancellations.get(0), object, requesterCanceled, null,
            "partial acquisition-cancellation confirmation");
         JavaTckSupport.check(requesterRecorder.acquisitions.isEmpty(),
            "requester acquired an attribute before negotiated confirmation");

         await(owner, callbackModel,
            () -> !ownerRecorder.divestitureConfirmations.isEmpty(),
            "partial negotiated-divestiture confirmation callback");
         OwnershipEvent divestitureConfirmation = ownerRecorder.divestitureConfirmations.get(0);
         JavaTckSupport.check(object.equals(divestitureConfirmation.object),
            "divestiture confirmation callback returned the wrong object");
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(
               divestitureConfirmation.attributes.equals(new HashSet<>(ownerRetainedSet))
                  && Arrays.equals(acquisitionTag, divestitureConfirmation.tag),
               "evoked divestiture confirmation did not identify the retained attribute and request tag");
         }

         owner.confirmDivestiture(object, ownerRetained, confirmationTag);
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerFirst)
               && !owner.isAttributeOwnedByFederate(object, ownerSecond)
               && !requester.isAttributeOwnedByFederate(object, requesterFirst)
               && requester.isAttributeOwnedByFederate(object, requesterSecond),
            "negotiated confirmation transferred the canceled attribute or retained the other");
         await(requester, callbackModel,
            () -> !requesterRecorder.acquisitions.isEmpty(),
            "partial negotiated acquisition notification");
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(requester);
         }
         JavaTckSupport.check(requesterRecorder.acquisitions.size() == 1
               && requesterRecorder.cancellations.size() == 1,
            "partial negotiated transfer duplicated acquisition or cancellation callbacks");
         checkEvent(requesterRecorder.acquisitions.get(0), object, requesterRetained,
            confirmationTag, "partial negotiated acquisition notification");

         owner.cancelNegotiatedAttributeOwnershipDivestiture(object, ownerCanceled);
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerFirst)
               && requester.isAttributeOwnedByFederate(object, requesterSecond),
            "canceling the remaining negotiation changed transferred ownership");
         requester.unpublishObjectClassAttributes(requesterClass, requesterRetained);
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
            try { owner.resignFederationExecution(
               ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
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
      JavaTckSupport.check(object.equals(event.object)
            && new HashSet<>(attributes).equals(event.attributes)
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
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)
                     && arguments != null && arguments.length == 3) {
                  acquisitions.add(event(arguments));
               } else if ("confirmAttributeOwnershipAcquisitionCancellation".equals(name)
                     && arguments != null && arguments.length == 2) {
                  cancellations.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], null));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK negotiated partial-ownership callbacks";
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
         acquisitions.clear();
         cancellations.clear();
      }

      private static OwnershipEvent event(Object[] arguments) {
         return new OwnershipEvent((ObjectInstanceHandle) arguments[0],
            (AttributeHandleSet) arguments[1], (byte[]) arguments[2]);
      }
   }
}
