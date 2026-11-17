package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RangeBounds;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks FDD default-range realization for a non-empty interaction region. */
final class RegionalInteractionPartialDefaultRangeTck {
   private static final String INTERACTION = "HLAinteractionRoot.TckInteraction";
   private static final String PARAMETER = "Payload";
   private static final String X_DIMENSION = "TckDimensionX";
   private static final String Y_DIMENSION = "TckDimensionY";
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private RegionalInteractionPartialDefaultRangeTck() { }

   static void runFromAdapter() throws Exception {
      run(RtiTckMain.factory(), RtiTckMain.requireAdapterFile("hla.rti.tck.defaultRangeFom"),
         RtiTckMain.callbackModel());
   }

   static void run(RtiFactory factory, String fom, CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador overlapping = factory.getRtiAmbassador();
      RTIambassador disjoint = factory.getRtiAmbassador();
      RTIambassador boundary = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder overlappingCallbacks = new Recorder();
      Recorder disjointCallbacks = new Recorder();
      Recorder boundaryCallbacks = new Recorder();
      RTIambassador[] ambassadors = {publisher, overlapping, disjoint, boundary};
      Recorder[] recorders = {
         publisherCallbacks, overlappingCallbacks, disjointCallbacks, boundaryCallbacks};
      boolean[] connected = new boolean[4];
      boolean[] joined = new boolean[4];
      boolean created = false;
      boolean published = false;
      boolean overlappingSubscribed = false;
      boolean disjointSubscribed = false;
      boolean boundarySubscribed = false;
      String federation = "java-tck-regional-interaction-partial-default-range-" + UUID.randomUUID();
      FederateHandle publisherFederate = null;
      InteractionClassHandle publisherInteraction = null;
      InteractionClassHandle overlappingInteraction = null;
      InteractionClassHandle disjointInteraction = null;
      InteractionClassHandle boundaryInteraction = null;
      ParameterHandle publisherParameter = null;
      ParameterHandle overlappingParameter = null;
      ParameterHandle disjointParameter = null;
      ParameterHandle boundaryParameter = null;
      RegionHandle sourceRegion = null;
      RegionHandle overlappingRegion = null;
      RegionHandle disjointRegion = null;
      RegionHandle boundaryRegion = null;
      RegionHandleSet sourceRegions = null;
      RegionHandleSet overlappingRegions = null;
      RegionHandleSet disjointRegions = null;
      RegionHandleSet boundaryRegions = null;
      try {
         JavaTckSupport.check(fom != null && !fom.isEmpty(),
            "adapter-supplied restricted default-range FOM is required");
         for (int index = 0; index < ambassadors.length; ++index) {
            ambassadors[index].connect(recorders[index].proxy(), callbackModel);
            connected[index] = true;
         }
         publisher.createFederationExecution(federation, fom);
         created = true;
         publisherFederate = publisher.joinFederationExecution(
            "partial-region-interaction-publisher", federation);
         joined[0] = true;
         overlapping.joinFederationExecution("partial-region-interaction-overlapping", federation);
         joined[1] = true;
         disjoint.joinFederationExecution("partial-region-interaction-disjoint", federation);
         joined[2] = true;
         boundary.joinFederationExecution("partial-region-interaction-boundary", federation);
         joined[3] = true;
         JavaTckSupport.check(publisherFederate != null && publisherFederate.encodedLength() > 0,
            "interaction publisher join returned an invalid federate handle");

         publisherInteraction = publisher.getInteractionClassHandle(INTERACTION);
         overlappingInteraction = overlapping.getInteractionClassHandle(INTERACTION);
         disjointInteraction = disjoint.getInteractionClassHandle(INTERACTION);
         boundaryInteraction = boundary.getInteractionClassHandle(INTERACTION);
         publisherParameter = publisher.getParameterHandle(publisherInteraction, PARAMETER);
         overlappingParameter = overlapping.getParameterHandle(overlappingInteraction, PARAMETER);
         disjointParameter = disjoint.getParameterHandle(disjointInteraction, PARAMETER);
         boundaryParameter = boundary.getParameterHandle(boundaryInteraction, PARAMETER);
         DimensionHandle publisherX = publisher.getDimensionHandle(X_DIMENSION);
         DimensionHandle publisherY = publisher.getDimensionHandle(Y_DIMENSION);
         DimensionHandle overlappingX = overlapping.getDimensionHandle(X_DIMENSION);
         DimensionHandle overlappingY = overlapping.getDimensionHandle(Y_DIMENSION);
         DimensionHandle disjointX = disjoint.getDimensionHandle(X_DIMENSION);
         DimensionHandle disjointY = disjoint.getDimensionHandle(Y_DIMENSION);
         DimensionHandle boundaryX = boundary.getDimensionHandle(X_DIMENSION);
         DimensionHandle boundaryY = boundary.getDimensionHandle(Y_DIMENSION);
         JavaTckSupport.check(publisherInteraction != null && publisherInteraction.encodedLength() > 0
               && publisherInteraction.equals(overlappingInteraction)
               && publisherInteraction.equals(disjointInteraction)
               && publisherInteraction.equals(boundaryInteraction)
               && publisherParameter != null && publisherParameter.encodedLength() > 0
               && publisherParameter.equals(overlappingParameter)
               && publisherParameter.equals(disjointParameter)
               && publisherParameter.equals(boundaryParameter)
               && publisherX != null && publisherX.encodedLength() > 0
               && publisherY != null && publisherY.encodedLength() > 0
               && publisherX.equals(overlappingX) && publisherY.equals(overlappingY)
               && publisherX.equals(disjointX) && publisherY.equals(disjointY)
               && publisherX.equals(boundaryX) && publisherY.equals(boundaryY),
            "members resolved inconsistent FDD interaction, parameter, or dimension handles");
         checkAvailableDimensions(publisher, publisherInteraction, publisherX, publisherY);
         checkAvailableDimensions(overlapping, overlappingInteraction, overlappingX, overlappingY);
         checkAvailableDimensions(disjoint, disjointInteraction, disjointX, disjointY);
         checkAvailableDimensions(boundary, boundaryInteraction, boundaryX, boundaryY);
         JavaTckSupport.check(publisher.getDimensionUpperBound(publisherX) == 10L
               && publisher.getDimensionUpperBound(publisherY) == 10L,
            "restricted default-range FOM did not retain the declared dimension bounds");

         publisher.publishInteractionClass(publisherInteraction);
         published = true;
         sourceRegion = createRegion(publisher, publisherX, null, 1, 4, false, 0, 0);
         DimensionHandleSet sourceDimensions = publisher.getDimensionHandleSet(sourceRegion);
         JavaTckSupport.check(sourceDimensions.size() == 1 && sourceDimensions.contains(publisherX)
               && !sourceDimensions.contains(publisherY),
            "non-empty source region did not remain dimensioned by X only");
         overlappingRegion = createRegion(overlapping, overlappingX, overlappingY,
            2, 3, false, 0, 5);
         disjointRegion = createRegion(disjoint, disjointX, disjointY, 2, 3, true, 6, 9);
         boundaryRegion = createRegion(boundary, boundaryX, boundaryY, 2, 3, true, 5, 7);
         sourceRegions = regionSet(publisher, sourceRegion);
         overlappingRegions = regionSet(overlapping, overlappingRegion);
         disjointRegions = regionSet(disjoint, disjointRegion);
         boundaryRegions = regionSet(boundary, boundaryRegion);
         overlapping.subscribeInteractionClassWithRegions(overlappingInteraction, overlappingRegions);
         overlappingSubscribed = true;
         disjoint.subscribeInteractionClassWithRegions(disjointInteraction, disjointRegions);
         disjointSubscribed = true;
         boundary.subscribeInteractionClassWithRegions(boundaryInteraction, boundaryRegions);
         boundarySubscribed = true;

         ParameterHandleValueMap parameters = publisher.getParameterHandleValueMapFactory().create(1);
         byte[] payload = new byte[] {0x4d, 0x21};
         byte[] tag = new byte[] {0x70, 0x61, 0x72, 0x74};
         parameters.put(publisherParameter, payload.clone());
         publisher.sendInteractionWithRegions(
            publisherInteraction, parameters, sourceRegions, tag.clone());
         await(callbackModel, overlapping,
            () -> overlappingCallbacks.interactions.size() >= 1,
            "delivery within the omitted Y dimension's FDD default range");
         settle(callbackModel, overlapping, disjoint, boundary);

         JavaTckSupport.check(overlappingCallbacks.interactions.size() == 1,
            "overlapping interaction receiver did not receive exactly one callback");
         Interaction received = overlappingCallbacks.interactions.get(0);
         byte[] receivedPayload = received.parameters.get(overlappingParameter);
         JavaTckSupport.check(received.interaction.equals(overlappingInteraction)
               && received.parameters.size() == 1 && receivedPayload != null
               && Arrays.equals(receivedPayload, payload) && Arrays.equals(received.tag, tag)
               && publisherFederate.equals(received.producer),
            "interaction did not preserve class, payload, tag, or producer metadata");
         JavaTckSupport.check(disjointCallbacks.interactions.isEmpty(),
            "interaction escaped the omitted Y dimension's FDD default [0,5) range");
         JavaTckSupport.check(boundaryCallbacks.interactions.isEmpty(),
            "interaction reached a Y region beginning at the FDD default's exclusive upper bound");
      } finally {
         if (overlappingSubscribed && joined[1]) {
            try { overlapping.unsubscribeInteractionClassWithRegions(
               overlappingInteraction, overlappingRegions); }
            catch (Exception ignored) { }
         }
         if (disjointSubscribed && joined[2]) {
            try { disjoint.unsubscribeInteractionClassWithRegions(
               disjointInteraction, disjointRegions); }
            catch (Exception ignored) { }
         }
         if (boundarySubscribed && joined[3]) {
            try { boundary.unsubscribeInteractionClassWithRegions(
               boundaryInteraction, boundaryRegions); }
            catch (Exception ignored) { }
         }
         if (published && joined[0] && publisherInteraction != null) {
            try { publisher.unpublishInteractionClass(publisherInteraction); }
            catch (Exception ignored) { }
         }
         if (sourceRegion != null && joined[0]) {
            try { publisher.deleteRegion(sourceRegion); }
            catch (Exception ignored) { }
         }
         if (overlappingRegion != null && joined[1]) {
            try { overlapping.deleteRegion(overlappingRegion); }
            catch (Exception ignored) { }
         }
         if (disjointRegion != null && joined[2]) {
            try { disjoint.deleteRegion(disjointRegion); }
            catch (Exception ignored) { }
         }
         if (boundaryRegion != null && joined[3]) {
            try { boundary.deleteRegion(boundaryRegion); }
            catch (Exception ignored) { }
         }
         if (joined[3]) resign(boundary);
         if (joined[2]) resign(disjoint);
         if (joined[1]) resign(overlapping);
         if (joined[0]) resign(publisher);
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         for (int index = 0; index < ambassadors.length; ++index) {
            if (connected[index]) {
               try { ambassadors[index].disconnect(); }
               catch (Exception ignored) { }
            }
         }
      }
   }

