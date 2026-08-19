package hla.rti1516_2025;

/** Test-fixture subset of the standard opaque transportation-type handle. */
public interface TransportationTypeHandle {
   int encodedLength();
   void encode(byte[] buffer, int offset);
}
