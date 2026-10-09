#include "internal/runtime/umbra_rti_ambassador.hpp"
#include "internal/runtime/umbra_rti_ambassador_service_failure_translation.hpp"
#include "internal/runtime/ambassador_string_helpers.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/ambassador_federation_lifecycle_support.hpp"

#include "internal/encoding/byte_order.hpp"
#include "internal/encoding/variable_length_data_2025.hpp"
#include "internal/observability/service_report_store.hpp"
#include "internal/runtime/rti_initialization_data.hpp"

#include <RTI/auth/AuthorizationResult.h>
#include <RTI/auth/Authorizer.h>
#include <RTI/auth/AuthorizerFactory.h>
#include <RTI/auth/Credentials.h>
#include <RTI/auth/HLAnoCredentials.h>

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/handles/attribute_handle.hpp"
#include "internal/handles/dimension_handle.hpp"
#include "internal/handles/federate_handle.hpp"
#include "internal/time/federate_time_state.hpp"
#include "internal/federation/federation_management_coordinator.hpp"
#include "internal/federation/federation_registry.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/time/federation_time_bounds.hpp"
#include "internal/time/federation_time_grant_policy.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/fom/fom_catalog.hpp"
#include "internal/fom/libxml2_fom_composer.hpp"
#include "internal/fom/libxml2_fom_validator.hpp"
#include "internal/handles/message_retraction_handle.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#include "internal/handles/object_class_handle.hpp"
#include "internal/handles/object_instance_handle.hpp"
#include "internal/handles/parameter_handle.hpp"
#include "internal/handles/region_handle.hpp"
#include "internal/time/reference_time_selection.hpp"
#include "internal/handles/transportation_type_handle.hpp"
#include "internal/runtime/utf8_string.hpp"
#include "internal/time/update_rate_gate.hpp"

#include <RTI/FederateAmbassador.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAvariableArray.h>
#include <RTI/time/HLAlogicalTimeFactoryFactory.h>
#endif

#include <algorithm>
#include <atomic>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <initializer_list>
#include <limits>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {
using namespace service_failure_translation;
namespace {

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
umbra::detail::UpdateRateGate& updateRateGate() {
  static umbra::detail::UpdateRateGate gate;
  return gate;
}

std::optional<std::string> updateRateFederationPrefix(
    std::wstring const& federationName) {
  auto const encodedFederationName = umbra::detail::utf8FromWide(federationName);
  if (!encodedFederationName) {
    return std::nullopt;
  }

  // A length-delimited namespace keeps teardown exact even when two valid
  // federation names share a textual slash-delimited prefix (for example,
  // "rate" and "rate/child").  The gate is process-local, so this opaque
  // representation is an internal key rather than a standards-facing value.
  return std::to_string(encodedFederationName->size()) + ":" +
      *encodedFederationName + "/";
}

std::optional<std::string> updateRateAdmissionKey(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t subscriptionGeneration,
    std::uint64_t attributeHandle) {
  auto prefix = updateRateFederationPrefix(federationName);
  if (!prefix) {
    return std::nullopt;
  }
  *prefix += std::to_string(receivingFederateId) + "/" +
      std::to_string(objectInstanceHandle) + "/" +
      std::to_string(subscriptionGeneration) + "/" +
      std::to_string(attributeHandle);
  return prefix;
}

void eraseUpdateRateHistoryForFederate(
    std::wstring const& federationName,
    std::uint64_t federateId) {
  // Delivery admission keys are scoped to the federation execution and
  // receiving federate.  Remove only the departed federate's history: other
  // live federates may still be receiving the same object/attribute stream,
  // and a later join must not inherit this lifetime's wall-clock state.
  auto prefix = updateRateFederationPrefix(federationName);
  if (!prefix) {
    return;
  }
  *prefix += std::to_string(federateId) + "/";
  updateRateGate().erasePrefix(*prefix);
}
#endif

std::wstring momExceptionDescription(Exception const& exception) {
  auto description = exception.name();
  auto const detail = exception.what();
  if (!detail.empty()) {
    description += L": ";
    description += detail;
  }
  return description;
}

void authorizeFederationOperation(
    rti1516_2025::Authorizer& authorizer,
    Credentials const& credentials,
    std::wstring const& federationName,
    std::wstring_view operationName) {
  auto const result =
      authorizer.authorizeFederationOperation(credentials, federationName);
  using Code = rti1516_2025::AuthorizationResult::Code;
  switch (result.getCode()) {
    case Code::AUTHORIZED:
      return;
    case Code::UNAUTHORIZED:
      throw Unauthorized(
          result.getMessage().empty()
              ? std::wstring(operationName) + L" is unauthorized."
              : result.getMessage());
    case Code::INVALID_CREDENTIALS:
      // The official federation-management APIs declare Unauthorized, but
      // not InvalidCredentials, for a federation-operation denial.
      throw Unauthorized(
          result.getMessage().empty()
              ? L"The connected federate is not authorized to invoke " +
                    std::wstring(operationName) + L"."
              : result.getMessage());
    case Code::AUTHORIZATION_ERROR:
      throw RTIinternalError(
          result.getMessage().empty()
              ? L"The configured authorizer could not evaluate " +
                    std::wstring(operationName) + L"."
              : result.getMessage());
  }
  throw RTIinternalError(
      L"The configured authorizer returned an unknown federation-operation result.");
}

void authorizeFederateOperation(
    rti1516_2025::Authorizer& authorizer,
    Credentials const& credentials,
    std::wstring const& federationName,
    std::wstring const& federateName,
    std::wstring const& federateType) {
  auto const result = authorizer.authorizeFederateOperation(
      credentials, federationName, federateName, federateType);
  using Code = rti1516_2025::AuthorizationResult::Code;
  switch (result.getCode()) {
    case Code::AUTHORIZED:
      return;
    case Code::UNAUTHORIZED:
    case Code::INVALID_CREDENTIALS:
      // The official Join APIs declare Unauthorized, but not
      // InvalidCredentials, for a federate-operation denial.
      throw Unauthorized(
          result.getMessage().empty()
              ? L"The connected federate is not authorized to join the federation execution."
              : result.getMessage());
    case Code::AUTHORIZATION_ERROR:
      throw RTIinternalError(
          result.getMessage().empty()
              ? L"The configured authorizer could not evaluate Join Federation Execution."
              : result.getMessage());
  }
  throw RTIinternalError(
      L"The configured authorizer returned an unknown federate-operation result.");
}

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)

#ifndef UMBRA_EMBEDDED_FOM_RESOURCE_DIRECTORY
#error "The embedded federation-management profile requires the vendored 1516.2 resource directory."
#endif

std::filesystem::path embeddedResourceDirectory() {
  // Development builds use the checked-in resource tree. Installed builds
  // use the same reviewed 1516.2 payload under CMAKE_INSTALL_FULL_DATADIR.
  // Prefer the source tree when it exists so the existing development profile
  // remains hermetic, then fall back to the packaged location. This lookup is
  // internal resource plumbing; caller-supplied FOM modules still follow the
  // standard createFederationExecution inputs.
  std::error_code error;
  auto hasRequiredResources = [&error](std::filesystem::path const& root) {
    error.clear();
    return std::filesystem::is_regular_file(
               root / "schemas" / "IEEE1516-FDD-2025.xsd", error) &&
        !error &&
        std::filesystem::is_regular_file(
            root / "schemas" / "IEEE1516-DIF-2025.xsd", error) &&
        !error &&
        std::filesystem::is_regular_file(
            root / "mim" / umbra::detail::hla::utf8::mom::standard_mim_file_name,
            error) &&
        !error;
  };

  auto const sourceTree = std::filesystem::path(UMBRA_EMBEDDED_FOM_RESOURCE_DIRECTORY);
  if (hasRequiredResources(sourceTree)) {
    return sourceTree;
  }

#ifdef UMBRA_EMBEDDED_FOM_RESOURCE_DIRECTORY_INSTALL
  auto const installedTree =
      std::filesystem::path(UMBRA_EMBEDDED_FOM_RESOURCE_DIRECTORY_INSTALL);
  if (hasRequiredResources(installedTree)) {
    return installedTree;
  }
#endif

  throw RTIinternalError(
      L"Umbra could not locate the installed IEEE 1516.2-2025 FOM resource payload.");
}

std::optional<umbra::detail::FomEditionResources> embedded2010Resources() {
#ifdef UMBRA_EMBEDDED_FOM_2010_RESOURCE_DIRECTORY
  auto const root = std::filesystem::path(UMBRA_EMBEDDED_FOM_2010_RESOURCE_DIRECTORY);
  auto regularFile = [](std::filesystem::path const& path) {
    std::error_code error;
    return std::filesystem::is_regular_file(path, error) && !error;
  };

  // Accept both the CERTI layout used by the sibling corpus and the compact
  // resource layout used by the 2025 payload. This is only path resolution;
  // the selected files are still validated and retained by the normal
  // provenance-bearing FOM pipeline.
  std::filesystem::path standardMim =
      root / "1516_1-2010" / "HLAstandardMIM.xml";
  std::filesystem::path difSchema =
      root / "1516_2-2010" / "IEEE1516-DIF-2010.xsd";
  std::filesystem::path fddSchema =
      root / "1516_1-2010" / "IEEE1516-FDD-2010.xsd";
  if (!regularFile(standardMim) || !regularFile(difSchema) || !regularFile(fddSchema)) {
    standardMim = root / "mim" / "HLAstandardMIM-2010.xml";
    difSchema = root / "schemas" / "IEEE1516-DIF-2010.xsd";
    fddSchema = root / "schemas" / "IEEE1516-FDD-2010.xsd";
  }
  if (!regularFile(standardMim) || !regularFile(difSchema) || !regularFile(fddSchema)) {
    throw RTIinternalError(
        L"Umbra could not locate the configured IEEE 1516.2-2010 schema/MIM resource payload.");
  }
  return umbra::detail::FomEditionResources{
      umbra::detail::FomStandardEdition::ieee1516_2010,
      std::move(standardMim),
      std::move(difSchema),
      std::move(fddSchema),
      std::wstring{umbra::detail::fomDifSchemaDesignator(
          umbra::detail::FomStandardEdition::ieee1516_2010)},
  };
#else
  return std::nullopt;
#endif
}

class EmbeddedFederationManagement final {
 public:
  EmbeddedFederationManagement()
      : resources_(embeddedResourceDirectory()),
        legacyResources_(embedded2010Resources()),
        composer_(
            resources_ / "schemas" / "IEEE1516-FDD-2025.xsd",
            legacyResources_ ? legacyResources_->fddSchemaPath : std::filesystem::path{}),
        coordinator_(
            validator_,
            composer_,
            timeSelector_,
            {
                resources_ / "mim" / umbra::detail::hla::utf8::mom::standard_mim_file_name,
                resources_ / "schemas" / "IEEE1516-DIF-2025.xsd",
                legacyResources_,
             }),
        registry_(std::make_shared<umbra::detail::EmbeddedFederationRegistry>()) {}

  [[nodiscard]] umbra::detail::EmbeddedFederationRegistry& registry() noexcept {
    return *registry_;
  }

  [[nodiscard]] std::shared_ptr<umbra::detail::EmbeddedFederationRegistry>
  replaceRegistryForTesting(
      std::shared_ptr<umbra::detail::EmbeddedFederationRegistry> replacement) {
    auto previous = std::move(registry_);
    registry_ = std::move(replacement);
    return previous;
  }

  [[nodiscard]] umbra::detail::ReferenceLogicalTimeSelector& timeSelector() noexcept {
    return timeSelector_;
  }

  [[nodiscard]] umbra::detail::FederationManagementCoordinator& coordinator() noexcept {
    return coordinator_;
  }

