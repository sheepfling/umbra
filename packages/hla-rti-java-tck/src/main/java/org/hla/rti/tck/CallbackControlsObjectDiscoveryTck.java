package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.ConfigurationResult;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Portable callback-enable gating for ordinary object discovery. */
final class CallbackControlsObjectDiscoveryTck {
   private CallbackControlsObjectDiscoveryTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String attributeName, CallbackModel callbackModel) throws Exception {
      run(factory, fom, objectClassName, attributeName, callbackModel, false);
   }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String attributeName, CallbackModel callbackModel,
         boolean drainCallbacksBeforeResign) throws Exception {
      run(factory, fom, objectClassName, attributeName, callbackModel,
         null, null, drainCallbacksBeforeResign);
   }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String attributeName, CallbackModel callbackModel, String rtiAddress,
         String additionalSettings, boolean drainCallbacksBeforeResign) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-callback-object-discovery-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      boolean published = false;
      boolean subscribed = false;
      try {
         connect(publisher, publisherRecorder, callbackModel, rtiAddress,
            additionalSettings, "publisher");
         publisherConnected = true;
         connect(receiver, receiverRecorder, callbackModel, rtiAddress,
            additionalSettings, "receiver");
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom);
         created = true;
         FederateHandle publisherHandle = publisher.joinFederationExecution(
            "java-tck-discovery-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-discovery-receiver", federation);
         receiverJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute =
            publisher.getAttributeHandle(publisherClass, attributeName);
         AttributeHandle receiverAttribute =
            receiver.getAttributeHandle(receiverClass, attributeName);
         JavaTckSupport.check(publisherClass.equals(receiverClass)
               && publisherAttribute.equals(receiverAttribute),
            "standard object-discovery FOM lookups returned inconsistent handles");
         AttributeHandleSet publisherAttributes =
            JavaTckSupport.attributeSet(publisher, publisherAttribute);
         AttributeHandleSet receiverAttributes =
            JavaTckSupport.attributeSet(receiver, receiverAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         receiver.subscribeObjectClassAttributes(receiverClass, receiverAttributes);
         subscribed = true;

         receiver.disableCallbacks();
         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(object != null,
            "callback-controlled registration returned a null object handle");
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(receiver.evokeMultipleCallbacks(0.0, 1.0),
               "evoked callback control did not admit the pending object-discovery event");
         } else {
            service(receiver, callbackModel, objectClassName);
         }
         JavaTckSupport.check(!receiverRecorder.hasDiscovery(object),
            "disabled callbacks exposed an object-discovery callback");

         receiver.enableCallbacks();
         await(receiver, callbackModel, objectClassName,
            () -> receiverRecorder.hasDiscovery(object),
            "re-enabled object-discovery callback did not arrive");
         Discovery discovery = receiverRecorder.discoveryFor(object);
         JavaTckSupport.check(discovery != null
               && object.equals(discovery.object)
               && receiverClass.equals(discovery.objectClass)
               && discovery.name != null && !discovery.name.isEmpty()
               && publisherHandle.equals(discovery.producer),
            "re-enabled object-discovery callback lost standard delivery data");

         receiver.unsubscribeObjectClassAttributes(receiverClass, receiverAttributes);
         subscribed = false;
         publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         published = false;
         if (drainCallbacksBeforeResign && callbackModel == CallbackModel.HLA_EVOKED) {
            drainPendingCallbacks(publisher);
            drainPendingCallbacks(receiver);
         }
         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS);
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
            try { publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
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

   private static void connect(RTIambassador ambassador, Recorder recorder,
         CallbackModel callbackModel, String rtiAddress, String additionalSettings,
         String role) throws Exception {
      if ((rtiAddress == null || rtiAddress.isEmpty())
            && (additionalSettings == null || additionalSettings.isEmpty())) {
         ambassador.connect(recorder.proxy(), callbackModel);
         return;
      }
      RtiConfiguration configuration = RtiConfiguration.createConfiguration();
      if (rtiAddress != null && !rtiAddress.isEmpty()) {
         configuration.withRtiAddress(rtiAddress);
      }
      if (additionalSettings != null && !additionalSettings.isEmpty()) {
         configuration.withAdditionalSettings(additionalSettings);
      }
      ConfigurationResult result = ambassador.connect(
         recorder.proxy(), callbackModel, configuration);
      JavaTckSupport.check(result != null, role + " Connect returned null");
      if (rtiAddress != null && !rtiAddress.isEmpty()) {
         JavaTckSupport.check(result.addressUsed,
            role + " Connect did not use the adapter-supplied endpoint");
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

   private static void drainPendingCallbacks(RTIambassador ambassador) throws Exception {
      while (ambassador.evokeMultipleCallbacks(0.0, 0.0)) { }
   }

   private interface Condition {
      boolean isTrue();
   }

   private static final class Discovery {
      private final ObjectInstanceHandle object;
      private final ObjectClassHandle objectClass;
      private final String name;
      private final FederateHandle producer;

      private Discovery(ObjectInstanceHandle object, ObjectClassHandle objectClass,
            String name, FederateHandle producer) {
         this.object = object;
         this.objectClass = objectClass;
         this.name = name;
         this.producer = producer;
      }
   }

   private static final class Recorder {
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)) {
                  discoveries.add(new Discovery((ObjectInstanceHandle) arguments[0],
                     (ObjectClassHandle) arguments[1], (String) arguments[2],
                     (FederateHandle) arguments[3]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK object-discovery callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }

      private boolean hasDiscovery(ObjectInstanceHandle object) {
         return discoveries.stream().anyMatch(event -> object.equals(event.object));
      }

      private Discovery discoveryFor(ObjectInstanceHandle object) {
         return discoveries.stream().filter(event -> object.equals(event.object))
            .findFirst().orElse(null);
      }
   }
}
