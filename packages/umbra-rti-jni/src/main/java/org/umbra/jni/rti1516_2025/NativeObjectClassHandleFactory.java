package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectClassHandleFactory;

/** Decoder for C++-validated encoded {@link ObjectClassHandle} values. */
final class NativeObjectClassHandleFactory implements ObjectClassHandleFactory {
   @Override
   public ObjectClassHandle decode(byte[] buffer, int offset) {
      if (buffer == null || offset < 0 || offset > buffer.length) {
         throw new IndexOutOfBoundsException("Invalid ObjectClassHandle decode buffer or offset");
      }
      byte[] value = new byte[buffer.length - offset];
      System.arraycopy(buffer, offset, value, 0, value.length);
      return new NativeObjectClassHandle(NativeBridge.nativeDecodeObjectClassHandle(value));
   }
}
