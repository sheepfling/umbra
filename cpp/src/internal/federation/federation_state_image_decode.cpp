#include "internal/federation/federation_state_image_codec_support.hpp"

namespace umbra::detail {
using namespace federation_state_image_codec_support;

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
  constexpr std::string_view deferredUpdateRegionPrefix =
      "deferredUpdateRegionAssociations=";
  if (interactionMarker.starts_with(deferredUpdateRegionPrefix)) {
    auto const associationCount = parseInteger<std::size_t>(
        interactionMarker.substr(deferredUpdateRegionPrefix.size()),
        "deferredUpdateRegionAssociations");
    image.deferredUpdateRegionAssociationsPresent = true;
    image.deferredUpdateRegionAssociations.reserve(associationCount);
    for (std::size_t associationIndex = 0U;
         associationIndex < associationCount;
         ++associationIndex) {
      auto const fields = split(cursor.valueFor("deferredUpdateRegionAssociation"));
      if (fields.size() != 4U) {
        throw std::runtime_error(
            "Malformed deferred update-region association in Umbra state image.");
      }
      image.deferredUpdateRegionAssociations.push_back({
          parseInteger<std::uint64_t>(
              fields[0],
              "deferred update-region association object"),
          parseInteger<std::uint64_t>(
              fields[1],
              "deferred update-region association federate"),
          parseInteger<std::uint64_t>(
              fields[2],
              "deferred update-region association attribute"),
          parseIdList(fields[3], "deferred update-region association regions"),
      });
    }
    validateDeferredUpdateRegionAssociationVector(
        image.deferredUpdateRegionAssociations,
        image.objects);
    interactionMarker = cursor.line();
  }
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
      if (fields.size() != 9U && fields.size() != 10U && fields.size() != 11U) {
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
      auto const reconstitutionValueCount = fields.size() >= 10U
          ? parseInteger<std::size_t>(
                fields[9], "TSO object-deletion invocation values")
          : 0U;
      message.sentOrderType = fields.size() == 11U
          ? parseInteger<std::uint32_t>(
                fields[10], "TSO object-deletion sent order")
          : 1U;
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
      if (fields.size() != 5U && fields.size() != 6U) {
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
      if (fields.size() == 6U) {
        point.lateJoinExpansionAllowed = parseBoolean(
            fields[5], "synchronization-point late-join expansion");
      }
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
  auto const saveHistoryMarker = cursor.line();
  if (saveHistoryMarker == "end") {
    cursor.finish();
    return image;
  }
  constexpr std::string_view saveHistoryPrefix = "saveHistory=";
  if (!saveHistoryMarker.starts_with(saveHistoryPrefix)) {
    throw std::runtime_error(
        "Missing saveHistory section in Umbra state image.");
  }
  auto const saveHistoryFields = split(
      saveHistoryMarker.substr(saveHistoryPrefix.size()));
  if (saveHistoryFields.size() != 4U) {
    throw std::runtime_error(
        "Malformed saveHistory section in Umbra state image.");
  }
  image.lastSaveName = decodeWide(saveHistoryFields[0]);
  image.lastSaveTimeEncoding = decodeOptional(saveHistoryFields[1]);
  image.nextSaveName = decodeWide(saveHistoryFields[2]);
  image.nextSaveTimeEncoding = decodeOptional(saveHistoryFields[3]);
  if (image.lastSaveName.empty() && image.lastSaveTimeEncoding.has_value()) {
    throw std::runtime_error(
        "Last-save time cannot be present without a last-save name.");
  }
  if (image.nextSaveName.empty() && image.nextSaveTimeEncoding.has_value()) {
    throw std::runtime_error(
        "Next-save time cannot be present without a next-save name.");
  }
  image.saveHistoryPresent = true;
  cursor.expect("end");
  cursor.finish();
  return image;
}

}  // namespace umbra::detail
