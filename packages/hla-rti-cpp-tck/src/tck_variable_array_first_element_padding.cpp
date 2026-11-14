#include "tck_data_encoding.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioVariableArrayFirstElementPadding(
    Options const&, rti::CallbackModel) {
  rti::HLAvariableArray values{rti::HLAfloat64BE{}};
  values.addElement(rti::HLAfloat64BE{-2.5});

  // The four-octet HLAinteger32BE count ends at offset four. Four zero octets
  // then align the first HLAfloat64BE element to the array's eight-octet
  // boundary, as required for this selected element type.
  std::vector<rti::Octet> const expected{
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(1U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0xc0U),
      static_cast<rti::Octet>(0x04U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U)};
  std::vector<rti::Octet> actual;
  values.encodeInto(actual);

  require(
      actual == expected,
      "HLAvariableArray did not align its first element after the count component");
  require(
      values.getOctetBoundary() == 8U &&
          values.getEncodedLength() == expected.size(),
      "HLAvariableArray first-element alignment metadata was incorrect");
}

}  // namespace hla_rti_cpp_tck::detail
