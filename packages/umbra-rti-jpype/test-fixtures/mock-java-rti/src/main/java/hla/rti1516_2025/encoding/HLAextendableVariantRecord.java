package hla.rti1516_2025.encoding;

public interface HLAextendableVariantRecord<T extends DataElement> extends DataElement {
   HLAextendableVariantRecord<T> setVariant(DataElement discriminant, T value);
   HLAextendableVariantRecord<T> setDiscriminant(DataElement discriminant);
   DataElement getDiscriminant();
   T getValue();
}
