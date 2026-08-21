package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.InteractionClassHandleFactory;

/** Decoder for C++-validated encoded {@link InteractionClassHandle} values. */
final class NativeInteractionClassHandleFactory implements InteractionClassHandleFactory {
   @Override
   public InteractionClassHandle decode(byte[] buffer, int offset) {
      if (buffer == null || offset < 0 || offset > buffer.length) {
         throw new IndexOutOfBoundsException(
            "Invalid InteractionClassHandle decode buffer or offset");
      }
      byte[] value = new byte[buffer.length - offset];
      System.arraycopy(buffer, offset, value, 0, value.length);
      return new NativeInteractionClassHandle(NativeBridge.nativeDecodeInteractionClassHandle(value));
   }
}
