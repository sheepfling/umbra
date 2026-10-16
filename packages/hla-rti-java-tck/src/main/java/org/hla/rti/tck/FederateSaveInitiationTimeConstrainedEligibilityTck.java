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

/** Checks save initiation eligibility while time-constrained members are advancing. */
final class FederateSaveInitiationTimeConstrainedEligibilityTck {
   private FederateSaveInitiationTimeConstrainedEligibilityTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, CallbackModel callbackModel) throws Exception {
      RTIambassador requester = factory.getRtiAmbassador();
      RTIambassador first = factory.getRtiAmbassador();
      RTIambassador second = factory.getRtiAmbassador();
      Recorder requesterRecorder = new Recorder();
      Recorder firstRecorder = new Recorder();
      Recorder secondRecorder = new Recorder();
      String federation = "java-tck-save-time-eligibility-" + UUID.randomUUID();
      String label = "java-tck-time-constrained-save-eligibility";
      boolean requesterConnected = false, firstConnected = false, secondConnected = false;
      boolean created = false, requesterJoined = false, firstJoined = false, secondJoined = false;
      boolean firstConstrained = false, secondConstrained = false, regulating = false;
      try {
         requester.connect(requesterRecorder.proxy(), callbackModel);
         requesterConnected = true;
         first.connect(firstRecorder.proxy(), callbackModel);
         firstConnected = true;
         second.connect(secondRecorder.proxy(), callbackModel);
         secondConnected = true;
         requester.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         requester.joinFederationExecution("java-tck-save-eligibility-requester", federation);
         requesterJoined = true;
         first.joinFederationExecution("java-tck-save-eligibility-first", federation);
         firstJoined = true;
         second.joinFederationExecution("java-tck-save-eligibility-second", federation);
         secondJoined = true;

         LogicalTimeFactory<?, ?> timeFactory = requester.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(first.getTimeFactory().getName())
               && timeFactory.getName().equals(second.getTimeFactory().getName()),
            "save-eligibility members selected different logical-time factories");
         LogicalTime initial = (LogicalTime) timeFactory.makeInitial();
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();

         requester.enableTimeRegulation(epsilon);
         regulating = true;
         await(callbackModel, new RTIambassador[] {requester}, objectClassName,
            () -> requesterRecorder.timeRegulationEnabled == 1,
            "requester time-regulation callback");
         first.enableTimeConstrained();
         firstConstrained = true;
         await(callbackModel, new RTIambassador[] {first}, objectClassName,
            () -> firstRecorder.timeConstrainedEnabled == 1,
            "first constrained callback");
         second.enableTimeConstrained();
         secondConstrained = true;
         await(callbackModel, new RTIambassador[] {second}, objectClassName,
            () -> secondRecorder.timeConstrainedEnabled == 1,
            "second constrained callback");

         requester.requestFederationSave(label);
         serviceAll(callbackModel, objectClassName,
            new RTIambassador[] {requester, first, second});
         JavaTckSupport.check(noSaveInitiations(requesterRecorder, firstRecorder, secondRecorder),
            "untimed save began before any constrained member was Time Advancing");

         LogicalTime firstTarget = add(initial, intervalOffset(epsilon, 5));
         first.timeAdvanceRequest(firstTarget);
         serviceAll(callbackModel, objectClassName,
            new RTIambassador[] {requester, first, second});
         JavaTckSupport.check(noSaveInitiations(requesterRecorder, firstRecorder, secondRecorder)
               && firstRecorder.grants.isEmpty(),
            "untimed save began before every constrained TAR was accepted");

         LogicalTime secondTarget = add(initial, intervalOffset(epsilon, 5));
         second.timeAdvanceRequest(secondTarget);
         await(callbackModel, new RTIambassador[] {requester, first, second}, objectClassName,
            () -> requesterRecorder.saveInitiations.size() == 1
               && firstRecorder.saveInitiations.size() == 1
               && secondRecorder.saveInitiations.size() == 1,
            "save initiation after both constrained TARs");
         checkInitiation(requesterRecorder, label, "requester");
         checkInitiation(firstRecorder, label, "first constrained member");
         checkInitiation(secondRecorder, label, "second constrained member");
         JavaTckSupport.check(firstRecorder.grants.isEmpty() && secondRecorder.grants.isEmpty(),
            "a constrained member received a time grant before save initiation");

         requester.federateSaveBegun();
         first.federateSaveBegun();
         second.federateSaveBegun();
         requester.federateSaveComplete();
         first.federateSaveComplete();
         second.federateSaveComplete();
         await(callbackModel, new RTIambassador[] {requester, first, second}, objectClassName,
            () -> requesterRecorder.savedCount == 1 && firstRecorder.savedCount == 1
               && secondRecorder.savedCount == 1,
            "Federation Saved after every member completes");
         JavaTckSupport.check(requesterRecorder.saveFailures.isEmpty()
               && firstRecorder.saveFailures.isEmpty() && secondRecorder.saveFailures.isEmpty(),
            "eligible untimed save unexpectedly failed");

         requester.timeAdvanceRequest(add(initial, intervalOffset(epsilon, 10)));
         await(callbackModel, new RTIambassador[] {requester, first, second}, objectClassName,
            () -> requesterRecorder.grants.size() == 1 && firstRecorder.grants.size() == 1
               && secondRecorder.grants.size() == 1,
            "time grants after save completion");
      } finally {
         if (secondConstrained && secondJoined) {
            try { second.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (firstConstrained && firstJoined) {
            try { first.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (regulating && requesterJoined) {
            try { requester.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (secondJoined) resign(second);
         if (firstJoined) resign(first);
         if (requesterJoined) resign(requester);
         if (created) {
            try { requester.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (secondConnected) disconnect(second);
         if (firstConnected) disconnect(first);
         if (requesterConnected) disconnect(requester);
      }
   }

   private static boolean noSaveInitiations(Recorder... recorders) {
      for (Recorder recorder : recorders) {
         if (!recorder.saveInitiations.isEmpty()) return false;
      }
      return true;
   }

   private static void checkInitiation(Recorder recorder, String expected, String member) {
      JavaTckSupport.check(recorder.saveInitiations.size() == 1
            && expected.equals(recorder.saveInitiations.get(0)),
         member + " did not receive exactly one initiation for the requested save");
      JavaTckSupport.check(recorder.callbackOrder.equals(List.of("initiateFederateSave")),
         member + " was granted time before its save-initiation callback");
   }

   private static void serviceAll(CallbackModel callbackModel, String objectClassName,
         RTIambassador[] ambassadors) throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) {
         for (RTIambassador ambassador : ambassadors) {
            ambassador.evokeMultipleCallbacks(0.0, 0.0);
         }
      }
   }

   private static void await(CallbackModel callbackModel, RTIambassador[] ambassadors,
         String objectClassName, Condition condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            for (RTIambassador ambassador : ambassadors) {
               ambassador.evokeMultipleCallbacks(0.0, 0.0);
            }
         } else if (ambassadors.length > 0) {
            ambassadors[0].getObjectClassHandle(objectClassName);
         }
         if (condition.isMet()) return;
         Thread.yield();
      }
      throw new AssertionError(description + " did not arrive before the deadline");
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

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   @FunctionalInterface
   private interface Condition { boolean isMet() throws Exception; }

   private static final class Recorder {
      private int timeRegulationEnabled;
      private int timeConstrainedEnabled;
      private int savedCount;
      private final List<String> saveInitiations = new CopyOnWriteArrayList<>();
      private final List<String> saveFailures = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) timeRegulationEnabled++;
               if ("timeConstrainedEnabled".equals(name)) timeConstrainedEnabled++;
               if ("initiateFederateSave".equals(name)) {
                  saveInitiations.add((String) arguments[0]);
                  callbackOrder.add(name);
               }
               if ("federationSaved".equals(name)) savedCount++;
               if ("federationNotSaved".equals(name)) {
                  saveFailures.add(String.valueOf(arguments[0]));
               }
               if ("timeAdvanceGrant".equals(name)) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK save eligibility recorder";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