 private:
  std::filesystem::path resources_;
  std::optional<umbra::detail::FomEditionResources> legacyResources_;
  umbra::detail::LibXml2FomValidator validator_;
  umbra::detail::LibXml2FomModuleComposer composer_;
  umbra::detail::ReferenceLogicalTimeSelector timeSelector_;
  umbra::detail::FederationManagementCoordinator coordinator_;
  std::shared_ptr<umbra::detail::EmbeddedFederationRegistry> registry_;
};

EmbeddedFederationManagement& embeddedFederationManagement() {
  static EmbeddedFederationManagement management;
  return management;
}

std::mutex& federationManagementMutex() {
  static std::mutex mutex;
  return mutex;
}

// Both direct services and RTI-initiated recipient services use this one
// reservation/write path. The caller holds federationManagementMutex() and
// the endpoint mutex, which keeps the selected file's serial order identical
// to its durable record order.
void appendSelectedServiceReportRecord(
    JoinedServiceReportEndpoint& endpoint,
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint16_t serviceGroup,
    umbra::detail::FederateServiceReportRecordEncoder const& encodeRecord) {
  if (!endpoint.writer || !encodeRecord) {
    throw RTIinternalError(
        L"Umbra is missing the joined-federate service-report file state.");
  }

  auto& registry = embeddedFederationManagement().registry();
  auto const plan = registry.planMomServiceReport(
      federationName,
      federateId,
      serviceGroup);
  switch (plan.disposition) {
    case umbra::detail::MomServiceReportDisposition::suppressed:
      return;
    case umbra::detail::MomServiceReportDisposition::interaction:
      // The direct untimed Send Interaction wrapper handles this disposition
      // before reaching the file helper. Other successful-void wrappers remain
      // source/lock-gated and must not reserve a file serial here.
      return;
    case umbra::detail::MomServiceReportDisposition::report_to_file:
      break;
    case umbra::detail::MomServiceReportDisposition::inconsistent_catalog:
    case umbra::detail::MomServiceReportDisposition::reported_federate_not_member:
    case umbra::detail::MomServiceReportDisposition::invalid_service_group:
      throw RTIinternalError(
          L"Umbra could not select a valid service-report destination.");
  }

  auto const reserved = registry.reserveMomServiceReport(
      federationName,
      federateId,
      serviceGroup);
  if (!reserved.acceptedForEmission ||
      reserved.routing.disposition !=
          umbra::detail::MomServiceReportDisposition::report_to_file) {
    throw RTIinternalError(
        L"Umbra could not reserve the selected service-report file record.");
  }

  try {
    endpoint.writer->append(encodeRecord(reserved.serialNumber));
  } catch (std::exception const&) {
    // Reporting to a configured file is normative embedded-profile behavior;
    // do not replace this writer or silently redirect the record to memory.
    throw RTIinternalError(
        L"Umbra could not append the selected service-report file record.");
  }
}

umbra::detail::FederateServiceReportRoute makeFederateServiceReportRoute(
    std::shared_ptr<JoinedServiceReportEndpoint> const& endpoint,
    std::wstring federationName,
    std::uint64_t federateId) {
  std::weak_ptr<JoinedServiceReportEndpoint> const weakEndpoint = endpoint;
  return [
      weakEndpoint,
      federationName = std::move(federationName),
      federateId](
      std::uint16_t serviceGroup,
      umbra::detail::FederateServiceReportRecordEncoder encodeRecord) mutable {
    auto activeEndpoint = weakEndpoint.lock();
    if (!activeEndpoint) {
      // The route can outlive an already queued plan, but not the joined
      // federate's report-file lifetime. A resigned/disconnected recipient is
      // no longer a joined federate for §11.5 reporting purposes.
      return;
    }
    std::scoped_lock lock(federationManagementMutex(), activeEndpoint->mutex);
    appendSelectedServiceReportRecord(
        *activeEndpoint,
        federationName,
        federateId,
        serviceGroup,
        encodeRecord);
  };
}

umbra::detail::FederatePublicServiceReportRoute
makeFederatePublicServiceReportRoute(
    std::wstring federationName,
    std::uint64_t federateId) {
  return [
      federationName = std::move(federationName),
      federateId](
      std::wstring const& service,
      umbra::detail::MomServiceType serviceType,
      std::vector<umbra::detail::MomServiceArgument> const& suppliedArguments,
      umbra::detail::MomServiceArgument const& returnedArgument,
      bool success,
      std::wstring const& exception) {
    static_cast<void>(emitAmbassadorSelectedMomServiceReportInteractionForFederate(
        federationName,
        federateId,
        service,
        serviceType,
        suppliedArguments,
        returnedArgument,
        success,
        exception));
  };
}

void requireConnected(umbra::detail::FederateLifecycle const& lifecycle) {
  if (lifecycle.state() == umbra::detail::FederateLifecycleState::not_connected) {
    throw NotConnected(L"The federation-management service requires an active RTI connection.");
  }
}

void requireDefinitionTimeFactory(
    EmbeddedFederationManagement& management,
    umbra::detail::FederationDefinition const& definition) {
  if (!definition.catalog) {
    throw RTIinternalError(L"The embedded federation definition has no logical-time catalog.");
  }
  auto selection = management.timeSelector().select(
      *definition.catalog,
      definition.logicalTimeImplementationName);
  switch (selection.status) {
    case umbra::detail::ReferenceLogicalTimeSelectionStatus::selected:
      return;
    case umbra::detail::ReferenceLogicalTimeSelectionStatus::factory_unavailable:
      throw CouldNotCreateLogicalTimeFactory(
          L"Umbra could not create the federation's logical-time factory.");
    case umbra::detail::ReferenceLogicalTimeSelectionStatus::inconsistent_fdd_time_representation:
      throw InconsistentFOM(
          L"The federation's documented time representation conflicts with its logical-time factory.");
  }
  throw RTIinternalError(L"Umbra encountered an unknown logical-time selection outcome.");
}

std::unique_ptr<LogicalTimeFactory> makeDefinitionTimeFactory(
    EmbeddedFederationManagement& management,
    umbra::detail::FederationDefinition const& definition) {
  if (!definition.catalog) {
    throw RTIinternalError(L"The embedded federation definition has no logical-time catalog.");
  }

  auto selection = management.timeSelector().select(
      *definition.catalog,
      definition.logicalTimeImplementationName);
  if (selection.status != umbra::detail::ReferenceLogicalTimeSelectionStatus::selected ||
      selection.selectedImplementationName != definition.logicalTimeImplementationName) {
    // Creation and additional-FOM joins establish this invariant before the
    // definition enters the registry. A failure here means private state has
    // been corrupted, not that a caller supplied a bad getTimeFactory request.
    throw RTIinternalError(
        L"The embedded federation's selected logical-time implementation is no longer valid.");
  }

  auto factory = HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
      definition.logicalTimeImplementationName);
  if (!factory || factory->getName() != definition.logicalTimeImplementationName) {
    throw RTIinternalError(
        L"Umbra could not recreate the embedded federation's selected logical-time factory.");
  }
  return factory;
}

std::shared_ptr<umbra::detail::FederateTimeState> makeFederateTimeState(
    EmbeddedFederationManagement& management,
    umbra::detail::FederationDefinition const& definition) {
  // Preserve the public creation/join failure mapping before treating this
  // stored definition as an internal invariant for later time services.
  requireDefinitionTimeFactory(management, definition);
  auto factory = makeDefinitionTimeFactory(management, definition);
  auto initial = factory->makeInitial();
  if (!initial || initial->implementationName() != definition.logicalTimeImplementationName) {
    throw RTIinternalError(
        L"Umbra could not create the selected initial logical time for the joined federate.");
  }
  std::shared_ptr<LogicalTime> sharedInitial = std::move(initial);
  return std::make_shared<umbra::detail::FederateTimeState>(
      definition.logicalTimeImplementationName,
      std::move(sharedInitial));
}

std::shared_ptr<LogicalTime> cloneReferenceLogicalTime(
    std::wstring const& implementationName,
    LogicalTime const& time) {
  if (time.implementationName() != implementationName) {
    throw InvalidLogicalTime(
        L"The requested logical time does not use the joined federation's selected implementation.");
  }

  auto factory = HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(implementationName);
  if (!factory || factory->getName() != implementationName) {
    throw RTIinternalError(
        L"Umbra could not recreate the joined federation's logical-time factory.");
  }

  try {
    auto cloned = factory->decodeLogicalTime(time.encode());
    if (!cloned || cloned->implementationName() != implementationName) {
      throw InvalidLogicalTime(
          L"The requested logical time is not valid for the joined federation's implementation.");
    }
    std::shared_ptr<LogicalTime> sharedClone = std::move(cloned);
    return sharedClone;
  } catch (InvalidLogicalTime const&) {
    throw;
  } catch (Exception const&) {
    throw InvalidLogicalTime(
        L"The requested logical time cannot be decoded by the joined federation's implementation.");
  }
}

void validateTsoTimestamp(
    umbra::detail::FederateTimeSnapshot const& timeSnapshot,
    LogicalTime const& timestamp) {
  if (timestamp.isInitial() || timestamp.isFinal()) {
    throw InvalidLogicalTime(
        L"A timestamped service requires a finite logical timestamp.");
  }
  if (!timeSnapshot.timeRegulating || !timeSnapshot.currentTime ||
      !timeSnapshot.lookahead) {
    return;
  }

  auto lowerBound = cloneReferenceLogicalTime(
      timeSnapshot.implementationName,
      timeSnapshot.timeAdvancePending && timeSnapshot.advanceRequestTime
          ? *timeSnapshot.advanceRequestTime
          : timeSnapshot.timeAdvancePending && timeSnapshot.requestedTime
          ? *timeSnapshot.requestedTime
          : *timeSnapshot.currentTime);
  try {
    if (timeSnapshot.optimisticTime && *lowerBound < *timeSnapshot.optimisticTime) {
      *lowerBound = *timeSnapshot.optimisticTime;
    }
    *lowerBound += *timeSnapshot.lookahead;
    if (timeSnapshot.minimumTimestampIsExclusive) {
      auto factory = HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
          timeSnapshot.implementationName);
      if (!factory || factory->getName() != timeSnapshot.implementationName) {
        throw InvalidLogicalTime(
            L"Umbra could not create the selected logical-time factory for TSO validation.");
      }
      auto epsilon = factory->makeEpsilon();
      if (!epsilon || epsilon->implementationName() != timeSnapshot.implementationName) {
        throw InvalidLogicalTime(
            L"Umbra could not create the selected logical-time epsilon for TSO validation.");
      }
      *lowerBound += *epsilon;
    }
    if (timestamp < *lowerBound) {
      throw InvalidLogicalTime(
          L"A timestamped service is earlier than the sender's current logical time plus lookahead.");
    }
  } catch (InvalidLogicalTime const&) {
    throw;
  } catch (Exception const&) {
    throw InvalidLogicalTime(
        L"The timestamped service cannot be compared with the sender's TSO lower bound.");
  }
}

std::shared_ptr<LogicalTime const> makeTsoRetractionLowerBound(
    umbra::detail::FederateTimeSnapshot const& timeSnapshot) {
  if (!timeSnapshot.timeRegulating || !timeSnapshot.currentTime ||
      !timeSnapshot.lookahead) {
    throw RTIinternalError(
        L"The joined federate has incomplete time-regulation state for Retract.");
  }

  LogicalTime const* baseTime = timeSnapshot.currentTime.get();
  if (timeSnapshot.timeAdvancePending) {
    if (!timeSnapshot.advanceRequestTime) {
      throw RTIinternalError(
          L"The joined federate has no recorded advance request for Retract.");
    }
    baseTime = timeSnapshot.advanceRequestTime.get();
  }
  if (!baseTime || baseTime->implementationName() != timeSnapshot.implementationName ||
      timeSnapshot.lookahead->implementationName() != timeSnapshot.implementationName) {
    throw RTIinternalError(
        L"The joined federate has inconsistent logical-time state for Retract.");
  }

  auto lowerBound = cloneReferenceLogicalTime(
      timeSnapshot.implementationName,
      *baseTime);
  try {
    // 8.22.3 compares the original timestamp to the current time (or the
    // most recent advance-request time) plus actual lookahead. Optimistic
    // time and the send-service epsilon rule do not alter this retraction
    // precondition.
    *lowerBound += *timeSnapshot.lookahead;
  } catch (Exception const&) {
    throw RTIinternalError(
        L"Umbra could not calculate the Retract time-plus-lookahead boundary.");
  }
  std::shared_ptr<LogicalTime const> immutableLowerBound = std::move(lowerBound);
  return immutableLowerBound;
}

