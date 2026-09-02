#include "internal/federation/federation_state_image.hpp"

#include "internal/runtime/utf8_string.hpp"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <limits>
#include <sstream>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace umbra::detail {
namespace {

constexpr char hexadecimal[] = "0123456789abcdef";

std::string hexEncode(std::string_view value) {
  std::string result;
  result.reserve(value.size() * 2U);
  for (unsigned char const byte : value) {
    result.push_back(hexadecimal[(byte >> 4U) & 0x0fU]);
    result.push_back(hexadecimal[byte & 0x0fU]);
  }
  return result;
}

unsigned char hexDigit(unsigned char value) {
  if (value >= '0' && value <= '9') {
    return static_cast<unsigned char>(value - '0');
  }
  if (value >= 'a' && value <= 'f') {
    return static_cast<unsigned char>(value - 'a' + 10U);
  }
  if (value >= 'A' && value <= 'F') {
    return static_cast<unsigned char>(value - 'A' + 10U);
  }
  throw std::runtime_error("Malformed hexadecimal value in Umbra state image.");
}

std::string hexDecode(std::string_view value) {
  if ((value.size() % 2U) != 0U) {
    throw std::runtime_error("Odd-length hexadecimal value in Umbra state image.");
  }
  std::string result;
  result.reserve(value.size() / 2U);
  for (std::size_t index = 0U; index < value.size(); index += 2U) {
    auto const high = hexDigit(static_cast<unsigned char>(value[index]));
    auto const low = hexDigit(static_cast<unsigned char>(value[index + 1U]));
    result.push_back(static_cast<char>((high << 4U) | low));
  }
  return result;
}

std::string encodeWide(std::wstring_view value) {
  auto const encoded = utf8FromWide(value);
  if (!encoded) {
    throw std::runtime_error("A state-image text field is not valid Unicode.");
  }
  return hexEncode(*encoded);
}

std::wstring decodeWide(std::string_view value) {
  auto const decoded = wideFromUtf8(hexDecode(value));
  if (!decoded) {
    throw std::runtime_error("A state-image text field is not valid UTF-8.");
  }
  return *decoded;
}

std::string encodeOptional(std::optional<std::string> const& value) {
  return value ? hexEncode(*value) : std::string{"-"};
}

std::optional<std::string> decodeOptional(std::string_view value) {
  if (value == "-") {
    return std::nullopt;
  }
  return hexDecode(value);
}

template <typename Integer>
Integer parseInteger(std::string_view value, char const* field) {
  Integer result{};
  if (value.empty()) {
    throw std::runtime_error(std::string{"Missing "} + field + " in Umbra state image.");
  }
  auto const* begin = value.data();
  auto const* end = value.data() + value.size();
  auto const parsed = std::from_chars(begin, end, result);
  if (parsed.ec != std::errc{} || parsed.ptr != end) {
    throw std::runtime_error(std::string{"Malformed "} + field + " in Umbra state image.");
  }
  return result;
}

std::vector<std::string_view> split(std::string_view value, char separator = '|') {
  std::vector<std::string_view> result;
  std::size_t begin = 0U;
  while (true) {
    auto const separatorPosition = value.find(separator, begin);
    if (separatorPosition == std::string_view::npos) {
      result.emplace_back(value.substr(begin));
      return result;
    }
    result.emplace_back(value.substr(begin, separatorPosition - begin));
    begin = separatorPosition + 1U;
  }
}

class Cursor final {
 public:
  explicit Cursor(std::string_view payload) : payload_(payload) {}

  [[nodiscard]] std::string_view line() {
    if (position_ > payload_.size()) {
      throw std::runtime_error("Trailing data in Umbra state image.");
    }
    if (position_ == payload_.size()) {
      throw std::runtime_error("Unexpected end of Umbra state image.");
    }
    auto const end = payload_.find('\n', position_);
    auto result = end == std::string_view::npos
        ? payload_.substr(position_)
        : payload_.substr(position_, end - position_);
    position_ = end == std::string_view::npos ? payload_.size() : end + 1U;
    if (!result.empty() && result.back() == '\r') {
      result.remove_suffix(1U);
    }
    return result;
  }

  [[nodiscard]] std::string_view valueFor(std::string_view key) {
    auto const current = line();
    auto const prefix = std::string{key} + "=";
    if (!current.starts_with(prefix)) {
      throw std::runtime_error(
          "Unexpected field in Umbra state image: " + std::string{current});
    }
    return current.substr(prefix.size());
  }

  void expect(std::string_view expected) {
    if (line() != expected) {
      throw std::runtime_error("Unexpected marker in Umbra state image.");
    }
  }

  void finish() {
    if (position_ != payload_.size()) {
      throw std::runtime_error("Trailing data in Umbra state image.");
    }
  }

