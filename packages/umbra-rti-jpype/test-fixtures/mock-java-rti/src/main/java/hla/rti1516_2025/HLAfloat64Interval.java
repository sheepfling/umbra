package hla.rti1516_2025;

import java.nio.ByteBuffer;

/** IEEE-754 double reference interval for the mock provider. */
public final class HLAfloat64Interval implements LogicalTimeInterval {
   /** The smallest positive IEEE-754 double, matching the native reference time. */
   private static final double EPSILON = Double.MIN_VALUE;

   private double value;
   private boolean zero;
   private boolean epsilon;

   public HLAfloat64Interval() {
      setZero();
   }

   public HLAfloat64Interval(double value) {
      setValue(value);
   }

   public void setZero() {
      value = 0.0;
      zero = true;
      epsilon = false;
   }

   public void setEpsilon() {
      value = EPSILON;
      zero = false;
      epsilon = true;
   }

   @Override
   public int encodedLength() { return Double.BYTES; }

   @Override
   public void encode(byte[] buffer, int offset) {
      ByteBuffer.wrap(buffer, offset, Double.BYTES).putDouble(value);
   }

   public void decode(byte[] encodedValue, int offset) {
      setValue(ByteBuffer.wrap(encodedValue, offset, Double.BYTES).getDouble());
   }

   private void setValue(double value) {
      this.value = value == 0.0 ? 0.0 : value;
      zero = this.value == 0.0;
      epsilon = this.value == EPSILON;
   }

   @Override
   public boolean isZero() { return zero; }

   @Override
   public boolean isEpsilon() { return epsilon; }

   @Override
   public LogicalTimeInterval add(LogicalTimeInterval addend) {
      if (!(addend instanceof HLAfloat64Interval)) {
         throw new IllegalArgumentException("logical-time interval implementation mismatch");
      }
      double interval = ((HLAfloat64Interval) addend).getInterval();
      if (value == Double.MAX_VALUE && interval == 0.0) {
         return new HLAfloat64Interval(value);
      }
      if (value >= Double.MAX_VALUE || interval > Double.MAX_VALUE - value) {
         throw new IllegalArgumentException("logical-time interval addition exceeds its range");
      }
      double result = value + interval;
      if (interval == Double.MIN_VALUE && result == value) {
         result = Math.nextAfter(value, Double.MAX_VALUE);
      }
      if (!Double.isFinite(result) || result < 0.0) {
         throw new IllegalArgumentException("logical-time interval addition produced an invalid value");
      }
      return new HLAfloat64Interval(result);
   }

   @Override
   public LogicalTimeInterval subtract(LogicalTimeInterval subtrahend) {
      if (!(subtrahend instanceof HLAfloat64Interval)) {
         throw new IllegalArgumentException("logical-time interval implementation mismatch");
      }
      double interval = ((HLAfloat64Interval) subtrahend).getInterval();
      if (interval > value) {
         throw new IllegalArgumentException("logical-time interval subtraction precedes zero");
      }
      double result = value - interval;
      if (interval == Double.MIN_VALUE && result == value && value > 0.0) {
         result = Math.nextAfter(value, 0.0);
      }
      if (!Double.isFinite(result) || result < 0.0) {
         throw new IllegalArgumentException("logical-time interval subtraction produced an invalid value");
      }
      return new HLAfloat64Interval(result);
   }

   @Override
   public void setToDifference(LogicalTime minuend, LogicalTime subtrahend) {
      if (!(minuend instanceof HLAfloat64Time)
         || !(subtrahend instanceof HLAfloat64Time)) {
         throw new IllegalArgumentException("logical-time implementation mismatch");
      }
      double left = ((HLAfloat64Time) minuend).getTime();
      double right = ((HLAfloat64Time) subtrahend).getTime();
      if (right > left) {
         throw new IllegalArgumentException("logical-time difference would be negative");
      }
      value = left - right;
      zero = value == 0.0;
      epsilon = value == Double.MIN_VALUE;
   }

   @Override
   public String implementationName() { return "HLAfloat64Time"; }

   public double getInterval() { return value; }

   @Override
   public String toString() { return zero ? "zero" : epsilon ? "epsilon" : Double.toString(value); }
}
