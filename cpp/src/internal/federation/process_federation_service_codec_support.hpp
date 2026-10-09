#pragma once

#include "internal/federation/process_federation_service_protocol.hpp"
#include "internal/federation/process_federation_service_helpers.hpp"

#include "internal/encoding/byte_order.hpp"
#include "internal/encoding/variable_length_data_2025.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/handles/dimension_handle.hpp"
#include "internal/handles/federate_handle.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/object_class_handle.hpp"
#include "internal/handles/region_handle.hpp"
#include "internal/handles/transportation_type_handle.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#include "internal/runtime/utf8_string.hpp"
#include "internal/time/federation_time_bounds.hpp"
#include "internal/time/federation_time_grant_policy.hpp"

#include <RTI/Exception.h>
#include <RTI/VariableLengthData.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/time/HLAlogicalTimeFactoryFactory.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cmath>
#include <limits>
#include <memory>
#include <iterator>
#include <set>
#include <utility>

#include "internal/federation/process_federation_service_payload_helpers.hpp"
namespace umbra::detail::process_federation_service_codec_support {
using process_federation_payload::parameterVector;

inline constexpr std::uint8_t kRtiOwnedMomAttributeEventMarker = 0xA5U;
inline constexpr std::uint8_t kRtiOwnedMomDiscoveryEventMarker = 0xA6U;
inline constexpr std::uint8_t kRtiOwnedMomInteractionEventMarker = 0xA7U;
inline constexpr std::uint8_t kRtiOwnedMomRemovalEventMarker = 0xA8U;
class PayloadWriter final {
 public:
  void unsigned8(std::uint8_t value) { bytes_.push_back(value); }

  void raw(std::span<std::uint8_t const> value) {
    bytes_.insert(bytes_.end(), value.begin(), value.end());
  }

  void unsigned32(std::uint32_t value) {
    appendUnsigned(bytes_, value, ByteOrder::big);
  }

  void unsigned64(std::uint64_t value) {
    appendUnsigned(bytes_, value, ByteOrder::big);
  }

  void real64(double value) {
    static_assert(sizeof(double) == sizeof(std::uint64_t));
    unsigned64(std::bit_cast<std::uint64_t>(value));
  }

  void wideString(std::wstring const& value) {
    if (value.size() > std::numeric_limits<std::uint32_t>::max()) {
      throw ProcessFederationServiceProtocolError(
          "A process federation service string is too long.");
    }
    unsigned32(static_cast<std::uint32_t>(value.size()));
    for (wchar_t character : value) {
      unsigned32(static_cast<std::uint32_t>(character));
    }
  }

  void string(std::string const& value) {
    if (value.size() > std::numeric_limits<std::uint32_t>::max()) {
      throw ProcessFederationServiceProtocolError(
          "A process federation service byte string is too long.");
    }
    unsigned32(static_cast<std::uint32_t>(value.size()));
    bytes_.insert(bytes_.end(), value.begin(), value.end());
  }

  void bytes(std::span<std::uint8_t const> value) {
    if (value.size() > std::numeric_limits<std::uint32_t>::max()) {
      throw ProcessFederationServiceProtocolError(
          "A process federation service byte sequence is too long.");
    }
    unsigned32(static_cast<std::uint32_t>(value.size()));
    bytes_.insert(bytes_.end(), value.begin(), value.end());
  }

  void unsigned64Vector(std::vector<std::uint64_t> const& values) {
    if (values.size() > std::numeric_limits<std::uint32_t>::max()) {
      throw ProcessFederationServiceProtocolError(
          "A process federation service handle vector is too long.");
    }
    unsigned32(static_cast<std::uint32_t>(values.size()));
    for (std::uint64_t value : values) {
      unsigned64(value);
    }
  }

  void wideStringVector(std::vector<std::wstring> const& values) {
    if (values.size() > std::numeric_limits<std::uint32_t>::max()) {
      throw ProcessFederationServiceProtocolError(
          "A process federation service string vector is too long.");
    }
    unsigned32(static_cast<std::uint32_t>(values.size()));
    for (auto const& value : values) {
      wideString(value);
    }
  }

