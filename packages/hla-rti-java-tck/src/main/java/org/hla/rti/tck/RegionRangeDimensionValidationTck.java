package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.RangeBounds;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Rejects range queries and changes for dimensions absent from a region. */
final class RegionRangeDimensionValidationTck {
   private RegionRangeDimensionValidationTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String dimensionName, CallbackModel callbackModel) throws Exception {
      RTIambassador participant = factory.getRtiAmbassador();
      String federation = "java-tck-region-range-dimension-validation-" + UUID.randomUUID();
      boolean connected = false, created = false, joined = false;
      RegionHandle region = null;
      try {
         participant.connect(callbacks(), callbackModel);
         connected = true;
         participant.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         participant.joinFederationExecution(
            "java-tck-region-range-dimension-validation", federation);
         joined = true;

         DimensionHandle dimension = participant.getDimensionHandle(dimensionName);
         long upperBound = participant.getDimensionUpperBound(dimension);
         JavaTckSupport.check(dimension != null && upperBound > 0,
            "adapter dimension must have a valid handle and positive upper bound");
         DimensionHandleSet dimensions = participant.getDimensionHandleSetFactory().create();
         dimensions.add(dimension);
         region = participant.createRegion(dimensions);
         participant.setRangeBounds(region, dimension, new RangeBounds(0, upperBound));
         RegionHandleSet regions = participant.getRegionHandleSetFactory().create();
         regions.add(region);
         participant.commitRegionModifications(regions);

         DimensionHandle absentDimension =
            JavaTckSupport.invalidHandle(DimensionHandle.class);
         RegionHandle committedRegion = region;
         JavaTckSupport.expectFailure(() -> participant.getRangeBounds(
               committedRegion, absentDimension),
            "RegionDoesNotContainSpecifiedDimension",
            "reading bounds for a dimension absent from the region");
         JavaTckSupport.expectFailure(() -> participant.setRangeBounds(
               committedRegion, absentDimension, new RangeBounds(0, upperBound)),
            "RegionDoesNotContainSpecifiedDimension",
            "setting bounds for a dimension absent from the region");

         participant.deleteRegion(region);
         region = null;
         participant.resignFederationExecution(ResignAction.NO_ACTION);
         joined = false;
         participant.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (region != null && joined) try { participant.deleteRegion(region); }
            catch (Exception ignored) { }
         if (joined) try { participant.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (created) try { participant.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (connected) try { participant.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK region dimension callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) return proxy == arguments[0];
            }
            return null;
         });
   }
}
