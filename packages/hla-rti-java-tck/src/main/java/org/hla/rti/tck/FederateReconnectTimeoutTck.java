package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.AdditionalSettingsResultCode;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.UUID;
import java.util.concurrent.atomic.AtomicReference;

/** Checks the public connection-loss outcome after the configured reconnect window. */
final class FederateReconnectTimeoutTck {
   private static final String EXPECTED_ADDITIONAL_SETTINGS =
      "FED_INT_HEART=0.02;FED_TIMEOUT_RECONNECT=0.25";

   private FederateReconnectTimeoutTck() { }

   static void run(RtiFactory factory, String fom, String mim,
         String timeImplementation, CallbackModel callbackModel,
         RtiConfiguration configuration, String additionalSettings,
         String readyMarkerPath, String serverStoppedMarkerPath,
         String callbackMarkerPath, long timeoutMillis) throws Exception {
      JavaTckSupport.check(EXPECTED_ADDITIONAL_SETTINGS.equals(additionalSettings),
         "adapter did not provide the expected provider-specific reconnect settings");
      JavaTckSupport.check(readyMarkerPath != null && !readyMarkerPath.isEmpty(),
         "reconnect-timeout outage-ready marker is required");
      JavaTckSupport.check(serverStoppedMarkerPath != null
            && !serverStoppedMarkerPath.isEmpty(),
         "reconnect-timeout server-stopped marker is required");
      JavaTckSupport.check(callbackMarkerPath != null && !callbackMarkerPath.isEmpty(),
         "reconnect-timeout callback marker is required");
      JavaTckSupport.check(timeoutMillis > 0,
         "reconnect-timeout callback wait must be positive");

      RTIambassador ambassador = factory.getRtiAmbassador();
      AtomicReference<String> connectionLoss = new AtomicReference<>();
      String federation = "java-tck-federate-reconnect-timeout-" + UUID.randomUUID();
      String federateName = "java-tck-reconnect-timeout-member-" + UUID.randomUUID();
      boolean connected = false;
      try {
         var connection = ambassador.connect(
            callbacks(connectionLoss), callbackModel, configuration);
         connected = true;
         JavaTckSupport.check(connection.additionalSettingsResultCode
               == AdditionalSettingsResultCode.SETTINGS_APPLIED,
            "connect did not apply the provider-specific reconnect timeout");
         ambassador.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         ambassador.joinFederationExecution(federateName, "java-tck-federate", federation);

         writeMarker(Path.of(readyMarkerPath), "ready\n");
         long outageDeadline = System.nanoTime() + timeoutMillis * 1_000_000L;
         waitForMarker(serverStoppedMarkerPath, "stopped",
            "the adapter's confirmed connection-loss outage", outageDeadline);
         JavaTckSupport.check(connectionLoss.get() == null,
            "connectionLost was reported before the adapter confirmed the outage");

         long reconnectStarted = System.nanoTime();
         long deadline = reconnectStarted + timeoutMillis * 1_000_000L;
         while (connectionLoss.get() == null && System.nanoTime() < deadline) {
            if (callbackModel == CallbackModel.HLA_EVOKED) {
               try {
                  ambassador.evokeMultipleCallbacks(0.0, 0.05);
               } catch (Exception ignored) {
                  // The provider may observe the outage before queueing connectionLost.
               }
            }
            if (connectionLoss.get() == null) Thread.sleep(1L);
         }
         String description = connectionLoss.get();
         JavaTckSupport.check(description != null && !description.isEmpty(),
            "timed out waiting for a non-empty connectionLost callback");

         long elapsedMillis = (System.nanoTime() - reconnectStarted) / 1_000_000L;
         JavaTckSupport.check(elapsedMillis >= 200L,
            "connectionLost arrived before the configured reconnect window elapsed");
         JavaTckSupport.check(elapsedMillis < 2_000L,
            "connectionLost arrived after the configured reconnect window");
         writeMarker(Path.of(callbackMarkerPath), "callback-ok\n");
         JavaTckSupport.expectFailure(() -> ambassador.listFederationExecutions(),
            "NotConnected",
            "public service after FED_TIMEOUT_RECONNECT expires");
      } finally {
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
            // The fixture publishes the marker only after stopping the host.
         }
         Thread.sleep(1L);
      }
      JavaTckSupport.check(false, "timed out waiting for " + description);
   }

   private static FederateAmbassador callbacks(AtomicReference<String> connectionLoss) {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if ("connectionLost".equals(method.getName())
                  && arguments != null && arguments.length == 1) {
               connectionLoss.compareAndSet(null, (String) arguments[0]);
            }
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) {
                  return "Java TCK reconnect-timeout callbacks";
               }
               if ("hashCode".equals(method.getName())) {
                  return System.identityHashCode(proxy);
               }
               if ("equals".equals(method.getName())) {
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
}
