package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.encoding.HLAopaqueData;
import java.lang.ref.Cleaner;
import java.util.Iterator;

/** Java lifetime wrapper for one native C++ {@code HLAopaqueData}. */
final class NativeHLAopaqueData implements HLAopaqueData {
   private static final Cleaner CLEANER = Cleaner.create();

   private static final class NativeHandle implements Runnable {
      private long value;
      NativeHandle(long value) { this.value = value; }
      long require() {
         if (value == 0L) throw new IllegalStateException("Native HLAopaqueData is closed");
         return value;
      }
      @Override public void run() {
         if (value != 0L) {
            NativeBridge.nativeDestroyHLAopaqueData(value);
            value = 0L;
         }
      }
   }

   private final NativeHandle nativeHandle;
   @SuppressWarnings("unused") private final Cleaner.Cleanable cleanable;

   NativeHLAopaqueData(byte[] value) {
      nativeHandle = new NativeHandle(NativeBridge.nativeCreateHLAopaqueData(value));
      cleanable = CLEANER.register(this, nativeHandle);
   }

   @Override public int size() { return NativeBridge.nativeHLAopaqueDataSize(nativeHandle.require()); }
   @Override public byte get(int index) {
      return NativeBridge.nativeGetHLAopaqueData(nativeHandle.require(), index);
   }
   @Override public byte[] getValue() {
      return NativeBridge.nativeGetHLAopaqueDataValue(nativeHandle.require());
   }
   @Override public HLAopaqueData setValue(byte[] value) {
      NativeBridge.nativeSetHLAopaqueData(nativeHandle.require(), value);
      return this;
   }
   @Override public Iterator<Byte> iterator() {
      byte[] snapshot = getValue();
      return new Iterator<Byte>() {
         private int index;
         @Override public boolean hasNext() { return index < snapshot.length; }
         @Override public Byte next() { return Byte.valueOf(snapshot[index++]); }
      };
   }
   @Override public int getOctetBoundary() {
      return NativeBridge.nativeHLAopaqueDataOctetBoundary(nativeHandle.require());
   }
   @Override public int getEncodedLength() {
      return NativeBridge.nativeHLAopaqueDataEncodedLength(nativeHandle.require());
   }
   @Override public byte[] toByteArray() {
      return NativeBridge.nativeHLAopaqueDataToByteArray(nativeHandle.require());
   }
   @Override public void encode(hla.rti1516_2025.encoding.ByteWrapper bytes) {
      NativeDataElementEncoding.encode(this, bytes);
   }
   @Override public hla.rti1516_2025.encoding.ByteWrapper encode() {
      return NativeDataElementEncoding.encode(this);
   }
   @Override public HLAopaqueData decode(hla.rti1516_2025.encoding.ByteWrapper bytes)
      throws hla.rti1516_2025.encoding.DecoderException {
      return NativeDataElementEncoding.decode(this, bytes);
   }
   @Override public HLAopaqueData decode(byte[] bytes) {
      NativeBridge.nativeDecodeHLAopaqueData(nativeHandle.require(), bytes);
      return this;
   }
}
