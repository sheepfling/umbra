package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** One full interaction-class unpublication does not withdraw another class. */
final class InteractionFullUnpublicationTck {
   private InteractionFullUnpublicationTck() { }

   static void run(RtiFactory factory, String fom, String firstName,
         String firstParameterName, String secondName, String secondParameterName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      String federation = "java-tck-interaction-full-unpublication-" + UUID.randomUUID();
      boolean publisherConnected = false, subscriberConnected = false, created = false;
      boolean publisherJoined = false, subscriberJoined = false;
      boolean firstPublished = false, secondPublished = false;
      boolean firstSubscribed = false, secondSubscribed = false;
      InteractionClassHandle first = null, second = null;
      InteractionClassHandle receivedFirst = null, receivedSecond = null;
      try {
         publisher.connect(emptyAmbassador(), callbackModel);
         publisherConnected = true;
         subscriber.connect(recorder.proxy(), callbackModel);
         subscriberConnected = true;
         publisher.createFederationExecution(federation, fom);
         created = true;
         publisher.joinFederationExecution("java-tck-full-unpublication-publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution("java-tck-full-unpublication-subscriber", federation);
         subscriberJoined = true;

         first = publisher.getInteractionClassHandle(firstName);
         second = publisher.getInteractionClassHandle(secondName);
         receivedFirst = subscriber.getInteractionClassHandle(firstName);
         receivedSecond = subscriber.getInteractionClassHandle(secondName);
         JavaTckSupport.check(!first.equals(second) && first.equals(receivedFirst)
               && second.equals(receivedSecond),
            "adapter FOM did not provide two distinct stable interaction classes");
         ParameterHandle firstParameter = publisher.getParameterHandle(first, firstParameterName);
         ParameterHandle secondParameter = publisher.getParameterHandle(second, secondParameterName);
         ParameterHandle receivedFirstParameter =
            subscriber.getParameterHandle(receivedFirst, firstParameterName);
         ParameterHandle receivedSecondParameter =
            subscriber.getParameterHandle(receivedSecond, secondParameterName);

         publisher.publishInteractionClass(first);
         firstPublished = true;
         publisher.publishInteractionClass(second);
         secondPublished = true;
         subscriber.subscribeInteractionClass(receivedFirst);
         firstSubscribed = true;
         subscriber.subscribeInteractionClass(receivedSecond);
         secondSubscribed = true;

         send(publisher, first, firstParameter, (byte) 0xa1);
         send(publisher, second, secondParameter, (byte) 0xb1);
         awaitCount(subscriber, recorder, 2, "baseline deliveries for both classes");
         JavaTckSupport.check(recorder.interactions.size() == 2
               && contains(recorder.interactions, receivedFirst, receivedFirstParameter, (byte) 0xa1)
               && contains(recorder.interactions, receivedSecond, receivedSecondParameter, (byte) 0xb1),
            "baseline did not deliver both interaction classes and parameters");

         recorder.interactions.clear();
         publisher.unpublishInteractionClass(first);
         firstPublished = false;
         InteractionClassHandle withdrawnFirst = first;
         JavaTckSupport.expectFailure(
            () -> send(publisher, withdrawnFirst, firstParameter, (byte) 0xa2),
            "InteractionClassNotPublished", "send after full unpublication of first class");
         send(publisher, second, secondParameter, (byte) 0xb2);
         awaitCount(subscriber, recorder, 1, "remaining published class delivery");
         JavaTckSupport.check(recorder.interactions.size() == 1
               && contains(recorder.interactions, receivedSecond, receivedSecondParameter, (byte) 0xb2),
            "withdrawn class arrived or the still-published class stopped working");

         recorder.interactions.clear();
         publisher.publishInteractionClass(first);
         firstPublished = true;
         send(publisher, first, firstParameter, (byte) 0xa3);
         awaitCount(subscriber, recorder, 1, "republished first class delivery");
         JavaTckSupport.check(recorder.interactions.size() == 1
               && contains(recorder.interactions, receivedFirst, receivedFirstParameter, (byte) 0xa3),
            "republishing did not restore first-class delivery");

         recorder.interactions.clear();
         publisher.unpublishInteractionClass(second);
         secondPublished = false;
         InteractionClassHandle withdrawnSecond = second;
         JavaTckSupport.expectFailure(
            () -> send(publisher, withdrawnSecond, secondParameter, (byte) 0xb3),
            "InteractionClassNotPublished", "send after full unpublication of second class");
         JavaTckSupport.drain(subscriber);
         JavaTckSupport.check(recorder.interactions.isEmpty(),
            "rejected send after second unpublication produced a callback");
      } finally {
         if (secondSubscribed && subscriberJoined) {
            try { subscriber.unsubscribeInteractionClass(receivedSecond); }
            catch (Exception ignored) { }
         }
         if (firstSubscribed && subscriberJoined) {
            try { subscriber.unsubscribeInteractionClass(receivedFirst); }
            catch (Exception ignored) { }
         }
         if (secondPublished && publisherJoined) {
            try { publisher.unpublishInteractionClass(second); }
            catch (Exception ignored) { }
         }
         if (firstPublished && publisherJoined) {
            try { publisher.unpublishInteractionClass(first); }
            catch (Exception ignored) { }
         }
         if (subscriberJoined) {
            try { subscriber.resignFederationExecution(ResignAction.NO_ACTION); }
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
         if (subscriberConnected) {
            try { subscriber.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void send(RTIambassador publisher, InteractionClassHandle interaction,
         ParameterHandle parameter, byte payload) throws Exception {
      ParameterHandleValueMap values = publisher.getParameterHandleValueMapFactory().create(1);
      values.put(parameter, new byte[] {payload});
      publisher.sendInteraction(interaction, values, new byte[] {0x55, payload});
   }

   private static boolean contains(List<Received> interactions, InteractionClassHandle interaction,
         ParameterHandle parameter, byte payload) {
      for (Received received : interactions) {
         if (interaction.equals(received.interaction) && received.parameters.size() == 1
               && Arrays.equals(new byte[] {payload}, received.parameters.get(parameter))
               && Arrays.equals(new byte[] {0x55, payload}, received.tag)) return true;
      }
      return false;
   }

   private static void awaitCount(RTIambassador subscriber, Recorder recorder, int count,
         String phase) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && recorder.interactions.size() < count) {
         JavaTckSupport.drain(subscriber);
         Thread.yield();
      }
      JavaTckSupport.check(recorder.interactions.size() >= count,
         phase + " did not arrive within 10 seconds");
   }

   private static final class Received {
      private InteractionClassHandle interaction;
      private ParameterHandleValueMap parameters;
      private byte[] tag;
   }

   private static final class Recorder {
      private final List<Received> interactions = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("receiveInteraction".equals(method.getName())
                     && arguments != null && arguments.length == 6) {
                  Received received = new Received();
                  received.interaction = (InteractionClassHandle) arguments[0];
                  received.parameters = ((ParameterHandleValueMap) arguments[1]).clone();
                  received.tag = ((byte[]) arguments[2]).clone();
                  interactions.add(received);
               }
               return objectMethod(proxy, method.getName(), arguments);
            });
      }
   }

   private static FederateAmbassador emptyAmbassador() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> objectMethod(proxy, method.getName(), arguments));
   }

   private static Object objectMethod(Object proxy, String method, Object[] arguments) {
      if ("toString".equals(method)) return "Java TCK interaction full unpublication callbacks";
      if ("hashCode".equals(method)) return System.identityHashCode(proxy);
      if ("equals".equals(method)) return proxy == (arguments == null ? null : arguments[0]);
      return null;
   }
}
