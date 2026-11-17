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

/** Re-evaluates direct regional update relevance when a callback is serviced. */
final class RegionalAttributeUpdateCallbackDdmRecheckTck {
   private static final long TIMEOUT_NANOS = 10_000_000_000L;

   private RegionalAttributeUpdateCallbackDdmRecheckTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName, String attributeName,
         String firstDimensionName, String secondDimensionName, String unrelatedDimensionName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador active = factory.getRtiAmbassador();
      RTIambassador rechecked = factory.getRtiAmbassador();
      RTIambassador emptySubscriber = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder activeCallbacks = new Recorder();
      Recorder recheckedCallbacks = new Recorder();
      Recorder emptySubscriberCallbacks = new Recorder();
      String federation = "java-tck-regional-callback-ddm-recheck-" + UUID.randomUUID();
      boolean publisherConnected = false, activeConnected = false, recheckedConnected = false;
      boolean emptySubscriberConnected = false;
      boolean created = false, publisherJoined = false, activeJoined = false, recheckedJoined = false;
      boolean emptySubscriberJoined = false;
      boolean published = false, activeSubscribed = false, recheckedSubscribed = false;
      boolean emptySubscriberSubscribed = false;
      ObjectClassHandle publisherClass = null, activeClass = null, recheckedClass = null;
      ObjectClassHandle emptySubscriberClass = null;
      AttributeHandle publisherAttribute = null, activeAttribute = null, recheckedAttribute = null;
      AttributeHandle emptySubscriberAttribute = null;
      AttributeHandleSet publisherAttributes = null, activeAttributes = null, recheckedAttributes = null;
      AttributeHandleSet emptySubscriberAttributes = null;
      AttributeSetRegionSetPairList publisherPair = null, activePair = null, recheckedPair = null;
      AttributeSetRegionSetPairList emptySourcePair = null;
      AttributeSetRegionSetPairList activeEmptyPair = null;
      AttributeSetRegionSetPairList emptySubscriberPair = null;
      RegionHandle sourceRegion = null, activeRegion = null, recheckedRegion = null;
      ObjectInstanceHandle object = null;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         active.connect(activeCallbacks.proxy(), callbackModel);
         activeConnected = true;
         rechecked.connect(recheckedCallbacks.proxy(), callbackModel);
         recheckedConnected = true;
         emptySubscriber.connect(emptySubscriberCallbacks.proxy(), callbackModel);
         emptySubscriberConnected = true;
         publisher.createFederationExecution(federation, fom);
         created = true;
         FederateHandle producingFederate = publisher.joinFederationExecution(
            "java-tck-regional-callback-publisher", federation);
         publisherJoined = true;
         String producingFederateName = publisher.getFederateName(producingFederate);
         JavaTckSupport.check(producingFederateName != null && !producingFederateName.isEmpty(),
            "regional publisher handle did not resolve to a federate name");
         active.joinFederationExecution("java-tck-regional-callback-active", federation);
         activeJoined = true;
         rechecked.joinFederationExecution("java-tck-regional-callback-rechecked", federation);
         recheckedJoined = true;
         emptySubscriber.joinFederationExecution("java-tck-regional-callback-empty", federation);
         emptySubscriberJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         activeClass = active.getObjectClassHandle(objectClassName);
         recheckedClass = rechecked.getObjectClassHandle(objectClassName);
         emptySubscriberClass = emptySubscriber.getObjectClassHandle(objectClassName);
         publisherAttribute = publisher.getAttributeHandle(publisherClass, attributeName);
         activeAttribute = active.getAttributeHandle(activeClass, attributeName);
         recheckedAttribute = rechecked.getAttributeHandle(recheckedClass, attributeName);
         emptySubscriberAttribute = emptySubscriber.getAttributeHandle(emptySubscriberClass, attributeName);
         DimensionHandle publisherFirst = publisher.getDimensionHandle(firstDimensionName);
         DimensionHandle publisherSecond = publisher.getDimensionHandle(secondDimensionName);
         DimensionHandle activeFirst = active.getDimensionHandle(firstDimensionName);
         DimensionHandle activeSecond = active.getDimensionHandle(secondDimensionName);
         DimensionHandle recheckedFirst = rechecked.getDimensionHandle(firstDimensionName);
         DimensionHandle recheckedSecond = rechecked.getDimensionHandle(secondDimensionName);
         DimensionHandle emptySubscriberFirst = emptySubscriber.getDimensionHandle(firstDimensionName);
         DimensionHandle emptySubscriberSecond = emptySubscriber.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(objectClassName.equals(publisher.getObjectClassName(publisherClass))
               && objectClassName.equals(active.getObjectClassName(activeClass))
               && objectClassName.equals(rechecked.getObjectClassName(recheckedClass))
               && objectClassName.equals(emptySubscriber.getObjectClassName(emptySubscriberClass))
               && attributeName.equals(publisher.getAttributeName(publisherClass, publisherAttribute))
               && attributeName.equals(active.getAttributeName(activeClass, activeAttribute))
               && attributeName.equals(rechecked.getAttributeName(recheckedClass, recheckedAttribute))
               && attributeName.equals(emptySubscriber.getAttributeName(
                  emptySubscriberClass, emptySubscriberAttribute)),
            "adapter DDM FOM returned inconsistent object-class or attribute names");
         checkDimensions(publisher, publisherClass, publisherFirst, publisherSecond,
            firstDimensionName, secondDimensionName);
         checkDimensions(active, activeClass, activeFirst, activeSecond,
            firstDimensionName, secondDimensionName);
         checkDimensions(rechecked, recheckedClass, recheckedFirst, recheckedSecond,
            firstDimensionName, secondDimensionName);
         checkDimensions(emptySubscriber, emptySubscriberClass, emptySubscriberFirst,
            emptySubscriberSecond, firstDimensionName, secondDimensionName);
         activeCallbacks.inspectRegionMetadata(active, activeFirst, activeSecond);
         recheckedCallbacks.inspectRegionMetadata(rechecked, recheckedFirst, recheckedSecond);

