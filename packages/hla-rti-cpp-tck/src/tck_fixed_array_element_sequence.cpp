#include "tck_data_encoding.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioFixedArrayElementSequence(Options const&, rti::CallbackModel) {
  rti::HLAfixedRecord prototype;
  prototype.appendElement(rti::HLAinteger32BE{}).appendElement(rti::HLAoctet{});
  rti::HLAfixedArray values{prototype, 2U};

  rti::HLAfixedRecord first;
  first.appendElement(rti::HLAinteger32BE{0x10203040})
      .appendElement(rti::HLAoctet{static_cast<rti::Octet>(0xa5U)});
  rti::HLAfixedRecord second;
  second.appendElement(rti::HLAinteger32BE{-2})
      .appendElement(rti::HLAoctet{static_cast<rti::Octet>(0x5aU)});
  values.set(0U, first);
  values.set(1U, second);

  // The first fixed-record element starts at offset zero. The second element
  // follows it in sequence after its required three-octet alignment padding.
  // The last five-octet element ends at offset 13 with no trailing padding.
  std::vector<rti::Octet> const expected{
      static_cast<rti::Octet>(0x10U),
      static_cast<rti::Octet>(0x20U),
      static_cast<rti::Octet>(0x30U),
      static_cast<rti::Octet>(0x40U),
      static_cast<rti::Octet>(0xa5U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0xffU),
      static_cast<rti::Octet>(0xffU),
      static_cast<rti::Octet>(0xffU),
      static_cast<rti::Octet>(0xfeU),
      static_cast<rti::Octet>(0x5aU)};
  std::vector<rti::Octet> actual;
  values.encodeInto(actual);

  require(
      actual == expected,
      "HLAfixedArray did not encode elements in order starting at offset zero");
  require(
      values.size() == 2U && values.getEncodedLength() == expected.size(),
      "HLAfixedArray size did not include inter-element padding without trailing padding");
}

}  // namespace hla_rti_cpp_tck::detail
