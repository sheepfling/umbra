package org.hla.rti.tck;

import hla.rti1516_2025.encoding.ByteWrapper;
import hla.rti1516_2025.encoding.DataElement;
import hla.rti1516_2025.encoding.DecoderException;
import hla.rti1516_2025.encoding.EncoderException;
import java.util.Arrays;

/** Java-standard subset of the provider-independent DataElement base contract. */
final class DataElementBaseTck {
   private static final int LENGTH = 16;

   private DataElementBaseTck() {
   }

   static void run() throws Exception {
      Boundary16Element element = new Boundary16Element();
      byte[] zeros = new byte[LENGTH];
      ByteWrapper encoded = element.encode();
      require(Arrays.equals(encoded.array(), zeros) && encoded.getPos() == LENGTH,
         "DataElement.encode did not return its complete encoding");
      require(Arrays.equals(element.toByteArray(), zeros),
         "DataElement.toByteArray changed the encoding");
      require(element.getEncodedLength() == LENGTH && element.getOctetBoundary() == LENGTH,
         "DataElement length or octet-boundary metadata changed");

      ByteWrapper appended = new ByteWrapper(LENGTH + 1);
      appended.put(0x7e);
      element.encode(appended);
      byte[] expectedAppended = new byte[LENGTH + 1];
      expectedAppended[0] = 0x7e;
      require(Arrays.equals(appended.array(), expectedAppended)
            && appended.getPos() == LENGTH + 1,
         "DataElement.encode(ByteWrapper) did not append its complete encoding");

      Boundary16Element decoded = new Boundary16Element();
      require(decoded.decode(element.toByteArray()) == decoded,
         "DataElement.decode(byte[]) did not return the target element");
      byte[] prefixed = new byte[LENGTH + 1];
      prefixed[0] = 0x7e;
      ByteWrapper input = new ByteWrapper(prefixed);
      input.advance(1);
      require(decoded.decode(input) == decoded && input.getPos() == LENGTH + 1,
         "DataElement.decode(ByteWrapper) did not consume the value at the current position");
      expectDecoderFailure(() -> decoded.decode(new byte[LENGTH - 1]),
         "DataElement.decode(byte[]) accepted a truncated encoding");
      expectDecoderFailure(() -> decoded.decode(new ByteWrapper(new byte[LENGTH - 1])),
         "DataElement.decode(ByteWrapper) accepted a truncated encoding");
   }

   private static void expectDecoderFailure(DecoderOperation operation, String message)
         throws Exception {
      try {
         operation.run();
      } catch (DecoderException expected) {
         return;
      }
      throw new AssertionError(message);
   }

   private static void require(boolean condition, String message) {
      if (!condition) {
         throw new AssertionError(message);
      }
   }

   @FunctionalInterface
   private interface DecoderOperation {
      void run() throws Exception;
   }

   private static final class Boundary16Element implements DataElement {
      @Override
      public int getOctetBoundary() {
         return LENGTH;
      }

      @Override
      public void encode(ByteWrapper wrapper) throws EncoderException {
         wrapper.put(new byte[LENGTH]);
      }

      @Override
      public ByteWrapper encode() throws EncoderException {
         ByteWrapper wrapper = new ByteWrapper(LENGTH);
         encode(wrapper);
         return wrapper;
      }

      @Override
      public int getEncodedLength() {
         return LENGTH;
      }

      @Override
      public byte[] toByteArray() throws EncoderException {
         return encode().array();
      }

      @Override
      public DataElement decode(ByteWrapper wrapper) throws DecoderException {
         try {
            wrapper.verify(LENGTH);
            wrapper.advance(LENGTH);
            return this;
         } catch (RuntimeException error) {
            throw new DecoderException("Boundary16Element has a truncated encoding", error);
         }
      }

      @Override
      public DataElement decode(byte[] bytes) throws DecoderException {
         if (bytes.length != LENGTH) {
            throw new DecoderException("Boundary16Element has an invalid encoding length");
         }
         return decode(new ByteWrapper(bytes));
      }
   }
}
