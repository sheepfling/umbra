package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.atomic.AtomicInteger;

/** Checks untimed save initiation for every unconstrained joined federate. */
final class FederateSaveInitiationEligibilityTck {
   private FederateSaveInitiationEligibilityTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador requester = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      Recorder requesterEvents = new Recorder();
      Recorder memberEvents = new Recorder();
      String federation = "java-tck-save-initiation-eligibility-" + UUID.randomUUID();
      String label = "java-tck-unconstrained-federate-save";
      boolean created = false;
      try {
         requester.connect(requesterEvents.proxy(), callbackModel);
         member.connect(memberEvents.proxy(), callbackModel);
         requester.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         requester.joinFederationExecution("java-tck-save-requester", federation);
         member.joinFederationExecution("java-tck-save-member", federation);

         requester.requestFederationSave(label);
         await(callbackModel, requester, member,
            () -> requesterEvents.saveInitiations.size() == 1
               && memberEvents.saveInitiations.size() == 1,
            "Initiate Federate Save for every unconstrained joined federate");
         JavaTckSupport.check(requesterEvents.saveInitiations.equals(List.of(label))
               && memberEvents.saveInitiations.equals(List.of(label)),
            "the untimed save did not initiate every joined unconstrained federate exactly once");

         requester.federateSaveBegun();
         member.federateSaveBegun();
         requester.federateSaveComplete();
         member.federateSaveComplete();
         await(callbackModel, requester, member,
            () -> requesterEvents.savedCount.get() == 1 && memberEvents.savedCount.get() == 1,
            "Federation Saved after all unconstrained federates complete");
         JavaTckSupport.check(requesterEvents.saveFailures.isEmpty()
               && memberEvents.saveFailures.isEmpty(),
            "the untimed save unexpectedly failed for a joined federate");
      } finally {
         try { member.resignFederationExecution(ResignAction.NO_ACTION); }
         catch (Exception ignored) { }
         try { requester.resignFederationExecution(ResignAction.NO_ACTION); }
         catch (Exception ignored) { }
         if (created) {
            try { requester.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         try { member.disconnect(); }
         catch (Exception ignored) { }
         try { requester.disconnect(); }
         catch (Exception ignored) { }
      }
   }

   private static void await(CallbackModel callbackModel, RTIambassador requester,
         RTIambassador member, Condition condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            requester.evokeMultipleCallbacks(0.0, 0.0);
            member.evokeMultipleCallbacks(0.0, 0.0);
         }
         if (condition.isMet()) return;
         Thread.yield();
      }
      throw new AssertionError(description + " did not arrive before the deadline");
   }

   @FunctionalInterface
   private interface Condition { boolean isMet(); }

   private static final class Recorder {
      private final List<String> saveInitiations = new CopyOnWriteArrayList<>();
      private final List<String> saveFailures = new CopyOnWriteArrayList<>();
      private final AtomicInteger savedCount = new AtomicInteger();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("initiateFederateSave".equals(name)
                     && arguments != null && arguments.length > 0) {
                  saveInitiations.add((String) arguments[0]);
               } else if ("federationSaved".equals(name)) {
                  savedCount.incrementAndGet();
               } else if ("federationNotSaved".equals(name)) {
                  saveFailures.add(arguments == null || arguments.length == 0
                     ? "save failed" : String.valueOf(arguments[0]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK save-initiation callbacks";
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
