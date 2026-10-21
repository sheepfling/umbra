package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
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

/** Replay one timestamped interaction saved behind pending NMR or NMRA. */
final class RestorePendingTsoNmrTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private RestorePendingTsoNmrTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName,
         CallbackModel callbackModel) throws Exception {
      run(factory, fom, timeImplementation, interactionClassName, parameterName,
         callbackModel, false);
   }

   static void runAvailable(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName,
         CallbackModel callbackModel) throws Exception {
      run(factory, fom, timeImplementation, interactionClassName, parameterName,
         callbackModel, true);
   }

   private static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName,
         CallbackModel callbackModel, boolean available) throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED
            || callbackModel == CallbackModel.HLA_IMMEDIATE,
         "pending-TSO NMR restore requires a standard callback model");
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
      String federation = "java-tck-restore-pending-tso-"
         + (available ? "nmra-" : "nmr-") + UUID.randomUUID();
      String saveLabel = available ? "tck-pending-nmra-tso-replay"
         : "tck-pending-nmr-tso-replay";
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
            "java-tck-pending-nmr-publisher", federation);
         joined.add(publisher);
         regulator.joinFederationExecution("java-tck-pending-nmr-regulator", federation);
         joined.add(regulator);
         receiver.joinFederationExecution("java-tck-pending-nmr-receiver", federation);
         joined.add(receiver);

         InteractionClassHandle pubInteraction =
            publisher.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle recvInteraction =
            receiver.getInteractionClassHandle(interactionClassName);
         ParameterHandle pubParameter =
            publisher.getParameterHandle(pubInteraction, parameterName);
         ParameterHandle recvParameter =
            receiver.getParameterHandle(recvInteraction, parameterName);
         publisher.publishInteractionClass(pubInteraction);
         publisher.changeInteractionOrderType(pubInteraction, OrderType.TIMESTAMP);
         receiver.subscribeInteractionClass(recvInteraction);

         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(regulator.getTimeFactory().getName())
               && timeFactory.getName().equals(receiver.getTimeFactory().getName()),
            "pending-TSO NMR members selected different time factories");
         LogicalTime<?, ?> initial = publisher.queryLogicalTime();
         LogicalTimeInterval<?> epsilon = timeFactory.makeEpsilon();
         LogicalTime<?, ?> time4 = offset(initial, epsilon, 4);
         LogicalTime<?, ?> time5 = offset(initial, epsilon, 5);
         LogicalTime<?, ?> regulatorTarget = available ? time4 : time5;
         LogicalTime<?, ?> time9 = offset(initial, epsilon, 9);
         LogicalTime<?, ?> time10 = offset(initial, epsilon, 10);
         byte[] payload = available
            ? new byte[] {'N', 'M', 'R', 'A', '-', 'T', 'S', 'O'}
            : new byte[] {'N', 'M', 'R', '-', 'T', 'S', 'O'};
         byte[] tag = available
            ? new byte[] {'N', 'M', 'R', 'A', '-', 'R', 'E', 'P', 'L', 'A', 'Y'}
            : new byte[] {'R', 'E', 'S', 'T', 'O', 'R', 'E'};
         ParameterHandleValueMap parameters =
            publisher.getParameterHandleValueMapFactory().create(1);
         parameters.put(pubParameter, payload);

         for (RTIambassador member : members) member.enableTimeConstrained();
         if (!immediate) {
            awaitAll(members, () -> publisherCallbacks.constrainedEnabled == 1
                  && regulatorCallbacks.constrainedEnabled == 1
                  && receiverCallbacks.constrainedEnabled == 1,
               "three pending-TSO NMR constrained enablements");
         }
         JavaTckSupport.check(publisherCallbacks.constrainedEnabled == 1
               && regulatorCallbacks.constrainedEnabled == 1
               && receiverCallbacks.constrainedEnabled == 1,
            "pending-TSO NMR constrained enablement was incomplete");
         publisher.enableTimeRegulation(epsilon);
         regulator.enableTimeRegulation(epsilon);
         if (!immediate) {
            awaitAll(members, () -> publisherCallbacks.regulationEnabled == 1
                  && regulatorCallbacks.regulationEnabled == 1,
               "two pending-TSO NMR regulation enablements");
         }
         JavaTckSupport.check(publisherCallbacks.regulationEnabled == 1
               && regulatorCallbacks.regulationEnabled == 1,
            "pending-TSO NMR regulation enablement was incomplete");

         if (available) receiver.nextMessageRequestAvailable(time10);
         else receiver.nextMessageRequest(time10);
         JavaTckSupport.check(receiverCallbacks.interactions.isEmpty(),
            "receiver saw TSO before the queued send");
         MessageRetractionReturn sent = publisher.sendInteraction(
            pubInteraction, parameters, tag, time5);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid
               && sent.handle != null && receiverCallbacks.interactions.isEmpty()
               && receiverCallbacks.grants.isEmpty(),
            "timestamped interaction was not queued behind pending NMR");
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
               && receiverCallbacks.interactions.size() == 1,
            "pending-TSO NMR save boundary or first delivery was incomplete");
         TimedInteraction original = receiverCallbacks.interactions.get(0);
         JavaTckSupport.check(original.interaction.equals(recvInteraction)
               && original.producer.equals(publisherHandle)
               && original.time.equals(time5)
               && original.sentOrder == OrderType.TIMESTAMP
               && original.receivedOrder == OrderType.TIMESTAMP
               && original.retraction != null
               && Arrays.equals(original.tag, tag)
               && original.parameters.size() == 1
               && Arrays.equals(original.parameters.get(recvParameter), payload),
            "original timestamped interaction changed public callback fields");
         List<String> originalOrder = receiverCallbacks.order;
         JavaTckSupport.check(originalOrder.indexOf("save-initiate") >= 0
               && originalOrder.indexOf("interaction")
                  > originalOrder.indexOf("save-initiate")
               && originalOrder.indexOf("grant") > originalOrder.indexOf("interaction"),
            "original timestamped interaction did not precede NMR grant");

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
               "three pending-TSO NMR restore initiations");
         }
         publisher.federateRestoreComplete();
         regulator.federateRestoreComplete();
         receiver.federateRestoreComplete();
         if (!immediate) {
            awaitAll(members, () -> publisherCallbacks.restoredCount == 1
                  && regulatorCallbacks.restoredCount == 1
                  && receiverCallbacks.restoredCount == 1
                  && receiverCallbacks.interactions.size() == 2
                  && receiverCallbacks.grants.size() == 3,
               "restored pending-TSO NMR delivery and grant");
         }
         JavaTckSupport.check(publisherCallbacks.restoredCount == 1
               && regulatorCallbacks.restoredCount == 1
               && receiverCallbacks.restoredCount == 1
               && receiverCallbacks.interactions.size() == 2
               && receiverCallbacks.grants.size() == 3
               && time5.equals(receiverCallbacks.grants.get(2)),
            "restore did not replay one interaction before restored NMR grant");
         TimedInteraction replayed = receiverCallbacks.interactions.get(1);
         JavaTckSupport.check(replayed.interaction.equals(original.interaction)
               && replayed.producer.equals(original.producer)
               && replayed.time.equals(original.time)
               && replayed.sentOrder == original.sentOrder
               && replayed.receivedOrder == original.receivedOrder
               && (replayed.retraction != null) == (original.retraction != null)
               && Arrays.equals(replayed.tag, tag)
               && replayed.parameters.size() == 1
               && Arrays.equals(replayed.parameters.get(recvParameter), payload),
            "replayed timestamped interaction changed public callback fields");
         List<String> restoreOrder = receiverCallbacks.order;
         JavaTckSupport.check(restoreOrder.indexOf("restore-complete") >= 0
               && restoreOrder.indexOf("interaction")
                  > restoreOrder.indexOf("restore-complete")
               && restoreOrder.indexOf("grant")
                  > restoreOrder.indexOf("interaction")
               && regulatorTarget.equals(publisher.queryLogicalTime())
               && regulatorTarget.equals(regulator.queryLogicalTime())
               && time5.equals(receiver.queryLogicalTime()),
            "restored interaction was not delivered between restoration and grant");

         for (int index = joined.size() - 1; index >= 0; --index) {
            joined.get(index).resignFederationExecution(ResignAction.NO_ACTION);
            joined.remove(index);
         }
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         for (int index = joined.size() - 1; index >= 0; --index) {
            try { joined.get(index).resignFederationExecution(ResignAction.NO_ACTION); }
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
            "evoked pending-NMR save callback thread did not finish");
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

   private static final class TimedInteraction {
      private final InteractionClassHandle interaction;
      private final ParameterHandleValueMap parameters;
      private final byte[] tag;
      private final FederateHandle producer;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;
      private final MessageRetractionHandle retraction;

      private TimedInteraction(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         parameters = ((ParameterHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
         producer = (FederateHandle) arguments[4];
         time = (LogicalTime<?, ?>) arguments[6];
         sentOrder = (OrderType) arguments[7];
         receivedOrder = (OrderType) arguments[8];
         retraction = (MessageRetractionHandle) arguments[9];
      }
   }

   private static final class Recorder {
      private final List<String> saveLabels = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<TimedInteraction> interactions = new CopyOnWriteArrayList<>();
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
               } else if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length == 10) {
                  interactions.add(new TimedInteraction(arguments));
                  order.add("interaction");
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
                  if ("toString".equals(name)) return "Java TCK pending-TSO NMR callbacks";
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
