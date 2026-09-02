package org.hla.rti.tck;

import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAASCIIchar;
import hla.rti1516_2025.encoding.HLAASCIIstring;
import hla.rti1516_2025.encoding.HLAboolean;
import hla.rti1516_2025.encoding.HLAbyte;
import hla.rti1516_2025.encoding.HLAfloat32BE;
import hla.rti1516_2025.encoding.HLAfloat32LE;
import hla.rti1516_2025.encoding.HLAfloat64BE;
import hla.rti1516_2025.encoding.HLAfloat64LE;
import hla.rti1516_2025.encoding.HLAinteger16BE;
import hla.rti1516_2025.encoding.HLAinteger16LE;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.encoding.HLAinteger32LE;
import hla.rti1516_2025.encoding.HLAinteger64BE;
import hla.rti1516_2025.encoding.HLAinteger64LE;
import hla.rti1516_2025.encoding.HLAoctet;
import hla.rti1516_2025.encoding.HLAoctetPairBE;
import hla.rti1516_2025.encoding.HLAoctetPairLE;
import hla.rti1516_2025.encoding.HLAopaqueData;
import hla.rti1516_2025.encoding.HLAunicodeChar;
import hla.rti1516_2025.encoding.HLAunicodeString;
import hla.rti1516_2025.encoding.HLAunsignedInteger16BE;
import hla.rti1516_2025.encoding.HLAunsignedInteger16LE;
import hla.rti1516_2025.encoding.HLAunsignedInteger32BE;
import hla.rti1516_2025.encoding.HLAunsignedInteger32LE;
import hla.rti1516_2025.encoding.HLAunsignedInteger64BE;
import hla.rti1516_2025.encoding.HLAunsignedInteger64LE;
import java.util.Arrays;

/**
 * Standard 1516.2 basic-data-element vectors.
 *
 * <p>This class is intentionally an API-only test boundary. It receives the
 * standard {@link EncoderFactory}; it does not know how the provider was
 * loaded and does not import a provider implementation type.
 */
final class BasicDataElementsTck {
   private BasicDataElementsTck() {
   }

   static void run(EncoderFactory encoder) throws Exception {
      fixedWidthIntegers(encoder);
      booleansAndOctets(encoder);
      floatingPoint(encoder);
      text(encoder);
      opaqueDataAndOctetPairs(encoder);
   }

