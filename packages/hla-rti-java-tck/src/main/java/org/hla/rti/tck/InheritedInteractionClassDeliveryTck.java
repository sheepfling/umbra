package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.encoding.HLAunicodeString;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verifies directional interaction inheritance and projection through base subscriptions. */
final class InheritedInteractionClassDeliveryTck {
   private static final long TIMEOUT_NANOS = 10_000_000_000L;

   private InheritedInteractionClassDeliveryTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String baseClassName, String derivedClassName,
         String baseParameterName, String derivedParameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador baseSubscriber = factory.getRtiAmbassador();
      RTIambassador derivedSubscriber = factory.getRtiAmbassador();
      Recorder baseCallbacks = new Recorder();
      Recorder derivedCallbacks = new Recorder();
      String federation = "java-tck-inherited-interaction-delivery-" + UUID.randomUUID();
      boolean publisherConnected = false, baseConnected = false, derivedConnected = false;
      boolean created = false, publisherJoined = false, baseJoined = false, derivedJoined = false;
      boolean publishedBase = false, publishedDerived = false;
      boolean baseSubscribed = false, derivedSubscribed = false;
      InteractionClassHandle base = null, derived = null, baseSubscriberBase = null;
      InteractionClassHandle derivedSubscriberClass = null;
      try {
         publisher.connect(noopFederateAmbassador(), callbackModel);
         publisherConnected = true;
         baseSubscriber.connect(baseCallbacks.proxy(), callbackModel);
         baseConnected = true;
         derivedSubscriber.connect(derivedCallbacks.proxy(), callbackModel);
         derivedConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-interaction-hierarchy-publisher", federation);
         publisherJoined = true;
         baseSubscriber.joinFederationExecution("java-tck-interaction-hierarchy-base", federation);
         baseJoined = true;
         derivedSubscriber.joinFederationExecution(
            "java-tck-interaction-hierarchy-derived", federation);
         derivedJoined = true;

         base = publisher.getInteractionClassHandle(baseClassName);
         derived = publisher.getInteractionClassHandle(derivedClassName);
         baseSubscriberBase = baseSubscriber.getInteractionClassHandle(baseClassName);
         derivedSubscriberClass = derivedSubscriber.getInteractionClassHandle(derivedClassName);
         JavaTckSupport.check(base != null && derived != null && base.equals(baseSubscriberBase)
               && derived.equals(derivedSubscriberClass),
            "interaction hierarchy handles were invalid or inconsistent across federates");

         ParameterHandle publisherBaseParameter = publisher.getParameterHandle(base, baseParameterName);
         ParameterHandle publisherInheritedParameter =
            publisher.getParameterHandle(derived, baseParameterName);
         ParameterHandle publisherDerivedParameter =
            publisher.getParameterHandle(derived, derivedParameterName);
         ParameterHandle baseSubscriberParameter =
            baseSubscriber.getParameterHandle(baseSubscriberBase, baseParameterName);
         ParameterHandle derivedSubscriberBaseParameter =
            derivedSubscriber.getParameterHandle(derivedSubscriberClass, baseParameterName);
         ParameterHandle derivedSubscriberParameter =
            derivedSubscriber.getParameterHandle(derivedSubscriberClass, derivedParameterName);
         JavaTckSupport.check(publisherBaseParameter.equals(publisherInheritedParameter)
               && publisherBaseParameter.equals(baseSubscriberParameter)
               && publisherBaseParameter.equals(derivedSubscriberBaseParameter)
               && publisherDerivedParameter.equals(derivedSubscriberParameter),
            "inherited interaction parameter handles were invalid or inconsistent");

         publisher.publishInteractionClass(base);
         publishedBase = true;
         publisher.publishInteractionClass(derived);
         publishedDerived = true;
         baseSubscriber.subscribeInteractionClass(baseSubscriberBase);
         baseSubscribed = true;
         derivedSubscriber.subscribeInteractionClass(derivedSubscriberClass);
         derivedSubscribed = true;

         HLAinteger32BE integer = factory.getEncoderFactory().createHLAinteger32BE(37);
         HLAunicodeString text = factory.getEncoderFactory().createHLAunicodeString("derived-value");
         byte[] integerBytes = integer.toByteArray();
         byte[] derivedBytes = text.toByteArray();
         byte[] derivedTag = new byte[] {0x49, 0x48, 0x44, 0x31};
         ParameterHandleValueMap derivedValues = publisher.getParameterHandleValueMapFactory()
            .create(2);
         derivedValues.put(publisherInheritedParameter, integerBytes.clone());
         derivedValues.put(publisherDerivedParameter, derivedBytes.clone());
         publisher.sendInteraction(derived, derivedValues, derivedTag.clone());

         await(baseSubscriber, callbackModel, baseCallbacks, "base subscription receiving derived class");
         await(derivedSubscriber, callbackModel, derivedCallbacks,
            "derived subscription receiving derived class");
         JavaTckSupport.check(baseCallbacks.interactions.size() == 1
               && derivedCallbacks.interactions.size() == 1,
            "derived interaction was not delivered exactly once to each matching subscription");
         assertDelivery(baseSubscriber, baseCallbacks.interactions.get(0), baseSubscriberBase,
            publisherBaseParameter, producer, integerBytes, derivedTag,
            "base subscription projection");
         JavaTckSupport.check(baseCallbacks.interactions.get(0).parameters.size() == 1
               && !baseCallbacks.interactions.get(0).parameters.containsKey(publisherDerivedParameter),
            "base subscription did not project to only the inherited parameter");
         assertDelivery(derivedSubscriber, derivedCallbacks.interactions.get(0),
            derivedSubscriberClass, derivedSubscriberBaseParameter, producer, integerBytes,
            derivedTag, "derived subscription inherited parameter");
         JavaTckSupport.check(derivedCallbacks.interactions.get(0).parameters.size() == 2
               && Arrays.equals(derivedBytes, derivedCallbacks.interactions.get(0)
                  .parameters.get(derivedSubscriberParameter)),
            "derived subscription did not include the derived parameter payload");

         baseCallbacks.interactions.clear();
         derivedCallbacks.interactions.clear();
         byte[] baseTag = new byte[] {0x49, 0x48, 0x42, 0x31};
         ParameterHandleValueMap baseValues = publisher.getParameterHandleValueMapFactory()
            .create(1);
         baseValues.put(publisherBaseParameter, integerBytes.clone());
         publisher.sendInteraction(base, baseValues, baseTag.clone());
         await(baseSubscriber, callbackModel, baseCallbacks, "base subscription receiving base class");
         pumpSeveral(derivedSubscriber, callbackModel);
         JavaTckSupport.check(baseCallbacks.interactions.size() == 1
               && derivedCallbacks.interactions.isEmpty(),
            "base interaction crossed the derived-only subscription boundary");
         assertDelivery(baseSubscriber, baseCallbacks.interactions.get(0), baseSubscriberBase,
            publisherBaseParameter, producer, integerBytes, baseTag, "base interaction delivery");
         JavaTckSupport.check(baseCallbacks.interactions.get(0).parameters.size() == 1,
            "base interaction delivered an unexpected parameter set");
      } finally {
         if (baseSubscribed && baseJoined && baseSubscriberBase != null) try {
            baseSubscriber.unsubscribeInteractionClass(baseSubscriberBase);
         } catch (Exception ignored) { }
         if (derivedSubscribed && derivedJoined && derivedSubscriberClass != null) try {
            derivedSubscriber.unsubscribeInteractionClass(derivedSubscriberClass);
         } catch (Exception ignored) { }
         if (publishedDerived && publisherJoined && derived != null) try {
            publisher.unpublishInteractionClass(derived);
         } catch (Exception ignored) { }
         if (publishedBase && publisherJoined && base != null) try {
            publisher.unpublishInteractionClass(base);
         } catch (Exception ignored) { }
         if (derivedJoined) try { derivedSubscriber.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (baseJoined) try { baseSubscriber.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (publisherJoined) try {
            publisher.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         } catch (Exception ignored) { }
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (derivedConnected) try { derivedSubscriber.disconnect(); } catch (Exception ignored) { }
         if (baseConnected) try { baseSubscriber.disconnect(); } catch (Exception ignored) { }
         if (publisherConnected) try { publisher.disconnect(); } catch (Exception ignored) { }
      }
   }

    static void runClosestAncestor(RtiFactory factory, String fom,
          String timeImplementation, String baseName, String middleName,
          String childName, CallbackModel callbackModel) throws Exception {
       RTIambassador publisher = factory.getRtiAmbassador();
       RTIambassador subscriber = factory.getRtiAmbassador();
       Recorder callbacks = new Recorder();
       String federation = "java-tck-interaction-closest-ancestor-" + UUID.randomUUID();
       boolean publisherConnected = false, subscriberConnected = false;
       boolean created = false, publisherJoined = false, subscriberJoined = false;
       boolean published = false, baseSubscribed = false, middleSubscribed = false;
       boolean exactSubscribed = false;
       InteractionClassHandle sentClass = null, baseClass = null, middleClass = null;
       InteractionClassHandle exactClass = null;
       try {
          publisher.connect(noopFederateAmbassador(), callbackModel);
          publisherConnected = true;
          subscriber.connect(callbacks.proxy(), callbackModel);
          subscriberConnected = true;
          publisher.createFederationExecution(federation, fom, timeImplementation);
          created = true;
          publisher.joinFederationExecution("java-tck-closest-ancestor-publisher", federation);
          publisherJoined = true;
          subscriber.joinFederationExecution("java-tck-closest-ancestor-subscriber", federation);
          subscriberJoined = true;

          sentClass = publisher.getInteractionClassHandle(childName);
          baseClass = subscriber.getInteractionClassHandle(baseName);
          middleClass = subscriber.getInteractionClassHandle(middleName);
          exactClass = subscriber.getInteractionClassHandle(childName);
          JavaTckSupport.check(sentClass != null && baseClass != null && middleClass != null
                && exactClass != null && sentClass.equals(exactClass)
                && !baseClass.equals(middleClass) && !middleClass.equals(exactClass),
             "adapter FOM did not provide a stable three-level interaction hierarchy");

          publisher.publishInteractionClass(sentClass);
          published = true;
          ParameterHandleValueMap empty = publisher.getParameterHandleValueMapFactory().create(0);
          subscriber.subscribeInteractionClass(baseClass);
          baseSubscribed = true;
          checkClosestAncestor(publisher, subscriber, callbackModel, callbacks,
             sentClass, baseClass, empty, "base-only ancestor subscription");
          subscriber.subscribeInteractionClass(middleClass);
          middleSubscribed = true;
          checkClosestAncestor(publisher, subscriber, callbackModel, callbacks,
             sentClass, middleClass, empty, "closest of two ancestor subscriptions");
          subscriber.subscribeInteractionClass(exactClass);
          exactSubscribed = true;
          checkClosestAncestor(publisher, subscriber, callbackModel, callbacks,
             sentClass, exactClass, empty, "exact sent-class subscription");
       } finally {
          if (exactSubscribed && subscriberJoined) try {
             subscriber.unsubscribeInteractionClass(exactClass);
          } catch (Exception ignored) { }
          if (middleSubscribed && subscriberJoined) try {
             subscriber.unsubscribeInteractionClass(middleClass);
          } catch (Exception ignored) { }
          if (baseSubscribed && subscriberJoined) try {
             subscriber.unsubscribeInteractionClass(baseClass);
          } catch (Exception ignored) { }
          if (published && publisherJoined) try {
             publisher.unpublishInteractionClass(sentClass);
          } catch (Exception ignored) { }
          if (subscriberJoined) try {
             subscriber.resignFederationExecution(ResignAction.NO_ACTION);
          } catch (Exception ignored) { }
          if (publisherJoined) try {
             publisher.resignFederationExecution(ResignAction.NO_ACTION);
          } catch (Exception ignored) { }
          if (created) try { publisher.destroyFederationExecution(federation); }
             catch (Exception ignored) { }
          if (subscriberConnected) try { subscriber.disconnect(); } catch (Exception ignored) { }
          if (publisherConnected) try { publisher.disconnect(); } catch (Exception ignored) { }
       }
    }

    private static void checkClosestAncestor(RTIambassador publisher, RTIambassador subscriber,
          CallbackModel model, Recorder callbacks, InteractionClassHandle sentClass,
          InteractionClassHandle expectedClass, ParameterHandleValueMap empty,
          String phase) throws Exception {
       callbacks.interactions.clear();
       publisher.sendInteraction(sentClass, empty, new byte[0]);
       await(subscriber, model, callbacks, phase);
       pumpSeveral(subscriber, model);
       JavaTckSupport.check(callbacks.interactions.size() == 1
             && callbacks.interactions.get(0).interaction.equals(expectedClass)
             && callbacks.interactions.get(0).parameters.isEmpty(),
          phase + " did not report exactly one callback at the closest subscribed class");
    }

    private static void assertDelivery(RTIambassador receiver, Interaction actual,
         InteractionClassHandle expectedClass, ParameterHandle expectedParameter,
         FederateHandle expectedProducer, byte[] expectedValue, byte[] expectedTag,
         String description) throws Exception {
      JavaTckSupport.check(actual.interaction.equals(expectedClass),
         description + " returned the wrong received interaction class");
      JavaTckSupport.check(actual.parameters.containsKey(expectedParameter)
            && Arrays.equals(expectedValue, actual.parameters.get(expectedParameter)),
         description + " omitted or changed the expected parameter payload");
      JavaTckSupport.check(Arrays.equals(expectedTag, actual.tag),
         description + " changed the sent user tag");
      JavaTckSupport.check(expectedProducer.equals(actual.producer),
         description + " reported the wrong producing federate");
      JavaTckSupport.check(actual.transportation != null,
         description + " returned null transportation");
      JavaTckSupport.check(actual.regionsEmpty,
         description + " returned nonempty or absent source regions");
      JavaTckSupport.check(!receiver.getTransportationTypeName(actual.transportation).isEmpty(),
         description + " returned an unresolvable transportation type");
   }

   private static void await(RTIambassador ambassador, CallbackModel model, Recorder recorder,
         String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && recorder.interactions.isEmpty()) {
         if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
         Thread.sleep(1L);
      }
      JavaTckSupport.check(!recorder.interactions.isEmpty(), "timed out waiting for " + description);
   }

