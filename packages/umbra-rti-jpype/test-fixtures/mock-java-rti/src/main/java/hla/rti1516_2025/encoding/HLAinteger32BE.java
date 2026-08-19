package hla.rti1516_2025.encoding;

public interface HLAinteger32BE extends DataElement {
   int getValue();

   HLAinteger32BE setValue(int value);

   @Override
   HLAinteger32BE decode(byte[] bytes);
}