 private:
  std::string_view payload_;
  std::size_t position_ = 0U;
};

void validateMemberVector(std::vector<FederationStateImageMember> const& members) {
  std::uint64_t previous = 0U;
  for (auto const& member : members) {
    if (member.id == 0U) {
      throw std::runtime_error("Invalid federate identity in Umbra state image.");
    }
    if (member.id <= previous) {
      throw std::runtime_error("Federate identities are not strictly ordered in Umbra state image.");
    }
    previous = member.id;

    std::uint64_t previousUpdateClassHandle = 0U;
    std::string previousTransportation;
    for (auto const& update : member.successfulUpdateCountsByClassAndTransportation) {
      if (update.objectClassHandle == 0U || update.count == 0U ||
          update.transportationName.empty() ||
          update.objectClassHandle < previousUpdateClassHandle ||
          (update.objectClassHandle == previousUpdateClassHandle &&
           update.transportationName <= previousTransportation)) {
        throw std::runtime_error(
            "Update telemetry buckets are not strictly ordered in Umbra state image.");
      }
      previousUpdateClassHandle = update.objectClassHandle;
      previousTransportation = update.transportationName;
    }
    std::uint64_t previousUpdatedObjectHandle = 0U;
    for (auto const objectInstanceHandle :
         member.successfullyUpdatedObjectInstanceHandles) {
      if (objectInstanceHandle == 0U ||
          objectInstanceHandle <= previousUpdatedObjectHandle) {
        throw std::runtime_error(
            "Updated object-instance handles are not strictly ordered in Umbra state image.");
      }
      previousUpdatedObjectHandle = objectInstanceHandle;
    }
    previousUpdatedObjectHandle = 0U;
    for (auto const& updatedObject :
         member.successfullyUpdatedObjectInstanceClassHandles) {
      if (updatedObject.objectInstanceHandle == 0U ||
          updatedObject.objectClassHandle == 0U ||
          updatedObject.objectInstanceHandle <= previousUpdatedObjectHandle) {
        throw std::runtime_error(
            "Updated object-class telemetry is not strictly ordered in Umbra state image.");
      }
      previousUpdatedObjectHandle = updatedObject.objectInstanceHandle;
    }

    std::uint64_t previousReflectionClassHandle = 0U;
    std::string previousReflectionTransportation;
    for (auto const& reflection :
         member.successfulReflectionCountsByClassAndTransportation) {
      if (reflection.objectClassHandle == 0U || reflection.count == 0U ||
          reflection.transportationName.empty() ||
          reflection.objectClassHandle < previousReflectionClassHandle ||
          (reflection.objectClassHandle == previousReflectionClassHandle &&
           reflection.transportationName <= previousReflectionTransportation)) {
        throw std::runtime_error(
            "Reflection telemetry buckets are not strictly ordered in Umbra state image.");
      }
      previousReflectionClassHandle = reflection.objectClassHandle;
      previousReflectionTransportation = reflection.transportationName;
    }
    previousUpdatedObjectHandle = 0U;
    for (auto const objectInstanceHandle :
         member.successfullyReflectedObjectInstanceHandles) {
      if (objectInstanceHandle == 0U ||
          objectInstanceHandle <= previousUpdatedObjectHandle) {
        throw std::runtime_error(
            "Reflected object-instance handles are not strictly ordered in Umbra state image.");
      }
      previousUpdatedObjectHandle = objectInstanceHandle;
    }
    previousUpdatedObjectHandle = 0U;
    for (auto const& reflectedObject :
         member.successfullyReflectedObjectInstanceClassHandles) {
      if (reflectedObject.objectInstanceHandle == 0U ||
          reflectedObject.objectClassHandle == 0U ||
          reflectedObject.objectInstanceHandle <= previousUpdatedObjectHandle) {
        throw std::runtime_error(
            "Reflected object-class telemetry is not strictly ordered in Umbra state image.");
      }
      previousUpdatedObjectHandle = reflectedObject.objectInstanceHandle;
    }

    std::uint64_t previousInteractionClassHandle = 0U;
    std::string previousInteractionTransportation;
    for (auto const& interaction :
         member.successfulInteractionCountsByClassAndTransportation) {
      if (interaction.interactionClassHandle == 0U || interaction.count == 0U ||
          interaction.transportationName.empty() ||
          interaction.interactionClassHandle < previousInteractionClassHandle ||
          (interaction.interactionClassHandle == previousInteractionClassHandle &&
           interaction.transportationName <= previousInteractionTransportation)) {
        throw std::runtime_error(
            "Interaction-send telemetry buckets are not strictly ordered in Umbra state image.");
      }
      previousInteractionClassHandle = interaction.interactionClassHandle;
      previousInteractionTransportation = interaction.transportationName;
    }
    previousInteractionClassHandle = 0U;
    previousInteractionTransportation.clear();
    for (auto const& interaction :
         member.successfulDirectedInteractionCountsByClassAndTransportation) {
      if (interaction.interactionClassHandle == 0U || interaction.count == 0U ||
          interaction.transportationName.empty() ||
          interaction.interactionClassHandle < previousInteractionClassHandle ||
          (interaction.interactionClassHandle == previousInteractionClassHandle &&
           interaction.transportationName <= previousInteractionTransportation)) {
        throw std::runtime_error(
            "Directed interaction-send telemetry buckets are not strictly ordered in Umbra state image.");
      }
      previousInteractionClassHandle = interaction.interactionClassHandle;
      previousInteractionTransportation = interaction.transportationName;
    }
    if (member.successfulDirectedInteractionsSentCount >
        member.successfulInteractionsSentCount) {
      throw std::runtime_error(
          "Directed interaction-send count exceeds total in Umbra state image.");
    }

    std::uint64_t previousReceiptClassHandle = 0U;
    std::string previousReceiptTransportation;
    for (auto const& interaction :
         member.successfulInteractionReceiptCountsByClassAndTransportation) {
      if (interaction.interactionClassHandle == 0U || interaction.count == 0U ||
          interaction.transportationName.empty() ||
          interaction.interactionClassHandle < previousReceiptClassHandle ||
          (interaction.interactionClassHandle == previousReceiptClassHandle &&
           interaction.transportationName <= previousReceiptTransportation)) {
        throw std::runtime_error(
            "Interaction-receipt telemetry buckets are not strictly ordered in Umbra state image.");
      }
      previousReceiptClassHandle = interaction.interactionClassHandle;
      previousReceiptTransportation = interaction.transportationName;
    }
    previousReceiptClassHandle = 0U;
    previousReceiptTransportation.clear();
    for (auto const& interaction :
         member.successfulDirectedInteractionReceiptCountsByClassAndTransportation) {
      if (interaction.interactionClassHandle == 0U || interaction.count == 0U ||
          interaction.transportationName.empty() ||
          interaction.interactionClassHandle < previousReceiptClassHandle ||
          (interaction.interactionClassHandle == previousReceiptClassHandle &&
           interaction.transportationName <= previousReceiptTransportation)) {
        throw std::runtime_error(
            "Directed interaction-receipt telemetry buckets are not strictly ordered in Umbra state image.");
      }
      previousReceiptClassHandle = interaction.interactionClassHandle;
      previousReceiptTransportation = interaction.transportationName;
    }
    if (member.successfulDirectedInteractionsReceivedCount >
        member.successfulInteractionsReceivedCount) {
      throw std::runtime_error(
          "Directed interaction-receipt count exceeds total in Umbra state image.");
    }
  }
}

void validateNameVector(
    std::vector<std::pair<std::uint64_t, std::wstring>> const& names) {
  std::uint64_t previous = 0U;
  for (auto const& [id, name] : names) {
    if (id == 0U || id <= previous) {
      throw std::runtime_error("Invalid federate name index in Umbra state image.");
    }
    previous = id;
  }
}

void validateObjectInstanceNameReservationVector(
    std::vector<FederationStateImageObjectInstanceNameReservation> const& reservations) {
  std::wstring previousName;
  for (auto const& reservation : reservations) {
    if (reservation.federateId == 0U || reservation.objectInstanceName.empty() ||
        (!previousName.empty() && reservation.objectInstanceName <= previousName)) {
      throw std::runtime_error(
          "Invalid object-instance-name reservation in Umbra state image.");
    }
    previousName = reservation.objectInstanceName;
  }
}

void validateSynchronizationPointVector(
    std::vector<FederationStateImageSynchronizationPoint> const& points) {
  std::wstring previousLabel;
  bool havePreviousLabel = false;
  auto validateIds = [](std::vector<std::uint64_t> const& ids,
                        char const* field) {
    std::uint64_t previous = 0U;
    for (auto const id : ids) {
      if (id == 0U || id <= previous) {
        throw std::runtime_error(std::string{field} +
                                 " are not strictly ordered in Umbra state image.");
      }
      previous = id;
    }
  };

  for (auto const& point : points) {
    if (havePreviousLabel && point.label <= previousLabel) {
      throw std::runtime_error(
          "Synchronization-point labels are not strictly ordered in Umbra state image.");
    }
    previousLabel = point.label;
    havePreviousLabel = true;
    validateIds(point.synchronizationSet, "Synchronization-point members");
    validateIds(point.announcedFederates, "Synchronization-point announced federates");
    std::uint64_t previousAchieved = 0U;
    for (auto const& [federateId, succeeded] : point.achievedFederates) {
      static_cast<void>(succeeded);
      if (federateId == 0U || federateId <= previousAchieved ||
          !std::binary_search(
              point.synchronizationSet.begin(),
              point.synchronizationSet.end(),
              federateId) ||
          !std::binary_search(
              point.announcedFederates.begin(),
              point.announcedFederates.end(),
              federateId)) {
        throw std::runtime_error(
            "Synchronization-point achievements are invalid in Umbra state image.");
      }
      previousAchieved = federateId;
    }
    for (auto const federateId : point.announcedFederates) {
      if (!std::binary_search(
              point.synchronizationSet.begin(),
              point.synchronizationSet.end(),
              federateId)) {
        throw std::runtime_error(
            "Synchronization-point announcements exceed the synchronization set.");
      }
    }
  }
}

void validateRegionVector(
    std::vector<FederationStateImageRegion> const& regions) {
  std::uint64_t previousRegionHandle = 0U;
  for (auto const& region : regions) {
    if (region.handle == 0U || region.handle <= previousRegionHandle ||
        region.ownerFederateId == 0U) {
      throw std::runtime_error(
          "Region identities are not strictly ordered in Umbra state image.");
    }
    previousRegionHandle = region.handle;

    std::uint64_t previousDimensionHandle = 0U;
    for (auto const dimensionHandle : region.dimensionHandles) {
      if (dimensionHandle == 0U || dimensionHandle <= previousDimensionHandle) {
        throw std::runtime_error(
            "Region dimensions are not strictly ordered in Umbra state image.");
      }
      previousDimensionHandle = dimensionHandle;
    }

    auto validateRanges = [&](std::vector<FederationStateImageRegionRange> const& ranges,
                              char const* field) {
      std::uint64_t previousRangeDimension = 0U;
      for (auto const& range : ranges) {
        if (range.dimensionHandle == 0U ||
            range.dimensionHandle <= previousRangeDimension ||
            range.lowerBound >= range.upperBound ||
            !std::binary_search(
                region.dimensionHandles.begin(),
                region.dimensionHandles.end(),
                range.dimensionHandle)) {
          throw std::runtime_error(std::string{field} +
                                   " are invalid in Umbra state image.");
        }
        previousRangeDimension = range.dimensionHandle;
      }
    };
    validateRanges(region.pendingRangeBounds, "Region pending ranges");
    validateRanges(region.committedRangeBounds, "Region committed ranges");
    if (region.specificationCommitted &&
        region.committedRangeBounds.size() != region.dimensionHandles.size()) {
      throw std::runtime_error(
          "A committed region has incomplete committed ranges in Umbra state image.");
    }
  }
}

void validateObjectClassAttributeDeclarationVector(
    std::vector<FederationStateImageObjectClassAttributeDeclarations> const& declarations) {
  auto validateHandles = [](std::vector<std::uint64_t> const& handles,
                            char const* field) {
    std::uint64_t previous = 0U;
    for (auto const handle : handles) {
      if (handle == 0U || handle <= previous) {
        throw std::runtime_error(std::string{field} +
                                 " are not strictly ordered in Umbra state image.");
      }
      previous = handle;
    }
  };
  std::uint64_t previousFederate = 0U;
  for (auto const& declaration : declarations) {
    if (declaration.federateId == 0U || declaration.federateId <= previousFederate) {
      throw std::runtime_error(
          "Invalid object-class declaration federate identity in Umbra state image.");
    }
    previousFederate = declaration.federateId;
    std::uint64_t previousClass = 0U;
    for (auto const& objectClass : declaration.classes) {
      if (objectClass.objectClassHandle == 0U ||
          objectClass.objectClassHandle <= previousClass) {
        throw std::runtime_error(
            "Object-class declarations are not strictly ordered in Umbra state image.");
      }
      previousClass = objectClass.objectClassHandle;
      validateHandles(
          objectClass.explicitlyPublishedAttributeHandles,
          "Published object-class attributes");

      std::uint64_t previousAttribute = 0U;
      for (auto const& subscription : objectClass.subscribedAttributes) {
        if (subscription.attributeHandle == 0U ||
            subscription.attributeHandle <= previousAttribute) {
          throw std::runtime_error(
              "Subscribed object-class attributes are not strictly ordered in Umbra state image.");
        }
        previousAttribute = subscription.attributeHandle;
      }

      previousAttribute = 0U;
      // Empty update-rate designators select the FOM default and remain a
      // valid, explicitly serialized value.
      for (auto const& value : objectClass.subscribedUpdateRateDesignators) {
        if (value.attributeHandle == 0U || value.attributeHandle <= previousAttribute) {
          throw std::runtime_error(
              "Subscribed update-rate designators are invalid in Umbra state image.");
        }
        previousAttribute = value.attributeHandle;
      }

      std::uint64_t previousRegion = 0U;
      previousAttribute = 0U;
      for (auto const& subscription : objectClass.regionalSubscribedAttributes) {
        if (subscription.attributeHandle == 0U || subscription.regionHandle == 0U ||
            subscription.attributeHandle < previousAttribute ||
            (subscription.attributeHandle == previousAttribute &&
             subscription.regionHandle <= previousRegion)) {
          throw std::runtime_error(
              "Regional object-class attributes are not strictly ordered in Umbra state image.");
        }
        previousAttribute = subscription.attributeHandle;
        previousRegion = subscription.regionHandle;
      }

      previousRegion = 0U;
      previousAttribute = 0U;
      // Regional subscriptions use the same empty-string default-rate
      // convention as ordinary subscriptions.
      for (auto const& value : objectClass.regionalSubscribedUpdateRateDesignators) {
        if (value.attributeHandle == 0U || value.regionHandle == 0U ||
            value.attributeHandle < previousAttribute ||
            (value.attributeHandle == previousAttribute &&
             value.regionHandle <= previousRegion)) {
          throw std::runtime_error(
              "Regional update-rate designators are invalid in Umbra state image.");
        }
        previousAttribute = value.attributeHandle;
        previousRegion = value.regionHandle;
      }

      previousAttribute = 0U;
      for (auto const& value : objectClass.defaultTransportationTypes) {
        if (value.attributeHandle == 0U || value.attributeHandle <= previousAttribute ||
            value.value.empty()) {
          throw std::runtime_error(
              "Default attribute transportation types are invalid in Umbra state image.");
        }
        previousAttribute = value.attributeHandle;
      }

      previousAttribute = 0U;
      for (auto const& value : objectClass.defaultOrderTypes) {
        if (value.attributeHandle == 0U || value.attributeHandle <= previousAttribute ||
            value.orderType == 0U || value.orderType > 2U) {
          throw std::runtime_error(
              "Default attribute order types are invalid in Umbra state image.");
        }
        previousAttribute = value.attributeHandle;
      }
    }
  }
}

void validateTimeVector(std::vector<FederationStateImageTimeState> const& states) {
  std::uint64_t previous = 0U;
  for (auto const& state : states) {
    if (state.federateId == 0U || state.federateId <= previous ||
        state.advanceMode > 5U) {
      throw std::runtime_error("Invalid time state in Umbra state image.");
    }
    if (state.applicationRequestLedgerPresent && state.nextGeneration == 0U) {
      throw std::runtime_error(
          "Invalid pending application request generation floor in Umbra state image.");
    }
    if (state.applicationRequestLedgerPresent) {
      bool const advancePending = (state.flags & (1U << 4U)) != 0U;
      bool const regulationPending = (state.flags & (1U << 5U)) != 0U;
      bool const constrainedPending = (state.flags & (1U << 6U)) != 0U;
      if (advancePending != (state.pendingGeneration != 0U) ||
          regulationPending != (state.pendingTimeRegulationGeneration != 0U) ||
          constrainedPending !=
              (state.pendingTimeConstrainedGeneration != 0U)) {
        throw std::runtime_error(
            "Pending application request flags and generations disagree in Umbra state image.");
      }
      if (state.pendingModifiedLookaheadEncoding.has_value() &&
          (state.flags & (1U << 1U)) == 0U) {
        throw std::runtime_error(
            "Deferred lookahead request is inconsistent with the saved time state.");
      }
    }
    previous = state.federateId;
  }
}

void validateObjectVector(std::vector<FederationStateImageObject> const& objects) {
  std::uint64_t previousObject = 0U;
  for (auto const& object : objects) {
    if (object.handle == 0U || object.handle <= previousObject) {
      throw std::runtime_error("Invalid object identity in Umbra state image.");
    }
    previousObject = object.handle;
    std::uint64_t previousAttribute = 0U;
    std::set<std::uint64_t> attributeHandles;
    for (auto const& attribute : object.attributes) {
      if (attribute.handle == 0U || attribute.handle <= previousAttribute) {
        throw std::runtime_error(
            "Invalid object attribute identity in Umbra state image.");
      }
      previousAttribute = attribute.handle;
      attributeHandles.insert(attribute.handle);
      if (!std::is_sorted(
              attribute.updateRegionHandles.begin(),
              attribute.updateRegionHandles.end()) ||
          std::adjacent_find(
              attribute.updateRegionHandles.begin(),
              attribute.updateRegionHandles.end()) !=
              attribute.updateRegionHandles.end()) {
        throw std::runtime_error(
            "Object attribute regions are not strictly ordered in Umbra state image.");
      }
    }
    std::uint64_t previousAttributeValue = 0U;
    for (auto const& value : object.attributeValues) {
      if (value.attributeHandle == 0U ||
          value.attributeHandle <= previousAttributeValue ||
          !attributeHandles.contains(value.attributeHandle)) {
        throw std::runtime_error(
            "Object application values are not strictly ordered or have unknown attributes in Umbra state image.");
      }
      previousAttributeValue = value.attributeHandle;
    }

    auto const typedPendingCount =
        object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.size() +
        object.pendingAttributeOwnershipAcquisitionRequests.size() +
        object.pendingAttributeOwnershipAcquisitionCancellations.size() +
        object.pendingAttributeOwnershipDivestitureIfWantedNotifications.size() +
        object.pendingConfirmDivestitureNotifications.size() +
        object.pendingAttributeTransportationTypeChanges.size() +
        object.pendingAttributeValueUpdateRequests.size() +
        object.pendingAttributeValueUpdateClassRequests.size() +
        object.pendingAttributeValueUpdateRegionalRequests.size() +
        object.pendingNegotiatedAttributeOwnershipDivestitures.size() +
        object.ownershipAssumptionRecipientsByAttribute.size() +
        object.ownershipAssumptionUserSuppliedTagsByAttribute.size() +
        object.pendingDiscoveryFederateIds.size() +
        object.pendingRemovalFederateIds.size() +
        object.connectionLossAutomaticRemovalFederateIds.size() +
        object.deferredConnectionLossTsoRemovalFederateIds.size() +
        object.pendingTimestampedRemovalFederateIds.size() +
        (object.pendingTimestampedDeletionMessageId.has_value() ? 1U : 0U);
    if (typedPendingCount > object.pendingOperationCount) {
      throw std::runtime_error(
          "Typed ownership reservations exceed the pending-operation fence in Umbra state image.");
    }
    std::uint64_t previousRequestId = 0U;
    for (auto const& request :
         object.pendingAttributeOwnershipAcquisitionIfAvailableRequests) {
      if (request.requestId == 0U || request.requestId <= previousRequestId ||
          request.requestingFederateId == 0U || request.requestSequence == 0U ||
          request.desiredAttributeHandles.empty()) {
        throw std::runtime_error(
            "If Available ownership reservations are not strictly ordered in Umbra state image.");
      }
      previousRequestId = request.requestId;
      std::uint64_t previousHandle = 0U;
      for (auto const attributeHandle : request.desiredAttributeHandles) {
        if (attributeHandle == 0U || attributeHandle <= previousHandle) {
          throw std::runtime_error(
              "If Available ownership reservation attributes are not strictly ordered in Umbra state image.");
        }
        previousHandle = attributeHandle;
      }
    }

    auto validateAttributeHandleList = [](std::vector<std::uint64_t> const& handles,
                                         char const* field) {
      std::uint64_t previousHandle = 0U;
      for (auto const attributeHandle : handles) {
        if (attributeHandle == 0U || attributeHandle <= previousHandle) {
          throw std::runtime_error(std::string{field} +
                                   " are not strictly ordered in Umbra state image.");
        }
        previousHandle = attributeHandle;
      }
    };
    std::uint64_t previousAttributeValueUpdateRequestId = 0U;
    for (auto const& request : object.pendingAttributeValueUpdateRequests) {
      if (request.requestId == 0U ||
          request.requestId <= previousAttributeValueUpdateRequestId ||
          request.requestingFederateId == 0U ||
          request.providingFederateId == 0U ||
          request.requestedAttributeHandles.empty()) {
        throw std::runtime_error(
            "Pending attribute value update requests are not strictly ordered in Umbra state image.");
      }
      previousAttributeValueUpdateRequestId = request.requestId;
      validateAttributeHandleList(
          request.requestedAttributeHandles,
          "Pending attribute value update request attributes");
    }
    std::uint64_t previousClassRequestId = 0U;
    for (auto const& request : object.pendingAttributeValueUpdateClassRequests) {
      if (request.requestId == 0U ||
          request.requestId <= previousClassRequestId ||
          request.requestingFederateId == 0U ||
          request.providingFederateId == 0U ||
          request.requestedObjectClassHandle == 0U ||
          request.requestedAttributeHandles.empty()) {
        throw std::runtime_error(
            "Pending object-class attribute value update requests are not strictly ordered in Umbra state image.");
      }
      previousClassRequestId = request.requestId;
      validateAttributeHandleList(
          request.requestedAttributeHandles,
          "Pending object-class attribute value update request attributes");
    }
    std::uint64_t previousRegionalRequestId = 0U;
    for (auto const& request : object.pendingAttributeValueUpdateRegionalRequests) {
      if (request.requestId == 0U ||
          request.requestId <= previousRegionalRequestId ||
          request.requestingFederateId == 0U ||
          request.providingFederateId == 0U ||
          request.requestedObjectClassHandle == 0U ||
          request.requestedAttributeHandles.empty()) {
        throw std::runtime_error(
            "Pending regional attribute value update requests are not strictly ordered in Umbra state image.");
      }
      previousRegionalRequestId = request.requestId;
      validateAttributeHandleList(
          request.requestedAttributeHandles,
          "Pending regional attribute value update request attributes");
      std::uint64_t previousAttributeHandle = 0U;
      for (auto const& [attributeHandle, regionHandles] :
           request.requestRegionsByAttribute) {
        if (attributeHandle == 0U || attributeHandle <= previousAttributeHandle ||
            !std::binary_search(
                request.requestedAttributeHandles.begin(),
                request.requestedAttributeHandles.end(),
                attributeHandle)) {
          throw std::runtime_error(
              "Pending regional attribute value update request regions are invalid in Umbra state image.");
        }
        previousAttributeHandle = attributeHandle;
        validateAttributeHandleList(
            regionHandles,
            "Pending regional attribute value update request regions");
      }
    }
    std::uint64_t previousRegularRequestId = 0U;
    for (auto const& request : object.pendingAttributeOwnershipAcquisitionRequests) {
      if (request.requestId == 0U || request.requestId <= previousRegularRequestId ||
          request.requestingFederateId == 0U || request.requestSequence == 0U ||
          request.desiredAttributeHandles.empty()) {
        throw std::runtime_error(
            "Regular ownership reservations are not strictly ordered in Umbra state image.");
      }
      previousRegularRequestId = request.requestId;
      validateAttributeHandleList(
          request.desiredAttributeHandles,
          "Regular ownership reservation attributes");
      validateAttributeHandleList(
          request.notificationQueuedAttributeHandles,
          "Regular ownership queued notification attributes");
      validateAttributeHandleList(
          request.unavailableQueuedAttributeHandles,
          "Regular ownership unavailable attributes");
      std::uint64_t previousOwner = 0U;
      for (auto const& [ownerFederateId, attributes] :
           request.releaseCallbacksQueuedByOwningFederate) {
        if (ownerFederateId == 0U || ownerFederateId <= previousOwner ||
            attributes.empty()) {
          throw std::runtime_error(
              "Regular ownership release callbacks are not strictly ordered in Umbra state image.");
        }
        previousOwner = ownerFederateId;
        validateAttributeHandleList(
            attributes,
            "Regular ownership release callback attributes");
      }
    }
    std::uint64_t previousCancellationId = 0U;
    for (auto const& cancellation :
         object.pendingAttributeOwnershipAcquisitionCancellations) {
      if (cancellation.cancellationId == 0U ||
          cancellation.cancellationId <= previousCancellationId ||
          cancellation.requestingFederateId == 0U ||
          cancellation.attributeHandles.empty()) {
        throw std::runtime_error(
            "Ownership acquisition cancellations are not strictly ordered in Umbra state image.");
      }
      previousCancellationId = cancellation.cancellationId;
      validateAttributeHandleList(
          cancellation.attributeHandles,
          "Ownership acquisition cancellation attributes");
    }
    std::uint64_t previousNotificationId = 0U;
    for (auto const& notification :
         object.pendingAttributeOwnershipDivestitureIfWantedNotifications) {
      if (notification.notificationId == 0U ||
          notification.notificationId <= previousNotificationId ||
          notification.receivingFederateId == 0U ||
          notification.attributeHandles.empty()) {
        throw std::runtime_error(
            "Divestiture If Wanted notifications are not strictly ordered in Umbra state image.");
      }
      previousNotificationId = notification.notificationId;
      validateAttributeHandleList(
          notification.attributeHandles,
          "Divestiture If Wanted notification attributes");
    }
    std::uint64_t previousConfirmNotificationId = 0U;
    for (auto const& notification : object.pendingConfirmDivestitureNotifications) {
      if (notification.notificationId == 0U ||
          notification.notificationId <= previousConfirmNotificationId ||
          notification.receivingFederateId == 0U ||
          notification.attributeHandles.empty()) {
        throw std::runtime_error(
            "Confirm Divestiture notifications are not strictly ordered in Umbra state image.");
      }
      previousConfirmNotificationId = notification.notificationId;
      validateAttributeHandleList(
          notification.attributeHandles,
          "Confirm Divestiture notification attributes");
    }
    std::uint64_t previousTransportationRequestId = 0U;
    for (auto const& request : object.pendingAttributeTransportationTypeChanges) {
      if (request.requestId == 0U ||
          request.requestId <= previousTransportationRequestId ||
          request.requestingFederateId == 0U ||
          request.attributeHandles.empty() ||
          request.transportationName.empty()) {
        throw std::runtime_error(
            "Attribute transportation-type changes are not strictly ordered in Umbra state image.");
      }
      previousTransportationRequestId = request.requestId;
      validateAttributeHandleList(
          request.attributeHandles,
          "Attribute transportation-type change attributes");
    }
    std::uint64_t previousNegotiatedAttributeHandle = 0U;
    for (auto const& divestiture : object.pendingNegotiatedAttributeOwnershipDivestitures) {
      if (divestiture.attributeHandle == 0U ||
          divestiture.attributeHandle <= previousNegotiatedAttributeHandle ||
          divestiture.divestingFederateId == 0U ||
          (divestiture.acquiringFederateId == 0U &&
           (divestiture.acquisitionRequestId != 0U ||
            divestiture.acquiringFederateIsIfAvailable ||
            divestiture.confirmationQueued ||
            divestiture.confirmationDelivered)) ||
          (divestiture.acquiringFederateId != 0U &&
           divestiture.acquisitionRequestId == 0U) ||
          divestiture.confirmationDelivered && !divestiture.confirmationQueued) {
        throw std::runtime_error(
            "Negotiated ownership divestitures are not valid in Umbra state image.");
      }
      previousNegotiatedAttributeHandle = divestiture.attributeHandle;
    }
    std::uint64_t previousAssumptionRecipientAttribute = 0U;
    for (auto const& assumption : object.ownershipAssumptionRecipientsByAttribute) {
      if (assumption.attributeHandle == 0U ||
          assumption.attributeHandle <= previousAssumptionRecipientAttribute) {
        throw std::runtime_error(
            "Ownership-assumption recipient ledgers are not strictly ordered in Umbra state image.");
      }
      previousAssumptionRecipientAttribute = assumption.attributeHandle;
      validateAttributeHandleList(
          assumption.recipientFederateIds,
          "Ownership-assumption recipient federates");
    }
    std::uint64_t previousAssumptionTagAttribute = 0U;
    for (auto const& assumption : object.ownershipAssumptionUserSuppliedTagsByAttribute) {
      if (assumption.attributeHandle == 0U ||
          assumption.attributeHandle <= previousAssumptionTagAttribute) {
        throw std::runtime_error(
            "Ownership-assumption tag ledgers are not strictly ordered in Umbra state image.");
      }
      previousAssumptionTagAttribute = assumption.attributeHandle;
    }
    std::uint64_t previousKnownFederateId = 0U;
    for (auto const& known : object.knownObjectClassHandlesByFederate) {
      if (known.federateId == 0U || known.federateId <= previousKnownFederateId ||
          known.objectClassHandle == 0U) {
        throw std::runtime_error(
            "Known object-class projections are not strictly ordered in Umbra state image.");
      }
      previousKnownFederateId = known.federateId;
    }
    auto validateFederateIdList = [](std::vector<std::uint64_t> const& federateIds,
                                     char const* field) {
      std::uint64_t previousFederateId = 0U;
      for (auto const federateId : federateIds) {
        if (federateId == 0U || federateId <= previousFederateId) {
          throw std::runtime_error(std::string{field} +
                                   " are not strictly ordered in Umbra state image.");
        }
        previousFederateId = federateId;
      }
    };
    validateFederateIdList(
        object.pendingDiscoveryFederateIds,
        "Pending object discovery federates");
    validateFederateIdList(
        object.pendingRemovalFederateIds,
        "Pending object removal federates");
    validateFederateIdList(
        object.connectionLossAutomaticRemovalFederateIds,
        "Connection-loss automatic object removal federates");
    validateFederateIdList(
        object.deferredConnectionLossTsoRemovalFederateIds,
        "Deferred connection-loss TSO object removal federates");
    validateFederateIdList(
        object.pendingTimestampedRemovalFederateIds,
        "Pending timestamped object removal federates");
    auto containsFederateId = [](std::vector<std::uint64_t> const& values,
                                 std::uint64_t federateId) {
      return std::binary_search(values.begin(), values.end(), federateId);
    };
    for (auto const federateId : object.connectionLossAutomaticRemovalFederateIds) {
      if (!containsFederateId(object.pendingRemovalFederateIds, federateId)) {
        throw std::runtime_error(
            "Connection-loss automatic removals must remain pending in Umbra state image.");
      }
    }
    for (auto const federateId : object.deferredConnectionLossTsoRemovalFederateIds) {
      if (!containsFederateId(object.pendingRemovalFederateIds, federateId) ||
          !containsFederateId(
              object.connectionLossAutomaticRemovalFederateIds,
              federateId)) {
        throw std::runtime_error(
            "Deferred connection-loss removals must retain their automatic-removal classification in Umbra state image.");
      }
    }
    if (!object.pendingTimestampedDeletionMessageId.has_value() &&
        !object.pendingTimestampedRemovalFederateIds.empty()) {
      throw std::runtime_error(
          "Pending timestamped removals require a deletion message identity in Umbra state image.");
    }
    if (object.pendingTimestampedDeletionMessageId.has_value() &&
        *object.pendingTimestampedDeletionMessageId == 0U) {
      throw std::runtime_error(
          "Invalid pending timestamped deletion message identity in Umbra state image.");
    }
  }
}

void validatePendingAttributeOwnershipQueryVector(
    std::vector<FederationStateImagePendingAttributeOwnershipQuery> const& queries) {
  std::uint64_t previousRequestId = 0U;
  for (auto const& query : queries) {
    if (query.requestId == 0U || query.requestId <= previousRequestId ||
        query.requestingFederateId == 0U || query.objectInstanceHandle == 0U ||
        query.reportKind > 2U ||
        (query.reportKind == 0U && query.owningFederateId == 0U) ||
        (query.reportKind != 0U && query.owningFederateId != 0U) ||
        query.requestedAttributeHandles.empty() ||
        !std::is_sorted(
            query.requestedAttributeHandles.begin(),
            query.requestedAttributeHandles.end()) ||
        std::adjacent_find(
            query.requestedAttributeHandles.begin(),
            query.requestedAttributeHandles.end()) !=
            query.requestedAttributeHandles.end() ||
        query.requestedAttributeHandles.front() == 0U) {
      throw std::runtime_error(
          "Pending Attribute Ownership queries are not valid in Umbra state image.");
    }
    previousRequestId = query.requestId;
  }
}

void validatePendingAttributeOwnershipAssumptionVector(
    std::vector<FederationStateImagePendingAttributeOwnershipAssumption> const& callbacks) {
  std::tuple<std::uint64_t, std::uint64_t, std::vector<std::uint64_t>, std::string>
      previous{};
  bool first = true;
  for (auto const& callback : callbacks) {
    if (callback.objectInstanceHandle == 0U ||
        callback.receivingFederateId == 0U ||
        callback.attributeHandles.empty() ||
        !std::is_sorted(
            callback.attributeHandles.begin(),
            callback.attributeHandles.end()) ||
        std::adjacent_find(
            callback.attributeHandles.begin(),
            callback.attributeHandles.end()) != callback.attributeHandles.end() ||
        callback.attributeHandles.front() == 0U) {
      throw std::runtime_error(
          "Pending Attribute Ownership Assumption callbacks are not valid in Umbra state image.");
    }
    auto const current = std::tuple{
        callback.objectInstanceHandle,
        callback.receivingFederateId,
        callback.attributeHandles,
        callback.userSuppliedTag};
    if (!first && !(previous < current)) {
      throw std::runtime_error(
          "Pending Attribute Ownership Assumption callbacks are not strictly ordered in Umbra state image.");
    }
    previous = current;
    first = false;
  }
}

void validateInteractionDeclarationVector(
    std::vector<FederationStateImageInteractionDeclaration> const& declarations) {
  std::uint64_t previousFederate = 0U;
  for (auto const& declaration : declarations) {
    if (declaration.federateId == 0U || declaration.federateId <= previousFederate) {
      throw std::runtime_error(
          "Invalid interaction declaration federate identity in Umbra state image.");
    }
    previousFederate = declaration.federateId;

    std::uint64_t previousHandle = 0U;
    for (auto const handle : declaration.publishedInteractionClasses) {
      if (handle == 0U || handle <= previousHandle) {
        throw std::runtime_error(
            "Published interaction classes are not strictly ordered in Umbra state image.");
      }
      previousHandle = handle;
    }

    previousHandle = 0U;
    for (auto const& subscription : declaration.subscribedInteractionClasses) {
      if (subscription.interactionClassHandle == 0U ||
          subscription.interactionClassHandle <= previousHandle) {
        throw std::runtime_error(
            "Subscribed interaction classes are not strictly ordered in Umbra state image.");
      }
      previousHandle = subscription.interactionClassHandle;
    }

    std::uint64_t previousInteraction = 0U;
    std::uint64_t previousRegion = 0U;
    for (auto const& subscription : declaration.regionalSubscribedInteractionClasses) {
      if (subscription.interactionClassHandle == 0U || subscription.regionHandle == 0U ||
          (subscription.interactionClassHandle < previousInteraction) ||
          (subscription.interactionClassHandle == previousInteraction &&
           subscription.regionHandle <= previousRegion)) {
        throw std::runtime_error(
            "Regional interaction subscriptions are not strictly ordered in Umbra state image.");
      }
      previousInteraction = subscription.interactionClassHandle;
      previousRegion = subscription.regionHandle;
    }

    std::uint64_t previousObjectClass = 0U;
    previousInteraction = 0U;
    for (auto const& directed : declaration.publishedObjectClassDirectedInteractions) {
      if (directed.objectClassHandle == 0U || directed.interactionClassHandle == 0U ||
          directed.objectClassHandle < previousObjectClass ||
          (directed.objectClassHandle == previousObjectClass &&
           directed.interactionClassHandle <= previousInteraction)) {
        throw std::runtime_error(
            "Published directed interactions are not strictly ordered in Umbra state image.");
      }
      previousObjectClass = directed.objectClassHandle;
      previousInteraction = directed.interactionClassHandle;
    }

    previousObjectClass = 0U;
    previousInteraction = 0U;
    for (auto const& directed : declaration.subscribedObjectClassDirectedInteractions) {
      if (directed.objectClassHandle == 0U || directed.interactionClassHandle == 0U ||
          directed.objectClassHandle < previousObjectClass ||
          (directed.objectClassHandle == previousObjectClass &&
           directed.interactionClassHandle <= previousInteraction)) {
        throw std::runtime_error(
            "Subscribed directed interactions are not strictly ordered in Umbra state image.");
      }
      previousObjectClass = directed.objectClassHandle;
      previousInteraction = directed.interactionClassHandle;
    }

    previousHandle = 0U;
    for (auto const& value : declaration.interactionTransportationTypes) {
      if (value.interactionClassHandle == 0U || value.interactionClassHandle <= previousHandle) {
        throw std::runtime_error(
            "Interaction transportation types are not strictly ordered in Umbra state image.");
      }
      previousHandle = value.interactionClassHandle;
    }

    previousHandle = 0U;
    for (auto const& value : declaration.interactionOrderTypes) {
      if (value.interactionClassHandle == 0U || value.interactionClassHandle <= previousHandle) {
        throw std::runtime_error(
            "Interaction order types are not strictly ordered in Umbra state image.");
      }
      previousHandle = value.interactionClassHandle;
    }

    previousHandle = 0U;
    for (auto const& value : declaration.pendingInteractionTransportationTypeChanges) {
      if (value.interactionClassHandle == 0U || value.interactionClassHandle <= previousHandle) {
        throw std::runtime_error(
            "Pending interaction transportation changes are not strictly ordered in Umbra state image.");
      }
      previousHandle = value.interactionClassHandle;
    }
  }
}

void validateTsoInteractionMessageVector(
    std::vector<FederationStateImageTsoInteractionMessage> const& messages) {
  std::uint64_t previousMessageId = 0U;
  for (auto const& message : messages) {
    if (message.messageId == 0U || message.messageId <= previousMessageId ||
        message.producingFederateId == 0U ||
        message.sentInteractionClassHandle == 0U) {
      throw std::runtime_error(
          "Invalid TSO interaction message identity in Umbra state image.");
    }
    previousMessageId = message.messageId;

    std::uint64_t previousHandle = 0U;
    for (auto const handle : message.sentParameterHandles) {
      if (handle == 0U || handle <= previousHandle) {
        throw std::runtime_error(
            "TSO interaction parameter handles are not strictly ordered in Umbra state image.");
      }
      previousHandle = handle;
    }

    previousHandle = 0U;
    for (auto const& parameter : message.parameters) {
      if (parameter.parameterHandle == 0U || parameter.parameterHandle <= previousHandle) {
        throw std::runtime_error(
            "TSO interaction parameters are not strictly ordered in Umbra state image.");
      }
      previousHandle = parameter.parameterHandle;
    }

    previousHandle = 0U;
    for (auto const handle : message.sentRegionHandles) {
      if (handle == 0U || handle <= previousHandle) {
        throw std::runtime_error(
            "TSO interaction region handles are not strictly ordered in Umbra state image.");
      }
      previousHandle = handle;
    }

    previousHandle = 0U;
    for (auto const& snapshot : message.sentRegionSnapshots) {
      if (snapshot.regionHandle == 0U || snapshot.regionHandle <= previousHandle) {
        throw std::runtime_error(
            "TSO interaction region snapshots are not strictly ordered in Umbra state image.");
      }
      previousHandle = snapshot.regionHandle;

      std::uint64_t previousDimension = 0U;
      for (auto const dimension : snapshot.dimensionHandles) {
        if (dimension == 0U || dimension <= previousDimension) {
          throw std::runtime_error(
              "TSO interaction region dimensions are not strictly ordered in Umbra state image.");
        }
        previousDimension = dimension;
      }

      previousDimension = 0U;
      for (auto const& range : snapshot.committedRangeBounds) {
        if (range.dimensionHandle == 0U || range.dimensionHandle <= previousDimension ||
            range.lowerBound > range.upperBound) {
          throw std::runtime_error(
              "TSO interaction region ranges are invalid in Umbra state image.");
        }
        previousDimension = range.dimensionHandle;
      }
    }
  }
}

void validateTsoInteractionRegionSnapshot(
    FederationStateImageInteractionRegionSnapshot const& snapshot) {
  if (snapshot.regionHandle == 0U ||
      snapshot.dimensionHandles.size() != snapshot.committedRangeBounds.size() ||
      !std::is_sorted(
          snapshot.dimensionHandles.begin(),
          snapshot.dimensionHandles.end()) ||
      std::adjacent_find(
          snapshot.dimensionHandles.begin(),
          snapshot.dimensionHandles.end()) != snapshot.dimensionHandles.end() ||
      !std::is_sorted(
          snapshot.committedRangeBounds.begin(),
          snapshot.committedRangeBounds.end(),
          [](auto const& first, auto const& second) {
            return first.dimensionHandle < second.dimensionHandle;
          }) ||
      std::adjacent_find(
          snapshot.committedRangeBounds.begin(),
          snapshot.committedRangeBounds.end(),
          [](auto const& first, auto const& second) {
            return first.dimensionHandle == second.dimensionHandle;
          }) != snapshot.committedRangeBounds.end()) {
    throw std::runtime_error(
        "Invalid TSO attribute-update region snapshot in Umbra state image.");
  }
  std::set<std::uint64_t> dimensions(
      snapshot.dimensionHandles.begin(),
      snapshot.dimensionHandles.end());
  if (dimensions.size() != snapshot.dimensionHandles.size() ||
      dimensions.contains(0U)) {
    throw std::runtime_error(
        "Invalid TSO attribute-update region dimensions in Umbra state image.");
  }
  for (auto const& range : snapshot.committedRangeBounds) {
    if (range.dimensionHandle == 0U ||
        !dimensions.contains(range.dimensionHandle) ||
        range.lowerBound > range.upperBound) {
      throw std::runtime_error(
          "Invalid TSO attribute-update region range in Umbra state image.");
    }
  }
}

void validateTsoAttributeUpdateMessageVector(
    std::vector<FederationStateImageTsoAttributeUpdateMessage> const& messages) {
  std::uint64_t previousMessageId = 0U;
  for (auto const& message : messages) {
    if (message.messageId == 0U || message.messageId <= previousMessageId ||
        message.producingFederateId == 0U ||
        message.objectInstanceHandle == 0U ||
        !message.timestampEncoding.has_value()) {
      throw std::runtime_error(
          "Invalid TSO attribute-update message identity in Umbra state image.");
    }
    previousMessageId = message.messageId;

    std::uint64_t previousHandle = 0U;
    for (auto const& attribute : message.attributes) {
      if (attribute.attributeHandle == 0U ||
          attribute.attributeHandle <= previousHandle) {
        throw std::runtime_error(
            "TSO attribute-update attributes are not strictly ordered in Umbra state image.");
      }
      previousHandle = attribute.attributeHandle;
    }

    std::uint64_t previousRecipient = 0U;
    std::set<std::uint64_t> explicitRegions;
    for (auto const& recipient : message.passelsByRecipient) {
      if (recipient.receivingFederateId == 0U ||
          recipient.receivingFederateId <= previousRecipient) {
        throw std::runtime_error(
            "TSO attribute-update recipients are not strictly ordered in Umbra state image.");
      }
      previousRecipient = recipient.receivingFederateId;
      for (auto const& passel : recipient.passels) {
        if ((passel.preferredOrderType != 1U &&
             passel.preferredOrderType != 2U) ||
            !std::is_sorted(
                passel.sentAttributeHandles.begin(),
                passel.sentAttributeHandles.end()) ||
            std::adjacent_find(
                passel.sentAttributeHandles.begin(),
                passel.sentAttributeHandles.end()) !=
                passel.sentAttributeHandles.end() ||
            !std::is_sorted(
                passel.sentRegionHandles.begin(),
                passel.sentRegionHandles.end()) ||
            std::adjacent_find(
                passel.sentRegionHandles.begin(),
                passel.sentRegionHandles.end()) !=
                passel.sentRegionHandles.end()) {
          throw std::runtime_error(
              "TSO attribute-update passel handles are not strictly ordered in Umbra state image.");
        }
        for (auto const handle : passel.sentAttributeHandles) {
          if (handle == 0U) {
            throw std::runtime_error(
                "TSO attribute-update passel has an invalid attribute handle in Umbra state image.");
          }
        }
        for (auto const handle : passel.sentRegionHandles) {
          if (handle == 0U || !explicitRegions.insert(handle).second) {
            // A region may be shared by several recipient passels; keep the
            // set only as an identity check for the message-level snapshots.
            if (handle == 0U) {
              throw std::runtime_error(
                  "TSO attribute-update passel has an invalid region handle in Umbra state image.");
            }
          }
        }
        std::uint64_t previousRegion = 0U;
        for (auto const& snapshot : passel.sentRegionSnapshots) {
          if (snapshot.regionHandle == 0U ||
              snapshot.regionHandle <= previousRegion ||
              !std::binary_search(
                  passel.sentRegionHandles.begin(),
                  passel.sentRegionHandles.end(),
                  snapshot.regionHandle)) {
            throw std::runtime_error(
                "TSO attribute-update passel region snapshots are not strictly ordered in Umbra state image.");
          }
          previousRegion = snapshot.regionHandle;
          validateTsoInteractionRegionSnapshot(snapshot);
        }
        if (passel.sentRegionSnapshots.size() !=
            passel.sentRegionHandles.size()) {
          throw std::runtime_error(
              "TSO attribute-update passel region snapshots do not match their handles in Umbra state image.");
        }
      }
    }

    std::uint64_t previousRegion = 0U;
    for (auto const& snapshot : message.sentRegionSnapshots) {
      if (snapshot.regionHandle == 0U ||
          snapshot.regionHandle <= previousRegion ||
          !explicitRegions.contains(snapshot.regionHandle)) {
        throw std::runtime_error(
            "TSO attribute-update message region snapshots are not strictly ordered in Umbra state image.");
      }
      previousRegion = snapshot.regionHandle;
      validateTsoInteractionRegionSnapshot(snapshot);
    }
    if (message.sentRegionSnapshots.size() != explicitRegions.size()) {
      throw std::runtime_error(
          "TSO attribute-update message region snapshots do not match their passels in Umbra state image.");
    }
  }
}

void validateTsoObjectDeletionMessageVector(
    std::vector<FederationStateImageTsoObjectDeletionMessage> const& messages) {
  std::uint64_t previousMessageId = 0U;
  for (auto const& message : messages) {
    if (message.messageId == 0U || message.messageId <= previousMessageId ||
        message.producingFederateId == 0U ||
        message.objectInstanceHandle == 0U ||
        !message.timestampEncoding.has_value()) {
      throw std::runtime_error(
          "Invalid TSO object-deletion message identity in Umbra state image.");
    }
    previousMessageId = message.messageId;

    std::uint64_t previousRecipient = 0U;
    for (auto const& recipient : message.recipients) {
      if (recipient.receivingFederateId == 0U ||
          recipient.receivingFederateId <= previousRecipient ||
          recipient.receivingFederateId == message.producingFederateId ||
          recipient.objectInstanceHandle != message.objectInstanceHandle) {
        throw std::runtime_error(
            "TSO object-deletion recipients are not strictly ordered in Umbra state image.");
      }
      previousRecipient = recipient.receivingFederateId;
    }

    if (!message.reconstitution.has_value()) {
      throw std::runtime_error(
          "TSO object-deletion message has no invocation snapshot in Umbra state image.");
    }
    auto const& reconstitution = *message.reconstitution;
    if (reconstitution.object.handle != message.objectInstanceHandle ||
        reconstitution.object.registeredObjectClassHandle == 0U ||
        reconstitution.object.producingFederateId != message.producingFederateId ||
        reconstitution.object.deleteAccepted) {
      throw std::runtime_error(
          "TSO object-deletion invocation snapshot does not match its message.");
    }
    std::vector<FederationStateImageObject> objects{reconstitution.object};
    validateObjectVector(objects);

    std::uint64_t previousKnownFederate = 0U;
    for (auto const& [federateId, objectClassHandle] :
         reconstitution.knownObjectClassHandlesByFederate) {
      if (federateId == 0U || federateId <= previousKnownFederate ||
          objectClassHandle == 0U) {
        throw std::runtime_error(
            "TSO object-deletion invocation known-class records are not strictly ordered in Umbra state image.");
      }
      previousKnownFederate = federateId;
    }
  }
}

void validateTsoRequestRetractionRecordVector(
    std::vector<FederationStateImageTsoRequestRetractionRecord> const& records) {
  std::uint64_t previousMessageId = 0U;
  for (auto const& record : records) {
    if (record.messageId == 0U || record.messageId <= previousMessageId ||
        record.producingFederateId == 0U ||
        (record.retractionApplied && !record.terminal) ||
        (!record.terminal && !record.timestampEncoding.has_value())) {
      throw std::runtime_error(
          "Invalid TSO Request Retraction record in Umbra state image.");
    }
    previousMessageId = record.messageId;

    std::uint64_t previousRecipient = 0U;
    for (auto const& recipient : record.recipientStates) {
      if (recipient.receivingFederateId == 0U ||
          recipient.receivingFederateId <= previousRecipient ||
          recipient.state > 3U) {
        throw std::runtime_error(
            "TSO Request Retraction recipients are not strictly ordered in Umbra state image.");
      }
      previousRecipient = recipient.receivingFederateId;
    }
  }
}

void validateTsoDirectedInteractionMessageVector(
    std::vector<FederationStateImageTsoDirectedInteractionMessage> const& messages) {
  std::uint64_t previousMessageId = 0U;
  for (auto const& message : messages) {
    if (message.messageId == 0U || message.messageId <= previousMessageId ||
        message.producingFederateId == 0U || message.objectInstanceHandle == 0U ||
        message.sentInteractionClassHandle == 0U) {
      throw std::runtime_error(
          "Invalid TSO directed interaction message identity in Umbra state image.");
    }
    previousMessageId = message.messageId;

    std::uint64_t previousHandle = 0U;
    for (auto const handle : message.sentParameterHandles) {
      if (handle == 0U || handle <= previousHandle) {
        throw std::runtime_error(
            "TSO directed interaction parameter handles are not strictly ordered in Umbra state image.");
      }
      previousHandle = handle;
    }

    previousHandle = 0U;
    for (auto const& parameter : message.parameters) {
      if (parameter.parameterHandle == 0U || parameter.parameterHandle <= previousHandle) {
        throw std::runtime_error(
            "TSO directed interaction parameters are not strictly ordered in Umbra state image.");
      }
      previousHandle = parameter.parameterHandle;
    }

    std::uint64_t previousRecipient = 0U;
    for (auto const& recipient : message.recipients) {
      if (recipient.receivingFederateId == 0U ||
          recipient.receivingFederateId <= previousRecipient ||
          recipient.objectInstanceHandle == 0U ||
          recipient.receivedInteractionClassHandle == 0U) {
        throw std::runtime_error(
            "TSO directed interaction recipients are not strictly ordered in Umbra state image.");
      }
      previousRecipient = recipient.receivingFederateId;
      previousHandle = 0U;
      for (auto const handle : recipient.receivedParameterHandles) {
        if (handle == 0U || handle <= previousHandle) {
          throw std::runtime_error(
              "TSO directed interaction recipient parameters are not strictly ordered in Umbra state image.");
        }
        previousHandle = handle;
      }
    }
  }
}

void validateTsoQueueEntryVector(
    std::vector<FederationStateImageTsoQueueEntry> const& entries) {
  std::uint64_t previousRecipient = 0U;
  std::uint32_t previousPhase = 0U;
  std::uint64_t previousSequence = 0U;
  std::uint64_t previousMessageId = 0U;
  bool havePrevious = false;
  for (auto const& entry : entries) {
    if (entry.messageId == 0U || entry.recipientFederateId == 0U ||
        entry.sequence == 0U || entry.phase > 2U ||
        !entry.timestampEncoding.has_value()) {
      throw std::runtime_error("Invalid TSO queue entry in Umbra state image.");
    }
    if (havePrevious) {
      auto const ordered =
          entry.recipientFederateId > previousRecipient ||
          (entry.recipientFederateId == previousRecipient &&
           (entry.phase > previousPhase ||
            (entry.phase == previousPhase &&
             (entry.sequence > previousSequence ||
              (entry.sequence == previousSequence &&
               entry.messageId > previousMessageId)))));
      if (!ordered) {
        throw std::runtime_error(
            "TSO queue entries are not strictly ordered in Umbra state image.");
      }
    }
    previousRecipient = entry.recipientFederateId;
    previousPhase = entry.phase;
    previousSequence = entry.sequence;
    previousMessageId = entry.messageId;
    havePrevious = true;
  }
}

std::vector<std::uint64_t> parseIdList(
    std::string_view value,
    char const* field) {
  if (value.empty()) {
    return {};
  }
  auto const fields = split(value, ',');
  std::vector<std::uint64_t> result;
  result.reserve(fields.size());
  for (auto const fieldValue : fields) {
    result.push_back(parseInteger<std::uint64_t>(fieldValue, field));
  }
  if (!std::is_sorted(result.begin(), result.end()) ||
      std::adjacent_find(result.begin(), result.end()) != result.end()) {
    throw std::runtime_error(std::string{"Unordered "} + field +
                             " in Umbra state image.");
  }
  return result;
}

template <typename T>
void appendScalar(std::string& result, char const* key, T value) {
  result += key;
  result += '=';
  result += std::to_string(value);
  result += '\n';
}

bool parseBoolean(std::string_view value, char const* field) {
  auto const parsed = parseInteger<std::uint32_t>(value, field);
  if (parsed > 1U) {
    throw std::runtime_error(std::string{"Malformed "} + field +
                             " in Umbra state image.");
  }
  return parsed != 0U;
}

}  // namespace

