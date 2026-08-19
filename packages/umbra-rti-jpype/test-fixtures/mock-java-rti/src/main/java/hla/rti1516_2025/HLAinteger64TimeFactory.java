package hla.rti1516_2025;

/** Integer reference-time factory for the mock provider. */
public final class HLAinteger64TimeFactory implements LogicalTimeFactory {
   @Override
   public String getName() { return "HLAinteger64Time"; }

   public HLAinteger64Time makeLogicalTime(long value) { return new HLAinteger64Time(value); }

   public HLAinteger64Interval makeLogicalTimeInterval(long value) {
      return new HLAinteger64Interval(value);
   }

   @Override
   public LogicalTime makeInitial() { return new HLAinteger64Time(); }

   @Override
   public LogicalTime makeFinal() {
      HLAinteger64Time result = new HLAinteger64Time();
      result.setFinal();
      return result;
   }

   @Override
   public LogicalTimeInterval makeZero() { return new HLAinteger64Interval(); }

   @Override
   public LogicalTimeInterval makeEpsilon() {
      HLAinteger64Interval result = new HLAinteger64Interval();
      result.setEpsilon();
      return result;
   }

   @Override
   public LogicalTime decodeLogicalTime(byte[] encodedValue, int offset) {
      HLAinteger64Time result = new HLAinteger64Time();
      result.decode(encodedValue, offset);
      return result;
   }

   @Override
   public LogicalTimeInterval decodeLogicalTimeInterval(byte[] encodedValue, int offset) {
      HLAinteger64Interval result = new HLAinteger64Interval();
      result.decode(encodedValue, offset);
      return result;
   }
}
