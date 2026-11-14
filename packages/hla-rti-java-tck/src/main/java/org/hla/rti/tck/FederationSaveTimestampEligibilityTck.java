package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TimeQueryReturn;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Checks timestamped Federation Save eligibility before and after regulation. */
final class FederationSaveTimestampEligibilityTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private FederationSaveTimestampEligibilityTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador ambassador = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      String federation = "java-tck-save-time-eligibility-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      boolean regulating = false;
      try {
         ambassador.connect(recorder.proxy(), callbackModel);
         connected = true;
         ambassador.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         ambassador.joinFederationExecution("java-tck-save-time-eligibility", federation);
         joined = true;

         LogicalTimeFactory<?, ?> timeFactory = ambassador.getTimeFactory();
         LogicalTime<?, ?> initial = ambassador.queryLogicalTime();
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         TimeQueryReturn galt = ambassador.queryGALT();
         JavaTckSupport.check(galt != null && !galt.timeIsValid,
            "GALT was defined before any federate enabled time regulation");

         LogicalTime<?, ?> saveTime = offset(initial, epsilon, 3);
         String rejectedLabel = "java-tck-save-time-ineligible";
         JavaTckSupport.expectFailure(
            () -> ambassador.requestFederationSave(rejectedLabel, saveTime),
            "FederateUnableToUseTime",
            "timestamped Federation Save by a non-regulating federate with undefined GALT");
         requireNoSaveCallbacks(ambassador, callbackModel, recorder);

         ambassador.enableTimeRegulation(epsilon);
         regulating = true;
         await(ambassador, callbackModel, () -> recorder.timeRegulationEnabled == 1,
            "time-regulation enablement for the eligible save request");

         String acceptedLabel = "java-tck-save-time-eligible-regulator";
         ambassador.requestFederationSave(acceptedLabel, saveTime);
         await(ambassador, callbackModel, () -> recorder.saveInitiations.size() == 1,
            "timestamped save initiation for a time-regulating federate");
         JavaTckSupport.check(recorder.saveInitiations.equals(Arrays.asList(acceptedLabel))
               && recorder.saveTimes.size() == 1
               && saveTime.equals(recorder.saveTimes.get(0)),
            "eligible timestamped Federation Save returned the wrong label or time");

         ambassador.federateSaveBegun();
         ambassador.federateSaveComplete();
         await(ambassador, callbackModel, () -> recorder.federationSavedCount == 1,
            "timestamped save completion for a time-regulating federate");
         JavaTckSupport.check(recorder.federationNotSaved.isEmpty()
               && recorder.callbackOrder.equals(Arrays.asList(
                  "initiateFederateSave", "federationSaved")),
            "eligible timestamped Federation Save reported failure or wrong callback order");

         ambassador.disableTimeRegulation();
         regulating = false;
         ambassador.resignFederationExecution(ResignAction.NO_ACTION);
         joined = false;
         ambassador.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (regulating && joined) {
            try { ambassador.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (joined) {
            try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { ambassador.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) {
            try { ambassador.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void requireNoSaveCallbacks(RTIambassador ambassador,
         CallbackModel callbackModel, Recorder recorder) throws Exception {
      long deadline = System.nanoTime() + 150_000_000L;
      while (System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(ambassador);
         }
         JavaTckSupport.check(recorder.saveInitiations.isEmpty()
               && recorder.saveTimes.isEmpty()
               && recorder.federationSavedCount == 0
               && recorder.federationNotSaved.isEmpty(),
            "rejected timestamped Federation Save left a save pending or emitted callbacks");
         Thread.sleep(1L);
      }
   }

   @FunctionalInterface
   private interface Ready { boolean isReady(); }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         Ready ready, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !ready.isReady()) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(ambassador);
         } else {
            Thread.sleep(1L);
         }
      }
      JavaTckSupport.check(ready.isReady(), "timed out waiting for " + description);
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime<?, ?> offset(LogicalTime start,
         LogicalTimeInterval interval, int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) {
         result = (LogicalTime) result.add(interval);
      }
      return result;
   }

   private static Object objectMethod(Object proxy, String name, Object[] arguments) {
      if ("toString".equals(name)) return "Java TCK timestamped save callbacks";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
      return null;
   }

   private static final class Recorder {
      private volatile int timeRegulationEnabled;
      private volatile int federationSavedCount;
      private final List<String> saveInitiations = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> saveTimes = new CopyOnWriteArrayList<>();
      private final List<String> federationNotSaved = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled++;
               } else if ("initiateFederateSave".equals(name)
                     && arguments != null && arguments.length == 2) {
                  saveInitiations.add((String) arguments[0]);
                  saveTimes.add((LogicalTime<?, ?>) arguments[1]);
                  callbackOrder.add(name);
               } else if ("federationSaved".equals(name)) {
                  federationSavedCount++;
                  callbackOrder.add(name);
               } else if ("federationNotSaved".equals(name)) {
                  federationNotSaved.add(arguments == null || arguments.length == 0
                     ? "unknown" : String.valueOf(arguments[0]));
                  callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  return objectMethod(proxy, name, arguments);
               }
               return null;
            });
      }
   }
}
