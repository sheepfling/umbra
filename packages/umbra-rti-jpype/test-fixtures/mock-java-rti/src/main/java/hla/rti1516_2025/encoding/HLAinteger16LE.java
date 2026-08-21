package hla.rti1516_2025.encoding;

public interface HLAinteger16LE extends DataElement {
   short getValue();

   HLAinteger16LE setValue(short value);

   @Override
   HLAinteger16LE decode(byte[] bytes);
}
