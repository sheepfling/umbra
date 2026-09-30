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
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;

/** Fans a queued timestamped object deletion out after its producer resigns. */
final class TimestampedObjectDeletionSourceResignationFanoutTck {
   private TimestampedObjectDeletionSourceResignationFanoutTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador first = factory.getRtiAmbassador();
      RTIambassador second = factory.getRtiAmbassador();
      RTIambassador clock = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder firstRecorder = new Recorder();
      Recorder secondRecorder = new Recorder();
      Recorder clockRecorder = new Recorder();
      String federation = "java-tck-deletion-source-resignation-" + UUID.randomUUID();
      boolean publisherConnected = false, firstConnected = false;
      boolean secondConnected = false, clockConnected = false, created = false;
      boolean publisherJoined = false, firstJoined = false, secondJoined = false;
      boolean clockJoined = false, published = false, firstSubscribed = false;
      boolean secondSubscribed = false, firstConstrained = false;
      boolean secondConstrained = false, publisherRegulating = false, clockRegulating = false;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel); publisherConnected = true;
         first.connect(firstRecorder.proxy(), callbackModel); firstConnected = true;
         second.connect(secondRecorder.proxy(), callbackModel); secondConnected = true;
         clock.connect(clockRecorder.proxy(), callbackModel); clockConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-deletion-source-publisher", federation);
         publisherJoined = true;
         first.joinFederationExecution("java-tck-deletion-source-first", federation);
         firstJoined = true;
         second.joinFederationExecution("java-tck-deletion-source-second", federation);
         secondJoined = true;
         clock.joinFederationExecution("java-tck-deletion-source-clock", federation);
         clockJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle firstClass = first.getObjectClassHandle(objectClassName);
         ObjectClassHandle secondClass = second.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         AttributeHandle firstAttribute = first.getAttributeHandle(firstClass, attributeName);
         AttributeHandle secondAttribute = second.getAttributeHandle(secondClass, attributeName);
         AttributeHandleSet publisherAttributes =
            JavaTckSupport.attributeSet(publisher, publisherAttribute);
         AttributeHandleSet firstAttributes = JavaTckSupport.attributeSet(first, firstAttribute);
         AttributeHandleSet secondAttributes =
            JavaTckSupport.attributeSet(second, secondAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         first.subscribeObjectClassAttributes(firstClass, firstAttributes);
         firstSubscribed = true;
         second.subscribeObjectClassAttributes(secondClass, secondAttributes);
         secondSubscribed = true;

         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         await(() -> firstRecorder.discoveries.contains(object)
               && secondRecorder.discoveries.contains(object),
            new RTIambassador[] {first, second}, "both deletion recipients' discovery");

         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         String timeFactoryName = timeFactory.getName();
         JavaTckSupport.check(timeFactoryName.equals(first.getTimeFactory().getName())
               && timeFactoryName.equals(second.getTimeFactory().getName())
               && timeFactoryName.equals(clock.getTimeFactory().getName()),
            "federates selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         LogicalTimeInterval lookahead = intervalOffset(epsilon, 5);
         first.enableTimeConstrained(); firstConstrained = true;
         second.enableTimeConstrained(); secondConstrained = true;
         await(() -> firstRecorder.timeConstrainedEnabledCount == 1
               && secondRecorder.timeConstrainedEnabledCount == 1,
            new RTIambassador[] {first, second}, "both Time Constrained callbacks");
         publisher.enableTimeRegulation(lookahead); publisherRegulating = true;
         await(() -> publisherRecorder.timeRegulationEnabledCount == 1,
            new RTIambassador[] {publisher}, "publisher Time Regulation callback");
         clock.enableTimeRegulation(lookahead); clockRegulating = true;
         await(() -> clockRecorder.timeRegulationEnabledCount == 1,
            new RTIambassador[] {clock}, "clock Time Regulation callback");

         LogicalTime initial = publisher.queryLogicalTime();
         JavaTckSupport.check(initial.equals(first.queryLogicalTime())
               && initial.equals(second.queryLogicalTime())
               && initial.equals(clock.queryLogicalTime()),
            "federates began at different logical times");
         LogicalTime messageTime = offset(initial, epsilon, 7);
         LogicalTime firstTarget = offset(initial, epsilon, 7);
         LogicalTime secondTarget = offset(initial, epsilon, 10);
         LogicalTime clockTarget = offset(initial, epsilon, 2);
         byte[] tag = new byte[] {0x52, 0x45, 0x53, 0x49, 0x47, 0x2D, 0x44};
         MessageRetractionReturn deletion = publisher.deleteObjectInstance(object, tag, messageTime);
         JavaTckSupport.check(deletion != null && deletion.retractionHandleIsValid
               && deletion.handle != null,
            "timestamped object deletion returned no valid retraction handle");
         JavaTckSupport.drain(first);
         JavaTckSupport.drain(second);
         JavaTckSupport.check(firstRecorder.removals.isEmpty()
               && secondRecorder.removals.isEmpty(),
            "deletion was delivered before either recipient frontier");

         first.timeAdvanceRequest(firstTarget);
         second.nextMessageRequest(secondTarget);
         JavaTckSupport.drain(first);
         JavaTckSupport.drain(second);
         JavaTckSupport.check(firstRecorder.grants.isEmpty()
               && secondRecorder.grants.isEmpty(),
            "a recipient was granted before the producer frontier");

         publisher.resignFederationExecution(ResignAction.UNCONDITIONALLY_DIVEST_ATTRIBUTES);
         publisherJoined = false;
         JavaTckSupport.drain(first);
         JavaTckSupport.drain(second);
         JavaTckSupport.check(firstRecorder.removals.isEmpty()
               && secondRecorder.removals.isEmpty()
               && firstRecorder.grants.isEmpty() && secondRecorder.grants.isEmpty(),
            "producer resignation released a queued recipient callback prematurely");
         expectNotExecutionMember(() -> publisher.retract(deletion.handle));

         clock.timeAdvanceRequest(clockTarget);
         await(() -> clockRecorder.grants.size() == 1,
            new RTIambassador[] {clock, first, second}, "clock grant at the first frontier");
         JavaTckSupport.check(clockTarget.equals(clockRecorder.grants.get(0)),
            "clock grant returned the wrong logical time");
         await(() -> firstRecorder.removals.size() == 1 && firstRecorder.grants.size() == 1,
            new RTIambassador[] {first, second, clock},
            "first recipient deletion delivery and grant");
         await(() -> secondRecorder.removals.size() == 1 && secondRecorder.grants.size() == 1,
            new RTIambassador[] {second, first, clock},
            "second recipient deletion delivery and grant");

         checkRemoval(firstRecorder, object, tag, producer, messageTime,
            deletion.handle, firstTarget);
         checkRemoval(secondRecorder, object, tag, producer, messageTime,
            deletion.handle, messageTime);
         JavaTckSupport.check(firstTarget.equals(first.queryLogicalTime())
               && messageTime.equals(second.queryLogicalTime())
               && clockTarget.equals(clock.queryLogicalTime()),
            "recipient or clock logical-time query disagreed with its grant");

         first.disableTimeConstrained(); firstConstrained = false;
         second.disableTimeConstrained(); secondConstrained = false;
         clock.disableTimeRegulation(); clockRegulating = false;
         first.unsubscribeObjectClassAttributes(firstClass, firstAttributes);
         firstSubscribed = false;
         second.unsubscribeObjectClassAttributes(secondClass, secondAttributes);
         secondSubscribed = false;
         publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         published = false;
         first.resignFederationExecution(ResignAction.NO_ACTION); firstJoined = false;
         second.resignFederationExecution(ResignAction.NO_ACTION); secondJoined = false;
         clock.resignFederationExecution(ResignAction.NO_ACTION); clockJoined = false;
         first.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (firstConstrained && firstJoined) try { first.disableTimeConstrained(); }
            catch (Exception ignored) { }
         if (secondConstrained && secondJoined) try { second.disableTimeConstrained(); }
            catch (Exception ignored) { }
         if (publisherRegulating && publisherJoined) try { publisher.disableTimeRegulation(); }
            catch (Exception ignored) { }
         if (clockRegulating && clockJoined) try { clock.disableTimeRegulation(); }
            catch (Exception ignored) { }
         if (firstSubscribed && firstJoined) try {
            ObjectClassHandle cls = first.getObjectClassHandle(objectClassName);
            first.unsubscribeObjectClassAttributes(cls, JavaTckSupport.attributeSet(
               first, first.getAttributeHandle(cls, attributeName)));
         } catch (Exception ignored) { }
         if (secondSubscribed && secondJoined) try {
            ObjectClassHandle cls = second.getObjectClassHandle(objectClassName);
            second.unsubscribeObjectClassAttributes(cls, JavaTckSupport.attributeSet(
               second, second.getAttributeHandle(cls, attributeName)));
         } catch (Exception ignored) { }
         if (published && publisherJoined) try {
            ObjectClassHandle cls = publisher.getObjectClassHandle(objectClassName);
            publisher.unpublishObjectClassAttributes(cls, JavaTckSupport.attributeSet(
               publisher, publisher.getAttributeHandle(cls, attributeName)));
         } catch (Exception ignored) { }
         if (publisherJoined) try {
            publisher.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         } catch (Exception ignored) { }
         if (firstJoined) try { first.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (secondJoined) try { second.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (clockJoined) try { clock.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (clockConnected) try { clock.disconnect(); } catch (Exception ignored) { }
         if (secondConnected) try { second.disconnect(); } catch (Exception ignored) { }
         if (firstConnected) try { first.disconnect(); } catch (Exception ignored) { }
         if (publisherConnected) try { publisher.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(epsilon);
      return result;
   }

   private static LogicalTimeInterval intervalOffset(LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTimeInterval result = epsilon;
      for (int i = 1; i < steps; ++i) result = (LogicalTimeInterval) result.add(epsilon);
      return result;
   }

   private static void checkRemoval(Recorder recorder, ObjectInstanceHandle object, byte[] tag,
         FederateHandle producer, LogicalTime<?, ?> messageTime,
         MessageRetractionHandle retraction, LogicalTime<?, ?> expectedGrant) {
      JavaTckSupport.check(recorder.removals.size() == 1 && recorder.grants.size() == 1,
         "recipient received duplicate timestamped removals or grants");
      Removal removal = recorder.removals.get(0);
      JavaTckSupport.check(object.equals(removal.object) && Arrays.equals(tag, removal.tag)
            && producer.equals(removal.producer),
         "timestamped removal returned the wrong object, tag, or producer");
      JavaTckSupport.check(messageTime.equals(removal.time)
            && removal.sentOrder == OrderType.TIMESTAMP
            && removal.receivedOrder == OrderType.TIMESTAMP
            && retraction.equals(removal.retraction),
         "timestamped removal lost its time, order, or retraction metadata");
      JavaTckSupport.check(expectedGrant.equals(recorder.grants.get(0))
            && recorder.callbackOrder.equals(Arrays.asList("removeObjectInstance", "timeAdvanceGrant")),
         "timestamped removal did not precede the correct receiver grant");
   }

   private static void await(Ready ready, RTIambassador[] ambassadors, String description)
         throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline) {
         for (RTIambassador ambassador : ambassadors) JavaTckSupport.drain(ambassador);
         if (ready.test()) return;
         Thread.sleep(1L);
      }
      throw new AssertionError(description + " timed out");
   }

   @FunctionalInterface
   private interface Ready { boolean test(); }

   private static void expectNotExecutionMember(JavaTckSupport.CheckedOperation operation)
         throws Exception {
      try {
         operation.run();
      } catch (Exception exception) {
         for (Class<?> type = exception.getClass(); type != null; type = type.getSuperclass()) {
            if ("FederateNotExecutionMember".equals(type.getSimpleName())) return;
         }
         throw new AssertionError("retraction after producer resignation threw "
            + exception.getClass().getName() + " instead of FederateNotExecutionMember", exception);
      }
      throw new AssertionError("retraction after producer resignation did not throw "
         + "FederateNotExecutionMember");
   }

   private static final class Removal {
      private ObjectInstanceHandle object;
      private byte[] tag;
      private FederateHandle producer;
      private LogicalTime<?, ?> time;
      private OrderType sentOrder, receivedOrder;
      private MessageRetractionHandle retraction;
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new ArrayList<>();
      private final List<Removal> removals = new ArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new ArrayList<>();
      private final List<String> callbackOrder = new ArrayList<>();
      private int timeRegulationEnabledCount, timeConstrainedEnabledCount;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("timeRegulationEnabled".equals(name)) {
                  ++timeRegulationEnabledCount;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  ++timeConstrainedEnabledCount;
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
                  removals.add(removal);
                  callbackOrder.add(name);
               } else if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK deletion source-resignation callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
