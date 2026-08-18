package hla.rti1516_2025;

public final class ConfigurationResult {
   public ConfigurationResult(
      boolean configurationUsed,
      boolean addressUsed,
      AdditionalSettingsResultCode additionalSettingsResultCode,
      String message)
   {
      this.configurationUsed = configurationUsed;
      this.addressUsed = addressUsed;
      this.additionalSettingsResultCode = additionalSettingsResultCode;
      this.message = message;
   }

   public final boolean configurationUsed;
   public final boolean addressUsed;
   public final AdditionalSettingsResultCode additionalSettingsResultCode;
   public final String message;
}
