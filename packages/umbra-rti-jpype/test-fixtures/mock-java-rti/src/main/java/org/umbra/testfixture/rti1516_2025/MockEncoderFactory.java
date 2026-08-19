package org.umbra.testfixture.rti1516_2025;

import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.DecoderException;
import hla.rti1516_2025.encoding.HLAboolean;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.encoding.HLAunicodeString;
import hla.rti1516_2025.encoding.HLAunsignedInteger32BE;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;

/**
 * Small real Java implementation of the four basic encodings Umbra C++ also
 * implements. It lets the JPype test prove factory dispatch and byte copying.
 */
public final class MockEncoderFactory implements EncoderFactory {
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
   public HLAboolean createHLAboolean() { return new BooleanElement(); }

   @Override
   public HLAboolean createHLAboolean(boolean value) { return new BooleanElement(value); }

   @Override
   public HLAunicodeString createHLAunicodeString() { return new UnicodeStringElement(); }

   @Override
   public HLAunicodeString createHLAunicodeString(String value) { return new UnicodeStringElement(value); }

   private static void requireLength(byte[] bytes, int expected) {
      if (bytes.length != expected) {
         throw new IllegalArgumentException("invalid fixed-width encoding length");
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
