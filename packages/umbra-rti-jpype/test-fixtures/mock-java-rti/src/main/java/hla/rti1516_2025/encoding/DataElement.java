package hla.rti1516_2025.encoding;

/** Minimal fixture mirror of the standard data-element operations used by Python. */
public interface DataElement {
   int getOctetBoundary();

   int getEncodedLength();

   byte[] toByteArray();

   DataElement decode(byte[] bytes);
}
