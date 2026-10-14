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
import hla.rti1516_2025.encoding.HLAbyte;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies HLAupdatesSent counts successful update invocations, including repeats. */
final class FederateMomUpdatesSentTck {
   private static final String MOM_CLASS = "HLAobjectRoot.HLAmanager.HLAfederate";
   private static final String SET_TIMING =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming";
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private FederateMomUpdatesSentTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder publisherRecorder = new Recorder();
      Recorder observerRecorder = new Recorder();
      String federation = "java-tck-federate-mom-updates-sent-" + UUID.randomUUID();
      boolean publisherConnected = false, observerConnected = false, created = false;
      boolean publisherJoined = false, observerJoined = false, momSubscribed = false;
      boolean appPublished = false, appSubscribed = false;
      ObjectClassHandle momClass = null, publisherClass = null, observerClass = null;
      AttributeHandleSet momAttributes = null, appAttributes = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         observer.connect(observerRecorder.proxy(), callbackModel);
         observerConnected = true;
         publisher.createFederationExecutionWithMIM(federation, new String[] {fom}, mim,
            timeImplementation);
         created = true;
         FederateHandle publisherFederate = publisher.joinFederationExecution(
            "updates-sent-publisher", "java-tck", federation);
         publisherJoined = true;

         momClass = publisher.getObjectClassHandle(MOM_CLASS);
         AttributeHandle federateAttribute = publisher.getAttributeHandle(momClass,
            "HLAfederateHandle");
         AttributeHandle updatesSentAttribute = publisher.getAttributeHandle(momClass,
            "HLAupdatesSent");
         TransportationTypeHandle reliable = publisher.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(MOM_CLASS.equals(publisher.getObjectClassName(momClass))
               && "HLAfederateHandle".equals(publisher.getAttributeName(momClass,
                  federateAttribute))
               && "HLAupdatesSent".equals(publisher.getAttributeName(momClass,
                  updatesSentAttribute))
               && "HLAreliable".equals(publisher.getTransportationTypeName(reliable)),
            "standard federate MOM updates-sent lookup did not round-trip");
         momAttributes = JavaTckSupport.attributeSet(publisher, federateAttribute,
            updatesSentAttribute);
         publisher.subscribeObjectClassAttributes(momClass, momAttributes, "");
         momSubscribed = true;

