package hla.rti1516_2025;

/** Test-fixture subset of the standard opaque object-class handle. */
public interface ObjectClassHandle {
   int encodedLength();
   void encode(byte[] buffer, int offset);
}
