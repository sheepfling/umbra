package hla.rti1516_2025.encoding;

public interface HLAunsignedInteger16LE extends DataElement {
   short getValue();

   HLAunsignedInteger16LE setValue(short value);

   HLAunsignedInteger16LE decode(byte[] bytes);
}
