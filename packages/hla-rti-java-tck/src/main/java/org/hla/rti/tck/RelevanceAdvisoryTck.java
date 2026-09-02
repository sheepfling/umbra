package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

/** P1.3 ordinary declaration-relevance and update-relevance callbacks. */
final class RelevanceAdvisoryTck {
   private RelevanceAdvisoryTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder subscriberRecorder = new Recorder();
      String federation = "java-tck-relevance-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean subscriberConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean subscriberJoined = false;
      try {
         publisher.connect(publisherRecorder.proxy(), CallbackModel.HLA_EVOKED);
         publisherConnected = true;
         subscriber.connect(subscriberRecorder.proxy(), CallbackModel.HLA_EVOKED);
         subscriberConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         publisher.joinFederationExecution("java-tck-relevance-publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution("java-tck-relevance-subscriber", federation);
         subscriberJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle subscriberClass = subscriber.getObjectClassHandle(objectClassName);
         AttributeHandle publisherName = publisher.getAttributeHandle(publisherClass, "Name");
         AttributeHandle publisherPayRate = publisher.getAttributeHandle(publisherClass, "PayRate");
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(publisherClass,
            attributeName);
         AttributeHandle subscriberName = subscriber.getAttributeHandle(subscriberClass, "Name");
         AttributeHandle subscriberPayRate = subscriber.getAttributeHandle(subscriberClass,
            "PayRate");
         AttributeHandle subscriberAttribute = subscriber.getAttributeHandle(subscriberClass,
            attributeName);
         AttributeHandleSet publisherNameSet = JavaTckSupport.attributeSet(publisher, publisherName);
         AttributeHandleSet publisherAllSet = JavaTckSupport.attributeSet(publisher,
            publisherName, publisherPayRate, publisherAttribute);
         AttributeHandleSet subscriberNameSet = JavaTckSupport.attributeSet(subscriber,
            subscriberName);
         AttributeHandleSet subscriberPayRateSet = JavaTckSupport.attributeSet(subscriber,
            subscriberPayRate);
         AttributeHandleSet subscriberAttributeSet = JavaTckSupport.attributeSet(subscriber,
            subscriberAttribute);
         InteractionClassHandle publisherInteraction =
            publisher.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle subscriberInteraction =
            subscriber.getInteractionClassHandle(interactionClassName);

         checkSwitches(publisher);
         publisher.setObjectClassRelevanceAdvisorySwitch(false);
         JavaTckSupport.check(!publisher.getObjectClassRelevanceAdvisorySwitch(),
            "object-class relevance advisory switch did not turn off");
         publisher.setObjectClassRelevanceAdvisorySwitch(true);
         publisher.setAttributeRelevanceAdvisorySwitch(false);
         JavaTckSupport.check(!publisher.getAttributeRelevanceAdvisorySwitch(),
            "attribute relevance advisory switch did not turn off");
         publisher.setAttributeRelevanceAdvisorySwitch(true);
         publisher.setInteractionRelevanceAdvisorySwitch(false);
         JavaTckSupport.check(!publisher.getInteractionRelevanceAdvisorySwitch(),
            "interaction relevance advisory switch did not turn off");
         publisher.setInteractionRelevanceAdvisorySwitch(true);

         // An active subscriber makes object registration relevant when the
         // publisher declaration overlaps it.  Repeating the declaration is
         // idempotent and does not repeat the advisory.
         subscriber.subscribeObjectClassAttributes(subscriberClass, subscriberNameSet);
         publisher.publishObjectClassAttributes(publisherClass, publisherAllSet);
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(publisherRecorder.starts.size() == 1
               && publisherRecorder.starts.get(0).equals(publisherClass),
            "active object subscription did not produce one start-registration advisory");
         publisher.publishObjectClassAttributes(publisherClass, publisherAllSet);
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(publisherRecorder.starts.size() == 1,
            "repeated object publication repeated start-registration advisory");

         subscriber.unsubscribeObjectClassAttributes(subscriberClass, subscriberNameSet);
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(publisherRecorder.stops.size() == 1
               && publisherRecorder.stops.get(0).equals(publisherClass),
            "removing active object subscription did not produce one stop-registration advisory");

         // Passive declarations are intentionally not relevant.  Promoting
         // that same declaration to active produces the transition.
         subscriber.subscribeObjectClassAttributesPassively(subscriberClass, subscriberNameSet);
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(publisherRecorder.starts.size() == 1
               && publisherRecorder.stops.size() == 1,
            "passive object subscription changed registration relevance");
         subscriber.subscribeObjectClassAttributes(subscriberClass, subscriberNameSet);
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(publisherRecorder.starts.size() == 2,
            "active promotion did not produce start-registration advisory");
         subscriber.unsubscribeObjectClassAttributes(subscriberClass, subscriberNameSet);
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(publisherRecorder.stops.size() == 2,
            "active demotion did not produce stop-registration advisory");
         subscriber.unsubscribeObjectClass(subscriberClass);

         // Interaction declaration relevance follows the same active versus
         // passive boundary, with its own typed callback.
         subscriber.subscribeInteractionClass(subscriberInteraction);
         publisher.publishInteractionClass(publisherInteraction);
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(publisherRecorder.interactionsOn.size() == 1
               && publisherRecorder.interactionsOn.get(0).equals(publisherInteraction),
            "active interaction subscription did not produce turn-on advisory");
         publisher.publishInteractionClass(publisherInteraction);
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(publisherRecorder.interactionsOn.size() == 1,
            "repeated interaction publication repeated turn-on advisory");
         subscriber.unsubscribeInteractionClass(subscriberInteraction);
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(publisherRecorder.interactionsOff.size() == 1
               && publisherRecorder.interactionsOff.get(0).equals(publisherInteraction),
            "removing active interaction subscription did not produce turn-off advisory");
         subscriber.subscribeInteractionClassPassively(subscriberInteraction);
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(publisherRecorder.interactionsOn.size() == 1,
            "passive interaction subscription changed interaction relevance");
         subscriber.subscribeInteractionClass(subscriberInteraction);
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(publisherRecorder.interactionsOn.size() == 2,
            "active interaction promotion did not produce turn-on advisory");
         subscriber.unsubscribeInteractionClass(subscriberInteraction);
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(publisherRecorder.interactionsOff.size() == 2,
            "active interaction demotion did not produce turn-off advisory");
         subscriber.unsubscribeInteractionClass(subscriberInteraction);

         // Keep one active attribute subscription in place while registering
         // an object.  Adding a second active attribute after discovery
         // exercises the non-regional turn-updates callbacks.
         subscriber.subscribeObjectClassAttributes(subscriberClass, subscriberNameSet);
         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.drain(subscriber);
         JavaTckSupport.check(subscriberRecorder.discoveries.size() == 1
               && subscriberRecorder.discoveries.get(0).equals(object),
            "active object subscriber did not discover the registered instance");
         publisherRecorder.updatesOn.clear();
         publisherRecorder.updatesOff.clear();

         subscriber.subscribeObjectClassAttributes(subscriberClass, subscriberPayRateSet);
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(publisherRecorder.updatesOn.size() == 1,
            "new active attribute subscription did not produce turn-updates-on");
         UpdateAdvisory initial = publisherRecorder.updatesOn.get(0);
         JavaTckSupport.check(object.equals(initial.object)
               && initial.attributes.contains(publisherPayRate)
               && initial.updateRate == null,
            "unnamed turn-updates-on advisory did not preserve instance and attributes");

         subscriber.unsubscribeObjectClassAttributes(subscriberClass, subscriberPayRateSet);
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(publisherRecorder.updatesOff.size() == 1,
            "active attribute removal did not produce turn-updates-off");
         JavaTckSupport.check(object.equals(publisherRecorder.updatesOff.get(0).object)
               && publisherRecorder.updatesOff.get(0).attributes.contains(publisherPayRate),
            "turn-updates-off advisory did not preserve instance and attributes");

         subscriber.subscribeObjectClassAttributes(subscriberClass, subscriberAttributeSet,
            "High");
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(publisherRecorder.updatesOn.size() == 2,
            "named active attribute subscription did not produce turn-updates-on");
         UpdateAdvisory named = publisherRecorder.updatesOn.get(1);
         JavaTckSupport.check(object.equals(named.object)
               && named.attributes.contains(publisherAttribute)
               && "High".equals(named.updateRate),
            "named turn-updates-on advisory did not preserve its update rate");
         subscriber.unsubscribeObjectClassAttributes(subscriberClass, subscriberAttributeSet);
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(publisherRecorder.updatesOff.size() == 2,
            "named active attribute removal did not produce turn-updates-off");

         subscriber.unsubscribeObjectClassAttributes(subscriberClass, subscriberNameSet);
         subscriber.unsubscribeObjectClass(subscriberClass);
         publisher.unpublishInteractionClass(publisherInteraction);
         publisher.unpublishObjectClassAttributes(publisherClass, publisherAllSet);
         publisher.unpublishObjectClass(publisherClass);
      } finally {
         if (subscriberJoined) resign(subscriber);
         if (publisherJoined) resign(publisher);
         if (created) {
            try {
               publisher.destroyFederationExecution(federation);
            } catch (Exception ignored) {
            }
         }
         if (subscriberConnected) disconnect(subscriber);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static void checkSwitches(RTIambassador ambassador) throws Exception {
      JavaTckSupport.check(ambassador.getObjectClassRelevanceAdvisorySwitch(),
         "FOM did not enable object-class relevance advisories");
      JavaTckSupport.check(ambassador.getAttributeRelevanceAdvisorySwitch(),
         "FOM did not enable attribute relevance advisories");
      JavaTckSupport.check(ambassador.getInteractionRelevanceAdvisorySwitch(),
         "FOM did not enable interaction relevance advisories");
   }

   private static void resign(RTIambassador ambassador) {
      try {
         ambassador.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
      } catch (Exception ignored) {
      }
   }

   private static void disconnect(RTIambassador ambassador) {
      try {
         ambassador.disconnect();
      } catch (Exception ignored) {
      }
   }

   private static final class UpdateAdvisory {
      private final ObjectInstanceHandle object;
      private final AttributeHandleSet attributes;
      private final String updateRate;

      private UpdateAdvisory(ObjectInstanceHandle object, AttributeHandleSet attributes,
            String updateRate) {
         this.object = object;
         this.attributes = attributes;
         this.updateRate = updateRate;
      }
   }

   private static final class Recorder {
      private final List<ObjectClassHandle> starts = new ArrayList<>();
      private final List<ObjectClassHandle> stops = new ArrayList<>();
      private final List<InteractionClassHandle> interactionsOn = new ArrayList<>();
      private final List<InteractionClassHandle> interactionsOff = new ArrayList<>();
      private final List<UpdateAdvisory> updatesOn = new ArrayList<>();
      private final List<UpdateAdvisory> updatesOff = new ArrayList<>();
      private final List<ObjectInstanceHandle> discoveries = new ArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("startRegistrationForObjectClass".equals(name)) {
                  starts.add((ObjectClassHandle) arguments[0]);
               } else if ("stopRegistrationForObjectClass".equals(name)) {
                  stops.add((ObjectClassHandle) arguments[0]);
               } else if ("turnInteractionsOn".equals(name)) {
                  interactionsOn.add((InteractionClassHandle) arguments[0]);
               } else if ("turnInteractionsOff".equals(name)) {
                  interactionsOff.add((InteractionClassHandle) arguments[0]);
               } else if ("turnUpdatesOnForObjectInstance".equals(name)) {
                  updatesOn.add(new UpdateAdvisory((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1],
                     arguments.length == 3 ? (String) arguments[2] : null));
               } else if ("turnUpdatesOffForObjectInstance".equals(name)) {
                  updatesOff.add(new UpdateAdvisory((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], null));
               } else if ("discoverObjectInstance".equals(name)) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK relevance callbacks";
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

