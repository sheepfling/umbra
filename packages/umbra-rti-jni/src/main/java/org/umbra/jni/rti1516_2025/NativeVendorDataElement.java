package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.encoding.ByteWrapper;
import hla.rti1516_2025.encoding.DataElement;
import hla.rti1516_2025.encoding.DecoderException;
import hla.rti1516_2025.encoding.EncoderException;

/**
 * Small provider-owned data-element carrier used only by the JNI boundary
 * probe.
 *
 * <p>The four-octet payload is intentionally not an IEEE encoding rule.  It
 * gives the Python tests a concrete Java object with an unknown implementation
 * name so they can verify raw-carrier preservation and provider-owned
 * malformed decode behavior without adding a second standard data element.</p>
 */
final class NativeVendorDataElement implements DataElement {
   private static final int PAYLOAD_LENGTH = 4;
   private byte[] payload;

   NativeVendorDataElement(byte[] payload) {
      if (payload == null || payload.length != PAYLOAD_LENGTH) {
         throw new IllegalArgumentException("Vendor data-element payload must contain four octets");
      }
      this.payload = payload.clone();
   }

   @Override public int getOctetBoundary() {
      return 1;
   }

   @Override public void encode(ByteWrapper wrapper) throws EncoderException {
      if (wrapper == null) {
         throw new EncoderException("Vendor data-element ByteWrapper must not be null");
      }
      try {
         wrapper.put(payload);
      } catch (RuntimeException error) {
         throw new EncoderException("Vendor data-element encode failed", error);
      }
   }

   @Override public ByteWrapper encode() throws EncoderException {
      ByteWrapper result = new ByteWrapper(payload.clone());
      result.reset();
      return result;
   }

   @Override public int getEncodedLength() {
      return payload.length;
   }

   @Override public byte[] toByteArray() throws EncoderException {
      return payload.clone();
   }

   @Override public DataElement decode(ByteWrapper wrapper) throws DecoderException {
      if (wrapper == null || wrapper.remaining() < PAYLOAD_LENGTH) {
         throw new DecoderException("Vendor data-element payload is truncated");
      }
      byte[] decoded = new byte[PAYLOAD_LENGTH];
      wrapper.get(decoded);
      payload = decoded;
      return this;
   }

   @Override public DataElement decode(byte[] value) throws DecoderException {
      if (value == null || value.length != PAYLOAD_LENGTH) {
         throw new DecoderException("Vendor data-element payload length mismatch");
      }
      payload = value.clone();
      return this;
   }
}
