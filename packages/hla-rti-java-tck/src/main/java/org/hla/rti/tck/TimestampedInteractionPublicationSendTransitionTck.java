package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Checks the publication precondition on timestamped Send Interaction. */
final class TimestampedInteractionPublicationSendTransitionTck {
   private TimestampedInteractionPublicationSendTransitionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-timestamped-publication-transition-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      boolean published = false;
      boolean subscribed = false;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         publisher.joinFederationExecution("java-tck-tso-publication-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-tso-publication-receiver", federation);
         receiverJoined = true;

         InteractionClassHandle publisherClass =
            publisher.getInteractionClassHandle(interactionName);
         InteractionClassHandle receiverClass =
            receiver.getInteractionClassHandle(interactionName);
         ParameterHandle publisherParameter =
            publisher.getParameterHandle(publisherClass, parameterName);
         receiver.getParameterHandle(receiverClass, parameterName);
         publisher.publishInteractionClass(publisherClass);
         published = true;
         publisher.changeInteractionOrderType(publisherClass, OrderType.TIMESTAMP);
         receiver.subscribeInteractionClass(receiverClass);
         subscribed = true;

         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverTimeFactory = receiver.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(receiverTimeFactory.getName()),
            "timestamped publication-transition members selected different logical-time factories");
         LogicalTimeInterval lookahead = (LogicalTimeInterval) timeFactory.makeEpsilon();
         publisher.enableTimeRegulation(lookahead);
         receiver.enableTimeConstrained();
         JavaTckSupport.drain(publisher);
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(publisherRecorder.timeRegulationEnabled
               && receiverRecorder.timeConstrainedEnabled,
            "timestamped publication-transition time-role callbacks were not observed");

         LogicalTime initial = publisher.queryLogicalTime();
         LogicalTime timestamp = offset(initial, lookahead, 5);
         byte[] payload = new byte[] {0x50, 0x55, 0x42};
         byte[] tag = new byte[] {0x54, 0x53, 0x4f};
         MessageRetractionReturn accepted = send(
            publisher, publisherClass, publisherParameter, payload, tag, timestamp);
         JavaTckSupport.check(accepted != null && accepted.retractionHandleIsValid
               && accepted.handle != null,
            "published timestamped Send Interaction returned no valid retraction handle");
         JavaTckSupport.check(receiverRecorder.timestampedInteractions == 0,
            "published timestamped interaction arrived before a receiver time advance");

         publisher.unpublishInteractionClass(publisherClass);
         published = false;
         JavaTckSupport.expectFailure(
            () -> send(publisher, publisherClass, publisherParameter, payload, tag, timestamp),
            "InteractionClassNotPublished", "timestamped send after interaction unpublication");
         JavaTckSupport.check(receiverRecorder.timestampedInteractions == 0,
            "rejected unpublished timestamped send created a receiver callback");
      } finally {
         if (subscribed && receiverJoined) {
            try { receiver.unsubscribeInteractionClass(
               receiver.getInteractionClassHandle(interactionName)); }
            catch (Exception ignored) { }
         }
         if (published && publisherJoined) {
            try { publisher.unpublishInteractionClass(
               publisher.getInteractionClassHandle(interactionName)); }
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

   private static MessageRetractionReturn send(RTIambassador publisher,
         InteractionClassHandle interaction,
         ParameterHandle parameter, byte[] payload, byte[] tag, LogicalTime time)
         throws Exception {
      ParameterHandleValueMap parameters =
         publisher.getParameterHandleValueMapFactory().create(1);
      parameters.put(parameter, payload);
      return publisher.sendInteraction(interaction, parameters, tag, time);
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval,
         int steps) throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static final class Recorder {
      private boolean timeRegulationEnabled;
      private boolean timeConstrainedEnabled;
      private int timestampedInteractions;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
               } else if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length == 10) {
                  ++timestampedInteractions;
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK timestamped publication callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }
   }
}
