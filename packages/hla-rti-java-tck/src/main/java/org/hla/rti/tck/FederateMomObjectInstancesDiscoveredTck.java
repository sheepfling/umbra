package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies HLAobjectInstancesDiscovered deltas and rediscovery in the standard federate MOM. */
final class FederateMomObjectInstancesDiscoveredTck {
   private static final String MOM_CLASS = "HLAobjectRoot.HLAmanager.HLAfederate";
   private static final String SET_TIMING =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming";
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private FederateMomObjectInstancesDiscoveredTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder ownerRecorder = new Recorder();
      Recorder observerRecorder = new Recorder();
      String federation = "java-tck-federate-mom-object-instances-discovered-" + UUID.randomUUID();
      boolean ownerConnected = false, observerConnected = false, created = false;
      boolean ownerJoined = false, observerJoined = false, momSubscribed = false;
      boolean appSubscribed = false, appPublished = false;
      ObjectClassHandle momClass = null, ownerClass = null, observerClass = null;
      AttributeHandleSet momAttributes = null, ownerAttributes = null, observerAttributes = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         observer.connect(observerRecorder.proxy(), callbackModel);
         observerConnected = true;
         owner.createFederationExecutionWithMIM(federation, new String[] {fom}, mim,
            timeImplementation);
         created = true;
         FederateHandle ownerFederate = owner.joinFederationExecution(
            "mom-discovered-owner", "java-tck", federation);
         ownerJoined = true;
         FederateHandle observerFederate = observer.joinFederationExecution(
            "mom-discovered-observer", "java-tck", federation);
         observerJoined = true;

         momClass = observer.getObjectClassHandle(MOM_CLASS);
         AttributeHandle federateAttribute = observer.getAttributeHandle(momClass,
            "HLAfederateHandle");
         AttributeHandle countAttribute = observer.getAttributeHandle(momClass,
            "HLAobjectInstancesDiscovered");
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(MOM_CLASS.equals(observer.getObjectClassName(momClass))
               && "HLAfederateHandle".equals(observer.getAttributeName(momClass,
                  federateAttribute))
               && "HLAobjectInstancesDiscovered".equals(observer.getAttributeName(momClass,
                  countAttribute))
               && "HLAreliable".equals(observer.getTransportationTypeName(reliable)),
            "standard federate MOM object-instances-discovered lookup did not round-trip");
         momAttributes = JavaTckSupport.attributeSet(observer, federateAttribute, countAttribute);
         observer.subscribeObjectClassAttributes(momClass, momAttributes, "");
         momSubscribed = true;

         byte[] encodedOwner = encode(ownerFederate);
         await(observer, callbackModel,
            () -> observerRecorder.momObject(federateAttribute, encodedOwner) != null,
            "owner federate MOM discovery");
         byte[] encodedObserver = encode(observerFederate);
         await(observer, callbackModel,
            () -> observerRecorder.momObject(federateAttribute, encodedObserver) != null,
            "observer federate MOM discovery");
         ObjectInstanceHandle observerMomObject = observerRecorder.momObject(federateAttribute,
            encodedObserver);
         ObjectInstanceHandle ownerMomObject = observerRecorder.momObject(federateAttribute,
            encodedOwner);
         JavaTckSupport.check(ownerMomObject != null && observerMomObject != null
               && momClass.equals(observer.getKnownObjectClassHandle(ownerMomObject))
               && momClass.equals(observer.getKnownObjectClassHandle(observerMomObject)),
            "observer found an incorrect joined-federate MOM object");
         int directBaseline = requestCount(observer, observerRecorder, callbackModel,
            observerMomObject, countAttribute, reliable, encoder, "initial discovered-object count");
         JavaTckSupport.check(directBaseline >= 0 && directBaseline <= Integer.MAX_VALUE - 3,
            "initial HLAobjectInstancesDiscovered count cannot safely support three discoveries");

