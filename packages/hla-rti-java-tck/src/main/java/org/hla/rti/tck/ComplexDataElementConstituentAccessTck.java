package org.hla.rti.tck;

import hla.rti1516_2025.encoding.DataElement;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAextendableVariantRecord;
import hla.rti1516_2025.encoding.HLAfixedArray;
import hla.rti1516_2025.encoding.HLAfixedRecord;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.encoding.HLAoctet;
import hla.rti1516_2025.encoding.HLAvariantRecord;
import hla.rti1516_2025.encoding.HLAvariableArray;

/** Verifies constituent access and composition through the standard Java encoding API. */
final class ComplexDataElementConstituentAccessTck {
   private static final byte SIMPLE_STATE = 0x2a;
   private static final byte SECOND_SIMPLE_STATE = 0x31;
   private static final byte NESTED_STATE = 0x6c;
   private static final int RECORD_IDENTITY = 1516;
   private static final int SIMPLE_DISCRIMINANT = 1;
   private static final int COMPLEX_DISCRIMINANT = 2;

   private ComplexDataElementConstituentAccessTck() {
   }

   static void run(EncoderFactory encoder) throws Exception {
      arrayConstituents(encoder);
      recordConstituents(encoder);
      variantConstituents(encoder);
      variantRoundTrips(encoder);
   }

   private static void arrayConstituents(EncoderFactory encoder) {
      HLAfixedArray<HLAoctet> fixed = encoder.createHLAfixedArray(
         encoder.createHLAoctet(SIMPLE_STATE),
         encoder.createHLAoctet(SECOND_SIMPLE_STATE));
      JavaTckSupport.check(fixed.size() == 2
            && fixed.get(0).getValue() == SIMPLE_STATE
            && fixed.get(1).getValue() == SECOND_SIMPLE_STATE,
         "HLAfixedArray did not expose its typed simple constituents");

      HLAfixedRecord nestedRecord = record(encoder, RECORD_IDENTITY, NESTED_STATE);
      HLAfixedArray<HLAfixedRecord> nestedFixed = encoder.createHLAfixedArray(nestedRecord);
      JavaTckSupport.check(nestedFixed.size() == 1,
         "HLAfixedArray did not retain its nested record constituent");
      checkRecord(nestedFixed.get(0), RECORD_IDENTITY, NESTED_STATE,
         "HLAfixedArray nested complex element");

      HLAvariableArray<HLAoctet> variable = encoder.createHLAvariableArray(
         index -> encoder.createHLAoctet());
      variable.addElement(encoder.createHLAoctet(SIMPLE_STATE));
      JavaTckSupport.check(variable.size() == 1
            && variable.get(0).getValue() == SIMPLE_STATE,
         "HLAvariableArray did not expose its typed simple constituent");

      HLAvariableArray<HLAfixedRecord> nestedVariable = encoder.createHLAvariableArray(
         index -> record(encoder, 0, (byte) 0));
      nestedVariable.addElement(nestedRecord);
      JavaTckSupport.check(nestedVariable.size() == 1,
         "HLAvariableArray did not retain its nested record constituent");
      checkRecord(nestedVariable.get(0), RECORD_IDENTITY, NESTED_STATE,
         "HLAvariableArray nested complex element");
   }

   private static void recordConstituents(EncoderFactory encoder) {
      HLAfixedRecord composite = encoder.createHLAfixedRecord();
      composite.add(encoder.createHLAoctet(SIMPLE_STATE));
      composite.add(record(encoder, RECORD_IDENTITY, NESTED_STATE));
      JavaTckSupport.check(composite.size() == 2
            && ((HLAoctet) composite.get(0)).getValue() == SIMPLE_STATE,
         "HLAfixedRecord did not expose its simple constituent and size");
      checkRecord((HLAfixedRecord) composite.get(1), RECORD_IDENTITY, NESTED_STATE,
         "HLAfixedRecord nested complex constituent");
   }

   private static void variantConstituents(EncoderFactory encoder) throws Exception {
      HLAinteger32BE simpleDiscriminant = encoder.createHLAinteger32BE(SIMPLE_DISCRIMINANT);
      HLAinteger32BE complexDiscriminant = encoder.createHLAinteger32BE(COMPLEX_DISCRIMINANT);
      HLAfixedRecord nestedRecord = record(encoder, RECORD_IDENTITY, NESTED_STATE);

      HLAvariantRecord<HLAinteger32BE> variant = encoder.createHLAvariantRecord(
         encoder.createHLAinteger32BE());
      variant.setVariant(simpleDiscriminant, encoder.createHLAoctet(SIMPLE_STATE));
      variant.setVariant(complexDiscriminant, nestedRecord);
      variant.setDiscriminant(simpleDiscriminant);
      checkVariant(variant.getDiscriminant(), variant.getValue(), SIMPLE_DISCRIMINANT,
         SIMPLE_STATE, "HLAvariantRecord simple alternative");
      variant.setDiscriminant(complexDiscriminant);
      checkVariant(variant.getDiscriminant(), variant.getValue(), COMPLEX_DISCRIMINANT,
         NESTED_STATE, "HLAvariantRecord nested complex alternative");

      HLAextendableVariantRecord<HLAinteger32BE> extendable =
         encoder.createHLAextendableVariantRecord(encoder.createHLAinteger32BE());
      extendable.setVariant(simpleDiscriminant, encoder.createHLAoctet(SIMPLE_STATE));
      extendable.setVariant(complexDiscriminant, nestedRecord);
      extendable.setDiscriminant(simpleDiscriminant);
      checkVariant(extendable.getDiscriminant(), extendable.getValue(), SIMPLE_DISCRIMINANT,
         SIMPLE_STATE, "HLAextendableVariantRecord simple alternative");
      extendable.setDiscriminant(complexDiscriminant);
      checkVariant(extendable.getDiscriminant(), extendable.getValue(), COMPLEX_DISCRIMINANT,
         NESTED_STATE, "HLAextendableVariantRecord nested complex alternative");
   }

