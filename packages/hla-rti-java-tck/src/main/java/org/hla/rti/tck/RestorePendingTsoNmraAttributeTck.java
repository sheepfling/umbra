package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.MessageRetractionReturn;
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
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;
import java.util.function.BooleanSupplier;

/** Replay a timestamped attribute update saved behind pending NMRA. */
final class RestorePendingTsoNmraAttributeTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private RestorePendingTsoNmraAttributeTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED
            || callbackModel == CallbackModel.HLA_IMMEDIATE,
         "pending-TSO NMRA attribute restore requires a standard callback model");
      boolean immediate = callbackModel == CallbackModel.HLA_IMMEDIATE;
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador regulator = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder regulatorCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      List<RTIambassador> members = List.of(publisher, regulator, receiver);
      List<RTIambassador> connected = new ArrayList<>();
      List<RTIambassador> joined = new ArrayList<>();
      String federation = "java-tck-restore-pending-tso-nmra-attribute-"
         + UUID.randomUUID();
      String saveLabel = "tck-pending-nmra-attribute-replay";
      boolean created = false;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         connected.add(publisher);
         regulator.connect(regulatorCallbacks.proxy(), callbackModel);
         connected.add(regulator);
         receiver.connect(receiverCallbacks.proxy(), callbackModel);
         connected.add(receiver);
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle publisherHandle = publisher.joinFederationExecution(
            "java-tck-pending-nmra-attribute-publisher", federation);
         joined.add(publisher);
         regulator.joinFederationExecution("java-tck-pending-nmra-attribute-regulator", federation);
         joined.add(regulator);
         receiver.joinFederationExecution("java-tck-pending-nmra-attribute-receiver", federation);
         joined.add(receiver);

         ObjectClassHandle objectClass = publisher.getObjectClassHandle(objectClassName);
         AttributeHandle attribute = publisher.getAttributeHandle(objectClass, attributeName);
         AttributeHandleSet attributes = publisher.getAttributeHandleSetFactory().create();
         attributes.add(attribute);
         publisher.publishObjectClassAttributes(objectClass, attributes);
         receiver.subscribeObjectClassAttributes(objectClass, attributes);
         publisher.changeDefaultAttributeOrderType(
            objectClass, attributes, OrderType.TIMESTAMP);
         ObjectInstanceHandle object = publisher.registerObjectInstance(objectClass);
         if (!immediate) {
            awaitAll(members, () -> receiverCallbacks.discoveries.size() == 1,
               "pending-NMRA attribute object discovery");
         }
         JavaTckSupport.check(receiverCallbacks.discoveries.size() == 1
               && receiverCallbacks.discoveries.get(0).object.equals(object)
               && receiverCallbacks.discoveries.get(0).objectClass.equals(objectClass),
            "pending-NMRA attribute object was not discovered exactly once");

         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(regulator.getTimeFactory().getName())
               && timeFactory.getName().equals(receiver.getTimeFactory().getName()),
            "pending-TSO NMRA attribute members selected different time factories");
         LogicalTime<?, ?> initial = publisher.queryLogicalTime();
         LogicalTimeInterval<?> epsilon = timeFactory.makeEpsilon();
         LogicalTime<?, ?> time4 = offset(initial, epsilon, 4);
         LogicalTime<?, ?> time5 = offset(initial, epsilon, 5);
         LogicalTime<?, ?> regulatorTarget = time4;
         LogicalTime<?, ?> time9 = offset(initial, epsilon, 9);
         LogicalTime<?, ?> time10 = offset(initial, epsilon, 10);
         byte[] payload = new byte[] {0x51, 0x22, 0x73};
         byte[] tag = new byte[] {'N', 'M', 'R', 'A', '-', 'U', 'P', 'D'};
         AttributeHandleValueMap values =
            publisher.getAttributeHandleValueMapFactory().create(1);
         values.put(attribute, payload);

         for (RTIambassador member : members) member.enableTimeConstrained();
         if (!immediate) {
            awaitAll(members, () -> publisherCallbacks.constrainedEnabled == 1
                  && regulatorCallbacks.constrainedEnabled == 1
                  && receiverCallbacks.constrainedEnabled == 1,
               "three pending-TSO NMRA attribute constrained enablements");
         }
         JavaTckSupport.check(publisherCallbacks.constrainedEnabled == 1
               && regulatorCallbacks.constrainedEnabled == 1
               && receiverCallbacks.constrainedEnabled == 1,
            "pending-TSO NMRA attribute constrained enablement was incomplete");
         publisher.enableTimeRegulation(epsilon);
         regulator.enableTimeRegulation(epsilon);
         if (!immediate) {
            awaitAll(members, () -> publisherCallbacks.regulationEnabled == 1
                  && regulatorCallbacks.regulationEnabled == 1,
               "two pending-TSO NMRA attribute regulation enablements");
         }
         JavaTckSupport.check(publisherCallbacks.regulationEnabled == 1
               && regulatorCallbacks.regulationEnabled == 1,
            "pending-TSO NMRA attribute regulation enablement was incomplete");

         receiver.nextMessageRequestAvailable(time10);
         JavaTckSupport.check(receiverCallbacks.reflections.isEmpty(),
            "receiver saw TSO before the queued send");
         MessageRetractionReturn sent = publisher.updateAttributeValues(
            object, values, tag, time5);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid
               && sent.handle != null && receiverCallbacks.reflections.isEmpty()
               && receiverCallbacks.grants.isEmpty(),
            "timestamped attribute reflection was not queued behind pending NMRA");
         receiver.requestFederationSave(saveLabel);
         JavaTckSupport.check(publisherCallbacks.saveLabels.isEmpty()
               && receiverCallbacks.saveLabels.isEmpty(),
            "save initiated before the three-member advance boundary");

         CountDownLatch completedSaves = new CountDownLatch(3);
         SaveHook publisherSave = () -> completeSave(
            publisher, completedSaves, immediate);
         SaveHook regulatorSave = () -> completeSave(
            regulator, completedSaves, immediate);
         SaveHook receiverSave = () -> completeSave(
            receiver, completedSaves, immediate);
         publisherCallbacks.saveHook = publisherSave;
         regulatorCallbacks.saveHook = regulatorSave;
         receiverCallbacks.saveHook = receiverSave;
         publisherCallbacks.order.clear();
         regulatorCallbacks.order.clear();
         receiverCallbacks.order.clear();
         if (immediate) for (RTIambassador member : members) member.disableCallbacks();
         publisher.timeAdvanceRequest(regulatorTarget);
         regulator.timeAdvanceRequest(regulatorTarget);
         if (immediate) {
            for (RTIambassador member : members) member.enableCallbacks();
         } else {
            drainConcurrently(members);
         }
         JavaTckSupport.check(completedSaves.getCount() == 0
               && publisherCallbacks.saveLabels.equals(List.of(saveLabel))
               && regulatorCallbacks.saveLabels.equals(List.of(saveLabel))
               && receiverCallbacks.saveLabels.equals(List.of(saveLabel))
               && publisherCallbacks.savedCount == 1
               && regulatorCallbacks.savedCount == 1
               && receiverCallbacks.savedCount == 1
               && publisherCallbacks.grants.equals(List.of(regulatorTarget))
               && regulatorCallbacks.grants.equals(List.of(regulatorTarget))
               && receiverCallbacks.grants.equals(List.of(time5))
               && receiverCallbacks.reflections.size() == 1,
            "pending-TSO NMRA attribute save boundary or first delivery was incomplete");
         TimedReflection original = receiverCallbacks.reflections.get(0);
         JavaTckSupport.check(original.object.equals(object)
               && original.producer.equals(publisherHandle)
               && original.time.equals(time5)
               && original.sentOrder == OrderType.TIMESTAMP
               && original.receivedOrder == OrderType.TIMESTAMP
               && original.retraction != null
               && Arrays.equals(original.tag, tag)
               && original.values.size() == 1
               && Arrays.equals(original.values.get(attribute), payload),
            "original timestamped attribute reflection changed public callback fields");
         List<String> originalOrder = receiverCallbacks.order;
         JavaTckSupport.check(originalOrder.indexOf("save-initiate") >= 0
               && originalOrder.indexOf("reflect")
                  > originalOrder.indexOf("save-initiate")
               && originalOrder.indexOf("grant") > originalOrder.indexOf("reflect"),
            "original timestamped attribute reflection did not precede NMRA grant");

         publisherCallbacks.saveHook = null;
         regulatorCallbacks.saveHook = null;
         receiverCallbacks.saveHook = null;
         for (RTIambassador member : members) member.timeAdvanceRequest(time9);
         if (!immediate) {
            awaitAll(members, () -> publisherCallbacks.grants.size() == 2
                  && regulatorCallbacks.grants.size() == 2
                  && receiverCallbacks.grants.size() == 2,
               "post-save three-member time-9 grants");
         }
         JavaTckSupport.check(publisherCallbacks.grants.size() == 2
               && regulatorCallbacks.grants.size() == 2
               && receiverCallbacks.grants.equals(List.of(time5, time9))
               && time9.equals(publisher.queryLogicalTime())
               && time9.equals(regulator.queryLogicalTime())
               && time9.equals(receiver.queryLogicalTime()),
            "post-save advances did not reach time 9 for all members");

         publisherCallbacks.order.clear();
         regulatorCallbacks.order.clear();
         receiverCallbacks.order.clear();
         receiver.requestFederationRestore(saveLabel);
         if (!immediate) {
            awaitAll(members, () -> publisherCallbacks.restoreLabels.size() == 1
                  && regulatorCallbacks.restoreLabels.size() == 1
                  && receiverCallbacks.restoreLabels.size() == 1,
               "three pending-TSO NMRA attribute restore initiations");
         }
         publisher.federateRestoreComplete();
         regulator.federateRestoreComplete();
         receiver.federateRestoreComplete();
         if (!immediate) {
            awaitAll(members, () -> publisherCallbacks.restoredCount == 1
                  && regulatorCallbacks.restoredCount == 1
                  && receiverCallbacks.restoredCount == 1
                  && receiverCallbacks.reflections.size() == 2
                  && receiverCallbacks.grants.size() == 3,
               "restored pending-TSO NMRA attribute delivery and grant");
         }
         JavaTckSupport.check(publisherCallbacks.restoredCount == 1
               && regulatorCallbacks.restoredCount == 1
               && receiverCallbacks.restoredCount == 1
               && receiverCallbacks.reflections.size() == 2
               && receiverCallbacks.grants.size() == 3
               && time5.equals(receiverCallbacks.grants.get(2)),
            "restore did not replay one attribute reflection before restored NMRA grant");
         TimedReflection replayed = receiverCallbacks.reflections.get(1);
         JavaTckSupport.check(replayed.object.equals(original.object)
               && replayed.producer.equals(original.producer)
               && replayed.time.equals(original.time)
               && replayed.sentOrder == original.sentOrder
               && replayed.receivedOrder == original.receivedOrder
               && (replayed.retraction != null) == (original.retraction != null)
               && Arrays.equals(replayed.tag, tag)
               && replayed.values.size() == 1
               && Arrays.equals(replayed.values.get(attribute), payload),
            "replayed timestamped attribute reflection changed public callback fields");
         List<String> restoreOrder = receiverCallbacks.order;
         JavaTckSupport.check(restoreOrder.indexOf("restore-complete") >= 0
               && restoreOrder.indexOf("reflect")
                  > restoreOrder.indexOf("restore-complete")
               && restoreOrder.indexOf("grant")
                  > restoreOrder.indexOf("reflect")
               && regulatorTarget.equals(publisher.queryLogicalTime())
               && regulatorTarget.equals(regulator.queryLogicalTime())
               && time5.equals(receiver.queryLogicalTime()),
            "restored attribute reflection was not delivered between restoration and grant");

         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         regulator.resignFederationExecution(ResignAction.NO_ACTION);
         publisher.resignFederationExecution(
            ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         joined.clear();
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         for (int index = joined.size() - 1; index >= 0; --index) {
            try { joined.get(index).resignFederationExecution(
               joined.get(index) == publisher
                  ? ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST
                  : ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         for (int index = connected.size() - 1; index >= 0; --index) {
            try { connected.get(index).disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void completeSave(RTIambassador member,
         CountDownLatch completedSaves, boolean immediate) throws Exception {
      member.federateSaveBegun();
      member.federateSaveComplete();
      completedSaves.countDown();
      if (!immediate) {
         JavaTckSupport.check(completedSaves.await(5, TimeUnit.SECONDS),
            "three evoked save callbacks did not reach the shared boundary");
      }
   }

   private static void drainConcurrently(List<RTIambassador> members) throws Exception {
      List<Thread> threads = new ArrayList<>();
      AtomicReference<Throwable> failure = new AtomicReference<>();
      for (RTIambassador member : members) {
         Thread thread = new Thread(() -> {
            try { while (member.evokeCallback(0.0)) { } }
            catch (Throwable error) { failure.compareAndSet(null, error); }
         }, "java-tck-pending-nmr-save-callback");
         thread.setDaemon(true);
         threads.add(thread);
         thread.start();
      }
      for (Thread thread : threads) thread.join(10_000);
      for (Thread thread : threads) {
         JavaTckSupport.check(!thread.isAlive(),
            "evoked pending-NMRA attribute save callback thread did not finish");
      }
      Throwable error = failure.get();
      if (error != null) throw new IllegalStateException("evoked save callback failed", error);
   }

   private static void awaitAll(List<RTIambassador> members,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         for (RTIambassador member : members) member.evokeCallback(0.0);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime offset(LogicalTime start,
         LogicalTimeInterval interval, int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) result = (LogicalTime) result.add(interval);
      return result;
   }

   @FunctionalInterface
   private interface SaveHook { void run() throws Exception; }

   private static final class TimedReflection {
      private final ObjectInstanceHandle object;
      private final AttributeHandleValueMap values;
      private final byte[] tag;
      private final FederateHandle producer;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;
      private final MessageRetractionHandle retraction;

      private TimedReflection(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         values = ((AttributeHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
         producer = (FederateHandle) arguments[4];
         time = (LogicalTime<?, ?>) arguments[6];
         sentOrder = (OrderType) arguments[7];
         receivedOrder = (OrderType) arguments[8];
         retraction = (MessageRetractionHandle) arguments[9];
      }
   }

   private static final class Discovery {
      private final ObjectInstanceHandle object;
      private final ObjectClassHandle objectClass;

      private Discovery(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         objectClass = (ObjectClassHandle) arguments[1];
      }
   }

   private static final class Recorder {
      private final List<String> saveLabels = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<TimedReflection> reflections = new CopyOnWriteArrayList<>();
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();
      private final List<String> restoreLabels = new CopyOnWriteArrayList<>();
      private final List<String> order = new CopyOnWriteArrayList<>();
      private volatile SaveHook saveHook;
      private volatile int constrainedEnabled;
      private volatile int regulationEnabled;
      private volatile int savedCount;
      private volatile int restoredCount;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeConstrainedEnabled".equals(name)) ++constrainedEnabled;
               else if ("timeRegulationEnabled".equals(name)) ++regulationEnabled;
               else if ("initiateFederateSave".equals(name)) {
                  saveLabels.add((String) arguments[0]);
                  order.add("save-initiate");
                  SaveHook hook = saveHook;
                  if (hook != null) hook.run();
               } else if ("federationSaved".equals(name)) ++savedCount;
               else if ("timeAdvanceGrant".equals(name)) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
                  order.add("grant");
               } else if ("discoverObjectInstance".equals(name)
                     && arguments != null && arguments.length == 4) {
                  discoveries.add(new Discovery(arguments));
                  order.add("discover");
               } else if ("reflectAttributeValues".equals(name)
                     && arguments != null && arguments.length == 10) {
                  reflections.add(new TimedReflection(arguments));
                  order.add("reflect");
               } else if ("initiateFederateRestore".equals(name)) {
                  restoreLabels.add((String) arguments[0]);
                  order.add("restore-initiate");
               } else if ("requestFederationRestoreSucceeded".equals(name)) {
                  order.add("restore-request-succeeded");
               } else if ("federationRestoreBegun".equals(name)) {
                  order.add("restore-begun");
               } else if ("federationRestored".equals(name)) {
                  ++restoredCount;
                  order.add("restore-complete");
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK pending-TSO NMRA attribute callbacks";
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
