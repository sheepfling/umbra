package hla.rti1516_2025;

/** Test-fixture subset of the standard opaque interaction-class handle. */
public interface InteractionClassHandle {
   int encodedLength();
   void encode(byte[] buffer, int offset);
}
