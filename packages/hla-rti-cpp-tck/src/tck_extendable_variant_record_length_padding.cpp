#include "tck_data_encoding.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioExtendableVariantRecordLengthPadding(
    Options const&, rti::CallbackModel) {
  rti::HLAextendableVariantRecord record{rti::HLAoctet{}};
  auto const discriminant =
      rti::HLAoctet{static_cast<rti::Octet>(1U)};
  record.addVariant(discriminant, rti::HLAfloat32LE{});
  record.setVariant(discriminant, rti::HLAfloat32LE{1.0F});

  // The one-octet discriminant leaves the four-octet encoded_length element
  // unaligned. Three zero octets make its start the smallest valid multiple
  // of four; the following length is four, then the alternative payload.
  std::vector<rti::Octet> const expected{
      static_cast<rti::Octet>(1U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(4U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0x80U),
      static_cast<rti::Octet>(0x3fU)};
  std::vector<rti::Octet> actual;
  record.encodeInto(actual);

  require(
      actual == expected,
      "HLAextendableVariantRecord did not align encoded_length after the discriminant");
  require(
      record.getEncodedLength() == expected.size(),
      "HLAextendableVariantRecord encoded length did not include discriminant padding");
}

}  // namespace hla_rti_cpp_tck::detail
