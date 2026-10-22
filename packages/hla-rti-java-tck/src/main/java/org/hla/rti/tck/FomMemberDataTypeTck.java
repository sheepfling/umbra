package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Attribute and parameter data-type categories through the standard RTI API. */
final class FomMemberDataTypeTck {
   private FomMemberDataTypeTck() { }

   static void run(RtiFactory factory, String valid, String attributeBasic,
         String parameterBasic, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      String federation = "java-tck-member-data-type-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      try {
         creator.connect(callbacks(), callbackModel);
         connected = true;
         creator.createFederationExecution(federation,
            new String[] {valid}, timeImplementation);
         created = true;
         creator.destroyFederationExecution(federation);
         created = false;
         for (String module : new String[] {attributeBasic, parameterBasic}) {
            JavaTckSupport.expectFailure(
               () -> creator.createFederationExecution(federation,
                  new String[] {module}, timeImplementation),
               "InconsistentFOM", "raw basic attribute or parameter data type");
            creator.createFederationExecution(federation,
               new String[] {valid}, timeImplementation);
            created = true;
            creator.destroyFederationExecution(federation);
            created = false;
         }
      } finally {
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
               if ("toString".equals(method.getName())) return "Java TCK member callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
