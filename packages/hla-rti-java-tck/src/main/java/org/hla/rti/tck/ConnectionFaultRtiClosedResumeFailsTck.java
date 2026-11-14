package org.hla.rti.tck;

import hla.rti1516_2025.AdditionalSettingsResultCode;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.ConfigurationResult;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.UUID;
import java.util.concurrent.atomic.AtomicReference;

/** Verifies public connection-loss reporting when RTI-side resume status is absent. */
final class ConnectionFaultRtiClosedResumeFailsTck {
   private static final String REQUIRED_SETTINGS =
      "FED_INT_HEART=0.02;FED_TIMEOUT_RECONNECT=0.25";

   private ConnectionFaultRtiClosedResumeFailsTck() { }

   static void run(RtiFactory factory, String fom, String mim,
         String timeImplementation, CallbackModel callbackModel,
         RtiConfiguration ownerConfiguration, RtiConfiguration recoveringConfiguration,
         String additionalSettings, String triggerMarkerPath, String completionMarkerPath,
         long timeoutMillis) throws Exception {
      JavaTckSupport.check(REQUIRED_SETTINGS.equals(additionalSettings),
         "RTI-closed resume failure requires the accelerated heartbeat settings");
      JavaTckSupport.check(triggerMarkerPath != null && !triggerMarkerPath.isEmpty()
            && completionMarkerPath != null && !completionMarkerPath.isEmpty(),
         "RTI-closed resume failure requires adapter marker paths");
      JavaTckSupport.check(timeoutMillis > 0, "connection-loss timeout must be positive");

      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador recovering = factory.getRtiAmbassador();
      AtomicReference<String> connectionLostDescription = new AtomicReference<>();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-rti-closed-resume-fails-" + suffix;
      String ownerName = "java-tck-resume-owner-" + suffix;
      String recoveringName = "java-tck-resume-member-" + suffix;
      boolean ownerConnected = false;
      boolean recoveringConnected = false;
      boolean ownerJoined = false;
      boolean recoveringJoined = false;
      boolean recoveryFailed = false;
      try {
         ConfigurationResult ownerResult = owner.connect(callbacks(null), callbackModel,
            ownerConfiguration);
         ownerConnected = true;
         checkConnection(ownerResult, "owner");
         owner.createFederationExecutionWithMIM(federation, new String[] {fom}, mim,
            timeImplementation);
         owner.joinFederationExecution(ownerName, "java-tck", federation);
         ownerJoined = true;

         ConfigurationResult recoveringResult = recovering.connect(
            callbacks(connectionLostDescription), callbackModel, recoveringConfiguration);
         recoveringConnected = true;
         checkConnection(recoveringResult, "recovering member");
         recovering.joinFederationExecution(recoveringName, "java-tck", federation);
         recoveringJoined = true;
         writeMarker(triggerMarkerPath, "invalidate-next-data-frame\n");

         awaitConnectionLost(recovering, recoveringName, callbackModel,
            connectionLostDescription, timeoutMillis);
         recoveryFailed = true;
         String description = connectionLostDescription.get();
         JavaTckSupport.check(description != null && !description.trim().isEmpty(),
            "ConnectionLost omitted its fault description");
         JavaTckSupport.expectFailure(recovering::listFederationExecutions, "NotConnected",
            "public service after the RTI closes the joined-federate socket");

         owner.resignFederationExecution(ResignAction.NO_ACTION);
         ownerJoined = false;
         owner.disconnect();
         ownerConnected = false;
         writeMarker(completionMarkerPath, "resume-failed\n");
      } finally {
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (ownerConnected) {
            try { owner.disconnect(); }
            catch (Exception ignored) { }
         }
         if (!recoveryFailed) {
            if (recoveringJoined) {
               try { recovering.resignFederationExecution(ResignAction.NO_ACTION); }
               catch (Exception ignored) { }
            }
            if (recoveringConnected) {
               try { recovering.disconnect(); }
               catch (Exception ignored) { }
            }
         }
      }
   }

   private static void checkConnection(ConfigurationResult result, String role) {
      JavaTckSupport.check(result != null && result.addressUsed,
         role + " Connect did not use its adapter-supplied address");
      JavaTckSupport.check(result.additionalSettingsResultCode
            == AdditionalSettingsResultCode.SETTINGS_APPLIED,
         role + " Connect did not apply the accelerated heartbeat settings");
   }

   private static void awaitConnectionLost(RTIambassador recovering, String recoveringName,
         CallbackModel callbackModel, AtomicReference<String> description,
         long timeoutMillis) throws Exception {
      long deadline = System.nanoTime() + timeoutMillis * 1_000_000L;
      while (System.nanoTime() < deadline && description.get() == null) {
         try {
            if (callbackModel == CallbackModel.HLA_EVOKED) {
               recovering.evokeMultipleCallbacks(0.0, 0.05);
            } else {
               recovering.getFederateHandle(recoveringName);
            }
         } catch (Exception ignored) {
            // An immediate service may report the expired recovery window before
            // the asynchronous ConnectionLost callback is delivered.
         }
         if (description.get() == null) Thread.sleep(1L);
      }
      JavaTckSupport.check(description.get() != null,
         "timed out waiting for ConnectionLost after RTI-closed session resume failure");
   }

   private static FederateAmbassador callbacks(AtomicReference<String> connectionLost) {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            String name = method.getName();
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(name)) return "Java TCK resume-failure callbacks";
               if ("hashCode".equals(name)) return System.identityHashCode(proxy);
               if ("equals".equals(name)) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            if (connectionLost != null && "connectionLost".equals(name)
                  && arguments != null && arguments.length > 0) {
               connectionLost.compareAndSet(null, String.valueOf(arguments[0]));
            }
            return null;
         });
   }

   private static void writeMarker(String path, String contents) throws Exception {
      Path marker = Path.of(path);
      Files.createDirectories(marker.toAbsolutePath().getParent());
      Files.write(marker, contents.getBytes(StandardCharsets.UTF_8));
   }
}
