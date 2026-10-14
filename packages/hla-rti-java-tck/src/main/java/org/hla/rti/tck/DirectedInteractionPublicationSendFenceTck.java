package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Whole-class directed publication fences sends until that publication is restored. */
final class DirectedInteractionPublicationSendFenceTck {
   private DirectedInteractionPublicationSendFenceTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         String parameterName, CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-directed-publication-fence-" + UUID.randomUUID();
      boolean publisherConnected = false, receiverConnected = false, created = false;
      boolean publisherJoined = false, receiverJoined = false;
      boolean publisherAttributesPublished = false, receiverAttributesPublished = false;
      boolean publisherAttributesSubscribed = false, receiverAttributesSubscribed = false;
      boolean directedPublished = false, directedSubscribed = false;
      ObjectClassHandle publisherClass = null, receiverClass = null;
      AttributeHandleSet publisherAttributes = null, receiverAttributes = null;
      InteractionClassHandle publisherInteraction = null, receiverInteraction = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-directed-publication-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-directed-publication-receiver", federation);
         receiverJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute =
            publisher.getAttributeHandle(publisherClass, attributeName);
         AttributeHandle receiverAttribute =
            receiver.getAttributeHandle(receiverClass, attributeName);
         publisherAttributes = JavaTckSupport.attributeSet(publisher, publisherAttribute);
         receiverAttributes = JavaTckSupport.attributeSet(receiver, receiverAttribute);
         publisherInteraction = publisher.getInteractionClassHandle(interactionClassName);
         receiverInteraction = receiver.getInteractionClassHandle(interactionClassName);
         ParameterHandle publisherParameter =
            publisher.getParameterHandle(publisherInteraction, parameterName);
         ParameterHandle receiverParameter =
            receiver.getParameterHandle(receiverInteraction, parameterName);
         JavaTckSupport.check(publisherClass.equals(receiverClass)
               && publisherAttribute.equals(receiverAttribute)
               && publisherInteraction.equals(receiverInteraction)
               && publisherParameter.equals(receiverParameter),
            "directed publication fence lookups returned inconsistent handles");

         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         publisherAttributesPublished = true;
         receiver.publishObjectClassAttributes(receiverClass, receiverAttributes);
         receiverAttributesPublished = true;
         publisher.subscribeObjectClassAttributes(publisherClass, publisherAttributes);
         publisherAttributesSubscribed = true;
         receiver.subscribeObjectClassAttributes(receiverClass, receiverAttributes);
         receiverAttributesSubscribed = true;
         publisher.publishObjectClassDirectedInteractions(publisherClass,
            JavaTckSupport.interactionSet(publisher, publisherInteraction));
         directedPublished = true;
         receiver.subscribeObjectClassDirectedInteractions(receiverClass,
            JavaTckSupport.interactionSet(receiver, receiverInteraction));
         directedSubscribed = true;

         ObjectInstanceHandle target = receiver.registerObjectInstance(receiverClass);
         await(publisher, callbackModel, objectClassName,
            () -> publisherRecorder.discoveries.contains(target),
            "directed publication fence target discovery");
         JavaTckSupport.check(publisher.getKnownObjectClassHandle(target).equals(publisherClass),
            "publisher learned the wrong directed target class");

         byte[] initialValue = new byte[] {0x44, 0x49, 0x52};
         byte[] initialTag = new byte[] {0x49, 0x4e, 0x49};
         send(publisher, publisherInteraction, target, publisherParameter, initialValue, initialTag);
         await(receiver, callbackModel, objectClassName,
            () -> receiverRecorder.directed.size() == 1,
            "initial directed interaction delivery");
         verify(receiver, receiverRecorder.directed.get(0), receiverInteraction,
            receiverParameter, target, initialValue, initialTag, producer, "initial");

         publisher.unpublishObjectClassDirectedInteractions(publisherClass);
         directedPublished = false;
         byte[] fencedValue = new byte[] {0x46, 0x45, 0x4e, 0x43, 0x45};
         byte[] fencedTag = new byte[] {0x46, 0x45, 0x4e};
         InteractionClassHandle unpublishedInteraction = publisherInteraction;
         JavaTckSupport.expectFailure(
            () -> send(publisher, unpublishedInteraction, target, publisherParameter,
               fencedValue, fencedTag),
            "InteractionClassNotPublished", "unpublished directed send was accepted");
         pump(receiver, callbackModel, objectClassName, 8);
         JavaTckSupport.check(receiverRecorder.directed.size() == 1,
            "unpublished directed interaction unexpectedly reached the receiver");

