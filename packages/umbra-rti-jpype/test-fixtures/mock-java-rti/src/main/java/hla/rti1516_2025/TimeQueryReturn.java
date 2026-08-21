package hla.rti1516_2025;

import hla.rti1516_2025.time.LogicalTime;

/** Java 1516 time-query result: a validity bit plus an optional time value. */
public final class TimeQueryReturn {
   public final boolean timeIsValid;
   public final LogicalTime<?, ?> time;

   public TimeQueryReturn(boolean timeIsValid, LogicalTime<?, ?> time) {
      this.timeIsValid = timeIsValid;
      this.time = time;
   }
}
