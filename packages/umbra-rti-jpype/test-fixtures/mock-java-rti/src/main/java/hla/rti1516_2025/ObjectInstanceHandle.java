package hla.rti1516_2025;

/** Test-fixture subset of the standard opaque object-instance handle. */
public interface ObjectInstanceHandle {
   int encodedLength();
   void encode(byte[] buffer, int offset);
}