         publisher.publishObjectClassDirectedInteractions(publisherClass,
            JavaTckSupport.interactionSet(publisher, publisherInteraction));
         directedPublished = true;
         byte[] restoredValue = new byte[] {0x52, 0x45, 0x53, 0x54, 0x4f, 0x52, 0x45};
         byte[] restoredTag = new byte[] {0x52, 0x45, 0x50};
         send(publisher, publisherInteraction, target, publisherParameter, restoredValue,
            restoredTag);
         await(receiver, callbackModel, objectClassName,
            () -> receiverRecorder.directed.size() == 2,
            "directed delivery after republication");
         verify(receiver, receiverRecorder.directed.get(1), receiverInteraction,
            receiverParameter, target, restoredValue, restoredTag, producer, "republished");
      } finally {
         if (directedSubscribed && receiverJoined) {
            try { receiver.unsubscribeObjectClassDirectedInteractions(receiverClass); }
            catch (Exception ignored) { }
         }
         if (directedPublished && publisherJoined) {
            try { publisher.unpublishObjectClassDirectedInteractions(publisherClass); }
            catch (Exception ignored) { }
         }
         if (receiverAttributesSubscribed && receiverJoined) {
            try { receiver.unsubscribeObjectClassAttributes(receiverClass, receiverAttributes); }
            catch (Exception ignored) { }
         }
         if (publisherAttributesSubscribed && publisherJoined) {
            try { publisher.unsubscribeObjectClassAttributes(publisherClass, publisherAttributes); }
            catch (Exception ignored) { }
         }
         if (receiverAttributesPublished && receiverJoined) {
            try { receiver.unpublishObjectClassAttributes(receiverClass, receiverAttributes); }
            catch (Exception ignored) { }
         }
         if (publisherAttributesPublished && publisherJoined) {
            try { publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes); }
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

   private static void send(RTIambassador publisher, InteractionClassHandle interaction,
         ObjectInstanceHandle target, ParameterHandle parameter, byte[] value, byte[] tag)
         throws Exception {
      ParameterHandleValueMap parameters =
         publisher.getParameterHandleValueMapFactory().create(1);
      parameters.put(parameter, value);
      publisher.sendDirectedInteraction(interaction, target, parameters, tag);
   }

   private static void verify(RTIambassador receiver, Directed received,
         InteractionClassHandle interaction, ParameterHandle parameter,
         ObjectInstanceHandle target, byte[] value, byte[] tag, FederateHandle producer,
         String phase) throws Exception {
      JavaTckSupport.check(interaction.equals(received.interaction)
            && target.equals(received.target)
            && received.parameters.size() == 1
            && Arrays.equals(value, received.parameters.get(parameter))
            && Arrays.equals(tag, received.tag)
            && producer.equals(received.producer)
            && received.transportation != null
            && "HLAreliable".equals(receiver.getTransportationTypeName(received.transportation)),
         phase + " directed callback lost target or standard delivery data");
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         String objectClassName, Condition condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && !condition.isTrue()) {
         service(ambassador, model, objectClassName);
         Thread.yield();
      }
      JavaTckSupport.check(condition.isTrue(), "timed out waiting for " + description);
   }

   private static void pump(RTIambassador ambassador, CallbackModel model,
         String objectClassName, int passes) throws Exception {
      for (int i = 0; i < passes; ++i) service(ambassador, model, objectClassName);
   }

   private static void service(RTIambassador ambassador, CallbackModel model,
         String objectClassName) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) ambassador.evokeMultipleCallbacks(0.0, 1.0);
      else ambassador.getObjectClassHandle(objectClassName);
   }

   @FunctionalInterface
   private interface Condition { boolean isTrue() throws Exception; }

   private static final class Directed {
      private final InteractionClassHandle interaction;
      private final ObjectInstanceHandle target;
      private final ParameterHandleValueMap parameters;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;

      private Directed(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         target = (ObjectInstanceHandle) arguments[1];
         parameters = ((ParameterHandleValueMap) arguments[2]).clone();
         tag = ((byte[]) arguments[3]).clone();
         transportation = (TransportationTypeHandle) arguments[4];
         producer = (FederateHandle) arguments[5];
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Directed> directed = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("receiveDirectedInteraction".equals(name) && arguments != null
                     && arguments.length == 6) {
                  directed.add(new Directed(arguments));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK directed publication fence";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
