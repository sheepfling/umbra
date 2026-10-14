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
void verifyBasicDataElementsContract();
void verifyCompositeDataElementsContract();

}  // namespace hla_rti_cpp_tck::detail
