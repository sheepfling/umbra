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
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Exercises portable ordinary object-deletion boundaries and callback metadata. */
final class ObjectDeletionServiceBoundariesTck {
   private ObjectDeletionServiceBoundariesTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String attributeName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      boolean ownerConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean receiverJoined = false;
      boolean published = false;
      boolean subscribed = false;
      ObjectClassHandle ownerClass = null;
      ObjectClassHandle receiverClass = null;
      AttributeHandle ownerAttribute = null;
      AttributeHandle receiverAttribute = null;
      String federation = "java-tck-object-deletion-boundaries-" + UUID.randomUUID();
      try {
         expectFailure(() -> owner.getObjectClassHandle(objectClassName), "NotConnected",
            "object-class lookup before connection");
         owner.connect(ownerCallbacks.proxy(), callbackModel);
         ownerConnected = true;
         expectFailure(() -> owner.getObjectClassHandle(objectClassName),
            "FederateNotExecutionMember",
            "object-class lookup before federation membership");
         receiver.connect(receiverCallbacks.proxy(), callbackModel);
         receiverConnected = true;
         owner.createFederationExecution(federation, fom);
         created = true;
         FederateHandle producer = owner.joinFederationExecution(
            "java-tck-deletion-owner", federation);
         ownerJoined = true;
         receiver.joinFederationExecution("java-tck-deletion-receiver", federation);
         receiverJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         receiverClass = receiver.getObjectClassHandle(objectClassName);
         ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         receiverAttribute = receiver.getAttributeHandle(receiverClass, attributeName);
         JavaTckSupport.check(ownerClass != null && receiverClass != null
               && ownerAttribute != null && receiverAttribute != null,
            "adapter FOM did not resolve the object class and attribute");
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet receiverAttributes = JavaTckSupport.attributeSet(receiver,
            receiverAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         published = true;
         receiver.subscribeObjectClassAttributes(receiverClass, receiverAttributes);
         subscribed = true;

         ObjectInstanceHandle locallyDeleted = owner.registerObjectInstance(ownerClass);
         ObjectInstanceHandle remotelyDeleted = owner.registerObjectInstance(ownerClass);
         ObjectInstanceHandle deletedOnResign = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(locallyDeleted != null && remotelyDeleted != null
               && deletedOnResign != null
               && !locallyDeleted.equals(remotelyDeleted)
               && !remotelyDeleted.equals(deletedOnResign)
               && !locallyDeleted.equals(deletedOnResign),
            "registration returned null or reused an object-instance handle");
         await(receiver, () -> receiverCallbacks.discoveries().size() == 3,
            "receiver did not discover all registered instances");
         JavaTckSupport.check(owner.getObjectInstanceHandle(
               owner.getObjectInstanceName(remotelyDeleted)).equals(remotelyDeleted),
            "registered object name and handle lookups did not round-trip");
         String localName = owner.getObjectInstanceName(locallyDeleted);
         String remoteName = owner.getObjectInstanceName(remotelyDeleted);

         ownerCallbacks.clearRemovals();
         receiverCallbacks.clearRemovals();
         owner.localDeleteObjectInstance(locallyDeleted);
         JavaTckSupport.check(ownerCallbacks.removals().isEmpty()
               && receiverCallbacks.removals().isEmpty(),
            "local object deletion produced a remote removal callback");
         expectFailure(() -> owner.getObjectInstanceName(locallyDeleted),
            "ObjectInstanceNotKnown", "name lookup after local deletion");
         expectFailure(() -> owner.getObjectInstanceHandle(localName),
            "ObjectInstanceNotKnown", "handle lookup after local deletion");

         byte[] tag = new byte[] {(byte) 0x6a, (byte) 0xb7, (byte) 0xc4};
         owner.deleteObjectInstance(remotelyDeleted, tag);
         await(receiver, () -> receiverCallbacks.removals().size() == 1,
            "receiver did not receive ordinary object removal");
         Removal removal = receiverCallbacks.removals().get(0);
         JavaTckSupport.check(removal.object.equals(remotelyDeleted)
               && Arrays.equals(removal.tag, tag)
               && producer.equals(removal.producer),
            "ordinary removal callback did not preserve object, tag, and producer");
         expectFailure(() -> owner.getObjectInstanceName(remotelyDeleted),
            "ObjectInstanceNotKnown", "name lookup after remote deletion");
         expectFailure(() -> owner.getObjectInstanceHandle(remoteName),
            "ObjectInstanceNotKnown", "handle lookup after remote deletion");

         owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes);
         published = false;
         owner.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         ownerJoined = false;
         await(receiver, () -> receiverCallbacks.removals().size() == 2,
            "resigning with DeleteObjects did not remove the remaining object");
         Removal resignationRemoval = receiverCallbacks.removals().get(1);
         JavaTckSupport.check(resignationRemoval.object.equals(deletedOnResign)
               && producer.equals(resignationRemoval.producer),
            "resignation removal callback identified the wrong object or producer");
         receiver.unsubscribeObjectClassAttributes(receiverClass, receiverAttributes);
         subscribed = false;
         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (receiverJoined && subscribed) {
            try {
               receiver.unsubscribeObjectClassAttributes(receiverClass,
                  JavaTckSupport.attributeSet(receiver, receiverAttribute));
            } catch (Exception ignored) { }
         }
         if (ownerJoined && published) {
            try {
               owner.unpublishObjectClassAttributes(ownerClass,
                  JavaTckSupport.attributeSet(owner, ownerAttribute));
            } catch (Exception ignored) { }
         }
         if (receiverJoined) {
            try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
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
         if (receiverConnected) {
            try { receiver.disconnect(); }
            catch (Exception ignored) { }
         }
         if (ownerConnected) {
            try { owner.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static void expectFailure(CheckedCall call, String expectedSimpleName,
         String operation) throws Exception {
      try {
         call.run();
      } catch (Exception failure) {
         JavaTckSupport.check(expectedSimpleName.equals(failure.getClass().getSimpleName()),
            operation + " threw " + failure.getClass().getSimpleName()
               + " instead of " + expectedSimpleName);
         return;
      }
      throw new AssertionError(operation + " unexpectedly succeeded");
   }

   private static void await(RTIambassador ambassador, BooleanSupplier condition,
         String failure) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         JavaTckSupport.drain(ambassador);
         Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), failure);
   }

   @FunctionalInterface
   private interface CheckedCall { void run() throws Exception; }

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
               if ("discoverObjectInstance".equals(name)) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("removeObjectInstance".equals(name) && arguments != null
                     && arguments.length == 3) {
                  removals.add(new Removal((ObjectInstanceHandle) arguments[0],
                     (byte[]) arguments[1], (FederateHandle) arguments[2]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK deletion boundary callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }
      private List<ObjectInstanceHandle> discoveries() { return List.copyOf(discoveries); }
      private List<Removal> removals() { return List.copyOf(removals); }
      private void clearRemovals() { removals.clear(); }
   }
}
