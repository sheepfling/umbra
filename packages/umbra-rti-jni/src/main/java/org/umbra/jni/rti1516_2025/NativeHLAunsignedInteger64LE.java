package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.encoding.HLAunsignedInteger64LE;
import java.lang.ref.Cleaner;

/** Java lifetime wrapper for one native C++ {@code HLAunsignedInteger64LE}. */
final class NativeHLAunsignedInteger64LE implements HLAunsignedInteger64LE {
   private static final Cleaner CLEANER = Cleaner.create();

   private static final class NativeHandle implements Runnable {
      private long value;
      NativeHandle(long value) { this.value = value; }
      long require() {
         if (value == 0L) throw new IllegalStateException("Native HLAunsignedInteger64LE is closed");
         return value;
      }
      @Override public void run() {
         if (value != 0L) {
            NativeBridge.nativeDestroyHLAunsignedInteger64LE(value);
            value = 0L;
         }
      }
   }

   private final NativeHandle nativeHandle;
   @SuppressWarnings("unused") private final Cleaner.Cleanable cleanable;

   NativeHLAunsignedInteger64LE(long value) {
      nativeHandle = new NativeHandle(NativeBridge.nativeCreateHLAunsignedInteger64LE(value));
      cleanable = CLEANER.register(this, nativeHandle);
   }

   @Override public long getValue() {
      return NativeBridge.nativeGetHLAunsignedInteger64LE(nativeHandle.require());
   }
   @Override public HLAunsignedInteger64LE setValue(long value) {
      NativeBridge.nativeSetHLAunsignedInteger64LE(nativeHandle.require(), value);
      return this;
   }
   @Override public int getOctetBoundary() {
      return NativeBridge.nativeHLAunsignedInteger64LEOctetBoundary(nativeHandle.require());
   }
   @Override public int getEncodedLength() {
      return NativeBridge.nativeHLAunsignedInteger64LEEncodedLength(nativeHandle.require());
   }
   @Override public byte[] toByteArray() {
      return NativeBridge.nativeHLAunsignedInteger64LEToByteArray(nativeHandle.require());
   }
   @Override public void encode(hla.rti1516_2025.encoding.ByteWrapper bytes) {
      NativeDataElementEncoding.encode(this, bytes);
   }
   @Override public hla.rti1516_2025.encoding.ByteWrapper encode() {
      return NativeDataElementEncoding.encode(this);
   }
   @Override public HLAunsignedInteger64LE decode(hla.rti1516_2025.encoding.ByteWrapper bytes)
      throws hla.rti1516_2025.encoding.DecoderException {
      return NativeDataElementEncoding.decode(this, bytes);
   }
   @Override public HLAunsignedInteger64LE decode(byte[] bytes) {
      NativeBridge.nativeDecodeHLAunsignedInteger64LE(nativeHandle.require(), bytes);
      return this;
   }
}
