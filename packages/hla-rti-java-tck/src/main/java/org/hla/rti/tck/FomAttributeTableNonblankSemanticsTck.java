package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.exceptions.InconsistentFOM;
import java.util.UUID;

/** Checks Create Federation Execution rejects an empty attribute Semantics field. */
final class FomAttributeTableNonblankSemanticsTck {
   private FomAttributeTableNonblankSemanticsTck() { }

   static void run(RtiFactory factory, String invalidFom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      String federation = "java-tck-empty-attribute-semantics-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean rejectedAsInconsistentFom = false;
      try {
         creator.connect(new NullFederateAmbassador(), callbackModel);
         connected = true;
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
            "an attribute row with an empty Semantics field was not rejected as InconsistentFOM");
      } finally {
         if (created) try {
            creator.destroyFederationExecution(federation);
         } catch (Exception ignored) { }
         if (connected) try { creator.disconnect(); } catch (Exception ignored) { }
      }
   }
}
