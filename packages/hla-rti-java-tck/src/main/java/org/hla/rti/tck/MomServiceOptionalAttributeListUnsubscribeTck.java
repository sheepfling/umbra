package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies that omitting HLAattributeList requests whole-class unsubscribe. */
final class MomServiceOptionalAttributeListUnsubscribeTck {
   private static final String UNSUBSCRIBE_INTERACTION =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAunsubscribeObjectClassAttributes";
   private static final long CALLBACK_TIMEOUT_NANOS = 15_000_000_000L;

   private MomServiceOptionalAttributeListUnsubscribeTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder subscriberRecorder = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-mom-service-optional-unsubscribe-" + suffix;
      String publisherName = "java-tck-mom-service-unsubscribe-publisher-" + suffix;
      String subscriberName = "java-tck-mom-service-unsubscribe-subscriber-" + suffix;
      boolean publisherConnected = false;
      boolean subscriberConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean subscriberJoined = false;
      boolean applicationPublished = false;
      boolean servicePublished = false;
      ObjectClassHandle publisherClass = null;
      AttributeHandle publisherAttribute = null;
      AttributeHandleSet publisherAttributes = null;
      InteractionClassHandle serviceInteraction = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         subscriber.connect(subscriberRecorder.proxy(), callbackModel);
         subscriberConnected = true;
         publisher.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;

         subscriber.joinFederationExecution(subscriberName, "java-tck", federation);
         subscriberJoined = true;
         ObjectClassHandle subscriberClass = subscriber.getObjectClassHandle(objectClassName);
         AttributeHandle subscriberAttribute = subscriber.getAttributeHandle(
            subscriberClass, attributeName);
         JavaTckSupport.check(objectClassName.equals(
                  subscriber.getObjectClassName(subscriberClass))
               && attributeName.equals(subscriber.getAttributeName(
                  subscriberClass, subscriberAttribute)),
            "adapter-supplied application object class or attribute did not round-trip");
         AttributeHandleSet subscriberAttributes =
            JavaTckSupport.attributeSet(subscriber, subscriberAttribute);
         subscriber.subscribeObjectClassAttributes(subscriberClass, subscriberAttributes, "");

