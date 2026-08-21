package hla.rti1516_2025.encoding;

public interface HLAvariantRecord<T extends DataElement> extends DataElement {
   HLAvariantRecord<T> setVariant(T discriminant, DataElement dataElement);

   HLAvariantRecord<T> setDiscriminant(T discriminant);

   T getDiscriminant();

   DataElement getValue();
}
