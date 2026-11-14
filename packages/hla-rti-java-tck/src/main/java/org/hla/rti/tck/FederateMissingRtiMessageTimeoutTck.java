package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.AdditionalSettingsResultCode;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederationExecutionInformation;
import hla.rti1516_2025.FederationExecutionInformationSet;
import hla.rti1516_2025.FederateHandle;
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

/** Checks recovery after RTI heartbeat replies stop during an idle evoked session. */
final class FederateMissingRtiMessageTimeoutTck {
   private static final String EXPECTED_ADDITIONAL_SETTINGS =
      "FED_INT_HEART=0.05;FED_TIMEOUT_HEART=0.4;FED_TIMEOUT_RECONNECT=4";

   private FederateMissingRtiMessageTimeoutTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel, RtiConfiguration configuration,
         String additionalSettings, String readyMarkerPath,
         String suppressedMarkerPath, String resumedMarkerPath,
         long timeoutMillis) throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "missing-RTI-message timeout coverage requires an idle HLA_EVOKED federate");
      JavaTckSupport.check(EXPECTED_ADDITIONAL_SETTINGS.equals(additionalSettings),
         "adapter did not provide the expected provider-specific missing-message settings");
      JavaTckSupport.check(readyMarkerPath != null && !readyMarkerPath.isEmpty(),
         "missing-message timeout readiness marker is required");
      JavaTckSupport.check(suppressedMarkerPath != null && !suppressedMarkerPath.isEmpty(),
         "missing-message timeout suppressed-reply marker is required");
      JavaTckSupport.check(resumedMarkerPath != null && !resumedMarkerPath.isEmpty(),
         "missing-message timeout resumed-session marker is required");
      JavaTckSupport.check(timeoutMillis > 0,
         "missing-message timeout marker wait must be positive");

      RTIambassador ambassador = factory.getRtiAmbassador();
      AtomicBoolean connectionLost = new AtomicBoolean();
      AtomicBoolean federationListed = new AtomicBoolean();
      String federation = "java-tck-missing-rti-message-timeout-" + UUID.randomUUID();
      String federateName = "java-tck-missing-rti-message-timeout-member-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         var connection = ambassador.connect(callbacks(connectionLost, federationListed,
            federation, timeImplementation), callbackModel, configuration);
         connected = true;
         JavaTckSupport.check(connection.additionalSettingsResultCode
               == AdditionalSettingsResultCode.SETTINGS_APPLIED,
            "connect did not apply the provider-specific missing-message timeout");
         ambassador.createFederationExecution(
            federation, new String[] {fom}, timeImplementation);
         created = true;
         ambassador.joinFederationExecution(
            federateName, "java-tck-federate", federation);
         joined = true;
         FederateHandle memberHandle = ambassador.getFederateHandle(federateName);

         writeMarker(Path.of(readyMarkerPath), "ready\n");
         long deadline = System.nanoTime() + timeoutMillis * 1_000_000L;
         waitForMarker(suppressedMarkerPath, "suppressed",
            "the adapter to suppress an RTI heartbeat reply", deadline);
         long firstReplySuppressed = System.nanoTime();
         waitForMarker(resumedMarkerPath, "resumed",
            "the configured missing-message timeout to recover the session", deadline);
         long recoveryElapsedMillis =
            (System.nanoTime() - firstReplySuppressed) / 1_000_000L;
         JavaTckSupport.check(recoveryElapsedMillis >= 250L,
            "the RTI resumed before the configured missing-message timeout could expire ("
               + recoveryElapsedMillis + " ms)");
         JavaTckSupport.check(recoveryElapsedMillis < 3_500L,
            "the RTI did not resume within the configured reconnect window");
         JavaTckSupport.check(!connectionLost.get(),
            "a recoverable missing-RTI-message timeout produced connectionLost");

         JavaTckSupport.check(memberHandle.equals(
               ambassador.getFederateHandle(federateName)),
            "the federate identity changed across missing-message timeout recovery");
         ambassador.listFederationExecutions();
         long serviceDeadline = System.nanoTime() + 1_000_000_000L;
         while (!federationListed.get() && System.nanoTime() < serviceDeadline) {
            ambassador.evokeMultipleCallbacks(0.0, 0.05);
         }
         JavaTckSupport.check(federationListed.get(),
            "the public federation-list service did not work after missing-message recovery");
         JavaTckSupport.check(!connectionLost.get(),
            "the recovered public session reported connectionLost");

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
         String description, long deadline) throws Exception {
      Path marker = Path.of(markerPath);
      while (System.nanoTime() < deadline) {
         try {
            if (expected.equals(Files.readString(marker, StandardCharsets.UTF_8).trim())) {
               return;
            }
         } catch (java.io.IOException ignored) {
            // The fixture publishes each marker only after the matching state change.
         }
         Thread.sleep(1L);
      }
      JavaTckSupport.check(false, "timed out waiting for " + description);
   }

   private static void writeMarker(Path path, String contents) throws Exception {
      Path parent = path.toAbsolutePath().getParent();
      if (parent != null) Files.createDirectories(parent);
      Files.write(path, contents.getBytes(StandardCharsets.UTF_8));
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
                  return "Java TCK missing-RTI-message timeout callbacks";
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
}
