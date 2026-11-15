#include "tck_data_encoding.hpp"

#include <string>
#include <type_traits>

namespace hla_rti_cpp_tck::detail {
namespace {

rti::HLAfixedRecord makeRecord(rti::Integer32 const identity,
                               rti::Octet const state) {
  rti::HLAfixedRecord record;
  record.appendElement(rti::HLAinteger32BE{identity})
      .appendElement(rti::HLAoctet{state});
  return record;
}

void requireRecordValues(rti::DataElement const& element,
                         rti::Integer32 const expectedIdentity,
                         rti::Octet const expectedState,
                         std::string const& context) {
  auto const& record = dynamic_cast<rti::HLAfixedRecord const&>(element);
  require(record.size() == 2U, context + " did not retain both record elements");
  auto const& identity =
      dynamic_cast<rti::HLAinteger32BE const&>(record.get(0U));
  auto const& state = dynamic_cast<rti::HLAoctet const&>(record[1U]);
  require(
      identity.get() == expectedIdentity && state.get() == expectedState,
      context + " did not expose the nested record constituent values");
}

void requireOctetValue(rti::DataElement const& element,
                       rti::Octet const expected,
                       std::string const& context) {
  auto const& octet = dynamic_cast<rti::HLAoctet const&>(element);
  require(octet.get() == expected, context + " did not expose its simple value");
}

void requireVariantDiscriminant(rti::DataElement const& element,
                                rti::Integer32 const expected,
                                std::string const& context) {
  auto const& discriminant =
      dynamic_cast<rti::HLAinteger32BE const&>(element);
  require(
      discriminant.get() == expected,
      context + " did not expose the selected discriminant");
}

rti::VariableLengthData encodeThroughDataElementBase(
    rti::DataElement const& element) {
  return element.encode();
}

void decodeThroughDataElementBase(
    rti::DataElement& element,
    rti::VariableLengthData const& encoded) {
  static_cast<void>(element.decode(encoded));
}

template <typename VariantRecord>
void requireSelectedVariantRoundTrip(
    std::string const& variantName,
    rti::HLAinteger32BE const& simpleDiscriminant,
    rti::HLAinteger32BE const& complexDiscriminant,
    rti::HLAoctet const& simpleValue,
    rti::HLAfixedRecord const& complexValue,
    rti::Integer32 const expectedIdentity,
    rti::Octet const expectedComplexState,
    bool const selectComplex) {
  auto addAlternatives = [&](VariantRecord& record) {
    record.addVariant(simpleDiscriminant, rti::HLAoctet{});
    record.addVariant(complexDiscriminant, complexValue);
  };
  auto const expectedDiscriminant =
      selectComplex ? complexDiscriminant.get() : simpleDiscriminant.get();
  auto const selection = selectComplex ? "complex" : "simple";
  auto const context = variantName + " selected " + selection + " alternative";

  VariantRecord source{rti::HLAinteger32BE{}};
  addAlternatives(source);
  if (selectComplex) {
    source.setVariant(simpleDiscriminant, simpleValue);
    source.setVariant(complexDiscriminant, complexValue);
    source.setDiscriminant(complexDiscriminant);
  } else {
    source.setVariant(complexDiscriminant, complexValue);
    source.setVariant(simpleDiscriminant, simpleValue);
    source.setDiscriminant(simpleDiscriminant);
  }
  requireVariantDiscriminant(
      source.getDiscriminant(), expectedDiscriminant, context + " before encode");
  if (selectComplex) {
    requireRecordValues(
        source.getVariant(), expectedIdentity, expectedComplexState,
        context + " composition");
  } else {
    requireOctetValue(source.getVariant(), simpleValue.get(),
                      context + " composition");
  }

  auto const encoded = encodeThroughDataElementBase(source);
  VariantRecord decoded{rti::HLAinteger32BE{}};
  addAlternatives(decoded);
  auto const previousDiscriminant =
      selectComplex ? simpleDiscriminant : complexDiscriminant;
  if (selectComplex) {
    decoded.setVariant(simpleDiscriminant, simpleValue);
    decoded.setDiscriminant(simpleDiscriminant);
  } else {
    decoded.setVariant(complexDiscriminant, complexValue);
    decoded.setDiscriminant(complexDiscriminant);
  }
  requireVariantDiscriminant(
      decoded.getDiscriminant(), previousDiscriminant.get(),
      context + " receiver before decode");
  decodeThroughDataElementBase(decoded, encoded);
  requireVariantDiscriminant(
      decoded.getDiscriminant(), expectedDiscriminant, context + " after decode");
  if (selectComplex) {
    requireRecordValues(
        decoded.getVariant(), expectedIdentity, expectedComplexState,
        context + " decoded composition");
  } else {
    requireOctetValue(decoded.getVariant(), simpleValue.get(),
                      context + " decoded composition");
  }
}

}  // namespace

void scenarioComplexDataElementConstituentAccess(
    Options const&,
    rti::CallbackModel) {
  static_assert(std::is_class_v<rti1516_2025::HLAfixedArray>);
  static_assert(std::is_class_v<rti1516_2025::HLAvariableArray>);
  static_assert(std::is_class_v<rti1516_2025::HLAfixedRecord>);
  static_assert(std::is_class_v<rti1516_2025::HLAvariantRecord>);
  static_assert(std::is_class_v<rti1516_2025::HLAextendableVariantRecord>);
  static_assert(
      std::is_class_v<rti1516_2025::HLAfixedArrayT<rti1516_2025::HLAoctet>>);
  static_assert(std::is_class_v<
                rti1516_2025::HLAvariableArrayT<rti1516_2025::HLAoctet>>);
  static_assert(std::is_class_v<
                rti1516_2025::HLAvariantRecordT<rti1516_2025::HLAinteger32BE>>);

  auto const simpleState = static_cast<rti::Octet>(0x2aU);
  auto const nestedState = static_cast<rti::Octet>(0x6cU);
  auto const nestedRecord = makeRecord(1516, nestedState);

  rti::HLAfixedArray simpleArray{rti::HLAoctet{}, 2U};
  simpleArray
      .set(0U, rti::HLAoctet{simpleState})
      .set(1U, rti::HLAoctet{static_cast<rti::Octet>(0x31U)});
  require(simpleArray.size() == 2U, "HLAfixedArray lost its elements");
  requireOctetValue(simpleArray.get(0U), simpleState, "HLAfixedArray::get");
  requireOctetValue(simpleArray[1U], static_cast<rti::Octet>(0x31U),
                    "HLAfixedArray::operator[]");
  auto const encodedSimpleArray = encodeThroughDataElementBase(simpleArray);
  rti::HLAfixedArray decodedSimpleArray{rti::HLAoctet{}, 2U};
  decodedSimpleArray
      .set(0U, rti::HLAoctet{static_cast<rti::Octet>(0x40U)})
      .set(1U, rti::HLAoctet{static_cast<rti::Octet>(0x41U)});
  require(decodedSimpleArray.size() == 2U,
          "HLAfixedArray decode replacement setup changed its fixed size");
  decodeThroughDataElementBase(decodedSimpleArray, encodedSimpleArray);
  require(
      decodedSimpleArray.size() == 2U,
      "HLAfixedArray encode/decode changed the element count");
  requireOctetValue(
      decodedSimpleArray.get(0U), simpleState,
      "HLAfixedArray decoded first element");
  requireOctetValue(
      decodedSimpleArray.get(1U), static_cast<rti::Octet>(0x31U),
      "HLAfixedArray decoded second element");
  rti::HLAfixedArrayT<rti::HLAoctet> templatedFixedArray{1U};
  templatedFixedArray.set(0U, rti::HLAoctet{simpleState});
  require(
      templatedFixedArray.size() == 1U &&
          templatedFixedArray.get(0U).get() == simpleState,
      "HLAfixedArrayT did not expose its templated element");
  auto const encodedTemplatedFixedArray =
      encodeThroughDataElementBase(templatedFixedArray);
  rti::HLAfixedArrayT<rti::HLAoctet> decodedTemplatedFixedArray{1U};
  decodedTemplatedFixedArray.set(0U, rti::HLAoctet{0x40U});
  decodeThroughDataElementBase(
      decodedTemplatedFixedArray, encodedTemplatedFixedArray);
  require(
      decodedTemplatedFixedArray.size() == 1U &&
          decodedTemplatedFixedArray.get(0U).get() == simpleState,
      "HLAfixedArrayT encode/decode did not preserve its element");

  rti::HLAfixedArray nestedFixedArray{nestedRecord, 1U};
  nestedFixedArray.set(0U, nestedRecord);
  requireRecordValues(nestedFixedArray.get(0U), 1516, nestedState,
                      "HLAfixedArray nested complex element");
  auto const encodedNestedFixedArray =
      encodeThroughDataElementBase(nestedFixedArray);
  rti::HLAfixedArray decodedNestedFixedArray{makeRecord(0, 0), 1U};
  decodeThroughDataElementBase(decodedNestedFixedArray, encodedNestedFixedArray);
  require(decodedNestedFixedArray.size() == 1U,
          "HLAfixedArray encode/decode changed its nested element count");
  requireRecordValues(decodedNestedFixedArray.get(0U), 1516, nestedState,
                      "HLAfixedArray decoded nested complex element");

  rti::HLAvariableArray simpleVariableArray{rti::HLAoctet{}};
  require(simpleVariableArray.size() == 0U,
          "HLAvariableArray prototype was counted as an encoded element");
  auto const encodedEmptyVariableArray =
      encodeThroughDataElementBase(simpleVariableArray);
  rti::HLAvariableArray decodedEmptyVariableArray{rti::HLAoctet{}};
  decodeThroughDataElementBase(
      decodedEmptyVariableArray, encodedEmptyVariableArray);
  require(decodedEmptyVariableArray.size() == 0U,
          "HLAvariableArray empty encode/decode counted its prototype");
  rti::HLAvariableArray decodedPreviouslyPopulatedVariableArray{
      rti::HLAoctet{}};
  decodedPreviouslyPopulatedVariableArray.addElement(
      rti::HLAoctet{simpleState});
  decodedPreviouslyPopulatedVariableArray.addElement(
      rti::HLAoctet{static_cast<rti::Octet>(0x31U)});
  require(decodedPreviouslyPopulatedVariableArray.size() == 2U,
          "HLAvariableArray decode replacement setup lost its elements");
  decodeThroughDataElementBase(
      decodedPreviouslyPopulatedVariableArray, encodedEmptyVariableArray);
  require(decodedPreviouslyPopulatedVariableArray.size() == 0U,
          "HLAvariableArray decoding an empty value retained stale elements");
  simpleVariableArray.addElement(rti::HLAoctet{simpleState});
  require(simpleVariableArray.size() == 1U,
          "HLAvariableArray lost its simple element");
  auto const secondVariableArrayState = static_cast<rti::Octet>(0x31U);
  simpleVariableArray.addElement(rti::HLAoctet{secondVariableArrayState});
  require(simpleVariableArray.size() == 2U,
          "HLAvariableArray::size did not track the second encoded element");
  requireOctetValue(simpleVariableArray[0U], simpleState,
                    "HLAvariableArray::operator[]");
  requireOctetValue(simpleVariableArray.get(1U), secondVariableArrayState,
                    "HLAvariableArray::get after size growth");
  auto const encodedSimpleVariableArray =
      encodeThroughDataElementBase(simpleVariableArray);
  rti::HLAvariableArray decodedSimpleVariableArray{rti::HLAoctet{}};
  decodedSimpleVariableArray.addElement(rti::HLAoctet{0x40U});
  decodedSimpleVariableArray.addElement(rti::HLAoctet{0x41U});
  decodedSimpleVariableArray.addElement(rti::HLAoctet{0x42U});
  require(decodedSimpleVariableArray.size() == 3U,
          "HLAvariableArray decode replacement setup lost its elements");
  decodeThroughDataElementBase(
      decodedSimpleVariableArray, encodedSimpleVariableArray);
  require(
      decodedSimpleVariableArray.size() == 2U,
      "HLAvariableArray encode/decode changed the element count");
  requireOctetValue(
      decodedSimpleVariableArray.get(0U), simpleState,
      "HLAvariableArray decoded first element");
  requireOctetValue(
      decodedSimpleVariableArray.get(1U), secondVariableArrayState,
      "HLAvariableArray decoded second element");
  rti::HLAvariableArrayT<rti::HLAoctet> templatedVariableArray;
  templatedVariableArray.addElement(rti::HLAoctet{simpleState});
  require(
      templatedVariableArray.size() == 1U &&
          templatedVariableArray.get(0U).get() == simpleState,
      "HLAvariableArrayT did not expose its templated element");
  auto const encodedTemplatedVariableArray =
      encodeThroughDataElementBase(templatedVariableArray);
  rti::HLAvariableArrayT<rti::HLAoctet> decodedTemplatedVariableArray;
  decodedTemplatedVariableArray.addElement(rti::HLAoctet{0x40U});
  decodedTemplatedVariableArray.addElement(rti::HLAoctet{0x41U});
  require(decodedTemplatedVariableArray.size() == 2U,
          "HLAvariableArrayT decode replacement setup lost its elements");
  decodeThroughDataElementBase(
      decodedTemplatedVariableArray, encodedTemplatedVariableArray);
  require(
      decodedTemplatedVariableArray.size() == 1U &&
          decodedTemplatedVariableArray.get(0U).get() == simpleState,
      "HLAvariableArrayT encode/decode did not preserve its element");

  rti::HLAvariableArray nestedVariableArray{nestedRecord};
  nestedVariableArray.addElement(nestedRecord);
  requireRecordValues(nestedVariableArray.get(0U), 1516, nestedState,
                      "HLAvariableArray nested complex element");
  auto const encodedNestedVariableArray =
      encodeThroughDataElementBase(nestedVariableArray);
  rti::HLAvariableArray decodedNestedVariableArray{makeRecord(0, 0)};
  decodeThroughDataElementBase(
      decodedNestedVariableArray, encodedNestedVariableArray);
  require(decodedNestedVariableArray.size() == 1U,
          "HLAvariableArray encode/decode changed its nested element count");
  requireRecordValues(decodedNestedVariableArray.get(0U), 1516, nestedState,
                      "HLAvariableArray decoded nested complex element");

  rti::HLAfixedRecord compositeRecord;
  compositeRecord
      .appendElement(rti::HLAoctet{simpleState})
      .appendElement(nestedRecord);
  require(compositeRecord.size() == 2U,
          "HLAfixedRecord did not expose both constituents");
  requireOctetValue(compositeRecord.get(0U), simpleState,
                    "HLAfixedRecord simple constituent");
  requireRecordValues(compositeRecord[1U], 1516, nestedState,
                      "HLAfixedRecord nested complex constituent");
  auto const encodedCompositeRecord =
      encodeThroughDataElementBase(compositeRecord);
  rti::HLAfixedRecord decodedCompositeRecord;
  decodedCompositeRecord
      .appendElement(rti::HLAoctet{})
      .appendElement(makeRecord(0, 0));
  decodeThroughDataElementBase(decodedCompositeRecord, encodedCompositeRecord);
  require(decodedCompositeRecord.size() == 2U,
          "HLAfixedRecord encode/decode changed its constituent count");
  requireOctetValue(decodedCompositeRecord.get(0U), simpleState,
                    "HLAfixedRecord decoded simple constituent");
  requireRecordValues(decodedCompositeRecord.get(1U), 1516, nestedState,
                      "HLAfixedRecord decoded nested complex constituent");
  auto const encodedNestedRecord = encodeThroughDataElementBase(nestedRecord);
  auto decodedNestedRecord = makeRecord(0, 0);
  decodeThroughDataElementBase(decodedNestedRecord, encodedNestedRecord);
  requireRecordValues(
      decodedNestedRecord, 1516, nestedState,
      "HLAfixedRecord encode/decode");

  rti::HLAinteger32BE const simpleDiscriminant{1};
  rti::HLAinteger32BE const complexDiscriminant{2};
  rti::HLAvariantRecord variantRecord{rti::HLAinteger32BE{}};
  variantRecord
      .addVariant(simpleDiscriminant, rti::HLAoctet{})
      .addVariant(complexDiscriminant, nestedRecord);
  variantRecord.setVariant(simpleDiscriminant, rti::HLAoctet{simpleState});
  variantRecord.setDiscriminant(simpleDiscriminant);
  requireVariantDiscriminant(variantRecord.getDiscriminant(), 1,
                             "HLAvariantRecord simple alternative");
  requireOctetValue(variantRecord.getVariant(), simpleState,
                    "HLAvariantRecord simple alternative");
  variantRecord.setVariant(complexDiscriminant, nestedRecord);
  variantRecord.setDiscriminant(complexDiscriminant);
  requireVariantDiscriminant(variantRecord.getDiscriminant(), 2,
                             "HLAvariantRecord complex alternative");
  requireRecordValues(variantRecord.getVariant(), 1516, nestedState,
                      "HLAvariantRecord nested complex alternative");
  rti::HLAvariantRecordT<rti::HLAinteger32BE> templatedVariantRecord;
  templatedVariantRecord
      .addVariant(simpleDiscriminant, rti::HLAoctet{})
      .addVariant(complexDiscriminant, nestedRecord)
      .setVariant(simpleDiscriminant, rti::HLAoctet{simpleState})
      .setDiscriminant(simpleDiscriminant);
  require(
      templatedVariantRecord.getDiscriminant().get() == 1,
      "HLAvariantRecordT did not expose its templated discriminant");
  requireOctetValue(templatedVariantRecord.getVariant(), simpleState,
                    "HLAvariantRecordT selected alternative");
  auto const encodedTemplatedVariantRecord =
      encodeThroughDataElementBase(templatedVariantRecord);
  rti::HLAvariantRecordT<rti::HLAinteger32BE> decodedTemplatedVariantRecord;
  decodedTemplatedVariantRecord
      .addVariant(simpleDiscriminant, rti::HLAoctet{})
      .addVariant(complexDiscriminant, nestedRecord)
      .setVariant(complexDiscriminant, nestedRecord)
      .setDiscriminant(complexDiscriminant);
  require(
      decodedTemplatedVariantRecord.getDiscriminant().get() == 2,
      "HLAvariantRecordT decode replacement setup selected the wrong variant");
  decodeThroughDataElementBase(
      decodedTemplatedVariantRecord, encodedTemplatedVariantRecord);
  require(
      decodedTemplatedVariantRecord.getDiscriminant().get() == 1,
      "HLAvariantRecordT encode/decode changed the selected discriminant");
  requireOctetValue(
      decodedTemplatedVariantRecord.getVariant(), simpleState,
      "HLAvariantRecordT decoded alternative");
  rti::HLAvariantRecordT<rti::HLAinteger32BE> complexTemplatedVariantRecord;
  complexTemplatedVariantRecord
      .addVariant(simpleDiscriminant, rti::HLAoctet{})
      .addVariant(complexDiscriminant, nestedRecord)
      .setVariant(simpleDiscriminant, rti::HLAoctet{simpleState})
      .setVariant(complexDiscriminant, nestedRecord)
      .setDiscriminant(complexDiscriminant);
  auto const encodedComplexTemplatedVariantRecord =
      encodeThroughDataElementBase(complexTemplatedVariantRecord);
  rti::HLAvariantRecordT<rti::HLAinteger32BE>
      decodedComplexTemplatedVariantRecord;
  decodedComplexTemplatedVariantRecord
      .addVariant(simpleDiscriminant, rti::HLAoctet{})
      .addVariant(complexDiscriminant, nestedRecord)
      .setVariant(simpleDiscriminant, rti::HLAoctet{simpleState})
      .setDiscriminant(simpleDiscriminant);
  decodeThroughDataElementBase(
      decodedComplexTemplatedVariantRecord,
      encodedComplexTemplatedVariantRecord);
  require(
      decodedComplexTemplatedVariantRecord.getDiscriminant().get() == 2,
      "HLAvariantRecordT decode did not replace simple with complex alternative");
  requireRecordValues(decodedComplexTemplatedVariantRecord.getVariant(), 1516,
                      nestedState,
                      "HLAvariantRecordT decoded complex alternative");

  rti::HLAextendableVariantRecord extendableVariantRecord{
      rti::HLAinteger32BE{}};
  extendableVariantRecord
      .addVariant(simpleDiscriminant, rti::HLAoctet{})
      .addVariant(complexDiscriminant, nestedRecord);
  extendableVariantRecord.setVariant(
      simpleDiscriminant, rti::HLAoctet{simpleState});
  extendableVariantRecord.setDiscriminant(simpleDiscriminant);
  requireVariantDiscriminant(
      extendableVariantRecord.getDiscriminant(), 1,
      "HLAextendableVariantRecord simple alternative");
  requireOctetValue(
      extendableVariantRecord.getVariant(), simpleState,
      "HLAextendableVariantRecord simple alternative");
  extendableVariantRecord.setVariant(complexDiscriminant, nestedRecord);
  extendableVariantRecord.setDiscriminant(complexDiscriminant);
  requireVariantDiscriminant(
      extendableVariantRecord.getDiscriminant(), 2,
      "HLAextendableVariantRecord complex alternative");
  requireRecordValues(
      extendableVariantRecord.getVariant(), 1516, nestedState,
      "HLAextendableVariantRecord nested complex alternative");

  requireSelectedVariantRoundTrip<rti::HLAvariantRecord>(
      "HLAvariantRecord",
      simpleDiscriminant,
      complexDiscriminant,
      rti::HLAoctet{simpleState},
      nestedRecord,
      1516,
      nestedState,
      false);
  requireSelectedVariantRoundTrip<rti::HLAvariantRecord>(
      "HLAvariantRecord",
      simpleDiscriminant,
      complexDiscriminant,
      rti::HLAoctet{simpleState},
      nestedRecord,
      1516,
      nestedState,
      true);
  requireSelectedVariantRoundTrip<rti::HLAextendableVariantRecord>(
      "HLAextendableVariantRecord",
      simpleDiscriminant,
      complexDiscriminant,
      rti::HLAoctet{simpleState},
      nestedRecord,
      1516,
      nestedState,
      false);
  requireSelectedVariantRoundTrip<rti::HLAextendableVariantRecord>(
      "HLAextendableVariantRecord",
      simpleDiscriminant,
      complexDiscriminant,
      rti::HLAoctet{simpleState},
      nestedRecord,
      1516,
      nestedState,
      true);
}

}  // namespace hla_rti_cpp_tck::detail
