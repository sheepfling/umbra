package hla.rti1516_2025;

/** Test-fixture subset of the standard opaque region handle. */
public interface RegionHandle {
   int encodedLength();
   void encode(byte[] buffer, int offset);
}
