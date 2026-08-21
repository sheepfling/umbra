package hla.rti1516_2025.encoding;

public interface HLAinteger32LE extends DataElement {
   int getValue();

   HLAinteger32LE setValue(int value);

   @Override
   HLAinteger32LE decode(byte[] bytes);
}
