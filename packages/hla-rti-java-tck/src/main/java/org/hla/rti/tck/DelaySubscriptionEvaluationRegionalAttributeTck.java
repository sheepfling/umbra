package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.AttributeRegionAssociation;
import hla.rti1516_2025.AttributeSetRegionSetPairList;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RangeBounds;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Tests receive-order regional attribute re-evaluation at the callback boundary. */
final class DelaySubscriptionEvaluationRegionalAttributeTck {
   private static final long TIMEOUT_NANOS = 10_000_000_000L;

   private DelaySubscriptionEvaluationRegionalAttributeTck() { }

   static void run(RtiFactory factory, String switchesFom, String ddmFom,
         String timeImplementation, String objectClassName, String attributeName,
         String firstDimensionName, String secondDimensionName, CallbackModel model)
         throws Exception {
      runConfiguration(factory, switchesFom, ddmFom, timeImplementation, objectClassName,
         attributeName, firstDimensionName, secondDimensionName, model, false);
      runConfiguration(factory, switchesFom, ddmFom, timeImplementation, objectClassName,
         attributeName, firstDimensionName, secondDimensionName, model, true);
   }

   private static void runConfiguration(RtiFactory factory, String switchesFom, String ddmFom,
         String timeImplementation, String objectClassName, String attributeName,
         String firstDimensionName, String secondDimensionName, CallbackModel model,
         boolean delayEnabled) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherEvents = new Recorder();
      Recorder receiverEvents = new Recorder();
      String federation = "java-tck-delay-subscription-ro-regional-" + delayEnabled
         + "-" + UUID.randomUUID();
      boolean publisherConnected = false, receiverConnected = false, created = false;
      boolean publisherJoined = false, receiverJoined = false;
      boolean subscribed = false, associated = false;
      RegionHandle sourceRegion = null, receiverRegion = null;
      ObjectClassHandle publisherClass = null, receiverClass = null;
      AttributeHandle publisherAttribute = null, receiverAttribute = null;
      AttributeHandleSet publisherAttributes = null, receiverAttributes = null;
      AttributeSetRegionSetPairList publisherPair = null, receiverPair = null;
      ObjectInstanceHandle object = null;
      try {
         publisher.connect(publisherEvents.proxy(), model);
         publisherConnected = true;
         receiver.connect(receiverEvents.proxy(), model);
         receiverConnected = true;
         String[] modules = delayEnabled
            ? new String[] {ddmFom, switchesFom} : new String[] {ddmFom};
         publisher.createFederationExecution(federation, modules, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-delay-regional-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-delay-regional-receiver", federation);
         receiverJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         receiverClass = receiver.getObjectClassHandle(objectClassName);
         publisherAttribute = publisher.getAttributeHandle(publisherClass, attributeName);
         receiverAttribute = receiver.getAttributeHandle(receiverClass, attributeName);
         DimensionHandle publisherFirst = publisher.getDimensionHandle(firstDimensionName);
         DimensionHandle publisherSecond = publisher.getDimensionHandle(secondDimensionName);
         DimensionHandle receiverFirst = receiver.getDimensionHandle(firstDimensionName);
         DimensionHandle receiverSecond = receiver.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(publisherClass.equals(receiverClass)
               && publisherAttribute.equals(receiverAttribute)
               && publisherFirst.equals(receiverFirst) && publisherSecond.equals(receiverSecond),
            "adapter DDM FOM returned inconsistent class, attribute, or dimension handles");
         checkDimensions(publisher, publisherClass, publisherFirst, publisherSecond,
            firstDimensionName, secondDimensionName);
         checkDimensions(receiver, receiverClass, receiverFirst, receiverSecond,
            firstDimensionName, secondDimensionName);
         JavaTckSupport.check(publisher.getDelaySubscriptionEvaluationSwitch() == delayEnabled
               && receiver.getDelaySubscriptionEvaluationSwitch() == delayEnabled,
            "regional Delay Subscription Evaluation switch did not match FOM composition");

         publisherAttributes = JavaTckSupport.attributeSet(publisher, publisherAttribute);
         receiverAttributes = JavaTckSupport.attributeSet(receiver, receiverAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         receiver.setConveyRegionDesignatorSetsSwitch(true);
         JavaTckSupport.check(receiver.getConveyRegionDesignatorSetsSwitch(),
            "receiver did not enable source-region designator callbacks");
         sourceRegion = createRegion(publisher, publisherFirst, publisherSecond, 1, 5);
         receiverRegion = createRegion(receiver, receiverFirst, receiverSecond, 2, 6);
         publisherPair = association(publisher, publisherAttributes, sourceRegion);
         receiverPair = association(receiver, receiverAttributes, receiverRegion);
         receiver.subscribeObjectClassAttributesWithRegions(receiverClass, receiverPair);
         subscribed = true;
         object = publisher.registerObjectInstanceWithRegions(publisherClass, publisherPair);
         associated = true;
         ObjectInstanceHandle registeredObject = object;
         await(receiver, model, () -> receiverEvents.discoveries.contains(registeredObject),
            "regional object discovery");
         JavaTckSupport.check(receiverClass.equals(receiver.getKnownObjectClassHandle(object)),
            "regional discovery did not establish the known object class");

         // D1 is generated while unsubscribed and region-disjoint. With DSE on,
         // callback admission must use the restored declaration and committed ranges.
         callbackGate(receiver, model, false);
         receiver.unsubscribeObjectClassAttributesWithRegions(receiverClass, receiverPair);
         setRange(receiver, receiverRegion, receiverFirst, receiverSecond, 7, 9);
         update(publisher, object, publisherAttribute, (byte) 0xD1);
         setRange(publisher, sourceRegion, publisherFirst, publisherSecond, 7, 9);
         setRange(receiver, receiverRegion, receiverFirst, receiverSecond, 2, 6);
         receiver.subscribeObjectClassAttributesWithRegions(receiverClass, receiverPair);
         subscribed = true;
         callbackGate(receiver, model, true);
         if (delayEnabled) {
            await(receiver, model, () -> receiverEvents.reflections.size() >= 1,
               "enabled regional late-subscription reflection");
         } else {
            pump(receiver, model);
         }
         List<Reflection> first = List.copyOf(receiverEvents.reflections);
         JavaTckSupport.check(delayEnabled ? first.size() == 1 : first.size() <= 1,
            "regional late subscription produced an unexpected reflection count");
         if (!first.isEmpty()) {
            verify(first.get(0), object, receiverAttribute, (byte) 0xD1, producer,
               sourceRegion, receiver, "late regional reflection");
         }
         receiverEvents.reflections.clear();

         // Queue D2 while relevant, then make it disjoint before callback service.
         // D3 restores overlap and proves that the D2 suppression is not a lost queue.
         callbackGate(receiver, model, false);
         setRange(publisher, sourceRegion, publisherFirst, publisherSecond, 1, 5);
         update(publisher, object, publisherAttribute, (byte) 0xD2);
         setRange(receiver, receiverRegion, receiverFirst, receiverSecond, 7, 9);
         setRange(publisher, sourceRegion, publisherFirst, publisherSecond, 7, 9);
         update(publisher, object, publisherAttribute, (byte) 0xD3);
         callbackGate(receiver, model, true);
         await(receiver, model, () -> receiverEvents.reflections.size() >= 1,
            "regional suppression and matching successor delivery");
         List<Reflection> afterRangeChange = List.copyOf(receiverEvents.reflections);
         JavaTckSupport.check(afterRangeChange.size() == 1,
            "disjoint regional passel escaped callback-boundary re-evaluation");
         verify(afterRangeChange.get(0), object, receiverAttribute, (byte) 0xD3, producer,
            sourceRegion, receiver, "matching successor reflection");

         receiver.unsubscribeObjectClassAttributesWithRegions(receiverClass, receiverPair);
         subscribed = false;
         publisher.unassociateRegionsForUpdates(object, publisherPair);
         associated = false;
         receiver.deleteRegion(receiverRegion);
         receiverRegion = null;
         publisher.deleteRegion(sourceRegion);
         sourceRegion = null;
         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (subscribed && receiverJoined && receiverClass != null && receiverPair != null) {
            try { receiver.unsubscribeObjectClassAttributesWithRegions(receiverClass, receiverPair); }
            catch (Exception ignored) { }
         }
         if (associated && publisherJoined && object != null && publisherPair != null) {
            try { publisher.unassociateRegionsForUpdates(object, publisherPair); }
            catch (Exception ignored) { }
         }
         if (receiverJoined) resign(receiver);
         if (publisherJoined) {
            try { publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         }
         if (receiverRegion != null && receiverJoined) deleteRegion(receiver, receiverRegion);
         if (sourceRegion != null && publisherJoined) deleteRegion(publisher, sourceRegion);
         if (created) {
            try { publisher.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (receiverConnected) disconnect(receiver);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static void checkDimensions(RTIambassador ambassador, ObjectClassHandle objectClass,
         DimensionHandle first, DimensionHandle second, String firstName, String secondName)
         throws Exception {
      java.util.Set<DimensionHandle> available =
         ambassador.getAvailableDimensionsForObjectClass(objectClass);
      JavaTckSupport.check(available.contains(first) && available.contains(second)
            && ambassador.getDimensionName(first).equals(firstName)
            && ambassador.getDimensionName(second).equals(secondName)
            && ambassador.getDimensionUpperBound(first) >= 9
            && ambassador.getDimensionUpperBound(second) >= 9,
         "adapter DDM object class omitted a configured dimension or required range");
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle first,
         DimensionHandle second, long lower, long upper) throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(first);
      dimensions.add(second);
      RegionHandle region = ambassador.createRegion(dimensions);
      setRange(ambassador, region, first, second, lower, upper);
      return region;
   }

   private static void setRange(RTIambassador ambassador, RegionHandle region,
         DimensionHandle first, DimensionHandle second, long lower, long upper) throws Exception {
      ambassador.setRangeBounds(region, first, new RangeBounds(lower, upper));
      ambassador.setRangeBounds(region, second, new RangeBounds(lower, upper));
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      regions.add(region);
      ambassador.commitRegionModifications(regions);
   }

   private static AttributeSetRegionSetPairList association(RTIambassador ambassador,
         AttributeHandleSet attributes, RegionHandle region) throws Exception {
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      regions.add(region);
      AttributeSetRegionSetPairList pairs =
         ambassador.getAttributeSetRegionSetPairListFactory().create(1);
      pairs.add(new AttributeRegionAssociation(attributes, regions));
      return pairs;
   }

   private static void update(RTIambassador publisher, ObjectInstanceHandle object,
         AttributeHandle attribute, byte marker) throws Exception {
      AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(1);
      values.put(attribute, new byte[] {marker});
      publisher.updateAttributeValues(object, values, new byte[] {marker});
   }

   private static void verify(Reflection reflection, ObjectInstanceHandle object,
         AttributeHandle attribute, byte marker, FederateHandle producer, RegionHandle sourceRegion,
         RTIambassador receiver, String phase) throws Exception {
      JavaTckSupport.check(object.equals(reflection.object)
            && reflection.values.size() == 1 && reflection.values.containsKey(attribute)
            && Arrays.equals(new byte[] {marker}, reflection.values.get(attribute))
            && Arrays.equals(new byte[] {marker}, reflection.tag)
            && producer.equals(reflection.producer) && reflection.transportation != null
            && !receiver.getTransportationTypeName(reflection.transportation).isEmpty()
            && reflection.regions != null && reflection.regions.size() == 1
            && reflection.regions.contains(sourceRegion),
         phase + " lost standard reflection metadata or source region designator");
   }

   private static void callbackGate(RTIambassador receiver, CallbackModel model, boolean open)
         throws Exception {
      if (model == CallbackModel.HLA_IMMEDIATE) {
         if (open) receiver.enableCallbacks(); else receiver.disableCallbacks();
      } else if (open) {
         while (receiver.evokeCallback(0.0)) { }
      }
   }

   private static void await(RTIambassador receiver, CallbackModel model,
         java.util.function.BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         pump(receiver, model);
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void pump(RTIambassador receiver, CallbackModel model) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(receiver);
      else Thread.yield();
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void deleteRegion(RTIambassador ambassador, RegionHandle region) {
      try { ambassador.deleteRegion(region); } catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static final class Reflection {
      final ObjectInstanceHandle object;
      final AttributeHandleValueMap values;
      final byte[] tag;
      final TransportationTypeHandle transportation;
      final FederateHandle producer;
      final RegionHandleSet regions;
      Reflection(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         values = ((AttributeHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
         transportation = (TransportationTypeHandle) arguments[3];
         producer = (FederateHandle) arguments[4];
         regions = arguments[5] == null ? null : ((RegionHandleSet) arguments[5]).clone();
      }
   }

   private static final class Recorder {
      final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               Object[] a = arguments == null ? new Object[0] : arguments;
               if ("discoverObjectInstance".equals(method.getName()) && a.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) a[0]);
               } else if ("reflectAttributeValues".equals(method.getName()) && a.length == 6) {
                  reflections.add(new Reflection(a));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(method.getName())) return "Java TCK regional DSE callbacks";
                  if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
                  if ("equals".equals(method.getName())) return proxy == a[0];
               }
               return null;
            });
      }
   }
}