         publisherAttributes = JavaTckSupport.attributeSet(publisher, publisherAttribute);
         activeAttributes = JavaTckSupport.attributeSet(active, activeAttribute);
         recheckedAttributes = JavaTckSupport.attributeSet(rechecked, recheckedAttribute);
         emptySubscriberAttributes = JavaTckSupport.attributeSet(emptySubscriber, emptySubscriberAttribute);
         requireInvalidDimensionSubscriptionRejected(
            rechecked, recheckedClass, recheckedAttributes, unrelatedDimensionName);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         sourceRegion = createRegion(publisher, publisherFirst, publisherSecond, 1, 5);
         activeRegion = createRegion(active, activeFirst, activeSecond, 2, 6);
         recheckedRegion = createRegion(rechecked, recheckedFirst, recheckedSecond, 2, 6);
         publisherPair = association(publisher, publisherAttributes, sourceRegion);
         activePair = association(active, activeAttributes, activeRegion);
         recheckedPair = association(rechecked, recheckedAttributes, recheckedRegion);
         activeEmptyPair = emptyAssociation(active, activeAttributes);
         emptySubscriberPair = emptyAssociation(emptySubscriber, emptySubscriberAttributes);
         active.setConveyRegionDesignatorSetsSwitch(true);
         rechecked.setConveyRegionDesignatorSetsSwitch(true);
         emptySubscriber.setConveyRegionDesignatorSetsSwitch(true);
         active.subscribeObjectClassAttributesWithRegions(activeClass, activePair);
         activeSubscribed = true;
         rechecked.subscribeObjectClassAttributesWithRegions(recheckedClass, recheckedPair);
         recheckedSubscribed = true;
         emptySubscriber.subscribeObjectClassAttributesWithRegions(
            emptySubscriberClass, emptySubscriberPair);
         emptySubscriberSubscribed = true;
         object = publisher.registerObjectInstanceWithRegions(publisherClass, publisherPair);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "regional callback DDM registration returned an invalid object handle");
         String objectName = publisher.getObjectInstanceName(object);
         JavaTckSupport.check(objectName != null && !objectName.isEmpty()
               && object.equals(publisher.getObjectInstanceHandle(objectName)),
            "regional callback DDM registration did not preserve its publisher-local object name");
         emptySourcePair = emptyAssociation(publisher, publisherAttributes);
         publisher.associateRegionsForUpdates(object, emptySourcePair);
         await(publisher, active, rechecked, callbackModel,
            () -> activeCallbacks.hasDiscovery(objectName)
               && recheckedCallbacks.hasDiscovery(objectName),
            "regional object discovery");
         ObjectInstanceHandle activeObject = active.getObjectInstanceHandle(objectName);
         ObjectInstanceHandle recheckedObject = rechecked.getObjectInstanceHandle(objectName);
         Discovery activeDiscovery = activeCallbacks.discovery(objectName);
         Discovery recheckedDiscovery = recheckedCallbacks.discovery(objectName);
         JavaTckSupport.check(activeDiscovery != null && activeDiscovery.object.equals(activeObject)
               && activeDiscovery.objectClass.equals(activeClass)
               && objectName.equals(active.getObjectInstanceName(activeObject))
               && activeObject.equals(active.getObjectInstanceHandle(objectName))
               && active.getObjectClassName(active.getKnownObjectClassHandle(activeObject))
                  .equals(objectClassName)
               && recheckedDiscovery != null && recheckedDiscovery.object.equals(recheckedObject)
               && recheckedDiscovery.objectClass.equals(recheckedClass)
               && objectName.equals(rechecked.getObjectInstanceName(recheckedObject))
               && recheckedObject.equals(rechecked.getObjectInstanceHandle(objectName))
               && rechecked.getObjectClassName(
                  rechecked.getKnownObjectClassHandle(recheckedObject)).equals(objectClassName),
            "regional callback DDM discovery changed a receiver-local object or class name");
         requireNoEmptyRegionCallbacks(emptySubscriber, emptySubscriberCallbacks,
            objectName, callbackModel, "after regional object registration");

         activeCallbacks.reflections.clear();
         recheckedCallbacks.reflections.clear();
         byte[] firstValue = new byte[] {0x41, 0x31};
         byte[] firstTag = new byte[] {0x52, 0x31};
         update(publisher, object, publisherAttribute, firstValue, firstTag);
         requireNoEmptyRegionCallbacks(emptySubscriber, emptySubscriberCallbacks,
            objectName, callbackModel, "after the first regional update");
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(activeCallbacks.reflections.isEmpty()
                  && recheckedCallbacks.reflections.isEmpty(),
               "evoked regional update arrived before callback servicing");
            moveRegion(rechecked, recheckedRegion, recheckedFirst, recheckedSecond, false);
            await(publisher, active, rechecked, callbackModel,
               () -> activeCallbacks.reflections.size() >= 1,
               "active receiver reflection after region mutation");
            for (int pass = 0; pass < 8; ++pass) pump(rechecked, callbackModel);
            JavaTckSupport.check(recheckedCallbacks.reflections.isEmpty(),
               "evoked regional callback delivered a stale disjoint reflection");
         } else {
            await(publisher, active, rechecked, callbackModel,
               () -> activeCallbacks.reflections.size() >= 1
                  && recheckedCallbacks.reflections.size() >= 1,
               "immediate regional reflections before region mutation");
            moveRegion(rechecked, recheckedRegion, recheckedFirst, recheckedSecond, false);
         }
         requireReflection(active, activeCallbacks.reflections, activeObject, objectName, activeAttribute,
            firstValue, firstTag, producingFederateName, activeFirst, activeSecond,
            "active receiver's first regional reflection");
         if (callbackModel == CallbackModel.HLA_IMMEDIATE) {
            requireReflection(rechecked, recheckedCallbacks.reflections, recheckedObject, objectName, recheckedAttribute,
               firstValue, firstTag, producingFederateName, recheckedFirst, recheckedSecond,
               "immediate rechecked receiver's first reflection");
         }
         JavaTckSupport.check(publisherCallbacks.reflections.isEmpty(),
            "regional attribute callback DDM update looped a reflection to the publisher");

         publisher.unassociateRegionsForUpdates(object, emptySourcePair);
         activeCallbacks.reflections.clear();
         recheckedCallbacks.reflections.clear();
         byte[] secondValue = new byte[] {0x41, 0x32};
         byte[] secondTag = new byte[] {0x52, 0x32};
         update(publisher, object, publisherAttribute, secondValue, secondTag);
         requireNoEmptyRegionCallbacks(emptySubscriber, emptySubscriberCallbacks,
            objectName, callbackModel, "after the second regional update");
         await(publisher, active, rechecked, callbackModel,
            () -> activeCallbacks.reflections.size() >= 1,
            "active receiver reflection while other region is disjoint");
         for (int pass = 0; pass < 8; ++pass) pump(rechecked, callbackModel);
         JavaTckSupport.check(recheckedCallbacks.reflections.isEmpty(),
            "regional callback delivered while the receiver region was disjoint");
         requireReflection(active, activeCallbacks.reflections, activeObject, objectName, activeAttribute,
            secondValue, secondTag, producingFederateName, activeFirst, activeSecond,
            "active receiver's second regional reflection");

         moveRegion(rechecked, recheckedRegion, recheckedFirst, recheckedSecond, true);
         activeCallbacks.reflections.clear();
         recheckedCallbacks.reflections.clear();
         byte[] restoredValue = new byte[] {0x41, 0x33};
         byte[] restoredTag = new byte[] {0x52, 0x33};
         update(publisher, object, publisherAttribute, restoredValue, restoredTag);
         requireNoEmptyRegionCallbacks(emptySubscriber, emptySubscriberCallbacks,
            objectName, callbackModel, "after the restored-overlap regional update");
         await(publisher, active, rechecked, callbackModel,
            () -> activeCallbacks.reflections.size() >= 1
               && recheckedCallbacks.reflections.size() >= 1,
            "regional reflection after restoring overlap");
         requireReflection(active, activeCallbacks.reflections, activeObject, objectName, activeAttribute,
            restoredValue, restoredTag, producingFederateName, activeFirst, activeSecond,
            "active receiver's restored-overlap reflection");
         requireReflection(rechecked, recheckedCallbacks.reflections, recheckedObject, objectName, recheckedAttribute,
            restoredValue, restoredTag, producingFederateName, recheckedFirst, recheckedSecond,
            "rechecked receiver's restored-overlap reflection");

         moveRegion(active, activeRegion, activeFirst, activeSecond, false);
         moveRegion(rechecked, recheckedRegion, recheckedFirst, recheckedSecond, false);
         activeCallbacks.reflections.clear();
         recheckedCallbacks.reflections.clear();
         publisher.unassociateRegionsForUpdates(object, publisherPair);
         byte[] unassociatedValue = new byte[] {0x41, 0x34};
         byte[] unassociatedTag = new byte[] {0x52, 0x34};
         update(publisher, object, publisherAttribute, unassociatedValue, unassociatedTag);
         requireNoEmptyRegionCallbacks(emptySubscriber, emptySubscriberCallbacks,
            objectName, callbackModel, "after source-region unassociation");
         await(publisher, active, rechecked, callbackModel,
            () -> activeCallbacks.reflections.size() >= 1
               && recheckedCallbacks.reflections.size() >= 1,
            "regional reflections after source-region unassociation");
         requireUnassociatedReflection(active, activeCallbacks.reflections, activeObject, objectName,
            activeAttribute, unassociatedValue, unassociatedTag, producingFederateName,
            "active receiver's post-unassociation reflection");
         requireUnassociatedReflection(rechecked, recheckedCallbacks.reflections, recheckedObject, objectName,
            recheckedAttribute, unassociatedValue, unassociatedTag, producingFederateName,
            "rechecked receiver's post-unassociation reflection");

         rechecked.unsubscribeObjectClassAttributesWithRegions(recheckedClass, recheckedPair);
         recheckedSubscribed = false;
         activeCallbacks.reflections.clear();
         recheckedCallbacks.reflections.clear();
         byte[] unsubscribedValue = new byte[] {0x41, 0x35};
         byte[] unsubscribedTag = new byte[] {0x52, 0x35};
         update(publisher, object, publisherAttribute, unsubscribedValue, unsubscribedTag);
         requireNoEmptyRegionCallbacks(emptySubscriber, emptySubscriberCallbacks,
            objectName, callbackModel, "after regional unsubscription");
         await(publisher, active, rechecked, callbackModel,
            () -> activeCallbacks.reflections.size() >= 1,
            "still-subscribed receiver after regional unsubscription");
         for (int pass = 0; pass < 8; ++pass) pump(rechecked, callbackModel);
         JavaTckSupport.check(recheckedCallbacks.reflections.isEmpty(),
            "regional update reached the receiver after its subscription was removed");
         requireUnassociatedReflection(active, activeCallbacks.reflections, activeObject, objectName, activeAttribute,
            unsubscribedValue, unsubscribedTag, producingFederateName,
            "still-subscribed receiver after regional unsubscription");

         publisher.associateRegionsForUpdates(object, publisherPair);
         moveRegion(active, activeRegion, activeFirst, activeSecond, true);
         moveRegion(rechecked, recheckedRegion, recheckedFirst, recheckedSecond, true);
         rechecked.subscribeObjectClassAttributesWithRegions(recheckedClass, recheckedPair);
         recheckedSubscribed = true;

         active.unsubscribeObjectClassAttributesWithRegions(activeClass, activeEmptyPair);
         activeCallbacks.reflections.clear();
         recheckedCallbacks.reflections.clear();
         byte[] emptyUnsubscribeOverlapValue = new byte[] {0x41, 0x36};
         byte[] emptyUnsubscribeOverlapTag = new byte[] {0x52, 0x36};
         update(publisher, object, publisherAttribute,
            emptyUnsubscribeOverlapValue, emptyUnsubscribeOverlapTag);
         requireNoEmptyRegionCallbacks(emptySubscriber, emptySubscriberCallbacks,
            objectName, callbackModel, "after empty regional unsubscribe with overlap");
         await(publisher, active, rechecked, callbackModel,
            () -> activeCallbacks.reflections.size() >= 1
               && recheckedCallbacks.reflections.size() >= 1,
            "overlap controls after empty regional unsubscribe");
         requireReflectionDelivery(active, activeCallbacks.reflections, activeObject, objectName, activeAttribute,
            emptyUnsubscribeOverlapValue, emptyUnsubscribeOverlapTag, producingFederateName,
            "overlap control after empty regional unsubscribe");
         requireReflection(rechecked, recheckedCallbacks.reflections, recheckedObject, objectName, recheckedAttribute,
            emptyUnsubscribeOverlapValue, emptyUnsubscribeOverlapTag, producingFederateName,
            recheckedFirst, recheckedSecond,
            "rechecked overlap control after empty regional unsubscribe");

         moveRegion(active, activeRegion, activeFirst, activeSecond, false);
         activeCallbacks.reflections.clear();
         recheckedCallbacks.reflections.clear();
         byte[] emptyUnsubscribeDisjointValue = new byte[] {0x41, 0x37};
         byte[] emptyUnsubscribeDisjointTag = new byte[] {0x52, 0x37};
         update(publisher, object, publisherAttribute,
            emptyUnsubscribeDisjointValue, emptyUnsubscribeDisjointTag);
         requireNoEmptyRegionCallbacks(emptySubscriber, emptySubscriberCallbacks,
            objectName, callbackModel, "after empty regional unsubscribe while disjoint");
         await(publisher, active, rechecked, callbackModel,
            () -> recheckedCallbacks.reflections.size() >= 1,
            "rechecked disjoint control after empty regional unsubscribe");
         JavaTckSupport.check(activeCallbacks.reflections.isEmpty(),
            "empty regional unsubscribe removed the active subscription's disjoint filter");
         requireReflection(rechecked, recheckedCallbacks.reflections, recheckedObject, objectName, recheckedAttribute,
            emptyUnsubscribeDisjointValue, emptyUnsubscribeDisjointTag, producingFederateName,
            recheckedFirst, recheckedSecond,
            "rechecked disjoint control after empty regional unsubscribe");

         active.subscribeObjectClassAttributesWithRegions(activeClass, activeEmptyPair);
         activeCallbacks.reflections.clear();
         recheckedCallbacks.reflections.clear();
         byte[] emptySubscribeDisjointValue = new byte[] {0x41, 0x38};
         byte[] emptySubscribeDisjointTag = new byte[] {0x52, 0x38};
         update(publisher, object, publisherAttribute,
            emptySubscribeDisjointValue, emptySubscribeDisjointTag);
         requireNoEmptyRegionCallbacks(emptySubscriber, emptySubscriberCallbacks,
            objectName, callbackModel, "after empty regional subscribe while disjoint");
         await(publisher, active, rechecked, callbackModel,
            () -> recheckedCallbacks.reflections.size() >= 1,
            "rechecked disjoint control after empty regional subscribe");
         JavaTckSupport.check(activeCallbacks.reflections.isEmpty(),
            "empty regional subscribe added a route while the active region was disjoint");
         requireReflection(rechecked, recheckedCallbacks.reflections, recheckedObject, objectName, recheckedAttribute,
            emptySubscribeDisjointValue, emptySubscribeDisjointTag, producingFederateName,
            recheckedFirst, recheckedSecond,
            "rechecked disjoint control after empty regional subscribe");

         moveRegion(active, activeRegion, activeFirst, activeSecond, true);
         activeCallbacks.reflections.clear();
         recheckedCallbacks.reflections.clear();
         byte[] emptySubscribeRestoredValue = new byte[] {0x41, 0x39};
         byte[] emptySubscribeRestoredTag = new byte[] {0x52, 0x39};
         update(publisher, object, publisherAttribute,
            emptySubscribeRestoredValue, emptySubscribeRestoredTag);
         requireNoEmptyRegionCallbacks(emptySubscriber, emptySubscriberCallbacks,
            objectName, callbackModel, "after empty regional subscribe with restored overlap");
         await(publisher, active, rechecked, callbackModel,
            () -> activeCallbacks.reflections.size() >= 1
               && recheckedCallbacks.reflections.size() >= 1,
            "overlap controls after empty regional subscribe");
         requireReflectionDelivery(active, activeCallbacks.reflections, activeObject, objectName, activeAttribute,
            emptySubscribeRestoredValue, emptySubscribeRestoredTag, producingFederateName,
            "active overlap control after empty regional subscribe");
         requireReflection(rechecked, recheckedCallbacks.reflections, recheckedObject, objectName, recheckedAttribute,
            emptySubscribeRestoredValue, emptySubscribeRestoredTag, producingFederateName,
            recheckedFirst, recheckedSecond,
            "rechecked overlap control after empty regional subscribe");

         active.unsubscribeObjectClassAttributesWithRegions(activeClass, activePair);
         activeSubscribed = false;
         rechecked.unsubscribeObjectClassAttributesWithRegions(recheckedClass, recheckedPair);
         recheckedSubscribed = false;
         emptySubscriber.unsubscribeObjectClassAttributesWithRegions(
            emptySubscriberClass, emptySubscriberPair);
         emptySubscriberSubscribed = false;
         publisher.unassociateRegionsForUpdates(object, publisherPair);
         publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         published = false;
         active.deleteRegion(activeRegion); activeRegion = null;
         rechecked.deleteRegion(recheckedRegion); recheckedRegion = null;
         publisher.deleteRegion(sourceRegion); sourceRegion = null;
         active.resignFederationExecution(ResignAction.NO_ACTION);
         activeJoined = false;
         rechecked.resignFederationExecution(ResignAction.NO_ACTION);
         recheckedJoined = false;
         emptySubscriber.resignFederationExecution(ResignAction.NO_ACTION);
         emptySubscriberJoined = false;
         publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (activeSubscribed && activeJoined && activePair != null) try {
            active.unsubscribeObjectClassAttributesWithRegions(activeClass, activePair);
         } catch (Exception ignored) { }
         if (recheckedSubscribed && recheckedJoined && recheckedPair != null) try {
            rechecked.unsubscribeObjectClassAttributesWithRegions(recheckedClass, recheckedPair);
         } catch (Exception ignored) { }
         if (emptySubscriberSubscribed && emptySubscriberJoined && emptySubscriberPair != null) try {
            emptySubscriber.unsubscribeObjectClassAttributesWithRegions(
               emptySubscriberClass, emptySubscriberPair);
         } catch (Exception ignored) { }
         if (published && publisherJoined) try {
            publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         } catch (Exception ignored) { }
         deleteRegion(active, activeRegion, activeJoined);
         deleteRegion(rechecked, recheckedRegion, recheckedJoined);
         deleteRegion(publisher, sourceRegion, publisherJoined);
         if (activeJoined) try { active.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (recheckedJoined) try { rechecked.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (emptySubscriberJoined) try {
            emptySubscriber.resignFederationExecution(ResignAction.NO_ACTION);
         } catch (Exception ignored) { }
         if (publisherJoined) try { publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (publisherConnected) try { publisher.disconnect(); } catch (Exception ignored) { }
         if (activeConnected) try { active.disconnect(); } catch (Exception ignored) { }
         if (recheckedConnected) try { rechecked.disconnect(); } catch (Exception ignored) { }
         if (emptySubscriberConnected) try { emptySubscriber.disconnect(); }
            catch (Exception ignored) { }
      }
   }

   private static void checkDimensions(RTIambassador ambassador, ObjectClassHandle objectClass,
         DimensionHandle first, DimensionHandle second, String firstName, String secondName)
         throws Exception {
      java.util.Set<DimensionHandle> dimensions =
         ambassador.getAvailableDimensionsForObjectClass(objectClass);
      JavaTckSupport.check(dimensions.contains(first) && dimensions.contains(second)
            && !first.equals(second)
            && firstName.equals(ambassador.getDimensionName(first))
            && secondName.equals(ambassador.getDimensionName(second))
            && ambassador.getDimensionUpperBound(first) >= 9
            && ambassador.getDimensionUpperBound(second) >= 9,
         "adapter FOM does not expose the dimensions and bounds required by regional DDM recheck");
   }

   private static void requireInvalidDimensionSubscriptionRejected(RTIambassador ambassador,
         ObjectClassHandle objectClass, AttributeHandleSet attributes, String dimensionName)
         throws Exception {
      DimensionHandle unrelatedDimension = ambassador.getDimensionHandle(dimensionName);
      java.util.Set<DimensionHandle> available =
         ambassador.getAvailableDimensionsForObjectClass(objectClass);
      JavaTckSupport.check(!available.contains(unrelatedDimension),
         "DDM subscription test dimension is available to the target object class");
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(unrelatedDimension);
      RegionHandle region = ambassador.createRegion(dimensions);
      try {
         ambassador.setRangeBounds(region, unrelatedDimension,
            new hla.rti1516_2025.RangeBounds(0L, 1L));
         RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
         regions.add(region);
         ambassador.commitRegionModifications(regions);
         AttributeSetRegionSetPairList pair = association(ambassador, attributes, region);
         boolean rejected = false;
         try {
            ambassador.subscribeObjectClassAttributesWithRegions(objectClass, pair);
         } catch (hla.rti1516_2025.exceptions.InvalidRegionContext expected) {
            rejected = true;
         }
         JavaTckSupport.check(rejected,
            "regional subscription accepted a region with an unavailable dimension");
      } finally {
         deleteRegion(ambassador, region, true);
      }
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle first,
         DimensionHandle second, long lower, long upper) throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(first);
      dimensions.add(second);
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
      AttributeSetRegionSetPairList result =
         ambassador.getAttributeSetRegionSetPairListFactory().create(1);
      result.add(new AttributeRegionAssociation(attributes, regions));
      return result;
   }

   private static AttributeSetRegionSetPairList emptyAssociation(RTIambassador ambassador,
         AttributeHandleSet attributes) throws Exception {
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      AttributeSetRegionSetPairList result =
         ambassador.getAttributeSetRegionSetPairListFactory().create(1);
      result.add(new AttributeRegionAssociation(attributes, regions));
      return result;
   }

   private static void requireNoEmptyRegionCallbacks(RTIambassador ambassador, Recorder callbacks,
         String objectName, CallbackModel model, String phase) throws Exception {
      for (int pass = 0; pass < 8; ++pass) pump(ambassador, model);
      JavaTckSupport.check(!callbacks.hasDiscovery(objectName),
         "empty regional subscription received discovery " + phase);
      JavaTckSupport.check(callbacks.reflections.isEmpty(),
         "empty regional subscription received a reflection " + phase);
   }

   private static void moveRegion(RTIambassador ambassador, RegionHandle region,
         DimensionHandle first, DimensionHandle second, boolean overlap) throws Exception {
      long lower = overlap ? 2 : 7;
      long upper = overlap ? 6 : 9;
      ambassador.setRangeBounds(region, first, new hla.rti1516_2025.RangeBounds(lower, upper));
      ambassador.setRangeBounds(region, second, new hla.rti1516_2025.RangeBounds(lower, upper));
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      regions.add(region);
      ambassador.commitRegionModifications(regions);
   }

   private static void update(RTIambassador publisher, ObjectInstanceHandle object,
         AttributeHandle attribute, byte[] value, byte[] tag) throws Exception {
      AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(1);
      values.put(attribute, value.clone());
      publisher.updateAttributeValues(object, values, tag.clone());
   }

   private static void requireReflection(RTIambassador receiver, List<Reflection> reflections,
         ObjectInstanceHandle object, String objectName, AttributeHandle attribute,
         byte[] value, byte[] tag, String producerName, DimensionHandle firstDimension,
         DimensionHandle secondDimension, String description) throws Exception {
      Reflection reflection = requireReflectionDelivery(receiver, reflections, object, objectName,
         attribute, value, tag, producerName, description);
      JavaTckSupport.check(reflection.regionCount == 1,
         description + " omitted the source-region designator copy");
      JavaTckSupport.check(reflection.regionInspectionError == null
            && reflection.regionDimensions != null && reflection.regionDimensions.size() == 2
            && reflection.regionDimensions.contains(firstDimension)
            && reflection.regionDimensions.contains(secondDimension)
            && reflection.firstBounds != null && reflection.firstBounds.lower == 1L
            && reflection.firstBounds.upper == 5L
            && reflection.secondBounds != null && reflection.secondBounds.lower == 1L
            && reflection.secondBounds.upper == 5L,
         description + " returned a source-region copy with the wrong dimensions or bounds");
   }

   private static void requireUnassociatedReflection(RTIambassador receiver,
         List<Reflection> reflections, ObjectInstanceHandle object, String objectName,
         AttributeHandle attribute, byte[] value, byte[] tag, String producerName,
         String description) throws Exception {
      Reflection reflection = requireReflectionDelivery(receiver, reflections, object, objectName,
         attribute, value, tag, producerName, description);
      JavaTckSupport.check(reflection.regionInspectionError == null
            && !reflection.containsOriginalSourceRegion,
         description + " still exposed the unassociated [1,5] source-region copy");
   }

   private static Reflection requireReflectionDelivery(RTIambassador receiver,
         List<Reflection> reflections, ObjectInstanceHandle object, String objectName,
         AttributeHandle attribute, byte[] value, byte[] tag, String producerName,
         String description) throws Exception {
      JavaTckSupport.check(reflections.size() == 1,
         description + " produced the wrong reflection count");
      Reflection reflection = reflections.get(0);
      JavaTckSupport.check(object.equals(reflection.object)
            && objectName.equals(receiver.getObjectInstanceName(object))
            && object.equals(receiver.getObjectInstanceHandle(objectName))
            && objectName.equals(receiver.getObjectInstanceName(reflection.object))
            && reflection.values.size() == 1
            && Arrays.equals(value, reflection.values.get(attribute))
            && Arrays.equals(tag, reflection.tag)
            && producerName.equals(receiver.getFederateName(reflection.producer)),
         description + " returned the wrong local object identity, value, tag, or producer");
      TransportationTypeHandle reliable = receiver.getTransportationTypeHandle("HLAreliable");
      JavaTckSupport.check(reliable.equals(reflection.transportation),
         description + " returned a transportation type other than HLAreliable");
      return reflection;
   }

   private static void await(RTIambassador first, RTIambassador second, RTIambassador third,
         CallbackModel model, java.util.function.BooleanSupplier condition, String description)
         throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         pump(first, model);
         pump(second, model);
         pump(third, model);
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void pump(RTIambassador ambassador, CallbackModel model) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
      else Thread.yield();
   }

   private static void deleteRegion(RTIambassador ambassador, RegionHandle region,
         boolean joined) {
      if (joined && region != null) try { ambassador.deleteRegion(region); }
         catch (Exception ignored) { }
   }

   private static final class Reflection {
      private ObjectInstanceHandle object;
      private AttributeHandleValueMap values;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private int regionCount = -1;
      private DimensionHandleSet regionDimensions;
      private hla.rti1516_2025.RangeBounds firstBounds;
      private hla.rti1516_2025.RangeBounds secondBounds;
      private boolean containsOriginalSourceRegion;
      private Exception regionInspectionError;
   }

   private static final class Discovery {
      private ObjectInstanceHandle object;
      private ObjectClassHandle objectClass;
      private String name;
   }

   private static final class Recorder {
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private RTIambassador regionInspector;
      private DimensionHandle firstDimension;
      private DimensionHandle secondDimension;

      private Discovery discovery(String objectName) {
         for (Discovery discovery : discoveries) {
            if (objectName.equals(discovery.name)) return discovery;
         }
         return null;
      }

      private boolean hasDiscovery(String objectName) {
         return discovery(objectName) != null;
      }

      private void inspectRegionMetadata(RTIambassador ambassador, DimensionHandle first,
            DimensionHandle second) {
         regionInspector = ambassador;
         firstDimension = first;
         secondDimension = second;
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 3) {
                  Discovery discovery = new Discovery();
                  discovery.object = (ObjectInstanceHandle) arguments[0];
                  discovery.objectClass = (ObjectClassHandle) arguments[1];
                  discovery.name = (String) arguments[2];
                  discoveries.add(discovery);
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 6) {
                  Reflection reflection = new Reflection();
                  reflection.object = (ObjectInstanceHandle) arguments[0];
                  reflection.values = ((AttributeHandleValueMap) arguments[1]).clone();
                  reflection.tag = ((byte[]) arguments[2]).clone();
                  reflection.transportation = (TransportationTypeHandle) arguments[3];
                  reflection.producer = (FederateHandle) arguments[4];
                  RegionHandleSet regions = (RegionHandleSet) arguments[5];
                  reflection.regionCount = regions == null ? -1 : regions.size();
                  if (regions != null && regionInspector != null) {
                     for (RegionHandle callbackRegion : regions) {
                        try {
                           DimensionHandleSet dimensions =
                              regionInspector.getDimensionHandleSet(callbackRegion);
                           if (dimensions.size() == 2 && dimensions.contains(firstDimension)
                                 && dimensions.contains(secondDimension)) {
                              hla.rti1516_2025.RangeBounds firstBounds =
                                 regionInspector.getRangeBounds(callbackRegion, firstDimension);
                              hla.rti1516_2025.RangeBounds secondBounds =
                                 regionInspector.getRangeBounds(callbackRegion, secondDimension);
                              if (reflection.regionDimensions == null) {
                                 reflection.regionDimensions = dimensions.clone();
                                 reflection.firstBounds = firstBounds;
                                 reflection.secondBounds = secondBounds;
                              }
                              if (firstBounds.lower == 1L && firstBounds.upper == 5L
                                    && secondBounds.lower == 1L && secondBounds.upper == 5L) {
                                 reflection.containsOriginalSourceRegion = true;
                              }
                           }
                        } catch (Exception error) {
                           reflection.regionInspectionError = error;
                           break;
                        }
                     }
                  }
                  reflections.add(reflection);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK regional callback DDM recheck";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
