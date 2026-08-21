package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.encoding.HLAoctetPairLE;
import java.lang.ref.Cleaner;

/** Java lifetime wrapper for one native C++ {@code HLAoctetPairLE}. */
final class NativeHLAoctetPairLE implements HLAoctetPairLE {
   private static final Cleaner CLEANER = Cleaner.create();

   private static final class NativeHandle implements Runnable {
      private long value;
      NativeHandle(long value) { this.value = value; }
      long require() {
         if (value == 0L) throw new IllegalStateException("Native HLAoctetPairLE is closed");
         return value;
      }
      @Override public void run() {
         if (value != 0L) {
            NativeBridge.nativeDestroyHLAoctetPairLE(value);
            value = 0L;
         }
      }
   }

   private final NativeHandle nativeHandle;
   @SuppressWarnings("unused") private final Cleaner.Cleanable cleanable;

   NativeHLAoctetPairLE(short value) {
      nativeHandle = new NativeHandle(NativeBridge.nativeCreateHLAoctetPairLE(value));
      cleanable = CLEANER.register(this, nativeHandle);
   }

   @Override public short getValue() {
      return NativeBridge.nativeGetHLAoctetPairLE(nativeHandle.require());
   }
   @Override public HLAoctetPairLE setValue(short value) {
      NativeBridge.nativeSetHLAoctetPairLE(nativeHandle.require(), value);
      return this;
   }
   @Override public int getOctetBoundary() {
      return NativeBridge.nativeHLAoctetPairLEOctetBoundary(nativeHandle.require());
   }
   @Override public int getEncodedLength() {
      return NativeBridge.nativeHLAoctetPairLEEncodedLength(nativeHandle.require());
   }
   @Override public byte[] toByteArray() {
      return NativeBridge.nativeHLAoctetPairLEToByteArray(nativeHandle.require());
   }
   @Override public void encode(hla.rti1516_2025.encoding.ByteWrapper bytes) {
      NativeDataElementEncoding.encode(this, bytes);
   }
   @Override public hla.rti1516_2025.encoding.ByteWrapper encode() {
      return NativeDataElementEncoding.encode(this);
   }
   @Override public HLAoctetPairLE decode(hla.rti1516_2025.encoding.ByteWrapper bytes)
      throws hla.rti1516_2025.encoding.DecoderException {
      return NativeDataElementEncoding.decode(this, bytes);
   }
   @Override public HLAoctetPairLE decode(byte[] bytes) {
      NativeBridge.nativeDecodeHLAoctetPairLE(nativeHandle.require(), bytes);
      return this;
   }
}
