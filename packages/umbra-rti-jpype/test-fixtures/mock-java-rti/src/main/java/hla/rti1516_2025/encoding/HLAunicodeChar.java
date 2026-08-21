package hla.rti1516_2025.encoding;

public interface HLAunicodeChar extends DataElement {
   short getValue();

   HLAunicodeChar setValue(short value);

   HLAunicodeChar decode(byte[] bytes);
}
