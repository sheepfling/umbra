package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.encoding.HLAunsignedInteger64BE;
import java.lang.ref.Cleaner;

/** Java lifetime wrapper for one native C++ {@code HLAunsignedInteger64BE}. */
final class NativeHLAunsignedInteger64BE implements HLAunsignedInteger64BE {
   private static final Cleaner CLEANER = Cleaner.create();

   private static final class NativeHandle implements Runnable {
      private long value;
      NativeHandle(long value) { this.value = value; }
      long require() {
         if (value == 0L) throw new IllegalStateException("Native HLAunsignedInteger64BE is closed");
         return value;
      }
      @Override public void run() {
         if (value != 0L) {
            NativeBridge.nativeDestroyHLAunsignedInteger64BE(value);
            value = 0L;
         }
      }
   }

   private final NativeHandle nativeHandle;
   @SuppressWarnings("unused") private final Cleaner.Cleanable cleanable;

   NativeHLAunsignedInteger64BE(long value) {
      nativeHandle = new NativeHandle(NativeBridge.nativeCreateHLAunsignedInteger64BE(value));
      cleanable = CLEANER.register(this, nativeHandle);
   }

   @Override public long getValue() {
      return NativeBridge.nativeGetHLAunsignedInteger64BE(nativeHandle.require());
   }
   @Override public HLAunsignedInteger64BE setValue(long value) {
      NativeBridge.nativeSetHLAunsignedInteger64BE(nativeHandle.require(), value);
      return this;
   }
   @Override public int getOctetBoundary() {
      return NativeBridge.nativeHLAunsignedInteger64BEOctetBoundary(nativeHandle.require());
   }
   @Override public int getEncodedLength() {
      return NativeBridge.nativeHLAunsignedInteger64BEEncodedLength(nativeHandle.require());
   }
   @Override public byte[] toByteArray() {
      return NativeBridge.nativeHLAunsignedInteger64BEToByteArray(nativeHandle.require());
   }
   @Override public void encode(hla.rti1516_2025.encoding.ByteWrapper bytes) {
      NativeDataElementEncoding.encode(this, bytes);
   }
   @Override public hla.rti1516_2025.encoding.ByteWrapper encode() {
      return NativeDataElementEncoding.encode(this);
   }
   @Override public HLAunsignedInteger64BE decode(hla.rti1516_2025.encoding.ByteWrapper bytes)
      throws hla.rti1516_2025.encoding.DecoderException {
      return NativeDataElementEncoding.decode(this, bytes);
   }
   @Override public HLAunsignedInteger64BE decode(byte[] bytes) {
      NativeBridge.nativeDecodeHLAunsignedInteger64BE(nativeHandle.require(), bytes);
      return this;
   }
}
