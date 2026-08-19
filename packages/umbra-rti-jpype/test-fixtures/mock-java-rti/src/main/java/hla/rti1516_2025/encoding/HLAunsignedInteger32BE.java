package hla.rti1516_2025.encoding;

public interface HLAunsignedInteger32BE extends DataElement {
   int getValue();

   HLAunsignedInteger32BE setValue(int value);

   @Override
   HLAunsignedInteger32BE decode(byte[] bytes);
}
