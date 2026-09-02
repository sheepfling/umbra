#pragma once

#include "internal/fom/fom_validation.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace umbra::detail {

// This is deliberately a neutral description of a data-type wire shape. It
// is not an XML encoding-name enum: RPR-specific source labels are mapped to
// these values only by the explicit RPR compatibility profile.
enum class FomWireEncodingKind {
  unspecified,
  fixed_width,
  fixed_array,
  variable_array,
  null_terminated_array,
  lengthless_array,
  alignment_padding_array,
  fixed_record,
  variant_record,
  extendable_variant_record,
  unrecognized,
};

// Catalog recognition and byte-level codec support are separate capabilities.
// metadata_only means that the source shape is understood well enough for
// validation/catalog/object lookup, but no runtime codec claim is made. This
// neutral header intentionally does not include the RPR byte-codec
// implementation; the RPR validation backend owns that opt-in promotion.
enum class FomWireCodecStatus {
  not_required,
  available,
  metadata_only,
  unsupported,
};

enum class FomByteOrder {
  unspecified,
  big,
  little,
};

struct FomWireEncodingDescriptor {
  // Exact source spelling, retained for diagnostics, audit, and future
  // round-trip/reporting surfaces.
  std::string sourceLabel;
  FomWireEncodingKind kind = FomWireEncodingKind::unspecified;
  FomWireCodecStatus codecStatus = FomWireCodecStatus::not_required;

  // Basic-data metadata. A zero width means that the source did not provide
  // a safely parsed width.
  std::uint32_t sizeBits = 0;
  FomByteOrder byteOrder = FomByteOrder::unspecified;

  // Neutral structural properties used by a future codec. These fields do
  // not themselves authorize encoding or decoding.
  std::uint32_t alignmentOctets = 0;
  bool elementCountInPayload = false;
  bool sentinelTerminated = false;
  bool extentSuppliedExternally = false;
  bool extensionLengthInPayload = false;
};

inline FomWireEncodingDescriptor normalizeFomWireEncoding(
    std::string_view sourceLabel,
    FomSourceCompatibility compatibility) {
  FomWireEncodingDescriptor result;
  result.sourceLabel = std::string(sourceLabel);
  if (sourceLabel.empty()) {
    return result;
  }

  if (sourceLabel == "HLAfixedArray") {
    result.kind = FomWireEncodingKind::fixed_array;
    result.codecStatus = FomWireCodecStatus::available;
    return result;
  }
  if (sourceLabel == "HLAvariableArray") {
    result.kind = FomWireEncodingKind::variable_array;
    result.codecStatus = FomWireCodecStatus::available;
    result.elementCountInPayload = true;
    return result;
  }
  if (sourceLabel == "HLAfixedRecord") {
    result.kind = FomWireEncodingKind::fixed_record;
    result.codecStatus = FomWireCodecStatus::available;
    return result;
  }
  if (sourceLabel == "HLAvariantRecord") {
    result.kind = FomWireEncodingKind::variant_record;
    result.codecStatus = FomWireCodecStatus::available;
    return result;
  }
  if (sourceLabel == "HLAextendableVariantRecord") {
    result.kind = FomWireEncodingKind::extendable_variant_record;
    result.codecStatus = FomWireCodecStatus::available;
    result.extensionLengthInPayload = true;
    return result;
  }

  // These labels are a source-dialect adapter, not additions to the 2025
  // standard encoding vocabulary. Keep this branch explicitly gated so a
  // strict 2025 catalog cannot accidentally acquire RPR semantics.
  if (compatibility != FomSourceCompatibility::rpr_2010) {
    result.kind = FomWireEncodingKind::unrecognized;
    result.codecStatus = FomWireCodecStatus::unsupported;
    return result;
  }
  if (sourceLabel == "RPRnullTerminatedArray") {
    result.kind = FomWireEncodingKind::null_terminated_array;
    result.codecStatus = FomWireCodecStatus::metadata_only;
    result.sentinelTerminated = true;
    return result;
  }
  if (sourceLabel == "RPRlengthlessArray") {
    result.kind = FomWireEncodingKind::lengthless_array;
    result.codecStatus = FomWireCodecStatus::metadata_only;
    result.extentSuppliedExternally = true;
    return result;
  }
  if (sourceLabel == "RPRpaddingTo32Array") {
    result.kind = FomWireEncodingKind::alignment_padding_array;
    result.codecStatus = FomWireCodecStatus::metadata_only;
    result.alignmentOctets = 4;
    result.extentSuppliedExternally = true;
    return result;
  }
  if (sourceLabel == "RPRpaddingTo64Array") {
    result.kind = FomWireEncodingKind::alignment_padding_array;
    result.codecStatus = FomWireCodecStatus::metadata_only;
    result.alignmentOctets = 8;
    result.extentSuppliedExternally = true;
    return result;
  }
  if (sourceLabel == "RPRextendedVariantRecord") {
    result.kind = FomWireEncodingKind::extendable_variant_record;
    result.codecStatus = FomWireCodecStatus::metadata_only;
    result.extensionLengthInPayload = true;
    return result;
  }

  result.kind = FomWireEncodingKind::unrecognized;
  result.codecStatus = FomWireCodecStatus::unsupported;
  return result;
}

inline FomWireEncodingDescriptor makeBasicFomWireEncoding(
    std::string_view sourceLabel,
    std::uint32_t sizeBits,
    FomByteOrder byteOrder) {
  FomWireEncodingDescriptor result;
  result.sourceLabel = std::string(sourceLabel);
  result.kind = FomWireEncodingKind::fixed_width;
  result.codecStatus = FomWireCodecStatus::metadata_only;
  result.sizeBits = sizeBits;
  result.byteOrder = byteOrder;
  return result;
}

}  // namespace umbra::detail
