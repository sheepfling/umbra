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

/** Verifies DELETE_OBJECTS_THEN_DIVEST across the departing federate's objects. */
final class ResignDeleteThenDivestCrossObjectTck {
   private ResignDeleteThenDivestCrossObjectTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador registrar = factory.getRtiAmbassador();
      RTIambassador departing = factory.getRtiAmbassador();
      RTIambassador recipient = factory.getRtiAmbassador();
      Recorder registrarCallbacks = new Recorder();
      Recorder departingCallbacks = new Recorder();
      Recorder recipientCallbacks = new Recorder();
      boolean[] connected = new boolean[3];
      boolean[] joined = new boolean[3];
      boolean created = false;
      String federation = "java-tck-resign-delete-then-divest-" + UUID.randomUUID();
      try {
         registrar.connect(registrarCallbacks.proxy(), callbackModel);
         connected[0] = true;
         departing.connect(departingCallbacks.proxy(), callbackModel);
         connected[1] = true;
         recipient.connect(recipientCallbacks.proxy(), callbackModel);
         connected[2] = true;

         registrar.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         registrar.joinFederationExecution("java-tck-divest-registrar", federation);
         joined[0] = true;
         FederateHandle departingHandle = departing.joinFederationExecution(
            "java-tck-divest-departing", federation);
         joined[1] = true;
         recipient.joinFederationExecution("java-tck-divest-recipient", federation);
         joined[2] = true;

         ObjectClassHandle registrarClass = registrar.getObjectClassHandle(objectClassName);
         ObjectClassHandle departingClass = departing.getObjectClassHandle(objectClassName);
         ObjectClassHandle recipientClass = recipient.getObjectClassHandle(objectClassName);
         AttributeHandle registrarAttribute = registrar.getAttributeHandle(
            registrarClass, attributeName);
         AttributeHandle departingAttribute = departing.getAttributeHandle(
            departingClass, attributeName);
         AttributeHandle recipientAttribute = recipient.getAttributeHandle(
            recipientClass, attributeName);
         JavaTckSupport.check(registrarClass != null && departingClass != null
               && recipientClass != null && registrarAttribute != null
               && departingAttribute != null && recipientAttribute != null,
            "adapter FOM did not resolve the class and attribute in all federates");
         JavaTckSupport.check(registrarClass.equals(departingClass)
               && registrarClass.equals(recipientClass)
               && registrarAttribute.equals(departingAttribute)
               && registrarAttribute.equals(recipientAttribute),
            "class or attribute handles changed across joined federates");

         AttributeHandleSet[] attributes = {
            JavaTckSupport.attributeSet(registrar, registrarAttribute),
            JavaTckSupport.attributeSet(departing, departingAttribute),
            JavaTckSupport.attributeSet(recipient, recipientAttribute)
         };
         registrar.publishObjectClassAttributes(registrarClass, attributes[0]);
         departing.publishObjectClassAttributes(departingClass, attributes[1]);
         recipient.publishObjectClassAttributes(recipientClass, attributes[2]);
         registrar.subscribeObjectClassAttributes(registrarClass, attributes[0]);
         departing.subscribeObjectClassAttributes(departingClass, attributes[1]);
         recipient.subscribeObjectClassAttributes(recipientClass, attributes[2]);

         ObjectInstanceHandle retainedObject = registrar.registerObjectInstance(registrarClass);
         String retainedName = registrar.getObjectInstanceName(retainedObject);
         await(departing, () -> departingCallbacks.discoveries().size() >= 1,
            "departing federate did not discover the retained object");
         await(recipient, () -> recipientCallbacks.discoveries().size() >= 1,
            "recipient did not discover the retained object");
         JavaTckSupport.check(registrarCallbacks.discoveries().isEmpty()
               && departingCallbacks.discoveries().equals(List.of(retainedObject))
               && recipientCallbacks.discoveries().equals(List.of(retainedObject)),
            "retained-object discovery had incorrect recipients or identity");

         registrar.unconditionalAttributeOwnershipDivestiture(
            retainedObject, attributes[0], new byte[0]);
         byte[] departingTag = {(byte) 0xd7, 0x25, 0x01};
         departing.attributeOwnershipAcquisitionIfAvailable(
            retainedObject, attributes[1], departingTag);
         await(departing, () -> !departingCallbacks.acquisitions().isEmpty(),
            "departing federate did not acquire the retained object's attribute");
         Acquisition departingAcquisition = departingCallbacks.acquisitions().get(0);
         JavaTckSupport.check(departingAcquisition.matches(
               retainedObject, departingAttribute, departingTag)
               && departing.isAttributeOwnedByFederate(retainedObject, departingAttribute),
            "departing federate did not acquire the retained-object attribute with its tag");

         ObjectInstanceHandle deletedObject = departing.registerObjectInstance(departingClass);
         String deletedName = departing.getObjectInstanceName(deletedObject);
         await(registrar, () -> registrarCallbacks.discoveries().size() >= 1,
            "registrar did not discover the departing object's registration");
         await(recipient, () -> recipientCallbacks.discoveries().size() >= 2,
            "recipient did not discover the departing object's registration");
         JavaTckSupport.check(deletedObject != null && !deletedObject.equals(retainedObject)
               && !deletedName.equals(retainedName)
               && registrarCallbacks.discoveries().size() == 1
               && registrarCallbacks.discoveries().contains(deletedObject)
               && recipientCallbacks.discoveries().size() == 2
               && recipientCallbacks.discoveries().containsAll(
                  List.of(retainedObject, deletedObject)),
            "departing-object discovery had incorrect recipients or identity");

         registrarCallbacks.clearRemovals();
         recipientCallbacks.clearRemovals();
         departing.resignFederationExecution(ResignAction.DELETE_OBJECTS_THEN_DIVEST);
         joined[1] = false;
         await(registrar, () -> !registrarCallbacks.removals().isEmpty(),
            "registrar did not receive removal for the departing object's deletion");
         await(recipient, () -> !recipientCallbacks.removals().isEmpty(),
            "recipient did not receive removal for the departing object's deletion");
         JavaTckSupport.check(registrarCallbacks.removals().size() == 1
               && registrarCallbacks.removals().get(0).matches(deletedObject, departingHandle)
               && recipientCallbacks.removals().size() == 1
               && recipientCallbacks.removals().get(0).matches(deletedObject, departingHandle),
            "resignation removal callbacks had incorrect object, tag, producer, or multiplicity");
         JavaTckSupport.check(retainedObject.equals(
               registrar.getObjectInstanceHandle(retainedName))
               && retainedObject.equals(recipient.getObjectInstanceHandle(retainedName)),
            "resignation deleted an object registered by another federate");
         expectFailure(() -> recipient.getObjectInstanceHandle(deletedName),
            "ObjectInstanceNotKnown", "lookup of the deleted object name");

         recipientCallbacks.clearAcquisitions();
         byte[] recipientTag = {(byte) 0xd7, 0x25, 0x02};
         recipient.attributeOwnershipAcquisitionIfAvailable(
            retainedObject, attributes[2], recipientTag);
         await(recipient, () -> !recipientCallbacks.acquisitions().isEmpty(),
            "recipient could not acquire the attribute divested at resignation");
         Acquisition recipientAcquisition = recipientCallbacks.acquisitions().get(0);
         JavaTckSupport.check(recipientAcquisition.matches(
               retainedObject, recipientAttribute, recipientTag)
               && !registrar.isAttributeOwnedByFederate(retainedObject, registrarAttribute)
               && recipient.isAttributeOwnedByFederate(retainedObject, recipientAttribute),
            "resignation did not release the retained-object attribute to another federate");

         registrarCallbacks.clearRemovals();
         registrar.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         joined[0] = false;
         await(recipient, () -> !recipientCallbacks.removals().isEmpty(),
            "recipient did not receive cleanup removal for the retained object");
         recipient.resignFederationExecution(ResignAction.NO_ACTION);
         joined[2] = false;
      } finally {
         if (joined[1]) resign(departing, ResignAction.DELETE_OBJECTS_THEN_DIVEST);
         if (joined[0]) resign(registrar, ResignAction.DELETE_OBJECTS);
         if (joined[2]) resign(recipient, ResignAction.DELETE_OBJECTS_THEN_DIVEST);
         if (created) {
            try { registrar.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         for (int i = 2; i >= 0; --i) {
            if (connected[i]) {
               try { ambassador(i, registrar, departing, recipient).disconnect(); }
               catch (Exception ignored) { }
            }
         }
      }
   }

   private static RTIambassador ambassador(int index, RTIambassador registrar,
         RTIambassador departing, RTIambassador recipient) {
      return index == 0 ? registrar : index == 1 ? departing : recipient;
   }

   private static void resign(RTIambassador ambassador, ResignAction action) {
      try { ambassador.resignFederationExecution(action); }
      catch (Exception ignored) { }
   }

   private static void expectFailure(CheckedCall call, String expected, String operation)
         throws Exception {
      try {
         call.run();
      } catch (Exception failure) {
         JavaTckSupport.check(expected.equals(failure.getClass().getSimpleName()),
            operation + " threw " + failure.getClass().getSimpleName()
               + " instead of " + expected);
         return;
      }
      throw new AssertionError(operation + " unexpectedly succeeded");
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

   @FunctionalInterface
   private interface CheckedCall { void run() throws Exception; }

   private static final class Acquisition {
      private final ObjectInstanceHandle object;
      private final AttributeHandleSet attributes;
      private final byte[] tag;
      private Acquisition(ObjectInstanceHandle object, AttributeHandleSet attributes, byte[] tag) {
         this.object = object;
         this.attributes = attributes;
         this.tag = tag == null ? new byte[0] : tag.clone();
      }
      private boolean matches(ObjectInstanceHandle expectedObject, AttributeHandle attribute,
            byte[] expectedTag) {
         return expectedObject.equals(object) && attributes.size() == 1
            && attributes.contains(attribute) && Arrays.equals(tag, expectedTag);
      }
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
      private boolean matches(ObjectInstanceHandle expectedObject, FederateHandle expectedProducer) {
         return expectedObject.equals(object) && tag.length == 0
            && expectedProducer.equals(producer);
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Removal> removals = new CopyOnWriteArrayList<>();
      private final List<Acquisition> acquisitions = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("removeObjectInstance".equals(name) && arguments != null
                     && arguments.length == 3) {
                  removals.add(new Removal((ObjectInstanceHandle) arguments[0],
                     (byte[]) arguments[1], (FederateHandle) arguments[2]));
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)
                     && arguments != null && arguments.length == 3) {
                  acquisitions.add(new Acquisition((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (byte[]) arguments[2]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK resign divest callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == arguments[0];
               }
               return null;
            });
      }

      private List<ObjectInstanceHandle> discoveries() { return List.copyOf(discoveries); }
      private List<Removal> removals() { return List.copyOf(removals); }
      private List<Acquisition> acquisitions() { return List.copyOf(acquisitions); }
      private void clearRemovals() { removals.clear(); }
      private void clearAcquisitions() { acquisitions.clear(); }
   }
}
