package org.umbra.testfixture.rti1516_2025;

import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.DecoderException;
import hla.rti1516_2025.encoding.EncoderException;
import hla.rti1516_2025.encoding.HLAboolean;
import hla.rti1516_2025.encoding.HLAbyte;
import hla.rti1516_2025.encoding.HLAASCIIchar;
import hla.rti1516_2025.encoding.HLAASCIIstring;
import hla.rti1516_2025.encoding.HLAfloat64BE;
import hla.rti1516_2025.encoding.HLAfloat64LE;
import hla.rti1516_2025.encoding.HLAfloat32BE;
import hla.rti1516_2025.encoding.HLAfloat32LE;
import hla.rti1516_2025.encoding.HLAinteger16BE;
import hla.rti1516_2025.encoding.HLAinteger16LE;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.encoding.HLAinteger32LE;
import hla.rti1516_2025.encoding.HLAinteger64BE;
import hla.rti1516_2025.encoding.HLAinteger64LE;
import hla.rti1516_2025.encoding.HLAunicodeString;
import hla.rti1516_2025.encoding.HLAunsignedInteger32BE;
import hla.rti1516_2025.encoding.HLAunsignedInteger16BE;
import hla.rti1516_2025.encoding.HLAunsignedInteger16LE;
import hla.rti1516_2025.encoding.HLAunsignedInteger32LE;
import hla.rti1516_2025.encoding.HLAunsignedInteger64BE;
import hla.rti1516_2025.encoding.HLAunsignedInteger64LE;
import hla.rti1516_2025.encoding.HLAoctet;
import hla.rti1516_2025.encoding.HLAoctetPairBE;
import hla.rti1516_2025.encoding.HLAoctetPairLE;
import hla.rti1516_2025.encoding.HLAopaqueData;
import hla.rti1516_2025.encoding.DataElement;
import hla.rti1516_2025.encoding.DataElementFactory;
import hla.rti1516_2025.encoding.HLAvariableArray;
import hla.rti1516_2025.encoding.HLAfixedArray;
import hla.rti1516_2025.encoding.HLAfixedRecord;
import hla.rti1516_2025.encoding.HLAvariantRecord;
import hla.rti1516_2025.encoding.HLAunicodeChar;
import java.io.ByteArrayOutputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

/**
 * Small real Java implementation of the scalar basic encodings Umbra C++ also
 * implements. It lets the JPype test prove factory dispatch and byte copying.
 */
public final class MockEncoderFactory implements EncoderFactory {
   @Override
   public HLAinteger16BE createHLAinteger16BE() { return new Integer16Element(); }

   @Override
   public HLAinteger16BE createHLAinteger16BE(short value) { return new Integer16Element(value); }

   @Override
   public HLAinteger16LE createHLAinteger16LE() { return new Integer16LittleEndianElement(); }

   @Override
   public HLAinteger16LE createHLAinteger16LE(short value) {
      return new Integer16LittleEndianElement(value);
   }

   @Override
   public HLAinteger32LE createHLAinteger32LE() { return new Integer32LittleEndianElement(); }

   @Override
   public HLAinteger32LE createHLAinteger32LE(int value) {
      return new Integer32LittleEndianElement(value);
   }

   @Override
   public HLAinteger64BE createHLAinteger64BE() { return new Integer64Element(); }

   @Override
   public HLAinteger64BE createHLAinteger64BE(long value) { return new Integer64Element(value); }

   @Override
   public HLAinteger64LE createHLAinteger64LE() { return new Integer64LittleEndianElement(); }

   @Override
   public HLAinteger64LE createHLAinteger64LE(long value) {
      return new Integer64LittleEndianElement(value);
   }

   @Override
   public HLAfloat64BE createHLAfloat64BE() { return new Float64Element(); }

   @Override
   public HLAfloat64BE createHLAfloat64BE(double value) { return new Float64Element(value); }

   @Override
   public HLAfloat32BE createHLAfloat32BE() { return new Float32Element(); }

   @Override
   public HLAfloat32BE createHLAfloat32BE(float value) { return new Float32Element(value); }

   @Override
   public HLAfloat64LE createHLAfloat64LE() { return new Float64LittleEndianElement(); }

   @Override
   public HLAfloat64LE createHLAfloat64LE(double value) { return new Float64LittleEndianElement(value); }

   @Override
   public HLAfloat32LE createHLAfloat32LE() { return new Float32LittleEndianElement(); }

   @Override
   public HLAfloat32LE createHLAfloat32LE(float value) {
      return new Float32LittleEndianElement(value);
   }

   @Override
   public HLAunsignedInteger16BE createHLAunsignedInteger16BE() {
      return new UnsignedInteger16Element();
   }

   @Override
   public HLAunsignedInteger16BE createHLAunsignedInteger16BE(short value) {
      return new UnsignedInteger16Element(value);
   }

   @Override
   public HLAunsignedInteger16LE createHLAunsignedInteger16LE() {
      return new UnsignedInteger16LittleEndianElement();
   }

   @Override
   public HLAunsignedInteger16LE createHLAunsignedInteger16LE(short value) {
      return new UnsignedInteger16LittleEndianElement(value);
   }

   @Override
   public HLAunsignedInteger32LE createHLAunsignedInteger32LE() {
      return new UnsignedInteger32LittleEndianElement();
   }

   @Override
   public HLAunsignedInteger32LE createHLAunsignedInteger32LE(int value) {
      return new UnsignedInteger32LittleEndianElement(value);
   }

   @Override
   public HLAinteger32BE createHLAinteger32BE() { return new Integer32Element(); }

   @Override
   public HLAinteger32BE createHLAinteger32BE(int value) { return new Integer32Element(value); }

   @Override
   public HLAunsignedInteger32BE createHLAunsignedInteger32BE() { return new UnsignedInteger32Element(); }

   @Override
   public HLAunsignedInteger32BE createHLAunsignedInteger32BE(int value) {
      return new UnsignedInteger32Element(value);
   }

   @Override
   public HLAunsignedInteger64BE createHLAunsignedInteger64BE() {
      return new UnsignedInteger64Element();
   }

   @Override
   public HLAunsignedInteger64BE createHLAunsignedInteger64BE(long value) {
      return new UnsignedInteger64Element(value);
   }

   @Override
   public HLAunsignedInteger64LE createHLAunsignedInteger64LE() {
      return new UnsignedInteger64LittleEndianElement();
   }

   @Override
   public HLAunsignedInteger64LE createHLAunsignedInteger64LE(long value) {
      return new UnsignedInteger64LittleEndianElement(value);
   }

   @Override
   public HLAbyte createHLAbyte() { return new ByteElement(); }

   @Override
   public HLAbyte createHLAbyte(byte value) { return new ByteElement(value); }

   @Override
   public HLAoctet createHLAoctet() { return new OctetElement(); }

   @Override
   public HLAoctet createHLAoctet(byte value) { return new OctetElement(value); }

   @Override
   public HLAASCIIchar createHLAASCIIchar() { return new ASCIICharElement(); }

   @Override
   public HLAASCIIchar createHLAASCIIchar(byte value) { return new ASCIICharElement(value); }

   @Override
   public HLAASCIIstring createHLAASCIIstring() { return new ASCIIStringElement(); }

   @Override
   public HLAASCIIstring createHLAASCIIstring(String value) { return new ASCIIStringElement(value); }

   @Override
   public HLAunicodeChar createHLAunicodeChar() { return new UnicodeCharElement(); }

   @Override
   public HLAunicodeChar createHLAunicodeChar(short value) { return new UnicodeCharElement(value); }

