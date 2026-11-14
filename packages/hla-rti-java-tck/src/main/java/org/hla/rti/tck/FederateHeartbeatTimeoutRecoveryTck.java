package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.AdditionalSettingsResultCode;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.FederationExecutionInformation;
import hla.rti1516_2025.FederationExecutionInformationSet;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.UUID;
import java.util.concurrent.atomic.AtomicBoolean;

/** Checks identity and public service recovery across a provider-specific heartbeat timeout. */
final class FederateHeartbeatTimeoutRecoveryTck {
   private static final String EXPECTED_ADDITIONAL_SETTINGS =
      "FED_INT_HEART=2;FED_TIMEOUT_HEART=2.5;FED_TIMEOUT_RECONNECT=4";

   private FederateHeartbeatTimeoutRecoveryTck() { }

   static void run(RtiFactory factory, String fom, String mim,
         String timeImplementation, CallbackModel callbackModel,
         RtiConfiguration configuration, String additionalSettings,
         String timeoutReadyMarkerPath, String inactiveMarkerPath,
         String resumedMarkerPath, long timeoutMillis) throws Exception {
      JavaTckSupport.check(EXPECTED_ADDITIONAL_SETTINGS.equals(additionalSettings),
         "adapter did not provide the expected provider-specific heartbeat settings");
      JavaTckSupport.check(timeoutReadyMarkerPath != null
            && !timeoutReadyMarkerPath.isEmpty(),
         "heartbeat-recovery timeout-ready marker is required");
      JavaTckSupport.check(inactiveMarkerPath != null && !inactiveMarkerPath.isEmpty(),
         "heartbeat-recovery inactive-session marker is required");
      JavaTckSupport.check(resumedMarkerPath != null && !resumedMarkerPath.isEmpty(),
         "heartbeat-recovery resumed marker is required");
      JavaTckSupport.check(timeoutMillis > 0,
         "heartbeat-recovery marker wait must be positive");

      RTIambassador ambassador = factory.getRtiAmbassador();
      AtomicBoolean connectionLost = new AtomicBoolean();
      AtomicBoolean federationListed = new AtomicBoolean();
      String federation = "java-tck-federate-heartbeat-recovery-" + UUID.randomUUID();
      String federateName = "java-tck-heartbeat-recovery-member-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         var connection = ambassador.connect(callbacks(connectionLost, federationListed,
            federation, timeImplementation), callbackModel, configuration);
         connected = true;
         JavaTckSupport.check(connection.additionalSettingsResultCode
               == AdditionalSettingsResultCode.SETTINGS_APPLIED,
            "connect did not apply the provider-specific heartbeat timeout settings");
         ambassador.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         ambassador.joinFederationExecution(
            federateName, "java-tck-federate", federation);
         joined = true;
         FederateHandle joinedHandle = ambassador.getFederateHandle(federateName);

         writeMarker(Path.of(timeoutReadyMarkerPath), "ready\n");
         long timeoutStarted = System.nanoTime();
         long recoveryDeadline = System.nanoTime() + timeoutMillis * 1_000_000L;
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            waitForMarker(inactiveMarkerPath, "inactive", "RTI inactive-session timeout",
               recoveryDeadline, connectionLost);
            long timeoutElapsedMillis = (System.nanoTime() - timeoutStarted) / 1_000_000L;
            JavaTckSupport.check(timeoutElapsedMillis >= 400L,
               "the RTI declared the session inactive before FED_TIMEOUT_HEART could expire ("
                  + timeoutElapsedMillis + " ms)");
            waitForMarker(resumedMarkerPath, "resumed", "server session resumption",
               recoveryDeadline, connectionLost);
         } else {
            long activeObservationDeadline = System.nanoTime() + 1_500_000_000L;
            while (System.nanoTime() < activeObservationDeadline) {
               JavaTckSupport.check(!connectionLost.get(),
                  "an active immediate-callback session reported connectionLost");
               Thread.sleep(1L);
            }
            JavaTckSupport.check(!"inactive".equals(readMarker(inactiveMarkerPath)),
               "the RTI expired an immediate-callback session that was still polling for callbacks");
         }
         JavaTckSupport.check(!connectionLost.get(),
            "the recoverable heartbeat interruption produced connectionLost");

         long serviceDeadline = System.nanoTime() + 3_000_000_000L;
         while (System.nanoTime() < serviceDeadline && !federationListed.get()
               && !connectionLost.get()) {
            try {
               ambassador.listFederationExecutions();
               if (callbackModel == CallbackModel.HLA_EVOKED) {
                  ambassador.evokeMultipleCallbacks(0.0, 0.05);
               }
               JavaTckSupport.check(!connectionLost.get(),
                  "the public service recovery attempt reported connectionLost");
            } catch (Exception ignored) {
               Thread.sleep(10L);
            }
         }
         JavaTckSupport.check(federationListed.get(),
            "listFederationExecutions did not report the federation after heartbeat recovery");
         JavaTckSupport.check(!connectionLost.get(),
            "the public service recovery attempt reported connectionLost");
         JavaTckSupport.check(joinedHandle.equals(ambassador.getFederateHandle(federateName)),
            "the RTI did not preserve joined federate identity across the silent-session timeout and reconnect");

         ambassador.resignFederationExecution(ResignAction.NO_ACTION);
         joined = false;
         ambassador.destroyFederationExecution(federation);
         created = false;
         ambassador.disconnect();
         connected = false;
      } finally {
         if (joined) {
            try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { ambassador.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) {
            try { ambassador.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static void waitForMarker(String markerPath, String expected,
         String description, long deadline, AtomicBoolean connectionLost) throws Exception {
      Path marker = Path.of(markerPath);
      while (System.nanoTime() < deadline) {
         try {
            if (expected.equals(Files.readString(marker, StandardCharsets.UTF_8).trim())) {
               return;
            }
         } catch (java.io.IOException ignored) {
            // The fixture publishes each marker only after the matching state change.
         }
         JavaTckSupport.check(!connectionLost.get(),
            "connectionLost occurred before " + description);
         Thread.sleep(1L);
      }
      JavaTckSupport.check(false, "timed out waiting for " + description);
   }

   private static FederateAmbassador callbacks(AtomicBoolean connectionLost,
         AtomicBoolean federationListed, String federation, String timeImplementation) {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            String name = method.getName();
            if ("connectionLost".equals(name)) {
               connectionLost.set(true);
            } else if ("reportFederationExecutions".equals(name)
                  && arguments != null && arguments.length == 1) {
               FederationExecutionInformationSet reports =
                  (FederationExecutionInformationSet) arguments[0];
               for (FederationExecutionInformation report : reports) {
                  if (federation.equals(report.federationExecutionName)
                        && timeImplementation.equals(report.logicalTimeImplementationName)) {
                     federationListed.set(true);
                  }
               }
            }
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(name)) {
                  return "Java TCK heartbeat-recovery callbacks";
               }
               if ("hashCode".equals(name)) {
                  return System.identityHashCode(proxy);
               }
               if ("equals".equals(name)) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }

   private static void writeMarker(Path path, String contents) throws Exception {
      Path parent = path.toAbsolutePath().getParent();
      if (parent != null) Files.createDirectories(parent);
      Files.write(path, contents.getBytes(StandardCharsets.UTF_8));
   }

   private static String readMarker(String markerPath) throws Exception {
      Path marker = Path.of(markerPath);
      if (!Files.isRegularFile(marker)) return "";
      return Files.readString(marker, StandardCharsets.UTF_8).trim();
   }
}
