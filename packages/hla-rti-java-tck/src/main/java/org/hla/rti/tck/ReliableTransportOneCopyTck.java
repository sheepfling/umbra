package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.HashSet;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verifies one receive-order callback per HLAreliable interaction send. */
final class ReliableTransportOneCopyTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final long SETTLE_NANOS = 40_000_000L;

   private ReliableTransportOneCopyTck() { }

   static void run(RtiFactory factory, String fom, String interactionName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador sender = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder senderCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      String federation = "java-tck-reliable-one-copy-" + UUID.randomUUID();
      String senderName = "java-tck-reliable-one-copy-sender";
      String receiverName = "java-tck-reliable-one-copy-receiver";
      boolean senderConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean senderJoined = false;
      boolean receiverJoined = false;
      boolean senderPublished = false;
      boolean receiverSubscribed = false;
      InteractionClassHandle senderInteraction = null;
      InteractionClassHandle receiverInteraction = null;
      try {
         sender.connect(senderCallbacks.proxy(), callbackModel);
         senderConnected = true;
         receiver.connect(receiverCallbacks.proxy(), callbackModel);
         receiverConnected = true;
         sender.createFederationExecution(federation, fom);
         created = true;

         FederateHandle senderHandle = sender.joinFederationExecution(
            senderName, "JavaTck", federation);
         senderJoined = true;
         JavaTckSupport.check(senderName.equals(sender.getFederateName(senderHandle)),
            "sender federate name did not resolve through its own RTIambassador");
         receiver.joinFederationExecution(receiverName, "JavaTck", federation);
         receiverJoined = true;

         senderInteraction = sender.getInteractionClassHandle(interactionName);
         receiverInteraction = receiver.getInteractionClassHandle(interactionName);
         JavaTckSupport.check(interactionName.equals(
               sender.getInteractionClassName(senderInteraction))
               && interactionName.equals(receiver.getInteractionClassName(receiverInteraction)),
            "interaction class name did not round-trip through each member's RTIambassador");
         TransportationTypeHandle senderReliable =
            sender.getTransportationTypeHandle("HLAreliable");
         TransportationTypeHandle receiverReliable =
            receiver.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check("HLAreliable".equals(
               sender.getTransportationTypeName(senderReliable))
               && "HLAreliable".equals(receiver.getTransportationTypeName(receiverReliable)),
            "the standard reliable transportation type was not available to both members");

         sender.publishInteractionClass(senderInteraction);
         senderPublished = true;
         receiver.subscribeInteractionClass(receiverInteraction);
         receiverSubscribed = true;

         byte[][] expectedTags = {{0x31}, {0x32}, {0x33}};
         ParameterHandleValueMap parameters =
            sender.getParameterHandleValueMapFactory().create(0);
         Set<Byte> expected = new HashSet<>();
         for (byte[] tag : expectedTags) {
            JavaTckSupport.check(tag.length == 1 && expected.add(tag[0]),
               "reliable one-copy fixture tags must be distinct single-byte values");
            sender.sendInteraction(senderInteraction, parameters, tag.clone());
         }

         awaitCount(sender, receiver, receiverCallbacks, callbackModel, expectedTags.length);
         settle(sender, receiver, callbackModel);
         JavaTckSupport.check(receiverCallbacks.interactions.size() == expectedTags.length,
            "HLAreliable did not deliver exactly one callback per sent interaction");

         Set<Byte> delivered = new HashSet<>();
         for (Delivery event : receiverCallbacks.interactions) {
            JavaTckSupport.check(receiverInteraction.equals(event.interaction)
                  && event.transportation != null
                  && receiverReliable.equals(event.transportation)
                  && "HLAreliable".equals(
                     receiver.getTransportationTypeName(event.transportation))
                  && senderName.equals(receiver.getFederateName(event.producer))
                  && event.regionsEmpty,
               "reliable callback changed its interaction, sender, transport, or region metadata");
            JavaTckSupport.check(event.tag.length == 1 && delivered.add(event.tag[0]),
               "reliable transport delivered a duplicate or malformed interaction tag");
         }
         JavaTckSupport.check(delivered.equals(expected),
            "reliable transport did not deliver every sent interaction exactly once");
         JavaTckSupport.check(senderCallbacks.interactions.isEmpty(),
            "unsubscribed sender received an interaction callback");
      } finally {
         if (receiverSubscribed && receiverJoined && receiverInteraction != null) {
            try { receiver.unsubscribeInteractionClass(receiverInteraction); }
            catch (Exception ignored) { }
         }
         if (senderPublished && senderJoined && senderInteraction != null) {
            try { sender.unpublishInteractionClass(senderInteraction); }
            catch (Exception ignored) { }
         }
         if (receiverJoined) {
            try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (senderJoined) {
            try { sender.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { sender.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (receiverConnected) {
            try { receiver.disconnect(); } catch (Exception ignored) { }
         }
         if (senderConnected) {
            try { sender.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void awaitCount(RTIambassador sender, RTIambassador receiver,
         Recorder callbacks,
         CallbackModel callbackModel, int expectedCount) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (callbacks.interactions.size() < expectedCount && System.nanoTime() < deadline) {
         serviceCallbacks(sender, callbackModel);
         serviceCallbacks(receiver, callbackModel);
      }
      JavaTckSupport.check(callbacks.interactions.size() >= expectedCount,
         "one reliable callback per sent interaction did not arrive before the deadline");
   }

   private static void settle(RTIambassador sender, RTIambassador receiver,
         CallbackModel callbackModel)
         throws Exception {
      long deadline = System.nanoTime() + SETTLE_NANOS;
      while (System.nanoTime() < deadline) {
         serviceCallbacks(sender, callbackModel);
         serviceCallbacks(receiver, callbackModel);
      }
   }

   private static void serviceCallbacks(RTIambassador receiver, CallbackModel callbackModel)
         throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) {
         JavaTckSupport.drain(receiver);
      } else {
         Thread.yield();
      }
   }

   private static final class Delivery {
      private final InteractionClassHandle interaction;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final boolean regionsEmpty;

      private Delivery(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         tag = ((byte[]) arguments[2]).clone();
         transportation = (TransportationTypeHandle) arguments[3];
         producer = (FederateHandle) arguments[4];
         regionsEmpty = arguments[5] == null || ((RegionHandleSet) arguments[5]).isEmpty();
      }
   }

   private static final class Recorder {
      private final CopyOnWriteArrayList<Delivery> interactions = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length == 6) {
                  interactions.add(new Delivery(arguments));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK reliable one-copy callbacks";
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