   @Override
   public HLAoctetPairBE createHLAoctetPairBE() { return new OctetPairBEElement(); }

   @Override
   public HLAoctetPairBE createHLAoctetPairBE(short value) { return new OctetPairBEElement(value); }

   @Override
   public HLAoctetPairLE createHLAoctetPairLE() { return new OctetPairLEElement(); }

   @Override
   public HLAoctetPairLE createHLAoctetPairLE(short value) { return new OctetPairLEElement(value); }

   @Override
   public HLAopaqueData createHLAopaqueData() { return new OpaqueDataElement(); }

   @Override
   public HLAopaqueData createHLAopaqueData(byte[] value) { return new OpaqueDataElement(value); }

   @Override
   public HLAvariableArray createHLAvariableArray(DataElementFactory factory, DataElement... elements) {
      return new VariableArrayElement(factory, elements);
   }

   @Override
   public HLAfixedArray createHLAfixedArray(DataElementFactory factory, int size) {
      return new FixedArrayElement(factory, size);
   }

   @Override
   public HLAfixedArray createHLAfixedArray(DataElement... elements) {
      if (elements == null || elements.length == 0) {
         throw new IllegalArgumentException("HLAfixedArray requires at least one element");
      }
      return new FixedArrayElement(index -> elements[0], elements.length);
   }

   @Override
   public HLAfixedRecord createHLAfixedRecord() { return new FixedRecordElement(); }

   @Override
   public HLAvariantRecord createHLAvariantRecord(DataElement discriminantPrototype) {
      return new VariantRecordElement(discriminantPrototype);
   }

   @Override
   public HLAboolean createHLAboolean() { return new BooleanElement(); }

   @Override
   public HLAboolean createHLAboolean(boolean value) { return new BooleanElement(value); }

   @Override
   public HLAunicodeString createHLAunicodeString() { return new UnicodeStringElement(); }

   @Override
   public HLAunicodeString createHLAunicodeString(String value) { return new UnicodeStringElement(value); }

   @Override
   public hla.rti1516_2025.encoding.HLAfederateHandle createHLAfederateHandle(
      hla.rti1516_2025.RTIambassador ambassador) { return unsupported(); }

   @Override
   public hla.rti1516_2025.encoding.HLAfederateHandle createHLAfederateHandle(
      hla.rti1516_2025.RTIambassador ambassador,
      hla.rti1516_2025.FederateHandle value) { return unsupported(); }

   @Override
   public hla.rti1516_2025.encoding.HLAobjectClassHandle createHLAobjectClassHandle(
      hla.rti1516_2025.RTIambassador ambassador) { return unsupported(); }

   @Override
   public hla.rti1516_2025.encoding.HLAobjectClassHandle createHLAobjectClassHandle(
      hla.rti1516_2025.RTIambassador ambassador,
      hla.rti1516_2025.ObjectClassHandle value) { return unsupported(); }

   @Override
   public hla.rti1516_2025.encoding.HLAinteractionClassHandle createHLAinteractionClassHandle(
      hla.rti1516_2025.RTIambassador ambassador) { return unsupported(); }

   @Override
   public hla.rti1516_2025.encoding.HLAinteractionClassHandle createHLAinteractionClassHandle(
      hla.rti1516_2025.RTIambassador ambassador,
      hla.rti1516_2025.InteractionClassHandle value) { return unsupported(); }

   @Override
   public hla.rti1516_2025.encoding.HLAobjectInstanceHandle createHLAobjectInstanceHandle(
      hla.rti1516_2025.RTIambassador ambassador) { return unsupported(); }

   @Override
   public hla.rti1516_2025.encoding.HLAobjectInstanceHandle createHLAobjectInstanceHandle(
      hla.rti1516_2025.RTIambassador ambassador,
      hla.rti1516_2025.ObjectInstanceHandle value) { return unsupported(); }

   @Override
   public hla.rti1516_2025.encoding.HLAattributeHandle createHLAattributeHandle(
      hla.rti1516_2025.RTIambassador ambassador) { return unsupported(); }

   @Override
   public hla.rti1516_2025.encoding.HLAattributeHandle createHLAattributeHandle(
      hla.rti1516_2025.RTIambassador ambassador,
      hla.rti1516_2025.AttributeHandle value) { return unsupported(); }

   @Override
   public hla.rti1516_2025.encoding.HLAparameterHandle createHLAparameterHandle(
      hla.rti1516_2025.RTIambassador ambassador) { return unsupported(); }

   @Override
   public hla.rti1516_2025.encoding.HLAparameterHandle createHLAparameterHandle(
      hla.rti1516_2025.RTIambassador ambassador,
      hla.rti1516_2025.ParameterHandle value) { return unsupported(); }

   @Override
   public hla.rti1516_2025.encoding.HLAdimensionHandle createHLAdimensionHandle(
      hla.rti1516_2025.RTIambassador ambassador) { return unsupported(); }

   @Override
   public hla.rti1516_2025.encoding.HLAdimensionHandle createHLAdimensionHandle(
      hla.rti1516_2025.RTIambassador ambassador,
      hla.rti1516_2025.DimensionHandle value) { return unsupported(); }

   @Override
   public hla.rti1516_2025.encoding.HLAregionHandle createHLAregionHandle(
      hla.rti1516_2025.RTIambassador ambassador) { return unsupported(); }

   @Override
   public hla.rti1516_2025.encoding.HLAregionHandle createHLAregionHandle(
      hla.rti1516_2025.RTIambassador ambassador,
      hla.rti1516_2025.RegionHandle value) { return unsupported(); }

   @Override
   public hla.rti1516_2025.encoding.HLAtransportationTypeHandle createHLAtransportationTypeHandle(
      hla.rti1516_2025.RTIambassador ambassador) { return unsupported(); }

   @Override
   public hla.rti1516_2025.encoding.HLAtransportationTypeHandle createHLAtransportationTypeHandle(
      hla.rti1516_2025.RTIambassador ambassador,
      hla.rti1516_2025.TransportationTypeHandle value) { return unsupported(); }

   @Override
   public hla.rti1516_2025.encoding.HLAmessageRetractionHandle createHLAmessageRetractionHandle(
      hla.rti1516_2025.RTIambassador ambassador) { return unsupported(); }

   @Override
   public hla.rti1516_2025.encoding.HLAmessageRetractionHandle createHLAmessageRetractionHandle(
      hla.rti1516_2025.RTIambassador ambassador,
      hla.rti1516_2025.MessageRetractionHandle value) { return unsupported(); }

   @Override
   public hla.rti1516_2025.encoding.HLAlogicalTime createHLAlogicalTime(
      hla.rti1516_2025.RTIambassador ambassador) { return unsupported(); }

   @Override
   public hla.rti1516_2025.encoding.HLAlogicalTime createHLAlogicalTime(
      hla.rti1516_2025.RTIambassador ambassador,
      hla.rti1516_2025.LogicalTime value) { return unsupported(); }

   @Override
   public hla.rti1516_2025.encoding.HLAlogicalTimeInterval createHLAlogicalTimeInterval(
      hla.rti1516_2025.RTIambassador ambassador) { return unsupported(); }

   @Override
   public hla.rti1516_2025.encoding.HLAlogicalTimeInterval createHLAlogicalTimeInterval(
      hla.rti1516_2025.RTIambassador ambassador,
      hla.rti1516_2025.LogicalTimeInterval value) { return unsupported(); }

   @Override
   public hla.rti1516_2025.encoding.HLAextendableVariantRecord createHLAextendableVariantRecord(
      DataElement discriminantPrototype) { return unsupported(); }

