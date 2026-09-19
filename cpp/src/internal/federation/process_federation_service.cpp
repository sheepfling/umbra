#include "internal/federation/process_federation_service.hpp"

#include "internal/runtime/utf8_string.hpp"
#include "internal/time/federation_time_bounds.hpp"
#include "internal/time/federation_time_grant_policy.hpp"

#include <RTI/Exception.h>
#include <RTI/VariableLengthData.h>
#include <RTI/time/HLAlogicalTimeFactoryFactory.h>

#include <algorithm>
#include <array>
#include <limits>
#include <memory>
#include <iterator>
#include <set>
#include <utility>

namespace umbra::detail {
namespace {

class PayloadWriter final {
 public:
  void unsigned8(std::uint8_t value) { bytes_.push_back(value); }

  void raw(std::span<std::uint8_t const> value) {
    bytes_.insert(bytes_.end(), value.begin(), value.end());
  }

  void unsigned32(std::uint32_t value) {
    bytes_.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xffU));
    bytes_.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xffU));
    bytes_.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
    bytes_.push_back(static_cast<std::uint8_t>(value & 0xffU));
  }

  void unsigned64(std::uint64_t value) {
    for (std::size_t shift = 56U; shift != 0U; shift -= 8U) {
      bytes_.push_back(static_cast<std::uint8_t>((value >> shift) & 0xffU));
    }
    bytes_.push_back(static_cast<std::uint8_t>(value & 0xffU));
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
    auto const value = (static_cast<std::uint32_t>(bytes_[offset_]) << 24U) |
        (static_cast<std::uint32_t>(bytes_[offset_ + 1U]) << 16U) |
        (static_cast<std::uint32_t>(bytes_[offset_ + 2U]) << 8U) |
        static_cast<std::uint32_t>(bytes_[offset_ + 3U]);
    offset_ += sizeof(std::uint32_t);
    return value;
  }

  [[nodiscard]] std::uint64_t unsigned64() {
    require(sizeof(std::uint64_t));
    std::uint64_t value = 0U;
    for (std::size_t index = 0U; index < sizeof(std::uint64_t); ++index) {
      value = (value << 8U) | static_cast<std::uint64_t>(bytes_[offset_ + index]);
    }
    offset_ += sizeof(std::uint64_t);
    return value;
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

void requireNonzero(std::uint64_t value, char const* message) {
  if (value == 0U) {
    throw ProcessFederationServiceProtocolError(message);
  }
}

[[nodiscard]] bool validResignAction(rti1516_2025::ResignAction action) noexcept {
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

[[nodiscard]] bool validOrderType(rti1516_2025::OrderType orderType) noexcept {
  return orderType == rti1516_2025::RECEIVE ||
      orderType == rti1516_2025::TIMESTAMP;
}

[[nodiscard]] TransportServiceMessage responseFor(
    TransportServiceMessage const& request,
    TransportServiceStatus status,
    std::vector<std::uint8_t> payload = {}) {
  return TransportServiceMessage{
      TransportServiceMessageKind::response,
      request.operation,
      status,
      request.requestId,
      std::move(payload)};
}

[[nodiscard]] std::vector<std::uint64_t> parameterVector(
    std::set<std::uint64_t> const& handles) {
  return {handles.begin(), handles.end()};
}

constexpr std::uint8_t kLastRegionServiceStatus = static_cast<std::uint8_t>(
    RegionServiceStatus::inconsistent_catalog);

constexpr std::uint8_t kLastObjectInstanceRegionAssociationStatus =
    static_cast<std::uint8_t>(
        ObjectInstanceRegionAssociationStatus::inconsistent_catalog);

constexpr std::uint8_t kLastAttributeOwnershipCheckStatus = static_cast<std::uint8_t>(
    AttributeOwnershipCheckStatus::inconsistent_catalog);

constexpr std::uint8_t kLastAttributeOwnershipQueryStatus = static_cast<std::uint8_t>(
    AttributeOwnershipQueryStatus::inconsistent_catalog);

constexpr std::uint8_t kLastAttributeOwnershipAcquisitionIfAvailableStatus =
    static_cast<std::uint8_t>(
        AttributeOwnershipAcquisitionIfAvailableStatus::inconsistent_catalog);

constexpr std::uint8_t kLastAttributeOwnershipAcquisitionStatus =
    static_cast<std::uint8_t>(
        AttributeOwnershipAcquisitionStatus::inconsistent_catalog);

constexpr std::uint8_t kLastAttributeOwnershipReleaseDeniedStatus =
    static_cast<std::uint8_t>(
        AttributeOwnershipReleaseDeniedStatus::inconsistent_catalog);

constexpr std::uint8_t kLastAttributeOwnershipAcquisitionCancellationStatus =
    static_cast<std::uint8_t>(
        AttributeOwnershipAcquisitionCancellationStatus::inconsistent_catalog);

constexpr std::uint8_t kLastNegotiatedAttributeOwnershipDivestitureStatus =
    static_cast<std::uint8_t>(
        NegotiatedAttributeOwnershipDivestitureStatus::inconsistent_catalog);

constexpr std::uint8_t kLastConfirmDivestitureStatus = static_cast<std::uint8_t>(
    ConfirmDivestitureStatus::inconsistent_catalog);

void writeAttributeOwnershipCheckStatus(
    PayloadWriter& writer,
    AttributeOwnershipCheckStatus status) {
  auto const encoded = static_cast<std::uint8_t>(status);
  if (encoded > kLastAttributeOwnershipCheckStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership check result has an invalid status.");
  }
  writer.unsigned8(encoded);
}

[[nodiscard]] AttributeOwnershipCheckStatus readAttributeOwnershipCheckStatus(
    PayloadReader& reader) {
  auto const encoded = reader.unsigned8();
  if (encoded > kLastAttributeOwnershipCheckStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership check result has an invalid status.");
  }
  return static_cast<AttributeOwnershipCheckStatus>(encoded);
}

void writeAttributeOwnershipQueryStatus(
    PayloadWriter& writer,
    AttributeOwnershipQueryStatus status) {
  auto const encoded = static_cast<std::uint8_t>(status);
  if (encoded > kLastAttributeOwnershipQueryStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership query result has an invalid status.");
  }
  writer.unsigned8(encoded);
}

[[nodiscard]] AttributeOwnershipQueryStatus readAttributeOwnershipQueryStatus(
    PayloadReader& reader) {
  auto const encoded = reader.unsigned8();
  if (encoded > kLastAttributeOwnershipQueryStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership query result has an invalid status.");
  }
  return static_cast<AttributeOwnershipQueryStatus>(encoded);
}

void writeAttributeOwnershipAcquisitionIfAvailableStatus(
    PayloadWriter& writer,
    AttributeOwnershipAcquisitionIfAvailableStatus status) {
  auto const encoded = static_cast<std::uint8_t>(status);
  if (encoded > kLastAttributeOwnershipAcquisitionIfAvailableStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership acquisition-if-available result has an invalid status.");
  }
  writer.unsigned8(encoded);
}

[[nodiscard]] AttributeOwnershipAcquisitionIfAvailableStatus
readAttributeOwnershipAcquisitionIfAvailableStatus(PayloadReader& reader) {
  auto const encoded = reader.unsigned8();
  if (encoded > kLastAttributeOwnershipAcquisitionIfAvailableStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership acquisition-if-available result has an invalid status.");
  }
  return static_cast<AttributeOwnershipAcquisitionIfAvailableStatus>(encoded);
}

void writeAttributeOwnershipAcquisitionStatus(
    PayloadWriter& writer,
    AttributeOwnershipAcquisitionStatus status) {
  auto const encoded = static_cast<std::uint8_t>(status);
  if (encoded > kLastAttributeOwnershipAcquisitionStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership acquisition result has an invalid status.");
  }
  writer.unsigned8(encoded);
}

[[nodiscard]] AttributeOwnershipAcquisitionStatus
readAttributeOwnershipAcquisitionStatus(PayloadReader& reader) {
  auto const encoded = reader.unsigned8();
  if (encoded > kLastAttributeOwnershipAcquisitionStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership acquisition result has an invalid status.");
  }
  return static_cast<AttributeOwnershipAcquisitionStatus>(encoded);
}

void writeAttributeOwnershipReleaseDeniedStatus(
    PayloadWriter& writer,
    AttributeOwnershipReleaseDeniedStatus status) {
  auto const encoded = static_cast<std::uint8_t>(status);
  if (encoded > kLastAttributeOwnershipReleaseDeniedStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership release-denied result has an invalid status.");
  }
  writer.unsigned8(encoded);
}

[[nodiscard]] AttributeOwnershipReleaseDeniedStatus
readAttributeOwnershipReleaseDeniedStatus(PayloadReader& reader) {
  auto const encoded = reader.unsigned8();
  if (encoded > kLastAttributeOwnershipReleaseDeniedStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership release-denied result has an invalid status.");
  }
  return static_cast<AttributeOwnershipReleaseDeniedStatus>(encoded);
}

void writeAttributeOwnershipAcquisitionCancellationStatus(
    PayloadWriter& writer,
    AttributeOwnershipAcquisitionCancellationStatus status) {
  auto const encoded = static_cast<std::uint8_t>(status);
  if (encoded > kLastAttributeOwnershipAcquisitionCancellationStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership acquisition-cancellation result has an invalid status.");
  }
  writer.unsigned8(encoded);
}

[[nodiscard]] AttributeOwnershipAcquisitionCancellationStatus
readAttributeOwnershipAcquisitionCancellationStatus(PayloadReader& reader) {
  auto const encoded = reader.unsigned8();
  if (encoded > kLastAttributeOwnershipAcquisitionCancellationStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership acquisition-cancellation result has an invalid status.");
  }
  return static_cast<AttributeOwnershipAcquisitionCancellationStatus>(encoded);
}

void writeNegotiatedAttributeOwnershipDivestitureStatus(
    PayloadWriter& writer,
    NegotiatedAttributeOwnershipDivestitureStatus status) {
  auto const encoded = static_cast<std::uint8_t>(status);
  if (encoded > kLastNegotiatedAttributeOwnershipDivestitureStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process negotiated-divestiture result has an invalid status.");
  }
  writer.unsigned8(encoded);
}

[[nodiscard]] NegotiatedAttributeOwnershipDivestitureStatus
readNegotiatedAttributeOwnershipDivestitureStatus(PayloadReader& reader) {
  auto const encoded = reader.unsigned8();
  if (encoded > kLastNegotiatedAttributeOwnershipDivestitureStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process negotiated-divestiture result has an invalid status.");
  }
  return static_cast<NegotiatedAttributeOwnershipDivestitureStatus>(encoded);
}

void writeConfirmDivestitureStatus(
    PayloadWriter& writer,
    ConfirmDivestitureStatus status) {
  auto const encoded = static_cast<std::uint8_t>(status);
  if (encoded > kLastConfirmDivestitureStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process Confirm Divestiture result has an invalid status.");
  }
  writer.unsigned8(encoded);
}

[[nodiscard]] ConfirmDivestitureStatus readConfirmDivestitureStatus(
    PayloadReader& reader) {
  auto const encoded = reader.unsigned8();
  if (encoded > kLastConfirmDivestitureStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process Confirm Divestiture result has an invalid status.");
  }
  return static_cast<ConfirmDivestitureStatus>(encoded);
}

void writeRegionServiceStatus(PayloadWriter& writer, RegionServiceStatus status) {
  auto const encoded = static_cast<std::uint8_t>(status);
  if (encoded > kLastRegionServiceStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process region result has an invalid status.");
  }
  writer.unsigned8(encoded);
}

[[nodiscard]] RegionServiceStatus readRegionServiceStatus(PayloadReader& reader) {
  auto const encoded = reader.unsigned8();
  if (encoded > kLastRegionServiceStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process region result has an invalid status.");
  }
  return static_cast<RegionServiceStatus>(encoded);
}

void writeObjectInstanceRegionAssociationStatus(
    PayloadWriter& writer,
    ObjectInstanceRegionAssociationStatus status) {
  auto const encoded = static_cast<std::uint8_t>(status);
  if (encoded > kLastObjectInstanceRegionAssociationStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process object regional association result has an invalid status.");
  }
  writer.unsigned8(encoded);
}

[[nodiscard]] ObjectInstanceRegionAssociationStatus
readObjectInstanceRegionAssociationStatus(PayloadReader& reader) {
  auto const encoded = reader.unsigned8();
  if (encoded > kLastObjectInstanceRegionAssociationStatus) {
    throw ProcessFederationServiceProtocolError(
        "A process object regional association result has an invalid status.");
  }
  return static_cast<ObjectInstanceRegionAssociationStatus>(encoded);
}

void validateHandleVector(
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

void writeAttributeRegionMap(
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

[[nodiscard]] std::map<std::uint64_t, std::set<std::uint64_t>>
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

void writeAttributeUpdateRegionMetadata(
    PayloadWriter& writer,
    ProcessFederationAttributeUpdateEvent const& event) {
  writer.unsigned8(event.sentRegionHandles.has_value() ? 1U : 0U);
  if (event.sentRegionHandles.has_value()) {
    writer.unsigned64Vector(parameterVector(*event.sentRegionHandles));
  }
  writer.unsigned8(event.defaultRegionUsed ? 1U : 0U);
}

void readAttributeUpdateRegionMetadata(
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

void writeInteractionRegionMetadata(
    PayloadWriter& writer,
    ProcessFederationInteractionEvent const& event) {
  writer.unsigned8(event.sentRegionHandles.has_value() ? 1U : 0U);
  if (event.sentRegionHandles.has_value()) {
    writer.unsigned64Vector(parameterVector(*event.sentRegionHandles));
  }
  writer.unsigned8(event.defaultRegionUsed ? 1U : 0U);
}

void readInteractionRegionMetadata(
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

void writeInteractionOrderMetadata(
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

void readInteractionOrderMetadata(
    PayloadReader& reader,
    ProcessFederationInteractionEvent& event) {
  if (reader.remaining() == 0U) {
    return;
  }
  auto const marker = reader.unsigned8();
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

void writeOptionalLogicalTime(
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

std::optional<ProcessFederationLogicalTime> readOptionalLogicalTime(
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

void writeOptionalMessageId(PayloadWriter& writer, std::uint64_t messageId) {
  writer.unsigned8(messageId == 0U ? 0U : 1U);
  if (messageId != 0U) {
    writer.unsigned64(messageId);
  }
}

std::uint64_t readOptionalMessageId(PayloadReader& reader) {
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

void validateProcessLogicalTime(
    ProcessFederationLogicalTime const& timestamp,
    std::wstring const& expectedImplementationName) {
  if (expectedImplementationName.empty() ||
      timestamp.implementationName != expectedImplementationName) {
    throw ProcessFederationServiceProtocolError(
        "A process federation timestamp uses a different logical-time implementation.");
  }

  auto factory = rti1516_2025::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
      expectedImplementationName);
  if (!factory || factory->getName() != expectedImplementationName) {
    throw ProcessFederationServiceProtocolError(
        "The process federation could not create its logical-time factory.");
  }

  rti1516_2025::VariableLengthData encoded;
  if (!timestamp.encoding.empty()) {
    encoded.setData(timestamp.encoding.data(), timestamp.encoding.size());
  }
  std::unique_ptr<rti1516_2025::LogicalTime> decoded;
  try {
    decoded = factory->decodeLogicalTime(encoded);
  } catch (rti1516_2025::Exception const&) {
    throw ProcessFederationServiceProtocolError(
        "A process federation timestamp could not be decoded by its logical-time factory.");
  }
  if (!decoded || decoded->implementationName() != expectedImplementationName ||
      decoded->isInitial() || decoded->isFinal()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation timestamp is not a finite logical time.");
  }
}

[[nodiscard]] std::shared_ptr<rti1516_2025::LogicalTime const>
decodeProcessLogicalTime(
    ProcessFederationLogicalTime const& timestamp,
    std::wstring const& expectedImplementationName) {
  validateProcessLogicalTime(timestamp, expectedImplementationName);
  auto factory = rti1516_2025::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
      expectedImplementationName);
  rti1516_2025::VariableLengthData encoded;
  if (!timestamp.encoding.empty()) {
    encoded.setData(timestamp.encoding.data(), timestamp.encoding.size());
  }
  auto decoded = factory->decodeLogicalTime(encoded);
  if (!decoded) {
    throw ProcessFederationServiceProtocolError(
        "A process federation timestamp could not be reconstructed.");
  }
  std::shared_ptr<rti1516_2025::LogicalTime const> result = std::move(decoded);
  return result;
}

[[nodiscard]] std::optional<ProcessFederationLogicalTime>
encodeProcessLogicalTime(
    std::shared_ptr<rti1516_2025::LogicalTime const> const& value) {
  if (!value || value->implementationName().empty()) {
    return std::nullopt;
  }
  auto const encoded = value->encode();
  std::vector<std::uint8_t> bytes;
  if (encoded.size() != 0U) {
    auto const* data = static_cast<std::uint8_t const*>(encoded.data());
    if (data == nullptr) {
      return std::nullopt;
    }
    bytes.assign(data, data + encoded.size());
  }
  return ProcessFederationLogicalTime{value->implementationName(), std::move(bytes)};
}

[[nodiscard]] std::shared_ptr<FederateTimeState> makeProcessFederateTimeState(
    FederationDefinition const& definition) {
  if (definition.logicalTimeImplementationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation definition requires a logical-time implementation.");
  }
  auto factory = rti1516_2025::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
      definition.logicalTimeImplementationName);
  if (!factory ||
      factory->getName() != definition.logicalTimeImplementationName) {
    throw ProcessFederationServiceProtocolError(
        "The process federation could not create its logical-time factory.");
  }
  auto initial = factory->makeInitial();
  if (!initial ||
      initial->implementationName() != definition.logicalTimeImplementationName) {
    throw ProcessFederationServiceProtocolError(
        "The process federation logical-time factory did not provide an initial value.");
  }
  return std::make_shared<FederateTimeState>(
      definition.logicalTimeImplementationName, std::move(initial));
}

[[nodiscard]] std::shared_ptr<rti1516_2025::LogicalTime const>
makeProcessTsoRetractionLowerBound(FederateTimeSnapshot const& snapshot) {
  if (!snapshot.timeRegulating || !snapshot.currentTime ||
      !snapshot.lookahead || snapshot.implementationName.empty()) {
    return {};
  }

  auto const* baseTime = snapshot.currentTime.get();
  if (snapshot.timeAdvancePending) {
    if (!snapshot.advanceRequestTime) {
      return {};
    }
    baseTime = snapshot.advanceRequestTime.get();
  }
  if (baseTime == nullptr ||
      baseTime->implementationName() != snapshot.implementationName ||
      snapshot.lookahead->implementationName() != snapshot.implementationName) {
    return {};
  }

  auto factory = rti1516_2025::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
      snapshot.implementationName);
  if (!factory || factory->getName() != snapshot.implementationName) {
    return {};
  }
  std::unique_ptr<rti1516_2025::LogicalTime> lowerBound;
  try {
    lowerBound = factory->decodeLogicalTime(baseTime->encode());
    if (!lowerBound ||
        lowerBound->implementationName() != snapshot.implementationName) {
      return {};
    }
    *lowerBound += *snapshot.lookahead;
  } catch (rti1516_2025::Exception const&) {
    return {};
  }
  std::shared_ptr<rti1516_2025::LogicalTime const> result = std::move(lowerBound);
  return result;
}

constexpr std::array<std::uint8_t, 4U> kInteractionEnvelopeMagic{
    0x55U, 0x31U, 0x35U, 0x49U};  // "U15I"
constexpr std::uint8_t kInteractionEnvelopeVersion = 1U;

}  // namespace

std::vector<std::uint8_t> encodeProcessFederationCreateRequest(
    ProcessFederationCreateRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation create request requires a federation name.");
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  // Keep the historical one-field payload byte-for-byte compatible for
  // private callers that still rely on the server-owned definition.  New
  // public process clients opt into the standards-facing suffix whenever a
  // FOM/MIM/time input is supplied.
  if (!request.fomModules.empty() || request.mimModule.has_value() ||
      !request.logicalTimeImplementationName.empty() ||
      request.hasFomInputs) {
    if (request.logicalTimeImplementationName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process federation create request requires a logical-time implementation.");
    }
    for (auto const& module : request.fomModules) {
      if (module.empty()) {
        throw ProcessFederationServiceProtocolError(
            "A process federation FOM module designator cannot be empty.");
      }
    }
    if (request.mimModule.has_value() && request.mimModule->empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process federation MIM module designator cannot be empty.");
    }
    writer.wideStringVector(request.fomModules);
    writer.unsigned8(request.mimModule.has_value() ? 1U : 0U);
    if (request.mimModule.has_value()) {
      writer.wideString(*request.mimModule);
    }
    writer.wideString(request.logicalTimeImplementationName);
  }
  return std::move(writer).finish();
}

ProcessFederationCreateRequest decodeProcessFederationCreateRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationCreateRequest result;
  result.federationName = reader.wideString();
  if (reader.remaining() != 0U) {
    result.hasFomInputs = true;
    result.fomModules = reader.wideStringVector();
    auto const hasMim = reader.unsigned8();
    if (hasMim > 1U) {
      throw ProcessFederationServiceProtocolError(
          "A process federation create request has an invalid MIM marker.");
    }
    if (hasMim != 0U) {
      result.mimModule = reader.wideString();
    }
    result.logicalTimeImplementationName = reader.wideString();
  }
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation create request requires a federation name.");
  }
  if (result.hasFomInputs && result.logicalTimeImplementationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation create request requires a logical-time implementation.");
  }
  if (std::any_of(
          result.fomModules.begin(),
          result.fomModules.end(),
          [](std::wstring const& module) { return module.empty(); }) ||
      (result.mimModule.has_value() && result.mimModule->empty())) {
    throw ProcessFederationServiceProtocolError(
        "A process federation create request requires non-empty module designators.");
  }
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationJoinRequest(
    ProcessFederationJoinRequest const& request) {
  if (request.federationName.empty() || request.federateType.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation join request requires execution and type names.");
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.wideString(request.federateType);
  writer.unsigned8(request.requestedFederateName.has_value() ? 1U : 0U);
  if (request.requestedFederateName.has_value()) {
    if (request.requestedFederateName->empty()) {
      throw ProcessFederationServiceProtocolError(
          "A requested process federate name cannot be empty.");
    }
    writer.wideString(*request.requestedFederateName);
  }
  for (auto const& module : request.additionalFomModules) {
    if (module.empty()) {
      throw ProcessFederationServiceProtocolError(
          "An additional process FOM module designator cannot be empty.");
    }
  }
  writer.wideStringVector(request.additionalFomModules);
  return std::move(writer).finish();
}

ProcessFederationJoinRequest decodeProcessFederationJoinRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationJoinRequest result;
  result.federationName = reader.wideString();
  result.federateType = reader.wideString();
  auto const hasRequestedName = reader.unsigned8();
  if (hasRequestedName > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process federation join request has an invalid name marker.");
  }
  if (hasRequestedName != 0U) {
    result.requestedFederateName = reader.wideString();
  }
  // Keep the original three-field request decodable for an already-running
  // private client. New clients append the vector count; an absent suffix is
  // the wire-compatible empty-module form, while any partial suffix remains
  // subject to the normal payload bounds/trailing-byte checks below.
  if (reader.remaining() != 0U) {
    result.additionalFomModules = reader.wideStringVector();
  }
  reader.finish();
  if (result.federationName.empty() || result.federateType.empty() ||
      (result.requestedFederateName.has_value() &&
       result.requestedFederateName->empty()) ||
      std::any_of(
          result.additionalFomModules.begin(),
          result.additionalFomModules.end(),
          [](std::wstring const& module) { return module.empty(); })) {
    throw ProcessFederationServiceProtocolError(
        "A process federation join request requires non-empty names.");
  }
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationResignRequest(
    ProcessFederationResignRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation resign request requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process federation resign request requires a federate identity.");
  if (!validResignAction(request.resignAction)) {
    throw ProcessFederationServiceProtocolError(
        "A process federation resign request has an invalid resign action.");
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned8(static_cast<std::uint8_t>(request.resignAction));
  return std::move(writer).finish();
}

ProcessFederationResignRequest decodeProcessFederationResignRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationResignRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  auto const action = reader.unsigned8();
  reader.finish();
  if (action > static_cast<std::uint8_t>(rti1516_2025::NO_ACTION)) {
    throw ProcessFederationServiceProtocolError(
        "A process federation resign request has an invalid resign action.");
  }
  result.resignAction = static_cast<rti1516_2025::ResignAction>(action);
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation resign request requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process federation resign request requires a federate identity.");
  if (!validResignAction(result.resignAction)) {
    throw ProcessFederationServiceProtocolError(
        "A process federation resign request has an invalid resign action.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationRegisterSynchronizationPointRequest(
    ProcessFederationRegisterSynchronizationPointRequest const& request) {
  if (request.federationName.empty() || request.label.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process synchronization-point registration requires execution and label names.");
  }
  requireNonzero(
      request.federateId,
      "A process synchronization-point registration requires a federate identity.");
  validateHandleVector(
      request.synchronizationSet,
      "A process synchronization-point registration requires sorted, unique federate handles.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.wideString(request.label);
  writer.bytes(request.userSuppliedTag);
  writer.unsigned8(request.synchronizationSetWasSupplied ? 1U : 0U);
  writer.unsigned64Vector(request.synchronizationSet);
  return std::move(writer).finish();
}

ProcessFederationRegisterSynchronizationPointRequest
decodeProcessFederationRegisterSynchronizationPointRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationRegisterSynchronizationPointRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.label = reader.wideString();
  result.userSuppliedTag = reader.bytes();
  auto const supplied = reader.unsigned8();
  if (supplied > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process synchronization-point registration has an invalid set marker.");
  }
  result.synchronizationSetWasSupplied = supplied != 0U;
  result.synchronizationSet = reader.unsigned64Vector();
  reader.finish();
  if (result.federationName.empty() || result.label.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process synchronization-point registration requires execution and label names.");
  }
  requireNonzero(
      result.federateId,
      "A process synchronization-point registration requires a federate identity.");
  validateHandleVector(
      result.synchronizationSet,
      "A process synchronization-point registration requires sorted, unique federate handles.");
  if (!result.synchronizationSetWasSupplied &&
      !result.synchronizationSet.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A default process synchronization-point registration cannot carry a federate set.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationSynchronizationPointAchievedRequest(
    ProcessFederationSynchronizationPointAchievedRequest const& request) {
  if (request.federationName.empty() || request.label.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process synchronization-point achievement requires execution and label names.");
  }
  requireNonzero(
      request.federateId,
      "A process synchronization-point achievement requires a federate identity.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.wideString(request.label);
  writer.unsigned8(request.successfully ? 1U : 0U);
  return std::move(writer).finish();
}

ProcessFederationSynchronizationPointAchievedRequest
decodeProcessFederationSynchronizationPointAchievedRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationSynchronizationPointAchievedRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.label = reader.wideString();
  auto const successfully = reader.unsigned8();
  if (successfully > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process synchronization-point achievement has an invalid success marker.");
  }
  result.successfully = successfully != 0U;
  reader.finish();
  if (result.federationName.empty() || result.label.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process synchronization-point achievement requires execution and label names.");
  }
  requireNonzero(
      result.federateId,
      "A process synchronization-point achievement requires a federate identity.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationRegisterSynchronizationPointResult(
    ProcessFederationRegisterSynchronizationPointResult const& result) {
  auto const status = static_cast<std::uint8_t>(result.status);
  if (status > static_cast<std::uint8_t>(
                   ProcessFederationSynchronizationPointRegistrationStatus::
                       callback_route_missing)) {
    throw ProcessFederationServiceProtocolError(
        "A process synchronization-point registration result has an invalid status.");
  }
  auto const failureReason = static_cast<std::uint8_t>(result.failureReason);
  if (failureReason > static_cast<std::uint8_t>(
                          rti1516_2025::SYNCHRONIZATION_SET_MEMBER_NOT_JOINED)) {
    throw ProcessFederationServiceProtocolError(
        "A process synchronization-point registration result has an invalid failure reason.");
  }
  PayloadWriter writer;
  writer.unsigned8(status);
  writer.unsigned8(result.succeeded ? 1U : 0U);
  writer.unsigned8(failureReason);
  return std::move(writer).finish();
}

ProcessFederationRegisterSynchronizationPointResult
decodeProcessFederationRegisterSynchronizationPointResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationRegisterSynchronizationPointResult result;
  auto const status = reader.unsigned8();
  auto const succeeded = reader.unsigned8();
  auto const failureReason = reader.unsigned8();
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   ProcessFederationSynchronizationPointRegistrationStatus::
                       callback_route_missing) ||
      succeeded > 1U ||
      failureReason > static_cast<std::uint8_t>(
                          rti1516_2025::SYNCHRONIZATION_SET_MEMBER_NOT_JOINED)) {
    throw ProcessFederationServiceProtocolError(
        "A process synchronization-point registration result is invalid.");
  }
  result.status = static_cast<
      ProcessFederationSynchronizationPointRegistrationStatus>(status);
  result.succeeded = succeeded != 0U;
  result.failureReason = static_cast<rti1516_2025::SynchronizationPointFailureReason>(
      failureReason);
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationSynchronizationPointAchievedResult(
    ProcessFederationSynchronizationPointAchievedResult const& result) {
  auto const status = static_cast<std::uint8_t>(result.status);
  if (status > static_cast<std::uint8_t>(
                   ProcessFederationSynchronizationPointAchievedStatus::
                       synchronization_point_label_not_announced)) {
    throw ProcessFederationServiceProtocolError(
        "A process synchronization-point achievement result has an invalid status.");
  }
  PayloadWriter writer;
  writer.unsigned8(status);
  return std::move(writer).finish();
}

ProcessFederationSynchronizationPointAchievedResult
decodeProcessFederationSynchronizationPointAchievedResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const status = reader.unsigned8();
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   ProcessFederationSynchronizationPointAchievedStatus::
                       synchronization_point_label_not_announced)) {
    throw ProcessFederationServiceProtocolError(
        "A process synchronization-point achievement result has an invalid status.");
  }
  return ProcessFederationSynchronizationPointAchievedResult{
      static_cast<ProcessFederationSynchronizationPointAchievedStatus>(status)};
}

std::vector<std::uint8_t> encodeProcessFederationSaveRequest(
    ProcessFederationSaveRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation-save request requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process federation-save request requires a federate identity.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.wideString(request.label);
  writeOptionalLogicalTime(writer, request.timestamp);
  return std::move(writer).finish();
}

ProcessFederationSaveRequest decodeProcessFederationSaveRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationSaveRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.label = reader.wideString();
  result.timestamp = readOptionalLogicalTime(reader);
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation-save request requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process federation-save request requires a federate identity.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationSaveControlResult(
    ProcessFederationSaveControlResult const& result) {
  auto const status = static_cast<std::uint8_t>(result.status);
  if (status > static_cast<std::uint8_t>(
                   FederationSaveControlStatus::inconsistent_temporal_state)) {
    throw ProcessFederationServiceProtocolError(
        "A process federation-save result has an invalid status.");
  }
  PayloadWriter writer;
  writer.unsigned8(status);
  return std::move(writer).finish();
}

ProcessFederationSaveControlResult decodeProcessFederationSaveControlResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const status = reader.unsigned8();
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   FederationSaveControlStatus::inconsistent_temporal_state)) {
    throw ProcessFederationServiceProtocolError(
        "A process federation-save result has an invalid status.");
  }
  return ProcessFederationSaveControlResult{
      static_cast<FederationSaveControlStatus>(status)};
}

std::vector<std::uint8_t> encodeProcessFederationRestoreRequest(
    ProcessFederationRestoreRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation-restore request requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process federation-restore request requires a federate identity.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.wideString(request.label);
  writer.unsigned8(request.callbacksEnabled ? 1U : 0U);
  return std::move(writer).finish();
}

ProcessFederationRestoreRequest decodeProcessFederationRestoreRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationRestoreRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.label = reader.wideString();
  if (reader.remaining() != 0U) {
    result.callbacksEnabled = reader.unsigned8() != 0U;
  }
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation-restore request requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process federation-restore request requires a federate identity.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationRestoreControlResult(
    ProcessFederationRestoreControlResult const& result) {
  auto const status = static_cast<std::uint8_t>(result.status);
  if (status > static_cast<std::uint8_t>(
                   FederationRestoreControlStatus::callback_route_missing)) {
    throw ProcessFederationServiceProtocolError(
        "A process federation-restore result has an invalid status.");
  }
  PayloadWriter writer;
  writer.unsigned8(status);
  return std::move(writer).finish();
}

ProcessFederationRestoreControlResult decodeProcessFederationRestoreControlResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const status = reader.unsigned8();
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   FederationRestoreControlStatus::callback_route_missing)) {
    throw ProcessFederationServiceProtocolError(
        "A process federation-restore result has an invalid status.");
  }
  return ProcessFederationRestoreControlResult{
      static_cast<FederationRestoreControlStatus>(status)};
}

std::vector<std::uint8_t>
encodeProcessFederationChangeInteractionOrderTypeRequest(
    ProcessFederationChangeInteractionOrderTypeRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process interaction order-type change requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process interaction order-type change requires a federate identity.");
  requireNonzero(
      request.interactionClassHandle,
      "A process interaction order-type change requires an interaction class.");
  if (!validOrderType(request.orderType)) {
    throw ProcessFederationServiceProtocolError(
        "A process interaction order-type change has an invalid order type.");
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.interactionClassHandle);
  writer.unsigned8(static_cast<std::uint8_t>(request.orderType));
  return std::move(writer).finish();
}

ProcessFederationChangeInteractionOrderTypeRequest
decodeProcessFederationChangeInteractionOrderTypeRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationChangeInteractionOrderTypeRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.interactionClassHandle = reader.unsigned64();
  auto const encodedOrderType = reader.unsigned8();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process interaction order-type change requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process interaction order-type change requires a federate identity.");
  requireNonzero(
      result.interactionClassHandle,
      "A process interaction order-type change requires an interaction class.");
  result.orderType = static_cast<rti1516_2025::OrderType>(encodedOrderType);
  if (!validOrderType(result.orderType)) {
    throw ProcessFederationServiceProtocolError(
        "A process interaction order-type change has an invalid order type.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationChangeAttributeOrderTypeRequest(
    ProcessFederationChangeAttributeOrderTypeRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute order-type change requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process attribute order-type change requires a federate identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process attribute order-type change requires an object instance.");
  for (auto const attributeHandle : request.attributeHandles) {
    requireNonzero(
        attributeHandle,
        "A process attribute order-type change requires non-zero attributes.");
  }
  if (!validOrderType(request.orderType)) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute order-type change has an invalid order type.");
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.objectInstanceHandle);
  writer.unsigned64Vector(request.attributeHandles);
  writer.unsigned8(static_cast<std::uint8_t>(request.orderType));
  return std::move(writer).finish();
}

ProcessFederationChangeAttributeOrderTypeRequest
decodeProcessFederationChangeAttributeOrderTypeRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationChangeAttributeOrderTypeRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  result.attributeHandles = reader.unsigned64Vector();
  auto const encodedOrderType = reader.unsigned8();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute order-type change requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process attribute order-type change requires a federate identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process attribute order-type change requires an object instance.");
  for (auto const attributeHandle : result.attributeHandles) {
    requireNonzero(
        attributeHandle,
        "A process attribute order-type change requires non-zero attributes.");
  }
  result.orderType = static_cast<rti1516_2025::OrderType>(encodedOrderType);
  if (!validOrderType(result.orderType)) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute order-type change has an invalid order type.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationChangeDefaultAttributeOrderTypeRequest(
    ProcessFederationChangeDefaultAttributeOrderTypeRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process default attribute order-type change requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process default attribute order-type change requires a federate identity.");
  requireNonzero(
      request.objectClassHandle,
      "A process default attribute order-type change requires an object class.");
  for (auto const attributeHandle : request.attributeHandles) {
    requireNonzero(
        attributeHandle,
        "A process default attribute order-type change requires non-zero attributes.");
  }
  if (!validOrderType(request.orderType)) {
    throw ProcessFederationServiceProtocolError(
        "A process default attribute order-type change has an invalid order type.");
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.objectClassHandle);
  writer.unsigned64Vector(request.attributeHandles);
  writer.unsigned8(static_cast<std::uint8_t>(request.orderType));
  return std::move(writer).finish();
}

ProcessFederationChangeDefaultAttributeOrderTypeRequest
decodeProcessFederationChangeDefaultAttributeOrderTypeRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationChangeDefaultAttributeOrderTypeRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectClassHandle = reader.unsigned64();
  result.attributeHandles = reader.unsigned64Vector();
  auto const encodedOrderType = reader.unsigned8();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process default attribute order-type change requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process default attribute order-type change requires a federate identity.");
  requireNonzero(
      result.objectClassHandle,
      "A process default attribute order-type change requires an object class.");
  for (auto const attributeHandle : result.attributeHandles) {
    requireNonzero(
        attributeHandle,
        "A process default attribute order-type change requires non-zero attributes.");
  }
  result.orderType = static_cast<rti1516_2025::OrderType>(encodedOrderType);
  if (!validOrderType(result.orderType)) {
    throw ProcessFederationServiceProtocolError(
        "A process default attribute order-type change has an invalid order type.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationChangeDefaultAttributeTransportationTypeRequest(
    ProcessFederationChangeDefaultAttributeTransportationTypeRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process default attribute transportation-type change requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process default attribute transportation-type change requires a federate identity.");
  requireNonzero(
      request.objectClassHandle,
      "A process default attribute transportation-type change requires an object class.");
  for (auto const attributeHandle : request.attributeHandles) {
    requireNonzero(
        attributeHandle,
        "A process default attribute transportation-type change requires non-zero attributes.");
  }
  requireNonzero(
      request.transportationTypeHandle,
      "A process default attribute transportation-type change requires a transportation type.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.objectClassHandle);
  writer.unsigned64Vector(request.attributeHandles);
  writer.unsigned64(request.transportationTypeHandle);
  return std::move(writer).finish();
}

ProcessFederationChangeDefaultAttributeTransportationTypeRequest
decodeProcessFederationChangeDefaultAttributeTransportationTypeRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationChangeDefaultAttributeTransportationTypeRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectClassHandle = reader.unsigned64();
  result.attributeHandles = reader.unsigned64Vector();
  result.transportationTypeHandle = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process default attribute transportation-type change requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process default attribute transportation-type change requires a federate identity.");
  requireNonzero(
      result.objectClassHandle,
      "A process default attribute transportation-type change requires an object class.");
  for (auto const attributeHandle : result.attributeHandles) {
    requireNonzero(
        attributeHandle,
        "A process default attribute transportation-type change requires non-zero attributes.");
  }
  requireNonzero(
      result.transportationTypeHandle,
      "A process default attribute transportation-type change requires a transportation type.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationRequestAttributeTransportationTypeChangeRequest(
    ProcessFederationRequestAttributeTransportationTypeChangeRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute transportation-type change requires a federation name.");
  }
  requireNonzero(
      request.requestingFederateId,
      "A process attribute transportation-type change requires a federate identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process attribute transportation-type change requires an object instance.");
  for (auto const attributeHandle : request.attributeHandles) {
    requireNonzero(
        attributeHandle,
        "A process attribute transportation-type change requires non-zero attributes.");
  }
  requireNonzero(
      request.transportationTypeHandle,
      "A process attribute transportation-type change requires a transportation type.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.requestingFederateId);
  writer.unsigned64(request.objectInstanceHandle);
  writer.unsigned64Vector(request.attributeHandles);
  writer.unsigned64(request.transportationTypeHandle);
  return std::move(writer).finish();
}

ProcessFederationRequestAttributeTransportationTypeChangeRequest
decodeProcessFederationRequestAttributeTransportationTypeChangeRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationRequestAttributeTransportationTypeChangeRequest result;
  result.federationName = reader.wideString();
  result.requestingFederateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  result.attributeHandles = reader.unsigned64Vector();
  result.transportationTypeHandle = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute transportation-type change requires a federation name.");
  }
  requireNonzero(
      result.requestingFederateId,
      "A process attribute transportation-type change requires a federate identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process attribute transportation-type change requires an object instance.");
  for (auto const attributeHandle : result.attributeHandles) {
    requireNonzero(
        attributeHandle,
        "A process attribute transportation-type change requires non-zero attributes.");
  }
  requireNonzero(
      result.transportationTypeHandle,
      "A process attribute transportation-type change requires a transportation type.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationQueryAttributeTransportationTypeRequest(
    ProcessFederationQueryAttributeTransportationTypeRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute transportation-type query requires a federation name.");
  }
  requireNonzero(
      request.requestingFederateId,
      "A process attribute transportation-type query requires a federate identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process attribute transportation-type query requires an object instance.");
  requireNonzero(
      request.attributeHandle,
      "A process attribute transportation-type query requires an attribute.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.requestingFederateId);
  writer.unsigned64(request.objectInstanceHandle);
  writer.unsigned64(request.attributeHandle);
  return std::move(writer).finish();
}

ProcessFederationQueryAttributeTransportationTypeRequest
decodeProcessFederationQueryAttributeTransportationTypeRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationQueryAttributeTransportationTypeRequest result;
  result.federationName = reader.wideString();
  result.requestingFederateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  result.attributeHandle = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute transportation-type query requires a federation name.");
  }
  requireNonzero(
      result.requestingFederateId,
      "A process attribute transportation-type query requires a federate identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process attribute transportation-type query requires an object instance.");
  requireNonzero(
      result.attributeHandle,
      "A process attribute transportation-type query requires an attribute.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationRequestInteractionTransportationTypeChangeRequest(
    ProcessFederationRequestInteractionTransportationTypeChangeRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process interaction transportation-type change requires a federation name.");
  }
  requireNonzero(
      request.requestingFederateId,
      "A process interaction transportation-type change requires a federate identity.");
  requireNonzero(
      request.interactionClassHandle,
      "A process interaction transportation-type change requires an interaction class.");
  requireNonzero(
      request.transportationTypeHandle,
      "A process interaction transportation-type change requires a transportation type.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.requestingFederateId);
  writer.unsigned64(request.interactionClassHandle);
  writer.unsigned64(request.transportationTypeHandle);
  return std::move(writer).finish();
}

ProcessFederationRequestInteractionTransportationTypeChangeRequest
decodeProcessFederationRequestInteractionTransportationTypeChangeRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationRequestInteractionTransportationTypeChangeRequest result;
  result.federationName = reader.wideString();
  result.requestingFederateId = reader.unsigned64();
  result.interactionClassHandle = reader.unsigned64();
  result.transportationTypeHandle = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process interaction transportation-type change requires a federation name.");
  }
  requireNonzero(
      result.requestingFederateId,
      "A process interaction transportation-type change requires a federate identity.");
  requireNonzero(
      result.interactionClassHandle,
      "A process interaction transportation-type change requires an interaction class.");
  requireNonzero(
      result.transportationTypeHandle,
      "A process interaction transportation-type change requires a transportation type.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationQueryInteractionTransportationTypeRequest(
    ProcessFederationQueryInteractionTransportationTypeRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process interaction transportation-type query requires a federation name.");
  }
  requireNonzero(
      request.requestingFederateId,
      "A process interaction transportation-type query requires a requester identity.");
  requireNonzero(
      request.queriedFederateId,
      "A process interaction transportation-type query requires a queried federate identity.");
  requireNonzero(
      request.interactionClassHandle,
      "A process interaction transportation-type query requires an interaction class.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.requestingFederateId);
  writer.unsigned64(request.queriedFederateId);
  writer.unsigned64(request.interactionClassHandle);
  return std::move(writer).finish();
}

ProcessFederationQueryInteractionTransportationTypeRequest
decodeProcessFederationQueryInteractionTransportationTypeRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationQueryInteractionTransportationTypeRequest result;
  result.federationName = reader.wideString();
  result.requestingFederateId = reader.unsigned64();
  result.queriedFederateId = reader.unsigned64();
  result.interactionClassHandle = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process interaction transportation-type query requires a federation name.");
  }
  requireNonzero(
      result.requestingFederateId,
      "A process interaction transportation-type query requires a requester identity.");
  requireNonzero(
      result.queriedFederateId,
      "A process interaction transportation-type query requires a queried federate identity.");
  requireNonzero(
      result.interactionClassHandle,
      "A process interaction transportation-type query requires an interaction class.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationSendInteractionRequest(
    ProcessFederationSendInteractionRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation interaction request requires a federation name.");
  }
  requireNonzero(
      request.producingFederateId,
      "A process federation interaction request requires a producer identity.");
  requireNonzero(
      request.interactionClassHandle,
      "A process federation interaction request requires an interaction class.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.producingFederateId);
  writer.unsigned64(request.interactionClassHandle);
  writer.unsigned64Vector(request.sentParameterHandles);
  writer.bytes(request.payload);
  writeOptionalLogicalTime(writer, request.timestamp);
  if (request.sentRegionHandles.has_value()) {
    writer.unsigned8(1U);
    writer.unsigned64Vector(parameterVector(*request.sentRegionHandles));
  }
  return std::move(writer).finish();
}

ProcessFederationSendInteractionRequest
decodeProcessFederationSendInteractionRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationSendInteractionRequest result;
  result.federationName = reader.wideString();
  result.producingFederateId = reader.unsigned64();
  result.interactionClassHandle = reader.unsigned64();
  result.sentParameterHandles = reader.unsigned64Vector();
  result.payload = reader.bytes();
  result.timestamp = readOptionalLogicalTime(reader);
  if (reader.remaining() != 0U) {
    auto const hasSentRegions = reader.unsigned8();
    if (hasSentRegions > 1U) {
      throw ProcessFederationServiceProtocolError(
          "A process federation interaction request has an invalid region marker.");
    }
    if (hasSentRegions != 0U) {
      auto regionVector = reader.unsigned64Vector();
      validateHandleVector(
          regionVector,
          "A process federation interaction request requires sorted, unique region handles.");
      result.sentRegionHandles.emplace(
          regionVector.begin(), regionVector.end());
    }
  }
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation interaction request requires a federation name.");
  }
  requireNonzero(
      result.producingFederateId,
      "A process federation interaction request requires a producer identity.");
  requireNonzero(
      result.interactionClassHandle,
      "A process federation interaction request requires an interaction class.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationSendDirectedInteractionRequest(
    ProcessFederationSendDirectedInteractionRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process directed-interaction request requires a federation name.");
  }
  requireNonzero(
      request.producingFederateId,
      "A process directed-interaction request requires a producer identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process directed-interaction request requires a target object instance.");
  requireNonzero(
      request.interactionClassHandle,
      "A process directed-interaction request requires an interaction class.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.producingFederateId);
  writer.unsigned64(request.objectInstanceHandle);
  writer.unsigned64(request.interactionClassHandle);
  writer.unsigned64Vector(request.sentParameterHandles);
  writer.bytes(request.payload);
  writeOptionalLogicalTime(writer, request.timestamp);
  return std::move(writer).finish();
}

ProcessFederationSendDirectedInteractionRequest
decodeProcessFederationSendDirectedInteractionRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationSendDirectedInteractionRequest result;
  result.federationName = reader.wideString();
  result.producingFederateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  result.interactionClassHandle = reader.unsigned64();
  result.sentParameterHandles = reader.unsigned64Vector();
  result.payload = reader.bytes();
  result.timestamp = readOptionalLogicalTime(reader);
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process directed-interaction request requires a federation name.");
  }
  requireNonzero(
      result.producingFederateId,
      "A process directed-interaction request requires a producer identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process directed-interaction request requires a target object instance.");
  requireNonzero(
      result.interactionClassHandle,
      "A process directed-interaction request requires an interaction class.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationRetractRequest(
    ProcessFederationRetractRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process retraction request requires a federation name.");
  }
  requireNonzero(
      request.producingFederateId,
      "A process retraction request requires a producer identity.");
  requireNonzero(
      request.messageId,
      "A process retraction request requires a message identity.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.producingFederateId);
  writer.unsigned64(request.messageId);
  return std::move(writer).finish();
}

ProcessFederationRetractRequest decodeProcessFederationRetractRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationRetractRequest result;
  result.federationName = reader.wideString();
  result.producingFederateId = reader.unsigned64();
  result.messageId = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process retraction request requires a federation name.");
  }
  requireNonzero(
      result.producingFederateId,
      "A process retraction request requires a producer identity.");
  requireNonzero(
      result.messageId,
      "A process retraction request requires a message identity.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationReceiveInteractionRequest(
    ProcessFederationReceiveInteractionRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation receive request requires a federation name.");
  }
  requireNonzero(
      request.receivingFederateId,
      "A process federation receive request requires a recipient identity.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.receivingFederateId);
  return std::move(writer).finish();
}

ProcessFederationReceiveInteractionRequest
decodeProcessFederationReceiveInteractionRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationReceiveInteractionRequest result;
  result.federationName = reader.wideString();
  result.receivingFederateId = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation receive request requires a federation name.");
  }
  requireNonzero(
      result.receivingFederateId,
      "A process federation receive request requires a recipient identity.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationAcknowledgeTsoDeliveryRequest(
    ProcessFederationAcknowledgeTsoDeliveryRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process TSO delivery acknowledgement requires a federation name.");
  }
  requireNonzero(
      request.receivingFederateId,
      "A process TSO delivery acknowledgement requires a recipient identity.");
  requireNonzero(
      request.messageId,
      "A process TSO delivery acknowledgement requires a message identity.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.receivingFederateId);
  writer.unsigned64(request.messageId);
  return std::move(writer).finish();
}

ProcessFederationAcknowledgeTsoDeliveryRequest
decodeProcessFederationAcknowledgeTsoDeliveryRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationAcknowledgeTsoDeliveryRequest result;
  result.federationName = reader.wideString();
  result.receivingFederateId = reader.unsigned64();
  result.messageId = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process TSO delivery acknowledgement requires a federation name.");
  }
  requireNonzero(
      result.receivingFederateId,
      "A process TSO delivery acknowledgement requires a recipient identity.");
  requireNonzero(
      result.messageId,
      "A process TSO delivery acknowledgement requires a message identity.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationQueryLogicalTimeRequest(
    ProcessFederationQueryLogicalTimeRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process logical-time query requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process logical-time query requires a federate identity.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  return std::move(writer).finish();
}

ProcessFederationQueryLogicalTimeRequest
decodeProcessFederationQueryLogicalTimeRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationQueryLogicalTimeRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process logical-time query requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process logical-time query requires a federate identity.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationEnableTimeRegulationRequest(
    ProcessFederationEnableTimeRegulationRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process time-regulation request requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process time-regulation request requires a federate identity.");
  if (request.lookahead.implementationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process time-regulation request requires a logical-time implementation.");
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.wideString(request.lookahead.implementationName);
  writer.bytes(request.lookahead.encoding);
  return std::move(writer).finish();
}

ProcessFederationEnableTimeRegulationRequest
decodeProcessFederationEnableTimeRegulationRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationEnableTimeRegulationRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.lookahead.implementationName = reader.wideString();
  result.lookahead.encoding = reader.bytes();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process time-regulation request requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process time-regulation request requires a federate identity.");
  if (result.lookahead.implementationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process time-regulation request requires a logical-time implementation.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationModifyLookaheadRequest(
    ProcessFederationModifyLookaheadRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process Modify Lookahead request requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process Modify Lookahead request requires a federate identity.");
  if (request.lookahead.implementationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process Modify Lookahead request requires a logical-time implementation.");
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.wideString(request.lookahead.implementationName);
  writer.bytes(request.lookahead.encoding);
  return std::move(writer).finish();
}

ProcessFederationModifyLookaheadRequest
decodeProcessFederationModifyLookaheadRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationModifyLookaheadRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.lookahead.implementationName = reader.wideString();
  result.lookahead.encoding = reader.bytes();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process Modify Lookahead request requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process Modify Lookahead request requires a federate identity.");
  if (result.lookahead.implementationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process Modify Lookahead request requires a logical-time implementation.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationEnableTimeConstrainedRequest(
    ProcessFederationEnableTimeConstrainedRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process time-constrained request requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process time-constrained request requires a federate identity.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  return std::move(writer).finish();
}

ProcessFederationEnableTimeConstrainedRequest
decodeProcessFederationEnableTimeConstrainedRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationEnableTimeConstrainedRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process time-constrained request requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process time-constrained request requires a federate identity.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationTimeAdvanceRequest(
    ProcessFederationTimeAdvanceRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process time-advance request requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process time-advance request requires a federate identity.");
  if (request.requestedTime.implementationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process time-advance request requires a logical-time implementation.");
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.wideString(request.requestedTime.implementationName);
  writer.bytes(request.requestedTime.encoding);
  return std::move(writer).finish();
}

ProcessFederationTimeAdvanceRequest decodeProcessFederationTimeAdvanceRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationTimeAdvanceRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.requestedTime.implementationName = reader.wideString();
  result.requestedTime.encoding = reader.bytes();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process time-advance request requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process time-advance request requires a federate identity.");
  if (result.requestedTime.implementationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process time-advance request requires a logical-time implementation.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationUpdateAttributeValuesRequest(
    ProcessFederationUpdateAttributeValuesRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation attribute update requires a federation name.");
  }
  requireNonzero(
      request.producingFederateId,
      "A process federation attribute update requires a producer identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process federation attribute update requires an object instance.");
  if (request.attributeValues.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation attribute update requires at least one attribute.");
  }
  if (request.attributeValues.size() >
      std::numeric_limits<std::uint32_t>::max()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation attribute update has too many attributes.");
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.producingFederateId);
  writer.unsigned64(request.objectInstanceHandle);
  writer.unsigned32(static_cast<std::uint32_t>(request.attributeValues.size()));
  std::set<std::uint64_t> seenHandles;
  for (auto const& [attributeHandle, value] : request.attributeValues) {
    requireNonzero(
        attributeHandle,
        "A process federation attribute update requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process federation attribute update repeats an attribute identity.");
    }
    writer.unsigned64(attributeHandle);
    writer.bytes(value);
  }
  writer.bytes(request.userSuppliedTag);
  writeOptionalLogicalTime(writer, request.timestamp);
  return std::move(writer).finish();
}

ProcessFederationUpdateAttributeValuesRequest
decodeProcessFederationUpdateAttributeValuesRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationUpdateAttributeValuesRequest result;
  result.federationName = reader.wideString();
  result.producingFederateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  auto const attributeCount = reader.unsigned32();
  result.attributeValues.reserve(attributeCount);
  std::set<std::uint64_t> seenHandles;
  for (std::uint32_t index = 0U; index < attributeCount; ++index) {
    auto const attributeHandle = reader.unsigned64();
    requireNonzero(
        attributeHandle,
        "A process federation attribute update requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process federation attribute update repeats an attribute identity.");
    }
    result.attributeValues.emplace_back(attributeHandle, reader.bytes());
  }
  result.userSuppliedTag = reader.bytes();
  result.timestamp = readOptionalLogicalTime(reader);
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation attribute update requires a federation name.");
  }
  requireNonzero(
      result.producingFederateId,
      "A process federation attribute update requires a producer identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process federation attribute update requires an object instance.");
  if (result.attributeValues.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation attribute update requires at least one attribute.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationRequestAttributeValueUpdateRequest(
    ProcessFederationRequestAttributeValueUpdateRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process Request Attribute Value Update requires a federation name.");
  }
  requireNonzero(
      request.requestingFederateId,
      "A process Request Attribute Value Update requires a requester identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process Request Attribute Value Update requires an object instance.");
  if (request.requestedAttributeHandles.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process Request Attribute Value Update requires at least one attribute.");
  }
  if (request.requestedAttributeHandles.size() >
      std::numeric_limits<std::uint32_t>::max()) {
    throw ProcessFederationServiceProtocolError(
        "A process Request Attribute Value Update has too many attributes.");
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.requestingFederateId);
  writer.unsigned64(request.objectInstanceHandle);
  writer.unsigned32(
      static_cast<std::uint32_t>(request.requestedAttributeHandles.size()));
  std::set<std::uint64_t> seenHandles;
  for (std::uint64_t const attributeHandle :
       request.requestedAttributeHandles) {
    requireNonzero(
        attributeHandle,
        "A process Request Attribute Value Update requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process Request Attribute Value Update repeats an attribute identity.");
    }
    writer.unsigned64(attributeHandle);
  }
  writer.bytes(request.userSuppliedTag);
  return std::move(writer).finish();
}

ProcessFederationRequestAttributeValueUpdateRequest
decodeProcessFederationRequestAttributeValueUpdateRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationRequestAttributeValueUpdateRequest result;
  result.federationName = reader.wideString();
  result.requestingFederateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  auto const attributeCount = reader.unsigned32();
  result.requestedAttributeHandles.reserve(attributeCount);
  std::set<std::uint64_t> seenHandles;
  for (std::uint32_t index = 0U; index < attributeCount; ++index) {
    auto const attributeHandle = reader.unsigned64();
    requireNonzero(
        attributeHandle,
        "A process Request Attribute Value Update requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process Request Attribute Value Update repeats an attribute identity.");
    }
    result.requestedAttributeHandles.push_back(attributeHandle);
  }
  result.userSuppliedTag = reader.bytes();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process Request Attribute Value Update requires a federation name.");
  }
  requireNonzero(
      result.requestingFederateId,
      "A process Request Attribute Value Update requires a requester identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process Request Attribute Value Update requires an object instance.");
  if (result.requestedAttributeHandles.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process Request Attribute Value Update requires at least one attribute.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationRequestAttributeValueUpdateClassRequest(
    ProcessFederationRequestAttributeValueUpdateClassRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process class Request Attribute Value Update requires a federation name.");
  }
  requireNonzero(
      request.requestingFederateId,
      "A process class Request Attribute Value Update requires a requester identity.");
  requireNonzero(
      request.objectClassHandle,
      "A process class Request Attribute Value Update requires an object class.");
  if (request.requestedAttributeHandles.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process class Request Attribute Value Update requires at least one attribute.");
  }
  if (request.requestedAttributeHandles.size() >
      std::numeric_limits<std::uint32_t>::max()) {
    throw ProcessFederationServiceProtocolError(
        "A process class Request Attribute Value Update has too many attributes.");
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.requestingFederateId);
  writer.unsigned64(request.objectClassHandle);
  writer.unsigned32(
      static_cast<std::uint32_t>(request.requestedAttributeHandles.size()));
  std::set<std::uint64_t> seenHandles;
  for (std::uint64_t const attributeHandle :
       request.requestedAttributeHandles) {
    requireNonzero(
        attributeHandle,
        "A process class Request Attribute Value Update requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process class Request Attribute Value Update repeats an attribute identity.");
    }
    writer.unsigned64(attributeHandle);
  }
  writer.bytes(request.userSuppliedTag);
  return std::move(writer).finish();
}

ProcessFederationRequestAttributeValueUpdateClassRequest
decodeProcessFederationRequestAttributeValueUpdateClassRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationRequestAttributeValueUpdateClassRequest result;
  result.federationName = reader.wideString();
  result.requestingFederateId = reader.unsigned64();
  result.objectClassHandle = reader.unsigned64();
  auto const attributeCount = reader.unsigned32();
  result.requestedAttributeHandles.reserve(attributeCount);
  std::set<std::uint64_t> seenHandles;
  for (std::uint32_t index = 0U; index < attributeCount; ++index) {
    auto const attributeHandle = reader.unsigned64();
    requireNonzero(
        attributeHandle,
        "A process class Request Attribute Value Update requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process class Request Attribute Value Update repeats an attribute identity.");
    }
    result.requestedAttributeHandles.push_back(attributeHandle);
  }
  result.userSuppliedTag = reader.bytes();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process class Request Attribute Value Update requires a federation name.");
  }
  requireNonzero(
      result.requestingFederateId,
      "A process class Request Attribute Value Update requires a requester identity.");
  requireNonzero(
      result.objectClassHandle,
      "A process class Request Attribute Value Update requires an object class.");
  if (result.requestedAttributeHandles.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process class Request Attribute Value Update requires at least one attribute.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationRequestAttributeValueUpdateClassWithRegionsRequest(
    ProcessFederationRequestAttributeValueUpdateClassWithRegionsRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process regional class Request Attribute Value Update requires a federation name.");
  }
  requireNonzero(
      request.requestingFederateId,
      "A process regional class Request Attribute Value Update requires a requester identity.");
  requireNonzero(
      request.objectClassHandle,
      "A process regional class Request Attribute Value Update requires an object class.");
  if (request.requestedAttributeHandles.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process regional class Request Attribute Value Update requires at least one attribute.");
  }
  if (request.requestedAttributeHandles.size() >
      std::numeric_limits<std::uint32_t>::max()) {
    throw ProcessFederationServiceProtocolError(
        "A process regional class Request Attribute Value Update has too many attributes.");
  }
  std::set<std::uint64_t> seenHandles;
  for (std::uint64_t const attributeHandle :
       request.requestedAttributeHandles) {
    requireNonzero(
        attributeHandle,
        "A process regional class Request Attribute Value Update requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process regional class Request Attribute Value Update repeats an attribute identity.");
    }
  }
  if (request.requestRegionsByAttribute.empty() ||
      request.requestRegionsByAttribute.size() != seenHandles.size()) {
    throw ProcessFederationServiceProtocolError(
        "A process regional class Request Attribute Value Update requires one region set per attribute.");
  }
  for (auto const& [attributeHandle, regionHandles] :
       request.requestRegionsByAttribute) {
    requireNonzero(
        attributeHandle,
        "A process regional class Request Attribute Value Update requires region-map attribute identities.");
    if (!seenHandles.contains(attributeHandle) || regionHandles.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process regional class Request Attribute Value Update has an invalid attribute-region pair.");
    }
    for (std::uint64_t const regionHandle : regionHandles) {
      requireNonzero(
          regionHandle,
          "A process regional class Request Attribute Value Update requires region identities.");
    }
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.requestingFederateId);
  writer.unsigned64(request.objectClassHandle);
  writer.unsigned32(
      static_cast<std::uint32_t>(request.requestedAttributeHandles.size()));
  for (std::uint64_t const attributeHandle :
       request.requestedAttributeHandles) {
    writer.unsigned64(attributeHandle);
  }
  writeAttributeRegionMap(writer, request.requestRegionsByAttribute);
  writer.bytes(request.userSuppliedTag);
  return std::move(writer).finish();
}

ProcessFederationRequestAttributeValueUpdateClassWithRegionsRequest
decodeProcessFederationRequestAttributeValueUpdateClassWithRegionsRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationRequestAttributeValueUpdateClassWithRegionsRequest result;
  result.federationName = reader.wideString();
  result.requestingFederateId = reader.unsigned64();
  result.objectClassHandle = reader.unsigned64();
  auto const attributeCount = reader.unsigned32();
  result.requestedAttributeHandles.reserve(attributeCount);
  std::set<std::uint64_t> seenHandles;
  for (std::uint32_t index = 0U; index < attributeCount; ++index) {
    auto const attributeHandle = reader.unsigned64();
    requireNonzero(
        attributeHandle,
        "A process regional class Request Attribute Value Update requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process regional class Request Attribute Value Update repeats an attribute identity.");
    }
    result.requestedAttributeHandles.push_back(attributeHandle);
  }
  result.requestRegionsByAttribute = readAttributeRegionMap(reader);
  result.userSuppliedTag = reader.bytes();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process regional class Request Attribute Value Update requires a federation name.");
  }
  requireNonzero(
      result.requestingFederateId,
      "A process regional class Request Attribute Value Update requires a requester identity.");
  requireNonzero(
      result.objectClassHandle,
      "A process regional class Request Attribute Value Update requires an object class.");
  if (seenHandles.empty() ||
      result.requestRegionsByAttribute.empty() ||
      result.requestRegionsByAttribute.size() != seenHandles.size()) {
    throw ProcessFederationServiceProtocolError(
        "A process regional class Request Attribute Value Update requires one region set per attribute.");
  }
  for (auto const& [attributeHandle, regionHandles] :
       result.requestRegionsByAttribute) {
    if (!seenHandles.contains(attributeHandle) || regionHandles.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process regional class Request Attribute Value Update has an invalid attribute-region pair.");
    }
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationAttributeOwnershipCheckRequest(
    ProcessFederationAttributeOwnershipCheckRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership check requires a federation name.");
  }
  requireNonzero(
      request.requestingFederateId,
      "A process attribute-ownership check requires a requester identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process attribute-ownership check requires an object instance.");
  requireNonzero(
      request.attributeHandle,
      "A process attribute-ownership check requires an attribute.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.requestingFederateId);
  writer.unsigned64(request.objectInstanceHandle);
  writer.unsigned64(request.attributeHandle);
  return std::move(writer).finish();
}

ProcessFederationAttributeOwnershipCheckRequest
decodeProcessFederationAttributeOwnershipCheckRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationAttributeOwnershipCheckRequest result;
  result.federationName = reader.wideString();
  result.requestingFederateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  result.attributeHandle = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership check requires a federation name.");
  }
  requireNonzero(
      result.requestingFederateId,
      "A process attribute-ownership check requires a requester identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process attribute-ownership check requires an object instance.");
  requireNonzero(
      result.attributeHandle,
      "A process attribute-ownership check requires an attribute.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationAttributeOwnershipQueryRequest(
    ProcessFederationAttributeOwnershipQueryRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership query requires a federation name.");
  }
  requireNonzero(
      request.requestingFederateId,
      "A process attribute-ownership query requires a requester identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process attribute-ownership query requires an object instance.");
  if (request.requestedAttributeHandles.empty() ||
      request.requestedAttributeHandles.size() >
          std::numeric_limits<std::uint32_t>::max()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership query requires a bounded attribute set.");
  }
  std::set<std::uint64_t> seenHandles;
  for (std::uint64_t const attributeHandle :
       request.requestedAttributeHandles) {
    requireNonzero(
        attributeHandle,
        "A process attribute-ownership query requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership query repeats an attribute identity.");
    }
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.requestingFederateId);
  writer.unsigned64(request.objectInstanceHandle);
  writer.unsigned32(static_cast<std::uint32_t>(
      request.requestedAttributeHandles.size()));
  for (std::uint64_t const attributeHandle :
       request.requestedAttributeHandles) {
    writer.unsigned64(attributeHandle);
  }
  return std::move(writer).finish();
}

ProcessFederationAttributeOwnershipQueryRequest
decodeProcessFederationAttributeOwnershipQueryRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationAttributeOwnershipQueryRequest result;
  result.federationName = reader.wideString();
  result.requestingFederateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  auto const attributeCount = reader.unsigned32();
  result.requestedAttributeHandles.reserve(attributeCount);
  std::set<std::uint64_t> seenHandles;
  for (std::uint32_t index = 0U; index < attributeCount; ++index) {
    auto const attributeHandle = reader.unsigned64();
    requireNonzero(
        attributeHandle,
        "A process attribute-ownership query requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership query repeats an attribute identity.");
    }
    result.requestedAttributeHandles.push_back(attributeHandle);
  }
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership query requires a federation name.");
  }
  requireNonzero(
      result.requestingFederateId,
      "A process attribute-ownership query requires a requester identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process attribute-ownership query requires an object instance.");
  if (result.requestedAttributeHandles.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership query requires attributes.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationAttributeOwnershipAcquisitionIfAvailableRequest(
    ProcessFederationAttributeOwnershipAcquisitionIfAvailableRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership acquisition-if-available request requires a federation name.");
  }
  requireNonzero(
      request.requestingFederateId,
      "A process attribute-ownership acquisition-if-available request requires a requester identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process attribute-ownership acquisition-if-available request requires an object instance.");
  if (request.desiredAttributeHandles.size() >
      std::numeric_limits<std::uint32_t>::max()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership acquisition-if-available request has too many attributes.");
  }
  std::set<std::uint64_t> seenHandles;
  for (std::uint64_t const attributeHandle : request.desiredAttributeHandles) {
    requireNonzero(
        attributeHandle,
        "A process attribute-ownership acquisition-if-available request requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership acquisition-if-available request repeats an attribute identity.");
    }
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.requestingFederateId);
  writer.unsigned64(request.objectInstanceHandle);
  writer.unsigned32(static_cast<std::uint32_t>(
      request.desiredAttributeHandles.size()));
  for (std::uint64_t const attributeHandle : request.desiredAttributeHandles) {
    writer.unsigned64(attributeHandle);
  }
  writer.bytes(request.userSuppliedTag);
  return std::move(writer).finish();
}

ProcessFederationAttributeOwnershipAcquisitionIfAvailableRequest
decodeProcessFederationAttributeOwnershipAcquisitionIfAvailableRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationAttributeOwnershipAcquisitionIfAvailableRequest result;
  result.federationName = reader.wideString();
  result.requestingFederateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  auto const attributeCount = reader.unsigned32();
  result.desiredAttributeHandles.reserve(attributeCount);
  std::set<std::uint64_t> seenHandles;
  for (std::uint32_t index = 0U; index < attributeCount; ++index) {
    auto const attributeHandle = reader.unsigned64();
    requireNonzero(
        attributeHandle,
        "A process attribute-ownership acquisition-if-available request requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership acquisition-if-available request repeats an attribute identity.");
    }
    result.desiredAttributeHandles.push_back(attributeHandle);
  }
  result.userSuppliedTag = reader.bytes();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership acquisition-if-available request requires a federation name.");
  }
  requireNonzero(
      result.requestingFederateId,
      "A process attribute-ownership acquisition-if-available request requires a requester identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process attribute-ownership acquisition-if-available request requires an object instance.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationAttributeOwnershipAcquisitionRequest(
    ProcessFederationAttributeOwnershipAcquisitionRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership acquisition request requires a federation name.");
  }
  requireNonzero(
      request.requestingFederateId,
      "A process attribute-ownership acquisition request requires a requester identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process attribute-ownership acquisition request requires an object instance.");
  if (request.desiredAttributeHandles.size() >
      std::numeric_limits<std::uint32_t>::max()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership acquisition request has too many attributes.");
  }
  std::set<std::uint64_t> seenHandles;
  for (std::uint64_t const attributeHandle : request.desiredAttributeHandles) {
    requireNonzero(
        attributeHandle,
        "A process attribute-ownership acquisition request requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership acquisition request repeats an attribute identity.");
    }
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.requestingFederateId);
  writer.unsigned64(request.objectInstanceHandle);
  writer.unsigned32(static_cast<std::uint32_t>(
      request.desiredAttributeHandles.size()));
  for (std::uint64_t const attributeHandle : request.desiredAttributeHandles) {
    writer.unsigned64(attributeHandle);
  }
  writer.bytes(request.userSuppliedTag);
  writer.unsigned8(request.callbacksEnabled ? 1U : 0U);
  return std::move(writer).finish();
}

ProcessFederationAttributeOwnershipAcquisitionRequest
decodeProcessFederationAttributeOwnershipAcquisitionRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationAttributeOwnershipAcquisitionRequest result;
  result.federationName = reader.wideString();
  result.requestingFederateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  auto const attributeCount = reader.unsigned32();
  result.desiredAttributeHandles.reserve(attributeCount);
  std::set<std::uint64_t> seenHandles;
  for (std::uint32_t index = 0U; index < attributeCount; ++index) {
    auto const attributeHandle = reader.unsigned64();
    requireNonzero(
        attributeHandle,
        "A process attribute-ownership acquisition request requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership acquisition request repeats an attribute identity.");
    }
    result.desiredAttributeHandles.push_back(attributeHandle);
  }
  result.userSuppliedTag = reader.bytes();
  if (reader.remaining() != 0U) {
    result.callbacksEnabled = reader.unsigned8() != 0U;
  }
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership acquisition request requires a federation name.");
  }
  requireNonzero(
      result.requestingFederateId,
      "A process attribute-ownership acquisition request requires a requester identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process attribute-ownership acquisition request requires an object instance.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationAttributeOwnershipReleaseDeniedRequest(
    ProcessFederationAttributeOwnershipReleaseDeniedRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership release-denied request requires a federation name.");
  }
  requireNonzero(
      request.owningFederateId,
      "A process attribute-ownership release-denied request requires an owner identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process attribute-ownership release-denied request requires an object instance.");
  if (request.attributeHandles.size() >
      std::numeric_limits<std::uint32_t>::max()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership release-denied request has too many attributes.");
  }
  std::set<std::uint64_t> seenHandles;
  for (std::uint64_t const attributeHandle : request.attributeHandles) {
    requireNonzero(
        attributeHandle,
        "A process attribute-ownership release-denied request requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership release-denied request repeats an attribute identity.");
    }
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.owningFederateId);
  writer.unsigned64(request.objectInstanceHandle);
  writer.unsigned32(static_cast<std::uint32_t>(request.attributeHandles.size()));
  for (std::uint64_t const attributeHandle : request.attributeHandles) {
    writer.unsigned64(attributeHandle);
  }
  writer.bytes(request.userSuppliedTag);
  return std::move(writer).finish();
}

ProcessFederationAttributeOwnershipReleaseDeniedRequest
decodeProcessFederationAttributeOwnershipReleaseDeniedRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationAttributeOwnershipReleaseDeniedRequest result;
  result.federationName = reader.wideString();
  result.owningFederateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  auto const attributeCount = reader.unsigned32();
  result.attributeHandles.reserve(attributeCount);
  std::set<std::uint64_t> seenHandles;
  for (std::uint32_t index = 0U; index < attributeCount; ++index) {
    auto const attributeHandle = reader.unsigned64();
    requireNonzero(
        attributeHandle,
        "A process attribute-ownership release-denied request requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership release-denied request repeats an attribute identity.");
    }
    result.attributeHandles.push_back(attributeHandle);
  }
  result.userSuppliedTag = reader.bytes();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership release-denied request requires a federation name.");
  }
  requireNonzero(
      result.owningFederateId,
      "A process attribute-ownership release-denied request requires an owner identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process attribute-ownership release-denied request requires an object instance.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationAttributeOwnershipAcquisitionCancellationRequest(
    ProcessFederationAttributeOwnershipAcquisitionCancellationRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership acquisition-cancellation request requires a federation name.");
  }
  requireNonzero(
      request.requestingFederateId,
      "A process attribute-ownership acquisition-cancellation request requires a requester identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process attribute-ownership acquisition-cancellation request requires an object instance.");
  if (request.attributeHandles.size() >
      std::numeric_limits<std::uint32_t>::max()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership acquisition-cancellation request has too many attributes.");
  }
  std::set<std::uint64_t> seenHandles;
  for (std::uint64_t const attributeHandle : request.attributeHandles) {
    requireNonzero(
        attributeHandle,
        "A process attribute-ownership acquisition-cancellation request requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership acquisition-cancellation request repeats an attribute identity.");
    }
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.requestingFederateId);
  writer.unsigned64(request.objectInstanceHandle);
  writer.unsigned32(static_cast<std::uint32_t>(request.attributeHandles.size()));
  for (std::uint64_t const attributeHandle : request.attributeHandles) {
    writer.unsigned64(attributeHandle);
  }
  return std::move(writer).finish();
}

ProcessFederationAttributeOwnershipAcquisitionCancellationRequest
decodeProcessFederationAttributeOwnershipAcquisitionCancellationRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationAttributeOwnershipAcquisitionCancellationRequest result;
  result.federationName = reader.wideString();
  result.requestingFederateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  auto const attributeCount = reader.unsigned32();
  result.attributeHandles.reserve(attributeCount);
  std::set<std::uint64_t> seenHandles;
  for (std::uint32_t index = 0U; index < attributeCount; ++index) {
    auto const attributeHandle = reader.unsigned64();
    requireNonzero(
        attributeHandle,
        "A process attribute-ownership acquisition-cancellation request requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership acquisition-cancellation request repeats an attribute identity.");
    }
    result.attributeHandles.push_back(attributeHandle);
  }
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership acquisition-cancellation request requires a federation name.");
  }
  requireNonzero(
      result.requestingFederateId,
      "A process attribute-ownership acquisition-cancellation request requires a requester identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process attribute-ownership acquisition-cancellation request requires an object instance.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationCancelNegotiatedAttributeOwnershipDivestitureRequest(
    ProcessFederationCancelNegotiatedAttributeOwnershipDivestitureRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process negotiated-divestiture cancellation request requires a federation name.");
  }
  requireNonzero(
      request.divestingFederateId,
      "A process negotiated-divestiture cancellation request requires a divesting federate identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process negotiated-divestiture cancellation request requires an object instance.");
  if (request.attributeHandles.size() >
      std::numeric_limits<std::uint32_t>::max()) {
    throw ProcessFederationServiceProtocolError(
        "A process negotiated-divestiture cancellation request has too many attributes.");
  }
  std::set<std::uint64_t> seenHandles;
  for (std::uint64_t const attributeHandle : request.attributeHandles) {
    requireNonzero(
        attributeHandle,
        "A process negotiated-divestiture cancellation request requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process negotiated-divestiture cancellation request repeats an attribute identity.");
    }
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.divestingFederateId);
  writer.unsigned64(request.objectInstanceHandle);
  writer.unsigned32(static_cast<std::uint32_t>(request.attributeHandles.size()));
  for (std::uint64_t const attributeHandle : request.attributeHandles) {
    writer.unsigned64(attributeHandle);
  }
  return std::move(writer).finish();
}

ProcessFederationCancelNegotiatedAttributeOwnershipDivestitureRequest
decodeProcessFederationCancelNegotiatedAttributeOwnershipDivestitureRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationCancelNegotiatedAttributeOwnershipDivestitureRequest result;
  result.federationName = reader.wideString();
  result.divestingFederateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  auto const attributeCount = reader.unsigned32();
  result.attributeHandles.reserve(attributeCount);
  std::set<std::uint64_t> seenHandles;
  for (std::uint32_t index = 0U; index < attributeCount; ++index) {
    auto const attributeHandle = reader.unsigned64();
    requireNonzero(
        attributeHandle,
        "A process negotiated-divestiture cancellation request requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process negotiated-divestiture cancellation request repeats an attribute identity.");
    }
    result.attributeHandles.push_back(attributeHandle);
  }
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process negotiated-divestiture cancellation request requires a federation name.");
  }
  requireNonzero(
      result.divestingFederateId,
      "A process negotiated-divestiture cancellation request requires a divesting federate identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process negotiated-divestiture cancellation request requires an object instance.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationNegotiatedAttributeOwnershipDivestitureRequest(
    ProcessFederationNegotiatedAttributeOwnershipDivestitureRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process negotiated-divestiture request requires a federation name.");
  }
  requireNonzero(
      request.divestingFederateId,
      "A process negotiated-divestiture request requires a divesting federate identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process negotiated-divestiture request requires an object instance.");
  if (request.attributeHandles.size() >
      std::numeric_limits<std::uint32_t>::max()) {
    throw ProcessFederationServiceProtocolError(
        "A process negotiated-divestiture request has too many attributes.");
  }
  std::set<std::uint64_t> seenHandles;
  for (std::uint64_t const attributeHandle : request.attributeHandles) {
    requireNonzero(
        attributeHandle,
        "A process negotiated-divestiture request requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process negotiated-divestiture request repeats an attribute identity.");
    }
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.divestingFederateId);
  writer.unsigned64(request.objectInstanceHandle);
  writer.unsigned32(static_cast<std::uint32_t>(request.attributeHandles.size()));
  for (std::uint64_t const attributeHandle : request.attributeHandles) {
    writer.unsigned64(attributeHandle);
  }
  writer.bytes(request.userSuppliedTag);
  return std::move(writer).finish();
}

ProcessFederationNegotiatedAttributeOwnershipDivestitureRequest
decodeProcessFederationNegotiatedAttributeOwnershipDivestitureRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationNegotiatedAttributeOwnershipDivestitureRequest result;
  result.federationName = reader.wideString();
  result.divestingFederateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  auto const attributeCount = reader.unsigned32();
  result.attributeHandles.reserve(attributeCount);
  std::set<std::uint64_t> seenHandles;
  for (std::uint32_t index = 0U; index < attributeCount; ++index) {
    auto const attributeHandle = reader.unsigned64();
    requireNonzero(
        attributeHandle,
        "A process negotiated-divestiture request requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process negotiated-divestiture request repeats an attribute identity.");
    }
    result.attributeHandles.push_back(attributeHandle);
  }
  result.userSuppliedTag = reader.bytes();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process negotiated-divestiture request requires a federation name.");
  }
  requireNonzero(
      result.divestingFederateId,
      "A process negotiated-divestiture request requires a divesting federate identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process negotiated-divestiture request requires an object instance.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationConfirmDivestitureRequest(
    ProcessFederationConfirmDivestitureRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process Confirm Divestiture request requires a federation name.");
  }
  requireNonzero(
      request.divestingFederateId,
      "A process Confirm Divestiture request requires a divesting federate identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process Confirm Divestiture request requires an object instance.");
  if (request.attributeHandles.size() >
      std::numeric_limits<std::uint32_t>::max()) {
    throw ProcessFederationServiceProtocolError(
        "A process Confirm Divestiture request has too many attributes.");
  }
  std::set<std::uint64_t> seenHandles;
  for (std::uint64_t const attributeHandle : request.attributeHandles) {
    requireNonzero(
        attributeHandle,
        "A process Confirm Divestiture request requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process Confirm Divestiture request repeats an attribute identity.");
    }
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.divestingFederateId);
  writer.unsigned64(request.objectInstanceHandle);
  writer.unsigned32(static_cast<std::uint32_t>(request.attributeHandles.size()));
  for (std::uint64_t const attributeHandle : request.attributeHandles) {
    writer.unsigned64(attributeHandle);
  }
  writer.bytes(request.userSuppliedTag);
  return std::move(writer).finish();
}

ProcessFederationConfirmDivestitureRequest
decodeProcessFederationConfirmDivestitureRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationConfirmDivestitureRequest result;
  result.federationName = reader.wideString();
  result.divestingFederateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  auto const attributeCount = reader.unsigned32();
  result.attributeHandles.reserve(attributeCount);
  std::set<std::uint64_t> seenHandles;
  for (std::uint32_t index = 0U; index < attributeCount; ++index) {
    auto const attributeHandle = reader.unsigned64();
    requireNonzero(
        attributeHandle,
        "A process Confirm Divestiture request requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process Confirm Divestiture request repeats an attribute identity.");
    }
    result.attributeHandles.push_back(attributeHandle);
  }
  result.userSuppliedTag = reader.bytes();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process Confirm Divestiture request requires a federation name.");
  }
  requireNonzero(
      result.divestingFederateId,
      "A process Confirm Divestiture request requires a divesting federate identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process Confirm Divestiture request requires an object instance.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationGetFederateHandleRequest(
    ProcessFederationGetFederateHandleRequest const& request) {
  if (request.federationName.empty() || request.federateName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federate-handle lookup requires execution and federate names.");
  }
  requireNonzero(
      request.federateId,
      "A process federate-handle lookup requires a requesting federate identity.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.wideString(request.federateName);
  return std::move(writer).finish();
}

ProcessFederationGetFederateHandleRequest
decodeProcessFederationGetFederateHandleRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationGetFederateHandleRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.federateName = reader.wideString();
  reader.finish();
  if (result.federationName.empty() || result.federateName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federate-handle lookup requires execution and federate names.");
  }
  requireNonzero(
      result.federateId,
      "A process federate-handle lookup requires a requesting federate identity.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationGetFederateNameRequest(
    ProcessFederationGetFederateNameRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federate-name lookup requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process federate-name lookup requires a requesting federate identity.");
  requireNonzero(
      request.targetFederateId,
      "A process federate-name lookup requires a target federate identity.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.targetFederateId);
  return std::move(writer).finish();
}

ProcessFederationGetFederateNameRequest
decodeProcessFederationGetFederateNameRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationGetFederateNameRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.targetFederateId = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federate-name lookup requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process federate-name lookup requires a requesting federate identity.");
  requireNonzero(
      result.targetFederateId,
      "A process federate-name lookup requires a target federate identity.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationNormalizeHandleRequest(
    ProcessFederationNormalizeHandleRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process handle-normalization request requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process handle-normalization request requires a requesting federate identity.");
  requireNonzero(
      request.handle,
      "A process handle-normalization request requires a non-zero handle.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.handle);
  return std::move(writer).finish();
}

ProcessFederationNormalizeHandleRequest
decodeProcessFederationNormalizeHandleRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationNormalizeHandleRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.handle = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process handle-normalization request requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process handle-normalization request requires a requesting federate identity.");
  requireNonzero(
      result.handle,
      "A process handle-normalization request requires a non-zero handle.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationGetInteractionClassHandleRequest(
    ProcessFederationGetInteractionClassHandleRequest const& request) {
  if (request.federationName.empty() || request.interactionClassName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation interaction-class lookup requires execution and class names.");
  }
  requireNonzero(
      request.federateId,
      "A process federation interaction-class lookup requires a federate identity.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.wideString(request.interactionClassName);
  return std::move(writer).finish();
}

ProcessFederationGetInteractionClassHandleRequest
decodeProcessFederationGetInteractionClassHandleRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationGetInteractionClassHandleRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.interactionClassName = reader.wideString();
  reader.finish();
  if (result.federationName.empty() || result.interactionClassName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation interaction-class lookup requires execution and class names.");
  }
  requireNonzero(
      result.federateId,
      "A process federation interaction-class lookup requires a federate identity.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationGetObjectClassHandleRequest(
    ProcessFederationGetObjectClassHandleRequest const& request) {
  if (request.federationName.empty() || request.objectClassName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation object-class lookup requires execution and class names.");
  }
  requireNonzero(
      request.federateId,
      "A process federation object-class lookup requires a federate identity.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.wideString(request.objectClassName);
  writer.unsigned8(request.callbacksEnabled ? 1U : 0U);
  return std::move(writer).finish();
}

ProcessFederationGetObjectClassHandleRequest
decodeProcessFederationGetObjectClassHandleRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationGetObjectClassHandleRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectClassName = reader.wideString();
  if (reader.remaining() != 0U) {
    result.callbacksEnabled = reader.unsigned8() != 0U;
  }
  reader.finish();
  if (result.federationName.empty() || result.objectClassName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation object-class lookup requires execution and class names.");
  }
  requireNonzero(
      result.federateId,
      "A process federation object-class lookup requires a federate identity.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationGetObjectClassNameRequest(
    ProcessFederationGetObjectClassNameRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation object-class name lookup requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process federation object-class name lookup requires a federate identity.");
  requireNonzero(
      request.objectClassHandle,
      "A process federation object-class name lookup requires an object class.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.objectClassHandle);
  return std::move(writer).finish();
}

ProcessFederationGetObjectClassNameRequest
decodeProcessFederationGetObjectClassNameRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationGetObjectClassNameRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectClassHandle = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation object-class name lookup requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process federation object-class name lookup requires a federate identity.");
  requireNonzero(
      result.objectClassHandle,
      "A process federation object-class name lookup requires an object class.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationGetInteractionClassNameRequest(
    ProcessFederationGetInteractionClassNameRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation interaction-class name lookup requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process federation interaction-class name lookup requires a federate identity.");
  requireNonzero(
      request.interactionClassHandle,
      "A process federation interaction-class name lookup requires an interaction class.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.interactionClassHandle);
  return std::move(writer).finish();
}

ProcessFederationGetInteractionClassNameRequest
decodeProcessFederationGetInteractionClassNameRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationGetInteractionClassNameRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.interactionClassHandle = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation interaction-class name lookup requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process federation interaction-class name lookup requires a federate identity.");
  requireNonzero(
      result.interactionClassHandle,
      "A process federation interaction-class name lookup requires an interaction class.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationGetAttributeNameRequest(
    ProcessFederationGetAttributeNameRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation attribute-name lookup requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process federation attribute-name lookup requires a federate identity.");
  requireNonzero(
      request.objectClassHandle,
      "A process federation attribute-name lookup requires an object class.");
  requireNonzero(
      request.attributeHandle,
      "A process federation attribute-name lookup requires an attribute.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.objectClassHandle);
  writer.unsigned64(request.attributeHandle);
  return std::move(writer).finish();
}

ProcessFederationGetAttributeNameRequest
decodeProcessFederationGetAttributeNameRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationGetAttributeNameRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectClassHandle = reader.unsigned64();
  result.attributeHandle = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation attribute-name lookup requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process federation attribute-name lookup requires a federate identity.");
  requireNonzero(
      result.objectClassHandle,
      "A process federation attribute-name lookup requires an object class.");
  requireNonzero(
      result.attributeHandle,
      "A process federation attribute-name lookup requires an attribute.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationGetParameterNameRequest(
    ProcessFederationGetParameterNameRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation parameter-name lookup requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process federation parameter-name lookup requires a federate identity.");
  requireNonzero(
      request.interactionClassHandle,
      "A process federation parameter-name lookup requires an interaction class.");
  requireNonzero(
      request.parameterHandle,
      "A process federation parameter-name lookup requires a parameter.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.interactionClassHandle);
  writer.unsigned64(request.parameterHandle);
  return std::move(writer).finish();
}

ProcessFederationGetParameterNameRequest
decodeProcessFederationGetParameterNameRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationGetParameterNameRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.interactionClassHandle = reader.unsigned64();
  result.parameterHandle = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation parameter-name lookup requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process federation parameter-name lookup requires a federate identity.");
  requireNonzero(
      result.interactionClassHandle,
      "A process federation parameter-name lookup requires an interaction class.");
  requireNonzero(
      result.parameterHandle,
      "A process federation parameter-name lookup requires a parameter.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationGetParameterHandleRequest(
    ProcessFederationGetParameterHandleRequest const& request) {
  if (request.federationName.empty() || request.parameterName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation parameter lookup requires execution and parameter names.");
  }
  requireNonzero(
      request.federateId,
      "A process federation parameter lookup requires a federate identity.");
  requireNonzero(
      request.interactionClassHandle,
      "A process federation parameter lookup requires an interaction class.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.interactionClassHandle);
  writer.wideString(request.parameterName);
  return std::move(writer).finish();
}

ProcessFederationGetParameterHandleRequest
decodeProcessFederationGetParameterHandleRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationGetParameterHandleRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.interactionClassHandle = reader.unsigned64();
  result.parameterName = reader.wideString();
  reader.finish();
  if (result.federationName.empty() || result.parameterName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation parameter lookup requires execution and parameter names.");
  }
  requireNonzero(
      result.federateId,
      "A process federation parameter lookup requires a federate identity.");
  requireNonzero(
      result.interactionClassHandle,
      "A process federation parameter lookup requires an interaction class.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationGetAttributeHandleRequest(
    ProcessFederationGetAttributeHandleRequest const& request) {
  if (request.federationName.empty() || request.attributeName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation attribute lookup requires execution and attribute names.");
  }
  requireNonzero(
      request.federateId,
      "A process federation attribute lookup requires a federate identity.");
  requireNonzero(
      request.objectClassHandle,
      "A process federation attribute lookup requires an object class.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.objectClassHandle);
  writer.wideString(request.attributeName);
  return std::move(writer).finish();
}

ProcessFederationGetAttributeHandleRequest
decodeProcessFederationGetAttributeHandleRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationGetAttributeHandleRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectClassHandle = reader.unsigned64();
  result.attributeName = reader.wideString();
  reader.finish();
  if (result.federationName.empty() || result.attributeName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation attribute lookup requires execution and attribute names.");
  }
  requireNonzero(
      result.federateId,
      "A process federation attribute lookup requires a federate identity.");
  requireNonzero(
      result.objectClassHandle,
      "A process federation attribute lookup requires an object class.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationGetObjectInstanceHandleRequest(
    ProcessFederationGetObjectInstanceHandleRequest const& request) {
  if (request.federationName.empty() || request.objectInstanceName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation object-instance handle lookup requires execution and instance names.");
  }
  requireNonzero(
      request.federateId,
      "A process federation object-instance handle lookup requires a federate identity.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.wideString(request.objectInstanceName);
  return std::move(writer).finish();
}

ProcessFederationGetObjectInstanceHandleRequest
decodeProcessFederationGetObjectInstanceHandleRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationGetObjectInstanceHandleRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectInstanceName = reader.wideString();
  reader.finish();
  if (result.federationName.empty() || result.objectInstanceName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation object-instance handle lookup requires execution and instance names.");
  }
  requireNonzero(
      result.federateId,
      "A process federation object-instance handle lookup requires a federate identity.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationGetObjectInstanceNameRequest(
    ProcessFederationGetObjectInstanceNameRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation object-instance name lookup requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process federation object-instance name lookup requires a federate identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process federation object-instance name lookup requires an object instance.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.objectInstanceHandle);
  return std::move(writer).finish();
}

ProcessFederationGetObjectInstanceNameRequest
decodeProcessFederationGetObjectInstanceNameRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationGetObjectInstanceNameRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation object-instance name lookup requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process federation object-instance name lookup requires a federate identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process federation object-instance name lookup requires an object instance.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationGetKnownObjectClassHandleRequest(
    ProcessFederationGetKnownObjectClassHandleRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process known-object class lookup requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process known-object class lookup requires a federate identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process known-object class lookup requires an object instance.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.objectInstanceHandle);
  return std::move(writer).finish();
}

ProcessFederationGetKnownObjectClassHandleRequest
decodeProcessFederationGetKnownObjectClassHandleRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationGetKnownObjectClassHandleRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process known-object class lookup requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process known-object class lookup requires a federate identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process known-object class lookup requires an object instance.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationInteractionClassDeclarationRequest(
    ProcessFederationInteractionClassDeclarationRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation interaction declaration requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process federation interaction declaration requires a federate identity.");
  requireNonzero(
      request.interactionClassHandle,
      "A process federation interaction declaration requires an interaction class.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.interactionClassHandle);
  writer.unsigned8(request.active ? 1U : 0U);
  return std::move(writer).finish();
}

ProcessFederationInteractionClassDeclarationRequest
decodeProcessFederationInteractionClassDeclarationRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationInteractionClassDeclarationRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.interactionClassHandle = reader.unsigned64();
  auto const active = reader.unsigned8();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation interaction declaration requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process federation interaction declaration requires a federate identity.");
  requireNonzero(
      result.interactionClassHandle,
      "A process federation interaction declaration requires an interaction class.");
  if (active > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process federation interaction declaration has an invalid state marker.");
  }
  result.active = active != 0U;
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationInteractionClassRegionalSubscriptionRequest(
    ProcessFederationInteractionClassRegionalSubscriptionRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process regional interaction subscription requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process regional interaction subscription requires a federate identity.");
  requireNonzero(
      request.interactionClassHandle,
      "A process regional interaction subscription requires an interaction class.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.interactionClassHandle);
  writer.unsigned64Vector(parameterVector(request.regionHandles));
  writer.unsigned8(request.active ? 1U : 0U);
  return std::move(writer).finish();
}

ProcessFederationInteractionClassRegionalSubscriptionRequest
decodeProcessFederationInteractionClassRegionalSubscriptionRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationInteractionClassRegionalSubscriptionRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.interactionClassHandle = reader.unsigned64();
  auto regionVector = reader.unsigned64Vector();
  validateHandleVector(
      regionVector,
      "A process regional interaction subscription requires sorted, unique region handles.");
  result.regionHandles.insert(regionVector.begin(), regionVector.end());
  auto const active = reader.unsigned8();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process regional interaction subscription requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process regional interaction subscription requires a federate identity.");
  requireNonzero(
      result.interactionClassHandle,
      "A process regional interaction subscription requires an interaction class.");
  if (active > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process regional interaction subscription has an invalid state marker.");
  }
  result.active = active != 0U;
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationObjectClassDirectedInteractionDeclarationRequest(
    ProcessFederationObjectClassDirectedInteractionDeclarationRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process directed-interaction declaration requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process directed-interaction declaration requires a federate identity.");
  requireNonzero(
      request.objectClassHandle,
      "A process directed-interaction declaration requires an object class.");
  if (request.interactionClassHandles) {
    validateHandleVector(
        *request.interactionClassHandles,
        "A process directed-interaction declaration requires sorted, unique interaction handles.");
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.objectClassHandle);
  writer.unsigned8(request.interactionClassHandles.has_value() ? 1U : 0U);
  if (request.interactionClassHandles) {
    writer.unsigned64Vector(*request.interactionClassHandles);
  }
  writer.unsigned8(request.universally ? 1U : 0U);
  return std::move(writer).finish();
}

ProcessFederationObjectClassDirectedInteractionDeclarationRequest
decodeProcessFederationObjectClassDirectedInteractionDeclarationRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationObjectClassDirectedInteractionDeclarationRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectClassHandle = reader.unsigned64();
  auto const hasInteractionClassHandles = reader.unsigned8();
  if (hasInteractionClassHandles > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process directed-interaction declaration has an invalid set marker.");
  }
  if (hasInteractionClassHandles != 0U) {
    result.interactionClassHandles = reader.unsigned64Vector();
    validateHandleVector(
        *result.interactionClassHandles,
        "A process directed-interaction declaration requires sorted, unique interaction handles.");
  }
  auto const universally = reader.unsigned8();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process directed-interaction declaration requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process directed-interaction declaration requires a federate identity.");
  requireNonzero(
      result.objectClassHandle,
      "A process directed-interaction declaration requires an object class.");
  if (universally > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process directed-interaction declaration has an invalid universal marker.");
  }
  result.universally = universally != 0U;
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationObjectClassAttributeDeclarationRequest(
    ProcessFederationObjectClassAttributeDeclarationRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation object-class declaration requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process federation object-class declaration requires a federate identity.");
  requireNonzero(
      request.objectClassHandle,
      "A process federation object-class declaration requires an object class.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.objectClassHandle);
  writer.unsigned64Vector(request.attributeHandles);
  return std::move(writer).finish();
}

ProcessFederationObjectClassAttributeDeclarationRequest
decodeProcessFederationObjectClassAttributeDeclarationRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationObjectClassAttributeDeclarationRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectClassHandle = reader.unsigned64();
  result.attributeHandles = reader.unsigned64Vector();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation object-class declaration requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process federation object-class declaration requires a federate identity.");
  requireNonzero(
      result.objectClassHandle,
      "A process federation object-class declaration requires an object class.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationObjectClassAttributeSubscriptionRequest(
    ProcessFederationObjectClassAttributeSubscriptionRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation object-class subscription requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process federation object-class subscription requires a federate identity.");
  requireNonzero(
      request.objectClassHandle,
      "A process federation object-class subscription requires an object class.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.objectClassHandle);
  writer.unsigned64Vector(request.attributeHandles);
  writer.unsigned8(request.active ? 1U : 0U);
  writer.string(request.updateRateDesignator);
  return std::move(writer).finish();
}

ProcessFederationObjectClassAttributeSubscriptionRequest
decodeProcessFederationObjectClassAttributeSubscriptionRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationObjectClassAttributeSubscriptionRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectClassHandle = reader.unsigned64();
  result.attributeHandles = reader.unsigned64Vector();
  auto const active = reader.unsigned8();
  result.updateRateDesignator = reader.string();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation object-class subscription requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process federation object-class subscription requires a federate identity.");
  requireNonzero(
      result.objectClassHandle,
      "A process federation object-class subscription requires an object class.");
  if (active > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process federation object-class subscription has an invalid state marker.");
  }
  result.active = active != 0U;
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationObjectClassAttributeRegionalSubscriptionRequest(
    ProcessFederationObjectClassAttributeRegionalSubscriptionRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process regional object-class subscription requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process regional object-class subscription requires a federate identity.");
  requireNonzero(
      request.objectClassHandle,
      "A process regional object-class subscription requires an object class.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.objectClassHandle);
  writeAttributeRegionMap(writer, request.attributesAndRegions);
  writer.unsigned8(request.active ? 1U : 0U);
  writer.string(request.updateRateDesignator);
  return std::move(writer).finish();
}

ProcessFederationObjectClassAttributeRegionalSubscriptionRequest
decodeProcessFederationObjectClassAttributeRegionalSubscriptionRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationObjectClassAttributeRegionalSubscriptionRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectClassHandle = reader.unsigned64();
  result.attributesAndRegions = readAttributeRegionMap(reader);
  auto const active = reader.unsigned8();
  result.updateRateDesignator = reader.string();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process regional object-class subscription requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process regional object-class subscription requires a federate identity.");
  requireNonzero(
      result.objectClassHandle,
      "A process regional object-class subscription requires an object class.");
  if (active > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process regional object-class subscription has an invalid state marker.");
  }
  result.active = active != 0U;
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationRegisterObjectInstanceRequest(
    ProcessFederationRegisterObjectInstanceRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation object registration requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process federation object registration requires a federate identity.");
  requireNonzero(
      request.objectClassHandle,
      "A process federation object registration requires an object class.");
  if (request.requestedObjectInstanceName.has_value() &&
      request.requestedObjectInstanceName->empty()) {
    throw ProcessFederationServiceProtocolError(
        "A requested process object instance name cannot be empty.");
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.objectClassHandle);
  writer.unsigned8(request.requestedObjectInstanceName.has_value() ? 1U : 0U);
  if (request.requestedObjectInstanceName.has_value()) {
    writer.wideString(*request.requestedObjectInstanceName);
  }
  return std::move(writer).finish();
}

ProcessFederationRegisterObjectInstanceRequest
decodeProcessFederationRegisterObjectInstanceRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationRegisterObjectInstanceRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectClassHandle = reader.unsigned64();
  auto const hasRequestedName = reader.unsigned8();
  if (hasRequestedName > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process federation object registration has an invalid name marker.");
  }
  if (hasRequestedName != 0U) {
    result.requestedObjectInstanceName = reader.wideString();
  }
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation object registration requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process federation object registration requires a federate identity.");
  requireNonzero(
      result.objectClassHandle,
      "A process federation object registration requires an object class.");
  if (result.requestedObjectInstanceName.has_value() &&
      result.requestedObjectInstanceName->empty()) {
    throw ProcessFederationServiceProtocolError(
        "A requested process object instance name cannot be empty.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationLocalDeleteObjectInstanceRequest(
    ProcessFederationLocalDeleteObjectInstanceRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process local object deletion requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process local object deletion requires a federate identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process local object deletion requires an object instance handle.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.objectInstanceHandle);
  return std::move(writer).finish();
}

ProcessFederationLocalDeleteObjectInstanceRequest
decodeProcessFederationLocalDeleteObjectInstanceRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationLocalDeleteObjectInstanceRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process local object deletion requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process local object deletion requires a federate identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process local object deletion requires an object instance handle.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationDeleteObjectInstanceRequest(
    ProcessFederationDeleteObjectInstanceRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process object deletion requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process object deletion requires a federate identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process object deletion requires an object instance handle.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.objectInstanceHandle);
  writer.bytes(request.userSuppliedTag);
  writeOptionalLogicalTime(writer, request.timestamp);
  return std::move(writer).finish();
}

ProcessFederationDeleteObjectInstanceRequest
decodeProcessFederationDeleteObjectInstanceRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationDeleteObjectInstanceRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  result.userSuppliedTag = reader.bytes();
  result.timestamp = readOptionalLogicalTime(reader);
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process object deletion requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process object deletion requires a federate identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process object deletion requires an object instance handle.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationReserveObjectInstanceNameRequest(
    ProcessFederationReserveObjectInstanceNameRequest const& request) {
  if (request.federationName.empty() || request.objectInstanceName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process object-instance name reservation requires execution and name values.");
  }
  requireNonzero(
      request.federateId,
      "A process object-instance name reservation requires a federate identity.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.wideString(request.objectInstanceName);
  return std::move(writer).finish();
}

ProcessFederationReserveObjectInstanceNameRequest
decodeProcessFederationReserveObjectInstanceNameRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationReserveObjectInstanceNameRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectInstanceName = reader.wideString();
  reader.finish();
  if (result.federationName.empty() || result.objectInstanceName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process object-instance name reservation requires execution and name values.");
  }
  requireNonzero(
      result.federateId,
      "A process object-instance name reservation requires a federate identity.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationReserveMultipleObjectInstanceNamesRequest(
    ProcessFederationReserveMultipleObjectInstanceNamesRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process multiple object-instance name reservation requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process multiple object-instance name reservation requires a federate identity.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.wideStringVector(
      std::vector<std::wstring>(request.objectInstanceNames.begin(),
                                request.objectInstanceNames.end()));
  return std::move(writer).finish();
}

ProcessFederationReserveMultipleObjectInstanceNamesRequest
decodeProcessFederationReserveMultipleObjectInstanceNamesRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationReserveMultipleObjectInstanceNamesRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  auto const names = reader.wideStringVector();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process multiple object-instance name reservation requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process multiple object-instance name reservation requires a federate identity.");
  for (auto const& name : names) {
    if (!result.objectInstanceNames.insert(name).second) {
      throw ProcessFederationServiceProtocolError(
          "A process multiple object-instance name request contains a duplicate name.");
    }
  }
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationGetDimensionHandleRequest(
    ProcessFederationGetDimensionHandleRequest const& request) {
  if (request.federationName.empty() || request.dimensionName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process dimension lookup requires execution and dimension names.");
  }
  requireNonzero(
      request.federateId,
      "A process dimension lookup requires a federate identity.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.string(request.dimensionName);
  return std::move(writer).finish();
}

ProcessFederationGetDimensionHandleRequest
decodeProcessFederationGetDimensionHandleRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationGetDimensionHandleRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.dimensionName = reader.string();
  reader.finish();
  if (result.federationName.empty() || result.dimensionName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process dimension lookup requires execution and dimension names.");
  }
  requireNonzero(
      result.federateId,
      "A process dimension lookup requires a federate identity.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationDimensionRequest(
    ProcessFederationDimensionRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process dimension request requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process dimension request requires a federate identity.");
  requireNonzero(
      request.dimensionHandle,
      "A process dimension request requires a dimension handle.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.dimensionHandle);
  return std::move(writer).finish();
}

ProcessFederationDimensionRequest decodeProcessFederationDimensionRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationDimensionRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.dimensionHandle = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process dimension request requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process dimension request requires a federate identity.");
  requireNonzero(
      result.dimensionHandle,
      "A process dimension request requires a dimension handle.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationClassHandleRequest(
    ProcessFederationClassHandleRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process available-dimensions request requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process available-dimensions request requires a federate identity.");
  requireNonzero(
      request.classHandle,
      "A process available-dimensions request requires a class handle.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.classHandle);
  return std::move(writer).finish();
}

ProcessFederationClassHandleRequest
decodeProcessFederationClassHandleRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationClassHandleRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.classHandle = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process available-dimensions request requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process available-dimensions request requires a federate identity.");
  requireNonzero(
      result.classHandle,
      "A process available-dimensions request requires a class handle.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationGetTransportationTypeHandleRequest(
    ProcessFederationGetTransportationTypeHandleRequest const& request) {
  if (request.federationName.empty() ||
      request.transportationTypeName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process transportation lookup requires execution and transportation names.");
  }
  requireNonzero(
      request.federateId,
      "A process transportation lookup requires a federate identity.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.string(request.transportationTypeName);
  return std::move(writer).finish();
}

ProcessFederationGetTransportationTypeHandleRequest
decodeProcessFederationGetTransportationTypeHandleRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationGetTransportationTypeHandleRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.transportationTypeName = reader.string();
  reader.finish();
  if (result.federationName.empty() ||
      result.transportationTypeName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process transportation lookup requires execution and transportation names.");
  }
  requireNonzero(
      result.federateId,
      "A process transportation lookup requires a federate identity.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationTransportationTypeRequest(
    ProcessFederationTransportationTypeRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process transportation name lookup requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process transportation name lookup requires a federate identity.");
  requireNonzero(
      request.transportationTypeHandle,
      "A process transportation name lookup requires a transportation handle.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.transportationTypeHandle);
  return std::move(writer).finish();
}

ProcessFederationTransportationTypeRequest
decodeProcessFederationTransportationTypeRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationTransportationTypeRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.transportationTypeHandle = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process transportation name lookup requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process transportation name lookup requires a federate identity.");
  requireNonzero(
      result.transportationTypeHandle,
      "A process transportation name lookup requires a transportation handle.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationCreateRegionRequest(
    ProcessFederationCreateRegionRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process create-region request requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process create-region request requires a federate identity.");
  validateHandleVector(
      request.dimensionHandles,
      "A process create-region request requires sorted, unique dimension handles.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64Vector(request.dimensionHandles);
  return std::move(writer).finish();
}

ProcessFederationCreateRegionRequest decodeProcessFederationCreateRegionRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationCreateRegionRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.dimensionHandles = reader.unsigned64Vector();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process create-region request requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process create-region request requires a federate identity.");
  validateHandleVector(
      result.dimensionHandles,
      "A process create-region request requires sorted, unique dimension handles.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationRegionSetRequest(
    ProcessFederationRegionSetRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process region-set request requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process region-set request requires a federate identity.");
  validateHandleVector(
      request.regionHandles,
      "A process region-set request requires sorted, unique region handles.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64Vector(request.regionHandles);
  return std::move(writer).finish();
}

ProcessFederationRegionSetRequest decodeProcessFederationRegionSetRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationRegionSetRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.regionHandles = reader.unsigned64Vector();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process region-set request requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process region-set request requires a federate identity.");
  validateHandleVector(
      result.regionHandles,
      "A process region-set request requires sorted, unique region handles.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationRegionRequest(
    ProcessFederationRegionRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process region request requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process region request requires a federate identity.");
  requireNonzero(
      request.regionHandle,
      "A process region request requires a region handle.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.regionHandle);
  return std::move(writer).finish();
}

ProcessFederationRegionRequest decodeProcessFederationRegionRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationRegionRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.regionHandle = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process region request requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process region request requires a federate identity.");
  requireNonzero(
      result.regionHandle,
      "A process region request requires a region handle.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationGetRangeBoundsRequest(
    ProcessFederationGetRangeBoundsRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process range-bounds request requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process range-bounds request requires a federate identity.");
  requireNonzero(
      request.regionHandle,
      "A process range-bounds request requires a region handle.");
  requireNonzero(
      request.dimensionHandle,
      "A process range-bounds request requires a dimension handle.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.regionHandle);
  writer.unsigned64(request.dimensionHandle);
  return std::move(writer).finish();
}

ProcessFederationGetRangeBoundsRequest
decodeProcessFederationGetRangeBoundsRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationGetRangeBoundsRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.regionHandle = reader.unsigned64();
  result.dimensionHandle = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process range-bounds request requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process range-bounds request requires a federate identity.");
  requireNonzero(
      result.regionHandle,
      "A process range-bounds request requires a region handle.");
  requireNonzero(
      result.dimensionHandle,
      "A process range-bounds request requires a dimension handle.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationSetRangeBoundsRequest(
    ProcessFederationSetRangeBoundsRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process set-range-bounds request requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process set-range-bounds request requires a federate identity.");
  requireNonzero(
      request.regionHandle,
      "A process set-range-bounds request requires a region handle.");
  requireNonzero(
      request.dimensionHandle,
      "A process set-range-bounds request requires a dimension handle.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.regionHandle);
  writer.unsigned64(request.dimensionHandle);
  writer.unsigned64(static_cast<std::uint64_t>(request.lowerBound));
  writer.unsigned64(static_cast<std::uint64_t>(request.upperBound));
  return std::move(writer).finish();
}

ProcessFederationSetRangeBoundsRequest
decodeProcessFederationSetRangeBoundsRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationSetRangeBoundsRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.regionHandle = reader.unsigned64();
  result.dimensionHandle = reader.unsigned64();
  auto const lower = reader.unsigned64();
  auto const upper = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process set-range-bounds request requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process set-range-bounds request requires a federate identity.");
  requireNonzero(
      result.regionHandle,
      "A process set-range-bounds request requires a region handle.");
  requireNonzero(
      result.dimensionHandle,
      "A process set-range-bounds request requires a dimension handle.");
  if (lower > std::numeric_limits<unsigned long>::max() ||
      upper > std::numeric_limits<unsigned long>::max()) {
    throw ProcessFederationServiceProtocolError(
        "A process set-range-bounds request exceeds the native bound type.");
  }
  result.lowerBound = static_cast<unsigned long>(lower);
  result.upperBound = static_cast<unsigned long>(upper);
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationRegisterObjectInstanceWithRegionsRequest(
    ProcessFederationRegisterObjectInstanceWithRegionsRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process regional registration requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process regional registration requires a federate identity.");
  requireNonzero(
      request.objectClassHandle,
      "A process regional registration requires an object class.");
  if (request.requestedObjectInstanceName.has_value() &&
      request.requestedObjectInstanceName->empty()) {
    throw ProcessFederationServiceProtocolError(
        "A requested process regional object instance name cannot be empty.");
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.objectClassHandle);
  writeAttributeRegionMap(writer, request.updateRegionsByAttribute);
  writer.unsigned8(request.requestedObjectInstanceName.has_value() ? 1U : 0U);
  if (request.requestedObjectInstanceName.has_value()) {
    writer.wideString(*request.requestedObjectInstanceName);
  }
  return std::move(writer).finish();
}

ProcessFederationRegisterObjectInstanceWithRegionsRequest
decodeProcessFederationRegisterObjectInstanceWithRegionsRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationRegisterObjectInstanceWithRegionsRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectClassHandle = reader.unsigned64();
  result.updateRegionsByAttribute = readAttributeRegionMap(reader);
  auto const hasRequestedName = reader.unsigned8();
  if (hasRequestedName > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process regional registration has an invalid name marker.");
  }
  if (hasRequestedName != 0U) {
    result.requestedObjectInstanceName = reader.wideString();
  }
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process regional registration requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process regional registration requires a federate identity.");
  requireNonzero(
      result.objectClassHandle,
      "A process regional registration requires an object class.");
  if (result.requestedObjectInstanceName.has_value() &&
      result.requestedObjectInstanceName->empty()) {
    throw ProcessFederationServiceProtocolError(
        "A requested process regional object instance name cannot be empty.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationObjectInstanceRegionAssociationRequest(
    ProcessFederationObjectInstanceRegionAssociationRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process object regional association requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process object regional association requires a federate identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process object regional association requires an object instance.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.objectInstanceHandle);
  writeAttributeRegionMap(writer, request.attributesAndRegions);
  return std::move(writer).finish();
}

ProcessFederationObjectInstanceRegionAssociationRequest
decodeProcessFederationObjectInstanceRegionAssociationRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationObjectInstanceRegionAssociationRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  result.attributesAndRegions = readAttributeRegionMap(reader);
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process object regional association requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process object regional association requires a federate identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process object regional association requires an object instance.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationJoinResult(
    ProcessFederationJoinResult const& result) {
  requireNonzero(
      result.federateId,
      "A process federation join result requires a federate identity.");
  if (result.federateName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation join result requires a federate name.");
  }
  PayloadWriter writer;
  writer.unsigned64(result.federateId);
  writer.wideString(result.federateName);
  // This is an optional wire suffix for compatibility with older private
  // endpoints. New services always populate it from the committed
  // federation definition; an empty value remains decodable for legacy test
  // fixtures and is rejected by the public time-factory path below.
  if (!result.logicalTimeImplementationName.empty()) {
    writer.wideString(result.logicalTimeImplementationName);
  }
  return std::move(writer).finish();
}

ProcessFederationJoinResult decodeProcessFederationJoinResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationJoinResult result;
  result.federateId = reader.unsigned64();
  result.federateName = reader.wideString();
  if (reader.remaining() != 0U) {
    result.logicalTimeImplementationName = reader.wideString();
  }
  reader.finish();
  requireNonzero(
      result.federateId,
      "A process federation join result requires a federate identity.");
  if (result.federateName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation join result requires a federate name.");
  }
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationQueryLogicalTimeResult(
    ProcessFederationQueryLogicalTimeResult const& result) {
  if (result.time.implementationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process logical-time result requires an implementation name.");
  }
  PayloadWriter writer;
  writer.wideString(result.time.implementationName);
  writer.bytes(result.time.encoding);
  return std::move(writer).finish();
}

ProcessFederationQueryLogicalTimeResult
decodeProcessFederationQueryLogicalTimeResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationQueryLogicalTimeResult result;
  result.time.implementationName = reader.wideString();
  result.time.encoding = reader.bytes();
  reader.finish();
  if (result.time.implementationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process logical-time result requires an implementation name.");
  }
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationQueryLookaheadResult(
    ProcessFederationQueryLookaheadResult const& result) {
  auto const status = static_cast<std::uint8_t>(result.status);
  if (status > static_cast<std::uint8_t>(
                   ProcessFederationLookaheadStatus::inactive)) {
    throw ProcessFederationServiceProtocolError(
        "A process lookahead result has an unknown status.");
  }
  bool const applied =
      result.status == ProcessFederationLookaheadStatus::applied;
  if (result.lookahead.has_value() != applied) {
    throw ProcessFederationServiceProtocolError(
        "A process lookahead result has an inconsistent interval marker.");
  }
  PayloadWriter writer;
  writer.unsigned8(status);
  writer.unsigned8(result.lookahead.has_value() ? 1U : 0U);
  if (result.lookahead) {
    if (result.lookahead->implementationName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process lookahead result requires a logical-time implementation.");
    }
    writer.wideString(result.lookahead->implementationName);
    writer.bytes(result.lookahead->encoding);
  }
  return std::move(writer).finish();
}

ProcessFederationQueryLookaheadResult
decodeProcessFederationQueryLookaheadResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const rawStatus = reader.unsigned8();
  if (rawStatus > static_cast<std::uint8_t>(
                      ProcessFederationLookaheadStatus::inactive)) {
    throw ProcessFederationServiceProtocolError(
        "A process lookahead result has an unknown status.");
  }
  auto const marker = reader.unsigned8();
  if (marker > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process lookahead result has an invalid interval marker.");
  }
  ProcessFederationQueryLookaheadResult result;
  result.status = static_cast<ProcessFederationLookaheadStatus>(rawStatus);
  bool const applied =
      result.status == ProcessFederationLookaheadStatus::applied;
  if ((marker != 0U) != applied) {
    throw ProcessFederationServiceProtocolError(
        "A process lookahead result has an inconsistent interval marker.");
  }
  if (marker != 0U) {
    ProcessFederationLogicalTimeInterval lookahead;
    lookahead.implementationName = reader.wideString();
    lookahead.encoding = reader.bytes();
    if (lookahead.implementationName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process lookahead result requires a logical-time implementation.");
    }
    result.lookahead = std::move(lookahead);
  }
  reader.finish();
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationModifyLookaheadResult(
    ProcessFederationModifyLookaheadResult const& result) {
  auto const rawStatus = static_cast<std::uint8_t>(result.status);
  if (rawStatus > static_cast<std::uint8_t>(
                      ProcessFederationModifyLookaheadStatus::invalid_lookahead)) {
    throw ProcessFederationServiceProtocolError(
        "A process Modify Lookahead result has an unknown status.");
  }
  PayloadWriter writer;
  writer.unsigned8(rawStatus);
  return std::move(writer).finish();
}

ProcessFederationModifyLookaheadResult
decodeProcessFederationModifyLookaheadResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const rawStatus = reader.unsigned8();
  if (rawStatus > static_cast<std::uint8_t>(
                      ProcessFederationModifyLookaheadStatus::invalid_lookahead)) {
    throw ProcessFederationServiceProtocolError(
        "A process Modify Lookahead result has an unknown status.");
  }
  ProcessFederationModifyLookaheadResult result;
  result.status = static_cast<ProcessFederationModifyLookaheadStatus>(rawStatus);
  reader.finish();
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationQueryTimeBoundsResult(
    ProcessFederationQueryTimeBoundsResult const& result) {
  auto const rawStatus = static_cast<std::uint8_t>(result.status);
  if (rawStatus > static_cast<std::uint8_t>(
                      ProcessFederationTimeBoundStatus::inconsistent_temporal_state)) {
    throw ProcessFederationServiceProtocolError(
        "A process time-bounds result has an unknown status.");
  }
  bool const available =
      result.status == ProcessFederationTimeBoundStatus::available;
  bool const undefined =
      result.status == ProcessFederationTimeBoundStatus::undefined;
  if (available && (!result.galt || !result.lits)) {
    throw ProcessFederationServiceProtocolError(
        "An available process time-bounds result requires GALT and LITS.");
  }
  if (!available && result.galt) {
    throw ProcessFederationServiceProtocolError(
        "A non-available process time-bounds result cannot carry GALT.");
  }
  if (!available && !undefined && result.lits) {
    throw ProcessFederationServiceProtocolError(
        "An error process time-bounds result cannot carry LITS.");
  }
  PayloadWriter writer;
  writer.unsigned8(rawStatus);
  writeOptionalLogicalTime(writer, result.galt);
  writeOptionalLogicalTime(writer, result.lits);
  return std::move(writer).finish();
}

ProcessFederationQueryTimeBoundsResult
decodeProcessFederationQueryTimeBoundsResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const rawStatus = reader.unsigned8();
  if (rawStatus > static_cast<std::uint8_t>(
                      ProcessFederationTimeBoundStatus::inconsistent_temporal_state)) {
    throw ProcessFederationServiceProtocolError(
        "A process time-bounds result has an unknown status.");
  }
  ProcessFederationQueryTimeBoundsResult result;
  result.status = static_cast<ProcessFederationTimeBoundStatus>(rawStatus);
  result.galt = readOptionalLogicalTime(reader);
  result.lits = readOptionalLogicalTime(reader);
  reader.finish();
  bool const available =
      result.status == ProcessFederationTimeBoundStatus::available;
  bool const undefined =
      result.status == ProcessFederationTimeBoundStatus::undefined;
  if (available && (!result.galt || !result.lits)) {
    throw ProcessFederationServiceProtocolError(
        "An available process time-bounds result requires GALT and LITS.");
  }
  if (!available && result.galt) {
    throw ProcessFederationServiceProtocolError(
        "A non-available process time-bounds result cannot carry GALT.");
  }
  if (!available && !undefined && result.lits) {
    throw ProcessFederationServiceProtocolError(
        "An error process time-bounds result cannot carry LITS.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationEnableTimeRegulationResult(
    ProcessFederationEnableTimeRegulationResult const& result) {
  auto const status = static_cast<std::uint8_t>(result.status);
  if (status > static_cast<std::uint8_t>(
                   ProcessFederationTimeEnableStatus::generation_exhausted)) {
    throw ProcessFederationServiceProtocolError(
        "A process time-regulation result has an unknown status.");
  }
  bool const applied =
      result.status == ProcessFederationTimeEnableStatus::applied;
  if (result.enabledTime.has_value() != applied) {
    throw ProcessFederationServiceProtocolError(
        "A process time-regulation result has an invalid enabled-time marker.");
  }
  PayloadWriter writer;
  writer.unsigned8(status);
  writer.unsigned8(result.enabledTime.has_value() ? 1U : 0U);
  if (result.enabledTime) {
    if (result.enabledTime->implementationName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process time-regulation result requires a logical-time implementation.");
    }
    writer.wideString(result.enabledTime->implementationName);
    writer.bytes(result.enabledTime->encoding);
  }
  return std::move(writer).finish();
}

ProcessFederationEnableTimeRegulationResult
decodeProcessFederationEnableTimeRegulationResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const rawStatus = reader.unsigned8();
  if (rawStatus > static_cast<std::uint8_t>(
                      ProcessFederationTimeEnableStatus::generation_exhausted)) {
    throw ProcessFederationServiceProtocolError(
        "A process time-regulation result has an unknown status.");
  }
  auto const marker = reader.unsigned8();
  if (marker > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process time-regulation result has an invalid enabled-time marker.");
  }
  ProcessFederationEnableTimeRegulationResult result;
  result.status = static_cast<ProcessFederationTimeEnableStatus>(rawStatus);
  bool const applied =
      result.status == ProcessFederationTimeEnableStatus::applied;
  if ((marker != 0U) != applied) {
    throw ProcessFederationServiceProtocolError(
        "A process time-regulation result has an inconsistent enabled-time marker.");
  }
  if (marker != 0U) {
    ProcessFederationLogicalTime enabledTime;
    enabledTime.implementationName = reader.wideString();
    enabledTime.encoding = reader.bytes();
    if (enabledTime.implementationName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process time-regulation result requires a logical-time implementation.");
    }
    result.enabledTime = std::move(enabledTime);
  }
  reader.finish();
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationEnableTimeConstrainedResult(
    ProcessFederationEnableTimeConstrainedResult const& result) {
  auto const status = static_cast<std::uint8_t>(result.status);
  if (status > static_cast<std::uint8_t>(
                   ProcessFederationTimeEnableStatus::generation_exhausted)) {
    throw ProcessFederationServiceProtocolError(
        "A process time-constrained result has an unknown status.");
  }
  bool const applied =
      result.status == ProcessFederationTimeEnableStatus::applied;
  if (result.enabledTime.has_value() != applied) {
    throw ProcessFederationServiceProtocolError(
        "A process time-constrained result has an invalid enabled-time marker.");
  }
  PayloadWriter writer;
  writer.unsigned8(status);
  writer.unsigned8(result.enabledTime.has_value() ? 1U : 0U);
  if (result.enabledTime) {
    if (result.enabledTime->implementationName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process time-constrained result requires a logical-time implementation.");
    }
    writer.wideString(result.enabledTime->implementationName);
    writer.bytes(result.enabledTime->encoding);
  }
  return std::move(writer).finish();
}

ProcessFederationEnableTimeConstrainedResult
decodeProcessFederationEnableTimeConstrainedResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const rawStatus = reader.unsigned8();
  if (rawStatus > static_cast<std::uint8_t>(
                      ProcessFederationTimeEnableStatus::generation_exhausted)) {
    throw ProcessFederationServiceProtocolError(
        "A process time-constrained result has an unknown status.");
  }
  auto const marker = reader.unsigned8();
  if (marker > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process time-constrained result has an invalid enabled-time marker.");
  }
  ProcessFederationEnableTimeConstrainedResult result;
  result.status = static_cast<ProcessFederationTimeEnableStatus>(rawStatus);
  bool const applied =
      result.status == ProcessFederationTimeEnableStatus::applied;
  if ((marker != 0U) != applied) {
    throw ProcessFederationServiceProtocolError(
        "A process time-constrained result has an inconsistent enabled-time marker.");
  }
  if (marker != 0U) {
    ProcessFederationLogicalTime enabledTime;
    enabledTime.implementationName = reader.wideString();
    enabledTime.encoding = reader.bytes();
    if (enabledTime.implementationName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process time-constrained result requires a logical-time implementation.");
    }
    result.enabledTime = std::move(enabledTime);
  }
  reader.finish();
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationTimeDisableResult(
    ProcessFederationTimeDisableResult const& result) {
  auto const status = static_cast<std::uint8_t>(result.status);
  if (status > static_cast<std::uint8_t>(
                   ProcessFederationTimeDisableStatus::inactive)) {
    throw ProcessFederationServiceProtocolError(
        "A process time-disable result has an unknown status.");
  }
  PayloadWriter writer;
  writer.unsigned8(status);
  return std::move(writer).finish();
}

ProcessFederationTimeDisableResult decodeProcessFederationTimeDisableResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const rawStatus = reader.unsigned8();
  if (rawStatus > static_cast<std::uint8_t>(
                      ProcessFederationTimeDisableStatus::inactive)) {
    throw ProcessFederationServiceProtocolError(
        "A process time-disable result has an unknown status.");
  }
  ProcessFederationTimeDisableResult result;
  result.status = static_cast<ProcessFederationTimeDisableStatus>(rawStatus);
  reader.finish();
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationTimeAdvanceResult(
    ProcessFederationTimeAdvanceResult const& result) {
  auto const status = static_cast<std::uint8_t>(result.status);
  if (status > static_cast<std::uint8_t>(
                   ProcessFederationTimeAdvanceStatus::generation_exhausted)) {
    throw ProcessFederationServiceProtocolError(
        "A process time-advance result has an unknown status.");
  }
  bool const applied =
      result.status == ProcessFederationTimeAdvanceStatus::applied;
  if (result.grantedTime && !applied) {
    throw ProcessFederationServiceProtocolError(
        "A process time-advance result has an invalid grant-time marker.");
  }
  if (result.optimisticTime && (!applied || !result.grantedTime)) {
    throw ProcessFederationServiceProtocolError(
        "A process Flush Queue Grant has an invalid optimistic-time marker.");
  }
  if (result.optimisticTime &&
      result.optimisticTime->implementationName !=
          result.grantedTime->implementationName) {
    throw ProcessFederationServiceProtocolError(
        "A process Flush Queue Grant uses mismatched logical-time implementations.");
  }
  PayloadWriter writer;
  writer.unsigned8(status);
  writer.unsigned8(result.grantedTime.has_value() ? 1U : 0U);
  if (result.grantedTime) {
    if (result.grantedTime->implementationName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process time-advance result requires a logical-time implementation.");
    }
    writer.wideString(result.grantedTime->implementationName);
    writer.bytes(result.grantedTime->encoding);
  }
  // The optional suffix preserves the original ordinary TAR wire shape while
  // allowing a Flush Queue Grant event to carry its second callback value.
  if (result.optimisticTime) {
    writer.unsigned8(1U);
    writer.wideString(result.optimisticTime->implementationName);
    writer.bytes(result.optimisticTime->encoding);
  }
  return std::move(writer).finish();
}

ProcessFederationTimeAdvanceResult decodeProcessFederationTimeAdvanceResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const rawStatus = reader.unsigned8();
  if (rawStatus > static_cast<std::uint8_t>(
                      ProcessFederationTimeAdvanceStatus::generation_exhausted)) {
    throw ProcessFederationServiceProtocolError(
        "A process time-advance result has an unknown status.");
  }
  auto const marker = reader.unsigned8();
  if (marker > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process time-advance result has an invalid grant-time marker.");
  }
  ProcessFederationTimeAdvanceResult result;
  result.status = static_cast<ProcessFederationTimeAdvanceStatus>(rawStatus);
  bool const applied =
      result.status == ProcessFederationTimeAdvanceStatus::applied;
  if (marker != 0U && !applied) {
    throw ProcessFederationServiceProtocolError(
        "A process time-advance result has an inconsistent grant-time marker.");
  }
  if (marker != 0U) {
    ProcessFederationLogicalTime grantedTime;
    grantedTime.implementationName = reader.wideString();
    grantedTime.encoding = reader.bytes();
    if (grantedTime.implementationName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process time-advance result requires a logical-time implementation.");
    }
    result.grantedTime = std::move(grantedTime);
  }
  if (reader.remaining() != 0U) {
    auto const optimisticMarker = reader.unsigned8();
    if (optimisticMarker != 1U || !result.grantedTime || !applied) {
      throw ProcessFederationServiceProtocolError(
          "A process Flush Queue Grant has an invalid optimistic-time marker.");
    }
    ProcessFederationLogicalTime optimisticTime;
    optimisticTime.implementationName = reader.wideString();
    optimisticTime.encoding = reader.bytes();
    if (optimisticTime.implementationName.empty() ||
        optimisticTime.implementationName !=
            result.grantedTime->implementationName) {
      throw ProcessFederationServiceProtocolError(
          "A process Flush Queue Grant uses an invalid optimistic logical time.");
    }
    result.optimisticTime = std::move(optimisticTime);
  }
  reader.finish();
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationSendInteractionResult(
    ProcessFederationSendInteractionResult const& result) {
  if (result.messageId == std::numeric_limits<std::uint64_t>::max()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation interaction result has an invalid message identity.");
  }
  PayloadWriter writer;
  writer.unsigned32(result.recipientCount);
  // The trailing identity is optional on decode so older private peers that
  // only returned a recipient count remain wire-compatible.
  writer.unsigned64(result.messageId);
  return std::move(writer).finish();
}

ProcessFederationSendInteractionResult
decodeProcessFederationSendInteractionResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationSendInteractionResult result;
  result.recipientCount = reader.unsigned32();
  if (reader.remaining() != 0U) {
    result.messageId = reader.unsigned64();
  }
  reader.finish();
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationRetractResult(
    ProcessFederationRetractResult const& result) {
  if (result.status > ProcessFederationRetractStatus::time_regulation_not_enabled) {
    throw ProcessFederationServiceProtocolError(
        "A process retraction result has an invalid status.");
  }
  PayloadWriter writer;
  writer.unsigned8(static_cast<std::uint8_t>(result.status));
  return std::move(writer).finish();
}

ProcessFederationRetractResult decodeProcessFederationRetractResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const status = reader.unsigned8();
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   ProcessFederationRetractStatus::time_regulation_not_enabled)) {
    throw ProcessFederationServiceProtocolError(
        "A process retraction result has an invalid status.");
  }
  return ProcessFederationRetractResult{
      static_cast<ProcessFederationRetractStatus>(status)};
}

std::vector<std::uint8_t>
encodeProcessFederationTsoDeliveryAcknowledgementResult(
    ProcessFederationTsoDeliveryAcknowledgementResult const& result) {
  if (result.status >
      ProcessFederationTsoDeliveryAcknowledgementStatus::not_in_transit) {
    throw ProcessFederationServiceProtocolError(
        "A process TSO delivery acknowledgement result has an invalid status.");
  }
  PayloadWriter writer;
  writer.unsigned8(static_cast<std::uint8_t>(result.status));
  return std::move(writer).finish();
}

ProcessFederationTsoDeliveryAcknowledgementResult
decodeProcessFederationTsoDeliveryAcknowledgementResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const status = reader.unsigned8();
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   ProcessFederationTsoDeliveryAcknowledgementStatus::
                       not_in_transit)) {
    throw ProcessFederationServiceProtocolError(
        "A process TSO delivery acknowledgement result has an invalid status.");
  }
  return ProcessFederationTsoDeliveryAcknowledgementResult{
      static_cast<ProcessFederationTsoDeliveryAcknowledgementStatus>(status)};
}

std::vector<std::uint8_t> encodeProcessFederationRequestRetractionEvent(
    ProcessFederationRequestRetractionEvent const& event) {
  requireNonzero(
      event.messageId,
      "A process retraction event requires a message identity.");
  PayloadWriter writer;
  writer.unsigned64(event.messageId);
  return std::move(writer).finish();
}

ProcessFederationRequestRetractionEvent
decodeProcessFederationRequestRetractionEvent(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationRequestRetractionEvent result{reader.unsigned64()};
  reader.finish();
  requireNonzero(
      result.messageId,
      "A process retraction event requires a message identity.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationHandleResult(
    ProcessFederationHandleResult const& result) {
  requireNonzero(
      result.handle,
      "A process federation handle result requires a non-zero handle.");
  PayloadWriter writer;
  writer.unsigned64(result.handle);
  return std::move(writer).finish();
}

ProcessFederationHandleResult decodeProcessFederationHandleResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationHandleResult result{reader.unsigned64()};
  reader.finish();
  requireNonzero(
      result.handle,
      "A process federation handle result requires a non-zero handle.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationStringResult(
    ProcessFederationStringResult const& result) {
  if (result.value.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation string result requires a non-empty value.");
  }
  PayloadWriter writer;
  writer.wideString(result.value);
  return std::move(writer).finish();
}

ProcessFederationStringResult decodeProcessFederationStringResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationStringResult result;
  result.value = reader.wideString();
  reader.finish();
  if (result.value.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation string result requires a non-empty value.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationRegisterObjectInstanceResult(
    ProcessFederationRegisterObjectInstanceResult const& result) {
  auto const status = static_cast<std::uint8_t>(result.status);
  if (status > static_cast<std::uint8_t>(
                   ObjectInstanceRegistrationStatus::inconsistent_catalog)) {
    throw ProcessFederationServiceProtocolError(
        "A process federation object registration result has an invalid status.");
  }
  if (result.status == ObjectInstanceRegistrationStatus::applied) {
    requireNonzero(
        result.objectInstanceHandle,
        "A process federation object registration result requires a non-zero handle.");
    if (result.objectInstanceName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process federation object registration result requires an instance name.");
    }
  } else if (result.objectInstanceHandle != 0U ||
             !result.objectInstanceName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A failed process federation object registration result cannot carry an identity.");
  }
  PayloadWriter writer;
  writer.unsigned64(result.objectInstanceHandle);
  writer.wideString(result.objectInstanceName);
  writer.unsigned8(status);
  return std::move(writer).finish();
}

ProcessFederationRegisterObjectInstanceResult
decodeProcessFederationRegisterObjectInstanceResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationRegisterObjectInstanceResult result;
  result.objectInstanceHandle = reader.unsigned64();
  result.objectInstanceName = reader.wideString();
  auto const status = reader.unsigned8();
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   ObjectInstanceRegistrationStatus::inconsistent_catalog)) {
    throw ProcessFederationServiceProtocolError(
        "A process federation object registration result has an invalid status.");
  }
  result.status = static_cast<ObjectInstanceRegistrationStatus>(status);
  if (result.status == ObjectInstanceRegistrationStatus::applied) {
    requireNonzero(
        result.objectInstanceHandle,
        "A process federation object registration result requires a non-zero handle.");
    if (result.objectInstanceName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process federation object registration result requires an instance name.");
    }
  } else if (result.objectInstanceHandle != 0U ||
             !result.objectInstanceName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A failed process federation object registration result cannot carry an identity.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationReserveObjectInstanceNameResult(
    ProcessFederationReserveObjectInstanceNameResult const& result) {
  if (result.objectInstanceName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process object-instance name reservation result requires a name.");
  }
  PayloadWriter writer;
  writer.unsigned8(result.succeeded ? 1U : 0U);
  writer.wideString(result.objectInstanceName);
  auto const status = static_cast<std::uint8_t>(result.status);
  if (status > static_cast<std::uint8_t>(
                   ObjectInstanceNameReservationStatus::callback_route_missing)) {
    throw ProcessFederationServiceProtocolError(
        "A process object-instance name reservation result has an invalid status.");
  }
  if (result.status != ObjectInstanceNameReservationStatus::applied &&
      result.succeeded) {
    throw ProcessFederationServiceProtocolError(
        "A failed process object-instance name reservation cannot succeed.");
  }
  writer.unsigned8(status);
  return std::move(writer).finish();
}

ProcessFederationReserveObjectInstanceNameResult
decodeProcessFederationReserveObjectInstanceNameResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationReserveObjectInstanceNameResult result;
  auto const succeeded = reader.unsigned8();
  if (succeeded > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process object-instance name reservation result has an invalid outcome marker.");
  }
  result.succeeded = succeeded != 0U;
  result.objectInstanceName = reader.wideString();
  auto const status = reader.unsigned8();
  reader.finish();
  if (result.objectInstanceName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process object-instance name reservation result requires a name.");
  }
  if (status > static_cast<std::uint8_t>(
                   ObjectInstanceNameReservationStatus::callback_route_missing)) {
    throw ProcessFederationServiceProtocolError(
        "A process object-instance name reservation result has an invalid status.");
  }
  result.status = static_cast<ObjectInstanceNameReservationStatus>(status);
  if (result.status != ObjectInstanceNameReservationStatus::applied &&
      result.succeeded) {
    throw ProcessFederationServiceProtocolError(
        "A failed process object-instance name reservation cannot succeed.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationObjectInstanceNameReleaseResult(
    ProcessFederationObjectInstanceNameReleaseResult const& result) {
  auto const status = static_cast<std::uint8_t>(result.status);
  if (status > static_cast<std::uint8_t>(
                   ObjectInstanceNameReservationStatus::callback_route_missing)) {
    throw ProcessFederationServiceProtocolError(
        "A process object-instance name release result has an invalid status.");
  }
  PayloadWriter writer;
  writer.unsigned8(status);
  return std::move(writer).finish();
}

ProcessFederationObjectInstanceNameReleaseResult
decodeProcessFederationObjectInstanceNameReleaseResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const status = reader.unsigned8();
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   ObjectInstanceNameReservationStatus::callback_route_missing)) {
    throw ProcessFederationServiceProtocolError(
        "A process object-instance name release result has an invalid status.");
  }
  return ProcessFederationObjectInstanceNameReleaseResult{
      static_cast<ObjectInstanceNameReservationStatus>(status)};
}

std::vector<std::uint8_t>
encodeProcessFederationReserveMultipleObjectInstanceNamesResult(
    ProcessFederationReserveMultipleObjectInstanceNamesResult const& result) {
  auto const status = static_cast<std::uint8_t>(result.status);
  if (status > static_cast<std::uint8_t>(
                   ObjectInstanceNameReservationStatus::callback_route_missing)) {
    throw ProcessFederationServiceProtocolError(
        "A process multiple object-instance name reservation result has an invalid status.");
  }
  PayloadWriter writer;
  writer.unsigned8(status);
  writer.wideStringVector(
      std::vector<std::wstring>(result.succeededNames.begin(),
                                result.succeededNames.end()));
  writer.wideStringVector(
      std::vector<std::wstring>(result.failedNames.begin(),
                                result.failedNames.end()));
  return std::move(writer).finish();
}

ProcessFederationReserveMultipleObjectInstanceNamesResult
decodeProcessFederationReserveMultipleObjectInstanceNamesResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationReserveMultipleObjectInstanceNamesResult result;
  auto const status = reader.unsigned8();
  auto const succeededNames = reader.wideStringVector();
  auto const failedNames = reader.wideStringVector();
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   ObjectInstanceNameReservationStatus::callback_route_missing)) {
    throw ProcessFederationServiceProtocolError(
        "A process multiple object-instance name reservation result has an invalid status.");
  }
  result.status = static_cast<ObjectInstanceNameReservationStatus>(status);
  for (auto const& name : succeededNames) {
    if (!result.succeededNames.insert(name).second) {
      throw ProcessFederationServiceProtocolError(
          "A process multiple object-instance name reservation result contains a duplicate success name.");
    }
  }
  for (auto const& name : failedNames) {
    if (!result.failedNames.insert(name).second ||
        result.succeededNames.contains(name)) {
      throw ProcessFederationServiceProtocolError(
          "A process multiple object-instance name reservation result contains overlapping names.");
    }
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationReleaseMultipleObjectInstanceNamesResult(
    ProcessFederationReleaseMultipleObjectInstanceNamesResult const& result) {
  auto const status = static_cast<std::uint8_t>(result.status);
  if (status > static_cast<std::uint8_t>(
                   ObjectInstanceNameReservationStatus::callback_route_missing)) {
    throw ProcessFederationServiceProtocolError(
        "A process multiple object-instance name release result has an invalid status.");
  }
  PayloadWriter writer;
  writer.unsigned8(status);
  return std::move(writer).finish();
}

ProcessFederationReleaseMultipleObjectInstanceNamesResult
decodeProcessFederationReleaseMultipleObjectInstanceNamesResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const status = reader.unsigned8();
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   ObjectInstanceNameReservationStatus::callback_route_missing)) {
    throw ProcessFederationServiceProtocolError(
        "A process multiple object-instance name release result has an invalid status.");
  }
  return ProcessFederationReleaseMultipleObjectInstanceNamesResult{
      static_cast<ObjectInstanceNameReservationStatus>(status)};
}

std::vector<std::uint8_t> encodeProcessFederationRegionStatusResult(
    ProcessFederationRegionStatusResult const& result) {
  PayloadWriter writer;
  writeRegionServiceStatus(writer, result.status);
  return std::move(writer).finish();
}

ProcessFederationRegionStatusResult decodeProcessFederationRegionStatusResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationRegionStatusResult result;
  result.status = readRegionServiceStatus(reader);
  reader.finish();
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationLocalDeleteObjectInstanceResult(
    ProcessFederationLocalDeleteObjectInstanceResult const& result) {
  auto const encoded = static_cast<std::uint8_t>(result.status);
  if (encoded > static_cast<std::uint8_t>(
                   LocalObjectInstanceDeletionStatus::federate_owns_attributes)) {
    throw ProcessFederationServiceProtocolError(
        "A process local object deletion result has an invalid status.");
  }
  PayloadWriter writer;
  writer.unsigned8(encoded);
  return std::move(writer).finish();
}

ProcessFederationLocalDeleteObjectInstanceResult
decodeProcessFederationLocalDeleteObjectInstanceResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const status = reader.unsigned8();
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   LocalObjectInstanceDeletionStatus::federate_owns_attributes)) {
    throw ProcessFederationServiceProtocolError(
        "A process local object deletion result has an invalid status.");
  }
  return {static_cast<LocalObjectInstanceDeletionStatus>(status)};
}

std::vector<std::uint8_t> encodeProcessFederationDeleteObjectInstanceResult(
    ProcessFederationDeleteObjectInstanceResult const& result) {
  auto const encoded = static_cast<std::uint8_t>(result.status);
  if (encoded > static_cast<std::uint8_t>(
                   ObjectInstanceDeletionStatus::inconsistent_catalog)) {
    throw ProcessFederationServiceProtocolError(
        "A process object deletion result has an invalid status.");
  }
  if (result.status != ObjectInstanceDeletionStatus::applied &&
      result.recipientCount != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process object deletion result cannot carry recipients.");
  }
  PayloadWriter writer;
  writer.unsigned8(encoded);
  writer.unsigned32(result.recipientCount);
  writeOptionalMessageId(writer, result.messageId);
  return std::move(writer).finish();
}

ProcessFederationDeleteObjectInstanceResult
decodeProcessFederationDeleteObjectInstanceResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const status = reader.unsigned8();
  auto const recipientCount = reader.unsigned32();
  auto const messageId = readOptionalMessageId(reader);
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   ObjectInstanceDeletionStatus::inconsistent_catalog)) {
    throw ProcessFederationServiceProtocolError(
        "A process object deletion result has an invalid status.");
  }
  if (status != static_cast<std::uint8_t>(ObjectInstanceDeletionStatus::applied) &&
      recipientCount != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process object deletion result cannot carry recipients.");
  }
  return {
      static_cast<ObjectInstanceDeletionStatus>(status), recipientCount, messageId};
}

std::vector<std::uint8_t>
encodeProcessFederationObjectInstanceRegionAssociationResult(
    ProcessFederationObjectInstanceRegionAssociationResult const& result) {
  PayloadWriter writer;
  writeObjectInstanceRegionAssociationStatus(writer, result.status);
  return std::move(writer).finish();
}

ProcessFederationObjectInstanceRegionAssociationResult
decodeProcessFederationObjectInstanceRegionAssociationResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationObjectInstanceRegionAssociationResult result;
  result.status = readObjectInstanceRegionAssociationStatus(reader);
  reader.finish();
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationAttributeScopeAdvisorySwitchRequest(
    ProcessFederationAttributeScopeAdvisorySwitchRequest const& request) {
  if (request.federationName.empty() || request.federateId == 0U) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-scope switch request requires federation and federate identities.");
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned8(request.switchValue ? 1U : 0U);
  return std::move(writer).finish();
}

ProcessFederationAttributeScopeAdvisorySwitchRequest
decodeProcessFederationAttributeScopeAdvisorySwitchRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationAttributeScopeAdvisorySwitchRequest request;
  request.federationName = reader.wideString();
  request.federateId = reader.unsigned64();
  auto const switchValue = reader.unsigned8();
  if (switchValue > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-scope switch request has an invalid boolean value.");
  }
  request.switchValue = switchValue != 0U;
  reader.finish();
  if (request.federationName.empty() || request.federateId == 0U) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-scope switch request requires federation and federate identities.");
  }
  return request;
}

std::vector<std::uint8_t> encodeProcessFederationBooleanResult(
    ProcessFederationBooleanResult const& result) {
  PayloadWriter writer;
  writer.unsigned8(result.value ? 1U : 0U);
  return std::move(writer).finish();
}

ProcessFederationBooleanResult decodeProcessFederationBooleanResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const value = reader.unsigned8();
  if (value > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process boolean result has an invalid value.");
  }
  reader.finish();
  return ProcessFederationBooleanResult{value != 0U};
}

std::vector<std::uint8_t>
encodeProcessFederationInteractionOrderTypeChangeResult(
    ProcessFederationInteractionOrderTypeChangeResult const& result) {
  auto const encoded = static_cast<std::uint8_t>(result.status);
  if (encoded > static_cast<std::uint8_t>(
                    InteractionOrderTypeChangeStatus::invalid_order_type)) {
    throw ProcessFederationServiceProtocolError(
        "A process interaction order-type change result has an invalid status.");
  }
  PayloadWriter writer;
  writer.unsigned8(encoded);
  return std::move(writer).finish();
}

ProcessFederationInteractionOrderTypeChangeResult
decodeProcessFederationInteractionOrderTypeChangeResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const status = reader.unsigned8();
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   InteractionOrderTypeChangeStatus::invalid_order_type)) {
    throw ProcessFederationServiceProtocolError(
        "A process interaction order-type change result has an invalid status.");
  }
  return {static_cast<InteractionOrderTypeChangeStatus>(status)};
}

std::vector<std::uint8_t>
encodeProcessFederationAttributeOrderTypeChangeResult(
    ProcessFederationAttributeOrderTypeChangeResult const& result) {
  auto const encoded = static_cast<std::uint8_t>(result.status);
  if (encoded > static_cast<std::uint8_t>(
                    AttributeOrderTypeChangeStatus::invalid_order_type)) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute order-type change result has an invalid status.");
  }
  PayloadWriter writer;
  writer.unsigned8(encoded);
  return std::move(writer).finish();
}

ProcessFederationAttributeOrderTypeChangeResult
decodeProcessFederationAttributeOrderTypeChangeResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const status = reader.unsigned8();
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   AttributeOrderTypeChangeStatus::invalid_order_type)) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute order-type change result has an invalid status.");
  }
  return {static_cast<AttributeOrderTypeChangeStatus>(status)};
}

std::vector<std::uint8_t>
encodeProcessFederationAttributeOrderTypeDefaultResult(
    ProcessFederationAttributeOrderTypeDefaultResult const& result) {
  auto const encoded = static_cast<std::uint8_t>(result.status);
  if (encoded > static_cast<std::uint8_t>(
                    AttributeOrderTypeDefaultStatus::invalid_order_type)) {
    throw ProcessFederationServiceProtocolError(
        "A process default attribute order-type result has an invalid status.");
  }
  PayloadWriter writer;
  writer.unsigned8(encoded);
  return std::move(writer).finish();
}

ProcessFederationAttributeOrderTypeDefaultResult
decodeProcessFederationAttributeOrderTypeDefaultResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const status = reader.unsigned8();
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   AttributeOrderTypeDefaultStatus::invalid_order_type)) {
    throw ProcessFederationServiceProtocolError(
        "A process default attribute order-type result has an invalid status.");
  }
  return {static_cast<AttributeOrderTypeDefaultStatus>(status)};
}

std::vector<std::uint8_t>
encodeProcessFederationAttributeTransportationTypeDefaultResult(
    ProcessFederationAttributeTransportationTypeDefaultResult const& result) {
  auto const encoded = static_cast<std::uint8_t>(result.status);
  if (encoded > static_cast<std::uint8_t>(
                    AttributeTransportationTypeDefaultStatus::inconsistent_catalog)) {
    throw ProcessFederationServiceProtocolError(
        "A process default attribute transportation-type result has an invalid status.");
  }
  PayloadWriter writer;
  writer.unsigned8(encoded);
  return std::move(writer).finish();
}

ProcessFederationAttributeTransportationTypeDefaultResult
decodeProcessFederationAttributeTransportationTypeDefaultResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const status = reader.unsigned8();
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   AttributeTransportationTypeDefaultStatus::inconsistent_catalog)) {
    throw ProcessFederationServiceProtocolError(
        "A process default attribute transportation-type result has an invalid status.");
  }
  return {static_cast<AttributeTransportationTypeDefaultStatus>(status)};
}

std::vector<std::uint8_t>
encodeProcessFederationAttributeTransportationTypeChangeResult(
    ProcessFederationAttributeTransportationTypeChangeResult const& result) {
  auto const encoded = static_cast<std::uint8_t>(result.status);
  if (encoded > static_cast<std::uint8_t>(
                    AttributeTransportationTypeChangeStatus::inconsistent_catalog)) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute transportation-type change result has an invalid status.");
  }
  PayloadWriter writer;
  writer.unsigned8(encoded);
  return std::move(writer).finish();
}

ProcessFederationAttributeTransportationTypeChangeResult
decodeProcessFederationAttributeTransportationTypeChangeResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const status = reader.unsigned8();
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   AttributeTransportationTypeChangeStatus::inconsistent_catalog)) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute transportation-type change result has an invalid status.");
  }
  return {static_cast<AttributeTransportationTypeChangeStatus>(status)};
}

std::vector<std::uint8_t>
encodeProcessFederationAttributeTransportationTypeQueryResult(
    ProcessFederationAttributeTransportationTypeQueryResult const& result) {
  auto const encoded = static_cast<std::uint8_t>(result.status);
  if (encoded > static_cast<std::uint8_t>(
                    AttributeTransportationTypeQueryStatus::inconsistent_catalog)) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute transportation-type query result has an invalid status.");
  }
  PayloadWriter writer;
  writer.unsigned8(encoded);
  return std::move(writer).finish();
}

ProcessFederationAttributeTransportationTypeQueryResult
decodeProcessFederationAttributeTransportationTypeQueryResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const status = reader.unsigned8();
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   AttributeTransportationTypeQueryStatus::inconsistent_catalog)) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute transportation-type query result has an invalid status.");
  }
  return {static_cast<AttributeTransportationTypeQueryStatus>(status)};
}

std::vector<std::uint8_t>
encodeProcessFederationInteractionTransportationTypeChangeResult(
    ProcessFederationInteractionTransportationTypeChangeResult const& result) {
  auto const encoded = static_cast<std::uint8_t>(result.status);
  if (encoded > static_cast<std::uint8_t>(
                    InteractionTransportationTypeChangeStatus::inconsistent_catalog)) {
    throw ProcessFederationServiceProtocolError(
        "A process interaction transportation-type change result has an invalid status.");
  }
  PayloadWriter writer;
  writer.unsigned8(encoded);
  return std::move(writer).finish();
}

ProcessFederationInteractionTransportationTypeChangeResult
decodeProcessFederationInteractionTransportationTypeChangeResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const status = reader.unsigned8();
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   InteractionTransportationTypeChangeStatus::inconsistent_catalog)) {
    throw ProcessFederationServiceProtocolError(
        "A process interaction transportation-type change result has an invalid status.");
  }
  return {static_cast<InteractionTransportationTypeChangeStatus>(status)};
}

std::vector<std::uint8_t>
encodeProcessFederationInteractionTransportationTypeQueryResult(
    ProcessFederationInteractionTransportationTypeQueryResult const& result) {
  auto const encoded = static_cast<std::uint8_t>(result.status);
  if (encoded > static_cast<std::uint8_t>(
                    InteractionTransportationTypeQueryStatus::inconsistent_catalog)) {
    throw ProcessFederationServiceProtocolError(
        "A process interaction transportation-type query result has an invalid status.");
  }
  PayloadWriter writer;
  writer.unsigned8(encoded);
  return std::move(writer).finish();
}

ProcessFederationInteractionTransportationTypeQueryResult
decodeProcessFederationInteractionTransportationTypeQueryResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const status = reader.unsigned8();
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   InteractionTransportationTypeQueryStatus::inconsistent_catalog)) {
    throw ProcessFederationServiceProtocolError(
        "A process interaction transportation-type query result has an invalid status.");
  }
  return {static_cast<InteractionTransportationTypeQueryStatus>(status)};
}

std::vector<std::uint8_t>
encodeProcessFederationAttributeOwnershipCheckResult(
    ProcessFederationAttributeOwnershipCheckResult const& result) {
  if (result.status != AttributeOwnershipCheckStatus::applied &&
      result.ownedByRequestingFederate) {
    throw ProcessFederationServiceProtocolError(
        "A failed process attribute-ownership check cannot report ownership.");
  }
  PayloadWriter writer;
  writeAttributeOwnershipCheckStatus(writer, result.status);
  writer.unsigned8(result.ownedByRequestingFederate ? 1U : 0U);
  return std::move(writer).finish();
}

ProcessFederationAttributeOwnershipCheckResult
decodeProcessFederationAttributeOwnershipCheckResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationAttributeOwnershipCheckResult result;
  result.status = readAttributeOwnershipCheckStatus(reader);
  auto const owned = reader.unsigned8();
  if (owned > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership check result has an invalid boolean value.");
  }
  result.ownedByRequestingFederate = owned != 0U;
  reader.finish();
  if (result.status != AttributeOwnershipCheckStatus::applied &&
      result.ownedByRequestingFederate) {
    throw ProcessFederationServiceProtocolError(
        "A failed process attribute-ownership check cannot report ownership.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationAttributeOwnershipQueryResult(
    ProcessFederationAttributeOwnershipQueryResult const& result) {
  if (result.status != AttributeOwnershipQueryStatus::applied &&
      result.recipientCount != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process attribute-ownership query cannot report recipients.");
  }
  PayloadWriter writer;
  writeAttributeOwnershipQueryStatus(writer, result.status);
  writer.unsigned32(result.recipientCount);
  return std::move(writer).finish();
}

ProcessFederationAttributeOwnershipQueryResult
decodeProcessFederationAttributeOwnershipQueryResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationAttributeOwnershipQueryResult result;
  result.status = readAttributeOwnershipQueryStatus(reader);
  result.recipientCount = reader.unsigned32();
  reader.finish();
  if (result.status != AttributeOwnershipQueryStatus::applied &&
      result.recipientCount != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process attribute-ownership query cannot report recipients.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationAttributeOwnershipAcquisitionIfAvailableResult(
    ProcessFederationAttributeOwnershipAcquisitionIfAvailableResult const& result) {
  if (result.status !=
          AttributeOwnershipAcquisitionIfAvailableStatus::applied &&
      result.recipientCount != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process attribute-ownership acquisition-if-available result cannot report recipients.");
  }
  PayloadWriter writer;
  writeAttributeOwnershipAcquisitionIfAvailableStatus(writer, result.status);
  writer.unsigned32(result.recipientCount);
  return std::move(writer).finish();
}

ProcessFederationAttributeOwnershipAcquisitionIfAvailableResult
decodeProcessFederationAttributeOwnershipAcquisitionIfAvailableResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationAttributeOwnershipAcquisitionIfAvailableResult result;
  result.status = readAttributeOwnershipAcquisitionIfAvailableStatus(reader);
  result.recipientCount = reader.unsigned32();
  reader.finish();
  if (result.status !=
          AttributeOwnershipAcquisitionIfAvailableStatus::applied &&
      result.recipientCount != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process attribute-ownership acquisition-if-available result cannot report recipients.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationAttributeOwnershipAcquisitionResult(
    ProcessFederationAttributeOwnershipAcquisitionResult const& result) {
  if (result.status != AttributeOwnershipAcquisitionStatus::applied &&
      result.recipientCount != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process attribute-ownership acquisition result cannot report recipients.");
  }
  PayloadWriter writer;
  writeAttributeOwnershipAcquisitionStatus(writer, result.status);
  writer.unsigned32(result.recipientCount);
  return std::move(writer).finish();
}

ProcessFederationAttributeOwnershipAcquisitionResult
decodeProcessFederationAttributeOwnershipAcquisitionResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationAttributeOwnershipAcquisitionResult result;
  result.status = readAttributeOwnershipAcquisitionStatus(reader);
  result.recipientCount = reader.unsigned32();
  reader.finish();
  if (result.status != AttributeOwnershipAcquisitionStatus::applied &&
      result.recipientCount != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process attribute-ownership acquisition result cannot report recipients.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationAttributeOwnershipReleaseDeniedResult(
    ProcessFederationAttributeOwnershipReleaseDeniedResult const& result) {
  if (result.status != AttributeOwnershipReleaseDeniedStatus::applied &&
      result.recipientCount != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process attribute-ownership release-denied result cannot report recipients.");
  }
  PayloadWriter writer;
  writeAttributeOwnershipReleaseDeniedStatus(writer, result.status);
  writer.unsigned32(result.recipientCount);
  return std::move(writer).finish();
}

ProcessFederationAttributeOwnershipReleaseDeniedResult
decodeProcessFederationAttributeOwnershipReleaseDeniedResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationAttributeOwnershipReleaseDeniedResult result;
  result.status = readAttributeOwnershipReleaseDeniedStatus(reader);
  result.recipientCount = reader.unsigned32();
  reader.finish();
  if (result.status != AttributeOwnershipReleaseDeniedStatus::applied &&
      result.recipientCount != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process attribute-ownership release-denied result cannot report recipients.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationAttributeOwnershipAcquisitionCancellationResult(
    ProcessFederationAttributeOwnershipAcquisitionCancellationResult const& result) {
  if (result.status !=
          AttributeOwnershipAcquisitionCancellationStatus::applied &&
      result.recipientCount != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process attribute-ownership acquisition-cancellation result cannot report recipients.");
  }
  PayloadWriter writer;
  writeAttributeOwnershipAcquisitionCancellationStatus(writer, result.status);
  writer.unsigned32(result.recipientCount);
  return std::move(writer).finish();
}

ProcessFederationAttributeOwnershipAcquisitionCancellationResult
decodeProcessFederationAttributeOwnershipAcquisitionCancellationResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationAttributeOwnershipAcquisitionCancellationResult result;
  result.status = readAttributeOwnershipAcquisitionCancellationStatus(reader);
  result.recipientCount = reader.unsigned32();
  reader.finish();
  if (result.status !=
          AttributeOwnershipAcquisitionCancellationStatus::applied &&
      result.recipientCount != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process attribute-ownership acquisition-cancellation result cannot report recipients.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationCancelNegotiatedAttributeOwnershipDivestitureResult(
    ProcessFederationCancelNegotiatedAttributeOwnershipDivestitureResult const& result) {
  if (result.status !=
          CancelNegotiatedAttributeOwnershipDivestitureStatus::applied &&
      result.recipientCount != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process negotiated-divestiture cancellation result cannot report recipients.");
  }
  PayloadWriter writer;
  auto const encoded = static_cast<std::uint8_t>(result.status);
  if (encoded > static_cast<std::uint8_t>(
                    CancelNegotiatedAttributeOwnershipDivestitureStatus::
                        inconsistent_catalog)) {
    throw ProcessFederationServiceProtocolError(
        "A process negotiated-divestiture cancellation result has an invalid status.");
  }
  writer.unsigned8(encoded);
  writer.unsigned32(result.recipientCount);
  return std::move(writer).finish();
}

ProcessFederationCancelNegotiatedAttributeOwnershipDivestitureResult
decodeProcessFederationCancelNegotiatedAttributeOwnershipDivestitureResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationCancelNegotiatedAttributeOwnershipDivestitureResult result;
  auto const encodedStatus = reader.unsigned8();
  if (encodedStatus > static_cast<std::uint8_t>(
                         CancelNegotiatedAttributeOwnershipDivestitureStatus::
                             inconsistent_catalog)) {
    throw ProcessFederationServiceProtocolError(
        "A process negotiated-divestiture cancellation result has an invalid status.");
  }
  result.status = static_cast<CancelNegotiatedAttributeOwnershipDivestitureStatus>(
      encodedStatus);
  result.recipientCount = reader.unsigned32();
  reader.finish();
  if (result.status !=
          CancelNegotiatedAttributeOwnershipDivestitureStatus::applied &&
      result.recipientCount != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process negotiated-divestiture cancellation result cannot report recipients.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationNegotiatedAttributeOwnershipDivestitureResult(
    ProcessFederationNegotiatedAttributeOwnershipDivestitureResult const& result) {
  if (result.status !=
          NegotiatedAttributeOwnershipDivestitureStatus::applied &&
      result.recipientCount != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process negotiated-divestiture result cannot report recipients.");
  }
  PayloadWriter writer;
  writeNegotiatedAttributeOwnershipDivestitureStatus(writer, result.status);
  writer.unsigned32(result.recipientCount);
  return std::move(writer).finish();
}

ProcessFederationNegotiatedAttributeOwnershipDivestitureResult
decodeProcessFederationNegotiatedAttributeOwnershipDivestitureResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationNegotiatedAttributeOwnershipDivestitureResult result;
  result.status = readNegotiatedAttributeOwnershipDivestitureStatus(reader);
  result.recipientCount = reader.unsigned32();
  reader.finish();
  if (result.status !=
          NegotiatedAttributeOwnershipDivestitureStatus::applied &&
      result.recipientCount != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process negotiated-divestiture result cannot report recipients.");
  }
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationConfirmDivestitureResult(
    ProcessFederationConfirmDivestitureResult const& result) {
  if (result.status != ConfirmDivestitureStatus::applied &&
      result.recipientCount != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process Confirm Divestiture result cannot report recipients.");
  }
  PayloadWriter writer;
  writeConfirmDivestitureStatus(writer, result.status);
  writer.unsigned32(result.recipientCount);
  return std::move(writer).finish();
}

ProcessFederationConfirmDivestitureResult
decodeProcessFederationConfirmDivestitureResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationConfirmDivestitureResult result;
  result.status = readConfirmDivestitureStatus(reader);
  result.recipientCount = reader.unsigned32();
  reader.finish();
  if (result.status != ConfirmDivestitureStatus::applied &&
      result.recipientCount != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process Confirm Divestiture result cannot report recipients.");
  }
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationCreateRegionResult(
    ProcessFederationCreateRegionResult const& result) {
  if (result.status == RegionServiceStatus::applied) {
    requireNonzero(
        result.regionHandle,
        "A successful process create-region result requires a region handle.");
  } else if (result.regionHandle != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process create-region result cannot carry a region handle.");
  }
  PayloadWriter writer;
  writeRegionServiceStatus(writer, result.status);
  writer.unsigned64(result.regionHandle);
  return std::move(writer).finish();
}

ProcessFederationCreateRegionResult decodeProcessFederationCreateRegionResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationCreateRegionResult result;
  result.status = readRegionServiceStatus(reader);
  result.regionHandle = reader.unsigned64();
  reader.finish();
  if (result.status == RegionServiceStatus::applied) {
    requireNonzero(
        result.regionHandle,
        "A successful process create-region result requires a region handle.");
  } else if (result.regionHandle != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process create-region result cannot carry a region handle.");
  }
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationDimensionSetResult(
    ProcessFederationDimensionSetResult const& result) {
  if (result.status != RegionServiceStatus::applied &&
      !result.dimensionHandles.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A failed process dimension-set result cannot carry handles.");
  }
  validateHandleVector(
      result.dimensionHandles,
      "A process dimension-set result requires sorted, unique handles.");
  PayloadWriter writer;
  writeRegionServiceStatus(writer, result.status);
  writer.unsigned64Vector(result.dimensionHandles);
  return std::move(writer).finish();
}

ProcessFederationDimensionSetResult decodeProcessFederationDimensionSetResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationDimensionSetResult result;
  result.status = readRegionServiceStatus(reader);
  result.dimensionHandles = reader.unsigned64Vector();
  reader.finish();
  validateHandleVector(
      result.dimensionHandles,
      "A process dimension-set result requires sorted, unique handles.");
  if (result.status != RegionServiceStatus::applied &&
      !result.dimensionHandles.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A failed process dimension-set result cannot carry handles.");
  }
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationRangeBoundsResult(
    ProcessFederationRangeBoundsResult const& result) {
  if (result.status != RegionServiceStatus::applied &&
      (result.lowerBound != 0UL || result.upperBound != 0UL)) {
    throw ProcessFederationServiceProtocolError(
        "A failed process range-bounds result cannot carry bounds.");
  }
  PayloadWriter writer;
  writeRegionServiceStatus(writer, result.status);
  writer.unsigned64(static_cast<std::uint64_t>(result.lowerBound));
  writer.unsigned64(static_cast<std::uint64_t>(result.upperBound));
  return std::move(writer).finish();
}

ProcessFederationRangeBoundsResult decodeProcessFederationRangeBoundsResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationRangeBoundsResult result;
  result.status = readRegionServiceStatus(reader);
  auto const lower = reader.unsigned64();
  auto const upper = reader.unsigned64();
  reader.finish();
  if (lower > std::numeric_limits<unsigned long>::max() ||
      upper > std::numeric_limits<unsigned long>::max()) {
    throw ProcessFederationServiceProtocolError(
        "A process range-bounds result exceeds the native bound type.");
  }
  result.lowerBound = static_cast<unsigned long>(lower);
  result.upperBound = static_cast<unsigned long>(upper);
  if (result.status != RegionServiceStatus::applied &&
      (result.lowerBound != 0UL || result.upperBound != 0UL)) {
    throw ProcessFederationServiceProtocolError(
        "A failed process range-bounds result cannot carry bounds.");
  }
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationDimensionUpperBoundResult(
    ProcessFederationDimensionUpperBoundResult const& result) {
  if (!result.found && result.upperBound != 0UL) {
    throw ProcessFederationServiceProtocolError(
        "An absent process dimension bound cannot carry a value.");
  }
  PayloadWriter writer;
  writer.unsigned8(result.found ? 1U : 0U);
  writer.unsigned64(static_cast<std::uint64_t>(result.upperBound));
  return std::move(writer).finish();
}

ProcessFederationDimensionUpperBoundResult
decodeProcessFederationDimensionUpperBoundResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const found = reader.unsigned8();
  if (found > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process dimension bound result has an invalid presence marker.");
  }
  auto const upper = reader.unsigned64();
  reader.finish();
  if (upper > std::numeric_limits<unsigned long>::max()) {
    throw ProcessFederationServiceProtocolError(
        "A process dimension bound result exceeds the native bound type.");
  }
  ProcessFederationDimensionUpperBoundResult result;
  result.found = found != 0U;
  result.upperBound = static_cast<unsigned long>(upper);
  if (!result.found && result.upperBound != 0UL) {
    throw ProcessFederationServiceProtocolError(
        "An absent process dimension bound cannot carry a value.");
  }
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationAvailableDimensionsResult(
    ProcessFederationAvailableDimensionsResult const& result) {
  if (!result.found && !result.dimensionHandles.empty()) {
    throw ProcessFederationServiceProtocolError(
        "An absent process available-dimensions result cannot carry handles.");
  }
  validateHandleVector(
      result.dimensionHandles,
      "A process available-dimensions result requires sorted, unique handles.");
  PayloadWriter writer;
  writer.unsigned8(result.found ? 1U : 0U);
  writer.unsigned64Vector(result.dimensionHandles);
  return std::move(writer).finish();
}

ProcessFederationAvailableDimensionsResult
decodeProcessFederationAvailableDimensionsResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const found = reader.unsigned8();
  if (found > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process available-dimensions result has an invalid presence marker.");
  }
  ProcessFederationAvailableDimensionsResult result;
  result.found = found != 0U;
  result.dimensionHandles = reader.unsigned64Vector();
  reader.finish();
  validateHandleVector(
      result.dimensionHandles,
      "A process available-dimensions result requires sorted, unique handles.");
  if (!result.found && !result.dimensionHandles.empty()) {
    throw ProcessFederationServiceProtocolError(
        "An absent process available-dimensions result cannot carry handles.");
  }
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationInteractionEnvelope(
    ProcessFederationInteractionEnvelope const& envelope) {
  if (envelope.parameterValues.size() >
      std::numeric_limits<std::uint32_t>::max()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation interaction envelope has too many parameters.");
  }
  PayloadWriter writer;
  writer.raw(kInteractionEnvelopeMagic);
  writer.unsigned8(kInteractionEnvelopeVersion);
  writer.unsigned32(
      static_cast<std::uint32_t>(envelope.parameterValues.size()));
  std::set<std::uint64_t> seenHandles;
  for (auto const& [parameterHandle, parameterValue] :
       envelope.parameterValues) {
    requireNonzero(
        parameterHandle,
        "A process federation interaction envelope requires parameter identities.");
    if (!seenHandles.insert(parameterHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process federation interaction envelope repeats a parameter identity.");
    }
    writer.unsigned64(parameterHandle);
    writer.bytes(parameterValue);
  }
  writer.bytes(envelope.userSuppliedTag);
  return std::move(writer).finish();
}

std::optional<ProcessFederationInteractionEnvelope>
decodeProcessFederationInteractionEnvelope(
    std::span<std::uint8_t const> encoded) {
  if (encoded.size() < kInteractionEnvelopeMagic.size() + 1U) {
    return std::nullopt;
  }
  for (std::size_t index = 0U; index < kInteractionEnvelopeMagic.size();
       ++index) {
    if (encoded[index] != kInteractionEnvelopeMagic[index]) {
      return std::nullopt;
    }
  }

  PayloadReader reader(encoded.subspan(kInteractionEnvelopeMagic.size()));
  if (reader.unsigned8() != kInteractionEnvelopeVersion) {
    throw ProcessFederationServiceProtocolError(
        "A process federation interaction envelope has an unsupported version.");
  }
  auto const parameterCount = reader.unsigned32();
  ProcessFederationInteractionEnvelope result;
  result.parameterValues.reserve(parameterCount);
  std::set<std::uint64_t> seenHandles;
  for (std::uint32_t index = 0U; index < parameterCount; ++index) {
    auto const parameterHandle = reader.unsigned64();
    requireNonzero(
        parameterHandle,
        "A process federation interaction envelope contains an invalid parameter identity.");
    if (!seenHandles.insert(parameterHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process federation interaction envelope repeats a parameter identity.");
    }
    result.parameterValues.emplace_back(parameterHandle, reader.bytes());
  }
  result.userSuppliedTag = reader.bytes();
  reader.finish();
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationReceiveInteractionResult(
    ProcessFederationReceiveInteractionResult const& result) {
  auto const eventCount = static_cast<unsigned>(result.event.has_value()) +
      static_cast<unsigned>(result.attributeEvent.has_value()) +
      static_cast<unsigned>(result.discoveryEvent.has_value()) +
      static_cast<unsigned>(result.scopeChangeEvent.has_value()) +
      static_cast<unsigned>(result.removalEvent.has_value()) +
      static_cast<unsigned>(result.attributeRelevanceAdvisoryEvent.has_value()) +
      static_cast<unsigned>(result.attributeValueUpdateRequestEvent.has_value()) +
      static_cast<unsigned>(result.attributeOwnershipQueryEvent.has_value()) +
      static_cast<unsigned>(
          result.attributeOwnershipAcquisitionIfAvailableEvent.has_value());
  const auto eventCountWithRegularAcquisition = eventCount + static_cast<unsigned>(
      result.attributeOwnershipAcquisitionEvent.has_value());
  const auto eventCountWithOwnershipUnavailable =
      eventCountWithRegularAcquisition + static_cast<unsigned>(
          result.attributeOwnershipUnavailableEvent.has_value());
  const auto eventCountWithTransportation =
      eventCountWithOwnershipUnavailable + static_cast<unsigned>(
          result.attributeTransportationTypeChangeEvent.has_value()) +
      static_cast<unsigned>(result.attributeTransportationTypeQueryEvent.has_value());
  const auto eventCountWithInteractionTransportation =
      eventCountWithTransportation + static_cast<unsigned>(
          result.interactionTransportationTypeChangeEvent.has_value()) +
      static_cast<unsigned>(result.interactionTransportationTypeQueryEvent.has_value());
  const auto eventCountWithSynchronization =
      eventCountWithInteractionTransportation +
      static_cast<unsigned>(result.synchronizationPointAnnouncementEvent.has_value()) +
      static_cast<unsigned>(result.federationSynchronizedEvent.has_value());
  const auto eventCountWithSave = eventCountWithSynchronization +
      static_cast<unsigned>(result.saveEvent.has_value());
  const auto eventCountWithRestore = eventCountWithSave +
      static_cast<unsigned>(result.restoreEvent.has_value());
  if (eventCountWithRestore > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process federation receive result cannot contain multiple events.");
  }
  PayloadWriter writer;
  unsigned eventKind = 0U;
  if (result.event) {
    eventKind = 1U;
  } else if (result.attributeEvent) {
    eventKind = 2U;
  } else if (result.discoveryEvent) {
    eventKind = 3U;
  } else if (result.scopeChangeEvent) {
    eventKind = 4U;
  } else if (result.attributeRelevanceAdvisoryEvent) {
    eventKind = 5U;
  } else if (result.removalEvent) {
    eventKind = 6U;
  } else if (result.attributeValueUpdateRequestEvent) {
    eventKind = 7U;
  } else if (result.attributeOwnershipQueryEvent) {
    eventKind = 8U;
  } else if (result.attributeOwnershipAcquisitionIfAvailableEvent) {
    eventKind = 9U;
  } else if (result.attributeOwnershipAcquisitionEvent) {
    eventKind = 10U;
  } else if (result.attributeOwnershipUnavailableEvent) {
    eventKind = 11U;
  } else if (result.attributeTransportationTypeChangeEvent) {
    eventKind = 12U;
  } else if (result.attributeTransportationTypeQueryEvent) {
    eventKind = 13U;
  } else if (result.interactionTransportationTypeChangeEvent) {
    eventKind = 14U;
  } else if (result.interactionTransportationTypeQueryEvent) {
    eventKind = 15U;
  } else if (result.synchronizationPointAnnouncementEvent) {
    eventKind = 16U;
  } else if (result.federationSynchronizedEvent) {
    eventKind = 17U;
  } else if (result.saveEvent) {
    eventKind = 18U;
  } else if (result.restoreEvent) {
    eventKind = 19U;
  }
  writer.unsigned8(eventKind);
  if (result.event.has_value()) {
    auto const& event = *result.event;
    requireNonzero(
        event.producingFederateId,
        "A process federation interaction event requires a producer identity.");
    requireNonzero(
        event.receivingFederateId,
        "A process federation interaction event requires a recipient identity.");
    requireNonzero(
        event.interactionClassHandle,
        "A process federation interaction event requires an interaction class.");
    writer.unsigned64(event.producingFederateId);
    writer.unsigned64(event.receivingFederateId);
    writer.unsigned64(event.interactionClassHandle);
    writer.unsigned64Vector(event.parameterHandles);
    writer.bytes(event.payload);
    writer.string(event.transportationName);
    writeOptionalLogicalTime(writer, event.timestamp);
    writer.unsigned8(event.objectInstanceHandle.has_value() ? 1U : 0U);
    if (event.objectInstanceHandle) {
      requireNonzero(
          *event.objectInstanceHandle,
          "A process directed-interaction event requires a target object instance.");
      writer.unsigned64(*event.objectInstanceHandle);
    }
    writer.unsigned8(event.retractionMessageId.has_value() ? 1U : 0U);
    if (event.retractionMessageId) {
      requireNonzero(
          *event.retractionMessageId,
          "A process directed-interaction event requires a retraction identity.");
      writer.unsigned64(*event.retractionMessageId);
    }
    writeInteractionRegionMetadata(writer, event);
    writeInteractionOrderMetadata(writer, event);
  } else if (result.attributeEvent.has_value()) {
    auto const& event = *result.attributeEvent;
    requireNonzero(
        event.producingFederateId,
        "A process federation attribute event requires a producer identity.");
    requireNonzero(
        event.receivingFederateId,
        "A process federation attribute event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process federation attribute event requires an object instance.");
    if (event.attributeValues.empty() || event.transportationName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process federation attribute event requires values and transportation.");
    }
    if (event.attributeValues.size() >
        std::numeric_limits<std::uint32_t>::max()) {
      throw ProcessFederationServiceProtocolError(
          "A process federation attribute event has too many attributes.");
    }
    writer.unsigned64(event.producingFederateId);
    writer.unsigned64(event.receivingFederateId);
    writer.unsigned64(event.objectInstanceHandle);
    writer.unsigned32(static_cast<std::uint32_t>(event.attributeValues.size()));
    std::set<std::uint64_t> seenHandles;
    for (auto const& [attributeHandle, value] : event.attributeValues) {
      requireNonzero(
          attributeHandle,
          "A process federation attribute event requires attribute identities.");
      if (!seenHandles.insert(attributeHandle).second) {
        throw ProcessFederationServiceProtocolError(
            "A process federation attribute event repeats an attribute identity.");
      }
      writer.unsigned64(attributeHandle);
      writer.bytes(value);
    }
    writer.bytes(event.userSuppliedTag);
    writer.string(event.transportationName);
    writeAttributeUpdateRegionMetadata(writer, event);
    writeOptionalLogicalTime(writer, event.timestamp);
    if (event.retractionMessageId) {
      requireNonzero(
          *event.retractionMessageId,
          "A process attribute event requires a retraction identity.");
      writer.unsigned64(*event.retractionMessageId);
    }
  } else if (result.discoveryEvent.has_value()) {
    auto const& event = *result.discoveryEvent;
    requireNonzero(
        event.receivingFederateId,
        "A process federation discovery event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process federation discovery event requires an object instance.");
    requireNonzero(
        event.objectClassHandle,
        "A process federation discovery event requires an object class.");
    requireNonzero(
        event.producingFederateId,
        "A process federation discovery event requires a producer identity.");
    if (event.objectInstanceName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process federation discovery event requires an object instance name.");
    }
    writer.unsigned64(event.receivingFederateId);
    writer.unsigned64(event.objectInstanceHandle);
    writer.unsigned64(event.objectClassHandle);
    writer.wideString(event.objectInstanceName);
    writer.unsigned64(event.producingFederateId);
  } else if (result.scopeChangeEvent.has_value()) {
    auto const& event = *result.scopeChangeEvent;
    requireNonzero(
        event.receivingFederateId,
        "A process scope event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process scope event requires an object instance.");
    if (event.attributeHandles.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process scope event requires attributes.");
    }
    if (event.attributeHandles.size() >
        std::numeric_limits<std::uint32_t>::max()) {
      throw ProcessFederationServiceProtocolError(
          "A process scope event has too many attributes.");
    }
    writer.unsigned64(event.receivingFederateId);
    writer.unsigned64(event.objectInstanceHandle);
    writer.unsigned32(static_cast<std::uint32_t>(event.attributeHandles.size()));
    for (std::uint64_t const attributeHandle : event.attributeHandles) {
      requireNonzero(
          attributeHandle,
          "A process scope event requires attribute identities.");
      writer.unsigned64(attributeHandle);
    }
    writer.unsigned8(event.inScope ? 1U : 0U);
  } else if (result.removalEvent.has_value()) {
    auto const& event = *result.removalEvent;
    requireNonzero(
        event.receivingFederateId,
        "A process object removal event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process object removal event requires an object instance.");
    requireNonzero(
        event.producingFederateId,
        "A process object removal event requires a producer identity.");
    writer.unsigned64(event.receivingFederateId);
    writer.unsigned64(event.objectInstanceHandle);
    writer.unsigned64(event.producingFederateId);
    writer.bytes(event.userSuppliedTag);
    writeOptionalLogicalTime(writer, event.timestamp);
    writeOptionalMessageId(
        writer,
        event.retractionMessageId.value_or(0U));
    // The private message identity remains present for timestamped queue
    // bookkeeping even when the public API must not expose a retraction
    // designator.  Append the projection bit so older payloads (which always
    // exposed the id) remain decodable by defaulting to true when absent.
    if (event.retractionMessageId) {
      writer.unsigned8(event.provideRetraction ? 1U : 0U);
    }
    if (event.timestamp) {
      if (!event.sentOrderType || !event.receivedOrderType) {
        throw ProcessFederationServiceProtocolError(
            "A timestamped process object removal event must carry both order classifications when present.");
      }
      auto const validOrder = [](rti1516_2025::OrderType order) {
        return order == rti1516_2025::RECEIVE ||
            order == rti1516_2025::TIMESTAMP;
      };
      if (!validOrder(*event.sentOrderType) ||
          !validOrder(*event.receivedOrderType)) {
        throw ProcessFederationServiceProtocolError(
            "A timestamped process object removal event has an invalid order classification.");
      }
      writer.unsigned8(1U);
      writer.unsigned8(static_cast<std::uint8_t>(*event.sentOrderType));
      writer.unsigned8(static_cast<std::uint8_t>(*event.receivedOrderType));
    }
  } else if (result.attributeRelevanceAdvisoryEvent.has_value()) {
    auto const& event = *result.attributeRelevanceAdvisoryEvent;
    requireNonzero(
        event.providingFederateId,
        "A process attribute relevance advisory requires an owning federate.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process attribute relevance advisory requires an object instance.");
    if (event.attributeHandles.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute relevance advisory requires attributes.");
    }
    if (event.attributeHandles.size() >
        std::numeric_limits<std::uint32_t>::max()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute relevance advisory has too many attributes.");
    }
    if (event.updateRateDesignator && event.updateRateDesignator->empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute relevance advisory cannot carry an empty update-rate designator.");
    }
    writer.unsigned64(event.providingFederateId);
    writer.unsigned64(event.receivingFederateId);
    writer.unsigned64(event.objectInstanceHandle);
    writer.unsigned32(static_cast<std::uint32_t>(event.attributeHandles.size()));
    for (std::uint64_t const attributeHandle : event.attributeHandles) {
      requireNonzero(
          attributeHandle,
          "A process attribute relevance advisory requires attribute identities.");
      writer.unsigned64(attributeHandle);
    }
    writer.unsigned8(event.turnUpdatesOn ? 1U : 0U);
    writer.unsigned8(event.updateRateDesignator.has_value() ? 1U : 0U);
    if (event.updateRateDesignator) {
      writer.string(*event.updateRateDesignator);
    }
  } else if (result.attributeValueUpdateRequestEvent.has_value()) {
    auto const& event = *result.attributeValueUpdateRequestEvent;
    requireNonzero(
        event.requestingFederateId,
        "A process attribute-value-update request event requires a requester.");
    requireNonzero(
        event.providingFederateId,
        "A process attribute-value-update request event requires a provider.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process attribute-value-update request event requires an object instance.");
    if (event.requestedAttributeHandles.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-value-update request event requires attributes.");
    }
    if (event.requestedAttributeHandles.size() >
        std::numeric_limits<std::uint32_t>::max()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-value-update request event has too many attributes.");
    }
    writer.unsigned64(event.requestingFederateId);
    writer.unsigned64(event.providingFederateId);
    writer.unsigned64(event.objectInstanceHandle);
    writer.unsigned32(static_cast<std::uint32_t>(
        event.requestedAttributeHandles.size()));
    for (std::uint64_t const attributeHandle :
         event.requestedAttributeHandles) {
      requireNonzero(
          attributeHandle,
          "A process attribute-value-update request event requires attribute identities.");
      writer.unsigned64(attributeHandle);
    }
    writer.bytes(event.userSuppliedTag);
  } else if (result.attributeOwnershipQueryEvent.has_value()) {
    auto const& event = *result.attributeOwnershipQueryEvent;
    requireNonzero(
        event.requestId,
        "A process attribute-ownership query event requires a request identity.");
    requireNonzero(
        event.receivingFederateId,
        "A process attribute-ownership query event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process attribute-ownership query event requires an object instance.");
    if (event.attributeHandles.empty() ||
        event.attributeHandles.size() >
            std::numeric_limits<std::uint32_t>::max()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership query event requires attributes.");
    }
    if (event.reportKind == AttributeOwnershipQueryReportKind::federate) {
      requireNonzero(
          event.owningFederateId,
          "A federate-owned process attribute-ownership query event requires an owner.");
    } else if (event.owningFederateId != 0U) {
      throw ProcessFederationServiceProtocolError(
          "A non-federate process attribute-ownership query event cannot carry an owner.");
    }
    writer.unsigned64(event.requestId);
    writer.unsigned64(event.receivingFederateId);
    writer.unsigned64(event.objectInstanceHandle);
    writer.unsigned8(static_cast<std::uint8_t>(event.reportKind));
    writer.unsigned64(event.owningFederateId);
    writer.unsigned32(static_cast<std::uint32_t>(event.attributeHandles.size()));
    for (std::uint64_t const attributeHandle : event.attributeHandles) {
      requireNonzero(
          attributeHandle,
          "A process attribute-ownership query event requires attribute identities.");
      writer.unsigned64(attributeHandle);
    }
  } else if (result.attributeOwnershipAcquisitionIfAvailableEvent.has_value()) {
    auto const& event = *result.attributeOwnershipAcquisitionIfAvailableEvent;
    requireNonzero(
        event.requestId,
        "A process attribute-ownership acquisition-if-available event requires a request identity.");
    requireNonzero(
        event.receivingFederateId,
        "A process attribute-ownership acquisition-if-available event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process attribute-ownership acquisition-if-available event requires an object instance.");
    if (event.securedAttributeHandles.empty() &&
        event.unavailableAttributeHandles.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership acquisition-if-available event requires a delivery.");
    }
    if (event.securedAttributeHandles.size() >
            std::numeric_limits<std::uint32_t>::max() ||
        event.unavailableAttributeHandles.size() >
            std::numeric_limits<std::uint32_t>::max()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership acquisition-if-available event has too many attributes.");
    }
    std::set<std::uint64_t> seenHandles;
    writer.unsigned64(event.requestId);
    writer.unsigned64(event.receivingFederateId);
    writer.unsigned64(event.objectInstanceHandle);
    writer.unsigned32(static_cast<std::uint32_t>(
        event.securedAttributeHandles.size()));
    for (std::uint64_t const attributeHandle : event.securedAttributeHandles) {
      requireNonzero(
          attributeHandle,
          "A process attribute-ownership acquisition-if-available event requires attribute identities.");
      seenHandles.insert(attributeHandle);
      writer.unsigned64(attributeHandle);
    }
    writer.unsigned32(static_cast<std::uint32_t>(
        event.unavailableAttributeHandles.size()));
    for (std::uint64_t const attributeHandle : event.unavailableAttributeHandles) {
      requireNonzero(
          attributeHandle,
          "A process attribute-ownership acquisition-if-available event requires attribute identities.");
      if (!seenHandles.insert(attributeHandle).second) {
        throw ProcessFederationServiceProtocolError(
            "A process attribute-ownership acquisition-if-available event repeats an attribute identity.");
      }
      writer.unsigned64(attributeHandle);
    }
    writer.bytes(event.userSuppliedTag);
  } else if (result.attributeOwnershipAcquisitionEvent.has_value()) {
    auto const& event = *result.attributeOwnershipAcquisitionEvent;
    if (event.kind != ProcessFederationAttributeOwnershipAcquisitionEventKind::
                    ownership_assumption) {
      requireNonzero(
          event.requestId,
          "A process attribute-ownership acquisition event requires a request identity.");
    }
    requireNonzero(
        event.requestingFederateId,
        "A process attribute-ownership acquisition event requires a requester identity.");
    requireNonzero(
        event.receivingFederateId,
        "A process attribute-ownership acquisition event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process attribute-ownership acquisition event requires an object instance.");
    if (event.attributeHandles.empty() ||
        event.attributeHandles.size() > std::numeric_limits<std::uint32_t>::max()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership acquisition event requires attributes.");
    }
    auto const encodedKind = static_cast<std::uint8_t>(event.kind);
    if (encodedKind > static_cast<std::uint8_t>(
                          ProcessFederationAttributeOwnershipAcquisitionEventKind::
                              confirm_divestiture_notification)) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership acquisition event has an invalid kind.");
    }
    writer.unsigned8(encodedKind);
    writer.unsigned64(event.requestId);
    writer.unsigned64(event.requestingFederateId);
    writer.unsigned64(event.receivingFederateId);
    writer.unsigned64(event.objectInstanceHandle);
    writer.unsigned32(static_cast<std::uint32_t>(event.attributeHandles.size()));
    for (std::uint64_t const attributeHandle : event.attributeHandles) {
      requireNonzero(
          attributeHandle,
          "A process attribute-ownership acquisition event requires attribute identities.");
      writer.unsigned64(attributeHandle);
    }
    writer.bytes(event.userSuppliedTag);
    if (event.kind == ProcessFederationAttributeOwnershipAcquisitionEventKind::
                      request_divestiture_confirmation) {
      writer.unsigned8(event.candidateIsIfAvailable ? 1U : 0U);
    }
  } else if (result.attributeOwnershipUnavailableEvent.has_value()) {
    auto const& event = *result.attributeOwnershipUnavailableEvent;
    requireNonzero(
        event.receivingFederateId,
        "A process attribute-ownership unavailable event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process attribute-ownership unavailable event requires an object instance.");
    if (event.attributeHandles.empty() ||
        event.attributeHandles.size() > std::numeric_limits<std::uint32_t>::max()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership unavailable event requires attributes.");
    }
    writer.unsigned64(event.receivingFederateId);
    writer.unsigned64(event.objectInstanceHandle);
    writer.unsigned32(static_cast<std::uint32_t>(event.attributeHandles.size()));
    for (std::uint64_t const attributeHandle : event.attributeHandles) {
      requireNonzero(
          attributeHandle,
          "A process attribute-ownership unavailable event requires attribute identities.");
      writer.unsigned64(attributeHandle);
    }
    writer.bytes(event.userSuppliedTag);
  } else if (result.attributeTransportationTypeChangeEvent.has_value()) {
    auto const& event = *result.attributeTransportationTypeChangeEvent;
    requireNonzero(
        event.requestId,
        "A process attribute transportation-type change event requires a request identity.");
    requireNonzero(
        event.receivingFederateId,
        "A process attribute transportation-type change event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process attribute transportation-type change event requires an object instance.");
    if (event.attributeHandles.empty() || event.transportationName.empty() ||
        event.attributeHandles.size() > std::numeric_limits<std::uint32_t>::max()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute transportation-type change event requires attributes and transportation.");
    }
    writer.unsigned64(event.requestId);
    writer.unsigned64(event.receivingFederateId);
    writer.unsigned64(event.objectInstanceHandle);
    writer.unsigned32(static_cast<std::uint32_t>(event.attributeHandles.size()));
    for (std::uint64_t const attributeHandle : event.attributeHandles) {
      requireNonzero(
          attributeHandle,
          "A process attribute transportation-type change event requires attribute identities.");
      writer.unsigned64(attributeHandle);
    }
    writer.string(event.transportationName);
  } else if (result.attributeTransportationTypeQueryEvent.has_value()) {
    auto const& event = *result.attributeTransportationTypeQueryEvent;
    requireNonzero(
        event.receivingFederateId,
        "A process attribute transportation-type query event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process attribute transportation-type query event requires an object instance.");
    requireNonzero(
        event.attributeHandle,
        "A process attribute transportation-type query event requires an attribute identity.");
    if (event.transportationName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute transportation-type query event requires transportation.");
    }
    writer.unsigned64(event.receivingFederateId);
    writer.unsigned64(event.objectInstanceHandle);
    writer.unsigned64(event.attributeHandle);
    writer.string(event.transportationName);
  } else if (result.interactionTransportationTypeChangeEvent.has_value()) {
    auto const& event = *result.interactionTransportationTypeChangeEvent;
    requireNonzero(
        event.receivingFederateId,
        "A process interaction transportation-type change event requires a recipient identity.");
    requireNonzero(
        event.interactionClassHandle,
        "A process interaction transportation-type change event requires an interaction class.");
    if (event.transportationName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process interaction transportation-type change event requires transportation.");
    }
    writer.unsigned64(event.receivingFederateId);
    writer.unsigned64(event.interactionClassHandle);
    writer.string(event.transportationName);
  } else if (result.interactionTransportationTypeQueryEvent.has_value()) {
    auto const& event = *result.interactionTransportationTypeQueryEvent;
    requireNonzero(
        event.receivingFederateId,
        "A process interaction transportation-type query event requires a recipient identity.");
    requireNonzero(
        event.queriedFederateId,
        "A process interaction transportation-type query event requires a queried federate identity.");
    requireNonzero(
        event.interactionClassHandle,
        "A process interaction transportation-type query event requires an interaction class.");
    if (event.transportationName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process interaction transportation-type query event requires transportation.");
    }
    writer.unsigned64(event.receivingFederateId);
    writer.unsigned64(event.queriedFederateId);
    writer.unsigned64(event.interactionClassHandle);
    writer.string(event.transportationName);
  } else if (result.synchronizationPointAnnouncementEvent.has_value()) {
    auto const& event = *result.synchronizationPointAnnouncementEvent;
    requireNonzero(
        event.receivingFederateId,
        "A process synchronization-point announcement requires a recipient identity.");
    if (event.label.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process synchronization-point announcement requires a label.");
    }
    writer.unsigned64(event.receivingFederateId);
    writer.wideString(event.label);
    writer.bytes(event.userSuppliedTag);
  } else if (result.federationSynchronizedEvent.has_value()) {
    auto const& event = *result.federationSynchronizedEvent;
    requireNonzero(
        event.receivingFederateId,
        "A process Federation Synchronized event requires a recipient identity.");
    if (event.label.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process Federation Synchronized event requires a label.");
    }
    auto const failedToSyncFederateIds =
        parameterVector(event.failedToSyncFederateIds);
    validateHandleVector(
        failedToSyncFederateIds,
        "A process Federation Synchronized event requires sorted, unique federate handles.");
    writer.unsigned64(event.receivingFederateId);
    writer.wideString(event.label);
    writer.unsigned64Vector(failedToSyncFederateIds);
  } else if (result.saveEvent.has_value()) {
    auto const& event = *result.saveEvent;
    requireNonzero(
        event.receivingFederateId,
        "A process federation-save event requires a recipient identity.");
    if (event.kind != FederationSaveNotificationKind::status &&
        event.label.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process federation-save event requires a label.");
    }
    if (event.kind == FederationSaveNotificationKind::status &&
        !event.label.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process federation-save status event cannot carry a label.");
    }
    if (event.kind != FederationSaveNotificationKind::initiate &&
        event.kind != FederationSaveNotificationKind::completed &&
        event.kind != FederationSaveNotificationKind::status) {
      throw ProcessFederationServiceProtocolError(
          "A process federation-save event has an unsupported notification kind.");
    }
    if (event.kind != FederationSaveNotificationKind::status &&
        !event.statuses.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A non-status federation-save event cannot carry status pairs.");
    }
    if (event.kind == FederationSaveNotificationKind::status &&
        event.statuses.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A federation-save status event requires status pairs.");
    }
    if (event.kind != FederationSaveNotificationKind::initiate &&
        event.timestamp) {
      throw ProcessFederationServiceProtocolError(
          "Only an initiate federation-save event may carry a timestamp.");
    }
    if (event.statuses.size() > std::numeric_limits<std::uint32_t>::max()) {
      throw ProcessFederationServiceProtocolError(
          "A federation-save status event has too many status pairs.");
    }
    auto const failureReason = static_cast<std::uint8_t>(event.failureReason);
    if (failureReason > static_cast<std::uint8_t>(rti1516_2025::SAVE_ABORTED)) {
      throw ProcessFederationServiceProtocolError(
          "A process federation-save event has an invalid failure reason.");
    }
    writer.unsigned8(static_cast<std::uint8_t>(event.kind));
    writer.unsigned64(event.receivingFederateId);
    writer.wideString(event.label);
    writer.unsigned8(event.successful ? 1U : 0U);
    writer.unsigned8(failureReason);
    writer.unsigned32(static_cast<std::uint32_t>(event.statuses.size()));
    std::uint64_t previousFederateId = 0U;
    for (auto const& [federateId, status] : event.statuses) {
      requireNonzero(
          federateId,
          "A federation-save status event requires federate identities.");
      if (federateId <= previousFederateId) {
        throw ProcessFederationServiceProtocolError(
            "A federation-save status event requires sorted, unique federate identities.");
      }
      switch (status) {
        case rti1516_2025::NO_SAVE_IN_PROGRESS:
        case rti1516_2025::FEDERATE_INSTRUCTED_TO_SAVE:
        case rti1516_2025::FEDERATE_SAVING:
        case rti1516_2025::FEDERATE_WAITING_FOR_FEDERATION_TO_SAVE:
          break;
        default:
          throw ProcessFederationServiceProtocolError(
              "A federation-save status event has an invalid SaveStatus.");
      }
      writer.unsigned64(federateId);
      writer.unsigned8(static_cast<std::uint8_t>(status));
      previousFederateId = federateId;
    }
    writeOptionalLogicalTime(writer, event.timestamp);
  } else if (result.restoreEvent.has_value()) {
    auto const& event = *result.restoreEvent;
    requireNonzero(
        event.receivingFederateId,
        "A process federation-restore event requires a recipient identity.");
    if (event.label.empty() &&
        event.kind != FederationRestoreNotificationKind::begin &&
        event.kind != FederationRestoreNotificationKind::status) {
      throw ProcessFederationServiceProtocolError(
          "A process federation-restore event requires a label.");
    }
    auto const encodedKind = static_cast<std::uint8_t>(event.kind);
    if (encodedKind > static_cast<std::uint8_t>(
                          FederationRestoreNotificationKind::status)) {
      throw ProcessFederationServiceProtocolError(
          "A process federation-restore event has an invalid notification kind.");
    }
    auto const failureReason = static_cast<std::uint8_t>(event.failureReason);
    if (failureReason > static_cast<std::uint8_t>(
                             rti1516_2025::RESTORE_ABORTED)) {
      throw ProcessFederationServiceProtocolError(
          "A process federation-restore event has an invalid failure reason.");
    }
    if (event.statuses.size() > std::numeric_limits<std::uint32_t>::max()) {
      throw ProcessFederationServiceProtocolError(
          "A process federation-restore event has too many status records.");
    }
    writer.unsigned8(encodedKind);
    writer.unsigned64(event.receivingFederateId);
    writer.wideString(event.label);
    writer.wideString(event.federateName);
    writer.unsigned64(event.preRestoreFederateId);
    writer.unsigned64(event.postRestoreFederateId);
    writer.unsigned8(event.successful ? 1U : 0U);
    writer.unsigned8(failureReason);
    writer.unsigned32(static_cast<std::uint32_t>(event.statuses.size()));
    std::uint64_t previousPreRestoreId = 0U;
    for (auto const& status : event.statuses) {
      requireNonzero(
          status.preRestoreFederateId,
          "A federation-restore status event requires pre-restore federate identities.");
      if (status.preRestoreFederateId <= previousPreRestoreId) {
        throw ProcessFederationServiceProtocolError(
            "A federation-restore status event requires sorted, unique pre-restore federate identities.");
      }
      auto const encodedStatus = static_cast<std::uint8_t>(status.status);
      if (encodedStatus > static_cast<std::uint8_t>(
              rti1516_2025::FEDERATE_WAITING_FOR_FEDERATION_TO_RESTORE)) {
        throw ProcessFederationServiceProtocolError(
            "A federation-restore status event has an invalid RestoreStatus.");
      }
      if (status.status == rti1516_2025::NO_RESTORE_IN_PROGRESS) {
        if (status.postRestoreFederateId != 0U) {
          throw ProcessFederationServiceProtocolError(
              "A no-restore-in-progress status event requires an invalid post-restore federate identity.");
        }
      } else {
        requireNonzero(
            status.postRestoreFederateId,
            "A federation-restore status event requires post-restore federate identities.");
      }
      writer.unsigned64(status.preRestoreFederateId);
      writer.unsigned64(status.postRestoreFederateId);
      writer.unsigned8(encodedStatus);
      previousPreRestoreId = status.preRestoreFederateId;
    }
  }
  return std::move(writer).finish();
}

ProcessFederationReceiveInteractionResult
decodeProcessFederationReceiveInteractionResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const eventKind = reader.unsigned8();
  if (eventKind > 19U) {
    throw ProcessFederationServiceProtocolError(
        "A process federation receive result has an invalid event marker.");
  }
  ProcessFederationReceiveInteractionResult result;
  if (eventKind == 1U) {
    ProcessFederationInteractionEvent event;
    event.producingFederateId = reader.unsigned64();
    event.receivingFederateId = reader.unsigned64();
    event.interactionClassHandle = reader.unsigned64();
    event.parameterHandles = reader.unsigned64Vector();
    event.payload = reader.bytes();
    event.transportationName = reader.string();
    event.timestamp = readOptionalLogicalTime(reader);
    if (reader.remaining() != 0U) {
      auto const hasTarget = reader.unsigned8();
      if (hasTarget > 1U) {
        throw ProcessFederationServiceProtocolError(
            "A process interaction event has an invalid directed-target marker.");
      }
      if (hasTarget != 0U) {
        event.objectInstanceHandle = reader.unsigned64();
        requireNonzero(
            *event.objectInstanceHandle,
            "A process directed-interaction event requires a target object instance.");
      }
      if (reader.remaining() != 0U) {
        auto const hasRetraction = reader.unsigned8();
        if (hasRetraction > 1U) {
          throw ProcessFederationServiceProtocolError(
              "A process interaction event has an invalid retraction marker.");
        }
        if (hasRetraction != 0U) {
          event.retractionMessageId = reader.unsigned64();
          requireNonzero(
              *event.retractionMessageId,
              "A process directed-interaction event requires a retraction identity.");
        }
      }
      readInteractionRegionMetadata(reader, event);
      readInteractionOrderMetadata(reader, event);
    }
    requireNonzero(
        event.producingFederateId,
        "A process federation interaction event requires a producer identity.");
    requireNonzero(
        event.receivingFederateId,
        "A process federation interaction event requires a recipient identity.");
    requireNonzero(
        event.interactionClassHandle,
        "A process federation interaction event requires an interaction class.");
    result.event = std::move(event);
  } else if (eventKind == 2U) {
    ProcessFederationAttributeUpdateEvent event;
    event.producingFederateId = reader.unsigned64();
    event.receivingFederateId = reader.unsigned64();
    event.objectInstanceHandle = reader.unsigned64();
    auto const attributeCount = reader.unsigned32();
    event.attributeValues.reserve(attributeCount);
    std::set<std::uint64_t> seenHandles;
    for (std::uint32_t index = 0U; index < attributeCount; ++index) {
      auto const attributeHandle = reader.unsigned64();
      requireNonzero(
          attributeHandle,
          "A process federation attribute event requires attribute identities.");
      if (!seenHandles.insert(attributeHandle).second) {
        throw ProcessFederationServiceProtocolError(
            "A process federation attribute event repeats an attribute identity.");
      }
      event.attributeValues.emplace_back(attributeHandle, reader.bytes());
    }
    event.userSuppliedTag = reader.bytes();
    event.transportationName = reader.string();
    readAttributeUpdateRegionMetadata(reader, event);
    event.timestamp = readOptionalLogicalTime(reader);
    if (reader.remaining() != 0U) {
      event.retractionMessageId = reader.unsigned64();
      requireNonzero(
          *event.retractionMessageId,
          "A process attribute event requires a retraction identity.");
    }
    requireNonzero(
        event.producingFederateId,
        "A process federation attribute event requires a producer identity.");
    requireNonzero(
        event.receivingFederateId,
        "A process federation attribute event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process federation attribute event requires an object instance.");
    if (event.attributeValues.empty() || event.transportationName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process federation attribute event requires values and transportation.");
    }
    result.attributeEvent = std::move(event);
  } else if (eventKind == 3U) {
    ProcessFederationObjectInstanceDiscoveryEvent event;
    event.receivingFederateId = reader.unsigned64();
    event.objectInstanceHandle = reader.unsigned64();
    event.objectClassHandle = reader.unsigned64();
    event.objectInstanceName = reader.wideString();
    event.producingFederateId = reader.unsigned64();
    requireNonzero(
        event.receivingFederateId,
        "A process federation discovery event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process federation discovery event requires an object instance.");
    requireNonzero(
        event.objectClassHandle,
        "A process federation discovery event requires an object class.");
    requireNonzero(
        event.producingFederateId,
        "A process federation discovery event requires a producer identity.");
    if (event.objectInstanceName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process federation discovery event requires an object instance name.");
    }
    result.discoveryEvent = std::move(event);
  } else if (eventKind == 4U) {
    ProcessFederationObjectInstanceScopeChangeEvent event;
    event.receivingFederateId = reader.unsigned64();
    event.objectInstanceHandle = reader.unsigned64();
    auto const attributeCount = reader.unsigned32();
    for (std::uint32_t index = 0U; index < attributeCount; ++index) {
      auto const attributeHandle = reader.unsigned64();
      requireNonzero(
          attributeHandle,
          "A process scope event requires attribute identities.");
      if (!event.attributeHandles.insert(attributeHandle).second) {
        throw ProcessFederationServiceProtocolError(
            "A process scope event repeats an attribute identity.");
      }
    }
    auto const inScope = reader.unsigned8();
    if (inScope > 1U) {
      throw ProcessFederationServiceProtocolError(
          "A process scope event has an invalid scope marker.");
    }
    event.inScope = inScope != 0U;
    requireNonzero(
        event.receivingFederateId,
        "A process scope event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process scope event requires an object instance.");
    if (event.attributeHandles.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process scope event requires attributes.");
    }
    result.scopeChangeEvent = std::move(event);
  } else if (eventKind == 6U) {
    ProcessFederationObjectInstanceRemovalEvent event;
    event.receivingFederateId = reader.unsigned64();
    event.objectInstanceHandle = reader.unsigned64();
    event.producingFederateId = reader.unsigned64();
    event.userSuppliedTag = reader.bytes();
    event.timestamp = readOptionalLogicalTime(reader);
    auto const retractionMessageId = readOptionalMessageId(reader);
    if (retractionMessageId != 0U) {
      event.retractionMessageId = retractionMessageId;
      if (reader.remaining() != 0U) {
        auto const provideRetraction = reader.unsigned8();
        if (provideRetraction > 1U) {
          throw ProcessFederationServiceProtocolError(
              "A process object removal event has an invalid retraction projection marker.");
        }
        event.provideRetraction = provideRetraction != 0U;
      } else {
        // Legacy process payloads had no projection marker and therefore
        // treated a present message id as publicly retraction-capable.
        event.provideRetraction = true;
      }
    }
    if (event.timestamp && reader.remaining() != 0U) {
      auto const orderMarker = reader.unsigned8();
      if (orderMarker > 1U) {
        throw ProcessFederationServiceProtocolError(
            "A process object removal event has an invalid order metadata marker.");
      }
      if (orderMarker != 0U) {
        auto decodeOrder = [](std::uint8_t encoded, char const* description) {
          if (encoded != static_cast<std::uint8_t>(rti1516_2025::RECEIVE) &&
              encoded != static_cast<std::uint8_t>(rti1516_2025::TIMESTAMP)) {
            throw ProcessFederationServiceProtocolError(description);
          }
          return static_cast<rti1516_2025::OrderType>(encoded);
        };
        event.sentOrderType = decodeOrder(
            reader.unsigned8(),
            "A process object removal event has an invalid sent order classification.");
        event.receivedOrderType = decodeOrder(
            reader.unsigned8(),
            "A process object removal event has an invalid received order classification.");
      }
    }
    requireNonzero(
        event.receivingFederateId,
        "A process object removal event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process object removal event requires an object instance.");
    requireNonzero(
        event.producingFederateId,
        "A process object removal event requires a producer identity.");
    result.removalEvent = std::move(event);
  } else if (eventKind == 5U) {
    ProcessFederationAttributeRelevanceAdvisoryEvent event;
    event.providingFederateId = reader.unsigned64();
    event.receivingFederateId = reader.unsigned64();
    event.objectInstanceHandle = reader.unsigned64();
    auto const attributeCount = reader.unsigned32();
    for (std::uint32_t index = 0U; index < attributeCount; ++index) {
      auto const attributeHandle = reader.unsigned64();
      requireNonzero(
          attributeHandle,
          "A process attribute relevance advisory requires attribute identities.");
      if (!event.attributeHandles.insert(attributeHandle).second) {
        throw ProcessFederationServiceProtocolError(
            "A process attribute relevance advisory repeats an attribute identity.");
      }
    }
    auto const turnUpdatesOn = reader.unsigned8();
    if (turnUpdatesOn > 1U) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute relevance advisory has an invalid direction marker.");
    }
    event.turnUpdatesOn = turnUpdatesOn != 0U;
    auto const hasUpdateRateDesignator = reader.unsigned8();
    if (hasUpdateRateDesignator > 1U) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute relevance advisory has an invalid update-rate marker.");
    }
    if (hasUpdateRateDesignator != 0U) {
      event.updateRateDesignator = reader.string();
      if (event.updateRateDesignator->empty()) {
        throw ProcessFederationServiceProtocolError(
            "A process attribute relevance advisory cannot carry an empty update-rate designator.");
      }
    }
    requireNonzero(
        event.providingFederateId,
        "A process attribute relevance advisory requires an owning federate.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process attribute relevance advisory requires an object instance.");
    if (event.attributeHandles.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute relevance advisory requires attributes.");
    }
    result.attributeRelevanceAdvisoryEvent = std::move(event);
  } else if (eventKind == 7U) {
    ProcessFederationAttributeValueUpdateRequestEvent event;
    event.requestingFederateId = reader.unsigned64();
    event.providingFederateId = reader.unsigned64();
    event.objectInstanceHandle = reader.unsigned64();
    auto const attributeCount = reader.unsigned32();
    for (std::uint32_t index = 0U; index < attributeCount; ++index) {
      auto const attributeHandle = reader.unsigned64();
      requireNonzero(
          attributeHandle,
          "A process attribute-value-update request event requires attribute identities.");
      if (!event.requestedAttributeHandles.insert(attributeHandle).second) {
        throw ProcessFederationServiceProtocolError(
            "A process attribute-value-update request event repeats an attribute identity.");
      }
    }
    event.userSuppliedTag = reader.bytes();
    requireNonzero(
        event.requestingFederateId,
        "A process attribute-value-update request event requires a requester.");
    requireNonzero(
        event.providingFederateId,
        "A process attribute-value-update request event requires a provider.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process attribute-value-update request event requires an object instance.");
    if (event.requestedAttributeHandles.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-value-update request event requires attributes.");
    }
    result.attributeValueUpdateRequestEvent = std::move(event);
  } else if (eventKind == 8U) {
    ProcessFederationAttributeOwnershipQueryEvent event;
    event.requestId = reader.unsigned64();
    event.receivingFederateId = reader.unsigned64();
    event.objectInstanceHandle = reader.unsigned64();
    auto const encodedReportKind = reader.unsigned8();
    if (encodedReportKind > static_cast<std::uint8_t>(
                                AttributeOwnershipQueryReportKind::rti)) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership query event has an invalid report kind.");
    }
    event.reportKind = static_cast<AttributeOwnershipQueryReportKind>(
        encodedReportKind);
    event.owningFederateId = reader.unsigned64();
    auto const attributeCount = reader.unsigned32();
    for (std::uint32_t index = 0U; index < attributeCount; ++index) {
      auto const attributeHandle = reader.unsigned64();
      requireNonzero(
          attributeHandle,
          "A process attribute-ownership query event requires attribute identities.");
      if (!event.attributeHandles.insert(attributeHandle).second) {
        throw ProcessFederationServiceProtocolError(
            "A process attribute-ownership query event repeats an attribute identity.");
      }
    }
    requireNonzero(
        event.requestId,
        "A process attribute-ownership query event requires a request identity.");
    requireNonzero(
        event.receivingFederateId,
        "A process attribute-ownership query event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process attribute-ownership query event requires an object instance.");
    if (event.attributeHandles.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership query event requires attributes.");
    }
    if (event.reportKind == AttributeOwnershipQueryReportKind::federate) {
      requireNonzero(
          event.owningFederateId,
          "A federate-owned process attribute-ownership query event requires an owner.");
    } else if (event.owningFederateId != 0U) {
      throw ProcessFederationServiceProtocolError(
          "A non-federate process attribute-ownership query event cannot carry an owner.");
    }
    result.attributeOwnershipQueryEvent = std::move(event);
  } else if (eventKind == 9U) {
    ProcessFederationAttributeOwnershipAcquisitionIfAvailableEvent event;
    event.requestId = reader.unsigned64();
    event.receivingFederateId = reader.unsigned64();
    event.objectInstanceHandle = reader.unsigned64();
    auto const securedCount = reader.unsigned32();
    for (std::uint32_t index = 0U; index < securedCount; ++index) {
      auto const attributeHandle = reader.unsigned64();
      requireNonzero(
          attributeHandle,
          "A process attribute-ownership acquisition-if-available event requires attribute identities.");
      if (!event.securedAttributeHandles.insert(attributeHandle).second) {
        throw ProcessFederationServiceProtocolError(
            "A process attribute-ownership acquisition-if-available event repeats an attribute identity.");
      }
    }
    auto const unavailableCount = reader.unsigned32();
    for (std::uint32_t index = 0U; index < unavailableCount; ++index) {
      auto const attributeHandle = reader.unsigned64();
      requireNonzero(
          attributeHandle,
          "A process attribute-ownership acquisition-if-available event requires attribute identities.");
      if (event.securedAttributeHandles.contains(attributeHandle) ||
          !event.unavailableAttributeHandles.insert(attributeHandle).second) {
        throw ProcessFederationServiceProtocolError(
            "A process attribute-ownership acquisition-if-available event repeats an attribute identity.");
      }
    }
    event.userSuppliedTag = reader.bytes();
    requireNonzero(
        event.requestId,
        "A process attribute-ownership acquisition-if-available event requires a request identity.");
    requireNonzero(
        event.receivingFederateId,
        "A process attribute-ownership acquisition-if-available event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process attribute-ownership acquisition-if-available event requires an object instance.");
    if (event.securedAttributeHandles.empty() &&
        event.unavailableAttributeHandles.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership acquisition-if-available event requires a delivery.");
    }
    result.attributeOwnershipAcquisitionIfAvailableEvent = std::move(event);
  } else if (eventKind == 10U) {
    ProcessFederationAttributeOwnershipAcquisitionEvent event;
    auto const encodedKind = reader.unsigned8();
    if (encodedKind > static_cast<std::uint8_t>(
                          ProcessFederationAttributeOwnershipAcquisitionEventKind::
                              confirm_divestiture_notification)) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership acquisition event has an invalid kind.");
    }
    event.kind = static_cast<
        ProcessFederationAttributeOwnershipAcquisitionEventKind>(encodedKind);
    event.requestId = reader.unsigned64();
    event.requestingFederateId = reader.unsigned64();
    event.receivingFederateId = reader.unsigned64();
    event.objectInstanceHandle = reader.unsigned64();
    auto const attributeCount = reader.unsigned32();
    for (std::uint32_t index = 0U; index < attributeCount; ++index) {
      auto const attributeHandle = reader.unsigned64();
      requireNonzero(
          attributeHandle,
          "A process attribute-ownership acquisition event requires attribute identities.");
      if (!event.attributeHandles.insert(attributeHandle).second) {
        throw ProcessFederationServiceProtocolError(
            "A process attribute-ownership acquisition event repeats an attribute identity.");
      }
    }
    event.userSuppliedTag = reader.bytes();
    if (event.kind == ProcessFederationAttributeOwnershipAcquisitionEventKind::
                        request_divestiture_confirmation) {
      auto const candidateIsIfAvailable = reader.unsigned8();
      if (candidateIsIfAvailable > 1U) {
        throw ProcessFederationServiceProtocolError(
            "A process request-divestiture-confirmation event has an invalid candidate marker.");
      }
      event.candidateIsIfAvailable = candidateIsIfAvailable != 0U;
    }
    if (event.kind != ProcessFederationAttributeOwnershipAcquisitionEventKind::
                    ownership_assumption) {
      requireNonzero(
          event.requestId,
          "A process attribute-ownership acquisition event requires a request identity.");
    }
    requireNonzero(
        event.requestingFederateId,
        "A process attribute-ownership acquisition event requires a requester identity.");
    requireNonzero(
        event.receivingFederateId,
        "A process attribute-ownership acquisition event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process attribute-ownership acquisition event requires an object instance.");
    if (event.attributeHandles.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership acquisition event requires attributes.");
    }
    result.attributeOwnershipAcquisitionEvent = std::move(event);
  } else if (eventKind == 11U) {
    ProcessFederationAttributeOwnershipUnavailableEvent event;
    event.receivingFederateId = reader.unsigned64();
    event.objectInstanceHandle = reader.unsigned64();
    auto const attributeCount = reader.unsigned32();
    for (std::uint32_t index = 0U; index < attributeCount; ++index) {
      auto const attributeHandle = reader.unsigned64();
      requireNonzero(
          attributeHandle,
          "A process attribute-ownership unavailable event requires attribute identities.");
      if (!event.attributeHandles.insert(attributeHandle).second) {
        throw ProcessFederationServiceProtocolError(
            "A process attribute-ownership unavailable event repeats an attribute identity.");
      }
    }
    event.userSuppliedTag = reader.bytes();
    requireNonzero(
        event.receivingFederateId,
        "A process attribute-ownership unavailable event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process attribute-ownership unavailable event requires an object instance.");
    if (event.attributeHandles.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership unavailable event requires attributes.");
    }
    result.attributeOwnershipUnavailableEvent = std::move(event);
  } else if (eventKind == 12U) {
    ProcessFederationAttributeTransportationTypeChangeEvent event;
    event.requestId = reader.unsigned64();
    event.receivingFederateId = reader.unsigned64();
    event.objectInstanceHandle = reader.unsigned64();
    auto const attributeCount = reader.unsigned32();
    for (std::uint32_t index = 0U; index < attributeCount; ++index) {
      auto const attributeHandle = reader.unsigned64();
      requireNonzero(
          attributeHandle,
          "A process attribute transportation-type change event requires attribute identities.");
      if (!event.attributeHandles.insert(attributeHandle).second) {
        throw ProcessFederationServiceProtocolError(
            "A process attribute transportation-type change event repeats an attribute identity.");
      }
    }
    event.transportationName = reader.string();
    requireNonzero(
        event.requestId,
        "A process attribute transportation-type change event requires a request identity.");
    requireNonzero(
        event.receivingFederateId,
        "A process attribute transportation-type change event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process attribute transportation-type change event requires an object instance.");
    if (event.attributeHandles.empty() || event.transportationName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute transportation-type change event requires attributes and transportation.");
    }
    result.attributeTransportationTypeChangeEvent = std::move(event);
  } else if (eventKind == 13U) {
    ProcessFederationAttributeTransportationTypeQueryEvent event;
    event.receivingFederateId = reader.unsigned64();
    event.objectInstanceHandle = reader.unsigned64();
    event.attributeHandle = reader.unsigned64();
    event.transportationName = reader.string();
    requireNonzero(
        event.receivingFederateId,
        "A process attribute transportation-type query event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process attribute transportation-type query event requires an object instance.");
    requireNonzero(
        event.attributeHandle,
        "A process attribute transportation-type query event requires an attribute identity.");
    if (event.transportationName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute transportation-type query event requires transportation.");
    }
    result.attributeTransportationTypeQueryEvent = std::move(event);
  } else if (eventKind == 14U) {
    ProcessFederationInteractionTransportationTypeChangeEvent event;
    event.receivingFederateId = reader.unsigned64();
    event.interactionClassHandle = reader.unsigned64();
    event.transportationName = reader.string();
    requireNonzero(
        event.receivingFederateId,
        "A process interaction transportation-type change event requires a recipient identity.");
    requireNonzero(
        event.interactionClassHandle,
        "A process interaction transportation-type change event requires an interaction class.");
    if (event.transportationName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process interaction transportation-type change event requires transportation.");
    }
    result.interactionTransportationTypeChangeEvent = std::move(event);
  } else if (eventKind == 15U) {
    ProcessFederationInteractionTransportationTypeQueryEvent event;
    event.receivingFederateId = reader.unsigned64();
    event.queriedFederateId = reader.unsigned64();
    event.interactionClassHandle = reader.unsigned64();
    event.transportationName = reader.string();
    requireNonzero(
        event.receivingFederateId,
        "A process interaction transportation-type query event requires a recipient identity.");
    requireNonzero(
        event.queriedFederateId,
        "A process interaction transportation-type query event requires a queried federate identity.");
    requireNonzero(
        event.interactionClassHandle,
        "A process interaction transportation-type query event requires an interaction class.");
    if (event.transportationName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process interaction transportation-type query event requires transportation.");
    }
    result.interactionTransportationTypeQueryEvent = std::move(event);
  } else if (eventKind == 16U) {
    ProcessFederationSynchronizationPointAnnouncementEvent event;
    event.receivingFederateId = reader.unsigned64();
    event.label = reader.wideString();
    event.userSuppliedTag = reader.bytes();
    requireNonzero(
        event.receivingFederateId,
        "A process synchronization-point announcement requires a recipient identity.");
    if (event.label.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process synchronization-point announcement requires a label.");
    }
    result.synchronizationPointAnnouncementEvent = std::move(event);
  } else if (eventKind == 17U) {
    ProcessFederationFederationSynchronizedEvent event;
    event.receivingFederateId = reader.unsigned64();
    event.label = reader.wideString();
    auto const failed = reader.unsigned64Vector();
    validateHandleVector(
        failed,
        "A process Federation Synchronized event requires sorted, unique federate handles.");
    event.failedToSyncFederateIds.insert(failed.begin(), failed.end());
    requireNonzero(
        event.receivingFederateId,
        "A process Federation Synchronized event requires a recipient identity.");
    if (event.label.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process Federation Synchronized event requires a label.");
    }
    result.federationSynchronizedEvent = std::move(event);
  } else if (eventKind == 18U) {
    ProcessFederationSaveEvent event;
    auto const encodedKind = reader.unsigned8();
    if (encodedKind > static_cast<std::uint8_t>(
                          FederationSaveNotificationKind::status)) {
      throw ProcessFederationServiceProtocolError(
          "A process federation-save event has an invalid notification kind.");
    }
    event.kind = static_cast<FederationSaveNotificationKind>(encodedKind);
    event.receivingFederateId = reader.unsigned64();
    event.label = reader.wideString();
    auto const successful = reader.unsigned8();
    if (successful > 1U) {
      throw ProcessFederationServiceProtocolError(
          "A process federation-save event has an invalid success marker.");
    }
    event.successful = successful != 0U;
    auto const failureReason = reader.unsigned8();
    if (failureReason > static_cast<std::uint8_t>(rti1516_2025::SAVE_ABORTED)) {
      throw ProcessFederationServiceProtocolError(
          "A process federation-save event has an invalid failure reason.");
    }
    event.failureReason =
        static_cast<rti1516_2025::SaveFailureReason>(failureReason);
    auto const statusCount = reader.unsigned32();
    event.statuses.reserve(statusCount);
    std::uint64_t previousFederateId = 0U;
    for (std::uint32_t index = 0U; index < statusCount; ++index) {
      auto const federateId = reader.unsigned64();
      requireNonzero(
          federateId,
          "A federation-save status event requires federate identities.");
      if (federateId <= previousFederateId) {
        throw ProcessFederationServiceProtocolError(
            "A federation-save status event requires sorted, unique federate identities.");
      }
      auto const encodedStatus = reader.unsigned8();
      if (encodedStatus > static_cast<std::uint8_t>(
                              rti1516_2025::FEDERATE_WAITING_FOR_FEDERATION_TO_SAVE)) {
        throw ProcessFederationServiceProtocolError(
            "A federation-save status event has an invalid SaveStatus.");
      }
      auto const status = static_cast<rti1516_2025::SaveStatus>(encodedStatus);
      switch (status) {
        case rti1516_2025::NO_SAVE_IN_PROGRESS:
        case rti1516_2025::FEDERATE_INSTRUCTED_TO_SAVE:
        case rti1516_2025::FEDERATE_SAVING:
        case rti1516_2025::FEDERATE_WAITING_FOR_FEDERATION_TO_SAVE:
          break;
        default:
          throw ProcessFederationServiceProtocolError(
              "A federation-save status event has an invalid SaveStatus.");
      }
      event.statuses.emplace_back(federateId, status);
      previousFederateId = federateId;
    }
    event.timestamp = readOptionalLogicalTime(reader);
    requireNonzero(
        event.receivingFederateId,
        "A process federation-save event requires a recipient identity.");
    if (event.kind != FederationSaveNotificationKind::status &&
        event.label.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process federation-save event requires a label.");
    }
    if (event.kind == FederationSaveNotificationKind::status &&
        (!event.label.empty() || event.statuses.empty())) {
      throw ProcessFederationServiceProtocolError(
          "A process federation-save status event requires status pairs and no label.");
    }
    if (event.kind != FederationSaveNotificationKind::status &&
        !event.statuses.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A non-status federation-save event cannot carry status pairs.");
    }
    if (event.kind != FederationSaveNotificationKind::initiate &&
        event.timestamp) {
      throw ProcessFederationServiceProtocolError(
          "Only an initiate federation-save event may carry a timestamp.");
    }
    result.saveEvent = std::move(event);
  } else if (eventKind == 19U) {
    ProcessFederationRestoreEvent event;
    auto const encodedKind = reader.unsigned8();
    if (encodedKind > static_cast<std::uint8_t>(
                          FederationRestoreNotificationKind::status)) {
      throw ProcessFederationServiceProtocolError(
          "A process federation-restore event has an invalid notification kind.");
    }
    event.kind = static_cast<FederationRestoreNotificationKind>(encodedKind);
    event.receivingFederateId = reader.unsigned64();
    event.label = reader.wideString();
    event.federateName = reader.wideString();
    event.preRestoreFederateId = reader.unsigned64();
    event.postRestoreFederateId = reader.unsigned64();
    auto const successful = reader.unsigned8();
    if (successful > 1U) {
      throw ProcessFederationServiceProtocolError(
          "A process federation-restore event has an invalid success marker.");
    }
    event.successful = successful != 0U;
    auto const failureReason = reader.unsigned8();
    if (failureReason > static_cast<std::uint8_t>(
                             rti1516_2025::RESTORE_ABORTED)) {
      throw ProcessFederationServiceProtocolError(
          "A process federation-restore event has an invalid failure reason.");
    }
    event.failureReason =
        static_cast<rti1516_2025::RestoreFailureReason>(failureReason);
    auto const statusCount = reader.unsigned32();
    event.statuses.reserve(statusCount);
    std::uint64_t previousPreRestoreId = 0U;
    for (std::uint32_t index = 0U; index < statusCount; ++index) {
      ProcessFederationRestoreEvent::StatusRecord status;
      status.preRestoreFederateId = reader.unsigned64();
      status.postRestoreFederateId = reader.unsigned64();
      requireNonzero(
          status.preRestoreFederateId,
          "A federation-restore status event requires pre-restore federate identities.");
      if (status.preRestoreFederateId <= previousPreRestoreId) {
        throw ProcessFederationServiceProtocolError(
            "A federation-restore status event requires sorted, unique pre-restore federate identities.");
      }
      auto const encodedStatus = reader.unsigned8();
      if (encodedStatus > static_cast<std::uint8_t>(
                              rti1516_2025::FEDERATE_WAITING_FOR_FEDERATION_TO_RESTORE)) {
        throw ProcessFederationServiceProtocolError(
            "A federation-restore status event has an invalid RestoreStatus.");
      }
      status.status = static_cast<rti1516_2025::RestoreStatus>(encodedStatus);
      if (status.status == rti1516_2025::NO_RESTORE_IN_PROGRESS) {
        if (status.postRestoreFederateId != 0U) {
          throw ProcessFederationServiceProtocolError(
              "A no-restore-in-progress status event requires an invalid post-restore federate identity.");
        }
      } else {
        requireNonzero(
            status.postRestoreFederateId,
            "A federation-restore status event requires post-restore federate identities.");
      }
      event.statuses.push_back(status);
      previousPreRestoreId = status.preRestoreFederateId;
    }
    requireNonzero(
        event.receivingFederateId,
        "A process federation-restore event requires a recipient identity.");
    if (event.label.empty() &&
        event.kind != FederationRestoreNotificationKind::begin &&
        event.kind != FederationRestoreNotificationKind::status) {
      throw ProcessFederationServiceProtocolError(
          "A process federation-restore event requires a label.");
    }
    result.restoreEvent = std::move(event);
  }
  reader.finish();
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationUpdateAttributeValuesResult(
    ProcessFederationUpdateAttributeValuesResult const& result) {
  PayloadWriter writer;
  writer.unsigned32(result.recipientCount);
  // Keep the ordinary response byte-for-byte compatible. The optional field
  // is present only for an accepted timestamp-ordered message.
  if (result.messageId != 0U) {
    writer.unsigned64(result.messageId);
  }
  return std::move(writer).finish();
}

ProcessFederationUpdateAttributeValuesResult
decodeProcessFederationUpdateAttributeValuesResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationUpdateAttributeValuesResult result{reader.unsigned32()};
  if (reader.remaining() != 0U) {
    if (reader.remaining() != sizeof(std::uint64_t)) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-update result has an invalid message identity.");
    }
    result.messageId = reader.unsigned64();
    requireNonzero(
        result.messageId,
        "A process attribute-update result cannot carry a zero message identity.");
  }
  reader.finish();
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationRequestAttributeValueUpdateResult(
    ProcessFederationRequestAttributeValueUpdateResult const& result) {
  PayloadWriter writer;
  writer.unsigned32(result.recipientCount);
  return std::move(writer).finish();
}

ProcessFederationRequestAttributeValueUpdateResult
decodeProcessFederationRequestAttributeValueUpdateResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationRequestAttributeValueUpdateResult result{
      reader.unsigned32()};
  reader.finish();
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationReceiveAttributeUpdateResult(
    ProcessFederationReceiveAttributeUpdateResult const& result) {
  PayloadWriter writer;
  writer.unsigned8(result.event.has_value() ? 1U : 0U);
  if (result.event.has_value()) {
    auto const& event = *result.event;
    requireNonzero(
        event.producingFederateId,
        "A process federation attribute event requires a producer identity.");
    requireNonzero(
        event.receivingFederateId,
        "A process federation attribute event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process federation attribute event requires an object instance.");
    if (event.attributeValues.empty() || event.transportationName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process federation attribute event requires values and transportation.");
    }
    if (event.attributeValues.size() >
        std::numeric_limits<std::uint32_t>::max()) {
      throw ProcessFederationServiceProtocolError(
          "A process federation attribute event has too many attributes.");
    }
    writer.unsigned64(event.producingFederateId);
    writer.unsigned64(event.receivingFederateId);
    writer.unsigned64(event.objectInstanceHandle);
    writer.unsigned32(static_cast<std::uint32_t>(event.attributeValues.size()));
    std::set<std::uint64_t> seenHandles;
    for (auto const& [attributeHandle, value] : event.attributeValues) {
      requireNonzero(
          attributeHandle,
          "A process federation attribute event requires attribute identities.");
      if (!seenHandles.insert(attributeHandle).second) {
        throw ProcessFederationServiceProtocolError(
            "A process federation attribute event repeats an attribute identity.");
      }
      writer.unsigned64(attributeHandle);
      writer.bytes(value);
    }
    writer.bytes(event.userSuppliedTag);
    writer.string(event.transportationName);
    writeAttributeUpdateRegionMetadata(writer, event);
    writeOptionalLogicalTime(writer, event.timestamp);
    if (event.retractionMessageId) {
      requireNonzero(
          *event.retractionMessageId,
          "A process attribute event requires a retraction identity.");
      writer.unsigned64(*event.retractionMessageId);
    }
  }
  return std::move(writer).finish();
}

ProcessFederationReceiveAttributeUpdateResult
decodeProcessFederationReceiveAttributeUpdateResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const hasEvent = reader.unsigned8();
  if (hasEvent > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process federation attribute result has an invalid event marker.");
  }
  ProcessFederationReceiveAttributeUpdateResult result;
  if (hasEvent != 0U) {
    ProcessFederationAttributeUpdateEvent event;
    event.producingFederateId = reader.unsigned64();
    event.receivingFederateId = reader.unsigned64();
    event.objectInstanceHandle = reader.unsigned64();
    auto const attributeCount = reader.unsigned32();
    event.attributeValues.reserve(attributeCount);
    std::set<std::uint64_t> seenHandles;
    for (std::uint32_t index = 0U; index < attributeCount; ++index) {
      auto const attributeHandle = reader.unsigned64();
      requireNonzero(
          attributeHandle,
          "A process federation attribute event requires attribute identities.");
      if (!seenHandles.insert(attributeHandle).second) {
        throw ProcessFederationServiceProtocolError(
            "A process federation attribute event repeats an attribute identity.");
      }
      event.attributeValues.emplace_back(attributeHandle, reader.bytes());
    }
    event.userSuppliedTag = reader.bytes();
    event.transportationName = reader.string();
    readAttributeUpdateRegionMetadata(reader, event);
    event.timestamp = readOptionalLogicalTime(reader);
    if (reader.remaining() != 0U) {
      event.retractionMessageId = reader.unsigned64();
      requireNonzero(
          *event.retractionMessageId,
          "A process attribute event requires a retraction identity.");
    }
    requireNonzero(
        event.producingFederateId,
        "A process federation attribute event requires a producer identity.");
    requireNonzero(
        event.receivingFederateId,
        "A process federation attribute event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process federation attribute event requires an object instance.");
    if (event.attributeValues.empty() || event.transportationName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process federation attribute event requires values and transportation.");
    }
    result.event = std::move(event);
  }
  reader.finish();
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationReceiveObjectInstanceDiscoveryResult(
    ProcessFederationReceiveObjectInstanceDiscoveryResult const& result) {
  PayloadWriter writer;
  writer.unsigned8(result.event.has_value() ? 1U : 0U);
  if (result.event.has_value()) {
    auto const& event = *result.event;
    requireNonzero(
        event.receivingFederateId,
        "A process discovery event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process discovery event requires an object instance.");
    requireNonzero(
        event.objectClassHandle,
        "A process discovery event requires an object class.");
    requireNonzero(
        event.producingFederateId,
        "A process discovery event requires a producer identity.");
    if (event.objectInstanceName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process discovery event requires an object instance name.");
    }
    writer.unsigned64(event.receivingFederateId);
    writer.unsigned64(event.objectInstanceHandle);
    writer.unsigned64(event.objectClassHandle);
    writer.wideString(event.objectInstanceName);
    writer.unsigned64(event.producingFederateId);
  }
  return std::move(writer).finish();
}

ProcessFederationReceiveObjectInstanceDiscoveryResult
decodeProcessFederationReceiveObjectInstanceDiscoveryResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const hasEvent = reader.unsigned8();
  if (hasEvent > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process discovery result has an invalid event marker.");
  }
  ProcessFederationReceiveObjectInstanceDiscoveryResult result;
  if (hasEvent != 0U) {
    ProcessFederationObjectInstanceDiscoveryEvent event;
    event.receivingFederateId = reader.unsigned64();
    event.objectInstanceHandle = reader.unsigned64();
    event.objectClassHandle = reader.unsigned64();
    event.objectInstanceName = reader.wideString();
    event.producingFederateId = reader.unsigned64();
    requireNonzero(
        event.receivingFederateId,
        "A process discovery event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process discovery event requires an object instance.");
    requireNonzero(
        event.objectClassHandle,
        "A process discovery event requires an object class.");
    requireNonzero(
        event.producingFederateId,
        "A process discovery event requires a producer identity.");
    if (event.objectInstanceName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process discovery event requires an object instance name.");
    }
    result.event = std::move(event);
  }
  reader.finish();
  return result;
}

ProcessFederationService::ProcessFederationService(
    EmbeddedFederationRegistry& registry,
    FederationDefinition federationDefinition,
    ProcessFederationServiceOptions options)
    : registry_(registry),
      federationDefinition_(
          std::make_shared<FederationDefinition const>(std::move(federationDefinition))),
      options_(options) {}

ProcessTransportServiceDispatcher::Handler ProcessFederationService::handlerFor(
    ProcessTransportSession& session) {
  {
    std::scoped_lock lock(mutex_);
    sessions_.try_emplace(&session);
  }
  return [this, &session](TransportServiceMessage const& request) {
    return handle(session, request);
  };
}

void ProcessFederationService::detach(ProcessTransportSession& session) noexcept {
  std::scoped_lock lock(mutex_);
  for (auto position = pendingPushedRetractionRecipients_.begin();
       position != pendingPushedRetractionRecipients_.end();) {
    auto& recipients = position->second;
    recipients.erase(
        std::remove(recipients.begin(), recipients.end(), &session),
        recipients.end());
    if (recipients.empty()) {
      position = pendingPushedRetractionRecipients_.erase(position);
    } else {
      ++position;
    }
  }
  auto const found = sessions_.find(&session);
  if (found == sessions_.end()) {
    return;
  }
  if (found->second.federateId != 0U) {
    sessionsByFederateId_.erase(found->second.federateId);
  }
  sessions_.erase(found);
}

bool ProcessFederationService::dispatchConnectionLossResult(
    std::wstring const& federationName,
    FederationRegistryResult result,
    std::optional<std::uint64_t> departedFederateId) {
  if (departedFederateId) {
    std::scoped_lock lock(mutex_);
    for (auto& [session, state] : sessions_) {
      static_cast<void>(session);
      if (!state.federationName || *state.federationName != federationName) {
        continue;
      }
      state.attributeOwnershipAcquisitionEvents.erase(
          std::remove_if(
              state.attributeOwnershipAcquisitionEvents.begin(),
              state.attributeOwnershipAcquisitionEvents.end(),
              [departedFederateId](
                  ProcessFederationAttributeOwnershipAcquisitionEvent const& event) {
                return event.requestingFederateId == *departedFederateId ||
                    event.receivingFederateId == *departedFederateId;
              }),
          state.attributeOwnershipAcquisitionEvents.end());
    }
  }
  if (!result.resignOwnershipAcquisitionWorkItems.empty() &&
      !enqueueAttributeOwnershipAcquisitionWorkItems(
          federationName,
          std::move(result.resignOwnershipAcquisitionWorkItems))) {
    return false;
  }
  if (!result.resignOwnershipAssumptions.empty()) {
    std::vector<AttributeOwnershipAssumptionRecipient> assumptions;
    assumptions.reserve(result.resignOwnershipAssumptions.size());
    for (auto& assumption : result.resignOwnershipAssumptions) {
      assumptions.push_back({
          assumption.receivingFederateId,
          assumption.objectInstanceHandle,
          std::move(assumption.attributeHandles),
          std::move(assumption.callbackRoute)});
    }
    if (!enqueueAttributeOwnershipAssumptionRecipients(
            federationName,
            std::move(assumptions))) {
      return false;
    }
  }
  if (!result.resignObjectRemovals.empty()) {
    std::vector<ObjectInstanceRemovalRecipient> removals;
    removals.reserve(result.resignObjectRemovals.size());
    for (auto& removal : result.resignObjectRemovals) {
      removals.push_back({
          removal.receivingFederateId,
          removal.objectInstanceHandle,
          std::move(removal.callbackRoute),
          std::move(removal.serviceReportRoute),
          removal.rtiOwnedMomObject});
    }
    if (!enqueueObjectInstanceRemovals(
            federationName,
            std::move(removals),
            {})) {
      return false;
    }
  }
  if (!result.synchronizationNotifications.empty() &&
      !enqueueFederationSynchronizedNotifications(
          federationName,
          std::move(result.synchronizationNotifications))) {
    return false;
  }
  return true;
}

TransportServiceMessage ProcessFederationService::handle(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  try {
    switch (request.operation) {
      case TransportServiceOperation::create_federation_execution:
        return handleCreate(request);
      case TransportServiceOperation::join_federation_execution:
        return handleJoin(session, request);
      case TransportServiceOperation::resign_federation_execution:
        return handleResign(session, request);
      case TransportServiceOperation::register_federation_synchronization_point:
        return handleRegisterFederationSynchronizationPoint(session, request);
      case TransportServiceOperation::synchronization_point_achieved:
        return handleSynchronizationPointAchieved(session, request);
      case TransportServiceOperation::request_federation_save:
        return handleRequestFederationSave(session, request);
      case TransportServiceOperation::federate_save_begun:
      case TransportServiceOperation::federate_save_complete:
      case TransportServiceOperation::federate_save_not_complete:
        return handleFederateSaveControl(session, request, request.operation);
      case TransportServiceOperation::query_federation_save_status:
        return handleQueryFederationSaveStatus(session, request);
      case TransportServiceOperation::abort_federation_save:
        return handleAbortFederationSave(session, request);
      case TransportServiceOperation::request_federation_restore:
        return handleRequestFederationRestore(session, request);
      case TransportServiceOperation::federate_restore_complete:
        return handleFederateRestoreComplete(session, request);
      case TransportServiceOperation::federate_restore_not_complete:
        return handleFederateRestoreNotComplete(session, request);
      case TransportServiceOperation::abort_federation_restore:
        return handleAbortFederationRestore(session, request);
      case TransportServiceOperation::query_federation_restore_status:
        return handleQueryFederationRestoreStatus(session, request);
      case TransportServiceOperation::send_interaction:
        return handleSendInteraction(session, request);
      case TransportServiceOperation::send_interaction_with_regions:
        return handleSendInteractionWithRegions(session, request);
      case TransportServiceOperation::send_directed_interaction:
        return handleSendDirectedInteraction(session, request);
      case TransportServiceOperation::retract:
        return handleRetract(session, request);
      case TransportServiceOperation::update_attribute_values:
        return handleUpdateAttributeValues(session, request);
      case TransportServiceOperation::request_attribute_value_update:
        return handleRequestAttributeValueUpdate(session, request);
      case TransportServiceOperation::request_attribute_value_update_class:
        return handleRequestAttributeValueUpdateClass(session, request);
      case TransportServiceOperation::request_attribute_value_update_class_with_regions:
        return handleRequestAttributeValueUpdateClassWithRegions(session, request);
      case TransportServiceOperation::is_attribute_owned_by_federate:
        return handleAttributeOwnershipCheck(session, request);
      case TransportServiceOperation::query_attribute_ownership:
        return handleQueryAttributeOwnership(session, request);
      case TransportServiceOperation::attribute_ownership_acquisition_if_available:
        return handleAttributeOwnershipAcquisitionIfAvailable(session, request);
      case TransportServiceOperation::attribute_ownership_acquisition:
        return handleAttributeOwnershipAcquisition(session, request);
      case TransportServiceOperation::attribute_ownership_release_denied:
        return handleAttributeOwnershipReleaseDenied(session, request);
      case TransportServiceOperation::cancel_attribute_ownership_acquisition:
        return handleAttributeOwnershipAcquisitionCancellation(session, request);
      case TransportServiceOperation::
          cancel_negotiated_attribute_ownership_divestiture:
        return handleCancelNegotiatedAttributeOwnershipDivestiture(
            session, request);
      case TransportServiceOperation::negotiated_attribute_ownership_divestiture:
        return handleNegotiatedAttributeOwnershipDivestiture(session, request);
      case TransportServiceOperation::confirm_divestiture:
        return handleConfirmDivestiture(session, request);
      case TransportServiceOperation::unconditional_attribute_ownership_divestiture:
        return handleUnconditionalAttributeOwnershipDivestiture(session, request);
      case TransportServiceOperation::receive_interaction:
        return handleReceiveInteraction(session, request);
      case TransportServiceOperation::acknowledge_tso_delivery:
        return handleAcknowledgeTsoDelivery(session, request);
      case TransportServiceOperation::query_logical_time:
        return handleQueryLogicalTime(session, request);
      case TransportServiceOperation::query_lookahead:
        return handleQueryLookahead(session, request);
      case TransportServiceOperation::modify_lookahead:
        return handleModifyLookahead(session, request);
      case TransportServiceOperation::enable_time_regulation:
        return handleEnableTimeRegulation(session, request);
      case TransportServiceOperation::query_time_bounds:
        return handleQueryTimeBounds(session, request);
      case TransportServiceOperation::enable_time_constrained:
        return handleEnableTimeConstrained(session, request);
      case TransportServiceOperation::disable_time_regulation:
        return handleDisableTimeRegulation(session, request);
      case TransportServiceOperation::disable_time_constrained:
        return handleDisableTimeConstrained(session, request);
      case TransportServiceOperation::time_advance_request:
        return handleTimeAdvanceRequest(session, request);
      case TransportServiceOperation::time_advance_request_available:
        return handleTimeAdvanceRequestAvailable(session, request);
      case TransportServiceOperation::next_message_request:
        return handleNextMessageRequest(session, request);
      case TransportServiceOperation::next_message_request_available:
        return handleNextMessageRequestAvailable(session, request);
      case TransportServiceOperation::flush_queue_request:
        return handleFlushQueueRequest(session, request);
      case TransportServiceOperation::receive_attribute_update:
        return handleReceiveAttributeUpdate(session, request);
      case TransportServiceOperation::receive_object_instance_discovery:
        return handleReceiveObjectInstanceDiscovery(session, request);
      case TransportServiceOperation::get_federate_handle:
        return handleGetFederateHandle(session, request);
      case TransportServiceOperation::get_federate_name:
        return handleGetFederateName(session, request);
      case TransportServiceOperation::normalize_federate_handle:
      case TransportServiceOperation::normalize_object_class_handle:
      case TransportServiceOperation::normalize_interaction_class_handle:
      case TransportServiceOperation::normalize_object_instance_handle:
        return handleNormalizeHandle(session, request);
      case TransportServiceOperation::get_interaction_class_handle:
        return handleGetInteractionClassHandle(session, request);
      case TransportServiceOperation::get_object_class_handle:
        return handleGetObjectClassHandle(session, request);
      case TransportServiceOperation::get_parameter_handle:
        return handleGetParameterHandle(session, request);
      case TransportServiceOperation::publish_object_class_attributes:
        return handleObjectClassAttributeDeclaration(session, request);
      case TransportServiceOperation::subscribe_object_class_attributes:
      case TransportServiceOperation::unsubscribe_object_class_attributes:
        return handleObjectClassAttributeSubscription(session, request);
      case TransportServiceOperation::subscribe_object_class_attributes_with_regions:
      case TransportServiceOperation::unsubscribe_object_class_attributes_with_regions:
        return handleObjectClassAttributeRegionalSubscription(session, request);
      case TransportServiceOperation::register_object_instance:
        return handleRegisterObjectInstance(session, request);
      case TransportServiceOperation::local_delete_object_instance:
        return handleLocalDeleteObjectInstance(session, request);
      case TransportServiceOperation::delete_object_instance:
        return handleDeleteObjectInstance(session, request);
      case TransportServiceOperation::reserve_object_instance_name:
        return handleReserveObjectInstanceName(session, request);
      case TransportServiceOperation::release_object_instance_name:
        return handleReleaseObjectInstanceName(session, request);
      case TransportServiceOperation::reserve_multiple_object_instance_names:
        return handleReserveMultipleObjectInstanceNames(session, request);
      case TransportServiceOperation::release_multiple_object_instance_names:
        return handleReleaseMultipleObjectInstanceNames(session, request);
      case TransportServiceOperation::get_dimension_handle:
        return handleGetDimensionHandle(session, request);
      case TransportServiceOperation::get_dimension_name:
        return handleGetDimensionName(session, request);
      case TransportServiceOperation::get_transportation_type_handle:
        return handleGetTransportationTypeHandle(session, request);
      case TransportServiceOperation::get_transportation_type_name:
        return handleGetTransportationTypeName(session, request);
      case TransportServiceOperation::get_dimension_upper_bound:
        return handleGetDimensionUpperBound(session, request);
      case TransportServiceOperation::get_available_dimensions_for_object_class:
        return handleGetAvailableDimensionsForObjectClass(session, request);
      case TransportServiceOperation::get_available_dimensions_for_interaction_class:
        return handleGetAvailableDimensionsForInteractionClass(session, request);
      case TransportServiceOperation::create_region:
        return handleCreateRegion(session, request);
      case TransportServiceOperation::commit_region_modifications:
        return handleCommitRegionModifications(session, request);
      case TransportServiceOperation::delete_region:
        return handleDeleteRegion(session, request);
      case TransportServiceOperation::get_dimension_handle_set:
        return handleGetDimensionHandleSet(session, request);
      case TransportServiceOperation::get_range_bounds:
        return handleGetRangeBounds(session, request);
      case TransportServiceOperation::set_range_bounds:
        return handleSetRangeBounds(session, request);
      case TransportServiceOperation::get_attribute_scope_advisory_switch:
        return handleGetAttributeScopeAdvisorySwitch(session, request);
      case TransportServiceOperation::set_attribute_scope_advisory_switch:
        return handleSetAttributeScopeAdvisorySwitch(session, request);
      case TransportServiceOperation::get_attribute_relevance_advisory_switch:
        return handleGetAttributeRelevanceAdvisorySwitch(session, request);
      case TransportServiceOperation::set_attribute_relevance_advisory_switch:
        return handleSetAttributeRelevanceAdvisorySwitch(session, request);
      case TransportServiceOperation::get_convey_region_designator_sets_switch:
        return handleGetConveyRegionDesignatorSetsSwitch(session, request);
      case TransportServiceOperation::set_convey_region_designator_sets_switch:
        return handleSetConveyRegionDesignatorSetsSwitch(session, request);
      case TransportServiceOperation::get_automatic_resign_directive:
        return handleGetAutomaticResignDirective(session, request);
      case TransportServiceOperation::set_automatic_resign_directive:
        return handleSetAutomaticResignDirective(session, request);
      case TransportServiceOperation::publish_object_class_directed_interactions:
      case TransportServiceOperation::unpublish_object_class_directed_interactions:
      case TransportServiceOperation::subscribe_object_class_directed_interactions:
      case TransportServiceOperation::unsubscribe_object_class_directed_interactions:
        return handleObjectClassDirectedInteractionDeclaration(session, request);
      case TransportServiceOperation::register_object_instance_with_regions:
        return handleRegisterObjectInstanceWithRegions(session, request);
      case TransportServiceOperation::associate_regions_for_updates:
      case TransportServiceOperation::unassociate_regions_for_updates:
        return handleObjectInstanceRegionAssociation(session, request);
      case TransportServiceOperation::get_attribute_handle:
        return handleGetAttributeHandle(session, request);
      case TransportServiceOperation::get_object_instance_handle:
        return handleGetObjectInstanceHandle(session, request);
      case TransportServiceOperation::get_object_instance_name:
        return handleGetObjectInstanceName(session, request);
      case TransportServiceOperation::get_known_object_class_handle:
        return handleGetKnownObjectClassHandle(session, request);
      case TransportServiceOperation::get_object_class_name:
        return handleGetObjectClassName(session, request);
      case TransportServiceOperation::get_interaction_class_name:
        return handleGetInteractionClassName(session, request);
      case TransportServiceOperation::get_attribute_name:
        return handleGetAttributeName(session, request);
      case TransportServiceOperation::get_parameter_name:
        return handleGetParameterName(session, request);
      case TransportServiceOperation::publish_interaction_class:
      case TransportServiceOperation::unpublish_interaction_class:
      case TransportServiceOperation::subscribe_interaction_class:
      case TransportServiceOperation::unsubscribe_interaction_class:
        return handleInteractionClassDeclaration(session, request);
      case TransportServiceOperation::change_interaction_order_type:
        return handleChangeInteractionOrderType(session, request);
      case TransportServiceOperation::change_attribute_order_type:
        return handleChangeAttributeOrderType(session, request);
      case TransportServiceOperation::change_default_attribute_order_type:
        return handleChangeDefaultAttributeOrderType(session, request);
      case TransportServiceOperation::change_default_attribute_transportation_type:
        return handleChangeDefaultAttributeTransportationType(session, request);
      case TransportServiceOperation::request_attribute_transportation_type_change:
        return handleRequestAttributeTransportationTypeChange(session, request);
      case TransportServiceOperation::query_attribute_transportation_type:
        return handleQueryAttributeTransportationType(session, request);
      case TransportServiceOperation::request_interaction_transportation_type_change:
        return handleRequestInteractionTransportationTypeChange(session, request);
      case TransportServiceOperation::query_interaction_transportation_type:
        return handleQueryInteractionTransportationType(session, request);
      case TransportServiceOperation::subscribe_interaction_class_with_regions:
      case TransportServiceOperation::unsubscribe_interaction_class_with_regions:
        return handleInteractionClassRegionalSubscription(session, request);
    }
  } catch (ProcessFederationServiceProtocolError const&) {
    return invalid(request);
  } catch (std::exception const&) {
    return internalError(request);
  }
  return invalid(request);
}

TransportServiceMessage ProcessFederationService::handleGetFederateHandle(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const lookupRequest =
      decodeProcessFederationGetFederateHandleRequest(request.payload);
  std::optional<FederateMembership> membership;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != lookupRequest.federationName ||
        state->second.federateId != lookupRequest.federateId ||
        !registry_.memberById(
            lookupRequest.federationName, lookupRequest.federateId)) {
      return rejected(request);
    }
    membership = registry_.memberByName(
        lookupRequest.federationName, lookupRequest.federateName);
  }
  if (!membership || membership->id == 0U) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationHandleResult(
          ProcessFederationHandleResult{membership->id}));
}

TransportServiceMessage ProcessFederationService::handleGetFederateName(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const lookupRequest =
      decodeProcessFederationGetFederateNameRequest(request.payload);
  std::optional<std::wstring> federateName;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != lookupRequest.federationName ||
        state->second.federateId != lookupRequest.federateId ||
        !registry_.memberById(
            lookupRequest.federationName, lookupRequest.federateId)) {
      return rejected(request);
    }
    federateName = registry_.federateNameFor(
        lookupRequest.federationName, lookupRequest.targetFederateId);
  }
  if (!federateName || federateName->empty()) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationStringResult(
          ProcessFederationStringResult{std::move(*federateName)}));
}

TransportServiceMessage ProcessFederationService::handleNormalizeHandle(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const normalizeRequest =
      decodeProcessFederationNormalizeHandleRequest(request.payload);
  std::optional<unsigned long> normalized;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != normalizeRequest.federationName ||
        state->second.federateId != normalizeRequest.federateId ||
        !registry_.memberById(
            normalizeRequest.federationName, normalizeRequest.federateId)) {
      return rejected(request);
    }

    switch (request.operation) {
      case TransportServiceOperation::normalize_federate_handle:
        normalized = registry_.normalizedFederateHandleValueFor(
            normalizeRequest.federationName, normalizeRequest.handle);
        break;
      case TransportServiceOperation::normalize_object_class_handle:
        normalized = registry_.normalizedObjectClassHandleValueFor(
            normalizeRequest.federationName, normalizeRequest.handle);
        break;
      case TransportServiceOperation::normalize_interaction_class_handle:
        normalized = registry_.normalizedInteractionClassHandleValueFor(
            normalizeRequest.federationName, normalizeRequest.handle);
        break;
      case TransportServiceOperation::normalize_object_instance_handle:
        normalized = registry_.normalizedObjectInstanceHandleValueFor(
            normalizeRequest.federationName, normalizeRequest.handle);
        break;
      default:
        return invalid(request);
    }
  }
  if (!normalized || *normalized == 0UL) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationHandleResult(
          ProcessFederationHandleResult{
              static_cast<std::uint64_t>(*normalized)}));
}

TransportServiceMessage ProcessFederationService::handleResign(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const resignRequest = decodeProcessFederationResignRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != resignRequest.federationName ||
        state->second.federateId != resignRequest.federateId ||
        !registry_.memberById(
            resignRequest.federationName, resignRequest.federateId)) {
      return rejected(request);
    }
  }

  auto const result = registry_.resign(
      resignRequest.federationName,
      resignRequest.federateId,
      resignRequest.resignAction);
  if (result.status != FederationRegistryStatus::applied) {
    return rejected(request);
  }
  if (!dispatchConnectionLossResult(
          resignRequest.federationName,
          result,
          resignRequest.federateId)) {
    return internalError(request);
  }
  auto scheduled = registry_.reevaluateTimeAdvanceGrants(
      resignRequest.federationName);
  if (scheduled.status != FederationTimeGrantStatus::applied) {
    return internalError(request);
  }
  dispatchTimeAdvanceGrants(
      resignRequest.federationName,
      std::move(scheduled.dispatches));
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state != sessions_.end()) {
      sessionsByFederateId_.erase(state->second.federateId);
      for (auto position = processTsoMessageProducers_.begin();
           position != processTsoMessageProducers_.end();) {
        if (position->second == state->second.federateId) {
          pendingPushedRetractionRecipients_.erase(position->first);
          position = processTsoMessageProducers_.erase(position);
        } else {
          ++position;
        }
      }
      state->second.federationName.reset();
      state->second.federateId = 0U;
      state->second.timeState.reset();
      state->second.interactionEvents.clear();
      state->second.attributeUpdateEvents.clear();
      state->second.objectInstanceDiscoveryEvents.clear();
      state->second.objectInstanceRemovalEvents.clear();
      state->second.objectInstanceScopeChangeEvents.clear();
      state->second.attributeRelevanceAdvisoryEvents.clear();
      state->second.attributeTransportationTypeChangeEvents.clear();
      state->second.attributeTransportationTypeQueryEvents.clear();
      state->second.interactionTransportationTypeChangeEvents.clear();
      state->second.interactionTransportationTypeQueryEvents.clear();
      state->second.attributeOwnershipQueryEvents.clear();
      state->second.attributeOwnershipAcquisitionIfAvailableEvents.clear();
      state->second.attributeOwnershipAcquisitionEvents.clear();
      state->second.attributeOwnershipUnavailableEvents.clear();
      state->second.synchronizationPointAnnouncementEvents.clear();
      state->second.federationSynchronizedEvents.clear();
      state->second.saveEvents.clear();
      state->second.restoreEvents.clear();
    }
  }
  return responseFor(request, TransportServiceStatus::ok);
}

TransportServiceMessage
ProcessFederationService::handleRegisterFederationSynchronizationPoint(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const registration =
      decodeProcessFederationRegisterSynchronizationPointRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != registration.federationName ||
        state->second.federateId != registration.federateId ||
        !registry_.memberById(
            registration.federationName, registration.federateId)) {
      return rejected(request);
    }
  }

  std::set<std::uint64_t> synchronizationSet(
      registration.synchronizationSet.begin(),
      registration.synchronizationSet.end());
  auto plan = registry_.registerSynchronizationPoint(
      registration.federationName,
      registration.federateId,
      registration.label,
      registration.userSuppliedTag,
      synchronizationSet,
      registration.synchronizationSetWasSupplied);
  ProcessFederationRegisterSynchronizationPointResult result;
  switch (plan.status) {
    case SynchronizationPointRegistrationStatus::applied:
      result.status =
          ProcessFederationSynchronizationPointRegistrationStatus::applied;
      break;
    case SynchronizationPointRegistrationStatus::federation_does_not_exist:
      result.status = ProcessFederationSynchronizationPointRegistrationStatus::
          federation_does_not_exist;
      break;
    case SynchronizationPointRegistrationStatus::federate_not_member:
      result.status =
          ProcessFederationSynchronizationPointRegistrationStatus::federate_not_member;
      break;
    case SynchronizationPointRegistrationStatus::callback_route_missing:
      result.status = ProcessFederationSynchronizationPointRegistrationStatus::
          callback_route_missing;
      break;
  }
  result.succeeded = plan.succeeded;
  result.failureReason = plan.failureReason;
  if (plan.status == SynchronizationPointRegistrationStatus::applied &&
      !plan.announcements.empty() &&
      !enqueueSynchronizationPointAnnouncements(
          registration.federationName,
          std::move(plan.announcements))) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationRegisterSynchronizationPointResult(result));
}

TransportServiceMessage ProcessFederationService::handleSynchronizationPointAchieved(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const achievement =
      decodeProcessFederationSynchronizationPointAchievedRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != achievement.federationName ||
        state->second.federateId != achievement.federateId ||
        !registry_.memberById(
            achievement.federationName, achievement.federateId)) {
      return rejected(request);
    }
  }

  auto plan = registry_.achieveSynchronizationPoint(
      achievement.federationName,
      achievement.federateId,
      achievement.label,
      achievement.successfully);
  ProcessFederationSynchronizationPointAchievedResult result;
  switch (plan.status) {
    case SynchronizationPointAchievedStatus::applied:
      result.status =
          ProcessFederationSynchronizationPointAchievedStatus::applied;
      break;
    case SynchronizationPointAchievedStatus::federation_does_not_exist:
      result.status = ProcessFederationSynchronizationPointAchievedStatus::
          federation_does_not_exist;
      break;
    case SynchronizationPointAchievedStatus::federate_not_member:
      result.status =
          ProcessFederationSynchronizationPointAchievedStatus::federate_not_member;
      break;
    case SynchronizationPointAchievedStatus::
        synchronization_point_label_not_announced:
      result.status = ProcessFederationSynchronizationPointAchievedStatus::
          synchronization_point_label_not_announced;
      break;
  }
  if (plan.status == SynchronizationPointAchievedStatus::applied &&
      !plan.synchronizationNotifications.empty() &&
      !enqueueFederationSynchronizedNotifications(
          achievement.federationName,
          std::move(plan.synchronizationNotifications))) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationSynchronizationPointAchievedResult(result));
}

TransportServiceMessage ProcessFederationService::handleRequestFederationSave(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const saveRequest = decodeProcessFederationSaveRequest(request.payload);
  if (saveRequest.label.empty()) {
    return rejected(request);
  }
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != saveRequest.federationName ||
        state->second.federateId != saveRequest.federateId ||
        !registry_.memberById(
            saveRequest.federationName, saveRequest.federateId)) {
      return rejected(request);
    }
  }
  std::shared_ptr<rti1516_2025::LogicalTime const> timestamp;
  if (saveRequest.timestamp) {
    auto const definition = registry_.definitionFor(saveRequest.federationName);
    if (!definition) {
      return responseFor(
          request,
          TransportServiceStatus::ok,
          encodeProcessFederationSaveControlResult(
              ProcessFederationSaveControlResult{
                  FederationSaveControlStatus::invalid_timed_save}));
    }
    timestamp = decodeProcessLogicalTime(
        *saveRequest.timestamp,
        definition->logicalTimeImplementationName);
  }
  auto result = timestamp
      ? registry_.requestFederationSave(
            saveRequest.federationName,
            saveRequest.federateId,
            saveRequest.label,
            std::move(timestamp))
      : registry_.requestFederationSave(
            saveRequest.federationName,
            saveRequest.federateId,
            saveRequest.label);
  if (result.status == FederationSaveControlStatus::applied &&
      !result.notifications.empty() &&
      !enqueueFederationSaveNotifications(
          saveRequest.federationName,
          std::move(result.notifications))) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationSaveControlResult(
          ProcessFederationSaveControlResult{result.status}));
}

TransportServiceMessage ProcessFederationService::handleFederateSaveControl(
    ProcessTransportSession& session,
    TransportServiceMessage const& request,
    TransportServiceOperation operation) {
  auto const saveRequest = decodeProcessFederationSaveRequest(request.payload);
  if (!saveRequest.label.empty()) {
    return rejected(request);
  }
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != saveRequest.federationName ||
        state->second.federateId != saveRequest.federateId ||
        !registry_.memberById(
            saveRequest.federationName, saveRequest.federateId)) {
      return rejected(request);
    }
  }
  FederationSaveControlResult result;
  if (operation == TransportServiceOperation::federate_save_begun) {
    result = registry_.federateSaveBegun(
        saveRequest.federationName, saveRequest.federateId);
  } else if (operation == TransportServiceOperation::federate_save_complete) {
    result = registry_.federateSaveComplete(
        saveRequest.federationName, saveRequest.federateId);
  } else {
    result = registry_.federateSaveNotComplete(
        saveRequest.federationName, saveRequest.federateId);
  }
  if (result.status == FederationSaveControlStatus::applied &&
      !result.notifications.empty() &&
      !enqueueFederationSaveNotifications(
          saveRequest.federationName,
          std::move(result.notifications))) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationSaveControlResult(
          ProcessFederationSaveControlResult{result.status}));
}

TransportServiceMessage ProcessFederationService::handleQueryFederationSaveStatus(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const saveRequest = decodeProcessFederationSaveRequest(request.payload);
  if (!saveRequest.label.empty()) {
    return rejected(request);
  }
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != saveRequest.federationName ||
        state->second.federateId != saveRequest.federateId ||
        !registry_.memberById(
            saveRequest.federationName, saveRequest.federateId)) {
      return rejected(request);
    }
  }
  auto result = registry_.queryFederationSaveStatus(
      saveRequest.federationName, saveRequest.federateId);
  if (result.status == FederationSaveControlStatus::applied &&
      !result.notifications.empty() &&
      !enqueueFederationSaveNotifications(
          saveRequest.federationName,
          std::move(result.notifications))) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationSaveControlResult(
          ProcessFederationSaveControlResult{result.status}));
}

TransportServiceMessage ProcessFederationService::handleAbortFederationSave(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const saveRequest = decodeProcessFederationSaveRequest(request.payload);
  if (!saveRequest.label.empty()) {
    return rejected(request);
  }
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != saveRequest.federationName ||
        state->second.federateId != saveRequest.federateId ||
        !registry_.memberById(
            saveRequest.federationName, saveRequest.federateId)) {
      return rejected(request);
    }
  }
  auto result = registry_.abortFederationSave(
      saveRequest.federationName, saveRequest.federateId);
  if (result.status == FederationSaveControlStatus::applied &&
      !result.notifications.empty() &&
      !enqueueFederationSaveNotifications(
          saveRequest.federationName,
          std::move(result.notifications))) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationSaveControlResult(
          ProcessFederationSaveControlResult{result.status}));
}

TransportServiceMessage ProcessFederationService::handleRequestFederationRestore(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const restoreRequest =
      decodeProcessFederationRestoreRequest(request.payload);
  if (restoreRequest.label.empty()) {
    return rejected(request);
  }
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != restoreRequest.federationName ||
        state->second.federateId != restoreRequest.federateId ||
        !registry_.memberById(
            restoreRequest.federationName, restoreRequest.federateId)) {
      return rejected(request);
    }
  }

  auto result = registry_.requestFederationRestore(
      restoreRequest.federationName,
      restoreRequest.federateId,
      restoreRequest.label);
  if (!result.notifications.empty() &&
      !enqueueFederationRestoreNotifications(
          restoreRequest.federationName,
          std::move(result.notifications))) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationRestoreControlResult(
          ProcessFederationRestoreControlResult{result.status}));
}

TransportServiceMessage ProcessFederationService::handleFederateRestoreComplete(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const restoreRequest =
      decodeProcessFederationRestoreRequest(request.payload);
  if (!restoreRequest.label.empty()) {
    return rejected(request);
  }
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != restoreRequest.federationName ||
        state->second.federateId != restoreRequest.federateId ||
        !registry_.memberById(
            restoreRequest.federationName, restoreRequest.federateId)) {
      return rejected(request);
    }
  }

  auto result = registry_.federateRestoreComplete(
      restoreRequest.federationName,
      restoreRequest.federateId);
  if (!result.notifications.empty() &&
      !enqueueFederationRestoreNotifications(
          restoreRequest.federationName,
          std::move(result.notifications))) {
    return internalError(request);
  }
  // Restore completion is the callback-order boundary.  The registry returns
  // value-only ownership-assumption work for the current process sessions;
  // enqueue it only after the lifecycle notifications so the existing
  // receive fence delivers Federation Restored before the assumption offer.
  // Other restore work-item families remain separate slices and are not
  // silently projected through this path.
  if (!result.attributeOwnershipAssumptionWorkItems.empty() &&
      !enqueueAttributeOwnershipAssumptionRecipients(
          restoreRequest.federationName,
          std::move(result.attributeOwnershipAssumptionWorkItems),
          !restoreRequest.callbacksEnabled)) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationRestoreControlResult(
          ProcessFederationRestoreControlResult{result.status}));
}

TransportServiceMessage
ProcessFederationService::handleFederateRestoreNotComplete(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const restoreRequest =
      decodeProcessFederationRestoreRequest(request.payload);
  if (!restoreRequest.label.empty()) {
    return rejected(request);
  }
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != restoreRequest.federationName ||
        state->second.federateId != restoreRequest.federateId ||
        !registry_.memberById(
            restoreRequest.federationName, restoreRequest.federateId)) {
      return rejected(request);
    }
  }

  auto result = registry_.federateRestoreNotComplete(
      restoreRequest.federationName,
      restoreRequest.federateId);
  if (!result.notifications.empty() &&
      !enqueueFederationRestoreNotifications(
          restoreRequest.federationName,
          std::move(result.notifications))) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationRestoreControlResult(
          ProcessFederationRestoreControlResult{result.status}));
}

TransportServiceMessage
ProcessFederationService::handleAbortFederationRestore(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const restoreRequest =
      decodeProcessFederationRestoreRequest(request.payload);
  if (!restoreRequest.label.empty()) {
    return rejected(request);
  }
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != restoreRequest.federationName ||
        state->second.federateId != restoreRequest.federateId ||
        !registry_.memberById(
            restoreRequest.federationName, restoreRequest.federateId)) {
      return rejected(request);
    }
  }

  auto result = registry_.abortFederationRestore(
      restoreRequest.federationName,
      restoreRequest.federateId);
  if (!result.notifications.empty() &&
      !enqueueFederationRestoreNotifications(
          restoreRequest.federationName,
          std::move(result.notifications))) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationRestoreControlResult(
          ProcessFederationRestoreControlResult{result.status}));
}

TransportServiceMessage
ProcessFederationService::handleQueryFederationRestoreStatus(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const restoreRequest =
      decodeProcessFederationRestoreRequest(request.payload);
  if (!restoreRequest.label.empty()) {
    return rejected(request);
  }
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != restoreRequest.federationName ||
        state->second.federateId != restoreRequest.federateId ||
        !registry_.memberById(
            restoreRequest.federationName, restoreRequest.federateId)) {
      return rejected(request);
    }
  }

  auto result = registry_.queryFederationRestoreStatus(
      restoreRequest.federationName,
      restoreRequest.federateId);
  if (!result.notifications.empty() &&
      !enqueueFederationRestoreNotifications(
          restoreRequest.federationName,
          std::move(result.notifications))) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationRestoreControlResult(
          ProcessFederationRestoreControlResult{result.status}));
}

TransportServiceMessage ProcessFederationService::handleCreate(
    TransportServiceMessage const& request) {
  auto const createRequest = decodeProcessFederationCreateRequest(request.payload);
  std::optional<FederationDefinition> preparedDefinition;
  if (createRequest.hasFomInputs) {
    if (!options_.createFomPreparation) {
      // The process transport must not silently discard the official FOM/MIM
      // inputs. A configured endpoint without the standards-derived
      // coordinator rejects the explicit form deterministically.
      return rejected(request);
    }
    preparedDefinition = options_.createFomPreparation(
        createRequest.fomModules,
        createRequest.mimModule,
        createRequest.logicalTimeImplementationName);
    if (!preparedDefinition || preparedDefinition->fomModules.empty() ||
        preparedDefinition->logicalTimeImplementationName.empty()) {
      return rejected(request);
    }
  }
  auto result = registry_.create(
      createRequest.federationName,
      preparedDefinition ? std::move(*preparedDefinition)
                         : *federationDefinition_);
  if (result.status != FederationRegistryStatus::applied) {
    return rejected(request);
  }
  return responseFor(request, TransportServiceStatus::ok);
}

TransportServiceMessage ProcessFederationService::handleJoin(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const joinRequest = decodeProcessFederationJoinRequest(request.payload);
  // Additional FOM preparation and the registry commit must observe one
  // definition.  Serialize this pair across process sessions so concurrent
  // joins cannot both compose from the same stale catalog and race a
  // replacement definition into the registry.
  std::scoped_lock joinLock(joinMutex_);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() || state->second.federateId != 0U) {
      return rejected(request);
    }
  }

  auto currentDefinition = registry_.definitionFor(joinRequest.federationName);
  if (!currentDefinition) {
    return rejected(request);
  }

  std::optional<FederationDefinition> replacementDefinition;
  if (!joinRequest.additionalFomModules.empty()) {
    if (!options_.additionalFomPreparation) {
      // The process transport does not invent FOM composition semantics. A
      // configured endpoint must supply the same standards-derived
      // coordinator used by the embedded profile; otherwise this request is
      // rejected deterministically instead of silently dropping modules.
      return rejected(request);
    }
    replacementDefinition = options_.additionalFomPreparation(
        *currentDefinition, joinRequest.additionalFomModules);
    if (!replacementDefinition || replacementDefinition->fomModules.empty()) {
      return rejected(request);
    }
  }

  auto const& effectiveDefinition = replacementDefinition
      ? *replacementDefinition
      : *currentDefinition;
  // Capture the selected implementation before an additional-FOM definition
  // is moved into the registry below. The Join response must describe the
  // committed definition, not the moved-from preparation object.
  auto logicalTimeImplementationName =
      effectiveDefinition.logicalTimeImplementationName;

  InteractionCallbackRoute callbackRoute;
  callbackRoute.submit = [](FederateCallbackInvocation) {};
  // The registry retains this route as the callback ownership boundary.  The
  // first process slice projects the callback payload into the receiver's
  // event queue below; a later public adapter will replace this bridge with
  // the receiver process's official FederateAmbassador dispatch.
  callbackRoute.receiveOrderSubmit = [](FederateCallbackInvocation) {};
  auto timeState = makeProcessFederateTimeState(effectiveDefinition);
  auto const processConnection = session.connection();
  auto timeAdvanceGrantDispatchFactory =
      [this,
       processConnection,
       federationName = joinRequest.federationName,
       timeState](std::uint64_t federateId,
                   std::uint64_t generation,
                   std::uint64_t dispatchIdentity)
      -> FederationTimeGrantDispatch {
    if (!processConnection || !timeState || federateId == 0U ||
        generation == 0U || dispatchIdentity == 0U) {
      return {};
    }
    return [this,
            processConnection,
            federationName,
            timeState,
            federateId,
            generation,
            dispatchIdentity] {
      auto const beginStatus = registry_.beginTimeAdvanceGrant(
          federationName,
          federateId,
          generation,
          dispatchIdentity);
      switch (beginStatus) {
        case FederationTimeGrantStatus::time_advance_not_pending:
        case FederationTimeGrantStatus::stale_generation:
        case FederationTimeGrantStatus::grant_not_ready:
        case FederationTimeGrantStatus::federate_not_member:
        case FederationTimeGrantStatus::federation_does_not_exist:
          // A resignation, restore, or competing re-evaluation may have
          // invalidated a queued dispatch. The registry deliberately fences
          // that work; do not manufacture a callback for stale state.
          return;
        case FederationTimeGrantStatus::inconsistent_temporal_state:
          throw std::runtime_error(
              "The process federation time-grant scheduler lost temporal state.");
        case FederationTimeGrantStatus::applied:
          break;
      }

      auto const pendingSnapshot = timeState->snapshot();
      if (!pendingSnapshot.requestedTime ||
          pendingSnapshot.requestedTime->implementationName() !=
              timeState->implementationName()) {
        throw std::runtime_error(
            "The process federation time-grant state has no usable delivery boundary.");
      }
      if (pendingSnapshot.advanceMode == FederateTimeAdvanceMode::flush_queue_request) {
        if (!pendingSnapshot.currentTime ||
            pendingSnapshot.currentTime->implementationName() !=
                timeState->implementationName()) {
          throw std::runtime_error(
              "The process federation Flush Queue Grant has no usable current-time boundary.");
        }

        auto execution = registry_.timeSnapshotFor(federationName);
        if (!execution) {
          throw std::runtime_error(
              "The process federation no longer records the Flush Queue requester.");
        }
        FederationFlushQueueGrantCalculator flushQueueGrantCalculator;
        auto flushQueueCalculation = flushQueueGrantCalculator.calculate(
            *execution, federateId);
        if (!flushQueueCalculation.calculated()) {
          throw std::runtime_error(
              "The process federation could not calculate the Flush Queue Grant times.");
        }

        auto factory = rti1516_2025::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
            pendingSnapshot.implementationName);
        if (!factory || factory->getName() != pendingSnapshot.implementationName) {
          throw std::runtime_error(
              "The process federation could not create the Flush Queue time factory.");
        }
        auto finalTime = factory->makeFinal();
        if (!finalTime || finalTime->implementationName() !=
                              pendingSnapshot.implementationName) {
          throw std::runtime_error(
              "The process federation could not create the Flush Queue boundary.");
        }

        auto encodedOptimistic = encodeProcessLogicalTime(
            flushQueueCalculation.optimisticTime);
        if (!encodedOptimistic) {
          throw std::runtime_error(
              "The process federation could not encode the Flush Queue optimistic time.");
        }

        // Keep the state in Time Advancing while all queued TSO payloads cross
        // the process callback boundary, then commit the two-value FQR state
        // immediately before publishing the matching event. Flush Queue
        // Request uses the caller-supplied frontier for delivery admission;
        // the calculated actual and optimistic values describe the resulting
        // grant and must not be used to suppress payloads admitted by FQR.
        dispatchTsoInteractionPayloads(
            federationName, federateId, *pendingSnapshot.requestedTime);
        auto flushResult = timeState->grantFlushQueue(
            generation,
            std::move(flushQueueCalculation.grantedTime),
            std::move(flushQueueCalculation.optimisticTime));
        if (flushResult.status != FederateTimeAdvanceStatus::applied) {
          throw std::runtime_error(
              "The process federation time state rejected the Flush Queue Grant.");
        }
        auto encodedGrant = encodeProcessLogicalTime(timeState->currentTime());
        if (!encodedGrant) {
          throw std::runtime_error(
              "The process federation could not encode the Flush Queue Grant time.");
        }

        ProcessFederationTimeAdvanceResult eventResult;
        eventResult.status = ProcessFederationTimeAdvanceStatus::applied;
        eventResult.grantedTime = std::move(encodedGrant);
        eventResult.optimisticTime = std::move(encodedOptimistic);
        ProcessTransportSession eventSession(processConnection);
        if (!eventSession.send(
                TransportServiceMessage{
                    TransportServiceMessageKind::event,
                    TransportServiceOperation::time_advance_grant,
                    TransportServiceStatus::ok,
                    0U,
                    encodeProcessFederationTimeAdvanceResult(eventResult)})) {
          throw std::runtime_error(
              "The process federation could not deliver the Flush Queue Grant event.");
        }
        return;
      }
      // Timestamped interaction callbacks precede the matching grant.  Keep
      // the FederateTimeState in Time Advancing until the registry has moved
      // every eligible process payload across its callback boundary.
      dispatchTsoInteractionPayloads(
          federationName,
          federateId,
          *pendingSnapshot.requestedTime);

      // A timestamped federation save has the same pre-grant boundary as the
      // embedded adapter: the constrained recipient must observe Initiate
      // Federate Save while it is still Time Advancing, before this grant is
      // published.  The registry owns eligibility and cross-federate ordering;
      // this process seam only projects the resulting callback event.
      auto const saveAdmission =
          registry_.admitTimedFederationSaveAtTimeAdvanceBoundary(
              federationName,
              federateId);
      if (saveAdmission.status != FederationSaveControlStatus::applied) {
        throw std::runtime_error(
            "The process federation could not admit a timestamped save at the time-advance boundary.");
      }
      if (saveAdmission.currentFederateLabel) {
        if (!saveAdmission.currentFederateTimestamp) {
          throw std::runtime_error(
              "The process federation admitted a timestamped save without its requested time.");
        }
        ProcessFederationSaveEvent saveEvent;
        saveEvent.kind = FederationSaveNotificationKind::initiate;
        saveEvent.receivingFederateId = federateId;
        saveEvent.label = *saveAdmission.currentFederateLabel;
        saveEvent.successful = true;
        saveEvent.timestamp = encodeProcessLogicalTime(
            saveAdmission.currentFederateTimestamp);
        if (!saveEvent.timestamp) {
          throw std::runtime_error(
              "The process federation could not encode the timestamped save boundary.");
        }
        ProcessFederationReceiveInteractionResult saveResult;
        saveResult.saveEvent = std::move(saveEvent);
        ProcessTransportSession eventSession(processConnection);
        if (!eventSession.send(TransportServiceMessage{
                TransportServiceMessageKind::event,
                TransportServiceOperation::receive_interaction,
                TransportServiceStatus::ok,
                0U,
                encodeProcessFederationReceiveInteractionResult(saveResult)})) {
          throw std::runtime_error(
              "The process federation could not deliver the timestamped save initiation event.");
        }
      }
      if (!saveAdmission.notifications.empty() &&
          !enqueueFederationSaveNotifications(
              federationName,
              std::move(saveAdmission.notifications))) {
        throw std::runtime_error(
            "The process federation could not enqueue the remaining timestamped save notifications.");
      }

      auto const grantedTime = timeState->grant(generation);
      auto encodedGrant = encodeProcessLogicalTime(grantedTime);
      if (!encodedGrant) {
        throw std::runtime_error(
            "The process federation time-grant state rejected its scheduled generation.");
      }
      ProcessFederationTimeAdvanceResult eventResult;
      eventResult.status = ProcessFederationTimeAdvanceStatus::applied;
      eventResult.grantedTime = std::move(encodedGrant);
      ProcessTransportSession eventSession(processConnection);
      if (!eventSession.send(
              TransportServiceMessage{
                  TransportServiceMessageKind::event,
                  TransportServiceOperation::time_advance_grant,
                  TransportServiceStatus::ok,
                  0U,
                  encodeProcessFederationTimeAdvanceResult(eventResult)})) {
        throw std::runtime_error(
            "The process federation could not deliver a time-advance grant event.");
      }
    };
  };
  FederationJoinResult result;
  if (replacementDefinition) {
    result = registry_.joinWithDefinitionAndTimeState(
        joinRequest.federationName,
        std::move(*replacementDefinition),
        timeState,
        joinRequest.federateType,
        joinRequest.requestedFederateName,
        std::move(callbackRoute),
        std::move(timeAdvanceGrantDispatchFactory));
  } else {
    result = registry_.joinWithTimeState(
        joinRequest.federationName,
        timeState,
        joinRequest.federateType,
        joinRequest.requestedFederateName,
        std::move(callbackRoute),
        std::move(timeAdvanceGrantDispatchFactory));
  }
  if (result.status != FederationRegistryStatus::applied || !result.membership) {
    return rejected(request);
  }

  {
    std::scoped_lock lock(mutex_);
    auto& state = sessions_.at(&session);
    state.federationName = joinRequest.federationName;
    state.federateId = result.membership->id;
    state.timeState = std::move(timeState);
    sessionsByFederateId_[state.federateId] = &session;
  }
  auto pendingAnnouncements = registry_.announcePendingSynchronizationPoints(
      joinRequest.federationName,
      result.membership->id);
  if (pendingAnnouncements.status !=
          SynchronizationPointAnnouncementStatus::applied ||
      (!pendingAnnouncements.announcements.empty() &&
       !enqueueSynchronizationPointAnnouncements(
           joinRequest.federationName,
           std::move(pendingAnnouncements.announcements)))) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationJoinResult(
          ProcessFederationJoinResult{
              result.membership->id,
              result.membership->name,
              std::move(logicalTimeImplementationName)}));
}

TransportServiceMessage ProcessFederationService::handleQueryLogicalTime(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const query = decodeProcessFederationQueryLogicalTimeRequest(request.payload);
  std::shared_ptr<FederateTimeState> timeState;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != query.federationName ||
        state->second.federateId != query.federateId ||
        !registry_.memberById(query.federationName, query.federateId)) {
      return rejected(request);
    }
    timeState = state->second.timeState;
  }

  // Query the state object established by Join. Process time-regulation and
  // grant operations mutate this retained object, so the read-only operation
  // must not reconstruct a new initial value for each query.
  if (!timeState) {
    return internalError(request);
  }
  auto const current = timeState->currentTime();
  if (!current || current->implementationName() != timeState->implementationName()) {
    return internalError(request);
  }
  auto const encoded = current->encode();
  std::vector<std::uint8_t> bytes;
  if (encoded.size() != 0U) {
    auto const* data = static_cast<unsigned char const*>(encoded.data());
    if (data == nullptr) {
      return internalError(request);
    }
    bytes.assign(data, data + encoded.size());
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationQueryLogicalTimeResult(
          ProcessFederationQueryLogicalTimeResult{
          ProcessFederationLogicalTime{
                  timeState->implementationName(),
                  std::move(bytes)}}));
}

TransportServiceMessage ProcessFederationService::handleQueryLookahead(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const query = decodeProcessFederationQueryLogicalTimeRequest(request.payload);
  std::shared_ptr<FederateTimeState> timeState;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != query.federationName ||
        state->second.federateId != query.federateId ||
        !registry_.memberById(query.federationName, query.federateId)) {
      return rejected(request);
    }
    timeState = state->second.timeState;
  }

  if (!timeState) {
    return internalError(request);
  }

  auto const current = timeState->currentLookahead();
  ProcessFederationQueryLookaheadResult result;
  switch (current.status) {
    case FederateTimeLookaheadStatus::applied: {
      if (!current.lookahead ||
          current.lookahead->implementationName() !=
              timeState->implementationName()) {
        return internalError(request);
      }
      auto const encoded = current.lookahead->encode();
      std::vector<std::uint8_t> bytes;
      if (encoded.size() != 0U) {
        auto const* data = static_cast<std::uint8_t const*>(encoded.data());
        if (data == nullptr) {
          return internalError(request);
        }
        bytes.assign(data, data + encoded.size());
      }
      result.status = ProcessFederationLookaheadStatus::applied;
      result.lookahead = ProcessFederationLogicalTimeInterval{
          timeState->implementationName(), std::move(bytes)};
      break;
    }
    case FederateTimeLookaheadStatus::not_enabled:
      result.status = ProcessFederationLookaheadStatus::not_enabled;
      break;
    case FederateTimeLookaheadStatus::inactive:
      result.status = ProcessFederationLookaheadStatus::inactive;
      break;
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationQueryLookaheadResult(result));
}

TransportServiceMessage ProcessFederationService::handleModifyLookahead(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const modify =
      decodeProcessFederationModifyLookaheadRequest(request.payload);
  std::shared_ptr<FederateTimeState> timeState;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != modify.federationName ||
        state->second.federateId != modify.federateId ||
        !registry_.memberById(modify.federationName, modify.federateId)) {
      return rejected(request);
    }
    timeState = state->second.timeState;
  }

  if (!timeState) {
    return internalError(request);
  }

  ProcessFederationModifyLookaheadResult processResult;
  processResult.status =
      ProcessFederationModifyLookaheadStatus::invalid_lookahead;
  if (modify.lookahead.implementationName != timeState->implementationName()) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationModifyLookaheadResult(processResult));
  }

  auto factory = rti1516_2025::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
      timeState->implementationName());
  if (!factory || factory->getName() != timeState->implementationName()) {
    return internalError(request);
  }
  rti1516_2025::VariableLengthData encodedLookahead;
  if (!modify.lookahead.encoding.empty()) {
    encodedLookahead.setData(
        modify.lookahead.encoding.data(), modify.lookahead.encoding.size());
  }
  std::unique_ptr<rti1516_2025::LogicalTimeInterval> decodedLookahead;
  try {
    decodedLookahead = factory->decodeLogicalTimeInterval(encodedLookahead);
    auto zero = factory->makeZero();
    if (!decodedLookahead || !zero ||
        decodedLookahead->implementationName() != timeState->implementationName() ||
        *decodedLookahead < *zero) {
      return responseFor(
          request,
          TransportServiceStatus::ok,
          encodeProcessFederationModifyLookaheadResult(processResult));
    }
  } catch (rti1516_2025::Exception const&) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationModifyLookaheadResult(processResult));
  }

  auto const stateResult = timeState->modifyLookahead(
      std::shared_ptr<rti1516_2025::LogicalTimeInterval>(
          std::move(decodedLookahead)));
  switch (stateResult) {
    case FederateTimeModifyLookaheadStatus::applied:
      processResult.status = ProcessFederationModifyLookaheadStatus::applied;
      break;
    case FederateTimeModifyLookaheadStatus::time_advance_pending:
      processResult.status =
          ProcessFederationModifyLookaheadStatus::time_advance_pending;
      break;
    case FederateTimeModifyLookaheadStatus::not_enabled:
      processResult.status = ProcessFederationModifyLookaheadStatus::not_enabled;
      break;
    case FederateTimeModifyLookaheadStatus::inactive:
      processResult.status = ProcessFederationModifyLookaheadStatus::inactive;
      break;
  }

  if (stateResult == FederateTimeModifyLookaheadStatus::applied) {
    auto scheduled = registry_.reevaluateTimeAdvanceGrants(
        modify.federationName);
    if (scheduled.status != FederationTimeGrantStatus::applied) {
      return internalError(request);
    }
    dispatchTimeAdvanceGrants(
        modify.federationName,
        std::move(scheduled.dispatches));
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationModifyLookaheadResult(processResult));
}

TransportServiceMessage ProcessFederationService::handleQueryTimeBounds(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const query = decodeProcessFederationQueryLogicalTimeRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != query.federationName ||
        state->second.federateId != query.federateId ||
        !registry_.memberById(query.federationName, query.federateId)) {
      return rejected(request);
    }
  }

  auto const snapshot = registry_.timeSnapshotFor(query.federationName);
  if (!snapshot) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationQueryTimeBoundsResult(
            ProcessFederationQueryTimeBoundsResult{
                ProcessFederationTimeBoundStatus::inconsistent_temporal_state,
                std::nullopt,
                std::nullopt}));
  }
  auto const bounds = FederationTimeBoundsCalculator{}.calculate(
      *snapshot, query.federateId);
  ProcessFederationQueryTimeBoundsResult result;
  switch (bounds.status) {
    case FederationTimeBoundStatus::available:
      result.status = ProcessFederationTimeBoundStatus::available;
      result.galt = encodeProcessLogicalTime(bounds.galt);
      result.lits = encodeProcessLogicalTime(bounds.lits);
      break;
    case FederationTimeBoundStatus::undefined:
      result.status = ProcessFederationTimeBoundStatus::undefined;
      result.lits = encodeProcessLogicalTime(bounds.lits);
      break;
    case FederationTimeBoundStatus::requesting_federate_not_registered:
      result.status =
          ProcessFederationTimeBoundStatus::requesting_federate_not_registered;
      break;
    case FederationTimeBoundStatus::factory_unavailable:
      result.status = ProcessFederationTimeBoundStatus::factory_unavailable;
      break;
    case FederationTimeBoundStatus::inconsistent_temporal_state:
      result.status =
          ProcessFederationTimeBoundStatus::inconsistent_temporal_state;
      break;
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationQueryTimeBoundsResult(result));
}

TransportServiceMessage ProcessFederationService::handleEnableTimeRegulation(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const enable =
      decodeProcessFederationEnableTimeRegulationRequest(request.payload);
  std::shared_ptr<FederateTimeState> timeState;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != enable.federationName ||
        state->second.federateId != enable.federateId ||
        !registry_.memberById(enable.federationName, enable.federateId)) {
      return rejected(request);
    }
    timeState = state->second.timeState;
  }
  if (!timeState) {
    return internalError(request);
  }

  // Decode the official interval before touching the federate state.  A
  // mismatched implementation or negative interval is a normal
  // InvalidLookahead outcome, while an unavailable factory is an endpoint
  // configuration failure.
  ProcessFederationEnableTimeRegulationResult processResult;
  processResult.status = ProcessFederationTimeEnableStatus::invalid_lookahead;
  if (enable.lookahead.implementationName != timeState->implementationName()) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationEnableTimeRegulationResult(processResult));
  }
  auto factory = rti1516_2025::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
      timeState->implementationName());
  if (!factory || factory->getName() != timeState->implementationName()) {
    return internalError(request);
  }
  rti1516_2025::VariableLengthData encodedLookahead;
  if (!enable.lookahead.encoding.empty()) {
    encodedLookahead.setData(
        enable.lookahead.encoding.data(), enable.lookahead.encoding.size());
  }
  std::unique_ptr<rti1516_2025::LogicalTimeInterval> decodedLookahead;
  try {
    decodedLookahead = factory->decodeLogicalTimeInterval(encodedLookahead);
  } catch (rti1516_2025::Exception const&) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationEnableTimeRegulationResult(processResult));
  }
  if (!decodedLookahead ||
      decodedLookahead->implementationName() != timeState->implementationName()) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationEnableTimeRegulationResult(processResult));
  }
  try {
    auto zero = factory->makeZero();
    if (!zero || *decodedLookahead < *zero) {
      return responseFor(
          request,
          TransportServiceStatus::ok,
          encodeProcessFederationEnableTimeRegulationResult(processResult));
    }
  } catch (rti1516_2025::Exception const&) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationEnableTimeRegulationResult(processResult));
  }

  auto requestResult = timeState->requestTimeRegulation(
      std::shared_ptr<rti1516_2025::LogicalTimeInterval>(
          std::move(decodedLookahead)));
  switch (requestResult.status) {
    case FederateTimeEnableStatus::applied:
      processResult.status = ProcessFederationTimeEnableStatus::applied;
      break;
    case FederateTimeEnableStatus::time_advance_pending:
      processResult.status =
          ProcessFederationTimeEnableStatus::time_advance_pending;
      break;
    case FederateTimeEnableStatus::request_pending:
      processResult.status = ProcessFederationTimeEnableStatus::request_pending;
      break;
    case FederateTimeEnableStatus::already_enabled:
      processResult.status = ProcessFederationTimeEnableStatus::already_enabled;
      break;
    case FederateTimeEnableStatus::invalid_lookahead:
      processResult.status =
          ProcessFederationTimeEnableStatus::invalid_lookahead;
      break;
    case FederateTimeEnableStatus::inactive:
      processResult.status = ProcessFederationTimeEnableStatus::inactive;
      break;
    case FederateTimeEnableStatus::generation_exhausted:
      processResult.status =
          ProcessFederationTimeEnableStatus::generation_exhausted;
      break;
  }

  if (requestResult.status == FederateTimeEnableStatus::applied) {
    // Complete the accepted role request against server-owned state and return
    // the exact value that the client callback bridge will deliver through
    // FederateAmbassador::timeRegulationEnabled. Any newly eligible grants are
    // dispatched through the federation-owned scheduler after this transition.
    auto enabledTime = timeState->grantTimeRegulation(requestResult.generation);
    if (!enabledTime) {
      return internalError(request);
    }
    auto const encodedTime = enabledTime->encode();
    std::vector<std::uint8_t> bytes;
    if (encodedTime.size() != 0U) {
      auto const* data = static_cast<unsigned char const*>(encodedTime.data());
      if (data == nullptr) {
        return internalError(request);
      }
      bytes.assign(data, data + encodedTime.size());
    }
    processResult.enabledTime = ProcessFederationLogicalTime{
        timeState->implementationName(), std::move(bytes)};
    auto scheduled = registry_.reevaluateTimeAdvanceGrants(enable.federationName);
    if (scheduled.status != FederationTimeGrantStatus::applied) {
      return internalError(request);
    }
    dispatchTimeAdvanceGrants(
        enable.federationName,
        std::move(scheduled.dispatches));
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationEnableTimeRegulationResult(processResult));
}

TransportServiceMessage ProcessFederationService::handleEnableTimeConstrained(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const enable =
      decodeProcessFederationEnableTimeConstrainedRequest(request.payload);
  std::shared_ptr<FederateTimeState> timeState;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != enable.federationName ||
        state->second.federateId != enable.federateId ||
        !registry_.memberById(enable.federationName, enable.federateId)) {
      return rejected(request);
    }
    timeState = state->second.timeState;
  }
  if (!timeState) {
    return internalError(request);
  }

  auto const requestResult = timeState->requestTimeConstrained();
  ProcessFederationEnableTimeConstrainedResult processResult;
  switch (requestResult.status) {
    case FederateTimeEnableStatus::applied:
      processResult.status = ProcessFederationTimeEnableStatus::applied;
      break;
    case FederateTimeEnableStatus::time_advance_pending:
      processResult.status =
          ProcessFederationTimeEnableStatus::time_advance_pending;
      break;
    case FederateTimeEnableStatus::request_pending:
      processResult.status = ProcessFederationTimeEnableStatus::request_pending;
      break;
    case FederateTimeEnableStatus::already_enabled:
      processResult.status = ProcessFederationTimeEnableStatus::already_enabled;
      break;
    case FederateTimeEnableStatus::invalid_lookahead:
      processResult.status =
          ProcessFederationTimeEnableStatus::invalid_lookahead;
      break;
    case FederateTimeEnableStatus::inactive:
      processResult.status = ProcessFederationTimeEnableStatus::inactive;
      break;
    case FederateTimeEnableStatus::generation_exhausted:
      processResult.status =
          ProcessFederationTimeEnableStatus::generation_exhausted;
      break;
  }

  if (requestResult.status == FederateTimeEnableStatus::applied) {
    // Complete the accepted role request against server-owned state and
    // return the exact callback value. Any newly eligible cross-federate time
    // grants are dispatched after this transition through the shared scheduler.
    auto enabledTime = timeState->grantTimeConstrained(requestResult.generation);
    if (!enabledTime) {
      return internalError(request);
    }
    processResult.enabledTime = encodeProcessLogicalTime(enabledTime);
    if (!processResult.enabledTime) {
      return internalError(request);
    }
    auto scheduled = registry_.reevaluateTimeAdvanceGrants(enable.federationName);
    if (scheduled.status != FederationTimeGrantStatus::applied) {
      return internalError(request);
    }
    dispatchTimeAdvanceGrants(
        enable.federationName,
        std::move(scheduled.dispatches));
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationEnableTimeConstrainedResult(processResult));
}

TransportServiceMessage ProcessFederationService::handleDisableTimeRegulation(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  return handleDisableTimeRole(session, request, true);
}

TransportServiceMessage ProcessFederationService::handleDisableTimeConstrained(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  return handleDisableTimeRole(session, request, false);
}

TransportServiceMessage ProcessFederationService::handleDisableTimeRole(
    ProcessTransportSession& session,
    TransportServiceMessage const& request,
    bool regulation) {
  auto const disable =
      decodeProcessFederationQueryLogicalTimeRequest(request.payload);
  std::shared_ptr<FederateTimeState> timeState;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != disable.federationName ||
        state->second.federateId != disable.federateId ||
        !registry_.memberById(disable.federationName, disable.federateId)) {
      return rejected(request);
    }
    timeState = state->second.timeState;
  }
  if (!timeState) {
    return internalError(request);
  }

  auto const stateResult = regulation
      ? timeState->disableTimeRegulation()
      : timeState->disableTimeConstrained();
  ProcessFederationTimeDisableResult processResult;
  switch (stateResult) {
    case FederateTimeDisableStatus::applied:
      processResult.status = ProcessFederationTimeDisableStatus::applied;
      break;
    case FederateTimeDisableStatus::not_enabled:
      processResult.status = ProcessFederationTimeDisableStatus::not_enabled;
      break;
    case FederateTimeDisableStatus::inactive:
      processResult.status = ProcessFederationTimeDisableStatus::inactive;
      break;
  }

  if (stateResult == FederateTimeDisableStatus::applied) {
    // Disabling either role can release a TAR that was waiting on the
    // corresponding temporal bound. Re-evaluate only after the role state has
    // changed, then dispatch any newly eligible grants through the same
    // federation-owned callback path used by enable and lookahead services.
    auto scheduled = registry_.reevaluateTimeAdvanceGrants(
        disable.federationName);
    if (scheduled.status != FederationTimeGrantStatus::applied) {
      return internalError(request);
    }
    dispatchTimeAdvanceGrants(
        disable.federationName,
        std::move(scheduled.dispatches));
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationTimeDisableResult(processResult));
}

TransportServiceMessage ProcessFederationService::handleTimeAdvanceRequest(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  return handleTimeAdvanceRequestForMode(
      session, request, FederateTimeAdvanceMode::time_advance_request);
}

TransportServiceMessage
ProcessFederationService::handleTimeAdvanceRequestAvailable(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  return handleTimeAdvanceRequestForMode(
      session,
      request,
      FederateTimeAdvanceMode::time_advance_request_available);
}

TransportServiceMessage ProcessFederationService::handleNextMessageRequest(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  return handleTimeAdvanceRequestForMode(
      session, request, FederateTimeAdvanceMode::next_message_request);
}

TransportServiceMessage
ProcessFederationService::handleNextMessageRequestAvailable(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  return handleTimeAdvanceRequestForMode(
      session,
      request,
      FederateTimeAdvanceMode::next_message_request_available);
}

TransportServiceMessage ProcessFederationService::handleFlushQueueRequest(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  return handleTimeAdvanceRequestForMode(
      session, request, FederateTimeAdvanceMode::flush_queue_request);
}

TransportServiceMessage ProcessFederationService::handleTimeAdvanceRequestForMode(
    ProcessTransportSession& session,
    TransportServiceMessage const& request,
    FederateTimeAdvanceMode mode) {
  auto const advance =
      decodeProcessFederationTimeAdvanceRequest(request.payload);
  std::shared_ptr<FederateTimeState> timeState;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != advance.federationName ||
        state->second.federateId != advance.federateId ||
        !registry_.memberById(advance.federationName, advance.federateId)) {
      return rejected(request);
    }
    timeState = state->second.timeState;
  }
  if (!timeState) {
    return internalError(request);
  }

  ProcessFederationTimeAdvanceResult processResult;
  std::shared_ptr<rti1516_2025::LogicalTime const> decodedRequestedTime;
  try {
    decodedRequestedTime = decodeProcessLogicalTime(
        advance.requestedTime, timeState->implementationName());
  } catch (ProcessFederationServiceProtocolError const&) {
    processResult.status =
        ProcessFederationTimeAdvanceStatus::invalid_logical_time;
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationTimeAdvanceResult(processResult));
  }

  std::shared_ptr<rti1516_2025::LogicalTime> effectiveTime;
  if (mode == FederateTimeAdvanceMode::next_message_request ||
      mode == FederateTimeAdvanceMode::next_message_request_available) {
    auto const queued = registry_.earliestTsoTimestampFor(
        advance.federationName, advance.federateId);
    if (queued && *queued) {
      try {
        auto encodedQueued = encodeProcessLogicalTime(*queued);
        if (encodedQueued) {
          auto candidate = decodeProcessLogicalTime(
              *encodedQueued, timeState->implementationName());
          auto const currentTime = timeState->currentTime();
          if (currentTime && *candidate >= *currentTime &&
              *candidate <= *decodedRequestedTime) {
            effectiveTime = std::const_pointer_cast<rti1516_2025::LogicalTime>(
                std::move(candidate));
          }
        }
      } catch (rti1516_2025::Exception const&) {
        processResult.status =
            ProcessFederationTimeAdvanceStatus::invalid_logical_time;
        return responseFor(
            request,
            TransportServiceStatus::ok,
            encodeProcessFederationTimeAdvanceResult(processResult));
      } catch (ProcessFederationServiceProtocolError const&) {
        processResult.status =
            ProcessFederationTimeAdvanceStatus::invalid_logical_time;
        return responseFor(
            request,
            TransportServiceStatus::ok,
            encodeProcessFederationTimeAdvanceResult(processResult));
      }
    }
  }

  FederateTimeAdvanceResult requestResult;
  auto mutableRequestedTime = std::const_pointer_cast<rti1516_2025::LogicalTime>(
      std::move(decodedRequestedTime));
  if (mode == FederateTimeAdvanceMode::time_advance_request_available) {
    requestResult = timeState->requestAdvanceAvailable(
        std::move(mutableRequestedTime));
  } else if (mode == FederateTimeAdvanceMode::next_message_request) {
    requestResult = timeState->requestNextMessageAdvance(
        std::move(mutableRequestedTime), std::move(effectiveTime));
  } else if (mode == FederateTimeAdvanceMode::next_message_request_available) {
    requestResult = timeState->requestNextMessageAvailableAdvance(
        std::move(mutableRequestedTime), std::move(effectiveTime));
  } else if (mode == FederateTimeAdvanceMode::flush_queue_request) {
    requestResult = timeState->requestFlushQueueAdvance(
        std::move(mutableRequestedTime));
  } else {
    requestResult = timeState->requestAdvance(std::move(mutableRequestedTime));
  }
  switch (requestResult.status) {
    case FederateTimeAdvanceStatus::applied:
      processResult.status = ProcessFederationTimeAdvanceStatus::applied;
      break;
    case FederateTimeAdvanceStatus::logical_time_already_passed:
      processResult.status =
          ProcessFederationTimeAdvanceStatus::logical_time_already_passed;
      break;
    case FederateTimeAdvanceStatus::time_advance_pending:
      processResult.status =
          ProcessFederationTimeAdvanceStatus::time_advance_pending;
      break;
    case FederateTimeAdvanceStatus::time_regulation_pending:
      processResult.status =
          ProcessFederationTimeAdvanceStatus::time_regulation_pending;
      break;
    case FederateTimeAdvanceStatus::time_constrained_pending:
      processResult.status =
          ProcessFederationTimeAdvanceStatus::time_constrained_pending;
      break;
    case FederateTimeAdvanceStatus::invalid_logical_time:
      processResult.status =
          ProcessFederationTimeAdvanceStatus::invalid_logical_time;
      break;
    case FederateTimeAdvanceStatus::inactive:
      processResult.status = ProcessFederationTimeAdvanceStatus::inactive;
      break;
    case FederateTimeAdvanceStatus::generation_exhausted:
      processResult.status =
          ProcessFederationTimeAdvanceStatus::generation_exhausted;
      break;
  }

  if (requestResult.status == FederateTimeAdvanceStatus::applied) {
    // Register the accepted request with the federation-owned scheduler. An
    // eligible grant is sent as an unsolicited event (possibly before this
    // response); a constrained request remains pending until a later temporal
    // state change re-evaluates the shared bound.
    auto scheduled = registry_.requestTimeAdvanceGrant(
        advance.federationName,
        advance.federateId,
        requestResult.generation);
    if (scheduled.status != FederationTimeGrantStatus::applied) {
      return internalError(request);
    }
    dispatchTimeAdvanceGrants(
        advance.federationName,
        std::move(scheduled.dispatches));
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationTimeAdvanceResult(processResult));
}

TransportServiceMessage ProcessFederationService::handleGetInteractionClassHandle(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const lookupRequest =
      decodeProcessFederationGetInteractionClassHandleRequest(request.payload);
  auto const encodedName = utf8FromWide(lookupRequest.interactionClassName);
  if (!encodedName) {
    return rejected(request);
  }

  std::optional<std::uint64_t> handle;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != lookupRequest.federationName ||
        state->second.federateId != lookupRequest.federateId ||
        !registry_.memberById(
            lookupRequest.federationName, lookupRequest.federateId)) {
      return rejected(request);
    }
    handle = registry_.interactionClassHandleFor(
        lookupRequest.federationName, *encodedName);
  }
  if (!handle) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationHandleResult(ProcessFederationHandleResult{*handle}));
}

TransportServiceMessage ProcessFederationService::handleGetObjectClassHandle(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const lookupRequest =
      decodeProcessFederationGetObjectClassHandleRequest(request.payload);
  auto const encodedName = utf8FromWide(lookupRequest.objectClassName);
  if (!encodedName) {
    return rejected(request);
  }

  std::optional<std::uint64_t> handle;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != lookupRequest.federationName ||
        state->second.federateId != lookupRequest.federateId ||
        !registry_.memberById(
            lookupRequest.federationName, lookupRequest.federateId)) {
      return rejected(request);
    }
    handle = registry_.objectClassHandleFor(
        lookupRequest.federationName, *encodedName);
  }
  if (!handle) {
    return rejected(request);
  }
  if (lookupRequest.callbacksEnabled &&
      !flushDeferredAttributeOwnershipAssumptionEvents(
          session,
          lookupRequest.federationName,
          lookupRequest.federateId)) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationHandleResult(ProcessFederationHandleResult{*handle}));
}

TransportServiceMessage ProcessFederationService::handleGetParameterHandle(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const lookupRequest =
      decodeProcessFederationGetParameterHandleRequest(request.payload);
  auto const encodedName = utf8FromWide(lookupRequest.parameterName);
  if (!encodedName) {
    return rejected(request);
  }

  std::optional<std::uint64_t> handle;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != lookupRequest.federationName ||
        state->second.federateId != lookupRequest.federateId ||
        !registry_.memberById(
            lookupRequest.federationName, lookupRequest.federateId)) {
      return rejected(request);
    }
    auto const interactionClassName = registry_.interactionClassNameFor(
        lookupRequest.federationName, lookupRequest.interactionClassHandle);
    if (!interactionClassName) {
      return rejected(request);
    }
    handle = registry_.parameterHandleFor(
        lookupRequest.federationName, *interactionClassName, *encodedName);
  }
  if (!handle) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationHandleResult(ProcessFederationHandleResult{*handle}));
}

TransportServiceMessage ProcessFederationService::handleGetAttributeHandle(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const lookupRequest =
      decodeProcessFederationGetAttributeHandleRequest(request.payload);
  auto const encodedName = utf8FromWide(lookupRequest.attributeName);
  if (!encodedName) {
    return rejected(request);
  }

  std::optional<std::uint64_t> handle;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != lookupRequest.federationName ||
        state->second.federateId != lookupRequest.federateId ||
        !registry_.memberById(
            lookupRequest.federationName, lookupRequest.federateId)) {
      return rejected(request);
    }
    auto const objectClassName = registry_.objectClassNameFor(
        lookupRequest.federationName, lookupRequest.objectClassHandle);
    if (!objectClassName) {
      return rejected(request);
    }
    handle = registry_.attributeHandleFor(
        lookupRequest.federationName, *objectClassName, *encodedName);
  }
  if (!handle) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationHandleResult(ProcessFederationHandleResult{*handle}));
}

TransportServiceMessage ProcessFederationService::handleGetObjectInstanceHandle(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const lookupRequest =
      decodeProcessFederationGetObjectInstanceHandleRequest(request.payload);
  std::optional<KnownObjectInstanceSnapshot> known;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != lookupRequest.federationName ||
        state->second.federateId != lookupRequest.federateId ||
        !registry_.memberById(
            lookupRequest.federationName, lookupRequest.federateId)) {
      return rejected(request);
    }
    known = registry_.knownObjectInstanceByNameFor(
        lookupRequest.federationName,
        lookupRequest.federateId,
        lookupRequest.objectInstanceName);
  }
  if (!known || known->objectInstanceHandle == 0U) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationHandleResult(
          ProcessFederationHandleResult{known->objectInstanceHandle}));
}

TransportServiceMessage ProcessFederationService::handleGetObjectInstanceName(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const lookupRequest =
      decodeProcessFederationGetObjectInstanceNameRequest(request.payload);
  std::optional<KnownObjectInstanceSnapshot> known;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != lookupRequest.federationName ||
        state->second.federateId != lookupRequest.federateId ||
        !registry_.memberById(
            lookupRequest.federationName, lookupRequest.federateId)) {
      return rejected(request);
    }
    known = registry_.knownObjectInstanceFor(
        lookupRequest.federationName,
        lookupRequest.federateId,
        lookupRequest.objectInstanceHandle);
  }
  if (!known || known->objectInstanceName.empty()) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationStringResult(
          ProcessFederationStringResult{std::move(known->objectInstanceName)}));
}

TransportServiceMessage
ProcessFederationService::handleGetKnownObjectClassHandle(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const lookupRequest =
      decodeProcessFederationGetKnownObjectClassHandleRequest(request.payload);
  std::optional<KnownObjectInstanceSnapshot> known;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != lookupRequest.federationName ||
        state->second.federateId != lookupRequest.federateId ||
        !registry_.memberById(
            lookupRequest.federationName, lookupRequest.federateId)) {
      return rejected(request);
    }
    known = registry_.knownObjectInstanceFor(
        lookupRequest.federationName,
        lookupRequest.federateId,
        lookupRequest.objectInstanceHandle);
  }
  if (!known || known->knownObjectClassHandle == 0U) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationHandleResult(
          ProcessFederationHandleResult{known->knownObjectClassHandle}));
}

TransportServiceMessage ProcessFederationService::handleGetObjectClassName(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const lookupRequest =
      decodeProcessFederationGetObjectClassNameRequest(request.payload);
  std::optional<std::string> encodedName;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != lookupRequest.federationName ||
        state->second.federateId != lookupRequest.federateId ||
        !registry_.memberById(
            lookupRequest.federationName, lookupRequest.federateId)) {
      return rejected(request);
    }
    encodedName = registry_.objectClassNameFor(
        lookupRequest.federationName, lookupRequest.objectClassHandle);
  }
  if (!encodedName) {
    return rejected(request);
  }
  auto const decodedName = wideFromUtf8(*encodedName);
  if (!decodedName) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationStringResult(
          ProcessFederationStringResult{*decodedName}));
}

TransportServiceMessage ProcessFederationService::handleGetInteractionClassName(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const lookupRequest =
      decodeProcessFederationGetInteractionClassNameRequest(request.payload);
  std::optional<std::string> encodedName;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != lookupRequest.federationName ||
        state->second.federateId != lookupRequest.federateId ||
        !registry_.memberById(
            lookupRequest.federationName, lookupRequest.federateId)) {
      return rejected(request);
    }
    encodedName = registry_.interactionClassNameFor(
        lookupRequest.federationName, lookupRequest.interactionClassHandle);
  }
  if (!encodedName) {
    return rejected(request);
  }
  auto const decodedName = wideFromUtf8(*encodedName);
  if (!decodedName) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationStringResult(
          ProcessFederationStringResult{*decodedName}));
}

TransportServiceMessage ProcessFederationService::handleGetAttributeName(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const lookupRequest =
      decodeProcessFederationGetAttributeNameRequest(request.payload);
  std::optional<std::string> encodedName;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != lookupRequest.federationName ||
        state->second.federateId != lookupRequest.federateId ||
        !registry_.memberById(
            lookupRequest.federationName, lookupRequest.federateId)) {
      return rejected(request);
    }
    auto const objectClassName = registry_.objectClassNameFor(
        lookupRequest.federationName, lookupRequest.objectClassHandle);
    if (!objectClassName) {
      return rejected(request);
    }
    encodedName = registry_.attributeNameFor(
        lookupRequest.federationName,
        *objectClassName,
        lookupRequest.attributeHandle);
  }
  if (!encodedName) {
    return rejected(request);
  }
  auto const decodedName = wideFromUtf8(*encodedName);
  if (!decodedName) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationStringResult(
          ProcessFederationStringResult{*decodedName}));
}

TransportServiceMessage ProcessFederationService::handleGetParameterName(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const lookupRequest =
      decodeProcessFederationGetParameterNameRequest(request.payload);
  std::optional<std::string> encodedName;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != lookupRequest.federationName ||
        state->second.federateId != lookupRequest.federateId ||
        !registry_.memberById(
            lookupRequest.federationName, lookupRequest.federateId)) {
      return rejected(request);
    }
    auto const interactionClassName = registry_.interactionClassNameFor(
        lookupRequest.federationName, lookupRequest.interactionClassHandle);
    if (!interactionClassName) {
      return rejected(request);
    }
    encodedName = registry_.parameterNameFor(
        lookupRequest.federationName,
        *interactionClassName,
        lookupRequest.parameterHandle);
  }
  if (!encodedName) {
    return rejected(request);
  }
  auto const decodedName = wideFromUtf8(*encodedName);
  if (!decodedName) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationStringResult(
          ProcessFederationStringResult{*decodedName}));
}

TransportServiceMessage ProcessFederationService::handleInteractionClassDeclaration(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const declarationRequest =
      decodeProcessFederationInteractionClassDeclarationRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != declarationRequest.federationName ||
        state->second.federateId != declarationRequest.federateId ||
        !registry_.memberById(
            declarationRequest.federationName, declarationRequest.federateId)) {
      return rejected(request);
    }
  }

  InteractionClassDeclarationStatus result =
      InteractionClassDeclarationStatus::federation_does_not_exist;
  switch (request.operation) {
    case TransportServiceOperation::publish_interaction_class:
      result = registry_.setInteractionClassPublication(
          declarationRequest.federationName,
          declarationRequest.federateId,
          declarationRequest.interactionClassHandle,
          true);
      break;
    case TransportServiceOperation::unpublish_interaction_class:
      result = registry_.setInteractionClassPublication(
          declarationRequest.federationName,
          declarationRequest.federateId,
          declarationRequest.interactionClassHandle,
          false);
      break;
    case TransportServiceOperation::subscribe_interaction_class:
      result = registry_.setInteractionClassSubscription(
          declarationRequest.federationName,
          declarationRequest.federateId,
          declarationRequest.interactionClassHandle,
          declarationRequest.active);
      break;
    case TransportServiceOperation::unsubscribe_interaction_class:
      result = registry_.setInteractionClassSubscription(
          declarationRequest.federationName,
          declarationRequest.federateId,
          declarationRequest.interactionClassHandle,
          std::nullopt);
      break;
    default:
      return invalid(request);
  }
  if (result != InteractionClassDeclarationStatus::applied) {
    return rejected(request);
  }
  return responseFor(request, TransportServiceStatus::ok);
}

TransportServiceMessage ProcessFederationService::handleChangeInteractionOrderType(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const changeRequest =
      decodeProcessFederationChangeInteractionOrderTypeRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != changeRequest.federationName ||
        state->second.federateId != changeRequest.federateId ||
        !registry_.memberById(
            changeRequest.federationName, changeRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const status = registry_.changeInteractionOrderType(
      changeRequest.federationName,
      changeRequest.federateId,
      changeRequest.interactionClassHandle,
      changeRequest.orderType);
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationInteractionOrderTypeChangeResult(
          ProcessFederationInteractionOrderTypeChangeResult{status}));
}

TransportServiceMessage ProcessFederationService::handleChangeAttributeOrderType(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const changeRequest =
      decodeProcessFederationChangeAttributeOrderTypeRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != changeRequest.federationName ||
        state->second.federateId != changeRequest.federateId ||
        !registry_.memberById(
            changeRequest.federationName, changeRequest.federateId)) {
      return rejected(request);
    }
  }
  std::set<std::uint64_t> attributeHandles(
      changeRequest.attributeHandles.begin(),
      changeRequest.attributeHandles.end());
  auto const status = registry_.changeAttributeOrderType(
      changeRequest.federationName,
      changeRequest.federateId,
      changeRequest.objectInstanceHandle,
      attributeHandles,
      changeRequest.orderType);
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationAttributeOrderTypeChangeResult(
          ProcessFederationAttributeOrderTypeChangeResult{status}));
}

TransportServiceMessage
ProcessFederationService::handleChangeDefaultAttributeOrderType(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const changeRequest =
      decodeProcessFederationChangeDefaultAttributeOrderTypeRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != changeRequest.federationName ||
        state->second.federateId != changeRequest.federateId ||
        !registry_.memberById(
            changeRequest.federationName, changeRequest.federateId)) {
      return rejected(request);
    }
  }
  std::set<std::uint64_t> attributeHandles(
      changeRequest.attributeHandles.begin(),
      changeRequest.attributeHandles.end());
  auto const status = registry_.changeDefaultAttributeOrderType(
      changeRequest.federationName,
      changeRequest.federateId,
      changeRequest.objectClassHandle,
      attributeHandles,
      changeRequest.orderType);
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationAttributeOrderTypeDefaultResult(
          ProcessFederationAttributeOrderTypeDefaultResult{status}));
}

TransportServiceMessage
ProcessFederationService::handleChangeDefaultAttributeTransportationType(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const changeRequest =
      decodeProcessFederationChangeDefaultAttributeTransportationTypeRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != changeRequest.federationName ||
        state->second.federateId != changeRequest.federateId ||
        !registry_.memberById(
            changeRequest.federationName, changeRequest.federateId)) {
      return rejected(request);
    }
  }
  std::set<std::uint64_t> attributeHandles(
      changeRequest.attributeHandles.begin(),
      changeRequest.attributeHandles.end());
  auto const transportationName = registry_.transportationTypeNameFor(
      changeRequest.federationName,
      changeRequest.transportationTypeHandle);
  auto const status = transportationName
                          ? registry_.changeDefaultAttributeTransportationType(
                                changeRequest.federationName,
                                changeRequest.federateId,
                                changeRequest.objectClassHandle,
                                attributeHandles,
                                *transportationName)
                          : AttributeTransportationTypeDefaultStatus::
                                invalid_transportation_type;
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationAttributeTransportationTypeDefaultResult(
          ProcessFederationAttributeTransportationTypeDefaultResult{status}));
}

TransportServiceMessage
ProcessFederationService::handleRequestAttributeTransportationTypeChange(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const changeRequest =
      decodeProcessFederationRequestAttributeTransportationTypeChangeRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != changeRequest.federationName ||
        state->second.federateId != changeRequest.requestingFederateId ||
        !registry_.memberById(
            changeRequest.federationName, changeRequest.requestingFederateId)) {
      return rejected(request);
    }
  }
  auto const transportationName = registry_.transportationTypeNameFor(
      changeRequest.federationName,
      changeRequest.transportationTypeHandle);
  auto plan = transportationName
                  ? registry_.planAttributeTransportationTypeChange(
                        changeRequest.federationName,
                        changeRequest.requestingFederateId,
                        changeRequest.objectInstanceHandle,
                        std::set<std::uint64_t>(
                            changeRequest.attributeHandles.begin(),
                            changeRequest.attributeHandles.end()),
                        *transportationName)
                  : AttributeTransportationTypeChangePlan{
                        AttributeTransportationTypeChangeStatus::
                            invalid_transportation_type};
  if (plan.status == AttributeTransportationTypeChangeStatus::applied &&
      plan.requestId != 0U) {
    ProcessFederationAttributeTransportationTypeChangeEvent event{
        plan.requestId,
        changeRequest.requestingFederateId,
        plan.objectInstanceHandle,
        std::move(plan.attributeHandles),
        std::move(plan.transportationName)};
    if (options_.pushReceiveOrderEvents) {
      if (!session.send(TransportServiceMessage{
              TransportServiceMessageKind::event,
              TransportServiceOperation::receive_interaction,
              TransportServiceStatus::ok,
              0U,
              encodeProcessFederationReceiveInteractionResult(
                  ProcessFederationReceiveInteractionResult{
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::move(event),
                      std::nullopt})})) {
        return internalError(request);
      }
    } else {
      std::scoped_lock lock(mutex_);
      auto const state = sessions_.find(&session);
      if (state == sessions_.end()) {
        registry_.cancelAttributeTransportationTypeChange(
            changeRequest.federationName,
            changeRequest.requestingFederateId,
            plan.requestId);
        return internalError(request);
      }
      state->second.attributeTransportationTypeChangeEvents.push_back(
          std::move(event));
    }
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationAttributeTransportationTypeChangeResult(
          ProcessFederationAttributeTransportationTypeChangeResult{
              plan.status}));
}

TransportServiceMessage
ProcessFederationService::handleQueryAttributeTransportationType(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const queryRequest =
      decodeProcessFederationQueryAttributeTransportationTypeRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != queryRequest.federationName ||
        state->second.federateId != queryRequest.requestingFederateId ||
        !registry_.memberById(
            queryRequest.federationName, queryRequest.requestingFederateId)) {
      return rejected(request);
    }
  }
  auto const plan = registry_.planAttributeTransportationTypeQuery(
      queryRequest.federationName,
      queryRequest.requestingFederateId,
      queryRequest.objectInstanceHandle,
      queryRequest.attributeHandle);
  if (plan.status == AttributeTransportationTypeQueryStatus::applied) {
    ProcessFederationAttributeTransportationTypeQueryEvent event{
        queryRequest.requestingFederateId,
        plan.objectInstanceHandle,
        plan.attributeHandle,
        plan.transportationName};
    if (options_.pushReceiveOrderEvents) {
      if (!session.send(TransportServiceMessage{
              TransportServiceMessageKind::event,
              TransportServiceOperation::receive_interaction,
              TransportServiceStatus::ok,
              0U,
              encodeProcessFederationReceiveInteractionResult(
                  ProcessFederationReceiveInteractionResult{
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::move(event)})})) {
        return internalError(request);
      }
    } else {
      std::scoped_lock lock(mutex_);
      auto const state = sessions_.find(&session);
      if (state == sessions_.end()) {
        return internalError(request);
      }
      state->second.attributeTransportationTypeQueryEvents.push_back(
          std::move(event));
    }
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationAttributeTransportationTypeQueryResult(
      ProcessFederationAttributeTransportationTypeQueryResult{
              plan.status}));
}

TransportServiceMessage
ProcessFederationService::handleRequestInteractionTransportationTypeChange(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const changeRequest =
      decodeProcessFederationRequestInteractionTransportationTypeChangeRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != changeRequest.federationName ||
        state->second.federateId != changeRequest.requestingFederateId ||
        !registry_.memberById(
            changeRequest.federationName, changeRequest.requestingFederateId)) {
      return rejected(request);
    }
  }
  auto const transportationName = registry_.transportationTypeNameFor(
      changeRequest.federationName,
      changeRequest.transportationTypeHandle);
  auto plan = transportationName
                  ? registry_.planInteractionTransportationTypeChange(
                        changeRequest.federationName,
                        changeRequest.requestingFederateId,
                        changeRequest.interactionClassHandle,
                        *transportationName)
                  : InteractionTransportationTypeChangePlan{
                        InteractionTransportationTypeChangeStatus::
                            invalid_transportation_type};
  if (plan.status == InteractionTransportationTypeChangeStatus::applied) {
    ProcessFederationInteractionTransportationTypeChangeEvent event{
        changeRequest.requestingFederateId,
        plan.interactionClassHandle,
        std::move(plan.transportationName)};
    if (options_.pushReceiveOrderEvents) {
      if (!session.send(TransportServiceMessage{
              TransportServiceMessageKind::event,
              TransportServiceOperation::receive_interaction,
              TransportServiceStatus::ok,
              0U,
              encodeProcessFederationReceiveInteractionResult(
                  ProcessFederationReceiveInteractionResult{
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::move(event),
                      std::nullopt})})) {
        return internalError(request);
      }
    } else {
      std::scoped_lock lock(mutex_);
      auto const state = sessions_.find(&session);
      if (state == sessions_.end()) {
        registry_.cancelInteractionTransportationTypeChange(
            changeRequest.federationName,
            changeRequest.requestingFederateId,
            plan.interactionClassHandle);
        return internalError(request);
      }
      state->second.interactionTransportationTypeChangeEvents.push_back(
          std::move(event));
    }
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationInteractionTransportationTypeChangeResult(
          ProcessFederationInteractionTransportationTypeChangeResult{
              plan.status}));
}

TransportServiceMessage
ProcessFederationService::handleQueryInteractionTransportationType(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const queryRequest =
      decodeProcessFederationQueryInteractionTransportationTypeRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != queryRequest.federationName ||
        state->second.federateId != queryRequest.requestingFederateId ||
        !registry_.memberById(
            queryRequest.federationName, queryRequest.requestingFederateId)) {
      return rejected(request);
    }
  }
  auto const plan = registry_.planInteractionTransportationTypeQuery(
      queryRequest.federationName,
      queryRequest.requestingFederateId,
      queryRequest.queriedFederateId,
      queryRequest.interactionClassHandle);
  if (plan.status == InteractionTransportationTypeQueryStatus::applied) {
    ProcessFederationInteractionTransportationTypeQueryEvent event{
        queryRequest.requestingFederateId,
        plan.queriedFederateId,
        plan.interactionClassHandle,
        plan.transportationName};
    if (options_.pushReceiveOrderEvents) {
      if (!session.send(TransportServiceMessage{
              TransportServiceMessageKind::event,
              TransportServiceOperation::receive_interaction,
              TransportServiceStatus::ok,
              0U,
              encodeProcessFederationReceiveInteractionResult(
                  ProcessFederationReceiveInteractionResult{
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::move(event)})})) {
        return internalError(request);
      }
    } else {
      std::scoped_lock lock(mutex_);
      auto const state = sessions_.find(&session);
      if (state == sessions_.end()) {
        return internalError(request);
      }
      state->second.interactionTransportationTypeQueryEvents.push_back(
          std::move(event));
    }
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationInteractionTransportationTypeQueryResult(
          ProcessFederationInteractionTransportationTypeQueryResult{
              plan.status}));
}

TransportServiceMessage
ProcessFederationService::handleInteractionClassRegionalSubscription(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const subscriptionRequest =
      decodeProcessFederationInteractionClassRegionalSubscriptionRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != subscriptionRequest.federationName ||
        state->second.federateId != subscriptionRequest.federateId ||
        !registry_.memberById(
            subscriptionRequest.federationName,
            subscriptionRequest.federateId)) {
      return rejected(request);
    }
  }

  auto const result =
      request.operation ==
              TransportServiceOperation::subscribe_interaction_class_with_regions
          ? registry_.setInteractionClassRegionalSubscription(
                subscriptionRequest.federationName,
                subscriptionRequest.federateId,
                subscriptionRequest.interactionClassHandle,
                subscriptionRequest.regionHandles,
                subscriptionRequest.active)
          : request.operation ==
                    TransportServiceOperation::unsubscribe_interaction_class_with_regions
                ? registry_.removeInteractionClassRegionalSubscription(
                      subscriptionRequest.federationName,
                      subscriptionRequest.federateId,
                      subscriptionRequest.interactionClassHandle,
                      subscriptionRequest.regionHandles)
                : RegionalInteractionClassDeclarationStatus::inconsistent_catalog;
  if (result != RegionalInteractionClassDeclarationStatus::applied) {
    return rejected(request);
  }
  return responseFor(request, TransportServiceStatus::ok);
}

TransportServiceMessage
ProcessFederationService::handleObjectClassDirectedInteractionDeclaration(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const declarationRequest =
      decodeProcessFederationObjectClassDirectedInteractionDeclarationRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != declarationRequest.federationName ||
        state->second.federateId != declarationRequest.federateId ||
        !registry_.memberById(
            declarationRequest.federationName, declarationRequest.federateId)) {
      return rejected(request);
    }
  }

  std::optional<std::set<std::uint64_t>> interactionClassHandles;
  if (declarationRequest.interactionClassHandles) {
    interactionClassHandles.emplace(
        declarationRequest.interactionClassHandles->begin(),
        declarationRequest.interactionClassHandles->end());
  }

  DirectedInteractionDeclarationStatus result =
      DirectedInteractionDeclarationStatus::inconsistent_catalog;
  switch (request.operation) {
    case TransportServiceOperation::publish_object_class_directed_interactions:
      result = registry_.publishObjectClassDirectedInteractions(
          declarationRequest.federationName,
          declarationRequest.federateId,
          declarationRequest.objectClassHandle,
          interactionClassHandles.value_or(std::set<std::uint64_t>{}));
      break;
    case TransportServiceOperation::unpublish_object_class_directed_interactions:
      result = registry_.unpublishObjectClassDirectedInteractions(
          declarationRequest.federationName,
          declarationRequest.federateId,
          declarationRequest.objectClassHandle,
          interactionClassHandles);
      break;
    case TransportServiceOperation::subscribe_object_class_directed_interactions:
      result = registry_.subscribeObjectClassDirectedInteractions(
          declarationRequest.federationName,
          declarationRequest.federateId,
          declarationRequest.objectClassHandle,
          interactionClassHandles.value_or(std::set<std::uint64_t>{}),
          declarationRequest.universally);
      break;
    case TransportServiceOperation::unsubscribe_object_class_directed_interactions:
      result = registry_.unsubscribeObjectClassDirectedInteractions(
          declarationRequest.federationName,
          declarationRequest.federateId,
          declarationRequest.objectClassHandle,
          interactionClassHandles);
      break;
    default:
      return invalid(request);
  }
  if (result != DirectedInteractionDeclarationStatus::applied) {
    return rejected(request);
  }
  return responseFor(request, TransportServiceStatus::ok);
}

TransportServiceMessage
ProcessFederationService::handleObjectClassAttributeDeclaration(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const declarationRequest =
      decodeProcessFederationObjectClassAttributeDeclarationRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != declarationRequest.federationName ||
        state->second.federateId != declarationRequest.federateId ||
        !registry_.memberById(
            declarationRequest.federationName, declarationRequest.federateId)) {
      return rejected(request);
    }
  }

  std::set<std::uint64_t> attributeHandles(
      declarationRequest.attributeHandles.begin(),
      declarationRequest.attributeHandles.end());
  auto const result = registry_.setObjectClassAttributePublication(
      declarationRequest.federationName,
      declarationRequest.federateId,
      declarationRequest.objectClassHandle,
      attributeHandles,
      true);
  if (result != ObjectClassAttributeDeclarationStatus::applied) {
    return rejected(request);
  }
  return responseFor(request, TransportServiceStatus::ok);
}

TransportServiceMessage
ProcessFederationService::handleObjectClassAttributeSubscription(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const subscriptionRequest =
      decodeProcessFederationObjectClassAttributeSubscriptionRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != subscriptionRequest.federationName ||
        state->second.federateId != subscriptionRequest.federateId ||
        !registry_.memberById(
            subscriptionRequest.federationName,
            subscriptionRequest.federateId)) {
      return rejected(request);
    }
  }

  std::set<std::uint64_t> attributeHandles(
      subscriptionRequest.attributeHandles.begin(),
      subscriptionRequest.attributeHandles.end());
  if (request.operation == TransportServiceOperation::unsubscribe_object_class_attributes &&
      attributeHandles.empty()) {
    auto const declaration = registry_.objectClassAttributeDeclarationFor(
        subscriptionRequest.federationName,
        subscriptionRequest.federateId,
        subscriptionRequest.objectClassHandle);
    if (!declaration) {
      return rejected(request);
    }
    for (auto const& [attributeHandle, active] : declaration->subscribedAttributes) {
      static_cast<void>(active);
      attributeHandles.insert(attributeHandle);
    }
  }

  auto const result = registry_.setObjectClassAttributeSubscriptionWithScopeChanges(
      subscriptionRequest.federationName,
      subscriptionRequest.federateId,
      subscriptionRequest.objectClassHandle,
      attributeHandles,
      request.operation == TransportServiceOperation::subscribe_object_class_attributes
          ? std::optional<bool>{subscriptionRequest.active}
          : std::nullopt,
      subscriptionRequest.updateRateDesignator);
  if (result.status != ObjectClassAttributeDeclarationStatus::applied) {
    return rejected(request);
  }
  if (request.operation ==
      TransportServiceOperation::subscribe_object_class_attributes) {
    // A subscription can make already-registered instances newly visible.
    // Keep that discovery projection on the same process event boundary as
    // registration rather than requiring a fixture to mutate the registry.
    // RTI-owned MOM objects use a separate registry ledger because they are
    // not federate-created instances, but the public subscription still has
    // to expose both projections through the same official discovery route.
    auto discoveries = registry_.planObjectInstanceDiscoveriesForFederate(
        subscriptionRequest.federationName,
        subscriptionRequest.federateId);
    auto momDiscoveries = registry_.planJoinedFederateMomObjectDiscoveriesForFederate(
        subscriptionRequest.federationName,
        subscriptionRequest.federateId);
    discoveries.insert(
        discoveries.end(),
        std::make_move_iterator(momDiscoveries.begin()),
        std::make_move_iterator(momDiscoveries.end()));
    if (!enqueueObjectInstanceDiscoveries(
            subscriptionRequest.federationName,
            std::move(discoveries))) {
      return internalError(request);
    }
  }
  if (!enqueueObjectInstanceScopeChanges(
          subscriptionRequest.federationName, std::move(result.recipients)) ||
      !enqueueAttributeRelevanceAdvisories(
          subscriptionRequest.federationName,
          std::move(result.attributeRelevanceAdvisories))) {
    return internalError(request);
  }
  return responseFor(request, TransportServiceStatus::ok);
}

TransportServiceMessage
ProcessFederationService::handleObjectClassAttributeRegionalSubscription(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const subscriptionRequest =
      decodeProcessFederationObjectClassAttributeRegionalSubscriptionRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != subscriptionRequest.federationName ||
        state->second.federateId != subscriptionRequest.federateId ||
        !registry_.memberById(
            subscriptionRequest.federationName,
            subscriptionRequest.federateId)) {
      return rejected(request);
    }
  }

  RegionalObjectClassAttributeSubscriptionScopePlan result;
  if (request.operation ==
      TransportServiceOperation::subscribe_object_class_attributes_with_regions) {
    result = registry_.setObjectClassAttributeRegionalSubscriptionWithScopeChanges(
        subscriptionRequest.federationName,
        subscriptionRequest.federateId,
        subscriptionRequest.objectClassHandle,
        subscriptionRequest.attributesAndRegions,
        subscriptionRequest.active,
        subscriptionRequest.updateRateDesignator);
  } else if (request.operation ==
             TransportServiceOperation::unsubscribe_object_class_attributes_with_regions) {
    result = registry_.removeObjectClassAttributeRegionalSubscriptionWithScopeChanges(
        subscriptionRequest.federationName,
        subscriptionRequest.federateId,
        subscriptionRequest.objectClassHandle,
        subscriptionRequest.attributesAndRegions);
  } else {
    return invalid(request);
  }
  if (result.status !=
      RegionalObjectClassAttributeDeclarationStatus::applied) {
    return rejected(request);
  }

  if (request.operation ==
      TransportServiceOperation::subscribe_object_class_attributes_with_regions) {
    // A regional subscription can make already-registered instances newly
    // visible. Discovery remains ordered before any scope transition. Include
    // RTI-owned MOM objects here as well; their immutable HLAfederate point is
    // evaluated by the registry's MOM discovery planner.
    auto discoveries = registry_.planObjectInstanceDiscoveriesForFederate(
        subscriptionRequest.federationName,
        subscriptionRequest.federateId);
    auto momDiscoveries = registry_.planJoinedFederateMomObjectDiscoveriesForFederate(
        subscriptionRequest.federationName,
        subscriptionRequest.federateId);
    discoveries.insert(
        discoveries.end(),
        std::make_move_iterator(momDiscoveries.begin()),
        std::make_move_iterator(momDiscoveries.end()));
    if (!enqueueObjectInstanceDiscoveries(
            subscriptionRequest.federationName,
            std::move(discoveries))) {
      return internalError(request);
    }
  }
  if (!enqueueObjectInstanceScopeChanges(
          subscriptionRequest.federationName, std::move(result.recipients))) {
    return internalError(request);
  }
  if (!enqueueAttributeRelevanceAdvisories(
          subscriptionRequest.federationName,
          std::move(result.attributeRelevanceAdvisories))) {
    return internalError(request);
  }
  return responseFor(request, TransportServiceStatus::ok);
}

TransportServiceMessage ProcessFederationService::handleRegisterObjectInstance(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const registrationRequest =
      decodeProcessFederationRegisterObjectInstanceRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != registrationRequest.federationName ||
        state->second.federateId != registrationRequest.federateId ||
        !registry_.memberById(
            registrationRequest.federationName, registrationRequest.federateId)) {
      return rejected(request);
    }
  }

  std::wstring const* requestedName = nullptr;
  if (registrationRequest.requestedObjectInstanceName.has_value()) {
    requestedName = &*registrationRequest.requestedObjectInstanceName;
  }
  auto const result = registry_.registerObjectInstance(
      registrationRequest.federationName,
      registrationRequest.federateId,
      registrationRequest.objectClassHandle,
      nullptr,
      requestedName);
  if (result.status != ObjectInstanceRegistrationStatus::applied) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationRegisterObjectInstanceResult(
            ProcessFederationRegisterObjectInstanceResult{
                0U,
                {},
                result.status}));
  }
  auto discoveries = registry_.planObjectInstanceDiscoveriesForInstance(
      registrationRequest.federationName,
      result.objectInstanceHandle);
  if (!enqueueObjectInstanceDiscoveries(
          registrationRequest.federationName,
          std::move(discoveries))) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationRegisterObjectInstanceResult(
          ProcessFederationRegisterObjectInstanceResult{
              result.objectInstanceHandle,
              result.objectInstanceName,
              result.status}));
}

TransportServiceMessage ProcessFederationService::handleReserveObjectInstanceName(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const reservationRequest =
      decodeProcessFederationReserveObjectInstanceNameRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != reservationRequest.federationName ||
        state->second.federateId != reservationRequest.federateId ||
        !registry_.memberById(
            reservationRequest.federationName, reservationRequest.federateId)) {
      return rejected(request);
    }
  }

  auto const result = registry_.reserveObjectInstanceName(
      reservationRequest.federationName,
      reservationRequest.federateId,
      reservationRequest.objectInstanceName);
  // Reservation callbacks remain local to the public process adapter for this
  // bounded slice. The process service owns the reservation ledger and returns
  // the accepted outcome; the client queues the official callback through its
  // existing dispatcher without transporting a registry callback closure.
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationReserveObjectInstanceNameResult(
          ProcessFederationReserveObjectInstanceNameResult{
              result.succeeded,
              result.objectInstanceName,
              result.status}));
}

TransportServiceMessage ProcessFederationService::handleReleaseObjectInstanceName(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const releaseRequest =
      decodeProcessFederationReserveObjectInstanceNameRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != releaseRequest.federationName ||
        state->second.federateId != releaseRequest.federateId ||
        !registry_.memberById(
            releaseRequest.federationName, releaseRequest.federateId)) {
      return rejected(request);
    }
  }

  auto const status = registry_.releaseObjectInstanceName(
      releaseRequest.federationName,
      releaseRequest.federateId,
      releaseRequest.objectInstanceName);
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationObjectInstanceNameReleaseResult(
          ProcessFederationObjectInstanceNameReleaseResult{status}));
}

TransportServiceMessage
ProcessFederationService::handleReserveMultipleObjectInstanceNames(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const reservationRequest =
      decodeProcessFederationReserveMultipleObjectInstanceNamesRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != reservationRequest.federationName ||
        state->second.federateId != reservationRequest.federateId ||
        !registry_.memberById(
            reservationRequest.federationName, reservationRequest.federateId)) {
      return rejected(request);
    }
  }

  auto const result = registry_.reserveMultipleObjectInstanceNames(
      reservationRequest.federationName,
      reservationRequest.federateId,
      reservationRequest.objectInstanceNames);
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationReserveMultipleObjectInstanceNamesResult(
          ProcessFederationReserveMultipleObjectInstanceNamesResult{
              result.status, result.succeededNames, result.failedNames}));
}

TransportServiceMessage
ProcessFederationService::handleReleaseMultipleObjectInstanceNames(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const releaseRequest =
      decodeProcessFederationReserveMultipleObjectInstanceNamesRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != releaseRequest.federationName ||
        state->second.federateId != releaseRequest.federateId ||
        !registry_.memberById(
            releaseRequest.federationName, releaseRequest.federateId)) {
      return rejected(request);
    }
  }

  auto const status = registry_.releaseMultipleObjectInstanceNames(
      releaseRequest.federationName,
      releaseRequest.federateId,
      releaseRequest.objectInstanceNames);
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationReleaseMultipleObjectInstanceNamesResult(
          ProcessFederationReleaseMultipleObjectInstanceNamesResult{status}));
}

TransportServiceMessage ProcessFederationService::handleLocalDeleteObjectInstance(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const deletionRequest =
      decodeProcessFederationLocalDeleteObjectInstanceRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != deletionRequest.federationName ||
        state->second.federateId != deletionRequest.federateId ||
        !registry_.memberById(
            deletionRequest.federationName, deletionRequest.federateId)) {
      return rejected(request);
    }
  }

  auto const status = registry_.localDeleteObjectInstance(
      deletionRequest.federationName,
      deletionRequest.federateId,
      deletionRequest.objectInstanceHandle);
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationLocalDeleteObjectInstanceResult(
          ProcessFederationLocalDeleteObjectInstanceResult{status}));
}

TransportServiceMessage ProcessFederationService::handleDeleteObjectInstance(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const deletionRequest =
      decodeProcessFederationDeleteObjectInstanceRequest(request.payload);
  std::shared_ptr<FederateTimeState> producingTimeState;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != deletionRequest.federationName ||
        state->second.federateId != deletionRequest.federateId ||
        !registry_.memberById(
            deletionRequest.federationName, deletionRequest.federateId)) {
      return rejected(request);
    }
    producingTimeState = state->second.timeState;
  }

  if (deletionRequest.timestamp) {
    // The process endpoint carries the official logical-time value across the
    // boundary and retains the registry's timestamped deletion ledger so the
    // callback carries the same immutable recipient snapshot and message
    // identity as the embedded path.  The public retraction designator is
    // projected only when the producer is time-regulating and the effective
    // HLAprivilegeToDeleteObject order is timestamp-ordered.
    auto const timestamp = decodeProcessLogicalTime(
        *deletionRequest.timestamp,
        federationDefinition_->logicalTimeImplementationName);
    auto const plan = registry_.planTsoObjectInstanceDeletion(
        deletionRequest.federationName,
        deletionRequest.federateId,
        deletionRequest.objectInstanceHandle);
    if (plan.status != ObjectInstanceDeletionStatus::applied) {
      return responseFor(
          request,
          TransportServiceStatus::ok,
          encodeProcessFederationDeleteObjectInstanceResult(
              ProcessFederationDeleteObjectInstanceResult{plan.status, 0U, 0U}));
    }
    auto const producingTimeSnapshot = producingTimeState
        ? producingTimeState->snapshot()
        : FederateTimeSnapshot{};
    bool const provideRetraction =
        producingTimeSnapshot.timeRegulating &&
        plan.preferredOrderType == rti1516_2025::TIMESTAMP;
    bool const queueTimestampedDeletion = provideRetraction;
    std::vector<std::uint64_t> queuedRecipientFederateIds;
    if (queueTimestampedDeletion && !plan.recipients.empty()) {
      auto const execution = registry_.timeSnapshotFor(
          deletionRequest.federationName);
      if (!execution) {
        return internalError(request);
      }
      std::set<std::uint64_t> timeConstrainedRecipients;
      for (auto const& federate : execution->federates) {
        if (federate.time.timeConstrained) {
          timeConstrainedRecipients.insert(federate.membership.id);
        }
      }
      queuedRecipientFederateIds.reserve(plan.recipients.size());
      for (auto const& recipient : plan.recipients) {
        if (timeConstrainedRecipients.contains(recipient.receivingFederateId)) {
          queuedRecipientFederateIds.push_back(recipient.receivingFederateId);
        }
      }
    }

    TsoObjectDeletionMessage message;
    message.producingFederateId = deletionRequest.federateId;
    message.objectInstanceHandle = deletionRequest.objectInstanceHandle;
    if (!deletionRequest.userSuppliedTag.empty()) {
      message.userSuppliedTag.setData(
          deletionRequest.userSuppliedTag.data(),
          deletionRequest.userSuppliedTag.size());
    }
    message.timestamp = timestamp;
    message.sentOrderType = plan.preferredOrderType;
    message.recipients.reserve(plan.recipients.size());
    for (auto const& recipient : plan.recipients) {
      message.recipients.push_back({
          recipient.receivingFederateId,
          recipient.objectInstanceHandle,
          recipient.callbackRoute,
          {}});
    }

    auto const enqueueResult = registry_.enqueueTsoObjectDeletion(
        deletionRequest.federationName,
        deletionRequest.federateId,
        deletionRequest.objectInstanceHandle,
        std::move(message),
        queuedRecipientFederateIds);
    if (enqueueResult.status != FederationTsoRegistryStatus::applied ||
        enqueueResult.queueStatus != TsoMessageQueueStatus::applied ||
        enqueueResult.messageId == 0U) {
      return rejected(request);
    }
    if (enqueueResult.recipients.size() >
        std::numeric_limits<std::uint32_t>::max()) {
      return internalError(request);
    }

    std::vector<ObjectInstanceRemovalRecipient> removals;
    removals.reserve(enqueueResult.recipients.size());
    std::set<std::uint64_t> queuedRecipients(
        queuedRecipientFederateIds.begin(), queuedRecipientFederateIds.end());
    for (auto const& recipient : enqueueResult.recipients) {
      if (queuedRecipients.contains(recipient.receivingFederateId)) {
        continue;
      }
      removals.push_back({
          recipient.receivingFederateId,
          recipient.objectInstanceHandle,
          recipient.callbackRoute,
          recipient.serviceReportRoute,
          false,
          plan.preferredOrderType,
          rti1516_2025::RECEIVE});
    }
    if (!enqueueObjectInstanceRemovals(
            deletionRequest.federationName,
            std::move(removals),
            deletionRequest.userSuppliedTag,
            deletionRequest.timestamp,
            enqueueResult.messageId,
            provideRetraction)) {
      return internalError(request);
    }
    {
      std::scoped_lock lock(mutex_);
      processTsoMessageProducers_.emplace(
          enqueueResult.messageId,
          deletionRequest.federateId);
    }
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationDeleteObjectInstanceResult(
            ProcessFederationDeleteObjectInstanceResult{
                ObjectInstanceDeletionStatus::applied,
                static_cast<std::uint32_t>(enqueueResult.recipients.size()),
                provideRetraction ? enqueueResult.messageId : 0U}));
  }

  auto deletion = registry_.deleteObjectInstance(
      deletionRequest.federationName,
      deletionRequest.federateId,
      deletionRequest.objectInstanceHandle);
  if (deletion.status != ObjectInstanceDeletionStatus::applied) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationDeleteObjectInstanceResult(
            ProcessFederationDeleteObjectInstanceResult{deletion.status, 0U}));
  }
  auto const recipientCount = deletion.recipients.size();
  if (recipientCount > std::numeric_limits<std::uint32_t>::max()) {
    return internalError(request);
  }
  if (!enqueueObjectInstanceRemovals(
          deletionRequest.federationName,
          std::move(deletion.recipients),
          deletionRequest.userSuppliedTag)) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationDeleteObjectInstanceResult(
          ProcessFederationDeleteObjectInstanceResult{
              deletion.status,
              static_cast<std::uint32_t>(recipientCount)}));
}

TransportServiceMessage ProcessFederationService::handleGetDimensionHandle(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const lookupRequest =
      decodeProcessFederationGetDimensionHandleRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != lookupRequest.federationName ||
        state->second.federateId != lookupRequest.federateId ||
        !registry_.memberById(
            lookupRequest.federationName, lookupRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const handle = registry_.dimensionHandleFor(
      lookupRequest.federationName, lookupRequest.dimensionName);
  if (!handle) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationHandleResult(ProcessFederationHandleResult{*handle}));
}

TransportServiceMessage ProcessFederationService::handleGetDimensionName(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const lookupRequest =
      decodeProcessFederationDimensionRequest(request.payload);
  std::optional<std::string> encodedName;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != lookupRequest.federationName ||
        state->second.federateId != lookupRequest.federateId ||
        !registry_.memberById(
            lookupRequest.federationName, lookupRequest.federateId)) {
      return rejected(request);
    }
    encodedName = registry_.dimensionNameFor(
        lookupRequest.federationName, lookupRequest.dimensionHandle);
  }
  if (!encodedName) {
    return rejected(request);
  }
  auto const decodedName = wideFromUtf8(*encodedName);
  if (!decodedName) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationStringResult(
          ProcessFederationStringResult{*decodedName}));
}

TransportServiceMessage
ProcessFederationService::handleGetTransportationTypeHandle(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const lookupRequest =
      decodeProcessFederationGetTransportationTypeHandleRequest(
          request.payload);
  std::optional<std::uint64_t> handle;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != lookupRequest.federationName ||
        state->second.federateId != lookupRequest.federateId ||
        !registry_.memberById(
            lookupRequest.federationName, lookupRequest.federateId)) {
      return rejected(request);
    }
    handle = registry_.transportationTypeHandleFor(
        lookupRequest.federationName, lookupRequest.transportationTypeName);
  }
  if (!handle) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationHandleResult(ProcessFederationHandleResult{*handle}));
}

TransportServiceMessage
ProcessFederationService::handleGetTransportationTypeName(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const lookupRequest =
      decodeProcessFederationTransportationTypeRequest(request.payload);
  std::optional<std::string> encodedName;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != lookupRequest.federationName ||
        state->second.federateId != lookupRequest.federateId ||
        !registry_.memberById(
            lookupRequest.federationName, lookupRequest.federateId)) {
      return rejected(request);
    }
    encodedName = registry_.transportationTypeNameFor(
        lookupRequest.federationName,
        lookupRequest.transportationTypeHandle);
  }
  if (!encodedName) {
    return rejected(request);
  }
  auto const decodedName = wideFromUtf8(*encodedName);
  if (!decodedName) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationStringResult(
          ProcessFederationStringResult{*decodedName}));
}

TransportServiceMessage ProcessFederationService::handleGetDimensionUpperBound(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const dimensionRequest =
      decodeProcessFederationDimensionRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != dimensionRequest.federationName ||
        state->second.federateId != dimensionRequest.federateId ||
        !registry_.memberById(
            dimensionRequest.federationName, dimensionRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const upperBound = registry_.dimensionUpperBoundFor(
      dimensionRequest.federationName, dimensionRequest.dimensionHandle);
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationDimensionUpperBoundResult(
          ProcessFederationDimensionUpperBoundResult{
              upperBound.has_value(), upperBound.value_or(0UL)}));
}

TransportServiceMessage
ProcessFederationService::handleGetAvailableDimensionsForObjectClass(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const classRequest =
      decodeProcessFederationClassHandleRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != classRequest.federationName ||
        state->second.federateId != classRequest.federateId ||
        !registry_.memberById(
            classRequest.federationName, classRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const handles = registry_.availableDimensionsForObjectClass(
      classRequest.federationName, classRequest.classHandle);
  std::vector<std::uint64_t> encodedHandles;
  if (handles) {
    encodedHandles.assign(handles->begin(), handles->end());
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationAvailableDimensionsResult(
          ProcessFederationAvailableDimensionsResult{
              handles.has_value(), std::move(encodedHandles)}));
}

TransportServiceMessage
ProcessFederationService::handleGetAvailableDimensionsForInteractionClass(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const classRequest =
      decodeProcessFederationClassHandleRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != classRequest.federationName ||
        state->second.federateId != classRequest.federateId ||
        !registry_.memberById(
            classRequest.federationName, classRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const handles = registry_.availableDimensionsForInteractionClass(
      classRequest.federationName, classRequest.classHandle);
  std::vector<std::uint64_t> encodedHandles;
  if (handles) {
    encodedHandles.assign(handles->begin(), handles->end());
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationAvailableDimensionsResult(
          ProcessFederationAvailableDimensionsResult{
              handles.has_value(), std::move(encodedHandles)}));
}

TransportServiceMessage ProcessFederationService::handleCreateRegion(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const createRequest =
      decodeProcessFederationCreateRegionRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != createRequest.federationName ||
        state->second.federateId != createRequest.federateId ||
        !registry_.memberById(
            createRequest.federationName, createRequest.federateId)) {
      return rejected(request);
    }
  }
  std::set<std::uint64_t> dimensions(
      createRequest.dimensionHandles.begin(), createRequest.dimensionHandles.end());
  auto const result = registry_.createRegion(
      createRequest.federationName, createRequest.federateId, dimensions);
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationCreateRegionResult(
          ProcessFederationCreateRegionResult{result.status, result.regionHandle}));
}

TransportServiceMessage ProcessFederationService::handleCommitRegionModifications(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const commitRequest =
      decodeProcessFederationRegionSetRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != commitRequest.federationName ||
        state->second.federateId != commitRequest.federateId ||
        !registry_.memberById(
            commitRequest.federationName, commitRequest.federateId)) {
      return rejected(request);
    }
  }
  std::set<std::uint64_t> regions(
      commitRequest.regionHandles.begin(), commitRequest.regionHandles.end());
  auto plan = registry_.commitRegionModificationsWithScopeChanges(
      commitRequest.federationName, commitRequest.federateId, regions);
  if (plan.status == RegionServiceStatus::applied &&
      !enqueueObjectInstanceDiscoveries(
          commitRequest.federationName, std::move(plan.discoveries))) {
    return internalError(request);
  }
  if (plan.status == RegionServiceStatus::applied &&
      !enqueueObjectInstanceScopeChanges(
          commitRequest.federationName, std::move(plan.recipients))) {
    return internalError(request);
  }
  if (plan.status == RegionServiceStatus::applied &&
      !enqueueAttributeRelevanceAdvisories(
          commitRequest.federationName,
          std::move(plan.attributeRelevanceAdvisories))) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationRegionStatusResult(
          ProcessFederationRegionStatusResult{plan.status}));
}

TransportServiceMessage ProcessFederationService::handleDeleteRegion(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const regionRequest = decodeProcessFederationRegionRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != regionRequest.federationName ||
        state->second.federateId != regionRequest.federateId ||
        !registry_.memberById(
            regionRequest.federationName, regionRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const status = registry_.deleteRegion(
      regionRequest.federationName,
      regionRequest.federateId,
      regionRequest.regionHandle);
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationRegionStatusResult(
          ProcessFederationRegionStatusResult{status}));
}

TransportServiceMessage ProcessFederationService::handleGetDimensionHandleSet(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const regionRequest = decodeProcessFederationRegionRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != regionRequest.federationName ||
        state->second.federateId != regionRequest.federateId ||
        !registry_.memberById(
            regionRequest.federationName, regionRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const result = registry_.dimensionHandleSetForRegion(
      regionRequest.federationName,
      regionRequest.federateId,
      regionRequest.regionHandle);
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationDimensionSetResult(
          ProcessFederationDimensionSetResult{
              result.status,
              std::vector<std::uint64_t>(
                  result.dimensionHandles.begin(), result.dimensionHandles.end())}));
}

TransportServiceMessage ProcessFederationService::handleGetRangeBounds(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const boundsRequest =
      decodeProcessFederationGetRangeBoundsRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != boundsRequest.federationName ||
        state->second.federateId != boundsRequest.federateId ||
        !registry_.memberById(
            boundsRequest.federationName, boundsRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const result = registry_.rangeBoundsForRegion(
      boundsRequest.federationName,
      boundsRequest.federateId,
      boundsRequest.regionHandle,
      boundsRequest.dimensionHandle);
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationRangeBoundsResult(
          ProcessFederationRangeBoundsResult{
              result.status, result.range.lowerBound, result.range.upperBound}));
}

TransportServiceMessage ProcessFederationService::handleSetRangeBounds(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const boundsRequest =
      decodeProcessFederationSetRangeBoundsRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != boundsRequest.federationName ||
        state->second.federateId != boundsRequest.federateId ||
        !registry_.memberById(
            boundsRequest.federationName, boundsRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const status = registry_.setRangeBounds(
      boundsRequest.federationName,
      boundsRequest.federateId,
      boundsRequest.regionHandle,
      boundsRequest.dimensionHandle,
      RegionRangeBounds{boundsRequest.lowerBound, boundsRequest.upperBound});
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationRegionStatusResult(
          ProcessFederationRegionStatusResult{status}));
}

TransportServiceMessage
ProcessFederationService::handleGetAttributeScopeAdvisorySwitch(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const switchRequest =
      decodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != switchRequest.federationName ||
        state->second.federateId != switchRequest.federateId ||
        !registry_.memberById(
            switchRequest.federationName, switchRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const switchValue = registry_.attributeScopeAdvisorySwitchFor(
      switchRequest.federationName, switchRequest.federateId);
  if (!switchValue) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationBooleanResult(
          ProcessFederationBooleanResult{*switchValue}));
}

TransportServiceMessage
ProcessFederationService::handleSetAttributeScopeAdvisorySwitch(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const switchRequest =
      decodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != switchRequest.federationName ||
        state->second.federateId != switchRequest.federateId ||
        !registry_.memberById(
            switchRequest.federationName, switchRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const status = registry_.setAttributeScopeAdvisorySwitch(
      switchRequest.federationName,
      switchRequest.federateId,
      switchRequest.switchValue);
  if (status != FederationRegistryStatus::applied) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationBooleanResult(
          ProcessFederationBooleanResult{switchRequest.switchValue}));
}

TransportServiceMessage
ProcessFederationService::handleGetAttributeRelevanceAdvisorySwitch(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const switchRequest =
      decodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != switchRequest.federationName ||
        state->second.federateId != switchRequest.federateId ||
        !registry_.memberById(
            switchRequest.federationName, switchRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const switchValue = registry_.attributeRelevanceAdvisorySwitchFor(
      switchRequest.federationName, switchRequest.federateId);
  if (!switchValue) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationBooleanResult(
          ProcessFederationBooleanResult{*switchValue}));
}

TransportServiceMessage
ProcessFederationService::handleSetAttributeRelevanceAdvisorySwitch(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const switchRequest =
      decodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != switchRequest.federationName ||
        state->second.federateId != switchRequest.federateId ||
        !registry_.memberById(
            switchRequest.federationName, switchRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const status = registry_.setAttributeRelevanceAdvisorySwitch(
      switchRequest.federationName,
      switchRequest.federateId,
      switchRequest.switchValue);
  if (status != FederationRegistryStatus::applied) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationBooleanResult(
          ProcessFederationBooleanResult{switchRequest.switchValue}));
}

TransportServiceMessage
ProcessFederationService::handleGetConveyRegionDesignatorSetsSwitch(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const switchRequest =
      decodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != switchRequest.federationName ||
        state->second.federateId != switchRequest.federateId ||
        !registry_.memberById(
            switchRequest.federationName, switchRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const switchValue = registry_.conveyRegionDesignatorSetsSwitchFor(
      switchRequest.federationName, switchRequest.federateId);
  if (!switchValue) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationBooleanResult(
          ProcessFederationBooleanResult{*switchValue}));
}

TransportServiceMessage
ProcessFederationService::handleSetConveyRegionDesignatorSetsSwitch(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const switchRequest =
      decodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != switchRequest.federationName ||
        state->second.federateId != switchRequest.federateId ||
        !registry_.memberById(
            switchRequest.federationName, switchRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const status = registry_.setConveyRegionDesignatorSetsSwitch(
      switchRequest.federationName,
      switchRequest.federateId,
      switchRequest.switchValue);
  if (status != FederationRegistryStatus::applied) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationBooleanResult(
          ProcessFederationBooleanResult{switchRequest.switchValue}));
}

TransportServiceMessage
ProcessFederationService::handleGetAutomaticResignDirective(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const directiveRequest =
      decodeProcessFederationResignRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != directiveRequest.federationName ||
        state->second.federateId != directiveRequest.federateId ||
        !registry_.memberById(
            directiveRequest.federationName, directiveRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const action = registry_.automaticResignActionFor(
      directiveRequest.federationName, directiveRequest.federateId);
  if (!action) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationResignRequest(
          ProcessFederationResignRequest{
              directiveRequest.federationName,
              directiveRequest.federateId,
              *action}));
}

TransportServiceMessage
ProcessFederationService::handleSetAutomaticResignDirective(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const directiveRequest =
      decodeProcessFederationResignRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != directiveRequest.federationName ||
        state->second.federateId != directiveRequest.federateId ||
        !registry_.memberById(
            directiveRequest.federationName, directiveRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const status = registry_.setAutomaticResignAction(
      directiveRequest.federationName,
      directiveRequest.federateId,
      directiveRequest.resignAction);
  if (status != FederationRegistryStatus::applied) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationBooleanResult(
          ProcessFederationBooleanResult{true}));
}

TransportServiceMessage ProcessFederationService::handleRegisterObjectInstanceWithRegions(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const registrationRequest =
      decodeProcessFederationRegisterObjectInstanceWithRegionsRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != registrationRequest.federationName ||
        state->second.federateId != registrationRequest.federateId ||
        !registry_.memberById(
            registrationRequest.federationName, registrationRequest.federateId)) {
      return rejected(request);
    }
  }
  std::wstring const* requestedName = nullptr;
  if (registrationRequest.requestedObjectInstanceName.has_value()) {
    requestedName = &*registrationRequest.requestedObjectInstanceName;
  }
  auto const result = registry_.registerObjectInstance(
      registrationRequest.federationName,
      registrationRequest.federateId,
      registrationRequest.objectClassHandle,
      &registrationRequest.updateRegionsByAttribute,
      requestedName);
  if (result.status != ObjectInstanceRegistrationStatus::applied) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationRegisterObjectInstanceResult(
            ProcessFederationRegisterObjectInstanceResult{0U, {}, result.status}));
  }
  auto discoveries = registry_.planObjectInstanceDiscoveriesForInstance(
      registrationRequest.federationName, result.objectInstanceHandle);
  if (!enqueueObjectInstanceDiscoveries(
          registrationRequest.federationName, std::move(discoveries))) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationRegisterObjectInstanceResult(
          ProcessFederationRegisterObjectInstanceResult{
              result.objectInstanceHandle,
              result.objectInstanceName,
              result.status}));
}

TransportServiceMessage
ProcessFederationService::handleObjectInstanceRegionAssociation(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const associationRequest =
      decodeProcessFederationObjectInstanceRegionAssociationRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != associationRequest.federationName ||
        state->second.federateId != associationRequest.federateId ||
        !registry_.memberById(
            associationRequest.federationName,
            associationRequest.federateId)) {
      return rejected(request);
    }
  }

  ObjectInstanceRegionAssociationScopePlan result;
  if (request.operation == TransportServiceOperation::associate_regions_for_updates) {
    result = registry_.associateRegionsForUpdatesWithScopeChanges(
        associationRequest.federationName,
        associationRequest.federateId,
        associationRequest.objectInstanceHandle,
        associationRequest.attributesAndRegions);
  } else if (
      request.operation ==
      TransportServiceOperation::unassociate_regions_for_updates) {
    result = registry_.unassociateRegionsForUpdatesWithScopeChanges(
        associationRequest.federationName,
        associationRequest.federateId,
        associationRequest.objectInstanceHandle,
        associationRequest.attributesAndRegions);
  } else {
    return invalid(request);
  }

  if (result.status == ObjectInstanceRegionAssociationStatus::applied) {
    // Discovery is ordered before the corresponding scope transition, and
    // owner-directed relevance advisories follow the same committed plan.
    if (!enqueueObjectInstanceDiscoveries(
            associationRequest.federationName, std::move(result.discoveries)) ||
        !enqueueObjectInstanceScopeChanges(
            associationRequest.federationName, std::move(result.recipients))) {
      return internalError(request);
    }
    if (!enqueueAttributeRelevanceAdvisories(
            associationRequest.federationName,
            std::move(result.attributeRelevanceAdvisories))) {
      return internalError(request);
    }
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationObjectInstanceRegionAssociationResult(
          ProcessFederationObjectInstanceRegionAssociationResult{result.status}));
}

void ProcessFederationService::dispatchTimeAdvanceGrants(
    std::wstring const& federationName,
    std::vector<FederationTimeGrantDispatch> dispatches) {
  // Each dispatch may mutate the shared temporal snapshot, so re-evaluate
  // between rounds instead of assuming that every grant selected from one
  // snapshot remains eligible after the first callback is committed.
  constexpr std::size_t maximumRounds = 1024U;
  std::size_t rounds = 0U;
  while (!dispatches.empty()) {
    if (++rounds > maximumRounds) {
      throw std::runtime_error(
          "The process federation time-grant scheduler did not converge.");
    }
    for (auto& dispatch : dispatches) {
      if (dispatch) {
        dispatch();
      }
    }
    auto const reevaluated = registry_.reevaluateTimeAdvanceGrants(federationName);
    if (reevaluated.status != FederationTimeGrantStatus::applied) {
      return;
    }
    dispatches = reevaluated.dispatches;
  }
}

void ProcessFederationService::dispatchTsoInteractionPayloads(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    rti1516_2025::LogicalTime const& boundary) {
  auto delivery = registry_.beginTsoPayloadDelivery(
      federationName,
      receivingFederateId,
      boundary,
      true);
  if (delivery.status != FederationTsoRegistryStatus::applied ||
      (delivery.deliveryStatus != FederationTsoDeliveryStatus::applied &&
       delivery.deliveryStatus != FederationTsoDeliveryStatus::no_messages)) {
    throw std::runtime_error(
        "The process federation could not begin timestamped interaction delivery.");
  }

  for (auto const& typedDelivery : delivery.deliveries) {
    if (auto const* attribute =
            std::get_if<TsoAttributeUpdateDelivery>(&typedDelivery)) {
      auto const& message = attribute->message;
      if (!message.timestamp) {
        throw std::runtime_error(
            "The process federation encountered a timestamped attribute update without a timestamp.");
      }
      ProcessTransportSession* receivingSession = nullptr;
      {
        std::scoped_lock lock(mutex_);
        auto const session = sessionsByFederateId_.find(receivingFederateId);
        if (session != sessionsByFederateId_.end()) {
          auto const state = sessions_.find(session->second);
          if (state != sessions_.end() && state->second.federationName.has_value() &&
              *state->second.federationName == federationName &&
              state->second.federateId == receivingFederateId) {
            receivingSession = session->second;
          }
        }
      }

      auto completeDelivery = [&] {
        auto const completed = registry_.completeTsoDelivery(
            federationName,
            attribute->queuedMessage);
        if (completed.status != FederationTsoRegistryStatus::applied ||
            (completed.delivery.status != FederationTsoDeliveryStatus::applied &&
             completed.delivery.status !=
                 FederationTsoDeliveryStatus::message_already_completed)) {
          throw std::runtime_error(
              "The process federation could not complete timestamped attribute delivery.");
        }
      };

      if (receivingSession == nullptr) {
        static_cast<void>(registry_.finishTsoRecipientCallbackSuppressed(
            federationName,
            receivingFederateId,
            message.messageId));
        completeDelivery();
        continue;
      }

      std::vector<ProcessFederationAttributeUpdateEvent> events;
      auto const passels = message.passelsByRecipient.find(receivingFederateId);
      if (passels != message.passelsByRecipient.end()) {
        for (auto const& passel : passels->second) {
          auto projection = registry_.receiveOrderAttributeUpdateRecipientFor(
              federationName,
              message.producingFederateId,
              receivingFederateId,
              message.objectInstanceHandle,
              passel.sentAttributeHandles,
              passel.sentRegionHandles.empty() ? nullptr : &passel.sentRegionHandles,
              passel.sentRegionSnapshots.empty()
                  ? nullptr
                  : &passel.sentRegionSnapshots);
          if (!projection) {
            continue;
          }
          ProcessFederationAttributeUpdateEvent event;
          event.producingFederateId = message.producingFederateId;
          event.receivingFederateId = receivingFederateId;
          event.objectInstanceHandle = message.objectInstanceHandle;
          event.transportationName = passel.transportationName;
          event.defaultRegionUsed = passel.defaultRegionUsed;
          if (projection->conveyRegionDesignatorSets &&
              (!passel.sentRegionHandles.empty() || passel.defaultRegionUsed)) {
            event.sentRegionHandles = passel.sentRegionHandles;
          }
          event.timestamp = encodeProcessLogicalTime(message.timestamp);
          event.retractionMessageId = message.messageId;
          if (message.userSuppliedTag.size() != 0U) {
            auto const* data = static_cast<std::uint8_t const*>(
                message.userSuppliedTag.data());
            if (data == nullptr) {
              throw std::runtime_error(
                  "The process federation could not copy a timestamped attribute tag.");
            }
            event.userSuppliedTag.assign(
                data,
                data + message.userSuppliedTag.size());
          }
          for (auto const attributeHandle : passel.sentAttributeHandles) {
            if (!projection->receivedAttributeHandles.contains(attributeHandle)) {
              continue;
            }
            auto const value = std::find_if(
                message.attributes.begin(),
                message.attributes.end(),
                [attributeHandle](TsoAttributeValue const& candidate) {
                  return candidate.first == attributeHandle;
                });
            if (value == message.attributes.end()) {
              continue;
            }
            std::vector<std::uint8_t> bytes(value->second.size());
            if (!bytes.empty()) {
              auto const* data = static_cast<std::uint8_t const*>(
                  value->second.data());
              if (data == nullptr) {
                throw std::runtime_error(
                    "The process federation could not copy a timestamped attribute value.");
              }
              std::copy(data, data + bytes.size(), bytes.begin());
            }
            event.attributeValues.emplace_back(attributeHandle, std::move(bytes));
          }
          if (!event.attributeValues.empty()) {
            events.push_back(std::move(event));
          }
        }
      }

      if (events.empty() ||
          !registry_.beginTsoAttributeUpdateCallback(
              federationName,
              receivingFederateId,
              message.messageId)) {
        static_cast<void>(registry_.finishTsoRecipientCallbackSuppressed(
            federationName,
            receivingFederateId,
            message.messageId));
        completeDelivery();
        continue;
      }
      for (auto& event : events) {
        if (!receivingSession->send(TransportServiceMessage{
                TransportServiceMessageKind::event,
                TransportServiceOperation::receive_attribute_update,
                TransportServiceStatus::ok,
                0U,
                encodeProcessFederationReceiveAttributeUpdateResult(
                    ProcessFederationReceiveAttributeUpdateResult{event})})) {
          throw std::runtime_error(
              "The process federation could not deliver a timestamped attribute event.");
        }
      }
      {
        std::scoped_lock lock(mutex_);
        pendingPushedRetractionRecipients_[message.messageId].push_back(
            receivingSession);
      }
      // Keep the coordinator entry in-transit until the process receiver has
      // crossed its callback boundary and sends acknowledge_tso_delivery.
      continue;
    }

    if (auto const* directed =
            std::get_if<TsoDirectedInteractionDelivery>(&typedDelivery)) {
      auto const& message = directed->message;
      if (!message.timestamp) {
        throw std::runtime_error(
            "The process federation encountered a timestamped directed interaction without a timestamp.");
      }

      ProcessTransportSession* receivingSession = nullptr;
      {
        std::scoped_lock lock(mutex_);
        auto const session = sessionsByFederateId_.find(receivingFederateId);
        if (session != sessionsByFederateId_.end()) {
          auto const state = sessions_.find(session->second);
          if (state != sessions_.end() && state->second.federationName.has_value() &&
              *state->second.federationName == federationName &&
              state->second.federateId == receivingFederateId) {
            receivingSession = session->second;
          }
        }
      }

      auto completeDelivery = [&] {
        auto const completed = registry_.completeTsoDelivery(
            federationName,
            directed->queuedMessage);
        if (completed.status != FederationTsoRegistryStatus::applied ||
            (completed.delivery.status != FederationTsoDeliveryStatus::applied &&
             completed.delivery.status !=
                 FederationTsoDeliveryStatus::message_already_completed)) {
          throw std::runtime_error(
              "The process federation could not complete timestamped directed-interaction delivery.");
        }
      };

      if (receivingSession == nullptr) {
        static_cast<void>(registry_.finishTsoRecipientCallbackSuppressed(
            federationName,
            receivingFederateId,
            message.messageId));
        completeDelivery();
        continue;
      }

      auto const projection = registry_.timestampedDirectedInteractionRecipientFor(
          federationName,
          message.producingFederateId,
          receivingFederateId,
          message.objectInstanceHandle,
          message.sentInteractionClassHandle,
          message.sentParameterHandles,
          message.messageId);
      if (!projection || !registry_.beginTsoInteractionCallback(
                             federationName,
                             receivingFederateId,
                             message.messageId)) {
        static_cast<void>(registry_.finishTsoRecipientCallbackSuppressed(
            federationName,
            receivingFederateId,
            message.messageId));
        completeDelivery();
        continue;
      }

      ProcessFederationInteractionEnvelope envelope;
      envelope.userSuppliedTag.resize(message.userSuppliedTag.size());
      if (!envelope.userSuppliedTag.empty()) {
        auto const* data = static_cast<std::uint8_t const*>(
            message.userSuppliedTag.data());
        if (data == nullptr) {
          throw std::runtime_error(
              "The process federation could not copy a timestamped directed-interaction tag.");
        }
        std::copy(
            data,
            data + message.userSuppliedTag.size(),
            envelope.userSuppliedTag.begin());
      }
      envelope.parameterValues.reserve(message.parameters.size());
      for (auto const& [parameterHandle, parameterValue] : message.parameters) {
        std::vector<std::uint8_t> bytes(parameterValue.size());
        if (!bytes.empty()) {
          auto const* data = static_cast<std::uint8_t const*>(
              parameterValue.data());
          if (data == nullptr) {
            throw std::runtime_error(
                "The process federation could not copy a timestamped directed-interaction parameter.");
          }
          std::copy(data, data + bytes.size(), bytes.begin());
        }
        envelope.parameterValues.emplace_back(parameterHandle, std::move(bytes));
      }

      auto const timestamp = encodeProcessLogicalTime(message.timestamp);
      if (!timestamp) {
        throw std::runtime_error(
            "The process federation could not encode a timestamped directed-interaction time.");
      }
      ProcessFederationInteractionEvent event{
          message.producingFederateId,
          receivingFederateId,
          projection->receivedInteractionClassHandle,
          parameterVector(projection->receivedParameterHandles),
          encodeProcessFederationInteractionEnvelope(envelope),
          message.transportationName,
          std::move(timestamp),
          projection->objectInstanceHandle,
          message.messageId};
      event.sentOrderType = rti1516_2025::TIMESTAMP;
      event.receivedOrderType = rti1516_2025::TIMESTAMP;
      if (!receivingSession->send(TransportServiceMessage{
              TransportServiceMessageKind::event,
              TransportServiceOperation::receive_interaction,
              TransportServiceStatus::ok,
              0U,
              encodeProcessFederationReceiveInteractionResult(
                  ProcessFederationReceiveInteractionResult{event})})) {
        throw std::runtime_error(
            "The process federation could not deliver a timestamped directed-interaction event.");
      }
      {
        std::scoped_lock lock(mutex_);
        pendingPushedRetractionRecipients_[message.messageId].push_back(
            receivingSession);
      }
      // Keep the coordinator entry in-transit until the process receiver has
      // crossed its callback boundary and sends acknowledge_tso_delivery.
      continue;
    }

    if (auto const* deletion =
            std::get_if<TsoObjectDeletionDelivery>(&typedDelivery)) {
      auto const& message = deletion->message;
      if (!message.timestamp) {
        static_cast<void>(registry_.finishTsoRecipientCallbackSuppressed(
            federationName,
            receivingFederateId,
            message.messageId));
        throw std::runtime_error(
            "The process federation encountered a timestamped object deletion without a timestamp.");
      }

      ProcessTransportSession* receivingSession = nullptr;
      {
        std::scoped_lock lock(mutex_);
        auto const session = sessionsByFederateId_.find(receivingFederateId);
        if (session != sessionsByFederateId_.end()) {
          auto const state = sessions_.find(session->second);
          if (state != sessions_.end() && state->second.federationName.has_value() &&
              *state->second.federationName == federationName &&
              state->second.federateId == receivingFederateId) {
            receivingSession = session->second;
          }
        }
      }

      auto completeDelivery = [&] {
        auto const completed = registry_.completeTsoDelivery(
            federationName,
            deletion->queuedMessage);
        if (completed.status != FederationTsoRegistryStatus::applied ||
            (completed.delivery.status != FederationTsoDeliveryStatus::applied &&
             completed.delivery.status !=
                 FederationTsoDeliveryStatus::message_already_completed)) {
          throw std::runtime_error(
              "The process federation could not complete timestamped object-deletion delivery.");
        }
      };

      if (receivingSession == nullptr) {
        static_cast<void>(registry_.finishTsoRecipientCallbackSuppressed(
            federationName,
            receivingFederateId,
            message.messageId));
        completeDelivery();
        continue;
      }

      auto const target = std::find_if(
          message.recipients.begin(),
          message.recipients.end(),
          [receivingFederateId](TsoObjectDeletionRecipient const& candidate) {
            return candidate.receivingFederateId == receivingFederateId;
          });
      if (target == message.recipients.end()) {
        static_cast<void>(registry_.finishTsoRecipientCallbackSuppressed(
            federationName,
            receivingFederateId,
            message.messageId));
        completeDelivery();
        continue;
      }

      std::optional<RemovedObjectInstanceSnapshot> removal;
      {
        std::scoped_lock lock(mutex_);
        removal = registry_.beginTsoObjectInstanceRemoval(
            federationName,
            receivingFederateId,
            message.objectInstanceHandle,
            message.messageId);
      }
      if (!removal) {
        static_cast<void>(registry_.finishTsoRecipientCallbackSuppressed(
            federationName,
            receivingFederateId,
            message.messageId));
        completeDelivery();
        continue;
      }

      ProcessFederationObjectInstanceRemovalEvent event{
          receivingFederateId,
          removal->objectInstanceHandle,
          removal->producingFederateId,
          {},
          encodeProcessLogicalTime(message.timestamp),
          message.messageId,
          true,
          message.sentOrderType,
          rti1516_2025::TIMESTAMP};
      event.userSuppliedTag.resize(message.userSuppliedTag.size());
      if (!event.userSuppliedTag.empty()) {
        auto const* data = static_cast<std::uint8_t const*>(
            message.userSuppliedTag.data());
        if (data == nullptr) {
          throw std::runtime_error(
              "The process federation could not copy a timestamped deletion tag.");
        }
        std::copy(
            data,
            data + message.userSuppliedTag.size(),
            event.userSuppliedTag.begin());
      }
      if (!event.timestamp) {
        throw std::runtime_error(
            "The process federation could not encode a timestamped object-deletion time.");
      }

      ProcessFederationReceiveInteractionResult result;
      result.removalEvent = std::move(event);
      if (!receivingSession->send(TransportServiceMessage{
              TransportServiceMessageKind::event,
              TransportServiceOperation::receive_interaction,
              TransportServiceStatus::ok,
              0U,
              encodeProcessFederationReceiveInteractionResult(
                  std::move(result))})) {
        throw std::runtime_error(
            "The process federation could not deliver a timestamped object-deletion event.");
      }
      {
        std::scoped_lock lock(mutex_);
        pendingPushedRetractionRecipients_[message.messageId].push_back(
            receivingSession);
      }
      // Keep the coordinator entry in-transit until the process receiver has
      // crossed its callback boundary and sends acknowledge_tso_delivery.
      continue;
    }

    auto const* interaction = std::get_if<TsoInteractionDelivery>(&typedDelivery);
    if (interaction == nullptr) {
      // The process profile currently reconstructs regular and directed
      // timestamped interactions. Timestamped object/removal payloads retain
      // their existing bounded immediate process paths; if a future slice
      // admits one here, fail loudly instead of manufacturing a callback with
      // the wrong event shape.
      throw std::runtime_error(
          "The process federation encountered an unsupported timestamped payload.");
    }

    auto const& message = interaction->message;
    if (!message.timestamp) {
      throw std::runtime_error(
          "The process federation encountered a timestamped interaction without a timestamp.");
    }
    ProcessTransportSession* receivingSession = nullptr;
    {
      std::scoped_lock lock(mutex_);
      auto const session = sessionsByFederateId_.find(receivingFederateId);
      if (session != sessionsByFederateId_.end()) {
        auto const state = sessions_.find(session->second);
        if (state != sessions_.end() && state->second.federationName.has_value() &&
            *state->second.federationName == federationName &&
            state->second.federateId == receivingFederateId) {
          receivingSession = session->second;
        }
      }
    }

    auto completeDelivery = [&] {
      auto const completed = registry_.completeTsoDelivery(
          federationName,
          interaction->queuedMessage);
      if (completed.status != FederationTsoRegistryStatus::applied ||
          (completed.delivery.status != FederationTsoDeliveryStatus::applied &&
           completed.delivery.status !=
               FederationTsoDeliveryStatus::message_already_completed)) {
        throw std::runtime_error(
            "The process federation could not complete timestamped interaction delivery.");
      }
    };

    if (receivingSession == nullptr) {
      static_cast<void>(registry_.finishTsoRecipientCallbackSuppressed(
          federationName,
          receivingFederateId,
          message.messageId));
      completeDelivery();
      continue;
    }

    auto projection = registry_.receiveOrderInteractionRecipientFor(
        federationName,
        message.producingFederateId,
        receivingFederateId,
        message.sentInteractionClassHandle,
        message.sentParameterHandles,
        message.sentRegionHandles.empty() ? nullptr : &message.sentRegionHandles,
        message.sentRegionSnapshots.empty() ? nullptr : &message.sentRegionSnapshots);
    if (!projection || !registry_.beginTsoInteractionCallback(
                           federationName,
                           receivingFederateId,
                           message.messageId)) {
      static_cast<void>(registry_.finishTsoRecipientCallbackSuppressed(
          federationName,
          receivingFederateId,
          message.messageId));
      completeDelivery();
      continue;
    }

    ProcessFederationInteractionEnvelope envelope;
    envelope.userSuppliedTag.resize(message.userSuppliedTag.size());
    if (!envelope.userSuppliedTag.empty()) {
      auto const* data = static_cast<std::uint8_t const*>(
          message.userSuppliedTag.data());
      if (data == nullptr) {
        throw std::runtime_error(
            "The process federation could not copy a timestamped interaction tag.");
      }
      std::copy(data,
                data + message.userSuppliedTag.size(),
                envelope.userSuppliedTag.begin());
    }
    envelope.parameterValues.reserve(message.parameters.size());
    for (auto const& [parameterHandle, parameterValue] : message.parameters) {
      std::vector<std::uint8_t> bytes(parameterValue.size());
      if (!bytes.empty()) {
        auto const* data = static_cast<std::uint8_t const*>(parameterValue.data());
        if (data == nullptr) {
          throw std::runtime_error(
              "The process federation could not copy a timestamped interaction parameter.");
        }
        std::copy(data, data + bytes.size(), bytes.begin());
      }
      envelope.parameterValues.emplace_back(parameterHandle, std::move(bytes));
    }

    auto const timestamp = encodeProcessLogicalTime(message.timestamp);
    if (!timestamp) {
      throw std::runtime_error(
          "The process federation could not encode a timestamped interaction time.");
    }
    ProcessFederationInteractionEvent event{
        message.producingFederateId,
        receivingFederateId,
        projection->receivedInteractionClassHandle,
        parameterVector(projection->receivedParameterHandles),
        encodeProcessFederationInteractionEnvelope(envelope),
        message.transportationName,
        std::move(timestamp),
        std::nullopt,
        message.messageId};
    event.sentOrderType = rti1516_2025::TIMESTAMP;
    event.receivedOrderType = rti1516_2025::TIMESTAMP;
    event.defaultRegionUsed =
        projection->conveyRegionDesignatorSets && message.defaultRegionUsed;
    if (projection->conveyRegionDesignatorSets &&
        (!message.sentRegionHandles.empty() || message.defaultRegionUsed)) {
      event.sentRegionHandles = message.sentRegionHandles;
    }

    // TSO payloads must cross the process event boundary before the matching
    // grant event, even for the compatibility polling service option.  The
    // client accepts this unsolicited interaction frame and orders it ahead
    // of the queued grant callback; the service option continues to govern
    // ordinary receive-order traffic only.
    if (!receivingSession->send(TransportServiceMessage{
            TransportServiceMessageKind::event,
            TransportServiceOperation::receive_interaction,
            TransportServiceStatus::ok,
            0U,
            encodeProcessFederationReceiveInteractionResult(
                ProcessFederationReceiveInteractionResult{event})})) {
      throw std::runtime_error(
          "The process federation could not deliver a timestamped interaction event.");
    }
    {
      std::scoped_lock lock(mutex_);
      pendingPushedRetractionRecipients_[message.messageId].push_back(
          receivingSession);
    }
    // Keep the coordinator entry in-transit until the process receiver has
    // crossed its callback boundary and sends acknowledge_tso_delivery.
  }
}

bool ProcessFederationService::enqueueObjectInstanceDiscoveries(
    std::wstring const& federationName,
    std::vector<ObjectInstanceDiscoveryRecipient> discoveries) {
  std::vector<std::pair<ProcessTransportSession*,
                        ProcessFederationObjectInstanceDiscoveryEvent>>
      pushedEvents;
  std::vector<AttributeRelevanceAdvisoryRecipient> initialAdvisories;
  for (auto const& planned : discoveries) {
    ProcessTransportSession* receivingSession = nullptr;
    {
      std::scoped_lock lock(mutex_);
      auto const session = sessionsByFederateId_.find(planned.receivingFederateId);
      if (session != sessionsByFederateId_.end()) {
        auto const state = sessions_.find(session->second);
        if (state != sessions_.end() &&
            state->second.federationName.has_value() &&
            *state->second.federationName == federationName &&
            state->second.federateId == planned.receivingFederateId) {
          receivingSession = session->second;
        }
      }
    }
    if (receivingSession == nullptr) {
      // The registry reservation was made before the session lookup. Release
      // it when a detached process can no longer receive the event so a later
      // declaration change can retry the discovery.
      if (planned.rtiOwnedMomObject) {
        registry_.cancelJoinedFederateMomObjectDiscovery(
            federationName,
            planned.receivingFederateId,
            planned.objectInstanceHandle);
      } else {
        registry_.cancelObjectInstanceDiscovery(
            federationName,
            planned.receivingFederateId,
            planned.objectInstanceHandle);
      }
      continue;
    }

    // Recheck the discovery predicate and establish the receiving federate's
    // known-instance state before exposing the event. The process service has
    // no user callback route of its own, so this registry transition is the
    // process-boundary delivery commit; the client then projects the exact
    // snapshot through the official callback bridge.
    auto const snapshot = planned.rtiOwnedMomObject
        ? registry_.beginJoinedFederateMomObjectDiscovery(
              federationName,
              planned.receivingFederateId,
              planned.objectInstanceHandle)
        : registry_.beginObjectInstanceDiscovery(
              federationName,
              planned.receivingFederateId,
              planned.objectInstanceHandle);
    if (!snapshot) {
      continue;
    }
    if (!planned.rtiOwnedMomObject) {
      auto plannedAdvisories = registry_
          .planInitialAttributeRelevanceAdvisoriesForDiscovery(
              federationName,
              planned.receivingFederateId,
              planned.objectInstanceHandle);
      initialAdvisories.insert(
          initialAdvisories.end(),
          std::make_move_iterator(plannedAdvisories.begin()),
          std::make_move_iterator(plannedAdvisories.end()));
    }
    ProcessFederationObjectInstanceDiscoveryEvent event{
        planned.receivingFederateId,
        snapshot->objectInstanceHandle,
        snapshot->knownObjectClassHandle,
        snapshot->objectInstanceName,
        snapshot->producingFederateId};
    if (options_.pushReceiveOrderEvents) {
      pushedEvents.emplace_back(receivingSession, std::move(event));
      continue;
    }

    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(receivingSession);
    if (state != sessions_.end() &&
        state->second.federationName.has_value() &&
        *state->second.federationName == federationName &&
        state->second.federateId == planned.receivingFederateId) {
      state->second.objectInstanceDiscoveryEvents.push_back(std::move(event));
    }
  }

  if (!options_.pushReceiveOrderEvents) {
    return enqueueAttributeRelevanceAdvisories(
        federationName,
        std::move(initialAdvisories));
  }
  for (auto const& pushedEvent : pushedEvents) {
    if (pushedEvent.first == nullptr || !pushedEvent.first->send(
            TransportServiceMessage{
                TransportServiceMessageKind::event,
                TransportServiceOperation::receive_object_instance_discovery,
                TransportServiceStatus::ok,
                0U,
                encodeProcessFederationReceiveObjectInstanceDiscoveryResult(
                    ProcessFederationReceiveObjectInstanceDiscoveryResult{
                        pushedEvent.second})})) {
      return false;
    }
  }
  return enqueueAttributeRelevanceAdvisories(
      federationName,
      std::move(initialAdvisories));
}

bool ProcessFederationService::enqueueObjectInstanceRemovals(
    std::wstring const& federationName,
    std::vector<ObjectInstanceRemovalRecipient> removals,
    std::vector<std::uint8_t> userSuppliedTag,
    std::optional<ProcessFederationLogicalTime> timestamp,
    std::uint64_t retractionMessageId,
    bool provideRetraction) {
  std::vector<std::pair<ProcessTransportSession*,
                        ProcessFederationObjectInstanceRemovalEvent>>
      pushedEvents;
  std::vector<ProcessTransportSession*> retractionRecipients;
  for (auto& planned : removals) {
    ProcessTransportSession* receivingSession = nullptr;
    {
      std::scoped_lock lock(mutex_);
      auto const session = sessionsByFederateId_.find(planned.receivingFederateId);
      if (session != sessionsByFederateId_.end()) {
        auto const state = sessions_.find(session->second);
        if (state != sessions_.end() &&
            state->second.federationName.has_value() &&
            *state->second.federationName == federationName &&
            state->second.federateId == planned.receivingFederateId) {
          receivingSession = session->second;
        }
      }
    }
    if (receivingSession == nullptr) {
      // The registry reserved this callback before the session lookup. A
      // detached process cannot receive an ordinary callback, so release the
      // ordinary reservation. A timestamped removal owns a TSO retraction
      // ledger entry instead; keep that entry until the producer retracts it
      // or the federation lifecycle cleans it up.
      if (retractionMessageId == 0U) {
        registry_.cancelObjectInstanceRemoval(
            federationName,
            planned.receivingFederateId,
            planned.objectInstanceHandle);
      }
      continue;
    }

    ProcessFederationObjectInstanceRemovalEvent event{
        planned.receivingFederateId,
        planned.objectInstanceHandle,
        0U,
        userSuppliedTag};
    event.timestamp = timestamp;
    if (retractionMessageId != 0U) {
      event.retractionMessageId = retractionMessageId;
      event.provideRetraction = provideRetraction;
      event.sentOrderType = planned.sentOrderType;
      event.receivedOrderType = planned.receivedOrderType;
    }
    if (options_.pushReceiveOrderEvents) {
      auto const snapshot = retractionMessageId == 0U
          ? registry_.beginObjectInstanceRemoval(
                federationName,
                planned.receivingFederateId,
                planned.objectInstanceHandle)
          : registry_.beginTsoObjectInstanceRemoval(
                federationName,
                planned.receivingFederateId,
                planned.objectInstanceHandle,
                retractionMessageId);
      if (!snapshot) {
        continue;
      }
      event.producingFederateId = snapshot->producingFederateId;
      pushedEvents.emplace_back(receivingSession, std::move(event));
      if (retractionMessageId != 0U) {
        retractionRecipients.push_back(receivingSession);
      }
      continue;
    }

    // Pull-mode sessions retain the pending reservation until their next
    // receive poll, where handleReceiveInteraction commits the callback-time
    // registry transition and fills the producer identity.
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(receivingSession);
    if (state != sessions_.end() &&
        state->second.federationName.has_value() &&
        *state->second.federationName == federationName &&
        state->second.federateId == planned.receivingFederateId) {
      state->second.objectInstanceRemovalEvents.push_back(std::move(event));
      if (retractionMessageId != 0U) {
        retractionRecipients.push_back(receivingSession);
      }
    } else {
      if (retractionMessageId == 0U) {
        registry_.cancelObjectInstanceRemoval(
            federationName,
            planned.receivingFederateId,
            planned.objectInstanceHandle);
      }
    }
  }

  if (retractionMessageId != 0U) {
    std::scoped_lock lock(mutex_);
    pendingPushedRetractionRecipients_.emplace(
        retractionMessageId,
        std::move(retractionRecipients));
  }

  if (!options_.pushReceiveOrderEvents) {
    return true;
  }
  for (auto& pushedEvent : pushedEvents) {
    ProcessFederationReceiveInteractionResult result;
    result.removalEvent = std::move(pushedEvent.second);
    if (pushedEvent.first == nullptr ||
        !pushedEvent.first->send(TransportServiceMessage{
            TransportServiceMessageKind::event,
            TransportServiceOperation::receive_interaction,
            TransportServiceStatus::ok,
            0U,
            encodeProcessFederationReceiveInteractionResult(result)})) {
      return false;
    }
  }
  return true;
}

bool ProcessFederationService::enqueueObjectInstanceScopeChanges(
    std::wstring const& federationName,
    std::vector<ObjectInstanceScopeChangeRecipient> changes) {
  std::vector<std::pair<ProcessTransportSession*,
                        ProcessFederationObjectInstanceScopeChangeEvent>>
      pushedEvents;
  for (auto const& planned : changes) {
    if (planned.attributeHandles.empty()) {
      continue;
    }
    ProcessTransportSession* receivingSession = nullptr;
    {
      std::scoped_lock lock(mutex_);
      auto const session = sessionsByFederateId_.find(planned.receivingFederateId);
      if (session != sessionsByFederateId_.end()) {
        auto const state = sessions_.find(session->second);
        if (state != sessions_.end() &&
            state->second.federationName.has_value() &&
            *state->second.federationName == federationName &&
            state->second.federateId == planned.receivingFederateId) {
          receivingSession = session->second;
        }
      }
    }
    if (receivingSession == nullptr) {
      continue;
    }

    // The registry owns the callback-time predicate. Recheck it before the
    // event crosses the process boundary so disabled switches, resignation,
    // deletion, or a second region/subscription mutation suppress stale work.
    auto const eligible = registry_.objectInstanceScopeAttributes(
        federationName,
        planned.receivingFederateId,
        planned.objectInstanceHandle,
        planned.attributeHandles,
        planned.inScope);
    if (eligible.empty()) {
      continue;
    }
    ProcessFederationObjectInstanceScopeChangeEvent event{
        planned.receivingFederateId,
        planned.objectInstanceHandle,
        eligible,
        planned.inScope};
    if (options_.pushReceiveOrderEvents) {
      pushedEvents.emplace_back(receivingSession, std::move(event));
      continue;
    }

    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(receivingSession);
    if (state != sessions_.end() &&
        state->second.federationName.has_value() &&
        *state->second.federationName == federationName &&
        state->second.federateId == planned.receivingFederateId) {
      state->second.objectInstanceScopeChangeEvents.push_back(std::move(event));
    }
  }

  if (!options_.pushReceiveOrderEvents) {
    return true;
  }
  for (auto& pushedEvent : pushedEvents) {
    ProcessFederationReceiveInteractionResult result;
    result.scopeChangeEvent = std::move(pushedEvent.second);
    if (pushedEvent.first == nullptr || !pushedEvent.first->send(
            TransportServiceMessage{
                TransportServiceMessageKind::event,
                TransportServiceOperation::receive_interaction,
                TransportServiceStatus::ok,
                0U,
                encodeProcessFederationReceiveInteractionResult(result)})) {
      return false;
    }
  }
  return true;
}

bool ProcessFederationService::enqueueSynchronizationPointAnnouncements(
    std::wstring const& federationName,
    std::vector<SynchronizationPointAnnouncement> announcements) {
  std::vector<std::pair<ProcessTransportSession*,
                        ProcessFederationSynchronizationPointAnnouncementEvent>>
      pushedEvents;
  for (auto& planned : announcements) {
    ProcessTransportSession* receivingSession = nullptr;
    {
      std::scoped_lock lock(mutex_);
      auto const session = sessionsByFederateId_.find(planned.receivingFederateId);
      if (session != sessionsByFederateId_.end()) {
        auto const state = sessions_.find(session->second);
        if (state != sessions_.end() && state->second.federationName.has_value() &&
            *state->second.federationName == federationName &&
            state->second.federateId == planned.receivingFederateId) {
          receivingSession = session->second;
        }
      }
    }
    if (receivingSession == nullptr) {
      continue;
    }
    ProcessFederationSynchronizationPointAnnouncementEvent event{
        planned.receivingFederateId,
        planned.label,
        std::vector<std::uint8_t>(
            planned.userSuppliedTag.begin(), planned.userSuppliedTag.end())};
    if (options_.pushReceiveOrderEvents) {
      pushedEvents.emplace_back(receivingSession, std::move(event));
      continue;
    }
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(receivingSession);
    if (state != sessions_.end() && state->second.federationName.has_value() &&
        *state->second.federationName == federationName &&
        state->second.federateId == planned.receivingFederateId) {
      state->second.synchronizationPointAnnouncementEvents.push_back(
          std::move(event));
    }
  }
  if (!options_.pushReceiveOrderEvents) {
    return true;
  }
  for (auto& pushedEvent : pushedEvents) {
    ProcessFederationReceiveInteractionResult result;
    result.synchronizationPointAnnouncementEvent = std::move(pushedEvent.second);
    if (pushedEvent.first == nullptr ||
        !pushedEvent.first->send(TransportServiceMessage{
            TransportServiceMessageKind::event,
            TransportServiceOperation::receive_interaction,
            TransportServiceStatus::ok,
            0U,
            encodeProcessFederationReceiveInteractionResult(result)})) {
      return false;
    }
  }
  return true;
}

bool ProcessFederationService::enqueueFederationSynchronizedNotifications(
    std::wstring const& federationName,
    std::vector<FederationSynchronizedNotification> notifications) {
  std::vector<std::pair<ProcessTransportSession*,
                        ProcessFederationFederationSynchronizedEvent>>
      pushedEvents;
  for (auto& planned : notifications) {
    if (planned.receivingFederateId == 0U) {
      continue;
    }
    ProcessTransportSession* receivingSession = nullptr;
    {
      std::scoped_lock lock(mutex_);
      auto const session = sessionsByFederateId_.find(planned.receivingFederateId);
      if (session != sessionsByFederateId_.end()) {
        auto const state = sessions_.find(session->second);
        if (state != sessions_.end() && state->second.federationName.has_value() &&
            *state->second.federationName == federationName &&
            state->second.federateId == planned.receivingFederateId) {
          receivingSession = session->second;
        }
      }
    }
    if (receivingSession == nullptr) {
      continue;
    }
    ProcessFederationFederationSynchronizedEvent event{
        planned.receivingFederateId,
        planned.label,
        planned.failedToSyncFederateIds};
    if (options_.pushReceiveOrderEvents) {
      pushedEvents.emplace_back(receivingSession, std::move(event));
      continue;
    }
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(receivingSession);
    if (state != sessions_.end() && state->second.federationName.has_value() &&
        *state->second.federationName == federationName &&
        state->second.federateId == planned.receivingFederateId) {
      state->second.federationSynchronizedEvents.push_back(std::move(event));
    }
  }
  if (!options_.pushReceiveOrderEvents) {
    return true;
  }
  for (auto& pushedEvent : pushedEvents) {
    ProcessFederationReceiveInteractionResult result;
    result.federationSynchronizedEvent = std::move(pushedEvent.second);
    if (pushedEvent.first == nullptr ||
        !pushedEvent.first->send(TransportServiceMessage{
            TransportServiceMessageKind::event,
            TransportServiceOperation::receive_interaction,
            TransportServiceStatus::ok,
            0U,
            encodeProcessFederationReceiveInteractionResult(result)})) {
      return false;
    }
  }
  return true;
}

bool ProcessFederationService::enqueueFederationSaveNotifications(
    std::wstring const& federationName,
    std::vector<FederationSaveNotification> notifications) {
  std::vector<std::pair<ProcessTransportSession*, ProcessFederationSaveEvent>>
      pushedEvents;
  for (auto& planned : notifications) {
    ProcessTransportSession* receivingSession = nullptr;
    {
      std::scoped_lock lock(mutex_);
      auto const session = sessionsByFederateId_.find(planned.receivingFederateId);
      if (session != sessionsByFederateId_.end()) {
        auto const state = sessions_.find(session->second);
        if (state != sessions_.end() && state->second.federationName.has_value() &&
            *state->second.federationName == federationName &&
            state->second.federateId == planned.receivingFederateId) {
          receivingSession = session->second;
        }
      }
    }
    if (receivingSession == nullptr) {
      continue;
    }
    ProcessFederationSaveEvent event{
        planned.kind,
        planned.receivingFederateId,
        planned.label,
        planned.successful,
        planned.failureReason,
        std::move(planned.statuses)};
    event.timestamp = encodeProcessLogicalTime(planned.timestamp);
    if (planned.timestamp && !event.timestamp) {
      return false;
    }
    if (options_.pushReceiveOrderEvents) {
      pushedEvents.emplace_back(receivingSession, std::move(event));
      continue;
    }
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(receivingSession);
    if (state != sessions_.end() && state->second.federationName.has_value() &&
        *state->second.federationName == federationName &&
        state->second.federateId == planned.receivingFederateId) {
      state->second.saveEvents.push_back(std::move(event));
    }
  }
  if (!options_.pushReceiveOrderEvents) {
    return true;
  }
  for (auto& pushedEvent : pushedEvents) {
    ProcessFederationReceiveInteractionResult result;
    result.saveEvent = std::move(pushedEvent.second);
    if (pushedEvent.first == nullptr ||
        !pushedEvent.first->send(TransportServiceMessage{
            TransportServiceMessageKind::event,
            TransportServiceOperation::receive_interaction,
            TransportServiceStatus::ok,
            0U,
            encodeProcessFederationReceiveInteractionResult(result)})) {
      return false;
    }
  }
  return true;
}

bool ProcessFederationService::enqueueFederationRestoreNotifications(
    std::wstring const& federationName,
    std::vector<FederationRestoreNotification> notifications) {
  std::vector<std::pair<ProcessTransportSession*, ProcessFederationRestoreEvent>>
      pushedEvents;
  for (auto& planned : notifications) {
    ProcessTransportSession* receivingSession = nullptr;
    {
      std::scoped_lock lock(mutex_);
      auto const session = sessionsByFederateId_.find(planned.receivingFederateId);
      if (session != sessionsByFederateId_.end()) {
        auto const state = sessions_.find(session->second);
        if (state != sessions_.end() && state->second.federationName.has_value() &&
            *state->second.federationName == federationName &&
            state->second.federateId == planned.receivingFederateId) {
          receivingSession = session->second;
        }
      }
    }
    if (receivingSession == nullptr) {
      continue;
    }
    ProcessFederationRestoreEvent event;
    event.kind = planned.kind;
    event.receivingFederateId = planned.receivingFederateId;
    event.label = planned.label;
    event.federateName = planned.federateName;
    event.preRestoreFederateId = planned.preRestoreFederateId;
    event.postRestoreFederateId = planned.postRestoreFederateId;
    event.successful = planned.successful;
    event.failureReason = planned.failureReason;
    event.statuses.reserve(planned.statuses.size());
    for (auto const& status : planned.statuses) {
      event.statuses.push_back(ProcessFederationRestoreEvent::StatusRecord{
          status.preRestoreFederateId,
          status.postRestoreFederateId,
          status.status});
    }
    if (options_.pushReceiveOrderEvents) {
      pushedEvents.emplace_back(receivingSession, std::move(event));
      continue;
    }
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(receivingSession);
    if (state != sessions_.end() && state->second.federationName.has_value() &&
        *state->second.federationName == federationName &&
        state->second.federateId == planned.receivingFederateId) {
      state->second.restoreEvents.push_back(std::move(event));
    }
  }
  if (!options_.pushReceiveOrderEvents) {
    return true;
  }
  for (auto& pushedEvent : pushedEvents) {
    ProcessFederationReceiveInteractionResult result;
    result.restoreEvent = std::move(pushedEvent.second);
    if (pushedEvent.first == nullptr ||
        !pushedEvent.first->send(TransportServiceMessage{
            TransportServiceMessageKind::event,
            TransportServiceOperation::receive_interaction,
            TransportServiceStatus::ok,
            0U,
            encodeProcessFederationReceiveInteractionResult(result)})) {
      return false;
    }
  }
  return true;
}

bool ProcessFederationService::enqueueAttributeRelevanceAdvisories(
    std::wstring const& federationName,
    std::vector<AttributeRelevanceAdvisoryRecipient> advisories) {
  std::vector<std::pair<ProcessTransportSession*,
                        ProcessFederationAttributeRelevanceAdvisoryEvent>>
      pushedEvents;
  for (auto const& planned : advisories) {
    if (planned.attributeHandles.empty() || planned.providingFederateId == 0U) {
      continue;
    }

    // Attribute Relevance Advisory is delivered to the providing/owning
    // federate, not to the receiving subscription federate.  Resolve that
    // session before crossing the process boundary; a detached owner cannot
    // receive a callback and therefore must not retain stale queued work.
    ProcessTransportSession* providingSession = nullptr;
    {
      std::scoped_lock lock(mutex_);
      auto const session = sessionsByFederateId_.find(planned.providingFederateId);
      if (session != sessionsByFederateId_.end()) {
        auto const state = sessions_.find(session->second);
        if (state != sessions_.end() &&
            state->second.federationName.has_value() &&
            *state->second.federationName == federationName &&
            state->second.federateId == planned.providingFederateId) {
          providingSession = session->second;
        }
      }
    }
    if (providingSession == nullptr) {
      continue;
    }

    // Recheck the switch, ownership, object lifetime, and relevance before
    // admission.  The registry deliberately accepts receivingFederateId == 0
    // for this owner-directed callback family.
    auto const eligible = registry_.attributeRelevanceAdvisoryAttributes(
        federationName,
        planned.providingFederateId,
        planned.receivingFederateId,
        planned.objectInstanceHandle,
        planned.attributeHandles,
        planned.turnUpdatesOn);
    if (eligible.empty()) {
      continue;
    }

    std::map<std::optional<std::string>, std::set<std::uint64_t>> byRate;
    for (std::uint64_t const attributeHandle : eligible) {
      auto const updateRateDesignator = planned.turnUpdatesOn
          ? registry_.attributeRelevanceAdvisoryUpdateRateDesignatorFor(
                federationName,
                planned.receivingFederateId,
                planned.objectInstanceHandle,
                attributeHandle)
          : std::nullopt;
      byRate[updateRateDesignator].insert(attributeHandle);
    }
    for (auto& [updateRateDesignator, attributeHandles] : byRate) {
      ProcessFederationAttributeRelevanceAdvisoryEvent event{
          planned.providingFederateId,
          planned.receivingFederateId,
          planned.objectInstanceHandle,
          std::move(attributeHandles),
          planned.turnUpdatesOn,
          std::move(updateRateDesignator)};
      if (options_.pushReceiveOrderEvents) {
        pushedEvents.emplace_back(providingSession, std::move(event));
      } else {
        std::scoped_lock lock(mutex_);
        auto const state = sessions_.find(providingSession);
        if (state != sessions_.end() &&
            state->second.federationName.has_value() &&
            *state->second.federationName == federationName &&
            state->second.federateId == planned.providingFederateId) {
          state->second.attributeRelevanceAdvisoryEvents.push_back(
              std::move(event));
        }
      }
    }
  }

  if (!options_.pushReceiveOrderEvents) {
    return true;
  }
  for (auto& pushedEvent : pushedEvents) {
    ProcessFederationReceiveInteractionResult result;
    result.attributeRelevanceAdvisoryEvent = std::move(pushedEvent.second);
    if (pushedEvent.first == nullptr ||
        !pushedEvent.first->send(TransportServiceMessage{
            TransportServiceMessageKind::event,
            TransportServiceOperation::receive_interaction,
            TransportServiceStatus::ok,
            0U,
            encodeProcessFederationReceiveInteractionResult(result)})) {
      return false;
    }
  }
  return true;
}

bool ProcessFederationService::enqueueAttributeOwnershipAcquisitionWorkItems(
    std::wstring const& federationName,
    std::vector<AttributeOwnershipAcquisitionWorkItem> workItems) {
  std::vector<std::pair<ProcessTransportSession*,
                        ProcessFederationAttributeOwnershipAcquisitionEvent>>
      pushedEvents;

  // Follow-up work may be exposed by the callback-boundary registry
  // transition (for example, the next queued requester after an unowned
  // notification). Keep it in the same deterministic order as the registry
  // returned it, without recursing through the service mutex.
  for (std::size_t index = 0U; index < workItems.size(); ++index) {
    auto workItem = std::move(workItems[index]);
    ProcessFederationAttributeOwnershipAcquisitionEventKind eventKind;
    std::uint64_t receivingFederateId = 0U;
    if (workItem.kind == AttributeOwnershipAcquisitionWorkKind::
                            acquisition_notification) {
      eventKind = ProcessFederationAttributeOwnershipAcquisitionEventKind::
          acquisition_notification;
      receivingFederateId = workItem.requestingFederateId;
    } else if (workItem.kind == AttributeOwnershipAcquisitionWorkKind::
                                    request_release) {
      eventKind = ProcessFederationAttributeOwnershipAcquisitionEventKind::
          request_release;
      receivingFederateId = workItem.receivingFederateId;
    } else if (workItem.kind == AttributeOwnershipAcquisitionWorkKind::
                                    request_divestiture_confirmation) {
      eventKind = ProcessFederationAttributeOwnershipAcquisitionEventKind::
          request_divestiture_confirmation;
      receivingFederateId = workItem.receivingFederateId;
    } else {
      // If Available delivery is projected through its own result slot.
      return false;
    }
    if (workItem.requestId == 0U || workItem.requestingFederateId == 0U ||
        receivingFederateId == 0U || workItem.objectInstanceHandle == 0U ||
        workItem.attributeHandles.empty()) {
      return false;
    }

    ProcessTransportSession* receivingSession = nullptr;
    {
      std::scoped_lock lock(mutex_);
      auto const session = sessionsByFederateId_.find(receivingFederateId);
      if (session != sessionsByFederateId_.end()) {
        auto const state = sessions_.find(session->second);
        if (state != sessions_.end() && state->second.federationName &&
            *state->second.federationName == federationName &&
            state->second.federateId == receivingFederateId) {
          receivingSession = session->second;
        }
      }
    }
    if (receivingSession == nullptr) {
      return false;
    }

    ProcessFederationAttributeOwnershipAcquisitionEvent event{
        eventKind,
        workItem.requestId,
        workItem.requestingFederateId,
        receivingFederateId,
        workItem.objectInstanceHandle,
        workItem.attributeHandles,
        workItem.userSuppliedTag,
        workItem.candidateIsIfAvailable};

    // A release request is a callback-boundary reservation, not an already
    // delivered callback. Keep it in the service queue until the recipient
    // polls so automatic connection-loss cleanup can cancel it atomically.
    if (options_.pushReceiveOrderEvents &&
        eventKind !=
            ProcessFederationAttributeOwnershipAcquisitionEventKind::request_release) {
      if (eventKind ==
          ProcessFederationAttributeOwnershipAcquisitionEventKind::
              acquisition_notification) {
        auto const delivery = registry_.beginAttributeOwnershipAcquisitionNotification(
            federationName,
            workItem.requestingFederateId,
            workItem.objectInstanceHandle,
            workItem.requestId,
            workItem.attributeHandles);
        if (!delivery) {
          continue;
        }
        event.attributeHandles = delivery->securedAttributeHandles;
        for (auto& followup : delivery->followupWorkItems) {
          workItems.push_back(std::move(followup));
        }
      } else if (eventKind ==
                 ProcessFederationAttributeOwnershipAcquisitionEventKind::
                     request_release) {
        auto const delivery = registry_.beginAttributeOwnershipAcquisitionRelease(
            federationName,
            workItem.requestingFederateId,
            workItem.receivingFederateId,
            workItem.objectInstanceHandle,
            workItem.requestId,
            workItem.attributeHandles);
        if (!delivery) {
          continue;
        }
        event.attributeHandles = delivery->candidateAttributeHandles;
      } else {
        auto const delivery = registry_.beginRequestDivestitureConfirmation(
            federationName,
            workItem.receivingFederateId,
            workItem.requestingFederateId,
            workItem.objectInstanceHandle,
            workItem.requestId,
            workItem.candidateIsIfAvailable,
            workItem.attributeHandles);
        if (!delivery) {
          continue;
        }
        event.attributeHandles = delivery->releasedAttributeHandles;
      }
      if (event.attributeHandles.empty()) {
        continue;
      }
      pushedEvents.emplace_back(receivingSession, std::move(event));
      continue;
    }

    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(receivingSession);
    if (state == sessions_.end() || !state->second.federationName ||
        *state->second.federationName != federationName ||
        state->second.federateId != receivingFederateId) {
      return false;
    }
    state->second.attributeOwnershipAcquisitionEvents.push_back(
        std::move(event));
  }

  if (!options_.pushReceiveOrderEvents) {
    return true;
  }
  for (auto& pushedEvent : pushedEvents) {
    ProcessFederationReceiveInteractionResult result;
    result.attributeOwnershipAcquisitionEvent = std::move(pushedEvent.second);
    if (pushedEvent.first == nullptr ||
        !pushedEvent.first->send(TransportServiceMessage{
            TransportServiceMessageKind::event,
            TransportServiceOperation::receive_interaction,
            TransportServiceStatus::ok,
            0U,
            encodeProcessFederationReceiveInteractionResult(result)})) {
      return false;
    }
  }
  return true;
}

bool ProcessFederationService::enqueueAttributeOwnershipAssumptionRecipients(
    std::wstring const& federationName,
    std::vector<AttributeOwnershipAssumptionRecipient> recipients,
    bool deferUntilCallbackEnabled) {
  for (auto& recipient : recipients) {
    if (recipient.receivingFederateId == 0U ||
        recipient.objectInstanceHandle == 0U ||
        recipient.attributeHandles.empty()) {
      return false;
    }
    ProcessTransportSession* receivingSession = nullptr;
    {
      std::scoped_lock lock(mutex_);
      auto const session = sessionsByFederateId_.find(recipient.receivingFederateId);
      if (session != sessionsByFederateId_.end()) {
        auto const state = sessions_.find(session->second);
        if (state != sessions_.end() && state->second.federationName &&
            *state->second.federationName == federationName &&
            state->second.federateId == recipient.receivingFederateId) {
          receivingSession = session->second;
        }
      }
    }
    if (receivingSession == nullptr) {
      return false;
    }

    // The ordinary ownership event frame keeps all process callback queues
    // ordered by one receive fence. Assumption has no request id, so the
    // candidate identity is repeated in both federate fields as a private
    // routing invariant and the callback bridge projects the official form.
    ProcessFederationAttributeOwnershipAcquisitionEvent event{
        ProcessFederationAttributeOwnershipAcquisitionEventKind::
            ownership_assumption,
        0U,
        recipient.receivingFederateId,
        recipient.receivingFederateId,
        recipient.objectInstanceHandle,
        recipient.attributeHandles,
        recipient.userSuppliedTag,
        false};
    if (options_.pushReceiveOrderEvents) {
      if (deferUntilCallbackEnabled) {
        std::scoped_lock lock(mutex_);
        auto const state = sessions_.find(receivingSession);
        if (state == sessions_.end() || !state->second.federationName ||
            *state->second.federationName != federationName ||
            state->second.federateId != recipient.receivingFederateId) {
          return false;
        }
        auto const duplicate = std::any_of(
            state->second.attributeOwnershipAcquisitionEvents.begin(),
            state->second.attributeOwnershipAcquisitionEvents.end(),
            [&event](ProcessFederationAttributeOwnershipAcquisitionEvent const& queued) {
              return queued.kind ==
                         ProcessFederationAttributeOwnershipAcquisitionEventKind::
                             ownership_assumption &&
                  queued.receivingFederateId == event.receivingFederateId &&
                  queued.objectInstanceHandle == event.objectInstanceHandle &&
                  queued.attributeHandles == event.attributeHandles &&
                  queued.userSuppliedTag == event.userSuppliedTag;
            });
        if (!duplicate) {
          state->second.attributeOwnershipAcquisitionEvents.push_back(
              std::move(event));
        }
        continue;
      }
      auto const delivery = registry_.attributeOwnershipAssumptionDeliveryFor(
          federationName,
          recipient.receivingFederateId,
          recipient.objectInstanceHandle,
          recipient.attributeHandles);
      if (!delivery || delivery->attributeHandles.empty()) {
        continue;
      }
      event.attributeHandles = delivery->attributeHandles;
      ProcessFederationReceiveInteractionResult pushedResult;
      pushedResult.attributeOwnershipAcquisitionEvent = std::move(event);
      if (!receivingSession->send(TransportServiceMessage{
              TransportServiceMessageKind::event,
              TransportServiceOperation::receive_interaction,
              TransportServiceStatus::ok,
              0U,
              encodeProcessFederationReceiveInteractionResult(pushedResult)})) {
        return false;
      }
      continue;
    }
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(receivingSession);
    if (state == sessions_.end() || !state->second.federationName ||
        *state->second.federationName != federationName ||
        state->second.federateId != recipient.receivingFederateId) {
      return false;
    }
    state->second.attributeOwnershipAcquisitionEvents.push_back(std::move(event));
  }
  return true;
}

bool ProcessFederationService::
    flushDeferredAttributeOwnershipAssumptionEvents(
        ProcessTransportSession& session,
        std::wstring const& federationName,
        std::uint64_t receivingFederateId) {
  std::vector<ProcessFederationAttributeOwnershipAcquisitionEvent> deferred;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() || !state->second.federationName ||
        *state->second.federationName != federationName ||
        state->second.federateId != receivingFederateId) {
      return false;
    }
    auto& queued = state->second.attributeOwnershipAcquisitionEvents;
    for (auto iterator = queued.begin(); iterator != queued.end();) {
      if (iterator->kind !=
              ProcessFederationAttributeOwnershipAcquisitionEventKind::
                  ownership_assumption ||
          iterator->receivingFederateId != receivingFederateId) {
        ++iterator;
        continue;
      }
      deferred.push_back(std::move(*iterator));
      iterator = queued.erase(iterator);
    }
  }

  for (auto& event : deferred) {
    auto const delivery = registry_.attributeOwnershipAssumptionDeliveryFor(
        federationName,
        receivingFederateId,
        event.objectInstanceHandle,
        event.attributeHandles);
    if (!delivery || delivery->attributeHandles.empty()) {
      continue;
    }
    event.attributeHandles = delivery->attributeHandles;
    ProcessFederationReceiveInteractionResult pushedResult;
    pushedResult.attributeOwnershipAcquisitionEvent = std::move(event);
    if (!session.send(TransportServiceMessage{
            TransportServiceMessageKind::event,
            TransportServiceOperation::receive_interaction,
            TransportServiceStatus::ok,
            0U,
            encodeProcessFederationReceiveInteractionResult(pushedResult)})) {
      return false;
    }
  }
  return true;
}

bool ProcessFederationService::enqueueConfirmDivestitureNotifications(
    std::wstring const& federationName,
    std::vector<ConfirmDivestitureNotification> notifications) {
  std::vector<std::pair<ProcessTransportSession*,
                        ProcessFederationAttributeOwnershipAcquisitionEvent>>
      pushedEvents;
  std::vector<AttributeOwnershipAcquisitionWorkItem> followupWorkItems;
  for (auto& notification : notifications) {
    if (notification.notificationId == 0U ||
        notification.receivingFederateId == 0U ||
        notification.objectInstanceHandle == 0U ||
        notification.attributeHandles.empty()) {
      return false;
    }

    ProcessTransportSession* receivingSession = nullptr;
    {
      std::scoped_lock lock(mutex_);
      auto const session =
          sessionsByFederateId_.find(notification.receivingFederateId);
      if (session != sessionsByFederateId_.end()) {
        auto const state = sessions_.find(session->second);
        if (state != sessions_.end() && state->second.federationName &&
            *state->second.federationName == federationName &&
            state->second.federateId == notification.receivingFederateId) {
          receivingSession = session->second;
        }
      }
    }
    if (receivingSession == nullptr) {
      return false;
    }

    // Confirm Divestiture has already committed ownership in the registry.
    // The callback boundary below consumes its typed reservation and makes
    // the secured set visible to the receiving federate in one receive-order
    // event frame.  Keep the private requester/recipient identities equal so
    // the public bridge can project the official acquisition notification.
    ProcessFederationAttributeOwnershipAcquisitionEvent event{
        ProcessFederationAttributeOwnershipAcquisitionEventKind::
            confirm_divestiture_notification,
        notification.notificationId,
        notification.receivingFederateId,
        notification.receivingFederateId,
        notification.objectInstanceHandle,
        notification.attributeHandles,
        notification.userSuppliedTag,
        false};

    if (options_.pushReceiveOrderEvents) {
      auto const delivery = registry_.beginConfirmDivestitureNotification(
          federationName,
          notification.receivingFederateId,
          notification.objectInstanceHandle,
          notification.notificationId,
          notification.attributeHandles);
      if (!delivery) {
        continue;
      }
      event.attributeHandles = delivery->securedAttributeHandles;
      if (!event.attributeHandles.empty()) {
        pushedEvents.emplace_back(receivingSession, std::move(event));
      }
      for (auto& followup : delivery->followupWorkItems) {
        followupWorkItems.push_back(std::move(followup));
      }
      continue;
    }

    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(receivingSession);
    if (state == sessions_.end() || !state->second.federationName ||
        *state->second.federationName != federationName ||
        state->second.federateId != notification.receivingFederateId) {
      return false;
    }
    state->second.attributeOwnershipAcquisitionEvents.push_back(
        std::move(event));
  }

  if (!options_.pushReceiveOrderEvents) {
    return true;
  }
  for (auto& pushedEvent : pushedEvents) {
    ProcessFederationReceiveInteractionResult result;
    result.attributeOwnershipAcquisitionEvent = std::move(pushedEvent.second);
    if (pushedEvent.first == nullptr ||
        !pushedEvent.first->send(TransportServiceMessage{
            TransportServiceMessageKind::event,
            TransportServiceOperation::receive_interaction,
            TransportServiceStatus::ok,
            0U,
            encodeProcessFederationReceiveInteractionResult(result)})) {
      return false;
    }
  }
  if (!followupWorkItems.empty() &&
      !enqueueAttributeOwnershipAcquisitionWorkItems(
          federationName, std::move(followupWorkItems))) {
    return false;
  }
  return true;
}

bool ProcessFederationService::enqueueAttributeOwnershipUnavailableRecipients(
    std::wstring const& federationName,
    std::vector<AttributeOwnershipUnavailableRecipient> recipients,
    std::vector<std::uint8_t> userSuppliedTag) {
  std::vector<std::pair<ProcessTransportSession*,
                        ProcessFederationAttributeOwnershipUnavailableEvent>>
      pushedEvents;
  for (auto& planned : recipients) {
    if (planned.receivingFederateId == 0U ||
        planned.objectInstanceHandle == 0U || planned.attributeHandles.empty()) {
      return false;
    }
    ProcessTransportSession* receivingSession = nullptr;
    {
      std::scoped_lock lock(mutex_);
      auto const session = sessionsByFederateId_.find(planned.receivingFederateId);
      if (session != sessionsByFederateId_.end()) {
        auto const state = sessions_.find(session->second);
        if (state != sessions_.end() && state->second.federationName &&
            *state->second.federationName == federationName &&
            state->second.federateId == planned.receivingFederateId) {
          receivingSession = session->second;
        }
      }
    }
    if (receivingSession == nullptr) {
      return false;
    }

    ProcessFederationAttributeOwnershipUnavailableEvent event{
        planned.receivingFederateId,
        planned.objectInstanceHandle,
        planned.attributeHandles,
        userSuppliedTag};
    if (options_.pushReceiveOrderEvents) {
      // Commit the callback reservation at the service boundary for pushed
      // events. Pull mode performs the same revalidation in receiveInteraction
      // immediately before exposing the event to the client.
      auto const delivery = registry_.attributeOwnershipUnavailableRecipientFor(
          federationName,
          event.receivingFederateId,
          event.objectInstanceHandle,
          event.attributeHandles);
      if (!delivery || delivery->attributeHandles.empty()) {
        continue;
      }
      event.attributeHandles = delivery->attributeHandles;
      pushedEvents.emplace_back(receivingSession, std::move(event));
    } else {
      std::scoped_lock lock(mutex_);
      auto const state = sessions_.find(receivingSession);
      if (state == sessions_.end() || !state->second.federationName ||
          *state->second.federationName != federationName ||
          state->second.federateId != planned.receivingFederateId) {
        return false;
      }
      state->second.attributeOwnershipUnavailableEvents.push_back(
          std::move(event));
    }
  }

  if (!options_.pushReceiveOrderEvents) {
    return true;
  }
  for (auto& pushedEvent : pushedEvents) {
    ProcessFederationReceiveInteractionResult result;
    result.attributeOwnershipUnavailableEvent = std::move(pushedEvent.second);
    if (pushedEvent.first == nullptr ||
        !pushedEvent.first->send(TransportServiceMessage{
            TransportServiceMessageKind::event,
            TransportServiceOperation::receive_interaction,
            TransportServiceStatus::ok,
            0U,
            encodeProcessFederationReceiveInteractionResult(result)})) {
      return false;
    }
  }
  return true;
}

TransportServiceMessage ProcessFederationService::handleUpdateAttributeValues(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const updateRequest =
      decodeProcessFederationUpdateAttributeValuesRequest(request.payload);
  if (updateRequest.timestamp) {
    validateProcessLogicalTime(
        *updateRequest.timestamp,
        federationDefinition_->logicalTimeImplementationName);
  }
  std::shared_ptr<FederateTimeState> producingTimeState;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != updateRequest.federationName ||
        state->second.federateId != updateRequest.producingFederateId ||
        !registry_.memberById(
            updateRequest.federationName, updateRequest.producingFederateId)) {
      return rejected(request);
    }
    producingTimeState = state->second.timeState;
  }

  std::vector<std::uint64_t> sentAttributeHandles;
  sentAttributeHandles.reserve(updateRequest.attributeValues.size());
  for (auto const& [attributeHandle, value] : updateRequest.attributeValues) {
    static_cast<void>(value);
    sentAttributeHandles.push_back(attributeHandle);
  }
  auto const plan = registry_.planReceiveOrderAttributeUpdate(
      updateRequest.federationName,
      updateRequest.producingFederateId,
      updateRequest.objectInstanceHandle,
      sentAttributeHandles);
  if (plan.status != ReceiveOrderAttributeUpdateStatus::applied) {
    return rejected(request);
  }

  auto const producingTimeSnapshot = producingTimeState
      ? producingTimeState->snapshot()
      : FederateTimeSnapshot{};
  bool const queueTimestampedAttributeUpdate =
      updateRequest.timestamp && producingTimeSnapshot.timeRegulating &&
      std::any_of(
          plan.passels.begin(),
          plan.passels.end(),
          [](ReceiveOrderAttributeUpdatePassel const& passel) {
            return passel.preferredOrderType == rti1516_2025::TIMESTAMP;
          });
  std::set<std::uint64_t> timeConstrainedRecipients;
  if (queueTimestampedAttributeUpdate) {
    auto const execution = registry_.timeSnapshotFor(updateRequest.federationName);
    if (!execution) {
      return internalError(request);
    }
    for (auto const& federate : execution->federates) {
      if (federate.time.timeConstrained) {
        timeConstrainedRecipients.insert(federate.membership.id);
      }
    }
  }

  std::uint64_t messageId = 0U;
  if (queueTimestampedAttributeUpdate) {
    TsoAttributeUpdateMessage message;
    message.producingFederateId = updateRequest.producingFederateId;
    message.objectInstanceHandle = updateRequest.objectInstanceHandle;
    message.userSuppliedTag.setData(
        updateRequest.userSuppliedTag.data(),
        updateRequest.userSuppliedTag.size());
    message.timestamp = decodeProcessLogicalTime(
        *updateRequest.timestamp,
        federationDefinition_->logicalTimeImplementationName);
    message.attributes.reserve(updateRequest.attributeValues.size());
    for (auto const& [attributeHandle, value] : updateRequest.attributeValues) {
      rti1516_2025::VariableLengthData encodedValue;
      if (!value.empty()) {
        encodedValue.setData(value.data(), value.size());
      }
      message.attributes.emplace_back(attributeHandle, std::move(encodedValue));
    }

    auto const snapshotsEqual = [](
        RegionSpecificationSnapshot const& left,
        RegionSpecificationSnapshot const& right) {
      if (left.dimensionHandles != right.dimensionHandles ||
          left.specificationCommitted != right.specificationCommitted ||
          left.committedRangeBounds.size() != right.committedRangeBounds.size()) {
        return false;
      }
      for (auto const& [dimensionHandle, leftBounds] : left.committedRangeBounds) {
        auto const rightBounds = right.committedRangeBounds.find(dimensionHandle);
        if (rightBounds == right.committedRangeBounds.end() ||
            leftBounds.lowerBound != rightBounds->second.lowerBound ||
            leftBounds.upperBound != rightBounds->second.upperBound) {
          return false;
        }
      }
      return true;
    };
    std::vector<std::uint64_t> queuedRecipients;
    std::vector<std::uint64_t> allTimestampedRecipients;
    for (auto const& passel : plan.passels) {
      if (passel.preferredOrderType != rti1516_2025::TIMESTAMP) {
        continue;
      }
      TsoAttributeUpdatePassel payloadPassel;
      payloadPassel.transportationName = passel.transportationName;
      payloadPassel.sentAttributeHandles = passel.sentAttributeHandles;
      payloadPassel.sentRegionHandles = passel.sentRegionHandles;
      payloadPassel.sentRegionSnapshots = passel.sentRegionSnapshots;
      payloadPassel.defaultRegionUsed = passel.defaultRegionUsed;
      payloadPassel.preferredOrderType = passel.preferredOrderType;
      for (auto const& plannedRecipient : passel.recipients) {
        allTimestampedRecipients.push_back(plannedRecipient.federateId);
        message.passelsByRecipient[plannedRecipient.federateId].push_back(
            payloadPassel);
        if (timeConstrainedRecipients.contains(plannedRecipient.federateId)) {
          queuedRecipients.push_back(plannedRecipient.federateId);
        }
        for (auto const& [regionHandle, snapshot] : passel.sentRegionSnapshots) {
          auto const [position, inserted] =
              message.sentRegionSnapshots.emplace(regionHandle, snapshot);
          if (!inserted && !snapshotsEqual(position->second, snapshot)) {
            return internalError(request);
          }
        }
      }
    }

    auto const result = registry_.enqueueTsoAttributeUpdate(
        updateRequest.federationName,
        std::move(message),
        queuedRecipients,
        allTimestampedRecipients);
    if (result.status != FederationTsoRegistryStatus::applied ||
        result.queueStatus != TsoMessageQueueStatus::applied ||
        result.messageId == 0U) {
      return rejected(request);
    }
    messageId = result.messageId;
    std::scoped_lock lock(mutex_);
    processTsoMessageProducers_.emplace(
        messageId,
        updateRequest.producingFederateId);
  }

  std::vector<std::pair<
      std::uint64_t,
      rti1516_2025::VariableLengthData>> acceptedAttributeValues;
  acceptedAttributeValues.reserve(updateRequest.attributeValues.size());
  for (auto const& [attributeHandle, value] : updateRequest.attributeValues) {
    rti1516_2025::VariableLengthData encodedValue;
    if (!value.empty()) {
      encodedValue.setData(value.data(), value.size());
    }
    acceptedAttributeValues.emplace_back(attributeHandle, std::move(encodedValue));
  }
  std::set<std::string> transportationNames;
  for (auto const& passel : plan.passels) {
    transportationNames.insert(passel.transportationName);
  }
  if (registry_.recordSuccessfulUpdateAttributeValues(
          updateRequest.federationName,
          updateRequest.producingFederateId,
          updateRequest.objectInstanceHandle,
          plan.registeredObjectClassHandle,
          transportationNames,
          queueTimestampedAttributeUpdate ? nullptr : &acceptedAttributeValues) !=
      FederationRegistryStatus::applied) {
    return internalError(request);
  }

  std::vector<ProcessFederationAttributeUpdateEvent> events;
  std::uint32_t recipientCount = 0U;
  std::uint32_t queuedRecipientCount = 0U;
  for (auto const& passel : plan.passels) {
    for (auto const& plannedRecipient : passel.recipients) {
      if (queueTimestampedAttributeUpdate &&
          passel.preferredOrderType == rti1516_2025::TIMESTAMP &&
          timeConstrainedRecipients.contains(plannedRecipient.federateId)) {
        if (queuedRecipientCount != std::numeric_limits<std::uint32_t>::max()) {
          ++queuedRecipientCount;
        }
        continue;
      }
      auto recipient = plannedRecipient;
      if (recipient.receivedAttributeHandles.empty()) {
        auto const current = registry_.receiveOrderAttributeUpdateRecipientFor(
            updateRequest.federationName,
            updateRequest.producingFederateId,
            plannedRecipient.federateId,
            updateRequest.objectInstanceHandle,
            passel.sentAttributeHandles);
        if (!current) {
          continue;
        }
        recipient = *current;
      }

      ProcessFederationAttributeUpdateEvent event;
      event.producingFederateId = updateRequest.producingFederateId;
      event.receivingFederateId = recipient.federateId;
      event.objectInstanceHandle = updateRequest.objectInstanceHandle;
      event.userSuppliedTag = updateRequest.userSuppliedTag;
      event.transportationName = passel.transportationName;
      event.sentRegionHandles = passel.sentRegionHandles.empty()
          ? std::nullopt
          : std::optional<std::set<std::uint64_t>>(passel.sentRegionHandles);
      event.defaultRegionUsed = passel.defaultRegionUsed;
      event.timestamp = updateRequest.timestamp;
      if (queueTimestampedAttributeUpdate &&
          passel.preferredOrderType == rti1516_2025::TIMESTAMP) {
        event.retractionMessageId = messageId;
      }
      for (auto const attributeHandle : passel.sentAttributeHandles) {
        if (!recipient.receivedAttributeHandles.contains(attributeHandle)) {
          continue;
        }
        auto const value = std::find_if(
            updateRequest.attributeValues.begin(),
            updateRequest.attributeValues.end(),
            [attributeHandle](ProcessFederationAttributeValue const& candidate) {
              return candidate.first == attributeHandle;
            });
        if (value != updateRequest.attributeValues.end()) {
          event.attributeValues.push_back(*value);
        }
      }
      if (event.attributeValues.empty()) {
        continue;
      }
      events.push_back(std::move(event));
    }
  }

  if (queueTimestampedAttributeUpdate && options_.pushReceiveOrderEvents) {
    events.erase(
        std::remove_if(
            events.begin(),
            events.end(),
            [&](ProcessFederationAttributeUpdateEvent const& event) {
              return event.retractionMessageId &&
                  !registry_.beginTsoAttributeUpdateCallback(
                      updateRequest.federationName,
                      event.receivingFederateId,
                      *event.retractionMessageId);
            }),
        events.end());
  }

  std::vector<std::pair<ProcessTransportSession*, ProcessFederationAttributeUpdateEvent>>
      pushedEvents;
  {
    std::scoped_lock lock(mutex_);
    for (auto& event : events) {
        auto const receivingSession =
            sessionsByFederateId_.find(event.receivingFederateId);
        if (receivingSession == sessionsByFederateId_.end()) {
          continue;
        }
        auto const receivingState = sessions_.find(receivingSession->second);
        if (receivingState == sessions_.end()) {
          continue;
        }
        auto* const receivingSessionPointer = receivingSession->second;
        bool const hasRetraction = event.retractionMessageId.has_value();
      if (options_.pushReceiveOrderEvents) {
        pushedEvents.emplace_back(receivingSession->second, std::move(event));
      } else {
        receivingState->second.attributeUpdateEvents.push_back(std::move(event));
      }
      if (hasRetraction) {
        // The push frame crosses the process callback boundary below. Pull
        // mode records the same recipient now so a producer Retract can
        // suppress a queued event before the receiver polls it.
        pendingPushedRetractionRecipients_[messageId].push_back(
            receivingSessionPointer);
      }
      if (recipientCount != std::numeric_limits<std::uint32_t>::max()) {
        ++recipientCount;
      }
    }
  }

  if (options_.pushReceiveOrderEvents) {
    for (auto const& pushedEvent : pushedEvents) {
      auto const eventMessage = TransportServiceMessage{
          TransportServiceMessageKind::event,
          TransportServiceOperation::receive_attribute_update,
          TransportServiceStatus::ok,
          0U,
          encodeProcessFederationReceiveAttributeUpdateResult(
              ProcessFederationReceiveAttributeUpdateResult{pushedEvent.second})};
      if (pushedEvent.first == nullptr || !pushedEvent.first->send(eventMessage)) {
        return internalError(request);
      }
    }
  }
  if (queueTimestampedAttributeUpdate) {
    auto const totalCount = static_cast<std::uint64_t>(recipientCount) +
        queuedRecipientCount;
    recipientCount = static_cast<std::uint32_t>(std::min<std::uint64_t>(
        totalCount,
        std::numeric_limits<std::uint32_t>::max()));
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationUpdateAttributeValuesResult(
          ProcessFederationUpdateAttributeValuesResult{recipientCount, messageId}));
}

TransportServiceMessage
ProcessFederationService::handleRequestAttributeValueUpdate(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const requestValue =
      decodeProcessFederationRequestAttributeValueUpdateRequest(request.payload);
  std::set<std::uint64_t> requestedAttributeHandles(
      requestValue.requestedAttributeHandles.begin(),
      requestValue.requestedAttributeHandles.end());
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != requestValue.federationName ||
        state->second.federateId != requestValue.requestingFederateId ||
        !registry_.memberById(
            requestValue.federationName,
            requestValue.requestingFederateId)) {
      return rejected(request);
    }
  }

  auto const plan = registry_.planAttributeValueUpdateRequest(
      requestValue.federationName,
      requestValue.requestingFederateId,
      requestValue.objectInstanceHandle,
      requestedAttributeHandles);
  if (plan.status != AttributeValueUpdateRequestStatus::applied) {
    return rejected(request);
  }

  std::vector<std::pair<ProcessTransportSession*,
                        ProcessFederationAttributeValueUpdateRequestEvent>>
      pushedEvents;
  std::uint32_t recipientCount = 0U;
  for (auto const& planned : plan.recipients) {
    ProcessTransportSession* providingSession = nullptr;
    {
      std::scoped_lock lock(mutex_);
      auto const found = sessionsByFederateId_.find(planned.providingFederateId);
      if (found != sessionsByFederateId_.end()) {
        auto const state = sessions_.find(found->second);
        if (state != sessions_.end() &&
            state->second.federationName.has_value() &&
            *state->second.federationName == requestValue.federationName &&
            state->second.federateId == planned.providingFederateId) {
          providingSession = found->second;
        }
      }
    }
    if (providingSession == nullptr) {
      continue;
    }

    // Persist and consume the private request at the process delivery
    // boundary. The process callback bridge has no registry reference, so the
    // service carries the already revalidated attribute subset in the event.
    auto const requestId = registry_.registerAttributeValueUpdateRequest(
        requestValue.federationName,
        requestValue.requestingFederateId,
        planned.providingFederateId,
        planned.objectInstanceHandle,
        planned.requestedAttributeHandles,
        requestValue.userSuppliedTag);
    if (!requestId) {
      return internalError(request);
    }
    auto const projection = registry_.beginAttributeValueUpdateProvideRecipientFor(
        requestValue.federationName,
        *requestId,
        requestValue.requestingFederateId,
        planned.providingFederateId,
        planned.objectInstanceHandle,
        planned.requestedAttributeHandles);
    if (!projection) {
      continue;
    }
    ProcessFederationAttributeValueUpdateRequestEvent event{
        requestValue.requestingFederateId,
        planned.providingFederateId,
        projection->objectInstanceHandle,
        projection->requestedAttributeHandles,
        requestValue.userSuppliedTag};
    if (options_.pushReceiveOrderEvents) {
      pushedEvents.emplace_back(providingSession, std::move(event));
    } else {
      std::scoped_lock lock(mutex_);
      auto const state = sessions_.find(providingSession);
      if (state != sessions_.end() &&
          state->second.federationName.has_value() &&
          *state->second.federationName == requestValue.federationName &&
          state->second.federateId == planned.providingFederateId) {
        state->second.attributeValueUpdateRequestEvents.push_back(
            std::move(event));
      } else {
        continue;
      }
    }
    if (recipientCount != std::numeric_limits<std::uint32_t>::max()) {
      ++recipientCount;
    }
  }

  if (options_.pushReceiveOrderEvents) {
    for (auto& pushedEvent : pushedEvents) {
      ProcessFederationReceiveInteractionResult result;
      result.attributeValueUpdateRequestEvent = std::move(pushedEvent.second);
      if (pushedEvent.first == nullptr ||
          !pushedEvent.first->send(TransportServiceMessage{
              TransportServiceMessageKind::event,
              TransportServiceOperation::receive_interaction,
              TransportServiceStatus::ok,
              0U,
              encodeProcessFederationReceiveInteractionResult(result)})) {
        return internalError(request);
      }
    }
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationRequestAttributeValueUpdateResult(
          ProcessFederationRequestAttributeValueUpdateResult{recipientCount}));
}

TransportServiceMessage
ProcessFederationService::handleRequestAttributeValueUpdateClass(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const requestValue =
      decodeProcessFederationRequestAttributeValueUpdateClassRequest(
          request.payload);
  std::set<std::uint64_t> requestedAttributeHandles(
      requestValue.requestedAttributeHandles.begin(),
      requestValue.requestedAttributeHandles.end());
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != requestValue.federationName ||
        state->second.federateId != requestValue.requestingFederateId ||
        !registry_.memberById(
            requestValue.federationName,
            requestValue.requestingFederateId)) {
      return rejected(request);
    }
  }

  auto const plan = registry_.planAttributeValueUpdateClassRequest(
      requestValue.federationName,
      requestValue.requestingFederateId,
      requestValue.objectClassHandle,
      requestedAttributeHandles);
  if (plan.status != AttributeValueUpdateClassRequestStatus::applied) {
    return rejected(request);
  }

  std::vector<std::pair<ProcessTransportSession*,
                        ProcessFederationAttributeValueUpdateRequestEvent>>
      pushedEvents;
  std::uint32_t recipientCount = 0U;
  for (auto const& planned : plan.recipients) {
    ProcessTransportSession* providingSession = nullptr;
    {
      std::scoped_lock lock(mutex_);
      auto const found =
          sessionsByFederateId_.find(planned.providingFederateId);
      if (found != sessionsByFederateId_.end()) {
        auto const state = sessions_.find(found->second);
        if (state != sessions_.end() &&
            state->second.federationName.has_value() &&
            *state->second.federationName == requestValue.federationName &&
            state->second.federateId == planned.providingFederateId) {
          providingSession = found->second;
        }
      }
    }
    if (providingSession == nullptr) {
      continue;
    }

    auto const requestId = registry_.registerAttributeValueUpdateClassRequest(
        requestValue.federationName,
        requestValue.requestingFederateId,
        planned.providingFederateId,
        planned.objectInstanceHandle,
        requestValue.objectClassHandle,
        planned.requestedAttributeHandles,
        requestValue.userSuppliedTag);
    if (!requestId) {
      return internalError(request);
    }
    auto const projection =
        registry_.beginAttributeValueUpdateClassProvideRecipientFor(
            requestValue.federationName,
            *requestId,
            requestValue.requestingFederateId,
            planned.providingFederateId,
            planned.objectInstanceHandle,
            requestValue.objectClassHandle,
            planned.requestedAttributeHandles);
    if (!projection) {
      continue;
    }
    ProcessFederationAttributeValueUpdateRequestEvent event{
        requestValue.requestingFederateId,
        planned.providingFederateId,
        projection->objectInstanceHandle,
        projection->requestedAttributeHandles,
        requestValue.userSuppliedTag};
    if (options_.pushReceiveOrderEvents) {
      pushedEvents.emplace_back(providingSession, std::move(event));
    } else {
      std::scoped_lock lock(mutex_);
      auto const state = sessions_.find(providingSession);
      if (state != sessions_.end() &&
          state->second.federationName.has_value() &&
          *state->second.federationName == requestValue.federationName &&
          state->second.federateId == planned.providingFederateId) {
        state->second.attributeValueUpdateRequestEvents.push_back(
            std::move(event));
      } else {
        continue;
      }
    }
    if (recipientCount != std::numeric_limits<std::uint32_t>::max()) {
      ++recipientCount;
    }
  }

  if (options_.pushReceiveOrderEvents) {
    for (auto& pushedEvent : pushedEvents) {
      ProcessFederationReceiveInteractionResult result;
      result.attributeValueUpdateRequestEvent = std::move(pushedEvent.second);
      if (pushedEvent.first == nullptr ||
          !pushedEvent.first->send(TransportServiceMessage{
              TransportServiceMessageKind::event,
              TransportServiceOperation::receive_interaction,
              TransportServiceStatus::ok,
              0U,
              encodeProcessFederationReceiveInteractionResult(result)})) {
        return internalError(request);
      }
    }
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationRequestAttributeValueUpdateResult(
          ProcessFederationRequestAttributeValueUpdateResult{recipientCount}));
}

TransportServiceMessage
ProcessFederationService::handleRequestAttributeValueUpdateClassWithRegions(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const requestValue =
      decodeProcessFederationRequestAttributeValueUpdateClassWithRegionsRequest(
          request.payload);
  std::set<std::uint64_t> requestedAttributeHandles(
      requestValue.requestedAttributeHandles.begin(),
      requestValue.requestedAttributeHandles.end());
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != requestValue.federationName ||
        state->second.federateId != requestValue.requestingFederateId ||
        !registry_.memberById(
            requestValue.federationName, requestValue.requestingFederateId)) {
      return rejected(request);
    }
  }

  auto const plan = registry_.planAttributeValueUpdateClassRequest(
      requestValue.federationName,
      requestValue.requestingFederateId,
      requestValue.objectClassHandle,
      requestedAttributeHandles,
      &requestValue.requestRegionsByAttribute);
  if (plan.status != AttributeValueUpdateClassRequestStatus::applied) {
    return rejected(request);
  }

  std::vector<std::pair<ProcessTransportSession*,
                        ProcessFederationAttributeValueUpdateRequestEvent>>
      pushedEvents;
  std::uint32_t recipientCount = 0U;
  for (auto const& planned : plan.recipients) {
    ProcessTransportSession* providingSession = nullptr;
    {
      std::scoped_lock lock(mutex_);
      auto const found =
          sessionsByFederateId_.find(planned.providingFederateId);
      if (found != sessionsByFederateId_.end()) {
        auto const state = sessions_.find(found->second);
        if (state != sessions_.end() &&
            state->second.federationName.has_value() &&
            *state->second.federationName == requestValue.federationName &&
            state->second.federateId == planned.providingFederateId) {
          providingSession = found->second;
        }
      }
    }
    if (providingSession == nullptr) {
      continue;
    }

    // Each provider group may own only a subset of the requested attributes;
    // persist the matching region pairs rather than the requester's full map.
    std::map<std::uint64_t, std::set<std::uint64_t>> providerRegions;
    for (std::uint64_t const attributeHandle :
         planned.requestedAttributeHandles) {
      auto const region = requestValue.requestRegionsByAttribute.find(
          attributeHandle);
      if (region == requestValue.requestRegionsByAttribute.end()) {
        return internalError(request);
      }
      providerRegions.emplace(attributeHandle, region->second);
    }
    auto const requestId = registry_.registerAttributeValueUpdateRegionalRequest(
        requestValue.federationName,
        requestValue.requestingFederateId,
        planned.providingFederateId,
        planned.objectInstanceHandle,
        requestValue.objectClassHandle,
        planned.requestedAttributeHandles,
        providerRegions,
        requestValue.userSuppliedTag);
    if (!requestId) {
      return internalError(request);
    }
    auto const projection =
        registry_.beginAttributeValueUpdateRegionalProvideRecipientFor(
            requestValue.federationName,
            *requestId,
            requestValue.requestingFederateId,
            planned.providingFederateId,
            planned.objectInstanceHandle,
            requestValue.objectClassHandle,
            planned.requestedAttributeHandles,
            providerRegions);
    if (!projection) {
      continue;
    }
    ProcessFederationAttributeValueUpdateRequestEvent event{
        requestValue.requestingFederateId,
        planned.providingFederateId,
        projection->objectInstanceHandle,
        projection->requestedAttributeHandles,
        requestValue.userSuppliedTag};
    if (options_.pushReceiveOrderEvents) {
      pushedEvents.emplace_back(providingSession, std::move(event));
    } else {
      std::scoped_lock lock(mutex_);
      auto const state = sessions_.find(providingSession);
      if (state != sessions_.end() &&
          state->second.federationName.has_value() &&
          *state->second.federationName == requestValue.federationName &&
          state->second.federateId == planned.providingFederateId) {
        state->second.attributeValueUpdateRequestEvents.push_back(
            std::move(event));
      } else {
        continue;
      }
    }
    if (recipientCount != std::numeric_limits<std::uint32_t>::max()) {
      ++recipientCount;
    }
  }

  if (options_.pushReceiveOrderEvents) {
    for (auto& pushedEvent : pushedEvents) {
      ProcessFederationReceiveInteractionResult result;
      result.attributeValueUpdateRequestEvent = std::move(pushedEvent.second);
      if (pushedEvent.first == nullptr ||
          !pushedEvent.first->send(TransportServiceMessage{
              TransportServiceMessageKind::event,
              TransportServiceOperation::receive_interaction,
              TransportServiceStatus::ok,
              0U,
              encodeProcessFederationReceiveInteractionResult(result)})) {
        return internalError(request);
      }
    }
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationRequestAttributeValueUpdateResult(
          ProcessFederationRequestAttributeValueUpdateResult{recipientCount}));
}

TransportServiceMessage ProcessFederationService::handleAttributeOwnershipCheck(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const checkRequest =
      decodeProcessFederationAttributeOwnershipCheckRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != checkRequest.federationName ||
        state->second.federateId != checkRequest.requestingFederateId ||
        !registry_.memberById(
            checkRequest.federationName,
            checkRequest.requestingFederateId)) {
      return rejected(request);
    }
  }

  auto const result = registry_.attributeOwnedByFederate(
      checkRequest.federationName,
      checkRequest.requestingFederateId,
      checkRequest.objectInstanceHandle,
      checkRequest.attributeHandle);
  ProcessFederationAttributeOwnershipCheckResult processResult{
      result.status,
      result.ownedByRequestingFederate};
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationAttributeOwnershipCheckResult(processResult));
}

TransportServiceMessage ProcessFederationService::handleQueryAttributeOwnership(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const queryRequest =
      decodeProcessFederationAttributeOwnershipQueryRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != queryRequest.federationName ||
        state->second.federateId != queryRequest.requestingFederateId ||
        !registry_.memberById(
            queryRequest.federationName,
            queryRequest.requestingFederateId)) {
      return rejected(request);
    }
  }

  auto const plan = registry_.planAttributeOwnershipQuery(
      queryRequest.federationName,
      queryRequest.requestingFederateId,
      queryRequest.objectInstanceHandle,
      std::set<std::uint64_t>(
          queryRequest.requestedAttributeHandles.begin(),
          queryRequest.requestedAttributeHandles.end()));
  if (plan.status != AttributeOwnershipQueryStatus::applied) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationAttributeOwnershipQueryResult(
            ProcessFederationAttributeOwnershipQueryResult{plan.status, 0U}));
  }

  std::vector<
      std::pair<ProcessTransportSession*,
                ProcessFederationAttributeOwnershipQueryEvent>> pushedEvents;
  std::uint32_t recipientCount = 0U;
  for (auto const& planned : plan.recipients) {
    if (planned.receivingFederateId != queryRequest.requestingFederateId ||
        planned.requestId == 0U || planned.attributeHandles.empty()) {
      return internalError(request);
    }
    ProcessTransportSession* receivingSession = nullptr;
    {
      std::scoped_lock lock(mutex_);
      auto const found =
          sessionsByFederateId_.find(planned.receivingFederateId);
      if (found != sessionsByFederateId_.end()) {
        auto const state = sessions_.find(found->second);
        if (state != sessions_.end() &&
            state->second.federationName.has_value() &&
            *state->second.federationName == queryRequest.federationName &&
            state->second.federateId == planned.receivingFederateId) {
          receivingSession = found->second;
        }
      }
      if (receivingSession == nullptr) {
        return internalError(request);
      }
      ProcessFederationAttributeOwnershipQueryEvent event{
          planned.requestId,
          planned.receivingFederateId,
          planned.objectInstanceHandle,
          planned.reportKind,
          planned.owningFederateId,
          planned.attributeHandles};
      if (options_.pushReceiveOrderEvents) {
        pushedEvents.emplace_back(receivingSession, std::move(event));
      } else {
        auto const state = sessions_.find(receivingSession);
        if (state == sessions_.end()) {
          return internalError(request);
        }
        state->second.attributeOwnershipQueryEvents.push_back(
            std::move(event));
      }
    }
    if (recipientCount != std::numeric_limits<std::uint32_t>::max()) {
      ++recipientCount;
    }
  }

  if (options_.pushReceiveOrderEvents) {
    for (auto& pushedEvent : pushedEvents) {
      ProcessFederationReceiveInteractionResult result;
      result.attributeOwnershipQueryEvent = std::move(pushedEvent.second);
      if (pushedEvent.first == nullptr ||
          !pushedEvent.first->send(TransportServiceMessage{
              TransportServiceMessageKind::event,
              TransportServiceOperation::receive_interaction,
              TransportServiceStatus::ok,
              0U,
              encodeProcessFederationReceiveInteractionResult(result)})) {
        return internalError(request);
      }
    }
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationAttributeOwnershipQueryResult(
          ProcessFederationAttributeOwnershipQueryResult{
              AttributeOwnershipQueryStatus::applied,
              recipientCount}));
}

TransportServiceMessage
ProcessFederationService::handleAttributeOwnershipAcquisition(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const acquisitionRequest =
      decodeProcessFederationAttributeOwnershipAcquisitionRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != acquisitionRequest.federationName ||
        state->second.federateId != acquisitionRequest.requestingFederateId ||
        !registry_.memberById(
            acquisitionRequest.federationName,
            acquisitionRequest.requestingFederateId)) {
      return rejected(request);
    }
  }

  auto plan = registry_.planAttributeOwnershipAcquisition(
      acquisitionRequest.federationName,
      acquisitionRequest.requestingFederateId,
      acquisitionRequest.objectInstanceHandle,
      std::set<std::uint64_t>(
          acquisitionRequest.desiredAttributeHandles.begin(),
          acquisitionRequest.desiredAttributeHandles.end()),
      acquisitionRequest.userSuppliedTag);
  if (plan.status != AttributeOwnershipAcquisitionStatus::applied) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationAttributeOwnershipAcquisitionResult(
            ProcessFederationAttributeOwnershipAcquisitionResult{
                plan.status,
                0U}));
  }
  auto const recipientCount = static_cast<std::uint32_t>(std::min<std::size_t>(
      plan.workItems.size(), std::numeric_limits<std::uint32_t>::max()));
  if (!plan.workItems.empty() &&
      !enqueueAttributeOwnershipAcquisitionWorkItems(
          acquisitionRequest.federationName,
          std::move(plan.workItems))) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationAttributeOwnershipAcquisitionResult(
          ProcessFederationAttributeOwnershipAcquisitionResult{
              AttributeOwnershipAcquisitionStatus::applied,
              recipientCount}));
}

TransportServiceMessage
ProcessFederationService::handleAttributeOwnershipAcquisitionIfAvailable(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const acquisitionRequest =
      decodeProcessFederationAttributeOwnershipAcquisitionIfAvailableRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != acquisitionRequest.federationName ||
        state->second.federateId != acquisitionRequest.requestingFederateId ||
        !registry_.memberById(
            acquisitionRequest.federationName,
            acquisitionRequest.requestingFederateId)) {
      return rejected(request);
    }
  }

  auto const plan = registry_.planAttributeOwnershipAcquisitionIfAvailable(
      acquisitionRequest.federationName,
      acquisitionRequest.requestingFederateId,
      acquisitionRequest.objectInstanceHandle,
      std::set<std::uint64_t>(
          acquisitionRequest.desiredAttributeHandles.begin(),
          acquisitionRequest.desiredAttributeHandles.end()),
      acquisitionRequest.userSuppliedTag);
  if (plan.status != AttributeOwnershipAcquisitionIfAvailableStatus::applied) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationAttributeOwnershipAcquisitionIfAvailableResult(
            ProcessFederationAttributeOwnershipAcquisitionIfAvailableResult{
                plan.status,
                0U}));
  }
  if (plan.requestId == 0U) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationAttributeOwnershipAcquisitionIfAvailableResult(
            ProcessFederationAttributeOwnershipAcquisitionIfAvailableResult{
                AttributeOwnershipAcquisitionIfAvailableStatus::applied,
                0U}));
  }

  ProcessFederationAttributeOwnershipAcquisitionIfAvailableEvent event{
      plan.requestId,
      acquisitionRequest.requestingFederateId,
      acquisitionRequest.objectInstanceHandle,
      {},
      {},
      acquisitionRequest.userSuppliedTag};
  if (options_.pushReceiveOrderEvents) {
    auto const delivery = registry_.beginAttributeOwnershipAcquisitionIfAvailable(
        acquisitionRequest.federationName,
        acquisitionRequest.requestingFederateId,
        acquisitionRequest.objectInstanceHandle,
        plan.requestId);
    if (!delivery) {
      return internalError(request);
    }
    event.securedAttributeHandles = delivery->securedAttributeHandles;
    event.unavailableAttributeHandles = delivery->unavailableAttributeHandles;
    ProcessFederationReceiveInteractionResult pushedResult;
    pushedResult.attributeOwnershipAcquisitionIfAvailableEvent =
        std::move(event);
    if (!session.send(TransportServiceMessage{
            TransportServiceMessageKind::event,
            TransportServiceOperation::receive_interaction,
            TransportServiceStatus::ok,
            0U,
            encodeProcessFederationReceiveInteractionResult(pushedResult)})) {
      return internalError(request);
    }
  } else {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != acquisitionRequest.federationName ||
        state->second.federateId != acquisitionRequest.requestingFederateId) {
      registry_.cancelAttributeOwnershipAcquisitionIfAvailable(
          acquisitionRequest.federationName,
          acquisitionRequest.requestingFederateId,
          acquisitionRequest.objectInstanceHandle,
          plan.requestId);
      return rejected(request);
    }
    state->second.attributeOwnershipAcquisitionIfAvailableEvents.push_back(
        std::move(event));
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationAttributeOwnershipAcquisitionIfAvailableResult(
      ProcessFederationAttributeOwnershipAcquisitionIfAvailableResult{
              AttributeOwnershipAcquisitionIfAvailableStatus::applied,
              1U}));
}

TransportServiceMessage
ProcessFederationService::handleAttributeOwnershipReleaseDenied(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const denialRequest =
      decodeProcessFederationAttributeOwnershipReleaseDeniedRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != denialRequest.federationName ||
        state->second.federateId != denialRequest.owningFederateId ||
        !registry_.memberById(
            denialRequest.federationName,
            denialRequest.owningFederateId)) {
      return rejected(request);
    }
  }

  auto plan = registry_.planAttributeOwnershipReleaseDenied(
      denialRequest.federationName,
      denialRequest.owningFederateId,
      denialRequest.objectInstanceHandle,
      std::set<std::uint64_t>(
          denialRequest.attributeHandles.begin(),
          denialRequest.attributeHandles.end()));
  if (plan.status != AttributeOwnershipReleaseDeniedStatus::applied) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationAttributeOwnershipReleaseDeniedResult(
            ProcessFederationAttributeOwnershipReleaseDeniedResult{
                plan.status,
                0U}));
  }

  auto const recipientCount = static_cast<std::uint32_t>(std::min<std::size_t>(
      plan.recipients.size(), std::numeric_limits<std::uint32_t>::max()));
  if (!plan.recipients.empty() &&
      !enqueueAttributeOwnershipUnavailableRecipients(
          denialRequest.federationName,
          std::move(plan.recipients),
          denialRequest.userSuppliedTag)) {
    return internalError(request);
  }
  if (!plan.followupWorkItems.empty() &&
      !enqueueAttributeOwnershipAcquisitionWorkItems(
          denialRequest.federationName,
          std::move(plan.followupWorkItems))) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationAttributeOwnershipReleaseDeniedResult(
          ProcessFederationAttributeOwnershipReleaseDeniedResult{
              AttributeOwnershipReleaseDeniedStatus::applied,
              recipientCount}));
}

TransportServiceMessage
ProcessFederationService::handleAttributeOwnershipAcquisitionCancellation(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const cancellationRequest =
      decodeProcessFederationAttributeOwnershipAcquisitionCancellationRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != cancellationRequest.federationName ||
        state->second.federateId != cancellationRequest.requestingFederateId ||
        !registry_.memberById(
            cancellationRequest.federationName,
            cancellationRequest.requestingFederateId)) {
      return rejected(request);
    }
  }

  auto plan = registry_.planAttributeOwnershipAcquisitionCancellation(
      cancellationRequest.federationName,
      cancellationRequest.requestingFederateId,
      cancellationRequest.objectInstanceHandle,
      std::set<std::uint64_t>(
          cancellationRequest.attributeHandles.begin(),
          cancellationRequest.attributeHandles.end()));
  if (plan.status !=
      AttributeOwnershipAcquisitionCancellationStatus::applied) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationAttributeOwnershipAcquisitionCancellationResult(
            ProcessFederationAttributeOwnershipAcquisitionCancellationResult{
                plan.status,
                0U}));
  }
  // A supplied-empty cancellation is successful but has no callback
  // reservation. Preserve that distinction in the typed result.
  if (plan.cancellationId == 0U || plan.attributeHandles.empty()) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationAttributeOwnershipAcquisitionCancellationResult(
            ProcessFederationAttributeOwnershipAcquisitionCancellationResult{
                AttributeOwnershipAcquisitionCancellationStatus::applied,
                0U}));
  }

  ProcessFederationAttributeOwnershipAcquisitionEvent event{
      ProcessFederationAttributeOwnershipAcquisitionEventKind::
          cancellation_confirmation,
      plan.cancellationId,
      cancellationRequest.requestingFederateId,
      cancellationRequest.requestingFederateId,
      cancellationRequest.objectInstanceHandle,
      plan.attributeHandles,
      {}};
  std::uint32_t recipientCount = 0U;
  if (options_.pushReceiveOrderEvents) {
    // Pushed process events cross the callback fence here, before the
    // unsolicited payload is sent to the client. Pull mode performs the same
    // transition in handleReceiveInteraction below.
    auto const delivery = registry_.beginAttributeOwnershipAcquisitionCancellation(
        cancellationRequest.federationName,
        cancellationRequest.requestingFederateId,
        cancellationRequest.objectInstanceHandle,
        plan.cancellationId,
        plan.attributeHandles);
    if (delivery && !delivery->confirmedAttributeHandles.empty()) {
      event.attributeHandles = delivery->confirmedAttributeHandles;
      ProcessFederationReceiveInteractionResult pushedResult;
      pushedResult.attributeOwnershipAcquisitionEvent = event;
      if (!session.send(TransportServiceMessage{
              TransportServiceMessageKind::event,
              TransportServiceOperation::receive_interaction,
              TransportServiceStatus::ok,
              0U,
              encodeProcessFederationReceiveInteractionResult(pushedResult)})) {
        return internalError(request);
      }
      recipientCount = 1U;
      if (!delivery->followupWorkItems.empty() &&
          !enqueueAttributeOwnershipAcquisitionWorkItems(
              cancellationRequest.federationName,
              std::move(delivery->followupWorkItems))) {
        return internalError(request);
      }
    }
  } else {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != cancellationRequest.federationName ||
        state->second.federateId != cancellationRequest.requestingFederateId) {
      return rejected(request);
    }
    state->second.attributeOwnershipAcquisitionEvents.push_back(
        std::move(event));
    recipientCount = 1U;
  }

  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationAttributeOwnershipAcquisitionCancellationResult(
          ProcessFederationAttributeOwnershipAcquisitionCancellationResult{
              AttributeOwnershipAcquisitionCancellationStatus::applied,
              recipientCount}));
}

TransportServiceMessage
ProcessFederationService::handleCancelNegotiatedAttributeOwnershipDivestiture(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const cancellationRequest =
      decodeProcessFederationCancelNegotiatedAttributeOwnershipDivestitureRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != cancellationRequest.federationName ||
        state->second.federateId != cancellationRequest.divestingFederateId ||
        !registry_.memberById(
            cancellationRequest.federationName,
            cancellationRequest.divestingFederateId)) {
      return rejected(request);
    }
  }

  auto plan = registry_.planCancelNegotiatedAttributeOwnershipDivestiture(
      cancellationRequest.federationName,
      cancellationRequest.divestingFederateId,
      cancellationRequest.objectInstanceHandle,
      std::set<std::uint64_t>(
          cancellationRequest.attributeHandles.begin(),
          cancellationRequest.attributeHandles.end()));
  if (plan.status !=
      CancelNegotiatedAttributeOwnershipDivestitureStatus::applied) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationCancelNegotiatedAttributeOwnershipDivestitureResult(
            ProcessFederationCancelNegotiatedAttributeOwnershipDivestitureResult{
                plan.status,
                0U}));
  }

  auto const recipientCount = static_cast<std::uint32_t>(std::min<std::size_t>(
      plan.followupWorkItems.size(), std::numeric_limits<std::uint32_t>::max()));
  if (!plan.followupWorkItems.empty() &&
      !enqueueAttributeOwnershipAcquisitionWorkItems(
          cancellationRequest.federationName,
          std::move(plan.followupWorkItems))) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationCancelNegotiatedAttributeOwnershipDivestitureResult(
          ProcessFederationCancelNegotiatedAttributeOwnershipDivestitureResult{
              CancelNegotiatedAttributeOwnershipDivestitureStatus::applied,
              recipientCount}));
}

TransportServiceMessage
ProcessFederationService::handleNegotiatedAttributeOwnershipDivestiture(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const divestitureRequest =
      decodeProcessFederationNegotiatedAttributeOwnershipDivestitureRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != divestitureRequest.federationName ||
        state->second.federateId != divestitureRequest.divestingFederateId ||
        !registry_.memberById(
            divestitureRequest.federationName,
            divestitureRequest.divestingFederateId)) {
      return rejected(request);
    }
  }

  auto plan = registry_.planNegotiatedAttributeOwnershipDivestiture(
      divestitureRequest.federationName,
      divestitureRequest.divestingFederateId,
      divestitureRequest.objectInstanceHandle,
      std::set<std::uint64_t>(
          divestitureRequest.attributeHandles.begin(),
          divestitureRequest.attributeHandles.end()),
      divestitureRequest.userSuppliedTag);
  if (plan.status !=
      NegotiatedAttributeOwnershipDivestitureStatus::applied) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationNegotiatedAttributeOwnershipDivestitureResult(
            ProcessFederationNegotiatedAttributeOwnershipDivestitureResult{
                plan.status,
                0U}));
  }

  auto const workItemCount = plan.workItems.size();
  auto const assumptionCount = plan.assumptionRecipients.size();
  auto const recipientCount = static_cast<std::uint32_t>(std::min<std::size_t>(
      workItemCount + assumptionCount,
      std::numeric_limits<std::uint32_t>::max()));
  if (!plan.workItems.empty() &&
      !enqueueAttributeOwnershipAcquisitionWorkItems(
          divestitureRequest.federationName,
          std::move(plan.workItems))) {
    return internalError(request);
  }
  if (!plan.assumptionRecipients.empty() &&
      !enqueueAttributeOwnershipAssumptionRecipients(
          divestitureRequest.federationName,
          std::move(plan.assumptionRecipients))) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationNegotiatedAttributeOwnershipDivestitureResult(
          ProcessFederationNegotiatedAttributeOwnershipDivestitureResult{
              NegotiatedAttributeOwnershipDivestitureStatus::applied,
              recipientCount}));
}

TransportServiceMessage ProcessFederationService::handleConfirmDivestiture(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const confirmRequest =
      decodeProcessFederationConfirmDivestitureRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != confirmRequest.federationName ||
        state->second.federateId != confirmRequest.divestingFederateId ||
        !registry_.memberById(
            confirmRequest.federationName,
            confirmRequest.divestingFederateId)) {
      return rejected(request);
    }
  }

  auto plan = registry_.planConfirmDivestiture(
      confirmRequest.federationName,
      confirmRequest.divestingFederateId,
      confirmRequest.objectInstanceHandle,
      std::set<std::uint64_t>(
          confirmRequest.attributeHandles.begin(),
          confirmRequest.attributeHandles.end()),
      confirmRequest.userSuppliedTag);
  if (plan.status != ConfirmDivestitureStatus::applied) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationConfirmDivestitureResult(
            ProcessFederationConfirmDivestitureResult{plan.status, 0U}));
  }

  auto const recipientCount = static_cast<std::uint32_t>(std::min<std::size_t>(
      plan.notifications.size(), std::numeric_limits<std::uint32_t>::max()));
  if (!plan.notifications.empty() &&
      !enqueueConfirmDivestitureNotifications(
          confirmRequest.federationName, std::move(plan.notifications))) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationConfirmDivestitureResult(
          ProcessFederationConfirmDivestitureResult{
              ConfirmDivestitureStatus::applied,
              recipientCount}));
}

TransportServiceMessage
ProcessFederationService::handleUnconditionalAttributeOwnershipDivestiture(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  // The private request shape is shared with regular acquisition: both
  // services carry one federation/member/object/attribute-set/tag tuple. The
  // operation identity keeps the state transition distinct at the service
  // boundary.
  auto const divestitureRequest =
      decodeProcessFederationAttributeOwnershipAcquisitionRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != divestitureRequest.federationName ||
        state->second.federateId != divestitureRequest.requestingFederateId ||
        !registry_.memberById(
            divestitureRequest.federationName,
            divestitureRequest.requestingFederateId)) {
      return rejected(request);
    }
  }

  auto const plan = registry_.planUnconditionalAttributeOwnershipDivestiture(
      divestitureRequest.federationName,
      divestitureRequest.requestingFederateId,
      divestitureRequest.objectInstanceHandle,
      std::set<std::uint64_t>(
          divestitureRequest.desiredAttributeHandles.begin(),
          divestitureRequest.desiredAttributeHandles.end()),
      divestitureRequest.userSuppliedTag);
  if (plan.status !=
      UnconditionalAttributeOwnershipDivestitureStatus::applied) {
    return rejected(request);
  }

  if (!plan.acquisitionWorkItems.empty() &&
      !enqueueAttributeOwnershipAcquisitionWorkItems(
          divestitureRequest.federationName,
          std::move(plan.acquisitionWorkItems))) {
    return internalError(request);
  }
  if (!plan.assumptionRecipients.empty() &&
      !enqueueAttributeOwnershipAssumptionRecipients(
          divestitureRequest.federationName,
          std::move(plan.assumptionRecipients),
          !divestitureRequest.callbacksEnabled)) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationBooleanResult(
          ProcessFederationBooleanResult{true}));
}

TransportServiceMessage ProcessFederationService::handleSendInteraction(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const sendRequest = decodeProcessFederationSendInteractionRequest(request.payload);
  if (request.operation == TransportServiceOperation::send_interaction_with_regions &&
      !sendRequest.sentRegionHandles.has_value()) {
    return invalid(request);
  }
  if (request.operation == TransportServiceOperation::send_interaction &&
      sendRequest.sentRegionHandles.has_value()) {
    return invalid(request);
  }
  if (sendRequest.timestamp) {
    validateProcessLogicalTime(
        *sendRequest.timestamp,
        federationDefinition_->logicalTimeImplementationName);
  }
  std::shared_ptr<FederateTimeState> producingTimeState;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != sendRequest.federationName ||
        state->second.federateId != sendRequest.producingFederateId ||
        !registry_.memberById(
            sendRequest.federationName,
            sendRequest.producingFederateId)) {
      return rejected(request);
    }
    producingTimeState = state->second.timeState;
  }

  auto const plan = registry_.planReceiveOrderInteraction(
      sendRequest.federationName,
      sendRequest.producingFederateId,
      sendRequest.interactionClassHandle,
      sendRequest.sentParameterHandles,
      sendRequest.sentRegionHandles
          ? &*sendRequest.sentRegionHandles
          : nullptr);
  if (plan.status != ReceiveOrderInteractionStatus::applied) {
    return rejected(request);
  }

  auto const producingTimeSnapshot = producingTimeState
      ? producingTimeState->snapshot()
      : FederateTimeSnapshot{};
  bool queueTimestampedInteraction =
      sendRequest.timestamp && producingTimeSnapshot.timeRegulating &&
      plan.preferredOrderType == rti1516_2025::TIMESTAMP;
  std::set<std::uint64_t> timeConstrainedRecipients;
  if (queueTimestampedInteraction) {
    auto const execution = registry_.timeSnapshotFor(sendRequest.federationName);
    if (!execution) {
      return internalError(request);
    }
    for (auto const& federate : execution->federates) {
      if (federate.time.timeConstrained) {
        timeConstrainedRecipients.insert(federate.membership.id);
      }
    }
  }

  std::uint64_t messageId = 0U;
  if (queueTimestampedInteraction) {
    auto const envelope =
        decodeProcessFederationInteractionEnvelope(sendRequest.payload);
    if (!envelope && !sendRequest.sentParameterHandles.empty()) {
      return rejected(request);
    }

    TsoInteractionMessage message;
    message.producingFederateId = sendRequest.producingFederateId;
    message.sentInteractionClassHandle = sendRequest.interactionClassHandle;
    message.sentParameterHandles = sendRequest.sentParameterHandles;
    message.transportationName = plan.transportationName;
    if (sendRequest.sentRegionHandles) {
      message.sentRegionHandles = *sendRequest.sentRegionHandles;
    }
    message.sentRegionSnapshots = plan.sentRegionSnapshots;
    message.defaultRegionUsed = plan.defaultRegionUsed;
    message.timestamp = decodeProcessLogicalTime(
        *sendRequest.timestamp,
        federationDefinition_->logicalTimeImplementationName);
    message.sentOrderType = rti1516_2025::TIMESTAMP;
    message.receivedOrderType = rti1516_2025::TIMESTAMP;
    if (envelope) {
      if (!envelope->userSuppliedTag.empty()) {
        message.userSuppliedTag.setData(
            envelope->userSuppliedTag.data(),
            envelope->userSuppliedTag.size());
      }
      for (std::uint64_t const parameterHandle : sendRequest.sentParameterHandles) {
        auto const found = std::find_if(
            envelope->parameterValues.begin(),
            envelope->parameterValues.end(),
            [parameterHandle](
                ProcessFederationInteractionParameterValue const& value) {
              return value.first == parameterHandle;
            });
        if (found == envelope->parameterValues.end()) {
          return rejected(request);
        }
        rti1516_2025::VariableLengthData value;
        if (!found->second.empty()) {
          value.setData(found->second.data(), found->second.size());
        }
        message.parameters.emplace_back(parameterHandle, std::move(value));
      }
    } else {
      message.userSuppliedTag.setData(
          sendRequest.payload.data(),
          sendRequest.payload.size());
    }

    std::vector<std::uint64_t> queuedRecipients;
    std::vector<std::uint64_t> allTimestampedRecipients;
    queuedRecipients.reserve(plan.recipients.size());
    allTimestampedRecipients.reserve(plan.recipients.size());
    for (auto const& recipient : plan.recipients) {
      allTimestampedRecipients.push_back(recipient.federateId);
      if (timeConstrainedRecipients.contains(recipient.federateId)) {
        queuedRecipients.push_back(recipient.federateId);
      }
    }
    auto const result = registry_.enqueueTsoInteraction(
        sendRequest.federationName,
        std::move(message),
        queuedRecipients,
        allTimestampedRecipients);
    if (result.status != FederationTsoRegistryStatus::applied ||
        result.queueStatus != TsoMessageQueueStatus::applied ||
        result.messageId == 0U) {
      return rejected(request);
    }
    messageId = result.messageId;
    {
      std::scoped_lock lock(mutex_);
      processTsoMessageProducers_.emplace(
          messageId,
          sendRequest.producingFederateId);
    }
  }

  std::vector<ProcessFederationInteractionEvent> events;
  events.reserve(plan.recipients.size());
  std::uint32_t queuedRecipientCount = 0U;
  for (auto const& recipient : plan.recipients) {
    if (queueTimestampedInteraction &&
        timeConstrainedRecipients.contains(recipient.federateId)) {
      if (queuedRecipientCount != std::numeric_limits<std::uint32_t>::max()) {
        ++queuedRecipientCount;
      }
      continue;
    }
    auto resolvedRecipient = recipient;
    if (resolvedRecipient.receivedInteractionClassHandle == 0U) {
      auto const current = registry_.receiveOrderInteractionRecipientFor(
          sendRequest.federationName,
          sendRequest.producingFederateId,
          recipient.federateId,
          sendRequest.interactionClassHandle,
          sendRequest.sentParameterHandles,
          sendRequest.sentRegionHandles
              ? &*sendRequest.sentRegionHandles
              : nullptr,
          plan.sentRegionSnapshots.empty()
              ? nullptr
              : &plan.sentRegionSnapshots);
      if (!current) {
        continue;
      }
      resolvedRecipient = *current;
    }
    if (resolvedRecipient.callbackRoute) {
      resolvedRecipient.callbackRoute.enqueueReceiveOrder(
          [](rti1516_2025::FederateAmbassador&) {});
    }
    ProcessFederationInteractionEvent event{
        sendRequest.producingFederateId,
        resolvedRecipient.federateId,
        resolvedRecipient.receivedInteractionClassHandle,
        parameterVector(resolvedRecipient.receivedParameterHandles),
        sendRequest.payload,
        plan.transportationName,
        sendRequest.timestamp,
        std::nullopt,
        messageId == 0U ? std::nullopt
                        : std::optional<std::uint64_t>(messageId)};
    event.defaultRegionUsed =
        resolvedRecipient.conveyRegionDesignatorSets && plan.defaultRegionUsed;
    if (resolvedRecipient.conveyRegionDesignatorSets &&
        (sendRequest.sentRegionHandles.has_value() || plan.defaultRegionUsed)) {
      event.sentRegionHandles = sendRequest.sentRegionHandles;
    }
    events.push_back(std::move(event));
  }

  std::uint32_t recipientCount = 0U;
  std::vector<std::pair<ProcessTransportSession*, ProcessFederationInteractionEvent>>
      pushedEvents;
  {
    std::scoped_lock lock(mutex_);
    for (auto& event : events) {
      auto const receivingSession = sessionsByFederateId_.find(event.receivingFederateId);
      if (receivingSession == sessionsByFederateId_.end()) {
        continue;
      }
      auto const state = sessions_.find(receivingSession->second);
      if (state == sessions_.end()) {
        continue;
      }
      if (options_.pushReceiveOrderEvents) {
        // The push path is exactly-once at this service boundary: the event
        // frame is the receiver's delivery, so do not also leave a duplicate
        // copy in the legacy polling queue.  The callback bridge owns
        // conversion to official C++ handle/value types.
        pushedEvents.emplace_back(receivingSession->second, event);
      } else {
        // The default service mode retains the original deterministic polling
        // seam used by the focused registry/service unit tests.
        state->second.interactionEvents.push_back(std::move(event));
      }
      if (recipientCount != std::numeric_limits<std::uint32_t>::max()) {
        ++recipientCount;
      }
    }
  }

  if (queueTimestampedInteraction) {
    auto const totalCount = static_cast<std::uint64_t>(recipientCount) +
        queuedRecipientCount;
    recipientCount = static_cast<std::uint32_t>(std::min<std::uint64_t>(
        totalCount,
        std::numeric_limits<std::uint32_t>::max()));
  }

  if (options_.pushReceiveOrderEvents) {
    for (auto const& pushedEvent : pushedEvents) {
      auto const eventMessage = TransportServiceMessage{
          TransportServiceMessageKind::event,
          TransportServiceOperation::receive_interaction,
          TransportServiceStatus::ok,
          0U,
          encodeProcessFederationReceiveInteractionResult(
              ProcessFederationReceiveInteractionResult{pushedEvent.second})};
      if (pushedEvent.first == nullptr || !pushedEvent.first->send(eventMessage)) {
        return internalError(request);
      }
      if (pushedEvent.second.retractionMessageId) {
        std::scoped_lock lock(mutex_);
        pendingPushedRetractionRecipients_[
            *pushedEvent.second.retractionMessageId]
            .push_back(pushedEvent.first);
      }
    }
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationSendInteractionResult(
          ProcessFederationSendInteractionResult{recipientCount, messageId}));
}

TransportServiceMessage ProcessFederationService::handleSendInteractionWithRegions(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  // The regional form uses the same validated interaction envelope and
  // recipient fan-out as ordinary Send Interaction.  The operation-specific
  // guard in handleSendInteraction requires an engaged region set, keeping
  // omitted regions distinguishable from the standard overload.
  return handleSendInteraction(session, request);
}

TransportServiceMessage ProcessFederationService::handleSendDirectedInteraction(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const sendRequest =
      decodeProcessFederationSendDirectedInteractionRequest(request.payload);
  if (sendRequest.timestamp) {
    validateProcessLogicalTime(
        *sendRequest.timestamp,
        federationDefinition_->logicalTimeImplementationName);
  }
  std::shared_ptr<FederateTimeState> producingTimeState;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != sendRequest.federationName ||
        state->second.federateId != sendRequest.producingFederateId ||
        !registry_.memberById(
            sendRequest.federationName,
             sendRequest.producingFederateId)) {
      return rejected(request);
    }
    producingTimeState = state->second.timeState;
  }

  auto const plan = registry_.planReceiveOrderDirectedInteraction(
      sendRequest.federationName,
      sendRequest.producingFederateId,
      sendRequest.objectInstanceHandle,
      sendRequest.interactionClassHandle,
      sendRequest.sentParameterHandles);
  if (plan.status != ReceiveOrderDirectedInteractionStatus::applied) {
    return rejected(request);
  }

  auto const producingTimeSnapshot = producingTimeState
      ? producingTimeState->snapshot()
      : FederateTimeSnapshot{};
  bool const queueTimestampedDirectedInteraction =
      sendRequest.timestamp && producingTimeSnapshot.timeRegulating;
  std::set<std::uint64_t> timeConstrainedRecipients;
  if (queueTimestampedDirectedInteraction) {
    auto const execution = registry_.timeSnapshotFor(sendRequest.federationName);
    if (!execution) {
      return internalError(request);
    }
    for (auto const& federate : execution->federates) {
      if (federate.time.timeConstrained) {
        timeConstrainedRecipients.insert(federate.membership.id);
      }
    }
  }

  std::vector<std::uint64_t> queuedRecipientIds;
  queuedRecipientIds.reserve(plan.recipients.size());
  if (queueTimestampedDirectedInteraction) {
    for (auto const& recipient : plan.recipients) {
      if (timeConstrainedRecipients.contains(recipient.federateId)) {
        queuedRecipientIds.push_back(recipient.federateId);
      }
    }
  }

  std::uint64_t messageId = 0U;
  if (queueTimestampedDirectedInteraction && !plan.recipients.empty()) {
    auto timestamp = decodeProcessLogicalTime(
        *sendRequest.timestamp,
        federationDefinition_->logicalTimeImplementationName);
    auto const envelope =
        decodeProcessFederationInteractionEnvelope(sendRequest.payload);
    if (!envelope && !sendRequest.sentParameterHandles.empty()) {
      return rejected(request);
    }

    TsoDirectedInteractionMessage message;
    message.producingFederateId = sendRequest.producingFederateId;
    message.objectInstanceHandle = sendRequest.objectInstanceHandle;
    message.sentInteractionClassHandle = sendRequest.interactionClassHandle;
    message.sentParameterHandles = sendRequest.sentParameterHandles;
    message.transportationName = plan.transportationName;
    message.timestamp = std::move(timestamp);
    message.sentOrderType = rti1516_2025::TIMESTAMP;
    message.receivedOrderType = rti1516_2025::RECEIVE;
    if (envelope) {
      message.userSuppliedTag.setData(
          envelope->userSuppliedTag.data(), envelope->userSuppliedTag.size());
      for (std::uint64_t const parameterHandle : sendRequest.sentParameterHandles) {
        auto const found = std::find_if(
            envelope->parameterValues.begin(),
            envelope->parameterValues.end(),
            [parameterHandle](ProcessFederationInteractionParameterValue const& value) {
              return value.first == parameterHandle;
            });
        if (found == envelope->parameterValues.end()) {
          return rejected(request);
        }
        rti1516_2025::VariableLengthData value;
        value.setData(found->second.data(), found->second.size());
        message.parameters.emplace_back(parameterHandle, std::move(value));
      }
    } else {
      message.userSuppliedTag.setData(
          sendRequest.payload.data(), sendRequest.payload.size());
    }

    InteractionCallbackRoute processRoute;
    processRoute.submit = [](FederateCallbackInvocation) {};
    for (auto const& recipient : plan.recipients) {
      auto resolvedRecipient = recipient;
      if (resolvedRecipient.receivedInteractionClassHandle == 0U) {
        auto const current = registry_.receiveOrderDirectedInteractionRecipientFor(
            sendRequest.federationName,
            sendRequest.producingFederateId,
            recipient.federateId,
            sendRequest.objectInstanceHandle,
            sendRequest.interactionClassHandle,
            sendRequest.sentParameterHandles);
        if (!current) {
          continue;
        }
        resolvedRecipient = *current;
      }
      message.recipients.push_back({
          resolvedRecipient.federateId,
          sendRequest.objectInstanceHandle,
          resolvedRecipient.receivedInteractionClassHandle,
          resolvedRecipient.receivedParameterHandles,
          processRoute});
    }
    if (message.recipients.empty()) {
      return responseFor(
          request,
          TransportServiceStatus::ok,
          encodeProcessFederationSendInteractionResult(
              ProcessFederationSendInteractionResult{}));
    }
    auto const result = registry_.enqueueTsoDirectedInteraction(
        sendRequest.federationName,
        std::move(message),
        queuedRecipientIds);
    if (result.status != FederationTsoRegistryStatus::applied ||
        result.queueStatus != TsoMessageQueueStatus::applied ||
        result.messageId == 0U) {
      return rejected(request);
    }
    messageId = result.messageId;
    {
      std::scoped_lock lock(mutex_);
      processTsoMessageProducers_.emplace(messageId, sendRequest.producingFederateId);
    }
  }

  std::vector<ProcessFederationInteractionEvent> events;
  events.reserve(plan.recipients.size());
  std::uint32_t queuedRecipientCount = 0U;
  for (auto const& recipient : plan.recipients) {
    if (queueTimestampedDirectedInteraction &&
        timeConstrainedRecipients.contains(recipient.federateId)) {
      if (queuedRecipientCount != std::numeric_limits<std::uint32_t>::max()) {
        ++queuedRecipientCount;
      }
      continue;
    }
    auto resolvedRecipient = recipient;
    if (resolvedRecipient.receivedInteractionClassHandle == 0U) {
      auto const current = registry_.receiveOrderDirectedInteractionRecipientFor(
          sendRequest.federationName,
          sendRequest.producingFederateId,
          recipient.federateId,
          sendRequest.objectInstanceHandle,
          sendRequest.interactionClassHandle,
          sendRequest.sentParameterHandles);
      if (!current) {
        continue;
      }
      resolvedRecipient = *current;
    }
    events.push_back(ProcessFederationInteractionEvent{
        sendRequest.producingFederateId,
        resolvedRecipient.federateId,
        resolvedRecipient.receivedInteractionClassHandle,
        parameterVector(resolvedRecipient.receivedParameterHandles),
        sendRequest.payload,
        plan.transportationName,
        sendRequest.timestamp,
        sendRequest.objectInstanceHandle,
        messageId == 0U ? std::nullopt
                        : std::optional<std::uint64_t>(messageId)});
  }

  if (messageId != 0U && options_.pushReceiveOrderEvents) {
    // The pushed frame is the process callback boundary for a timestamped
    // directed interaction. Establish the recipient reservation before the
    // frame is exposed so the receiver's post-callback acknowledgement makes
    // the producer's retraction designator terminal, just as the pull path
    // does in handleReceiveInteraction.
    events.erase(
        std::remove_if(
            events.begin(),
            events.end(),
            [&](ProcessFederationInteractionEvent const& event) {
              return !registry_.beginTsoInteractionCallback(
                  sendRequest.federationName,
                  event.receivingFederateId,
                  messageId);
            }),
        events.end());
  }

  std::uint32_t recipientCount = 0U;
  std::vector<std::pair<ProcessTransportSession*, ProcessFederationInteractionEvent>>
      pushedEvents;
  std::vector<ProcessTransportSession*> retractionRecipients;
  {
    std::scoped_lock lock(mutex_);
    for (auto& event : events) {
      auto const receivingSession = sessionsByFederateId_.find(event.receivingFederateId);
      if (receivingSession == sessionsByFederateId_.end()) {
        continue;
      }
      auto const state = sessions_.find(receivingSession->second);
      if (state == sessions_.end()) {
        continue;
      }
      if (options_.pushReceiveOrderEvents) {
        pushedEvents.emplace_back(receivingSession->second, event);
      } else {
        state->second.interactionEvents.push_back(std::move(event));
      }
      if (messageId != 0U) {
        retractionRecipients.push_back(receivingSession->second);
      }
      if (recipientCount != std::numeric_limits<std::uint32_t>::max()) {
        ++recipientCount;
      }
    }
  }

  if (queueTimestampedDirectedInteraction) {
    auto const totalCount = static_cast<std::uint64_t>(recipientCount) +
        queuedRecipientCount;
    recipientCount = static_cast<std::uint32_t>(std::min<std::uint64_t>(
        totalCount,
        std::numeric_limits<std::uint32_t>::max()));
  }

  if (messageId != 0U) {
    std::scoped_lock lock(mutex_);
    pendingPushedRetractionRecipients_.emplace(
        messageId, std::move(retractionRecipients));
  }
  if (options_.pushReceiveOrderEvents) {
    for (auto const& pushedEvent : pushedEvents) {
      auto const eventMessage = TransportServiceMessage{
          TransportServiceMessageKind::event,
          TransportServiceOperation::receive_interaction,
          TransportServiceStatus::ok,
          0U,
          encodeProcessFederationReceiveInteractionResult(
              ProcessFederationReceiveInteractionResult{pushedEvent.second})};
      if (pushedEvent.first == nullptr || !pushedEvent.first->send(eventMessage)) {
        return internalError(request);
      }
    }
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationSendInteractionResult(
          ProcessFederationSendInteractionResult{recipientCount, messageId}));
}

TransportServiceMessage ProcessFederationService::handleRetract(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const retractRequest = decodeProcessFederationRetractRequest(request.payload);
  std::shared_ptr<FederateTimeState> producingTimeState;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != retractRequest.federationName ||
        state->second.federateId != retractRequest.producingFederateId ||
        !registry_.memberById(
            retractRequest.federationName, retractRequest.producingFederateId)) {
        return rejected(request);
    }
    producingTimeState = state->second.timeState;
    auto const owner = processTsoMessageProducers_.find(retractRequest.messageId);
    if (owner == processTsoMessageProducers_.end() ||
        owner->second != retractRequest.producingFederateId) {
      return responseFor(
          request,
          TransportServiceStatus::ok,
          encodeProcessFederationRetractResult(
              {ProcessFederationRetractStatus::invalid_handle}));
      }
  }

  // A retraction designator remains live while a joined federate temporarily
  // disables time regulation, but the Retract service itself is only legal
  // while the producer is currently time regulating. Keep this check at the
  // process service boundary so the accepted message and its producer-owned
  // identity remain untouched for a later re-enable.  When regulation is
  // active, enforce the strict timestamp-versus-(current/requested time plus
  // actual lookahead) precondition from IEEE 1516.1-2025 clause 8.22.3.
  auto const producingTimeSnapshot =
      producingTimeState ? producingTimeState->snapshot() : FederateTimeSnapshot{};
  if (!producingTimeSnapshot.timeRegulating) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationRetractResult(
            {ProcessFederationRetractStatus::time_regulation_not_enabled}));
  }

  auto const retractionLowerBound =
      makeProcessTsoRetractionLowerBound(producingTimeSnapshot);
  if (!retractionLowerBound) {
    return internalError(request);
  }

  auto const result = registry_.retractTsoMessageForProducer(
      retractRequest.federationName,
      retractRequest.producingFederateId,
      retractRequest.messageId,
      retractionLowerBound,
      true);
  ProcessFederationRetractStatus status =
      ProcessFederationRetractStatus::invalid_handle;
  if (result.status == FederationTsoRegistryStatus::federate_not_member) {
    status = ProcessFederationRetractStatus::federate_not_member;
  } else if (result.status == FederationTsoRegistryStatus::applied &&
             !result.timestampEligible) {
    status = ProcessFederationRetractStatus::message_no_longer_retractable;
  } else if (result.status == FederationTsoRegistryStatus::applied) {
    switch (result.queueResult.status) {
      case TsoMessageQueueStatus::applied:
        status = ProcessFederationRetractStatus::applied;
        break;
      case TsoMessageQueueStatus::message_already_retracted:
      case TsoMessageQueueStatus::message_already_delivered:
      case TsoMessageQueueStatus::message_not_found:
        status = ProcessFederationRetractStatus::message_no_longer_retractable;
        break;
      default:
        status = ProcessFederationRetractStatus::invalid_handle;
        break;
    }
  }

  if (status == ProcessFederationRetractStatus::applied) {
    std::vector<ProcessTransportSession*> pushedRecipients;
    {
      std::scoped_lock lock(mutex_);
      // Retain the producer entry until the next Retract observes the
      // terminalized queue state. This lets a repeated use of the same
      // designator report MessageCanNoLongerBeRetracted instead of looking
      // like an unrelated invalid handle.
      for (auto& [ignoredSession, state] : sessions_) {
        state.interactionEvents.erase(
            std::remove_if(
                state.interactionEvents.begin(),
                state.interactionEvents.end(),
                [&retractRequest](ProcessFederationInteractionEvent const& event) {
                  return event.retractionMessageId == retractRequest.messageId;
                }),
            state.interactionEvents.end());
        state.objectInstanceRemovalEvents.erase(
            std::remove_if(
                state.objectInstanceRemovalEvents.begin(),
                state.objectInstanceRemovalEvents.end(),
                [&retractRequest](
                    ProcessFederationObjectInstanceRemovalEvent const& event) {
                  return event.retractionMessageId == retractRequest.messageId;
                }),
            state.objectInstanceRemovalEvents.end());
        state.attributeUpdateEvents.erase(
            std::remove_if(
                state.attributeUpdateEvents.begin(),
                state.attributeUpdateEvents.end(),
                [&retractRequest](
                    ProcessFederationAttributeUpdateEvent const& event) {
                  return event.retractionMessageId == retractRequest.messageId;
                }),
            state.attributeUpdateEvents.end());
      }
      auto pushed = pendingPushedRetractionRecipients_.find(
          retractRequest.messageId);
      if (pushed != pendingPushedRetractionRecipients_.end()) {
        pushedRecipients = std::move(pushed->second);
        pendingPushedRetractionRecipients_.erase(pushed);
      }
    }
    for (auto* recipient : pushedRecipients) {
      if (recipient == nullptr ||
          !recipient->send(TransportServiceMessage{
              TransportServiceMessageKind::event,
              TransportServiceOperation::request_retraction,
              TransportServiceStatus::ok,
              0U,
              encodeProcessFederationRequestRetractionEvent(
                  ProcessFederationRequestRetractionEvent{
                      retractRequest.messageId})})) {
        return internalError(request);
      }
    }
  } else if (status == ProcessFederationRetractStatus::message_no_longer_retractable) {
    std::scoped_lock lock(mutex_);
    processTsoMessageProducers_.erase(retractRequest.messageId);
    pendingPushedRetractionRecipients_.erase(retractRequest.messageId);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationRetractResult({status}));
}

TransportServiceMessage ProcessFederationService::handleReceiveInteraction(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const receiveRequest = decodeProcessFederationReceiveInteractionRequest(
      request.payload);
  ProcessFederationReceiveInteractionResult result;
  std::vector<AttributeOwnershipAcquisitionWorkItem> acquisitionFollowups;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != receiveRequest.federationName ||
        state->second.federateId != receiveRequest.receivingFederateId ||
        !registry_.memberById(
            receiveRequest.federationName,
            receiveRequest.receivingFederateId)) {
      return rejected(request);
    }
    if (!state->second.synchronizationPointAnnouncementEvents.empty()) {
      result.synchronizationPointAnnouncementEvent = std::move(
          state->second.synchronizationPointAnnouncementEvents.front());
      state->second.synchronizationPointAnnouncementEvents.pop_front();
    } else if (!state->second.federationSynchronizedEvents.empty()) {
      result.federationSynchronizedEvent = std::move(
          state->second.federationSynchronizedEvents.front());
      state->second.federationSynchronizedEvents.pop_front();
    } else if (!state->second.saveEvents.empty()) {
      result.saveEvent = std::move(state->second.saveEvents.front());
      state->second.saveEvents.pop_front();
    } else if (!state->second.restoreEvents.empty()) {
      result.restoreEvent = std::move(state->second.restoreEvents.front());
      state->second.restoreEvents.pop_front();
    } else if (!state->second.interactionEvents.empty()) {
      result.event = std::move(state->second.interactionEvents.front());
      state->second.interactionEvents.pop_front();
    } else if (!state->second.objectInstanceDiscoveryEvents.empty()) {
      // Object discovery is delivered before reflections for a newly
      // registered instance, matching the official callback ordering while
      // retaining the single receive-interaction polling fence.
      result.discoveryEvent = std::move(
          state->second.objectInstanceDiscoveryEvents.front());
      state->second.objectInstanceDiscoveryEvents.pop_front();
    } else if (!state->second.objectInstanceRemovalEvents.empty()) {
      // A receive-order deletion reserves the recipient at the producer's
      // service boundary, but the known-instance transition commits only when
      // this recipient crosses its callback boundary. Skip stale reservations
      // (for example after resignation) without manufacturing a callback.
      while (!state->second.objectInstanceRemovalEvents.empty()) {
        auto event = std::move(
            state->second.objectInstanceRemovalEvents.front());
        state->second.objectInstanceRemovalEvents.pop_front();
        auto const snapshot = event.retractionMessageId
            ? registry_.beginTsoObjectInstanceRemoval(
                  receiveRequest.federationName,
                  receiveRequest.receivingFederateId,
                  event.objectInstanceHandle,
                  *event.retractionMessageId)
            : registry_.beginObjectInstanceRemoval(
                  receiveRequest.federationName,
                  receiveRequest.receivingFederateId,
                  event.objectInstanceHandle);
        if (!snapshot) {
          continue;
        }
        event.producingFederateId = snapshot->producingFederateId;
        result.removalEvent = std::move(event);
        break;
      }
    } else {
      // Scope transitions are rechecked at the process delivery boundary so
      // a later region/subscription mutation, switch disable, or resignation
      // cannot leak stale Attributes In/Out Of Scope work.
      while (!state->second.objectInstanceScopeChangeEvents.empty()) {
        auto event = std::move(
            state->second.objectInstanceScopeChangeEvents.front());
        state->second.objectInstanceScopeChangeEvents.pop_front();
        auto const eligible = registry_.objectInstanceScopeAttributes(
            receiveRequest.federationName,
            receiveRequest.receivingFederateId,
            event.objectInstanceHandle,
            event.attributeHandles,
            event.inScope);
        if (eligible.empty()) {
          continue;
        }
        event.attributeHandles = eligible;
        result.scopeChangeEvent = std::move(event);
        break;
      }
      if (!result.scopeChangeEvent &&
          !state->second.attributeRelevanceAdvisoryEvents.empty()) {
        while (!state->second.attributeRelevanceAdvisoryEvents.empty()) {
          auto event = std::move(
              state->second.attributeRelevanceAdvisoryEvents.front());
          state->second.attributeRelevanceAdvisoryEvents.pop_front();
          if (event.providingFederateId != receiveRequest.receivingFederateId) {
            continue;
          }
          auto const eligible = registry_.attributeRelevanceAdvisoryAttributes(
              receiveRequest.federationName,
              event.providingFederateId,
              event.receivingFederateId,
              event.objectInstanceHandle,
              event.attributeHandles,
              event.turnUpdatesOn);
          if (eligible.empty()) {
            continue;
          }
          event.attributeHandles = eligible;
          if (event.turnUpdatesOn) {
            event.updateRateDesignator =
                registry_.attributeRelevanceAdvisoryUpdateRateDesignatorFor(
                    receiveRequest.federationName,
                    event.receivingFederateId,
                    event.objectInstanceHandle,
                    *event.attributeHandles.begin());
          } else {
            event.updateRateDesignator.reset();
          }
          result.attributeRelevanceAdvisoryEvent = std::move(event);
          break;
        }
      }
      if (!result.scopeChangeEvent &&
          !result.attributeRelevanceAdvisoryEvent &&
          !state->second.attributeValueUpdateRequestEvents.empty()) {
        result.attributeValueUpdateRequestEvent = std::move(
            state->second.attributeValueUpdateRequestEvents.front());
        state->second.attributeValueUpdateRequestEvents.pop_front();
      }
      if (!result.scopeChangeEvent &&
          !result.attributeRelevanceAdvisoryEvent &&
          !result.attributeValueUpdateRequestEvent &&
          !state->second.attributeUpdateEvents.empty()) {
        // Keep the original receive-interaction polling operation as the
        // single deterministic receive fence. Attribute reflections use the
        // adjacent result slot so older process clients do not need a second
        // competing poll that would consume a response intended for an
        // interaction test server.
        result.attributeEvent =
            std::move(state->second.attributeUpdateEvents.front());
        state->second.attributeUpdateEvents.pop_front();
      }
      if (!result.scopeChangeEvent &&
          !result.attributeRelevanceAdvisoryEvent &&
          !result.attributeValueUpdateRequestEvent &&
          !result.attributeEvent) {
        while (!state->second.attributeTransportationTypeChangeEvents.empty()) {
          auto event = std::move(
              state->second.attributeTransportationTypeChangeEvents.front());
          state->second.attributeTransportationTypeChangeEvents.pop_front();
          auto const delivery = registry_.beginAttributeTransportationTypeChange(
              receiveRequest.federationName,
              receiveRequest.receivingFederateId,
              event.requestId);
          if (!delivery || delivery->attributeHandles.empty() ||
              delivery->transportationName.empty()) {
            continue;
          }
          event.objectInstanceHandle = delivery->objectInstanceHandle;
          event.attributeHandles = std::move(delivery->attributeHandles);
          event.transportationName = std::move(delivery->transportationName);
          result.attributeTransportationTypeChangeEvent = std::move(event);
          break;
        }
      }
      if (!result.scopeChangeEvent &&
          !result.attributeRelevanceAdvisoryEvent &&
          !result.attributeValueUpdateRequestEvent &&
          !result.attributeEvent &&
          !result.attributeTransportationTypeChangeEvent) {
        while (!state->second.attributeTransportationTypeQueryEvents.empty()) {
          auto event = std::move(
              state->second.attributeTransportationTypeQueryEvents.front());
          state->second.attributeTransportationTypeQueryEvents.pop_front();
          auto const projection = registry_.attributeTransportationTypeQueryFor(
              receiveRequest.federationName,
              receiveRequest.receivingFederateId,
              event.objectInstanceHandle,
              event.attributeHandle);
          if (!projection || projection->transportationName.empty()) {
            continue;
          }
          event.objectInstanceHandle = projection->objectInstanceHandle;
          event.attributeHandle = projection->attributeHandle;
          event.transportationName = projection->transportationName;
          result.attributeTransportationTypeQueryEvent = std::move(event);
          break;
        }
      }
      if (!result.scopeChangeEvent &&
          !result.attributeRelevanceAdvisoryEvent &&
          !result.attributeValueUpdateRequestEvent &&
          !result.attributeEvent &&
          !result.attributeTransportationTypeChangeEvent &&
          !result.attributeTransportationTypeQueryEvent) {
        while (!state->second.attributeOwnershipQueryEvents.empty()) {
          auto event = std::move(
              state->second.attributeOwnershipQueryEvents.front());
          state->second.attributeOwnershipQueryEvents.pop_front();
          auto const projection = registry_.attributeOwnershipQueryRecipientFor(
              receiveRequest.federationName,
              event.requestId,
              receiveRequest.receivingFederateId,
              event.objectInstanceHandle,
              event.reportKind,
              event.owningFederateId,
              event.attributeHandles);
          if (!projection) {
            continue;
          }
          event.attributeHandles = projection->attributeHandles;
          if (event.attributeHandles.empty()) {
            continue;
          }
          result.attributeOwnershipQueryEvent = std::move(event);
          break;
        }
      }
      if (!result.scopeChangeEvent &&
          !result.attributeRelevanceAdvisoryEvent &&
          !result.attributeValueUpdateRequestEvent &&
          !result.attributeEvent &&
          !result.attributeTransportationTypeChangeEvent &&
          !result.attributeTransportationTypeQueryEvent) {
        while (!state->second.interactionTransportationTypeChangeEvents.empty()) {
          auto event = std::move(
              state->second.interactionTransportationTypeChangeEvents.front());
          state->second.interactionTransportationTypeChangeEvents.pop_front();
          auto const transportationName =
              registry_.beginInteractionTransportationTypeChange(
                  receiveRequest.federationName,
                  event.receivingFederateId,
                  event.interactionClassHandle);
          if (!transportationName || transportationName->empty()) {
            continue;
          }
          event.transportationName = std::move(*transportationName);
          result.interactionTransportationTypeChangeEvent = std::move(event);
          break;
        }
      }
      if (!result.scopeChangeEvent &&
          !result.attributeRelevanceAdvisoryEvent &&
          !result.attributeValueUpdateRequestEvent &&
          !result.attributeEvent &&
          !result.attributeTransportationTypeChangeEvent &&
          !result.attributeTransportationTypeQueryEvent &&
          !result.interactionTransportationTypeChangeEvent) {
        while (!state->second.interactionTransportationTypeQueryEvents.empty()) {
          auto event = std::move(
              state->second.interactionTransportationTypeQueryEvents.front());
          state->second.interactionTransportationTypeQueryEvents.pop_front();
          auto const projection = registry_.interactionTransportationTypeQueryFor(
              receiveRequest.federationName,
              event.receivingFederateId,
              event.queriedFederateId,
              event.interactionClassHandle);
          if (!projection || projection->transportationName.empty()) {
            continue;
          }
          event.interactionClassHandle = projection->interactionClassHandle;
          event.transportationName = projection->transportationName;
          result.interactionTransportationTypeQueryEvent = std::move(event);
          break;
        }
      }
      if (!result.scopeChangeEvent &&
          !result.attributeRelevanceAdvisoryEvent &&
          !result.attributeValueUpdateRequestEvent &&
          !result.attributeEvent &&
          !result.attributeTransportationTypeChangeEvent &&
          !result.attributeTransportationTypeQueryEvent &&
          !result.attributeOwnershipQueryEvent) {
        while (!state->second.attributeOwnershipUnavailableEvents.empty()) {
          auto event = std::move(
              state->second.attributeOwnershipUnavailableEvents.front());
          state->second.attributeOwnershipUnavailableEvents.pop_front();
          auto const delivery = registry_.attributeOwnershipUnavailableRecipientFor(
              receiveRequest.federationName,
              event.receivingFederateId,
              event.objectInstanceHandle,
              event.attributeHandles);
          if (!delivery || delivery->attributeHandles.empty()) {
            continue;
          }
          event.attributeHandles = delivery->attributeHandles;
          result.attributeOwnershipUnavailableEvent = std::move(event);
          break;
        }
      }
      if (!result.scopeChangeEvent &&
          !result.attributeRelevanceAdvisoryEvent &&
          !result.attributeValueUpdateRequestEvent &&
          !result.attributeEvent &&
          !result.attributeTransportationTypeChangeEvent &&
          !result.attributeTransportationTypeQueryEvent &&
          !result.attributeOwnershipQueryEvent &&
          !result.attributeOwnershipUnavailableEvent) {
        while (!state->second.attributeOwnershipAcquisitionEvents.empty()) {
          auto event = std::move(
              state->second.attributeOwnershipAcquisitionEvents.front());
          state->second.attributeOwnershipAcquisitionEvents.pop_front();
          if (event.kind ==
              ProcessFederationAttributeOwnershipAcquisitionEventKind::
                  acquisition_notification) {
            auto const delivery =
                registry_.beginAttributeOwnershipAcquisitionNotification(
                    receiveRequest.federationName,
                    event.requestingFederateId,
                    event.objectInstanceHandle,
                    event.requestId,
                    event.attributeHandles);
            if (!delivery) {
              continue;
            }
            event.attributeHandles = delivery->securedAttributeHandles;
            for (auto& followup : delivery->followupWorkItems) {
              acquisitionFollowups.push_back(std::move(followup));
            }
          } else if (event.kind ==
                     ProcessFederationAttributeOwnershipAcquisitionEventKind::
                         request_release) {
            auto const delivery =
                registry_.beginAttributeOwnershipAcquisitionRelease(
                    receiveRequest.federationName,
                    event.requestingFederateId,
                    event.receivingFederateId,
                    event.objectInstanceHandle,
                    event.requestId,
                    event.attributeHandles);
            if (!delivery) {
              continue;
            }
            event.attributeHandles = delivery->candidateAttributeHandles;
          } else if (event.kind ==
                     ProcessFederationAttributeOwnershipAcquisitionEventKind::
                         cancellation_confirmation) {
            auto const delivery =
                registry_.beginAttributeOwnershipAcquisitionCancellation(
                    receiveRequest.federationName,
                    event.requestingFederateId,
                    event.objectInstanceHandle,
                    event.requestId,
                    event.attributeHandles);
            if (!delivery) {
              continue;
            }
            event.attributeHandles = delivery->confirmedAttributeHandles;
            for (auto& followup : delivery->followupWorkItems) {
              acquisitionFollowups.push_back(std::move(followup));
            }
          } else if (event.kind ==
                     ProcessFederationAttributeOwnershipAcquisitionEventKind::
                         request_divestiture_confirmation) {
            auto const delivery =
                registry_.beginRequestDivestitureConfirmation(
                    receiveRequest.federationName,
                    event.receivingFederateId,
                    event.requestingFederateId,
                    event.objectInstanceHandle,
                    event.requestId,
                    event.candidateIsIfAvailable,
                    event.attributeHandles);
            if (!delivery) {
              continue;
            }
            event.attributeHandles = delivery->releasedAttributeHandles;
          } else if (event.kind ==
                     ProcessFederationAttributeOwnershipAcquisitionEventKind::
                         confirm_divestiture_notification) {
            if (event.receivingFederateId != event.requestingFederateId) {
              continue;
            }
            auto const delivery =
                registry_.beginConfirmDivestitureNotification(
                    receiveRequest.federationName,
                    event.receivingFederateId,
                    event.objectInstanceHandle,
                    event.requestId,
                    event.attributeHandles);
            if (!delivery) {
              continue;
            }
            event.attributeHandles = delivery->securedAttributeHandles;
            for (auto& followup : delivery->followupWorkItems) {
              acquisitionFollowups.push_back(std::move(followup));
            }
          } else if (event.kind ==
                     ProcessFederationAttributeOwnershipAcquisitionEventKind::
                         ownership_assumption) {
            auto const delivery = registry_.attributeOwnershipAssumptionDeliveryFor(
                receiveRequest.federationName,
                event.receivingFederateId,
                event.objectInstanceHandle,
                event.attributeHandles);
            if (!delivery) {
              continue;
            }
            event.attributeHandles = delivery->attributeHandles;
          } else {
            continue;
          }
          if (event.attributeHandles.empty()) {
            continue;
          }
          result.attributeOwnershipAcquisitionEvent = std::move(event);
          break;
        }
      }
      if (!result.scopeChangeEvent &&
          !result.attributeRelevanceAdvisoryEvent &&
          !result.attributeValueUpdateRequestEvent &&
          !result.attributeEvent &&
          !result.attributeTransportationTypeChangeEvent &&
          !result.attributeTransportationTypeQueryEvent &&
          !result.attributeOwnershipQueryEvent &&
          !result.attributeOwnershipAcquisitionEvent &&
          !result.attributeOwnershipUnavailableEvent) {
        while (!state->second
                    .attributeOwnershipAcquisitionIfAvailableEvents.empty()) {
          auto event = std::move(state->second
                                     .attributeOwnershipAcquisitionIfAvailableEvents
                                     .front());
          state->second.attributeOwnershipAcquisitionIfAvailableEvents.pop_front();
          auto const delivery =
              registry_.beginAttributeOwnershipAcquisitionIfAvailable(
                  receiveRequest.federationName,
                  receiveRequest.receivingFederateId,
                  event.objectInstanceHandle,
                  event.requestId);
          if (!delivery) {
            continue;
          }
          event.securedAttributeHandles = delivery->securedAttributeHandles;
          event.unavailableAttributeHandles =
              delivery->unavailableAttributeHandles;
          if (event.securedAttributeHandles.empty() &&
              event.unavailableAttributeHandles.empty()) {
            continue;
          }
          result.attributeOwnershipAcquisitionIfAvailableEvent =
              std::move(event);
          break;
        }
      }
    }
  }
  if (!acquisitionFollowups.empty() &&
      !enqueueAttributeOwnershipAcquisitionWorkItems(
          receiveRequest.federationName,
          std::move(acquisitionFollowups))) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationReceiveInteractionResult(result));
}

TransportServiceMessage ProcessFederationService::handleAcknowledgeTsoDelivery(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const acknowledgement =
      decodeProcessFederationAcknowledgeTsoDeliveryRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != acknowledgement.federationName ||
        state->second.federateId != acknowledgement.receivingFederateId ||
        !registry_.memberById(
            acknowledgement.federationName,
            acknowledgement.receivingFederateId)) {
      return rejected(request);
    }
  }

  auto const delivery = registry_.completeTsoDeliveryFor(
      acknowledgement.federationName,
      acknowledgement.receivingFederateId,
      acknowledgement.messageId);
  if (delivery.status != FederationTsoRegistryStatus::applied) {
    return rejected(request);
  }

  ProcessFederationTsoDeliveryAcknowledgementResult result;
  switch (delivery.delivery.status) {
    case FederationTsoDeliveryStatus::applied:
      result.status =
          ProcessFederationTsoDeliveryAcknowledgementStatus::applied;
      break;
    case FederationTsoDeliveryStatus::message_already_completed:
      result.status = ProcessFederationTsoDeliveryAcknowledgementStatus::
          already_completed;
      break;
    case FederationTsoDeliveryStatus::message_not_in_transit:
    case FederationTsoDeliveryStatus::no_messages:
      result.status =
          ProcessFederationTsoDeliveryAcknowledgementStatus::not_in_transit;
      break;
    case FederationTsoDeliveryStatus::invalid_recipient:
    case FederationTsoDeliveryStatus::invalid_boundary:
      return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationTsoDeliveryAcknowledgementResult(result));
}

TransportServiceMessage ProcessFederationService::handleReceiveAttributeUpdate(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const receiveRequest = decodeProcessFederationReceiveInteractionRequest(
      request.payload);
  ProcessFederationReceiveAttributeUpdateResult result;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != receiveRequest.federationName ||
        state->second.federateId != receiveRequest.receivingFederateId ||
        !registry_.memberById(
            receiveRequest.federationName, receiveRequest.receivingFederateId)) {
      return rejected(request);
    }
    while (!state->second.attributeUpdateEvents.empty()) {
      auto event = std::move(state->second.attributeUpdateEvents.front());
      state->second.attributeUpdateEvents.pop_front();
      if (event.retractionMessageId &&
          !registry_.beginTsoAttributeUpdateCallback(
              receiveRequest.federationName,
              receiveRequest.receivingFederateId,
              *event.retractionMessageId)) {
        // A legal Retract may have won before this pull-mode event crossed
        // the callback boundary. Consume the stale frame without exposing a
        // reflection callback.
        continue;
      }
      result.event = std::move(event);
      break;
    }
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationReceiveAttributeUpdateResult(result));
}

TransportServiceMessage
ProcessFederationService::handleReceiveObjectInstanceDiscovery(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const receiveRequest = decodeProcessFederationReceiveInteractionRequest(
      request.payload);
  ProcessFederationReceiveObjectInstanceDiscoveryResult result;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != receiveRequest.federationName ||
        state->second.federateId != receiveRequest.receivingFederateId ||
        !registry_.memberById(
            receiveRequest.federationName,
            receiveRequest.receivingFederateId)) {
      return rejected(request);
    }
    if (!state->second.objectInstanceDiscoveryEvents.empty()) {
      result.event = std::move(
          state->second.objectInstanceDiscoveryEvents.front());
      state->second.objectInstanceDiscoveryEvents.pop_front();
    }
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationReceiveObjectInstanceDiscoveryResult(result));
}

TransportServiceMessage ProcessFederationService::rejected(
    TransportServiceMessage const& request) const {
  return responseFor(request, TransportServiceStatus::rejected);
}

TransportServiceMessage ProcessFederationService::invalid(
    TransportServiceMessage const& request) const {
  return responseFor(request, TransportServiceStatus::invalid_request);
}

TransportServiceMessage ProcessFederationService::internalError(
    TransportServiceMessage const& request) const {
  return responseFor(request, TransportServiceStatus::internal_error);
}

}  // namespace umbra::detail
