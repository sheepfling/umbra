package hla.rti1516_2025.encoding;

public interface HLAunsignedInteger64BE extends DataElement {
   long getValue();

   HLAunsignedInteger64BE setValue(long value);

   @Override
   HLAunsignedInteger64BE decode(byte[] bytes);
}
