package hla.rti1516_2025.time;

public interface HLAinteger64Time extends LogicalTime<HLAinteger64Time, HLAinteger64Interval> {
   HLAinteger64Time add(HLAinteger64Interval interval);
   HLAinteger64Time subtract(HLAinteger64Interval interval);
   HLAinteger64Interval distance(HLAinteger64Time time);
   long getValue();
}
