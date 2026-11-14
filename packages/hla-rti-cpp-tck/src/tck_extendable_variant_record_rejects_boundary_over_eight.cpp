#include "tck_data_encoding.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioExtendableVariantRecordRejectsBoundaryOverEight(
    Options const&, rti::CallbackModel) {
  rti::HLAextendableVariantRecord record{rti::HLAoctet{}};
  Boundary16Element alternative;
  bool rejected = false;
  try {
    record.addVariant(rti::HLAoctet{static_cast<rti::Octet>(1U)}, alternative);
  } catch (rti::EncoderException const&) {
    rejected = true;
  }

  require(
      rejected,
      "HLAextendableVariantRecord accepted an alternative with a 16-octet boundary");
}

}  // namespace hla_rti_cpp_tck::detail
