package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Checks Create failure for an adapter-declared unavailable time factory. */
final class FomUnavailableLogicalTimeFactoryTck {
   private FomUnavailableLogicalTimeFactoryTck() { }

   static void run(RtiFactory factory, String unconstrainedFom,
         String unsupportedTimeName, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(unsupportedTimeName != null
            && !unsupportedTimeName.isEmpty(),
         "adapter must declare a logical-time factory name it does not support");
      RTIambassador creator = factory.getRtiAmbassador();
      String federation = "java-tck-fom-unavailable-time-" + UUID.randomUUID();
      boolean connected = false;
      boolean[] created = {false};
      try {
         creator.connect(callbacks(), callbackModel);
         connected = true;
         JavaTckSupport.expectFailure(
            () -> {
               creator.createFederationExecution(
                  federation, unconstrainedFom, unsupportedTimeName);
               created[0] = true;
            },
            "CouldNotCreateLogicalTimeFactory",
            "Create with an adapter-declared unavailable logical-time factory");
         creator.createFederationExecution(federation, unconstrainedFom);
         created[0] = true;
         creator.destroyFederationExecution(federation);
         created[0] = false;
      } finally {
         if (created[0]) try { creator.destroyFederationExecution(federation); }
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
               if ("toString".equals(method.getName())) return "Java TCK unavailable time callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
