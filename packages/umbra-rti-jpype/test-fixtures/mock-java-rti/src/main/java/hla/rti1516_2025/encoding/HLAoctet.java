package hla.rti1516_2025.encoding;

public interface HLAoctet extends DataElement {
   byte getValue();

   HLAoctet setValue(byte value);

   HLAoctet decode(byte[] bytes);
}
