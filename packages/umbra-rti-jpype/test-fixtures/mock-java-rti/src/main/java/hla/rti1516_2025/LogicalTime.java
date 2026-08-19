package hla.rti1516_2025;

/** Minimal logical-time value used by the timestamped fixture methods. */
public interface LogicalTime {
   int encodedLength();
   void encode(byte[] buffer, int offset);
   boolean isInitial();
   boolean isFinal();
   LogicalTime add(LogicalTimeInterval addend);
   LogicalTime subtract(LogicalTimeInterval subtrahend);
   String implementationName();
   String toString();
}
