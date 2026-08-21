package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.encoding.HLAinteger64LE;
import java.lang.ref.Cleaner;

/** Java lifetime wrapper for one native C++ {@code HLAinteger64LE}. */
final class NativeHLAinteger64LE implements HLAinteger64LE {
   private static final Cleaner CLEANER = Cleaner.create();

   private static final class NativeHandle implements Runnable {
      private long value;

      NativeHandle(long value) { this.value = value; }

      long require() {
         if (value == 0L) throw new IllegalStateException("Native HLAinteger64LE is closed");
         return value;
      }

      @Override
      public void run() {
         if (value != 0L) {
            NativeBridge.nativeDestroyHLAinteger64LE(value);
            value = 0L;
         }
      }
   }

   private final NativeHandle nativeHandle;
   @SuppressWarnings("unused")
   private final Cleaner.Cleanable cleanable;

   NativeHLAinteger64LE(long value) {
      nativeHandle = new NativeHandle(NativeBridge.nativeCreateHLAinteger64LE(value));
      cleanable = CLEANER.register(this, nativeHandle);
   }

   @Override
   public long getValue() {
      return NativeBridge.nativeGetHLAinteger64LE(nativeHandle.require());
   }

   @Override
   public HLAinteger64LE setValue(long value) {
      NativeBridge.nativeSetHLAinteger64LE(nativeHandle.require(), value);
      return this;
   }

   @Override
   public int getOctetBoundary() {
      return NativeBridge.nativeHLAinteger64LEOctetBoundary(nativeHandle.require());
   }

   @Override
   public int getEncodedLength() {
      return NativeBridge.nativeHLAinteger64LEEncodedLength(nativeHandle.require());
   }

   @Override
   public byte[] toByteArray() {
      return NativeBridge.nativeHLAinteger64LEToByteArray(nativeHandle.require());
   }

   @Override public void encode(hla.rti1516_2025.encoding.ByteWrapper bytes) {

      NativeDataElementEncoding.encode(this, bytes);

   }

   @Override public hla.rti1516_2025.encoding.ByteWrapper encode() {

      return NativeDataElementEncoding.encode(this);

   }

   @Override public HLAinteger64LE decode(hla.rti1516_2025.encoding.ByteWrapper bytes)

      throws hla.rti1516_2025.encoding.DecoderException {

      return NativeDataElementEncoding.decode(this, bytes);

   }

   @Override public HLAinteger64LE decode(byte[] bytes) {
      NativeBridge.nativeDecodeHLAinteger64LE(nativeHandle.require(), bytes);
      return this;
   }
}
