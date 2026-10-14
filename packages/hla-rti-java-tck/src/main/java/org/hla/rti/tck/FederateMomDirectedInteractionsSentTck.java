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

/** Verifies ordinary and directed send accounting in the standard federate MOM. */
final class FederateMomDirectedInteractionsSentTck {
   private static final String MOM_CLASS = "HLAobjectRoot.HLAmanager.HLAfederate";
   private static final String SET_TIMING =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming";
   private static final String TIMEOUT_MESSAGE = "timed out waiting for ";
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private FederateMomDirectedInteractionsSentTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String attributeName, String interactionName,
         String parameterName, CallbackModel callbackModel) throws Exception {
      RTIambassador sender = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder senderRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-federate-mom-directed-interactions-sent-" + UUID.randomUUID();
      boolean senderConnected = false, receiverConnected = false, created = false;
      boolean senderJoined = false, receiverJoined = false;
      boolean momSubscribed = false, senderTargetSubscribed = false;
      boolean targetPublished = false, ordinarySubscribed = false, directedSubscribed = false;
      boolean ordinaryPublished = false, directedPublished = false;
      ObjectClassHandle senderObjectClass = null, receiverObjectClass = null, momClass = null;
      AttributeHandleSet momAttributes = null, senderObjectAttributes = null;
      AttributeHandleSet receiverObjectAttributes = null;
      InteractionClassHandle senderInteraction = null, receiverInteraction = null;
      ObjectInstanceHandle targetObject = null;
      try {
         sender.connect(senderRecorder.proxy(), callbackModel);
         senderConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         sender.createFederationExecutionWithMIM(federation, new String[] {fom}, mim,
            timeImplementation);
         created = true;
         FederateHandle senderFederate = sender.joinFederationExecution(
            "directed-interactions-sent-sender", "java-tck", federation);
         senderJoined = true;

         momClass = sender.getObjectClassHandle(MOM_CLASS);
         AttributeHandle federateHandleAttribute = sender.getAttributeHandle(momClass,
            "HLAfederateHandle");
         AttributeHandle interactionsSentAttribute = sender.getAttributeHandle(momClass,
            "HLAinteractionsSent");
         AttributeHandle directedSentAttribute = sender.getAttributeHandle(momClass,
            "HLAdirectedInteractionsSent");
         TransportationTypeHandle reliable = sender.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(MOM_CLASS.equals(sender.getObjectClassName(momClass))
               && "HLAfederateHandle".equals(sender.getAttributeName(momClass,
                  federateHandleAttribute))
               && "HLAinteractionsSent".equals(sender.getAttributeName(momClass,
                  interactionsSentAttribute))
               && "HLAdirectedInteractionsSent".equals(sender.getAttributeName(momClass,
                  directedSentAttribute))
               && "HLAreliable".equals(sender.getTransportationTypeName(reliable)),
            "standard MOM directed-interactions-sent lookup did not round-trip");
         momAttributes = JavaTckSupport.attributeSet(sender, federateHandleAttribute,
            interactionsSentAttribute, directedSentAttribute);
         sender.subscribeObjectClassAttributes(momClass, momAttributes, "");
         momSubscribed = true;

         receiver.joinFederationExecution("directed-interactions-sent-receiver", "java-tck",
            federation);
         receiverJoined = true;
         senderObjectClass = sender.getObjectClassHandle(objectClassName);
         receiverObjectClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle senderObjectAttribute = sender.getAttributeHandle(senderObjectClass,
            attributeName);
         AttributeHandle receiverObjectAttribute = receiver.getAttributeHandle(receiverObjectClass,
            attributeName);
         senderInteraction = sender.getInteractionClassHandle(interactionName);
         receiverInteraction = receiver.getInteractionClassHandle(interactionName);
         ParameterHandle senderParameter = sender.getParameterHandle(senderInteraction,
            parameterName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(receiverInteraction,
            parameterName);
         JavaTckSupport.check(senderObjectClass.equals(receiverObjectClass)
               && senderObjectAttribute.equals(receiverObjectAttribute)
               && senderInteraction.equals(receiverInteraction)
               && senderParameter.equals(receiverParameter)
               && objectClassName.equals(sender.getObjectClassName(senderObjectClass))
               && objectClassName.equals(receiver.getObjectClassName(receiverObjectClass))
               && attributeName.equals(sender.getAttributeName(senderObjectClass,
                  senderObjectAttribute))
               && attributeName.equals(receiver.getAttributeName(receiverObjectClass,
                  receiverObjectAttribute))
               && interactionName.equals(sender.getInteractionClassName(senderInteraction))
               && interactionName.equals(receiver.getInteractionClassName(receiverInteraction))
               && parameterName.equals(sender.getParameterName(senderInteraction, senderParameter))
               && parameterName.equals(receiver.getParameterName(receiverInteraction,
                  receiverParameter)),
            "adapter-supplied object and interaction handles did not round-trip");
         InteractionClassHandle expectedInteraction = receiverInteraction;

         senderObjectAttributes = JavaTckSupport.attributeSet(sender, senderObjectAttribute);
         receiverObjectAttributes = JavaTckSupport.attributeSet(receiver, receiverObjectAttribute);
         receiver.publishObjectClassAttributes(receiverObjectClass, receiverObjectAttributes);
         targetPublished = true;
         receiver.subscribeInteractionClass(receiverInteraction);
         ordinarySubscribed = true;
         receiver.subscribeObjectClassDirectedInteractions(receiverObjectClass,
            JavaTckSupport.interactionSet(receiver, receiverInteraction));
         directedSubscribed = true;
         sender.subscribeObjectClassAttributes(senderObjectClass, senderObjectAttributes, "");
         senderTargetSubscribed = true;
         sender.publishInteractionClass(senderInteraction);
         ordinaryPublished = true;
         sender.publishObjectClassDirectedInteractions(senderObjectClass,
            JavaTckSupport.interactionSet(sender, senderInteraction));
         directedPublished = true;

         targetObject = receiver.registerObjectInstance(receiverObjectClass);
         ObjectInstanceHandle expectedTarget = targetObject;
         await(sender, callbackModel,
            () -> senderRecorder.discoveries.contains(expectedTarget),
            "sender discovery of the directed-interaction target");
         byte[] encodedSender = encode(senderFederate);
         await(sender, callbackModel,
            () -> senderRecorder.momObject(federateHandleAttribute, encodedSender) != null,
            "sender federate MOM object discovery");
         ObjectInstanceHandle senderMomObject = senderRecorder.momObject(
            federateHandleAttribute, encodedSender);
         JavaTckSupport.check(senderMomObject != null
               && momClass.equals(sender.getKnownObjectClassHandle(senderMomObject)),
            "sender MOM discovery returned the wrong object class");

         JavaTckSupport.check(requestCount(sender, senderRecorder, callbackModel, senderMomObject,
               interactionsSentAttribute, reliable, encoder, "initial interactions-sent") == 0
               && requestCount(sender, senderRecorder, callbackModel, senderMomObject,
                  directedSentAttribute, reliable, encoder,
                  "initial directed-interactions-sent") == 0,
            "MOM interaction-send counters were not initially zero");
         InteractionClassHandle setTiming = sender.getInteractionClassHandle(SET_TIMING);
         ParameterHandle timingFederate = sender.getParameterHandle(setTiming, "HLAfederate");
         ParameterHandle reportPeriod = sender.getParameterHandle(setTiming, "HLAreportPeriod");
         JavaTckSupport.check(SET_TIMING.equals(sender.getInteractionClassName(setTiming))
               && "HLAfederate".equals(sender.getParameterName(setTiming, timingFederate))
               && "HLAreportPeriod".equals(sender.getParameterName(setTiming, reportPeriod)),
            "standard HLAsetTiming lookup did not round-trip");
         sendTiming(sender, setTiming, timingFederate, reportPeriod, senderFederate, 1, encoder);
         Thread.sleep(20L);
         int totalBaseline = requestCount(sender, senderRecorder, callbackModel, senderMomObject,
            interactionsSentAttribute, reliable, encoder, "total baseline with periodic reports");
         int directedBaseline = requestCount(sender, senderRecorder, callbackModel, senderMomObject,
            directedSentAttribute, reliable, encoder, "directed baseline with periodic reports");
         JavaTckSupport.check(directedBaseline == 0,
            "periodic-report setup changed the directed-send baseline");

         byte[] ordinaryPayload = new byte[] {(byte) 0xA2};
         byte[] directPayload = new byte[] {(byte) 0xD3};
         byte[] periodicPayload = new byte[] {(byte) 0xD4};
         int ordinaryStart = receiverRecorder.ordinary.size();
         send(sender, senderInteraction, senderParameter, ordinaryPayload);
         await(receiver, callbackModel,
            () -> countOrdinary(receiverRecorder, ordinaryStart, expectedInteraction,
               receiverParameter, senderFederate, ordinaryPayload, reliable) >= 1,
            "ordinary interaction delivery");
         JavaTckSupport.check(countOrdinary(receiverRecorder, ordinaryStart, receiverInteraction,
               receiverParameter, senderFederate, ordinaryPayload, reliable) == 1
               && receiverRecorder.ordinary.size() == ordinaryStart + 1
               && receiverRecorder.directed.isEmpty(),
            "ordinary send did not stay on the ordinary callback route");
         JavaTckSupport.check(requestCount(sender, senderRecorder, callbackModel, senderMomObject,
               interactionsSentAttribute, reliable, encoder,
               "total after ordinary send") == totalBaseline + 1
               && requestCount(sender, senderRecorder, callbackModel, senderMomObject,
                  directedSentAttribute, reliable, encoder,
                  "directed after ordinary send") == directedBaseline,
            "directed-send counter included an ordinary send");

         int directStart = receiverRecorder.directed.size();
         for (int i = 0; i < 3; ++i)
            sendDirected(sender, senderInteraction, targetObject, senderParameter, directPayload);
         ObjectInstanceHandle target = targetObject;
         await(receiver, callbackModel,
            () -> countDirected(receiverRecorder, directStart, expectedInteraction, target,
               receiverParameter, senderFederate, directPayload, reliable) >= 3,
            "three direct directed-interaction deliveries");
         JavaTckSupport.check(countDirected(receiverRecorder, directStart, receiverInteraction,
               target, receiverParameter, senderFederate, directPayload, reliable) == 3
               && receiverRecorder.directed.size() == directStart + 3
               && countOrdinary(receiverRecorder, ordinaryStart, receiverInteraction,
                  receiverParameter, senderFederate, ordinaryPayload, reliable) == 1
               && receiverRecorder.ordinary.size() == ordinaryStart + 1,
            "directed sends returned duplicate callbacks or used the ordinary route");
         JavaTckSupport.check(requestCount(sender, senderRecorder, callbackModel, senderMomObject,
               interactionsSentAttribute, reliable, encoder,
               "total after direct directed sends") == totalBaseline + 4
               && requestCount(sender, senderRecorder, callbackModel, senderMomObject,
                  directedSentAttribute, reliable, encoder,
                  "direct count after direct directed sends") == directedBaseline + 3,
            "direct MOM counters did not separate ordinary and directed sends");

         int periodicTotalBaseline = requestCount(sender, senderRecorder, callbackModel,
            senderMomObject, interactionsSentAttribute, reliable, encoder,
            "total before periodic directed sends");
         int periodicDirectedBaseline = requestCount(sender, senderRecorder, callbackModel,
            senderMomObject, directedSentAttribute, reliable, encoder,
            "directed baseline before periodic sends");
         JavaTckSupport.check(periodicDirectedBaseline == directedBaseline + 3,
            "directed-send baseline changed before periodic sends");
         int periodicReflectionStart = senderRecorder.reflections.size();
         int periodicStart = receiverRecorder.directed.size();
         for (int i = 0; i < 2; ++i)
            sendDirected(sender, senderInteraction, targetObject, senderParameter, periodicPayload);
         await(receiver, callbackModel,
            () -> countDirected(receiverRecorder, periodicStart, expectedInteraction, target,
               receiverParameter, senderFederate, periodicPayload, reliable) >= 2,
            "two periodic-phase directed-interaction deliveries");
         JavaTckSupport.check(countDirected(receiverRecorder, periodicStart, receiverInteraction,
               target, receiverParameter, senderFederate, periodicPayload, reliable) == 2
               && receiverRecorder.directed.size() == periodicStart + 2
               && receiverRecorder.ordinary.size() == ordinaryStart + 1,
            "periodic directed sends returned duplicate or ordinary callbacks");
         int expectedTotal = periodicTotalBaseline + 2;
         int expectedDirected = periodicDirectedBaseline + 2;
         await(sender, callbackModel,
            () -> findCountAtLeast(senderRecorder, periodicReflectionStart, senderMomObject,
               interactionsSentAttribute, encoder, expectedTotal) != null,
            "periodic total interactions-sent value");
         await(sender, callbackModel,
            () -> findCountAtLeast(senderRecorder, periodicReflectionStart, senderMomObject,
               directedSentAttribute, encoder, expectedDirected) != null,
            "periodic directed-interactions-sent value");
         Reflection periodicTotal = findCountAtLeast(senderRecorder, periodicReflectionStart,
            senderMomObject, interactionsSentAttribute, encoder, expectedTotal);
         Reflection periodicDirected = findCountAtLeast(senderRecorder, periodicReflectionStart,
            senderMomObject, directedSentAttribute, encoder, expectedDirected);
         JavaTckSupport.check(periodicTotal != null
               && verifyCount(periodicTotal, interactionsSentAttribute, reliable, encoder,
                  "periodic interactions-sent") == expectedTotal
               && periodicDirected != null
               && verifyCount(periodicDirected, directedSentAttribute, reliable, encoder,
                  "periodic directed-interactions-sent") == expectedDirected,
            "periodic MOM counts did not match the accepted directed sends");
         JavaTckSupport.check(requestCount(sender, senderRecorder, callbackModel, senderMomObject,
               interactionsSentAttribute, reliable, encoder,
               "direct total after periodic sends") == expectedTotal
               && requestCount(sender, senderRecorder, callbackModel, senderMomObject,
                  directedSentAttribute, reliable, encoder,
                  "direct directed count after periodic sends") == expectedDirected,
            "direct MOM counters did not increase once per accepted directed send");
         sendTiming(sender, setTiming, timingFederate, reportPeriod, senderFederate, 0, encoder);
         JavaTckSupport.check(requestCount(sender, senderRecorder, callbackModel, senderMomObject,
               directedSentAttribute, reliable, encoder,
               "final directed-interactions-sent") == expectedDirected,
            "directed-interactions-sent changed after periodic reporting was disabled");

         receiver.unsubscribeObjectClassDirectedInteractions(receiverObjectClass);
         directedSubscribed = false;
         receiver.unsubscribeInteractionClass(receiverInteraction);
         ordinarySubscribed = false;
         sender.unsubscribeObjectClassAttributes(senderObjectClass, senderObjectAttributes);
         senderTargetSubscribed = false;
         sender.unsubscribeObjectClassAttributes(momClass, momAttributes);
         momSubscribed = false;
         sender.unpublishInteractionClass(senderInteraction);
         ordinaryPublished = false;
         sender.unpublishObjectClassDirectedInteractions(senderObjectClass,
            JavaTckSupport.interactionSet(sender, senderInteraction));
         directedPublished = false;
         receiver.unpublishObjectClassAttributes(receiverObjectClass, receiverObjectAttributes);
         targetPublished = false;
         receiver.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         receiverJoined = false;
         sender.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         senderJoined = false;
         sender.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (directedSubscribed && receiverJoined && receiverObjectClass != null
               && receiverInteraction != null) try {
            receiver.unsubscribeObjectClassDirectedInteractions(receiverObjectClass);
         } catch (Exception ignored) { }
         if (ordinarySubscribed && receiverJoined && receiverInteraction != null) try {
            receiver.unsubscribeInteractionClass(receiverInteraction);
         } catch (Exception ignored) { }
         if (senderTargetSubscribed && senderJoined && senderObjectClass != null
               && senderObjectAttributes != null) try {
            sender.unsubscribeObjectClassAttributes(senderObjectClass, senderObjectAttributes);
         } catch (Exception ignored) { }
         if (momSubscribed && senderJoined && momClass != null && momAttributes != null) try {
            sender.unsubscribeObjectClassAttributes(momClass, momAttributes);
         } catch (Exception ignored) { }
         if (ordinaryPublished && senderJoined && senderInteraction != null) try {
            sender.unpublishInteractionClass(senderInteraction);
         } catch (Exception ignored) { }
         if (directedPublished && senderJoined && senderObjectClass != null
               && senderInteraction != null) try {
            sender.unpublishObjectClassDirectedInteractions(senderObjectClass,
               JavaTckSupport.interactionSet(sender, senderInteraction));
         } catch (Exception ignored) { }
         if (targetPublished && receiverJoined && receiverObjectClass != null
               && receiverObjectAttributes != null) try {
            receiver.unpublishObjectClassAttributes(receiverObjectClass, receiverObjectAttributes);
         } catch (Exception ignored) { }
         if (receiverJoined) resign(receiver);
         if (senderJoined) resign(sender);
         if (created) try { sender.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (receiverConnected) disconnect(receiver);
         if (senderConnected) disconnect(sender);
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
               && reliable.equals(event.transportation)
               && event.sentOrder == OrderType.RECEIVE
               && event.receivedOrder == OrderType.RECEIVE) ++count;
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
      JavaTckSupport.check(condition.getAsBoolean(), TIMEOUT_MESSAGE + description);
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
      private final OrderType sentOrder, receivedOrder;
      private Ordinary(InteractionClassHandle interaction, ParameterHandleValueMap parameters,
            byte[] tag, TransportationTypeHandle transportation, FederateHandle producer,
            OrderType sentOrder, OrderType receivedOrder) {
         this.interaction = interaction;
         this.parameters = parameters.clone();
         this.tag = tag.clone();
         this.transportation = transportation;
         this.producer = producer;
         this.sentOrder = sentOrder;
         this.receivedOrder = receivedOrder;
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
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     (OrderType) arguments[6], (OrderType) arguments[7]));
               } else if ("receiveDirectedInteraction".equals(name) && arguments != null
                     && arguments.length == 6) {
                  directed.add(new Directed((InteractionClassHandle) arguments[0],
                     (ObjectInstanceHandle) arguments[1], (ParameterHandleValueMap) arguments[2],
                     (byte[]) arguments[3], (TransportationTypeHandle) arguments[4],
                     (FederateHandle) arguments[5]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK directed-send MOM callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name))
                     return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
