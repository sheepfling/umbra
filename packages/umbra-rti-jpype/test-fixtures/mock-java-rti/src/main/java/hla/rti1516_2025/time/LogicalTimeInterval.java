package hla.rti1516_2025.time;

import hla.rti1516_2025.exceptions.RTIexception;
import java.io.Serializable;

/** Fixture declaration matching the IEEE 1516.1-2025 logical-time namespace. */
public interface LogicalTimeInterval<U extends LogicalTimeInterval<U>> extends Comparable<U>, Serializable {
   boolean isZero();
   boolean isEpsilon();
   U add(U interval) throws RTIexception;
   U subtract(U interval) throws RTIexception;
   int encodedLength();
   void encode(byte[] buffer, int offset);
}
