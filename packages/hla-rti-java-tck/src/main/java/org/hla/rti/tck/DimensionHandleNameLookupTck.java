package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.util.UUID;

/** Checks dimension-name lookup, handle-to-name round-trip, and NameNotFound. */
final class DimensionHandleNameLookupTck {
   private DimensionHandleNameLookupTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String dimensionName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-dimension-handle-name-" + UUID.randomUUID();
      String ownerName = "java-tck-dimension-name-owner-" + UUID.randomUUID();
      String memberName = "java-tck-dimension-name-member-" + UUID.randomUUID();
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

         DimensionHandle dimension = owner.getDimensionHandle(dimensionName);
         JavaTckSupport.check(dimension != null && dimension.encodedLength() > 0,
            "Get Dimension Handle returned an invalid handle for a defined dimension");
         JavaTckSupport.check(dimensionName.equals(owner.getDimensionName(dimension)),
            "Get Dimension Name did not return the name associated with the supplied handle");
         JavaTckSupport.expectFailure(
            () -> owner.getDimensionHandle("CppTckDimensionThatIsNotDefined"),
            "NameNotFound", "looking up an undefined FDD dimension name");
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
