#include "tck_data_encoding.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

rti::Octet fixedRecordOctet(rti::HLAfixedRecord const& record) {
  return dynamic_cast<rti::HLAoctet const&>(record.get(0U)).get();
}

void requireActiveIntegerVariant(
    rti::HLAvariantRecordT<rti::HLAoctet> const& record,
    rti::Octet expectedDiscriminant,
    rti::Integer32 expectedValue,
    std::string const& context) {
  require(
      record.getDiscriminant().get() == expectedDiscriminant &&
          dynamic_cast<rti::HLAinteger32BE const&>(record.getVariant()).get() ==
              expectedValue,
      context);
}

}  // namespace

void verifyRecordCopyCloneExternalMemoryContract() {
  rti::HLAoctet externalRecordElement{static_cast<rti::Octet>(0x41U)};
  rti::HLAfixedRecord fixedRecord;
  fixedRecord.appendElementPointer(&externalRecordElement);
  rti::HLAfixedRecord fixedRecordCopy{fixedRecord};
  auto fixedRecordCloneBase = fixedRecord.clone();
  auto* fixedRecordClone =
      dynamic_cast<rti::HLAfixedRecord*>(fixedRecordCloneBase.get());
  require(
      fixedRecordClone != nullptr,
      "HLAfixedRecord clone lost its concrete API type");

  externalRecordElement.set(static_cast<rti::Octet>(0x42U));
  require(
      fixedRecord.size() == 1U && fixedRecordCopy.size() == 1U &&
          fixedRecordClone->size() == 1U &&
          fixedRecordOctet(fixedRecord) == static_cast<rti::Octet>(0x42U) &&
          fixedRecordOctet(fixedRecordCopy) == static_cast<rti::Octet>(0x41U) &&
          fixedRecordOctet(*fixedRecordClone) ==
              static_cast<rti::Octet>(0x41U) &&
          &fixedRecordCopy.get(0U) != &externalRecordElement &&
          &fixedRecordClone->get(0U) != &externalRecordElement,
      "HLAfixedRecord copy or clone retained external element storage");
  fixedRecordCopy.set(
      0U, rti::HLAoctet{static_cast<rti::Octet>(0x51U)});
  fixedRecordClone->set(
      0U, rti::HLAoctet{static_cast<rti::Octet>(0x61U)});
  require(
      externalRecordElement.get() == static_cast<rti::Octet>(0x42U) &&
          fixedRecordOctet(fixedRecordCopy) ==
              static_cast<rti::Octet>(0x51U) &&
          fixedRecordOctet(*fixedRecordClone) ==
              static_cast<rti::Octet>(0x61U),
      "HLAfixedRecord copy or clone mutation reached another value");

  auto const integerDiscriminant =
      rti::HLAoctet{static_cast<rti::Octet>(7U)};
  auto const alternateDiscriminant =
      rti::HLAoctet{static_cast<rti::Octet>(8U)};
  rti::HLAinteger32BE externalVariant{0x01020304};
  rti::HLAvariantRecordT<rti::HLAoctet> variantRecord;
  variantRecord
      .addVariantPointer(integerDiscriminant, &externalVariant)
      .addVariant(alternateDiscriminant, rti::HLAfloat64BE{})
      .setDiscriminant(integerDiscriminant);
  rti::HLAvariantRecordT<rti::HLAoctet> variantRecordCopy{variantRecord};
  auto variantRecordCloneBase = variantRecord.clone();
  auto* variantRecordClone =
      dynamic_cast<rti::HLAvariantRecordT<rti::HLAoctet>*>(
          variantRecordCloneBase.get());
  require(
      variantRecordClone != nullptr,
      "HLAvariantRecordT clone lost its concrete API type");

  externalVariant.set(0x11121314);
  requireActiveIntegerVariant(
      variantRecord,
      static_cast<rti::Octet>(7U),
      0x11121314,
      "HLAvariantRecord did not observe active caller-owned value changes");
  requireActiveIntegerVariant(
      variantRecordCopy,
      static_cast<rti::Octet>(7U),
      0x01020304,
      "HLAvariantRecord copy did not preserve an independent active value");
  requireActiveIntegerVariant(
      *variantRecordClone,
      static_cast<rti::Octet>(7U),
      0x01020304,
      "HLAvariantRecord clone did not preserve an independent active value");
  require(
      &variantRecordCopy.getVariant() != &externalVariant &&
          &variantRecordClone->getVariant() != &externalVariant,
      "HLAvariantRecord copy or clone retained the external alternative");

  variantRecord.setDiscriminant(alternateDiscriminant);
  variantRecordCopy.setVariant(
      integerDiscriminant, rti::HLAinteger32BE{0x21222324});
  variantRecordClone->setVariant(
      integerDiscriminant, rti::HLAinteger32BE{0x31323334});
  require(
      variantRecord.getDiscriminant().get() ==
              static_cast<rti::Octet>(8U) &&
          dynamic_cast<rti::HLAfloat64BE const&>(
              variantRecord.getVariant()).get() == 0.0 &&
          externalVariant.get() == 0x11121314,
      "HLAvariantRecord active alternative change affected external storage");
  requireActiveIntegerVariant(
      variantRecordCopy,
      static_cast<rti::Octet>(7U),
      0x21222324,
      "HLAvariantRecord copy mutation changed its discriminant or clone");
  requireActiveIntegerVariant(
      *variantRecordClone,
      static_cast<rti::Octet>(7U),
      0x31323334,
      "HLAvariantRecord clone mutation changed its discriminant or copy");
}

}  // namespace hla_rti_cpp_tck::detail
