package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.util.UUID;

/** Checks the exact DDM dimension set available for an interaction class and the empty root result. */
final class AvailableInteractionClassDimensionsTck {
   private AvailableInteractionClassDimensionsTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String firstDimensionName, String secondDimensionName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-available-interaction-class-dimensions-" + UUID.randomUUID();
      String ownerName = "java-tck-available-interaction-dimensions-owner-" + UUID.randomUUID();
      String memberName = "java-tck-available-interaction-dimensions-member-" + UUID.randomUUID();
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
         InteractionClassHandle rootClass = owner.getInteractionClassHandle("HLAinteractionRoot");
         DimensionHandle firstDimension = owner.getDimensionHandle(firstDimensionName);
         DimensionHandle secondDimension = owner.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(interactionClass != null && rootClass != null
               && firstDimension != null && secondDimension != null
               && !firstDimension.equals(secondDimension),
            "adapter DDM FOM did not provide the selected interaction class and two distinct dimensions");

         DimensionHandleSet interactionDimensions =
            owner.getAvailableDimensionsForInteractionClass(interactionClass);
         JavaTckSupport.check(interactionDimensions != null && interactionDimensions.size() == 2
               && interactionDimensions.contains(firstDimension)
               && interactionDimensions.contains(secondDimension),
            "Get Available Dimensions For Class did not return the exact two interaction dimensions");
         DimensionHandleSet rootDimensions =
            owner.getAvailableDimensionsForInteractionClass(rootClass);
         JavaTckSupport.check(rootDimensions != null && rootDimensions.isEmpty(),
            "Get Available Dimensions For Class did not return an empty set for HLAinteractionRoot");
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
