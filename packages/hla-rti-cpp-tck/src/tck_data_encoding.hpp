#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

class Boundary16Element final : public rti::DataElement {
 public:
  std::unique_ptr<rti::DataElement> clone() const override;
  rti::VariableLengthData encode() const override;
  void encode(rti::VariableLengthData& value) const override;
  void encodeInto(std::vector<rti::Octet>& bytes) const override;
  Boundary16Element& decode(
      rti::VariableLengthData const& value) override;
  std::size_t decodeFrom(
      std::vector<rti::Octet> const& bytes,
      std::size_t index) override;
  std::size_t getEncodedLength() const override;
  unsigned int getOctetBoundary() const override;
};

void verifyVariableLengthDataContract();
void verifyStandardDataElementContract();
void verifyBasicDataElementsIntegerContract();
void verifyBasicDataElementsContract();
void scenarioBasicDataElementEncodeDecodeContract(
    Options const&, rti::CallbackModel);
void scenarioBasicDataElementValueOperationsContract(
    Options const&, rti::CallbackModel);
void scenarioComplexDataElementConstituentAccess(
    Options const&, rti::CallbackModel);
void verifyFloatingBasicDataElementExternalMemoryContract();
void verifyScalarBasicDataElementExternalMemoryContract();
void verifyAsciiBasicDataElementExternalMemoryContract();
void verifyUnicodeBasicDataElementExternalMemoryContract();
void verifySixteenBitIntegerExternalMemoryContract();
void verifyArrayCopyCloneExternalMemoryContract();
void verifyRecordCopyCloneExternalMemoryContract();
void verifyExtendableVariantCopyCloneContract();
void verifyExtendableVariantRawBufferContract();
void verifyCompositeDataElementsContract();
void scenarioEncoderRoundTrip(Options const&, rti::CallbackModel);
void scenarioFixedRecordAlignment(Options const&, rti::CallbackModel);
void scenarioVariantRecordDiscriminantPadding(
    Options const&, rti::CallbackModel);
void scenarioVariantRecordUnmappedDiscriminant(
    Options const&, rti::CallbackModel);
void scenarioExtendableVariantRecordLengthPadding(
    Options const&, rti::CallbackModel);
void scenarioExtendableVariantRecordAlternativePadding(
    Options const&, rti::CallbackModel);
void scenarioExtendableVariantRecordUnmappedDiscriminant(
    Options const&, rti::CallbackModel);
void scenarioExtendableVariantRecordRejectsBoundaryOverEight(
    Options const&, rti::CallbackModel);
void scenarioFixedArrayElementSequence(
    Options const&, rti::CallbackModel);
void scenarioVariableArrayElementSequence(
    Options const&, rti::CallbackModel);
void scenarioVariableArrayFirstElementPadding(
    Options const&, rti::CallbackModel);
void scenarioVariableArrayInterElementPadding(
    Options const&, rti::CallbackModel);

}  // namespace hla_rti_cpp_tck::detail
