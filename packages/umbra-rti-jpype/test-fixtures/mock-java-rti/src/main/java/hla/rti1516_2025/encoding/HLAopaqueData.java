package hla.rti1516_2025.encoding;

public interface HLAopaqueData extends DataElement, Iterable<Byte> {
   int size();

   byte get(int index);

   byte[] getValue();

   HLAopaqueData setValue(byte[] value);

   HLAopaqueData decode(byte[] bytes);
}