   private static <T> T unsupported() {
      throw new UnsupportedOperationException("Mock encoder does not implement this carrier");
   }

   private static void requireLength(byte[] bytes, int expected) {
      if (bytes.length != expected) {
         throw new IllegalArgumentException("invalid fixed-width encoding length");
      }
   }

   /**
    * Return one element's encoded length at an offset, including the recursive
    * count/padding rules of constructed elements.  The public Java DataElement
    * contract only exposes decode(byte[]), so the fixture has to provide this
    * small decodeFrom-equivalent when a constructed element is nested inside a
    * record or another array.
    */
   private static int encodedElementLength(DataElement element, byte[] bytes, int offset) {
      if (offset < 0 || offset > bytes.length) {
         throw new DecoderException("invalid nested DataElement offset");
      }
      if (element instanceof FixedArrayElement) {
         return ((FixedArrayElement)element).encodedLengthFrom(bytes, offset);
      }
      if (element instanceof VariableArrayElement) {
         return ((VariableArrayElement)element).encodedLengthFrom(bytes, offset);
      }
      if (element instanceof FixedRecordElement) {
         return ((FixedRecordElement)element).encodedLengthFrom(bytes, offset);
      }
      if (element instanceof VariantRecordElement) {
         return ((VariantRecordElement)element).encodedLengthFrom(bytes, offset);
      }
      if (element instanceof HLAASCIIstring || element instanceof HLAopaqueData ||
            element instanceof HLAunicodeString) {
         if (bytes.length - offset < 4) {
            throw new DecoderException("truncated variable-length DataElement count");
         }
         int count = ByteBuffer.wrap(bytes, offset, 4).getInt();
         if (count < 0 || (element instanceof HLAunicodeString && count > Integer.MAX_VALUE / 2)) {
            throw new DecoderException("invalid variable-length DataElement count");
         }
         return 4 + (element instanceof HLAunicodeString ? count * 2 : count);
      }
      int length = element.getEncodedLength();
      if (length < 0 || bytes.length - offset < length) {
         throw new DecoderException("truncated fixed-width DataElement");
      }
      return length;
   }

   private static final class Integer16Element implements HLAinteger16BE {
      private short value;

      Integer16Element() { }

      Integer16Element(short value) { this.value = value; }

      @Override public int getOctetBoundary() { return 2; }
      @Override public int getEncodedLength() { return 2; }
      @Override public short getValue() { return value; }
      @Override public HLAinteger16BE setValue(short value) { this.value = value; return this; }
      @Override public byte[] toByteArray() { return ByteBuffer.allocate(2).putShort(value).array(); }
      @Override public HLAinteger16BE decode(byte[] bytes) {
         requireLength(bytes, 2);
         value = ByteBuffer.wrap(bytes).getShort();
         return this;
      }
   }

   private static final class Integer16LittleEndianElement implements HLAinteger16LE {
      private short value;

      Integer16LittleEndianElement() { }

      Integer16LittleEndianElement(short value) { this.value = value; }

