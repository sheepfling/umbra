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

/** Checks that Get Range Bounds rejects a deleted region designator. */
final class DeletedRegionRangeBoundsInvalidRegionTck {
   private DeletedRegionRangeBoundsInvalidRegionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String dimensionName, CallbackModel callbackModel) throws Exception {
      RTIambassador ambassador = factory.getRtiAmbassador();
      String federation = "java-tck-deleted-region-range-bounds-" + UUID.randomUUID();
      String federateName = "java-tck-deleted-region-range-owner-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      boolean regionCreated = false;
      boolean regionDeleted = false;
      RegionHandle region = null;
      try {
         ambassador.connect(new NullFederateAmbassador(), callbackModel);
         connected = true;
         ambassador.createFederationExecution(federation, new String[] {fom}, timeImplementation);
         created = true;
         ambassador.joinFederationExecution(federateName, "java-tck", federation);
         joined = true;

         DimensionHandle dimension = ambassador.getDimensionHandle(dimensionName);
         JavaTckSupport.check(dimension != null && dimension.encodedLength() > 0,
            "DDM fixture returned an invalid range dimension");
         DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
         dimensions.add(dimension);
         region = ambassador.createRegion(dimensions);
         JavaTckSupport.check(region != null && region.encodedLength() > 0,
            "RTI returned an invalid region handle");
         regionCreated = true;

         ambassador.setRangeBounds(region, dimension, new RangeBounds(2, 8));
         RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
         regions.add(region);
         ambassador.commitRegionModifications(regions);

         ambassador.deleteRegion(region);
         regionDeleted = true;
         RegionHandle deletedRegion = region;
         JavaTckSupport.expectFailure(
            () -> ambassador.getRangeBounds(deletedRegion, dimension),
            "InvalidRegion",
            "Get Range Bounds for a deleted region designator");
      } finally {
         if (joined && regionCreated && !regionDeleted) try {
            ambassador.deleteRegion(region);
         } catch (Exception ignored) { }
         if (joined) try {
            ambassador.resignFederationExecution(ResignAction.NO_ACTION);
         } catch (Exception ignored) { }
         if (created) try {
            ambassador.destroyFederationExecution(federation);
         } catch (Exception ignored) { }
         if (connected) try { ambassador.disconnect(); } catch (Exception ignored) { }
      }
   }
}
