package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Checks ordinary object-attribute declaration membership boundaries after resignation. */
final class ObjectAttributeDeclarationsAfterResignationTck {
   private ObjectAttributeDeclarationsAfterResignationTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador rti = factory.getRtiAmbassador();
      String federation = "java-tck-object-attribute-declarations-resign-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         rti.connect(callbacks(), callbackModel);
         connected = true;
         rti.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         rti.joinFederationExecution("java-tck-object-attribute-declarations", federation);
         joined = true;

         ObjectClassHandle objectClass = rti.getObjectClassHandle(objectClassName);
         AttributeHandle attribute = rti.getAttributeHandle(objectClass, attributeName);
         JavaTckSupport.check(objectClassName.equals(rti.getObjectClassName(objectClass))
               && attributeName.equals(rti.getAttributeName(objectClass, attribute)),
            "adapter FOM did not resolve the requested object-class attribute handles");
         AttributeHandleSet attributes = JavaTckSupport.attributeSet(rti, attribute);

         rti.publishObjectClassAttributes(objectClass, attributes);
         rti.unpublishObjectClassAttributes(objectClass, attributes);
         rti.publishObjectClassAttributes(objectClass, attributes);
         rti.unpublishObjectClass(objectClass);
         rti.subscribeObjectClassAttributes(objectClass, attributes);
         rti.unsubscribeObjectClassAttributes(objectClass, attributes);
         rti.subscribeObjectClassAttributes(objectClass, attributes);
         rti.unsubscribeObjectClass(objectClass);

         rti.resignFederationExecution(ResignAction.NO_ACTION);
         joined = false;

         JavaTckSupport.expectFailure(
            () -> rti.publishObjectClassAttributes(objectClass, attributes),
            "FederateNotExecutionMember",
            "publishing valid object attributes after resignation");
         JavaTckSupport.expectFailure(
            () -> rti.unpublishObjectClass(objectClass),
            "FederateNotExecutionMember",
            "unpublishing a valid object class after resignation");
         JavaTckSupport.expectFailure(
            () -> rti.unpublishObjectClassAttributes(objectClass, attributes),
            "FederateNotExecutionMember",
            "unpublishing valid object attributes after resignation");
         JavaTckSupport.expectFailure(
            () -> rti.subscribeObjectClassAttributes(objectClass, attributes),
            "FederateNotExecutionMember",
            "subscribing to valid object attributes after resignation");
         JavaTckSupport.expectFailure(
            () -> rti.unsubscribeObjectClass(objectClass),
            "FederateNotExecutionMember",
            "unsubscribing a valid object class after resignation");
         JavaTckSupport.expectFailure(
            () -> rti.unsubscribeObjectClassAttributes(objectClass, attributes),
            "FederateNotExecutionMember",
            "unsubscribing valid object attributes after resignation");

         rti.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (joined) {
            try { rti.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { rti.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) {
            try { rti.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) {
                  return "Java TCK object-attribute declaration callback sink";
               }
               if ("hashCode".equals(method.getName())) {
                  return System.identityHashCode(proxy);
               }
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
