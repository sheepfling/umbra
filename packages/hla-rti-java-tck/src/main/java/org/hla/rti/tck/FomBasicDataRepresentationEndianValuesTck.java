package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.util.UUID;

/** Accepts the two declared basic-data Endian values through federation create and join. */
final class FomBasicDataRepresentationEndianValuesTck {
   private FomBasicDataRepresentationEndianValuesTck() { }

   static void run(RtiFactory factory, String bigEndianFom, String littleEndianFom,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(timeImplementation != null && !timeImplementation.isEmpty(),
         "basic-data Endian acceptance requires an adapter logical-time name");
      JavaTckSupport.check(bigEndianFom != null && !bigEndianFom.isEmpty(),
         "adapter did not supply the Big Endian basic-data FOM");
      JavaTckSupport.check(littleEndianFom != null && !littleEndianFom.isEmpty(),
         "adapter did not supply the Little Endian basic-data FOM");

      RTIambassador rti = factory.getRtiAmbassador();
      boolean connected = false;
      try {
         rti.connect(new NullFederateAmbassador(), callbackModel);
         connected = true;
         createJoinResignAndDestroy(rti, bigEndianFom, "big", timeImplementation);
         createJoinResignAndDestroy(rti, littleEndianFom, "little", timeImplementation);
      } finally {
         if (connected) {
            try { rti.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void createJoinResignAndDestroy(RTIambassador rti, String fom,
         String endian, String timeImplementation) throws Exception {
      String federation = "java-tck-basic-data-endian-" + endian + "-" + UUID.randomUUID();
      boolean created = false;
      boolean joined = false;
      try {
         rti.createFederationExecution(federation, new String[] {fom}, timeImplementation);
         created = true;
         FederateHandle handle = rti.joinFederationExecution(
            "java-tck-basic-data-endian-member", federation);
         joined = true;
         JavaTckSupport.check(handle != null,
            endian + " Endian FOM did not establish federation membership");
         rti.resignFederationExecution(ResignAction.NO_ACTION);
         joined = false;
         rti.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (joined) {
            try { rti.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { rti.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
      }
   }
}
