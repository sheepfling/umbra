package hla.rti1516_2025;

/** Fixture decoder matching the standard handle-factory boundary. */
public interface TransportationTypeHandleFactory {
   TransportationTypeHandle decode(byte[] buffer, int offset);

   TransportationTypeHandle getHLAdefaultReliable();

   TransportationTypeHandle getHLAdefaultBestEffort();
}
