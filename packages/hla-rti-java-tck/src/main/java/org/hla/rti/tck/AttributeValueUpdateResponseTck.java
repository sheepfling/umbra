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
import java.util.HashSet;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks an ordinary provider response to an instance-scoped value request. */
final class AttributeValueUpdateResponseTck {
   private AttributeValueUpdateResponseTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder memberRecorder = new Recorder();
      String federation = "java-tck-attribute-value-response-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean memberConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean memberJoined = false;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         member.connect(memberRecorder.proxy(), callbackModel);
         memberConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle ownerFederate = owner.joinFederationExecution(
            "java-tck-response-owner", federation);
         ownerJoined = true;
         member.joinFederationExecution("java-tck-response-member", federation);
         memberJoined = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle memberClass = member.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle memberAttribute = member.getAttributeHandle(memberClass, attributeName);
         JavaTckSupport.check(objectClassName.equals(owner.getObjectClassName(ownerClass))
               && objectClassName.equals(member.getObjectClassName(memberClass))
               && attributeName.equals(owner.getAttributeName(ownerClass, ownerAttribute))
               && attributeName.equals(member.getAttributeName(memberClass, memberAttribute)),
            "adapter FOM did not resolve the requested response class and attribute");
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet memberAttributes = JavaTckSupport.attributeSet(member, memberAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         member.subscribeObjectClassAttributes(memberClass, memberAttributes);

         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         await(member, () -> memberRecorder.discovered(object),
            "member did not discover the published object before the deadline");
         ObjectInstanceHandle knownObject = memberRecorder.discoveredObject;
         JavaTckSupport.check(object.equals(knownObject),
            "member discovery returned a different object handle");

         ownerRecorder.clearProvided();
         memberRecorder.clearReflections();
         byte[] requestTag = new byte[] {0x52, 0x45, 0x51};
         byte[] responseValue = new byte[] {0x5a, 0x25, 0x07};
         byte[] responseTag = new byte[] {0x52, 0x45, 0x53};
         member.requestAttributeValueUpdate(knownObject, memberAttributes, requestTag);
         await(owner, () -> !ownerRecorder.provided().isEmpty(),
            "provider did not receive Provide Attribute Value Update before the deadline");
         JavaTckSupport.drain(member);
         JavaTckSupport.check(memberRecorder.reflections().isEmpty(),
            "attribute values were reflected before the provider answered");
         List<ProvidedUpdate> requests = ownerRecorder.provided();
         JavaTckSupport.check(requests.size() == 1
               && object.equals(requests.get(0).object)
               && new HashSet<>(ownerAttributes).equals(requests.get(0).attributes)
               && Arrays.equals(requestTag, requests.get(0).tag),
            "provider callback returned the wrong object, attributes, or request tag");

         AttributeHandleValueMap responseValues =
            owner.getAttributeHandleValueMapFactory().create(1);
         responseValues.put(ownerAttribute, responseValue.clone());
         owner.updateAttributeValues(object, responseValues, responseTag);
         await(member, () -> !memberRecorder.reflections().isEmpty(),
            "member did not receive the provider's attribute response before the deadline");
         List<Reflection> reflections = memberRecorder.reflections();
         JavaTckSupport.check(reflections.size() == 1,
            "member received an unexpected number of response reflections");
         Reflection reflection = reflections.get(0);
         TransportationTypeHandle reliable = member.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(!reflection.timestampOrdered
               && knownObject.equals(reflection.object)
               && reflection.values.size() == 1
               && Arrays.equals(responseValue, reflection.values.get(memberAttribute))
               && Arrays.equals(responseTag, reflection.tag)
               && reliable.equals(reflection.transportation)
               && ownerFederate.equals(reflection.producer)
               && (reflection.regions == null || reflection.regions.isEmpty()),
            "response reflection returned the wrong value, tag, producer, order, or transport metadata");

         member.unsubscribeObjectClassAttributes(memberClass, memberAttributes);
         owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes);
         member.resignFederationExecution(ResignAction.NO_ACTION);
         memberJoined = false;
         owner.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         ownerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (memberJoined) {
            try { member.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (memberConnected) {
            try { member.disconnect(); }
            catch (Exception ignored) { }
         }
         if (ownerConnected) {
            try { owner.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static void await(RTIambassador ambassador, BooleanSupplier condition,
         String failure) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         JavaTckSupport.drain(ambassador);
         Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), failure);
   }

   private static final class ProvidedUpdate {
      private final ObjectInstanceHandle object;
      private final HashSet<AttributeHandle> attributes;
      private final byte[] tag;

      private ProvidedUpdate(ObjectInstanceHandle object, AttributeHandleSet attributes,
            byte[] tag) {
         this.object = object;
         this.attributes = new HashSet<>(attributes);
         this.tag = tag.clone();
      }
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final AttributeHandleValueMap values;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final RegionHandleSet regions;
      private final boolean timestampOrdered;

      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap values,
            byte[] tag, TransportationTypeHandle transportation, FederateHandle producer,
            RegionHandleSet regions, boolean timestampOrdered) {
         this.object = object;
         this.values = values.clone();
         this.tag = tag.clone();
         this.transportation = transportation;
         this.producer = producer;
         this.regions = regions;
         this.timestampOrdered = timestampOrdered;
      }
   }

   private static final class Recorder {
      private final List<ProvidedUpdate> provided = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private volatile ObjectInstanceHandle discoveredObject;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)) {
                  discoveredObject = (ObjectInstanceHandle) arguments[0];
               } else if ("provideAttributeValueUpdate".equals(name)) {
                  provided.add(new ProvidedUpdate((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (byte[]) arguments[2]));
               } else if ("reflectAttributeValues".equals(name)) {
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     (RegionHandleSet) arguments[5], arguments.length != 6));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK value-response callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }

      private boolean discovered(ObjectInstanceHandle object) {
         return object.equals(discoveredObject);
      }

      private List<ProvidedUpdate> provided() {
         return List.copyOf(provided);
      }

      private List<Reflection> reflections() {
         return List.copyOf(reflections);
      }

      private void clearProvided() {
         provided.clear();
      }

      private void clearReflections() {
         reflections.clear();
      }
   }
}
