package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Re-evaluate ordinary interaction subscriptions at the callback boundary. */
final class DelaySubscriptionEvaluationInteractionTck {
   private DelaySubscriptionEvaluationInteractionTck() { }

   static void run(RtiFactory factory, String fom, String switchesFom,
         String timeImplementation, String interactionClassName, CallbackModel model)
         throws Exception {
      runConfiguration(factory, fom, switchesFom, timeImplementation,
         interactionClassName, model, false);
      runConfiguration(factory, fom, switchesFom, timeImplementation,
         interactionClassName, model, true);
   }

   private static void runConfiguration(RtiFactory factory, String fom, String switchesFom,
         String timeImplementation, String interactionClassName, CallbackModel model,
         boolean enabled) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder receiverEvents = new Recorder();
      String federation = "java-tck-delay-subscription-" + enabled + "-" + UUID.randomUUID();
      boolean publisherConnected = false, receiverConnected = false, created = false;
      boolean publisherJoined = false, receiverJoined = false;
      try {
         publisher.connect(noopAmbassador(), model);
         publisherConnected = true;
         receiver.connect(receiverEvents.proxy(), model);
         receiverConnected = true;
         String[] modules = enabled
            ? new String[] {fom, switchesFom}
            : new String[] {fom};
         publisher.createFederationExecution(federation, modules, timeImplementation);
         created = true;
         FederateHandle publisherHandle = publisher.joinFederationExecution(
            "java-tck-delay-subscription-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-delay-subscription-receiver", federation);
         receiverJoined = true;

         InteractionClassHandle publishedClass =
            publisher.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle subscribedClass =
            receiver.getInteractionClassHandle(interactionClassName);
         JavaTckSupport.check(publishedClass.equals(subscribedClass),
            "adapter FOM returned inconsistent interaction handles");
         JavaTckSupport.check(publisher.getDelaySubscriptionEvaluationSwitch() == enabled
               && receiver.getDelaySubscriptionEvaluationSwitch() == enabled,
            "Delay Subscription Evaluation switch did not match the composed FOM");
         publisher.publishInteractionClass(publishedClass);

         if (model == CallbackModel.HLA_IMMEDIATE) receiver.disableCallbacks();
         publisher.sendInteraction(publishedClass, emptyParameters(publisher), new byte[0]);
         receiver.subscribeInteractionClass(subscribedClass);
         callbackBoundary(receiver, model);
         ReceivedInteraction first = receiverEvents.lastOrNull();
         JavaTckSupport.check(!enabled || first != null,
            "enabled Delay Subscription Evaluation discarded the pre-subscription interaction");
         if (first != null) {
            checkInteraction(first, subscribedClass, publisherHandle,
               "pre-subscription interaction");
         }

         receiverEvents.clear();
         if (model == CallbackModel.HLA_IMMEDIATE) receiver.disableCallbacks();
         publisher.sendInteraction(publishedClass, emptyParameters(publisher), new byte[0]);
         receiver.unsubscribeInteractionClass(subscribedClass);
         callbackBoundary(receiver, model);
         JavaTckSupport.check(receiverEvents.interactions().isEmpty(),
            "callback-boundary unsubscription did not suppress the queued interaction");

         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         publisher.resignFederationExecution(ResignAction.NO_ACTION);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (receiverJoined) resign(receiver);
         if (publisherJoined) resign(publisher);
         if (created) {
            try { publisher.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (receiverConnected) disconnect(receiver);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static FederateAmbassador noopAmbassador() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         (proxy, method, args) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK publisher callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) return proxy == (args == null ? null : args[0]);
            }
            return null;
         });
   }

   private static ParameterHandleValueMap emptyParameters(RTIambassador ambassador)
         throws Exception {
      return ambassador.getParameterHandleValueMapFactory().create(0);
   }

   private static void callbackBoundary(RTIambassador receiver, CallbackModel model)
         throws Exception {
      if (model == CallbackModel.HLA_IMMEDIATE) {
         receiver.enableCallbacks();
      } else {
         while (receiver.evokeCallback(0.0)) { }
      }
   }

   private static void checkInteraction(ReceivedInteraction event,
         InteractionClassHandle expectedClass, FederateHandle expectedProducer,
         String description) {
      JavaTckSupport.check(expectedClass.equals(event.interaction)
            && event.parameters.isEmpty()
            && event.tag.length == 0
            && expectedProducer.equals(event.producer)
            && event.transportation != null
            && event.regionsEmpty,
         description + " callback metadata did not match the standard delivery");
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static final class ReceivedInteraction {
      final InteractionClassHandle interaction;
      final ParameterHandleValueMap parameters;
      final byte[] tag;
      final TransportationTypeHandle transportation;
      final FederateHandle producer;
      final boolean regionsEmpty;
      ReceivedInteraction(Object[] args) {
         interaction = (InteractionClassHandle) args[0];
         parameters = ((ParameterHandleValueMap) args[1]).clone();
         tag = ((byte[]) args[2]).clone();
         transportation = (TransportationTypeHandle) args[3];
         producer = (FederateHandle) args[4];
         regionsEmpty = args.length != 6 || args[5] == null
            || ((RegionHandleSet) args[5]).isEmpty();
      }
   }

   private static final class Recorder {
      private final List<ReceivedInteraction> interactions = new CopyOnWriteArrayList<>();
      FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, args) -> {
               Object[] a = args == null ? new Object[0] : args;
               if ("receiveInteraction".equals(method.getName()) && a.length == 6) {
                  interactions.add(new ReceivedInteraction(a));
               } else if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(method.getName())) return "Java TCK delayed subscriptions";
                  if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
                  if ("equals".equals(method.getName())) return proxy == a[0];
               }
               return null;
            });
      }
      List<ReceivedInteraction> interactions() { return List.copyOf(interactions); }
      ReceivedInteraction lastOrNull() {
         return interactions.isEmpty() ? null : interactions.get(interactions.size() - 1);
      }
      void clear() { interactions.clear(); }
   }
}