   private static void variantRoundTrips(EncoderFactory encoder) throws Exception {
      for (boolean selectComplex : new boolean[] {false, true}) {
         roundTripVariantRecord(encoder, selectComplex);
         roundTripExtendableVariantRecord(encoder, selectComplex);
      }
   }

   private static void roundTripVariantRecord(EncoderFactory encoder, boolean selectComplex)
         throws Exception {
      HLAinteger32BE simpleDiscriminant = encoder.createHLAinteger32BE(SIMPLE_DISCRIMINANT);
      HLAinteger32BE complexDiscriminant = encoder.createHLAinteger32BE(COMPLEX_DISCRIMINANT);
      HLAvariantRecord<HLAinteger32BE> source = encoder.createHLAvariantRecord(
         encoder.createHLAinteger32BE());
      if (selectComplex) {
         source.setVariant(simpleDiscriminant, encoder.createHLAoctet(SIMPLE_STATE));
         source.setVariant(complexDiscriminant, record(encoder, RECORD_IDENTITY, NESTED_STATE));
         source.setDiscriminant(complexDiscriminant);
      } else {
         source.setVariant(complexDiscriminant, record(encoder, RECORD_IDENTITY, NESTED_STATE));
         source.setVariant(simpleDiscriminant, encoder.createHLAoctet(SIMPLE_STATE));
         source.setDiscriminant(simpleDiscriminant);
      }

      HLAvariantRecord<HLAinteger32BE> decoded = encoder.createHLAvariantRecord(
         encoder.createHLAinteger32BE());
      decoded.setVariant(simpleDiscriminant, encoder.createHLAoctet());
      decoded.setVariant(complexDiscriminant, record(encoder, 0, (byte) 0));
      decoded.decode(source.toByteArray());
      checkVariant(decoded.getDiscriminant(), decoded.getValue(),
         selectComplex ? COMPLEX_DISCRIMINANT : SIMPLE_DISCRIMINANT,
         selectComplex ? NESTED_STATE : SIMPLE_STATE,
         "HLAvariantRecord selected alternative round trip");
   }

   private static void roundTripExtendableVariantRecord(EncoderFactory encoder,
         boolean selectComplex) throws Exception {
      HLAinteger32BE simpleDiscriminant = encoder.createHLAinteger32BE(SIMPLE_DISCRIMINANT);
      HLAinteger32BE complexDiscriminant = encoder.createHLAinteger32BE(COMPLEX_DISCRIMINANT);
      HLAextendableVariantRecord<HLAinteger32BE> source =
         encoder.createHLAextendableVariantRecord(encoder.createHLAinteger32BE());
      if (selectComplex) {
         source.setVariant(simpleDiscriminant, encoder.createHLAoctet(SIMPLE_STATE));
         source.setVariant(complexDiscriminant, record(encoder, RECORD_IDENTITY, NESTED_STATE));
         source.setDiscriminant(complexDiscriminant);
      } else {
         source.setVariant(complexDiscriminant, record(encoder, RECORD_IDENTITY, NESTED_STATE));
         source.setVariant(simpleDiscriminant, encoder.createHLAoctet(SIMPLE_STATE));
         source.setDiscriminant(simpleDiscriminant);
      }

      HLAextendableVariantRecord<HLAinteger32BE> decoded =
         encoder.createHLAextendableVariantRecord(encoder.createHLAinteger32BE());
      decoded.setVariant(simpleDiscriminant, encoder.createHLAoctet());
      decoded.setVariant(complexDiscriminant, record(encoder, 0, (byte) 0));
      decoded.decode(source.toByteArray());
      checkVariant(decoded.getDiscriminant(), decoded.getValue(),
         selectComplex ? COMPLEX_DISCRIMINANT : SIMPLE_DISCRIMINANT,
         selectComplex ? NESTED_STATE : SIMPLE_STATE,
         "HLAextendableVariantRecord selected alternative round trip");
   }

   private static HLAfixedRecord record(EncoderFactory encoder, int identity, byte state) {
      HLAfixedRecord record = encoder.createHLAfixedRecord();
      record.add(encoder.createHLAinteger32BE(identity));
      record.add(encoder.createHLAoctet(state));
      return record;
   }

   private static void checkRecord(HLAfixedRecord actual, int identity, byte state,
         String context) {
      JavaTckSupport.check(actual.size() == 2
            && ((HLAinteger32BE) actual.get(0)).getValue() == identity
            && ((HLAoctet) actual.get(1)).getValue() == state,
         context + " did not expose both nested record values");
   }

   private static void checkVariant(HLAinteger32BE actualDiscriminant, DataElement actualValue,
         int expectedDiscriminant, byte expectedState, String context) {
      JavaTckSupport.check(actualDiscriminant.getValue() == expectedDiscriminant,
         context + " did not expose the selected discriminant");
      if (expectedDiscriminant == SIMPLE_DISCRIMINANT) {
         JavaTckSupport.check(((HLAoctet) actualValue).getValue() == expectedState,
            context + " did not expose its simple alternative");
      } else {
         checkRecord((HLAfixedRecord) actualValue, RECORD_IDENTITY, expectedState, context);
      }
   }
}
