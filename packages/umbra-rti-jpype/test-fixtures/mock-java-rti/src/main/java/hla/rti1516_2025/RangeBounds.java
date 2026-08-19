package hla.rti1516_2025;

/** Java-shaped mutable range-bounds value used by the DDM fixture. */
public final class RangeBounds {
   private long lowerBound;
   private long upperBound;

   public RangeBounds(long lowerBound, long upperBound) {
      this.lowerBound = lowerBound;
      this.upperBound = upperBound;
   }

   public long getLowerBound() { return lowerBound; }
   public long getUpperBound() { return upperBound; }
   public void setLowerBound(long lowerBound) { this.lowerBound = lowerBound; }
   public void setUpperBound(long upperBound) { this.upperBound = upperBound; }
}
