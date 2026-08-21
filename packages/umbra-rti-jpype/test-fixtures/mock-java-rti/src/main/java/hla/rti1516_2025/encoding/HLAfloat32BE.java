package hla.rti1516_2025.encoding;

public interface HLAfloat32BE extends DataElement {
   float getValue();

   HLAfloat32BE setValue(float value);

   @Override
   HLAfloat32BE decode(byte[] bytes);
}
