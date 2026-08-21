package hla.rti1516_2025.time;

public interface HLAinteger64TimeFactory extends LogicalTimeFactory<HLAinteger64Time, HLAinteger64Interval> {
   HLAinteger64Time makeTime(long value);
   HLAinteger64Interval makeInterval(long value);
}
