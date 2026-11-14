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

/** Checks that a peer cannot query bounds for a region whose designator was not conveyed. */
final class UnconveyedPeerRegionRangeBoundsInvalidRegionTck {
   private UnconveyedPeerRegionRangeBoundsInvalidRegionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String dimensionName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-unconveyed-peer-range-bounds-" + UUID.randomUUID();
      String ownerName = "java-tck-unconveyed-range-owner-" + UUID.randomUUID();
      String memberName = "java-tck-unconveyed-range-member-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean memberConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean memberJoined = false;
      RegionHandle ownerRegion = null;
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

         DimensionHandle ownerDimension = owner.getDimensionHandle(dimensionName);
         DimensionHandle memberDimension = member.getDimensionHandle(dimensionName);
         JavaTckSupport.check(ownerDimension != null && ownerDimension.encodedLength() > 0
               && memberDimension != null && memberDimension.encodedLength() > 0,
            "DDM fixture returned an invalid range dimension");
         DimensionHandleSet dimensions = owner.getDimensionHandleSetFactory().create();
         dimensions.add(ownerDimension);
         ownerRegion = owner.createRegion(dimensions);
         JavaTckSupport.check(ownerRegion != null && ownerRegion.encodedLength() > 0,
            "Owner could not create the DDM range region");

         owner.setRangeBounds(ownerRegion, ownerDimension, new RangeBounds(2, 8));
         RegionHandleSet regions = owner.getRegionHandleSetFactory().create();
         regions.add(ownerRegion);
         owner.commitRegionModifications(regions);

         RegionHandle peerRegion = ownerRegion;
         JavaTckSupport.expectFailure(() -> member.getRangeBounds(peerRegion, memberDimension),
            "InvalidRegion",
            "Get Range Bounds for a peer-created region not conveyed to the member");
      } finally {
         if (ownerJoined && ownerRegion != null) try {
            owner.deleteRegion(ownerRegion);
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
