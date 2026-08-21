package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.encoding.HLAinteger32LE;
import java.lang.ref.Cleaner;

/** Java lifetime wrapper for one native C++ {@code HLAinteger32LE}. */
final class NativeHLAinteger32LE implements HLAinteger32LE {
   private static final Cleaner CLEANER = Cleaner.create();

   private static final class NativeHandle implements Runnable {
      private long value;

      NativeHandle(long value) {
         this.value = value;
      }

      long require() {
         if (value == 0L) throw new IllegalStateException("Native HLAinteger32LE is closed");
         return value;
      }

      @Override
      public void run() {
         if (value != 0L) {
            NativeBridge.nativeDestroyHLAinteger32LE(value);
            value = 0L;
         }
      }
   }

   private final NativeHandle nativeHandle;
   @SuppressWarnings("unused")
   private final Cleaner.Cleanable cleanable;

   NativeHLAinteger32LE(int value) {
      nativeHandle = new NativeHandle(NativeBridge.nativeCreateHLAinteger32LE(value));
      cleanable = CLEANER.register(this, nativeHandle);
   }

   @Override
   public int getValue() {
      return NativeBridge.nativeGetHLAinteger32LE(nativeHandle.require());
   }

   @Override
   public HLAinteger32LE setValue(int value) {
      NativeBridge.nativeSetHLAinteger32LE(nativeHandle.require(), value);
      return this;
   }

   @Override
   public int getOctetBoundary() {
      return NativeBridge.nativeHLAinteger32LEOctetBoundary(nativeHandle.require());
   }

   @Override
   public int getEncodedLength() {
      return NativeBridge.nativeHLAinteger32LEEncodedLength(nativeHandle.require());
   }

   @Override
   public byte[] toByteArray() {
      return NativeBridge.nativeHLAinteger32LEToByteArray(nativeHandle.require());
   }

   @Override public void encode(hla.rti1516_2025.encoding.ByteWrapper bytes) {

      NativeDataElementEncoding.encode(this, bytes);

   }

   @Override public hla.rti1516_2025.encoding.ByteWrapper encode() {

      return NativeDataElementEncoding.encode(this);

   }

   @Override public HLAinteger32LE decode(hla.rti1516_2025.encoding.ByteWrapper bytes)

      throws hla.rti1516_2025.encoding.DecoderException {

      return NativeDataElementEncoding.decode(this, bytes);

   }

   @Override public HLAinteger32LE decode(byte[] bytes) {
      NativeBridge.nativeDecodeHLAinteger32LE(nativeHandle.require(), bytes);
      return this;
   }
}
