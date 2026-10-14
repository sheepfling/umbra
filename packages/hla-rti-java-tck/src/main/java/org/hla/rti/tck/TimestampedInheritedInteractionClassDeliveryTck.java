package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.encoding.HLAunicodeString;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verifies timestamped interaction inheritance, projection, and time-constrained delivery. */
final class TimestampedInheritedInteractionClassDeliveryTck {
   private static final long TIMEOUT_NANOS = 10_000_000_000L;

   private TimestampedInheritedInteractionClassDeliveryTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String baseClassName, String derivedClassName, String baseParameterName,
         String derivedParameterName, CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador baseSubscriber = factory.getRtiAmbassador();
      RTIambassador derivedSubscriber = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder baseCallbacks = new Recorder();
      Recorder derivedCallbacks = new Recorder();
      String federation = "java-tck-timestamped-inherited-interaction-" + UUID.randomUUID();
      boolean publisherConnected = false, baseConnected = false, derivedConnected = false;
      boolean created = false, publisherJoined = false, baseJoined = false, derivedJoined = false;
      boolean published = false, baseSubscribed = false, derivedSubscribed = false;
      boolean regulating = false, baseConstrained = false, derivedConstrained = false;
      InteractionClassHandle base = null, derived = null, baseSubscriberBase = null;
      InteractionClassHandle derivedSubscriberClass = null;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         baseSubscriber.connect(baseCallbacks.proxy(), callbackModel);
         baseConnected = true;
         derivedSubscriber.connect(derivedCallbacks.proxy(), callbackModel);
         derivedConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-timestamped-hierarchy-publisher", federation);
         publisherJoined = true;
         baseSubscriber.joinFederationExecution("java-tck-timestamped-hierarchy-base", federation);
         baseJoined = true;
         derivedSubscriber.joinFederationExecution(
            "java-tck-timestamped-hierarchy-derived", federation);
         derivedJoined = true;

         base = publisher.getInteractionClassHandle(baseClassName);
         derived = publisher.getInteractionClassHandle(derivedClassName);
         baseSubscriberBase = baseSubscriber.getInteractionClassHandle(baseClassName);
         derivedSubscriberClass = derivedSubscriber.getInteractionClassHandle(derivedClassName);
         JavaTckSupport.check(base != null && derived != null && base.equals(baseSubscriberBase)
               && derived.equals(derivedSubscriberClass),
            "timestamped interaction hierarchy handles were inconsistent across federates");

         ParameterHandle publisherBaseParameter = publisher.getParameterHandle(base, baseParameterName);
         ParameterHandle publisherInheritedParameter =
            publisher.getParameterHandle(derived, baseParameterName);
         ParameterHandle publisherDerivedParameter =
            publisher.getParameterHandle(derived, derivedParameterName);
         ParameterHandle baseSubscriberParameter =
            baseSubscriber.getParameterHandle(baseSubscriberBase, baseParameterName);
         ParameterHandle derivedSubscriberBaseParameter =
            derivedSubscriber.getParameterHandle(derivedSubscriberClass, baseParameterName);
         ParameterHandle derivedSubscriberParameter =
            derivedSubscriber.getParameterHandle(derivedSubscriberClass, derivedParameterName);
         JavaTckSupport.check(publisherBaseParameter.equals(publisherInheritedParameter)
               && publisherBaseParameter.equals(baseSubscriberParameter)
               && publisherBaseParameter.equals(derivedSubscriberBaseParameter)
               && publisherDerivedParameter.equals(derivedSubscriberParameter),
            "timestamped inherited interaction parameter handles were inconsistent");

         publisher.publishInteractionClass(derived);
         published = true;
         publisher.changeInteractionOrderType(derived, OrderType.TIMESTAMP);
         baseSubscriber.subscribeInteractionClass(baseSubscriberBase);
         baseSubscribed = true;
         derivedSubscriber.subscribeInteractionClass(derivedSubscriberClass);
         derivedSubscribed = true;

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> baseFactory = baseSubscriber.getTimeFactory();
         LogicalTimeFactory<?, ?> derivedFactory = derivedSubscriber.getTimeFactory();
         JavaTckSupport.check(publisherFactory.getName().equals(baseFactory.getName())
               && publisherFactory.getName().equals(derivedFactory.getName()),
            "timestamped interaction hierarchy selected different logical-time factories");
         LogicalTimeInterval publisherEpsilon =
            (LogicalTimeInterval) publisherFactory.makeEpsilon();
         LogicalTimeInterval baseEpsilon = (LogicalTimeInterval) baseFactory.makeEpsilon();
         LogicalTimeInterval derivedEpsilon = (LogicalTimeInterval) derivedFactory.makeEpsilon();
         baseSubscriber.enableTimeConstrained();
         baseConstrained = true;
         await(baseSubscriber, callbackModel, () -> baseCallbacks.timeConstrainedEnabled,
            "base subscriber time-constrained callback");
         derivedSubscriber.enableTimeConstrained();
         derivedConstrained = true;
         await(derivedSubscriber, callbackModel, () -> derivedCallbacks.timeConstrainedEnabled,
            "derived subscriber time-constrained callback");
         publisher.enableTimeRegulation(publisherEpsilon);
         regulating = true;
         await(publisher, callbackModel, () -> publisherCallbacks.timeRegulationEnabled,
            "publisher time-regulation callback");

