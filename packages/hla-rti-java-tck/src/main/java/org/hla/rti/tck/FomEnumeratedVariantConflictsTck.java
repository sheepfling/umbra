package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Two independent Annex C.3 rejection shapes through the official Java API. */
final class FomEnumeratedVariantConflictsTck {
   private FomEnumeratedVariantConflictsTck() { }

   static void run(RtiFactory factory, String first, String second,
         String timeImplementation, CallbackModel callbackModel,
         boolean nonextendableVariant) throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      String federation = "java-tck-enumerated-variant-conflict-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      try {
         creator.connect(callbacks(), callbackModel);
         connected = true;
         for (String module : new String[] {first, second}) {
            creator.createFederationExecution(federation,
               new String[] {module}, timeImplementation);
            created = true;
            creator.destroyFederationExecution(federation);
            created = false;
         }
         JavaTckSupport.expectFailure(
            () -> creator.createFederationExecution(federation,
               new String[] {first, second}, timeImplementation),
            "InconsistentFOM", nonextendableVariant
               ? "different alternatives on one named HLAvariantRecord"
               : "one enumerated value assigned to different names");
         creator.createFederationExecution(federation,
            new String[] {first}, timeImplementation);
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
               if ("toString".equals(method.getName())) return "Java TCK FOM conflict callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
