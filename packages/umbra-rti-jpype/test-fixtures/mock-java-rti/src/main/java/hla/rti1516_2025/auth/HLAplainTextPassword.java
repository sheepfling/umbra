package hla.rti1516_2025.auth;

import java.nio.charset.StandardCharsets;
import java.nio.ByteBuffer;

/** Standard HLAplainTextPassword credential carrier. */
public final class HLAplainTextPassword extends Credentials {
   public static final String HLA_PLAIN_TEXT_PASSWORD_TYPE = "HLAplainTextPassword";

   public HLAplainTextPassword(String password) {
      super(HLA_PLAIN_TEXT_PASSWORD_TYPE, encode(password));
   }

   public HLAplainTextPassword(byte[] encodedPassword) {
      super(HLA_PLAIN_TEXT_PASSWORD_TYPE, encodedPassword);
   }

   public String decode() {
      byte[] encoded = getData();
      if (encoded.length < Integer.BYTES) {
         throw new IllegalArgumentException("HLAplainTextPassword encoding is truncated");
      }
      int units = ByteBuffer.wrap(encoded, 0, Integer.BYTES).getInt();
      if (units < 0 || encoded.length != Integer.BYTES + units * 2) {
         throw new IllegalArgumentException("HLAplainTextPassword encoding is invalid");
      }
      return new String(encoded, Integer.BYTES, units * 2, StandardCharsets.UTF_16BE);
   }

   public static byte[] encode(String password) {
      byte[] utf16 = password.getBytes(StandardCharsets.UTF_16BE);
      return ByteBuffer.allocate(Integer.BYTES + utf16.length)
         .putInt(password.length())
         .put(utf16)
         .array();
   }
}