void retireTsoMessagePayloadsAtRetractionBoundary(
    std::wstring const& federationName,
    std::uint64_t producingFederateId,
    umbra::detail::FederateTimeState const& timeState) {
  auto const timeSnapshot = timeState.snapshot();
  if (!timeSnapshot.timeRegulating) {
    return;
  }
  auto const retractionLowerBound = makeTsoRetractionLowerBound(timeSnapshot);
  static_cast<void>(embeddedFederationManagement().registry().retireTsoMessagePayloadsAtOrBefore(
      federationName,
      producingFederateId,
      *retractionLowerBound));
}

std::shared_ptr<LogicalTimeInterval> cloneReferenceLogicalTimeInterval(
    std::wstring const& implementationName,
    LogicalTimeInterval const& interval) {
  if (interval.implementationName() != implementationName) {
    throw InvalidLookahead(
        L"The requested lookahead does not use the joined federation's selected implementation.");
  }

  auto factory = HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(implementationName);
  if (!factory || factory->getName() != implementationName) {
    throw RTIinternalError(
        L"Umbra could not recreate the joined federation's logical-time factory.");
  }

  std::unique_ptr<LogicalTimeInterval> cloned;
  try {
    cloned = factory->decodeLogicalTimeInterval(interval.encode());
  } catch (Exception const&) {
    throw InvalidLookahead(
        L"The requested lookahead cannot be decoded by the joined federation's implementation.");
  }
  if (!cloned || cloned->implementationName() != implementationName) {
    throw InvalidLookahead(
        L"The requested lookahead is not valid for the joined federation's implementation.");
  }

  std::unique_ptr<LogicalTimeInterval> zero;
  try {
    zero = factory->makeZero();
  } catch (Exception const&) {
    throw RTIinternalError(
        L"Umbra could not create the selected zero logical-time interval.");
  }
  if (!zero || zero->implementationName() != implementationName) {
    throw RTIinternalError(
        L"Umbra could not create a valid zero logical-time interval.");
  }

  try {
    if (*cloned < *zero) {
      throw InvalidLookahead(L"The requested lookahead must not be negative.");
    }
  } catch (InvalidLookahead const&) {
    throw;
  } catch (Exception const&) {
    throw InvalidLookahead(
        L"The requested lookahead cannot be compared with the selected zero interval.");
  }

  std::shared_ptr<LogicalTimeInterval> sharedClone = std::move(cloned);
  return sharedClone;
}

void copyQueriedLogicalTime(LogicalTime& target, LogicalTime const& source) {
  if (target.implementationName() != source.implementationName()) {
    throw RTIinternalError(
        L"Query Logical Time requires an output value from the joined federation's implementation.");
  }
  try {
    target = source;
  } catch (InvalidLogicalTime const&) {
    throw RTIinternalError(
        L"Query Logical Time could not copy the joined federate's logical time.");
  }
}

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
std::unique_ptr<LogicalTime> decodeProcessLogicalTimeValue(
    umbra::detail::ProcessFederationLogicalTime const& value) {
  if (value.implementationName.empty()) {
    throw RTIinternalError(
        L"The process endpoint returned a logical-time value without an implementation name.");
  }
  auto factory = HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
      value.implementationName);
  if (!factory || factory->getName() != value.implementationName) {
    throw RTIinternalError(
        L"The process endpoint returned an unknown logical-time implementation.");
  }
  VariableLengthData encoded;
  if (!value.encoding.empty()) {
    encoded.setData(value.encoding.data(), value.encoding.size());
  }
  std::unique_ptr<LogicalTime> decoded;
  try {
    decoded = factory->decodeLogicalTime(encoded);
  } catch (Exception const&) {
    throw RTIinternalError(
        L"The process endpoint returned an invalid logical-time encoding.");
  }
  if (!decoded || decoded->implementationName() != value.implementationName) {
    throw RTIinternalError(
        L"The process endpoint returned an invalid logical-time value.");
  }
  return decoded;
}

std::unique_ptr<LogicalTimeInterval> decodeProcessLogicalTimeIntervalValue(
    umbra::detail::ProcessFederationLogicalTimeInterval const& value) {
  if (value.implementationName.empty()) {
    throw RTIinternalError(
        L"The process endpoint returned a logical-time interval without an implementation name.");
  }
  auto factory = HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
      value.implementationName);
  if (!factory || factory->getName() != value.implementationName) {
    throw RTIinternalError(
        L"The process endpoint returned an unknown logical-time implementation.");
  }
  VariableLengthData encoded;
  if (!value.encoding.empty()) {
    encoded.setData(value.encoding.data(), value.encoding.size());
  }
  std::unique_ptr<LogicalTimeInterval> decoded;
  try {
    decoded = factory->decodeLogicalTimeInterval(encoded);
  } catch (Exception const&) {
    throw RTIinternalError(
        L"The process endpoint returned an invalid logical-time interval encoding.");
  }
  if (!decoded || decoded->implementationName() != value.implementationName) {
    throw RTIinternalError(
        L"The process endpoint returned an invalid logical-time interval.");
  }
  return decoded;
}
#endif

void copyQueriedLogicalTimeInterval(
    LogicalTimeInterval& target,
    LogicalTimeInterval const& source) {
  if (target.implementationName() != source.implementationName()) {
    throw RTIinternalError(
        L"Query Lookahead requires an output value from the joined federation's implementation.");
  }
  try {
    target = source;
  } catch (Exception const&) {
    throw RTIinternalError(
        L"Query Lookahead could not copy the joined federate's logical-time interval.");
  }
}

using AttributeValue = std::pair<std::uint64_t, VariableLengthData>;
using InteractionParameterValue = std::pair<std::uint64_t, VariableLengthData>;

std::optional<std::set<std::uint64_t>> attributeHandleValues(
    AttributeHandleSet const& attributes) {
  std::set<std::uint64_t> result;
  for (AttributeHandle const& attribute : attributes) {
    auto const value = attributeHandleValue(attribute);
    if (!value) {
      return std::nullopt;
    }
    result.insert(*value);
  }
  return result;
}

struct AttributeRegionPairValues {
  std::map<std::uint64_t, std::set<std::uint64_t>> values;
  bool invalidAttributeHandle = false;
  bool invalidRegionHandle = false;
};

AttributeRegionPairValues attributeRegionPairValues(
    AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions) {
  AttributeRegionPairValues result;
  for (auto const& [attributes, regions] : attributesAndRegions) {
    std::set<std::uint64_t> attributeValues;
    for (AttributeHandle const& attribute : attributes) {
      auto const value = attributeHandleValue(attribute);
      if (!value) {
        result.invalidAttributeHandle = true;
        continue;
      }
      attributeValues.insert(*value);
    }
    std::set<std::uint64_t> regionValues;
    for (RegionHandle const& region : regions) {
      auto const value = regionHandleValue(region);
      if (!value) {
        result.invalidRegionHandle = true;
        continue;
      }
      regionValues.insert(*value);
    }
    for (std::uint64_t const attributeValue : attributeValues) {
      auto& destination = result.values[attributeValue];
      destination.insert(regionValues.begin(), regionValues.end());
    }
  }
  return result;
}

VariableLengthData makeVariableLengthData(std::vector<unsigned char> const& bytes) {
  if (bytes.empty()) {
    return VariableLengthData();
  }
  return VariableLengthData(bytes.data(), bytes.size());
}

// HLAstandardMIM represents HLAswitch as an HLAinteger32BE enumerated value:
// Disabled is zero and Enabled is one.  Keep this decoder deliberately narrow
// so a malformed or unknown enumerator cannot silently change federation-wide
// state through the MOM adjustment interaction.
AttributeHandleValueMap projectAttributeValues(
    std::vector<AttributeValue> const& sentAttributes,
    std::set<std::uint64_t> const& receivedAttributeHandles) {
  AttributeHandleValueMap result;
  for (auto const& [attributeHandle, attributeValue] : sentAttributes) {
    if (receivedAttributeHandles.contains(attributeHandle)) {
      result.emplace(makeAttributeHandle(attributeHandle), attributeValue);
    }
  }
  return result;
}

std::optional<std::vector<std::uint64_t>> interactionParameterHandleValues(
    ParameterHandleValueMap const& parameterValues) {
  std::vector<std::uint64_t> result;
  result.reserve(parameterValues.size());
  for (auto const& [parameterHandle, parameterValue] : parameterValues) {
    static_cast<void>(parameterValue);
    auto const value = parameterHandleValue(parameterHandle);
    if (!value) {
      return std::nullopt;
    }
    result.push_back(*value);
  }
  return result;
}

std::vector<InteractionParameterValue> copyInteractionParameterValues(
    ParameterHandleValueMap const& parameterValues) {
  std::vector<InteractionParameterValue> result;
  result.reserve(parameterValues.size());
  for (auto const& [parameterHandle, parameterValue] : parameterValues) {
    auto const value = parameterHandleValue(parameterHandle);
    if (!value) {
      throw RTIinternalError(
          L"The Send Interaction parameter map changed while Umbra was copying its values.");
    }
    result.emplace_back(*value, parameterValue);
  }
  return result;
}

ParameterHandleValueMap projectInteractionParameterValues(
    std::vector<InteractionParameterValue> const& sentParameters,
    std::set<std::uint64_t> const& receivedParameterHandles) {
  ParameterHandleValueMap result;
  for (auto const& [parameterHandle, parameterValue] : sentParameters) {
    if (receivedParameterHandles.contains(parameterHandle)) {
      result.emplace(makeParameterHandle(parameterHandle), parameterValue);
    }
  }
  return result;
}

umbra::detail::FederateCallbackRoute makeFederateCallbackRoute(
    std::shared_ptr<umbra::detail::CallbackDispatcher> const& callbackDispatcher,
    std::shared_ptr<CallbackSession> const& callbackSession) {
  std::weak_ptr<umbra::detail::CallbackDispatcher> const dispatcher = callbackDispatcher;
  std::weak_ptr<CallbackSession> const session = callbackSession;
  auto pendingReceiveOrder =
      std::make_shared<std::atomic<std::size_t>>(0U);
  auto enqueue = [dispatcher, session, pendingReceiveOrder](
                     umbra::detail::FederateCallbackInvocation invocation,
                     bool receiveOrder) mutable {
    if (!invocation) {
      return;
    }
    if (receiveOrder) {
      pendingReceiveOrder->fetch_add(1U, std::memory_order_relaxed);
    }
    auto callbackDispatcher = dispatcher.lock();
    if (!callbackDispatcher) {
      if (receiveOrder) {
        pendingReceiveOrder->fetch_sub(1U, std::memory_order_relaxed);
      }
      return;
    }

    // Callers invoke this route only after releasing federation state locks.
    // An immediate dispatcher can enter user code synchronously here.
    callbackDispatcher->submit([
        session,
        pendingReceiveOrder,
        receiveOrder,
        invocation = std::move(invocation)]() mutable {
      if (receiveOrder) {
        pendingReceiveOrder->fetch_sub(1U, std::memory_order_relaxed);
      }
      auto callbackSession = session.lock();
      if (!callbackSession) {
        return;
      }
      callbackSession->invoke(std::move(invocation));
    });
  };

  umbra::detail::FederateCallbackRoute route;
  route.submit = [enqueue](umbra::detail::FederateCallbackInvocation invocation) mutable {
    enqueue(std::move(invocation), false);
  };
  route.receiveOrderSubmit = [enqueue](
      umbra::detail::FederateCallbackInvocation invocation) mutable {
    enqueue(std::move(invocation), true);
  };
  route.pendingReceiveOrderCount = [pendingReceiveOrder] {
    return pendingReceiveOrder->load(std::memory_order_relaxed);
  };
  return route;
}

