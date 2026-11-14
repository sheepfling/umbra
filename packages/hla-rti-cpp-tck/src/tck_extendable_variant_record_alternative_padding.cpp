#include "tck_data_encoding.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioExtendableVariantRecordAlternativePadding(
    Options const&, rti::CallbackModel) {
  auto const discriminant = rti::HLAfloat64BE{1.5};
  rti::HLAextendableVariantRecord record{rti::HLAfloat64BE{}};
  record.addVariant(discriminant, rti::HLAinteger32BE{});
  record.setVariant(discriminant, rti::HLAinteger32BE{0x10203040});

  // The eight-octet discriminant and four-octet length leave the alternative
  // at offset 12. Four zero octets align it to the record's eight-octet
  // boundary. The four-byte alternative then ends at offset 20: the record
  // includes the alignment padding but adds no trailing padding.
  std::vector<rti::Octet> const expected{
      static_cast<rti::Octet>(0x3fU),
      static_cast<rti::Octet>(0xf8U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(4U),
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
      "HLAextendableVariantRecord did not align the alternative or emitted trailing padding");
  require(
      record.getEncodedLength() == expected.size(),
      "HLAextendableVariantRecord size did not include alignment padding exactly once");
}

}  // namespace hla_rti_cpp_tck::detail