   private static void fixedWidthIntegers(EncoderFactory encoder) throws Exception {
      HLAinteger16BE signed16BE = encoder.createHLAinteger16BE((short) -2);
      checkBytes(signed16BE.toByteArray(), 0xff, 0xfe);
      HLAinteger16BE decodedSigned16BE = encoder.createHLAinteger16BE();
      decodedSigned16BE.decode(signed16BE.toByteArray());
      check(decodedSigned16BE.getValue() == -2, "signed 16-bit big-endian value changed");

      HLAinteger16LE signed16LE = encoder.createHLAinteger16LE((short) -2);
      checkBytes(signed16LE.toByteArray(), 0xfe, 0xff);
      HLAinteger16LE decodedSigned16LE = encoder.createHLAinteger16LE();
      decodedSigned16LE.decode(signed16LE.toByteArray());
      check(decodedSigned16LE.getValue() == -2, "signed 16-bit little-endian value changed");

      HLAunsignedInteger16BE unsigned16BE = encoder.createHLAunsignedInteger16BE((short) 0x1234);
      checkBytes(unsigned16BE.toByteArray(), 0x12, 0x34);
      HLAunsignedInteger16BE decodedUnsigned16BE = encoder.createHLAunsignedInteger16BE();
      decodedUnsigned16BE.decode(unsigned16BE.toByteArray());
      check((decodedUnsigned16BE.getValue() & 0xffff) == 0x1234,
         "unsigned 16-bit big-endian value changed");

      HLAunsignedInteger16LE unsigned16LE = encoder.createHLAunsignedInteger16LE((short) 0x1234);
      checkBytes(unsigned16LE.toByteArray(), 0x34, 0x12);
      HLAunsignedInteger16LE decodedUnsigned16LE = encoder.createHLAunsignedInteger16LE();
      decodedUnsigned16LE.decode(unsigned16LE.toByteArray());
      check((decodedUnsigned16LE.getValue() & 0xffff) == 0x1234,
         "unsigned 16-bit little-endian value changed");

      HLAinteger32BE signed32BE = encoder.createHLAinteger32BE(0x1234abcd);
      checkBytes(signed32BE.toByteArray(), 0x12, 0x34, 0xab, 0xcd);
      HLAinteger32BE decodedSigned32BE = encoder.createHLAinteger32BE();
      decodedSigned32BE.decode(signed32BE.toByteArray());
      check(decodedSigned32BE.getValue() == 0x1234abcd,
         "signed 32-bit big-endian value changed");

      HLAinteger32LE signed32LE = encoder.createHLAinteger32LE(0x1234abcd);
      checkBytes(signed32LE.toByteArray(), 0xcd, 0xab, 0x34, 0x12);
      HLAinteger32LE decodedSigned32LE = encoder.createHLAinteger32LE();
      decodedSigned32LE.decode(signed32LE.toByteArray());
      check(decodedSigned32LE.getValue() == 0x1234abcd,
         "signed 32-bit little-endian value changed");

      HLAunsignedInteger32BE unsigned32BE = encoder.createHLAunsignedInteger32BE(0x1234abcd);
      checkBytes(unsigned32BE.toByteArray(), 0x12, 0x34, 0xab, 0xcd);
      HLAunsignedInteger32BE decodedUnsigned32BE = encoder.createHLAunsignedInteger32BE();
      decodedUnsigned32BE.decode(unsigned32BE.toByteArray());
      check(decodedUnsigned32BE.getValue() == 0x1234abcd,
         "unsigned 32-bit big-endian value changed");

      HLAunsignedInteger32LE unsigned32LE = encoder.createHLAunsignedInteger32LE(0x1234abcd);
      checkBytes(unsigned32LE.toByteArray(), 0xcd, 0xab, 0x34, 0x12);
      HLAunsignedInteger32LE decodedUnsigned32LE = encoder.createHLAunsignedInteger32LE();
      decodedUnsigned32LE.decode(unsigned32LE.toByteArray());
      check(decodedUnsigned32LE.getValue() == 0x1234abcd,
         "unsigned 32-bit little-endian value changed");

      HLAinteger64BE signed64BE = encoder.createHLAinteger64BE(-2L);
      checkBytes(signed64BE.toByteArray(), 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe);
      HLAinteger64BE decodedSigned64BE = encoder.createHLAinteger64BE();
      decodedSigned64BE.decode(signed64BE.toByteArray());
      check(decodedSigned64BE.getValue() == -2L, "signed 64-bit big-endian value changed");

      HLAinteger64LE signed64LE = encoder.createHLAinteger64LE(-2L);
      checkBytes(signed64LE.toByteArray(), 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff);
      HLAinteger64LE decodedSigned64LE = encoder.createHLAinteger64LE();
      decodedSigned64LE.decode(signed64LE.toByteArray());
      check(decodedSigned64LE.getValue() == -2L, "signed 64-bit little-endian value changed");

      long unsigned64Value = 0x0123456789abcdefL;
      HLAunsignedInteger64BE unsigned64BE = encoder.createHLAunsignedInteger64BE(unsigned64Value);
      checkBytes(unsigned64BE.toByteArray(), 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef);
      HLAunsignedInteger64BE decodedUnsigned64BE = encoder.createHLAunsignedInteger64BE();
      decodedUnsigned64BE.decode(unsigned64BE.toByteArray());
      check(decodedUnsigned64BE.getValue() == unsigned64Value,
         "unsigned 64-bit big-endian value changed");

      HLAunsignedInteger64LE unsigned64LE = encoder.createHLAunsignedInteger64LE(unsigned64Value);
      checkBytes(unsigned64LE.toByteArray(), 0xef, 0xcd, 0xab, 0x89, 0x67, 0x45, 0x23, 0x01);
      HLAunsignedInteger64LE decodedUnsigned64LE = encoder.createHLAunsignedInteger64LE();
      decodedUnsigned64LE.decode(unsigned64LE.toByteArray());
      check(decodedUnsigned64LE.getValue() == unsigned64Value,
         "unsigned 64-bit little-endian value changed");

      expectFailure(() -> decodedSigned16BE.decode(new byte[] {0x01}),
         "DecoderException", "truncated 16-bit input was accepted");
      expectFailure(() -> decodedSigned64BE.decode(new byte[] {0x01, 0x02, 0x03}),
         "DecoderException", "truncated 64-bit input was accepted");
   }

