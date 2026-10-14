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

/** Verifies distinct-object accounting in the standard federate MOM. */
final class FederateMomObjectInstancesUpdatedTck {
   private static final String MOM_CLASS = "HLAobjectRoot.HLAmanager.HLAfederate";
   private static final String SET_TIMING =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming";
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private FederateMomObjectInstancesUpdatedTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder publisherRecorder = new Recorder();
      Recorder observerRecorder = new Recorder();
      String federation = "java-tck-federate-mom-object-instances-updated-" + UUID.randomUUID();
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
            "object-instances-updated-publisher", "java-tck", federation);
         publisherJoined = true;

         momClass = publisher.getObjectClassHandle(MOM_CLASS);
         AttributeHandle federateAttribute = publisher.getAttributeHandle(momClass,
            "HLAfederateHandle");
         AttributeHandle countAttribute = publisher.getAttributeHandle(momClass,
            "HLAobjectInstancesUpdated");
         TransportationTypeHandle reliable = publisher.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(MOM_CLASS.equals(publisher.getObjectClassName(momClass))
               && "HLAfederateHandle".equals(publisher.getAttributeName(momClass,
                  federateAttribute))
               && "HLAobjectInstancesUpdated".equals(publisher.getAttributeName(momClass,
                  countAttribute))
               && "HLAreliable".equals(publisher.getTransportationTypeName(reliable)),
            "standard federate MOM object-instances-updated lookup did not round-trip");
         momAttributes = JavaTckSupport.attributeSet(publisher, federateAttribute, countAttribute);
         publisher.subscribeObjectClassAttributes(momClass, momAttributes, "");
         momSubscribed = true;

         observer.joinFederationExecution("object-instances-updated-observer", "java-tck",
            federation);
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
         awaitDiscoveries(observer, observerRecorder, callbackModel, firstObject, secondObject);
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
               countAttribute, reliable, encoder, "initial object-instances-updated") == 0,
            "HLAobjectInstancesUpdated was nonzero before application updates");

         sendUpdate(publisher, observer, observerRecorder, callbackModel, firstObject,
            publisherAttribute, observerAttribute, publisherFederate, (byte) 0x41, reliable,
            encoder, "first object update");
         JavaTckSupport.check(requestCount(publisher, publisherRecorder, callbackModel, momObject,
               countAttribute, reliable, encoder, "count after first object update") == 1,
            "HLAobjectInstancesUpdated did not count the first updated object");
         sendUpdate(publisher, observer, observerRecorder, callbackModel, firstObject,
            publisherAttribute, observerAttribute, publisherFederate, (byte) 0x42, reliable,
            encoder, "repeated update to first object");
         JavaTckSupport.check(requestCount(publisher, publisherRecorder, callbackModel, momObject,
               countAttribute, reliable, encoder, "count after repeated first-object update") == 1,
            "HLAobjectInstancesUpdated counted repeated updates to one object more than once");
         sendUpdate(publisher, observer, observerRecorder, callbackModel, secondObject,
            publisherAttribute, observerAttribute, publisherFederate, (byte) 0x43, reliable,
            encoder, "update to second object");
         int directBaseline = requestCount(publisher, publisherRecorder, callbackModel, momObject,
            countAttribute, reliable, encoder, "direct distinct-object baseline");
         JavaTckSupport.check(directBaseline == 2,
            "HLAobjectInstancesUpdated did not count two distinct objects");

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
         ObjectInstanceHandle thirdObject = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(thirdObject != null && !thirdObject.equals(firstObject)
               && !thirdObject.equals(secondObject),
            "third object registration returned an invalid or duplicate handle");
         awaitDiscoveries(observer, observerRecorder, callbackModel, thirdObject);
         sendUpdate(publisher, observer, observerRecorder, callbackModel, thirdObject,
            publisherAttribute, observerAttribute, publisherFederate, (byte) 0x44, reliable,
            encoder, "first periodic-phase update to third object");
         sendUpdate(publisher, observer, observerRecorder, callbackModel, thirdObject,
            publisherAttribute, observerAttribute, publisherFederate, (byte) 0x45, reliable,
            encoder, "repeated periodic-phase update to third object");

         await(publisher, callbackModel,
            () -> findCountAtLeast(publisherRecorder, periodicReflectionStart, momObject,
               countAttribute, encoder, 3) != null,
            "periodic object-instances-updated value of three");
         Reflection periodic = findCountAtLeast(publisherRecorder, periodicReflectionStart,
            momObject, countAttribute, encoder, 3);
         JavaTckSupport.check(periodic != null
               && verifyCount(periodic, countAttribute, reliable, encoder,
                  "periodic object-instances-updated") == 3,
            "periodic object-instances-updated did not count three distinct updated objects");
         JavaTckSupport.check(requestCount(publisher, publisherRecorder, callbackModel, momObject,
               countAttribute, reliable, encoder, "direct count after periodic updates") == 3,
            "direct object-instances-updated disagreed with periodic reporting");
         sendTiming(publisher, setTiming, timingFederate, reportPeriod, publisherFederate, 0,
            encoder);
         JavaTckSupport.check(requestCount(publisher, publisherRecorder, callbackModel, momObject,
               countAttribute, reliable, encoder, "count after disabling periodic reports") == 3,
            "HLAobjectInstancesUpdated changed after periodic reporting was disabled");

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

   /** Verifies the registration, distinct-updated-object, and update-call MOM counters together. */
   static void runAllCounters(RtiFactory factory, String fom, String mim,
         String timeImplementation, String objectClassName, String attributeName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder publisherRecorder = new Recorder();
      Recorder observerRecorder = new Recorder();
      String federation = "java-tck-federate-mom-updated-object-count-periodic-" + UUID.randomUUID();
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
            "updated-count-subject", "java-tck", federation);
         publisherJoined = true;

         momClass = publisher.getObjectClassHandle(MOM_CLASS);
         AttributeHandle federateAttribute = publisher.getAttributeHandle(momClass,
            "HLAfederateHandle");
         AttributeHandle registeredAttribute = publisher.getAttributeHandle(momClass,
            "HLAobjectInstancesRegistered");
         AttributeHandle updatedAttribute = publisher.getAttributeHandle(momClass,
            "HLAobjectInstancesUpdated");
         AttributeHandle sentAttribute = publisher.getAttributeHandle(momClass, "HLAupdatesSent");
         TransportationTypeHandle reliable = publisher.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(MOM_CLASS.equals(publisher.getObjectClassName(momClass))
               && "HLAfederateHandle".equals(publisher.getAttributeName(momClass,
                  federateAttribute))
               && "HLAobjectInstancesRegistered".equals(publisher.getAttributeName(momClass,
                  registeredAttribute))
               && "HLAobjectInstancesUpdated".equals(publisher.getAttributeName(momClass,
                  updatedAttribute))
               && "HLAupdatesSent".equals(publisher.getAttributeName(momClass, sentAttribute))
               && "HLAreliable".equals(publisher.getTransportationTypeName(reliable)),
            "standard joined-federate MOM counter lookup did not round-trip");
         momAttributes = JavaTckSupport.attributeSet(publisher, federateAttribute,
            registeredAttribute, updatedAttribute, sentAttribute);
         AttributeHandleSet countAttributes = JavaTckSupport.attributeSet(publisher,
            registeredAttribute, updatedAttribute, sentAttribute);
         publisher.subscribeObjectClassAttributes(momClass, momAttributes, "");
         momSubscribed = true;

         observer.joinFederationExecution("updated-count-observer", "java-tck", federation);
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

         byte[] encodedFederate = encode(publisherFederate);
         await(publisher, callbackModel,
            () -> publisherRecorder.momObject(federateAttribute, encodedFederate) != null,
            "publisher federate MOM discovery");
         ObjectInstanceHandle momObject = publisherRecorder.momObject(federateAttribute,
            encodedFederate);
         JavaTckSupport.check(momObject != null
               && momClass.equals(publisher.getKnownObjectClassHandle(momObject)),
            "publisher MOM discovery returned the wrong object class");
         checkCounts(requestCounts(publisher, publisherRecorder, callbackModel, momObject,
               countAttributes, registeredAttribute, updatedAttribute, sentAttribute, reliable,
               encoder, "initial counters"), 0, 0, 0, "initial counters");

         ObjectInstanceHandle firstObject = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(firstObject != null, "first registration returned an invalid handle");
         awaitDiscoveries(observer, observerRecorder, callbackModel, firstObject);
         checkCounts(requestCounts(publisher, publisherRecorder, callbackModel, momObject,
               countAttributes, registeredAttribute, updatedAttribute, sentAttribute, reliable,
               encoder, "counters after first registration"), 1, 0, 0,
            "counters after first registration");
         sendUpdate(publisher, observer, observerRecorder, callbackModel, firstObject,
            publisherAttribute, observerAttribute, publisherFederate, (byte) 0x51, reliable,
            encoder, "first updated-count object update");
         checkCounts(requestCounts(publisher, publisherRecorder, callbackModel, momObject,
               countAttributes, registeredAttribute, updatedAttribute, sentAttribute, reliable,
               encoder, "counters after first update"), 1, 1, 1,
            "counters after first update");
         sendUpdate(publisher, observer, observerRecorder, callbackModel, firstObject,
            publisherAttribute, observerAttribute, publisherFederate, (byte) 0x52, reliable,
            encoder, "repeated updated-count object update");
         checkCounts(requestCounts(publisher, publisherRecorder, callbackModel, momObject,
               countAttributes, registeredAttribute, updatedAttribute, sentAttribute, reliable,
               encoder, "counters after repeated update"), 1, 1, 2,
            "counters after repeated update");

         ObjectInstanceHandle secondObject = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(secondObject != null && !secondObject.equals(firstObject),
            "second registration returned an invalid or duplicate handle");
         awaitDiscoveries(observer, observerRecorder, callbackModel, secondObject);
         checkCounts(requestCounts(publisher, publisherRecorder, callbackModel, momObject,
               countAttributes, registeredAttribute, updatedAttribute, sentAttribute, reliable,
               encoder, "counters after second registration"), 2, 1, 2,
            "counters after second registration");
         sendUpdate(publisher, observer, observerRecorder, callbackModel, secondObject,
            publisherAttribute, observerAttribute, publisherFederate, (byte) 0x53, reliable,
            encoder, "second updated-count object update");
         checkCounts(requestCounts(publisher, publisherRecorder, callbackModel, momObject,
               countAttributes, registeredAttribute, updatedAttribute, sentAttribute, reliable,
               encoder, "counters after second-object update"), 2, 2, 3,
            "counters after second-object update");

         InteractionClassHandle setTiming = publisher.getInteractionClassHandle(SET_TIMING);
         ParameterHandle timingFederate = publisher.getParameterHandle(setTiming, "HLAfederate");
         ParameterHandle reportPeriod = publisher.getParameterHandle(setTiming, "HLAreportPeriod");
         JavaTckSupport.check(SET_TIMING.equals(publisher.getInteractionClassName(setTiming))
               && "HLAfederate".equals(publisher.getParameterName(setTiming, timingFederate))
               && "HLAreportPeriod".equals(publisher.getParameterName(setTiming, reportPeriod)),
            "standard HLAsetTiming lookup did not round-trip");
         int periodicStart = publisherRecorder.reflections.size();
         sendTiming(publisher, setTiming, timingFederate, reportPeriod, publisherFederate, 1,
            encoder);
         await(publisher, callbackModel,
            () -> findAllCounts(publisherRecorder, periodicStart, momObject, countAttributes)
               != null,
            "periodic joined-federate MOM counters");
         CountValues periodic = decodeCounts(findAllCounts(publisherRecorder, periodicStart,
            momObject, countAttributes), registeredAttribute, updatedAttribute, sentAttribute,
            reliable, encoder, "periodic joined-federate MOM counters");
         checkCounts(periodic, 2, 2, 3, "periodic joined-federate MOM counters");
         sendTiming(publisher, setTiming, timingFederate, reportPeriod, publisherFederate, 0,
            encoder);

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

   private static CountValues requestCounts(RTIambassador rti, Recorder recorder,
         CallbackModel model, ObjectInstanceHandle object, AttributeHandleSet attributes,
         AttributeHandle registered, AttributeHandle updated, AttributeHandle sent,
         TransportationTypeHandle reliable, EncoderFactory encoder, String description)
         throws Exception {
      int first = recorder.reflections.size();
      rti.requestAttributeValueUpdate(object, attributes, new byte[0]);
      await(rti, model, () -> findAllCounts(recorder, first, object, attributes) != null,
         description);
      return decodeCounts(findAllCounts(recorder, first, object, attributes), registered,
         updated, sent, reliable, encoder, description);
   }

   private static Reflection findAllCounts(Recorder recorder, int first,
         ObjectInstanceHandle object, AttributeHandleSet attributes) {
      for (int i = Math.min(first, recorder.reflections.size());
            i < recorder.reflections.size(); ++i) {
         Reflection reflection = recorder.reflections.get(i);
         if (object.equals(reflection.object) && reflection.values.size() == attributes.size()
               && reflection.values.keySet().containsAll(attributes)) return reflection;
      }
      return null;
   }

   private static CountValues decodeCounts(Reflection reflection, AttributeHandle registered,
         AttributeHandle updated, AttributeHandle sent, TransportationTypeHandle reliable,
         EncoderFactory encoder, String description) throws Exception {
      JavaTckSupport.check(reflection != null && reflection.values.size() == 3
            && reflection.tag.length == 0 && reliable.equals(reflection.transportation)
            && reflection.producer == null
            && (reflection.regions == null || reflection.regions.isEmpty()),
         description + " returned non-standard MOM reflection metadata");
      return new CountValues(verifyCountValue(reflection, registered, encoder, description),
         verifyCountValue(reflection, updated, encoder, description),
         verifyCountValue(reflection, sent, encoder, description));
   }

   private static int verifyCountValue(Reflection reflection, AttributeHandle attribute,
         EncoderFactory encoder, String description) throws Exception {
      HLAinteger32BE value = encoder.createHLAinteger32BE();
      value.decode(reflection.values.get(attribute));
      JavaTckSupport.check(value.getValue() >= 0, description + " returned a negative count");
      return value.getValue();
   }

   private static void checkCounts(CountValues values, int registered, int updated, int sent,
         String description) {
      JavaTckSupport.check(values.registered == registered && values.updated == updated
            && values.sent == sent, description + " had unexpected registered/updated/sent values");
   }

   private static final class CountValues {
      private final int registered, updated, sent;
      private CountValues(int registered, int updated, int sent) {
         this.registered = registered;
         this.updated = updated;
         this.sent = sent;
      }
   }

   private static void awaitDiscoveries(RTIambassador observer, Recorder recorder,
         CallbackModel model, ObjectInstanceHandle... expected) throws Exception {
      await(observer, model, () -> {
         for (ObjectInstanceHandle object : expected)
            if (!recorder.discoveries.contains(object)) return false;
         return true;
      }, "observer discovery of application objects");
   }

   private static void sendUpdate(RTIambassador publisher, RTIambassador observer,
         Recorder recorder, CallbackModel model, ObjectInstanceHandle object,
         AttributeHandle publisherAttribute, AttributeHandle observerAttribute,
         FederateHandle publisherFederate, byte payload, TransportationTypeHandle reliable,
         EncoderFactory encoder, String description) throws Exception {
      int first = recorder.reflections.size();
      byte[] encoded = encoder.createHLAbyte(payload).toByteArray();
      AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(1);
      values.put(publisherAttribute, encoded);
      publisher.updateAttributeValues(object, values, new byte[0]);
      await(observer, model,
         () -> findUpdate(recorder, first, object, observerAttribute, encoded) != null,
         description);
      Reflection reflection = findUpdate(recorder, first, object, observerAttribute, encoded);
      JavaTckSupport.check(reflection != null
            && countUpdateReflections(recorder, first, object, observerAttribute, encoded) == 1
            && reflection.values.size() == 1 && reflection.tag.length == 0
            && reliable.equals(reflection.transportation)
            && publisherFederate.equals(reflection.producer)
            && (reflection.regions == null || reflection.regions.isEmpty()),
         description + " returned missing, duplicate, or non-standard reflection data");
   }

   private static Reflection findUpdate(Recorder recorder, int first, ObjectInstanceHandle object,
         AttributeHandle attribute, byte[] expected) {
      for (int i = Math.min(first, recorder.reflections.size());
            i < recorder.reflections.size(); ++i) {
         Reflection reflection = recorder.reflections.get(i);
         byte[] value = reflection.values.get(attribute);
         if (object.equals(reflection.object) && value != null && Arrays.equals(expected, value))
            return reflection;
      }
      return null;
   }

   private static int countUpdateReflections(Recorder recorder, int first,
         ObjectInstanceHandle object, AttributeHandle attribute, byte[] expected) {
      int count = 0;
      for (int i = Math.min(first, recorder.reflections.size());
            i < recorder.reflections.size(); ++i) {
         Reflection reflection = recorder.reflections.get(i);
         byte[] value = reflection.values.get(attribute);
         if (object.equals(reflection.object) && value != null && Arrays.equals(expected, value))
            ++count;
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
                  if ("toString".equals(name)) return "Java TCK object-instances-updated MOM callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name))
                     return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
