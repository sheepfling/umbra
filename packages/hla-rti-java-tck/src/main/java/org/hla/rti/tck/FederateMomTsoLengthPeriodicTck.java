package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.MessageRetractionReturn;
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
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks direct and periodic standard MOM reporting around queued timestamped delivery. */
final class FederateMomTsoLengthPeriodicTck {
   private static final String MOM_CLASS = "HLAobjectRoot.HLAmanager.HLAfederate";
   private static final String FEDERATE_HANDLE = "HLAfederateHandle";
   private static final String TSO_LENGTH = "HLATSOlength";
   private static final String SET_TIMING =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming";
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private FederateMomTsoLengthPeriodicTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String interactionName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador observer = factory.getRtiAmbassador();
      RTIambassador target = factory.getRtiAmbassador();
      RTIambassador publisher = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder observerRecorder = new Recorder();
      Recorder targetRecorder = new Recorder();
      Recorder publisherRecorder = new Recorder();
      String federation = "java-tck-federate-mom-tso-length-" + UUID.randomUUID();
      boolean observerConnected = false, targetConnected = false, publisherConnected = false;
      boolean created = false, observerJoined = false, targetJoined = false, publisherJoined = false;
      boolean subscribedMom = false, subscribedInteraction = false, publishedInteraction = false;
      boolean constrained = false, regulating = false;
      ObjectClassHandle momClass = null;
      AttributeHandleSet momSubscription = null;
      InteractionClassHandle publisherInteraction = null, targetInteraction = null;
      try {
         observer.connect(observerRecorder.proxy(), callbackModel);
         observerConnected = true;
         target.connect(targetRecorder.proxy(), callbackModel);
         targetConnected = true;
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         observer.createFederationExecutionWithMIM(federation, new String[] {fom}, mim,
            timeImplementation);
         created = true;
         observer.joinFederationExecution("tso-length-observer", "java-tck", federation);
         observerJoined = true;

         momClass = observer.getObjectClassHandle(MOM_CLASS);
         AttributeHandle federateAttribute = observer.getAttributeHandle(momClass, FEDERATE_HANDLE);
         AttributeHandle tsoLengthAttribute = observer.getAttributeHandle(momClass, TSO_LENGTH);
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(MOM_CLASS.equals(observer.getObjectClassName(momClass))
               && FEDERATE_HANDLE.equals(observer.getAttributeName(momClass, federateAttribute))
               && TSO_LENGTH.equals(observer.getAttributeName(momClass, tsoLengthAttribute))
               && "HLAreliable".equals(observer.getTransportationTypeName(reliable)),
            "standard federate MOM TSO-length lookup did not round-trip");
         momSubscription = JavaTckSupport.attributeSet(observer, federateAttribute,
            tsoLengthAttribute);
         observer.subscribeObjectClassAttributes(momClass, momSubscription, "");
         subscribedMom = true;

         FederateHandle targetFederate = target.joinFederationExecution(
            "tso-length-target", "java-tck", federation);
         targetJoined = true;
         FederateHandle publisherFederate = publisher.joinFederationExecution(
            "tso-length-publisher", "java-tck", federation);
         publisherJoined = true;
         byte[] encodedTarget = encode(targetFederate);
         await(observer, callbackModel,
            () -> observerRecorder.momObject(federateAttribute, encodedTarget) != null,
            "target federate MOM discovery");
         ObjectInstanceHandle targetMomObject = observerRecorder.momObject(
            federateAttribute, encodedTarget);
         JavaTckSupport.check(targetMomObject != null
               && momClass.equals(observer.getKnownObjectClassHandle(targetMomObject)),
            "target federate MOM discovery returned the wrong object class");

         publisherInteraction = publisher.getInteractionClassHandle(interactionName);
         targetInteraction = target.getInteractionClassHandle(interactionName);
         ParameterHandle publisherParameter = publisher.getParameterHandle(
            publisherInteraction, parameterName);
         ParameterHandle targetParameter = target.getParameterHandle(targetInteraction, parameterName);
         JavaTckSupport.check(publisherInteraction.equals(targetInteraction)
               && publisherInteraction != null && publisherParameter != null
               && targetParameter != null,
            "adapter-supplied timestamped interaction or parameter lookup failed");
         publisher.publishInteractionClass(publisherInteraction);
         publishedInteraction = true;
         publisher.changeInteractionOrderType(publisherInteraction, OrderType.TIMESTAMP);
         target.subscribeInteractionClass(targetInteraction);
         subscribedInteraction = true;

         LogicalTimeFactory<?, ?> targetTimeFactory = target.getTimeFactory();
         LogicalTimeFactory<?, ?> publisherTimeFactory = publisher.getTimeFactory();
         JavaTckSupport.check(targetTimeFactory.getName().equals(publisherTimeFactory.getName()),
            "timestamped interaction members selected different logical-time factories");
         LogicalTime<?, ?> targetInitial = (LogicalTime<?, ?>) targetTimeFactory.makeInitial();
         LogicalTime<?, ?> publisherInitial = (LogicalTime<?, ?>) publisherTimeFactory.makeInitial();
         LogicalTimeInterval targetEpsilon = (LogicalTimeInterval) targetTimeFactory.makeEpsilon();
         LogicalTimeInterval publisherEpsilon =
            (LogicalTimeInterval) publisherTimeFactory.makeEpsilon();
         JavaTckSupport.check(targetInitial.equals(publisherInitial)
               && targetEpsilon.equals(publisherEpsilon),
            "timestamped interaction members selected different initial times or epsilons");
         target.enableTimeConstrained();
         constrained = true;
         await(target, callbackModel, () -> targetRecorder.timeConstrainedEnabled == 1,
            "target timeConstrainedEnabled callback");
         publisher.enableTimeRegulation(publisherEpsilon);
         regulating = true;
         await(publisher, callbackModel, () -> publisherRecorder.timeRegulationEnabled == 1,
            "publisher timeRegulationEnabled callback");

         LogicalTime<?, ?> messageTime = add(publisherInitial, publisherEpsilon, 2);
         LogicalTime<?, ?> targetMessageTime = add(targetInitial, targetEpsilon, 2);
         byte[] payload = new byte[] {0x54, 0x53};
         ParameterHandleValueMap values = publisher.getParameterHandleValueMapFactory().create(1);
         values.put(publisherParameter, payload);
         MessageRetractionReturn sent = publisher.sendInteraction(
            publisherInteraction, values, new byte[0], messageTime);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid && sent.handle != null,
            "timestamped interaction send returned no valid retraction handle");
         JavaTckSupport.check(targetRecorder.interactions.isEmpty(),
            "target received the timestamped interaction before its time advance");
         requestTsoLength(observer, observerRecorder, callbackModel, targetMomObject,
            tsoLengthAttribute, reliable, encoder, 1, "direct TSO-length request");

