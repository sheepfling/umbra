package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.encoding.HLAfloat64BE;
import java.lang.ref.Cleaner;

/** Java lifetime wrapper for one native C++ {@code HLAfloat64BE}. */
final class NativeHLAfloat64BE implements HLAfloat64BE {
   private static final Cleaner CLEANER = Cleaner.create();

   private static final class NativeHandle implements Runnable {
      private long value;

      NativeHandle(long value) { this.value = value; }

      long require() {
         if (value == 0L) throw new IllegalStateException("Native HLAfloat64BE is closed");
         return value;
      }

      @Override
      public void run() {
         if (value != 0L) {
            NativeBridge.nativeDestroyHLAfloat64BE(value);
            value = 0L;
         }
      }
   }

   private final NativeHandle nativeHandle;
   @SuppressWarnings("unused")
   private final Cleaner.Cleanable cleanable;

   NativeHLAfloat64BE(double value) {
      nativeHandle = new NativeHandle(NativeBridge.nativeCreateHLAfloat64BE(value));
      cleanable = CLEANER.register(this, nativeHandle);
   }

   @Override
   public double getValue() { return NativeBridge.nativeGetHLAfloat64BE(nativeHandle.require()); }

   @Override
   public HLAfloat64BE setValue(double value) {
      NativeBridge.nativeSetHLAfloat64BE(nativeHandle.require(), value);
      return this;
   }

   @Override
   public int getOctetBoundary() {
      return NativeBridge.nativeHLAfloat64BEOctetBoundary(nativeHandle.require());
   }

   @Override
   public int getEncodedLength() {
      return NativeBridge.nativeHLAfloat64BEEncodedLength(nativeHandle.require());
   }

   @Override
   public byte[] toByteArray() {
      return NativeBridge.nativeHLAfloat64BEToByteArray(nativeHandle.require());
   }

   @Override public void encode(hla.rti1516_2025.encoding.ByteWrapper bytes) {

      NativeDataElementEncoding.encode(this, bytes);

   }

   @Override public hla.rti1516_2025.encoding.ByteWrapper encode() {

      return NativeDataElementEncoding.encode(this);

   }

   @Override public HLAfloat64BE decode(hla.rti1516_2025.encoding.ByteWrapper bytes)

      throws hla.rti1516_2025.encoding.DecoderException {

      return NativeDataElementEncoding.decode(this, bytes);

   }

   @Override public HLAfloat64BE decode(byte[] bytes) {
      NativeBridge.nativeDecodeHLAfloat64BE(nativeHandle.require(), bytes);
      return this;
   }
}
