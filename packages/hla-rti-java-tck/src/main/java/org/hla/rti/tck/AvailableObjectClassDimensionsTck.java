package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.util.UUID;

/** Checks the exact DDM dimension set available for an object class and the empty root result. */
final class AvailableObjectClassDimensionsTck {
   private AvailableObjectClassDimensionsTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String firstDimensionName, String secondDimensionName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-available-object-class-dimensions-" + UUID.randomUUID();
      String ownerName = "java-tck-available-dimensions-owner-" + UUID.randomUUID();
      String memberName = "java-tck-available-dimensions-member-" + UUID.randomUUID();
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

         ObjectClassHandle objectClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle rootClass = owner.getObjectClassHandle("HLAobjectRoot");
         DimensionHandle firstDimension = owner.getDimensionHandle(firstDimensionName);
         DimensionHandle secondDimension = owner.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(objectClass != null && rootClass != null
               && firstDimension != null && secondDimension != null
               && !firstDimension.equals(secondDimension),
            "adapter DDM FOM did not provide the selected object class and two distinct dimensions");

         DimensionHandleSet objectDimensions =
            owner.getAvailableDimensionsForObjectClass(objectClass);
         JavaTckSupport.check(objectDimensions != null && objectDimensions.size() == 2
               && objectDimensions.contains(firstDimension)
               && objectDimensions.contains(secondDimension),
            "Get Available Dimensions For Class did not return the exact two DDM dimensions");
         DimensionHandleSet rootDimensions =
            owner.getAvailableDimensionsForObjectClass(rootClass);
         JavaTckSupport.check(rootDimensions != null && rootDimensions.isEmpty(),
            "Get Available Dimensions For Class did not return an empty set for HLAobjectRoot");
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
