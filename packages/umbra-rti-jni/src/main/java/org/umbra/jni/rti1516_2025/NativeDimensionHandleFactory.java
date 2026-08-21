package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleFactory;

/** Decoder for C++-validated encoded {@link DimensionHandle} values. */
final class NativeDimensionHandleFactory implements DimensionHandleFactory {
   @Override
   public DimensionHandle decode(byte[] buffer, int offset) {
      if (buffer == null || offset < 0 || offset > buffer.length) {
         throw new IndexOutOfBoundsException("Invalid DimensionHandle decode buffer or offset");
      }
      byte[] value = new byte[buffer.length - offset];
      System.arraycopy(buffer, offset, value, 0, value.length);
      return new NativeDimensionHandle(NativeBridge.nativeDecodeDimensionHandle(value));
   }
}
