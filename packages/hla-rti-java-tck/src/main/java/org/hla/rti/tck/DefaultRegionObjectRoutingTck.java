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
import hla.rti1516_2025.RangeBounds;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verifies default-source routing through ordinary and regional object subscriptions. */
final class DefaultRegionObjectRoutingTck {
   private static final long TIMEOUT_NANOS = 10_000_000_000L;
   private DefaultRegionObjectRoutingTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName, String attributeName,
         String firstDimensionName, String secondDimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador regional = factory.getRtiAmbassador();
      RTIambassador mixed = factory.getRtiAmbassador();
      Recorder regionalCallbacks = new Recorder();
      Recorder mixedCallbacks = new Recorder();
      String federation = "java-tck-default-region-object-routing-" + UUID.randomUUID();
      String publisherName = "default-region-object-publisher";
      String regionalName = "default-region-object-regional";
      String mixedName = "default-region-object-mixed";
      boolean publisherConnected = false, regionalConnected = false, mixedConnected = false;
      boolean created = false, publisherJoined = false, regionalJoined = false, mixedJoined = false;
      boolean published = false, regionalSubscribed = false, mixedOrdinarySubscribed = false;
      boolean mixedRegionalSubscribed = false, sourceAssociated = false;
      boolean mixedSourceAssociated = false;
      boolean sourceCreated = false, regionalRegionCreated = false, mixedRegionCreated = false;
      boolean mixedSourceRegionCreated = false;
      ObjectClassHandle publisherClass = null, regionalClass = null, mixedClass = null;
      AttributeHandle publisherAttribute = null, regionalAttribute = null, mixedAttribute = null;
      AttributeHandleSet publisherAttributes = null, regionalAttributes = null, mixedAttributes = null;
      AttributeSetRegionSetPairList sourcePair = null, regionalPair = null, mixedPair = null;
      AttributeSetRegionSetPairList mixedSourcePair = null;
      RegionHandle sourceRegion = null, regionalRegion = null, mixedRegion = null;
      RegionHandle mixedSourceRegion = null;
      ObjectInstanceHandle object = null;
      try {
         publisher.connect(noopFederateAmbassador(), callbackModel); publisherConnected = true;
         regional.connect(regionalCallbacks.proxy(), callbackModel); regionalConnected = true;
         mixed.connect(mixedCallbacks.proxy(), callbackModel); mixedConnected = true;
         publisher.createFederationExecution(federation, fom); created = true;
         publisher.joinFederationExecution(publisherName, "tck-type", federation);
         publisherJoined = true;
         regional.joinFederationExecution(regionalName, "tck-type", federation);
         regionalJoined = true;
         mixed.joinFederationExecution(mixedName, "tck-type", federation);
         mixedJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         regionalClass = regional.getObjectClassHandle(objectClassName);
         mixedClass = mixed.getObjectClassHandle(objectClassName);
         publisherAttribute = publisher.getAttributeHandle(publisherClass, attributeName);
         regionalAttribute = regional.getAttributeHandle(regionalClass, attributeName);
         mixedAttribute = mixed.getAttributeHandle(mixedClass, attributeName);
         DimensionHandle publisherFirst = publisher.getDimensionHandle(firstDimensionName);
         DimensionHandle publisherSecond = publisher.getDimensionHandle(secondDimensionName);
         DimensionHandle regionalFirst = regional.getDimensionHandle(firstDimensionName);
         DimensionHandle regionalSecond = regional.getDimensionHandle(secondDimensionName);
         DimensionHandle mixedFirst = mixed.getDimensionHandle(firstDimensionName);
         DimensionHandle mixedSecond = mixed.getDimensionHandle(secondDimensionName);
         regionalCallbacks.inspectSourceRegions(regional, regionalFirst, regionalSecond);
         mixedCallbacks.inspectSourceRegions(mixed, mixedFirst, mixedSecond);
         long[] publisherBounds = defaultRoutingDimensionBounds(publisher, publisherClass,
            publisherFirst, publisherSecond, firstDimensionName, secondDimensionName);
         long[] regionalBounds = defaultRoutingDimensionBounds(regional, regionalClass,
            regionalFirst, regionalSecond, firstDimensionName, secondDimensionName);
         long[] mixedBounds = defaultRoutingDimensionBounds(mixed, mixedClass,
            mixedFirst, mixedSecond, firstDimensionName, secondDimensionName);
         JavaTckSupport.check(Arrays.equals(publisherBounds, regionalBounds)
               && Arrays.equals(publisherBounds, mixedBounds),
            "default-region members reported different FDD dimension upper bounds");
         long firstUpperBound = publisherBounds[0];
         long secondUpperBound = publisherBounds[1];
         JavaTckSupport.check(firstUpperBound >= 2 && secondUpperBound >= 2,
            "default-region boundary routing requires a positive unit in each FDD dimension");

         publisherAttributes = JavaTckSupport.attributeSet(publisher, publisherAttribute);
         regionalAttributes = JavaTckSupport.attributeSet(regional, regionalAttribute);
         mixedAttributes = JavaTckSupport.attributeSet(mixed, mixedAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         sourceRegion = createRegion(publisher, publisherFirst, publisherSecond,
            0, firstUpperBound - 1, 0, secondUpperBound - 1);
         sourceCreated = true;
         regionalRegion = createRegion(regional, regionalFirst, regionalSecond, 0, 1, 0, 1);
         regionalRegionCreated = true;
         mixedRegion = createRegion(mixed, mixedFirst, mixedSecond,
            firstUpperBound - 1, firstUpperBound, secondUpperBound - 1, secondUpperBound);
         mixedRegionCreated = true;
         mixedSourceRegion = createRegion(publisher, publisherFirst, publisherSecond,
            firstUpperBound - 1, firstUpperBound, secondUpperBound - 1, secondUpperBound);
         mixedSourceRegionCreated = true;
         verifyRegion(publisher, sourceRegion, publisherFirst, publisherSecond,
            0, firstUpperBound - 1, 0, secondUpperBound - 1, "default-region source");
         verifyRegion(regional, regionalRegion, regionalFirst, regionalSecond,
            0, 1, 0, 1, "overlapping regional subscription");
         verifyRegion(mixed, mixedRegion, mixedFirst, mixedSecond,
            firstUpperBound - 1, firstUpperBound, secondUpperBound - 1, secondUpperBound,
            "disjoint regional subscription");
         verifyRegion(publisher, mixedSourceRegion, publisherFirst, publisherSecond,
            firstUpperBound - 1, firstUpperBound, secondUpperBound - 1, secondUpperBound,
            "matching explicit source");
         sourcePair = association(publisher, publisherAttributes, sourceRegion);
         regionalPair = association(regional, regionalAttributes, regionalRegion);
         mixedPair = association(mixed, mixedAttributes, mixedRegion);
         mixedSourcePair = association(publisher, publisherAttributes, mixedSourceRegion);

         regional.setConveyRegionDesignatorSetsSwitch(true);
         mixed.setConveyRegionDesignatorSetsSwitch(true);
         JavaTckSupport.check(regional.getConveyRegionDesignatorSetsSwitch()
               && mixed.getConveyRegionDesignatorSetsSwitch(),
            "default-region receivers did not enable region-designator callbacks");
         regional.subscribeObjectClassAttributesWithRegions(regionalClass, regionalPair);
         regionalSubscribed = true;
         mixed.subscribeObjectClassAttributes(mixedClass, mixedAttributes);
         mixedOrdinarySubscribed = true;
         mixed.subscribeObjectClassAttributesWithRegions(mixedClass, mixedPair);
         mixedRegionalSubscribed = true;

         object = publisher.registerObjectInstanceWithRegions(publisherClass, sourcePair);
         sourceAssociated = true;
         JavaTckSupport.check(object != null,
            "default-region registration returned an invalid object handle");
         String objectName = publisher.getObjectInstanceName(object);
         await(regional, mixed, callbackModel,
            () -> regionalCallbacks.discoveryNames.contains(objectName)
               && mixedCallbacks.discoveryNames.contains(objectName),
            "default-region object discovery");
         settle(regional, mixed, callbackModel);
         ObjectInstanceHandle regionalObject = regional.getObjectInstanceHandle(objectName);
         ObjectInstanceHandle mixedObject = mixed.getObjectInstanceHandle(objectName);
         JavaTckSupport.check(regionalCallbacks.discoveries.size() == 1
               && regionalCallbacks.discoveries.contains(regionalObject)
               && mixedCallbacks.discoveries.size() == 1
               && mixedCallbacks.discoveries.contains(mixedObject),
            "default-region discovery did not resolve once through each member's own handle");

         mixed.unsubscribeObjectClassAttributesWithRegions(mixedClass, mixedPair);
         mixedRegionalSubscribed = false;
         mixed.subscribeObjectClassAttributes(mixedClass, mixedAttributes);
         mixedOrdinarySubscribed = true;
         settle(regional, mixed, callbackModel);
         JavaTckSupport.check(mixedCallbacks.discoveryNames.contains(objectName)
               && mixedCallbacks.discoveries.size() == 1,
            "ordinary default-region discovery produced an unexpected callback count");

         mixed.subscribeObjectClassAttributesWithRegions(mixedClass, mixedPair);
         mixedRegionalSubscribed = true;
         publisher.unassociateRegionsForUpdates(object, sourcePair);
         sourceAssociated = false;
         regionalCallbacks.reflections.clear();
         mixedCallbacks.reflections.clear();
         byte[] defaultValue = new byte[] {(byte) 0xd0, 0x01};
         byte[] defaultTag = new byte[] {0x44, 0x31};
         update(publisher, object, publisherAttribute, defaultValue, defaultTag);
         await(regional, mixed, callbackModel,
            () -> regionalCallbacks.reflections.size() >= 1 && mixedCallbacks.reflections.size() >= 1,
            "default-source ordinary attribute reflection");
         settle(regional, mixed, callbackModel);
         JavaTckSupport.check(regionalCallbacks.reflections.size() == 1
               && mixedCallbacks.reflections.size() == 1,
            "default-source update produced an unexpected callback count");
         assertReflection(regionalCallbacks.reflections.get(0), regional, regionalObject,
            regionalAttribute, publisherName, defaultValue, defaultTag, null,
            "regional default-source reflection");
         assertReflection(mixedCallbacks.reflections.get(0), mixed, mixedObject,
            mixedAttribute, publisherName, defaultValue, defaultTag, null,
            "mixed default-source reflection");

         publisher.associateRegionsForUpdates(object, sourcePair);
         sourceAssociated = true;
         byte[] explicitValue = new byte[] {(byte) 0xe0, 0x02};
         byte[] explicitTag = new byte[] {0x44, 0x32};
         update(publisher, object, publisherAttribute, explicitValue, explicitTag);
         await(regional, mixed, callbackModel,
            () -> regionalCallbacks.reflections.size() >= 2
               && mixedCallbacks.reflections.size() >= 2,
            "explicit-source regional attribute reflection");
         settle(regional, mixed, callbackModel);
         assertReflection(regionalCallbacks.reflections.get(1), regional, regionalObject,
            regionalAttribute, publisherName, explicitValue, explicitTag,
            new long[] {0, firstUpperBound - 1, 0, secondUpperBound - 1},
            "explicit-source reflection");
         assertReflection(mixedCallbacks.reflections.get(1), mixed, mixedObject,
            mixedAttribute, publisherName, explicitValue, explicitTag,
            new long[] {0, firstUpperBound - 1, 0, secondUpperBound - 1},
            "ordinary default-region reflection with a disjoint regional declaration");
         JavaTckSupport.check(mixedCallbacks.reflections.size() == 2,
            "disjoint regional declaration duplicated the ordinary default-region reflection");
         JavaTckSupport.check(regionalCallbacks.reflections.size() == 2,
            "explicit-source update produced an unexpected regional callback count");

         publisher.unassociateRegionsForUpdates(object, sourcePair);
         sourceAssociated = false;
         byte[] restoredValue = new byte[] {(byte) 0xd0, 0x03};
         byte[] restoredTag = new byte[] {0x44, 0x33};
         update(publisher, object, publisherAttribute, restoredValue, restoredTag);
         await(regional, mixed, callbackModel,
            () -> regionalCallbacks.reflections.size() >= 3 && mixedCallbacks.reflections.size() >= 3,
            "restored default-source reflection");
         settle(regional, mixed, callbackModel);
         assertReflection(regionalCallbacks.reflections.get(2), regional, regionalObject,
            regionalAttribute, publisherName, restoredValue, restoredTag, null,
            "regional restored default-source reflection");
         assertReflection(mixedCallbacks.reflections.get(2), mixed, mixedObject,
            mixedAttribute, publisherName, restoredValue, restoredTag, null,
            "mixed restored default-source reflection");
         JavaTckSupport.check(mixedCallbacks.reflections.size() == 3,
            "restored default-source update produced an unexpected callback count");
         JavaTckSupport.check(regionalCallbacks.reflections.size() == 3,
            "restored default-source update produced an unexpected regional callback count");

         mixed.unsubscribeObjectClassAttributes(mixedClass, mixedAttributes);
         mixedOrdinarySubscribed = false;
         publisher.associateRegionsForUpdates(object, sourcePair);
         sourceAssociated = true;
         byte[] unsubscribedDefaultValue = new byte[] {(byte) 0xd0, 0x04};
         byte[] unsubscribedDefaultTag = new byte[] {0x44, 0x34};
         update(publisher, object, publisherAttribute,
            unsubscribedDefaultValue, unsubscribedDefaultTag);
         await(regional, mixed, callbackModel,
            () -> regionalCallbacks.reflections.size() >= 4,
            "explicit-source update after ordinary unsubscribe");
         settle(regional, mixed, callbackModel);
         assertReflection(regionalCallbacks.reflections.get(3), regional, regionalObject,
            regionalAttribute, publisherName, unsubscribedDefaultValue, unsubscribedDefaultTag,
            new long[] {0, firstUpperBound - 1, 0, secondUpperBound - 1},
            "explicit-source reflection after ordinary unsubscribe");
         JavaTckSupport.check(mixedCallbacks.reflections.size() == 3,
            "ordinary unsubscribe retained the implicit default-region route for a disjoint source");
         JavaTckSupport.check(regionalCallbacks.reflections.size() == 4,
            "ordinary unsubscribe changed delivery to the remaining regional subscriber");
         publisher.unassociateRegionsForUpdates(object, sourcePair);
         sourceAssociated = false;

         publisher.associateRegionsForUpdates(object, mixedSourcePair);
         mixedSourceAssociated = true;
         byte[] retainedRegionalValue = new byte[] {(byte) 0xe0, 0x04};
         byte[] retainedRegionalTag = new byte[] {0x44, 0x35};
         update(publisher, object, publisherAttribute,
            retainedRegionalValue, retainedRegionalTag);
         await(regional, mixed, callbackModel,
            () -> mixedCallbacks.reflections.size() >= 4,
            "explicit regional update after ordinary unsubscribe");
         settle(regional, mixed, callbackModel);
         assertReflection(mixedCallbacks.reflections.get(3), mixed, mixedObject,
            mixedAttribute, publisherName, retainedRegionalValue, retainedRegionalTag,
            new long[] {firstUpperBound - 1, firstUpperBound,
               secondUpperBound - 1, secondUpperBound},
            "explicit regional reflection after ordinary unsubscribe");
         JavaTckSupport.check(regionalCallbacks.reflections.size() == 4
               && mixedCallbacks.reflections.size() == 4,
            "final default/regional routing delivered an extra or missing reflection");
         publisher.unassociateRegionsForUpdates(object, mixedSourcePair);
         mixedSourceAssociated = false;

         mixed.unsubscribeObjectClassAttributesWithRegions(mixedClass, mixedPair);
         mixedRegionalSubscribed = false;
         regional.unsubscribeObjectClassAttributesWithRegions(regionalClass, regionalPair);
         regionalSubscribed = false;
         publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         published = false;
         deleteRegion(publisher, sourceRegion); sourceCreated = false;
         deleteRegion(publisher, mixedSourceRegion); mixedSourceRegionCreated = false;
         deleteRegion(regional, regionalRegion); regionalRegionCreated = false;
         deleteRegion(mixed, mixedRegion); mixedRegionCreated = false;
         publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         publisherJoined = false;
         regional.resignFederationExecution(ResignAction.NO_ACTION); regionalJoined = false;
         mixed.resignFederationExecution(ResignAction.NO_ACTION); mixedJoined = false;
         publisher.destroyFederationExecution(federation); created = false;
      } finally {
         if (publisherJoined && mixedSourceAssociated && object != null && mixedSourcePair != null) try {
            publisher.unassociateRegionsForUpdates(object, mixedSourcePair);
         } catch (Exception ignored) { }
         if (publisherJoined && sourceAssociated && object != null && sourcePair != null) try {
            publisher.unassociateRegionsForUpdates(object, sourcePair);
         } catch (Exception ignored) { }
         if (mixedJoined && mixedRegionalSubscribed && mixedPair != null) try {
            mixed.unsubscribeObjectClassAttributesWithRegions(mixedClass, mixedPair);
         } catch (Exception ignored) { }
         if (mixedJoined && mixedOrdinarySubscribed && mixedAttributes != null) try {
            mixed.unsubscribeObjectClassAttributes(mixedClass, mixedAttributes);
         } catch (Exception ignored) { }
         if (regionalJoined && regionalSubscribed && regionalPair != null) try {
            regional.unsubscribeObjectClassAttributesWithRegions(regionalClass, regionalPair);
         } catch (Exception ignored) { }
         if (published && publisherJoined && publisherAttributes != null) try {
            publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         } catch (Exception ignored) { }
         if (publisherJoined) deleteRegion(publisher, sourceRegion, sourceCreated);
         if (publisherJoined) deleteRegion(publisher, mixedSourceRegion, mixedSourceRegionCreated);
         if (regionalJoined) deleteRegion(regional, regionalRegion, regionalRegionCreated);
         if (mixedJoined) deleteRegion(mixed, mixedRegion, mixedRegionCreated);
         if (mixedJoined) try { mixed.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (regionalJoined) try { regional.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (publisherJoined) try {
            publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         } catch (Exception ignored) { }
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (publisherConnected) try { publisher.disconnect(); } catch (Exception ignored) { }
         if (regionalConnected) try { regional.disconnect(); } catch (Exception ignored) { }
         if (mixedConnected) try { mixed.disconnect(); } catch (Exception ignored) { }
      }
   }

   static void registrationAtomicity(RtiFactory factory, String fom, String objectClassName,
         String attributeName, String firstDimensionName, String secondDimensionName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador overlap = factory.getRtiAmbassador();
      RTIambassador disjoint = factory.getRtiAmbassador();
      Recorder overlapCallbacks = new Recorder();
      Recorder disjointCallbacks = new Recorder();
      String federation = "java-tck-regional-registration-atomicity-" + UUID.randomUUID();
      boolean ownerConnected = false, overlapConnected = false, disjointConnected = false;
      boolean created = false, ownerJoined = false, overlapJoined = false, disjointJoined = false;
      boolean published = false, overlapSubscribed = false, disjointSubscribed = false;
      boolean sourceAssociated = false;
      ObjectClassHandle ownerClass = null, overlapClass = null, disjointClass = null;
      AttributeHandle ownerAttribute = null, overlapAttribute = null, disjointAttribute = null;
      AttributeHandleSet ownerAttributes = null, overlapAttributes = null, disjointAttributes = null;
      AttributeSetRegionSetPairList sourcePair = null, overlapPair = null, disjointPair = null;
      RegionHandle sourceRegion = null, overlapRegion = null, disjointRegion = null;
      ObjectInstanceHandle object = null;
      try {
         owner.connect(noopFederateAmbassador(), callbackModel);
         ownerConnected = true;
         overlap.connect(overlapCallbacks.proxy(), callbackModel);
         overlapConnected = true;
         disjoint.connect(disjointCallbacks.proxy(), callbackModel);
         disjointConnected = true;
         owner.createFederationExecution(federation, fom);
         created = true;
         owner.joinFederationExecution("java-tck-registration-atomicity-owner", federation);
         ownerJoined = true;
         overlap.joinFederationExecution("java-tck-registration-atomicity-overlap", federation);
         overlapJoined = true;
         disjoint.joinFederationExecution("java-tck-registration-atomicity-disjoint", federation);
         disjointJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         overlapClass = overlap.getObjectClassHandle(objectClassName);
         disjointClass = disjoint.getObjectClassHandle(objectClassName);
         ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         overlapAttribute = overlap.getAttributeHandle(overlapClass, attributeName);
         disjointAttribute = disjoint.getAttributeHandle(disjointClass, attributeName);
         DimensionHandle ownerFirst = owner.getDimensionHandle(firstDimensionName);
         DimensionHandle ownerSecond = owner.getDimensionHandle(secondDimensionName);
         DimensionHandle overlapFirst = overlap.getDimensionHandle(firstDimensionName);
         DimensionHandle overlapSecond = overlap.getDimensionHandle(secondDimensionName);
         DimensionHandle disjointFirst = disjoint.getDimensionHandle(firstDimensionName);
         DimensionHandle disjointSecond = disjoint.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(ownerClass.equals(overlapClass)
               && ownerClass.equals(disjointClass)
               && ownerAttribute.equals(overlapAttribute)
               && ownerAttribute.equals(disjointAttribute)
               && ownerFirst.equals(overlapFirst) && ownerFirst.equals(disjointFirst)
               && ownerSecond.equals(overlapSecond) && ownerSecond.equals(disjointSecond),
            "adapter DDM FOM handles changed across registration-atomicity federates");
         checkDimensions(owner, ownerClass, ownerFirst, ownerSecond);
         checkDimensions(overlap, overlapClass, overlapFirst, overlapSecond);
         checkDimensions(disjoint, disjointClass, disjointFirst, disjointSecond);

         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         overlapAttributes = JavaTckSupport.attributeSet(overlap, overlapAttribute);
         disjointAttributes = JavaTckSupport.attributeSet(disjoint, disjointAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         published = true;
         sourceRegion = createRegion(owner, ownerFirst, ownerSecond, 1, 5);
         overlapRegion = createRegion(overlap, overlapFirst, overlapSecond, 2, 6);
         disjointRegion = createRegion(disjoint, disjointFirst, disjointSecond, 7, 9);
         sourcePair = association(owner, ownerAttributes, sourceRegion);
         overlapPair = association(overlap, overlapAttributes, overlapRegion);
         disjointPair = association(disjoint, disjointAttributes, disjointRegion);
         overlap.subscribeObjectClassAttributesWithRegions(overlapClass, overlapPair);
         overlapSubscribed = true;
         disjoint.subscribeObjectClassAttributesWithRegions(disjointClass, disjointPair);
         disjointSubscribed = true;

         object = owner.registerObjectInstanceWithRegions(ownerClass, sourcePair);
         sourceAssociated = true;
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "region-qualified registration returned an invalid object handle");
         ObjectInstanceHandle registered = object;
         await(overlap, disjoint, callbackModel,
            () -> overlapCallbacks.discoveries.contains(registered),
            "overlapping regional discovery after atomic registration");
         pumpSeveral(overlap, disjoint, callbackModel);
         JavaTckSupport.check(overlapCallbacks.discoveries.size() == 1
               && disjointCallbacks.discoveries.isEmpty(),
            "region-qualified registration exposed the object to a disjoint subscriber");
      } finally {
         if (ownerJoined && sourceAssociated && object != null && sourcePair != null) try {
            owner.unassociateRegionsForUpdates(object, sourcePair);
         } catch (Exception ignored) { }
         if (overlapJoined && overlapSubscribed && overlapPair != null) try {
            overlap.unsubscribeObjectClassAttributesWithRegions(overlapClass, overlapPair);
         } catch (Exception ignored) { }
         if (disjointJoined && disjointSubscribed && disjointPair != null) try {
            disjoint.unsubscribeObjectClassAttributesWithRegions(disjointClass, disjointPair);
         } catch (Exception ignored) { }
         if (ownerJoined && published && ownerAttributes != null) try {
            owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes);
         } catch (Exception ignored) { }
         if (ownerJoined && sourceRegion != null) deleteRegion(owner, sourceRegion);
         if (overlapJoined && overlapRegion != null) deleteRegion(overlap, overlapRegion);
         if (disjointJoined && disjointRegion != null) deleteRegion(disjoint, disjointRegion);
         if (overlapJoined) try { overlap.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (disjointJoined) try { disjoint.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (ownerJoined) try { owner.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         if (created) try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (disjointConnected) try { disjoint.disconnect(); } catch (Exception ignored) { }
         if (overlapConnected) try { overlap.disconnect(); } catch (Exception ignored) { }
         if (ownerConnected) try { owner.disconnect(); } catch (Exception ignored) { }
      }
   }

   static void registrationOmittedAttributeDefaultRegion(RtiFactory factory, String fom,
         String objectClassName, String explicitlyAssociatedAttributeName,
         String omittedAttributeName, String firstDimensionName, String secondDimensionName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder callbacks = new Recorder();
      String federation = "java-tck-registration-omitted-attribute-default-region-"
         + UUID.randomUUID();
      boolean ownerConnected = false, subscriberConnected = false, created = false;
      boolean ownerJoined = false, subscriberJoined = false, published = false;
      boolean subscribed = false, sourceAssociated = false;
      ObjectClassHandle ownerClass = null, subscriberClass = null;
      AttributeHandle ownerExplicit = null, ownerOmitted = null;
      AttributeHandle subscriberExplicit = null, subscriberOmitted = null;
      AttributeHandleSet ownerAttributes = null, subscriberAttributes = null;
      AttributeSetRegionSetPairList sourcePair = null, subscriptionPair = null;
      RegionHandle sourceRegion = null, subscriptionRegion = null;
      ObjectInstanceHandle object = null;
      try {
         owner.connect(noopFederateAmbassador(), callbackModel);
         ownerConnected = true;
         subscriber.connect(callbacks.proxy(), callbackModel);
         subscriberConnected = true;
         owner.createFederationExecution(federation, fom);
         created = true;
         FederateHandle producer = owner.joinFederationExecution(
            "java-tck-registration-omitted-owner", federation);
         ownerJoined = true;
         subscriber.joinFederationExecution("java-tck-registration-omitted-subscriber", federation);
         subscriberJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         subscriberClass = subscriber.getObjectClassHandle(objectClassName);
         ownerExplicit = owner.getAttributeHandle(ownerClass, explicitlyAssociatedAttributeName);
         ownerOmitted = owner.getAttributeHandle(ownerClass, omittedAttributeName);
         subscriberExplicit = subscriber.getAttributeHandle(
            subscriberClass, explicitlyAssociatedAttributeName);
         subscriberOmitted = subscriber.getAttributeHandle(subscriberClass, omittedAttributeName);
         DimensionHandle ownerFirst = owner.getDimensionHandle(firstDimensionName);
         DimensionHandle ownerSecond = owner.getDimensionHandle(secondDimensionName);
         DimensionHandle subscriberFirst = subscriber.getDimensionHandle(firstDimensionName);
         DimensionHandle subscriberSecond = subscriber.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(ownerClass.equals(subscriberClass)
               && ownerExplicit.equals(subscriberExplicit) && ownerOmitted.equals(subscriberOmitted)
               && !ownerExplicit.equals(ownerOmitted)
               && ownerFirst.equals(subscriberFirst) && ownerSecond.equals(subscriberSecond),
            "adapter multi-attribute DDM handles changed across registration federates");
         checkDimensions(owner, ownerClass, ownerFirst, ownerSecond);
         checkDimensions(subscriber, subscriberClass, subscriberFirst, subscriberSecond);

         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerExplicit, ownerOmitted);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         published = true;
         sourceRegion = createRegion(owner, ownerFirst, ownerSecond, 1, 5);
         subscriptionRegion = createRegion(subscriber, subscriberFirst, subscriberSecond, 7, 9);
         sourcePair = association(owner, JavaTckSupport.attributeSet(owner, ownerExplicit),
            sourceRegion);
         subscriptionPair = association(subscriber,
            JavaTckSupport.attributeSet(subscriber, subscriberOmitted), subscriptionRegion);
         subscriber.subscribeObjectClassAttributesWithRegions(subscriberClass, subscriptionPair);
         subscribed = true;

         object = owner.registerObjectInstanceWithRegions(ownerClass, sourcePair);
         sourceAssociated = true;
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "region-qualified registration returned an invalid object handle");
         ObjectInstanceHandle registered = object;
         await(owner, subscriber, callbackModel,
            () -> callbacks.discoveries.contains(registered),
            "discovery through an omitted attribute's default region");
         JavaTckSupport.check(callbacks.discoveries.size() == 1,
            "omitted-attribute default-region discovery produced duplicate callbacks");

         byte[] value = new byte[] {0x73, 0x52, 0x31};
         byte[] tag = new byte[] {0x44, 0x52};
         update(owner, object, ownerOmitted, value, tag);
         await(owner, subscriber, callbackModel, () -> !callbacks.reflections.isEmpty(),
            "reflection through the omitted attribute's default region");
         JavaTckSupport.check(callbacks.reflections.size() == 1,
            "omitted-attribute default-region update produced duplicate reflections");
         Reflection reflection = callbacks.reflections.get(0);
         JavaTckSupport.check(object.equals(reflection.object) && reflection.values.size() == 1
               && Arrays.equals(value, reflection.values.get(subscriberOmitted))
               && Arrays.equals(tag, reflection.tag) && producer.equals(reflection.producer),
            "default-region reflection changed the object, omitted attribute value, tag, or producer");
      } finally {
         if (ownerJoined && sourceAssociated && object != null && sourcePair != null) try {
            owner.unassociateRegionsForUpdates(object, sourcePair);
         } catch (Exception ignored) { }
         if (subscriberJoined && subscribed && subscriptionPair != null) try {
            subscriber.unsubscribeObjectClassAttributesWithRegions(subscriberClass, subscriptionPair);
         } catch (Exception ignored) { }
         if (ownerJoined && published && ownerAttributes != null) try {
            owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes);
         } catch (Exception ignored) { }
         if (ownerJoined && sourceRegion != null) deleteRegion(owner, sourceRegion);
         if (subscriberJoined && subscriptionRegion != null) deleteRegion(subscriber, subscriptionRegion);
         if (subscriberJoined) try { subscriber.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (ownerJoined) try { owner.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         if (created) try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (subscriberConnected) try { subscriber.disconnect(); } catch (Exception ignored) { }
         if (ownerConnected) try { owner.disconnect(); } catch (Exception ignored) { }
      }
   }

   static void registrationEmptyRegionSetDefaultRegion(RtiFactory factory, String fom,
         String objectClassName, String attributeName, String firstDimensionName,
         String secondDimensionName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder callbacks = new Recorder();
      String federation = "java-tck-registration-empty-region-set-default-region-"
         + UUID.randomUUID();
      boolean ownerConnected = false, subscriberConnected = false, created = false;
      boolean ownerJoined = false, subscriberJoined = false, published = false, subscribed = false;
      ObjectClassHandle ownerClass = null, subscriberClass = null;
      AttributeHandle ownerAttribute = null, subscriberAttribute = null;
      AttributeHandleSet ownerAttributes = null, subscriberAttributes = null;
      AttributeSetRegionSetPairList sourcePair = null, subscriptionPair = null;
      RegionHandle subscriptionRegion = null;
      ObjectInstanceHandle object = null;
      try {
         owner.connect(noopFederateAmbassador(), callbackModel);
         ownerConnected = true;
         subscriber.connect(callbacks.proxy(), callbackModel);
         subscriberConnected = true;
         owner.createFederationExecution(federation, fom);
         created = true;
         FederateHandle producer = owner.joinFederationExecution(
            "java-tck-registration-empty-regions-owner", federation);
         ownerJoined = true;
         subscriber.joinFederationExecution(
            "java-tck-registration-empty-regions-subscriber", federation);
         subscriberJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         subscriberClass = subscriber.getObjectClassHandle(objectClassName);
         ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         subscriberAttribute = subscriber.getAttributeHandle(subscriberClass, attributeName);
         DimensionHandle ownerFirst = owner.getDimensionHandle(firstDimensionName);
         DimensionHandle ownerSecond = owner.getDimensionHandle(secondDimensionName);
         DimensionHandle subscriberFirst = subscriber.getDimensionHandle(firstDimensionName);
         DimensionHandle subscriberSecond = subscriber.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(ownerClass.equals(subscriberClass)
               && ownerAttribute.equals(subscriberAttribute)
               && ownerFirst.equals(subscriberFirst) && ownerSecond.equals(subscriberSecond),
            "adapter multi-attribute DDM handles changed across empty-region-set federates");
         checkDimensions(owner, ownerClass, ownerFirst, ownerSecond);
         checkDimensions(subscriber, subscriberClass, subscriberFirst, subscriberSecond);

         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         subscriberAttributes = JavaTckSupport.attributeSet(subscriber, subscriberAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         published = true;
         subscriptionRegion = createRegion(subscriber, subscriberFirst, subscriberSecond, 7, 9);
         RegionHandleSet emptyRegions = owner.getRegionHandleSetFactory().create();
         sourcePair = owner.getAttributeSetRegionSetPairListFactory().create(1);
         sourcePair.add(new AttributeRegionAssociation(ownerAttributes, emptyRegions));
         subscriptionPair = association(subscriber, subscriberAttributes, subscriptionRegion);
         subscriber.subscribeObjectClassAttributesWithRegions(subscriberClass, subscriptionPair);
         subscribed = true;

         object = owner.registerObjectInstanceWithRegions(ownerClass, sourcePair);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "empty-region-set registration returned an invalid object handle");
         ObjectInstanceHandle registered = object;
         await(owner, subscriber, callbackModel,
            () -> callbacks.discoveries.contains(registered),
            "discovery through an empty region set's default-region association");
         JavaTckSupport.check(callbacks.discoveries.size() == 1,
            "empty-region-set default-region discovery produced duplicate callbacks");

         byte[] value = new byte[] {0x45, 0x52, 0x53};
         byte[] tag = new byte[] {0x45, 0x52};
         update(owner, object, ownerAttribute, value, tag);
         await(owner, subscriber, callbackModel, () -> !callbacks.reflections.isEmpty(),
            "reflection through an empty region set's default-region association");
         JavaTckSupport.check(callbacks.reflections.size() == 1,
            "empty-region-set default-region update produced duplicate reflections");
         Reflection reflection = callbacks.reflections.get(0);
         JavaTckSupport.check(object.equals(reflection.object) && reflection.values.size() == 1
               && Arrays.equals(value, reflection.values.get(subscriberAttribute))
               && Arrays.equals(tag, reflection.tag) && producer.equals(reflection.producer),
            "empty-region-set reflection changed the object, attribute value, tag, or producer");
      } finally {
         if (subscriberJoined && subscribed && subscriptionPair != null) try {
            subscriber.unsubscribeObjectClassAttributesWithRegions(subscriberClass, subscriptionPair);
         } catch (Exception ignored) { }
         if (ownerJoined && published && ownerAttributes != null) try {
            owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes);
         } catch (Exception ignored) { }
         if (subscriberJoined && subscriptionRegion != null) deleteRegion(subscriber, subscriptionRegion);
         if (subscriberJoined) try { subscriber.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (ownerJoined) try { owner.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         if (created) try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (subscriberConnected) try { subscriber.disconnect(); } catch (Exception ignored) { }
         if (ownerConnected) try { owner.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static void checkDimensions(RTIambassador ambassador, ObjectClassHandle objectClass,
         DimensionHandle first, DimensionHandle second) throws Exception {
      DimensionHandleSet available = ambassador.getAvailableDimensionsForObjectClass(objectClass);
      JavaTckSupport.check(available.contains(first) && available.contains(second)
            && !first.equals(second) && ambassador.getDimensionUpperBound(first) >= 9
            && ambassador.getDimensionUpperBound(second) >= 9,
         "adapter object class must expose both DDM dimensions through range 9");
   }

   private static long[] defaultRoutingDimensionBounds(RTIambassador ambassador,
         ObjectClassHandle objectClass, DimensionHandle first, DimensionHandle second,
         String firstName, String secondName) throws Exception {
      DimensionHandleSet available = ambassador.getAvailableDimensionsForObjectClass(objectClass);
      JavaTckSupport.check(available.contains(first) && available.contains(second)
            && !first.equals(second) && firstName.equals(ambassador.getDimensionName(first))
            && secondName.equals(ambassador.getDimensionName(second)),
         "adapter object class must expose the configured default-region dimensions");
      return new long[] {
         ambassador.getDimensionUpperBound(first), ambassador.getDimensionUpperBound(second)
      };
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle first,
         DimensionHandle second, long lower, long upper) throws Exception {
      return createRegion(ambassador, first, second, lower, upper, lower, upper);
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle first,
         DimensionHandle second, long firstLower, long firstUpper,
         long secondLower, long secondUpper) throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(first); dimensions.add(second);
      RegionHandle region = ambassador.createRegion(dimensions);
      ambassador.setRangeBounds(region, first,
         new hla.rti1516_2025.RangeBounds(firstLower, firstUpper));
      ambassador.setRangeBounds(region, second,
         new hla.rti1516_2025.RangeBounds(secondLower, secondUpper));
      RegionHandleSet regionSet = ambassador.getRegionHandleSetFactory().create();
      regionSet.add(region);
      ambassador.commitRegionModifications(regionSet);
      return region;
   }

   private static void verifyRegion(RTIambassador ambassador, RegionHandle region,
         DimensionHandle first, DimensionHandle second, long firstLower, long firstUpper,
         long secondLower, long secondUpper, String description) throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSet(region);
      RangeBounds firstBounds = ambassador.getRangeBounds(region, first);
      RangeBounds secondBounds = ambassador.getRangeBounds(region, second);
      JavaTckSupport.check(dimensions.size() == 2 && dimensions.contains(first)
            && dimensions.contains(second) && firstBounds.lower == firstLower
            && firstBounds.upper == firstUpper && secondBounds.lower == secondLower
            && secondBounds.upper == secondUpper,
         description + " returned the wrong DDM dimensions or range bounds");
   }

   private static AttributeSetRegionSetPairList association(RTIambassador ambassador,
         AttributeHandleSet attributes, RegionHandle region) throws Exception {
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      regions.add(region);
      AttributeSetRegionSetPairList result =
         ambassador.getAttributeSetRegionSetPairListFactory().create(1);
      result.add(new AttributeRegionAssociation(attributes, regions));
      return result;
   }

   private static void update(RTIambassador ambassador, ObjectInstanceHandle object,
         AttributeHandle attribute, byte[] value, byte[] tag) throws Exception {
      AttributeHandleValueMap values = ambassador.getAttributeHandleValueMapFactory().create(1);
      values.put(attribute, value.clone());
      ambassador.updateAttributeValues(object, values, tag.clone());
   }

   private static void assertReflection(Reflection reflection, RTIambassador receiver,
         ObjectInstanceHandle object, AttributeHandle attribute, String producerName,
         byte[] value, byte[] tag, long[] sourceBounds, String description) throws Exception {
      JavaTckSupport.check(object.equals(reflection.object) && reflection.values.size() == 1
            && Arrays.equals(value, reflection.values.get(attribute))
            && Arrays.equals(tag, reflection.tag)
            && producerName.equals(receiver.getFederateName(reflection.producer)),
         description + " changed object, attribute value, tag, or producer");
      JavaTckSupport.check(reflection.transportation != null
            && reflection.transportation.encodedLength() > 0,
         description + " returned an invalid transportation handle");
      JavaTckSupport.check(reflection.regionSetPresent
            && (sourceBounds == null ? reflection.regionCount == 0
               : reflection.regionCount == 1 && reflection.regionInspectionFailure == null
                  && reflection.regionDimensionsMatch
                  && reflection.firstSourceBounds != null && reflection.secondSourceBounds != null
                  && reflection.firstSourceBounds.lower == sourceBounds[0]
                  && reflection.firstSourceBounds.upper == sourceBounds[1]
                  && reflection.secondSourceBounds.lower == sourceBounds[2]
                  && reflection.secondSourceBounds.upper == sourceBounds[3]),
         description + " returned incorrect source-region metadata");
   }

   private static void await(RTIambassador first, RTIambassador second, CallbackModel model,
         java.util.function.BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         pump(first, model); pump(second, model); Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void pumpSeveral(RTIambassador first, RTIambassador second, CallbackModel model)
         throws Exception {
      for (int pass = 0; pass < 8; ++pass) { pump(first, model); pump(second, model); }
   }

   private static void settle(RTIambassador first, RTIambassador second, CallbackModel model)
         throws Exception {
      if (model == CallbackModel.HLA_EVOKED) {
         pumpSeveral(first, second, model);
         return;
      }
      for (int pass = 0; pass < 32; ++pass) {
         Thread.yield();
         Thread.sleep(1L);
      }
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
               if ("toString".equals(method.getName())) return "Java TCK default-region object publisher";
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
      private boolean regionSetPresent;
      private int regionCount;
      private boolean regionDimensionsMatch;
      private RangeBounds firstSourceBounds;
      private RangeBounds secondSourceBounds;
      private String regionInspectionFailure;
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<String> discoveryNames = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private volatile RTIambassador regionInspector;
      private DimensionHandle firstDimension;
      private DimensionHandle secondDimension;

      private void inspectSourceRegions(RTIambassador inspector, DimensionHandle first,
            DimensionHandle second) {
         firstDimension = first;
         secondDimension = second;
         regionInspector = inspector;
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class}, (proxy, method, args) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && args != null && args.length >= 3) {
                  discoveries.add((ObjectInstanceHandle) args[0]);
                  discoveryNames.add((String) args[2]);
               } else if ("reflectAttributeValues".equals(name) && args != null && args.length == 6) {
                  Reflection reflection = new Reflection();
                  reflection.object = (ObjectInstanceHandle) args[0];
                  reflection.values = ((AttributeHandleValueMap) args[1]).clone();
                  reflection.tag = ((byte[]) args[2]).clone();
                  reflection.transportation = (TransportationTypeHandle) args[3];
                  reflection.producer = (FederateHandle) args[4];
                  reflection.regions = (RegionHandleSet) args[5];
                  reflection.regionSetPresent = reflection.regions != null;
                  reflection.regionCount = reflection.regions == null ? -1 : reflection.regions.size();
                  if (regionInspector != null && reflection.regionCount == 1) {
                     try {
                        RegionHandle source = reflection.regions.iterator().next();
                        DimensionHandleSet dimensions = regionInspector.getDimensionHandleSet(source);
                        reflection.regionDimensionsMatch = dimensions.size() == 2
                           && dimensions.contains(firstDimension) && dimensions.contains(secondDimension);
                        reflection.firstSourceBounds = regionInspector.getRangeBounds(source, firstDimension);
                        reflection.secondSourceBounds = regionInspector.getRangeBounds(source, secondDimension);
                     } catch (Exception failure) {
                        reflection.regionInspectionFailure = failure.getClass().getSimpleName()
                           + ": " + failure.getMessage();
                     }
                  }
                  reflections.add(reflection);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK default-region object callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (args == null ? null : args[0]);
               }
               return null;
            });
      }
   }
}
