package hla.rti1516_2025.time;

public interface HLAfloat64Time extends LogicalTime<HLAfloat64Time, HLAfloat64Interval> {
   HLAfloat64Time add(HLAfloat64Interval interval);
   HLAfloat64Time subtract(HLAfloat64Interval interval);
   HLAfloat64Interval distance(HLAfloat64Time time);
   double getValue();
}
