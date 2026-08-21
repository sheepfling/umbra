package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.encoding.HLAinteger32BE;
import java.lang.ref.Cleaner;

/** Java lifetime wrapper for one native C++ {@code HLAinteger32BE}. */
final class NativeHLAinteger32BE implements HLAinteger32BE {
   private static final Cleaner CLEANER = Cleaner.create();

   private static final class NativeHandle implements Runnable {
      private long value;

      NativeHandle(long value) {
         this.value = value;
      }

      long require() {
         if (value == 0L) throw new IllegalStateException("Native HLAinteger32BE is closed");
         return value;
      }

      @Override
      public void run() {
         if (value != 0L) {
            NativeBridge.nativeDestroyHLAinteger32BE(value);
            value = 0L;
         }
      }
   }

   private final NativeHandle nativeHandle;
   @SuppressWarnings("unused")
   private final Cleaner.Cleanable cleanable;

   NativeHLAinteger32BE(int value) {
      nativeHandle = new NativeHandle(NativeBridge.nativeCreateHLAinteger32BE(value));
      cleanable = CLEANER.register(this, nativeHandle);
   }

   @Override
   public int getValue() {
      return NativeBridge.nativeGetHLAinteger32BE(nativeHandle.require());
   }

   @Override
   public HLAinteger32BE setValue(int value) {
      NativeBridge.nativeSetHLAinteger32BE(nativeHandle.require(), value);
      return this;
   }

   @Override
   public int getOctetBoundary() {
      return NativeBridge.nativeHLAinteger32BEOctetBoundary(nativeHandle.require());
   }

   @Override
   public int getEncodedLength() {
      return NativeBridge.nativeHLAinteger32BEEncodedLength(nativeHandle.require());
   }

   @Override
   public byte[] toByteArray() {
      return NativeBridge.nativeHLAinteger32BEToByteArray(nativeHandle.require());
   }

   @Override public void encode(hla.rti1516_2025.encoding.ByteWrapper bytes) {

      NativeDataElementEncoding.encode(this, bytes);

   }

   @Override public hla.rti1516_2025.encoding.ByteWrapper encode() {

      return NativeDataElementEncoding.encode(this);

   }

   @Override public HLAinteger32BE decode(hla.rti1516_2025.encoding.ByteWrapper bytes)

      throws hla.rti1516_2025.encoding.DecoderException {

      return NativeDataElementEncoding.decode(this, bytes);

   }

   @Override public HLAinteger32BE decode(byte[] bytes) {
      NativeBridge.nativeDecodeHLAinteger32BE(nativeHandle.require(), bytes);
      return this;
   }
}
