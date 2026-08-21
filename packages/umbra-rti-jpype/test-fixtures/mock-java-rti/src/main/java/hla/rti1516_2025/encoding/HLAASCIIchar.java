package hla.rti1516_2025.encoding;

public interface HLAASCIIchar extends DataElement {
   byte getValue();

   HLAASCIIchar setValue(byte value);

   HLAASCIIchar decode(byte[] bytes);
}
