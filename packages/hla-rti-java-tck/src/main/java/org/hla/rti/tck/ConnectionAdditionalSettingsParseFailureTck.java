package org.hla.rti.tck;

import hla.rti1516_2025.AdditionalSettingsResultCode;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.ConfigurationResult;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.RtiFactory;

/** Checks that malformed optional settings do not prevent Connect from completing. */
final class ConnectionAdditionalSettingsParseFailureTck {
   private ConnectionAdditionalSettingsParseFailureTck() { }

   static void run(RtiFactory factory, CallbackModel callbackModel,
         String malformedAdditionalSettings) throws Exception {
      JavaTckSupport.check(malformedAdditionalSettings != null
            && !malformedAdditionalSettings.isEmpty(),
         "adapter-supplied malformed additional settings are required");
      RTIambassador ambassador = factory.getRtiAmbassador();
      RtiConfiguration configuration = RtiConfiguration.createConfiguration()
         .withAdditionalSettings(malformedAdditionalSettings);
      boolean connected = false;
      try {
         ConfigurationResult result = ambassador.connect(
            new NullFederateAmbassador(), callbackModel, configuration);
         connected = true;
         JavaTckSupport.check(result != null
               && result.additionalSettingsResultCode
                  == AdditionalSettingsResultCode.SETTINGS_FAILED_TO_PARSE,
            "Connect did not complete with SETTINGS_FAILED_TO_PARSE for the adapter-supplied malformed setting"
               + (result == null ? "; no ConfigurationResult was returned"
                  : "; received " + result.additionalSettingsResultCode));
         ambassador.disconnect();
         connected = false;
      } finally {
         if (connected) {
            try { ambassador.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }
}
