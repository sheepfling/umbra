package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.util.UUID;

/** Checks the exact dimension sets reported for one- and two-dimensional regions. */
final class DimensionHandleSetForRegionTck {
   private DimensionHandleSetForRegionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String firstDimensionName, String secondDimensionName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-dimension-set-region-" + UUID.randomUUID();
      String ownerName = "java-tck-dimension-set-owner-" + UUID.randomUUID();
      String memberName = "java-tck-dimension-set-member-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean memberConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean memberJoined = false;
      RegionHandle oneDimensionRegion = null;
      RegionHandle twoDimensionRegion = null;
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

         DimensionHandle firstDimension = owner.getDimensionHandle(firstDimensionName);
         DimensionHandle secondDimension = owner.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(firstDimension != null && secondDimension != null
               && !firstDimension.equals(secondDimension),
            "DDM fixture did not provide two distinct dimension handles");

         DimensionHandleSet oneDimension = owner.getDimensionHandleSetFactory().create();
         oneDimension.add(firstDimension);
         oneDimensionRegion = owner.createRegion(oneDimension);
         DimensionHandleSet twoDimensions = owner.getDimensionHandleSetFactory().create();
         twoDimensions.add(firstDimension);
         twoDimensions.add(secondDimension);
         twoDimensionRegion = owner.createRegion(twoDimensions);
         JavaTckSupport.check(oneDimensionRegion != null && twoDimensionRegion != null,
            "DDM fixture region creation returned a null handle");

         DimensionHandleSet oneDimensionResult = owner.getDimensionHandleSet(oneDimensionRegion);
         JavaTckSupport.check(oneDimensionResult != null && oneDimensionResult.size() == 1
               && oneDimensionResult.contains(firstDimension),
            "Get Dimension Handle Set did not return the single dimension of its region");
         DimensionHandleSet twoDimensionResult = owner.getDimensionHandleSet(twoDimensionRegion);
         JavaTckSupport.check(twoDimensionResult != null && twoDimensionResult.size() == 2
               && twoDimensionResult.contains(firstDimension)
               && twoDimensionResult.contains(secondDimension),
            "Get Dimension Handle Set did not return the exact dimensions of its region");
      } finally {
         if (ownerJoined && twoDimensionRegion != null) try {
            owner.deleteRegion(twoDimensionRegion);
         } catch (Exception ignored) { }
         if (ownerJoined && oneDimensionRegion != null) try {
            owner.deleteRegion(oneDimensionRegion);
         } catch (Exception ignored) { }
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
