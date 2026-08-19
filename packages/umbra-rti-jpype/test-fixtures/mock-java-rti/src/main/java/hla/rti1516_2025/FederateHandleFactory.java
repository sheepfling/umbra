package hla.rti1516_2025;

/** Fixture decoder matching the standard federate-handle factory boundary. */
public interface FederateHandleFactory {
   FederateHandle decode(byte[] buffer, int offset);
}
