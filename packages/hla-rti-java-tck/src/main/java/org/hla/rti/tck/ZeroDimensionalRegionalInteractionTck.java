package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Confirms that an explicit zero-dimensional interaction region does not overlap default. */
final class ZeroDimensionalRegionalInteractionTck {
   private static final long TIMEOUT_NANOS = 10_000_000_000L;

   private ZeroDimensionalRegionalInteractionTck() { }

   static void run(RtiFactory factory, String fom, String interactionName, String parameterName,
         String firstDimensionName, String secondDimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder callbacks = new Recorder();
      String federation = "java-tck-zero-dimensional-regional-interaction-" + UUID.randomUUID();
      boolean publisherConnected = false, subscriberConnected = false, created = false;
      boolean publisherJoined = false, subscriberJoined = false, published = false, subscribed = false;
      RegionHandle zeroRegion = null;
      InteractionClassHandle publisherInteraction = null, subscriberInteraction = null;
      try {
         publisher.connect(noopFederateAmbassador(), callbackModel);
         publisherConnected = true;
         subscriber.connect(callbacks.proxy(), callbackModel);
         subscriberConnected = true;
         publisher.createFederationExecution(federation, fom);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-zero-dimensional-publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution("java-tck-zero-dimensional-subscriber", federation);
         subscriberJoined = true;

         publisherInteraction = publisher.getInteractionClassHandle(interactionName);
         subscriberInteraction = subscriber.getInteractionClassHandle(interactionName);
         ParameterHandle publisherParameter =
            publisher.getParameterHandle(publisherInteraction, parameterName);
         ParameterHandle subscriberParameter =
            subscriber.getParameterHandle(subscriberInteraction, parameterName);
         DimensionHandle publisherFirst = publisher.getDimensionHandle(firstDimensionName);
         DimensionHandle publisherSecond = publisher.getDimensionHandle(secondDimensionName);
         DimensionHandle subscriberFirst = subscriber.getDimensionHandle(firstDimensionName);
         DimensionHandle subscriberSecond = subscriber.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(publisherInteraction.equals(subscriberInteraction)
               && publisherParameter.equals(subscriberParameter)
               && publisherFirst.equals(subscriberFirst) && publisherSecond.equals(subscriberSecond),
            "adapter FOM handles changed across zero-dimensional interaction federates");
         JavaTckSupport.check(publisher.getAvailableDimensionsForInteractionClass(
                  publisherInteraction).contains(publisherFirst)
               && publisher.getAvailableDimensionsForInteractionClass(
                  publisherInteraction).contains(publisherSecond),
            "adapter interaction class must expose both configured dimensions");

         publisher.publishInteractionClass(publisherInteraction);
         published = true;
         subscriber.subscribeInteractionClass(subscriberInteraction);
         subscribed = true;

         byte[] value = new byte[] {0x2a, 0x15};
         byte[] tag = new byte[] {0x7a, 0x64, 0x6d};
         send(publisher, publisherInteraction, publisherParameter, value, tag);
         await(subscriber, callbackModel, () -> callbacks.interactions.size() >= 1,
            "ordinary interaction baseline");
         JavaTckSupport.check(callbacks.interactions.size() == 1,
            "ordinary interaction baseline delivered a duplicate callback");
         assertDelivery(subscriber, callbacks.interactions.get(0), subscriberInteraction,
            subscriberParameter, producer, value, tag, "ordinary interaction baseline");

         DimensionHandleSet noDimensions = publisher.getDimensionHandleSetFactory().create();
         zeroRegion = publisher.createRegion(noDimensions);
         JavaTckSupport.check(zeroRegion != null
               && publisher.getDimensionHandleSet(zeroRegion).isEmpty(),
            "zero-dimensional region creation returned an invalid handle or nonempty dimensions");
         RegionHandleSet regionSet = publisher.getRegionHandleSetFactory().create();
         regionSet.add(zeroRegion);
         publisher.commitRegionModifications(regionSet);
         JavaTckSupport.check(publisher.getDimensionHandleSet(zeroRegion).isEmpty(),
            "zero-dimensional region changed dimensions after commit");

         callbacks.interactions.clear();
         sendWithRegion(publisher, publisherInteraction, publisherParameter, zeroRegion, value, tag);
         pumpSeveral(subscriber, callbackModel);
         JavaTckSupport.check(callbacks.interactions.isEmpty(),
            "explicit zero-dimensional interaction realization overlapped the default region");

         subscriber.unsubscribeInteractionClass(subscriberInteraction);
         subscribed = false;
         publisher.unpublishInteractionClass(publisherInteraction);
         published = false;
         publisher.deleteRegion(zeroRegion);
         zeroRegion = null;
         subscriber.resignFederationExecution(ResignAction.NO_ACTION);
         subscriberJoined = false;
         publisher.resignFederationExecution(ResignAction.NO_ACTION);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (subscribed && subscriberJoined && subscriberInteraction != null) try {
            subscriber.unsubscribeInteractionClass(subscriberInteraction);
         } catch (Exception ignored) { }
         if (published && publisherJoined && publisherInteraction != null) try {
            publisher.unpublishInteractionClass(publisherInteraction);
         } catch (Exception ignored) { }
         if (zeroRegion != null && publisherJoined) try { publisher.deleteRegion(zeroRegion); }
            catch (Exception ignored) { }
         if (subscriberJoined) try { subscriber.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (publisherJoined) try {
            publisher.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         } catch (Exception ignored) { }
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (publisherConnected) try { publisher.disconnect(); } catch (Exception ignored) { }
         if (subscriberConnected) try { subscriber.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static void send(RTIambassador publisher, InteractionClassHandle interaction,
         ParameterHandle parameter, byte[] value, byte[] tag) throws Exception {
      ParameterHandleValueMap values = publisher.getParameterHandleValueMapFactory().create(1);
      values.put(parameter, value.clone());
      publisher.sendInteraction(interaction, values, tag.clone());
   }

   private static void sendWithRegion(RTIambassador publisher, InteractionClassHandle interaction,
         ParameterHandle parameter, RegionHandle region, byte[] value, byte[] tag) throws Exception {
      ParameterHandleValueMap values = publisher.getParameterHandleValueMapFactory().create(1);
      values.put(parameter, value.clone());
      RegionHandleSet regions = publisher.getRegionHandleSetFactory().create();
      regions.add(region);
      publisher.sendInteractionWithRegions(interaction, values, regions, tag.clone());
   }

   private static void assertDelivery(RTIambassador receiver, Interaction actual,
         InteractionClassHandle expectedClass, ParameterHandle expectedParameter,
         FederateHandle expectedProducer, byte[] expectedValue, byte[] expectedTag,
         String description) throws Exception {
      JavaTckSupport.check(actual.callbackArity == 6
            && actual.interaction.equals(expectedClass)
            && actual.parameters.size() == 1
            && Arrays.equals(expectedValue, actual.parameters.get(expectedParameter))
            && Arrays.equals(expectedTag, actual.tag)
            && expectedProducer.equals(actual.producer)
            && actual.regions != null && actual.regions.isEmpty()
            && actual.transportation != null,
         description + " changed standard payload or callback metadata");
      JavaTckSupport.check(!receiver.getTransportationTypeName(actual.transportation).isEmpty(),
         description + " returned an unresolvable transportation type");
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         java.util.function.BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         pump(ambassador, model);
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void pumpSeveral(RTIambassador ambassador, CallbackModel model) throws Exception {
      for (int pass = 0; pass < 8; ++pass) pump(ambassador, model);
   }

   private static void pump(RTIambassador ambassador, CallbackModel model) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
      else Thread.yield();
   }

   private static FederateAmbassador noopFederateAmbassador() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK zero-dimensional publisher";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) return proxy == arguments[0];
            }
            return null;
         });
   }

   private static final class Interaction {
      private final int callbackArity;
      private final InteractionClassHandle interaction;
      private final Map<ParameterHandle, byte[]> parameters = new HashMap<>();
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final List<RegionHandle> regions;

      private Interaction(Object[] arguments) {
         callbackArity = arguments.length;
         interaction = (InteractionClassHandle) arguments[0];
         ParameterHandleValueMap values = (ParameterHandleValueMap) arguments[1];
         values.forEach((handle, value) -> parameters.put(handle, value.clone()));
         tag = ((byte[]) arguments[2]).clone();
         transportation = (TransportationTypeHandle) arguments[3];
         producer = (FederateHandle) arguments[4];
         RegionHandleSet regionSet = (RegionHandleSet) arguments[5];
         regions = regionSet == null ? null : new java.util.ArrayList<>(regionSet);
      }
   }

   private static final class Recorder {
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("receiveInteraction".equals(method.getName()) && arguments != null
                     && arguments.length == 6) {
                  interactions.add(new Interaction(arguments));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(method.getName())) return "Java TCK zero-dimensional callbacks";
                  if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
                  if ("equals".equals(method.getName())) return proxy == arguments[0];
               }
               return null;
            });
      }
   }
}
