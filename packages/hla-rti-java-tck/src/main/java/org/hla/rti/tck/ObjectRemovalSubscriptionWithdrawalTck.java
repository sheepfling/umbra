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

/** Verifies terminal removal delivery when one discovered subscriber withdraws. */
final class ObjectRemovalSubscriptionWithdrawalTck {
   private ObjectRemovalSubscriptionWithdrawalTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String attributeName, CallbackModel callbackModel) throws Exception {
      RTIambassador[] ambassadors = new RTIambassador[3];
      Recorder[] callbacks = new Recorder[3];
      boolean[] connected = new boolean[3];
      boolean[] joined = new boolean[3];
      boolean[] subscribed = new boolean[3];
      ObjectClassHandle[] classes = new ObjectClassHandle[3];
      AttributeHandle[] attributes = new AttributeHandle[3];
      boolean created = false;
      boolean published = false;
      String federation = "java-tck-removal-withdrawal-" + UUID.randomUUID();
      try {
         for (int i = 0; i < ambassadors.length; ++i) {
            ambassadors[i] = factory.getRtiAmbassador();
            callbacks[i] = new Recorder();
            ambassadors[i].connect(callbacks[i].proxy(), callbackModel);
            connected[i] = true;
         }
         ambassadors[0].createFederationExecution(federation, fom);
         created = true;
         FederateHandle producer = ambassadors[0].joinFederationExecution(
            "java-tck-removal-owner", federation);
         joined[0] = true;
         ambassadors[1].joinFederationExecution("java-tck-removal-active", federation);
         joined[1] = true;
         ambassadors[2].joinFederationExecution("java-tck-removal-withdrawn", federation);
         joined[2] = true;

         for (int i = 0; i < ambassadors.length; ++i) {
            classes[i] = ambassadors[i].getObjectClassHandle(objectClassName);
            attributes[i] = ambassadors[i].getAttributeHandle(classes[i], attributeName);
            JavaTckSupport.check(classes[i] != null && attributes[i] != null,
               "adapter FOM did not resolve the object class and attribute");
         }
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(
            ambassadors[0], attributes[0]);
         ambassadors[0].publishObjectClassAttributes(classes[0], ownerAttributes);
         published = true;
         for (int i = 1; i < ambassadors.length; ++i) {
            ambassadors[i].subscribeObjectClassAttributes(classes[i],
               JavaTckSupport.attributeSet(ambassadors[i], attributes[i]));
            subscribed[i] = true;
         }

         ObjectInstanceHandle object = ambassadors[0].registerObjectInstance(classes[0]);
         JavaTckSupport.check(object != null, "object registration returned a null handle");
         await(ambassadors[1], () -> callbacks[1].discoveries().contains(object),
            "active subscriber did not discover the object");
         await(ambassadors[2], () -> callbacks[2].discoveries().contains(object),
            "withdrawn subscriber did not discover the object");
         for (Recorder recorder : callbacks) recorder.clearRemovals();

         byte[] tag = new byte[] {0x4f, 0x42, 0x4a};
         ambassadors[0].deleteObjectInstance(object, tag);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(callbacks[1].removals().isEmpty()
                  && callbacks[2].removals().isEmpty(),
               "evoked removal callback arrived before callback servicing");
            ambassadors[2].unsubscribeObjectClassAttributes(classes[2],
               JavaTckSupport.attributeSet(ambassadors[2], attributes[2]));
            subscribed[2] = false;
         }

         await(ambassadors[1], () -> callbacks[1].removals().size() == 1,
            "active subscriber did not receive the terminal removal");
         await(ambassadors[2], () -> callbacks[2].removals().size() == 1,
            "withdrawn subscriber did not receive its already-queued terminal removal");
         if (callbackModel == CallbackModel.HLA_IMMEDIATE) {
            ambassadors[2].unsubscribeObjectClassAttributes(classes[2],
               JavaTckSupport.attributeSet(ambassadors[2], attributes[2]));
            subscribed[2] = false;
         }
         JavaTckSupport.check(callbacks[0].removals().isEmpty(),
            "ordinary removal looped back to its owner");
         assertRemoval(callbacks[1].removals().get(0), object, tag, producer,
            "active subscriber");
         assertRemoval(callbacks[2].removals().get(0), object, tag, producer,
            "withdrawn subscriber");

         ambassadors[1].unsubscribeObjectClassAttributes(classes[1],
            JavaTckSupport.attributeSet(ambassadors[1], attributes[1]));
         subscribed[1] = false;
         ambassadors[0].unpublishObjectClassAttributes(classes[0], ownerAttributes);
         published = false;
         for (int i = 2; i >= 0; --i) {
            ambassadors[i].resignFederationExecution(ResignAction.NO_ACTION);
            joined[i] = false;
         }
         ambassadors[0].destroyFederationExecution(federation);
         created = false;
      } finally {
         for (int i = 1; i < ambassadors.length; ++i) {
            if (ambassadors[i] != null && joined[i] && subscribed[i]) {
               try {
                  ambassadors[i].unsubscribeObjectClassAttributes(classes[i],
                     JavaTckSupport.attributeSet(ambassadors[i], attributes[i]));
               } catch (Exception ignored) { }
            }
         }
         if (ambassadors[0] != null && joined[0] && published) {
            try {
               ambassadors[0].unpublishObjectClassAttributes(classes[0],
                  JavaTckSupport.attributeSet(ambassadors[0], attributes[0]));
            } catch (Exception ignored) { }
         }
         for (int i = ambassadors.length - 1; i >= 0; --i) {
            if (ambassadors[i] != null && joined[i]) {
               try { ambassadors[i].resignFederationExecution(ResignAction.NO_ACTION); }
               catch (Exception ignored) { }
            }
         }
         if (created && ambassadors[0] != null) {
            try { ambassadors[0].destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         for (int i = ambassadors.length - 1; i >= 0; --i) {
            if (ambassadors[i] != null && connected[i]) {
               try { ambassadors[i].disconnect(); }
               catch (Exception ignored) { }
            }
         }
      }
   }

   private static void assertRemoval(Removal removal, ObjectInstanceHandle object,
         byte[] tag, FederateHandle producer, String recipient) {
      JavaTckSupport.check(object.equals(removal.object) && Arrays.equals(tag, removal.tag)
            && producer.equals(removal.producer),
         recipient + " received incorrect terminal-removal metadata");
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
                  if ("toString".equals(name)) return "Java TCK removal withdrawal callbacks";
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
