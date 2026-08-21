package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.encoding.HLAfloat32BE;
import java.lang.ref.Cleaner;

/** Java lifetime wrapper for one native C++ {@code HLAfloat32BE}. */
final class NativeHLAfloat32BE implements HLAfloat32BE {
   private static final Cleaner CLEANER = Cleaner.create();

   private static final class NativeHandle implements Runnable {
      private long value;

      NativeHandle(long value) { this.value = value; }

      long require() {
         if (value == 0L) throw new IllegalStateException("Native HLAfloat32BE is closed");
         return value;
      }

      @Override
      public void run() {
         if (value != 0L) {
            NativeBridge.nativeDestroyHLAfloat32BE(value);
            value = 0L;
         }
      }
   }

   private final NativeHandle nativeHandle;
   @SuppressWarnings("unused")
   private final Cleaner.Cleanable cleanable;

   NativeHLAfloat32BE(float value) {
      nativeHandle = new NativeHandle(NativeBridge.nativeCreateHLAfloat32BE(value));
      cleanable = CLEANER.register(this, nativeHandle);
   }

   @Override
   public float getValue() { return NativeBridge.nativeGetHLAfloat32BE(nativeHandle.require()); }

   @Override
   public HLAfloat32BE setValue(float value) {
      NativeBridge.nativeSetHLAfloat32BE(nativeHandle.require(), value);
      return this;
   }

   @Override
   public int getOctetBoundary() {
      return NativeBridge.nativeHLAfloat32BEOctetBoundary(nativeHandle.require());
   }

   @Override
   public int getEncodedLength() {
      return NativeBridge.nativeHLAfloat32BEEncodedLength(nativeHandle.require());
   }

   @Override
   public byte[] toByteArray() {
      return NativeBridge.nativeHLAfloat32BEToByteArray(nativeHandle.require());
   }

   @Override public void encode(hla.rti1516_2025.encoding.ByteWrapper bytes) {

      NativeDataElementEncoding.encode(this, bytes);

   }

   @Override public hla.rti1516_2025.encoding.ByteWrapper encode() {

      return NativeDataElementEncoding.encode(this);

   }

   @Override public HLAfloat32BE decode(hla.rti1516_2025.encoding.ByteWrapper bytes)

      throws hla.rti1516_2025.encoding.DecoderException {

      return NativeDataElementEncoding.decode(this, bytes);

   }

   @Override public HLAfloat32BE decode(byte[] bytes) {
      NativeBridge.nativeDecodeHLAfloat32BE(nativeHandle.require(), bytes);
      return this;
   }
}
