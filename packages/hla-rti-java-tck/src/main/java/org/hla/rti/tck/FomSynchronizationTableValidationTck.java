package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.exceptions.InconsistentFOM;
import java.util.UUID;

/** Checks selected FOM synchronization-table label and Capability rules. */
final class FomSynchronizationTableValidationTck {
   private FomSynchronizationTableValidationTck() { }

   static void run(RtiFactory factory, String validFom, String invalidCapabilityFom,
         String invalidLabelFom, String timeImplementation, CallbackModel callbackModel)
         throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      String federation = "java-tck-synchronization-table-" + UUID.randomUUID();
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

         requireRejected(creator, federation, invalidCapabilityFom, timeImplementation,
            "a FOM synchronization-point Capability field other than NA");
         requireRejected(creator, federation, invalidLabelFom, timeImplementation,
            "a user synchronization-point label using the reserved HLA prefix");

         creator.createFederationExecution(
            federation, new String[] {validFom}, timeImplementation);
         created = true;
         creator.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (created) try {
            creator.destroyFederationExecution(federation);
         } catch (Exception ignored) { }
         if (connected) try { creator.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static void requireRejected(RTIambassador creator, String federation,
         String invalidFom, String timeImplementation, String description) throws Exception {
      boolean created = false;
      boolean rejectedAsInconsistentFom = false;
      try {
         creator.createFederationExecution(
            federation, new String[] {invalidFom}, timeImplementation);
         created = true;
      } catch (InconsistentFOM expected) {
         rejectedAsInconsistentFom = true;
      }
      if (created) {
         creator.destroyFederationExecution(federation);
      }
      JavaTckSupport.check(rejectedAsInconsistentFom,
         description + " was not rejected as InconsistentFOM");
   }
}
