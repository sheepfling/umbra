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
import java.util.HashSet;
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

   static void run(RtiFactory factory, String fom, String baseClassName,
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
         publisher.createFederationExecution(federation, fom);
         created = true;
         FederateHandle publisherFederate =
            publisher.joinFederationExecution(publisherName, federation);
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
         JavaTckSupport.check(publisherDerivedClass.equals(subscriberDerivedClass)
               && baseClass.equals(subscriberBaseClass),
            "adapter FOM class lookups returned inconsistent inheritance handles");

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
               && publisherIdentity.equals(baseIdentity)
               && publisherIdentity.equals(derivedIdentity)
               && publisherDerived.equals(derivedAttribute)
               && !publisherIdentity.equals(publisherDerived),
            "inherited and derived attributes did not retain stable distinct handles");

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
               && object.equals(publisher.getObjectInstanceHandle(objectName)),
            "publisher object identity lookup did not round-trip");

         await(baseSubscriber, model, baseClassName,
            () -> baseEvents.discoveries().size() == 1,
            "base-class subscriber discovery");
         await(derivedSubscriber, model, derivedClassName,
            () -> derivedEvents.discoveries().size() == 1,
            "derived-class subscriber discovery");
         checkDiscovery(baseEvents.discoveries(), object, publisherFederate,
            baseSubscriber, objectName, "base-class subscriber");
         checkDiscovery(derivedEvents.discoveries(), object, publisherFederate,
            derivedSubscriber, objectName, "derived-class subscriber");

         AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(2);
         values.put(publisherIdentity, IDENTITY.clone());
         values.put(publisherDerived, DERIVED.clone());
         publisher.updateAttributeValues(object, values, TAG.clone());

         await(baseSubscriber, model, baseClassName,
            () -> !baseEvents.reflections().isEmpty(), "base-class attribute reflection");
         await(derivedSubscriber, model, derivedClassName,
            () -> !derivedEvents.reflections().isEmpty(), "derived-class attribute reflection");
         checkReflection(last(baseEvents.reflections()), object, baseIdentity, null,
            IDENTITY, null, TAG, publisherFederate, "base-class subscriber");
         checkReflection(last(derivedEvents.reflections()), object, derivedIdentity,
            derivedAttribute, IDENTITY, DERIVED, TAG, publisherFederate,
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

   private static void checkDiscovery(List<Discovery> discoveries, ObjectInstanceHandle object,
         FederateHandle producer, RTIambassador subscriber, String name, String description)
         throws Exception {
      JavaTckSupport.check(discoveries.size() == 1
            && object.equals(discoveries.get(0).object)
            && name.equals(discoveries.get(0).name)
            && producer.equals(discoveries.get(0).producer),
         description + " received duplicate or incorrect discovery metadata");
      JavaTckSupport.check(object.equals(subscriber.getObjectInstanceHandle(name))
            && name.equals(subscriber.getObjectInstanceName(object)),
         description + " did not install object identity lookups");
   }

   private static void checkReflection(Reflection event, ObjectInstanceHandle object,
         AttributeHandle inherited, AttributeHandle derived, byte[] inheritedValue,
         byte[] derivedValue, byte[] tag, FederateHandle producer, String description) {
      JavaTckSupport.check(object.equals(event.object)
            && Arrays.equals(tag, event.tag)
            && producer.equals(event.producer)
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
      final String name;
      final FederateHandle producer;
      Discovery(Object[] args) {
         object = (ObjectInstanceHandle) args[0];
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
