package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;
import java.util.function.BooleanSupplier;

/** Tests cancellation of pending ownership acquisition after adapter-induced connection loss. */
final class ConnectionLossAutomaticCancelPendingAcquisitionTck {
   private ConnectionLossAutomaticCancelPendingAcquisitionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel,
         RtiConfiguration ownerConfiguration, RtiConfiguration lostConfiguration,
         RtiConfiguration survivorConfiguration, String federation, String ownerName,
         String lostName, String federateType, String readyMarkerPath,
         String completionMarkerPath, long timeoutMillis) throws Exception {
      JavaTckSupport.check(readyMarkerPath != null && !readyMarkerPath.isEmpty(),
         "adapter connection-loss readiness marker is required");
      JavaTckSupport.check(completionMarkerPath != null && !completionMarkerPath.isEmpty(),
         "adapter connection-loss completion marker is required");
      JavaTckSupport.check(timeoutMillis > 0, "connection-loss timeout must be positive");
      JavaTckSupport.check(federation != null && !federation.isEmpty()
            && ownerName != null && !ownerName.isEmpty()
            && lostName != null && !lostName.isEmpty()
            && federateType != null && !federateType.isEmpty(),
         "adapter connection-loss federation and member names are required");

      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador lost = factory.getRtiAmbassador();
      RTIambassador survivor = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder lostCallbacks = new Recorder();
      Recorder survivorCallbacks = new Recorder();
      AtomicReference<String> loss = new AtomicReference<>();
      AtomicInteger lossCallbackCount = new AtomicInteger();
      boolean ownerConnected = false;
      boolean survivorConnected = false;
      boolean ownerJoined = false;
      boolean survivorJoined = false;
      boolean ownerResigned = false;
      boolean survivorResigned = false;
      String stage = "connect owner";
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel, ownerConfiguration);
         ownerConnected = true;
         stage = "connect lost member";
         lost.connect(connectionLossCallbacks(lostCallbacks, loss, lossCallbackCount),
            callbackModel, lostConfiguration);
         stage = "create and join federation";
         owner.createFederationExecution(federation, fom, timeImplementation);
         owner.joinFederationExecution(ownerName, federateType, federation);
         ownerJoined = true;
         lost.joinFederationExecution(lostName, federateType, federation);
         survivor.connect(survivorCallbacks.proxy(), callbackModel, survivorConfiguration);
         survivorConnected = true;
         survivor.joinFederationExecution(lostName + "-survivor", federateType, federation);
         survivorJoined = true;

         stage = "resolve object and attribute handles";
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

         stage = "register and discover object";
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null, "owner registration returned no object handle");
         await(() -> lostCallbacks.discoveries.stream()
               .anyMatch(discovery -> object.equals(discovery.object)), lost, callbackModel,
            timeoutMillis, "lost member did not discover the object");
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(lost);
         List<Discovery> lostDiscoveries = lostCallbacks.discoveries.stream()
            .filter(discovery -> object.equals(discovery.object)).toList();
         JavaTckSupport.check(lostDiscoveries.size() == 1
               && lostClass.equals(lostDiscoveries.get(0).objectClass),
            "lost member discovery changed the object, class, or callback count");
         JavaTckSupport.check(!lost.isAttributeOwnedByFederate(object, lostAttribute),
            "lost member unexpectedly owns the attribute before connection loss");
         await(() -> survivorCallbacks.discoveries.stream()
               .anyMatch(discovery -> object.equals(discovery.object)), survivor, callbackModel,
            timeoutMillis, "survivor did not discover the object");
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(survivor);
         List<Discovery> survivorDiscoveries = survivorCallbacks.discoveries.stream()
            .filter(discovery -> object.equals(discovery.object)).toList();
         JavaTckSupport.check(survivorDiscoveries.size() == 1
               && survivorClass.equals(survivorDiscoveries.get(0).objectClass),
            "survivor discovery changed the object, class, or callback count");
         JavaTckSupport.check(!survivor.isAttributeOwnedByFederate(object, survivorAttribute),
            "survivor unexpectedly owns the attribute before connection loss");

         byte[] acquisitionTag = new byte[] {(byte) 0xd6, 0x25};
         byte[] divestitureTag = new byte[] {(byte) 0xd7, 0x25};
         lost.attributeOwnershipAcquisition(object, lostAttributes, acquisitionTag.clone());
         stage = "configure pending-acquisition resignation";
         JavaTckSupport.check(ownerCallbacks.releaseRequests.isEmpty(),
            "pending acquisition produced an owner release callback before fault cleanup");
         lost.setAutomaticResignDirective(ResignAction.CANCEL_PENDING_OWNERSHIP_ACQUISITIONS);
         JavaTckSupport.check(lost.getAutomaticResignDirective()
               == ResignAction.CANCEL_PENDING_OWNERSHIP_ACQUISITIONS,
            "pending-acquisition cancellation directive did not round-trip");
         writeMarker(Path.of(readyMarkerPath), "ready\n");

         stage = "await connection loss callback";
         long deadline = System.nanoTime() + timeoutMillis * 1_000_000L;
         while (loss.get() == null && System.nanoTime() < deadline) {
            if (callbackModel == CallbackModel.HLA_EVOKED) {
               try {
                  lost.evokeMultipleCallbacks(0.0, 0.05);
               } catch (Exception ignored) {
                  // The transport fixture can surface loss before queuing connectionLost.
               }
            }
            if (loss.get() == null) Thread.sleep(1L);
         }
         JavaTckSupport.check(loss.get() != null && !loss.get().isEmpty(),
            "timed out waiting for a non-empty connectionLost callback");
         JavaTckSupport.check(lossCallbackCount.get() == 1,
            "connection loss delivered duplicate connectionLost callbacks");
         writeMarker(Path.of(completionMarkerPath), "callback-ok\n");

         // Pump the owner after cancellation and prove no stale release request survived.
         stage = "pump owner after cancellation";
         for (int pass = 0; callbackModel == CallbackModel.HLA_EVOKED && pass < 8; pass++) {
            owner.evokeCallback(0.0);
         }
         JavaTckSupport.check(ownerCallbacks.releaseRequests.isEmpty(),
            "automatic cancellation delivered stale owner release work");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute),
            "automatic cancellation changed the current owner's ownership");

         owner.unconditionalAttributeOwnershipDivestiture(
            object, ownerAttributes, divestitureTag.clone());
         stage = "await survivor ownership assumption";
         await(() -> !survivorCallbacks.assumptions.isEmpty(), survivor, callbackModel,
            timeoutMillis, "survivor ownership assumption after owner divestiture");
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(survivor);
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

         survivor.resignFederationExecution(ResignAction.NO_ACTION);
         stage = "resign federation members";
         survivorJoined = false;
         survivorResigned = true;
         owner.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         ownerJoined = false;
         ownerResigned = true;
      } catch (Exception error) {
         throw new Exception("pending-acquisition conversion failed during " + stage
               + ": " + error.getMessage(), error);
      } finally {
          if (survivorJoined && !survivorResigned) {
            try { survivor.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
          if (ownerJoined && !ownerResigned) {
            try { owner.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         }
          if (ownerConnected) {
             try { owner.disconnect(); } catch (Exception ignored) { }
          }
          if (survivorConnected) {
             try { survivor.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static FederateAmbassador connectionLossCallbacks(
         Recorder recorder, AtomicReference<String> loss, AtomicInteger callbackCount) {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if ("connectionLost".equals(method.getName())
                  && arguments != null && arguments.length == 1) {
               callbackCount.incrementAndGet();
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

   private static final class Discovery {
      private final ObjectInstanceHandle object;
      private final ObjectClassHandle objectClass;

      private Discovery(ObjectInstanceHandle object, ObjectClassHandle objectClass) {
         this.object = object;
         this.objectClass = objectClass;
      }
   }

   private static final class Recorder {
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();
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
               && arguments != null && arguments.length >= 2) {
            discoveries.add(new Discovery((ObjectInstanceHandle) arguments[0],
               (ObjectClassHandle) arguments[1]));
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
