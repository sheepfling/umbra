package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleFactory;

/** Decoder for C++-validated encoded {@link AttributeHandle} values. */
final class NativeAttributeHandleFactory implements AttributeHandleFactory {
   @Override
   public AttributeHandle decode(byte[] buffer, int offset) {
      if (buffer == null || offset < 0 || offset > buffer.length) {
         throw new IndexOutOfBoundsException("Invalid AttributeHandle decode buffer or offset");
      }
      byte[] value = new byte[buffer.length - offset];
      System.arraycopy(buffer, offset, value, 0, value.length);
      return new NativeAttributeHandle(NativeBridge.nativeDecodeAttributeHandle(value));
   }
}
