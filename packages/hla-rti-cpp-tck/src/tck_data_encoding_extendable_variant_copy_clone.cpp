#include "tck_data_encoding.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

void requireActiveShortExtendableVariant(
    rti::HLAextendableVariantRecordT<rti::HLAoctet> const& record,
    rti::Octet expectedDiscriminant,
    rti::Integer16 expectedValue,
    std::string const& context) {
  require(
      record.getDiscriminant().get() == expectedDiscriminant &&
          dynamic_cast<rti::HLAinteger16BE const&>(record.getVariant()).get() ==
              expectedValue,
      context);
}

}  // namespace

void verifyExtendableVariantCopyCloneContract() {
  auto const originalDiscriminant =
      rti::HLAoctet{static_cast<rti::Octet>(2U)};
  auto const sourceAddedDiscriminant =
      rti::HLAoctet{static_cast<rti::Octet>(3U)};
  auto const copyAddedDiscriminant =
      rti::HLAoctet{static_cast<rti::Octet>(4U)};
  rti::HLAinteger16BE externalValue{23};
  rti::HLAextendableVariantRecordT<rti::HLAoctet> original;
  original
      .addVariantPointer(originalDiscriminant, &externalValue)
      .setDiscriminant(originalDiscriminant);
  rti::HLAextendableVariantRecordT<rti::HLAoctet> copied{original};
  auto clonedBase = original.clone();
  auto* cloned = dynamic_cast<
      rti::HLAextendableVariantRecordT<rti::HLAoctet>*>(clonedBase.get());
  require(cloned != nullptr, "HLAextendableVariantRecordT clone lost its type");

  externalValue.set(24);
  requireActiveShortExtendableVariant(
      original,
      static_cast<rti::Octet>(2U),
      24,
      "HLAextendableVariantRecordT did not observe caller-owned value changes");
  requireActiveShortExtendableVariant(
      copied,
      static_cast<rti::Octet>(2U),
      23,
      "HLAextendableVariantRecordT copy retained external storage");
  requireActiveShortExtendableVariant(
      *cloned,
      static_cast<rti::Octet>(2U),
      23,
      "HLAextendableVariantRecordT clone retained external storage");
  require(
      &copied.getVariant() != &externalValue &&
          &cloned->getVariant() != &externalValue,
      "HLAextendableVariantRecordT copy or clone retained the external value");

  original.addVariant(sourceAddedDiscriminant, rti::HLAfloat64BE{});
  copied.addVariant(copyAddedDiscriminant, rti::HLAoctet{});
  require(
      original.isSameTypeAs(sourceAddedDiscriminant, rti::HLAfloat64BE{}) &&
          !copied.isSameTypeAs(sourceAddedDiscriminant, rti::HLAfloat64BE{}) &&
          !cloned->isSameTypeAs(
              sourceAddedDiscriminant, rti::HLAfloat64BE{}) &&
          copied.isSameTypeAs(copyAddedDiscriminant, rti::HLAoctet{}) &&
          !original.isSameTypeAs(copyAddedDiscriminant, rti::HLAoctet{}) &&
          !cloned->isSameTypeAs(copyAddedDiscriminant, rti::HLAoctet{}),
      "HLAextendableVariantRecordT copy or clone shared alternative metadata");

  copied.setVariant(
      originalDiscriminant, rti::HLAinteger16BE{25});
  cloned->setVariant(
      originalDiscriminant, rti::HLAinteger16BE{27});
  requireActiveShortExtendableVariant(
      copied,
      static_cast<rti::Octet>(2U),
      25,
      "HLAextendableVariantRecordT copy mutation changed active state");
  requireActiveShortExtendableVariant(
      *cloned,
      static_cast<rti::Octet>(2U),
      27,
      "HLAextendableVariantRecordT clone mutation changed active state");
  require(
      externalValue.get() == 24 &&
          original.getDiscriminant().get() == static_cast<rti::Octet>(3U),
      "HLAextendableVariantRecordT copy or clone mutation changed source storage or active discriminant");
}

}  // namespace hla_rti_cpp_tck::detail
