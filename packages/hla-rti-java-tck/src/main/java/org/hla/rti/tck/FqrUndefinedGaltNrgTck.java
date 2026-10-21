package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TimeQueryReturn;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Undefined-GALT FQR behavior for both adapter FOM NRG settings. */
final class FqrUndefinedGaltNrgTck {
   private FqrUndefinedGaltNrgTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName, CallbackModel model)
         throws Exception {
      JavaTckSupport.check(model == CallbackModel.HLA_EVOKED,
         "undefined-GALT FQR has native evidence only for evoked callbacks");
      runOne(factory, fom, timeImplementation, interactionClassName, parameterName,
         model, false);
      runOne(factory, fom, timeImplementation, interactionClassName, parameterName,
         model, true);
   }

   private static void runOne(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName, CallbackModel model,
         boolean queued) throws Exception {
      RTIambassador requester = factory.getRtiAmbassador();
      RTIambassador producer = factory.getRtiAmbassador();
      Recorder requesterEvents = new Recorder();
      Recorder producerEvents = new Recorder();
      String federation = "java-tck-fqr-undefined-galt-"
         + (queued ? "queued-" : "empty-") + UUID.randomUUID();
      boolean requesterConnected = false, producerConnected = false;
      boolean created = false, requesterJoined = false, producerJoined = false;
      try {
         requester.connect(requesterEvents.proxy(), model);
         requesterConnected = true;
         producer.connect(producerEvents.proxy(), model);
         producerConnected = true;
         requester.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         requester.joinFederationExecution("fqr-undefined-galt-requester", federation);
         requesterJoined = true;
         FederateHandle producerHandle = producer.joinFederationExecution(
            "fqr-undefined-galt-producer", federation);
         producerJoined = true;
         boolean nrgEnabled = requester.getNonRegulatedGrantSwitch();
         JavaTckSupport.check(producer.getNonRegulatedGrantSwitch() == nrgEnabled,
            "federates disagreed on the adapter FOM non-regulated grant switch");
         LogicalTimeFactory<?, ?> timeFactory = requester.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(producer.getTimeFactory().getName()),
            "federates selected different logical-time implementations");
         LogicalTime<?, ?> initial = requester.queryLogicalTime();
         LogicalTimeInterval<?> epsilon = timeFactory.makeEpsilon();
         LogicalTime<?, ?> messageTime = offset(initial, epsilon, 5);
         LogicalTime<?, ?> requestTime = offset(initial, epsilon, 10);
         requester.enableTimeConstrained();
         await(requester, () -> requesterEvents.constrainedEnabled == 1,
            "undefined-GALT time-constrained enablement");

         InteractionClassHandle interaction = null;
         ParameterHandle parameter = null;
         byte[] payload = {'N'};
         byte[] tag = {'F', 'Q', 'R'};
         if (queued) {
            interaction = producer.getInteractionClassHandle(interactionClassName);
            parameter = producer.getParameterHandle(interaction, parameterName);
            JavaTckSupport.check(interaction.equals(
                  requester.getInteractionClassHandle(interactionClassName)),
               "adapter FOM has mismatched interaction handles");
            producer.publishInteractionClass(interaction);
            producer.changeInteractionOrderType(interaction, OrderType.TIMESTAMP);
            requester.subscribeInteractionClass(interaction);
            producer.enableTimeRegulation(epsilon);
            await(producer, () -> producerEvents.regulationEnabled == 1,
               "undefined-GALT producer regulation");
            ParameterHandleValueMap values =
               producer.getParameterHandleValueMapFactory().create(1);
            values.put(parameter, payload.clone());
            MessageRetractionReturn retraction = producer.sendInteraction(
               interaction, values, tag.clone(), messageTime);
            JavaTckSupport.check(retraction != null && retraction.retractionHandleIsValid
                  && retraction.handle != null,
               "queued interaction lacks a retraction handle");
            producer.disableTimeRegulation();
            JavaTckSupport.drain(producer);
         }

         TimeQueryReturn galt = requester.queryGALT();
         JavaTckSupport.check(!galt.timeIsValid,
            "undefined-GALT FQR setup unexpectedly had a defined GALT");
         requester.flushQueueRequest(requestTime);
         await(requester, () -> requesterEvents.grants.size() == 1
               && (!queued || requesterEvents.interactions.size() == 1),
            "undefined-GALT Flush Queue Grant");
         LogicalTime<?, ?> expectedOptimistic = queued ? messageTime : requestTime;
         LogicalTime<?, ?> expectedActual = nrgEnabled ? expectedOptimistic : initial;
         Grant grant = requesterEvents.grants.get(0);
         JavaTckSupport.check(grant.actual.equals(expectedActual)
               && grant.optimistic.equals(expectedOptimistic),
            "undefined-GALT FQG actual or optimistic time disagreed with NRG switch");
         JavaTckSupport.check(requesterEvents.order.equals(queued
                  ? List.of("interaction", "flush-grant") : List.of("flush-grant")),
            "undefined-GALT FQR callback order differed from delivery order");
         JavaTckSupport.check(requesterEvents.interactions.size() == (queued ? 1 : 0),
            "undefined-GALT FQR delivered the wrong number of interactions");
         if (queued) {
            Delivery delivered = requesterEvents.interactions.get(0);
            JavaTckSupport.check(delivered.interaction.equals(interaction)
                  && delivered.parameters.size() == 1
                  && Arrays.equals(delivered.parameters.get(parameter), payload)
                  && Arrays.equals(delivered.tag, tag)
                  && delivered.time.equals(messageTime)
                  && delivered.producer.equals(producerHandle)
                  && delivered.sentOrder == OrderType.TIMESTAMP
                  && delivered.receivedOrder == OrderType.TIMESTAMP,
               "undefined-GALT FQR changed the delivered interaction fields");
         }
         JavaTckSupport.check(!requester.queryGALT().timeIsValid,
            "undefined-GALT FQR fabricated a GALT after its grant");
         JavaTckSupport.check(requester.queryLogicalTime().equals(expectedActual),
            "undefined-GALT requester logical time after FQR");

         requester.disableTimeConstrained();
         requester.resignFederationExecution(ResignAction.NO_ACTION);
         requesterJoined = false;
         producer.resignFederationExecution(ResignAction.NO_ACTION);
         producerJoined = false;
         requester.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (requesterJoined) try { requester.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (producerJoined) try { producer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (created) try { requester.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (producerConnected) try { producer.disconnect(); } catch (Exception ignored) { }
         if (requesterConnected) try { requester.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static void await(RTIambassador member, BooleanSupplier ready,
         String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (!ready.getAsBoolean() && System.nanoTime() < deadline)
         member.evokeCallback(0.0);
      JavaTckSupport.check(ready.getAsBoolean(), "timed out waiting for " + description);
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval epsilon,
         int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index)
         result = (LogicalTime) result.add(epsilon);
      return result;
   }

   private static final class Grant {
      LogicalTime<?, ?> actual, optimistic;
   }

   private static final class Delivery {
      InteractionClassHandle interaction;
      ParameterHandleValueMap parameters;
      byte[] tag;
      FederateHandle producer;
      LogicalTime<?, ?> time;
      OrderType sentOrder, receivedOrder;
   }

   private static final class Recorder {
      volatile int constrainedEnabled, regulationEnabled;
      final List<Grant> grants = new CopyOnWriteArrayList<>();
      final List<Delivery> interactions = new CopyOnWriteArrayList<>();
      final List<String> order = new CopyOnWriteArrayList<>();

      FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class}, (proxy, method, args) -> {
               String name = method.getName();
               if ("timeConstrainedEnabled".equals(name)) constrainedEnabled++;
               else if ("timeRegulationEnabled".equals(name)) regulationEnabled++;
               else if ("receiveInteraction".equals(name) && args.length == 10) {
                  Delivery delivery = new Delivery();
                  delivery.interaction = (InteractionClassHandle) args[0];
                  delivery.parameters = ((ParameterHandleValueMap) args[1]).clone();
                  delivery.tag = ((byte[]) args[2]).clone();
                  delivery.producer = (FederateHandle) args[4];
                  delivery.time = (LogicalTime<?, ?>) args[6];
                  delivery.sentOrder = (OrderType) args[7];
                  delivery.receivedOrder = (OrderType) args[8];
                  interactions.add(delivery);
                  order.add("interaction");
               } else if ("flushQueueGrant".equals(name) && args.length == 2) {
                  Grant grant = new Grant();
                  grant.actual = (LogicalTime<?, ?>) args[0];
                  grant.optimistic = (LogicalTime<?, ?>) args[1];
                  grants.add(grant);
                  order.add("flush-grant");
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK undefined-GALT FQR callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == args[0];
               }
               return null;
            });
      }
   }
}
