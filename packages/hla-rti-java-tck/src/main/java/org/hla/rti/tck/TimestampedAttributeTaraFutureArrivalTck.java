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
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TimeQueryReturn;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** A TSO attribute update arriving while TARA or NMR is pending. */
final class TimestampedAttributeTaraFutureArrivalTck {
   private TimestampedAttributeTaraFutureArrivalTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      runMode(factory, fom, timeImplementation, objectClassName, attributeName,
         callbackModel, false);
   }

   static void runNmr(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      runMode(factory, fom, timeImplementation, objectClassName, attributeName,
         callbackModel, true);
   }

   private static void runMode(RtiFactory factory, String fom,
         String timeImplementation, String objectClassName, String attributeName,
         CallbackModel callbackModel, boolean nmr) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-attribute-" + (nmr ? "nmr" : "tara")
         + "-future-" + UUID.randomUUID();
      boolean publisherConnected = false, receiverConnected = false;
      boolean created = false, publisherJoined = false, receiverJoined = false;
      boolean published = false, subscribed = false;
      boolean regulating = false, constrained = false;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-attribute-tara-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-attribute-tara-receiver", federation);
         receiverJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         AttributeHandle receiverAttribute = receiver.getAttributeHandle(
            receiverClass, attributeName);
         JavaTckSupport.check(publisherClass.equals(receiverClass)
               && publisherAttribute.equals(receiverAttribute),
            "adapter FOM produced inconsistent object or attribute handles");
         AttributeHandleSet publishedAttributes = JavaTckSupport.attributeSet(
            publisher, publisherAttribute);
         AttributeHandleSet subscribedAttributes = JavaTckSupport.attributeSet(
            receiver, receiverAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publishedAttributes);
         published = true;
         publisher.changeDefaultAttributeOrderType(
            publisherClass, publishedAttributes, OrderType.TIMESTAMP);
         receiver.subscribeObjectClassAttributes(receiverClass, subscribedAttributes);
         subscribed = true;

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         JavaTckSupport.check(publisherFactory.getName().equals(receiverFactory.getName()),
            "federates selected different logical-time factories");
         LogicalTime<?, ?> initial = publisher.queryLogicalTime();
         LogicalTime<?, ?> receiverInitial = receiver.queryLogicalTime();
         JavaTckSupport.check(initial.equals(receiverInitial),
            "federates began at different logical times");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) publisherFactory.makeEpsilon();
         receiver.enableTimeConstrained();
         constrained = true;
         publisher.enableTimeRegulation(nmr ? intervalOffset(epsilon, 5) : epsilon);
         regulating = true;
         await(() -> receiverRecorder.constrained && publisherRecorder.regulating,
            publisher, receiver, "time-role callbacks");

         LogicalTime<?, ?> publisherCurrent = offset(initial, epsilon, 2);
         publisher.timeAdvanceRequest(publisherCurrent);
         await(() -> publisherRecorder.grants.size() == 1,
            publisher, receiver, "regulator's initial grant");
         JavaTckSupport.check(publisherCurrent.equals(publisherRecorder.grants.get(0)),
            "regulator advanced to the wrong logical time");
         LogicalTime<?, ?> updateTime = offset(publisherCurrent, epsilon,
            nmr ? 5 : 3);
         LogicalTime<?, ?> requestBoundary = offset(receiverInitial,
            (LogicalTimeInterval) receiverFactory.makeEpsilon(), nmr ? 10 : 6);
         if (nmr) {
            TimeQueryReturn galt = receiver.queryGALT();
            JavaTckSupport.check(galt.timeIsValid && updateTime.equals(galt.time),
               "NMR receiver did not observe the defined pre-request GALT");
         }

         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         await(() -> receiverRecorder.discovery != null,
            publisher, receiver, "object discovery");
         Discovery discovery = receiverRecorder.discovery;
         JavaTckSupport.check(object.equals(discovery.object)
               && receiverClass.equals(discovery.objectClass)
               && discovery.name != null && !discovery.name.isEmpty()
               && producer.equals(discovery.producer),
            "receiver discovered the wrong object, class, or producer");

         receiverRecorder.callbackOrder.clear();
         if (nmr) receiver.nextMessageRequest(requestBoundary);
         else receiver.timeAdvanceRequestAvailable(requestBoundary);
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRecorder.reflections.isEmpty()
               && receiverRecorder.grants.isEmpty(),
            "request completed before the future attribute update arrived");

         byte[] value = nmr
            ? new byte[] {0x4e, 0x4d, 0x52, 0x37}
            : new byte[] {0x54, 0x41, 0x52, 0x41, 0x35};
         byte[] tag = nmr
            ? new byte[] {0x4e, 0x4d, 0x52}
            : new byte[] {0x54, 0x41, 0x52, 0x41};
         AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(1);
         values.put(publisherAttribute, value);
         MessageRetractionReturn sent = publisher.updateAttributeValues(
            object, values, tag.clone(), updateTime);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid
               && sent.handle != null,
            "timestamped update returned no valid retraction handle");
         if (!nmr) {
            JavaTckSupport.check(receiverRecorder.reflections.isEmpty(),
               "future reflection arrived before the regulator advanced");
         }

         if (!nmr) {
            LogicalTime<?, ?> publisherAdvance = offset(publisherCurrent,
               epsilon, 4);
            publisher.timeAdvanceRequest(publisherAdvance);
            await(() -> publisherRecorder.grants.size() == 2,
               publisher, receiver, "regulator's follow-up grant");
            JavaTckSupport.check(publisherAdvance.equals(publisherRecorder.grants.get(1)),
               "regulator's follow-up grant had the wrong logical time");
         }
         await(() -> receiverRecorder.reflections.size() >= 1
               && receiverRecorder.grants.size() >= 1,
            publisher, receiver, "reflection and TARA grant");
         JavaTckSupport.check(receiverRecorder.reflections.size() == 1
               && receiverRecorder.grants.size() == 1
               && receiverRecorder.callbackOrder.equals(Arrays.asList(
                  "reflectAttributeValues", "timeAdvanceGrant")),
            "receiver granted before its single future reflection");

         Reflection reflection = receiverRecorder.reflections.get(0);
         JavaTckSupport.check(object.equals(reflection.object)
               && reflection.values.size() == 1
               && Arrays.equals(value, reflection.values.get(receiverAttribute))
               && Arrays.equals(tag, reflection.tag)
               && producer.equals(reflection.producer),
            "reflection returned the wrong object, value, tag, or producer");
         JavaTckSupport.check(updateTime.equals(reflection.time)
               && reflection.sentOrder == OrderType.TIMESTAMP
               && reflection.receivedOrder == OrderType.TIMESTAMP
               && sent.handle.equals(reflection.retraction)
               && reflection.transportation != null
               && !receiver.getTransportationTypeName(reflection.transportation).isEmpty(),
            "reflection returned incorrect time, order, retraction, or transport metadata");
         LogicalTime<?, ?> expectedGrant = nmr ? updateTime : requestBoundary;
         JavaTckSupport.check(expectedGrant.equals(receiverRecorder.grants.get(0))
               && expectedGrant.equals(receiver.queryLogicalTime()),
            "request did not grant and advance to the expected time");

         receiver.disableTimeConstrained();
         constrained = false;
         publisher.disableTimeRegulation();
         regulating = false;
         receiver.unsubscribeObjectClassAttributes(receiverClass, subscribedAttributes);
         subscribed = false;
         publisher.unpublishObjectClassAttributes(publisherClass, publishedAttributes);
         published = false;
         publisher.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         publisherJoined = false;
         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (constrained && receiverJoined) {
            try { receiver.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (regulating && publisherJoined) {
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (subscribed && receiverJoined) {
            try {
               ObjectClassHandle cls = receiver.getObjectClassHandle(objectClassName);
               receiver.unsubscribeObjectClassAttributes(cls, JavaTckSupport.attributeSet(
                  receiver, receiver.getAttributeHandle(cls, attributeName)));
            } catch (Exception ignored) { }
         }
         if (published && publisherJoined) {
            try {
               ObjectClassHandle cls = publisher.getObjectClassHandle(objectClassName);
               publisher.unpublishObjectClassAttributes(cls, JavaTckSupport.attributeSet(
                  publisher, publisher.getAttributeHandle(cls, attributeName)));
            } catch (Exception ignored) { }
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(
               ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); } catch (Exception ignored) { }
         }
         if (receiverJoined) {
            try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
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

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTimeInterval intervalOffset(LogicalTimeInterval epsilon,
         int steps) throws Exception {
      LogicalTimeInterval result = epsilon;
      for (int i = 1; i < steps; ++i) {
         result = (LogicalTimeInterval) result.add(epsilon);
      }
      return result;
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval epsilon,
         int steps) throws Exception {
      LogicalTime time = start;
      for (int i = 0; i < steps; ++i) time = (LogicalTime) time.add(epsilon);
      return time;
   }

   private static void await(Ready ready, RTIambassador publisher,
         RTIambassador receiver, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline) {
         JavaTckSupport.drain(publisher);
         JavaTckSupport.drain(receiver);
         if (ready.isReady()) return;
         Thread.sleep(1L);
      }
      throw new AssertionError(description + " timed out");
   }

   @FunctionalInterface
   private interface Ready { boolean isReady(); }

   private static final class Discovery {
      private ObjectInstanceHandle object;
      private ObjectClassHandle objectClass;
      private String name;
      private FederateHandle producer;
   }

   private static final class Reflection {
      private ObjectInstanceHandle object;
      private AttributeHandleValueMap values;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private LogicalTime<?, ?> time;
      private OrderType sentOrder;
      private OrderType receivedOrder;
      private MessageRetractionHandle retraction;
   }

   private static final class Recorder {
      private volatile boolean regulating;
      private volatile boolean constrained;
      private volatile Discovery discovery;
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) regulating = true;
               else if ("timeConstrainedEnabled".equals(name)) constrained = true;
               else if ("discoverObjectInstance".equals(name)
                     && arguments != null && arguments.length == 4) {
                  Discovery event = new Discovery();
                  event.object = (ObjectInstanceHandle) arguments[0];
                  event.objectClass = (ObjectClassHandle) arguments[1];
                  event.name = (String) arguments[2];
                  event.producer = (FederateHandle) arguments[3];
                  discovery = event;
               } else if ("reflectAttributeValues".equals(name)
                     && arguments != null && arguments.length == 10) {
                  Reflection event = new Reflection();
                  event.object = (ObjectInstanceHandle) arguments[0];
                  event.values = ((AttributeHandleValueMap) arguments[1]).clone();
                  event.tag = ((byte[]) arguments[2]).clone();
                  event.transportation = (TransportationTypeHandle) arguments[3];
                  event.producer = (FederateHandle) arguments[4];
                  event.time = (LogicalTime<?, ?>) arguments[6];
                  event.sentOrder = (OrderType) arguments[7];
                  event.receivedOrder = (OrderType) arguments[8];
                  event.retraction = (MessageRetractionHandle) arguments[9];
                  reflections.add(event);
                  callbackOrder.add(name);
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK future TARA attribute callbacks";
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
