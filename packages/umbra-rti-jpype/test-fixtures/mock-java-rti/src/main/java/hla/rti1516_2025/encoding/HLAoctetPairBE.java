package hla.rti1516_2025.encoding;

public interface HLAoctetPairBE extends DataElement {
   short getValue();

   HLAoctetPairBE setValue(short value);

   HLAoctetPairBE decode(byte[] bytes);
}
