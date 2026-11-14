#include "tck_data_encoding.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioVariantRecordDiscriminantPadding(
    Options const&, rti::CallbackModel) {
  rti::HLAvariantRecord record{rti::HLAoctet{}};
  record.addVariant(
      rti::HLAoctet{static_cast<rti::Octet>(1U)},
      rti::HLAinteger32BE{});
  record.addVariant(
      rti::HLAoctet{static_cast<rti::Octet>(2U)},
      rti::HLAfloat64BE{});
  record.setVariant(
      rti::HLAoctet{static_cast<rti::Octet>(1U)},
      rti::HLAinteger32BE{0x10203040});

  // The one-octet discriminant precedes an alternative selected from the
  // four-octet type, but the other alternative requires an eight-octet
  // boundary. Equation (2)'s minimum padding is therefore seven zero octets.
  std::vector<rti::Octet> const expected{
      static_cast<rti::Octet>(1U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0x10U),
      static_cast<rti::Octet>(0x20U),
      static_cast<rti::Octet>(0x30U),
      static_cast<rti::Octet>(0x40U)};
  std::vector<rti::Octet> actual;
  record.encodeInto(actual);

  require(
      actual == expected,
      "HLAvariantRecord discriminant padding did not align the selected alternative to the maximum alternative boundary");
  require(
      record.getEncodedLength() == expected.size() &&
          record.getOctetBoundary() == 8U,
      "HLAvariantRecord discriminant padding metadata was incorrect");
}

}  // namespace hla_rti_cpp_tck::detail
