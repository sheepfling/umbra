package org.hla.rti.tck;

import hla.rti1516_2025.encoding.ByteWrapper;
import hla.rti1516_2025.encoding.DataElement;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAASCIIstring;
import hla.rti1516_2025.encoding.HLAbyte;
import hla.rti1516_2025.encoding.HLAfixedArray;
import hla.rti1516_2025.encoding.HLAfixedRecord;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.encoding.HLAoctet;
import hla.rti1516_2025.encoding.HLAvariantRecord;
import hla.rti1516_2025.encoding.HLAvariableArray;
import hla.rti1516_2025.encoding.HLAextendableVariantRecord;
import java.util.Arrays;
import java.util.function.Supplier;

/** Checks composite DataElement encoding and decoding within a shared ByteWrapper cursor. */
final class CompositeDataElementByteWrapperCursorTck {
   private static final int PREFIX = 0x7e;
   private static final byte FOLLOWING_OCTET = (byte) 0xa5;

   private CompositeDataElementByteWrapperCursorTck() {
   }

   static void run(EncoderFactory encoder) throws Exception {
      HLAfixedArray<HLAbyte> fixedArray = encoder.createHLAfixedArray(
         encoder.createHLAbyte((byte) 0x11), encoder.createHLAbyte((byte) 0x22));
      verifyCursor("HLAfixedArray", fixedArray,
         () -> encoder.createHLAfixedArray(index -> encoder.createHLAbyte(), 2), encoder);

      HLAvariableArray<HLAbyte> variableArray = encoder.createHLAvariableArray(
         index -> encoder.createHLAbyte());
      variableArray.addElement(encoder.createHLAbyte((byte) 0x33));
      variableArray.addElement(encoder.createHLAbyte((byte) 0x44));
      verifyCursor("HLAvariableArray", variableArray,
         () -> encoder.createHLAvariableArray(index -> encoder.createHLAbyte()), encoder);

      HLAfixedRecord record = encoder.createHLAfixedRecord();
      record.add(encoder.createHLAinteger32BE(0x10203040));
      record.add(encoder.createHLAoctet((byte) 0x5a));
      verifyCursor("HLAfixedRecord", record, () -> {
         HLAfixedRecord decoded = encoder.createHLAfixedRecord();
         decoded.add(encoder.createHLAinteger32BE());
         decoded.add(encoder.createHLAoctet());
         return decoded;
      }, encoder);

      HLAinteger32BE ready = encoder.createHLAinteger32BE(1);
      HLAinteger32BE busy = encoder.createHLAinteger32BE(2);
      HLAvariantRecord<HLAinteger32BE> variant = encoder.createHLAvariantRecord(
         encoder.createHLAinteger32BE());
      variant.setVariant(ready, encoder.createHLAinteger32BE());
      variant.setVariant(busy, encoder.createHLAASCIIstring());
      variant.setDiscriminant(ready);
      variant.setVariant(ready, encoder.createHLAinteger32BE(0x50607080));
      verifyCursor("HLAvariantRecord", variant, () -> {
         HLAvariantRecord<HLAinteger32BE> decoded = encoder.createHLAvariantRecord(
            encoder.createHLAinteger32BE());
         decoded.setVariant(ready, encoder.createHLAinteger32BE());
         decoded.setVariant(busy, encoder.createHLAASCIIstring());
         return decoded;
      }, encoder);

      HLAextendableVariantRecord<HLAinteger32BE> extendable =
         encoder.createHLAextendableVariantRecord(encoder.createHLAinteger32BE());
      extendable.setVariant(ready, encoder.createHLAinteger32BE());
      extendable.setVariant(busy, encoder.createHLAASCIIstring());
      extendable.setDiscriminant(ready);
      extendable.setVariant(ready, encoder.createHLAinteger32BE(0x60708010));
      verifyCursor("HLAextendableVariantRecord", extendable, () -> {
         HLAextendableVariantRecord<HLAinteger32BE> decoded =
            encoder.createHLAextendableVariantRecord(encoder.createHLAinteger32BE());
         decoded.setVariant(ready, encoder.createHLAinteger32BE());
         decoded.setVariant(busy, encoder.createHLAASCIIstring());
         return decoded;
      }, encoder);
   }

   private static <E extends DataElement> void verifyCursor(
         String type, E source, Supplier<E> createEmpty, EncoderFactory encoder) throws Exception {
      byte[] encoded = source.toByteArray();
      byte[] sharedBuffer = new byte[encoded.length + 2];
      ByteWrapper output = new ByteWrapper(sharedBuffer);
      output.put(PREFIX);
      int elementStart = output.getPos();
      source.encode(output);
      int elementEnd = output.getPos();
      check(elementEnd == elementStart + encoded.length,
         type + " encode(ByteWrapper) wrote an unexpected number of octets");
      check(Arrays.equals(encoded, Arrays.copyOfRange(sharedBuffer, elementStart, elementEnd)),
         type + " encode(ByteWrapper) changed its byte-array encoding");

      HLAoctet following = encoder.createHLAoctet(FOLLOWING_OCTET);
      following.encode(output);
      check(output.getPos() == sharedBuffer.length,
         type + " encode(ByteWrapper) did not leave room for the following element");

      ByteWrapper input = new ByteWrapper(sharedBuffer);
      input.advance(elementStart);
      E decoded = createEmpty.get();
      decoded.decode(input);
      check(input.getPos() == elementEnd,
         type + " decode(ByteWrapper) consumed the following element");
      check(Arrays.equals(encoded, decoded.toByteArray()),
         type + " decode(ByteWrapper) changed the composite value");

      HLAoctet decodedFollowing = encoder.createHLAoctet();
      decodedFollowing.decode(input);
      check(input.getPos() == sharedBuffer.length
            && decodedFollowing.getValue() == FOLLOWING_OCTET,
         type + " left the following HLAoctet unreadable at the decode cursor");
   }

   private static void check(boolean condition, String message) {
      if (!condition) {
         throw new AssertionError(message);
      }
   }
}
