package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeRegionAssociation;
import hla.rti1516_2025.AttributeSetRegionSetPairList;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;

/** Re-evaluate timestamped attribute subscriptions at the time-grant boundary. */
final class DelaySubscriptionEvaluationTimestampedAttributeTck {
   private DelaySubscriptionEvaluationTimestampedAttributeTck() { }

   static void run(RtiFactory factory, String fom, String switchesFom,
         String timeImplementation, String objectClassName, String attributeName,
         CallbackModel model) throws Exception {
      runConfiguration(factory, fom, switchesFom, timeImplementation,
         objectClassName, attributeName, model, false);
      runConfiguration(factory, fom, switchesFom, timeImplementation,
         objectClassName, attributeName, model, true);
   }

   static void runRegional(RtiFactory factory, String switchesFom, String ddmFom,
         String timeImplementation, String objectClassName, String attributeName,
         String firstDimensionName, String secondDimensionName, CallbackModel model)
         throws Exception {
      runRegionalConfiguration(factory, switchesFom, ddmFom, timeImplementation,
         objectClassName, attributeName, firstDimensionName, secondDimensionName, model, false);
      runRegionalConfiguration(factory, switchesFom, ddmFom, timeImplementation,
         objectClassName, attributeName, firstDimensionName, secondDimensionName, model, true);
   }

