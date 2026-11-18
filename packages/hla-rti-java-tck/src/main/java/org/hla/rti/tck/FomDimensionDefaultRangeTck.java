package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RangeBounds;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Public FOM dimension and region default-range boundary checks. */
final class FomDimensionDefaultRangeTck {
   private FomDimensionDefaultRangeTck() { }

   static void run(RtiFactory factory, String outside, String bounded,
         String mim, String timeImplementation, CallbackModel callbackModel)
         throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-dimension-default-range-" + UUID.randomUUID();
      boolean creatorConnected = false;
      boolean memberConnected = false;
      boolean memberJoined = false;
      boolean created = false;
      RegionHandle region = null;
      try {
         creator.connect(callbacks(), callbackModel);
         creatorConnected = true;
         member.connect(callbacks(), callbackModel);
         memberConnected = true;
         JavaTckSupport.expectFailure(() -> creator.createFederationExecutionWithMIM(
               federation, new String[] {outside}, mim, timeImplementation),
            "InconsistentFOM", "dimension default range beyond upperBound");

         creator.createFederationExecutionWithMIM(federation,
            new String[] {bounded}, mim, timeImplementation);
         created = true;
         member.joinFederationExecution("java-tck-dimension-range-member", federation);
         memberJoined = true;
         String name = "TckBoundedDefaultDimension";
         DimensionHandle dimension = member.getDimensionHandle(name);
         JavaTckSupport.check(dimension != null
               && name.equals(member.getDimensionName(dimension)),
            "bounded dimension default range did not compose publicly");
         long upperBound = member.getDimensionUpperBound(dimension);
         JavaTckSupport.check(upperBound == 100,
            "bounded dimension did not expose its exact upper bound");
         DimensionHandleSet dimensions =
            member.getDimensionHandleSetFactory().create();
         dimensions.add(dimension);
         region = member.createRegion(dimensions);
         JavaTckSupport.check(region != null && region.encodedLength() > 0,
            "bounded default dimension could not be used in a region");
         RangeBounds initialBounds = member.getRangeBounds(region, dimension);
         JavaTckSupport.check(initialBounds.lower == 0
               && initialBounds.upper == upperBound,
            "a new region did not receive the FOM-supplied default range");
         RegionHandleSet regions = member.getRegionHandleSetFactory().create();
         regions.add(region);
         member.commitRegionModifications(regions);
         RangeBounds committedBounds = member.getRangeBounds(region, dimension);
         JavaTckSupport.check(committedBounds.lower == 0
               && committedBounds.upper == upperBound,
            "committing a default-only region changed the FOM-supplied range");
         member.setRangeBounds(region, dimension, new RangeBounds(0, 1));
         RangeBounds minimumSpan = member.getRangeBounds(region, dimension);
         JavaTckSupport.check(minimumSpan.lower == 0 && minimumSpan.upper == 1,
            "the minimum one-unit region interval did not round-trip");
         RegionHandle rangeTestRegion = region;
         JavaTckSupport.expectFailure(
            () -> member.setRangeBounds(rangeTestRegion, dimension, new RangeBounds(1, 1)),
            "InvalidRangeBound", "a zero-width region interval");
         member.setRangeBounds(region, dimension, new RangeBounds(0, upperBound));
         RangeBounds explicitBounds = member.getRangeBounds(region, dimension);
         JavaTckSupport.check(explicitBounds.lower == 0
               && explicitBounds.upper == upperBound,
            "an explicit full-dimension interval did not preserve bound order");
         member.commitRegionModifications(regions);
         RangeBounds recommittedBounds = member.getRangeBounds(region, dimension);
         JavaTckSupport.check(recommittedBounds.lower == 0
               && recommittedBounds.upper == upperBound,
            "committing explicit full-dimension bounds changed the interval");
         member.deleteRegion(region);
         region = null;
         member.resignFederationExecution(ResignAction.NO_ACTION);
         memberJoined = false;
         creator.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (region != null) try { member.deleteRegion(region); }
            catch (Exception ignored) { }
         if (memberJoined) try { member.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (created) try { creator.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (memberConnected) try { member.disconnect(); }
            catch (Exception ignored) { }
         if (creatorConnected) try { creator.disconnect(); }
            catch (Exception ignored) { }
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK dimension callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
