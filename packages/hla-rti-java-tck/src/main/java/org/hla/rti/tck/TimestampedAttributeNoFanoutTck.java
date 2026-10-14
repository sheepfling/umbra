package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Verifies timestamped attribute retraction and terminalization with no eligible subscriber. */
final class TimestampedAttributeNoFanoutTck {
   private TimestampedAttributeNoFanoutTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      Recorder callbacks = new Recorder();
      String federation = "java-tck-timestamped-attribute-no-fanout-" + UUID.randomUUID();
      boolean connected = false, created = false, joined = false, published = false;
      ObjectClassHandle objectClass = null;
      AttributeHandle attribute = null;
      AttributeHandleSet attributes = null;
      try {
         publisher.connect(callbacks.proxy(), callbackModel);
         connected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         publisher.joinFederationExecution("java-tck-no-fanout-owner", federation);
         joined = true;

         objectClass = publisher.getObjectClassHandle(objectClassName);
         attribute = publisher.getAttributeHandle(objectClass, attributeName);
         JavaTckSupport.check(objectClass != null && attribute != null
               && objectClassName.equals(publisher.getObjectClassName(objectClass))
               && attributeName.equals(publisher.getAttributeName(objectClass, attribute)),
            "adapter FOM did not resolve and round-trip no-fanout attribute handles");
         attributes = JavaTckSupport.attributeSet(publisher, attribute);
         publisher.publishObjectClassAttributes(objectClass, attributes);
         published = true;
         publisher.changeDefaultAttributeOrderType(objectClass, attributes, OrderType.TIMESTAMP);
         ObjectInstanceHandle object = publisher.registerObjectInstance(objectClass);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "timestamped no-fanout registration returned an invalid object handle");

         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         LogicalTimeInterval lookahead = (LogicalTimeInterval) timeFactory.makeEpsilon();
         publisher.enableTimeRegulation(lookahead);
         await(publisher, callbackModel, () -> callbacks.timeRegulationEnabled,
            "publisher time-regulation callback");

         LogicalTime messageTime = offset(publisher.queryLogicalTime(), lookahead, 3);
         byte[] value = new byte[] {0x41, 0x42, 0x43};
         byte[] tag = new byte[] {0x4e, 0x46};
         MessageRetractionReturn first = update(
            publisher, object, attribute, value, tag, messageTime);
         requireValid(first, "first timestamped attribute update without subscribers");
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(callbacks.timestampedReflections == 0
               && callbacks.retractionCallbacks == 0,
            "timestamped no-fanout update produced a local callback immediately after send");

         publisher.retract(first.handle);
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(callbacks.timestampedReflections == 0
               && callbacks.retractionCallbacks == 0,
            "timestamped no-fanout retraction produced a local callback");
         JavaTckSupport.expectFailure(() -> publisher.retract(first.handle),
            "MessageCanNoLongerBeRetracted",
            "retracting the already retracted no-fanout attribute update");

         MessageRetractionReturn second = update(
            publisher, object, attribute, value, tag, messageTime);
         requireValid(second, "second timestamped attribute update without subscribers");
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(callbacks.timestampedReflections == 0
               && callbacks.retractionCallbacks == 0,
            "second no-fanout update produced a local callback before the grant");
         publisher.timeAdvanceRequest(messageTime);
         await(publisher, callbackModel, () -> !callbacks.grants.isEmpty(),
            "publisher time-advance grant");
         JavaTckSupport.check(callbacks.grants.size() == 1
               && messageTime.equals(callbacks.grants.get(0))
               && messageTime.equals(publisher.queryLogicalTime()),
            "publisher did not receive the requested logical-time grant");
         JavaTckSupport.check(callbacks.timestampedReflections == 0
               && callbacks.retractionCallbacks == 0,
            "timestamped no-fanout produced a local reflection or retraction callback");
         JavaTckSupport.expectFailure(() -> publisher.retract(second.handle),
            "MessageCanNoLongerBeRetracted",
            "retracting a no-fanout update after advancing past its timestamp");

         publisher.disableTimeRegulation();
         if (published) {
            publisher.unpublishObjectClassAttributes(objectClass, attributes);
            published = false;
         }
      } finally {
         if (published && joined) {
            try { publisher.unpublishObjectClassAttributes(objectClass, attributes); }
            catch (Exception ignored) { }
         }
         if (joined) {
            try { publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static MessageRetractionReturn update(RTIambassador publisher,
         ObjectInstanceHandle object, AttributeHandle attribute, byte[] value, byte[] tag,
         LogicalTime<?, ?> time) throws Exception {
      AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(1);
      values.put(attribute, value.clone());
      return publisher.updateAttributeValues(object, values, tag.clone(), time);
   }

   private static void requireValid(MessageRetractionReturn result, String description) {
      JavaTckSupport.check(result != null && result.retractionHandleIsValid && result.handle != null,
         description + " returned no valid public retraction handle");
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         java.util.function.BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static final class Recorder {
      private volatile boolean timeRegulationEnabled;
      private volatile int timestampedReflections;
      private volatile int retractionCallbacks;
      private final java.util.List<LogicalTime<?, ?>> grants =
         new java.util.concurrent.CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("reflectAttributeValues".equals(name)
                     && arguments != null && arguments.length == 10) {
                  ++timestampedReflections;
               } else if ("requestRetraction".equals(name)) {
                  ++retractionCallbacks;
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK attribute no-fanout callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
