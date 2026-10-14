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

/** Portable callback-enable gating for ownership-assumption notifications. */
final class CallbackControlsOwnershipAssumptionTck {
   private static final byte[] ACQUISITION_TAG = new byte[] {0x41, 0x53, 0x53};

   private CallbackControlsOwnershipAssumptionTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String attributeName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador peer = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder peerRecorder = new Recorder();
      String federation = "java-tck-callback-ownership-assumption-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean peerConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean peerJoined = false;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         peer.connect(peerRecorder.proxy(), callbackModel);
         peerConnected = true;
         owner.createFederationExecution(federation, fom);
         created = true;
         owner.joinFederationExecution("java-tck-ownership-owner", federation);
         ownerJoined = true;
         peer.joinFederationExecution("java-tck-ownership-peer", federation);
         peerJoined = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle peerClass = peer.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle peerAttribute = peer.getAttributeHandle(peerClass, attributeName);
         AttributeHandle privilegeToDelete = owner.getAttributeHandle(
            ownerClass, "HLAprivilegeToDeleteObject");
         JavaTckSupport.check(ownerClass.equals(peerClass)
               && ownerAttribute.equals(peerAttribute)
               && privilegeToDelete != null,
            "standard FOM lookup returned invalid ownership-assumption handles");

         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet peerAttributes = JavaTckSupport.attributeSet(peer, peerAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         peer.publishObjectClassAttributes(peerClass, peerAttributes);
         peer.subscribeObjectClassAttributes(peerClass, peerAttributes);
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "ownership-assumption registration returned a null object handle");
         await(peer, callbackModel, objectClassName,
            () -> peerRecorder.discoveries.contains(object),
            "peer did not discover the registered object");

         peerRecorder.clearOwnershipEvents();
         peer.disableCallbacks();
         owner.resignFederationExecution(ResignAction.UNCONDITIONALLY_DIVEST_ATTRIBUTES);
         ownerJoined = false;
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(peer.evokeMultipleCallbacks(0.0, 1.0),
               "evoked callback control did not admit the ownership-assumption event");
         } else {
            service(peer, callbackModel, objectClassName);
         }
         JavaTckSupport.check(peerRecorder.assumptions().isEmpty(),
            "disabled callbacks exposed a requestAttributeOwnershipAssumption callback");

         peer.enableCallbacks();
         await(peer, callbackModel, objectClassName,
            () -> !peerRecorder.assumptions().isEmpty(),
            "re-enabled ownership-assumption callback did not arrive");
         List<OwnershipEvent> assumptions = peerRecorder.assumptions();
         OwnershipEvent assumption = assumptions.get(0);
         JavaTckSupport.check(assumptions.size() == 1
               && object.equals(assumption.object)
               && new HashSet<>(peerAttributes).equals(assumption.attributes)
               && assumption.tag.length == 0,
            "ownership-assumption callback lost its object, attributes, or empty tag");

         peerRecorder.clearOwnershipEvents();
         peer.attributeOwnershipAcquisition(object, peerAttributes, ACQUISITION_TAG.clone());
         await(peer, callbackModel, objectClassName,
            () -> !peerRecorder.acquisitions().isEmpty(),
            "peer did not receive attributeOwnershipAcquisitionNotification");
         List<OwnershipEvent> acquisitions = peerRecorder.acquisitions();
         OwnershipEvent acquisition = acquisitions.get(0);
         JavaTckSupport.check(acquisitions.size() == 1
               && object.equals(acquisition.object)
               && new HashSet<>(peerAttributes).equals(acquisition.attributes)
               && Arrays.equals(ACQUISITION_TAG, acquisition.tag)
               && peer.isAttributeOwnedByFederate(object, peerAttribute),
            "ownership acquisition callback or standard ownership query returned wrong state");

         peer.resignFederationExecution(ResignAction.UNCONDITIONALLY_DIVEST_ATTRIBUTES);
         peerJoined = false;
         peer.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (peerJoined) {
            try { peer.resignFederationExecution(ResignAction.UNCONDITIONALLY_DIVEST_ATTRIBUTES); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.UNCONDITIONALLY_DIVEST_ATTRIBUTES); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { peer.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (peerConnected) {
            try { peer.disconnect(); }
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
         this.tag = tag.clone();
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> assumptions = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> acquisitions = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("requestAttributeOwnershipAssumption".equals(name)) {
                  assumptions.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (byte[]) arguments[2]));
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)) {
                  acquisitions.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (byte[]) arguments[2]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK ownership callback recorder";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }

      private List<OwnershipEvent> assumptions() {
         return List.copyOf(assumptions);
      }

      private List<OwnershipEvent> acquisitions() {
         return List.copyOf(acquisitions);
      }

      private void clearOwnershipEvents() {
         assumptions.clear();
         acquisitions.clear();
      }
   }
}
