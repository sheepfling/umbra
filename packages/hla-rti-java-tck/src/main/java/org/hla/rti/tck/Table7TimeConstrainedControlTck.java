package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.function.BooleanSupplier;

/** Verifies the Table 7 MOM requests disable and enable time-constrained mode. */
final class Table7TimeConstrainedControlTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final String DISABLE_REQUEST =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAdisableTimeConstrained";
   private static final String ENABLE_REQUEST =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAenableTimeConstrained";

   private Table7TimeConstrainedControlTck() {
   }

   static void run(RtiFactory factory, String fom, String mim,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(fom != null && !fom.isEmpty()
            && mim != null && !mim.isEmpty()
            && timeImplementation != null && !timeImplementation.isEmpty(),
         "Table 7 time-constrained testing requires adapter FOM, MIM, and time configuration");

      RTIambassador member = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-table7-time-constrained-" + suffix;
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      List<InteractionClassHandle> published = new ArrayList<>();
      try {
         member.connect(recorder.proxy(), callbackModel);
         connected = true;
         member.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle memberHandle = member.joinFederationExecution(
            "java-tck-table7-time-constrained-" + suffix, federation);
         joined = true;
         JavaTckSupport.check(memberHandle != null,
            "time-constrained effect probe could not resolve its federate handle");

         InteractionClassHandle disableClass = member.getInteractionClassHandle(DISABLE_REQUEST);
         InteractionClassHandle enableClass = member.getInteractionClassHandle(ENABLE_REQUEST);
         ParameterHandle disableFederate =
            member.getParameterHandle(disableClass, "HLAfederate");
         ParameterHandle enableFederate =
            member.getParameterHandle(enableClass, "HLAfederate");
         JavaTckSupport.check(disableClass != null && enableClass != null
               && disableFederate != null && enableFederate != null,
            "standard MIM omitted time-constrained requests or HLAfederate");
         member.publishInteractionClass(disableClass);
         published.add(disableClass);
         member.publishInteractionClass(enableClass);
         published.add(enableClass);

         byte[] encodedFederate = new byte[memberHandle.encodedLength()];
         memberHandle.encode(encodedFederate, 0);
         int beforeInitialEnable = recorder.enabledCallbacks.get();
         member.enableTimeConstrained();
         await(member, callbackModel,
            () -> recorder.enabledCallbacks.get() == beforeInitialEnable + 1,
            "initial timeConstrainedEnabled callback");

         sendRequest(member, disableClass, disableFederate, encodedFederate);
         int beforeReenable = recorder.enabledCallbacks.get();
         member.enableTimeConstrained();
         await(member, callbackModel,
            () -> recorder.enabledCallbacks.get() == beforeReenable + 1,
            "timeConstrainedEnabled after HLAdisableTimeConstrained");
         member.disableTimeConstrained();

         int beforeMomEnable = recorder.enabledCallbacks.get();
         sendRequest(member, enableClass, enableFederate, encodedFederate);
         await(member, callbackModel,
            () -> recorder.enabledCallbacks.get() == beforeMomEnable + 1,
            "timeConstrainedEnabled after HLAenableTimeConstrained");
         member.disableTimeConstrained();
      } finally {
         if (joined) {
            try { member.disableTimeConstrained(); }
            catch (Exception ignored) { }
         }
         for (InteractionClassHandle request : published) {
            try { member.unpublishInteractionClass(request); }
            catch (Exception ignored) { }
         }
         if (joined) {
            try { member.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { member.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) {
            try { member.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static void sendRequest(RTIambassador member, InteractionClassHandle request,
         ParameterHandle federateParameter, byte[] encodedFederate) throws Exception {
      ParameterHandleValueMap parameters =
         member.getParameterHandleValueMapFactory().create(1);
      parameters.put(federateParameter, encodedFederate);
      member.sendInteraction(request, parameters, new byte[0]);
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
               if ("timeConstrainedEnabled".equals(name)) {
                  enabledCallbacks.incrementAndGet();
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Table 7 time-constrained recorder";
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
