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

/** Checks independent ordinary value requests from two active subscribers. */
final class AttributeValueUpdateMultiRequesterTck {
   private AttributeValueUpdateMultiRequesterTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador first = factory.getRtiAmbassador();
      RTIambassador second = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder firstRecorder = new Recorder();
      Recorder secondRecorder = new Recorder();
      String federation = "java-tck-value-update-multi-requester-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean firstConnected = false;
      boolean secondConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean firstJoined = false;
      boolean secondJoined = false;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         first.connect(firstRecorder.proxy(), callbackModel);
         firstConnected = true;
         second.connect(secondRecorder.proxy(), callbackModel);
         secondConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle ownerFederate = owner.joinFederationExecution(
            "java-tck-multi-requester-owner", federation);
         ownerJoined = true;
         first.joinFederationExecution("java-tck-multi-requester-first", federation);
         firstJoined = true;
         second.joinFederationExecution("java-tck-multi-requester-second", federation);
         secondJoined = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle firstClass = first.getObjectClassHandle(objectClassName);
         ObjectClassHandle secondClass = second.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle firstAttribute = first.getAttributeHandle(firstClass, attributeName);
         AttributeHandle secondAttribute = second.getAttributeHandle(secondClass, attributeName);
         JavaTckSupport.check(objectClassName.equals(owner.getObjectClassName(ownerClass))
               && objectClassName.equals(first.getObjectClassName(firstClass))
               && objectClassName.equals(second.getObjectClassName(secondClass))
               && attributeName.equals(owner.getAttributeName(ownerClass, ownerAttribute))
               && attributeName.equals(first.getAttributeName(firstClass, firstAttribute))
               && attributeName.equals(second.getAttributeName(secondClass, secondAttribute)),
            "adapter FOM did not resolve the multi-requester class and attribute");
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet firstAttributes = JavaTckSupport.attributeSet(first, firstAttribute);
         AttributeHandleSet secondAttributes = JavaTckSupport.attributeSet(second, secondAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         first.subscribeObjectClassAttributes(firstClass, firstAttributes);
         second.subscribeObjectClassAttributes(secondClass, secondAttributes);

         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         await(first, () -> firstRecorder.discovered(object),
            "first requester did not discover the published object before the deadline");
         await(second, () -> secondRecorder.discovered(object),
            "second requester did not discover the published object before the deadline");
         String objectName = owner.getObjectInstanceName(object);
         ObjectInstanceHandle firstObject = first.getObjectInstanceHandle(objectName);
         ObjectInstanceHandle secondObject = second.getObjectInstanceHandle(objectName);
         JavaTckSupport.check(!objectName.isEmpty() && object.equals(firstObject)
               && object.equals(secondObject)
               && firstClass.equals(first.getKnownObjectClassHandle(firstObject))
               && secondClass.equals(second.getKnownObjectClassHandle(secondObject)),
            "multi-requester discovery did not establish stable object identity");

         byte[] firstRequestTag = new byte[] {0x51, 0x52, 0x31};
         byte[] secondRequestTag = new byte[] {0x51, 0x52, 0x32};
         byte[] responseValue = new byte[] {0x41, 0x56, 0x52, 0x31};
         byte[] responseTag = new byte[] {0x52, 0x53, 0x50};
         ownerRecorder.clearProvided();
         firstRecorder.clearProvided();
         secondRecorder.clearProvided();
         first.requestAttributeValueUpdate(firstObject, firstAttributes, firstRequestTag);
         second.requestAttributeValueUpdate(secondObject, secondAttributes, secondRequestTag);
         await(owner, () -> ownerRecorder.provided().size() >= 2,
            "provider did not receive both requester callbacks before the deadline");
         JavaTckSupport.drain(first);
         JavaTckSupport.drain(second);
         JavaTckSupport.check(firstRecorder.provided().isEmpty()
               && secondRecorder.provided().isEmpty(),
            "provider callback looped to one of the requesters");
         List<ProvidedUpdate> requests = ownerRecorder.provided();
         boolean sawFirst = false;
         boolean sawSecond = false;
         for (ProvidedUpdate request : requests) {
            JavaTckSupport.check(object.equals(request.object)
                  && new HashSet<>(ownerAttributes).equals(request.attributes),
               "provider callback returned the wrong object or attribute set");
            if (Arrays.equals(firstRequestTag, request.tag)) {
               JavaTckSupport.check(!sawFirst, "first requester callback was duplicated");
               sawFirst = true;
            } else if (Arrays.equals(secondRequestTag, request.tag)) {
               JavaTckSupport.check(!sawSecond, "second requester callback was duplicated");
               sawSecond = true;
            } else {
               throw new AssertionError("provider callback returned an unknown request tag");
            }
         }
         JavaTckSupport.check(requests.size() == 2 && sawFirst && sawSecond,
            "provider did not receive exactly one callback for each requester");

         firstRecorder.clearReflections();
         secondRecorder.clearReflections();
         AttributeHandleValueMap responseValues =
            owner.getAttributeHandleValueMapFactory().create(1);
         responseValues.put(ownerAttribute, responseValue.clone());
         owner.updateAttributeValues(object, responseValues, responseTag);
         await(first, () -> !firstRecorder.reflections().isEmpty(),
            "first requester did not receive the provider update before the deadline");
         await(second, () -> !secondRecorder.reflections().isEmpty(),
            "second requester did not receive the provider update before the deadline");
         TransportationTypeHandle reliable = first.getTransportationTypeHandle("HLAreliable");
         assertResponse(firstRecorder.reflections(), object, firstAttribute, responseValue,
            responseTag, ownerFederate, reliable, "first requester");
         assertResponse(secondRecorder.reflections(), object, secondAttribute, responseValue,
            responseTag, ownerFederate, reliable, "second requester");

         first.unsubscribeObjectClassAttributes(firstClass, firstAttributes);
         second.unsubscribeObjectClassAttributes(secondClass, secondAttributes);
         owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes);
         first.resignFederationExecution(ResignAction.NO_ACTION);
         firstJoined = false;
         second.resignFederationExecution(ResignAction.NO_ACTION);
         secondJoined = false;
         owner.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         ownerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (secondJoined) {
            try { second.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (firstJoined) {
            try { first.resignFederationExecution(ResignAction.NO_ACTION); }
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
         if (secondConnected) {
            try { second.disconnect(); }
            catch (Exception ignored) { }
         }
         if (firstConnected) {
            try { first.disconnect(); }
            catch (Exception ignored) { }
         }
         if (ownerConnected) {
            try { owner.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static void assertResponse(List<Reflection> reflections,
         ObjectInstanceHandle object, AttributeHandle attribute, byte[] value, byte[] tag,
         FederateHandle producer, TransportationTypeHandle reliable, String requester) {
      JavaTckSupport.check(reflections.size() == 1,
         requester + " received an unexpected number of response reflections");
      Reflection reflection = reflections.get(0);
      JavaTckSupport.check(!reflection.timestampOrdered && object.equals(reflection.object)
            && reflection.values.size() == 1
            && Arrays.equals(value, reflection.values.get(attribute))
            && Arrays.equals(tag, reflection.tag)
            && reliable.equals(reflection.transportation)
            && producer.equals(reflection.producer)
            && (reflection.regions == null || reflection.regions.isEmpty()),
         requester + " received incorrect response value or standard reflection metadata");
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
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
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
                  if ("toString".equals(name)) return "Java TCK multi-requester callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }

      private boolean discovered(ObjectInstanceHandle object) {
         return object.equals(discoveredObject);
      }

      private List<ProvidedUpdate> provided() { return List.copyOf(provided); }
      private List<Reflection> reflections() { return List.copyOf(reflections); }
      private void clearProvided() { provided.clear(); }
      private void clearReflections() { reflections.clear(); }
   }
}
