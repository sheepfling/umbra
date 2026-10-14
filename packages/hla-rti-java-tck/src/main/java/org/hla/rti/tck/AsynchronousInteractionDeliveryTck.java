package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Receive-order interactions retained by asynchronous delivery and released by time advance. */
final class AsynchronousInteractionDeliveryTck {
   private AsynchronousInteractionDeliveryTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName, String objectClassName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-asynchronous-interaction-" + UUID.randomUUID();
      boolean publisherConnected = false, receiverConnected = false, created = false;
      boolean publisherJoined = false, receiverJoined = false, timeConstrained = false;
      boolean published = false, subscribed = false;
      InteractionClassHandle publisherInteraction = null, receiverInteraction = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-asynchronous-interaction-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-asynchronous-interaction-receiver", federation);
         receiverJoined = true;

         publisherInteraction = publisher.getInteractionClassHandle(interactionClassName);
         receiverInteraction = receiver.getInteractionClassHandle(interactionClassName);
         ParameterHandle publisherParameter =
            publisher.getParameterHandle(publisherInteraction, parameterName);
         ParameterHandle receiverParameter =
            receiver.getParameterHandle(receiverInteraction, parameterName);
         JavaTckSupport.check(publisherInteraction.equals(receiverInteraction)
               && publisherParameter.equals(receiverParameter),
            "asynchronous interaction lookups returned inconsistent handles");
         publisher.publishInteractionClass(publisherInteraction);
         published = true;
         receiver.subscribeInteractionClass(receiverInteraction);
         subscribed = true;

         receiver.enableTimeConstrained();
         timeConstrained = true;
         await(receiver, callbackModel, objectClassName,
            () -> receiverRecorder.timeConstrainedEnabled,
            "time-constrained callback");

         byte[] value = new byte[] {(byte) 0xa5, 0x51};
         byte[] firstTag = new byte[] {0x41, 0x53, 0x59, 0x31};
         send(publisher, publisherInteraction, publisherParameter, value, firstTag);
         serviceRepeatedly(receiver, callbackModel, objectClassName, 8);
         JavaTckSupport.check(receiverRecorder.deliveries.isEmpty(),
            "receive-order interaction bypassed disabled asynchronous delivery");

         receiver.enableAsynchronousDelivery();
         JavaTckSupport.expectFailure(receiver::enableAsynchronousDelivery,
            "AsynchronousDeliveryAlreadyEnabled",
            "enabling asynchronous delivery twice was accepted");
         await(receiver, callbackModel, objectClassName,
            () -> receiverRecorder.deliveries.size() == 1,
            "asynchronous-delivery enable did not release the retained interaction");
         verify(receiver, receiverRecorder.deliveries.get(0), receiverInteraction,
            receiverParameter, value, firstTag, producer, "enable release");

         receiverRecorder.deliveries.clear();
         receiverRecorder.callbackSequence.clear();
         receiver.disableAsynchronousDelivery();
         JavaTckSupport.expectFailure(receiver::disableAsynchronousDelivery,
            "AsynchronousDeliveryAlreadyDisabled",
            "disabling asynchronous delivery twice was accepted");
         byte[] secondTag = new byte[] {0x41, 0x53, 0x59, 0x32};
         send(publisher, publisherInteraction, publisherParameter, value, secondTag);
         serviceRepeatedly(receiver, callbackModel, objectClassName, 8);
         JavaTckSupport.check(receiverRecorder.deliveries.isEmpty(),
            "disabled asynchronous delivery did not retain the next interaction");

