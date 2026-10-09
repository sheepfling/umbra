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
import java.util.HashSet;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks ordinary registration discovery fan-out and object identity for two subscribers. */
final class ObjectRegistrationMultiRecipientTck {
   private ObjectRegistrationMultiRecipientTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String attributeName, CallbackModel callbackModel) throws Exception {
      RTIambassador[] rti = new RTIambassador[3];
      Recorder[] recorders = new Recorder[3];
      boolean[] connected = new boolean[3];
      boolean[] joined = new boolean[3];
      boolean[] subscribed = new boolean[3];
      ObjectClassHandle[] classes = new ObjectClassHandle[3];
      AttributeHandle[] attributes = new AttributeHandle[3];
      boolean federationCreated = false;
      boolean published = false;
      String federation = "java-tck-object-registration-fanout-" + UUID.randomUUID();
      try {
         for (int i = 0; i < rti.length; ++i) {
            rti[i] = factory.getRtiAmbassador();
            recorders[i] = new Recorder();
            rti[i].connect(recorders[i].proxy(), callbackModel);
            connected[i] = true;
         }
         rti[0].createFederationExecution(federation, fom);
         federationCreated = true;
         FederateHandle producer = rti[0].joinFederationExecution(
            "java-tck-registration-fanout-publisher", federation);
         joined[0] = true;
         rti[1].joinFederationExecution("java-tck-registration-fanout-first", federation);
         joined[1] = true;
         rti[2].joinFederationExecution("java-tck-registration-fanout-second", federation);
         joined[2] = true;

         for (int i = 0; i < rti.length; ++i) {
            classes[i] = rti[i].getObjectClassHandle(objectClassName);
            attributes[i] = rti[i].getAttributeHandle(classes[i], attributeName);
            JavaTckSupport.check(classes[i] != null && attributes[i] != null,
               "adapter FOM did not resolve the multi-recipient object class and attribute");
         }
         for (int i = 1; i < rti.length; ++i) {
            JavaTckSupport.check(classes[0].equals(classes[i])
                  && attributes[0].equals(attributes[i]),
               "object class or attribute handles changed across federates");
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
            "multi-recipient registration returned invalid or duplicate object handles");
         String firstName = rti[0].getObjectInstanceName(firstObject);
         String secondName = rti[0].getObjectInstanceName(secondObject);
         JavaTckSupport.check(firstName != null && !firstName.isEmpty()
               && secondName != null && !secondName.isEmpty() && !firstName.equals(secondName),
            "multi-recipient registration returned invalid or duplicate object names");

         await(rti[1], () -> recorders[1].discoveries().size() >= 2,
            "first subscriber did not discover both registered objects before the deadline");
         await(rti[2], () -> recorders[2].discoveries().size() >= 2,
            "second subscriber did not discover both registered objects before the deadline");
         JavaTckSupport.check(recorders[0].discoveries().isEmpty()
               && recorders[1].discoveries().size() == 2
               && recorders[2].discoveries().size() == 2,
            "registration discovery delivered an incorrect number of callbacks");

         assertReceiver(rti[1], recorders[1], classes[1], firstObject, secondObject,
            firstName, secondName, producer, "first subscriber");
         assertReceiver(rti[2], recorders[2], classes[2], firstObject, secondObject,
            firstName, secondName, producer, "second subscriber");
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
         if (federationCreated && rti[0] != null) {
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

   private static void assertReceiver(RTIambassador rti, Recorder recorder,
         ObjectClassHandle objectClass, ObjectInstanceHandle firstObject,
         ObjectInstanceHandle secondObject, String firstName, String secondName,
         FederateHandle producer, String description) throws Exception {
      List<Discovery> events = recorder.discoveries();
      JavaTckSupport.check(events.size() == 2,
         description + " did not receive exactly two discovery callbacks");
      HashSet<ObjectInstanceHandle> expected = new HashSet<>();
      expected.add(firstObject);
      expected.add(secondObject);
      HashSet<ObjectInstanceHandle> observed = new HashSet<>();
      for (Discovery event : events) {
         JavaTckSupport.check(event.objectClass.equals(objectClass)
               && producer.equals(event.producer),
            description + " received incorrect discovery class or producer metadata");
         JavaTckSupport.check((event.object.equals(firstObject) && firstName.equals(event.name))
               || (event.object.equals(secondObject) && secondName.equals(event.name)),
            description + " received an incorrect object handle/name pair");
         JavaTckSupport.check(observed.add(event.object),
            description + " received a duplicate discovery callback");
      }
      JavaTckSupport.check(observed.equals(expected),
         description + " did not discover both registered objects");
      JavaTckSupport.check(firstObject.equals(rti.getObjectInstanceHandle(firstName))
            && secondObject.equals(rti.getObjectInstanceHandle(secondName))
            && firstName.equals(rti.getObjectInstanceName(firstObject))
            && secondName.equals(rti.getObjectInstanceName(secondObject))
            && objectClass.equals(rti.getKnownObjectClassHandle(firstObject))
            && objectClass.equals(rti.getKnownObjectClassHandle(secondObject)),
         description + " did not preserve both standard object identities");
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
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length == 4) {
                  discoveries.add(new Discovery((ObjectInstanceHandle) arguments[0],
                     (ObjectClassHandle) arguments[1], (String) arguments[2],
                     (FederateHandle) arguments[3]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK registration discovery recorder";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }

      private List<Discovery> discoveries() { return List.copyOf(discoveries); }
   }
}
