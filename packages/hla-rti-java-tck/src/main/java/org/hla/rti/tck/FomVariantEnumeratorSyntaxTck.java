package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Variant discriminant enumerator syntax through the standard Java API. */
final class FomVariantEnumeratorSyntaxTck {
   private FomVariantEnumeratorSyntaxTck() { }

   static void run(RtiFactory factory, String range, String hlaother,
         String omitted, String malformedRange, String compositeHlaother,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      String federation = "java-tck-variant-enumerator-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      try {
         creator.connect(callbacks(), callbackModel);
         connected = true;
         for (String accepted : new String[] {range, hlaother, omitted}) {
            creator.createFederationExecution(federation,
               new String[] {accepted}, timeImplementation);
            created = true;
            creator.destroyFederationExecution(federation);
            created = false;
         }
         for (String rejected : new String[] {malformedRange, compositeHlaother}) {
            JavaTckSupport.expectFailure(
               () -> creator.createFederationExecution(federation,
                  new String[] {rejected}, timeImplementation),
               "InconsistentFOM", "malformed variant discriminant enumerator");
         }
         creator.createFederationExecution(federation,
            new String[] {range}, timeImplementation);
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
               if ("toString".equals(method.getName())) return "Java TCK enumerator callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
