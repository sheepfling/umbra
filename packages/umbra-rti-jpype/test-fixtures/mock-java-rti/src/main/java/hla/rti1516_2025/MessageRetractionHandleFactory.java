package hla.rti1516_2025;

/** Test-fixture subset of the standard message-retraction decoder factory. */
public interface MessageRetractionHandleFactory {
   MessageRetractionHandle decode(byte[] buffer, int offset);
}
