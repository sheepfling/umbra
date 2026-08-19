package hla.rti1516_2025;

import java.nio.ByteBuffer;

/** Signed 64-bit reference interval for the mock provider. */
public final class HLAinteger64Interval implements LogicalTimeInterval {
   private long value;
   private boolean zero;
   private boolean epsilon;

   public HLAinteger64Interval() {
      setZero();
   }

   public HLAinteger64Interval(long value) {
      this.value = value;
      zero = value == 0L;
      epsilon = value == 1L;
   }

   public void setZero() {
      value = 0L;
      zero = true;
      epsilon = false;
   }

   public void setEpsilon() {
      value = 1L;
      zero = false;
      epsilon = true;
   }

   @Override
   public int encodedLength() { return Long.BYTES; }

   @Override
   public void encode(byte[] buffer, int offset) {
      ByteBuffer.wrap(buffer, offset, Long.BYTES).putLong(value);
   }

   public void decode(byte[] encodedValue, int offset) {
      value = ByteBuffer.wrap(encodedValue, offset, Long.BYTES).getLong();
      zero = value == 0L;
      epsilon = value == 1L;
   }

   @Override
   public boolean isZero() { return zero; }

   @Override
   public boolean isEpsilon() { return epsilon; }

   @Override
   public LogicalTimeInterval add(LogicalTimeInterval addend) {
      if (!(addend instanceof HLAinteger64Interval)) {
         throw new IllegalArgumentException("logical-time interval implementation mismatch");
      }
      long result;
      try {
         result = Math.addExact(value, ((HLAinteger64Interval) addend).getInterval());
      } catch (ArithmeticException error) {
         throw new IllegalArgumentException("logical-time interval addition exceeds its range", error);
      }
      return new HLAinteger64Interval(result);
   }

   @Override
   public LogicalTimeInterval subtract(LogicalTimeInterval subtrahend) {
      if (!(subtrahend instanceof HLAinteger64Interval)) {
         throw new IllegalArgumentException("logical-time interval implementation mismatch");
      }
      long valueToSubtract = ((HLAinteger64Interval) subtrahend).getInterval();
      if (valueToSubtract > value) {
         throw new IllegalArgumentException("logical-time interval subtraction precedes zero");
      }
      return new HLAinteger64Interval(value - valueToSubtract);
   }

   @Override
   public void setToDifference(LogicalTime minuend, LogicalTime subtrahend) {
      if (!(minuend instanceof HLAinteger64Time)
         || !(subtrahend instanceof HLAinteger64Time)) {
         throw new IllegalArgumentException("logical-time implementation mismatch");
      }
      long left = ((HLAinteger64Time) minuend).getTime();
      long right = ((HLAinteger64Time) subtrahend).getTime();
      if (right > left) {
         throw new IllegalArgumentException("logical-time difference would be negative");
      }
      value = left - right;
      zero = value == 0L;
      epsilon = value == 1L;
   }

   @Override
   public String implementationName() { return "HLAinteger64Time"; }

   public long getInterval() { return value; }

   @Override
   public String toString() { return zero ? "zero" : epsilon ? "epsilon" : Long.toString(value); }
}
