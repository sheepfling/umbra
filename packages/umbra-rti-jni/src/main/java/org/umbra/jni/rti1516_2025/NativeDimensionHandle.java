package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.DimensionHandle;
import java.util.Arrays;

/** Immutable Java copy of a standard encoded dimension handle. */
final class NativeDimensionHandle implements DimensionHandle {
   private final byte[] encoded;

   NativeDimensionHandle(byte[] value) {
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
      if (!(other instanceof DimensionHandle)) return false;
      DimensionHandle candidate = (DimensionHandle) other;
      if (candidate.encodedLength() != encoded.length) return false;
      byte[] candidateBytes = new byte[encoded.length];
      candidate.encode(candidateBytes, 0);
      return Arrays.equals(encoded, candidateBytes);
   }

   @Override
   public int hashCode() {
      return Arrays.hashCode(encoded);
   }
}
