package hla.rti1516_2025;

import java.nio.ByteBuffer;

/** Signed 64-bit reference time for the mock provider. */
public final class HLAinteger64Time implements LogicalTime {
   private long value;
   private boolean initial;
   private boolean terminal;

   public HLAinteger64Time() {
      setInitial();
   }

   public HLAinteger64Time(long value) {
      this.value = value;
   }

   public void setInitial() {
      value = 0L;
      initial = true;
      terminal = false;
   }

   public void setFinal() {
      value = Long.MAX_VALUE;
      initial = false;
      terminal = true;
   }

   @Override
   public int encodedLength() {
      return Long.BYTES;
   }

   @Override
   public void encode(byte[] buffer, int offset) {
      ByteBuffer.wrap(buffer, offset, Long.BYTES).putLong(value);
   }

   public void decode(byte[] encodedValue, int offset) {
      value = ByteBuffer.wrap(encodedValue, offset, Long.BYTES).getLong();
      initial = value == 0L;
      terminal = value == Long.MAX_VALUE;
   }

   @Override
   public boolean isInitial() { return initial; }

   @Override
   public boolean isFinal() { return terminal; }

   @Override
   public LogicalTime add(LogicalTimeInterval addend) {
      if (!(addend instanceof HLAinteger64Interval)) {
         throw new IllegalArgumentException("logical-time interval implementation mismatch");
      }
      long result;
      try {
         result = Math.addExact(value, ((HLAinteger64Interval) addend).getInterval());
      } catch (ArithmeticException error) {
         throw new IllegalArgumentException("logical-time addition exceeds the final value", error);
      }
      if (result < 0L) {
         throw new IllegalArgumentException("logical-time addition produced a negative value");
      }
      return new HLAinteger64Time(result);
   }

   @Override
   public LogicalTime subtract(LogicalTimeInterval subtrahend) {
      if (!(subtrahend instanceof HLAinteger64Interval)) {
         throw new IllegalArgumentException("logical-time interval implementation mismatch");
      }
      long valueToSubtract = ((HLAinteger64Interval) subtrahend).getInterval();
      if (valueToSubtract > value) {
         throw new IllegalArgumentException("logical-time subtraction precedes the initial value");
      }
      return new HLAinteger64Time(value - valueToSubtract);
   }

   @Override
   public String implementationName() { return "HLAinteger64Time"; }

   public long getTime() { return value; }

   @Override
   public String toString() { return initial ? "initial" : terminal ? "final" : Long.toString(value); }
}
