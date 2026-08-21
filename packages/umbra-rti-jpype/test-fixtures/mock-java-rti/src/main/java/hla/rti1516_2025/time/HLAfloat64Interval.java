package hla.rti1516_2025.time;

public interface HLAfloat64Interval extends LogicalTimeInterval<HLAfloat64Interval> {
   HLAfloat64Interval add(HLAfloat64Interval interval);
   HLAfloat64Interval subtract(HLAfloat64Interval interval);
   double getValue();
}
