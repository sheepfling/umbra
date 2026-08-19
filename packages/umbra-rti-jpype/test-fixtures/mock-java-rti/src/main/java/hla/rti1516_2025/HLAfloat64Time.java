package hla.rti1516_2025;

import java.nio.ByteBuffer;

/** IEEE-754 double reference time for the mock provider. */
public final class HLAfloat64Time implements LogicalTime {
   private double value;
   private boolean initial;
   private boolean terminal;

   public HLAfloat64Time() {
      setInitial();
   }

   public HLAfloat64Time(double value) {
      setValue(value);
   }

   public void setInitial() {
      value = 0.0;
      initial = true;
      terminal = false;
   }

   public void setFinal() {
      value = Double.MAX_VALUE;
      initial = false;
      terminal = true;
   }

   @Override
   public int encodedLength() {
      return Double.BYTES;
   }

   @Override
   public void encode(byte[] buffer, int offset) {
      ByteBuffer.wrap(buffer, offset, Double.BYTES).putDouble(value);
   }

   public void decode(byte[] encodedValue, int offset) {
      setValue(ByteBuffer.wrap(encodedValue, offset, Double.BYTES).getDouble());
   }

   private void setValue(double value) {
      this.value = value == 0.0 ? 0.0 : value;
      initial = this.value == 0.0;
      terminal = this.value == Double.MAX_VALUE;
   }

   @Override
   public boolean isInitial() { return initial; }

   @Override
   public boolean isFinal() { return terminal; }

   @Override
   public LogicalTime add(LogicalTimeInterval addend) {
      if (!(addend instanceof HLAfloat64Interval)) {
         throw new IllegalArgumentException("logical-time interval implementation mismatch");
      }
      double interval = ((HLAfloat64Interval) addend).getInterval();
      if (value == Double.MAX_VALUE && interval == 0.0) {
         return new HLAfloat64Time(value);
      }
      if (value >= Double.MAX_VALUE || interval > Double.MAX_VALUE - value) {
         throw new IllegalArgumentException("logical-time addition exceeds the final value");
      }
      double result = value + interval;
      if (interval == Double.MIN_VALUE && result == value) {
         result = Math.nextAfter(value, Double.MAX_VALUE);
      }
      if (!Double.isFinite(result) || result < 0.0) {
         throw new IllegalArgumentException("logical-time addition produced an invalid value");
      }
      return new HLAfloat64Time(result);
   }

   @Override
   public LogicalTime subtract(LogicalTimeInterval subtrahend) {
      if (!(subtrahend instanceof HLAfloat64Interval)) {
         throw new IllegalArgumentException("logical-time interval implementation mismatch");
      }
      double interval = ((HLAfloat64Interval) subtrahend).getInterval();
      if (interval > value) {
         throw new IllegalArgumentException("logical-time subtraction precedes the initial value");
      }
      double result = value - interval;
      if (interval == Double.MIN_VALUE && result == value && value > 0.0) {
         result = Math.nextAfter(value, 0.0);
      }
      if (!Double.isFinite(result) || result < 0.0) {
         throw new IllegalArgumentException("logical-time subtraction produced an invalid value");
      }
      return new HLAfloat64Time(result);
   }

   @Override
   public String implementationName() { return "HLAfloat64Time"; }

   public double getTime() { return value; }

   @Override
   public String toString() { return initial ? "initial" : terminal ? "final" : Double.toString(value); }
}
