package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.encoding.DataElement;
import hla.rti1516_2025.encoding.HLAfixedRecord;
import java.lang.ref.Cleaner;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.NoSuchElementException;

/** Java projection of one native C++ {@code HLAfixedRecord}. */
@SuppressWarnings({"rawtypes", "unchecked"})
final class NativeHLAfixedRecord implements HLAfixedRecord {
   private static final Cleaner CLEANER = Cleaner.create();

   private static final class NativeHandle implements Runnable {
      private long value;
      NativeHandle(long value) { this.value = value; }
      long require() {
         if (value == 0L) throw new IllegalStateException("Native HLAfixedRecord is closed");
         return value;
      }
      @Override public void run() {
         if (value != 0L) {
            NativeBridge.nativeDestroyHLAfixedRecord(value);
            value = 0L;
         }
      }
   }

   private final ArrayList<DataElement> elements = new ArrayList<DataElement>();
   private final NativeHandle nativeHandle;
   @SuppressWarnings("unused") private final Cleaner.Cleanable cleanable;

   NativeHLAfixedRecord() {
      nativeHandle = new NativeHandle(NativeBridge.nativeCreateHLAfixedRecord());
      cleanable = CLEANER.register(this, nativeHandle);
   }

   @Override public void add(DataElement dataElement) {
      if (dataElement == null) throw new IllegalArgumentException("HLAfixedRecord element must not be null");
      NativeBridge.nativeAppendHLAfixedRecordElement(nativeHandle.require(), dataElement);
      elements.add(dataElement);
   }

   /** Compatibility spelling retained for older Python callers. */
   public void appendElement(DataElement dataElement) { add(dataElement); }

   @Override public int size() { return NativeBridge.nativeHLAfixedRecordSize(nativeHandle.require()); }

   @Override public DataElement get(int index) {
      if (index < 0 || index >= elements.size()) throw new IndexOutOfBoundsException("HLAfixedRecord index is out of range");
      DataElement value = elements.get(index);
      return NativeDataElementEncoding.decode(
         value, NativeBridge.nativeHLAfixedRecordElementEncoding(nativeHandle.require(), index));
   }

   /** Standard mutable record operation, used by the Python adapter when present. */
   public void set(int index, DataElement value) {
      if (value == null) throw new IllegalArgumentException("HLAfixedRecord element must not be null");
      if (index < 0 || index >= elements.size()) throw new IndexOutOfBoundsException("HLAfixedRecord index is out of range");
      NativeBridge.nativeSetHLAfixedRecordElement(nativeHandle.require(), index, value);
      elements.set(index, value);
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
      return NativeBridge.nativeHLAfixedRecordOctetBoundary(nativeHandle.require());
   }
   @Override public int getEncodedLength() {
      return NativeBridge.nativeHLAfixedRecordEncodedLength(nativeHandle.require());
   }
   @Override public byte[] toByteArray() {
      return NativeBridge.nativeHLAfixedRecordToByteArray(nativeHandle.require());
   }
   @Override public void encode(hla.rti1516_2025.encoding.ByteWrapper bytes) {
      NativeDataElementEncoding.encode(this, bytes);
   }
   @Override public hla.rti1516_2025.encoding.ByteWrapper encode() {
      return NativeDataElementEncoding.encode(this);
   }
   @Override public HLAfixedRecord decode(hla.rti1516_2025.encoding.ByteWrapper bytes)
      throws hla.rti1516_2025.encoding.DecoderException {
      return NativeDataElementEncoding.decode(this, bytes);
   }
   @Override public HLAfixedRecord decode(byte[] bytes) {
      NativeBridge.nativeDecodeHLAfixedRecord(nativeHandle.require(), bytes);
      return this;
   }

   /** Internal JNI composition hook; not part of the standard Java API. */
   long nativeHandleForComposition() { return nativeHandle.require(); }
}
