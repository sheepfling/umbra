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

/** Verifies producer MOM deletion counts separately from recipient removal callbacks. */
final class FederateMomObjectInstancesDeletedTck {
   private static final String MOM_CLASS = "HLAobjectRoot.HLAmanager.HLAfederate";
   private static final String SET_TIMING =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming";
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private FederateMomObjectInstancesDeletedTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder publisherRecorder = new Recorder();
      Recorder observerRecorder = new Recorder();
      String federation = "java-tck-federate-mom-object-instances-deleted-" + UUID.randomUUID();
      boolean publisherConnected = false, observerConnected = false, created = false;
      boolean publisherJoined = false, observerJoined = false, momSubscribed = false;
      boolean appSubscribed = false, appPublished = false;
      ObjectClassHandle momClass = null, publisherClass = null, observerClass = null;
      AttributeHandleSet momAttributes = null, publisherAttributes = null, observerAttributes = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         observer.connect(observerRecorder.proxy(), callbackModel);
         observerConnected = true;
         publisher.createFederationExecutionWithMIM(federation, new String[] {fom}, mim,
            timeImplementation);
         created = true;
         FederateHandle publisherFederate = publisher.joinFederationExecution(
            "mom-deleted-publisher", "java-tck", federation);
         publisherJoined = true;

         momClass = publisher.getObjectClassHandle(MOM_CLASS);
         AttributeHandle federateAttribute = publisher.getAttributeHandle(momClass,
            "HLAfederateHandle");
         AttributeHandle countAttribute = publisher.getAttributeHandle(momClass,
            "HLAobjectInstancesDeleted");
         TransportationTypeHandle reliable = publisher.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(MOM_CLASS.equals(publisher.getObjectClassName(momClass))
               && "HLAfederateHandle".equals(publisher.getAttributeName(momClass,
                  federateAttribute))
               && "HLAobjectInstancesDeleted".equals(publisher.getAttributeName(momClass,
                  countAttribute))
               && "HLAreliable".equals(publisher.getTransportationTypeName(reliable)),
            "standard federate MOM object-instances-deleted lookup did not round-trip");
         momAttributes = JavaTckSupport.attributeSet(publisher, federateAttribute, countAttribute);
         publisher.subscribeObjectClassAttributes(momClass, momAttributes, "");
         momSubscribed = true;