   private static void pumpSeveral(RTIambassador ambassador, CallbackModel model) throws Exception {
      for (int i = 0; i < 8; ++i) {
         if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
      }
   }

   private static FederateAmbassador noopFederateAmbassador() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK interaction hierarchy publisher";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) return proxy == arguments[0];
            }
            return null;
         });
   }

   private static final class Interaction {
      private final InteractionClassHandle interaction;
      private final Map<ParameterHandle, byte[]> parameters = new HashMap<>();
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final boolean regionsEmpty;

      private Interaction(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         ParameterHandleValueMap values = (ParameterHandleValueMap) arguments[1];
         values.forEach((handle, value) -> parameters.put(handle, value.clone()));
         tag = ((byte[]) arguments[2]).clone();
         transportation = (TransportationTypeHandle) arguments[3];
         producer = (FederateHandle) arguments[4];
         RegionHandleSet regions = (RegionHandleSet) arguments[5];
         regionsEmpty = regions == null || regions.isEmpty();
      }
   }

   private static final class Recorder {
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("receiveInteraction".equals(method.getName()) && arguments != null
                     && arguments.length == 6) interactions.add(new Interaction(arguments));
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(method.getName())) return "Java TCK inherited interaction callbacks";
                  if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
                  if ("equals".equals(method.getName())) return proxy == arguments[0];
               }
               return null;
            });
      }
   }
}
