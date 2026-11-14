package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.exceptions.InvalidFOM;
import java.util.UUID;

/** Checks all defined Automatic Resign Action FOM settings through the public API. */
final class FomAutomaticResignActionSettingTck {
   private FomAutomaticResignActionSettingTck() { }

   static void run(RtiFactory factory, String invalidFom, String unconditionallyDivestFom,
         String deleteObjectsFom, String cancelPendingFom, String deleteThenDivestFom,
         String cancelThenDeleteThenDivestFom, String noActionFom, String mim,
         String timeImplementation, CallbackModel callbackModel, String federateType)
         throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-resign-action-setting-" + UUID.randomUUID();
      String memberName = "java-tck-resign-action-member-" + UUID.randomUUID();
      String[] validFoms = {
         unconditionallyDivestFom,
         deleteObjectsFom,
         cancelPendingFom,
         deleteThenDivestFom,
         cancelThenDeleteThenDivestFom,
         noActionFom
      };
      ResignAction[] expectedActions = {
         ResignAction.UNCONDITIONALLY_DIVEST_ATTRIBUTES,
         ResignAction.DELETE_OBJECTS,
         ResignAction.CANCEL_PENDING_OWNERSHIP_ACQUISITIONS,
         ResignAction.DELETE_OBJECTS_THEN_DIVEST,
         ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST,
         ResignAction.NO_ACTION
      };
      boolean creatorConnected = false;
      boolean memberConnected = false;
      boolean federationCreated = false;
      boolean memberJoined = false;
      try {
         creator.connect(new NullFederateAmbassador(), callbackModel);
         creatorConnected = true;
         member.connect(new NullFederateAmbassador(), callbackModel);
         memberConnected = true;

         try {
            creator.createFederationExecutionWithMIM(
               federation, new String[] {invalidFom}, mim, timeImplementation);
            federationCreated = true;
            throw new AssertionError("unsupported Automatic Resign Action value was accepted");
         } catch (InvalidFOM expected) {
            // The invalid Create must leave the federation name available for the valid cases.
         }

         for (int index = 0; index < validFoms.length; ++index) {
            creator.createFederationExecutionWithMIM(
               federation, new String[] {validFoms[index]}, mim, timeImplementation);
            federationCreated = true;
            member.joinFederationExecution(memberName, federateType, federation);
            memberJoined = true;
            JavaTckSupport.check(expectedActions[index] == member.getAutomaticResignDirective(),
               "public Automatic Resign Action did not match its FOM switches-table setting");
            member.resignFederationExecution(ResignAction.NO_ACTION);
            memberJoined = false;
            creator.destroyFederationExecution(federation);
            federationCreated = false;
         }
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
}
