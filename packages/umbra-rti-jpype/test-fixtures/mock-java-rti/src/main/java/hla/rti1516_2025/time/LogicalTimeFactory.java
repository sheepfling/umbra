package hla.rti1516_2025.time;

import java.io.Serializable;

/** Fixture declaration matching the IEEE 1516.1-2025 logical-time factory. */
public interface LogicalTimeFactory<T extends LogicalTime<T, U>, U extends LogicalTimeInterval<U>>
   extends Serializable {
   T decodeTime(byte[] encoded, int offset);
   U decodeInterval(byte[] encoded, int offset);
   T makeInitial();
   T makeFinal();
   U makeZero();
   U makeEpsilon();
   String getName();
}
