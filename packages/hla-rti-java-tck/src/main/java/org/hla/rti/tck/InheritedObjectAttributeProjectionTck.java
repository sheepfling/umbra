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
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verify inherited and derived attribute projection through ordinary subscriptions. */
final class InheritedObjectAttributeProjectionTck {
   private static final byte[] IDENTITY = new byte[] {0x49, 0x44};
   private static final byte[] DERIVED = new byte[] {0x44, 0x56};
   private static final byte[] TAG = new byte[] {0x50, 0x52, 0x4a};

   private InheritedObjectAttributeProjectionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String baseClassName,
         String derivedClassName, String identityAttributeName,
         String derivedAttributeName, CallbackModel model) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador baseSubscriber = factory.getRtiAmbassador();
      RTIambassador derivedSubscriber = factory.getRtiAmbassador();
      Recorder publisherEvents = new Recorder();
      Recorder baseEvents = new Recorder();
      Recorder derivedEvents = new Recorder();
      String federation = "java-tck-inherited-attributes-" + UUID.randomUUID();
      String publisherName = "java-tck-inheritance-publisher";
      boolean publisherConnected = false, baseConnected = false, derivedConnected = false;
      boolean created = false, publisherJoined = false, baseJoined = false, derivedJoined = false;
      try {
         publisher.connect(publisherEvents.proxy(), model);
         publisherConnected = true;
         baseSubscriber.connect(baseEvents.proxy(), model);
         baseConnected = true;
         derivedSubscriber.connect(derivedEvents.proxy(), model);
         derivedConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         publisher.joinFederationExecution(
            publisherName, "inheritance-publisher", federation);
         publisherJoined = true;
         baseSubscriber.joinFederationExecution("java-tck-inheritance-base", federation);
         baseJoined = true;
         derivedSubscriber.joinFederationExecution("java-tck-inheritance-derived", federation);
         derivedJoined = true;

         ObjectClassHandle publisherDerivedClass =
            publisher.getObjectClassHandle(derivedClassName);
         ObjectClassHandle baseClass = publisher.getObjectClassHandle(baseClassName);
         ObjectClassHandle subscriberBaseClass =
            baseSubscriber.getObjectClassHandle(baseClassName);
         ObjectClassHandle subscriberDerivedClass =
            derivedSubscriber.getObjectClassHandle(derivedClassName);
         JavaTckSupport.check(derivedClassName.equals(
                  publisher.getObjectClassName(publisherDerivedClass))
               && baseClassName.equals(publisher.getObjectClassName(baseClass))
               && baseClassName.equals(baseSubscriber.getObjectClassName(subscriberBaseClass))
               && derivedClassName.equals(
                  derivedSubscriber.getObjectClassName(subscriberDerivedClass)),
            "adapter FOM class lookups did not round-trip within each federate");

         AttributeHandle publisherIdentity = publisher.getAttributeHandle(
            publisherDerivedClass, identityAttributeName);
         AttributeHandle publisherDerived = publisher.getAttributeHandle(
            publisherDerivedClass, derivedAttributeName);
         AttributeHandle publisherBaseIdentity = publisher.getAttributeHandle(
            baseClass, identityAttributeName);
         AttributeHandle baseIdentity = baseSubscriber.getAttributeHandle(
            subscriberBaseClass, identityAttributeName);
         AttributeHandle derivedIdentity = derivedSubscriber.getAttributeHandle(
            subscriberDerivedClass, identityAttributeName);
         AttributeHandle derivedAttribute = derivedSubscriber.getAttributeHandle(
            subscriberDerivedClass, derivedAttributeName);
         JavaTckSupport.check(publisherIdentity.equals(publisherBaseIdentity)
               && !publisherIdentity.equals(publisherDerived)
               && identityAttributeName.equals(baseSubscriber.getAttributeName(
                  subscriberBaseClass, baseIdentity))
               && identityAttributeName.equals(derivedSubscriber.getAttributeName(
                  subscriberDerivedClass, derivedIdentity))
               && derivedAttributeName.equals(derivedSubscriber.getAttributeName(
                  subscriberDerivedClass, derivedAttribute)),
            "inherited and derived attributes did not round-trip within each federate");

         AttributeHandleSet published =
            JavaTckSupport.attributeSet(publisher, publisherIdentity, publisherDerived);
         AttributeHandleSet baseAttributes = JavaTckSupport.attributeSet(
            baseSubscriber, baseIdentity);
         AttributeHandleSet derivedAttributes = JavaTckSupport.attributeSet(
            derivedSubscriber, derivedIdentity, derivedAttribute);
         publisher.publishObjectClassAttributes(publisherDerivedClass, published);
         baseSubscriber.subscribeObjectClassAttributes(subscriberBaseClass, baseAttributes);
         derivedSubscriber.subscribeObjectClassAttributes(
            subscriberDerivedClass, derivedAttributes);

         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherDerivedClass);
         JavaTckSupport.check(object != null,
            "derived-class registration returned a null object handle");
         String objectName = publisher.getObjectInstanceName(object);
         JavaTckSupport.check(objectName != null && !objectName.isEmpty()
               && object.equals(publisher.getObjectInstanceHandle(objectName))
               && publisherDerivedClass.equals(publisher.getKnownObjectClassHandle(object)),
            "publisher object identity or actual-class lookup did not round-trip");

         await(baseSubscriber, model, baseClassName,
            () -> baseEvents.discoveries().size() == 1,
            "base-class subscriber discovery");
         await(derivedSubscriber, model, derivedClassName,
            () -> derivedEvents.discoveries().size() == 1,
            "derived-class subscriber discovery");
         ObjectInstanceHandle baseObject = checkDiscovery(baseEvents.discoveries(),
            subscriberBaseClass, publisherName, baseSubscriber, objectName,
            "base-class subscriber");
         ObjectInstanceHandle derivedObject = checkDiscovery(derivedEvents.discoveries(),
            subscriberDerivedClass, publisherName, derivedSubscriber, objectName,
            "derived-class subscriber");

         AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(2);
         values.put(publisherIdentity, IDENTITY.clone());
         values.put(publisherDerived, DERIVED.clone());
         publisher.updateAttributeValues(object, values, TAG.clone());

         await(baseSubscriber, model, baseClassName,
            () -> !baseEvents.reflections().isEmpty(), "base-class attribute reflection");
         await(derivedSubscriber, model, derivedClassName,
            () -> !derivedEvents.reflections().isEmpty(), "derived-class attribute reflection");
         checkReflection(last(baseEvents.reflections()), baseObject, baseIdentity, null,
            IDENTITY, null, TAG, publisherName, baseSubscriber, "base-class subscriber");
         checkReflection(last(derivedEvents.reflections()), derivedObject, derivedIdentity,
            derivedAttribute, IDENTITY, DERIVED, TAG, publisherName, derivedSubscriber,
            "derived-class subscriber");
         service(publisher, model, derivedClassName);
         JavaTckSupport.check(publisherEvents.reflections().isEmpty(),
            "attribute update looped back to the publisher");

         baseSubscriber.unsubscribeObjectClassAttributes(subscriberBaseClass, baseAttributes);
         derivedSubscriber.unsubscribeObjectClassAttributes(
            subscriberDerivedClass, derivedAttributes);
         publisher.unpublishObjectClassAttributes(publisherDerivedClass, published);
         baseSubscriber.resignFederationExecution(ResignAction.NO_ACTION);
         baseJoined = false;
         derivedSubscriber.resignFederationExecution(ResignAction.NO_ACTION);
         derivedJoined = false;
         publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (derivedJoined) resign(derivedSubscriber, ResignAction.NO_ACTION);
         if (baseJoined) resign(baseSubscriber, ResignAction.NO_ACTION);
         if (publisherJoined) resign(publisher, ResignAction.DELETE_OBJECTS);
         if (created) {
            try { publisher.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (derivedConnected) disconnect(derivedSubscriber);
         if (baseConnected) disconnect(baseSubscriber);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static ObjectInstanceHandle checkDiscovery(List<Discovery> discoveries,
         ObjectClassHandle expectedClass, String producerName, RTIambassador subscriber,
         String name, String description) throws Exception {
      JavaTckSupport.check(discoveries.size() == 1,
         description + " received " + discoveries.size() + " discoveries instead of one");
      ObjectInstanceHandle object = subscriber.getObjectInstanceHandle(name);
      Discovery discovery = discoveries.get(0);
      JavaTckSupport.check(object.equals(discovery.object),
         description + " discovered object differs from its local name lookup");
      JavaTckSupport.check(expectedClass.equals(discovery.objectClass),
         description + " reported a different discovery class");
      JavaTckSupport.check(name.equals(discovery.name),
         description + " reported a different object name");
      JavaTckSupport.check(producerName.equals(subscriber.getFederateName(discovery.producer)),
         description + " reported a different producing federate");
      JavaTckSupport.check(name.equals(subscriber.getObjectInstanceName(object))
            && expectedClass.equals(subscriber.getKnownObjectClassHandle(object)),
         description + " did not install object identity lookups");
      return object;
   }

   private static void checkReflection(Reflection event, ObjectInstanceHandle object,
         AttributeHandle inherited, AttributeHandle derived, byte[] inheritedValue,
         byte[] derivedValue, byte[] tag, String producerName,
         RTIambassador subscriber, String description) throws Exception {
      JavaTckSupport.check(object.equals(event.object)
            && Arrays.equals(tag, event.tag)
            && producerName.equals(subscriber.getFederateName(event.producer))
            && event.transportation != null
            && event.values.size() == (derived == null ? 1 : 2)
            && Arrays.equals(inheritedValue, event.values.get(inherited))
            && (derived == null || Arrays.equals(derivedValue, event.values.get(derived))),
         description + " received an incorrect inherited-attribute projection");
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         String serviceClassName, Condition condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (!condition.test() && System.nanoTime() < deadline) {
         service(ambassador, model, serviceClassName);
         Thread.yield();
      }
      JavaTckSupport.check(condition.test(), "timed out waiting for " + description);
   }

   private static void service(RTIambassador ambassador, CallbackModel model,
         String serviceClassName) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) {
         ambassador.evokeMultipleCallbacks(0.0, 1.0);
      } else {
         ambassador.getObjectClassHandle(serviceClassName);
      }
   }

   private static void resign(RTIambassador ambassador, ResignAction action) {
      try { ambassador.resignFederationExecution(action); } catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static Reflection last(List<Reflection> reflections) {
      return reflections.get(reflections.size() - 1);
   }

   private interface Condition { boolean test(); }

   private static final class Discovery {
      final ObjectInstanceHandle object;
      final ObjectClassHandle objectClass;
      final String name;
      final FederateHandle producer;
      Discovery(Object[] args) {
         object = (ObjectInstanceHandle) args[0];
         objectClass = (ObjectClassHandle) args[1];
         name = (String) args[2];
         producer = (FederateHandle) args[3];
      }
   }

   private static final class Reflection {
      final ObjectInstanceHandle object;
      final Map<AttributeHandle, byte[]> values;
      final byte[] tag;
      final TransportationTypeHandle transportation;
      final FederateHandle producer;
      Reflection(Object[] args) {
         object = (ObjectInstanceHandle) args[0];
         AttributeHandleValueMap source = (AttributeHandleValueMap) args[1];
         java.util.HashMap<AttributeHandle, byte[]> copy = new java.util.HashMap<>();
         source.forEach((key, value) -> copy.put(key, value.clone()));
         values = Map.copyOf(copy);
         tag = ((byte[]) args[2]).clone();
         transportation = (TransportationTypeHandle) args[3];
         producer = (FederateHandle) args[4];
      }
   }

   private static final class Recorder {
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, args) -> {
               Object[] a = args == null ? new Object[0] : args;
               if ("discoverObjectInstance".equals(method.getName()) && a.length == 4) {
                  discoveries.add(new Discovery(a));
               } else if ("reflectAttributeValues".equals(method.getName())) {
                  reflections.add(new Reflection(a));
               } else if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(method.getName())) return "Java TCK inheritance callbacks";
                  if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
                  if ("equals".equals(method.getName())) return proxy == a[0];
               }
               return null;
            });
      }
      List<Discovery> discoveries() { return List.copyOf(discoveries); }
      List<Reflection> reflections() { return List.copyOf(reflections); }
   }
}
