package hla.rti1516_2025.encoding;

/** Fixture subset of the official encoder factory. */
public interface EncoderFactory {
   HLAinteger16BE createHLAinteger16BE();

   HLAinteger16BE createHLAinteger16BE(short value);

   HLAinteger16LE createHLAinteger16LE();

   HLAinteger16LE createHLAinteger16LE(short value);

   HLAinteger32LE createHLAinteger32LE();

   HLAinteger32LE createHLAinteger32LE(int value);

   HLAinteger64BE createHLAinteger64BE();

   HLAinteger64BE createHLAinteger64BE(long value);

   HLAinteger64LE createHLAinteger64LE();

   HLAinteger64LE createHLAinteger64LE(long value);

   HLAfloat64BE createHLAfloat64BE();

   HLAfloat64BE createHLAfloat64BE(double value);

   HLAfloat32BE createHLAfloat32BE();

   HLAfloat32BE createHLAfloat32BE(float value);

   HLAfloat64LE createHLAfloat64LE();

   HLAfloat64LE createHLAfloat64LE(double value);

   HLAfloat32LE createHLAfloat32LE();

   HLAfloat32LE createHLAfloat32LE(float value);

   HLAunsignedInteger16BE createHLAunsignedInteger16BE();

   HLAunsignedInteger16BE createHLAunsignedInteger16BE(short value);

   HLAunsignedInteger16LE createHLAunsignedInteger16LE();

   HLAunsignedInteger16LE createHLAunsignedInteger16LE(short value);

   HLAunsignedInteger32LE createHLAunsignedInteger32LE();

   HLAunsignedInteger32LE createHLAunsignedInteger32LE(int value);

   HLAinteger32BE createHLAinteger32BE();

   HLAinteger32BE createHLAinteger32BE(int value);

   HLAunsignedInteger32BE createHLAunsignedInteger32BE();

   HLAunsignedInteger32BE createHLAunsignedInteger32BE(int value);

   HLAunsignedInteger64BE createHLAunsignedInteger64BE();

   HLAunsignedInteger64BE createHLAunsignedInteger64BE(long value);

   HLAunsignedInteger64LE createHLAunsignedInteger64LE();

   HLAunsignedInteger64LE createHLAunsignedInteger64LE(long value);

   HLAbyte createHLAbyte();

   HLAbyte createHLAbyte(byte value);

   HLAoctet createHLAoctet();

   HLAoctet createHLAoctet(byte value);

   HLAASCIIchar createHLAASCIIchar();

   HLAASCIIchar createHLAASCIIchar(byte value);

   HLAASCIIstring createHLAASCIIstring();

   HLAASCIIstring createHLAASCIIstring(String value);

   HLAunicodeChar createHLAunicodeChar();

   HLAunicodeChar createHLAunicodeChar(short value);

   HLAoctetPairBE createHLAoctetPairBE();

   HLAoctetPairBE createHLAoctetPairBE(short value);

   HLAoctetPairLE createHLAoctetPairLE();

   HLAoctetPairLE createHLAoctetPairLE(short value);

   HLAopaqueData createHLAopaqueData();

   HLAopaqueData createHLAopaqueData(byte[] value);

   HLAvariableArray createHLAvariableArray(DataElementFactory factory, DataElement... elements);

   HLAfixedArray createHLAfixedArray(DataElementFactory factory, int size);

   HLAfixedRecord createHLAfixedRecord();

   HLAvariantRecord createHLAvariantRecord(DataElement discriminantPrototype);

   HLAboolean createHLAboolean();

   HLAboolean createHLAboolean(boolean value);

   HLAunicodeString createHLAunicodeString();

   HLAunicodeString createHLAunicodeString(String value);
}