  [[nodiscard]] std::vector<std::uint8_t> finish() && {
    if (bytes_.size() > kTransportMaximumPayloadBytes) {
      throw ProcessFederationServiceProtocolError(
          "A process federation service payload exceeds the transport maximum.");
    }
    return std::move(bytes_);
  }

 private:
  std::vector<std::uint8_t> bytes_;
};

class PayloadReader final {
 public:
  explicit PayloadReader(std::span<std::uint8_t const> bytes) : bytes_(bytes) {
    if (bytes.size() > kTransportMaximumPayloadBytes) {
      throw ProcessFederationServiceProtocolError(
          "A process federation service payload exceeds the transport maximum.");
    }
  }

  [[nodiscard]] std::uint8_t unsigned8() {
    require(sizeof(std::uint8_t));
    return bytes_[offset_++];
  }

  [[nodiscard]] std::uint32_t unsigned32() {
    require(sizeof(std::uint32_t));
    std::uint32_t value = 0U;
    static_cast<void>(readUnsigned(
        static_cast<void const *>(bytes_.data() + offset_),
        sizeof(value),
        ByteOrder::big,
        value));
    offset_ += sizeof(std::uint32_t);
    return value;
  }

  [[nodiscard]] std::uint64_t unsigned64() {
    require(sizeof(std::uint64_t));
    std::uint64_t value = 0U;
    static_cast<void>(readUnsigned(
        static_cast<void const *>(bytes_.data() + offset_),
        sizeof(value),
        ByteOrder::big,
        value));
    offset_ += sizeof(std::uint64_t);
    return value;
  }

  [[nodiscard]] double real64() {
    static_assert(sizeof(double) == sizeof(std::uint64_t));
    return std::bit_cast<double>(unsigned64());
  }

  [[nodiscard]] std::wstring wideString() {
    auto const count = checkedCount(unsigned32(), sizeof(std::uint32_t));
    std::wstring result;
    result.reserve(count);
    for (std::size_t index = 0U; index < count; ++index) {
      auto const codeUnit = unsigned32();
      if (codeUnit > static_cast<std::uint32_t>(std::numeric_limits<wchar_t>::max())) {
        throw ProcessFederationServiceProtocolError(
            "A process federation service string contains an invalid code unit.");
      }
      result.push_back(static_cast<wchar_t>(codeUnit));
    }
    return result;
  }

  [[nodiscard]] std::string string() {
    auto const count = checkedCount(unsigned32(), sizeof(std::uint8_t));
    auto const sequence = take(count);
    return std::string(
        reinterpret_cast<char const*>(sequence.data()), sequence.size());
  }

  [[nodiscard]] std::vector<std::uint8_t> bytes() {
    auto const count = checkedCount(unsigned32(), sizeof(std::uint8_t));
    auto const sequence = take(count);
    return {sequence.begin(), sequence.end()};
  }

  [[nodiscard]] std::vector<std::uint64_t> unsigned64Vector() {
    auto const count = checkedCount(unsigned32(), sizeof(std::uint64_t));
    std::vector<std::uint64_t> result;
    result.reserve(count);
    for (std::size_t index = 0U; index < count; ++index) {
      result.push_back(unsigned64());
    }
    return result;
  }

  [[nodiscard]] std::vector<std::wstring> wideStringVector() {
    // Every element starts with a four-byte code-unit count. This lower
    // bound keeps a malformed count from reserving an unbounded vector before
    // the individual strings are checked.
    auto const count = checkedCount(unsigned32(), sizeof(std::uint32_t));
    std::vector<std::wstring> result;
    result.reserve(count);
    for (std::size_t index = 0U; index < count; ++index) {
      result.push_back(wideString());
    }
    return result;
  }

  [[nodiscard]] std::size_t count(std::size_t minimumElementSize) {
    return checkedCount(unsigned32(), minimumElementSize);
  }

  void finish() const {
    if (offset_ != bytes_.size()) {
      throw ProcessFederationServiceProtocolError(
          "A process federation service payload has trailing bytes.");
    }
  }

  [[nodiscard]] std::size_t remaining() const noexcept {
    return bytes_.size() - offset_;
  }

 private:
  void require(std::size_t count) const {
    if (count > bytes_.size() - offset_) {
      throw ProcessFederationServiceProtocolError(
          "A process federation service payload is truncated.");
    }
  }