         LogicalTime publisherInitial = publisher.queryLogicalTime();
         LogicalTime baseInitial = baseSubscriber.queryLogicalTime();
         LogicalTime derivedInitial = derivedSubscriber.queryLogicalTime();
         JavaTckSupport.check(publisherInitial.equals(baseInitial)
               && publisherInitial.equals(derivedInitial),
            "timestamped interaction federates began at different logical times");
         LogicalTime sendTime = addSteps(publisherInitial, publisherEpsilon, 4);
         LogicalTime baseTarget = addSteps(baseInitial, baseEpsilon, 4);
         LogicalTime derivedTarget = addSteps(derivedInitial, derivedEpsilon, 4);
         JavaTckSupport.check(sendTime.equals(baseTarget) && sendTime.equals(derivedTarget),
            "interaction timestamp and recipient advances selected different logical times");

         HLAinteger32BE integer = factory.getEncoderFactory().createHLAinteger32BE(0x13579bdf);
         HLAunicodeString text = factory.getEncoderFactory()
            .createHLAunicodeString("timestamped-derived");
         byte[] integerBytes = integer.toByteArray();
         byte[] derivedBytes = text.toByteArray();
         byte[] tag = new byte[] {0x54, 0x53, 0x4f, 0x31};
         ParameterHandleValueMap values = publisher.getParameterHandleValueMapFactory().create(2);
         values.put(publisherInheritedParameter, integerBytes.clone());
         values.put(publisherDerivedParameter, derivedBytes.clone());
         MessageRetractionReturn sent = publisher.sendInteraction(derived, values, tag.clone(), sendTime);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid && sent.handle != null,
            "timestamped inherited interaction returned an invalid retraction handle");

         pumpSeveral(baseSubscriber, derivedSubscriber, callbackModel);
         JavaTckSupport.check(baseCallbacks.interactions.isEmpty()
               && derivedCallbacks.interactions.isEmpty(),
            "time-constrained hierarchy received the timestamped send before advancing");

         baseSubscriber.timeAdvanceRequest(baseTarget);
         derivedSubscriber.timeAdvanceRequest(derivedTarget);
         publisher.timeAdvanceRequest(sendTime);
         awaitBoth(baseSubscriber, derivedSubscriber, callbackModel,
            () -> baseCallbacks.interactions.size() == 1 && baseCallbacks.grants.size() == 1
               && derivedCallbacks.interactions.size() == 1 && derivedCallbacks.grants.size() == 1,
            "timestamped inherited delivery and subscriber grants");
         await(publisher, callbackModel, () -> publisherCallbacks.grants.size() == 1,
            "publisher time-advance grant");
         pumpSeveral(baseSubscriber, derivedSubscriber, callbackModel);
         JavaTckSupport.check(baseCallbacks.interactions.size() == 1
               && derivedCallbacks.interactions.size() == 1
               && baseCallbacks.grants.size() == 1 && derivedCallbacks.grants.size() == 1
               && publisherCallbacks.grants.size() == 1,
            "timestamped derived interaction was not delivered once per subscriber and grant");

         Interaction baseReceived = baseCallbacks.interactions.get(0);
         assertTimestampedMetadata(baseSubscriber, baseReceived, baseSubscriberBase,
            publisherBaseParameter, producer, integerBytes, tag, sendTime, sent.handle,
            "base subscription");
         JavaTckSupport.check(baseReceived.parameters.size() == 1
               && !baseReceived.parameters.containsKey(publisherDerivedParameter),
            "base subscription did not project timestamped delivery to base parameters");

         Interaction derivedReceived = derivedCallbacks.interactions.get(0);
         assertTimestampedMetadata(derivedSubscriber, derivedReceived, derivedSubscriberClass,
            derivedSubscriberBaseParameter, producer, integerBytes, tag, sendTime, sent.handle,
            "derived subscription");
         JavaTckSupport.check(derivedReceived.parameters.size() == 2
               && Arrays.equals(derivedBytes, derivedReceived.parameters.get(derivedSubscriberParameter)),
            "derived subscription did not receive the derived timestamped parameter");
         JavaTckSupport.check(baseTarget.equals(baseCallbacks.grants.get(0))
               && derivedTarget.equals(derivedCallbacks.grants.get(0))
               && sendTime.equals(publisherCallbacks.grants.get(0)),
            "timestamped inherited interaction returned incorrect grant times");

         baseSubscriber.disableTimeConstrained();
         baseConstrained = false;
         derivedSubscriber.disableTimeConstrained();
         derivedConstrained = false;
         publisher.disableTimeRegulation();
         regulating = false;
         baseSubscriber.unsubscribeInteractionClass(baseSubscriberBase);
         baseSubscribed = false;
         derivedSubscriber.unsubscribeInteractionClass(derivedSubscriberClass);
         derivedSubscribed = false;
         publisher.unpublishInteractionClass(derived);
         published = false;
      } finally {
         if (baseConstrained && baseJoined) try { baseSubscriber.disableTimeConstrained(); }
            catch (Exception ignored) { }
         if (derivedConstrained && derivedJoined) try { derivedSubscriber.disableTimeConstrained(); }
            catch (Exception ignored) { }
         if (regulating && publisherJoined) try { publisher.disableTimeRegulation(); }
            catch (Exception ignored) { }
         if (baseSubscribed && baseJoined && baseSubscriberBase != null) try {
            baseSubscriber.unsubscribeInteractionClass(baseSubscriberBase);
         } catch (Exception ignored) { }
         if (derivedSubscribed && derivedJoined && derivedSubscriberClass != null) try {
            derivedSubscriber.unsubscribeInteractionClass(derivedSubscriberClass);
         } catch (Exception ignored) { }
         if (published && publisherJoined && derived != null) try {
            publisher.unpublishInteractionClass(derived);
         } catch (Exception ignored) { }
         if (derivedJoined) try { derivedSubscriber.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (baseJoined) try { baseSubscriber.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (publisherJoined) try {
            publisher.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         } catch (Exception ignored) { }
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (derivedConnected) try { derivedSubscriber.disconnect(); } catch (Exception ignored) { }
         if (baseConnected) try { baseSubscriber.disconnect(); } catch (Exception ignored) { }
         if (publisherConnected) try { publisher.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static void assertTimestampedMetadata(RTIambassador receiver, Interaction actual,
         InteractionClassHandle expectedClass, ParameterHandle expectedParameter,
         FederateHandle expectedProducer, byte[] expectedValue, byte[] expectedTag,
         LogicalTime<?, ?> expectedTime, MessageRetractionHandle expectedRetraction,
         String description) throws Exception {
      JavaTckSupport.check(actual.interaction.equals(expectedClass)
            && actual.parameters.containsKey(expectedParameter)
            && Arrays.equals(expectedValue, actual.parameters.get(expectedParameter))
            && Arrays.equals(expectedTag, actual.tag)
            && expectedProducer.equals(actual.producer)
            && expectedTime.equals(actual.time)
            && actual.sentOrder == OrderType.TIMESTAMP
            && actual.receivedOrder == OrderType.TIMESTAMP
            && expectedRetraction.equals(actual.retraction)
            && actual.transportation != null && actual.regionsEmpty,
         description + " changed timestamped interaction metadata");
      JavaTckSupport.check(!receiver.getTransportationTypeName(actual.transportation).isEmpty()
            && actual.time != null && !actual.time.toString().isEmpty(),
         description + " returned unresolvable time or transportation metadata");
   }

   private static LogicalTime addSteps(LogicalTime start, LogicalTimeInterval interval, int count)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < count; ++i) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         java.util.function.BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         pump(ambassador, model);
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void awaitBoth(RTIambassador first, RTIambassador second, CallbackModel model,
         java.util.function.BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         pump(first, model);
         pump(second, model);
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void pumpSeveral(RTIambassador first, RTIambassador second, CallbackModel model)
         throws Exception {
      for (int i = 0; i < 8; ++i) {
         pump(first, model);
         pump(second, model);
      }
   }

   private static void pump(RTIambassador ambassador, CallbackModel model) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
      else Thread.yield();
   }

   private static FederateAmbassador noopFederateAmbassador() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK timestamped hierarchy publisher";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) return proxy == arguments[0];
            }
            return null;
         });
   }

   private static final class Interaction {
      private final InteractionClassHandle interaction;
      private final Map<ParameterHandle, byte[]> parameters = new HashMap<>();
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;
      private final MessageRetractionHandle retraction;
      private final boolean regionsEmpty;

      private Interaction(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         ParameterHandleValueMap values = (ParameterHandleValueMap) arguments[1];
         values.forEach((handle, value) -> parameters.put(handle, value.clone()));
         tag = ((byte[]) arguments[2]).clone();
         transportation = (TransportationTypeHandle) arguments[3];
         producer = (FederateHandle) arguments[4];
         RegionHandleSet regions = (RegionHandleSet) arguments[5];
         regionsEmpty = regions != null && regions.isEmpty();
         time = (LogicalTime<?, ?>) arguments[6];
         sentOrder = (OrderType) arguments[7];
         receivedOrder = (OrderType) arguments[8];
         retraction = (MessageRetractionHandle) arguments[9];
      }
   }

   private static final class Recorder {
      private volatile boolean timeRegulationEnabled;
      private volatile boolean timeConstrainedEnabled;
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) timeRegulationEnabled = true;
               else if ("timeConstrainedEnabled".equals(name)) timeConstrainedEnabled = true;
               else if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 10) interactions.add(new Interaction(arguments));
               else if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) grants.add((LogicalTime<?, ?>) arguments[0]);
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK timestamped hierarchy callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == arguments[0];
               }
               return null;
            });
      }
   }
}
