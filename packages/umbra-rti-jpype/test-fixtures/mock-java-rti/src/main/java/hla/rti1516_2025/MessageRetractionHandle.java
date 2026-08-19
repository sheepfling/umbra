package hla.rti1516_2025;

/** Test-fixture opaque handle returned by timestamped services. */
public interface MessageRetractionHandle {
   int encodedLength();
   void encode(byte[] buffer, int offset);
}
