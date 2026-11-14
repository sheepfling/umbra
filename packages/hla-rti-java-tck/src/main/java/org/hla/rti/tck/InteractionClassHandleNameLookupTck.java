package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.util.UUID;

/** Checks a valid interaction-class handle and its exact FOM name round-trip. */
final class InteractionClassHandleNameLookupTck {
   private InteractionClassHandleNameLookupTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-interaction-class-handle-lookup-" + UUID.randomUUID();
      String ownerName = "java-tck-interaction-class-handle-owner-" + UUID.randomUUID();
      String memberName = "java-tck-interaction-class-handle-member-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean memberConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean memberJoined = false;
      try {
         owner.connect(new NullFederateAmbassador(), callbackModel);
         ownerConnected = true;
         member.connect(new NullFederateAmbassador(), callbackModel);
         memberConnected = true;
         owner.createFederationExecution(federation, new String[] {fom}, timeImplementation);
         created = true;
         owner.joinFederationExecution(ownerName, "java-tck", federation);
         ownerJoined = true;
         member.joinFederationExecution(memberName, "java-tck", federation);
         memberJoined = true;

         InteractionClassHandle interactionClass =
            owner.getInteractionClassHandle(interactionClassName);
         JavaTckSupport.check(interactionClass != null && interactionClass.encodedLength() > 0,
            "Interaction class handle lookup returned an invalid handle");
         JavaTckSupport.check(interactionClassName.equals(
               owner.getInteractionClassName(interactionClass)),
            "Get Interaction Class Name did not return the supplied interaction class name");
      } finally {
         if (memberJoined) try {
            member.resignFederationExecution(ResignAction.NO_ACTION);
         } catch (Exception ignored) { }
         if (ownerJoined) try {
            owner.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         } catch (Exception ignored) { }
         if (created) try {
            owner.destroyFederationExecution(federation);
         } catch (Exception ignored) { }
         if (memberConnected) try { member.disconnect(); } catch (Exception ignored) { }
         if (ownerConnected) try { owner.disconnect(); } catch (Exception ignored) { }
      }
   }
}
