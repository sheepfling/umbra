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
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** P1.3 ordinary declaration-relevance and update-relevance callbacks. */
final class RelevanceAdvisoryTck {
   private RelevanceAdvisoryTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String nameAttributeName, String rateAttributeName,
         String attributeName, String namedUpdateRateName, String interactionClassName,
         CallbackModel callbackModel)
         throws Exception {
      JavaTckSupport.check(!nameAttributeName.equals(rateAttributeName)
            && !nameAttributeName.equals(attributeName)
            && !rateAttributeName.equals(attributeName),
         "relevance-advisory FOM attribute names must be distinct");
      JavaTckSupport.check(namedUpdateRateName != null
            && !namedUpdateRateName.trim().isEmpty(),
         "adapter must provide a non-empty named update-rate label");
      checkSwitchLifecycle(factory, callbackModel);
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
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         subscriber.connect(subscriberRecorder.proxy(), callbackModel);
         subscriberConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         publisher.joinFederationExecution("java-tck-relevance-publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution("java-tck-relevance-subscriber", federation);
         subscriberJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle subscriberClass = subscriber.getObjectClassHandle(objectClassName);
         AttributeHandle publisherName = publisher.getAttributeHandle(
            publisherClass, nameAttributeName);
         AttributeHandle publisherPayRate = publisher.getAttributeHandle(
            publisherClass, rateAttributeName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(publisherClass,
            attributeName);
         AttributeHandle subscriberName = subscriber.getAttributeHandle(
            subscriberClass, nameAttributeName);
         AttributeHandle subscriberPayRate = subscriber.getAttributeHandle(subscriberClass,
            rateAttributeName);
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
         roundTripBooleanSwitch(
            publisher::getAttributeScopeAdvisorySwitch,
            publisher::setAttributeScopeAdvisorySwitch,
            "attribute-scope advisory switch");
         roundTripBooleanSwitch(
            publisher::getConveyRegionDesignatorSetsSwitch,
            publisher::setConveyRegionDesignatorSetsSwitch,
            "convey-region-designator-sets switch");
         roundTripBooleanSwitch(
            publisher::getServiceReportingSwitch,
            publisher::setServiceReportingSwitch,
            "service-reporting switch");
         roundTripBooleanSwitch(
            publisher::getExceptionReportingSwitch,
            publisher::setExceptionReportingSwitch,
            "exception-reporting switch");
         roundTripBooleanSwitch(
            publisher::getSendServiceReportsToFileSwitch,
            publisher::setSendServiceReportsToFileSwitch,
            "send-service-reports-to-file switch");
         checkReadOnlySwitch(publisher::getAutoProvideSwitch, "auto-provide switch");
         checkReadOnlySwitch(publisher::getDelaySubscriptionEvaluationSwitch,
            "delay-subscription-evaluation switch");
         checkReadOnlySwitch(publisher::getAdvisoriesUseKnownClassSwitch,
            "advisories-use-known-class switch");
         checkReadOnlySwitch(publisher::getAllowRelaxedDDMSwitch, "allow-relaxed-DDM switch");
         checkReadOnlySwitch(publisher::getNonRegulatedGrantSwitch, "non-regulated-grant switch");

         // An active subscriber makes object registration relevant when the
         // publisher declaration overlaps it.  Repeating the declaration is
         // idempotent and does not repeat the advisory.
         subscriber.subscribeObjectClassAttributes(subscriberClass, subscriberNameSet);
         publisher.publishObjectClassAttributes(publisherClass, publisherAllSet);
         await(publisher, callbackModel, () -> publisherRecorder.starts.size() >= 1,
            "active object registration relevance advisory");
         JavaTckSupport.check(publisherRecorder.starts.size() == 1
               && publisherRecorder.starts.get(0).equals(publisherClass),
            "active object subscription did not produce one start-registration advisory");
         publisher.publishObjectClassAttributes(publisherClass, publisherAllSet);
         drain(publisher, callbackModel);
         JavaTckSupport.check(publisherRecorder.starts.size() == 1,
            "repeated object publication repeated start-registration advisory");

         subscriber.unsubscribeObjectClassAttributes(subscriberClass, subscriberNameSet);
         await(publisher, callbackModel, () -> publisherRecorder.stops.size() >= 1,
            "object registration relevance stop advisory");
         JavaTckSupport.check(publisherRecorder.stops.size() == 1
               && publisherRecorder.stops.get(0).equals(publisherClass),
            "removing active object subscription did not produce one stop-registration advisory");

         // Passive declarations are intentionally not relevant.  Promoting
         // that same declaration to active produces the transition.
         subscriber.subscribeObjectClassAttributesPassively(subscriberClass, subscriberNameSet);
         drain(publisher, callbackModel);
         JavaTckSupport.check(publisherRecorder.starts.size() == 1
               && publisherRecorder.stops.size() == 1,
            "passive object subscription changed registration relevance");
         subscriber.subscribeObjectClassAttributes(subscriberClass, subscriberNameSet);
         await(publisher, callbackModel, () -> publisherRecorder.starts.size() >= 2,
            "active object registration promotion advisory");
         JavaTckSupport.check(publisherRecorder.starts.size() == 2,
            "active promotion did not produce start-registration advisory");
         subscriber.unsubscribeObjectClassAttributes(subscriberClass, subscriberNameSet);
         await(publisher, callbackModel, () -> publisherRecorder.stops.size() >= 2,
            "active object registration demotion advisory");
         JavaTckSupport.check(publisherRecorder.stops.size() == 2,
            "active demotion did not produce stop-registration advisory");
         subscriber.unsubscribeObjectClass(subscriberClass);

         // Interaction declaration relevance follows the same active versus
         // passive boundary, with its own typed callback.
         subscriber.subscribeInteractionClass(subscriberInteraction);
         publisher.publishInteractionClass(publisherInteraction);
         await(publisher, callbackModel, () -> publisherRecorder.interactionsOn.size() >= 1,
            "active interaction turn-on advisory");
         JavaTckSupport.check(publisherRecorder.interactionsOn.size() == 1
               && publisherRecorder.interactionsOn.get(0).equals(publisherInteraction),
            "active interaction subscription did not produce turn-on advisory");
         publisher.publishInteractionClass(publisherInteraction);
         drain(publisher, callbackModel);
         JavaTckSupport.check(publisherRecorder.interactionsOn.size() == 1,
            "repeated interaction publication repeated turn-on advisory");
         subscriber.unsubscribeInteractionClass(subscriberInteraction);
         await(publisher, callbackModel, () -> publisherRecorder.interactionsOff.size() >= 1,
            "active interaction turn-off advisory");
         JavaTckSupport.check(publisherRecorder.interactionsOff.size() == 1
               && publisherRecorder.interactionsOff.get(0).equals(publisherInteraction),
            "removing active interaction subscription did not produce turn-off advisory");
         subscriber.subscribeInteractionClassPassively(subscriberInteraction);
         drain(publisher, callbackModel);
         JavaTckSupport.check(publisherRecorder.interactionsOn.size() == 1,
            "passive interaction subscription changed interaction relevance");
         subscriber.subscribeInteractionClass(subscriberInteraction);
         await(publisher, callbackModel, () -> publisherRecorder.interactionsOn.size() >= 2,
            "active interaction promotion advisory");
         JavaTckSupport.check(publisherRecorder.interactionsOn.size() == 2,
            "active interaction promotion did not produce turn-on advisory");
         subscriber.unsubscribeInteractionClass(subscriberInteraction);
         await(publisher, callbackModel, () -> publisherRecorder.interactionsOff.size() >= 2,
            "active interaction demotion advisory");
         JavaTckSupport.check(publisherRecorder.interactionsOff.size() == 2,
            "active interaction demotion did not produce turn-off advisory");
         subscriber.unsubscribeInteractionClass(subscriberInteraction);

         // Keep one active attribute subscription in place while registering
         // an object.  Adding a second active attribute after discovery
         // exercises the non-regional turn-updates callbacks.
         subscriber.subscribeObjectClassAttributes(subscriberClass, subscriberNameSet);
         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         await(subscriber, callbackModel, () -> subscriberRecorder.discoveries.size() >= 1,
            "active object discovery");
         JavaTckSupport.check(subscriberRecorder.discoveries.size() == 1
               && subscriberRecorder.discoveries.get(0).equals(object),
            "active object subscriber did not discover the registered instance");
         for (int pass = 0; pass < 8; ++pass) {
            drain(publisher, callbackModel);
         }
         publisherRecorder.updatesOn.clear();
         publisherRecorder.updatesOff.clear();

         subscriber.subscribeObjectClassAttributes(subscriberClass, subscriberPayRateSet);
         await(publisher, callbackModel, () -> publisherRecorder.updatesOn.size() >= 1,
            "unnamed attribute turn-updates-on advisory");
         JavaTckSupport.check(publisherRecorder.updatesOn.size() == 1,
            "new active attribute subscription did not produce turn-updates-on");
         UpdateAdvisory initial = publisherRecorder.updatesOn.get(0);
         JavaTckSupport.check(object.equals(initial.object)
               && initial.attributes.contains(publisherPayRate)
               && initial.updateRate == null,
            "unnamed turn-updates-on advisory did not preserve instance and attributes");

         subscriber.unsubscribeObjectClassAttributes(subscriberClass, subscriberPayRateSet);
         await(publisher, callbackModel, () -> publisherRecorder.updatesOff.size() >= 1,
            "unnamed attribute turn-updates-off advisory");
         JavaTckSupport.check(publisherRecorder.updatesOff.size() == 1,
            "active attribute removal did not produce turn-updates-off");
         JavaTckSupport.check(object.equals(publisherRecorder.updatesOff.get(0).object)
               && publisherRecorder.updatesOff.get(0).attributes.contains(publisherPayRate),
            "turn-updates-off advisory did not preserve instance and attributes");

         subscriber.subscribeObjectClassAttributes(subscriberClass, subscriberAttributeSet,
            namedUpdateRateName);
         await(publisher, callbackModel, () -> publisherRecorder.updatesOn.size() >= 2,
            "named attribute turn-updates-on advisory");
         JavaTckSupport.check(publisherRecorder.updatesOn.size() == 2,
            "named active attribute subscription did not produce turn-updates-on");
         UpdateAdvisory named = publisherRecorder.updatesOn.get(1);
         JavaTckSupport.check(object.equals(named.object)
               && named.attributes.contains(publisherAttribute)
               && namedUpdateRateName.equals(named.updateRate),
            "named turn-updates-on advisory did not preserve its update rate");
         double namedUpdateRate = subscriber.getUpdateRateValue(namedUpdateRateName);
         JavaTckSupport.check(namedUpdateRate > 0.0,
            "named update-rate lookup did not return a positive FOM value");
         JavaTckSupport.check(subscriber.getUpdateRateValueForAttribute(
               object, subscriberAttribute) == namedUpdateRate,
            "attribute update-rate lookup did not return the active subscription rate");
         subscriber.unsubscribeObjectClassAttributes(subscriberClass, subscriberAttributeSet);
         await(publisher, callbackModel, () -> publisherRecorder.updatesOff.size() >= 2,
            "named attribute turn-updates-off advisory");
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

   private static void checkSwitchLifecycle(RtiFactory factory, CallbackModel callbackModel)
         throws Exception {
      RTIambassador ambassador = factory.getRtiAmbassador();
      boolean connected = false;
      try {
         requireSwitchLifecycle(ambassador, "NotConnected", "before connect");
         ambassador.connect(new Recorder().proxy(), callbackModel);
         connected = true;
         requireSwitchLifecycle(ambassador, "FederateNotExecutionMember",
            "after connect before joining an execution");
      } finally {
         if (connected) disconnect(ambassador);
      }
   }

   private static void requireSwitchLifecycle(RTIambassador ambassador, String expected,
         String phase) throws Exception {
      requireBooleanSwitchLifecycle(ambassador::getObjectClassRelevanceAdvisorySwitch,
         ambassador::setObjectClassRelevanceAdvisorySwitch, expected, phase,
         "object-class relevance advisory switch");
      requireBooleanSwitchLifecycle(ambassador::getAttributeRelevanceAdvisorySwitch,
         ambassador::setAttributeRelevanceAdvisorySwitch, expected, phase,
         "attribute relevance advisory switch");
      requireBooleanSwitchLifecycle(ambassador::getAttributeScopeAdvisorySwitch,
         ambassador::setAttributeScopeAdvisorySwitch, expected, phase,
         "attribute-scope advisory switch");
      requireBooleanSwitchLifecycle(ambassador::getInteractionRelevanceAdvisorySwitch,
         ambassador::setInteractionRelevanceAdvisorySwitch, expected, phase,
         "interaction relevance advisory switch");
      requireBooleanSwitchLifecycle(ambassador::getConveyRegionDesignatorSetsSwitch,
         ambassador::setConveyRegionDesignatorSetsSwitch, expected, phase,
         "convey-region-designator-sets switch");
      requireFailure(ambassador::getAutomaticResignDirective, expected,
         "getting the automatic resign directive " + phase);
      requireFailure(() -> ambassador.setAutomaticResignDirective(ResignAction.NO_ACTION),
         expected, "setting the automatic resign directive " + phase);
      requireBooleanSwitchLifecycle(ambassador::getServiceReportingSwitch,
         ambassador::setServiceReportingSwitch, expected, phase,
         "service-reporting switch");
      requireBooleanSwitchLifecycle(ambassador::getExceptionReportingSwitch,
         ambassador::setExceptionReportingSwitch, expected, phase,
         "exception-reporting switch");
      requireBooleanSwitchLifecycle(ambassador::getSendServiceReportsToFileSwitch,
         ambassador::setSendServiceReportsToFileSwitch, expected, phase,
         "send-service-reports-to-file switch");
      requireReadOnlySwitchLifecycle(ambassador::getAutoProvideSwitch, expected, phase,
         "auto-provide switch");
      requireReadOnlySwitchLifecycle(ambassador::getDelaySubscriptionEvaluationSwitch, expected,
         phase, "delay-subscription-evaluation switch");
      requireReadOnlySwitchLifecycle(ambassador::getAdvisoriesUseKnownClassSwitch, expected,
         phase, "advisories-use-known-class switch");
      requireReadOnlySwitchLifecycle(ambassador::getAllowRelaxedDDMSwitch, expected, phase,
         "allow-relaxed-DDM switch");
      requireReadOnlySwitchLifecycle(ambassador::getNonRegulatedGrantSwitch, expected, phase,
         "non-regulated-grant switch");
   }

   private static void requireBooleanSwitchLifecycle(CheckedBooleanGetter getter,
         CheckedBooleanSetter setter, String expected, String phase, String description)
         throws Exception {
      requireFailure(getter::get, expected, "getting the " + description + " " + phase);
      requireFailure(() -> setter.set(false), expected,
         "setting the " + description + " " + phase);
   }

   private static void requireReadOnlySwitchLifecycle(CheckedBooleanGetter getter,
         String expected, String phase, String description) throws Exception {
      requireFailure(getter::get, expected, "getting the " + description + " " + phase);
   }

   private static void requireFailure(JavaTckSupport.CheckedOperation operation,
         String expected, String description) throws Exception {
      JavaTckSupport.expectFailure(operation, expected, description);
   }

   @FunctionalInterface
   private interface CheckedBooleanGetter { boolean get() throws Exception; }

   @FunctionalInterface
   private interface CheckedBooleanSetter { void set(boolean value) throws Exception; }

   private static void checkSwitches(RTIambassador ambassador) throws Exception {
      JavaTckSupport.check(ambassador.getObjectClassRelevanceAdvisorySwitch(),
         "FOM did not enable object-class relevance advisories");
      JavaTckSupport.check(ambassador.getAttributeRelevanceAdvisorySwitch(),
         "FOM did not enable attribute relevance advisories");
      JavaTckSupport.check(ambassador.getInteractionRelevanceAdvisorySwitch(),
         "FOM did not enable interaction relevance advisories");
   }

   private static void roundTripBooleanSwitch(CheckedBooleanGetter getter,
         CheckedBooleanSetter setter, String description) throws Exception {
      boolean original = getter.get();
      setter.set(false);
      JavaTckSupport.check(!getter.get(), description + " did not turn off");
      setter.set(true);
      JavaTckSupport.check(getter.get(), description + " did not turn on");
      setter.set(original);
      JavaTckSupport.check(getter.get() == original, description + " did not restore its state");
   }

   private static void checkReadOnlySwitch(CheckedBooleanGetter getter, String description)
         throws Exception {
      boolean first = getter.get();
      JavaTckSupport.check(getter.get() == first, description + " was not stable across queries");
   }

   private static void drain(RTIambassador ambassador, CallbackModel callbackModel)
         throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) {
         JavaTckSupport.drain(ambassador);
      }
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         drain(ambassador, callbackModel);
         Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
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
      private final List<ObjectClassHandle> starts = new CopyOnWriteArrayList<>();
      private final List<ObjectClassHandle> stops = new CopyOnWriteArrayList<>();
      private final List<InteractionClassHandle> interactionsOn = new CopyOnWriteArrayList<>();
      private final List<InteractionClassHandle> interactionsOff = new CopyOnWriteArrayList<>();
      private final List<UpdateAdvisory> updatesOn = new CopyOnWriteArrayList<>();
      private final List<UpdateAdvisory> updatesOff = new CopyOnWriteArrayList<>();
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();

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

