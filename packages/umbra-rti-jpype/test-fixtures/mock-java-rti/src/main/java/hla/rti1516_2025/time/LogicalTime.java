package hla.rti1516_2025.time;

import hla.rti1516_2025.exceptions.RTIexception;
import java.io.Serializable;

/** Fixture declaration matching the IEEE 1516.1-2025 logical-time namespace. */
public interface LogicalTime<T extends LogicalTime<T, U>, U extends LogicalTimeInterval<U>>
   extends Comparable<T>, Serializable {
   boolean isInitial();
   boolean isFinal();
   T add(U interval) throws RTIexception;
   T subtract(U interval) throws RTIexception;
   U distance(T time) throws RTIexception;
   int encodedLength();
   void encode(byte[] buffer, int offset);
}
