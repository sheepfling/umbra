package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
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

/** Tests DELETE_OBJECTS after adapter-induced connection loss using only the standard HLA API. */
final class AutomaticResignDirectiveDeleteObjectsTck {
   private AutomaticResignDirectiveDeleteObjectsTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         CallbackModel callbackModel, String readyMarkerPath, String completionMarkerPath,
         long timeoutMillis) throws Exception {
      JavaTckSupport.check(readyMarkerPath != null && !readyMarkerPath.isEmpty(),
         "adapter connection-loss readiness marker is required");
      JavaTckSupport.check(completionMarkerPath != null && !completionMarkerPath.isEmpty(),
         "adapter connection-loss completion marker is required");
      JavaTckSupport.check(timeoutMillis > 0, "connection-loss timeout must be positive");

      RTIambassador survivor = factory.getRtiAmbassador();
      RTIambassador lost = factory.getRtiAmbassador();
      Recorder survivorCallbacks = new Recorder();
      AtomicReference<String> loss = new AtomicReference<>();
      String federation = "java-tck-auto-resign-delete-" + UUID.randomUUID();
      boolean survivorConnected = false;
      boolean lostConnected = false;
      boolean created = false;
      boolean survivorJoined = false;
      try {
         survivor.connect(survivorCallbacks.proxy(), callbackModel);
         survivorConnected = true;
         lost.connect(connectionLossCallbacks(loss), callbackModel);
         lostConnected = true;
         survivor.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         survivor.joinFederationExecution("auto-resign-survivor", federation);
         survivorJoined = true;
         FederateHandle lostHandle = lost.joinFederationExecution(
            "auto-resign-lost", federation);

         ObjectClassHandle survivorClass = survivor.getObjectClassHandle(objectClassName);
         ObjectClassHandle lostClass = lost.getObjectClassHandle(objectClassName);
         AttributeHandle survivorAttribute = survivor.getAttributeHandle(
            survivorClass, attributeName);
         AttributeHandle lostAttribute = lost.getAttributeHandle(lostClass, attributeName);
         AttributeHandleSet survivorAttributes =
            JavaTckSupport.attributeSet(survivor, survivorAttribute);
         AttributeHandleSet lostAttributes = JavaTckSupport.attributeSet(lost, lostAttribute);
         lost.publishObjectClassAttributes(lostClass, lostAttributes);
         survivor.subscribeObjectClassAttributes(survivorClass, survivorAttributes);

         ObjectInstanceHandle object = lost.registerObjectInstance(lostClass);
         JavaTckSupport.check(object != null, "registration returned no object handle");
         String objectName = lost.getObjectInstanceName(object);
         await(() -> survivorCallbacks.hasDiscovery(object), survivor, callbackModel,
            timeoutMillis, "survivor did not discover the registered object");
         JavaTckSupport.check(objectName != null && !objectName.isEmpty()
               && object.equals(survivor.getObjectInstanceHandle(objectName)),
            "object-name lookup failed before automatic resignation");

         lost.setAutomaticResignDirective(ResignAction.DELETE_OBJECTS);
         JavaTckSupport.check(lost.getAutomaticResignDirective() == ResignAction.DELETE_OBJECTS,
            "DELETE_OBJECTS automatic resign directive did not round-trip");
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
               // The adapter may surface transport loss before queuing its callback.
            }
            if (loss.get() == null) Thread.sleep(1L);
         }
         JavaTckSupport.check(loss.get() != null && !loss.get().isEmpty(),
            "timed out waiting for a non-empty connectionLost callback");

         await(() -> !survivorCallbacks.removals().isEmpty(), survivor, callbackModel,
            timeoutMillis, "survivor did not receive automatic DELETE_OBJECTS removal");
         List<Removal> removals = survivorCallbacks.removals();
         JavaTckSupport.check(removals.size() == 1,
            "automatic resignation delivered duplicate object-removal callbacks");
         Removal removal = removals.get(0);
         JavaTckSupport.check(object.equals(removal.object)
               && Arrays.equals(removal.tag, new byte[0])
               && lostHandle.equals(removal.producer),
            "automatic removal changed object, empty-tag, or producer metadata");
         JavaTckSupport.expectFailure(() -> survivor.getObjectInstanceHandle(objectName),
            "ObjectInstanceNotKnown", "name lookup after automatic object deletion");

         // The adapter fixture uses this ordinary interaction as its completion handshake.
         survivor.sendInteraction(signalClass,
            survivor.getParameterHandleValueMapFactory().create(0), new byte[0]);
         try {
            lost.disconnect();
         } catch (Exception ignored) {
            // The provider may already have completed local connection-loss cleanup.
         }
         lostConnected = false;
         writeMarker(Path.of(completionMarkerPath), "callback-ok\n");
      } finally {
         if (lostConnected) {
            try { lost.disconnect(); } catch (Exception ignored) { }
         }
         if (survivorJoined) {
            try { survivor.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { survivor.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (survivorConnected) {
            try { survivor.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static FederateAmbassador connectionLossCallbacks(AtomicReference<String> loss) {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if ("connectionLost".equals(method.getName())
                  && arguments != null && arguments.length == 1) {
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
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(ambassador);
         }
         Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), failure);
   }

   private static final class Removal {
      private final ObjectInstanceHandle object;
      private final byte[] tag;
      private final FederateHandle producer;

      private Removal(ObjectInstanceHandle object, byte[] tag, FederateHandle producer) {
         this.object = object;
         this.tag = tag == null ? null : tag.clone();
         this.producer = producer;
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Removal> removals = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)
                     && arguments != null && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("removeObjectInstance".equals(name)
                     && arguments != null && arguments.length == 3) {
                  removals.add(new Removal((ObjectInstanceHandle) arguments[0],
                     (byte[]) arguments[1], (FederateHandle) arguments[2]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK survivor callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == arguments[0];
               }
               return null;
            });
      }

      private boolean hasDiscovery(ObjectInstanceHandle object) {
         return discoveries.contains(object);
      }

      private List<Removal> removals() {
         return List.copyOf(removals);
      }
   }
}
