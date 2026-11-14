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
import hla.rti1516_2025.encoding.HLAunicodeChar;
import hla.rti1516_2025.encoding.HLAunicodeString;
import hla.rti1516_2025.encoding.HLAunsignedInteger16BE;
import hla.rti1516_2025.encoding.HLAunsignedInteger16LE;
import hla.rti1516_2025.encoding.HLAunsignedInteger32BE;
import hla.rti1516_2025.encoding.HLAunsignedInteger32LE;
import hla.rti1516_2025.encoding.HLAunsignedInteger64BE;
import hla.rti1516_2025.encoding.HLAunsignedInteger64LE;
import java.util.Objects;
import java.util.function.BiConsumer;
import java.util.function.Function;
import java.util.function.Supplier;

/** Exercises Table 22 getValue/setValue behavior through the official Java API. */
final class BasicDataElementValueOperationsTck {
   private BasicDataElementValueOperationsTck() { }

   static void run(EncoderFactory encoder) {
      value(encoder::createHLAASCIIchar, HLAASCIIchar::setValue,
         HLAASCIIchar::getValue, (byte) 'A', (byte) 'Z', "HLAASCIIchar");
      value(encoder::createHLAASCIIstring, HLAASCIIstring::setValue,
         HLAASCIIstring::getValue, "Amber", "1516", "HLAASCIIstring");
      value(encoder::createHLAboolean, HLAboolean::setValue,
         HLAboolean::getValue, false, true, "HLAboolean");
      value(encoder::createHLAbyte, HLAbyte::setValue,
         HLAbyte::getValue, (byte) 0x2a, (byte) 0x6c, "HLAbyte");
      value(encoder::createHLAoctet, HLAoctet::setValue,
         HLAoctet::getValue, (byte) 0x2a, (byte) 0x6c, "HLAoctet");

      value(encoder::createHLAfloat32BE, HLAfloat32BE::setValue,
         HLAfloat32BE::getValue, -2.5F, 6.25F, "HLAfloat32BE");
      value(encoder::createHLAfloat32LE, HLAfloat32LE::setValue,
         HLAfloat32LE::getValue, -2.5F, 6.25F, "HLAfloat32LE");
      value(encoder::createHLAfloat64BE, HLAfloat64BE::setValue,
         HLAfloat64BE::getValue, -2.5, 6.25, "HLAfloat64BE");
      value(encoder::createHLAfloat64LE, HLAfloat64LE::setValue,
         HLAfloat64LE::getValue, -2.5, 6.25, "HLAfloat64LE");

      value(encoder::createHLAinteger16BE, HLAinteger16BE::setValue,
         HLAinteger16BE::getValue, (short) -2, (short) 17, "HLAinteger16BE");
      value(encoder::createHLAinteger16LE, HLAinteger16LE::setValue,
         HLAinteger16LE::getValue, (short) -2, (short) 17, "HLAinteger16LE");
      value(encoder::createHLAinteger32BE, HLAinteger32BE::setValue,
         HLAinteger32BE::getValue, -2, 17, "HLAinteger32BE");
      value(encoder::createHLAinteger32LE, HLAinteger32LE::setValue,
         HLAinteger32LE::getValue, -2, 17, "HLAinteger32LE");
      value(encoder::createHLAinteger64BE, HLAinteger64BE::setValue,
         HLAinteger64BE::getValue, -2L, 17L, "HLAinteger64BE");
      value(encoder::createHLAinteger64LE, HLAinteger64LE::setValue,
         HLAinteger64LE::getValue, -2L, 17L, "HLAinteger64LE");

      value(encoder::createHLAunsignedInteger16BE, HLAunsignedInteger16BE::setValue,
         HLAunsignedInteger16BE::getValue, (short) 2, (short) 17, "HLAunsignedInteger16BE");
      value(encoder::createHLAunsignedInteger16LE, HLAunsignedInteger16LE::setValue,
         HLAunsignedInteger16LE::getValue, (short) 2, (short) 17, "HLAunsignedInteger16LE");
      value(encoder::createHLAunsignedInteger32BE, HLAunsignedInteger32BE::setValue,
         HLAunsignedInteger32BE::getValue, 2, 17, "HLAunsignedInteger32BE");
      value(encoder::createHLAunsignedInteger32LE, HLAunsignedInteger32LE::setValue,
         HLAunsignedInteger32LE::getValue, 2, 17, "HLAunsignedInteger32LE");
      value(encoder::createHLAunsignedInteger64BE, HLAunsignedInteger64BE::setValue,
         HLAunsignedInteger64BE::getValue, 2L, 17L, "HLAunsignedInteger64BE");
      value(encoder::createHLAunsignedInteger64LE, HLAunsignedInteger64LE::setValue,
         HLAunsignedInteger64LE::getValue, 2L, 17L, "HLAunsignedInteger64LE");

      value(encoder::createHLAoctetPairBE, HLAoctetPairBE::setValue,
         HLAoctetPairBE::getValue, (short) 0x2a6c, (short) 0x6c2a, "HLAoctetPairBE");
      value(encoder::createHLAoctetPairLE, HLAoctetPairLE::setValue,
         HLAoctetPairLE::getValue, (short) 0x2a6c, (short) 0x6c2a, "HLAoctetPairLE");
      value(encoder::createHLAunicodeChar, HLAunicodeChar::setValue,
         HLAunicodeChar::getValue, (short) '\u03a9', (short) '\u2603', "HLAunicodeChar");
      value(encoder::createHLAunicodeString, HLAunicodeString::setValue,
         HLAunicodeString::getValue, "Amber", "1516", "HLAunicodeString");
   }

   private static <E, V> void value(
         Supplier<E> factory,
         BiConsumer<E, V> setValue,
         Function<E, V> getValue,
         V first,
         V second,
         String name) {
      E element = Objects.requireNonNull(factory.get(), name + " factory returned null");
      setValue.accept(element, first);
      JavaTckSupport.check(Objects.equals(first, getValue.apply(element)),
         name + " getValue did not return its first setValue");
      setValue.accept(element, second);
      JavaTckSupport.check(Objects.equals(second, getValue.apply(element)),
         name + " getValue did not return its updated setValue");
   }
}
