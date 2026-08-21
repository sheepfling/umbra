package hla.rti1516_2025.encoding;

public interface HLAbyte extends DataElement {
   byte getValue();

   HLAbyte setValue(byte value);

   HLAbyte decode(byte[] bytes);
}