   private static void booleansAndOctets(EncoderFactory encoder) throws Exception {
      HLAboolean trueValue = encoder.createHLAboolean(true);
      HLAboolean falseValue = encoder.createHLAboolean(false);
      checkBytes(trueValue.toByteArray(), 0, 0, 0, 1);
      checkBytes(falseValue.toByteArray(), 0, 0, 0, 0);
      HLAboolean decodedTrue = encoder.createHLAboolean();
      decodedTrue.decode(trueValue.toByteArray());
      check(decodedTrue.getValue(), "true boolean decoded as false");
      HLAboolean decodedFalse = encoder.createHLAboolean();
      decodedFalse.decode(falseValue.toByteArray());
      check(!decodedFalse.getValue(), "false boolean decoded as true");
      expectFailure(() -> decodedFalse.decode(new byte[] {0, 0, 0, 2}),
         "DecoderException", "invalid boolean encoding was accepted");

      HLAoctet octet = encoder.createHLAoctet((byte) 0xa5);
      HLAbyte byteValue = encoder.createHLAbyte((byte) 0xa5);
      checkBytes(octet.toByteArray(), 0xa5);
      checkBytes(byteValue.toByteArray(), 0xa5);
      HLAoctet decodedOctet = encoder.createHLAoctet();
      decodedOctet.decode(octet.toByteArray());
      check((decodedOctet.getValue() & 0xff) == 0xa5, "HLAoctet value changed");
      HLAbyte decodedByte = encoder.createHLAbyte();
      decodedByte.decode(byteValue.toByteArray());
      check((decodedByte.getValue() & 0xff) == 0xa5, "HLAbyte value changed");
      expectFailure(() -> decodedOctet.decode(new byte[] {(byte) 0xa5, 0}),
         "DecoderException", "trailing octet data was accepted");
   }

