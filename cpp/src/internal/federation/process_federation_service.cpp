#include "internal/federation/process_federation_service.hpp"

#include "internal/runtime/utf8_string.hpp"

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
  auto const count = reader.count(sizeof(std::uint64_t) * 2U);
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
  return std::move(writer).finish();
}

ProcessFederationCreateRequest decodeProcessFederationCreateRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationCreateRequest result{reader.wideString()};
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation create request requires a federation name.");
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
  reader.finish();
  if (result.federationName.empty() || result.federateType.empty() ||
      (result.requestedFederateName.has_value() &&
       result.requestedFederateName->empty())) {
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
  return std::move(writer).finish();
}

ProcessFederationJoinResult decodeProcessFederationJoinResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationJoinResult result;
  result.federateId = reader.unsigned64();
  result.federateName = reader.wideString();
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
  if (result.status > ProcessFederationRetractStatus::federate_not_member) {
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
                   ProcessFederationRetractStatus::federate_not_member)) {
    throw ProcessFederationServiceProtocolError(
        "A process retraction result has an invalid status.");
  }
  return ProcessFederationRetractResult{
      static_cast<ProcessFederationRetractStatus>(status)};
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
      static_cast<unsigned>(result.attributeRelevanceAdvisoryEvent.has_value());
  if (eventCount > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process federation receive result cannot contain multiple events.");
  }
  PayloadWriter writer;
  writer.unsigned8(result.event.has_value()
                       ? 1U
                       : (result.attributeEvent.has_value()
                              ? 2U
                              : (result.discoveryEvent.has_value()
                              ? 3U
                              : (result.scopeChangeEvent.has_value()
                                     ? 4U
                                     : (result.removalEvent.has_value()
                                            ? 6U
                                            : (result.attributeRelevanceAdvisoryEvent
                                                       .has_value()
                                                   ? 5U
                                                   : 0U))))));
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
  }
  return std::move(writer).finish();
}

ProcessFederationReceiveInteractionResult
decodeProcessFederationReceiveInteractionResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const eventKind = reader.unsigned8();
  if (eventKind > 6U) {
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
  }
  reader.finish();
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationUpdateAttributeValuesResult(
    ProcessFederationUpdateAttributeValuesResult const& result) {
  PayloadWriter writer;
  writer.unsigned32(result.recipientCount);
  return std::move(writer).finish();
}

ProcessFederationUpdateAttributeValuesResult
decodeProcessFederationUpdateAttributeValuesResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationUpdateAttributeValuesResult result{reader.unsigned32()};
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
      case TransportServiceOperation::send_interaction:
        return handleSendInteraction(session, request);
      case TransportServiceOperation::send_directed_interaction:
        return handleSendDirectedInteraction(session, request);
      case TransportServiceOperation::retract:
        return handleRetract(session, request);
      case TransportServiceOperation::update_attribute_values:
        return handleUpdateAttributeValues(session, request);
      case TransportServiceOperation::receive_interaction:
        return handleReceiveInteraction(session, request);
      case TransportServiceOperation::receive_attribute_update:
        return handleReceiveAttributeUpdate(session, request);
      case TransportServiceOperation::receive_object_instance_discovery:
        return handleReceiveObjectInstanceDiscovery(session, request);
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
      case TransportServiceOperation::get_dimension_handle:
        return handleGetDimensionHandle(session, request);
      case TransportServiceOperation::get_dimension_upper_bound:
        return handleGetDimensionUpperBound(session, request);
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
      case TransportServiceOperation::publish_interaction_class:
      case TransportServiceOperation::unpublish_interaction_class:
      case TransportServiceOperation::subscribe_interaction_class:
      case TransportServiceOperation::unsubscribe_interaction_class:
        return handleInteractionClassDeclaration(session, request);
    }
  } catch (ProcessFederationServiceProtocolError const&) {
    return invalid(request);
  } catch (std::exception const&) {
    return internalError(request);
  }
  return invalid(request);
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
      state->second.interactionEvents.clear();
      state->second.attributeUpdateEvents.clear();
      state->second.objectInstanceDiscoveryEvents.clear();
      state->second.objectInstanceRemovalEvents.clear();
      state->second.objectInstanceScopeChangeEvents.clear();
      state->second.attributeRelevanceAdvisoryEvents.clear();
    }
  }
  return responseFor(request, TransportServiceStatus::ok);
}

