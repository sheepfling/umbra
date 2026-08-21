package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.encoding.HLAinteger64BE;
import java.lang.ref.Cleaner;

/** Java lifetime wrapper for one native C++ {@code HLAinteger64BE}. */
final class NativeHLAinteger64BE implements HLAinteger64BE {
   private static final Cleaner CLEANER = Cleaner.create();

   private static final class NativeHandle implements Runnable {
      private long value;

      NativeHandle(long value) { this.value = value; }

      long require() {
         if (value == 0L) throw new IllegalStateException("Native HLAinteger64BE is closed");
         return value;
      }

      @Override
      public void run() {
         if (value != 0L) {
            NativeBridge.nativeDestroyHLAinteger64BE(value);
            value = 0L;
         }
      }
   }

   private final NativeHandle nativeHandle;
   @SuppressWarnings("unused")
   private final Cleaner.Cleanable cleanable;

   NativeHLAinteger64BE(long value) {
      nativeHandle = new NativeHandle(NativeBridge.nativeCreateHLAinteger64BE(value));
      cleanable = CLEANER.register(this, nativeHandle);
   }

   @Override
   public long getValue() {
      return NativeBridge.nativeGetHLAinteger64BE(nativeHandle.require());
   }

   @Override
   public HLAinteger64BE setValue(long value) {
      NativeBridge.nativeSetHLAinteger64BE(nativeHandle.require(), value);
      return this;
   }

   @Override
   public int getOctetBoundary() {
      return NativeBridge.nativeHLAinteger64BEOctetBoundary(nativeHandle.require());
   }

   @Override
   public int getEncodedLength() {
      return NativeBridge.nativeHLAinteger64BEEncodedLength(nativeHandle.require());
   }

   @Override
   public byte[] toByteArray() {
      return NativeBridge.nativeHLAinteger64BEToByteArray(nativeHandle.require());
   }

   @Override public void encode(hla.rti1516_2025.encoding.ByteWrapper bytes) {

      NativeDataElementEncoding.encode(this, bytes);

   }

   @Override public hla.rti1516_2025.encoding.ByteWrapper encode() {

      return NativeDataElementEncoding.encode(this);

   }

   @Override public HLAinteger64BE decode(hla.rti1516_2025.encoding.ByteWrapper bytes)

      throws hla.rti1516_2025.encoding.DecoderException {

      return NativeDataElementEncoding.decode(this, bytes);

   }

   @Override public HLAinteger64BE decode(byte[] bytes) {
      NativeBridge.nativeDecodeHLAinteger64BE(nativeHandle.require(), bytes);
      return this;
   }
}
