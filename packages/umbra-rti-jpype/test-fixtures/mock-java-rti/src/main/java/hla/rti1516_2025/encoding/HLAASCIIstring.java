package hla.rti1516_2025.encoding;

public interface HLAASCIIstring extends DataElement {
   String getValue();

   HLAASCIIstring setValue(String value);

   HLAASCIIstring decode(byte[] bytes);
}