         publisher.joinFederationExecution(publisherName, "java-tck", federation);
         publisherJoined = true;
         publisherClass = publisher.getObjectClassHandle(objectClassName);
         publisherAttribute = publisher.getAttributeHandle(publisherClass, attributeName);
         JavaTckSupport.check(publisherClass.equals(subscriberClass)
               && publisherAttribute.equals(subscriberAttribute)
               && objectClassName.equals(publisher.getObjectClassName(publisherClass))
               && attributeName.equals(publisher.getAttributeName(
                  publisherClass, publisherAttribute)),
            "publisher and subscriber application handles did not round-trip equally");
         publisherAttributes = JavaTckSupport.attributeSet(publisher, publisherAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         applicationPublished = true;

         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         await(subscriber, publisher, callbackModel,
            () -> subscriberRecorder.discovered(object),
            "subscriber discovery before optional-argument unsubscribe");
         byte[] firstValue = new byte[] {0x41, 0x52};
         publisher.updateAttributeValues(object, values(publisher, publisherAttribute, firstValue),
            new byte[0]);
         await(subscriber, publisher, callbackModel,
            () -> !subscriberRecorder.reflections.isEmpty(),
            "initial active subscription reflection");
         JavaTckSupport.check(subscriberRecorder.reflections.size() == 1
               && object.equals(subscriberRecorder.reflections.get(0).object)
               && subscriberRecorder.reflections.get(0).values.size() == 1
               && Arrays.equals(firstValue,
                  subscriberRecorder.reflections.get(0).values.get(subscriberAttribute)),
            "initial active subscription did not reflect the expected attribute");

         subscriberRecorder.reflections.clear();
         serviceInteraction = subscriber.getInteractionClassHandle(UNSUBSCRIBE_INTERACTION);
         ParameterHandle objectClassParameter = subscriber.getParameterHandle(
            serviceInteraction, "HLAobjectClass");
         JavaTckSupport.check(UNSUBSCRIBE_INTERACTION.equals(
                  subscriber.getInteractionClassName(serviceInteraction))
               && "HLAobjectClass".equals(
                  subscriber.getParameterName(serviceInteraction, objectClassParameter)),
            "standard HLAunsubscribeObjectClassAttributes lookup did not round-trip");
         subscriber.publishInteractionClass(serviceInteraction);
         servicePublished = true;
         ParameterHandleValueMap parameters =
            subscriber.getParameterHandleValueMapFactory().create(1);
         parameters.put(objectClassParameter, JavaTckSupport.encode(
            subscriberClass.encodedLength(), subscriberClass::encode));
         JavaTckSupport.check(parameters.size() == 1
               && parameters.containsKey(objectClassParameter),
            "optional-argument unsubscribe did not omit HLAattributeList");
         subscriber.sendInteraction(serviceInteraction, parameters, new byte[0]);

         byte[] laterValue = new byte[] {0x63, 0x74};
         publisher.updateAttributeValues(object, values(publisher, publisherAttribute, laterValue),
            new byte[0]);
         waitForNoReflection(subscriber, publisher, callbackModel, subscriberRecorder);
      } finally {
         if (servicePublished && subscriberJoined && serviceInteraction != null) {
            try { subscriber.unpublishInteractionClass(serviceInteraction); }
            catch (Exception ignored) { }
         }
         if (applicationPublished && publisherJoined && publisherClass != null
               && publisherAttributes != null) {
            try {
               publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
            } catch (Exception ignored) { }
         }
         if (subscriberJoined) {
            try { subscriber.resignFederationExecution(ResignAction.NO_ACTION); }
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
         if (subscriberConnected) {
            try { subscriber.disconnect(); }
            catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static AttributeHandleValueMap values(RTIambassador ambassador,
         AttributeHandle attribute, byte[] value) throws Exception {
      AttributeHandleValueMap result =
         ambassador.getAttributeHandleValueMapFactory().create(1);
      result.put(attribute, value);
      return result;
   }

   private static void await(RTIambassador subscriber, RTIambassador publisher,
         CallbackModel callbackModel, BooleanSupplier ready, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!ready.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            subscriber.evokeCallback(0.01);
            publisher.evokeCallback(0.01);
         } else {
            Thread.yield();
         }
      }
      JavaTckSupport.check(ready.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void waitForNoReflection(RTIambassador subscriber, RTIambassador publisher,
         CallbackModel callbackModel, Recorder recorder) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (recorder.reflections.isEmpty() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            subscriber.evokeCallback(0.01);
            publisher.evokeCallback(0.01);
         } else {
            Thread.sleep(1);
         }
      }
      JavaTckSupport.check(recorder.reflections.isEmpty(),
         "omitting HLAattributeList did not perform whole-class unsubscribe");
   }

   private static Object objectMethod(Object proxy, String name, Object[] arguments) {
      if ("toString".equals(name)) return "Java TCK optional MOM unsubscribe callbacks";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
      return null;
   }

   private static final class Recorder {
      private final java.util.Set<ObjectInstanceHandle> discoveries =
         ConcurrentHashMap.newKeySet();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)
                     && arguments != null && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("reflectAttributeValues".equals(name)
                     && arguments != null && arguments.length >= 2
                     && arguments[1] instanceof AttributeHandleValueMap) {
                  reflections.add(new Reflection(arguments));
               }
               return objectMethod(proxy, name, arguments);
            });
      }

      private boolean discovered(ObjectInstanceHandle object) {
         return discoveries.contains(object);
      }
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final Map<AttributeHandle, byte[]> values = new java.util.LinkedHashMap<>();

      private Reflection(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         AttributeHandleValueMap source = (AttributeHandleValueMap) arguments[1];
         for (AttributeHandle handle : source.keySet()) {
            values.put(handle, source.get(handle).clone());
         }
      }
   }
}
