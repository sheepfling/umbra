package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Annex C.5 synchronization-point duplicate cases through the standard API. */
final class FomSynchronizationDuplicateTck {
   private FomSynchronizationDuplicateTck() { }

   static void run(RtiFactory factory, String base, String second,
         String timeImplementation, CallbackModel callbackModel,
         boolean noteConflict) throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      String federation = "java-tck-synchronization-duplicate-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      try {
         creator.connect(callbacks(), callbackModel);
         connected = true;
         for (String module : new String[] {base, second}) {
            creator.createFederationExecution(federation,
               new String[] {module}, timeImplementation);
            created = true;
            creator.destroyFederationExecution(federation);
            created = false;
         }
         if (noteConflict) {
            JavaTckSupport.expectFailure(
               () -> creator.createFederationExecution(federation,
                  new String[] {base, second}, timeImplementation),
               "InconsistentFOM", "different note references on one synchronization point");
            creator.createFederationExecution(federation,
               new String[] {base}, timeImplementation);
         } else {
            creator.createFederationExecution(federation,
               new String[] {base, second}, timeImplementation);
         }
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
               if ("toString".equals(method.getName())) return "Java TCK synchronization callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
