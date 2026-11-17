package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies hierarchy-aware registration discovery and subscription eligibility. */
final class ObjectRegistrationDiscoveryLifecycleTck {
   private static final int PUBLISHER = 0;
   private static final int EXACT = 1;
   private static final int BASE = 2;
   private static final int CANCELLED = 3;
   private static final int LATE = 4;
   private static final int UNRELATED = 5;
   private static final int FEDERATE_COUNT = 6;

   private ObjectRegistrationDiscoveryLifecycleTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String baseClassName,
         String derivedClassName, String identityAttributeName,
         String derivedAttributeName, String unrelatedClassName,
         String unrelatedAttributeName, CallbackModel callbackModel) throws Exception {
      RTIambassador[] rti = new RTIambassador[FEDERATE_COUNT];
      Recorder[] recorders = new Recorder[FEDERATE_COUNT];
      boolean[] connected = new boolean[FEDERATE_COUNT];
      boolean[] joined = new boolean[FEDERATE_COUNT];
      boolean[] subscribed = new boolean[FEDERATE_COUNT];
      boolean published = false;
      boolean federationCreated = false;
      String federation = "java-tck-object-registration-discovery-" + UUID.randomUUID();
      String[] roles = {"publisher", "exact", "base", "cancelled", "late", "unrelated"};
      try {
         for (int i = 0; i < FEDERATE_COUNT; ++i) {
            rti[i] = factory.getRtiAmbassador();
            recorders[i] = new Recorder();
            rti[i].connect(recorders[i].proxy(), callbackModel);
            connected[i] = true;
         }
         rti[PUBLISHER].createFederationExecution(federation, fom, timeImplementation);
         federationCreated = true;
         String publisherName = "java-tck-object-registration-publisher";
         for (int i = 0; i < FEDERATE_COUNT; ++i) {
            rti[i].joinFederationExecution(
               "java-tck-object-registration-" + roles[i],
               "registration-" + roles[i], federation);
            joined[i] = true;
         }

         JavaTckSupport.expectFailure(
            () -> rti[PUBLISHER].getObjectClassHandle(
               baseClassName + ".TckMissingFddClass"),
            "NameNotFound", "resolving an object class absent from the FDD vocabulary");

         ObjectClassHandle[] derivedClasses = new ObjectClassHandle[FEDERATE_COUNT];
         ObjectClassHandle[] baseClasses = new ObjectClassHandle[FEDERATE_COUNT];
         AttributeHandle[] identityAttributes = new AttributeHandle[FEDERATE_COUNT];
         AttributeHandle[] derivedAttributes = new AttributeHandle[FEDERATE_COUNT];
         for (int i = 0; i < FEDERATE_COUNT; ++i) {
            derivedClasses[i] = rti[i].getObjectClassHandle(derivedClassName);
            baseClasses[i] = rti[i].getObjectClassHandle(baseClassName);
            identityAttributes[i] = rti[i].getAttributeHandle(
               derivedClasses[i], identityAttributeName);
            derivedAttributes[i] = rti[i].getAttributeHandle(
               derivedClasses[i], derivedAttributeName);
            JavaTckSupport.check(derivedClasses[i] != null && baseClasses[i] != null
                  && !derivedClasses[i].equals(baseClasses[i])
                  && identityAttributes[i] != null && derivedAttributes[i] != null
                  && derivedClassName.equals(rti[i].getObjectClassName(derivedClasses[i]))
                  && baseClassName.equals(rti[i].getObjectClassName(baseClasses[i]))
                  && identityAttributeName.equals(rti[i].getAttributeName(
                     derivedClasses[i], identityAttributes[i]))
                  && derivedAttributeName.equals(rti[i].getAttributeName(
                     derivedClasses[i], derivedAttributes[i])),
               "typed object registration FOM lookup returned a null handle");
         }
         ObjectClassHandle unrelatedClass = rti[UNRELATED].getObjectClassHandle(
            unrelatedClassName);
         AttributeHandle unrelatedAttribute = rti[UNRELATED].getAttributeHandle(
            unrelatedClass, unrelatedAttributeName);
         JavaTckSupport.check(unrelatedClass != null && unrelatedAttribute != null
               && unrelatedClassName.equals(rti[UNRELATED].getObjectClassName(unrelatedClass))
               && unrelatedAttributeName.equals(
                  rti[UNRELATED].getAttributeName(unrelatedClass, unrelatedAttribute)),
            "adapter FOM did not resolve the unrelated subscriber class and attribute");
         JavaTckSupport.expectFailure(
            () -> rti[PUBLISHER].getAttributeHandle(
               baseClasses[PUBLISHER], derivedAttributeName),
            "NameNotFound", "resolving a derived-only attribute on its superclass");
         JavaTckSupport.check(rti[PUBLISHER].getAttributeHandle(
               baseClasses[PUBLISHER], identityAttributeName)
               .equals(identityAttributes[PUBLISHER]),
            "inherited identity attribute handle changed between base and derived classes");

         JavaTckSupport.expectFailure(
            () -> rti[PUBLISHER].registerObjectInstance(derivedClasses[PUBLISHER]),
            "ObjectClassNotPublished",
            "registering the derived object class before publication");
         rti[PUBLISHER].publishObjectClassAttributes(
            derivedClasses[PUBLISHER], JavaTckSupport.attributeSet(rti[PUBLISHER]));
         JavaTckSupport.expectFailure(
            () -> rti[PUBLISHER].registerObjectInstance(derivedClasses[PUBLISHER]),
            "ObjectClassNotPublished",
            "registering the derived object class after publishing an empty attribute set");

         AttributeHandleSet publisherAttributes = JavaTckSupport.attributeSet(
            rti[PUBLISHER], identityAttributes[PUBLISHER], derivedAttributes[PUBLISHER]);
         AttributeHandleSet exactAttributes = JavaTckSupport.attributeSet(
            rti[EXACT], derivedAttributes[EXACT]);
         AttributeHandleSet baseAttributes = JavaTckSupport.attributeSet(
            rti[BASE], identityAttributes[BASE]);
         AttributeHandleSet cancelledAttributes = JavaTckSupport.attributeSet(
            rti[CANCELLED], derivedAttributes[CANCELLED]);
         AttributeHandleSet lateAttributes = JavaTckSupport.attributeSet(
            rti[LATE], derivedAttributes[LATE]);
         AttributeHandleSet unrelatedAttributes = JavaTckSupport.attributeSet(
            rti[UNRELATED], unrelatedAttribute);

         JavaTckSupport.expectFailure(
            () -> rti[PUBLISHER].publishObjectClassAttributes(
               baseClasses[PUBLISHER],
               JavaTckSupport.attributeSet(rti[PUBLISHER], derivedAttributes[PUBLISHER])),
            "AttributeNotDefined", "publishing a derived-only attribute through its superclass");
         JavaTckSupport.expectFailure(
            () -> rti[BASE].subscribeObjectClassAttributes(
               baseClasses[BASE],
               JavaTckSupport.attributeSet(rti[BASE], derivedAttributes[BASE])),
            "AttributeNotDefined", "subscribing to a derived-only attribute through its superclass");

         rti[EXACT].subscribeObjectClassAttributes(derivedClasses[EXACT], exactAttributes);
         subscribed[EXACT] = true;
         rti[BASE].subscribeObjectClassAttributes(baseClasses[BASE], baseAttributes);
         subscribed[BASE] = true;
         rti[CANCELLED].subscribeObjectClassAttributes(
            derivedClasses[CANCELLED], cancelledAttributes);
         subscribed[CANCELLED] = true;
         rti[UNRELATED].subscribeObjectClassAttributes(unrelatedClass, unrelatedAttributes);
         subscribed[UNRELATED] = true;
         rti[PUBLISHER].publishObjectClassAttributes(
            derivedClasses[PUBLISHER], publisherAttributes);
         published = true;

         ObjectInstanceHandle object = rti[PUBLISHER].registerObjectInstance(
            derivedClasses[PUBLISHER]);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "object registration returned an invalid handle");
         JavaTckSupport.check(
            rti[PUBLISHER].isAttributeOwnedByFederate(
               object, identityAttributes[PUBLISHER])
               && rti[PUBLISHER].isAttributeOwnedByFederate(
                  object, derivedAttributes[PUBLISHER]),
            "registration did not make the publisher owner of every published class attribute");
         String objectName = rti[PUBLISHER].getObjectInstanceName(object);
         JavaTckSupport.check(object != null && objectName != null && !objectName.isEmpty()
               && object.equals(rti[PUBLISHER].getObjectInstanceHandle(objectName))
               && derivedClasses[PUBLISHER].equals(
                  rti[PUBLISHER].getKnownObjectClassHandle(object)),
            "publisher object identity or actual-class lookup did not round-trip");
         JavaTckSupport.expectFailure(
            () -> rti[LATE].getObjectInstanceHandle(objectName),
            "ObjectInstanceNotKnown",
            "looking up an undiscovered object by name");

         if (callbackModel == CallbackModel.HLA_EVOKED) {
            for (int index : new int[] {EXACT, BASE, CANCELLED, UNRELATED}) {
               JavaTckSupport.check(recorders[index].discoveries.isEmpty(),
                  "evoked object discovery arrived before callback servicing");
            }
            JavaTckSupport.expectFailure(
               () -> rti[EXACT].getObjectInstanceHandle(objectName),
               "ObjectInstanceNotKnown",
               "looking up an evoked object before discovery");

            rti[CANCELLED].unsubscribeObjectClassAttributes(
               derivedClasses[CANCELLED], cancelledAttributes);
            subscribed[CANCELLED] = false;
            JavaTckSupport.drain(rti[CANCELLED]);
            JavaTckSupport.check(recorders[CANCELLED].discoveries.isEmpty(),
               "cancelled evoked discovery remained queued after unsubscribe");
            JavaTckSupport.drain(rti[EXACT]);
            JavaTckSupport.drain(rti[BASE]);
         } else {
            JavaTckSupport.check(recorders[EXACT].discoveries.size() == 1
                  && recorders[BASE].discoveries.size() == 1
                  && recorders[CANCELLED].discoveries.size() == 1
                  && recorders[UNRELATED].discoveries.isEmpty(),
               "immediate registration did not discover the object once per subscriber");
            rti[CANCELLED].unsubscribeObjectClassAttributes(
               derivedClasses[CANCELLED], cancelledAttributes);
            subscribed[CANCELLED] = false;
         }

         assertDiscovery(rti[EXACT], recorders[EXACT], objectName,
            derivedClasses[EXACT], publisherName, "exact-class discovery");
         assertDiscovery(rti[BASE], recorders[BASE], objectName,
            baseClasses[BASE], publisherName, "base-class promoted discovery");

         rti[LATE].subscribeObjectClassAttributes(derivedClasses[LATE], lateAttributes);
         subscribed[LATE] = true;
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(recorders[LATE].discoveries.isEmpty(),
               "evoked late discovery arrived before callback servicing");
            JavaTckSupport.drain(rti[LATE]);
         }
         assertDiscovery(rti[LATE], recorders[LATE], objectName,
            derivedClasses[LATE], publisherName, "late exact-class discovery");
         ObjectInstanceHandle baseObject = rti[BASE].getObjectInstanceHandle(objectName);
         JavaTckSupport.expectFailure(
            () -> rti[BASE].requestAttributeValueUpdate(
               baseObject,
               JavaTckSupport.attributeSet(rti[BASE], derivedAttributes[BASE]),
               new byte[0]),
            "AttributeNotDefined",
            "requesting a derived-only attribute for an object known through its base class");

         ObjectInstanceHandle exactObject = rti[EXACT].getObjectInstanceHandle(objectName);
         AttributeHandleValueMap unownedValues =
            rti[EXACT].getAttributeHandleValueMapFactory().create(1);
         unownedValues.put(derivedAttributes[EXACT], new byte[] {0x61});
         JavaTckSupport.expectFailure(
            () -> rti[EXACT].updateAttributeValues(exactObject, unownedValues, new byte[0]),
            "AttributeNotOwned",
            "updating an instance attribute from a federate that does not own it");
         JavaTckSupport.check(recorders[EXACT].reflections.isEmpty()
               && recorders[BASE].reflections.isEmpty()
               && recorders[LATE].reflections.isEmpty(),
            "a rejected non-owner update produced an attribute reflection");

         if (callbackModel == CallbackModel.HLA_EVOKED) {
            for (int pass = 0; pass < 8; ++pass) JavaTckSupport.drain(rti[UNRELATED]);
            JavaTckSupport.check(recorders[CANCELLED].discoveries.isEmpty(),
               "unsubscribed evoked subscriber received object discovery");
            JavaTckSupport.expectFailure(
               () -> rti[CANCELLED].getObjectInstanceHandle(objectName),
               "ObjectInstanceNotKnown", "looking up an unsubscribed object by name");
         } else {
            rti[CANCELLED].unsubscribeObjectClassAttributes(
               derivedClasses[CANCELLED], cancelledAttributes);
            subscribed[CANCELLED] = false;
            JavaTckSupport.check(recorders[CANCELLED].discoveries.size() == 1,
               "immediate subscriber changed a delivered discovery count after unsubscribe");
         }
         JavaTckSupport.check(recorders[UNRELATED].discoveries.isEmpty(),
            "an unrelated root-class subscription discovered a derived object");

         byte[] identityValue = new byte[] {0x31, 0x32};
         byte[] derivedValue = new byte[] {0x41, 0x42, 0x43};
         byte[] updateTag = new byte[] {0x51, 0x52};
         AttributeHandleValueMap update =
            rti[PUBLISHER].getAttributeHandleValueMapFactory().create(2);
         update.put(identityAttributes[PUBLISHER], identityValue.clone());
         update.put(derivedAttributes[PUBLISHER], derivedValue.clone());
         rti[PUBLISHER].updateAttributeValues(object, update, updateTag.clone());

         assertReflectedAttribute(rti[EXACT], recorders[EXACT], callbackModel,
            derivedAttributes[EXACT], derivedValue, updateTag, publisherName,
            "exact-class post-discovery update");
         assertReflectedAttribute(rti[BASE], recorders[BASE], callbackModel,
            identityAttributes[BASE], identityValue, updateTag, publisherName,
            "base-class post-discovery update");
         assertReflectedAttribute(rti[LATE], recorders[LATE], callbackModel,
            derivedAttributes[LATE], derivedValue, updateTag, publisherName,
            "late-subscription post-discovery update");
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            for (int pass = 0; pass < 8; ++pass) {
               JavaTckSupport.drain(rti[CANCELLED]);
               JavaTckSupport.drain(rti[UNRELATED]);
            }
         }
         JavaTckSupport.check(recorders[CANCELLED].reflections.isEmpty()
               && recorders[UNRELATED].reflections.isEmpty(),
            "cancelled or unrelated subscriber received the attribute update");

         rti[EXACT].subscribeObjectClassAttributes(derivedClasses[EXACT], exactAttributes);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(rti[EXACT]);
         }
         JavaTckSupport.check(recorders[EXACT].discoveries.size() == 1,
            "repeating an active object subscription generated duplicate discovery");
      } finally {
         if (rti[PUBLISHER] != null && joined[PUBLISHER] && published) {
            try {
               rti[PUBLISHER].unpublishObjectClassAttributes(
                  rti[PUBLISHER].getObjectClassHandle(derivedClassName),
                  JavaTckSupport.attributeSet(rti[PUBLISHER],
                     rti[PUBLISHER].getAttributeHandle(
                        rti[PUBLISHER].getObjectClassHandle(derivedClassName),
                        identityAttributeName),
                     rti[PUBLISHER].getAttributeHandle(
                        rti[PUBLISHER].getObjectClassHandle(derivedClassName),
                        derivedAttributeName)));
            } catch (Exception ignored) { }
         }
         for (int i = 0; i < rti.length; ++i) {
            if (rti[i] != null && joined[i] && subscribed[i]) {
               try {
                  ObjectClassHandle subscribedClass = rti[i].getObjectClassHandle(
                     i == BASE ? baseClassName
                        : i == UNRELATED ? unrelatedClassName : derivedClassName);
                  AttributeHandle subscribedAttribute = rti[i].getAttributeHandle(
                     subscribedClass, i == BASE ? identityAttributeName
                        : i == UNRELATED ? unrelatedAttributeName : derivedAttributeName);
                  rti[i].unsubscribeObjectClassAttributes(subscribedClass,
                     JavaTckSupport.attributeSet(rti[i], subscribedAttribute));
               } catch (Exception ignored) { }
            }
         }
         for (int i = rti.length - 1; i >= 0; --i) {
            if (rti[i] != null && joined[i]) {
               try {
                  rti[i].resignFederationExecution(
                     i == PUBLISHER ? ResignAction.DELETE_OBJECTS : ResignAction.NO_ACTION);
               } catch (Exception ignored) { }
            }
         }
         if (federationCreated && rti[PUBLISHER] != null) {
            try { rti[PUBLISHER].destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         for (int i = rti.length - 1; i >= 0; --i) {
            if (rti[i] != null && connected[i]) {
               try { rti[i].disconnect(); } catch (Exception ignored) { }
            }
         }
      }
   }

   static void runOverlappingSubscriptionsSingleDiscovery(RtiFactory factory, String fom,
         String timeImplementation, String baseClassName, String derivedClassName,
         String identityAttributeName, String derivedAttributeName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder subscriberCallbacks = new Recorder();
      String federation = "java-tck-overlapping-subscriptions-" + UUID.randomUUID();
      boolean publisherConnected = false, subscriberConnected = false, federationCreated = false;
      boolean publisherJoined = false, subscriberJoined = false;
      boolean published = false, baseSubscribed = false, derivedSubscribed = false;
      ObjectClassHandle publisherClass = null, subscriberBaseClass = null;
      ObjectClassHandle subscriberDerivedClass = null;
      AttributeHandle publisherIdentity = null, publisherDerived = null;
      AttributeHandle subscriberBaseIdentity = null, subscriberDerivedIdentity = null;
      AttributeHandle subscriberDerivedAttribute = null;
      AttributeHandleSet publisherAttributes = null;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         subscriber.connect(subscriberCallbacks.proxy(), callbackModel);
         subscriberConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         federationCreated = true;
         String publisherName = "java-tck-overlap-publisher";
         publisher.joinFederationExecution(publisherName, "overlap-publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution(
            "java-tck-overlap-subscriber", "overlap-subscriber", federation);
         subscriberJoined = true;

         publisherClass = publisher.getObjectClassHandle(derivedClassName);
         subscriberBaseClass = subscriber.getObjectClassHandle(baseClassName);
         subscriberDerivedClass = subscriber.getObjectClassHandle(derivedClassName);
         publisherIdentity = publisher.getAttributeHandle(publisherClass, identityAttributeName);
         publisherDerived = publisher.getAttributeHandle(publisherClass, derivedAttributeName);
         subscriberBaseIdentity = subscriber.getAttributeHandle(
            subscriberBaseClass, identityAttributeName);
         subscriberDerivedIdentity = subscriber.getAttributeHandle(
            subscriberDerivedClass, identityAttributeName);
         subscriberDerivedAttribute = subscriber.getAttributeHandle(
            subscriberDerivedClass, derivedAttributeName);
         JavaTckSupport.check(publisherClass != null && subscriberBaseClass != null
               && subscriberDerivedClass != null && !subscriberBaseClass.equals(subscriberDerivedClass)
               && publisherIdentity != null && publisherDerived != null
               && subscriberBaseIdentity != null && subscriberDerivedIdentity != null
               && subscriberDerivedAttribute != null
               && subscriberBaseIdentity.equals(subscriberDerivedIdentity)
               && derivedClassName.equals(publisher.getObjectClassName(publisherClass))
               && baseClassName.equals(subscriber.getObjectClassName(subscriberBaseClass))
               && derivedClassName.equals(
                  subscriber.getObjectClassName(subscriberDerivedClass)),
            "adapter hierarchy FOM did not resolve the overlapping subscription handles");

         publisherAttributes = JavaTckSupport.attributeSet(
            publisher, publisherIdentity, publisherDerived);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         subscriber.subscribeObjectClassAttributes(subscriberBaseClass,
            JavaTckSupport.attributeSet(subscriber, subscriberBaseIdentity));
         baseSubscribed = true;
         subscriber.subscribeObjectClassAttributes(subscriberDerivedClass,
            JavaTckSupport.attributeSet(subscriber, subscriberDerivedIdentity,
               subscriberDerivedAttribute));
         derivedSubscribed = true;

         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "overlapping-subscription registration returned an invalid object handle");
         long callbackDeadline = System.nanoTime() + 5_000_000_000L;
         while (subscriberCallbacks.discoveries.isEmpty()
               && System.nanoTime() < callbackDeadline) {
            if (callbackModel == CallbackModel.HLA_EVOKED) {
               JavaTckSupport.drain(subscriber);
            } else {
               Thread.yield();
            }
         }
         JavaTckSupport.check(!subscriberCallbacks.discoveries.isEmpty(),
            "overlapping subscriptions did not discover the registered object");

         long quietDeadline = System.nanoTime() + 150_000_000L;
         while (System.nanoTime() < quietDeadline) {
            if (callbackModel == CallbackModel.HLA_EVOKED) {
               JavaTckSupport.drain(subscriber);
            } else {
               Thread.sleep(2L);
            }
         }
         JavaTckSupport.check(subscriberCallbacks.discoveries.size() == 1,
            "one registration produced duplicate discovery callbacks for overlapping subscriptions");
         Discovery discovery = subscriberCallbacks.discoveries.get(0);
         String objectName = publisher.getObjectInstanceName(object);
         ObjectInstanceHandle subscriberObject = subscriber.getObjectInstanceHandle(objectName);
         JavaTckSupport.check(subscriberObject.equals(discovery.object)
               && subscriberDerivedClass.equals(discovery.objectClass)
               && objectName.equals(discovery.name)
               && subscriberDerivedClass.equals(
                  subscriber.getKnownObjectClassHandle(subscriberObject))
               && objectName.equals(subscriber.getObjectInstanceName(subscriberObject)),
            "overlapping subscriptions did not report the registered class and object identity");

         subscriberCallbacks.reflections.clear();
         byte[] identityValue = new byte[] {0x41, 0x42};
         byte[] identityTag = new byte[] {0x51, 0x52};
         AttributeHandleValueMap identityValues =
            publisher.getAttributeHandleValueMapFactory().create(1);
         identityValues.put(publisherIdentity, identityValue.clone());
         publisher.updateAttributeValues(object, identityValues, identityTag.clone());
         await(subscriber, callbackModel, () -> !subscriberCallbacks.reflections.isEmpty(),
            "one reflection for the overlapping inherited attribute");
         long reflectionQuietDeadline = System.nanoTime() + 150_000_000L;
         while (System.nanoTime() < reflectionQuietDeadline) {
            if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(subscriber);
            else Thread.sleep(2L);
         }
         assertReflectedAttribute(subscriber, subscriberCallbacks, callbackModel,
            subscriberBaseIdentity, identityValue, identityTag, publisherName,
            "overlapping base/derived inherited-attribute update");

         subscriber.unsubscribeObjectClassAttributes(subscriberDerivedClass,
            JavaTckSupport.attributeSet(subscriber, subscriberDerivedIdentity,
               subscriberDerivedAttribute));
         derivedSubscribed = false;
         subscriber.unsubscribeObjectClassAttributes(subscriberBaseClass,
            JavaTckSupport.attributeSet(subscriber, subscriberBaseIdentity));
         baseSubscribed = false;
         publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         published = false;
         subscriber.resignFederationExecution(ResignAction.NO_ACTION);
         subscriberJoined = false;
         publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         federationCreated = false;
      } finally {
         if (subscriberJoined && derivedSubscribed && subscriberDerivedClass != null
               && subscriberDerivedIdentity != null && subscriberDerivedAttribute != null) {
            try { subscriber.unsubscribeObjectClassAttributes(subscriberDerivedClass,
               JavaTckSupport.attributeSet(subscriber,
                  subscriberDerivedIdentity, subscriberDerivedAttribute)); }
            catch (Exception ignored) { }
         }
         if (subscriberJoined && baseSubscribed && subscriberBaseClass != null
               && subscriberBaseIdentity != null) {
            try { subscriber.unsubscribeObjectClassAttributes(subscriberBaseClass,
               JavaTckSupport.attributeSet(subscriber, subscriberBaseIdentity)); }
            catch (Exception ignored) { }
         }
         if (publisherJoined && published && publisherClass != null && publisherAttributes != null) {
            try { publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes); }
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
         if (federationCreated) {
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

   private static void assertDiscovery(RTIambassador rti, Recorder recorder,
         String objectName, ObjectClassHandle expectedClass,
         String publisherName, String description) throws Exception {
      JavaTckSupport.check(recorder.discoveries.size() == 1,
         description + " did not deliver exactly one discovery callback");
      Discovery actual = recorder.discoveries.get(0);
      ObjectInstanceHandle localObject = rti.getObjectInstanceHandle(objectName);
      JavaTckSupport.check(localObject.equals(actual.object)
            && expectedClass.equals(actual.objectClass)
            && objectName.equals(actual.name)
            && publisherName.equals(rti.getFederateName(actual.producer)),
         description + " returned incorrect object, class, name, or producer metadata");
      JavaTckSupport.check(expectedClass.equals(rti.getKnownObjectClassHandle(localObject))
            && objectName.equals(rti.getObjectInstanceName(localObject)),
         description + " did not establish stable known-object identity lookups");
   }

   private static void assertReflectedAttribute(RTIambassador rti, Recorder recorder,
         CallbackModel callbackModel, AttributeHandle expectedAttribute,
         byte[] expectedValue, byte[] expectedTag, String publisherName,
         String description) throws Exception {
      await(rti, callbackModel, () -> !recorder.reflections.isEmpty(), description);
      JavaTckSupport.check(recorder.reflections.size() == 1
            && recorder.discoveries.size() == 1,
         description + " did not produce exactly one reflection for its discovered object");
      Reflection actual = recorder.reflections.get(0);
      byte[] actualValue = actual.values.get(expectedAttribute);
      String actualProducer = rti.getFederateName(actual.producer);
      boolean correctObject = actual.object.equals(recorder.discoveries.get(0).object);
      boolean correctValue = actual.values.size() == 1
            && actual.values.containsKey(expectedAttribute)
            && Arrays.equals(expectedValue, actualValue);
      boolean correctTag = Arrays.equals(expectedTag, actual.tag);
      boolean correctProducer = publisherName.equals(actualProducer);
      JavaTckSupport.check(correctObject && correctValue && correctTag && correctProducer,
         description + " reflected object=" + actual.object + " (expected discovered object="
            + recorder.discoveries.get(0).object + "), value count=" + actual.values.size()
            + ", expected attribute present=" + actual.values.containsKey(expectedAttribute)
            + ", value=" + Arrays.toString(actualValue) + ", tag=" + Arrays.toString(actual.tag)
            + ", producer=" + actualProducer + " (expected " + publisherName + ")");
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.sleep(2L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static final class Discovery {
      private ObjectInstanceHandle object;
      private ObjectClassHandle objectClass;
      private String name;
      private FederateHandle producer;
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final AttributeHandleValueMap values;
      private final byte[] tag;
      private final FederateHandle producer;

      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap values,
            byte[] tag, FederateHandle producer) {
         this.object = object;
         this.values = values.clone();
         this.tag = tag.clone();
         this.producer = producer;
      }
   }

   private static final class Recorder {
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)
                     && arguments != null && arguments.length == 4) {
                  Discovery event = new Discovery();
                  event.object = (ObjectInstanceHandle) arguments[0];
                  event.objectClass = (ObjectClassHandle) arguments[1];
                  event.name = (String) arguments[2];
                  event.producer = (FederateHandle) arguments[3];
                  discoveries.add(event);
               }
               if ("reflectAttributeValues".equals(name)
                     && arguments != null && arguments.length == 6) {
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (FederateHandle) arguments[4]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK object discovery recorder";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }
   }
}
