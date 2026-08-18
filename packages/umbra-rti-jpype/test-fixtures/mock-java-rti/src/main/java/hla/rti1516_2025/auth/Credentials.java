package hla.rti1516_2025.auth;

/** Test-fixture credential value with the standard connection shape. */
public class Credentials {
   private final String type;
   private final byte[] data;

   public Credentials(String type, byte[] data) {
      this.type = type;
      this.data = data.clone();
   }

   public String getType() { return type; }
   public byte[] getData() { return data.clone(); }
}