// Request Retraction is a direct standard callback, not receive-order message
// traffic.  In particular it must not be deferred behind the recipient's time
// state: it tells the recipient to invalidate an already delivered TSO
// message.  The registry recheck protects a captured route from a later
// resignation/disconnect before user code is entered.
void queueRequestRetraction(
    umbra::detail::FederateCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t messageId) {
  if (!callbackRoute || messageId == 0 || receivingFederateId == 0) {
    return;
  }
  callbackRoute([
      federationName = std::move(federationName),
      receivingFederateId,
      messageId](FederateAmbassador& recipient) {
    bool mayDeliver = false;
    {
      std::scoped_lock lock(federationManagementMutex());
      mayDeliver = embeddedFederationRegistry()
          .canDeliverTsoRequestRetraction(
              federationName,
              receivingFederateId,
              messageId);
    }
    if (!mayDeliver) {
      return;
    }
    recipient.requestRetraction(makeMessageRetractionHandle(messageId));
  });
}

std::shared_ptr<umbra::detail::FederateTimeState> lookupFederateTimeState(
    std::wstring const& federationName,
    std::uint64_t federateId) {
  std::scoped_lock lock(federationManagementMutex());
  return embeddedFederationManagement().registry().timeStateFor(
      federationName,
      federateId);
}

void recordSuccessfulInteractionReceipt(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t receivedInteractionClassHandle,
    std::string transportationName,
    bool directed) {
  std::scoped_lock lock(federationManagementMutex());
  static_cast<void>(embeddedFederationManagement().registry()
                        .recordSuccessfulInteractionReceipt(
                            federationName,
                            receivingFederateId,
                            receivedInteractionClassHandle,
                            transportationName,
                            directed));
}

void recordSuccessfulReflectionReceipt(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::string transportationName) {
  std::scoped_lock lock(federationManagementMutex());
  static_cast<void>(embeddedFederationManagement().registry()
                        .recordSuccessfulReflectionReceipt(
                            federationName,
                            receivingFederateId,
                            objectInstanceHandle,
                            transportationName));
}

// Receive-order messages use the ordinary callback route, but their delivery
// is additionally gated by the recipient's temporal state.  Deferring the
// route invocation (rather than the projected callback payload) preserves the
// existing last-moment subscription/object/ownership checks when the message
// becomes eligible.  This also works for HLA_IMMEDIATE: a blocked callback is
// retained in the federate time state instead of recursively re-submitting to
// an immediate dispatcher.
void submitReceiveOrderCallback(
    umbra::detail::FederateCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t receivingFederateId,
    umbra::detail::FederateCallbackInvocation invocation) {
  if (!callbackRoute || !invocation) {
    return;
  }

  callbackRoute.enqueueReceiveOrder([
      callbackRoute,
      federationName = std::move(federationName),
      receivingFederateId,
      invocation = std::move(invocation)](
      FederateAmbassador& recipient) mutable {
    auto timeState = lookupFederateTimeState(
        federationName,
        receivingFederateId);
    if (timeState && !timeState->receiveOrderDeliveryAllowed()) {
      timeState->deferAsynchronousReceive([
          callbackRoute = std::move(callbackRoute),
          federationName,
          receivingFederateId,
          invocation = std::move(invocation)]() mutable {
        submitReceiveOrderCallback(
            std::move(callbackRoute),
            federationName,
            receivingFederateId,
            std::move(invocation));
      });
      return;
    }

    invocation(recipient);
  });
}

std::wstring declarationAdvisoryServiceReportRecord(
    umbra::detail::DeclarationAdvisoryKind kind,
    std::uint64_t classHandle,
    std::uint32_t serialNumber) {
  using umbra::detail::MomArgumentType;
  using umbra::detail::formatMomInteractionClassHandle;
  using umbra::detail::formatMomObjectClassHandle;
  using umbra::detail::formatMomSuccessfulVoidServiceReportRecord;

  switch (kind) {
    case umbra::detail::DeclarationAdvisoryKind::start_registration_for_object_class:
      return formatMomSuccessfulVoidServiceReportRecord(
          serialNumber,
          L"StartRegistrationForObjectClass",
          {{MomArgumentType::object_class_handle,
            L"Object class designator",
            formatMomObjectClassHandle(makeObjectClassHandle(classHandle))}});
    case umbra::detail::DeclarationAdvisoryKind::stop_registration_for_object_class:
      return formatMomSuccessfulVoidServiceReportRecord(
          serialNumber,
          L"StopRegistrationForObjectClass",
          {{MomArgumentType::object_class_handle,
            L"Object class designator",
            formatMomObjectClassHandle(makeObjectClassHandle(classHandle))}});
    case umbra::detail::DeclarationAdvisoryKind::turn_interactions_on:
      return formatMomSuccessfulVoidServiceReportRecord(
          serialNumber,
          L"TurnInteractionsOn",
          {{MomArgumentType::interaction_class_handle,
            L"Interaction class designator",
            formatMomInteractionClassHandle(makeInteractionClassHandle(classHandle))}});
    case umbra::detail::DeclarationAdvisoryKind::turn_interactions_off:
      return formatMomSuccessfulVoidServiceReportRecord(
          serialNumber,
          L"TurnInteractionsOff",
          {{MomArgumentType::interaction_class_handle,
            L"Interaction class designator",
            formatMomInteractionClassHandle(makeInteractionClassHandle(classHandle))}});
  }
  throw RTIinternalError(L"Umbra encountered an unknown declaration advisory kind.");
}

void queueDeclarationAdvisories(
    std::vector<umbra::detail::DeclarationAdvisory> advisories) {
  for (auto& advisory : advisories) {
    if (!advisory.callbackRoute) {
      throw RTIinternalError(
          L"The embedded federation has a declaration advisory without a callback route.");
    }
    if (advisory.serviceReportRoute) {
      // §§5.14--5.17 are RTI-initiated services at the publisher receiving
      // the relevance advisory. Append to that joined federate's already
      // selected report file before its HLA_EVOKED callback is queued.
      advisory.serviceReportRoute(
          static_cast<std::uint16_t>(
              umbra::detail::MomServiceType::declaration_management),
          [kind = advisory.kind, classHandle = advisory.classHandle](
              std::uint32_t serialNumber) {
            return declarationAdvisoryServiceReportRecord(kind, classHandle, serialNumber);
          });
    }
    advisory.callbackRoute([
        kind = advisory.kind,
        classHandle = advisory.classHandle](FederateAmbassador& recipient) {
      switch (kind) {
        case umbra::detail::DeclarationAdvisoryKind::start_registration_for_object_class:
          recipient.startRegistrationForObjectClass(makeObjectClassHandle(classHandle));
          return;
        case umbra::detail::DeclarationAdvisoryKind::stop_registration_for_object_class:
          recipient.stopRegistrationForObjectClass(makeObjectClassHandle(classHandle));
          return;
        case umbra::detail::DeclarationAdvisoryKind::turn_interactions_on:
          recipient.turnInteractionsOn(makeInteractionClassHandle(classHandle));
          return;
        case umbra::detail::DeclarationAdvisoryKind::turn_interactions_off:
          recipient.turnInteractionsOff(makeInteractionClassHandle(classHandle));
          return;
      }
      throw RTIinternalError(L"Umbra encountered an unknown declaration advisory kind.");
    });
  }
}

void submitSynchronizationPointRegistration(
    umbra::detail::SynchronizationPointRegistrationPlan plan) {
  if (!plan.registrationCallback) {
    throw RTIinternalError(
        L"The embedded federation could not schedule the synchronization-point registration result.");
  }
  if (plan.succeeded) {
    plan.registrationCallback([
        label = std::move(plan.label)](FederateAmbassador& recipient) {
      recipient.synchronizationPointRegistrationSucceeded(label);
    });
  } else {
    auto const failureReason = plan.failureReason;
    plan.registrationCallback([
        label = std::move(plan.label),
        failureReason](FederateAmbassador& recipient) {
      recipient.synchronizationPointRegistrationFailed(label, failureReason);
    });
  }
  submitAmbassadorSynchronizationPointAnnouncements(std::move(plan.announcements));
}

// RTI-originated MOM traffic uses the mandatory predefined reliable type.  A
// federation-scoped FOM name must instead go through the catalog-aware helper
// below so custom declarations retain their execution-local handle value.
TransportationTypeHandle transportationHandleFromEmbeddedName(
    std::string const& transportationName,
    wchar_t const* context) {
  auto const wideName = umbra::detail::wideFromUtf8(transportationName);
  auto const value = wideName
      ? standardTransportationTypeValue(*wideName)
      : std::nullopt;
  if (!value) {
    throw RTIinternalError(context);
  }
  return makeTransportationTypeHandle(*value);
}

std::optional<std::uint64_t> transportationTypeValueForFederation(
    std::wstring const& federationName,
    std::wstring_view transportationTypeName);

TransportationTypeHandle transportationHandleFromFederationName(
    std::wstring const& federationName,
    std::string const& transportationName,
    wchar_t const* context) {
  auto const wideName = umbra::detail::wideFromUtf8(transportationName);
  auto const value = wideName
      ? transportationTypeValueForFederation(federationName, *wideName)
      : std::nullopt;
  if (!value) {
    throw RTIinternalError(context);
  }
  return makeTransportationTypeHandle(*value);
}

std::optional<std::uint64_t> transportationTypeValueForFederation(
    std::wstring const& federationName,
    std::wstring_view transportationTypeName) {
  auto const encodedName = umbra::detail::utf8FromWide(transportationTypeName);
  if (!encodedName) {
    return std::nullopt;
  }
  std::scoped_lock lock(federationManagementMutex());
  return embeddedFederationManagement().registry().transportationTypeHandleFor(
      federationName,
      *encodedName);
}

std::optional<bool> transportationTypeReliableForFederation(
    std::wstring const& federationName,
    std::string const& transportationTypeName) {
  if (transportationTypeName == umbra::detail::hla::utf8::mom::reliable) {
    return true;
  }
  if (transportationTypeName == umbra::detail::hla::utf8::mom::best_effort) {
    return false;
  }
  std::scoped_lock lock(federationManagementMutex());
  auto const definition = embeddedFederationManagement().registry().definitionFor(
      federationName);
  if (!definition || !definition->catalog) {
    return std::nullopt;
  }
  auto const transportation = definition->catalog->transportationType(
      transportationTypeName);
  return transportation == nullptr
      ? std::nullopt
      : std::optional<bool>{transportation->reliable};
}

// TransportationTypeHandle values are opaque only at the public binding
// boundary.  Once a federate is joined, the composed execution catalog is the
// authority for both the two predefined transportation types and any
// execution-scoped FOM transportation declarations.  Control services must
// resolve against that catalog rather than treating the predefined pair as the
// complete handle universe.
std::optional<std::string> embeddedTransportationNameFromFederation(
    std::wstring const& federationName,
    TransportationTypeHandle const& transportationType) {
  auto const value = transportationTypeHandleValue(transportationType);
  if (!value) {
    return std::nullopt;
  }
  return embeddedFederationManagement().registry().transportationTypeNameFor(
      federationName,
      *value);
}

