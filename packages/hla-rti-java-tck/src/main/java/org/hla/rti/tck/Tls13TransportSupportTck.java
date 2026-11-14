package org.hla.rti.tck;

import hla.rti1516_2025.AdditionalSettingsResultCode;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.ConfigurationResult;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederationExecutionInformation;
import hla.rti1516_2025.FederationExecutionInformationSet;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.exceptions.ConnectionFailed;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.List;

/** Exercises the Java public Connect and federation-query services over TLS. */
final class Tls13TransportSupportTck {
   private static final String ABSENT_FEDERATION = "__java_tck_tls_absent_federation__";
   private static final String DEFAULT_PORT_ABSENT_FEDERATION =
      "__java_tck_tls_default_port_absent_federation__";
   private static final String ALTERNATE_PORT_ABSENT_FEDERATION =
      "__java_tck_tls_alternate_port_absent_federation__";

   private Tls13TransportSupportTck() { }

   static void run(RtiFactory factory, CallbackModel callbackModel, String rtiAddress,
         String additionalSettings, long timeoutMillis) throws Exception {
      run(factory, callbackModel, rtiAddress, additionalSettings, timeoutMillis,
         ABSENT_FEDERATION);
   }

   static void run(RtiFactory factory, CallbackModel callbackModel, String rtiAddress,
         String additionalSettings, long timeoutMillis, String absentFederation)
         throws Exception {
      JavaTckSupport.check(rtiAddress != null && rtiAddress.startsWith("tls://"),
         "TLS transport coverage requires an adapter-supplied tls:// RTI address");
      JavaTckSupport.check(additionalSettings == null || !additionalSettings.trim().isEmpty(),
         "TLS coverage requires non-empty adapter-supplied mode settings when present");
      JavaTckSupport.check(timeoutMillis > 0, "TLS callback timeout must be positive");
      JavaTckSupport.check(absentFederation != null && !absentFederation.isEmpty(),
         "TLS federation query requires an absent-federation marker");

      RtiConfiguration configuration = RtiConfiguration.createConfiguration()
         .withRtiAddress(rtiAddress);
      if (additionalSettings != null) {
         configuration = configuration.withAdditionalSettings(additionalSettings);
      }

      RTIambassador ambassador = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      boolean connected = false;
      try {
         ConfigurationResult result = ambassador.connect(recorder.proxy(), callbackModel,
            configuration);
         connected = true;
         JavaTckSupport.check(result != null && result.addressUsed,
            "Connect did not report use of the adapter-supplied TLS RTI address");
         if (additionalSettings != null) {
            JavaTckSupport.check(result.additionalSettingsResultCode
                  == AdditionalSettingsResultCode.SETTINGS_APPLIED,
               "Connect did not report the adapter-supplied TLS trust settings as applied");
         }

         ambassador.listFederationExecutions();
         awaitFederationReport(ambassador, recorder, callbackModel, timeoutMillis);
         JavaTckSupport.check(recorder.omitsFederation(absentFederation),
            "TLS federation-execution report unexpectedly listed the absent federation");
      } finally {
         if (connected) {
            try { ambassador.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   static void runDefaultPort(RtiFactory factory, CallbackModel callbackModel,
         String rtiAddress, long timeoutMillis) throws Exception {
      JavaTckSupport.check(rtiAddress != null && rtiAddress.startsWith("tls://"),
         "TLS default-port coverage requires an adapter-supplied tls:// RTI address");
      JavaTckSupport.check(!hasExplicitPort(rtiAddress),
         "TLS default-port coverage requires an RTI address with no explicit port");
      run(factory, callbackModel, rtiAddress, null, timeoutMillis,
         DEFAULT_PORT_ABSENT_FEDERATION);
   }

   static void runAlternateAddressPort(RtiFactory factory, CallbackModel callbackModel,
         String rtiAddress, long timeoutMillis) throws Exception {
      int port = explicitNumericPort(rtiAddress);
      JavaTckSupport.check(port != 15165,
         "TLS alternate-address coverage requires a port other than the default 15165");
      run(factory, callbackModel, rtiAddress, null, timeoutMillis,
         ALTERNATE_PORT_ABSENT_FEDERATION);
   }

   private static int explicitNumericPort(String rtiAddress) {
      JavaTckSupport.check(rtiAddress != null && rtiAddress.startsWith("tls://"),
         "TLS alternate-address coverage requires an adapter-supplied tls:// RTI address");
      int authorityStart = "tls://".length();
      int authorityEnd = rtiAddress.length();
      for (char delimiter : new char[] {'/', '?', '#'}) {
         int delimiterIndex = rtiAddress.indexOf(delimiter, authorityStart);
         if (delimiterIndex >= 0) authorityEnd = Math.min(authorityEnd, delimiterIndex);
      }
      String authority = rtiAddress.substring(authorityStart, authorityEnd);
      int portSeparator;
      if (authority.startsWith("[")) {
         int closingBracket = authority.indexOf(']');
         JavaTckSupport.check(closingBracket >= 0,
            "TLS alternate RTI address has an unterminated IPv6 host");
         portSeparator = closingBracket + 1;
         JavaTckSupport.check(portSeparator < authority.length()
               && authority.charAt(portSeparator) == ':',
            "TLS alternate-address coverage requires an explicit port");
      } else {
         portSeparator = authority.lastIndexOf(':');
         JavaTckSupport.check(portSeparator >= 0,
            "TLS alternate-address coverage requires an explicit port");
      }
      String portText = authority.substring(portSeparator + 1);
      JavaTckSupport.check(!portText.isEmpty(),
         "TLS alternate-address coverage requires a valid numeric port");
      int port = 0;
      for (int index = 0; index < portText.length(); index++) {
         char character = portText.charAt(index);
         JavaTckSupport.check(character >= '0' && character <= '9',
            "TLS alternate-address coverage requires a numeric port");
         int digit = character - '0';
         JavaTckSupport.check(port <= (65535 - digit) / 10,
            "TLS alternate-address coverage requires a port in [1, 65535]");
         port = port * 10 + digit;
      }
      JavaTckSupport.check(port > 0,
         "TLS alternate-address coverage requires a port in [1, 65535]");
      return port;
   }

   private static boolean hasExplicitPort(String rtiAddress) {
      int authorityStart = "tls://".length();
      int authorityEnd = rtiAddress.length();
      for (char delimiter : new char[] {'/', '?', '#'}) {
         int delimiterIndex = rtiAddress.indexOf(delimiter, authorityStart);
         if (delimiterIndex >= 0) authorityEnd = Math.min(authorityEnd, delimiterIndex);
      }
      String authority = rtiAddress.substring(authorityStart, authorityEnd);
      if (authority.startsWith("[")) {
         int closingBracket = authority.indexOf(']');
         JavaTckSupport.check(closingBracket >= 0,
            "TLS default-port RTI address has an unterminated IPv6 host");
         return closingBracket + 1 < authority.length();
      }
      return authority.indexOf(':') >= 0;
   }

   static void expectUntrustedServerFailure(RtiFactory factory, CallbackModel callbackModel,
         String rtiAddress, String additionalSettings) throws Exception {
      JavaTckSupport.check(rtiAddress != null && rtiAddress.startsWith("tls://"),
         "TLS server-certificate rejection requires an adapter-supplied tls:// RTI address");
      JavaTckSupport.check(additionalSettings != null && !additionalSettings.trim().isEmpty(),
         "TLS server-certificate rejection requires adapter-supplied validation settings");

      RTIambassador ambassador = factory.getRtiAmbassador();
      RtiConfiguration configuration = RtiConfiguration.createConfiguration()
         .withRtiAddress(rtiAddress)
         .withAdditionalSettings(additionalSettings);
      boolean rejected = false;
      try {
         ambassador.connect(new Recorder().proxy(), callbackModel, configuration);
      } catch (ConnectionFailed expected) {
         rejected = true;
      }
      if (!rejected) {
         try { ambassador.disconnect(); }
         catch (Exception ignored) { }
      }
      JavaTckSupport.check(rejected,
         "Connect did not reject the untrusted TLS server certificate with ConnectionFailed");
   }

   private static void awaitFederationReport(RTIambassador ambassador, Recorder recorder,
         CallbackModel callbackModel, long timeoutMillis) throws Exception {
      long deadline = System.nanoTime() + timeoutMillis * 1_000_000L;
      while (!recorder.hasReport() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            ambassador.evokeMultipleCallbacks(0.0, 0.01);
         } else {
            Thread.sleep(1L);
         }
      }
      JavaTckSupport.check(recorder.hasReport(),
         "timed out waiting for the TLS federation-execution report");
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
                     return "Java TCK TLS transport callbacks";
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
