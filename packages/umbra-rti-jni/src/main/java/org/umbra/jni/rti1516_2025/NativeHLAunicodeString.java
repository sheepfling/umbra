package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.encoding.HLAunicodeString;
import java.lang.ref.Cleaner;

/** Java lifetime wrapper for one native C++ {@code HLAunicodeString}. */
final class NativeHLAunicodeString implements HLAunicodeString {
   private static final Cleaner CLEANER = Cleaner.create();

   private static final class NativeHandle implements Runnable {
      private long value;
      NativeHandle(long value) { this.value = value; }
      long require() {
         if (value == 0L) throw new IllegalStateException("Native HLAunicodeString is closed");
         return value;
      }
      @Override public void run() {
         if (value != 0L) {
            NativeBridge.nativeDestroyHLAunicodeString(value);
            value = 0L;
         }
      }
   }

   private final NativeHandle nativeHandle;
   @SuppressWarnings("unused") private final Cleaner.Cleanable cleanable;

   NativeHLAunicodeString(String value) {
      nativeHandle = new NativeHandle(NativeBridge.nativeCreateHLAunicodeString(value));
      cleanable = CLEANER.register(this, nativeHandle);
   }

   @Override public String getValue() {
      return NativeBridge.nativeGetHLAunicodeString(nativeHandle.require());
   }
   @Override public HLAunicodeString setValue(String value) {
      NativeBridge.nativeSetHLAunicodeString(nativeHandle.require(), value);
      return this;
   }
   @Override public int getOctetBoundary() {
      return NativeBridge.nativeHLAunicodeStringOctetBoundary(nativeHandle.require());
   }
   @Override public int getEncodedLength() {
      return NativeBridge.nativeHLAunicodeStringEncodedLength(nativeHandle.require());
   }
   @Override public byte[] toByteArray() {
      return NativeBridge.nativeHLAunicodeStringToByteArray(nativeHandle.require());
   }
   @Override public void encode(hla.rti1516_2025.encoding.ByteWrapper bytes) {
      NativeDataElementEncoding.encode(this, bytes);
   }
   @Override public hla.rti1516_2025.encoding.ByteWrapper encode() {
      return NativeDataElementEncoding.encode(this);
   }
   @Override public HLAunicodeString decode(hla.rti1516_2025.encoding.ByteWrapper bytes)
      throws hla.rti1516_2025.encoding.DecoderException {
      return NativeDataElementEncoding.decode(this, bytes);
   }
   @Override public HLAunicodeString decode(byte[] bytes) {
      NativeBridge.nativeDecodeHLAunicodeString(nativeHandle.require(), bytes);
      return this;
   }
}