   private static void floatingPoint(EncoderFactory encoder) throws Exception {
      HLAfloat32BE singleBE = encoder.createHLAfloat32BE(-2.5f);
      HLAfloat32LE singleLE = encoder.createHLAfloat32LE(-2.5f);
      checkBytes(singleBE.toByteArray(), 0xc0, 0x20, 0, 0);
      checkBytes(singleLE.toByteArray(), 0, 0, 0x20, 0xc0);
      HLAfloat32BE decodedSingleBE = encoder.createHLAfloat32BE();
      decodedSingleBE.decode(singleBE.toByteArray());
      check(Float.floatToIntBits(decodedSingleBE.getValue()) == Float.floatToIntBits(-2.5f),
         "32-bit big-endian float changed");
      HLAfloat32LE decodedSingleLE = encoder.createHLAfloat32LE();
      decodedSingleLE.decode(singleLE.toByteArray());
      check(Float.floatToIntBits(decodedSingleLE.getValue()) == Float.floatToIntBits(-2.5f),
         "32-bit little-endian float changed");

      HLAfloat64BE doubleBE = encoder.createHLAfloat64BE(-2.5d);
      HLAfloat64LE doubleLE = encoder.createHLAfloat64LE(-2.5d);
      checkBytes(doubleBE.toByteArray(), 0xc0, 0x04, 0, 0, 0, 0, 0, 0);
      checkBytes(doubleLE.toByteArray(), 0, 0, 0, 0, 0, 0, 0x04, 0xc0);
      HLAfloat64BE decodedDoubleBE = encoder.createHLAfloat64BE();
      decodedDoubleBE.decode(doubleBE.toByteArray());
      check(Double.doubleToLongBits(decodedDoubleBE.getValue()) == Double.doubleToLongBits(-2.5d),
         "64-bit big-endian float changed");
      HLAfloat64LE decodedDoubleLE = encoder.createHLAfloat64LE();
      decodedDoubleLE.decode(doubleLE.toByteArray());
      check(Double.doubleToLongBits(decodedDoubleLE.getValue()) == Double.doubleToLongBits(-2.5d),
         "64-bit little-endian float changed");

      expectFailure(() -> decodedSingleBE.decode(new byte[] {(byte) 0xc0, 0x20, 0}),
         "DecoderException", "truncated 32-bit float input was accepted");
      expectFailure(() -> decodedDoubleLE.decode(new byte[] {(byte) 0xc0, 0x04, 0, 0, 0, 0, 0}),
         "DecoderException", "truncated 64-bit float input was accepted");
   }

   private static void text(EncoderFactory encoder) throws Exception {
      HLAASCIIchar asciiCharacter = encoder.createHLAASCIIchar((byte) 'A');
      checkBytes(asciiCharacter.toByteArray(), 0x41);
      HLAASCIIchar decodedAsciiCharacter = encoder.createHLAASCIIchar();
      decodedAsciiCharacter.decode(asciiCharacter.toByteArray());
      check(decodedAsciiCharacter.getValue() == (byte) 'A', "ASCII character changed");

      String asciiText = "A\u0000Z";
      HLAASCIIstring asciiString = encoder.createHLAASCIIstring(asciiText);
      checkBytes(asciiString.toByteArray(), 0, 0, 0, 3, 0x41, 0, 0x5a);
      check(asciiString.getEncodedLength() == 7 && asciiString.getOctetBoundary() == 4,
         "ASCII string layout changed");
      HLAASCIIstring decodedAsciiString = encoder.createHLAASCIIstring();
      decodedAsciiString.decode(asciiString.toByteArray());
      check(asciiText.equals(decodedAsciiString.getValue()), "ASCII string changed");

      expectFailure(() -> encoder.createHLAASCIIchar((byte) 0x80).toByteArray(),
         "EncoderException", "non-ASCII character was encoded");
      expectFailure(() -> encoder.createHLAASCIIstring("\u0080").toByteArray(),
         "EncoderException", "non-ASCII string was encoded");
      expectFailure(() -> decodedAsciiString.decode(new byte[] {0, 0, 0, 1}),
         "DecoderException", "truncated ASCII string was accepted");

      HLAunicodeChar unicodeCharacter = encoder.createHLAunicodeChar((short) 0x03a9);
      checkBytes(unicodeCharacter.toByteArray(), 0x03, 0xa9);
      HLAunicodeChar decodedUnicodeCharacter = encoder.createHLAunicodeChar();
      decodedUnicodeCharacter.decode(unicodeCharacter.toByteArray());
      check((decodedUnicodeCharacter.getValue() & 0xffff) == 0x03a9,
         "Unicode character changed");

      String unicodeText = "A\ud83d\ude00";
      HLAunicodeString unicodeString = encoder.createHLAunicodeString(unicodeText);
      checkBytes(unicodeString.toByteArray(),
         0, 0, 0, 3, 0, 0x41, 0xd8, 0x3d, 0xde, 0);
      check(unicodeString.getEncodedLength() == 10 && unicodeString.getOctetBoundary() == 4,
         "Unicode string layout changed");
      HLAunicodeString decodedUnicodeString = encoder.createHLAunicodeString();
      decodedUnicodeString.decode(unicodeString.toByteArray());
      check(unicodeText.equals(decodedUnicodeString.getValue()), "Unicode string changed");
      expectFailure(() -> encoder.createHLAunicodeString("\ud800").toByteArray(),
         "EncoderException", "unmatched Unicode surrogate was encoded");
      expectFailure(() -> decodedUnicodeString.decode(new byte[] {0, 0, 0, 2, (byte) 0xd8, 0}),
         "DecoderException", "malformed Unicode surrogate was accepted");
   }