std::string FederationStateImageCodec::encode(FederationStateImage const& image) {
  auto members = image.members;
  std::sort(
      members.begin(),
      members.end(),
      [](auto const& first, auto const& second) { return first.id < second.id; });
  validateMemberVector(members);

  auto names = image.federateNamesById;
  std::sort(
      names.begin(),
      names.end(),
      [](auto const& first, auto const& second) { return first.first < second.first; });
  validateNameVector(names);

  auto reservedObjectInstanceNames = image.reservedObjectInstanceNames;
  std::sort(
      reservedObjectInstanceNames.begin(),
      reservedObjectInstanceNames.end(),
      [](auto const& first, auto const& second) {
        return std::pair{first.objectInstanceName, first.federateId} <
            std::pair{second.objectInstanceName, second.federateId};
      });
  validateObjectInstanceNameReservationVector(reservedObjectInstanceNames);

  auto synchronizationPoints = image.synchronizationPoints;
  std::sort(
      synchronizationPoints.begin(),
      synchronizationPoints.end(),
      [](auto const& first, auto const& second) {
        return first.label < second.label;
      });
  for (auto& point : synchronizationPoints) {
    std::sort(point.synchronizationSet.begin(), point.synchronizationSet.end());
    std::sort(point.announcedFederates.begin(), point.announcedFederates.end());
    std::sort(
        point.achievedFederates.begin(),
        point.achievedFederates.end(),
        [](auto const& first, auto const& second) {
          return first.first < second.first;
        });
  }
  validateSynchronizationPointVector(synchronizationPoints);
  if (image.synchronizationPointCount != synchronizationPoints.size()) {
    throw std::logic_error(
        "Synchronization-point count does not match the typed state-image section.");
  }

  auto regions = image.regions;
  std::sort(
      regions.begin(),
      regions.end(),
      [](auto const& first, auto const& second) {
        return first.handle < second.handle;
      });
  for (auto& region : regions) {
    std::sort(region.dimensionHandles.begin(), region.dimensionHandles.end());
    std::sort(
        region.pendingRangeBounds.begin(),
        region.pendingRangeBounds.end(),
        [](auto const& first, auto const& second) {
          return first.dimensionHandle < second.dimensionHandle;
        });
    std::sort(
        region.committedRangeBounds.begin(),
        region.committedRangeBounds.end(),
        [](auto const& first, auto const& second) {
          return first.dimensionHandle < second.dimensionHandle;
        });
  }
  validateRegionVector(regions);
  if (image.regionCount != regions.size()) {
    throw std::logic_error(
        "Region count does not match the typed state-image section.");
  }

  auto objectClassAttributeDeclarations = image.objectClassAttributeDeclarations;
  std::sort(
      objectClassAttributeDeclarations.begin(),
      objectClassAttributeDeclarations.end(),
      [](auto const& first, auto const& second) {
        return first.federateId < second.federateId;
      });
  for (auto& declaration : objectClassAttributeDeclarations) {
    std::sort(
        declaration.classes.begin(),
        declaration.classes.end(),
        [](auto const& first, auto const& second) {
          return first.objectClassHandle < second.objectClassHandle;
        });
    for (auto& objectClass : declaration.classes) {
      std::sort(
          objectClass.explicitlyPublishedAttributeHandles.begin(),
          objectClass.explicitlyPublishedAttributeHandles.end());
      std::sort(
          objectClass.subscribedAttributes.begin(),
          objectClass.subscribedAttributes.end(),
          [](auto const& first, auto const& second) {
            return first.attributeHandle < second.attributeHandle;
          });
      std::sort(
          objectClass.subscribedUpdateRateDesignators.begin(),
          objectClass.subscribedUpdateRateDesignators.end(),
          [](auto const& first, auto const& second) {
            return first.attributeHandle < second.attributeHandle;
          });
      std::sort(
          objectClass.regionalSubscribedAttributes.begin(),
          objectClass.regionalSubscribedAttributes.end(),
          [](auto const& first, auto const& second) {
            return std::pair{first.attributeHandle, first.regionHandle} <
                std::pair{second.attributeHandle, second.regionHandle};
          });
      std::sort(
          objectClass.regionalSubscribedUpdateRateDesignators.begin(),
          objectClass.regionalSubscribedUpdateRateDesignators.end(),
          [](auto const& first, auto const& second) {
            return std::pair{first.attributeHandle, first.regionHandle} <
                std::pair{second.attributeHandle, second.regionHandle};
          });
      std::sort(
          objectClass.defaultTransportationTypes.begin(),
          objectClass.defaultTransportationTypes.end(),
          [](auto const& first, auto const& second) {
            return first.attributeHandle < second.attributeHandle;
          });
      std::sort(
          objectClass.defaultOrderTypes.begin(),
          objectClass.defaultOrderTypes.end(),
          [](auto const& first, auto const& second) {
            return first.attributeHandle < second.attributeHandle;
          });
    }
  }
  validateObjectClassAttributeDeclarationVector(objectClassAttributeDeclarations);
  if (image.objectClassDeclarationCount != objectClassAttributeDeclarations.size()) {
    throw std::logic_error(
        "Object-class declaration count does not match the typed state-image section.");
  }

  auto timeStates = image.timeStates;
  std::sort(
      timeStates.begin(),
      timeStates.end(),
      [](auto const& first, auto const& second) {
        return first.federateId < second.federateId;
      });
  validateTimeVector(timeStates);

  auto objects = image.objects;
  std::sort(
      objects.begin(),
      objects.end(),
      [](auto const& first, auto const& second) {
        return first.handle < second.handle;
      });
  for (auto& object : objects) {
    std::sort(
        object.attributes.begin(),
        object.attributes.end(),
        [](auto const& first, auto const& second) {
          return first.handle < second.handle;
        });
    std::sort(
        object.pendingAttributeValueUpdateRequests.begin(),
        object.pendingAttributeValueUpdateRequests.end(),
        [](auto const& first, auto const& second) {
          return first.requestId < second.requestId;
        });
    for (auto& request : object.pendingAttributeValueUpdateRequests) {
      std::sort(
          request.requestedAttributeHandles.begin(),
          request.requestedAttributeHandles.end());
    }
    std::sort(
        object.pendingAttributeValueUpdateClassRequests.begin(),
        object.pendingAttributeValueUpdateClassRequests.end(),
        [](auto const& first, auto const& second) {
          return first.requestId < second.requestId;
        });
    for (auto& request : object.pendingAttributeValueUpdateClassRequests) {
      std::sort(
          request.requestedAttributeHandles.begin(),
          request.requestedAttributeHandles.end());
    }
    std::sort(
        object.pendingAttributeValueUpdateRegionalRequests.begin(),
        object.pendingAttributeValueUpdateRegionalRequests.end(),
        [](auto const& first, auto const& second) {
          return first.requestId < second.requestId;
        });
    for (auto& request : object.pendingAttributeValueUpdateRegionalRequests) {
      std::sort(
          request.requestedAttributeHandles.begin(),
          request.requestedAttributeHandles.end());
      std::sort(
          request.requestRegionsByAttribute.begin(),
          request.requestRegionsByAttribute.end(),
          [](auto const& first, auto const& second) {
            return first.first < second.first;
          });
      for (auto& [attributeHandle, regionHandles] :
           request.requestRegionsByAttribute) {
        static_cast<void>(attributeHandle);
        std::sort(regionHandles.begin(), regionHandles.end());
      }
    }
    std::sort(
        object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.begin(),
        object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.end(),
        [](auto const& first, auto const& second) {
          return first.requestId < second.requestId;
        });
    for (auto& request :
         object.pendingAttributeOwnershipAcquisitionIfAvailableRequests) {
      std::sort(
          request.desiredAttributeHandles.begin(),
          request.desiredAttributeHandles.end());
    }
    std::sort(
        object.pendingAttributeOwnershipAcquisitionRequests.begin(),
        object.pendingAttributeOwnershipAcquisitionRequests.end(),
        [](auto const& first, auto const& second) {
          return first.requestId < second.requestId;
        });
    for (auto& request : object.pendingAttributeOwnershipAcquisitionRequests) {
      std::sort(
          request.desiredAttributeHandles.begin(),
          request.desiredAttributeHandles.end());
      std::sort(
          request.notificationQueuedAttributeHandles.begin(),
          request.notificationQueuedAttributeHandles.end());
      std::sort(
          request.unavailableQueuedAttributeHandles.begin(),
          request.unavailableQueuedAttributeHandles.end());
      std::sort(
          request.releaseCallbacksQueuedByOwningFederate.begin(),
          request.releaseCallbacksQueuedByOwningFederate.end(),
          [](auto const& first, auto const& second) {
            return first.first < second.first;
          });
      for (auto& [ownerFederateId, attributes] :
           request.releaseCallbacksQueuedByOwningFederate) {
        static_cast<void>(ownerFederateId);
        std::sort(attributes.begin(), attributes.end());
      }
    }
    std::sort(
        object.pendingAttributeOwnershipAcquisitionCancellations.begin(),
        object.pendingAttributeOwnershipAcquisitionCancellations.end(),
        [](auto const& first, auto const& second) {
          return first.cancellationId < second.cancellationId;
        });
    for (auto& cancellation :
         object.pendingAttributeOwnershipAcquisitionCancellations) {
      std::sort(
          cancellation.attributeHandles.begin(),
          cancellation.attributeHandles.end());
    }
    std::sort(
        object.pendingAttributeOwnershipDivestitureIfWantedNotifications.begin(),
        object.pendingAttributeOwnershipDivestitureIfWantedNotifications.end(),
        [](auto const& first, auto const& second) {
          return first.notificationId < second.notificationId;
        });
    for (auto& notification :
         object.pendingAttributeOwnershipDivestitureIfWantedNotifications) {
      std::sort(
          notification.attributeHandles.begin(),
          notification.attributeHandles.end());
    }
    std::sort(
        object.pendingConfirmDivestitureNotifications.begin(),
        object.pendingConfirmDivestitureNotifications.end(),
        [](auto const& first, auto const& second) {
          return first.notificationId < second.notificationId;
        });
    for (auto& notification : object.pendingConfirmDivestitureNotifications) {
      std::sort(
          notification.attributeHandles.begin(),
          notification.attributeHandles.end());
    }
    std::sort(
        object.pendingAttributeTransportationTypeChanges.begin(),
        object.pendingAttributeTransportationTypeChanges.end(),
        [](auto const& first, auto const& second) {
          return first.requestId < second.requestId;
        });
    for (auto& request : object.pendingAttributeTransportationTypeChanges) {
      std::sort(request.attributeHandles.begin(), request.attributeHandles.end());
    }
    std::sort(
        object.pendingNegotiatedAttributeOwnershipDivestitures.begin(),
        object.pendingNegotiatedAttributeOwnershipDivestitures.end(),
        [](auto const& first, auto const& second) {
          return first.attributeHandle < second.attributeHandle;
        });
    std::sort(
        object.ownershipAssumptionRecipientsByAttribute.begin(),
        object.ownershipAssumptionRecipientsByAttribute.end(),
        [](auto const& first, auto const& second) {
          return first.attributeHandle < second.attributeHandle;
        });
    for (auto& assumption : object.ownershipAssumptionRecipientsByAttribute) {
      std::sort(
          assumption.recipientFederateIds.begin(),
          assumption.recipientFederateIds.end());
    }
    std::sort(
        object.ownershipAssumptionUserSuppliedTagsByAttribute.begin(),
        object.ownershipAssumptionUserSuppliedTagsByAttribute.end(),
        [](auto const& first, auto const& second) {
          return first.attributeHandle < second.attributeHandle;
        });
    std::sort(
        object.knownObjectClassHandlesByFederate.begin(),
        object.knownObjectClassHandlesByFederate.end(),
        [](auto const& first, auto const& second) {
          return first.federateId < second.federateId;
        });
    std::sort(
        object.pendingDiscoveryFederateIds.begin(),
        object.pendingDiscoveryFederateIds.end());
    std::sort(
        object.pendingRemovalFederateIds.begin(),
        object.pendingRemovalFederateIds.end());
  }
  validateObjectVector(objects);

  auto pendingAttributeOwnershipQueries =
      image.pendingAttributeOwnershipQueries;
  std::sort(
      pendingAttributeOwnershipQueries.begin(),
      pendingAttributeOwnershipQueries.end(),
      [](auto const& first, auto const& second) {
        return first.requestId < second.requestId;
      });
  for (auto& query : pendingAttributeOwnershipQueries) {
    std::sort(
        query.requestedAttributeHandles.begin(),
        query.requestedAttributeHandles.end());
  }
  validatePendingAttributeOwnershipQueryVector(
      pendingAttributeOwnershipQueries);

  auto pendingAttributeOwnershipAssumptions =
      image.pendingAttributeOwnershipAssumptions;
  std::sort(
      pendingAttributeOwnershipAssumptions.begin(),
      pendingAttributeOwnershipAssumptions.end(),
      [](auto const& first, auto const& second) {
        return std::tuple{
                   first.objectInstanceHandle,
                   first.receivingFederateId,
                   first.attributeHandles,
                   first.userSuppliedTag} <
            std::tuple{
                second.objectInstanceHandle,
                second.receivingFederateId,
                second.attributeHandles,
                second.userSuppliedTag};
      });
  for (auto& callback : pendingAttributeOwnershipAssumptions) {
    std::sort(callback.attributeHandles.begin(), callback.attributeHandles.end());
  }
  validatePendingAttributeOwnershipAssumptionVector(
      pendingAttributeOwnershipAssumptions);

  auto interactionDeclarations = image.interactionDeclarations;
  std::sort(
      interactionDeclarations.begin(),
      interactionDeclarations.end(),
      [](auto const& first, auto const& second) {
        return first.federateId < second.federateId;
      });
  for (auto& declaration : interactionDeclarations) {
    std::sort(
        declaration.publishedInteractionClasses.begin(),
        declaration.publishedInteractionClasses.end());
    std::sort(
        declaration.subscribedInteractionClasses.begin(),
        declaration.subscribedInteractionClasses.end(),
        [](auto const& first, auto const& second) {
          return first.interactionClassHandle < second.interactionClassHandle;
        });
    std::sort(
        declaration.regionalSubscribedInteractionClasses.begin(),
        declaration.regionalSubscribedInteractionClasses.end(),
        [](auto const& first, auto const& second) {
          return std::pair{first.interactionClassHandle, first.regionHandle} <
              std::pair{second.interactionClassHandle, second.regionHandle};
        });
    std::sort(
        declaration.publishedObjectClassDirectedInteractions.begin(),
        declaration.publishedObjectClassDirectedInteractions.end(),
        [](auto const& first, auto const& second) {
          return std::pair{first.objectClassHandle, first.interactionClassHandle} <
              std::pair{second.objectClassHandle, second.interactionClassHandle};
        });
    std::sort(
        declaration.subscribedObjectClassDirectedInteractions.begin(),
        declaration.subscribedObjectClassDirectedInteractions.end(),
        [](auto const& first, auto const& second) {
          return std::pair{first.objectClassHandle, first.interactionClassHandle} <
              std::pair{second.objectClassHandle, second.interactionClassHandle};
        });
    std::sort(
        declaration.interactionTransportationTypes.begin(),
        declaration.interactionTransportationTypes.end(),
        [](auto const& first, auto const& second) {
          return first.interactionClassHandle < second.interactionClassHandle;
        });
    std::sort(
        declaration.interactionOrderTypes.begin(),
        declaration.interactionOrderTypes.end(),
        [](auto const& first, auto const& second) {
          return first.interactionClassHandle < second.interactionClassHandle;
        });
    std::sort(
        declaration.pendingInteractionTransportationTypeChanges.begin(),
        declaration.pendingInteractionTransportationTypeChanges.end(),
        [](auto const& first, auto const& second) {
          return first.interactionClassHandle < second.interactionClassHandle;
        });
  }
  validateInteractionDeclarationVector(interactionDeclarations);

  auto tsoInteractionMessages = image.tsoInteractionMessages;
  std::sort(
      tsoInteractionMessages.begin(),
      tsoInteractionMessages.end(),
      [](auto const& first, auto const& second) {
        return first.messageId < second.messageId;
      });
  for (auto& message : tsoInteractionMessages) {
    std::sort(
        message.sentParameterHandles.begin(),
        message.sentParameterHandles.end());
    std::sort(
        message.parameters.begin(),
        message.parameters.end(),
        [](auto const& first, auto const& second) {
          return first.parameterHandle < second.parameterHandle;
        });
    std::sort(message.sentRegionHandles.begin(), message.sentRegionHandles.end());
    std::sort(
        message.sentRegionSnapshots.begin(),
        message.sentRegionSnapshots.end(),
        [](auto const& first, auto const& second) {
          return first.regionHandle < second.regionHandle;
        });
    for (auto& snapshot : message.sentRegionSnapshots) {
      std::sort(snapshot.dimensionHandles.begin(), snapshot.dimensionHandles.end());
      std::sort(
          snapshot.committedRangeBounds.begin(),
          snapshot.committedRangeBounds.end(),
          [](auto const& first, auto const& second) {
            return first.dimensionHandle < second.dimensionHandle;
          });
    }
  }
  validateTsoInteractionMessageVector(tsoInteractionMessages);

  auto tsoAttributeUpdateMessages = image.tsoAttributeUpdateMessages;
  std::sort(
      tsoAttributeUpdateMessages.begin(),
      tsoAttributeUpdateMessages.end(),
      [](auto const& first, auto const& second) {
        return first.messageId < second.messageId;
      });
  for (auto& message : tsoAttributeUpdateMessages) {
    std::sort(
        message.attributes.begin(),
        message.attributes.end(),
        [](auto const& first, auto const& second) {
          return first.attributeHandle < second.attributeHandle;
        });
    std::sort(
        message.passelsByRecipient.begin(),
        message.passelsByRecipient.end(),
        [](auto const& first, auto const& second) {
          return first.receivingFederateId < second.receivingFederateId;
        });
    for (auto& recipient : message.passelsByRecipient) {
      for (auto& passel : recipient.passels) {
        std::sort(
            passel.sentAttributeHandles.begin(),
            passel.sentAttributeHandles.end());
        std::sort(
            passel.sentRegionHandles.begin(),
            passel.sentRegionHandles.end());
        std::sort(
            passel.sentRegionSnapshots.begin(),
            passel.sentRegionSnapshots.end(),
            [](auto const& first, auto const& second) {
              return first.regionHandle < second.regionHandle;
            });
        for (auto& snapshot : passel.sentRegionSnapshots) {
          std::sort(snapshot.dimensionHandles.begin(), snapshot.dimensionHandles.end());
          std::sort(
              snapshot.committedRangeBounds.begin(),
              snapshot.committedRangeBounds.end(),
              [](auto const& first, auto const& second) {
                return first.dimensionHandle < second.dimensionHandle;
              });
        }
      }
    }
    std::sort(
        message.sentRegionSnapshots.begin(),
        message.sentRegionSnapshots.end(),
        [](auto const& first, auto const& second) {
          return first.regionHandle < second.regionHandle;
        });
    for (auto& snapshot : message.sentRegionSnapshots) {
      std::sort(snapshot.dimensionHandles.begin(), snapshot.dimensionHandles.end());
      std::sort(
          snapshot.committedRangeBounds.begin(),
          snapshot.committedRangeBounds.end(),
          [](auto const& first, auto const& second) {
            return first.dimensionHandle < second.dimensionHandle;
          });
    }
  }
  validateTsoAttributeUpdateMessageVector(tsoAttributeUpdateMessages);

  auto tsoObjectDeletionMessages = image.tsoObjectDeletionMessages;
  std::sort(
      tsoObjectDeletionMessages.begin(),
      tsoObjectDeletionMessages.end(),
      [](auto const& first, auto const& second) {
        return first.messageId < second.messageId;
      });
  for (auto& message : tsoObjectDeletionMessages) {
    std::sort(
        message.recipients.begin(),
        message.recipients.end(),
        [](auto const& first, auto const& second) {
          return first.receivingFederateId < second.receivingFederateId;
        });
  }
  validateTsoObjectDeletionMessageVector(tsoObjectDeletionMessages);

  auto tsoRequestRetractionRecords = image.tsoRequestRetractionRecords;
  std::sort(
      tsoRequestRetractionRecords.begin(),
      tsoRequestRetractionRecords.end(),
      [](auto const& first, auto const& second) {
        return first.messageId < second.messageId;
      });
  for (auto& record : tsoRequestRetractionRecords) {
    std::sort(
        record.recipientStates.begin(),
        record.recipientStates.end(),
        [](auto const& first, auto const& second) {
          return first.receivingFederateId < second.receivingFederateId;
        });
  }
  validateTsoRequestRetractionRecordVector(tsoRequestRetractionRecords);

  auto tsoDirectedInteractionMessages = image.tsoDirectedInteractionMessages;
  std::sort(
      tsoDirectedInteractionMessages.begin(),
      tsoDirectedInteractionMessages.end(),
      [](auto const& first, auto const& second) {
        return first.messageId < second.messageId;
      });
  for (auto& message : tsoDirectedInteractionMessages) {
    std::sort(
        message.sentParameterHandles.begin(),
        message.sentParameterHandles.end());
    std::sort(
        message.parameters.begin(),
        message.parameters.end(),
        [](auto const& first, auto const& second) {
          return first.parameterHandle < second.parameterHandle;
        });
    std::sort(
        message.recipients.begin(),
        message.recipients.end(),
        [](auto const& first, auto const& second) {
          return first.receivingFederateId < second.receivingFederateId;
        });
    for (auto& recipient : message.recipients) {
      std::sort(
          recipient.receivedParameterHandles.begin(),
          recipient.receivedParameterHandles.end());
    }
  }
  validateTsoDirectedInteractionMessageVector(tsoDirectedInteractionMessages);

  auto tsoQueueEntries = image.tsoQueueEntries;
  std::sort(
      tsoQueueEntries.begin(),
      tsoQueueEntries.end(),
      [](auto const& first, auto const& second) {
        if (first.recipientFederateId != second.recipientFederateId) {
          return first.recipientFederateId < second.recipientFederateId;
        }
        if (first.phase != second.phase) {
          return first.phase < second.phase;
        }
        if (first.sequence != second.sequence) {
          return first.sequence < second.sequence;
        }
        return first.messageId < second.messageId;
      });
  validateTsoQueueEntryVector(tsoQueueEntries);

  std::string result;
  result.reserve(
      512U + members.size() * 80U + names.size() * 40U +
      timeStates.size() * 220U + objects.size() * 160U +
      interactionDeclarations.size() * 180U + tsoInteractionMessages.size() * 260U +
      tsoAttributeUpdateMessages.size() * 300U +
      tsoObjectDeletionMessages.size() * 180U +
      tsoRequestRetractionRecords.size() * 140U +
      tsoDirectedInteractionMessages.size() * 260U + tsoQueueEntries.size() * 80U +
      pendingAttributeOwnershipQueries.size() * 100U +
      pendingAttributeOwnershipAssumptions.size() * 120U +
      reservedObjectInstanceNames.size() * 80U +
      synchronizationPoints.size() * 180U +
      regions.size() * 180U +
      objectClassAttributeDeclarations.size() * 180U);
  result += FederationStateImage::format;
  result += '\n';
  result += "federationName=";
  result += encodeWide(image.federationName);
  result += '\n';
  result += "logicalTimeImplementation=";
  result += encodeWide(image.logicalTimeImplementationName);
  result += '\n';
  appendScalar(result, "normalizationSeed", image.normalizationSeed);
  appendScalar(result, "federationSwitches", image.federationSwitches);
  result += "sectionCounts=";
  result += std::to_string(image.interactionDeclarationCount);
  result += ',';
  result += std::to_string(image.synchronizationPointCount);
  result += ',';
  result += std::to_string(image.objectClassDeclarationCount);
  result += ',';
  result += std::to_string(image.regionCount);
  result += ',';
  result += std::to_string(image.objectInstanceCount);
  result += ',';
  result += std::to_string(image.tsoInteractionMessageCount);
  result += ',';
  result += std::to_string(image.tsoAttributeUpdateMessageCount);
  result += ',';
  result += std::to_string(image.tsoObjectDeletionMessageCount);
  result += ',';
  result += std::to_string(image.tsoDirectedInteractionMessageCount);
  result += '\n';
  result += "allocators=";
  result += std::to_string(image.nextRegionHandle);
  result += ',';
  result += std::to_string(image.nextSubscriptionGeneration);
  result += ',';
  result += std::to_string(image.nextObjectInstanceHandle);
  result += ',';
  result += std::to_string(image.nextAttributeOwnershipAcquisitionIfAvailableRequestId);
  result += ',';
  result += std::to_string(image.nextAttributeOwnershipAcquisitionRequestId);
  result += ',';
  result += std::to_string(image.nextAttributeOwnershipAcquisitionRequestSequence);
  result += ',';
  result += std::to_string(image.nextAttributeOwnershipAcquisitionCancellationId);
  result += ',';
  result += std::to_string(image.nextAttributeOwnershipDivestitureIfWantedNotificationId);
  result += ',';
  result += std::to_string(image.nextConfirmDivestitureNotificationId);
  result += ',';
  result += std::to_string(image.nextAttributeTransportationTypeChangeRequestId);
  result += ',';
  result += std::to_string(image.nextAttributeValueUpdateRequestId);
  result += ',';
  result += std::to_string(image.nextAttributeOwnershipQueryRequestId);
  result += ',';
  result += std::to_string(image.nextTimeAdvanceGrantDispatchIdentity);
  result += '\n';

  appendScalar(result, "members", members.size());
  for (auto const& member : members) {
    result += "member=";
    result += std::to_string(member.id);
    result += '|';
    result += encodeWide(member.name);
    result += '|';
    result += encodeWide(member.type);
    result += '|';
    result += std::to_string(member.switches);
    result += '|';
    result += std::to_string(member.automaticResignAction);
    result += '|';
    result += std::to_string(member.momReportPeriodSeconds);
    result += '|';
    result += std::to_string(member.nextMomServiceReportSerialNumber);
    result += '|';
    result += std::to_string(member.successfulUpdateAttributeValuesCount);
    result += '|';
    result += std::to_string(
        member.successfulUpdateCountsByClassAndTransportation.size());
    result += '|';
    result += std::to_string(member.successfullyUpdatedObjectInstanceHandles.size());
    result += '|';
    result += std::to_string(
        member.successfullyUpdatedObjectInstanceClassHandles.size());
    result += '|';
    result += std::to_string(member.successfulReflectionsReceivedCount);
    result += '|';
    result += std::to_string(member.successfulObjectInstanceRegistrationsCount);
    result += '|';
    result += std::to_string(member.successfulObjectInstanceDeletionsCount);
    result += '|';
    result += std::to_string(member.successfulObjectInstanceRemovalsCount);
    result += '|';
    result += std::to_string(member.successfulObjectInstanceDiscoveriesCount);
    result += '|';
    result += std::to_string(
        member.successfulReflectionCountsByClassAndTransportation.size());
    result += '|';
    result += std::to_string(member.successfullyReflectedObjectInstanceHandles.size());
    result += '|';
    result += std::to_string(
        member.successfullyReflectedObjectInstanceClassHandles.size());
    result += '|';
    result += std::to_string(member.successfulInteractionsSentCount);
    result += '|';
    result += std::to_string(member.successfulDirectedInteractionsSentCount);
    result += '|';
    result += std::to_string(
        member.successfulInteractionCountsByClassAndTransportation.size());
    result += '|';
    result += std::to_string(
        member.successfulDirectedInteractionCountsByClassAndTransportation.size());
    result += '|';
    result += std::to_string(member.successfulInteractionsReceivedCount);
    result += '|';
    result += std::to_string(member.successfulDirectedInteractionsReceivedCount);
    result += '|';
    result += std::to_string(
        member.successfulInteractionReceiptCountsByClassAndTransportation.size());
    result += '|';
    result += std::to_string(
        member.successfulDirectedInteractionReceiptCountsByClassAndTransportation.size());
    result += '\n';
    for (auto const& update : member.successfulUpdateCountsByClassAndTransportation) {
      result += "memberUpdateCount=";
      result += std::to_string(member.id);
      result += '|';
      result += std::to_string(update.objectClassHandle);
      result += '|';
      result += hexEncode(update.transportationName);
      result += '|';
      result += std::to_string(update.count);
      result += '\n';
    }
    for (auto const objectInstanceHandle :
         member.successfullyUpdatedObjectInstanceHandles) {
      result += "memberUpdatedObject=";
      result += std::to_string(member.id);
      result += '|';
      result += std::to_string(objectInstanceHandle);
      result += '\n';
    }
    for (auto const& updatedObject :
         member.successfullyUpdatedObjectInstanceClassHandles) {
      result += "memberUpdatedObjectClass=";
      result += std::to_string(member.id);
      result += '|';
      result += std::to_string(updatedObject.objectInstanceHandle);
      result += '|';
      result += std::to_string(updatedObject.objectClassHandle);
      result += '\n';
    }
    for (auto const& reflection :
         member.successfulReflectionCountsByClassAndTransportation) {
      result += "memberReflectionCount=";
      result += std::to_string(member.id);
      result += '|';
      result += std::to_string(reflection.objectClassHandle);
      result += '|';
      result += hexEncode(reflection.transportationName);
      result += '|';
      result += std::to_string(reflection.count);
      result += '\n';
    }
    for (auto const objectInstanceHandle :
         member.successfullyReflectedObjectInstanceHandles) {
      result += "memberReflectedObject=";
      result += std::to_string(member.id);
      result += '|';
      result += std::to_string(objectInstanceHandle);
      result += '\n';
    }
    for (auto const& reflectedObject :
         member.successfullyReflectedObjectInstanceClassHandles) {
      result += "memberReflectedObjectClass=";
      result += std::to_string(member.id);
      result += '|';
      result += std::to_string(reflectedObject.objectInstanceHandle);
      result += '|';
      result += std::to_string(reflectedObject.objectClassHandle);
      result += '\n';
    }
    for (auto const& interaction :
         member.successfulInteractionCountsByClassAndTransportation) {
      result += "memberInteractionSendCount=";
      result += std::to_string(member.id);
      result += '|';
      result += std::to_string(interaction.interactionClassHandle);
      result += '|';
      result += hexEncode(interaction.transportationName);
      result += '|';
      result += std::to_string(interaction.count);
      result += '\n';
    }
    for (auto const& interaction :
         member.successfulDirectedInteractionCountsByClassAndTransportation) {
      result += "memberDirectedInteractionSendCount=";
      result += std::to_string(member.id);
      result += '|';
      result += std::to_string(interaction.interactionClassHandle);
      result += '|';
      result += hexEncode(interaction.transportationName);
      result += '|';
      result += std::to_string(interaction.count);
      result += '\n';
    }
    for (auto const& interaction :
         member.successfulInteractionReceiptCountsByClassAndTransportation) {
      result += "memberInteractionReceiptCount=";
      result += std::to_string(member.id);
      result += '|';
      result += std::to_string(interaction.interactionClassHandle);
      result += '|';
      result += hexEncode(interaction.transportationName);
      result += '|';
      result += std::to_string(interaction.count);
      result += '\n';
    }
    for (auto const& interaction :
         member.successfulDirectedInteractionReceiptCountsByClassAndTransportation) {
      result += "memberDirectedInteractionReceiptCount=";
      result += std::to_string(member.id);
      result += '|';
      result += std::to_string(interaction.interactionClassHandle);
      result += '|';
      result += hexEncode(interaction.transportationName);
      result += '|';
      result += std::to_string(interaction.count);
      result += '\n';
    }
  }

  appendScalar(result, "federateNames", names.size());
  for (auto const& [id, name] : names) {
    result += "name=";
    result += std::to_string(id);
    result += '|';
    result += encodeWide(name);
    result += '\n';
  }

  appendScalar(result, "timeStates", timeStates.size());
  for (auto const& state : timeStates) {
    result += "time=";
    result += std::to_string(state.federateId);
    result += '|';
    result += encodeWide(state.implementationName);
    result += '|';
    result += std::to_string(state.flags);
    result += '|';
    result += std::to_string(state.pendingGeneration);
    result += '|';
    result += std::to_string(state.advanceMode);
    result += '|';
    result += encodeOptional(state.currentTimeEncoding);
    result += '|';
    result += encodeOptional(state.optimisticTimeEncoding);
    result += '|';
    result += encodeOptional(state.requestedTimeEncoding);
    result += '|';
    result += encodeOptional(state.advanceRequestTimeEncoding);
    result += '|';
    result += encodeOptional(state.lookaheadEncoding);
    result += '|';
    result += encodeOptional(state.requestedLookaheadEncoding);
    result += '|';
    result += std::to_string(state.queuedTsoCount);
    result += '|';
    result += std::to_string(state.inTransitTsoCount);
    result += '|';
    result += std::to_string(state.deliveredTsoCount);
    result += '|';
    result += std::to_string(state.pendingTimeRegulationGeneration);
    result += '|';
    result += std::to_string(state.pendingTimeConstrainedGeneration);
    result += '|';
    result += std::to_string(state.nextGeneration);
    result += '|';
    result += encodeOptional(state.pendingModifiedLookaheadEncoding);
    result += '\n';
  }
  appendScalar(result, "objects", objects.size());
  for (auto const& object : objects) {
    result += "object=";
    result += std::to_string(object.handle);
    result += '|';
    result += encodeWide(object.name);
    result += '|';
    result += std::to_string(object.registeredObjectClassHandle);
    result += '|';
    result += std::to_string(object.producingFederateId);
    result += '|';
    result += (object.deleteAccepted ? "1" : "0");
    result += '|';
    result += std::to_string(object.pendingOperationCount);
    result += '|';
    result += std::to_string(object.attributes.size());
    result += '|';
    result += std::to_string(
        object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.size());
    result += '|';
    result += std::to_string(object.pendingAttributeOwnershipAcquisitionRequests.size());
    result += '|';
    result += std::to_string(
        object.pendingAttributeOwnershipAcquisitionCancellations.size());
    result += '|';
    result += std::to_string(
        object.pendingAttributeOwnershipDivestitureIfWantedNotifications.size());
    result += '|';
    result += std::to_string(object.pendingConfirmDivestitureNotifications.size());
    result += '|';
    result += std::to_string(object.pendingAttributeTransportationTypeChanges.size());
    result += '|';
    result += std::to_string(object.pendingNegotiatedAttributeOwnershipDivestitures.size());
    result += '|';
    result += std::to_string(object.ownershipAssumptionRecipientsByAttribute.size());
    result += '|';
    result += std::to_string(object.ownershipAssumptionUserSuppliedTagsByAttribute.size());
    result += '|';
    result += std::to_string(object.knownObjectClassHandlesByFederate.size());
    result += '|';
    result += std::to_string(object.pendingDiscoveryFederateIds.size());
    result += '|';
    result += std::to_string(object.pendingRemovalFederateIds.size());
    result += '|';
    result += std::to_string(object.connectionLossAutomaticRemovalFederateIds.size());
    result += '|';
    result += std::to_string(object.deferredConnectionLossTsoRemovalFederateIds.size());
    result += '|';
    result += std::to_string(object.pendingTimestampedRemovalFederateIds.size());
    result += '|';
    if (object.pendingTimestampedDeletionMessageId.has_value()) {
      result += std::to_string(*object.pendingTimestampedDeletionMessageId);
    } else {
      result += '-';
    }
    if (object.attributeValuesPresent || !object.attributeValues.empty() ||
        object.pendingAttributeValueUpdateRequestsPresent ||
        !object.pendingAttributeValueUpdateRequests.empty() ||
        object.pendingAttributeValueUpdateClassRequestsPresent ||
        !object.pendingAttributeValueUpdateClassRequests.empty() ||
        object.pendingAttributeValueUpdateRegionalRequestsPresent ||
        !object.pendingAttributeValueUpdateRegionalRequests.empty()) {
      result += '|';
      result += std::to_string(object.attributeValues.size());
    }
    if (object.pendingAttributeValueUpdateRequestsPresent ||
        !object.pendingAttributeValueUpdateRequests.empty() ||
        object.pendingAttributeValueUpdateClassRequestsPresent ||
        !object.pendingAttributeValueUpdateClassRequests.empty() ||
        object.pendingAttributeValueUpdateRegionalRequestsPresent ||
        !object.pendingAttributeValueUpdateRegionalRequests.empty()) {
      result += '|';
      result += std::to_string(object.pendingAttributeValueUpdateRequests.size());
    }
    if (object.pendingAttributeValueUpdateClassRequestsPresent ||
        !object.pendingAttributeValueUpdateClassRequests.empty() ||
        object.pendingAttributeValueUpdateRegionalRequestsPresent ||
        !object.pendingAttributeValueUpdateRegionalRequests.empty()) {
      result += '|';
      result += std::to_string(object.pendingAttributeValueUpdateClassRequests.size());
    }
    if (object.pendingAttributeValueUpdateRegionalRequestsPresent ||
        !object.pendingAttributeValueUpdateRegionalRequests.empty()) {
      result += '|';
      result += std::to_string(object.pendingAttributeValueUpdateRegionalRequests.size());
    }
    result += '\n';
    for (auto const& attribute : object.attributes) {
      result += "attribute=";
      result += std::to_string(attribute.handle);
      result += '|';
      result += std::to_string(attribute.ownerFederateId);
      result += '|';
      result += hexEncode(attribute.transportationName);
      result += '|';
      result += std::to_string(attribute.orderType);
      result += '|';
      for (std::size_t index = 0U; index < attribute.updateRegionHandles.size(); ++index) {
        if (index != 0U) {
          result += ',';
        }
        result += std::to_string(attribute.updateRegionHandles[index]);
      }
      result += '\n';
    }
    for (auto const& value : object.attributeValues) {
      result += "objectAttributeValue=";
      result += std::to_string(value.attributeHandle);
      result += '|';
      result += hexEncode(value.value);
      result += '\n';
    }
    for (auto const& request : object.pendingAttributeValueUpdateRequests) {
      result += "pendingAttributeValueUpdate=";
      result += std::to_string(request.requestId);
      result += '|';
      result += std::to_string(request.requestingFederateId);
      result += '|';
      result += std::to_string(request.providingFederateId);
      result += '|';
      result += hexEncode(request.userSuppliedTag);
      result += '|';
      for (std::size_t index = 0U;
           index < request.requestedAttributeHandles.size();
           ++index) {
        if (index != 0U) {
          result += ',';
        }
        result += std::to_string(request.requestedAttributeHandles[index]);
      }
      result += '\n';
    }
    for (auto const& request : object.pendingAttributeValueUpdateClassRequests) {
      result += "pendingAttributeValueUpdateClass=";
      result += std::to_string(request.requestId);
      result += '|';
      result += std::to_string(request.requestingFederateId);
      result += '|';
      result += std::to_string(request.providingFederateId);
      result += '|';
      result += std::to_string(request.requestedObjectClassHandle);
      result += '|';
      result += hexEncode(request.userSuppliedTag);
      result += '|';
      for (std::size_t index = 0U;
           index < request.requestedAttributeHandles.size();
           ++index) {
        if (index != 0U) {
          result += ',';
        }
        result += std::to_string(request.requestedAttributeHandles[index]);
      }
      result += '\n';
    }
    for (auto const& request : object.pendingAttributeValueUpdateRegionalRequests) {
      result += "pendingAttributeValueUpdateRegional=";
      result += std::to_string(request.requestId);
      result += '|';
      result += std::to_string(request.requestingFederateId);
      result += '|';
      result += std::to_string(request.providingFederateId);
      result += '|';
      result += std::to_string(request.requestedObjectClassHandle);
      result += '|';
      result += hexEncode(request.userSuppliedTag);
      result += '|';
      for (std::size_t index = 0U;
           index < request.requestedAttributeHandles.size();
           ++index) {
        if (index != 0U) {
          result += ',';
        }
        result += std::to_string(request.requestedAttributeHandles[index]);
      }
      result += '|';
      for (std::size_t index = 0U;
           index < request.requestRegionsByAttribute.size();
           ++index) {
        if (index != 0U) {
          result += ';';
        }
        auto const& [attributeHandle, regionHandles] =
            request.requestRegionsByAttribute[index];
        result += std::to_string(attributeHandle);
        result += ':';
        for (std::size_t regionIndex = 0U;
             regionIndex < regionHandles.size();
             ++regionIndex) {
          if (regionIndex != 0U) {
            result += ',';
          }
          result += std::to_string(regionHandles[regionIndex]);
        }
      }
      result += '\n';
    }
    for (auto const& request :
         object.pendingAttributeOwnershipAcquisitionIfAvailableRequests) {
      result += "pendingOwnershipIfAvailable=";
      result += std::to_string(request.requestId);
      result += '|';
      result += std::to_string(request.requestingFederateId);
      result += '|';
      result += std::to_string(request.requestSequence);
      result += '|';
      result += hexEncode(request.userSuppliedTag);
      result += '|';
      for (std::size_t index = 0U;
           index < request.desiredAttributeHandles.size();
           ++index) {
        if (index != 0U) {
          result += ',';
        }
        result += std::to_string(request.desiredAttributeHandles[index]);
      }
      result += '\n';
    }
    for (auto const& request : object.pendingAttributeOwnershipAcquisitionRequests) {
      result += "pendingOwnership=";
      result += std::to_string(request.requestId);
      result += '|';
      result += std::to_string(request.requestingFederateId);
      result += '|';
      result += std::to_string(request.requestSequence);
      result += '|';
      result += hexEncode(request.userSuppliedTag);
      auto appendAttributeList = [&result](std::vector<std::uint64_t> const& handles) {
        result += '|';
        for (std::size_t index = 0U; index < handles.size(); ++index) {
          if (index != 0U) {
            result += ',';
          }
          result += std::to_string(handles[index]);
        }
      };
      appendAttributeList(request.desiredAttributeHandles);
      appendAttributeList(request.notificationQueuedAttributeHandles);
      appendAttributeList(request.unavailableQueuedAttributeHandles);
      result += '|';
      result += std::to_string(
          request.releaseCallbacksQueuedByOwningFederate.size());
      result += '\n';
      for (auto const& [ownerFederateId, attributes] :
           request.releaseCallbacksQueuedByOwningFederate) {
        result += "pendingOwnershipRelease=";
        result += std::to_string(ownerFederateId);
        result += '|';
        for (std::size_t index = 0U; index < attributes.size(); ++index) {
          if (index != 0U) {
            result += ',';
          }
          result += std::to_string(attributes[index]);
        }
        result += '\n';
      }
    }
    for (auto const& cancellation :
         object.pendingAttributeOwnershipAcquisitionCancellations) {
      result += "pendingOwnershipCancellation=";
      result += std::to_string(cancellation.cancellationId);
      result += '|';
      result += std::to_string(cancellation.requestingFederateId);
      result += '|';
      for (std::size_t index = 0U;
           index < cancellation.attributeHandles.size();
           ++index) {
        if (index != 0U) {
          result += ',';
        }
        result += std::to_string(cancellation.attributeHandles[index]);
      }
      result += '\n';
    }
    for (auto const& notification :
         object.pendingAttributeOwnershipDivestitureIfWantedNotifications) {
      result += "pendingOwnershipDivestitureIfWanted=";
      result += std::to_string(notification.notificationId);
      result += '|';
      result += std::to_string(notification.receivingFederateId);
      result += '|';
      result += hexEncode(notification.userSuppliedTag);
      result += '|';
      for (std::size_t index = 0U;
           index < notification.attributeHandles.size();
           ++index) {
        if (index != 0U) {
          result += ',';
        }
        result += std::to_string(notification.attributeHandles[index]);
      }
      result += '\n';
    }
    for (auto const& notification : object.pendingConfirmDivestitureNotifications) {
      result += "pendingOwnershipConfirmDivestiture=";
      result += std::to_string(notification.notificationId);
      result += '|';
      result += std::to_string(notification.receivingFederateId);
      result += '|';
      result += hexEncode(notification.userSuppliedTag);
      result += '|';
      for (std::size_t index = 0U;
           index < notification.attributeHandles.size();
           ++index) {
        if (index != 0U) {
          result += ',';
        }
        result += std::to_string(notification.attributeHandles[index]);
      }
      result += '\n';
    }
    for (auto const& request : object.pendingAttributeTransportationTypeChanges) {
      result += "pendingAttributeTransportationTypeChange=";
      result += std::to_string(request.requestId);
      result += '|';
      result += std::to_string(request.requestingFederateId);
      result += '|';
      for (std::size_t index = 0U;
           index < request.attributeHandles.size();
           ++index) {
        if (index != 0U) {
          result += ',';
        }
        result += std::to_string(request.attributeHandles[index]);
      }
      result += '|';
      result += hexEncode(request.transportationName);
      result += '\n';
    }
    for (auto const& divestiture :
         object.pendingNegotiatedAttributeOwnershipDivestitures) {
      result += "pendingOwnershipNegotiatedDivestiture=";
      result += std::to_string(divestiture.attributeHandle);
      result += '|';
      result += std::to_string(divestiture.divestingFederateId);
      result += '|';
      result += std::to_string(divestiture.acquiringFederateId);
      result += '|';
      result += std::to_string(divestiture.acquisitionRequestId);
      result += '|';
      result += divestiture.acquiringFederateIsIfAvailable ? '1' : '0';
      result += '|';
      result += divestiture.confirmationQueued ? '1' : '0';
      result += '|';
      result += divestiture.confirmationDelivered ? '1' : '0';
      result += '|';
      result += hexEncode(divestiture.userSuppliedTag);
      result += '\n';
    }
    for (auto const& assumption : object.ownershipAssumptionRecipientsByAttribute) {
      result += "pendingOwnershipAssumptionRecipients=";
      result += std::to_string(assumption.attributeHandle);
      result += '|';
      for (std::size_t index = 0U;
           index < assumption.recipientFederateIds.size();
           ++index) {
        if (index != 0U) {
          result += ',';
        }
        result += std::to_string(assumption.recipientFederateIds[index]);
      }
      result += '\n';
    }
    for (auto const& assumption : object.ownershipAssumptionUserSuppliedTagsByAttribute) {
      result += "pendingOwnershipAssumptionTag=";
      result += std::to_string(assumption.attributeHandle);
      result += '|';
      result += hexEncode(assumption.userSuppliedTag);
      result += '\n';
    }
    for (auto const& known : object.knownObjectClassHandlesByFederate) {
      result += "objectKnownClass=";
      result += std::to_string(known.federateId);
      result += '|';
      result += std::to_string(known.objectClassHandle);
      result += '\n';
    }
    for (auto const federateId : object.pendingDiscoveryFederateIds) {
      result += "objectPendingDiscovery=";
      result += std::to_string(federateId);
      result += '\n';
    }
    for (auto const federateId : object.pendingRemovalFederateIds) {
      result += "objectPendingRemoval=";
      result += std::to_string(federateId);
      result += '\n';
    }
    for (auto const federateId : object.connectionLossAutomaticRemovalFederateIds) {
      result += "objectConnectionLossAutomaticRemoval=";
      result += std::to_string(federateId);
      result += '\n';
    }
    for (auto const federateId : object.deferredConnectionLossTsoRemovalFederateIds) {
      result += "objectDeferredConnectionLossTsoRemoval=";
      result += std::to_string(federateId);
      result += '\n';
    }
    for (auto const federateId : object.pendingTimestampedRemovalFederateIds) {
      result += "objectPendingTimestampedRemoval=";
      result += std::to_string(federateId);
      result += '\n';
    }
  }
  appendScalar(
      result,
      "pendingAttributeOwnershipQueries",
      pendingAttributeOwnershipQueries.size());
  for (auto const& query : pendingAttributeOwnershipQueries) {
    result += "pendingAttributeOwnershipQuery=";
    result += std::to_string(query.requestId);
    result += '|';
    result += std::to_string(query.requestingFederateId);
    result += '|';
    result += std::to_string(query.objectInstanceHandle);
    result += '|';
    result += std::to_string(query.reportKind);
    result += '|';
    result += std::to_string(query.owningFederateId);
    result += '|';
    for (std::size_t index = 0U;
         index < query.requestedAttributeHandles.size();
         ++index) {
      if (index != 0U) {
        result += ',';
      }
      result += std::to_string(query.requestedAttributeHandles[index]);
    }
    result += '\n';
  }
  appendScalar(
      result,
      "pendingAttributeOwnershipAssumptionCallbacks",
      pendingAttributeOwnershipAssumptions.size());
  for (auto const& callback : pendingAttributeOwnershipAssumptions) {
    result += "pendingAttributeOwnershipAssumptionCallback=";
    result += std::to_string(callback.objectInstanceHandle);
    result += '|';
    result += std::to_string(callback.receivingFederateId);
    result += '|';
    for (std::size_t index = 0U; index < callback.attributeHandles.size(); ++index) {
      if (index != 0U) {
        result += ',';
      }
      result += std::to_string(callback.attributeHandles[index]);
    }
    result += '|';
    result += hexEncode(callback.userSuppliedTag);
    result += '\n';
  }
  appendScalar(result, "interactionDeclarations", interactionDeclarations.size());
  for (auto const& declaration : interactionDeclarations) {
    result += "interactionDeclaration=";
    result += std::to_string(declaration.federateId);
    result += '|';
    result += std::to_string(declaration.publishedInteractionClasses.size());
    result += '|';
    result += std::to_string(declaration.subscribedInteractionClasses.size());
    result += '|';
    result += std::to_string(declaration.regionalSubscribedInteractionClasses.size());
    result += '|';
    result += std::to_string(declaration.publishedObjectClassDirectedInteractions.size());
    result += '|';
    result += std::to_string(declaration.subscribedObjectClassDirectedInteractions.size());
    result += '|';
    result += std::to_string(declaration.interactionTransportationTypes.size());
    result += '|';
    result += std::to_string(declaration.interactionOrderTypes.size());
    result += '|';
    result += std::to_string(
        declaration.pendingInteractionTransportationTypeChanges.size());
    result += '\n';
    for (auto const handle : declaration.publishedInteractionClasses) {
      result += "publishedInteraction=";
      result += std::to_string(declaration.federateId);
      result += '|';
      result += std::to_string(handle);
      result += '\n';
    }
    for (auto const& subscription : declaration.subscribedInteractionClasses) {
      result += "subscribedInteraction=";
      result += std::to_string(declaration.federateId);
      result += '|';
      result += std::to_string(subscription.interactionClassHandle);
      result += '|';
      result += (subscription.active ? "1" : "0");
      result += '\n';
    }
    for (auto const& subscription : declaration.regionalSubscribedInteractionClasses) {
      result += "regionalSubscribedInteraction=";
      result += std::to_string(declaration.federateId);
      result += '|';
      result += std::to_string(subscription.interactionClassHandle);
      result += '|';
      result += std::to_string(subscription.regionHandle);
      result += '|';
      result += (subscription.active ? "1" : "0");
      result += '\n';
    }
    for (auto const& directed : declaration.publishedObjectClassDirectedInteractions) {
      result += "publishedDirectedInteraction=";
      result += std::to_string(declaration.federateId);
      result += '|';
      result += std::to_string(directed.objectClassHandle);
      result += '|';
      result += std::to_string(directed.interactionClassHandle);
      result += '\n';
    }
    for (auto const& directed : declaration.subscribedObjectClassDirectedInteractions) {
      result += "subscribedDirectedInteraction=";
      result += std::to_string(declaration.federateId);
      result += '|';
      result += std::to_string(directed.objectClassHandle);
      result += '|';
      result += std::to_string(directed.interactionClassHandle);
      result += '|';
      result += (directed.active ? "1" : "0");
      result += '\n';
    }
    for (auto const& value : declaration.interactionTransportationTypes) {
      result += "interactionTransport=";
      result += std::to_string(declaration.federateId);
      result += '|';
      result += std::to_string(value.interactionClassHandle);
      result += '|';
      result += hexEncode(value.value);
      result += '\n';
    }
    for (auto const& value : declaration.interactionOrderTypes) {
      result += "interactionOrder=";
      result += std::to_string(declaration.federateId);
      result += '|';
      result += std::to_string(value.interactionClassHandle);
      result += '|';
      result += std::to_string(value.orderType);
      result += '\n';
    }
    for (auto const& value : declaration.pendingInteractionTransportationTypeChanges) {
      result += "interactionPendingTransport=";
      result += std::to_string(declaration.federateId);
      result += '|';
      result += std::to_string(value.interactionClassHandle);
      result += '|';
      result += hexEncode(value.value);
      result += '\n';
    }
  }
  appendScalar(result, "tsoInteractionMessages", tsoInteractionMessages.size());
  for (auto const& message : tsoInteractionMessages) {
    result += "tsoInteractionMessage=";
    result += std::to_string(message.messageId);
    result += '|';
    result += std::to_string(message.producingFederateId);
    result += '|';
    result += std::to_string(message.sentInteractionClassHandle);
    result += '|';
    result += (message.defaultRegionUsed ? "1" : "0");
    result += '|';
    result += std::to_string(message.sentOrderType);
    result += '|';
    result += std::to_string(message.receivedOrderType);
    result += '|';
    result += encodeOptional(message.timestampEncoding);
    result += '|';
    result += hexEncode(message.userSuppliedTag);
    result += '|';
    result += hexEncode(message.transportationName);
    result += '|';
    result += std::to_string(message.sentParameterHandles.size());
    result += '|';
    result += std::to_string(message.parameters.size());
    result += '|';
    result += std::to_string(message.sentRegionHandles.size());
    result += '|';
    result += std::to_string(message.sentRegionSnapshots.size());
    result += '\n';
    for (auto const handle : message.sentParameterHandles) {
      result += "tsoInteractionParameterHandle=";
      result += std::to_string(handle);
      result += '\n';
    }
    for (auto const& parameter : message.parameters) {
      result += "tsoInteractionParameter=";
      result += std::to_string(parameter.parameterHandle);
      result += '|';
      result += hexEncode(parameter.value);
      result += '\n';
    }
    for (auto const handle : message.sentRegionHandles) {
      result += "tsoInteractionRegionHandle=";
      result += std::to_string(handle);
      result += '\n';
    }
    for (auto const& snapshot : message.sentRegionSnapshots) {
      result += "tsoInteractionRegionSnapshot=";
      result += std::to_string(snapshot.regionHandle);
      result += '|';
      result += (snapshot.specificationCommitted ? "1" : "0");
      result += '|';
      result += std::to_string(snapshot.dimensionHandles.size());
      result += '|';
      result += std::to_string(snapshot.committedRangeBounds.size());
      result += '\n';
      for (auto const dimension : snapshot.dimensionHandles) {
        result += "tsoInteractionRegionDimension=";
        result += std::to_string(dimension);
        result += '\n';
      }
      for (auto const& range : snapshot.committedRangeBounds) {
        result += "tsoInteractionRegionRange=";
        result += std::to_string(range.dimensionHandle);
        result += '|';
        result += std::to_string(range.lowerBound);
        result += '|';
        result += std::to_string(range.upperBound);
        result += '\n';
      }
    }
  }
  appendScalar(
      result,
      "tsoDirectedInteractionMessages",
      tsoDirectedInteractionMessages.size());
  for (auto const& message : tsoDirectedInteractionMessages) {
    result += "tsoDirectedInteractionMessage=";
    result += std::to_string(message.messageId);
    result += '|';
    result += std::to_string(message.producingFederateId);
    result += '|';
    result += std::to_string(message.objectInstanceHandle);
    result += '|';
    result += std::to_string(message.sentInteractionClassHandle);
    result += '|';
    result += std::to_string(message.sentOrderType);
    result += '|';
    result += std::to_string(message.receivedOrderType);
    result += '|';
    result += encodeOptional(message.timestampEncoding);
    result += '|';
    result += hexEncode(message.userSuppliedTag);
    result += '|';
    result += hexEncode(message.transportationName);
    result += '|';
    result += std::to_string(message.sentParameterHandles.size());
    result += '|';
    result += std::to_string(message.parameters.size());
    result += '|';
    result += std::to_string(message.recipients.size());
    result += '\n';
    for (auto const handle : message.sentParameterHandles) {
      result += "tsoDirectedInteractionParameterHandle=";
      result += std::to_string(handle);
      result += '\n';
    }
    for (auto const& parameter : message.parameters) {
      result += "tsoDirectedInteractionParameter=";
      result += std::to_string(parameter.parameterHandle);
      result += '|';
      result += hexEncode(parameter.value);
      result += '\n';
    }
    for (auto const& recipient : message.recipients) {
      result += "tsoDirectedInteractionRecipient=";
      result += std::to_string(recipient.receivingFederateId);
      result += '|';
      result += std::to_string(recipient.objectInstanceHandle);
      result += '|';
      result += std::to_string(recipient.receivedInteractionClassHandle);
      result += '|';
      result += std::to_string(recipient.receivedParameterHandles.size());
      result += '\n';
      for (auto const handle : recipient.receivedParameterHandles) {
        result += "tsoDirectedInteractionRecipientParameter=";
        result += std::to_string(handle);
        result += '\n';
      }
    }
  }
  auto appendRegionSnapshot = [&result](
                                  std::string_view marker,
                                  std::string_view dimensionMarker,
                                  std::string_view rangeMarker,
                                  FederationStateImageInteractionRegionSnapshot const& snapshot) {
    result += marker;
    result += '=';
    result += std::to_string(snapshot.regionHandle);
    result += '|';
    result += (snapshot.specificationCommitted ? "1" : "0");
    result += '|';
    result += std::to_string(snapshot.dimensionHandles.size());
    result += '|';
    result += std::to_string(snapshot.committedRangeBounds.size());
    result += '\n';
    for (auto const dimension : snapshot.dimensionHandles) {
      result += dimensionMarker;
      result += '=';
      result += std::to_string(dimension);
      result += '\n';
    }
    for (auto const& range : snapshot.committedRangeBounds) {
      result += rangeMarker;
      result += '=';
      result += std::to_string(range.dimensionHandle);
      result += '|';
      result += std::to_string(range.lowerBound);
      result += '|';
      result += std::to_string(range.upperBound);
      result += '\n';
    }
  };
  appendScalar(
      result,
      "tsoAttributeUpdateMessages",
      tsoAttributeUpdateMessages.size());
  for (auto const& message : tsoAttributeUpdateMessages) {
    result += "tsoAttributeUpdateMessage=";
    result += std::to_string(message.messageId);
    result += '|';
    result += std::to_string(message.producingFederateId);
    result += '|';
    result += std::to_string(message.objectInstanceHandle);
    result += '|';
    result += encodeOptional(message.timestampEncoding);
    result += '|';
    result += hexEncode(message.userSuppliedTag);
    result += '|';
    result += std::to_string(message.attributes.size());
    result += '|';
    result += std::to_string(message.passelsByRecipient.size());
    result += '|';
    result += std::to_string(message.sentRegionSnapshots.size());
    result += '\n';
    for (auto const& attribute : message.attributes) {
      result += "tsoAttributeUpdateAttribute=";
      result += std::to_string(attribute.attributeHandle);
      result += '|';
      result += hexEncode(attribute.value);
      result += '\n';
    }
    for (auto const& recipient : message.passelsByRecipient) {
      result += "tsoAttributeUpdateRecipient=";
      result += std::to_string(recipient.receivingFederateId);
      result += '|';
      result += std::to_string(recipient.passels.size());
      result += '\n';
      for (auto const& passel : recipient.passels) {
        result += "tsoAttributeUpdatePassel=";
        result += hexEncode(passel.transportationName);
        result += '|';
        result += (passel.defaultRegionUsed ? "1" : "0");
        result += '|';
        result += std::to_string(passel.preferredOrderType);
        result += '|';
        result += std::to_string(passel.sentAttributeHandles.size());
        result += '|';
        result += std::to_string(passel.sentRegionHandles.size());
        result += '|';
        result += std::to_string(passel.sentRegionSnapshots.size());
        result += '\n';
        for (auto const handle : passel.sentAttributeHandles) {
          result += "tsoAttributeUpdatePasselAttributeHandle=";
          result += std::to_string(handle);
          result += '\n';
        }
        for (auto const handle : passel.sentRegionHandles) {
          result += "tsoAttributeUpdatePasselRegionHandle=";
          result += std::to_string(handle);
          result += '\n';
        }
        for (auto const& snapshot : passel.sentRegionSnapshots) {
          appendRegionSnapshot(
              "tsoAttributeUpdatePasselRegionSnapshot",
              "tsoAttributeUpdatePasselRegionDimension",
              "tsoAttributeUpdatePasselRegionRange",
              snapshot);
        }
      }
    }
    for (auto const& snapshot : message.sentRegionSnapshots) {
      appendRegionSnapshot(
          "tsoAttributeUpdateRegionSnapshot",
          "tsoAttributeUpdateRegionDimension",
          "tsoAttributeUpdateRegionRange",
          snapshot);
    }
  }
  appendScalar(
      result,
      "tsoObjectDeletionMessages",
      tsoObjectDeletionMessages.size());
  for (auto const& message : tsoObjectDeletionMessages) {
    result += "tsoObjectDeletionMessage=";
    result += std::to_string(message.messageId);
    result += '|';
    result += std::to_string(message.producingFederateId);
    result += '|';
    result += std::to_string(message.objectInstanceHandle);
    result += '|';
    result += encodeOptional(message.timestampEncoding);
    result += '|';
    result += hexEncode(message.userSuppliedTag);
    result += '|';
    result += std::to_string(message.recipients.size());
    result += '|';
    result += message.reconstitution.has_value() ? "1" : "0";
    result += '|';
    result += message.reconstitution
        ? std::to_string(message.reconstitution->knownObjectClassHandlesByFederate.size())
        : "0";
    result += '|';
    result += message.reconstitution
        ? std::to_string(message.reconstitution->object.attributes.size())
        : "0";
    result += '|';
    result += message.reconstitution
        ? std::to_string(message.reconstitution->object.attributeValues.size())
        : "0";
    result += '\n';
    for (auto const& recipient : message.recipients) {
      result += "tsoObjectDeletionRecipient=";
      result += std::to_string(recipient.receivingFederateId);
      result += '|';
      result += std::to_string(recipient.objectInstanceHandle);
      result += '\n';
    }
    if (message.reconstitution) {
      auto const& object = message.reconstitution->object;
      result += "tsoObjectDeletionReconstitution=";
      result += std::to_string(object.handle);
      result += '|';
      result += encodeWide(object.name);
      result += '|';
      result += std::to_string(object.registeredObjectClassHandle);
      result += '|';
      result += std::to_string(object.producingFederateId);
      result += '|';
      result += object.deleteAccepted ? "1" : "0";
      result += '|';
      result += std::to_string(object.pendingOperationCount);
      result += '\n';
      for (auto const& attribute : object.attributes) {
        result += "tsoObjectDeletionReconstitutionAttribute=";
        result += std::to_string(attribute.handle);
        result += '|';
        result += std::to_string(attribute.ownerFederateId);
        result += '|';
        result += hexEncode(attribute.transportationName);
        result += '|';
        result += std::to_string(attribute.orderType);
        result += '|';
        for (std::size_t index = 0U;
             index < attribute.updateRegionHandles.size(); ++index) {
          if (index != 0U) {
            result += ',';
          }
          result += std::to_string(attribute.updateRegionHandles[index]);
        }
        result += '\n';
      }
      for (auto const& value : object.attributeValues) {
        result += "tsoObjectDeletionReconstitutionValue=";
        result += std::to_string(value.attributeHandle);
        result += '|';
        result += hexEncode(value.value);
        result += '\n';
      }
      for (auto const& [federateId, objectClassHandle] :
           message.reconstitution->knownObjectClassHandlesByFederate) {
        result += "tsoObjectDeletionReconstitutionKnown=";
        result += std::to_string(federateId);
        result += '|';
        result += std::to_string(objectClassHandle);
        result += '\n';
      }
    }
  }
  appendScalar(
      result,
      "tsoRequestRetractionRecords",
      tsoRequestRetractionRecords.size());
  for (auto const& record : tsoRequestRetractionRecords) {
    result += "tsoRequestRetractionRecord=";
    result += std::to_string(record.messageId);
    result += '|';
    result += std::to_string(record.producingFederateId);
    result += '|';
    result += encodeOptional(record.timestampEncoding);
    result += '|';
    result += record.retractionApplied ? "1" : "0";
    result += '|';
    result += record.terminal ? "1" : "0";
    result += '|';
    result += record.producerResigned ? "1" : "0";
    result += '|';
    result += record.deliveryRequiredAfterConnectionLoss ? "1" : "0";
    result += '|';
    result += std::to_string(record.recipientStates.size());
    result += '\n';
    for (auto const& recipient : record.recipientStates) {
      result += "tsoRequestRetractionRecipient=";
      result += std::to_string(recipient.receivingFederateId);
      result += '|';
      result += std::to_string(recipient.state);
      result += '\n';
    }
  }
  appendScalar(result, "tsoQueueEntries", tsoQueueEntries.size());
  for (auto const& entry : tsoQueueEntries) {
    result += "tsoQueue=";
    result += std::to_string(entry.recipientFederateId);
    result += '|';
    result += std::to_string(entry.messageId);
    result += '|';
    result += std::to_string(entry.sequence);
    result += '|';
    result += std::to_string(entry.phase);
    result += '|';
    result += encodeOptional(entry.timestampEncoding);
    result += '\n';
  }

  appendScalar(
      result,
      "reservedObjectInstanceNames",
      reservedObjectInstanceNames.size());
  for (auto const& reservation : reservedObjectInstanceNames) {
    result += "reservedObjectInstanceName=";
    result += std::to_string(reservation.federateId);
    result += '|';
    result += encodeWide(reservation.objectInstanceName);
    result += '\n';
  }
  appendScalar(result, "synchronizationPoints", synchronizationPoints.size());
  for (auto const& point : synchronizationPoints) {
    result += "synchronizationPoint=";
    result += encodeWide(point.label);
    result += '|';
    result += hexEncode(point.userSuppliedTag);
    result += '|';
    result += std::to_string(point.synchronizationSet.size());
    result += '|';
    result += std::to_string(point.announcedFederates.size());
    result += '|';
    result += std::to_string(point.achievedFederates.size());
    result += '\n';
    for (auto const federateId : point.synchronizationSet) {
      result += "synchronizationPointMember=";
      result += std::to_string(federateId);
      result += '\n';
    }
    for (auto const federateId : point.announcedFederates) {
      result += "synchronizationPointAnnounced=";
      result += std::to_string(federateId);
      result += '\n';
    }
    for (auto const& [federateId, succeeded] : point.achievedFederates) {
      result += "synchronizationPointAchieved=";
      result += std::to_string(federateId);
      result += '|';
      result += succeeded ? "1" : "0";
      result += '\n';
    }
  }
  appendScalar(result, "regions", regions.size());
  for (auto const& region : regions) {
    result += "region=";
    result += std::to_string(region.handle);
    result += '|';
    result += std::to_string(region.ownerFederateId);
    result += '|';
    result += region.specificationCommitted ? "1" : "0";
    result += '|';
    result += region.inUse ? "1" : "0";
    result += '|';
    result += std::to_string(region.dimensionHandles.size());
    result += '|';
    result += std::to_string(region.pendingRangeBounds.size());
    result += '|';
    result += std::to_string(region.committedRangeBounds.size());
    result += '\n';
    for (auto const dimensionHandle : region.dimensionHandles) {
      result += "regionDimension=";
      result += std::to_string(dimensionHandle);
      result += '\n';
    }
    for (auto const& range : region.pendingRangeBounds) {
      result += "regionPendingRange=";
      result += std::to_string(range.dimensionHandle);
      result += '|';
      result += std::to_string(range.lowerBound);
      result += '|';
      result += std::to_string(range.upperBound);
      result += '\n';
    }
    for (auto const& range : region.committedRangeBounds) {
      result += "regionCommittedRange=";
      result += std::to_string(range.dimensionHandle);
      result += '|';
      result += std::to_string(range.lowerBound);
      result += '|';
      result += std::to_string(range.upperBound);
      result += '\n';
    }
  }
  appendScalar(
      result,
      "objectClassAttributeDeclarations",
      objectClassAttributeDeclarations.size());
  for (auto const& declaration : objectClassAttributeDeclarations) {
    result += "objectClassAttributeDeclaration=";
    result += std::to_string(declaration.federateId);
    result += '|';
    result += std::to_string(declaration.subscriptionGeneration);
    result += '|';
    result += std::to_string(declaration.classes.size());
    result += '\n';
    for (auto const& objectClass : declaration.classes) {
      result += "objectClassAttributeDeclarationClass=";
      result += std::to_string(declaration.federateId);
      result += '|';
      result += std::to_string(objectClass.objectClassHandle);
      result += '|';
      result += objectClass.privilegeToDeleteExplicitlyUnpublished ? "1" : "0";
      result += '|';
      result += std::to_string(objectClass.explicitlyPublishedAttributeHandles.size());
      result += '|';
      result += std::to_string(objectClass.subscribedAttributes.size());
      result += '|';
      result += std::to_string(objectClass.subscribedUpdateRateDesignators.size());
      result += '|';
      result += std::to_string(objectClass.regionalSubscribedAttributes.size());
      result += '|';
      result += std::to_string(
          objectClass.regionalSubscribedUpdateRateDesignators.size());
      result += '|';
      result += std::to_string(objectClass.defaultTransportationTypes.size());
      result += '|';
      result += std::to_string(objectClass.defaultOrderTypes.size());
      result += '\n';
      for (auto const attributeHandle : objectClass.explicitlyPublishedAttributeHandles) {
        result += "objectClassAttributePublished=";
        result += std::to_string(declaration.federateId);
        result += '|';
        result += std::to_string(objectClass.objectClassHandle);
        result += '|';
        result += std::to_string(attributeHandle);
        result += '\n';
      }
      for (auto const& subscription : objectClass.subscribedAttributes) {
        result += "objectClassAttributeSubscribed=";
        result += std::to_string(declaration.federateId);
        result += '|';
        result += std::to_string(objectClass.objectClassHandle);
        result += '|';
        result += std::to_string(subscription.attributeHandle);
        result += '|';
        result += subscription.active ? "1" : "0";
        result += '\n';
      }
      for (auto const& value : objectClass.subscribedUpdateRateDesignators) {
        result += "objectClassAttributeSubscribedRate=";
        result += std::to_string(declaration.federateId);
        result += '|';
        result += std::to_string(objectClass.objectClassHandle);
        result += '|';
        result += std::to_string(value.attributeHandle);
        result += '|';
        result += hexEncode(value.value);
        result += '\n';
      }
      for (auto const& subscription : objectClass.regionalSubscribedAttributes) {
        result += "objectClassAttributeRegionalSubscribed=";
        result += std::to_string(declaration.federateId);
        result += '|';
        result += std::to_string(objectClass.objectClassHandle);
        result += '|';
        result += std::to_string(subscription.attributeHandle);
        result += '|';
        result += std::to_string(subscription.regionHandle);
        result += '|';
        result += subscription.active ? "1" : "0";
        result += '\n';
      }
      for (auto const& value : objectClass.regionalSubscribedUpdateRateDesignators) {
        result += "objectClassAttributeRegionalSubscribedRate=";
        result += std::to_string(declaration.federateId);
        result += '|';
        result += std::to_string(objectClass.objectClassHandle);
        result += '|';
        result += std::to_string(value.attributeHandle);
        result += '|';
        result += std::to_string(value.regionHandle);
        result += '|';
        result += hexEncode(value.value);
        result += '\n';
      }
      for (auto const& value : objectClass.defaultTransportationTypes) {
        result += "objectClassAttributeDefaultTransport=";
        result += std::to_string(declaration.federateId);
        result += '|';
        result += std::to_string(objectClass.objectClassHandle);
        result += '|';
        result += std::to_string(value.attributeHandle);
        result += '|';
        result += hexEncode(value.value);
        result += '\n';
      }
      for (auto const& value : objectClass.defaultOrderTypes) {
        result += "objectClassAttributeDefaultOrder=";
        result += std::to_string(declaration.federateId);
        result += '|';
        result += std::to_string(objectClass.objectClassHandle);
        result += '|';
        result += std::to_string(value.attributeHandle);
        result += '|';
        result += std::to_string(value.orderType);
        result += '\n';
      }
    }
  }
  result += "end\n";
  return result;
}

