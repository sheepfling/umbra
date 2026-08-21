package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleFactory;

/** Decoder for C++-validated encoded {@link ParameterHandle} values. */
final class NativeParameterHandleFactory implements ParameterHandleFactory {
   @Override
   public ParameterHandle decode(byte[] buffer, int offset) {
      if (buffer == null || offset < 0 || offset > buffer.length) {
         throw new IndexOutOfBoundsException("Invalid ParameterHandle decode buffer or offset");
      }
      byte[] value = new byte[buffer.length - offset];
      System.arraycopy(buffer, offset, value, 0, value.length);
      return new NativeParameterHandle(NativeBridge.nativeDecodeParameterHandle(value));
   }
}
