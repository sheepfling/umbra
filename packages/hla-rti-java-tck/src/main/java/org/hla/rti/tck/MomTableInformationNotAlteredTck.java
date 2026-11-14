package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Verifies that a FOM cannot alter a standard HLAfederate attribute definition. */
final class MomTableInformationNotAlteredTck {
   private MomTableInformationNotAlteredTck() { }

   static void run(RtiFactory factory, String fom, String conflictFom, String mim,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      String federation =
         "java-tck-mom-table-information-not-altered-" + UUID.randomUUID();
      boolean connected = false;
      boolean[] federationMayExist = {false};
      try {
         creator.connect(callbacks(), callbackModel);
         connected = true;
         JavaTckSupport.expectFailure(() -> {
            federationMayExist[0] = true;
            creator.createFederationExecutionWithMIM(
               federation, new String[] {conflictFom}, mim, timeImplementation);
         }, "InconsistentFOM",
            "changing HLAfederate.HLAfederateName transportation from HLAreliable");
         federationMayExist[0] = false;

         federationMayExist[0] = true;
         creator.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         creator.destroyFederationExecution(federation);
         federationMayExist[0] = false;
      } finally {
         if (connected && federationMayExist[0]) {
            try { creator.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) {
            try { creator.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK MOM integrity callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