TransportServiceMessage ProcessFederationService::handleCreate(
    TransportServiceMessage const& request) {
  auto const createRequest = decodeProcessFederationCreateRequest(request.payload);
  auto const result = registry_.create(
      createRequest.federationName, *federationDefinition_);
  if (result.status != FederationRegistryStatus::applied) {
    return rejected(request);
  }
  return responseFor(request, TransportServiceStatus::ok);
}

TransportServiceMessage ProcessFederationService::handleJoin(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const joinRequest = decodeProcessFederationJoinRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() || state->second.federateId != 0U) {
      return rejected(request);
    }
  }

  InteractionCallbackRoute callbackRoute;
  callbackRoute.submit = [](FederateCallbackInvocation) {};
  // The registry retains this route as the callback ownership boundary.  The
  // first process slice projects the callback payload into the receiver's
  // event queue below; a later public adapter will replace this bridge with
  // the receiver process's official FederateAmbassador dispatch.
  callbackRoute.receiveOrderSubmit = [](FederateCallbackInvocation) {};
  auto const result = registry_.join(
      joinRequest.federationName,
      joinRequest.federateType,
      joinRequest.requestedFederateName,
      std::move(callbackRoute));
  if (result.status != FederationRegistryStatus::applied || !result.membership) {
    return rejected(request);
  }

  {
    std::scoped_lock lock(mutex_);
    auto& state = sessions_.at(&session);
    state.federationName = joinRequest.federationName;
    state.federateId = result.membership->id;
    sessionsByFederateId_[state.federateId] = &session;
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationJoinResult(
          ProcessFederationJoinResult{
              result.membership->id,
              result.membership->name}));
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
    auto discoveries = registry_.planObjectInstanceDiscoveriesForFederate(
        subscriptionRequest.federationName,
        subscriptionRequest.federateId);
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
    // visible. Discovery remains ordered before any scope transition.
    auto discoveries = registry_.planObjectInstanceDiscoveriesForFederate(
        subscriptionRequest.federationName,
        subscriptionRequest.federateId);
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

  if (deletionRequest.timestamp) {
    // The process endpoint does not yet expose time regulation or grant
    // services, but it can still carry the official logical-time value across
    // the boundary. Retain the registry's timestamped deletion ledger so the
    // callback carries the same immutable recipient snapshot and message
    // identity as the embedded path; the public process adapter deliberately
    // keeps its returned MessageRetractionHandle invalid until that temporal
    // control plane is exposed.
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

    TsoObjectDeletionMessage message;
    message.producingFederateId = deletionRequest.federateId;
    message.objectInstanceHandle = deletionRequest.objectInstanceHandle;
    if (!deletionRequest.userSuppliedTag.empty()) {
      message.userSuppliedTag.setData(
          deletionRequest.userSuppliedTag.data(),
          deletionRequest.userSuppliedTag.size());
    }
    message.timestamp = timestamp;
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
        {});
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
    for (auto const& recipient : enqueueResult.recipients) {
      removals.push_back({
          recipient.receivingFederateId,
          recipient.objectInstanceHandle,
          recipient.callbackRoute,
          recipient.serviceReportRoute,
          false});
    }
    if (!enqueueObjectInstanceRemovals(
            deletionRequest.federationName,
            std::move(removals),
            deletionRequest.userSuppliedTag,
            deletionRequest.timestamp,
            enqueueResult.messageId)) {
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
                enqueueResult.messageId}));
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
      registry_.cancelObjectInstanceDiscovery(
          federationName,
          planned.receivingFederateId,
          planned.objectInstanceHandle);
      continue;
    }

    // Recheck the discovery predicate and establish the receiving federate's
    // known-instance state before exposing the event. The process service has
    // no user callback route of its own, so this registry transition is the
    // process-boundary delivery commit; the client then projects the exact
    // snapshot through the official callback bridge.
    auto const snapshot = registry_.beginObjectInstanceDiscovery(
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
    std::uint64_t retractionMessageId) {
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
          &acceptedAttributeValues) != FederationRegistryStatus::applied) {
    return internalError(request);
  }

  std::vector<ProcessFederationAttributeUpdateEvent> events;
  std::uint32_t recipientCount = 0U;
  for (auto const& passel : plan.passels) {
    for (auto const& plannedRecipient : passel.recipients) {
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
        if (options_.pushReceiveOrderEvents) {
          pushedEvents.emplace_back(receivingSession->second, std::move(event));
        } else {
          receivingState->second.attributeUpdateEvents.push_back(std::move(event));
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
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationUpdateAttributeValuesResult(
          ProcessFederationUpdateAttributeValuesResult{recipientCount}));
}

TransportServiceMessage ProcessFederationService::handleSendInteraction(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const sendRequest = decodeProcessFederationSendInteractionRequest(request.payload);
  if (sendRequest.timestamp) {
    validateProcessLogicalTime(
        *sendRequest.timestamp,
        federationDefinition_->logicalTimeImplementationName);
  }
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
  }

  auto const plan = registry_.planReceiveOrderInteraction(
      sendRequest.federationName,
      sendRequest.producingFederateId,
      sendRequest.interactionClassHandle,
      sendRequest.sentParameterHandles);
  if (plan.status != ReceiveOrderInteractionStatus::applied) {
    return rejected(request);
  }

  std::vector<ProcessFederationInteractionEvent> events;
  events.reserve(plan.recipients.size());
  for (auto const& recipient : plan.recipients) {
    auto resolvedRecipient = recipient;
    if (resolvedRecipient.receivedInteractionClassHandle == 0U) {
      auto const current = registry_.receiveOrderInteractionRecipientFor(
          sendRequest.federationName,
          sendRequest.producingFederateId,
          recipient.federateId,
          sendRequest.interactionClassHandle,
          sendRequest.sentParameterHandles);
      if (!current) {
        continue;
      }
      resolvedRecipient = *current;
    }
    if (resolvedRecipient.callbackRoute) {
      resolvedRecipient.callbackRoute.enqueueReceiveOrder(
          [](rti1516_2025::FederateAmbassador&) {});
    }
    events.push_back(ProcessFederationInteractionEvent{
        sendRequest.producingFederateId,
        resolvedRecipient.federateId,
        resolvedRecipient.receivedInteractionClassHandle,
        parameterVector(resolvedRecipient.receivedParameterHandles),
        sendRequest.payload,
        plan.transportationName,
        sendRequest.timestamp});
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
          ProcessFederationSendInteractionResult{recipientCount}));
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

  std::uint64_t messageId = 0U;
  if (sendRequest.timestamp && !plan.recipients.empty()) {
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
        {});
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

  auto const result = registry_.retractTsoMessageForProducer(
      retractRequest.federationName,
      retractRequest.producingFederateId,
      retractRequest.messageId,
      {},
      false);
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
      processTsoMessageProducers_.erase(retractRequest.messageId);
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
    if (!state->second.interactionEvents.empty()) {
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
    }
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationReceiveInteractionResult(result));
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
    if (!state->second.attributeUpdateEvents.empty()) {
      result.event = std::move(state->second.attributeUpdateEvents.front());
      state->second.attributeUpdateEvents.pop_front();
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
