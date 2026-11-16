package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verifies derived-object discovery and inherited reflection through a base-class subscription. */
final class ObjectClassAbstractionSubscriptionTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final long DUPLICATE_CALLBACK_WINDOW_NANOS = 200_000_000L;

   private ObjectClassAbstractionSubscriptionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String baseObjectClassName, String derivedObjectClassName,
         String identityAttributeName, CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder subscriberRecorder = new Recorder();
      String federation = "java-tck-object-class-abstraction-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean subscriberConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean subscriberJoined = false;
      boolean published = false;
      boolean subscribed = false;
      ObjectClassHandle publisherDerivedClass = null;
      ObjectClassHandle subscriberBaseClass = null;
      AttributeHandleSet publisherAttributes = null;
      AttributeHandleSet subscriberAttributes = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         subscriber.connect(subscriberRecorder.proxy(), callbackModel);
         subscriberConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         publisher.joinFederationExecution(
            "java-tck-abstraction-publisher", "tck-type", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution(
            "java-tck-abstraction-subscriber", "tck-type", federation);
         subscriberJoined = true;

         publisherDerivedClass = publisher.getObjectClassHandle(derivedObjectClassName);
         subscriberBaseClass = subscriber.getObjectClassHandle(baseObjectClassName);
         AttributeHandle publisherIdentity = publisher.getAttributeHandle(
            publisherDerivedClass, identityAttributeName);
         AttributeHandle subscriberIdentity = subscriber.getAttributeHandle(
            subscriberBaseClass, identityAttributeName);
         JavaTckSupport.check(!publisherDerivedClass.equals(subscriberBaseClass)
               && publisherIdentity != null && publisherIdentity.equals(subscriberIdentity),
            "adapter hierarchy FOM did not expose the derived class and inherited identity attribute");

         publisherAttributes = JavaTckSupport.attributeSet(publisher, publisherIdentity);
         subscriberAttributes = JavaTckSupport.attributeSet(subscriber, subscriberIdentity);
         publisher.publishObjectClassAttributes(publisherDerivedClass, publisherAttributes);
         published = true;
         subscriber.subscribeObjectClassAttributes(subscriberBaseClass, subscriberAttributes);
         subscribed = true;

         ObjectInstanceHandle publisherObject =
            publisher.registerObjectInstance(publisherDerivedClass);
         JavaTckSupport.check(publisherObject != null && publisherObject.encodedLength() > 0,
            "derived object registration returned an invalid handle");
         String objectName = publisher.getObjectInstanceName(publisherObject);
         JavaTckSupport.check(objectName != null && !objectName.isEmpty(),
            "derived object registration did not return an object name");
         await(subscriber, callbackModel,
            () -> !subscriberRecorder.discoveries.isEmpty(),
            "discovery through the abstract object-class subscription");
         settle(subscriber, callbackModel);
         ObjectInstanceHandle subscriberObject = subscriber.getObjectInstanceHandle(objectName);
         Discovery discovery = subscriberRecorder.discoveries.get(0);
         JavaTckSupport.check(subscriberRecorder.discoveries.size() == 1
               && publisherRecorder.discoveries.isEmpty()
               && subscriberObject.equals(discovery.object)
               && subscriberBaseClass.equals(discovery.objectClass)
               && subscriberBaseClass.equals(
                  subscriber.getKnownObjectClassHandle(subscriberObject)),
            "derived registration was not discovered once through the subscriber's base-class view");

         byte[] identityValue = new byte[] {0x41, 0x42};
         AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(1);
         values.put(publisherIdentity, identityValue);
         publisher.updateAttributeValues(publisherObject, values, new byte[0]);
         await(subscriber, callbackModel,
            () -> !subscriberRecorder.reflections.isEmpty(),
            "inherited identity reflection through the abstract object-class subscription");
         settle(subscriber, callbackModel);
         JavaTckSupport.check(subscriberRecorder.reflections.size() == 1,
            "the inherited identity update produced a duplicate reflection");
         Reflection reflection = subscriberRecorder.reflections.get(0);
         byte[] observed = reflection.values.get(subscriberIdentity);
         JavaTckSupport.check(subscriberObject.equals(reflection.object)
               && reflection.values.size() == 1 && observed != null
               && Arrays.equals(identityValue, observed),
            "the abstract-class subscription did not reflect the inherited identity value");

         subscriber.unsubscribeObjectClassAttributes(subscriberBaseClass, subscriberAttributes);
         subscribed = false;
         publisher.unpublishObjectClassAttributes(publisherDerivedClass, publisherAttributes);
         published = false;
      } finally {
         if (subscribed && subscriberJoined && subscriberBaseClass != null
               && subscriberAttributes != null) {
            try { subscriber.unsubscribeObjectClassAttributes(
               subscriberBaseClass, subscriberAttributes); } catch (Exception ignored) { }
         }
         if (published && publisherJoined && publisherDerivedClass != null
               && publisherAttributes != null) {
            try { publisher.unpublishObjectClassAttributes(
               publisherDerivedClass, publisherAttributes); } catch (Exception ignored) { }
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
            try { publisher.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (subscriberConnected) {
            try { subscriber.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void await(RTIambassador subscriber, CallbackModel model,
         Condition condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.isTrue() && System.nanoTime() < deadline) {
         serviceCallbacks(subscriber, model);
      }
      JavaTckSupport.check(condition.isTrue(), "timed out waiting for " + description);
   }

   private static void settle(RTIambassador subscriber, CallbackModel model) throws Exception {
      long deadline = System.nanoTime() + DUPLICATE_CALLBACK_WINDOW_NANOS;
      while (System.nanoTime() < deadline) serviceCallbacks(subscriber, model);
   }

   private static void serviceCallbacks(RTIambassador subscriber, CallbackModel model)
         throws Exception {
      if (model == CallbackModel.HLA_EVOKED) subscriber.evokeMultipleCallbacks(0.0, 0.01);
      else Thread.sleep(1L);
   }

   private interface Condition {
      boolean isTrue();
   }

   private static final class Recorder {
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length == 4) {
                  discoveries.add(new Discovery(arguments));
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 6) {
                  reflections.add(new Reflection(arguments));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK object abstraction callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }
   }

   private static final class Discovery {
      private final ObjectInstanceHandle object;
      private final ObjectClassHandle objectClass;

      private Discovery(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         objectClass = (ObjectClassHandle) arguments[1];
      }
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final AttributeHandleValueMap values;

      private Reflection(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         values = ((AttributeHandleValueMap) arguments[1]).clone();
      }
   }
}
