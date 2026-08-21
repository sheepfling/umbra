package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.encoding.HLAASCIIstring;
import java.lang.ref.Cleaner;

/** Java lifetime wrapper for one native C++ {@code HLAASCIIstring}. */
final class NativeHLAASCIIstring implements HLAASCIIstring {
   private static final Cleaner CLEANER = Cleaner.create();

   private static final class NativeHandle implements Runnable {
      private long value;
      NativeHandle(long value) { this.value = value; }
      long require() {
         if (value == 0L) throw new IllegalStateException("Native HLAASCIIstring is closed");
         return value;
      }
      @Override public void run() {
         if (value != 0L) {
            NativeBridge.nativeDestroyHLAASCIIstring(value);
            value = 0L;
         }
      }
   }

   private final NativeHandle nativeHandle;
   @SuppressWarnings("unused") private final Cleaner.Cleanable cleanable;

   NativeHLAASCIIstring(String value) {
      nativeHandle = new NativeHandle(NativeBridge.nativeCreateHLAASCIIstring(value));
      cleanable = CLEANER.register(this, nativeHandle);
   }

   @Override public String getValue() {
      return NativeBridge.nativeGetHLAASCIIstring(nativeHandle.require());
   }
   @Override public HLAASCIIstring setValue(String value) {
      NativeBridge.nativeSetHLAASCIIstring(nativeHandle.require(), value);
      return this;
   }
   @Override public int getOctetBoundary() {
      return NativeBridge.nativeHLAASCIIstringOctetBoundary(nativeHandle.require());
   }
   @Override public int getEncodedLength() {
      return NativeBridge.nativeHLAASCIIstringEncodedLength(nativeHandle.require());
   }
   @Override public byte[] toByteArray() {
      return NativeBridge.nativeHLAASCIIstringToByteArray(nativeHandle.require());
   }
   @Override public void encode(hla.rti1516_2025.encoding.ByteWrapper bytes) {
      NativeDataElementEncoding.encode(this, bytes);
   }
   @Override public hla.rti1516_2025.encoding.ByteWrapper encode() {
      return NativeDataElementEncoding.encode(this);
   }
   @Override public HLAASCIIstring decode(hla.rti1516_2025.encoding.ByteWrapper bytes)
      throws hla.rti1516_2025.encoding.DecoderException {
      return NativeDataElementEncoding.decode(this, bytes);
   }
   @Override public HLAASCIIstring decode(byte[] bytes) {
      NativeBridge.nativeDecodeHLAASCIIstring(nativeHandle.require(), bytes);
      return this;
   }
}
