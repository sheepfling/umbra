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

void verifyFloatingBasicDataElementExternalMemoryContract() {
  verifyExternalMemorySynchronization<rti::HLAfloat32BE>(
      "HLAfloat32BE",
      1.0F,
      -2.5F,
      {static_cast<rti::Octet>(0x7eU),
       static_cast<rti::Octet>(0xc0U),
       static_cast<rti::Octet>(0x20U),
       static_cast<rti::Octet>(0x00U),
       static_cast<rti::Octet>(0x00U)},
      5.5F,
      12.5F);
  verifyExternalMemorySynchronization<rti::HLAfloat32LE>(
      "HLAfloat32LE",
      1.0F,
      -2.5F,
      {static_cast<rti::Octet>(0x7eU),
       static_cast<rti::Octet>(0x00U),
       static_cast<rti::Octet>(0x00U),
       static_cast<rti::Octet>(0x20U),
       static_cast<rti::Octet>(0xc0U)},
      5.5F,
      12.5F);
  verifyExternalMemorySynchronization<rti::HLAfloat64BE>(
      "HLAfloat64BE",
      1.0,
      -2.5,
      {static_cast<rti::Octet>(0x7eU),
       static_cast<rti::Octet>(0xc0U),
       static_cast<rti::Octet>(0x04U),
       static_cast<rti::Octet>(0x00U),
       static_cast<rti::Octet>(0x00U),
       static_cast<rti::Octet>(0x00U),
       static_cast<rti::Octet>(0x00U),
       static_cast<rti::Octet>(0x00U),
       static_cast<rti::Octet>(0x00U)},
      5.5,
      12.5);
  verifyExternalMemorySynchronization<rti::HLAfloat64LE>(
      "HLAfloat64LE",
      1.0,
      -2.5,
      {static_cast<rti::Octet>(0x7eU),
       static_cast<rti::Octet>(0x00U),
       static_cast<rti::Octet>(0x00U),
       static_cast<rti::Octet>(0x00U),
       static_cast<rti::Octet>(0x00U),
       static_cast<rti::Octet>(0x00U),
       static_cast<rti::Octet>(0x00U),
       static_cast<rti::Octet>(0x04U),
       static_cast<rti::Octet>(0xc0U)},
      5.5,
      12.5);
}

}  // namespace hla_rti_cpp_tck::detail
