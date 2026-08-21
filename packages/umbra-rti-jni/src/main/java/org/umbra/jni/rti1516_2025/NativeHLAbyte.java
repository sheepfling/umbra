package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.encoding.HLAbyte;
import java.lang.ref.Cleaner;

/** Java lifetime wrapper for one native C++ {@code HLAbyte}. */
final class NativeHLAbyte implements HLAbyte {
   private static final Cleaner CLEANER = Cleaner.create();

   private static final class NativeHandle implements Runnable {
      private long value;
      NativeHandle(long value) { this.value = value; }
      long require() {
         if (value == 0L) throw new IllegalStateException("Native HLAbyte is closed");
         return value;
      }
      @Override public void run() {
         if (value != 0L) {
            NativeBridge.nativeDestroyHLAbyte(value);
            value = 0L;
         }
      }
   }

   private final NativeHandle nativeHandle;
   @SuppressWarnings("unused") private final Cleaner.Cleanable cleanable;

   NativeHLAbyte(byte value) {
      nativeHandle = new NativeHandle(NativeBridge.nativeCreateHLAbyte(value));
      cleanable = CLEANER.register(this, nativeHandle);
   }

   @Override public byte getValue() { return NativeBridge.nativeGetHLAbyte(nativeHandle.require()); }
   @Override public HLAbyte setValue(byte value) {
      NativeBridge.nativeSetHLAbyte(nativeHandle.require(), value);
      return this;
   }
   @Override public int getOctetBoundary() {
      return NativeBridge.nativeHLAbyteOctetBoundary(nativeHandle.require());
   }
   @Override public int getEncodedLength() {
      return NativeBridge.nativeHLAbyteEncodedLength(nativeHandle.require());
   }
   @Override public byte[] toByteArray() {
      return NativeBridge.nativeHLAbyteToByteArray(nativeHandle.require());
   }
   @Override public void encode(hla.rti1516_2025.encoding.ByteWrapper bytes) {
      NativeDataElementEncoding.encode(this, bytes);
   }
   @Override public hla.rti1516_2025.encoding.ByteWrapper encode() {
      return NativeDataElementEncoding.encode(this);
   }
   @Override public HLAbyte decode(hla.rti1516_2025.encoding.ByteWrapper bytes)
      throws hla.rti1516_2025.encoding.DecoderException {
      return NativeDataElementEncoding.decode(this, bytes);
   }
   @Override public HLAbyte decode(byte[] bytes) {
      NativeBridge.nativeDecodeHLAbyte(nativeHandle.require(), bytes);
      return this;
   }
}
