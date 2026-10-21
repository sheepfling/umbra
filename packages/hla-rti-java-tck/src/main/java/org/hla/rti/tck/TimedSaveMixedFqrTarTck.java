package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.SaveFailureReason;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Four-member timed save with two FQR and one ordinary TAR participant. */
final class TimedSaveMixedFqrTarTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private TimedSaveMixedFqrTarTck() { }

   static void run(RtiFactory factory, String fom,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "mixed FQR/TAR timed save requires evoked callbacks");
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador first = factory.getRtiAmbassador();
      RTIambassador second = factory.getRtiAmbassador();
      RTIambassador third = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder firstCallbacks = new Recorder();
      Recorder secondCallbacks = new Recorder();
      Recorder thirdCallbacks = new Recorder();
      List<RTIambassador> connected = new ArrayList<>();
      List<RTIambassador> joined = new ArrayList<>();
      String federation = "java-tck-timed-save-mixed-fqr-tar-" + UUID.randomUUID();
      String saveLabel = "tck-timed-save-mixed-fqr-tar";
      boolean created = false;
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel);
         connected.add(owner);
         first.connect(firstCallbacks.proxy(), callbackModel);
         connected.add(first);
         second.connect(secondCallbacks.proxy(), callbackModel);
         connected.add(second);
         third.connect(thirdCallbacks.proxy(), callbackModel);
         connected.add(third);
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-mixed-owner", federation);
         joined.add(owner);
         first.joinFederationExecution("java-tck-mixed-first", federation);
         joined.add(first);
         second.joinFederationExecution("java-tck-mixed-second", federation);
         joined.add(second);
         third.joinFederationExecution("java-tck-mixed-third", federation);
         joined.add(third);

         LogicalTimeFactory<?, ?> timeFactory = owner.getTimeFactory();
         for (RTIambassador member : List.of(first, second, third)) {
            JavaTckSupport.check(timeFactory.getName().equals(member.getTimeFactory().getName()),
               "mixed FQR/TAR members selected different logical-time factories");
         }
         LogicalTime<?, ?> initial = owner.queryLogicalTime();
         LogicalTimeInterval<?> epsilon = timeFactory.makeEpsilon();
         LogicalTime<?, ?> saveTime = offset(initial, epsilon, 5);
         LogicalTime<?, ?> flushTime = offset(initial, epsilon, 6);
         LogicalTime<?, ?> laterFlushTime = offset(initial, epsilon, 7);

         for (RTIambassador member : List.of(first, second, third)) {
            member.enableTimeConstrained();
         }
         await(first, () -> firstCallbacks.constrainedEnabled == 1,
            "first mixed member constrained-role enablement");
         await(second, () -> secondCallbacks.constrainedEnabled == 1,
            "second mixed member constrained-role enablement");
         await(third, () -> thirdCallbacks.constrainedEnabled == 1,
            "third mixed member constrained-role enablement");
         owner.enableTimeRegulation(epsilon);
         await(owner, () -> ownerCallbacks.regulationEnabled == 1,
            "mixed FQR/TAR regulator enablement");

         owner.requestFederationSave(saveLabel, saveTime);
         first.flushQueueRequest(flushTime);
         second.flushQueueRequest(laterFlushTime);
         third.timeAdvanceRequest(saveTime);
         owner.timeAdvanceRequest(saveTime);

         await(first, () -> firstCallbacks.labels.size() == 1
               && firstCallbacks.flushGrants.size() == 1,
            "first mixed FQR save initiation and grant");
         checkFqr(firstCallbacks, saveLabel, saveTime, flushTime,
            "first mixed FQR member");
         JavaTckSupport.check(secondCallbacks.labels.isEmpty()
               && thirdCallbacks.labels.isEmpty()
               && ownerCallbacks.labels.isEmpty(),
            "another mixed member received save initiation without callback service");

         await(second, () -> secondCallbacks.labels.size() == 1
               && secondCallbacks.flushGrants.size() == 1,
            "second mixed FQR save initiation and grant");
         checkFqr(secondCallbacks, saveLabel, saveTime, flushTime,
            "second mixed FQR member");
         JavaTckSupport.check(thirdCallbacks.labels.isEmpty()
               && ownerCallbacks.labels.isEmpty(),
            "TAR or owner member received save initiation without callback service");

         await(third, () -> thirdCallbacks.labels.size() == 1
               && thirdCallbacks.grants.size() == 1,
            "ordinary TAR mixed save initiation and grant");
         JavaTckSupport.check(thirdCallbacks.labels.equals(List.of(saveLabel))
               && thirdCallbacks.saveTimes.size() == 1
               && saveTime.equals(thirdCallbacks.saveTimes.get(0))
               && saveTime.equals(thirdCallbacks.grants.get(0))
               && thirdCallbacks.order.equals(List.of("save-initiate", "grant")),
            "ordinary TAR member did not initiate save before its boundary grant");
         JavaTckSupport.check(ownerCallbacks.labels.isEmpty(),
            "owner received save initiation without callback service");

         await(owner, () -> ownerCallbacks.labels.size() == 1,
            "mixed FQR/TAR owner save initiation");
         JavaTckSupport.check(ownerCallbacks.labels.equals(List.of(saveLabel))
               && ownerCallbacks.saveTimes.size() == 1
               && saveTime.equals(ownerCallbacks.saveTimes.get(0)),
            "owner did not receive matching timed save initiation");

         for (RTIambassador member : List.of(owner, first, second, third)) {
            member.federateSaveBegun();
         }
         for (RTIambassador member : List.of(owner, first, second, third)) {
            member.federateSaveComplete();
         }
         awaitAll(List.of(owner, first, second, third),
            () -> ownerCallbacks.savedCount == 1 && firstCallbacks.savedCount == 1
               && secondCallbacks.savedCount == 1 && thirdCallbacks.savedCount == 1,
            "four-member mixed FQR/TAR save completion");
         for (Recorder recorder : List.of(ownerCallbacks, firstCallbacks,
               secondCallbacks, thirdCallbacks)) {
            JavaTckSupport.check(recorder.failures.isEmpty(),
               "mixed FQR/TAR member received Federation Not Saved");
         }

         for (RTIambassador member : List.of(first, second, third)) {
            member.disableTimeConstrained();
         }
         owner.disableTimeRegulation();
         for (int index = joined.size() - 1; index >= 0; --index) {
            joined.get(index).resignFederationExecution(ResignAction.NO_ACTION);
            joined.remove(index);
         }
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         for (int index = joined.size() - 1; index >= 0; --index) {
            try { joined.get(index).resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         for (int index = connected.size() - 1; index >= 0; --index) {
            try { connected.get(index).disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void checkFqr(Recorder callbacks, String label,
         LogicalTime<?, ?> saveTime, LogicalTime<?, ?> grantTime,
         String description) {
      JavaTckSupport.check(callbacks.labels.equals(List.of(label))
            && callbacks.saveTimes.size() == 1
            && saveTime.equals(callbacks.saveTimes.get(0))
            && grantTime.equals(callbacks.flushGrants.get(0))
            && callbacks.order.equals(List.of("save-initiate", "flush-grant")),
         description + " did not initiate save before actual grant 6");
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime offset(LogicalTime start,
         LogicalTimeInterval interval, int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) {
         result = (LogicalTime) result.add(interval);
      }
      return result;
   }

   private static void await(RTIambassador ambassador,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         ambassador.evokeCallback(0.0);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void awaitAll(List<RTIambassador> members,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         for (RTIambassador member : members) member.evokeCallback(0.0);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static final class Recorder {
      private final List<String> labels = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> saveTimes = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> flushGrants = new CopyOnWriteArrayList<>();
      private final List<String> order = new CopyOnWriteArrayList<>();
      private final List<SaveFailureReason> failures = new CopyOnWriteArrayList<>();
      private volatile int regulationEnabled;
      private volatile int constrainedEnabled;
      private volatile int savedCount;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) ++regulationEnabled;
               else if ("timeConstrainedEnabled".equals(name)) ++constrainedEnabled;
               else if ("timeAdvanceGrant".equals(name)) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
                  order.add("grant");
               } else if ("flushQueueGrant".equals(name)) {
                  flushGrants.add((LogicalTime<?, ?>) arguments[0]);
                  order.add("flush-grant");
               } else if ("initiateFederateSave".equals(name)) {
                  labels.add((String) arguments[0]);
                  if (arguments.length == 2) {
                     saveTimes.add((LogicalTime<?, ?>) arguments[1]);
                  }
                  order.add("save-initiate");
               } else if ("federationSaved".equals(name)) {
                  ++savedCount;
                  order.add("save-complete");
               } else if ("federationNotSaved".equals(name)) {
                  failures.add((SaveFailureReason) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK mixed FQR/TAR callbacks";
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
