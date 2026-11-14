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

void verifySixteenBitIntegerExternalMemoryContract() {
  verifyExternalMemorySynchronization<rti::HLAinteger16BE>(
      "HLAinteger16BE",
      static_cast<rti::Integer16>(42),
      static_cast<rti::Integer16>(-1234),
      {static_cast<rti::Octet>(0x7eU),
       static_cast<rti::Octet>(0xfbU),
       static_cast<rti::Octet>(0x2eU)},
      static_cast<rti::Integer16>(2345),
      static_cast<rti::Integer16>(-23456));
  verifyExternalMemorySynchronization<rti::HLAinteger16LE>(
      "HLAinteger16LE",
      static_cast<rti::Integer16>(42),
      static_cast<rti::Integer16>(-1234),
      {static_cast<rti::Octet>(0x7eU),
       static_cast<rti::Octet>(0x2eU),
       static_cast<rti::Octet>(0xfbU)},
      static_cast<rti::Integer16>(2345),
      static_cast<rti::Integer16>(-23456));
  verifyExternalMemorySynchronization<rti::HLAunsignedInteger16BE>(
      "HLAunsignedInteger16BE",
      static_cast<rti::UnsignedInteger16>(0x1234U),
      static_cast<rti::UnsignedInteger16>(0xa5c3U),
      {static_cast<rti::Octet>(0x7eU),
       static_cast<rti::Octet>(0xa5U),
       static_cast<rti::Octet>(0xc3U)},
      static_cast<rti::UnsignedInteger16>(0x5a6bU),
      static_cast<rti::UnsignedInteger16>(0xc3d4U));
  verifyExternalMemorySynchronization<rti::HLAunsignedInteger16LE>(
      "HLAunsignedInteger16LE",
      static_cast<rti::UnsignedInteger16>(0x1234U),
      static_cast<rti::UnsignedInteger16>(0xa5c3U),
      {static_cast<rti::Octet>(0x7eU),
       static_cast<rti::Octet>(0xc3U),
       static_cast<rti::Octet>(0xa5U)},
      static_cast<rti::UnsignedInteger16>(0x5a6bU),
      static_cast<rti::UnsignedInteger16>(0xc3d4U));
}

}  // namespace hla_rti_cpp_tck::detail
