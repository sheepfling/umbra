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
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Requires invalid equal and reversed region ranges to be rejected. */
final class RegionRangeValidationTck {
   private RegionRangeValidationTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String dimensionName, CallbackModel callbackModel) throws Exception {
      RTIambassador participant = factory.getRtiAmbassador();
      String federation = "java-tck-region-range-validation-" + UUID.randomUUID();
      boolean connected = false, created = false, joined = false;
      RegionHandle region = null;
      try {
         participant.connect(callbacks(), callbackModel);
         connected = true;
         participant.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         participant.joinFederationExecution("java-tck-region-range-validation", federation);
         joined = true;

         DimensionHandle dimension = participant.getDimensionHandle(dimensionName);
         long upperBound = participant.getDimensionUpperBound(dimension);
         JavaTckSupport.check(dimension != null && upperBound > 0,
            "adapter dimension must have a valid handle and positive upper bound");

         DimensionHandleSet dimensions = participant.getDimensionHandleSetFactory().create();
         dimensions.add(dimension);
         region = participant.createRegion(dimensions);
         JavaTckSupport.check(region != null && region.encodedLength() > 0,
            "region range validation returned an invalid region handle");
         participant.setRangeBounds(region, dimension, new hla.rti1516_2025.RangeBounds(0, upperBound));
         RegionHandleSet regions = participant.getRegionHandleSetFactory().create();
         regions.add(region);
         participant.commitRegionModifications(regions);
         hla.rti1516_2025.RangeBounds actual = participant.getRangeBounds(region, dimension);
         JavaTckSupport.check(actual.lower == 0 && actual.upper == upperBound,
            "range ending at the adapter dimension upper bound did not round-trip");

         RegionHandle committedRegion = region;
         JavaTckSupport.expectFailure(() -> participant.setRangeBounds(committedRegion,
               dimension, new hla.rti1516_2025.RangeBounds(upperBound, upperBound)),
            "InvalidRangeBound", "setting equal lower and upper bounds");
         JavaTckSupport.expectFailure(() -> participant.setRangeBounds(committedRegion,
               dimension, new hla.rti1516_2025.RangeBounds(upperBound, 0)),
            "InvalidRangeBound", "setting a lower bound greater than the upper bound");

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
               if ("toString".equals(method.getName())) return "Java TCK region range callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) return proxy == arguments[0];
            }
            return null;
         });
   }
}
