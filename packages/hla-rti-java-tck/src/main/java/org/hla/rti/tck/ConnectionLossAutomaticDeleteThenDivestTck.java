package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
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
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;
import java.util.function.BooleanSupplier;

/** Tests adapter-induced connection loss with automatic delete-then-divest cleanup. */
final class ConnectionLossAutomaticDeleteThenDivestTck {
   private ConnectionLossAutomaticDeleteThenDivestTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel,
         RtiConfiguration survivorConfiguration, RtiConfiguration lostConfiguration,
         String federation, String survivorName, String lostName, String federateType,
         String readyMarkerPath, String completionMarkerPath, long timeoutMillis)
         throws Exception {
      JavaTckSupport.check(readyMarkerPath != null && !readyMarkerPath.isEmpty(),
         "adapter connection-loss readiness marker is required");
      JavaTckSupport.check(completionMarkerPath != null && !completionMarkerPath.isEmpty(),
         "adapter connection-loss completion marker is required");
      JavaTckSupport.check(timeoutMillis > 0, "connection-loss timeout must be positive");
      JavaTckSupport.check(federation != null && !federation.isEmpty()
            && survivorName != null && !survivorName.isEmpty()
            && lostName != null && !lostName.isEmpty()
            && federateType != null && !federateType.isEmpty(),
         "adapter connection-loss federation and member names are required");

      RTIambassador survivor = factory.getRtiAmbassador();
      RTIambassador lost = factory.getRtiAmbassador();
      Recorder survivorCallbacks = new Recorder();
      Recorder lostCallbacks = new Recorder();
      AtomicReference<String> loss = new AtomicReference<>();
      AtomicInteger lossCallbackCount = new AtomicInteger();
      boolean survivorConnected = false;
      boolean survivorJoined = false;
      try {
         survivor.connect(survivorCallbacks.proxy(), callbackModel, survivorConfiguration);
         survivorConnected = true;
         lost.connect(lostCallbacks.proxy(loss, lossCallbackCount), callbackModel,
            lostConfiguration);
         survivor.createFederationExecution(federation, fom, timeImplementation);
         survivor.joinFederationExecution(survivorName, federateType, federation);
         survivorJoined = true;
         FederateHandle lostFederate = lost.joinFederationExecution(
            lostName, federateType, federation);
         JavaTckSupport.check(lostFederate != null,
            "connection-loss member join returned no federate handle");

         ObjectClassHandle survivorClass = survivor.getObjectClassHandle(objectClassName);
         ObjectClassHandle lostClass = lost.getObjectClassHandle(objectClassName);
         AttributeHandle survivorAttribute = survivor.getAttributeHandle(
            survivorClass, attributeName);
         AttributeHandle lostAttribute = lost.getAttributeHandle(lostClass, attributeName);
         AttributeHandleSet survivorAttributes =
            JavaTckSupport.attributeSet(survivor, survivorAttribute);
         AttributeHandleSet lostAttributes = JavaTckSupport.attributeSet(lost, lostAttribute);
         survivor.publishObjectClassAttributes(survivorClass, survivorAttributes);
         lost.publishObjectClassAttributes(lostClass, lostAttributes);
         survivor.subscribeObjectClassAttributes(survivorClass, survivorAttributes);
         lost.subscribeObjectClassAttributes(lostClass, lostAttributes);

         ObjectInstanceHandle retainedObject = survivor.registerObjectInstance(survivorClass);
         String retainedName = survivor.getObjectInstanceName(retainedObject);
         await(() -> lostCallbacks.discoveries.stream()
               .anyMatch(discovery -> retainedObject.equals(discovery.object)), lost,
            callbackModel, timeoutMillis, "lost member did not discover retained object");
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(lost);
         List<Discovery> retainedDiscoveries = lostCallbacks.discoveries.stream()
            .filter(discovery -> retainedObject.equals(discovery.object)).toList();
         JavaTckSupport.check(retainedDiscoveries.size() == 1
               && lostClass.equals(retainedDiscoveries.get(0).objectClass),
            "lost member discovery changed the retained object, class, or callback count");
         byte[] divestitureTag = new byte[] {(byte) 0xD4, (byte) 0x25};
         survivor.unconditionalAttributeOwnershipDivestiture(
            retainedObject, survivorAttributes, divestitureTag);
         await(() -> lostCallbacks.assumptions.size() == 1, lost,
            callbackModel, timeoutMillis,
            "lost member did not receive the retained-attribute assumption");
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(lost);
         JavaTckSupport.check(lostCallbacks.assumptions.size() == 1,
            "setup divestiture delivered duplicate ownership-assumption callbacks");
         OwnershipEvent divestiture = lostCallbacks.assumptions.get(0);
         JavaTckSupport.check(retainedObject.equals(divestiture.object)
               && divestiture.attributes.size() == 1
               && divestiture.attributes.contains(lostAttribute)
               && java.util.Arrays.equals(divestitureTag, divestiture.tag),
            "setup divestiture changed its object, attribute, or tag");
         byte[] acquisitionTag = new byte[] {(byte) 0xD5, (byte) 0x25};
         lost.attributeOwnershipAcquisitionIfAvailable(
            retainedObject, lostAttributes, acquisitionTag);
         await(() -> lostCallbacks.acquisitions.size() == 1, lost,
            callbackModel, timeoutMillis, "lost member did not acquire retained attribute");
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(lost);
         JavaTckSupport.check(lostCallbacks.acquisitions.size() == 1,
            "setup acquisition delivered duplicate ownership notifications");
         OwnershipEvent acquisition = lostCallbacks.acquisitions.get(0);
         JavaTckSupport.check(retainedObject.equals(acquisition.object)
               && acquisition.attributes.size() == 1
               && acquisition.attributes.contains(lostAttribute)
               && java.util.Arrays.equals(acquisitionTag, acquisition.tag)
               && lost.isAttributeOwnedByFederate(retainedObject, lostAttribute),
            "retained attribute acquisition callback did not preserve object, attribute, and tag");

         ObjectInstanceHandle departingObject = lost.registerObjectInstance(lostClass);
         JavaTckSupport.check(departingObject != null && !departingObject.equals(retainedObject),
            "lost member registered an invalid or duplicate departing object");
         String deletedObjectName = lost.getObjectInstanceName(departingObject);
         JavaTckSupport.check(deletedObjectName != null && !deletedObjectName.isEmpty(),
            "lost member's departing object has no name");
         await(() -> survivorCallbacks.discoveries.stream()
               .anyMatch(discovery -> departingObject.equals(discovery.object)), survivor,
            callbackModel, timeoutMillis, "survivor did not discover departing object");
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(survivor);
         List<Discovery> departingDiscoveries = survivorCallbacks.discoveries.stream()
            .filter(discovery -> departingObject.equals(discovery.object)).toList();
         JavaTckSupport.check(departingDiscoveries.size() == 1
               && survivorClass.equals(departingDiscoveries.get(0).objectClass),
            "survivor discovery changed the departing object, class, or callback count");
         lost.setAutomaticResignDirective(ResignAction.DELETE_OBJECTS_THEN_DIVEST);
         JavaTckSupport.check(lost.getAutomaticResignDirective()
               == ResignAction.DELETE_OBJECTS_THEN_DIVEST,
            "automatic delete-then-divest directive did not round-trip");

         writeMarker(Path.of(readyMarkerPath), "ready\n");

         long deadline = System.nanoTime() + timeoutMillis * 1_000_000L;
         while (loss.get() == null && System.nanoTime() < deadline) {
             if (callbackModel == CallbackModel.HLA_EVOKED) {
                try {
                   lost.evokeMultipleCallbacks(0.0, 0.05);
                } catch (Exception ignored) {
                   // The transport fixture may expose loss before queuing connectionLost.
                }
            }
            if (loss.get() == null) Thread.sleep(1L);
         }
         JavaTckSupport.check(loss.get() != null && !loss.get().isEmpty(),
            "timed out waiting for a non-empty connectionLost callback");
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(lost);
         JavaTckSupport.check(lossCallbackCount.get() == 1,
            "connection loss delivered duplicate connectionLost callbacks");

         await(() -> !survivorCallbacks.assumptions.isEmpty()
               && survivorCallbacks.removals.size() == 1, survivor,
            callbackModel, timeoutMillis, "delete-then-divest cleanup callbacks");
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(survivor);
         List<OwnershipEvent> assumptions = List.copyOf(survivorCallbacks.assumptions);
         List<RemovalEvent> removals = List.copyOf(survivorCallbacks.removals);
         JavaTckSupport.check(assumptions.size() == 1,
            "connection loss delivered duplicate ownership-assumption callbacks");
         OwnershipEvent assumption = assumptions.get(0);
         JavaTckSupport.check(retainedObject.equals(assumption.object)
               && assumption.attributes.size() == 1
               && assumption.attributes.contains(survivorAttribute)
               && assumption.tag.length == 0,
            "automatic delete-then-divest offered the wrong retained attribute or tag");
         JavaTckSupport.check(removals.size() == 1
               && departingObject.equals(removals.get(0).object)
               && removals.get(0).tag.length == 0
               && lostFederate.equals(removals.get(0).producer),
            "automatic delete-then-divest did not remove exactly the departing object");
         JavaTckSupport.check(retainedName != null && !retainedName.isEmpty()
               && retainedObject.equals(survivor.getObjectInstanceHandle(retainedName)),
            "automatic delete-then-divest removed the retained object");
         JavaTckSupport.expectFailure(
            () -> survivor.getObjectInstanceHandle(deletedObjectName),
            "ObjectInstanceNotKnown", "lookup after automatic object deletion");
         JavaTckSupport.check(!survivor.isAttributeOwnedByFederate(
               retainedObject, survivorAttribute),
            "survivor unexpectedly owns the retained attribute offered by automatic divestiture");

         writeMarker(Path.of(completionMarkerPath), "callback-ok\n");
      } finally {
         if (survivorJoined) {
            try { survivor.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (survivorConnected) {
            try { survivor.disconnect(); } catch (Exception ignored) { }
         }
      }
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

   private static final class Discovery {
      private final ObjectInstanceHandle object;
      private final ObjectClassHandle objectClass;
      private Discovery(ObjectInstanceHandle object, ObjectClassHandle objectClass) {
         this.object = object;
         this.objectClass = objectClass;
      }
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

   private static final class RemovalEvent {
      private final ObjectInstanceHandle object;
      private final byte[] tag;
      private final FederateHandle producer;
      private RemovalEvent(ObjectInstanceHandle object, byte[] tag, FederateHandle producer) {
         this.object = object;
         this.tag = tag.clone();
         this.producer = producer;
      }
   }

   private static final class Recorder {
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> acquisitions = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> assumptions = new CopyOnWriteArrayList<>();
      private final List<RemovalEvent> removals = new CopyOnWriteArrayList<>();
      private FederateAmbassador proxy() { return proxy(null, null); }
      private FederateAmbassador proxy(AtomicReference<String> connectionLoss,
            AtomicInteger connectionLossCount) {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("connectionLost".equals(name) && arguments != null
                     && arguments.length == 1 && connectionLoss != null) {
                  connectionLossCount.incrementAndGet();
                  connectionLoss.set((String) arguments[0]);
               } else if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 2) {
                  discoveries.add(new Discovery((ObjectInstanceHandle) arguments[0],
                     (ObjectClassHandle) arguments[1]));
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)
                     && arguments != null && arguments.length == 3) {
                  acquisitions.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (byte[]) arguments[2]));
               } else if ("requestAttributeOwnershipAssumption".equals(name)
                     && arguments != null && arguments.length == 3) {
                  assumptions.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (byte[]) arguments[2]));
               } else if ("removeObjectInstance".equals(name) && arguments != null
                     && arguments.length == 3) {
                  removals.add(new RemovalEvent((ObjectInstanceHandle) arguments[0],
                     (byte[]) arguments[1], (FederateHandle) arguments[2]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK delete-divest survivor callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == arguments[0];
               }
               return null;
            });
      }
   }
}
