package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.exceptions.InvalidLogicalTime;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verifies the post-TAR timestamp cutoff for timestamped attribute updates. */
final class TimestampedAttributeTarPostGrantCutoffTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 15_000_000_000L;

   private TimestampedAttributeTarPostGrantCutoffTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador receiver = factory.getRtiAmbassador();
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador frontier = factory.getRtiAmbassador();
      Recorder receiverRecorder = new Recorder();
      Recorder publisherRecorder = new Recorder();
      Recorder frontierRecorder = new Recorder();
      String federation = "java-tck-attribute-tar-cutoff-" + UUID.randomUUID();
      boolean receiverConnected = false;
      boolean publisherConnected = false;
      boolean frontierConnected = false;
      boolean created = false;
      boolean receiverJoined = false;
      boolean publisherJoined = false;
      boolean frontierJoined = false;
      boolean receiverConstrained = false;
      boolean publisherRegulating = false;
      boolean frontierRegulating = false;
      boolean published = false;
      boolean subscribed = false;
      ObjectClassHandle publisherClass = null;
      ObjectClassHandle receiverClass = null;
      AttributeHandle publisherAttribute = null;
      AttributeHandle receiverAttribute = null;
      ObjectInstanceHandle object = null;
      try {
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         frontier.connect(frontierRecorder.proxy(), callbackModel);
         frontierConnected = true;
         receiver.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         receiver.joinFederationExecution("java-tck-attribute-cutoff-receiver", federation);
         receiverJoined = true;
         FederateHandle publisherFederate = publisher.joinFederationExecution(
            "java-tck-attribute-cutoff-late-publisher", federation);
         publisherJoined = true;
         frontier.joinFederationExecution("java-tck-attribute-cutoff-frontier", federation);
         frontierJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         receiverClass = receiver.getObjectClassHandle(objectClassName);
         publisherAttribute = publisher.getAttributeHandle(publisherClass, attributeName);
         receiverAttribute = receiver.getAttributeHandle(receiverClass, attributeName);
         AttributeHandleSet publisherAttributes =
            publisher.getAttributeHandleSetFactory().create();
         publisherAttributes.add(publisherAttribute);
         AttributeHandleSet receiverAttributes =
            receiver.getAttributeHandleSetFactory().create();
         receiverAttributes.add(receiverAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         publisher.changeDefaultAttributeOrderType(
            publisherClass, publisherAttributes, OrderType.TIMESTAMP);
         receiver.subscribeObjectClassAttributes(receiverClass, receiverAttributes);
         subscribed = true;
         object = publisher.registerObjectInstance(publisherClass);
         ObjectInstanceHandle registeredObject = object;
         await(callbackModel, receiver, "attribute cutoff object discovery",
            () -> registeredObject.equals(receiverRecorder.discoveredObject));

         LogicalTimeFactory<?, ?> receiverTimeFactory = receiver.getTimeFactory();
         LogicalTimeFactory<?, ?> publisherTimeFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> frontierTimeFactory = frontier.getTimeFactory();
         JavaTckSupport.check(receiverTimeFactory.getName().equals(publisherTimeFactory.getName())
               && receiverTimeFactory.getName().equals(frontierTimeFactory.getName()),
            "attribute cutoff federates selected different logical-time factories");
         LogicalTime receiverInitial = receiver.queryLogicalTime();
         LogicalTime publisherInitial = publisher.queryLogicalTime();
         LogicalTime frontierInitial = frontier.queryLogicalTime();
         JavaTckSupport.check(receiverInitial.equals(publisherInitial)
               && receiverInitial.equals(frontierInitial),
            "attribute cutoff federates began at different logical times");
         LogicalTimeInterval receiverEpsilon =
            (LogicalTimeInterval) receiverTimeFactory.makeEpsilon();
         LogicalTimeInterval publisherEpsilon =
            (LogicalTimeInterval) publisherTimeFactory.makeEpsilon();
         LogicalTimeInterval frontierEpsilon =
            (LogicalTimeInterval) frontierTimeFactory.makeEpsilon();

         receiver.enableTimeConstrained();
         receiverConstrained = true;
         await(callbackModel, receiver, "receiver Time Constrained callback",
            () -> receiverRecorder.timeConstrainedEnabled);
         frontier.enableTimeRegulation(frontierEpsilon);
         frontierRegulating = true;
         await(callbackModel, frontier, "frontier Time Regulation callback",
            () -> frontierRecorder.timeRegulationEnabled);

         LogicalTime receiverAtFive = offset(receiverInitial, receiverEpsilon, 5);
         LogicalTime frontierAtFive = offset(frontierInitial, frontierEpsilon, 5);
         frontier.timeAdvanceRequest(frontierAtFive);
         await(callbackModel, frontier, "frontier TAR(5) grant",
            () -> frontierRecorder.timeAdvanceGrantTimes.size() == 1);
         receiver.timeAdvanceRequest(receiverAtFive);
         await(callbackModel, receiver, "completed receiver TAR(5) grant",
            () -> receiverRecorder.timeAdvanceGrantTimes.size() == 1);
         JavaTckSupport.check(receiverAtFive.equals(receiverRecorder.timeAdvanceGrantTimes.get(0))
               && receiverRecorder.reflections.isEmpty(),
            "receiver did not complete TAR(5) without an attribute reflection");

         // This producer joins the regulating set only after the receiver has
         // completed TAR(5), so its timestamp-4/5 updates arrive after that grant.
         publisher.enableTimeRegulation(publisherEpsilon);
         publisherRegulating = true;
         await(callbackModel, publisher, "late publisher Time Regulation callback",
            () -> publisherRecorder.timeRegulationEnabled);

         LogicalTime publisherAtFour = offset(publisherInitial, publisherEpsilon, 4);
         LogicalTime publisherAtFive = offset(publisherInitial, publisherEpsilon, 5);
         LogicalTime publisherAtSix = offset(publisherInitial, publisherEpsilon, 6);
         LogicalTime frontierAtSix = offset(frontierInitial, frontierEpsilon, 6);
         LogicalTime receiverAtSix = offset(receiverInitial, receiverEpsilon, 6);
         byte[] belowValue = new byte[] {0x41, 0x54, 0x34};
         AttributeHandleValueMap belowValues =
            publisher.getAttributeHandleValueMapFactory().create(1);
         belowValues.put(publisherAttribute, belowValue);
         try {
            MessageRetractionReturn below = publisher.updateAttributeValues(
               object, belowValues, new byte[] {'b', 'e', 'l', 'o', 'w'}, publisherAtFour);
            JavaTckSupport.check(below != null && below.retractionHandleIsValid
                  && below.handle != null,
               "accepted timestamp-4 attribute update returned no valid retraction handle");
         } catch (InvalidLogicalTime allowedBelowBoundaryRejection) {
            // The assertion is about delivery after the grant, not sender acceptance.
         }

         byte[] equalValue = new byte[] {0x41, 0x54, 0x35};
         AttributeHandleValueMap equalValues =
            publisher.getAttributeHandleValueMapFactory().create(1);
         equalValues.put(publisherAttribute, equalValue);
         MessageRetractionReturn equal = publisher.updateAttributeValues(
            object, equalValues, new byte[] {'e', 'q', 'u', 'a', 'l'}, publisherAtFive);
         JavaTckSupport.check(equal != null && equal.retractionHandleIsValid && equal.handle != null,
            "timestamp-5 attribute update returned no valid retraction handle");
         JavaTckSupport.check(receiverRecorder.reflections.isEmpty(),
            "late timestamped attribute update crossed completed TAR(5) immediately");

         publisher.timeAdvanceRequest(publisherAtFour);
         await(callbackModel, publisher, "publisher TAR(4) grant",
            () -> publisherRecorder.timeAdvanceGrantTimes.size() == 1);
         JavaTckSupport.check(receiverRecorder.reflections.isEmpty(),
            "timestamped attribute update at or below 5 arrived while publisher advanced to 4");

         publisher.timeAdvanceRequest(publisherAtSix);
         frontier.timeAdvanceRequest(frontierAtSix);
         await(callbackModel, "producer frontiers at 6",
            () -> publisherRecorder.timeAdvanceGrantTimes.size() == 2
               && frontierRecorder.timeAdvanceGrantTimes.size() == 2,
            publisher, frontier);
         receiver.timeAdvanceRequest(receiverAtSix);
         await(callbackModel, "receiver TAR(6) grant",
            () -> receiverRecorder.timeAdvanceGrantTimes.size() == 2,
            receiver, publisher, frontier);
         JavaTckSupport.check(receiverRecorder.reflections.isEmpty(),
            "an attribute update at or below completed TAR(5) was delivered later");

         LogicalTime publisherAtSeven = offset(publisherInitial, publisherEpsilon, 7);
         LogicalTime publisherAtEight = offset(publisherInitial, publisherEpsilon, 8);
         LogicalTime frontierAtEight = offset(frontierInitial, frontierEpsilon, 8);
         LogicalTime receiverAtEight = offset(receiverInitial, receiverEpsilon, 8);
         byte[] positiveValue = new byte[] {0x41, 0x54, 0x37};
         AttributeHandleValueMap positiveValues =
            publisher.getAttributeHandleValueMapFactory().create(1);
         positiveValues.put(publisherAttribute, positiveValue);
         MessageRetractionReturn positive = publisher.updateAttributeValues(
            object, positiveValues, new byte[] {'a', 'f', 't', 'e', 'r'}, publisherAtSeven);
         JavaTckSupport.check(positive != null && positive.retractionHandleIsValid
               && positive.handle != null,
            "timestamp-7 positive control returned no valid retraction handle");
         publisher.timeAdvanceRequest(publisherAtEight);
         frontier.timeAdvanceRequest(frontierAtEight);
         await(callbackModel, "positive-control producer frontiers at 8",
            () -> publisherRecorder.timeAdvanceGrantTimes.size() == 3
               && frontierRecorder.timeAdvanceGrantTimes.size() == 3,
            publisher, frontier);
         receiver.timeAdvanceRequest(receiverAtEight);
         await(callbackModel, "positive-control receiver TAR(8)",
            () -> receiverRecorder.timeAdvanceGrantTimes.size() == 3,
            receiver, publisher, frontier);

         JavaTckSupport.check(receiverRecorder.reflections.size() == 1,
            "post-cutoff positive-control attribute update was not reflected exactly once");
         Reflection reflection = receiverRecorder.reflections.get(0);
         JavaTckSupport.check(object.equals(reflection.object)
               && reflection.values.size() == 1
               && Arrays.equals(positiveValue, reflection.values.get(receiverAttribute))
               && Arrays.equals(new byte[] {'a', 'f', 't', 'e', 'r'}, reflection.tag)
               && publisherFederate.equals(reflection.producer)
               && publisherAtSeven.equals(reflection.time)
               && reflection.sentOrder == OrderType.TIMESTAMP
               && reflection.receivedOrder == OrderType.TIMESTAMP,
            "post-cutoff timestamp-7 reflection returned incorrect attribute data or time");
      } finally {
         if (receiverConstrained && receiverJoined) {
            try { receiver.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (publisherRegulating && publisherJoined) {
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (frontierRegulating && frontierJoined) {
            try { frontier.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (subscribed && receiverJoined && receiverClass != null && receiverAttribute != null) {
            try {
               AttributeHandleSet attributes = receiver.getAttributeHandleSetFactory().create();
               attributes.add(receiverAttribute);
               receiver.unsubscribeObjectClassAttributes(receiverClass, attributes);
            } catch (Exception ignored) { }
         }
         if (published && publisherJoined && publisherClass != null && publisherAttribute != null) {
            try {
               AttributeHandleSet attributes = publisher.getAttributeHandleSetFactory().create();
               attributes.add(publisherAttribute);
               publisher.unpublishObjectClassAttributes(publisherClass, attributes);
            } catch (Exception ignored) { }
         }
         if (frontierJoined) resign(frontier, ResignAction.NO_ACTION);
         if (publisherJoined) resign(publisher, ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         if (receiverJoined) resign(receiver, ResignAction.NO_ACTION);
         if (created) {
            try { receiver.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (frontierConnected) disconnect(frontier);
         if (publisherConnected) disconnect(publisher);
         if (receiverConnected) disconnect(receiver);
      }
   }

   @FunctionalInterface
   private interface Condition { boolean isMet() throws Exception; }

   private static void await(CallbackModel model, String description, Condition condition,
         RTIambassador... ambassadors) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.isMet()) {
         for (RTIambassador ambassador : ambassadors) {
            if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         }
         Thread.yield();
      }
      JavaTckSupport.check(condition.isMet(), "timed out waiting for " + description);
   }

   private static void await(CallbackModel model, RTIambassador ambassador,
         String description, Condition condition) throws Exception {
      await(model, description, condition, new RTIambassador[] {ambassador});
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval,
         int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static void resign(RTIambassador ambassador, ResignAction action) {
      try { ambassador.resignFederationExecution(action); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); }
      catch (Exception ignored) { }
   }

   private static Object objectMethod(Object proxy, String name, Object[] arguments) {
      if ("toString".equals(name)) return "Java TCK timestamped attribute cutoff callbacks";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
      return null;
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final AttributeHandleValueMap values;
      private final byte[] tag;
      private final FederateHandle producer;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;

      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap values, byte[] tag,
            FederateHandle producer, LogicalTime<?, ?> time, OrderType sentOrder,
            OrderType receivedOrder) {
         this.object = object;
         this.values = values;
         this.tag = tag;
         this.producer = producer;
         this.time = time;
         this.sentOrder = sentOrder;
         this.receivedOrder = receivedOrder;
      }
   }

   private static final class Recorder {
      private volatile ObjectInstanceHandle discoveredObject;
      private volatile boolean timeConstrainedEnabled;
      private volatile boolean timeRegulationEnabled;
      private final List<LogicalTime<?, ?>> timeAdvanceGrantTimes =
         new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveredObject = (ObjectInstanceHandle) arguments[0];
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
               } else if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  timeAdvanceGrantTimes.add((LogicalTime<?, ?>) arguments[0]);
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 10) {
                  AttributeHandleValueMap values =
                     ((AttributeHandleValueMap) arguments[1]).clone();
                  reflections.add(new Reflection(
                     (ObjectInstanceHandle) arguments[0],
                     values,
                     ((byte[]) arguments[2]).clone(),
                     (FederateHandle) arguments[4],
                     (LogicalTime<?, ?>) arguments[6],
                     (OrderType) arguments[7],
                     (OrderType) arguments[8]));
               }
               return objectMethod(proxy, name, arguments);
            });
      }
   }
}
