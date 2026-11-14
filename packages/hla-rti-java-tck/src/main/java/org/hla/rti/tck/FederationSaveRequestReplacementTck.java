package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Checks replacement of an outstanding timestamped Federation Save request. */
final class FederationSaveRequestReplacementTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private FederationSaveRequestReplacementTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador constrained = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder constrainedRecorder = new Recorder();
      String federation = "java-tck-save-request-replacement-" + UUID.randomUUID();
      String supersededLabel = "java-tck-save-request-superseded";
      String replacementLabel = "java-tck-save-request-replacement";
      boolean ownerConnected = false;
      boolean constrainedConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean constrainedJoined = false;
      boolean ownerRegulating = false;
      boolean constrainedTimeConstrained = false;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         constrained.connect(constrainedRecorder.proxy(), callbackModel);
         constrainedConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-save-replacement-owner", federation);
         ownerJoined = true;
         constrained.joinFederationExecution(
            "java-tck-save-replacement-constrained", federation);
         constrainedJoined = true;

         LogicalTimeFactory<?, ?> timeFactory = owner.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(
               constrained.getTimeFactory().getName()),
            "save-request members selected different logical-time factories");
         LogicalTime<?, ?> initial = owner.queryLogicalTime();
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         LogicalTime<?, ?> supersededTime = offset(initial, epsilon, 5);
         LogicalTime<?, ?> replacementTime = offset(initial, epsilon, 7);

         constrained.enableTimeConstrained();
         constrainedTimeConstrained = true;
         await(callbackModel, new RTIambassador[] {constrained},
            () -> constrainedRecorder.timeConstrainedEnabled == 1,
            "save-request constrained-role enablement");
         owner.enableTimeRegulation(epsilon);
         ownerRegulating = true;
         await(callbackModel, new RTIambassador[] {owner},
            () -> ownerRecorder.timeRegulationEnabled == 1,
            "save-request regulator enablement");

         owner.requestFederationSave(supersededLabel, supersededTime);
         owner.requestFederationSave(replacementLabel, replacementTime);

         constrained.timeAdvanceRequest(supersededTime);
         owner.timeAdvanceRequest(supersededTime);
         await(callbackModel, new RTIambassador[] {owner, constrained},
            () -> ownerRecorder.grants.size() == 1
               && constrainedRecorder.grants.size() == 1,
            "first time advance before the replacement save timestamp");
         JavaTckSupport.check(supersededTime.equals(ownerRecorder.grants.get(0))
               && supersededTime.equals(constrainedRecorder.grants.get(0))
               && ownerRecorder.saveInitiations.isEmpty()
               && constrainedRecorder.saveInitiations.isEmpty(),
            "superseded save request initiated at its former timestamp");

         constrained.timeAdvanceRequest(replacementTime);
         owner.timeAdvanceRequest(replacementTime);
         await(callbackModel, new RTIambassador[] {owner, constrained},
            () -> ownerRecorder.grants.size() == 2
               && constrainedRecorder.grants.size() == 2
               && ownerRecorder.saveInitiations.size() == 1
               && constrainedRecorder.saveInitiations.size() == 1,
            "replacement save initiation at its requested timestamp");

         checkReplacementInitiation(ownerRecorder, replacementLabel,
            replacementTime, "owner");
         checkReplacementInitiation(constrainedRecorder, replacementLabel,
            replacementTime, "time-constrained member");
         JavaTckSupport.check(replacementTime.equals(ownerRecorder.grants.get(1))
               && replacementTime.equals(constrainedRecorder.grants.get(1))
               && ownerRecorder.federationSavedCount == 0
               && constrainedRecorder.federationSavedCount == 0,
            "replacement save did not initiate at its requested timestamp");

         owner.federateSaveBegun();
         constrained.federateSaveBegun();
         owner.federateSaveComplete();
         constrained.federateSaveComplete();
         await(callbackModel, new RTIambassador[] {owner, constrained},
            () -> ownerRecorder.federationSavedCount == 1
               && constrainedRecorder.federationSavedCount == 1,
            "replacement save completion");
         JavaTckSupport.check(ownerRecorder.federationNotSaved.isEmpty()
               && constrainedRecorder.federationNotSaved.isEmpty(),
            "replacement save failed instead of completing");

         constrained.disableTimeConstrained();
         constrainedTimeConstrained = false;
         owner.disableTimeRegulation();
         ownerRegulating = false;
         constrained.resignFederationExecution(ResignAction.NO_ACTION);
         constrainedJoined = false;
         owner.resignFederationExecution(ResignAction.NO_ACTION);
         ownerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (constrainedTimeConstrained && constrainedJoined) {
            try { constrained.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (ownerRegulating && ownerJoined) {
            try { owner.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (constrainedJoined) resign(constrained);
         if (ownerJoined) resign(owner);
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (constrainedConnected) disconnect(constrained);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static void checkReplacementInitiation(Recorder recorder, String label,
         LogicalTime<?, ?> time, String member) {
      JavaTckSupport.check(recorder.saveInitiations.size() == 1
            && label.equals(recorder.saveInitiations.get(0).label)
            && time.equals(recorder.saveInitiations.get(0).time),
         member + " did not receive exactly one initiation with the replacement label and time");
   }

   private static void await(CallbackModel callbackModel,
         RTIambassador[] ambassadors, Ready ready, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !ready.isReady()) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            for (RTIambassador ambassador : ambassadors) {
               JavaTckSupport.drain(ambassador);
            }
         } else {
            Thread.sleep(1L);
         }
      }
      JavaTckSupport.check(ready.isReady(), "timed out waiting for " + description);
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime<?, ?> offset(LogicalTime start,
         LogicalTimeInterval epsilon, int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) {
         result = (LogicalTime) result.add(epsilon);
      }
      return result;
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static Object objectMethod(Object proxy, String name, Object[] arguments) {
      if ("toString".equals(name)) return "Java TCK save-request replacement callbacks";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
      return null;
   }

   @FunctionalInterface
   private interface Ready { boolean isReady(); }

   private static final class SaveInitiation {
      private final String label;
      private final LogicalTime<?, ?> time;

      private SaveInitiation(String label, LogicalTime<?, ?> time) {
         this.label = label;
         this.time = time;
      }
   }

   private static final class Recorder {
      private volatile int timeRegulationEnabled;
      private volatile int timeConstrainedEnabled;
      private volatile int federationSavedCount;
      private final List<SaveInitiation> saveInitiations = new CopyOnWriteArrayList<>();
      private final List<String> federationNotSaved = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled++;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled++;
               } else if ("initiateFederateSave".equals(name)) {
                  String label = arguments == null || arguments.length == 0
                     ? null : (String) arguments[0];
                  LogicalTime<?, ?> time = arguments != null && arguments.length > 1
                     ? (LogicalTime<?, ?>) arguments[1] : null;
                  saveInitiations.add(new SaveInitiation(label, time));
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length > 0) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
               } else if ("federationSaved".equals(name)) {
                  federationSavedCount++;
               } else if ("federationNotSaved".equals(name)) {
                  federationNotSaved.add(arguments == null || arguments.length == 0
                     ? "unknown" : String.valueOf(arguments[0]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  return objectMethod(proxy, name, arguments);
               }
               return null;
            });
      }
   }
}
