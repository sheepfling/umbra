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
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks receiver-dependent order metadata for one sent TSO attribute update. */
final class TimestampedAttributePerRecipientOrderConversionTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private TimestampedAttributePerRecipientOrderConversionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador sender = factory.getRtiAmbassador();
      RTIambassador constrained = factory.getRtiAmbassador();
      RTIambassador unconstrained = factory.getRtiAmbassador();
      Recorder senderCallbacks = new Recorder();
      Recorder constrainedCallbacks = new Recorder();
      Recorder unconstrainedCallbacks = new Recorder();
      String federation = "java-tck-tso-attribute-recipient-" + UUID.randomUUID();
      boolean senderConnected = false;
      boolean constrainedConnected = false;
      boolean unconstrainedConnected = false;
      boolean created = false;
      boolean senderJoined = false;
      boolean constrainedJoined = false;
      boolean unconstrainedJoined = false;
      boolean published = false;
      boolean constrainedSubscribed = false;
      boolean unconstrainedSubscribed = false;
      boolean timeConstrained = false;
      boolean regulating = false;
      ObjectClassHandle senderClass = null;
      ObjectClassHandle constrainedClass = null;
      ObjectClassHandle unconstrainedClass = null;
      AttributeHandle senderAttribute = null;
      AttributeHandle constrainedAttribute = null;
      AttributeHandle unconstrainedAttribute = null;
      AttributeHandleSet senderAttributes = null;
      AttributeHandleSet constrainedAttributes = null;
      AttributeHandleSet unconstrainedAttributes = null;
      try {
         sender.connect(senderCallbacks.proxy(), callbackModel);
         senderConnected = true;
         constrained.connect(constrainedCallbacks.proxy(), callbackModel);
         constrainedConnected = true;
         unconstrained.connect(unconstrainedCallbacks.proxy(), callbackModel);
         unconstrainedConnected = true;
         sender.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = sender.joinFederationExecution(
            "java-tck-tso-attribute-sender", federation);
         senderJoined = true;
         constrained.joinFederationExecution(
            "java-tck-tso-attribute-constrained", federation);
         constrainedJoined = true;
         unconstrained.joinFederationExecution(
            "java-tck-tso-attribute-unconstrained", federation);
         unconstrainedJoined = true;

         senderClass = sender.getObjectClassHandle(objectClassName);
         constrainedClass = constrained.getObjectClassHandle(objectClassName);
         unconstrainedClass = unconstrained.getObjectClassHandle(objectClassName);
         senderAttribute = sender.getAttributeHandle(senderClass, attributeName);
         constrainedAttribute = constrained.getAttributeHandle(
            constrainedClass, attributeName);
         unconstrainedAttribute = unconstrained.getAttributeHandle(
            unconstrainedClass, attributeName);
         JavaTckSupport.check(senderClass != null && constrainedClass != null
               && unconstrainedClass != null && senderAttribute != null
               && constrainedAttribute != null && unconstrainedAttribute != null
               && senderClass.equals(constrainedClass)
               && senderClass.equals(unconstrainedClass)
               && senderAttribute.equals(constrainedAttribute)
               && senderAttribute.equals(unconstrainedAttribute)
               && objectClassName.equals(sender.getObjectClassName(senderClass))
               && attributeName.equals(sender.getAttributeName(senderClass, senderAttribute)),
            "adapter FOM did not resolve common object/attribute handles");

         senderAttributes = JavaTckSupport.attributeSet(sender, senderAttribute);
         constrainedAttributes = JavaTckSupport.attributeSet(
            constrained, constrainedAttribute);
         unconstrainedAttributes = JavaTckSupport.attributeSet(
            unconstrained, unconstrainedAttribute);
         sender.publishObjectClassAttributes(senderClass, senderAttributes);
         published = true;
         constrained.subscribeObjectClassAttributes(
            constrainedClass, constrainedAttributes);
         constrainedSubscribed = true;
         unconstrained.subscribeObjectClassAttributes(
            unconstrainedClass, unconstrainedAttributes);
         unconstrainedSubscribed = true;
         sender.changeDefaultAttributeOrderType(
            senderClass, senderAttributes, OrderType.TIMESTAMP);

         ObjectInstanceHandle object = sender.registerObjectInstance(senderClass);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "timestamped attribute conversion registered an invalid object handle");
         await(constrained, callbackModel, objectClassName,
            () -> constrainedCallbacks.discoveries.contains(object),
            "constrained attribute recipient discovery");
         await(unconstrained, callbackModel, objectClassName,
            () -> unconstrainedCallbacks.discoveries.contains(object),
            "unconstrained attribute recipient discovery");

         LogicalTimeFactory<?, ?> senderFactory = sender.getTimeFactory();
         LogicalTimeFactory<?, ?> constrainedFactory = constrained.getTimeFactory();
         LogicalTimeFactory<?, ?> unconstrainedFactory = unconstrained.getTimeFactory();
         JavaTckSupport.check(senderFactory.getName().equals(constrainedFactory.getName())
               && senderFactory.getName().equals(unconstrainedFactory.getName()),
            "adapter recipients selected different logical-time representations");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) senderFactory.makeEpsilon();
         LogicalTimeInterval lookahead = (LogicalTimeInterval) senderFactory.makeZero();
         for (int index = 0; index < 5; ++index) {
            lookahead = (LogicalTimeInterval) lookahead.add(epsilon);
         }

         constrained.enableTimeConstrained();
         timeConstrained = true;
         await(constrained, callbackModel, objectClassName,
            () -> constrainedCallbacks.timeConstrainedEnabled,
            "constrained attribute recipient enablement");
         sender.enableTimeRegulation(lookahead);
         regulating = true;
         await(sender, callbackModel, objectClassName,
            () -> senderCallbacks.timeRegulationEnabled,
            "timestamped attribute sender regulation enablement");
         senderCallbacks.sequence.clear();
         constrainedCallbacks.sequence.clear();
         unconstrainedCallbacks.sequence.clear();

         LogicalTime initial = sender.queryLogicalTime();
         JavaTckSupport.check(initial.equals(constrained.queryLogicalTime())
               && initial.equals(unconstrained.queryLogicalTime()),
            "adapter recipients began at different logical times");
         LogicalTime messageTime = offset(initial, epsilon, 6);
         LogicalTime senderTarget = offset(initial, epsilon, 2);
         byte[] value = {0x54, 0x53, 0x4f};
         byte[] tag = {0x50, 0x52};
         AttributeHandleValueMap values =
            sender.getAttributeHandleValueMapFactory().create(1);
         values.put(senderAttribute, value.clone());
         MessageRetractionReturn retraction = sender.updateAttributeValues(
            object, values, tag.clone(), messageTime);
         JavaTckSupport.check(retraction != null && retraction.retractionHandleIsValid
               && retraction.handle != null,
            "timestamped Update Attribute Values returned no valid retraction designator");

         await(unconstrained, callbackModel, objectClassName,
            () -> unconstrainedCallbacks.timestamped.size() == 1,
            "unconstrained time-bearing receive-order reflection");
         JavaTckSupport.check(unconstrainedCallbacks.receiveOrder.isEmpty()
               && unconstrainedCallbacks.timeAdvanceGrants.isEmpty()
               && constrainedCallbacks.timestamped.isEmpty()
               && constrainedCallbacks.timeAdvanceGrants.isEmpty()
               && unconstrainedCallbacks.sequence.equals(Arrays.asList("reflect")),
            "timestamped attribute update did not separate unconstrained RO from constrained TSO delivery");
         checkDelivery(unconstrained, unconstrainedCallbacks.timestamped.get(0), object,
            unconstrainedAttribute, producer, OrderType.RECEIVE, messageTime, value,
            tag, retraction.handle, "unconstrained receive-order conversion");

         constrained.timeAdvanceRequest(messageTime);
         sender.timeAdvanceRequest(senderTarget);
         awaitBoth(sender, constrained, callbackModel, objectClassName,
            () -> senderCallbacks.timeAdvanceGrants.size() == 1
               && constrainedCallbacks.timestamped.size() == 1
               && constrainedCallbacks.timeAdvanceGrants.size() == 1,
            "constrained timestamped reflection and matching grants");
         JavaTckSupport.check(constrainedCallbacks.sequence.equals(
               Arrays.asList("reflect", "timeAdvanceGrant")),
            "constrained timestamped reflection did not precede its matching grant");
         JavaTckSupport.check(messageTime.equals(constrainedCallbacks.timeAdvanceGrants.get(0))
               && messageTime.equals(constrained.queryLogicalTime()),
            "constrained recipient received a grant different from its request");
         checkDelivery(constrained, constrainedCallbacks.timestamped.get(0), object,
            constrainedAttribute, producer, OrderType.TIMESTAMP, messageTime, value,
            tag, retraction.handle, "constrained timestamp-order delivery");
         JavaTckSupport.check(unconstrainedCallbacks.timestamped.size() == 1
               && unconstrainedCallbacks.sequence.equals(Arrays.asList("reflect"))
               && unconstrainedCallbacks.timeAdvanceGrants.isEmpty(),
            "unconstrained recipient received a duplicate reflection or grant");
      } finally {
         if (timeConstrained && constrainedJoined) {
            try { constrained.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (regulating && senderJoined) {
            try { sender.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (constrainedSubscribed && constrainedJoined) {
            try { constrained.unsubscribeObjectClassAttributes(
               constrainedClass, constrainedAttributes); } catch (Exception ignored) { }
         }
         if (unconstrainedSubscribed && unconstrainedJoined) {
            try { unconstrained.unsubscribeObjectClassAttributes(
               unconstrainedClass, unconstrainedAttributes); } catch (Exception ignored) { }
         }
         if (published && senderJoined) {
            try { sender.unpublishObjectClassAttributes(
               senderClass, senderAttributes); } catch (Exception ignored) { }
         }
         if (unconstrainedJoined) {
            try { unconstrained.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (constrainedJoined) {
            try { constrained.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (senderJoined) {
            try { sender.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { sender.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (unconstrainedConnected) {
            try { unconstrained.disconnect(); } catch (Exception ignored) { }
         }
         if (constrainedConnected) {
            try { constrained.disconnect(); } catch (Exception ignored) { }
         }
         if (senderConnected) {
            try { sender.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void checkDelivery(RTIambassador receiver, Reflection delivery,
         ObjectInstanceHandle object, AttributeHandle attribute, FederateHandle producer,
         OrderType receivedOrder, LogicalTime<?, ?> messageTime, byte[] value, byte[] tag,
         MessageRetractionHandle retraction, String description) throws Exception {
      TransportationTypeHandle transportation = delivery.transportation;
      JavaTckSupport.check(object.equals(delivery.object) && delivery.values.size() == 1
            && Arrays.equals(value, delivery.values.get(attribute))
            && Arrays.equals(tag, delivery.tag) && producer.equals(delivery.producer)
            && messageTime.equals(delivery.time) && delivery.sentOrder == OrderType.TIMESTAMP
            && delivery.receivedOrder == receivedOrder
            && retraction.equals(delivery.retraction)
            && delivery.regionsEmpty && transportation != null
            && !receiver.getTransportationTypeName(transportation).isEmpty(),
         description + " changed the attribute data or order/retraction metadata");
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) {
         result = (LogicalTime) result.add(interval);
      }
      return result;
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         String objectClassName, BooleanSupplier condition, String description)
         throws Exception {
      awaitBoth(ambassador, null, model, objectClassName, condition, description);
   }

   private static void awaitBoth(RTIambassador first, RTIambassador second,
         CallbackModel model, String objectClassName, BooleanSupplier condition,
         String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(first);
            if (second != null) JavaTckSupport.drain(second);
         } else {
            first.getObjectClassHandle(objectClassName);
            if (second != null) second.getObjectClassHandle(objectClassName);
            Thread.yield();
         }
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final AttributeHandleValueMap values;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final boolean regionsEmpty;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;
      private final MessageRetractionHandle retraction;

      private Reflection(Object[] arguments, boolean timestamped) {
         object = (ObjectInstanceHandle) arguments[0];
         values = ((AttributeHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
         transportation = (TransportationTypeHandle) arguments[3];
         producer = (FederateHandle) arguments[4];
         regionsEmpty = arguments[5] == null
            || ((hla.rti1516_2025.RegionHandleSet) arguments[5]).isEmpty();
         if (timestamped) {
            time = (LogicalTime<?, ?>) arguments[6];
            sentOrder = (OrderType) arguments[7];
            receivedOrder = (OrderType) arguments[8];
            retraction = (MessageRetractionHandle) arguments[9];
         } else {
            time = null;
            sentOrder = null;
            receivedOrder = null;
            retraction = null;
         }
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Reflection> receiveOrder = new CopyOnWriteArrayList<>();
      private final List<Reflection> timestamped = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> timeAdvanceGrants = new CopyOnWriteArrayList<>();
      private final List<String> sequence = new CopyOnWriteArrayList<>();
      private volatile boolean timeConstrainedEnabled;
      private volatile boolean timeRegulationEnabled;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length > 0) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
               } else if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 10) {
                  timestamped.add(new Reflection(arguments, true));
                  sequence.add("reflect");
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 6) {
                  receiveOrder.add(new Reflection(arguments, false));
                  sequence.add("reflect");
               } else if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  timeAdvanceGrants.add((LogicalTime<?, ?>) arguments[0]);
                  sequence.add("timeAdvanceGrant");
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK timestamped attribute callbacks";
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