  [[nodiscard]] std::size_t checkedCount(
      std::uint32_t count,
      std::size_t elementSize) const {
    auto const converted = static_cast<std::size_t>(count);
    if (elementSize != 0U && converted > (bytes_.size() - offset_) / elementSize) {
      throw ProcessFederationServiceProtocolError(
          "A process federation service vector exceeds its payload.");
    }
    return converted;
  }

  [[nodiscard]] std::span<std::uint8_t const> take(std::size_t count) {
    require(count);
    auto const result = bytes_.subspan(offset_, count);
    offset_ += count;
    return result;
  }

  std::span<std::uint8_t const> bytes_;
  std::size_t offset_ = 0U;
};

inline void requireNonzero(std::uint64_t value, char const* message) {
  if (value == 0U) {
    throw ProcessFederationServiceProtocolError(message);
  }
}
[[nodiscard]] inline bool validResignAction(rti1516_2025::ResignAction action) noexcept {
  switch (action) {
    case rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES:
    case rti1516_2025::DELETE_OBJECTS:
    case rti1516_2025::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS:
    case rti1516_2025::DELETE_OBJECTS_THEN_DIVEST:
    case rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST:
    case rti1516_2025::NO_ACTION:
      return true;
  }
  return false;
}
[[nodiscard]] inline bool validOrderType(rti1516_2025::OrderType orderType) noexcept {
  return orderType == rti1516_2025::RECEIVE ||
      orderType == rti1516_2025::TIMESTAMP;
}
inline constexpr std::uint8_t kLastRegionServiceStatus = static_cast<std::uint8_t>(
    RegionServiceStatus::inconsistent_catalog);

inline constexpr std::uint8_t kLastAttributeValueUpdateClassRequestStatus =
    static_cast<std::uint8_t>(
        AttributeValueUpdateClassRequestStatus::inconsistent_catalog);

inline void writeAttributeValueUpdateClassRequestStatus(
    PayloadWriter& writer,
    AttributeValueUpdateClassRequestStatus status) {
  auto const encoded = static_cast<std::uint8_t>(status);
  if (encoded > kLastAttributeValueUpdateClassRequestStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process class Request Attribute Value Update result has an invalid status.");
  }
  writer.unsigned8(encoded);
}

[[nodiscard]] inline AttributeValueUpdateClassRequestStatus
readAttributeValueUpdateClassRequestStatus(PayloadReader& reader) {
  auto const encoded = reader.unsigned8();
  if (encoded > kLastAttributeValueUpdateClassRequestStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process class Request Attribute Value Update result has an invalid status.");
  }
  return static_cast<AttributeValueUpdateClassRequestStatus>(encoded);
}

inline constexpr std::uint8_t kLastObjectInstanceRegionAssociationStatus =
    static_cast<std::uint8_t>(
        ObjectInstanceRegionAssociationStatus::inconsistent_catalog);

inline constexpr std::uint8_t kLastAttributeOwnershipCheckStatus = static_cast<std::uint8_t>(
    AttributeOwnershipCheckStatus::inconsistent_catalog);

inline constexpr std::uint8_t kLastAttributeOwnershipQueryStatus = static_cast<std::uint8_t>(
    AttributeOwnershipQueryStatus::inconsistent_catalog);

inline constexpr std::uint8_t kLastAttributeOwnershipAcquisitionIfAvailableStatus =
    static_cast<std::uint8_t>(
        AttributeOwnershipAcquisitionIfAvailableStatus::inconsistent_catalog);

inline constexpr std::uint8_t kLastAttributeOwnershipAcquisitionStatus =
    static_cast<std::uint8_t>(
        AttributeOwnershipAcquisitionStatus::inconsistent_catalog);

inline constexpr std::uint8_t kLastAttributeOwnershipReleaseDeniedStatus =
    static_cast<std::uint8_t>(
        AttributeOwnershipReleaseDeniedStatus::inconsistent_catalog);

inline constexpr std::uint8_t kLastAttributeOwnershipAcquisitionCancellationStatus =
    static_cast<std::uint8_t>(
        AttributeOwnershipAcquisitionCancellationStatus::inconsistent_catalog);

inline constexpr std::uint8_t kLastNegotiatedAttributeOwnershipDivestitureStatus =
    static_cast<std::uint8_t>(
        NegotiatedAttributeOwnershipDivestitureStatus::inconsistent_catalog);

inline constexpr std::uint8_t kLastConfirmDivestitureStatus = static_cast<std::uint8_t>(
    ConfirmDivestitureStatus::inconsistent_catalog);

inline void writeAttributeOwnershipCheckStatus(
    PayloadWriter& writer,
    AttributeOwnershipCheckStatus status) {
  auto const encoded = static_cast<std::uint8_t>(status);
  if (encoded > kLastAttributeOwnershipCheckStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership check result has an invalid status.");
  }
  writer.unsigned8(encoded);
}

[[nodiscard]] inline AttributeOwnershipCheckStatus readAttributeOwnershipCheckStatus(
    PayloadReader& reader) {
  auto const encoded = reader.unsigned8();
  if (encoded > kLastAttributeOwnershipCheckStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership check result has an invalid status.");
  }
  return static_cast<AttributeOwnershipCheckStatus>(encoded);
}

inline void writeAttributeOwnershipQueryStatus(
    PayloadWriter& writer,
    AttributeOwnershipQueryStatus status) {
  auto const encoded = static_cast<std::uint8_t>(status);
  if (encoded > kLastAttributeOwnershipQueryStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership query result has an invalid status.");
  }
  writer.unsigned8(encoded);
}

[[nodiscard]] inline AttributeOwnershipQueryStatus readAttributeOwnershipQueryStatus(
    PayloadReader& reader) {
  auto const encoded = reader.unsigned8();
  if (encoded > kLastAttributeOwnershipQueryStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership query result has an invalid status.");
  }
  return static_cast<AttributeOwnershipQueryStatus>(encoded);
}

inline void writeAttributeOwnershipAcquisitionIfAvailableStatus(
    PayloadWriter& writer,
    AttributeOwnershipAcquisitionIfAvailableStatus status) {
  auto const encoded = static_cast<std::uint8_t>(status);
  if (encoded > kLastAttributeOwnershipAcquisitionIfAvailableStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership acquisition-if-available result has an invalid status.");
  }
  writer.unsigned8(encoded);
}

[[nodiscard]] inline AttributeOwnershipAcquisitionIfAvailableStatus
readAttributeOwnershipAcquisitionIfAvailableStatus(PayloadReader& reader) {
  auto const encoded = reader.unsigned8();
  if (encoded > kLastAttributeOwnershipAcquisitionIfAvailableStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership acquisition-if-available result has an invalid status.");
  }
  return static_cast<AttributeOwnershipAcquisitionIfAvailableStatus>(encoded);
}

inline void writeAttributeOwnershipAcquisitionStatus(
    PayloadWriter& writer,
    AttributeOwnershipAcquisitionStatus status) {
  auto const encoded = static_cast<std::uint8_t>(status);
  if (encoded > kLastAttributeOwnershipAcquisitionStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership acquisition result has an invalid status.");
  }
  writer.unsigned8(encoded);
}

[[nodiscard]] inline AttributeOwnershipAcquisitionStatus
readAttributeOwnershipAcquisitionStatus(PayloadReader& reader) {
  auto const encoded = reader.unsigned8();
  if (encoded > kLastAttributeOwnershipAcquisitionStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership acquisition result has an invalid status.");
  }
  return static_cast<AttributeOwnershipAcquisitionStatus>(encoded);
}

inline void writeAttributeOwnershipReleaseDeniedStatus(
    PayloadWriter& writer,
    AttributeOwnershipReleaseDeniedStatus status) {
  auto const encoded = static_cast<std::uint8_t>(status);
  if (encoded > kLastAttributeOwnershipReleaseDeniedStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership release-denied result has an invalid status.");
  }
  writer.unsigned8(encoded);
}

[[nodiscard]] inline AttributeOwnershipReleaseDeniedStatus
readAttributeOwnershipReleaseDeniedStatus(PayloadReader& reader) {
  auto const encoded = reader.unsigned8();
  if (encoded > kLastAttributeOwnershipReleaseDeniedStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership release-denied result has an invalid status.");
  }
  return static_cast<AttributeOwnershipReleaseDeniedStatus>(encoded);
}

inline void writeAttributeOwnershipAcquisitionCancellationStatus(
    PayloadWriter& writer,
    AttributeOwnershipAcquisitionCancellationStatus status) {
  auto const encoded = static_cast<std::uint8_t>(status);
  if (encoded > kLastAttributeOwnershipAcquisitionCancellationStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership acquisition-cancellation result has an invalid status.");
  }
  writer.unsigned8(encoded);
}

[[nodiscard]] inline AttributeOwnershipAcquisitionCancellationStatus
readAttributeOwnershipAcquisitionCancellationStatus(PayloadReader& reader) {
  auto const encoded = reader.unsigned8();
  if (encoded > kLastAttributeOwnershipAcquisitionCancellationStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership acquisition-cancellation result has an invalid status.");
  }
  return static_cast<AttributeOwnershipAcquisitionCancellationStatus>(encoded);
}

inline void writeNegotiatedAttributeOwnershipDivestitureStatus(
    PayloadWriter& writer,
    NegotiatedAttributeOwnershipDivestitureStatus status) {
  auto const encoded = static_cast<std::uint8_t>(status);
  if (encoded > kLastNegotiatedAttributeOwnershipDivestitureStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process negotiated-divestiture result has an invalid status.");
  }
  writer.unsigned8(encoded);
}

[[nodiscard]] inline NegotiatedAttributeOwnershipDivestitureStatus
readNegotiatedAttributeOwnershipDivestitureStatus(PayloadReader& reader) {
  auto const encoded = reader.unsigned8();
  if (encoded > kLastNegotiatedAttributeOwnershipDivestitureStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process negotiated-divestiture result has an invalid status.");
  }
  return static_cast<NegotiatedAttributeOwnershipDivestitureStatus>(encoded);
}

inline void writeConfirmDivestitureStatus(
    PayloadWriter& writer,
    ConfirmDivestitureStatus status) {
  auto const encoded = static_cast<std::uint8_t>(status);
  if (encoded > kLastConfirmDivestitureStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process Confirm Divestiture result has an invalid status.");
  }
  writer.unsigned8(encoded);
}

[[nodiscard]] inline ConfirmDivestitureStatus readConfirmDivestitureStatus(
    PayloadReader& reader) {
  auto const encoded = reader.unsigned8();
  if (encoded > kLastConfirmDivestitureStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process Confirm Divestiture result has an invalid status.");
  }
  return static_cast<ConfirmDivestitureStatus>(encoded);
}

inline void writeRegionServiceStatus(PayloadWriter& writer, RegionServiceStatus status) {
  auto const encoded = static_cast<std::uint8_t>(status);
  if (encoded > kLastRegionServiceStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process region result has an invalid status.");
  }
  writer.unsigned8(encoded);
}

[[nodiscard]] inline RegionServiceStatus readRegionServiceStatus(PayloadReader& reader) {
  auto const encoded = reader.unsigned8();
  if (encoded > kLastRegionServiceStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process region result has an invalid status.");
  }
  return static_cast<RegionServiceStatus>(encoded);
}

inline void writeObjectInstanceRegionAssociationStatus(
    PayloadWriter& writer,
    ObjectInstanceRegionAssociationStatus status) {
  auto const encoded = static_cast<std::uint8_t>(status);
  if (encoded > kLastObjectInstanceRegionAssociationStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process object regional association result has an invalid status.");
  }
  writer.unsigned8(encoded);
}

[[nodiscard]] inline ObjectInstanceRegionAssociationStatus
readObjectInstanceRegionAssociationStatus(PayloadReader& reader) {
  auto const encoded = reader.unsigned8();
  if (encoded > kLastObjectInstanceRegionAssociationStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process object regional association result has an invalid status.");
  }
  return static_cast<ObjectInstanceRegionAssociationStatus>(encoded);
}

inline void validateHandleVector(
    std::vector<std::uint64_t> const& values,
    char const* message) {
  std::uint64_t previous = 0U;
  for (std::uint64_t value : values) {
    if (value == 0U || (previous != 0U && value <= previous)) {
      throw ProcessFederationServiceProtocolError(message);
    }
    previous = value;
  }
}

inline void writeAttributeRegionMap(
    PayloadWriter& writer,
    std::map<std::uint64_t, std::set<std::uint64_t>> const& values) {
  if (values.size() > std::numeric_limits<std::uint32_t>::max()) {
    throw ProcessFederationServiceProtocolError(
        "A process regional registration attribute map is too large.");
  }
  writer.unsigned32(static_cast<std::uint32_t>(values.size()));
  for (auto const& [attributeHandle, regionHandles] : values) {
    requireNonzero(
        attributeHandle,
        "A process regional registration requires non-zero attribute handles.");
    writer.unsigned64(attributeHandle);
    writer.unsigned64Vector(parameterVector(regionHandles));
  }
}

[[nodiscard]] inline std::map<std::uint64_t, std::set<std::uint64_t>>
readAttributeRegionMap(PayloadReader& reader) {
  std::map<std::uint64_t, std::set<std::uint64_t>> result;
  // A zero-length region vector is meaningful for the 2025 default-region
  // form.  The structural minimum is therefore the attribute identity plus
  // the vector count; unsigned64Vector() performs the exact remaining-byte
  // check for any supplied region identities below.
  auto const count = reader.count(sizeof(std::uint64_t) + sizeof(std::uint32_t));
  for (std::size_t index = 0U; index < count; ++index) {
    auto const attributeHandle = reader.unsigned64();
    requireNonzero(
        attributeHandle,
        "A process regional registration requires non-zero attribute handles.");
    if (result.contains(attributeHandle)) {
      throw ProcessFederationServiceProtocolError(
          "A process regional registration repeats an attribute handle.");
    }
    auto regionVector = reader.unsigned64Vector();
    validateHandleVector(
        regionVector,
        "A process regional registration requires sorted, unique region handles.");
    result.emplace(
        attributeHandle,
        std::set<std::uint64_t>(regionVector.begin(), regionVector.end()));
  }
  return result;
}

inline void writeAttributeUpdateRegionMetadata(
    PayloadWriter& writer,
    ProcessFederationAttributeUpdateEvent const& event) {
  writer.unsigned8(event.sentRegionHandles.has_value() ? 1U : 0U);
  if (event.sentRegionHandles.has_value()) {
    writer.unsigned64Vector(parameterVector(*event.sentRegionHandles));
  }
  writer.unsigned8(event.defaultRegionUsed ? 1U : 0U);
}

inline void readAttributeUpdateRegionMetadata(
    PayloadReader& reader,
    ProcessFederationAttributeUpdateEvent& event) {
  auto const hasSentRegions = reader.unsigned8();
  if (hasSentRegions > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process federation attribute event has an invalid region marker.");
  }
  if (hasSentRegions != 0U) {
    auto regionVector = reader.unsigned64Vector();
    validateHandleVector(
        regionVector,
        "A process federation attribute event requires sorted, unique region handles.");
    event.sentRegionHandles.emplace(regionVector.begin(), regionVector.end());
  }
  auto const defaultRegionUsed = reader.unsigned8();
  if (defaultRegionUsed > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process federation attribute event has an invalid default-region marker.");
  }
  event.defaultRegionUsed = defaultRegionUsed != 0U;
}

inline void writeInteractionRegionMetadata(
    PayloadWriter& writer,
    ProcessFederationInteractionEvent const& event) {
  writer.unsigned8(event.sentRegionHandles.has_value() ? 1U : 0U);
  if (event.sentRegionHandles.has_value()) {
    writer.unsigned64Vector(parameterVector(*event.sentRegionHandles));
  }
  writer.unsigned8(event.defaultRegionUsed ? 1U : 0U);
}

inline void readInteractionRegionMetadata(
    PayloadReader& reader,
    ProcessFederationInteractionEvent& event) {
  if (reader.remaining() == 0U) {
    return;
  }
  auto const hasSentRegions = reader.unsigned8();
  if (hasSentRegions > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process federation interaction event has an invalid region marker.");
  }
  if (hasSentRegions != 0U) {
    auto regionVector = reader.unsigned64Vector();
    validateHandleVector(
        regionVector,
        "A process federation interaction event requires sorted, unique region handles.");
    event.sentRegionHandles.emplace(regionVector.begin(), regionVector.end());
  }
  if (reader.remaining() == 0U) {
    return;
  }
  auto const defaultRegionUsed = reader.unsigned8();
  if (defaultRegionUsed > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process federation interaction event has an invalid default-region marker.");
  }
  event.defaultRegionUsed = defaultRegionUsed != 0U;
}

inline void writeInteractionOrderMetadata(
    PayloadWriter& writer,
    ProcessFederationInteractionEvent const& event) {
  if (!event.sentOrderType && !event.receivedOrderType) {
    return;
  }
  if (!event.sentOrderType || !event.receivedOrderType) {
    throw ProcessFederationServiceProtocolError(
        "A process interaction event must carry both order classifications when present.");
  }
  writer.unsigned8(1U);
  writer.unsigned8(static_cast<std::uint8_t>(*event.sentOrderType));
  writer.unsigned8(static_cast<std::uint8_t>(*event.receivedOrderType));
}

inline void readInteractionOrderMetadata(
    PayloadReader& reader,
    ProcessFederationInteractionEvent& event) {
  if (reader.remaining() == 0U) {
    return;
  }
  auto const marker = reader.unsigned8();
  if (marker == kRtiOwnedMomInteractionEventMarker) {
    event.rtiOwnedMomInteraction = true;
    return;
  }
  if (marker > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process interaction event has an invalid order metadata marker.");
  }
  if (marker == 0U) {
    return;
  }
  auto decodeOrder = [](std::uint8_t encoded, char const* description) {
    if (encoded != static_cast<std::uint8_t>(rti1516_2025::RECEIVE) &&
        encoded != static_cast<std::uint8_t>(rti1516_2025::TIMESTAMP)) {
      throw ProcessFederationServiceProtocolError(description);
    }
    return static_cast<rti1516_2025::OrderType>(encoded);
  };
  event.sentOrderType = decodeOrder(
      reader.unsigned8(),
      "A process interaction event has an invalid sent order classification.");
  event.receivedOrderType = decodeOrder(
      reader.unsigned8(),
      "A process interaction event has an invalid received order classification.");
}

inline void writeOptionalLogicalTime(
    PayloadWriter& writer,
    std::optional<ProcessFederationLogicalTime> const& timestamp) {
  writer.unsigned8(timestamp.has_value() ? 1U : 0U);
  if (!timestamp) {
    return;
  }
  if (timestamp->implementationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation timestamp requires a logical-time implementation name.");
  }
  writer.wideString(timestamp->implementationName);
  writer.bytes(timestamp->encoding);
}

inline std::optional<ProcessFederationLogicalTime> readOptionalLogicalTime(
    PayloadReader& reader) {
  // The first process protocol revision had no timestamp field. Treat an
  // absent trailing field as the ordinary receive-order form so an older
  // private peer remains readable while new payloads carry an explicit
  // presence marker.
  if (reader.remaining() == 0U) {
    return std::nullopt;
  }
  auto const marker = reader.unsigned8();
  if (marker > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process federation timestamp has an invalid presence marker.");
  }
  if (marker == 0U) {
    return std::nullopt;
  }
  ProcessFederationLogicalTime result;
  result.implementationName = reader.wideString();
  result.encoding = reader.bytes();
  if (result.implementationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation timestamp requires a logical-time implementation name.");
  }
  return result;
}

inline void writeOptionalMessageId(PayloadWriter& writer, std::uint64_t messageId) {
  writer.unsigned8(messageId == 0U ? 0U : 1U);
  if (messageId != 0U) {
    writer.unsigned64(messageId);
  }
}

inline std::uint64_t readOptionalMessageId(PayloadReader& reader) {
  // The first process protocol revision had no message identity on object
  // removal events/results. Treat an absent trailing field as the ordinary
  // receive-order form so old private payloads remain readable.
  if (reader.remaining() == 0U) {
    return 0U;
  }
  auto const marker = reader.unsigned8();
  if (marker > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process federation message identity has an invalid presence marker.");
  }
  if (marker == 0U) {
    return 0U;
  }
  auto const messageId = reader.unsigned64();
  requireNonzero(
      messageId,
      "A process federation message identity cannot be zero when present.");
  return messageId;
}


}  // namespace umbra::detail::process_federation_service_codec_support
