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

/** Portable callback-enable gating for acquisition-cancellation completion. */
final class CallbackControlsOwnershipAcquisitionCancellationTck {
   private static final byte[] REQUEST_TAG = new byte[] {0x43, 0x41, 0x4e};

   private CallbackControlsOwnershipAcquisitionCancellationTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String attributeName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      String federation = "java-tck-callback-ownership-cancellation-" + UUID.randomUUID();
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
         owner.joinFederationExecution("java-tck-cancellation-owner", federation);
         ownerJoined = true;
         requester.joinFederationExecution("java-tck-cancellation-requester", federation);
         requesterJoined = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle requesterClass = requester.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle requesterAttribute =
            requester.getAttributeHandle(requesterClass, attributeName);
         JavaTckSupport.check(ownerClass.equals(requesterClass)
               && ownerAttribute.equals(requesterAttribute),
            "standard FOM lookups returned mismatched ownership-cancellation handles");
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet requesterAttributes =
            JavaTckSupport.attributeSet(requester, requesterAttribute);

         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         requester.publishObjectClassAttributes(requesterClass, requesterAttributes);
         requester.subscribeObjectClassAttributes(requesterClass, requesterAttributes);
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "ownership-cancellation registration returned a null object handle");
         await(requester, callbackModel, objectClassName,
            () -> requesterRecorder.discoveries.contains(object),
            "requester did not discover the registered object");

         ownerRecorder.clearOwnershipEvents();
         requesterRecorder.clearOwnershipEvents();
         requester.attributeOwnershipAcquisition(
            object, requesterAttributes, REQUEST_TAG.clone());
         await(owner, callbackModel, objectClassName,
            () -> ownerRecorder.releaseRequests().size() == 1,
            "owner did not receive requestAttributeOwnershipRelease");
         List<OwnershipEvent> releaseRequests = ownerRecorder.releaseRequests();
         OwnershipEvent releaseRequest = releaseRequests.get(0);
         JavaTckSupport.check(releaseRequests.size() == 1
               && object.equals(releaseRequest.object)
               && new HashSet<>(ownerAttributes).equals(releaseRequest.attributes)
               && Arrays.equals(REQUEST_TAG, releaseRequest.tag),
            "ownership-release request lost its object, attributes, or acquisition tag");

         requester.disableCallbacks();
         requester.cancelAttributeOwnershipAcquisition(object, requesterAttributes);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(requester.evokeMultipleCallbacks(0.0, 1.0),
               "evoked callback control did not admit the ownership-cancellation event");
         } else {
            service(requester, callbackModel, objectClassName);
         }
         JavaTckSupport.check(requesterRecorder.cancellations().isEmpty(),
            "disabled callbacks exposed an ownership-acquisition cancellation callback");
         checkOwnership(owner, requester, object, ownerAttribute, requesterAttribute,
            "cancellation while callbacks were disabled");

         requester.enableCallbacks();
         await(requester, callbackModel, objectClassName,
            () -> requesterRecorder.cancellations().size() == 1,
            "re-enabled ownership-acquisition cancellation callback did not arrive");
         List<OwnershipEvent> cancellations = requesterRecorder.cancellations();
         OwnershipEvent cancellation = cancellations.get(0);
         JavaTckSupport.check(cancellations.size() == 1
               && object.equals(cancellation.object)
               && new HashSet<>(requesterAttributes).equals(cancellation.attributes)
               && cancellation.tag == null,
            "cancellation completion lost its object/attributes or unexpectedly included a tag");
         checkOwnership(owner, requester, object, ownerAttribute, requesterAttribute,
            "cancellation completion callback");

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

   private static void checkOwnership(RTIambassador owner, RTIambassador requester,
         ObjectInstanceHandle object, AttributeHandle ownerAttribute,
         AttributeHandle requesterAttribute, String phase) throws Exception {
      JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute)
            && !requester.isAttributeOwnedByFederate(object, requesterAttribute),
         phase + " changed the standard ownership state");
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
                  releaseRequests.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (byte[]) arguments[2]));
               } else if ("confirmAttributeOwnershipAcquisitionCancellation".equals(name)) {
                  cancellations.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], null));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK ownership-cancellation callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }

      private List<OwnershipEvent> releaseRequests() {
         return List.copyOf(releaseRequests);
      }

      private List<OwnershipEvent> cancellations() {
         return List.copyOf(cancellations);
      }

      private void clearOwnershipEvents() {
         releaseRequests.clear();
         cancellations.clear();
      }
   }
}
