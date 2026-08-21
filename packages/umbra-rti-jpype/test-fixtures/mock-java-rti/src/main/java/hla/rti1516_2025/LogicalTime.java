package hla.rti1516_2025;

import hla.rti1516_2025.exceptions.RTIexception;

/** Minimal logical-time value used by the timestamped fixture methods. */
public interface LogicalTime extends hla.rti1516_2025.time.LogicalTime<LogicalTime, LogicalTimeInterval> {
   int encodedLength();
   void encode(byte[] buffer, int offset);
   boolean isInitial();
   boolean isFinal();
   LogicalTime add(LogicalTimeInterval addend) throws RTIexception;
   LogicalTime subtract(LogicalTimeInterval subtrahend) throws RTIexception;
   @Override default LogicalTimeInterval distance(LogicalTime time) {
      throw new UnsupportedOperationException("fixture logical-time distance is implementation-specific");
   }
   @Override default int compareTo(LogicalTime time) { return toString().compareTo(time.toString()); }
   String implementationName();
   String toString();
}
