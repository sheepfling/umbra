package hla.rti1516_2025.encoding;

public interface HLAvariableArray<T extends DataElement> extends DataElement, Iterable<T> {
   HLAvariableArray<T> addElement(T dataElement);

   int size();

   T get(int index);

   HLAvariableArray<T> resize(int size);
}