         InteractionClassHandle setTiming = observer.getInteractionClassHandle(SET_TIMING);
         ParameterHandle timingFederate = observer.getParameterHandle(setTiming, "HLAfederate");
         ParameterHandle reportPeriod = observer.getParameterHandle(setTiming, "HLAreportPeriod");
         JavaTckSupport.check(SET_TIMING.equals(observer.getInteractionClassName(setTiming))
               && "HLAfederate".equals(observer.getParameterName(setTiming, timingFederate))
               && "HLAreportPeriod".equals(observer.getParameterName(setTiming, reportPeriod)),
            "standard HLAsetTiming lookup did not round-trip");
         int beforePeriodic = observerRecorder.reflections.size();
         sendTiming(observer, setTiming, timingFederate, reportPeriod, targetFederate, 1, encoder);
         await(observer, callbackModel,
            () -> findTsoLength(observerRecorder, beforePeriodic, targetMomObject,
               tsoLengthAttribute) != null,
            "periodic TSO-length report");
         verifyTsoLength(findTsoLength(observerRecorder, beforePeriodic, targetMomObject,
            tsoLengthAttribute), tsoLengthAttribute, reliable, encoder, 1,
            "periodic TSO-length report");

         sendTiming(observer, setTiming, timingFederate, reportPeriod, targetFederate, 0, encoder);
         target.timeAdvanceRequest(targetMessageTime);
         publisher.timeAdvanceRequest(messageTime);
         awaitBoth(target, publisher, callbackModel,
            () -> targetRecorder.timeAdvanceGrant != null
               && publisherRecorder.timeAdvanceGrant != null
               && !targetRecorder.interactions.isEmpty(),
            "timestamped delivery and independent grants");
         JavaTckSupport.check(targetRecorder.interactions.size() == 1
               && targetMessageTime.equals(targetRecorder.timeAdvanceGrant)
               && messageTime.equals(publisherRecorder.timeAdvanceGrant),
            "timestamped interaction or time-advance grant count/time was incorrect");
         JavaTckSupport.check(targetRecorder.callbackOrder.equals(
               Arrays.asList("receiveInteraction", "timeAdvanceGrant")),
            "timestamped interaction delivery did not precede the target grant");
         Interaction actual = targetRecorder.interactions.get(0);
         JavaTckSupport.check(targetInteraction.equals(actual.interaction)
               && actual.parameters.size() == 1
               && Arrays.equals(payload, actual.parameters.get(targetParameter))
               && actual.tag.length == 0 && publisherFederate.equals(actual.producer)
               && messageTime.equals(actual.time)
               && actual.sentOrder == OrderType.TIMESTAMP
               && actual.receivedOrder == OrderType.TIMESTAMP
               && sent.handle.equals(actual.retraction)
               && reliable.equals(actual.transportation),
            "timestamped interaction callback omitted standard delivery metadata");
         requestTsoLength(observer, observerRecorder, callbackModel, targetMomObject,
            tsoLengthAttribute, reliable, encoder, 0, "post-grant TSO-length request");

