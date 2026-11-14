#include "tck_data_encoding.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioVariableArrayInterElementPadding(
    Options const&, rti::CallbackModel) {
  rti::HLAfixedRecord prototype;
  prototype.appendElement(rti::HLAinteger32BE{}).appendElement(rti::HLAoctet{});
  rti::HLAvariableArray values{prototype};

  rti::HLAfixedRecord first;
  first.appendElement(rti::HLAinteger32BE{1})
      .appendElement(rti::HLAoctet{static_cast<rti::Octet>(0x11U)});
  rti::HLAfixedRecord second;
  second.appendElement(rti::HLAinteger32BE{2})
      .appendElement(rti::HLAoctet{static_cast<rti::Octet>(0x22U)});
  values.addElement(first).addElement(second);

  // The first five-octet record ends at offset nine. Three zero octets reach
  // the fixed record's four-octet boundary before the second element; these
  // inter-element bytes are included in the variable-array encoded size.
  // HLAfixedArray's corresponding rule excludes padding after the final
  // element, so the second record ends the array at offset seventeen.
  std::vector<rti::Octet> const expected{
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(2U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(1U),
      static_cast<rti::Octet>(0x11U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(2U),
      static_cast<rti::Octet>(0x22U)};
  std::vector<rti::Octet> actual;
  values.encodeInto(actual);

  require(
      actual == expected,
      "HLAvariableArray did not align its second element after the first record");
  require(
      values.getOctetBoundary() == 4U &&
          values.size() == 2U &&
          values.getEncodedLength() == expected.size(),
      "HLAvariableArray size did not include inter-element padding");
}

}  // namespace hla_rti_cpp_tck::detail
