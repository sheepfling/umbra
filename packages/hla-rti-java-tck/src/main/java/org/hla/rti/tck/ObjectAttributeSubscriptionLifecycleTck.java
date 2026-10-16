package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Tests passive, active, downgraded, and withdrawn object-attribute subscriptions. */
final class ObjectAttributeSubscriptionLifecycleTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private ObjectAttributeSubscriptionLifecycleTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String attributeName, String secondaryAttributeName,
         CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder subscriberCallbacks = new Recorder();
      String federation = "java-tck-object-attribute-subscriptions-" + UUID.randomUUID();
      boolean publisherConnected = false, subscriberConnected = false, created = false;
      boolean publisherJoined = false, subscriberJoined = false;
      boolean published = false, subscribed = false;
      ObjectClassHandle publisherClass = null, subscriberClass = null;
      AttributeHandle publisherAttribute = null, subscriberAttribute = null;
      AttributeHandle publisherSecondaryAttribute = null, subscriberSecondaryAttribute = null;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         subscriber.connect(subscriberCallbacks.proxy(), callbackModel);
         subscriberConnected = true;
         publisher.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-object-subscription-publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution("java-tck-object-subscription-subscriber", federation);
         subscriberJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         subscriberClass = subscriber.getObjectClassHandle(objectClassName);
         publisherAttribute = publisher.getAttributeHandle(publisherClass, attributeName);
         subscriberAttribute = subscriber.getAttributeHandle(subscriberClass, attributeName);
         publisherSecondaryAttribute = publisher.getAttributeHandle(
            publisherClass, secondaryAttributeName);
         subscriberSecondaryAttribute = subscriber.getAttributeHandle(
            subscriberClass, secondaryAttributeName);
         JavaTckSupport.check(publisherClass != null && subscriberClass != null
               && publisherAttribute != null && subscriberAttribute != null
               && publisherSecondaryAttribute != null && subscriberSecondaryAttribute != null
               && !publisherAttribute.equals(publisherSecondaryAttribute)
               && !subscriberAttribute.equals(subscriberSecondaryAttribute),
            "adapter FOM did not resolve object-attribute subscription lifecycle handles");
         AttributeHandleSet publisherAttributes = JavaTckSupport.attributeSet(
            publisher, publisherAttribute, publisherSecondaryAttribute);
         AttributeHandleSet subscriberAttributes = JavaTckSupport.attributeSet(
            subscriber, subscriberAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;

         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
             "pre-subscription registration returned an invalid object handle");

         byte[] initialValue = new byte[] {0x49, 0x4e, 0x49};
         byte[] initialFilteredValue = new byte[] {0x46, 0x49, 0x4c};
         byte[] initialTag = new byte[] {0x49, 0x30};
         sendUpdate(publisher, publisherAttribute, publisherSecondaryAttribute, object,
            initialValue, initialFilteredValue, initialTag);

         ObjectClassHandle momFederateClass = subscriber.getObjectClassHandle(
            "HLAobjectRoot.HLAmanager.HLAfederate");
         JavaTckSupport.check(momFederateClass != null
               && !subscriberClass.equals(momFederateClass),
            "standard MOM fixture did not provide a distinct HLAfederate class");
         AttributeHandle momFederateHandleAttribute = subscriber.getAttributeHandle(
            momFederateClass, "HLAfederateHandle");
         JavaTckSupport.check(momFederateHandleAttribute != null,
            "standard MOM fixture did not provide the HLAfederateHandle attribute");
         ObjectClassHandle objectClassForInvalidSubscribe = subscriberClass;
         AttributeHandleSet outOfClassAttributes = JavaTckSupport.attributeSet(
            subscriber, momFederateHandleAttribute);
         JavaTckSupport.expectFailure(
            () -> subscriber.subscribeObjectClassAttributes(objectClassForInvalidSubscribe,
               outOfClassAttributes),
            "AttributeNotDefined",
            "subscribing with an attribute handle from the unrelated standard HLAfederate class");
         pumpQuiet(subscriber, callbackModel);
         JavaTckSupport.check(subscriberCallbacks.discoveries.isEmpty()
               && subscriberCallbacks.reflections.isEmpty(),
            "rejected out-of-class subscription changed discovery or reflection state");

         AttributeHandleSet emptyAttributes = JavaTckSupport.attributeSet(subscriber);
         subscriber.subscribeObjectClassAttributes(subscriberClass, emptyAttributes);
         pumpQuiet(subscriber, callbackModel);
         JavaTckSupport.check(subscriberCallbacks.discoveries.isEmpty()
               && subscriberCallbacks.reflections.isEmpty(),
            "empty active object-attribute subscription affected a pre-existing object");

         subscriber.subscribeObjectClassAttributesPassively(subscriberClass, subscriberAttributes);
         subscribed = true;
         pumpQuiet(subscriber, callbackModel);
         JavaTckSupport.check(subscriberCallbacks.discoveries.isEmpty(),
            "passive object-attribute subscription unexpectedly discovered an object");

         ObjectInstanceHandle laterObject = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(laterObject != null && laterObject.encodedLength() > 0,
            "passive-subscription registration returned an invalid object handle");
         pumpQuiet(subscriber, callbackModel);
         JavaTckSupport.check(subscriberCallbacks.discoveries.isEmpty()
               && subscriberCallbacks.reflections.isEmpty(),
            "passive object-attribute subscription affected a later registration");

         subscriber.subscribeObjectClassAttributes(subscriberClass, subscriberAttributes);
         await(subscriber, callbackModel,
            () -> subscriberCallbacks.discoveries.size() == 2,
            "active object-attribute subscription discovery");
         Discovery discovery = findDiscovery(subscriberCallbacks.discoveries, object);
         Discovery laterDiscovery = findDiscovery(subscriberCallbacks.discoveries, laterObject);
         JavaTckSupport.check(discovery != null && laterDiscovery != null
               && subscriberClass.equals(discovery.objectClass)
               && subscriberClass.equals(laterDiscovery.objectClass),
            "active object-attribute subscription returned the wrong discovery identities");
         String objectName = publisher.getObjectInstanceName(object);
         JavaTckSupport.check(objectName != null && !objectName.isEmpty()
               && object.equals(subscriber.getObjectInstanceHandle(objectName))
               && subscriberClass.equals(subscriber.getKnownObjectClassHandle(object)),
            "active object-attribute subscription did not establish stable identity lookups");
         String laterObjectName = publisher.getObjectInstanceName(laterObject);
         JavaTckSupport.check(laterObjectName != null && !laterObjectName.isEmpty()
               && laterObject.equals(subscriber.getObjectInstanceHandle(laterObjectName))
               && subscriberClass.equals(subscriber.getKnownObjectClassHandle(laterObject)),
            "active object-attribute subscription did not identify the later registration");

         await(subscriber, callbackModel, () -> !subscriberCallbacks.reflections.isEmpty(),
            "initial active object-attribute subscription reflection");
         assertReflection(subscriberCallbacks.reflections.get(0), object, subscriberAttribute,
            subscriberSecondaryAttribute, initialValue, initialTag, producer,
            "initial active object-attribute subscription");
         subscriberCallbacks.reflections.clear();

         byte[] activeValue = new byte[] {0x41, 0x43, 0x54};
         byte[] activeFilteredValue = new byte[] {0x46, 0x49, 0x4c};
         byte[] activeTag = new byte[] {0x41, 0x31};
         sendUpdate(publisher, publisherAttribute, publisherSecondaryAttribute, object,
            activeValue, activeFilteredValue, activeTag);
         await(subscriber, callbackModel, () -> !subscriberCallbacks.reflections.isEmpty(),
            "active object-attribute subscription reflection");
         assertReflection(subscriberCallbacks.reflections.get(0), object, subscriberAttribute,
            subscriberSecondaryAttribute, activeValue, activeTag, producer,
            "active object-attribute subscription");

         subscriberCallbacks.reflections.clear();
         subscriber.subscribeObjectClassAttributesPassively(subscriberClass, subscriberAttributes);
         byte[] downgradedValue = new byte[] {0x44, 0x4f, 0x57};
         byte[] downgradedFilteredValue = new byte[] {0x46, 0x49, 0x4c};
         byte[] downgradedTag = new byte[] {0x44, 0x32};
         sendUpdate(publisher, publisherAttribute, publisherSecondaryAttribute, object,
            downgradedValue, downgradedFilteredValue, downgradedTag);
         pumpQuiet(subscriber, callbackModel);
         JavaTckSupport.check(subscriberCallbacks.reflections.isEmpty(),
            "passive object-attribute downgrade did not suppress reflection");
         JavaTckSupport.check(subscriberClass.equals(subscriber.getKnownObjectClassHandle(object)),
            "passive downgrade discarded the known object identity");

         subscriber.subscribeObjectClassAttributes(subscriberClass, subscriberAttributes);
         byte[] reactivatedValue = new byte[] {0x52, 0x45, 0x41};
         byte[] reactivatedFilteredValue = new byte[] {0x46, 0x49, 0x4c};
         byte[] reactivatedTag = new byte[] {0x52, 0x33};
         sendUpdate(publisher, publisherAttribute, publisherSecondaryAttribute, object,
            reactivatedValue, reactivatedFilteredValue, reactivatedTag);
         await(subscriber, callbackModel, () -> !subscriberCallbacks.reflections.isEmpty(),
            "reactivated object-attribute subscription reflection");
         assertReflection(subscriberCallbacks.reflections.get(0), object, subscriberAttribute,
            subscriberSecondaryAttribute, reactivatedValue, reactivatedTag, producer,
            "reactivated object-attribute subscription");

         subscriberCallbacks.reflections.clear();
         subscriber.unsubscribeObjectClassAttributes(subscriberClass, subscriberAttributes);
         subscribed = false;
         byte[] withdrawnValue = new byte[] {0x57, 0x49, 0x54};
         byte[] withdrawnFilteredValue = new byte[] {0x46, 0x49, 0x4c};
         byte[] withdrawnTag = new byte[] {0x57, 0x34};
         sendUpdate(publisher, publisherAttribute, publisherSecondaryAttribute, object,
            withdrawnValue, withdrawnFilteredValue, withdrawnTag);
         pumpQuiet(subscriber, callbackModel);
         JavaTckSupport.check(subscriberCallbacks.reflections.isEmpty(),
            "unsubscribed object-attribute declaration unexpectedly reflected an update");
      } finally {
         if (subscriberJoined && subscribed) {
            try { subscriber.unsubscribeObjectClassAttributes(subscriberClass,
               JavaTckSupport.attributeSet(subscriber, subscriberAttribute)); }
            catch (Exception ignored) { }
         }
         if (publisherJoined && published) {
            try { publisher.unpublishObjectClassAttributes(publisherClass,
               JavaTckSupport.attributeSet(publisher, publisherAttribute,
                  publisherSecondaryAttribute)); }
            catch (Exception ignored) { }
         }
         if (subscriberJoined) {
            try { subscriber.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (subscriberConnected) {
            try { subscriber.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   static void runPassiveSubscriptionDoesNotStartRegistration(RtiFactory factory, String fom,
         String timeImplementation, String objectClassName, String attributeName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder subscriberCallbacks = new Recorder();
      String federation = "java-tck-passive-subscription-no-start-registration-"
         + UUID.randomUUID();
      boolean publisherConnected = false, subscriberConnected = false, created = false;
      boolean publisherJoined = false, subscriberJoined = false;
      boolean published = false, subscribed = false;
      ObjectClassHandle publisherClass = null, subscriberClass = null;
      AttributeHandle publisherAttribute = null, subscriberAttribute = null;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         subscriber.connect(subscriberCallbacks.proxy(), callbackModel);
         subscriberConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         publisher.joinFederationExecution("java-tck-passive-subscription-publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution("java-tck-passive-subscription-subscriber", federation);
         subscriberJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         subscriberClass = subscriber.getObjectClassHandle(objectClassName);
         publisherAttribute = publisher.getAttributeHandle(publisherClass, attributeName);
         subscriberAttribute = subscriber.getAttributeHandle(subscriberClass, attributeName);
         JavaTckSupport.check(publisherClass != null && subscriberClass != null
               && publisherAttribute != null && subscriberAttribute != null,
            "adapter FOM did not resolve passive-subscription advisory handles");

         AttributeHandleSet publisherAttributes = JavaTckSupport.attributeSet(
            publisher, publisherAttribute);
         AttributeHandleSet subscriberAttributes = JavaTckSupport.attributeSet(
            subscriber, subscriberAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;

         subscriber.subscribeObjectClassAttributes(subscriberClass, subscriberAttributes);
         subscribed = true;
         await(publisher, callbackModel,
            () -> !publisherCallbacks.registrationStarts.isEmpty(),
            "active-subscription Start Registration positive control");
         JavaTckSupport.check(publisherCallbacks.registrationStarts.size() == 1
               && publisherClass.equals(publisherCallbacks.registrationStarts.get(0)),
            "active-subscription positive control reported an unexpected object class");

         subscriber.unsubscribeObjectClassAttributes(subscriberClass, subscriberAttributes);
         subscribed = false;
         pumpQuiet(publisher, callbackModel);
         publisherCallbacks.registrationStarts.clear();

         subscriber.subscribeObjectClassAttributesPassively(subscriberClass, subscriberAttributes);
         subscribed = true;
         pumpQuiet(publisher, callbackModel);
         JavaTckSupport.check(publisherCallbacks.registrationStarts.isEmpty(),
            "passive subscription caused Start Registration at the publisher");

         subscriber.unsubscribeObjectClassAttributes(subscriberClass, subscriberAttributes);
         subscribed = false;
         publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         published = false;
         subscriber.resignFederationExecution(ResignAction.NO_ACTION);
         subscriberJoined = false;
         publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (subscriberJoined && subscribed && subscriberClass != null
               && subscriberAttribute != null) {
            try { subscriber.unsubscribeObjectClassAttributes(subscriberClass,
               JavaTckSupport.attributeSet(subscriber, subscriberAttribute)); }
            catch (Exception ignored) { }
         }
         if (publisherJoined && published && publisherClass != null
               && publisherAttribute != null) {
            try { publisher.unpublishObjectClassAttributes(publisherClass,
               JavaTckSupport.attributeSet(publisher, publisherAttribute)); }
            catch (Exception ignored) { }
         }
         if (subscriberJoined) {
            try { subscriber.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (subscriberConnected) {
            try { subscriber.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   static void runAccumulatingAttributeSubscriptionInvocations(RtiFactory factory, String fom,
         String timeImplementation, String objectClassName, String firstAttributeName,
         String secondAttributeName, CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder subscriberCallbacks = new Recorder();
      String federation = "java-tck-attribute-subscription-accumulate-" + UUID.randomUUID();
      boolean publisherConnected = false, subscriberConnected = false, created = false;
      boolean publisherJoined = false, subscriberJoined = false;
      boolean published = false, subscribed = false;
      ObjectClassHandle publisherClass = null, subscriberClass = null;
      AttributeHandle publisherFirst = null, publisherSecond = null;
      AttributeHandle subscriberFirst = null, subscriberSecond = null;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         subscriber.connect(subscriberCallbacks.proxy(), callbackModel);
         subscriberConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         publisher.joinFederationExecution("java-tck-attribute-accumulate-publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution("java-tck-attribute-accumulate-subscriber", federation);
         subscriberJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         subscriberClass = subscriber.getObjectClassHandle(objectClassName);
         publisherFirst = publisher.getAttributeHandle(publisherClass, firstAttributeName);
         publisherSecond = publisher.getAttributeHandle(publisherClass, secondAttributeName);
         subscriberFirst = subscriber.getAttributeHandle(subscriberClass, firstAttributeName);
         subscriberSecond = subscriber.getAttributeHandle(subscriberClass, secondAttributeName);
         JavaTckSupport.check(publisherClass != null && subscriberClass != null
               && publisherFirst != null && publisherSecond != null
               && subscriberFirst != null && subscriberSecond != null
               && !publisherFirst.equals(publisherSecond)
               && !subscriberFirst.equals(subscriberSecond),
            "adapter FOM did not resolve distinct subscription attributes");

         AttributeHandleSet publisherAttributes = JavaTckSupport.attributeSet(
            publisher, publisherFirst, publisherSecond);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         subscriber.subscribeObjectClassAttributes(subscriberClass,
            JavaTckSupport.attributeSet(subscriber, subscriberFirst));
         subscribed = true;

         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "accumulating-subscription registration returned an invalid object handle");
         await(subscriber, callbackModel,
            () -> findDiscovery(subscriberCallbacks.discoveries, object) != null,
            "first attribute subscription discovery");

         subscriber.subscribeObjectClassAttributes(subscriberClass,
            JavaTckSupport.attributeSet(subscriber, subscriberSecond));
         byte[] firstValue = new byte[] {0x41, 0x43, 0x43, 0x31};
         byte[] secondValue = new byte[] {0x41, 0x43, 0x43, 0x32};
         byte[] tag = new byte[] {0x41, 0x44, 0x44};
         sendUpdate(publisher, publisherFirst, publisherSecond, object,
            firstValue, secondValue, tag);
         await(subscriber, callbackModel,
            () -> !subscriberCallbacks.reflections.isEmpty(),
            "accumulated attribute subscription reflection");

         JavaTckSupport.check(subscriberCallbacks.reflections.size() == 1,
            "accumulating subscription produced an unexpected reflection count");
         Reflection reflection = subscriberCallbacks.reflections.get(0);
         JavaTckSupport.check(object.equals(reflection.object)
               && reflection.values.size() == 2
               && reflection.values.containsKey(subscriberFirst)
               && reflection.values.containsKey(subscriberSecond)
               && Arrays.equals(firstValue, reflection.values.get(subscriberFirst))
               && Arrays.equals(secondValue, reflection.values.get(subscriberSecond))
               && Arrays.equals(tag, reflection.tag),
            "the later class subscription replaced rather than accumulated the earlier set");

         subscriber.unsubscribeObjectClassAttributes(subscriberClass,
            JavaTckSupport.attributeSet(subscriber, subscriberFirst, subscriberSecond));
         subscribed = false;
         publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         published = false;
         subscriber.resignFederationExecution(ResignAction.NO_ACTION);
         subscriberJoined = false;
         publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (subscriberJoined && subscribed && subscriberClass != null
               && subscriberFirst != null && subscriberSecond != null) {
            try { subscriber.unsubscribeObjectClassAttributes(subscriberClass,
               JavaTckSupport.attributeSet(subscriber, subscriberFirst, subscriberSecond)); }
            catch (Exception ignored) { }
         }
         if (publisherJoined && published && publisherClass != null
               && publisherFirst != null && publisherSecond != null) {
            try { publisher.unpublishObjectClassAttributes(publisherClass,
               JavaTckSupport.attributeSet(publisher, publisherFirst, publisherSecond)); }
            catch (Exception ignored) { }
         }
         if (subscriberJoined) {
            try { subscriber.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (subscriberConnected) {
            try { subscriber.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   static void runEmptyAttributeSubscriptionNoOp(RtiFactory factory, String fom, String mim,
         String timeImplementation, String objectClassName, String attributeName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder subscriberCallbacks = new Recorder();
      String federation = "java-tck-empty-attribute-subscription-" + UUID.randomUUID();
      boolean publisherConnected = false, subscriberConnected = false, created = false;
      boolean publisherJoined = false, subscriberJoined = false, subscribed = false;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         subscriber.connect(subscriberCallbacks.proxy(), callbackModel);
         subscriberConnected = true;
         publisher.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-empty-subscription-publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution("java-tck-empty-subscription-subscriber", federation);
         subscriberJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle subscriberClass = subscriber.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         AttributeHandle subscriberAttribute = subscriber.getAttributeHandle(
            subscriberClass, attributeName);
         JavaTckSupport.check(publisherClass != null && subscriberClass != null
               && publisherAttribute != null && subscriberAttribute != null,
            "empty-subscription lookup returned a null class or attribute handle");

         AttributeHandleSet publishedAttributes =
            JavaTckSupport.attributeSet(publisher, publisherAttribute);
         AttributeHandleSet subscribedAttributes =
            JavaTckSupport.attributeSet(subscriber, subscriberAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publishedAttributes);
         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "empty-subscription registration returned an invalid object handle");

         byte[] initialValue = new byte[] {0x45, 0x4d, 0x50, 0x54};
         byte[] initialTag = new byte[] {0x49, 0x4e, 0x49};
         sendSingleUpdate(publisher, publisherAttribute, object, initialValue, initialTag);
         subscriber.subscribeObjectClassAttributes(subscriberClass, subscribedAttributes);
         subscribed = true;
         await(subscriber, callbackModel,
            () -> findDiscovery(subscriberCallbacks.discoveries, object) != null,
            "empty-subscription initial discovery");
         await(subscriber, callbackModel, () -> {
            for (Reflection reflection : subscriberCallbacks.reflections) {
               if (object.equals(reflection.object)
                     && reflection.values.containsKey(subscriberAttribute)) return true;
            }
            return false;
         }, "empty-subscription initial reflection");

         subscriber.subscribeObjectClassAttributes(
            subscriberClass, JavaTckSupport.attributeSet(subscriber));
         byte[] nextValue = new byte[] {0x4e, 0x4f, 0x2d, 0x4f};
         byte[] nextTag = new byte[] {0x4e, 0x45, 0x58};
         sendSingleUpdate(publisher, publisherAttribute, object, nextValue, nextTag);
         await(subscriber, callbackModel,
            () -> subscriberCallbacks.reflections.size() >= 2,
            "empty-subscription retained active update delivery");

         List<Reflection> reflections = subscriberCallbacks.reflections;
         Reflection last = reflections.get(reflections.size() - 1);
         JavaTckSupport.check(reflections.size() == 2 && object.equals(last.object)
               && last.values.size() == 1 && last.values.containsKey(subscriberAttribute)
               && Arrays.equals(nextValue, last.values.get(subscriberAttribute))
               && Arrays.equals(nextTag, last.tag),
            "empty attribute set changed the existing active subscription");

         subscriber.unsubscribeObjectClassAttributes(subscriberClass, subscribedAttributes);
         subscribed = false;
         subscriber.resignFederationExecution(ResignAction.NO_ACTION);
         subscriberJoined = false;
         publisher.resignFederationExecution(ResignAction.NO_ACTION);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (subscribed && subscriberJoined) {
            try {
               ObjectClassHandle objectClass = subscriber.getObjectClassHandle(objectClassName);
               AttributeHandle attribute = subscriber.getAttributeHandle(objectClass, attributeName);
               subscriber.unsubscribeObjectClassAttributes(
                  objectClass, JavaTckSupport.attributeSet(subscriber, attribute));
            } catch (Exception ignored) { }
         }
         if (subscriberJoined) {
            try { subscriber.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (subscriberConnected) {
            try { subscriber.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void sendSingleUpdate(RTIambassador publisher, AttributeHandle attribute,
         ObjectInstanceHandle object, byte[] value, byte[] tag) throws Exception {
      AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(1);
      values.put(attribute, value);
      publisher.updateAttributeValues(object, values, tag);
   }

   private static void sendUpdate(RTIambassador publisher, AttributeHandle attribute,
         AttributeHandle filteredAttribute, ObjectInstanceHandle object, byte[] value,
         byte[] filteredValue, byte[] tag) throws Exception {
      AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(2);
      values.put(attribute, value);
      values.put(filteredAttribute, filteredValue);
      publisher.updateAttributeValues(object, values, tag);
   }

   private static Discovery findDiscovery(List<Discovery> discoveries,
         ObjectInstanceHandle object) {
      for (Discovery discovery : discoveries) {
         if (object.equals(discovery.object)) return discovery;
      }
      return null;
   }

   private static void assertReflection(Reflection reflection, ObjectInstanceHandle object,
         AttributeHandle attribute, AttributeHandle excludedAttribute, byte[] value, byte[] tag,
         FederateHandle producer, String description) {
      JavaTckSupport.check(object.equals(reflection.object),
         description + " returned the wrong object handle");
      JavaTckSupport.check(reflection.values.size() == 1
            && reflection.values.containsKey(attribute)
            && !reflection.values.containsKey(excludedAttribute)
            && Arrays.equals(value, reflection.values.get(attribute)),
         description + " returned the wrong attribute set or value");
      JavaTckSupport.check(Arrays.equals(tag, reflection.tag),
         description + " returned the wrong user tag");
      JavaTckSupport.check(producer.equals(reflection.producer),
         description + " returned the wrong producing federate");
      JavaTckSupport.check(reflection.transportation != null && reflection.transportation.encodedLength() > 0,
         description + " returned an invalid transportation handle");
      JavaTckSupport.check(reflection.regionsEmpty,
         description + " unexpectedly returned source regions");
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void pumpQuiet(RTIambassador ambassador, CallbackModel callbackModel)
         throws Exception {
      long deadline = System.nanoTime() + 150_000_000L;
      while (System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.sleep(2L);
      }
   }

   private static final class Discovery {
      private final ObjectInstanceHandle object;
      private final ObjectClassHandle objectClass;

      private Discovery(ObjectInstanceHandle object, ObjectClassHandle objectClass) {
         this.object = object;
         this.objectClass = objectClass;
      }
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final Map<AttributeHandle, byte[]> values;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final boolean regionsEmpty;

      private Reflection(ObjectInstanceHandle object, Map<AttributeHandle, byte[]> values,
            byte[] tag, TransportationTypeHandle transportation, FederateHandle producer,
            boolean regionsEmpty) {
         this.object = object;
         this.values = values;
         this.tag = tag;
         this.transportation = transportation;
         this.producer = producer;
         this.regionsEmpty = regionsEmpty;
      }
   }

   private static final class Recorder {
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<ObjectClassHandle> registrationStarts = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("startRegistrationForObjectClass".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  registrationStarts.add((ObjectClassHandle) arguments[0]);
               }
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 2) {
                  discoveries.add(new Discovery((ObjectInstanceHandle) arguments[0],
                     (ObjectClassHandle) arguments[1]));
               }
               if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length >= 5) {
                  Map<AttributeHandle, byte[]> values = new LinkedHashMap<>();
                  AttributeHandleValueMap map = (AttributeHandleValueMap) arguments[1];
                  for (AttributeHandle attribute : map.keySet()) {
                     values.put(attribute, map.get(attribute).clone());
                  }
                  byte[] tag = arguments[2] instanceof byte[] ? ((byte[]) arguments[2]).clone()
                     : new byte[0];
                  boolean regionsEmpty = arguments.length < 6 || arguments[5] == null
                     || ((RegionHandleSet) arguments[5]).isEmpty();
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0], values, tag,
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     regionsEmpty));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK object-attribute subscription callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
