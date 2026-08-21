package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.encoding.HLAfloat64LE;
import java.lang.ref.Cleaner;

/** Java lifetime wrapper for one native C++ {@code HLAfloat64LE}. */
final class NativeHLAfloat64LE implements HLAfloat64LE {
   private static final Cleaner CLEANER = Cleaner.create();

   private static final class NativeHandle implements Runnable {
      private long value;

      NativeHandle(long value) { this.value = value; }

      long require() {
         if (value == 0L) throw new IllegalStateException("Native HLAfloat64LE is closed");
         return value;
      }

      @Override
      public void run() {
         if (value != 0L) {
            NativeBridge.nativeDestroyHLAfloat64LE(value);
            value = 0L;
         }
      }
   }

   private final NativeHandle nativeHandle;
   @SuppressWarnings("unused")
   private final Cleaner.Cleanable cleanable;

   NativeHLAfloat64LE(double value) {
      nativeHandle = new NativeHandle(NativeBridge.nativeCreateHLAfloat64LE(value));
      cleanable = CLEANER.register(this, nativeHandle);
   }

   @Override
   public double getValue() { return NativeBridge.nativeGetHLAfloat64LE(nativeHandle.require()); }

   @Override
   public HLAfloat64LE setValue(double value) {
      NativeBridge.nativeSetHLAfloat64LE(nativeHandle.require(), value);
      return this;
   }

   @Override
   public int getOctetBoundary() {
      return NativeBridge.nativeHLAfloat64LEOctetBoundary(nativeHandle.require());
   }

   @Override
   public int getEncodedLength() {
      return NativeBridge.nativeHLAfloat64LEEncodedLength(nativeHandle.require());
   }

   @Override
   public byte[] toByteArray() {
      return NativeBridge.nativeHLAfloat64LEToByteArray(nativeHandle.require());
   }

   @Override public void encode(hla.rti1516_2025.encoding.ByteWrapper bytes) {

      NativeDataElementEncoding.encode(this, bytes);

   }

   @Override public hla.rti1516_2025.encoding.ByteWrapper encode() {

      return NativeDataElementEncoding.encode(this);

   }

   @Override public HLAfloat64LE decode(hla.rti1516_2025.encoding.ByteWrapper bytes)

      throws hla.rti1516_2025.encoding.DecoderException {

      return NativeDataElementEncoding.decode(this, bytes);

   }

   @Override public HLAfloat64LE decode(byte[] bytes) {
      NativeBridge.nativeDecodeHLAfloat64LE(nativeHandle.require(), bytes);
      return this;
   }
}
