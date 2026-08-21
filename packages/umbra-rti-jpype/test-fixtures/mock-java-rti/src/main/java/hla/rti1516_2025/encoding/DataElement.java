package hla.rti1516_2025.encoding;

/** Minimal fixture mirror of the standard data-element operations used by Python. */
public interface DataElement {
   int getOctetBoundary();

   default void encode(ByteWrapper byteWrapper) {
      byteWrapper.put(toByteArray());
   }

   default ByteWrapper encode() {
      return new ByteWrapper(toByteArray());
   }

   int getEncodedLength();

   byte[] toByteArray();

   default DataElement decode(ByteWrapper byteWrapper) throws DecoderException {
      byte[] remaining = new byte[byteWrapper.remaining()];
      byteWrapper.get(remaining);
      return decode(remaining);
   }

   DataElement decode(byte[] bytes) throws DecoderException;
}
