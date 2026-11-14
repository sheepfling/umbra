package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Checks timestamp preservation for an attribute update from an unregulated sender. */
final class UnregulatedSenderTimestampedAttributeUpdateTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 5_000_000_000L;

   private UnregulatedSenderTimestampedAttributeUpdateTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      String federation = "java-tck-unregulated-timestamped-attribute-" + UUID.randomUUID();
      boolean publisherConnected = false, receiverConnected = false;
      boolean federationCreated = false;
      boolean publisherJoined = false, receiverJoined = false;
      boolean published = false, subscribed = false;
      ObjectClassHandle publisherClass = null, receiverClass = null;
      AttributeHandleSet publisherAttributes = null, receiverAttributes = null;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverCallbacks.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         federationCreated = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-unregulated-attribute-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-unregulated-attribute-receiver", federation);
         receiverJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         AttributeHandle receiverAttribute = receiver.getAttributeHandle(
            receiverClass, attributeName);
         JavaTckSupport.check(publisherClass != null && receiverClass != null
               && publisherAttribute != null && receiverAttribute != null,
            "adapter attribute FOM did not resolve the standard handles");
         publisherAttributes = publisher.getAttributeHandleSetFactory().create();
         publisherAttributes.add(publisherAttribute);
         receiverAttributes = receiver.getAttributeHandleSetFactory().create();
         receiverAttributes.add(receiverAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         receiver.subscribeObjectClassAttributes(receiverClass, receiverAttributes);
         subscribed = true;

         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         await(receiver, callbackModel, objectClassName,
            () -> object.equals(receiverCallbacks.discoveredObject), "object discovery");

         LogicalTimeFactory<?, ?> publisherTimeFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverTimeFactory = receiver.getTimeFactory();
         JavaTckSupport.check(publisherTimeFactory.getName().equals(receiverTimeFactory.getName()),
            "unregulated attribute federation selected different logical-time factories");
         LogicalTime<?, ?> publisherInitial = publisher.queryLogicalTime();
         LogicalTime<?, ?> receiverInitial = receiver.queryLogicalTime();
         JavaTckSupport.check(publisherInitial.equals(receiverInitial),
            "joined attribute federates did not start at the same logical time");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) publisherTimeFactory.makeEpsilon();
         LogicalTime<?, ?> timestamp = offset(publisherInitial, epsilon, 6);

         byte[] payload = {0x55, 0x4E, 0x52, 0x2D, 0x41};
         byte[] tag = {0x55, 0x4E, 0x52, 0x2D, 0x54};
         AttributeHandleValueMap values =
            publisher.getAttributeHandleValueMapFactory().create(1);
         values.put(publisherAttribute, payload.clone());
         // Neither member enables Time Regulation or Time Constrained.
         publisher.updateAttributeValues(object, values, tag.clone(), timestamp);
         await(receiver, callbackModel, objectClassName,
            () -> receiverCallbacks.reflections.size() == 1, "timestamped attribute reflection");

         Reflection reflection = receiverCallbacks.reflections.get(0);
         JavaTckSupport.check(receiverCallbacks.reflections.size() == 1
               && object.equals(reflection.object)
               && reflection.values.size() == 1
               && Arrays.equals(payload, reflection.values.get(receiverAttribute))
               && Arrays.equals(tag, reflection.tag)
               && producer.equals(reflection.producer)
               && timestamp.equals(reflection.time),
            "RTI did not preserve the unregulated sender's timestamped attribute update");
      } finally {
         if (subscribed && receiverJoined && receiverClass != null && receiverAttributes != null) {
            try { receiver.unsubscribeObjectClassAttributes(receiverClass, receiverAttributes); }
            catch (Exception ignored) { }
         }
         if (published && publisherJoined && publisherClass != null && publisherAttributes != null) {
            try { publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes); }
            catch (Exception ignored) { }
         }
         if (receiverJoined) {
            try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (federationCreated) {
            try { publisher.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (receiverConnected) {
            try { receiver.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         String objectClassName, Condition condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.isMet() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) {
            ambassador.evokeMultipleCallbacks(0.0, 0.0);
         } else {
            ambassador.getObjectClassHandle(objectClassName);
            Thread.yield();
         }
      }
      JavaTckSupport.check(condition.isMet(), "timed out waiting for " + description);
   }

   @FunctionalInterface
   private interface Condition { boolean isMet() throws Exception; }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final AttributeHandleValueMap values;
      private final byte[] tag;
      private final FederateHandle producer;
      private final LogicalTime<?, ?> time;

      private Reflection(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         values = ((AttributeHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
         producer = (FederateHandle) arguments[4];
         time = (LogicalTime<?, ?>) arguments[6];
      }
   }

   private static final class Recorder {
      private volatile ObjectInstanceHandle discoveredObject;
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveredObject = (ObjectInstanceHandle) arguments[0];
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 10) {
                  reflections.add(new Reflection(arguments));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) {
                     return "Java TCK unregulated timestamped attribute callbacks";
                  }
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }
   }
}
