package hla.rti1516_2025.encoding;

public interface HLAunicodeString extends DataElement {
   String getValue();

   HLAunicodeString setValue(String value);

   @Override
   HLAunicodeString decode(byte[] bytes);
}
