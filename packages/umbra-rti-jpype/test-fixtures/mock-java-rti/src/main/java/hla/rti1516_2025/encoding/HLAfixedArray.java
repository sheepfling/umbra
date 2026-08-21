package hla.rti1516_2025.encoding;

public interface HLAfixedArray<T extends DataElement> extends DataElement, Iterable<T> {
   int size();
   T get(int index);
}
