package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.encoding.HLAunsignedInteger32LE;
import java.lang.ref.Cleaner;

/** Java lifetime wrapper for one native C++ {@code HLAunsignedInteger32LE}. */
final class NativeHLAunsignedInteger32LE implements HLAunsignedInteger32LE {
   private static final Cleaner CLEANER = Cleaner.create();

   private static final class NativeHandle implements Runnable {
      private long value;
      NativeHandle(long value) { this.value = value; }
      long require() {
         if (value == 0L) throw new IllegalStateException("Native HLAunsignedInteger32LE is closed");
         return value;
      }
      @Override public void run() {
         if (value != 0L) {
            NativeBridge.nativeDestroyHLAunsignedInteger32LE(value);
            value = 0L;
         }
      }
   }

   private final NativeHandle nativeHandle;
   @SuppressWarnings("unused") private final Cleaner.Cleanable cleanable;

   NativeHLAunsignedInteger32LE(int value) {
      nativeHandle = new NativeHandle(NativeBridge.nativeCreateHLAunsignedInteger32LE(value));
      cleanable = CLEANER.register(this, nativeHandle);
   }

   @Override public int getValue() {
      return NativeBridge.nativeGetHLAunsignedInteger32LE(nativeHandle.require());
   }
   @Override public HLAunsignedInteger32LE setValue(int value) {
      NativeBridge.nativeSetHLAunsignedInteger32LE(nativeHandle.require(), value);
      return this;
   }
   @Override public int getOctetBoundary() {
      return NativeBridge.nativeHLAunsignedInteger32LEOctetBoundary(nativeHandle.require());
   }
   @Override public int getEncodedLength() {
      return NativeBridge.nativeHLAunsignedInteger32LEEncodedLength(nativeHandle.require());
   }
   @Override public byte[] toByteArray() {
      return NativeBridge.nativeHLAunsignedInteger32LEToByteArray(nativeHandle.require());
   }
   @Override public void encode(hla.rti1516_2025.encoding.ByteWrapper bytes) {
      NativeDataElementEncoding.encode(this, bytes);
   }
   @Override public hla.rti1516_2025.encoding.ByteWrapper encode() {
      return NativeDataElementEncoding.encode(this);
   }
   @Override public HLAunsignedInteger32LE decode(hla.rti1516_2025.encoding.ByteWrapper bytes)
      throws hla.rti1516_2025.encoding.DecoderException {
      return NativeDataElementEncoding.decode(this, bytes);
   }
   @Override public HLAunsignedInteger32LE decode(byte[] bytes) {
      NativeBridge.nativeDecodeHLAunsignedInteger32LE(nativeHandle.require(), bytes);
      return this;
   }
}
