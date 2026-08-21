package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.encoding.DataElement;
import hla.rti1516_2025.encoding.DataElementFactory;
import hla.rti1516_2025.encoding.HLAfixedArray;
import java.lang.ref.Cleaner;
import java.util.Iterator;
import java.util.NoSuchElementException;

/** Java projection of one native C++ {@code HLAfixedArray}. */
@SuppressWarnings({"rawtypes", "unchecked"})
final class NativeHLAfixedArray implements HLAfixedArray {
   private static final Cleaner CLEANER = Cleaner.create();
   private static final class NativeHandle implements Runnable {
      private long value;
      NativeHandle(long value) { this.value = value; }
      long require() { if (value == 0L) throw new IllegalStateException("Native HLAfixedArray is closed"); return value; }
      @Override public void run() { if (value != 0L) { NativeBridge.nativeDestroyHLAfixedArray(value); value = 0L; } }
   }
   private final DataElementFactory factory;
   private final NativeHandle nativeHandle;
   @SuppressWarnings("unused") private final Cleaner.Cleanable cleanable;

   NativeHLAfixedArray(DataElementFactory factory, int size) {
      this.factory = factory;
      DataElement prototype = (DataElement) factory.createElement(0);
      if (prototype == null) throw new IllegalArgumentException("DataElementFactory returned null prototype");
      nativeHandle = new NativeHandle(NativeBridge.nativeCreateHLAfixedArray(prototype, size));
      cleanable = CLEANER.register(this, nativeHandle);
   }
   @Override public int size() { return NativeBridge.nativeHLAfixedArraySize(nativeHandle.require()); }
   @Override public DataElement get(int index) {
      DataElement value = (DataElement) factory.createElement(index);
      if (value == null) throw new IllegalArgumentException("DataElementFactory returned null element");
      return NativeDataElementEncoding.decode(
         value, NativeBridge.nativeHLAfixedArrayElementEncoding(nativeHandle.require(), index));
   }
   /** Standard mutable fixed-array operation, used by the Python adapter when present. */
   public void set(int index, DataElement value) {
      if (value == null) throw new IllegalArgumentException("HLAfixedArray element must not be null");
      NativeBridge.nativeSetHLAfixedArrayElement(nativeHandle.require(), index, value);
   }
   @Override public Iterator<DataElement> iterator() {
      return new Iterator<DataElement>() { private int index;
         @Override public boolean hasNext() { return index < size(); }
         @Override public DataElement next() { if (!hasNext()) throw new NoSuchElementException(); return get(index++); }
      };
   }
   @Override public int getOctetBoundary() { return NativeBridge.nativeHLAfixedArrayOctetBoundary(nativeHandle.require()); }
   @Override public int getEncodedLength() { return NativeBridge.nativeHLAfixedArrayEncodedLength(nativeHandle.require()); }
   @Override public byte[] toByteArray() { return NativeBridge.nativeHLAfixedArrayToByteArray(nativeHandle.require()); }
   @Override public void encode(hla.rti1516_2025.encoding.ByteWrapper bytes) {
      NativeDataElementEncoding.encode(this, bytes);
   }
   @Override public hla.rti1516_2025.encoding.ByteWrapper encode() {
      return NativeDataElementEncoding.encode(this);
   }
   @Override public HLAfixedArray decode(hla.rti1516_2025.encoding.ByteWrapper bytes)
      throws hla.rti1516_2025.encoding.DecoderException {
      return NativeDataElementEncoding.decode(this, bytes);
   }
   @Override public HLAfixedArray decode(byte[] bytes) { NativeBridge.nativeDecodeHLAfixedArray(nativeHandle.require(), bytes); return this; }

   /** Internal JNI composition hook; not part of the standard Java API. */
   long nativeHandleForComposition() { return nativeHandle.require(); }
}
