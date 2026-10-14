package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
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

/** Verifies unconditional-divest resignation leaves another federate's object registered. */
final class ResignUnconditionallyDivestPreservesObjectsTck {
   private ResignUnconditionallyDivestPreservesObjectsTck() { }

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
      String federation = "java-tck-resign-unconditional-divest-" + UUID.randomUUID();
      try {
         registrar.connect(registrarCallbacks.proxy(), callbackModel);
         connected[0] = true;
         departing.connect(departingCallbacks.proxy(), callbackModel);
         connected[1] = true;
         recipient.connect(recipientCallbacks.proxy(), callbackModel);
         connected[2] = true;

         registrar.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         registrar.joinFederationExecution("java-tck-uncond-owner", federation);
         joined[0] = true;
         departing.joinFederationExecution("java-tck-uncond-departing", federation);
         joined[1] = true;
         recipient.joinFederationExecution("java-tck-uncond-recipient", federation);
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

         ObjectInstanceHandle object = registrar.registerObjectInstance(registrarClass);
         String objectName = registrar.getObjectInstanceName(object);
         await(departing, () -> !departingCallbacks.discoveries().isEmpty(),
            "departing federate did not discover the retained object");
         await(recipient, () -> !recipientCallbacks.discoveries().isEmpty(),
            "recipient did not discover the retained object");
         JavaTckSupport.check(departingCallbacks.discoveries().equals(List.of(object))
               && recipientCallbacks.discoveries().equals(List.of(object)),
            "retained-object discovery had incorrect identity or multiplicity");

         registrar.unconditionalAttributeOwnershipDivestiture(
            object, attributes[0], new byte[0]);
         byte[] departingTag = {(byte) 0xd7, 0x25, 0x11};
         departing.attributeOwnershipAcquisitionIfAvailable(
            object, attributes[1], departingTag);
         await(departing, () -> !departingCallbacks.acquisitions().isEmpty(),
            "departing federate did not acquire the retained object's attribute");
         Acquisition setup = departingCallbacks.acquisitions().get(0);
         JavaTckSupport.check(setup.matches(object, departingAttribute, departingTag)
               && departing.isAttributeOwnedByFederate(object, departingAttribute),
            "setup did not establish ownership of the retained object's attribute");

         registrarCallbacks.clearRemovals();
         recipientCallbacks.clearRemovals();
         departing.resignFederationExecution(ResignAction.UNCONDITIONALLY_DIVEST_ATTRIBUTES);
         joined[1] = false;
         JavaTckSupport.check(object.equals(registrar.getObjectInstanceHandle(objectName))
               && object.equals(recipient.getObjectInstanceHandle(objectName)),
            "unconditional-divest resignation removed the surviving federate's object");
         JavaTckSupport.check(registrarCallbacks.removals().isEmpty()
               && recipientCallbacks.removals().isEmpty()
               && !registrar.isAttributeOwnedByFederate(object, registrarAttribute),
            "resignation removed the object or retained the departed ownership");

         byte[] recipientTag = {(byte) 0xd7, 0x25, 0x12};
         recipient.attributeOwnershipAcquisitionIfAvailable(
            object, attributes[2], recipientTag);
         await(recipient, () -> !recipientCallbacks.acquisitions().isEmpty(),
            "recipient could not acquire the attribute released at resignation");
         Acquisition acquisition = recipientCallbacks.acquisitions().get(0);
         JavaTckSupport.check(acquisition.matches(object, recipientAttribute, recipientTag)
               && recipient.isAttributeOwnedByFederate(object, recipientAttribute),
            "released attribute acquisition had incorrect object, set, tag, or ownership");

         registrar.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         joined[0] = false;
         await(recipient, () -> !recipientCallbacks.removals().isEmpty(),
            "recipient did not receive cleanup removal for the retained object");
         JavaTckSupport.check(recipientCallbacks.removals().equals(List.of(object)),
            "cleanup removal had incorrect object identity or multiplicity");
         recipient.resignFederationExecution(ResignAction.NO_ACTION);
         joined[2] = false;
      } finally {
         if (joined[1]) resign(departing, ResignAction.UNCONDITIONALLY_DIVEST_ATTRIBUTES);
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

   private static void await(RTIambassador ambassador, BooleanSupplier condition,
         String failure) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         JavaTckSupport.drain(ambassador);
         Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), failure);
   }

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

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<ObjectInstanceHandle> removals = new CopyOnWriteArrayList<>();
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
                     && arguments.length >= 1) {
                  removals.add((ObjectInstanceHandle) arguments[0]);
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)
                     && arguments != null && arguments.length == 3) {
                  acquisitions.add(new Acquisition((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (byte[]) arguments[2]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK unconditional-divest callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == arguments[0];
               }
               return null;
            });
      }

      private List<ObjectInstanceHandle> discoveries() { return List.copyOf(discoveries); }
      private List<ObjectInstanceHandle> removals() { return List.copyOf(removals); }
      private List<Acquisition> acquisitions() { return List.copyOf(acquisitions); }
      private void clearRemovals() { removals.clear(); }
   }
}
