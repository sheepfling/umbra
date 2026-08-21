package hla.rti1516_2025.encoding;

public interface HLAunsignedInteger64LE extends DataElement {
   long getValue();

   HLAunsignedInteger64LE setValue(long value);

   @Override
   HLAunsignedInteger64LE decode(byte[] bytes);
}
