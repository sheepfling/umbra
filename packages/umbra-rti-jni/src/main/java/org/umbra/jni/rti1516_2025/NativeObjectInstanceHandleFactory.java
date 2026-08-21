package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ObjectInstanceHandleFactory;

/** Decoder for C++-validated encoded {@link ObjectInstanceHandle} values. */
final class NativeObjectInstanceHandleFactory implements ObjectInstanceHandleFactory {
   @Override
   public ObjectInstanceHandle decode(byte[] buffer, int offset) {
      if (buffer == null || offset < 0 || offset > buffer.length) {
         throw new IndexOutOfBoundsException("Invalid ObjectInstanceHandle decode buffer or offset");
      }
      byte[] value = new byte[buffer.length - offset];
      System.arraycopy(buffer, offset, value, 0, value.length);
      return new NativeObjectInstanceHandle(NativeBridge.nativeDecodeObjectInstanceHandle(value));
   }
}