         observer.unsubscribeObjectClassAttributes(momClass, momSubscription);
         subscribedMom = false;
         target.unsubscribeInteractionClass(targetInteraction);
         subscribedInteraction = false;
         publisher.unpublishInteractionClass(publisherInteraction);
         publishedInteraction = false;
         target.disableTimeConstrained();
         constrained = false;
         publisher.disableTimeRegulation();
         regulating = false;
         target.resignFederationExecution(ResignAction.NO_ACTION);
         targetJoined = false;
         publisher.resignFederationExecution(ResignAction.NO_ACTION);
         publisherJoined = false;
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         observerJoined = false;
         observer.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (constrained && targetJoined) try { target.disableTimeConstrained(); }
            catch (Exception ignored) { }
         if (regulating && publisherJoined) try { publisher.disableTimeRegulation(); }
            catch (Exception ignored) { }
         if (subscribedMom && observerJoined && momClass != null && momSubscription != null) try {
            observer.unsubscribeObjectClassAttributes(momClass, momSubscription);
         } catch (Exception ignored) { }
         if (subscribedInteraction && targetJoined && targetInteraction != null) try {
            target.unsubscribeInteractionClass(targetInteraction);
         } catch (Exception ignored) { }
         if (publishedInteraction && publisherJoined && publisherInteraction != null) try {
            publisher.unpublishInteractionClass(publisherInteraction);
         } catch (Exception ignored) { }
         if (targetJoined) resign(target);
         if (publisherJoined) resign(publisher);
         if (observerJoined) resign(observer);
         if (created) try { observer.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (targetConnected) disconnect(target);
         if (publisherConnected) disconnect(publisher);
         if (observerConnected) disconnect(observer);
      }
   }

   private static void requestTsoLength(RTIambassador observer, Recorder recorder,
         CallbackModel callbackModel, ObjectInstanceHandle object, AttributeHandle attribute,
         TransportationTypeHandle reliable, EncoderFactory encoder, int expected,
         String description) throws Exception {
      int first = recorder.reflections.size();
      observer.requestAttributeValueUpdate(object, JavaTckSupport.attributeSet(observer, attribute),
         new byte[0]);
      await(observer, callbackModel,
         () -> findTsoLength(recorder, first, object, attribute) != null, description);
      verifyTsoLength(findTsoLength(recorder, first, object, attribute), attribute, reliable,
         encoder, expected, description);
   }

   private static Reflection findTsoLength(Recorder recorder, int first,
         ObjectInstanceHandle object, AttributeHandle attribute) {
      List<Reflection> reflections = recorder.reflections;
      for (int i = Math.min(first, reflections.size()); i < reflections.size(); ++i) {
         Reflection reflection = reflections.get(i);
         if (object.equals(reflection.object) && reflection.values.containsKey(attribute))
            return reflection;
      }
      return null;
   }

   private static void verifyTsoLength(Reflection reflection, AttributeHandle attribute,
         TransportationTypeHandle reliable, EncoderFactory encoder, int expected,
         String description) throws Exception {
      JavaTckSupport.check(reflection != null && reflection.values.size() == 1
            && reflection.tag.length == 0 && reliable.equals(reflection.transportation)
            && reflection.producer == null && (reflection.regions == null
               || reflection.regions.isEmpty()) && !reflection.timestampOrdered,
         description + " returned non-standard MOM reflection metadata");
      HLAinteger32BE value = encoder.createHLAinteger32BE();
      byte[] encoded = reflection.values.get(attribute);
      JavaTckSupport.check(encoded != null && encoded.length > 0,
         description + " omitted the encoded HLATSOlength value");
      value.decode(encoded);
      JavaTckSupport.check(value.getValue() == expected,
         description + " returned the wrong HLATSOlength value");
   }

   private static void sendTiming(RTIambassador observer, InteractionClassHandle interaction,
         ParameterHandle federateParameter, ParameterHandle periodParameter,
         FederateHandle federate, int seconds, EncoderFactory encoder) throws Exception {
      ParameterHandleValueMap parameters = observer.getParameterHandleValueMapFactory().create(2);
      parameters.put(federateParameter, encode(federate));
      parameters.put(periodParameter, encoder.createHLAinteger32BE(seconds).toByteArray());
      observer.sendInteraction(interaction, parameters, new byte[0]);
   }

   private static byte[] encode(FederateHandle handle) throws Exception {
      byte[] bytes = new byte[handle.encodedLength()];
      handle.encode(bytes, 0);
      return bytes;
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime add(LogicalTime initial, LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTime value = initial;
      for (int i = 0; i < steps; ++i) value = (LogicalTime) value.add(epsilon);
      return value;
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void awaitBoth(RTIambassador first, RTIambassador second, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         if (model == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(first);
            JavaTckSupport.drain(second);
         } else Thread.yield();
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
      private InteractionClassHandle interaction;
      private ParameterHandleValueMap parameters;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private LogicalTime<?, ?> time;
      private OrderType sentOrder;
      private OrderType receivedOrder;
      private MessageRetractionHandle retraction;
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
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();
      private volatile int timeConstrainedEnabled;
      private volatile int timeRegulationEnabled;
      private volatile LogicalTime<?, ?> timeAdvanceGrant;

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
               } else if ("timeConstrainedEnabled".equals(name)) {
                  ++timeConstrainedEnabled;
               } else if ("timeRegulationEnabled".equals(name)) {
                  ++timeRegulationEnabled;
               } else if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 10) {
                  Interaction event = new Interaction();
                  event.interaction = (InteractionClassHandle) arguments[0];
                  event.parameters = ((ParameterHandleValueMap) arguments[1]).clone();
                  event.tag = ((byte[]) arguments[2]).clone();
                  event.transportation = (TransportationTypeHandle) arguments[3];
                  event.producer = (FederateHandle) arguments[4];
                  event.time = (LogicalTime<?, ?>) arguments[6];
                  event.sentOrder = (OrderType) arguments[7];
                  event.receivedOrder = (OrderType) arguments[8];
                  event.retraction = (MessageRetractionHandle) arguments[9];
                  interactions.add(event);
                  callbackOrder.add(name);
               } else if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  timeAdvanceGrant = (LogicalTime<?, ?>) arguments[0];
                  callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK TSO-length MOM callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
