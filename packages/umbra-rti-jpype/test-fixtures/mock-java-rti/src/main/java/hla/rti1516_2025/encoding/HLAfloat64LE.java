package hla.rti1516_2025.encoding;

public interface HLAfloat64LE extends DataElement {
   double getValue();

   HLAfloat64LE setValue(double value);

   @Override
   HLAfloat64LE decode(byte[] bytes);
}