   private static void runRegionalConfiguration(RtiFactory factory, String switchesFom,
         String ddmFom, String timeImplementation, String objectClassName,
         String attributeName, String firstDimensionName, String secondDimensionName,
         CallbackModel model, boolean enabled) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherEvents = new Recorder();
      Recorder receiverEvents = new Recorder();
      String federation = "java-tck-delay-subscription-regional-timestamped-attribute-"
         + enabled + "-" + UUID.randomUUID();
      boolean publisherConnected = false, receiverConnected = false, created = false;
      boolean publisherJoined = false, receiverJoined = false;
      boolean regulating = false, constrained = false, published = false;
      boolean subscribed = false, associated = false;
      RegionHandle sourceRegion = null, receiverRegion = null;
      ObjectClassHandle publisherClass = null, receiverClass = null;
      AttributeHandle publisherAttribute = null, receiverAttribute = null;
      AttributeHandleSet publisherAttributes = null, receiverAttributes = null;
      AttributeSetRegionSetPairList publisherPair = null, receiverPair = null;
      ObjectInstanceHandle object = null;
      try {
         publisher.connect(publisherEvents.proxy(), model); publisherConnected = true;
         receiver.connect(receiverEvents.proxy(), model); receiverConnected = true;
         String[] modules = enabled ? new String[] {ddmFom, switchesFom} : new String[] {ddmFom};
         publisher.createFederationExecution(federation, modules, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-regional-dse-publisher", federation); publisherJoined = true;
         receiver.joinFederationExecution("java-tck-regional-dse-receiver", federation);
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
            "adapter DDM FOM returned inconsistent regional delay-subscription handles");
         checkDimensions(publisher, publisherClass, publisherFirst, publisherSecond);
         checkDimensions(receiver, receiverClass, receiverFirst, receiverSecond);
         JavaTckSupport.check(publisher.getDelaySubscriptionEvaluationSwitch() == enabled
               && receiver.getDelaySubscriptionEvaluationSwitch() == enabled,
            "regional Delay Subscription Evaluation switch did not match composed FOM modules");

         publisherAttributes = JavaTckSupport.attributeSet(publisher, publisherAttribute);
         receiverAttributes = JavaTckSupport.attributeSet(receiver, receiverAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         publisher.changeDefaultAttributeOrderType(publisherClass, publisherAttributes,
            OrderType.TIMESTAMP);
         receiver.setConveyRegionDesignatorSetsSwitch(true);
         JavaTckSupport.check(receiver.getConveyRegionDesignatorSetsSwitch(),
            "regional delay-subscription receiver did not enable region designators");
         sourceRegion = createRegion(publisher, publisherFirst, publisherSecond, 1, 5);
         receiverRegion = createRegion(receiver, receiverFirst, receiverSecond, 2, 6);
         publisherPair = association(publisher, publisherAttributes, sourceRegion);
         receiverPair = association(receiver, receiverAttributes, receiverRegion);
         object = publisher.registerObjectInstanceWithRegions(publisherClass, publisherPair);
         associated = true;
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "regional delay-subscription registration returned an invalid object handle");
         receiver.subscribeObjectClassAttributesWithRegions(receiverClass, receiverPair);
         subscribed = true;
         ObjectInstanceHandle registeredObject = object;
         awaitPair(model, publisher, receiver, objectClassName,
            () -> registeredObject.equals(receiverEvents.discoveredObject),
            "regional object discovery");
         JavaTckSupport.check(receiverClass.equals(receiver.getKnownObjectClassHandle(object)),
            "regional discovery did not establish the known object class");
         receiver.unsubscribeObjectClassAttributesWithRegions(receiverClass, receiverPair);
         subscribed = false;

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         JavaTckSupport.check(publisherFactory.getName().equals(receiverFactory.getName()),
            "regional delay-subscription members selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) publisherFactory.makeEpsilon();
         publisher.enableTimeRegulation(epsilon); regulating = true;
         await(model, publisher, objectClassName,
            () -> publisherEvents.timeRegulationEnabled != null, "Time Regulation enablement");
         receiver.enableTimeConstrained(); constrained = true;
         await(model, receiver, objectClassName,
            () -> receiverEvents.timeConstrainedEnabled != null, "Time Constrained enablement");

         LogicalTime firstTime = add(publisher.queryLogicalTime(), intervalOffset(epsilon, 2));
         LogicalTime firstReceiverTime = add(
            receiver.queryLogicalTime(), intervalOffset(epsilon, 2));
         JavaTckSupport.check(firstTime.equals(firstReceiverTime),
            "regional members derived different first grant times");
         byte[] firstValue = new byte[] {0x44, 0x53, 0x52, 0x31};
         byte[] secondValue = new byte[] {0x44, 0x53, 0x52, 0x32};
         AttributeHandleValueMap firstValues =
            publisher.getAttributeHandleValueMapFactory().create(1);
         firstValues.put(publisherAttribute, firstValue.clone());
         MessageRetractionReturn firstRetraction = publisher.updateAttributeValues(
            object, firstValues, new byte[0], firstTime);
         JavaTckSupport.check(!enabled || (firstRetraction != null
               && firstRetraction.retractionHandleIsValid && firstRetraction.handle != null),
            "enabled regional timestamped update returned an invalid retraction handle");
         JavaTckSupport.check(receiverEvents.reflections.isEmpty(),
            "regional timestamped update arrived before the constrained grant");
         receiver.subscribeObjectClassAttributesWithRegions(receiverClass, receiverPair);
         subscribed = true;
         receiver.timeAdvanceRequest(firstReceiverTime);
         publisher.timeAdvanceRequest(firstTime);
         int minimumFirstCount = enabled ? 1 : 0;
         awaitPair(model, publisher, receiver, objectClassName,
            () -> publisherEvents.grants.size() >= 1 && receiverEvents.grants.size() >= 1
               && receiverEvents.reflections.size() >= minimumFirstCount,
            "first regional delay-subscription grant");
         int firstDeliveryCount = receiverEvents.reflections.size();
         JavaTckSupport.check(enabled ? firstDeliveryCount == 1 : firstDeliveryCount <= 1,
            "regional Delay Subscription Evaluation delivered an unexpected first-reflection count");
         if (firstDeliveryCount > 0) {
            JavaTckSupport.check(firstRetraction != null && firstRetraction.handle != null,
               "delivered regional reflection has no retraction designator");
            verifyRegional(receiverEvents.reflections.get(0), object, receiverAttribute,
               firstValue, producer, firstTime, firstRetraction.handle, sourceRegion,
               receiver, "first regional grant");
         }

         LogicalTime secondTime = add(firstTime, epsilon);
         LogicalTime secondReceiverTime = add(firstReceiverTime, epsilon);
         AttributeHandleValueMap secondValues =
            publisher.getAttributeHandleValueMapFactory().create(1);
         secondValues.put(publisherAttribute, secondValue.clone());
         MessageRetractionReturn secondRetraction = publisher.updateAttributeValues(
            object, secondValues, new byte[0], secondTime);
         JavaTckSupport.check(secondRetraction != null
               && secondRetraction.retractionHandleIsValid && secondRetraction.handle != null,
            "regional timestamped update returned an invalid second retraction handle");
         receiver.unsubscribeObjectClassAttributesWithRegions(receiverClass, receiverPair);
         subscribed = false;
         receiver.timeAdvanceRequest(secondReceiverTime);
         publisher.timeAdvanceRequest(secondTime);
         awaitPair(model, publisher, receiver, objectClassName,
            () -> publisherEvents.grants.size() >= 2 && receiverEvents.grants.size() >= 2,
            "second regional delay-subscription grant");
         JavaTckSupport.check(receiverEvents.reflections.size() == firstDeliveryCount,
            "regional timestamped update was delivered after unsubscribe");

         receiver.disableTimeConstrained(); constrained = false;
         publisher.disableTimeRegulation(); regulating = false;
         publisher.unassociateRegionsForUpdates(object, publisherPair); associated = false;
         receiver.deleteRegion(receiverRegion); receiverRegion = null;
         publisher.deleteRegion(sourceRegion); sourceRegion = null;
         publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         published = false;
         receiver.resignFederationExecution(ResignAction.NO_ACTION); receiverJoined = false;
         publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS); publisherJoined = false;
         publisher.destroyFederationExecution(federation); created = false;
      } finally {
         if (constrained && receiverJoined) try { receiver.disableTimeConstrained(); }
            catch (Exception ignored) { }
         if (regulating && publisherJoined) try { publisher.disableTimeRegulation(); }
            catch (Exception ignored) { }
         if (subscribed && receiverJoined && receiverPair != null) try {
            receiver.unsubscribeObjectClassAttributesWithRegions(receiverClass, receiverPair);
         } catch (Exception ignored) { }
         if (associated && publisherJoined && object != null && publisherPair != null) try {
            publisher.unassociateRegionsForUpdates(object, publisherPair);
         } catch (Exception ignored) { }
         if (published && publisherJoined) try {
            publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         } catch (Exception ignored) { }
         deleteRegion(receiver, receiverRegion, receiverJoined);
         deleteRegion(publisher, sourceRegion, publisherJoined);
         if (receiverJoined) resign(receiver);
         if (publisherJoined) resignDelete(publisher);
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (receiverConnected) disconnect(receiver);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static void runConfiguration(RtiFactory factory, String fom, String switchesFom,
         String timeImplementation, String objectClassName, String attributeName,
         CallbackModel model, boolean enabled) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherEvents = new Recorder();
      Recorder receiverEvents = new Recorder();
      String federation = "java-tck-delay-subscription-timestamped-attribute-"
         + enabled + "-" + UUID.randomUUID();
      boolean publisherConnected = false, receiverConnected = false, created = false;
      boolean publisherJoined = false, receiverJoined = false;
      boolean regulating = false, constrained = false;
      try {
         publisher.connect(publisherEvents.proxy(), model);
         publisherConnected = true;
         receiver.connect(receiverEvents.proxy(), model);
         receiverConnected = true;
         String[] modules = enabled ? new String[] {fom, switchesFom} : new String[] {fom};
         publisher.createFederationExecution(federation, modules, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-delay-tso-attribute-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-delay-tso-attribute-receiver", federation);
         receiverJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute =
            publisher.getAttributeHandle(publisherClass, attributeName);
         AttributeHandle receiverAttribute =
            receiver.getAttributeHandle(receiverClass, attributeName);
         JavaTckSupport.check(publisherClass.equals(receiverClass)
               && publisherAttribute.equals(receiverAttribute),
            "adapter FOM returned inconsistent object/attribute handles");
         JavaTckSupport.check(publisher.getDelaySubscriptionEvaluationSwitch() == enabled
               && receiver.getDelaySubscriptionEvaluationSwitch() == enabled,
            "Delay Subscription Evaluation switch did not match the composed FOM");

         publisher.publishObjectClassAttributes(publisherClass,
            JavaTckSupport.attributeSet(publisher, publisherAttribute));
         publisher.changeDefaultAttributeOrderType(publisherClass,
            JavaTckSupport.attributeSet(publisher, publisherAttribute), OrderType.TIMESTAMP);
         receiver.subscribeObjectClassAttributes(receiverClass,
            JavaTckSupport.attributeSet(receiver, receiverAttribute));
         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         if (model == CallbackModel.HLA_EVOKED) service(receiver, model, objectClassName);
         JavaTckSupport.check(object.equals(receiverEvents.discoveredObject)
               && receiverClass.equals(receiver.getKnownObjectClassHandle(object)),
            "timestamped attribute discovery did not establish the known object");
         receiver.unsubscribeObjectClassAttributes(receiverClass,
            JavaTckSupport.attributeSet(receiver, receiverAttribute));

         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         publisher.enableTimeRegulation(epsilon);
         regulating = true;
         await(model, publisher, objectClassName,
            () -> publisherEvents.timeRegulationEnabled != null,
            "time-regulation enablement");
         receiver.enableTimeConstrained();
         constrained = true;
         await(model, receiver, objectClassName,
            () -> receiverEvents.timeConstrainedEnabled != null,
            "time-constrained enablement");

         LogicalTime firstTime = add(publisher.queryLogicalTime(), intervalOffset(epsilon, 2));
         LogicalTime firstReceiverTime = add(
            receiver.queryLogicalTime(), intervalOffset(epsilon, 2));
         JavaTckSupport.check(firstTime.equals(firstReceiverTime),
            "federates derived different first grant times");
         byte[] payload = new byte[] {(byte) 0xD5, 0x54};
         AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(1);
         values.put(publisherAttribute, payload);

         MessageRetractionReturn firstRetraction = publisher.updateAttributeValues(
            object, values, new byte[0], firstTime);
         JavaTckSupport.check(!enabled || (firstRetraction != null
               && firstRetraction.retractionHandleIsValid && firstRetraction.handle != null),
            "enabled timestamped attribute update returned an invalid retraction handle");
         receiver.subscribeObjectClassAttributes(receiverClass,
            JavaTckSupport.attributeSet(receiver, receiverAttribute));
         receiver.timeAdvanceRequest(firstReceiverTime);
         publisher.timeAdvanceRequest(firstTime);
         awaitPair(model, publisher, receiver, objectClassName,
            () -> publisherEvents.grants.size() >= 1 && receiverEvents.grants.size() >= 1
               && receiverEvents.reflections.size() >= (enabled ? 1 : 0),
            "first timestamped attribute grant");

         int firstDeliveryCount = receiverEvents.reflections.size();
         JavaTckSupport.check(enabled ? firstDeliveryCount == 1 : firstDeliveryCount <= 1,
            "unexpected first timestamped reflection count");
         if (firstDeliveryCount != 0) {
            JavaTckSupport.check(firstRetraction != null && firstRetraction.handle != null,
               "delivered timestamped reflection has no matching retraction designator");
            verify(receiverEvents.reflections.get(0), object, receiverAttribute, payload,
               producer, firstTime, firstRetraction.handle, receiver, "first grant");
         }

         LogicalTime secondTime = add(firstTime, epsilon);
         LogicalTime secondReceiverTime = add(firstReceiverTime, epsilon);
         MessageRetractionReturn secondRetraction = publisher.updateAttributeValues(
            object, values, new byte[0], secondTime);
         JavaTckSupport.check(secondRetraction != null
               && secondRetraction.retractionHandleIsValid && secondRetraction.handle != null,
            "second timestamped update returned an invalid retraction handle");
         receiver.unsubscribeObjectClassAttributes(receiverClass,
            JavaTckSupport.attributeSet(receiver, receiverAttribute));
         receiver.timeAdvanceRequest(secondReceiverTime);
         publisher.timeAdvanceRequest(secondTime);
         awaitPair(model, publisher, receiver, objectClassName,
            () -> publisherEvents.grants.size() >= 2 && receiverEvents.grants.size() >= 2,
            "second timestamped attribute grant");
         JavaTckSupport.check(receiverEvents.reflections.size() == firstDeliveryCount,
            "timestamped attribute reflection arrived after unsubscribe");

         receiver.disableTimeConstrained();
         constrained = false;
         publisher.disableTimeRegulation();
         regulating = false;
         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (constrained && receiverJoined) {
            try { receiver.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (regulating && publisherJoined) {
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (receiverJoined) resign(receiver);
         if (publisherJoined) resignDelete(publisher);
         if (created) {
            try { publisher.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (receiverConnected) disconnect(receiver);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static LogicalTime add(LogicalTime start, LogicalTimeInterval interval)
         throws Exception {
      return (LogicalTime) start.add(interval);
   }

   private static LogicalTimeInterval intervalOffset(LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTimeInterval result = epsilon;
      for (int i = 1; i < steps; ++i) result = (LogicalTimeInterval) result.add(epsilon);
      return result;
   }

   private static void verify(Reflection reflection, ObjectInstanceHandle object,
         AttributeHandle attribute, byte[] payload, FederateHandle producer, LogicalTime time,
         MessageRetractionHandle retraction, RTIambassador receiver, String phase)
         throws Exception {
      JavaTckSupport.check(object.equals(reflection.object)
            && reflection.values.size() == 1
            && reflection.values.containsKey(attribute)
            && Arrays.equals(payload, reflection.values.get(attribute))
            && reflection.tag.length == 0
            && producer.equals(reflection.producer)
            && time.equals(reflection.time)
            && reflection.sentOrder == OrderType.TIMESTAMP
            && reflection.receivedOrder == OrderType.TIMESTAMP
            && retraction.equals(reflection.retraction)
            && reflection.transportation != null
            && !receiver.getTransportationTypeName(reflection.transportation).isEmpty()
            && reflection.regionsEmpty,
         phase + " timestamped reflection lost standard delivery metadata");
   }

   private static void checkDimensions(RTIambassador ambassador, ObjectClassHandle objectClass,
         DimensionHandle first, DimensionHandle second) throws Exception {
      java.util.Set<DimensionHandle> available =
         ambassador.getAvailableDimensionsForObjectClass(objectClass);
      JavaTckSupport.check(available.contains(first) && available.contains(second)
            && ambassador.getDimensionUpperBound(first) >= 6
            && ambassador.getDimensionUpperBound(second) >= 6,
         "adapter DDM FOM does not expose the configured usable dimensions");
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle first,
         DimensionHandle second, long lower, long upper) throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(first);
      dimensions.add(second);
      RegionHandle region = ambassador.createRegion(dimensions);
      ambassador.setRangeBounds(region, first, new hla.rti1516_2025.RangeBounds(lower, upper));
      ambassador.setRangeBounds(region, second, new hla.rti1516_2025.RangeBounds(lower, upper));
      RegionHandleSet regions = regionSet(ambassador, region);
      ambassador.commitRegionModifications(regions);
      return region;
   }

   private static RegionHandleSet regionSet(RTIambassador ambassador, RegionHandle region)
         throws Exception {
      RegionHandleSet result = ambassador.getRegionHandleSetFactory().create();
      result.add(region);
      return result;
   }

   private static AttributeSetRegionSetPairList association(RTIambassador ambassador,
         AttributeHandleSet attributes, RegionHandle region) throws Exception {
      AttributeSetRegionSetPairList result =
         ambassador.getAttributeSetRegionSetPairListFactory().create(1);
      result.add(new AttributeRegionAssociation(attributes, regionSet(ambassador, region)));
      return result;
   }

   private static void verifyRegional(Reflection reflection, ObjectInstanceHandle object,
         AttributeHandle attribute, byte[] payload, FederateHandle producer, LogicalTime time,
         MessageRetractionHandle retraction, RegionHandle sourceRegion,
         RTIambassador receiver, String phase) throws Exception {
      JavaTckSupport.check(object.equals(reflection.object)
            && reflection.values.size() == 1 && reflection.values.containsKey(attribute)
            && Arrays.equals(payload, reflection.values.get(attribute))
            && reflection.tag.length == 0 && producer.equals(reflection.producer)
            && time.equals(reflection.time) && reflection.sentOrder == OrderType.TIMESTAMP
            && reflection.receivedOrder == OrderType.TIMESTAMP
            && retraction.equals(reflection.retraction)
            && reflection.transportation != null
            && !receiver.getTransportationTypeName(reflection.transportation).isEmpty()
            && reflection.regions != null && reflection.regions.size() == 1
            && reflection.regions.contains(sourceRegion),
         phase + " regional timestamped reflection lost standard metadata or source designator");
   }

   private static void deleteRegion(RTIambassador ambassador, RegionHandle region,
         boolean joined) {
      if (joined && region != null) try { ambassador.deleteRegion(region); }
         catch (Exception ignored) { }
   }

   private static void service(RTIambassador ambassador, CallbackModel model,
         String objectClassName) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) ambassador.evokeMultipleCallbacks(0.0, 1.0);
      else ambassador.getObjectClassHandle(objectClassName);
   }

   private static void await(CallbackModel model, RTIambassador ambassador,
         String objectClassName, Condition condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline) {
         service(ambassador, model, objectClassName);
         if (condition.isTrue()) return;
         Thread.yield();
      }
      throw new AssertionError("timed out waiting for " + description);
   }

   private static void awaitPair(CallbackModel model, RTIambassador publisher,
         RTIambassador receiver, String objectClassName, Condition condition,
         String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline) {
         service(publisher, model, objectClassName);
         service(receiver, model, objectClassName);
         if (condition.isTrue()) return;
         Thread.yield();
      }
      throw new AssertionError("timed out waiting for " + description);
   }

   @FunctionalInterface
   private interface Condition { boolean isTrue() throws Exception; }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void resignDelete(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static final class Reflection {
      ObjectInstanceHandle object;
      AttributeHandleValueMap values;
      byte[] tag;
      TransportationTypeHandle transportation;
      FederateHandle producer;
      boolean regionsEmpty;
      RegionHandleSet regions;
      LogicalTime<?, ?> time;
      OrderType sentOrder;
      OrderType receivedOrder;
      MessageRetractionHandle retraction;
   }

   private static final class Recorder {
      ObjectInstanceHandle discoveredObject;
      LogicalTime<?, ?> timeRegulationEnabled;
      LogicalTime<?, ?> timeConstrainedEnabled;
      final List<Reflection> reflections = new ArrayList<>();
      final List<LogicalTime<?, ?>> grants = new ArrayList<>();

      FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, args) -> {
               Object[] a = args == null ? new Object[0] : args;
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && a.length >= 1) {
                  discoveredObject = (ObjectInstanceHandle) a[0];
               } else if ("timeRegulationEnabled".equals(name) && a.length == 1) {
                  timeRegulationEnabled = (LogicalTime<?, ?>) a[0];
               } else if ("timeConstrainedEnabled".equals(name) && a.length == 1) {
                  timeConstrainedEnabled = (LogicalTime<?, ?>) a[0];
               } else if ("reflectAttributeValues".equals(name) && a.length == 10) {
                  Reflection reflection = new Reflection();
                  reflection.object = (ObjectInstanceHandle) a[0];
                  reflection.values = ((AttributeHandleValueMap) a[1]).clone();
                  reflection.tag = ((byte[]) a[2]).clone();
                  reflection.transportation = (TransportationTypeHandle) a[3];
                  reflection.producer = (FederateHandle) a[4];
                  RegionHandleSet regions = (RegionHandleSet) a[5];
                  reflection.regionsEmpty = regions == null || regions.isEmpty();
                  reflection.regions = regions;
                  reflection.time = (LogicalTime<?, ?>) a[6];
                  reflection.sentOrder = (OrderType) a[7];
                  reflection.receivedOrder = (OrderType) a[8];
                  reflection.retraction = (MessageRetractionHandle) a[9];
                  reflections.add(reflection);
               } else if ("timeAdvanceGrant".equals(name) && a.length == 1) {
                  grants.add((LogicalTime<?, ?>) a[0]);
               } else if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK timestamped DSE callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == a[0];
               }
               return null;
            });
      }
   }
}
