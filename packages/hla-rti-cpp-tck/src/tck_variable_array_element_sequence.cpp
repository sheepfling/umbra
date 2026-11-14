#include "tck_data_encoding.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioVariableArrayElementSequence(
    Options const&, rti::CallbackModel) {
  rti::HLAvariableArray values{rti::HLAinteger16BE{}};
  values.addElement(rti::HLAinteger16BE{0x1234});
  values.addElement(rti::HLAinteger16BE{0x5678});

  // The signed four-octet count starts at offset zero, followed by exactly two
  // encoded HLAinteger16BE elements in array order.
  std::vector<rti::Octet> const expected{
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(2U),
      static_cast<rti::Octet>(0x12U),
      static_cast<rti::Octet>(0x34U),
      static_cast<rti::Octet>(0x56U),
      static_cast<rti::Octet>(0x78U)};
  std::vector<rti::Octet> actual;
  values.encodeInto(actual);

  require(
      actual == expected,
      "HLAvariableArray did not place its count at offset zero before its elements");
  require(
      values.size() == 2U && values.getEncodedLength() == expected.size(),
      "HLAvariableArray count or encoded length did not match its elements");
}

}  // namespace hla_rti_cpp_tck::detail
