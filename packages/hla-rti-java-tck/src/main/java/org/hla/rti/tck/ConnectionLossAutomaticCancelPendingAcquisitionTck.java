package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.atomic.AtomicReference;
import java.util.function.BooleanSupplier;

/** Tests cancellation of pending ownership acquisition after adapter-induced connection loss. */
final class ConnectionLossAutomaticCancelPendingAcquisitionTck {
   private ConnectionLossAutomaticCancelPendingAcquisitionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         CallbackModel callbackModel, String readyMarkerPath, String completionMarkerPath,
         long timeoutMillis) throws Exception {
      JavaTckSupport.check(readyMarkerPath != null && !readyMarkerPath.isEmpty(),
         "adapter connection-loss readiness marker is required");
      JavaTckSupport.check(completionMarkerPath != null && !completionMarkerPath.isEmpty(),
         "adapter connection-loss completion marker is required");
      JavaTckSupport.check(timeoutMillis > 0, "connection-loss timeout must be positive");

      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador lost = factory.getRtiAmbassador();
      RTIambassador survivor = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder lostCallbacks = new Recorder();
      Recorder survivorCallbacks = new Recorder();
      AtomicReference<String> loss = new AtomicReference<>();
      String federation = "java-tck-loss-cancel-acquisition-" + UUID.randomUUID();
      boolean[] connected = new boolean[3];
      boolean created = false;
      boolean[] joined = new boolean[3];
      boolean ownerResigned = false;
      boolean survivorResigned = false;
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel);
         connected[0] = true;
         lost.connect(connectionLossCallbacks(lostCallbacks, loss), callbackModel);
         connected[1] = true;
         survivor.connect(survivorCallbacks.proxy(), callbackModel);
         connected[2] = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("loss-cancel-owner", federation);
         joined[0] = true;
         lost.joinFederationExecution("loss-cancel-member", federation);
         joined[1] = true;
         survivor.joinFederationExecution("loss-cancel-survivor", federation);
         joined[2] = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle lostClass = lost.getObjectClassHandle(objectClassName);
         ObjectClassHandle survivorClass = survivor.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle lostAttribute = lost.getAttributeHandle(lostClass, attributeName);
         AttributeHandle survivorAttribute = survivor.getAttributeHandle(
            survivorClass, attributeName);
         AttributeHandleSet ownerAttributes =
            JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet lostAttributes = JavaTckSupport.attributeSet(lost, lostAttribute);
         AttributeHandleSet survivorAttributes =
            JavaTckSupport.attributeSet(survivor, survivorAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         lost.publishObjectClassAttributes(lostClass, lostAttributes);
         lost.subscribeObjectClassAttributes(lostClass, lostAttributes);
         survivor.publishObjectClassAttributes(survivorClass, survivorAttributes);
         survivor.subscribeObjectClassAttributes(survivorClass, survivorAttributes);

         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null, "owner registration returned no object handle");
         await(() -> lostCallbacks.discoveries.contains(object), lost, callbackModel,
            timeoutMillis, "lost member did not discover the object");
         await(() -> survivorCallbacks.discoveries.contains(object), survivor, callbackModel,
            timeoutMillis, "survivor did not discover the object");

         byte[] acquisitionTag = new byte[] {(byte) 0xd6, 0x25};
         byte[] divestitureTag = new byte[] {(byte) 0xd7, 0x25};
         lost.attributeOwnershipAcquisition(object, lostAttributes, acquisitionTag.clone());
         JavaTckSupport.check(ownerCallbacks.releaseRequests.isEmpty(),
            "pending acquisition produced an owner release callback before fault cleanup");
         lost.setAutomaticResignDirective(ResignAction.CANCEL_PENDING_OWNERSHIP_ACQUISITIONS);
         JavaTckSupport.check(lost.getAutomaticResignDirective()
               == ResignAction.CANCEL_PENDING_OWNERSHIP_ACQUISITIONS,
            "pending-acquisition cancellation directive did not round-trip");
         InteractionClassHandle signalClass =
            survivor.getInteractionClassHandle(interactionClassName);
         JavaTckSupport.check(signalClass != null,
            "survivor could not resolve the adapter-configured signal interaction");
         survivor.publishInteractionClass(signalClass);
         writeMarker(Path.of(readyMarkerPath), "ready\n");

         long deadline = System.nanoTime() + timeoutMillis * 1_000_000L;
         while (loss.get() == null && System.nanoTime() < deadline) {
            try {
               lost.evokeMultipleCallbacks(0.0, 0.05);
            } catch (Exception ignored) {
               // The transport fixture can surface loss before queuing connectionLost.
            }
            if (loss.get() == null) Thread.sleep(1L);
         }
         JavaTckSupport.check(loss.get() != null && !loss.get().isEmpty(),
            "timed out waiting for a non-empty connectionLost callback");

         // Signal fixture progress after observing loss, as in the other adapter-backed cases.
         survivor.sendInteraction(signalClass,
            survivor.getParameterHandleValueMapFactory().create(0), new byte[0]);
         try {
            lost.disconnect();
         } catch (Exception ignored) {
            // Local cleanup may already have followed the transport-loss callback.
         }
         connected[1] = false;
         joined[1] = false;

         // Pump the owner after cancellation and prove no stale release request survived.
         for (int pass = 0; pass < 8; pass++) {
            if (callbackModel == CallbackModel.HLA_EVOKED) {
               JavaTckSupport.drain(owner);
            } else {
               // The adapter-managed immediate transport may need an official
               // callback-service call to observe unsolicited frames.
               owner.evokeCallback(0.0);
            }
         }
         JavaTckSupport.check(ownerCallbacks.releaseRequests.isEmpty(),
            "automatic cancellation delivered stale owner release work");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute),
            "automatic cancellation changed the current owner's ownership");

         owner.unconditionalAttributeOwnershipDivestiture(
            object, ownerAttributes, divestitureTag.clone());
         await(() -> !survivorCallbacks.assumptions.isEmpty(), survivor, callbackModel,
            timeoutMillis, "survivor ownership assumption after owner divestiture");
         List<OwnershipEvent> assumptions = List.copyOf(survivorCallbacks.assumptions);
         JavaTckSupport.check(assumptions.size() == 1,
            "owner divestiture delivered duplicate survivor assumption callbacks");
         OwnershipEvent assumption = assumptions.get(0);
         JavaTckSupport.check(object.equals(assumption.object)
               && assumption.attributes.size() == 1
               && assumption.attributes.contains(survivorAttribute)
               && Arrays.equals(divestitureTag, assumption.tag),
            "survivor assumption after cancellation had incorrect object, attributes, or tag");
         JavaTckSupport.check(!survivor.isAttributeOwnedByFederate(object, survivorAttribute),
            "survivor acquired ownership before making an acquisition request");

         writeMarker(Path.of(completionMarkerPath), "callback-ok\n");
         survivor.resignFederationExecution(ResignAction.NO_ACTION);
         joined[2] = false;
         survivorResigned = true;
         owner.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         joined[0] = false;
         ownerResigned = true;
      } finally {
         if (joined[2] && !survivorResigned) {
            try { survivor.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (joined[0] && !ownerResigned) {
            try { owner.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         for (int index = 2; index >= 0; index--) {
            if (connected[index]) {
               try {
                  (index == 0 ? owner : index == 1 ? lost : survivor).disconnect();
               } catch (Exception ignored) { }
            }
         }
      }
   }

   private static FederateAmbassador connectionLossCallbacks(
         Recorder recorder, AtomicReference<String> loss) {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if ("connectionLost".equals(method.getName())
                  && arguments != null && arguments.length == 1) {
               loss.set((String) arguments[0]);
            }
            return recorder.invoke(proxy, method, arguments);
         });
   }

   private static void writeMarker(Path path, String contents) throws Exception {
      Path parent = path.toAbsolutePath().getParent();
      if (parent != null) Files.createDirectories(parent);
      Files.write(path, contents.getBytes(StandardCharsets.UTF_8));
   }

   private static void await(BooleanSupplier condition, RTIambassador ambassador,
         CallbackModel callbackModel, long timeoutMillis, String failure) throws Exception {
      long deadline = System.nanoTime() + timeoutMillis * 1_000_000L;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), failure);
   }

   private static final class OwnershipEvent {
      private final ObjectInstanceHandle object;
      private final AttributeHandleSet attributes;
      private final byte[] tag;

      private OwnershipEvent(ObjectInstanceHandle object, AttributeHandleSet attributes,
            byte[] tag) {
         this.object = object;
         this.attributes = attributes.clone();
         this.tag = tag == null ? new byte[0] : tag.clone();
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> assumptions = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> releaseRequests = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            this::invoke);
      }

      private Object invoke(Object proxy, java.lang.reflect.Method method, Object[] arguments) {
         String name = method.getName();
         if ("discoverObjectInstance".equals(name)
               && arguments != null && arguments.length >= 1) {
            discoveries.add((ObjectInstanceHandle) arguments[0]);
         } else if ("requestAttributeOwnershipAssumption".equals(name)
               && arguments != null && arguments.length == 3) {
            assumptions.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
               (AttributeHandleSet) arguments[1], (byte[]) arguments[2]));
         } else if ("requestAttributeOwnershipRelease".equals(name)
               && arguments != null && arguments.length == 3) {
            releaseRequests.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
               (AttributeHandleSet) arguments[1], (byte[]) arguments[2]));
         }
         if (method.getDeclaringClass() == Object.class) {
            if ("toString".equals(name)) return "Java TCK connection-loss ownership callbacks";
            if ("hashCode".equals(name)) return System.identityHashCode(proxy);
            if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
         }
         return null;
      }
   }
}
