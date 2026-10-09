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
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks full and selective ordinary multi-attribute value requests and responses. */
final class MultiAttributeValueUpdateRequestResponseTck {
   private MultiAttributeValueUpdateRequestResponseTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String firstAttributeName, String secondAttributeName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      String federation = "java-tck-multi-attribute-request-response-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean requesterConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean requesterJoined = false;
      boolean published = false;
      boolean subscribed = false;
      ObjectClassHandle ownerClass = null;
      ObjectClassHandle requesterClass = null;
      AttributeHandle ownerFirst = null;
      AttributeHandle ownerSecond = null;
      AttributeHandle requesterFirst = null;
      AttributeHandle requesterSecond = null;
      AttributeHandleSet ownerAttributes = null;
      AttributeHandleSet requesterAttributes = null;
      ObjectInstanceHandle object = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         requester.connect(requesterRecorder.proxy(), callbackModel);
         requesterConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle ownerFederate = owner.joinFederationExecution(
            "java-tck-multi-attribute-owner", federation);
         ownerJoined = true;
         requester.joinFederationExecution("java-tck-multi-attribute-requester", federation);
         requesterJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         requesterClass = requester.getObjectClassHandle(objectClassName);
         ownerFirst = owner.getAttributeHandle(ownerClass, firstAttributeName);
         ownerSecond = owner.getAttributeHandle(ownerClass, secondAttributeName);
         requesterFirst = requester.getAttributeHandle(requesterClass, firstAttributeName);
         requesterSecond = requester.getAttributeHandle(requesterClass, secondAttributeName);
         JavaTckSupport.check(ownerClass.equals(requesterClass)
               && ownerFirst.equals(requesterFirst) && ownerSecond.equals(requesterSecond)
               && !ownerFirst.equals(ownerSecond),
            "adapter multi-attribute FOM returned inconsistent class or attribute handles");

         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerFirst, ownerSecond);
         requesterAttributes = JavaTckSupport.attributeSet(requester,
            requesterFirst, requesterSecond);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         published = true;
         requester.subscribeObjectClassAttributes(requesterClass, requesterAttributes);
         subscribed = true;
         ObjectInstanceHandle registeredObject = owner.registerObjectInstance(ownerClass);
         object = registeredObject;
         await(requester, () -> requesterRecorder.discovered(registeredObject),
            "requester did not discover the multi-attribute object before the deadline");
         JavaTckSupport.check(object.equals(requesterRecorder.discoveredObject),
            "multi-attribute object discovery returned the wrong handle");

         TransportationTypeHandle reliable =
            requester.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(reliable != null,
            "requester could not resolve the standard reliable transportation type");

         Map<AttributeHandle, byte[]> bothValues = new HashMap<>();
         bothValues.put(requesterFirst, new byte[] {0x11});
         bothValues.put(requesterSecond, new byte[] {0x22});
         runRequest(owner, requester, ownerRecorder, requesterRecorder, ownerFederate,
            object, requesterRecorder.discoveredObject, ownerFirst, ownerSecond,
            requesterFirst, requesterSecond, reliable, requesterAttributes,
            new byte[] {0x52, 0x31}, bothValues, new byte[] {0x52, 0x41},
            "full multi-attribute request");

         Map<AttributeHandle, byte[]> firstValue = new HashMap<>();
         firstValue.put(requesterFirst, new byte[] {0x33});
         runRequest(owner, requester, ownerRecorder, requesterRecorder, ownerFederate,
            object, requesterRecorder.discoveredObject, ownerFirst, ownerSecond,
            requesterFirst, requesterSecond, reliable,
            JavaTckSupport.attributeSet(requester, requesterFirst),
            new byte[] {0x52, 0x32}, firstValue, new byte[] {0x52, 0x42},
            "first-only multi-attribute request");

         Map<AttributeHandle, byte[]> secondValue = new HashMap<>();
         secondValue.put(requesterSecond, new byte[] {0x44});
         runRequest(owner, requester, ownerRecorder, requesterRecorder, ownerFederate,
            object, requesterRecorder.discoveredObject, ownerFirst, ownerSecond,
            requesterFirst, requesterSecond, reliable,
            JavaTckSupport.attributeSet(requester, requesterSecond),
            new byte[] {0x52, 0x33}, secondValue, new byte[] {0x52, 0x43},
            "second-only multi-attribute request");
         JavaTckSupport.check(ownerRecorder.reflections().isEmpty(),
            "multi-attribute value-request response looped back to the provider");

         requester.unsubscribeObjectClassAttributes(requesterClass, requesterAttributes);
         subscribed = false;
         owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes);
         published = false;
         requester.resignFederationExecution(ResignAction.NO_ACTION);
         requesterJoined = false;
         owner.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         ownerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (requesterJoined && subscribed && requesterClass != null) {
            try {
               requester.unsubscribeObjectClassAttributes(requesterClass,
                  JavaTckSupport.attributeSet(requester, requesterFirst, requesterSecond));
            } catch (Exception ignored) { }
         }
         if (ownerJoined && published && ownerClass != null) {
            try {
               owner.unpublishObjectClassAttributes(ownerClass,
                  JavaTckSupport.attributeSet(owner, ownerFirst, ownerSecond));
            } catch (Exception ignored) { }
         }
         if (requesterJoined) {
            try { requester.resignFederationExecution(ResignAction.NO_ACTION); }
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
         if (requesterConnected) {
            try { requester.disconnect(); }
            catch (Exception ignored) { }
         }
         if (ownerConnected) {
            try { owner.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static void runRequest(RTIambassador owner, RTIambassador requester,
         Recorder ownerRecorder, Recorder requesterRecorder, FederateHandle ownerFederate,
         ObjectInstanceHandle object, ObjectInstanceHandle knownObject,
         AttributeHandle ownerFirst, AttributeHandle ownerSecond,
         AttributeHandle requesterFirst, AttributeHandle requesterSecond,
         TransportationTypeHandle reliable, AttributeHandleSet requestedAttributes,
         byte[] requestTag, Map<AttributeHandle, byte[]> expectedValues,
         byte[] responseTag, String description) throws Exception {
      HashSet<AttributeHandle> expectedOwnerAttributes = new HashSet<>();
      for (AttributeHandle handle : requestedAttributes) {
         if (handle.equals(requesterFirst)) {
            expectedOwnerAttributes.add(ownerFirst);
         } else if (handle.equals(requesterSecond)) {
            expectedOwnerAttributes.add(ownerSecond);
         } else {
            throw new AssertionError(description + " requested an unknown attribute handle");
         }
      }
      ownerRecorder.clearProvided();
      requesterRecorder.clearReflections();
      requester.requestAttributeValueUpdate(knownObject, requestedAttributes, requestTag);
      await(owner, () -> !ownerRecorder.provided().isEmpty(),
         description + " did not reach the provider before the deadline");
      JavaTckSupport.drain(requester);
      List<ProvidedUpdate> requests = ownerRecorder.provided();
      JavaTckSupport.check(requests.size() == 1 && object.equals(requests.get(0).object)
            && expectedOwnerAttributes.equals(requests.get(0).attributes)
            && Arrays.equals(requestTag, requests.get(0).tag),
         description + " provider callback returned the wrong object, attributes, or tag");
      JavaTckSupport.check(requesterRecorder.reflections().isEmpty(),
         description + " was reflected before the provider answered");

      AttributeHandleValueMap response =
         owner.getAttributeHandleValueMapFactory().create(expectedValues.size());
      for (Map.Entry<AttributeHandle, byte[]> entry : expectedValues.entrySet()) {
         AttributeHandle ownerAttribute = entry.getKey().equals(requesterFirst)
            ? ownerFirst : ownerSecond;
         response.put(ownerAttribute, entry.getValue().clone());
      }
      owner.updateAttributeValues(object, response, responseTag);
      await(requester, () -> !requesterRecorder.reflections().isEmpty(),
         description + " response reflection was not delivered before the deadline");
      List<Reflection> reflections = requesterRecorder.reflections();
      JavaTckSupport.check(reflections.size() == 1,
         description + " produced an unexpected number of reflection callbacks");
      Reflection reflection = reflections.get(0);
      JavaTckSupport.check(object.equals(reflection.object)
            && reflection.values.size() == expectedValues.size()
            && Arrays.equals(responseTag, reflection.tag)
            && reliable.equals(reflection.transportation)
            && ownerFederate.equals(reflection.producer)
            && !reflection.timestampOrdered
            && (reflection.regions == null || reflection.regions.isEmpty()),
         description + " reflection returned incorrect object or standard metadata");
      for (Map.Entry<AttributeHandle, byte[]> entry : expectedValues.entrySet()) {
         JavaTckSupport.check(Arrays.equals(entry.getValue(), reflection.values.get(entry.getKey())),
            description + " reflection returned an incorrect attribute value");
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
                  if ("toString".equals(name)) return "Java TCK multi-attribute request callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
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
