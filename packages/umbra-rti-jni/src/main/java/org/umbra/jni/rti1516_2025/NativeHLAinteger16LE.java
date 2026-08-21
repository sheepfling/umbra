package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.encoding.HLAinteger16LE;
import java.lang.ref.Cleaner;

/** Java lifetime wrapper for one native C++ {@code HLAinteger16LE}. */
final class NativeHLAinteger16LE implements HLAinteger16LE {
   private static final Cleaner CLEANER = Cleaner.create();

   private static final class NativeHandle implements Runnable {
      private long value;

      NativeHandle(long value) { this.value = value; }

      long require() {
         if (value == 0L) throw new IllegalStateException("Native HLAinteger16LE is closed");
         return value;
      }

      @Override
      public void run() {
         if (value != 0L) {
            NativeBridge.nativeDestroyHLAinteger16LE(value);
            value = 0L;
         }
      }
   }

   private final NativeHandle nativeHandle;
   @SuppressWarnings("unused")
   private final Cleaner.Cleanable cleanable;

   NativeHLAinteger16LE(short value) {
      nativeHandle = new NativeHandle(NativeBridge.nativeCreateHLAinteger16LE(value));
      cleanable = CLEANER.register(this, nativeHandle);
   }

   @Override
   public short getValue() {
      return NativeBridge.nativeGetHLAinteger16LE(nativeHandle.require());
   }

   @Override
   public HLAinteger16LE setValue(short value) {
      NativeBridge.nativeSetHLAinteger16LE(nativeHandle.require(), value);
      return this;
   }

   @Override
   public int getOctetBoundary() {
      return NativeBridge.nativeHLAinteger16LEOctetBoundary(nativeHandle.require());
   }

   @Override
   public int getEncodedLength() {
      return NativeBridge.nativeHLAinteger16LEEncodedLength(nativeHandle.require());
   }

   @Override
   public byte[] toByteArray() {
      return NativeBridge.nativeHLAinteger16LEToByteArray(nativeHandle.require());
   }

   @Override public void encode(hla.rti1516_2025.encoding.ByteWrapper bytes) {

      NativeDataElementEncoding.encode(this, bytes);

   }

   @Override public hla.rti1516_2025.encoding.ByteWrapper encode() {

      return NativeDataElementEncoding.encode(this);

   }

   @Override public HLAinteger16LE decode(hla.rti1516_2025.encoding.ByteWrapper bytes)

      throws hla.rti1516_2025.encoding.DecoderException {

      return NativeDataElementEncoding.decode(this, bytes);

   }

   @Override public HLAinteger16LE decode(byte[] bytes) {
      NativeBridge.nativeDecodeHLAinteger16LE(nativeHandle.require(), bytes);
      return this;
   }
}
