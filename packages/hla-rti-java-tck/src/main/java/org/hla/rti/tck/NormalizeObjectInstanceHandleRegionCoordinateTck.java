package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RangeBounds;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.util.UUID;

/** Uses a normalized object-instance handle as an HLAobjectInstance region coordinate. */
final class NormalizeObjectInstanceHandleRegionCoordinateTck {
   private NormalizeObjectInstanceHandleRegionCoordinateTck() { }

   static void run(RtiFactory factory, String modelFom, String mim, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador rti = factory.getRtiAmbassador();
      String federation = "java-tck-normalize-object-instance-region-coordinate-" + UUID.randomUUID();
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
         rti.joinFederationExecution("java-tck-normalize-object-instance-region-coordinate", federation);
         joined = true;

         ObjectClassHandle objectClass = rti.getObjectClassHandle(objectClassName);
         AttributeHandle attribute = rti.getAttributeHandle(objectClass, attributeName);
         JavaTckSupport.check(objectClass != null && objectClass.encodedLength() > 0
               && attribute != null && attribute.encodedLength() > 0,
            "model FOM did not resolve the configured object class and attribute");
         AttributeHandleSet attributes = JavaTckSupport.attributeSet(rti, attribute);
         rti.publishObjectClassAttributes(objectClass, attributes);
         ObjectInstanceHandle object = rti.registerObjectInstance(objectClass);
         DimensionHandle dimension = rti.getDimensionHandle("HLAobjectInstance");
         JavaTckSupport.check(object != null && object.encodedLength() > 0
               && dimension != null && dimension.encodedLength() > 0,
            "object registration or standard HLAobjectInstance dimension lookup failed");

         long normalized = rti.normalizeObjectInstanceHandle(object);
         long dimensionUpperBound = rti.getDimensionUpperBound(dimension);
         JavaTckSupport.check(normalized >= 0 && normalized < dimensionUpperBound,
            "normalized object-instance handle cannot form a point range in HLAobjectInstance");

         DimensionHandleSet dimensions = rti.getDimensionHandleSetFactory().create();
         dimensions.add(dimension);
         region = rti.createRegion(dimensions);
         JavaTckSupport.check(region != null && region.encodedLength() > 0,
            "region creation for the HLAobjectInstance dimension failed");

         long expectedUpperBound = normalized + 1L;
         rti.setRangeBounds(region, dimension, new RangeBounds(normalized, expectedUpperBound));
         RegionHandleSet regions = rti.getRegionHandleSetFactory().create();
         regions.add(region);
         rti.commitRegionModifications(regions);

         RangeBounds actual = rti.getRangeBounds(region, dimension);
         JavaTckSupport.check(actual != null && actual.lower == normalized
               && actual.upper == expectedUpperBound,
            "Get Range Bounds did not preserve the normalized object-instance handle as the HLAobjectInstance point range");
      } finally {
         if (joined && region != null) {
            try { rti.deleteRegion(region); } catch (Exception ignored) { }
         }
         if (joined) {
            try { rti.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
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
