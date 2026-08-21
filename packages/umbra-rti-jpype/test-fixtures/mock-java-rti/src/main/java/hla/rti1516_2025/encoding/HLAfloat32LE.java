package hla.rti1516_2025.encoding;

public interface HLAfloat32LE extends DataElement {
   float getValue();

   HLAfloat32LE setValue(float value);

   @Override
   HLAfloat32LE decode(byte[] bytes);
}
