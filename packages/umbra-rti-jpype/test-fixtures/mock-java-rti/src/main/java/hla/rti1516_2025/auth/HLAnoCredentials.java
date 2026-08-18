package hla.rti1516_2025.auth;

/** Standard no-credentials marker used by the connection overloads. */
public final class HLAnoCredentials extends Credentials {
   public HLAnoCredentials() {
      super("HLAnoCredentials", new byte[0]);
   }
}