      @Override public int getOctetBoundary() { return 2; }
      @Override public int getEncodedLength() { return 2; }
      @Override public short getValue() { return value; }
      @Override public HLAinteger16LE setValue(short value) { this.value = value; return this; }
      @Override public byte[] toByteArray() {
         return ByteBuffer.allocate(2).order(ByteOrder.LITTLE_ENDIAN).putShort(value).array();
      }
      @Override public HLAinteger16LE decode(byte[] bytes) {
         requireLength(bytes, 2);
         value = ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN).getShort();
         return this;
      }
   }

   private static final class Float64Element implements HLAfloat64BE {
      private double value;

      Float64Element() { }

      Float64Element(double value) { this.value = value; }

      @Override public int getOctetBoundary() { return 8; }
      @Override public int getEncodedLength() { return 8; }
      @Override public double getValue() { return value; }
      @Override public HLAfloat64BE setValue(double value) { this.value = value; return this; }
      @Override public byte[] toByteArray() { return ByteBuffer.allocate(8).putDouble(value).array(); }
      @Override public HLAfloat64BE decode(byte[] bytes) {
         requireLength(bytes, 8);
         value = ByteBuffer.wrap(bytes).getDouble();
         return this;
      }
   }

   private static final class Float32Element implements HLAfloat32BE {
      private float value;

      Float32Element() { }

      Float32Element(float value) { this.value = value; }

      @Override public int getOctetBoundary() { return 4; }
      @Override public int getEncodedLength() { return 4; }
      @Override public float getValue() { return value; }
      @Override public HLAfloat32BE setValue(float value) { this.value = value; return this; }
      @Override public byte[] toByteArray() { return ByteBuffer.allocate(4).putFloat(value).array(); }
      @Override public HLAfloat32BE decode(byte[] bytes) {
         requireLength(bytes, 4);
         value = ByteBuffer.wrap(bytes).getFloat();
         return this;
      }
   }

   private static final class Integer32LittleEndianElement implements HLAinteger32LE {
      private int value;

      Integer32LittleEndianElement() { }

      Integer32LittleEndianElement(int value) { this.value = value; }

      @Override public int getOctetBoundary() { return 4; }
      @Override public int getEncodedLength() { return 4; }
      @Override public int getValue() { return value; }
      @Override public HLAinteger32LE setValue(int value) { this.value = value; return this; }
      @Override public byte[] toByteArray() {
         return ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN).putInt(value).array();
      }
      @Override public HLAinteger32LE decode(byte[] bytes) {
         requireLength(bytes, 4);
         value = ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN).getInt();
         return this;
      }
   }

   private static final class Float64LittleEndianElement implements HLAfloat64LE {
      private double value;

      Float64LittleEndianElement() { }

      Float64LittleEndianElement(double value) { this.value = value; }

      @Override public int getOctetBoundary() { return 8; }
      @Override public int getEncodedLength() { return 8; }
      @Override public double getValue() { return value; }
      @Override public HLAfloat64LE setValue(double value) { this.value = value; return this; }
      @Override public byte[] toByteArray() {
         return ByteBuffer.allocate(8).order(ByteOrder.LITTLE_ENDIAN).putDouble(value).array();
      }
      @Override public HLAfloat64LE decode(byte[] bytes) {
         requireLength(bytes, 8);
         value = ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN).getDouble();
         return this;
      }
   }

   private static final class Float32LittleEndianElement implements HLAfloat32LE {
      private float value;

      Float32LittleEndianElement() { }

      Float32LittleEndianElement(float value) { this.value = value; }

      @Override public int getOctetBoundary() { return 4; }
      @Override public int getEncodedLength() { return 4; }
      @Override public float getValue() { return value; }
      @Override public HLAfloat32LE setValue(float value) { this.value = value; return this; }
      @Override public byte[] toByteArray() {
         return ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN).putFloat(value).array();
      }
      @Override public HLAfloat32LE decode(byte[] bytes) {
         requireLength(bytes, 4);
         value = ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN).getFloat();
         return this;
      }
   }

   private static final class Integer32Element implements HLAinteger32BE {
      private int value;

      Integer32Element() { }

      Integer32Element(int value) { this.value = value; }

      @Override public int getOctetBoundary() { return 4; }
      @Override public int getEncodedLength() { return 4; }
      @Override public int getValue() { return value; }
      @Override public HLAinteger32BE setValue(int value) { this.value = value; return this; }
      @Override public byte[] toByteArray() { return ByteBuffer.allocate(4).putInt(value).array(); }
      @Override public HLAinteger32BE decode(byte[] bytes) {
         requireLength(bytes, 4);
         value = ByteBuffer.wrap(bytes).getInt();
         return this;
      }
   }

   private static final class Integer64Element implements HLAinteger64BE {
      private long value;

      Integer64Element() { }

      Integer64Element(long value) { this.value = value; }

      @Override public int getOctetBoundary() { return 8; }
      @Override public int getEncodedLength() { return 8; }
      @Override public long getValue() { return value; }
      @Override public HLAinteger64BE setValue(long value) { this.value = value; return this; }
      @Override public byte[] toByteArray() { return ByteBuffer.allocate(8).putLong(value).array(); }
      @Override public HLAinteger64BE decode(byte[] bytes) {
         requireLength(bytes, 8);
         value = ByteBuffer.wrap(bytes).getLong();
         return this;
      }
   }

   private static final class Integer64LittleEndianElement implements HLAinteger64LE {
      private long value;

      Integer64LittleEndianElement() { }

      Integer64LittleEndianElement(long value) { this.value = value; }

      @Override public int getOctetBoundary() { return 8; }
      @Override public int getEncodedLength() { return 8; }
      @Override public long getValue() { return value; }
      @Override public HLAinteger64LE setValue(long value) { this.value = value; return this; }
      @Override public byte[] toByteArray() {
         return ByteBuffer.allocate(8).order(ByteOrder.LITTLE_ENDIAN).putLong(value).array();
      }
      @Override public HLAinteger64LE decode(byte[] bytes) {
         requireLength(bytes, 8);
         value = ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN).getLong();
         return this;
      }
   }

   private static final class UnsignedInteger64Element implements HLAunsignedInteger64BE {
      private long value;

      UnsignedInteger64Element() { }

      UnsignedInteger64Element(long value) { this.value = value; }

      @Override public int getOctetBoundary() { return 8; }
      @Override public int getEncodedLength() { return 8; }
      @Override public long getValue() { return value; }
      @Override public HLAunsignedInteger64BE setValue(long value) { this.value = value; return this; }
      @Override public byte[] toByteArray() { return ByteBuffer.allocate(8).putLong(value).array(); }
      @Override public HLAunsignedInteger64BE decode(byte[] bytes) {
         requireLength(bytes, 8);
         value = ByteBuffer.wrap(bytes).getLong();
         return this;
      }
   }

   private static final class UnsignedInteger64LittleEndianElement implements HLAunsignedInteger64LE {
      private long value;

      UnsignedInteger64LittleEndianElement() { }

      UnsignedInteger64LittleEndianElement(long value) { this.value = value; }

      @Override public int getOctetBoundary() { return 8; }
      @Override public int getEncodedLength() { return 8; }
      @Override public long getValue() { return value; }
      @Override public HLAunsignedInteger64LE setValue(long value) { this.value = value; return this; }
      @Override public byte[] toByteArray() {
         return ByteBuffer.allocate(8).order(ByteOrder.LITTLE_ENDIAN).putLong(value).array();
      }
      @Override public HLAunsignedInteger64LE decode(byte[] bytes) {
         requireLength(bytes, 8);
         value = ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN).getLong();
         return this;
      }
   }

   private static final class UnsignedInteger16Element implements HLAunsignedInteger16BE {
      private short value;

      UnsignedInteger16Element() { }

      UnsignedInteger16Element(short value) { this.value = value; }

      @Override public int getOctetBoundary() { return 2; }
      @Override public int getEncodedLength() { return 2; }
      @Override public short getValue() { return value; }
      @Override public HLAunsignedInteger16BE setValue(short value) { this.value = value; return this; }
      @Override public byte[] toByteArray() { return ByteBuffer.allocate(2).putShort(value).array(); }
      @Override public HLAunsignedInteger16BE decode(byte[] bytes) {
         requireLength(bytes, 2);
         value = ByteBuffer.wrap(bytes).getShort();
         return this;
      }
   }

   private static final class UnsignedInteger16LittleEndianElement implements HLAunsignedInteger16LE {
      private short value;

      UnsignedInteger16LittleEndianElement() { }

      UnsignedInteger16LittleEndianElement(short value) { this.value = value; }

      @Override public int getOctetBoundary() { return 2; }
      @Override public int getEncodedLength() { return 2; }
      @Override public short getValue() { return value; }
      @Override public HLAunsignedInteger16LE setValue(short value) { this.value = value; return this; }
      @Override public byte[] toByteArray() {
         return ByteBuffer.allocate(2).order(ByteOrder.LITTLE_ENDIAN).putShort(value).array();
      }
      @Override public HLAunsignedInteger16LE decode(byte[] bytes) {
         requireLength(bytes, 2);
         value = ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN).getShort();
         return this;
      }
   }

   private static final class UnsignedInteger32Element implements HLAunsignedInteger32BE {
      private int value;

      UnsignedInteger32Element() { }

      UnsignedInteger32Element(int value) { this.value = value; }

      @Override public int getOctetBoundary() { return 4; }
      @Override public int getEncodedLength() { return 4; }
      @Override public int getValue() { return value; }
      @Override public HLAunsignedInteger32BE setValue(int value) { this.value = value; return this; }
      @Override public byte[] toByteArray() { return ByteBuffer.allocate(4).putInt(value).array(); }
      @Override public HLAunsignedInteger32BE decode(byte[] bytes) {
         requireLength(bytes, 4);
         value = ByteBuffer.wrap(bytes).getInt();
         return this;
      }
   }

   private static final class UnsignedInteger32LittleEndianElement implements HLAunsignedInteger32LE {
      private int value;

      UnsignedInteger32LittleEndianElement() { }

      UnsignedInteger32LittleEndianElement(int value) { this.value = value; }

      @Override public int getOctetBoundary() { return 4; }
      @Override public int getEncodedLength() { return 4; }
      @Override public int getValue() { return value; }
      @Override public HLAunsignedInteger32LE setValue(int value) { this.value = value; return this; }
      @Override public byte[] toByteArray() {
         return ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN).putInt(value).array();
      }
      @Override public HLAunsignedInteger32LE decode(byte[] bytes) {
         requireLength(bytes, 4);
         value = ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN).getInt();
         return this;
      }
   }

   private static final class BooleanElement implements HLAboolean {
      private boolean value;

      BooleanElement() { }

      BooleanElement(boolean value) { this.value = value; }

      @Override public int getOctetBoundary() { return 4; }
      @Override public int getEncodedLength() { return 4; }
      @Override public boolean getValue() { return value; }
      @Override public HLAboolean setValue(boolean value) { this.value = value; return this; }
      @Override public byte[] toByteArray() {
         return ByteBuffer.allocate(4).putInt(value ? 1 : 0).array();
      }
      @Override public HLAboolean decode(byte[] bytes) {
         requireLength(bytes, 4);
         int decoded = ByteBuffer.wrap(bytes).getInt();
         if (decoded != 0 && decoded != 1) {
            throw new DecoderException("invalid HLAboolean encoding");
         }
         value = decoded == 1;
         return this;
      }
   }

   private static final class ByteElement implements HLAbyte {
      private byte value;

      ByteElement() { }

      ByteElement(byte value) { this.value = value; }

      @Override public int getOctetBoundary() { return 1; }
      @Override public int getEncodedLength() { return 1; }
      @Override public byte getValue() { return value; }
      @Override public HLAbyte setValue(byte value) { this.value = value; return this; }
      @Override public byte[] toByteArray() { return new byte[] {value}; }
      @Override public HLAbyte decode(byte[] bytes) {
         requireLength(bytes, 1);
         value = bytes[0];
         return this;
      }
   }

   private static final class OctetElement implements HLAoctet {
      private byte value;

      OctetElement() { }

      OctetElement(byte value) { this.value = value; }

      @Override public int getOctetBoundary() { return 1; }
      @Override public int getEncodedLength() { return 1; }
      @Override public byte getValue() { return value; }
      @Override public HLAoctet setValue(byte value) { this.value = value; return this; }
      @Override public byte[] toByteArray() { return new byte[] {value}; }
      @Override public HLAoctet decode(byte[] bytes) {
         requireLength(bytes, 1);
         value = bytes[0];
         return this;
      }
   }

   private static void requireAscii(char value) {
      if (value > 0x7f) {
         throw new IllegalArgumentException("value is not ASCII");
      }
   }

   private static final class ASCIICharElement implements HLAASCIIchar {
      private byte value;

      ASCIICharElement() { }

      ASCIICharElement(byte value) { this.value = value; }

      @Override public int getOctetBoundary() { return 1; }
      @Override public int getEncodedLength() { return 1; }
      @Override public byte getValue() { return value; }
      @Override public HLAASCIIchar setValue(byte value) { this.value = value; return this; }
      @Override public byte[] toByteArray() {
         requireAscii((char)(value & 0xff));
         return new byte[] {value};
      }
      @Override public HLAASCIIchar decode(byte[] bytes) {
         requireLength(bytes, 1);
         value = bytes[0];
         requireAscii((char)(value & 0xff));
         return this;
      }
   }

   private static final class ASCIIStringElement implements HLAASCIIstring {
      private String value = "";

      ASCIIStringElement() { }

      ASCIIStringElement(String value) { this.value = value; }

      private void validate() {
         for (int index = 0; index < value.length(); index++) {
            requireAscii(value.charAt(index));
         }
      }

      @Override public int getOctetBoundary() { return 4; }
      @Override public int getEncodedLength() { validate(); return 4 + value.length(); }
      @Override public String getValue() { return value; }
      @Override public HLAASCIIstring setValue(String value) { this.value = value; return this; }
      @Override public byte[] toByteArray() {
         validate();
         return ByteBuffer.allocate(4 + value.length()).putInt(value.length())
            .put(value.getBytes(StandardCharsets.US_ASCII)).array();
      }
      @Override public HLAASCIIstring decode(byte[] bytes) {
         if (bytes.length < 4) {
            throw new IllegalArgumentException("truncated HLAASCIIstring encoding");
         }
         ByteBuffer buffer = ByteBuffer.wrap(bytes).order(ByteOrder.BIG_ENDIAN);
         int length = buffer.getInt();
         if (length < 0 || bytes.length != 4 + length) {
            throw new IllegalArgumentException("invalid HLAASCIIstring encoding");
         }
         byte[] payload = new byte[length];
         buffer.get(payload);
         String decoded = new String(payload, StandardCharsets.US_ASCII);
         for (int index = 0; index < decoded.length(); index++) {
            requireAscii(decoded.charAt(index));
         }
         value = decoded;
         return this;
      }
   }

   private static final class UnicodeCharElement implements HLAunicodeChar {
      private short value;

      UnicodeCharElement() { }

      UnicodeCharElement(short value) { this.value = value; }

      @Override public int getOctetBoundary() { return 2; }
      @Override public int getEncodedLength() { return 2; }
      @Override public short getValue() { return value; }
      @Override public HLAunicodeChar setValue(short value) { this.value = value; return this; }
      @Override public byte[] toByteArray() {
         return ByteBuffer.allocate(2).order(ByteOrder.BIG_ENDIAN).putShort(value).array();
      }
      @Override public HLAunicodeChar decode(byte[] bytes) {
         requireLength(bytes, 2);
         value = ByteBuffer.wrap(bytes).order(ByteOrder.BIG_ENDIAN).getShort();
         return this;
      }
   }

   private static final class OctetPairBEElement implements HLAoctetPairBE {
      private short value;

      OctetPairBEElement() { }

      OctetPairBEElement(short value) { this.value = value; }

      @Override public int getOctetBoundary() { return 2; }
      @Override public int getEncodedLength() { return 2; }
      @Override public short getValue() { return value; }
      @Override public HLAoctetPairBE setValue(short value) { this.value = value; return this; }
      @Override public byte[] toByteArray() { return ByteBuffer.allocate(2).putShort(value).array(); }
      @Override public HLAoctetPairBE decode(byte[] bytes) {
         requireLength(bytes, 2);
         value = ByteBuffer.wrap(bytes).getShort();
         return this;
      }
   }

   private static final class OctetPairLEElement implements HLAoctetPairLE {
      private short value;

      OctetPairLEElement() { }

      OctetPairLEElement(short value) { this.value = value; }

      @Override public int getOctetBoundary() { return 2; }
      @Override public int getEncodedLength() { return 2; }
      @Override public short getValue() { return value; }
      @Override public HLAoctetPairLE setValue(short value) { this.value = value; return this; }
      @Override public byte[] toByteArray() {
         return ByteBuffer.allocate(2).order(ByteOrder.LITTLE_ENDIAN).putShort(value).array();
      }
      @Override public HLAoctetPairLE decode(byte[] bytes) {
         requireLength(bytes, 2);
         value = ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN).getShort();
         return this;
      }
   }

   private static final class OpaqueDataElement implements HLAopaqueData {
      private byte[] value = new byte[0];

      OpaqueDataElement() { }

      OpaqueDataElement(byte[] value) { setValue(value); }

      @Override public int getOctetBoundary() { return 4; }
      @Override public int getEncodedLength() { return 4 + value.length; }
      @Override public int size() { return value.length; }
      @Override public byte get(int index) { return value[index]; }
      @Override public byte[] getValue() { return value.clone(); }
      @Override public java.util.Iterator<Byte> iterator() {
         final byte[] snapshot = value.clone();
         return new java.util.Iterator<Byte>() {
            private int index;

            @Override public boolean hasNext() { return index < snapshot.length; }
            @Override public Byte next() { return snapshot[index++]; }
         };
      }
      @Override public HLAopaqueData setValue(byte[] value) { this.value = value.clone(); return this; }
      @Override public byte[] toByteArray() {
         return ByteBuffer.allocate(4 + value.length).order(ByteOrder.BIG_ENDIAN)
            .putInt(value.length).put(value).array();
      }
      @Override public HLAopaqueData decode(byte[] bytes) {
         if (bytes.length < 4) {
            throw new DecoderException("truncated HLAopaqueData encoding");
         }
         ByteBuffer buffer = ByteBuffer.wrap(bytes).order(ByteOrder.BIG_ENDIAN);
         int length = buffer.getInt();
         if (length < 0 || bytes.length != 4 + length) {
            throw new DecoderException("invalid HLAopaqueData encoding");
         }
         byte[] decoded = new byte[length];
         buffer.get(decoded);
         value = decoded;
         return this;
      }
   }

   private static final class FixedArrayElement implements HLAfixedArray<DataElement> {
      private final DataElementFactory<DataElement> factory;
      private final DataElement prototype;
      private final List<DataElement> elements = new ArrayList<>();

      @SuppressWarnings("unchecked")
      FixedArrayElement(DataElementFactory factory, int size) {
         if (size < 0) throw new EncoderException("negative HLAfixedArray size");
         this.factory = (DataElementFactory<DataElement>)factory;
         this.prototype = this.factory.createElement(0);
         for (int index = 0; index < size; index++) {
            DataElement element = this.factory.createElement(index);
            if (!element.getClass().equals(prototype.getClass())) {
               throw new EncoderException("DataElementFactory returned an inconsistent type");
            }
            elements.add(element);
         }
      }

      @SuppressWarnings("unchecked")
      FixedArrayElement(FixedArrayElement source) {
         this.factory = source.factory;
         this.prototype = this.factory.createElement(0);
         for (int index = 0; index < source.elements.size(); index++) {
            DataElement element = this.factory.createElement(index);
            if (!element.getClass().equals(prototype.getClass())) {
               throw new EncoderException("DataElementFactory returned an inconsistent type");
            }
            element.decode(source.elements.get(index).toByteArray());
            elements.add(element);
         }
      }

      private static int padding(int length, int boundary) {
         int remainder = length % boundary;
         return remainder == 0 ? 0 : boundary - remainder;
      }

      private static void requirePadding(byte[] bytes, int offset, int count) {
         if (offset < 0 || bytes.length - offset < count) {
            throw new DecoderException("truncated HLAfixedArray padding");
         }
         for (int index = 0; index < count; index++) {
            if (bytes[offset + index] != 0) {
               throw new DecoderException("nonzero HLAfixedArray padding");
            }
         }
      }

      private int elementLength(byte[] bytes, int offset) {
         return encodedElementLength(prototype, bytes, offset);
      }

      private int encodedLengthFrom(byte[] bytes, int offset) {
         int cursor = offset;
         for (int index = 0; index < elements.size(); index++) {
            int length = encodedElementLength(elements.get(index), bytes, cursor);
            if (bytes.length - cursor < length) {
               throw new DecoderException("truncated HLAfixedArray element");
            }
            cursor += length;
            if (index + 1 < elements.size()) {
               int elementPadding = padding(length, prototype.getOctetBoundary());
               requirePadding(bytes, cursor, elementPadding);
               cursor += elementPadding;
            }
         }
         return cursor - offset;
      }

      @Override public int getOctetBoundary() { return prototype.getOctetBoundary(); }
      @Override public int getEncodedLength() { return toByteArray().length; }
      @Override public int size() { return elements.size(); }
      @Override public DataElement get(int index) { return elements.get(index); }
      @Override public Iterator<DataElement> iterator() { return elements.iterator(); }
      @Override public byte[] toByteArray() {
         ByteArrayOutputStream output = new ByteArrayOutputStream();
         for (int index = 0; index < elements.size(); index++) {
            byte[] encoded = elements.get(index).toByteArray();
            output.writeBytes(encoded);
            if (index + 1 < elements.size()) {
               output.writeBytes(new byte[padding(encoded.length, prototype.getOctetBoundary())]);
            }
         }
         return output.toByteArray();
      }
      @Override public DataElement decode(byte[] bytes) {
         int offset = 0;
         for (int index = 0; index < elements.size(); index++) {
            int length = elementLength(bytes, offset);
            if (length < 0 || bytes.length - offset < length) {
               throw new DecoderException("truncated HLAfixedArray element");
            }
            elements.get(index).decode(java.util.Arrays.copyOfRange(bytes, offset, offset + length));
            offset += length;
            if (index + 1 < elements.size()) {
               int elementPadding = padding(length, prototype.getOctetBoundary());
               requirePadding(bytes, offset, elementPadding);
               offset += elementPadding;
            }
         }
         if (offset != bytes.length) throw new DecoderException("trailing HLAfixedArray data");
         return this;
      }
   }

   private static final class FixedRecordElement implements HLAfixedRecord<DataElement> {
      private final List<DataElement> elements = new ArrayList<>();

      FixedRecordElement() { }

      FixedRecordElement(FixedRecordElement source) {
         for (DataElement element : source.elements) elements.add(copyElement(element));
      }

      private static int paddingAfter(int offset, int length, int boundary) {
         int remainder = (offset + length) % boundary;
         return remainder == 0 ? 0 : boundary - remainder;
      }

      private static void requirePadding(byte[] bytes, int offset, int count) {
         if (offset < 0 || bytes.length - offset < count) {
            throw new DecoderException("truncated HLAfixedRecord padding");
         }
         for (int index = 0; index < count; index++) {
            if (bytes[offset + index] != 0) {
               throw new DecoderException("nonzero HLAfixedRecord padding");
            }
         }
      }

      private static DataElement copyElement(DataElement source) {
         if (source instanceof FixedArrayElement) return new FixedArrayElement((FixedArrayElement)source);
         if (source instanceof VariableArrayElement) return new VariableArrayElement((VariableArrayElement)source);
         if (source instanceof FixedRecordElement) return new FixedRecordElement((FixedRecordElement)source);
         if (source instanceof VariantRecordElement) return new VariantRecordElement((VariantRecordElement)source);
         try {
            java.lang.reflect.Constructor<?> constructor = source.getClass().getDeclaredConstructor();
            constructor.setAccessible(true);
            DataElement copy = (DataElement)constructor.newInstance();
            copy.decode(source.toByteArray());
            return copy;
         } catch (ReflectiveOperationException | RuntimeException error) {
            throw new EncoderException("HLAfixedRecord element cannot be copied");
         }
      }

      private static int elementLength(DataElement element, byte[] bytes, int offset) {
         return encodedElementLength(element, bytes, offset);
      }

      private int encodedLengthFrom(byte[] bytes, int offset) {
         int cursor = offset;
         int recordOffset = 0;
         for (int index = 0; index < elements.size(); index++) {
            int length = encodedElementLength(elements.get(index), bytes, cursor);
            if (bytes.length - cursor < length) {
               throw new DecoderException("truncated HLAfixedRecord element");
            }
            cursor += length;
            if (index + 1 < elements.size()) {
               int componentPadding = paddingAfter(recordOffset, length,
                  elements.get(index + 1).getOctetBoundary());
               requirePadding(bytes, cursor, componentPadding);
               cursor += componentPadding;
               recordOffset += length + componentPadding;
            }
         }
         return cursor - offset;
      }

      @Override public int getOctetBoundary() {
         int boundary = 1;
         for (DataElement element : elements) boundary = Math.max(boundary, element.getOctetBoundary());
         return boundary;
      }

      @Override public int getEncodedLength() { return toByteArray().length; }
      @Override public int size() { return elements.size(); }
      @Override public DataElement get(int index) { return elements.get(index); }
      @Override public Iterator<DataElement> iterator() { return elements.iterator(); }

      @Override public void appendElement(DataElement dataElement) {
         if (dataElement == null) throw new EncoderException("null HLAfixedRecord element");
         elements.add(copyElement(dataElement));
      }

      @Override public byte[] toByteArray() {
         ByteArrayOutputStream output = new ByteArrayOutputStream();
         int recordOffset = 0;
         for (int index = 0; index < elements.size(); index++) {
            byte[] encoded = elements.get(index).toByteArray();
            output.writeBytes(encoded);
            if (index + 1 < elements.size()) {
               int padding = paddingAfter(recordOffset, encoded.length,
                  elements.get(index + 1).getOctetBoundary());
               output.writeBytes(new byte[padding]);
               recordOffset += encoded.length + padding;
            }
         }
         return output.toByteArray();
      }

      @Override public DataElement decode(byte[] bytes) {
         int offset = 0;
         int recordOffset = 0;
         for (int index = 0; index < elements.size(); index++) {
            int length = elementLength(elements.get(index), bytes, offset);
            if (length < 0 || bytes.length - offset < length) {
               throw new DecoderException("truncated HLAfixedRecord element");
            }
            elements.get(index).decode(java.util.Arrays.copyOfRange(bytes, offset, offset + length));
            offset += length;
            if (index + 1 < elements.size()) {
               int padding = paddingAfter(recordOffset, length,
                  elements.get(index + 1).getOctetBoundary());
               requirePadding(bytes, offset, padding);
               offset += padding;
               recordOffset += length + padding;
            }
         }
         if (offset != bytes.length) throw new DecoderException("trailing HLAfixedRecord data");
         return this;
      }
   }

   private static final class VariantRecordElement implements HLAvariantRecord<DataElement> {
      private static final class VariantSlot {
         final DataElement discriminant;
         final DataElement value;

         VariantSlot(DataElement discriminant, DataElement value) {
            this.discriminant = discriminant;
            this.value = value;
         }
      }

      private final DataElement discriminantPrototype;
      private DataElement currentDiscriminant;
      private final List<VariantSlot> variants = new ArrayList<>();

      VariantRecordElement(DataElement discriminantPrototype) {
         if (discriminantPrototype == null) {
            throw new EncoderException("null HLAvariantRecord discriminant prototype");
         }
         this.discriminantPrototype = copyElement(discriminantPrototype);
         this.currentDiscriminant = copyElement(discriminantPrototype);
      }

      VariantRecordElement(VariantRecordElement source) {
         this.discriminantPrototype = copyElement(source.discriminantPrototype);
         this.currentDiscriminant = copyElement(source.currentDiscriminant);
         for (VariantSlot slot : source.variants) {
            variants.add(new VariantSlot(copyElement(slot.discriminant), copyElement(slot.value)));
         }
      }

      private static DataElement copyElement(DataElement source) {
         if (source instanceof FixedArrayElement) return new FixedArrayElement((FixedArrayElement)source);
         if (source instanceof VariableArrayElement) return new VariableArrayElement((VariableArrayElement)source);
         if (source instanceof FixedRecordElement) return new FixedRecordElement((FixedRecordElement)source);
         if (source instanceof VariantRecordElement) return new VariantRecordElement((VariantRecordElement)source);
         try {
            java.lang.reflect.Constructor<?> constructor = source.getClass().getDeclaredConstructor();
            constructor.setAccessible(true);
            DataElement copy = (DataElement)constructor.newInstance();
            copy.decode(source.toByteArray());
            return copy;
         } catch (ReflectiveOperationException | RuntimeException error) {
            throw new EncoderException("HLAvariantRecord element cannot be copied");
         }
      }

      private static int padding(int length, int boundary) {
         int remainder = length % boundary;
         return remainder == 0 ? 0 : boundary - remainder;
      }

      private static void requirePadding(byte[] bytes, int offset, int count) {
         if (offset < 0 || bytes.length - offset < count) {
            throw new DecoderException("truncated HLAvariantRecord padding");
         }
         for (int index = 0; index < count; index++) {
            if (bytes[offset + index] != 0) {
               throw new DecoderException("nonzero HLAvariantRecord padding");
            }
         }
      }

      private static int elementLength(DataElement element, byte[] bytes, int offset) {
         return encodedElementLength(element, bytes, offset);
      }

      private int encodedLengthFrom(byte[] bytes, int offset) {
         int discriminantLength = encodedElementLength(discriminantPrototype, bytes, offset);
         if (bytes.length - offset < discriminantLength) {
            throw new DecoderException("truncated HLAvariantRecord discriminant");
         }
         DataElement decodedDiscriminant = copyElement(discriminantPrototype);
         decodedDiscriminant.decode(java.util.Arrays.copyOfRange(
            bytes, offset, offset + discriminantLength));
         VariantSlot slot = find(decodedDiscriminant);
         if (slot == null) return discriminantLength;
         int cursor = offset + discriminantLength;
         int alternativePadding = padding(discriminantLength, maximumAlternativeBoundary());
         requirePadding(bytes, cursor, alternativePadding);
         cursor += alternativePadding;
         int valueLength = encodedElementLength(slot.value, bytes, cursor);
         if (bytes.length - cursor < valueLength) {
            throw new DecoderException("truncated HLAvariantRecord value");
         }
         return cursor + valueLength - offset;
      }

      private VariantSlot find(DataElement discriminant) {
         byte[] encoded = discriminant.toByteArray();
         for (VariantSlot slot : variants) {
            if (slot.discriminant.getClass().equals(discriminant.getClass()) &&
                  java.util.Arrays.equals(slot.discriminant.toByteArray(), encoded)) {
               return slot;
            }
         }
         return null;
      }

      private void requireDiscriminantType(DataElement discriminant) {
         if (!discriminantPrototype.getClass().equals(discriminant.getClass())) {
            throw new EncoderException("HLAvariantRecord discriminant type mismatch");
         }
      }

      private int maximumAlternativeBoundary() {
         int boundary = 1;
         for (VariantSlot slot : variants) boundary = Math.max(boundary, slot.value.getOctetBoundary());
         return boundary;
      }

      @Override public int getOctetBoundary() {
         return Math.max(discriminantPrototype.getOctetBoundary(), maximumAlternativeBoundary());
      }

      @Override public int getEncodedLength() { return toByteArray().length; }

      @Override public HLAvariantRecord<DataElement> setVariant(DataElement discriminant, DataElement dataElement) {
         if (dataElement == null) throw new EncoderException("null HLAvariantRecord value");
         requireDiscriminantType(discriminant);
         VariantSlot slot = find(discriminant);
         if (slot == null) {
            variants.add(new VariantSlot(copyElement(discriminant), copyElement(dataElement)));
         } else {
            if (!slot.value.getClass().equals(dataElement.getClass())) {
               throw new EncoderException("HLAvariantRecord replacement type mismatch");
            }
            slot.value.decode(dataElement.toByteArray());
         }
         currentDiscriminant = copyElement(discriminant);
         return this;
      }

      @Override public HLAvariantRecord<DataElement> setDiscriminant(DataElement discriminant) {
         requireDiscriminantType(discriminant);
         currentDiscriminant = copyElement(discriminant);
         return this;
      }

      @Override public DataElement getDiscriminant() { return currentDiscriminant; }

      @Override public DataElement getValue() {
         VariantSlot slot = find(currentDiscriminant);
         return slot == null ? null : slot.value;
      }

      @Override public byte[] toByteArray() {
         byte[] discriminant = currentDiscriminant.toByteArray();
         VariantSlot slot = find(currentDiscriminant);
         if (slot == null) return discriminant;
         byte[] value = slot.value.toByteArray();
         ByteArrayOutputStream output = new ByteArrayOutputStream();
         output.writeBytes(discriminant);
         output.writeBytes(new byte[padding(discriminant.length, maximumAlternativeBoundary())]);
         output.writeBytes(value);
         return output.toByteArray();
      }

      @Override public DataElement decode(byte[] bytes) {
         int discriminantLength = elementLength(discriminantPrototype, bytes, 0);
         if (bytes.length < discriminantLength) {
            throw new DecoderException("truncated HLAvariantRecord discriminant");
         }
         DataElement decodedDiscriminant = copyElement(discriminantPrototype);
         decodedDiscriminant.decode(java.util.Arrays.copyOfRange(bytes, 0, discriminantLength));
         currentDiscriminant = decodedDiscriminant;
         VariantSlot slot = find(decodedDiscriminant);
         if (slot == null) {
            if (bytes.length != discriminantLength) {
               throw new DecoderException("trailing HLAvariantRecord data");
            }
            return this;
         }
         int offset = discriminantLength;
         int alternativePadding = padding(discriminantLength, maximumAlternativeBoundary());
         requirePadding(bytes, offset, alternativePadding);
         offset += alternativePadding;
         int valueLength = elementLength(slot.value, bytes, offset);
         if (bytes.length - offset < valueLength) {
            throw new DecoderException("truncated HLAvariantRecord value");
         }
         slot.value.decode(java.util.Arrays.copyOfRange(bytes, offset, offset + valueLength));
         offset += valueLength;
         if (offset != bytes.length) throw new DecoderException("trailing HLAvariantRecord data");
         return this;
      }
   }

   private static final class VariableArrayElement implements HLAvariableArray<DataElement> {
      private final DataElementFactory<DataElement> factory;
      private final DataElement prototype;
      private final List<DataElement> elements = new ArrayList<>();

      @SuppressWarnings("unchecked")
      VariableArrayElement(DataElementFactory factory, DataElement... initialElements) {
         this.factory = (DataElementFactory<DataElement>)factory;
         this.prototype = this.factory.createElement(0);
         for (DataElement element : initialElements) {
            addElement(element);
         }
      }

      @SuppressWarnings("unchecked")
      VariableArrayElement(VariableArrayElement source) {
         this.factory = source.factory;
         this.prototype = this.factory.createElement(0);
         for (DataElement element : source.elements) addElement(element);
      }

      private static int padding(int length, int boundary) {
         int remainder = length % boundary;
         return remainder == 0 ? 0 : boundary - remainder;
      }

      private static void requirePadding(byte[] bytes, int offset, int count) {
         if (offset < 0 || bytes.length - offset < count) {
            throw new DecoderException("truncated HLAvariableArray padding");
         }
         for (int index = 0; index < count; index++) {
            if (bytes[offset + index] != 0) {
               throw new DecoderException("nonzero HLAvariableArray padding");
            }
         }
      }

      private DataElement copyElement(DataElement source) {
         DataElement copy = factory.createElement(elements.size());
         if (!copy.getClass().equals(prototype.getClass())) {
            throw new EncoderException("DataElementFactory returned an inconsistent type");
         }
         copy.decode(source.toByteArray());
         return copy;
      }

      private int elementLength(byte[] bytes, int offset) {
         return encodedElementLength(prototype, bytes, offset);
      }

      private int encodedLengthFrom(byte[] bytes, int offset) {
         if (bytes.length - offset < 4) {
            throw new DecoderException("truncated HLAvariableArray count");
         }
         int count = ByteBuffer.wrap(bytes, offset, 4).getInt();
         if (count < 0) throw new DecoderException("negative HLAvariableArray count");
         int cursor = offset + 4;
         if (count == 0) return 4;
         int leadingPadding = padding(4, getOctetBoundary());
         requirePadding(bytes, cursor, leadingPadding);
         cursor += leadingPadding;
         for (int index = 0; index < count; index++) {
            int length = encodedElementLength(prototype, bytes, cursor);
            if (bytes.length - cursor < length) {
               throw new DecoderException("truncated HLAvariableArray element");
            }
            cursor += length;
            if (index + 1 < count) {
               int elementPadding = padding(length, prototype.getOctetBoundary());
               requirePadding(bytes, cursor, elementPadding);
               cursor += elementPadding;
            }
         }
         return cursor - offset;
      }

      @Override public int getOctetBoundary() { return Math.max(4, prototype.getOctetBoundary()); }
      @Override public int getEncodedLength() { return toByteArray().length; }
      @Override public int size() { return elements.size(); }
      @Override public HLAvariableArray<DataElement> addElement(DataElement dataElement) {
         if (!dataElement.getClass().equals(prototype.getClass())) {
            throw new EncoderException("HLAvariableArray element type mismatch");
         }
         elements.add(copyElement(dataElement));
         return this;
      }
      @Override public HLAvariableArray<DataElement> resize(int size) {
         if (size < 0) throw new EncoderException("negative HLAvariableArray size");
         while (elements.size() > size) elements.remove(elements.size() - 1);
         while (elements.size() < size) elements.add(copyElement(prototype));
         return this;
      }
      @Override public DataElement get(int index) { return elements.get(index); }
      @Override public Iterator<DataElement> iterator() { return elements.iterator(); }
      @Override public byte[] toByteArray() {
         ByteArrayOutputStream output = new ByteArrayOutputStream();
         output.writeBytes(ByteBuffer.allocate(4).putInt(elements.size()).array());
         if (elements.isEmpty()) return output.toByteArray();
         output.writeBytes(new byte[padding(4, getOctetBoundary())]);
         for (int index = 0; index < elements.size(); index++) {
            byte[] encoded = elements.get(index).toByteArray();
            output.writeBytes(encoded);
            if (index + 1 < elements.size()) {
               output.writeBytes(new byte[padding(encoded.length, prototype.getOctetBoundary())]);
            }
         }
         return output.toByteArray();
      }
      @Override public DataElement decode(byte[] bytes) {
         if (bytes.length < 4) throw new DecoderException("truncated HLAvariableArray count");
         int count = ByteBuffer.wrap(bytes).getInt();
         if (count < 0) throw new DecoderException("negative HLAvariableArray count");
         int offset = 4;
         elements.clear();
         if (count > 0) {
            int leading = padding(4, getOctetBoundary());
            requirePadding(bytes, offset, leading);
            offset += leading;
         }
         for (int index = 0; index < count; index++) {
            int length = elementLength(bytes, offset);
            if (length < 0 || bytes.length - offset < length) {
               throw new DecoderException("truncated HLAvariableArray element");
            }
            DataElement element = factory.createElement(index);
            element.decode(java.util.Arrays.copyOfRange(bytes, offset, offset + length));
            elements.add(element);
            offset += length;
            if (index + 1 < count) {
               int elementPadding = padding(length, prototype.getOctetBoundary());
               requirePadding(bytes, offset, elementPadding);
               offset += elementPadding;
            }
         }
         if (offset != bytes.length) throw new DecoderException("trailing HLAvariableArray data");
         return this;
      }
   }

   private static final class UnicodeStringElement implements HLAunicodeString {
      private String value = "";

      UnicodeStringElement() { }

      UnicodeStringElement(String value) { this.value = value; }

      @Override public int getOctetBoundary() { return 4; }
      @Override public int getEncodedLength() { return 4 + value.getBytes(StandardCharsets.UTF_16BE).length; }
      @Override public String getValue() { return value; }
      @Override public HLAunicodeString setValue(String value) { this.value = value; return this; }
      @Override public byte[] toByteArray() {
         byte[] payload = value.getBytes(StandardCharsets.UTF_16BE);
         return ByteBuffer.allocate(4 + payload.length).order(ByteOrder.BIG_ENDIAN)
            .putInt(payload.length / 2).put(payload).array();
      }
      @Override public HLAunicodeString decode(byte[] bytes) {
         if (bytes.length < 4) {
            throw new IllegalArgumentException("truncated HLAunicodeString encoding");
         }
         ByteBuffer buffer = ByteBuffer.wrap(bytes).order(ByteOrder.BIG_ENDIAN);
         int elementCount = buffer.getInt();
         if (elementCount < 0 || elementCount > (Integer.MAX_VALUE / 2)) {
            throw new IllegalArgumentException("invalid HLAunicodeString encoding");
         }
         int payloadLength = elementCount * 2;
         if (bytes.length != 4 + payloadLength) {
            throw new IllegalArgumentException("invalid HLAunicodeString encoding");
         }
         byte[] payload = new byte[payloadLength];
         buffer.get(payload);
         value = new String(payload, StandardCharsets.UTF_16BE);
         return this;
      }
   }
}
