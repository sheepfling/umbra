package hla.rti1516_2025.encoding;

public interface HLAfixedRecord<T extends DataElement> extends DataElement, Iterable<T> {
   void appendElement(T dataElement);

   @SuppressWarnings("unchecked")
   default void add(DataElement dataElement) { appendElement((T) dataElement); }

   int size();

   T get(int index);
}