   private static void opaqueDataAndOctetPairs(EncoderFactory encoder) throws Exception {
      byte[] payload = new byte[] {(byte) 0xa5, 0, 0x5a};
      HLAopaqueData opaque = encoder.createHLAopaqueData(payload);
      checkBytes(opaque.toByteArray(), 0, 0, 0, 3, 0xa5, 0, 0x5a);
      check(opaque.getEncodedLength() == 7, "opaque-data encoded length changed");
      check(opaque.getOctetBoundary() == 4, "opaque-data octet boundary changed");
      check(opaque.size() == payload.length, "opaque-data payload length changed");
      check(Arrays.equals(payload, opaque.getValue()), "opaque-data payload changed");
      HLAopaqueData decodedOpaque = encoder.createHLAopaqueData();
      decodedOpaque.decode(opaque.toByteArray());
      check(Arrays.equals(payload, decodedOpaque.getValue()), "opaque-data decode changed payload");
      expectFailure(() -> decodedOpaque.decode(new byte[] {0, 0, 0, 2, 0x12}),
         "DecoderException", "truncated opaque-data payload was accepted");

      HLAoctetPairBE pairBE = encoder.createHLAoctetPairBE((short) 0x1234);
      HLAoctetPairLE pairLE = encoder.createHLAoctetPairLE((short) 0x1234);
      checkBytes(pairBE.toByteArray(), 0x12, 0x34);
      checkBytes(pairLE.toByteArray(), 0x34, 0x12);
      HLAoctetPairBE decodedPairBE = encoder.createHLAoctetPairBE();
      decodedPairBE.decode(pairBE.toByteArray());
      check((decodedPairBE.getValue() & 0xffff) == 0x1234, "BE octet pair changed");
      HLAoctetPairLE decodedPairLE = encoder.createHLAoctetPairLE();
      decodedPairLE.decode(pairLE.toByteArray());
      check((decodedPairLE.getValue() & 0xffff) == 0x1234, "LE octet pair changed");
   }

   private static void checkBytes(byte[] actual, int... expected) {
      if (actual.length != expected.length) {
         throw new AssertionError("encoded length " + actual.length + " != " + expected.length);
      }
      for (int i = 0; i < expected.length; i++) {
         if ((actual[i] & 0xff) != expected[i]) {
            throw new AssertionError("encoded octet " + i + " was "
               + Integer.toHexString(actual[i] & 0xff) + " instead of "
               + Integer.toHexString(expected[i]));
         }
      }
   }

   private static void check(boolean condition, String message) {
      if (!condition) {
         throw new AssertionError(message);
      }
   }

   private static void expectFailure(CheckedOperation operation, String expectedType,
         String message) throws Exception {
      try {
         operation.run();
      } catch (Throwable error) {
         Throwable current = error;
         while (current != null) {
            if (expectedType.equals(current.getClass().getSimpleName())) {
               return;
            }
            current = current.getCause();
         }
         throw new AssertionError(message + "; got " + error, error);
      }
      throw new AssertionError(message);
   }

   @FunctionalInterface
   private interface CheckedOperation {
      void run() throws Exception;
   }
}
