package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.AdditionalSettingsResultCode;
import hla.rti1516_2025.FederateAmbassador;
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

/** Checks that the RTI sends heartbeats while an evoked federate is idle. */
final class FederateIdleHeartbeatTck {
   private static final String EXPECTED_ADDITIONAL_SETTINGS =
      "FED_INT_HEART=0.1;FED_TIMEOUT_HEART=2;FED_TIMEOUT_RECONNECT=0.25";
   private static final long IDLE_WINDOW_MILLIS = 1_500L;

   private FederateIdleHeartbeatTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel, RtiConfiguration configuration,
         String additionalSettings, String readyMarkerPath,
         String observedMarkerPath, String inactiveMarkerPath,
         long markerTimeoutMillis) throws Exception {
      JavaTckSupport.check(EXPECTED_ADDITIONAL_SETTINGS.equals(additionalSettings),
         "adapter did not provide the expected provider-specific idle-heartbeat settings");
      JavaTckSupport.check(readyMarkerPath != null && !readyMarkerPath.isEmpty(),
         "idle-heartbeat readiness marker is required");
      JavaTckSupport.check(observedMarkerPath != null && !observedMarkerPath.isEmpty(),
         "idle-heartbeat observation marker is required");
      JavaTckSupport.check(inactiveMarkerPath != null && !inactiveMarkerPath.isEmpty(),
         "idle-heartbeat inactivity marker is required");
      JavaTckSupport.check(markerTimeoutMillis > 0,
         "idle-heartbeat marker wait must be positive");

      RTIambassador ambassador = factory.getRtiAmbassador();
      AtomicBoolean connectionLost = new AtomicBoolean();
      AtomicBoolean federationListed = new AtomicBoolean();
      String federation = "java-tck-federate-idle-heartbeat-" + UUID.randomUUID();
      String federateName = "java-tck-idle-heartbeat-member-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         var connection = ambassador.connect(callbacks(connectionLost, federationListed,
            federation, timeImplementation), callbackModel, configuration);
         connected = true;
         JavaTckSupport.check(connection.additionalSettingsResultCode
               == AdditionalSettingsResultCode.SETTINGS_APPLIED,
            "connect did not apply the provider-specific idle-heartbeat interval");
         ambassador.createFederationExecution(
            federation, new String[] {fom}, timeImplementation);
         created = true;
         ambassador.joinFederationExecution(
            federateName, "java-tck-federate", federation);
         joined = true;

         long idleStarted = System.nanoTime();
         long observationDeadline = idleStarted + IDLE_WINDOW_MILLIS * 1_000_000L;
         writeMarker(Path.of(readyMarkerPath), "ready\n");
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            waitForMarker(observedMarkerPath, "heartbeat",
               "a heartbeat from the otherwise-idle evoked federate",
               observationDeadline);
         }

         while (System.nanoTime() < observationDeadline) {
            if (callbackModel == CallbackModel.HLA_IMMEDIATE) {
               JavaTckSupport.check(!connectionLost.get(),
                  "the idle federate lost its public RTI connection during the heartbeat window");
            }
            Thread.sleep(1L);
         }
         JavaTckSupport.check(!Files.exists(Path.of(inactiveMarkerPath)),
            "the idle federate exceeded the RTI heartbeat timeout despite the configured interval");
         JavaTckSupport.check(!connectionLost.get(),
            "the idle federate lost its public RTI connection during the heartbeat window");

         ambassador.listFederationExecutions();
         long serviceDeadline = System.nanoTime() + 1_000_000_000L;
         while (!federationListed.get() && System.nanoTime() < serviceDeadline) {
            if (callbackModel == CallbackModel.HLA_EVOKED) {
               ambassador.evokeMultipleCallbacks(0.0, 0.05);
            } else {
               Thread.sleep(1L);
            }
         }
         JavaTckSupport.check(federationListed.get(),
            "the public federation-list service did not succeed after the idle heartbeat interval");
         JavaTckSupport.check(!connectionLost.get(),
            "the public federate session was lost after the idle heartbeat interval");

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
            // The fixture publishes the marker only after observing the frame.
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
                  return "Java TCK idle-heartbeat callbacks";
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
