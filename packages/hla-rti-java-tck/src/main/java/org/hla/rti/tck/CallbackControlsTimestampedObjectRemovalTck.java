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
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;

/** P2 callback-control checks for timestamped object removal and name retirement. */
final class CallbackControlsTimestampedObjectRemovalTck {
   private CallbackControlsTimestampedObjectRemovalTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador constrained = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder constrainedRecorder = new Recorder();
      String federation = "java-tck-callback-tso-removal-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean constrainedConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean constrainedJoined = false;
      boolean regulating = false;
      boolean timeConstrained = false;
      ObjectClassHandle ownerClass = null;
      ObjectClassHandle constrainedClass = null;
      AttributeHandleSet ownerAttributes = null;
      AttributeHandleSet constrainedAttributes = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         constrained.connect(constrainedRecorder.proxy(), callbackModel);
         constrainedConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = owner.joinFederationExecution(
            "java-tck-callback-removal-owner", federation);
         ownerJoined = true;
         constrained.joinFederationExecution("java-tck-callback-removal-constrained", federation);
         constrainedJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         constrainedClass = constrained.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle constrainedAttribute = constrained.getAttributeHandle(
            constrainedClass, attributeName);
         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         constrainedAttributes = JavaTckSupport.attributeSet(constrained, constrainedAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         constrained.subscribeObjectClassAttributes(constrainedClass, constrainedAttributes);

         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         String objectName = owner.getObjectInstanceName(object);
         JavaTckSupport.check(objectName != null && !objectName.isEmpty(),
            "timestamped object registration returned an empty instance name");
         await(callbackModel, () -> object.equals(constrainedRecorder.discoveredObject),
            constrained, objectClassName);

         LogicalTimeFactory<?, ?> timeFactory = owner.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(constrained.getTimeFactory().getName()),
            "time-managed federates selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         owner.enableTimeRegulation(epsilon);
         regulating = true;
         await(callbackModel, () -> ownerRecorder.timeRegulationEnabled != null,
            owner, objectClassName);
         constrained.enableTimeConstrained();
         timeConstrained = true;
         await(callbackModel, () -> constrainedRecorder.timeConstrainedEnabled != null,
            constrained, objectClassName);

         LogicalTime target = add(owner.queryLogicalTime(), intervalOffset(epsilon, 4));
         LogicalTime constrainedTarget = add(
            constrained.queryLogicalTime(), intervalOffset(epsilon, 4));
         JavaTckSupport.check(target.equals(constrainedTarget),
            "time-managed federates derived different removal targets");
         byte[] tag = new byte[] {0x52, 0x4d, 0x56};
         constrained.disableCallbacks();
         MessageRetractionReturn retraction = owner.deleteObjectInstance(object, tag, target);
         JavaTckSupport.check(retraction != null && retraction.retractionHandleIsValid
               && retraction.handle != null,
            "timestamped Delete Object Instance returned an invalid retraction handle");
         constrained.timeAdvanceRequest(constrainedTarget);
         owner.timeAdvanceRequest(target);
         await(callbackModel, () -> ownerRecorder.timeAdvanceGrant != null,
            owner, objectClassName);

         service(constrained, callbackModel, objectClassName);
         JavaTckSupport.check(constrainedRecorder.removals.isEmpty()
               && constrainedRecorder.timeAdvanceGrant == null,
            "disabled callbacks exposed timestamped removal or its grant");
         JavaTckSupport.check(object.equals(constrained.getObjectInstanceHandle(objectName)),
            "disabled timestamped removal retired the object name before delivery");

         constrained.enableCallbacks();
         await(callbackModel, () -> constrainedRecorder.removals.size() == 1
               && constrainedRecorder.timeAdvanceGrant != null,
            constrained, objectClassName);
         JavaTckSupport.check(constrainedRecorder.removals.size() == 1,
            "re-enabled callback controls delivered duplicate timestamped removals");
         Removal removal = constrainedRecorder.removals.get(0);
         JavaTckSupport.check(object.equals(removal.object)
               && Arrays.equals(tag, removal.tag)
               && producer.equals(removal.producer)
               && target.equals(removal.time)
               && removal.sentOrder == OrderType.TIMESTAMP
               && removal.receivedOrder == OrderType.TIMESTAMP
               && retraction.handle.equals(removal.retraction),
            "re-enabled timestamped removal returned incorrect callback metadata");
         JavaTckSupport.check(target.equals(constrainedRecorder.timeAdvanceGrant)
               && Arrays.asList("removeObjectInstance", "timeAdvanceGrant")
                  .equals(constrainedRecorder.callbackSequence),
            "timestamped removal did not arrive once before its matching grant");
         JavaTckSupport.expectFailure(
            () -> constrained.getObjectInstanceHandle(objectName),
            "ObjectInstanceNotKnown",
            "timestamped removal did not retire its object name after delivery");
      } finally {
         if (timeConstrained && constrainedJoined) {
            try { constrained.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (regulating && ownerJoined) {
            try { owner.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (constrainedAttributes != null && constrainedJoined && constrainedClass != null) {
            try { constrained.unsubscribeObjectClassAttributes(constrainedClass, constrainedAttributes); }
            catch (Exception ignored) { }
         }
         if (ownerAttributes != null && ownerJoined && ownerClass != null) {
            try { owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes); }
            catch (Exception ignored) { }
         }
         if (constrainedJoined) resign(constrained);
         if (ownerJoined) resign(owner);
         if (created) {
            try { owner.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (constrainedConnected) disconnect(constrained);
         if (ownerConnected) disconnect(owner);
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
      throw new AssertionError("timestamped object-removal callback did not arrive before deadline");
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
      private ObjectInstanceHandle discoveredObject;
      private LogicalTime<?, ?> timeRegulationEnabled;
      private LogicalTime<?, ?> timeConstrainedEnabled;
      private LogicalTime<?, ?> timeAdvanceGrant;
      private final List<Removal> removals = new ArrayList<>();
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
               } else if ("removeObjectInstance".equals(name)
                     && arguments != null && arguments.length == 7) {
                  Removal removal = new Removal();
                  removal.object = (ObjectInstanceHandle) arguments[0];
                  removal.tag = ((byte[]) arguments[1]).clone();
                  removal.producer = (FederateHandle) arguments[2];
                  removal.time = (LogicalTime<?, ?>) arguments[3];
                  removal.sentOrder = (OrderType) arguments[4];
                  removal.receivedOrder = (OrderType) arguments[5];
                  removal.retraction = (MessageRetractionHandle) arguments[6];
                  removals.add(removal);
                  callbackSequence.add(name);
               } else if ("timeAdvanceGrant".equals(name)) {
                  timeAdvanceGrant = (LogicalTime<?, ?>) arguments[0];
                  callbackSequence.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK timestamped removal recorder";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == arguments[0];
               }
               return null;
            });
      }
   }
}