void queueConfirmAttributeTransportationTypeChange(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t requestId) {
  callbackRoute([
      federationName = std::move(federationName),
      requestingFederateId,
      requestId](FederateAmbassador& recipient) mutable {
    std::optional<umbra::detail::AttributeTransportationTypeChangeDelivery> delivery;
    {
      std::scoped_lock lock(federationManagementMutex());
      delivery = embeddedFederationManagement().registry()
                     .beginAttributeTransportationTypeChange(
                         federationName,
                         requestingFederateId,
                         requestId);
    }
    if (!delivery) {
      return;
    }
    AttributeHandleSet attributes;
    for (std::uint64_t const attributeHandle : delivery->attributeHandles) {
      attributes.insert(makeAttributeHandle(attributeHandle));
    }
    recipient.confirmAttributeTransportationTypeChange(
        makeObjectInstanceHandle(delivery->objectInstanceHandle),
        attributes,
        transportationHandleFromFederationName(
            federationName,
            delivery->transportationName,
            L"The embedded federation could not reconstruct an attribute transportation type confirmation."));
  });
}

void queueReportAttributeTransportationType(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t attributeHandle) {
  callbackRoute([
      federationName = std::move(federationName),
      requestingFederateId,
      objectInstanceHandle,
      attributeHandle](FederateAmbassador& recipient) mutable {
    std::optional<umbra::detail::AttributeTransportationTypeQueryPlan> plan;
    {
      std::scoped_lock lock(federationManagementMutex());
      plan = embeddedFederationManagement().registry()
                 .attributeTransportationTypeQueryFor(
                     federationName,
                     requestingFederateId,
                     objectInstanceHandle,
                     attributeHandle);
    }
    if (!plan) {
      return;
    }
    recipient.reportAttributeTransportationType(
        makeObjectInstanceHandle(plan->objectInstanceHandle),
        makeAttributeHandle(plan->attributeHandle),
        transportationHandleFromFederationName(
            federationName,
            plan->transportationName,
            L"The embedded federation could not reconstruct an attribute transportation type report."));
  });
}

void queueConfirmInteractionTransportationTypeChange(
    umbra::detail::InteractionCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t interactionClassHandle) {
  callbackRoute([
      federationName = std::move(federationName),
      requestingFederateId,
      interactionClassHandle](FederateAmbassador& recipient) mutable {
    std::optional<std::string> transportationName;
    {
      std::scoped_lock lock(federationManagementMutex());
      transportationName = embeddedFederationManagement().registry()
                               .beginInteractionTransportationTypeChange(
                                   federationName,
                                   requestingFederateId,
                                   interactionClassHandle);
    }
    if (!transportationName) {
      return;
    }
    recipient.confirmInteractionTransportationTypeChange(
        makeInteractionClassHandle(interactionClassHandle),
        transportationHandleFromFederationName(
            federationName,
            *transportationName,
            L"The embedded federation could not reconstruct an interaction transportation type confirmation."));
  });
}

void queueReportInteractionTransportationType(
    umbra::detail::InteractionCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t queriedFederateId,
    std::uint64_t interactionClassHandle) {
  callbackRoute([
      federationName = std::move(federationName),
      requestingFederateId,
      queriedFederateId,
      interactionClassHandle](FederateAmbassador& recipient) mutable {
    std::optional<umbra::detail::InteractionTransportationTypeQueryPlan> plan;
    {
      std::scoped_lock lock(federationManagementMutex());
      plan = embeddedFederationManagement().registry()
                 .interactionTransportationTypeQueryFor(
                     federationName,
                     requestingFederateId,
                     queriedFederateId,
                     interactionClassHandle);
    }
    if (!plan) {
      return;
    }
    recipient.reportInteractionTransportationType(
        makeFederateHandle(plan->queriedFederateId),
        makeInteractionClassHandle(plan->interactionClassHandle),
        transportationHandleFromFederationName(
            federationName,
            plan->transportationName,
            L"The embedded federation could not reconstruct an interaction transportation type report."));
  });
}

void queueTimestampedReflectAttributeUpdate(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t producingFederateId,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::vector<AttributeValue> sentAttributes,
    std::vector<umbra::detail::TsoAttributeUpdatePassel> passels,
    VariableLengthData userSuppliedTag,
    std::shared_ptr<LogicalTime const> timestamp,
    OrderType sentOrderType,
    OrderType receivedOrderType,
    std::optional<std::uint64_t> retractionMessageId) {
  submitReceiveOrderCallback(
      std::move(callbackRoute),
      federationName,
      receivingFederateId,
      [
          federationName,
          producingFederateId,
          receivingFederateId,
          objectInstanceHandle,
          sentAttributes = std::move(sentAttributes),
          passels = std::move(passels),
          userSuppliedTag = std::move(userSuppliedTag),
          timestamp = std::move(timestamp),
          sentOrderType,
          receivedOrderType,
          retractionMessageId](FederateAmbassador& recipient) mutable {
        if (!timestamp) {
          finishSuppressedTsoRecipientCallbackIfNeeded(
              federationName,
              receivingFederateId,
              retractionMessageId);
          return;
        }

        std::optional<MessageRetractionHandle> retraction;
        if (retractionMessageId) {
          retraction.emplace(makeMessageRetractionHandle(*retractionMessageId));
        }

        bool callbackBegan = false;
        for (auto const& passel : passels) {
          std::optional<umbra::detail::ReceiveOrderAttributeUpdateRecipient> projection;
          {
            std::scoped_lock lock(federationManagementMutex());
            auto& registry = embeddedFederationManagement().registry();
            projection = registry.receiveOrderAttributeUpdateRecipientFor(
                federationName,
                producingFederateId,
                receivingFederateId,
                objectInstanceHandle,
                passel.sentAttributeHandles,
                passel.sentRegionHandles.empty() ? nullptr : &passel.sentRegionHandles,
                passel.sentRegionSnapshots.empty()
                    ? nullptr
                    : &passel.sentRegionSnapshots);
          }
          if (!projection) {
            continue;
          }
          auto const transportationName = umbra::detail::wideFromUtf8(
              passel.transportationName);
          auto const transportationValue = transportationName
              ? transportationTypeValueForFederation(federationName, *transportationName)
              : std::nullopt;
          auto const reliableTransportation = transportationTypeReliableForFederation(
              federationName,
              passel.transportationName);
          if (!transportationValue || !reliableTransportation) {
            throw RTIinternalError(
                L"The embedded federation could not reconstruct a timestamped attribute callback transportation policy.");
          }

          AttributeHandleValueMap attributeValues = projectAttributeValues(
              sentAttributes,
              projection->receivedAttributeHandles);
          if (!*reliableTransportation) {
            for (auto iterator = attributeValues.begin(); iterator != attributeValues.end();) {
              std::uint64_t numeric = 0;
              for (auto const candidate : passel.sentAttributeHandles) {
                if (makeAttributeHandle(candidate) == iterator->first) {
                  numeric = candidate;
                  break;
                }
              }
              auto const rate = projection->maximumUpdateRatesByAttribute.find(numeric);
              auto const key = updateRateAdmissionKey(
                  federationName,
                  receivingFederateId,
                  objectInstanceHandle,
                  projection->subscriptionGeneration,
                  numeric);
              bool const admitted = !key || updateRateGate().admit(
                  *key,
                  rate == projection->maximumUpdateRatesByAttribute.end()
                      ? 0.0
                      : rate->second,
                  false);
              if (!admitted) {
                iterator = attributeValues.erase(iterator);
              } else {
                ++iterator;
              }
            }
          }
          if (attributeValues.empty()) {
            continue;
          }
          if (retractionMessageId) {
            std::scoped_lock lock(federationManagementMutex());
            if (!embeddedFederationManagement().registry().beginTsoAttributeUpdateCallback(
                    federationName,
                    receivingFederateId,
                    *retractionMessageId)) {
              return;
            }
          }
          callbackBegan = true;
          {
            std::scoped_lock lock(federationManagementMutex());
            static_cast<void>(embeddedFederationManagement().registry()
                                  .recordSuccessfulObjectInstanceReflection(
                                      federationName,
                                      receivingFederateId,
                                      objectInstanceHandle));
          }
          std::optional<RegionHandleSet> optionalSentRegions;
          if (projection->conveyRegionDesignatorSets &&
              (!passel.sentRegionHandles.empty() || passel.defaultRegionUsed)) {
            optionalSentRegions.emplace();
            for (std::uint64_t const regionHandle : passel.sentRegionHandles) {
              optionalSentRegions->insert(makeRegionHandle(regionHandle));
            }
          }

          recordSuccessfulReflectionReceipt(
              federationName,
              receivingFederateId,
              objectInstanceHandle,
              passel.transportationName);
          recipient.reflectAttributeValues(
              makeObjectInstanceHandle(objectInstanceHandle),
              attributeValues,
              userSuppliedTag,
              makeTransportationTypeHandle(*transportationValue),
              makeFederateHandle(producingFederateId),
              optionalSentRegions ? &*optionalSentRegions : nullptr,
              *timestamp,
              sentOrderType,
              receivedOrderType,
              retraction ? &*retraction : nullptr);
        }
        if (!callbackBegan && retractionMessageId) {
          std::scoped_lock lock(federationManagementMutex());
          static_cast<void>(embeddedFederationManagement().registry()
                                .finishTsoRecipientCallbackSuppressed(
                                    federationName,
                                    receivingFederateId,
                                    *retractionMessageId));
        }
      });
}

void queueAttributeValueUpdateProvide(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    umbra::detail::FederateServiceReportRoute serviceReportRoute,
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t providingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> requestedAttributeHandles,
    VariableLengthData userSuppliedTag,
    std::uint64_t pendingRequestId) {
  callbackRoute([
      federationName = std::move(federationName),
      requestingFederateId,
      providingFederateId,
      objectInstanceHandle,
      requestedAttributeHandles = std::move(requestedAttributeHandles),
      userSuppliedTag = std::move(userSuppliedTag),
      pendingRequestId,
      serviceReportRoute = std::move(serviceReportRoute)](FederateAmbassador& provider) mutable {
    std::optional<umbra::detail::AttributeValueUpdateProvideRecipient> projection;
    {
      std::scoped_lock lock(federationManagementMutex());
      if (pendingRequestId != 0U) {
        projection = embeddedFederationManagement().registry()
            .beginAttributeValueUpdateProvideRecipientFor(
                federationName,
                pendingRequestId,
                requestingFederateId,
                providingFederateId,
                objectInstanceHandle,
                requestedAttributeHandles);
      } else {
        projection = embeddedFederationManagement().registry()
            .attributeValueUpdateProvideRecipientFor(
                federationName,
                requestingFederateId,
                providingFederateId,
                objectInstanceHandle,
                requestedAttributeHandles,
                true);
      }
    }
    if (!projection) {
      return;
    }

    AttributeHandleSet attributes;
    for (std::uint64_t const attributeHandle : projection->requestedAttributeHandles) {
      attributes.insert(makeAttributeHandle(attributeHandle));
    }
    if (serviceReportRoute) {
      // §6.22 is the provider's RTI-initiated callback. The projection above
      // is the actual delivery boundary, so report only after it succeeds and
      // immediately before user code can observe the callback.
      serviceReportRoute(
          static_cast<std::uint16_t>(umbra::detail::MomServiceType::object_management),
          [
              objectInstanceHandle = projection->objectInstanceHandle,
              requestedAttributeHandles = projection->requestedAttributeHandles,
              userSuppliedTag](std::uint32_t serialNumber) {
            return makeAmbassadorProvideAttributeValueUpdateServiceReportRecord(
                objectInstanceHandle,
                requestedAttributeHandles,
                userSuppliedTag,
                serialNumber);
          });
    }
    provider.provideAttributeValueUpdate(
        makeObjectInstanceHandle(projection->objectInstanceHandle),
        attributes,
        userSuppliedTag);
  });
}

