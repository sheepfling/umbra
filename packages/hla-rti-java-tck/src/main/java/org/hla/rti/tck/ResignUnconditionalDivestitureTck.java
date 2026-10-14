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
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks unconditional resign-time divestiture and a peer's subsequent acquisition. */
final class ResignUnconditionalDivestitureTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private ResignUnconditionalDivestitureTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador peer = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder peerCallbacks = new Recorder();
      String federation = "java-tck-resign-unconditional-divestiture-" + UUID.randomUUID();
      boolean[] connected = new boolean[2], joined = new boolean[2];
      boolean created = false;
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel);
         connected[0] = true;
         peer.connect(peerCallbacks.proxy(), callbackModel);
         connected[1] = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-resign-divest-owner", federation);
         joined[0] = true;
         peer.joinFederationExecution("java-tck-resign-divest-peer", federation);
         joined[1] = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle peerClass = peer.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle peerAttribute = peer.getAttributeHandle(peerClass, attributeName);
         AttributeHandle privilegeToDelete = owner.getAttributeHandle(ownerClass,
            "HLAprivilegeToDeleteObject");
         JavaTckSupport.check(objectClassName.equals(owner.getObjectClassName(ownerClass))
               && objectClassName.equals(peer.getObjectClassName(peerClass))
               && attributeName.equals(owner.getAttributeName(ownerClass, ownerAttribute))
               && attributeName.equals(peer.getAttributeName(peerClass, peerAttribute))
               && "HLAprivilegeToDeleteObject".equals(owner.getAttributeName(ownerClass,
                  privilegeToDelete)),
            "adapter FOM or standard privilege attribute lookup did not round-trip");
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet peerAttributes = JavaTckSupport.attributeSet(peer, peerAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         peer.publishObjectClassAttributes(peerClass, peerAttributes);
         peer.subscribeObjectClassAttributes(peerClass, peerAttributes);

         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         await(peer, callbackModel, () -> peerCallbacks.discoveries.contains(object),
            "peer object discovery");
         JavaTckSupport.expectFailure(() -> owner.resignFederationExecution(ResignAction.NO_ACTION),
            "FederateOwnsAttributes", "resigning while still owning published attributes");

         peerCallbacks.assumptions.clear();
         owner.resignFederationExecution(ResignAction.UNCONDITIONALLY_DIVEST_ATTRIBUTES);
         joined[0] = false;
         await(peer, callbackModel, () -> !peerCallbacks.assumptions.isEmpty(),
            "unconditional resign-time ownership assumption");
         OwnershipEvent assumption = peerCallbacks.assumptions.get(0);
         JavaTckSupport.check(object.equals(assumption.object)
               && assumption.attributes.size() == 2
               && assumption.attributes.contains(peerAttribute)
               && assumption.attributes.contains(privilegeToDelete)
               && assumption.tag.length == 0,
            "resignation ownership assumption contained the wrong attributes, object, or tag");

         byte[] acquisitionTag = new byte[] {(byte) 0xd1, 0x25};
         peerCallbacks.acquisitions.clear();
         peer.attributeOwnershipAcquisition(object, peerAttributes, acquisitionTag.clone());
         await(peer, callbackModel, () -> !peerCallbacks.acquisitions.isEmpty(),
            "peer acquisition notification");
         OwnershipEvent acquisition = peerCallbacks.acquisitions.get(0);
         JavaTckSupport.check(object.equals(acquisition.object)
               && acquisition.attributes.size() == 1
               && acquisition.attributes.contains(peerAttribute)
               && Arrays.equals(acquisitionTag, acquisition.tag)
               && peer.isAttributeOwnedByFederate(object, peerAttribute),
            "peer acquisition notification or ownership query returned incorrect metadata");

         peer.resignFederationExecution(ResignAction.UNCONDITIONALLY_DIVEST_ATTRIBUTES);
         joined[1] = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (joined[1]) {
            try { peer.resignFederationExecution(ResignAction.UNCONDITIONALLY_DIVEST_ATTRIBUTES); }
            catch (Exception ignored) { }
         }
         if (joined[0]) {
            try { owner.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected[1]) { try { peer.disconnect(); } catch (Exception ignored) { } }
         if (connected[0]) { try { owner.disconnect(); } catch (Exception ignored) { } }
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

   private static final class OwnershipEvent {
      private final ObjectInstanceHandle object;
      private final AttributeHandleSet attributes;
      private final byte[] tag;

      private OwnershipEvent(ObjectInstanceHandle object, AttributeHandleSet attributes,
            byte[] tag) {
         this.object = object;
         this.attributes = attributes.clone();
         this.tag = tag.clone();
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> assumptions = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> acquisitions = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
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
                  if ("toString".equals(name)) return "Java TCK resign-divestiture callbacks";
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
