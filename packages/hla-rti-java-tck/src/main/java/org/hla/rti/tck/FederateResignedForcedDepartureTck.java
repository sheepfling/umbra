package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.exceptions.FederateNotExecutionMember;
import java.lang.reflect.Proxy;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardOpenOption;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;

/** Mirrors the public Java callback checks for one adapter-triggered RTI resignation. */
final class FederateResignedForcedDepartureTck {
   private static final String FEDERATION = "process-execution";
   private static final String OWNER_NAME = "package-process-sender";
   private static final String DEPARTED_NAME = "package-process-receiver";
   private static final String FEDERATE_TYPE = "package-process-type";
   private static final long TIMEOUT_NANOS = TimeUnit.SECONDS.toNanos(10);

   private FederateResignedForcedDepartureTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String rtiAddress, Path coordinationDirectory, CallbackModel callbackModel)
         throws Exception {
      JavaTckSupport.check(rtiAddress != null && !rtiAddress.isEmpty(),
         "the forced-resignation case requires the current-process adapter address");
      JavaTckSupport.check(coordinationDirectory != null
            && Files.isDirectory(coordinationDirectory),
         "the forced-resignation coordination directory is unavailable");

      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador departed = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder departedRecorder = new Recorder();
      RtiConfiguration configuration = RtiConfiguration.createConfiguration()
         .withRtiAddress(rtiAddress);
      boolean ownerConnected = false, departedConnected = false;
      boolean ownerJoined = false, departedJoined = false;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel, configuration);
         ownerConnected = true;
         departed.connect(departedRecorder.proxy(), callbackModel, configuration);
         departedConnected = true;

         owner.createFederationExecution(FEDERATION, fom, timeImplementation);
         FederateHandle ownerHandle = owner.joinFederationExecution(
            OWNER_NAME, FEDERATE_TYPE, FEDERATION);
         ownerJoined = true;
         departed.joinFederationExecution(DEPARTED_NAME, FEDERATE_TYPE, FEDERATION);
         departedJoined = true;
         JavaTckSupport.check(ownerHandle != null,
            "the process-profile owner did not receive its federate handle");

         Files.writeString(
            coordinationDirectory.resolve("federate-resigned-ready.ok"),
            "ready\n",
            StandardCharsets.UTF_8,
            StandardOpenOption.CREATE,
            StandardOpenOption.TRUNCATE_EXISTING);
         awaitForcedResignation(departed, departedRecorder, callbackModel);
         JavaTckSupport.check(departedRecorder.federateResignedCount.get() == 1
               && departedRecorder.federateResignedReason.get() != null
               && !departedRecorder.federateResignedReason.get().isEmpty(),
            "the RTI-forced departure did not deliver exactly one Federate Resigned reason");
         JavaTckSupport.check(departedRecorder.connectionLostReason.get() == null,
            "the forced Federate Resigned callback was replaced by Connection Lost");

         boolean membershipRejected = false;
         try {
            departed.getFederateHandle(DEPARTED_NAME);
         } catch (FederateNotExecutionMember expected) {
            membershipRejected = true;
         }
         JavaTckSupport.check(membershipRejected,
            "membership lookup did not reject the forcibly resigned federate");
         departedJoined = false;

         departed.joinFederationExecution(DEPARTED_NAME, FEDERATE_TYPE, FEDERATION);
         departedJoined = true;
         departed.resignFederationExecution(ResignAction.NO_ACTION);
         departedJoined = false;
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            while (departed.evokeMultipleCallbacks(0.0, 0.0)) { }
         }
         JavaTckSupport.check(departedRecorder.federateResignedCount.get() == 1,
            "federate-initiated resignation generated a Federate Resigned callback");

         owner.resignFederationExecution(ResignAction.NO_ACTION);
         ownerJoined = false;
      } finally {
         if (departedJoined) {
            try { departed.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (departedConnected) {
            try { departed.disconnect(); } catch (Exception ignored) { }
         }
         if (ownerConnected) {
            try { owner.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void awaitForcedResignation(RTIambassador departed, Recorder recorder,
         CallbackModel callbackModel) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (recorder.federateResignedCount.get() == 0 && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            departed.evokeCallback(0.0);
         } else {
            Thread.sleep(1L);
         }
      }
      JavaTckSupport.check(recorder.federateResignedCount.get() > 0,
         "timed out waiting for the RTI-forced Federate Resigned callback");
   }

   private static final class Recorder {
      private final AtomicInteger federateResignedCount = new AtomicInteger();
      private final AtomicReference<String> federateResignedReason = new AtomicReference<>();
      private final AtomicReference<String> connectionLostReason = new AtomicReference<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("federateResigned".equals(name)
                     && arguments != null && arguments.length == 1) {
                  federateResignedReason.set((String) arguments[0]);
                  federateResignedCount.incrementAndGet();
               } else if ("connectionLost".equals(name)
                     && arguments != null && arguments.length == 1) {
                  connectionLostReason.set((String) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK forced-resignation callbacks";
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
