package hla.rti1516_2025;

/** Fixture decoder matching the standard region-handle factory boundary. */
public interface RegionHandleFactory {
   RegionHandle decode(byte[] buffer, int offset);
}
