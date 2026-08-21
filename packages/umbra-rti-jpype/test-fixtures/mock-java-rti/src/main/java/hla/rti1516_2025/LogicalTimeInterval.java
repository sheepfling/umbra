package hla.rti1516_2025;

/** Minimal provider-created logical-time interval contract used by the JVM fixture. */
public interface LogicalTimeInterval extends hla.rti1516_2025.time.LogicalTimeInterval<LogicalTimeInterval> {
   int encodedLength();
   void encode(byte[] buffer, int offset);
   boolean isZero();
   boolean isEpsilon();
   LogicalTimeInterval add(LogicalTimeInterval addend);
   LogicalTimeInterval subtract(LogicalTimeInterval subtrahend);
   @Override default int compareTo(LogicalTimeInterval interval) {
      return toString().compareTo(interval.toString());
   }
   void setToDifference(LogicalTime minuend, LogicalTime subtrahend);
   String implementationName();
   String toString();
}
