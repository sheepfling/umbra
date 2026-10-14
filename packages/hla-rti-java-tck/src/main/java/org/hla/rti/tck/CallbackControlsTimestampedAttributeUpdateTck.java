package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;

/** P2 callback-control checks for timestamped attribute reflection and grant ordering. */
final class CallbackControlsTimestampedAttributeUpdateTck {
   private CallbackControlsTimestampedAttributeUpdateTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder subscriberRecorder = new Recorder();
      String federation = "java-tck-callback-tso-attribute-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean subscriberConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean subscriberJoined = false;
      boolean regulating = false;
      boolean constrained = false;
      ObjectClassHandle publisherClass = null;
      ObjectClassHandle subscriberClass = null;
      AttributeHandleSet published = null;
      AttributeHandleSet subscribed = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         subscriber.connect(subscriberRecorder.proxy(), callbackModel);
         subscriberConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-callback-tso-publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution("java-tck-callback-tso-subscriber", federation);
         subscriberJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         subscriberClass = subscriber.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         AttributeHandle subscriberAttribute = subscriber.getAttributeHandle(
            subscriberClass, attributeName);
         published = publisher.getAttributeHandleSetFactory().create();
         published.add(publisherAttribute);
         subscribed = subscriber.getAttributeHandleSetFactory().create();
         subscribed.add(subscriberAttribute);
         publisher.publishObjectClassAttributes(publisherClass, published);
         publisher.changeDefaultAttributeOrderType(
            publisherClass, published, OrderType.TIMESTAMP);
         subscriber.subscribeObjectClassAttributes(subscriberClass, subscribed);

         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         await(callbackModel, () -> object.equals(subscriberRecorder.discoveredObject),
            subscriber, objectClassName);

         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(subscriber.getTimeFactory().getName()),
            "publisher and subscriber selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         publisher.enableTimeRegulation(epsilon);
         regulating = true;
         await(callbackModel, () -> publisherRecorder.timeRegulationEnabled != null,
            publisher, objectClassName);
         subscriber.enableTimeConstrained();
         constrained = true;
         await(callbackModel, () -> subscriberRecorder.timeConstrainedEnabled != null,
            subscriber, objectClassName);

         LogicalTime target = add(publisher.queryLogicalTime(), intervalOffset(epsilon, 4));
         LogicalTime constrainedTarget = add(
            subscriber.queryLogicalTime(), intervalOffset(epsilon, 4));
         JavaTckSupport.check(target.equals(constrainedTarget),
            "time-managed federates derived different advance targets");
         byte[] payload = new byte[] {0x41, 0x54, 0x54};
         byte[] tag = new byte[] {0x54, 0x53, 0x4f};
         AttributeHandleValueMap values =
            publisher.getAttributeHandleValueMapFactory().create(1);
         values.put(publisherAttribute, payload);

         subscriber.disableCallbacks();
         MessageRetractionReturn retraction = publisher.updateAttributeValues(
            object, values, tag, target);
         JavaTckSupport.check(retraction != null && retraction.retractionHandleIsValid
               && retraction.handle != null,
            "timestamped Update Attribute Values returned an invalid retraction designator");
         subscriber.timeAdvanceRequest(constrainedTarget);
         publisher.timeAdvanceRequest(target);
         await(callbackModel, () -> publisherRecorder.timeAdvanceGrant != null,
            publisher, objectClassName);

         service(subscriber, callbackModel, objectClassName);
         JavaTckSupport.check(subscriberRecorder.reflections.isEmpty()
               && subscriberRecorder.timeAdvanceGrant == null,
            "disabled callbacks exposed timestamped reflection or its grant");

