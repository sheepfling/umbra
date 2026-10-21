package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.SaveFailureReason;
import hla.rti1516_2025.exceptions.CallNotAllowedFromWithinCallback;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** A strict TARA timed-save callback rejects resignation before its grant. */
final class TimedSaveTaraCallbackResignationTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private TimedSaveTaraCallbackResignationTck() { }

   static void run(RtiFactory factory, String fom,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "timed TARA save-callback resignation requires evoked callbacks");
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador tara = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder taraCallbacks = new Recorder(tara);
      String federation = "java-tck-timed-save-tara-active-" + UUID.randomUUID();
      String saveLabel = "tck-timed-tara-callback-resignation-save";
      boolean ownerConnected = false;
      boolean taraConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean taraJoined = false;
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel);
         ownerConnected = true;
         tara.connect(taraCallbacks.proxy(), callbackModel);
         taraConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-timed-tara-owner", federation);
         ownerJoined = true;
         tara.joinFederationExecution("java-tck-timed-tara-member", federation);
         taraJoined = true;

         LogicalTimeFactory<?, ?> timeFactory = owner.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(tara.getTimeFactory().getName()),
            "timed TARA save members selected different logical-time factories");
         LogicalTime<?, ?> initial = owner.queryLogicalTime();
         LogicalTimeInterval<?> epsilon = timeFactory.makeEpsilon();
         LogicalTime<?, ?> saveTime = offset(initial, epsilon, 5);
         LogicalTime<?, ?> advanceTime = offset(initial, epsilon, 6);
         tara.enableTimeConstrained();
         await(tara, () -> taraCallbacks.constrainedEnabled == 1,
            "timed TARA save constrained-role enablement");
         owner.enableTimeRegulation(epsilon);
         await(owner, () -> ownerCallbacks.regulationEnabled == 1,
            "timed TARA save regulator enablement");

         taraCallbacks.resignOnInitiate = true;
         owner.requestFederationSave(saveLabel, saveTime);
         tara.timeAdvanceRequestAvailable(advanceTime);
         owner.timeAdvanceRequest(saveTime);
         await(owner, () -> ownerCallbacks.grants.size() == 1,
            "timed TARA save regulator grant");
         JavaTckSupport.check(saveTime.equals(ownerCallbacks.grants.get(0))
               && ownerCallbacks.labels.isEmpty(),
            "regulator did not grant the save time before its initiation callback");

         await(tara,
            () -> taraCallbacks.labels.size() == 1 && taraCallbacks.grants.size() == 1,
            "timed TARA save initiation and grant");
         JavaTckSupport.check(taraCallbacks.resignationRejectedInsideCallback,
            "resignation inside timed TARA Initiate Federate Save was not rejected");
         JavaTckSupport.check(taraCallbacks.labels.equals(List.of(saveLabel))
               && taraCallbacks.saveTimes.size() == 1
               && saveTime.equals(taraCallbacks.saveTimes.get(0))
               && advanceTime.equals(taraCallbacks.grants.get(0))
               && taraCallbacks.order.equals(List.of("save-initiate", "grant")),
            "timed TARA save did not initiate before its matching advance grant");

         await(owner, () -> ownerCallbacks.labels.size() == 1,
            "owner timed TARA save initiation");
         JavaTckSupport.check(ownerCallbacks.labels.equals(List.of(saveLabel))
               && ownerCallbacks.saveTimes.size() == 1
               && saveTime.equals(ownerCallbacks.saveTimes.get(0))
               && ownerCallbacks.order.equals(List.of("grant", "save-initiate")),
            "owner did not receive grant before its timed TARA save initiation");
         owner.federateSaveBegun();
         tara.federateSaveBegun();
         owner.federateSaveComplete();
         tara.federateSaveComplete();
         awaitBoth(owner, tara,
            () -> ownerCallbacks.savedCount == 1 && taraCallbacks.savedCount == 1,
            "timed TARA save completion after rejected callback resignation");
         JavaTckSupport.check(ownerCallbacks.failures.isEmpty()
               && taraCallbacks.failures.isEmpty()
               && ownerCallbacks.order.equals(
                  List.of("grant", "save-initiate", "save-complete"))
               && taraCallbacks.order.equals(
                  List.of("save-initiate", "grant", "save-complete")),
            "rejected callback-time resignation incorrectly failed the timed TARA save");

         tara.disableTimeConstrained();
         owner.disableTimeRegulation();
         tara.resignFederationExecution(ResignAction.NO_ACTION);
         taraJoined = false;
         owner.resignFederationExecution(ResignAction.NO_ACTION);
         ownerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (taraJoined) resign(tara);
         if (ownerJoined) resign(owner);
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (taraConnected) disconnect(tara);
         if (ownerConnected) disconnect(owner);
      }
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime offset(LogicalTime start,
         LogicalTimeInterval interval, int steps) throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(interval);
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

   private static void awaitBoth(RTIambassador first, RTIambassador second,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         first.evokeCallback(0.0);
         second.evokeCallback(0.0);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static final class Recorder {
      private final RTIambassador ambassador;
      private final List<String> labels = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> saveTimes = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<String> order = new CopyOnWriteArrayList<>();
      private final List<SaveFailureReason> failures = new CopyOnWriteArrayList<>();
      private volatile int regulationEnabled;
      private volatile int constrainedEnabled;
      private volatile int savedCount;
      private volatile boolean resignOnInitiate;
      private volatile boolean resignationRejectedInsideCallback;

      private Recorder() { this(null); }
      private Recorder(RTIambassador ambassador) { this.ambassador = ambassador; }

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
               } else if ("initiateFederateSave".equals(name)) {
                  labels.add((String) arguments[0]);
                  if (arguments.length == 2) {
                     saveTimes.add((LogicalTime<?, ?>) arguments[1]);
                  }
                  order.add("save-initiate");
                  if (resignOnInitiate) {
                     try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
                     catch (CallNotAllowedFromWithinCallback expected) {
                        resignationRejectedInsideCallback = true;
                     }
                  }
               } else if ("federationSaved".equals(name)) {
                  ++savedCount;
                  order.add("save-complete");
               } else if ("federationNotSaved".equals(name)) {
                  failures.add((SaveFailureReason) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK timed TARA save callbacks";
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