   private static void checkAvailableDimensions(RTIambassador ambassador,
         InteractionClassHandle interaction, DimensionHandle x, DimensionHandle y)
         throws Exception {
      DimensionHandleSet available = ambassador.getAvailableDimensionsForInteractionClass(interaction);
      JavaTckSupport.check(available.size() == 2 && available.contains(x) && available.contains(y),
         "sent interaction class did not expose both FDD-associated dimensions");
   }

   private static RegionHandle createRegion(RTIambassador ambassador,
         DimensionHandle x, DimensionHandle y, long xLower, long xUpper,
         boolean setYBounds, long yLower, long yUpper) throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(x);
      if (y != null) dimensions.add(y);
      RegionHandle region = ambassador.createRegion(dimensions);
      JavaTckSupport.check(region != null && region.encodedLength() > 0,
         "createRegion returned an invalid interaction region handle");
      ambassador.setRangeBounds(region, x, new RangeBounds(xLower, xUpper));
      if (y != null) {
         if (setYBounds) ambassador.setRangeBounds(region, y, new RangeBounds(yLower, yUpper));
         RangeBounds pendingY = ambassador.getRangeBounds(region, y);
         JavaTckSupport.check(pendingY != null && pendingY.lower == yLower
               && pendingY.upper == yUpper,
            setYBounds ? "explicit interaction-region Y bounds did not round-trip before commit"
               : "RTI did not supply the interaction dimension's [0,5) FDD default before commit");
      }
      RegionHandleSet regions = regionSet(ambassador, region);
      ambassador.commitRegionModifications(regions);
      if (y != null) {
         RangeBounds committedY = ambassador.getRangeBounds(region, y);
         JavaTckSupport.check(committedY != null && committedY.lower == yLower
               && committedY.upper == yUpper,
            "interaction-region Y range changed at commit");
      }
      return region;
   }

   private static RegionHandleSet regionSet(RTIambassador ambassador, RegionHandle region)
         throws Exception {
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      regions.add(region);
      return regions;
   }

   private static void await(CallbackModel callbackModel, RTIambassador ambassador,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.01);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void settle(CallbackModel callbackModel, RTIambassador... ambassadors)
         throws Exception {
      long deadline = System.nanoTime() + 250_000_000L;
      while (System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            for (RTIambassador ambassador : ambassadors) ambassador.evokeCallback(0.0);
         } else {
            Thread.yield();
         }
      }
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static final class Interaction {
      private final InteractionClassHandle interaction;
      private final Map<ParameterHandle, byte[]> parameters = new HashMap<>();
      private final byte[] tag;
      private final FederateHandle producer;

      private Interaction(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         ParameterHandleValueMap values = (ParameterHandleValueMap) arguments[1];
         values.forEach((handle, value) -> parameters.put(handle, value.clone()));
         tag = ((byte[]) arguments[2]).clone();
         producer = (FederateHandle) arguments[4];
      }
   }

   private static final class Recorder {
      private final java.util.List<Interaction> interactions = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("receiveInteraction".equals(method.getName()) && arguments != null
                     && arguments.length == 6) {
                  interactions.add(new Interaction(arguments));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(method.getName())) return "Java TCK partial region interaction";
                  if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
                  if ("equals".equals(method.getName())) return proxy == arguments[0];
               }
               return null;
            });
      }
   }
}
