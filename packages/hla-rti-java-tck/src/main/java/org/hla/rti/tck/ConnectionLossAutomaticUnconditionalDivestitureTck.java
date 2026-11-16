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

/** Tests automatic unconditional divestiture after adapter-induced connection loss. */
final class ConnectionLossAutomaticUnconditionalDivestitureTck {
   private ConnectionLossAutomaticUnconditionalDivestitureTck() { }

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
      AtomicReference<String> loss = new AtomicReference<>();
      AtomicInteger lossCallbackCount = new AtomicInteger();
      boolean survivorConnected = false;
      boolean survivorJoined = false;
      try {
         survivor.connect(survivorCallbacks.proxy(), callbackModel, survivorConfiguration);
         survivorConnected = true;
         lost.connect(connectionLossCallbacks(loss, lossCallbackCount), callbackModel,
            lostConfiguration);
         survivor.createFederationExecution(federation, fom, timeImplementation);
         survivor.joinFederationExecution(survivorName, federateType, federation);
         survivorJoined = true;
         FederateHandle lostFederate = lost.joinFederationExecution(
            lostName, federateType, federation);
         JavaTckSupport.check(lostFederate != null,
            "connection-loss member join returned no federate handle");

         ObjectClassHandle lostClass = lost.getObjectClassHandle(objectClassName);
         ObjectClassHandle survivorClass = survivor.getObjectClassHandle(objectClassName);
         JavaTckSupport.check(survivorClass != null && lostClass != null,
            "automatic connection-loss object-class lookup returned no handle");
         AttributeHandle lostAttribute = lost.getAttributeHandle(lostClass, attributeName);
         AttributeHandle survivorAttribute = survivor.getAttributeHandle(
            survivorClass, attributeName);
         AttributeHandle lostPrivilege = lost.getAttributeHandle(
            lostClass, "HLAprivilegeToDeleteObject");
         AttributeHandle survivorPrivilege = survivor.getAttributeHandle(
            survivorClass, "HLAprivilegeToDeleteObject");
         JavaTckSupport.check(survivorAttribute != null && lostAttribute != null
               && survivorPrivilege != null && lostPrivilege != null,
            "automatic connection-loss attribute lookup returned no handle");
         AttributeHandleSet survivorAttributes =
            JavaTckSupport.attributeSet(survivor, survivorAttribute);
         AttributeHandleSet lostAttributes = JavaTckSupport.attributeSet(lost, lostAttribute);

         lost.publishObjectClassAttributes(lostClass, lostAttributes);
         survivor.publishObjectClassAttributes(survivorClass, survivorAttributes);
         survivor.subscribeObjectClassAttributes(survivorClass, survivorAttributes);
         ObjectInstanceHandle object = lost.registerObjectInstance(lostClass);
         JavaTckSupport.check(object != null, "registration returned no object handle");
         String objectName = lost.getObjectInstanceName(object);
         await(() -> survivorCallbacks.discoveries.stream()
               .anyMatch(discovery -> object.equals(discovery.object)), survivor, callbackModel,
            timeoutMillis, "survivor did not discover the registered object");
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(survivor);
         }
         List<Discovery> discoveries = List.copyOf(survivorCallbacks.discoveries);
         JavaTckSupport.check(discoveries.size() == 1
               && object.equals(discoveries.get(0).object)
               && survivorClass.equals(discoveries.get(0).objectClass),
            "survivor discovery changed the object, class, or callback count");

         lost.setAutomaticResignDirective(ResignAction.UNCONDITIONALLY_DIVEST_ATTRIBUTES);
         JavaTckSupport.check(lost.getAutomaticResignDirective()
               == ResignAction.UNCONDITIONALLY_DIVEST_ATTRIBUTES,
            "automatic unconditional-divestiture directive did not round-trip");
         writeMarker(Path.of(readyMarkerPath), "ready\n");

         long deadline = System.nanoTime() + timeoutMillis * 1_000_000L;
         while (loss.get() == null && System.nanoTime() < deadline) {
            if (callbackModel == CallbackModel.HLA_EVOKED) {
               try {
                  lost.evokeMultipleCallbacks(0.0, 0.05);
               } catch (Exception ignored) {
                  // The transport fixture can expose loss before queuing connectionLost.
               }
            }
            if (loss.get() == null) Thread.sleep(1L);
         }
         JavaTckSupport.check(loss.get() != null && !loss.get().isEmpty(),
            "timed out waiting for a non-empty connectionLost callback");
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(lost);
         JavaTckSupport.check(lossCallbackCount.get() == 1,
            "connection loss delivered duplicate connectionLost callbacks");
         writeMarker(Path.of(completionMarkerPath), "callback-ok\n");
         JavaTckSupport.check(lostName.equals(survivor.getFederateName(lostFederate)),
            "connection loss changed the lost member's federate name");

         await(() -> !survivorCallbacks.assumptions.isEmpty(), survivor, callbackModel,
            timeoutMillis, "automatic unconditional-divestiture assumption callback");
         List<OwnershipEvent> assumptions = List.copyOf(survivorCallbacks.assumptions);
         JavaTckSupport.check(assumptions.size() == 1,
            "connection loss delivered duplicate ownership-assumption callbacks");
         OwnershipEvent assumption = assumptions.get(0);
         JavaTckSupport.check(object.equals(assumption.object)
               && assumption.attributes.size() == 2
               && assumption.attributes.contains(survivorAttribute)
               && assumption.attributes.contains(survivorPrivilege)
               && assumption.tag.length == 0,
            "automatic divestiture offered the wrong object, attributes, or tag");
         JavaTckSupport.check(lossCallbackCount.get() == 1,
            "connection loss delivered duplicate connectionLost callbacks");
         JavaTckSupport.check(objectName != null && !objectName.isEmpty()
               && object.equals(survivor.getObjectInstanceHandle(objectName)),
            "automatic unconditional divestiture removed the retained object");
         JavaTckSupport.check(!survivor.isAttributeOwnedByFederate(object, survivorAttribute)
               && !survivor.isAttributeOwnedByFederate(object, survivorPrivilege),
            "survivor unexpectedly owns an attribute offered by automatic divestiture");
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

   private static FederateAmbassador connectionLossCallbacks(
         AtomicReference<String> loss, AtomicInteger callbackCount) {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if ("connectionLost".equals(method.getName())
                  && arguments != null && arguments.length == 1) {
               callbackCount.incrementAndGet();
               loss.set((String) arguments[0]);
            }
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK lost-member callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) return proxy == arguments[0];
            }
            return null;
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
         this.tag = tag.clone();
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

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)
                     && arguments != null && arguments.length >= 2) {
                  discoveries.add(new Discovery((ObjectInstanceHandle) arguments[0],
                     (ObjectClassHandle) arguments[1]));
               } else if ("requestAttributeOwnershipAssumption".equals(name)
                     && arguments != null && arguments.length == 3) {
                  assumptions.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (byte[]) arguments[2]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK divestiture survivor callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == arguments[0];
               }
               return null;
            });
      }
   }
}
