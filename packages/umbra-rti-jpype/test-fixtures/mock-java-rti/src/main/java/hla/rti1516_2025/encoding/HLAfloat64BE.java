package hla.rti1516_2025.encoding;

public interface HLAfloat64BE extends DataElement {
   double getValue();

   HLAfloat64BE setValue(double value);

   @Override
   HLAfloat64BE decode(byte[] bytes);
}
