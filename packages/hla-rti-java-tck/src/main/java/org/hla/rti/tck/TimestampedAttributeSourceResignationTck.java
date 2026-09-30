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
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
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

/** Preserves queued timestamped attribute delivery after its source resigns. */
final class TimestampedAttributeSourceResignationTck {
   private TimestampedAttributeSourceResignationTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador survivor = factory.getRtiAmbassador();
      RTIambassador clock = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder survivorRecorder = new Recorder();
      Recorder clockRecorder = new Recorder();
      String federation = "java-tck-timestamped-attribute-source-resignation-"
         + UUID.randomUUID();
      boolean ownerConnected = false, survivorConnected = false, clockConnected = false;
      boolean created = false, ownerJoined = false, survivorJoined = false, clockJoined = false;
      boolean survivorSubscribed = false, survivorPublished = false;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         survivor.connect(survivorRecorder.proxy(), callbackModel);
         survivorConnected = true;
         clock.connect(clockRecorder.proxy(), callbackModel);
         clockConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = owner.joinFederationExecution(
            "java-tck-attribute-resignation-owner", federation);
         ownerJoined = true;
         survivor.joinFederationExecution("java-tck-attribute-resignation-survivor", federation);
         survivorJoined = true;
         clock.joinFederationExecution("java-tck-attribute-resignation-clock", federation);
         clockJoined = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle survivorClass = survivor.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle survivorAttribute = survivor.getAttributeHandle(survivorClass, attributeName);
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet survivorAttributes = JavaTckSupport.attributeSet(
            survivor, survivorAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         survivor.publishObjectClassAttributes(survivorClass, survivorAttributes);
         survivorPublished = true;
         survivor.subscribeObjectClassAttributes(survivorClass, survivorAttributes);
         survivorSubscribed = true;
         owner.changeDefaultAttributeOrderType(ownerClass, ownerAttributes, OrderType.TIMESTAMP);

         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         await(() -> survivorRecorder.discovered.contains(object), survivor,
            "source-resignation object discovery");

         LogicalTimeFactory<?, ?> timeFactory = owner.getTimeFactory();
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         LogicalTime ownerInitial = owner.queryLogicalTime();
         LogicalTime survivorInitial = survivor.queryLogicalTime();
         LogicalTime clockInitial = clock.queryLogicalTime();
         check(ownerInitial.equals(survivorInitial) && ownerInitial.equals(clockInitial),
            "source-resignation members began at different logical times");
         LogicalTimeInterval ownerLookahead = intervalOffset(epsilon, 5);
         LogicalTimeInterval clockLookahead = intervalOffset(epsilon, 5);

         survivor.enableTimeConstrained();
         await(() -> survivorRecorder.timeConstrainedEnabled, survivor,
            "survivor timeConstrainedEnabled callback");
         owner.enableTimeRegulation(ownerLookahead);
         await(() -> ownerRecorder.timeRegulationEnabled, owner,
            "source timeRegulationEnabled callback");
         clock.enableTimeRegulation(clockLookahead);
         await(() -> clockRecorder.timeRegulationEnabled, clock,
            "clock timeRegulationEnabled callback");

         LogicalTime messageTime = offset(ownerInitial, epsilon, 5);
         LogicalTime survivorTarget = offset(survivorInitial, epsilon, 5);
         LogicalTime clockTarget = offset(clockInitial, epsilon, 2);
         byte[] value = new byte[] {(byte) 0xCA, (byte) 0xFE};
         byte[] updateTag = new byte[] {0x52, 0x45, 0x53, 0x49, 0x47, 0x4E};
         byte[] acquisitionTag = new byte[] {0x41, 0x43, 0x51, 0x2D, 0x52};
         AttributeHandleValueMap values = owner.getAttributeHandleValueMapFactory().create(1);
         values.put(ownerAttribute, value);
         MessageRetractionReturn update = owner.updateAttributeValues(
            object, values, updateTag, messageTime);
         check(update != null && update.retractionHandleIsValid && update.handle != null,
            "timestamped source update returned no valid retraction designator");
         check(survivorRecorder.reflections.isEmpty(),
            "timestamped update reflected before the survivor advanced");

         owner.resignFederationExecution(ResignAction.UNCONDITIONALLY_DIVEST_ATTRIBUTES);
         ownerJoined = false;
         await(() -> !survivorRecorder.assumptions.isEmpty(), survivor,
            "ownership assumption after source resignation");
         OwnershipEvent assumption = survivorRecorder.assumptions.get(0);
         check(object.equals(assumption.object) && assumption.attributes.size() == 1
               && assumption.attributes.contains(survivorAttribute),
            "source-resignation assumption callback returned incorrect metadata");

         survivor.attributeOwnershipAcquisitionIfAvailable(
            object, survivorAttributes, acquisitionTag);
         await(() -> !survivorRecorder.acquisitions.isEmpty(), survivor,
            "attribute acquisition after source resignation");
         OwnershipEvent acquisition = survivorRecorder.acquisitions.get(0);
         check(object.equals(acquisition.object) && acquisition.attributes.size() == 1
               && acquisition.attributes.contains(survivorAttribute)
               && Arrays.equals(acquisitionTag, acquisition.tag),
            "source-resignation acquisition callback returned incorrect metadata");
         check(survivor.isAttributeOwnedByFederate(object, survivorAttribute),
            "survivor did not acquire the source's attribute");
         expectException(() -> owner.retract(update.handle), "FederateNotExecutionMember",
            "retracting the queued update after its producer resigned");

         survivorRecorder.callbackOrder.clear();
         survivor.timeAdvanceRequest(survivorTarget);
         clock.timeAdvanceRequest(clockTarget);
         await(() -> !survivorRecorder.reflections.isEmpty()
               && survivorRecorder.grants.size() == 1, survivor,
            "queued update after source resignation");
         await(() -> clockRecorder.grants.size() == 1, clock,
            "regulating clock advance after source resignation");

         check(survivorRecorder.reflections.size() == 1,
            "source resignation caused duplicate timestamped reflections");
         Reflection reflection = survivorRecorder.reflections.get(0);
         check(object.equals(reflection.object) && reflection.values.size() == 1
               && Arrays.equals(value, reflection.values.get(survivorAttribute)),
            "source-resignation reflection returned the wrong object or attribute value");
         check(Arrays.equals(updateTag, reflection.tag) && producer.equals(reflection.producer)
               && messageTime.equals(reflection.time),
            "source-resignation reflection changed tag, producer, or timestamp");
         check(reflection.transportation != null
               && !survivor.getTransportationTypeName(reflection.transportation).isEmpty(),
            "source-resignation reflection returned an unknown transportation type");
         check(reflection.sentOrder == OrderType.TIMESTAMP
               && reflection.receivedOrder == OrderType.TIMESTAMP
               && update.handle.equals(reflection.retraction),
            "source-resignation reflection changed order or retraction metadata");
         check(survivorTarget.equals(survivorRecorder.grants.get(0))
               && clockTarget.equals(clockRecorder.grants.get(0)),
            "source-resignation advances returned incorrect grant times");
         check(survivorRecorder.callbackOrder.size() == 2
               && "reflectAttributeValues".equals(survivorRecorder.callbackOrder.get(0))
               && "timeAdvanceGrant".equals(survivorRecorder.callbackOrder.get(1)),
            "source-resignation reflection was delivered after its matching grant");

         survivor.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         survivorJoined = false;
         clock.resignFederationExecution(ResignAction.NO_ACTION);
         clockJoined = false;
         survivor.disconnect();
         survivorConnected = false;
         clock.disconnect();
         clockConnected = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (survivorSubscribed && survivorJoined) try {
            survivor.unsubscribeObjectClassAttributes(
               survivor.getObjectClassHandle(objectClassName),
               JavaTckSupport.attributeSet(survivor,
                  survivor.getAttributeHandle(survivor.getObjectClassHandle(objectClassName),
                     attributeName)));
         } catch (Exception ignored) { }
         if (survivorPublished && survivorJoined) try {
            survivor.unpublishObjectClassAttributes(
               survivor.getObjectClassHandle(objectClassName),
               JavaTckSupport.attributeSet(survivor,
                  survivor.getAttributeHandle(survivor.getObjectClassHandle(objectClassName),
                     attributeName)));
         } catch (Exception ignored) { }
         if (survivorJoined) try {
            survivor.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         } catch (Exception ignored) { }
         if (clockJoined) try { clock.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (ownerJoined) try {
            owner.resignFederationExecution(ResignAction.UNCONDITIONALLY_DIVEST_ATTRIBUTES);
         } catch (Exception ignored) { }
         if (created) try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (survivorConnected) try { survivor.disconnect(); } catch (Exception ignored) { }
         if (clockConnected) try { clock.disconnect(); } catch (Exception ignored) { }
         if (ownerConnected) try { owner.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static void expectException(JavaTckSupport.CheckedOperation operation,
         String expected, String description) throws Exception {
      try {
         operation.run();
      } catch (Exception exception) {
         for (Class<?> type = exception.getClass(); type != null; type = type.getSuperclass()) {
            if (expected.equals(type.getSimpleName())) return;
         }
         throw new AssertionError(description + " threw " + exception.getClass().getName()
            + " instead of " + expected, exception);
      }
      throw new AssertionError(description + " did not throw " + expected);
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTimeInterval intervalOffset(LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTimeInterval result = epsilon;
      for (int i = 1; i < steps; ++i) {
         result = (LogicalTimeInterval) ((LogicalTimeInterval) result).add(epsilon);
      }
      return result;
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime<?, ?> offset(LogicalTime<?, ?> start,
         LogicalTimeInterval epsilon, int steps) throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(epsilon);
      return result;
   }

   private static void await(ReadyCheck ready, RTIambassador ambassador, String description)
         throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline) {
         JavaTckSupport.drain(ambassador);
         if (ready.isReady()) return;
         Thread.sleep(1L);
      }
      throw new AssertionError(description + " timed out");
   }

   private static void check(boolean condition, String message) {
      if (!condition) throw new AssertionError(message);
   }

   @FunctionalInterface
   private interface ReadyCheck { boolean isReady(); }

   private static final class OwnershipEvent {
      private final ObjectInstanceHandle object;
      private final AttributeHandleSet attributes;
      private final byte[] tag;
      private OwnershipEvent(ObjectInstanceHandle object, AttributeHandleSet attributes,
            byte[] tag) {
         this.object = object;
         this.attributes = attributes;
         this.tag = tag;
      }
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final AttributeHandleValueMap values;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;
      private final MessageRetractionHandle retraction;
      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap values, byte[] tag,
            TransportationTypeHandle transportation, FederateHandle producer,
            LogicalTime<?, ?> time, OrderType sentOrder, OrderType receivedOrder,
            MessageRetractionHandle retraction) {
         this.object = object;
         this.values = values;
         this.tag = tag;
         this.transportation = transportation;
         this.producer = producer;
         this.time = time;
         this.sentOrder = sentOrder;
         this.receivedOrder = receivedOrder;
         this.retraction = retraction;
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discovered = new ArrayList<>();
      private final List<OwnershipEvent> assumptions = new ArrayList<>();
      private final List<OwnershipEvent> acquisitions = new ArrayList<>();
      private final List<Reflection> reflections = new ArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new ArrayList<>();
      private final List<String> callbackOrder = new ArrayList<>();
      private boolean timeRegulationEnabled;
      private boolean timeConstrainedEnabled;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discovered.add((ObjectInstanceHandle) arguments[0]);
               } else if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
               } else if ("requestAttributeOwnershipAssumption".equals(name)
                     && arguments != null && arguments.length == 3) {
                  assumptions.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], ((byte[]) arguments[2]).clone()));
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)
                     && arguments != null && arguments.length == 3) {
                  acquisitions.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], ((byte[]) arguments[2]).clone()));
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 10) {
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     ((AttributeHandleValueMap) arguments[1]).clone(),
                     ((byte[]) arguments[2]).clone(), (TransportationTypeHandle) arguments[3],
                     (FederateHandle) arguments[4], (LogicalTime<?, ?>) arguments[6],
                     (OrderType) arguments[7], (OrderType) arguments[8],
                     (MessageRetractionHandle) arguments[9]));
                  callbackOrder.add(name);
               } else if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK resignation callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
