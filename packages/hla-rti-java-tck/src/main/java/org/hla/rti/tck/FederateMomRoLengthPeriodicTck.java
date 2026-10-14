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

/** Checks direct and periodic standard MOM reporting of queued receive-order interactions. */
final class FederateMomRoLengthPeriodicTck {
   private static final String MOM_CLASS = "HLAobjectRoot.HLAmanager.HLAfederate";
   private static final String FEDERATE_HANDLE = "HLAfederateHandle";
   private static final String RO_LENGTH = "HLAROlength";
   private static final String SET_TIMING =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming";
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private FederateMomRoLengthPeriodicTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String interactionName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador receiver = factory.getRtiAmbassador();
      RTIambassador publisher = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder receiverRecorder = new Recorder();
      Recorder publisherRecorder = new Recorder();
      String federation = "java-tck-federate-mom-ro-length-" + UUID.randomUUID();
      boolean receiverConnected = false, publisherConnected = false, created = false;
      boolean receiverJoined = false, publisherJoined = false, momSubscribed = false;
      boolean interactionSubscribed = false, interactionPublished = false;
      boolean callbacksDisabled = false;
      ObjectClassHandle momClass = null;
      AttributeHandleSet momAttributes = null;
      InteractionClassHandle receiverInteraction = null, publisherInteraction = null;
      try {
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         publisher.createFederationExecutionWithMIM(federation, new String[] {fom}, mim,
            timeImplementation);
         created = true;
         FederateHandle receiverFederate = receiver.joinFederationExecution(
            "ro-length-receiver", "java-tck", federation);
         receiverJoined = true;

         momClass = receiver.getObjectClassHandle(MOM_CLASS);
         AttributeHandle federateAttribute = receiver.getAttributeHandle(momClass, FEDERATE_HANDLE);
         AttributeHandle roLengthAttribute = receiver.getAttributeHandle(momClass, RO_LENGTH);
         TransportationTypeHandle reliable = receiver.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(MOM_CLASS.equals(receiver.getObjectClassName(momClass))
               && FEDERATE_HANDLE.equals(receiver.getAttributeName(momClass, federateAttribute))
               && RO_LENGTH.equals(receiver.getAttributeName(momClass, roLengthAttribute))
               && "HLAreliable".equals(receiver.getTransportationTypeName(reliable)),
            "standard federate MOM RO-length lookup did not round-trip");
         momAttributes = JavaTckSupport.attributeSet(receiver, federateAttribute,
            roLengthAttribute);
         receiver.subscribeObjectClassAttributes(momClass, momAttributes, "");
         momSubscribed = true;

         receiverInteraction = receiver.getInteractionClassHandle(interactionName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(
            receiverInteraction, parameterName);
         JavaTckSupport.check(interactionName.equals(receiver.getInteractionClassName(
               receiverInteraction))
               && parameterName.equals(receiver.getParameterName(receiverInteraction,
                  receiverParameter)),
            "adapter-supplied receive-order interaction lookup did not round-trip");
         InteractionClassHandle expectedInteraction = receiverInteraction;
         receiver.subscribeInteractionClass(receiverInteraction);
         interactionSubscribed = true;

         FederateHandle publisherFederate = publisher.joinFederationExecution(
            "ro-length-publisher", "java-tck", federation);
         publisherJoined = true;
         publisherInteraction = publisher.getInteractionClassHandle(interactionName);
         ParameterHandle publisherParameter = publisher.getParameterHandle(
            publisherInteraction, parameterName);
         JavaTckSupport.check(receiverInteraction.equals(publisherInteraction)
               && receiverParameter.equals(publisherParameter),
            "receive-order interaction handles differed between federates");
         publisher.publishInteractionClass(publisherInteraction);
         interactionPublished = true;

         byte[] encodedReceiver = encode(receiverFederate);
         byte[] encodedPublisher = encode(publisherFederate);
         await(receiver, callbackModel,
            () -> receiverRecorder.momObject(federateAttribute, encodedReceiver) != null
               && receiverRecorder.momObject(federateAttribute, encodedPublisher) != null,
            "joined federate MOM discovery");
         ObjectInstanceHandle receiverMomObject = receiverRecorder.momObject(
            federateAttribute, encodedReceiver);
         JavaTckSupport.check(receiverMomObject != null
               && momClass.equals(receiver.getKnownObjectClassHandle(receiverMomObject)),
            "receiver MOM discovery returned the wrong object class");

         requireEmptyQueue(receiver, receiverRecorder, callbackModel, receiverMomObject,
            roLengthAttribute, reliable, encoder, "initial receive-order queue");
         InteractionClassHandle setTiming = receiver.getInteractionClassHandle(SET_TIMING);
         ParameterHandle timingFederate = receiver.getParameterHandle(setTiming, "HLAfederate");
         ParameterHandle reportPeriod = receiver.getParameterHandle(setTiming, "HLAreportPeriod");
         JavaTckSupport.check(SET_TIMING.equals(receiver.getInteractionClassName(setTiming))
               && "HLAfederate".equals(receiver.getParameterName(setTiming, timingFederate))
               && "HLAreportPeriod".equals(receiver.getParameterName(setTiming, reportPeriod)),
            "standard HLAsetTiming lookup did not round-trip");

         byte[] payload = new byte[] {0x52, 0x4f};
         int directInteractionStart = receiverRecorder.interactions.size();
         if (callbackModel == CallbackModel.HLA_IMMEDIATE) {
            receiver.disableCallbacks();
            callbacksDisabled = true;
         }
         for (int i = 0; i < 3; ++i) {
            sendApplicationInteraction(publisher, publisherInteraction, publisherParameter, payload);
            Thread.sleep(20L);
         }
         Thread.sleep(100L);
         receiver.getObjectClassHandle(MOM_CLASS);
         int directReflectionStart = receiverRecorder.reflections.size();
         receiver.requestAttributeValueUpdate(receiverMomObject,
            JavaTckSupport.attributeSet(receiver, roLengthAttribute), new byte[0]);
         if (callbacksDisabled) {
            receiver.enableCallbacks();
            callbacksDisabled = false;
         }
         await(receiver, callbackModel,
            () -> countInteractions(receiverRecorder, directInteractionStart,
               expectedInteraction, receiverParameter, publisherFederate, payload) >= 3
               && findRoLength(receiverRecorder, directReflectionStart, receiverMomObject,
                  roLengthAttribute) != null,
            "direct positive RO-length and queued receive-order messages");
         JavaTckSupport.check(countInteractions(receiverRecorder, directInteractionStart,
               expectedInteraction, receiverParameter, publisherFederate, payload) == 3,
            "direct RO-length phase delivered an unexpected interaction count");
         verifyRoLength(findRoLength(receiverRecorder, directReflectionStart, receiverMomObject,
            roLengthAttribute), roLengthAttribute, reliable, encoder, -1, true,
            "direct positive RO-length report");
         requireEmptyQueue(receiver, receiverRecorder, callbackModel, receiverMomObject,
            roLengthAttribute, reliable, encoder, "direct RO-length after delivery");

         int periodicReflectionStart = receiverRecorder.reflections.size();
         int periodicInteractionStart = receiverRecorder.interactions.size();
         sendTiming(receiver, setTiming, timingFederate, reportPeriod,
            receiverFederate, 1, encoder);
         Thread.sleep(20L);
         if (callbackModel == CallbackModel.HLA_IMMEDIATE) {
            receiver.disableCallbacks();
            callbacksDisabled = true;
         }
         sendApplicationInteraction(publisher, publisherInteraction, publisherParameter, payload);
         Thread.sleep(2_200L);
         if (callbacksDisabled) {
            receiver.enableCallbacks();
            callbacksDisabled = false;
         }
         await(receiver, callbackModel,
            () -> countInteractions(receiverRecorder, periodicInteractionStart,
               expectedInteraction, receiverParameter, publisherFederate, payload) >= 1
               && findPositiveRoLength(receiverRecorder, periodicReflectionStart,
                  receiverMomObject, roLengthAttribute, encoder) != null,
            "periodic positive RO-length and queued receive-order message");
         verifyRoLength(findPositiveRoLength(receiverRecorder, periodicReflectionStart,
            receiverMomObject, roLengthAttribute, encoder), roLengthAttribute, reliable, encoder,
            -1, true, "periodic positive RO-length report");
         sendTiming(receiver, setTiming, timingFederate, reportPeriod,
            receiverFederate, 0, encoder);
         JavaTckSupport.check(countInteractions(receiverRecorder, periodicInteractionStart,
               expectedInteraction, receiverParameter, publisherFederate, payload) == 1,
            "periodic RO-length phase delivered an unexpected interaction count");
         drainEvokedCallbacks(receiver, callbackModel);
         Thread.sleep(50L);
         requireEmptyQueue(receiver, receiverRecorder, callbackModel, receiverMomObject,
            roLengthAttribute, reliable, encoder, "periodic RO-length after delivery");

         receiver.unsubscribeObjectClassAttributes(momClass, momAttributes);
         momSubscribed = false;
         receiver.unsubscribeInteractionClass(receiverInteraction);
         interactionSubscribed = false;
         publisher.unpublishInteractionClass(publisherInteraction);
         interactionPublished = false;
         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         publisher.resignFederationExecution(ResignAction.NO_ACTION);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (callbacksDisabled) try { receiver.enableCallbacks(); } catch (Exception ignored) { }
         if (momSubscribed && receiverJoined && momClass != null && momAttributes != null) try {
            receiver.unsubscribeObjectClassAttributes(momClass, momAttributes);
         } catch (Exception ignored) { }
         if (interactionSubscribed && receiverJoined && receiverInteraction != null) try {
            receiver.unsubscribeInteractionClass(receiverInteraction);
         } catch (Exception ignored) { }
         if (interactionPublished && publisherJoined && publisherInteraction != null) try {
            publisher.unpublishInteractionClass(publisherInteraction);
         } catch (Exception ignored) { }
         if (receiverJoined) resign(receiver);
         if (publisherJoined) resign(publisher);
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (receiverConnected) disconnect(receiver);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static void requireEmptyQueue(RTIambassador receiver, Recorder recorder,
         CallbackModel callbackModel, ObjectInstanceHandle momObject, AttributeHandle roLength,
         TransportationTypeHandle reliable, EncoderFactory encoder, String description)
         throws Exception {
      for (int attempt = 0; attempt < 5; ++attempt) {
         int value = requestRoLength(receiver, recorder, callbackModel, momObject, roLength,
            reliable, encoder, description + " request " + (attempt + 1));
         if (value == 0) return;
         drainEvokedCallbacks(receiver, callbackModel);
         Thread.sleep(20L);
      }
      throw new AssertionError(description + " did not reach an empty queue");
   }

   private static int requestRoLength(RTIambassador receiver, Recorder recorder,
         CallbackModel callbackModel, ObjectInstanceHandle momObject, AttributeHandle roLength,
         TransportationTypeHandle reliable, EncoderFactory encoder, String description)
         throws Exception {
      int first = recorder.reflections.size();
      receiver.requestAttributeValueUpdate(momObject,
         JavaTckSupport.attributeSet(receiver, roLength), new byte[0]);
      await(receiver, callbackModel,
         () -> findRoLength(recorder, first, momObject, roLength) != null, description);
      Reflection reflection = findRoLength(recorder, first, momObject, roLength);
      JavaTckSupport.check(reflection != null, description + " returned no MOM value");
      return verifyRoLength(reflection, roLength, reliable, encoder, -1, false, description);
   }

   private static int verifyRoLength(Reflection reflection, AttributeHandle roLength,
         TransportationTypeHandle reliable, EncoderFactory encoder, int expected,
         boolean requirePositive, String description) throws Exception {
      JavaTckSupport.check(reflection != null && reflection.values.size() == 1
            && reflection.values.containsKey(roLength) && reflection.tag.length == 0
            && reliable.equals(reflection.transportation) && reflection.producer == null
            && (reflection.regions == null || reflection.regions.isEmpty())
            && !reflection.timestampOrdered,
         description + " returned non-standard MOM reflection metadata");
      byte[] bytes = reflection.values.get(roLength);
      JavaTckSupport.check(bytes != null && bytes.length > 0,
         description + " omitted the encoded HLAROlength value");
      HLAinteger32BE encoded = encoder.createHLAinteger32BE();
      encoded.decode(bytes);
      int value = encoded.getValue();
      JavaTckSupport.check(value >= 0, description + " returned a negative queue length");
      if (expected >= 0) JavaTckSupport.check(value == expected,
         description + " returned an unexpected queue length");
      if (requirePositive) JavaTckSupport.check(value > 0,
         description + " did not report a queued receive-order message");
      return value;
   }

   private static Reflection findRoLength(Recorder recorder, int first,
         ObjectInstanceHandle object, AttributeHandle attribute) {
      List<Reflection> reflections = recorder.reflections;
      for (int i = Math.min(first, reflections.size()); i < reflections.size(); ++i) {
         Reflection reflection = reflections.get(i);
         if (object.equals(reflection.object) && reflection.values.containsKey(attribute))
            return reflection;
      }
      return null;
   }

   private static Reflection findPositiveRoLength(Recorder recorder, int first,
         ObjectInstanceHandle object, AttributeHandle attribute, EncoderFactory encoder) {
      List<Reflection> reflections = recorder.reflections;
      for (int i = Math.min(first, reflections.size()); i < reflections.size(); ++i) {
         Reflection reflection = reflections.get(i);
         if (!object.equals(reflection.object) || !reflection.values.containsKey(attribute))
            continue;
         byte[] bytes = reflection.values.get(attribute);
         if (bytes == null || bytes.length == 0) continue;
         HLAinteger32BE value = encoder.createHLAinteger32BE();
         try { value.decode(bytes); }
         catch (Exception error) {
            throw new IllegalStateException("invalid HLAROlength encoding", error);
         }
         if (value.getValue() > 0) return reflection;
      }
      return null;
   }

   private static int countInteractions(Recorder recorder, int first,
         InteractionClassHandle interaction, ParameterHandle parameter, FederateHandle producer,
         byte[] payload) {
      int count = 0;
      for (int i = Math.min(first, recorder.interactions.size());
            i < recorder.interactions.size(); ++i) {
         Interaction event = recorder.interactions.get(i);
         byte[] value = event.parameters.get(parameter);
         if (interaction.equals(event.interaction) && producer.equals(event.producer)
               && value != null && Arrays.equals(payload, value) && event.tag.length == 0
               && (event.regions == null || event.regions.isEmpty())
               && event.sentOrder == OrderType.RECEIVE
               && event.receivedOrder == OrderType.RECEIVE) ++count;
      }
      return count;
   }

   private static void sendApplicationInteraction(RTIambassador publisher,
         InteractionClassHandle interaction, ParameterHandle parameter, byte[] payload)
         throws Exception {
      ParameterHandleValueMap values = publisher.getParameterHandleValueMapFactory().create(1);
      values.put(parameter, payload.clone());
      publisher.sendInteraction(interaction, values, new byte[0]);
   }

   private static void sendTiming(RTIambassador receiver, InteractionClassHandle interaction,
         ParameterHandle federateParameter, ParameterHandle periodParameter, FederateHandle target,
         int seconds, EncoderFactory encoder) throws Exception {
      ParameterHandleValueMap values = receiver.getParameterHandleValueMapFactory().create(2);
      values.put(federateParameter, encode(target));
      values.put(periodParameter, encoder.createHLAinteger32BE(seconds).toByteArray());
      receiver.sendInteraction(interaction, values, new byte[0]);
   }

   private static byte[] encode(FederateHandle handle) throws Exception {
      byte[] bytes = new byte[handle.encodedLength()];
      handle.encode(bytes, 0);
      return bytes;
   }

   private static void drainEvokedCallbacks(RTIambassador receiver, CallbackModel model)
         throws Exception {
      if (model != CallbackModel.HLA_EVOKED) return;
      for (int pass = 0; pass < 512; ++pass) {
         if (!receiver.evokeCallback(0.0)) return;
      }
      throw new AssertionError("receive-order callback queue did not drain");
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         if (model == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.0);
         else Thread.yield();
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static final class Interaction {
      private final InteractionClassHandle interaction;
      private final ParameterHandleValueMap parameters;
      private final byte[] tag;
      private final FederateHandle producer;
      private final RegionHandleSet regions;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;
      private Interaction(InteractionClassHandle interaction, ParameterHandleValueMap parameters,
            byte[] tag, FederateHandle producer, RegionHandleSet regions, OrderType sentOrder,
            OrderType receivedOrder) {
         this.interaction = interaction;
         this.parameters = parameters.clone();
         this.tag = tag.clone();
         this.producer = producer;
         this.regions = regions == null ? null : (RegionHandleSet) regions.clone();
         this.sentOrder = sentOrder;
         this.receivedOrder = receivedOrder;
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
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();

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
               if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 6) {
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     (RegionHandleSet) arguments[5], false));
               } else if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 8) {
                  interactions.add(new Interaction((InteractionClassHandle) arguments[0],
                     (ParameterHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (FederateHandle) arguments[4], (RegionHandleSet) arguments[5],
                     (OrderType) arguments[6], (OrderType) arguments[7]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK RO-length MOM callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
