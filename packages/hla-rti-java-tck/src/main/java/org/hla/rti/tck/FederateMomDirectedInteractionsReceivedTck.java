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
import hla.rti1516_2025.OrderType;
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

/** Verifies ordinary and directed delivery accounting in the standard federate MOM. */
final class FederateMomDirectedInteractionsReceivedTck {
   private static final String MOM_CLASS = "HLAobjectRoot.HLAmanager.HLAfederate";
   private static final String SET_TIMING =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming";
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private FederateMomDirectedInteractionsReceivedTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String attributeName, String interactionName,
         String parameterName, CallbackModel callbackModel) throws Exception {
      RTIambassador receiver = factory.getRtiAmbassador();
      RTIambassador publisher = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder receiverRecorder = new Recorder();
      Recorder publisherRecorder = new Recorder();
      String federation = "java-tck-federate-mom-directed-interactions-received-" + UUID.randomUUID();
      boolean receiverConnected = false, publisherConnected = false, created = false;
      boolean receiverJoined = false, publisherJoined = false, momSubscribed = false;
      boolean receiverDirectedSubscribed = false, receiverInteractionSubscribed = false;
      boolean publisherObjectSubscribed = false, targetPublished = false;
      boolean publisherInteractionPublished = false, publisherDirectedPublished = false;
      ObjectClassHandle receiverObjectClass = null, publisherObjectClass = null, momClass = null;
      AttributeHandleSet momAttributes = null, receiverObjectAttributes = null;
      InteractionClassHandle receiverInteraction = null, publisherInteraction = null;
      ObjectInstanceHandle targetObject = null;
      try {
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         publisher.createFederationExecutionWithMIM(federation, new String[] {fom}, mim,
            timeImplementation);
         created = true;
         FederateHandle receiverFederate = receiver.joinFederationExecution(
            "directed-interactions-received-receiver", "java-tck", federation);
         receiverJoined = true;

         momClass = receiver.getObjectClassHandle(MOM_CLASS);
         AttributeHandle federateHandleAttribute = receiver.getAttributeHandle(momClass,
            "HLAfederateHandle");
         AttributeHandle interactionsReceivedAttribute = receiver.getAttributeHandle(momClass,
            "HLAinteractionsReceived");
         AttributeHandle directedReceivedAttribute = receiver.getAttributeHandle(momClass,
            "HLAdirectedInteractionsReceived");
         TransportationTypeHandle reliable = receiver.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(MOM_CLASS.equals(receiver.getObjectClassName(momClass))
               && "HLAfederateHandle".equals(receiver.getAttributeName(momClass,
                  federateHandleAttribute))
               && "HLAinteractionsReceived".equals(receiver.getAttributeName(momClass,
                  interactionsReceivedAttribute))
               && "HLAdirectedInteractionsReceived".equals(receiver.getAttributeName(momClass,
                  directedReceivedAttribute))
               && "HLAreliable".equals(receiver.getTransportationTypeName(reliable)),
            "standard MOM directed-interactions-received lookup did not round-trip");
         momAttributes = JavaTckSupport.attributeSet(receiver, federateHandleAttribute,
            interactionsReceivedAttribute, directedReceivedAttribute);
         receiver.subscribeObjectClassAttributes(momClass, momAttributes, "");
         momSubscribed = true;

         receiverObjectClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle receiverObjectAttribute = receiver.getAttributeHandle(receiverObjectClass,
            attributeName);
         receiverInteraction = receiver.getInteractionClassHandle(interactionName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(receiverInteraction,
            parameterName);
         JavaTckSupport.check(objectClassName.equals(receiver.getObjectClassName(receiverObjectClass))
               && attributeName.equals(receiver.getAttributeName(receiverObjectClass,
                  receiverObjectAttribute))
               && interactionName.equals(receiver.getInteractionClassName(receiverInteraction))
               && parameterName.equals(receiver.getParameterName(receiverInteraction,
                  receiverParameter)),
            "adapter-supplied receiver object and interaction handles did not round-trip");
         receiverObjectAttributes = JavaTckSupport.attributeSet(receiver, receiverObjectAttribute);
         receiver.publishObjectClassAttributes(receiverObjectClass, receiverObjectAttributes);
         targetPublished = true;
         receiver.subscribeInteractionClass(receiverInteraction);
         receiverInteractionSubscribed = true;
         receiver.subscribeObjectClassDirectedInteractions(receiverObjectClass,
            JavaTckSupport.interactionSet(receiver, receiverInteraction));
         receiverDirectedSubscribed = true;

         FederateHandle publisherFederate = publisher.joinFederationExecution(
            "directed-interactions-received-publisher", "java-tck", federation);
         publisherJoined = true;
         publisherObjectClass = publisher.getObjectClassHandle(objectClassName);
         AttributeHandle publisherObjectAttribute = publisher.getAttributeHandle(
            publisherObjectClass, attributeName);
         publisherInteraction = publisher.getInteractionClassHandle(interactionName);
         ParameterHandle publisherParameter = publisher.getParameterHandle(publisherInteraction,
            parameterName);
         JavaTckSupport.check(receiverObjectClass.equals(publisherObjectClass)
               && receiverObjectAttribute.equals(publisherObjectAttribute)
               && receiverInteraction.equals(publisherInteraction)
               && receiverParameter.equals(publisherParameter)
               && objectClassName.equals(publisher.getObjectClassName(publisherObjectClass))
               && attributeName.equals(publisher.getAttributeName(publisherObjectClass,
                  publisherObjectAttribute))
               && interactionName.equals(publisher.getInteractionClassName(publisherInteraction))
               && parameterName.equals(publisher.getParameterName(publisherInteraction,
                  publisherParameter)),
            "application handles differed between joined federates");
         publisher.subscribeObjectClassAttributes(publisherObjectClass,
            JavaTckSupport.attributeSet(publisher, publisherObjectAttribute), "");
         publisherObjectSubscribed = true;
         publisher.publishInteractionClass(publisherInteraction);
         publisherInteractionPublished = true;
         publisher.publishObjectClassDirectedInteractions(publisherObjectClass,
            JavaTckSupport.interactionSet(publisher, publisherInteraction));
         publisherDirectedPublished = true;

         targetObject = receiver.registerObjectInstance(receiverObjectClass);
         ObjectInstanceHandle expectedTarget = targetObject;
         await(publisher, callbackModel,
            () -> publisherRecorder.discoveries.contains(expectedTarget),
            "publisher discovery of the directed-interaction target");
         byte[] encodedReceiver = encode(receiverFederate);
         await(receiver, callbackModel,
            () -> receiverRecorder.momObject(federateHandleAttribute, encodedReceiver) != null,
            "receiver federate MOM object discovery");
         ObjectInstanceHandle receiverMomObject = receiverRecorder.momObject(
            federateHandleAttribute, encodedReceiver);
         JavaTckSupport.check(receiverMomObject != null
               && momClass.equals(receiver.getKnownObjectClassHandle(receiverMomObject)),
            "receiver MOM discovery returned the wrong object class");

         JavaTckSupport.check(requestCount(receiver, receiverRecorder, callbackModel,
               receiverMomObject, interactionsReceivedAttribute, reliable, encoder,
               "initial interactions-received") == 0
               && requestCount(receiver, receiverRecorder, callbackModel, receiverMomObject,
                  directedReceivedAttribute, reliable, encoder,
                  "initial directed-interactions-received") == 0,
            "MOM interaction-receive counters were not initially zero");

         byte[] ordinaryPayload = new byte[] {(byte) 0xA1};
         byte[] directPayload = new byte[] {(byte) 0xD1};
         byte[] periodicPayload = new byte[] {(byte) 0xD2};
         int ordinaryStart = receiverRecorder.ordinary.size();
         send(publisher, publisherInteraction, publisherParameter, ordinaryPayload);
         InteractionClassHandle expectedInteraction = receiverInteraction;
         await(receiver, callbackModel,
            () -> countOrdinary(receiverRecorder, ordinaryStart, expectedInteraction,
               receiverParameter, publisherFederate, ordinaryPayload, reliable) >= 1,
            "ordinary interaction delivery before directed delivery");
         JavaTckSupport.check(countOrdinary(receiverRecorder, ordinaryStart, receiverInteraction,
               receiverParameter, publisherFederate, ordinaryPayload, reliable) == 1
               && receiverRecorder.ordinary.size() == ordinaryStart + 1
               && receiverRecorder.directed.isEmpty(),
            "ordinary interaction did not stay on the ordinary callback route");
         JavaTckSupport.check(requestCount(receiver, receiverRecorder, callbackModel,
               receiverMomObject, interactionsReceivedAttribute, reliable, encoder,
               "total after ordinary delivery") == 1
               && requestCount(receiver, receiverRecorder, callbackModel, receiverMomObject,
                  directedReceivedAttribute, reliable, encoder,
                  "directed count after ordinary delivery") == 0,
            "directed-interactions-received included an ordinary callback");

         int directStart = receiverRecorder.directed.size();
         ObjectInstanceHandle expectedObject = targetObject;
         for (int i = 0; i < 3; ++i)
            sendDirected(publisher, publisherInteraction, targetObject, publisherParameter,
               directPayload);
         await(receiver, callbackModel,
            () -> countDirected(receiverRecorder, directStart, expectedInteraction,
               expectedObject, receiverParameter, publisherFederate, directPayload, reliable) >= 3,
            "three direct directed-interaction callbacks");
         JavaTckSupport.check(countDirected(receiverRecorder, directStart, receiverInteraction,
               targetObject, receiverParameter, publisherFederate, directPayload, reliable) == 3
               && receiverRecorder.directed.size() == directStart + 3
               && countOrdinary(receiverRecorder, ordinaryStart, receiverInteraction,
                  receiverParameter, publisherFederate, ordinaryPayload, reliable) == 1
               && receiverRecorder.ordinary.size() == ordinaryStart + 1,
            "directed sends returned duplicate callbacks or used the ordinary route");
         JavaTckSupport.check(requestCount(receiver, receiverRecorder, callbackModel,
               receiverMomObject, interactionsReceivedAttribute, reliable, encoder,
               "total after direct directed delivery") == 4
               && requestCount(receiver, receiverRecorder, callbackModel, receiverMomObject,
                  directedReceivedAttribute, reliable, encoder,
                  "direct count after direct directed delivery") == 3,
            "direct MOM counters did not separate ordinary and directed callbacks");

         int periodicStart = receiverRecorder.directed.size();
         int periodicReflectionStart = receiverRecorder.reflections.size();
         InteractionClassHandle setTiming = receiver.getInteractionClassHandle(SET_TIMING);
         ParameterHandle timingFederate = receiver.getParameterHandle(setTiming, "HLAfederate");
         ParameterHandle reportPeriod = receiver.getParameterHandle(setTiming, "HLAreportPeriod");
         JavaTckSupport.check(SET_TIMING.equals(receiver.getInteractionClassName(setTiming))
               && "HLAfederate".equals(receiver.getParameterName(setTiming, timingFederate))
               && "HLAreportPeriod".equals(receiver.getParameterName(setTiming, reportPeriod)),
            "standard HLAsetTiming lookup did not round-trip");
         sendTiming(receiver, setTiming, timingFederate, reportPeriod, receiverFederate, 1, encoder);
         Thread.sleep(20L);
         for (int i = 0; i < 2; ++i)
            sendDirected(publisher, publisherInteraction, targetObject, publisherParameter,
               periodicPayload);
         await(receiver, callbackModel,
            () -> countDirected(receiverRecorder, periodicStart, expectedInteraction,
               expectedObject, receiverParameter, publisherFederate, periodicPayload, reliable) >= 2,
            "two periodic-phase directed-interaction callbacks");
         JavaTckSupport.check(countDirected(receiverRecorder, periodicStart, receiverInteraction,
               targetObject, receiverParameter, publisherFederate, periodicPayload, reliable) == 2
               && receiverRecorder.directed.size() == periodicStart + 2
               && receiverRecorder.ordinary.size() == ordinaryStart + 1,
            "periodic directed sends returned duplicate or ordinary callbacks");
         await(receiver, callbackModel,
            () -> findCountAtLeast(receiverRecorder, periodicReflectionStart, receiverMomObject,
               directedReceivedAttribute, encoder, 5) != null,
            "periodic directed-interactions-received value of five");
         await(receiver, callbackModel,
            () -> findCountAtLeast(receiverRecorder, periodicReflectionStart, receiverMomObject,
               interactionsReceivedAttribute, encoder, 6) != null,
            "periodic interactions-received value of six");
         Reflection periodicDirected = findCountAtLeast(receiverRecorder, periodicReflectionStart,
            receiverMomObject, directedReceivedAttribute, encoder, 5);
         Reflection periodicTotal = findCountAtLeast(receiverRecorder, periodicReflectionStart,
            receiverMomObject, interactionsReceivedAttribute, encoder, 6);
         JavaTckSupport.check(periodicDirected != null
               && verifyCount(periodicDirected, directedReceivedAttribute, reliable, encoder,
                  "periodic directed-interactions-received") == 5
               && periodicTotal != null
               && verifyCount(periodicTotal, interactionsReceivedAttribute, reliable, encoder,
                  "periodic interactions-received") == 6,
            "periodic MOM receive counts did not match delivered callbacks");
         sendTiming(receiver, setTiming, timingFederate, reportPeriod, receiverFederate, 0, encoder);
         JavaTckSupport.check(requestCount(receiver, receiverRecorder, callbackModel,
               receiverMomObject, interactionsReceivedAttribute, reliable, encoder,
               "final interactions-received") == 6
               && requestCount(receiver, receiverRecorder, callbackModel, receiverMomObject,
                  directedReceivedAttribute, reliable, encoder,
                  "final directed-interactions-received") == 5,
            "MOM receive counters changed after periodic reporting was disabled");

         receiver.unsubscribeObjectClassDirectedInteractions(receiverObjectClass);
         receiverDirectedSubscribed = false;
         receiver.unsubscribeInteractionClass(receiverInteraction);
         receiverInteractionSubscribed = false;
         receiver.unsubscribeObjectClassAttributes(momClass, momAttributes);
         momSubscribed = false;
         publisher.unsubscribeObjectClassAttributes(publisherObjectClass,
            JavaTckSupport.attributeSet(publisher, publisher.getAttributeHandle(publisherObjectClass,
               attributeName)));
         publisherObjectSubscribed = false;
         publisher.unpublishInteractionClass(publisherInteraction);
         publisherInteractionPublished = false;
         publisher.unpublishObjectClassDirectedInteractions(publisherObjectClass,
            JavaTckSupport.interactionSet(publisher, publisherInteraction));
         publisherDirectedPublished = false;
         receiver.unpublishObjectClassAttributes(receiverObjectClass, receiverObjectAttributes);
         targetPublished = false;
         receiver.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         receiverJoined = false;
         publisher.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (receiverDirectedSubscribed && receiverJoined && receiverObjectClass != null) try {
            receiver.unsubscribeObjectClassDirectedInteractions(receiverObjectClass);
         } catch (Exception ignored) { }
         if (receiverInteractionSubscribed && receiverJoined && receiverInteraction != null) try {
            receiver.unsubscribeInteractionClass(receiverInteraction);
         } catch (Exception ignored) { }
         if (momSubscribed && receiverJoined && momClass != null && momAttributes != null) try {
            receiver.unsubscribeObjectClassAttributes(momClass, momAttributes);
         } catch (Exception ignored) { }
         if (publisherObjectSubscribed && publisherJoined && publisherObjectClass != null) try {
            publisher.unsubscribeObjectClassAttributes(publisherObjectClass,
               JavaTckSupport.attributeSet(publisher,
                  publisher.getAttributeHandle(publisherObjectClass, attributeName)));
         } catch (Exception ignored) { }
         if (publisherInteractionPublished && publisherJoined && publisherInteraction != null) try {
            publisher.unpublishInteractionClass(publisherInteraction);
         } catch (Exception ignored) { }
         if (publisherDirectedPublished && publisherJoined && publisherObjectClass != null
               && publisherInteraction != null) try {
            publisher.unpublishObjectClassDirectedInteractions(publisherObjectClass,
               JavaTckSupport.interactionSet(publisher, publisherInteraction));
         } catch (Exception ignored) { }
         if (targetPublished && receiverJoined && receiverObjectClass != null
               && receiverObjectAttributes != null) try {
            receiver.unpublishObjectClassAttributes(receiverObjectClass, receiverObjectAttributes);
         } catch (Exception ignored) { }
         if (receiverJoined) resign(receiver);
         if (publisherJoined) resign(publisher);
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (publisherConnected) disconnect(publisher);
         if (receiverConnected) disconnect(receiver);
      }
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
            && (reflection.regions == null || reflection.regions.isEmpty())
            && !reflection.timestampOrdered,
         description + " returned non-standard MOM reflection metadata");
      HLAinteger32BE value = encoder.createHLAinteger32BE();
      value.decode(reflection.values.get(attribute));
      JavaTckSupport.check(value.getValue() >= 0,
         description + " returned a negative counter value");
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

   private static int countOrdinary(Recorder recorder, int first,
         InteractionClassHandle expectedClass, ParameterHandle parameter, FederateHandle producer,
         byte[] payload, TransportationTypeHandle reliable) {
      int count = 0;
      for (int i = Math.min(first, recorder.ordinary.size()); i < recorder.ordinary.size(); ++i) {
         Ordinary event = recorder.ordinary.get(i);
         byte[] value = event.parameters.get(parameter);
         if (expectedClass.equals(event.interaction) && producer.equals(event.producer)
               && value != null && Arrays.equals(payload, value) && event.tag.length == 0
               && reliable.equals(event.transportation)) ++count;
      }
      return count;
   }

   private static int countDirected(Recorder recorder, int first,
         InteractionClassHandle expectedClass, ObjectInstanceHandle target,
         ParameterHandle parameter, FederateHandle producer, byte[] payload,
         TransportationTypeHandle reliable) {
      int count = 0;
      for (int i = Math.min(first, recorder.directed.size()); i < recorder.directed.size(); ++i) {
         Directed event = recorder.directed.get(i);
         byte[] value = event.parameters.get(parameter);
         if (expectedClass.equals(event.interaction) && target.equals(event.target)
               && producer.equals(event.producer) && value != null
               && Arrays.equals(payload, value) && event.tag.length == 0
               && reliable.equals(event.transportation)) ++count;
      }
      return count;
   }

   private static void send(RTIambassador rti, InteractionClassHandle interaction,
         ParameterHandle parameter, byte[] payload) throws Exception {
      ParameterHandleValueMap values = rti.getParameterHandleValueMapFactory().create(1);
      values.put(parameter, payload.clone());
      rti.sendInteraction(interaction, values, new byte[0]);
   }

   private static void sendDirected(RTIambassador rti, InteractionClassHandle interaction,
         ObjectInstanceHandle target, ParameterHandle parameter, byte[] payload) throws Exception {
      ParameterHandleValueMap values = rti.getParameterHandleValueMapFactory().create(1);
      values.put(parameter, payload.clone());
      rti.sendDirectedInteraction(interaction, target, values, new byte[0]);
   }

   private static void sendTiming(RTIambassador rti, InteractionClassHandle interaction,
         ParameterHandle federateParameter, ParameterHandle periodParameter,
         FederateHandle target, int period, EncoderFactory encoder) throws Exception {
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

   private static final class Ordinary {
      private final InteractionClassHandle interaction;
      private final ParameterHandleValueMap parameters;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private Ordinary(InteractionClassHandle interaction, ParameterHandleValueMap parameters,
            byte[] tag, TransportationTypeHandle transportation, FederateHandle producer) {
         this.interaction = interaction;
         this.parameters = parameters.clone();
         this.tag = tag.clone();
         this.transportation = transportation;
         this.producer = producer;
      }
   }

   private static final class Directed {
      private final InteractionClassHandle interaction;
      private final ObjectInstanceHandle target;
      private final ParameterHandleValueMap parameters;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private Directed(InteractionClassHandle interaction, ObjectInstanceHandle target,
            ParameterHandleValueMap parameters, byte[] tag,
            TransportationTypeHandle transportation, FederateHandle producer) {
         this.interaction = interaction;
         this.target = target;
         this.parameters = parameters.clone();
         this.tag = tag.clone();
         this.transportation = transportation;
         this.producer = producer;
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
      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap values, byte[] tag,
            TransportationTypeHandle transportation, FederateHandle producer,
            RegionHandleSet regions, boolean timestampOrdered) {
         this.object = object;
         this.values = values.clone();
         this.tag = tag.clone();
         this.transportation = transportation;
         this.producer = producer;
         this.regions = regions == null ? null : (RegionHandleSet) regions.clone();
         this.timestampOrdered = timestampOrdered;
      }
   }

   private static final class Recorder {
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<Ordinary> ordinary = new CopyOnWriteArrayList<>();
      private final List<Directed> directed = new CopyOnWriteArrayList<>();
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();

      private ObjectInstanceHandle momObject(AttributeHandle attribute, byte[] expected) {
         for (Reflection reflection : reflections) {
            byte[] actual = reflection.values.get(attribute);
            if (actual != null && Arrays.equals(expected, actual)) return reflection.object;
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
                     (RegionHandleSet) arguments[5], false));
               } else if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 8) {
                  ordinary.add(new Ordinary((InteractionClassHandle) arguments[0],
                     (ParameterHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4]));
               } else if ("receiveDirectedInteraction".equals(name) && arguments != null
                     && arguments.length == 6) {
                  directed.add(new Directed((InteractionClassHandle) arguments[0],
                     (ObjectInstanceHandle) arguments[1], (ParameterHandleValueMap) arguments[2],
                     (byte[]) arguments[3], (TransportationTypeHandle) arguments[4],
                     (FederateHandle) arguments[5]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK directed-receive MOM callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name))
                     return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
