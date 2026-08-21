package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.ParameterHandle;
import java.util.Arrays;

/** Immutable Java copy of a standard encoded interaction-parameter handle. */
final class NativeParameterHandle implements ParameterHandle {
   private final byte[] encoded;

   NativeParameterHandle(byte[] value) {
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
      return other instanceof ParameterHandle && sameEncoding((ParameterHandle) other);
   }

   @Override
   public int hashCode() {
      return Arrays.hashCode(encoded);
   }

   private boolean sameEncoding(ParameterHandle other) {
      if (other.encodedLength() != encoded.length) return false;
      byte[] candidate = new byte[encoded.length];
      other.encode(candidate, 0);
      return Arrays.equals(encoded, candidate);
   }
}
