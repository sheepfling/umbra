package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.encoding.HLAunicodeChar;
import java.lang.ref.Cleaner;

/** Java lifetime wrapper for one native C++ {@code HLAunicodeChar}. */
final class NativeHLAunicodeChar implements HLAunicodeChar {
   private static final Cleaner CLEANER = Cleaner.create();

   private static final class NativeHandle implements Runnable {
      private long value;
      NativeHandle(long value) { this.value = value; }
      long require() {
         if (value == 0L) throw new IllegalStateException("Native HLAunicodeChar is closed");
         return value;
      }
      @Override public void run() {
         if (value != 0L) {
            NativeBridge.nativeDestroyHLAunicodeChar(value);
            value = 0L;
         }
      }
   }

   private final NativeHandle nativeHandle;
   @SuppressWarnings("unused") private final Cleaner.Cleanable cleanable;

   NativeHLAunicodeChar(short value) {
      nativeHandle = new NativeHandle(NativeBridge.nativeCreateHLAunicodeChar(value));
      cleanable = CLEANER.register(this, nativeHandle);
   }

   @Override public short getValue() {
      return NativeBridge.nativeGetHLAunicodeChar(nativeHandle.require());
   }
   @Override public HLAunicodeChar setValue(short value) {
      NativeBridge.nativeSetHLAunicodeChar(nativeHandle.require(), value);
      return this;
   }
   @Override public int getOctetBoundary() {
      return NativeBridge.nativeHLAunicodeCharOctetBoundary(nativeHandle.require());
   }
   @Override public int getEncodedLength() {
      return NativeBridge.nativeHLAunicodeCharEncodedLength(nativeHandle.require());
   }
   @Override public byte[] toByteArray() {
      return NativeBridge.nativeHLAunicodeCharToByteArray(nativeHandle.require());
   }
   @Override public void encode(hla.rti1516_2025.encoding.ByteWrapper bytes) {
      NativeDataElementEncoding.encode(this, bytes);
   }
   @Override public hla.rti1516_2025.encoding.ByteWrapper encode() {
      return NativeDataElementEncoding.encode(this);
   }
   @Override public HLAunicodeChar decode(hla.rti1516_2025.encoding.ByteWrapper bytes)
      throws hla.rti1516_2025.encoding.DecoderException {
      return NativeDataElementEncoding.decode(this, bytes);
   }
   @Override public HLAunicodeChar decode(byte[] bytes) {
      NativeBridge.nativeDecodeHLAunicodeChar(nativeHandle.require(), bytes);
      return this;
   }
}
