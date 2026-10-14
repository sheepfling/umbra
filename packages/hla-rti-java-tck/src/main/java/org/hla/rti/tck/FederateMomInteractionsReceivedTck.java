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

/** Verifies direct and periodic standard MOM HLAinteractionsReceived counts. */
final class FederateMomInteractionsReceivedTck {
   private static final String MOM_CLASS = "HLAobjectRoot.HLAmanager.HLAfederate";
   private static final String FEDERATE_HANDLE = "HLAfederateHandle";
   private static final String INTERACTIONS_RECEIVED = "HLAinteractionsReceived";
   private static final String SET_TIMING =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming";
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private FederateMomInteractionsReceivedTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String interactionName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador receiver = factory.getRtiAmbassador();
      RTIambassador publisher = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-federate-mom-interactions-received-" + UUID.randomUUID();
      boolean receiverConnected = false, publisherConnected = false, created = false;
      boolean receiverJoined = false, publisherJoined = false, momSubscribed = false;
      boolean interactionSubscribed = false, interactionPublished = false;
      ObjectClassHandle momClass = null;
      AttributeHandleSet momAttributes = null;
      InteractionClassHandle receiverInteraction = null, publisherInteraction = null;
      try {
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.connect(noOpCallbacks(), callbackModel);
         publisherConnected = true;
         publisher.createFederationExecutionWithMIM(federation, new String[] {fom}, mim,
            timeImplementation);
         created = true;
         FederateHandle receiverFederate = receiver.joinFederationExecution(
            "interactions-received-receiver", "java-tck", federation);
         receiverJoined = true;

         momClass = receiver.getObjectClassHandle(MOM_CLASS);
         AttributeHandle federateAttribute = receiver.getAttributeHandle(momClass, FEDERATE_HANDLE);
         AttributeHandle countAttribute = receiver.getAttributeHandle(momClass,
            INTERACTIONS_RECEIVED);
         TransportationTypeHandle reliable = receiver.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(MOM_CLASS.equals(receiver.getObjectClassName(momClass))
               && FEDERATE_HANDLE.equals(receiver.getAttributeName(momClass, federateAttribute))
               && INTERACTIONS_RECEIVED.equals(receiver.getAttributeName(momClass, countAttribute))
               && "HLAreliable".equals(receiver.getTransportationTypeName(reliable)),
            "standard federate MOM interactions-received lookup did not round-trip");
         momAttributes = JavaTckSupport.attributeSet(receiver, federateAttribute, countAttribute);
         receiver.subscribeObjectClassAttributes(momClass, momAttributes, "");
         momSubscribed = true;

         receiverInteraction = receiver.getInteractionClassHandle(interactionName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(
            receiverInteraction, parameterName);
         JavaTckSupport.check(interactionName.equals(receiver.getInteractionClassName(
               receiverInteraction))
               && parameterName.equals(receiver.getParameterName(receiverInteraction,
                  receiverParameter)),
            "adapter-supplied application interaction lookup did not round-trip");
         InteractionClassHandle expectedInteraction = receiverInteraction;
         receiver.subscribeInteractionClass(receiverInteraction);
         interactionSubscribed = true;

         FederateHandle publisherFederate = publisher.joinFederationExecution(
            "interactions-received-publisher", "java-tck", federation);
         publisherJoined = true;
         publisherInteraction = publisher.getInteractionClassHandle(interactionName);
         ParameterHandle publisherParameter = publisher.getParameterHandle(
            publisherInteraction, parameterName);
         JavaTckSupport.check(receiverInteraction.equals(publisherInteraction)
               && receiverParameter.equals(publisherParameter),
            "application interaction handles differed between joined federates");
         publisher.publishInteractionClass(publisherInteraction);
         interactionPublished = true;

         byte[] encodedReceiver = encode(receiverFederate);
         await(receiver, callbackModel,
            () -> receiverRecorder.momObject(federateAttribute, encodedReceiver) != null,
            "receiver federate MOM discovery");
         ObjectInstanceHandle receiverMomObject = receiverRecorder.momObject(
            federateAttribute, encodedReceiver);
         JavaTckSupport.check(receiverMomObject != null
               && momClass.equals(receiver.getKnownObjectClassHandle(receiverMomObject)),
            "receiver MOM discovery returned the wrong object class");

         JavaTckSupport.check(requestCount(receiver, receiverRecorder, callbackModel,
               receiverMomObject, countAttribute, reliable, encoder,
               "initial interactions-received request") == 0,
            "initial HLAinteractionsReceived value was not zero");

         byte[] firstPayload = new byte[] {0x49, 0x52, 0x01};
         byte[] periodicPayload = new byte[] {0x49, 0x52, 0x02};
         int directStart = receiverRecorder.interactions.size();
         for (int i = 0; i < 3; ++i) {
            sendApplicationInteraction(publisher, publisherInteraction, publisherParameter,
               firstPayload);
         }
         await(receiver, callbackModel,
            () -> countApplicationInteractions(receiverRecorder, directStart,
               expectedInteraction, receiverParameter, publisherFederate, firstPayload) == 3,
            "three direct application interaction callbacks");
         JavaTckSupport.check(requestCount(receiver, receiverRecorder, callbackModel,
               receiverMomObject, countAttribute, reliable, encoder,
               "direct interactions-received request") == 3,
            "direct HLAinteractionsReceived value did not match three delivered callbacks");

         InteractionClassHandle setTiming = receiver.getInteractionClassHandle(SET_TIMING);
         ParameterHandle timingFederate = receiver.getParameterHandle(setTiming, "HLAfederate");
         ParameterHandle reportPeriod = receiver.getParameterHandle(setTiming, "HLAreportPeriod");
         JavaTckSupport.check(SET_TIMING.equals(receiver.getInteractionClassName(setTiming))
               && "HLAfederate".equals(receiver.getParameterName(setTiming, timingFederate))
               && "HLAreportPeriod".equals(receiver.getParameterName(setTiming, reportPeriod)),
            "standard HLAsetTiming lookup did not round-trip");
         int periodicStart = receiverRecorder.interactions.size();
         int periodicReflectionStart = receiverRecorder.reflections.size();
         sendTiming(receiver, setTiming, timingFederate, reportPeriod, receiverFederate, 1,
            encoder);
         Thread.sleep(20L);
         for (int i = 0; i < 2; ++i) {
            sendApplicationInteraction(publisher, publisherInteraction, publisherParameter,
               periodicPayload);
         }
         await(receiver, callbackModel,
            () -> countApplicationInteractions(receiverRecorder, periodicStart,
               expectedInteraction, receiverParameter, publisherFederate, periodicPayload) == 2,
            "two periodic-phase application interaction callbacks");
         await(receiver, callbackModel,
            () -> findCountAtLeast(receiverRecorder, periodicReflectionStart, receiverMomObject,
               countAttribute, encoder, 5) != null,
            "periodic interactions-received count of five");
         Reflection periodicReflection = findCountAtLeast(receiverRecorder,
            periodicReflectionStart, receiverMomObject, countAttribute, encoder, 5);
         JavaTckSupport.check(periodicReflection != null
               && verifyCount(periodicReflection, countAttribute, reliable, encoder,
                  "periodic interactions-received report") == 5,
            "periodic HLAinteractionsReceived value did not equal five");

         sendTiming(receiver, setTiming, timingFederate, reportPeriod, receiverFederate, 0,
            encoder);
         JavaTckSupport.check(requestCount(receiver, receiverRecorder, callbackModel,
               receiverMomObject, countAttribute, reliable, encoder,
               "final interactions-received request") == 5,
            "HLAinteractionsReceived changed after periodic reporting was disabled");
         JavaTckSupport.check(countApplicationInteractions(receiverRecorder, directStart,
               receiverInteraction, receiverParameter, publisherFederate, firstPayload) == 3
               && countApplicationInteractions(receiverRecorder, periodicStart,
                  receiverInteraction, receiverParameter, publisherFederate,
                  periodicPayload) == 2,
            "application callback counts differed from the direct and periodic phases");

         receiver.unsubscribeInteractionClass(receiverInteraction);
         interactionSubscribed = false;
         receiver.unsubscribeObjectClassAttributes(momClass, momAttributes);
         momSubscribed = false;
         publisher.unpublishInteractionClass(publisherInteraction);
         interactionPublished = false;
         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         publisher.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
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

   static void runSent(RtiFactory factory, String fom, String mim, String timeImplementation,
         String interactionName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-federate-mom-interactions-sent-" + UUID.randomUUID();
      boolean publisherConnected = false, receiverConnected = false, created = false;
      boolean publisherJoined = false, receiverJoined = false, momSubscribed = false;
      boolean interactionSubscribed = false, interactionPublished = false;
      ObjectClassHandle momClass = null;
      AttributeHandleSet momAttributes = null;
      InteractionClassHandle publisherInteraction = null, receiverInteraction = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecutionWithMIM(federation, new String[] {fom}, mim,
            timeImplementation);
         created = true;
         FederateHandle publisherFederate = publisher.joinFederationExecution(
            "interactions-sent-publisher", "java-tck", federation);
         publisherJoined = true;

         momClass = publisher.getObjectClassHandle(MOM_CLASS);
         AttributeHandle federateAttribute = publisher.getAttributeHandle(momClass, FEDERATE_HANDLE);
         AttributeHandle countAttribute = publisher.getAttributeHandle(momClass,
            "HLAinteractionsSent");
         TransportationTypeHandle reliable = publisher.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(MOM_CLASS.equals(publisher.getObjectClassName(momClass))
               && FEDERATE_HANDLE.equals(publisher.getAttributeName(momClass, federateAttribute))
               && "HLAinteractionsSent".equals(publisher.getAttributeName(momClass, countAttribute))
               && "HLAreliable".equals(publisher.getTransportationTypeName(reliable)),
            "standard federate MOM interactions-sent lookup did not round-trip");
         momAttributes = JavaTckSupport.attributeSet(publisher, federateAttribute, countAttribute);
         publisher.subscribeObjectClassAttributes(momClass, momAttributes, "");
         momSubscribed = true;

         publisherInteraction = publisher.getInteractionClassHandle(interactionName);
         ParameterHandle publisherParameter = publisher.getParameterHandle(
            publisherInteraction, parameterName);
         JavaTckSupport.check(interactionName.equals(publisher.getInteractionClassName(
               publisherInteraction))
               && parameterName.equals(publisher.getParameterName(publisherInteraction,
                  publisherParameter)),
            "adapter-supplied application interaction lookup did not round-trip");
         publisher.publishInteractionClass(publisherInteraction);
         interactionPublished = true;

         FederateHandle receiverFederate = receiver.joinFederationExecution(
            "interactions-sent-receiver", "java-tck", federation);
         receiverJoined = true;
         receiverInteraction = receiver.getInteractionClassHandle(interactionName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(
            receiverInteraction, parameterName);
         JavaTckSupport.check(publisherInteraction.equals(receiverInteraction)
               && publisherParameter.equals(receiverParameter),
            "application interaction handles differed between joined federates");
         InteractionClassHandle expectedSentInteraction = receiverInteraction;
         receiver.subscribeInteractionClass(receiverInteraction);
         interactionSubscribed = true;

         byte[] encodedPublisher = encode(publisherFederate);
         await(publisher, callbackModel,
            () -> publisherRecorder.momObject(federateAttribute, encodedPublisher) != null,
            "publisher federate MOM discovery");
         ObjectInstanceHandle publisherMomObject = publisherRecorder.momObject(
            federateAttribute, encodedPublisher);
         JavaTckSupport.check(publisherMomObject != null
               && momClass.equals(publisher.getKnownObjectClassHandle(publisherMomObject)),
            "publisher MOM discovery returned the wrong object class");
         JavaTckSupport.check(requestCount(publisher, publisherRecorder, callbackModel,
               publisherMomObject, countAttribute, reliable, encoder,
               "initial interactions-sent request") == 0,
            "initial HLAinteractionsSent value was not zero");

         InteractionClassHandle setTiming = publisher.getInteractionClassHandle(SET_TIMING);
         ParameterHandle timingFederate = publisher.getParameterHandle(setTiming, "HLAfederate");
         ParameterHandle reportPeriod = publisher.getParameterHandle(setTiming, "HLAreportPeriod");
         JavaTckSupport.check(SET_TIMING.equals(publisher.getInteractionClassName(setTiming))
               && "HLAfederate".equals(publisher.getParameterName(setTiming, timingFederate))
               && "HLAreportPeriod".equals(publisher.getParameterName(setTiming, reportPeriod)),
            "standard HLAsetTiming lookup did not round-trip");
         sendTiming(publisher, setTiming, timingFederate, reportPeriod, publisherFederate, 1,
            encoder);
         Thread.sleep(20L);
         int baseline = requestCount(publisher, publisherRecorder, callbackModel,
            publisherMomObject, countAttribute, reliable, encoder,
            "interactions-sent baseline with periodic reports enabled");
         int periodicStart = publisherRecorder.reflections.size();
         int interactionStart = receiverRecorder.interactions.size();
         byte[] payload = new byte[] {0x49, 0x53, 0x01};
         for (int i = 0; i < 3; ++i)
            sendApplicationInteraction(publisher, publisherInteraction, publisherParameter,
               payload);
         await(receiver, callbackModel,
            () -> countApplicationInteractions(receiverRecorder, interactionStart,
               expectedSentInteraction, receiverParameter, publisherFederate, payload) == 3,
            "three delivered application interactions");
         int expected = baseline + 3;
         await(publisher, callbackModel,
            () -> findCountAtLeast(publisherRecorder, periodicStart, publisherMomObject,
               countAttribute, encoder, expected) != null,
            "periodic interactions-sent report");
         Reflection periodic = findCountAtLeast(publisherRecorder, periodicStart,
            publisherMomObject, countAttribute, encoder, expected);
         JavaTckSupport.check(periodic != null
               && verifyCount(periodic, countAttribute, reliable, encoder,
                  "periodic interactions-sent report") == expected,
            "periodic HLAinteractionsSent value did not match the accepted sends");
         JavaTckSupport.check(requestCount(publisher, publisherRecorder, callbackModel,
               publisherMomObject, countAttribute, reliable, encoder,
               "direct interactions-sent request") == expected,
            "direct HLAinteractionsSent value did not increase once per accepted send");

         sendTiming(publisher, setTiming, timingFederate, reportPeriod, publisherFederate, 0,
            encoder);
         receiver.unsubscribeInteractionClass(receiverInteraction);
         interactionSubscribed = false;
         publisher.unsubscribeObjectClassAttributes(momClass, momAttributes);
         momSubscribed = false;
         publisher.unpublishInteractionClass(publisherInteraction);
         interactionPublished = false;
         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         publisher.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (momSubscribed && publisherJoined && momClass != null && momAttributes != null) try {
            publisher.unsubscribeObjectClassAttributes(momClass, momAttributes);
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

   private static int requestCount(RTIambassador receiver, Recorder recorder,
         CallbackModel callbackModel, ObjectInstanceHandle momObject, AttributeHandle countAttribute,
         TransportationTypeHandle reliable, EncoderFactory encoder, String description)
         throws Exception {
      int first = recorder.reflections.size();
      receiver.requestAttributeValueUpdate(momObject,
         JavaTckSupport.attributeSet(receiver, countAttribute), new byte[0]);
      await(receiver, callbackModel,
         () -> findCount(recorder, first, momObject, countAttribute) != null, description);
      Reflection reflection = findCount(recorder, first, momObject, countAttribute);
      JavaTckSupport.check(reflection != null, description + " produced no MOM value");
      return verifyCount(reflection, countAttribute, reliable, encoder, description);
   }

   private static int verifyCount(Reflection reflection, AttributeHandle countAttribute,
         TransportationTypeHandle reliable, EncoderFactory encoder, String description)
         throws Exception {
      JavaTckSupport.check(reflection != null && reflection.values.size() == 1
            && reflection.values.containsKey(countAttribute) && reflection.tag.length == 0
            && reliable.equals(reflection.transportation) && reflection.producer == null
            && (reflection.regions == null || reflection.regions.isEmpty())
            && !reflection.timestampOrdered,
         description + " returned non-standard MOM reflection metadata");
      byte[] bytes = reflection.values.get(countAttribute);
      JavaTckSupport.check(bytes != null && bytes.length > 0,
         description + " omitted the encoded HLAinteractionsReceived value");
      HLAinteger32BE encoded = encoder.createHLAinteger32BE();
      encoded.decode(bytes);
      int value = encoded.getValue();
      JavaTckSupport.check(value >= 0, description + " returned a negative interaction count");
      return value;
   }

   private static Reflection findCount(Recorder recorder, int first,
         ObjectInstanceHandle object, AttributeHandle attribute) {
      List<Reflection> reflections = recorder.reflections;
      for (int i = Math.min(first, reflections.size()); i < reflections.size(); ++i) {
         Reflection reflection = reflections.get(i);
         if (object.equals(reflection.object) && reflection.values.containsKey(attribute))
            return reflection;
      }
      return null;
   }

   private static Reflection findCountAtLeast(Recorder recorder, int first,
         ObjectInstanceHandle object, AttributeHandle attribute, EncoderFactory encoder,
         int threshold) {
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
            throw new IllegalStateException("invalid HLAinteractionsReceived encoding", error);
         }
         if (value.getValue() >= threshold) return reflection;
      }
      return null;
   }

   private static int countApplicationInteractions(Recorder recorder, int first,
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
         ParameterHandle federateParameter, ParameterHandle periodParameter,
         FederateHandle target, int seconds, EncoderFactory encoder) throws Exception {
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

   private static FederateAmbassador noOpCallbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK publisher callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) return proxy == (arguments == null ? null : arguments[0]);
            }
            return null;
         });
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
                  if ("toString".equals(name)) return "Java TCK interactions-received MOM callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
