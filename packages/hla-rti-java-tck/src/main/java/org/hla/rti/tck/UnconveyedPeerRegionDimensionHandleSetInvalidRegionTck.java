package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.util.UUID;

/** Checks that a peer cannot look up a region's dimensions without its designator. */
final class UnconveyedPeerRegionDimensionHandleSetInvalidRegionTck {
   private UnconveyedPeerRegionDimensionHandleSetInvalidRegionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String dimensionName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-unconveyed-peer-region-dimensions-" + UUID.randomUUID();
      String ownerName = "java-tck-unconveyed-region-owner-" + UUID.randomUUID();
      String memberName = "java-tck-unconveyed-region-member-" + UUID.randomUUID();
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

         DimensionHandle dimension = owner.getDimensionHandle(dimensionName);
         JavaTckSupport.check(dimension != null && dimension.encodedLength() > 0,
            "DDM fixture returned an invalid region dimension");
         DimensionHandleSet dimensions = owner.getDimensionHandleSetFactory().create();
         dimensions.add(dimension);
         ownerRegion = owner.createRegion(dimensions);
         JavaTckSupport.check(ownerRegion != null && ownerRegion.encodedLength() > 0,
            "Owner could not create the DDM region");

         RegionHandle peerRegion = ownerRegion;
         JavaTckSupport.expectFailure(() -> member.getDimensionHandleSet(peerRegion),
            "InvalidRegion",
            "Get Dimension Handle Set for a peer-owned region not conveyed to the member");
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
