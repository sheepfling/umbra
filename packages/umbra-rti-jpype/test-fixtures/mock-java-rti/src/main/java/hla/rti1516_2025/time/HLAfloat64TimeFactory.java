package hla.rti1516_2025.time;

public interface HLAfloat64TimeFactory extends LogicalTimeFactory<HLAfloat64Time, HLAfloat64Interval> {
   HLAfloat64Time makeTime(double value);
   HLAfloat64Interval makeInterval(double value);
}