         LogicalTimeFactory<?, ?> timeFactory = receiver.getTimeFactory();
         LogicalTime<?, ?> current = receiver.queryLogicalTime();
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         LogicalTime next = add((LogicalTime) current, epsilon);
         receiver.timeAdvanceRequest(next);
         await(receiver, callbackModel, objectClassName,
            () -> receiverRecorder.deliveries.size() == 1
               && next.equals(receiverRecorder.timeAdvanceGrant),
            "time advance did not release the retained interaction before its grant");
         verify(receiver, receiverRecorder.deliveries.get(0), receiverInteraction,
            receiverParameter, value, secondTag, producer, "time-advance release");
         JavaTckSupport.check(Arrays.asList("receiveInteraction", "timeAdvanceGrant")
               .equals(receiverRecorder.callbackSequence),
            "retained interaction was not delivered before its matching time-advance grant");
         JavaTckSupport.expectFailure(receiver::disableAsynchronousDelivery,
            "AsynchronousDeliveryAlreadyDisabled",
            "disabled asynchronous-delivery state changed after time-advance release");
      } finally {
         if (timeConstrained && receiverJoined) {
            try { receiver.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (subscribed && receiverJoined && receiverInteraction != null) {
            try { receiver.unsubscribeInteractionClass(receiverInteraction); }
            catch (Exception ignored) { }
         }
         if (published && publisherJoined && publisherInteraction != null) {
            try { publisher.unpublishInteractionClass(publisherInteraction); }
            catch (Exception ignored) { }
         }
         if (receiverJoined) {
            try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (receiverConnected) {
            try { receiver.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void send(RTIambassador publisher, InteractionClassHandle interaction,
         ParameterHandle parameter, byte[] value, byte[] tag) throws Exception {
      ParameterHandleValueMap values = publisher.getParameterHandleValueMapFactory().create(1);
      values.put(parameter, value);
      publisher.sendInteraction(interaction, values, tag);
   }

   private static void verify(RTIambassador receiver, Delivery delivery,
         InteractionClassHandle interaction, ParameterHandle parameter, byte[] value,
         byte[] tag, FederateHandle producer, String phase) throws Exception {
      JavaTckSupport.check(interaction.equals(delivery.interaction)
            && delivery.parameters.size() == 1
            && Arrays.equals(value, delivery.parameters.get(parameter))
            && Arrays.equals(tag, delivery.tag)
            && producer.equals(delivery.producer)
            && delivery.transportation != null
            && "HLAreliable".equals(receiver.getTransportationTypeName(delivery.transportation))
            && (delivery.regions == null || delivery.regions.isEmpty()),
         phase + " interaction callback lost its standard delivery data");
   }

   private static void serviceRepeatedly(RTIambassador ambassador, CallbackModel model,
         String objectClassName, int passes) throws Exception {
      for (int index = 0; index < passes; ++index) service(ambassador, model, objectClassName);
   }

   private static LogicalTime add(LogicalTime start, LogicalTimeInterval interval)
         throws Exception {
      return (LogicalTime) start.add(interval);
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         String objectClassName, Condition condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && !condition.isTrue()) {
         service(ambassador, model, objectClassName);
         Thread.yield();
      }
      JavaTckSupport.check(condition.isTrue(), "timed out waiting for " + description);
   }

   private static void service(RTIambassador ambassador, CallbackModel model,
         String objectClassName) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) ambassador.evokeMultipleCallbacks(0.0, 0.0);
      else ambassador.getObjectClassHandle(objectClassName);
   }

   @FunctionalInterface
   private interface Condition { boolean isTrue() throws Exception; }

   private static final class Delivery {
      private final InteractionClassHandle interaction;
      private final ParameterHandleValueMap parameters;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final hla.rti1516_2025.RegionHandleSet regions;

      private Delivery(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         parameters = ((ParameterHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
         transportation = (TransportationTypeHandle) arguments[3];
         producer = (FederateHandle) arguments[4];
         regions = (hla.rti1516_2025.RegionHandleSet) arguments[5];
      }
   }

   private static final class Recorder {
      private final List<Delivery> deliveries = new CopyOnWriteArrayList<>();
      private final List<String> callbackSequence = new CopyOnWriteArrayList<>();
      private volatile boolean timeConstrainedEnabled;
      private volatile LogicalTime<?, ?> timeAdvanceGrant;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
               } else if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 6) {
                  deliveries.add(new Delivery(arguments));
                  callbackSequence.add(name);
               } else if ("timeAdvanceGrant".equals(name)) {
                  timeAdvanceGrant = (LogicalTime<?, ?>) arguments[0];
                  callbackSequence.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK asynchronous interaction callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
