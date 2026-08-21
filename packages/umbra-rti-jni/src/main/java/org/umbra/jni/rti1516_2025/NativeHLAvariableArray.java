package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.encoding.DataElement;
import hla.rti1516_2025.encoding.DataElementFactory;
import hla.rti1516_2025.encoding.HLAvariableArray;
import java.lang.ref.Cleaner;
import java.util.Iterator;
import java.util.NoSuchElementException;

/** Java projection of one native C++ {@code HLAvariableArray}. */
@SuppressWarnings({"rawtypes", "unchecked"})
final class NativeHLAvariableArray implements HLAvariableArray {
   private static final Cleaner CLEANER = Cleaner.create();

   private static final class NativeHandle implements Runnable {
      private long value;
      NativeHandle(long value) { this.value = value; }
      long require() {
         if (value == 0L) throw new IllegalStateException("Native HLAvariableArray is closed");
         return value;
      }
      @Override public void run() {
         if (value != 0L) {
            NativeBridge.nativeDestroyHLAvariableArray(value);
            value = 0L;
         }
      }
   }

   private final DataElementFactory factory;
   private final NativeHandle nativeHandle;
   @SuppressWarnings("unused") private final Cleaner.Cleanable cleanable;

   NativeHLAvariableArray(DataElementFactory factory, DataElement[] initialElements) {
      this.factory = factory;
      DataElement prototype = (DataElement) factory.createElement(0);
      if (prototype == null) throw new IllegalArgumentException("DataElementFactory returned null prototype");
      nativeHandle = new NativeHandle(NativeBridge.nativeCreateHLAvariableArray(prototype));
      cleanable = CLEANER.register(this, nativeHandle);
      for (DataElement element : initialElements) addElement(element);
   }

   @Override public HLAvariableArray addElement(DataElement dataElement) {
      if (dataElement == null) throw new IllegalArgumentException("HLAvariableArray element must not be null");
      NativeBridge.nativeAddHLAvariableArrayElement(nativeHandle.require(), dataElement);
      return this;
   }

   @Override public HLAvariableArray resize(int size) {
      if (size < 0) throw new IllegalArgumentException("HLAvariableArray size must not be negative");
      NativeBridge.nativeResizeHLAvariableArray(nativeHandle.require(), size);
      return this;
   }

   @Override public int size() {
      return NativeBridge.nativeHLAvariableArraySize(nativeHandle.require());
   }

   @Override public DataElement get(int index) {
      DataElement value = (DataElement) factory.createElement(index);
      if (value == null) throw new IllegalArgumentException("DataElementFactory returned null element");
      return NativeDataElementEncoding.decode(
         value, NativeBridge.nativeHLAvariableArrayElementEncoding(nativeHandle.require(), index));
   }

   @Override public Iterator<DataElement> iterator() {
      return new Iterator<DataElement>() {
         private int index;

         @Override public boolean hasNext() { return index < size(); }

         @Override public DataElement next() {
            if (!hasNext()) throw new NoSuchElementException();
            return get(index++);
         }
      };
   }

   @Override public int getOctetBoundary() {
      return NativeBridge.nativeHLAvariableArrayOctetBoundary(nativeHandle.require());
   }
   @Override public int getEncodedLength() {
      return NativeBridge.nativeHLAvariableArrayEncodedLength(nativeHandle.require());
   }
   @Override public byte[] toByteArray() {
      return NativeBridge.nativeHLAvariableArrayToByteArray(nativeHandle.require());
   }
   @Override public void encode(hla.rti1516_2025.encoding.ByteWrapper bytes) {
      NativeDataElementEncoding.encode(this, bytes);
   }
   @Override public hla.rti1516_2025.encoding.ByteWrapper encode() {
      return NativeDataElementEncoding.encode(this);
   }
   @Override public HLAvariableArray decode(hla.rti1516_2025.encoding.ByteWrapper bytes)
      throws hla.rti1516_2025.encoding.DecoderException {
      return NativeDataElementEncoding.decode(this, bytes);
   }
   @Override public HLAvariableArray decode(byte[] bytes) {
      NativeBridge.nativeDecodeHLAvariableArray(nativeHandle.require(), bytes);
      return this;
   }

   /** Internal JNI composition hook; not part of the standard Java API. */
   long nativeHandleForComposition() { return nativeHandle.require(); }
}
