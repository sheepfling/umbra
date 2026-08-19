package hla.rti1516_2025;

/** Test-fixture subset of the standard opaque federate-handle interface. */
public interface FederateHandle {
   int encodedLength();
   void encode(byte[] buffer, int offset);
}
