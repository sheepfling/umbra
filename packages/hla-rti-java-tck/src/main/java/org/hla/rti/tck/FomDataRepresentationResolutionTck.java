package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** FOM representation and special instance-identifier rules through the standard API. */
final class FomDataRepresentationResolutionTck {
   private FomDataRepresentationResolutionTck() { }

   static void run(RtiFactory factory, String special, String missingSimple,
         String missingEnumerated, String invalidSpecial, String invalidBasic,
         String attributeProvider, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      String federation = "java-tck-data-representation-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         creator.connect(callbacks(), callbackModel);
         connected = true;
         creator.createFederationExecution(federation,
            new String[] {special}, timeImplementation);
         created = true;
         creator.joinFederationExecution("java-tck-representation-member", federation);
         joined = true;
         creator.resignFederationExecution(ResignAction.NO_ACTION);
         joined = false;
         creator.destroyFederationExecution(federation);
         created = false;

         for (String invalid : new String[] {
               missingSimple, missingEnumerated, invalidSpecial}) {
            JavaTckSupport.expectFailure(
               () -> creator.createFederationExecution(federation,
                  new String[] {invalid}, timeImplementation),
               "InconsistentFOM", "invalid data representation");
            creator.createFederationExecution(federation,
               new String[] {special}, timeImplementation);
            created = true;
            creator.destroyFederationExecution(federation);
            created = false;
         }
         JavaTckSupport.expectFailure(
            () -> creator.createFederationExecution(federation,
               new String[] {invalidBasic, attributeProvider}, timeImplementation),
            "InconsistentFOM", "invalid ordinary reference representation");
         creator.createFederationExecution(federation,
            new String[] {special}, timeImplementation);
         created = true;
         creator.destroyFederationExecution(federation);
         created = false;
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
               if ("toString".equals(method.getName())) return "Java TCK representation callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
