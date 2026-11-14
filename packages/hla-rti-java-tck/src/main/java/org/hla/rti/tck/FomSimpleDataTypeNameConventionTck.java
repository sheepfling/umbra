package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.exceptions.InconsistentFOM;
import java.util.UUID;

/** Checks selected simple data type name restrictions through the public API. */
final class FomSimpleDataTypeNameConventionTck {
   private FomSimpleDataTypeNameConventionTck() { }

   static void run(RtiFactory factory, String validFom, String[] invalidFoms,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      String[] descriptions = {
         "a simple data type name containing a period",
         "a simple data type name beginning with a mixed-case HLA prefix",
         "a simple data type name matching the mixed-case reserved NA marker"
      };
      JavaTckSupport.check(invalidFoms.length == descriptions.length,
         "the adapter must supply all three focused invalid-name FOMs");

      RTIambassador creator = factory.getRtiAmbassador();
      String federation = "java-tck-simple-data-type-name-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      try {
         creator.connect(new NullFederateAmbassador(), callbackModel);
         connected = true;
         creator.createFederationExecution(
            federation, new String[] {validFom}, timeImplementation);
         created = true;
         creator.destroyFederationExecution(federation);
         created = false;

         for (int index = 0; index < invalidFoms.length; ++index) {
            boolean rejectedAsInconsistentFom = false;
            try {
               creator.createFederationExecution(
                  federation, new String[] {invalidFoms[index]}, timeImplementation);
               created = true;
            } catch (InconsistentFOM expected) {
               rejectedAsInconsistentFom = true;
            }
            if (created) {
               creator.destroyFederationExecution(federation);
               created = false;
            }
            JavaTckSupport.check(rejectedAsInconsistentFom,
               descriptions[index] + " was not rejected as InconsistentFOM");
         }
      } finally {
         if (created) try {
            creator.destroyFederationExecution(federation);
         } catch (Exception ignored) { }
         if (connected) try { creator.disconnect(); } catch (Exception ignored) { }
      }
   }
}