         FederateHandle observerFederate = observer.joinFederationExecution(
            "mom-deleted-observer", "java-tck", federation);
         observerJoined = true;
         publisherClass = publisher.getObjectClassHandle(objectClassName);
         observerClass = observer.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(publisherClass,
            attributeName);
         AttributeHandle observerAttribute = observer.getAttributeHandle(observerClass,
            attributeName);
         JavaTckSupport.check(publisherClass.equals(observerClass)
               && publisherAttribute.equals(observerAttribute)
               && objectClassName.equals(publisher.getObjectClassName(publisherClass))
               && objectClassName.equals(observer.getObjectClassName(observerClass))
               && attributeName.equals(publisher.getAttributeName(publisherClass,
                  publisherAttribute))
               && attributeName.equals(observer.getAttributeName(observerClass, observerAttribute)),
            "adapter-supplied object class or attribute handles did not round-trip");
         publisherAttributes = JavaTckSupport.attributeSet(publisher, publisherAttribute);
         observerAttributes = JavaTckSupport.attributeSet(observer, observerAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         appPublished = true;
         observer.subscribeObjectClassAttributes(observerClass, observerAttributes, "");
         appSubscribed = true;

         ObjectInstanceHandle firstObject = publisher.registerObjectInstance(publisherClass);
         ObjectInstanceHandle secondObject = publisher.registerObjectInstance(publisherClass);
         ObjectInstanceHandle thirdObject = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(firstObject != null && secondObject != null && thirdObject != null
               && !firstObject.equals(secondObject) && !firstObject.equals(thirdObject)
               && !secondObject.equals(thirdObject),
            "application registration returned invalid or duplicate object handles");
         String firstObjectName = publisher.getObjectInstanceName(firstObject);
         String secondObjectName = publisher.getObjectInstanceName(secondObject);
         JavaTckSupport.check(firstObjectName != null && !firstObjectName.isEmpty()
               && secondObjectName != null && !secondObjectName.isEmpty(),
            "application registration returned an empty object instance name");
         await(observer, callbackModel,
            () -> observerRecorder.discovered(firstObject) && observerRecorder.discovered(secondObject)
               && observerRecorder.discovered(thirdObject),
            "observer discovery of three application objects");

         byte[] encodedPublisher = encode(publisherFederate);
         await(publisher, callbackModel,
            () -> publisherRecorder.momObject(federateAttribute, encodedPublisher) != null,
            "publisher federate MOM discovery");
         ObjectInstanceHandle publisherMomObject = publisherRecorder.momObject(federateAttribute,
            encodedPublisher);
         JavaTckSupport.check(publisherMomObject != null
               && momClass.equals(publisher.getKnownObjectClassHandle(publisherMomObject)),
            "publisher found the wrong joined-federate MOM object");
         JavaTckSupport.check(requestCount(publisher, publisherRecorder, callbackModel,
               publisherMomObject, countAttribute, reliable, encoder,
               "initial object-instances-deleted count") == 0,
            "HLAobjectInstancesDeleted was nonzero before object deletion");

         deleteAndObserve(publisher, observer, observerRecorder, callbackModel, publisherFederate,
            firstObject, new byte[] {(byte) 0x61}, "first receive-order deletion");
         JavaTckSupport.check(requestCount(publisher, publisherRecorder, callbackModel,
               publisherMomObject, countAttribute, reliable, encoder,
               "count after first deletion") == 1,
            "HLAobjectInstancesDeleted did not count the first successful deletion");
         JavaTckSupport.expectFailure(
            () -> observer.getObjectInstanceHandle(firstObjectName),
            "ObjectInstanceNotKnown",
            "first deleted-object-count removal did not retire its object name");
         deleteAndObserve(publisher, observer, observerRecorder, callbackModel, publisherFederate,
            secondObject, new byte[] {(byte) 0x62}, "second receive-order deletion");
         int directBaseline = requestCount(publisher, publisherRecorder, callbackModel,
            publisherMomObject, countAttribute, reliable, encoder,
            "direct object-instances-deleted baseline");
         JavaTckSupport.check(directBaseline == 2,
            "HLAobjectInstancesDeleted did not count two successful deletion invocations");
         JavaTckSupport.expectFailure(
            () -> observer.getObjectInstanceHandle(secondObjectName),
            "ObjectInstanceNotKnown",
            "second deleted-object-count removal did not retire its object name");

         InteractionClassHandle twoDeleteTiming = publisher.getInteractionClassHandle(SET_TIMING);
         ParameterHandle twoDeleteFederate = publisher.getParameterHandle(
            twoDeleteTiming, "HLAfederate");
         ParameterHandle twoDeletePeriod = publisher.getParameterHandle(
            twoDeleteTiming, "HLAreportPeriod");
         JavaTckSupport.check(SET_TIMING.equals(publisher.getInteractionClassName(twoDeleteTiming))
               && "HLAfederate".equals(publisher.getParameterName(
                  twoDeleteTiming, twoDeleteFederate))
               && "HLAreportPeriod".equals(publisher.getParameterName(
                  twoDeleteTiming, twoDeletePeriod)),
            "standard HLAsetTiming lookup did not round-trip");
         int twoDeletePeriodicStart = publisherRecorder.reflections.size();
         sendTiming(publisher, twoDeleteTiming, twoDeleteFederate, twoDeletePeriod,
            publisherFederate, 1, encoder);
         await(publisher, callbackModel,
            () -> findCountAtLeast(publisherRecorder, twoDeletePeriodicStart, publisherMomObject,
               countAttribute, encoder, 2) != null,
            "periodic HLAobjectInstancesDeleted value after two deletions");
         Reflection twoDeletePeriodic = findCountAtLeast(publisherRecorder,
            twoDeletePeriodicStart, publisherMomObject, countAttribute, encoder, 2);
         JavaTckSupport.check(twoDeletePeriodic != null
               && verifyCount(twoDeletePeriodic, countAttribute, reliable, encoder,
                  "periodic count after two deletions") == 2,
            "periodic HLAobjectInstancesDeleted did not report two successful deletions");
         sendTiming(publisher, twoDeleteTiming, twoDeleteFederate, twoDeletePeriod,
            publisherFederate, 0, encoder);
         JavaTckSupport.check(requestCount(publisher, publisherRecorder, callbackModel,
               publisherMomObject, countAttribute, reliable, encoder,
               "direct count after two-delete periodic report") == 2,
            "direct HLAobjectInstancesDeleted disagreed with the two-delete periodic value");

         InteractionClassHandle setTiming = publisher.getInteractionClassHandle(SET_TIMING);
         ParameterHandle timingFederate = publisher.getParameterHandle(setTiming, "HLAfederate");
         ParameterHandle reportPeriod = publisher.getParameterHandle(setTiming, "HLAreportPeriod");
         JavaTckSupport.check(SET_TIMING.equals(publisher.getInteractionClassName(setTiming))
               && "HLAfederate".equals(publisher.getParameterName(setTiming, timingFederate))
               && "HLAreportPeriod".equals(publisher.getParameterName(setTiming, reportPeriod)),
            "standard HLAsetTiming lookup did not round-trip");
         int periodicStart = publisherRecorder.reflections.size();
         sendTiming(publisher, setTiming, timingFederate, reportPeriod, publisherFederate, 1, encoder);
         deleteAndObserve(publisher, observer, observerRecorder, callbackModel, publisherFederate,
            thirdObject, new byte[] {(byte) 0x63}, "periodic-phase receive-order deletion");
         await(publisher, callbackModel,
            () -> findCountAtLeast(publisherRecorder, periodicStart, publisherMomObject,
               countAttribute, encoder, 3) != null,
            "periodic object-instances-deleted value");
         Reflection periodic = findCountAtLeast(publisherRecorder, periodicStart, publisherMomObject,
            countAttribute, encoder, 3);
         JavaTckSupport.check(periodic != null && verifyCount(periodic, countAttribute, reliable,
               encoder, "periodic object-instances-deleted") == 3,
            "periodic HLAobjectInstancesDeleted did not equal successful deletion invocations");
         JavaTckSupport.check(requestCount(publisher, publisherRecorder, callbackModel,
               publisherMomObject, countAttribute, reliable, encoder,
               "direct count after periodic report") == 3,
            "direct HLAobjectInstancesDeleted disagreed with periodic reporting");
         sendTiming(publisher, setTiming, timingFederate, reportPeriod, publisherFederate, 0, encoder);
         JavaTckSupport.check(requestCount(publisher, publisherRecorder, callbackModel,
               publisherMomObject, countAttribute, reliable, encoder,
               "count after disabling periodic reports") == 3,
            "HLAobjectInstancesDeleted changed after periodic reporting was disabled");

         observer.unsubscribeObjectClassAttributes(observerClass, observerAttributes);
         appSubscribed = false;
         publisher.unsubscribeObjectClassAttributes(momClass, momAttributes);
         momSubscribed = false;
         publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         appPublished = false;
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         observerJoined = false;
         publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (appSubscribed && observerJoined && observerClass != null && observerAttributes != null)
            try { observer.unsubscribeObjectClassAttributes(observerClass, observerAttributes); }
            catch (Exception ignored) { }
         if (momSubscribed && publisherJoined && momClass != null && momAttributes != null)
            try { publisher.unsubscribeObjectClassAttributes(momClass, momAttributes); }
            catch (Exception ignored) { }
         if (appPublished && publisherJoined && publisherClass != null && publisherAttributes != null)
            try { publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes); }
            catch (Exception ignored) { }
         if (observerJoined) resign(observer);
         if (publisherJoined) resign(publisher);
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (observerConnected) disconnect(observer);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static void deleteAndObserve(RTIambassador publisher, RTIambassador observer,
         Recorder recorder, CallbackModel model, FederateHandle publisherFederate,
         ObjectInstanceHandle object, byte[] tag, String description) throws Exception {
      int before = recorder.removals.size();
      publisher.deleteObjectInstance(object, tag);
      if (model == CallbackModel.HLA_EVOKED)
         JavaTckSupport.check(recorder.removals.size() == before,
            description + " arrived before callback servicing");
      await(observer, model, () -> recorder.removal(object, before) != null, description);
      Removal removal = recorder.removal(object, before);
      JavaTckSupport.check(recorder.removalCount(object, before) == 1 && removal != null
            && Arrays.equals(tag, removal.tag) && publisherFederate.equals(removal.producer),
         description + " produced missing, duplicate, or incorrectly attributed callback metadata");
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
      JavaTckSupport.check(value.getValue() >= 0, description + " returned a negative count");
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
   private static final class Removal {
      private final ObjectInstanceHandle object;
      private final byte[] tag;
      private final FederateHandle producer;
      private Removal(ObjectInstanceHandle object, byte[] tag, FederateHandle producer) {
         this.object = object;
         this.tag = tag.clone();
         this.producer = producer;
      }
   }
   private static final class Recorder {
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Removal> removals = new CopyOnWriteArrayList<>();
      private ObjectInstanceHandle momObject(AttributeHandle attribute, byte[] expected) {
         for (Reflection reflection : reflections) {
            byte[] value = reflection.values.get(attribute);
            if (value != null && Arrays.equals(expected, value)) return reflection.object;
         }
         return null;
      }
      private boolean discovered(ObjectInstanceHandle object) { return discoveries.contains(object); }
      private Removal removal(ObjectInstanceHandle object, int start) {
         for (int i = Math.min(start, removals.size()); i < removals.size(); ++i)
            if (object.equals(removals.get(i).object)) return removals.get(i);
         return null;
      }
      private int removalCount(ObjectInstanceHandle object, int start) {
         int count = 0;
         for (int i = Math.min(start, removals.size()); i < removals.size(); ++i)
            if (object.equals(removals.get(i).object)) ++count;
         return count;
      }
      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("removeObjectInstance".equals(name) && arguments != null
                     && arguments.length == 3) {
                  removals.add(new Removal((ObjectInstanceHandle) arguments[0],
                     (byte[]) arguments[1], (FederateHandle) arguments[2]));
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 6) {
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     (RegionHandleSet) arguments[5]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK object-instances-deleted MOM callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
