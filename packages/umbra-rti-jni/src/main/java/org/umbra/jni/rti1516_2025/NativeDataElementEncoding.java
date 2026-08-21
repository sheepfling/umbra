package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.encoding.ByteWrapper;
import hla.rti1516_2025.encoding.DataElement;
import hla.rti1516_2025.encoding.DecoderException;
import java.util.Arrays;

/**
 * Adapts the bridge's native byte-array encoding boundary to the standard
 * {@link ByteWrapper} cursor contract.
 *
 * <p>C++ owns every encoded representation.  This helper only copies the
 * already-encoded octets to or from a Java cursor; it never encodes a value
 * in Java.  Decoding starts at the current wrapper position and advances it
 * by the native element's decoded length, allowing elements to participate in
 * a surrounding standard composite.</p>
 */
final class NativeDataElementEncoding {
   private NativeDataElementEncoding() { }

   static void encode(DataElement element, ByteWrapper destination) {
      if (destination == null) throw new IllegalArgumentException("ByteWrapper must not be null");
      destination.put(element.toByteArray());
   }

   static ByteWrapper encode(DataElement element) {
      ByteWrapper result = new ByteWrapper(element.toByteArray());
      result.reset();
      return result;
   }

   static <T extends DataElement> T decode(T element, ByteWrapper source)
      throws DecoderException {
      if (source == null) throw new DecoderException("ByteWrapper must not be null");
      byte[] buffer = source.array();
      int offset = source.getPos();
      byte[] remaining = Arrays.copyOfRange(buffer, offset, offset + source.remaining());
      element.decode(remaining);
      int consumed = element.getEncodedLength();
      if (consumed > source.remaining()) {
         throw new DecoderException("Native decoder consumed beyond ByteWrapper limit");
      }
      source.advance(consumed);
      return element;
   }

   static <T extends DataElement> T decode(T element, byte[] source) {
      try {
         element.decode(source);
         return element;
      } catch (DecoderException error) {
         throw new IllegalStateException("Native element decoding failed", error);
      }
   }
}
