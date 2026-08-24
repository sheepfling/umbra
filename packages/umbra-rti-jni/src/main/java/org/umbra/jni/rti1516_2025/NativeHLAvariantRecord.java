package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.encoding.DataElement;
import hla.rti1516_2025.encoding.DecoderException;
import hla.rti1516_2025.encoding.HLAvariantRecord;
import java.lang.ref.Cleaner;
import java.util.Base64;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.Map;

/** Java projection of one native C++ {@code HLAvariantRecord}. */
@SuppressWarnings({"rawtypes", "unchecked"})
final class NativeHLAvariantRecord implements HLAvariantRecord {
   private static final Cleaner CLEANER = Cleaner.create();

   private static final class NativeHandle implements Runnable {
      private long value;
      NativeHandle(long value) { this.value = value; }
      long require() {
         if (value == 0L) throw new IllegalStateException("Native HLAvariantRecord is closed");
         return value;
      }
      @Override public void run() {
         if (value != 0L) {
            NativeBridge.nativeDestroyHLAvariantRecord(value);
            value = 0L;
         }
      }
   }

   private static String key(DataElement discriminant) {
      return Base64.getEncoder().encodeToString(discriminant.toByteArray());
   }

   private final DataElement discriminant;
   private final Map<String, DataElement> variants = new LinkedHashMap<String, DataElement>();
   private final NativeHandle nativeHandle;
   @SuppressWarnings("unused") private final Cleaner.Cleanable cleanable;

   NativeHLAvariantRecord(DataElement discriminantPrototype) {
      if (discriminantPrototype == null) {
         throw new IllegalArgumentException("HLAvariantRecord discriminant prototype must not be null");
      }
      discriminant = discriminantPrototype;
      nativeHandle = new NativeHandle(
         NativeBridge.nativeCreateHLAvariantRecord(discriminantPrototype));
      cleanable = CLEANER.register(this, nativeHandle);
   }

   @Override public HLAvariantRecord setVariant(DataElement discriminantValue, DataElement value) {
      if (discriminantValue == null || value == null) {
         throw new IllegalArgumentException("HLAvariantRecord discriminant and value must not be null");
      }
      NativeBridge.nativeSetHLAvariantRecordVariant(
         nativeHandle.require(), discriminantValue, value);
      variants.put(key(discriminantValue), value);
      return this;
   }

   @Override public HLAvariantRecord setDiscriminant(DataElement discriminantValue) {
      if (discriminantValue == null) {
         throw new IllegalArgumentException("HLAvariantRecord discriminant must not be null");
      }
      NativeBridge.nativeSetHLAvariantRecordDiscriminant(nativeHandle.require(), discriminantValue);
      return this;
   }

   @Override public DataElement getDiscriminant() {
      return NativeDataElementEncoding.decode(
         discriminant,
         NativeBridge.nativeHLAvariantRecordDiscriminantEncoding(nativeHandle.require()));
   }

   @Override public DataElement getValue() {
      DataElement value = variants.get(key(getDiscriminant()));
      if (value == null) return null;
      return NativeDataElementEncoding.decode(
         value, NativeBridge.nativeHLAvariantRecordValueEncoding(nativeHandle.require()));
   }

   @Override public int getOctetBoundary() {
      return NativeBridge.nativeHLAvariantRecordOctetBoundary(nativeHandle.require());
   }
   @Override public int getEncodedLength() {
      return NativeBridge.nativeHLAvariantRecordEncodedLength(nativeHandle.require());
   }
   @Override public byte[] toByteArray() {
      return NativeBridge.nativeHLAvariantRecordToByteArray(nativeHandle.require());
   }
   @Override public void encode(hla.rti1516_2025.encoding.ByteWrapper bytes) {
      NativeDataElementEncoding.encode(this, bytes);
   }
   @Override public hla.rti1516_2025.encoding.ByteWrapper encode() {
      return NativeDataElementEncoding.encode(this);
   }
   @Override public HLAvariantRecord decode(hla.rti1516_2025.encoding.ByteWrapper bytes)
      throws hla.rti1516_2025.encoding.DecoderException {
      if (bytes == null) throw new DecoderException("ByteWrapper must not be null");
      int offset = bytes.getPos();
      int remaining = bytes.remaining();
      int discriminantLength = discriminant.getEncodedLength();
      if (remaining < discriminantLength) {
         throw new DecoderException("HLAvariantRecord discriminant is truncated");
      }

      byte[] source = bytes.array();
      byte[] discriminantBytes = Arrays.copyOfRange(
         source, offset, offset + discriminantLength);
      // Decode the fixed-width discriminant independently, then project that
      // value into the native record so C++ selects the mapped alternative and
      // computes the standard alignment/encoded width.
      NativeDataElementEncoding.decode(
         discriminant, new hla.rti1516_2025.encoding.ByteWrapper(discriminantBytes));
      setDiscriminant(discriminant);
      DataElement mapped = variants.get(key(discriminant));
      int consumed = mapped == null
         ? discriminantLength
         : NativeBridge.nativeHLAvariantRecordEncodedLength(nativeHandle.require());
      if (remaining < consumed) {
         throw new DecoderException("HLAvariantRecord alternative is truncated");
      }
      byte[] encoded = Arrays.copyOfRange(source, offset, offset + consumed);
      NativeBridge.nativeDecodeHLAvariantRecord(nativeHandle.require(), encoded);
      bytes.advance(consumed);
      return this;
   }
   @Override public HLAvariantRecord decode(byte[] bytes) {
      NativeBridge.nativeDecodeHLAvariantRecord(nativeHandle.require(), bytes);
      return this;
   }

   /** Internal JNI composition hook; not part of the standard Java API. */
   long nativeHandleForComposition() { return nativeHandle.require(); }
}
