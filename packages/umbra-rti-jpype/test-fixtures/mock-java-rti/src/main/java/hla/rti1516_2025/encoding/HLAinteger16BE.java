package hla.rti1516_2025.encoding;

public interface HLAinteger16BE extends DataElement {
   short getValue();

   HLAinteger16BE setValue(short value);

   @Override
   HLAinteger16BE decode(byte[] bytes);
}
