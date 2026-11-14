package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.RangeBounds;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.util.UUID;

/** Uses a normalized HLAobjectRoot handle as an HLAobjectClass region coordinate. */
final class NormalizeObjectClassHandleRegionCoordinateTck {
   private NormalizeObjectClassHandleRegionCoordinateTck() { }

   static void run(RtiFactory factory, String modelFom, String mim, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador rti = factory.getRtiAmbassador();
      String federation = "java-tck-normalize-object-class-region-coordinate-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      RegionHandle region = null;
      try {
         rti.connect(new NullFederateAmbassador(), callbackModel);
         connected = true;
         rti.createFederationExecutionWithMIM(federation, new String[] {modelFom}, mim,
            timeImplementation);
         created = true;
         rti.joinFederationExecution("java-tck-normalize-object-class-region-coordinate", federation);
         joined = true;

         ObjectClassHandle objectClass = rti.getObjectClassHandle("HLAobjectRoot");
         DimensionHandle dimension = rti.getDimensionHandle("HLAobjectClass");
         JavaTckSupport.check(objectClass != null && objectClass.encodedLength() > 0
               && dimension != null && dimension.encodedLength() > 0,
            "model FOM and standard MIM did not resolve HLAobjectRoot and HLAobjectClass");

         long normalized = rti.normalizeObjectClassHandle(objectClass);
         long dimensionUpperBound = rti.getDimensionUpperBound(dimension);
         JavaTckSupport.check(normalized >= 0 && normalized < dimensionUpperBound,
            "normalized HLAobjectRoot handle cannot form a point range in HLAobjectClass");

         DimensionHandleSet dimensions = rti.getDimensionHandleSetFactory().create();
         dimensions.add(dimension);
         region = rti.createRegion(dimensions);
         JavaTckSupport.check(region != null && region.encodedLength() > 0,
            "region creation for the HLAobjectClass dimension failed");

         long expectedUpperBound = normalized + 1L;
         rti.setRangeBounds(region, dimension, new RangeBounds(normalized, expectedUpperBound));
         RegionHandleSet regions = rti.getRegionHandleSetFactory().create();
         regions.add(region);
         rti.commitRegionModifications(regions);

         RangeBounds actual = rti.getRangeBounds(region, dimension);
         JavaTckSupport.check(actual != null && actual.lower == normalized
               && actual.upper == expectedUpperBound,
            "Get Range Bounds did not preserve the normalized object-class handle as the HLAobjectClass point range");
      } finally {
         if (joined && region != null) {
            try { rti.deleteRegion(region); } catch (Exception ignored) { }
         }
         if (joined) {
            try { rti.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { rti.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) {
            try { rti.disconnect(); } catch (Exception ignored) { }
         }
      }
   }
}
