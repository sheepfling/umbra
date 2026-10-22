package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Merged variant ranges and non-extendable HLAother through the standard API. */
final class FomVariantRangeSemanticsTck {
   private FomVariantRangeSemanticsTck() { }

   static void run(RtiFactory factory, String base, String validExtension,
         String conflictExtension, String hlaother, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      String federation = "java-tck-variant-range-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      String[] resolved = {base, validExtension};
      try {
         creator.connect(callbacks(), callbackModel);
         connected = true;
         creator.createFederationExecution(federation, resolved, timeImplementation);
         created = true;
         creator.destroyFederationExecution(federation);
         created = false;
         creator.createFederationExecution(federation,
            new String[] {hlaother}, timeImplementation);
         created = true;
         creator.destroyFederationExecution(federation);
         created = false;
         JavaTckSupport.expectFailure(
            () -> creator.createFederationExecution(federation,
               new String[] {base, conflictExtension}, timeImplementation),
            "InconsistentFOM", "merged range overlapping a different named alternative");
         creator.createFederationExecution(federation, resolved, timeImplementation);
         created = true;
         creator.destroyFederationExecution(federation);
         created = false;
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
               if ("toString".equals(method.getName())) return "Java TCK range callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
