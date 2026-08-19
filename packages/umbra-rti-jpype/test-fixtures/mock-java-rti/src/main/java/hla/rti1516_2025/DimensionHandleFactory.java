package hla.rti1516_2025;

/** Fixture decoder matching the standard handle-factory boundary. */
public interface DimensionHandleFactory {
   DimensionHandle decode(byte[] buffer, int offset);
}
