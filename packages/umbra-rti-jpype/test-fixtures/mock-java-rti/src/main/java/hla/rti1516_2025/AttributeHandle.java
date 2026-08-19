package hla.rti1516_2025;

/** Test-fixture subset of the standard opaque attribute handle. */
public interface AttributeHandle {
   int encodedLength();
   void encode(byte[] buffer, int offset);
}
