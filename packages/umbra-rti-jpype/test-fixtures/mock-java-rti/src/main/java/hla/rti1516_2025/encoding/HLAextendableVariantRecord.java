package hla.rti1516_2025.encoding;

public interface HLAextendableVariantRecord extends DataElement {
   HLAextendableVariantRecord setVariant(DataElement discriminant, DataElement value);
   HLAextendableVariantRecord setDiscriminant(DataElement discriminant);
   DataElement getDiscriminant();
   DataElement getValue();
}
