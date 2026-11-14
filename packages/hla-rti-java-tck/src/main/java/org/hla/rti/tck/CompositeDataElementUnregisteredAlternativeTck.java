package org.hla.rti.tck;

import hla.rti1516_2025.encoding.EncoderException;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAbyte;
import hla.rti1516_2025.encoding.HLAextendableVariantRecord;
import hla.rti1516_2025.encoding.HLAfloat64BE;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.encoding.HLAvariantRecord;
import java.util.Arrays;

/** Checks mapped and unmapped alternative access for both Java variant-record families. */
final class CompositeDataElementUnregisteredAlternativeTck {
   private CompositeDataElementUnregisteredAlternativeTck() {
   }

   static void run(EncoderFactory encoder) throws Exception {
      HLAbyte selected = encoder.createHLAbyte((byte) 1);
      HLAvariantRecord<HLAbyte> variant = encoder.createHLAvariantRecord(encoder.createHLAbyte());
      variant.setVariant(selected, encoder.createHLAinteger32BE());
      variant.setVariant(encoder.createHLAbyte((byte) 2), encoder.createHLAfloat64BE());
      variant.setVariant(selected, encoder.createHLAinteger32BE(0x10203040));
      byte[] selectedEncoding = variant.toByteArray();
      check(variant.getDiscriminant().getValue() == 1
            && Arrays.equals(selectedEncoding, new byte[] {
               1, 0, 0, 0, 0, 0, 0, 0, 0x10, 0x20, 0x30, 0x40}),
         "HLAvariantRecord did not encode its selected alternative: "
            + Arrays.toString(selectedEncoding));
      check(((HLAinteger32BE) variant.getValue()).getValue() == 0x10203040,
         "HLAvariantRecord did not expose its selected alternative");

      variant.setDiscriminant(encoder.createHLAbyte((byte) 0x7f));
      check(Arrays.equals(variant.toByteArray(), new byte[] {(byte) 0x7f}),
         "HLAvariantRecord encoded data for an unregistered alternative");
      expectEncoderFailure(() -> variant.getValue(),
         "HLAvariantRecord exposed an unregistered alternative");

      HLAfloat64BE mappedDiscriminant = encoder.createHLAfloat64BE(1.5);
      HLAextendableVariantRecord<HLAfloat64BE> extendable =
         encoder.createHLAextendableVariantRecord(encoder.createHLAfloat64BE());
      extendable.setVariant(mappedDiscriminant, encoder.createHLAinteger32BE());
      extendable.setDiscriminant(encoder.createHLAfloat64BE(2.5));
      checkBytes(extendable.toByteArray(),
         0x40, 0x04, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
      check(extendable.getOctetBoundary() == 8 && extendable.getEncodedLength() == 16,
         "HLAextendableVariantRecord unmapped-discriminant layout metadata was incorrect");
      expectEncoderFailure(() -> extendable.getValue(),
         "HLAextendableVariantRecord exposed an unregistered alternative");

      HLAextendableVariantRecord<HLAbyte> decodedUnknown =
         encoder.createHLAextendableVariantRecord(encoder.createHLAbyte());
      decodedUnknown.setVariant(encoder.createHLAbyte((byte) 1), encoder.createHLAfloat32LE());
      decodedUnknown.decode(new byte[] {
         0x7f, 0, 0, 0, 0, 0, 0, 3, (byte) 0xaa, (byte) 0xbb, (byte) 0xcc});
      check(decodedUnknown.getDiscriminant().getValue() == (byte) 0x7f,
         "HLAextendableVariantRecord did not preserve an unknown decoded discriminant");
      expectEncoderFailure(() -> decodedUnknown.getValue(),
         "HLAextendableVariantRecord exposed a decoded unregistered alternative");
   }

   private static void checkBytes(byte[] actual, int... expected) {
      check(actual.length == expected.length,
         "encoded length " + actual.length + " != " + expected.length);
      for (int i = 0; i < expected.length; i++) {
         check((actual[i] & 0xff) == expected[i],
            "encoded octet " + i + " was " + Integer.toHexString(actual[i] & 0xff)
               + " instead of " + Integer.toHexString(expected[i]));
      }
   }

   private static void expectEncoderFailure(CheckedOperation operation, String message)
         throws Exception {
      try {
         operation.run();
      } catch (EncoderException expected) {
         return;
      }
      throw new AssertionError(message);
   }

   private static void check(boolean condition, String message) {
      if (!condition) {
         throw new AssertionError(message);
      }
   }

   @FunctionalInterface
   private interface CheckedOperation {
      void run() throws Exception;
   }
}
