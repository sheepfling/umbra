package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks interaction transportation query metadata and its membership boundary. */
final class InteractionTransportationQueryAfterResignationTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private InteractionTransportationQueryAfterResignationTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, CallbackModel callbackModel) throws Exception {
      RTIambassador requester = factory.getRtiAmbassador();
      RTIambassador publisher = factory.getRtiAmbassador();
      Recorder callbacks = new Recorder();
      String federation = "java-tck-interaction-transport-query-" + UUID.randomUUID();
      boolean requesterConnected = false, publisherConnected = false, created = false;
      boolean requesterJoined = false, publisherJoined = false, published = false;
      try {
         requester.connect(callbacks.proxy(), callbackModel);
         requesterConnected = true;
         publisher.connect(callbacks.proxy(), callbackModel);
         publisherConnected = true;
         requester.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle requesterHandle = requester.joinFederationExecution(
            "java-tck-transport-query-requester", federation);
         requesterJoined = true;
         FederateHandle publisherHandle = publisher.joinFederationExecution(
            "java-tck-transport-query-publisher", federation);
         publisherJoined = true;

         InteractionClassHandle requesterClass =
            requester.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle publisherClass =
            publisher.getInteractionClassHandle(interactionClassName);
         JavaTckSupport.check(requesterClass != null && publisherClass != null
               && interactionClassName.equals(requester.getInteractionClassName(requesterClass))
               && interactionClassName.equals(publisher.getInteractionClassName(publisherClass))
               && publisherHandle != null,
            "adapter FOM did not resolve the interaction class and active publisher handles");

         publisher.publishInteractionClass(publisherClass);
         published = true;
         callbacks.interactionReports.clear();
         requester.queryInteractionTransportationType(publisherHandle, requesterClass);
         await(requester, callbackModel,
            () -> !callbacks.interactionReports.isEmpty(),
            "valid interaction transportation report");
         JavaTckSupport.check(callbacks.interactionReports.size() == 1,
            "interaction transportation query returned an unexpected report count");
         InteractionTransportReport report = callbacks.interactionReports.get(0);
         JavaTckSupport.check(publisherHandle.equals(report.federate)
               && requesterClass.equals(report.interaction)
               && report.transportation != null
               && !requester.getTransportationTypeName(report.transportation).isEmpty(),
            "interaction transportation report returned the wrong peer, class, or type");

         requester.resignFederationExecution(ResignAction.NO_ACTION);
         requesterJoined = false;
         JavaTckSupport.expectFailure(
            () -> requester.queryInteractionTransportationType(publisherHandle, requesterClass),
            "FederateNotExecutionMember",
            "querying a valid active peer and interaction class after resignation");

         publisher.unpublishInteractionClass(publisherClass);
         published = false;
         publisher.resignFederationExecution(ResignAction.NO_ACTION);
         publisherJoined = false;
         requester.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (requesterJoined) {
            try { requester.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (publisherJoined && published) {
            try {
               publisher.unpublishInteractionClass(
                  publisher.getInteractionClassHandle(interactionClassName));
            } catch (Exception ignored) { }
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { requester.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
         if (requesterConnected) {
            try { requester.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static final class InteractionTransportReport {
      private final FederateHandle federate;
      private final InteractionClassHandle interaction;
      private final TransportationTypeHandle transportation;

      private InteractionTransportReport(FederateHandle federate,
            InteractionClassHandle interaction, TransportationTypeHandle transportation) {
         this.federate = federate;
         this.interaction = interaction;
         this.transportation = transportation;
      }
   }

   private static final class Recorder {
      private final List<InteractionTransportReport> interactionReports =
         new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("reportInteractionTransportationType".equals(name)) {
                  interactionReports.add(new InteractionTransportReport(
                     (FederateHandle) arguments[0], (InteractionClassHandle) arguments[1],
                     (TransportationTypeHandle) arguments[2]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK interaction transport query callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
