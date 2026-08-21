package hla.rti1516_2025.encoding;

public interface HLAunsignedInteger16BE extends DataElement {
   short getValue();

   HLAunsignedInteger16BE setValue(short value);

   @Override
   HLAunsignedInteger16BE decode(byte[] bytes);
}
