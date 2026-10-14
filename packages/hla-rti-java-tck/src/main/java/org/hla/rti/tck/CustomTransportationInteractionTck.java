package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies ordinary interaction delivery and query with an adapter-named FOM transport. */
final class CustomTransportationInteractionTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private CustomTransportationInteractionTck() { }

   static void run(RtiFactory factory, String fom, String interactionName,
         String parameterName, String transportationName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador producer = factory.getRtiAmbassador();
      RTIambassador consumer = factory.getRtiAmbassador();
      Recorder producerCallbacks = new Recorder();
      Recorder consumerCallbacks = new Recorder();
      String federation = "java-tck-custom-transport-interaction-" + UUID.randomUUID();
      boolean producerConnected = false, consumerConnected = false, created = false;
      boolean producerJoined = false, consumerJoined = false;
      boolean published = false, subscribed = false;
      try {
         producer.connect(producerCallbacks.proxy(), callbackModel);
         producerConnected = true;
         consumer.connect(consumerCallbacks.proxy(), callbackModel);
         consumerConnected = true;
         producer.createFederationExecution(federation, fom);
         created = true;
         FederateHandle producerHandle = producer.joinFederationExecution(
            "java-tck-custom-transport-producer", federation);
         producerJoined = true;
         consumer.joinFederationExecution("java-tck-custom-transport-consumer", federation);
         consumerJoined = true;

         InteractionClassHandle producerInteraction =
            producer.getInteractionClassHandle(interactionName);
         InteractionClassHandle consumerInteraction =
            consumer.getInteractionClassHandle(interactionName);
         ParameterHandle producerParameter =
            producer.getParameterHandle(producerInteraction, parameterName);
         ParameterHandle consumerParameter =
            consumer.getParameterHandle(consumerInteraction, parameterName);
         TransportationTypeHandle producerTransportation =
            producer.getTransportationTypeHandle(transportationName);
         TransportationTypeHandle consumerTransportation =
            consumer.getTransportationTypeHandle(transportationName);
         JavaTckSupport.check(interactionName.equals(
               producer.getInteractionClassName(producerInteraction))
               && interactionName.equals(consumer.getInteractionClassName(consumerInteraction))
               && parameterName.equals(producer.getParameterName(
                  producerInteraction, producerParameter))
               && parameterName.equals(consumer.getParameterName(
                  consumerInteraction, consumerParameter))
               && transportationName.equals(producer.getTransportationTypeName(
                  producerTransportation))
               && transportationName.equals(consumer.getTransportationTypeName(
                  consumerTransportation)),
            "adapter FOM class, parameter, or custom transportation name did not round-trip");

         producer.publishInteractionClass(producerInteraction);
         published = true;
         consumer.subscribeInteractionClass(consumerInteraction);
         subscribed = true;

         HLAinteger32BE encoded = factory.getEncoderFactory().createHLAinteger32BE(42);
         byte[] payload = encoded.toByteArray();
         byte[] tag = new byte[] {0x43, 0x54, 0x58};
         ParameterHandleValueMap parameters =
            producer.getParameterHandleValueMapFactory().create(1);
         parameters.put(producerParameter, payload.clone());
         producer.sendInteraction(producerInteraction, parameters, tag.clone());

         await(consumer, callbackModel, consumerCallbacks,
            "custom transportation interaction was not delivered");
         JavaTckSupport.check(consumerCallbacks.interactions.size() == 1,
            "custom transportation interaction produced an unexpected callback count");
         Interaction received = consumerCallbacks.interactions.get(0);
         JavaTckSupport.check(consumerInteraction.equals(received.interaction)
               && received.parameters.size() == 1
               && Arrays.equals(payload, received.parameters.get(consumerParameter))
               && Arrays.equals(tag, received.tag)
               && producerHandle.equals(received.producer)
               && received.transportation != null
               && transportationName.equals(consumer.getTransportationTypeName(
                  received.transportation))
               && received.regionsEmpty,
            "custom transportation interaction changed the standard callback metadata");

         producerCallbacks.transportReports.clear();
         producer.queryInteractionTransportationType(producerHandle, producerInteraction);
         await(producer, callbackModel, producerCallbacks,
            "custom transportation query did not produce a report");
         JavaTckSupport.check(producerCallbacks.transportReports.size() == 1,
            "custom transportation query produced an unexpected report count");
         TransportReport report = producerCallbacks.transportReports.get(0);
         JavaTckSupport.check(producerHandle.equals(report.federate)
               && producerInteraction.equals(report.interaction)
               && report.transportation != null
               && transportationName.equals(producer.getTransportationTypeName(
                  report.transportation)),
            "custom transportation query reported the wrong federate, class, or type");

         consumer.unsubscribeInteractionClass(consumerInteraction);
         subscribed = false;
         producer.unpublishInteractionClass(producerInteraction);
         published = false;
      } finally {
         if (consumerJoined && subscribed) {
            try { consumer.unsubscribeInteractionClass(
               consumer.getInteractionClassHandle(interactionName)); }
            catch (Exception ignored) { }
         }
         if (producerJoined && published) {
            try { producer.unpublishInteractionClass(
               producer.getInteractionClassHandle(interactionName)); }
            catch (Exception ignored) { }
         }
         if (consumerJoined) resign(consumer);
         if (producerJoined) resign(producer);
         if (created) {
            try { producer.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (consumerConnected) disconnect(consumer);
         if (producerConnected) disconnect(producer);
      }
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         Recorder recorder, String failure) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && recorder.pendingCount() == 0) {
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
      }
      JavaTckSupport.check(recorder.pendingCount() > 0, failure);
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); }
      catch (Exception ignored) { }
   }

   private static final class Interaction {
      private final InteractionClassHandle interaction;
      private final ParameterHandleValueMap parameters;
      private final byte[] tag;
      private final FederateHandle producer;
      private final TransportationTypeHandle transportation;
      private final boolean regionsEmpty;

      private Interaction(InteractionClassHandle interaction, ParameterHandleValueMap parameters,
            byte[] tag, TransportationTypeHandle transportation, FederateHandle producer,
            RegionHandleSet regions) {
         this.interaction = interaction;
         this.parameters = parameters.clone();
         this.tag = tag.clone();
         this.transportation = transportation;
         this.producer = producer;
         this.regionsEmpty = regions == null || regions.isEmpty();
      }
   }

   private static final class TransportReport {
      private final FederateHandle federate;
      private final InteractionClassHandle interaction;
      private final TransportationTypeHandle transportation;

      private TransportReport(FederateHandle federate, InteractionClassHandle interaction,
            TransportationTypeHandle transportation) {
         this.federate = federate;
         this.interaction = interaction;
         this.transportation = transportation;
      }
   }

   private static final class Recorder {
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();
      private final List<TransportReport> transportReports = new CopyOnWriteArrayList<>();

      private int pendingCount() { return interactions.size() + transportReports.size(); }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 6) {
                  interactions.add(new Interaction((InteractionClassHandle) arguments[0],
                     (ParameterHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     (RegionHandleSet) arguments[5]));
               } else if ("reportInteractionTransportationType".equals(name)
                     && arguments != null && arguments.length == 3) {
                  transportReports.add(new TransportReport((FederateHandle) arguments[0],
                     (InteractionClassHandle) arguments[1],
                     (TransportationTypeHandle) arguments[2]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK custom transportation callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
