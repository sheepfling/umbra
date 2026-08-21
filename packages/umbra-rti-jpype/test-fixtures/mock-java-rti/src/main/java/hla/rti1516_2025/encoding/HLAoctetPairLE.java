package hla.rti1516_2025.encoding;

public interface HLAoctetPairLE extends DataElement {
   short getValue();

   HLAoctetPairLE setValue(short value);

   HLAoctetPairLE decode(byte[] bytes);
}