void queueAttributeValueUpdateClassProvide(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    umbra::detail::FederateServiceReportRoute serviceReportRoute,
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t providingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t requestedObjectClassHandle,
    std::set<std::uint64_t> requestedAttributeHandles,
    VariableLengthData userSuppliedTag,
    std::optional<std::map<std::uint64_t, std::set<std::uint64_t>>>
        requestRegionsByAttribute = std::nullopt,
    std::uint64_t pendingRequestId = 0U) {
  callbackRoute([
      federationName = std::move(federationName),
      requestingFederateId,
      providingFederateId,
      objectInstanceHandle,
      requestedObjectClassHandle,
      requestedAttributeHandles = std::move(requestedAttributeHandles),
      userSuppliedTag = std::move(userSuppliedTag),
      serviceReportRoute = std::move(serviceReportRoute),
      requestRegionsByAttribute = std::move(requestRegionsByAttribute),
      pendingRequestId](
      FederateAmbassador& provider) mutable {
    std::optional<umbra::detail::AttributeValueUpdateProvideRecipient> projection;
    {
      std::scoped_lock lock(federationManagementMutex());
      if (pendingRequestId != 0U) {
        if (requestRegionsByAttribute) {
          projection = embeddedFederationManagement().registry()
                           .beginAttributeValueUpdateRegionalProvideRecipientFor(
                               federationName,
                               pendingRequestId,
                               requestingFederateId,
                               providingFederateId,
                               objectInstanceHandle,
                               requestedObjectClassHandle,
                               requestedAttributeHandles,
                               *requestRegionsByAttribute);
        } else {
          projection = embeddedFederationManagement().registry()
                           .beginAttributeValueUpdateClassProvideRecipientFor(
                               federationName,
                               pendingRequestId,
                               requestingFederateId,
                               providingFederateId,
                               objectInstanceHandle,
                               requestedObjectClassHandle,
                               requestedAttributeHandles);
        }
      } else {
        projection = embeddedFederationManagement().registry()
                         .attributeValueUpdateClassProvideRecipientFor(
                             federationName,
                             requestingFederateId,
                             providingFederateId,
                             objectInstanceHandle,
                             requestedObjectClassHandle,
                             requestedAttributeHandles,
                             requestRegionsByAttribute ? &*requestRegionsByAttribute : nullptr);
      }
    }
    if (!projection) {
      return;
    }

    AttributeHandleSet attributes;
    for (std::uint64_t const attributeHandle : projection->requestedAttributeHandles) {
      attributes.insert(makeAttributeHandle(attributeHandle));
    }
    if (serviceReportRoute) {
      // The class and regional §6.21 planners converge on the same §6.22
      // provider callback. Its callback-time projection is likewise the only
      // point at which a selected-file record becomes durable.
      serviceReportRoute(
          static_cast<std::uint16_t>(umbra::detail::MomServiceType::object_management),
          [
              objectInstanceHandle = projection->objectInstanceHandle,
              requestedAttributeHandles = projection->requestedAttributeHandles,
              userSuppliedTag](std::uint32_t serialNumber) {
            return makeAmbassadorProvideAttributeValueUpdateServiceReportRecord(
                objectInstanceHandle,
                requestedAttributeHandles,
                userSuppliedTag,
                serialNumber);
          });
    }
    provider.provideAttributeValueUpdate(
        makeObjectInstanceHandle(projection->objectInstanceHandle),
        attributes,
        userSuppliedTag);
  });
}

FederationExecutionInformationVector federationExecutionReport(
    std::vector<umbra::detail::FederationExecutionSummary> const& federations) {
  FederationExecutionInformationVector report;
  report.reserve(federations.size());
  for (auto const& federation : federations) {
    report.emplace_back(federation.name, federation.logicalTimeImplementationName);
  }
  return report;
}

FederationExecutionMemberInformationVector federationExecutionMemberReport(
    std::vector<umbra::detail::FederateMembership> const& members) {
  FederationExecutionMemberInformationVector report;
  report.reserve(members.size());
  for (auto const& member : members) {
    report.emplace_back(member.name, member.type);
  }
  return report;
}

void requireValidResignAction(ResignAction resignAction) {
  switch (resignAction) {
    case UNCONDITIONALLY_DIVEST_ATTRIBUTES:
    case DELETE_OBJECTS:
    case CANCEL_PENDING_OWNERSHIP_ACQUISITIONS:
    case DELETE_OBJECTS_THEN_DIVEST:
    case CANCEL_THEN_DELETE_THEN_DIVEST:
    case NO_ACTION:
      return;
  }
  throw InvalidResignAction(L"The supplied resign action is not an IEEE 1516.1 ResignAction value.");
}

#endif

}  // namespace

std::wstring describeAmbassadorException(Exception const& exception) {
  return momExceptionDescription(exception);
}

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
std::mutex& ambassadorFederationManagementMutex() {
  return federationManagementMutex();
}

void requireConnectedForFederationManagement(
    umbra::detail::FederateLifecycle const& lifecycle) {
  requireConnected(lifecycle);
}

umbra::detail::EmbeddedFederationRegistry& embeddedFederationRegistry() {
  return embeddedFederationManagement().registry();
}

umbra::detail::FederationManagementCoordinator& embeddedFederationCoordinator() {
  return embeddedFederationManagement().coordinator();
}

void requireAmbassadorValidResignAction(ResignAction resignAction) {
  requireValidResignAction(resignAction);
}

void eraseAmbassadorUpdateRateHistoryForFederation(
    std::wstring const& federationName) {
  auto prefix = updateRateFederationPrefix(federationName);
  if (prefix) {
    updateRateGate().erasePrefix(*prefix);
  }
}

std::shared_ptr<umbra::detail::FederateTimeState> ambassadorMakeFederateTimeState(
    umbra::detail::FederationDefinition const& definition) {
  return makeFederateTimeState(embeddedFederationManagement(), definition);
}

std::unique_ptr<LogicalTimeFactory> ambassadorMakeDefinitionTimeFactory(
    umbra::detail::FederationDefinition const& definition) {
  return makeDefinitionTimeFactory(embeddedFederationManagement(), definition);
}

umbra::detail::FederateCallbackRoute ambassadorMakeFederateCallbackRoute(
    std::shared_ptr<umbra::detail::CallbackDispatcher> const& callbackDispatcher,
    std::shared_ptr<CallbackSession> const& callbackSession) {
  return makeFederateCallbackRoute(callbackDispatcher, callbackSession);
}

umbra::detail::FederateServiceReportRoute ambassadorMakeFederateServiceReportRoute(
    std::shared_ptr<JoinedServiceReportEndpoint> const& endpoint,
    std::wstring federationName,
    std::uint64_t federateId) {
  return makeFederateServiceReportRoute(
      endpoint,
      std::move(federationName),
      federateId);
}

umbra::detail::FederatePublicServiceReportRoute
ambassadorMakeFederatePublicServiceReportRoute(
    std::wstring federationName,
    std::uint64_t federateId) {
  return makeFederatePublicServiceReportRoute(
      std::move(federationName),
      federateId);
}

void submitAmbassadorSynchronizationPointRegistration(
    umbra::detail::SynchronizationPointRegistrationPlan plan) {
  submitSynchronizationPointRegistration(std::move(plan));
}

void eraseAmbassadorUpdateRateHistoryForFederate(
    std::wstring const& federationName,
    std::uint64_t federateId) {
  eraseUpdateRateHistoryForFederate(federationName, federateId);
}

void appendAmbassadorSelectedServiceReportRecord(
    JoinedServiceReportEndpoint& endpoint,
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint16_t serviceGroup,
    umbra::detail::FederateServiceReportRecordEncoder const& encodeRecord) {
  appendSelectedServiceReportRecord(
      endpoint,
      federationName,
      federateId,
      serviceGroup,
      encodeRecord);
}

void submitAmbassadorReceiveOrderCallback(
    umbra::detail::FederateCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t receivingFederateId,
    umbra::detail::FederateCallbackInvocation invocation) {
  submitReceiveOrderCallback(
      std::move(callbackRoute),
      std::move(federationName),
      receivingFederateId,
      std::move(invocation));
}

void queueAmbassadorTimestampedReflectAttributeUpdate(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t producingFederateId,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::vector<AmbassadorAttributeValue> sentAttributes,
    std::vector<umbra::detail::TsoAttributeUpdatePassel> passels,
    VariableLengthData userSuppliedTag,
    std::shared_ptr<LogicalTime const> timestamp,
    OrderType sentOrderType,
    OrderType receivedOrderType,
    std::optional<std::uint64_t> retractionMessageId) {
  queueTimestampedReflectAttributeUpdate(
      std::move(callbackRoute),
      std::move(federationName),
      producingFederateId,
      receivingFederateId,
      objectInstanceHandle,
      std::move(sentAttributes),
      std::move(passels),
      std::move(userSuppliedTag),
      std::move(timestamp),
      sentOrderType,
      receivedOrderType,
      retractionMessageId);
}

TransportationTypeHandle ambassadorTransportationHandleFromEmbeddedName(
    std::string const& transportationName,
    wchar_t const* context) {
  return transportationHandleFromEmbeddedName(transportationName, context);
}

TransportationTypeHandle ambassadorTransportationHandleFromFederationName(
    std::wstring const& federationName,
    std::string const& transportationName,
    wchar_t const* context) {
  return transportationHandleFromFederationName(
      federationName,
      transportationName,
      context);
}

std::optional<std::uint64_t> ambassadorTransportationTypeValueForFederation(
    std::wstring const& federationName,
    std::wstring_view transportationTypeName) {
  return transportationTypeValueForFederation(
      federationName,
      transportationTypeName);
}

std::optional<bool> ambassadorTransportationTypeReliableForFederation(
    std::wstring const& federationName,
    std::string const& transportationTypeName) {
  return transportationTypeReliableForFederation(
      federationName,
      transportationTypeName);
}

AttributeHandleValueMap ambassadorProjectAttributeValues(
    std::vector<AmbassadorAttributeValue> const& sentAttributes,
    std::set<std::uint64_t> const& receivedAttributeHandles) {
  return projectAttributeValues(sentAttributes, receivedAttributeHandles);
}

std::optional<std::string> ambassadorUpdateRateAdmissionKey(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t subscriptionGeneration,
    std::uint64_t attributeHandle) {
  return updateRateAdmissionKey(
      federationName,
      receivingFederateId,
      objectInstanceHandle,
      subscriptionGeneration,
      attributeHandle);
}

bool ambassadorAdmitUpdateRate(
    std::string const& key,
    double maximumRate,
    bool reliable) {
  return updateRateGate().admit(key, maximumRate, reliable);
}

void recordAmbassadorSuccessfulReflectionReceipt(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::string transportationName) {
  recordSuccessfulReflectionReceipt(
      federationName,
      receivingFederateId,
      objectInstanceHandle,
      std::move(transportationName));
}

ParameterHandleValueMap ambassadorProjectInteractionParameterValues(
    std::vector<AmbassadorInteractionParameterValue> const& sentParameters,
    std::set<std::uint64_t> const& receivedParameterHandles) {
  return projectInteractionParameterValues(sentParameters, receivedParameterHandles);
}

std::optional<std::vector<std::uint64_t>> ambassadorInteractionParameterHandleValues(
    ParameterHandleValueMap const& parameterValues) {
  return interactionParameterHandleValues(parameterValues);
}

std::vector<AmbassadorInteractionParameterValue> ambassadorCopyInteractionParameterValues(
    ParameterHandleValueMap const& parameterValues) {
  return copyInteractionParameterValues(parameterValues);
}

void recordAmbassadorSuccessfulInteractionReceipt(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t receivedInteractionClassHandle,
    std::string transportationName,
    bool directed) {
  recordSuccessfulInteractionReceipt(
      federationName,
      receivingFederateId,
      receivedInteractionClassHandle,
      std::move(transportationName),
      directed);
}

