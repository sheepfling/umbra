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
import hla.rti1516_2025.RegionHandleSet;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Tests local object deletion boundaries, rediscovery, and continued updates. */
final class LocalDeleteObjectInstanceTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private LocalDeleteObjectInstanceTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador unjoined = factory.getRtiAmbassador();
      Recorder unjoinedCallbacks = new Recorder();
      RTIambassador owner = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder requesterCallbacks = new Recorder();
      RTIambassador freshRequester = null;
      Recorder freshCallbacks = new Recorder();
      String federation = "java-tck-local-delete-object-instance-" + UUID.randomUUID();
      boolean unjoinedConnected = false, ownerConnected = false, requesterConnected = false;
      boolean freshConnected = false, created = false, ownerJoined = false;
      boolean requesterJoined = false, freshJoined = false;
      boolean ownerPublished = false, requesterSubscribed = false, freshSubscribed = false;
      ObjectClassHandle ownerClass = null, requesterClass = null, freshClass = null;
      AttributeHandle ownerAttribute = null, requesterAttribute = null, freshAttribute = null;
      try {
         ObjectInstanceHandle invalidObject =
            JavaTckSupport.invalidHandle(ObjectInstanceHandle.class);
         JavaTckSupport.expectFailure(() -> owner.localDeleteObjectInstance(invalidObject),
            "NotConnected", "local deletion before connect");
         unjoined.connect(unjoinedCallbacks.proxy(), callbackModel);
         unjoinedConnected = true;
         JavaTckSupport.expectFailure(() -> unjoined.localDeleteObjectInstance(invalidObject),
            "FederateNotExecutionMember", "local deletion after connect before join");

         owner.connect(ownerCallbacks.proxy(), callbackModel);
         ownerConnected = true;
         requester.connect(requesterCallbacks.proxy(), callbackModel);
         requesterConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = owner.joinFederationExecution(
            "java-tck-local-delete-owner", federation);
         ownerJoined = true;
         requester.joinFederationExecution("java-tck-local-delete-requester", federation);
         requesterJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         requesterClass = requester.getObjectClassHandle(objectClassName);
         ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         requesterAttribute = requester.getAttributeHandle(requesterClass, attributeName);
         JavaTckSupport.check(ownerClass != null && requesterClass != null
               && ownerAttribute != null && requesterAttribute != null,
            "adapter FOM did not resolve local-delete object class and attribute");
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet requesterAttributes = JavaTckSupport.attributeSet(
            requester, requesterAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         ownerPublished = true;
         requester.subscribeObjectClassAttributes(requesterClass, requesterAttributes);
         requesterSubscribed = true;

         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "local-delete setup returned an invalid object handle");
         await(requester, callbackModel, () -> hasDiscovery(requesterCallbacks, object),
            "local-delete object discovery");
         Discovery discovery = findDiscovery(requesterCallbacks, object);
         JavaTckSupport.check(requesterClass.equals(discovery.objectClass)
               && requesterClass.equals(requester.getKnownObjectClassHandle(object)),
            "local-delete discovery did not establish object identity");

         JavaTckSupport.expectFailure(() -> owner.localDeleteObjectInstance(object),
            "FederateOwnsAttributes", "local deletion while owning object attributes");

         byte[] acquisitionTag = new byte[] {0x51, 0x18};
         requester.publishObjectClassAttributes(requesterClass, requesterAttributes);
         requester.attributeOwnershipAcquisition(object, requesterAttributes, acquisitionTag);
         JavaTckSupport.expectFailure(() -> requester.localDeleteObjectInstance(object),
            "OwnershipAcquisitionPending", "local deletion while ownership acquisition is pending");
         requester.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         requesterJoined = false;

         RTIambassador fresh = factory.getRtiAmbassador();
         freshRequester = fresh;
         fresh.connect(freshCallbacks.proxy(), callbackModel);
         freshConnected = true;
         fresh.joinFederationExecution("java-tck-local-delete-requester-fresh", federation);
         freshJoined = true;
         freshClass = fresh.getObjectClassHandle(objectClassName);
         freshAttribute = fresh.getAttributeHandle(freshClass, attributeName);
         JavaTckSupport.check(freshClass != null && freshAttribute != null,
            "fresh requester FOM lookup returned invalid handles");
         AttributeHandleSet freshAttributes = JavaTckSupport.attributeSet(
            fresh, freshAttribute);
         fresh.subscribeObjectClassAttributes(freshClass, freshAttributes);
         freshSubscribed = true;
         await(fresh, callbackModel, () -> hasDiscovery(freshCallbacks, object),
            "fresh requester object discovery");

         freshCallbacks.removals.clear();
         fresh.localDeleteObjectInstance(object);
         JavaTckSupport.check(freshCallbacks.removals.isEmpty(),
            "local object deletion unexpectedly delivered a removal callback");
         JavaTckSupport.expectFailure(() -> fresh.getKnownObjectClassHandle(object),
            "ObjectInstanceNotKnown", "object identity after local deletion");

         freshCallbacks.discoveries.clear();
         fresh.subscribeObjectClassAttributes(freshClass, freshAttributes);
         await(fresh, callbackModel, () -> hasDiscovery(freshCallbacks, object),
            "rediscovery after local deletion");
         JavaTckSupport.check(freshClass.equals(fresh.getKnownObjectClassHandle(object)),
            "rediscovery after local deletion did not restore object identity");

         freshCallbacks.reflections.clear();
         byte[] value = new byte[] {0x04, 0x20};
         AttributeHandleValueMap values = owner.getAttributeHandleValueMapFactory().create(1);
         values.put(ownerAttribute, value);
         byte[] updateTag = new byte[] {0x7a};
         owner.updateAttributeValues(object, values, updateTag);
         await(fresh, callbackModel,
            () -> freshCallbacks.reflections.size() == 1,
            "post-local-delete attribute reflection");
         Reflection reflection = freshCallbacks.reflections.get(0);
         JavaTckSupport.check(object.equals(reflection.object)
               && Arrays.equals(updateTag, reflection.tag)
               && producer.equals(reflection.producer)
               && Arrays.equals(value, reflection.values.get(freshAttribute)),
            "post-local-delete reflection returned the wrong metadata or value");
      } finally {
         if (freshJoined && freshSubscribed) {
            try { freshRequester.unsubscribeObjectClassAttributes(freshClass,
               JavaTckSupport.attributeSet(freshRequester, freshAttribute)); }
            catch (Exception ignored) { }
         }
         if (requesterJoined && requesterSubscribed) {
            try { requester.unsubscribeObjectClassAttributes(requesterClass,
               JavaTckSupport.attributeSet(requester, requesterAttribute)); }
            catch (Exception ignored) { }
         }
         if (ownerJoined && ownerPublished) {
            try { owner.unpublishObjectClassAttributes(ownerClass,
               JavaTckSupport.attributeSet(owner, ownerAttribute)); }
            catch (Exception ignored) { }
         }
         if (freshJoined) {
            try { freshRequester.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (requesterJoined) {
            try { requester.resignFederationExecution(
               ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (freshConnected) {
            try { freshRequester.disconnect(); } catch (Exception ignored) { }
         }
         if (requesterConnected) {
            try { requester.disconnect(); } catch (Exception ignored) { }
         }
         if (ownerConnected) {
            try { owner.disconnect(); } catch (Exception ignored) { }
         }
         if (unjoinedConnected) {
            try { unjoined.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static boolean hasDiscovery(Recorder recorder, ObjectInstanceHandle object) {
      return recorder.discoveries.stream().anyMatch(event -> object.equals(event.object));
   }

   private static Discovery findDiscovery(Recorder recorder, ObjectInstanceHandle object) {
      return recorder.discoveries.stream().filter(event -> object.equals(event.object))
         .findFirst().orElseThrow(() -> new AssertionError("object discovery was not recorded"));
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

   private static final class Discovery {
      private final ObjectInstanceHandle object;
      private final ObjectClassHandle objectClass;
      private final String name;
      private final FederateHandle producer;

      private Discovery(ObjectInstanceHandle object, ObjectClassHandle objectClass,
            String name, FederateHandle producer) {
         this.object = object;
         this.objectClass = objectClass;
         this.name = name;
         this.producer = producer;
      }
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final Map<AttributeHandle, byte[]> values;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;

      private Reflection(ObjectInstanceHandle object, Map<AttributeHandle, byte[]> values,
            byte[] tag, TransportationTypeHandle transportation, FederateHandle producer) {
         this.object = object;
         this.values = values;
         this.tag = tag;
         this.transportation = transportation;
         this.producer = producer;
      }
   }

   private static final class Recorder {
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();
      private final List<ObjectInstanceHandle> removals = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length == 4) {
                  discoveries.add(new Discovery((ObjectInstanceHandle) arguments[0],
                     (ObjectClassHandle) arguments[1], (String) arguments[2],
                     (FederateHandle) arguments[3]));
               }
               if ("removeObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  removals.add((ObjectInstanceHandle) arguments[0]);
               }
               if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length >= 5) {
                  Map<AttributeHandle, byte[]> values = new LinkedHashMap<>();
                  AttributeHandleValueMap callbackValues = (AttributeHandleValueMap) arguments[1];
                  for (AttributeHandle attribute : callbackValues.keySet()) {
                     values.put(attribute, callbackValues.get(attribute).clone());
                  }
                  byte[] tag = arguments[2] instanceof byte[] ? ((byte[]) arguments[2]).clone()
                     : new byte[0];
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0], values, tag,
                     (TransportationTypeHandle) arguments[3],
                     (FederateHandle) arguments[4]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK local-delete callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
