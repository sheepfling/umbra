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
import hla.rti1516_2025.exceptions.NameNotFound;
import java.lang.reflect.Proxy;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies the public API effects of purging an inactive joined member. */
final class ConnectionFaultInactiveSessionPurgeTck {
   private ConnectionFaultInactiveSessionPurgeTck() { }

   static void run(RtiFactory factory, String fom, String mim,
         String timeImplementation, String objectClassName, String attributeName,
         CallbackModel callbackModel, RtiConfiguration ownerConfiguration,
         RtiConfiguration memberConfiguration, String triggerMarkerPath,
         String completionMarkerPath, long timeoutMillis) throws Exception {
      JavaTckSupport.check(triggerMarkerPath != null && !triggerMarkerPath.isEmpty(),
         "inactive-session purge trigger marker is required");
      JavaTckSupport.check(completionMarkerPath != null && !completionMarkerPath.isEmpty(),
         "inactive-session purge completion marker is required");
      JavaTckSupport.check(timeoutMillis > 0,
         "inactive-session purge timeout must be positive");

      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-inactive-session-purge-" + suffix;
      String ownerName = "java-tck-purge-owner-" + suffix;
      String memberName = "java-tck-purge-member-" + suffix;
      boolean ownerConnected = false;
      boolean memberConnected = false;
      boolean ownerJoined = false;
      boolean memberJoined = false;
      boolean memberTransportMayBeClosed = false;
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel, ownerConfiguration);
         ownerConnected = true;
         owner.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         owner.joinFederationExecution(ownerName, "java-tck-federate", federation);
         ownerJoined = true;

         member.connect(emptyCallbacks(), callbackModel, memberConfiguration);
         memberConnected = true;
         FederateHandle memberHandle = member.joinFederationExecution(
            memberName, "java-tck-federate", federation);
         memberJoined = true;
         FederateHandle observedMember = owner.getFederateHandle(memberName);
         JavaTckSupport.check(memberHandle != null && memberHandle.equals(observedMember),
            "owner did not resolve the joined member's stable federate handle");

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle memberClass = member.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(
            ownerClass, attributeName);
         AttributeHandle memberAttribute = member.getAttributeHandle(
            memberClass, attributeName);
         JavaTckSupport.check(ownerClass != null && memberClass != null
               && ownerAttribute != null && memberAttribute != null,
            "inactive-session purge setup returned an invalid object handle");
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(
            owner, ownerAttribute);
         AttributeHandleSet memberAttributes = JavaTckSupport.attributeSet(
            member, memberAttribute);
         owner.subscribeObjectClassAttributes(ownerClass, ownerAttributes);
         member.publishObjectClassAttributes(memberClass, memberAttributes);
         ObjectInstanceHandle memberObject = member.registerObjectInstance(memberClass);
         JavaTckSupport.check(memberObject != null,
            "inactive-session purge object registration returned no handle");
         await(() -> ownerCallbacks.hasDiscovery(memberObject), owner,
            callbackModel, timeoutMillis,
            "owner did not discover the member's registered object");

         member.setAutomaticResignDirective(ResignAction.DELETE_OBJECTS_THEN_DIVEST);
         JavaTckSupport.check(member.getAutomaticResignDirective()
               == ResignAction.DELETE_OBJECTS_THEN_DIVEST,
            "DELETE_OBJECTS_THEN_DIVEST automatic-resign directive did not round-trip");
         ownerCallbacks.clearRemovals();
         writeMarker(Path.of(triggerMarkerPath), "drop-member-transport\n");
         memberTransportMayBeClosed = true;

         boolean purged = false;
         long deadline = System.nanoTime() + timeoutMillis * 1_000_000L;
         while (System.nanoTime() < deadline) {
            try {
               FederateHandle observed = owner.getFederateHandle(memberName);
               JavaTckSupport.check(memberHandle.equals(observed),
                  "inactive-session purge changed the member's stable handle");
            } catch (NameNotFound expected) {
               purged = true;
               memberJoined = false;
               break;
            }
            if (callbackModel == CallbackModel.HLA_EVOKED) {
               JavaTckSupport.drain(owner);
            }
            Thread.sleep(10L);
         }
         JavaTckSupport.check(purged,
            "RTI did not remove the member after its inactive-session purge timeout");

         await(() -> ownerCallbacks.hasRemoval(memberObject, memberHandle), owner,
            callbackModel, timeoutMillis,
            "owner did not receive the automatic-resign object-removal callback");
         List<Removal> removals = ownerCallbacks.removals();
         JavaTckSupport.check(removals.size() == 1,
            "inactive-session purge delivered duplicate object-removal callbacks");
         Removal removal = removals.get(0);
         JavaTckSupport.check(memberObject.equals(removal.object)
               && removal.tag != null && removal.tag.length == 0
               && memberHandle.equals(removal.producer),
            "automatic removal changed object, empty-tag, or member-handle metadata");

         owner.resignFederationExecution(ResignAction.NO_ACTION);
         ownerJoined = false;
         owner.disconnect();
         ownerConnected = false;
         disconnectQuietly(member);
         memberConnected = false;
         writeMarker(Path.of(completionMarkerPath), "recovered\n");
      } finally {
         if (memberJoined && !memberTransportMayBeClosed) {
            try { member.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { memberJoined = false; }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { ownerJoined = false; }
         }
         if (memberConnected) disconnectQuietly(member);
         if (ownerConnected) {
            try { owner.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static FederateAmbassador emptyCallbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> objectMethod(proxy, method.getName(), arguments));
   }

   private static Object objectMethod(Object proxy, String name, Object[] arguments) {
      if ("toString".equals(name)) return "Java TCK inactive-session purge callbacks";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
      return null;
   }

   private static void disconnectQuietly(RTIambassador ambassador) {
      try { ambassador.disconnect(); }
      catch (Exception ignored) { }
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
         Thread.sleep(1L);
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
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
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
               return objectMethod(proxy, name, arguments);
            });
      }

      private boolean hasDiscovery(ObjectInstanceHandle object) {
         return discoveries.contains(object);
      }

      private boolean hasRemoval(ObjectInstanceHandle object, FederateHandle producer) {
         return removals.stream().anyMatch(removal -> object.equals(removal.object)
            && producer.equals(removal.producer));
      }

      private void clearRemovals() {
         removals.clear();
      }

      private List<Removal> removals() {
         return List.copyOf(removals);
      }
   }
}
