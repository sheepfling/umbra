#pragma once

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

namespace umbra::detail::federation_state_image_codec_support {

inline constexpr char hexadecimal[] = "0123456789abcdef";

inline std::string hexEncode(std::string_view value) {
  std::string result;
  result.reserve(value.size() * 2U);
  for (unsigned char const byte : value) {
    result.push_back(hexadecimal[(byte >> 4U) & 0x0fU]);
    result.push_back(hexadecimal[byte & 0x0fU]);
  }
  return result;
}

inline unsigned char hexDigit(unsigned char value) {
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

inline std::string hexDecode(std::string_view value) {
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

inline std::string encodeWide(std::wstring_view value) {
  auto const encoded = utf8FromWide(value);
  if (!encoded) {
    throw std::runtime_error("A state-image text field is not valid Unicode.");
  }
  return hexEncode(*encoded);
}

inline std::wstring decodeWide(std::string_view value) {
  auto const decoded = wideFromUtf8(hexDecode(value));
  if (!decoded) {
    throw std::runtime_error("A state-image text field is not valid UTF-8.");
  }
  return *decoded;
}

inline std::string encodeOptional(std::optional<std::string> const& value) {
  return value ? hexEncode(*value) : std::string{"-"};
}

inline std::optional<std::string> decodeOptional(std::string_view value) {
  if (value == "-") {
    return std::nullopt;
  }
  return hexDecode(value);
}

template <typename Integer>
inline Integer parseInteger(std::string_view value, char const* field) {
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

inline std::vector<std::string_view> split(std::string_view value, char separator = '|') {
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

inline void validateMemberVector(std::vector<FederationStateImageMember> const& members) {
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

inline void validateNameVector(
    std::vector<std::pair<std::uint64_t, std::wstring>> const& names) {
  std::uint64_t previous = 0U;
  for (auto const& [id, name] : names) {
    if (id == 0U || id <= previous) {
      throw std::runtime_error("Invalid federate name index in Umbra state image.");
    }
    previous = id;
  }
}

inline void validateObjectInstanceNameReservationVector(
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

inline void validateSynchronizationPointVector(
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

inline void validateRegionVector(
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

inline void validateObjectClassAttributeDeclarationVector(
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

inline void validateTimeVector(std::vector<FederationStateImageTimeState> const& states) {
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

inline void validateObjectVector(std::vector<FederationStateImageObject> const& objects) {
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

inline void validateDeferredUpdateRegionAssociationVector(
    std::vector<FederationStateImageDeferredUpdateRegionAssociation> const& associations,
    std::vector<FederationStateImageObject> const& objects) {
  std::tuple<std::uint64_t, std::uint64_t, std::uint64_t> previous{};
  bool first = true;
  for (auto const& association : associations) {
    auto const current = std::tuple{
        association.objectInstanceHandle,
        association.federateId,
        association.attributeHandle,
    };
    if (association.objectInstanceHandle == 0U || association.federateId == 0U ||
        association.attributeHandle == 0U || (!first && current <= previous) ||
        association.regionHandles.empty() ||
        !std::is_sorted(
            association.regionHandles.begin(), association.regionHandles.end()) ||
        std::adjacent_find(
            association.regionHandles.begin(),
            association.regionHandles.end()) != association.regionHandles.end()) {
      throw std::runtime_error(
          "Deferred update-region associations are not strictly ordered in Umbra state image.");
    }
    auto const object = std::find_if(
        objects.begin(),
        objects.end(),
        [&association](FederationStateImageObject const& candidate) {
          return candidate.handle == association.objectInstanceHandle;
        });
    if (object == objects.end() ||
        std::none_of(
            object->attributes.begin(),
            object->attributes.end(),
            [&association](FederationStateImageObjectAttribute const& attribute) {
              return attribute.handle == association.attributeHandle;
            })) {
      throw std::runtime_error(
          "Deferred update-region association references an unknown object attribute in Umbra state image.");
    }
    previous = current;
    first = false;
    for (auto const regionHandle : association.regionHandles) {
      if (regionHandle == 0U) {
        throw std::runtime_error(
            "Deferred update-region association has an invalid region in Umbra state image.");
      }
    }
  }
}

inline void validatePendingAttributeOwnershipQueryVector(
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

inline void validatePendingAttributeOwnershipAssumptionVector(
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

inline void validateInteractionDeclarationVector(
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

inline void validateTsoInteractionMessageVector(
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

inline void validateTsoInteractionRegionSnapshot(
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

inline void validateTsoAttributeUpdateMessageVector(
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

inline void validateTsoObjectDeletionMessageVector(
    std::vector<FederationStateImageTsoObjectDeletionMessage> const& messages) {
  std::uint64_t previousMessageId = 0U;
  for (auto const& message : messages) {
    if (message.messageId == 0U || message.messageId <= previousMessageId ||
        message.producingFederateId == 0U ||
        message.objectInstanceHandle == 0U ||
        !message.timestampEncoding.has_value() ||
        message.sentOrderType == 0U || message.sentOrderType > 2U) {
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

inline void validateTsoRequestRetractionRecordVector(
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

inline void validateTsoDirectedInteractionMessageVector(
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

inline void validateTsoQueueEntryVector(
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

inline std::vector<std::uint64_t> parseIdList(
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
inline void appendScalar(std::string& result, char const* key, T value) {
  result += key;
  result += '=';
  result += std::to_string(value);
  result += '\n';
}

inline bool parseBoolean(std::string_view value, char const* field) {
  auto const parsed = parseInteger<std::uint32_t>(value, field);
  if (parsed > 1U) {
    throw std::runtime_error(std::string{"Malformed "} + field +
                             " in Umbra state image.");
  }
  return parsed != 0U;
}

}  // namespace umbra::detail::federation_state_image_codec_support
