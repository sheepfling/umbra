#include "tck_data_encoding.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioVariantRecordUnmappedDiscriminant(
    Options const&, rti::CallbackModel) {
  rti::HLAvariantRecord record{rti::HLAoctet{}};
  record.addVariant(
      rti::HLAoctet{static_cast<rti::Octet>(1U)},
      rti::HLAinteger32BE{});
  record.addVariant(
      rti::HLAoctet{static_cast<rti::Octet>(2U)},
      rti::HLAfloat64BE{});
  record.setDiscriminant(
      rti::HLAoctet{static_cast<rti::Octet>(0x7fU)});

  // No alternative is associated with 0x7f. Even though the mapped
  // alternatives have an eight-octet maximum boundary, the encoded record is
  // only the discriminant and has no following padding.
  std::vector<rti::Octet> const expected{
      static_cast<rti::Octet>(0x7fU)};
  std::vector<rti::Octet> actual;
  record.encodeInto(actual);

  require(
      actual == expected,
      "HLAvariantRecord added padding after an unmapped discriminant");
  require(
      record.getOctetBoundary() == 8U &&
          record.getEncodedLength() == expected.size(),
      "HLAvariantRecord unmapped-discriminant layout metadata was incorrect");
}

}  // namespace hla_rti_cpp_tck::detail
