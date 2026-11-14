package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.UUID;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.function.BooleanSupplier;

/** Verifies the Table 7 modify-lookahead request changes the subject lookahead. */
final class Table7ModifyLookaheadTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final String REQUEST =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAmodifyLookahead";

   private Table7ModifyLookaheadTck() {
   }

   static void run(RtiFactory factory, String fom, String mim,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(fom != null && !fom.isEmpty()
            && mim != null && !mim.isEmpty()
            && timeImplementation != null && !timeImplementation.isEmpty(),
         "Table 7 modify-lookahead testing requires adapter FOM, MIM, and time configuration");

      RTIambassador subject = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-table7-modify-lookahead-" + suffix;
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      boolean regulating = false;
      boolean published = false;
      InteractionClassHandle requestClass = null;
      try {
         subject.connect(recorder.proxy(), callbackModel);
         connected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         subject.joinFederationExecution("java-tck-table7-modify-lookahead-" + suffix,
            federation);
         joined = true;

         requestClass = subject.getInteractionClassHandle(REQUEST);
         ParameterHandle lookaheadParameter =
            subject.getParameterHandle(requestClass, "HLAlookahead");
         JavaTckSupport.check(requestClass != null && lookaheadParameter != null,
            "standard MIM omitted HLAmodifyLookahead or its HLAlookahead parameter");
         subject.publishInteractionClass(requestClass);
         published = true;

         LogicalTimeInterval epsilon =
            (LogicalTimeInterval) subject.getTimeFactory().makeEpsilon();
         JavaTckSupport.check(epsilon != null,
            "modify-lookahead probe could not allocate the adapter epsilon interval");
         int callbacksBeforeEnable = recorder.enabledCallbacks.get();
         subject.enableTimeRegulation(epsilon);
         regulating = true;
         await(subject, callbackModel,
            () -> recorder.enabledCallbacks.get() >= callbacksBeforeEnable + 1,
            "timeRegulationEnabled callback before HLAmodifyLookahead");
         JavaTckSupport.check(recorder.enabledCallbacks.get() == callbacksBeforeEnable + 1,
            "initial time regulation delivered an unexpected number of callbacks");

         LogicalTimeInterval requestedLookahead =
            (LogicalTimeInterval) subject.getTimeFactory().makeEpsilon();
         requestedLookahead = (LogicalTimeInterval) requestedLookahead.add(epsilon);
         byte[] encodedLookahead = new byte[requestedLookahead.encodedLength()];
         requestedLookahead.encode(encodedLookahead, 0);
         ParameterHandleValueMap parameters =
            subject.getParameterHandleValueMapFactory().create(1);
         parameters.put(lookaheadParameter, encodedLookahead);
         subject.sendInteraction(requestClass, parameters, new byte[0]);

         JavaTckSupport.check(requestedLookahead.equals(subject.queryLookahead()),
            "HLAmodifyLookahead did not apply the requested HLAlookahead interval");
         subject.disableTimeRegulation();
         regulating = false;
      } finally {
         if (regulating) {
            try { subject.disableTimeRegulation(); }
            catch (Exception ignored) { }
         }
         if (published && requestClass != null) {
            try { subject.unpublishInteractionClass(requestClass); }
            catch (Exception ignored) { }
         }
         if (joined) {
            try { subject.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) {
            try { subject.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static void await(RTIambassador member, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean()) {
         if (System.nanoTime() >= deadline) {
            throw new AssertionError("timed out waiting for " + description);
         }
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            member.evokeCallback(0.01);
         } else {
            Thread.sleep(1L);
         }
      }
   }

   private static final class Recorder {
      private final AtomicInteger enabledCallbacks = new AtomicInteger();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  enabledCallbacks.incrementAndGet();
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Table 7 modify-lookahead recorder";
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
