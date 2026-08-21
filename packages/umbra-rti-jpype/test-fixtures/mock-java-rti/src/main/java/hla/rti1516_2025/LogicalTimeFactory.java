package hla.rti1516_2025;

/** Minimal logical-time factory contract used by the JVM fixture. */
public interface LogicalTimeFactory
   extends hla.rti1516_2025.time.LogicalTimeFactory<LogicalTime, LogicalTimeInterval> {
   String getName();
   LogicalTime makeInitial();
   LogicalTime makeFinal();
   LogicalTimeInterval makeZero();
   LogicalTimeInterval makeEpsilon();
   LogicalTime decodeLogicalTime(byte[] encodedValue, int offset);
   LogicalTimeInterval decodeLogicalTimeInterval(byte[] encodedValue, int offset);
   @Override default LogicalTime decodeTime(byte[] encodedValue, int offset) {
      return decodeLogicalTime(encodedValue, offset);
   }
   @Override default LogicalTimeInterval decodeInterval(byte[] encodedValue, int offset) {
      return decodeLogicalTimeInterval(encodedValue, offset);
   }
}
