package hla.rti1516_2025;

/** Test-fixture subset of the standard opaque parameter handle. */
public interface ParameterHandle {
   int encodedLength();
   void encode(byte[] buffer, int offset);
}
