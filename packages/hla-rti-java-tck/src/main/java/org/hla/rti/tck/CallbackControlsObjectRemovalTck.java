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

/** Portable callback-enable gating for ordinary object removal. */
final class CallbackControlsObjectRemovalTck {
   private static final byte[] BASELINE_TAG = new byte[] {0x52, 0x4d, 0x31};
   private static final byte[] GATED_TAG = new byte[] {0x52, 0x4d, 0x32};

   private CallbackControlsObjectRemovalTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String attributeName, CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-callback-object-removal-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      boolean published = false;
      boolean subscribed = false;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom);
         created = true;
         FederateHandle publisherHandle = publisher.joinFederationExecution(
            "java-tck-removal-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-removal-receiver", federation);
         receiverJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute =
            publisher.getAttributeHandle(publisherClass, attributeName);
         AttributeHandle receiverAttribute =
            receiver.getAttributeHandle(receiverClass, attributeName);
         JavaTckSupport.check(publisherClass.equals(receiverClass)
               && publisherAttribute.equals(receiverAttribute),
            "standard object-removal FOM lookups returned inconsistent handles");
         AttributeHandleSet publisherAttributes =
            JavaTckSupport.attributeSet(publisher, publisherAttribute);
         AttributeHandleSet receiverAttributes =
            JavaTckSupport.attributeSet(receiver, receiverAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         receiver.subscribeObjectClassAttributes(receiverClass, receiverAttributes);
         subscribed = true;

         ObjectInstanceHandle baselineObject = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(baselineObject != null,
            "baseline object-removal registration returned a null handle");
         await(receiver, callbackModel, objectClassName,
            () -> receiverRecorder.discoveries.contains(baselineObject),
            "receiver did not discover the baseline object");
         publisher.deleteObjectInstance(baselineObject, BASELINE_TAG.clone());
         await(receiver, callbackModel, objectClassName,
            () -> receiverRecorder.hasRemoval(baselineObject, BASELINE_TAG, publisherHandle),
            "baseline object-removal callback did not arrive");

         receiverRecorder.clearEvents();
         ObjectInstanceHandle gatedObject = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(gatedObject != null,
            "gated object-removal registration returned a null handle");
         await(receiver, callbackModel, objectClassName,
            () -> receiverRecorder.discoveries.contains(gatedObject),
            "receiver did not discover the gated object");

         receiver.disableCallbacks();
         publisher.deleteObjectInstance(gatedObject, GATED_TAG.clone());
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(receiver.evokeMultipleCallbacks(0.0, 1.0),
               "evoked callback control did not admit the pending object-removal event");
         } else {
            service(receiver, callbackModel, objectClassName);
         }
         JavaTckSupport.check(
            !receiverRecorder.hasRemoval(gatedObject, GATED_TAG, publisherHandle),
            "disabled callbacks exposed an object-removal callback");

         receiver.enableCallbacks();
         await(receiver, callbackModel, objectClassName,
            () -> receiverRecorder.hasRemoval(gatedObject, GATED_TAG, publisherHandle),
            "re-enabled object-removal callback did not arrive");
         Removal removal = receiverRecorder.lastRemoval();
         JavaTckSupport.check(removal != null
               && gatedObject.equals(removal.object)
               && Arrays.equals(GATED_TAG, removal.tag)
               && publisherHandle.equals(removal.producer),
            "re-enabled object-removal callback lost standard delivery data");

         receiver.unsubscribeObjectClassAttributes(receiverClass, receiverAttributes);
         subscribed = false;
         publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         published = false;
         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         publisher.resignFederationExecution(ResignAction.NO_ACTION);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (subscribed) {
            try {
               ObjectClassHandle objectClass = receiver.getObjectClassHandle(objectClassName);
               AttributeHandle attribute = receiver.getAttributeHandle(objectClass, attributeName);
               receiver.unsubscribeObjectClassAttributes(
                  objectClass, JavaTckSupport.attributeSet(receiver, attribute));
            } catch (Exception ignored) { }
         }
         if (published) {
            try {
               ObjectClassHandle objectClass = publisher.getObjectClassHandle(objectClassName);
               AttributeHandle attribute = publisher.getAttributeHandle(objectClass, attributeName);
               publisher.unpublishObjectClassAttributes(
                  objectClass, JavaTckSupport.attributeSet(publisher, attribute));
            } catch (Exception ignored) { }
         }
         if (receiverJoined) {
            try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (receiverConnected) {
            try { receiver.disconnect(); }
            catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); }
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

   private static final class Removal {
      private final ObjectInstanceHandle object;
      private final byte[] tag;
      private final FederateHandle producer;

      private Removal(ObjectInstanceHandle object, byte[] tag, FederateHandle producer) {
         this.object = object;
         this.tag = tag.clone();
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
               if ("discoverObjectInstance".equals(name)) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("removeObjectInstance".equals(name)) {
                  removals.add(new Removal((ObjectInstanceHandle) arguments[0],
                     (byte[]) arguments[1], (FederateHandle) arguments[2]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK object-removal callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }

      private boolean hasRemoval(ObjectInstanceHandle object, byte[] tag,
            FederateHandle producer) {
         return removals.stream().anyMatch(removal -> object.equals(removal.object)
            && Arrays.equals(tag, removal.tag) && producer.equals(removal.producer));
      }

      private Removal lastRemoval() {
         return removals.isEmpty() ? null : removals.get(removals.size() - 1);
      }

      private void clearEvents() {
         discoveries.clear();
         removals.clear();
      }
   }
}
