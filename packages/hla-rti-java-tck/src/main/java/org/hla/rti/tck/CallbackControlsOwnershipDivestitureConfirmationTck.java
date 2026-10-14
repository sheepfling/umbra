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
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Portable callback-enable gating for negotiated-divestiture confirmation. */
final class CallbackControlsOwnershipDivestitureConfirmationTck {
   private static final byte[] ACQUISITION_TAG = new byte[] {0x4e, 0x45, 0x47};
   private static final byte[] DIVESTITURE_TAG = new byte[] {0x44, 0x49, 0x56};

   private CallbackControlsOwnershipDivestitureConfirmationTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String attributeName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      String federation = "java-tck-callback-ownership-divestiture-confirmation-"
         + UUID.randomUUID();
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
         owner.joinFederationExecution("java-tck-divestiture-owner", federation);
         ownerJoined = true;
         requester.joinFederationExecution("java-tck-divestiture-requester", federation);
         requesterJoined = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle requesterClass = requester.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle requesterAttribute =
            requester.getAttributeHandle(requesterClass, attributeName);
         JavaTckSupport.check(ownerClass.equals(requesterClass)
               && ownerAttribute.equals(requesterAttribute),
            "standard FOM lookups returned mismatched divestiture-confirmation handles");
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet requesterAttributes =
            JavaTckSupport.attributeSet(requester, requesterAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         requester.publishObjectClassAttributes(requesterClass, requesterAttributes);
         requester.subscribeObjectClassAttributes(requesterClass, requesterAttributes);
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "divestiture-confirmation registration returned a null object handle");
         await(requester, callbackModel, objectClassName,
            () -> requesterRecorder.discoveries.contains(object),
            "requester did not discover the registered object");

         ownerRecorder.clearOwnershipEvents();
         requesterRecorder.clearOwnershipEvents();
         requester.attributeOwnershipAcquisition(
            object, requesterAttributes, ACQUISITION_TAG.clone());

         owner.disableCallbacks();
         owner.negotiatedAttributeOwnershipDivestiture(
            object, ownerAttributes, DIVESTITURE_TAG.clone());
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(owner.evokeMultipleCallbacks(0.0, 1.0),
               "evoked callback control did not admit the pending divestiture confirmation");
         } else {
            service(owner, callbackModel, objectClassName);
         }
         JavaTckSupport.check(ownerRecorder.divestitureConfirmations().isEmpty(),
            "disabled callbacks exposed a requestDivestitureConfirmation callback");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && !requester.isAttributeOwnedByFederate(object, requesterAttribute),
            "negotiated divestiture changed ownership while its callback was disabled");

         owner.enableCallbacks();
         await(owner, callbackModel, objectClassName,
            () -> ownerRecorder.divestitureConfirmations().size() == 1,
            "re-enabled divestiture-confirmation callback did not arrive");
         List<OwnershipEvent> confirmations = ownerRecorder.divestitureConfirmations();
         OwnershipEvent confirmation = confirmations.get(0);
         JavaTckSupport.check(confirmations.size() == 1
               && object.equals(confirmation.object)
               && new HashSet<>(ownerAttributes).equals(confirmation.attributes)
               && Arrays.equals(ACQUISITION_TAG, confirmation.tag)
               && owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && !requester.isAttributeOwnedByFederate(object, requesterAttribute),
            "re-enabled divestiture confirmation or ownership query returned the wrong state");

         owner.cancelNegotiatedAttributeOwnershipDivestiture(object, ownerAttributes);
         await(owner, callbackModel, objectClassName,
            () -> ownerRecorder.releaseRequests().size() == 1,
            "canceling divestiture did not restore the ownership release request");
         List<OwnershipEvent> releaseRequests = ownerRecorder.releaseRequests();
         OwnershipEvent releaseRequest = releaseRequests.get(0);
         JavaTckSupport.check(releaseRequests.size() == 1
               && object.equals(releaseRequest.object)
               && new HashSet<>(ownerAttributes).equals(releaseRequest.attributes)
               && Arrays.equals(ACQUISITION_TAG, releaseRequest.tag),
            "divestiture cancellation restored a release request with the wrong payload");
         requester.cancelAttributeOwnershipAcquisition(object, requesterAttributes);
         await(requester, callbackModel, objectClassName,
            () -> requesterRecorder.cancellations().size() == 1,
            "requester acquisition cancellation was not confirmed");
         OwnershipEvent cancellation = requesterRecorder.cancellations().get(0);
         JavaTckSupport.check(object.equals(cancellation.object)
               && new HashSet<>(requesterAttributes).equals(cancellation.attributes)
               && owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && !requester.isAttributeOwnedByFederate(object, requesterAttribute),
            "acquisition-cancellation callback or final ownership state was incorrect");

         requester.resignFederationExecution(ResignAction.NO_ACTION);
         requesterJoined = false;
         owner.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         ownerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
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
         if (requesterConnected) {
            try { requester.disconnect(); }
            catch (Exception ignored) { }
         }
         if (ownerConnected) {
            try { owner.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         String objectClassName, Condition condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && !condition.isTrue()) {
         service(ambassador, model, objectClassName);
         Thread.yield();
      }
      JavaTckSupport.check(condition.isTrue(), "timed out waiting for " + description);
   }

   private static void service(RTIambassador ambassador, CallbackModel model,
         String objectClassName) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) {
         ambassador.evokeMultipleCallbacks(0.0, 1.0);
      } else {
         ambassador.getObjectClassHandle(objectClassName);
      }
   }

   private interface Condition {
      boolean isTrue();
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
      private final List<OwnershipEvent> releaseRequests = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> divestitureConfirmations = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> cancellations = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("requestAttributeOwnershipRelease".equals(name)) {
                  releaseRequests.add(event(arguments));
               } else if ("requestDivestitureConfirmation".equals(name)) {
                  divestitureConfirmations.add(event(arguments));
               } else if ("confirmAttributeOwnershipAcquisitionCancellation".equals(name)) {
                  cancellations.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], null));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK divestiture callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }

      private static OwnershipEvent event(Object[] arguments) {
         return new OwnershipEvent((ObjectInstanceHandle) arguments[0],
            (AttributeHandleSet) arguments[1], (byte[]) arguments[2]);
      }

      private List<OwnershipEvent> releaseRequests() { return List.copyOf(releaseRequests); }

      private List<OwnershipEvent> divestitureConfirmations() {
         return List.copyOf(divestitureConfirmations);
      }

      private List<OwnershipEvent> cancellations() { return List.copyOf(cancellations); }

      private void clearOwnershipEvents() {
         releaseRequests.clear();
         divestitureConfirmations.clear();
         cancellations.clear();
      }
   }
}
