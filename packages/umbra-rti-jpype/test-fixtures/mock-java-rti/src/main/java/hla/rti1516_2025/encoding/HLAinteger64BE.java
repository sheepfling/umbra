package hla.rti1516_2025.encoding;

public interface HLAinteger64BE extends DataElement {
   long getValue();

   HLAinteger64BE setValue(long value);

   @Override
   HLAinteger64BE decode(byte[] bytes);
}
