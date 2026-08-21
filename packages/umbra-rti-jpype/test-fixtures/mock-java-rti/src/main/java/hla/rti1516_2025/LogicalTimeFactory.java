package hla.rti1516_2025;

import hla.rti1516_2025.exceptions.RTIexception;

/** Minimal logical-time factory contract used by the JVM fixture. */
public interface LogicalTimeFactory
   extends hla.rti1516_2025.time.LogicalTimeFactory<LogicalTime, LogicalTimeInterval> {
   String getName();
   LogicalTime makeInitial();
   LogicalTime makeFinal();
   LogicalTimeInterval makeZero();
   LogicalTimeInterval makeEpsilon();
   LogicalTime decodeLogicalTime(byte[] encodedValue, int offset) throws RTIexception;
   LogicalTimeInterval decodeLogicalTimeInterval(byte[] encodedValue, int offset) throws RTIexception;
   @Override default LogicalTime decodeTime(byte[] encodedValue, int offset) throws RTIexception {
      return decodeLogicalTime(encodedValue, offset);
   }
   @Override default LogicalTimeInterval decodeInterval(byte[] encodedValue, int offset) throws RTIexception {
      return decodeLogicalTimeInterval(encodedValue, offset);
   }
}
