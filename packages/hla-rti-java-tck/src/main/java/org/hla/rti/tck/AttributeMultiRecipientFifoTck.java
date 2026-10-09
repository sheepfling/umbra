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
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks FIFO ordinary attribute update delivery to two active subscribers. */
final class AttributeMultiRecipientFifoTck {
   private AttributeMultiRecipientFifoTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String attributeName, CallbackModel callbackModel) throws Exception {
      RTIambassador[] rti = new RTIambassador[3];
      Recorder[] recorders = new Recorder[3];
      boolean[] connected = new boolean[3];
      boolean[] joined = new boolean[3];
      boolean[] subscribed = new boolean[3];
      ObjectClassHandle[] classes = new ObjectClassHandle[3];
      AttributeHandle[] attributes = new AttributeHandle[3];
      boolean created = false;
      boolean published = false;
      String federation = "java-tck-attribute-fanout-" + UUID.randomUUID();
      try {
         for (int i = 0; i < rti.length; ++i) {
            rti[i] = factory.getRtiAmbassador();
            recorders[i] = new Recorder();
            rti[i].connect(recorders[i].proxy(), callbackModel);
            connected[i] = true;
         }
         rti[0].createFederationExecution(federation, fom);
         created = true;
         FederateHandle producer = rti[0].joinFederationExecution(
            "java-tck-attribute-fifo-publisher", federation);
         joined[0] = true;
         rti[1].joinFederationExecution("java-tck-attribute-fifo-first", federation);
         joined[1] = true;
         rti[2].joinFederationExecution("java-tck-attribute-fifo-second", federation);
         joined[2] = true;

         for (int i = 0; i < rti.length; ++i) {
            classes[i] = rti[i].getObjectClassHandle(objectClassName);
            attributes[i] = rti[i].getAttributeHandle(classes[i], attributeName);
            JavaTckSupport.check(classes[i] != null && attributes[i] != null,
               "adapter FOM did not resolve the attribute FIFO class and attribute");
         }
         for (int i = 1; i < rti.length; ++i) {
            JavaTckSupport.check(classes[0].equals(classes[i])
                  && attributes[0].equals(attributes[i]),
               "attribute FIFO handles changed across federates");
         }
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(rti[0], attributes[0]);
         AttributeHandleSet firstAttributes = JavaTckSupport.attributeSet(rti[1], attributes[1]);
         AttributeHandleSet secondAttributes = JavaTckSupport.attributeSet(rti[2], attributes[2]);
         rti[0].publishObjectClassAttributes(classes[0], ownerAttributes);
         published = true;
         rti[1].subscribeObjectClassAttributes(classes[1], firstAttributes);
         subscribed[1] = true;
         rti[2].subscribeObjectClassAttributes(classes[2], secondAttributes);
         subscribed[2] = true;

         ObjectInstanceHandle object = rti[0].registerObjectInstance(classes[0]);
         await(rti[1], () -> recorders[1].discovered(object),
            "first subscriber did not discover the object before the deadline");
         await(rti[2], () -> recorders[2].discovered(object),
            "second subscriber did not discover the object before the deadline");
         JavaTckSupport.check(object.equals(recorders[1].discoveredObject)
               && object.equals(recorders[2].discoveredObject)
               && classes[1].equals(recorders[1].discovery.objectClass)
               && classes[2].equals(recorders[2].discovery.objectClass)
               && producer.equals(recorders[1].discovery.producer)
               && producer.equals(recorders[2].discovery.producer),
            "attribute update subscribers discovered the wrong object or producer metadata");
         recorders[1].clearReflections();
         recorders[2].clearReflections();

         byte[] firstValue = new byte[] {0x46, 0x49, 0x52, 0x53, 0x54};
         byte[] firstTag = new byte[] {0x31};
         byte[] secondValue = new byte[] {0x53, 0x45, 0x43, 0x4f, 0x4e, 0x44};
         byte[] secondTag = new byte[] {0x32};
         send(rti[0], object, attributes[0], firstValue, firstTag);
         send(rti[0], object, attributes[0], secondValue, secondTag);
         await(rti[1], () -> recorders[1].reflections().size() >= 2,
            "first subscriber did not receive both updates before the deadline");
         await(rti[2], () -> recorders[2].reflections().size() >= 2,
            "second subscriber did not receive both updates before the deadline");
         JavaTckSupport.check(recorders[0].reflections().isEmpty(),
            "ordinary attribute updates looped back to the publisher");
         assertDeliveries(rti[1], recorders[1].reflections(), object, attributes[1],
            firstValue, firstTag, secondValue, secondTag, producer, "first subscriber");
         assertDeliveries(rti[2], recorders[2].reflections(), object, attributes[2],
            firstValue, firstTag, secondValue, secondTag, producer, "second subscriber");

         rti[1].unsubscribeObjectClassAttributes(classes[1], firstAttributes);
         subscribed[1] = false;
         rti[2].unsubscribeObjectClassAttributes(classes[2], secondAttributes);
         subscribed[2] = false;
         rti[0].unpublishObjectClassAttributes(classes[0], ownerAttributes);
         published = false;
         rti[2].resignFederationExecution(ResignAction.NO_ACTION);
         joined[2] = false;
         rti[1].resignFederationExecution(ResignAction.NO_ACTION);
         joined[1] = false;
         rti[0].resignFederationExecution(ResignAction.NO_ACTION);
         joined[0] = false;
         rti[0].destroyFederationExecution(federation);
         created = false;
      } finally {
         for (int i = 1; i < rti.length; ++i) {
            if (rti[i] != null && joined[i] && subscribed[i]) {
               try {
                  rti[i].unsubscribeObjectClassAttributes(classes[i],
                     JavaTckSupport.attributeSet(rti[i], attributes[i]));
               } catch (Exception ignored) { }
            }
         }
         if (rti[0] != null && joined[0] && published) {
            try {
               rti[0].unpublishObjectClassAttributes(classes[0],
                  JavaTckSupport.attributeSet(rti[0], attributes[0]));
            } catch (Exception ignored) { }
         }
         for (int i = rti.length - 1; i >= 0; --i) {
            if (rti[i] != null && joined[i]) {
               try {
                  rti[i].resignFederationExecution(
                     i == 0 ? ResignAction.DELETE_OBJECTS : ResignAction.NO_ACTION);
               } catch (Exception ignored) { }
            }
         }
         if (created && rti[0] != null) {
            try { rti[0].destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         for (int i = rti.length - 1; i >= 0; --i) {
            if (rti[i] != null && connected[i]) {
               try { rti[i].disconnect(); }
               catch (Exception ignored) { }
            }
         }
      }
   }

   private static void send(RTIambassador publisher, ObjectInstanceHandle object,
         AttributeHandle attribute, byte[] value, byte[] tag) throws Exception {
      AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(1);
      values.put(attribute, value.clone());
      publisher.updateAttributeValues(object, values, tag.clone());
   }

   private static void assertDeliveries(RTIambassador receiver, List<Reflection> reflections,
         ObjectInstanceHandle object, AttributeHandle attribute, byte[] firstValue,
         byte[] firstTag, byte[] secondValue, byte[] secondTag, FederateHandle producer,
         String description) throws Exception {
      JavaTckSupport.check(reflections.size() == 2,
         description + " received an unexpected reflection callback count");
      assertDelivery(receiver, reflections.get(0), object, attribute, firstValue,
         firstTag, producer, description + " first receive-order update");
      assertDelivery(receiver, reflections.get(1), object, attribute, secondValue,
         secondTag, producer, description + " second receive-order update");
   }

   private static void assertDelivery(RTIambassador receiver, Reflection reflection,
         ObjectInstanceHandle object, AttributeHandle attribute, byte[] value, byte[] tag,
         FederateHandle producer, String description) throws Exception {
      JavaTckSupport.check(!reflection.timestampOrdered && object.equals(reflection.object)
            && reflection.values.size() == 1
            && Arrays.equals(value, reflection.values.get(attribute))
            && Arrays.equals(tag, reflection.tag)
            && producer.equals(reflection.producer)
            && reflection.transportation != null
            && !receiver.getTransportationTypeName(reflection.transportation).isEmpty(),
         description + " returned incorrect object, value, tag, producer, or transport metadata");
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

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final AttributeHandleValueMap values;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final boolean timestampOrdered;

      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap values,
            byte[] tag, TransportationTypeHandle transportation, FederateHandle producer,
            boolean timestampOrdered) {
         this.object = object;
         this.values = values.clone();
         this.tag = tag.clone();
         this.transportation = transportation;
         this.producer = producer;
         this.timestampOrdered = timestampOrdered;
      }
   }

   private static final class Discovery {
      private final ObjectClassHandle objectClass;
      private final FederateHandle producer;

      private Discovery(ObjectClassHandle objectClass, FederateHandle producer) {
         this.objectClass = objectClass;
         this.producer = producer;
      }
   }

   private static final class Recorder {
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private volatile ObjectInstanceHandle discoveredObject;
      private volatile Discovery discovery;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)) {
                  discovery = new Discovery((ObjectClassHandle) arguments[1],
                     (FederateHandle) arguments[3]);
                  discoveredObject = (ObjectInstanceHandle) arguments[0];
               } else if ("reflectAttributeValues".equals(name)) {
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     arguments.length != 6));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK attribute FIFO callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }

      private boolean discovered(ObjectInstanceHandle object) {
         return object.equals(discoveredObject);
      }

      private List<Reflection> reflections() { return List.copyOf(reflections); }
      private void clearReflections() { reflections.clear(); }
   }
}
