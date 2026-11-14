#include "tck_data_encoding.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

template <typename Element, typename Value>
void verifyExternalMemorySynchronization(
    std::string const& helperName,
    Value initialValue,
    Value callerUpdatedValue,
    std::vector<rti::Octet> expectedEncoding,
    Value setValue,
    Value decodedValue) {
  Value callerOwnedValue = initialValue;
  Element element{&callerOwnedValue};
  Element copiedElement{element};
  auto clonedElementBase = element.clone();
  auto* clonedElement = dynamic_cast<Element*>(clonedElementBase.get());
  require(
      clonedElement != nullptr,
      helperName + " clone did not preserve its concrete helper type");

  callerOwnedValue = callerUpdatedValue;
  std::vector<rti::Octet> actualEncoding{
      static_cast<rti::Octet>(0x7eU)};
  element.encodeInto(actualEncoding);
  require(
      element.get() == callerUpdatedValue &&
          actualEncoding == expectedEncoding &&
          copiedElement.get() == initialValue &&
          clonedElement->get() == initialValue,
      helperName + " did not observe caller-owned memory changes");

  element.set(setValue);
  require(
      callerOwnedValue == setValue,
      helperName + " set did not update caller-owned memory");

  Element decodedValueElement{decodedValue};
  element.decode(decodedValueElement.encode());
  require(
      callerOwnedValue == decodedValue && element.get() == decodedValue &&
          copiedElement.get() == initialValue &&
          clonedElement->get() == initialValue,
      helperName + " decode did not update caller-owned memory");

  copiedElement.set(setValue);
  clonedElement->set(callerUpdatedValue);
  require(
      callerOwnedValue == decodedValue && element.get() == decodedValue &&
          copiedElement.get() == setValue &&
          clonedElement->get() == callerUpdatedValue,
      helperName + " copy or clone retained external-memory coupling");
}

}  // namespace

void verifyUnicodeBasicDataElementExternalMemoryContract() {
  verifyExternalMemorySynchronization<rti::HLAunicodeChar>(
      "HLAunicodeChar",
      static_cast<wchar_t>(0x0041U),
      static_cast<wchar_t>(0x03a9U),
      {static_cast<rti::Octet>(0x7eU),
       static_cast<rti::Octet>(0x03U),
       static_cast<rti::Octet>(0xa9U)},
      static_cast<wchar_t>(0x03bbU),
      static_cast<wchar_t>(0x20acU));
  verifyExternalMemorySynchronization<rti::HLAunicodeString>(
      "HLAunicodeString",
      std::wstring{L"seed"},
      std::wstring{L"A\U0001f600"},
      {static_cast<rti::Octet>(0x7eU),
       static_cast<rti::Octet>(0x00U),
       static_cast<rti::Octet>(0x00U),
       static_cast<rti::Octet>(0x00U),
       static_cast<rti::Octet>(0x03U),
       static_cast<rti::Octet>(0x00U),
       static_cast<rti::Octet>(0x41U),
       static_cast<rti::Octet>(0xd8U),
       static_cast<rti::Octet>(0x3dU),
       static_cast<rti::Octet>(0xdeU),
       static_cast<rti::Octet>(0x00U)},
      std::wstring{L"set"},
      std::wstring{L"Z\U0001f642"});
}

}  // namespace hla_rti_cpp_tck::detail
