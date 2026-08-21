package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.encoding.HLAoctetPairBE;
import java.lang.ref.Cleaner;

/** Java lifetime wrapper for one native C++ {@code HLAoctetPairBE}. */
final class NativeHLAoctetPairBE implements HLAoctetPairBE {
   private static final Cleaner CLEANER = Cleaner.create();

   private static final class NativeHandle implements Runnable {
      private long value;
      NativeHandle(long value) { this.value = value; }
      long require() {
         if (value == 0L) throw new IllegalStateException("Native HLAoctetPairBE is closed");
         return value;
      }
      @Override public void run() {
         if (value != 0L) {
            NativeBridge.nativeDestroyHLAoctetPairBE(value);
            value = 0L;
         }
      }
   }

   private final NativeHandle nativeHandle;
   @SuppressWarnings("unused") private final Cleaner.Cleanable cleanable;

   NativeHLAoctetPairBE(short value) {
      nativeHandle = new NativeHandle(NativeBridge.nativeCreateHLAoctetPairBE(value));
      cleanable = CLEANER.register(this, nativeHandle);
   }

   @Override public short getValue() {
      return NativeBridge.nativeGetHLAoctetPairBE(nativeHandle.require());
   }
   @Override public HLAoctetPairBE setValue(short value) {
      NativeBridge.nativeSetHLAoctetPairBE(nativeHandle.require(), value);
      return this;
   }
   @Override public int getOctetBoundary() {
      return NativeBridge.nativeHLAoctetPairBEOctetBoundary(nativeHandle.require());
   }
   @Override public int getEncodedLength() {
      return NativeBridge.nativeHLAoctetPairBEEncodedLength(nativeHandle.require());
   }
   @Override public byte[] toByteArray() {
      return NativeBridge.nativeHLAoctetPairBEToByteArray(nativeHandle.require());
   }
   @Override public void encode(hla.rti1516_2025.encoding.ByteWrapper bytes) {
      NativeDataElementEncoding.encode(this, bytes);
   }
   @Override public hla.rti1516_2025.encoding.ByteWrapper encode() {
      return NativeDataElementEncoding.encode(this);
   }
   @Override public HLAoctetPairBE decode(hla.rti1516_2025.encoding.ByteWrapper bytes)
      throws hla.rti1516_2025.encoding.DecoderException {
      return NativeDataElementEncoding.decode(this, bytes);
   }
   @Override public HLAoctetPairBE decode(byte[] bytes) {
      NativeBridge.nativeDecodeHLAoctetPairBE(nativeHandle.require(), bytes);
      return this;
   }
}
