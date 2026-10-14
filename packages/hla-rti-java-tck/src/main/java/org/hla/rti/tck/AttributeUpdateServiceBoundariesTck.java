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
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies ordinary attribute Update/Reflect delivery to active and passive subscribers. */
final class AttributeUpdateServiceBoundariesTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private AttributeUpdateServiceBoundariesTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String attributeName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador active = factory.getRtiAmbassador();
      RTIambassador passive = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder activeCallbacks = new Recorder();
      Recorder passiveCallbacks = new Recorder();
      String federation = "java-tck-attribute-update-boundaries-" + UUID.randomUUID();
      boolean ownerConnected = false, activeConnected = false, passiveConnected = false;
      boolean created = false, ownerJoined = false, activeJoined = false, passiveJoined = false;
      boolean published = false, activeSubscribed = false, passiveSubscribed = false;
      ObjectClassHandle ownerClass = null, activeClass = null, passiveClass = null;
      AttributeHandle ownerAttribute = null, activeAttribute = null, passiveAttribute = null;
      AttributeHandleSet ownerAttributes = null, activeAttributes = null, passiveAttributes = null;
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel);
         ownerConnected = true;
         active.connect(activeCallbacks.proxy(), callbackModel);
         activeConnected = true;
         passive.connect(passiveCallbacks.proxy(), callbackModel);
         passiveConnected = true;
         owner.createFederationExecution(federation, fom);
         created = true;
         FederateHandle producer = owner.joinFederationExecution(
            "java-tck-attribute-update-owner", federation);
         ownerJoined = true;
         active.joinFederationExecution("java-tck-attribute-update-active", federation);
         activeJoined = true;
         passive.joinFederationExecution("java-tck-attribute-update-passive", federation);
         passiveJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         activeClass = active.getObjectClassHandle(objectClassName);
         passiveClass = passive.getObjectClassHandle(objectClassName);
         ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         activeAttribute = active.getAttributeHandle(activeClass, attributeName);
         passiveAttribute = passive.getAttributeHandle(passiveClass, attributeName);
         JavaTckSupport.check(objectClassName.equals(owner.getObjectClassName(ownerClass))
               && objectClassName.equals(active.getObjectClassName(activeClass))
               && objectClassName.equals(passive.getObjectClassName(passiveClass))
               && attributeName.equals(owner.getAttributeName(ownerClass, ownerAttribute))
               && attributeName.equals(active.getAttributeName(activeClass, activeAttribute))
               && attributeName.equals(passive.getAttributeName(passiveClass, passiveAttribute)),
            "adapter FOM object-class or attribute handles did not round-trip");
         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         activeAttributes = JavaTckSupport.attributeSet(active, activeAttribute);
         passiveAttributes = JavaTckSupport.attributeSet(passive, passiveAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         published = true;
         active.subscribeObjectClassAttributes(activeClass, activeAttributes);
         activeSubscribed = true;
         passive.subscribeObjectClassAttributesPassively(passiveClass, passiveAttributes);
         passiveSubscribed = true;

         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "ordinary attribute update registration returned an invalid object handle");
         await(active, callbackModel,
            () -> !activeCallbacks.discoveries.isEmpty(), "active object discovery");
         await(passive, callbackModel,
            () -> !passiveCallbacks.discoveries.isEmpty(),
            "passive discovery after active declaration establishes relevance");
         JavaTckSupport.check(activeCallbacks.discoveries.size() == 1
               && passiveCallbacks.discoveries.size() == 1
               && object.equals(activeCallbacks.discoveries.get(0).object)
               && object.equals(passiveCallbacks.discoveries.get(0).object)
               && activeClass.equals(activeCallbacks.discoveries.get(0).objectClass)
               && passiveClass.equals(passiveCallbacks.discoveries.get(0).objectClass)
               && producer.equals(activeCallbacks.discoveries.get(0).producer)
               && producer.equals(passiveCallbacks.discoveries.get(0).producer),
            "active or passive discovery returned incorrect object metadata");

         byte[] value = new byte[] {0x10, 0x20, 0x30};
         byte[] tag = new byte[] {0x54, 0x43, 0x4b};
         AttributeHandleValueMap values = owner.getAttributeHandleValueMapFactory().create(1);
         values.put(ownerAttribute, value.clone());
         owner.updateAttributeValues(object, values, tag.clone());
         await(active, callbackModel,
            () -> !activeCallbacks.reflections.isEmpty(), "active attribute reflection");
         await(passive, callbackModel,
            () -> !passiveCallbacks.reflections.isEmpty(),
            "passive reflection after active declaration establishes relevance");
         JavaTckSupport.check(activeCallbacks.reflections.size() == 1
               && passiveCallbacks.reflections.size() == 1,
            "ordinary attribute update delivered an unexpected active or passive callback count");
         assertReflection(activeCallbacks.reflections.get(0), object, activeAttribute,
            value, tag, producer, active);
         assertReflection(passiveCallbacks.reflections.get(0), object, passiveAttribute,
            value, tag, producer, passive);
         JavaTckSupport.check(ownerCallbacks.reflections.isEmpty(),
            "ordinary attribute update looped a reflection back to its producer");

         passive.unsubscribeObjectClassAttributes(passiveClass, passiveAttributes);
         passiveSubscribed = false;
         active.unsubscribeObjectClassAttributes(activeClass, activeAttributes);
         activeSubscribed = false;
         owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes);
         published = false;
      } finally {
         if (passiveSubscribed && passiveJoined) {
            try { passive.unsubscribeObjectClassAttributes(passiveClass, passiveAttributes); }
            catch (Exception ignored) { }
         }
         if (activeSubscribed && activeJoined) {
            try { active.unsubscribeObjectClassAttributes(activeClass, activeAttributes); }
            catch (Exception ignored) { }
         }
         if (published && ownerJoined) {
            try { owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes); }
            catch (Exception ignored) { }
         }
         if (passiveJoined) resign(passive);
         if (activeJoined) resign(active);
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (passiveConnected) disconnect(passive);
         if (activeConnected) disconnect(active);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static void assertReflection(Reflection reflection, ObjectInstanceHandle object,
         AttributeHandle expectedAttribute, byte[] value, byte[] tag, FederateHandle producer,
         RTIambassador inspector) throws Exception {
      JavaTckSupport.check(object.equals(reflection.object)
            && reflection.values.size() == 1
            && Arrays.equals(value, reflection.values.get(expectedAttribute))
            && Arrays.equals(tag, reflection.tag) && producer.equals(reflection.producer)
            && reflection.transportation != null
            && !inspector.getTransportationTypeName(reflection.transportation).isEmpty()
            && reflection.regionsEmpty,
         "ordinary reflection changed object, attribute, value, tag, producer, or metadata");
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); }
      catch (Exception ignored) { }
   }

   private static final class Discovery {
      private final ObjectInstanceHandle object;
      private final ObjectClassHandle objectClass;
      private final FederateHandle producer;

      private Discovery(ObjectInstanceHandle object, ObjectClassHandle objectClass,
            FederateHandle producer) {
         this.object = object;
         this.objectClass = objectClass;
         this.producer = producer;
      }
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final AttributeHandleValueMap values;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final boolean regionsEmpty;

      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap values,
            byte[] tag, TransportationTypeHandle transportation, FederateHandle producer,
            boolean regionsEmpty) {
         this.object = object;
         this.values = values.clone();
         this.tag = tag.clone();
         this.transportation = transportation;
         this.producer = producer;
         this.regionsEmpty = regionsEmpty;
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
                  discoveries.add(new Discovery((ObjectInstanceHandle) arguments[0],
                     (ObjectClassHandle) arguments[1], (FederateHandle) arguments[3]));
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 6) {
                  hla.rti1516_2025.RegionHandleSet regions =
                     (hla.rti1516_2025.RegionHandleSet) arguments[5];
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     regions == null || regions.isEmpty()));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK attribute update callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