FederationStateImage FederationStateImageCodec::decode(std::string_view payload) {
  Cursor cursor(payload);
  cursor.expect(FederationStateImage::format);
  FederationStateImage image;
  image.federationName = decodeWide(cursor.valueFor("federationName"));
  image.logicalTimeImplementationName =
      decodeWide(cursor.valueFor("logicalTimeImplementation"));
  image.normalizationSeed =
      parseInteger<std::uint64_t>(cursor.valueFor("normalizationSeed"), "normalizationSeed");
  image.federationSwitches =
      parseInteger<std::uint32_t>(cursor.valueFor("federationSwitches"), "federationSwitches");

  auto const sectionFields = split(cursor.valueFor("sectionCounts"), ',');
  if (sectionFields.size() != 9U) {
    throw std::runtime_error("Malformed sectionCounts in Umbra state image.");
  }
  image.interactionDeclarationCount =
      parseInteger<std::uint64_t>(sectionFields[0], "interactionDeclarationCount");
  image.synchronizationPointCount =
      parseInteger<std::uint64_t>(sectionFields[1], "synchronizationPointCount");
  image.objectClassDeclarationCount =
      parseInteger<std::uint64_t>(sectionFields[2], "objectClassDeclarationCount");
  image.regionCount = parseInteger<std::uint64_t>(sectionFields[3], "regionCount");
  image.objectInstanceCount =
      parseInteger<std::uint64_t>(sectionFields[4], "objectInstanceCount");
  image.tsoInteractionMessageCount =
      parseInteger<std::uint64_t>(sectionFields[5], "tsoInteractionMessageCount");
  image.tsoAttributeUpdateMessageCount =
      parseInteger<std::uint64_t>(sectionFields[6], "tsoAttributeUpdateMessageCount");
  image.tsoObjectDeletionMessageCount =
      parseInteger<std::uint64_t>(sectionFields[7], "tsoObjectDeletionMessageCount");
  image.tsoDirectedInteractionMessageCount =
      parseInteger<std::uint64_t>(sectionFields[8], "tsoDirectedInteractionMessageCount");

  auto const allocatorFields = split(cursor.valueFor("allocators"), ',');
  if (allocatorFields.size() != 11U && allocatorFields.size() != 12U &&
      allocatorFields.size() != 13U) {
    throw std::runtime_error("Malformed allocators in Umbra state image.");
  }
  image.nextRegionHandle = parseInteger<std::uint64_t>(allocatorFields[0], "nextRegionHandle");
  image.nextSubscriptionGeneration =
      parseInteger<std::uint64_t>(allocatorFields[1], "nextSubscriptionGeneration");
  image.nextObjectInstanceHandle =
      parseInteger<std::uint64_t>(allocatorFields[2], "nextObjectInstanceHandle");
  image.nextAttributeOwnershipAcquisitionIfAvailableRequestId = parseInteger<std::uint64_t>(
      allocatorFields[3], "nextAttributeOwnershipAcquisitionIfAvailableRequestId");
  image.nextAttributeOwnershipAcquisitionRequestId =
      parseInteger<std::uint64_t>(allocatorFields[4], "nextAttributeOwnershipAcquisitionRequestId");
  image.nextAttributeOwnershipAcquisitionRequestSequence = parseInteger<std::uint64_t>(
      allocatorFields[5], "nextAttributeOwnershipAcquisitionRequestSequence");
  image.nextAttributeOwnershipAcquisitionCancellationId = parseInteger<std::uint64_t>(
      allocatorFields[6], "nextAttributeOwnershipAcquisitionCancellationId");
  image.nextAttributeOwnershipDivestitureIfWantedNotificationId = parseInteger<std::uint64_t>(
      allocatorFields[7], "nextAttributeOwnershipDivestitureIfWantedNotificationId");
  image.nextConfirmDivestitureNotificationId =
      parseInteger<std::uint64_t>(allocatorFields[8], "nextConfirmDivestitureNotificationId");
  image.nextAttributeTransportationTypeChangeRequestId = parseInteger<std::uint64_t>(
      allocatorFields[9], "nextAttributeTransportationTypeChangeRequestId");
  if (allocatorFields.size() == 13U) {
    image.nextAttributeValueUpdateRequestId = parseInteger<std::uint64_t>(
        allocatorFields[10], "nextAttributeValueUpdateRequestId");
    image.nextAttributeOwnershipQueryRequestId = parseInteger<std::uint64_t>(
        allocatorFields[11], "nextAttributeOwnershipQueryRequestId");
    image.nextTimeAdvanceGrantDispatchIdentity = parseInteger<std::uint64_t>(
        allocatorFields[12], "nextTimeAdvanceGrantDispatchIdentity");
  } else if (allocatorFields.size() == 12U) {
    image.nextAttributeValueUpdateRequestId = parseInteger<std::uint64_t>(
        allocatorFields[10], "nextAttributeValueUpdateRequestId");
    image.nextTimeAdvanceGrantDispatchIdentity = parseInteger<std::uint64_t>(
        allocatorFields[11], "nextTimeAdvanceGrantDispatchIdentity");
  } else {
    image.nextTimeAdvanceGrantDispatchIdentity = parseInteger<std::uint64_t>(
        allocatorFields[10], "nextTimeAdvanceGrantDispatchIdentity");
  }

  auto const memberCount =
      parseInteger<std::size_t>(cursor.valueFor("members"), "members");
  image.members.reserve(memberCount);
  for (std::size_t index = 0U; index < memberCount; ++index) {
    auto const fields = split(cursor.valueFor("member"));
    if (fields.size() != 7U && fields.size() != 11U && fields.size() != 12U &&
        fields.size() != 16U && fields.size() != 19U && fields.size() != 23U &&
        fields.size() != 27U) {
      throw std::runtime_error("Malformed member in Umbra state image.");
    }
    FederationStateImageMember member{
        parseInteger<std::uint64_t>(fields[0], "member id"),
        decodeWide(fields[1]),
        decodeWide(fields[2]),
        parseInteger<std::uint32_t>(fields[3], "member switches"),
        parseInteger<std::uint32_t>(fields[4], "member automaticResignAction"),
        parseInteger<std::int32_t>(fields[5], "member momReportPeriodSeconds"),
        parseInteger<std::uint32_t>(fields[6], "member nextMomServiceReportSerialNumber"),
    };
    auto const updateCount = fields.size() >= 11U
        ? parseInteger<std::size_t>(fields[8], "member update telemetry buckets")
        : 0U;
    auto const updatedObjectCount = fields.size() >= 11U
        ? parseInteger<std::size_t>(fields[9], "member updated object handles")
        : 0U;
    auto const updatedObjectClassCount = fields.size() >= 11U
        ? parseInteger<std::size_t>(
              fields[10], "member updated object-class projections")
        : 0U;
    if (fields.size() >= 12U) {
      member.successfulReflectionsReceivedCount = parseInteger<std::uint64_t>(
          fields[11], "member reflections received count");
      member.reflectionTelemetryPresent = true;
    }
    if (fields.size() == 16U || fields.size() == 19U || fields.size() == 23U ||
        fields.size() == 27U) {
      member.successfulObjectInstanceRegistrationsCount =
          parseInteger<std::uint64_t>(fields[12], "member registrations count");
      member.successfulObjectInstanceDeletionsCount =
          parseInteger<std::uint64_t>(fields[13], "member deletions count");
      member.successfulObjectInstanceRemovalsCount =
          parseInteger<std::uint64_t>(fields[14], "member removals count");
      member.successfulObjectInstanceDiscoveriesCount =
          parseInteger<std::uint64_t>(fields[15], "member discoveries count");
      member.objectLifecycleTelemetryPresent = true;
    }
    auto const reflectionCount = fields.size() == 19U || fields.size() == 23U ||
        fields.size() == 27U
        ? parseInteger<std::size_t>(fields[16], "member reflection telemetry buckets")
        : 0U;
    auto const reflectedObjectCount = fields.size() == 19U || fields.size() == 23U ||
        fields.size() == 27U
        ? parseInteger<std::size_t>(fields[17], "member reflected object handles")
        : 0U;
    auto const reflectedObjectClassCount = fields.size() == 19U || fields.size() == 23U ||
        fields.size() == 27U
        ? parseInteger<std::size_t>(
              fields[18], "member reflected object-class projections")
        : 0U;
    if (fields.size() == 11U || fields.size() == 12U || fields.size() == 16U ||
        fields.size() == 19U || fields.size() == 23U || fields.size() == 27U) {
      member.successfulUpdateAttributeValuesCount = parseInteger<std::uint64_t>(
          fields[7], "member successful update count");
      member.successfulUpdateCountsByClassAndTransportation.reserve(updateCount);
      for (std::size_t record = 0U; record < updateCount; ++record) {
        auto const recordFields = split(cursor.valueFor("memberUpdateCount"));
        if (recordFields.size() != 4U ||
            parseInteger<std::uint64_t>(
                recordFields[0], "member update telemetry federate") != member.id) {
          throw std::runtime_error(
              "Mismatched member update telemetry record in Umbra state image.");
        }
        member.successfulUpdateCountsByClassAndTransportation.push_back({
            parseInteger<std::uint64_t>(
                recordFields[1], "member update telemetry object class"),
            hexDecode(recordFields[2]),
            parseInteger<std::uint64_t>(recordFields[3], "member update telemetry count"),
        });
      }
      member.successfullyUpdatedObjectInstanceHandles.reserve(updatedObjectCount);
      for (std::size_t record = 0U; record < updatedObjectCount; ++record) {
        auto const recordFields = split(cursor.valueFor("memberUpdatedObject"));
        if (recordFields.size() != 2U ||
            parseInteger<std::uint64_t>(
                recordFields[0], "member updated-object federate") != member.id) {
          throw std::runtime_error(
              "Mismatched member updated-object record in Umbra state image.");
        }
        member.successfullyUpdatedObjectInstanceHandles.push_back(
            parseInteger<std::uint64_t>(
                recordFields[1], "member updated-object handle"));
      }
      member.successfullyUpdatedObjectInstanceClassHandles.reserve(
          updatedObjectClassCount);
      for (std::size_t record = 0U; record < updatedObjectClassCount; ++record) {
        auto const recordFields = split(cursor.valueFor("memberUpdatedObjectClass"));
        if (recordFields.size() != 3U ||
            parseInteger<std::uint64_t>(
                recordFields[0], "member updated-object-class federate") != member.id) {
          throw std::runtime_error(
              "Mismatched member updated-object-class record in Umbra state image.");
        }
        member.successfullyUpdatedObjectInstanceClassHandles.push_back({
            parseInteger<std::uint64_t>(
                recordFields[1], "member updated-object-class object handle"),
            parseInteger<std::uint64_t>(
                recordFields[2], "member updated-object-class class handle"),
        });
      }
    }
    if (fields.size() == 19U || fields.size() == 23U || fields.size() == 27U) {
      member.successfulReflectionCountsByClassAndTransportation.reserve(
          reflectionCount);
      for (std::size_t record = 0U; record < reflectionCount; ++record) {
        auto const recordFields = split(cursor.valueFor("memberReflectionCount"));
        if (recordFields.size() != 4U ||
            parseInteger<std::uint64_t>(
                recordFields[0], "member reflection telemetry federate") != member.id) {
          throw std::runtime_error(
              "Mismatched member reflection telemetry record in Umbra state image.");
        }
        member.successfulReflectionCountsByClassAndTransportation.push_back({
            parseInteger<std::uint64_t>(
                recordFields[1], "member reflection telemetry object class"),
            hexDecode(recordFields[2]),
            parseInteger<std::uint64_t>(
                recordFields[3], "member reflection telemetry count"),
        });
      }
      member.successfullyReflectedObjectInstanceHandles.reserve(
          reflectedObjectCount);
      for (std::size_t record = 0U; record < reflectedObjectCount; ++record) {
        auto const recordFields = split(cursor.valueFor("memberReflectedObject"));
        if (recordFields.size() != 2U ||
            parseInteger<std::uint64_t>(
                recordFields[0], "member reflected-object federate") != member.id) {
          throw std::runtime_error(
              "Mismatched member reflected-object record in Umbra state image.");
        }
        member.successfullyReflectedObjectInstanceHandles.push_back(
            parseInteger<std::uint64_t>(
                recordFields[1], "member reflected-object handle"));
      }
      member.successfullyReflectedObjectInstanceClassHandles.reserve(
          reflectedObjectClassCount);
      for (std::size_t record = 0U; record < reflectedObjectClassCount; ++record) {
        auto const recordFields = split(cursor.valueFor("memberReflectedObjectClass"));
        if (recordFields.size() != 3U ||
            parseInteger<std::uint64_t>(
                recordFields[0], "member reflected-object-class federate") != member.id) {
          throw std::runtime_error(
              "Mismatched member reflected-object-class record in Umbra state image.");
        }
        member.successfullyReflectedObjectInstanceClassHandles.push_back({
            parseInteger<std::uint64_t>(
                recordFields[1], "member reflected-object-class object handle"),
            parseInteger<std::uint64_t>(
                recordFields[2], "member reflected-object-class class handle"),
        });
      }
      member.reflectionProjectionTelemetryPresent = true;
    }
    if (fields.size() == 23U || fields.size() == 27U) {
      member.successfulInteractionsSentCount = parseInteger<std::uint64_t>(
          fields[19], "member interactions sent count");
      member.successfulDirectedInteractionsSentCount = parseInteger<std::uint64_t>(
          fields[20], "member directed interactions sent count");
      auto const interactionCount = parseInteger<std::size_t>(
          fields[21], "member interaction-send telemetry buckets");
      auto const directedInteractionCount = parseInteger<std::size_t>(
          fields[22], "member directed interaction-send telemetry buckets");
      member.successfulInteractionCountsByClassAndTransportation.reserve(
          interactionCount);
      for (std::size_t record = 0U; record < interactionCount; ++record) {
        auto const recordFields = split(cursor.valueFor("memberInteractionSendCount"));
        if (recordFields.size() != 4U ||
            parseInteger<std::uint64_t>(
                recordFields[0], "member interaction-send federate") != member.id) {
          throw std::runtime_error(
              "Mismatched member interaction-send telemetry record in Umbra state image.");
        }
        member.successfulInteractionCountsByClassAndTransportation.push_back({
            parseInteger<std::uint64_t>(
                recordFields[1], "member interaction-send class"),
            hexDecode(recordFields[2]),
            parseInteger<std::uint64_t>(
                recordFields[3], "member interaction-send count"),
        });
      }
      member.successfulDirectedInteractionCountsByClassAndTransportation.reserve(
          directedInteractionCount);
      for (std::size_t record = 0U; record < directedInteractionCount; ++record) {
        auto const recordFields = split(
            cursor.valueFor("memberDirectedInteractionSendCount"));
        if (recordFields.size() != 4U ||
            parseInteger<std::uint64_t>(
                recordFields[0], "member directed interaction-send federate") != member.id) {
          throw std::runtime_error(
              "Mismatched member directed interaction-send telemetry record in Umbra state image.");
        }
        member.successfulDirectedInteractionCountsByClassAndTransportation.push_back({
            parseInteger<std::uint64_t>(
                recordFields[1], "member directed interaction-send class"),
            hexDecode(recordFields[2]),
            parseInteger<std::uint64_t>(
                recordFields[3], "member directed interaction-send count"),
        });
      }
      member.interactionSendTelemetryPresent = true;
    }
    if (fields.size() == 27U) {
      member.successfulInteractionsReceivedCount = parseInteger<std::uint64_t>(
          fields[23], "member interactions received count");
      member.successfulDirectedInteractionsReceivedCount = parseInteger<std::uint64_t>(
          fields[24], "member directed interactions received count");
      auto const interactionReceiptCount = parseInteger<std::size_t>(
          fields[25], "member interaction-receipt telemetry buckets");
      auto const directedInteractionReceiptCount = parseInteger<std::size_t>(
          fields[26], "member directed interaction-receipt telemetry buckets");
      member.successfulInteractionReceiptCountsByClassAndTransportation.reserve(
          interactionReceiptCount);
      for (std::size_t record = 0U; record < interactionReceiptCount; ++record) {
        auto const recordFields = split(cursor.valueFor("memberInteractionReceiptCount"));
        if (recordFields.size() != 4U ||
            parseInteger<std::uint64_t>(
                recordFields[0], "member interaction-receipt federate") != member.id) {
          throw std::runtime_error(
              "Mismatched member interaction-receipt telemetry record in Umbra state image.");
        }
        member.successfulInteractionReceiptCountsByClassAndTransportation.push_back({
            parseInteger<std::uint64_t>(
                recordFields[1], "member interaction-receipt class"),
            hexDecode(recordFields[2]),
            parseInteger<std::uint64_t>(
                recordFields[3], "member interaction-receipt count"),
        });
      }
      member.successfulDirectedInteractionReceiptCountsByClassAndTransportation.reserve(
          directedInteractionReceiptCount);
      for (std::size_t record = 0U; record < directedInteractionReceiptCount; ++record) {
        auto const recordFields = split(
            cursor.valueFor("memberDirectedInteractionReceiptCount"));
        if (recordFields.size() != 4U ||
            parseInteger<std::uint64_t>(
                recordFields[0], "member directed interaction-receipt federate") != member.id) {
          throw std::runtime_error(
              "Mismatched member directed interaction-receipt telemetry record in Umbra state image.");
        }
        member.successfulDirectedInteractionReceiptCountsByClassAndTransportation.push_back({
            parseInteger<std::uint64_t>(
                recordFields[1], "member directed interaction-receipt class"),
            hexDecode(recordFields[2]),
            parseInteger<std::uint64_t>(
                recordFields[3], "member directed interaction-receipt count"),
        });
      }
      member.interactionReceiptTelemetryPresent = true;
    }
    image.members.push_back(std::move(member));
  }
  validateMemberVector(image.members);

  auto const nameCount =
      parseInteger<std::size_t>(cursor.valueFor("federateNames"), "federateNames");
  image.federateNamesById.reserve(nameCount);
  for (std::size_t index = 0U; index < nameCount; ++index) {
    auto const fields = split(cursor.valueFor("name"));
    if (fields.size() != 2U) {
      throw std::runtime_error("Malformed federate name in Umbra state image.");
    }
    image.federateNamesById.emplace_back(
        parseInteger<std::uint64_t>(fields[0], "federate name id"),
        decodeWide(fields[1]));
  }
  validateNameVector(image.federateNamesById);

  auto const timeCount =
      parseInteger<std::size_t>(cursor.valueFor("timeStates"), "timeStates");
  image.timeStates.reserve(timeCount);
  for (std::size_t index = 0U; index < timeCount; ++index) {
    auto const fields = split(cursor.valueFor("time"));
    if (fields.size() != 14U && fields.size() != 17U && fields.size() != 18U) {
      throw std::runtime_error("Malformed time state in Umbra state image.");
    }
    FederationStateImageTimeState state;
    state.federateId = parseInteger<std::uint64_t>(fields[0], "time federateId");
    state.implementationName = decodeWide(fields[1]);
    state.flags = parseInteger<std::uint32_t>(fields[2], "time flags");
    state.pendingGeneration =
        parseInteger<std::uint64_t>(fields[3], "time pendingGeneration");
    state.advanceMode =
        parseInteger<std::uint32_t>(fields[4], "time advanceMode");
    state.currentTimeEncoding = decodeOptional(fields[5]);
    state.optimisticTimeEncoding = decodeOptional(fields[6]);
    state.requestedTimeEncoding = decodeOptional(fields[7]);
    state.advanceRequestTimeEncoding = decodeOptional(fields[8]);
    state.lookaheadEncoding = decodeOptional(fields[9]);
    state.requestedLookaheadEncoding = decodeOptional(fields[10]);
    state.queuedTsoCount =
        parseInteger<std::uint64_t>(fields[11], "time queuedTsoCount");
    state.inTransitTsoCount =
        parseInteger<std::uint64_t>(fields[12], "time inTransitTsoCount");
    state.deliveredTsoCount =
        parseInteger<std::uint64_t>(fields[13], "time deliveredTsoCount");
    if (fields.size() == 17U) {
      state.pendingTimeRegulationGeneration = parseInteger<std::uint64_t>(
          fields[14], "time pending regulation generation");
      state.pendingTimeConstrainedGeneration = parseInteger<std::uint64_t>(
          fields[15], "time pending constrained generation");
      state.nextGeneration =
          parseInteger<std::uint64_t>(fields[16], "time next generation");
      state.applicationRequestLedgerPresent = true;
    } else if (fields.size() == 18U) {
      state.pendingTimeRegulationGeneration = parseInteger<std::uint64_t>(
          fields[14], "time pending regulation generation");
      state.pendingTimeConstrainedGeneration = parseInteger<std::uint64_t>(
          fields[15], "time pending constrained generation");
      state.nextGeneration =
          parseInteger<std::uint64_t>(fields[16], "time next generation");
      state.pendingModifiedLookaheadEncoding = decodeOptional(fields[17]);
      state.applicationRequestLedgerPresent = true;
    }
    image.timeStates.push_back(std::move(state));
  }
  validateTimeVector(image.timeStates);
  auto const objectMarker = cursor.line();
  if (objectMarker == "end") {
    // Accept the original v1 control/temporal payload, which predates the
    // optional application-object section. Newly encoded images always emit
    // the section explicitly.
    cursor.finish();
    return image;
  }
  constexpr std::string_view objectPrefix = "objects=";
  if (!objectMarker.starts_with(objectPrefix)) {
    throw std::runtime_error("Missing objects section in Umbra state image.");
  }
  auto const objectCount = parseInteger<std::size_t>(
      objectMarker.substr(objectPrefix.size()), "objects");
  image.objects.reserve(objectCount);
  for (std::size_t index = 0U; index < objectCount; ++index) {
    auto const fields = split(cursor.valueFor("object"));
    if (fields.size() < 7U ||
        (fields.size() > 19U && fields.size() != 23U && fields.size() != 24U &&
         fields.size() != 25U && fields.size() != 26U && fields.size() != 27U)) {
      throw std::runtime_error("Malformed object in Umbra state image.");
    }
    auto object = FederationStateImageObject{
        parseInteger<std::uint64_t>(fields[0], "object handle"),
        decodeWide(fields[1]),
        parseInteger<std::uint64_t>(fields[2], "object class handle"),
        parseInteger<std::uint64_t>(fields[3], "object producer"),
        parseInteger<std::uint32_t>(fields[4], "object deleteAccepted") != 0U,
        parseInteger<std::uint64_t>(fields[5], "object pendingOperationCount"),
        {},
    };
    auto const attributeCount =
        parseInteger<std::size_t>(fields[6], "object attributes");
    auto const pendingIfAvailableCount = fields.size() >= 8U
        ? parseInteger<std::size_t>(
              fields[7], "object pending If Available ownership reservations")
        : 0U;
    auto const pendingRegularCount = fields.size() >= 9U
        ? parseInteger<std::size_t>(
              fields[8], "object pending regular ownership reservations")
        : 0U;
    auto const pendingCancellationCount = fields.size() >= 10U
        ? parseInteger<std::size_t>(
              fields[9], "object pending ownership cancellations")
        : 0U;
    auto const pendingDivestitureIfWantedCount = fields.size() >= 11U
        ? parseInteger<std::size_t>(
              fields[10], "object pending Divestiture If Wanted notifications")
        : 0U;
    auto const pendingConfirmDivestitureCount = fields.size() >= 12U
        ? parseInteger<std::size_t>(
              fields[11], "object pending Confirm Divestiture notifications")
        : 0U;
    auto const pendingTransportationChangeCount = fields.size() >= 13U
        ? parseInteger<std::size_t>(
              fields[12], "object pending transportation-type changes")
        : 0U;
    auto const pendingNegotiatedDivestitureCount = fields.size() >= 14U
        ? parseInteger<std::size_t>(
              fields[13], "object pending negotiated divestitures")
        : 0U;
    auto const ownershipAssumptionRecipientCount = fields.size() >= 15U
        ? parseInteger<std::size_t>(
              fields[14], "object ownership-assumption recipient ledgers")
        : 0U;
    auto const ownershipAssumptionTagCount = fields.size() >= 16U
        ? parseInteger<std::size_t>(
              fields[15], "object ownership-assumption tag ledgers")
        : 0U;
    auto const knownObjectClassCount = fields.size() >= 17U
        ? parseInteger<std::size_t>(
              fields[16], "object known-class projections")
        : 0U;
    auto const pendingDiscoveryCount = fields.size() >= 18U
        ? parseInteger<std::size_t>(
              fields[17], "object pending discovery federates")
        : 0U;
    auto const pendingRemovalCount = fields.size() >= 19U
        ? parseInteger<std::size_t>(
              fields[18], "object pending removal federates")
        : 0U;
    auto const connectionLossAutomaticRemovalCount = fields.size() >= 20U
        ? parseInteger<std::size_t>(
              fields[19], "object connection-loss automatic removal federates")
        : 0U;
    auto const deferredConnectionLossTsoRemovalCount = fields.size() >= 21U
        ? parseInteger<std::size_t>(
              fields[20], "object deferred connection-loss TSO removals")
        : 0U;
    auto const pendingTimestampedRemovalCount = fields.size() >= 22U
        ? parseInteger<std::size_t>(
              fields[21], "object pending timestamped removals")
        : 0U;
    auto const attributeValueCount = (fields.size() == 24U || fields.size() == 25U ||
                                      fields.size() == 26U || fields.size() == 27U)
        ? parseInteger<std::size_t>(fields[23], "object application values")
        : 0U;
    auto const pendingAttributeValueUpdateCount = (fields.size() == 25U ||
                                                   fields.size() == 26U ||
                                                   fields.size() == 27U)
        ? parseInteger<std::size_t>(
              fields[24], "object pending attribute value update requests")
        : 0U;
    auto const pendingAttributeValueUpdateClassCount =
        (fields.size() == 26U || fields.size() == 27U)
        ? parseInteger<std::size_t>(
              fields[25], "object pending object-class attribute value update requests")
        : 0U;
    auto const pendingAttributeValueUpdateRegionalCount = fields.size() == 27U
        ? parseInteger<std::size_t>(
              fields[26], "object pending regional attribute value update requests")
        : 0U;
    object.attributeValuesPresent = fields.size() == 24U || fields.size() == 25U ||
                                    fields.size() == 26U || fields.size() == 27U;
    object.pendingAttributeValueUpdateRequestsPresent = fields.size() == 25U ||
                                                        fields.size() == 26U ||
                                                        fields.size() == 27U;
    object.pendingAttributeValueUpdateClassRequestsPresent = fields.size() == 26U ||
                                                              fields.size() == 27U;
    object.pendingAttributeValueUpdateRegionalRequestsPresent = fields.size() == 27U;
    if ((fields.size() == 23U || fields.size() == 24U || fields.size() == 25U ||
         fields.size() == 26U || fields.size() == 27U) &&
        fields[22] != "-") {
      object.pendingTimestampedDeletionMessageId = parseInteger<std::uint64_t>(
          fields[22], "object pending timestamped deletion message");
    }
    object.attributes.reserve(attributeCount);
    for (std::size_t attributeIndex = 0U; attributeIndex < attributeCount;
         ++attributeIndex) {
      auto const attributeFields = split(cursor.valueFor("attribute"));
      if (attributeFields.size() != 5U) {
        throw std::runtime_error("Malformed object attribute in Umbra state image.");
      }
      object.attributes.push_back({
          parseInteger<std::uint64_t>(attributeFields[0], "attribute handle"),
          parseInteger<std::uint64_t>(attributeFields[1], "attribute owner"),
          hexDecode(attributeFields[2]),
          parseInteger<std::uint32_t>(attributeFields[3], "attribute order"),
          parseIdList(attributeFields[4], "attribute regions"),
      });
    }
    object.attributeValues.reserve(attributeValueCount);
    for (std::size_t valueIndex = 0U; valueIndex < attributeValueCount;
         ++valueIndex) {
      auto const valueFields = split(cursor.valueFor("objectAttributeValue"));
      if (valueFields.size() != 2U) {
        throw std::runtime_error(
            "Malformed object application value in Umbra state image.");
      }
      object.attributeValues.push_back({
          parseInteger<std::uint64_t>(
              valueFields[0], "object application value attribute"),
          hexDecode(valueFields[1]),
      });
    }
    object.pendingAttributeValueUpdateRequests.reserve(
        pendingAttributeValueUpdateCount);
    for (std::size_t requestIndex = 0U;
         requestIndex < pendingAttributeValueUpdateCount;
         ++requestIndex) {
      auto const requestFields =
          split(cursor.valueFor("pendingAttributeValueUpdate"));
      if (requestFields.size() != 5U) {
        throw std::runtime_error(
            "Malformed pending attribute value update request in Umbra state image.");
      }
      object.pendingAttributeValueUpdateRequests.push_back({
          parseInteger<std::uint64_t>(
              requestFields[0], "attribute value update request id"),
          parseInteger<std::uint64_t>(
              requestFields[1], "attribute value update requesting federate"),
          parseInteger<std::uint64_t>(
              requestFields[2], "attribute value update providing federate"),
          parseIdList(
              requestFields[4],
              "attribute value update requested attributes"),
          hexDecode(requestFields[3]),
      });
    }
    object.pendingAttributeValueUpdateClassRequests.reserve(
        pendingAttributeValueUpdateClassCount);
    for (std::size_t requestIndex = 0U;
         requestIndex < pendingAttributeValueUpdateClassCount;
         ++requestIndex) {
      auto const requestFields =
          split(cursor.valueFor("pendingAttributeValueUpdateClass"));
      if (requestFields.size() != 6U) {
        throw std::runtime_error(
            "Malformed pending object-class attribute value update request in Umbra state image.");
      }
      object.pendingAttributeValueUpdateClassRequests.push_back({
          parseInteger<std::uint64_t>(
              requestFields[0], "object-class attribute value update request id"),
          parseInteger<std::uint64_t>(
              requestFields[1], "object-class attribute value update requesting federate"),
          parseInteger<std::uint64_t>(
              requestFields[2], "object-class attribute value update providing federate"),
          parseInteger<std::uint64_t>(
              requestFields[3], "object-class attribute value update requested class"),
          parseIdList(
              requestFields[5],
              "object-class attribute value update requested attributes"),
          hexDecode(requestFields[4]),
      });
    }
    object.pendingAttributeValueUpdateRegionalRequests.reserve(
        pendingAttributeValueUpdateRegionalCount);
    for (std::size_t requestIndex = 0U;
         requestIndex < pendingAttributeValueUpdateRegionalCount;
         ++requestIndex) {
      auto const requestFields =
          split(cursor.valueFor("pendingAttributeValueUpdateRegional"));
      if (requestFields.size() != 7U) {
        throw std::runtime_error(
            "Malformed pending regional attribute value update request in Umbra state image.");
      }
      FederationStateImagePendingAttributeValueUpdateRegional request;
      request.requestId = parseInteger<std::uint64_t>(
          requestFields[0], "regional attribute value update request id");
      request.requestingFederateId = parseInteger<std::uint64_t>(
          requestFields[1], "regional attribute value update requesting federate");
      request.providingFederateId = parseInteger<std::uint64_t>(
          requestFields[2], "regional attribute value update providing federate");
      request.requestedObjectClassHandle = parseInteger<std::uint64_t>(
          requestFields[3], "regional attribute value update requested class");
      request.userSuppliedTag = hexDecode(requestFields[4]);
      request.requestedAttributeHandles = parseIdList(
          requestFields[5],
          "regional attribute value update requested attributes");
      if (!requestFields[6].empty()) {
        auto const attributeRegionPairs = split(requestFields[6], ';');
        for (auto const& attributeRegionPair : attributeRegionPairs) {
          auto const pairFields = split(attributeRegionPair, ':');
          if (pairFields.size() != 2U) {
            throw std::runtime_error(
                "Malformed pending regional attribute value update region pair in Umbra state image.");
          }
          request.requestRegionsByAttribute.push_back({
              parseInteger<std::uint64_t>(
                  pairFields[0],
                  "regional attribute value update requested attribute"),
              parseIdList(
                  pairFields[1],
                  "regional attribute value update requested regions"),
          });
        }
      }
      object.pendingAttributeValueUpdateRegionalRequests.push_back(
          std::move(request));
    }
    object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.reserve(
        pendingIfAvailableCount);
    for (std::size_t requestIndex = 0U;
         requestIndex < pendingIfAvailableCount;
         ++requestIndex) {
      auto const requestFields =
          split(cursor.valueFor("pendingOwnershipIfAvailable"));
      if (requestFields.size() != 5U) {
        throw std::runtime_error(
            "Malformed If Available ownership reservation in Umbra state image.");
      }
      object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.push_back({
          parseInteger<std::uint64_t>(requestFields[0], "ownership request id"),
          parseInteger<std::uint64_t>(
              requestFields[1], "ownership requesting federate"),
          parseInteger<std::uint64_t>(requestFields[2], "ownership request sequence"),
          parseIdList(requestFields[4], "ownership desired attributes"),
          hexDecode(requestFields[3]),
      });
    }
    object.pendingAttributeOwnershipAcquisitionRequests.reserve(
        pendingRegularCount);
    for (std::size_t requestIndex = 0U;
         requestIndex < pendingRegularCount;
         ++requestIndex) {
      auto const requestFields = split(cursor.valueFor("pendingOwnership"));
      if (requestFields.size() != 8U) {
        throw std::runtime_error(
            "Malformed regular ownership reservation in Umbra state image.");
      }
      auto request = FederationStateImagePendingAttributeOwnershipAcquisition{
          parseInteger<std::uint64_t>(requestFields[0], "ownership request id"),
          parseInteger<std::uint64_t>(
              requestFields[1], "ownership requesting federate"),
          parseInteger<std::uint64_t>(requestFields[2], "ownership request sequence"),
          parseIdList(requestFields[4], "ownership desired attributes"),
          parseIdList(requestFields[5], "ownership queued notification attributes"),
          parseIdList(requestFields[6], "ownership unavailable attributes"),
          {},
          hexDecode(requestFields[3]),
      };
      auto const releaseCount = parseInteger<std::size_t>(
          requestFields[7], "ownership release callback count");
      request.releaseCallbacksQueuedByOwningFederate.reserve(releaseCount);
      for (std::size_t releaseIndex = 0U;
           releaseIndex < releaseCount;
           ++releaseIndex) {
        auto const releaseFields =
            split(cursor.valueFor("pendingOwnershipRelease"));
        if (releaseFields.size() != 2U) {
          throw std::runtime_error(
              "Malformed ownership release callback in Umbra state image.");
        }
        request.releaseCallbacksQueuedByOwningFederate.emplace_back(
            parseInteger<std::uint64_t>(
                releaseFields[0], "ownership release federate"),
            parseIdList(
                releaseFields[1],
                "ownership release callback attributes"));
      }
      object.pendingAttributeOwnershipAcquisitionRequests.push_back(
          std::move(request));
    }
    object.pendingAttributeOwnershipAcquisitionCancellations.reserve(
        pendingCancellationCount);
    for (std::size_t cancellationIndex = 0U;
         cancellationIndex < pendingCancellationCount;
         ++cancellationIndex) {
      auto const cancellationFields =
          split(cursor.valueFor("pendingOwnershipCancellation"));
      if (cancellationFields.size() != 3U) {
        throw std::runtime_error(
            "Malformed ownership acquisition cancellation in Umbra state image.");
      }
      object.pendingAttributeOwnershipAcquisitionCancellations.push_back({
          parseInteger<std::uint64_t>(
              cancellationFields[0], "ownership cancellation id"),
          parseInteger<std::uint64_t>(
              cancellationFields[1], "ownership cancellation federate"),
          parseIdList(
              cancellationFields[2],
              "ownership cancellation attributes"),
      });
    }
    object.pendingAttributeOwnershipDivestitureIfWantedNotifications.reserve(
        pendingDivestitureIfWantedCount);
    for (std::size_t notificationIndex = 0U;
         notificationIndex < pendingDivestitureIfWantedCount;
         ++notificationIndex) {
      auto const notificationFields =
          split(cursor.valueFor("pendingOwnershipDivestitureIfWanted"));
      if (notificationFields.size() != 3U && notificationFields.size() != 4U) {
        throw std::runtime_error(
            "Malformed Divestiture If Wanted notification in Umbra state image.");
      }
      FederationStateImagePendingAttributeOwnershipDivestitureIfWanted notification;
      notification.notificationId = parseInteger<std::uint64_t>(
          notificationFields[0], "ownership notification id");
      notification.receivingFederateId = parseInteger<std::uint64_t>(
          notificationFields[1], "ownership notification federate");
      if (notificationFields.size() == 4U) {
        notification.userSuppliedTag = hexDecode(notificationFields[2]);
        notification.attributeHandles = parseIdList(
            notificationFields[3],
            "ownership notification attributes");
      } else {
        // Accept the pre-tag v1 image form so old durable commits remain
        // readable; newly encoded images always carry the fourth field.
        notification.attributeHandles = parseIdList(
            notificationFields[2],
            "ownership notification attributes");
      }
      object.pendingAttributeOwnershipDivestitureIfWantedNotifications.push_back(
          std::move(notification));
    }
    object.pendingConfirmDivestitureNotifications.reserve(
        pendingConfirmDivestitureCount);
    for (std::size_t notificationIndex = 0U;
         notificationIndex < pendingConfirmDivestitureCount;
         ++notificationIndex) {
      auto const notificationFields =
          split(cursor.valueFor("pendingOwnershipConfirmDivestiture"));
      if (notificationFields.size() != 3U && notificationFields.size() != 4U) {
        throw std::runtime_error(
            "Malformed Confirm Divestiture notification in Umbra state image.");
      }
      FederationStateImagePendingConfirmDivestiture notification;
      notification.notificationId = parseInteger<std::uint64_t>(
          notificationFields[0], "confirm divestiture notification id");
      notification.receivingFederateId = parseInteger<std::uint64_t>(
          notificationFields[1], "confirm divestiture notification federate");
      if (notificationFields.size() == 4U) {
        notification.userSuppliedTag = hexDecode(notificationFields[2]);
        notification.attributeHandles = parseIdList(
            notificationFields[3],
            "confirm divestiture notification attributes");
      } else {
        // Accept pre-tag images; newly encoded images always carry the
        // explicit fourth field so an empty tag remains unambiguous.
        notification.attributeHandles = parseIdList(
            notificationFields[2],
            "confirm divestiture notification attributes");
      }
      object.pendingConfirmDivestitureNotifications.push_back(
          std::move(notification));
    }
    object.pendingAttributeTransportationTypeChanges.reserve(
        pendingTransportationChangeCount);
    for (std::size_t requestIndex = 0U;
         requestIndex < pendingTransportationChangeCount;
         ++requestIndex) {
      auto const requestFields =
          split(cursor.valueFor("pendingAttributeTransportationTypeChange"));
      if (requestFields.size() != 4U) {
        throw std::runtime_error(
            "Malformed attribute transportation-type change in Umbra state image.");
      }
      object.pendingAttributeTransportationTypeChanges.push_back({
          parseInteger<std::uint64_t>(
              requestFields[0], "transportation-type change request id"),
          parseInteger<std::uint64_t>(
              requestFields[1], "transportation-type change requester"),
          parseIdList(
              requestFields[2],
              "transportation-type change attributes"),
          hexDecode(requestFields[3]),
      });
    }
    object.pendingNegotiatedAttributeOwnershipDivestitures.reserve(
        pendingNegotiatedDivestitureCount);
    for (std::size_t divestitureIndex = 0U;
         divestitureIndex < pendingNegotiatedDivestitureCount;
         ++divestitureIndex) {
      auto const divestitureFields =
          split(cursor.valueFor("pendingOwnershipNegotiatedDivestiture"));
      if (divestitureFields.size() != 8U) {
        throw std::runtime_error(
            "Malformed negotiated ownership divestiture in Umbra state image.");
      }
      auto parseFlag = [](std::string_view value, char const* field) {
        auto const flag = parseInteger<std::uint32_t>(value, field);
        if (flag > 1U) {
          throw std::runtime_error(std::string{"Invalid "} + field +
                                   " in Umbra state image.");
        }
        return flag != 0U;
      };
      object.pendingNegotiatedAttributeOwnershipDivestitures.push_back({
          parseInteger<std::uint64_t>(
              divestitureFields[0], "negotiated divestiture attribute"),
          parseInteger<std::uint64_t>(
              divestitureFields[1], "negotiated divesting federate"),
          parseInteger<std::uint64_t>(
              divestitureFields[2], "negotiated acquiring federate"),
          parseInteger<std::uint64_t>(
              divestitureFields[3], "negotiated acquisition request"),
          parseFlag(divestitureFields[4], "negotiated If Available flag"),
          parseFlag(divestitureFields[5], "negotiated confirmation queued flag"),
          parseFlag(divestitureFields[6], "negotiated confirmation delivered flag"),
          hexDecode(divestitureFields[7]),
      });
    }
    object.ownershipAssumptionRecipientsByAttribute.reserve(
        ownershipAssumptionRecipientCount);
    for (std::size_t assumptionIndex = 0U;
         assumptionIndex < ownershipAssumptionRecipientCount;
         ++assumptionIndex) {
      auto const assumptionFields =
          split(cursor.valueFor("pendingOwnershipAssumptionRecipients"));
      if (assumptionFields.size() != 2U) {
        throw std::runtime_error(
            "Malformed ownership-assumption recipient ledger in Umbra state image.");
      }
      object.ownershipAssumptionRecipientsByAttribute.push_back({
          parseInteger<std::uint64_t>(
              assumptionFields[0], "ownership-assumption attribute"),
          parseIdList(
              assumptionFields[1],
              "ownership-assumption recipient federates"),
      });
    }
    object.ownershipAssumptionUserSuppliedTagsByAttribute.reserve(
        ownershipAssumptionTagCount);
    for (std::size_t assumptionIndex = 0U;
         assumptionIndex < ownershipAssumptionTagCount;
         ++assumptionIndex) {
      auto const assumptionFields =
          split(cursor.valueFor("pendingOwnershipAssumptionTag"));
      if (assumptionFields.size() != 2U) {
        throw std::runtime_error(
            "Malformed ownership-assumption tag ledger in Umbra state image.");
      }
      object.ownershipAssumptionUserSuppliedTagsByAttribute.push_back({
          parseInteger<std::uint64_t>(
              assumptionFields[0], "ownership-assumption tag attribute"),
          hexDecode(assumptionFields[1]),
      });
    }
    object.knownObjectClassHandlesByFederate.reserve(knownObjectClassCount);
    for (std::size_t knownIndex = 0U;
         knownIndex < knownObjectClassCount;
         ++knownIndex) {
      auto const knownFields = split(cursor.valueFor("objectKnownClass"));
      if (knownFields.size() != 2U) {
        throw std::runtime_error(
            "Malformed known object-class projection in Umbra state image.");
      }
      object.knownObjectClassHandlesByFederate.push_back({
          parseInteger<std::uint64_t>(
              knownFields[0], "known object-class federate"),
          parseInteger<std::uint64_t>(
              knownFields[1], "known object-class handle"),
      });
    }
    object.pendingDiscoveryFederateIds.reserve(pendingDiscoveryCount);
    for (std::size_t pendingIndex = 0U;
         pendingIndex < pendingDiscoveryCount;
         ++pendingIndex) {
      object.pendingDiscoveryFederateIds.push_back(parseInteger<std::uint64_t>(
          cursor.valueFor("objectPendingDiscovery"),
          "pending object discovery federate"));
    }
    object.pendingRemovalFederateIds.reserve(pendingRemovalCount);
    for (std::size_t pendingIndex = 0U;
         pendingIndex < pendingRemovalCount;
         ++pendingIndex) {
      object.pendingRemovalFederateIds.push_back(parseInteger<std::uint64_t>(
          cursor.valueFor("objectPendingRemoval"),
          "pending object removal federate"));
    }
    object.connectionLossAutomaticRemovalFederateIds.reserve(
        connectionLossAutomaticRemovalCount);
    for (std::size_t pendingIndex = 0U;
         pendingIndex < connectionLossAutomaticRemovalCount;
         ++pendingIndex) {
      object.connectionLossAutomaticRemovalFederateIds.push_back(
          parseInteger<std::uint64_t>(
              cursor.valueFor("objectConnectionLossAutomaticRemoval"),
              "connection-loss automatic removal federate"));
    }
    object.deferredConnectionLossTsoRemovalFederateIds.reserve(
        deferredConnectionLossTsoRemovalCount);
    for (std::size_t pendingIndex = 0U;
         pendingIndex < deferredConnectionLossTsoRemovalCount;
         ++pendingIndex) {
      object.deferredConnectionLossTsoRemovalFederateIds.push_back(
          parseInteger<std::uint64_t>(
              cursor.valueFor("objectDeferredConnectionLossTsoRemoval"),
              "deferred connection-loss TSO removal federate"));
    }
    object.pendingTimestampedRemovalFederateIds.reserve(
        pendingTimestampedRemovalCount);
    for (std::size_t pendingIndex = 0U;
         pendingIndex < pendingTimestampedRemovalCount;
         ++pendingIndex) {
      object.pendingTimestampedRemovalFederateIds.push_back(
          parseInteger<std::uint64_t>(
              cursor.valueFor("objectPendingTimestampedRemoval"),
              "pending timestamped removal federate"));
    }
    image.objects.push_back(std::move(object));
  }
  validateObjectVector(image.objects);
  auto interactionMarker = cursor.line();
  constexpr std::string_view pendingQueryPrefix =
      "pendingAttributeOwnershipQueries=";
  if (interactionMarker.starts_with(pendingQueryPrefix)) {
    auto const pendingQueryCount = parseInteger<std::size_t>(
        interactionMarker.substr(pendingQueryPrefix.size()),
        "pendingAttributeOwnershipQueries");
    image.pendingAttributeOwnershipQueriesPresent = true;
    image.pendingAttributeOwnershipQueries.reserve(pendingQueryCount);
    for (std::size_t queryIndex = 0U; queryIndex < pendingQueryCount;
         ++queryIndex) {
      auto const queryFields =
          split(cursor.valueFor("pendingAttributeOwnershipQuery"));
      if (queryFields.size() != 6U) {
        throw std::runtime_error(
            "Malformed pending Attribute Ownership query in Umbra state image.");
      }
      image.pendingAttributeOwnershipQueries.push_back({
          parseInteger<std::uint64_t>(
              queryFields[0], "Attribute Ownership query request id"),
          parseInteger<std::uint64_t>(
              queryFields[1], "Attribute Ownership query requester"),
          parseInteger<std::uint64_t>(
              queryFields[2], "Attribute Ownership query object"),
          parseInteger<std::uint32_t>(
              queryFields[3], "Attribute Ownership query report kind"),
          parseInteger<std::uint64_t>(
              queryFields[4], "Attribute Ownership query owner"),
          parseIdList(
              queryFields[5],
              "Attribute Ownership query attributes"),
      });
    }
    validatePendingAttributeOwnershipQueryVector(
        image.pendingAttributeOwnershipQueries);
    interactionMarker = cursor.line();
  }
  constexpr std::string_view pendingAssumptionPrefix =
      "pendingAttributeOwnershipAssumptionCallbacks=";
  if (interactionMarker.starts_with(pendingAssumptionPrefix)) {
    auto const pendingAssumptionCount = parseInteger<std::size_t>(
        interactionMarker.substr(pendingAssumptionPrefix.size()),
        "pendingAttributeOwnershipAssumptionCallbacks");
    image.pendingAttributeOwnershipAssumptionsPresent = true;
    image.pendingAttributeOwnershipAssumptions.reserve(pendingAssumptionCount);
    for (std::size_t callbackIndex = 0U;
         callbackIndex < pendingAssumptionCount;
         ++callbackIndex) {
      auto const callbackFields = split(
          cursor.valueFor("pendingAttributeOwnershipAssumptionCallback"));
      if (callbackFields.size() != 4U) {
        throw std::runtime_error(
            "Malformed pending Attribute Ownership Assumption callback in Umbra state image.");
      }
      image.pendingAttributeOwnershipAssumptions.push_back({
          parseInteger<std::uint64_t>(
              callbackFields[0], "Attribute Ownership Assumption callback object"),
          parseInteger<std::uint64_t>(
              callbackFields[1], "Attribute Ownership Assumption callback recipient"),
          parseIdList(
              callbackFields[2],
              "Attribute Ownership Assumption callback attributes"),
          hexDecode(callbackFields[3]),
      });
    }
    validatePendingAttributeOwnershipAssumptionVector(
        image.pendingAttributeOwnershipAssumptions);
    interactionMarker = cursor.line();
  }
  if (interactionMarker == "end") {
    // Accept object-bearing v1 payloads written before interaction
    // declarations became a typed section.
    cursor.finish();
    return image;
  }
  constexpr std::string_view interactionPrefix = "interactionDeclarations=";
  if (!interactionMarker.starts_with(interactionPrefix)) {
    throw std::runtime_error(
        "Missing interactionDeclarations section in Umbra state image.");
  }
  auto const interactionCount = parseInteger<std::size_t>(
      interactionMarker.substr(interactionPrefix.size()), "interactionDeclarations");
  image.interactionDeclarations.reserve(interactionCount);
  for (std::size_t index = 0U; index < interactionCount; ++index) {
    auto const fields = split(cursor.valueFor("interactionDeclaration"));
    if (fields.size() != 9U) {
      throw std::runtime_error(
          "Malformed interaction declaration in Umbra state image.");
    }
    FederationStateImageInteractionDeclaration declaration;
    declaration.federateId =
        parseInteger<std::uint64_t>(fields[0], "interaction federateId");
    if (declaration.federateId == 0U) {
      throw std::runtime_error(
          "Invalid interaction declaration federate identity in Umbra state image.");
    }
    auto const publishedCount =
        parseInteger<std::size_t>(fields[1], "published interactions");
    auto const subscribedCount =
        parseInteger<std::size_t>(fields[2], "subscribed interactions");
    auto const regionalCount =
        parseInteger<std::size_t>(fields[3], "regional interactions");
    auto const publishedDirectedCount =
        parseInteger<std::size_t>(fields[4], "published directed interactions");
    auto const subscribedDirectedCount =
        parseInteger<std::size_t>(fields[5], "subscribed directed interactions");
    auto const transportCount =
        parseInteger<std::size_t>(fields[6], "interaction transportation types");
    auto const orderCount =
        parseInteger<std::size_t>(fields[7], "interaction order types");
    auto const pendingTransportCount = parseInteger<std::size_t>(
        fields[8], "pending interaction transportation changes");

    declaration.publishedInteractionClasses.reserve(publishedCount);
    for (std::size_t record = 0U; record < publishedCount; ++record) {
      auto const recordFields = split(cursor.valueFor("publishedInteraction"));
      if (recordFields.size() != 2U ||
          parseInteger<std::uint64_t>(recordFields[0], "published interaction federateId") !=
              declaration.federateId) {
        throw std::runtime_error(
            "Mismatched published interaction record in Umbra state image.");
      }
      declaration.publishedInteractionClasses.push_back(
          parseInteger<std::uint64_t>(recordFields[1], "published interaction handle"));
    }

    declaration.subscribedInteractionClasses.reserve(subscribedCount);
    for (std::size_t record = 0U; record < subscribedCount; ++record) {
      auto const recordFields = split(cursor.valueFor("subscribedInteraction"));
      if (recordFields.size() != 3U ||
          parseInteger<std::uint64_t>(recordFields[0], "subscribed interaction federateId") !=
              declaration.federateId) {
        throw std::runtime_error(
            "Mismatched subscribed interaction record in Umbra state image.");
      }
      declaration.subscribedInteractionClasses.push_back({
          parseInteger<std::uint64_t>(recordFields[1], "subscribed interaction handle"),
          parseBoolean(recordFields[2], "subscribed interaction active"),
      });
    }

    declaration.regionalSubscribedInteractionClasses.reserve(regionalCount);
    for (std::size_t record = 0U; record < regionalCount; ++record) {
      auto const recordFields =
          split(cursor.valueFor("regionalSubscribedInteraction"));
      if (recordFields.size() != 4U ||
          parseInteger<std::uint64_t>(recordFields[0], "regional interaction federateId") !=
              declaration.federateId) {
        throw std::runtime_error(
            "Mismatched regional interaction record in Umbra state image.");
      }
      declaration.regionalSubscribedInteractionClasses.push_back({
          parseInteger<std::uint64_t>(recordFields[1], "regional interaction handle"),
          parseInteger<std::uint64_t>(recordFields[2], "regional interaction region"),
          parseBoolean(recordFields[3], "regional interaction active"),
      });
    }

    declaration.publishedObjectClassDirectedInteractions.reserve(
        publishedDirectedCount);
    for (std::size_t record = 0U; record < publishedDirectedCount; ++record) {
      auto const recordFields =
          split(cursor.valueFor("publishedDirectedInteraction"));
      if (recordFields.size() != 3U ||
          parseInteger<std::uint64_t>(recordFields[0], "published directed federateId") !=
              declaration.federateId) {
        throw std::runtime_error(
            "Mismatched published directed interaction record in Umbra state image.");
      }
      declaration.publishedObjectClassDirectedInteractions.push_back({
          parseInteger<std::uint64_t>(recordFields[1], "published directed object class"),
          parseInteger<std::uint64_t>(recordFields[2], "published directed interaction"),
      });
    }

    declaration.subscribedObjectClassDirectedInteractions.reserve(
        subscribedDirectedCount);
    for (std::size_t record = 0U; record < subscribedDirectedCount; ++record) {
      auto const recordFields =
          split(cursor.valueFor("subscribedDirectedInteraction"));
      if (recordFields.size() != 4U ||
          parseInteger<std::uint64_t>(recordFields[0], "subscribed directed federateId") !=
              declaration.federateId) {
        throw std::runtime_error(
            "Mismatched subscribed directed interaction record in Umbra state image.");
      }
      declaration.subscribedObjectClassDirectedInteractions.push_back({
          parseInteger<std::uint64_t>(recordFields[1], "subscribed directed object class"),
          parseInteger<std::uint64_t>(recordFields[2], "subscribed directed interaction"),
          parseBoolean(recordFields[3], "subscribed directed interaction active"),
      });
    }

    declaration.interactionTransportationTypes.reserve(transportCount);
    for (std::size_t record = 0U; record < transportCount; ++record) {
      auto const recordFields = split(cursor.valueFor("interactionTransport"));
      if (recordFields.size() != 3U ||
          parseInteger<std::uint64_t>(recordFields[0], "interaction transport federateId") !=
              declaration.federateId) {
        throw std::runtime_error(
            "Mismatched interaction transportation record in Umbra state image.");
      }
      declaration.interactionTransportationTypes.push_back({
          parseInteger<std::uint64_t>(recordFields[1], "interaction transport handle"),
          hexDecode(recordFields[2]),
      });
    }

    declaration.interactionOrderTypes.reserve(orderCount);
    for (std::size_t record = 0U; record < orderCount; ++record) {
      auto const recordFields = split(cursor.valueFor("interactionOrder"));
      if (recordFields.size() != 3U ||
          parseInteger<std::uint64_t>(recordFields[0], "interaction order federateId") !=
              declaration.federateId) {
        throw std::runtime_error(
            "Mismatched interaction order record in Umbra state image.");
      }
      declaration.interactionOrderTypes.push_back({
          parseInteger<std::uint64_t>(recordFields[1], "interaction order handle"),
          parseInteger<std::uint32_t>(recordFields[2], "interaction order type"),
      });
    }

    declaration.pendingInteractionTransportationTypeChanges.reserve(
        pendingTransportCount);
    for (std::size_t record = 0U; record < pendingTransportCount; ++record) {
      auto const recordFields =
          split(cursor.valueFor("interactionPendingTransport"));
      if (recordFields.size() != 3U ||
          parseInteger<std::uint64_t>(recordFields[0], "pending interaction transport federateId") !=
              declaration.federateId) {
        throw std::runtime_error(
            "Mismatched pending interaction transportation record in Umbra state image.");
      }
      declaration.pendingInteractionTransportationTypeChanges.push_back({
          parseInteger<std::uint64_t>(recordFields[1], "pending interaction transport handle"),
          hexDecode(recordFields[2]),
      });
    }
    image.interactionDeclarations.push_back(std::move(declaration));
  }
  validateInteractionDeclarationVector(image.interactionDeclarations);
  auto const tsoInteractionMarker = cursor.line();
  if (tsoInteractionMarker == "end") {
    // Accept interaction-declaration payloads written before the typed TSO
    // interaction-message section was introduced.
    cursor.finish();
    return image;
  }
  constexpr std::string_view tsoInteractionPrefix = "tsoInteractionMessages=";
  if (!tsoInteractionMarker.starts_with(tsoInteractionPrefix)) {
    throw std::runtime_error(
        "Missing tsoInteractionMessages section in Umbra state image.");
  }
  auto const tsoInteractionCount = parseInteger<std::size_t>(
      tsoInteractionMarker.substr(tsoInteractionPrefix.size()),
      "tsoInteractionMessages");
  image.tsoInteractionMessages.reserve(tsoInteractionCount);
  for (std::size_t index = 0U; index < tsoInteractionCount; ++index) {
    auto const fields = split(cursor.valueFor("tsoInteractionMessage"));
    if (fields.size() != 13U) {
      throw std::runtime_error(
          "Malformed TSO interaction message in Umbra state image.");
    }
    FederationStateImageTsoInteractionMessage message;
    message.messageId = parseInteger<std::uint64_t>(fields[0], "TSO interaction message id");
    message.producingFederateId =
        parseInteger<std::uint64_t>(fields[1], "TSO interaction producer");
    message.sentInteractionClassHandle =
        parseInteger<std::uint64_t>(fields[2], "TSO interaction class handle");
    message.defaultRegionUsed = parseBoolean(fields[3], "TSO interaction default region");
    message.sentOrderType =
        parseInteger<std::uint32_t>(fields[4], "TSO interaction sent order");
    message.receivedOrderType =
        parseInteger<std::uint32_t>(fields[5], "TSO interaction received order");
    message.timestampEncoding = decodeOptional(fields[6]);
    message.userSuppliedTag = hexDecode(fields[7]);
    message.transportationName = hexDecode(fields[8]);
    auto const sentParameterHandleCount =
        parseInteger<std::size_t>(fields[9], "TSO interaction parameter handles");
    auto const parameterCount =
        parseInteger<std::size_t>(fields[10], "TSO interaction parameters");
    auto const sentRegionHandleCount =
        parseInteger<std::size_t>(fields[11], "TSO interaction region handles");
    auto const regionSnapshotCount =
        parseInteger<std::size_t>(fields[12], "TSO interaction region snapshots");

    message.sentParameterHandles.reserve(sentParameterHandleCount);
    for (std::size_t record = 0U; record < sentParameterHandleCount; ++record) {
      message.sentParameterHandles.push_back(parseInteger<std::uint64_t>(
          cursor.valueFor("tsoInteractionParameterHandle"),
          "TSO interaction parameter handle"));
    }

    message.parameters.reserve(parameterCount);
    for (std::size_t record = 0U; record < parameterCount; ++record) {
      auto const parameterFields = split(cursor.valueFor("tsoInteractionParameter"));
      if (parameterFields.size() != 2U) {
        throw std::runtime_error(
            "Malformed TSO interaction parameter in Umbra state image.");
      }
      message.parameters.push_back({
          parseInteger<std::uint64_t>(
              parameterFields[0], "TSO interaction parameter handle"),
          hexDecode(parameterFields[1]),
      });
    }

    message.sentRegionHandles.reserve(sentRegionHandleCount);
    for (std::size_t record = 0U; record < sentRegionHandleCount; ++record) {
      message.sentRegionHandles.push_back(parseInteger<std::uint64_t>(
          cursor.valueFor("tsoInteractionRegionHandle"),
          "TSO interaction region handle"));
    }

    message.sentRegionSnapshots.reserve(regionSnapshotCount);
    for (std::size_t record = 0U; record < regionSnapshotCount; ++record) {
      auto const snapshotFields =
          split(cursor.valueFor("tsoInteractionRegionSnapshot"));
      if (snapshotFields.size() != 4U) {
        throw std::runtime_error(
            "Malformed TSO interaction region snapshot in Umbra state image.");
      }
      FederationStateImageInteractionRegionSnapshot snapshot;
      snapshot.regionHandle = parseInteger<std::uint64_t>(
          snapshotFields[0], "TSO interaction snapshot region handle");
      snapshot.specificationCommitted = parseBoolean(
          snapshotFields[1], "TSO interaction snapshot committed");
      auto const dimensionCount = parseInteger<std::size_t>(
          snapshotFields[2], "TSO interaction snapshot dimensions");
      auto const rangeCount = parseInteger<std::size_t>(
          snapshotFields[3], "TSO interaction snapshot ranges");
      snapshot.dimensionHandles.reserve(dimensionCount);
      for (std::size_t dimension = 0U; dimension < dimensionCount; ++dimension) {
        snapshot.dimensionHandles.push_back(parseInteger<std::uint64_t>(
            cursor.valueFor("tsoInteractionRegionDimension"),
            "TSO interaction snapshot dimension"));
      }
      snapshot.committedRangeBounds.reserve(rangeCount);
      for (std::size_t range = 0U; range < rangeCount; ++range) {
        auto const rangeFields = split(cursor.valueFor("tsoInteractionRegionRange"));
        if (rangeFields.size() != 3U) {
          throw std::runtime_error(
              "Malformed TSO interaction region range in Umbra state image.");
        }
        snapshot.committedRangeBounds.push_back({
            parseInteger<std::uint64_t>(
                rangeFields[0], "TSO interaction range dimension"),
            parseInteger<std::uint64_t>(rangeFields[1], "TSO interaction range lower"),
            parseInteger<std::uint64_t>(rangeFields[2], "TSO interaction range upper"),
        });
      }
      message.sentRegionSnapshots.push_back(std::move(snapshot));
    }
    image.tsoInteractionMessages.push_back(std::move(message));
  }
  validateTsoInteractionMessageVector(image.tsoInteractionMessages);
  auto const directedInteractionMarker = cursor.line();
  if (directedInteractionMarker == "end") {
    // Accept ordinary TSO payloads written before directed interaction
    // messages became a typed section.
    cursor.finish();
    return image;
  }
  constexpr std::string_view directedInteractionPrefix =
      "tsoDirectedInteractionMessages=";
  if (!directedInteractionMarker.starts_with(directedInteractionPrefix)) {
    throw std::runtime_error(
        "Missing tsoDirectedInteractionMessages section in Umbra state image.");
  }
  auto const directedInteractionCount = parseInteger<std::size_t>(
      directedInteractionMarker.substr(directedInteractionPrefix.size()),
      "tsoDirectedInteractionMessages");
  image.tsoDirectedInteractionMessages.reserve(directedInteractionCount);
  for (std::size_t index = 0U; index < directedInteractionCount; ++index) {
    auto const fields = split(cursor.valueFor("tsoDirectedInteractionMessage"));
    if (fields.size() != 12U) {
      throw std::runtime_error(
          "Malformed TSO directed interaction message in Umbra state image.");
    }
    FederationStateImageTsoDirectedInteractionMessage message;
    message.messageId = parseInteger<std::uint64_t>(
        fields[0], "TSO directed interaction message id");
    message.producingFederateId = parseInteger<std::uint64_t>(
        fields[1], "TSO directed interaction producer");
    message.objectInstanceHandle = parseInteger<std::uint64_t>(
        fields[2], "TSO directed interaction object instance");
    message.sentInteractionClassHandle = parseInteger<std::uint64_t>(
        fields[3], "TSO directed interaction class handle");
    message.sentOrderType = parseInteger<std::uint32_t>(
        fields[4], "TSO directed interaction sent order");
    message.receivedOrderType = parseInteger<std::uint32_t>(
        fields[5], "TSO directed interaction received order");
    message.timestampEncoding = decodeOptional(fields[6]);
    message.userSuppliedTag = hexDecode(fields[7]);
    message.transportationName = hexDecode(fields[8]);
    auto const sentParameterHandleCount = parseInteger<std::size_t>(
        fields[9], "TSO directed interaction parameter handles");
    auto const parameterCount = parseInteger<std::size_t>(
        fields[10], "TSO directed interaction parameters");
    auto const recipientCount = parseInteger<std::size_t>(
        fields[11], "TSO directed interaction recipients");

    message.sentParameterHandles.reserve(sentParameterHandleCount);
    for (std::size_t record = 0U; record < sentParameterHandleCount; ++record) {
      message.sentParameterHandles.push_back(parseInteger<std::uint64_t>(
          cursor.valueFor("tsoDirectedInteractionParameterHandle"),
          "TSO directed interaction parameter handle"));
    }
    message.parameters.reserve(parameterCount);
    for (std::size_t record = 0U; record < parameterCount; ++record) {
      auto const parameterFields =
          split(cursor.valueFor("tsoDirectedInteractionParameter"));
      if (parameterFields.size() != 2U) {
        throw std::runtime_error(
            "Malformed TSO directed interaction parameter in Umbra state image.");
      }
      message.parameters.push_back({
          parseInteger<std::uint64_t>(
              parameterFields[0], "TSO directed interaction parameter handle"),
          hexDecode(parameterFields[1]),
      });
    }
    message.recipients.reserve(recipientCount);
    for (std::size_t record = 0U; record < recipientCount; ++record) {
      auto const recipientFields =
          split(cursor.valueFor("tsoDirectedInteractionRecipient"));
      if (recipientFields.size() != 4U) {
        throw std::runtime_error(
            "Malformed TSO directed interaction recipient in Umbra state image.");
      }
      FederationStateImageTsoDirectedInteractionRecipient recipient;
      recipient.receivingFederateId = parseInteger<std::uint64_t>(
          recipientFields[0], "TSO directed interaction recipient federate");
      recipient.objectInstanceHandle = parseInteger<std::uint64_t>(
          recipientFields[1], "TSO directed interaction recipient object");
      recipient.receivedInteractionClassHandle = parseInteger<std::uint64_t>(
          recipientFields[2], "TSO directed interaction recipient class");
      auto const recipientParameterCount = parseInteger<std::size_t>(
          recipientFields[3], "TSO directed interaction recipient parameters");
      recipient.receivedParameterHandles.reserve(recipientParameterCount);
      for (std::size_t parameter = 0U; parameter < recipientParameterCount;
           ++parameter) {
        recipient.receivedParameterHandles.push_back(parseInteger<std::uint64_t>(
            cursor.valueFor("tsoDirectedInteractionRecipientParameter"),
            "TSO directed interaction recipient parameter"));
      }
      message.recipients.push_back(std::move(recipient));
    }
    image.tsoDirectedInteractionMessages.push_back(std::move(message));
  }
  validateTsoDirectedInteractionMessageVector(
      image.tsoDirectedInteractionMessages);
  auto decodeRegionSnapshot = [&cursor](
                                  std::string_view snapshotMarker,
                                  std::string_view dimensionMarker,
                                  std::string_view rangeMarker) {
    auto const snapshotFields = split(cursor.valueFor(snapshotMarker));
    if (snapshotFields.size() != 4U) {
      throw std::runtime_error(
          "Malformed TSO attribute-update region snapshot in Umbra state image.");
    }
    FederationStateImageInteractionRegionSnapshot snapshot;
    snapshot.regionHandle = parseInteger<std::uint64_t>(
        snapshotFields[0], "TSO attribute-update snapshot region handle");
    snapshot.specificationCommitted = parseBoolean(
        snapshotFields[1], "TSO attribute-update snapshot committed");
    auto const dimensionCount = parseInteger<std::size_t>(
        snapshotFields[2], "TSO attribute-update snapshot dimensions");
    auto const rangeCount = parseInteger<std::size_t>(
        snapshotFields[3], "TSO attribute-update snapshot ranges");
    snapshot.dimensionHandles.reserve(dimensionCount);
    for (std::size_t dimension = 0U; dimension < dimensionCount; ++dimension) {
      snapshot.dimensionHandles.push_back(parseInteger<std::uint64_t>(
          cursor.valueFor(dimensionMarker),
          "TSO attribute-update snapshot dimension"));
    }
    snapshot.committedRangeBounds.reserve(rangeCount);
    for (std::size_t range = 0U; range < rangeCount; ++range) {
      auto const rangeFields = split(cursor.valueFor(rangeMarker));
      if (rangeFields.size() != 3U) {
        throw std::runtime_error(
            "Malformed TSO attribute-update region range in Umbra state image.");
      }
      snapshot.committedRangeBounds.push_back({
          parseInteger<std::uint64_t>(
              rangeFields[0], "TSO attribute-update range dimension"),
          parseInteger<std::uint64_t>(
              rangeFields[1], "TSO attribute-update range lower"),
          parseInteger<std::uint64_t>(
              rangeFields[2], "TSO attribute-update range upper"),
      });
    }
    return snapshot;
  };

  auto queueMarker = cursor.line();
  constexpr std::string_view attributeUpdatePrefix =
      "tsoAttributeUpdateMessages=";
  if (queueMarker.starts_with(attributeUpdatePrefix)) {
    auto const attributeUpdateCount = parseInteger<std::size_t>(
        queueMarker.substr(attributeUpdatePrefix.size()),
        "tsoAttributeUpdateMessages");
    image.tsoAttributeUpdateMessages.reserve(attributeUpdateCount);
    for (std::size_t index = 0U; index < attributeUpdateCount; ++index) {
      auto const fields = split(cursor.valueFor("tsoAttributeUpdateMessage"));
      if (fields.size() != 8U) {
        throw std::runtime_error(
            "Malformed TSO attribute-update message in Umbra state image.");
      }
      FederationStateImageTsoAttributeUpdateMessage message;
      message.messageId = parseInteger<std::uint64_t>(
          fields[0], "TSO attribute-update message id");
      message.producingFederateId = parseInteger<std::uint64_t>(
          fields[1], "TSO attribute-update producer");
      message.objectInstanceHandle = parseInteger<std::uint64_t>(
          fields[2], "TSO attribute-update object instance");
      message.timestampEncoding = decodeOptional(fields[3]);
      message.userSuppliedTag = hexDecode(fields[4]);
      auto const attributeCount = parseInteger<std::size_t>(
          fields[5], "TSO attribute-update attributes");
      auto const recipientCount = parseInteger<std::size_t>(
          fields[6], "TSO attribute-update recipients");
      auto const messageSnapshotCount = parseInteger<std::size_t>(
          fields[7], "TSO attribute-update message snapshots");

      message.attributes.reserve(attributeCount);
      for (std::size_t attribute = 0U; attribute < attributeCount; ++attribute) {
        auto const attributeFields = split(
            cursor.valueFor("tsoAttributeUpdateAttribute"));
        if (attributeFields.size() != 2U) {
          throw std::runtime_error(
              "Malformed TSO attribute-update attribute in Umbra state image.");
        }
        message.attributes.push_back({
            parseInteger<std::uint64_t>(
                attributeFields[0], "TSO attribute-update attribute handle"),
            hexDecode(attributeFields[1]),
        });
      }

      message.passelsByRecipient.reserve(recipientCount);
      for (std::size_t recipient = 0U; recipient < recipientCount; ++recipient) {
        auto const recipientFields = split(
            cursor.valueFor("tsoAttributeUpdateRecipient"));
        if (recipientFields.size() != 2U) {
          throw std::runtime_error(
              "Malformed TSO attribute-update recipient in Umbra state image.");
        }
        FederationStateImageTsoAttributeUpdateRecipient savedRecipient;
        savedRecipient.receivingFederateId = parseInteger<std::uint64_t>(
            recipientFields[0], "TSO attribute-update recipient federate");
        auto const passelCount = parseInteger<std::size_t>(
            recipientFields[1], "TSO attribute-update recipient passels");
        savedRecipient.passels.reserve(passelCount);
        for (std::size_t passel = 0U; passel < passelCount; ++passel) {
          auto const passelFields = split(
              cursor.valueFor("tsoAttributeUpdatePassel"));
          if (passelFields.size() != 6U) {
            throw std::runtime_error(
                "Malformed TSO attribute-update passel in Umbra state image.");
          }
          FederationStateImageTsoAttributeUpdatePassel savedPassel;
          savedPassel.transportationName = hexDecode(passelFields[0]);
          savedPassel.defaultRegionUsed = parseBoolean(
              passelFields[1], "TSO attribute-update passel default region");
          savedPassel.preferredOrderType = parseInteger<std::uint32_t>(
              passelFields[2], "TSO attribute-update passel order");
          auto const passelAttributeCount = parseInteger<std::size_t>(
              passelFields[3], "TSO attribute-update passel attributes");
          auto const passelRegionCount = parseInteger<std::size_t>(
              passelFields[4], "TSO attribute-update passel regions");
          auto const passelSnapshotCount = parseInteger<std::size_t>(
              passelFields[5], "TSO attribute-update passel snapshots");
          savedPassel.sentAttributeHandles.reserve(passelAttributeCount);
          for (std::size_t handle = 0U; handle < passelAttributeCount; ++handle) {
            savedPassel.sentAttributeHandles.push_back(parseInteger<std::uint64_t>(
                cursor.valueFor("tsoAttributeUpdatePasselAttributeHandle"),
                "TSO attribute-update passel attribute handle"));
          }
          savedPassel.sentRegionHandles.reserve(passelRegionCount);
          for (std::size_t handle = 0U; handle < passelRegionCount; ++handle) {
            savedPassel.sentRegionHandles.push_back(parseInteger<std::uint64_t>(
                cursor.valueFor("tsoAttributeUpdatePasselRegionHandle"),
                "TSO attribute-update passel region handle"));
          }
          savedPassel.sentRegionSnapshots.reserve(passelSnapshotCount);
          for (std::size_t snapshot = 0U; snapshot < passelSnapshotCount; ++snapshot) {
            savedPassel.sentRegionSnapshots.push_back(decodeRegionSnapshot(
                "tsoAttributeUpdatePasselRegionSnapshot",
                "tsoAttributeUpdatePasselRegionDimension",
                "tsoAttributeUpdatePasselRegionRange"));
          }
          savedRecipient.passels.push_back(std::move(savedPassel));
        }
        message.passelsByRecipient.push_back(std::move(savedRecipient));
      }

      message.sentRegionSnapshots.reserve(messageSnapshotCount);
      for (std::size_t snapshot = 0U; snapshot < messageSnapshotCount; ++snapshot) {
        message.sentRegionSnapshots.push_back(decodeRegionSnapshot(
            "tsoAttributeUpdateRegionSnapshot",
            "tsoAttributeUpdateRegionDimension",
            "tsoAttributeUpdateRegionRange"));
      }
      image.tsoAttributeUpdateMessages.push_back(std::move(message));
    }
    validateTsoAttributeUpdateMessageVector(image.tsoAttributeUpdateMessages);
    queueMarker = cursor.line();
  }
  constexpr std::string_view objectDeletionPrefix =
      "tsoObjectDeletionMessages=";
  if (queueMarker.starts_with(objectDeletionPrefix)) {
    auto const objectDeletionCount = parseInteger<std::size_t>(
        queueMarker.substr(objectDeletionPrefix.size()),
        "tsoObjectDeletionMessages");
    image.tsoObjectDeletionMessages.reserve(objectDeletionCount);
    for (std::size_t index = 0U; index < objectDeletionCount; ++index) {
      auto const fields = split(cursor.valueFor("tsoObjectDeletionMessage"));
      if (fields.size() != 9U && fields.size() != 10U) {
        throw std::runtime_error(
            "Malformed TSO object-deletion message in Umbra state image.");
      }
      FederationStateImageTsoObjectDeletionMessage message;
      message.messageId = parseInteger<std::uint64_t>(
          fields[0], "TSO object-deletion message id");
      message.producingFederateId = parseInteger<std::uint64_t>(
          fields[1], "TSO object-deletion producer");
      message.objectInstanceHandle = parseInteger<std::uint64_t>(
          fields[2], "TSO object-deletion object instance");
      message.timestampEncoding = decodeOptional(fields[3]);
      message.userSuppliedTag = hexDecode(fields[4]);
      auto const recipientCount = parseInteger<std::size_t>(
          fields[5], "TSO object-deletion recipients");
      auto const hasReconstitution = parseBoolean(
          fields[6], "TSO object-deletion invocation snapshot");
      auto const knownClassCount = parseInteger<std::size_t>(
          fields[7], "TSO object-deletion invocation known classes");
      auto const reconstitutionAttributeCount = parseInteger<std::size_t>(
          fields[8], "TSO object-deletion invocation attributes");
      auto const reconstitutionValueCount = fields.size() == 10U
          ? parseInteger<std::size_t>(
                fields[9], "TSO object-deletion invocation values")
          : 0U;
      if (!hasReconstitution &&
          (knownClassCount != 0U || reconstitutionAttributeCount != 0U ||
           reconstitutionValueCount != 0U)) {
        throw std::runtime_error(
            "TSO object-deletion invocation snapshot counts are nonzero when absent.");
      }
      message.recipients.reserve(recipientCount);
      for (std::size_t recipient = 0U; recipient < recipientCount; ++recipient) {
        auto const recipientFields = split(
            cursor.valueFor("tsoObjectDeletionRecipient"));
        if (recipientFields.size() != 2U) {
          throw std::runtime_error(
              "Malformed TSO object-deletion recipient in Umbra state image.");
        }
        message.recipients.push_back({
            parseInteger<std::uint64_t>(
                recipientFields[0], "TSO object-deletion recipient federate"),
            parseInteger<std::uint64_t>(
                recipientFields[1], "TSO object-deletion recipient object"),
        });
      }
      if (hasReconstitution) {
        auto const objectFields = split(
            cursor.valueFor("tsoObjectDeletionReconstitution"));
        if (objectFields.size() != 6U) {
          throw std::runtime_error(
              "Malformed TSO object-deletion invocation snapshot in Umbra state image.");
        }
        FederationStateImageTsoObjectDeletionReconstitution reconstitution;
        reconstitution.object.handle = parseInteger<std::uint64_t>(
            objectFields[0], "TSO object-deletion snapshot object handle");
        reconstitution.object.name = decodeWide(objectFields[1]);
        reconstitution.object.registeredObjectClassHandle = parseInteger<std::uint64_t>(
            objectFields[2], "TSO object-deletion snapshot object class");
        reconstitution.object.producingFederateId = parseInteger<std::uint64_t>(
            objectFields[3], "TSO object-deletion snapshot producer");
        reconstitution.object.deleteAccepted = parseBoolean(
            objectFields[4], "TSO object-deletion snapshot delete state");
        reconstitution.object.pendingOperationCount = parseInteger<std::uint64_t>(
            objectFields[5], "TSO object-deletion snapshot pending operations");
        reconstitution.object.attributes.reserve(reconstitutionAttributeCount);
        for (std::size_t attribute = 0U;
             attribute < reconstitutionAttributeCount; ++attribute) {
          auto const attributeFields = split(
              cursor.valueFor("tsoObjectDeletionReconstitutionAttribute"));
          if (attributeFields.size() != 5U) {
            throw std::runtime_error(
                "Malformed TSO object-deletion invocation attribute in Umbra state image.");
          }
          reconstitution.object.attributes.push_back({
              parseInteger<std::uint64_t>(
                  attributeFields[0], "TSO object-deletion snapshot attribute handle"),
              parseInteger<std::uint64_t>(
                  attributeFields[1], "TSO object-deletion snapshot attribute owner"),
              hexDecode(attributeFields[2]),
              parseInteger<std::uint32_t>(
                  attributeFields[3], "TSO object-deletion snapshot attribute order"),
              parseIdList(
                  attributeFields[4],
                  "TSO object-deletion snapshot attribute regions"),
          });
        }
        reconstitution.object.attributeValuesPresent = fields.size() == 10U;
        reconstitution.object.attributeValues.reserve(reconstitutionValueCount);
        for (std::size_t value = 0U; value < reconstitutionValueCount; ++value) {
          auto const valueFields = split(
              cursor.valueFor("tsoObjectDeletionReconstitutionValue"));
          if (valueFields.size() != 2U) {
            throw std::runtime_error(
                "Malformed TSO object-deletion invocation value in Umbra state image.");
          }
          reconstitution.object.attributeValues.push_back({
              parseInteger<std::uint64_t>(
                  valueFields[0], "TSO object-deletion snapshot value attribute"),
              hexDecode(valueFields[1]),
          });
        }
        reconstitution.knownObjectClassHandlesByFederate.reserve(knownClassCount);
        for (std::size_t known = 0U; known < knownClassCount; ++known) {
          auto const knownFields = split(
              cursor.valueFor("tsoObjectDeletionReconstitutionKnown"));
          if (knownFields.size() != 2U) {
            throw std::runtime_error(
                "Malformed TSO object-deletion invocation known-class record in Umbra state image.");
          }
          reconstitution.knownObjectClassHandlesByFederate.emplace_back(
              parseInteger<std::uint64_t>(
                  knownFields[0], "TSO object-deletion snapshot known federate"),
              parseInteger<std::uint64_t>(
                  knownFields[1], "TSO object-deletion snapshot known object class"));
        }
        message.reconstitution = std::move(reconstitution);
      }
      image.tsoObjectDeletionMessages.push_back(std::move(message));
    }
    validateTsoObjectDeletionMessageVector(image.tsoObjectDeletionMessages);
    queueMarker = cursor.line();
  }
  constexpr std::string_view retractionPrefix =
      "tsoRequestRetractionRecords=";
  if (queueMarker.starts_with(retractionPrefix)) {
    auto const retractionCount = parseInteger<std::size_t>(
        queueMarker.substr(retractionPrefix.size()),
        "tsoRequestRetractionRecords");
    image.tsoRequestRetractionRecords.reserve(retractionCount);
    for (std::size_t index = 0U; index < retractionCount; ++index) {
      auto const fields = split(cursor.valueFor("tsoRequestRetractionRecord"));
      if (fields.size() != 8U) {
        throw std::runtime_error(
            "Malformed TSO Request Retraction record in Umbra state image.");
      }
      FederationStateImageTsoRequestRetractionRecord record;
      record.messageId = parseInteger<std::uint64_t>(
          fields[0], "TSO Request Retraction message id");
      record.producingFederateId = parseInteger<std::uint64_t>(
          fields[1], "TSO Request Retraction producer");
      record.timestampEncoding = decodeOptional(fields[2]);
      record.retractionApplied = parseBoolean(
          fields[3], "TSO Request Retraction applied");
      record.terminal = parseBoolean(fields[4], "TSO Request Retraction terminal");
      record.producerResigned = parseBoolean(
          fields[5], "TSO Request Retraction producer resigned");
      record.deliveryRequiredAfterConnectionLoss = parseBoolean(
          fields[6], "TSO Request Retraction connection-loss delivery");
      auto const recipientCount = parseInteger<std::size_t>(
          fields[7], "TSO Request Retraction recipients");
      record.recipientStates.reserve(recipientCount);
      for (std::size_t recipient = 0U; recipient < recipientCount; ++recipient) {
        auto const recipientFields = split(
            cursor.valueFor("tsoRequestRetractionRecipient"));
        if (recipientFields.size() != 2U) {
          throw std::runtime_error(
              "Malformed TSO Request Retraction recipient in Umbra state image.");
        }
        record.recipientStates.push_back({
            parseInteger<std::uint64_t>(
                recipientFields[0], "TSO Request Retraction recipient federate"),
            parseInteger<std::uint32_t>(
                recipientFields[1], "TSO Request Retraction recipient state"),
        });
      }
      image.tsoRequestRetractionRecords.push_back(std::move(record));
    }
    validateTsoRequestRetractionRecordVector(image.tsoRequestRetractionRecords);
    queueMarker = cursor.line();
  }
  if (queueMarker == "end") {
    // Accept directed-message payloads written before recipient queue phase
    // records were made explicit.
    cursor.finish();
    return image;
  }
  constexpr std::string_view queuePrefix = "tsoQueueEntries=";
  if (!queueMarker.starts_with(queuePrefix)) {
    throw std::runtime_error(
        "Missing tsoQueueEntries section in Umbra state image.");
  }
  auto const queueCount = parseInteger<std::size_t>(
      queueMarker.substr(queuePrefix.size()), "tsoQueueEntries");
  image.tsoQueueEntries.reserve(queueCount);
  for (std::size_t index = 0U; index < queueCount; ++index) {
    auto const fields = split(cursor.valueFor("tsoQueue"));
    if (fields.size() != 5U) {
      throw std::runtime_error("Malformed TSO queue entry in Umbra state image.");
    }
    image.tsoQueueEntries.push_back({
        parseInteger<std::uint64_t>(fields[1], "TSO queue message id"),
        parseInteger<std::uint64_t>(fields[0], "TSO queue recipient"),
        parseInteger<std::uint64_t>(fields[2], "TSO queue sequence"),
        parseInteger<std::uint32_t>(fields[3], "TSO queue phase"),
        decodeOptional(fields[4]),
    });
  }
  validateTsoQueueEntryVector(image.tsoQueueEntries);
  auto const reservationMarker = cursor.line();
  if (reservationMarker == "end") {
    // Accept v1 payloads written before reserved object-instance names became
    // an explicit application-state section.
    cursor.finish();
    return image;
  }
  constexpr std::string_view reservationPrefix = "reservedObjectInstanceNames=";
  if (!reservationMarker.starts_with(reservationPrefix)) {
    throw std::runtime_error(
        "Missing reservedObjectInstanceNames section in Umbra state image.");
  }
  auto const reservationCount = parseInteger<std::size_t>(
      reservationMarker.substr(reservationPrefix.size()),
      "reservedObjectInstanceNames");
  image.reservedObjectInstanceNames.reserve(reservationCount);
  for (std::size_t index = 0U; index < reservationCount; ++index) {
    auto const fields = split(cursor.valueFor("reservedObjectInstanceName"));
    if (fields.size() != 2U) {
      throw std::runtime_error(
          "Malformed reserved object-instance name in Umbra state image.");
    }
    image.reservedObjectInstanceNames.push_back({
        parseInteger<std::uint64_t>(fields[0], "reserved object-instance federate"),
        decodeWide(fields[1]),
    });
  }
  validateObjectInstanceNameReservationVector(image.reservedObjectInstanceNames);
  image.reservedObjectInstanceNamesPresent = true;
  auto declarationMarker = cursor.line();
  if (declarationMarker == "end") {
    // Accept v1 payloads written before synchronization points and
    // object-class attribute declarations became explicit state sections.
    cursor.finish();
    return image;
  }
  constexpr std::string_view synchronizationPrefix = "synchronizationPoints=";
  if (declarationMarker.starts_with(synchronizationPrefix)) {
    auto const synchronizationCount = parseInteger<std::size_t>(
        declarationMarker.substr(synchronizationPrefix.size()),
        "synchronizationPoints");
    if (image.synchronizationPointCount != synchronizationCount) {
      throw std::runtime_error(
          "Synchronization-point count does not match the typed state-image section.");
    }
    image.synchronizationPoints.reserve(synchronizationCount);
    for (std::size_t index = 0U; index < synchronizationCount; ++index) {
      auto const fields = split(cursor.valueFor("synchronizationPoint"));
      if (fields.size() != 5U) {
        throw std::runtime_error(
            "Malformed synchronization point in Umbra state image.");
      }
      FederationStateImageSynchronizationPoint point;
      point.label = decodeWide(fields[0]);
      point.userSuppliedTag = hexDecode(fields[1]);
      auto const synchronizationSetCount = parseInteger<std::size_t>(
          fields[2], "synchronization-point member count");
      auto const announcedCount = parseInteger<std::size_t>(
          fields[3], "synchronization-point announcement count");
      auto const achievedCount = parseInteger<std::size_t>(
          fields[4], "synchronization-point achievement count");
      point.synchronizationSet.reserve(synchronizationSetCount);
      for (std::size_t member = 0U; member < synchronizationSetCount; ++member) {
        point.synchronizationSet.push_back(parseInteger<std::uint64_t>(
            cursor.valueFor("synchronizationPointMember"),
            "synchronization-point member"));
      }
      point.announcedFederates.reserve(announcedCount);
      for (std::size_t announced = 0U; announced < announcedCount; ++announced) {
        point.announcedFederates.push_back(parseInteger<std::uint64_t>(
            cursor.valueFor("synchronizationPointAnnounced"),
            "synchronization-point announced federate"));
      }
      point.achievedFederates.reserve(achievedCount);
      for (std::size_t achieved = 0U; achieved < achievedCount; ++achieved) {
        auto const achievedFields = split(
            cursor.valueFor("synchronizationPointAchieved"));
        if (achievedFields.size() != 2U) {
          throw std::runtime_error(
              "Malformed synchronization-point achievement in Umbra state image.");
        }
        point.achievedFederates.emplace_back(
            parseInteger<std::uint64_t>(
                achievedFields[0], "synchronization-point achieved federate"),
            parseBoolean(
                achievedFields[1], "synchronization-point achievement result"));
      }
      image.synchronizationPoints.push_back(std::move(point));
    }
    validateSynchronizationPointVector(image.synchronizationPoints);
    image.synchronizationPointsPresent = true;
    declarationMarker = cursor.line();
    if (declarationMarker == "end") {
      // Accept v1 payloads written before object-class attribute declarations
      // became an explicit application-state section.
      cursor.finish();
      return image;
    }
  }
  constexpr std::string_view regionPrefix = "regions=";
  if (declarationMarker.starts_with(regionPrefix)) {
    auto const regionCount = parseInteger<std::size_t>(
        declarationMarker.substr(regionPrefix.size()),
        "regions");
    if (image.regionCount != regionCount) {
      throw std::runtime_error(
          "Region count does not match the typed state-image section.");
    }
    image.regions.reserve(regionCount);
    for (std::size_t index = 0U; index < regionCount; ++index) {
      auto const fields = split(cursor.valueFor("region"));
      if (fields.size() != 7U) {
        throw std::runtime_error("Malformed region in Umbra state image.");
      }
      FederationStateImageRegion region;
      region.handle = parseInteger<std::uint64_t>(fields[0], "region handle");
      region.ownerFederateId = parseInteger<std::uint64_t>(
          fields[1], "region owner");
      region.specificationCommitted = parseBoolean(
          fields[2], "region committed state");
      region.inUse = parseBoolean(fields[3], "region in-use state");
      auto const dimensionCount = parseInteger<std::size_t>(
          fields[4], "region dimensions");
      auto const pendingRangeCount = parseInteger<std::size_t>(
          fields[5], "region pending ranges");
      auto const committedRangeCount = parseInteger<std::size_t>(
          fields[6], "region committed ranges");
      region.dimensionHandles.reserve(dimensionCount);
      for (std::size_t dimension = 0U; dimension < dimensionCount; ++dimension) {
        region.dimensionHandles.push_back(parseInteger<std::uint64_t>(
            cursor.valueFor("regionDimension"),
            "region dimension"));
      }
      auto decodeRanges = [&](char const* recordName,
                              char const* fieldName,
                              std::size_t count,
                              std::vector<FederationStateImageRegionRange>& target) {
        target.reserve(count);
        for (std::size_t range = 0U; range < count; ++range) {
          auto const rangeFields = split(cursor.valueFor(recordName));
          if (rangeFields.size() != 3U) {
            throw std::runtime_error(std::string{"Malformed "} + fieldName +
                                     " in Umbra state image.");
          }
          target.push_back({
              parseInteger<std::uint64_t>(rangeFields[0], fieldName),
              parseInteger<std::uint64_t>(rangeFields[1], fieldName),
              parseInteger<std::uint64_t>(rangeFields[2], fieldName),
          });
        }
      };
      decodeRanges(
          "regionPendingRange",
          "region pending range",
          pendingRangeCount,
          region.pendingRangeBounds);
      decodeRanges(
          "regionCommittedRange",
          "region committed range",
          committedRangeCount,
          region.committedRangeBounds);
      image.regions.push_back(std::move(region));
    }
    validateRegionVector(image.regions);
    image.regionsPresent = true;
    declarationMarker = cursor.line();
    if (declarationMarker == "end") {
      // Accept v1 payloads written before object-class attribute declarations
      // became an explicit application-state section.
      cursor.finish();
      return image;
    }
  }
  constexpr std::string_view declarationPrefix =
      "objectClassAttributeDeclarations=";
  if (!declarationMarker.starts_with(declarationPrefix)) {
    throw std::runtime_error(
        "Missing objectClassAttributeDeclarations section in Umbra state image.");
  }
  auto const declarationCount = parseInteger<std::size_t>(
      declarationMarker.substr(declarationPrefix.size()),
      "objectClassAttributeDeclarations");
  if (image.objectClassDeclarationCount != declarationCount) {
    throw std::runtime_error(
        "Object-class declaration count does not match the typed state-image section.");
  }
  image.objectClassAttributeDeclarations.reserve(declarationCount);
  for (std::size_t index = 0U; index < declarationCount; ++index) {
    auto const fields = split(cursor.valueFor("objectClassAttributeDeclaration"));
    if (fields.size() != 3U) {
      throw std::runtime_error(
          "Malformed object-class attribute declaration in Umbra state image.");
    }
    FederationStateImageObjectClassAttributeDeclarations declaration;
    declaration.federateId = parseInteger<std::uint64_t>(
        fields[0], "object-class declaration federate");
    declaration.subscriptionGeneration = parseInteger<std::uint64_t>(
        fields[1], "object-class declaration subscription generation");
    auto const classCount = parseInteger<std::size_t>(
        fields[2], "object-class declaration classes");
    declaration.classes.reserve(classCount);
    for (std::size_t classIndex = 0U; classIndex < classCount; ++classIndex) {
      auto const classFields = split(
          cursor.valueFor("objectClassAttributeDeclarationClass"));
      if (classFields.size() != 10U ||
          parseInteger<std::uint64_t>(
              classFields[0], "object-class declaration class federate") !=
              declaration.federateId) {
        throw std::runtime_error(
            "Mismatched object-class declaration class in Umbra state image.");
      }
      FederationStateImageObjectClassAttributeClass objectClass;
      objectClass.objectClassHandle = parseInteger<std::uint64_t>(
          classFields[1], "object-class declaration object class");
      objectClass.privilegeToDeleteExplicitlyUnpublished = parseBoolean(
          classFields[2], "object-class declaration delete privilege");
      auto const publishedCount = parseInteger<std::size_t>(
          classFields[3], "object-class declaration published attributes");
      auto const subscribedCount = parseInteger<std::size_t>(
          classFields[4], "object-class declaration subscribed attributes");
      auto const subscribedRateCount = parseInteger<std::size_t>(
          classFields[5], "object-class declaration subscribed rates");
      auto const regionalSubscribedCount = parseInteger<std::size_t>(
          classFields[6], "object-class declaration regional subscriptions");
      auto const regionalRateCount = parseInteger<std::size_t>(
          classFields[7], "object-class declaration regional rates");
      auto const defaultTransportCount = parseInteger<std::size_t>(
          classFields[8], "object-class declaration default transports");
      auto const defaultOrderCount = parseInteger<std::size_t>(
          classFields[9], "object-class declaration default orders");

      auto requireClassIdentity = [&](std::vector<std::string_view> const& recordFields,
                                      std::size_t expectedSize,
                                      char const* field) {
        if (recordFields.size() != expectedSize ||
            parseInteger<std::uint64_t>(
                recordFields[0], field) != declaration.federateId ||
            parseInteger<std::uint64_t>(
                recordFields[1], field) != objectClass.objectClassHandle) {
          throw std::runtime_error(
              "Mismatched object-class attribute declaration record in Umbra state image.");
        }
      };

      objectClass.explicitlyPublishedAttributeHandles.reserve(publishedCount);
      for (std::size_t record = 0U; record < publishedCount; ++record) {
        auto const recordFields = split(
            cursor.valueFor("objectClassAttributePublished"));
        requireClassIdentity(recordFields, 3U, "published declaration federate");
        objectClass.explicitlyPublishedAttributeHandles.push_back(
            parseInteger<std::uint64_t>(
                recordFields[2], "published declaration attribute"));
      }

      objectClass.subscribedAttributes.reserve(subscribedCount);
      for (std::size_t record = 0U; record < subscribedCount; ++record) {
        auto const recordFields = split(
            cursor.valueFor("objectClassAttributeSubscribed"));
        requireClassIdentity(recordFields, 4U, "subscribed declaration federate");
        objectClass.subscribedAttributes.push_back({
            parseInteger<std::uint64_t>(
                recordFields[2], "subscribed declaration attribute"),
            parseBoolean(recordFields[3], "subscribed declaration active"),
        });
      }

      objectClass.subscribedUpdateRateDesignators.reserve(subscribedRateCount);
      for (std::size_t record = 0U; record < subscribedRateCount; ++record) {
        auto const recordFields = split(
            cursor.valueFor("objectClassAttributeSubscribedRate"));
        requireClassIdentity(recordFields, 4U, "subscribed-rate declaration federate");
        objectClass.subscribedUpdateRateDesignators.push_back({
            parseInteger<std::uint64_t>(
                recordFields[2], "subscribed-rate declaration attribute"),
            hexDecode(recordFields[3]),
        });
      }

      objectClass.regionalSubscribedAttributes.reserve(regionalSubscribedCount);
      for (std::size_t record = 0U; record < regionalSubscribedCount; ++record) {
        auto const recordFields = split(
            cursor.valueFor("objectClassAttributeRegionalSubscribed"));
        requireClassIdentity(recordFields, 5U, "regional declaration federate");
        objectClass.regionalSubscribedAttributes.push_back({
            parseInteger<std::uint64_t>(
                recordFields[2], "regional declaration attribute"),
            parseInteger<std::uint64_t>(
                recordFields[3], "regional declaration region"),
            parseBoolean(recordFields[4], "regional declaration active"),
        });
      }

      objectClass.regionalSubscribedUpdateRateDesignators.reserve(regionalRateCount);
      for (std::size_t record = 0U; record < regionalRateCount; ++record) {
        auto const recordFields = split(
            cursor.valueFor("objectClassAttributeRegionalSubscribedRate"));
        requireClassIdentity(recordFields, 5U, "regional-rate declaration federate");
        objectClass.regionalSubscribedUpdateRateDesignators.push_back({
            parseInteger<std::uint64_t>(
                recordFields[2], "regional-rate declaration attribute"),
            parseInteger<std::uint64_t>(
                recordFields[3], "regional-rate declaration region"),
            hexDecode(recordFields[4]),
        });
      }

      objectClass.defaultTransportationTypes.reserve(defaultTransportCount);
      for (std::size_t record = 0U; record < defaultTransportCount; ++record) {
        auto const recordFields = split(
            cursor.valueFor("objectClassAttributeDefaultTransport"));
        requireClassIdentity(recordFields, 4U, "default-transport declaration federate");
        objectClass.defaultTransportationTypes.push_back({
            parseInteger<std::uint64_t>(
                recordFields[2], "default-transport declaration attribute"),
            hexDecode(recordFields[3]),
        });
      }

      objectClass.defaultOrderTypes.reserve(defaultOrderCount);
      for (std::size_t record = 0U; record < defaultOrderCount; ++record) {
        auto const recordFields = split(
            cursor.valueFor("objectClassAttributeDefaultOrder"));
        requireClassIdentity(recordFields, 4U, "default-order declaration federate");
        objectClass.defaultOrderTypes.push_back({
            parseInteger<std::uint64_t>(
                recordFields[2], "default-order declaration attribute"),
            parseInteger<std::uint32_t>(
                recordFields[3], "default-order declaration order"),
        });
      }
      declaration.classes.push_back(std::move(objectClass));
    }
    image.objectClassAttributeDeclarations.push_back(std::move(declaration));
  }
  validateObjectClassAttributeDeclarationVector(image.objectClassAttributeDeclarations);
  image.objectClassAttributeDeclarationsPresent = true;
  cursor.expect("end");
  cursor.finish();
  return image;
}

}  // namespace umbra::detail
