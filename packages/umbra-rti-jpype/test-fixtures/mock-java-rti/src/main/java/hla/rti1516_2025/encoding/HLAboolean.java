package hla.rti1516_2025.encoding;

public interface HLAboolean extends DataElement {
   boolean getValue();

   HLAboolean setValue(boolean value);

   @Override
   HLAboolean decode(byte[] bytes);
}
