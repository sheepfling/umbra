package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Array cardinality and predefined encoding checks through the standard API. */
final class FomArrayCardinalityTck {
   private FomArrayCardinalityTck() { }

   static void run(RtiFactory factory, String valid, String[] invalid,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      String federation = "java-tck-array-cardinality-" + UUID.randomUUID();
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
         for (String module : invalid) {
            JavaTckSupport.expectFailure(
               () -> creator.createFederationExecution(federation,
                  new String[] {module}, timeImplementation),
               "InconsistentFOM", "invalid array cardinality or predefined encoding");
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
               if ("toString".equals(method.getName())) return "Java TCK array callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
