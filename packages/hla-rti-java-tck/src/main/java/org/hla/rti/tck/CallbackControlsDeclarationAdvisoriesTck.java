package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

/** Callback enable/disable behavior for standard declaration-relevance advisories. */
final class CallbackControlsDeclarationAdvisoriesTck {
   private CallbackControlsDeclarationAdvisoriesTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder subscriberRecorder = new Recorder();
      String federation = "java-tck-callback-declarations-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean subscriberConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean subscriberJoined = false;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         subscriber.connect(subscriberRecorder.proxy(), callbackModel);
         subscriberConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         publisher.joinFederationExecution(
            "java-tck-callback-declarations-publisher", "publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution(
            "java-tck-callback-declarations-subscriber", "subscriber", federation);
         subscriberJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle subscriberClass = subscriber.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(publisherClass,
            attributeName);
         AttributeHandle subscriberAttribute = subscriber.getAttributeHandle(subscriberClass,
            attributeName);
         InteractionClassHandle publisherInteraction =
            publisher.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle subscriberInteraction =
            subscriber.getInteractionClassHandle(interactionClassName);
         JavaTckSupport.check(publisherClass.equals(subscriberClass)
               && publisherAttribute.equals(subscriberAttribute)
               && publisherInteraction.equals(subscriberInteraction),
            "standard FOM lookups returned mismatched declaration-advisory handles");

         AttributeHandleSet publisherAttributes =
            JavaTckSupport.attributeSet(publisher, publisherAttribute);
         AttributeHandleSet subscriberAttributes =
            JavaTckSupport.attributeSet(subscriber, subscriberAttribute);
         subscriber.subscribeObjectClassAttributes(subscriberClass, subscriberAttributes);
         publisher.disableCallbacks();
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         service(publisher, callbackModel, objectClassName);
         JavaTckSupport.check(publisherRecorder.registrationStarts.isEmpty(),
            "disabled callbacks exposed a start-registration advisory");

         publisher.enableCallbacks();
         await(publisher, callbackModel, objectClassName,
            () -> publisherRecorder.registrationStarts.size() == 1,
            "re-enabled start-registration advisory");
         JavaTckSupport.check(publisherClass.equals(
               publisherRecorder.registrationStarts.get(0)),
            "start-registration advisory returned the wrong object-class handle");

         publisher.disableCallbacks();
         subscriber.unsubscribeObjectClassAttributes(subscriberClass, subscriberAttributes);
         service(publisher, callbackModel, objectClassName);
         JavaTckSupport.check(publisherRecorder.registrationStops.isEmpty(),
            "disabled callbacks exposed a stop-registration advisory");
         publisher.enableCallbacks();
         await(publisher, callbackModel, objectClassName,
            () -> publisherRecorder.registrationStops.size() == 1,
            "re-enabled stop-registration advisory");
         JavaTckSupport.check(publisherClass.equals(
               publisherRecorder.registrationStops.get(0)),
            "stop-registration advisory returned the wrong object-class handle");

         publisherRecorder.clearAdvisories();
         publisher.disableCallbacks();
         subscriber.subscribeInteractionClass(subscriberInteraction);
         publisher.publishInteractionClass(publisherInteraction);
         service(publisher, callbackModel, objectClassName);
         JavaTckSupport.check(publisherRecorder.interactionsOn.isEmpty(),
            "disabled callbacks exposed an interaction turn-on advisory");
         publisher.enableCallbacks();
         await(publisher, callbackModel, objectClassName,
            () -> publisherRecorder.interactionsOn.size() == 1,
            "re-enabled interaction turn-on advisory");
         JavaTckSupport.check(publisherInteraction.equals(
               publisherRecorder.interactionsOn.get(0)),
            "interaction turn-on advisory returned the wrong interaction-class handle");

         publisher.disableCallbacks();
         subscriber.unsubscribeInteractionClass(subscriberInteraction);
         service(publisher, callbackModel, objectClassName);
         JavaTckSupport.check(publisherRecorder.interactionsOff.isEmpty(),
            "disabled callbacks exposed an interaction turn-off advisory");
         publisher.enableCallbacks();
         await(publisher, callbackModel, objectClassName,
            () -> publisherRecorder.interactionsOff.size() == 1,
            "re-enabled interaction turn-off advisory");
         JavaTckSupport.check(publisherInteraction.equals(
               publisherRecorder.interactionsOff.get(0)),
            "interaction turn-off advisory returned the wrong interaction-class handle");
      } finally {
         if (subscriberJoined) {
            try { subscriber.unsubscribeObjectClass(
               subscriber.getObjectClassHandle(objectClassName)); } catch (Exception ignored) { }
            resign(subscriber);
         }
         if (publisherJoined) {
            try { publisher.unpublishInteractionClass(
               publisher.getInteractionClassHandle(interactionClassName)); }
            catch (Exception ignored) { }
            try {
               publisher.unpublishObjectClassAttributes(
                  publisher.getObjectClassHandle(objectClassName),
                  JavaTckSupport.attributeSet(publisher,
                     publisher.getAttributeHandle(publisher.getObjectClassHandle(objectClassName),
                        attributeName)));
            } catch (Exception ignored) { }
            resign(publisher);
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (subscriberConnected) disconnect(subscriber);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         String objectClassName, Condition condition, String description) throws Exception {
      for (int attempt = 0; attempt < 1000 && !condition.isTrue(); attempt++) {
         service(ambassador, model, objectClassName);
         Thread.sleep(10);
      }
      JavaTckSupport.check(condition.isTrue(), "timed out waiting for " + description);
   }

   private static void service(RTIambassador ambassador, CallbackModel model,
         String objectClassName) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) {
         ambassador.evokeMultipleCallbacks(0.0, 0.01);
      } else {
         ambassador.getObjectClassHandle(objectClassName);
      }
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private interface Condition {
      boolean isTrue();
   }

   private static final class Recorder {
      private final List<ObjectClassHandle> registrationStarts = new ArrayList<>();
      private final List<ObjectClassHandle> registrationStops = new ArrayList<>();
      private final List<InteractionClassHandle> interactionsOn = new ArrayList<>();
      private final List<InteractionClassHandle> interactionsOff = new ArrayList<>();

      private void clearAdvisories() {
         registrationStarts.clear();
         registrationStops.clear();
         interactionsOn.clear();
         interactionsOff.clear();
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("startRegistrationForObjectClass".equals(name)) {
                  registrationStarts.add((ObjectClassHandle) arguments[0]);
               } else if ("stopRegistrationForObjectClass".equals(name)) {
                  registrationStops.add((ObjectClassHandle) arguments[0]);
               } else if ("turnInteractionsOn".equals(name)) {
                  interactionsOn.add((InteractionClassHandle) arguments[0]);
               } else if ("turnInteractionsOff".equals(name)) {
                  interactionsOff.add((InteractionClassHandle) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK declaration advisory callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
