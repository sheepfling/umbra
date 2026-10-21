package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Annex C.3 module merging through only the standard Java RTI interface. */
final class FomDataTypeMergeTck {
   private FomDataTypeMergeTck() { }

   static void forwardReference(RtiFactory factory, String consumer, String provider,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      String federation = "java-tck-data-type-forward-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      try {
         creator.connect(callbacks(), callbackModel);
         connected = true;
         JavaTckSupport.expectFailure(
            () -> creator.createFederationExecution(
               federation, new String[] {consumer}, timeImplementation),
            "InconsistentFOM", "unresolved array element data type");
         creator.createFederationExecution(
            federation, new String[] {consumer, provider}, timeImplementation);
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

   static void conflictingDefinition(RtiFactory factory, String consumer,
         String provider, String conflictingProvider,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      String federation = "java-tck-data-type-conflict-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      try {
         creator.connect(callbacks(), callbackModel);
         connected = true;
         creator.createFederationExecution(
            federation, new String[] {consumer, provider}, timeImplementation);
         created = true;
         creator.destroyFederationExecution(federation);
         created = false;
         creator.createFederationExecution(
            federation, new String[] {consumer, conflictingProvider}, timeImplementation);
         created = true;
         creator.destroyFederationExecution(federation);
         created = false;

         JavaTckSupport.expectFailure(
            () -> creator.createFederationExecution(federation,
               new String[] {consumer, provider, conflictingProvider},
               timeImplementation), "InconsistentFOM",
            "conflicting definitions of one named data type");
         creator.createFederationExecution(
            federation, new String[] {consumer, provider}, timeImplementation);
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
               if ("toString".equals(method.getName())) return "Java TCK FOM merge callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
