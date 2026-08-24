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
      // HLAvariantRecord has a discriminant-dependent encoded width.  Its
      // native byte-array decoder is intentionally strict, so let the record
      // inspect its discriminant, decode exactly one mapped alternative, and
      // advance the caller's cursor without feeding a following field into
      // the C++ exact-payload validator.
      if (element instanceof NativeHLAvariantRecord) {
         @SuppressWarnings("unchecked")
         T variant = (T) ((NativeHLAvariantRecord) element).decode(source);
         return variant;
      }
      byte[] buffer = source.array();
      int offset = source.getPos();
      int available = source.remaining();
      if (available <= 0) {
         throw new DecoderException("ByteWrapper contains no data element encoding");
      }

      // The standard cursor may contain more than one element.  C++'s
      // byte-array decoders deliberately reject trailing bytes, so probe
      // prefixes until one complete native element is accepted.  This is
      // required for fixed records/arrays as well as variable-length
      // strings/arrays; the successful prefix is the exact cursor advance.
      Exception lastError = null;
      for (int length = 1; length <= available; ++length) {
         try {
            element.decode(Arrays.copyOfRange(buffer, offset, offset + length));
            source.advance(length);
            return element;
         } catch (Exception error) {
            lastError = error;
         }
      }
      if (lastError instanceof DecoderException) {
         throw (DecoderException) lastError;
      }
      if (lastError instanceof RuntimeException) {
         throw (RuntimeException) lastError;
      }
      if (lastError != null) {
         DecoderException failure = new DecoderException("Native element decoding failed");
         failure.initCause(lastError);
         throw failure;
      }
      throw new DecoderException("ByteWrapper contains no valid data element encoding");
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
