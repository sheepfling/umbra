package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.encoding.HLAboolean;
import java.lang.ref.Cleaner;

/** Java lifetime wrapper for one native C++ {@code HLAboolean}. */
final class NativeHLAboolean implements HLAboolean {
   private static final Cleaner CLEANER = Cleaner.create();

   private static final class NativeHandle implements Runnable {
      private long value;
      NativeHandle(long value) { this.value = value; }
      long require() {
         if (value == 0L) throw new IllegalStateException("Native HLAboolean is closed");
         return value;
      }
      @Override public void run() {
         if (value != 0L) {
            NativeBridge.nativeDestroyHLAboolean(value);
            value = 0L;
         }
      }
   }

   private final NativeHandle nativeHandle;
   @SuppressWarnings("unused") private final Cleaner.Cleanable cleanable;

   NativeHLAboolean(boolean value) {
      nativeHandle = new NativeHandle(NativeBridge.nativeCreateHLAboolean(value));
      cleanable = CLEANER.register(this, nativeHandle);
   }

   @Override public boolean getValue() {
      return NativeBridge.nativeGetHLAboolean(nativeHandle.require());
   }
   @Override public HLAboolean setValue(boolean value) {
      NativeBridge.nativeSetHLAboolean(nativeHandle.require(), value);
      return this;
   }
   @Override public int getOctetBoundary() {
      return NativeBridge.nativeHLAbooleanOctetBoundary(nativeHandle.require());
   }
   @Override public int getEncodedLength() {
      return NativeBridge.nativeHLAbooleanEncodedLength(nativeHandle.require());
   }
   @Override public byte[] toByteArray() {
      return NativeBridge.nativeHLAbooleanToByteArray(nativeHandle.require());
   }
   @Override public void encode(hla.rti1516_2025.encoding.ByteWrapper bytes) {
      NativeDataElementEncoding.encode(this, bytes);
   }
   @Override public hla.rti1516_2025.encoding.ByteWrapper encode() {
      return NativeDataElementEncoding.encode(this);
   }
   @Override public HLAboolean decode(hla.rti1516_2025.encoding.ByteWrapper bytes)
      throws hla.rti1516_2025.encoding.DecoderException {
      return NativeDataElementEncoding.decode(this, bytes);
   }
   @Override public HLAboolean decode(byte[] bytes) {
      NativeBridge.nativeDecodeHLAboolean(nativeHandle.require(), bytes);
      return this;
   }
}
