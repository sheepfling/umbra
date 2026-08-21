package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.encoding.HLAASCIIchar;
import java.lang.ref.Cleaner;

/** Java lifetime wrapper for one native C++ {@code HLAASCIIchar}. */
final class NativeHLAASCIIchar implements HLAASCIIchar {
   private static final Cleaner CLEANER = Cleaner.create();

   private static final class NativeHandle implements Runnable {
      private long value;
      NativeHandle(long value) { this.value = value; }
      long require() {
         if (value == 0L) throw new IllegalStateException("Native HLAASCIIchar is closed");
         return value;
      }
      @Override public void run() {
         if (value != 0L) {
            NativeBridge.nativeDestroyHLAASCIIchar(value);
            value = 0L;
         }
      }
   }

   private final NativeHandle nativeHandle;
   @SuppressWarnings("unused") private final Cleaner.Cleanable cleanable;

   NativeHLAASCIIchar(byte value) {
      nativeHandle = new NativeHandle(NativeBridge.nativeCreateHLAASCIIchar(value));
      cleanable = CLEANER.register(this, nativeHandle);
   }

   @Override public byte getValue() {
      return NativeBridge.nativeGetHLAASCIIchar(nativeHandle.require());
   }
   @Override public HLAASCIIchar setValue(byte value) {
      NativeBridge.nativeSetHLAASCIIchar(nativeHandle.require(), value);
      return this;
   }
   @Override public int getOctetBoundary() {
      return NativeBridge.nativeHLAASCIIcharOctetBoundary(nativeHandle.require());
   }
   @Override public int getEncodedLength() {
      return NativeBridge.nativeHLAASCIIcharEncodedLength(nativeHandle.require());
   }
   @Override public byte[] toByteArray() {
      return NativeBridge.nativeHLAASCIIcharToByteArray(nativeHandle.require());
   }
   @Override public void encode(hla.rti1516_2025.encoding.ByteWrapper bytes) {
      NativeDataElementEncoding.encode(this, bytes);
   }
   @Override public hla.rti1516_2025.encoding.ByteWrapper encode() {
      return NativeDataElementEncoding.encode(this);
   }
   @Override public HLAASCIIchar decode(hla.rti1516_2025.encoding.ByteWrapper bytes)
      throws hla.rti1516_2025.encoding.DecoderException {
      return NativeDataElementEncoding.decode(this, bytes);
   }
   @Override public HLAASCIIchar decode(byte[] bytes) {
      NativeBridge.nativeDecodeHLAASCIIchar(nativeHandle.require(), bytes);
      return this;
   }
}
