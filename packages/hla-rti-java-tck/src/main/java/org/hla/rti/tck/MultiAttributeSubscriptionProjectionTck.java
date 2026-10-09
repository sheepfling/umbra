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

/** Checks standard multi-attribute reflection projection for three subscription sets. */
final class MultiAttributeSubscriptionProjectionTck {
   private MultiAttributeSubscriptionProjectionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String firstAttributeName, String secondAttributeName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador[] rti = new RTIambassador[4];
      Recorder[] recorders = new Recorder[4];
      boolean[] connected = new boolean[4];
      boolean[] joined = new boolean[4];
      boolean[] subscribed = new boolean[4];
      ObjectClassHandle[] classes = new ObjectClassHandle[4];
      AttributeHandle[] firstAttributes = new AttributeHandle[4];
      AttributeHandle[] secondAttributes = new AttributeHandle[4];
      boolean created = false;
      boolean published = false;
      String federation = "java-tck-multi-attribute-projection-" + UUID.randomUUID();
      try {
         for (int i = 0; i < rti.length; ++i) {
            rti[i] = factory.getRtiAmbassador();
            recorders[i] = new Recorder();
            rti[i].connect(recorders[i].proxy(), callbackModel);
            connected[i] = true;
         }
         rti[0].createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = rti[0].joinFederationExecution(
            "java-tck-multi-attribute-publisher", federation);
         joined[0] = true;
         rti[1].joinFederationExecution("java-tck-multi-attribute-both", federation);
         joined[1] = true;
         rti[2].joinFederationExecution("java-tck-multi-attribute-first", federation);
         joined[2] = true;
         rti[3].joinFederationExecution("java-tck-multi-attribute-second", federation);
         joined[3] = true;

         for (int i = 0; i < rti.length; ++i) {
            classes[i] = rti[i].getObjectClassHandle(objectClassName);
            firstAttributes[i] = rti[i].getAttributeHandle(classes[i], firstAttributeName);
            secondAttributes[i] = rti[i].getAttributeHandle(classes[i], secondAttributeName);
            JavaTckSupport.check(classes[i] != null && firstAttributes[i] != null
                  && secondAttributes[i] != null,
               "adapter multi-attribute FOM lookup returned a null handle");
            JavaTckSupport.check(!firstAttributes[i].equals(secondAttributes[i]),
               "adapter multi-attribute FOM resolved both names to one attribute handle");
         }
         for (int i = 1; i < rti.length; ++i) {
            JavaTckSupport.check(classes[0].equals(classes[i])
                  && firstAttributes[0].equals(firstAttributes[i])
                  && secondAttributes[0].equals(secondAttributes[i]),
               "multi-attribute handles changed across federates");
         }

         AttributeHandleSet publishedAttributes = JavaTckSupport.attributeSet(
            rti[0], firstAttributes[0], secondAttributes[0]);
         AttributeHandleSet bothSet = JavaTckSupport.attributeSet(
            rti[1], firstAttributes[1], secondAttributes[1]);
         AttributeHandleSet firstOnlySet = JavaTckSupport.attributeSet(rti[2], firstAttributes[2]);
         AttributeHandleSet secondOnlySet = JavaTckSupport.attributeSet(rti[3], secondAttributes[3]);
         rti[0].publishObjectClassAttributes(classes[0], publishedAttributes);
         published = true;
         rti[1].subscribeObjectClassAttributes(classes[1], bothSet);
         subscribed[1] = true;
         rti[2].subscribeObjectClassAttributes(classes[2], firstOnlySet);
         subscribed[2] = true;
         rti[3].subscribeObjectClassAttributes(classes[3], secondOnlySet);
         subscribed[3] = true;

         ObjectInstanceHandle object = rti[0].registerObjectInstance(classes[0]);
         await(rti[1], () -> recorders[1].discovered(object),
            "both-attribute subscriber did not discover the object before the deadline");
         await(rti[2], () -> recorders[2].discovered(object),
            "first-only subscriber did not discover the object before the deadline");
         await(rti[3], () -> recorders[3].discovered(object),
            "second-only subscriber did not discover the object before the deadline");
         for (int i = 1; i < rti.length; ++i) {
            JavaTckSupport.check(recorders[i].discoveredObject.equals(object),
               "multi-attribute subscriber discovered a different object handle");
         }
         for (Recorder recorder : recorders) {
            recorder.clearReflections();
         }

         byte[] bothTag = new byte[] {(byte) 0xb1, 0x01};
         byte[] firstTag = new byte[] {(byte) 0xf1, 0x02};
         byte[] secondTag = new byte[] {0x51, 0x03};
         update(rti[0], object, firstAttributes[0], secondAttributes[0], true, true,
            0x11, 0x22, bothTag);
         await(rti[1], () -> recorders[1].reflections().size() >= 1,
            "both-attribute subscriber missed the combined update");
         await(rti[2], () -> recorders[2].reflections().size() >= 1,
            "first-only subscriber missed the combined update");
         await(rti[3], () -> recorders[3].reflections().size() >= 1,
            "second-only subscriber missed the combined update");
         assertReflection(rti[1], recorders[1].reflections().get(0), object,
            firstAttributes[1], secondAttributes[1], new byte[] {0x11},
            new byte[] {0x22}, bothTag, producer, "both-attribute subscriber");
         assertReflection(rti[2], recorders[2].reflections().get(0), object,
            firstAttributes[2], secondAttributes[2], new byte[] {0x11}, null,
            bothTag, producer, "first-only subscriber");
         assertReflection(rti[3], recorders[3].reflections().get(0), object,
            firstAttributes[3], secondAttributes[3], null, new byte[] {0x22},
            bothTag, producer, "second-only subscriber");

         update(rti[0], object, firstAttributes[0], secondAttributes[0], true, false,
            0x33, 0x00, firstTag);
         await(rti[1], () -> recorders[1].reflections().size() >= 2,
            "both-attribute subscriber missed the first-only update");
         await(rti[2], () -> recorders[2].reflections().size() >= 2,
            "first-only subscriber missed its attribute update");
         drain(rti[3]);
         JavaTckSupport.check(recorders[3].reflections().size() == 1,
            "second-only subscriber received an unrequested first-attribute update");
         assertReflection(rti[1], recorders[1].reflections().get(1), object,
            firstAttributes[1], secondAttributes[1], new byte[] {0x33}, null,
            firstTag, producer, "both-attribute subscriber first-only projection");
         assertReflection(rti[2], recorders[2].reflections().get(1), object,
            firstAttributes[2], secondAttributes[2], new byte[] {0x33}, null,
            firstTag, producer, "first-only subscriber projection");

         update(rti[0], object, firstAttributes[0], secondAttributes[0], false, true,
            0x00, 0x44, secondTag);
         await(rti[1], () -> recorders[1].reflections().size() >= 3,
            "both-attribute subscriber missed the second-only update");
         await(rti[3], () -> recorders[3].reflections().size() >= 2,
            "second-only subscriber missed its attribute update");
         JavaTckSupport.check(recorders[2].reflections().size() == 2,
            "first-only subscriber received an unrequested second-attribute update");
         assertReflection(rti[1], recorders[1].reflections().get(2), object,
            firstAttributes[1], secondAttributes[1], null, new byte[] {0x44},
            secondTag, producer, "both-attribute subscriber second-only projection");
         assertReflection(rti[3], recorders[3].reflections().get(1), object,
            firstAttributes[3], secondAttributes[3], null, new byte[] {0x44},
            secondTag, producer, "second-only subscriber projection");
         JavaTckSupport.check(recorders[0].reflections().isEmpty(),
            "ordinary multi-attribute update was reflected to its publisher");
      } finally {
         for (int i = 1; i < rti.length; ++i) {
            if (rti[i] != null && joined[i] && subscribed[i]) {
               try {
                  AttributeHandleSet set = i == 1
                     ? JavaTckSupport.attributeSet(rti[i], firstAttributes[i], secondAttributes[i])
                     : JavaTckSupport.attributeSet(rti[i],
                        i == 2 ? firstAttributes[i] : secondAttributes[i]);
                  rti[i].unsubscribeObjectClassAttributes(classes[i], set);
               } catch (Exception ignored) { }
            }
         }
         if (rti[0] != null && joined[0] && published) {
            try {
               rti[0].unpublishObjectClassAttributes(classes[0],
                  JavaTckSupport.attributeSet(rti[0], firstAttributes[0], secondAttributes[0]));
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

   private static void update(RTIambassador publisher, ObjectInstanceHandle object,
         AttributeHandle first, AttributeHandle second, boolean includeFirst,
         boolean includeSecond, int firstValue, int secondValue, byte[] tag) throws Exception {
      AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(2);
      if (includeFirst) values.put(first, new byte[] {(byte) firstValue});
      if (includeSecond) values.put(second, new byte[] {(byte) secondValue});
      publisher.updateAttributeValues(object, values, tag);
   }

   private static void assertReflection(RTIambassador receiver, Reflection reflection,
         ObjectInstanceHandle object, AttributeHandle first, AttributeHandle second,
         byte[] expectedFirst, byte[] expectedSecond, byte[] expectedTag,
         FederateHandle producer, String description) throws Exception {
      TransportationTypeHandle reliable = receiver.getTransportationTypeHandle("HLAreliable");
      boolean firstMatches = expectedFirst == null
         ? !reflection.values.containsKey(first)
         : Arrays.equals(expectedFirst, reflection.values.get(first));
      boolean secondMatches = expectedSecond == null
         ? !reflection.values.containsKey(second)
         : Arrays.equals(expectedSecond, reflection.values.get(second));
      int expectedCount = (expectedFirst == null ? 0 : 1) + (expectedSecond == null ? 0 : 1);
      JavaTckSupport.check(!reflection.timestampOrdered && object.equals(reflection.object)
            && reflection.values.size() == expectedCount && firstMatches && secondMatches
            && Arrays.equals(expectedTag, reflection.tag)
            && producer.equals(reflection.producer)
            && reliable.equals(reflection.transportation),
         description + " received the wrong projected values or delivery metadata");
   }

   private static void drain(RTIambassador ambassador) throws Exception {
      for (int i = 0; i < 8; ++i) JavaTckSupport.drain(ambassador);
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

   private static final class Recorder {
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private volatile ObjectInstanceHandle discoveredObject;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)) {
                  discoveredObject = (ObjectInstanceHandle) arguments[0];
               } else if ("reflectAttributeValues".equals(name)) {
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     arguments.length != 6));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK attribute projection callbacks";
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
