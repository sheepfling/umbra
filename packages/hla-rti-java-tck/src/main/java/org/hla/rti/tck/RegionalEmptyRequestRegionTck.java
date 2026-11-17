package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeRegionAssociation;
import hla.rti1516_2025.AttributeSetRegionSetPairList;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RangeBounds;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Empty regional request-set no-op and subsequent ordinary class request behavior. */
final class RegionalEmptyRequestRegionTck {
   private static final long TIMEOUT_NANOS = 10_000_000_000L;
   private static final String OBJECT_CLASS = "HLAobjectRoot.TckKnownClassObject";

   private RegionalEmptyRequestRegionTck() { }

   static void run(RtiFactory factory, String fom, String firstDimensionName,
         String secondDimensionName, CallbackModel model) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder requesterCallbacks = new Recorder();
      String federation = "java-tck-regional-empty-request-" + UUID.randomUUID();
      boolean ownerConnected = false, requesterConnected = false, created = false;
      boolean ownerJoined = false, requesterJoined = false, published = false;
      ObjectClassHandle ownerClass = null, requesterClass = null;
      AttributeHandle ownerAttribute = null, requesterAttribute = null;
      AttributeHandleSet ownerAttributes = null, requesterAttributes = null;
      RegionHandle probeRegion = null;
      boolean probeRegionCreated = false;
      try {
         owner.connect(ownerCallbacks.proxy(), model);
         ownerConnected = true;
         requester.connect(requesterCallbacks.proxy(), model);
         requesterConnected = true;
         owner.createFederationExecution(federation, fom);
         created = true;
         owner.joinFederationExecution("regional-empty-request-owner", federation);
         ownerJoined = true;
         requester.joinFederationExecution("regional-empty-request-requester", federation);
         requesterJoined = true;

         ownerClass = owner.getObjectClassHandle(OBJECT_CLASS);
         requesterClass = requester.getObjectClassHandle(OBJECT_CLASS);
         ownerAttribute = owner.getAttributeHandle(ownerClass, "Name");
         requesterAttribute = requester.getAttributeHandle(requesterClass, "Name");
         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         requesterAttributes = JavaTckSupport.attributeSet(requester, requesterAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         published = true;
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null && object.encodedLength() > 0
               && owner.isAttributeOwnedByFederate(object, ownerAttribute),
            "empty regional request did not use an owned default-region object");

         RegionHandleSet emptyRegions = requester.getRegionHandleSetFactory().create();
         AttributeSetRegionSetPairList emptyPair = pair(
            requester, requesterAttributes, emptyRegions);
         byte[] emptyRequestTag = new byte[] {0x19, 0x36};
         byte[] ordinaryRequestTag = new byte[] {0x51, 0x63};
         ownerCallbacks.provided.clear();
         requester.requestAttributeValueUpdateWithRegions(
            requesterClass, emptyPair, emptyRequestTag);
         requester.requestAttributeValueUpdate(
            requesterClass, requesterAttributes, ordinaryRequestTag);
         await(owner, requester, model, () -> ownerCallbacks.provided.stream().anyMatch(
               update -> Arrays.equals(ordinaryRequestTag, update.tag)),
            "ordinary class request after an empty regional request");
         List<ProvidedUpdate> requests = List.copyOf(ownerCallbacks.provided);
         JavaTckSupport.check(requests.size() == 1 && object.equals(requests.get(0).object)
               && ownerAttributes.equals(requests.get(0).attributes)
               && Arrays.equals(ordinaryRequestTag, requests.get(0).tag)
               && requests.stream().noneMatch(update -> Arrays.equals(emptyRequestTag, update.tag)),
            "empty region set generated a callback or changed the ordinary request result");

         // Java has no portable default-invalid RegionHandle value. Keep a standard API-created
         // handle, delete its region, and use that stale designator for the invalid-region check.
         DimensionHandle first = requester.getDimensionHandle(firstDimensionName);
         DimensionHandle second = requester.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(!first.equals(second)
               && requester.getAvailableDimensionsForObjectClass(requesterClass).contains(first)
               && requester.getAvailableDimensionsForObjectClass(requesterClass).contains(second),
            "empty request adapter class omitted one of its two DDM dimensions");
         DimensionHandleSet probeDimensions = requester.getDimensionHandleSetFactory().create();
         probeDimensions.add(first);
         probeDimensions.add(second);
         probeRegion = requester.createRegion(probeDimensions);
         probeRegionCreated = true;
         requester.setRangeBounds(probeRegion, first, new RangeBounds(1, 5));
         requester.setRangeBounds(probeRegion, second, new RangeBounds(1, 5));
         RegionHandleSet probeCommit = requester.getRegionHandleSetFactory().create();
         probeCommit.add(probeRegion);
         requester.commitRegionModifications(probeCommit);
         RegionHandle invalidRegion = probeRegion;
         requester.deleteRegion(probeRegion);
         probeRegionCreated = false;
         RegionHandleSet invalidRegions = requester.getRegionHandleSetFactory().create();
         invalidRegions.add(invalidRegion);
         AttributeSetRegionSetPairList invalidPair = pair(
            requester, requesterAttributes, invalidRegions);
         ObjectClassHandle invalidRequestClass = requesterClass;
         JavaTckSupport.expectFailure(() -> requester.requestAttributeValueUpdateWithRegions(
               invalidRequestClass, invalidPair, emptyRequestTag),
            "InvalidRegion", "regional request with a nonempty invalid region designator");
      } finally {
         if (requesterJoined) deleteRegion(requester, probeRegion, probeRegionCreated);
         if (ownerJoined && published && ownerClass != null && ownerAttributes != null) try {
            owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes);
         } catch (Exception ignored) { }
         if (requesterJoined) try { requester.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (ownerJoined) try { owner.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         if (created) try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (requesterConnected) try { requester.disconnect(); }
            catch (Exception ignored) { }
         if (ownerConnected) try { owner.disconnect(); }
            catch (Exception ignored) { }
      }
   }

   private static AttributeSetRegionSetPairList pair(RTIambassador ambassador,
         AttributeHandleSet attributes, RegionHandleSet regions) throws Exception {
      AttributeSetRegionSetPairList result =
         ambassador.getAttributeSetRegionSetPairListFactory().create(1);
      result.add(new AttributeRegionAssociation(attributes, regions));
      return result;
   }

   private static void deleteRegion(RTIambassador ambassador, RegionHandle region,
         boolean created) {
      if (created && region != null) try { ambassador.deleteRegion(region); }
         catch (Exception ignored) { }
   }

   private static void await(RTIambassador first, RTIambassador second, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         if (model == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(first);
            JavaTckSupport.drain(second);
         } else Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static final class ProvidedUpdate {
      private final ObjectInstanceHandle object;
      private final AttributeHandleSet attributes;
      private final byte[] tag;
      private ProvidedUpdate(ObjectInstanceHandle object, AttributeHandleSet attributes,
            byte[] tag) {
         this.object = object;
         this.attributes = attributes;
         this.tag = tag;
      }
   }

   private static final class Recorder {
      private final List<ProvidedUpdate> provided = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("provideAttributeValueUpdate".equals(name) && arguments != null
                     && arguments.length == 3) {
                  provided.add(new ProvidedUpdate((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) ((AttributeHandleSet) arguments[1]).clone(),
                     ((byte[]) arguments[2]).clone()));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK empty regional request callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == arguments[0];
               }
               return null;
            });
      }
   }
}
