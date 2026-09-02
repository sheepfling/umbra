#include <catch2/catch_test_macros.hpp>

#include "internal/federation/federation_registry.hpp"
#include "internal/federation/federation_state_image.hpp"
#include "internal/fom/libxml2_fom_composer.hpp"
#include "internal/fom/libxml2_fom_validator.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include <RTI/time/HLAinteger64Time.h>
#include <RTI/time/HLAinteger64Interval.h>

namespace {

using umbra::detail::EmbeddedFederationRegistry;
using umbra::detail::FederationDefinition;
using umbra::detail::FederationRegistryStatus;
using umbra::detail::FederationTimeGrantStatus;
using umbra::detail::FomModuleKind;
using umbra::detail::PrevalidatedFomModule;
using umbra::detail::InstrumentationLayer;
using umbra::detail::ObjectInstanceNameReservationStatus;
using umbra::detail::FomCompositionStatus;
using umbra::detail::FomValidationStatus;
using umbra::detail::LibXml2FomModuleComposer;
using umbra::detail::LibXml2FomValidator;

PrevalidatedFomModule module(std::wstring designator, std::wstring source) {
  return {
      std::move(designator),
      std::move(source),
      L"C:/fom/IEEE1516-DIF-2025.xsd",
      FomModuleKind::fom,
      L"IEEE1516-DIF-2025.xsd",
  };
}

FederationDefinition validDefinition() {
  return {
      {
          module(L"file:///fom/base.xml", L"C:/fom/base.xml"),
          module(L"file:///fom/extensions.xml", L"C:/fom/extensions.xml"),
      },
      L"HLAinteger64Time",
  };
}

std::filesystem::path resourcePath(std::filesystem::path const& relative) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "third_party" / "ieee1516.2-2025" / "resources" / relative;
}

PrevalidatedFomModule validatedModule(
    std::filesystem::path const& source,
    FomModuleKind kind,
    std::wstring designator) {
  LibXml2FomValidator validator;
  auto result = validator.validate({
      source,
      resourcePath("schemas/IEEE1516-DIF-2025.xsd"),
      kind,
      std::move(designator),
      L"IEEE1516-DIF-2025.xsd",
  });
  REQUIRE(result.status == FomValidationStatus::valid);
  REQUIRE(result.module.has_value());
  return *result.module;
}

FederationDefinition composedRestaurantDefinition() {
  std::vector<PrevalidatedFomModule> modules{
      validatedModule(
          resourcePath("mim/HLAstandardMIM-2025.xml"),
          FomModuleKind::mim,
          L"urn:umbra:test:registry-mim"),
      validatedModule(
          resourcePath("examples/RestaurantFOMmodule-2025.xml"),
          FomModuleKind::fom,
          L"urn:umbra:test:registry-restaurant"),
  };
  LibXml2FomModuleComposer composer(
      resourcePath("schemas/IEEE1516-FDD-2025.xsd"));
  auto result = composer.compose(modules);
  CAPTURE(result.diagnostics);
  REQUIRE(result.status == FomCompositionStatus::valid);
  REQUIRE(result.catalog);
  REQUIRE(result.fdd);
  return {
      std::move(result.modules),
      L"HLAinteger64Time",
      std::move(result.catalog),
      std::move(result.fdd),
  };
}

FederationDefinition composedDirectedInteractionDefinition() {
  std::filesystem::path const testData =
      std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  std::vector<PrevalidatedFomModule> modules{
      validatedModule(
          resourcePath("mim/HLAstandardMIM-2025.xml"),
          FomModuleKind::mim,
          L"urn:umbra:test:directed-registry-mim"),
      validatedModule(
          testData / "directed-interaction-object-consumer-fom.xml",
          FomModuleKind::fom,
          L"urn:umbra:test:directed-registry-object"),
      validatedModule(
          testData / "directed-interaction-interaction-provider-fom.xml",
          FomModuleKind::fom,
          L"urn:umbra:test:directed-registry-interaction"),
  };
  LibXml2FomModuleComposer composer(
      resourcePath("schemas/IEEE1516-FDD-2025.xsd"));
  auto result = composer.compose(modules);
  CAPTURE(result.diagnostics);
  REQUIRE(result.status == FomCompositionStatus::valid);
  REQUIRE(result.catalog);
  REQUIRE(result.fdd);
  return {
      std::move(result.modules),
      L"HLAinteger64Time",
      std::move(result.catalog),
      std::move(result.fdd),
  };
}

FederationDefinition composedParameterizedDirectedInteractionDefinition() {
  std::filesystem::path const testData =
      std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  std::vector<PrevalidatedFomModule> modules{
      validatedModule(
          resourcePath("mim/HLAstandardMIM-2025.xml"),
          FomModuleKind::mim,
          L"urn:umbra:test:directed-parameter-registry-mim"),
      validatedModule(
          testData / "directed-parameter-interaction-object-consumer-fom.xml",
          FomModuleKind::fom,
          L"urn:umbra:test:directed-parameter-registry-object"),
      validatedModule(
          testData / "directed-parameter-interaction-interaction-provider-fom.xml",
          FomModuleKind::fom,
          L"urn:umbra:test:directed-parameter-registry-interaction"),
  };
  LibXml2FomModuleComposer composer(
      resourcePath("schemas/IEEE1516-FDD-2025.xsd"));
  auto result = composer.compose(modules);
  CAPTURE(result.diagnostics);
  REQUIRE(result.status == FomCompositionStatus::valid);
  REQUIRE(result.catalog);
  REQUIRE(result.fdd);
  return {
      std::move(result.modules),
      L"HLAinteger64Time",
      std::move(result.catalog),
      std::move(result.fdd),
  };
}

std::filesystem::path temporarySaveCommitDirectory() {
  static std::atomic_uint64_t next{0U};
  auto const timestamp = std::chrono::high_resolution_clock::now()
                             .time_since_epoch()
                             .count();
  return std::filesystem::temp_directory_path() /
      ("umbra-federation-save-commit-" + std::to_string(timestamp) + "-" +
       std::to_string(++next));
}

umbra::detail::FederateCallbackRoute noOpCallbackRoute() {
  umbra::detail::FederateCallbackRoute route;
  route.submit = [](umbra::detail::FederateCallbackInvocation) {};
  return route;
}

}  // namespace

TEST_CASE(
    "Filesystem fresh-registry restore preserves one timestamped regional interaction payload and source-region snapshot",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-regional-interaction-tso-ddm][tso-queue-state]"
    "[tso-payload-state][tso-interaction-state][tso-regional-interaction-state]"
    "[interaction-management][ddm][time-management]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.send-interaction-with-regions]"
    "[federate.callback.receive-interaction]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][federate.callback.federation-restored]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto producer = source.join(
      L"exercise", L"regional-producer", L"regional-producer", noOpCallbackRoute());
  auto receiverATime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  auto receiverA = source.joinWithTimeState(
      L"exercise", receiverATime, L"regional-receiver-a", L"regional-receiver-a",
      noOpCallbackRoute());
  auto receiverBTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  auto receiverB = source.joinWithTimeState(
      L"exercise", receiverBTime, L"regional-receiver-b", L"regional-receiver-b",
      noOpCallbackRoute());
  REQUIRE(producer.membership);
  REQUIRE(receiverA.membership);
  REQUIRE(receiverB.membership);

  auto const interactionClass = source.interactionClassHandleFor(
      L"exercise",
      "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
  auto const dimension = source.dimensionHandleFor(L"exercise", "ServerId");
  REQUIRE(interactionClass.has_value());
  REQUIRE(dimension.has_value());
  REQUIRE(source.setInteractionClassPublication(
      L"exercise", producer.membership->id, *interactionClass, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);

  auto sourceRegion = source.createRegion(
      L"exercise", producer.membership->id, {*dimension});
  REQUIRE(sourceRegion.status == umbra::detail::RegionServiceStatus::applied);
  REQUIRE(sourceRegion.regionHandle != 0U);
  REQUIRE(source.setRangeBounds(
      L"exercise", producer.membership->id, sourceRegion.regionHandle, *dimension,
      umbra::detail::RegionRangeBounds{2UL, 4UL}) ==
      umbra::detail::RegionServiceStatus::applied);
  REQUIRE(source.commitRegionModifications(
      L"exercise", producer.membership->id, {sourceRegion.regionHandle}) ==
      umbra::detail::RegionServiceStatus::applied);

  auto makeReceiverRegion = [&](std::uint64_t receiverId) {
    auto region = source.createRegion(L"exercise", receiverId, {*dimension});
    REQUIRE(region.status == umbra::detail::RegionServiceStatus::applied);
    REQUIRE(region.regionHandle != 0U);
    REQUIRE(source.setRangeBounds(
        L"exercise", receiverId, region.regionHandle, *dimension,
        umbra::detail::RegionRangeBounds{2UL, 4UL}) ==
        umbra::detail::RegionServiceStatus::applied);
    REQUIRE(source.commitRegionModifications(
        L"exercise", receiverId, {region.regionHandle}) ==
        umbra::detail::RegionServiceStatus::applied);
    REQUIRE(source.setInteractionClassRegionalSubscription(
        L"exercise", receiverId, *interactionClass, {region.regionHandle}, true) ==
        umbra::detail::RegionalInteractionClassDeclarationStatus::applied);
    return region.regionHandle;
  };
  auto const receiverARegion = makeReceiverRegion(receiverA.membership->id);
  auto const receiverBRegion = makeReceiverRegion(receiverB.membership->id);

  std::set<std::uint64_t> const sentRegionHandles{sourceRegion.regionHandle};
  auto plan = source.planReceiveOrderInteraction(
      L"exercise", producer.membership->id, *interactionClass, {},
      &sentRegionHandles);
  REQUIRE(plan.status == umbra::detail::ReceiveOrderInteractionStatus::applied);
  REQUIRE(plan.transportationName == "HLAreliable");
  REQUIRE(plan.preferredOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(plan.defaultRegionUsed == false);
  REQUIRE(plan.sentRegionSnapshots.size() == 1U);
  REQUIRE(plan.sentRegionSnapshots.contains(sourceRegion.regionHandle));
  REQUIRE(plan.recipients.size() == 2U);
  REQUIRE(std::set<std::uint64_t>{
              plan.recipients[0].federateId,
              plan.recipients[1].federateId} ==
      std::set<std::uint64_t>{receiverA.membership->id, receiverB.membership->id});

  std::string const tagBytes{"regional-interaction-fs", 22U};
  umbra::detail::TsoInteractionMessage interaction;
  interaction.producingFederateId = producer.membership->id;
  interaction.sentInteractionClassHandle = *interactionClass;
  interaction.userSuppliedTag = rti1516_2025::VariableLengthData(
      tagBytes.data(), tagBytes.size());
  interaction.transportationName = plan.transportationName;
  interaction.sentRegionHandles = sentRegionHandles;
  interaction.sentRegionSnapshots = plan.sentRegionSnapshots;
  interaction.defaultRegionUsed = plan.defaultRegionUsed;
  interaction.timestamp =
      std::make_shared<rti1516_2025::HLAinteger64Time>(6);
  interaction.sentOrderType = plan.preferredOrderType;
  interaction.receivedOrderType = plan.preferredOrderType;
  auto const enqueued = source.enqueueTsoInteraction(
      L"exercise", std::move(interaction),
      {receiverA.membership->id, receiverB.membership->id},
      {receiverA.membership->id, receiverB.membership->id});
  REQUIRE(enqueued.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(enqueued.queueStatus == umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(enqueued.enqueuedRecipientCount == 2U);

  auto inTransit = source.beginTsoPayloadDelivery(
      L"exercise", receiverA.membership->id,
      rti1516_2025::HLAinteger64Time(6), true);
  REQUIRE(inTransit.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(inTransit.deliveryStatus ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(inTransit.deliveries.size() == 1U);
  auto const* inTransitInteraction =
      std::get_if<umbra::detail::TsoInteractionDelivery>(
          &inTransit.deliveries.front());
  REQUIRE(inTransitInteraction != nullptr);
  REQUIRE(inTransitInteraction->message.messageId == enqueued.messageId);
  auto const inTransitMessage = inTransitInteraction->queuedMessage;

  std::wstring const saveLabel = L"regional-interaction-tso-ddm-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", receiverA.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", receiverA.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", receiverB.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", producer.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", receiverA.membership->id).saveCompletedSuccessfully);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", receiverB.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", producer.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.tsoInteractionMessages.size() == 1U);
  auto const& savedMessage = image.tsoInteractionMessages.front();
  REQUIRE(savedMessage.messageId == enqueued.messageId);
  REQUIRE(savedMessage.sentInteractionClassHandle == *interactionClass);
  REQUIRE(savedMessage.transportationName == "HLAreliable");
  REQUIRE(savedMessage.sentRegionHandles ==
      std::vector<std::uint64_t>{sourceRegion.regionHandle});
  REQUIRE(savedMessage.sentRegionSnapshots.size() == 1U);
  REQUIRE(savedMessage.sentRegionSnapshots.front().regionHandle ==
      sourceRegion.regionHandle);
  REQUIRE(savedMessage.sentRegionSnapshots.front().dimensionHandles ==
      std::vector<std::uint64_t>{*dimension});
  REQUIRE(savedMessage.sentRegionSnapshots.front().committedRangeBounds.size() == 1U);
  REQUIRE(savedMessage.sentRegionSnapshots.front().committedRangeBounds.front().lowerBound ==
      2U);
  REQUIRE(savedMessage.sentRegionSnapshots.front().committedRangeBounds.front().upperBound ==
      4U);
  REQUIRE(savedMessage.sentOrderType ==
      static_cast<std::uint32_t>(rti1516_2025::TIMESTAMP));
  REQUIRE(savedMessage.receivedOrderType ==
      static_cast<std::uint32_t>(rti1516_2025::TIMESTAMP));
  REQUIRE(savedMessage.timestampEncoding.has_value());
  REQUIRE(image.tsoRequestRetractionRecords.size() == 1U);
  REQUIRE(image.tsoRequestRetractionRecords.front().recipientStates.size() == 2U);
  REQUIRE(image.tsoQueueEntries.size() == 2U);

  // Mutating and resigning the live source after the save must not change the
  // invocation-time region realization retained by the TSO payload.
  REQUIRE(source.setRangeBounds(
      L"exercise", producer.membership->id, sourceRegion.regionHandle, *dimension,
      umbra::detail::RegionRangeBounds{8UL, 10UL}) ==
      umbra::detail::RegionServiceStatus::applied);
  REQUIRE(source.commitRegionModifications(
      L"exercise", producer.membership->id, {sourceRegion.regionHandle}) ==
      umbra::detail::RegionServiceStatus::applied);
  REQUIRE(source.resign(
      L"exercise", producer.membership->id, rti1516_2025::NO_ACTION).status ==
      FederationRegistryStatus::applied);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedProducer = restarted.join(
      L"exercise", L"regional-producer", L"regional-producer", noOpCallbackRoute());
  auto restartedReceiverATime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  auto restartedReceiverA = restarted.joinWithTimeState(
      L"exercise", restartedReceiverATime, L"regional-receiver-a", L"regional-receiver-a",
      noOpCallbackRoute());
  auto restartedReceiverBTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  auto restartedReceiverB = restarted.joinWithTimeState(
      L"exercise", restartedReceiverBTime, L"regional-receiver-b", L"regional-receiver-b",
      noOpCallbackRoute());
  REQUIRE(restartedProducer.membership);
  REQUIRE(restartedReceiverA.membership);
  REQUIRE(restartedReceiverB.membership);
  REQUIRE(restartedProducer.membership->id == producer.membership->id);
  REQUIRE(restartedReceiverA.membership->id == receiverA.membership->id);
  REQUIRE(restartedReceiverB.membership->id == receiverB.membership->id);

  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedReceiverA.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", restartedReceiverA.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", restartedReceiverB.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", restartedProducer.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  auto const restartedInteraction = restarted.interactionClassHandleFor(
      L"exercise",
      "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
  auto const restartedDimension = restarted.dimensionHandleFor(L"exercise", "ServerId");
  REQUIRE(restartedInteraction.has_value());
  REQUIRE(restartedDimension.has_value());
  REQUIRE(*restartedInteraction == *interactionClass);
  REQUIRE(*restartedDimension == *dimension);
  auto restoredSourceBounds = restarted.rangeBoundsForRegion(
      L"exercise", restartedProducer.membership->id, sourceRegion.regionHandle,
      *restartedDimension);
  REQUIRE(restoredSourceBounds.status == umbra::detail::RegionServiceStatus::applied);
  REQUIRE(restoredSourceBounds.range.lowerBound == 2UL);
  REQUIRE(restoredSourceBounds.range.upperBound == 4UL);
  auto restoredReceiverADimensions = restarted.dimensionHandleSetForRegion(
      L"exercise", restartedReceiverA.membership->id, receiverARegion);
  auto restoredReceiverBDimensions = restarted.dimensionHandleSetForRegion(
      L"exercise", restartedReceiverB.membership->id, receiverBRegion);
  REQUIRE(restoredReceiverADimensions.dimensionHandles ==
      std::set<std::uint64_t>{*restartedDimension});
  REQUIRE(restoredReceiverBDimensions.dimensionHandles ==
      std::set<std::uint64_t>{*restartedDimension});

  // The live source region is now disjoint, but the restored payload carries
  // the accepted [2, 4) snapshot for callback-boundary DDM evaluation.
  REQUIRE(restarted.setRangeBounds(
      L"exercise", restartedProducer.membership->id, sourceRegion.regionHandle,
      *restartedDimension, umbra::detail::RegionRangeBounds{9UL, 11UL}) ==
      umbra::detail::RegionServiceStatus::applied);
  REQUIRE(restarted.commitRegionModifications(
      L"exercise", restartedProducer.membership->id, {sourceRegion.regionHandle}) ==
      umbra::detail::RegionServiceStatus::applied);
  auto liveNoOverlap = restarted.receiveOrderInteractionRecipientFor(
      L"exercise", restartedProducer.membership->id, restartedReceiverB.membership->id,
      *restartedInteraction, {}, &sentRegionHandles);
  REQUIRE_FALSE(liveNoOverlap.has_value());

  REQUIRE(restarted.completeTsoDelivery(
      L"exercise", inTransitMessage).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  auto restored = restarted.beginTsoPayloadDelivery(
      L"exercise", restartedReceiverB.membership->id,
      rti1516_2025::HLAinteger64Time(6), true);
  REQUIRE(restored.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(restored.deliveryStatus ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(restored.deliveries.size() == 1U);
  auto const* restoredInteractionPayload =
      std::get_if<umbra::detail::TsoInteractionDelivery>(
          &restored.deliveries.front());
  REQUIRE(restoredInteractionPayload != nullptr);
  auto snapshotRecipient = restarted.receiveOrderInteractionRecipientFor(
      L"exercise", restartedProducer.membership->id,
      restartedReceiverB.membership->id, *restartedInteraction, {},
      &sentRegionHandles,
      &restoredInteractionPayload->message.sentRegionSnapshots);
  REQUIRE(snapshotRecipient.has_value());
  REQUIRE(snapshotRecipient->federateId == restartedReceiverB.membership->id);
  REQUIRE(restoredInteractionPayload->message.messageId == enqueued.messageId);
  REQUIRE(restoredInteractionPayload->message.sentRegionHandles == sentRegionHandles);
  REQUIRE(restoredInteractionPayload->message.sentRegionSnapshots.size() == 1U);
  REQUIRE(restoredInteractionPayload->message.sentRegionSnapshots.at(
              sourceRegion.regionHandle).committedRangeBounds.at(*dimension).lowerBound == 2U);
  REQUIRE(restoredInteractionPayload->message.sentRegionSnapshots.at(
              sourceRegion.regionHandle).committedRangeBounds.at(*dimension).upperBound == 4U);
  REQUIRE(restoredInteractionPayload->message.transportationName == "HLAreliable");
  REQUIRE(restoredInteractionPayload->message.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(restoredInteractionPayload->message.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(restoredInteractionPayload->message.userSuppliedTag.size() == tagBytes.size());
  REQUIRE(std::string(
              static_cast<char const*>(
                  restoredInteractionPayload->message.userSuppliedTag.data()),
              tagBytes.size()) == tagBytes);
  auto const* restoredTimestamp = dynamic_cast<rti1516_2025::HLAinteger64Time const*>(
      restoredInteractionPayload->message.timestamp.get());
  REQUIRE(restoredTimestamp != nullptr);
  REQUIRE(restoredTimestamp->getTime() == 6);
  REQUIRE(restarted.completeTsoDelivery(
      L"exercise", restoredInteractionPayload->queuedMessage).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(restarted.beginTsoPayloadDelivery(
      L"exercise", restartedReceiverB.membership->id,
      rti1516_2025::HLAinteger64Time(6), true).deliveryStatus ==
      umbra::detail::FederationTsoDeliveryStatus::no_messages);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem fresh-registry restore rebinds a pending object-instance Request Attribute Value Update",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-pending-attribute-value-update][pending-application-request-state][attribute-value-update]"
    "[object-management][object-lifecycle-state][application-value-state]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto requester = source.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(owner.membership);
  REQUIRE(requester.membership);

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", requester.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", requester.membership->id, registered.objectInstanceHandle));

  std::string const valueBytes{"\x73\x74", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *efficiency,
      rti1516_2025::VariableLengthData(valueBytes.data(), valueBytes.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise",
      owner.membership->id,
      registered.objectInstanceHandle,
      *server,
      {"HLAreliable"},
      &values) == FederationRegistryStatus::applied);

  auto planned = source.planAttributeValueUpdateRequest(
      L"exercise",
      requester.membership->id,
      registered.objectInstanceHandle,
      efficiencyOnly);
  REQUIRE(planned.status ==
      umbra::detail::AttributeValueUpdateRequestStatus::applied);
  REQUIRE(planned.recipients.size() == 1U);
  REQUIRE(planned.recipients.front().providingFederateId == owner.membership->id);
  std::vector<unsigned char> const requestTag{'a', 'v', 'u', '-', 'r', 'e', 's', 't', 'a', 'r', 't'};
  auto requestId = source.registerAttributeValueUpdateRequest(
      L"exercise",
      requester.membership->id,
      owner.membership->id,
      registered.objectInstanceHandle,
      efficiencyOnly,
      requestTag);
  REQUIRE(requestId.has_value());

  std::wstring const saveLabel = L"pending-attribute-value-update-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", requester.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", requester.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.pendingAttributeValueUpdateRequests.size() == 1U);
  auto const& savedRequest =
      savedObject.pendingAttributeValueUpdateRequests.front();
  REQUIRE(savedRequest.requestId == *requestId);
  REQUIRE(savedRequest.requestingFederateId == requester.membership->id);
  REQUIRE(savedRequest.providingFederateId == owner.membership->id);
  REQUIRE(savedRequest.requestedAttributeHandles ==
      std::vector<std::uint64_t>{*efficiency});
  REQUIRE(savedRequest.userSuppliedTag ==
      std::string(requestTag.begin(), requestTag.end()));
  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedOwner = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto restartedRequester = restarted.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedRequester.membership);
  REQUIRE(restartedOwner.membership->id == owner.membership->id);
  REQUIRE(restartedRequester.membership->id == requester.membership->id);

  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedRequester.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto firstRestore = restarted.federateRestoreComplete(
      L"exercise", restartedOwner.membership->id);
  REQUIRE(firstRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(firstRestore.attributeValueUpdateProvideWorkItems.empty());
  auto restored = restarted.federateRestoreComplete(
      L"exercise", restartedRequester.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.attributeValueUpdateProvideWorkItems.size() == 1U);
  auto const& restoredWork =
      restored.attributeValueUpdateProvideWorkItems.front();
  REQUIRE(restoredWork.requestId == *requestId);
  REQUIRE(restoredWork.requestingFederateId == restartedRequester.membership->id);
  REQUIRE(restoredWork.providingFederateId == restartedOwner.membership->id);
  REQUIRE(restoredWork.objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(restoredWork.requestedAttributeHandles == efficiencyOnly);
  REQUIRE(restoredWork.userSuppliedTag == requestTag);

  auto provider = restarted.beginAttributeValueUpdateProvideRecipientFor(
      L"exercise",
      restoredWork.requestId,
      restoredWork.requestingFederateId,
      restoredWork.providingFederateId,
      restoredWork.objectInstanceHandle,
      restoredWork.requestedAttributeHandles);
  REQUIRE(provider.has_value());
  REQUIRE(provider->requestedAttributeHandles == efficiencyOnly);
  REQUIRE_FALSE(restarted.beginAttributeValueUpdateProvideRecipientFor(
      L"exercise",
      restoredWork.requestId,
      restoredWork.requestingFederateId,
      restoredWork.providingFederateId,
      restoredWork.objectInstanceHandle,
      restoredWork.requestedAttributeHandles));

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem fresh-registry restore rebinds a pending object-class Request Attribute Value Update",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-class-pending-attribute-value-update][pending-application-request-state][attribute-value-update]"
    "[object-management][object-lifecycle-state][application-value-state]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto requester = source.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(owner.membership);
  REQUIRE(requester.membership);

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);

  std::string const valueBytes{"xy", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *efficiency,
      rti1516_2025::VariableLengthData(valueBytes.data(), valueBytes.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise",
      owner.membership->id,
      registered.objectInstanceHandle,
      *server,
      {"HLAreliable"},
      &values) == FederationRegistryStatus::applied);

  auto planned = source.planAttributeValueUpdateClassRequest(
      L"exercise",
      requester.membership->id,
      *server,
      efficiencyOnly);
  REQUIRE(planned.status ==
      umbra::detail::AttributeValueUpdateClassRequestStatus::applied);
  REQUIRE(planned.recipients.size() == 1U);
  REQUIRE(planned.recipients.front().objectInstanceHandle ==
      registered.objectInstanceHandle);
  REQUIRE(planned.recipients.front().providingFederateId == owner.membership->id);
  std::vector<unsigned char> const requestTag{'c', 'l', 'a', 's', 's'};
  auto requestId = source.registerAttributeValueUpdateClassRequest(
      L"exercise",
      requester.membership->id,
      owner.membership->id,
      registered.objectInstanceHandle,
      *server,
      efficiencyOnly,
      requestTag);
  REQUIRE(requestId.has_value());

  std::wstring const saveLabel = L"pending-class-attribute-value-update-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", requester.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", requester.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.pendingAttributeValueUpdateClassRequests.size() == 1U);
  auto const& savedRequest =
      savedObject.pendingAttributeValueUpdateClassRequests.front();
  REQUIRE(savedRequest.requestId == *requestId);
  REQUIRE(savedRequest.requestingFederateId == requester.membership->id);
  REQUIRE(savedRequest.providingFederateId == owner.membership->id);
  REQUIRE(savedRequest.requestedObjectClassHandle == *server);
  REQUIRE(savedRequest.requestedAttributeHandles ==
      std::vector<std::uint64_t>{*efficiency});
  REQUIRE(savedRequest.userSuppliedTag ==
      std::string(requestTag.begin(), requestTag.end()));

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedOwner = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto restartedRequester = restarted.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedRequester.membership);
  REQUIRE(restartedOwner.membership->id == owner.membership->id);
  REQUIRE(restartedRequester.membership->id == requester.membership->id);

  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedRequester.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto firstRestore = restarted.federateRestoreComplete(
      L"exercise", restartedOwner.membership->id);
  REQUIRE(firstRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(firstRestore.attributeValueUpdateClassProvideWorkItems.empty());
  auto restored = restarted.federateRestoreComplete(
      L"exercise", restartedRequester.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.attributeValueUpdateClassProvideWorkItems.size() == 1U);
  auto const& restoredWork =
      restored.attributeValueUpdateClassProvideWorkItems.front();
  REQUIRE(restoredWork.requestId == *requestId);
  REQUIRE(restoredWork.requestingFederateId == restartedRequester.membership->id);
  REQUIRE(restoredWork.providingFederateId == restartedOwner.membership->id);
  REQUIRE(restoredWork.objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(restoredWork.requestedObjectClassHandle == *server);
  REQUIRE(restoredWork.requestedAttributeHandles == efficiencyOnly);
  REQUIRE(restoredWork.userSuppliedTag == requestTag);

  auto provider = restarted.beginAttributeValueUpdateClassProvideRecipientFor(
      L"exercise",
      restoredWork.requestId,
      restoredWork.requestingFederateId,
      restoredWork.providingFederateId,
      restoredWork.objectInstanceHandle,
      restoredWork.requestedObjectClassHandle,
      restoredWork.requestedAttributeHandles);
  REQUIRE(provider.has_value());
  REQUIRE(provider->requestedAttributeHandles == efficiencyOnly);
  REQUIRE_FALSE(restarted.beginAttributeValueUpdateClassProvideRecipientFor(
      L"exercise",
      restoredWork.requestId,
      restoredWork.requestingFederateId,
      restoredWork.providingFederateId,
      restoredWork.objectInstanceHandle,
      restoredWork.requestedObjectClassHandle,
      restoredWork.requestedAttributeHandles));

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem fresh-registry restore rebinds a pending regional object-class Request Attribute Value Update",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-regional-pending-attribute-value-update][process-restart-regional-pending-attribute-value-update-negative][process-restart-regional-pending-attribute-value-update-provider-departure][pending-application-request-state][attribute-value-update]"
    "[object-management][ddm][region-state][object-lifecycle-state][application-value-state]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto requester = source.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(owner.membership);
  REQUIRE(requester.membership);

  auto const soda = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Food.Drink.Soda", "Flavor");
  auto const dimension = source.dimensionHandleFor(L"exercise", "SodaFlavor");
  REQUIRE(soda.has_value());
  REQUIRE(flavor.has_value());
  REQUIRE(dimension.has_value());
  std::set<std::uint64_t> const flavorOnly{*flavor};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *soda, flavorOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto ownerRegion = source.createRegion(
      L"exercise", owner.membership->id, {*dimension});
  auto requesterRegion = source.createRegion(
      L"exercise", requester.membership->id, {*dimension});
  REQUIRE(ownerRegion.status == umbra::detail::RegionServiceStatus::applied);
  REQUIRE(requesterRegion.status == umbra::detail::RegionServiceStatus::applied);
  REQUIRE(source.setRangeBounds(
      L"exercise", owner.membership->id, ownerRegion.regionHandle, *dimension,
      umbra::detail::RegionRangeBounds{0UL, 2UL}) ==
      umbra::detail::RegionServiceStatus::applied);
  REQUIRE(source.setRangeBounds(
      L"exercise", requester.membership->id, requesterRegion.regionHandle,
      *dimension, umbra::detail::RegionRangeBounds{1UL, 3UL}) ==
      umbra::detail::RegionServiceStatus::applied);
  REQUIRE(source.commitRegionModifications(
      L"exercise", owner.membership->id, {ownerRegion.regionHandle}) ==
      umbra::detail::RegionServiceStatus::applied);
  REQUIRE(source.commitRegionModifications(
      L"exercise", requester.membership->id, {requesterRegion.regionHandle}) ==
      umbra::detail::RegionServiceStatus::applied);

  std::map<std::uint64_t, std::set<std::uint64_t>> const ownerUpdateRegions{
      {*flavor, {ownerRegion.regionHandle}}};
  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *soda, &ownerUpdateRegions);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  std::string const valueBytes{"regional-value", 14U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *flavor,
      rti1516_2025::VariableLengthData(valueBytes.data(), valueBytes.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise",
      owner.membership->id,
      registered.objectInstanceHandle,
      *soda,
      {"HLAreliable"},
      &values) == FederationRegistryStatus::applied);

  std::map<std::uint64_t, std::set<std::uint64_t>> const requestRegions{
      {*flavor, {requesterRegion.regionHandle}}};
  auto planned = source.planAttributeValueUpdateClassRequest(
      L"exercise", requester.membership->id, *soda, flavorOnly,
      &requestRegions);
  REQUIRE(planned.status ==
      umbra::detail::AttributeValueUpdateClassRequestStatus::applied);
  REQUIRE(planned.recipients.size() == 1U);
  REQUIRE(planned.recipients.front().objectInstanceHandle ==
      registered.objectInstanceHandle);
  REQUIRE(planned.recipients.front().providingFederateId == owner.membership->id);
  REQUIRE(planned.recipients.front().requestedAttributeHandles == flavorOnly);

  std::vector<unsigned char> const requestTag{
      'r', 'e', 'g', '-', 'r', 'e', 's', 't', 'a', 'r', 't'};
  auto requestId = source.registerAttributeValueUpdateRegionalRequest(
      L"exercise",
      requester.membership->id,
      owner.membership->id,
      registered.objectInstanceHandle,
      *soda,
      flavorOnly,
      requestRegions,
      requestTag);
  REQUIRE(requestId.has_value());

  std::wstring const saveLabel =
      L"pending-regional-attribute-value-update-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", requester.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", requester.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.pendingAttributeValueUpdateRegionalRequests.size() == 1U);
  auto const& savedRequest =
      savedObject.pendingAttributeValueUpdateRegionalRequests.front();
  REQUIRE(savedRequest.requestId == *requestId);
  REQUIRE(savedRequest.requestingFederateId == requester.membership->id);
  REQUIRE(savedRequest.providingFederateId == owner.membership->id);
  REQUIRE(savedRequest.requestedObjectClassHandle == *soda);
  REQUIRE(savedRequest.requestedAttributeHandles ==
      std::vector<std::uint64_t>{*flavor});
  REQUIRE(savedRequest.requestRegionsByAttribute ==
      std::vector<std::pair<std::uint64_t, std::vector<std::uint64_t>>>{
          {*flavor, {requesterRegion.regionHandle}}});
  REQUIRE(savedRequest.userSuppliedTag ==
      std::string(requestTag.begin(), requestTag.end()));
  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedOwner = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto restartedRequester = restarted.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedRequester.membership);
  REQUIRE(restartedOwner.membership->id == owner.membership->id);
  REQUIRE(restartedRequester.membership->id == requester.membership->id);

  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedRequester.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto firstRestore = restarted.federateRestoreComplete(
      L"exercise", restartedOwner.membership->id);
  REQUIRE(firstRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(firstRestore.attributeValueUpdateRegionalProvideWorkItems.empty());
  auto restored = restarted.federateRestoreComplete(
      L"exercise", restartedRequester.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.attributeValueUpdateRegionalProvideWorkItems.size() == 1U);
  auto const& restoredWork =
      restored.attributeValueUpdateRegionalProvideWorkItems.front();
  REQUIRE(restoredWork.requestId == *requestId);
  REQUIRE(restoredWork.requestingFederateId == restartedRequester.membership->id);
  REQUIRE(restoredWork.providingFederateId == restartedOwner.membership->id);
  REQUIRE(restoredWork.objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(restoredWork.requestedObjectClassHandle == *soda);
  REQUIRE(restoredWork.requestedAttributeHandles == flavorOnly);
  REQUIRE(restoredWork.requestRegionsByAttribute == requestRegions);
  REQUIRE(restoredWork.userSuppliedTag == requestTag);

  auto provider = restarted.beginAttributeValueUpdateRegionalProvideRecipientFor(
      L"exercise",
      restoredWork.requestId,
      restoredWork.requestingFederateId,
      restoredWork.providingFederateId,
      restoredWork.objectInstanceHandle,
      restoredWork.requestedObjectClassHandle,
      restoredWork.requestedAttributeHandles,
      restoredWork.requestRegionsByAttribute);
  REQUIRE(provider.has_value());
  REQUIRE(provider->requestedAttributeHandles == flavorOnly);
  REQUIRE_FALSE(restarted.beginAttributeValueUpdateRegionalProvideRecipientFor(
      L"exercise",
      restoredWork.requestId,
      restoredWork.requestingFederateId,
      restoredWork.providingFederateId,
      restoredWork.objectInstanceHandle,
      restoredWork.requestedObjectClassHandle,
      restoredWork.requestedAttributeHandles,
      restoredWork.requestRegionsByAttribute));

  // The callback-entry boundary is one-shot even when the supplied identity
  // is wrong.  A retry with the original identity must not resurrect work
  // that was already consumed by the mismatched attempt.
  auto mismatchedRequestId = restarted.registerAttributeValueUpdateRegionalRequest(
      L"exercise",
      restartedRequester.membership->id,
      restartedOwner.membership->id,
      registered.objectInstanceHandle,
      *soda,
      flavorOnly,
      requestRegions,
      requestTag);
  REQUIRE(mismatchedRequestId.has_value());
  REQUIRE_FALSE(restarted.beginAttributeValueUpdateRegionalProvideRecipientFor(
      L"exercise",
      *mismatchedRequestId,
      restartedOwner.membership->id,
      restartedRequester.membership->id,
      registered.objectInstanceHandle,
      *soda,
      flavorOnly,
      requestRegions));
  REQUIRE_FALSE(restarted.beginAttributeValueUpdateRegionalProvideRecipientFor(
      L"exercise",
      *mismatchedRequestId,
      restartedRequester.membership->id,
      restartedOwner.membership->id,
      registered.objectInstanceHandle,
      *soda,
      flavorOnly,
      requestRegions));

  // Region identity is retained across save/restore, but its committed bounds
  // may change before the callback begins.  A now-disjoint request is
  // suppressed and remains consumed exactly once.
  auto staleRegionRequestId = restarted.registerAttributeValueUpdateRegionalRequest(
      L"exercise",
      restartedRequester.membership->id,
      restartedOwner.membership->id,
      registered.objectInstanceHandle,
      *soda,
      flavorOnly,
      requestRegions,
      requestTag);
  REQUIRE(staleRegionRequestId.has_value());
  auto departedRequesterRequestId =
      restarted.registerAttributeValueUpdateRegionalRequest(
          L"exercise",
          restartedRequester.membership->id,
          restartedOwner.membership->id,
          registered.objectInstanceHandle,
          *soda,
          flavorOnly,
          requestRegions,
          requestTag);
  REQUIRE(departedRequesterRequestId.has_value());
  REQUIRE(restarted.setRangeBounds(
      L"exercise",
      restartedRequester.membership->id,
      requesterRegion.regionHandle,
      *dimension,
      umbra::detail::RegionRangeBounds{3UL, 4UL}) ==
      umbra::detail::RegionServiceStatus::applied);
  REQUIRE(restarted.commitRegionModifications(
      L"exercise",
      restartedRequester.membership->id,
      {requesterRegion.regionHandle}) ==
      umbra::detail::RegionServiceStatus::applied);
  REQUIRE_FALSE(restarted.beginAttributeValueUpdateRegionalProvideRecipientFor(
      L"exercise",
      *staleRegionRequestId,
      restartedRequester.membership->id,
      restartedOwner.membership->id,
      registered.objectInstanceHandle,
      *soda,
      flavorOnly,
      requestRegions));
  REQUIRE_FALSE(restarted.beginAttributeValueUpdateRegionalProvideRecipientFor(
      L"exercise",
      *staleRegionRequestId,
      restartedRequester.membership->id,
      restartedOwner.membership->id,
      registered.objectInstanceHandle,
      *soda,
      flavorOnly,
      requestRegions));

  // Provider departure is a separate lifecycle fence: an accepted request
  // must be retired when the providing joined federate resigns before the
  // callback-entry boundary, and must not become a late Provide callback.
  REQUIRE(restarted.setRangeBounds(
      L"exercise",
      restartedRequester.membership->id,
      requesterRegion.regionHandle,
      *dimension,
      umbra::detail::RegionRangeBounds{1UL, 3UL}) ==
      umbra::detail::RegionServiceStatus::applied);
  REQUIRE(restarted.commitRegionModifications(
      L"exercise",
      restartedRequester.membership->id,
      {requesterRegion.regionHandle}) ==
      umbra::detail::RegionServiceStatus::applied);
  auto providerDepartureRequestId =
      restarted.registerAttributeValueUpdateRegionalRequest(
          L"exercise",
          restartedRequester.membership->id,
          restartedOwner.membership->id,
          registered.objectInstanceHandle,
          *soda,
          flavorOnly,
          requestRegions,
          requestTag);
  REQUIRE(providerDepartureRequestId.has_value());
  REQUIRE(restarted.resign(
      L"exercise",
      restartedOwner.membership->id,
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST).status ==
      FederationRegistryStatus::applied);
  REQUIRE_FALSE(restarted.beginAttributeValueUpdateRegionalProvideRecipientFor(
      L"exercise",
      *providerDepartureRequestId,
      restartedRequester.membership->id,
      restartedOwner.membership->id,
      registered.objectInstanceHandle,
      *soda,
      flavorOnly,
      requestRegions));
  REQUIRE_FALSE(restarted.beginAttributeValueUpdateRegionalProvideRecipientFor(
      L"exercise",
      *providerDepartureRequestId,
      restartedRequester.membership->id,
      restartedOwner.membership->id,
      registered.objectInstanceHandle,
      *soda,
      flavorOnly,
      requestRegions));

  // A pending request cannot induce a callback after its requester resigns;
  // the request is consumed at the same callback boundary.
  REQUIRE(restarted.resign(
      L"exercise",
      restartedRequester.membership->id,
      rti1516_2025::NO_ACTION).status == FederationRegistryStatus::applied);
  REQUIRE_FALSE(restarted.beginAttributeValueUpdateRegionalProvideRecipientFor(
      L"exercise",
      *departedRequesterRequestId,
      restartedRequester.membership->id,
      restartedOwner.membership->id,
      registered.objectInstanceHandle,
      *soda,
      flavorOnly,
      requestRegions));
  REQUIRE_FALSE(restarted.beginAttributeValueUpdateRegionalProvideRecipientFor(
      L"exercise",
      *departedRequesterRequestId,
      restartedRequester.membership->id,
      restartedOwner.membership->id,
      registered.objectInstanceHandle,
      *soda,
      flavorOnly,
      requestRegions));

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem federation save commits publish unique durable envelopes",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  umbra::detail::FilesystemFederationSaveCommitStore store(directory);
  umbra::detail::FederationSaveCommitDescriptor descriptor{
      L"exercise / unsafe",
      L"checkpoint \"one\"",
      L"HLAinteger64Time",
      {7U, 42U},
      true};
  umbra::detail::FederationStateImage stateImage;
  stateImage.federationName = descriptor.federationName;
  stateImage.logicalTimeImplementationName = descriptor.logicalTimeImplementationName;
  stateImage.members = {{7U, L"alice", L"trainer", 0U, 0U, 0, 0U}};
  descriptor.stateImage = umbra::detail::FederationStateImageCodec::encode(stateImage);

  REQUIRE_NOTHROW(store.commit(descriptor));
  REQUIRE_NOTHROW(store.commit(descriptor));
  REQUIRE(std::filesystem::is_directory(directory));

  std::vector<std::filesystem::path> manifests;
  for (auto const& entry : std::filesystem::directory_iterator(directory)) {
    if (entry.is_regular_file() && entry.path().extension() == ".json") {
      manifests.push_back(entry.path());
    }
  }
  REQUIRE(manifests.size() == 2U);
  std::ifstream input(manifests.front(), std::ios::binary);
  std::string contents{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
  REQUIRE(contents.find("umbra-federation-save-commit/v1") != std::string::npos);
  REQUIRE(contents.find("exercise / unsafe") != std::string::npos);
  REQUIRE(contents.find("checkpoint \\\"one\\\"") != std::string::npos);
  REQUIRE(contents.find("\"timed\": true") != std::string::npos);
  REQUIRE(contents.find("\"memberFederateIds\": [7, 42]") != std::string::npos);

  auto loaded = store.load(L"exercise / unsafe", L"checkpoint \"one\"");
  REQUIRE(loaded.has_value());
  REQUIRE(loaded->logicalTimeImplementationName == L"HLAinteger64Time");
  REQUIRE(loaded->memberFederateIds == std::vector<std::uint64_t>{7U, 42U});
  REQUIRE(loaded->timed);
  REQUIRE(loaded->stateImage == descriptor.stateImage);
  REQUIRE(umbra::detail::FederationStateImageCodec::decode(loaded->stateImage)
      .federationName == descriptor.federationName);
  REQUIRE_FALSE(store.load(L"missing", L"checkpoint").has_value());

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores a by-ownership directed interaction and follows target ownership handoff",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-directed-interaction-ownership-handoff][interaction-declaration-state]"
    "[directed-interaction][directed-routing][ownership-ledger-state][object-visibility-state]"
    "[application-value-state][ownership-management][declaration-management]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedDirectedInteractionDefinition()).status ==
      FederationRegistryStatus::applied);
  auto publisher = source.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto initialOwner = source.join(
      L"exercise", L"owner", L"initial-owner", noOpCallbackRoute());
  auto handoffOwner = source.join(
      L"exercise", L"owner", L"handoff-owner", noOpCallbackRoute());
  REQUIRE(publisher.status == FederationRegistryStatus::applied);
  REQUIRE(initialOwner.status == FederationRegistryStatus::applied);
  REQUIRE(handoffOwner.status == FederationRegistryStatus::applied);
  REQUIRE(publisher.membership);
  REQUIRE(initialOwner.membership);
  REQUIRE(handoffOwner.membership);
  auto const publisherId = publisher.membership->id;
  auto const initialOwnerId = initialOwner.membership->id;
  auto const handoffOwnerId = handoffOwner.membership->id;

  auto const objectClass = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject");
  auto const interactionClass = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.UmbraDirectedFixtureInteraction");
  auto const marker = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject", "DirectedTargetMarker");
  auto const privilegeToDelete = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject",
      "HLAprivilegeToDeleteObject");
  REQUIRE(objectClass.has_value());
  REQUIRE(interactionClass.has_value());
  REQUIRE(marker.has_value());
  REQUIRE(privilegeToDelete.has_value());

  std::set<std::uint64_t> const markerOnly{*marker};
  std::set<std::uint64_t> const ownershipTargetAttributes{
      *marker, *privilegeToDelete};
  std::set<std::uint64_t> const directedOnly{*interactionClass};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", initialOwnerId, *objectClass, markerOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", handoffOwnerId, *objectClass, markerOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", publisherId, *objectClass, markerOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", handoffOwnerId, *objectClass, markerOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.publishObjectClassDirectedInteractions(
      L"exercise", publisherId, *objectClass, directedOnly) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);
  REQUIRE(source.subscribeObjectClassDirectedInteractions(
      L"exercise", initialOwnerId, *objectClass, directedOnly, false) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);
  REQUIRE(source.subscribeObjectClassDirectedInteractions(
      L"exercise", handoffOwnerId, *objectClass, directedOnly, false) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", initialOwnerId, *objectClass);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", publisherId, registered.objectInstanceHandle).has_value());
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", handoffOwnerId, registered.objectInstanceHandle).has_value());

  std::string const markerValue{"directed-owner-handoff", 22U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *marker,
      rti1516_2025::VariableLengthData(markerValue.data(), markerValue.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise", initialOwnerId, registered.objectInstanceHandle, *objectClass,
      {"HLAreliable"}, &values) == FederationRegistryStatus::applied);

  auto sourcePlan = source.planReceiveOrderDirectedInteraction(
      L"exercise", publisherId, registered.objectInstanceHandle,
      *interactionClass, {});
  REQUIRE(sourcePlan.status ==
      umbra::detail::ReceiveOrderDirectedInteractionStatus::applied);
  REQUIRE(sourcePlan.recipients.size() == 1U);
  REQUIRE(sourcePlan.recipients.front().federateId == initialOwnerId);

  std::wstring const saveLabel =
      L"directed-interaction-ownership-handoff-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", publisherId, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", publisherId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", initialOwnerId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", handoffOwnerId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", publisherId).saveCompletedSuccessfully);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", initialOwnerId).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", handoffOwnerId).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  REQUIRE(image.objects.front().knownObjectClassHandlesByFederate.size() == 3U);
  REQUIRE(image.objects.front().attributeValuesPresent);
  auto const savedMarker = std::find_if(
      image.objects.front().attributes.begin(),
      image.objects.front().attributes.end(),
      [marker](auto const& attribute) { return attribute.handle == *marker; });
  REQUIRE(savedMarker != image.objects.front().attributes.end());
  REQUIRE(savedMarker->ownerFederateId == initialOwnerId);
  REQUIRE(image.interactionDeclarations.size() == 3U);
  REQUIRE(image.interactionDeclarations[0]
              .publishedObjectClassDirectedInteractions.size() == 1U);
  REQUIRE(image.interactionDeclarations[1]
              .subscribedObjectClassDirectedInteractions.size() == 1U);
  REQUIRE(image.interactionDeclarations[2]
              .subscribedObjectClassDirectedInteractions.size() == 1U);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedDirectedInteractionDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedPublisher = restarted.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto restartedInitialOwner = restarted.join(
      L"exercise", L"owner", L"initial-owner", noOpCallbackRoute());
  auto restartedHandoffOwner = restarted.join(
      L"exercise", L"owner", L"handoff-owner", noOpCallbackRoute());
  REQUIRE(restartedPublisher.status == FederationRegistryStatus::applied);
  REQUIRE(restartedInitialOwner.status == FederationRegistryStatus::applied);
  REQUIRE(restartedHandoffOwner.status == FederationRegistryStatus::applied);
  REQUIRE(restartedPublisher.membership);
  REQUIRE(restartedInitialOwner.membership);
  REQUIRE(restartedHandoffOwner.membership);
  REQUIRE(restartedPublisher.membership->id == publisherId);
  REQUIRE(restartedInitialOwner.membership->id == initialOwnerId);
  REQUIRE(restartedHandoffOwner.membership->id == handoffOwnerId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", publisherId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", publisherId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", initialOwnerId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(
      L"exercise", handoffOwnerId);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  auto restoredInitialOwnerState = restarted.attributeOwnedByFederate(
      L"exercise", initialOwnerId, registered.objectInstanceHandle, *marker);
  REQUIRE(restoredInitialOwnerState.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE(restoredInitialOwnerState.ownedByRequestingFederate);
  auto restoredHandoffOwnerState = restarted.attributeOwnedByFederate(
      L"exercise", handoffOwnerId, registered.objectInstanceHandle, *marker);
  REQUIRE(restoredHandoffOwnerState.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE_FALSE(restoredHandoffOwnerState.ownedByRequestingFederate);

  auto restoredBeforeHandoff = restarted.planReceiveOrderDirectedInteraction(
      L"exercise", publisherId, registered.objectInstanceHandle,
      *interactionClass, {});
  REQUIRE(restoredBeforeHandoff.status ==
      umbra::detail::ReceiveOrderDirectedInteractionStatus::applied);
  REQUIRE(restoredBeforeHandoff.recipients.size() == 1U);
  REQUIRE(restoredBeforeHandoff.recipients.front().federateId == initialOwnerId);

  std::vector<unsigned char> const acquisitionTag{
      'o', 'w', 'n', 'e', 'r', '-', 'h', 'a', 'n', 'd', 'o', 'f', 'f'};
  auto acquisition = restarted.planAttributeOwnershipAcquisition(
      L"exercise", handoffOwnerId, registered.objectInstanceHandle,
      ownershipTargetAttributes, acquisitionTag);
  REQUIRE(acquisition.status ==
      umbra::detail::AttributeOwnershipAcquisitionStatus::applied);
  REQUIRE(acquisition.workItems.size() == 1U);
  auto const& releaseWork = acquisition.workItems.front();
  REQUIRE(releaseWork.kind ==
      umbra::detail::AttributeOwnershipAcquisitionWorkKind::request_release);
  REQUIRE(releaseWork.requestingFederateId == handoffOwnerId);
  REQUIRE(releaseWork.receivingFederateId == initialOwnerId);
  REQUIRE(releaseWork.attributeHandles == ownershipTargetAttributes);
  auto releaseDelivery = restarted.beginAttributeOwnershipAcquisitionRelease(
      L"exercise", handoffOwnerId, initialOwnerId,
      registered.objectInstanceHandle, releaseWork.requestId,
      releaseWork.attributeHandles);
  REQUIRE(releaseDelivery.has_value());
  REQUIRE(releaseDelivery->candidateAttributeHandles == ownershipTargetAttributes);

  std::vector<unsigned char> const divestitureTag{
      'd', 'i', 'r', 'e', 'c', 't', 'e', 'd', '-', 'h', 'a', 'n', 'd', 'o', 'f', 'f'};
  auto divestiture = restarted.planAttributeOwnershipDivestitureIfWanted(
      L"exercise", initialOwnerId, registered.objectInstanceHandle,
      ownershipTargetAttributes, divestitureTag);
  REQUIRE(divestiture.status ==
      umbra::detail::AttributeOwnershipDivestitureIfWantedStatus::applied);
  REQUIRE(divestiture.divestedAttributeHandles == ownershipTargetAttributes);
  REQUIRE(divestiture.notifications.size() == 1U);
  auto const& notification = divestiture.notifications.front();
  REQUIRE(notification.receivingFederateId == handoffOwnerId);
  REQUIRE(notification.attributeHandles == ownershipTargetAttributes);
  REQUIRE(notification.userSuppliedTag == divestitureTag);
  auto notificationDelivery =
      restarted.beginAttributeOwnershipDivestitureIfWantedNotification(
          L"exercise", handoffOwnerId, registered.objectInstanceHandle,
          notification.notificationId, ownershipTargetAttributes);
  REQUIRE(notificationDelivery.has_value());
  REQUIRE(notificationDelivery->securedAttributeHandles == ownershipTargetAttributes);
  REQUIRE(notificationDelivery->followupWorkItems.empty());

  auto handedOffInitialOwnerState = restarted.attributeOwnedByFederate(
      L"exercise", initialOwnerId, registered.objectInstanceHandle, *marker);
  REQUIRE(handedOffInitialOwnerState.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE_FALSE(handedOffInitialOwnerState.ownedByRequestingFederate);
  auto handedOffOwnerState = restarted.attributeOwnedByFederate(
      L"exercise", handoffOwnerId, registered.objectInstanceHandle, *marker);
  REQUIRE(handedOffOwnerState.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE(handedOffOwnerState.ownedByRequestingFederate);

  auto afterHandoff = restarted.planReceiveOrderDirectedInteraction(
      L"exercise", publisherId, registered.objectInstanceHandle,
      *interactionClass, {});
  REQUIRE(afterHandoff.status ==
      umbra::detail::ReceiveOrderDirectedInteractionStatus::applied);
  REQUIRE(afterHandoff.recipients.size() == 1U);
  REQUIRE(afterHandoff.recipients.front().federateId == handoffOwnerId);
  REQUIRE(afterHandoff.recipients.front().objectInstanceHandle ==
      registered.objectInstanceHandle);
  REQUIRE(afterHandoff.recipients.front().receivedInteractionClassHandle ==
      *interactionClass);

  std::wstring const roundTripLabel =
      L"directed-interaction-ownership-handoff-round-trip";
  REQUIRE(restarted.requestFederationSave(
      L"exercise", publisherId, roundTripLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveBegun(
      L"exercise", publisherId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveBegun(
      L"exercise", initialOwnerId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveBegun(
      L"exercise", handoffOwnerId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(restarted.federateSaveComplete(
      L"exercise", publisherId).saveCompletedSuccessfully);
  REQUIRE_FALSE(restarted.federateSaveComplete(
      L"exercise", initialOwnerId).saveCompletedSuccessfully);
  REQUIRE(restarted.federateSaveComplete(
      L"exercise", handoffOwnerId).saveCompletedSuccessfully);
  auto roundTrip = store->load(L"exercise", roundTripLabel);
  REQUIRE(roundTrip.has_value());
  auto roundTripImage = umbra::detail::FederationStateImageCodec::decode(
      roundTrip->stateImage);
  REQUIRE(roundTripImage.objects.size() == 1U);
  auto const roundTripMarker = std::find_if(
      roundTripImage.objects.front().attributes.begin(),
      roundTripImage.objects.front().attributes.end(),
      [marker](auto const& attribute) { return attribute.handle == *marker; });
  REQUIRE(roundTripMarker != roundTripImage.objects.front().attributes.end());
  REQUIRE(roundTripMarker->ownerFederateId == handoffOwnerId);
  REQUIRE(roundTripImage.interactionDeclarations.size() == 3U);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem fresh-registry directed TSO rechecks target ownership before callback and retracts suppressed delivery",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-directed-interaction-tso-ownership-callback]"
    "[tso-directed-interaction-state][tso-retraction-ledger-state][directed-interaction]"
    "[directed-routing][ownership-ledger-state][object-visibility-state]"
    "[ownership-management][time-management]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedDirectedInteractionDefinition()).status ==
      FederationRegistryStatus::applied);
  auto publisher = source.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto initialOwner = source.join(
      L"exercise", L"owner", L"initial-owner", noOpCallbackRoute());
  auto handoffOwner = source.join(
      L"exercise", L"owner", L"handoff-owner", noOpCallbackRoute());
  REQUIRE(publisher.membership);
  REQUIRE(initialOwner.membership);
  REQUIRE(handoffOwner.membership);
  auto const publisherId = publisher.membership->id;
  auto const initialOwnerId = initialOwner.membership->id;
  auto const handoffOwnerId = handoffOwner.membership->id;

  auto const objectClass = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject");
  auto const interactionClass = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.UmbraDirectedFixtureInteraction");
  auto const marker = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject", "DirectedTargetMarker");
  auto const privilegeToDelete = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject",
      "HLAprivilegeToDeleteObject");
  REQUIRE(objectClass);
  REQUIRE(interactionClass);
  REQUIRE(marker);
  REQUIRE(privilegeToDelete);

  std::set<std::uint64_t> const markerOnly{*marker};
  std::set<std::uint64_t> const ownershipTargetAttributes{
      *marker, *privilegeToDelete};
  std::set<std::uint64_t> const directedOnly{*interactionClass};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", initialOwnerId, *objectClass, markerOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", handoffOwnerId, *objectClass, markerOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", publisherId, *objectClass, markerOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", handoffOwnerId, *objectClass, markerOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.publishObjectClassDirectedInteractions(
      L"exercise", publisherId, *objectClass, directedOnly) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);
  REQUIRE(source.subscribeObjectClassDirectedInteractions(
      L"exercise", initialOwnerId, *objectClass, directedOnly, false) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);
  REQUIRE(source.subscribeObjectClassDirectedInteractions(
      L"exercise", handoffOwnerId, *objectClass, directedOnly, false) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", initialOwnerId, *objectClass);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", publisherId, registered.objectInstanceHandle));
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", handoffOwnerId, registered.objectInstanceHandle));

  std::string const markerValue{"directed-tso-ownership", 22U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *marker,
      rti1516_2025::VariableLengthData(markerValue.data(), markerValue.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise", initialOwnerId, registered.objectInstanceHandle, *objectClass,
      {"HLAreliable"}, &values) == FederationRegistryStatus::applied);

  auto sourcePlan = source.planReceiveOrderDirectedInteraction(
      L"exercise", publisherId, registered.objectInstanceHandle,
      *interactionClass, {});
  REQUIRE(sourcePlan.status ==
      umbra::detail::ReceiveOrderDirectedInteractionStatus::applied);
  REQUIRE(sourcePlan.recipients.size() == 1U);
  REQUIRE(sourcePlan.recipients.front().federateId == initialOwnerId);

  std::string const tagBytes{"ownership-tso", 12U};
  umbra::detail::TsoDirectedInteractionMessage message;
  message.producingFederateId = publisherId;
  message.objectInstanceHandle = registered.objectInstanceHandle;
  message.sentInteractionClassHandle = *interactionClass;
  message.userSuppliedTag = rti1516_2025::VariableLengthData(
      tagBytes.data(), tagBytes.size());
  message.transportationName = "HLAreliable";
  message.timestamp = std::make_shared<rti1516_2025::HLAinteger64Time>(5);
  auto const& selected = sourcePlan.recipients.front();
  message.recipients.push_back({
      selected.federateId,
      selected.objectInstanceHandle,
      selected.receivedInteractionClassHandle,
      selected.receivedParameterHandles,
      selected.callbackRoute});
  auto const enqueued = source.enqueueTsoDirectedInteraction(
      L"exercise", std::move(message), {initialOwnerId});
  REQUIRE(enqueued.status ==
      umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(enqueued.queueStatus ==
      umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(enqueued.enqueuedRecipientCount == 1U);

  auto saveAll = [&](EmbeddedFederationRegistry& registry,
                     std::wstring const& label) {
    REQUIRE(registry.requestFederationSave(
        L"exercise", publisherId, label).status ==
        umbra::detail::FederationSaveControlStatus::applied);
    REQUIRE(registry.federateSaveBegun(
        L"exercise", publisherId).status ==
        umbra::detail::FederationSaveControlStatus::applied);
    REQUIRE(registry.federateSaveBegun(
        L"exercise", initialOwnerId).status ==
        umbra::detail::FederationSaveControlStatus::applied);
    REQUIRE(registry.federateSaveBegun(
        L"exercise", handoffOwnerId).status ==
        umbra::detail::FederationSaveControlStatus::applied);
    REQUIRE_FALSE(registry.federateSaveComplete(
        L"exercise", publisherId).saveCompletedSuccessfully);
    REQUIRE_FALSE(registry.federateSaveComplete(
        L"exercise", initialOwnerId).saveCompletedSuccessfully);
    REQUIRE(registry.federateSaveComplete(
        L"exercise", handoffOwnerId).saveCompletedSuccessfully);
  };

  std::wstring const saveLabel =
      L"directed-interaction-tso-ownership-callback-process-restart";
  saveAll(source, saveLabel);
  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed);
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.tsoDirectedInteractionMessages.size() == 1U);
  REQUIRE(image.tsoDirectedInteractionMessages.front().messageId ==
      enqueued.messageId);
  REQUIRE(image.tsoDirectedInteractionMessages.front().recipients.size() == 1U);
  REQUIRE(image.tsoRequestRetractionRecords.size() == 1U);
  REQUIRE(image.tsoRequestRetractionRecords.front().recipientStates.size() == 1U);
  REQUIRE(image.tsoRequestRetractionRecords.front().recipientStates.front().state == 0U);
  REQUIRE(image.tsoQueueEntries.size() == 1U);
  REQUIRE(image.tsoQueueEntries.front().phase == 0U);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedDirectedInteractionDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedPublisher = restarted.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto restartedInitialOwner = restarted.join(
      L"exercise", L"owner", L"initial-owner", noOpCallbackRoute());
  auto restartedHandoffOwner = restarted.join(
      L"exercise", L"owner", L"handoff-owner", noOpCallbackRoute());
  REQUIRE(restartedPublisher.membership);
  REQUIRE(restartedInitialOwner.membership);
  REQUIRE(restartedHandoffOwner.membership);
  REQUIRE(restartedPublisher.membership->id == publisherId);
  REQUIRE(restartedInitialOwner.membership->id == initialOwnerId);
  REQUIRE(restartedHandoffOwner.membership->id == handoffOwnerId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", publisherId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", publisherId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", initialOwnerId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", handoffOwnerId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  auto const restartedObjectClass = restarted.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject");
  auto const restartedInteractionClass = restarted.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.UmbraDirectedFixtureInteraction");
  auto const restartedMarker = restarted.attributeHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject", "DirectedTargetMarker");
  auto const restartedPrivilege = restarted.attributeHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject",
      "HLAprivilegeToDeleteObject");
  REQUIRE(restartedObjectClass);
  REQUIRE(restartedInteractionClass);
  REQUIRE(restartedMarker);
  REQUIRE(restartedPrivilege);
  REQUIRE(*restartedObjectClass == *objectClass);
  REQUIRE(*restartedInteractionClass == *interactionClass);
  REQUIRE(*restartedMarker == *marker);
  REQUIRE(*restartedPrivilege == *privilegeToDelete);

  auto restoredBeforeHandoff = restarted.planReceiveOrderDirectedInteraction(
      L"exercise", publisherId, registered.objectInstanceHandle,
      *restartedInteractionClass, {});
  REQUIRE(restoredBeforeHandoff.status ==
      umbra::detail::ReceiveOrderDirectedInteractionStatus::applied);
  REQUIRE(restoredBeforeHandoff.recipients.size() == 1U);
  REQUIRE(restoredBeforeHandoff.recipients.front().federateId == initialOwnerId);

  std::vector<unsigned char> const acquisitionTag{
      't', 's', 'o', '-', 'o', 'w', 'n', 'e', 'r'};
  auto acquisition = restarted.planAttributeOwnershipAcquisition(
      L"exercise", handoffOwnerId, registered.objectInstanceHandle,
      ownershipTargetAttributes, acquisitionTag);
  REQUIRE(acquisition.status ==
      umbra::detail::AttributeOwnershipAcquisitionStatus::applied);
  REQUIRE(acquisition.workItems.size() == 1U);
  auto const& releaseWork = acquisition.workItems.front();
  REQUIRE(releaseWork.kind ==
      umbra::detail::AttributeOwnershipAcquisitionWorkKind::request_release);
  auto releaseDelivery = restarted.beginAttributeOwnershipAcquisitionRelease(
      L"exercise", handoffOwnerId, initialOwnerId,
      registered.objectInstanceHandle, releaseWork.requestId,
      releaseWork.attributeHandles);
  REQUIRE(releaseDelivery);
  auto divestiture = restarted.planAttributeOwnershipDivestitureIfWanted(
      L"exercise", initialOwnerId, registered.objectInstanceHandle,
      ownershipTargetAttributes, acquisitionTag);
  REQUIRE(divestiture.status ==
      umbra::detail::AttributeOwnershipDivestitureIfWantedStatus::applied);
  REQUIRE(divestiture.notifications.size() == 1U);
  auto const& notification = divestiture.notifications.front();
  auto notificationDelivery =
      restarted.beginAttributeOwnershipDivestitureIfWantedNotification(
          L"exercise", handoffOwnerId, registered.objectInstanceHandle,
          notification.notificationId, ownershipTargetAttributes);
  REQUIRE(notificationDelivery);

  auto handedOffRoute = restarted.planReceiveOrderDirectedInteraction(
      L"exercise", publisherId, registered.objectInstanceHandle,
      *restartedInteractionClass, {});
  REQUIRE(handedOffRoute.status ==
      umbra::detail::ReceiveOrderDirectedInteractionStatus::applied);
  REQUIRE(handedOffRoute.recipients.size() == 1U);
  REQUIRE(handedOffRoute.recipients.front().federateId == handoffOwnerId);

  auto delivery = restarted.beginTsoPayloadDelivery(
      L"exercise", initialOwnerId,
      rti1516_2025::HLAinteger64Time(5), true);
  REQUIRE(delivery.status ==
      umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(delivery.deliveryStatus ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(delivery.deliveries.size() == 1U);
  auto const* directed = std::get_if<umbra::detail::TsoDirectedInteractionDelivery>(
      &delivery.deliveries.front());
  REQUIRE(directed);
  REQUIRE(directed->message.messageId == enqueued.messageId);
  REQUIRE_FALSE(restarted.timestampedDirectedInteractionRecipientFor(
      L"exercise", publisherId, initialOwnerId,
      registered.objectInstanceHandle, *restartedInteractionClass, {},
      enqueued.messageId));
  REQUIRE(restarted.finishTsoRecipientCallbackSuppressed(
      L"exercise", initialOwnerId, enqueued.messageId));
  REQUIRE_FALSE(restarted.beginTsoInteractionCallback(
      L"exercise", initialOwnerId, enqueued.messageId));
  REQUIRE(restarted.completeTsoDelivery(
      L"exercise", directed->queuedMessage).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);

  auto const retracted = restarted.retractTsoMessageForProducer(
      L"exercise", publisherId, enqueued.messageId,
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  REQUIRE(retracted.status ==
      umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(retracted.queueResult.status ==
      umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(retracted.requestRetractionNotifications.empty());
  REQUIRE_FALSE(restarted.canDeliverTsoRequestRetraction(
      L"exercise", initialOwnerId, enqueued.messageId));

  std::wstring const terminalSaveLabel =
      L"directed-interaction-tso-ownership-callback-terminal";
  saveAll(restarted, terminalSaveLabel);
  auto terminal = store->load(L"exercise", terminalSaveLabel);
  REQUIRE(terminal);
  auto terminalImage = umbra::detail::FederationStateImageCodec::decode(
      terminal->stateImage);
  REQUIRE(terminalImage.tsoDirectedInteractionMessages.empty());
  REQUIRE(terminalImage.tsoRequestRetractionRecords.size() == 1U);
  REQUIRE(terminalImage.tsoRequestRetractionRecords.front().retractionApplied);
  REQUIRE(terminalImage.tsoRequestRetractionRecords.front().terminal);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem fresh-registry directed TSO delivers an eligible by-ownership recipient and issues Request Retraction",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-directed-interaction-tso-ownership-delivery-retraction]"
    "[tso-directed-interaction-state][tso-retraction-ledger-state][directed-interaction]"
    "[directed-routing][ownership-ledger-state][object-visibility-state]"
    "[ownership-management][time-management]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedDirectedInteractionDefinition()).status ==
      FederationRegistryStatus::applied);
  auto publisher = source.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto owner = source.join(
      L"exercise", L"owner", L"owner", noOpCallbackRoute());
  REQUIRE(publisher.membership);
  REQUIRE(owner.membership);
  auto const publisherId = publisher.membership->id;
  auto const ownerId = owner.membership->id;

  auto const objectClass = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject");
  auto const interactionClass = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.UmbraDirectedFixtureInteraction");
  auto const marker = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject", "DirectedTargetMarker");
  REQUIRE(objectClass);
  REQUIRE(interactionClass);
  REQUIRE(marker);

  std::set<std::uint64_t> const markerOnly{*marker};
  std::set<std::uint64_t> const directedOnly{*interactionClass};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", ownerId, *objectClass, markerOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", publisherId, *objectClass, markerOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.publishObjectClassDirectedInteractions(
      L"exercise", publisherId, *objectClass, directedOnly) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);
  REQUIRE(source.subscribeObjectClassDirectedInteractions(
      L"exercise", ownerId, *objectClass, directedOnly, false) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", ownerId, *objectClass);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", publisherId, registered.objectInstanceHandle));

  std::string const markerValue{"eligible-directed-tso", 21U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *marker,
      rti1516_2025::VariableLengthData(markerValue.data(), markerValue.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise", ownerId, registered.objectInstanceHandle, *objectClass,
      {"HLAreliable"}, &values) == FederationRegistryStatus::applied);

  auto sourcePlan = source.planReceiveOrderDirectedInteraction(
      L"exercise", publisherId, registered.objectInstanceHandle,
      *interactionClass, {});
  REQUIRE(sourcePlan.status ==
      umbra::detail::ReceiveOrderDirectedInteractionStatus::applied);
  REQUIRE(sourcePlan.recipients.size() == 1U);
  REQUIRE(sourcePlan.recipients.front().federateId == ownerId);

  std::string const tagBytes{"eligible-tso", 12U};
  umbra::detail::TsoDirectedInteractionMessage message;
  message.producingFederateId = publisherId;
  message.objectInstanceHandle = registered.objectInstanceHandle;
  message.sentInteractionClassHandle = *interactionClass;
  message.userSuppliedTag = rti1516_2025::VariableLengthData(
      tagBytes.data(), tagBytes.size());
  message.transportationName = "HLAreliable";
  message.timestamp = std::make_shared<rti1516_2025::HLAinteger64Time>(5);
  auto const& selected = sourcePlan.recipients.front();
  message.recipients.push_back({
      selected.federateId,
      selected.objectInstanceHandle,
      selected.receivedInteractionClassHandle,
      selected.receivedParameterHandles,
      selected.callbackRoute});
  auto const enqueued = source.enqueueTsoDirectedInteraction(
      L"exercise", std::move(message), {ownerId});
  REQUIRE(enqueued.status ==
      umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(enqueued.queueStatus ==
      umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(enqueued.enqueuedRecipientCount == 1U);

  auto saveAll = [&](EmbeddedFederationRegistry& registry,
                     std::wstring const& label) {
    REQUIRE(registry.requestFederationSave(
        L"exercise", publisherId, label).status ==
        umbra::detail::FederationSaveControlStatus::applied);
    REQUIRE(registry.federateSaveBegun(
        L"exercise", publisherId).status ==
        umbra::detail::FederationSaveControlStatus::applied);
    REQUIRE(registry.federateSaveBegun(
        L"exercise", ownerId).status ==
        umbra::detail::FederationSaveControlStatus::applied);
    REQUIRE_FALSE(registry.federateSaveComplete(
        L"exercise", publisherId).saveCompletedSuccessfully);
    REQUIRE(registry.federateSaveComplete(
        L"exercise", ownerId).saveCompletedSuccessfully);
  };

  std::wstring const saveLabel =
      L"directed-interaction-tso-ownership-delivery-retraction-process-restart";
  saveAll(source, saveLabel);
  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed);
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.tsoDirectedInteractionMessages.size() == 1U);
  REQUIRE(image.tsoDirectedInteractionMessages.front().messageId ==
      enqueued.messageId);
  REQUIRE(image.tsoDirectedInteractionMessages.front().recipients.size() == 1U);
  REQUIRE(image.tsoRequestRetractionRecords.size() == 1U);
  REQUIRE(image.tsoRequestRetractionRecords.front().recipientStates.size() == 1U);
  REQUIRE(image.tsoRequestRetractionRecords.front().recipientStates.front().state == 0U);
  REQUIRE(image.tsoQueueEntries.size() == 1U);
  REQUIRE(image.tsoQueueEntries.front().messageId == enqueued.messageId);
  REQUIRE(image.tsoQueueEntries.front().recipientFederateId == ownerId);
  REQUIRE(image.tsoQueueEntries.front().phase == 0U);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedDirectedInteractionDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedPublisher = restarted.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto restartedOwner = restarted.join(
      L"exercise", L"owner", L"owner", noOpCallbackRoute());
  REQUIRE(restartedPublisher.membership);
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedPublisher.membership->id == publisherId);
  REQUIRE(restartedOwner.membership->id == ownerId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", publisherId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", publisherId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", ownerId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  auto const restartedObjectClass = restarted.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject");
  auto const restartedInteractionClass = restarted.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.UmbraDirectedFixtureInteraction");
  REQUIRE(restartedObjectClass);
  REQUIRE(restartedInteractionClass);
  REQUIRE(*restartedObjectClass == *objectClass);
  REQUIRE(*restartedInteractionClass == *interactionClass);

  auto restoredRoute = restarted.planReceiveOrderDirectedInteraction(
      L"exercise", publisherId, registered.objectInstanceHandle,
      *restartedInteractionClass, {});
  REQUIRE(restoredRoute.status ==
      umbra::detail::ReceiveOrderDirectedInteractionStatus::applied);
  REQUIRE(restoredRoute.recipients.size() == 1U);
  REQUIRE(restoredRoute.recipients.front().federateId == ownerId);
  auto restoredRecipient = restarted.timestampedDirectedInteractionRecipientFor(
      L"exercise", publisherId, ownerId, registered.objectInstanceHandle,
      *restartedInteractionClass, {}, enqueued.messageId);
  REQUIRE(restoredRecipient);
  REQUIRE(restoredRecipient->federateId == ownerId);

  auto delivery = restarted.beginTsoPayloadDelivery(
      L"exercise", ownerId, rti1516_2025::HLAinteger64Time(5), true);
  REQUIRE(delivery.status ==
      umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(delivery.deliveryStatus ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(delivery.deliveries.size() == 1U);
  auto const* directed = std::get_if<umbra::detail::TsoDirectedInteractionDelivery>(
      &delivery.deliveries.front());
  REQUIRE(directed);
  REQUIRE(directed->message.messageId == enqueued.messageId);
  REQUIRE(restarted.beginTsoInteractionCallback(
      L"exercise", ownerId, enqueued.messageId));
  REQUIRE(restarted.completeTsoDelivery(
      L"exercise", directed->queuedMessage).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);

  auto const retracted = restarted.retractTsoMessageForProducer(
      L"exercise", publisherId, enqueued.messageId,
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  REQUIRE(retracted.status ==
      umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(retracted.timestampEligible);
  REQUIRE(retracted.queueResult.status ==
      umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(retracted.requestRetractionNotifications.size() == 1U);
  REQUIRE(retracted.requestRetractionNotifications.front().receivingFederateId == ownerId);
  REQUIRE(retracted.requestRetractionNotifications.front().messageId == enqueued.messageId);
  REQUIRE(restarted.canDeliverTsoRequestRetraction(
      L"exercise", ownerId, enqueued.messageId));

  std::wstring const terminalSaveLabel =
      L"directed-interaction-tso-ownership-delivery-retraction-terminal";
  saveAll(restarted, terminalSaveLabel);
  auto terminal = store->load(L"exercise", terminalSaveLabel);
  REQUIRE(terminal);
  auto terminalImage = umbra::detail::FederationStateImageCodec::decode(
      terminal->stateImage);
  REQUIRE(terminalImage.tsoDirectedInteractionMessages.empty());
  REQUIRE(terminalImage.tsoRequestRetractionRecords.size() == 1U);
  REQUIRE(terminalImage.tsoRequestRetractionRecords.front().retractionApplied);
  REQUIRE(terminalImage.tsoRequestRetractionRecords.front().terminal);
  REQUIRE(terminalImage.tsoRequestRetractionRecords.front().recipientStates.front().state == 3U);
  REQUIRE(terminalImage.tsoQueueEntries.size() == 1U);
  REQUIRE(terminalImage.tsoQueueEntries.front().messageId == enqueued.messageId);
  REQUIRE(terminalImage.tsoQueueEntries.front().recipientFederateId == ownerId);
  REQUIRE(terminalImage.tsoQueueEntries.front().phase == 2U);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem fresh-registry directed TSO fan-out preserves an eligible recipient and suppresses an unsubscribed recipient",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-directed-interaction-tso-fanout-positive-negative]"
    "[tso-directed-interaction-state][tso-retraction-ledger-state][directed-interaction]"
    "[directed-routing][object-visibility-state][time-management]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedDirectedInteractionDefinition()).status ==
      FederationRegistryStatus::applied);
  auto publisher = source.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto eligible = source.join(
      L"exercise", L"owner", L"eligible-owner", noOpCallbackRoute());
  auto unsubscribed = source.join(
      L"exercise", L"observer", L"unsubscribed-observer", noOpCallbackRoute());
  REQUIRE(publisher.membership);
  REQUIRE(eligible.membership);
  REQUIRE(unsubscribed.membership);
  auto const publisherId = publisher.membership->id;
  auto const eligibleId = eligible.membership->id;
  auto const unsubscribedId = unsubscribed.membership->id;

  auto const objectClass = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject");
  auto const interactionClass = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.UmbraDirectedFixtureInteraction");
  auto const marker = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject", "DirectedTargetMarker");
  REQUIRE(objectClass);
  REQUIRE(interactionClass);
  REQUIRE(marker);

  std::set<std::uint64_t> const markerOnly{*marker};
  std::set<std::uint64_t> const directedOnly{*interactionClass};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", eligibleId, *objectClass, markerOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", publisherId, *objectClass, markerOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", unsubscribedId, *objectClass, markerOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.publishObjectClassDirectedInteractions(
      L"exercise", publisherId, *objectClass, directedOnly) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);
  REQUIRE(source.subscribeObjectClassDirectedInteractions(
      L"exercise", eligibleId, *objectClass, directedOnly, true) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);
  REQUIRE(source.subscribeObjectClassDirectedInteractions(
      L"exercise", unsubscribedId, *objectClass, directedOnly, true) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", eligibleId, *objectClass);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 2U);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", publisherId, registered.objectInstanceHandle));
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", unsubscribedId, registered.objectInstanceHandle));

  std::string const markerValue{"directed-tso-fanout", 19U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *marker,
      rti1516_2025::VariableLengthData(markerValue.data(), markerValue.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise", eligibleId, registered.objectInstanceHandle, *objectClass,
      {"HLAreliable"}, &values) == FederationRegistryStatus::applied);

  auto sourcePlan = source.planReceiveOrderDirectedInteraction(
      L"exercise", publisherId, registered.objectInstanceHandle,
      *interactionClass, {});
  REQUIRE(sourcePlan.status ==
      umbra::detail::ReceiveOrderDirectedInteractionStatus::applied);
  REQUIRE(sourcePlan.recipients.size() == 2U);
  auto const eligibleSelection = std::find_if(
      sourcePlan.recipients.begin(), sourcePlan.recipients.end(),
      [eligibleId](auto const& recipient) {
        return recipient.federateId == eligibleId;
      });
  auto const unsubscribedSelection = std::find_if(
      sourcePlan.recipients.begin(), sourcePlan.recipients.end(),
      [unsubscribedId](auto const& recipient) {
        return recipient.federateId == unsubscribedId;
      });
  REQUIRE(eligibleSelection != sourcePlan.recipients.end());
  REQUIRE(unsubscribedSelection != sourcePlan.recipients.end());

  std::string const tagBytes{"fanout-tso", 10U};
  umbra::detail::TsoDirectedInteractionMessage message;
  message.producingFederateId = publisherId;
  message.objectInstanceHandle = registered.objectInstanceHandle;
  message.sentInteractionClassHandle = *interactionClass;
  message.userSuppliedTag = rti1516_2025::VariableLengthData(
      tagBytes.data(), tagBytes.size());
  message.transportationName = "HLAreliable";
  message.timestamp = std::make_shared<rti1516_2025::HLAinteger64Time>(5);
  message.recipients.push_back({
      eligibleSelection->federateId,
      eligibleSelection->objectInstanceHandle,
      eligibleSelection->receivedInteractionClassHandle,
      eligibleSelection->receivedParameterHandles,
      eligibleSelection->callbackRoute});
  message.recipients.push_back({
      unsubscribedSelection->federateId,
      unsubscribedSelection->objectInstanceHandle,
      unsubscribedSelection->receivedInteractionClassHandle,
      unsubscribedSelection->receivedParameterHandles,
      unsubscribedSelection->callbackRoute});
  auto const enqueued = source.enqueueTsoDirectedInteraction(
      L"exercise", std::move(message), {eligibleId, unsubscribedId});
  REQUIRE(enqueued.status ==
      umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(enqueued.queueStatus ==
      umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(enqueued.enqueuedRecipientCount == 2U);

  auto saveAll = [&](EmbeddedFederationRegistry& registry,
                     std::wstring const& label) {
    REQUIRE(registry.requestFederationSave(
        L"exercise", publisherId, label).status ==
        umbra::detail::FederationSaveControlStatus::applied);
    REQUIRE(registry.federateSaveBegun(
        L"exercise", publisherId).status ==
        umbra::detail::FederationSaveControlStatus::applied);
    REQUIRE(registry.federateSaveBegun(
        L"exercise", eligibleId).status ==
        umbra::detail::FederationSaveControlStatus::applied);
    REQUIRE(registry.federateSaveBegun(
        L"exercise", unsubscribedId).status ==
        umbra::detail::FederationSaveControlStatus::applied);
    REQUIRE_FALSE(registry.federateSaveComplete(
        L"exercise", publisherId).saveCompletedSuccessfully);
    REQUIRE_FALSE(registry.federateSaveComplete(
        L"exercise", eligibleId).saveCompletedSuccessfully);
    REQUIRE(registry.federateSaveComplete(
        L"exercise", unsubscribedId).saveCompletedSuccessfully);
  };

  std::wstring const saveLabel =
      L"directed-interaction-tso-fanout-process-restart";
  saveAll(source, saveLabel);
  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed);
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.tsoDirectedInteractionMessages.size() == 1U);
  REQUIRE(image.tsoDirectedInteractionMessages.front().messageId ==
      enqueued.messageId);
  REQUIRE(image.tsoDirectedInteractionMessages.front().recipients.size() == 2U);
  std::set<std::uint64_t> savedRecipients;
  for (auto const& recipient : image.tsoDirectedInteractionMessages.front().recipients) {
    savedRecipients.insert(recipient.receivingFederateId);
  }
  REQUIRE(savedRecipients == std::set<std::uint64_t>{eligibleId, unsubscribedId});
  REQUIRE(image.tsoRequestRetractionRecords.size() == 1U);
  REQUIRE(image.tsoRequestRetractionRecords.front().recipientStates.size() == 2U);
  for (auto const& recipient : image.tsoRequestRetractionRecords.front().recipientStates) {
    REQUIRE(recipient.state == 0U);
  }
  REQUIRE(image.tsoQueueEntries.size() == 2U);
  for (auto const& queueEntry : image.tsoQueueEntries) {
    REQUIRE(queueEntry.messageId == enqueued.messageId);
    REQUIRE(queueEntry.phase == 0U);
  }

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedDirectedInteractionDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedPublisher = restarted.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto restartedEligible = restarted.join(
      L"exercise", L"owner", L"eligible-owner", noOpCallbackRoute());
  auto restartedUnsubscribed = restarted.join(
      L"exercise", L"observer", L"unsubscribed-observer", noOpCallbackRoute());
  REQUIRE(restartedPublisher.membership);
  REQUIRE(restartedEligible.membership);
  REQUIRE(restartedUnsubscribed.membership);
  REQUIRE(restartedPublisher.membership->id == publisherId);
  REQUIRE(restartedEligible.membership->id == eligibleId);
  REQUIRE(restartedUnsubscribed.membership->id == unsubscribedId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", publisherId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", publisherId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", eligibleId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", unsubscribedId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  auto const restartedObjectClass = restarted.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject");
  auto const restartedInteractionClass = restarted.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.UmbraDirectedFixtureInteraction");
  auto const restartedMarker = restarted.attributeHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject", "DirectedTargetMarker");
  REQUIRE(restartedObjectClass);
  REQUIRE(restartedInteractionClass);
  REQUIRE(restartedMarker);
  REQUIRE(*restartedObjectClass == *objectClass);
  REQUIRE(*restartedInteractionClass == *interactionClass);
  REQUIRE(*restartedMarker == *marker);

  auto restoredRoute = restarted.planReceiveOrderDirectedInteraction(
      L"exercise", publisherId, registered.objectInstanceHandle,
      *restartedInteractionClass, {});
  REQUIRE(restoredRoute.status ==
      umbra::detail::ReceiveOrderDirectedInteractionStatus::applied);
  REQUIRE(restoredRoute.recipients.size() == 2U);
  REQUIRE(restarted.timestampedDirectedInteractionRecipientFor(
      L"exercise", publisherId, eligibleId, registered.objectInstanceHandle,
      *restartedInteractionClass, {}, enqueued.messageId));
  REQUIRE(restarted.timestampedDirectedInteractionRecipientFor(
      L"exercise", publisherId, unsubscribedId, registered.objectInstanceHandle,
      *restartedInteractionClass, {}, enqueued.messageId));

  REQUIRE(restarted.unsubscribeObjectClassDirectedInteractions(
      L"exercise", unsubscribedId, *restartedObjectClass,
      std::optional<std::set<std::uint64_t>>{directedOnly}) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);
  auto routeAfterUnsubscribe = restarted.planReceiveOrderDirectedInteraction(
      L"exercise", publisherId, registered.objectInstanceHandle,
      *restartedInteractionClass, {});
  REQUIRE(routeAfterUnsubscribe.status ==
      umbra::detail::ReceiveOrderDirectedInteractionStatus::applied);
  REQUIRE(routeAfterUnsubscribe.recipients.size() == 1U);
  REQUIRE(routeAfterUnsubscribe.recipients.front().federateId == eligibleId);
  REQUIRE(restarted.timestampedDirectedInteractionRecipientFor(
      L"exercise", publisherId, eligibleId, registered.objectInstanceHandle,
      *restartedInteractionClass, {}, enqueued.messageId));
  REQUIRE_FALSE(restarted.timestampedDirectedInteractionRecipientFor(
      L"exercise", publisherId, unsubscribedId, registered.objectInstanceHandle,
      *restartedInteractionClass, {}, enqueued.messageId));

  auto eligibleDelivery = restarted.beginTsoPayloadDelivery(
      L"exercise", eligibleId, rti1516_2025::HLAinteger64Time(5), true);
  REQUIRE(eligibleDelivery.status ==
      umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(eligibleDelivery.deliveryStatus ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(eligibleDelivery.deliveries.size() == 1U);
  auto const* eligibleDirected = std::get_if<umbra::detail::TsoDirectedInteractionDelivery>(
      &eligibleDelivery.deliveries.front());
  REQUIRE(eligibleDirected);
  REQUIRE(eligibleDirected->message.messageId == enqueued.messageId);
  REQUIRE(restarted.beginTsoInteractionCallback(
      L"exercise", eligibleId, enqueued.messageId));
  REQUIRE(restarted.completeTsoDelivery(
      L"exercise", eligibleDirected->queuedMessage).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);

  auto unsubscribedDelivery = restarted.beginTsoPayloadDelivery(
      L"exercise", unsubscribedId, rti1516_2025::HLAinteger64Time(5), true);
  REQUIRE(unsubscribedDelivery.status ==
      umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(unsubscribedDelivery.deliveryStatus ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(unsubscribedDelivery.deliveries.size() == 1U);
  auto const* unsubscribedDirected = std::get_if<umbra::detail::TsoDirectedInteractionDelivery>(
      &unsubscribedDelivery.deliveries.front());
  REQUIRE(unsubscribedDirected);
  REQUIRE(unsubscribedDirected->message.messageId == enqueued.messageId);
  REQUIRE(restarted.finishTsoRecipientCallbackSuppressed(
      L"exercise", unsubscribedId, enqueued.messageId));
  REQUIRE_FALSE(restarted.beginTsoInteractionCallback(
      L"exercise", unsubscribedId, enqueued.messageId));
  REQUIRE(restarted.completeTsoDelivery(
      L"exercise", unsubscribedDirected->queuedMessage).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);

  auto const retracted = restarted.retractTsoMessageForProducer(
      L"exercise", publisherId, enqueued.messageId,
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  REQUIRE(retracted.status ==
      umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(retracted.timestampEligible);
  REQUIRE(retracted.queueResult.status ==
      umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(retracted.requestRetractionNotifications.size() == 1U);
  REQUIRE(retracted.requestRetractionNotifications.front().receivingFederateId ==
      eligibleId);
  REQUIRE(retracted.requestRetractionNotifications.front().messageId ==
      enqueued.messageId);
  REQUIRE(restarted.canDeliverTsoRequestRetraction(
      L"exercise", eligibleId, enqueued.messageId));
  REQUIRE_FALSE(restarted.canDeliverTsoRequestRetraction(
      L"exercise", unsubscribedId, enqueued.messageId));

  std::wstring const terminalSaveLabel =
      L"directed-interaction-tso-fanout-terminal";
  saveAll(restarted, terminalSaveLabel);
  auto terminal = store->load(L"exercise", terminalSaveLabel);
  REQUIRE(terminal);
  auto terminalImage = umbra::detail::FederationStateImageCodec::decode(
      terminal->stateImage);
  REQUIRE(terminalImage.tsoDirectedInteractionMessages.empty());
  REQUIRE(terminalImage.tsoRequestRetractionRecords.size() == 1U);
  auto const& terminalRecord = terminalImage.tsoRequestRetractionRecords.front();
  REQUIRE(terminalRecord.retractionApplied);
  REQUIRE(terminalRecord.terminal);
  REQUIRE(terminalRecord.recipientStates.size() == 2U);
  auto const eligibleState = std::find_if(
      terminalRecord.recipientStates.begin(), terminalRecord.recipientStates.end(),
      [eligibleId](auto const& recipient) {
        return recipient.receivingFederateId == eligibleId;
      });
  auto const unsubscribedState = std::find_if(
      terminalRecord.recipientStates.begin(), terminalRecord.recipientStates.end(),
      [unsubscribedId](auto const& recipient) {
        return recipient.receivingFederateId == unsubscribedId;
      });
  REQUIRE(eligibleState != terminalRecord.recipientStates.end());
  REQUIRE(unsubscribedState != terminalRecord.recipientStates.end());
  REQUIRE(eligibleState->state == 3U);
  REQUIRE(unsubscribedState->state == 2U);
  REQUIRE(terminalImage.tsoQueueEntries.size() == 2U);
  for (auto const& queueEntry : terminalImage.tsoQueueEntries) {
    REQUIRE(queueEntry.messageId == enqueued.messageId);
    REQUIRE(queueEntry.phase == 2U);
  }

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem fresh-registry parameterized directed TSO preserves parameter projection and timestamped order metadata",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-directed-interaction-tso-parameter-projection]"
    "[tso-directed-interaction-state][tso-payload-state][directed-interaction]"
    "[directed-routing][object-visibility-state][time-management]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(
      L"exercise", composedParameterizedDirectedInteractionDefinition()).status ==
      FederationRegistryStatus::applied);
  auto publisher = source.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto owner = source.join(
      L"exercise", L"owner", L"owner", noOpCallbackRoute());
  REQUIRE(publisher.membership);
  REQUIRE(owner.membership);
  auto const publisherId = publisher.membership->id;
  auto const ownerId = owner.membership->id;

  auto const objectClass = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedParameterFixtureObject");
  auto const interactionClass = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.UmbraDirectedParameterFixtureInteraction");
  auto const marker = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedParameterFixtureObject",
      "DirectedTargetMarker");
  auto const payload = source.parameterHandleFor(
      L"exercise", "HLAinteractionRoot.UmbraDirectedParameterFixtureInteraction",
      "DirectedPayload");
  REQUIRE(objectClass);
  REQUIRE(interactionClass);
  REQUIRE(marker);
  REQUIRE(payload);
  REQUIRE(source.parameterNameFor(
      L"exercise", "HLAinteractionRoot.UmbraDirectedParameterFixtureInteraction",
      *payload) == std::optional<std::string>{"DirectedPayload"});

  std::set<std::uint64_t> const markerOnly{*marker};
  std::set<std::uint64_t> const directedOnly{*interactionClass};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", ownerId, *objectClass, markerOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", publisherId, *objectClass, markerOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.publishObjectClassDirectedInteractions(
      L"exercise", publisherId, *objectClass, directedOnly) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);
  REQUIRE(source.subscribeObjectClassDirectedInteractions(
      L"exercise", ownerId, *objectClass, directedOnly, false) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", ownerId, *objectClass);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", publisherId, registered.objectInstanceHandle));

  std::string const markerValue{"parameterized-directed", 22U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *marker,
      rti1516_2025::VariableLengthData(markerValue.data(), markerValue.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise", ownerId, registered.objectInstanceHandle, *objectClass,
      {"HLAreliable"}, &values) == FederationRegistryStatus::applied);

  auto sourcePlan = source.planReceiveOrderDirectedInteraction(
      L"exercise", publisherId, registered.objectInstanceHandle,
      *interactionClass, {*payload});
  REQUIRE(sourcePlan.status ==
      umbra::detail::ReceiveOrderDirectedInteractionStatus::applied);
  REQUIRE(sourcePlan.transportationName == "HLAreliable");
  REQUIRE(sourcePlan.preferredOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(sourcePlan.recipients.size() == 1U);
  REQUIRE(sourcePlan.recipients.front().federateId == ownerId);
  REQUIRE(sourcePlan.recipients.front().receivedParameterHandles ==
      std::set<std::uint64_t>{*payload});

  std::string const payloadBytes{"parameterized-directed", 22U};
  std::string const tagBytes{"parameter-tso", 13U};
  umbra::detail::TsoDirectedInteractionMessage message;
  message.producingFederateId = publisherId;
  message.objectInstanceHandle = registered.objectInstanceHandle;
  message.sentInteractionClassHandle = *interactionClass;
  message.sentParameterHandles = {*payload};
  message.parameters = {{
      *payload,
      rti1516_2025::VariableLengthData(payloadBytes.data(), payloadBytes.size()),
  }};
  message.userSuppliedTag = rti1516_2025::VariableLengthData(
      tagBytes.data(), tagBytes.size());
  message.transportationName = sourcePlan.transportationName;
  message.sentOrderType = rti1516_2025::TIMESTAMP;
  message.receivedOrderType = rti1516_2025::TIMESTAMP;
  auto const& selected = sourcePlan.recipients.front();
  message.recipients.push_back({
      selected.federateId,
      selected.objectInstanceHandle,
      selected.receivedInteractionClassHandle,
      selected.receivedParameterHandles,
      selected.callbackRoute});
  message.timestamp = std::make_shared<rti1516_2025::HLAinteger64Time>(7);
  auto const enqueued = source.enqueueTsoDirectedInteraction(
      L"exercise", std::move(message), {ownerId});
  REQUIRE(enqueued.status ==
      umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(enqueued.queueStatus ==
      umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(enqueued.enqueuedRecipientCount == 1U);

  auto saveAll = [&](EmbeddedFederationRegistry& registry,
                     std::wstring const& label) {
    REQUIRE(registry.requestFederationSave(
        L"exercise", publisherId, label).status ==
        umbra::detail::FederationSaveControlStatus::applied);
    REQUIRE(registry.federateSaveBegun(
        L"exercise", publisherId).status ==
        umbra::detail::FederationSaveControlStatus::applied);
    REQUIRE(registry.federateSaveBegun(
        L"exercise", ownerId).status ==
        umbra::detail::FederationSaveControlStatus::applied);
    REQUIRE_FALSE(registry.federateSaveComplete(
        L"exercise", publisherId).saveCompletedSuccessfully);
    REQUIRE(registry.federateSaveComplete(
        L"exercise", ownerId).saveCompletedSuccessfully);
  };

  std::wstring const saveLabel =
      L"directed-interaction-tso-parameter-projection-process-restart";
  saveAll(source, saveLabel);
  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed);
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.tsoDirectedInteractionMessages.size() == 1U);
  auto const& savedMessage = image.tsoDirectedInteractionMessages.front();
  REQUIRE(savedMessage.messageId == enqueued.messageId);
  REQUIRE(savedMessage.sentParameterHandles == std::vector<std::uint64_t>{*payload});
  REQUIRE(savedMessage.parameters.size() == 1U);
  REQUIRE(savedMessage.parameters.front().parameterHandle == *payload);
  REQUIRE(savedMessage.parameters.front().value == payloadBytes);
  REQUIRE(savedMessage.userSuppliedTag == tagBytes);
  REQUIRE(savedMessage.transportationName == "HLAreliable");
  REQUIRE(savedMessage.sentOrderType ==
      static_cast<std::uint32_t>(rti1516_2025::TIMESTAMP));
  REQUIRE(savedMessage.receivedOrderType ==
      static_cast<std::uint32_t>(rti1516_2025::TIMESTAMP));
  REQUIRE(savedMessage.recipients.size() == 1U);
  REQUIRE(savedMessage.recipients.front().receivingFederateId == ownerId);
  REQUIRE(savedMessage.recipients.front().receivedParameterHandles ==
      std::vector<std::uint64_t>{*payload});
  REQUIRE(image.tsoRequestRetractionRecords.size() == 1U);
  REQUIRE(image.tsoQueueEntries.size() == 1U);
  REQUIRE(image.tsoQueueEntries.front().recipientFederateId == ownerId);
  REQUIRE(image.tsoQueueEntries.front().phase == 0U);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(
      L"exercise", composedParameterizedDirectedInteractionDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedPublisher = restarted.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto restartedOwner = restarted.join(
      L"exercise", L"owner", L"owner", noOpCallbackRoute());
  REQUIRE(restartedPublisher.membership);
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedPublisher.membership->id == publisherId);
  REQUIRE(restartedOwner.membership->id == ownerId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", publisherId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", publisherId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", ownerId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  auto const restartedObjectClass = restarted.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedParameterFixtureObject");
  auto const restartedInteractionClass = restarted.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.UmbraDirectedParameterFixtureInteraction");
  auto const restartedPayload = restarted.parameterHandleFor(
      L"exercise", "HLAinteractionRoot.UmbraDirectedParameterFixtureInteraction",
      "DirectedPayload");
  REQUIRE(restartedObjectClass);
  REQUIRE(restartedInteractionClass);
  REQUIRE(restartedPayload);
  REQUIRE(*restartedObjectClass == *objectClass);
  REQUIRE(*restartedInteractionClass == *interactionClass);
  REQUIRE(*restartedPayload == *payload);

  auto restoredRoute = restarted.planReceiveOrderDirectedInteraction(
      L"exercise", publisherId, registered.objectInstanceHandle,
      *restartedInteractionClass, {*restartedPayload});
  REQUIRE(restoredRoute.status ==
      umbra::detail::ReceiveOrderDirectedInteractionStatus::applied);
  REQUIRE(restoredRoute.transportationName == "HLAreliable");
  REQUIRE(restoredRoute.preferredOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(restoredRoute.recipients.size() == 1U);
  REQUIRE(restoredRoute.recipients.front().federateId == ownerId);
  REQUIRE(restoredRoute.recipients.front().receivedParameterHandles ==
      std::set<std::uint64_t>{*restartedPayload});
  auto restoredRecipient = restarted.timestampedDirectedInteractionRecipientFor(
      L"exercise", publisherId, ownerId, registered.objectInstanceHandle,
      *restartedInteractionClass, {*restartedPayload}, enqueued.messageId);
  REQUIRE(restoredRecipient);
  REQUIRE(restoredRecipient->receivedParameterHandles ==
      std::set<std::uint64_t>{*restartedPayload});

  auto delivery = restarted.beginTsoPayloadDelivery(
      L"exercise", ownerId, rti1516_2025::HLAinteger64Time(7), true);
  REQUIRE(delivery.status ==
      umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(delivery.deliveryStatus ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(delivery.deliveries.size() == 1U);
  auto const* directed = std::get_if<umbra::detail::TsoDirectedInteractionDelivery>(
      &delivery.deliveries.front());
  REQUIRE(directed);
  REQUIRE(directed->message.messageId == enqueued.messageId);
  REQUIRE(directed->message.sentParameterHandles ==
      std::vector<std::uint64_t>{*restartedPayload});
  REQUIRE(directed->message.parameters.size() == 1U);
  REQUIRE(directed->message.parameters.front().first == *restartedPayload);
  REQUIRE(std::string(
              static_cast<char const*>(directed->message.parameters.front().second.data()),
              payloadBytes.size()) == payloadBytes);
  REQUIRE(directed->message.transportationName == "HLAreliable");
  REQUIRE(directed->message.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(directed->message.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(restarted.beginTsoInteractionCallback(
      L"exercise", ownerId, enqueued.messageId));
  REQUIRE(restarted.completeTsoDelivery(
      L"exercise", directed->queuedMessage).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);

  auto const retracted = restarted.retractTsoMessageForProducer(
      L"exercise", publisherId, enqueued.messageId,
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  REQUIRE(retracted.status ==
      umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(retracted.timestampEligible);
  REQUIRE(retracted.requestRetractionNotifications.size() == 1U);
  REQUIRE(retracted.requestRetractionNotifications.front().receivingFederateId ==
      ownerId);
  REQUIRE(restarted.canDeliverTsoRequestRetraction(
      L"exercise", ownerId, enqueued.messageId));

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Federation state images round-trip canonical control, temporal, object, and interaction declaration state",
    "[unit][kernel][federation-registry][save-restore][durable-save][state-image]"
    "[tso-retraction-ledger-state][ownership-ledger-state][object-visibility-state]"
    "[object-lifecycle-state][membership-update-state][membership-reflection-state]"
    "[membership-lifecycle-state][membership-interaction-send-state]"
    "[membership-interaction-receive-state][object-class-declaration-state]"
    "[synchronization-state][region-state][application-value-state]"
    "[pending-application-request-state]") {
  umbra::detail::FederationStateImage image;
  image.federationName = L"exercise / Δ";
  image.logicalTimeImplementationName = L"HLAinteger64Time";
  image.normalizationSeed = 0x123456789abcdef0ULL;
  image.federationSwitches = 0x1fU;
  image.interactionDeclarationCount = 1U;
  image.objectClassDeclarationCount = 1U;
  image.regionCount = 1U;
  image.objectInstanceCount = 3U;
  image.nextObjectInstanceHandle = 19U;
  image.members.push_back({
      7U,
      L"alice / Δ",
      L"trainer",
      0xa5U,
      4U,
      -3,
      12U,
  });
  image.members.front().successfulUpdateAttributeValuesCount = 3U;
  image.members.front().successfulUpdateCountsByClassAndTransportation = {
      {4U, "HLAreliable", 3U},
  };
  image.members.front().successfullyUpdatedObjectInstanceHandles = {19U, 20U};
  image.members.front().successfullyUpdatedObjectInstanceClassHandles = {
      {19U, 4U},
      {20U, 4U},
  };
  image.members.front().successfulReflectionsReceivedCount = 9U;
  image.members.front().successfulObjectInstanceRegistrationsCount = 10U;
  image.members.front().successfulObjectInstanceDeletionsCount = 11U;
  image.members.front().successfulObjectInstanceRemovalsCount = 12U;
  image.members.front().successfulObjectInstanceDiscoveriesCount = 13U;
  image.members.front().successfulReflectionCountsByClassAndTransportation = {
      {4U, "HLAreliable", 2U},
  };
  image.members.front().successfullyReflectedObjectInstanceHandles = {19U, 20U};
  image.members.front().successfullyReflectedObjectInstanceClassHandles = {
      {19U, 4U},
      {20U, 4U},
  };
  image.members.front().successfulInteractionsSentCount = 6U;
  image.members.front().successfulDirectedInteractionsSentCount = 2U;
  image.members.front().successfulInteractionCountsByClassAndTransportation = {
      {22U, "HLAreliable", 6U},
      {23U, "HLAbestEffort", 1U},
  };
  image.members.front().successfulDirectedInteractionCountsByClassAndTransportation = {
      {22U, "HLAreliable", 2U},
  };
  image.members.front().successfulInteractionsReceivedCount = 8U;
  image.members.front().successfulDirectedInteractionsReceivedCount = 3U;
  image.members.front().successfulInteractionReceiptCountsByClassAndTransportation = {
      {22U, "HLAreliable", 8U},
      {23U, "HLAbestEffort", 1U},
  };
  image.members.front().successfulDirectedInteractionReceiptCountsByClassAndTransportation = {
      {22U, "HLAreliable", 3U},
  };
  image.federateNamesById.emplace_back(7U, L"alice / Δ");
  image.reservedObjectInstanceNames = {
      {7U, L"reserved-table"},
  };
  image.synchronizationPointCount = 1U;
  image.synchronizationPoints = {
      {L"restore-sync", std::string{"sync\0", 5U}, {7U}, {7U}, {{7U, false}}},
  };
  image.regions = {
      {8U, 7U, {8U}, {{8U, 1U, 20U}}, {{8U, 2U, 20U}}, true, true},
  };
  umbra::detail::FederationStateImageObjectClassAttributeDeclarations declarations;
  declarations.federateId = 7U;
  declarations.subscriptionGeneration = 17U;
  umbra::detail::FederationStateImageObjectClassAttributeClass objectClass;
  objectClass.objectClassHandle = 4U;
  objectClass.privilegeToDeleteExplicitlyUnpublished = true;
  objectClass.explicitlyPublishedAttributeHandles = {3U};
  objectClass.subscribedAttributes = {{3U, true}};
  // An empty designator is the normative request for the FOM default rate;
  // the state image must preserve that distinction from a missing record.
  objectClass.subscribedUpdateRateDesignators = {{3U, ""}};
  objectClass.regionalSubscribedAttributes = {{3U, 8U, true}};
  objectClass.regionalSubscribedUpdateRateDesignators = {{3U, 8U, "HLAreliable"}};
  objectClass.defaultTransportationTypes = {{3U, "HLAreliable"}};
  objectClass.defaultOrderTypes = {{3U, 1U}};
  declarations.classes.push_back(std::move(objectClass));
  image.objectClassAttributeDeclarations.push_back(std::move(declarations));

  umbra::detail::FederationStateImageTimeState time;
  time.federateId = 7U;
  time.implementationName = L"HLAinteger64Time";
  time.flags = 0x7fU;
  time.pendingGeneration = 11U;
  time.advanceMode = 2U;
  time.pendingTimeRegulationGeneration = 12U;
  time.pendingTimeConstrainedGeneration = 13U;
  time.nextGeneration = 14U;
  time.pendingModifiedLookaheadEncoding = std::string{"\x10\x11", 2U};
  time.applicationRequestLedgerPresent = true;
  time.currentTimeEncoding = std::string{"\0\x01\x02", 3U};
  time.lookaheadEncoding = std::string{"\x03\x04", 2U};
  time.queuedTsoCount = 2U;
  time.inTransitTsoCount = 1U;
  image.timeStates.push_back(std::move(time));
  image.objects.push_back({
      19U,
      L"table-19",
      4U,
      7U,
      false,
      16U,
      {{3U, 7U, "HLAreliable", 1U, {8U, 9U}}},
  });
  image.objects.front().attributeValues = {
      {3U, std::string{"\0\xfe", 2U}},
  };
  image.objects.front().attributeValuesPresent = true;
  image.objects.front().pendingAttributeOwnershipAcquisitionIfAvailableRequests.push_back({
      41U,
      7U,
      12U,
      {3U},
      std::string{"if-available\0", 13U},
  });
  image.objects.front().pendingAttributeOwnershipAcquisitionRequests.push_back({
      42U,
      7U,
      13U,
      {3U},
      {3U},
      {},
      {{8U, {3U}}},
      std::string{"regular\0", 8U},
  });
  image.objects.front().pendingAttributeOwnershipAcquisitionCancellations.push_back({
      43U,
      7U,
      {3U},
  });
  image.objects.front().pendingAttributeOwnershipDivestitureIfWantedNotifications.push_back({
      44U,
      8U,
      {3U},
      std::string{"divestiture-if-wanted\0", 22U},
  });
  image.objects.front().pendingConfirmDivestitureNotifications.push_back({
      45U,
      8U,
      {3U},
      std::string{"confirm-divestiture\0", 20U},
  });
  image.objects.front().pendingAttributeTransportationTypeChanges.push_back({
      46U,
      7U,
      {3U},
      "HLAbestEffort",
  });
  image.objects.front().pendingNegotiatedAttributeOwnershipDivestitures.push_back({
      3U,
      7U,
      8U,
      42U,
      false,
      true,
      false,
      std::string{"negotiated\0", 11U},
  });
  image.objects.front().ownershipAssumptionRecipientsByAttribute.push_back({
      3U,
      {8U, 9U},
  });
  image.objects.front().ownershipAssumptionUserSuppliedTagsByAttribute.push_back({
      3U,
      std::string{"assume\0", 7U},
  });
  image.objects.front().knownObjectClassHandlesByFederate.push_back({
      7U,
      4U,
  });
  image.objects.front().pendingDiscoveryFederateIds = {8U};
  image.objects.front().pendingRemovalFederateIds = {9U};
  image.objects.front().connectionLossAutomaticRemovalFederateIds = {9U};
  image.objects.front().deferredConnectionLossTsoRemovalFederateIds = {9U};
  image.objects.front().pendingTimestampedRemovalFederateIds = {8U};
  image.objects.front().pendingTimestampedDeletionMessageId = 88U;
  umbra::detail::FederationStateImageInteractionDeclaration interactions;
  interactions.federateId = 7U;
  interactions.publishedInteractionClasses = {21U};
  interactions.subscribedInteractionClasses = {{22U, true}};
  interactions.regionalSubscribedInteractionClasses = {{22U, 8U, true}};
  interactions.publishedObjectClassDirectedInteractions = {{4U, 23U}};
  interactions.subscribedObjectClassDirectedInteractions = {{4U, 24U, false}};
  interactions.interactionTransportationTypes = {{22U, "HLAreliable"}};
  interactions.interactionOrderTypes = {{22U, 1U}};
  interactions.pendingInteractionTransportationTypeChanges = {{22U, "HLAbestEffort"}};
  image.interactionDeclarations.push_back(std::move(interactions));

  umbra::detail::FederationStateImageTsoInteractionMessage tsoInteraction;
  tsoInteraction.messageId = 55U;
  tsoInteraction.producingFederateId = 7U;
  tsoInteraction.sentInteractionClassHandle = 22U;
  tsoInteraction.sentParameterHandles = {31U};
  tsoInteraction.parameters = {{31U, std::string{"\0\x01", 2U}}};
  tsoInteraction.userSuppliedTag = std::string{"tag\0", 4U};
  tsoInteraction.transportationName = "HLAreliable";
  tsoInteraction.sentRegionHandles = {8U};
  tsoInteraction.sentRegionSnapshots = {{
      8U,
      true,
      {8U},
      {{8U, 10U, 20U}},
  }};
  tsoInteraction.defaultRegionUsed = false;
  tsoInteraction.timestampEncoding = std::string{"\x05\x06", 2U};
  tsoInteraction.sentOrderType = 1U;
  tsoInteraction.receivedOrderType = 1U;
  image.tsoInteractionMessageCount = 1U;
  image.tsoInteractionMessages.push_back(std::move(tsoInteraction));

  umbra::detail::FederationStateImageTsoAttributeUpdateMessage attributeUpdate;
  attributeUpdate.messageId = 66U;
  attributeUpdate.producingFederateId = 7U;
  attributeUpdate.objectInstanceHandle = 19U;
  attributeUpdate.attributes = {{7U, std::string{"\x0a\x0b", 2U}}};
  attributeUpdate.userSuppliedTag = std::string{"attribute", 9U};
  attributeUpdate.passelsByRecipient = {{
      8U,
      {{
          "HLAreliable",
          {7U},
          {},
          {},
          false,
          1U,
      }},
  }};
  attributeUpdate.timestampEncoding = std::string{"\x0c\x0d", 2U};
  image.tsoAttributeUpdateMessageCount = 1U;
  image.tsoAttributeUpdateMessages.push_back(std::move(attributeUpdate));

  umbra::detail::FederationStateImageTsoDirectedInteractionMessage directedInteraction;
  directedInteraction.messageId = 77U;
  directedInteraction.producingFederateId = 7U;
  directedInteraction.objectInstanceHandle = 19U;
  directedInteraction.sentInteractionClassHandle = 24U;
  directedInteraction.sentParameterHandles = {32U};
  directedInteraction.parameters = {{32U, std::string{"\x07", 1U}}};
  directedInteraction.userSuppliedTag = std::string{"directed", 8U};
  directedInteraction.transportationName = "HLAreliable";
  directedInteraction.recipients = {{8U, 19U, 24U, {32U}}};
  directedInteraction.timestampEncoding = std::string{"\x08\x09", 2U};
  directedInteraction.sentOrderType = 1U;
  directedInteraction.receivedOrderType = 1U;
  image.tsoDirectedInteractionMessageCount = 1U;
  image.tsoDirectedInteractionMessages.push_back(std::move(directedInteraction));
  umbra::detail::FederationStateImageTsoObjectDeletionMessage objectDeletion;
  objectDeletion.messageId = 88U;
  objectDeletion.producingFederateId = 7U;
  objectDeletion.objectInstanceHandle = 19U;
  objectDeletion.userSuppliedTag = std::string{"delete\0", 7U};
  objectDeletion.recipients = {{8U, 19U}};
  objectDeletion.timestampEncoding = std::string{"\x0e\x0f", 2U};
  umbra::detail::FederationStateImageTsoObjectDeletionReconstitution reconstitution;
  reconstitution.object = {
      19U,
      L"table-19",
      4U,
      7U,
      false,
      0U,
      {{3U, 7U, "HLAreliable", 1U, {8U, 9U}}},
  };
  reconstitution.knownObjectClassHandlesByFederate = {{7U, 4U}, {8U, 4U}};
  objectDeletion.reconstitution = std::move(reconstitution);
  image.tsoObjectDeletionMessageCount = 1U;
  image.tsoObjectDeletionMessages.push_back(std::move(objectDeletion));
  umbra::detail::FederationStateImageTsoRequestRetractionRecord retraction;
  retraction.messageId = 55U;
  retraction.producingFederateId = 7U;
  retraction.timestampEncoding = std::string{"\x05\x06", 2U};
  retraction.recipientStates = {{8U, 1U}, {9U, 2U}};
  image.tsoRequestRetractionRecords.push_back(std::move(retraction));
  umbra::detail::FederationStateImageTsoRequestRetractionRecord terminalRetraction;
  terminalRetraction.messageId = 99U;
  terminalRetraction.producingFederateId = 7U;
  terminalRetraction.retractionApplied = true;
  terminalRetraction.terminal = true;
  terminalRetraction.producerResigned = true;
  terminalRetraction.recipientStates = {{8U, 3U}};
  image.tsoRequestRetractionRecords.push_back(std::move(terminalRetraction));
  image.tsoQueueEntries = {
      {55U, 8U, 3U, 0U, std::string{"\x05\x06", 2U}},
      {88U, 8U, 5U, 0U, std::string{"\x0e\x0f", 2U}},
      {77U, 8U, 4U, 1U, std::string{"\x08\x09", 2U}},
  };

  auto const encoded = umbra::detail::FederationStateImageCodec::encode(image);
  REQUIRE(encoded.starts_with("umbra-federation-state/v1\n"));
  auto const decoded = umbra::detail::FederationStateImageCodec::decode(encoded);
  REQUIRE(decoded.federationName == image.federationName);
  REQUIRE(decoded.logicalTimeImplementationName == image.logicalTimeImplementationName);
  REQUIRE(decoded.normalizationSeed == image.normalizationSeed);
  REQUIRE(decoded.members.size() == 1U);
  REQUIRE(decoded.members.front().name == L"alice / Δ");
  REQUIRE(decoded.members.front().momReportPeriodSeconds == -3);
  REQUIRE(decoded.members.front().successfulUpdateAttributeValuesCount == 3U);
  REQUIRE(decoded.members.front().successfulUpdateCountsByClassAndTransportation.size() == 1U);
  REQUIRE(decoded.members.front().successfulUpdateCountsByClassAndTransportation.front()
              .objectClassHandle == 4U);
  REQUIRE(decoded.members.front().successfulUpdateCountsByClassAndTransportation.front()
              .transportationName == "HLAreliable");
  REQUIRE(decoded.members.front().successfullyUpdatedObjectInstanceHandles ==
      std::vector<std::uint64_t>{19U, 20U});
  REQUIRE(decoded.members.front().successfullyUpdatedObjectInstanceClassHandles.size() == 2U);
  REQUIRE(decoded.members.front().successfullyUpdatedObjectInstanceClassHandles.back()
              .objectClassHandle == 4U);
  REQUIRE(decoded.members.front().successfulReflectionsReceivedCount == 9U);
  REQUIRE(decoded.members.front().successfulObjectInstanceRegistrationsCount == 10U);
  REQUIRE(decoded.members.front().successfulObjectInstanceDeletionsCount == 11U);
  REQUIRE(decoded.members.front().successfulObjectInstanceRemovalsCount == 12U);
  REQUIRE(decoded.members.front().successfulObjectInstanceDiscoveriesCount == 13U);
  REQUIRE(decoded.members.front().successfulReflectionCountsByClassAndTransportation.size() ==
      1U);
  REQUIRE(decoded.members.front().successfulReflectionCountsByClassAndTransportation.front()
              .objectClassHandle == 4U);
  REQUIRE(decoded.members.front().successfulReflectionCountsByClassAndTransportation.front()
              .transportationName == "HLAreliable");
  REQUIRE(decoded.members.front().successfullyReflectedObjectInstanceHandles ==
      std::vector<std::uint64_t>{19U, 20U});
  REQUIRE(decoded.members.front().successfullyReflectedObjectInstanceClassHandles.size() == 2U);
  REQUIRE(decoded.members.front().successfullyReflectedObjectInstanceClassHandles.back()
              .objectClassHandle == 4U);
  REQUIRE(decoded.members.front().successfulInteractionsSentCount == 6U);
  REQUIRE(decoded.members.front().successfulDirectedInteractionsSentCount == 2U);
  REQUIRE(decoded.members.front().successfulInteractionCountsByClassAndTransportation.size() ==
      2U);
  REQUIRE(decoded.members.front().successfulInteractionCountsByClassAndTransportation.front()
              .interactionClassHandle == 22U);
  REQUIRE(decoded.members.front().successfulDirectedInteractionCountsByClassAndTransportation
              .size() == 1U);
  REQUIRE(decoded.members.front()
              .successfulDirectedInteractionCountsByClassAndTransportation.front()
              .count == 2U);
  REQUIRE(decoded.members.front().successfulInteractionsReceivedCount == 8U);
  REQUIRE(decoded.members.front().successfulDirectedInteractionsReceivedCount == 3U);
  REQUIRE(decoded.members.front().successfulInteractionReceiptCountsByClassAndTransportation
              .size() == 2U);
  REQUIRE(decoded.members.front().successfulInteractionReceiptCountsByClassAndTransportation
              .front().interactionClassHandle == 22U);
  REQUIRE(decoded.members.front().successfulDirectedInteractionReceiptCountsByClassAndTransportation
              .size() == 1U);
  REQUIRE(decoded.members.front().successfulDirectedInteractionReceiptCountsByClassAndTransportation
              .front().count == 3U);
  REQUIRE(decoded.reservedObjectInstanceNames.size() == 1U);
  REQUIRE(decoded.reservedObjectInstanceNames.front().federateId == 7U);
  REQUIRE(decoded.reservedObjectInstanceNames.front().objectInstanceName ==
      L"reserved-table");
  REQUIRE(decoded.synchronizationPoints.size() == 1U);
  REQUIRE(decoded.synchronizationPoints.front().label == L"restore-sync");
  REQUIRE(decoded.synchronizationPoints.front().userSuppliedTag ==
      std::string{"sync\0", 5U});
  REQUIRE(decoded.synchronizationPoints.front().synchronizationSet ==
      std::vector<std::uint64_t>{7U});
  REQUIRE(decoded.synchronizationPoints.front().announcedFederates ==
      std::vector<std::uint64_t>{7U});
  REQUIRE(decoded.synchronizationPoints.front().achievedFederates ==
      std::vector<std::pair<std::uint64_t, bool>>{{7U, false}});
  REQUIRE(decoded.regions.size() == 1U);
  REQUIRE(decoded.regions.front().handle == 8U);
  REQUIRE(decoded.regions.front().ownerFederateId == 7U);
  REQUIRE(decoded.regions.front().dimensionHandles ==
      std::vector<std::uint64_t>{8U});
  REQUIRE(decoded.regions.front().pendingRangeBounds.front().lowerBound == 1U);
  REQUIRE(decoded.regions.front().committedRangeBounds.front().upperBound == 20U);
  REQUIRE(decoded.regions.front().specificationCommitted);
  REQUIRE(decoded.regions.front().inUse);
  REQUIRE(decoded.objectClassAttributeDeclarations.size() == 1U);
  REQUIRE(decoded.objectClassAttributeDeclarations.front().federateId == 7U);
  REQUIRE(decoded.objectClassAttributeDeclarations.front().subscriptionGeneration == 17U);
  REQUIRE(decoded.objectClassAttributeDeclarations.front().classes.size() == 1U);
  auto const& decodedObjectClass =
      decoded.objectClassAttributeDeclarations.front().classes.front();
  REQUIRE(decodedObjectClass.objectClassHandle == 4U);
  REQUIRE(decodedObjectClass.privilegeToDeleteExplicitlyUnpublished);
  REQUIRE(decodedObjectClass.explicitlyPublishedAttributeHandles ==
      std::vector<std::uint64_t>{3U});
  REQUIRE(decodedObjectClass.subscribedAttributes.size() == 1U);
  REQUIRE(decodedObjectClass.subscribedAttributes.front().active);
  REQUIRE(decodedObjectClass.subscribedUpdateRateDesignators.size() == 1U);
  REQUIRE(decodedObjectClass.subscribedUpdateRateDesignators.front().value.empty());
  REQUIRE(decodedObjectClass.regionalSubscribedAttributes.size() == 1U);
  REQUIRE(decodedObjectClass.regionalSubscribedAttributes.front().regionHandle == 8U);
  REQUIRE(decodedObjectClass.regionalSubscribedUpdateRateDesignators.size() == 1U);
  REQUIRE(decodedObjectClass.regionalSubscribedUpdateRateDesignators.front().value ==
      "HLAreliable");
  REQUIRE(decodedObjectClass.defaultTransportationTypes.size() == 1U);
  REQUIRE(decodedObjectClass.defaultTransportationTypes.front().value == "HLAreliable");
  REQUIRE(decodedObjectClass.defaultOrderTypes.size() == 1U);
  REQUIRE(decodedObjectClass.defaultOrderTypes.front().orderType == 1U);
  REQUIRE(decoded.timeStates.size() == 1U);
  REQUIRE(decoded.timeStates.front().currentTimeEncoding ==
      std::optional<std::string>{std::string{"\0\x01\x02", 3U}});
  REQUIRE(decoded.timeStates.front().pendingTimeRegulationGeneration == 12U);
  REQUIRE(decoded.timeStates.front().pendingTimeConstrainedGeneration == 13U);
  REQUIRE(decoded.timeStates.front().nextGeneration == 14U);
  REQUIRE(decoded.timeStates.front().pendingModifiedLookaheadEncoding ==
      std::optional<std::string>{std::string{"\x10\x11", 2U}});
  REQUIRE(decoded.timeStates.front().applicationRequestLedgerPresent);
  REQUIRE(decoded.timeStates.front().queuedTsoCount == 2U);
  REQUIRE(decoded.objects.size() == 1U);
  REQUIRE(decoded.objects.front().name == L"table-19");
  REQUIRE(decoded.objects.front().attributes.size() == 1U);
  REQUIRE(decoded.objects.front().attributes.front().ownerFederateId == 7U);
  REQUIRE(decoded.objects.front().attributes.front().updateRegionHandles ==
      std::vector<std::uint64_t>{8U, 9U});
  REQUIRE(decoded.objects.front().attributeValuesPresent);
  REQUIRE(decoded.objects.front().attributeValues.size() == 1U);
  REQUIRE(decoded.objects.front().attributeValues.front().attributeHandle == 3U);
  REQUIRE(decoded.objects.front().attributeValues.front().value ==
      std::string{"\0\xfe", 2U});
  REQUIRE(decoded.objects.front()
              .pendingAttributeOwnershipAcquisitionIfAvailableRequests.size() == 1U);
  REQUIRE(decoded.objects.front()
              .pendingAttributeOwnershipAcquisitionIfAvailableRequests.front()
              .requestId == 41U);
  REQUIRE(decoded.objects.front()
              .pendingAttributeOwnershipAcquisitionIfAvailableRequests.front()
              .requestSequence == 12U);
  REQUIRE(decoded.objects.front()
              .pendingAttributeOwnershipAcquisitionIfAvailableRequests.front()
              .desiredAttributeHandles == std::vector<std::uint64_t>{3U});
  REQUIRE(decoded.objects.front()
              .pendingAttributeOwnershipAcquisitionIfAvailableRequests.front()
              .userSuppliedTag == std::string{"if-available\0", 13U});
  REQUIRE(decoded.objects.front().pendingAttributeOwnershipAcquisitionRequests.size() == 1U);
  REQUIRE(decoded.objects.front().pendingAttributeOwnershipAcquisitionRequests.front()
              .requestId == 42U);
  REQUIRE(decoded.objects.front().pendingAttributeOwnershipAcquisitionRequests.front()
              .notificationQueuedAttributeHandles == std::vector<std::uint64_t>{3U});
  REQUIRE(decoded.objects.front().pendingAttributeOwnershipAcquisitionRequests.front()
              .releaseCallbacksQueuedByOwningFederate ==
          std::vector<std::pair<std::uint64_t, std::vector<std::uint64_t>>>{{8U, {3U}}});
  REQUIRE(decoded.objects.front().pendingAttributeOwnershipAcquisitionRequests.front()
              .userSuppliedTag == std::string{"regular\0", 8U});
  REQUIRE(decoded.objects.front().pendingAttributeOwnershipAcquisitionCancellations.size() == 1U);
  REQUIRE(decoded.objects.front().pendingAttributeOwnershipAcquisitionCancellations.front()
              .cancellationId == 43U);
  REQUIRE(decoded.objects.front().pendingAttributeOwnershipAcquisitionCancellations.front()
              .requestingFederateId == 7U);
  REQUIRE(decoded.objects.front().pendingAttributeOwnershipAcquisitionCancellations.front()
              .attributeHandles == std::vector<std::uint64_t>{3U});
  REQUIRE(decoded.objects.front().pendingAttributeOwnershipDivestitureIfWantedNotifications.size() == 1U);
  REQUIRE(decoded.objects.front().pendingAttributeOwnershipDivestitureIfWantedNotifications.front()
              .notificationId == 44U);
  REQUIRE(decoded.objects.front().pendingAttributeOwnershipDivestitureIfWantedNotifications.front()
              .receivingFederateId == 8U);
  REQUIRE(decoded.objects.front().pendingAttributeOwnershipDivestitureIfWantedNotifications.front()
              .attributeHandles == std::vector<std::uint64_t>{3U});
  REQUIRE(decoded.objects.front().pendingAttributeOwnershipDivestitureIfWantedNotifications.front()
              .userSuppliedTag == std::string{"divestiture-if-wanted\0", 22U});
  REQUIRE(decoded.objects.front().pendingConfirmDivestitureNotifications.size() == 1U);
  REQUIRE(decoded.objects.front().pendingConfirmDivestitureNotifications.front()
              .notificationId == 45U);
  REQUIRE(decoded.objects.front().pendingConfirmDivestitureNotifications.front()
              .receivingFederateId == 8U);
  REQUIRE(decoded.objects.front().pendingConfirmDivestitureNotifications.front()
              .attributeHandles == std::vector<std::uint64_t>{3U});
  REQUIRE(decoded.objects.front().pendingConfirmDivestitureNotifications.front()
              .userSuppliedTag == std::string{"confirm-divestiture\0", 20U});
  REQUIRE(decoded.objects.front().pendingAttributeTransportationTypeChanges.size() == 1U);
  REQUIRE(decoded.objects.front().pendingAttributeTransportationTypeChanges.front()
              .requestId == 46U);
  REQUIRE(decoded.objects.front().pendingAttributeTransportationTypeChanges.front()
              .requestingFederateId == 7U);
  REQUIRE(decoded.objects.front().pendingAttributeTransportationTypeChanges.front()
              .attributeHandles == std::vector<std::uint64_t>{3U});
  REQUIRE(decoded.objects.front().pendingAttributeTransportationTypeChanges.front()
              .transportationName == "HLAbestEffort");
  REQUIRE(decoded.objects.front().pendingNegotiatedAttributeOwnershipDivestitures.size() == 1U);
  REQUIRE(decoded.objects.front().pendingNegotiatedAttributeOwnershipDivestitures.front()
              .attributeHandle == 3U);
  REQUIRE(decoded.objects.front().pendingNegotiatedAttributeOwnershipDivestitures.front()
              .divestingFederateId == 7U);
  REQUIRE(decoded.objects.front().pendingNegotiatedAttributeOwnershipDivestitures.front()
              .acquiringFederateId == 8U);
  REQUIRE(decoded.objects.front().pendingNegotiatedAttributeOwnershipDivestitures.front()
              .acquisitionRequestId == 42U);
  REQUIRE(decoded.objects.front().pendingNegotiatedAttributeOwnershipDivestitures.front()
              .confirmationQueued);
  REQUIRE_FALSE(decoded.objects.front().pendingNegotiatedAttributeOwnershipDivestitures.front()
                    .confirmationDelivered);
  REQUIRE(decoded.objects.front().pendingNegotiatedAttributeOwnershipDivestitures.front()
              .userSuppliedTag == std::string{"negotiated\0", 11U});
  REQUIRE(decoded.objects.front().ownershipAssumptionRecipientsByAttribute.size() == 1U);
  REQUIRE(decoded.objects.front().ownershipAssumptionRecipientsByAttribute.front()
              .attributeHandle == 3U);
  REQUIRE(decoded.objects.front().ownershipAssumptionRecipientsByAttribute.front()
              .recipientFederateIds == std::vector<std::uint64_t>{8U, 9U});
  REQUIRE(decoded.objects.front().ownershipAssumptionUserSuppliedTagsByAttribute.size() == 1U);
  REQUIRE(decoded.objects.front().ownershipAssumptionUserSuppliedTagsByAttribute.front()
              .attributeHandle == 3U);
  REQUIRE(decoded.objects.front().ownershipAssumptionUserSuppliedTagsByAttribute.front()
              .userSuppliedTag == std::string{"assume\0", 7U});
  REQUIRE(decoded.objects.front().knownObjectClassHandlesByFederate.size() == 1U);
  REQUIRE(decoded.objects.front().knownObjectClassHandlesByFederate.front().federateId == 7U);
  REQUIRE(decoded.objects.front().knownObjectClassHandlesByFederate.front().objectClassHandle == 4U);
  REQUIRE(decoded.objects.front().pendingDiscoveryFederateIds ==
      std::vector<std::uint64_t>{8U});
  REQUIRE(decoded.objects.front().pendingRemovalFederateIds ==
      std::vector<std::uint64_t>{9U});
  REQUIRE(decoded.objects.front().connectionLossAutomaticRemovalFederateIds ==
      std::vector<std::uint64_t>{9U});
  REQUIRE(decoded.objects.front().deferredConnectionLossTsoRemovalFederateIds ==
      std::vector<std::uint64_t>{9U});
  REQUIRE(decoded.objects.front().pendingTimestampedRemovalFederateIds ==
      std::vector<std::uint64_t>{8U});
  REQUIRE(decoded.objects.front().pendingTimestampedDeletionMessageId ==
      std::optional<std::uint64_t>{88U});
  REQUIRE(decoded.interactionDeclarations.size() == 1U);
  REQUIRE(decoded.interactionDeclarations.front().federateId == 7U);
  REQUIRE(decoded.interactionDeclarations.front().publishedInteractionClasses ==
      std::vector<std::uint64_t>{21U});
  REQUIRE(decoded.interactionDeclarations.front().subscribedInteractionClasses.front().active);
  REQUIRE(decoded.interactionDeclarations.front()
              .regionalSubscribedInteractionClasses.front()
              .regionHandle == 8U);
  REQUIRE(decoded.interactionDeclarations.front()
              .publishedObjectClassDirectedInteractions.front()
              .interactionClassHandle == 23U);
  REQUIRE_FALSE(decoded.interactionDeclarations.front()
                    .subscribedObjectClassDirectedInteractions.front()
                    .active);
  REQUIRE(decoded.interactionDeclarations.front()
              .interactionTransportationTypes.front()
              .value == "HLAreliable");
  REQUIRE(decoded.interactionDeclarations.front().interactionOrderTypes.front().orderType == 1U);
  REQUIRE(decoded.interactionDeclarations.front()
              .pendingInteractionTransportationTypeChanges.front()
              .value == "HLAbestEffort");
  REQUIRE(decoded.tsoInteractionMessages.size() == 1U);
  REQUIRE(decoded.tsoInteractionMessages.front().messageId == 55U);
  REQUIRE(decoded.tsoInteractionMessages.front().parameters.front().value ==
      std::string{"\0\x01", 2U});
  REQUIRE(decoded.tsoInteractionMessages.front().userSuppliedTag ==
      std::string{"tag\0", 4U});
  REQUIRE(decoded.tsoInteractionMessages.front().sentRegionSnapshots.front()
              .committedRangeBounds.front()
              .upperBound == 20U);
  REQUIRE(decoded.tsoInteractionMessages.front().timestampEncoding ==
      std::optional<std::string>{std::string{"\x05\x06", 2U}});
  REQUIRE(decoded.tsoInteractionMessages.front().sentParameterHandles ==
      std::vector<std::uint64_t>{31U});
  REQUIRE(decoded.tsoInteractionMessages.front().sentRegionHandles ==
      std::vector<std::uint64_t>{8U});
  REQUIRE(decoded.tsoAttributeUpdateMessages.size() == 1U);
  REQUIRE(decoded.tsoAttributeUpdateMessages.front().messageId == 66U);
  REQUIRE(decoded.tsoAttributeUpdateMessages.front().attributes.front().attributeHandle == 7U);
  REQUIRE(decoded.tsoAttributeUpdateMessages.front().attributes.front().value ==
      std::string{"\x0a\x0b", 2U});
  REQUIRE(decoded.tsoAttributeUpdateMessages.front().passelsByRecipient.front()
              .passels.front()
              .sentAttributeHandles == std::vector<std::uint64_t>{7U});
  REQUIRE(decoded.tsoDirectedInteractionMessages.size() == 1U);
  REQUIRE(decoded.tsoDirectedInteractionMessages.front().messageId == 77U);
  REQUIRE(decoded.tsoDirectedInteractionMessages.front().objectInstanceHandle == 19U);
  REQUIRE(decoded.tsoDirectedInteractionMessages.front().recipients.front()
              .receivingFederateId == 8U);
  REQUIRE(decoded.tsoDirectedInteractionMessages.front().recipients.front()
              .receivedParameterHandles == std::vector<std::uint64_t>{32U});
  REQUIRE(decoded.tsoObjectDeletionMessages.size() == 1U);
  REQUIRE(decoded.tsoObjectDeletionMessages.front().messageId == 88U);
  REQUIRE(decoded.tsoObjectDeletionMessages.front().userSuppliedTag ==
      std::string{"delete\0", 7U});
  REQUIRE(decoded.tsoObjectDeletionMessages.front().recipients.front()
              .objectInstanceHandle == 19U);
  REQUIRE(decoded.tsoObjectDeletionMessages.front().reconstitution.has_value());
  REQUIRE(decoded.tsoObjectDeletionMessages.front().reconstitution->object.name ==
      L"table-19");
  REQUIRE(decoded.tsoObjectDeletionMessages.front().reconstitution
              ->knownObjectClassHandlesByFederate.size() == 2U);
  REQUIRE(decoded.tsoRequestRetractionRecords.size() == 2U);
  REQUIRE(decoded.tsoRequestRetractionRecords.front().messageId == 55U);
  REQUIRE(decoded.tsoRequestRetractionRecords.front().recipientStates.size() == 2U);
  REQUIRE(decoded.tsoRequestRetractionRecords.front().recipientStates.front().state == 1U);
  REQUIRE(decoded.tsoRequestRetractionRecords.front().recipientStates.back().state == 2U);
  REQUIRE(decoded.tsoRequestRetractionRecords.back().messageId == 99U);
  REQUIRE(decoded.tsoRequestRetractionRecords.back().timestampEncoding == std::nullopt);
  REQUIRE(decoded.tsoRequestRetractionRecords.back().retractionApplied);
  REQUIRE(decoded.tsoRequestRetractionRecords.back().recipientStates.front().state == 3U);
  REQUIRE(decoded.tsoQueueEntries.size() == 3U);
  REQUIRE(decoded.tsoQueueEntries.front().phase == 0U);
  REQUIRE(decoded.tsoQueueEntries.back().phase == 1U);
  REQUIRE(decoded.tsoQueueEntries.back().sequence == 4U);
  REQUIRE(umbra::detail::FederationStateImageCodec::encode(decoded) == encoded);

  REQUIRE_THROWS(umbra::detail::FederationStateImageCodec::decode(encoded + "junk"));
  auto malformed = encoded;
  malformed.replace(0U, std::string_view{"umbra-federation-state/v1"}.size(),
      "umbra-federation-state/v0");
  REQUIRE_THROWS(umbra::detail::FederationStateImageCodec::decode(malformed));
  auto malformedLifecycle = image;
  malformedLifecycle.objects.front().pendingTimestampedDeletionMessageId.reset();
  REQUIRE_THROWS(umbra::detail::FederationStateImageCodec::encode(malformedLifecycle));
  malformedLifecycle = image;
  malformedLifecycle.objects.front().deferredConnectionLossTsoRemovalFederateIds = {7U};
  REQUIRE_THROWS(umbra::detail::FederationStateImageCodec::encode(malformedLifecycle));
  auto malformedReservation = image;
  malformedReservation.reservedObjectInstanceNames.push_back({7U, L"reserved-table"});
  REQUIRE_THROWS(umbra::detail::FederationStateImageCodec::encode(malformedReservation));
  auto malformedSynchronization = image;
  malformedSynchronization.synchronizationPoints.front().announcedFederates = {8U};
  REQUIRE_THROWS(
      umbra::detail::FederationStateImageCodec::encode(malformedSynchronization));
  auto malformedRegion = image;
  malformedRegion.regions.front().pendingRangeBounds.front().dimensionHandle = 9U;
  REQUIRE_THROWS(umbra::detail::FederationStateImageCodec::encode(malformedRegion));
  malformedRegion = image;
  malformedRegion.regions.front().committedRangeBounds.front().lowerBound = 20U;
  REQUIRE_THROWS(umbra::detail::FederationStateImageCodec::encode(malformedRegion));
  auto malformedDeclaration = image;
  malformedDeclaration.objectClassAttributeDeclarations.front().classes.front()
      .subscribedAttributes.push_back({3U, true});
  REQUIRE_THROWS(umbra::detail::FederationStateImageCodec::encode(malformedDeclaration));
  auto malformedDeferredLookahead = image;
  malformedDeferredLookahead.timeStates.front().flags &= ~(1U << 1U);
  REQUIRE_THROWS(
      umbra::detail::FederationStateImageCodec::encode(malformedDeferredLookahead));
}

TEST_CASE(
    "Federation restore rebinds pending time-role callbacks and fences stale work",
    "[unit][kernel][federation-registry][save-restore][durable-save][restore]"
    "[time-management][time-role][callbacks][pending-application-request-state]") {
  auto store = std::make_shared<umbra::detail::MemoryFederationSaveCommitStore>();
  EmbeddedFederationRegistry registry({}, store);
  REQUIRE(registry.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);

  auto timeState = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  std::size_t dispatchCount = 0U;
  std::size_t grantCount = 0U;
  auto roleFactory = [timeState, &dispatchCount, &grantCount](
      std::uint64_t federateId,
      std::uint64_t generation,
      umbra::detail::FederationTimeRoleEnableKind kind)
      -> umbra::detail::FederationTimeGrantDispatch {
    REQUIRE(federateId != 0U);
    REQUIRE(generation != 0U);
    auto const callbackEpoch = timeState->callbackEpoch();
    return [timeState, generation, kind, callbackEpoch, &dispatchCount, &grantCount] {
      ++dispatchCount;
      std::shared_ptr<rti1516_2025::LogicalTime const> enabledTime;
      if (kind == umbra::detail::FederationTimeRoleEnableKind::regulation) {
        enabledTime = timeState->grantTimeRegulationIfCurrent(
            generation,
            callbackEpoch);
      } else {
        enabledTime = timeState->grantTimeConstrainedIfCurrent(
            generation,
            callbackEpoch);
      }
      if (enabledTime) {
        ++grantCount;
      }
    };
  };

  auto joined = registry.joinWithTimeState(
      L"exercise",
      timeState,
      L"trainer",
      L"alice",
      noOpCallbackRoute(),
      {},
      std::move(roleFactory));
  REQUIRE(joined.status == FederationRegistryStatus::applied);
  REQUIRE(joined.membership);

  auto request = timeState->requestTimeRegulation(
      std::make_shared<rti1516_2025::HLAinteger64Interval>(2));
  REQUIRE(request.status == umbra::detail::FederateTimeEnableStatus::applied);
  auto const staleEpoch = timeState->callbackEpoch();

  REQUIRE(registry.requestFederationSave(
      L"exercise", joined.membership->id, L"pending-role-checkpoint").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", joined.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveComplete(
      L"exercise", joined.membership->id).saveCompletedSuccessfully);

  // Mutate the live state so restore has to reapply the still-pending role
  // request rather than merely retaining the current enabled mode.
  REQUIRE(timeState->grantTimeRegulation(request.generation));
  REQUIRE(timeState->snapshot().timeRegulating);

  REQUIRE(registry.requestFederationRestore(
      L"exercise", joined.membership->id, L"pending-role-checkpoint").status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = registry.federateRestoreComplete(
      L"exercise", joined.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.timeRoleEnableDispatches.size() == 1U);
  REQUIRE_FALSE(timeState->snapshot().timeRegulating);
  REQUIRE(timeState->snapshot().timeRegulationPending);

  // The closure that existed before restore carries the old epoch and cannot
  // consume the restored generation, even though the generation value itself
  // is intentionally reused by the saved application-request ledger.
  REQUIRE_FALSE(timeState->grantTimeRegulationIfCurrent(
      request.generation,
      staleEpoch));
  REQUIRE(dispatchCount == 0U);
  REQUIRE(grantCount == 0U);

  auto dispatch = std::move(restored.timeRoleEnableDispatches.front());
  dispatch();
  REQUIRE(dispatchCount == 1U);
  REQUIRE(grantCount == 1U);
  REQUIRE(timeState->snapshot().timeRegulating);
  REQUIRE_FALSE(timeState->snapshot().timeRegulationPending);
}

TEST_CASE(
    "Federation restore rehydrates accepted Update Attribute Values telemetry",
    "[unit][kernel][federation-registry][save-restore][durable-save][restore]"
    "[membership-update-state]") {
  auto store = std::make_shared<umbra::detail::MemoryFederationSaveCommitStore>();
  EmbeddedFederationRegistry registry({}, store);
  REQUIRE(registry.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto joined = registry.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(joined.status == FederationRegistryStatus::applied);
  REQUIRE(joined.membership);

  auto const federateId = joined.membership->id;
  REQUIRE(registry.recordSuccessfulUpdateAttributeValues(
      L"exercise", federateId, 19U, 4U, {"HLAreliable"}) ==
      FederationRegistryStatus::applied);
  REQUIRE(registry.requestFederationSave(
      L"exercise", federateId, L"telemetry-restore").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveComplete(
      L"exercise", federateId).saveCompletedSuccessfully);

  // Mutate the live lifetime ledger after the save. Restore must return to the
  // accepted count and distinct-object projection captured in the image.
  REQUIRE(registry.recordSuccessfulUpdateAttributeValues(
      L"exercise", federateId, 20U, 4U, {"HLAreliable"}) ==
      FederationRegistryStatus::applied);
  auto restore = registry.requestFederationRestore(
      L"exercise", federateId, L"telemetry-restore");
  REQUIRE(restore.status == umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(registry.federateRestoreComplete(
      L"exercise", federateId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  REQUIRE(registry.requestFederationSave(
      L"exercise", federateId, L"telemetry-after-restore").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveComplete(
      L"exercise", federateId).saveCompletedSuccessfully);
  auto commits = store->snapshotCommits();
  REQUIRE(commits.size() == 2U);
  auto restoredImage = umbra::detail::FederationStateImageCodec::decode(
      commits.back().stateImage);
  REQUIRE(restoredImage.members.size() == 1U);
  REQUIRE(restoredImage.members.front().successfulUpdateAttributeValuesCount == 1U);
  REQUIRE(restoredImage.members.front().successfullyUpdatedObjectInstanceHandles ==
      std::vector<std::uint64_t>{19U});
  REQUIRE(restoredImage.members.front().successfullyUpdatedObjectInstanceClassHandles.size() ==
      1U);
  REQUIRE(restoredImage.members.front().successfulUpdateCountsByClassAndTransportation.front()
              .count == 1U);
}

TEST_CASE(
    "Federation restore rehydrates accepted reflection callback count",
    "[unit][kernel][federation-registry][save-restore][durable-save][restore]"
    "[membership-reflection-state]") {
  auto store = std::make_shared<umbra::detail::MemoryFederationSaveCommitStore>();
  EmbeddedFederationRegistry registry({}, store);
  REQUIRE(registry.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto joined = registry.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(joined.status == FederationRegistryStatus::applied);
  REQUIRE(joined.membership);

  auto const federateId = joined.membership->id;
  REQUIRE(registry.recordSuccessfulReflectionReceipt(
      L"exercise", federateId, 19U, "HLAreliable") ==
      FederationRegistryStatus::applied);
  REQUIRE(registry.requestFederationSave(
      L"exercise", federateId, L"reflection-restore").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveComplete(
      L"exercise", federateId).saveCompletedSuccessfully);

  // A later accepted callback must not survive restore of the earlier
  // joined-federate lifetime statistic.
  REQUIRE(registry.recordSuccessfulReflectionReceipt(
      L"exercise", federateId, 20U, "HLAbestEffort") ==
      FederationRegistryStatus::applied);
  auto restore = registry.requestFederationRestore(
      L"exercise", federateId, L"reflection-restore");
  REQUIRE(restore.status == umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(registry.federateRestoreComplete(
      L"exercise", federateId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  REQUIRE(registry.requestFederationSave(
      L"exercise", federateId, L"reflection-after-restore").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveComplete(
      L"exercise", federateId).saveCompletedSuccessfully);
  auto commits = store->snapshotCommits();
  REQUIRE(commits.size() == 2U);
  auto restoredImage = umbra::detail::FederationStateImageCodec::decode(
      commits.back().stateImage);
  REQUIRE(restoredImage.members.size() == 1U);
  REQUIRE(restoredImage.members.front().successfulReflectionsReceivedCount == 1U);
}

TEST_CASE(
    "Federation restore rehydrates accepted interaction-send counters",
    "[unit][kernel][federation-registry][save-restore][durable-save][restore]"
    "[membership-interaction-send-state]") {
  auto store = std::make_shared<umbra::detail::MemoryFederationSaveCommitStore>();
  EmbeddedFederationRegistry registry({}, store);
  REQUIRE(registry.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto joined = registry.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(joined.status == FederationRegistryStatus::applied);
  REQUIRE(joined.membership);

  auto const federateId = joined.membership->id;
  REQUIRE(registry.recordSuccessfulInteractionSend(
      L"exercise", federateId, 0U, "", false) ==
      FederationRegistryStatus::applied);
  REQUIRE(registry.recordSuccessfulInteractionSend(
      L"exercise", federateId, 0U, "", true) ==
      FederationRegistryStatus::applied);
  REQUIRE(registry.requestFederationSave(
      L"exercise", federateId, L"interaction-send-restore").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveComplete(
      L"exercise", federateId).saveCompletedSuccessfully);

  REQUIRE(registry.recordSuccessfulInteractionSend(
      L"exercise", federateId, 0U, "", true) ==
      FederationRegistryStatus::applied);
  auto restore = registry.requestFederationRestore(
      L"exercise", federateId, L"interaction-send-restore");
  REQUIRE(restore.status == umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(registry.federateRestoreComplete(
      L"exercise", federateId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  REQUIRE(registry.requestFederationSave(
      L"exercise", federateId, L"interaction-send-after-restore").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveComplete(
      L"exercise", federateId).saveCompletedSuccessfully);
  auto commits = store->snapshotCommits();
  REQUIRE(commits.size() == 2U);
  auto restoredImage = umbra::detail::FederationStateImageCodec::decode(
      commits.back().stateImage);
  REQUIRE(restoredImage.members.size() == 1U);
  REQUIRE(restoredImage.members.front().successfulInteractionsSentCount == 2U);
  REQUIRE(restoredImage.members.front().successfulDirectedInteractionsSentCount == 1U);
}

TEST_CASE(
    "Federation restore rehydrates accepted interaction-receipt counters",
    "[unit][kernel][federation-registry][save-restore][durable-save][restore]"
    "[membership-interaction-receive-state]") {
  auto store = std::make_shared<umbra::detail::MemoryFederationSaveCommitStore>();
  EmbeddedFederationRegistry registry({}, store);
  REQUIRE(registry.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto joined = registry.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(joined.status == FederationRegistryStatus::applied);
  REQUIRE(joined.membership);

  auto const federateId = joined.membership->id;
  REQUIRE(registry.recordSuccessfulInteractionReceipt(
      L"exercise", federateId, 0U, "", false) ==
      FederationRegistryStatus::applied);
  REQUIRE(registry.recordSuccessfulInteractionReceipt(
      L"exercise", federateId, 0U, "", true) ==
      FederationRegistryStatus::applied);
  REQUIRE(registry.requestFederationSave(
      L"exercise", federateId, L"interaction-receipt-restore").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveComplete(
      L"exercise", federateId).saveCompletedSuccessfully);

  REQUIRE(registry.recordSuccessfulInteractionReceipt(
      L"exercise", federateId, 0U, "", true) ==
      FederationRegistryStatus::applied);
  auto restore = registry.requestFederationRestore(
      L"exercise", federateId, L"interaction-receipt-restore");
  REQUIRE(restore.status == umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(registry.federateRestoreComplete(
      L"exercise", federateId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  REQUIRE(registry.requestFederationSave(
      L"exercise", federateId, L"interaction-receipt-after-restore").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveComplete(
      L"exercise", federateId).saveCompletedSuccessfully);
  auto commits = store->snapshotCommits();
  REQUIRE(commits.size() == 2U);
  auto restoredImage = umbra::detail::FederationStateImageCodec::decode(
      commits.back().stateImage);
  REQUIRE(restoredImage.members.size() == 1U);
  REQUIRE(restoredImage.members.front().successfulInteractionsReceivedCount == 2U);
  REQUIRE(restoredImage.members.front().successfulDirectedInteractionsReceivedCount == 1U);
}

TEST_CASE(
    "Federation restore rehydrates reserved object-instance names",
    "[unit][kernel][federation-registry][save-restore][durable-save][restore]"
    "[object-name-reservation-state]") {
  auto store = std::make_shared<umbra::detail::MemoryFederationSaveCommitStore>();
  EmbeddedFederationRegistry registry({}, store);
  REQUIRE(registry.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto joined = registry.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(joined.status == FederationRegistryStatus::applied);
  REQUIRE(joined.membership);

  auto const federateId = joined.membership->id;
  auto reservation = registry.reserveObjectInstanceName(
      L"exercise", federateId, L"reserved-table");
  REQUIRE(reservation.status == ObjectInstanceNameReservationStatus::applied);
  REQUIRE(reservation.succeeded);
  REQUIRE(registry.requestFederationSave(
      L"exercise", federateId, L"object-name-reservation-restore").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveComplete(
      L"exercise", federateId).saveCompletedSuccessfully);

  REQUIRE(registry.releaseObjectInstanceName(
      L"exercise", federateId, L"reserved-table") ==
      ObjectInstanceNameReservationStatus::applied);
  auto restore = registry.requestFederationRestore(
      L"exercise", federateId, L"object-name-reservation-restore");
  REQUIRE(restore.status == umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(registry.federateRestoreComplete(
      L"exercise", federateId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  auto duplicate = registry.reserveObjectInstanceName(
      L"exercise", federateId, L"reserved-table");
  REQUIRE(duplicate.status == ObjectInstanceNameReservationStatus::applied);
  REQUIRE_FALSE(duplicate.succeeded);
  REQUIRE(registry.requestFederationSave(
      L"exercise", federateId, L"object-name-reservation-after-restore").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveComplete(
      L"exercise", federateId).saveCompletedSuccessfully);
  auto commits = store->snapshotCommits();
  REQUIRE(commits.size() == 2U);
  auto restoredImage = umbra::detail::FederationStateImageCodec::decode(
      commits.back().stateImage);
  REQUIRE(restoredImage.reservedObjectInstanceNames.size() == 1U);
  REQUIRE(restoredImage.reservedObjectInstanceNames.front().federateId == federateId);
  REQUIRE(restoredImage.reservedObjectInstanceNames.front().objectInstanceName ==
      L"reserved-table");
}

TEST_CASE(
    "Filesystem federation save commits do not leave a temporary file on encoding failure",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][failure]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  umbra::detail::FilesystemFederationSaveCommitStore store(directory);
  umbra::detail::FederationSaveCommitDescriptor descriptor{
      L"exercise",
      std::wstring{L"checkpoint-"} + std::wstring(1U, static_cast<wchar_t>(0xD800U)),
      L"HLAinteger64Time",
      {7U},
      false};

  REQUIRE_THROWS(store.commit(descriptor));
  REQUIRE(std::filesystem::is_directory(directory));
  REQUIRE(std::filesystem::is_empty(directory));
  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Federation save completion commits before saved notifications",
    "[unit][kernel][federation-registry][save-restore][durable-save]"
    "[tso-retraction-ledger-state]") {
  auto store = std::make_shared<umbra::detail::MemoryFederationSaveCommitStore>();
  EmbeddedFederationRegistry registry({}, store);
  REQUIRE(registry.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto joined = registry.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(joined.status == FederationRegistryStatus::applied);
  REQUIRE(joined.membership);

  umbra::detail::TsoInteractionMessage pendingInteraction;
  std::vector<rti1516_2025::Octet> pendingParameterBytes{0x01U, 0x02U};
  std::vector<rti1516_2025::Octet> pendingTagBytes{0x03U};
  pendingInteraction.producingFederateId = joined.membership->id;
  pendingInteraction.sentInteractionClassHandle = 1U;
  pendingInteraction.sentParameterHandles = {2U};
  pendingInteraction.parameters = {{
      2U,
      rti1516_2025::VariableLengthData(
          pendingParameterBytes.data(),
          pendingParameterBytes.size()),
  }};
  pendingInteraction.userSuppliedTag = rti1516_2025::VariableLengthData(
      pendingTagBytes.data(),
      pendingTagBytes.size());
  pendingInteraction.transportationName = "HLAreliable";
  pendingInteraction.timestamp =
      std::make_shared<rti1516_2025::HLAinteger64Time>(7);
  auto const enqueuedInteraction = registry.enqueueTsoInteraction(
      L"exercise",
      std::move(pendingInteraction),
      {joined.membership->id},
      {joined.membership->id});
  REQUIRE(enqueuedInteraction.status ==
      umbra::detail::FederationTsoRegistryStatus::applied);

  auto requested = registry.requestFederationSave(
      L"exercise", joined.membership->id, L"checkpoint");
  REQUIRE(requested.status == umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(requested.notifications.size() == 1U);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", joined.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  auto completed = registry.federateSaveComplete(
      L"exercise", joined.membership->id);
  REQUIRE(completed.saveCompletedSuccessfully);
  REQUIRE(completed.notifications.size() == 1U);
  REQUIRE(completed.notifications.front().successful);

  auto commits = store->snapshotCommits();
  REQUIRE(commits.size() == 1U);
  REQUIRE(commits.front().federationName == L"exercise");
  REQUIRE(commits.front().label == L"checkpoint");
  REQUIRE(commits.front().logicalTimeImplementationName == L"HLAinteger64Time");
  REQUIRE(commits.front().memberFederateIds == std::vector<std::uint64_t>{joined.membership->id});
  REQUIRE_FALSE(commits.front().timed);
  REQUIRE(commits.front().stateImage.starts_with("umbra-federation-state/v1\n"));
  auto const image = umbra::detail::FederationStateImageCodec::decode(
      commits.front().stateImage);
  REQUIRE(image.federationName == L"exercise");
  REQUIRE(image.members.size() == 1U);
  REQUIRE(image.members.front().id == joined.membership->id);
  REQUIRE(image.tsoInteractionMessages.size() == 1U);
  REQUIRE(image.tsoInteractionMessages.front().messageId ==
      enqueuedInteraction.messageId);
  REQUIRE(image.tsoInteractionMessages.front().parameters.front().value ==
      std::string{"\x01\x02", 2U});
  REQUIRE(image.tsoRequestRetractionRecords.size() == 1U);
  REQUIRE(image.tsoRequestRetractionRecords.front().messageId ==
      enqueuedInteraction.messageId);
  REQUIRE(image.tsoRequestRetractionRecords.front().recipientStates.size() == 1U);
  REQUIRE(image.tsoRequestRetractionRecords.front().recipientStates.front().state == 0U);
  REQUIRE(image.tsoQueueEntries.size() == 1U);
  REQUIRE(image.tsoQueueEntries.front().messageId == enqueuedInteraction.messageId);
  REQUIRE(image.tsoQueueEntries.front().recipientFederateId ==
      joined.membership->id);
  REQUIRE(image.tsoQueueEntries.front().phase == 0U);
}

TEST_CASE(
    "Federation save images snapshot queued in-transit and delivered TSO phases",
    "[unit][kernel][federation-registry][save-restore][durable-save][state-image][tso-queue-state]") {
  auto store = std::make_shared<umbra::detail::MemoryFederationSaveCommitStore>();
  EmbeddedFederationRegistry registry({}, store);
  REQUIRE(registry.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);

  auto receiverTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  auto receiver = registry.joinWithTimeState(
      L"exercise",
      receiverTime,
      L"receiver",
      L"receiver",
      noOpCallbackRoute());
  auto observer = registry.join(
      L"exercise",
      L"observer",
      L"observer",
      noOpCallbackRoute());
  REQUIRE(receiver.membership);
  REQUIRE(observer.membership);

  auto enqueue = [&](std::int64_t timestamp) {
    auto const allocated = registry.allocateTsoMessageId(L"exercise");
    REQUIRE(allocated.status == umbra::detail::FederationTsoRegistryStatus::applied);
    auto const enqueued = registry.enqueueTsoMessage(
        L"exercise",
        allocated.messageId,
        receiver.membership->id,
        std::make_shared<rti1516_2025::HLAinteger64Time>(timestamp));
    REQUIRE(enqueued.status == umbra::detail::FederationTsoRegistryStatus::applied);
    REQUIRE(enqueued.queueStatus == umbra::detail::TsoMessageQueueStatus::applied);
    return allocated.messageId;
  };

  auto const deliveredId = enqueue(5);
  auto const inTransitId = enqueue(6);
  auto const queuedId = enqueue(7);

  auto delivered = registry.beginTsoDelivery(
      L"exercise",
      receiver.membership->id,
      rti1516_2025::HLAinteger64Time(5),
      true);
  REQUIRE(delivered.delivery.status == umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(delivered.delivery.messages.size() == 1U);
  REQUIRE(delivered.delivery.messages.front().messageId == deliveredId);
  REQUIRE(registry.completeTsoDelivery(
      L"exercise", delivered.delivery.messages.front()).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);

  auto inTransit = registry.beginTsoDelivery(
      L"exercise",
      receiver.membership->id,
      rti1516_2025::HLAinteger64Time(6),
      true);
  REQUIRE(inTransit.delivery.status == umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(inTransit.delivery.messages.size() == 1U);
  REQUIRE(inTransit.delivery.messages.front().messageId == inTransitId);

  auto requested = registry.requestFederationSave(
      L"exercise", receiver.membership->id, L"phase-snapshot");
  REQUIRE(requested.status == umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(requested.notifications.size() == 2U);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", receiver.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", observer.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(registry.federateSaveComplete(
      L"exercise", receiver.membership->id).saveCompletedSuccessfully);
  REQUIRE(registry.federateSaveComplete(
      L"exercise", observer.membership->id).saveCompletedSuccessfully);

  auto const commits = store->snapshotCommits();
  REQUIRE(commits.size() == 1U);
  auto const image = umbra::detail::FederationStateImageCodec::decode(
      commits.front().stateImage);
  REQUIRE(image.tsoQueueEntries.size() == 3U);
  auto const phaseFor = [&](std::uint64_t messageId) {
    auto const entry = std::find_if(
        image.tsoQueueEntries.begin(),
        image.tsoQueueEntries.end(),
        [messageId](umbra::detail::FederationStateImageTsoQueueEntry const& candidate) {
          return candidate.messageId == messageId;
        });
    REQUIRE(entry != image.tsoQueueEntries.end());
    REQUIRE(entry->recipientFederateId == receiver.membership->id);
    return entry->phase;
  };
  REQUIRE(phaseFor(deliveredId) == 2U);
  REQUIRE(phaseFor(inTransitId) == 1U);
  REQUIRE(phaseFor(queuedId) == 0U);
}

TEST_CASE(
    "Federation restore rehydrates the saved TSO queue phases",
    "[unit][kernel][federation-registry][save-restore][durable-save][restore][tso-queue-state]") {
  auto store = std::make_shared<umbra::detail::MemoryFederationSaveCommitStore>();
  EmbeddedFederationRegistry registry({}, store);
  REQUIRE(registry.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);

  auto receiverTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  auto receiver = registry.joinWithTimeState(
      L"exercise",
      receiverTime,
      L"receiver",
      L"receiver",
      noOpCallbackRoute());
  auto observer = registry.join(
      L"exercise",
      L"observer",
      L"observer",
      noOpCallbackRoute());
  REQUIRE(receiver.membership);
  REQUIRE(observer.membership);

  auto enqueue = [&](std::int64_t timestamp) {
    auto const allocated = registry.allocateTsoMessageId(L"exercise");
    REQUIRE(allocated.status == umbra::detail::FederationTsoRegistryStatus::applied);
    auto const enqueued = registry.enqueueTsoMessage(
        L"exercise",
        allocated.messageId,
        receiver.membership->id,
        std::make_shared<rti1516_2025::HLAinteger64Time>(timestamp));
    REQUIRE(enqueued.status == umbra::detail::FederationTsoRegistryStatus::applied);
    REQUIRE(enqueued.queueStatus == umbra::detail::TsoMessageQueueStatus::applied);
    return allocated.messageId;
  };

  auto const deliveredId = enqueue(5);
  auto const inTransitId = enqueue(6);
  auto const queuedId = enqueue(7);

  auto delivered = registry.beginTsoDelivery(
      L"exercise",
      receiver.membership->id,
      rti1516_2025::HLAinteger64Time(5),
      true);
  REQUIRE(delivered.delivery.status == umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(delivered.delivery.messages.size() == 1U);
  REQUIRE(delivered.delivery.messages.front().messageId == deliveredId);
  auto const deliveredMessage = delivered.delivery.messages.front();
  REQUIRE(registry.completeTsoDelivery(L"exercise", deliveredMessage).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);

  auto inTransit = registry.beginTsoDelivery(
      L"exercise",
      receiver.membership->id,
      rti1516_2025::HLAinteger64Time(6),
      true);
  REQUIRE(inTransit.delivery.status == umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(inTransit.delivery.messages.size() == 1U);
  REQUIRE(inTransit.delivery.messages.front().messageId == inTransitId);
  auto const inTransitMessage = inTransit.delivery.messages.front();

  REQUIRE(registry.requestFederationSave(
      L"exercise", receiver.membership->id, L"phase-restore").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", receiver.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", observer.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(registry.federateSaveComplete(
      L"exercise", receiver.membership->id).saveCompletedSuccessfully);
  REQUIRE(registry.federateSaveComplete(
      L"exercise", observer.membership->id).saveCompletedSuccessfully);

  // Mutate the live queue after the save boundary. A successful restore must
  // replace this post-save state with the three phase records captured above.
  REQUIRE(registry.completeTsoDelivery(L"exercise", inTransitMessage).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  auto const postSave = enqueue(8);
  auto postSaveDelivery = registry.beginTsoDelivery(
      L"exercise",
      receiver.membership->id,
      rti1516_2025::HLAinteger64Time(8),
      true);
  REQUIRE(postSaveDelivery.delivery.status == umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(postSaveDelivery.delivery.messages.size() == 2U);
  REQUIRE(std::any_of(
      postSaveDelivery.delivery.messages.begin(),
      postSaveDelivery.delivery.messages.end(),
      [queuedId](umbra::detail::TsoQueuedMessage const& message) {
        return message.messageId == queuedId;
      }));
  REQUIRE(std::any_of(
      postSaveDelivery.delivery.messages.begin(),
      postSaveDelivery.delivery.messages.end(),
      [postSave](umbra::detail::TsoQueuedMessage const& message) {
        return message.messageId == postSave;
      }));

  auto restore = registry.requestFederationRestore(
      L"exercise", receiver.membership->id, L"phase-restore");
  REQUIRE(restore.status == umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(registry.federateRestoreComplete(
      L"exercise", receiver.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = registry.federateRestoreComplete(
      L"exercise", observer.membership->id);
  REQUIRE(restored.status == umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.notifications.size() == 2U);
  REQUIRE(std::all_of(
      restored.notifications.begin(),
      restored.notifications.end(),
      [](umbra::detail::FederationRestoreNotification const& notification) {
        return notification.successful;
      }));

  auto restoredInTransit = registry.completeTsoDelivery(
      L"exercise", inTransitMessage);
  REQUIRE(restoredInTransit.delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  auto restoredDelivered = registry.completeTsoDelivery(
      L"exercise", deliveredMessage);
  REQUIRE(restoredDelivered.delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::message_already_completed);

  auto restoredQueued = registry.beginTsoDelivery(
      L"exercise",
      receiver.membership->id,
      rti1516_2025::HLAinteger64Time(7),
      true);
  REQUIRE(restoredQueued.delivery.status == umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(restoredQueued.delivery.messages.size() == 1U);
  REQUIRE(restoredQueued.delivery.messages.front().messageId == queuedId);
  REQUIRE(registry.completeTsoDelivery(
      L"exercise", restoredQueued.delivery.messages.front()).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);

  auto const noPostSaveMessage = registry.beginTsoDelivery(
      L"exercise",
      receiver.membership->id,
      rti1516_2025::HLAinteger64Time(8),
      true);
  REQUIRE(noPostSaveMessage.delivery.status == umbra::detail::FederationTsoDeliveryStatus::no_messages);
}

TEST_CASE(
    "Federation restore rehydrates timestamped application payloads from the durable image",
    "[unit][kernel][federation-registry][save-restore][durable-save][restore][tso-payload-state][tso-interaction-state][tso-directed-interaction-state][tso-attribute-update-state]") {
  auto store = std::make_shared<umbra::detail::MemoryFederationSaveCommitStore>();
  EmbeddedFederationRegistry registry({}, store);
  REQUIRE(registry.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);

  auto receiverTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  auto producer = registry.join(
      L"exercise", L"producer", L"producer", noOpCallbackRoute());
  auto receiver = registry.joinWithTimeState(
      L"exercise",
      receiverTime,
      L"receiver",
      L"receiver",
      noOpCallbackRoute());
  REQUIRE(producer.membership);
  REQUIRE(receiver.membership);

  umbra::detail::TsoInteractionMessage interaction;
  interaction.producingFederateId = producer.membership->id;
  interaction.sentInteractionClassHandle = 1U;
  interaction.sentParameterHandles = {2U};
  interaction.parameters = {{
      2U,
      rti1516_2025::VariableLengthData(
          std::string{"\x10\x11", 2U}.data(),
          2U),
  }};
  interaction.userSuppliedTag = rti1516_2025::VariableLengthData(
      std::string{"ordinary", 8U}.data(),
      8U);
  interaction.transportationName = "HLAreliable";
  interaction.timestamp =
      std::make_shared<rti1516_2025::HLAinteger64Time>(7);
  auto const ordinary = registry.enqueueTsoInteraction(
      L"exercise",
      std::move(interaction),
      {receiver.membership->id},
      {receiver.membership->id});
  REQUIRE(ordinary.status == umbra::detail::FederationTsoRegistryStatus::applied);

  umbra::detail::TsoDirectedInteractionMessage directed;
  directed.producingFederateId = producer.membership->id;
  directed.objectInstanceHandle = 91U;
  directed.sentInteractionClassHandle = 3U;
  directed.sentParameterHandles = {4U};
  directed.parameters = {{
      4U,
      rti1516_2025::VariableLengthData(
          std::string{"\x20\x21", 2U}.data(),
          2U),
  }};
  directed.userSuppliedTag = rti1516_2025::VariableLengthData(
      std::string{"directed", 8U}.data(),
      8U);
  directed.transportationName = "HLAreliable";
  directed.recipients = {{
      receiver.membership->id,
      91U,
      3U,
      {4U},
      noOpCallbackRoute(),
  }};
  directed.timestamp =
      std::make_shared<rti1516_2025::HLAinteger64Time>(8);
  auto const directedResult = registry.enqueueTsoDirectedInteraction(
      L"exercise",
      std::move(directed),
      {receiver.membership->id});
  REQUIRE(directedResult.status ==
      umbra::detail::FederationTsoRegistryStatus::applied);

  umbra::detail::TsoAttributeUpdateMessage attributeUpdate;
  attributeUpdate.producingFederateId = producer.membership->id;
  attributeUpdate.objectInstanceHandle = 92U;
  attributeUpdate.attributes = {{
      5U,
      rti1516_2025::VariableLengthData(
          std::string{"\x30\x31", 2U}.data(),
          2U),
  }};
  attributeUpdate.userSuppliedTag = rti1516_2025::VariableLengthData(
      std::string{"attribute", 9U}.data(),
      9U);
  umbra::detail::TsoAttributeUpdatePassel passel;
  passel.transportationName = "HLAreliable";
  passel.sentAttributeHandles = {5U};
  passel.preferredOrderType = rti1516_2025::TIMESTAMP;
  attributeUpdate.passelsByRecipient.emplace(
      receiver.membership->id,
      std::vector<umbra::detail::TsoAttributeUpdatePassel>{passel});
  attributeUpdate.timestamp =
      std::make_shared<rti1516_2025::HLAinteger64Time>(9);
  auto const attributeResult = registry.enqueueTsoAttributeUpdate(
      L"exercise",
      std::move(attributeUpdate),
      {receiver.membership->id},
      {receiver.membership->id});
  REQUIRE(attributeResult.status ==
      umbra::detail::FederationTsoRegistryStatus::applied);

  REQUIRE(registry.requestFederationSave(
      L"exercise", receiver.membership->id, L"payload-restore").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", receiver.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", producer.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(registry.federateSaveComplete(
      L"exercise", receiver.membership->id).saveCompletedSuccessfully);
  REQUIRE(registry.federateSaveComplete(
      L"exercise", producer.membership->id).saveCompletedSuccessfully);

  // Consume the live queue after the save. Restore must put all payloads back
  // from the durable image rather than retaining this post-save delivery state.
  auto delivered = registry.beginTsoPayloadDelivery(
      L"exercise",
      receiver.membership->id,
      rti1516_2025::HLAinteger64Time(9),
      true);
  REQUIRE(delivered.deliveryStatus ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(delivered.deliveries.size() == 3U);
  for (auto const& payload : delivered.deliveries) {
    std::visit(
        [&](auto const& typed) {
          REQUIRE(registry.completeTsoDelivery(
              L"exercise", typed.queuedMessage).delivery.status ==
              umbra::detail::FederationTsoDeliveryStatus::applied);
        },
        payload);
  }

  REQUIRE(registry.requestFederationRestore(
      L"exercise", receiver.membership->id, L"payload-restore").status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(registry.federateRestoreComplete(
      L"exercise", receiver.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(registry.federateRestoreComplete(
      L"exercise", producer.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  auto restored = registry.beginTsoPayloadDelivery(
      L"exercise",
      receiver.membership->id,
      rti1516_2025::HLAinteger64Time(9),
      true);
  REQUIRE(restored.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(restored.deliveryStatus ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(restored.deliveries.size() == 3U);

  auto const interactionIt = std::find_if(
      restored.deliveries.begin(),
      restored.deliveries.end(),
      [](umbra::detail::TsoPayloadDelivery const& payload) {
        return std::holds_alternative<umbra::detail::TsoInteractionDelivery>(payload);
      });
  REQUIRE(interactionIt != restored.deliveries.end());
  auto const* restoredInteraction =
      std::get_if<umbra::detail::TsoInteractionDelivery>(&*interactionIt);
  REQUIRE(restoredInteraction != nullptr);
  REQUIRE(restoredInteraction->message.messageId == ordinary.messageId);
  REQUIRE(restoredInteraction->message.parameters.size() == 1U);
  REQUIRE(restoredInteraction->message.parameters.front().first == 2U);
  REQUIRE(restoredInteraction->message.parameters.front().second.size() == 2U);
  REQUIRE(std::string(
              static_cast<char const*>(
                  restoredInteraction->message.parameters.front().second.data()),
              2U) == std::string{"\x10\x11", 2U});
  REQUIRE(restoredInteraction->message.userSuppliedTag.size() == 8U);
  REQUIRE(restoredInteraction->message.timestamp->implementationName() ==
      L"HLAinteger64Time");

  auto const directedIt = std::find_if(
      restored.deliveries.begin(),
      restored.deliveries.end(),
      [](umbra::detail::TsoPayloadDelivery const& payload) {
        return std::holds_alternative<umbra::detail::TsoDirectedInteractionDelivery>(payload);
      });
  REQUIRE(directedIt != restored.deliveries.end());
  auto const* restoredDirected =
      std::get_if<umbra::detail::TsoDirectedInteractionDelivery>(&*directedIt);
  REQUIRE(restoredDirected != nullptr);
  REQUIRE(restoredDirected->message.messageId == directedResult.messageId);
  REQUIRE(restoredDirected->message.recipients.size() == 1U);
  REQUIRE(restoredDirected->message.recipients.front().receivingFederateId ==
      receiver.membership->id);
  REQUIRE(restoredDirected->message.recipients.front().callbackRoute);
  REQUIRE(restoredDirected->message.parameters.front().second.size() == 2U);
  REQUIRE(std::string(
              static_cast<char const*>(
                  restoredDirected->message.parameters.front().second.data()),
              2U) == std::string{"\x20\x21", 2U});

  auto const attributeIt = std::find_if(
      restored.deliveries.begin(),
      restored.deliveries.end(),
      [](umbra::detail::TsoPayloadDelivery const& payload) {
        return std::holds_alternative<umbra::detail::TsoAttributeUpdateDelivery>(payload);
      });
  REQUIRE(attributeIt != restored.deliveries.end());
  auto const* restoredAttribute =
      std::get_if<umbra::detail::TsoAttributeUpdateDelivery>(&*attributeIt);
  REQUIRE(restoredAttribute != nullptr);
  REQUIRE(restoredAttribute->message.messageId == attributeResult.messageId);
  REQUIRE(restoredAttribute->message.attributes.size() == 1U);
  REQUIRE(restoredAttribute->message.attributes.front().first == 5U);
  REQUIRE(restoredAttribute->message.attributes.front().second.size() == 2U);
  REQUIRE(std::string(
              static_cast<char const*>(
                  restoredAttribute->message.attributes.front().second.data()),
              2U) == std::string{"\x30\x31", 2U});
  REQUIRE(restoredAttribute->message.passelsByRecipient.size() == 1U);
  REQUIRE(restoredAttribute->message.passelsByRecipient.begin()->second.size() == 1U);
  REQUIRE(restoredAttribute->message.passelsByRecipient.begin()->second.front()
              .sentAttributeHandles == std::vector<std::uint64_t>{5U});

  for (auto const& payload : restored.deliveries) {
    std::visit(
        [&](auto const& typed) {
          REQUIRE(registry.completeTsoDelivery(
              L"exercise", typed.queuedMessage).delivery.status ==
              umbra::detail::FederationTsoDeliveryStatus::applied);
        },
        payload);
  }
}

TEST_CASE(
    "Filesystem fresh-registry restore rehydrates one timestamped interaction payload across queued and in-transit recipients",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][tso-queue-state][tso-payload-state][tso-interaction-state]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);

  auto producer = source.join(
      L"exercise", L"producer", L"producer", noOpCallbackRoute());
  auto receiverATime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  auto receiverA = source.joinWithTimeState(
      L"exercise",
      receiverATime,
      L"receiver-a",
      L"receiver-a",
      noOpCallbackRoute());
  auto receiverBTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  auto receiverB = source.joinWithTimeState(
      L"exercise",
      receiverBTime,
      L"receiver-b",
      L"receiver-b",
      noOpCallbackRoute());
  REQUIRE(producer.membership);
  REQUIRE(receiverA.membership);
  REQUIRE(receiverB.membership);

  std::string const payloadBytes{"\x40\x41", 2U};
  std::string const tagBytes{"filesystem", 10U};
  umbra::detail::TsoInteractionMessage interaction;
  interaction.producingFederateId = producer.membership->id;
  interaction.sentInteractionClassHandle = 1U;
  interaction.sentParameterHandles = {2U};
  interaction.parameters = {{
      2U,
      rti1516_2025::VariableLengthData(payloadBytes.data(), payloadBytes.size()),
  }};
  interaction.userSuppliedTag = rti1516_2025::VariableLengthData(
      tagBytes.data(), tagBytes.size());
  interaction.transportationName = "HLAreliable";
  interaction.timestamp =
      std::make_shared<rti1516_2025::HLAinteger64Time>(5);
  auto const enqueued = source.enqueueTsoInteraction(
      L"exercise",
      std::move(interaction),
      {receiverA.membership->id, receiverB.membership->id},
      {receiverA.membership->id, receiverB.membership->id});
  REQUIRE(enqueued.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(enqueued.queueStatus == umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(enqueued.enqueuedRecipientCount == 2U);

  // Begin only receiver A's delivery and hold it in transit. Receiver B's
  // queue entry remains queued at the same timestamp when the durable image is
  // captured.
  auto inTransit = source.beginTsoPayloadDelivery(
      L"exercise",
      receiverA.membership->id,
      rti1516_2025::HLAinteger64Time(5),
      true);
  REQUIRE(inTransit.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(inTransit.deliveryStatus ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(inTransit.deliveries.size() == 1U);
  auto const* inTransitInteraction =
      std::get_if<umbra::detail::TsoInteractionDelivery>(
          &inTransit.deliveries.front());
  REQUIRE(inTransitInteraction != nullptr);
  REQUIRE(inTransitInteraction->message.messageId == enqueued.messageId);
  auto const inTransitMessage = inTransitInteraction->queuedMessage;

  REQUIRE(source.requestFederationSave(
      L"exercise",
      receiverA.membership->id,
      L"tso-payload-process-restart")
      .status == umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", receiverA.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", receiverB.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", producer.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", receiverA.membership->id).saveCompletedSuccessfully);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", receiverB.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", producer.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", L"tso-payload-process-restart");
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.tsoInteractionMessages.size() == 1U);
  REQUIRE(image.tsoInteractionMessages.front().messageId == enqueued.messageId);
  REQUIRE(image.tsoInteractionMessages.front().parameters.size() == 1U);
  REQUIRE(image.tsoInteractionMessages.front().parameters.front().value ==
      payloadBytes);
  REQUIRE(image.tsoInteractionMessages.front().userSuppliedTag == tagBytes);
  REQUIRE(image.tsoInteractionMessages.front().timestampEncoding.has_value());
  REQUIRE(image.tsoRequestRetractionRecords.size() == 1U);
  REQUIRE(image.tsoRequestRetractionRecords.front().recipientStates.size() == 2U);
  REQUIRE(image.tsoQueueEntries.size() == 2U);
  auto const phaseFor = [&](std::uint64_t recipientId) {
    auto const entry = std::find_if(
        image.tsoQueueEntries.begin(),
        image.tsoQueueEntries.end(),
        [recipientId, messageId = enqueued.messageId](
            umbra::detail::FederationStateImageTsoQueueEntry const& candidate) {
          return candidate.messageId == messageId &&
              candidate.recipientFederateId == recipientId;
        });
    REQUIRE(entry != image.tsoQueueEntries.end());
    return entry->phase;
  };
  REQUIRE(phaseFor(receiverA.membership->id) == 1U);
  REQUIRE(phaseFor(receiverB.membership->id) == 0U);

  // Rebuild the federation from the durable file in a new registry. Joining in
  // the original order gives the route-free image the same federate identities,
  // while each new member supplies a fresh callback/time-state route.
  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedProducer = restarted.join(
      L"exercise", L"producer", L"producer", noOpCallbackRoute());
  auto restartedReceiverATime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  auto restartedReceiverA = restarted.joinWithTimeState(
      L"exercise",
      restartedReceiverATime,
      L"receiver-a",
      L"receiver-a",
      noOpCallbackRoute());
  auto restartedReceiverBTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  auto restartedReceiverB = restarted.joinWithTimeState(
      L"exercise",
      restartedReceiverBTime,
      L"receiver-b",
      L"receiver-b",
      noOpCallbackRoute());
  REQUIRE(restartedProducer.membership);
  REQUIRE(restartedReceiverA.membership);
  REQUIRE(restartedReceiverB.membership);
  REQUIRE(restartedProducer.membership->id == producer.membership->id);
  REQUIRE(restartedReceiverA.membership->id == receiverA.membership->id);
  REQUIRE(restartedReceiverB.membership->id == receiverB.membership->id);

  REQUIRE(restarted.requestFederationRestore(
      L"exercise",
      restartedReceiverA.membership->id,
      L"tso-payload-process-restart")
      .status == umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", restartedReceiverA.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", restartedReceiverB.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", restartedProducer.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  // The saved in-transit record is completed using its durable identity; the
  // other recipient can then consume the restored payload from its queued
  // phase, including the original bytes, tag, order, transport, and timestamp.
  REQUIRE(restarted.completeTsoDelivery(
      L"exercise", inTransitMessage).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  auto restored = restarted.beginTsoPayloadDelivery(
      L"exercise",
      restartedReceiverB.membership->id,
      rti1516_2025::HLAinteger64Time(5),
      true);
  REQUIRE(restored.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(restored.deliveryStatus ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(restored.deliveries.size() == 1U);
  auto const* restoredInteraction =
      std::get_if<umbra::detail::TsoInteractionDelivery>(
          &restored.deliveries.front());
  REQUIRE(restoredInteraction != nullptr);
  REQUIRE(restoredInteraction->message.messageId == enqueued.messageId);
  REQUIRE(restoredInteraction->message.parameters.size() == 1U);
  REQUIRE(restoredInteraction->message.parameters.front().second.size() ==
      payloadBytes.size());
  REQUIRE(std::string(
              static_cast<char const*>(
                  restoredInteraction->message.parameters.front().second.data()),
              payloadBytes.size()) == payloadBytes);
  REQUIRE(restoredInteraction->message.userSuppliedTag.size() == tagBytes.size());
  REQUIRE(restoredInteraction->message.transportationName == "HLAreliable");
  REQUIRE(restoredInteraction->message.timestamp->implementationName() ==
      L"HLAinteger64Time");
  auto const* restoredTimestamp =
      dynamic_cast<rti1516_2025::HLAinteger64Time const*>(
          restoredInteraction->message.timestamp.get());
  REQUIRE(restoredTimestamp != nullptr);
  REQUIRE(restoredTimestamp->getTime() == 5);
  REQUIRE(restarted.completeTsoDelivery(
      L"exercise", restoredInteraction->queuedMessage).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(restarted.beginTsoPayloadDelivery(
      L"exercise",
      restartedReceiverB.membership->id,
      rti1516_2025::HLAinteger64Time(5),
      true).deliveryStatus == umbra::detail::FederationTsoDeliveryStatus::no_messages);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem fresh-registry restore rehydrates one timestamped directed interaction payload across queued and in-transit recipients",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][tso-queue-state][tso-payload-state][tso-directed-interaction-state]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);

  auto producer = source.join(
      L"exercise", L"producer", L"producer", noOpCallbackRoute());
  auto receiverATime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  auto receiverA = source.joinWithTimeState(
      L"exercise",
      receiverATime,
      L"receiver-a",
      L"receiver-a",
      noOpCallbackRoute());
  auto receiverBTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  auto receiverB = source.joinWithTimeState(
      L"exercise",
      receiverBTime,
      L"receiver-b",
      L"receiver-b",
      noOpCallbackRoute());
  REQUIRE(producer.membership);
  REQUIRE(receiverA.membership);
  REQUIRE(receiverB.membership);

  std::string const payloadBytes{"\x50\x51", 2U};
  std::string const tagBytes{"directedfs", 10U};
  umbra::detail::TsoDirectedInteractionMessage directed;
  directed.producingFederateId = producer.membership->id;
  directed.objectInstanceHandle = 91U;
  directed.sentInteractionClassHandle = 3U;
  directed.sentParameterHandles = {4U};
  directed.parameters = {{
      4U,
      rti1516_2025::VariableLengthData(payloadBytes.data(), payloadBytes.size()),
  }};
  directed.userSuppliedTag = rti1516_2025::VariableLengthData(
      tagBytes.data(), tagBytes.size());
  directed.transportationName = "HLAreliable";
  directed.recipients = {{
      receiverA.membership->id,
      91U,
      3U,
      {4U},
      noOpCallbackRoute(),
  }, {
      receiverB.membership->id,
      91U,
      3U,
      {4U},
      noOpCallbackRoute(),
  }};
  directed.timestamp =
      std::make_shared<rti1516_2025::HLAinteger64Time>(5);
  auto const enqueued = source.enqueueTsoDirectedInteraction(
      L"exercise",
      std::move(directed),
      {receiverA.membership->id, receiverB.membership->id});
  REQUIRE(enqueued.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(enqueued.queueStatus == umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(enqueued.enqueuedRecipientCount == 2U);

  auto inTransit = source.beginTsoPayloadDelivery(
      L"exercise",
      receiverA.membership->id,
      rti1516_2025::HLAinteger64Time(5),
      true);
  REQUIRE(inTransit.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(inTransit.deliveryStatus ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(inTransit.deliveries.size() == 1U);
  auto const* inTransitDirected =
      std::get_if<umbra::detail::TsoDirectedInteractionDelivery>(
          &inTransit.deliveries.front());
  REQUIRE(inTransitDirected != nullptr);
  REQUIRE(inTransitDirected->message.messageId == enqueued.messageId);
  auto const inTransitMessage = inTransitDirected->queuedMessage;

  REQUIRE(source.requestFederationSave(
      L"exercise",
      receiverA.membership->id,
      L"directed-tso-payload-process-restart")
      .status == umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", receiverA.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", receiverB.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", producer.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", receiverA.membership->id).saveCompletedSuccessfully);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", receiverB.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", producer.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", L"directed-tso-payload-process-restart");
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.tsoDirectedInteractionMessages.size() == 1U);
  REQUIRE(image.tsoDirectedInteractionMessages.front().messageId == enqueued.messageId);
  REQUIRE(image.tsoDirectedInteractionMessages.front().objectInstanceHandle == 91U);
  REQUIRE(image.tsoDirectedInteractionMessages.front().parameters.size() == 1U);
  REQUIRE(image.tsoDirectedInteractionMessages.front().parameters.front().value ==
      payloadBytes);
  REQUIRE(image.tsoDirectedInteractionMessages.front().userSuppliedTag == tagBytes);
  REQUIRE(image.tsoDirectedInteractionMessages.front().recipients.size() == 2U);
  REQUIRE(image.tsoRequestRetractionRecords.size() == 1U);
  REQUIRE(image.tsoRequestRetractionRecords.front().recipientStates.size() == 2U);
  REQUIRE(image.tsoQueueEntries.size() == 2U);
  auto const phaseFor = [&](std::uint64_t recipientId) {
    auto const entry = std::find_if(
        image.tsoQueueEntries.begin(),
        image.tsoQueueEntries.end(),
        [recipientId, messageId = enqueued.messageId](
            umbra::detail::FederationStateImageTsoQueueEntry const& candidate) {
          return candidate.messageId == messageId &&
              candidate.recipientFederateId == recipientId;
        });
    REQUIRE(entry != image.tsoQueueEntries.end());
    return entry->phase;
  };
  REQUIRE(phaseFor(receiverA.membership->id) == 1U);
  REQUIRE(phaseFor(receiverB.membership->id) == 0U);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedProducer = restarted.join(
      L"exercise", L"producer", L"producer", noOpCallbackRoute());
  auto restartedReceiverATime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  auto restartedReceiverA = restarted.joinWithTimeState(
      L"exercise",
      restartedReceiverATime,
      L"receiver-a",
      L"receiver-a",
      noOpCallbackRoute());
  auto restartedReceiverBTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  auto restartedReceiverB = restarted.joinWithTimeState(
      L"exercise",
      restartedReceiverBTime,
      L"receiver-b",
      L"receiver-b",
      noOpCallbackRoute());
  REQUIRE(restartedProducer.membership);
  REQUIRE(restartedReceiverA.membership);
  REQUIRE(restartedReceiverB.membership);
  REQUIRE(restartedProducer.membership->id == producer.membership->id);
  REQUIRE(restartedReceiverA.membership->id == receiverA.membership->id);
  REQUIRE(restartedReceiverB.membership->id == receiverB.membership->id);

  REQUIRE(restarted.requestFederationRestore(
      L"exercise",
      restartedReceiverA.membership->id,
      L"directed-tso-payload-process-restart")
      .status == umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", restartedReceiverA.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", restartedReceiverB.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", restartedProducer.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  REQUIRE(restarted.completeTsoDelivery(
      L"exercise", inTransitMessage).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  auto restored = restarted.beginTsoPayloadDelivery(
      L"exercise",
      restartedReceiverB.membership->id,
      rti1516_2025::HLAinteger64Time(5),
      true);
  REQUIRE(restored.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(restored.deliveryStatus ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(restored.deliveries.size() == 1U);
  auto const* restoredDirected =
      std::get_if<umbra::detail::TsoDirectedInteractionDelivery>(
          &restored.deliveries.front());
  REQUIRE(restoredDirected != nullptr);
  REQUIRE(restoredDirected->message.messageId == enqueued.messageId);
  REQUIRE(restoredDirected->message.objectInstanceHandle == 91U);
  REQUIRE(restoredDirected->message.parameters.size() == 1U);
  REQUIRE(restoredDirected->message.parameters.front().second.size() ==
      payloadBytes.size());
  REQUIRE(std::string(
              static_cast<char const*>(
                  restoredDirected->message.parameters.front().second.data()),
              payloadBytes.size()) == payloadBytes);
  REQUIRE(restoredDirected->message.userSuppliedTag.size() == tagBytes.size());
  REQUIRE(restoredDirected->message.transportationName == "HLAreliable");
  REQUIRE(restoredDirected->message.recipients.size() == 2U);
  auto const recipient = std::find_if(
      restoredDirected->message.recipients.begin(),
      restoredDirected->message.recipients.end(),
      [id = restartedReceiverB.membership->id](
          umbra::detail::TsoDirectedInteractionRecipient const& candidate) {
        return candidate.receivingFederateId == id;
      });
  REQUIRE(recipient != restoredDirected->message.recipients.end());
  REQUIRE(recipient->objectInstanceHandle == 91U);
  REQUIRE(recipient->receivedInteractionClassHandle == 3U);
  REQUIRE(recipient->receivedParameterHandles == std::set<std::uint64_t>{4U});
  REQUIRE(recipient->callbackRoute);
  REQUIRE(restoredDirected->message.timestamp->implementationName() ==
      L"HLAinteger64Time");
  auto const* restoredTimestamp =
      dynamic_cast<rti1516_2025::HLAinteger64Time const*>(
          restoredDirected->message.timestamp.get());
  REQUIRE(restoredTimestamp != nullptr);
  REQUIRE(restoredTimestamp->getTime() == 5);
  REQUIRE(restarted.completeTsoDelivery(
      L"exercise", restoredDirected->queuedMessage).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(restarted.beginTsoPayloadDelivery(
      L"exercise",
      restartedReceiverB.membership->id,
      rti1516_2025::HLAinteger64Time(5),
      true).deliveryStatus == umbra::detail::FederationTsoDeliveryStatus::no_messages);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem fresh-registry restore rehydrates one timestamped attribute-update payload across queued and in-transit recipients",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][tso-queue-state][tso-payload-state][tso-attribute-update-state]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);

  auto producer = source.join(
      L"exercise", L"producer", L"producer", noOpCallbackRoute());
  auto receiverATime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  auto receiverA = source.joinWithTimeState(
      L"exercise",
      receiverATime,
      L"receiver-a",
      L"receiver-a",
      noOpCallbackRoute());
  auto receiverBTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  auto receiverB = source.joinWithTimeState(
      L"exercise",
      receiverBTime,
      L"receiver-b",
      L"receiver-b",
      noOpCallbackRoute());
  REQUIRE(producer.membership);
  REQUIRE(receiverA.membership);
  REQUIRE(receiverB.membership);

  std::string const payloadBytes{"\x60\x61", 2U};
  std::string const tagBytes{"attrfs", 6U};
  umbra::detail::TsoAttributeUpdateMessage attributeUpdate;
  attributeUpdate.producingFederateId = producer.membership->id;
  attributeUpdate.objectInstanceHandle = 92U;
  attributeUpdate.attributes = {{
      5U,
      rti1516_2025::VariableLengthData(payloadBytes.data(), payloadBytes.size()),
  }};
  attributeUpdate.userSuppliedTag = rti1516_2025::VariableLengthData(
      tagBytes.data(), tagBytes.size());
  attributeUpdate.timestamp =
      std::make_shared<rti1516_2025::HLAinteger64Time>(5);
  for (auto const recipientId : {
           receiverA.membership->id,
           receiverB.membership->id,
       }) {
    umbra::detail::TsoAttributeUpdatePassel passel;
    passel.transportationName = "HLAreliable";
    passel.sentAttributeHandles = {5U};
    passel.preferredOrderType = rti1516_2025::TIMESTAMP;
    attributeUpdate.passelsByRecipient.emplace(recipientId, std::vector<
        umbra::detail::TsoAttributeUpdatePassel>{std::move(passel)});
  }
  auto const enqueued = source.enqueueTsoAttributeUpdate(
      L"exercise",
      std::move(attributeUpdate),
      {receiverA.membership->id, receiverB.membership->id},
      {receiverA.membership->id, receiverB.membership->id});
  REQUIRE(enqueued.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(enqueued.queueStatus == umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(enqueued.enqueuedRecipientCount == 2U);

  auto inTransit = source.beginTsoPayloadDelivery(
      L"exercise",
      receiverA.membership->id,
      rti1516_2025::HLAinteger64Time(5),
      true);
  REQUIRE(inTransit.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(inTransit.deliveryStatus ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(inTransit.deliveries.size() == 1U);
  auto const* inTransitAttribute =
      std::get_if<umbra::detail::TsoAttributeUpdateDelivery>(
          &inTransit.deliveries.front());
  REQUIRE(inTransitAttribute != nullptr);
  REQUIRE(inTransitAttribute->message.messageId == enqueued.messageId);
  auto const inTransitMessage = inTransitAttribute->queuedMessage;

  REQUIRE(source.requestFederationSave(
      L"exercise",
      receiverA.membership->id,
      L"attribute-tso-payload-process-restart")
      .status == umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", receiverA.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", receiverB.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", producer.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", receiverA.membership->id).saveCompletedSuccessfully);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", receiverB.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", producer.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", L"attribute-tso-payload-process-restart");
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.tsoAttributeUpdateMessages.size() == 1U);
  REQUIRE(image.tsoAttributeUpdateMessages.front().messageId == enqueued.messageId);
  REQUIRE(image.tsoAttributeUpdateMessages.front().objectInstanceHandle == 92U);
  REQUIRE(image.tsoAttributeUpdateMessages.front().attributes.size() == 1U);
  REQUIRE(image.tsoAttributeUpdateMessages.front().attributes.front().value ==
      payloadBytes);
  REQUIRE(image.tsoAttributeUpdateMessages.front().userSuppliedTag == tagBytes);
  REQUIRE(image.tsoAttributeUpdateMessages.front().passelsByRecipient.size() == 2U);
  REQUIRE(image.tsoRequestRetractionRecords.size() == 1U);
  REQUIRE(image.tsoRequestRetractionRecords.front().recipientStates.size() == 2U);
  REQUIRE(image.tsoQueueEntries.size() == 2U);
  auto const phaseFor = [&](std::uint64_t recipientId) {
    auto const entry = std::find_if(
        image.tsoQueueEntries.begin(),
        image.tsoQueueEntries.end(),
        [recipientId, messageId = enqueued.messageId](
            umbra::detail::FederationStateImageTsoQueueEntry const& candidate) {
          return candidate.messageId == messageId &&
              candidate.recipientFederateId == recipientId;
        });
    REQUIRE(entry != image.tsoQueueEntries.end());
    return entry->phase;
  };
  REQUIRE(phaseFor(receiverA.membership->id) == 1U);
  REQUIRE(phaseFor(receiverB.membership->id) == 0U);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedProducer = restarted.join(
      L"exercise", L"producer", L"producer", noOpCallbackRoute());
  auto restartedReceiverATime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  auto restartedReceiverA = restarted.joinWithTimeState(
      L"exercise",
      restartedReceiverATime,
      L"receiver-a",
      L"receiver-a",
      noOpCallbackRoute());
  auto restartedReceiverBTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  auto restartedReceiverB = restarted.joinWithTimeState(
      L"exercise",
      restartedReceiverBTime,
      L"receiver-b",
      L"receiver-b",
      noOpCallbackRoute());
  REQUIRE(restartedProducer.membership);
  REQUIRE(restartedReceiverA.membership);
  REQUIRE(restartedReceiverB.membership);
  REQUIRE(restartedProducer.membership->id == producer.membership->id);
  REQUIRE(restartedReceiverA.membership->id == receiverA.membership->id);
  REQUIRE(restartedReceiverB.membership->id == receiverB.membership->id);

  REQUIRE(restarted.requestFederationRestore(
      L"exercise",
      restartedReceiverA.membership->id,
      L"attribute-tso-payload-process-restart")
      .status == umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", restartedReceiverA.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", restartedReceiverB.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", restartedProducer.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  REQUIRE(restarted.completeTsoDelivery(
      L"exercise", inTransitMessage).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  auto restored = restarted.beginTsoPayloadDelivery(
      L"exercise",
      restartedReceiverB.membership->id,
      rti1516_2025::HLAinteger64Time(5),
      true);
  REQUIRE(restored.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(restored.deliveryStatus ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(restored.deliveries.size() == 1U);
  auto const* restoredAttribute =
      std::get_if<umbra::detail::TsoAttributeUpdateDelivery>(
          &restored.deliveries.front());
  REQUIRE(restoredAttribute != nullptr);
  REQUIRE(restoredAttribute->message.messageId == enqueued.messageId);
  REQUIRE(restoredAttribute->message.objectInstanceHandle == 92U);
  REQUIRE(restoredAttribute->message.attributes.size() == 1U);
  REQUIRE(restoredAttribute->message.attributes.front().second.size() ==
      payloadBytes.size());
  REQUIRE(std::string(
              static_cast<char const*>(
                  restoredAttribute->message.attributes.front().second.data()),
              payloadBytes.size()) == payloadBytes);
  REQUIRE(restoredAttribute->message.userSuppliedTag.size() == tagBytes.size());
  REQUIRE(restoredAttribute->message.passelsByRecipient.size() == 2U);
  auto const passel = restoredAttribute->message.passelsByRecipient.find(
      restartedReceiverB.membership->id);
  REQUIRE(passel != restoredAttribute->message.passelsByRecipient.end());
  REQUIRE(passel->second.size() == 1U);
  REQUIRE(passel->second.front().transportationName == "HLAreliable");
  REQUIRE(passel->second.front().sentAttributeHandles == std::vector<std::uint64_t>{5U});
  REQUIRE(passel->second.front().preferredOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(restoredAttribute->message.timestamp->implementationName() ==
      L"HLAinteger64Time");
  auto const* restoredTimestamp =
      dynamic_cast<rti1516_2025::HLAinteger64Time const*>(
          restoredAttribute->message.timestamp.get());
  REQUIRE(restoredTimestamp != nullptr);
  REQUIRE(restoredTimestamp->getTime() == 5);
  REQUIRE(restarted.completeTsoDelivery(
      L"exercise", restoredAttribute->queuedMessage).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(restarted.beginTsoPayloadDelivery(
      L"exercise",
      restartedReceiverB.membership->id,
      rti1516_2025::HLAinteger64Time(5),
      true).deliveryStatus == umbra::detail::FederationTsoDeliveryStatus::no_messages);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem fresh-registry restore preserves one timestamped regional attribute-update payload across source-region mutation and resignation",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][tso-queue-state][tso-payload-state][tso-attribute-update-state]"
    "[tso-regional-attribute-update-state][tso-regional-attribute-update-resignation-state]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);

  auto producer = source.join(
      L"exercise", L"producer", L"producer", noOpCallbackRoute());
  auto receiverATime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  auto receiverA = source.joinWithTimeState(
      L"exercise",
      receiverATime,
      L"receiver-a",
      L"receiver-a",
      noOpCallbackRoute());
  auto receiverBTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  auto receiverB = source.joinWithTimeState(
      L"exercise",
      receiverBTime,
      L"receiver-b",
      L"receiver-b",
      noOpCallbackRoute());
  REQUIRE(producer.membership);
  REQUIRE(receiverA.membership);
  REQUIRE(receiverB.membership);

  auto const dimension = source.dimensionHandleFor(L"exercise", "ServerId");
  REQUIRE(dimension.has_value());
  auto sourceRegion = source.createRegion(
      L"exercise", producer.membership->id, {*dimension});
  REQUIRE(sourceRegion.status == umbra::detail::RegionServiceStatus::applied);
  REQUIRE(sourceRegion.regionHandle != 0U);
  REQUIRE(source.setRangeBounds(
      L"exercise", producer.membership->id, sourceRegion.regionHandle, *dimension,
      umbra::detail::RegionRangeBounds{2UL, 4UL}) ==
      umbra::detail::RegionServiceStatus::applied);
  REQUIRE(source.commitRegionModifications(
      L"exercise", producer.membership->id, {sourceRegion.regionHandle}) ==
      umbra::detail::RegionServiceStatus::applied);

  std::string const payloadBytes{"\x63\x64", 2U};
  std::string const tagBytes{"regionalfs", 10U};
  umbra::detail::RegionSpecificationSnapshot const regionSnapshot{
      std::set<std::uint64_t>{*dimension},
      std::map<std::uint64_t, umbra::detail::RegionRangeBounds>{
          {*dimension, umbra::detail::RegionRangeBounds{2UL, 4UL}}},
      true};
  umbra::detail::TsoAttributeUpdateMessage attributeUpdate;
  attributeUpdate.producingFederateId = producer.membership->id;
  attributeUpdate.objectInstanceHandle = 93U;
  attributeUpdate.attributes = {{
      5U,
      rti1516_2025::VariableLengthData(payloadBytes.data(), payloadBytes.size()),
  }};
  attributeUpdate.userSuppliedTag = rti1516_2025::VariableLengthData(
      tagBytes.data(), tagBytes.size());
  attributeUpdate.timestamp =
      std::make_shared<rti1516_2025::HLAinteger64Time>(6);
  attributeUpdate.sentRegionSnapshots.emplace(sourceRegion.regionHandle, regionSnapshot);
  umbra::detail::TsoAttributeUpdatePassel regionalPassel;
  regionalPassel.transportationName = "HLAreliable";
  regionalPassel.sentAttributeHandles = {5U};
  regionalPassel.sentRegionHandles = {sourceRegion.regionHandle};
  regionalPassel.sentRegionSnapshots.emplace(sourceRegion.regionHandle, regionSnapshot);
  regionalPassel.preferredOrderType = rti1516_2025::TIMESTAMP;
  for (auto const recipientId : {
           receiverA.membership->id,
           receiverB.membership->id,
       }) {
    attributeUpdate.passelsByRecipient.emplace(
        recipientId,
        std::vector<umbra::detail::TsoAttributeUpdatePassel>{regionalPassel});
  }
  auto const enqueued = source.enqueueTsoAttributeUpdate(
      L"exercise",
      std::move(attributeUpdate),
      {receiverA.membership->id, receiverB.membership->id},
      {receiverA.membership->id, receiverB.membership->id});
  REQUIRE(enqueued.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(enqueued.queueStatus == umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(enqueued.enqueuedRecipientCount == 2U);

  auto inTransit = source.beginTsoPayloadDelivery(
      L"exercise",
      receiverA.membership->id,
      rti1516_2025::HLAinteger64Time(6),
      true);
  REQUIRE(inTransit.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(inTransit.deliveryStatus ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(inTransit.deliveries.size() == 1U);
  auto const* inTransitAttribute =
      std::get_if<umbra::detail::TsoAttributeUpdateDelivery>(
          &inTransit.deliveries.front());
  REQUIRE(inTransitAttribute != nullptr);
  REQUIRE(inTransitAttribute->message.messageId == enqueued.messageId);
  auto const inTransitMessage = inTransitAttribute->queuedMessage;

  REQUIRE(source.requestFederationSave(
      L"exercise",
      receiverA.membership->id,
      L"regional-attribute-tso-payload-process-restart")
      .status == umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", receiverA.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", receiverB.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", producer.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", receiverA.membership->id).saveCompletedSuccessfully);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", receiverB.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", producer.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(
      L"exercise", L"regional-attribute-tso-payload-process-restart");
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.tsoAttributeUpdateMessages.size() == 1U);
  auto const& savedMessage = image.tsoAttributeUpdateMessages.front();
  REQUIRE(savedMessage.messageId == enqueued.messageId);
  REQUIRE(savedMessage.objectInstanceHandle == 93U);
  REQUIRE(savedMessage.attributes.size() == 1U);
  REQUIRE(savedMessage.attributes.front().value == payloadBytes);
  REQUIRE(savedMessage.userSuppliedTag == tagBytes);
  REQUIRE(savedMessage.passelsByRecipient.size() == 2U);
  REQUIRE(savedMessage.sentRegionSnapshots.size() == 1U);
  REQUIRE(savedMessage.sentRegionSnapshots.front().regionHandle == sourceRegion.regionHandle);
  REQUIRE(savedMessage.sentRegionSnapshots.front().dimensionHandles ==
      std::vector<std::uint64_t>{*dimension});
  REQUIRE(savedMessage.sentRegionSnapshots.front().committedRangeBounds.size() == 1U);
  REQUIRE(savedMessage.sentRegionSnapshots.front().committedRangeBounds.front().dimensionHandle ==
      *dimension);
  REQUIRE(savedMessage.sentRegionSnapshots.front().committedRangeBounds.front().lowerBound ==
      2U);
  REQUIRE(savedMessage.sentRegionSnapshots.front().committedRangeBounds.front().upperBound ==
      4U);
  for (auto const& recipient : savedMessage.passelsByRecipient) {
    REQUIRE(recipient.passels.size() == 1U);
    auto const& savedPassel = recipient.passels.front();
    REQUIRE(savedPassel.sentAttributeHandles == std::vector<std::uint64_t>{5U});
    REQUIRE(savedPassel.sentRegionHandles ==
        std::vector<std::uint64_t>{sourceRegion.regionHandle});
    REQUIRE(savedPassel.sentRegionSnapshots.size() == 1U);
    REQUIRE(savedPassel.sentRegionSnapshots.front().regionHandle == sourceRegion.regionHandle);
    REQUIRE(savedPassel.sentRegionSnapshots.front().dimensionHandles ==
        std::vector<std::uint64_t>{*dimension});
    REQUIRE(savedPassel.sentRegionSnapshots.front().committedRangeBounds.front().lowerBound ==
        2U);
    REQUIRE(savedPassel.sentRegionSnapshots.front().committedRangeBounds.front().upperBound ==
        4U);
  }
  REQUIRE(image.tsoRequestRetractionRecords.size() == 1U);
  REQUIRE(image.tsoRequestRetractionRecords.front().recipientStates.size() == 2U);
  REQUIRE(image.tsoQueueEntries.size() == 2U);
  auto const phaseFor = [&](std::uint64_t recipientId) {
    auto const entry = std::find_if(
        image.tsoQueueEntries.begin(),
        image.tsoQueueEntries.end(),
        [recipientId, messageId = enqueued.messageId](
            umbra::detail::FederationStateImageTsoQueueEntry const& candidate) {
          return candidate.messageId == messageId &&
              candidate.recipientFederateId == recipientId;
        });
    REQUIRE(entry != image.tsoQueueEntries.end());
    return entry->phase;
  };
  REQUIRE(phaseFor(receiverA.membership->id) == 1U);
  REQUIRE(phaseFor(receiverB.membership->id) == 0U);

  // The saved image has already captured the [2, 4) invocation snapshot.
  // Mutate the live source region and then resign its owner; the queued and
  // in-transit payload must retain the immutable snapshot rather than reread
  // the source region (which no longer exists after resignation).
  REQUIRE(source.setRangeBounds(
      L"exercise", producer.membership->id, sourceRegion.regionHandle, *dimension,
      umbra::detail::RegionRangeBounds{8UL, 10UL}) ==
      umbra::detail::RegionServiceStatus::applied);
  REQUIRE(source.commitRegionModifications(
      L"exercise", producer.membership->id, {sourceRegion.regionHandle}) ==
      umbra::detail::RegionServiceStatus::applied);
  REQUIRE(source.resign(
      L"exercise", producer.membership->id, rti1516_2025::NO_ACTION).status ==
      FederationRegistryStatus::applied);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedProducer = restarted.join(
      L"exercise", L"producer", L"producer", noOpCallbackRoute());
  auto restartedReceiverATime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  auto restartedReceiverA = restarted.joinWithTimeState(
      L"exercise",
      restartedReceiverATime,
      L"receiver-a",
      L"receiver-a",
      noOpCallbackRoute());
  auto restartedReceiverBTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  auto restartedReceiverB = restarted.joinWithTimeState(
      L"exercise",
      restartedReceiverBTime,
      L"receiver-b",
      L"receiver-b",
      noOpCallbackRoute());
  REQUIRE(restartedProducer.membership);
  REQUIRE(restartedReceiverA.membership);
  REQUIRE(restartedReceiverB.membership);
  REQUIRE(restartedProducer.membership->id == producer.membership->id);
  REQUIRE(restartedReceiverA.membership->id == receiverA.membership->id);
  REQUIRE(restartedReceiverB.membership->id == receiverB.membership->id);

  REQUIRE(restarted.requestFederationRestore(
      L"exercise",
      restartedReceiverA.membership->id,
      L"regional-attribute-tso-payload-process-restart")
      .status == umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", restartedReceiverA.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", restartedReceiverB.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", restartedProducer.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  // The durable region specification is restored with the invocation-time
  // bounds, then changed again in the restarted live registry.  Delivery must
  // still expose [2, 4) from the saved message/passel snapshot.
  auto restoredRegionDimensions = restarted.dimensionHandleSetForRegion(
      L"exercise", restartedProducer.membership->id, sourceRegion.regionHandle);
  REQUIRE(restoredRegionDimensions.status ==
      umbra::detail::RegionServiceStatus::applied);
  REQUIRE(restoredRegionDimensions.dimensionHandles ==
      std::set<std::uint64_t>{*dimension});
  auto restoredRegionBounds = restarted.rangeBoundsForRegion(
      L"exercise", restartedProducer.membership->id, sourceRegion.regionHandle, *dimension);
  REQUIRE(restoredRegionBounds.status == umbra::detail::RegionServiceStatus::applied);
  REQUIRE(restoredRegionBounds.range.lowerBound == 2UL);
  REQUIRE(restoredRegionBounds.range.upperBound == 4UL);
  REQUIRE(restarted.setRangeBounds(
      L"exercise", restartedProducer.membership->id, sourceRegion.regionHandle, *dimension,
      umbra::detail::RegionRangeBounds{9UL, 11UL}) ==
      umbra::detail::RegionServiceStatus::applied);
  REQUIRE(restarted.commitRegionModifications(
      L"exercise", restartedProducer.membership->id, {sourceRegion.regionHandle}) ==
      umbra::detail::RegionServiceStatus::applied);

  REQUIRE(restarted.completeTsoDelivery(
      L"exercise", inTransitMessage).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  auto restored = restarted.beginTsoPayloadDelivery(
      L"exercise",
      restartedReceiverB.membership->id,
      rti1516_2025::HLAinteger64Time(6),
      true);
  REQUIRE(restored.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(restored.deliveryStatus ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(restored.deliveries.size() == 1U);
  auto const* restoredAttribute =
      std::get_if<umbra::detail::TsoAttributeUpdateDelivery>(
          &restored.deliveries.front());
  REQUIRE(restoredAttribute != nullptr);
  REQUIRE(restoredAttribute->message.messageId == enqueued.messageId);
  REQUIRE(restoredAttribute->message.objectInstanceHandle == 93U);
  REQUIRE(restoredAttribute->message.attributes.size() == 1U);
  REQUIRE(restoredAttribute->message.attributes.front().second.size() ==
      payloadBytes.size());
  REQUIRE(std::string(
              static_cast<char const*>(
                  restoredAttribute->message.attributes.front().second.data()),
              payloadBytes.size()) == payloadBytes);
  REQUIRE(restoredAttribute->message.userSuppliedTag.size() == tagBytes.size());
  REQUIRE(restoredAttribute->message.sentRegionSnapshots.size() == 1U);
  REQUIRE(restoredAttribute->message.sentRegionSnapshots.at(sourceRegion.regionHandle)
              .dimensionHandles == std::set<std::uint64_t>{*dimension});
  REQUIRE(restoredAttribute->message.sentRegionSnapshots.at(sourceRegion.regionHandle)
              .committedRangeBounds.at(*dimension).lowerBound == 2U);
  REQUIRE(restoredAttribute->message.sentRegionSnapshots.at(sourceRegion.regionHandle)
              .committedRangeBounds.at(*dimension).upperBound == 4U);
  auto const passel = restoredAttribute->message.passelsByRecipient.find(
      restartedReceiverB.membership->id);
  REQUIRE(passel != restoredAttribute->message.passelsByRecipient.end());
  REQUIRE(passel->second.size() == 1U);
  REQUIRE(passel->second.front().sentRegionHandles ==
      std::set<std::uint64_t>{sourceRegion.regionHandle});
  REQUIRE(passel->second.front().sentRegionSnapshots.size() == 1U);
  REQUIRE(passel->second.front().sentRegionSnapshots.at(sourceRegion.regionHandle)
              .committedRangeBounds.at(*dimension).lowerBound == 2U);
  REQUIRE(passel->second.front().sentRegionSnapshots.at(sourceRegion.regionHandle)
              .committedRangeBounds.at(*dimension).upperBound == 4U);
  REQUIRE(restoredAttribute->message.timestamp->implementationName() ==
      L"HLAinteger64Time");
  auto const* restoredTimestamp = dynamic_cast<rti1516_2025::HLAinteger64Time const*>(
      restoredAttribute->message.timestamp.get());
  REQUIRE(restoredTimestamp != nullptr);
  REQUIRE(restoredTimestamp->getTime() == 6);
  REQUIRE(restarted.completeTsoDelivery(
      L"exercise", restoredAttribute->queuedMessage).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(restarted.beginTsoPayloadDelivery(
      L"exercise",
      restartedReceiverB.membership->id,
      rti1516_2025::HLAinteger64Time(6),
      true).deliveryStatus == umbra::detail::FederationTsoDeliveryStatus::no_messages);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem fresh-registry restore rehydrates one timestamped object-deletion payload across queued and in-transit recipients",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][tso-queue-state][tso-payload-state][tso-object-deletion-state]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto producer = source.join(
      L"exercise", L"producer", L"producer", noOpCallbackRoute());
  auto receiverATime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  auto receiverA = source.joinWithTimeState(
      L"exercise",
      receiverATime,
      L"receiver-a",
      L"receiver-a",
      noOpCallbackRoute());
  auto receiverBTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  auto receiverB = source.joinWithTimeState(
      L"exercise",
      receiverBTime,
      L"receiver-b",
      L"receiver-b",
      noOpCallbackRoute());
  REQUIRE(producer.membership);
  REQUIRE(receiverA.membership);
  REQUIRE(receiverB.membership);

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const deletePrivilege = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "HLAprivilegeToDeleteObject");
  REQUIRE(server.has_value());
  REQUIRE(deletePrivilege.has_value());
  std::set<std::uint64_t> const deleteAttribute{*deletePrivilege};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", producer.membership->id, *server, deleteAttribute, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", receiverA.membership->id, *server, deleteAttribute, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", receiverB.membership->id, *server, deleteAttribute, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", producer.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 2U);
  for (auto const& discovery : discoveries) {
    REQUIRE(source.beginObjectInstanceDiscovery(
        L"exercise",
        discovery.receivingFederateId,
        discovery.objectInstanceHandle)
                 .has_value());
  }

  std::string const valueBytes{"\x71\x72", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *deletePrivilege,
      rti1516_2025::VariableLengthData(valueBytes.data(), valueBytes.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise",
      producer.membership->id,
      registered.objectInstanceHandle,
      *server,
      {"HLAreliable"},
      &values) == FederationRegistryStatus::applied);

  std::string const tagBytes{"deletefs", 8U};
  umbra::detail::TsoObjectDeletionMessage deletion;
  deletion.userSuppliedTag = rti1516_2025::VariableLengthData(
      tagBytes.data(), tagBytes.size());
  deletion.timestamp =
      std::make_shared<rti1516_2025::HLAinteger64Time>(5);
  auto const enqueued = source.enqueueTsoObjectDeletion(
      L"exercise",
      producer.membership->id,
      registered.objectInstanceHandle,
      std::move(deletion),
      {receiverA.membership->id, receiverB.membership->id});
  REQUIRE(enqueued.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(enqueued.queueStatus == umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(enqueued.deletionStatus ==
      umbra::detail::ObjectInstanceDeletionStatus::applied);
  REQUIRE(enqueued.enqueuedRecipientCount == 2U);
  REQUIRE(enqueued.recipients.size() == 2U);

  auto inTransit = source.beginTsoPayloadDelivery(
      L"exercise",
      receiverA.membership->id,
      rti1516_2025::HLAinteger64Time(5),
      true);
  REQUIRE(inTransit.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(inTransit.deliveryStatus ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(inTransit.deliveries.size() == 1U);
  auto const* inTransitDeletion =
      std::get_if<umbra::detail::TsoObjectDeletionDelivery>(
          &inTransit.deliveries.front());
  REQUIRE(inTransitDeletion != nullptr);
  REQUIRE(inTransitDeletion->message.messageId == enqueued.messageId);
  auto const inTransitMessage = inTransitDeletion->queuedMessage;

  std::wstring const saveLabel = L"deletion-tso-payload-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", receiverA.membership->id, saveLabel)
      .status == umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", receiverA.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", receiverB.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", producer.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", receiverA.membership->id).saveCompletedSuccessfully);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", receiverB.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", producer.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.handle == registered.objectInstanceHandle);
  REQUIRE(savedObject.pendingTimestampedDeletionMessageId == enqueued.messageId);
  REQUIRE(savedObject.pendingTimestampedRemovalFederateIds ==
      std::vector<std::uint64_t>{receiverA.membership->id, receiverB.membership->id});
  REQUIRE(savedObject.attributeValuesPresent);
  REQUIRE(savedObject.attributeValues.size() == 1U);
  REQUIRE(savedObject.attributeValues.front().attributeHandle == *deletePrivilege);
  REQUIRE(savedObject.attributeValues.front().value == valueBytes);
  REQUIRE(image.tsoObjectDeletionMessages.size() == 1U);
  auto const& savedDeletion = image.tsoObjectDeletionMessages.front();
  REQUIRE(savedDeletion.messageId == enqueued.messageId);
  REQUIRE(savedDeletion.objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(savedDeletion.userSuppliedTag == tagBytes);
  REQUIRE(savedDeletion.recipients.size() == 2U);
  REQUIRE(savedDeletion.timestampEncoding.has_value());
  REQUIRE(savedDeletion.reconstitution.has_value());
  REQUIRE(savedDeletion.reconstitution->object.handle == registered.objectInstanceHandle);
  REQUIRE(savedDeletion.reconstitution->object.attributeValues.size() == 1U);
  REQUIRE(savedDeletion.reconstitution->knownObjectClassHandlesByFederate.size() == 3U);
  REQUIRE(image.tsoRequestRetractionRecords.size() == 1U);
  REQUIRE(image.tsoRequestRetractionRecords.front().messageId == enqueued.messageId);
  REQUIRE(image.tsoRequestRetractionRecords.front().recipientStates.size() == 2U);
  REQUIRE(image.tsoQueueEntries.size() == 2U);
  auto const phaseFor = [&](std::uint64_t recipientId) {
    auto const entry = std::find_if(
        image.tsoQueueEntries.begin(),
        image.tsoQueueEntries.end(),
        [recipientId, messageId = enqueued.messageId](
            umbra::detail::FederationStateImageTsoQueueEntry const& candidate) {
          return candidate.messageId == messageId &&
              candidate.recipientFederateId == recipientId;
        });
    REQUIRE(entry != image.tsoQueueEntries.end());
    return entry->phase;
  };
  REQUIRE(phaseFor(receiverA.membership->id) == 1U);
  REQUIRE(phaseFor(receiverB.membership->id) == 0U);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedProducer = restarted.join(
      L"exercise", L"producer", L"producer", noOpCallbackRoute());
  auto restartedReceiverA = restarted.joinWithTimeState(
      L"exercise",
      std::make_shared<umbra::detail::FederateTimeState>(
          L"HLAinteger64Time",
          std::make_shared<rti1516_2025::HLAinteger64Time>(0)),
      L"receiver-a",
      L"receiver-a",
      noOpCallbackRoute());
  auto restartedReceiverB = restarted.joinWithTimeState(
      L"exercise",
      std::make_shared<umbra::detail::FederateTimeState>(
          L"HLAinteger64Time",
          std::make_shared<rti1516_2025::HLAinteger64Time>(0)),
      L"receiver-b",
      L"receiver-b",
      noOpCallbackRoute());
  REQUIRE(restartedProducer.membership);
  REQUIRE(restartedReceiverA.membership);
  REQUIRE(restartedReceiverB.membership);
  REQUIRE(restartedProducer.membership->id == producer.membership->id);
  REQUIRE(restartedReceiverA.membership->id == receiverA.membership->id);
  REQUIRE(restartedReceiverB.membership->id == receiverB.membership->id);

  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedReceiverA.membership->id, saveLabel)
      .status == umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", restartedReceiverA.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", restartedReceiverB.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", restartedProducer.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  REQUIRE(restarted.completeTsoDelivery(
      L"exercise", inTransitMessage).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  auto restored = restarted.beginTsoPayloadDelivery(
      L"exercise",
      restartedReceiverB.membership->id,
      rti1516_2025::HLAinteger64Time(5),
      true);
  REQUIRE(restored.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(restored.deliveryStatus ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(restored.deliveries.size() == 1U);
  auto const* restoredDeletion =
      std::get_if<umbra::detail::TsoObjectDeletionDelivery>(
          &restored.deliveries.front());
  REQUIRE(restoredDeletion != nullptr);
  REQUIRE(restoredDeletion->message.messageId == enqueued.messageId);
  REQUIRE(restoredDeletion->message.objectInstanceHandle ==
      registered.objectInstanceHandle);
  REQUIRE(restoredDeletion->message.userSuppliedTag.size() == tagBytes.size());
  REQUIRE(restoredDeletion->message.recipients.size() == 2U);
  auto const recipient = std::find_if(
      restoredDeletion->message.recipients.begin(),
      restoredDeletion->message.recipients.end(),
      [id = restartedReceiverB.membership->id](
          umbra::detail::TsoObjectDeletionRecipient const& candidate) {
        return candidate.receivingFederateId == id;
      });
  REQUIRE(recipient != restoredDeletion->message.recipients.end());
  REQUIRE(recipient->objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(recipient->callbackRoute);
  REQUIRE(restoredDeletion->message.timestamp->implementationName() ==
      L"HLAinteger64Time");
  auto const* restoredTimestamp = dynamic_cast<rti1516_2025::HLAinteger64Time const*>(
      restoredDeletion->message.timestamp.get());
  REQUIRE(restoredTimestamp != nullptr);
  REQUIRE(restoredTimestamp->getTime() == 5);
  REQUIRE(restarted.completeTsoDelivery(
      L"exercise", restoredDeletion->queuedMessage).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(restarted.beginTsoPayloadDelivery(
      L"exercise",
      restartedReceiverB.membership->id,
      rti1516_2025::HLAinteger64Time(5),
      true).deliveryStatus == umbra::detail::FederationTsoDeliveryStatus::no_messages);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Federation save completion reports not-saved when durable commit fails",
    "[unit][kernel][federation-registry][save-restore][durable-save][failure]") {
  class FailingStore final : public umbra::detail::FederationSaveCommitStore {
   public:
    void commit(umbra::detail::FederationSaveCommitDescriptor const&) override {
      throw std::runtime_error("injected save commit failure");
    }

    [[nodiscard]] std::optional<umbra::detail::FederationSaveCommitDescriptor> load(
        std::wstring const&,
        std::wstring const&) const override {
      return std::nullopt;
    }
  };

  auto store = std::make_shared<FailingStore>();
  EmbeddedFederationRegistry registry({}, store);
  REQUIRE(registry.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto joined = registry.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(joined.status == FederationRegistryStatus::applied);
  REQUIRE(joined.membership);
  REQUIRE(registry.requestFederationSave(
      L"exercise", joined.membership->id, L"checkpoint").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", joined.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);

  auto completed = registry.federateSaveComplete(
      L"exercise", joined.membership->id);
  REQUIRE_FALSE(completed.saveCompletedSuccessfully);
  REQUIRE(completed.notifications.size() == 1U);
  REQUIRE_FALSE(completed.notifications.front().successful);
  REQUIRE(completed.notifications.front().failureReason ==
      rti1516_2025::RTI_UNABLE_TO_SAVE);
}

TEST_CASE(
    "Filesystem federation save commit permits restore admission after reload",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(directory);
  EmbeddedFederationRegistry registry({}, store);
  REQUIRE(registry.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto joined = registry.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(joined.status == FederationRegistryStatus::applied);
  REQUIRE(joined.membership);
  REQUIRE(registry.requestFederationSave(
      L"exercise", joined.membership->id, L"checkpoint").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", joined.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveComplete(
      L"exercise", joined.membership->id).saveCompletedSuccessfully);

  auto restore = registry.requestFederationRestore(
      L"exercise", joined.membership->id, L"checkpoint");
  REQUIRE(restore.status == umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restore.notifications.size() == 3U);
  auto restored = registry.federateRestoreComplete(
      L"exercise", joined.membership->id);
  REQUIRE(restored.notifications.size() == 1U);
  REQUIRE(restored.notifications.front().successful);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores route-free control and temporal state in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][pending-application-request-state]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto sourceTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  auto sourceJoined = source.joinWithTimeState(
      L"exercise",
      sourceTime,
      L"trainer",
      L"alice",
      noOpCallbackRoute());
  REQUIRE(sourceJoined.status == FederationRegistryStatus::applied);
  REQUIRE(sourceJoined.membership);

  auto sourceRoleRequest = sourceTime->requestTimeRegulation(
      std::make_shared<rti1516_2025::HLAinteger64Interval>(2));
  REQUIRE(sourceRoleRequest.status ==
      umbra::detail::FederateTimeEnableStatus::applied);
  REQUIRE(sourceTime->snapshot().timeRegulationPending);

  REQUIRE(source.requestFederationSave(
      L"exercise", sourceJoined.membership->id, L"process-restart-checkpoint")
      .status == umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", sourceJoined.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveComplete(
      L"exercise", sourceJoined.membership->id).saveCompletedSuccessfully);

  // A new registry has no process-local Federation snapshot. It joins the
  // same member identity, loads only the durable route-free image, and keeps
  // its new live callback route/factory.
  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  std::size_t roleDispatchCount = 0U;
  std::size_t roleGrantCount = 0U;
  auto roleFactory = [restartedTime, &roleDispatchCount, &roleGrantCount](
      std::uint64_t federateId,
      std::uint64_t generation,
      umbra::detail::FederationTimeRoleEnableKind kind)
      -> umbra::detail::FederationTimeGrantDispatch {
    REQUIRE(federateId != 0U);
    REQUIRE(generation != 0U);
    auto const callbackEpoch = restartedTime->callbackEpoch();
    return [restartedTime,
            generation,
            kind,
            callbackEpoch,
            &roleDispatchCount,
            &roleGrantCount] {
      ++roleDispatchCount;
      auto enabledTime = kind == umbra::detail::FederationTimeRoleEnableKind::regulation
          ? restartedTime->grantTimeRegulationIfCurrent(generation, callbackEpoch)
          : restartedTime->grantTimeConstrainedIfCurrent(generation, callbackEpoch);
      if (enabledTime) {
        ++roleGrantCount;
      }
    };
  };
  auto restartedJoined = restarted.joinWithTimeState(
      L"exercise",
      restartedTime,
      L"trainer",
      L"alice",
      noOpCallbackRoute(),
      {},
      std::move(roleFactory));
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == sourceJoined.membership->id);

  auto restore = restarted.requestFederationRestore(
      L"exercise",
      restartedJoined.membership->id,
      L"process-restart-checkpoint");
  REQUIRE(restore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(
      L"exercise", restartedJoined.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.timeRoleEnableDispatches.size() == 1U);
  REQUIRE_FALSE(restartedTime->snapshot().timeRegulating);
  REQUIRE(restartedTime->snapshot().timeRegulationPending);
  REQUIRE(restartedTime->snapshot().nextGeneration ==
      sourceTime->snapshot().nextGeneration);

  auto dispatch = std::move(restored.timeRoleEnableDispatches.front());
  dispatch();
  REQUIRE(roleDispatchCount == 1U);
  REQUIRE(roleGrantCount == 1U);
  REQUIRE(restartedTime->snapshot().timeRegulating);
  REQUIRE_FALSE(restartedTime->snapshot().timeRegulationPending);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores pending time advance and deferred lookahead in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][pending-application-request-state]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto sourceTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  std::size_t sourceFactoryCalls = 0U;
  auto sourceTimeAdvanceFactory = [&source, sourceTime, &sourceFactoryCalls](
      std::uint64_t federateId,
      std::uint64_t generation,
      std::uint64_t dispatchIdentity)
      -> umbra::detail::FederationTimeGrantDispatch {
    ++sourceFactoryCalls;
    return [&source,
            sourceTime,
            federateId,
            generation,
            dispatchIdentity] {
      if (source.beginTimeAdvanceGrant(
              L"exercise",
              federateId,
              generation,
              dispatchIdentity) ==
          FederationTimeGrantStatus::applied) {
        static_cast<void>(sourceTime->grant(generation));
      }
    };
  };
  auto sourceJoined = source.joinWithTimeState(
      L"exercise",
      sourceTime,
      L"trainer",
      L"alice",
      noOpCallbackRoute(),
      std::move(sourceTimeAdvanceFactory));
  REQUIRE(sourceJoined.status == FederationRegistryStatus::applied);
  REQUIRE(sourceJoined.membership);

  auto regulation = sourceTime->requestTimeRegulation(
      std::make_shared<rti1516_2025::HLAinteger64Interval>(5));
  REQUIRE(regulation.status == umbra::detail::FederateTimeEnableStatus::applied);
  REQUIRE(sourceTime->grantTimeRegulation(regulation.generation));

  auto deferredLookahead = sourceTime->modifyLookahead(
      std::make_shared<rti1516_2025::HLAinteger64Interval>(1));
  REQUIRE(deferredLookahead ==
      umbra::detail::FederateTimeModifyLookaheadStatus::applied);
  auto advance = sourceTime->requestAdvance(
      std::make_shared<rti1516_2025::HLAinteger64Time>(5));
  REQUIRE(advance.status == umbra::detail::FederateTimeAdvanceStatus::applied);
  REQUIRE(sourceTime->snapshot().timeAdvancePending);
  REQUIRE(sourceTime->snapshot().pendingModifiedLookahead);
  auto scheduled = source.requestTimeAdvanceGrant(
      L"exercise",
      sourceJoined.membership->id,
      advance.generation);
  REQUIRE(scheduled.status == FederationTimeGrantStatus::applied);
  REQUIRE(scheduled.dispatches.size() == 1U);
  REQUIRE(sourceFactoryCalls == 1U);

  REQUIRE(source.requestFederationSave(
      L"exercise",
      sourceJoined.membership->id,
      L"pending-application-request-checkpoint")
      .status == umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise",
      sourceJoined.membership->id)
      .status == umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveComplete(
      L"exercise",
      sourceJoined.membership->id)
      .saveCompletedSuccessfully);
  std::move(scheduled.dispatches.front())();
  REQUIRE_FALSE(sourceTime->snapshot().timeAdvancePending);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  std::size_t restartedFactoryCalls = 0U;
  auto restartedTimeAdvanceFactory = [&restarted, restartedTime, &restartedFactoryCalls](
      std::uint64_t federateId,
      std::uint64_t generation,
      std::uint64_t dispatchIdentity)
      -> umbra::detail::FederationTimeGrantDispatch {
    ++restartedFactoryCalls;
    return [&restarted,
            restartedTime,
            federateId,
            generation,
            dispatchIdentity] {
      if (restarted.beginTimeAdvanceGrant(
              L"exercise",
              federateId,
              generation,
              dispatchIdentity) ==
          FederationTimeGrantStatus::applied) {
        static_cast<void>(restartedTime->grant(generation));
      }
    };
  };
  auto restartedJoined = restarted.joinWithTimeState(
      L"exercise",
      restartedTime,
      L"trainer",
      L"alice",
      noOpCallbackRoute(),
      std::move(restartedTimeAdvanceFactory));
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == sourceJoined.membership->id);

  REQUIRE(restarted.requestFederationRestore(
      L"exercise",
      restartedJoined.membership->id,
      L"pending-application-request-checkpoint")
      .status == umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(
      L"exercise",
      restartedJoined.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restartedFactoryCalls == 1U);
  REQUIRE(restored.timeAdvanceGrantDispatches.size() == 1U);
  std::move(restored.timeAdvanceGrantDispatches.front())();

  auto restoredTime = restartedTime->snapshot();
  REQUIRE_FALSE(restoredTime.timeAdvancePending);
  REQUIRE(restoredTime.advanceMode ==
      umbra::detail::FederateTimeAdvanceMode::none);
  REQUIRE(restoredTime.lookahead);
  auto const* restoredLookahead =
      dynamic_cast<rti1516_2025::HLAinteger64Interval const*>(
          restoredTime.lookahead.get());
  REQUIRE(restoredLookahead);
  REQUIRE(restoredLookahead->getInterval() == 1);
  REQUIRE_FALSE(restoredTime.pendingModifiedLookahead);
  auto const* restoredCurrentTime =
      dynamic_cast<rti1516_2025::HLAinteger64Time const*>(
          restoredTime.currentTime.get());
  REQUIRE(restoredCurrentTime);
  REQUIRE(restoredCurrentTime->getTime() == 5);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores object-instance-name reservations in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][object-name-reservation-state]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto sourceJoined = source.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(sourceJoined.status == FederationRegistryStatus::applied);
  REQUIRE(sourceJoined.membership);
  auto const federateId = sourceJoined.membership->id;
  auto reservation = source.reserveObjectInstanceName(
      L"exercise", federateId, L"process-restart-reserved-table");
  REQUIRE(reservation.status == ObjectInstanceNameReservationStatus::applied);
  REQUIRE(reservation.succeeded);

  REQUIRE(source.requestFederationSave(
      L"exercise", federateId, L"process-restart-name-checkpoint").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveComplete(L"exercise", federateId)
      .saveCompletedSuccessfully);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedJoined = restarted.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == federateId);

  REQUIRE(restarted.requestFederationRestore(
      L"exercise", federateId, L"process-restart-name-checkpoint").status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(L"exercise", federateId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  auto duplicate = restarted.reserveObjectInstanceName(
      L"exercise", federateId, L"process-restart-reserved-table");
  REQUIRE(duplicate.status == ObjectInstanceNameReservationStatus::applied);
  REQUIRE_FALSE(duplicate.succeeded);
  REQUIRE(restarted.releaseObjectInstanceName(
      L"exercise", federateId, L"process-restart-reserved-table") ==
      ObjectInstanceNameReservationStatus::applied);
  auto available = restarted.reserveObjectInstanceName(
      L"exercise", federateId, L"process-restart-reserved-table");
  REQUIRE(available.status == ObjectInstanceNameReservationStatus::applied);
  REQUIRE(available.succeeded);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores synchronization points in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][synchronization-state]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto sourceJoined = source.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(sourceJoined.status == FederationRegistryStatus::applied);
  REQUIRE(sourceJoined.membership);
  auto const federateId = sourceJoined.membership->id;
  auto registered = source.registerSynchronizationPoint(
      L"exercise",
      federateId,
      L"process-restart-barrier",
      {0x01U, 0x02U},
      {federateId});
  REQUIRE(registered.status ==
      umbra::detail::SynchronizationPointRegistrationStatus::applied);

  REQUIRE(source.requestFederationSave(
      L"exercise", federateId, L"process-restart-sync-checkpoint").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveComplete(L"exercise", federateId)
      .saveCompletedSuccessfully);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedJoined = restarted.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == federateId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", federateId, L"process-restart-sync-checkpoint").status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(L"exercise", federateId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  REQUIRE(restarted.requestFederationSave(
      L"exercise", federateId, L"process-restart-sync-after-restore").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveBegun(L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveComplete(L"exercise", federateId)
      .saveCompletedSuccessfully);
  auto committed = store->load(
      L"exercise", L"process-restart-sync-after-restore");
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.synchronizationPoints.size() == 1U);
  REQUIRE(image.synchronizationPoints.front().label ==
      L"process-restart-barrier");
  REQUIRE(image.synchronizationPoints.front().userSuppliedTag ==
      std::string{"\x01\x02", 2U});
  REQUIRE(image.synchronizationPoints.front().synchronizationSet ==
      std::vector<std::uint64_t>{federateId});

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores federation-owned regions in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][region-state]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto sourceJoined = source.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(sourceJoined.status == FederationRegistryStatus::applied);
  REQUIRE(sourceJoined.membership);
  auto const federateId = sourceJoined.membership->id;
  auto dimension = source.dimensionHandleFor(L"exercise", "ServerId");
  REQUIRE(dimension.has_value());
  auto created = source.createRegion(L"exercise", federateId, {*dimension});
  REQUIRE(created.status == umbra::detail::RegionServiceStatus::applied);
  REQUIRE(created.regionHandle != 0U);
  REQUIRE(source.setRangeBounds(
      L"exercise", federateId, created.regionHandle, *dimension,
      umbra::detail::RegionRangeBounds{2UL, 5UL}) ==
      umbra::detail::RegionServiceStatus::applied);
  REQUIRE(source.commitRegionModifications(
      L"exercise", federateId, {created.regionHandle}) ==
      umbra::detail::RegionServiceStatus::applied);

  REQUIRE(source.requestFederationSave(
      L"exercise", federateId, L"process-restart-region-checkpoint").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveComplete(L"exercise", federateId)
      .saveCompletedSuccessfully);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedJoined = restarted.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == federateId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", federateId, L"process-restart-region-checkpoint").status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(L"exercise", federateId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  auto dimensions = restarted.dimensionHandleSetForRegion(
      L"exercise", federateId, created.regionHandle);
  REQUIRE(dimensions.status == umbra::detail::RegionServiceStatus::applied);
  REQUIRE(dimensions.dimensionHandles == std::set<std::uint64_t>{*dimension});
  auto bounds = restarted.rangeBoundsForRegion(
      L"exercise", federateId, created.regionHandle, *dimension);
  REQUIRE(bounds.status == umbra::detail::RegionServiceStatus::applied);
  REQUIRE(bounds.range.lowerBound == 2UL);
  REQUIRE(bounds.range.upperBound == 5UL);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores object-class attribute declarations in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][object-class-declaration-state]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto sourceJoined = source.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(sourceJoined.status == FederationRegistryStatus::applied);
  REQUIRE(sourceJoined.membership);
  auto const federateId = sourceJoined.membership->id;
  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());

  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", federateId, *server, {*efficiency}, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", federateId, *server, {*efficiency}, true, "High") ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.changeDefaultAttributeTransportationType(
      L"exercise", federateId, *server, {*efficiency}, "HLAreliable") ==
      umbra::detail::AttributeTransportationTypeDefaultStatus::applied);
  REQUIRE(source.changeDefaultAttributeOrderType(
      L"exercise", federateId, *server, {*efficiency}, rti1516_2025::TIMESTAMP) ==
      umbra::detail::AttributeOrderTypeDefaultStatus::applied);

  auto sourceDeclaration = source.objectClassAttributeDeclarationFor(
      L"exercise", federateId, *server);
  REQUIRE(sourceDeclaration.has_value());
  REQUIRE(sourceDeclaration->explicitlyPublishedAttributes ==
      std::set<std::uint64_t>{*efficiency});
  REQUIRE(sourceDeclaration->subscribedAttributes.at(*efficiency));
  REQUIRE(sourceDeclaration->subscribedUpdateRateDesignators.at(*efficiency) ==
      "High");
  auto const savedGeneration = sourceDeclaration->subscriptionGeneration;
  REQUIRE(savedGeneration != 0U);

  REQUIRE(source.requestFederationSave(
      L"exercise", federateId, L"process-restart-object-class-checkpoint").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveComplete(L"exercise", federateId)
      .saveCompletedSuccessfully);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedJoined = restarted.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == federateId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", federateId, L"process-restart-object-class-checkpoint").status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(L"exercise", federateId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  auto restoredDeclaration = restarted.objectClassAttributeDeclarationFor(
      L"exercise", federateId, *server);
  REQUIRE(restoredDeclaration.has_value());
  REQUIRE(restoredDeclaration->explicitlyPublishedAttributes ==
      std::set<std::uint64_t>{*efficiency});
  REQUIRE(restoredDeclaration->subscribedAttributes.at(*efficiency));
  REQUIRE(restoredDeclaration->subscribedUpdateRateDesignators.at(*efficiency) ==
      "High");
  REQUIRE(restoredDeclaration->subscriptionGeneration == savedGeneration);

  REQUIRE(restarted.requestFederationSave(
      L"exercise", federateId, L"process-restart-object-class-after-restore").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveBegun(L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveComplete(L"exercise", federateId)
      .saveCompletedSuccessfully);
  auto committed = store->load(
      L"exercise", L"process-restart-object-class-after-restore");
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objectClassAttributeDeclarations.size() == 1U);
  REQUIRE(image.objectClassAttributeDeclarations.front().federateId == federateId);
  REQUIRE(image.objectClassAttributeDeclarations.front().subscriptionGeneration ==
      savedGeneration);
  REQUIRE(image.objectClassAttributeDeclarations.front().classes.size() == 1U);
  auto const& restoredClass =
      image.objectClassAttributeDeclarations.front().classes.front();
  REQUIRE(restoredClass.objectClassHandle == *server);
  REQUIRE(restoredClass.explicitlyPublishedAttributeHandles ==
      std::vector<std::uint64_t>{*efficiency});
  REQUIRE(restoredClass.subscribedAttributes.size() == 1U);
  REQUIRE(restoredClass.subscribedAttributes.front().attributeHandle == *efficiency);
  REQUIRE(restoredClass.subscribedAttributes.front().active);
  REQUIRE(restoredClass.subscribedUpdateRateDesignators.size() == 1U);
  REQUIRE(restoredClass.subscribedUpdateRateDesignators.front().attributeHandle == *efficiency);
  REQUIRE(restoredClass.subscribedUpdateRateDesignators.front().value == "High");
  REQUIRE(restoredClass.defaultTransportationTypes.size() == 1U);
  REQUIRE(restoredClass.defaultTransportationTypes.front().attributeHandle == *efficiency);
  REQUIRE(restoredClass.defaultTransportationTypes.front().value == "HLAreliable");
  REQUIRE(restoredClass.defaultOrderTypes.size() == 1U);
  REQUIRE(restoredClass.defaultOrderTypes.front().attributeHandle == *efficiency);
  REQUIRE(restoredClass.defaultOrderTypes.front().orderType == 2U);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores latest object application values in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][application-value-state]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto sourceJoined = source.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(sourceJoined.status == FederationRegistryStatus::applied);
  REQUIRE(sourceJoined.membership);
  auto const federateId = sourceJoined.membership->id;
  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", federateId, *server, {*efficiency}, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", federateId, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);

  std::vector<rti1516_2025::Octet> valueBytes{
      static_cast<rti1516_2025::Octet>(0x01U),
      static_cast<rti1516_2025::Octet>(0xfeU),
      static_cast<rti1516_2025::Octet>(0x7fU)};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{
      {*efficiency,
       rti1516_2025::VariableLengthData(valueBytes.data(), valueBytes.size())},
  };
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise",
      federateId,
      registered.objectInstanceHandle,
      *server,
      {"HLAreliable"},
      &values) == FederationRegistryStatus::applied);

  REQUIRE(source.requestFederationSave(
      L"exercise", federateId, L"process-restart-application-value-checkpoint").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveComplete(L"exercise", federateId)
      .saveCompletedSuccessfully);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedJoined = restarted.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == federateId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", federateId, L"process-restart-application-value-checkpoint").status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(L"exercise", federateId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  // A fresh registry has no process-local object snapshot. Saving immediately
  // after restore therefore proves that object identity and the latest value
  // were materialized from the durable image rather than retained in memory.
  REQUIRE(restarted.requestFederationSave(
      L"exercise", federateId, L"process-restart-application-value-after-restore").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveBegun(L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveComplete(L"exercise", federateId)
      .saveCompletedSuccessfully);

  auto committed = store->load(
      L"exercise", L"process-restart-application-value-after-restore");
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& object = image.objects.front();
  REQUIRE(object.handle == registered.objectInstanceHandle);
  REQUIRE(object.name == registered.objectInstanceName);
  REQUIRE(object.registeredObjectClassHandle == *server);
  REQUIRE(object.attributeValuesPresent);
  REQUIRE(object.attributeValues.size() == 1U);
  REQUIRE(object.attributeValues.front().attributeHandle == *efficiency);
  REQUIRE(object.attributeValues.front().value ==
      std::string{reinterpret_cast<char const*>(valueBytes.data()), valueBytes.size()});

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores ownership assumption search state and continues with a newly eligible federate",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-ownership-assumption-search][ownership-ledger-state]"
    "[ownership-assumption-research]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto firstCandidate = source.join(
      L"exercise", L"candidate-one", L"candidate-one", noOpCallbackRoute());
  auto secondCandidate = source.join(
      L"exercise", L"candidate-two", L"candidate-two", noOpCallbackRoute());
  REQUIRE(owner.membership);
  REQUIRE(firstCandidate.membership);
  REQUIRE(secondCandidate.membership);

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", firstCandidate.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", firstCandidate.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  auto const& firstDiscovery = discoveries.front();
  REQUIRE(firstDiscovery.receivingFederateId == firstCandidate.membership->id);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise",
      firstDiscovery.receivingFederateId,
      registered.objectInstanceHandle)
      .has_value());

  std::string const valueBytes{"\x51\x52", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *efficiency,
      rti1516_2025::VariableLengthData(valueBytes.data(), valueBytes.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise",
      owner.membership->id,
      registered.objectInstanceHandle,
      *server,
      {"HLAreliable"},
      &values) == FederationRegistryStatus::applied);
  std::vector<unsigned char> const divestitureTag{'a', 's', 's', 'u', 'm', 'e'};
  auto divestiture = source.planUnconditionalAttributeOwnershipDivestiture(
      L"exercise",
      owner.membership->id,
      registered.objectInstanceHandle,
      efficiencyOnly,
      divestitureTag);
  REQUIRE(divestiture.status ==
      umbra::detail::UnconditionalAttributeOwnershipDivestitureStatus::applied);
  REQUIRE(divestiture.assumptionRecipients.size() == 1U);
  REQUIRE(divestiture.assumptionRecipients.front().receivingFederateId ==
      firstCandidate.membership->id);
  REQUIRE(divestiture.assumptionRecipients.front().attributeHandles == efficiencyOnly);

  std::wstring const saveLabel = L"ownership-assumption-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", firstCandidate.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", secondCandidate.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", firstCandidate.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", secondCandidate.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.ownershipAssumptionRecipientsByAttribute.size() == 1U);
  REQUIRE(savedObject.ownershipAssumptionRecipientsByAttribute.front().attributeHandle ==
      *efficiency);
  REQUIRE(savedObject.ownershipAssumptionRecipientsByAttribute.front().recipientFederateIds ==
      std::vector<std::uint64_t>{firstCandidate.membership->id});
  REQUIRE(savedObject.ownershipAssumptionUserSuppliedTagsByAttribute.size() == 1U);
  REQUIRE(savedObject.ownershipAssumptionUserSuppliedTagsByAttribute.front().attributeHandle ==
      *efficiency);
  REQUIRE(savedObject.ownershipAssumptionUserSuppliedTagsByAttribute.front().userSuppliedTag ==
      std::string(divestitureTag.begin(), divestitureTag.end()));
  REQUIRE(image.pendingAttributeOwnershipAssumptionsPresent);
  REQUIRE(image.pendingAttributeOwnershipAssumptions.size() == 1U);
  REQUIRE(image.pendingAttributeOwnershipAssumptions.front().objectInstanceHandle ==
      registered.objectInstanceHandle);
  REQUIRE(image.pendingAttributeOwnershipAssumptions.front().receivingFederateId ==
      firstCandidate.membership->id);
  REQUIRE(image.pendingAttributeOwnershipAssumptions.front().attributeHandles ==
      std::vector<std::uint64_t>{*efficiency});
  REQUIRE(image.pendingAttributeOwnershipAssumptions.front().userSuppliedTag ==
      std::string(divestitureTag.begin(), divestitureTag.end()));
  REQUIRE(savedObject.pendingOperationCount == 3U);
  REQUIRE(savedObject.attributes.size() == 2U);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedOwner = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto restartedFirstCandidate = restarted.join(
      L"exercise", L"candidate-one", L"candidate-one", noOpCallbackRoute());
  auto restartedSecondCandidate = restarted.join(
      L"exercise", L"candidate-two", L"candidate-two", noOpCallbackRoute());
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedFirstCandidate.membership);
  REQUIRE(restartedSecondCandidate.membership);
  REQUIRE(restartedOwner.membership->id == owner.membership->id);
  REQUIRE(restartedFirstCandidate.membership->id == firstCandidate.membership->id);
  REQUIRE(restartedSecondCandidate.membership->id == secondCandidate.membership->id);

  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedOwner.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto firstRestore = restarted.federateRestoreComplete(
      L"exercise", restartedOwner.membership->id);
  REQUIRE(firstRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto secondRestore = restarted.federateRestoreComplete(
      L"exercise", restartedFirstCandidate.membership->id);
  REQUIRE(secondRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto finalRestore = restarted.federateRestoreComplete(
      L"exercise", restartedSecondCandidate.membership->id);
  REQUIRE(finalRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(finalRestore.attributeOwnershipAssumptionWorkItems.size() == 1U);
  REQUIRE(finalRestore.attributeOwnershipAssumptionWorkItems.front().receivingFederateId ==
      restartedFirstCandidate.membership->id);
  REQUIRE(finalRestore.attributeOwnershipAssumptionWorkItems.front().attributeHandles ==
      efficiencyOnly);
  REQUIRE(finalRestore.attributeOwnershipAssumptionWorkItems.front().userSuppliedTag ==
      divestitureTag);
  auto reboundDelivery = restarted.attributeOwnershipAssumptionDeliveryFor(
      L"exercise",
      restartedFirstCandidate.membership->id,
      registered.objectInstanceHandle,
      efficiencyOnly);
  REQUIRE(reboundDelivery.has_value());
  REQUIRE(reboundDelivery->attributeHandles == efficiencyOnly);
  // Candidate two was not known at save time. Its later discovery and
  // publication continue the restored search without repeating candidate
  // one's already-recorded offer.
  REQUIRE(restarted.setObjectClassAttributeSubscription(
      L"exercise",
      restartedSecondCandidate.membership->id,
      *server,
      efficiencyOnly,
      true) == umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  auto restartedDiscoveries = restarted.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(restartedDiscoveries.size() == 1U);
  REQUIRE(restartedDiscoveries.front().receivingFederateId ==
      restartedSecondCandidate.membership->id);
  REQUIRE(restarted.beginObjectInstanceDiscovery(
      L"exercise",
      restartedSecondCandidate.membership->id,
      registered.objectInstanceHandle)
      .has_value());
  REQUIRE(restarted.setObjectClassAttributePublication(
      L"exercise",
      restartedSecondCandidate.membership->id,
      *server,
      efficiencyOnly,
      true) == umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  auto continuation = restarted.planAttributeOwnershipAssumptionsForFederate(
      L"exercise",
      restartedSecondCandidate.membership->id,
      registered.objectInstanceHandle,
      &efficiencyOnly);
  REQUIRE(continuation.size() == 1U);
  REQUIRE(continuation.front().receivingFederateId ==
      restartedSecondCandidate.membership->id);
  REQUIRE(continuation.front().objectInstanceHandle ==
      registered.objectInstanceHandle);
  REQUIRE(continuation.front().attributeHandles == efficiencyOnly);
  REQUIRE(continuation.front().userSuppliedTag == divestitureTag);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Federation restore rejects an in-memory snapshot when its durable envelope is gone",
    "[unit][kernel][federation-registry][save-restore][durable-save][restore][failure]") {
  class CommitThenForgetStore final : public umbra::detail::FederationSaveCommitStore {
   public:
    void commit(umbra::detail::FederationSaveCommitDescriptor const& descriptor) override {
      lastCommit = descriptor;
    }

    [[nodiscard]] std::optional<umbra::detail::FederationSaveCommitDescriptor> load(
        std::wstring const&,
        std::wstring const&) const override {
      return std::nullopt;
    }

    umbra::detail::FederationSaveCommitDescriptor lastCommit;
  };

  auto store = std::make_shared<CommitThenForgetStore>();
  EmbeddedFederationRegistry registry({}, store);
  REQUIRE(registry.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto joined = registry.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(joined.status == FederationRegistryStatus::applied);
  REQUIRE(joined.membership);
  REQUIRE(registry.requestFederationSave(
      L"exercise", joined.membership->id, L"checkpoint").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", joined.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveComplete(
      L"exercise", joined.membership->id).saveCompletedSuccessfully);

  auto restore = registry.requestFederationRestore(
      L"exercise", joined.membership->id, L"checkpoint");
  REQUIRE(restore.status == umbra::detail::FederationRestoreControlStatus::snapshot_not_found);
  REQUIRE(restore.notifications.size() == 1U);
  REQUIRE(restore.notifications.front().kind ==
      umbra::detail::FederationRestoreNotificationKind::request_failed);
}

TEST_CASE(
    "Filesystem state image restores pending regular ownership release work in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][ownership-ledger-state][attribute-ownership-acquisition]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto requester = source.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(owner.status == FederationRegistryStatus::applied);
  REQUIRE(requester.status == FederationRegistryStatus::applied);
  REQUIRE(owner.membership);
  REQUIRE(requester.membership);

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", requester.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(discoveries.front().receivingFederateId == requester.membership->id);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise",
      requester.membership->id,
      registered.objectInstanceHandle)
      .has_value());

  // The requester must publish after discovery so the saved object contains
  // both live known-class projections and a valid publication precondition.
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", requester.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  std::string const valueBytes{"\x71\x72", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *efficiency,
      rti1516_2025::VariableLengthData(valueBytes.data(), valueBytes.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise",
      owner.membership->id,
      registered.objectInstanceHandle,
      *server,
      {"HLAreliable"},
      &values) == FederationRegistryStatus::applied);

  std::vector<unsigned char> const acquisitionTag{'r', 'e', 's', 't', 'a', 'r', 't'};
  auto acquisition = source.planAttributeOwnershipAcquisition(
      L"exercise",
      requester.membership->id,
      registered.objectInstanceHandle,
      efficiencyOnly,
      acquisitionTag);
  REQUIRE(acquisition.status ==
      umbra::detail::AttributeOwnershipAcquisitionStatus::applied);
  REQUIRE(acquisition.workItems.size() == 1U);
  REQUIRE(acquisition.workItems.front().kind ==
      umbra::detail::AttributeOwnershipAcquisitionWorkKind::request_release);
  REQUIRE(acquisition.workItems.front().requestingFederateId == requester.membership->id);
  REQUIRE(acquisition.workItems.front().receivingFederateId == owner.membership->id);
  REQUIRE(acquisition.workItems.front().objectInstanceHandle ==
      registered.objectInstanceHandle);
  REQUIRE(acquisition.workItems.front().attributeHandles == efficiencyOnly);
  REQUIRE(acquisition.workItems.front().userSuppliedTag == acquisitionTag);

  std::wstring const saveLabel = L"ownership-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", requester.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", requester.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.handle == registered.objectInstanceHandle);
  REQUIRE(savedObject.attributeValuesPresent);
  REQUIRE(savedObject.attributeValues.size() == 1U);
  REQUIRE(savedObject.attributeValues.front().attributeHandle == *efficiency);
  REQUIRE(savedObject.attributeValues.front().value == valueBytes);
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionRequests.size() == 1U);
  auto const& savedRequest =
      savedObject.pendingAttributeOwnershipAcquisitionRequests.front();
  REQUIRE(savedRequest.requestId == acquisition.workItems.front().requestId);
  REQUIRE(savedRequest.requestingFederateId == requester.membership->id);
  REQUIRE(savedRequest.desiredAttributeHandles ==
      std::vector<std::uint64_t>{*efficiency});
  REQUIRE(savedRequest.unavailableQueuedAttributeHandles.empty());
  REQUIRE(savedRequest.releaseCallbacksQueuedByOwningFederate.size() == 1U);
  REQUIRE(savedRequest.releaseCallbacksQueuedByOwningFederate.front().first ==
      owner.membership->id);
  REQUIRE(savedRequest.releaseCallbacksQueuedByOwningFederate.front().second ==
      std::vector<std::uint64_t>{*efficiency});

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedOwner = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto restartedRequester = restarted.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedRequester.membership);
  REQUIRE(restartedOwner.membership->id == owner.membership->id);
  REQUIRE(restartedRequester.membership->id == requester.membership->id);

  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedRequester.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto firstRestore = restarted.federateRestoreComplete(
      L"exercise", restartedOwner.membership->id);
  REQUIRE(firstRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(firstRestore.ownershipAcquisitionWorkItems.empty());
  auto restored = restarted.federateRestoreComplete(
      L"exercise", restartedRequester.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.ownershipAcquisitionWorkItems.size() == 1U);
  auto const& restoredWork = restored.ownershipAcquisitionWorkItems.front();
  REQUIRE(restoredWork.kind ==
      umbra::detail::AttributeOwnershipAcquisitionWorkKind::request_release);
  REQUIRE(restoredWork.requestingFederateId == restartedRequester.membership->id);
  REQUIRE(restoredWork.receivingFederateId == restartedOwner.membership->id);
  REQUIRE(restoredWork.objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(restoredWork.requestId == savedRequest.requestId);
  REQUIRE(restoredWork.attributeHandles == efficiencyOnly);
  REQUIRE(restoredWork.userSuppliedTag == acquisitionTag);

  auto restoredOwnership = restarted.attributeOwnedByFederate(
      L"exercise",
      restartedOwner.membership->id,
      registered.objectInstanceHandle,
      *efficiency);
  REQUIRE(restoredOwnership.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE(restoredOwnership.ownedByRequestingFederate);
  auto requesterOwnership = restarted.attributeOwnedByFederate(
      L"exercise",
      restartedRequester.membership->id,
      registered.objectInstanceHandle,
      *efficiency);
  REQUIRE(requesterOwnership.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE_FALSE(requesterOwnership.ownedByRequestingFederate);

  auto releaseDelivery = restarted.beginAttributeOwnershipAcquisitionRelease(
      L"exercise",
      restoredWork.requestingFederateId,
      restoredWork.receivingFederateId,
      restoredWork.objectInstanceHandle,
      restoredWork.requestId,
      restoredWork.attributeHandles);
  REQUIRE(releaseDelivery.has_value());
  REQUIRE(releaseDelivery->objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(releaseDelivery->candidateAttributeHandles == efficiencyOnly);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores pending If Available ownership callback in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][ownership-ledger-state][attribute-ownership-acquisition]"
    "[attribute-ownership-acquisition-if-available]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto requester = source.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(owner.status == FederationRegistryStatus::applied);
  REQUIRE(requester.status == FederationRegistryStatus::applied);
  REQUIRE(owner.membership);
  REQUIRE(requester.membership);

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", requester.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(discoveries.front().receivingFederateId == requester.membership->id);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise",
      requester.membership->id,
      registered.objectInstanceHandle)
      .has_value());

  // The requester must publish after discovery so the saved object contains
  // both live known-class projections and a valid publication precondition.
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", requester.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  std::string const valueBytes{"\x73\x74", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *efficiency,
      rti1516_2025::VariableLengthData(valueBytes.data(), valueBytes.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise",
      owner.membership->id,
      registered.objectInstanceHandle,
      *server,
      {"HLAreliable"},
      &values) == FederationRegistryStatus::applied);

  std::vector<unsigned char> const acquisitionTag{'w', 't', 'a', '-', 'r', 'e', 's', 't', 'a', 'r', 't'};
  auto acquisition = source.planAttributeOwnershipAcquisitionIfAvailable(
      L"exercise",
      requester.membership->id,
      registered.objectInstanceHandle,
      efficiencyOnly,
      acquisitionTag);
  REQUIRE(acquisition.status ==
      umbra::detail::AttributeOwnershipAcquisitionIfAvailableStatus::applied);
  REQUIRE(acquisition.requestId != 0U);
  REQUIRE(acquisition.callbackRoute);

  std::wstring const saveLabel = L"ownership-if-available-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", requester.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", requester.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.handle == registered.objectInstanceHandle);
  REQUIRE(savedObject.attributeValuesPresent);
  REQUIRE(savedObject.attributeValues.size() == 1U);
  REQUIRE(savedObject.attributeValues.front().attributeHandle == *efficiency);
  REQUIRE(savedObject.attributeValues.front().value == valueBytes);
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionIfAvailableRequests.size() == 1U);
  auto const& savedRequest =
      savedObject.pendingAttributeOwnershipAcquisitionIfAvailableRequests.front();
  REQUIRE(savedRequest.requestId == acquisition.requestId);
  REQUIRE(savedRequest.requestingFederateId == requester.membership->id);
  REQUIRE(savedRequest.requestSequence != 0U);
  REQUIRE(savedRequest.desiredAttributeHandles ==
      std::vector<std::uint64_t>{*efficiency});
  REQUIRE(savedRequest.userSuppliedTag ==
      std::string(acquisitionTag.begin(), acquisitionTag.end()));

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedOwner = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto restartedRequester = restarted.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedRequester.membership);
  REQUIRE(restartedOwner.membership->id == owner.membership->id);
  REQUIRE(restartedRequester.membership->id == requester.membership->id);

  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedRequester.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto firstRestore = restarted.federateRestoreComplete(
      L"exercise", restartedOwner.membership->id);
  REQUIRE(firstRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(firstRestore.ownershipAcquisitionWorkItems.empty());
  auto restored = restarted.federateRestoreComplete(
      L"exercise", restartedRequester.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.ownershipAcquisitionWorkItems.size() == 1U);
  auto const& restoredWork = restored.ownershipAcquisitionWorkItems.front();
  REQUIRE(restoredWork.kind ==
      umbra::detail::AttributeOwnershipAcquisitionWorkKind::if_available_notification);
  REQUIRE(restoredWork.requestingFederateId == restartedRequester.membership->id);
  REQUIRE(restoredWork.receivingFederateId == restartedRequester.membership->id);
  REQUIRE(restoredWork.objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(restoredWork.requestId == savedRequest.requestId);
  REQUIRE(restoredWork.attributeHandles.empty());
  REQUIRE(restoredWork.userSuppliedTag == acquisitionTag);
  REQUIRE(restoredWork.candidateIsIfAvailable);

  auto restoredOwner = restarted.attributeOwnedByFederate(
      L"exercise",
      restartedOwner.membership->id,
      registered.objectInstanceHandle,
      *efficiency);
  REQUIRE(restoredOwner.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE(restoredOwner.ownedByRequestingFederate);
  auto restoredRequesterOwnership = restarted.attributeOwnedByFederate(
      L"exercise",
      restartedRequester.membership->id,
      registered.objectInstanceHandle,
      *efficiency);
  REQUIRE(restoredRequesterOwnership.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE_FALSE(restoredRequesterOwnership.ownedByRequestingFederate);

  auto unavailable = restarted.beginAttributeOwnershipAcquisitionIfAvailable(
      L"exercise",
      restartedRequester.membership->id,
      registered.objectInstanceHandle,
      restoredWork.requestId);
  REQUIRE(unavailable.has_value());
  REQUIRE(unavailable->securedAttributeHandles.empty());
  REQUIRE(unavailable->unavailableAttributeHandles == efficiencyOnly);
  REQUIRE_FALSE(restarted.beginAttributeOwnershipAcquisitionIfAvailable(
      L"exercise",
      restartedRequester.membership->id,
      registered.objectInstanceHandle,
      restoredWork.requestId));

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores pending negotiated owner confirmation in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][ownership-ledger-state][attribute-ownership-acquisition]"
    "[negotiated-attribute-ownership-divestiture]"
    "[process-restart-attribute-ownership-negotiated-owner-confirmation]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto requester = source.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(owner.status == FederationRegistryStatus::applied);
  REQUIRE(requester.status == FederationRegistryStatus::applied);
  REQUIRE(owner.membership);
  REQUIRE(requester.membership);

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", requester.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(discoveries.front().receivingFederateId == requester.membership->id);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise",
      requester.membership->id,
      registered.objectInstanceHandle)
      .has_value());

  // The requester must publish after discovery so the saved object contains
  // both live known-class projections and a valid publication precondition.
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", requester.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  std::string const valueBytes{"\x75\x76", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *efficiency,
      rti1516_2025::VariableLengthData(valueBytes.data(), valueBytes.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise",
      owner.membership->id,
      registered.objectInstanceHandle,
      *server,
      {"HLAreliable"},
      &values) == FederationRegistryStatus::applied);

  std::vector<unsigned char> const acquisitionTag{'n', 'e', 'g', '-', 'r', 'e', 's', 't', 'a', 'r', 't'};
  auto acquisition = source.planAttributeOwnershipAcquisition(
      L"exercise",
      requester.membership->id,
      registered.objectInstanceHandle,
      efficiencyOnly,
      acquisitionTag);
  REQUIRE(acquisition.status ==
      umbra::detail::AttributeOwnershipAcquisitionStatus::applied);
  REQUIRE(acquisition.workItems.size() == 1U);
  REQUIRE(acquisition.workItems.front().kind ==
      umbra::detail::AttributeOwnershipAcquisitionWorkKind::request_release);

  std::vector<unsigned char> const divestitureTag{'d', 'i', 'v', '-', 'r', 'e', 's', 't', 'a', 'r', 't'};
  auto divestiture = source.planNegotiatedAttributeOwnershipDivestiture(
      L"exercise",
      owner.membership->id,
      registered.objectInstanceHandle,
      efficiencyOnly,
      divestitureTag);
  REQUIRE(divestiture.status ==
      umbra::detail::NegotiatedAttributeOwnershipDivestitureStatus::applied);
  REQUIRE(divestiture.workItems.size() == 1U);
  auto const& sourceConfirmation = divestiture.workItems.front();
  REQUIRE(sourceConfirmation.kind ==
      umbra::detail::AttributeOwnershipAcquisitionWorkKind::request_divestiture_confirmation);
  REQUIRE(sourceConfirmation.requestingFederateId == requester.membership->id);
  REQUIRE(sourceConfirmation.receivingFederateId == owner.membership->id);
  REQUIRE(sourceConfirmation.objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(sourceConfirmation.attributeHandles == efficiencyOnly);
  REQUIRE(sourceConfirmation.userSuppliedTag == acquisitionTag);

  std::wstring const saveLabel = L"ownership-negotiated-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", requester.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", requester.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.handle == registered.objectInstanceHandle);
  REQUIRE(savedObject.attributeValuesPresent);
  REQUIRE(savedObject.attributeValues.size() == 1U);
  REQUIRE(savedObject.attributeValues.front().attributeHandle == *efficiency);
  REQUIRE(savedObject.attributeValues.front().value == valueBytes);
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionRequests.size() == 1U);
  auto const& savedRequest = savedObject.pendingAttributeOwnershipAcquisitionRequests.front();
  REQUIRE(savedRequest.requestId == sourceConfirmation.requestId);
  REQUIRE(savedRequest.requestingFederateId == requester.membership->id);
  REQUIRE(savedRequest.desiredAttributeHandles ==
      std::vector<std::uint64_t>{*efficiency});
  REQUIRE(savedRequest.unavailableQueuedAttributeHandles.empty());
  REQUIRE(savedRequest.releaseCallbacksQueuedByOwningFederate.size() == 1U);
  REQUIRE(savedRequest.releaseCallbacksQueuedByOwningFederate.front().first ==
      owner.membership->id);
  REQUIRE(savedRequest.releaseCallbacksQueuedByOwningFederate.front().second ==
      std::vector<std::uint64_t>{*efficiency});
  REQUIRE(savedObject.pendingNegotiatedAttributeOwnershipDivestitures.size() == 1U);
  auto const& savedDivestiture =
      savedObject.pendingNegotiatedAttributeOwnershipDivestitures.front();
  REQUIRE(savedDivestiture.attributeHandle == *efficiency);
  REQUIRE(savedDivestiture.divestingFederateId == owner.membership->id);
  REQUIRE(savedDivestiture.acquiringFederateId == requester.membership->id);
  REQUIRE(savedDivestiture.acquisitionRequestId == savedRequest.requestId);
  REQUIRE_FALSE(savedDivestiture.acquiringFederateIsIfAvailable);
  REQUIRE(savedDivestiture.confirmationQueued);
  REQUIRE_FALSE(savedDivestiture.confirmationDelivered);
  REQUIRE(savedDivestiture.userSuppliedTag ==
      std::string(divestitureTag.begin(), divestitureTag.end()));

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedOwner = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto restartedRequester = restarted.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedRequester.membership);
  REQUIRE(restartedOwner.membership->id == owner.membership->id);
  REQUIRE(restartedRequester.membership->id == requester.membership->id);

  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedRequester.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto firstRestore = restarted.federateRestoreComplete(
      L"exercise", restartedOwner.membership->id);
  REQUIRE(firstRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(firstRestore.ownershipAcquisitionWorkItems.empty());
  auto restored = restarted.federateRestoreComplete(
      L"exercise", restartedRequester.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.ownershipAcquisitionWorkItems.size() == 1U);
  auto const& restoredWork = restored.ownershipAcquisitionWorkItems.front();
  REQUIRE(restoredWork.kind ==
      umbra::detail::AttributeOwnershipAcquisitionWorkKind::request_divestiture_confirmation);
  REQUIRE(restoredWork.requestingFederateId == restartedRequester.membership->id);
  REQUIRE(restoredWork.receivingFederateId == restartedOwner.membership->id);
  REQUIRE(restoredWork.objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(restoredWork.requestId == savedRequest.requestId);
  REQUIRE(restoredWork.attributeHandles == efficiencyOnly);
  REQUIRE(restoredWork.userSuppliedTag == acquisitionTag);
  REQUIRE_FALSE(restoredWork.candidateIsIfAvailable);

  auto restoredOwnerState = restarted.attributeOwnedByFederate(
      L"exercise",
      restartedOwner.membership->id,
      registered.objectInstanceHandle,
      *efficiency);
  REQUIRE(restoredOwnerState.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE(restoredOwnerState.ownedByRequestingFederate);
  auto confirmation = restarted.beginRequestDivestitureConfirmation(
      L"exercise",
      restartedOwner.membership->id,
      restartedRequester.membership->id,
      registered.objectInstanceHandle,
      restoredWork.requestId,
      false,
      restoredWork.attributeHandles);
  REQUIRE(confirmation.has_value());
  REQUIRE(confirmation->objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(confirmation->releasedAttributeHandles == efficiencyOnly);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores pending negotiated If Available owner confirmation in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][ownership-ledger-state][attribute-ownership-acquisition]"
    "[attribute-ownership-acquisition-if-available][negotiated-attribute-ownership-divestiture]"
    "[process-restart-attribute-ownership-negotiated-if-available-owner-confirmation]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto requester = source.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(owner.status == FederationRegistryStatus::applied);
  REQUIRE(requester.status == FederationRegistryStatus::applied);
  REQUIRE(owner.membership);
  REQUIRE(requester.membership);

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", requester.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(discoveries.front().receivingFederateId == requester.membership->id);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise",
      requester.membership->id,
      registered.objectInstanceHandle)
      .has_value());

  // Keep the requester published after discovery so the saved object carries
  // the same live publication precondition used by the callback boundary.
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", requester.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  std::string const valueBytes{"\x77\x78", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *efficiency,
      rti1516_2025::VariableLengthData(valueBytes.data(), valueBytes.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise",
      owner.membership->id,
      registered.objectInstanceHandle,
      *server,
      {"HLAreliable"},
      &values) == FederationRegistryStatus::applied);

  std::vector<unsigned char> const acquisitionTag{
      'w', 't', 'a', '-', 'n', 'e', 'g', '-', 'r', 'e', 's', 't', 'a', 'r', 't'};
  auto acquisition = source.planAttributeOwnershipAcquisitionIfAvailable(
      L"exercise",
      requester.membership->id,
      registered.objectInstanceHandle,
      efficiencyOnly,
      acquisitionTag);
  REQUIRE(acquisition.status ==
      umbra::detail::AttributeOwnershipAcquisitionIfAvailableStatus::applied);
  REQUIRE(acquisition.requestId != 0U);
  REQUIRE(acquisition.callbackRoute);

  std::vector<unsigned char> const divestitureTag{
      'd', 'i', 'v', '-', 'w', 't', 'a', '-', 'r', 'e', 's', 't', 'a', 'r', 't'};
  auto divestiture = source.planNegotiatedAttributeOwnershipDivestiture(
      L"exercise",
      owner.membership->id,
      registered.objectInstanceHandle,
      efficiencyOnly,
      divestitureTag);
  REQUIRE(divestiture.status ==
      umbra::detail::NegotiatedAttributeOwnershipDivestitureStatus::applied);
  REQUIRE(divestiture.workItems.size() == 1U);
  auto const& sourceConfirmation = divestiture.workItems.front();
  REQUIRE(sourceConfirmation.kind ==
      umbra::detail::AttributeOwnershipAcquisitionWorkKind::request_divestiture_confirmation);
  REQUIRE(sourceConfirmation.requestingFederateId == requester.membership->id);
  REQUIRE(sourceConfirmation.receivingFederateId == owner.membership->id);
  REQUIRE(sourceConfirmation.objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(sourceConfirmation.requestId == acquisition.requestId);
  REQUIRE(sourceConfirmation.attributeHandles == efficiencyOnly);
  REQUIRE(sourceConfirmation.userSuppliedTag == acquisitionTag);
  REQUIRE(sourceConfirmation.candidateIsIfAvailable);

  std::wstring const saveLabel = L"ownership-negotiated-if-available-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", requester.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", requester.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.handle == registered.objectInstanceHandle);
  REQUIRE(savedObject.attributeValuesPresent);
  REQUIRE(savedObject.attributeValues.size() == 1U);
  REQUIRE(savedObject.attributeValues.front().attributeHandle == *efficiency);
  REQUIRE(savedObject.attributeValues.front().value == valueBytes);
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionRequests.empty());
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionIfAvailableRequests.size() == 1U);
  auto const& savedRequest =
      savedObject.pendingAttributeOwnershipAcquisitionIfAvailableRequests.front();
  REQUIRE(savedRequest.requestId == acquisition.requestId);
  REQUIRE(savedRequest.requestingFederateId == requester.membership->id);
  REQUIRE(savedRequest.requestSequence != 0U);
  REQUIRE(savedRequest.desiredAttributeHandles ==
      std::vector<std::uint64_t>{*efficiency});
  REQUIRE(savedRequest.userSuppliedTag ==
      std::string(acquisitionTag.begin(), acquisitionTag.end()));
  REQUIRE(savedObject.pendingNegotiatedAttributeOwnershipDivestitures.size() == 1U);
  auto const& savedDivestiture =
      savedObject.pendingNegotiatedAttributeOwnershipDivestitures.front();
  REQUIRE(savedDivestiture.attributeHandle == *efficiency);
  REQUIRE(savedDivestiture.divestingFederateId == owner.membership->id);
  REQUIRE(savedDivestiture.acquiringFederateId == requester.membership->id);
  REQUIRE(savedDivestiture.acquisitionRequestId == savedRequest.requestId);
  REQUIRE(savedDivestiture.acquiringFederateIsIfAvailable);
  REQUIRE(savedDivestiture.confirmationQueued);
  REQUIRE_FALSE(savedDivestiture.confirmationDelivered);
  REQUIRE(savedDivestiture.userSuppliedTag ==
      std::string(divestitureTag.begin(), divestitureTag.end()));

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedOwner = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto restartedRequester = restarted.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedRequester.membership);
  REQUIRE(restartedOwner.membership->id == owner.membership->id);
  REQUIRE(restartedRequester.membership->id == requester.membership->id);

  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedRequester.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto firstRestore = restarted.federateRestoreComplete(
      L"exercise", restartedOwner.membership->id);
  REQUIRE(firstRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(firstRestore.ownershipAcquisitionWorkItems.empty());
  auto restored = restarted.federateRestoreComplete(
      L"exercise", restartedRequester.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.ownershipAcquisitionWorkItems.size() == 1U);
  auto const& restoredWork = restored.ownershipAcquisitionWorkItems.front();
  REQUIRE(restoredWork.kind ==
      umbra::detail::AttributeOwnershipAcquisitionWorkKind::request_divestiture_confirmation);
  REQUIRE(restoredWork.requestingFederateId == restartedRequester.membership->id);
  REQUIRE(restoredWork.receivingFederateId == restartedOwner.membership->id);
  REQUIRE(restoredWork.objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(restoredWork.requestId == savedRequest.requestId);
  REQUIRE(restoredWork.attributeHandles == efficiencyOnly);
  REQUIRE(restoredWork.userSuppliedTag == acquisitionTag);
  REQUIRE(restoredWork.candidateIsIfAvailable);

  auto restoredOwnerState = restarted.attributeOwnedByFederate(
      L"exercise",
      restartedOwner.membership->id,
      registered.objectInstanceHandle,
      *efficiency);
  REQUIRE(restoredOwnerState.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE(restoredOwnerState.ownedByRequestingFederate);
  auto confirmation = restarted.beginRequestDivestitureConfirmation(
      L"exercise",
      restartedOwner.membership->id,
      restartedRequester.membership->id,
      registered.objectInstanceHandle,
      restoredWork.requestId,
      true,
      restoredWork.attributeHandles);
  REQUIRE(confirmation.has_value());
  REQUIRE(confirmation->objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(confirmation->releasedAttributeHandles == efficiencyOnly);

  auto confirmed = restarted.planConfirmDivestiture(
      L"exercise",
      restartedOwner.membership->id,
      registered.objectInstanceHandle,
      efficiencyOnly,
      divestitureTag);
  REQUIRE(confirmed.status == umbra::detail::ConfirmDivestitureStatus::applied);
  REQUIRE(confirmed.notifications.size() == 1U);
  auto const& notification = confirmed.notifications.front();
  REQUIRE(notification.receivingFederateId == restartedRequester.membership->id);
  REQUIRE(notification.objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(notification.attributeHandles == efficiencyOnly);
  auto notificationDelivery = restarted.beginConfirmDivestitureNotification(
      L"exercise",
      restartedRequester.membership->id,
      registered.objectInstanceHandle,
      notification.notificationId,
      notification.attributeHandles);
  REQUIRE(notificationDelivery.has_value());
  REQUIRE(notificationDelivery->securedAttributeHandles == efficiencyOnly);

  auto requesterOwnership = restarted.attributeOwnedByFederate(
      L"exercise",
      restartedRequester.membership->id,
      registered.objectInstanceHandle,
      *efficiency);
  REQUIRE(requesterOwnership.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE(requesterOwnership.ownedByRequestingFederate);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores a mixed negotiated ownership ledger in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][ownership-ledger-state][attribute-ownership-acquisition]"
    "[attribute-ownership-acquisition-if-available][negotiated-attribute-ownership-divestiture]"
    "[process-restart-attribute-ownership-mixed-negotiated-ownership]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto requester = source.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(owner.status == FederationRegistryStatus::applied);
  REQUIRE(requester.status == FederationRegistryStatus::applied);
  REQUIRE(owner.membership);
  REQUIRE(requester.membership);

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  auto const cheerfulness = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Cheerfulness");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  REQUIRE(cheerfulness.has_value());
  std::set<std::uint64_t> const mixedAttributes{*efficiency, *cheerfulness};
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  std::set<std::uint64_t> const cheerfulnessOnly{*cheerfulness};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *server, mixedAttributes, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", requester.membership->id, *server, mixedAttributes, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(discoveries.front().receivingFederateId == requester.membership->id);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise",
      requester.membership->id,
      registered.objectInstanceHandle)
      .has_value());
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", requester.membership->id, *server, mixedAttributes, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  std::string const efficiencyValue{"\x75\x76", 2U};
  std::string const cheerfulnessValue{"\x77\x78", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{
      {*efficiency,
       rti1516_2025::VariableLengthData(efficiencyValue.data(), efficiencyValue.size())},
      {*cheerfulness,
       rti1516_2025::VariableLengthData(cheerfulnessValue.data(), cheerfulnessValue.size())},
  };
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise",
      owner.membership->id,
      registered.objectInstanceHandle,
      *server,
      {"HLAreliable"},
      &values) == FederationRegistryStatus::applied);

  std::vector<unsigned char> const regularTag{
      'r', 'e', 'g', '-', 'm', 'i', 'x', '-', 'r', 'e', 's', 't', 'a', 'r', 't'};
  auto regular = source.planAttributeOwnershipAcquisition(
      L"exercise",
      requester.membership->id,
      registered.objectInstanceHandle,
      efficiencyOnly,
      regularTag);
  REQUIRE(regular.status ==
      umbra::detail::AttributeOwnershipAcquisitionStatus::applied);
  REQUIRE(regular.workItems.size() == 1U);
  REQUIRE(regular.workItems.front().kind ==
      umbra::detail::AttributeOwnershipAcquisitionWorkKind::request_release);
  auto const regularRequestId = regular.workItems.front().requestId;

  std::vector<unsigned char> const ifAvailableTag{
      'w', 't', 'a', '-', 'm', 'i', 'x', '-', 'r', 'e', 's', 't', 'a', 'r', 't'};
  auto ifAvailable = source.planAttributeOwnershipAcquisitionIfAvailable(
      L"exercise",
      requester.membership->id,
      registered.objectInstanceHandle,
      cheerfulnessOnly,
      ifAvailableTag);
  REQUIRE(ifAvailable.status ==
      umbra::detail::AttributeOwnershipAcquisitionIfAvailableStatus::applied);
  REQUIRE(ifAvailable.requestId != 0U);

  std::vector<unsigned char> const divestitureTag{
      'd', 'i', 'v', '-', 'm', 'i', 'x', '-', 'r', 'e', 's', 't', 'a', 'r', 't'};
  auto divestiture = source.planNegotiatedAttributeOwnershipDivestiture(
      L"exercise",
      owner.membership->id,
      registered.objectInstanceHandle,
      mixedAttributes,
      divestitureTag);
  REQUIRE(divestiture.status ==
      umbra::detail::NegotiatedAttributeOwnershipDivestitureStatus::applied);
  REQUIRE(divestiture.workItems.size() == 2U);
  auto const regularConfirmation = std::ranges::find_if(
      divestiture.workItems,
      [](umbra::detail::AttributeOwnershipAcquisitionWorkItem const& work) {
        return work.candidateIsIfAvailable == false;
      });
  auto const ifAvailableConfirmation = std::ranges::find_if(
      divestiture.workItems,
      [](umbra::detail::AttributeOwnershipAcquisitionWorkItem const& work) {
        return work.candidateIsIfAvailable;
      });
  REQUIRE(regularConfirmation != divestiture.workItems.end());
  REQUIRE(ifAvailableConfirmation != divestiture.workItems.end());
  REQUIRE(regularConfirmation->requestingFederateId == requester.membership->id);
  REQUIRE(regularConfirmation->receivingFederateId == owner.membership->id);
  REQUIRE(regularConfirmation->requestId == regularRequestId);
  REQUIRE(regularConfirmation->attributeHandles == efficiencyOnly);
  REQUIRE(regularConfirmation->userSuppliedTag == regularTag);
  REQUIRE(ifAvailableConfirmation->requestingFederateId == requester.membership->id);
  REQUIRE(ifAvailableConfirmation->receivingFederateId == owner.membership->id);
  REQUIRE(ifAvailableConfirmation->requestId == ifAvailable.requestId);
  REQUIRE(ifAvailableConfirmation->attributeHandles == cheerfulnessOnly);
  REQUIRE(ifAvailableConfirmation->userSuppliedTag == ifAvailableTag);

  std::wstring const saveLabel = L"ownership-negotiated-mixed-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", requester.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", requester.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.handle == registered.objectInstanceHandle);
  REQUIRE(savedObject.attributeValuesPresent);
  REQUIRE(savedObject.attributeValues.size() == 2U);
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionRequests.size() == 1U);
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionIfAvailableRequests.size() == 1U);
  auto const& savedRegular = savedObject.pendingAttributeOwnershipAcquisitionRequests.front();
  auto const& savedIfAvailable =
      savedObject.pendingAttributeOwnershipAcquisitionIfAvailableRequests.front();
  REQUIRE(savedRegular.requestId == regularRequestId);
  REQUIRE(savedRegular.requestingFederateId == requester.membership->id);
  REQUIRE(savedRegular.desiredAttributeHandles ==
      std::vector<std::uint64_t>{*efficiency});
  REQUIRE(savedRegular.releaseCallbacksQueuedByOwningFederate.size() == 1U);
  REQUIRE(savedIfAvailable.requestId == ifAvailable.requestId);
  REQUIRE(savedIfAvailable.requestingFederateId == requester.membership->id);
  REQUIRE(savedIfAvailable.desiredAttributeHandles ==
      std::vector<std::uint64_t>{*cheerfulness});
  REQUIRE(savedObject.pendingNegotiatedAttributeOwnershipDivestitures.size() == 2U);
  auto const savedRegularDivestiture = std::ranges::find_if(
      savedObject.pendingNegotiatedAttributeOwnershipDivestitures,
      [&](umbra::detail::FederationStateImagePendingNegotiatedAttributeOwnershipDivestiture const& saved) {
        return saved.attributeHandle == *efficiency;
      });
  auto const savedIfAvailableDivestiture = std::ranges::find_if(
      savedObject.pendingNegotiatedAttributeOwnershipDivestitures,
      [&](umbra::detail::FederationStateImagePendingNegotiatedAttributeOwnershipDivestiture const& saved) {
        return saved.attributeHandle == *cheerfulness;
      });
  REQUIRE(savedRegularDivestiture !=
      savedObject.pendingNegotiatedAttributeOwnershipDivestitures.end());
  REQUIRE(savedIfAvailableDivestiture !=
      savedObject.pendingNegotiatedAttributeOwnershipDivestitures.end());
  REQUIRE_FALSE(savedRegularDivestiture->acquiringFederateIsIfAvailable);
  REQUIRE(savedRegularDivestiture->confirmationQueued);
  REQUIRE_FALSE(savedRegularDivestiture->confirmationDelivered);
  REQUIRE(savedIfAvailableDivestiture->acquiringFederateIsIfAvailable);
  REQUIRE(savedIfAvailableDivestiture->confirmationQueued);
  REQUIRE_FALSE(savedIfAvailableDivestiture->confirmationDelivered);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedOwner = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto restartedRequester = restarted.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedRequester.membership);
  REQUIRE(restartedOwner.membership->id == owner.membership->id);
  REQUIRE(restartedRequester.membership->id == requester.membership->id);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedRequester.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto firstRestore = restarted.federateRestoreComplete(
      L"exercise", restartedOwner.membership->id);
  REQUIRE(firstRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(firstRestore.ownershipAcquisitionWorkItems.empty());
  auto restored = restarted.federateRestoreComplete(
      L"exercise", restartedRequester.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.ownershipAcquisitionWorkItems.size() == 2U);
  auto const restoredRegularConfirmation = std::ranges::find_if(
      restored.ownershipAcquisitionWorkItems,
      [](umbra::detail::AttributeOwnershipAcquisitionWorkItem const& work) {
        return !work.candidateIsIfAvailable;
      });
  auto const restoredIfAvailableConfirmation = std::ranges::find_if(
      restored.ownershipAcquisitionWorkItems,
      [](umbra::detail::AttributeOwnershipAcquisitionWorkItem const& work) {
        return work.candidateIsIfAvailable;
      });
  REQUIRE(restoredRegularConfirmation != restored.ownershipAcquisitionWorkItems.end());
  REQUIRE(restoredIfAvailableConfirmation != restored.ownershipAcquisitionWorkItems.end());
  REQUIRE(restoredRegularConfirmation->kind ==
      umbra::detail::AttributeOwnershipAcquisitionWorkKind::request_divestiture_confirmation);
  REQUIRE(restoredIfAvailableConfirmation->kind ==
      umbra::detail::AttributeOwnershipAcquisitionWorkKind::request_divestiture_confirmation);
  REQUIRE(restoredRegularConfirmation->requestId == savedRegular.requestId);
  REQUIRE(restoredRegularConfirmation->attributeHandles == efficiencyOnly);
  REQUIRE(restoredRegularConfirmation->userSuppliedTag == regularTag);
  REQUIRE(restoredIfAvailableConfirmation->requestId == savedIfAvailable.requestId);
  REQUIRE(restoredIfAvailableConfirmation->attributeHandles == cheerfulnessOnly);
  REQUIRE(restoredIfAvailableConfirmation->userSuppliedTag == ifAvailableTag);

  REQUIRE(restarted.attributeOwnedByFederate(
      L"exercise", restartedOwner.membership->id,
      registered.objectInstanceHandle, *efficiency).ownedByRequestingFederate);
  REQUIRE(restarted.attributeOwnedByFederate(
      L"exercise", restartedOwner.membership->id,
      registered.objectInstanceHandle, *cheerfulness).ownedByRequestingFederate);
  REQUIRE(restarted.beginRequestDivestitureConfirmation(
      L"exercise", restartedOwner.membership->id,
      restartedRequester.membership->id,
      registered.objectInstanceHandle,
      restoredRegularConfirmation->requestId,
      false,
      efficiencyOnly));
  REQUIRE(restarted.beginRequestDivestitureConfirmation(
      L"exercise", restartedOwner.membership->id,
      restartedRequester.membership->id,
      registered.objectInstanceHandle,
      restoredIfAvailableConfirmation->requestId,
      true,
      cheerfulnessOnly));

  auto confirmed = restarted.planConfirmDivestiture(
      L"exercise",
      restartedOwner.membership->id,
      registered.objectInstanceHandle,
      mixedAttributes,
      divestitureTag);
  REQUIRE(confirmed.status == umbra::detail::ConfirmDivestitureStatus::applied);
  REQUIRE(confirmed.notifications.size() == 1U);
  auto const& notification = confirmed.notifications.front();
  REQUIRE(notification.receivingFederateId == restartedRequester.membership->id);
  REQUIRE(notification.attributeHandles == mixedAttributes);
  auto notificationDelivery = restarted.beginConfirmDivestitureNotification(
      L"exercise",
      restartedRequester.membership->id,
      registered.objectInstanceHandle,
      notification.notificationId,
      notification.attributeHandles);
  REQUIRE(notificationDelivery.has_value());
  REQUIRE(notificationDelivery->securedAttributeHandles == mixedAttributes);
  REQUIRE(restarted.attributeOwnedByFederate(
      L"exercise", restartedRequester.membership->id,
      registered.objectInstanceHandle, *efficiency).ownedByRequestingFederate);
  REQUIRE(restarted.attributeOwnedByFederate(
      L"exercise", restartedRequester.membership->id,
      registered.objectInstanceHandle, *cheerfulness).ownedByRequestingFederate);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores the reverse asymmetric mixed negotiated confirmation in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-asymmetric-mixed-confirmation-reverse][ownership-ledger-state]"
    "[attribute-ownership-acquisition][attribute-ownership-acquisition-if-available]"
    "[negotiated-attribute-ownership-divestiture]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto requester = source.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(owner.membership);
  REQUIRE(requester.membership);

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  auto const cheerfulness = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Cheerfulness");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  REQUIRE(cheerfulness.has_value());
  std::set<std::uint64_t> const mixedAttributes{*efficiency, *cheerfulness};
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  std::set<std::uint64_t> const cheerfulnessOnly{*cheerfulness};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *server, mixedAttributes, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", requester.membership->id, *server, mixedAttributes, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", requester.membership->id,
      registered.objectInstanceHandle));
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", requester.membership->id, *server, mixedAttributes, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  std::string const efficiencyValue{"\x85\x86", 2U};
  std::string const cheerfulnessValue{"\x87\x88", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{
      {*efficiency,
       rti1516_2025::VariableLengthData(efficiencyValue.data(), efficiencyValue.size())},
      {*cheerfulness,
       rti1516_2025::VariableLengthData(cheerfulnessValue.data(), cheerfulnessValue.size())},
  };
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise", owner.membership->id, registered.objectInstanceHandle,
      *server, {"HLAreliable"}, &values) == FederationRegistryStatus::applied);

  std::vector<unsigned char> const regularTag{
      'r', 'e', 'g', '-', 'm', 'i', 'x', '-', 'r', 'e', 'v'};
  auto regular = source.planAttributeOwnershipAcquisition(
      L"exercise", requester.membership->id, registered.objectInstanceHandle,
      efficiencyOnly, regularTag);
  REQUIRE(regular.status ==
      umbra::detail::AttributeOwnershipAcquisitionStatus::applied);
  REQUIRE(regular.workItems.size() == 1U);
  auto const regularRequestId = regular.workItems.front().requestId;

  std::vector<unsigned char> const ifAvailableTag{
      'w', 't', 'a', '-', 'm', 'i', 'x', '-', 'r', 'e', 'v'};
  auto ifAvailable = source.planAttributeOwnershipAcquisitionIfAvailable(
      L"exercise", requester.membership->id, registered.objectInstanceHandle,
      cheerfulnessOnly, ifAvailableTag);
  REQUIRE(ifAvailable.status ==
      umbra::detail::AttributeOwnershipAcquisitionIfAvailableStatus::applied);
  REQUIRE(ifAvailable.requestId != 0U);

  std::vector<unsigned char> const divestitureTag{
      'd', 'i', 'v', '-', 'm', 'i', 'x', '-', 'r', 'e', 'v'};
  auto divestiture = source.planNegotiatedAttributeOwnershipDivestiture(
      L"exercise", owner.membership->id, registered.objectInstanceHandle,
      mixedAttributes, divestitureTag);
  REQUIRE(divestiture.status ==
      umbra::detail::NegotiatedAttributeOwnershipDivestitureStatus::applied);
  REQUIRE(divestiture.workItems.size() == 2U);
  auto const regularConfirmation = std::ranges::find_if(
      divestiture.workItems,
      [](umbra::detail::AttributeOwnershipAcquisitionWorkItem const& work) {
        return !work.candidateIsIfAvailable;
      });
  auto const ifAvailableConfirmation = std::ranges::find_if(
      divestiture.workItems,
      [](umbra::detail::AttributeOwnershipAcquisitionWorkItem const& work) {
        return work.candidateIsIfAvailable;
      });
  REQUIRE(regularConfirmation != divestiture.workItems.end());
  REQUIRE(ifAvailableConfirmation != divestiture.workItems.end());
  REQUIRE(regularConfirmation->requestId == regularRequestId);
  REQUIRE(ifAvailableConfirmation->requestId == ifAvailable.requestId);
  REQUIRE(source.beginRequestDivestitureConfirmation(
      L"exercise", owner.membership->id, requester.membership->id,
      registered.objectInstanceHandle, ifAvailable.requestId, true,
      cheerfulnessOnly));
  REQUIRE_FALSE(source.beginRequestDivestitureConfirmation(
      L"exercise", owner.membership->id, requester.membership->id,
      registered.objectInstanceHandle, ifAvailable.requestId, true,
      cheerfulnessOnly));

  std::wstring const saveLabel = L"ownership-negotiated-mixed-asymmetric-reverse-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", requester.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", requester.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.attributeValuesPresent);
  REQUIRE(savedObject.attributeValues.size() == 2U);
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionRequests.size() == 1U);
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionIfAvailableRequests.size() == 1U);
  REQUIRE(savedObject.pendingNegotiatedAttributeOwnershipDivestitures.size() == 2U);
  auto const savedRegular = std::ranges::find_if(
      savedObject.pendingNegotiatedAttributeOwnershipDivestitures,
      [&](umbra::detail::FederationStateImagePendingNegotiatedAttributeOwnershipDivestiture const& saved) {
        return saved.attributeHandle == *efficiency;
      });
  auto const savedIfAvailable = std::ranges::find_if(
      savedObject.pendingNegotiatedAttributeOwnershipDivestitures,
      [&](umbra::detail::FederationStateImagePendingNegotiatedAttributeOwnershipDivestiture const& saved) {
        return saved.attributeHandle == *cheerfulness;
      });
  REQUIRE(savedRegular != savedObject.pendingNegotiatedAttributeOwnershipDivestitures.end());
  REQUIRE(savedIfAvailable != savedObject.pendingNegotiatedAttributeOwnershipDivestitures.end());
  REQUIRE_FALSE(savedRegular->confirmationDelivered);
  REQUIRE(savedIfAvailable->confirmationQueued);
  REQUIRE(savedIfAvailable->confirmationDelivered);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedOwner = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto restartedRequester = restarted.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedRequester.membership);
  REQUIRE(restartedOwner.membership->id == owner.membership->id);
  REQUIRE(restartedRequester.membership->id == requester.membership->id);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedRequester.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto firstRestore = restarted.federateRestoreComplete(
      L"exercise", restartedOwner.membership->id);
  REQUIRE(firstRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(firstRestore.ownershipAcquisitionWorkItems.empty());
  auto restored = restarted.federateRestoreComplete(
      L"exercise", restartedRequester.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.ownershipAcquisitionWorkItems.size() == 1U);
  auto const restoredRegularConfirmation = std::ranges::find_if(
      restored.ownershipAcquisitionWorkItems,
      [](umbra::detail::AttributeOwnershipAcquisitionWorkItem const& work) {
        return !work.candidateIsIfAvailable;
      });
  REQUIRE(restoredRegularConfirmation != restored.ownershipAcquisitionWorkItems.end());
  REQUIRE(restoredRegularConfirmation->kind ==
      umbra::detail::AttributeOwnershipAcquisitionWorkKind::request_divestiture_confirmation);
  REQUIRE(restoredRegularConfirmation->requestId == regularRequestId);
  REQUIRE(restoredRegularConfirmation->attributeHandles == efficiencyOnly);
  REQUIRE_FALSE(restarted.beginRequestDivestitureConfirmation(
      L"exercise", restartedOwner.membership->id,
      restartedRequester.membership->id, registered.objectInstanceHandle,
      ifAvailable.requestId, true, cheerfulnessOnly));
  REQUIRE(restarted.beginRequestDivestitureConfirmation(
      L"exercise", restartedOwner.membership->id,
      restartedRequester.membership->id, registered.objectInstanceHandle,
      regularRequestId, false, efficiencyOnly));
  REQUIRE_FALSE(restarted.beginRequestDivestitureConfirmation(
      L"exercise", restartedOwner.membership->id,
      restartedRequester.membership->id, registered.objectInstanceHandle,
      regularRequestId, false, efficiencyOnly));
  REQUIRE(restarted.attributeOwnedByFederate(
      L"exercise", restartedOwner.membership->id,
      registered.objectInstanceHandle, *efficiency).ownedByRequestingFederate);
  REQUIRE(restarted.attributeOwnedByFederate(
      L"exercise", restartedOwner.membership->id,
      registered.objectInstanceHandle, *cheerfulness).ownedByRequestingFederate);

  auto confirmed = restarted.planConfirmDivestiture(
      L"exercise", restartedOwner.membership->id,
      registered.objectInstanceHandle, mixedAttributes, divestitureTag);
  REQUIRE(confirmed.status == umbra::detail::ConfirmDivestitureStatus::applied);
  REQUIRE(confirmed.notifications.size() == 1U);
  auto const& notification = confirmed.notifications.front();
  REQUIRE(notification.attributeHandles == mixedAttributes);
  auto notificationDelivery = restarted.beginConfirmDivestitureNotification(
      L"exercise", restartedRequester.membership->id,
      registered.objectInstanceHandle, notification.notificationId,
      notification.attributeHandles);
  REQUIRE(notificationDelivery.has_value());
  REQUIRE(notificationDelivery->securedAttributeHandles == mixedAttributes);
  REQUIRE(restarted.attributeOwnedByFederate(
      L"exercise", restartedRequester.membership->id,
      registered.objectInstanceHandle, *efficiency).ownedByRequestingFederate);
  REQUIRE(restarted.attributeOwnedByFederate(
      L"exercise", restartedRequester.membership->id,
      registered.objectInstanceHandle, *cheerfulness).ownedByRequestingFederate);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores an asymmetric mixed negotiated confirmation in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-asymmetric-mixed-confirmation][ownership-ledger-state]"
    "[attribute-ownership-acquisition][attribute-ownership-acquisition-if-available]"
    "[negotiated-attribute-ownership-divestiture]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto requester = source.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(owner.membership);
  REQUIRE(requester.membership);

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  auto const cheerfulness = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Cheerfulness");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  REQUIRE(cheerfulness.has_value());
  std::set<std::uint64_t> const mixedAttributes{*efficiency, *cheerfulness};
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  std::set<std::uint64_t> const cheerfulnessOnly{*cheerfulness};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *server, mixedAttributes, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", requester.membership->id, *server, mixedAttributes, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", requester.membership->id,
      registered.objectInstanceHandle));
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", requester.membership->id, *server, mixedAttributes, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  std::string const efficiencyValue{"\x81\x82", 2U};
  std::string const cheerfulnessValue{"\x83\x84", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{
      {*efficiency,
       rti1516_2025::VariableLengthData(efficiencyValue.data(), efficiencyValue.size())},
      {*cheerfulness,
       rti1516_2025::VariableLengthData(cheerfulnessValue.data(), cheerfulnessValue.size())},
  };
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise", owner.membership->id, registered.objectInstanceHandle,
      *server, {"HLAreliable"}, &values) == FederationRegistryStatus::applied);

  std::vector<unsigned char> const regularTag{
      'r', 'e', 'g', '-', 'm', 'i', 'x', '-', 'a', 's', 'y', 'm'};
  auto regular = source.planAttributeOwnershipAcquisition(
      L"exercise", requester.membership->id, registered.objectInstanceHandle,
      efficiencyOnly, regularTag);
  REQUIRE(regular.status ==
      umbra::detail::AttributeOwnershipAcquisitionStatus::applied);
  REQUIRE(regular.workItems.size() == 1U);
  auto const regularRequestId = regular.workItems.front().requestId;

  std::vector<unsigned char> const ifAvailableTag{
      'w', 't', 'a', '-', 'm', 'i', 'x', '-', 'a', 's', 'y', 'm'};
  auto ifAvailable = source.planAttributeOwnershipAcquisitionIfAvailable(
      L"exercise", requester.membership->id, registered.objectInstanceHandle,
      cheerfulnessOnly, ifAvailableTag);
  REQUIRE(ifAvailable.status ==
      umbra::detail::AttributeOwnershipAcquisitionIfAvailableStatus::applied);
  REQUIRE(ifAvailable.requestId != 0U);

  std::vector<unsigned char> const divestitureTag{
      'd', 'i', 'v', '-', 'm', 'i', 'x', '-', 'a', 's', 'y', 'm'};
  auto divestiture = source.planNegotiatedAttributeOwnershipDivestiture(
      L"exercise", owner.membership->id, registered.objectInstanceHandle,
      mixedAttributes, divestitureTag);
  REQUIRE(divestiture.status ==
      umbra::detail::NegotiatedAttributeOwnershipDivestitureStatus::applied);
  REQUIRE(divestiture.workItems.size() == 2U);
  auto const regularConfirmation = std::ranges::find_if(
      divestiture.workItems,
      [](umbra::detail::AttributeOwnershipAcquisitionWorkItem const& work) {
        return !work.candidateIsIfAvailable;
      });
  auto const ifAvailableConfirmation = std::ranges::find_if(
      divestiture.workItems,
      [](umbra::detail::AttributeOwnershipAcquisitionWorkItem const& work) {
        return work.candidateIsIfAvailable;
      });
  REQUIRE(regularConfirmation != divestiture.workItems.end());
  REQUIRE(ifAvailableConfirmation != divestiture.workItems.end());
  REQUIRE(regularConfirmation->requestId == regularRequestId);
  REQUIRE(ifAvailableConfirmation->requestId == ifAvailable.requestId);
  REQUIRE(source.beginRequestDivestitureConfirmation(
      L"exercise", owner.membership->id, requester.membership->id,
      registered.objectInstanceHandle, regularRequestId, false,
      efficiencyOnly));
  REQUIRE_FALSE(source.beginRequestDivestitureConfirmation(
      L"exercise", owner.membership->id, requester.membership->id,
      registered.objectInstanceHandle, regularRequestId, false, efficiencyOnly));

  std::wstring const saveLabel = L"ownership-negotiated-mixed-asymmetric-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", requester.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", requester.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.attributeValuesPresent);
  REQUIRE(savedObject.attributeValues.size() == 2U);
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionRequests.size() == 1U);
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionIfAvailableRequests.size() == 1U);
  REQUIRE(savedObject.pendingNegotiatedAttributeOwnershipDivestitures.size() == 2U);
  auto const savedRegular = std::ranges::find_if(
      savedObject.pendingNegotiatedAttributeOwnershipDivestitures,
      [&](umbra::detail::FederationStateImagePendingNegotiatedAttributeOwnershipDivestiture const& saved) {
        return saved.attributeHandle == *efficiency;
      });
  auto const savedIfAvailable = std::ranges::find_if(
      savedObject.pendingNegotiatedAttributeOwnershipDivestitures,
      [&](umbra::detail::FederationStateImagePendingNegotiatedAttributeOwnershipDivestiture const& saved) {
        return saved.attributeHandle == *cheerfulness;
      });
  REQUIRE(savedRegular != savedObject.pendingNegotiatedAttributeOwnershipDivestitures.end());
  REQUIRE(savedIfAvailable != savedObject.pendingNegotiatedAttributeOwnershipDivestitures.end());
  REQUIRE(savedRegular->confirmationQueued);
  REQUIRE(savedRegular->confirmationDelivered);
  REQUIRE(savedIfAvailable->acquiringFederateIsIfAvailable);
  REQUIRE(savedIfAvailable->confirmationQueued);
  REQUIRE_FALSE(savedIfAvailable->confirmationDelivered);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedOwner = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto restartedRequester = restarted.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedRequester.membership);
  REQUIRE(restartedOwner.membership->id == owner.membership->id);
  REQUIRE(restartedRequester.membership->id == requester.membership->id);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedRequester.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto firstRestore = restarted.federateRestoreComplete(
      L"exercise", restartedOwner.membership->id);
  REQUIRE(firstRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(firstRestore.ownershipAcquisitionWorkItems.empty());
  auto restored = restarted.federateRestoreComplete(
      L"exercise", restartedRequester.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.ownershipAcquisitionWorkItems.size() == 1U);
  auto const restoredIfAvailableConfirmation = std::ranges::find_if(
      restored.ownershipAcquisitionWorkItems,
      [](umbra::detail::AttributeOwnershipAcquisitionWorkItem const& work) {
        return work.candidateIsIfAvailable;
      });
  REQUIRE(restoredIfAvailableConfirmation != restored.ownershipAcquisitionWorkItems.end());
  REQUIRE(restoredIfAvailableConfirmation->kind ==
      umbra::detail::AttributeOwnershipAcquisitionWorkKind::request_divestiture_confirmation);
  REQUIRE(restoredIfAvailableConfirmation->requestId == ifAvailable.requestId);
  REQUIRE(restoredIfAvailableConfirmation->attributeHandles == cheerfulnessOnly);
  REQUIRE_FALSE(restarted.beginRequestDivestitureConfirmation(
      L"exercise", restartedOwner.membership->id,
      restartedRequester.membership->id, registered.objectInstanceHandle,
      regularRequestId, false, efficiencyOnly));
  REQUIRE(restarted.beginRequestDivestitureConfirmation(
      L"exercise", restartedOwner.membership->id,
      restartedRequester.membership->id, registered.objectInstanceHandle,
      ifAvailable.requestId, true, cheerfulnessOnly));
  REQUIRE_FALSE(restarted.beginRequestDivestitureConfirmation(
      L"exercise", restartedOwner.membership->id,
      restartedRequester.membership->id, registered.objectInstanceHandle,
      ifAvailable.requestId, true, cheerfulnessOnly));
  REQUIRE(restarted.attributeOwnedByFederate(
      L"exercise", restartedOwner.membership->id,
      registered.objectInstanceHandle, *efficiency).ownedByRequestingFederate);
  REQUIRE(restarted.attributeOwnedByFederate(
      L"exercise", restartedOwner.membership->id,
      registered.objectInstanceHandle, *cheerfulness).ownedByRequestingFederate);

  auto confirmed = restarted.planConfirmDivestiture(
      L"exercise", restartedOwner.membership->id,
      registered.objectInstanceHandle, mixedAttributes, divestitureTag);
  REQUIRE(confirmed.status == umbra::detail::ConfirmDivestitureStatus::applied);
  REQUIRE(confirmed.notifications.size() == 1U);
  auto const& notification = confirmed.notifications.front();
  REQUIRE(notification.attributeHandles == mixedAttributes);
  auto notificationDelivery = restarted.beginConfirmDivestitureNotification(
      L"exercise", restartedRequester.membership->id,
      registered.objectInstanceHandle, notification.notificationId,
      notification.attributeHandles);
  REQUIRE(notificationDelivery.has_value());
  REQUIRE(notificationDelivery->securedAttributeHandles == mixedAttributes);
  REQUIRE(restarted.attributeOwnedByFederate(
      L"exercise", restartedRequester.membership->id,
      registered.objectInstanceHandle, *efficiency).ownedByRequestingFederate);
  REQUIRE(restarted.attributeOwnedByFederate(
      L"exercise", restartedRequester.membership->id,
      registered.objectInstanceHandle, *cheerfulness).ownedByRequestingFederate);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image preserves mixed delivered negotiated confirmations in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][ownership-ledger-state][attribute-ownership-acquisition]"
    "[attribute-ownership-acquisition-if-available][negotiated-attribute-ownership-divestiture]"
    "[process-restart-mixed-confirmation-delivered][process-restart-negotiated-mixed-confirmation-delivered]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto requester = source.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(owner.membership);
  REQUIRE(requester.membership);

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  auto const cheerfulness = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Cheerfulness");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  REQUIRE(cheerfulness.has_value());
  std::set<std::uint64_t> const mixedAttributes{*efficiency, *cheerfulness};
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  std::set<std::uint64_t> const cheerfulnessOnly{*cheerfulness};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *server, mixedAttributes, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", requester.membership->id, *server, mixedAttributes, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", requester.membership->id,
      registered.objectInstanceHandle));
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", requester.membership->id, *server, mixedAttributes, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  std::string const efficiencyValue{"\x79\x7a", 2U};
  std::string const cheerfulnessValue{"\x7d\x7e", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{
      {*efficiency,
       rti1516_2025::VariableLengthData(efficiencyValue.data(), efficiencyValue.size())},
      {*cheerfulness,
       rti1516_2025::VariableLengthData(cheerfulnessValue.data(), cheerfulnessValue.size())},
  };
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise", owner.membership->id, registered.objectInstanceHandle,
      *server, {"HLAreliable"}, &values) == FederationRegistryStatus::applied);

  std::vector<unsigned char> const regularTag{
      'r', 'e', 'g', '-', 'm', 'i', 'x', '-', 'd', 'e', 'l', 'i', 'v', 'e', 'r', 'e', 'd'};
  auto regular = source.planAttributeOwnershipAcquisition(
      L"exercise", requester.membership->id, registered.objectInstanceHandle,
      efficiencyOnly, regularTag);
  REQUIRE(regular.status ==
      umbra::detail::AttributeOwnershipAcquisitionStatus::applied);
  REQUIRE(regular.workItems.size() == 1U);
  auto const regularRequestId = regular.workItems.front().requestId;

  std::vector<unsigned char> const ifAvailableTag{
      'w', 't', 'a', '-', 'm', 'i', 'x', '-', 'd', 'e', 'l', 'i', 'v', 'e', 'r', 'e', 'd'};
  auto ifAvailable = source.planAttributeOwnershipAcquisitionIfAvailable(
      L"exercise", requester.membership->id, registered.objectInstanceHandle,
      cheerfulnessOnly, ifAvailableTag);
  REQUIRE(ifAvailable.status ==
      umbra::detail::AttributeOwnershipAcquisitionIfAvailableStatus::applied);
  REQUIRE(ifAvailable.requestId != 0U);

  std::vector<unsigned char> const divestitureTag{
      'd', 'i', 'v', '-', 'm', 'i', 'x', '-', 'd', 'e', 'l', 'i', 'v', 'e', 'r', 'e', 'd'};
  auto divestiture = source.planNegotiatedAttributeOwnershipDivestiture(
      L"exercise", owner.membership->id, registered.objectInstanceHandle,
      mixedAttributes, divestitureTag);
  REQUIRE(divestiture.status ==
      umbra::detail::NegotiatedAttributeOwnershipDivestitureStatus::applied);
  REQUIRE(divestiture.workItems.size() == 2U);
  auto const regularConfirmation = std::ranges::find_if(
      divestiture.workItems,
      [](umbra::detail::AttributeOwnershipAcquisitionWorkItem const& work) {
        return !work.candidateIsIfAvailable;
      });
  auto const ifAvailableConfirmation = std::ranges::find_if(
      divestiture.workItems,
      [](umbra::detail::AttributeOwnershipAcquisitionWorkItem const& work) {
        return work.candidateIsIfAvailable;
      });
  REQUIRE(regularConfirmation != divestiture.workItems.end());
  REQUIRE(ifAvailableConfirmation != divestiture.workItems.end());
  REQUIRE(regularConfirmation->requestId == regularRequestId);
  REQUIRE(ifAvailableConfirmation->requestId == ifAvailable.requestId);
  REQUIRE(regularConfirmation->attributeHandles == efficiencyOnly);
  REQUIRE(ifAvailableConfirmation->attributeHandles == cheerfulnessOnly);
  REQUIRE(source.beginRequestDivestitureConfirmation(
      L"exercise", owner.membership->id, requester.membership->id,
      registered.objectInstanceHandle, regularRequestId, false,
      efficiencyOnly));
  REQUIRE(source.beginRequestDivestitureConfirmation(
      L"exercise", owner.membership->id, requester.membership->id,
      registered.objectInstanceHandle, ifAvailable.requestId, true,
      cheerfulnessOnly));
  REQUIRE_FALSE(source.beginRequestDivestitureConfirmation(
      L"exercise", owner.membership->id, requester.membership->id,
      registered.objectInstanceHandle, regularRequestId, false, efficiencyOnly));
  REQUIRE_FALSE(source.beginRequestDivestitureConfirmation(
      L"exercise", owner.membership->id, requester.membership->id,
      registered.objectInstanceHandle, ifAvailable.requestId, true, cheerfulnessOnly));

  std::wstring const saveLabel = L"ownership-negotiated-mixed-delivered-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", requester.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", requester.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.attributeValuesPresent);
  REQUIRE(savedObject.attributeValues.size() == 2U);
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionRequests.size() == 1U);
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionIfAvailableRequests.size() == 1U);
  REQUIRE(savedObject.pendingNegotiatedAttributeOwnershipDivestitures.size() == 2U);
  auto const savedRegular = std::ranges::find_if(
      savedObject.pendingNegotiatedAttributeOwnershipDivestitures,
      [&](umbra::detail::FederationStateImagePendingNegotiatedAttributeOwnershipDivestiture const& saved) {
        return saved.attributeHandle == *efficiency;
      });
  auto const savedIfAvailable = std::ranges::find_if(
      savedObject.pendingNegotiatedAttributeOwnershipDivestitures,
      [&](umbra::detail::FederationStateImagePendingNegotiatedAttributeOwnershipDivestiture const& saved) {
        return saved.attributeHandle == *cheerfulness;
      });
  REQUIRE(savedRegular != savedObject.pendingNegotiatedAttributeOwnershipDivestitures.end());
  REQUIRE(savedIfAvailable != savedObject.pendingNegotiatedAttributeOwnershipDivestitures.end());
  REQUIRE(savedRegular->confirmationQueued);
  REQUIRE(savedRegular->confirmationDelivered);
  REQUIRE(savedIfAvailable->acquiringFederateIsIfAvailable);
  REQUIRE(savedIfAvailable->confirmationQueued);
  REQUIRE(savedIfAvailable->confirmationDelivered);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedOwner = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto restartedRequester = restarted.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedRequester.membership);
  REQUIRE(restartedOwner.membership->id == owner.membership->id);
  REQUIRE(restartedRequester.membership->id == requester.membership->id);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedRequester.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto firstRestore = restarted.federateRestoreComplete(
      L"exercise", restartedOwner.membership->id);
  REQUIRE(firstRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(firstRestore.ownershipAcquisitionWorkItems.empty());
  auto restored = restarted.federateRestoreComplete(
      L"exercise", restartedRequester.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.ownershipAcquisitionWorkItems.empty());
  REQUIRE_FALSE(restarted.beginRequestDivestitureConfirmation(
      L"exercise", restartedOwner.membership->id,
      restartedRequester.membership->id, registered.objectInstanceHandle,
      regularRequestId, false, efficiencyOnly));
  REQUIRE_FALSE(restarted.beginRequestDivestitureConfirmation(
      L"exercise", restartedOwner.membership->id,
      restartedRequester.membership->id, registered.objectInstanceHandle,
      ifAvailable.requestId, true, cheerfulnessOnly));
  REQUIRE(restarted.attributeOwnedByFederate(
      L"exercise", restartedOwner.membership->id,
      registered.objectInstanceHandle, *efficiency).ownedByRequestingFederate);
  REQUIRE(restarted.attributeOwnedByFederate(
      L"exercise", restartedOwner.membership->id,
      registered.objectInstanceHandle, *cheerfulness).ownedByRequestingFederate);

  auto confirmed = restarted.planConfirmDivestiture(
      L"exercise", restartedOwner.membership->id,
      registered.objectInstanceHandle, mixedAttributes, divestitureTag);
  REQUIRE(confirmed.status == umbra::detail::ConfirmDivestitureStatus::applied);
  REQUIRE(confirmed.notifications.size() == 1U);
  auto const& notification = confirmed.notifications.front();
  REQUIRE(notification.attributeHandles == mixedAttributes);
  auto notificationDelivery = restarted.beginConfirmDivestitureNotification(
      L"exercise", restartedRequester.membership->id,
      registered.objectInstanceHandle, notification.notificationId,
      notification.attributeHandles);
  REQUIRE(notificationDelivery.has_value());
  REQUIRE(notificationDelivery->securedAttributeHandles == mixedAttributes);
  REQUIRE(restarted.attributeOwnedByFederate(
      L"exercise", restartedRequester.membership->id,
      registered.objectInstanceHandle, *efficiency).ownedByRequestingFederate);
  REQUIRE(restarted.attributeOwnedByFederate(
      L"exercise", restartedRequester.membership->id,
      registered.objectInstanceHandle, *cheerfulness).ownedByRequestingFederate);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image preserves delivered negotiated owner confirmation in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-negotiated-confirmation-delivered][ownership-ledger-state][attribute-ownership-acquisition]"
    "[negotiated-attribute-ownership-divestiture]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto requester = source.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(owner.status == FederationRegistryStatus::applied);
  REQUIRE(requester.status == FederationRegistryStatus::applied);
  REQUIRE(owner.membership);
  REQUIRE(requester.membership);

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", requester.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(discoveries.front().receivingFederateId == requester.membership->id);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise",
      requester.membership->id,
      registered.objectInstanceHandle)
      .has_value());
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", requester.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  std::string const valueBytes{"\x79\x7a", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *efficiency,
      rti1516_2025::VariableLengthData(valueBytes.data(), valueBytes.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise",
      owner.membership->id,
      registered.objectInstanceHandle,
      *server,
      {"HLAreliable"},
      &values) == FederationRegistryStatus::applied);

  std::vector<unsigned char> const acquisitionTag{
      'r', 'e', 'g', '-', 'd', 'e', 'l', 'i', 'v', 'e', 'r', 'e', 'd'};
  auto acquisition = source.planAttributeOwnershipAcquisition(
      L"exercise",
      requester.membership->id,
      registered.objectInstanceHandle,
      efficiencyOnly,
      acquisitionTag);
  REQUIRE(acquisition.status ==
      umbra::detail::AttributeOwnershipAcquisitionStatus::applied);
  REQUIRE(acquisition.workItems.size() == 1U);
  REQUIRE(acquisition.workItems.front().kind ==
      umbra::detail::AttributeOwnershipAcquisitionWorkKind::request_release);
  auto const acquisitionRequestId = acquisition.workItems.front().requestId;

  std::vector<unsigned char> const divestitureTag{
      'd', 'i', 'v', '-', 'd', 'e', 'l', 'i', 'v', 'e', 'r', 'e', 'd'};
  auto divestiture = source.planNegotiatedAttributeOwnershipDivestiture(
      L"exercise",
      owner.membership->id,
      registered.objectInstanceHandle,
      efficiencyOnly,
      divestitureTag);
  REQUIRE(divestiture.status ==
      umbra::detail::NegotiatedAttributeOwnershipDivestitureStatus::applied);
  REQUIRE(divestiture.workItems.size() == 1U);
  auto const& sourceConfirmation = divestiture.workItems.front();
  REQUIRE(sourceConfirmation.requestId == acquisitionRequestId);
  REQUIRE_FALSE(sourceConfirmation.candidateIsIfAvailable);
  auto deliveredBeforeSave = source.beginRequestDivestitureConfirmation(
      L"exercise",
      owner.membership->id,
      requester.membership->id,
      registered.objectInstanceHandle,
      sourceConfirmation.requestId,
      false,
      sourceConfirmation.attributeHandles);
  REQUIRE(deliveredBeforeSave.has_value());
  REQUIRE(deliveredBeforeSave->releasedAttributeHandles == efficiencyOnly);
  REQUIRE_FALSE(source.beginRequestDivestitureConfirmation(
      L"exercise",
      owner.membership->id,
      requester.membership->id,
      registered.objectInstanceHandle,
      sourceConfirmation.requestId,
      false,
      sourceConfirmation.attributeHandles));

  std::wstring const saveLabel = L"ownership-negotiated-delivered-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", requester.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", requester.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.attributeValuesPresent);
  REQUIRE(savedObject.attributeValues.size() == 1U);
  REQUIRE(savedObject.attributeValues.front().value == valueBytes);
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionRequests.size() == 1U);
  REQUIRE(savedObject.pendingNegotiatedAttributeOwnershipDivestitures.size() == 1U);
  auto const& savedRequest = savedObject.pendingAttributeOwnershipAcquisitionRequests.front();
  auto const& savedDivestiture =
      savedObject.pendingNegotiatedAttributeOwnershipDivestitures.front();
  REQUIRE(savedRequest.requestId == acquisitionRequestId);
  REQUIRE(savedRequest.releaseCallbacksQueuedByOwningFederate.size() == 1U);
  REQUIRE(savedDivestiture.acquisitionRequestId == acquisitionRequestId);
  REQUIRE(savedDivestiture.confirmationQueued);
  REQUIRE(savedDivestiture.confirmationDelivered);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedOwner = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto restartedRequester = restarted.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedRequester.membership);
  REQUIRE(restartedOwner.membership->id == owner.membership->id);
  REQUIRE(restartedRequester.membership->id == requester.membership->id);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedRequester.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto firstRestore = restarted.federateRestoreComplete(
      L"exercise", restartedOwner.membership->id);
  REQUIRE(firstRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(firstRestore.ownershipAcquisitionWorkItems.empty());
  auto restored = restarted.federateRestoreComplete(
      L"exercise", restartedRequester.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.ownershipAcquisitionWorkItems.empty());
  REQUIRE_FALSE(restarted.beginRequestDivestitureConfirmation(
      L"exercise",
      restartedOwner.membership->id,
      restartedRequester.membership->id,
      registered.objectInstanceHandle,
      acquisitionRequestId,
      false,
      efficiencyOnly));

  auto restoredOwnerState = restarted.attributeOwnedByFederate(
      L"exercise",
      restartedOwner.membership->id,
      registered.objectInstanceHandle,
      *efficiency);
  REQUIRE(restoredOwnerState.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE(restoredOwnerState.ownedByRequestingFederate);
  auto confirmed = restarted.planConfirmDivestiture(
      L"exercise",
      restartedOwner.membership->id,
      registered.objectInstanceHandle,
      efficiencyOnly,
      divestitureTag);
  REQUIRE(confirmed.status == umbra::detail::ConfirmDivestitureStatus::applied);
  REQUIRE(confirmed.notifications.size() == 1U);
  auto const& notification = confirmed.notifications.front();
  auto notificationDelivery = restarted.beginConfirmDivestitureNotification(
      L"exercise",
      restartedRequester.membership->id,
      registered.objectInstanceHandle,
      notification.notificationId,
      notification.attributeHandles);
  REQUIRE(notificationDelivery.has_value());
  REQUIRE(notificationDelivery->securedAttributeHandles == efficiencyOnly);
  auto requesterOwnership = restarted.attributeOwnedByFederate(
      L"exercise",
      restartedRequester.membership->id,
      registered.objectInstanceHandle,
      *efficiency);
  REQUIRE(requesterOwnership.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE(requesterOwnership.ownedByRequestingFederate);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image preserves delivered negotiated If Available confirmation in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-negotiated-if-available-confirmation-delivered]"
    "[ownership-ledger-state][attribute-ownership-acquisition]"
    "[attribute-ownership-acquisition-if-available][negotiated-attribute-ownership-divestiture]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto requester = source.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(owner.status == FederationRegistryStatus::applied);
  REQUIRE(requester.status == FederationRegistryStatus::applied);
  REQUIRE(owner.membership);
  REQUIRE(requester.membership);

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", requester.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(discoveries.front().receivingFederateId == requester.membership->id);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise",
      requester.membership->id,
      registered.objectInstanceHandle)
      .has_value());
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", requester.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  std::string const valueBytes{"\x7b\x7c", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *efficiency,
      rti1516_2025::VariableLengthData(valueBytes.data(), valueBytes.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise",
      owner.membership->id,
      registered.objectInstanceHandle,
      *server,
      {"HLAreliable"},
      &values) == FederationRegistryStatus::applied);

  std::vector<unsigned char> const acquisitionTag{
      'w', 't', 'a', '-', 'd', 'e', 'l', 'i', 'v', 'e', 'r', 'e', 'd'};
  auto acquisition = source.planAttributeOwnershipAcquisitionIfAvailable(
      L"exercise",
      requester.membership->id,
      registered.objectInstanceHandle,
      efficiencyOnly,
      acquisitionTag);
  REQUIRE(acquisition.status ==
      umbra::detail::AttributeOwnershipAcquisitionIfAvailableStatus::applied);
  REQUIRE(acquisition.requestId != 0U);

  std::vector<unsigned char> const divestitureTag{
      'd', 'i', 'v', '-', 'w', 't', 'a', '-', 'd', 'e', 'l', 'i', 'v', 'e', 'r', 'e', 'd'};
  auto divestiture = source.planNegotiatedAttributeOwnershipDivestiture(
      L"exercise",
      owner.membership->id,
      registered.objectInstanceHandle,
      efficiencyOnly,
      divestitureTag);
  REQUIRE(divestiture.status ==
      umbra::detail::NegotiatedAttributeOwnershipDivestitureStatus::applied);
  REQUIRE(divestiture.workItems.size() == 1U);
  auto const& sourceConfirmation = divestiture.workItems.front();
  REQUIRE(sourceConfirmation.requestId == acquisition.requestId);
  REQUIRE(sourceConfirmation.candidateIsIfAvailable);
  auto deliveredBeforeSave = source.beginRequestDivestitureConfirmation(
      L"exercise",
      owner.membership->id,
      requester.membership->id,
      registered.objectInstanceHandle,
      sourceConfirmation.requestId,
      true,
      sourceConfirmation.attributeHandles);
  REQUIRE(deliveredBeforeSave.has_value());
  REQUIRE(deliveredBeforeSave->releasedAttributeHandles == efficiencyOnly);
  REQUIRE_FALSE(source.beginRequestDivestitureConfirmation(
      L"exercise",
      owner.membership->id,
      requester.membership->id,
      registered.objectInstanceHandle,
      sourceConfirmation.requestId,
      true,
      sourceConfirmation.attributeHandles));

  std::wstring const saveLabel = L"ownership-negotiated-if-available-delivered-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", requester.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", requester.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.attributeValuesPresent);
  REQUIRE(savedObject.attributeValues.size() == 1U);
  REQUIRE(savedObject.attributeValues.front().value == valueBytes);
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionRequests.empty());
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionIfAvailableRequests.size() == 1U);
  auto const& savedRequest =
      savedObject.pendingAttributeOwnershipAcquisitionIfAvailableRequests.front();
  REQUIRE(savedRequest.requestId == acquisition.requestId);
  REQUIRE(savedRequest.requestingFederateId == requester.membership->id);
  REQUIRE(savedRequest.desiredAttributeHandles ==
      std::vector<std::uint64_t>{*efficiency});
  REQUIRE(savedObject.pendingNegotiatedAttributeOwnershipDivestitures.size() == 1U);
  auto const& savedDivestiture =
      savedObject.pendingNegotiatedAttributeOwnershipDivestitures.front();
  REQUIRE(savedDivestiture.acquiringFederateIsIfAvailable);
  REQUIRE(savedDivestiture.confirmationQueued);
  REQUIRE(savedDivestiture.confirmationDelivered);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedOwner = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto restartedRequester = restarted.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedRequester.membership);
  REQUIRE(restartedOwner.membership->id == owner.membership->id);
  REQUIRE(restartedRequester.membership->id == requester.membership->id);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedRequester.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto firstRestore = restarted.federateRestoreComplete(
      L"exercise", restartedOwner.membership->id);
  REQUIRE(firstRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(firstRestore.ownershipAcquisitionWorkItems.empty());
  auto restored = restarted.federateRestoreComplete(
      L"exercise", restartedRequester.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.ownershipAcquisitionWorkItems.empty());
  REQUIRE_FALSE(restarted.beginRequestDivestitureConfirmation(
      L"exercise",
      restartedOwner.membership->id,
      restartedRequester.membership->id,
      registered.objectInstanceHandle,
      acquisition.requestId,
      true,
      efficiencyOnly));

  auto restoredOwnerState = restarted.attributeOwnedByFederate(
      L"exercise",
      restartedOwner.membership->id,
      registered.objectInstanceHandle,
      *efficiency);
  REQUIRE(restoredOwnerState.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE(restoredOwnerState.ownedByRequestingFederate);
  auto confirmed = restarted.planConfirmDivestiture(
      L"exercise",
      restartedOwner.membership->id,
      registered.objectInstanceHandle,
      efficiencyOnly,
      divestitureTag);
  REQUIRE(confirmed.status == umbra::detail::ConfirmDivestitureStatus::applied);
  REQUIRE(confirmed.notifications.size() == 1U);
  auto const& notification = confirmed.notifications.front();
  auto notificationDelivery = restarted.beginConfirmDivestitureNotification(
      L"exercise",
      restartedRequester.membership->id,
      registered.objectInstanceHandle,
      notification.notificationId,
      notification.attributeHandles);
  REQUIRE(notificationDelivery.has_value());
  REQUIRE(notificationDelivery->securedAttributeHandles == efficiencyOnly);
  auto requesterOwnership = restarted.attributeOwnedByFederate(
      L"exercise",
      restartedRequester.membership->id,
      registered.objectInstanceHandle,
      *efficiency);
  REQUIRE(requesterOwnership.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE(requesterOwnership.ownedByRequestingFederate);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Federation restore rejects malformed mixed negotiated confirmation images",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore][failure]"
    "[process-restart][process-restart-malformed-mixed-confirmation][ownership-ledger-state]"
    "[attribute-ownership-acquisition][attribute-ownership-acquisition-if-available]"
    "[negotiated-attribute-ownership-divestiture]") {
  enum class Mutation {
    missingIfAvailableCandidate,
    mismatchedIfAvailableCandidate,
    staleConfirmationFlags,
  };
  class MutatingStore final : public umbra::detail::FederationSaveCommitStore {
   public:
    MutatingStore(
        umbra::detail::FederationSaveCommitDescriptor descriptor,
        Mutation mutation)
        : committed(std::move(descriptor)), mutation(mutation) {}

    void commit(umbra::detail::FederationSaveCommitDescriptor const&) override {}

    [[nodiscard]] std::optional<umbra::detail::FederationSaveCommitDescriptor> load(
        std::wstring const& federationName,
        std::wstring const& label) const override {
      if (committed.federationName != federationName || committed.label != label) {
        return std::nullopt;
      }
      auto result = committed;
      auto image = umbra::detail::FederationStateImageCodec::decode(
          result.stateImage);
      REQUIRE(image.objects.size() == 1U);
      auto& object = image.objects.front();
      REQUIRE(object.pendingAttributeOwnershipAcquisitionRequests.size() == 1U);
      REQUIRE(object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.size() == 1U);
      REQUIRE(object.pendingNegotiatedAttributeOwnershipDivestitures.size() == 2U);
      switch (mutation) {
        case Mutation::missingIfAvailableCandidate:
          object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.clear();
          REQUIRE(object.pendingOperationCount > 0U);
          --object.pendingOperationCount;
          break;
        case Mutation::mismatchedIfAvailableCandidate:
          for (auto& divestiture :
               object.pendingNegotiatedAttributeOwnershipDivestitures) {
            if (divestiture.acquiringFederateIsIfAvailable) {
              ++divestiture.acquisitionRequestId;
            }
          }
          break;
        case Mutation::staleConfirmationFlags:
          for (auto& divestiture :
               object.pendingNegotiatedAttributeOwnershipDivestitures) {
            if (!divestiture.acquiringFederateIsIfAvailable) {
              divestiture.confirmationQueued = false;
              divestiture.confirmationDelivered = true;
            }
          }
          break;
      }
      result.stateImage = umbra::detail::FederationStateImageCodec::encode(image);
      return result;
    }

   private:
    umbra::detail::FederationSaveCommitDescriptor committed;
    Mutation mutation;
  };

  auto sourceStore = std::make_shared<umbra::detail::MemoryFederationSaveCommitStore>();
  EmbeddedFederationRegistry source({}, sourceStore);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto requester = source.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(owner.membership);
  REQUIRE(requester.membership);

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  auto const cheerfulness = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Cheerfulness");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  REQUIRE(cheerfulness.has_value());
  std::set<std::uint64_t> const mixedAttributes{*efficiency, *cheerfulness};
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  std::set<std::uint64_t> const cheerfulnessOnly{*cheerfulness};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *server, mixedAttributes, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", requester.membership->id, *server, mixedAttributes, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", requester.membership->id,
      registered.objectInstanceHandle));
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", requester.membership->id, *server, mixedAttributes, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  std::string const efficiencyValue{"\x91\x92", 2U};
  std::string const cheerfulnessValue{"\x93\x94", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{
      {*efficiency,
       rti1516_2025::VariableLengthData(efficiencyValue.data(), efficiencyValue.size())},
      {*cheerfulness,
       rti1516_2025::VariableLengthData(cheerfulnessValue.data(), cheerfulnessValue.size())},
  };
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise", owner.membership->id, registered.objectInstanceHandle,
      *server, {"HLAreliable"}, &values) == FederationRegistryStatus::applied);
  auto regular = source.planAttributeOwnershipAcquisition(
      L"exercise", requester.membership->id, registered.objectInstanceHandle,
      efficiencyOnly, {'r', 'e', 'g', '-', 'm', 'a', 'l', 'f', 'o', 'r', 'm'});
  REQUIRE(regular.status ==
      umbra::detail::AttributeOwnershipAcquisitionStatus::applied);
  REQUIRE(regular.workItems.size() == 1U);
  auto ifAvailable = source.planAttributeOwnershipAcquisitionIfAvailable(
      L"exercise", requester.membership->id, registered.objectInstanceHandle,
      cheerfulnessOnly, {'w', 't', 'a', '-', 'm', 'a', 'l', 'f', 'o', 'r', 'm'});
  REQUIRE(ifAvailable.status ==
      umbra::detail::AttributeOwnershipAcquisitionIfAvailableStatus::applied);
  REQUIRE(ifAvailable.requestId != 0U);
  auto divestiture = source.planNegotiatedAttributeOwnershipDivestiture(
      L"exercise", owner.membership->id, registered.objectInstanceHandle,
      mixedAttributes, {'d', 'i', 'v', '-', 'm', 'a', 'l', 'f', 'o', 'r', 'm'});
  REQUIRE(divestiture.status ==
      umbra::detail::NegotiatedAttributeOwnershipDivestitureStatus::applied);
  REQUIRE(divestiture.workItems.size() == 2U);

  std::wstring const saveLabel = L"ownership-negotiated-malformed-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", requester.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", requester.membership->id).saveCompletedSuccessfully);
  auto commits = sourceStore->snapshotCommits();
  REQUIRE(commits.size() == 1U);
  auto const baseCommit = commits.front();

  for (auto const mutation : {
           Mutation::missingIfAvailableCandidate,
           Mutation::mismatchedIfAvailableCandidate,
           Mutation::staleConfirmationFlags}) {
    auto store = std::make_shared<MutatingStore>(baseCommit, mutation);
    EmbeddedFederationRegistry restarted({}, store);
    REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
        FederationRegistryStatus::applied);
    auto restartedOwner = restarted.join(
        L"exercise", L"publisher", L"owner", noOpCallbackRoute());
    auto restartedRequester = restarted.join(
        L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
    REQUIRE(restartedOwner.membership);
    REQUIRE(restartedRequester.membership);
    auto restore = restarted.requestFederationRestore(
        L"exercise", restartedRequester.membership->id, saveLabel);
    REQUIRE(restore.status ==
        umbra::detail::FederationRestoreControlStatus::snapshot_not_found);
    REQUIRE(restore.notifications.size() == 1U);
    REQUIRE(restore.notifications.front().kind ==
        umbra::detail::FederationRestoreNotificationKind::request_failed);
    REQUIRE(restore.notifications.front().receivingFederateId ==
        restartedRequester.membership->id);
  }
}

TEST_CASE(
    "Filesystem state image restores a pending Divestiture If Wanted notification in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-divestiture-if-wanted][ownership-ledger-state]"
    "[attribute-ownership-divestiture-if-wanted][attribute-ownership-acquisition]"
    "[attribute-ownership-acquisition-if-available]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto requester = source.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(owner.status == FederationRegistryStatus::applied);
  REQUIRE(requester.status == FederationRegistryStatus::applied);
  REQUIRE(owner.membership);
  REQUIRE(requester.membership);

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", requester.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(discoveries.front().receivingFederateId == requester.membership->id);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", requester.membership->id,
      registered.objectInstanceHandle)
      .has_value());
  // The requester must publish after discovery to remain an eligible acquirer
  // when the owner evaluates Divestiture If Wanted.
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", requester.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  std::string const valueBytes{"\x7d\x7e", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *efficiency,
      rti1516_2025::VariableLengthData(valueBytes.data(), valueBytes.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise", owner.membership->id, registered.objectInstanceHandle,
      *server, {"HLAreliable"}, &values) == FederationRegistryStatus::applied);

  std::vector<unsigned char> const acquisitionTag{
      'r', 'e', 'g', '-', 'd', 'i', 'v', '-', 'w', 'a', 'n', 't'};
  auto acquisition = source.planAttributeOwnershipAcquisition(
      L"exercise", requester.membership->id,
      registered.objectInstanceHandle, efficiencyOnly, acquisitionTag);
  REQUIRE(acquisition.status ==
      umbra::detail::AttributeOwnershipAcquisitionStatus::applied);
  REQUIRE(acquisition.workItems.size() == 1U);
  REQUIRE(acquisition.workItems.front().kind ==
      umbra::detail::AttributeOwnershipAcquisitionWorkKind::request_release);

  std::vector<unsigned char> const divestitureTag{
      'd', 'i', 'v', '-', 'i', 'f', '-', 'w', 'a', 'n', 't'};
  auto divestiture = source.planAttributeOwnershipDivestitureIfWanted(
      L"exercise", owner.membership->id, registered.objectInstanceHandle,
      efficiencyOnly, divestitureTag);
  REQUIRE(divestiture.status ==
      umbra::detail::AttributeOwnershipDivestitureIfWantedStatus::applied);
  REQUIRE(divestiture.divestedAttributeHandles == efficiencyOnly);
  REQUIRE(divestiture.notifications.size() == 1U);
  auto const& sourceNotification = divestiture.notifications.front();
  REQUIRE(sourceNotification.notificationId != 0U);
  REQUIRE(sourceNotification.receivingFederateId == requester.membership->id);
  REQUIRE(sourceNotification.objectInstanceHandle ==
      registered.objectInstanceHandle);
  REQUIRE(sourceNotification.attributeHandles == efficiencyOnly);
  REQUIRE(sourceNotification.userSuppliedTag == divestitureTag);
  // The notification is intentionally left pending for the save/restart
  // boundary; its callback begins only after restore.
  auto sourceOwnership = source.attributeOwnedByFederate(
      L"exercise", requester.membership->id,
      registered.objectInstanceHandle, *efficiency);
  REQUIRE(sourceOwnership.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE(sourceOwnership.ownedByRequestingFederate);
  auto sourceOwnerState = source.attributeOwnedByFederate(
      L"exercise", owner.membership->id,
      registered.objectInstanceHandle, *efficiency);
  REQUIRE(sourceOwnerState.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE_FALSE(sourceOwnerState.ownedByRequestingFederate);

  std::wstring const saveLabel = L"ownership-divestiture-if-wanted-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", requester.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", requester.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.handle == registered.objectInstanceHandle);
  REQUIRE(savedObject.attributeValuesPresent);
  REQUIRE(savedObject.attributeValues.size() == 1U);
  REQUIRE(savedObject.attributeValues.front().attributeHandle == *efficiency);
  REQUIRE(savedObject.attributeValues.front().value == valueBytes);
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionRequests.empty());
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionIfAvailableRequests.empty());
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionCancellations.empty());
  REQUIRE(savedObject.pendingAttributeOwnershipDivestitureIfWantedNotifications.size() == 1U);
  auto const& savedNotification =
      savedObject.pendingAttributeOwnershipDivestitureIfWantedNotifications.front();
  REQUIRE(savedNotification.notificationId == sourceNotification.notificationId);
  REQUIRE(savedNotification.receivingFederateId == requester.membership->id);
  REQUIRE(savedNotification.attributeHandles ==
      std::vector<std::uint64_t>{*efficiency});
  REQUIRE(savedObject.pendingOperationCount == 1U);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedOwner = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto restartedRequester = restarted.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedRequester.membership);
  REQUIRE(restartedOwner.membership->id == owner.membership->id);
  REQUIRE(restartedRequester.membership->id == requester.membership->id);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedRequester.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto firstRestore = restarted.federateRestoreComplete(
      L"exercise", restartedOwner.membership->id);
  REQUIRE(firstRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(firstRestore.ownershipAcquisitionWorkItems.empty());
  auto restored = restarted.federateRestoreComplete(
      L"exercise", restartedRequester.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.ownershipAcquisitionWorkItems.empty());

  auto restoredOwnerState = restarted.attributeOwnedByFederate(
      L"exercise", restartedOwner.membership->id,
      registered.objectInstanceHandle, *efficiency);
  REQUIRE(restoredOwnerState.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE_FALSE(restoredOwnerState.ownedByRequestingFederate);
  auto restoredRequesterState = restarted.attributeOwnedByFederate(
      L"exercise", restartedRequester.membership->id,
      registered.objectInstanceHandle, *efficiency);
  REQUIRE(restoredRequesterState.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE(restoredRequesterState.ownedByRequestingFederate);

  auto notificationDelivery =
      restarted.beginAttributeOwnershipDivestitureIfWantedNotification(
          L"exercise", restartedRequester.membership->id,
          registered.objectInstanceHandle, savedNotification.notificationId,
          efficiencyOnly);
  REQUIRE(notificationDelivery.has_value());
  REQUIRE(notificationDelivery->objectInstanceHandle ==
      registered.objectInstanceHandle);
  REQUIRE(notificationDelivery->securedAttributeHandles == efficiencyOnly);
  REQUIRE(notificationDelivery->followupWorkItems.empty());
  REQUIRE_FALSE(restarted.beginAttributeOwnershipDivestitureIfWantedNotification(
      L"exercise", restartedRequester.membership->id,
      registered.objectInstanceHandle, savedNotification.notificationId,
      efficiencyOnly));

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores a pending Confirm Divestiture notification in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-confirm-divestiture][ownership-ledger-state]"
    "[confirm-divestiture][negotiated-attribute-ownership-divestiture]"
    "[attribute-ownership-acquisition]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto requester = source.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(owner.status == FederationRegistryStatus::applied);
  REQUIRE(requester.status == FederationRegistryStatus::applied);
  REQUIRE(owner.membership);
  REQUIRE(requester.membership);

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", requester.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(discoveries.front().receivingFederateId == requester.membership->id);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", requester.membership->id,
      registered.objectInstanceHandle)
      .has_value());
  // The requester must publish after discovery so it remains an eligible
  // acquisition target through the negotiated confirmation boundary.
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", requester.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  std::string const valueBytes{"\x7f\x01", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *efficiency,
      rti1516_2025::VariableLengthData(valueBytes.data(), valueBytes.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise", owner.membership->id, registered.objectInstanceHandle,
      *server, {"HLAreliable"}, &values) == FederationRegistryStatus::applied);

  std::vector<unsigned char> const acquisitionTag{
      'r', 'e', 'g', '-', 'c', 'o', 'n', 'f', 'i', 'r', 'm'};
  auto acquisition = source.planAttributeOwnershipAcquisition(
      L"exercise", requester.membership->id,
      registered.objectInstanceHandle, efficiencyOnly, acquisitionTag);
  REQUIRE(acquisition.status ==
      umbra::detail::AttributeOwnershipAcquisitionStatus::applied);
  REQUIRE(acquisition.workItems.size() == 1U);
  REQUIRE(acquisition.workItems.front().kind ==
      umbra::detail::AttributeOwnershipAcquisitionWorkKind::request_release);
  auto const acquisitionRequestId = acquisition.workItems.front().requestId;

  std::vector<unsigned char> const divestitureTag{
      'd', 'i', 'v', '-', 'c', 'o', 'n', 'f', 'i', 'r', 'm'};
  auto divestiture = source.planNegotiatedAttributeOwnershipDivestiture(
      L"exercise", owner.membership->id, registered.objectInstanceHandle,
      efficiencyOnly, divestitureTag);
  REQUIRE(divestiture.status ==
      umbra::detail::NegotiatedAttributeOwnershipDivestitureStatus::applied);
  REQUIRE(divestiture.workItems.size() == 1U);
  auto const& sourceConfirmation = divestiture.workItems.front();
  REQUIRE(sourceConfirmation.requestId == acquisitionRequestId);
  REQUIRE_FALSE(sourceConfirmation.candidateIsIfAvailable);
  auto released = source.beginRequestDivestitureConfirmation(
      L"exercise", owner.membership->id, requester.membership->id,
      registered.objectInstanceHandle, sourceConfirmation.requestId, false,
      efficiencyOnly);
  REQUIRE(released.has_value());
  REQUIRE(released->releasedAttributeHandles == efficiencyOnly);

  std::vector<unsigned char> const confirmTag{
      'c', 'o', 'n', 'f', '-', 'd', 'i', 'v'};
  auto confirmed = source.planConfirmDivestiture(
      L"exercise", owner.membership->id, registered.objectInstanceHandle,
      efficiencyOnly, confirmTag);
  REQUIRE(confirmed.status ==
      umbra::detail::ConfirmDivestitureStatus::applied);
  REQUIRE(confirmed.notifications.size() == 1U);
  auto const& sourceNotification = confirmed.notifications.front();
  REQUIRE(sourceNotification.notificationId != 0U);
  REQUIRE(sourceNotification.receivingFederateId == requester.membership->id);
  REQUIRE(sourceNotification.objectInstanceHandle ==
      registered.objectInstanceHandle);
  REQUIRE(sourceNotification.attributeHandles == efficiencyOnly);
  REQUIRE(sourceNotification.userSuppliedTag == confirmTag);

  std::wstring const saveLabel = L"ownership-confirm-divestiture-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", requester.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", requester.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.handle == registered.objectInstanceHandle);
  REQUIRE(savedObject.attributeValuesPresent);
  REQUIRE(savedObject.attributeValues.size() == 1U);
  REQUIRE(savedObject.attributeValues.front().attributeHandle == *efficiency);
  REQUIRE(savedObject.attributeValues.front().value == valueBytes);
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionRequests.empty());
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionIfAvailableRequests.empty());
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionCancellations.empty());
  REQUIRE(savedObject.pendingAttributeOwnershipDivestitureIfWantedNotifications.empty());
  REQUIRE(savedObject.pendingNegotiatedAttributeOwnershipDivestitures.empty());
  REQUIRE(savedObject.pendingConfirmDivestitureNotifications.size() == 1U);
  auto const& savedNotification =
      savedObject.pendingConfirmDivestitureNotifications.front();
  REQUIRE(savedNotification.notificationId == sourceNotification.notificationId);
  REQUIRE(savedNotification.receivingFederateId == requester.membership->id);
  REQUIRE(savedNotification.attributeHandles ==
      std::vector<std::uint64_t>{*efficiency});
  REQUIRE(savedNotification.userSuppliedTag ==
      std::string(reinterpret_cast<char const*>(confirmTag.data()),
                  confirmTag.size()));
  REQUIRE(savedObject.pendingOperationCount == 1U);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedOwner = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto restartedRequester = restarted.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedRequester.membership);
  REQUIRE(restartedOwner.membership->id == owner.membership->id);
  REQUIRE(restartedRequester.membership->id == requester.membership->id);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedRequester.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto firstRestore = restarted.federateRestoreComplete(
      L"exercise", restartedOwner.membership->id);
  REQUIRE(firstRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(firstRestore.ownershipAcquisitionWorkItems.empty());
  auto restored = restarted.federateRestoreComplete(
      L"exercise", restartedRequester.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.ownershipAcquisitionWorkItems.empty());

  auto restoredOwnerState = restarted.attributeOwnedByFederate(
      L"exercise", restartedOwner.membership->id,
      registered.objectInstanceHandle, *efficiency);
  REQUIRE(restoredOwnerState.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE_FALSE(restoredOwnerState.ownedByRequestingFederate);
  auto restoredRequesterState = restarted.attributeOwnedByFederate(
      L"exercise", restartedRequester.membership->id,
      registered.objectInstanceHandle, *efficiency);
  REQUIRE(restoredRequesterState.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE(restoredRequesterState.ownedByRequestingFederate);

  auto notificationDelivery = restarted.beginConfirmDivestitureNotification(
      L"exercise", restartedRequester.membership->id,
      registered.objectInstanceHandle, savedNotification.notificationId,
      efficiencyOnly);
  REQUIRE(notificationDelivery.has_value());
  REQUIRE(notificationDelivery->objectInstanceHandle ==
      registered.objectInstanceHandle);
  REQUIRE(notificationDelivery->securedAttributeHandles == efficiencyOnly);
  REQUIRE(notificationDelivery->followupWorkItems.empty());
  REQUIRE_FALSE(restarted.beginConfirmDivestitureNotification(
      L"exercise", restartedRequester.membership->id,
      registered.objectInstanceHandle, savedNotification.notificationId,
      efficiencyOnly));

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores a pending ownership-acquisition cancellation in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-ownership-acquisition-cancellation][ownership-ledger-state]"
    "[attribute-ownership-acquisition][attribute-ownership-acquisition-cancellation]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto requester = source.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(owner.status == FederationRegistryStatus::applied);
  REQUIRE(requester.status == FederationRegistryStatus::applied);
  REQUIRE(owner.membership);
  REQUIRE(requester.membership);

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", requester.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(discoveries.front().receivingFederateId == requester.membership->id);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", requester.membership->id,
      registered.objectInstanceHandle)
      .has_value());
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", requester.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  std::string const valueBytes{"\x7b\x02", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *efficiency,
      rti1516_2025::VariableLengthData(valueBytes.data(), valueBytes.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise", owner.membership->id, registered.objectInstanceHandle,
      *server, {"HLAreliable"}, &values) == FederationRegistryStatus::applied);

  std::vector<unsigned char> const acquisitionTag{
      'r', 'e', 'g', '-', 'c', 'a', 'n', 'c', 'e', 'l'};
  auto acquisition = source.planAttributeOwnershipAcquisition(
      L"exercise", requester.membership->id,
      registered.objectInstanceHandle, efficiencyOnly, acquisitionTag);
  REQUIRE(acquisition.status ==
      umbra::detail::AttributeOwnershipAcquisitionStatus::applied);
  REQUIRE(acquisition.workItems.size() == 1U);
  REQUIRE(acquisition.workItems.front().kind ==
      umbra::detail::AttributeOwnershipAcquisitionWorkKind::request_release);

  auto cancellation = source.planAttributeOwnershipAcquisitionCancellation(
      L"exercise", requester.membership->id,
      registered.objectInstanceHandle, efficiencyOnly);
  REQUIRE(cancellation.status ==
      umbra::detail::AttributeOwnershipAcquisitionCancellationStatus::applied);
  REQUIRE(cancellation.cancellationId != 0U);
  REQUIRE(cancellation.attributeHandles == efficiencyOnly);
  REQUIRE(cancellation.callbackRoute);

  std::wstring const saveLabel = L"ownership-acquisition-cancellation-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", requester.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", requester.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.handle == registered.objectInstanceHandle);
  REQUIRE(savedObject.attributeValuesPresent);
  REQUIRE(savedObject.attributeValues.size() == 1U);
  REQUIRE(savedObject.attributeValues.front().attributeHandle == *efficiency);
  REQUIRE(savedObject.attributeValues.front().value == valueBytes);
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionRequests.size() == 1U);
  auto const& savedRequest = savedObject.pendingAttributeOwnershipAcquisitionRequests.front();
  REQUIRE(savedRequest.requestingFederateId == requester.membership->id);
  REQUIRE(savedRequest.desiredAttributeHandles ==
      std::vector<std::uint64_t>{*efficiency});
  REQUIRE(savedRequest.userSuppliedTag ==
      std::string(acquisitionTag.begin(), acquisitionTag.end()));
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionCancellations.size() == 1U);
  auto const& savedCancellation =
      savedObject.pendingAttributeOwnershipAcquisitionCancellations.front();
  REQUIRE(savedCancellation.cancellationId == cancellation.cancellationId);
  REQUIRE(savedCancellation.requestingFederateId == requester.membership->id);
  REQUIRE(savedCancellation.attributeHandles ==
      std::vector<std::uint64_t>{*efficiency});
  REQUIRE(savedObject.pendingOperationCount == 2U);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedOwner = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto restartedRequester = restarted.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedRequester.membership);
  REQUIRE(restartedOwner.membership->id == owner.membership->id);
  REQUIRE(restartedRequester.membership->id == requester.membership->id);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedRequester.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto firstRestore = restarted.federateRestoreComplete(
      L"exercise", restartedOwner.membership->id);
  REQUIRE(firstRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(firstRestore.ownershipAcquisitionWorkItems.empty());
  REQUIRE(firstRestore.ownershipAcquisitionCancellationWorkItems.empty());
  auto restored = restarted.federateRestoreComplete(
      L"exercise", restartedRequester.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.ownershipAcquisitionWorkItems.empty());
  REQUIRE(restored.ownershipAcquisitionCancellationWorkItems.size() == 1U);
  auto const& restoredCancellation =
      restored.ownershipAcquisitionCancellationWorkItems.front();
  REQUIRE(restoredCancellation.requestingFederateId ==
      restartedRequester.membership->id);
  REQUIRE(restoredCancellation.objectInstanceHandle ==
      registered.objectInstanceHandle);
  REQUIRE(restoredCancellation.cancellationId == cancellation.cancellationId);
  REQUIRE(restoredCancellation.attributeHandles == efficiencyOnly);
  REQUIRE(restoredCancellation.callbackRoute);

  auto delivered = restarted.beginAttributeOwnershipAcquisitionCancellation(
      L"exercise", restartedRequester.membership->id,
      registered.objectInstanceHandle, restoredCancellation.cancellationId,
      efficiencyOnly);
  REQUIRE(delivered.has_value());
  REQUIRE(delivered->objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(delivered->confirmedAttributeHandles == efficiencyOnly);
  REQUIRE(delivered->followupWorkItems.empty());
  REQUIRE_FALSE(restarted.beginAttributeOwnershipAcquisitionCancellation(
      L"exercise", restartedRequester.membership->id,
      registered.objectInstanceHandle, restoredCancellation.cancellationId,
      efficiencyOnly));

  auto restoredOwnerState = restarted.attributeOwnedByFederate(
      L"exercise", restartedOwner.membership->id,
      registered.objectInstanceHandle, *efficiency);
  REQUIRE(restoredOwnerState.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE(restoredOwnerState.ownedByRequestingFederate);
  auto restoredRequesterState = restarted.attributeOwnedByFederate(
      L"exercise", restartedRequester.membership->id,
      registered.objectInstanceHandle, *efficiency);
  REQUIRE(restoredRequesterState.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE_FALSE(restoredRequesterState.ownedByRequestingFederate);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores a pending attribute transportation-type change in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-attribute-transportation-type-change][ownership-ledger-state]"
    "[attribute-transportation-type-change][transportation-management]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto sourceJoined = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  REQUIRE(sourceJoined.status == FederationRegistryStatus::applied);
  REQUIRE(sourceJoined.membership);
  auto const ownerId = sourceJoined.membership->id;

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", ownerId, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", ownerId, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);

  std::string const valueBytes{"\x4a\x06", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *efficiency,
      rti1516_2025::VariableLengthData(valueBytes.data(), valueBytes.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise", ownerId, registered.objectInstanceHandle,
      *server, {"HLAreliable"}, &values) == FederationRegistryStatus::applied);

  auto change = source.planAttributeTransportationTypeChange(
      L"exercise", ownerId, registered.objectInstanceHandle,
      efficiencyOnly, "HLAbestEffort");
  REQUIRE(change.status ==
      umbra::detail::AttributeTransportationTypeChangeStatus::applied);
  REQUIRE(change.requestId != 0U);
  REQUIRE(change.objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(change.attributeHandles == efficiencyOnly);
  REQUIRE(change.transportationName == "HLAbestEffort");
  REQUIRE(change.callbackRoute);

  std::wstring const saveLabel = L"attribute-transportation-type-change-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", ownerId, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", ownerId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveComplete(
      L"exercise", ownerId).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.handle == registered.objectInstanceHandle);
  REQUIRE(savedObject.attributeValuesPresent);
  REQUIRE(savedObject.attributeValues.size() == 1U);
  REQUIRE(savedObject.attributeValues.front().attributeHandle == *efficiency);
  REQUIRE(savedObject.attributeValues.front().value == valueBytes);
  REQUIRE(savedObject.pendingAttributeTransportationTypeChanges.size() == 1U);
  auto const& savedChange =
      savedObject.pendingAttributeTransportationTypeChanges.front();
  REQUIRE(savedChange.requestId == change.requestId);
  REQUIRE(savedChange.requestingFederateId == ownerId);
  REQUIRE(savedChange.attributeHandles ==
      std::vector<std::uint64_t>{*efficiency});
  REQUIRE(savedChange.transportationName == "HLAbestEffort");
  REQUIRE(savedObject.pendingOperationCount == 1U);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedJoined = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == ownerId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", ownerId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(L"exercise", ownerId);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.attributeTransportationTypeChangeWorkItems.size() == 1U);
  auto const& restoredChange =
      restored.attributeTransportationTypeChangeWorkItems.front();
  REQUIRE(restoredChange.requestingFederateId == ownerId);
  REQUIRE(restoredChange.objectInstanceHandle ==
      registered.objectInstanceHandle);
  REQUIRE(restoredChange.requestId == change.requestId);
  REQUIRE(restoredChange.callbackRoute);

  auto delivered = restarted.beginAttributeTransportationTypeChange(
      L"exercise", ownerId, restoredChange.requestId);
  REQUIRE(delivered.has_value());
  REQUIRE(delivered->objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(delivered->attributeHandles == efficiencyOnly);
  REQUIRE(delivered->transportationName == "HLAbestEffort");
  REQUIRE_FALSE(restarted.beginAttributeTransportationTypeChange(
      L"exercise", ownerId, restoredChange.requestId));

  auto query = restarted.attributeTransportationTypeQueryFor(
      L"exercise", ownerId, registered.objectInstanceHandle, *efficiency);
  REQUIRE(query.has_value());
  REQUIRE(query->transportationName == "HLAbestEffort");

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores a pending interaction transportation-type change in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-interaction-transportation-type-change][interaction-declaration-state]"
    "[interaction-transportation-type-change][transportation-management]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto sourceJoined = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  REQUIRE(sourceJoined.status == FederationRegistryStatus::applied);
  REQUIRE(sourceJoined.membership);
  auto const ownerId = sourceJoined.membership->id;

  auto const takeOrder = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.ServerAction.TakeOrder");
  REQUIRE(takeOrder.has_value());
  REQUIRE(source.setInteractionClassPublication(
      L"exercise", ownerId, *takeOrder, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);

  auto change = source.planInteractionTransportationTypeChange(
      L"exercise", ownerId, *takeOrder, "HLAbestEffort");
  REQUIRE(change.status ==
      umbra::detail::InteractionTransportationTypeChangeStatus::applied);
  REQUIRE(change.interactionClassHandle == *takeOrder);
  REQUIRE(change.transportationName == "HLAbestEffort");
  REQUIRE(change.callbackRoute);

  std::wstring const saveLabel =
      L"interaction-transportation-type-change-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", ownerId, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", ownerId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveComplete(
      L"exercise", ownerId).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.interactionDeclarationCount == 1U);
  REQUIRE(image.interactionDeclarations.size() == 1U);
  auto const& savedDeclaration = image.interactionDeclarations.front();
  REQUIRE(savedDeclaration.federateId == ownerId);
  REQUIRE(savedDeclaration.publishedInteractionClasses ==
      std::vector<std::uint64_t>{*takeOrder});
  REQUIRE(savedDeclaration.subscribedInteractionClasses.empty());
  REQUIRE(savedDeclaration.regionalSubscribedInteractionClasses.empty());
  REQUIRE(savedDeclaration.publishedObjectClassDirectedInteractions.empty());
  REQUIRE(savedDeclaration.subscribedObjectClassDirectedInteractions.empty());
  REQUIRE(savedDeclaration.interactionTransportationTypes.empty());
  REQUIRE(savedDeclaration.interactionOrderTypes.empty());
  REQUIRE(savedDeclaration.pendingInteractionTransportationTypeChanges.size() == 1U);
  auto const& savedChange =
      savedDeclaration.pendingInteractionTransportationTypeChanges.front();
  REQUIRE(savedChange.interactionClassHandle == *takeOrder);
  REQUIRE(savedChange.value == "HLAbestEffort");
  REQUIRE(image.objects.empty());
  REQUIRE(image.tsoInteractionMessages.empty());
  REQUIRE(image.tsoAttributeUpdateMessages.empty());
  REQUIRE(image.tsoObjectDeletionMessages.empty());
  REQUIRE(image.tsoDirectedInteractionMessages.empty());
  REQUIRE(image.tsoRequestRetractionRecords.empty());
  REQUIRE(image.tsoQueueEntries.empty());

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedJoined = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == ownerId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", ownerId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(L"exercise", ownerId);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.interactionTransportationTypeChangeWorkItems.size() == 1U);
  auto const& restoredChange =
      restored.interactionTransportationTypeChangeWorkItems.front();
  REQUIRE(restoredChange.requestingFederateId == ownerId);
  REQUIRE(restoredChange.interactionClassHandle == *takeOrder);
  REQUIRE(restoredChange.callbackRoute);

  auto delivered = restarted.beginInteractionTransportationTypeChange(
      L"exercise", ownerId, restoredChange.interactionClassHandle);
  REQUIRE(delivered.has_value());
  REQUIRE(*delivered == "HLAbestEffort");
  REQUIRE_FALSE(restarted.beginInteractionTransportationTypeChange(
      L"exercise", ownerId, restoredChange.interactionClassHandle));

  auto query = restarted.interactionTransportationTypeQueryFor(
      L"exercise", ownerId, ownerId, *takeOrder);
  REQUIRE(query.has_value());
  REQUIRE(query->transportationName == "HLAbestEffort");

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Federation restore rejects a corrupt durable state image",
    "[unit][kernel][federation-registry][save-restore][durable-save][state-image][restore][failure]") {
  class CorruptStateImageStore final : public umbra::detail::FederationSaveCommitStore {
   public:
    void commit(umbra::detail::FederationSaveCommitDescriptor const& descriptor) override {
      committed = descriptor;
    }

    [[nodiscard]] std::optional<umbra::detail::FederationSaveCommitDescriptor> load(
        std::wstring const&,
        std::wstring const&) const override {
      auto result = committed;
      auto image = umbra::detail::FederationStateImageCodec::decode(
          result.stateImage);
      ++image.normalizationSeed;
      if (image.interactionDeclarations.empty() && !image.members.empty()) {
        umbra::detail::FederationStateImageInteractionDeclaration declaration;
        declaration.federateId = image.members.front().id;
        declaration.publishedInteractionClasses = {1U};
        image.interactionDeclarations.push_back(std::move(declaration));
        image.interactionDeclarationCount = image.interactionDeclarations.size();
      } else if (!image.interactionDeclarations.empty()) {
        image.interactionDeclarations.front().publishedInteractionClasses.push_back(
            image.interactionDeclarations.front().publishedInteractionClasses.empty()
                ? 1U
                : image.interactionDeclarations.front().publishedInteractionClasses.back() + 1U);
      }
      result.stateImage = umbra::detail::FederationStateImageCodec::encode(image);
      return result;
    }

    umbra::detail::FederationSaveCommitDescriptor committed;
  };

  auto store = std::make_shared<CorruptStateImageStore>();
  EmbeddedFederationRegistry registry({}, store);
  REQUIRE(registry.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto joined = registry.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(joined.status == FederationRegistryStatus::applied);
  REQUIRE(joined.membership);
  REQUIRE(registry.requestFederationSave(
      L"exercise", joined.membership->id, L"checkpoint").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", joined.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveComplete(
      L"exercise", joined.membership->id).saveCompletedSuccessfully);

  auto restore = registry.requestFederationRestore(
      L"exercise", joined.membership->id, L"checkpoint");
  REQUIRE(restore.status == umbra::detail::FederationRestoreControlStatus::snapshot_not_found);
  REQUIRE(restore.notifications.size() == 1U);
}

TEST_CASE(
    "Filesystem state image restores a published interaction declaration in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-interaction-declaration][interaction-declaration-state]"
    "[interaction-management][declaration-management]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto sourceJoined = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  REQUIRE(sourceJoined.status == FederationRegistryStatus::applied);
  REQUIRE(sourceJoined.membership);
  auto const ownerId = sourceJoined.membership->id;

  auto const takeOrder = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.ServerAction.TakeOrder");
  REQUIRE(takeOrder.has_value());
  REQUIRE(source.setInteractionClassPublication(
      L"exercise", ownerId, *takeOrder, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);

  std::wstring const saveLabel = L"interaction-declaration-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", ownerId, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", ownerId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveComplete(
      L"exercise", ownerId).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.interactionDeclarationCount == 1U);
  REQUIRE(image.interactionDeclarations.size() == 1U);
  auto const& savedDeclaration = image.interactionDeclarations.front();
  REQUIRE(savedDeclaration.federateId == ownerId);
  REQUIRE(savedDeclaration.publishedInteractionClasses ==
      std::vector<std::uint64_t>{*takeOrder});
  REQUIRE(savedDeclaration.subscribedInteractionClasses.empty());
  REQUIRE(savedDeclaration.regionalSubscribedInteractionClasses.empty());
  REQUIRE(savedDeclaration.publishedObjectClassDirectedInteractions.empty());
  REQUIRE(savedDeclaration.subscribedObjectClassDirectedInteractions.empty());
  REQUIRE(savedDeclaration.interactionTransportationTypes.empty());
  REQUIRE(savedDeclaration.interactionOrderTypes.empty());
  REQUIRE(savedDeclaration.pendingInteractionTransportationTypeChanges.empty());
  REQUIRE(image.objects.empty());
  REQUIRE(image.tsoInteractionMessages.empty());
  REQUIRE(image.tsoAttributeUpdateMessages.empty());
  REQUIRE(image.tsoObjectDeletionMessages.empty());
  REQUIRE(image.tsoDirectedInteractionMessages.empty());
  REQUIRE(image.tsoRequestRetractionRecords.empty());
  REQUIRE(image.tsoQueueEntries.empty());

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedJoined = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == ownerId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", ownerId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(L"exercise", ownerId);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.interactionTransportationTypeChangeWorkItems.empty());

  auto declaration = restarted.interactionClassDeclarationFor(
      L"exercise", ownerId, *takeOrder);
  REQUIRE(declaration.has_value());
  REQUIRE(declaration->published);
  REQUIRE_FALSE(declaration->subscriptionActive.has_value());

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores an interaction subscription declaration in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-interaction-declaration][interaction-declaration-state]"
    "[interaction-management][declaration-management]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto sourceJoined = source.join(
      L"exercise", L"subscriber", L"subscriber", noOpCallbackRoute());
  REQUIRE(sourceJoined.status == FederationRegistryStatus::applied);
  REQUIRE(sourceJoined.membership);
  auto const subscriberId = sourceJoined.membership->id;

  auto const takeOrder = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.ServerAction.TakeOrder");
  REQUIRE(takeOrder.has_value());
  REQUIRE(source.setInteractionClassSubscription(
      L"exercise", subscriberId, *takeOrder, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);

  std::wstring const saveLabel = L"interaction-subscription-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", subscriberId, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", subscriberId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveComplete(
      L"exercise", subscriberId).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.interactionDeclarationCount == 1U);
  REQUIRE(image.interactionDeclarations.size() == 1U);
  auto const& savedDeclaration = image.interactionDeclarations.front();
  REQUIRE(savedDeclaration.federateId == subscriberId);
  REQUIRE(savedDeclaration.publishedInteractionClasses.empty());
  REQUIRE(savedDeclaration.subscribedInteractionClasses.size() == 1U);
  REQUIRE(savedDeclaration.subscribedInteractionClasses.front().interactionClassHandle ==
      *takeOrder);
  REQUIRE(savedDeclaration.subscribedInteractionClasses.front().active);
  REQUIRE(savedDeclaration.regionalSubscribedInteractionClasses.empty());
  REQUIRE(savedDeclaration.publishedObjectClassDirectedInteractions.empty());
  REQUIRE(savedDeclaration.subscribedObjectClassDirectedInteractions.empty());
  REQUIRE(savedDeclaration.interactionTransportationTypes.empty());
  REQUIRE(savedDeclaration.interactionOrderTypes.empty());
  REQUIRE(savedDeclaration.pendingInteractionTransportationTypeChanges.empty());
  REQUIRE(image.objects.empty());
  REQUIRE(image.tsoInteractionMessages.empty());
  REQUIRE(image.tsoAttributeUpdateMessages.empty());
  REQUIRE(image.tsoObjectDeletionMessages.empty());
  REQUIRE(image.tsoDirectedInteractionMessages.empty());
  REQUIRE(image.tsoRequestRetractionRecords.empty());
  REQUIRE(image.tsoQueueEntries.empty());

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedJoined = restarted.join(
      L"exercise", L"subscriber", L"subscriber", noOpCallbackRoute());
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == subscriberId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", subscriberId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(L"exercise", subscriberId);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.interactionTransportationTypeChangeWorkItems.empty());

  auto declaration = restarted.interactionClassDeclarationFor(
      L"exercise", subscriberId, *takeOrder);
  REQUIRE(declaration.has_value());
  REQUIRE_FALSE(declaration->published);
  REQUIRE(declaration->subscriptionActive.has_value());
  REQUIRE(*declaration->subscriptionActive);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores a same-class interaction publication and subscription in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-interaction-declaration][interaction-declaration-state]"
    "[interaction-management][declaration-management]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto sourceJoined = source.join(
      L"exercise", L"publisher-subscriber", L"dual", noOpCallbackRoute());
  REQUIRE(sourceJoined.status == FederationRegistryStatus::applied);
  REQUIRE(sourceJoined.membership);
  auto const federateId = sourceJoined.membership->id;

  auto const takeOrder = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.ServerAction.TakeOrder");
  REQUIRE(takeOrder.has_value());
  REQUIRE(source.setInteractionClassPublication(
      L"exercise", federateId, *takeOrder, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);
  REQUIRE(source.setInteractionClassSubscription(
      L"exercise", federateId, *takeOrder, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);

  std::wstring const saveLabel = L"interaction-publication-subscription-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", federateId, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveComplete(
      L"exercise", federateId).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.interactionDeclarationCount == 1U);
  REQUIRE(image.interactionDeclarations.size() == 1U);
  auto const& savedDeclaration = image.interactionDeclarations.front();
  REQUIRE(savedDeclaration.federateId == federateId);
  REQUIRE(savedDeclaration.publishedInteractionClasses ==
      std::vector<std::uint64_t>{*takeOrder});
  REQUIRE(savedDeclaration.subscribedInteractionClasses.size() == 1U);
  REQUIRE(savedDeclaration.subscribedInteractionClasses.front().interactionClassHandle ==
      *takeOrder);
  REQUIRE(savedDeclaration.subscribedInteractionClasses.front().active);
  REQUIRE(savedDeclaration.regionalSubscribedInteractionClasses.empty());
  REQUIRE(savedDeclaration.publishedObjectClassDirectedInteractions.empty());
  REQUIRE(savedDeclaration.subscribedObjectClassDirectedInteractions.empty());
  REQUIRE(savedDeclaration.interactionTransportationTypes.empty());
  REQUIRE(savedDeclaration.interactionOrderTypes.empty());
  REQUIRE(savedDeclaration.pendingInteractionTransportationTypeChanges.empty());
  REQUIRE(image.objects.empty());
  REQUIRE(image.tsoInteractionMessages.empty());
  REQUIRE(image.tsoAttributeUpdateMessages.empty());
  REQUIRE(image.tsoObjectDeletionMessages.empty());
  REQUIRE(image.tsoDirectedInteractionMessages.empty());
  REQUIRE(image.tsoRequestRetractionRecords.empty());
  REQUIRE(image.tsoQueueEntries.empty());

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedJoined = restarted.join(
      L"exercise", L"publisher-subscriber", L"dual", noOpCallbackRoute());
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == federateId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", federateId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(L"exercise", federateId);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.interactionTransportationTypeChangeWorkItems.empty());

  auto declaration = restarted.interactionClassDeclarationFor(
      L"exercise", federateId, *takeOrder);
  REQUIRE(declaration.has_value());
  REQUIRE(declaration->published);
  REQUIRE(declaration->subscriptionActive.has_value());
  REQUIRE(*declaration->subscriptionActive);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores a regional interaction subscription in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-interaction-declaration][interaction-declaration-state]"
    "[interaction-management][ddm][regional-interaction]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto sourceJoined = source.join(
      L"exercise", L"regional-subscriber", L"regional", noOpCallbackRoute());
  REQUIRE(sourceJoined.status == FederationRegistryStatus::applied);
  REQUIRE(sourceJoined.membership);
  auto const federateId = sourceJoined.membership->id;

  auto const dimension = source.dimensionHandleFor(L"exercise", "ServerId");
  auto const takeOrder = source.interactionClassHandleFor(
      L"exercise",
      "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
  REQUIRE(dimension.has_value());
  REQUIRE(takeOrder.has_value());
  auto created = source.createRegion(L"exercise", federateId, {*dimension});
  REQUIRE(created.status == umbra::detail::RegionServiceStatus::applied);
  REQUIRE(created.regionHandle != 0U);
  REQUIRE(source.setRangeBounds(
      L"exercise", federateId, created.regionHandle, *dimension,
      umbra::detail::RegionRangeBounds{2UL, 5UL}) ==
      umbra::detail::RegionServiceStatus::applied);
  REQUIRE(source.commitRegionModifications(
      L"exercise", federateId, {created.regionHandle}) ==
      umbra::detail::RegionServiceStatus::applied);
  REQUIRE(source.setInteractionClassRegionalSubscription(
      L"exercise", federateId, *takeOrder, {created.regionHandle}, true) ==
      umbra::detail::RegionalInteractionClassDeclarationStatus::applied);

  std::wstring const saveLabel = L"interaction-regional-subscription-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", federateId, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveComplete(
      L"exercise", federateId).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.interactionDeclarationCount == 1U);
  REQUIRE(image.interactionDeclarations.size() == 1U);
  auto const& savedDeclaration = image.interactionDeclarations.front();
  REQUIRE(savedDeclaration.federateId == federateId);
  REQUIRE(savedDeclaration.publishedInteractionClasses.empty());
  REQUIRE(savedDeclaration.subscribedInteractionClasses.empty());
  REQUIRE(savedDeclaration.regionalSubscribedInteractionClasses.size() == 1U);
  auto const& savedSubscription =
      savedDeclaration.regionalSubscribedInteractionClasses.front();
  REQUIRE(savedSubscription.interactionClassHandle == *takeOrder);
  REQUIRE(savedSubscription.regionHandle == created.regionHandle);
  REQUIRE(savedSubscription.active);
  REQUIRE(image.regions.size() == 1U);
  REQUIRE(image.objects.empty());
  REQUIRE(image.tsoInteractionMessages.empty());
  REQUIRE(image.tsoAttributeUpdateMessages.empty());
  REQUIRE(image.tsoObjectDeletionMessages.empty());
  REQUIRE(image.tsoDirectedInteractionMessages.empty());
  REQUIRE(image.tsoRequestRetractionRecords.empty());
  REQUIRE(image.tsoQueueEntries.empty());

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedJoined = restarted.join(
      L"exercise", L"regional-subscriber", L"regional", noOpCallbackRoute());
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == federateId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", federateId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(L"exercise", federateId);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.interactionTransportationTypeChangeWorkItems.empty());

  auto dimensions = restarted.dimensionHandleSetForRegion(
      L"exercise", federateId, created.regionHandle);
  REQUIRE(dimensions.status == umbra::detail::RegionServiceStatus::applied);
  REQUIRE(dimensions.dimensionHandles == std::set<std::uint64_t>{*dimension});
  auto bounds = restarted.rangeBoundsForRegion(
      L"exercise", federateId, created.regionHandle, *dimension);
  REQUIRE(bounds.status == umbra::detail::RegionServiceStatus::applied);
  REQUIRE(bounds.range.lowerBound == 2UL);
  REQUIRE(bounds.range.upperBound == 5UL);

  std::wstring const roundTripLabel =
      L"interaction-regional-subscription-round-trip";
  REQUIRE(restarted.requestFederationSave(
      L"exercise", federateId, roundTripLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveBegun(
      L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveComplete(
      L"exercise", federateId).saveCompletedSuccessfully);
  auto roundTrip = store->load(L"exercise", roundTripLabel);
  REQUIRE(roundTrip.has_value());
  auto roundTripImage = umbra::detail::FederationStateImageCodec::decode(
      roundTrip->stateImage);
  REQUIRE(roundTripImage.interactionDeclarations.size() == 1U);
  REQUIRE(roundTripImage.interactionDeclarations.front()
              .regionalSubscribedInteractionClasses.size() == 1U);
  REQUIRE(roundTripImage.interactionDeclarations.front()
              .regionalSubscribedInteractionClasses.front()
              .regionHandle == created.regionHandle);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores multiple interaction declaration entries in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-interaction-declaration][interaction-declaration-state]"
    "[interaction-management][declaration-management]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto sourceJoined = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  REQUIRE(sourceJoined.status == FederationRegistryStatus::applied);
  REQUIRE(sourceJoined.membership);
  auto const ownerId = sourceJoined.membership->id;

  auto const customerSeated = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.CustomerTransactions.CustomerSeated");
  auto const takeOrder = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.ServerAction.TakeOrder");
  REQUIRE(customerSeated.has_value());
  REQUIRE(takeOrder.has_value());
  std::vector<std::uint64_t> expectedPublished{
      *customerSeated,
      *takeOrder,
  };
  std::ranges::sort(expectedPublished);
  REQUIRE(source.setInteractionClassPublication(
      L"exercise", ownerId, *customerSeated, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);
  REQUIRE(source.setInteractionClassPublication(
      L"exercise", ownerId, *takeOrder, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);

  std::wstring const saveLabel = L"interaction-multiple-declarations-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", ownerId, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", ownerId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveComplete(
      L"exercise", ownerId).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.interactionDeclarationCount == 1U);
  REQUIRE(image.interactionDeclarations.size() == 1U);
  auto const& savedDeclaration = image.interactionDeclarations.front();
  REQUIRE(savedDeclaration.federateId == ownerId);
  REQUIRE(savedDeclaration.publishedInteractionClasses == expectedPublished);
  REQUIRE(savedDeclaration.subscribedInteractionClasses.empty());
  REQUIRE(savedDeclaration.regionalSubscribedInteractionClasses.empty());
  REQUIRE(savedDeclaration.publishedObjectClassDirectedInteractions.empty());
  REQUIRE(savedDeclaration.subscribedObjectClassDirectedInteractions.empty());
  REQUIRE(savedDeclaration.interactionTransportationTypes.empty());
  REQUIRE(savedDeclaration.interactionOrderTypes.empty());
  REQUIRE(savedDeclaration.pendingInteractionTransportationTypeChanges.empty());
  REQUIRE(image.objects.empty());
  REQUIRE(image.tsoInteractionMessages.empty());
  REQUIRE(image.tsoAttributeUpdateMessages.empty());
  REQUIRE(image.tsoObjectDeletionMessages.empty());
  REQUIRE(image.tsoDirectedInteractionMessages.empty());
  REQUIRE(image.tsoRequestRetractionRecords.empty());
  REQUIRE(image.tsoQueueEntries.empty());

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedJoined = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == ownerId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", ownerId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(L"exercise", ownerId);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.interactionTransportationTypeChangeWorkItems.empty());

  auto customerDeclaration = restarted.interactionClassDeclarationFor(
      L"exercise", ownerId, *customerSeated);
  auto takeOrderDeclaration = restarted.interactionClassDeclarationFor(
      L"exercise", ownerId, *takeOrder);
  REQUIRE(customerDeclaration.has_value());
  REQUIRE(customerDeclaration->published);
  REQUIRE_FALSE(customerDeclaration->subscriptionActive.has_value());
  REQUIRE(takeOrderDeclaration.has_value());
  REQUIRE(takeOrderDeclaration->published);
  REQUIRE_FALSE(takeOrderDeclaration->subscriptionActive.has_value());

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores independent interaction declarations for multiple federates",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-interaction-declaration][interaction-declaration-state]"
    "[interaction-management][declaration-management][multi-federate-callback-ordering]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto first = source.join(
      L"exercise", L"publisher-a", L"publisher-a", noOpCallbackRoute());
  auto second = source.join(
      L"exercise", L"publisher-b", L"publisher-b", noOpCallbackRoute());
  REQUIRE(first.status == FederationRegistryStatus::applied);
  REQUIRE(second.status == FederationRegistryStatus::applied);
  REQUIRE(first.membership);
  REQUIRE(second.membership);
  auto const firstId = first.membership->id;
  auto const secondId = second.membership->id;

  auto const firstClass = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.CustomerTransactions.CustomerSeated");
  auto const secondClass = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.ServerAction.TakeOrder");
  REQUIRE(firstClass.has_value());
  REQUIRE(secondClass.has_value());
  REQUIRE(source.setInteractionClassPublication(
      L"exercise", firstId, *firstClass, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);
  REQUIRE(source.setInteractionClassPublication(
      L"exercise", secondId, *secondClass, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);

  std::wstring const saveLabel = L"interaction-multi-federate-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", firstId, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", firstId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", secondId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", firstId).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", secondId).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.interactionDeclarationCount == 2U);
  REQUIRE(image.interactionDeclarations.size() == 2U);
  REQUIRE(image.interactionDeclarations[0].federateId == firstId);
  REQUIRE(image.interactionDeclarations[1].federateId == secondId);
  REQUIRE(image.interactionDeclarations[0].publishedInteractionClasses ==
      std::vector<std::uint64_t>{*firstClass});
  REQUIRE(image.interactionDeclarations[1].publishedInteractionClasses ==
      std::vector<std::uint64_t>{*secondClass});
  REQUIRE(image.objects.empty());
  REQUIRE(image.tsoInteractionMessages.empty());
  REQUIRE(image.tsoAttributeUpdateMessages.empty());
  REQUIRE(image.tsoObjectDeletionMessages.empty());
  REQUIRE(image.tsoDirectedInteractionMessages.empty());
  REQUIRE(image.tsoRequestRetractionRecords.empty());
  REQUIRE(image.tsoQueueEntries.empty());

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedFirst = restarted.join(
      L"exercise", L"publisher-a", L"publisher-a", noOpCallbackRoute());
  auto restartedSecond = restarted.join(
      L"exercise", L"publisher-b", L"publisher-b", noOpCallbackRoute());
  REQUIRE(restartedFirst.status == FederationRegistryStatus::applied);
  REQUIRE(restartedSecond.status == FederationRegistryStatus::applied);
  REQUIRE(restartedFirst.membership);
  REQUIRE(restartedSecond.membership);
  REQUIRE(restartedFirst.membership->id == firstId);
  REQUIRE(restartedSecond.membership->id == secondId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", firstId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", firstId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(
      L"exercise", secondId);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.interactionTransportationTypeChangeWorkItems.empty());

  auto firstDeclaration = restarted.interactionClassDeclarationFor(
      L"exercise", firstId, *firstClass);
  auto secondDeclaration = restarted.interactionClassDeclarationFor(
      L"exercise", secondId, *secondClass);
  REQUIRE(firstDeclaration.has_value());
  REQUIRE(firstDeclaration->published);
  REQUIRE(secondDeclaration.has_value());
  REQUIRE(secondDeclaration->published);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores a committed interaction transportation-type override in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-interaction-transportation-type-override]"
    "[interaction-declaration-state]"
    "[interaction-transportation-type-change][transportation-management]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto sourceJoined = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  REQUIRE(sourceJoined.status == FederationRegistryStatus::applied);
  REQUIRE(sourceJoined.membership);
  auto const ownerId = sourceJoined.membership->id;

  auto const takeOrder = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.ServerAction.TakeOrder");
  REQUIRE(takeOrder.has_value());
  REQUIRE(source.setInteractionClassPublication(
      L"exercise", ownerId, *takeOrder, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);

  auto change = source.planInteractionTransportationTypeChange(
      L"exercise", ownerId, *takeOrder, "HLAbestEffort");
  REQUIRE(change.status ==
      umbra::detail::InteractionTransportationTypeChangeStatus::applied);
  auto committedChange = source.beginInteractionTransportationTypeChange(
      L"exercise", ownerId, *takeOrder);
  REQUIRE(committedChange.has_value());
  REQUIRE(*committedChange == "HLAbestEffort");
  REQUIRE_FALSE(source.beginInteractionTransportationTypeChange(
      L"exercise", ownerId, *takeOrder));

  auto sourceQuery = source.interactionTransportationTypeQueryFor(
      L"exercise", ownerId, ownerId, *takeOrder);
  REQUIRE(sourceQuery.has_value());
  REQUIRE(sourceQuery->transportationName == "HLAbestEffort");

  std::wstring const saveLabel =
      L"interaction-transportation-type-override-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", ownerId, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", ownerId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveComplete(
      L"exercise", ownerId).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.interactionDeclarationCount == 1U);
  REQUIRE(image.interactionDeclarations.size() == 1U);
  auto const& savedDeclaration = image.interactionDeclarations.front();
  REQUIRE(savedDeclaration.federateId == ownerId);
  REQUIRE(savedDeclaration.publishedInteractionClasses ==
      std::vector<std::uint64_t>{*takeOrder});
  REQUIRE(savedDeclaration.subscribedInteractionClasses.empty());
  REQUIRE(savedDeclaration.regionalSubscribedInteractionClasses.empty());
  REQUIRE(savedDeclaration.interactionTransportationTypes.size() == 1U);
  REQUIRE(savedDeclaration.interactionTransportationTypes.front().interactionClassHandle ==
      *takeOrder);
  REQUIRE(savedDeclaration.interactionTransportationTypes.front().value ==
      "HLAbestEffort");
  REQUIRE(savedDeclaration.interactionOrderTypes.empty());
  REQUIRE(savedDeclaration.pendingInteractionTransportationTypeChanges.empty());
  REQUIRE(image.objects.empty());
  REQUIRE(image.tsoInteractionMessages.empty());
  REQUIRE(image.tsoAttributeUpdateMessages.empty());
  REQUIRE(image.tsoObjectDeletionMessages.empty());
  REQUIRE(image.tsoDirectedInteractionMessages.empty());
  REQUIRE(image.tsoRequestRetractionRecords.empty());
  REQUIRE(image.tsoQueueEntries.empty());

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedJoined = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == ownerId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", ownerId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(L"exercise", ownerId);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.interactionTransportationTypeChangeWorkItems.empty());

  auto restoredDeclaration = restarted.interactionClassDeclarationFor(
      L"exercise", ownerId, *takeOrder);
  REQUIRE(restoredDeclaration.has_value());
  REQUIRE(restoredDeclaration->published);
  auto restoredQuery = restarted.interactionTransportationTypeQueryFor(
      L"exercise", ownerId, ownerId, *takeOrder);
  REQUIRE(restoredQuery.has_value());
  REQUIRE(restoredQuery->transportationName == "HLAbestEffort");

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores a mixed interaction publication and subscription for multiple federates",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-interaction-declaration][interaction-declaration-state]"
    "[interaction-management][declaration-management][multi-federate-callback-ordering]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto publisher = source.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto subscriber = source.join(
      L"exercise", L"subscriber", L"subscriber", noOpCallbackRoute());
  REQUIRE(publisher.status == FederationRegistryStatus::applied);
  REQUIRE(subscriber.status == FederationRegistryStatus::applied);
  REQUIRE(publisher.membership);
  REQUIRE(subscriber.membership);
  auto const publisherId = publisher.membership->id;
  auto const subscriberId = subscriber.membership->id;

  auto const publishedClass = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.CustomerTransactions.CustomerSeated");
  auto const subscribedClass = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.ServerAction.TakeOrder");
  REQUIRE(publishedClass.has_value());
  REQUIRE(subscribedClass.has_value());
  REQUIRE(source.setInteractionClassPublication(
      L"exercise", publisherId, *publishedClass, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);
  REQUIRE(source.setInteractionClassSubscription(
      L"exercise", subscriberId, *subscribedClass, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);

  std::wstring const saveLabel = L"interaction-mixed-multi-federate-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", publisherId, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", publisherId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", subscriberId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", publisherId).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", subscriberId).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.interactionDeclarationCount == 2U);
  REQUIRE(image.interactionDeclarations.size() == 2U);
  REQUIRE(image.interactionDeclarations[0].federateId == publisherId);
  REQUIRE(image.interactionDeclarations[1].federateId == subscriberId);
  REQUIRE(image.interactionDeclarations[0].publishedInteractionClasses ==
      std::vector<std::uint64_t>{*publishedClass});
  REQUIRE(image.interactionDeclarations[0].subscribedInteractionClasses.empty());
  REQUIRE(image.interactionDeclarations[1].publishedInteractionClasses.empty());
  REQUIRE(image.interactionDeclarations[1].subscribedInteractionClasses.size() == 1U);
  REQUIRE(image.interactionDeclarations[1].subscribedInteractionClasses.front().
      interactionClassHandle == *subscribedClass);
  REQUIRE(image.interactionDeclarations[1].subscribedInteractionClasses.front().active);
  REQUIRE(image.objects.empty());
  REQUIRE(image.tsoInteractionMessages.empty());
  REQUIRE(image.tsoAttributeUpdateMessages.empty());
  REQUIRE(image.tsoObjectDeletionMessages.empty());
  REQUIRE(image.tsoDirectedInteractionMessages.empty());
  REQUIRE(image.tsoRequestRetractionRecords.empty());
  REQUIRE(image.tsoQueueEntries.empty());

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedPublisher = restarted.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto restartedSubscriber = restarted.join(
      L"exercise", L"subscriber", L"subscriber", noOpCallbackRoute());
  REQUIRE(restartedPublisher.status == FederationRegistryStatus::applied);
  REQUIRE(restartedSubscriber.status == FederationRegistryStatus::applied);
  REQUIRE(restartedPublisher.membership);
  REQUIRE(restartedSubscriber.membership);
  REQUIRE(restartedPublisher.membership->id == publisherId);
  REQUIRE(restartedSubscriber.membership->id == subscriberId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", publisherId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", publisherId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(
      L"exercise", subscriberId);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.interactionTransportationTypeChangeWorkItems.empty());

  auto restoredPublisherDeclaration = restarted.interactionClassDeclarationFor(
      L"exercise", publisherId, *publishedClass);
  REQUIRE(restoredPublisherDeclaration.has_value());
  REQUIRE(restoredPublisherDeclaration->published);
  REQUIRE_FALSE(restoredPublisherDeclaration->subscriptionActive.has_value());
  auto restoredSubscriberDeclaration = restarted.interactionClassDeclarationFor(
      L"exercise", subscriberId, *subscribedClass);
  REQUIRE(restoredSubscriberDeclaration.has_value());
  REQUIRE_FALSE(restoredSubscriberDeclaration->published);
  REQUIRE(restoredSubscriberDeclaration->subscriptionActive.has_value());
  REQUIRE(*restoredSubscriberDeclaration->subscriptionActive);

  auto publisherOtherClass = restarted.interactionClassDeclarationFor(
      L"exercise", publisherId, *subscribedClass);
  auto subscriberOtherClass = restarted.interactionClassDeclarationFor(
      L"exercise", subscriberId, *publishedClass);
  REQUIRE(publisherOtherClass.has_value());
  REQUIRE_FALSE(publisherOtherClass->published);
  REQUIRE_FALSE(publisherOtherClass->subscriptionActive.has_value());
  REQUIRE(subscriberOtherClass.has_value());
  REQUIRE_FALSE(subscriberOtherClass->published);
  REQUIRE_FALSE(subscriberOtherClass->subscriptionActive.has_value());

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores a mixed interaction override with a subscription for multiple federates",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-interaction-mixed-override][interaction-declaration-state]"
    "[interaction-transportation-type-change][transportation-management]"
    "[interaction-management][declaration-management][multi-federate-callback-ordering]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto publisher = source.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto subscriber = source.join(
      L"exercise", L"subscriber", L"subscriber", noOpCallbackRoute());
  REQUIRE(publisher.status == FederationRegistryStatus::applied);
  REQUIRE(subscriber.status == FederationRegistryStatus::applied);
  REQUIRE(publisher.membership);
  REQUIRE(subscriber.membership);
  auto const publisherId = publisher.membership->id;
  auto const subscriberId = subscriber.membership->id;

  auto const publishedClass = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.CustomerTransactions.CustomerSeated");
  auto const subscribedClass = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.ServerAction.TakeOrder");
  REQUIRE(publishedClass.has_value());
  REQUIRE(subscribedClass.has_value());
  REQUIRE(source.setInteractionClassPublication(
      L"exercise", publisherId, *publishedClass, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);
  REQUIRE(source.setInteractionClassSubscription(
      L"exercise", subscriberId, *subscribedClass, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);
  REQUIRE(source.planInteractionTransportationTypeChange(
      L"exercise", publisherId, *publishedClass, "HLAbestEffort").status ==
      umbra::detail::InteractionTransportationTypeChangeStatus::applied);
  auto committedChange = source.beginInteractionTransportationTypeChange(
      L"exercise", publisherId, *publishedClass);
  REQUIRE(committedChange.has_value());
  REQUIRE(*committedChange == "HLAbestEffort");

  std::wstring const saveLabel = L"interaction-mixed-override-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", publisherId, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", publisherId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", subscriberId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", publisherId).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", subscriberId).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.interactionDeclarationCount == 2U);
  REQUIRE(image.interactionDeclarations.size() == 2U);
  REQUIRE(image.interactionDeclarations[0].federateId == publisherId);
  REQUIRE(image.interactionDeclarations[1].federateId == subscriberId);
  REQUIRE(image.interactionDeclarations[0].publishedInteractionClasses ==
      std::vector<std::uint64_t>{*publishedClass});
  REQUIRE(image.interactionDeclarations[0].interactionTransportationTypes.size() == 1U);
  REQUIRE(image.interactionDeclarations[0].interactionTransportationTypes.front().
      interactionClassHandle == *publishedClass);
  REQUIRE(image.interactionDeclarations[0].interactionTransportationTypes.front().value ==
      "HLAbestEffort");
  REQUIRE(image.interactionDeclarations[1].publishedInteractionClasses.empty());
  REQUIRE(image.interactionDeclarations[1].subscribedInteractionClasses.size() == 1U);
  REQUIRE(image.interactionDeclarations[1].subscribedInteractionClasses.front().
      interactionClassHandle == *subscribedClass);
  REQUIRE(image.interactionDeclarations[1].subscribedInteractionClasses.front().active);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedPublisher = restarted.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto restartedSubscriber = restarted.join(
      L"exercise", L"subscriber", L"subscriber", noOpCallbackRoute());
  REQUIRE(restartedPublisher.status == FederationRegistryStatus::applied);
  REQUIRE(restartedSubscriber.status == FederationRegistryStatus::applied);
  REQUIRE(restartedPublisher.membership);
  REQUIRE(restartedSubscriber.membership);
  REQUIRE(restartedPublisher.membership->id == publisherId);
  REQUIRE(restartedSubscriber.membership->id == subscriberId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", publisherId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", publisherId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(
      L"exercise", subscriberId);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.interactionTransportationTypeChangeWorkItems.empty());

  auto restoredQuery = restarted.interactionTransportationTypeQueryFor(
      L"exercise", publisherId, publisherId, *publishedClass);
  REQUIRE(restoredQuery.has_value());
  REQUIRE(restoredQuery->transportationName == "HLAbestEffort");
  auto restoredSubscriberDeclaration = restarted.interactionClassDeclarationFor(
      L"exercise", subscriberId, *subscribedClass);
  REQUIRE(restoredSubscriberDeclaration.has_value());
  REQUIRE_FALSE(restoredSubscriberDeclaration->published);
  REQUIRE(restoredSubscriberDeclaration->subscriptionActive.has_value());
  REQUIRE(*restoredSubscriberDeclaration->subscriptionActive);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE("The embedded federation registry preserves a prevalidated definition", "[unit][kernel][federation-registry][foundation][federation-management]") {
  EmbeddedFederationRegistry registry;

  REQUIRE(registry.create(L"exercise", validDefinition()).status == FederationRegistryStatus::applied);
  REQUIRE(registry.contains(L"exercise"));

  auto definition = registry.definitionFor(L"exercise");
  REQUIRE(definition.has_value());
  REQUIRE(definition->logicalTimeImplementationName == L"HLAinteger64Time");
  REQUIRE(definition->fomModules.size() == 2);
  REQUIRE(definition->fomModules.front().designator == L"file:///fom/base.xml");
  REQUIRE(definition->fomModules.front().schemaDesignator == L"IEEE1516-DIF-2025.xsd");
}

TEST_CASE(
    "The embedded federation registry exposes internal operation timing",
    "[unit][kernel][federation-registry][instrumentation][foundation][federation-management]") {
  auto instrumentation = std::make_shared<umbra::detail::RuntimeInstrumentation>();
  EmbeddedFederationRegistry registry(instrumentation);

  REQUIRE(registry.create(L"instrumented", validDefinition()).status ==
      FederationRegistryStatus::applied);

  auto const snapshot = registry.runtimeInstrumentationSnapshotForTesting();
  auto const found = std::find_if(
      snapshot.operations.begin(),
      snapshot.operations.end(),
      [](umbra::detail::InstrumentationOperationSnapshot const& operation) {
        return operation.layer == InstrumentationLayer::federation_registry &&
            operation.name == "create";
      });
  REQUIRE(found != snapshot.operations.end());
  REQUIRE(found->calls == 1);
  REQUIRE(found->totalDurationNanoseconds > 0);
}

TEST_CASE("The embedded federation registry rejects missing definitions without imposing name policy", "[unit][kernel][federation-registry][foundation][federation-management]") {
  EmbeddedFederationRegistry registry;
  FederationDefinition noModules;
  FederationDefinition repeatedDesignators{
      {
          module(L"file:///fom/base.xml", L"C:/fom/base.xml"),
          module(L"file:///fom/base.xml", L"C:/fom/base.xml"),
      },
      L"",
  };

  REQUIRE(registry.create(L"", validDefinition()).status == FederationRegistryStatus::applied);
  REQUIRE(registry.create(L"no-modules", noModules).status == FederationRegistryStatus::invalid_request);
  REQUIRE(
      registry.create(L"repeated-module-designators", repeatedDesignators).status ==
      FederationRegistryStatus::applied);
  REQUIRE(registry.create(L"exercise", validDefinition()).status == FederationRegistryStatus::applied);
  REQUIRE(
      registry.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::federation_already_exists);

  auto explicitEmptyName = registry.join(L"exercise", L"", L"");
  REQUIRE(explicitEmptyName.status == FederationRegistryStatus::applied);
  REQUIRE(explicitEmptyName.membership.has_value());
  REQUIRE(explicitEmptyName.membership->name.empty());
  REQUIRE(explicitEmptyName.membership->type.empty());
}

TEST_CASE("The embedded federation registry maintains active membership and destroy invariants", "[unit][kernel][federation-registry][foundation][federation-management]") {
  EmbeddedFederationRegistry registry;
  REQUIRE(registry.create(L"exercise", validDefinition()).status == FederationRegistryStatus::applied);

  auto named = registry.join(L"exercise", L"trainer", L"alice");
  REQUIRE(named.status == FederationRegistryStatus::applied);
  REQUIRE(named.membership.has_value());
  REQUIRE(named.membership->name == L"alice");
  REQUIRE(named.membership->id != 0);

  auto generated = registry.join(L"exercise", L"observer");
  REQUIRE(generated.status == FederationRegistryStatus::applied);
  REQUIRE(generated.membership.has_value());
  REQUIRE(generated.membership->name == L"federate-2");
  REQUIRE(generated.membership->id != named.membership->id);
  REQUIRE(registry.memberCount(L"exercise") == 2);

  auto namedByName = registry.memberByName(L"exercise", L"alice");
  REQUIRE(namedByName.has_value());
  REQUIRE(namedByName->id == named.membership->id);
  auto generatedById = registry.memberById(L"exercise", generated.membership->id);
  REQUIRE(generatedById.has_value());
  REQUIRE(generatedById->name == generated.membership->name);
  REQUIRE_FALSE(registry.memberByName(L"exercise", L"missing").has_value());
  REQUIRE_FALSE(registry.memberById(L"missing", named.membership->id).has_value());

  REQUIRE(
      registry.join(L"exercise", L"trainer", L"alice").status ==
      FederationRegistryStatus::federate_name_already_in_use);
  REQUIRE(
      registry.destroy(L"exercise").status == FederationRegistryStatus::federates_currently_joined);

  REQUIRE(
      registry.resign(L"exercise", named.membership->id).status == FederationRegistryStatus::applied);
  REQUIRE_FALSE(registry.memberByName(L"exercise", L"alice").has_value());
  REQUIRE_FALSE(registry.memberById(L"exercise", named.membership->id).has_value());
  REQUIRE(
      registry.resign(L"exercise", generated.membership->id).status == FederationRegistryStatus::applied);
  REQUIRE(registry.memberCount(L"exercise") == 0);
  REQUIRE(registry.destroy(L"exercise").status == FederationRegistryStatus::applied);
  REQUIRE_FALSE(registry.contains(L"exercise"));
}

TEST_CASE(
    "The registry retains captured regional attribute TSO snapshots instead of rereading live regions",
    "[unit][kernel][federation-registry][tso][ddm][timestamped-regional-attribute-update]") {
  EmbeddedFederationRegistry registry;
  REQUIRE(registry.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto producer = registry.join(L"exercise", L"producer", L"producer");
  auto receiver = registry.join(L"exercise", L"receiver", L"receiver");
  REQUIRE(producer.membership.has_value());
  REQUIRE(receiver.membership.has_value());

  // The opaque source handle is deliberately absent from the live registry.
  // A timestamped service has already accepted its committed specification,
  // so queue admission must retain the supplied invocation snapshot rather
  // than attempting a second lookup against mutable federation state.
  umbra::detail::TsoAttributeUpdateMessage message;
  message.producingFederateId = producer.membership->id;
  message.objectInstanceHandle = 91;
  message.attributes.emplace_back(1, rti1516_2025::VariableLengthData{});
  message.timestamp = std::make_shared<rti1516_2025::HLAinteger64Time>(7);

  umbra::detail::TsoAttributeUpdatePassel passel;
  passel.sentAttributeHandles = {1};
  passel.sentRegionHandles = {42};
  passel.sentRegionSnapshots.emplace(
      42,
      umbra::detail::RegionSpecificationSnapshot{
          {17},
          {{17, umbra::detail::RegionRangeBounds{0, 5}}},
          true});
  message.sentRegionSnapshots = passel.sentRegionSnapshots;
  message.passelsByRecipient.emplace(
      receiver.membership->id,
      std::vector<umbra::detail::TsoAttributeUpdatePassel>{passel});

  auto const enqueued = registry.enqueueTsoAttributeUpdate(
      L"exercise",
      std::move(message),
      {receiver.membership->id},
      {receiver.membership->id});
  REQUIRE(enqueued.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(enqueued.queueStatus == umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(enqueued.messageId != 0);

  auto const delivery = registry.beginTsoPayloadDelivery(
      L"exercise",
      receiver.membership->id,
      rti1516_2025::HLAinteger64Time(7),
      true);
  REQUIRE(delivery.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(delivery.deliveryStatus == umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(delivery.deliveries.size() == 1);
  auto const* attributeDelivery =
      std::get_if<umbra::detail::TsoAttributeUpdateDelivery>(&delivery.deliveries.front());
  REQUIRE(attributeDelivery != nullptr);
  REQUIRE(attributeDelivery->message.sentRegionSnapshots.size() == 1);
  REQUIRE(attributeDelivery->message.sentRegionSnapshots.contains(42));
  auto const& snapshot = attributeDelivery->message.sentRegionSnapshots.at(42);
  REQUIRE(snapshot.dimensionHandles == std::set<std::uint64_t>{17});
  REQUIRE(snapshot.committedRangeBounds.at(17).lowerBound == 0);
  REQUIRE(snapshot.committedRangeBounds.at(17).upperBound == 5);
}

TEST_CASE("The embedded federation registry makes generated names unique despite user lookalikes", "[unit][kernel][federation-registry][foundation][federation-management]") {
  EmbeddedFederationRegistry registry;
  REQUIRE(registry.create(L"exercise", validDefinition()).status == FederationRegistryStatus::applied);

  auto lookalike = registry.join(L"exercise", L"trainer", L"federate-1");
  REQUIRE(lookalike.status == FederationRegistryStatus::applied);
  auto generated = registry.join(L"exercise", L"observer");
  REQUIRE(generated.status == FederationRegistryStatus::applied);
  REQUIRE(generated.membership.has_value());
  REQUIRE(generated.membership->name == L"federate-2");
  REQUIRE(generated.membership->id == 2);
}

TEST_CASE("The embedded federation registry commits an additional-module definition with membership", "[unit][kernel][federation-registry][foundation][federation-management]") {
  EmbeddedFederationRegistry registry;
  auto original = validDefinition();
  auto replacement = validDefinition();
  replacement.logicalTimeImplementationName = L"HLAfloat64Time";

  REQUIRE(registry.create(L"exercise", original).status == FederationRegistryStatus::applied);
  auto joined = registry.joinWithDefinition(L"exercise", replacement, L"observer", L"bob");
  REQUIRE(joined.status == FederationRegistryStatus::applied);
  REQUIRE(joined.membership.has_value());
  REQUIRE(registry.memberCount(L"exercise") == 1);
  auto afterJoin = registry.definitionFor(L"exercise");
  REQUIRE(afterJoin.has_value());
  REQUIRE(afterJoin->logicalTimeImplementationName == L"HLAfloat64Time");

  auto incompatibleReplacement = validDefinition();
  incompatibleReplacement.logicalTimeImplementationName = L"HLAinteger64Time";
  auto rejected = registry.joinWithDefinition(
      L"exercise",
      incompatibleReplacement,
      L"observer",
      L"carol");
  REQUIRE(rejected.status == FederationRegistryStatus::invalid_request);
  REQUIRE(registry.memberCount(L"exercise") == 1);
  auto afterRejectedJoin = registry.definitionFor(L"exercise");
  REQUIRE(afterRejectedJoin.has_value());
  REQUIRE(afterRejectedJoin->logicalTimeImplementationName == L"HLAfloat64Time");

  auto duplicate = registry.joinWithDefinition(
      L"exercise",
      replacement,
      L"observer",
      L"bob");
  REQUIRE(duplicate.status == FederationRegistryStatus::federate_name_already_in_use);
}

TEST_CASE("The embedded federation registry reports missing federation and membership distinctly", "[unit][kernel][federation-registry][foundation][federation-management]") {
  EmbeddedFederationRegistry registry;

  REQUIRE(
      registry.join(L"missing", L"trainer").status ==
      FederationRegistryStatus::federation_does_not_exist);
  REQUIRE(
      registry.destroy(L"missing").status == FederationRegistryStatus::federation_does_not_exist);

  REQUIRE(registry.create(L"exercise", validDefinition()).status == FederationRegistryStatus::applied);
  REQUIRE(
      registry.resign(L"exercise", 42).status == FederationRegistryStatus::federate_not_member);
}

TEST_CASE(
    "The registry commits runtime time state with its federate membership",
    "[unit][kernel][federation-registry][time-management]") {
  EmbeddedFederationRegistry registry;
  REQUIRE(registry.create(L"exercise", validDefinition()).status == FederationRegistryStatus::applied);

  auto timeState = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  auto joined = registry.joinWithTimeState(L"exercise", timeState, L"trainer", L"alice");
  REQUIRE(joined.status == FederationRegistryStatus::applied);
  REQUIRE(joined.membership.has_value());

  auto advance = timeState->requestAdvance(std::make_shared<rti1516_2025::HLAinteger64Time>(3));
  REQUIRE(advance.generation != 0);
  timeState.reset();

  auto snapshot = registry.timeSnapshotFor(L"exercise");
  REQUIRE(snapshot.has_value());
  REQUIRE(snapshot->definition.logicalTimeImplementationName == L"HLAinteger64Time");
  REQUIRE(snapshot->federates.size() == 1);
  REQUIRE(snapshot->federates.front().membership.id == joined.membership->id);
  REQUIRE(snapshot->federates.front().membership.name == L"alice");
  REQUIRE(snapshot->federates.front().time.timeAdvancePending);
  REQUIRE(snapshot->federates.front().time.requestedTime);

  REQUIRE(
      registry.resign(L"exercise", joined.membership->id).status ==
      FederationRegistryStatus::applied);
  snapshot = registry.timeSnapshotFor(L"exercise");
  REQUIRE(snapshot.has_value());
  REQUIRE(snapshot->federates.empty());
}

TEST_CASE(
    "The registry projects private TSO coordination into its time snapshot",
    "[unit][kernel][federation-registry][time-management][tso][lits]") {
  EmbeddedFederationRegistry registry;
  REQUIRE(registry.create(L"exercise", validDefinition()).status == FederationRegistryStatus::applied);

  auto receiverTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  auto observerTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  auto receiver = registry.joinWithTimeState(L"exercise", receiverTime, L"receiver", L"receiver");
  auto observer = registry.joinWithTimeState(L"exercise", observerTime, L"observer", L"observer");
  REQUIRE(receiver.status == FederationRegistryStatus::applied);
  REQUIRE(observer.status == FederationRegistryStatus::applied);
  REQUIRE(receiver.membership);
  REQUIRE(observer.membership);

  auto const messageId = registry.allocateTsoMessageId(L"exercise");
  REQUIRE(messageId.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(messageId.messageId != 0);
  auto const timestamp = std::make_shared<rti1516_2025::HLAinteger64Time>(5);
  REQUIRE(
      registry.enqueueTsoMessage(
          L"exercise",
          messageId.messageId,
          receiver.membership->id,
          timestamp)
          .queueStatus == umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(
      registry.enqueueTsoMessage(
          L"exercise",
          messageId.messageId,
          observer.membership->id,
          timestamp)
          .queueStatus == umbra::detail::TsoMessageQueueStatus::applied);

  auto snapshot = registry.timeSnapshotFor(L"exercise");
  REQUIRE(snapshot.has_value());
  REQUIRE(snapshot->federates.size() == 2);
  REQUIRE(snapshot->federates[0].queuedTsoMessages.size() == 1);
  REQUIRE(snapshot->federates[0].inTransitTsoMessages.empty());
  REQUIRE(snapshot->federates[0].deliveredTsoMessagesSinceLastAdvance.empty());

  auto delivery = registry.beginTsoDelivery(
      L"exercise",
      receiver.membership->id,
      rti1516_2025::HLAinteger64Time(5),
      true);
  REQUIRE(delivery.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(delivery.delivery.status == umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(delivery.delivery.messages.size() == 1);

  snapshot = registry.timeSnapshotFor(L"exercise");
  REQUIRE(snapshot.has_value());
  REQUIRE(snapshot->federates[0].queuedTsoMessages.empty());
  REQUIRE(snapshot->federates[0].inTransitTsoMessages.size() == 1);
  REQUIRE(snapshot->federates[1].queuedTsoMessages.size() == 1);

  auto completed = registry.completeTsoDelivery(L"exercise", delivery.delivery.messages.front());
  REQUIRE(completed.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(completed.delivery.status == umbra::detail::FederationTsoDeliveryStatus::applied);
  snapshot = registry.timeSnapshotFor(L"exercise");
  REQUIRE(snapshot.has_value());
  REQUIRE(snapshot->federates[0].inTransitTsoMessages.empty());
  REQUIRE(snapshot->federates[0].deliveredTsoMessagesSinceLastAdvance.size() == 1);

  auto const retraction = registry.retractTsoMessage(L"exercise", messageId.messageId);
  REQUIRE(retraction.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(retraction.queueResult.status == umbra::detail::TsoMessageQueueStatus::message_already_delivered);

  REQUIRE(
      registry.resign(L"exercise", receiver.membership->id).status ==
      FederationRegistryStatus::applied);
  snapshot = registry.timeSnapshotFor(L"exercise");
  REQUIRE(snapshot.has_value());
  REQUIRE(snapshot->federates.size() == 1);
  REQUIRE(snapshot->federates.front().membership.id == observer.membership->id);
  REQUIRE(snapshot->federates.front().queuedTsoMessages.size() == 1);
}

TEST_CASE(
    "The TSO recipient ledger makes a suppressed callback terminal",
    "[unit][kernel][federation-registry][time-management][tso][request-retraction]") {
  EmbeddedFederationRegistry registry;
  REQUIRE(registry.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);

  auto producer = registry.join(L"exercise", L"producer", L"producer");
  umbra::detail::InteractionCallbackRoute receiverRoute;
  receiverRoute.submit = [](umbra::detail::FederateCallbackInvocation) {};
  auto receiver = registry.join(
      L"exercise",
      L"receiver",
      L"receiver",
      std::move(receiverRoute));
  REQUIRE(producer.membership);
  REQUIRE(receiver.membership);

  umbra::detail::TsoInteractionMessage message;
  message.producingFederateId = producer.membership->id;
  message.sentInteractionClassHandle = 1;
  message.timestamp = std::make_shared<rti1516_2025::HLAinteger64Time>(7);
  auto const enqueued = registry.enqueueTsoInteraction(
      L"exercise",
      std::move(message),
      {receiver.membership->id},
      {receiver.membership->id});
  REQUIRE(enqueued.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(enqueued.queueStatus == umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(enqueued.messageId != 0);

  // A callback can be invalidated after TSO admission (for example, by a
  // late projection failure or a malformed private payload).  The delivery
  // path must close that recipient as suppressed rather than leave it
  // pending or report a Request Retraction for a callback that never began.
  REQUIRE(registry.finishTsoRecipientCallbackSuppressed(
      L"exercise",
      receiver.membership->id,
      enqueued.messageId));
  REQUIRE_FALSE(registry.beginTsoInteractionCallback(
      L"exercise",
      receiver.membership->id,
      enqueued.messageId));

  auto const retracted = registry.retractTsoMessageForProducer(
      L"exercise",
      producer.membership->id,
      enqueued.messageId,
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  REQUIRE(retracted.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(retracted.queueResult.status == umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(retracted.requestRetractionNotifications.empty());
  REQUIRE_FALSE(registry.canDeliverTsoRequestRetraction(
      L"exercise",
      receiver.membership->id,
      enqueued.messageId));
}

TEST_CASE(
    "The registry schedules a newly eligible constrained TAR after a regulator advances",
    "[unit][kernel][federation-registry][time-management][galt][time-advance-grant]") {
  EmbeddedFederationRegistry registry;
  REQUIRE(registry.create(L"exercise", validDefinition()).status == FederationRegistryStatus::applied);

  auto receiverTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  auto regulatorTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  auto receiver = registry.joinWithTimeState(L"exercise", receiverTime, L"receiver", L"receiver");
  auto regulator = registry.joinWithTimeState(L"exercise", regulatorTime, L"regulator", L"regulator");
  REQUIRE(receiver.membership);
  REQUIRE(regulator.membership);

  auto constrained = receiverTime->requestTimeConstrained();
  REQUIRE(constrained.generation != 0);
  REQUIRE(receiverTime->grantTimeConstrained(constrained.generation));
  auto regulation = regulatorTime->requestTimeRegulation(
      std::make_shared<rti1516_2025::HLAinteger64Interval>(2));
  REQUIRE(regulation.generation != 0);
  REQUIRE(regulatorTime->grantTimeRegulation(regulation.generation));

  auto receiverAdvance = receiverTime->requestAdvance(
      std::make_shared<rti1516_2025::HLAinteger64Time>(2));
  REQUIRE(receiverAdvance.generation != 0);
  std::size_t receiverDispatches = 0;
  auto waiting = registry.requestTimeAdvanceGrant(
      L"exercise",
      receiver.membership->id,
      receiverAdvance.generation,
      [&receiverDispatches] { ++receiverDispatches; });
  REQUIRE(waiting.status == FederationTimeGrantStatus::applied);
  REQUIRE(waiting.dispatches.empty());

  // The regulator's pending target 1 plus lookahead 2 raises the receiver's
  // GALT from 2 to 3, making the receiver's TAR(2) strictly below the bound
  // even before the regulator receives its own grant.
  auto regulatorAdvance = regulatorTime->requestAdvance(
      std::make_shared<rti1516_2025::HLAinteger64Time>(1));
  REQUIRE(regulatorAdvance.generation != 0);
  std::size_t regulatorDispatches = 0;
  auto eligible = registry.requestTimeAdvanceGrant(
      L"exercise",
      regulator.membership->id,
      regulatorAdvance.generation,
      [&regulatorDispatches] { ++regulatorDispatches; });
  REQUIRE(eligible.status == FederationTimeGrantStatus::applied);
  REQUIRE(eligible.dispatches.size() == 2);
  for (auto& dispatch : eligible.dispatches) {
    dispatch();
  }
  REQUIRE(receiverDispatches == 1);
  REQUIRE(regulatorDispatches == 1);

  REQUIRE(
      registry.beginTimeAdvanceGrant(
          L"exercise",
          receiver.membership->id,
          receiverAdvance.generation) == FederationTimeGrantStatus::applied);
  REQUIRE(receiverTime->grant(receiverAdvance.generation));
  REQUIRE(
      registry.beginTimeAdvanceGrant(
          L"exercise",
          regulator.membership->id,
          regulatorAdvance.generation) == FederationTimeGrantStatus::applied);
  REQUIRE(regulatorTime->grant(regulatorAdvance.generation));

  auto receiverSnapshot = receiverTime->snapshot();
  auto regulatorSnapshot = regulatorTime->snapshot();
  REQUIRE_FALSE(receiverSnapshot.timeAdvancePending);
  REQUIRE_FALSE(regulatorSnapshot.timeAdvancePending);
}

TEST_CASE(
    "Filesystem state image restores a directed interaction publication and subscription for multiple federates",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-directed-interaction-declaration][interaction-declaration-state]"
    "[directed-interaction][directed-declaration][declaration-management][multi-federate-callback-ordering]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedDirectedInteractionDefinition()).status ==
      FederationRegistryStatus::applied);
  auto publisher = source.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto subscriber = source.join(
      L"exercise", L"subscriber", L"subscriber", noOpCallbackRoute());
  REQUIRE(publisher.status == FederationRegistryStatus::applied);
  REQUIRE(subscriber.status == FederationRegistryStatus::applied);
  REQUIRE(publisher.membership);
  REQUIRE(subscriber.membership);
  auto const publisherId = publisher.membership->id;
  auto const subscriberId = subscriber.membership->id;

  auto const objectClass = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject");
  auto const interactionClass = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.UmbraDirectedFixtureInteraction");
  REQUIRE(objectClass.has_value());
  REQUIRE(interactionClass.has_value());
  std::set<std::uint64_t> directedClasses{*interactionClass};
  REQUIRE(source.publishObjectClassDirectedInteractions(
      L"exercise", publisherId, *objectClass, directedClasses) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);
  REQUIRE(source.subscribeObjectClassDirectedInteractions(
      L"exercise", subscriberId, *objectClass, directedClasses, true) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);

  std::wstring const saveLabel = L"directed-interaction-declaration-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", publisherId, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", publisherId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", subscriberId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", publisherId).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", subscriberId).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.interactionDeclarationCount == 2U);
  REQUIRE(image.interactionDeclarations.size() == 2U);
  REQUIRE(image.interactionDeclarations[0].federateId == publisherId);
  REQUIRE(image.interactionDeclarations[1].federateId == subscriberId);
  auto const& savedPublisher = image.interactionDeclarations[0];
  auto const& savedSubscriber = image.interactionDeclarations[1];
  REQUIRE(savedPublisher.publishedInteractionClasses.empty());
  REQUIRE(savedPublisher.subscribedInteractionClasses.empty());
  REQUIRE(savedPublisher.regionalSubscribedInteractionClasses.empty());
  REQUIRE(savedPublisher.publishedObjectClassDirectedInteractions.size() == 1U);
  REQUIRE(savedPublisher.publishedObjectClassDirectedInteractions.front().objectClassHandle ==
      *objectClass);
  REQUIRE(savedPublisher.publishedObjectClassDirectedInteractions.front().interactionClassHandle ==
      *interactionClass);
  REQUIRE(savedPublisher.subscribedObjectClassDirectedInteractions.empty());
  REQUIRE(savedPublisher.interactionTransportationTypes.empty());
  REQUIRE(savedPublisher.interactionOrderTypes.empty());
  REQUIRE(savedPublisher.pendingInteractionTransportationTypeChanges.empty());
  REQUIRE(savedSubscriber.publishedInteractionClasses.empty());
  REQUIRE(savedSubscriber.subscribedInteractionClasses.empty());
  REQUIRE(savedSubscriber.regionalSubscribedInteractionClasses.empty());
  REQUIRE(savedSubscriber.publishedObjectClassDirectedInteractions.empty());
  REQUIRE(savedSubscriber.subscribedObjectClassDirectedInteractions.size() == 1U);
  REQUIRE(savedSubscriber.subscribedObjectClassDirectedInteractions.front().objectClassHandle ==
      *objectClass);
  REQUIRE(savedSubscriber.subscribedObjectClassDirectedInteractions.front().interactionClassHandle ==
      *interactionClass);
  REQUIRE(savedSubscriber.subscribedObjectClassDirectedInteractions.front().active);
  REQUIRE(savedSubscriber.interactionTransportationTypes.empty());
  REQUIRE(savedSubscriber.interactionOrderTypes.empty());
  REQUIRE(savedSubscriber.pendingInteractionTransportationTypeChanges.empty());
  REQUIRE(image.objects.empty());
  REQUIRE(image.tsoInteractionMessages.empty());
  REQUIRE(image.tsoAttributeUpdateMessages.empty());
  REQUIRE(image.tsoObjectDeletionMessages.empty());
  REQUIRE(image.tsoDirectedInteractionMessages.empty());
  REQUIRE(image.tsoRequestRetractionRecords.empty());
  REQUIRE(image.tsoQueueEntries.empty());

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedDirectedInteractionDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedPublisher = restarted.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto restartedSubscriber = restarted.join(
      L"exercise", L"subscriber", L"subscriber", noOpCallbackRoute());
  REQUIRE(restartedPublisher.status == FederationRegistryStatus::applied);
  REQUIRE(restartedSubscriber.status == FederationRegistryStatus::applied);
  REQUIRE(restartedPublisher.membership);
  REQUIRE(restartedSubscriber.membership);
  REQUIRE(restartedPublisher.membership->id == publisherId);
  REQUIRE(restartedSubscriber.membership->id == subscriberId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", publisherId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", publisherId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(
      L"exercise", subscriberId);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.interactionTransportationTypeChangeWorkItems.empty());

  // The second commit proves the restored runtime maps serialize back to the
  // same directed pair instead of merely accepting the original image.
  std::wstring const roundTripLabel =
      L"directed-interaction-declaration-round-trip";
  REQUIRE(restarted.requestFederationSave(
      L"exercise", publisherId, roundTripLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveBegun(
      L"exercise", publisherId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveBegun(
      L"exercise", subscriberId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(restarted.federateSaveComplete(
      L"exercise", publisherId).saveCompletedSuccessfully);
  REQUIRE(restarted.federateSaveComplete(
      L"exercise", subscriberId).saveCompletedSuccessfully);
  auto roundTrip = store->load(L"exercise", roundTripLabel);
  REQUIRE(roundTrip.has_value());
  auto roundTripImage = umbra::detail::FederationStateImageCodec::decode(
      roundTrip->stateImage);
  REQUIRE(roundTripImage.interactionDeclarations.size() == 2U);
  REQUIRE(roundTripImage.interactionDeclarations[0]
              .publishedObjectClassDirectedInteractions.size() == 1U);
  REQUIRE(roundTripImage.interactionDeclarations[0]
              .publishedObjectClassDirectedInteractions.front()
              .objectClassHandle == *objectClass);
  REQUIRE(roundTripImage.interactionDeclarations[0]
              .publishedObjectClassDirectedInteractions.front()
              .interactionClassHandle == *interactionClass);
  REQUIRE(roundTripImage.interactionDeclarations[1]
              .subscribedObjectClassDirectedInteractions.size() == 1U);
  REQUIRE(roundTripImage.interactionDeclarations[1]
              .subscribedObjectClassDirectedInteractions.front()
              .objectClassHandle == *objectClass);
  REQUIRE(roundTripImage.interactionDeclarations[1]
              .subscribedObjectClassDirectedInteractions.front()
              .interactionClassHandle == *interactionClass);
  REQUIRE(roundTripImage.interactionDeclarations[1]
              .subscribedObjectClassDirectedInteractions.front()
              .active);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores a directed interaction target and receive-order route",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-directed-interaction-routing][interaction-declaration-state]"
    "[directed-interaction][directed-routing][object-visibility-state][object-lifecycle-state]"
    "[object-class-declaration-state][interaction-management][declaration-management]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedDirectedInteractionDefinition()).status ==
      FederationRegistryStatus::applied);
  auto publisher = source.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto subscriber = source.join(
      L"exercise", L"subscriber", L"subscriber", noOpCallbackRoute());
  REQUIRE(publisher.status == FederationRegistryStatus::applied);
  REQUIRE(subscriber.status == FederationRegistryStatus::applied);
  REQUIRE(publisher.membership);
  REQUIRE(subscriber.membership);
  auto const publisherId = publisher.membership->id;
  auto const subscriberId = subscriber.membership->id;

  auto const objectClass = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject");
  auto const interactionClass = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.UmbraDirectedFixtureInteraction");
  auto const marker = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject", "DirectedTargetMarker");
  REQUIRE(objectClass.has_value());
  REQUIRE(interactionClass.has_value());
  REQUIRE(marker.has_value());

  std::set<std::uint64_t> const markerOnly{*marker};
  std::set<std::uint64_t> const directedOnly{*interactionClass};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", publisherId, *objectClass, markerOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", subscriberId, *objectClass, markerOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.publishObjectClassDirectedInteractions(
      L"exercise", publisherId, *objectClass, directedOnly) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);
  REQUIRE(source.subscribeObjectClassDirectedInteractions(
      L"exercise", subscriberId, *objectClass, directedOnly, true) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", publisherId, *objectClass);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(discoveries.front().receivingFederateId == subscriberId);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", subscriberId, registered.objectInstanceHandle).has_value());

  std::string const markerValue{"directed-target", 15U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *marker,
      rti1516_2025::VariableLengthData(markerValue.data(), markerValue.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise", publisherId, registered.objectInstanceHandle, *objectClass,
      {"HLAreliable"}, &values) == FederationRegistryStatus::applied);

  std::wstring const saveLabel = L"directed-interaction-routing-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", publisherId, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", publisherId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", subscriberId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", publisherId).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", subscriberId).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  REQUIRE(image.objects.front().handle == registered.objectInstanceHandle);
  REQUIRE(image.objects.front().knownObjectClassHandlesByFederate.size() == 2U);
  REQUIRE(image.objects.front().attributeValuesPresent);
  REQUIRE(image.interactionDeclarations.size() == 2U);
  REQUIRE(image.interactionDeclarations[0]
              .publishedObjectClassDirectedInteractions.size() == 1U);
  REQUIRE(image.interactionDeclarations[1]
              .subscribedObjectClassDirectedInteractions.size() == 1U);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedDirectedInteractionDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedPublisher = restarted.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto restartedSubscriber = restarted.join(
      L"exercise", L"subscriber", L"subscriber", noOpCallbackRoute());
  REQUIRE(restartedPublisher.status == FederationRegistryStatus::applied);
  REQUIRE(restartedSubscriber.status == FederationRegistryStatus::applied);
  REQUIRE(restartedPublisher.membership);
  REQUIRE(restartedSubscriber.membership);
  REQUIRE(restartedPublisher.membership->id == publisherId);
  REQUIRE(restartedSubscriber.membership->id == subscriberId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", publisherId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", publisherId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(
      L"exercise", subscriberId);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.interactionTransportationTypeChangeWorkItems.empty());

  auto restoredPlan = restarted.planReceiveOrderDirectedInteraction(
      L"exercise", publisherId, registered.objectInstanceHandle,
      *interactionClass, {});
  REQUIRE(restoredPlan.status ==
      umbra::detail::ReceiveOrderDirectedInteractionStatus::applied);
  REQUIRE(restoredPlan.transportationName == "HLAreliable");
  REQUIRE(restoredPlan.recipients.size() == 1U);
  REQUIRE(restoredPlan.recipients.front().federateId == subscriberId);
  REQUIRE(restoredPlan.recipients.front().objectInstanceHandle ==
      registered.objectInstanceHandle);
  REQUIRE(restoredPlan.recipients.front().receivedInteractionClassHandle ==
      *interactionClass);
  REQUIRE(restoredPlan.recipients.front().callbackRoute);

  std::wstring const roundTripLabel =
      L"directed-interaction-routing-round-trip";
  REQUIRE(restarted.requestFederationSave(
      L"exercise", publisherId, roundTripLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveBegun(
      L"exercise", publisherId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveBegun(
      L"exercise", subscriberId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(restarted.federateSaveComplete(
      L"exercise", publisherId).saveCompletedSuccessfully);
  REQUIRE(restarted.federateSaveComplete(
      L"exercise", subscriberId).saveCompletedSuccessfully);
  auto roundTrip = store->load(L"exercise", roundTripLabel);
  REQUIRE(roundTrip.has_value());
  auto roundTripImage = umbra::detail::FederationStateImageCodec::decode(
      roundTrip->stateImage);
  REQUIRE(roundTripImage.objects.size() == 1U);
  REQUIRE(roundTripImage.objects.front().handle == registered.objectInstanceHandle);
  REQUIRE(roundTripImage.objects.front().attributeValuesPresent);
  REQUIRE(roundTripImage.interactionDeclarations.size() == 2U);
  REQUIRE(roundTripImage.interactionDeclarations[0]
              .publishedObjectClassDirectedInteractions.size() == 1U);
  REQUIRE(roundTripImage.interactionDeclarations[1]
              .subscribedObjectClassDirectedInteractions.size() == 1U);
  REQUIRE(roundTripImage.interactionDeclarations[1]
              .subscribedObjectClassDirectedInteractions.front().active);

  std::filesystem::remove_all(directory, ignored);
}
