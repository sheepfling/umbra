package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.FederateHandle;
import java.util.Arrays;

/** Immutable Java copy of the standard encoded federate-handle value. */
final class NativeFederateHandle implements FederateHandle {
   private final byte[] encoded;

   NativeFederateHandle(byte[] value) {
      encoded = value.clone();
   }

   @Override
   public int encodedLength() {
      return encoded.length;
   }

   @Override
   public void encode(byte[] buffer, int offset) {
      System.arraycopy(encoded, 0, buffer, offset, encoded.length);
   }

   @Override
   public boolean equals(Object other) {
      return other instanceof FederateHandle && sameEncoding((FederateHandle) other);
   }

   @Override
   public int hashCode() {
      return Arrays.hashCode(encoded);
   }

   private boolean sameEncoding(FederateHandle other) {
      if (other.encodedLength() != encoded.length) return false;
      byte[] candidate = new byte[encoded.length];
      other.encode(candidate, 0);
      return Arrays.equals(encoded, candidate);
   }
}
