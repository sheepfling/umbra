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

/** A strict NMRA timed-save callback rejects resignation before its grant. */
final class TimedSaveNmraCallbackResignationTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private TimedSaveNmraCallbackResignationTck() { }

   static void run(RtiFactory factory, String fom,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "timed NMRA save-callback resignation requires evoked callbacks");
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador nmra = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder nmraCallbacks = new Recorder(nmra);
      String federation = "java-tck-timed-save-nmra-active-" + UUID.randomUUID();
      String saveLabel = "tck-timed-nmra-callback-resignation-save";
      boolean ownerConnected = false;
      boolean nmraConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean nmraJoined = false;
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel);
         ownerConnected = true;
         nmra.connect(nmraCallbacks.proxy(), callbackModel);
         nmraConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-timed-nmra-owner", federation);
         ownerJoined = true;
         nmra.joinFederationExecution("java-tck-timed-nmra-member", federation);
         nmraJoined = true;

         LogicalTimeFactory<?, ?> timeFactory = owner.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(nmra.getTimeFactory().getName()),
            "timed NMRA save members selected different logical-time factories");
         LogicalTime<?, ?> initial = owner.queryLogicalTime();
         LogicalTimeInterval<?> epsilon = timeFactory.makeEpsilon();
         LogicalTime<?, ?> saveTime = offset(initial, epsilon, 5);
         LogicalTime<?, ?> advanceTime = offset(initial, epsilon, 6);
         nmra.enableTimeConstrained();
         await(nmra, () -> nmraCallbacks.constrainedEnabled == 1,
            "timed NMRA save constrained-role enablement");
         owner.enableTimeRegulation(epsilon);
         await(owner, () -> ownerCallbacks.regulationEnabled == 1,
            "timed NMRA save regulator enablement");

         nmraCallbacks.resignOnInitiate = true;
         owner.requestFederationSave(saveLabel, saveTime);
         nmra.nextMessageRequestAvailable(advanceTime);
         owner.timeAdvanceRequest(saveTime);
         await(owner, () -> ownerCallbacks.grants.size() == 1,
            "timed NMRA save regulator grant");
         JavaTckSupport.check(saveTime.equals(ownerCallbacks.grants.get(0))
               && ownerCallbacks.labels.isEmpty(),
            "regulator did not grant the save time before its initiation callback");

         await(nmra,
            () -> nmraCallbacks.labels.size() == 1 && nmraCallbacks.grants.size() == 1,
            "timed NMRA save initiation and grant");
         JavaTckSupport.check(nmraCallbacks.resignationRejectedInsideCallback,
            "resignation inside timed NMRA Initiate Federate Save was not rejected");
         JavaTckSupport.check(nmraCallbacks.labels.equals(List.of(saveLabel))
               && nmraCallbacks.saveTimes.size() == 1
               && saveTime.equals(nmraCallbacks.saveTimes.get(0))
               && advanceTime.equals(nmraCallbacks.grants.get(0))
               && nmraCallbacks.order.equals(List.of("save-initiate", "grant")),
            "timed NMRA save did not initiate before its matching advance grant");

         await(owner, () -> ownerCallbacks.labels.size() == 1,
            "owner timed NMRA save initiation");
         JavaTckSupport.check(ownerCallbacks.labels.equals(List.of(saveLabel))
               && ownerCallbacks.saveTimes.size() == 1
               && saveTime.equals(ownerCallbacks.saveTimes.get(0))
               && ownerCallbacks.order.equals(List.of("grant", "save-initiate")),
            "owner did not receive grant before its timed NMRA save initiation");
         owner.federateSaveBegun();
         nmra.federateSaveBegun();
         owner.federateSaveComplete();
         nmra.federateSaveComplete();
         awaitBoth(owner, nmra,
            () -> ownerCallbacks.savedCount == 1 && nmraCallbacks.savedCount == 1,
            "timed NMRA save completion after rejected callback resignation");
         JavaTckSupport.check(ownerCallbacks.failures.isEmpty()
               && nmraCallbacks.failures.isEmpty()
               && ownerCallbacks.order.equals(
                  List.of("grant", "save-initiate", "save-complete"))
               && nmraCallbacks.order.equals(
                  List.of("save-initiate", "grant", "save-complete")),
            "rejected callback-time resignation incorrectly failed the timed NMRA save");

         nmra.disableTimeConstrained();
         owner.disableTimeRegulation();
         nmra.resignFederationExecution(ResignAction.NO_ACTION);
         nmraJoined = false;
         owner.resignFederationExecution(ResignAction.NO_ACTION);
         ownerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (nmraJoined) resign(nmra);
         if (ownerJoined) resign(owner);
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (nmraConnected) disconnect(nmra);
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
                  if ("toString".equals(name)) return "Java TCK timed NMRA save callbacks";
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
