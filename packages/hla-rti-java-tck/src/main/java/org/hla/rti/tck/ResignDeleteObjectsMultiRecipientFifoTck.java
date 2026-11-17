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
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks FIFO object-removal fan-out caused by DELETE_OBJECTS resignation. */
final class ResignDeleteObjectsMultiRecipientFifoTck {
   private ResignDeleteObjectsMultiRecipientFifoTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador[] ambassadors = new RTIambassador[3];
      Recorder[] callbacks = new Recorder[3];
      boolean[] connected = new boolean[3];
      boolean[] joined = new boolean[3];
      boolean[] subscribed = new boolean[3];
      ObjectClassHandle[] classes = new ObjectClassHandle[3];
      AttributeHandle[] attributes = new AttributeHandle[3];
      boolean created = false;
      boolean published = false;
      String federation = "java-tck-resign-delete-fanout-" + UUID.randomUUID();
      try {
         for (int i = 0; i < ambassadors.length; ++i) {
            ambassadors[i] = factory.getRtiAmbassador();
            callbacks[i] = new Recorder();
            ambassadors[i].connect(callbacks[i].proxy(), callbackModel);
            connected[i] = true;
         }
         ambassadors[0].createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = ambassadors[0].joinFederationExecution(
            "java-tck-resign-owner", federation);
         joined[0] = true;
         ambassadors[1].joinFederationExecution("java-tck-resign-first", federation);
         joined[1] = true;
         ambassadors[2].joinFederationExecution("java-tck-resign-second", federation);
         joined[2] = true;
         String producerName = ambassadors[0].getFederateName(producer);
         FederateHandle[] localProducers = new FederateHandle[ambassadors.length];
         for (int i = 1; i < ambassadors.length; ++i) {
            localProducers[i] = ambassadors[i].getFederateHandle(producerName);
            JavaTckSupport.check(producerName.equals(
                  ambassadors[i].getFederateName(localProducers[i])),
               "subscriber did not resolve the producer federate by name");
         }

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

         ObjectInstanceHandle first = ambassadors[0].registerObjectInstance(classes[0]);
         ObjectInstanceHandle second = ambassadors[0].registerObjectInstance(classes[0]);
         JavaTckSupport.check(first != null && second != null && !first.equals(second),
            "resign fan-out registration returned null or duplicate handles");
         String firstName = ambassadors[0].getObjectInstanceName(first);
         String secondName = ambassadors[0].getObjectInstanceName(second);
         JavaTckSupport.check(firstName != null && !firstName.isEmpty()
               && secondName != null && !secondName.isEmpty() && !firstName.equals(secondName),
            "resign fan-out registration returned null or duplicate names");
         for (int i = 1; i < ambassadors.length; ++i) {
            final int recipient = i;
            await(ambassadors[i], () -> callbacks[recipient].hasDiscoveryNames(
               firstName, secondName), "subscriber did not discover both objects");
         }
         ObjectInstanceHandle[][] localObjects = new ObjectInstanceHandle[ambassadors.length][2];
         for (int i = 1; i < ambassadors.length; ++i) {
            localObjects[i][0] = ambassadors[i].getObjectInstanceHandle(firstName);
            localObjects[i][1] = ambassadors[i].getObjectInstanceHandle(secondName);
            JavaTckSupport.check(localObjects[i][0] != null && localObjects[i][1] != null
                  && !localObjects[i][0].equals(localObjects[i][1])
                  && firstName.equals(ambassadors[i].getObjectInstanceName(localObjects[i][0]))
                  && secondName.equals(ambassadors[i].getObjectInstanceName(localObjects[i][1]))
                  && classes[i].equals(ambassadors[i].getKnownObjectClassHandle(localObjects[i][0]))
                  && classes[i].equals(ambassadors[i].getKnownObjectClassHandle(localObjects[i][1])),
               "subscriber did not resolve both objects to its local handles and class");
            assertDiscoveries(callbacks[i].discoveries(), firstName, localObjects[i][0],
               secondName, localObjects[i][1], classes[i], i == 1 ? "first subscriber" :
                  "second subscriber");
         }
         for (Recorder recorder : callbacks) recorder.clearRemovals();

         ambassadors[0].resignFederationExecution(ResignAction.DELETE_OBJECTS);
         joined[0] = false;
         await(ambassadors[1], () -> callbacks[1].removals().size() == 2,
            "first subscriber did not receive both resign-time removals");
         await(ambassadors[2], () -> callbacks[2].removals().size() == 2,
            "second subscriber did not receive both resign-time removals");
         JavaTckSupport.check(callbacks[0].removals().isEmpty(),
            "resign-time object removal looped back to its owner");
         assertRemovals(callbacks[1].removals(), localObjects[1][0], localObjects[1][1],
            localProducers[1], "first subscriber");
         assertRemovals(callbacks[2].removals(), localObjects[2][0], localObjects[2][1],
            localProducers[2], "second subscriber");

         for (int i = 1; i < ambassadors.length; ++i) {
            ambassadors[i].unsubscribeObjectClassAttributes(classes[i],
               JavaTckSupport.attributeSet(ambassadors[i], attributes[i]));
            subscribed[i] = false;
         }
         published = false;
         for (int i = ambassadors.length - 1; i >= 1; --i) {
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
               try {
                  ambassadors[i].resignFederationExecution(
                     i == 0 ? ResignAction.DELETE_OBJECTS : ResignAction.NO_ACTION);
               } catch (Exception ignored) { }
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

   private static void assertRemovals(List<Removal> removals, ObjectInstanceHandle first,
         ObjectInstanceHandle second, FederateHandle producer, String recipient) {
      JavaTckSupport.check(removals.size() == 2
            && first.equals(removals.get(0).object) && removals.get(0).tag.length == 0
            && producer.equals(removals.get(0).producer)
            && second.equals(removals.get(1).object) && removals.get(1).tag.length == 0
            && producer.equals(removals.get(1).producer),
         recipient + " received resign-time removals out of order or with incorrect metadata");
   }

   private static void assertDiscoveries(List<Discovery> discoveries, String firstName,
         ObjectInstanceHandle first, String secondName, ObjectInstanceHandle second,
         ObjectClassHandle objectClass, String recipient) {
      Discovery firstDiscovery = findDiscovery(discoveries, firstName);
      Discovery secondDiscovery = findDiscovery(discoveries, secondName);
      JavaTckSupport.check(discoveries.size() == 2 && firstDiscovery != null
            && first.equals(firstDiscovery.object) && objectClass.equals(firstDiscovery.objectClass)
            && secondDiscovery != null && second.equals(secondDiscovery.object)
            && objectClass.equals(secondDiscovery.objectClass),
         recipient + " discovery callbacks did not preserve its local object identities");
   }

   private static Discovery findDiscovery(List<Discovery> discoveries, String name) {
      for (Discovery discovery : discoveries) {
         if (name.equals(discovery.name)) return discovery;
      }
      return null;
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
         this.tag = tag == null ? new byte[0] : tag.clone();
         this.producer = producer;
      }
   }

   private static final class Discovery {
      private final ObjectInstanceHandle object;
      private final ObjectClassHandle objectClass;
      private final String name;
      private Discovery(ObjectInstanceHandle object, ObjectClassHandle objectClass, String name) {
         this.object = object;
         this.objectClass = objectClass;
         this.name = name;
      }
   }

   private static final class Recorder {
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();
      private final List<Removal> removals = new CopyOnWriteArrayList<>();
      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)) {
                  discoveries.add(new Discovery((ObjectInstanceHandle) arguments[0],
                     (ObjectClassHandle) arguments[1], (String) arguments[2]));
               } else if ("removeObjectInstance".equals(name) && arguments != null
                     && arguments.length == 3) {
                  removals.add(new Removal((ObjectInstanceHandle) arguments[0],
                     (byte[]) arguments[1], (FederateHandle) arguments[2]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK resign deletion callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }
      private List<Discovery> discoveries() { return List.copyOf(discoveries); }
      private boolean hasDiscoveryNames(String first, String second) {
         return findDiscovery(discoveries, first) != null && findDiscovery(discoveries, second) != null;
       }
      private List<Removal> removals() { return List.copyOf(removals); }
      private void clearRemovals() { removals.clear(); }
   }
}
