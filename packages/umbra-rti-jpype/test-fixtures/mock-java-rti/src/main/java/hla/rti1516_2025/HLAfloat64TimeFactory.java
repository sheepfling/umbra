package hla.rti1516_2025;

/** Floating reference-time factory for the mock provider. */
public final class HLAfloat64TimeFactory implements LogicalTimeFactory {
   @Override
   public String getName() { return "HLAfloat64Time"; }

   public HLAfloat64Time makeLogicalTime(double value) { return new HLAfloat64Time(value); }

   public HLAfloat64Interval makeLogicalTimeInterval(double value) {
      return new HLAfloat64Interval(value);
   }

   @Override
   public LogicalTime makeInitial() { return new HLAfloat64Time(); }

   @Override
   public LogicalTime makeFinal() {
      HLAfloat64Time result = new HLAfloat64Time();
      result.setFinal();
      return result;
   }

   @Override
   public LogicalTimeInterval makeZero() { return new HLAfloat64Interval(); }

   @Override
   public LogicalTimeInterval makeEpsilon() {
      HLAfloat64Interval result = new HLAfloat64Interval();
      result.setEpsilon();
      return result;
   }

   @Override
   public LogicalTime decodeLogicalTime(byte[] encodedValue, int offset) {
      HLAfloat64Time result = new HLAfloat64Time();
      result.decode(encodedValue, offset);
      return result;
   }

   @Override
   public LogicalTimeInterval decodeLogicalTimeInterval(byte[] encodedValue, int offset) {
      HLAfloat64Interval result = new HLAfloat64Interval();
      result.decode(encodedValue, offset);
      return result;
   }
}
