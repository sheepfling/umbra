package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
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
import java.util.UUID;

/** Checks that a queued TSO preserves LITS after its regulating source resigns. */
final class QueryLitsSourceResignationTck {
   private QueryLitsSourceResignationTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-query-lits-source-resignation-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      boolean published = false;
      boolean subscribed = false;
      boolean publisherRegulating = false;
      boolean receiverConstrained = false;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         publisher.joinFederationExecution("java-tck-query-lits-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-query-lits-receiver", federation);
         receiverJoined = true;

         InteractionClassHandle publisherInteraction =
            publisher.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle receiverInteraction =
            receiver.getInteractionClassHandle(interactionClassName);
         ParameterHandle publisherParameter =
            publisher.getParameterHandle(publisherInteraction, parameterName);
         ParameterHandle receiverParameter =
            receiver.getParameterHandle(receiverInteraction, parameterName);
         publisher.publishInteractionClass(publisherInteraction);
         published = true;
         publisher.changeInteractionOrderType(publisherInteraction, OrderType.TIMESTAMP);
         receiver.subscribeInteractionClass(receiverInteraction);
         subscribed = true;

         LogicalTimeFactory<?, ?> publisherTimeFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverTimeFactory = receiver.getTimeFactory();
         JavaTckSupport.check(publisherTimeFactory.getName().equals(
               receiverTimeFactory.getName()),
            "source and receiver selected different logical-time factories");
         LogicalTime initial = publisher.queryLogicalTime();
         LogicalTimeInterval epsilon =
            (LogicalTimeInterval) publisherTimeFactory.makeEpsilon();
         receiver.enableTimeConstrained();
         receiverConstrained = true;
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRecorder.timeConstrainedEnabled == 1,
            "receiver did not complete Time Constrained enablement");
         publisher.enableTimeRegulation(epsilon);
         publisherRegulating = true;
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(publisherRecorder.timeRegulationEnabled == 1,
            "publisher did not complete Time Regulation enablement");

         LogicalTime expectedActiveBound = add(initial, epsilon);
         expectBoundary(receiver, expectedActiveBound, "active-source GALT", true);
         expectBoundary(receiver, expectedActiveBound, "active-source LITS", false);

         LogicalTime messageTime = add(initial, offset(epsilon, 5));
         byte[] payload = new byte[] {0x51, 0x4c, 0x49, 0x54, 0x53};
         byte[] tag = new byte[] {0x51, 0x55, 0x45, 0x55, 0x45};
         ParameterHandleValueMap parameters =
            publisher.getParameterHandleValueMapFactory().create(1);
         parameters.put(publisherParameter, payload);
         MessageRetractionReturn sent =
            publisher.sendInteraction(publisherInteraction, parameters, tag, messageTime);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid
               && sent.handle != null,
            "timestamped interaction returned no valid retraction handle");
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRecorder.timestampedInteractions == 0,
            "queued timestamped interaction was delivered before its timestamp");

         publisher.resignFederationExecution(ResignAction.NO_ACTION);
         publisherJoined = false;
         publisherRegulating = false;
         expectUndefined(receiver, "GALT after source resignation", true);
         expectBoundary(receiver, messageTime, "queued-message LITS after source resignation",
            false);
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRecorder.timestampedInteractions == 0,
            "querying bounds delivered the queued timestamped interaction");
      } finally {
         if (receiverConstrained && receiverJoined) {
            try { receiver.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (publisherRegulating && publisherJoined) {
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (subscribed && receiverJoined) {
            try { receiver.unsubscribeInteractionClass(
               receiver.getInteractionClassHandle(interactionClassName)); }
            catch (Exception ignored) { }
         }
         if (published && publisherJoined) {
            try { publisher.unpublishInteractionClass(
               publisher.getInteractionClassHandle(interactionClassName)); }
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
            try { receiver.destroyFederationExecution(federation); }
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

   private static void expectUndefined(RTIambassador ambassador, String description,
         boolean galt) throws Exception {
      TimeQueryReturn result = galt ? ambassador.queryGALT() : ambassador.queryLITS();
      JavaTckSupport.check(!result.timeIsValid,
         description + " unexpectedly returned a defined bound");
   }

   private static void expectBoundary(RTIambassador ambassador, LogicalTime expected,
         String description, boolean galt) throws Exception {
      TimeQueryReturn result = galt ? ambassador.queryGALT() : ambassador.queryLITS();
      JavaTckSupport.check(result.timeIsValid,
         description + " unexpectedly returned an undefined bound");
      JavaTckSupport.check(expected.equals(result.time),
         description + " returned an incorrect logical time");
   }

   private static LogicalTime add(LogicalTime time, LogicalTimeInterval interval)
         throws Exception {
      return (LogicalTime) time.add(interval);
   }

   private static LogicalTimeInterval offset(LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTimeInterval result = epsilon;
      for (int i = 1; i < steps; ++i) {
         result = (LogicalTimeInterval) result.add(epsilon);
      }
      return result;
   }

   private static final class Recorder {
      private volatile int timeRegulationEnabled;
      private volatile int timeConstrainedEnabled;
      private volatile int timestampedInteractions;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) timeRegulationEnabled++;
               if ("timeConstrainedEnabled".equals(name)) timeConstrainedEnabled++;
               if ("receiveInteraction".equals(name)) timestampedInteractions++;
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK Query LITS callbacks";
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
