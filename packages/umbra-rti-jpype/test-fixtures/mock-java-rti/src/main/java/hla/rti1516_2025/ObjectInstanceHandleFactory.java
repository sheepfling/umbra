package hla.rti1516_2025;

/** Fixture decoder matching the standard object-instance-handle boundary. */
public interface ObjectInstanceHandleFactory {
   ObjectInstanceHandle decode(byte[] buffer, int offset);
}
