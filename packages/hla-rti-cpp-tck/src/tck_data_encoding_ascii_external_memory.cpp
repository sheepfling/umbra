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

void verifyAsciiBasicDataElementExternalMemoryContract() {
  verifyExternalMemorySynchronization<rti::HLAASCIIchar>(
      "HLAASCIIchar",
      'A',
      'Z',
      {static_cast<rti::Octet>(0x7eU),
       static_cast<rti::Octet>('Z')},
      'B',
      'C');
  verifyExternalMemorySynchronization<rti::HLAASCIIstring>(
      "HLAASCIIstring",
      std::string{"seed", 4U},
      std::string{"A\0Z!", 4U},
      {static_cast<rti::Octet>(0x7eU),
       static_cast<rti::Octet>(0U),
       static_cast<rti::Octet>(0U),
       static_cast<rti::Octet>(0U),
       static_cast<rti::Octet>(4U),
       static_cast<rti::Octet>('A'),
       static_cast<rti::Octet>(0U),
       static_cast<rti::Octet>('Z'),
       static_cast<rti::Octet>('!')},
      std::string{"set", 3U},
      std::string{"ok\0!", 4U});
}

}  // namespace hla_rti_cpp_tck::detail
