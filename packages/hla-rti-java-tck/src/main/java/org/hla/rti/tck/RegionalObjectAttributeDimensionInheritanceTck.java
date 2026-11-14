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
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks inherited object-attribute FDD dimensions and regional delivery. */
final class RegionalObjectAttributeDimensionInheritanceTck {
   private static final String BASE_CLASS = "HLAobjectRoot.TckAttributeDimensionBaseObject";
   private static final String DERIVED_CLASS =
      "HLAobjectRoot.TckAttributeDimensionBaseObject.TckAttributeDimensionDerivedObject";
   private static final String BASE_ATTRIBUTE = "BaseValue";
   private static final String DERIVED_ATTRIBUTE = "DerivedValue";
   private static final String X_DIMENSION = "TckDimensionX";
   private static final String Y_DIMENSION = "TckDimensionY";
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private RegionalObjectAttributeDimensionInheritanceTck() { }

   static void run(RtiFactory factory, String fom, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(fom != null && !fom.isEmpty(),
         "adapter-supplied multi-attribute DDM FOM is required");
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador xReceiver = factory.getRtiAmbassador();
      RTIambassador yReceiver = factory.getRtiAmbassador();
      RTIambassador disjointReceiver = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder xCallbacks = new Recorder();
      Recorder yCallbacks = new Recorder();
      Recorder disjointCallbacks = new Recorder();
      RTIambassador[] ambassadors = {
         publisher, xReceiver, yReceiver, disjointReceiver
      };
      Recorder[] recorders = {
         publisherCallbacks, xCallbacks, yCallbacks, disjointCallbacks
      };
      boolean[] connected = new boolean[4];
      boolean[] joined = new boolean[4];
      boolean created = false;
      boolean published = false;
      String federation = "java-tck-regional-object-attribute-dimension-inheritance-"
         + UUID.randomUUID();
      Handles publisherHandles = null;
      Handles xHandles = null;
      Handles yHandles = null;
      Handles disjointHandles = null;
      AttributeHandleSet publishedAttributes = null;
      AttributeSetRegionSetPairList sourcePairs = null;
      AttributeSetRegionSetPairList invalidBaseRegistration = null;
      AttributeSetRegionSetPairList xReceiverPairs = null;
      AttributeSetRegionSetPairList yReceiverPairs = null;
      AttributeSetRegionSetPairList disjointReceiverPairs = null;
      AttributeSetRegionSetPairList invalidBaseSubscription = null;
      RegionHandle invalidBaseSubscriptionRegion = null;
      ObjectInstanceHandle object = null;
      ObjectInstanceHandle[] unexpectedInvalidRegistration = new ObjectInstanceHandle[1];
      List<RegionHandle> publisherRegions = new ArrayList<>();
      List<RegionHandle> xRegions = new ArrayList<>();
      List<RegionHandle> yRegions = new ArrayList<>();
      List<RegionHandle> disjointRegions = new ArrayList<>();
      FederateHandle publisherFederate = null;
      try {
         for (int index = 0; index < ambassadors.length; ++index) {
            ambassadors[index].connect(recorders[index].proxy(), callbackModel);
            connected[index] = true;
         }
         publisher.createFederationExecution(federation, fom);
         created = true;
         publisherFederate = publisher.joinFederationExecution(
            "attribute-dimension-publisher", federation);
         joined[0] = true;
         xReceiver.joinFederationExecution("attribute-dimension-x-receiver", federation);
         joined[1] = true;
         yReceiver.joinFederationExecution("attribute-dimension-y-receiver", federation);
         joined[2] = true;
         disjointReceiver.joinFederationExecution(
            "attribute-dimension-disjoint-receiver", federation);
         joined[3] = true;
         JavaTckSupport.check(publisherFederate != null && publisherFederate.encodedLength() > 0,
            "publisher join returned an invalid federate handle");

         publisherHandles = resolveHandles(publisher);
         xHandles = resolveHandles(xReceiver);
         yHandles = resolveHandles(yReceiver);
         disjointHandles = resolveHandles(disjointReceiver);
         checkSameHandles(publisherHandles, xHandles, "X receiver");
         checkSameHandles(publisherHandles, yHandles, "Y receiver");
         checkSameHandles(publisherHandles, disjointHandles, "disjoint receiver");

         publishedAttributes = JavaTckSupport.attributeSet(publisher,
            publisherHandles.inheritedBaseValue, publisherHandles.derivedValue);
         publisher.publishObjectClassAttributes(publisherHandles.derivedClass, publishedAttributes);
         published = true;

         RegionHandle sourceX = createRegion(publisher, publisherRegions,
            new DimensionHandle[] {publisherHandles.x}, new long[] {10}, new long[] {20},
            "BaseValue source X");
         RegionHandle sourceXY = createRegion(publisher, publisherRegions,
            new DimensionHandle[] {publisherHandles.x, publisherHandles.y},
            new long[] {10, 10}, new long[] {20, 20}, "DerivedValue source X/Y");
         RegionHandle invalidBaseYRegion = createRegion(publisher, publisherRegions,
            new DimensionHandle[] {publisherHandles.y}, new long[] {10}, new long[] {20},
            "invalid Y-only region for inherited BaseValue");
         invalidBaseRegistration = pairList(publisher,
            publisherHandles.inheritedBaseValue, invalidBaseYRegion);
         ObjectClassHandle publishedClass = publisherHandles.derivedClass;
         AttributeSetRegionSetPairList invalidRegistrationPairs = invalidBaseRegistration;
         JavaTckSupport.expectFailure(
            () -> unexpectedInvalidRegistration[0] = publisher.registerObjectInstanceWithRegions(
               publishedClass, invalidRegistrationPairs),
            "InvalidRegionContext",
            "derived-class registration associating inherited BaseValue with Y");

         RegionHandle xReceiverBaseRegion = createRegion(xReceiver, xRegions,
            new DimensionHandle[] {xHandles.x}, new long[] {12}, new long[] {18},
            "X receiver overlapping BaseValue");
         RegionHandle xReceiverDerivedRegion = createRegion(xReceiver, xRegions,
            new DimensionHandle[] {xHandles.x, xHandles.y},
            new long[] {12, 50}, new long[] {18, 60},
            "X receiver disjoint from DerivedValue");
         RegionHandle yReceiverBaseRegion = createRegion(yReceiver, yRegions,
            new DimensionHandle[] {yHandles.x}, new long[] {50}, new long[] {60},
            "Y receiver disjoint from BaseValue");
         RegionHandle yReceiverDerivedRegion = createRegion(yReceiver, yRegions,
            new DimensionHandle[] {yHandles.x, yHandles.y},
            new long[] {12, 12}, new long[] {18, 18},
            "Y receiver overlapping DerivedValue");
         RegionHandle disjointBaseRegion = createRegion(disjointReceiver, disjointRegions,
            new DimensionHandle[] {disjointHandles.x}, new long[] {50}, new long[] {60},
            "disjoint receiver BaseValue region");
         RegionHandle disjointDerivedRegion = createRegion(disjointReceiver, disjointRegions,
            new DimensionHandle[] {disjointHandles.x, disjointHandles.y},
            new long[] {50, 50}, new long[] {60, 60},
            "disjoint receiver DerivedValue region");
         xReceiverPairs = pairList(xReceiver, xHandles.inheritedBaseValue, xReceiverBaseRegion,
            xHandles.derivedValue, xReceiverDerivedRegion);
         yReceiverPairs = pairList(yReceiver, yHandles.inheritedBaseValue, yReceiverBaseRegion,
            yHandles.derivedValue, yReceiverDerivedRegion);
         disjointReceiverPairs = pairList(disjointReceiver,
            disjointHandles.inheritedBaseValue, disjointBaseRegion,
            disjointHandles.derivedValue, disjointDerivedRegion);

         invalidBaseSubscriptionRegion = createRegion(xReceiver, xRegions,
            new DimensionHandle[] {xHandles.y}, new long[] {10}, new long[] {20},
            "invalid Y-only subscription region for inherited BaseValue");
         invalidBaseSubscription = pairList(xReceiver,
            xHandles.inheritedBaseValue, invalidBaseSubscriptionRegion);
         ObjectClassHandle receiverClass = xHandles.derivedClass;
         AttributeSetRegionSetPairList invalidSubscriptionPairs = invalidBaseSubscription;
         JavaTckSupport.expectFailure(
            () -> xReceiver.subscribeObjectClassAttributesWithRegions(
               receiverClass, invalidSubscriptionPairs),
            "InvalidRegionContext",
            "regional subscription associating inherited BaseValue with Y");
         JavaTckSupport.expectFailure(
            () -> xReceiver.requestAttributeValueUpdateWithRegions(
               receiverClass, invalidSubscriptionPairs, new byte[] {0x72, 0x65, 0x67}),
            "InvalidRegionContext",
            "regional value request associating inherited BaseValue with Y");
         xReceiver.deleteRegion(invalidBaseSubscriptionRegion);
         xRegions.remove(invalidBaseSubscriptionRegion);
         invalidBaseSubscriptionRegion = null;

         xReceiver.subscribeObjectClassAttributesWithRegions(xHandles.derivedClass, xReceiverPairs);
         yReceiver.subscribeObjectClassAttributesWithRegions(yHandles.derivedClass, yReceiverPairs);
         disjointReceiver.subscribeObjectClassAttributesWithRegions(
            disjointHandles.derivedClass, disjointReceiverPairs);

         sourcePairs = pairList(publisher, publisherHandles.inheritedBaseValue, sourceX,
            publisherHandles.derivedValue, sourceXY);
         object = publisher.registerObjectInstanceWithRegions(
            publisherHandles.derivedClass, sourcePairs);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "attribute-specific region registration returned an invalid object handle");

         ObjectInstanceHandle registeredObject = object;
         JavaTckSupport.expectFailure(
            () -> publisher.associateRegionsForUpdates(
               registeredObject, invalidRegistrationPairs),
            "InvalidRegionContext",
            "instance association changed inherited BaseValue dimensions to Y");

         ObjectInstanceHandle registeredObjectForCallbacks = object;
         await(callbackModel, new RTIambassador[] {xReceiver, yReceiver},
            () -> hasDiscovery(xCallbacks, registeredObjectForCallbacks)
               && hasDiscovery(yCallbacks, registeredObjectForCallbacks),
            "matching regional object discovery");
         JavaTckSupport.check(xCallbacks.discoveries.size() == 1
               && yCallbacks.discoveries.size() == 1
               && xHandles.derivedClass.equals(xCallbacks.discoveries.get(0).objectClass)
               && yHandles.derivedClass.equals(yCallbacks.discoveries.get(0).objectClass)
               && object.equals(xCallbacks.discoveries.get(0).object)
               && object.equals(yCallbacks.discoveries.get(0).object),
            "regional discovery returned the wrong derived class or object handle");

         AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(2);
         byte[] baseValue = new byte[] {0x31, 0x32};
         byte[] derivedValue = new byte[] {0x41, 0x42};
         byte[] tag = new byte[] {0x64, 0x64, 0x6d};
         values.put(publisherHandles.inheritedBaseValue, baseValue.clone());
         values.put(publisherHandles.derivedValue, derivedValue.clone());
         publisher.updateAttributeValues(object, values, tag.clone());

         await(callbackModel, new RTIambassador[] {xReceiver, yReceiver},
            () -> !xCallbacks.reflections.isEmpty() && !yCallbacks.reflections.isEmpty(),
            "attribute-specific X/Y regional reflections");
         settle(callbackModel, new RTIambassador[] {xReceiver, yReceiver, disjointReceiver});
         requireOnlyAttributeReflection(xCallbacks, object,
            xHandles.inheritedBaseValue, xHandles.derivedValue, baseValue, tag,
            publisherFederate, "receiver overlapping only the inherited BaseValue X region");
         requireOnlyAttributeReflection(yCallbacks, object,
            yHandles.derivedValue, yHandles.inheritedBaseValue, derivedValue, tag,
            publisherFederate, "receiver overlapping only the derived DerivedValue X/Y region");
         JavaTckSupport.check(disjointCallbacks.reflections.isEmpty(),
            "attribute updates escaped both disjoint per-attribute regions");
      } finally {
         if (joined[0] && unexpectedInvalidRegistration[0] != null
               && invalidBaseRegistration != null) {
            try {
               publisher.unassociateRegionsForUpdates(
                  unexpectedInvalidRegistration[0], invalidBaseRegistration);
            } catch (Exception ignored) { }
            try { publisher.deleteObjectInstance(unexpectedInvalidRegistration[0], new byte[0]); }
            catch (Exception ignored) { }
         }
         if (joined[0] && object != null) {
            if (invalidBaseRegistration != null) {
               try { publisher.unassociateRegionsForUpdates(object, invalidBaseRegistration); }
               catch (Exception ignored) { }
            }
            if (sourcePairs != null) {
               try { publisher.unassociateRegionsForUpdates(object, sourcePairs); }
               catch (Exception ignored) { }
            }
         }
         safeUnsubscribe(xReceiver, xHandles == null ? null : xHandles.derivedClass,
            invalidBaseSubscription, joined[1]);
         safeUnsubscribe(xReceiver, xHandles == null ? null : xHandles.derivedClass,
            xReceiverPairs, joined[1]);
         safeUnsubscribe(yReceiver, yHandles == null ? null : yHandles.derivedClass,
            yReceiverPairs, joined[2]);
         safeUnsubscribe(disjointReceiver,
            disjointHandles == null ? null : disjointHandles.derivedClass,
            disjointReceiverPairs, joined[3]);
         if (joined[0] && published && publisherHandles != null && publishedAttributes != null) {
            try {
               publisher.unpublishObjectClassAttributes(
                  publisherHandles.derivedClass, publishedAttributes);
            } catch (Exception ignored) { }
         }
         deleteRegions(publisher, publisherRegions, joined[0]);
         deleteRegions(xReceiver, xRegions, joined[1]);
         deleteRegions(yReceiver, yRegions, joined[2]);
         deleteRegions(disjointReceiver, disjointRegions, joined[3]);
         if (joined[3]) resign(disjointReceiver, ResignAction.NO_ACTION);
         if (joined[2]) resign(yReceiver, ResignAction.NO_ACTION);
         if (joined[1]) resign(xReceiver, ResignAction.NO_ACTION);
         if (joined[0]) resign(publisher, ResignAction.DELETE_OBJECTS);
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

   static void runDefaultRegionFddDimensionsFromAdapter() throws Exception {
      runDefaultRegionFddDimensions(RtiTckMain.factory(),
         RtiTckMain.requireAdapterFile("hla.rti.tck.multiAttributeFom"),
         RtiTckMain.callbackModel());
   }

   static void runDefaultRegionFddDimensions(RtiFactory factory, String fom,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(fom != null && !fom.isEmpty(),
         "adapter-supplied multi-attribute DDM FOM is required");
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador baseReceiver = factory.getRtiAmbassador();
      RTIambassador derivedReceiver = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder baseCallbacks = new Recorder();
      Recorder derivedCallbacks = new Recorder();
      RTIambassador[] ambassadors = {publisher, baseReceiver, derivedReceiver};
      Recorder[] recorders = {publisherCallbacks, baseCallbacks, derivedCallbacks};
      boolean[] connected = new boolean[3];
      boolean[] joined = new boolean[3];
      boolean created = false;
      boolean published = false;
      String federation = "java-tck-regional-object-attribute-default-region-fdd-dimensions-"
         + UUID.randomUUID();
      Handles publisherHandles = null;
      Handles baseHandles = null;
      Handles derivedHandles = null;
      FederateHandle publisherFederate = null;
      AttributeHandleSet publishedAttributes = null;
      AttributeSetRegionSetPairList invalidBaseSubscription = null;
      AttributeSetRegionSetPairList baseSubscription = null;
      AttributeSetRegionSetPairList derivedSubscription = null;
      RegionHandle invalidBaseYRegion = null;
      ObjectInstanceHandle object = null;
      List<RegionHandle> publisherRegions = new ArrayList<>();
      List<RegionHandle> baseRegions = new ArrayList<>();
      List<RegionHandle> derivedRegions = new ArrayList<>();
      try {
         for (int index = 0; index < ambassadors.length; ++index) {
            ambassadors[index].connect(recorders[index].proxy(), callbackModel);
            connected[index] = true;
         }
         publisher.createFederationExecution(federation, fom);
         created = true;
         publisherFederate = publisher.joinFederationExecution(
            "attribute-default-region-publisher", federation);
         joined[0] = true;
         baseReceiver.joinFederationExecution("attribute-default-region-base-receiver", federation);
         joined[1] = true;
         derivedReceiver.joinFederationExecution(
            "attribute-default-region-derived-receiver", federation);
         joined[2] = true;
         JavaTckSupport.check(publisherFederate != null && publisherFederate.encodedLength() > 0,
            "publisher join returned an invalid federate handle");

         publisherHandles = resolveHandles(publisher);
         baseHandles = resolveHandles(baseReceiver);
         derivedHandles = resolveHandles(derivedReceiver);
         checkSameHandles(publisherHandles, baseHandles, "base receiver");
         checkSameHandles(publisherHandles, derivedHandles, "derived receiver");
         long xUpperBound = publisher.getDimensionUpperBound(publisherHandles.x);
         long yUpperBound = publisher.getDimensionUpperBound(publisherHandles.y);
         JavaTckSupport.check(xUpperBound == 100L && yUpperBound == 100L,
            "multi-attribute FDD dimensions did not retain their declared upper bounds");
         long xHighLower = xUpperBound - 1L;
         long yHighLower = yUpperBound - 1L;

         publishedAttributes = JavaTckSupport.attributeSet(publisher,
            publisherHandles.inheritedBaseValue, publisherHandles.derivedValue);
         publisher.publishObjectClassAttributes(publisherHandles.derivedClass, publishedAttributes);
         published = true;

         RegionHandle baseHighRegion = createRegion(baseReceiver, baseRegions,
            new DimensionHandle[] {baseHandles.x}, new long[] {xHighLower},
            new long[] {xUpperBound}, "near-upper-bound inherited BaseValue subscription");
         RegionHandle derivedHighYRegion = createRegion(derivedReceiver, derivedRegions,
            new DimensionHandle[] {derivedHandles.y}, new long[] {yHighLower},
            new long[] {yUpperBound}, "near-upper-bound derived DerivedValue Y subscription");
         invalidBaseYRegion = createRegion(baseReceiver, baseRegions,
            new DimensionHandle[] {baseHandles.y}, new long[] {yHighLower},
            new long[] {yUpperBound}, "unavailable Y-only inherited BaseValue subscription");
         invalidBaseSubscription = pairList(baseReceiver,
            baseHandles.inheritedBaseValue, invalidBaseYRegion);
         ObjectClassHandle baseClass = baseHandles.derivedClass;
         AttributeSetRegionSetPairList invalidSubscriptionPairs = invalidBaseSubscription;
         JavaTckSupport.expectFailure(
            () -> baseReceiver.subscribeObjectClassAttributesWithRegions(
               baseClass, invalidSubscriptionPairs),
            "InvalidRegionContext",
            "FDD-unavailable Y dimension for inherited BaseValue subscription");
         baseReceiver.deleteRegion(invalidBaseYRegion);
         baseRegions.remove(invalidBaseYRegion);
         invalidBaseYRegion = null;

         baseSubscription = pairList(baseReceiver, baseHandles.inheritedBaseValue, baseHighRegion);
         derivedSubscription = pairList(derivedReceiver,
            derivedHandles.derivedValue, derivedHighYRegion);
         baseReceiver.subscribeObjectClassAttributesWithRegions(
            baseHandles.derivedClass, baseSubscription);
         derivedReceiver.subscribeObjectClassAttributesWithRegions(
            derivedHandles.derivedClass, derivedSubscription);

         object = publisher.registerObjectInstance(publisherHandles.derivedClass);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "default-region object registration returned an invalid object handle");
         ObjectInstanceHandle registeredObject = object;
         await(callbackModel, new RTIambassador[] {baseReceiver, derivedReceiver},
            () -> hasDiscovery(baseCallbacks, registeredObject)
               && hasDiscovery(derivedCallbacks, registeredObject),
            "default-region object discovery through FDD-associated attributes");
         JavaTckSupport.check(baseCallbacks.discoveries.size() == 1
               && derivedCallbacks.discoveries.size() == 1
               && baseHandles.derivedClass.equals(baseCallbacks.discoveries.get(0).objectClass)
               && derivedHandles.derivedClass.equals(derivedCallbacks.discoveries.get(0).objectClass)
               && object.equals(baseCallbacks.discoveries.get(0).object)
               && object.equals(derivedCallbacks.discoveries.get(0).object),
            "default-region discovery returned the wrong class or object handle");

         AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(2);
         byte[] baseValue = new byte[] {0x51, 0x52};
         byte[] derivedValue = new byte[] {0x61, 0x62};
         byte[] tag = new byte[] {0x66, 0x64, 0x64};
         values.put(publisherHandles.inheritedBaseValue, baseValue.clone());
         values.put(publisherHandles.derivedValue, derivedValue.clone());
         publisher.updateAttributeValues(object, values, tag.clone());

         await(callbackModel, new RTIambassador[] {baseReceiver, derivedReceiver},
            () -> !baseCallbacks.reflections.isEmpty() && !derivedCallbacks.reflections.isEmpty(),
            "implicit default-region delivery at FDD upper-bound ranges");
         settle(callbackModel, new RTIambassador[] {baseReceiver, derivedReceiver});
         requireOnlyAttributeReflection(baseCallbacks, object,
            baseHandles.inheritedBaseValue, baseHandles.derivedValue, baseValue, tag,
            publisherFederate,
            "X-only receiver near the FDD upper bound for inherited BaseValue");
         requireOnlyAttributeReflection(derivedCallbacks, object,
            derivedHandles.derivedValue, derivedHandles.inheritedBaseValue, derivedValue, tag,
            publisherFederate,
            "Y-only receiver near the FDD upper bound for derived DerivedValue");
      } finally {
         safeUnsubscribe(baseReceiver, baseHandles == null ? null : baseHandles.derivedClass,
            invalidBaseSubscription, joined[1]);
         safeUnsubscribe(baseReceiver, baseHandles == null ? null : baseHandles.derivedClass,
            baseSubscription, joined[1]);
         safeUnsubscribe(derivedReceiver,
            derivedHandles == null ? null : derivedHandles.derivedClass,
            derivedSubscription, joined[2]);
         if (joined[0] && published && publisherHandles != null && publishedAttributes != null) {
            try {
               publisher.unpublishObjectClassAttributes(
                  publisherHandles.derivedClass, publishedAttributes);
            } catch (Exception ignored) { }
         }
         deleteRegions(publisher, publisherRegions, joined[0]);
         deleteRegions(baseReceiver, baseRegions, joined[1]);
         deleteRegions(derivedReceiver, derivedRegions, joined[2]);
         if (joined[2]) resign(derivedReceiver, ResignAction.NO_ACTION);
         if (joined[1]) resign(baseReceiver, ResignAction.NO_ACTION);
         if (joined[0]) resign(publisher, ResignAction.DELETE_OBJECTS);
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

   static void runUnspecifiedDimensionDefaultRangeFromAdapter() throws Exception {
      runUnspecifiedDimensionDefaultRange(RtiTckMain.factory(),
         RtiTckMain.requireAdapterFile("hla.rti.tck.multiAttributeFom"),
         RtiTckMain.callbackModel());
   }

   static void runUnspecifiedDimensionDefaultRange(RtiFactory factory, String fom,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(fom != null && !fom.isEmpty(),
         "adapter-supplied multi-attribute DDM FOM is required");
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador baseReceiver = factory.getRtiAmbassador();
      RTIambassador yReceiver = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder baseCallbacks = new Recorder();
      Recorder yCallbacks = new Recorder();
      RTIambassador[] ambassadors = {publisher, baseReceiver, yReceiver};
      Recorder[] recorders = {publisherCallbacks, baseCallbacks, yCallbacks};
      boolean[] connected = new boolean[3];
      boolean[] joined = new boolean[3];
      boolean created = false;
      boolean published = false;
      String federation = "java-tck-regional-object-attribute-unspecified-default-range-"
         + UUID.randomUUID();
      Handles publisherHandles = null;
      Handles baseHandles = null;
      Handles yHandles = null;
      FederateHandle publisherFederate = null;
      AttributeHandleSet publishedAttributes = null;
      AttributeSetRegionSetPairList sourcePairs = null;
      AttributeSetRegionSetPairList baseSubscription = null;
      AttributeSetRegionSetPairList ySubscription = null;
      ObjectInstanceHandle object = null;
      List<RegionHandle> publisherRegions = new ArrayList<>();
      List<RegionHandle> baseRegions = new ArrayList<>();
      List<RegionHandle> yRegions = new ArrayList<>();
      try {
         for (int index = 0; index < ambassadors.length; ++index) {
            ambassadors[index].connect(recorders[index].proxy(), callbackModel);
            connected[index] = true;
         }
         publisher.createFederationExecution(federation, fom);
         created = true;
         publisherFederate = publisher.joinFederationExecution(
            "attribute-unspecified-range-publisher", federation);
         joined[0] = true;
         baseReceiver.joinFederationExecution(
            "attribute-unspecified-range-base-receiver", federation);
         joined[1] = true;
         yReceiver.joinFederationExecution("attribute-unspecified-range-y-receiver", federation);
         joined[2] = true;
         JavaTckSupport.check(publisherFederate != null && publisherFederate.encodedLength() > 0,
            "publisher join returned an invalid federate handle");

         publisherHandles = resolveHandles(publisher);
         baseHandles = resolveHandles(baseReceiver);
         yHandles = resolveHandles(yReceiver);
         checkSameHandles(publisherHandles, baseHandles, "base receiver");
         checkSameHandles(publisherHandles, yHandles, "Y receiver");
         long xUpperBound = publisher.getDimensionUpperBound(publisherHandles.x);
         long yUpperBound = publisher.getDimensionUpperBound(publisherHandles.y);
         JavaTckSupport.check(xUpperBound == 100L && yUpperBound == 100L,
            "FDD default-range fixture must use [0, upperBound) for dimensions X and Y");

         publishedAttributes = JavaTckSupport.attributeSet(publisher,
            publisherHandles.inheritedBaseValue, publisherHandles.derivedValue);
         publisher.publishObjectClassAttributes(publisherHandles.derivedClass, publishedAttributes);

         RegionHandle baseSourceX = createRegion(publisher, publisherRegions,
            new DimensionHandle[] {publisherHandles.x}, new long[] {10}, new long[] {20},
            "inherited BaseValue X-only source specification");
         RegionHandle derivedSourceX = createRegion(publisher, publisherRegions,
            new DimensionHandle[] {publisherHandles.x}, new long[] {10}, new long[] {20},
            "derived DerivedValue X-only source specification");
         DimensionHandleSet derivedSourceDimensions = publisher.getDimensionHandleSet(derivedSourceX);
         JavaTckSupport.check(derivedSourceDimensions.size() == 1
               && derivedSourceDimensions.contains(publisherHandles.x)
               && !derivedSourceDimensions.contains(publisherHandles.y),
            "derived source region specification explicitly included Y instead of leaving it unspecified");

         RegionHandle baseReceiverX = createRegion(baseReceiver, baseRegions,
            new DimensionHandle[] {baseHandles.x}, new long[] {10}, new long[] {20},
            "BaseValue X receiver region");
         RegionHandle yReceiverHigh = createRegion(yReceiver, yRegions,
            new DimensionHandle[] {yHandles.y}, new long[] {yUpperBound - 1L},
            new long[] {yUpperBound}, "Y-only receiver region at the FDD default range's upper edge");
         sourcePairs = pairList(publisher, publisherHandles.inheritedBaseValue, baseSourceX,
            publisherHandles.derivedValue, derivedSourceX);
         baseSubscription = pairList(baseReceiver, baseHandles.inheritedBaseValue, baseReceiverX);
         ySubscription = pairList(yReceiver, yHandles.derivedValue, yReceiverHigh);
         baseReceiver.subscribeObjectClassAttributesWithRegions(
            baseHandles.derivedClass, baseSubscription);
         yReceiver.subscribeObjectClassAttributesWithRegions(
            yHandles.derivedClass, ySubscription);

         object = publisher.registerObjectInstanceWithRegions(
            publisherHandles.derivedClass, sourcePairs);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "FDD unspecified-dimension regional registration returned an invalid object handle");
         ObjectInstanceHandle registeredObject = object;
         await(callbackModel, new RTIambassador[] {baseReceiver, yReceiver},
            () -> hasDiscovery(baseCallbacks, registeredObject)
               && hasDiscovery(yCallbacks, registeredObject),
            "discovery for explicit X and implicit default-Y source dimensions");
         JavaTckSupport.check(baseCallbacks.discoveries.size() == 1
               && yCallbacks.discoveries.size() == 1
               && baseHandles.derivedClass.equals(baseCallbacks.discoveries.get(0).objectClass)
               && yHandles.derivedClass.equals(yCallbacks.discoveries.get(0).objectClass)
               && object.equals(baseCallbacks.discoveries.get(0).object)
               && object.equals(yCallbacks.discoveries.get(0).object),
            "unspecified-dimension discovery returned the wrong class or object handle");

         AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(2);
         byte[] baseValue = new byte[] {0x71, 0x72};
         byte[] derivedValue = new byte[] {(byte) 0x81, (byte) 0x82};
         byte[] tag = new byte[] {0x76, 0x77};
         values.put(publisherHandles.inheritedBaseValue, baseValue.clone());
         values.put(publisherHandles.derivedValue, derivedValue.clone());
         publisher.updateAttributeValues(object, values, tag.clone());

         await(callbackModel, new RTIambassador[] {baseReceiver, yReceiver},
            () -> !baseCallbacks.reflections.isEmpty() && !yCallbacks.reflections.isEmpty(),
            "FDD default-range routing for the omitted Y source dimension");
         settle(callbackModel, new RTIambassador[] {baseReceiver, yReceiver});
         requireOnlyAttributeReflection(baseCallbacks, object,
            baseHandles.inheritedBaseValue, baseHandles.derivedValue, baseValue, tag,
            publisherFederate, "X receiver for the inherited BaseValue source region");
         requireOnlyAttributeReflection(yCallbacks, object,
            yHandles.derivedValue, yHandles.inheritedBaseValue, derivedValue, tag,
            publisherFederate, "Y-only receiver reached through the FDD default range");
      } finally {
         if (joined[0] && object != null && sourcePairs != null) {
            try { publisher.unassociateRegionsForUpdates(object, sourcePairs); }
            catch (Exception ignored) { }
         }
         safeUnsubscribe(baseReceiver, baseHandles == null ? null : baseHandles.derivedClass,
            baseSubscription, joined[1]);
         safeUnsubscribe(yReceiver, yHandles == null ? null : yHandles.derivedClass,
            ySubscription, joined[2]);
         if (joined[0] && published && publisherHandles != null && publishedAttributes != null) {
            try {
               publisher.unpublishObjectClassAttributes(
                  publisherHandles.derivedClass, publishedAttributes);
            } catch (Exception ignored) { }
         }
         deleteRegions(publisher, publisherRegions, joined[0]);
         deleteRegions(baseReceiver, baseRegions, joined[1]);
         deleteRegions(yReceiver, yRegions, joined[2]);
         if (joined[2]) resign(yReceiver, ResignAction.NO_ACTION);
         if (joined[1]) resign(baseReceiver, ResignAction.NO_ACTION);
         if (joined[0]) resign(publisher, ResignAction.DELETE_OBJECTS);
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

   static void runExcludedDimensionNotRealizedFromAdapter() throws Exception {
      String fom = RtiTckMain.requireAdapterFile("hla.rti.tck.excludedDimensionsFom");
      runExcludedDimensionNotRealized(RtiTckMain.factory(), fom, RtiTckMain.callbackModel());
   }

   static void runExcludedDimensionNotRealized(RtiFactory factory, String fom,
         CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador xReceiver = factory.getRtiAmbassador();
      RTIambassador yReceiver = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder xCallbacks = new Recorder();
      Recorder yCallbacks = new Recorder();
      RTIambassador[] ambassadors = {publisher, xReceiver, yReceiver};
      Recorder[] recorders = {publisherCallbacks, xCallbacks, yCallbacks};
      boolean[] connected = new boolean[3];
      boolean[] joined = new boolean[3];
      boolean created = false;
      boolean published = false;
      String federation = "java-tck-regional-object-attribute-excluded-dimension-"
         + UUID.randomUUID();
      FederateHandle publisherFederate = null;
      ObjectClassHandle objectClass = null;
      ObjectClassHandle xObjectClass = null;
      ObjectClassHandle yObjectClass = null;
      AttributeHandle attribute = null;
      AttributeHandle xAttribute = null;
      AttributeHandle yAttribute = null;
      DimensionHandle x = null;
      DimensionHandle y = null;
      DimensionHandle xReceiverDimension = null;
      DimensionHandle yReceiverDimension = null;
      AttributeHandleSet attributes = null;
      AttributeSetRegionSetPairList sourcePairs = null;
      AttributeSetRegionSetPairList xSubscription = null;
      AttributeSetRegionSetPairList ySubscription = null;
      ObjectInstanceHandle object = null;
      List<RegionHandle> publisherRegions = new ArrayList<>();
      List<RegionHandle> xRegions = new ArrayList<>();
      List<RegionHandle> yRegions = new ArrayList<>();
      try {
         for (int index = 0; index < ambassadors.length; ++index) {
            ambassadors[index].connect(recorders[index].proxy(), callbackModel);
            connected[index] = true;
         }
         publisher.createFederationExecution(federation, fom);
         created = true;
         publisherFederate = publisher.joinFederationExecution(
            "attribute-excluded-dimension-publisher", federation);
         joined[0] = true;
         xReceiver.joinFederationExecution("attribute-excluded-dimension-x-receiver", federation);
         joined[1] = true;
         yReceiver.joinFederationExecution("attribute-excluded-dimension-y-receiver", federation);
         joined[2] = true;
         JavaTckSupport.check(publisherFederate != null && publisherFederate.encodedLength() > 0,
            "publisher join returned an invalid federate handle");

         objectClass = publisher.getObjectClassHandle("HLAobjectRoot.TckObject");
         xObjectClass = xReceiver.getObjectClassHandle("HLAobjectRoot.TckObject");
         yObjectClass = yReceiver.getObjectClassHandle("HLAobjectRoot.TckObject");
         attribute = publisher.getAttributeHandle(objectClass, "Value");
         xAttribute = xReceiver.getAttributeHandle(xObjectClass, "Value");
         yAttribute = yReceiver.getAttributeHandle(yObjectClass, "Value");
         x = publisher.getDimensionHandle("TckDimensionX");
         y = publisher.getDimensionHandle("TckDimensionY");
         xReceiverDimension = xReceiver.getDimensionHandle("TckDimensionX");
         yReceiverDimension = yReceiver.getDimensionHandle("TckDimensionY");
         JavaTckSupport.check(objectClass != null && objectClass.encodedLength() > 0
               && objectClass.equals(xObjectClass) && objectClass.equals(yObjectClass)
               && attribute != null && attribute.encodedLength() > 0
               && attribute.equals(xAttribute) && attribute.equals(yAttribute)
               && x != null && x.encodedLength() > 0 && y != null && y.encodedLength() > 0
               && x.equals(xReceiverDimension) && y.equals(yReceiverDimension),
            "members resolved inconsistent object-attribute excluded-dimension handles");
         DimensionHandleSet availableDimensions =
            publisher.getAvailableDimensionsForObjectClass(objectClass);
         JavaTckSupport.check(availableDimensions.size() == 2
               && availableDimensions.contains(x) && availableDimensions.contains(y)
               && publisher.getDimensionUpperBound(x) == 10L
               && publisher.getDimensionUpperBound(y) == 10L,
            "excluded-dimension object FOM did not expose X/Y with its declared bounds");

         attributes = JavaTckSupport.attributeSet(publisher, attribute);
         publisher.publishObjectClassAttributes(objectClass, attributes);
         published = true;
         RegionHandle sourceX = createRegion(publisher, publisherRegions,
            new DimensionHandle[] {x}, new long[] {1}, new long[] {4},
            "excluded-dimension X-only source");
         RegionHandle receiverX = createRegion(xReceiver, xRegions,
            new DimensionHandle[] {xReceiverDimension}, new long[] {2}, new long[] {5},
            "excluded-dimension X receiver");
         RegionHandle receiverY = createRegion(yReceiver, yRegions,
            new DimensionHandle[] {yReceiverDimension}, new long[] {1}, new long[] {4},
            "excluded-dimension Y receiver");
         sourcePairs = pairList(publisher, attribute, sourceX);
         xSubscription = pairList(xReceiver, xAttribute, receiverX);
         ySubscription = pairList(yReceiver, yAttribute, receiverY);
         xReceiver.subscribeObjectClassAttributesWithRegions(xObjectClass, xSubscription);
         yReceiver.subscribeObjectClassAttributesWithRegions(yObjectClass, ySubscription);
         object = publisher.registerObjectInstanceWithRegions(objectClass, sourcePairs);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "excluded-dimension regional registration returned an invalid object handle");
         ObjectInstanceHandle registeredObject = object;
         await(callbackModel, new RTIambassador[] {xReceiver},
            () -> hasDiscovery(xCallbacks, registeredObject),
            "X-overlapping object discovery with excluded Y omitted");
         settle(callbackModel, new RTIambassador[] {xReceiver, yReceiver});
         JavaTckSupport.check(xCallbacks.discoveries.size() == 1
               && objectClass.equals(xCallbacks.discoveries.get(0).objectClass)
               && object.equals(xCallbacks.discoveries.get(0).object)
               && yCallbacks.discoveries.isEmpty(),
            "FDD-excluded Y was realized for object discovery");

         AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(1);
         byte[] value = new byte[] {0x51, 0x52};
         byte[] tag = new byte[] {0x61, 0x62};
         values.put(attribute, value.clone());
         publisher.updateAttributeValues(object, values, tag.clone());
         await(callbackModel, new RTIambassador[] {xReceiver},
            () -> !xCallbacks.reflections.isEmpty(),
            "X-only positive-control reflection with Y excluded by the FDD");
         settle(callbackModel, new RTIambassador[] {xReceiver, yReceiver});
         JavaTckSupport.check(xCallbacks.reflections.size() == 1,
            "overlapping X receiver did not receive exactly one reflection");
         Reflection reflection = xCallbacks.reflections.get(0);
         byte[] reflectedValue = reflection.values.get(xAttribute);
         JavaTckSupport.check(object.equals(reflection.object)
               && reflection.values.size() == 1 && reflectedValue != null
               && Arrays.equals(reflectedValue, value) && Arrays.equals(reflection.tag, tag)
               && publisherFederate.equals(reflection.producer),
            "X positive control received the wrong object, value, tag, or producer");
         JavaTckSupport.check(yCallbacks.discoveries.isEmpty()
               && yCallbacks.reflections.isEmpty(),
            "FDD-excluded Y was implicitly realized for object discovery or update delivery");
      } finally {
         if (joined[0] && object != null && sourcePairs != null) {
            try { publisher.unassociateRegionsForUpdates(object, sourcePairs); }
            catch (Exception ignored) { }
         }
         safeUnsubscribe(xReceiver, xObjectClass, xSubscription, joined[1]);
         safeUnsubscribe(yReceiver, yObjectClass, ySubscription, joined[2]);
         if (joined[0] && published && objectClass != null && attributes != null) {
            try { publisher.unpublishObjectClassAttributes(objectClass, attributes); }
            catch (Exception ignored) { }
         }
         deleteRegions(publisher, publisherRegions, joined[0]);
         deleteRegions(xReceiver, xRegions, joined[1]);
         deleteRegions(yReceiver, yRegions, joined[2]);
         if (joined[2]) resign(yReceiver, ResignAction.NO_ACTION);
         if (joined[1]) resign(xReceiver, ResignAction.NO_ACTION);
         if (joined[0]) resign(publisher, ResignAction.DELETE_OBJECTS);
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

   private static Handles resolveHandles(RTIambassador ambassador) throws Exception {
      ObjectClassHandle baseClass = ambassador.getObjectClassHandle(BASE_CLASS);
      ObjectClassHandle derivedClass = ambassador.getObjectClassHandle(DERIVED_CLASS);
      AttributeHandle baseValue = ambassador.getAttributeHandle(baseClass, BASE_ATTRIBUTE);
      AttributeHandle inheritedBaseValue =
         ambassador.getAttributeHandle(derivedClass, BASE_ATTRIBUTE);
      AttributeHandle derivedValue = ambassador.getAttributeHandle(derivedClass, DERIVED_ATTRIBUTE);
      DimensionHandle x = ambassador.getDimensionHandle(X_DIMENSION);
      DimensionHandle y = ambassador.getDimensionHandle(Y_DIMENSION);
      JavaTckSupport.check(baseClass != null && baseClass.encodedLength() > 0
            && derivedClass != null && derivedClass.encodedLength() > 0
            && baseValue != null && baseValue.encodedLength() > 0
            && inheritedBaseValue != null && inheritedBaseValue.encodedLength() > 0
            && derivedValue != null && derivedValue.encodedLength() > 0
            && x != null && x.encodedLength() > 0 && y != null && y.encodedLength() > 0,
         "attribute-dimension FOM lookup returned an invalid handle");
      JavaTckSupport.check(baseValue.equals(inheritedBaseValue),
         "inherited BaseValue changed handle when resolved through the derived object class");
      DimensionHandleSet baseDimensions = ambassador.getAvailableDimensionsForObjectClass(baseClass);
      DimensionHandleSet derivedDimensions =
         ambassador.getAvailableDimensionsForObjectClass(derivedClass);
      JavaTckSupport.check(baseDimensions.size() == 1 && baseDimensions.contains(x)
            && !baseDimensions.contains(y) && derivedDimensions.size() == 2
            && derivedDimensions.contains(x) && derivedDimensions.contains(y),
         "base/derived FDD classes did not expose their declared X and X/Y dimensions");
      return new Handles(baseClass, derivedClass, baseValue, inheritedBaseValue,
         derivedValue, x, y);
   }

   private static void checkSameHandles(Handles expected, Handles actual, String member)
         throws Exception {
      JavaTckSupport.check(expected.baseClass.equals(actual.baseClass)
            && expected.derivedClass.equals(actual.derivedClass)
            && expected.baseValue.equals(actual.baseValue)
            && expected.inheritedBaseValue.equals(actual.inheritedBaseValue)
            && expected.derivedValue.equals(actual.derivedValue)
            && expected.x.equals(actual.x) && expected.y.equals(actual.y),
         member + " resolved inconsistent attribute-dimension FDD handles");
   }

   private static RegionHandle createRegion(RTIambassador ambassador,
         List<RegionHandle> createdRegions, DimensionHandle[] dimensions,
         long[] lowerBounds, long[] upperBounds, String description) throws Exception {
      JavaTckSupport.check(dimensions.length > 0 && dimensions.length == lowerBounds.length
            && dimensions.length == upperBounds.length,
         description + " fixture must provide one bound pair per region dimension");
      DimensionHandleSet regionDimensions = ambassador.getDimensionHandleSetFactory().create();
      for (DimensionHandle dimension : dimensions) regionDimensions.add(dimension);
      RegionHandle region = ambassador.createRegion(regionDimensions);
      createdRegions.add(region);
      for (int index = 0; index < dimensions.length; ++index) {
         ambassador.setRangeBounds(region, dimensions[index],
            new RangeBounds(lowerBounds[index], upperBounds[index]));
      }
      commitRegion(ambassador, region);
      DimensionHandleSet retainedDimensions = ambassador.getDimensionHandleSet(region);
      JavaTckSupport.check(retainedDimensions.size() == dimensions.length,
         description + " region retained the wrong number of dimensions");
      for (DimensionHandle dimension : dimensions) {
         JavaTckSupport.check(retainedDimensions.contains(dimension),
            description + " region lost one of its FDD-associated dimensions");
      }
      return region;
   }

   private static AttributeSetRegionSetPairList pairList(RTIambassador ambassador,
         AttributeHandle firstAttribute, RegionHandle firstRegion,
         AttributeHandle secondAttribute, RegionHandle secondRegion) throws Exception {
      AttributeSetRegionSetPairList result =
         ambassador.getAttributeSetRegionSetPairListFactory().create(2);
      result.add(new AttributeRegionAssociation(JavaTckSupport.attributeSet(
         ambassador, firstAttribute), regionSet(ambassador, firstRegion)));
      result.add(new AttributeRegionAssociation(JavaTckSupport.attributeSet(
         ambassador, secondAttribute), regionSet(ambassador, secondRegion)));
      return result;
   }

   private static AttributeSetRegionSetPairList pairList(RTIambassador ambassador,
         AttributeHandle attribute, RegionHandle region) throws Exception {
      AttributeSetRegionSetPairList result =
         ambassador.getAttributeSetRegionSetPairListFactory().create(1);
      result.add(new AttributeRegionAssociation(JavaTckSupport.attributeSet(ambassador, attribute),
         regionSet(ambassador, region)));
      return result;
   }

   private static RegionHandleSet regionSet(RTIambassador ambassador, RegionHandle region)
         throws Exception {
      RegionHandleSet result = ambassador.getRegionHandleSetFactory().create();
      result.add(region);
      return result;
   }

   private static void commitRegion(RTIambassador ambassador, RegionHandle region)
         throws Exception {
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      regions.add(region);
      ambassador.commitRegionModifications(regions);
   }

   private static boolean hasDiscovery(Recorder recorder, ObjectInstanceHandle object) {
      for (Discovery discovery : recorder.discoveries) {
         if (object.equals(discovery.object)) return true;
      }
      return false;
   }

   private static void requireOnlyAttributeReflection(Recorder recorder,
         ObjectInstanceHandle object, AttributeHandle expectedAttribute,
         AttributeHandle otherAttribute, byte[] expectedValue, byte[] expectedTag,
         FederateHandle expectedProducer, String description) {
      JavaTckSupport.check(recorder.reflections.size() == 1,
         description + " did not receive exactly one attribute reflection");
      Reflection reflection = recorder.reflections.get(0);
      byte[] receivedValue = reflection.values.get(expectedAttribute);
      JavaTckSupport.check(object.equals(reflection.object)
            && reflection.values.size() == 1 && receivedValue != null
            && !reflection.values.containsKey(otherAttribute)
            && Arrays.equals(receivedValue, expectedValue)
            && Arrays.equals(reflection.tag, expectedTag)
            && expectedProducer.equals(reflection.producer),
         description + " received the wrong object, attribute, value, tag, or producer");
   }

   private static void await(CallbackModel callbackModel, RTIambassador[] ambassadors,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            for (RTIambassador ambassador : ambassadors) ambassador.evokeCallback(0.01);
         } else {
            Thread.yield();
         }
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void settle(CallbackModel callbackModel, RTIambassador[] ambassadors)
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

   private static void deleteRegions(RTIambassador ambassador,
         List<RegionHandle> regions, boolean joined) {
      if (!joined) return;
      for (RegionHandle region : regions) {
         try { ambassador.deleteRegion(region); }
         catch (Exception ignored) { }
      }
   }

   private static void resign(RTIambassador ambassador, ResignAction action) {
      try { ambassador.resignFederationExecution(action); }
      catch (Exception ignored) { }
   }

   private static final class Handles {
      private final ObjectClassHandle baseClass;
      private final ObjectClassHandle derivedClass;
      private final AttributeHandle baseValue;
      private final AttributeHandle inheritedBaseValue;
      private final AttributeHandle derivedValue;
      private final DimensionHandle x;
      private final DimensionHandle y;

      private Handles(ObjectClassHandle baseClass, ObjectClassHandle derivedClass,
            AttributeHandle baseValue, AttributeHandle inheritedBaseValue,
            AttributeHandle derivedValue, DimensionHandle x, DimensionHandle y) {
         this.baseClass = baseClass;
         this.derivedClass = derivedClass;
         this.baseValue = baseValue;
         this.inheritedBaseValue = inheritedBaseValue;
         this.derivedValue = derivedValue;
         this.x = x;
         this.y = y;
      }
   }

   private static final class Discovery {
      private final ObjectInstanceHandle object;
      private final ObjectClassHandle objectClass;

      private Discovery(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         objectClass = (ObjectClassHandle) arguments[1];
      }
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final Map<AttributeHandle, byte[]> values = new LinkedHashMap<>();
      private final byte[] tag;
      private final FederateHandle producer;

      private Reflection(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         AttributeHandleValueMap received = (AttributeHandleValueMap) arguments[1];
         received.forEach((attribute, value) -> values.put(attribute, value.clone()));
         tag = ((byte[]) arguments[2]).clone();
         producer = (FederateHandle) arguments[4];
      }
   }

   private static final class Recorder {
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length == 4) {
                  discoveries.add(new Discovery(arguments));
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 6) {
                  reflections.add(new Reflection(arguments));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) {
                     return "Java TCK inherited object-attribute dimension callbacks";
                  }
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == arguments[0];
               }
               return null;
            });
      }
   }

   private static void safeUnsubscribe(RTIambassador ambassador, ObjectClassHandle objectClass,
         AttributeSetRegionSetPairList pairs, boolean joined) {
      if (joined && objectClass != null && pairs != null) {
         try { ambassador.unsubscribeObjectClassAttributesWithRegions(objectClass, pairs); }
         catch (Exception ignored) { }
      }
   }
}
