package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.encoding.HLAfloat32LE;
import java.lang.ref.Cleaner;

/** Java lifetime wrapper for one native C++ {@code HLAfloat32LE}. */
final class NativeHLAfloat32LE implements HLAfloat32LE {
   private static final Cleaner CLEANER = Cleaner.create();

   private static final class NativeHandle implements Runnable {
      private long value;

      NativeHandle(long value) { this.value = value; }

      long require() {
         if (value == 0L) throw new IllegalStateException("Native HLAfloat32LE is closed");
         return value;
      }

      @Override
      public void run() {
         if (value != 0L) {
            NativeBridge.nativeDestroyHLAfloat32LE(value);
            value = 0L;
         }
      }
   }

   private final NativeHandle nativeHandle;
   @SuppressWarnings("unused")
   private final Cleaner.Cleanable cleanable;

   NativeHLAfloat32LE(float value) {
      nativeHandle = new NativeHandle(NativeBridge.nativeCreateHLAfloat32LE(value));
      cleanable = CLEANER.register(this, nativeHandle);
   }

   @Override
   public float getValue() { return NativeBridge.nativeGetHLAfloat32LE(nativeHandle.require()); }

   @Override
   public HLAfloat32LE setValue(float value) {
      NativeBridge.nativeSetHLAfloat32LE(nativeHandle.require(), value);
      return this;
   }

   @Override
   public int getOctetBoundary() {
      return NativeBridge.nativeHLAfloat32LEOctetBoundary(nativeHandle.require());
   }

   @Override
   public int getEncodedLength() {
      return NativeBridge.nativeHLAfloat32LEEncodedLength(nativeHandle.require());
   }

   @Override
   public byte[] toByteArray() {
      return NativeBridge.nativeHLAfloat32LEToByteArray(nativeHandle.require());
   }

   @Override public void encode(hla.rti1516_2025.encoding.ByteWrapper bytes) {

      NativeDataElementEncoding.encode(this, bytes);

   }

   @Override public hla.rti1516_2025.encoding.ByteWrapper encode() {

      return NativeDataElementEncoding.encode(this);

   }

   @Override public HLAfloat32LE decode(hla.rti1516_2025.encoding.ByteWrapper bytes)

      throws hla.rti1516_2025.encoding.DecoderException {

      return NativeDataElementEncoding.decode(this, bytes);

   }

   @Override public HLAfloat32LE decode(byte[] bytes) {
      NativeBridge.nativeDecodeHLAfloat32LE(nativeHandle.require(), bytes);
      return this;
   }
}
