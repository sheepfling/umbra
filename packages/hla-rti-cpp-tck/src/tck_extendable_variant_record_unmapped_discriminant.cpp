#include "tck_data_encoding.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioExtendableVariantRecordUnmappedDiscriminant(
    Options const&, rti::CallbackModel) {
  auto const mappedDiscriminant = rti::HLAfloat64BE{1.5};
  auto const unmappedDiscriminant = rti::HLAfloat64BE{2.5};
  rti::HLAextendableVariantRecord record{rti::HLAfloat64BE{}};
  record.addVariant(mappedDiscriminant, rti::HLAinteger32BE{});
  record.setDiscriminant(unmappedDiscriminant);

  // The unmapped discriminant is followed by encoded_length == 0 at offset
  // eight. Equation (4) still contributes four separate alignment octets
  // after the length field: (8 + 4 + 4) mod 8 == 0.
  std::vector<rti::Octet> const expected{
      static_cast<rti::Octet>(0x40U),
      static_cast<rti::Octet>(0x04U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U)};
  std::vector<rti::Octet> actual;
  record.encodeInto(actual);

  require(
      actual == expected,
      "HLAextendableVariantRecord did not encode a zero length and required alignment for an unmapped discriminant");
  require(
      record.getOctetBoundary() == 8U &&
          record.getEncodedLength() == expected.size(),
      "HLAextendableVariantRecord unmapped-discriminant layout metadata was incorrect");
}

}  // namespace hla_rti_cpp_tck::detail
