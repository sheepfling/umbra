package hla.rti1516_2025.encoding;

public interface DataElementFactory<T extends DataElement> {
   T createElement(int index);
}
