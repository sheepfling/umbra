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
      boolean publisherConnected = false, regionalConnected = false, mixedConnected = false;
      boolean created = false, publisherJoined = false, regionalJoined = false, mixedJoined = false;
      boolean published = false, regionalSubscribed = false, mixedOrdinarySubscribed = false;
      boolean mixedRegionalSubscribed = false, sourceAssociated = false;
      boolean sourceCreated = false, regionalRegionCreated = false, mixedRegionCreated = false;
      ObjectClassHandle publisherClass = null, regionalClass = null, mixedClass = null;
      AttributeHandle publisherAttribute = null, regionalAttribute = null, mixedAttribute = null;
      AttributeHandleSet publisherAttributes = null, regionalAttributes = null, mixedAttributes = null;
      AttributeSetRegionSetPairList sourcePair = null, regionalPair = null, mixedPair = null;
      RegionHandle sourceRegion = null, regionalRegion = null, mixedRegion = null;
      ObjectInstanceHandle object = null;
      try {
         publisher.connect(noopFederateAmbassador(), callbackModel); publisherConnected = true;
         regional.connect(regionalCallbacks.proxy(), callbackModel); regionalConnected = true;
         mixed.connect(mixedCallbacks.proxy(), callbackModel); mixedConnected = true;
         publisher.createFederationExecution(federation, fom); created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "default-region-object-publisher", federation); publisherJoined = true;
         regional.joinFederationExecution("default-region-object-regional", federation);
         regionalJoined = true;
         mixed.joinFederationExecution("default-region-object-mixed", federation); mixedJoined = true;

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
         JavaTckSupport.check(publisherClass.equals(regionalClass)
               && publisherClass.equals(mixedClass)
               && publisherAttribute.equals(regionalAttribute)
               && publisherAttribute.equals(mixedAttribute)
               && publisherFirst.equals(regionalFirst) && publisherSecond.equals(regionalSecond)
               && publisherFirst.equals(mixedFirst) && publisherSecond.equals(mixedSecond),
            "adapter DDM FOM handles changed across default-region object federates");
         checkDimensions(publisher, publisherClass, publisherFirst, publisherSecond);
         checkDimensions(regional, regionalClass, regionalFirst, regionalSecond);
         checkDimensions(mixed, mixedClass, mixedFirst, mixedSecond);

         publisherAttributes = JavaTckSupport.attributeSet(publisher, publisherAttribute);
         regionalAttributes = JavaTckSupport.attributeSet(regional, regionalAttribute);
         mixedAttributes = JavaTckSupport.attributeSet(mixed, mixedAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         sourceRegion = createRegion(publisher, publisherFirst, publisherSecond, 1, 5);
         sourceCreated = true;
         regionalRegion = createRegion(regional, regionalFirst, regionalSecond, 2, 6);
         regionalRegionCreated = true;
         mixedRegion = createRegion(mixed, mixedFirst, mixedSecond, 7, 9);
         mixedRegionCreated = true;
         sourcePair = association(publisher, publisherAttributes, sourceRegion);
         regionalPair = association(regional, regionalAttributes, regionalRegion);
         mixedPair = association(mixed, mixedAttributes, mixedRegion);

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
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "default-region registration returned an invalid object handle");
         ObjectInstanceHandle registered = object;
         await(regional, mixed, callbackModel,
            () -> regionalCallbacks.discoveries.contains(registered), "regional object discovery");
         pumpSeveral(regional, mixed, callbackModel);
         JavaTckSupport.check(!mixedCallbacks.discoveries.contains(object),
            "disjoint regional declaration did not mask the ordinary subscription");

         mixed.unsubscribeObjectClassAttributesWithRegions(mixedClass, mixedPair);
         mixedRegionalSubscribed = false;
         mixed.subscribeObjectClassAttributes(mixedClass, mixedAttributes);
         mixedOrdinarySubscribed = true;
         await(regional, mixed, callbackModel,
            () -> mixedCallbacks.discoveries.contains(registered),
            "ordinary subscription discovery through the default source region");
         JavaTckSupport.check(mixedCallbacks.discoveries.size() == 1,
            "ordinary default-region discovery produced duplicate callbacks");

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
         assertReflection(regionalCallbacks.reflections.get(0), object, regionalAttribute,
            producer, defaultValue, defaultTag, null, "regional default-source reflection");
         assertReflection(mixedCallbacks.reflections.get(0), object, mixedAttribute,
            producer, defaultValue, defaultTag, null, "mixed default-source reflection");

         publisher.associateRegionsForUpdates(object, sourcePair);
         sourceAssociated = true;
         byte[] explicitValue = new byte[] {(byte) 0xe0, 0x02};
         byte[] explicitTag = new byte[] {0x44, 0x32};
         update(publisher, object, publisherAttribute, explicitValue, explicitTag);
         await(regional, mixed, callbackModel,
            () -> regionalCallbacks.reflections.size() >= 2,
            "explicit-source regional attribute reflection");
         pumpSeveral(regional, mixed, callbackModel);
         assertReflection(regionalCallbacks.reflections.get(1), object, regionalAttribute,
            producer, explicitValue, explicitTag, sourceRegion, "explicit-source reflection");
         JavaTckSupport.check(mixedCallbacks.reflections.size() == 1,
            "disjoint explicit subscription received an explicit-source update");

         publisher.unassociateRegionsForUpdates(object, sourcePair);
         sourceAssociated = false;
         byte[] restoredValue = new byte[] {(byte) 0xd0, 0x03};
         byte[] restoredTag = new byte[] {0x44, 0x33};
         update(publisher, object, publisherAttribute, restoredValue, restoredTag);
         await(regional, mixed, callbackModel,
            () -> regionalCallbacks.reflections.size() >= 3 && mixedCallbacks.reflections.size() >= 2,
            "restored default-source reflection");
         assertReflection(regionalCallbacks.reflections.get(2), object, regionalAttribute,
            producer, restoredValue, restoredTag, null, "regional restored default-source reflection");
         assertReflection(mixedCallbacks.reflections.get(1), object, mixedAttribute,
            producer, restoredValue, restoredTag, null, "mixed restored default-source reflection");

         mixed.unsubscribeObjectClassAttributesWithRegions(mixedClass, mixedPair);
         mixedRegionalSubscribed = false;
         mixed.unsubscribeObjectClassAttributes(mixedClass, mixedAttributes);
         mixedOrdinarySubscribed = false;
         regional.unsubscribeObjectClassAttributesWithRegions(regionalClass, regionalPair);
         regionalSubscribed = false;
         publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         published = false;
         deleteRegion(publisher, sourceRegion); sourceCreated = false;
         deleteRegion(regional, regionalRegion); regionalRegionCreated = false;
         deleteRegion(mixed, mixedRegion); mixedRegionCreated = false;
         publisher.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         publisherJoined = false;
         regional.resignFederationExecution(ResignAction.NO_ACTION); regionalJoined = false;
         mixed.resignFederationExecution(ResignAction.NO_ACTION); mixedJoined = false;
         publisher.destroyFederationExecution(federation); created = false;
      } finally {
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
         if (regionalJoined) deleteRegion(regional, regionalRegion, regionalRegionCreated);
         if (mixedJoined) deleteRegion(mixed, mixedRegion, mixedRegionCreated);
         if (mixedJoined) try { mixed.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (regionalJoined) try { regional.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (publisherJoined) try {
            publisher.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
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

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle first,
         DimensionHandle second, long lower, long upper) throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(first); dimensions.add(second);
      RegionHandle region = ambassador.createRegion(dimensions);
      ambassador.setRangeBounds(region, first, new hla.rti1516_2025.RangeBounds(lower, upper));
      ambassador.setRangeBounds(region, second, new hla.rti1516_2025.RangeBounds(lower, upper));
      RegionHandleSet regionSet = ambassador.getRegionHandleSetFactory().create();
      regionSet.add(region);
      ambassador.commitRegionModifications(regionSet);
      return region;
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

   private static void assertReflection(Reflection reflection, ObjectInstanceHandle object,
         AttributeHandle attribute, FederateHandle producer, byte[] value, byte[] tag,
         RegionHandle sourceRegion, String description) throws Exception {
      JavaTckSupport.check(object.equals(reflection.object) && reflection.values.size() == 1
            && Arrays.equals(value, reflection.values.get(attribute))
            && Arrays.equals(tag, reflection.tag) && producer.equals(reflection.producer),
         description + " changed object, attribute value, tag, or producer");
      JavaTckSupport.check(reflection.transportation != null,
         description + " returned an invalid transportation handle");
      JavaTckSupport.check(reflection.regions != null
            && (sourceRegion == null ? reflection.regions.isEmpty()
               : reflection.regions.size() == 1 && reflection.regions.contains(sourceRegion)),
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
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class}, (proxy, method, args) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && args != null) {
                  discoveries.add((ObjectInstanceHandle) args[0]);
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
                  if ("toString".equals(name)) return "Java TCK default-region object callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (args == null ? null : args[0]);
               }
               return null;
            });
      }
   }
}