AmbassadorAttributeRegionPairValues ambassadorAttributeRegionPairValues(
    AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions) {
  auto result = attributeRegionPairValues(attributesAndRegions);
  return {
      std::move(result.values),
      result.invalidAttributeHandle,
      result.invalidRegionHandle};
}

std::optional<std::set<std::uint64_t>> ambassadorAttributeHandleValues(
    AttributeHandleSet const& attributes) {
  return attributeHandleValues(attributes);
}

[[noreturn]] void throwObjectClassAttributeDeclarationFailureForFederationManagement(
    umbra::detail::ObjectClassAttributeDeclarationStatus status) {
  throwObjectClassAttributeDeclarationFailure(status);
}

[[noreturn]] void throwObjectInstanceNameReservationFailureForFederationManagement(
    umbra::detail::ObjectInstanceNameReservationStatus status,
    std::wstring const& operation) {
  throwObjectInstanceNameReservationFailure(status, operation);
}

[[noreturn]] void throwObjectInstanceRegistrationFailureForFederationManagement(
    umbra::detail::ObjectInstanceRegistrationStatus status) {
  throwObjectInstanceRegistrationFailure(status);
}

[[noreturn]] void throwObjectInstanceRegionAssociationFailureForFederationManagement(
    umbra::detail::ObjectInstanceRegionAssociationStatus status) {
  throwObjectInstanceRegionAssociationFailure(status);
}

void queueAmbassadorDeclarationAdvisories(
    std::vector<umbra::detail::DeclarationAdvisory> advisories) {
  queueDeclarationAdvisories(std::move(advisories));
}

void queueAmbassadorConfirmAttributeTransportationTypeChange(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t requestId) {
  queueConfirmAttributeTransportationTypeChange(
      std::move(callbackRoute),
      std::move(federationName),
      requestingFederateId,
      requestId);
}

void queueAmbassadorReportAttributeTransportationType(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t attributeHandle) {
  queueReportAttributeTransportationType(
      std::move(callbackRoute),
      std::move(federationName),
      requestingFederateId,
      objectInstanceHandle,
      attributeHandle);
}

void queueAmbassadorAttributeValueUpdateProvide(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    umbra::detail::FederateServiceReportRoute serviceReportRoute,
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t providingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> requestedAttributeHandles,
    VariableLengthData userSuppliedTag,
    std::uint64_t pendingRequestId) {
  queueAttributeValueUpdateProvide(
      std::move(callbackRoute),
      std::move(serviceReportRoute),
      std::move(federationName),
      requestingFederateId,
      providingFederateId,
      objectInstanceHandle,
      std::move(requestedAttributeHandles),
      std::move(userSuppliedTag),
      pendingRequestId);
}

void queueAmbassadorAttributeValueUpdateClassProvide(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    umbra::detail::FederateServiceReportRoute serviceReportRoute,
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t providingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t requestedObjectClassHandle,
    std::set<std::uint64_t> requestedAttributeHandles,
    VariableLengthData userSuppliedTag,
    std::optional<std::map<std::uint64_t, std::set<std::uint64_t>>>
        requestRegionsByAttribute,
    std::uint64_t pendingRequestId) {
  queueAttributeValueUpdateClassProvide(
      std::move(callbackRoute),
      std::move(serviceReportRoute),
      std::move(federationName),
      requestingFederateId,
      providingFederateId,
      objectInstanceHandle,
      requestedObjectClassHandle,
      std::move(requestedAttributeHandles),
      std::move(userSuppliedTag),
      std::move(requestRegionsByAttribute),
      pendingRequestId);
}

void validateAmbassadorTsoTimestamp(
    umbra::detail::FederateTimeSnapshot const& timeSnapshot,
    LogicalTime const& timestamp) {
  validateTsoTimestamp(timeSnapshot, timestamp);
}

VariableLengthData makeAmbassadorVariableLengthData(
    std::vector<unsigned char> const& bytes) {
  return makeVariableLengthData(bytes);
}

void queueAmbassadorRequestRetraction(
    umbra::detail::FederateCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t messageId) {
  queueRequestRetraction(
      std::move(callbackRoute),
      std::move(federationName),
      receivingFederateId,
      messageId);
}

std::shared_ptr<LogicalTime const> makeAmbassadorTsoRetractionLowerBound(
    umbra::detail::FederateTimeSnapshot const& timeSnapshot) {
  return makeTsoRetractionLowerBound(timeSnapshot);
}

std::shared_ptr<LogicalTime> cloneAmbassadorReferenceLogicalTime(
    std::wstring const& implementationName,
    LogicalTime const& time) {
  return cloneReferenceLogicalTime(implementationName, time);
}

void copyAmbassadorQueriedLogicalTime(LogicalTime &target, LogicalTime const &source) {
  copyQueriedLogicalTime(target, source);
}

void copyAmbassadorQueriedLogicalTimeInterval(
    LogicalTimeInterval &target,
    LogicalTimeInterval const &source) {
  copyQueriedLogicalTimeInterval(target, source);
}

void retireAmbassadorTsoMessagePayloadsAtRetractionBoundary(
    std::wstring const& federationName,
    std::uint64_t producingFederateId,
    umbra::detail::FederateTimeState const& timeState) {
  retireTsoMessagePayloadsAtRetractionBoundary(
      federationName,
      producingFederateId,
      timeState);
}

std::shared_ptr<LogicalTimeInterval> cloneAmbassadorReferenceLogicalTimeInterval(
    std::wstring const& implementationName,
    LogicalTimeInterval const& interval) {
  return cloneReferenceLogicalTimeInterval(implementationName, interval);
}

std::unique_ptr<LogicalTime> decodeAmbassadorProcessLogicalTimeValue(
    umbra::detail::ProcessFederationLogicalTime const &value) {
  return decodeProcessLogicalTimeValue(value);
}

std::unique_ptr<LogicalTimeInterval> decodeAmbassadorProcessLogicalTimeIntervalValue(
    umbra::detail::ProcessFederationLogicalTimeInterval const &value) {
  return decodeProcessLogicalTimeIntervalValue(value);
}

[[noreturn]] void throwAmbassadorInteractionTransportationTypeChangeFailure(
    umbra::detail::InteractionTransportationTypeChangeStatus status) {
  throwInteractionTransportationTypeChangeFailure(status);
}

std::optional<std::string> ambassadorTransportationNameFromFederation(
    std::wstring const& federationName,
    TransportationTypeHandle const& transportationType) {
  return embeddedTransportationNameFromFederation(federationName, transportationType);
}

void queueAmbassadorConfirmInteractionTransportationTypeChange(
    umbra::detail::InteractionCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t interactionClassHandle) {
  queueConfirmInteractionTransportationTypeChange(
      std::move(callbackRoute),
      std::move(federationName),
      requestingFederateId,
      interactionClassHandle);
}

[[noreturn]] void throwAmbassadorInteractionTransportationTypeQueryFailure(
    umbra::detail::InteractionTransportationTypeQueryStatus status) {
  throwInteractionTransportationTypeQueryFailure(status);
}

void queueAmbassadorReportInteractionTransportationType(
    umbra::detail::InteractionCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t queriedFederateId,
    std::uint64_t interactionClassHandle) {
  queueReportInteractionTransportationType(
      std::move(callbackRoute),
      std::move(federationName),
      requestingFederateId,
      queriedFederateId,
      interactionClassHandle);
}
#endif

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
bool ambassadorMomExceptionIsParameterError(Exception const& exception) noexcept {
  if (exception.name() == L"InteractionParameterNotDefined") {
    return true;
  }
  auto const detail = exception.what();
  return detail.find(L"received an invalid") != std::wstring::npos ||
      detail.find(L"received an unexpected") != std::wstring::npos ||
      detail.find(L"requires the HLA") != std::wstring::npos ||
      detail.find(L"requires at least one declared parameter") != std::wstring::npos ||
      detail.find(L"requires a non-negative HLA") != std::wstring::npos;
}

UmbraRtiAmbassador::UmbraRtiAmbassador(
    ServiceReportStoreTestSeam,
    std::unique_ptr<umbra::detail::ServiceReportStore> serviceReportStore)
    : injectedServiceReportStoreForTesting_(std::move(serviceReportStore)) {
  if (!injectedServiceReportStoreForTesting_) {
    throw std::invalid_argument(
        "Umbra's internal service-report test seam requires a non-null store.");
  }
}

std::optional<umbra::detail::JoinedFederateMomObjectSnapshot>
UmbraRtiAmbassador::joinedFederateMomObjectSnapshotForTesting() const {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  if (!joinedFederationName_ || !joinedFederateId_ ||
      activeServiceReportStoreIsTestOnly_) {
    return std::nullopt;
  }
  return embeddedFederationManagement().registry().joinedFederateMomObjectFor(
      *joinedFederationName_,
      *joinedFederateId_);
}
#endif

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
std::shared_ptr<umbra::detail::EmbeddedFederationRegistry>
replaceEmbeddedFederationRegistryForTesting(
    std::shared_ptr<umbra::detail::EmbeddedFederationRegistry> replacement) {
  if (!replacement) {
    throw std::invalid_argument(
        "Umbra's embedded federation-registry test seam requires a non-null registry.");
  }
  std::scoped_lock lock(federationManagementMutex());
  return embeddedFederationManagement().replaceRegistryForTesting(std::move(replacement));
}
#endif

UmbraRtiAmbassador::UmbraRtiAmbassador(
    AuthorizerFactoryTestSeam,
    std::unique_ptr<rti1516_2025::AuthorizerFactory> authorizerFactory) {
  if (!authorizerFactory) {
    throw std::invalid_argument(
        "Umbra's internal authorizer test seam requires a non-null factory.");
  }
  authorizer_ = authorizerFactory->getAuthorizer();
  if (!authorizer_) {
    throw std::invalid_argument(
        "Umbra's internal authorizer test factory returned no authorizer.");
  }
}

UmbraRtiAmbassador::~UmbraRtiAmbassador() {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  stopPeriodicMomScheduler();
  if (processFederationClient_) {
    processFederationClient_->close();
    processFederationClient_.reset();
  }
  if (transportConnection_) {
    umbra::detail::embeddedTransportHub().disconnect(static_cast<RTIambassador*>(this));
    transportConnection_->close();
    transportConnection_.reset();
  }
#endif
  callbacks_->reset();
  if (callbackSession_) {
    callbackSession_->close();
  }
}

umbra::detail::RuntimeInstrumentationSnapshot
UmbraRtiAmbassador::runtimeInstrumentationSnapshotForTesting() const {
  auto result = instrumentation_->snapshot();
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  auto registry = embeddedFederationManagement().registry()
      .runtimeInstrumentationSnapshotForTesting();
  result.operations.insert(
      result.operations.end(),
      registry.operations.begin(),
      registry.operations.end());
  result.nextCorrelationId = std::max(result.nextCorrelationId, registry.nextCorrelationId);
#endif
  return result;
}

umbra::detail::RuntimeInstrumentationSnapshotProvider
UmbraRtiAmbassador::runtimeInstrumentationSnapshotProviderForTesting() const {
  return [this] { return runtimeInstrumentationSnapshotForTesting(); };
}

umbra::detail::RuntimeInstrumentation::Scope UmbraRtiAmbassador::beginRtiCall(
    std::string_view operation) const {
  auto const callStartNanoseconds =
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::steady_clock::now().time_since_epoch())
          .count();
  lastRtiCallStartNanoseconds_.store(
      callStartNanoseconds, std::memory_order_relaxed);
  return instrumentation_->begin(
      umbra::detail::InstrumentationLayer::rti_ambassador,
      operation);
}