         ownerClass = owner.getObjectClassHandle(objectClassName);
         observerClass = observer.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle observerAttribute = observer.getAttributeHandle(observerClass,
            attributeName);
         JavaTckSupport.check(ownerClass.equals(observerClass)
               && ownerAttribute.equals(observerAttribute)
               && objectClassName.equals(owner.getObjectClassName(ownerClass))
               && objectClassName.equals(observer.getObjectClassName(observerClass))
               && attributeName.equals(owner.getAttributeName(ownerClass, ownerAttribute))
               && attributeName.equals(observer.getAttributeName(observerClass, observerAttribute)),
            "adapter-supplied object class or attribute handles did not round-trip");
         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         observerAttributes = JavaTckSupport.attributeSet(observer, observerAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         appPublished = true;
         observer.subscribeObjectClassAttributes(observerClass, observerAttributes, "");
         appSubscribed = true;

         int firstDiscoveryStart = observerRecorder.discoveries.size();
         ObjectInstanceHandle firstObject = owner.registerObjectInstance(ownerClass);
         ObjectInstanceHandle secondObject = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(firstObject != null && secondObject != null
               && !firstObject.equals(secondObject),
            "application registration returned invalid or duplicate object handles");
         awaitNewDiscoveries(observer, observerRecorder, callbackModel, firstDiscoveryStart,
            firstObject, secondObject);
         JavaTckSupport.check(requestCount(observer, observerRecorder, callbackModel,
               observerMomObject, countAttribute, reliable, encoder,
               "direct count after two application discoveries") == directBaseline + 2,
            "HLAobjectInstancesDiscovered did not increase once for each new object");

         int rediscoveryStart = observerRecorder.discoveries.size();
         observer.localDeleteObjectInstance(firstObject);
         JavaTckSupport.expectFailure(() -> observer.getKnownObjectClassHandle(firstObject),
            "ObjectInstanceNotKnown", "known-object query after local deletion");
         observer.unsubscribeObjectClassAttributes(observerClass, observerAttributes);
         appSubscribed = false;
         observer.subscribeObjectClassAttributes(observerClass, observerAttributes, "");
         appSubscribed = true;
         awaitNewDiscoveries(observer, observerRecorder, callbackModel, rediscoveryStart,
            firstObject);
         Discovery rediscovered = observerRecorder.discovery(firstObject, rediscoveryStart);
         JavaTckSupport.check(rediscovered != null && observerClass.equals(rediscovered.objectClass)
               && ownerFederate.equals(rediscovered.producer)
               && observerClass.equals(observer.getKnownObjectClassHandle(firstObject)),
            "local-delete rediscovery had incorrect object, class, or producer metadata");
         JavaTckSupport.check(requestCount(observer, observerRecorder, callbackModel,
               observerMomObject, countAttribute, reliable, encoder,
               "direct count after local-delete rediscovery") == directBaseline + 3,
            "HLAobjectInstancesDiscovered did not count rediscovery");

         InteractionClassHandle setTiming = observer.getInteractionClassHandle(SET_TIMING);
         ParameterHandle timingFederate = observer.getParameterHandle(setTiming, "HLAfederate");
         ParameterHandle reportPeriod = observer.getParameterHandle(setTiming, "HLAreportPeriod");
         JavaTckSupport.check(SET_TIMING.equals(observer.getInteractionClassName(setTiming))
               && "HLAfederate".equals(observer.getParameterName(setTiming, timingFederate))
               && "HLAreportPeriod".equals(observer.getParameterName(setTiming, reportPeriod)),
            "standard HLAsetTiming lookup did not round-trip");
         int periodicStart = observerRecorder.reflections.size();
         sendTiming(observer, setTiming, timingFederate, reportPeriod, observerFederate, 1, encoder);
         await(observer, callbackModel,
            () -> findCountAtLeast(observerRecorder, periodicStart, observerMomObject,
               countAttribute, encoder, directBaseline + 3) != null,
            "periodic object-instances-discovered value");
         Reflection periodic = findCountAtLeast(observerRecorder, periodicStart, observerMomObject,
            countAttribute, encoder, directBaseline + 3);
         JavaTckSupport.check(periodic != null && verifyCount(periodic, countAttribute, reliable,
               encoder, "periodic object-instances-discovered") == directBaseline + 3,
            "periodic HLAobjectInstancesDiscovered disagreed with direct count");
         JavaTckSupport.check(requestCount(observer, observerRecorder, callbackModel, observerMomObject,
               countAttribute, reliable, encoder, "direct count after periodic report")
                  == directBaseline + 3,
            "direct HLAobjectInstancesDiscovered disagreed with periodic report");
         sendTiming(observer, setTiming, timingFederate, reportPeriod, observerFederate, 0, encoder);
         JavaTckSupport.check(requestCount(observer, observerRecorder, callbackModel, observerMomObject,
               countAttribute, reliable, encoder, "count after disabling periodic reports")
                  == directBaseline + 3,
            "discovered-object count changed after periodic reporting was disabled");

         observer.unsubscribeObjectClassAttributes(observerClass, observerAttributes);
         appSubscribed = false;
         observer.unsubscribeObjectClassAttributes(momClass, momAttributes);
         momSubscribed = false;
         owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes);
         appPublished = false;
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         observerJoined = false;
         owner.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         ownerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (appSubscribed && observerJoined && observerClass != null && observerAttributes != null)
            try { observer.unsubscribeObjectClassAttributes(observerClass, observerAttributes); }
            catch (Exception ignored) { }
         if (momSubscribed && observerJoined && momClass != null && momAttributes != null)
            try { observer.unsubscribeObjectClassAttributes(momClass, momAttributes); }
            catch (Exception ignored) { }
         if (appPublished && ownerJoined && ownerClass != null && ownerAttributes != null)
            try { owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes); }
            catch (Exception ignored) { }
         if (observerJoined) resign(observer);
         if (ownerJoined) resign(owner);
         if (created) try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (observerConnected) disconnect(observer);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static int requestCount(RTIambassador rti, Recorder recorder, CallbackModel model,
         ObjectInstanceHandle object, AttributeHandle attribute, TransportationTypeHandle reliable,
         EncoderFactory encoder, String description) throws Exception {
      int start = recorder.reflections.size();
      rti.requestAttributeValueUpdate(object, JavaTckSupport.attributeSet(rti, attribute),
         new byte[0]);
      await(rti, model, () -> findCount(recorder, start, object, attribute) != null, description);
      Reflection reflection = findCount(recorder, start, object, attribute);
      JavaTckSupport.check(reflection != null, description + " produced no MOM value");
      return verifyCount(reflection, attribute, reliable, encoder, description);
   }

   private static int verifyCount(Reflection reflection, AttributeHandle attribute,
         TransportationTypeHandle reliable, EncoderFactory encoder, String description)
         throws Exception {
      JavaTckSupport.check(reflection.values.size() == 1
            && reflection.values.containsKey(attribute) && reflection.tag.length == 0
            && reliable.equals(reflection.transportation) && reflection.producer == null
            && (reflection.regions == null || reflection.regions.isEmpty()),
         description + " returned non-standard MOM reflection metadata");
      HLAinteger32BE value = encoder.createHLAinteger32BE();
      value.decode(reflection.values.get(attribute));
      JavaTckSupport.check(value.getValue() >= 0,
         description + " returned a negative object count");
      return value.getValue();
   }

   private static Reflection findCount(Recorder recorder, int start, ObjectInstanceHandle object,
         AttributeHandle attribute) {
      for (int i = Math.min(start, recorder.reflections.size()); i < recorder.reflections.size(); ++i) {
         Reflection reflection = recorder.reflections.get(i);
         if (object.equals(reflection.object) && reflection.values.containsKey(attribute))
            return reflection;
      }
      return null;
   }

   private static Reflection findCountAtLeast(Recorder recorder, int start,
         ObjectInstanceHandle object, AttributeHandle attribute, EncoderFactory encoder, int minimum) {
      for (int i = Math.min(start, recorder.reflections.size()); i < recorder.reflections.size(); ++i) {
         Reflection reflection = recorder.reflections.get(i);
         if (!object.equals(reflection.object) || !reflection.values.containsKey(attribute)) continue;
         HLAinteger32BE value = encoder.createHLAinteger32BE();
         try { value.decode(reflection.values.get(attribute)); }
         catch (Exception error) { throw new IllegalStateException("invalid MOM counter encoding", error); }
         if (value.getValue() >= minimum) return reflection;
      }
      return null;
   }

   private static void sendTiming(RTIambassador rti, InteractionClassHandle interaction,
         ParameterHandle federateParameter, ParameterHandle periodParameter, FederateHandle target,
         int period, EncoderFactory encoder) throws Exception {
      ParameterHandleValueMap values = rti.getParameterHandleValueMapFactory().create(2);
      values.put(federateParameter, encode(target));
      values.put(periodParameter, encoder.createHLAinteger32BE(period).toByteArray());
      rti.sendInteraction(interaction, values, new byte[0]);
   }

   private static byte[] encode(FederateHandle handle) throws Exception {
      byte[] bytes = new byte[handle.encodedLength()];
      handle.encode(bytes, 0);
      return bytes;
   }

   private static void awaitNewDiscoveries(RTIambassador rti, Recorder recorder,
         CallbackModel model, int start, ObjectInstanceHandle... objects) throws Exception {
      await(rti, model, () -> {
         for (ObjectInstanceHandle object : objects)
            if (recorder.discovery(object, start) == null) return false;
         return true;
      }, "application object discovery");
   }

   private static void await(RTIambassador rti, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         if (model == CallbackModel.HLA_EVOKED) rti.evokeCallback(0.0);
         else Thread.yield();
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void resign(RTIambassador rti) {
      try { rti.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador rti) {
      try { rti.disconnect(); } catch (Exception ignored) { }
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final AttributeHandleValueMap values;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final RegionHandleSet regions;
      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap values, byte[] tag,
            TransportationTypeHandle transportation, FederateHandle producer,
            RegionHandleSet regions) {
         this.object = object;
         this.values = values.clone();
         this.tag = tag.clone();
         this.transportation = transportation;
         this.producer = producer;
         this.regions = regions == null ? null : (RegionHandleSet) regions.clone();
      }
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

   private static final class Recorder {
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();

      private ObjectInstanceHandle momObject(AttributeHandle attribute, byte[] expected) {
         for (Reflection reflection : reflections) {
            byte[] value = reflection.values.get(attribute);
            if (value != null && Arrays.equals(expected, value)) return reflection.object;
         }
         return null;
      }

      private Discovery discovery(ObjectInstanceHandle object, int start) {
         for (int i = Math.min(start, discoveries.size()); i < discoveries.size(); ++i)
            if (object.equals(discoveries.get(i).object)) return discoveries.get(i);
         return null;
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 4) {
                  discoveries.add(new Discovery((ObjectInstanceHandle) arguments[0],
                     (ObjectClassHandle) arguments[1], (FederateHandle) arguments[3]));
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 6) {
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     (RegionHandleSet) arguments[5]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK object-instances-discovered MOM callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
