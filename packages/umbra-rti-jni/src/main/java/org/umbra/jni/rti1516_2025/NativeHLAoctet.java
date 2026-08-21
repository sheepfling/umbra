package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.encoding.HLAoctet;
import java.lang.ref.Cleaner;

/** Java lifetime wrapper for one native C++ {@code HLAoctet}. */
final class NativeHLAoctet implements HLAoctet {
   private static final Cleaner CLEANER = Cleaner.create();

   private static final class NativeHandle implements Runnable {
      private long value;
      NativeHandle(long value) { this.value = value; }
      long require() {
         if (value == 0L) throw new IllegalStateException("Native HLAoctet is closed");
         return value;
      }
      @Override public void run() {
         if (value != 0L) {
            NativeBridge.nativeDestroyHLAoctet(value);
            value = 0L;
         }
      }
   }

   private final NativeHandle nativeHandle;
   @SuppressWarnings("unused") private final Cleaner.Cleanable cleanable;

   NativeHLAoctet(byte value) {
      nativeHandle = new NativeHandle(NativeBridge.nativeCreateHLAoctet(value));
      cleanable = CLEANER.register(this, nativeHandle);
   }

   @Override public byte getValue() { return NativeBridge.nativeGetHLAoctet(nativeHandle.require()); }
   @Override public HLAoctet setValue(byte value) {
      NativeBridge.nativeSetHLAoctet(nativeHandle.require(), value);
      return this;
   }
   @Override public int getOctetBoundary() {
      return NativeBridge.nativeHLAoctetOctetBoundary(nativeHandle.require());
   }
   @Override public int getEncodedLength() {
      return NativeBridge.nativeHLAoctetEncodedLength(nativeHandle.require());
   }
   @Override public byte[] toByteArray() {
      return NativeBridge.nativeHLAoctetToByteArray(nativeHandle.require());
   }
   @Override public void encode(hla.rti1516_2025.encoding.ByteWrapper bytes) {
      NativeDataElementEncoding.encode(this, bytes);
   }
   @Override public hla.rti1516_2025.encoding.ByteWrapper encode() {
      return NativeDataElementEncoding.encode(this);
   }
   @Override public HLAoctet decode(hla.rti1516_2025.encoding.ByteWrapper bytes)
      throws hla.rti1516_2025.encoding.DecoderException {
      return NativeDataElementEncoding.decode(this, bytes);
   }
   @Override public HLAoctet decode(byte[] bytes) {
      NativeBridge.nativeDecodeHLAoctet(nativeHandle.require(), bytes);
      return this;
   }
}
