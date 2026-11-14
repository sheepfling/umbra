package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.util.UUID;

/** Checks that public Create accepts one conforming Model Identification Name. */
final class FomModelIdentificationNameValidTck {
   private FomModelIdentificationNameValidTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      String federation = "java-tck-model-identification-name-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      try {
         creator.connect(new NullFederateAmbassador(), callbackModel);
         connected = true;
         creator.createFederationExecution(
            federation, new String[] {fom}, timeImplementation);
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
}