         observer.joinFederationExecution("updates-sent-observer", "java-tck", federation);
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
               && attributeName.equals(observer.getAttributeName(observerClass,
                  observerAttribute)),
            "adapter-supplied application object and attribute handles did not round-trip");
         appAttributes = JavaTckSupport.attributeSet(publisher, publisherAttribute);
         publisher.publishObjectClassAttributes(publisherClass, appAttributes);
         appPublished = true;
         observer.subscribeObjectClassAttributes(observerClass,
            JavaTckSupport.attributeSet(observer, observerAttribute), "");
         appSubscribed = true;

         ObjectInstanceHandle firstObject = publisher.registerObjectInstance(publisherClass);
         ObjectInstanceHandle secondObject = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(firstObject != null && secondObject != null
               && !firstObject.equals(secondObject),
            "application registration returned invalid or duplicate object handles");
         ObjectInstanceHandle expectedFirst = firstObject, expectedSecond = secondObject;
         await(observer, callbackModel,
            () -> observerRecorder.discoveries.contains(expectedFirst)
               && observerRecorder.discoveries.contains(expectedSecond),
            "observer discovery of both application objects");
         byte[] encodedFederate = encode(publisherFederate);
         await(publisher, callbackModel,
            () -> publisherRecorder.momObject(federateAttribute, encodedFederate) != null,
            "publisher federate MOM discovery");
         ObjectInstanceHandle momObject = publisherRecorder.momObject(federateAttribute,
            encodedFederate);
         JavaTckSupport.check(momObject != null
               && momClass.equals(publisher.getKnownObjectClassHandle(momObject)),
            "publisher MOM discovery returned the wrong object class");

         JavaTckSupport.check(requestCount(publisher, publisherRecorder, callbackModel, momObject,
               updatesSentAttribute, reliable, encoder, "initial updates-sent") == 0,
            "HLAupdatesSent was nonzero before application updates");

         byte[] firstPayload = new byte[] {0x61};
         byte[] repeatedPayload = new byte[] {0x62};
         byte[] secondObjectPayload = new byte[] {0x63};
         sendUpdate(publisher, observer, observerRecorder, callbackModel, firstObject,
            publisherAttribute, observerAttribute, publisherFederate, firstPayload, reliable,
            encoder,
            "first receive-order attribute update");
         JavaTckSupport.check(requestCount(publisher, publisherRecorder, callbackModel, momObject,
               updatesSentAttribute, reliable, encoder, "updates-sent after first update") == 1,
            "HLAupdatesSent did not count the first update invocation");
         sendUpdate(publisher, observer, observerRecorder, callbackModel, firstObject,
            publisherAttribute, observerAttribute, publisherFederate, repeatedPayload, reliable,
            encoder,
            "repeated receive-order update to one object");
         JavaTckSupport.check(requestCount(publisher, publisherRecorder, callbackModel, momObject,
               updatesSentAttribute, reliable, encoder, "updates-sent after repeated update") == 2,
            "HLAupdatesSent did not count a repeated update invocation");
         sendUpdate(publisher, observer, observerRecorder, callbackModel, secondObject,
            publisherAttribute, observerAttribute, publisherFederate, secondObjectPayload, reliable,
            encoder,
            "receive-order update to a second object");
         int directBaseline = requestCount(publisher, publisherRecorder, callbackModel, momObject,
            updatesSentAttribute, reliable, encoder, "updates-sent direct baseline");
         JavaTckSupport.check(directBaseline == 3,
            "HLAupdatesSent did not count three successful invocations");

         InteractionClassHandle setTiming = publisher.getInteractionClassHandle(SET_TIMING);
         ParameterHandle timingFederate = publisher.getParameterHandle(setTiming, "HLAfederate");
         ParameterHandle reportPeriod = publisher.getParameterHandle(setTiming, "HLAreportPeriod");
         JavaTckSupport.check(SET_TIMING.equals(publisher.getInteractionClassName(setTiming))
               && "HLAfederate".equals(publisher.getParameterName(setTiming, timingFederate))
               && "HLAreportPeriod".equals(publisher.getParameterName(setTiming, reportPeriod)),
            "standard HLAsetTiming lookup did not round-trip");
         int periodicReflectionStart = publisherRecorder.reflections.size();
         sendTiming(publisher, setTiming, timingFederate, reportPeriod, publisherFederate, 1,
            encoder);
         JavaTckSupport.check(requestCount(publisher, publisherRecorder, callbackModel, momObject,
               updatesSentAttribute, reliable, encoder, "updates-sent after enabling reports")
               == directBaseline,
            "HLAsetTiming changed the updates-sent count");

         byte[] periodicFirstPayload = new byte[] {0x64};
         byte[] periodicSecondPayload = new byte[] {0x65};
         sendUpdate(publisher, observer, observerRecorder, callbackModel, firstObject,
            publisherAttribute, observerAttribute, publisherFederate, periodicFirstPayload,
            reliable, encoder,
            "first periodic-phase receive-order update");
         sendUpdate(publisher, observer, observerRecorder, callbackModel, secondObject,
            publisherAttribute, observerAttribute, publisherFederate, periodicSecondPayload,
            reliable, encoder,
            "second periodic-phase receive-order update");
         int expectedCount = directBaseline + 2;
         await(publisher, callbackModel,
            () -> findCountAtLeast(publisherRecorder, periodicReflectionStart, momObject,
               updatesSentAttribute, encoder, expectedCount) != null,
            "periodic HLAupdatesSent value");
         Reflection periodic = findCountAtLeast(publisherRecorder, periodicReflectionStart,
            momObject, updatesSentAttribute, encoder, expectedCount);
         JavaTckSupport.check(periodic != null
               && verifyCount(periodic, updatesSentAttribute, reliable, encoder,
                  "periodic updates-sent") == expectedCount,
            "periodic HLAupdatesSent did not match successful update invocations");
         JavaTckSupport.check(requestCount(publisher, publisherRecorder, callbackModel, momObject,
               updatesSentAttribute, reliable, encoder, "direct updates-sent after periodic updates")
               == expectedCount,
            "direct HLAupdatesSent omitted periodic-phase update invocations");
         sendTiming(publisher, setTiming, timingFederate, reportPeriod, publisherFederate, 0,
            encoder);
         JavaTckSupport.check(requestCount(publisher, publisherRecorder, callbackModel, momObject,
               updatesSentAttribute, reliable, encoder, "updates-sent after disabling reports")
               == expectedCount,
            "HLAupdatesSent changed after periodic reporting was disabled");

         observer.unsubscribeObjectClassAttributes(observerClass,
            JavaTckSupport.attributeSet(observer, observerAttribute));
         appSubscribed = false;
         publisher.unsubscribeObjectClassAttributes(momClass, momAttributes);
         momSubscribed = false;
         publisher.unpublishObjectClassAttributes(publisherClass, appAttributes);
         appPublished = false;
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         observerJoined = false;
         publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (appSubscribed && observerJoined && observerClass != null) try {
            observer.unsubscribeObjectClassAttributes(observerClass,
               JavaTckSupport.attributeSet(observer,
                  observer.getAttributeHandle(observerClass, attributeName)));
         } catch (Exception ignored) { }
         if (momSubscribed && publisherJoined && momClass != null && momAttributes != null) try {
            publisher.unsubscribeObjectClassAttributes(momClass, momAttributes);
         } catch (Exception ignored) { }
         if (appPublished && publisherJoined && publisherClass != null && appAttributes != null) try {
            publisher.unpublishObjectClassAttributes(publisherClass, appAttributes);
         } catch (Exception ignored) { }
         if (observerJoined) resign(observer);
         if (publisherJoined) resign(publisher);
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (observerConnected) disconnect(observer);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static void sendUpdate(RTIambassador publisher, RTIambassador observer,
         Recorder observerRecorder, CallbackModel callbackModel, ObjectInstanceHandle object,
         AttributeHandle publisherAttribute, AttributeHandle observerAttribute,
         FederateHandle publisherFederate, byte[] payload, TransportationTypeHandle reliable,
         EncoderFactory encoder, String description) throws Exception {
      int first = observerRecorder.reflections.size();
      HLAbyte value = encoder.createHLAbyte(payload[0]);
      AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(1);
      values.put(publisherAttribute, value.toByteArray());
      publisher.updateAttributeValues(object, values, new byte[0]);
      await(observer, callbackModel,
         () -> findUpdate(observerRecorder, first, object, observerAttribute, payload) != null,
         description);
      Reflection reflection = findUpdate(observerRecorder, first, object, observerAttribute, payload);
      JavaTckSupport.check(reflection != null
            && countUpdateReflections(observerRecorder, first, object, observerAttribute, payload) == 1
            && reflection.values.size() == 1 && reflection.tag.length == 0
            && reliable.equals(reflection.transportation)
            && publisherFederate.equals(reflection.producer) && (reflection.regions == null
               || reflection.regions.isEmpty()),
         description + " produced missing, duplicate, or non-standard reflection data");
   }

   private static Reflection findUpdate(Recorder recorder, int first, ObjectInstanceHandle object,
         AttributeHandle attribute, byte[] payload) {
      byte[] encoded = new byte[] {payload[0]};
      for (int i = Math.min(first, recorder.reflections.size());
            i < recorder.reflections.size(); ++i) {
         Reflection reflection = recorder.reflections.get(i);
         byte[] value = reflection.values.get(attribute);
         if (reflection.object.equals(object) && value != null && Arrays.equals(encoded, value))
            return reflection;
      }
      return null;
   }

   private static int countUpdateReflections(Recorder recorder, int first,
         ObjectInstanceHandle object, AttributeHandle attribute, byte[] payload) {
      int count = 0;
      for (int i = Math.min(first, recorder.reflections.size());
            i < recorder.reflections.size(); ++i) {
         Reflection reflection = recorder.reflections.get(i);
         byte[] value = reflection.values.get(attribute);
         if (reflection.object.equals(object) && value != null && value.length == 1
               && value[0] == payload[0]) ++count;
      }
      return count;
   }

   private static int requestCount(RTIambassador rti, Recorder recorder, CallbackModel model,
         ObjectInstanceHandle object, AttributeHandle attribute,
         TransportationTypeHandle reliable, EncoderFactory encoder, String description)
         throws Exception {
      int first = recorder.reflections.size();
      rti.requestAttributeValueUpdate(object, JavaTckSupport.attributeSet(rti, attribute),
         new byte[0]);
      await(rti, model, () -> findCount(recorder, first, object, attribute) != null, description);
      Reflection reflection = findCount(recorder, first, object, attribute);
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

   private static Reflection findCount(Recorder recorder, int first, ObjectInstanceHandle object,
         AttributeHandle attribute) {
      for (int i = Math.min(first, recorder.reflections.size());
            i < recorder.reflections.size(); ++i) {
         Reflection reflection = recorder.reflections.get(i);
         if (object.equals(reflection.object) && reflection.values.containsKey(attribute))
            return reflection;
      }
      return null;
   }

   private static Reflection findCountAtLeast(Recorder recorder, int first,
         ObjectInstanceHandle object, AttributeHandle attribute, EncoderFactory encoder,
         int minimum) {
      for (int i = Math.min(first, recorder.reflections.size());
            i < recorder.reflections.size(); ++i) {
         Reflection reflection = recorder.reflections.get(i);
         if (!object.equals(reflection.object) || !reflection.values.containsKey(attribute))
            continue;
         HLAinteger32BE value = encoder.createHLAinteger32BE();
         try { value.decode(reflection.values.get(attribute)); }
         catch (Exception error) { throw new IllegalStateException("invalid MOM counter encoding", error); }
         if (value.getValue() >= minimum) return reflection;
      }
      return null;
   }

   private static void sendTiming(RTIambassador rti, InteractionClassHandle interaction,
         ParameterHandle federateParameter, ParameterHandle periodParameter, FederateHandle target, int period,
         EncoderFactory encoder) throws Exception {
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
      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap values,
            byte[] tag, TransportationTypeHandle transportation, FederateHandle producer,
            RegionHandleSet regions) {
         this.object = object;
         this.values = values.clone();
         this.tag = tag.clone();
         this.transportation = transportation;
         this.producer = producer;
         this.regions = regions == null ? null : (RegionHandleSet) regions.clone();
      }
   }

   private static final class Recorder {
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();

      private ObjectInstanceHandle momObject(AttributeHandle attribute, byte[] expected) {
         for (Reflection reflection : reflections) {
            byte[] value = reflection.values.get(attribute);
            if (value != null && Arrays.equals(expected, value)) return reflection.object;
         }
         return null;
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 2) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 6) {
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     (RegionHandleSet) arguments[5]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK updates-sent MOM callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name))
                     return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
