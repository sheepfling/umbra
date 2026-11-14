package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.util.UUID;

/** Checks transportation declarations with different Semantics compose through Create and Join. */
final class FomTransportationDescriptionIgnoredTck {
   private FomTransportationDescriptionIgnoredTck() { }

   static void run(RtiFactory factory, String baseFom, String descriptionDifferentFom,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-transportation-description-" + UUID.randomUUID();
      String memberName = "java-tck-transportation-description-member-" + UUID.randomUUID();
      String federateType = System.getProperty("hla.rti.tck.federateType", "tck-type");
      boolean creatorConnected = false;
      boolean memberConnected = false;
      boolean federationCreated = false;
      boolean memberJoined = false;
      try {
         creator.connect(new NullFederateAmbassador(), callbackModel);
         creatorConnected = true;
         member.connect(new NullFederateAmbassador(), callbackModel);
         memberConnected = true;

         creator.createFederationExecution(
            federation, new String[] {baseFom, descriptionDifferentFom}, timeImplementation);
         federationCreated = true;
         member.joinFederationExecution(memberName, federateType, federation);
         memberJoined = true;
         verifyTransportationLookup(member);
         member.resignFederationExecution(ResignAction.NO_ACTION);
         memberJoined = false;
         creator.destroyFederationExecution(federation);
         federationCreated = false;

         creator.createFederationExecution(
            federation, new String[] {baseFom}, timeImplementation);
         federationCreated = true;
         member.joinFederationExecution(memberName, federateType, federation,
            new String[] {descriptionDifferentFom});
         memberJoined = true;
         verifyTransportationLookup(member);
         member.resignFederationExecution(ResignAction.NO_ACTION);
         memberJoined = false;
         creator.destroyFederationExecution(federation);
         federationCreated = false;
      } finally {
         if (memberJoined) try {
            member.resignFederationExecution(ResignAction.NO_ACTION);
         } catch (Exception ignored) { }
         if (federationCreated) try {
            creator.destroyFederationExecution(federation);
         } catch (Exception ignored) { }
         if (memberConnected) try { member.disconnect(); } catch (Exception ignored) { }
         if (creatorConnected) try { creator.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static void verifyTransportationLookup(RTIambassador ambassador) throws Exception {
      TransportationTypeHandle handle = ambassador.getTransportationTypeHandle("TckMergeTransport");
      JavaTckSupport.check("TckMergeTransport".equals(ambassador.getTransportationTypeName(handle)),
         "FDD-equivalent transportation declaration was not exposed by public lookup");
   }
}
