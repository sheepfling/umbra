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

/** Checks FIFO ordinary removal delivery to two subscribers without owner loopback. */
final class ObjectRemovalMultiRecipientFifoTck {
   private ObjectRemovalMultiRecipientFifoTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation, String objectClassName,
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
      String federation = "java-tck-object-removal-fanout-" + UUID.randomUUID();
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
            "java-tck-removal-publisher", federation);
         joined[0] = true;
         rti[1].joinFederationExecution("java-tck-removal-first", federation);
         joined[1] = true;
         rti[2].joinFederationExecution("java-tck-removal-second", federation);
         joined[2] = true;

         for (int i = 0; i < rti.length; ++i) {
            classes[i] = rti[i].getObjectClassHandle(objectClassName);
            attributes[i] = rti[i].getAttributeHandle(classes[i], attributeName);
            JavaTckSupport.check(classes[i] != null && attributes[i] != null,
               "adapter FOM did not resolve the removal class and attribute");
         }
         for (int i = 1; i < rti.length; ++i) {
            JavaTckSupport.check(classes[0].equals(classes[i])
                  && attributes[0].equals(attributes[i]),
               "removal class or attribute handles changed across federates");
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

         ObjectInstanceHandle firstObject = rti[0].registerObjectInstance(classes[0]);
         ObjectInstanceHandle secondObject = rti[0].registerObjectInstance(classes[0]);
         JavaTckSupport.check(firstObject != null && secondObject != null
               && !firstObject.equals(secondObject),
            "object removal scenario registered invalid or duplicate object handles");
         await(rti[1], () -> recorders[1].discoveries().size() >= 2,
            "first subscriber did not discover both objects before the deadline");
         await(rti[2], () -> recorders[2].discoveries().size() >= 2,
            "second subscriber did not discover both objects before the deadline");
         JavaTckSupport.check(recorders[0].discoveries().isEmpty()
               && recorders[1].discoveries().size() == 2
               && recorders[2].discoveries().size() == 2
               && recorders[1].discoveryHandles().containsAll(List.of(firstObject, secondObject))
               && recorders[2].discoveryHandles().containsAll(List.of(firstObject, secondObject)),
            "object registration discovery had incorrect recipients or identities");
         recorders[0].clearRemovals();
         recorders[1].clearRemovals();
         recorders[2].clearRemovals();

         byte[] firstTag = new byte[] {(byte) 0xd1, (byte) 0xe2, (byte) 0xf3};
         byte[] secondTag = new byte[] {(byte) 0xa4, (byte) 0xb5, (byte) 0xc6};
         rti[0].deleteObjectInstance(firstObject, firstTag);
         rti[0].deleteObjectInstance(secondObject, secondTag);
         await(rti[1], () -> recorders[1].removals().size() >= 2,
            "first subscriber did not receive both removals before the deadline");
         await(rti[2], () -> recorders[2].removals().size() >= 2,
            "second subscriber did not receive both removals before the deadline");
         JavaTckSupport.check(recorders[0].removals().isEmpty(),
            "ordinary object removal looped back to its owner");
         assertRemoval(recorders[1].removals(), firstObject, secondObject,
            firstTag, secondTag, producer, "first subscriber");
         assertRemoval(recorders[2].removals(), firstObject, secondObject,
            firstTag, secondTag, producer, "second subscriber");

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
               }
               catch (Exception ignored) { }
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

   private static void assertRemoval(List<Removal> removals,
         ObjectInstanceHandle firstObject, ObjectInstanceHandle secondObject,
         byte[] firstTag, byte[] secondTag, FederateHandle producer, String receiver) {
      JavaTckSupport.check(removals.size() == 2
            && firstObject.equals(removals.get(0).object)
            && Arrays.equals(firstTag, removals.get(0).tag)
            && producer.equals(removals.get(0).producer)
            && secondObject.equals(removals.get(1).object)
            && Arrays.equals(secondTag, removals.get(1).tag)
            && producer.equals(removals.get(1).producer),
         receiver + " received removals out of order or with incorrect metadata");
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
                  if ("toString".equals(name)) return "Java TCK object removal callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }

      private List<ObjectInstanceHandle> discoveries() { return List.copyOf(discoveries); }
      private List<ObjectInstanceHandle> discoveryHandles() { return discoveries(); }
      private List<Removal> removals() { return List.copyOf(removals); }
      private void clearRemovals() { removals.clear(); }
   }
}
