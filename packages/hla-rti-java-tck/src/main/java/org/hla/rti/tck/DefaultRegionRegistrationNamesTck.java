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
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Tests default-source registration overloads and generated object-name identity. */
final class DefaultRegionRegistrationNamesTck {
   private static final long TIMEOUT_NANOS = 10_000_000_000L;
   private DefaultRegionRegistrationNamesTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName, String attributeName,
         String firstDimensionName, String secondDimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder callbacks = new Recorder();
      String federation = "java-tck-default-region-registration-names-" + UUID.randomUUID();
      boolean publisherConnected = false, observerConnected = false, created = false;
      boolean publisherJoined = false, observerJoined = false, published = false, subscribed = false;
      boolean observerRegionCreated = false;
      ObjectClassHandle publisherClass = null, observerClass = null;
      AttributeHandle publisherAttribute = null, observerAttribute = null;
      AttributeHandleSet publisherAttributes = null, observerAttributes = null;
      AttributeSetRegionSetPairList observerPair = null;
      RegionHandle observerRegion = null;
      try {
         publisher.connect(noopFederateAmbassador(), callbackModel); publisherConnected = true;
         observer.connect(callbacks.proxy(), callbackModel); observerConnected = true;
         publisher.createFederationExecution(federation, fom); created = true;
         String publisherType = "default-registration-publisher";
         FederateHandle producer = publisher.joinFederationExecution(publisherType, federation);
         publisherJoined = true;
         String publisherName = publisher.getFederateName(producer);
         JavaTckSupport.check(publisherName != null && !publisherName.isEmpty(),
            "publisher federate handle did not resolve to a name");
         observer.joinFederationExecution("default-registration-observer", federation);
         observerJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         observerClass = observer.getObjectClassHandle(objectClassName);
         publisherAttribute = publisher.getAttributeHandle(publisherClass, attributeName);
         observerAttribute = observer.getAttributeHandle(observerClass, attributeName);
         DimensionHandle publisherFirst = publisher.getDimensionHandle(firstDimensionName);
         DimensionHandle publisherSecond = publisher.getDimensionHandle(secondDimensionName);
         DimensionHandle observerFirst = observer.getDimensionHandle(firstDimensionName);
         DimensionHandle observerSecond = observer.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(objectClassName.equals(publisher.getObjectClassName(publisherClass))
               && objectClassName.equals(observer.getObjectClassName(observerClass))
               && attributeName.equals(publisher.getAttributeName(publisherClass, publisherAttribute))
               && attributeName.equals(observer.getAttributeName(observerClass, observerAttribute)),
            "adapter DDM FOM returned inconsistent registration class or attribute names");
         checkDimensions(publisher, publisherClass, publisherFirst, publisherSecond,
            firstDimensionName, secondDimensionName);
         checkDimensions(observer, observerClass, observerFirst, observerSecond,
            firstDimensionName, secondDimensionName);

         publisherAttributes = JavaTckSupport.attributeSet(publisher, publisherAttribute);
         observerAttributes = JavaTckSupport.attributeSet(observer, observerAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         observerRegion = createRegion(observer, observerFirst, observerSecond, 2, 6);
         observerRegionCreated = true;
         observer.setConveyRegionDesignatorSetsSwitch(true);
         JavaTckSupport.check(observer.getConveyRegionDesignatorSetsSwitch(),
            "default-region observer did not enable region-designator callbacks");
         observerPair = association(observer, observerAttributes, observerRegion);
         observer.subscribeObjectClassAttributesWithRegions(observerClass, observerPair);
         subscribed = true;

         AttributeSetRegionSetPairList emptyRegionPair =
            associationWithEmptyRegions(publisher, publisherAttributes);
         AttributeSetRegionSetPairList emptyPairCollection =
            publisher.getAttributeSetRegionSetPairListFactory().create(0);
         ObjectInstanceHandle first =
            publisher.registerObjectInstanceWithRegions(publisherClass, emptyRegionPair);
         ObjectInstanceHandle second =
            publisher.registerObjectInstanceWithRegions(publisherClass, emptyPairCollection);
         ObjectInstanceHandle third = publisher.registerObjectInstance(publisherClass);
         List<ObjectInstanceHandle> objects = Arrays.asList(first, second, third);
         for (ObjectInstanceHandle object : objects) {
            JavaTckSupport.check(object != null && object.encodedLength() > 0,
               "default-region registration returned an invalid object handle");
         }
         JavaTckSupport.check(!first.equals(second) && !first.equals(third) && !second.equals(third),
            "default-region registration reused an object handle");

         List<String> names = new ArrayList<>();
         Set<String> uniqueNames = new HashSet<>();
         for (ObjectInstanceHandle object : objects) {
            String name = publisher.getObjectInstanceName(object);
            JavaTckSupport.check(name != null && !name.isEmpty()
                  && object.equals(publisher.getObjectInstanceHandle(name))
                  && uniqueNames.add(name),
               "default-region registration returned a missing, duplicate, or non-round-tripping name");
            names.add(name);
         }

         await(publisher, observer, callbackModel,
            () -> callbacks.discoveries.size() >= objects.size(), "default-region discoveries");
         JavaTckSupport.check(callbacks.discoveries.size() == objects.size(),
            "default-region registration produced duplicate discovery callbacks");
         Set<String> discoveredNames = new HashSet<>();
         for (Discovery discovery : callbacks.discoveries) {
            JavaTckSupport.check(discovery.name != null && discoveredNames.add(discovery.name),
               "default-region registration produced a missing or duplicate discovery name");
         }
         JavaTckSupport.check(discoveredNames.equals(uniqueNames),
            "default-region discovery names did not match the registered objects");
         List<ObjectInstanceHandle> observerObjects = new ArrayList<>();
         for (int index = 0; index < objects.size(); ++index) {
            String name = names.get(index);
            Discovery discovery = null;
            for (Discovery candidate : callbacks.discoveries) {
               if (name.equals(candidate.name)) { discovery = candidate; break; }
            }
            ObjectInstanceHandle observerObject = observer.getObjectInstanceHandle(name);
            JavaTckSupport.check(discovery != null && discovery.object.equals(observerObject)
                  && discovery.objectClass.equals(observerClass)
                  && observer.getKnownObjectClassHandle(observerObject).equals(observerClass)
                  && name.equals(observer.getObjectInstanceName(observerObject))
                  && observerObject.equals(observer.getObjectInstanceHandle(name)),
               "default-region discovery changed object class or generated object identity");
            observerObjects.add(observerObject);
         }

         byte[][] values = { {(byte) 0xa1, 0x01}, {(byte) 0xa1, 0x02}, {(byte) 0xa1, 0x03} };
         byte[][] tags = { {(byte) 0xb1, 0x01}, {(byte) 0xb1, 0x02}, {(byte) 0xb1, 0x03} };
         for (int index = 0; index < objects.size(); ++index) {
            AttributeHandleValueMap update = publisher.getAttributeHandleValueMapFactory().create(1);
            update.put(publisherAttribute, values[index].clone());
            publisher.updateAttributeValues(objects.get(index), update, tags[index].clone());
         }
         await(publisher, observer, callbackModel,
            () -> callbacks.reflections.size() >= objects.size(), "default-region reflections");
         JavaTckSupport.check(callbacks.reflections.size() == objects.size(),
            "default-region updates produced an unexpected reflection count");
         for (int index = 0; index < objects.size(); ++index) {
            Reflection found = null;
            for (Reflection reflection : callbacks.reflections) {
               if (observerObjects.get(index).equals(reflection.object)) { found = reflection; break; }
            }
            JavaTckSupport.check(found != null, "default-region registration omitted an object update");
            assertReflection(found, observer, observerObjects.get(index), observerAttribute,
               publisherName, values[index], tags[index], "default-region reflection " + index);
         }

         observer.unsubscribeObjectClassAttributesWithRegions(observerClass, observerPair);
         subscribed = false;
         publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         published = false;
         deleteRegion(observer, observerRegion); observerRegionCreated = false;
         publisher.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         publisherJoined = false;
         observer.resignFederationExecution(ResignAction.NO_ACTION); observerJoined = false;
         publisher.destroyFederationExecution(federation); created = false;
      } finally {
         if (observerJoined && subscribed && observerPair != null) try {
            observer.unsubscribeObjectClassAttributesWithRegions(observerClass, observerPair);
         } catch (Exception ignored) { }
         if (published && publisherJoined && publisherAttributes != null) try {
            publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         } catch (Exception ignored) { }
         if (observerJoined) deleteRegion(observer, observerRegion, observerRegionCreated);
         if (publisherJoined) try {
            publisher.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         } catch (Exception ignored) { }
         if (observerJoined) try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (publisherConnected) try { publisher.disconnect(); } catch (Exception ignored) { }
         if (observerConnected) try { observer.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static void checkDimensions(RTIambassador ambassador, ObjectClassHandle objectClass,
         DimensionHandle first, DimensionHandle second, String firstName, String secondName)
         throws Exception {
      DimensionHandleSet dimensions = ambassador.getAvailableDimensionsForObjectClass(objectClass);
      JavaTckSupport.check(dimensions.contains(first) && dimensions.contains(second)
            && !first.equals(second) && firstName.equals(ambassador.getDimensionName(first))
            && secondName.equals(ambassador.getDimensionName(second))
            && ambassador.getDimensionUpperBound(first) >= 6
            && ambassador.getDimensionUpperBound(second) >= 6,
         "adapter object class must expose two distinct DDM dimensions through range 6");
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle first,
         DimensionHandle second, long lower, long upper) throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(first); dimensions.add(second);
      RegionHandle region = ambassador.createRegion(dimensions);
      ambassador.setRangeBounds(region, first, new hla.rti1516_2025.RangeBounds(lower, upper));
      ambassador.setRangeBounds(region, second, new hla.rti1516_2025.RangeBounds(lower, upper));
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      regions.add(region);
      ambassador.commitRegionModifications(regions);
      return region;
   }

   private static AttributeSetRegionSetPairList association(RTIambassador ambassador,
         AttributeHandleSet attributes, RegionHandle region) throws Exception {
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      regions.add(region);
      return association(ambassador, attributes, regions);
   }

   private static AttributeSetRegionSetPairList associationWithEmptyRegions(
         RTIambassador ambassador, AttributeHandleSet attributes) throws Exception {
      return association(ambassador, attributes, ambassador.getRegionHandleSetFactory().create());
   }

   private static AttributeSetRegionSetPairList association(RTIambassador ambassador,
         AttributeHandleSet attributes, RegionHandleSet regions) throws Exception {
      AttributeSetRegionSetPairList result =
         ambassador.getAttributeSetRegionSetPairListFactory().create(1);
      result.add(new AttributeRegionAssociation(attributes, regions));
      return result;
   }

   private static void assertReflection(Reflection reflection, RTIambassador receiver,
         ObjectInstanceHandle object, AttributeHandle attribute, String producerName,
         byte[] value, byte[] tag, String description) throws Exception {
      JavaTckSupport.check(object.equals(reflection.object) && reflection.values.size() == 1
            && Arrays.equals(value, reflection.values.get(attribute))
            && Arrays.equals(tag, reflection.tag)
            && producerName.equals(receiver.getFederateName(reflection.producer)),
         description + " changed object, value, tag, or producer");
      JavaTckSupport.check(reflection.transportation != null
            && reflection.transportation.encodedLength() > 0,
         description + " returned an invalid transportation handle");
      JavaTckSupport.check(reflection.regions != null && reflection.regions.isEmpty(),
         description + " did not carry an explicitly empty default-source region set");
   }

   private static void await(RTIambassador first, RTIambassador second, CallbackModel model,
         java.util.function.BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         pump(first, model); pump(second, model); Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void pump(RTIambassador ambassador, CallbackModel model) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
      else Thread.yield();
   }

   private static void deleteRegion(RTIambassador ambassador, RegionHandle region) {
      deleteRegion(ambassador, region, region != null);
   }

   private static void deleteRegion(RTIambassador ambassador, RegionHandle region, boolean created) {
      if (created && region != null) try { ambassador.deleteRegion(region); }
         catch (Exception ignored) { }
   }

   private static FederateAmbassador noopFederateAmbassador() {
      return (FederateAmbassador) Proxy.newProxyInstance(FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class}, (proxy, method, args) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK default registration publisher";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) return proxy == args[0];
            }
            return null;
         });
   }

   private static final class Reflection {
      private ObjectInstanceHandle object;
      private AttributeHandleValueMap values;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private RegionHandleSet regions;
   }

   private static final class Discovery {
      private ObjectInstanceHandle object;
      private ObjectClassHandle objectClass;
      private String name;
   }

   private static final class Recorder {
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class}, (proxy, method, args) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && args != null && args.length >= 3) {
                  Discovery discovery = new Discovery();
                  discovery.object = (ObjectInstanceHandle) args[0];
                  discovery.objectClass = (ObjectClassHandle) args[1];
                  discovery.name = (String) args[2];
                  discoveries.add(discovery);
               } else if ("reflectAttributeValues".equals(name) && args != null && args.length == 6) {
                  Reflection reflection = new Reflection();
                  reflection.object = (ObjectInstanceHandle) args[0];
                  reflection.values = ((AttributeHandleValueMap) args[1]).clone();
                  reflection.tag = ((byte[]) args[2]).clone();
                  reflection.transportation = (TransportationTypeHandle) args[3];
                  reflection.producer = (FederateHandle) args[4];
                  reflection.regions = (RegionHandleSet) args[5];
                  reflections.add(reflection);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK default registration callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (args == null ? null : args[0]);
               }
               return null;
            });
      }
   }
}
