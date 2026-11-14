package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.ConfigurationResult;
import hla.rti1516_2025.AdditionalSettingsResultCode;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederationExecutionInformation;
import hla.rti1516_2025.FederationExecutionInformationSet;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.List;

/** Exercises the Java public Connect and List Federation Executions services over WebSocket. */
final class WebSocketTransportSupportTck {
   private static final String ABSENT_FEDERATION =
      "__java_tck_websocket_absent_federation__";

   private WebSocketTransportSupportTck() { }

   static void run(RtiFactory factory, CallbackModel callbackModel, String rtiAddress,
         long timeoutMillis) throws Exception {
      run(factory, callbackModel, rtiAddress, null, "ws://", "WebSocket",
         timeoutMillis);
   }

   static void runSecure(RtiFactory factory, CallbackModel callbackModel, String rtiAddress,
         String additionalSettings, long timeoutMillis) throws Exception {
      JavaTckSupport.check(additionalSettings != null && !additionalSettings.trim().isEmpty(),
         "Secure WebSocket coverage requires adapter-supplied TLS trust settings");
      run(factory, callbackModel, rtiAddress, additionalSettings, "wss://",
         "Secure WebSocket", timeoutMillis);
   }

   private static void run(RtiFactory factory, CallbackModel callbackModel, String rtiAddress,
         String additionalSettings, String addressScheme, String transportName,
         long timeoutMillis) throws Exception {
      JavaTckSupport.check(rtiAddress != null && rtiAddress.startsWith(addressScheme),
         transportName + " coverage requires an adapter-supplied " + addressScheme
            + " RTI address");
      JavaTckSupport.check(timeoutMillis > 0, "WebSocket callback timeout must be positive");

      RTIambassador ambassador = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      boolean connected = false;
      try {
         RtiConfiguration configuration = RtiConfiguration.createConfiguration()
            .withRtiAddress(rtiAddress);
         if (additionalSettings != null) {
            configuration = configuration.withAdditionalSettings(additionalSettings);
         }
         ConfigurationResult result = ambassador.connect(recorder.proxy(), callbackModel,
            configuration);
         connected = true;
         JavaTckSupport.check(result != null && result.addressUsed,
            "Connect did not report use of the adapter-supplied " + transportName
               + " RTI address");
         if (additionalSettings != null) {
            JavaTckSupport.check(result.additionalSettingsResultCode
                  == AdditionalSettingsResultCode.SETTINGS_APPLIED,
               "Connect did not report the adapter-supplied TLS trust settings as applied");
         }

         ambassador.listFederationExecutions();
         awaitFederationReport(ambassador, recorder, callbackModel, timeoutMillis);
         JavaTckSupport.check(recorder.omitsFederation(ABSENT_FEDERATION),
            transportName + " federation-execution report unexpectedly listed the absent federation");
      } finally {
         if (connected) {
            try { ambassador.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static void awaitFederationReport(RTIambassador ambassador, Recorder recorder,
         CallbackModel callbackModel, long timeoutMillis)
         throws Exception {
      long deadline = System.nanoTime() + timeoutMillis * 1_000_000L;
      while (!recorder.hasReport() && System.nanoTime() < deadline) {
         try {
            if (callbackModel == CallbackModel.HLA_EVOKED) {
               ambassador.evokeMultipleCallbacks(0.0, 0.01);
            } else {
               Thread.sleep(1L);
            }
         } catch (Exception ignored) {
            // The callback model may deliver the report while this service is in flight.
         }
         if (!recorder.hasReport()) Thread.sleep(1L);
      }
      JavaTckSupport.check(recorder.hasReport(),
         "timed out waiting for the WebSocket federation-execution report");
   }

   private static final class Recorder {
      private final List<FederationExecutionInformationSet> reports = new ArrayList<>();

      private synchronized boolean hasReport() {
         return !reports.isEmpty();
      }

      private synchronized boolean omitsFederation(String name) {
         if (reports.isEmpty()) return false;
         for (FederationExecutionInformationSet report : reports) {
            for (FederationExecutionInformation information : report) {
               if (name.equals(information.federationExecutionName)) return false;
            }
         }
         return true;
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("reportFederationExecutions".equals(method.getName())
                     && arguments != null && arguments.length == 1) {
                  synchronized (this) {
                     reports.add(
                        ((FederationExecutionInformationSet) arguments[0]).clone());
                  }
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(method.getName())) {
                     return "Java TCK WebSocket transport callbacks";
                  }
                  if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
                  if ("equals".equals(method.getName())) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }
   }
}
