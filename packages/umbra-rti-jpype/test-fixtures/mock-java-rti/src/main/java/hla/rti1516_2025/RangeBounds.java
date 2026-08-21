package hla.rti1516_2025;

/** Java-shaped mutable range-bounds value used by the DDM fixture. */
public final class RangeBounds {
   public final long lower;
   public final long upper;

   public RangeBounds(long lowerBound, long upperBound) {
      this.lower = lowerBound;
      this.upper = upperBound;
   }

   public long getLowerBound() { return lower; }
   public long getUpperBound() { return upper; }
}
