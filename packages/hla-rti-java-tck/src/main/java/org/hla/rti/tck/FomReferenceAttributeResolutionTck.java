package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Reference-data attribute and representation rules through the standard API. */
final class FomReferenceAttributeResolutionTck {
   private FomReferenceAttributeResolutionTck() { }

   static void run(RtiFactory factory, String consumer, String provider,
         String missing, String mismatched, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      String federation = "java-tck-reference-attribute-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         creator.connect(callbacks(), callbackModel);
         connected = true;
         creator.createFederationExecution(federation,
            new String[] {consumer, provider}, timeImplementation);
         created = true;
         creator.joinFederationExecution("java-tck-reference-attribute-member", federation);
         joined = true;
         ObjectClassHandle objectClass = creator.getObjectClassHandle(
            "HLAobjectRoot.TckReferenceAttributeBase.TckReferenceAttributeChild");
         AttributeHandle identifier = creator.getAttributeHandle(objectClass, "Identifier");
         JavaTckSupport.check("Identifier".equals(
               creator.getAttributeName(objectClass, identifier)),
            "later FOM module did not supply the inherited referenced attribute");
         creator.resignFederationExecution(ResignAction.NO_ACTION);
         joined = false;
         creator.destroyFederationExecution(federation);
         created = false;

         for (String module : new String[] {missing, mismatched}) {
            JavaTckSupport.expectFailure(
               () -> creator.createFederationExecution(federation,
                  new String[] {module, provider}, timeImplementation),
               "InconsistentFOM", "unresolved or mismatched reference attribute");
            creator.createFederationExecution(federation,
               new String[] {consumer, provider}, timeImplementation);
            created = true;
            creator.destroyFederationExecution(federation);
            created = false;
         }
      } finally {
         if (joined) try { creator.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (created) try { creator.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (connected) try { creator.disconnect(); }
            catch (Exception ignored) { }
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK reference-attribute callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
