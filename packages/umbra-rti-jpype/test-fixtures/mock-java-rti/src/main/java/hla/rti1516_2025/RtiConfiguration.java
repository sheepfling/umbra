package hla.rti1516_2025;

/** Small Java-shaped connection configuration value for the adapter fixture. */
public final class RtiConfiguration {
   private String configurationName = "";
   private String rtiAddress = "";
   private String additionalSettings = "";

   private RtiConfiguration() { }

   public static RtiConfiguration createConfiguration() {
      return new RtiConfiguration();
   }

   public RtiConfiguration withConfigurationName(String value) {
      configurationName = value;
      return this;
   }

   public RtiConfiguration withRtiAddress(String value) {
      rtiAddress = value;
      return this;
   }

   public RtiConfiguration withAdditionalSettings(String value) {
      additionalSettings = value;
      return this;
   }

   public String configurationName() { return configurationName; }
   public String rtiAddress() { return rtiAddress; }
   public String additionalSettings() { return additionalSettings; }
}