void UmbraRtiAmbassador::disconnect() {
  auto instrumentationScope = beginRtiCall("disconnect");
  if (callbacks_->isExecutingCallback()) {
    throw CallNotAllowedFromWithinCallback(
        L"Disconnect cannot be called from within a federate callback.");
  }
  std::shared_ptr<CallbackSession> callbackSession;
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  std::shared_ptr<umbra::detail::TransportConnection> transportConnection;
  std::unique_ptr<umbra::detail::ProcessFederationClient>
      processFederationClient;
#endif
  {
    std::scoped_lock lock(mutex_);

    switch (lifecycle_.state()) {
      case umbra::detail::FederateLifecycleState::not_connected:
        throw NotConnected(L"The RTI ambassador has no active connection to disconnect.");
      case umbra::detail::FederateLifecycleState::joined:
        throw FederateIsExecutionMember(
            L"A joined federate must resign before disconnecting from the RTI.");
      case umbra::detail::FederateLifecycleState::not_joined:
        break;
    }

    if (lifecycle_.apply(umbra::detail::FederateLifecycleEvent::disconnect) !=
        umbra::detail::FederateLifecycleResult::applied) {
      throw RTIinternalError(L"Umbra could not apply the Disconnect lifecycle transition.");
    }

    callbackSession = std::move(callbackSession_);
    {
      std::scoped_lock authorizerLock(authorizerMutex_);
      authorizer_.reset();
    }
    // A disconnected ambassador cannot retain the filesystem directory/store
    // selected for its former connection. Completed joined-federate report
    // files remain on disk; only connection-owned state is released here.
    serviceReportStore_.reset();
    serviceReportConnection_ = {};
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
    transportConnection = std::move(transportConnection_);
    processFederationClient = std::move(processFederationClient_);
    processEndpointActive_ = false;
    processTimeRegulationCallbackPending_ = false;
    processTimeConstrainedCallbackPending_ = false;
    joinedServiceReport_.reset();
    processServiceReportFile_.reset();
    activeServiceReportStoreIsTestOnly_ = false;
    fomStandardEdition_ = umbra::detail::FomStandardEdition::ieee1516_2025;
#endif
    callbacks_->reset();
  }

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  stopPeriodicMomScheduler();
#endif

  // Do not hold the ambassador lock while an in-flight callback drains: the
  // recipient may make a re-entrant RTI call before it returns.
  if (callbackSession) {
    callbackSession->close();
  }
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (transportConnection) {
    umbra::detail::embeddedTransportHub().disconnect(static_cast<RTIambassador*>(this));
    transportConnection->close();
  }
  if (processFederationClient) {
    processFederationClient->close();
  }
#endif
}

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)

void UmbraRtiAmbassador::pumpProcessAttributeUpdates(bool drainAll) {
  if (!processEndpointActive_) {
    return;
  }

  std::wstring federationName;
  std::uint64_t receivingFederateId = 0U;
  umbra::detail::ProcessFederationClient* processClient = nullptr;
  {
    std::scoped_lock lock(mutex_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      return;
    }
    processClient = processFederationClient_.get();
    if (processClient == nullptr) {
      throw RTIinternalError(
          L"The configured process endpoint has no active federation client.");
    }
    federationName = *joinedFederationName_;
    receivingFederateId = *joinedFederateId_;
  }

  try {
    while (true) {
      auto event = processClient->receiveAttributeUpdate(
          federationName, receivingFederateId);
      if (event) {
        processClient->dispatchAttributeUpdate(std::move(*event));
      }
      while (processClient->pendingPushedAttributeUpdateCount() != 0U) {
        processClient->dispatchPushedAttributeUpdate();
      }
      if (!drainAll || !event) {
        break;
      }
    }
  } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
    throw RTIinternalError(wideAscii(error.what()));
  } catch (umbra::detail::ProcessFederationCallbackBridgeError const& error) {
    throw RTIinternalError(wideAscii(error.what()));
  } catch (umbra::detail::ProcessFederationClientError const& error) {
    throw RTIinternalError(wideAscii(error.what()));
  }
}


void UmbraRtiAmbassador::authorizeFederationOperationForConnectedFederate(
    std::wstring const& federationName,
    std::wstring_view operationName) {
  std::optional<Credentials> connectedCredentials;
  {
    std::scoped_lock stateLock(mutex_);
    requireConnected(lifecycle_);
    connectedCredentials = serviceReportConnection_.credentials;
  }

  HLAnoCredentials noCredentials;
  Credentials const& credentials = connectedCredentials
      ? *connectedCredentials
      : static_cast<Credentials const&>(noCredentials);

  // Keep the configured implementation alive and serialize its calls with
  // Disconnect while never holding the ambassador state mutex across plugin
  // code.
  std::scoped_lock authorizerLock(authorizerMutex_);
  if (authorizer_) {
    authorizeFederationOperation(
        *authorizer_, credentials, federationName, operationName);
  }
}

void UmbraRtiAmbassador::authorizeFederateOperationForConnectedFederate(
    std::wstring const& federationName,
    std::wstring const& federateName,
    std::wstring const& federateType) {
  std::optional<Credentials> connectedCredentials;
  {
    std::scoped_lock stateLock(mutex_);
    requireConnected(lifecycle_);
    connectedCredentials = serviceReportConnection_.credentials;
  }

  HLAnoCredentials noCredentials;
  Credentials const& credentials = connectedCredentials
      ? *connectedCredentials
      : static_cast<Credentials const&>(noCredentials);

  // Keep the configured implementation alive and serialize its calls with
  // Disconnect while never holding the ambassador state mutex across plugin
  // code.
  std::scoped_lock authorizerLock(authorizerMutex_);
  if (authorizer_) {
    authorizeFederateOperation(
        *authorizer_,
        credentials,
        federationName,
        federateName,
        federateType);
  }
}

void UmbraRtiAmbassador::listFederationExecutions() {
  auto instrumentationScope = beginRtiCall("listFederationExecutions");
  std::shared_ptr<CallbackSession> callbackSession;
  FederationExecutionInformationVector report;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    callbackSession = callbackSession_;
    if (!callbackSession) {
      throw RTIinternalError(L"The embedded connection has no federate ambassador callback recipient.");
    }
    report = federationExecutionReport(
        embeddedFederationManagement().registry().federationExecutions());
  }

  // The dispatcher may invoke an immediate callback synchronously. Submit only
  // after releasing both runtime locks so federate code can safely make an RTI
  // call from the report callback.
  callbacks_->submit([callbackSession = std::move(callbackSession), report = std::move(report)] {
    callbackSession->invoke([&report](FederateAmbassador& recipient) {
      recipient.reportFederationExecutions(report);
    });
  });
}

void UmbraRtiAmbassador::listFederationExecutionMembers(
    std::wstring const& federationName) {
  auto instrumentationScope = beginRtiCall("listFederationExecutionMembers");
  std::shared_ptr<CallbackSession> callbackSession;
  std::optional<std::vector<umbra::detail::FederateMembership>> members;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    callbackSession = callbackSession_;
    if (!callbackSession) {
      throw RTIinternalError(L"The embedded connection has no federate ambassador callback recipient.");
    }
    members = embeddedFederationManagement().registry().membersFor(federationName);
  }

  if (!members) {
    callbacks_->submit([callbackSession = std::move(callbackSession), federationName] {
      callbackSession->invoke([&federationName](FederateAmbassador& recipient) {
        recipient.reportFederationExecutionDoesNotExist(federationName);
      });
    });
    return;
  }

  auto report = federationExecutionMemberReport(*members);
  callbacks_->submit([
      callbackSession = std::move(callbackSession),
      federationName,
      report = std::move(report)] {
    callbackSession->invoke([&federationName, &report](FederateAmbassador& recipient) {
      recipient.reportFederationExecutionMembers(federationName, report);
    });
  });
}

FederateHandle UmbraRtiAmbassador::joinFederationExecution(
    std::wstring const& federateType,
    std::wstring const& federationName,
    std::vector<std::wstring> const& additionalFomModules) {
  auto instrumentationScope = beginRtiCall("joinFederationExecution");
  return joinFederationExecutionImpl(
      std::nullopt,
      federateType,
      federationName,
      additionalFomModules);
}

FederateHandle UmbraRtiAmbassador::joinFederationExecution(
    std::wstring const& federateName,
    std::wstring const& federateType,
    std::wstring const& federationName,
    std::vector<std::wstring> const& additionalFomModules) {
  auto instrumentationScope = beginRtiCall("joinFederationExecution");
  return joinFederationExecutionImpl(
      std::optional<std::wstring>{federateName},
      federateType,
      federationName,
      additionalFomModules);
}

ObjectClassHandle UmbraRtiAmbassador::getKnownObjectClassHandle(
    ObjectInstanceHandle const& objectInstance) {
  auto instrumentationScope = beginRtiCall("getKnownObjectClassHandle");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    std::optional<std::uint64_t> objectInstanceHandleValueResult;
    {
      std::scoped_lock lock(mutex_);
      requireConnected(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Get Known Object Class Handle requires membership in a federation execution.");
      }
      objectInstanceHandleValueResult = objectInstanceHandleValue(objectInstance);
      if (!objectInstanceHandleValueResult) {
        throw ObjectInstanceNotKnown(
            L"The supplied ObjectInstanceHandle is not known to this federate.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    std::optional<std::uint64_t> handle;
    try {
      handle = processClient->lookupKnownObjectClassHandle(
          std::move(federationName), federateId, *objectInstanceHandleValueResult);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (!handle) {
      throw ObjectInstanceNotKnown(
          L"The supplied ObjectInstanceHandle is not known to this federate.");
    }
    return makeObjectClassHandle(*handle);
  }
#endif
  ObjectClassHandle result;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Get Known Object Class Handle requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const objectInstanceHandleValueResult = objectInstanceHandleValue(objectInstance);
    if (!objectInstanceHandleValueResult) {
      throw ObjectInstanceNotKnown(
          L"The supplied ObjectInstanceHandle is not known to this federate.");
    }
    auto const known = registry.knownObjectInstanceFor(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectInstanceHandleValueResult);
    if (!known) {
      throw ObjectInstanceNotKnown(
          L"The supplied ObjectInstanceHandle is not known to this federate.");
    }
    result = makeObjectClassHandle(known->knownObjectClassHandle);
  }
  appendSuccessfulServiceReportToFileIfSelected(
      L"GetKnownObjectClassHandle",
      umbra::detail::MomServiceType::support_services,
      {{umbra::detail::MomArgumentType::object_instance_handle,
        L"Object instance handle",
        umbra::detail::formatMomObjectInstanceHandle(objectInstance)}},
      {umbra::detail::MomArgumentType::object_class_handle,
       L"Object class handle",
       umbra::detail::formatMomObjectClassHandle(result)});
  return result;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Known Object Class Handle", exception);
    appendFailedServiceReportToFileIfSelected(
        L"GetKnownObjectClassHandle",
        umbra::detail::MomServiceType::support_services,
        {{umbra::detail::MomArgumentType::object_instance_handle,
          L"Object instance handle",
          umbra::detail::formatMomObjectInstanceHandle(objectInstance)}},
        momExceptionDescription(exception));
    throw;
  }
}


RegionHandle UmbraRtiAmbassador::decodeRegionHandle(
    VariableLengthData const& encodedValue) const {
  auto instrumentationScope = beginRtiCall("decodeRegionHandle");
  try {
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Decode Region Handle requires membership in a federation execution.");
  }
  return ::rti1516_2025::umbra_binding_detail::decodeRegionHandle(encodedValue);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Decode Region Handle", exception);
    throw;
  }
}



MessageRetractionHandle UmbraRtiAmbassador::decodeMessageRetractionHandle(
    VariableLengthData const& encodedValue) const {
  auto instrumentationScope = beginRtiCall("decodeMessageRetractionHandle");
  try {
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Decode Message Retraction Handle requires membership in a federation execution.");
  }
  return ::rti1516_2025::umbra_binding_detail::decodeMessageRetractionHandle(encodedValue);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Decode Message Retraction Handle", exception);
    throw;
  }
}

#endif

}  // namespace rti1516_2025::umbra_binding_detail
