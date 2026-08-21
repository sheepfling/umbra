package hla.rti1516_2025.time;

public interface HLAinteger64Interval extends LogicalTimeInterval<HLAinteger64Interval> {
   HLAinteger64Interval add(HLAinteger64Interval interval);
   HLAinteger64Interval subtract(HLAinteger64Interval interval);
   long getValue();
}
