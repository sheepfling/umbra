package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.exceptions.InconsistentFOM;
import java.util.UUID;

/** Checks Create rejects an array whose Element Type reference is unresolved. */
final class FomArrayElementTypeReferenceTck {
   private FomArrayElementTypeReferenceTck() { }

   static void run(RtiFactory factory, String validFom, String invalidFom,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      String federation = "java-tck-array-element-type-reference-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean rejectedAsInconsistentFom = false;
      try {
         creator.connect(new NullFederateAmbassador(), callbackModel);
         connected = true;
         creator.createFederationExecution(
            federation, new String[] {validFom}, timeImplementation);
         created = true;
         creator.destroyFederationExecution(federation);
         created = false;

         try {
            creator.createFederationExecution(
               federation, new String[] {invalidFom}, timeImplementation);
            created = true;
         } catch (InconsistentFOM expected) {
            rejectedAsInconsistentFom = true;
         }
         if (created) {
            creator.destroyFederationExecution(federation);
            created = false;
         }
         JavaTckSupport.check(rejectedAsInconsistentFom,
            "an unresolved array Element Type was not rejected as InconsistentFOM");
      } finally {
         if (created) try {
            creator.destroyFederationExecution(federation);
         } catch (Exception ignored) { }
         if (connected) try { creator.disconnect(); } catch (Exception ignored) { }
      }
   }
}
