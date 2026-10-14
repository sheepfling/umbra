package org.hla.rti.tck;

import java.lang.reflect.Proxy;
import java.util.UUID;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;

/** Portable IEEE 1516.1-2025 NO_ACTION rejection while object attributes are owned. */
final class ResignNoActionOwnedAttributesTck {
   private ResignNoActionOwnedAttributesTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-resign-no-action-owned-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean memberConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean memberJoined = false;
      boolean ownerResigned = false;
      boolean memberResigned = false;
      try {
         owner.connect(callbackProxy(), callbackModel);
         ownerConnected = true;
         member.connect(callbackProxy(), callbackModel);
         memberConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("no-action-owner", "tck-type", federation);
         ownerJoined = true;
         member.joinFederationExecution("no-action-member", "tck-type", federation);
         memberJoined = true;

         ObjectClassHandle objectClass = owner.getObjectClassHandle(objectClassName);
         AttributeHandle attribute = owner.getAttributeHandle(objectClass, attributeName);
         JavaTckSupport.check(objectClass != null && attribute != null,
            "object-class or attribute lookup returned null");
         AttributeHandleSet attributes = JavaTckSupport.attributeSet(owner, attribute);
         owner.publishObjectClassAttributes(objectClass, attributes);

         ObjectInstanceHandle object = owner.registerObjectInstance(objectClass);
         JavaTckSupport.check(object != null,
            "registration returned no object-instance handle");
         String objectName = owner.getObjectInstanceName(object);
         JavaTckSupport.check(objectName != null && !objectName.isEmpty(),
            "registration returned an empty object-instance name");

         JavaTckSupport.expectFailure(
            () -> owner.resignFederationExecution(ResignAction.NO_ACTION),
            "FederateOwnsAttributes",
            "resignation with NO_ACTION while owning an object's attributes");
         JavaTckSupport.check(objectName.equals(owner.getObjectInstanceName(object)),
            "rejected NO_ACTION resignation changed object identity");

         owner.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         ownerResigned = true;
         member.resignFederationExecution(ResignAction.NO_ACTION);
         memberResigned = true;
      } finally {
         if (ownerJoined && !ownerResigned) {
            try {
               owner.resignFederationExecution(ResignAction.DELETE_OBJECTS);
               ownerResigned = true;
            } catch (Exception ignored) { }
         }
         if (memberJoined && !memberResigned) {
            try {
               member.resignFederationExecution(ResignAction.NO_ACTION);
               memberResigned = true;
            } catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (memberConnected) {
            try { member.disconnect(); }
            catch (Exception ignored) { }
         }
         if (ownerConnected) {
            try { owner.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static FederateAmbassador callbackProxy() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK no-action callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
