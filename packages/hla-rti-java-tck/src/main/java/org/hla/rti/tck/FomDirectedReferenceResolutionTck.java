package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.InteractionClassHandleSet;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.UUID;

/** Later-module directed-interaction reference through the standard API. */
final class FomDirectedReferenceResolutionTck {
   private FomDirectedReferenceResolutionTck() { }

   static void run(RtiFactory factory, String objectFom, String interactionFom,
         String missing, String timeImplementation, CallbackModel callbackModel)
         throws Exception {
      RTIambassador sender = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder observation = new Recorder();
      String federation = "java-tck-directed-reference-" + UUID.randomUUID();
      boolean senderConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean senderJoined = false;
      boolean receiverJoined = false;
      try {
         sender.connect(new Recorder().proxy(), callbackModel);
         senderConnected = true;
         receiver.connect(observation.proxy(), callbackModel);
         receiverConnected = true;
         sender.createFederationExecution(federation,
            new String[] {objectFom, interactionFom}, timeImplementation);
         created = true;
         sender.joinFederationExecution("java-tck-directed-reference-sender", federation);
         senderJoined = true;
         receiver.joinFederationExecution("java-tck-directed-reference-receiver", federation);
         receiverJoined = true;

         String className = "HLAobjectRoot.TckDirectedReferenceObject";
         String interactionName = "HLAinteractionRoot.TckDirectedReferenceInteraction";
         ObjectClassHandle senderClass = sender.getObjectClassHandle(className);
         ObjectClassHandle receiverClass = receiver.getObjectClassHandle(className);
         InteractionClassHandle senderInteraction = sender.getInteractionClassHandle(
            interactionName);
         InteractionClassHandle receiverInteraction = receiver.getInteractionClassHandle(
            interactionName);
         AttributeHandle marker = sender.getAttributeHandle(senderClass, "Marker");
         JavaTckSupport.check(senderClass.equals(receiverClass) &&
               senderInteraction.equals(receiverInteraction) && marker != null,
            "later FOM module did not resolve the directed interaction and target class");
         AttributeHandleSet attributes = JavaTckSupport.attributeSet(sender, marker);
         InteractionClassHandleSet interactions = JavaTckSupport.interactionSet(
            sender, senderInteraction);
         sender.publishObjectClassAttributes(senderClass, attributes);
         receiver.subscribeObjectClassAttributes(receiverClass, attributes);
         sender.publishObjectClassDirectedInteractions(senderClass, interactions);
         receiver.subscribeObjectClassDirectedInteractions(receiverClass, interactions);
         ObjectInstanceHandle target = sender.registerObjectInstance(senderClass);
         await(receiver, callbackModel, () -> target.equals(observation.discovered),
            "directed-reference receiver did not discover the target");

         byte[] tag = new byte[] {0x44, 0x52, 0x46};
         ParameterHandleValueMap empty = sender.getParameterHandleValueMapFactory().create(0);
         sender.sendDirectedInteraction(senderInteraction, target, empty, tag);
         await(receiver, callbackModel, () -> target.equals(observation.receivedTarget),
            "later-module directed interaction was not delivered");
         JavaTckSupport.check(receiverInteraction.equals(observation.receivedClass) &&
               observation.receivedParameterCount == 0 &&
               Arrays.equals(tag, observation.receivedTag),
            "later-module directed callback did not preserve class, parameters, and tag");

         receiver.unsubscribeObjectClassDirectedInteractions(receiverClass, interactions);
         sender.unpublishObjectClassDirectedInteractions(senderClass, interactions);
         receiver.unsubscribeObjectClassAttributes(receiverClass, attributes);
         sender.unpublishObjectClassAttributes(senderClass, attributes);
         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         sender.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         senderJoined = false;
         sender.destroyFederationExecution(federation);
         created = false;

         JavaTckSupport.expectFailure(
            () -> sender.createFederationExecution(federation,
               new String[] {missing}, timeImplementation),
            "InconsistentFOM", "unresolved directed-interaction class reference");
         sender.createFederationExecution(federation,
            new String[] {objectFom, interactionFom}, timeImplementation);
         created = true;
         sender.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (receiverJoined) try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (senderJoined) try { sender.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         if (created) try { sender.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (receiverConnected) try { receiver.disconnect(); }
            catch (Exception ignored) { }
         if (senderConnected) try { sender.disconnect(); }
            catch (Exception ignored) { }
      }
   }

   private interface Check { boolean ok(); }

   private static void await(RTIambassador ambassador, CallbackModel model,
         Check condition, String message) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && !condition.ok()) {
         if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         Thread.yield();
      }
      JavaTckSupport.check(condition.ok(), message);
   }

   private static final class Recorder {
      private volatile ObjectInstanceHandle discovered;
      private volatile ObjectInstanceHandle receivedTarget;
      private volatile InteractionClassHandle receivedClass;
      private volatile int receivedParameterCount;
      private volatile byte[] receivedTag;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)) {
                  discovered = (ObjectInstanceHandle) arguments[0];
               } else if ("receiveDirectedInteraction".equals(name)) {
                  receivedClass = (InteractionClassHandle) arguments[0];
                  receivedParameterCount = ((ParameterHandleValueMap) arguments[2]).size();
                  receivedTag = ((byte[]) arguments[3]).clone();
                  receivedTarget = (ObjectInstanceHandle) arguments[1];
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK directed-reference callbacks";
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
