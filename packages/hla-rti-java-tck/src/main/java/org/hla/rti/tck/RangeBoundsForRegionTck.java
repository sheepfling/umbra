package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.RangeBounds;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.util.UUID;

/** Checks the current range bounds returned for a configured region dimension. */
final class RangeBoundsForRegionTck {
   private RangeBoundsForRegionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String dimensionName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-range-bounds-for-region-" + UUID.randomUUID();
      String ownerName = "java-tck-range-bounds-owner-" + UUID.randomUUID();
      String memberName = "java-tck-range-bounds-member-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean memberConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean memberJoined = false;
      RegionHandle region = null;
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

         DimensionHandle dimension = owner.getDimensionHandle(dimensionName);
         JavaTckSupport.check(dimension != null && dimension.encodedLength() > 0,
            "DDM fixture returned an invalid range dimension");
         DimensionHandleSet dimensions = owner.getDimensionHandleSetFactory().create();
         dimensions.add(dimension);
         region = owner.createRegion(dimensions);
         JavaTckSupport.check(region != null && region.encodedLength() > 0,
            "DDM fixture range region creation failed");

         long lowerBound = 3;
         long upperBound = 9;
         owner.setRangeBounds(region, dimension, new RangeBounds(lowerBound, upperBound));
         RegionHandleSet regions = owner.getRegionHandleSetFactory().create();
         regions.add(region);
         owner.commitRegionModifications(regions);

         RangeBounds bounds = owner.getRangeBounds(region, dimension);
         JavaTckSupport.check(bounds != null && bounds.lower == lowerBound
               && bounds.upper == upperBound,
            "Get Range Bounds did not return the current bounds for the specified region dimension");
      } finally {
         if (ownerJoined && region != null) try {
            owner.deleteRegion(region);
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
