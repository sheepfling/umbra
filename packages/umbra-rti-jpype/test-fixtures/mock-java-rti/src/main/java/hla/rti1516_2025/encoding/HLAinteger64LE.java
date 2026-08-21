package hla.rti1516_2025.encoding;

public interface HLAinteger64LE extends DataElement {
   long getValue();

   HLAinteger64LE setValue(long value);

   @Override
   HLAinteger64LE decode(byte[] bytes);
}
