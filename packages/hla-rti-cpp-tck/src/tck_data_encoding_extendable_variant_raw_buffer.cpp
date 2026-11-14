#include "tck_data_encoding.hpp"

namespace hla_rti_cpp_tck::detail {

void verifyExtendableVariantRawBufferContract() {
  rti::HLAoctet const discriminant{static_cast<rti::Octet>(1U)};
  rti::HLAextendableVariantRecordT<rti::HLAoctet> record;
  record.addVariant(discriminant, rti::HLAinteger16BE{});
  record.setVariant(discriminant, rti::HLAinteger16BE{9});

  std::vector<rti::Octet> buffer{static_cast<rti::Octet>(0x7eU)};
  record.encodeInto(buffer);
  std::vector<rti::Octet> const expected{
      static_cast<rti::Octet>(0x7eU),
      static_cast<rti::Octet>(1U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(2U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(9U)};
  require(
      buffer == expected,
      "HLAextendableVariantRecordT encodeInto did not append the exact encoding");

  rti::HLAextendableVariantRecordT<rti::HLAoctet> decoded;
  decoded.addVariant(discriminant, rti::HLAinteger16BE{});
  auto const cursor = decoded.decodeFrom(buffer, 1U);
  require(
      cursor == buffer.size() &&
          decoded.getDiscriminant().get() ==
              static_cast<rti::Octet>(1U) &&
          dynamic_cast<rti::HLAinteger16BE const&>(decoded.getVariant()).get() == 9,
      "HLAextendableVariantRecordT decodeFrom did not decode the prefixed slice");

  auto truncated = buffer;
  truncated.pop_back();
  rti::HLAextendableVariantRecordT<rti::HLAoctet> truncatedRecord;
  truncatedRecord.addVariant(discriminant, rti::HLAinteger16BE{});
  requireException(
      [&] { truncatedRecord.decodeFrom(truncated, 1U); },
      L"EncoderException",
      "truncated HLAextendableVariantRecordT decodeFrom slice");
}

}  // namespace hla_rti_cpp_tck::detail
