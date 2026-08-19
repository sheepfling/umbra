package hla.rti1516_2025.encoding;

/** Fixture subset of the official encoder factory. */
public interface EncoderFactory {
   HLAinteger32BE createHLAinteger32BE();

   HLAinteger32BE createHLAinteger32BE(int value);

   HLAunsignedInteger32BE createHLAunsignedInteger32BE();

   HLAunsignedInteger32BE createHLAunsignedInteger32BE(int value);

   HLAboolean createHLAboolean();

   HLAboolean createHLAboolean(boolean value);

   HLAunicodeString createHLAunicodeString();

   HLAunicodeString createHLAunicodeString(String value);
}
