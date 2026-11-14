package org.hla.rti.tck;

import hla.rti1516_2025.encoding.DataElement;
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
import hla.rti1516_2025.encoding.HLAunicodeChar;
import hla.rti1516_2025.encoding.HLAunicodeString;
import hla.rti1516_2025.encoding.HLAunsignedInteger16BE;
import hla.rti1516_2025.encoding.HLAunsignedInteger16LE;
import hla.rti1516_2025.encoding.HLAunsignedInteger32BE;
import hla.rti1516_2025.encoding.HLAunsignedInteger32LE;
import hla.rti1516_2025.encoding.HLAunsignedInteger64BE;
import hla.rti1516_2025.encoding.HLAunsignedInteger64LE;
import java.util.Objects;
import java.util.function.Function;
import java.util.function.Supplier;

/** Exact Java API counterpart of the two-value basic data-element round-trip case. */
final class BasicDataElementEncodeDecodeContractTck {
   private BasicDataElementEncodeDecodeContractTck() {
   }

   static void run(EncoderFactory encoder) throws Exception {
      roundTrip("HLAASCIIchar", (byte) 'A', (byte) 'Z', encoder::createHLAASCIIchar,
         encoder::createHLAASCIIchar, HLAASCIIchar::getValue);
      roundTrip("HLAASCIIstring", "Amber", "1516", encoder::createHLAASCIIstring,
         encoder::createHLAASCIIstring, HLAASCIIstring::getValue);
      roundTrip("HLAboolean", false, true, encoder::createHLAboolean,
         encoder::createHLAboolean, HLAboolean::getValue);

      byte firstOctet = 0x2a;
      byte secondOctet = 0x6c;
      roundTrip("HLAbyte", firstOctet, secondOctet, encoder::createHLAbyte,
         encoder::createHLAbyte, HLAbyte::getValue);
      roundTrip("HLAoctet", firstOctet, secondOctet, encoder::createHLAoctet,
         encoder::createHLAoctet, HLAoctet::getValue);

      roundTrip("HLAfloat32BE", -2.5f, 6.25f, encoder::createHLAfloat32BE,
         encoder::createHLAfloat32BE, HLAfloat32BE::getValue);
      roundTrip("HLAfloat32LE", -2.5f, 6.25f, encoder::createHLAfloat32LE,
         encoder::createHLAfloat32LE, HLAfloat32LE::getValue);
      roundTrip("HLAfloat64BE", -2.5d, 6.25d, encoder::createHLAfloat64BE,
         encoder::createHLAfloat64BE, HLAfloat64BE::getValue);
      roundTrip("HLAfloat64LE", -2.5d, 6.25d, encoder::createHLAfloat64LE,
         encoder::createHLAfloat64LE, HLAfloat64LE::getValue);

      roundTrip("HLAinteger16BE", (short) -2, (short) 17, encoder::createHLAinteger16BE,
         encoder::createHLAinteger16BE, HLAinteger16BE::getValue);
      roundTrip("HLAinteger16LE", (short) -2, (short) 17, encoder::createHLAinteger16LE,
         encoder::createHLAinteger16LE, HLAinteger16LE::getValue);
      roundTrip("HLAinteger32BE", -2, 17, encoder::createHLAinteger32BE,
         encoder::createHLAinteger32BE, HLAinteger32BE::getValue);
      roundTrip("HLAinteger32LE", -2, 17, encoder::createHLAinteger32LE,
         encoder::createHLAinteger32LE, HLAinteger32LE::getValue);
      roundTrip("HLAinteger64BE", -2L, 17L, encoder::createHLAinteger64BE,
         encoder::createHLAinteger64BE, HLAinteger64BE::getValue);
      roundTrip("HLAinteger64LE", -2L, 17L, encoder::createHLAinteger64LE,
         encoder::createHLAinteger64LE, HLAinteger64LE::getValue);

      roundTrip("HLAunsignedInteger16BE", (short) 2, (short) 17,
         encoder::createHLAunsignedInteger16BE, encoder::createHLAunsignedInteger16BE,
         HLAunsignedInteger16BE::getValue);
      roundTrip("HLAunsignedInteger16LE", (short) 2, (short) 17,
         encoder::createHLAunsignedInteger16LE, encoder::createHLAunsignedInteger16LE,
         HLAunsignedInteger16LE::getValue);
      roundTrip("HLAunsignedInteger32BE", 2, 17, encoder::createHLAunsignedInteger32BE,
         encoder::createHLAunsignedInteger32BE, HLAunsignedInteger32BE::getValue);
      roundTrip("HLAunsignedInteger32LE", 2, 17, encoder::createHLAunsignedInteger32LE,
         encoder::createHLAunsignedInteger32LE, HLAunsignedInteger32LE::getValue);
      roundTrip("HLAunsignedInteger64BE", 2L, 17L, encoder::createHLAunsignedInteger64BE,
         encoder::createHLAunsignedInteger64BE, HLAunsignedInteger64BE::getValue);
      roundTrip("HLAunsignedInteger64LE", 2L, 17L, encoder::createHLAunsignedInteger64LE,
         encoder::createHLAunsignedInteger64LE, HLAunsignedInteger64LE::getValue);

      roundTrip("HLAoctetPairBE", (short) 0x2a6c, (short) 0x6c2a,
         encoder::createHLAoctetPairBE, encoder::createHLAoctetPairBE,
         HLAoctetPairBE::getValue);
      roundTrip("HLAoctetPairLE", (short) 0x2a6c, (short) 0x6c2a,
         encoder::createHLAoctetPairLE, encoder::createHLAoctetPairLE,
         HLAoctetPairLE::getValue);
      roundTrip("HLAunicodeChar", (short) 0x03a9, (short) 0x2603,
         encoder::createHLAunicodeChar, encoder::createHLAunicodeChar,
         HLAunicodeChar::getValue);
      roundTrip("HLAunicodeString", "Amber", "1516", encoder::createHLAunicodeString,
         encoder::createHLAunicodeString, HLAunicodeString::getValue);
   }

   private static <E extends DataElement, V> void roundTrip(
         String type,
         V first,
         V second,
         Function<V, E> create,
         Supplier<E> createEmpty,
         Function<E, V> getValue) throws Exception {
      oneRoundTrip(type, "first", first, create, createEmpty, getValue);
      oneRoundTrip(type, "updated", second, create, createEmpty, getValue);
   }

   private static <E extends DataElement, V> void oneRoundTrip(
         String type,
         String sample,
         V value,
         Function<V, E> create,
         Supplier<E> createEmpty,
         Function<E, V> getValue) throws Exception {
      E encoded = create.apply(value);
      E decoded = createEmpty.get();
      decoded.decode(encoded.toByteArray());
      if (!Objects.equals(getValue.apply(decoded), value)) {
         throw new AssertionError(type + " encode/decode did not preserve the " + sample + " value");
      }
   }
}
