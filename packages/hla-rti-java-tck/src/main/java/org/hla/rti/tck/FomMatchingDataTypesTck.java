package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Matching named data-type declarations through the standard Java RTI API. */
final class FomMatchingDataTypesTck {
   private FomMatchingDataTypesTck() { }

   static void run(RtiFactory factory, String first, String second,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      String federation = "java-tck-matching-data-types-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      try {
         creator.connect(callbacks(), callbackModel);
         connected = true;
         for (String[] modules : new String[][] {
               {first}, {second}, {first, second}}) {
            creator.createFederationExecution(federation, modules, timeImplementation);
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
               if ("toString".equals(method.getName())) return "Java TCK matching types callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
