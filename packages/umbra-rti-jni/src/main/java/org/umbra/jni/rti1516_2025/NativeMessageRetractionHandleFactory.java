package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.MessageRetractionHandleFactory;

/** Decoder whose wire validation and handle identity remain C++-owned. */
final class NativeMessageRetractionHandleFactory implements MessageRetractionHandleFactory {
   private final long nativeHandle;

   NativeMessageRetractionHandleFactory(long nativeHandle) {
      this.nativeHandle = nativeHandle;
   }

   @Override
   public MessageRetractionHandle decode(byte[] buffer, int offset) {
      if (buffer == null || offset < 0 || offset > buffer.length) {
         throw new IndexOutOfBoundsException(
            "Invalid MessageRetractionHandle decode buffer or offset");
      }
      byte[] value = new byte[buffer.length - offset];
      System.arraycopy(buffer, offset, value, 0, value.length);
      return new NativeMessageRetractionHandle(
         NativeBridge.nativeDecodeMessageRetractionHandle(nativeHandle, value));
   }
}
