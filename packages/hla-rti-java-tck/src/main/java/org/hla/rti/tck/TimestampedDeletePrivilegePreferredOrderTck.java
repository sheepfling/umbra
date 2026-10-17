package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
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
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Checks that timestamped deletion follows the current order of the delete-privilege attribute. */
final class TimestampedDeletePrivilegePreferredOrderTck {
   private TimestampedDeletePrivilegePreferredOrderTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      run(factory, fom, timeImplementation, objectClassName, attributeName,
         callbackModel, OrderType.RECEIVE);
   }

   static void runTimestampOrder(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      run(factory, fom, timeImplementation, objectClassName, attributeName,
         callbackModel, OrderType.TIMESTAMP);
   }

   private static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel,
         OrderType preferredOrder) throws Exception {
      RTIambassador sender = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder senderRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-delete-privilege-order-" + UUID.randomUUID();
      boolean senderConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean senderJoined = false;
      boolean receiverJoined = false;
      boolean regulating = false;
      boolean published = false;
      boolean subscribed = false;
      ObjectClassHandle senderClass = null;
      ObjectClassHandle receiverClass = null;
      AttributeHandleSet senderAttributes = null;
      AttributeHandleSet receiverAttributes = null;
      try {
         sender.connect(senderRecorder.proxy(), callbackModel);
         senderConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         sender.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = sender.joinFederationExecution(
            "java-tck-delete-order-sender", federation);
         senderJoined = true;
         receiver.joinFederationExecution("java-tck-delete-order-receiver", federation);
         receiverJoined = true;

         senderClass = sender.getObjectClassHandle(objectClassName);
         receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle senderAttribute = sender.getAttributeHandle(senderClass, attributeName);
         AttributeHandle receiverAttribute = receiver.getAttributeHandle(receiverClass, attributeName);
         AttributeHandle privilegeToDelete = sender.getAttributeHandle(
            senderClass, "HLAprivilegeToDeleteObject");
         JavaTckSupport.check(senderClass != null && receiverClass != null
               && senderClass.equals(receiverClass) && senderAttribute != null
               && receiverAttribute != null && senderAttribute.equals(receiverAttribute)
               && privilegeToDelete != null,
            "adapter FOM did not provide the configured object and standard delete-privilege attribute");

         senderAttributes = JavaTckSupport.attributeSet(sender, senderAttribute);
         receiverAttributes = JavaTckSupport.attributeSet(receiver, receiverAttribute);
         sender.publishObjectClassAttributes(senderClass, senderAttributes);
         published = true;
         receiver.subscribeObjectClassAttributes(receiverClass, receiverAttributes);
         subscribed = true;
         ObjectInstanceHandle object = sender.registerObjectInstance(senderClass);
         JavaTckSupport.check(object != null,
            "timestamped delete registration returned no object handle");
         await(receiver, callbackModel,
            () -> receiverRecorder.discoveries.contains(object),
            "timestamped delete object discovery");

         LogicalTimeFactory<?, ?> timeFactory = sender.getTimeFactory();
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         LogicalTime<?, ?> initial = sender.queryLogicalTime();
         sender.enableTimeRegulation(epsilon);
         regulating = true;
         await(sender, callbackModel,
            () -> senderRecorder.timeRegulationEnabled != null,
            "timestamped delete sender regulation callback");

         AttributeHandleSet deletePrivilegeSet = JavaTckSupport.attributeSet(
            sender, privilegeToDelete);
         sender.changeAttributeOrderType(object, deletePrivilegeSet, preferredOrder);
         LogicalTime<?, ?> timestamp = offset(initial, epsilon, 2);
         byte[] tag = new byte[] {0x50, 0x52, 0x45, 0x46};
         MessageRetractionReturn retraction = sender.deleteObjectInstance(
            object, tag.clone(), timestamp);
          boolean timestampPreferred = preferredOrder == OrderType.TIMESTAMP;
          JavaTckSupport.check(retraction != null
                && retraction.retractionHandleIsValid == timestampPreferred
                && (timestampPreferred ? retraction.handle != null : retraction.handle == null),
             "timestamped deletion returned a retraction designator inconsistent with the preferred order");

         await(receiver, callbackModel,
            () -> receiverRecorder.timestampedRemovals.size() == 1,
             "order-preferred timestamped object removal");
         Removal removal = receiverRecorder.timestampedRemovals.get(0);
         JavaTckSupport.check(receiverRecorder.timestampedRemovals.size() == 1
               && receiverRecorder.untimedRemovals.isEmpty()
               && object.equals(removal.object) && Arrays.equals(tag, removal.tag)
               && producer.equals(removal.producer) && timestamp.equals(removal.time)
               && removal.sentOrder == preferredOrder
                && removal.receivedOrder == (timestampPreferred
                      ? OrderType.RECEIVE : preferredOrder)
                && (timestampPreferred
                      ? retraction.handle.equals(removal.retraction)
                      : removal.retraction == null)
               && receiverRecorder.callbackSequence.size() == 1
               && "removeObjectInstance".equals(receiverRecorder.callbackSequence.get(0)),
            "timestamped deletion did not preserve the privilege attribute's preferred order and message data");
      } finally {
         if (regulating && senderJoined) {
            try { sender.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (subscribed && receiverJoined && receiverClass != null && receiverAttributes != null) {
            try { receiver.unsubscribeObjectClassAttributes(receiverClass, receiverAttributes); }
            catch (Exception ignored) { }
         }
         if (published && senderJoined && senderClass != null && senderAttributes != null) {
            try { sender.unpublishObjectClassAttributes(senderClass, senderAttributes); }
            catch (Exception ignored) { }
         }
         if (receiverJoined) {
            try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (senderJoined) {
            try { sender.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { sender.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (receiverConnected) {
            try { receiver.disconnect(); } catch (Exception ignored) { }
         }
         if (senderConnected) {
            try { sender.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval,
         int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) {
         result = (LogicalTime) result.add(interval);
      }
      return result;
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         Condition condition, String description) throws Exception {
      long deadline = System.nanoTime() + 15_000_000_000L;
      while (System.nanoTime() < deadline) {
         if (condition.isMet()) return;
         if (model == CallbackModel.HLA_EVOKED) {
            ambassador.evokeMultipleCallbacks(0.0, 0.0);
         } else {
            Thread.yield();
         }
      }
      JavaTckSupport.check(condition.isMet(), "timed out waiting for " + description);
   }

   @FunctionalInterface
   private interface Condition { boolean isMet() throws Exception; }

   private static final class Removal {
      private ObjectInstanceHandle object;
      private byte[] tag;
      private FederateHandle producer;
      private LogicalTime<?, ?> time;
      private OrderType sentOrder;
      private OrderType receivedOrder;
      private MessageRetractionHandle retraction;
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Removal> timestampedRemovals = new CopyOnWriteArrayList<>();
      private final List<Boolean> untimedRemovals = new CopyOnWriteArrayList<>();
      private final List<String> callbackSequence = new CopyOnWriteArrayList<>();
      private volatile LogicalTime<?, ?> timeRegulationEnabled;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = (LogicalTime<?, ?>) arguments[0];
               } else if ("removeObjectInstance".equals(name) && arguments != null
                     && arguments.length == 7) {
                  Removal removal = new Removal();
                  removal.object = (ObjectInstanceHandle) arguments[0];
                  removal.tag = ((byte[]) arguments[1]).clone();
                  removal.producer = (FederateHandle) arguments[2];
                  removal.time = (LogicalTime<?, ?>) arguments[3];
                  removal.sentOrder = (OrderType) arguments[4];
                  removal.receivedOrder = (OrderType) arguments[5];
                  removal.retraction = (MessageRetractionHandle) arguments[6];
                  timestampedRemovals.add(removal);
                  callbackSequence.add(name);
               } else if ("removeObjectInstance".equals(name) && arguments != null
                     && arguments.length == 3) {
                  untimedRemovals.add(Boolean.TRUE);
                  callbackSequence.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK preferred-order removal recorder";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == arguments[0];
               }
               return null;
            });
      }
   }
}
