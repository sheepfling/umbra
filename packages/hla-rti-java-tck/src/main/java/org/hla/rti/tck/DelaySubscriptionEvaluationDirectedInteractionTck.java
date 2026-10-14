package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Re-evaluate directed-interaction subscriptions at the callback boundary. */
final class DelaySubscriptionEvaluationDirectedInteractionTck {
   private DelaySubscriptionEvaluationDirectedInteractionTck() { }

   static void run(RtiFactory factory, String fom, String switchesFom,
         String timeImplementation, String objectClassName, String attributeName,
         String interactionClassName, CallbackModel model) throws Exception {
      runConfiguration(factory, fom, switchesFom, timeImplementation, objectClassName,
         attributeName, interactionClassName, model, false);
      runConfiguration(factory, fom, switchesFom, timeImplementation, objectClassName,
         attributeName, interactionClassName, model, true);
   }

   private static void runConfiguration(RtiFactory factory, String fom, String switchesFom,
         String timeImplementation, String objectClassName, String attributeName,
         String interactionClassName, CallbackModel model, boolean enabled) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder events = new Recorder();
      String federation = "java-tck-delay-subscription-directed-" + enabled + "-" + UUID.randomUUID();
      boolean ownerConnected = false, receiverConnected = false, created = false;
      boolean ownerJoined = false, receiverJoined = false;
      try {
         owner.connect(noopAmbassador(), model);
         ownerConnected = true;
         receiver.connect(events.proxy(), model);
         receiverConnected = true;
         String[] modules = enabled ? new String[] {fom, switchesFom} : new String[] {fom};
         owner.createFederationExecution(federation, modules, timeImplementation);
         created = true;
         FederateHandle producer = owner.joinFederationExecution(
            "java-tck-delay-directed-owner", federation);
         ownerJoined = true;
         receiver.joinFederationExecution("java-tck-delay-directed-receiver", federation);
         receiverJoined = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle receiverAttribute = receiver.getAttributeHandle(receiverClass, attributeName);
         InteractionClassHandle ownerInteraction =
            owner.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle receiverInteraction =
            receiver.getInteractionClassHandle(interactionClassName);
         JavaTckSupport.check(ownerClass.equals(receiverClass)
               && ownerAttribute.equals(receiverAttribute)
               && ownerInteraction.equals(receiverInteraction),
            "adapter FOM returned inconsistent directed-interaction handles");
         JavaTckSupport.check(owner.getDelaySubscriptionEvaluationSwitch() == enabled
               && receiver.getDelaySubscriptionEvaluationSwitch() == enabled,
            "Delay Subscription Evaluation switch did not match the composed FOM");

         owner.publishObjectClassAttributes(ownerClass,
            JavaTckSupport.attributeSet(owner, ownerAttribute));
         receiver.subscribeObjectClassAttributes(receiverClass,
            JavaTckSupport.attributeSet(receiver, receiverAttribute));
         owner.publishObjectClassDirectedInteractions(ownerClass,
            JavaTckSupport.interactionSet(owner, ownerInteraction));
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(receiver);
         JavaTckSupport.check(events.discoveries.contains(object)
               && receiverClass.equals(receiver.getKnownObjectClassHandle(object)),
            "receiver did not discover the target object before directed delivery");

         if (model == CallbackModel.HLA_IMMEDIATE) receiver.disableCallbacks();
         owner.sendDirectedInteraction(ownerInteraction, object,
            owner.getParameterHandleValueMapFactory().create(0), new byte[0]);
         receiver.subscribeObjectClassDirectedInteractions(receiverClass,
            JavaTckSupport.interactionSet(receiver, receiverInteraction));
         callbackBoundary(receiver, model);

         Directed first = events.lastOrNull();
         JavaTckSupport.check(enabled ? events.directed.size() == 1
               : events.directed.size() <= 1,
            "unexpected first directed-interaction delivery count");
         if (first != null) {
            verify(first, receiverInteraction, object, producer, "pre-subscription send");
         }

         events.clearDirected();
         if (model == CallbackModel.HLA_IMMEDIATE) receiver.disableCallbacks();
         owner.sendDirectedInteraction(ownerInteraction, object,
            owner.getParameterHandleValueMapFactory().create(0), new byte[0]);
         receiver.unsubscribeObjectClassDirectedInteractions(receiverClass,
            JavaTckSupport.interactionSet(receiver, receiverInteraction));
         callbackBoundary(receiver, model);
         JavaTckSupport.check(events.directed.isEmpty(),
            "callback-boundary directed unsubscribe did not suppress delivery");

         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         owner.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         ownerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (receiverJoined) resign(receiver);
         if (ownerJoined) resignDelete(owner);
         if (created) {
            try { owner.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (receiverConnected) disconnect(receiver);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static void verify(Directed event, InteractionClassHandle interaction,
         ObjectInstanceHandle object, FederateHandle producer, String phase) {
      JavaTckSupport.check(interaction.equals(event.interaction)
            && object.equals(event.target)
            && event.parameters.isEmpty()
            && event.tag.length == 0
            && producer.equals(event.producer)
            && event.transportation != null,
         phase + " directed callback lost standard delivery metadata");
   }

   private static void callbackBoundary(RTIambassador receiver, CallbackModel model)
         throws Exception {
      if (model == CallbackModel.HLA_IMMEDIATE) receiver.enableCallbacks();
      else while (receiver.evokeCallback(0.0)) { }
   }

   private static FederateAmbassador noopAmbassador() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, args) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK directed publisher callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) return proxy == (args == null ? null : args[0]);
            }
            return null;
         });
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void resignDelete(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static final class Directed {
      final InteractionClassHandle interaction;
      final ObjectInstanceHandle target;
      final ParameterHandleValueMap parameters;
      final byte[] tag;
      final TransportationTypeHandle transportation;
      final FederateHandle producer;
      Directed(Object[] args) {
         interaction = (InteractionClassHandle) args[0];
         target = (ObjectInstanceHandle) args[1];
         parameters = ((ParameterHandleValueMap) args[2]).clone();
         tag = ((byte[]) args[3]).clone();
         transportation = (TransportationTypeHandle) args[4];
         producer = (FederateHandle) args[5];
      }
   }

   private static final class Recorder {
      final List<Directed> directed = new CopyOnWriteArrayList<>();
      final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, args) -> {
               Object[] a = args == null ? new Object[0] : args;
               if ("discoverObjectInstance".equals(method.getName()) && a.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) a[0]);
               } else if ("receiveDirectedInteraction".equals(method.getName()) && a.length == 6) {
                  directed.add(new Directed(a));
               } else if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(method.getName())) return "Java TCK directed DSE callbacks";
                  if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
                  if ("equals".equals(method.getName())) return proxy == a[0];
               }
               return null;
            });
      }
      Directed lastOrNull() { return directed.isEmpty() ? null : directed.get(directed.size() - 1); }
      void clearDirected() { directed.clear(); }
   }
}
