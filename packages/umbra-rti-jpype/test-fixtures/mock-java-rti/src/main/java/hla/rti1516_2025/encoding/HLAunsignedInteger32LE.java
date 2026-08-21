package hla.rti1516_2025.encoding;

public interface HLAunsignedInteger32LE extends DataElement {
   int getValue();

   HLAunsignedInteger32LE setValue(int value);

   @Override
   HLAunsignedInteger32LE decode(byte[] bytes);
}