         subscriber.enableCallbacks();
         await(callbackModel, () -> subscriberRecorder.reflections.size() == 1
               && subscriberRecorder.timeAdvanceGrant != null,
            subscriber, objectClassName);
         JavaTckSupport.check(subscriberRecorder.reflections.size() == 1,
            "re-enabled callback controls delivered duplicate timestamped reflections");
         Reflection reflection = subscriberRecorder.reflections.get(0);
         JavaTckSupport.check(object.equals(reflection.object)
               && reflection.values.size() == 1
               && Arrays.equals(payload, reflection.values.get(subscriberAttribute))
               && Arrays.equals(tag, reflection.tag)
               && producer.equals(reflection.producer)
               && target.equals(reflection.time)
               && reflection.sentOrder == OrderType.TIMESTAMP
               && reflection.receivedOrder == OrderType.TIMESTAMP
               && reflection.transportation != null
               && !subscriber.getTransportationTypeName(reflection.transportation).isEmpty()
               && reflection.regionsEmpty
               && retraction.handle.equals(reflection.retraction),
            "re-enabled timestamped reflection returned incorrect standard callback metadata");
         JavaTckSupport.check(target.equals(subscriberRecorder.timeAdvanceGrant)
               && Arrays.asList("reflectAttributeValues", "timeAdvanceGrant")
                  .equals(subscriberRecorder.callbackSequence),
            "timestamped reflection was not delivered once before its matching grant");
      } finally {
         if (constrained && subscriberJoined) {
            try { subscriber.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (regulating && publisherJoined) {
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (subscribed != null && subscriberJoined && subscriberClass != null) {
            try { subscriber.unsubscribeObjectClassAttributes(subscriberClass, subscribed); }
            catch (Exception ignored) { }
         }
         if (published != null && publisherJoined && publisherClass != null) {
            try { publisher.unpublishObjectClassAttributes(publisherClass, published); }
            catch (Exception ignored) { }
         }
         if (subscriberJoined) resign(subscriber);
         if (publisherJoined) resign(publisher);
         if (created) {
            try { publisher.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (subscriberConnected) disconnect(subscriber);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static LogicalTime add(LogicalTime start, LogicalTimeInterval interval)
         throws Exception {
      return (LogicalTime) start.add(interval);
   }

   private static LogicalTimeInterval intervalOffset(LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTimeInterval result = epsilon;
      for (int i = 1; i < steps; ++i) result = (LogicalTimeInterval) result.add(epsilon);
      return result;
   }

   private static void service(RTIambassador ambassador, CallbackModel model,
         String objectClassName) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) ambassador.evokeMultipleCallbacks(0.0, 1.0);
      else ambassador.getObjectClassHandle(objectClassName);
   }

   private static void await(CallbackModel model, Condition condition,
         RTIambassador ambassador, String objectClassName) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline) {
         service(ambassador, model, objectClassName);
         if (condition.isMet()) return;
         Thread.yield();
      }
      throw new AssertionError("timestamped attribute callback did not arrive before deadline");
   }

   @FunctionalInterface
   private interface Condition { boolean isMet() throws Exception; }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static final class Reflection {
      private ObjectInstanceHandle object;
      private AttributeHandleValueMap values;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private boolean regionsEmpty;
      private LogicalTime<?, ?> time;
      private OrderType sentOrder;
      private OrderType receivedOrder;
      private MessageRetractionHandle retraction;
   }

   private static final class Recorder {
      private ObjectInstanceHandle discoveredObject;
      private LogicalTime<?, ?> timeRegulationEnabled;
      private LogicalTime<?, ?> timeConstrainedEnabled;
      private LogicalTime<?, ?> timeAdvanceGrant;
      private final List<Reflection> reflections = new ArrayList<>();
      private final List<String> callbackSequence = new ArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)) {
                  discoveredObject = (ObjectInstanceHandle) arguments[0];
               } else if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = (LogicalTime<?, ?>) arguments[0];
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = (LogicalTime<?, ?>) arguments[0];
               } else if ("reflectAttributeValues".equals(name)
                     && arguments != null && arguments.length == 10) {
                  Reflection reflection = new Reflection();
                  reflection.object = (ObjectInstanceHandle) arguments[0];
                  reflection.values = ((AttributeHandleValueMap) arguments[1]).clone();
                  reflection.tag = ((byte[]) arguments[2]).clone();
                  reflection.transportation = (TransportationTypeHandle) arguments[3];
                  reflection.producer = (FederateHandle) arguments[4];
                  RegionHandleSet regions = (RegionHandleSet) arguments[5];
                  reflection.regionsEmpty = regions == null || regions.isEmpty();
                  reflection.time = (LogicalTime<?, ?>) arguments[6];
                  reflection.sentOrder = (OrderType) arguments[7];
                  reflection.receivedOrder = (OrderType) arguments[8];
                  reflection.retraction = (MessageRetractionHandle) arguments[9];
                  reflections.add(reflection);
                  callbackSequence.add(name);
               } else if ("timeAdvanceGrant".equals(name)) {
                  timeAdvanceGrant = (LogicalTime<?, ?>) arguments[0];
                  callbackSequence.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK timestamped callback recorder";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == arguments[0];
               }
               return null;
            });
      }
   }
}
