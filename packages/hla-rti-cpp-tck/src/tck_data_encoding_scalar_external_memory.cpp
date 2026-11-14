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

void verifyScalarBasicDataElementExternalMemoryContract() {
  verifyExternalMemorySynchronization<rti::HLAboolean>(
      "HLAboolean",
      false,
      true,
      {static_cast<rti::Octet>(0x7eU),
       static_cast<rti::Octet>(0x00U),
       static_cast<rti::Octet>(0x00U),
       static_cast<rti::Octet>(0x00U),
       static_cast<rti::Octet>(0x01U)},
      false,
      true);
  verifyExternalMemorySynchronization<rti::HLAoctet>(
      "HLAoctet",
      static_cast<rti::Octet>(0x11U),
      static_cast<rti::Octet>(0xa5U),
      {static_cast<rti::Octet>(0x7eU),
       static_cast<rti::Octet>(0xa5U)},
      static_cast<rti::Octet>(0x5aU),
      static_cast<rti::Octet>(0xc3U));
  verifyExternalMemorySynchronization<rti::HLAbyte>(
      "HLAbyte",
      static_cast<rti::Octet>(0x22U),
      static_cast<rti::Octet>(0x5cU),
      {static_cast<rti::Octet>(0x7eU),
       static_cast<rti::Octet>(0x5cU)},
      static_cast<rti::Octet>(0xe1U),
      static_cast<rti::Octet>(0x93U));
}

}  // namespace hla_rti_cpp_tck::detail
