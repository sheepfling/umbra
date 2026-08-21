package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.encoding.HLAunsignedInteger32BE;
import java.lang.ref.Cleaner;

/** Java lifetime wrapper for one native C++ {@code HLAunsignedInteger32BE}. */
final class NativeHLAunsignedInteger32BE implements HLAunsignedInteger32BE {
   private static final Cleaner CLEANER = Cleaner.create();

   private static final class NativeHandle implements Runnable {
      private long value;
      NativeHandle(long value) { this.value = value; }
      long require() {
         if (value == 0L) throw new IllegalStateException("Native HLAunsignedInteger32BE is closed");
         return value;
      }
      @Override public void run() {
         if (value != 0L) {
            NativeBridge.nativeDestroyHLAunsignedInteger32BE(value);
            value = 0L;
         }
      }
   }

   private final NativeHandle nativeHandle;
   @SuppressWarnings("unused") private final Cleaner.Cleanable cleanable;

   NativeHLAunsignedInteger32BE(int value) {
      nativeHandle = new NativeHandle(NativeBridge.nativeCreateHLAunsignedInteger32BE(value));
      cleanable = CLEANER.register(this, nativeHandle);
   }

   @Override public int getValue() {
      return NativeBridge.nativeGetHLAunsignedInteger32BE(nativeHandle.require());
   }
   @Override public HLAunsignedInteger32BE setValue(int value) {
      NativeBridge.nativeSetHLAunsignedInteger32BE(nativeHandle.require(), value);
      return this;
   }
   @Override public int getOctetBoundary() {
      return NativeBridge.nativeHLAunsignedInteger32BEOctetBoundary(nativeHandle.require());
   }
   @Override public int getEncodedLength() {
      return NativeBridge.nativeHLAunsignedInteger32BEEncodedLength(nativeHandle.require());
   }
   @Override public byte[] toByteArray() {
      return NativeBridge.nativeHLAunsignedInteger32BEToByteArray(nativeHandle.require());
   }
   @Override public void encode(hla.rti1516_2025.encoding.ByteWrapper bytes) {
      NativeDataElementEncoding.encode(this, bytes);
   }
   @Override public hla.rti1516_2025.encoding.ByteWrapper encode() {
      return NativeDataElementEncoding.encode(this);
   }
   @Override public HLAunsignedInteger32BE decode(hla.rti1516_2025.encoding.ByteWrapper bytes)
      throws hla.rti1516_2025.encoding.DecoderException {
      return NativeDataElementEncoding.decode(this, bytes);
   }
   @Override public HLAunsignedInteger32BE decode(byte[] bytes) {
      NativeBridge.nativeDecodeHLAunsignedInteger32BE(nativeHandle.require(), bytes);
      return this;
   }
}
