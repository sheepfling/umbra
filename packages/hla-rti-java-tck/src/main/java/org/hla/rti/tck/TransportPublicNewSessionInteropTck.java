package org.hla.rti.tck;

import hla.rti1516_2025.AdditionalSettingsResultCode;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.ConfigurationResult;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.RtiFactory;

/** Checks Java public connection configuration against the independent Table 33 peer. */
final class TransportPublicNewSessionInteropTck {
   private static final String HEARTBEAT_SETTINGS =
      "FED_INT_HEART=0.1;FED_TIMEOUT_HEART=2;FED_TIMEOUT_RECONNECT=0.25";

   private TransportPublicNewSessionInteropTck() { }

   static void run(RtiFactory factory, CallbackModel callbackModel, String peerAddress)
         throws Exception {
      JavaTckSupport.check(peerAddress != null
            && peerAddress.startsWith("tcp://127.0.0.1:"),
         "the Table 33 public-API case requires its independent loopback peer adapter");

      RTIambassador rti = factory.getRtiAmbassador();
      boolean connected = false;
      try {
         RtiConfiguration configuration = RtiConfiguration.createConfiguration()
            .withRtiAddress(peerAddress)
            .withAdditionalSettings(HEARTBEAT_SETTINGS);
         ConfigurationResult connection = rti.connect(
            new NullFederateAmbassador(), callbackModel, configuration);
         connected = true;
         JavaTckSupport.check(connection != null,
            "public Java Connect returned a null ConfigurationResult");
         JavaTckSupport.check(
            connection.additionalSettingsResultCode == AdditionalSettingsResultCode.SETTINGS_APPLIED,
            "the public Java Connect call did not apply the bounded heartbeat settings");
         Thread.sleep(500L);
      } finally {
         if (connected) {
            rti.disconnect();
         }
      }
   }
}
