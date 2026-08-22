#include "internal/umbra_rti_ambassador.hpp"

#include "internal/service_report_store.hpp"

#include <RTI/auth/HLAnoCredentials.h>

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/attribute_handle.hpp"
#include "internal/dimension_handle.hpp"
#include "internal/federate_handle.hpp"
#include "internal/federate_time_state.hpp"
#include "internal/federation_management_coordinator.hpp"
#include "internal/federation_registry.hpp"
#include "internal/federation_time_bounds.hpp"
#include "internal/federation_time_grant_policy.hpp"
#include "internal/interaction_class_handle.hpp"
#include "internal/libxml2_fom_composer.hpp"
#include "internal/libxml2_fom_validator.hpp"
#include "internal/message_retraction_handle.hpp"
#include "internal/mom_service_report_encoding.hpp"
#include "internal/object_class_handle.hpp"
#include "internal/object_instance_handle.hpp"
#include "internal/parameter_handle.hpp"
#include "internal/region_handle.hpp"
#include "internal/reference_time_selection.hpp"
#include "internal/transportation_type_handle.hpp"
#include "internal/utf8_string.hpp"
#include "internal/update_rate_gate.hpp"

#include <RTI/FederateAmbassador.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/time/HLAlogicalTimeFactoryFactory.h>
#endif

#include <algorithm>
#include <atomic>
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
#include <type_traits>
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {
namespace {

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
umbra::detail::UpdateRateGate& updateRateGate() {
  static umbra::detail::UpdateRateGate gate;
  return gate;
}
#endif

void validateCallbackModel(CallbackModel callbackModel) {
  switch (callbackModel) {
    case HLA_IMMEDIATE:
    case HLA_EVOKED:
      return;
  }
  throw UnsupportedCallbackModel(L"Umbra supports only HLA_IMMEDIATE and HLA_EVOKED callback models.");
}

ConfigurationResult ignoredConfigurationResult() {
  // The embedded connection backend recognizes only its documented
  // serviceReportDirectory setting. Other endpoint/additional-settings fields
  // remain opaque and must not be reported as applied.
  return ConfigurationResult(false, false, SETTINGS_IGNORED);
}

void requireNoCredentialsWhenAuthorizationIsDisabled(
    Credentials const& credentials) {
  // §12.5 treats the predefined, empty HLAnoCredentials envelope as the
  // explicit form of connecting without credentials.  Umbra has no configured
  // authorizer yet, so a non-no-credentials envelope must not be silently
  // accepted as though it had been authenticated.
  if (credentials.getType() == HLAnoCredentialsType &&
      credentials.getData().size() == 0U) {
    return;
  }
  throw Unauthorized(
      L"Umbra has no authorization service configured for supplied credentials.");
}

std::filesystem::path configuredServiceReportDirectory(
    RtiConfiguration const* configuration,
    bool& settingsApplied) {
  settingsApplied = false;
  std::filesystem::path defaultDirectory;
  try {
    // A caller that needs durable, administrator-chosen retention supplies
    // serviceReportDirectory explicitly.  Keep the embedded profile's
    // implementation-defined default out of the source tree and still on a
    // real filesystem.
    defaultDirectory = std::filesystem::temp_directory_path() / "umbra-service-reports";
  } catch (std::filesystem::filesystem_error const&) {
    throw RTIinternalError(
        L"Umbra could not determine a default service-report directory.");
  }
  if (configuration == nullptr || configuration->additionalSettings().empty()) {
    return defaultDirectory;
  }
  constexpr std::wstring_view prefix = L"serviceReportDirectory=";
  auto const& settings = configuration->additionalSettings();
  if (!settings.starts_with(prefix)) {
    // RtiConfiguration::additionalSettings is an official opaque string.
    // Preserve the binding's existing ignored-settings behavior for values
    // owned by other profiles rather than treating every such value as an
    // Umbra configuration error.
    return defaultDirectory;
  }
  if (settings.size() == prefix.size()) {
    throw RTIinternalError(
        L"Umbra serviceReportDirectory requires a nonempty directory.");
  }
  settingsApplied = true;
  return std::filesystem::path(settings.substr(prefix.size()));
}

std::filesystem::path validateServiceReportDirectory(std::filesystem::path directory) {
  std::error_code error;
  auto absoluteDirectory = std::filesystem::absolute(directory, error);
  if (error) {
    throw RTIinternalError(
        L"Umbra could not resolve the configured service-report directory.");
  }
  std::filesystem::create_directories(absoluteDirectory, error);
  if (error || !std::filesystem::is_directory(absoluteDirectory, error) || error) {
    throw RTIinternalError(
        L"Umbra could not create or access the configured service-report directory.");
  }
  return absoluteDirectory.lexically_normal();
}

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
std::wstring serviceReportCallbackModelName(CallbackModel callbackModel) {
  switch (callbackModel) {
    case HLA_IMMEDIATE:
      return L"HLA_IMMEDIATE";
    case HLA_EVOKED:
      return L"HLA_EVOKED";
  }
  throw UnsupportedCallbackModel(
      L"Umbra supports only HLA_IMMEDIATE and HLA_EVOKED callback models.");
}

std::uint64_t nextServiceReportJoinIdentifier() {
  static std::atomic_uint64_t next{1U};
  auto const sequence = next.fetch_add(1U, std::memory_order_relaxed);
  auto const ticks = static_cast<std::uint64_t>(
      std::chrono::system_clock::now().time_since_epoch().count());
  // The local sequence separates joins in one process even when clocks have
  // the same resolution; mixing wall-clock ticks reduces cross-process
  // collision likelihood without assigning the path format to callers.
  return ticks ^ (sequence + 0x9e3779b97f4a7c15ULL + (ticks << 6U) + (ticks >> 2U));
}

constexpr std::wstring_view kUmbraEmbeddedFederateHost = L"umbra-embedded";
constexpr std::wstring_view kUmbraRtiVersion = L"Umbra 0.1.0";

std::vector<std::wstring> firstFomModuleDesignatorsSpecifiedAtJoin(
    std::vector<umbra::detail::PrevalidatedFomModule> const& modules) {
  std::set<std::filesystem::path> seenSources;
  std::vector<std::wstring> result;
  result.reserve(modules.size());
  for (auto const& module : modules) {
    if (module.kind == umbra::detail::FomModuleKind::fom &&
        seenSources.insert(module.sourcePath).second) {
      result.push_back(module.designator);
    }
  }
  return result;
}

std::wstring formatJoinedFederateServiceReportInitialRecord(
    ServiceReportConnectionSnapshot const& connection,
    std::wstring const& federationName,
    umbra::detail::FederationDefinition const& definition,
    umbra::detail::FederateMembership const& membership,
    bool autoProvide,
    std::vector<umbra::detail::PrevalidatedFomModule> const& fomModulesSpecifiedAtJoin) {
  umbra::detail::MomServiceReportInitialRecord record;
  record.callbackModel = serviceReportCallbackModelName(connection.callbackModel);
  record.configurationName = connection.configurationName;
  record.rtiAddress = connection.rtiAddress;
  record.additionalSettings = connection.additionalSettings;

  record.federationName = federationName;
  record.rtiVersion = std::wstring{kUmbraRtiVersion};
  record.timeImplementationName = definition.logicalTimeImplementationName;
  record.autoProvide = autoProvide;
  record.federateHandle = makeFederateHandle(membership.id).toString();
  record.federateName = membership.name;
  record.federateType = membership.type;
  // This in-process profile has no transport-host discovery layer yet.  The
  // stable profile identity is more truthful than manufacturing a network
  // host value; a remote transport will supply the actual host before this
  // behavior is promoted as MOM conformance evidence.
  record.federateHost = std::wstring{kUmbraEmbeddedFederateHost};

  for (auto const& module : definition.fomModules) {
    if (module.kind == umbra::detail::FomModuleKind::mim) {
      record.mimDesignator = module.designator;
    } else {
      record.federationFomModuleDesignators.push_back(module.designator);
    }
  }
  record.federateFomModuleDesignators =
      firstFomModuleDesignatorsSpecifiedAtJoin(fomModulesSpecifiedAtJoin);
  return umbra::detail::formatMomServiceReportInitialRecord(record);
}
#endif

umbra::detail::CallbackDispatchModel toDispatchModel(CallbackModel callbackModel) {
  switch (callbackModel) {
    case HLA_IMMEDIATE:
      return umbra::detail::CallbackDispatchModel::immediate;
    case HLA_EVOKED:
      return umbra::detail::CallbackDispatchModel::evoked;
  }
  throw UnsupportedCallbackModel(L"Umbra supports only HLA_IMMEDIATE and HLA_EVOKED callback models.");
}

std::chrono::milliseconds callbackWaitDuration(double seconds) {
  if (!std::isfinite(seconds) || seconds <= 0.0) {
    return std::chrono::milliseconds::zero();
  }

  double const maximumMilliseconds =
      static_cast<double>(std::chrono::milliseconds::max().count());
  if (seconds >= maximumMilliseconds / 1000.0) {
    return std::chrono::milliseconds::max();
  }
  return std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::duration<double>(seconds));
}

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)

#ifndef UMBRA_EMBEDDED_FOM_RESOURCE_DIRECTORY
#error "The embedded federation-management profile requires the vendored 1516.2 resource directory."
#endif

std::filesystem::path embeddedResourceDirectory() {
  return std::filesystem::path(UMBRA_EMBEDDED_FOM_RESOURCE_DIRECTORY);
}

class EmbeddedFederationManagement final {
 public:
  EmbeddedFederationManagement()
      : resources_(embeddedResourceDirectory()),
        composer_(resources_ / "schemas" / "IEEE1516-FDD-2025.xsd"),
        coordinator_(
            validator_,
            composer_,
            timeSelector_,
            {
                resources_ / "mim" / "HLAstandardMIM-2025.xml",
                resources_ / "schemas" / "IEEE1516-DIF-2025.xsd",
            }) {}

  [[nodiscard]] umbra::detail::EmbeddedFederationRegistry& registry() noexcept {
    return registry_;
  }

  [[nodiscard]] umbra::detail::ReferenceLogicalTimeSelector& timeSelector() noexcept {
    return timeSelector_;
  }

  [[nodiscard]] umbra::detail::FederationManagementCoordinator& coordinator() noexcept {
    return coordinator_;
  }

 private:
  std::filesystem::path resources_;
  umbra::detail::LibXml2FomValidator validator_;
  umbra::detail::LibXml2FomModuleComposer composer_;
  umbra::detail::ReferenceLogicalTimeSelector timeSelector_;
  umbra::detail::FederationManagementCoordinator coordinator_;
  umbra::detail::EmbeddedFederationRegistry registry_;
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

void requireConnected(umbra::detail::FederateLifecycle const& lifecycle) {
  if (lifecycle.state() == umbra::detail::FederateLifecycleState::not_connected) {
    throw NotConnected(L"The federation-management service requires an active RTI connection.");
  }
}

[[noreturn]] void throwInteractionClassDeclarationFailure(
    umbra::detail::InteractionClassDeclarationStatus status) {
  using Status = umbra::detail::InteractionClassDeclarationStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::federate_not_member:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    case Status::interaction_class_not_defined:
      throw InteractionClassNotDefined(
          L"The supplied InteractionClassHandle is not defined in this federation execution.");
    case Status::federate_service_invocations_are_being_reported_via_mom:
      throw FederateServiceInvocationsAreBeingReportedViaMOM(
          L"The report-service-invocation interaction cannot be subscribed while service reporting is enabled.");
    case Status::applied:
      break;
  }
  throw RTIinternalError(L"Umbra encountered an unknown interaction declaration outcome.");
}

[[noreturn]] void throwDirectedInteractionDeclarationFailure(
    umbra::detail::DirectedInteractionDeclarationStatus status,
    std::wstring const& operation) {
  using Status = umbra::detail::DirectedInteractionDeclarationStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::federate_not_member:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    case Status::object_class_not_defined:
      throw ObjectClassNotDefined(
          operation + L" received an ObjectClassHandle that is not defined in this federation.");
    case Status::interaction_class_not_defined:
      throw InteractionClassNotDefined(
          operation + L" received an InteractionClassHandle that is not defined in this federation.");
    case Status::interaction_not_defined_for_object_class:
      throw InteractionClassNotDefined(
          operation + L" received an interaction that is not a directed interaction of the object class.");
    case Status::inconsistent_catalog:
      throw RTIinternalError(
          operation + L" could not resolve the directed interaction against the composed FOM.");
    case Status::applied:
      break;
  }
  throw RTIinternalError(
      operation + L" encountered an unknown directed interaction declaration outcome.");
}

[[noreturn]] void throwRegionalInteractionClassDeclarationFailure(
    umbra::detail::RegionalInteractionClassDeclarationStatus status) {
  using Status = umbra::detail::RegionalInteractionClassDeclarationStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::federate_not_member:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    case Status::interaction_class_not_defined:
      throw InteractionClassNotDefined(
          L"The supplied InteractionClassHandle is not defined in this federation execution.");
    case Status::federate_service_invocations_are_being_reported_via_mom:
      throw FederateServiceInvocationsAreBeingReportedViaMOM(
          L"The report-service-invocation interaction cannot be subscribed while service reporting is enabled.");
    case Status::invalid_region_context:
      throw InvalidRegionContext(
          L"The supplied region dimensions are not available for this interaction class.");
    case Status::region_not_created_by_this_federate:
      throw RegionNotCreatedByThisFederate(
          L"The interaction regional declaration requires regions created by this federate.");
    case Status::invalid_region:
      throw InvalidRegion(
          L"The interaction regional declaration received an invalid region specification.");
    case Status::inconsistent_catalog:
      throw RTIinternalError(
          L"Umbra could not resolve the interaction regional declaration against the FOM.");
    case Status::applied:
      break;
  }
  throw RTIinternalError(
      L"Umbra encountered an unknown regional interaction declaration outcome.");
}

[[noreturn]] void throwObjectClassAttributeDeclarationFailure(
    umbra::detail::ObjectClassAttributeDeclarationStatus status) {
  using Status = umbra::detail::ObjectClassAttributeDeclarationStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::federate_not_member:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    case Status::object_class_not_defined:
      throw ObjectClassNotDefined(
          L"The supplied ObjectClassHandle is not defined in this federation execution.");
    case Status::attribute_not_defined:
      throw AttributeNotDefined(
          L"The supplied AttributeHandle is not defined for this object class.");
    case Status::invalid_update_rate_designator:
      throw InvalidUpdateRateDesignator(
          L"The supplied update-rate designator is not defined by the current FDD.");
    case Status::ownership_acquisition_pending:
      throw OwnershipAcquisitionPending(
          L"Unpublish Object Class Attributes cannot remove a publication required by a "
          L"pending ownership acquisition.");
    case Status::inconsistent_catalog:
      throw RTIinternalError(
          L"Umbra could not resolve object-class declaration state in the embedded federation.");
    case Status::applied:
      break;
  }
  throw RTIinternalError(L"Umbra encountered an unknown object attribute declaration outcome.");
}

[[noreturn]] void throwRegionServiceFailure(
    umbra::detail::RegionServiceStatus status,
    std::wstring const& operation) {
  using Status = umbra::detail::RegionServiceStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::federate_not_member:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    case Status::invalid_dimension:
      throw InvalidDimensionHandle(
          operation + L" requires dimensions defined in the federation FOM.");
    case Status::invalid_region:
    case Status::incomplete_region:
      throw InvalidRegion(
          operation + L" received a region that is not a valid region specification.");
    case Status::region_not_created_by_this_federate:
      throw RegionNotCreatedByThisFederate(
          operation + L" requires a region created by this federate.");
    case Status::region_in_use:
      throw RegionInUseForUpdateOrSubscription(
          operation + L" cannot delete a region that is still in use.");
    case Status::dimension_not_in_region:
      throw RegionDoesNotContainSpecifiedDimension(
          operation + L" requires a dimension contained by the region.");
    case Status::invalid_range_bound:
      throw InvalidRangeBound(
          operation + L" requires 0 <= lowerBound < upperBound <= dimension upper bound.");
    case Status::inconsistent_catalog:
      throw RTIinternalError(
          operation + L" could not resolve the composed FOM dimension catalog.");
    case Status::applied:
      break;
  }
  throw RTIinternalError(operation + L" encountered an unknown region-service outcome.");
}

[[noreturn]] void throwObjectInstanceRegistrationFailure(
    umbra::detail::ObjectInstanceRegistrationStatus status) {
  using Status = umbra::detail::ObjectInstanceRegistrationStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::federate_not_member:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    case Status::object_class_not_defined:
      throw ObjectClassNotDefined(
          L"The supplied ObjectClassHandle is not defined in this federation execution.");
    case Status::object_class_not_published:
      throw ObjectClassNotPublished(
          L"Register Object Instance requires publication of the supplied object class.");
    case Status::object_instance_name_in_use:
      throw ObjectInstanceNameInUse(
          L"Register Object Instance requires an object instance name that is not in use.");
    case Status::object_instance_name_not_reserved:
      throw ObjectInstanceNameNotReserved(
          L"Register Object Instance requires an object instance name reserved by this federate.");
    case Status::attribute_not_published:
      throw AttributeNotPublished(
          L"Register Object Instance With Regions requires publication of every associated attribute.");
    case Status::attribute_not_defined:
      throw AttributeNotDefined(
          L"The supplied AttributeHandle is not defined for this object class.");
    case Status::invalid_region_context:
      throw InvalidRegionContext(
          L"The supplied region dimensions are not available for this object class.");
    case Status::region_not_created_by_this_federate:
      throw RegionNotCreatedByThisFederate(
          L"The object regional declaration requires regions created by this federate.");
    case Status::invalid_region:
      throw InvalidRegion(
          L"The object regional declaration received an invalid region specification.");
    case Status::object_instance_handle_exhausted:
    case Status::inconsistent_catalog:
      throw RTIinternalError(
          L"Umbra could not create a coherent object instance in the embedded federation.");
    case Status::applied:
      break;
  }
  throw RTIinternalError(L"Umbra encountered an unknown object registration outcome.");
}

[[noreturn]] void throwObjectInstanceNameReservationFailure(
    umbra::detail::ObjectInstanceNameReservationStatus status,
    std::wstring const& operation) {
  using Status = umbra::detail::ObjectInstanceNameReservationStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::federate_not_member:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    case Status::illegal_name:
      throw IllegalName(operation + L" received an illegal object instance name.");
    case Status::name_set_was_empty:
      throw NameSetWasEmpty(operation + L" requires a non-empty object instance name set.");
    case Status::object_instance_name_not_reserved:
      throw ObjectInstanceNameNotReserved(
          operation + L" requires a name reserved by this federate.");
    case Status::callback_route_missing:
      throw RTIinternalError(
          operation + L" has no callback route for the joined federate.");
    case Status::applied:
      break;
  }
  throw RTIinternalError(operation + L" encountered an unknown reservation outcome.");
}

[[noreturn]] void throwRegionalObjectClassAttributeDeclarationFailure(
    umbra::detail::RegionalObjectClassAttributeDeclarationStatus status) {
  using Status = umbra::detail::RegionalObjectClassAttributeDeclarationStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::federate_not_member:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    case Status::object_class_not_defined:
      throw ObjectClassNotDefined(
          L"The supplied ObjectClassHandle is not defined in this federation execution.");
    case Status::attribute_not_defined:
      throw AttributeNotDefined(
          L"The supplied AttributeHandle is not defined for this object class.");
    case Status::invalid_update_rate_designator:
      throw InvalidUpdateRateDesignator(
          L"The supplied update-rate designator is not defined by the current FDD.");
    case Status::invalid_region_context:
      throw InvalidRegionContext(
          L"The supplied region dimensions are not available for this object class.");
    case Status::region_not_created_by_this_federate:
      throw RegionNotCreatedByThisFederate(
          L"The object regional subscription requires regions created by this federate.");
    case Status::invalid_region:
      throw InvalidRegion(
          L"The object regional subscription received an invalid region specification.");
    case Status::inconsistent_catalog:
      throw RTIinternalError(
          L"Umbra could not resolve the object regional subscription against the FOM.");
    case Status::applied:
      break;
  }
  throw RTIinternalError(
      L"Umbra encountered an unknown regional object subscription outcome.");
}

[[noreturn]] void throwObjectInstanceRegionAssociationFailure(
    umbra::detail::ObjectInstanceRegionAssociationStatus status) {
  using Status = umbra::detail::ObjectInstanceRegionAssociationStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::federate_not_member:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    case Status::object_instance_not_known:
      throw ObjectInstanceNotKnown(
          L"The supplied ObjectInstanceHandle is not known to this federate.");
    case Status::attribute_not_defined:
      throw AttributeNotDefined(
          L"The supplied AttributeHandle is not defined for this object instance.");
    case Status::invalid_region_context:
      throw InvalidRegionContext(
          L"The supplied region dimensions are not available for this object class.");
    case Status::region_not_created_by_this_federate:
      throw RegionNotCreatedByThisFederate(
          L"The object regional association requires regions created by this federate.");
    case Status::invalid_region:
      throw InvalidRegion(
          L"The object regional association received an invalid region specification.");
    case Status::inconsistent_catalog:
      throw RTIinternalError(
          L"Umbra could not resolve the object regional association against the FOM.");
    case Status::applied:
      break;
  }
  throw RTIinternalError(
      L"Umbra encountered an unknown object regional association outcome.");
}

[[noreturn]] void throwObjectInstanceDeletionFailure(
    umbra::detail::ObjectInstanceDeletionStatus status) {
  using Status = umbra::detail::ObjectInstanceDeletionStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::federate_not_member:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    case Status::object_instance_not_known:
      throw ObjectInstanceNotKnown(
          L"The supplied ObjectInstanceHandle is not known to this federate.");
    case Status::delete_privilege_not_held:
      throw DeletePrivilegeNotHeld(
          L"Delete Object Instance requires ownership of HLAprivilegeToDeleteObject.");
    case Status::inconsistent_catalog:
      throw RTIinternalError(
          L"Umbra could not resolve a coherent object instance deletion state.");
    case Status::applied:
      break;
  }
  throw RTIinternalError(L"Umbra encountered an unknown object deletion outcome.");
}

[[noreturn]] void throwLocalObjectInstanceDeletionFailure(
    umbra::detail::LocalObjectInstanceDeletionStatus status) {
  using Status = umbra::detail::LocalObjectInstanceDeletionStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::federate_not_member:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    case Status::object_instance_not_known:
      throw ObjectInstanceNotKnown(
          L"The supplied ObjectInstanceHandle is not known to this federate.");
    case Status::ownership_acquisition_pending:
      throw OwnershipAcquisitionPending(
          L"Local Delete Object Instance cannot discard a pending ownership acquisition.");
    case Status::federate_owns_attributes:
      throw FederateOwnsAttributes(
          L"Local Delete Object Instance requires the federate to own no instance attributes.");
    case Status::applied:
      break;
  }
  throw RTIinternalError(L"Umbra encountered an unknown local object deletion outcome.");
}

[[noreturn]] void throwReceiveOrderInteractionFailure(
    umbra::detail::ReceiveOrderInteractionStatus status) {
  using Status = umbra::detail::ReceiveOrderInteractionStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::producing_federate_not_member:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    case Status::interaction_class_not_defined:
      throw InteractionClassNotDefined(
          L"The supplied InteractionClassHandle is not defined in this federation execution.");
    case Status::interaction_class_not_published:
      throw InteractionClassNotPublished(
          L"Send Interaction requires publication of the supplied interaction class.");
    case Status::interaction_parameter_not_defined:
      throw InteractionParameterNotDefined(
          L"The supplied ParameterHandle is not defined for the interaction class.");
    case Status::invalid_region_context:
      throw InvalidRegionContext(
          L"The supplied region dimensions are not available for this interaction class.");
    case Status::region_not_created_by_this_federate:
      throw RegionNotCreatedByThisFederate(
          L"Send Interaction With Regions requires regions created by this federate.");
    case Status::invalid_region:
      throw InvalidRegion(
          L"Send Interaction With Regions received an invalid region specification.");
    case Status::inconsistent_catalog:
      throw RTIinternalError(
          L"Umbra could not resolve a coherent interaction class and transportation definition.");
    case Status::applied:
      break;
  }
  throw RTIinternalError(L"Umbra encountered an unknown Send Interaction planning outcome.");
}

[[noreturn]] void throwReceiveOrderDirectedInteractionFailure(
    umbra::detail::ReceiveOrderDirectedInteractionStatus status) {
  using Status = umbra::detail::ReceiveOrderDirectedInteractionStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::producing_federate_not_member:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    case Status::object_instance_not_known:
      throw ObjectInstanceNotKnown(
          L"Send Directed Interaction requires a target ObjectInstanceHandle known to this federate.");
    case Status::interaction_class_not_defined:
      throw InteractionClassNotDefined(
          L"The supplied InteractionClassHandle is not defined in this federation execution.");
    case Status::interaction_class_not_published:
      throw InteractionClassNotPublished(
          L"Send Directed Interaction requires publication for the target object class.");
    case Status::interaction_parameter_not_defined:
      throw InteractionParameterNotDefined(
          L"The supplied ParameterHandle is not defined for the directed interaction class.");
    case Status::inconsistent_catalog:
      throw RTIinternalError(
          L"Umbra could not resolve a coherent directed interaction and transportation definition.");
    case Status::applied:
      break;
  }
  throw RTIinternalError(
      L"Umbra encountered an unknown Send Directed Interaction planning outcome.");
}

[[noreturn]] void throwReceiveOrderAttributeUpdateFailure(
    umbra::detail::ReceiveOrderAttributeUpdateStatus status) {
  using Status = umbra::detail::ReceiveOrderAttributeUpdateStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::producing_federate_not_member:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    case Status::object_instance_not_known:
      throw ObjectInstanceNotKnown(
          L"The supplied ObjectInstanceHandle is not known to this federate.");
    case Status::attribute_not_owned:
      throw AttributeNotOwned(
          L"Update Attribute Values requires ownership of every supplied attribute.");
    case Status::attribute_not_defined:
      throw AttributeNotDefined(
          L"The supplied AttributeHandle is not defined for this object instance.");
    case Status::inconsistent_catalog:
      throw RTIinternalError(
          L"Umbra could not resolve a coherent object attribute transportation definition.");
    case Status::applied:
      break;
  }
  throw RTIinternalError(
      L"Umbra encountered an unknown Update Attribute Values planning outcome.");
}

[[noreturn]] void throwAttributeValueUpdateRequestFailure(
    umbra::detail::AttributeValueUpdateRequestStatus status) {
  using Status = umbra::detail::AttributeValueUpdateRequestStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::requesting_federate_not_member:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    case Status::object_instance_not_known:
      throw ObjectInstanceNotKnown(
          L"The supplied ObjectInstanceHandle is not known to this federate.");
    case Status::attribute_not_defined:
      throw AttributeNotDefined(
          L"The supplied AttributeHandle is not defined for this object's known class.");
    case Status::inconsistent_catalog:
      throw RTIinternalError(
          L"Umbra could not resolve a coherent object-instance attribute request.");
    case Status::applied:
      break;
  }
  throw RTIinternalError(
      L"Umbra encountered an unknown Request Attribute Value Update planning outcome.");
}

[[noreturn]] void throwJoinedFederateMomAttributeValueUpdateFailure(
    umbra::detail::JoinedFederateMomAttributeValueUpdateStatus status) {
  using Status = umbra::detail::JoinedFederateMomAttributeValueUpdateStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::requesting_federate_not_member:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    case Status::object_instance_not_known:
      throw ObjectInstanceNotKnown(
          L"The supplied ObjectInstanceHandle is not known to this federate.");
    case Status::attribute_not_defined:
      throw AttributeNotDefined(
          L"The supplied AttributeHandle is not defined for this MOM object instance.");
    case Status::inconsistent_catalog:
      throw RTIinternalError(
          L"Umbra could not resolve a coherent joined-federate MOM attribute request.");
    case Status::applied:
    case Status::not_rti_owned_object:
      break;
  }
  throw RTIinternalError(
      L"Umbra encountered an unknown joined-federate MOM attribute request outcome.");
}

[[noreturn]] void throwAttributeValueUpdateClassRequestFailure(
    umbra::detail::AttributeValueUpdateClassRequestStatus status) {
  using Status = umbra::detail::AttributeValueUpdateClassRequestStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::requesting_federate_not_member:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    case Status::object_class_not_defined:
      throw ObjectClassNotDefined(
          L"The supplied ObjectClassHandle is not defined in this federation execution.");
    case Status::attribute_not_defined:
      throw AttributeNotDefined(
          L"The supplied AttributeHandle is not available at the requested object class.");
    case Status::invalid_region_context:
      throw InvalidRegionContext(
          L"The supplied request region dimensions are not available for the object class.");
    case Status::region_not_created_by_this_federate:
      throw RegionNotCreatedByThisFederate(
          L"A Request Attribute Value Update With Regions region was not created by this federate.");
    case Status::invalid_region:
      throw InvalidRegion(
          L"The supplied Request Attribute Value Update With Regions region is not a committed specification.");
    case Status::inconsistent_catalog:
      throw RTIinternalError(
          L"Umbra could not resolve a coherent object-class attribute request.");
    case Status::applied:
      break;
  }
  throw RTIinternalError(
      L"Umbra encountered an unknown object-class Request Attribute Value Update planning outcome.");
}

[[noreturn]] void throwAttributeOwnershipQueryFailure(
    umbra::detail::AttributeOwnershipQueryStatus status) {
  using Status = umbra::detail::AttributeOwnershipQueryStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::requesting_federate_not_member:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    case Status::object_instance_not_known:
      throw ObjectInstanceNotKnown(
          L"The supplied ObjectInstanceHandle is not known to this federate.");
    case Status::attribute_not_defined:
      throw AttributeNotDefined(
          L"The supplied AttributeHandle is not defined for this object's known class.");
    case Status::inconsistent_catalog:
      throw RTIinternalError(
          L"Umbra could not resolve a coherent 2025 attribute ownership query.");
    case Status::applied:
      break;
  }
  throw RTIinternalError(
      L"Umbra encountered an unknown Query Attribute Ownership planning outcome.");
}

[[noreturn]] void throwAttributeOwnershipCheckFailure(
    umbra::detail::AttributeOwnershipCheckStatus status) {
  using Status = umbra::detail::AttributeOwnershipCheckStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::requesting_federate_not_member:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    case Status::object_instance_not_known:
      throw ObjectInstanceNotKnown(
          L"The supplied ObjectInstanceHandle is not known to this federate.");
    case Status::attribute_not_defined:
      throw AttributeNotDefined(
          L"The supplied AttributeHandle is not defined for this object's known class.");
    case Status::inconsistent_catalog:
      throw RTIinternalError(
          L"Umbra could not resolve a coherent 2025 attribute ownership check.");
    case Status::applied:
      break;
  }
  throw RTIinternalError(
      L"Umbra encountered an unknown Is Attribute Owned By Federate planning outcome.");
}

[[noreturn]] void throwAttributeOwnershipAcquisitionIfAvailableFailure(
    umbra::detail::AttributeOwnershipAcquisitionIfAvailableStatus status) {
  using Status = umbra::detail::AttributeOwnershipAcquisitionIfAvailableStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::requesting_federate_not_member:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    case Status::object_instance_not_known:
      throw ObjectInstanceNotKnown(
          L"The supplied ObjectInstanceHandle is not known to this federate.");
    case Status::attribute_already_being_acquired:
      throw AttributeAlreadyBeingAcquired(
          L"Attribute Ownership Acquisition If Available is already pending for an attribute.");
    case Status::attribute_not_published:
      throw AttributeNotPublished(
          L"Attribute Ownership Acquisition If Available requires publication of every attribute.");
    case Status::object_class_not_published:
      throw ObjectClassNotPublished(
          L"Attribute Ownership Acquisition If Available requires publication of the known class.");
    case Status::federate_owns_attributes:
      throw FederateOwnsAttributes(
          L"Attribute Ownership Acquisition If Available cannot acquire an attribute already owned by this federate.");
    case Status::attribute_not_defined:
      throw AttributeNotDefined(
          L"The supplied AttributeHandle is not defined for this object's known class.");
    case Status::inconsistent_catalog:
      throw RTIinternalError(
          L"Umbra could not resolve a coherent 2025 attribute ownership acquisition state.");
    case Status::applied:
      break;
  }
  throw RTIinternalError(
      L"Umbra encountered an unknown Attribute Ownership Acquisition If Available planning outcome.");
}

[[noreturn]] void throwAttributeOwnershipAcquisitionFailure(
    umbra::detail::AttributeOwnershipAcquisitionStatus status) {
  using Status = umbra::detail::AttributeOwnershipAcquisitionStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::requesting_federate_not_member:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    case Status::object_instance_not_known:
      throw ObjectInstanceNotKnown(
          L"The supplied ObjectInstanceHandle is not known to this federate.");
    case Status::attribute_not_published:
      throw AttributeNotPublished(
          L"Attribute Ownership Acquisition requires publication of every attribute.");
    case Status::object_class_not_published:
      throw ObjectClassNotPublished(
          L"Attribute Ownership Acquisition requires publication of the known class.");
    case Status::federate_owns_attributes:
      throw FederateOwnsAttributes(
          L"Attribute Ownership Acquisition cannot acquire an attribute already owned by this federate.");
    case Status::attribute_not_defined:
      throw AttributeNotDefined(
          L"The supplied AttributeHandle is not defined for this object's known class.");
    case Status::inconsistent_catalog:
      throw RTIinternalError(
          L"Umbra could not resolve a coherent 2025 regular ownership-acquisition state.");
    case Status::applied:
      break;
  }
  throw RTIinternalError(
      L"Umbra encountered an unknown Attribute Ownership Acquisition planning outcome.");
}

[[noreturn]] void throwAttributeOwnershipReleaseDeniedFailure(
    umbra::detail::AttributeOwnershipReleaseDeniedStatus status) {
  using Status = umbra::detail::AttributeOwnershipReleaseDeniedStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::owning_federate_not_member:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    case Status::object_instance_not_known:
      throw ObjectInstanceNotKnown(
          L"The supplied ObjectInstanceHandle is not known to this federate.");
    case Status::attribute_not_owned:
      throw AttributeNotOwned(
          L"Attribute Ownership Release Denied requires ownership of every supplied attribute.");
    case Status::attribute_not_defined:
      throw AttributeNotDefined(
          L"The supplied AttributeHandle is not defined for this object's known class.");
    case Status::inconsistent_catalog:
      throw RTIinternalError(
          L"Umbra could not resolve a coherent 2025 ownership-release-denied state.");
    case Status::applied:
      break;
  }
  throw RTIinternalError(
      L"Umbra encountered an unknown Attribute Ownership Release Denied planning outcome.");
}

[[noreturn]] void throwAttributeOwnershipDivestitureIfWantedFailure(
    umbra::detail::AttributeOwnershipDivestitureIfWantedStatus status) {
  using Status = umbra::detail::AttributeOwnershipDivestitureIfWantedStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::divesting_federate_not_member:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    case Status::object_instance_not_known:
      throw ObjectInstanceNotKnown(
          L"The supplied ObjectInstanceHandle is not known to this federate.");
    case Status::attribute_not_owned:
      throw AttributeNotOwned(
          L"Attribute Ownership Divestiture If Wanted requires ownership of every supplied attribute.");
    case Status::attribute_not_defined:
      throw AttributeNotDefined(
          L"The supplied AttributeHandle is not defined for this object's known class.");
    case Status::inconsistent_catalog:
      throw RTIinternalError(
          L"Umbra could not resolve a coherent 2025 ownership-divestiture-if-wanted state.");
    case Status::applied:
      break;
  }
  throw RTIinternalError(
      L"Umbra encountered an unknown Attribute Ownership Divestiture If Wanted planning outcome.");
}

[[noreturn]] void throwUnconditionalAttributeOwnershipDivestitureFailure(
    umbra::detail::UnconditionalAttributeOwnershipDivestitureStatus status) {
  using Status = umbra::detail::UnconditionalAttributeOwnershipDivestitureStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::divesting_federate_not_member:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    case Status::object_instance_not_known:
      throw ObjectInstanceNotKnown(
          L"The supplied ObjectInstanceHandle is not known to this federate.");
    case Status::attribute_not_owned:
      throw AttributeNotOwned(
          L"Unconditional Attribute Ownership Divestiture requires ownership of every supplied attribute.");
    case Status::attribute_not_defined:
      throw AttributeNotDefined(
          L"The supplied AttributeHandle is not defined for this object's known class.");
    case Status::inconsistent_catalog:
      throw RTIinternalError(
          L"Umbra could not resolve a coherent 2025 unconditional ownership-divestiture state.");
    case Status::applied:
      break;
  }
  throw RTIinternalError(
      L"Umbra encountered an unknown Unconditional Attribute Ownership Divestiture planning outcome.");
}

[[noreturn]] void throwNegotiatedAttributeOwnershipDivestitureFailure(
    umbra::detail::NegotiatedAttributeOwnershipDivestitureStatus status) {
  using Status = umbra::detail::NegotiatedAttributeOwnershipDivestitureStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::divesting_federate_not_member:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    case Status::object_instance_not_known:
      throw ObjectInstanceNotKnown(
          L"The supplied ObjectInstanceHandle is not known to this federate.");
    case Status::attribute_not_owned:
      throw AttributeNotOwned(
          L"Negotiated Attribute Ownership Divestiture requires ownership of every supplied attribute.");
    case Status::attribute_not_defined:
      throw AttributeNotDefined(
          L"The supplied AttributeHandle is not defined for this object's known class.");
    case Status::attribute_already_being_divested:
      throw AttributeAlreadyBeingDivested(
          L"The supplied AttributeHandle is already in negotiated divestiture.");
    case Status::inconsistent_catalog:
      throw RTIinternalError(
          L"Umbra could not resolve a coherent 2025 negotiated-divestiture state.");
    case Status::applied:
      break;
  }
  throw RTIinternalError(
      L"Umbra encountered an unknown Negotiated Attribute Ownership Divestiture planning outcome.");
}

[[noreturn]] void throwConfirmDivestitureFailure(
    umbra::detail::ConfirmDivestitureStatus status) {
  using Status = umbra::detail::ConfirmDivestitureStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::divesting_federate_not_member:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    case Status::object_instance_not_known:
      throw ObjectInstanceNotKnown(
          L"The supplied ObjectInstanceHandle is not known to this federate.");
    case Status::attribute_not_owned:
      throw AttributeNotOwned(
          L"Confirm Divestiture requires ownership of every supplied attribute.");
    case Status::attribute_not_defined:
      throw AttributeNotDefined(
          L"The supplied AttributeHandle is not defined for this object's known class.");
    case Status::attribute_divestiture_was_not_requested:
      throw AttributeDivestitureWasNotRequested(
          L"Confirm Divestiture requires a prior Request Divestiture Confirmation callback.");
    case Status::no_acquisition_pending:
      throw NoAcquisitionPending(
          L"Confirm Divestiture requires a still-pending regular acquisition.");
    case Status::inconsistent_catalog:
      throw RTIinternalError(
          L"Umbra could not resolve a coherent 2025 Confirm Divestiture state.");
    case Status::applied:
      break;
  }
  throw RTIinternalError(
      L"Umbra encountered an unknown Confirm Divestiture planning outcome.");
}

[[noreturn]] void throwCancelNegotiatedAttributeOwnershipDivestitureFailure(
    umbra::detail::CancelNegotiatedAttributeOwnershipDivestitureStatus status) {
  using Status = umbra::detail::CancelNegotiatedAttributeOwnershipDivestitureStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::divesting_federate_not_member:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    case Status::object_instance_not_known:
      throw ObjectInstanceNotKnown(
          L"The supplied ObjectInstanceHandle is not known to this federate.");
    case Status::attribute_not_owned:
      throw AttributeNotOwned(
          L"Cancel Negotiated Attribute Ownership Divestiture requires ownership of every supplied attribute.");
    case Status::attribute_not_defined:
      throw AttributeNotDefined(
          L"The supplied AttributeHandle is not defined for this object's known class.");
    case Status::attribute_divestiture_was_not_requested:
      throw AttributeDivestitureWasNotRequested(
          L"Cancel Negotiated Attribute Ownership Divestiture requires a pending divestiture.");
    case Status::inconsistent_catalog:
      throw RTIinternalError(
          L"Umbra could not resolve a coherent 2025 negotiated-divestiture cancellation state.");
    case Status::applied:
      break;
  }
  throw RTIinternalError(
      L"Umbra encountered an unknown Cancel Negotiated Attribute Ownership Divestiture planning outcome.");
}

[[noreturn]] void throwAttributeOwnershipAcquisitionCancellationFailure(
    umbra::detail::AttributeOwnershipAcquisitionCancellationStatus status) {
  using Status = umbra::detail::AttributeOwnershipAcquisitionCancellationStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::requesting_federate_not_member:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    case Status::object_instance_not_known:
      throw ObjectInstanceNotKnown(
          L"The supplied ObjectInstanceHandle is not known to this federate.");
    case Status::attribute_acquisition_was_not_requested:
      throw AttributeAcquisitionWasNotRequested(
          L"Cancel Attribute Ownership Acquisition requires a pending regular acquisition.");
    case Status::attribute_already_owned:
      throw AttributeAlreadyOwned(
          L"Cancel Attribute Ownership Acquisition cannot cancel an attribute already owned by this federate.");
    case Status::attribute_not_defined:
      throw AttributeNotDefined(
          L"The supplied AttributeHandle is not defined for this object's known class.");
    case Status::inconsistent_catalog:
      throw RTIinternalError(
          L"Umbra could not resolve a coherent 2025 ownership-acquisition cancellation state.");
    case Status::applied:
      break;
  }
  throw RTIinternalError(
      L"Umbra encountered an unknown Cancel Attribute Ownership Acquisition planning outcome.");
}

[[noreturn]] void throwAttributeTransportationTypeChangeFailure(
    umbra::detail::AttributeTransportationTypeChangeStatus status) {
  using Status = umbra::detail::AttributeTransportationTypeChangeStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::requesting_federate_not_member:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    case Status::object_instance_not_known:
      throw ObjectInstanceNotKnown(
          L"The supplied ObjectInstanceHandle is not known to this federate.");
    case Status::attribute_already_being_changed:
      throw AttributeAlreadyBeingChanged(
          L"An attribute transportation type change is already pending.");
    case Status::attribute_not_owned:
      throw AttributeNotOwned(
          L"Request Attribute Transportation Type Change requires ownership of every supplied attribute.");
    case Status::attribute_not_defined:
      throw AttributeNotDefined(
          L"The supplied AttributeHandle is not defined for this object's known class.");
    case Status::invalid_transportation_type:
      throw InvalidTransportationTypeHandle(
          L"The supplied TransportationTypeHandle is not supported by the embedded 2025 profile.");
    case Status::callback_route_missing:
    case Status::inconsistent_catalog:
      throw RTIinternalError(
          L"Umbra could not resolve a coherent attribute transportation type change.");
    case Status::applied:
      break;
  }
  throw RTIinternalError(
      L"Umbra encountered an unknown attribute transportation type change outcome.");
}

[[noreturn]] void throwAttributeTransportationTypeDefaultFailure(
    umbra::detail::AttributeTransportationTypeDefaultStatus status) {
  using Status = umbra::detail::AttributeTransportationTypeDefaultStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::requesting_federate_not_member:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    case Status::object_class_not_defined:
      throw ObjectClassNotDefined(
          L"The supplied ObjectClassHandle is not defined in this federation execution.");
    case Status::attribute_not_defined:
      throw AttributeNotDefined(
          L"The supplied AttributeHandle is not defined for this object class.");
    case Status::invalid_transportation_type:
      throw InvalidTransportationTypeHandle(
          L"The supplied TransportationTypeHandle is not supported by the embedded 2025 profile.");
    case Status::inconsistent_catalog:
      throw RTIinternalError(
          L"Umbra could not resolve a coherent attribute transportation type default.");
    case Status::applied:
      break;
  }
  throw RTIinternalError(
      L"Umbra encountered an unknown attribute transportation type default outcome.");
}

[[noreturn]] void throwAttributeTransportationTypeQueryFailure(
    umbra::detail::AttributeTransportationTypeQueryStatus status) {
  using Status = umbra::detail::AttributeTransportationTypeQueryStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::requesting_federate_not_member:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    case Status::object_instance_not_known:
      throw ObjectInstanceNotKnown(
          L"The supplied ObjectInstanceHandle is not known to this federate.");
    case Status::attribute_not_defined:
      throw AttributeNotDefined(
          L"The supplied AttributeHandle is not defined for this object's known class.");
    case Status::callback_route_missing:
    case Status::inconsistent_catalog:
      throw RTIinternalError(
          L"Umbra could not resolve a coherent attribute transportation type query.");
    case Status::applied:
      break;
  }
  throw RTIinternalError(
      L"Umbra encountered an unknown attribute transportation type query outcome.");
}

[[noreturn]] void throwInteractionTransportationTypeChangeFailure(
    umbra::detail::InteractionTransportationTypeChangeStatus status) {
  using Status = umbra::detail::InteractionTransportationTypeChangeStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::requesting_federate_not_member:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    case Status::interaction_class_already_being_changed:
      throw InteractionClassAlreadyBeingChanged(
          L"An interaction transportation type change is already pending.");
    case Status::interaction_class_not_published:
      throw InteractionClassNotPublished(
          L"Request Interaction Transportation Type Change requires publication of the interaction class.");
    case Status::interaction_class_not_defined:
      throw InteractionClassNotDefined(
          L"The supplied InteractionClassHandle is not defined in this federation execution.");
    case Status::invalid_transportation_type:
      throw InvalidTransportationTypeHandle(
          L"The supplied TransportationTypeHandle is not supported by the embedded 2025 profile.");
    case Status::callback_route_missing:
    case Status::inconsistent_catalog:
      throw RTIinternalError(
          L"Umbra could not resolve a coherent interaction transportation type change.");
    case Status::applied:
      break;
  }
  throw RTIinternalError(
      L"Umbra encountered an unknown interaction transportation type change outcome.");
}

[[noreturn]] void throwInteractionTransportationTypeQueryFailure(
    umbra::detail::InteractionTransportationTypeQueryStatus status) {
  using Status = umbra::detail::InteractionTransportationTypeQueryStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::requesting_federate_not_member:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    case Status::interaction_class_not_defined:
      throw InteractionClassNotDefined(
          L"The supplied InteractionClassHandle is not defined in this federation execution.");
    case Status::callback_route_missing:
    case Status::inconsistent_catalog:
      throw RTIinternalError(
          L"Umbra could not resolve a coherent interaction transportation type query.");
    case Status::applied:
      break;
  }
  throw RTIinternalError(
      L"Umbra encountered an unknown interaction transportation type query outcome.");
}

[[noreturn]] void throwAttributeOrderTypeChangeFailure(
    umbra::detail::AttributeOrderTypeChangeStatus status) {
  using Status = umbra::detail::AttributeOrderTypeChangeStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::requesting_federate_not_member:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    case Status::object_instance_not_known:
      throw ObjectInstanceNotKnown(
          L"The supplied ObjectInstanceHandle is not known to this federate.");
    case Status::attribute_not_owned:
      throw AttributeNotOwned(
          L"Change Attribute Order Type requires ownership of every supplied attribute.");
    case Status::attribute_not_defined:
      throw AttributeNotDefined(
          L"The supplied AttributeHandle is not defined for this object's known class.");
    case Status::invalid_order_type:
      throw RTIinternalError(
          L"The supplied OrderType is not supported by the embedded 2025 profile.");
    case Status::applied:
      break;
  }
  throw RTIinternalError(L"Umbra encountered an unknown attribute order type change outcome.");
}

[[noreturn]] void throwAttributeOrderTypeDefaultFailure(
    umbra::detail::AttributeOrderTypeDefaultStatus status) {
  using Status = umbra::detail::AttributeOrderTypeDefaultStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::requesting_federate_not_member:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    case Status::object_class_not_defined:
      throw ObjectClassNotDefined(
          L"The supplied ObjectClassHandle is not defined in this federation execution.");
    case Status::attribute_not_defined:
      throw AttributeNotDefined(
          L"The supplied AttributeHandle is not defined for this object class.");
    case Status::invalid_order_type:
      throw RTIinternalError(
          L"The supplied OrderType is not supported by the embedded 2025 profile.");
    case Status::applied:
      break;
  }
  throw RTIinternalError(L"Umbra encountered an unknown attribute order type default outcome.");
}

[[noreturn]] void throwInteractionOrderTypeChangeFailure(
    umbra::detail::InteractionOrderTypeChangeStatus status) {
  using Status = umbra::detail::InteractionOrderTypeChangeStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::requesting_federate_not_member:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    case Status::interaction_class_not_published:
      throw InteractionClassNotPublished(
          L"Change Interaction Order Type requires publication of the interaction class.");
    case Status::interaction_class_not_defined:
      throw InteractionClassNotDefined(
          L"The supplied InteractionClassHandle is not defined in this federation execution.");
    case Status::invalid_order_type:
      throw RTIinternalError(
          L"The supplied OrderType is not supported by the embedded 2025 profile.");
    case Status::applied:
      break;
  }
  throw RTIinternalError(L"Umbra encountered an unknown interaction order type change outcome.");
}

[[noreturn]] void throwPreparationFailure(
    umbra::detail::FederationPreparationResult const& preparation) {
  using Status = umbra::detail::FederationPreparationStatus;
  switch (preparation.status) {
    case Status::fom_not_found:
      throw CouldNotOpenFOM(L"Umbra could not open a supplied FOM module.");
    case Status::fom_unreadable:
    case Status::fom_parse_error:
      throw ErrorReadingFOM(L"Umbra could not safely read a supplied FOM module.");
    case Status::invalid_fom:
      throw InvalidFOM(L"A supplied FOM module is not valid against the selected IEEE schema.");
    case Status::mim_not_found:
      throw CouldNotOpenMIM(L"Umbra could not open the selected MIM module.");
    case Status::mim_unreadable:
    case Status::mim_parse_error:
      throw ErrorReadingMIM(L"Umbra could not safely read the selected MIM module.");
    case Status::invalid_mim:
      throw InvalidMIM(L"The selected MIM module is not valid against the selected IEEE schema.");
    case Status::standard_mim_designator_supplied:
      throw DesignatorIsHLAstandardMIM(
          L"A supplied MIM designator must not be HLAstandardMIM.");
    case Status::inconsistent_fom:
      throw InconsistentFOM(
          L"The supplied FOM/MIM modules or documented time representation cannot form one FDD.");
    case Status::time_factory_unavailable:
      throw CouldNotCreateLogicalTimeFactory(
          L"Umbra could not create the requested logical-time factory.");
    case Status::applied:
    case Status::backend_failure:
      throw RTIinternalError(
          L"Umbra could not prepare the federation-management request in the embedded backend.");
  }
  throw RTIinternalError(L"Umbra encountered an unknown federation preparation outcome.");
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

void submitTimeAdvanceGrantDispatches(
    std::vector<umbra::detail::FederationTimeGrantDispatch> dispatches) {
  for (auto& dispatch : dispatches) {
    if (dispatch) {
      dispatch();
    }
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

std::optional<std::set<std::uint64_t>> interactionClassHandleValues(
    InteractionClassHandleSet const& interactionClasses) {
  std::set<std::uint64_t> result;
  for (InteractionClassHandle const& interactionClass : interactionClasses) {
    auto const value = interactionClassHandleValue(interactionClass);
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

DimensionHandleSet makeDimensionHandleSet(std::set<std::uint64_t> const& handles) {
  DimensionHandleSet result;
  for (std::uint64_t const handle : handles) {
    result.insert(makeDimensionHandle(handle));
  }
  return result;
}

std::vector<unsigned char> copyVariableLengthDataBytes(VariableLengthData const& value) {
  auto const* bytes = static_cast<unsigned char const*>(value.data());
  if (bytes == nullptr || value.size() == 0) {
    return {};
  }
  return {bytes, bytes + value.size()};
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
std::optional<bool> decodeHlaSwitch(VariableLengthData const& value) {
  auto const* bytes = static_cast<unsigned char const*>(value.data());
  if (bytes == nullptr || value.size() != 4) {
    return std::nullopt;
  }
  std::uint32_t const encoded =
      (static_cast<std::uint32_t>(bytes[0]) << 24U) |
      (static_cast<std::uint32_t>(bytes[1]) << 16U) |
      (static_cast<std::uint32_t>(bytes[2]) << 8U) |
      static_cast<std::uint32_t>(bytes[3]);
  if (encoded > 1U) {
    return std::nullopt;
  }
  return encoded == 1U;
}

// HLAstandardMIM represents HLAresignAction with HLAinteger32BE values zero
// through five.  Decode by the MIM vocabulary rather than relying on an enum
// cast so malformed incoming MOM payloads cannot silently alter a federate's
// automatic-resign policy.
std::optional<ResignAction> decodeHlaResignAction(VariableLengthData const& value) {
  auto const* bytes = static_cast<unsigned char const*>(value.data());
  if (bytes == nullptr || value.size() != 4) {
    return std::nullopt;
  }
  std::uint32_t const encoded =
      (static_cast<std::uint32_t>(bytes[0]) << 24U) |
      (static_cast<std::uint32_t>(bytes[1]) << 16U) |
      (static_cast<std::uint32_t>(bytes[2]) << 8U) |
      static_cast<std::uint32_t>(bytes[3]);
  switch (encoded) {
    case 0U:
      return UNCONDITIONALLY_DIVEST_ATTRIBUTES;
    case 1U:
      return DELETE_OBJECTS;
    case 2U:
      return CANCEL_PENDING_OWNERSHIP_ACQUISITIONS;
    case 3U:
      return DELETE_OBJECTS_THEN_DIVEST;
    case 4U:
      return CANCEL_THEN_DELETE_THEN_DIVEST;
    case 5U:
      return NO_ACTION;
    default:
      return std::nullopt;
  }
}

std::optional<std::vector<std::uint64_t>> attributeValueHandleValues(
    AttributeHandleValueMap const& attributeValues) {
  std::vector<std::uint64_t> result;
  result.reserve(attributeValues.size());
  for (auto const& [attributeHandle, attributeValue] : attributeValues) {
    static_cast<void>(attributeValue);
    auto const value = attributeHandleValue(attributeHandle);
    if (!value) {
      return std::nullopt;
    }
    result.push_back(*value);
  }
  return result;
}

std::vector<AttributeValue> copyAttributeValues(
    AttributeHandleValueMap const& attributeValues) {
  std::vector<AttributeValue> result;
  result.reserve(attributeValues.size());
  for (auto const& [attributeHandle, attributeValue] : attributeValues) {
    auto const value = attributeHandleValue(attributeHandle);
    if (!value) {
      throw RTIinternalError(
          L"The Update Attribute Values map changed while Umbra was copying its values.");
    }
    result.emplace_back(*value, attributeValue);
  }
  return result;
}

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
      mayDeliver = embeddedFederationManagement().registry()
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
    bool directed) {
  std::scoped_lock lock(federationManagementMutex());
  static_cast<void>(embeddedFederationManagement().registry()
                        .recordSuccessfulInteractionReceipt(
                            federationName,
                            receivingFederateId,
                            directed));
}

void recordSuccessfulReflectionReceipt(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId) {
  std::scoped_lock lock(federationManagementMutex());
  static_cast<void>(embeddedFederationManagement().registry()
                        .recordSuccessfulReflectionReceipt(
                            federationName,
                            receivingFederateId));
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

void flushAsynchronousReceiveCallbacks(
    std::shared_ptr<umbra::detail::FederateTimeState> const& timeState) {
  if (!timeState) {
    return;
  }
  auto callbacks = timeState->takeEligibleAsynchronousReceiveCallbacks();
  for (auto& callback : callbacks) {
    if (callback) {
      callback();
    }
  }
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

VariableLengthData copySynchronizationPointTag(
    std::vector<unsigned char> const& tag) {
  return VariableLengthData(
      tag.empty() ? nullptr : static_cast<void const*>(tag.data()),
      tag.size());
}

void submitSynchronizationPointAnnouncements(
    std::vector<umbra::detail::SynchronizationPointAnnouncement> announcements) {
  for (auto& announcement : announcements) {
    if (!announcement.callbackRoute) {
      continue;
    }
    if (announcement.serviceReportRoute) {
      // §4.16 is an RTI-initiated service at each receiving joined federate.
      // Append that federate's selected-file record before its callback is
      // queued, preserving the per-recipient serial sequence and HLA_EVOKED
      // observation boundary.
      announcement.serviceReportRoute(
          static_cast<std::uint16_t>(
              umbra::detail::MomServiceType::federation_management),
          [
              label = announcement.label,
              userSuppliedTag = announcement.userSuppliedTag](
              std::uint32_t serialNumber) {
            auto tag = copySynchronizationPointTag(userSuppliedTag);
            return umbra::detail::formatMomSuccessfulVoidServiceReportRecord(
                serialNumber,
                L"AnnounceSynchronizationPoint",
                {{umbra::detail::MomArgumentType::string,
                  L"Synchronization point label",
                  umbra::detail::formatMomString(label)},
                 {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
                  L"User-supplied tag",
                  umbra::detail::formatMomUserSuppliedTag(tag)}});
          });
    }
    announcement.callbackRoute([
        label = std::move(announcement.label),
        userSuppliedTag = std::move(announcement.userSuppliedTag)](
        FederateAmbassador& recipient) mutable {
      auto tag = copySynchronizationPointTag(userSuppliedTag);
      recipient.announceSynchronizationPoint(label, tag);
    });
  }
}

void submitFederationSynchronizedNotifications(
    std::vector<umbra::detail::FederationSynchronizedNotification> notifications) {
  for (auto& notification : notifications) {
    if (!notification.callbackRoute) {
      continue;
    }
    if (notification.serviceReportRoute) {
      // §4.18, like §4.16, is an RTI-initiated service at every recipient.
      // Preserve each joined federate's own selection and serial sequence
      // before the corresponding C++ callback can be evoked.
      notification.serviceReportRoute(
          static_cast<std::uint16_t>(
              umbra::detail::MomServiceType::federation_management),
          [
              label = notification.label,
              failedToSyncFederateIds = notification.failedToSyncFederateIds](
              std::uint32_t serialNumber) {
            FederateHandleSet failedToSyncSet;
            for (std::uint64_t federateId : failedToSyncFederateIds) {
              failedToSyncSet.insert(makeFederateHandle(federateId));
            }
            return umbra::detail::formatMomSuccessfulVoidServiceReportRecord(
                serialNumber,
                L"FederationSynchronized",
                {{umbra::detail::MomArgumentType::string,
                  L"Synchronization point label",
                  umbra::detail::formatMomString(label)},
                 {umbra::detail::MomArgumentType::federate_handle_set,
                  L"Set of joined federate designators",
                  umbra::detail::formatMomFederateHandleSet(failedToSyncSet)}});
          });
    }
    notification.callbackRoute([
        label = std::move(notification.label),
        failedToSyncFederateIds = std::move(notification.failedToSyncFederateIds)](
        FederateAmbassador& recipient) mutable {
      FederateHandleSet failedToSyncSet;
      for (std::uint64_t federateId : failedToSyncFederateIds) {
        failedToSyncSet.insert(makeFederateHandle(federateId));
      }
      recipient.federationSynchronized(label, failedToSyncSet);
    });
  }
}

enum class FederationSaveServiceFailure {
  request,
  begun,
  completion,
  abort,
  query,
};

[[noreturn]] void throwFederationSaveServiceFailure(
    umbra::detail::FederationSaveControlStatus status,
    std::wstring const& operation,
    FederationSaveServiceFailure failure) {
  using Status = umbra::detail::FederationSaveControlStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::federate_not_member:
      throw FederateNotExecutionMember(
          operation + L" requires membership in an active federation execution.");
    case Status::callback_route_missing:
      throw RTIinternalError(
          operation + L" could not find the joined federate's callback route.");
    case Status::invalid_timed_save:
      throw RTIinternalError(
          operation + L" could not commit a coherent timestamped save request.");
    case Status::inconsistent_temporal_state:
      throw RTIinternalError(
          operation + L" could not observe a coherent federation time state.");
    case Status::save_in_progress:
      if (failure == FederationSaveServiceFailure::request) {
        throw SaveInProgress(operation + L" cannot overlap an existing federation save.");
      }
      break;
    case Status::restore_in_progress:
      throw RestoreInProgress(operation + L" cannot overlap an active federation restore.");
    case Status::save_not_initiated:
      if (failure == FederationSaveServiceFailure::begun) {
        throw SaveNotInitiated(
            operation + L" was not preceded by an Initiate Federate Save callback.");
      }
      break;
    case Status::federate_has_not_begun_save:
      if (failure == FederationSaveServiceFailure::completion) {
        throw FederateHasNotBegunSave(
            operation + L" requires the federate to report Save Begun first.");
      }
      break;
    case Status::save_not_in_progress:
      if (failure == FederationSaveServiceFailure::abort) {
        throw SaveNotInProgress(operation + L" has no federation save to abort.");
      }
      break;
    case Status::applied:
      break;
  }
  throw RTIinternalError(operation + L" encountered an unknown save-control outcome.");
}

// Save/restore notification submission is defined before the generic MOM
// queue helpers below.  Keep this narrow declaration here so callback
// delivery can publish the event-time HLAfederateState transition without
// changing the public API or coupling the registry to the ambassador.
void queueJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
    std::wstring const& federationName,
    std::uint64_t joinedFederateId,
    std::vector<std::string_view> attributeNames,
    std::optional<std::uint64_t> excludedReceivingFederateId = std::nullopt);

void submitFederationSaveNotifications(
    std::wstring const& federationName,
    std::vector<umbra::detail::FederationSaveNotification> notifications) {
  for (auto& notification : notifications) {
    if (!notification.callbackRoute) {
      continue;
    }
    switch (notification.kind) {
      case umbra::detail::FederationSaveNotificationKind::initiate:
        if (notification.serviceReportRoute) {
          // §4.20 is an RTI-initiated service at each recipient.  The
          // recipient-owned route preserves its selected file and serial
          // sequence before HLA_EVOKED work can expose Initiate Federate Save.
          notification.serviceReportRoute(
              static_cast<std::uint16_t>(
                  umbra::detail::MomServiceType::federation_management),
              [
                  label = notification.label,
                  timestamp = notification.timestamp](std::uint32_t serialNumber) {
                std::vector<umbra::detail::MomServiceArgument> suppliedArguments{
                    {umbra::detail::MomArgumentType::string,
                     L"Federation save label",
                     umbra::detail::formatMomString(label)},
                };
                if (timestamp) {
                  suppliedArguments.push_back({
                      umbra::detail::MomArgumentType::logical_time,
                      L"Optional timestamp",
                      umbra::detail::formatMomLogicalTime(*timestamp),
                  });
                } else {
                  suppliedArguments.push_back({
                      umbra::detail::MomArgumentType::null_value,
                      L"Optional timestamp",
                      umbra::detail::formatMomNull(),
                  });
                }
                return umbra::detail::formatMomSuccessfulVoidServiceReportRecord(
                    serialNumber,
                    L"InitiateFederateSave",
                    suppliedArguments);
              });
        }
        notification.callbackRoute([
            federationName,
            receivingFederateId = notification.receivingFederateId,
            label = std::move(notification.label),
            timestamp = std::move(notification.timestamp)](FederateAmbassador& recipient) {
          if (timestamp) {
            recipient.initiateFederateSave(label, *timestamp);
          } else {
            recipient.initiateFederateSave(label);
          }
          queueJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
              federationName,
              receivingFederateId,
              {"HLAfederateState"},
              receivingFederateId);
        });
        break;
      case umbra::detail::FederationSaveNotificationKind::completed:
        if (notification.serviceReportRoute) {
          // §4.23 is RTI-initiated at every joined federate that received the
          // corresponding save instruction. Append the selected recipient's
          // result form before its callback is queued.
          notification.serviceReportRoute(
              static_cast<std::uint16_t>(
                  umbra::detail::MomServiceType::federation_management),
              [
                  successful = notification.successful,
                  failureReason = notification.failureReason](std::uint32_t serialNumber) {
                std::vector<umbra::detail::MomServiceArgument> suppliedArguments{
                    {umbra::detail::MomArgumentType::boolean,
                     L"Federation save-success indicator",
                     umbra::detail::formatMomBoolean(successful)},
                };
                if (successful) {
                  suppliedArguments.push_back({
                      umbra::detail::MomArgumentType::null_value,
                      L"Optional failure reason",
                      umbra::detail::formatMomNull(),
                  });
                } else {
                  suppliedArguments.push_back({
                      umbra::detail::MomArgumentType::save_failure_reason,
                      L"Optional failure reason",
                      umbra::detail::formatMomSaveFailureReason(failureReason),
                  });
                }
                return umbra::detail::formatMomSuccessfulVoidServiceReportRecord(
                    serialNumber,
                    L"FederationSaved",
                    suppliedArguments);
              });
        }
        if (notification.successful) {
          notification.callbackRoute([
              federationName,
              receivingFederateId = notification.receivingFederateId](
              FederateAmbassador& recipient) {
            recipient.federationSaved();
            queueJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
                federationName,
                receivingFederateId,
                {"HLAfederateState"});
          });
        } else {
          auto const reason = notification.failureReason;
          notification.callbackRoute([reason](FederateAmbassador& recipient) {
            recipient.federationNotSaved(reason);
          });
        }
        break;
      case umbra::detail::FederationSaveNotificationKind::status:
        if (notification.serviceReportRoute) {
          // §4.26 is RTI-initiated at the querying joined federate. Preserve
          // the exact Table 5 status-pair array in its selected report file
          // before the callback can expose the same response.
          notification.serviceReportRoute(
              static_cast<std::uint16_t>(
                  umbra::detail::MomServiceType::federation_management),
              [statuses = notification.statuses](std::uint32_t serialNumber) {
                FederateHandleSaveStatusPairVector response;
                response.reserve(statuses.size());
                for (auto const& [federateId, status] : statuses) {
                  response.emplace_back(makeFederateHandle(federateId), status);
                }
                return umbra::detail::formatMomSuccessfulVoidServiceReportRecord(
                    serialNumber,
                    L"FederationSaveStatusResponse",
                    {{umbra::detail::MomArgumentType::federate_handle_save_status_pair_set,
                      L"List of joined federates and save status for each",
                      umbra::detail::formatMomFederateHandleSaveStatusPairVector(response)}});
              });
        }
        notification.callbackRoute([
            statuses = std::move(notification.statuses)](FederateAmbassador& recipient) {
          FederateHandleSaveStatusPairVector response;
          response.reserve(statuses.size());
          for (auto const& [federateId, status] : statuses) {
            response.emplace_back(makeFederateHandle(federateId), status);
          }
          recipient.federationSaveStatusResponse(response);
        });
        break;
    }
  }
}

enum class FederationRestoreServiceFailure {
  request,
  completion,
  abort,
  query,
};

[[noreturn]] void throwFederationRestoreServiceFailure(
    umbra::detail::FederationRestoreControlStatus status,
    std::wstring const& operation,
    FederationRestoreServiceFailure failure) {
  using Status = umbra::detail::FederationRestoreControlStatus;
  switch (status) {
    case Status::federation_does_not_exist:
    case Status::federate_not_member:
      throw FederateNotExecutionMember(
          operation + L" requires membership in an active federation execution.");
    case Status::save_in_progress:
      throw SaveInProgress(operation + L" cannot overlap an active federation save.");
    case Status::restore_in_progress:
      if (failure == FederationRestoreServiceFailure::request) {
        throw RestoreInProgress(operation + L" cannot overlap an active federation restore.");
      }
      break;
    case Status::restore_not_requested:
      if (failure == FederationRestoreServiceFailure::completion) {
        throw RestoreNotRequested(
            operation + L" requires an accepted federation restore request.");
      }
      break;
    case Status::restore_not_in_progress:
      if (failure == FederationRestoreServiceFailure::abort) {
        throw RestoreNotInProgress(operation + L" has no federation restore to abort.");
      }
      break;
    case Status::callback_route_missing:
      throw RTIinternalError(
          operation + L" could not find the joined federate's callback route.");
    case Status::snapshot_not_found:
    case Status::membership_mismatch:
    case Status::applied:
      break;
  }
  throw RTIinternalError(operation + L" encountered an unknown restore-control outcome.");
}

void submitFederationRestoreNotifications(
    std::wstring const& federationName,
    std::vector<umbra::detail::FederationRestoreNotification> notifications) {
  for (auto& notification : notifications) {
    if (!notification.callbackRoute) {
      continue;
    }
    switch (notification.kind) {
      case umbra::detail::FederationRestoreNotificationKind::request_succeeded:
        if (notification.serviceReportRoute) {
          // §4.28 is RTI-initiated at the requesting joined federate.  Write
          // its immutable selected-file result form before the success
          // callback can become observable in either callback model.
          notification.serviceReportRoute(
              static_cast<std::uint16_t>(
                  umbra::detail::MomServiceType::federation_management),
              [label = notification.label](std::uint32_t serialNumber) {
                return umbra::detail::formatMomSuccessfulVoidServiceReportRecord(
                    serialNumber,
                    L"ConfirmFederationRestorationRequest",
                    {{umbra::detail::MomArgumentType::string,
                      L"Federation save label",
                      umbra::detail::formatMomString(label)},
                     {umbra::detail::MomArgumentType::boolean,
                      L"Request-success indicator",
                      umbra::detail::formatMomBoolean(true)}});
              });
        }
        notification.callbackRoute([
            federationName,
            receivingFederateId = notification.receivingFederateId,
            label = std::move(notification.label)](FederateAmbassador& recipient) {
          recipient.requestFederationRestoreSucceeded(label);
          // The successful Confirm Federation Restoration Request service is
          // one of the MIM-defined HLAfederateState update boundaries.  The
          // restore ledger is already in FederateRestoreInProgress here, so
          // capture that event-time value through the normal MOM seam.
          queueJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
              federationName,
              receivingFederateId,
              {"HLAfederateState"});
        });
        break;
      case umbra::detail::FederationRestoreNotificationKind::request_failed:
        if (notification.serviceReportRoute) {
          // A rejected restore request remains a normally returned §4.27
          // invocation, whose negative §4.28 result is reported before the
          // requester can receive requestFederationRestoreFailed().
          notification.serviceReportRoute(
              static_cast<std::uint16_t>(
                  umbra::detail::MomServiceType::federation_management),
              [label = notification.label](std::uint32_t serialNumber) {
                return umbra::detail::formatMomSuccessfulVoidServiceReportRecord(
                    serialNumber,
                    L"ConfirmFederationRestorationRequest",
                    {{umbra::detail::MomArgumentType::string,
                      L"Federation save label",
                      umbra::detail::formatMomString(label)},
                     {umbra::detail::MomArgumentType::boolean,
                      L"Request-success indicator",
                      umbra::detail::formatMomBoolean(false)}});
              });
        }
        notification.callbackRoute([
            label = std::move(notification.label)](FederateAmbassador& recipient) {
          recipient.requestFederationRestoreFailed(label);
        });
        break;
      case umbra::detail::FederationRestoreNotificationKind::begin:
        if (notification.serviceReportRoute) {
          // §4.29 is RTI-initiated at every joined federate, including the
          // requester. Each recipient's immutable selected-file route must
          // append the no-argument Table 5 form before the callback is queued.
          notification.serviceReportRoute(
              static_cast<std::uint16_t>(
                  umbra::detail::MomServiceType::federation_management),
              [](std::uint32_t serialNumber) {
                return umbra::detail::formatMomSuccessfulVoidServiceReportRecord(
                    serialNumber,
                    L"FederationRestoreBegun",
                    {});
              });
        }
        notification.callbackRoute([
            federationName,
            receivingFederateId = notification.receivingFederateId](
            FederateAmbassador& recipient) {
          recipient.federationRestoreBegun();
          queueJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
              federationName,
              receivingFederateId,
              {"HLAfederateState"});
        });
        break;
      case umbra::detail::FederationRestoreNotificationKind::initiate:
        if (notification.serviceReportRoute) {
          // §4.30 is RTI-initiated at each restoring joined federate.  Keep
          // the recipient's immutable selected-file route with this work so
          // the complete label/designator/name form precedes the callback.
          notification.serviceReportRoute(
              static_cast<std::uint16_t>(
                  umbra::detail::MomServiceType::federation_management),
              [label = notification.label,
               federateName = notification.federateName,
               postRestoreFederateId = notification.postRestoreFederateId](
                  std::uint32_t serialNumber) {
                return umbra::detail::formatMomSuccessfulVoidServiceReportRecord(
                    serialNumber,
                    L"InitiateFederateRestore",
                    {{umbra::detail::MomArgumentType::string,
                      L"Federation save label",
                      umbra::detail::formatMomString(label)},
                     {umbra::detail::MomArgumentType::federate_handle,
                      L"Joined federate designator",
                      umbra::detail::formatMomFederateHandle(
                          makeFederateHandle(postRestoreFederateId))},
                     {umbra::detail::MomArgumentType::string,
                      L"Federate name",
                      umbra::detail::formatMomString(federateName)}});
              });
        }
        notification.callbackRoute([
            federationName,
            receivingFederateId = notification.receivingFederateId,
            label = std::move(notification.label),
            federateName = std::move(notification.federateName),
            postRestoreFederateId = notification.postRestoreFederateId](
            FederateAmbassador& recipient) {
          recipient.initiateFederateRestore(
              label,
              federateName,
              makeFederateHandle(postRestoreFederateId));
          queueJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
              federationName,
              receivingFederateId,
              {"HLAfederateState"});
        });
        break;
      case umbra::detail::FederationRestoreNotificationKind::completed:
        if (notification.successful) {
          notification.callbackRoute([
              federationName,
              receivingFederateId = notification.receivingFederateId](
              FederateAmbassador& recipient) {
            recipient.federationRestored();
            queueJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
                federationName,
                receivingFederateId,
                {"HLAfederateState"});
          });
        } else {
          auto const reason = notification.failureReason;
          notification.callbackRoute([reason](FederateAmbassador& recipient) {
            recipient.federationNotRestored(reason);
          });
        }
        break;
      case umbra::detail::FederationRestoreNotificationKind::status:
        if (notification.serviceReportRoute) {
          // §4.35 is RTI-initiated at the querying joined federate. Preserve
          // the selected file's serial sequence and the exact descriptor
          // vector before the callback can expose that response. The type-20
          // MIM identity reconciles Table 5's malformed collection row.
          notification.serviceReportRoute(
              static_cast<std::uint16_t>(
                  umbra::detail::MomServiceType::federation_management),
              [statuses = notification.statuses](std::uint32_t serialNumber) {
                FederateRestoreStatusVector response;
                response.reserve(statuses.size());
                for (auto const& status : statuses) {
                  response.emplace_back(
                      makeFederateHandle(status.preRestoreFederateId),
                      makeFederateHandle(status.postRestoreFederateId),
                      status.status);
                }
                return umbra::detail::formatMomSuccessfulVoidServiceReportRecord(
                    serialNumber,
                    L"FederationRestoreStatusResponse",
                    {{umbra::detail::MomArgumentType::federate_restore_status_set,
                      L"List of joined federates and restore status for each",
                      umbra::detail::formatMomFederateRestoreStatusVector(response)}});
              });
        }
        notification.callbackRoute([
            statuses = std::move(notification.statuses)](FederateAmbassador& recipient) {
          FederateRestoreStatusVector response;
          response.reserve(statuses.size());
          for (auto const& status : statuses) {
            response.emplace_back(
                makeFederateHandle(status.preRestoreFederateId),
                makeFederateHandle(status.postRestoreFederateId),
                status.status);
          }
          recipient.federationRestoreStatusResponse(response);
        });
        break;
    }
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
  submitSynchronizationPointAnnouncements(std::move(plan.announcements));
}

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

std::optional<std::string> embeddedTransportationNameFromHandle(
    TransportationTypeHandle const& transportationType) {
  auto const value = transportationTypeHandleValue(transportationType);
  auto const name = value ? standardTransportationTypeName(*value) : std::nullopt;
  if (!name) {
    return std::nullopt;
  }
  return umbra::detail::utf8FromWide(*name);
}

std::optional<OrderType> standardOrderTypeValue(std::wstring const& orderTypeName) {
  if (orderTypeName == L"Receive") {
    return RECEIVE;
  }
  if (orderTypeName == L"TimeStamp") {
    return TIMESTAMP;
  }
  return std::nullopt;
}

std::optional<std::wstring> standardOrderTypeName(OrderType orderType) {
  switch (orderType) {
    case RECEIVE:
      return L"Receive";
    case TIMESTAMP:
      return L"TimeStamp";
  }
  return std::nullopt;
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
        transportationHandleFromEmbeddedName(
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
        transportationHandleFromEmbeddedName(
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
        transportationHandleFromEmbeddedName(
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
        transportationHandleFromEmbeddedName(
            plan->transportationName,
            L"The embedded federation could not reconstruct an interaction transportation type report."));
  });
}

void queueReceiveOrderInteraction(
    umbra::detail::InteractionCallbackRoute callbackRoute,
    std::wstring federationName,
    umbra::detail::InteractionProducer producingSource,
    std::uint64_t receivingFederateId,
    std::uint64_t sentInteractionClassHandle,
    std::vector<InteractionParameterValue> sentParameters,
    VariableLengthData userSuppliedTag,
    TransportationTypeHandle transportationType,
    std::optional<std::set<std::uint64_t>> sentRegionHandles = std::nullopt,
    bool defaultRegionUsed = false) {
  auto const producingFederateId = producingSource.joinedFederateId();
  if (producingSource.kind() != umbra::detail::InteractionProducer::Kind::joined_federate ||
      !producingFederateId || *producingFederateId == 0U) {
    // The official callback requires a FederateHandle. Keep RTI-originated
    // MOM traffic out of this ordinary joined-federate delivery path until
    // its producer-designator rule is sourced; do not manufacture an invalid
    // FederateHandle(0) to make the callback type fit.
    throw RTIinternalError(
        L"The ordinary Receive Interaction callback path requires a joined-federate producer.");
  }
  std::vector<std::uint64_t> sentParameterHandles;
  sentParameterHandles.reserve(sentParameters.size());
  for (auto const& [parameterHandle, parameterValue] : sentParameters) {
    static_cast<void>(parameterValue);
    sentParameterHandles.push_back(parameterHandle);
  }

  submitReceiveOrderCallback(
      std::move(callbackRoute),
      federationName,
      receivingFederateId,
      [
      federationName,
      producingFederateId = *producingFederateId,
      receivingFederateId,
      sentInteractionClassHandle,
      sentParameterHandles = std::move(sentParameterHandles),
      sentParameters = std::move(sentParameters),
      userSuppliedTag = std::move(userSuppliedTag),
      transportationType = std::move(transportationType),
      sentRegionHandles = std::move(sentRegionHandles),
      defaultRegionUsed](FederateAmbassador& recipient) mutable {
    std::optional<umbra::detail::ReceiveOrderInteractionRecipient> projection;
    {
      std::scoped_lock lock(federationManagementMutex());
      projection = embeddedFederationManagement().registry().receiveOrderInteractionRecipientFor(
          federationName,
          producingFederateId,
          receivingFederateId,
          sentInteractionClassHandle,
          sentParameterHandles,
          sentRegionHandles ? &*sentRegionHandles : nullptr);
    }
    if (!projection) {
      return;
    }

    // The recipient can resign or change its subscription after a Send
    // Interaction is accepted.  The registry projection above is therefore
    // deliberately re-evaluated immediately before user callback delivery.
    ParameterHandleValueMap parameterValues = projectInteractionParameterValues(
        sentParameters,
        projection->receivedParameterHandles);
    std::optional<RegionHandleSet> optionalSentRegions;
    if (projection->conveyRegionDesignatorSets &&
        (sentRegionHandles || defaultRegionUsed)) {
      optionalSentRegions.emplace();
      if (sentRegionHandles) {
        for (std::uint64_t const regionHandle : *sentRegionHandles) {
          optionalSentRegions->insert(makeRegionHandle(regionHandle));
        }
      }
    }
    recordSuccessfulInteractionReceipt(federationName, receivingFederateId, false);
    recipient.receiveInteraction(
        makeInteractionClassHandle(projection->receivedInteractionClassHandle),
        parameterValues,
        userSuppliedTag,
        transportationType,
        makeFederateHandle(producingFederateId),
        optionalSentRegions ? &*optionalSentRegions : nullptr);
  });
}

// HLAreportFederateLost is a source-mandated RTI-originated receive-order MOM
// interaction. It intentionally has its own queue rather than widening the
// ordinary sender path above: generic RTI-created MOM traffic still lacks a
// standards-backed public producer-designator mapping. This narrow adapter
// represents the non-joined RTI producer with the official default-invalid
// FederateHandle, never a numeric zero or the lost federate's handle.
void queueFederateLostReport(
    std::wstring federationName,
    umbra::detail::FederateLostReportPlan report,
    std::wstring faultDescription) {
  if (report.status != umbra::detail::FederateLostReportStatus::applied ||
      report.reportedFederateId == 0U || !report.lastKnownTime ||
      report.routing.interactionClassHandle == 0U ||
      report.routing.federateParameterHandle == 0U ||
      report.routing.federateNameParameterHandle == 0U ||
      report.routing.timestampParameterHandle == 0U ||
      report.routing.faultDescriptionParameterHandle == 0U) {
    return;
  }

  std::vector<InteractionParameterValue> sentParameters;
  sentParameters.reserve(4U);
  sentParameters.emplace_back(
      report.routing.federateParameterHandle,
      makeFederateHandle(report.reportedFederateId).encode());
  sentParameters.emplace_back(
      report.routing.federateNameParameterHandle,
      HLAunicodeString{report.reportedFederateName}.encode());
  sentParameters.emplace_back(
      report.routing.timestampParameterHandle,
      report.lastKnownTime->encode());
  sentParameters.emplace_back(
      report.routing.faultDescriptionParameterHandle,
      HLAunicodeString{faultDescription}.encode());
  auto const reliableTransportation = transportationHandleFromEmbeddedName(
      "HLAreliable",
      L"The embedded federation could not reconstruct HLAreportFederateLost transportation.");

  for (auto& plannedRecipient : report.recipients) {
    if (!plannedRecipient.callbackRoute || plannedRecipient.federateId == 0U) {
      continue;
    }
    submitReceiveOrderCallback(
        std::move(plannedRecipient.callbackRoute),
        federationName,
        plannedRecipient.federateId,
        [
        federationName,
        reportedFederateId = report.reportedFederateId,
        receivingFederateId = plannedRecipient.federateId,
        sentParameters,
        reliableTransportation](FederateAmbassador& recipient) mutable {
      std::optional<umbra::detail::ReceiveOrderInteractionRecipient> projection;
      {
        std::scoped_lock lock(federationManagementMutex());
        projection = embeddedFederationManagement().registry().federateLostReportRecipientFor(
            federationName,
            reportedFederateId,
            receivingFederateId);
      }
      if (!projection) {
        return;
      }

      ParameterHandleValueMap parameterValues = projectInteractionParameterValues(
          sentParameters,
          projection->receivedParameterHandles);
      recipient.receiveInteraction(
          makeInteractionClassHandle(projection->receivedInteractionClassHandle),
          parameterValues,
          VariableLengthData{},
          reliableTransportation,
          FederateHandle{},
          nullptr);
    });
  }
}

// HLAreportException is RTI-originated receive-order MOM traffic selected by
// the failing member's Exception Reporting Switch. Keep its two-parameter
// payload on the same callback-time subscription/reprojection path as the
// bounded service-invocation and federate-lost reports.
void queueExceptionReport(
    std::wstring federationName,
    umbra::detail::ExceptionReportPlan report,
    std::wstring service,
    std::wstring exception) {
  if (report.status != umbra::detail::ExceptionReportStatus::applied ||
      report.reportedFederateId == 0U ||
      report.routing.interactionClassHandle == 0U ||
      report.routing.serviceParameterHandle == 0U ||
      report.routing.exceptionParameterHandle == 0U ||
      report.recipients.empty()) {
    return;
  }

  std::vector<InteractionParameterValue> sentParameters;
  sentParameters.reserve(2U);
  sentParameters.emplace_back(
      report.routing.serviceParameterHandle,
      HLAunicodeString{service}.encode());
  sentParameters.emplace_back(
      report.routing.exceptionParameterHandle,
      HLAunicodeString{exception}.encode());
  auto const reliableTransportation = transportationHandleFromEmbeddedName(
      "HLAreliable",
      L"The embedded federation could not reconstruct HLAreportException transportation.");

  for (auto& plannedRecipient : report.recipients) {
    if (!plannedRecipient.callbackRoute || plannedRecipient.federateId == 0U) {
      continue;
    }
    auto callbackRoute = std::move(plannedRecipient.callbackRoute);
    auto const receivingFederateId = plannedRecipient.federateId;
    submitReceiveOrderCallback(
        std::move(callbackRoute),
        federationName,
        receivingFederateId,
        [
            federationName,
            reportedFederateId = report.reportedFederateId,
            receivingFederateId,
            interactionClassHandle = report.routing.interactionClassHandle,
            sentParameters,
            reliableTransportation](FederateAmbassador& recipient) mutable {
      std::optional<umbra::detail::ReceiveOrderInteractionRecipient> projection;
      {
        std::scoped_lock lock(federationManagementMutex());
        projection = embeddedFederationManagement().registry().exceptionReportRecipientFor(
            federationName,
            reportedFederateId,
            receivingFederateId);
      }
      if (!projection) {
        return;
      }

      auto const parameterValues = projectInteractionParameterValues(
          sentParameters,
          projection->receivedParameterHandles);
      recipient.receiveInteraction(
          makeInteractionClassHandle(projection->receivedInteractionClassHandle),
          parameterValues,
          VariableLengthData{},
          reliableTransportation,
          FederateHandle{},
          nullptr);
    });
  }
}

// §11.5.1 report interactions are RTI-originated receive-order traffic.  This
// direct-service slice uses the same private registry planner as the file
// sink, but keeps the endpoint and source out of the public callback payload:
// the standard FederateHandle{} value is the binding's existing representation
// for an RTI producer.  The recipient is re-planned immediately before the
// callback so a late unsubscription, region change, or resignation cannot
// leak a stale report.
void queueMomServiceReportInteraction(
    std::wstring federationName,
    std::uint64_t reportedFederateId,
    std::uint16_t serviceGroup,
    umbra::detail::ReservedMomServiceReport reservation,
    std::vector<InteractionParameterValue> sentParameters,
    TransportationTypeHandle transportationType) {
  if (!reservation.acceptedForEmission ||
      reservation.routing.disposition !=
          umbra::detail::MomServiceReportDisposition::interaction ||
      reservation.routing.interactionClassHandle == 0U ||
      reservation.routing.recipients.empty()) {
    return;
  }

  for (auto& plannedRecipient : reservation.routing.recipients) {
    if (!plannedRecipient.callbackRoute || plannedRecipient.federateId == 0U) {
      continue;
    }
    auto callbackRoute = std::move(plannedRecipient.callbackRoute);
    auto const receivingFederateId = plannedRecipient.federateId;
    submitReceiveOrderCallback(
        std::move(callbackRoute),
        federationName,
        receivingFederateId,
        [federationName,
         reportedFederateId,
         receivingFederateId,
         serviceGroup,
         interactionClassHandle = reservation.routing.interactionClassHandle,
         sentParameters,
         transportationType](FederateAmbassador& recipient) mutable {
      std::optional<umbra::detail::ReceiveOrderInteractionRecipient> projection;
      {
        std::scoped_lock lock(federationManagementMutex());
        projection = embeddedFederationManagement().registry().momServiceReportRecipientFor(
            federationName,
            reportedFederateId,
            receivingFederateId,
            serviceGroup);
      }
      if (!projection) {
        return;
      }

      auto const parameterValues = projectInteractionParameterValues(
          sentParameters,
          projection->receivedParameterHandles);
      recipient.receiveInteraction(
          makeInteractionClassHandle(projection->receivedInteractionClassHandle),
          parameterValues,
          VariableLengthData{},
          transportationType,
          FederateHandle{},
          nullptr);
    });
  }
}

void queueTimestampedReceiveOrderInteraction(
    umbra::detail::InteractionCallbackRoute callbackRoute,
    std::wstring federationName,
    umbra::detail::InteractionProducer producingSource,
    std::uint64_t receivingFederateId,
    std::uint64_t sentInteractionClassHandle,
    std::vector<InteractionParameterValue> sentParameters,
    VariableLengthData userSuppliedTag,
    TransportationTypeHandle transportationType,
    std::shared_ptr<LogicalTime const> timestamp,
    OrderType sentOrderType,
    OrderType receivedOrderType,
    std::optional<std::uint64_t> retractionMessageId,
    std::optional<std::set<std::uint64_t>> sentRegionHandles = std::nullopt,
    bool defaultRegionUsed = false) {
  auto const producingFederateId = producingSource.joinedFederateId();
  if (producingSource.kind() != umbra::detail::InteractionProducer::Kind::joined_federate ||
      !producingFederateId || *producingFederateId == 0U) {
    // See the receive-order counterpart above. A timestamped callback also
    // carries a FederateHandle and cannot be the escape hatch for an
    // unresolved RTI-originated producer designator.
    throw RTIinternalError(
        L"The timestamped Receive Interaction callback path requires a joined-federate producer.");
  }
  std::vector<std::uint64_t> sentParameterHandles;
  sentParameterHandles.reserve(sentParameters.size());
  for (auto const& [parameterHandle, parameterValue] : sentParameters) {
    static_cast<void>(parameterValue);
    sentParameterHandles.push_back(parameterHandle);
  }

  submitReceiveOrderCallback(
      std::move(callbackRoute),
      federationName,
      receivingFederateId,
      [
      federationName,
      producingFederateId = *producingFederateId,
      receivingFederateId,
      sentInteractionClassHandle,
      sentParameterHandles = std::move(sentParameterHandles),
      sentParameters = std::move(sentParameters),
      userSuppliedTag = std::move(userSuppliedTag),
      transportationType = std::move(transportationType),
      timestamp = std::move(timestamp),
      sentOrderType,
      receivedOrderType,
      retractionMessageId,
      sentRegionHandles = std::move(sentRegionHandles),
      defaultRegionUsed](FederateAmbassador& recipient) mutable {
    if (!timestamp) {
      return;
    }
    std::optional<umbra::detail::ReceiveOrderInteractionRecipient> projection;
    bool callbackMayBegin = !retractionMessageId;
    {
      std::scoped_lock lock(federationManagementMutex());
      auto& registry = embeddedFederationManagement().registry();
      projection = registry.receiveOrderInteractionRecipientFor(
          federationName,
          producingFederateId,
          receivingFederateId,
          sentInteractionClassHandle,
          sentParameterHandles,
          sentRegionHandles ? &*sentRegionHandles : nullptr);
      if (projection && retractionMessageId) {
        callbackMayBegin = registry.beginTsoInteractionCallback(
            federationName,
            receivingFederateId,
            *retractionMessageId);
      }
    }
    if (!projection || !callbackMayBegin) {
      if (retractionMessageId) {
        std::scoped_lock lock(federationManagementMutex());
        static_cast<void>(embeddedFederationManagement().registry()
                              .finishTsoRecipientCallbackSuppressed(
                                  federationName,
                                  receivingFederateId,
                                  *retractionMessageId));
      }
      return;
    }

    ParameterHandleValueMap parameterValues = projectInteractionParameterValues(
        sentParameters,
        projection->receivedParameterHandles);
    std::optional<MessageRetractionHandle> retraction;
    if (retractionMessageId) {
      retraction.emplace(makeMessageRetractionHandle(*retractionMessageId));
    }
    std::optional<RegionHandleSet> optionalSentRegions;
    if (projection->conveyRegionDesignatorSets &&
        (sentRegionHandles || defaultRegionUsed)) {
      optionalSentRegions.emplace();
      if (sentRegionHandles) {
        for (auto const regionHandle : *sentRegionHandles) {
          optionalSentRegions->insert(makeRegionHandle(regionHandle));
        }
      }
    }
    recordSuccessfulInteractionReceipt(federationName, receivingFederateId, false);
    recipient.receiveInteraction(
        makeInteractionClassHandle(projection->receivedInteractionClassHandle),
        parameterValues,
        userSuppliedTag,
        transportationType,
        makeFederateHandle(producingFederateId),
        optionalSentRegions ? &*optionalSentRegions : nullptr,
        *timestamp,
        sentOrderType,
        receivedOrderType,
        retraction ? &*retraction : nullptr);
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
        projection = registry
            .receiveOrderAttributeUpdateRecipientFor(
                federationName,
                producingFederateId,
                receivingFederateId,
                objectInstanceHandle,
                passel.sentAttributeHandles,
                passel.sentRegionHandles.empty() ? nullptr : &passel.sentRegionHandles);
      }
      if (!projection) {
        continue;
      }
      auto const reliableTransportation = passel.transportationName == "HLAreliable";

      auto const transportationName = umbra::detail::wideFromUtf8(
          passel.transportationName);
      auto const transportationValue = transportationName
          ? standardTransportationTypeValue(*transportationName)
          : std::nullopt;
      if (!transportationValue) {
        throw RTIinternalError(
            L"The embedded federation could not reconstruct a timestamped attribute callback transportation type.");
      }

      AttributeHandleValueMap attributeValues = projectAttributeValues(
          sentAttributes,
          projection->receivedAttributeHandles);
      if (!reliableTransportation) {
        for (auto iterator = attributeValues.begin(); iterator != attributeValues.end();) {
          std::uint64_t numeric = 0;
          for (auto const candidate : passel.sentAttributeHandles) {
            if (makeAttributeHandle(candidate) == iterator->first) {
              numeric = candidate;
              break;
            }
          }
          auto const rate = projection->maximumUpdateRatesByAttribute.find(numeric);
          auto const key = federationName + L"/" +
              std::to_wstring(receivingFederateId) + L"/" +
              std::to_wstring(objectInstanceHandle) + L"/" +
              std::to_wstring(projection->subscriptionGeneration) + L"/" +
              std::to_wstring(numeric);
          auto const encoded = umbra::detail::utf8FromWide(key);
          bool const admitted = !encoded || updateRateGate().admit(
              *encoded,
              rate == projection->maximumUpdateRatesByAttribute.end()
                  ? projection->maximumUpdateRate
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

      recordSuccessfulReflectionReceipt(federationName, receivingFederateId);
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

void queueReceiveOrderDirectedInteraction(
    umbra::detail::InteractionCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t producingFederateId,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t sentInteractionClassHandle,
    std::vector<InteractionParameterValue> sentParameters,
    VariableLengthData userSuppliedTag,
    TransportationTypeHandle transportationType) {
  std::vector<std::uint64_t> sentParameterHandles;
  sentParameterHandles.reserve(sentParameters.size());
  for (auto const& [parameterHandle, parameterValue] : sentParameters) {
    static_cast<void>(parameterValue);
    sentParameterHandles.push_back(parameterHandle);
  }

  submitReceiveOrderCallback(
      std::move(callbackRoute),
      federationName,
      receivingFederateId,
      [
      federationName,
      producingFederateId,
      receivingFederateId,
      objectInstanceHandle,
      sentInteractionClassHandle,
      sentParameterHandles = std::move(sentParameterHandles),
      sentParameters = std::move(sentParameters),
      userSuppliedTag = std::move(userSuppliedTag),
      transportationType = std::move(transportationType)](FederateAmbassador& recipient) mutable {
    std::optional<umbra::detail::ReceiveOrderDirectedInteractionRecipient> projection;
    {
      std::scoped_lock lock(federationManagementMutex());
      projection = embeddedFederationManagement().registry()
          .receiveOrderDirectedInteractionRecipientFor(
              federationName,
              producingFederateId,
              receivingFederateId,
              objectInstanceHandle,
              sentInteractionClassHandle,
              sentParameterHandles);
    }
    if (!projection) {
      return;
    }

    ParameterHandleValueMap parameterValues = projectInteractionParameterValues(
        sentParameters,
        projection->receivedParameterHandles);
    recordSuccessfulInteractionReceipt(federationName, receivingFederateId, true);
    recipient.receiveDirectedInteraction(
        makeInteractionClassHandle(projection->receivedInteractionClassHandle),
        makeObjectInstanceHandle(projection->objectInstanceHandle),
        parameterValues,
        userSuppliedTag,
        transportationType,
        makeFederateHandle(producingFederateId));
  });
}

void queueTimestampedReceiveOrderDirectedInteraction(
    umbra::detail::InteractionCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t producingFederateId,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t sentInteractionClassHandle,
    std::vector<InteractionParameterValue> sentParameters,
    VariableLengthData userSuppliedTag,
    TransportationTypeHandle transportationType,
    std::shared_ptr<LogicalTime const> timestamp,
    OrderType sentOrderType,
    OrderType receivedOrderType,
    std::optional<std::uint64_t> retractionMessageId) {
  std::vector<std::uint64_t> sentParameterHandles;
  sentParameterHandles.reserve(sentParameters.size());
  for (auto const& [parameterHandle, parameterValue] : sentParameters) {
    static_cast<void>(parameterValue);
    sentParameterHandles.push_back(parameterHandle);
  }

  submitReceiveOrderCallback(
      std::move(callbackRoute),
      federationName,
      receivingFederateId,
      [
      federationName,
      producingFederateId,
      receivingFederateId,
      objectInstanceHandle,
      sentInteractionClassHandle,
      sentParameterHandles = std::move(sentParameterHandles),
      sentParameters = std::move(sentParameters),
      userSuppliedTag = std::move(userSuppliedTag),
      transportationType = std::move(transportationType),
      timestamp = std::move(timestamp),
      sentOrderType,
      receivedOrderType,
      retractionMessageId](FederateAmbassador& recipient) mutable {
    if (!timestamp) {
      return;
    }
    std::optional<umbra::detail::ReceiveOrderDirectedInteractionRecipient> projection;
    bool callbackMayBegin = !retractionMessageId;
    {
      std::scoped_lock lock(federationManagementMutex());
      auto& registry = embeddedFederationManagement().registry();
      projection = registry
          .receiveOrderDirectedInteractionRecipientFor(
              federationName,
              producingFederateId,
              receivingFederateId,
              objectInstanceHandle,
              sentInteractionClassHandle,
              sentParameterHandles);
      if (projection && retractionMessageId) {
        callbackMayBegin = registry.beginTsoInteractionCallback(
            federationName,
            receivingFederateId,
            *retractionMessageId);
      }
    }
    if (!projection || !callbackMayBegin) {
      if (retractionMessageId) {
        std::scoped_lock lock(federationManagementMutex());
        static_cast<void>(embeddedFederationManagement().registry()
                              .finishTsoRecipientCallbackSuppressed(
                                  federationName,
                                  receivingFederateId,
                                  *retractionMessageId));
      }
      return;
    }

    ParameterHandleValueMap parameterValues = projectInteractionParameterValues(
        sentParameters,
        projection->receivedParameterHandles);
    std::optional<MessageRetractionHandle> retraction;
    if (retractionMessageId) {
      retraction.emplace(makeMessageRetractionHandle(*retractionMessageId));
    }
    recordSuccessfulInteractionReceipt(federationName, receivingFederateId, true);
    recipient.receiveDirectedInteraction(
        makeInteractionClassHandle(projection->receivedInteractionClassHandle),
        makeObjectInstanceHandle(projection->objectInstanceHandle),
        parameterValues,
        userSuppliedTag,
        transportationType,
        makeFederateHandle(producingFederateId),
        *timestamp,
        sentOrderType,
        receivedOrderType,
        retraction ? &*retraction : nullptr);
  });
}

void queueReceiveOrderAttributeUpdate(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t producingFederateId,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::vector<std::uint64_t> sentAttributeHandles,
    std::vector<AttributeValue> sentAttributes,
    VariableLengthData userSuppliedTag,
    TransportationTypeHandle transportationType,
    double maximumUpdateRate = 0.0,
    bool reliableTransportation = false,
    std::optional<std::set<std::uint64_t>> sentRegionHandles = std::nullopt,
    bool defaultRegionUsed = false) {
  submitReceiveOrderCallback(
      std::move(callbackRoute),
      federationName,
      receivingFederateId,
      [
      federationName,
      producingFederateId,
      receivingFederateId,
      objectInstanceHandle,
      sentAttributeHandles = std::move(sentAttributeHandles),
      sentAttributes = std::move(sentAttributes),
      userSuppliedTag = std::move(userSuppliedTag),
      transportationType = std::move(transportationType),
      maximumUpdateRate,
      reliableTransportation,
      sentRegionHandles = std::move(sentRegionHandles),
      defaultRegionUsed](FederateAmbassador& recipient) mutable {
    std::optional<umbra::detail::ReceiveOrderAttributeUpdateRecipient> projection;
    {
      std::scoped_lock lock(federationManagementMutex());
      projection = embeddedFederationManagement().registry().receiveOrderAttributeUpdateRecipientFor(
          federationName,
          producingFederateId,
          receivingFederateId,
          objectInstanceHandle,
          sentAttributeHandles,
          sentRegionHandles ? &*sentRegionHandles : nullptr);
    }
    if (!projection) {
      return;
    }

    // Recipient eligibility is re-evaluated immediately before the callback,
    // but only against the submitted attributes in this one FOM-transport
    // passel. A changed subscription can remove values; it cannot combine or
    // divide this passel with another Update Attribute Values request.
    AttributeHandleValueMap attributeValues = projectAttributeValues(
        sentAttributes,
        projection->receivedAttributeHandles);
    if (!reliableTransportation) {
      for (auto iterator = attributeValues.begin(); iterator != attributeValues.end();) {
        auto const handle = iterator->first;
        std::uint64_t numeric = 0;
        for (auto const candidate : sentAttributeHandles) {
          if (makeAttributeHandle(candidate) == handle) {
            numeric = candidate;
            break;
          }
        }
        auto const rate = projection->maximumUpdateRatesByAttribute.find(numeric);
        auto const key = federationName + L"/" +
            std::to_wstring(receivingFederateId) + L"/" +
            std::to_wstring(objectInstanceHandle) + L"/" +
            std::to_wstring(projection->subscriptionGeneration) + L"/" +
            std::to_wstring(numeric);
        auto const encoded = umbra::detail::utf8FromWide(key);
        bool const admitted = !encoded ||
            updateRateGate().admit(*encoded, rate == projection->maximumUpdateRatesByAttribute.end()
                                             ? maximumUpdateRate
                                             : rate->second,
                                   false);
        if (!admitted) {
          iterator = attributeValues.erase(iterator);
        } else {
          ++iterator;
        }
      }
      if (attributeValues.empty()) {
        return;
      }
    }
    std::optional<RegionHandleSet> optionalSentRegions;
    if (projection->conveyRegionDesignatorSets &&
        (sentRegionHandles || defaultRegionUsed)) {
      optionalSentRegions.emplace();
      if (sentRegionHandles) {
        for (std::uint64_t const regionHandle : *sentRegionHandles) {
          optionalSentRegions->insert(makeRegionHandle(regionHandle));
        }
      }
    }
    {
      std::scoped_lock lock(federationManagementMutex());
      static_cast<void>(embeddedFederationManagement().registry()
                            .recordSuccessfulObjectInstanceReflection(
                                federationName,
                                receivingFederateId,
                                objectInstanceHandle));
    }
    recordSuccessfulReflectionReceipt(federationName, receivingFederateId);
    recipient.reflectAttributeValues(
        makeObjectInstanceHandle(objectInstanceHandle),
        attributeValues,
        userSuppliedTag,
        transportationType,
        makeFederateHandle(producingFederateId),
        optionalSentRegions ? &*optionalSentRegions : nullptr);
  });
}

std::wstring provideAttributeValueUpdateServiceReportRecord(
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& requestedAttributeHandles,
    VariableLengthData const& userSuppliedTag,
    std::uint32_t serialNumber);

void queueAttributeValueUpdateProvide(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    umbra::detail::FederateServiceReportRoute serviceReportRoute,
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t providingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> requestedAttributeHandles,
    VariableLengthData userSuppliedTag) {
  callbackRoute([
      federationName = std::move(federationName),
      requestingFederateId,
      providingFederateId,
      objectInstanceHandle,
      requestedAttributeHandles = std::move(requestedAttributeHandles),
      userSuppliedTag = std::move(userSuppliedTag),
      serviceReportRoute = std::move(serviceReportRoute)](FederateAmbassador& provider) mutable {
    std::optional<umbra::detail::AttributeValueUpdateProvideRecipient> projection;
    {
      std::scoped_lock lock(federationManagementMutex());
      projection = embeddedFederationManagement().registry().attributeValueUpdateProvideRecipientFor(
          federationName,
          requestingFederateId,
          providingFederateId,
          objectInstanceHandle,
          requestedAttributeHandles);
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
            return provideAttributeValueUpdateServiceReportRecord(
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
        requestRegionsByAttribute = std::nullopt) {
  callbackRoute([
      federationName = std::move(federationName),
      requestingFederateId,
      providingFederateId,
      objectInstanceHandle,
      requestedObjectClassHandle,
      requestedAttributeHandles = std::move(requestedAttributeHandles),
      userSuppliedTag = std::move(userSuppliedTag),
      serviceReportRoute = std::move(serviceReportRoute),
      requestRegionsByAttribute = std::move(requestRegionsByAttribute)](
      FederateAmbassador& provider) mutable {
    std::optional<umbra::detail::AttributeValueUpdateProvideRecipient> projection;
    {
      std::scoped_lock lock(federationManagementMutex());
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
            return provideAttributeValueUpdateServiceReportRecord(
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

void queueAttributeOwnershipQueryReport(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    umbra::detail::AttributeOwnershipQueryReportKind reportKind,
    std::uint64_t owningFederateId,
    std::set<std::uint64_t> requestedAttributeHandles) {
  callbackRoute([
      federationName = std::move(federationName),
      requestingFederateId,
      objectInstanceHandle,
      reportKind,
      owningFederateId,
      requestedAttributeHandles = std::move(requestedAttributeHandles)](
                    FederateAmbassador& requester) mutable {
    std::optional<umbra::detail::AttributeOwnershipQueryRecipient> projection;
    {
      std::scoped_lock lock(federationManagementMutex());
      projection = embeddedFederationManagement().registry().attributeOwnershipQueryRecipientFor(
          federationName,
          requestingFederateId,
          objectInstanceHandle,
          reportKind,
          owningFederateId,
          requestedAttributeHandles);
    }
    if (!projection) {
      return;
    }

    AttributeHandleSet attributes;
    for (std::uint64_t const attributeHandle : projection->attributeHandles) {
      attributes.insert(makeAttributeHandle(attributeHandle));
    }
    switch (projection->reportKind) {
      case umbra::detail::AttributeOwnershipQueryReportKind::federate:
        requester.informAttributeOwnership(
            makeObjectInstanceHandle(projection->objectInstanceHandle),
            attributes,
            makeFederateHandle(projection->owningFederateId));
        return;
      case umbra::detail::AttributeOwnershipQueryReportKind::unowned:
        requester.attributeIsNotOwned(
            makeObjectInstanceHandle(projection->objectInstanceHandle),
            attributes);
        return;
      case umbra::detail::AttributeOwnershipQueryReportKind::rti:
        requester.attributeIsOwnedByRTI(
            makeObjectInstanceHandle(projection->objectInstanceHandle),
            attributes);
        return;
    }
  });
}

void queueAttributeOwnershipAcquisitionIfAvailableReport(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t requestId,
    VariableLengthData userSuppliedTag) {
  callbackRoute([
      federationName = std::move(federationName),
      requestingFederateId,
      objectInstanceHandle,
      requestId,
      userSuppliedTag = std::move(userSuppliedTag)](FederateAmbassador& requester) mutable {
    std::optional<umbra::detail::AttributeOwnershipAcquisitionIfAvailableDelivery> delivery;
    {
      std::scoped_lock lock(federationManagementMutex());
      delivery = embeddedFederationManagement().registry()
                     .beginAttributeOwnershipAcquisitionIfAvailable(
                         federationName,
                         requestingFederateId,
                         objectInstanceHandle,
                         requestId);
    }
    if (!delivery) {
      return;
    }

    if (!delivery->securedAttributeHandles.empty()) {
      AttributeHandleSet securedAttributes;
      for (std::uint64_t const attributeHandle : delivery->securedAttributeHandles) {
        securedAttributes.insert(makeAttributeHandle(attributeHandle));
      }
      requester.attributeOwnershipAcquisitionNotification(
          makeObjectInstanceHandle(delivery->objectInstanceHandle),
          securedAttributes,
          userSuppliedTag);
    }
    if (!delivery->unavailableAttributeHandles.empty()) {
      AttributeHandleSet unavailableAttributes;
      for (std::uint64_t const attributeHandle : delivery->unavailableAttributeHandles) {
        unavailableAttributes.insert(makeAttributeHandle(attributeHandle));
      }
      requester.attributeOwnershipUnavailable(
          makeObjectInstanceHandle(delivery->objectInstanceHandle),
          unavailableAttributes,
          userSuppliedTag);
    }
  });
}

void queueAttributeOwnershipAcquisitionWorkItems(
    std::vector<umbra::detail::AttributeOwnershipAcquisitionWorkItem> workItems,
    std::wstring const& federationName);

void queueAttributeOwnershipAcquisitionWorkItem(
    umbra::detail::AttributeOwnershipAcquisitionWorkItem workItem,
    std::wstring federationName) {
  if (!workItem.callbackRoute) {
    throw RTIinternalError(
        L"The embedded federation has an ownership-acquisition callback without a route.");
  }

  switch (workItem.kind) {
    case umbra::detail::AttributeOwnershipAcquisitionWorkKind::acquisition_notification:
      workItem.callbackRoute([
          federationName = std::move(federationName),
          requestingFederateId = workItem.requestingFederateId,
          objectInstanceHandle = workItem.objectInstanceHandle,
          requestId = workItem.requestId,
          scheduledAttributeHandles = std::move(workItem.attributeHandles),
          userSuppliedTag = std::move(workItem.userSuppliedTag)](
                                 FederateAmbassador& requester) mutable {
        std::optional<umbra::detail::AttributeOwnershipAcquisitionNotificationDelivery> delivery;
        {
          std::scoped_lock lock(federationManagementMutex());
          delivery = embeddedFederationManagement().registry()
                         .beginAttributeOwnershipAcquisitionNotification(
                             federationName,
                             requestingFederateId,
                             objectInstanceHandle,
                             requestId,
                             scheduledAttributeHandles);
        }
        if (!delivery) {
          return;
        }

        auto queueFollowup = [&] {
          queueAttributeOwnershipAcquisitionWorkItems(
              std::move(delivery->followupWorkItems),
              federationName);
        };
        if (!delivery->securedAttributeHandles.empty()) {
          AttributeHandleSet securedAttributes;
          for (std::uint64_t const attributeHandle : delivery->securedAttributeHandles) {
            securedAttributes.insert(makeAttributeHandle(attributeHandle));
          }
          VariableLengthData const callbackTag = makeVariableLengthData(userSuppliedTag);
          try {
            requester.attributeOwnershipAcquisitionNotification(
                makeObjectInstanceHandle(delivery->objectInstanceHandle),
                securedAttributes,
                callbackTag);
          } catch (...) {
            // The registry has committed the ownership transition before user
            // code. Wake any subsequently pending acquisition even if this
            // callback reports FederateInternalError to its dispatcher.
            queueFollowup();
            throw;
          }
        }
        queueFollowup();
      });
      return;
    case umbra::detail::AttributeOwnershipAcquisitionWorkKind::
        request_divestiture_confirmation:
      workItem.callbackRoute([
          federationName = std::move(federationName),
          acquiringFederateId = workItem.requestingFederateId,
          divestingFederateId = workItem.receivingFederateId,
          objectInstanceHandle = workItem.objectInstanceHandle,
          acquisitionRequestId = workItem.requestId,
          scheduledAttributeHandles = std::move(workItem.attributeHandles),
          userSuppliedTag = std::move(workItem.userSuppliedTag)](
                                     FederateAmbassador& owner) mutable {
        std::optional<umbra::detail::RequestDivestitureConfirmationDelivery> delivery;
        {
          std::scoped_lock lock(federationManagementMutex());
          delivery = embeddedFederationManagement().registry()
                         .beginRequestDivestitureConfirmation(
                             federationName,
                             divestingFederateId,
                             acquiringFederateId,
                             objectInstanceHandle,
                             acquisitionRequestId,
                             scheduledAttributeHandles);
        }
        if (!delivery) {
          return;
        }

        AttributeHandleSet releasedAttributes;
        for (std::uint64_t const attributeHandle : delivery->releasedAttributeHandles) {
          releasedAttributes.insert(makeAttributeHandle(attributeHandle));
        }
        VariableLengthData const callbackTag = makeVariableLengthData(userSuppliedTag);
        owner.requestDivestitureConfirmation(
            makeObjectInstanceHandle(delivery->objectInstanceHandle),
            releasedAttributes,
            callbackTag);
      });
      return;
    case umbra::detail::AttributeOwnershipAcquisitionWorkKind::request_release:
      workItem.callbackRoute([
          federationName = std::move(federationName),
          requestingFederateId = workItem.requestingFederateId,
          owningFederateId = workItem.receivingFederateId,
          objectInstanceHandle = workItem.objectInstanceHandle,
          requestId = workItem.requestId,
          scheduledAttributeHandles = std::move(workItem.attributeHandles),
          userSuppliedTag = std::move(workItem.userSuppliedTag)](
                                 FederateAmbassador& owner) mutable {
        std::optional<umbra::detail::AttributeOwnershipAcquisitionReleaseDelivery> delivery;
        {
          std::scoped_lock lock(federationManagementMutex());
          delivery = embeddedFederationManagement().registry()
                         .beginAttributeOwnershipAcquisitionRelease(
                             federationName,
                             requestingFederateId,
                             owningFederateId,
                             objectInstanceHandle,
                             requestId,
                             scheduledAttributeHandles);
        }
        if (!delivery) {
          return;
        }

        AttributeHandleSet candidateAttributes;
        for (std::uint64_t const attributeHandle : delivery->candidateAttributeHandles) {
          candidateAttributes.insert(makeAttributeHandle(attributeHandle));
        }
        VariableLengthData const callbackTag = makeVariableLengthData(userSuppliedTag);
        owner.requestAttributeOwnershipRelease(
            makeObjectInstanceHandle(delivery->objectInstanceHandle),
            candidateAttributes,
            callbackTag);
      });
      return;
  }
  throw RTIinternalError(
      L"Umbra encountered an unknown regular ownership-acquisition callback kind.");
}

void queueAttributeOwnershipAcquisitionWorkItems(
    std::vector<umbra::detail::AttributeOwnershipAcquisitionWorkItem> workItems,
    std::wstring const& federationName) {
  // Submit only after the registry and invoking ambassador have released
  // their locks. HLA_IMMEDIATE can enter either the acquirer or current owner
  // synchronously, including a Release Denied response.
  for (auto& workItem : workItems) {
    queueAttributeOwnershipAcquisitionWorkItem(std::move(workItem), federationName);
  }
}

void queueAttributeOwnershipAcquisitionCancellationConfirmation(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t cancellationId,
    std::set<std::uint64_t> attributeHandles) {
  if (!callbackRoute) {
    throw RTIinternalError(
        L"The embedded federation has an ownership-acquisition cancellation without a callback route.");
  }

  callbackRoute([
      federationName = std::move(federationName),
      requestingFederateId,
      objectInstanceHandle,
      cancellationId,
      attributeHandles = std::move(attributeHandles)](FederateAmbassador& requester) mutable {
    std::optional<umbra::detail::AttributeOwnershipAcquisitionCancellationDelivery> delivery;
    {
      std::scoped_lock lock(federationManagementMutex());
      delivery = embeddedFederationManagement().registry()
                     .beginAttributeOwnershipAcquisitionCancellation(
                         federationName,
                         requestingFederateId,
                         objectInstanceHandle,
                         cancellationId,
                         attributeHandles);
    }
    if (!delivery) {
      return;
    }

    auto queueFollowup = [&] {
      queueAttributeOwnershipAcquisitionWorkItems(
          std::move(delivery->followupWorkItems),
          federationName);
    };
    if (!delivery->confirmedAttributeHandles.empty()) {
      AttributeHandleSet confirmedAttributes;
      for (std::uint64_t const attributeHandle : delivery->confirmedAttributeHandles) {
        confirmedAttributes.insert(makeAttributeHandle(attributeHandle));
      }
      try {
        requester.confirmAttributeOwnershipAcquisitionCancellation(
            makeObjectInstanceHandle(delivery->objectInstanceHandle),
            confirmedAttributes);
      } catch (...) {
        queueFollowup();
        throw;
      }
    }
    queueFollowup();
  });
}

void queueAttributeOwnershipDivestitureIfWantedNotifications(
    std::vector<umbra::detail::AttributeOwnershipDivestitureIfWantedNotification> notifications,
    std::wstring const& federationName) {
  for (auto& notification : notifications) {
    if (!notification.callbackRoute) {
      throw RTIinternalError(
          L"The embedded federation has an ownership-divestiture notification without a route.");
    }

    notification.callbackRoute([
        federationName,
        receivingFederateId = notification.receivingFederateId,
        objectInstanceHandle = notification.objectInstanceHandle,
        notificationId = notification.notificationId,
        scheduledAttributeHandles = std::move(notification.attributeHandles),
        userSuppliedTag = std::move(notification.userSuppliedTag)](
                                    FederateAmbassador& requester) mutable {
      std::optional<umbra::detail::AttributeOwnershipDivestitureIfWantedDelivery> delivery;
      {
        std::scoped_lock lock(federationManagementMutex());
        delivery = embeddedFederationManagement().registry()
                       .beginAttributeOwnershipDivestitureIfWantedNotification(
                           federationName,
                           receivingFederateId,
                           objectInstanceHandle,
                           notificationId,
                           scheduledAttributeHandles);
      }
      if (!delivery) {
        return;
      }

      auto queueFollowup = [&] {
        queueAttributeOwnershipAcquisitionWorkItems(
            std::move(delivery->followupWorkItems),
            federationName);
      };
      if (!delivery->securedAttributeHandles.empty()) {
        AttributeHandleSet securedAttributes;
        for (std::uint64_t const attributeHandle : delivery->securedAttributeHandles) {
          securedAttributes.insert(makeAttributeHandle(attributeHandle));
        }
        VariableLengthData const callbackTag = makeVariableLengthData(userSuppliedTag);
        try {
          requester.attributeOwnershipAcquisitionNotification(
              makeObjectInstanceHandle(delivery->objectInstanceHandle),
              securedAttributes,
              callbackTag);
        } catch (...) {
          // Ownership moved synchronously at the service return. Continue
          // planning older regular requests even if the federate callback
          // reports FederateInternalError to its dispatcher.
          queueFollowup();
          throw;
        }
      }
      queueFollowup();
    });
  }
}

void queueConfirmDivestitureNotifications(
    std::vector<umbra::detail::ConfirmDivestitureNotification> notifications,
    std::wstring const& federationName) {
  for (auto& notification : notifications) {
    if (!notification.callbackRoute) {
      throw RTIinternalError(
          L"The embedded federation has a Confirm Divestiture notification without a route.");
    }
    notification.callbackRoute([
        federationName,
        receivingFederateId = notification.receivingFederateId,
        objectInstanceHandle = notification.objectInstanceHandle,
        notificationId = notification.notificationId,
        scheduledAttributeHandles = std::move(notification.attributeHandles),
        userSuppliedTag = std::move(notification.userSuppliedTag)](
                                    FederateAmbassador& requester) mutable {
      std::optional<umbra::detail::ConfirmDivestitureNotificationDelivery> delivery;
      {
        std::scoped_lock lock(federationManagementMutex());
        delivery = embeddedFederationManagement().registry()
                       .beginConfirmDivestitureNotification(
                           federationName,
                           receivingFederateId,
                           objectInstanceHandle,
                           notificationId,
                           scheduledAttributeHandles);
      }
      if (!delivery) {
        return;
      }

      auto queueFollowup = [&] {
        queueAttributeOwnershipAcquisitionWorkItems(
            std::move(delivery->followupWorkItems),
            federationName);
      };
      if (!delivery->securedAttributeHandles.empty()) {
        AttributeHandleSet securedAttributes;
        for (std::uint64_t const attributeHandle : delivery->securedAttributeHandles) {
          securedAttributes.insert(makeAttributeHandle(attributeHandle));
        }
        VariableLengthData const callbackTag = makeVariableLengthData(userSuppliedTag);
        try {
          requester.attributeOwnershipAcquisitionNotification(
              makeObjectInstanceHandle(delivery->objectInstanceHandle),
              securedAttributes,
              callbackTag);
        } catch (...) {
          queueFollowup();
          throw;
        }
      }
      queueFollowup();
    });
  }
}

void queueAttributeOwnershipAssumptionRecipients(
    std::vector<umbra::detail::AttributeOwnershipAssumptionRecipient> recipients,
    std::wstring const& federationName,
    VariableLengthData const& userSuppliedTag) {
  for (auto& recipient : recipients) {
    if (!recipient.callbackRoute) {
      throw RTIinternalError(
          L"The embedded federation has an ownership-assumption callback without a route.");
    }

    recipient.callbackRoute([
        federationName,
        receivingFederateId = recipient.receivingFederateId,
        objectInstanceHandle = recipient.objectInstanceHandle,
        scheduledAttributeHandles = std::move(recipient.attributeHandles),
        userSuppliedTag](FederateAmbassador& candidate) mutable {
      std::optional<umbra::detail::AttributeOwnershipAssumptionDelivery> delivery;
      {
        std::scoped_lock lock(federationManagementMutex());
        delivery = embeddedFederationManagement().registry()
                       .attributeOwnershipAssumptionDeliveryFor(
                           federationName,
                           receivingFederateId,
                           objectInstanceHandle,
                           scheduledAttributeHandles);
      }
      if (!delivery) {
        return;
      }

      AttributeHandleSet offeredAttributes;
      for (std::uint64_t const attributeHandle : delivery->attributeHandles) {
        offeredAttributes.insert(makeAttributeHandle(attributeHandle));
      }
      candidate.requestAttributeOwnershipAssumption(
          makeObjectInstanceHandle(delivery->objectInstanceHandle),
          offeredAttributes,
          userSuppliedTag);
    });
  }
}

void queueAttributeOwnershipUnavailableRecipients(
    std::vector<umbra::detail::AttributeOwnershipUnavailableRecipient> recipients,
    std::wstring const& federationName,
    VariableLengthData const& userSuppliedTag) {
  for (auto& recipient : recipients) {
    if (!recipient.callbackRoute) {
      throw RTIinternalError(
          L"The embedded federation has an ownership-unavailable recipient without a callback route.");
    }
    recipient.callbackRoute([
        federationName,
        receivingFederateId = recipient.receivingFederateId,
        objectInstanceHandle = recipient.objectInstanceHandle,
        attributeHandles = std::move(recipient.attributeHandles),
        userSuppliedTag](FederateAmbassador& requester) mutable {
      std::optional<umbra::detail::AttributeOwnershipUnavailableRecipient> delivery;
      {
        std::scoped_lock lock(federationManagementMutex());
        delivery = embeddedFederationManagement().registry()
                       .attributeOwnershipUnavailableRecipientFor(
                           federationName,
                           receivingFederateId,
                           objectInstanceHandle,
                           attributeHandles);
      }
      if (!delivery) {
        return;
      }

      AttributeHandleSet unavailableAttributes;
      for (std::uint64_t const attributeHandle : delivery->attributeHandles) {
        unavailableAttributes.insert(makeAttributeHandle(attributeHandle));
      }
      requester.attributeOwnershipUnavailable(
          makeObjectInstanceHandle(delivery->objectInstanceHandle),
          unavailableAttributes,
          userSuppliedTag);
    });
  }
}

void queueObjectInstanceNameReservation(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t federateId,
    bool succeeded,
    std::wstring objectInstanceName) {
  if (!callbackRoute) {
    throw RTIinternalError(
        L"The embedded federation has an object-instance name reservation without a callback route.");
  }
  callbackRoute([
      federationName = std::move(federationName),
      federateId,
      succeeded,
      objectInstanceName = std::move(objectInstanceName)](FederateAmbassador& recipient) mutable {
    {
      std::scoped_lock lock(federationManagementMutex());
      if (!embeddedFederationManagement().registry().memberById(
              federationName,
              federateId)) {
        return;
      }
    }
    if (succeeded) {
      recipient.objectInstanceNameReservationSucceeded(objectInstanceName);
    } else {
      recipient.objectInstanceNameReservationFailed(objectInstanceName);
    }
  });
}

void queueMultipleObjectInstanceNameReservation(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t federateId,
    bool succeeded,
    std::set<std::wstring> objectInstanceNames) {
  if (!callbackRoute) {
    throw RTIinternalError(
        L"The embedded federation has multiple object-instance name reservation without a callback route.");
  }
  callbackRoute([
      federationName = std::move(federationName),
      federateId,
      succeeded,
      objectInstanceNames = std::move(objectInstanceNames)](
      FederateAmbassador& recipient) mutable {
    {
      std::scoped_lock lock(federationManagementMutex());
      if (!embeddedFederationManagement().registry().memberById(
              federationName,
              federateId)) {
        return;
      }
    }
    if (succeeded) {
      recipient.multipleObjectInstanceNameReservationSucceeded(objectInstanceNames);
    } else {
      recipient.multipleObjectInstanceNameReservationFailed(objectInstanceNames);
    }
  });
}

std::wstring discoverObjectInstanceServiceReportRecord(
    std::uint64_t objectInstanceHandle,
    std::uint64_t objectClassHandle,
    std::wstring const& objectInstanceName,
    std::uint64_t producingFederateId,
    std::uint32_t serialNumber) {
  using umbra::detail::MomArgumentType;
  using umbra::detail::formatMomFederateHandle;
  using umbra::detail::formatMomObjectClassHandle;
  using umbra::detail::formatMomObjectInstanceHandle;
  using umbra::detail::formatMomString;
  using umbra::detail::formatMomSuccessfulVoidServiceReportRecord;

  return formatMomSuccessfulVoidServiceReportRecord(
      serialNumber,
      L"DiscoverObjectInstance",
      {{MomArgumentType::object_instance_handle,
        L"Object instance handle",
        formatMomObjectInstanceHandle(makeObjectInstanceHandle(objectInstanceHandle))},
       {MomArgumentType::object_class_handle,
        L"Object class designator",
        formatMomObjectClassHandle(makeObjectClassHandle(objectClassHandle))},
       {MomArgumentType::string,
        L"Object instance name",
        formatMomString(objectInstanceName)},
       {MomArgumentType::federate_handle,
        L"Producing joined federate designator",
        formatMomFederateHandle(makeFederateHandle(producingFederateId))}});
}

std::wstring removeObjectInstanceServiceReportRecord(
    std::uint64_t objectInstanceHandle,
    VariableLengthData const& userSuppliedTag,
    std::uint64_t producingFederateId,
    OrderType sentOrderType,
    LogicalTime const* optionalTimestamp,
    std::optional<OrderType> optionalReceivedOrderType,
    std::optional<std::uint64_t> optionalRetractionHandle,
    std::uint32_t serialNumber) {
  using umbra::detail::MomArgumentType;
  using umbra::detail::formatMomFederateHandle;
  using umbra::detail::formatMomLogicalTime;
  using umbra::detail::formatMomMessageRetractionHandle;
  using umbra::detail::formatMomNull;
  using umbra::detail::formatMomObjectInstanceHandle;
  using umbra::detail::formatMomOrderType;
  using umbra::detail::formatMomSuccessfulVoidServiceReportRecord;
  using umbra::detail::formatMomUserSuppliedTag;

  // Preserve the §6.17 narrative order, including every optional slot as a
  // Table 5 Null when the selected callback overload did not supply it.
  std::vector<umbra::detail::MomServiceArgument> suppliedArguments{
      {MomArgumentType::object_instance_handle,
       L"Object instance designator",
       formatMomObjectInstanceHandle(makeObjectInstanceHandle(objectInstanceHandle))},
      {MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       formatMomUserSuppliedTag(userSuppliedTag)},
      {MomArgumentType::order_type,
       L"Sent message order type",
       formatMomOrderType(sentOrderType)},
      {MomArgumentType::federate_handle,
       L"Producing joined federate designator",
       formatMomFederateHandle(makeFederateHandle(producingFederateId))},
  };
  if (optionalTimestamp) {
    suppliedArguments.push_back(
        {MomArgumentType::logical_time,
         L"Optional timestamp",
         formatMomLogicalTime(*optionalTimestamp)});
  } else {
    suppliedArguments.push_back(
        {MomArgumentType::null_value, L"Optional timestamp", formatMomNull()});
  }
  if (optionalReceivedOrderType) {
    suppliedArguments.push_back(
        {MomArgumentType::order_type,
         L"Optional receive message order type",
         formatMomOrderType(*optionalReceivedOrderType)});
  } else {
    suppliedArguments.push_back(
        {MomArgumentType::null_value,
         L"Optional receive message order type",
         formatMomNull()});
  }
  if (optionalRetractionHandle) {
    suppliedArguments.push_back(
        {MomArgumentType::message_retraction_handle,
         L"Optional message retraction designator",
         formatMomMessageRetractionHandle(*optionalRetractionHandle)});
  } else {
    suppliedArguments.push_back(
        {MomArgumentType::null_value,
         L"Optional message retraction designator",
         formatMomNull()});
  }

  return formatMomSuccessfulVoidServiceReportRecord(
      serialNumber,
      L"RemoveObjectInstance",
      suppliedArguments);
}

std::wstring provideAttributeValueUpdateServiceReportRecord(
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& requestedAttributeHandles,
    VariableLengthData const& userSuppliedTag,
    std::uint32_t serialNumber) {
  using umbra::detail::MomArgumentType;
  using umbra::detail::formatMomAttributeHandleSet;
  using umbra::detail::formatMomObjectInstanceHandle;
  using umbra::detail::formatMomSuccessfulVoidServiceReportRecord;
  using umbra::detail::formatMomUserSuppliedTag;

  AttributeHandleSet attributes;
  for (std::uint64_t const attributeHandle : requestedAttributeHandles) {
    attributes.insert(makeAttributeHandle(attributeHandle));
  }
  // §6.22.1 fixes this callback's three supplied arguments. The copied
  // request tag is retained through callback-time revalidation, including the
  // zero-length RTI-invoked Auto Provide form.
  return formatMomSuccessfulVoidServiceReportRecord(
      serialNumber,
      L"ProvideAttributeValueUpdate",
      {{MomArgumentType::object_instance_handle,
        L"Object instance designator",
        formatMomObjectInstanceHandle(makeObjectInstanceHandle(objectInstanceHandle))},
       {MomArgumentType::attribute_handle_set,
        L"Set of attribute designators",
        formatMomAttributeHandleSet(attributes)},
       {MomArgumentType::table_5_user_supplied_tag,
        L"User-supplied tag",
        formatMomUserSuppliedTag(userSuppliedTag)}});
}

void queueObjectInstanceDiscovery(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    umbra::detail::FederateServiceReportRoute serviceReportRoute,
    std::wstring federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    bool rtiOwnedMomObject) {
  callbackRoute([
      federationName = std::move(federationName),
      receivingFederateId,
      objectInstanceHandle,
      rtiOwnedMomObject,
      serviceReportRoute = std::move(serviceReportRoute)](FederateAmbassador& recipient) {
    std::optional<umbra::detail::KnownObjectInstanceSnapshot> discovery;
    {
      std::scoped_lock lock(federationManagementMutex());
      if (rtiOwnedMomObject) {
        discovery = embeddedFederationManagement().registry()
                        .beginJoinedFederateMomObjectDiscovery(
                            federationName,
                            receivingFederateId,
                            objectInstanceHandle);
      } else {
        discovery = embeddedFederationManagement().registry().beginObjectInstanceDiscovery(
            federationName,
            receivingFederateId,
            objectInstanceHandle);
      }
    }
    if (!discovery) {
      return;
    }

    if (serviceReportRoute) {
      // §6.9 is an RTI-initiated recipient service. The registry's
      // callback-time recheck above is the actual delivery boundary, so only
      // an invoked Discover Object Instance appends a selected-file record.
      // This preserves the report-before-callback ordering in HLA_EVOKED
      // without recording a cancelled or stale planned discovery.
      serviceReportRoute(
          static_cast<std::uint16_t>(umbra::detail::MomServiceType::object_management),
          [
              objectInstanceHandle = discovery->objectInstanceHandle,
              objectClassHandle = discovery->knownObjectClassHandle,
              objectInstanceName = discovery->objectInstanceName,
              producingFederateId = discovery->producingFederateId](
              std::uint32_t serialNumber) {
            return discoverObjectInstanceServiceReportRecord(
                objectInstanceHandle,
                objectClassHandle,
                objectInstanceName,
                producingFederateId,
                serialNumber);
          });
    }

    // The registry commits the recipient's known-instance state before this
    // callback, so a FederateAmbassador may safely use the matching 2025
    // support services from within Discover Object Instance.
    recipient.discoverObjectInstance(
        makeObjectInstanceHandle(discovery->objectInstanceHandle),
        makeObjectClassHandle(discovery->knownObjectClassHandle),
        discovery->objectInstanceName,
        rtiOwnedMomObject
            ? FederateHandle{}
            : makeFederateHandle(discovery->producingFederateId));

    if (rtiOwnedMomObject) {
      // The initial MOM values are RTI-owned and are reflected directly after
      // discovery. Revalidate the known-instance and active subscription
      // boundary so a queued callback cannot leak values after a declaration
      // change. Regional eligibility is evaluated against the immutable
      // HLAfederate point; periodic and other conditional scheduling remains
      // a later slice (event-driven switch updates use their own queue path).
      std::optional<umbra::detail::JoinedFederateMomAttributeValueUpdateRecipient>
          initialValues;
      {
        std::scoped_lock lock(federationManagementMutex());
        auto plan = embeddedFederationManagement().registry()
                        .planJoinedFederateMomAttributeValueUpdate(
                            federationName,
                            receivingFederateId,
                            objectInstanceHandle,
                            discovery->initialAttributeHandles,
                            true);
        if (plan.status ==
                umbra::detail::JoinedFederateMomAttributeValueUpdateStatus::applied &&
            plan.recipient) {
          initialValues = std::move(plan.recipient);
        }
      }
      if (initialValues && !initialValues->attributeValues.empty()) {
        AttributeHandleValueMap attributeValues;
        for (auto const& [attributeHandle, value] : initialValues->attributeValues) {
          attributeValues.emplace(makeAttributeHandle(attributeHandle), value);
        }
        recipient.reflectAttributeValues(
            makeObjectInstanceHandle(initialValues->objectInstanceHandle),
            attributeValues,
            VariableLengthData{},
            transportationHandleFromEmbeddedName(
                "HLAreliable",
                L"The embedded federation could not reconstruct the MOM transportation type."),
            FederateHandle{},
            nullptr);
      }
      return;
    }

    // Auto Provide is an RTI-invoked service that follows a newly completed
    // discovery. It solicits each current owner of an in-scope attribute with
    // the mandatory empty tag; the provider's callback-time recheck still
    // suppresses stale work after resignation, deletion, or scope changes.
    umbra::detail::AttributeValueUpdateRequestPlan autoProvidePlan;
    {
      std::scoped_lock lock(federationManagementMutex());
      autoProvidePlan = embeddedFederationManagement().registry()
                            .planAutoProvideForDiscovery(
                                federationName,
                                receivingFederateId,
                                objectInstanceHandle);
    }
    if (autoProvidePlan.status !=
        umbra::detail::AttributeValueUpdateRequestStatus::applied) {
      throw RTIinternalError(
          L"The embedded federation could not plan Auto Provide callbacks after discovery.");
    }
    for (auto& provider : autoProvidePlan.recipients) {
      if (!provider.callbackRoute) {
        throw RTIinternalError(
            L"The embedded federation has an Auto Provide recipient without a callback route.");
      }
      queueAttributeValueUpdateProvide(
          std::move(provider.callbackRoute),
          std::move(provider.serviceReportRoute),
          federationName,
          receivingFederateId,
          provider.providingFederateId,
          provider.objectInstanceHandle,
          std::move(provider.requestedAttributeHandles),
          VariableLengthData());
    }

    // Discovery establishes the known-class boundary required for ownership
    // assumption eligibility. Continue any prior resign/divestiture search
    // only after the standard Discover callback has entered user code, so an
    // immediate callback model cannot observe an assumption before discovery.
    std::vector<umbra::detail::AttributeOwnershipAssumptionRecipient>
        newlyEligibleAssumptions;
    {
      std::scoped_lock lock(federationManagementMutex());
      newlyEligibleAssumptions = embeddedFederationManagement().registry()
                                     .planAttributeOwnershipAssumptionsForFederate(
                                         federationName,
                                         receivingFederateId);
    }
    queueAttributeOwnershipAssumptionRecipients(
        std::move(newlyEligibleAssumptions),
        federationName,
        VariableLengthData());
  });
}

void queueObjectInstanceDiscoveries(
    std::vector<umbra::detail::ObjectInstanceDiscoveryRecipient> discoveries,
    std::wstring const& federationName) {
  // Do not hold a sender's ambassador lock or the federation lock while
  // submitting. HLA_IMMEDIATE may enter another federate's discovery callback
  // synchronously, including its support-service calls.
  for (std::size_t index = 0; index < discoveries.size(); ++index) {
    auto& discovery = discoveries[index];
    try {
      if (!discovery.callbackRoute) {
        throw RTIinternalError(
            L"The embedded federation has a discovery recipient without a callback route.");
      }
      queueObjectInstanceDiscovery(
          std::move(discovery.callbackRoute),
          std::move(discovery.serviceReportRoute),
          federationName,
          discovery.receivingFederateId,
          discovery.objectInstanceHandle,
          discovery.rtiOwnedMomObject);
    } catch (...) {
      // An immediate callback can have committed known-instance state before
      // propagating a user exception.  The registry clears only still-pending
      // reservations, preserving that committed state while allowing all
      // undelivered recipients to be reconsidered later.
      std::scoped_lock lock(federationManagementMutex());
      auto& registry = embeddedFederationManagement().registry();
      for (std::size_t pending = index; pending < discoveries.size(); ++pending) {
        if (discoveries[pending].rtiOwnedMomObject) {
          registry.cancelJoinedFederateMomObjectDiscovery(
              federationName,
              discoveries[pending].receivingFederateId,
              discoveries[pending].objectInstanceHandle);
        } else {
          registry.cancelObjectInstanceDiscovery(
              federationName,
              discoveries[pending].receivingFederateId,
              discoveries[pending].objectInstanceHandle);
        }
      }
      throw;
    }
  }
}

void queueJoinedFederateMomAttributeValueUpdate(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> requestedAttributeHandles,
    bool requireActiveSubscription = false,
    std::optional<std::map<std::uint64_t, VariableLengthData>>
        plannedAttributeValues = std::nullopt) {
  if (!callbackRoute) {
    throw RTIinternalError(
        L"The embedded federation has a MOM attribute reflection without a callback route.");
  }
  callbackRoute([
      federationName = std::move(federationName),
      receivingFederateId,
      objectInstanceHandle,
      requestedAttributeHandles = std::move(requestedAttributeHandles),
      requireActiveSubscription,
      plannedAttributeValues = std::move(plannedAttributeValues)](
      FederateAmbassador& recipient) mutable {
    std::optional<umbra::detail::JoinedFederateMomAttributeValueUpdateRecipient>
        reflection;
    {
      std::scoped_lock lock(federationManagementMutex());
      auto plan = embeddedFederationManagement().registry()
                      .planJoinedFederateMomAttributeValueUpdate(
                          federationName,
                          receivingFederateId,
                          objectInstanceHandle,
                          requestedAttributeHandles,
                          requireActiveSubscription);
      if (plan.status ==
              umbra::detail::JoinedFederateMomAttributeValueUpdateStatus::applied &&
          plan.recipient) {
        reflection = std::move(plan.recipient);
      }
    }
    if (!reflection || reflection->attributeValues.empty()) {
      return;
    }

    AttributeHandleValueMap attributeValues;
    auto const& values = plannedAttributeValues.has_value()
        ? *plannedAttributeValues
        : reflection->attributeValues;
    for (auto const& [attributeHandle, value] : values) {
      attributeValues.emplace(makeAttributeHandle(attributeHandle), value);
    }
    recipient.reflectAttributeValues(
        makeObjectInstanceHandle(reflection->objectInstanceHandle),
        attributeValues,
        VariableLengthData{},
        transportationHandleFromEmbeddedName(
            "HLAreliable",
            L"The embedded federation could not reconstruct the MOM transportation type."),
        FederateHandle{},
        nullptr);
  });
}

void queueJoinedFederateMomConditionalAttributeUpdate(
    std::wstring const& federationName,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> requestedAttributeHandles,
    std::optional<std::uint64_t> excludedReceivingFederateId = std::nullopt) {
  umbra::detail::JoinedFederateMomAttributeValueUpdateClassPlan plan;
  {
    std::scoped_lock lock(federationManagementMutex());
    plan = embeddedFederationManagement().registry()
               .planJoinedFederateMomAttributeValueUpdateForObject(
                   federationName,
                   objectInstanceHandle,
                   requestedAttributeHandles,
                   true,
                   excludedReceivingFederateId);
  }
  for (auto& recipient : plan.recipients) {
    if (!recipient.callbackRoute || recipient.attributeValues.empty()) {
      continue;
    }
    queueJoinedFederateMomAttributeValueUpdate(
        std::move(recipient.callbackRoute),
        federationName,
        recipient.receivingFederateId,
        recipient.objectInstanceHandle,
        requestedAttributeHandles,
        true,
        std::move(recipient.attributeValues));
  }
}

struct JoinedFederateMomConditionalWork {
  std::wstring federationName;
  std::uint64_t objectInstanceHandle = 0;
  std::set<std::uint64_t> attributeHandles;
};

std::optional<JoinedFederateMomConditionalWork>
joinedFederateMomConditionalWorkFor(
    umbra::detail::EmbeddedFederationRegistry& registry,
    std::wstring const& federationName,
    std::uint64_t joinedFederateId,
    std::vector<std::string_view> attributeNames) {
  auto const object = registry.joinedFederateMomObjectFor(
      federationName,
      joinedFederateId);
  if (!object) {
    return std::nullopt;
  }
  JoinedFederateMomConditionalWork result;
  result.federationName = federationName;
  result.objectInstanceHandle = object->objectInstanceHandle;
  for (std::string_view const attributeName : attributeNames) {
    auto const handle = registry.attributeHandleFor(
        federationName,
        "HLAobjectRoot.HLAmanager.HLAfederate",
        std::string(attributeName));
    if (!handle) {
      return std::nullopt;
    }
    result.attributeHandles.insert(*handle);
  }
  return result;
}

std::optional<JoinedFederateMomConditionalWork>
federationMomConditionalWorkFor(
    umbra::detail::EmbeddedFederationRegistry& registry,
    std::wstring const& federationName,
    std::vector<std::string_view> attributeNames) {
  auto const object = registry.federationMomObjectFor(federationName);
  if (!object) {
    return std::nullopt;
  }
  JoinedFederateMomConditionalWork result;
  result.federationName = federationName;
  result.objectInstanceHandle = object->objectInstanceHandle;
  for (std::string_view const attributeName : attributeNames) {
    auto const handle = registry.attributeHandleFor(
        federationName,
        "HLAobjectRoot.HLAmanager.HLAfederation",
        std::string(attributeName));
    if (!handle) {
      return std::nullopt;
    }
    result.attributeHandles.insert(*handle);
  }
  return result;
}

void queueJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
    std::wstring const& federationName,
    std::uint64_t joinedFederateId,
    std::vector<std::string_view> attributeNames,
    std::optional<std::uint64_t> excludedReceivingFederateId) {
  std::optional<JoinedFederateMomConditionalWork> work;
  {
    std::scoped_lock lock(federationManagementMutex());
    work = joinedFederateMomConditionalWorkFor(
        embeddedFederationManagement().registry(),
        federationName,
        joinedFederateId,
        std::move(attributeNames));
  }
  if (work) {
    queueJoinedFederateMomConditionalAttributeUpdate(
        work->federationName,
        work->objectInstanceHandle,
        std::move(work->attributeHandles),
        excludedReceivingFederateId);
  }
}

void queueFederationMomConditionalAttributeUpdate(
    std::wstring const& federationName,
    std::vector<std::string_view> attributeNames,
    std::optional<std::uint64_t> excludedReceivingFederateId = std::nullopt) {
  std::optional<JoinedFederateMomConditionalWork> work;
  {
    std::scoped_lock lock(federationManagementMutex());
    work = federationMomConditionalWorkFor(
        embeddedFederationManagement().registry(),
        federationName,
        std::move(attributeNames));
  }
  if (work) {
    queueJoinedFederateMomConditionalAttributeUpdate(
        work->federationName,
        work->objectInstanceHandle,
        std::move(work->attributeHandles),
        excludedReceivingFederateId);
  }
}

// HLA_EVOKED applications expose callback delivery through the Evoke
// services, so the embedded profile claims due wall-clock periods at the
// callback boundary rather than running user code from a registry timer
// thread.  The registry owns each target's immutable deadline and advances it
// once; this helper only applies the ordinary active-subscription MOM planner
// after releasing the federation lock.
void pumpDueJoinedFederateMomPeriodicUpdates(std::wstring const& federationName) {
  std::vector<umbra::detail::JoinedFederateMomPeriodicUpdate> due;
  {
    std::scoped_lock lock(federationManagementMutex());
    due = embeddedFederationManagement().registry()
        .takeDueJoinedFederateMomPeriodicUpdates(
            federationName,
            std::chrono::steady_clock::now());
  }
  for (auto const& update : due) {
    if (update.objectInstanceHandle == 0U || update.attributeHandles.empty()) {
      continue;
    }
    queueJoinedFederateMomConditionalAttributeUpdate(
        federationName,
        update.objectInstanceHandle,
        update.attributeHandles);
  }
}

void queueObjectInstanceScopeChanges(
    std::vector<umbra::detail::ObjectInstanceScopeChangeRecipient> changes,
    std::wstring const& federationName) {
  for (auto& change : changes) {
    // Scope-transition planning is deliberately independent from Attribute
    // Relevance planning. Avoid enqueueing a no-op Attributes In/Out Of Scope
    // callback when the receiving federate has its separate Attribute Scope
    // Advisory switch disabled; the callback-time check below still protects
    // the enabled case from stale queued state.
    {
      std::scoped_lock lock(federationManagementMutex());
      auto const scopeSwitch = embeddedFederationManagement().registry()
          .attributeScopeAdvisorySwitchFor(federationName, change.receivingFederateId);
      if (!scopeSwitch || !*scopeSwitch) {
        continue;
      }
    }
    if (!change.callbackRoute) {
      throw RTIinternalError(
          L"The embedded federation has an object-instance scope change without a callback route.");
    }
    change.callbackRoute([
        federationName,
        receivingFederateId = change.receivingFederateId,
        objectInstanceHandle = change.objectInstanceHandle,
        scheduledAttributeHandles = std::move(change.attributeHandles),
        expectedInScope = change.inScope](FederateAmbassador& recipient) mutable {
      std::set<std::uint64_t> eligibleAttributeHandles;
      {
        std::scoped_lock lock(federationManagementMutex());
        eligibleAttributeHandles = embeddedFederationManagement().registry()
            .objectInstanceScopeAttributes(
                federationName,
                receivingFederateId,
                objectInstanceHandle,
                scheduledAttributeHandles,
                expectedInScope);
      }
      if (eligibleAttributeHandles.empty()) {
        return;
      }

      AttributeHandleSet attributes;
      for (std::uint64_t const attributeHandle : eligibleAttributeHandles) {
        attributes.insert(makeAttributeHandle(attributeHandle));
      }
      if (expectedInScope) {
        recipient.attributesInScope(
            makeObjectInstanceHandle(objectInstanceHandle),
            attributes);
      } else {
        recipient.attributesOutOfScope(
            makeObjectInstanceHandle(objectInstanceHandle),
            attributes);
      }
    });
  }
}

void queueAttributeRelevanceAdvisories(
    std::vector<umbra::detail::AttributeRelevanceAdvisoryRecipient> advisories,
    std::wstring const& federationName) {
  for (auto& advisory : advisories) {
    if (!advisory.callbackRoute) {
      throw RTIinternalError(
          L"The embedded federation has an attribute relevance advisory without a callback route.");
    }
    advisory.callbackRoute([
        federationName,
        providingFederateId = advisory.providingFederateId,
        receivingFederateId = advisory.receivingFederateId,
        objectInstanceHandle = advisory.objectInstanceHandle,
        scheduledAttributeHandles = std::move(advisory.attributeHandles),
        turnUpdatesOn = advisory.turnUpdatesOn](
        FederateAmbassador& recipient) mutable {
      std::map<std::optional<std::string>, std::set<std::uint64_t>>
          eligibleAttributeHandlesByUpdateRate;
      {
        std::scoped_lock lock(federationManagementMutex());
        auto& registry = embeddedFederationManagement().registry();
        auto const eligibleAttributeHandles = registry
            .attributeRelevanceAdvisoryAttributes(
                federationName,
                providingFederateId,
                receivingFederateId,
                objectInstanceHandle,
                scheduledAttributeHandles,
                turnUpdatesOn);
        for (std::uint64_t const attributeHandle : eligibleAttributeHandles) {
          auto const updateRateDesignator = turnUpdatesOn
              ? registry.attributeRelevanceAdvisoryUpdateRateDesignatorFor(
                    federationName,
                    receivingFederateId,
                    objectInstanceHandle,
                    attributeHandle)
              : std::nullopt;
          eligibleAttributeHandlesByUpdateRate[updateRateDesignator].insert(
              attributeHandle);
        }
      }
      if (eligibleAttributeHandlesByUpdateRate.empty()) {
        return;
      }

      for (auto const& [updateRateDesignator, eligibleAttributeHandles] :
           eligibleAttributeHandlesByUpdateRate) {
        AttributeHandleSet attributes;
        for (std::uint64_t const attributeHandle : eligibleAttributeHandles) {
          attributes.insert(makeAttributeHandle(attributeHandle));
        }
        if (turnUpdatesOn) {
          if (updateRateDesignator) {
            auto const wideDesignator = umbra::detail::wideFromUtf8(*updateRateDesignator);
            if (!wideDesignator) {
              throw RTIinternalError(
                  L"The embedded federation retained an invalid UTF-8 update-rate designator.");
            }
            recipient.turnUpdatesOnForObjectInstance(
                makeObjectInstanceHandle(objectInstanceHandle),
                attributes,
                *wideDesignator);
          } else {
            recipient.turnUpdatesOnForObjectInstance(
                makeObjectInstanceHandle(objectInstanceHandle),
                attributes);
          }
        } else {
          recipient.turnUpdatesOffForObjectInstance(
              makeObjectInstanceHandle(objectInstanceHandle),
              attributes);
        }
      }
    });
  }
}

void queueObjectInstanceRemoval(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    umbra::detail::FederateServiceReportRoute serviceReportRoute,
    std::wstring federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    VariableLengthData userSuppliedTag,
    bool rtiOwnedMomObject) {
  submitReceiveOrderCallback(
      std::move(callbackRoute),
      federationName,
      receivingFederateId,
      [
      federationName,
      receivingFederateId,
      objectInstanceHandle,
      rtiOwnedMomObject,
      userSuppliedTag = std::move(userSuppliedTag),
      serviceReportRoute = std::move(serviceReportRoute)](FederateAmbassador& recipient) mutable {
    std::optional<umbra::detail::RemovedObjectInstanceSnapshot> removal;
    {
      std::scoped_lock lock(federationManagementMutex());
      auto& registry = embeddedFederationManagement().registry();
      removal = rtiOwnedMomObject
          ? registry.beginJoinedFederateMomObjectRemoval(
                federationName,
                receivingFederateId,
                objectInstanceHandle)
          : registry.beginObjectInstanceRemoval(
                federationName,
                receivingFederateId,
                objectInstanceHandle);
    }
    if (!removal) {
      return;
    }

    if (serviceReportRoute) {
      // §6.17 is RTI-initiated at the receiving federate. The callback-time
      // transition above is therefore the only point at which a selected-file
      // report may reserve a serial and become durable.
      serviceReportRoute(
          static_cast<std::uint16_t>(umbra::detail::MomServiceType::object_management),
          [
              objectInstanceHandle = removal->objectInstanceHandle,
              userSuppliedTag,
              producingFederateId = removal->producingFederateId](
              std::uint32_t serialNumber) {
            return removeObjectInstanceServiceReportRecord(
                objectInstanceHandle,
                userSuppliedTag,
                producingFederateId,
                RECEIVE,
                nullptr,
                std::nullopt,
                std::nullopt,
                serialNumber);
          });
    }

    // Removal commits the recipient's transition to unknown before its
    // callback. That mirrors the lifecycle boundary rather than preserving a
    // stale instance through user code after Remove Object Instance begins.
    recipient.removeObjectInstance(
        makeObjectInstanceHandle(removal->objectInstanceHandle),
        userSuppliedTag,
        rtiOwnedMomObject
            ? FederateHandle{}
            : makeFederateHandle(removal->producingFederateId));
  });
}

void queueObjectInstanceRemovals(
    std::vector<umbra::detail::ObjectInstanceRemovalRecipient> removals,
    std::wstring const& federationName,
    VariableLengthData const& userSuppliedTag) {
  // As with discovery, callback submission runs after all sender and
  // federation locks are released. HLA_IMMEDIATE may synchronously enter a
  // recipient's Remove Object Instance callback.
  for (std::size_t index = 0; index < removals.size(); ++index) {
    auto& removal = removals[index];
    try {
      if (!removal.callbackRoute) {
        throw RTIinternalError(
            L"The embedded federation has a removal recipient without a callback route.");
      }
      queueObjectInstanceRemoval(
          std::move(removal.callbackRoute),
          std::move(removal.serviceReportRoute),
          federationName,
          removal.receivingFederateId,
          removal.objectInstanceHandle,
          userSuppliedTag,
          removal.rtiOwnedMomObject);
    } catch (...) {
      // A synchronous callback may already have removed the current
      // recipient's known state. Clear only still-pending reservations so the
      // remaining known recipients are not stranded in an in-flight state.
      std::scoped_lock lock(federationManagementMutex());
      auto& registry = embeddedFederationManagement().registry();
      for (std::size_t pending = index; pending < removals.size(); ++pending) {
        if (removals[pending].rtiOwnedMomObject) {
          registry.cancelJoinedFederateMomObjectRemoval(
              federationName,
              removals[pending].receivingFederateId,
              removals[pending].objectInstanceHandle);
        } else {
          registry.cancelObjectInstanceRemoval(
              federationName,
              removals[pending].receivingFederateId,
              removals[pending].objectInstanceHandle);
        }
      }
      throw;
    }
  }
}

void queueTimestampedObjectInstanceRemoval(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    umbra::detail::FederateServiceReportRoute serviceReportRoute,
    std::wstring federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t messageId,
    VariableLengthData userSuppliedTag,
    std::shared_ptr<LogicalTime const> timestamp,
    bool provideRetraction) {
  if (!callbackRoute || !timestamp || messageId == 0) {
    throw RTIinternalError(
        L"The embedded federation has an incomplete timestamped object-removal payload.");
  }
  submitReceiveOrderCallback(
      std::move(callbackRoute),
      federationName,
      receivingFederateId,
      [
      federationName,
      receivingFederateId,
      objectInstanceHandle,
      messageId,
      userSuppliedTag = std::move(userSuppliedTag),
      timestamp = std::move(timestamp),
      provideRetraction,
      serviceReportRoute = std::move(serviceReportRoute)](FederateAmbassador& recipient) mutable {
    std::optional<umbra::detail::RemovedObjectInstanceSnapshot> removal;
    {
      std::scoped_lock lock(federationManagementMutex());
      removal = embeddedFederationManagement().registry().beginTsoObjectInstanceRemoval(
          federationName,
          receivingFederateId,
          objectInstanceHandle,
          messageId);
    }
    if (!removal) {
      return;
    }

    if (serviceReportRoute) {
      // This path is timestamped at the sending federate, but reaches a
      // non-time-constrained recipient through its receive-order callback
      // boundary. Record both order types explicitly before user code runs.
      serviceReportRoute(
          static_cast<std::uint16_t>(umbra::detail::MomServiceType::object_management),
          [
              objectInstanceHandle = removal->objectInstanceHandle,
              userSuppliedTag,
              producingFederateId = removal->producingFederateId,
              timestamp,
              messageId,
              provideRetraction](std::uint32_t serialNumber) {
            return removeObjectInstanceServiceReportRecord(
                objectInstanceHandle,
                userSuppliedTag,
                producingFederateId,
                TIMESTAMP,
                timestamp.get(),
                RECEIVE,
                provideRetraction
                    ? std::optional<std::uint64_t>{messageId}
                    : std::nullopt,
                serialNumber);
          });
    }

    std::optional<MessageRetractionHandle> retraction;
    if (provideRetraction) {
      retraction.emplace(makeMessageRetractionHandle(messageId));
    }
    recipient.removeObjectInstance(
        makeObjectInstanceHandle(removal->objectInstanceHandle),
        userSuppliedTag,
        makeFederateHandle(removal->producingFederateId),
        *timestamp,
        TIMESTAMP,
        RECEIVE,
        retraction ? &*retraction : nullptr);
  });
}

umbra::detail::FederationTimeGrantDispatch makeTimeAdvanceGrantDispatch(
    std::shared_ptr<umbra::detail::CallbackDispatcher> const& callbackDispatcher,
    std::shared_ptr<CallbackSession> const& callbackSession,
    std::shared_ptr<umbra::detail::FederateTimeState> const& timeState,
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t generation,
    std::uint64_t dispatchIdentity) {
  std::weak_ptr<umbra::detail::CallbackDispatcher> const dispatcher = callbackDispatcher;
  std::weak_ptr<CallbackSession> const session = callbackSession;
  std::weak_ptr<umbra::detail::FederateTimeState> const federateTimeState = timeState;

  return [
      dispatcher,
      session,
      federateTimeState,
      federationName = std::move(federationName),
      federateId,
      generation,
      dispatchIdentity] {
    auto callbackDispatcher = dispatcher.lock();
    if (!callbackDispatcher) {
      return;
    }

    callbackDispatcher->submit([
        session,
        federateTimeState,
        federationName,
        federateId,
        generation,
        dispatchIdentity] {
      auto callbackSession = session.lock();
      auto timeState = federateTimeState.lock();
      if (!callbackSession || !timeState) {
        return;
      }

      std::vector<umbra::detail::FederationTimeGrantDispatch> newlyEligible;
      std::vector<umbra::detail::FederationSaveNotification> immediateSaveNotifications;
      std::vector<umbra::detail::FederationSaveNotification> timedSaveNotifications;
      std::vector<umbra::detail::ObjectInstanceRemovalRecipient>
          postGrantObjectRemovals;
      bool grantCompleted = false;
      callbackSession->invoke([
          timeState = std::move(timeState),
          federationName,
          federateId,
          generation,
          dispatchIdentity,
          &newlyEligible,
          &immediateSaveNotifications,
          &timedSaveNotifications,
          &postGrantObjectRemovals,
          &grantCompleted](FederateAmbassador& recipient) {
        auto const pendingSnapshot = timeState->snapshot();
        bool const flushQueue =
            pendingSnapshot.advanceMode ==
            umbra::detail::FederateTimeAdvanceMode::flush_queue_request;
        std::shared_ptr<LogicalTime const> grantedTime;
        std::shared_ptr<LogicalTime const> optimisticTime;
        std::shared_ptr<LogicalTime> flushQueueGrantedTime;
        std::shared_ptr<LogicalTime> flushQueueOptimisticTime;
        std::vector<umbra::detail::TsoPayloadDelivery> tsoDeliveries;
        std::optional<std::wstring> immediateSaveLabel;
        if (!flushQueue) {
          {
            std::scoped_lock lock(federationManagementMutex());
            auto& registry = embeddedFederationManagement().registry();
            if (registry.beginTimeAdvanceGrant(
                    federationName,
                    federateId,
                    generation,
                    dispatchIdentity) !=
                umbra::detail::FederationTimeGrantStatus::applied) {
              return;
            }
            auto admission = registry.admitImmediateFederationSaveAtTimeAdvanceBoundary(
                federationName,
                federateId);
            if (admission.status != umbra::detail::FederationSaveControlStatus::applied) {
              throw RTIinternalError(
                  L"The embedded federation could not admit an untimed save at the time-advance boundary.");
            }
            immediateSaveLabel = std::move(admission.currentFederateLabel);
            immediateSaveNotifications = std::move(admission.notifications);
          }

          // Initiate Federate Save must reach a time-constrained recipient
          // while its private temporal state is still Time Advancing. This is
          // deliberately a direct pre-grant callback rather than ordinary
          // queued federation-control work, whose FIFO position could follow
          // this recipient's Time Advance Grant.
          if (immediateSaveLabel.has_value()) {
            recipient.initiateFederateSave(*immediateSaveLabel);
            queueJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
                federationName,
                federateId,
                {"HLAfederateState"},
                federateId);
          }

          {
            std::scoped_lock lock(federationManagementMutex());
            auto& registry = embeddedFederationManagement().registry();
            if (!pendingSnapshot.requestedTime ||
                pendingSnapshot.requestedTime->implementationName() !=
                    pendingSnapshot.implementationName) {
              throw RTIinternalError(
                  L"The embedded Time Advance Grant has incomplete temporal state.");
            }
            // Keep the federate Time Advancing while its TSO payloads and a
            // possible timestamped Initiate Federate Save are delivered. The
            // actual private grant mutation occurs below immediately before
            // the matching Time Advance Grant callback.
            grantedTime = cloneReferenceLogicalTime(
                pendingSnapshot.implementationName,
                *pendingSnapshot.requestedTime);
            auto tso = registry.beginTsoPayloadDelivery(
                federationName,
                federateId,
                *grantedTime,
                true);
            if (tso.status != umbra::detail::FederationTsoRegistryStatus::applied ||
                (tso.deliveryStatus != umbra::detail::FederationTsoDeliveryStatus::applied &&
                 tso.deliveryStatus != umbra::detail::FederationTsoDeliveryStatus::no_messages)) {
              throw RTIinternalError(
                  L"The embedded federation could not begin the timestamped delivery boundary.");
            }
            tsoDeliveries = std::move(tso.deliveries);
          }
        } else {
          std::scoped_lock lock(federationManagementMutex());
          auto& registry = embeddedFederationManagement().registry();
          if (!pendingSnapshot.currentTime || !pendingSnapshot.requestedTime ||
                pendingSnapshot.currentTime->implementationName() !=
                    pendingSnapshot.implementationName ||
                pendingSnapshot.requestedTime->implementationName() !=
                    pendingSnapshot.implementationName) {
              throw RTIinternalError(
                  L"The embedded Flush Queue Request has incomplete temporal state.");
            }

            auto execution = registry.timeSnapshotFor(federationName);
            if (!execution) {
              throw FederateNotExecutionMember(
                  L"The embedded federation no longer records the Flush Queue requester.");
            }
            umbra::detail::FederationFlushQueueGrantCalculator flushQueueGrantCalculator;
            auto flushQueueCalculation = flushQueueGrantCalculator.calculate(
                *execution,
                federateId);
            if (!flushQueueCalculation.calculated()) {
              throw RTIinternalError(
                  L"The embedded federation could not calculate the Flush Queue Grant times.");
            }

            auto factory = HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
                pendingSnapshot.implementationName);
            if (!factory || factory->getName() != pendingSnapshot.implementationName) {
              throw RTIinternalError(
                  L"The embedded federation could not create the Flush Queue time factory.");
            }
            auto finalTime = factory->makeFinal();
            if (!finalTime || finalTime->implementationName() != pendingSnapshot.implementationName) {
              throw RTIinternalError(
                  L"The embedded federation could not create the Flush Queue boundary.");
            }

            if (registry.beginTimeAdvanceGrant(
                    federationName,
                    federateId,
                    generation,
                    dispatchIdentity) !=
                umbra::detail::FederationTimeGrantStatus::applied) {
              return;
            }
            auto tso = registry.beginTsoPayloadDelivery(
                federationName,
                federateId,
                *finalTime,
                true);
            if (tso.status != umbra::detail::FederationTsoRegistryStatus::applied ||
                (tso.deliveryStatus != umbra::detail::FederationTsoDeliveryStatus::applied &&
                 tso.deliveryStatus != umbra::detail::FederationTsoDeliveryStatus::no_messages)) {
              throw RTIinternalError(
                  L"The embedded federation could not flush the timestamped delivery queue.");
            }
            tsoDeliveries = std::move(tso.deliveries);
            // Preserve Time Advancing through queued TSO delivery and any
            // timestamped-save admission. The actual mutation to the FQR
            // grant time is deliberately deferred until the direct callback
            // boundary below.
            flushQueueGrantedTime = std::move(flushQueueCalculation.grantedTime);
            flushQueueOptimisticTime = std::move(flushQueueCalculation.optimisticTime);
        }

        // TSO callbacks are delivered before the matching Time Advance Grant
        // callback. Each entry is completed even when user code throws, so a
        // callback exception cannot leave a private in-transit reservation.
        for (auto const& delivery : tsoDeliveries) {
          try {
            std::visit(
                [&recipient, &federationName, federateId](auto const& typedDelivery) {
                  using Delivery = std::decay_t<decltype(typedDelivery)>;
                  if constexpr (std::is_same_v<
                                    Delivery,
                                    umbra::detail::TsoInteractionDelivery>) {
                    auto const& message = typedDelivery.message;
                    std::optional<umbra::detail::ReceiveOrderInteractionRecipient>
                        projection;
                    bool callbackMayBegin = false;
                    {
                      std::scoped_lock lock(federationManagementMutex());
                      auto& registry = embeddedFederationManagement().registry();
                      projection = registry.receiveOrderInteractionRecipientFor(
                              federationName,
                              message.producingFederateId,
                              federateId,
                              message.sentInteractionClassHandle,
                              message.sentParameterHandles,
                              message.sentRegionHandles.empty()
                                  ? nullptr
                                  : &message.sentRegionHandles);
                      if (projection) {
                        callbackMayBegin = registry.beginTsoInteractionCallback(
                            federationName,
                            federateId,
                            message.messageId);
                      }
                    }
                    if (!projection || !callbackMayBegin) {
                      std::scoped_lock lock(federationManagementMutex());
                      static_cast<void>(embeddedFederationManagement().registry()
                                            .finishTsoRecipientCallbackSuppressed(
                                                federationName,
                                                federateId,
                                                message.messageId));
                      return;
                    }

                    if (!message.timestamp) {
                      throw RTIinternalError(
                          L"The embedded federation could not reconstruct a timestamped interaction callback.");
                    }
                    ParameterHandleValueMap parameterValues = projectInteractionParameterValues(
                        message.parameters,
                        projection->receivedParameterHandles);
                    auto const transportationName = umbra::detail::wideFromUtf8(
                        message.transportationName);
                    auto const transportationValue = transportationName
                        ? standardTransportationTypeValue(*transportationName)
                        : std::nullopt;
                    if (!transportationValue) {
                      throw RTIinternalError(
                          L"The embedded federation could not reconstruct a timestamped interaction callback transportation type.");
                    }
                    std::optional<RegionHandleSet> optionalSentRegions;
                    if (projection->conveyRegionDesignatorSets &&
                        (!message.sentRegionHandles.empty() || message.defaultRegionUsed)) {
                      optionalSentRegions.emplace();
                      for (auto const regionHandle : message.sentRegionHandles) {
                        optionalSentRegions->insert(makeRegionHandle(regionHandle));
                      }
                    }
                    auto retraction = makeMessageRetractionHandle(message.messageId);
                    recipient.receiveInteraction(
                        makeInteractionClassHandle(projection->receivedInteractionClassHandle),
                        parameterValues,
                        message.userSuppliedTag,
                        makeTransportationTypeHandle(*transportationValue),
                        makeFederateHandle(message.producingFederateId),
                        optionalSentRegions ? &*optionalSentRegions : nullptr,
                        *message.timestamp,
                        message.sentOrderType,
                        message.receivedOrderType,
                        &retraction);
                  } else if constexpr (std::is_same_v<
                                           Delivery,
                                           umbra::detail::TsoAttributeUpdateDelivery>) {
                    auto const& message = typedDelivery.message;
                    if (!message.timestamp) {
                      throw RTIinternalError(
                          L"The embedded federation could not reconstruct a timestamped attribute callback.");
                    }
                    auto const passels = message.passelsByRecipient.find(federateId);
                    if (passels == message.passelsByRecipient.end()) {
                      return;
                    }
                    auto retraction = makeMessageRetractionHandle(message.messageId);
                    bool callbackBegan = false;
                    for (auto const& passel : passels->second) {
                      std::optional<umbra::detail::ReceiveOrderAttributeUpdateRecipient>
                          projection;
                      bool callbackMayBegin = false;
                      {
                        std::scoped_lock lock(federationManagementMutex());
                        auto& registry = embeddedFederationManagement().registry();
                        projection = registry
                            .receiveOrderAttributeUpdateRecipientFor(
                                federationName,
                                message.producingFederateId,
                                federateId,
                                message.objectInstanceHandle,
                                passel.sentAttributeHandles,
                                passel.sentRegionHandles.empty()
                                    ? nullptr
                                    : &passel.sentRegionHandles);
                      }
                      if (!projection) {
                        continue;
                      }

                      auto const transportationName = umbra::detail::wideFromUtf8(
                          passel.transportationName);
                      auto const transportationValue = transportationName
                          ? standardTransportationTypeValue(*transportationName)
                          : std::nullopt;
                      if (!transportationValue) {
                        throw RTIinternalError(
                            L"The embedded federation could not reconstruct a timestamped attribute callback transportation type.");
                      }
                      AttributeHandleValueMap attributeValues = projectAttributeValues(
                          message.attributes,
                          projection->receivedAttributeHandles);
                      // Update-rate reduction is a delivery-boundary policy.
                      // The queued TSO path must apply the same per-attribute
                      // gate as the immediate timestamped path; otherwise a
                      // time-constrained recipient would bypass the FDD rate
                      // selected by its subscription.
                      if (passel.transportationName != "HLAreliable") {
                        for (auto iterator = attributeValues.begin();
                             iterator != attributeValues.end();) {
                          std::uint64_t numeric = 0;
                          for (auto const candidate : passel.sentAttributeHandles) {
                            if (makeAttributeHandle(candidate) == iterator->first) {
                              numeric = candidate;
                              break;
                            }
                          }
                          auto const rate = projection->maximumUpdateRatesByAttribute.find(
                              numeric);
                          auto const key = federationName + L"/" +
                              std::to_wstring(federateId) + L"/" +
                              std::to_wstring(message.objectInstanceHandle) + L"/" +
                              std::to_wstring(projection->subscriptionGeneration) + L"/" +
                              std::to_wstring(numeric);
                          auto const encoded = umbra::detail::utf8FromWide(key);
                          bool const admitted = !encoded || updateRateGate().admit(
                              *encoded,
                              rate == projection->maximumUpdateRatesByAttribute.end()
                                  ? projection->maximumUpdateRate
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
                      {
                        std::scoped_lock lock(federationManagementMutex());
                        callbackMayBegin = embeddedFederationManagement().registry()
                            .beginTsoAttributeUpdateCallback(
                                federationName,
                                federateId,
                                message.messageId);
                      }
                      if (!callbackMayBegin) {
                        return;
                      }

                      callbackBegan = true;
                      {
                        std::scoped_lock lock(federationManagementMutex());
                        static_cast<void>(embeddedFederationManagement().registry()
                                              .recordSuccessfulObjectInstanceReflection(
                                                  federationName,
                                                  federateId,
                                                  message.objectInstanceHandle));
                      }

                      std::optional<RegionHandleSet> optionalSentRegions;
                      if (projection->conveyRegionDesignatorSets &&
                          (!passel.sentRegionHandles.empty() || passel.defaultRegionUsed)) {
                        optionalSentRegions.emplace();
                        for (std::uint64_t const regionHandle : passel.sentRegionHandles) {
                          optionalSentRegions->insert(makeRegionHandle(regionHandle));
                        }
                      }
                      recipient.reflectAttributeValues(
                          makeObjectInstanceHandle(message.objectInstanceHandle),
                          attributeValues,
                          message.userSuppliedTag,
                          makeTransportationTypeHandle(*transportationValue),
                          makeFederateHandle(message.producingFederateId),
                          optionalSentRegions ? &*optionalSentRegions : nullptr,
                          *message.timestamp,
                          passel.preferredOrderType,
                          TIMESTAMP,
                          &retraction);
                    }
                    if (!callbackBegan) {
                      std::scoped_lock lock(federationManagementMutex());
                      static_cast<void>(embeddedFederationManagement().registry()
                                            .finishTsoRecipientCallbackSuppressed(
                                                federationName,
                                                federateId,
                                                message.messageId));
                    }
                  } else if constexpr (std::is_same_v<
                                           Delivery,
                                           umbra::detail::TsoObjectDeletionDelivery>) {
                    auto const& message = typedDelivery.message;
                    if (!message.timestamp) {
                      throw RTIinternalError(
                          L"The embedded federation could not reconstruct a timestamped object-removal callback.");
                    }
                    auto const target = std::find_if(
                        message.recipients.begin(),
                        message.recipients.end(),
                        [federateId](umbra::detail::TsoObjectDeletionRecipient const& candidate) {
                          return candidate.receivingFederateId == federateId;
                        });
                    if (target == message.recipients.end()) {
                      return;
                    }

                    std::optional<umbra::detail::RemovedObjectInstanceSnapshot> removal;
                    {
                      std::scoped_lock lock(federationManagementMutex());
                      removal = embeddedFederationManagement().registry()
                          .beginTsoObjectInstanceRemoval(
                              federationName,
                              federateId,
                              message.objectInstanceHandle,
                              message.messageId);
                    }
                    if (!removal) {
                      return;
                    }

                    if (target->serviceReportRoute) {
                      // The TSO delivery boundary has now committed this
                      // recipient's removal. Its selected report file must
                      // receive the full timestamped §6.17 form before the
                      // recipient can observe the callback.
                      target->serviceReportRoute(
                          static_cast<std::uint16_t>(
                              umbra::detail::MomServiceType::object_management),
                          [
                              objectInstanceHandle = removal->objectInstanceHandle,
                              userSuppliedTag = message.userSuppliedTag,
                              producingFederateId = removal->producingFederateId,
                              timestamp = message.timestamp,
                              messageId = message.messageId](std::uint32_t serialNumber) {
                            return removeObjectInstanceServiceReportRecord(
                                objectInstanceHandle,
                                userSuppliedTag,
                                producingFederateId,
                                TIMESTAMP,
                                timestamp.get(),
                                TIMESTAMP,
                                messageId,
                                serialNumber);
                          });
                    }

                    auto retraction = makeMessageRetractionHandle(message.messageId);
                    recipient.removeObjectInstance(
                        makeObjectInstanceHandle(removal->objectInstanceHandle),
                        message.userSuppliedTag,
                        makeFederateHandle(removal->producingFederateId),
                        *message.timestamp,
                        TIMESTAMP,
                        TIMESTAMP,
                        &retraction);
                  } else {
                    auto const& message = typedDelivery.message;
                    if (!message.timestamp) {
                      throw RTIinternalError(
                          L"The embedded federation could not reconstruct a timestamped directed-interaction callback.");
                    }
                    auto const target = std::find_if(
                        message.recipients.begin(),
                        message.recipients.end(),
                        [federateId](umbra::detail::TsoDirectedInteractionRecipient const& candidate) {
                          return candidate.receivingFederateId == federateId;
                        });
                    if (target == message.recipients.end()) {
                      return;
                    }
                    auto const transportationName = umbra::detail::wideFromUtf8(
                        message.transportationName);
                    auto const transportationValue = transportationName
                        ? standardTransportationTypeValue(*transportationName)
                        : std::nullopt;
                    if (!transportationValue) {
                      throw RTIinternalError(
                          L"The embedded federation could not reconstruct a timestamped directed-interaction transportation type.");
                    }
                    std::optional<umbra::detail::ReceiveOrderDirectedInteractionRecipient>
                        projection;
                    bool callbackMayBegin = false;
                    {
                      std::scoped_lock lock(federationManagementMutex());
                      auto& registry = embeddedFederationManagement().registry();
                      projection = registry.timestampedDirectedInteractionRecipientFor(
                          federationName,
                          message.producingFederateId,
                          federateId,
                          message.objectInstanceHandle,
                          message.sentInteractionClassHandle,
                          message.sentParameterHandles,
                          message.messageId);
                      if (projection) {
                        callbackMayBegin = registry.beginTsoInteractionCallback(
                            federationName,
                            federateId,
                            message.messageId);
                      }
                    }
                    if (!projection || !callbackMayBegin) {
                      std::scoped_lock lock(federationManagementMutex());
                      static_cast<void>(embeddedFederationManagement().registry()
                                            .finishTsoRecipientCallbackSuppressed(
                                                federationName,
                                                federateId,
                                                message.messageId));
                      return;
                    }
                    ParameterHandleValueMap parameterValues = projectInteractionParameterValues(
                        message.parameters,
                        projection->receivedParameterHandles);
                    auto retraction = makeMessageRetractionHandle(message.messageId);
                    recipient.receiveDirectedInteraction(
                        makeInteractionClassHandle(projection->receivedInteractionClassHandle),
                        makeObjectInstanceHandle(projection->objectInstanceHandle),
                        parameterValues,
                        message.userSuppliedTag,
                        makeTransportationTypeHandle(*transportationValue),
                        makeFederateHandle(message.producingFederateId),
                        *message.timestamp,
                        message.sentOrderType,
                        message.receivedOrderType,
                        &retraction);
                  }
                },
                delivery);
          } catch (...) {
            std::scoped_lock lock(federationManagementMutex());
            static_cast<void>(embeddedFederationManagement().registry().completeTsoDelivery(
                federationName,
                std::visit(
                    [](auto const& typedDelivery) {
                      return typedDelivery.queuedMessage;
                    },
                    delivery)));
            throw;
          }

          std::scoped_lock lock(federationManagementMutex());
          auto& registry = embeddedFederationManagement().registry();
          auto const completed = registry.completeTsoDelivery(
              federationName,
              std::visit(
                  [](auto const& typedDelivery) {
                    return typedDelivery.queuedMessage;
                  },
                  delivery));
          if (completed.status != umbra::detail::FederationTsoRegistryStatus::applied ||
              (completed.delivery.status != umbra::detail::FederationTsoDeliveryStatus::applied &&
               completed.delivery.status !=
                   umbra::detail::FederationTsoDeliveryStatus::message_already_completed)) {
            throw RTIinternalError(
                L"The embedded federation could not complete the timestamped delivery.");
          }
          auto releasedRemovals = registry
              .releaseConnectionLossDeferredObjectInstanceRemovals(
                  federationName,
                  federateId);
          for (auto& removal : releasedRemovals) {
            postGrantObjectRemovals.push_back(std::move(removal));
          }
        }

        if (!flushQueue) {
          std::optional<std::wstring> timedSaveLabel;
          std::shared_ptr<LogicalTime const> timedSaveTimestamp;
          {
            std::scoped_lock lock(federationManagementMutex());
            auto admission = embeddedFederationManagement().registry()
                .admitTimedFederationSaveAtTimeAdvanceBoundary(
                    federationName,
                    federateId);
            if (admission.status != umbra::detail::FederationSaveControlStatus::applied) {
              throw RTIinternalError(
                  L"The embedded federation could not admit a timestamped save at the time-advance boundary.");
            }
            timedSaveLabel = std::move(admission.currentFederateLabel);
            timedSaveTimestamp = std::move(admission.currentFederateTimestamp);
            timedSaveNotifications = std::move(admission.notifications);
          }

          // The timestamped path reaches this point only after the recipient
          // has received all in-process TSO payloads through the scheduled
          // save time. It is still Time Advancing because grant() has not yet
          // changed its private state.
          if (timedSaveLabel.has_value()) {
            if (!timedSaveTimestamp) {
              throw RTIinternalError(
                  L"The embedded federation admitted a timestamped save without its requested time.");
            }
            recipient.initiateFederateSave(
                *timedSaveLabel,
                *timedSaveTimestamp);
            queueJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
                federationName,
                federateId,
                {"HLAfederateState"},
                federateId);
          }

          {
            std::scoped_lock lock(federationManagementMutex());
            auto& registry = embeddedFederationManagement().registry();
            grantedTime = timeState->grant(generation);
            if (!grantedTime) {
              return;
            }
            grantCompleted = true;
            auto scheduled = registry.reevaluateTimeAdvanceGrants(federationName);
            if (scheduled.status == umbra::detail::FederationTimeGrantStatus::applied) {
              newlyEligible = std::move(scheduled.dispatches);
            }
          }
        } else {
          if (!flushQueueGrantedTime || !flushQueueOptimisticTime) {
            throw RTIinternalError(
                L"The embedded Flush Queue Grant has no calculated logical times.");
          }

          std::optional<std::wstring> timedSaveLabel;
          std::shared_ptr<LogicalTime const> timedSaveTimestamp;
          std::shared_ptr<LogicalTime const> const flushQueueGrantForAdmission =
              flushQueueGrantedTime;
          {
            std::scoped_lock lock(federationManagementMutex());
            auto admission = embeddedFederationManagement().registry()
                .admitTimedFederationSaveAtFlushQueueGrantBoundary(
                    federationName,
                    federateId,
                    flushQueueGrantForAdmission);
            if (admission.status != umbra::detail::FederationSaveControlStatus::applied) {
              throw RTIinternalError(
                  L"The embedded federation could not admit a timestamped save at the Flush Queue boundary.");
            }
            timedSaveLabel = std::move(admission.currentFederateLabel);
            timedSaveTimestamp = std::move(admission.currentFederateTimestamp);
            timedSaveNotifications = std::move(admission.notifications);
          }

          // A qualifying FQR direct admission follows its complete queued TSO
          // delivery set but precedes the private grant transition, preserving
          // the recipient's required Time Advancing state.
          if (timedSaveLabel.has_value()) {
            if (!timedSaveTimestamp) {
              throw RTIinternalError(
                  L"The embedded federation admitted a timestamped save without its requested time.");
            }
            recipient.initiateFederateSave(
                *timedSaveLabel,
                *timedSaveTimestamp);
            queueJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
                federationName,
                federateId,
                {"HLAfederateState"},
                federateId);
          }

          {
            std::scoped_lock lock(federationManagementMutex());
            auto& registry = embeddedFederationManagement().registry();
            auto stateOptimistic = cloneReferenceLogicalTime(
                pendingSnapshot.implementationName,
                *flushQueueOptimisticTime);
            if (!stateOptimistic) {
              throw RTIinternalError(
                  L"The embedded Flush Queue Grant could not clone its optimistic logical time.");
            }
            auto const flushResult = timeState->grantFlushQueue(
                generation,
                std::move(flushQueueGrantedTime),
                std::move(stateOptimistic));
            if (flushResult.status != umbra::detail::FederateTimeAdvanceStatus::applied) {
              throw RTIinternalError(
                  L"The embedded federate time state rejected the Flush Queue Grant.");
            }
            grantedTime = timeState->currentTime();
            optimisticTime = std::move(flushQueueOptimisticTime);
            grantCompleted = true;
            auto scheduled = registry.reevaluateTimeAdvanceGrants(federationName);
            if (scheduled.status == umbra::detail::FederationTimeGrantStatus::applied) {
              newlyEligible = std::move(scheduled.dispatches);
            }
          }
        }

        // The state mutation happens immediately before its matching standard
        // callback, after the registry has rechecked the current shared bound.
        if (flushQueue) {
          if (!optimisticTime) {
            throw RTIinternalError(
                L"The embedded Flush Queue Grant has no optimistic logical time.");
          }
          recipient.flushQueueGrant(*grantedTime, *optimisticTime);
        } else {
          recipient.timeAdvanceGrant(*grantedTime);
        }

      });

      if (grantCompleted) {
        // The grant has completed the private transition back to Time Granted
        // (or the Flush Queue equivalent).  Publish the conditional MOM value
        // only after the matching public callback has been invoked.
        queueJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
            federationName,
            federateId,
            {"HLAtimeManagerState"});
      }

      // A receive-order automatic-resign removal that encountered a protected
      // connection-loss TSO boundary is resubmitted only after the matching
      // grant callback. That preserves the reflection/directed callback first
      // and lets the normal asynchronous-delivery gate decide its next
      // receive-order window.
      queueObjectInstanceRemovals(
          std::move(postGrantObjectRemovals),
          federationName,
          VariableLengthData());
      submitFederationSaveNotifications(
          federationName,
          std::move(immediateSaveNotifications));
      submitFederationSaveNotifications(
          federationName,
          std::move(timedSaveNotifications));
      submitTimeAdvanceGrantDispatches(std::move(newlyEligible));
    });
  };
}

umbra::detail::FederationTimeGrantDispatchFactory makeTimeAdvanceGrantDispatchFactory(
    std::shared_ptr<umbra::detail::CallbackDispatcher> const& callbackDispatcher,
    std::shared_ptr<CallbackSession> const& callbackSession,
    std::shared_ptr<umbra::detail::FederateTimeState> const& timeState,
    std::wstring federationName) {
  std::weak_ptr<umbra::detail::CallbackDispatcher> const dispatcher = callbackDispatcher;
  std::weak_ptr<CallbackSession> const session = callbackSession;
  std::weak_ptr<umbra::detail::FederateTimeState> const federateTimeState = timeState;

  return [
      dispatcher,
      session,
      federateTimeState,
      federationName = std::move(federationName)](
      std::uint64_t federateId,
      std::uint64_t generation,
      std::uint64_t dispatchIdentity) {
    auto callbackDispatcher = dispatcher.lock();
    auto callbackSession = session.lock();
    auto timeState = federateTimeState.lock();
    if (!callbackDispatcher || !callbackSession || !timeState) {
      return umbra::detail::FederationTimeGrantDispatch{};
    }
    return makeTimeAdvanceGrantDispatch(
        callbackDispatcher,
        callbackSession,
        timeState,
        federationName,
        federateId,
        generation,
        dispatchIdentity);
  };
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

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
void UmbraRtiAmbassador::startPeriodicMomScheduler() {
  if (callbackModel_ != HLA_IMMEDIATE || periodicMomScheduler_.joinable()) {
    return;
  }
  periodicMomScheduler_ = std::jthread(
      [this](std::stop_token stopToken) {
        periodicMomSchedulerLoop(stopToken);
      });
}

void UmbraRtiAmbassador::stopPeriodicMomScheduler() noexcept {
  if (!periodicMomScheduler_.joinable()) {
    return;
  }
  periodicMomScheduler_.request_stop();
  periodicMomScheduler_.join();
}

void UmbraRtiAmbassador::periodicMomSchedulerLoop(std::stop_token stopToken) {
  using namespace std::chrono_literals;
  while (!stopToken.stop_requested()) {
    std::optional<std::wstring> federationName;
    {
      std::scoped_lock lock(mutex_, federationManagementMutex());
      if (lifecycle_.state() == umbra::detail::FederateLifecycleState::joined &&
          callbackModel_ == HLA_IMMEDIATE && joinedFederationName_ &&
          joinedFederateId_) {
        federationName = *joinedFederationName_;
      }
    }
    if (federationName) {
      // The registry owns deadline claiming. This thread only supplies the
      // immediate callback model's missing callback-boundary pump; routing
      // and callback-time revalidation remain in the ordinary planner.
      pumpDueJoinedFederateMomPeriodicUpdates(*federationName);
    }
    std::this_thread::sleep_for(25ms);
  }
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

ConfigurationResult UmbraRtiAmbassador::connect(
    FederateAmbassador& federateAmbassador,
    CallbackModel callbackModel) {
  auto instrumentationScope = beginRtiCall("connect");
  return connectImpl(federateAmbassador, callbackModel, nullptr);
}

ConfigurationResult UmbraRtiAmbassador::connect(
    FederateAmbassador& federateAmbassador,
    CallbackModel callbackModel,
    RtiConfiguration const& configuration) {
  auto instrumentationScope = beginRtiCall("connect");
  return connectImpl(federateAmbassador, callbackModel, &configuration);
}

ConfigurationResult UmbraRtiAmbassador::connect(
    FederateAmbassador& federateAmbassador,
    CallbackModel callbackModel,
    Credentials const& credentials) {
  auto instrumentationScope = beginRtiCall("connect");
  if (callbacks_->isExecutingCallback()) {
    throw CallNotAllowedFromWithinCallback(
        L"Connect cannot be called from within a federate callback.");
  }
  requireNoCredentialsWhenAuthorizationIsDisabled(credentials);
  return connectImpl(federateAmbassador, callbackModel, nullptr);
}

ConfigurationResult UmbraRtiAmbassador::connect(
    FederateAmbassador& federateAmbassador,
    CallbackModel callbackModel,
    RtiConfiguration const& configuration,
    Credentials const& credentials) {
  auto instrumentationScope = beginRtiCall("connect");
  if (callbacks_->isExecutingCallback()) {
    throw CallNotAllowedFromWithinCallback(
        L"Connect cannot be called from within a federate callback.");
  }
  requireNoCredentialsWhenAuthorizationIsDisabled(credentials);
  return connectImpl(federateAmbassador, callbackModel, &configuration);
}

ConfigurationResult UmbraRtiAmbassador::connectImpl(
    FederateAmbassador& federateAmbassador,
    CallbackModel callbackModel,
    RtiConfiguration const* configuration) {
  auto instrumentationScope = beginRtiCall("connectImpl");
  if (callbacks_->isExecutingCallback()) {
    throw CallNotAllowedFromWithinCallback(
        L"Connect cannot be called from within a federate callback.");
  }
  validateCallbackModel(callbackModel);

  bool settingsApplied = false;
  std::unique_ptr<umbra::detail::ServiceReportStore> serviceReportStore;
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  // The factory-created runtime always takes the filesystem path. An
  // explicitly constructed internal federation-management test ambassador
  // may instead supply its deterministic store; that construction path is not
  // reachable through the official RTI configuration surface.
  bool const useInjectedServiceReportStore =
      static_cast<bool>(injectedServiceReportStoreForTesting_);
  if (!useInjectedServiceReportStore) {
#endif
    auto const serviceReportDirectory = validateServiceReportDirectory(
        configuredServiceReportDirectory(configuration, settingsApplied));
    serviceReportStore = std::make_unique<umbra::detail::FilesystemServiceReportStore>(
        serviceReportDirectory,
        instrumentation_);
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  }
#endif
  ServiceReportConnectionSnapshot serviceReportConnection;
  serviceReportConnection.callbackModel = callbackModel;
  if (configuration != nullptr) {
    serviceReportConnection.configurationName = configuration->configurationName();
    serviceReportConnection.rtiAddress = configuration->rtiAddress();
    serviceReportConnection.additionalSettings = configuration->additionalSettings();
  }

  // Construct the truthful result before mutating lifecycle state so a failed
  // allocation cannot leave a partially established connection behind.
  ConfigurationResult result = ignoredConfigurationResult();
  if (settingsApplied) {
    result = ConfigurationResult(
        true, false, SETTINGS_APPLIED,
        L"Umbra applied serviceReportDirectory for embedded service-report files.");
  }
  auto callbackSession =
      std::make_shared<CallbackSession>(federateAmbassador, instrumentation_);

  std::scoped_lock lock(mutex_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::not_connected) {
    throw AlreadyConnected(L"The RTI ambassador already has an active connection.");
  }
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  auto transportConnection = umbra::detail::embeddedTransportHub().connect(
      static_cast<RTIambassador*>(this),
      [this](std::wstring faultDescription) {
        handleEmbeddedTransportFailure(std::move(faultDescription));
      },
      [this](std::wstring reasonForResign) {
        return handleEmbeddedFederateResignation(std::move(reasonForResign));
      },
      instrumentation_);
  if (useInjectedServiceReportStore) {
    serviceReportStore = std::move(injectedServiceReportStoreForTesting_);
    if (!serviceReportStore) {
      throw RTIinternalError(
          L"Umbra's internal service-report test store was already consumed.");
    }
  }
#endif
  if (lifecycle_.apply(umbra::detail::FederateLifecycleEvent::connect) !=
      umbra::detail::FederateLifecycleResult::applied) {
    throw AlreadyConnected(L"The RTI ambassador already has an active connection.");
  }

  callbackSession_ = std::move(callbackSession);
  callbackModel_ = callbackModel;
  callbacks_->configure(toDispatchModel(callbackModel));
  serviceReportStore_ = std::move(serviceReportStore);
  serviceReportConnection_ = std::move(serviceReportConnection);
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  transportConnection_ = std::move(transportConnection);
  activeServiceReportStoreIsTestOnly_ = useInjectedServiceReportStore;
  startPeriodicMomScheduler();
#endif
  return result;
}

UmbraRtiAmbassador::~UmbraRtiAmbassador() {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  stopPeriodicMomScheduler();
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
  std::shared_ptr<umbra::detail::EmbeddedTransportConnection> transportConnection;
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
    // A disconnected ambassador cannot retain the filesystem directory/store
    // selected for its former connection. Completed joined-federate report
    // files remain on disk; only connection-owned state is released here.
    serviceReportStore_.reset();
    serviceReportConnection_ = {};
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
    transportConnection = std::move(transportConnection_);
    joinedServiceReport_.reset();
    activeServiceReportStoreIsTestOnly_ = false;
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
#endif
}

bool UmbraRtiAmbassador::evokeCallback(double approximateMinimumTimeInSeconds) {
  auto instrumentationScope = beginRtiCall("evokeCallback");
  if (callbacks_->isExecutingCallback()) {
    throw CallNotAllowedFromWithinCallback(
        L"Evoke Callback cannot be called from within a federate callback.");
  }
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  auto pumpPeriodicMomUpdates = [this] {
    std::optional<std::wstring> federationName;
    {
      std::scoped_lock lock(mutex_, federationManagementMutex());
      if (lifecycle_.state() == umbra::detail::FederateLifecycleState::joined &&
          joinedFederationName_ && joinedFederateId_) {
        federationName = *joinedFederationName_;
      }
    }
    if (federationName) {
      pumpDueJoinedFederateMomPeriodicUpdates(*federationName);
    }
  };
  pumpPeriodicMomUpdates();
#endif
  auto result = callbacks_->evokeOne(callbackWaitDuration(approximateMinimumTimeInSeconds));
  if (!result) {
    // A deadline may have elapsed while the dispatcher was waiting.  Claim
    // and submit it now, then give this Evoke call one normal callback slot.
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
    pumpPeriodicMomUpdates();
#endif
    if (callbacks_->pendingCount() != 0U) {
      result = callbacks_->evokeOne(std::chrono::milliseconds::zero());
    }
  }
  return result;
}

bool UmbraRtiAmbassador::evokeMultipleCallbacks(
    double approximateMinimumTimeInSeconds,
    double approximateMaximumTimeInSeconds) {
  auto instrumentationScope = beginRtiCall("evokeMultipleCallbacks");
  if (callbacks_->isExecutingCallback()) {
    throw CallNotAllowedFromWithinCallback(
        L"Evoke Multiple Callbacks cannot be called from within a federate callback.");
  }
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  auto pumpPeriodicMomUpdates = [this] {
    std::optional<std::wstring> federationName;
    {
      std::scoped_lock lock(mutex_, federationManagementMutex());
      if (lifecycle_.state() == umbra::detail::FederateLifecycleState::joined &&
          joinedFederationName_ && joinedFederateId_) {
        federationName = *joinedFederationName_;
      }
    }
    if (federationName) {
      pumpDueJoinedFederateMomPeriodicUpdates(*federationName);
    }
  };
  pumpPeriodicMomUpdates();
#endif
  auto const result = callbacks_->evokeMultiple(
      callbackWaitDuration(approximateMinimumTimeInSeconds),
      callbackWaitDuration(approximateMaximumTimeInSeconds));
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  pumpPeriodicMomUpdates();
#endif
  return result || callbacks_->pendingCount() != 0U;
}

void UmbraRtiAmbassador::enableCallbacks() {
  auto instrumentationScope = beginRtiCall("enableCallbacks");
  callbacks_->setEnabled(true);
}

void UmbraRtiAmbassador::disableCallbacks() {
  auto instrumentationScope = beginRtiCall("disableCallbacks");
  callbacks_->setEnabled(false);
}

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)

void UmbraRtiAmbassador::requireFederationServiceOperationAvailable(
    std::wstring const& operation) const {
  auto instrumentationScope = beginRtiCall("requireFederationServiceOperationAvailable");
  if (!joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        operation + L" requires membership in a federation execution.");
  }

  auto const status = embeddedFederationManagement().registry().serviceOperationStatus(
      *joinedFederationName_,
      *joinedFederateId_);
  switch (status) {
    case umbra::detail::FederationServiceOperationStatus::available:
      return;
    case umbra::detail::FederationServiceOperationStatus::federation_does_not_exist:
    case umbra::detail::FederationServiceOperationStatus::federate_not_member:
      throw FederateNotExecutionMember(
          operation + L" cannot proceed because this federate is not an execution member.");
    case umbra::detail::FederationServiceOperationStatus::save_in_progress:
      throw SaveInProgress(operation + L" is unavailable during federation save.");
    case umbra::detail::FederationServiceOperationStatus::restore_in_progress:
      throw RestoreInProgress(operation + L" is unavailable during federation restore.");
  }
  throw RTIinternalError(operation + L" encountered an unknown federation operation state.");
}

bool UmbraRtiAmbassador::emitSelectedMomServiceReportInteraction(
    std::wstring const& service,
    umbra::detail::MomServiceType serviceType,
    std::vector<umbra::detail::MomServiceArgument> const& suppliedArguments,
    umbra::detail::MomServiceArgument const& returnedArgument,
    bool success,
    std::wstring const& exception) const {
  if (!joinedFederationName_ || !joinedFederateId_ || !joinedServiceReport_ ||
      !joinedServiceReport_->endpoint || !joinedServiceReport_->endpoint->writer) {
    throw RTIinternalError(
        L"Umbra is missing the joined-federate service-report file state.");
  }

  auto& registry = embeddedFederationManagement().registry();
  auto const plan = registry.planMomServiceReport(
      *joinedFederationName_,
      *joinedFederateId_,
      static_cast<std::uint16_t>(serviceType));
  if (plan.disposition != umbra::detail::MomServiceReportDisposition::interaction) {
    return false;
  }

  auto reservation = registry.reserveMomServiceReport(
      *joinedFederationName_,
      *joinedFederateId_,
      static_cast<std::uint16_t>(serviceType));
  if (!reservation.acceptedForEmission ||
      reservation.routing.disposition !=
          umbra::detail::MomServiceReportDisposition::interaction ||
      reservation.routing.reportParameterHandles.size() != 7U) {
    throw RTIinternalError(
        L"Umbra could not reserve the selected service-report interaction.");
  }
  if (reservation.serialNumber >
      static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max())) {
    throw RTIinternalError(
        L"Umbra exhausted the signed HLAreportServiceInvocation serial range.");
  }

  auto const encoded = umbra::detail::encodeMomServiceInvocation(
      service,
      serviceType,
      success,
      suppliedArguments,
      returnedArgument,
      exception,
      static_cast<std::int32_t>(reservation.serialNumber));
  std::vector<InteractionParameterValue> reportParameters;
  reportParameters.reserve(reservation.routing.reportParameterHandles.size());
  reportParameters.emplace_back(
      reservation.routing.reportParameterHandles[0], encoded.service);
  reportParameters.emplace_back(
      reservation.routing.reportParameterHandles[1], encoded.serviceType);
  reportParameters.emplace_back(
      reservation.routing.reportParameterHandles[2], encoded.successIndicator);
  reportParameters.emplace_back(
      reservation.routing.reportParameterHandles[3], encoded.suppliedArguments);
  reportParameters.emplace_back(
      reservation.routing.reportParameterHandles[4], encoded.returnedArgument);
  reportParameters.emplace_back(
      reservation.routing.reportParameterHandles[5], encoded.exception);
  reportParameters.emplace_back(
      reservation.routing.reportParameterHandles[6], encoded.serialNumber);
  auto const reliableTransportation = transportationHandleFromEmbeddedName(
      "HLAreliable",
      L"The embedded federation could not reconstruct HLAreportServiceInvocation transportation.");
  // HLA_IMMEDIATE may enter the observer's Java/Python callback before this
  // helper returns. The caller deliberately invokes this helper without its
  // report-file mutex held.
  queueMomServiceReportInteraction(
      *joinedFederationName_,
      *joinedFederateId_,
      static_cast<std::uint16_t>(serviceType),
      std::move(reservation),
      std::move(reportParameters),
      reliableTransportation);
  return true;
}

void UmbraRtiAmbassador::emitExceptionReport(
    std::wstring const& service,
    rti1516_2025::Exception const& exception) const noexcept {
  try {
    std::optional<std::wstring> federationName;
    std::optional<std::uint64_t> reportedFederateId;
    {
      std::scoped_lock lock(mutex_, federationManagementMutex());
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        return;
      }
      federationName = *joinedFederationName_;
      reportedFederateId = *joinedFederateId_;
    }

    auto const report = embeddedFederationManagement().registry().planExceptionReport(
        *federationName,
        *reportedFederateId);
    if (report.status != umbra::detail::ExceptionReportStatus::applied) {
      return;
    }
    auto exceptionText = exception.name();
    auto const detail = exception.what();
    if (!detail.empty()) {
      exceptionText += L": ";
      exceptionText += detail;
    }
    queueExceptionReport(
        *federationName,
        report,
        service,
        std::move(exceptionText));
  } catch (...) {
    // Exception reporting is advisory traffic. Never replace the original
    // C++ service exception with a routing/catalog/callback failure.
  }
}

void UmbraRtiAmbassador::appendSuccessfulVoidServiceReportToFileIfSelected(
    std::wstring const& service,
    umbra::detail::MomServiceType serviceType,
    std::vector<umbra::detail::MomServiceArgument> const& suppliedArguments,
    bool emitInteraction) const {
  // File-selected service wrappers call this while their service transaction
  // locks are held, so the plan/reservation pair remains stable.  The direct
  // untimed Send Interaction wrapper opts into emitInteraction only after its
  // planning locks have been released; that path may submit the ordinary
  // receive-order callback route without re-entering a held federation lock.
  if (!joinedFederationName_ || !joinedFederateId_ || !joinedServiceReport_ ||
      !joinedServiceReport_->endpoint || !joinedServiceReport_->endpoint->writer) {
    throw RTIinternalError(
        L"Umbra is missing the joined-federate service-report file state.");
  }

  if (emitInteraction && emitSelectedMomServiceReportInteraction(
      service,
      serviceType,
      suppliedArguments,
      {umbra::detail::MomArgumentType::null_value,
       L"",
       umbra::detail::formatMomNull()})) {
    return;
  }
  auto const endpoint = joinedServiceReport_->endpoint;
  std::scoped_lock reportLock(endpoint->mutex);
  appendSelectedServiceReportRecord(
      *endpoint,
      *joinedFederationName_,
      *joinedFederateId_,
      static_cast<std::uint16_t>(serviceType),
      [&service, &suppliedArguments](std::uint32_t serialNumber) {
        return umbra::detail::formatMomSuccessfulVoidServiceReportRecord(
            serialNumber,
            service,
            suppliedArguments);
      });
}

void UmbraRtiAmbassador::appendReservedSuccessfulVoidServiceReportToFile(
    std::uint32_t serialNumber,
    std::wstring const& service,
    std::vector<umbra::detail::MomServiceArgument> const& suppliedArguments) const {
  if (!joinedServiceReport_ || !joinedServiceReport_->endpoint ||
      !joinedServiceReport_->endpoint->writer) {
    throw RTIinternalError(
        L"Umbra is missing the joined-federate service-report file state.");
  }

  auto const endpoint = joinedServiceReport_->endpoint;
  std::scoped_lock reportLock(endpoint->mutex);
  try {
    endpoint->writer->append(
        umbra::detail::formatMomSuccessfulVoidServiceReportRecord(
            serialNumber, service, suppliedArguments));
  } catch (std::exception const&) {
    // Reporting to a configured file is normative embedded-profile behavior;
    // do not replace this writer or silently redirect the record to memory.
    throw RTIinternalError(
        L"Umbra could not append the selected service-report file record.");
  }
}

void UmbraRtiAmbassador::handleEmbeddedTransportFailure(
    std::wstring faultDescription) {
  auto instrumentationScope = beginRtiCall("handleEmbeddedTransportFailure");
  static_cast<void>(handleEmbeddedMembershipLoss(
      EmbeddedMembershipLossKind::connection_lost,
      std::move(faultDescription)));
}

bool UmbraRtiAmbassador::handleEmbeddedFederateResignation(
    std::wstring reasonForResign) {
  auto instrumentationScope = beginRtiCall("handleEmbeddedFederateResignation");
  return handleEmbeddedMembershipLoss(
      EmbeddedMembershipLossKind::rti_resigned,
      std::move(reasonForResign));
}

bool UmbraRtiAmbassador::handleEmbeddedMembershipLoss(
    EmbeddedMembershipLossKind kind,
    std::wstring reason) {
  auto instrumentationScope = beginRtiCall("handleEmbeddedMembershipLoss");
  bool const connectionLost = kind == EmbeddedMembershipLossKind::connection_lost;
  std::vector<umbra::detail::FederationTimeGrantDispatch> newlyEligible;
  std::vector<umbra::detail::FederationSynchronizedNotification>
      synchronizationNotifications;
  std::vector<umbra::detail::FederationSaveNotification> saveNotifications;
  std::vector<umbra::detail::FederationRestoreNotification> restoreNotifications;
  std::vector<umbra::detail::ObjectInstanceRemovalRecipient> objectRemovals;
  std::vector<umbra::detail::AttributeOwnershipAssumptionRecipient>
      ownershipAssumptions;
  std::vector<umbra::detail::AttributeOwnershipAcquisitionWorkItem>
      ownershipAcquisitionWorkItems;
  std::vector<umbra::detail::DeclarationAdvisory> declarationAdvisories;
  std::optional<umbra::detail::FederateLostReportPlan> federateLostReport;
  std::shared_ptr<CallbackSession> callbackSession;
  std::wstring federationName;
  bool finalServiceReportAppendFailed = false;

  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      // A graceful or previous forced transition already removed membership.
      // The endpoint remains usable after an RTI-initiated resignation, but
      // there is no second lifecycle event to report.
      return false;
    }

    federationName = *joinedFederationName_;
    if (connectionLost) {
      // The registry owns the last-granted time and active subscription state,
      // both of which disappear as part of the forced resignation. Capture
      // the source-mandated loss report first, then let the normal automatic
      // resign machinery mutate membership and application state.
      auto report = embeddedFederationManagement().registry().planFederateLostReport(
          federationName,
          *joinedFederateId_);
      if (report.status == umbra::detail::FederateLostReportStatus::applied) {
        federateLostReport = std::move(report);
      }
      // A corrupted private catalog/time state must not keep a known transport
      // fault joined indefinitely. Normal prevalidated executions always have
      // a plan; this defensive escape preserves authoritative loss cleanup
      // without manufacturing a malformed MOM interaction.
    }
    auto& registry = embeddedFederationManagement().registry();
    auto resigned = connectionLost
        ? registry.connectionLostWithFinalServiceReportReservation(
              federationName,
              *joinedFederateId_,
              static_cast<std::uint16_t>(
                  umbra::detail::MomServiceType::federation_management))
        : registry.resignWithFinalServiceReportReservation(
              federationName,
              *joinedFederateId_,
              rti1516_2025::NO_ACTION,
              static_cast<std::uint16_t>(
                  umbra::detail::MomServiceType::federation_management));
    if (!connectionLost &&
        resigned.status != umbra::detail::FederationRegistryStatus::applied) {
      // This deliberately bounded RTI-control seam uses the standard
      // NO_ACTION resign path. It only accepts a member whose ordinary
      // no-disposition preconditions already hold; a future administration or
      // watchdog source must supply its own fully specified disposition policy.
      return false;
    }
    if (resigned.status == umbra::detail::FederationRegistryStatus::applied) {
      synchronizationNotifications = std::move(resigned.synchronizationNotifications);
      saveNotifications = std::move(resigned.saveNotifications);
      restoreNotifications = std::move(resigned.restoreNotifications);
      objectRemovals.reserve(resigned.resignObjectRemovals.size());
      for (auto& removal : resigned.resignObjectRemovals) {
        objectRemovals.push_back({
            removal.receivingFederateId,
            removal.objectInstanceHandle,
            std::move(removal.callbackRoute),
            std::move(removal.serviceReportRoute),
            removal.rtiOwnedMomObject,
        });
      }
      ownershipAssumptions.reserve(resigned.resignOwnershipAssumptions.size());
      for (auto& assumption : resigned.resignOwnershipAssumptions) {
        ownershipAssumptions.push_back({
            assumption.receivingFederateId,
            assumption.objectInstanceHandle,
            std::move(assumption.attributeHandles),
            std::move(assumption.callbackRoute),
        });
      }
      ownershipAcquisitionWorkItems =
          std::move(resigned.resignOwnershipAcquisitionWorkItems);

      // Membership removal can change the relevance of declarations owned by
      // surviving federates. Compute those transitions from the post-removal
      // state before the departing member's route disappears from the adapter.
      declarationAdvisories = embeddedFederationManagement().registry()
          .planDeclarationAdvisories(federationName);

      auto scheduled = embeddedFederationManagement().registry()
          .reevaluateTimeAdvanceGrants(federationName);
      if (scheduled.status == umbra::detail::FederationTimeGrantStatus::applied) {
        newlyEligible = std::move(scheduled.dispatches);
      }
      auto timedSave = embeddedFederationManagement().registry()
          .reevaluateTimedFederationSave(federationName);
      if (timedSave.status == umbra::detail::FederationSaveControlStatus::applied) {
        for (auto& notification : timedSave.notifications) {
          saveNotifications.push_back(std::move(notification));
        }
      }
    }

    // A Connection Lost event is authoritative even if a best-effort cleanup
    // callback could not be prepared: the ambassador becomes Not Connected.
    // The distinct Federate Resigned event stays connected but leaves the
    // Joined state, after the registry has accepted its bounded NO_ACTION
    // disposition above.
    auto const lifecycleEvent = connectionLost
        ? umbra::detail::FederateLifecycleEvent::connection_lost
        : umbra::detail::FederateLifecycleEvent::rti_resign;
    if (lifecycle_.apply(lifecycleEvent) !=
        umbra::detail::FederateLifecycleResult::applied) {
      return false;
    }
    if (resigned.finalServiceReportFileSerialNumber.has_value()) {
      // Both §4.4 Connection Lost and §4.13 Federate Resigned are RTI-invoked
      // HLA services. Each has one source-defined text argument. Section
      // 11.5.1 leaves the descriptive argument name implementation-dependent;
      // keep its name anchored to the corresponding service narrative. As
      // with federate-initiated resignation, reserve and append before the
      // joined member and its writer disappear.
      try {
        appendReservedSuccessfulVoidServiceReportToFile(
            *resigned.finalServiceReportFileSerialNumber,
            connectionLost ? L"ConnectionLost" : L"FederateResigned",
            {{umbra::detail::MomArgumentType::string,
              connectionLost ? L"Fault description" : L"Reason for resigning",
              umbra::detail::formatMomString(reason)}});
      } catch (RTIinternalError const&) {
        // The RTI-side service action has already completed its authoritative
        // membership transition. Finish the lifetime cleanup and callback
        // routing before returning the deterministic storage error; do not
        // leave either lifecycle carrying stale joined state.
        finalServiceReportAppendFailed = true;
      }
    }
    if (federateTimeState_) {
      federateTimeState_->deactivate();
    }
    federateTimeState_.reset();
    // The file itself is retained for its completed joined-federate lifetime;
    // releasing the writer here ensures a later Join receives a new identity.
    joinedServiceReport_.reset();
    joinedFederationName_.reset();
    joinedFederateId_.reset();
    if (connectionLost) {
      transportConnection_.reset();
      callbackSession = std::move(callbackSession_);
      // Faulted transport input invalidates queued work from the old endpoint;
      // the Connection Lost callback itself is submitted below on that session.
    } else {
      callbackSession = callbackSession_;
      // Previous joined-state callbacks must not leak across the RTI-initiated
      // resignation. The connected session survives to deliver the official
      // Federate Resigned callback and later work after a fresh Join.
    }
    callbacks_->reset();
  }

  // Queue the RTI-originated loss interaction before every automatic-resign
  // consequence. This preserves report-before-cleanup callback ordering for
  // evoked recipients without invoking user code while federation locks are
  // held. Callback-time subscription/lifecycle rechecks remain in the report
  // route because surviving federates may change state before they evoke it.
  if (federateLostReport) {
    queueFederateLostReport(
        federationName,
        std::move(*federateLostReport),
        reason);
  }
  submitTimeAdvanceGrantDispatches(std::move(newlyEligible));
  submitFederationSaveNotifications(
      federationName,
      std::move(saveNotifications));
  submitFederationRestoreNotifications(
      federationName,
      std::move(restoreNotifications));
  submitFederationSynchronizedNotifications(std::move(synchronizationNotifications));
  queueDeclarationAdvisories(std::move(declarationAdvisories));
  VariableLengthData emptyMembershipLossTag;
  queueAttributeOwnershipAcquisitionWorkItems(
      std::move(ownershipAcquisitionWorkItems),
      federationName);
  queueAttributeOwnershipAssumptionRecipients(
      std::move(ownershipAssumptions),
      federationName,
      emptyMembershipLossTag);
  queueObjectInstanceRemovals(
      std::move(objectRemovals),
      federationName,
      emptyMembershipLossTag);
  queueFederationMomConditionalAttributeUpdate(
      federationName,
      {"HLAfederatesInFederation"});

  if (callbackSession) {
    callbacks_->submit([
        callbackSession = std::move(callbackSession),
        connectionLost,
        reason = std::move(reason)]() mutable {
      callbackSession->invoke([
          connectionLost,
          reason = std::move(reason)](
          FederateAmbassador& recipient) mutable {
        // Both callback paths occur only after the irreversible membership
        // transition. Do not allow a faulty recipient to revive or escape the
        // private control/transport path.
        try {
          if (connectionLost) {
            recipient.connectionLost(reason);
          } else {
            recipient.federateResigned(reason);
          }
        } catch (...) {
        }
      });
      });
  }
  if (finalServiceReportAppendFailed) {
    throw RTIinternalError(
        L"Umbra could not append the selected service-report file record.");
  }
  return true;
}

void UmbraRtiAmbassador::createFederationExecution(
    std::wstring const& federationName,
    std::wstring const& fomModule,
    std::wstring const& logicalTimeImplementationName) {
  auto instrumentationScope = beginRtiCall("createFederationExecution");
  createFederationExecution(
      federationName,
      std::vector<std::wstring>{fomModule},
      logicalTimeImplementationName);
}

void UmbraRtiAmbassador::createFederationExecution(
    std::wstring const& federationName,
    std::vector<std::wstring> const& fomModules,
    std::wstring const& logicalTimeImplementationName) {
  auto instrumentationScope = beginRtiCall("createFederationExecution");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);

  auto& management = embeddedFederationManagement();
  auto preparation = management.coordinator().prepareCreate(
      fomModules,
      std::nullopt,
      logicalTimeImplementationName);
  if (!preparation.accepted()) {
    throwPreparationFailure(preparation);
  }

  auto created = management.registry().create(federationName, std::move(*preparation.definition));
  switch (created.status) {
    case umbra::detail::FederationRegistryStatus::applied:
      return;
    case umbra::detail::FederationRegistryStatus::federation_already_exists:
      throw FederationExecutionAlreadyExists(
          L"A federation execution with the supplied name already exists.");
    case umbra::detail::FederationRegistryStatus::invalid_request:
    case umbra::detail::FederationRegistryStatus::federation_does_not_exist:
    case umbra::detail::FederationRegistryStatus::federates_currently_joined:
    case umbra::detail::FederationRegistryStatus::federate_name_already_in_use:
    case umbra::detail::FederationRegistryStatus::federate_not_member:
      throw RTIinternalError(L"Umbra could not commit the prepared federation definition.");
  }
  throw RTIinternalError(L"Umbra encountered an unknown federation-registry creation outcome.");
  } catch (Exception const& exception) {
    emitExceptionReport(L"Create Federation Execution", exception);
    throw;
  }
}

void UmbraRtiAmbassador::createFederationExecutionWithMIM(
    std::wstring const& federationName,
    std::vector<std::wstring> const& fomModules,
    std::wstring const& mimModule,
    std::wstring const& logicalTimeImplementationName) {
  auto instrumentationScope = beginRtiCall("createFederationExecutionWithMIM");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);

  auto& management = embeddedFederationManagement();
  auto preparation = management.coordinator().prepareCreate(
      fomModules,
      std::optional<std::wstring>{mimModule},
      logicalTimeImplementationName);
  if (!preparation.accepted()) {
    throwPreparationFailure(preparation);
  }

  auto created = management.registry().create(federationName, std::move(*preparation.definition));
  switch (created.status) {
    case umbra::detail::FederationRegistryStatus::applied:
      return;
    case umbra::detail::FederationRegistryStatus::federation_already_exists:
      throw FederationExecutionAlreadyExists(
          L"A federation execution with the supplied name already exists.");
    case umbra::detail::FederationRegistryStatus::invalid_request:
    case umbra::detail::FederationRegistryStatus::federation_does_not_exist:
    case umbra::detail::FederationRegistryStatus::federates_currently_joined:
    case umbra::detail::FederationRegistryStatus::federate_name_already_in_use:
    case umbra::detail::FederationRegistryStatus::federate_not_member:
      throw RTIinternalError(L"Umbra could not commit the prepared federation definition.");
  }
  throw RTIinternalError(L"Umbra encountered an unknown federation-registry creation outcome.");
  } catch (Exception const& exception) {
    emitExceptionReport(L"Create Federation Execution With MIM", exception);
    throw;
  }
}

void UmbraRtiAmbassador::destroyFederationExecution(std::wstring const& federationName) {
  auto instrumentationScope = beginRtiCall("destroyFederationExecution");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);

  auto destroyed = embeddedFederationManagement().registry().destroy(federationName);
  switch (destroyed.status) {
    case umbra::detail::FederationRegistryStatus::applied:
      // A later execution may reuse federate/object handle values; do not
      // carry wall-clock admission history across federation lifetimes.
      updateRateGate().clear();
      return;
    case umbra::detail::FederationRegistryStatus::federation_does_not_exist:
      throw FederationExecutionDoesNotExist(L"The supplied federation execution does not exist.");
    case umbra::detail::FederationRegistryStatus::federates_currently_joined:
      throw FederatesCurrentlyJoined(
          L"A federation execution cannot be destroyed while federates remain joined.");
    case umbra::detail::FederationRegistryStatus::invalid_request:
    case umbra::detail::FederationRegistryStatus::federation_already_exists:
    case umbra::detail::FederationRegistryStatus::federate_name_already_in_use:
    case umbra::detail::FederationRegistryStatus::federate_not_member:
      throw RTIinternalError(L"Umbra could not destroy the federation execution.");
  }
  throw RTIinternalError(L"Umbra encountered an unknown federation-registry destruction outcome.");
  } catch (Exception const& exception) {
    emitExceptionReport(L"Destroy Federation Execution", exception);
    throw;
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

FederateHandle UmbraRtiAmbassador::joinFederationExecutionImpl(
    std::optional<std::wstring> requestedFederateName,
    std::wstring const& federateType,
    std::wstring const& federationName,
    std::vector<std::wstring> const& additionalFomModules) {
  auto instrumentationScope = beginRtiCall("joinFederationExecutionImpl");
  try {
  if (callbacks_->isExecutingCallback()) {
    throw CallNotAllowedFromWithinCallback(
        L"Join Federation Execution cannot be called from within a federate callback.");
  }

  FederateHandle result;
  std::vector<umbra::detail::FederationTimeGrantDispatch> newlyEligible;
  std::vector<umbra::detail::SynchronizationPointAnnouncement>
      newlyJoinedSynchronizationAnnouncements;
  std::vector<umbra::detail::AttributeOwnershipAssumptionRecipient>
      newlyEligibleOwnershipAssumptions;
  std::vector<umbra::detail::ObjectInstanceDiscoveryRecipient>
      newlyJoinedMomDiscoveries;
  std::uint64_t joinedFederateId = 0;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    if (lifecycle_.state() == umbra::detail::FederateLifecycleState::joined) {
      throw FederateAlreadyExecutionMember(
          L"This RTI ambassador is already joined to a federation execution.");
    }

    auto& management = embeddedFederationManagement();
    auto currentDefinition = management.registry().definitionFor(federationName);
    if (!currentDefinition) {
      throw FederationExecutionDoesNotExist(L"The supplied federation execution does not exist.");
    }

    auto callbackSession = callbackSession_;
    auto callbackDispatcher = callbacks_;
    if (!callbackSession || !callbackDispatcher) {
      throw RTIinternalError(
          L"The embedded connection has no federate ambassador callback recipient.");
    }
    // Register this endpoint as part of the registry's membership transaction.
    // In particular, an additional-FOM join must not commit its definition and
    // membership before an allocation failure can prevent federate callback delivery.
    auto callbackRoute = makeFederateCallbackRoute(
        callbackDispatcher,
        callbackSession);

    umbra::detail::FederationJoinResult joined;
    std::shared_ptr<umbra::detail::FederateTimeState> timeState;
    std::vector<umbra::detail::PrevalidatedFomModule> fomModulesSpecifiedAtJoin;
    if (additionalFomModules.empty()) {
      // Allocate and validate the initial logical-time state before the registry
      // commits membership, so a factory/allocation failure cannot leave a
      // joined federate without the required initial time position.
      timeState = makeFederateTimeState(management, *currentDefinition);
      auto timeAdvanceGrantDispatchFactory = makeTimeAdvanceGrantDispatchFactory(
          callbackDispatcher,
          callbackSession,
          timeState,
          federationName);
      joined = management.registry().joinWithTimeState(
          federationName,
          timeState,
          federateType,
          std::move(requestedFederateName),
          std::move(callbackRoute),
          std::move(timeAdvanceGrantDispatchFactory));
    } else {
      auto preparation = management.coordinator().prepareAdditionalModules(
          *currentDefinition,
          additionalFomModules);
      if (!preparation.accepted()) {
        throwPreparationFailure(preparation);
      }
      if (!preparation.definition ||
          preparation.definition->fomModules.size() < additionalFomModules.size()) {
        throw RTIinternalError(
            L"Umbra could not retain the validated FOM modules supplied at Join.");
      }
      auto const firstJoinModule = preparation.definition->fomModules.end() -
          static_cast<std::ptrdiff_t>(additionalFomModules.size());
      fomModulesSpecifiedAtJoin.assign(
          firstJoinModule,
          preparation.definition->fomModules.end());
      for (auto const& module : fomModulesSpecifiedAtJoin) {
        if (module.kind != umbra::detail::FomModuleKind::fom) {
          throw RTIinternalError(
              L"Umbra could not retain a valid FOM-module designator for Join.");
        }
      }
      timeState = makeFederateTimeState(management, *preparation.definition);
      auto timeAdvanceGrantDispatchFactory = makeTimeAdvanceGrantDispatchFactory(
          callbackDispatcher,
          callbackSession,
          timeState,
          federationName);
      joined = management.registry().joinWithDefinitionAndTimeState(
          federationName,
          std::move(*preparation.definition),
          timeState,
          federateType,
          std::move(requestedFederateName),
          std::move(callbackRoute),
          std::move(timeAdvanceGrantDispatchFactory));
    }

    switch (joined.status) {
      case umbra::detail::FederationRegistryStatus::applied:
        break;
      case umbra::detail::FederationRegistryStatus::federation_does_not_exist:
        throw FederationExecutionDoesNotExist(L"The supplied federation execution does not exist.");
      case umbra::detail::FederationRegistryStatus::federate_name_already_in_use:
        throw FederateNameAlreadyInUse(
            L"The supplied federate name is already in use in this federation execution.");
      case umbra::detail::FederationRegistryStatus::invalid_request:
      case umbra::detail::FederationRegistryStatus::federation_already_exists:
      case umbra::detail::FederationRegistryStatus::federates_currently_joined:
      case umbra::detail::FederationRegistryStatus::federate_not_member:
        throw RTIinternalError(L"Umbra could not commit the federate membership.");
    }
    if (!joined.membership) {
      throw RTIinternalError(L"Umbra could not complete the federate membership transaction.");
    }

    // Report-file allocation is part of the joined-federate lifetime, not a
    // lazy side effect of a later switch update.  The registry has supplied
    // the final membership identity at this point; if filesystem creation
    // fails, remove that new membership before exposing a successful Join.
    auto rollbackJoinedMembership = [&]() {
      auto const rolledBack = management.registry().resign(
          federationName,
          joined.membership->id,
          NO_ACTION);
      if (timeState) {
        timeState->deactivate();
      }
      if (rolledBack.status != umbra::detail::FederationRegistryStatus::applied) {
        throw RTIinternalError(
            L"Umbra could not roll back a joined federate after service-report setup failed.");
      }
    };

    auto joinedDefinition = management.registry().definitionFor(federationName);
    auto const autoProvide = management.registry().autoProvideSwitchFor(
        federationName,
        joined.membership->id);
    if (!joinedDefinition || !autoProvide || !serviceReportStore_) {
      rollbackJoinedMembership();
      throw RTIinternalError(
          L"Umbra could not initialize the joined federate's service-report state.");
    }

    // The federation-execution MOM object is independent of any one joined
    // federate's report-file lifetime.  Establish it once at the first
    // successful Join so later standard subscriptions can discover the
    // static HLAmanager.HLAfederation attributes through the same RTI-owned
    // object callback path as HLAmanager.HLAfederate.
    auto const federationMomStatus = management.registry().establishFederationMomObject(
        federationName,
        std::wstring{kUmbraRtiVersion},
        L"HLAstandardMIM");
    if (federationMomStatus != umbra::detail::JoinedFederateMomObjectStatus::applied &&
        federationMomStatus != umbra::detail::JoinedFederateMomObjectStatus::already_established) {
      rollbackJoinedMembership();
      throw RTIinternalError(
          L"Umbra could not establish the federation execution's RTI-owned MOM object state.");
    }

    std::optional<JoinedServiceReportState> pendingServiceReport;
    try {
      auto const joinIdentifier = nextServiceReportJoinIdentifier();
      auto const initialRecord = formatJoinedFederateServiceReportInitialRecord(
          serviceReportConnection_,
          federationName,
          *joinedDefinition,
          *joined.membership,
          *autoProvide,
          fomModulesSpecifiedAtJoin);
      auto writer = serviceReportStore_->createForJoinedFederate({
          federationName,
          joined.membership->name,
          joined.membership->id,
          joinIdentifier,
          initialRecord,
      });
      if (!writer ||
          (!activeServiceReportStoreIsTestOnly_ && writer->location().empty())) {
        throw RTIinternalError(
            L"Umbra's runtime service-report store did not return a filesystem location.");
      }
      auto location = writer->location();
      auto endpoint = std::make_shared<JoinedServiceReportEndpoint>();
      endpoint->writer = std::move(writer);
      if (!activeServiceReportStoreIsTestOnly_) {
        // The production writer has now allocated the one immutable location
        // required for this joined-federate lifetime. Establish the private
        // MIM object from that exact value before the Join becomes visible;
        // test-only memory stores intentionally do not invent a public-facing
        // report-file designator.
        auto const momObjectStatus = management.registry().establishJoinedFederateMomObject(
            federationName,
            joined.membership->id,
            {
                std::wstring{kUmbraEmbeddedFederateHost},
                std::wstring{kUmbraRtiVersion},
                fomModulesSpecifiedAtJoin,
                location.wstring(),
            });
        if (momObjectStatus != umbra::detail::JoinedFederateMomObjectStatus::applied) {
          throw RTIinternalError(
              L"Umbra could not establish the joined federate's RTI-owned MOM object state.");
        }
      }
      pendingServiceReport.emplace(JoinedServiceReportState{
          joined.membership->id,
          joinIdentifier,
          std::move(location),
          endpoint,
      });
      // Attach the writer only after it has selected the immutable joined
      // file. The weak route lets RTI-initiated services report at recipient
      // federates without retaining their endpoint beyond resignation.
      auto const routeStatus = management.registry().setServiceReportRoute(
          federationName,
          joined.membership->id,
          makeFederateServiceReportRoute(
              endpoint,
              federationName,
              joined.membership->id));
      if (routeStatus != umbra::detail::FederationRegistryStatus::applied) {
        throw RTIinternalError(
            L"Umbra could not attach the joined federate's service-report route.");
      }
      // Move the fully constructed lifetime state into the ambassador before
      // changing the lifecycle. If this allocation/move were ever to fail,
      // the catch below can still roll the registry membership and route back.
      joinedServiceReport_ = std::move(pendingServiceReport);
    } catch (...) {
      rollbackJoinedMembership();
      throw RTIinternalError(
          L"Umbra could not create the configured service-report file for the joined federate.");
    }

    if (lifecycle_.apply(umbra::detail::FederateLifecycleEvent::join) !=
        umbra::detail::FederateLifecycleResult::applied) {
      joinedServiceReport_.reset();
      rollbackJoinedMembership();
      throw RTIinternalError(L"Umbra could not complete the Join Federation Execution transition.");
    }

    joinedFederationName_ = federationName;
    joinedFederateId_ = joined.membership->id;
    joinedFederateId = joined.membership->id;
    federateTimeState_ = std::move(timeState);
    result = makeFederateHandle(joined.membership->id);

    // The RTI-owned joined-federate MOM object was established before the
    // lifecycle transition. Existing subscribers may therefore need a
    // discovery now that this joined-federate lifetime is visible.
    if (auto const momObject = management.registry().joinedFederateMomObjectFor(
            federationName,
            joined.membership->id)) {
      newlyJoinedMomDiscoveries = management.registry()
          .planJoinedFederateMomObjectDiscoveriesForInstance(
              federationName,
              momObject->objectInstanceHandle);
    }

    auto synchronizationAnnouncements =
        management.registry().announcePendingSynchronizationPoints(
            federationName,
            joined.membership->id);
    if (synchronizationAnnouncements.status ==
        umbra::detail::SynchronizationPointAnnouncementStatus::applied) {
      newlyJoinedSynchronizationAnnouncements =
          std::move(synchronizationAnnouncements.announcements);
    }

    // A newly joined federate is a possible future assumption recipient. It
    // normally becomes eligible only after discovery/publication, but run the
    // same registry planner at the membership boundary so a compatible
    // restored/extended definition cannot bypass the search state.
    newlyEligibleOwnershipAssumptions =
        management.registry().planAttributeOwnershipAssumptionsForFederate(
            federationName,
            joined.membership->id);

    // An additional FOM may alter federation-wide time metadata such as the
    // NRG switch. Re-evaluate existing private TARs only after the new
    // definition and membership have committed, then submit the actions after
    // releasing the runtime locks below.
    auto scheduled = management.registry().reevaluateTimeAdvanceGrants(federationName);
    if (scheduled.status == umbra::detail::FederationTimeGrantStatus::applied) {
      newlyEligible = std::move(scheduled.dispatches);
    }
  }
  submitTimeAdvanceGrantDispatches(std::move(newlyEligible));
  submitSynchronizationPointAnnouncements(
      std::move(newlyJoinedSynchronizationAnnouncements));
  queueAttributeOwnershipAssumptionRecipients(
      std::move(newlyEligibleOwnershipAssumptions),
      federationName,
      VariableLengthData());
  queueObjectInstanceDiscoveries(std::move(newlyJoinedMomDiscoveries), federationName);
  // Join Federation Execution is an explicit HLAfederateState conditional
  // update boundary.  Queue it after discovery so an immediate subscriber
  // observes the object before its state reflection, while the conditional
  // helper still snapshots ActiveFederate from the committed ledger.
  queueJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
      federationName,
      joinedFederateId,
      {"HLAfederateState"});
  queueFederationMomConditionalAttributeUpdate(
      federationName,
      {"HLAfederatesInFederation"});
  if (!additionalFomModules.empty()) {
    queueFederationMomConditionalAttributeUpdate(
        federationName,
        {"HLAFOMmoduleDesignatorList"});
  }
  return result;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Join Federation Execution", exception);
    throw;
  }
}

void UmbraRtiAmbassador::resignFederationExecution(ResignAction resignAction) {
  auto instrumentationScope = beginRtiCall("resignFederationExecution");
  try {
  if (callbacks_->isExecutingCallback()) {
    throw CallNotAllowedFromWithinCallback(
        L"Resign Federation Execution cannot be called from within a federate callback.");
  }

  std::vector<umbra::detail::FederationTimeGrantDispatch> newlyEligible;
  std::vector<umbra::detail::FederationSynchronizedNotification>
      synchronizationNotifications;
  std::vector<umbra::detail::FederationSaveNotification> saveNotifications;
  std::vector<umbra::detail::FederationRestoreNotification> restoreNotifications;
  std::vector<umbra::detail::ObjectInstanceRemovalRecipient> resignObjectRemovals;
  std::vector<umbra::detail::AttributeOwnershipAssumptionRecipient>
      resignOwnershipAssumptions;
  std::vector<umbra::detail::AttributeOwnershipAcquisitionWorkItem>
      resignOwnershipAcquisitionWorkItems;
  std::vector<umbra::detail::DeclarationAdvisory> declarationAdvisories;
  std::wstring federationName;
  bool finalServiceReportAppendFailed = false;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"This RTI ambassador is not a member of a federation execution.");
    }
    requireValidResignAction(resignAction);

    federationName = *joinedFederationName_;
    auto resigned = embeddedFederationManagement().registry()
        .resignWithFinalServiceReportReservation(
        federationName,
        *joinedFederateId_,
        resignAction,
        static_cast<std::uint16_t>(
            umbra::detail::MomServiceType::federation_management));
    if (resigned.status == umbra::detail::FederationRegistryStatus::federate_not_member ||
        resigned.status == umbra::detail::FederationRegistryStatus::federation_does_not_exist) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    if (resigned.status == umbra::detail::FederationRegistryStatus::invalid_resign_action) {
      throw InvalidResignAction(
          L"The supplied resign action is not an IEEE 1516.1 ResignAction value.");
    }
    if (resigned.status ==
        umbra::detail::FederationRegistryStatus::ownership_acquisition_pending) {
      throw OwnershipAcquisitionPending(
          L"Resign Federation Execution cannot leave an ownership acquisition pending.");
    }
    if (resigned.status == umbra::detail::FederationRegistryStatus::federate_owns_attributes) {
      throw FederateOwnsAttributes(
          L"Resign Federation Execution cannot leave instance attributes owned by the federate.");
    }
    if (resigned.status != umbra::detail::FederationRegistryStatus::applied ||
        lifecycle_.apply(umbra::detail::FederateLifecycleEvent::resign) !=
            umbra::detail::FederateLifecycleResult::applied) {
      throw RTIinternalError(L"Umbra could not complete the Resign Federation Execution transition.");
    }
    if (resigned.finalServiceReportFileSerialNumber.has_value()) {
      // Section 4.12 supplies one action argument.  Section 11.5.1 leaves
      // descriptive argument text implementation-dependent; Umbra uses the
      // corresponding Table 20 MIM parameter spelling while preserving the
      // Table 5 type-44 ResignAction encoding and official enum value.
      try {
        appendReservedSuccessfulVoidServiceReportToFile(
            *resigned.finalServiceReportFileSerialNumber,
            L"ResignFederationExecution",
            {{umbra::detail::MomArgumentType::resign_action,
              L"HLAresignAction",
              umbra::detail::formatMomResignAction(resignAction)}});
      } catch (RTIinternalError const&) {
        // Membership is already irrevocably removed.  Finish tearing down the
        // local joined-federate lifetime and submit its surviving work before
        // surfacing the deterministic file error; do not retain a stale
        // writer or silently replace it with an in-memory sink.
        finalServiceReportAppendFailed = true;
      }
    }
    synchronizationNotifications = std::move(resigned.synchronizationNotifications);
    saveNotifications = std::move(resigned.saveNotifications);
    restoreNotifications = std::move(resigned.restoreNotifications);
    resignObjectRemovals.reserve(resigned.resignObjectRemovals.size());
    for (auto& removal : resigned.resignObjectRemovals) {
      resignObjectRemovals.push_back({
          removal.receivingFederateId,
          removal.objectInstanceHandle,
          std::move(removal.callbackRoute),
          std::move(removal.serviceReportRoute),
          removal.rtiOwnedMomObject,
      });
    }
    resignOwnershipAssumptions.reserve(resigned.resignOwnershipAssumptions.size());
    for (auto& assumption : resigned.resignOwnershipAssumptions) {
      resignOwnershipAssumptions.push_back({
          assumption.receivingFederateId,
          assumption.objectInstanceHandle,
          std::move(assumption.attributeHandles),
          std::move(assumption.callbackRoute),
      });
    }
    resignOwnershipAcquisitionWorkItems =
        std::move(resigned.resignOwnershipAcquisitionWorkItems);
    // Membership removal can itself eliminate the last active subscriber for
    // another publisher. Compute the ordinary declaration advisories while
    // the registry still has the post-resign state, but queue them only after
    // all federation and ambassador locks have been released.
    declarationAdvisories = embeddedFederationManagement().registry()
        .planDeclarationAdvisories(federationName);

    if (federateTimeState_) {
      federateTimeState_->deactivate();
    }
    federateTimeState_.reset();
    // Do not retain a report writer beyond the resigned membership.  The
    // filesystem file is intentionally neither truncated nor removed.
    joinedServiceReport_.reset();
    joinedFederationName_.reset();
    joinedFederateId_.reset();

    auto scheduled = embeddedFederationManagement().registry().reevaluateTimeAdvanceGrants(
        federationName);
    if (scheduled.status == umbra::detail::FederationTimeGrantStatus::applied) {
      newlyEligible = std::move(scheduled.dispatches);
    }
    auto timedSave = embeddedFederationManagement().registry()
        .reevaluateTimedFederationSave(federationName);
    if (timedSave.status == umbra::detail::FederationSaveControlStatus::applied) {
      for (auto& notification : timedSave.notifications) {
        saveNotifications.push_back(std::move(notification));
      }
    }
  }
  submitTimeAdvanceGrantDispatches(std::move(newlyEligible));
  submitFederationSaveNotifications(
      federationName,
      std::move(saveNotifications));
  submitFederationRestoreNotifications(
      federationName,
      std::move(restoreNotifications));
  submitFederationSynchronizedNotifications(std::move(synchronizationNotifications));
  queueDeclarationAdvisories(std::move(declarationAdvisories));
  VariableLengthData emptyResignTag;
  queueAttributeOwnershipAcquisitionWorkItems(
      std::move(resignOwnershipAcquisitionWorkItems),
      federationName);
  queueAttributeOwnershipAssumptionRecipients(
      std::move(resignOwnershipAssumptions),
      federationName,
      emptyResignTag);
  queueObjectInstanceRemovals(
      std::move(resignObjectRemovals),
      federationName,
      emptyResignTag);
  queueFederationMomConditionalAttributeUpdate(
      federationName,
      {"HLAfederatesInFederation"});
  if (finalServiceReportAppendFailed) {
    throw RTIinternalError(
        L"Umbra could not append the selected service-report file record.");
  }
  } catch (Exception const& exception) {
    emitExceptionReport(L"Resign Federation Execution", exception);
    throw;
  }
}

void UmbraRtiAmbassador::registerFederationSynchronizationPoint(
    std::wstring const& synchronizationPointLabel,
    VariableLengthData const& userSuppliedTag) {
  auto instrumentationScope = beginRtiCall("registerFederationSynchronizationPoint");
  registerFederationSynchronizationPointImpl(
      synchronizationPointLabel,
      userSuppliedTag,
      FederateHandleSet{},
      false);
}

void UmbraRtiAmbassador::registerFederationSynchronizationPoint(
    std::wstring const& synchronizationPointLabel,
    VariableLengthData const& userSuppliedTag,
    FederateHandleSet const& synchronizationSet) {
  auto instrumentationScope = beginRtiCall("registerFederationSynchronizationPoint");
  registerFederationSynchronizationPointImpl(
      synchronizationPointLabel,
      userSuppliedTag,
      synchronizationSet,
      true);
}

void UmbraRtiAmbassador::registerFederationSynchronizationPointImpl(
    std::wstring const& synchronizationPointLabel,
    VariableLengthData const& userSuppliedTag,
    FederateHandleSet const& synchronizationSet,
    bool synchronizationSetWasSupplied) {
  try {
  std::vector<unsigned char> copiedTag;
  if (userSuppliedTag.size() != 0) {
    auto const* data = static_cast<unsigned char const*>(userSuppliedTag.data());
    if (data == nullptr) {
      throw RTIinternalError(
          L"The synchronization-point tag reported a nonzero size with no data pointer.");
    }
    copiedTag.assign(data, data + userSuppliedTag.size());
  }
  auto const reportUserSuppliedTag =
      umbra::detail::formatMomUserSuppliedTag(userSuppliedTag);

  std::set<std::uint64_t> requestedFederateIds;
  for (auto const& federate : synchronizationSet) {
    auto const federateId = federateHandleValue(federate);
    if (!federateId) {
      throw InvalidFederateHandle(
          L"Register Federation Synchronization Point requires valid FederateHandle values.");
    }
    requestedFederateIds.insert(*federateId);
  }

  umbra::detail::SynchronizationPointRegistrationPlan plan;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Register Federation Synchronization Point");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Register Federation Synchronization Point requires membership in a federation execution.");
    }
    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    plan = registry.registerSynchronizationPoint(
        *joinedFederationName_,
        *joinedFederateId_,
        synchronizationPointLabel,
        std::move(copiedTag),
        requestedFederateIds);
    switch (plan.status) {
      case umbra::detail::SynchronizationPointRegistrationStatus::applied:
        break;
      case umbra::detail::SynchronizationPointRegistrationStatus::federation_does_not_exist:
      case umbra::detail::SynchronizationPointRegistrationStatus::federate_not_member:
        throw FederateNotExecutionMember(
            L"The embedded federation no longer records this RTI ambassador as a member.");
      case umbra::detail::SynchronizationPointRegistrationStatus::callback_route_missing:
        throw RTIinternalError(
            L"The embedded federation has no callback route for synchronization-point registration.");
    }
    // Section 4.14 supplies three arguments and returns None. Section 11.5.1
    // requires every optional argument position to be retained as Null when
    // unused, so the two public C++ overloads must not collapse an omitted
    // synchronization set into a supplied-empty Array<FederateHandle>.
    // This invocation record is committed before its separate §4.15
    // registration-result and §4.16 announcement callbacks are queued. The
    // §4.15 RTI-invoked confirmation is then an independently reportable
    // service at this same joined federate: retain its optional failure slot
    // as Null on success and use the Table 5 type-56 enum on failure.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"RegisterFederationSynchronizationPoint",
        umbra::detail::MomServiceType::federation_management,
        {{umbra::detail::MomArgumentType::string,
          L"Synchronization point label",
          umbra::detail::formatMomString(synchronizationPointLabel)},
         {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
          L"User-supplied tag",
          reportUserSuppliedTag},
         {synchronizationSetWasSupplied
              ? umbra::detail::MomArgumentType::federate_handle_set
              : umbra::detail::MomArgumentType::null_value,
          L"Optional set of joined federate designators",
          synchronizationSetWasSupplied
              ? umbra::detail::formatMomFederateHandleSet(synchronizationSet)
              : umbra::detail::formatMomNull()}});
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"ConfirmSynchronizationPointRegistration",
        umbra::detail::MomServiceType::federation_management,
        {{umbra::detail::MomArgumentType::string,
          L"Synchronization point label",
          umbra::detail::formatMomString(synchronizationPointLabel)},
         {umbra::detail::MomArgumentType::boolean,
          L"Registration-success indicator",
          umbra::detail::formatMomBoolean(plan.succeeded)},
         {plan.succeeded
              ? umbra::detail::MomArgumentType::null_value
              : umbra::detail::MomArgumentType::synchronization_point_failure_reason,
          L"Optional failure reason",
          plan.succeeded
              ? umbra::detail::formatMomNull()
              : umbra::detail::formatMomSynchronizationPointFailureReason(
                    plan.failureReason)}});
  }
  submitSynchronizationPointRegistration(std::move(plan));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Register Federation Synchronization Point", exception);
    throw;
  }
}

void UmbraRtiAmbassador::synchronizationPointAchieved(
    std::wstring const& synchronizationPointLabel,
    bool successfully) {
  auto instrumentationScope = beginRtiCall("synchronizationPointAchieved");
  try {
  umbra::detail::SynchronizationPointAchievedPlan plan;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Synchronization Point Achieved");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Synchronization Point Achieved requires membership in a federation execution.");
    }
    auto& registry = embeddedFederationManagement().registry();
    plan = registry.achieveSynchronizationPoint(
        *joinedFederationName_,
        *joinedFederateId_,
        synchronizationPointLabel,
        successfully);
    switch (plan.status) {
      case umbra::detail::SynchronizationPointAchievedStatus::applied:
        break;
      case umbra::detail::SynchronizationPointAchievedStatus::federation_does_not_exist:
      case umbra::detail::SynchronizationPointAchievedStatus::federate_not_member:
        throw FederateNotExecutionMember(
            L"The embedded federation no longer records this RTI ambassador as a member.");
      case umbra::detail::SynchronizationPointAchievedStatus::
          synchronization_point_label_not_announced:
        throw SynchronizationPointLabelNotAnnounced(
            L"The supplied synchronization-point label has not been announced to this federate.");
    }
    // Section 4.17 defines the accepted achievement as the service boundary;
    // the distinct Federation Synchronized callbacks are consequences that
    // are queued only after this source-backed successful-void record exists.
    // The C++ defaulted Boolean represents the effective optional
    // synchronization-success indicator, just as the report wrappers for
    // other defaulted Boolean selectors do.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"SynchronizationPointAchieved",
        umbra::detail::MomServiceType::federation_management,
        {{umbra::detail::MomArgumentType::string,
          L"Synchronization point label",
          umbra::detail::formatMomString(synchronizationPointLabel)},
         {umbra::detail::MomArgumentType::boolean,
          L"Optional synchronization-success indicator",
          umbra::detail::formatMomBoolean(successfully)}});
  }
  submitFederationSynchronizedNotifications(
      std::move(plan.synchronizationNotifications));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Synchronization Point Achieved", exception);
    throw;
  }
}

void UmbraRtiAmbassador::requestFederationSave(std::wstring const& label) {
  auto instrumentationScope = beginRtiCall("requestFederationSave");
  try {
  std::vector<umbra::detail::FederationSaveNotification> notifications;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Request Federation Save requires membership in a federation execution.");
    }

    auto result = embeddedFederationManagement().registry().requestFederationSave(
        *joinedFederationName_,
        *joinedFederateId_,
        label);
    if (result.status != umbra::detail::FederationSaveControlStatus::applied) {
      throwFederationSaveServiceFailure(
          result.status,
          L"Request Federation Save",
          FederationSaveServiceFailure::request);
    }
    // The official C++ binding exposes §4.19's optional timestamp through
    // two overloads.  This untimed entry point must retain the corresponding
    // Table 5 argument position as Null rather than conflating it with a
    // supplied logical-time value.  The accepted request is reportable before
    // its separately queued Initiate Federate Save callbacks are submitted.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"RequestFederationSave",
        umbra::detail::MomServiceType::federation_management,
        {{umbra::detail::MomArgumentType::string,
          L"Federation save label",
          umbra::detail::formatMomString(label)},
         {umbra::detail::MomArgumentType::null_value,
          L"Optional timestamp",
          umbra::detail::formatMomNull()}});
    notifications = std::move(result.notifications);
  }
  submitFederationSaveNotifications(
      *joinedFederationName_,
      std::move(notifications));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Request Federation Save", exception);
    throw;
  }
}

void UmbraRtiAmbassador::requestFederationSave(
    std::wstring const& label,
    LogicalTime const& time) {
  auto instrumentationScope = beginRtiCall("requestFederationSave");
  try {
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  std::wstring federationName;
  std::uint64_t federateId = 0;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Request Federation Save with a timestamp requires membership in a federation execution.");
    }
    timeState = federateTimeState_;
    federationName = *joinedFederationName_;
    federateId = *joinedFederateId_;
  }

  // Decode the caller's polymorphic LogicalTime before taking runtime locks.
  // This both fences the selected IEEE implementation and gives the public
  // InvalidLogicalTime mapping for malformed or initial/final values.
  auto timestamp = cloneReferenceLogicalTime(timeState->implementationName(), time);
  validateTsoTimestamp(timeState->snapshot(), *timestamp);
  // Table 5 gives LogicalTime the quoted value from time.toString().  Format
  // the private clone before locking, then commit that exact supplied
  // timestamp only if §4.19 accepts the request below.
  auto const reportTimestampArgument = umbra::detail::MomServiceArgument{
      umbra::detail::MomArgumentType::logical_time,
      L"Optional timestamp",
      umbra::detail::formatMomLogicalTime(*timestamp),
  };

  umbra::detail::FederationTimeExecutionSnapshot execution;
  {
    std::scoped_lock lock(federationManagementMutex());
    auto snapshot = embeddedFederationManagement().registry().timeSnapshotFor(
        federationName);
    if (!snapshot) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    execution = std::move(*snapshot);
  }

  auto const requester = std::find_if(
      execution.federates.begin(),
      execution.federates.end(),
      [federateId](umbra::detail::FederationTimeFederateSnapshot const& candidate) {
        return candidate.membership.id == federateId;
      });
  if (requester == execution.federates.end()) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  if (requester->time.implementationName != timeState->implementationName() ||
      execution.definition.logicalTimeImplementationName != timeState->implementationName()) {
    throw RTIinternalError(
        L"The joined federation has inconsistent logical-time implementation state.");
  }

  auto equalOrEarlier = [](LogicalTime const& left, LogicalTime const& right) {
    try {
      return left <= right;
    } catch (Exception const&) {
      throw InvalidLogicalTime(
          L"The timestamped federation-save request cannot be compared in the selected implementation.");
    }
  };

  if (!requester->time.timeRegulating) {
    auto const bounds = umbra::detail::FederationTimeBoundsCalculator{}.calculate(
        execution,
        federateId);
    switch (bounds.status) {
      case umbra::detail::FederationTimeBoundStatus::available:
        if (!bounds.galt) {
          throw RTIinternalError(
              L"Umbra computed an available GALT without a logical-time value.");
        }
        if (equalOrEarlier(*timestamp, *bounds.galt)) {
          throw LogicalTimeAlreadyPassed(
              L"The timestamped federation-save request must be later than the joined federate's GALT.");
        }
        break;
      case umbra::detail::FederationTimeBoundStatus::undefined:
        throw FederateUnableToUseTime(
            L"A non-regulating federate may request a timestamped save only when GALT is defined.");
      case umbra::detail::FederationTimeBoundStatus::requesting_federate_not_registered:
        throw FederateNotExecutionMember(
            L"The embedded federation no longer records this RTI ambassador as a member.");
      case umbra::detail::FederationTimeBoundStatus::factory_unavailable:
      case umbra::detail::FederationTimeBoundStatus::inconsistent_temporal_state:
        throw RTIinternalError(
            L"Umbra could not calculate a coherent GALT for the timestamped federation-save request.");
    }
  } else if (requester->time.currentTime &&
             equalOrEarlier(*timestamp, *requester->time.currentTime)) {
    throw LogicalTimeAlreadyPassed(
        L"The timestamped federation-save request is not later than the regulating federate's current time.");
  }

  // A timestamped request must be strictly beyond every current position of a
  // constrained member.  The registry repeats this check at commit time so a
  // concurrent grant/resign cannot turn an admitted request into an invalid
  // one.
  for (auto const& federate : execution.federates) {
    if (!federate.time.timeConstrained) {
      continue;
    }
    if (!federate.time.currentTime) {
      throw RTIinternalError(
          L"A time-constrained federate has no current logical time for timed-save validation.");
    }
    if (equalOrEarlier(*timestamp, *federate.time.currentTime)) {
      throw LogicalTimeAlreadyPassed(
          L"The timestamped federation-save request is not later than every time-constrained federate.");
    }
  }

  std::vector<umbra::detail::FederationSaveNotification> notifications;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_ ||
        *joinedFederationName_ != federationName || *joinedFederateId_ != federateId ||
        federateTimeState_ != timeState) {
      throw FederateNotExecutionMember(
          L"Request Federation Save with a timestamp requires an active joined federate.");
    }

    auto result = embeddedFederationManagement().registry().requestFederationSave(
        federationName,
        federateId,
        label,
        std::move(timestamp));
    if (result.status != umbra::detail::FederationSaveControlStatus::applied) {
      throwFederationSaveServiceFailure(
          result.status,
          L"Request Federation Save",
          FederationSaveServiceFailure::request);
    }
    // This is the supplied-timestamp counterpart to the overload above.  A
    // successful request records the admitted §4.19 invocation before any
    // time-bound Initiate Federate Save callback work becomes eligible.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"RequestFederationSave",
        umbra::detail::MomServiceType::federation_management,
        {{umbra::detail::MomArgumentType::string,
          L"Federation save label",
          umbra::detail::formatMomString(label)},
         reportTimestampArgument});
    notifications = std::move(result.notifications);
  }
  submitFederationSaveNotifications(
      federationName,
      std::move(notifications));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Request Federation Save", exception);
    throw;
  }
}

void UmbraRtiAmbassador::federateSaveBegun() {
  auto instrumentationScope = beginRtiCall("federateSaveBegun");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Federate Save Begun requires membership in a federation execution.");
  }

  auto result = embeddedFederationManagement().registry().federateSaveBegun(
      *joinedFederationName_,
      *joinedFederateId_);
  if (result.status != umbra::detail::FederationSaveControlStatus::applied) {
    throwFederationSaveServiceFailure(
        result.status,
        L"Federate Save Begun",
        FederationSaveServiceFailure::begun);
  }
  // Section 4.21 has no supplied or returned arguments. Its accepted state
  // transition is therefore the report boundary; save completion/failure and
  // the later Federation Saved callback remain separate service boundaries.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"FederateSaveBegun",
      umbra::detail::MomServiceType::federation_management,
      {});
  } catch (Exception const& exception) {
    emitExceptionReport(L"Federate Save Begun", exception);
    throw;
  }
}

void UmbraRtiAmbassador::federateSaveComplete() {
  auto instrumentationScope = beginRtiCall("federateSaveComplete");
  try {
  std::vector<umbra::detail::FederationSaveNotification> notifications;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Federate Save Complete requires membership in a federation execution.");
    }

    auto result = embeddedFederationManagement().registry().federateSaveComplete(
        *joinedFederationName_,
        *joinedFederateId_);
    if (result.status != umbra::detail::FederationSaveControlStatus::applied) {
      throwFederationSaveServiceFailure(
          result.status,
          L"Federate Save Complete",
          FederationSaveServiceFailure::completion);
    }
    // The official C++ binding represents the §4.22 success selector through
    // separate Complete/Not Complete calls. Both are the one Federate Save
    // Complete service report, distinguished by its required Boolean argument.
    // Write the accepted service record before the registry's eventual
    // Federation Saved notification can be submitted below.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"FederateSaveComplete",
        umbra::detail::MomServiceType::federation_management,
        {{umbra::detail::MomArgumentType::boolean,
          L"Federate save-success indicator",
          umbra::detail::formatMomBoolean(true)}});
    notifications = std::move(result.notifications);
  }
  submitFederationSaveNotifications(
      *joinedFederationName_,
      std::move(notifications));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Federate Save Complete", exception);
    throw;
  }
}

void UmbraRtiAmbassador::federateSaveNotComplete() {
  auto instrumentationScope = beginRtiCall("federateSaveNotComplete");
  try {
  std::vector<umbra::detail::FederationSaveNotification> notifications;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Federate Save Not Complete requires membership in a federation execution.");
    }

    auto result = embeddedFederationManagement().registry().federateSaveNotComplete(
        *joinedFederationName_,
        *joinedFederateId_);
    if (result.status != umbra::detail::FederationSaveControlStatus::applied) {
      throwFederationSaveServiceFailure(
          result.status,
          L"Federate Save Not Complete",
          FederationSaveServiceFailure::completion);
    }
    // §4.22 has one reportable service. The C++ failure spelling selects the
    // same service with its required save-success indicator set to false.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"FederateSaveComplete",
        umbra::detail::MomServiceType::federation_management,
        {{umbra::detail::MomArgumentType::boolean,
          L"Federate save-success indicator",
          umbra::detail::formatMomBoolean(false)}});
    notifications = std::move(result.notifications);
  }
  submitFederationSaveNotifications(
      *joinedFederationName_,
      std::move(notifications));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Federate Save Complete", exception);
    throw;
  }
}

void UmbraRtiAmbassador::abortFederationSave() {
  auto instrumentationScope = beginRtiCall("abortFederationSave");
  try {
  std::vector<umbra::detail::FederationSaveNotification> notifications;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Abort Federation Save requires membership in a federation execution.");
    }

    auto result = embeddedFederationManagement().registry().abortFederationSave(
        *joinedFederationName_,
        *joinedFederateId_);
    if (result.status != umbra::detail::FederationSaveControlStatus::applied) {
      throwFederationSaveServiceFailure(
          result.status,
          L"Abort Federation Save",
          FederationSaveServiceFailure::abort);
    }
    // Section 4.24 has no supplied or returned arguments. The accepted abort
    // request is the report boundary; the later Federation Saved/Not Saved
    // callback communicates the result of the aborted save operation.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"AbortFederationSave",
        umbra::detail::MomServiceType::federation_management,
        {});
    notifications = std::move(result.notifications);
  }
  submitFederationSaveNotifications(
      *joinedFederationName_,
      std::move(notifications));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Abort Federation Save", exception);
    throw;
  }
}

void UmbraRtiAmbassador::queryFederationSaveStatus() {
  auto instrumentationScope = beginRtiCall("queryFederationSaveStatus");
  try {
  std::vector<umbra::detail::FederationSaveNotification> notifications;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Query Federation Save Status requires membership in a federation execution.");
    }

    auto result = embeddedFederationManagement().registry().queryFederationSaveStatus(
        *joinedFederationName_,
        *joinedFederateId_);
    if (result.status != umbra::detail::FederationSaveControlStatus::applied) {
      throwFederationSaveServiceFailure(
          result.status,
          L"Query Federation Save Status",
          FederationSaveServiceFailure::query);
    }
    // Section 4.25 has no supplied or returned arguments. Report the accepted
    // query before its separately queued Federation Save Status Response
    // callback provides the current member-status vector.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"QueryFederationSaveStatus",
        umbra::detail::MomServiceType::federation_management,
        {});
    notifications = std::move(result.notifications);
  }
  submitFederationSaveNotifications(
      *joinedFederationName_,
      std::move(notifications));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Query Federation Save Status", exception);
    throw;
  }
}

void UmbraRtiAmbassador::requestFederationRestore(std::wstring const& label) {
  auto instrumentationScope = beginRtiCall("requestFederationRestore");
  try {
  std::vector<umbra::detail::FederationRestoreNotification> notifications;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Request Federation Restore requires membership in a federation execution.");
    }

    auto result = embeddedFederationManagement().registry().requestFederationRestore(
        *joinedFederationName_,
        *joinedFederateId_,
        label);
    if (result.status != umbra::detail::FederationRestoreControlStatus::applied &&
        result.status != umbra::detail::FederationRestoreControlStatus::snapshot_not_found &&
        result.status != umbra::detail::FederationRestoreControlStatus::membership_mismatch) {
      throwFederationRestoreServiceFailure(
          result.status,
          L"Request Federation Restore",
          FederationRestoreServiceFailure::request);
    }
    // Section 4.27 has one supplied Federation save label and no returned
    // arguments.  A missing snapshot or membership mismatch is communicated
    // by the separately queued Confirm Federation Restoration Request
    // callback, not by failure of this public service invocation.  Record all
    // normally returned request forms before submitting those callbacks.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"RequestFederationRestore",
        umbra::detail::MomServiceType::federation_management,
        {{umbra::detail::MomArgumentType::string,
          L"Federation save label",
          umbra::detail::formatMomString(label)}});
    notifications = std::move(result.notifications);
  }
  submitFederationRestoreNotifications(
      *joinedFederationName_,
      std::move(notifications));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Request Federation Restore", exception);
    throw;
  }
}

void UmbraRtiAmbassador::federateRestoreComplete() {
  auto instrumentationScope = beginRtiCall("federateRestoreComplete");
  try {
  std::vector<umbra::detail::FederationRestoreNotification> notifications;
  std::vector<umbra::detail::FederationTimeGrantDispatch> timeAdvanceGrantDispatches;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Federate Restore Complete requires membership in a federation execution.");
    }

    auto result = embeddedFederationManagement().registry().federateRestoreComplete(
        *joinedFederationName_,
        *joinedFederateId_);
    if (result.status != umbra::detail::FederationRestoreControlStatus::applied) {
      throwFederationRestoreServiceFailure(
          result.status,
          L"Federate Restore Complete",
          FederationRestoreServiceFailure::completion);
    }
    // The official C++ binding represents §4.31's one required
    // restore-success indicator with the Complete/Not Complete selector pair.
    // Both forms therefore use the single Federate Restore Complete service
    // report, distinguished by the required Boolean, before the registry's
    // Federation Restored callback and any restored time-grant work are
    // submitted below.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"FederateRestoreComplete",
        umbra::detail::MomServiceType::federation_management,
        {{umbra::detail::MomArgumentType::boolean,
          L"Federate restore-success indicator",
          umbra::detail::formatMomBoolean(true)}});
    notifications = std::move(result.notifications);
    timeAdvanceGrantDispatches = std::move(result.timeAdvanceGrantDispatches);
  }
  submitFederationRestoreNotifications(
      *joinedFederationName_,
      std::move(notifications));
  submitTimeAdvanceGrantDispatches(std::move(timeAdvanceGrantDispatches));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Federate Restore Complete", exception);
    throw;
  }
}

void UmbraRtiAmbassador::federateRestoreNotComplete() {
  auto instrumentationScope = beginRtiCall("federateRestoreNotComplete");
  try {
  std::vector<umbra::detail::FederationRestoreNotification> notifications;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Federate Restore Not Complete requires membership in a federation execution.");
    }

    auto result = embeddedFederationManagement().registry().federateRestoreNotComplete(
        *joinedFederationName_,
        *joinedFederateId_);
    if (result.status != umbra::detail::FederationRestoreControlStatus::applied) {
      throwFederationRestoreServiceFailure(
          result.status,
          L"Federate Restore Not Complete",
          FederationRestoreServiceFailure::completion);
    }
    // §4.31 has the same reportable service for the C++ failure selector; its
    // required restore-success indicator is false rather than making this a
    // distinct successful-void service name.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"FederateRestoreComplete",
        umbra::detail::MomServiceType::federation_management,
        {{umbra::detail::MomArgumentType::boolean,
          L"Federate restore-success indicator",
          umbra::detail::formatMomBoolean(false)}});
    notifications = std::move(result.notifications);
  }
  submitFederationRestoreNotifications(
      *joinedFederationName_,
      std::move(notifications));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Federate Restore Complete", exception);
    throw;
  }
}

void UmbraRtiAmbassador::abortFederationRestore() {
  auto instrumentationScope = beginRtiCall("abortFederationRestore");
  try {
  std::vector<umbra::detail::FederationRestoreNotification> notifications;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Abort Federation Restore requires membership in a federation execution.");
    }

    auto result = embeddedFederationManagement().registry().abortFederationRestore(
        *joinedFederationName_,
        *joinedFederateId_);
    if (result.status != umbra::detail::FederationRestoreControlStatus::applied) {
      throwFederationRestoreServiceFailure(
          result.status,
          L"Abort Federation Restore",
          FederationRestoreServiceFailure::abort);
    }
    // Section 4.33 has no supplied or returned arguments. The accepted abort
    // request is the report boundary; the later restore-result callback
    // communicates the outcome of the aborted restore operation.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"AbortFederationRestore",
        umbra::detail::MomServiceType::federation_management,
        {});
    notifications = std::move(result.notifications);
  }
  submitFederationRestoreNotifications(
      *joinedFederationName_,
      std::move(notifications));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Abort Federation Restore", exception);
    throw;
  }
}

void UmbraRtiAmbassador::queryFederationRestoreStatus() {
  auto instrumentationScope = beginRtiCall("queryFederationRestoreStatus");
  try {
  std::vector<umbra::detail::FederationRestoreNotification> notifications;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Query Federation Restore Status requires membership in a federation execution.");
    }

    auto result = embeddedFederationManagement().registry().queryFederationRestoreStatus(
        *joinedFederationName_,
        *joinedFederateId_);
    if (result.status != umbra::detail::FederationRestoreControlStatus::applied) {
      throwFederationRestoreServiceFailure(
          result.status,
          L"Query Federation Restore Status",
          FederationRestoreServiceFailure::query);
    }
    // Section 4.34 has no supplied or returned arguments. The accepted query
    // is the report boundary; the later Federation Restore Status Response
    // callback carries the restore-status descriptor vector.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"QueryFederationRestoreStatus",
        umbra::detail::MomServiceType::federation_management,
        {});
    notifications = std::move(result.notifications);
  }
  submitFederationRestoreNotifications(
      *joinedFederationName_,
      std::move(notifications));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Query Federation Restore Status", exception);
    throw;
  }
}

std::unique_ptr<LogicalTimeFactory> UmbraRtiAmbassador::getTimeFactory() const {
  auto instrumentationScope = beginRtiCall("getTimeFactory");
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"A logical-time factory is available only to a joined federate.");
  }

  auto definition = embeddedFederationManagement().registry().definitionFor(*joinedFederationName_);
  if (!definition) {
    // The registry cannot normally lose a federation with an active member;
    // preserve the public membership error if an external backend later makes
    // that state observable.
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador's federation execution.");
  }
  return makeDefinitionTimeFactory(embeddedFederationManagement(), *definition);
}

FederateHandle UmbraRtiAmbassador::getFederateHandle(std::wstring const& federateName) {
  auto instrumentationScope = beginRtiCall("getFederateHandle");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Federate Handle requires membership in a federation execution.");
  }

  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }

  auto membership = registry.memberByName(*joinedFederationName_, federateName);
  if (!membership) {
    throw NameNotFound(L"The supplied federate name is not active in this federation execution.");
  }
  if (membership->id == 0) {
    throw RTIinternalError(L"The embedded federation returned an invalid federate identity.");
  }
  return makeFederateHandle(membership->id);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Federate Handle", exception);
    throw;
  }
}

std::wstring UmbraRtiAmbassador::getFederateName(FederateHandle const& federate) {
  auto instrumentationScope = beginRtiCall("getFederateName");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Federate Name requires membership in a federation execution.");
  }

  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }

  auto const federateId = federateHandleValue(federate);
  if (!federateId) {
    throw InvalidFederateHandle(L"Get Federate Name requires a valid FederateHandle.");
  }

  auto federateName = registry.federateNameFor(*joinedFederationName_, *federateId);
  if (!federateName) {
    throw FederateHandleNotKnown(
        L"The supplied FederateHandle is not known in this federation execution.");
  }
  return *federateName;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Federate Name", exception);
    throw;
  }
}

ObjectClassHandle UmbraRtiAmbassador::getObjectClassHandle(
    std::wstring const& objectClassName) {
  auto instrumentationScope = beginRtiCall("getObjectClassHandle");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Object Class Handle requires membership in a federation execution.");
  }

  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }

  auto const encodedName = umbra::detail::utf8FromWide(objectClassName);
  if (!encodedName) {
    throw NameNotFound(L"The supplied object class name is not valid UTF-8 text.");
  }
  auto const handle = registry.objectClassHandleFor(*joinedFederationName_, *encodedName);
  if (!handle) {
    throw NameNotFound(L"The supplied object class name is not defined in this federation execution.");
  }
  return makeObjectClassHandle(*handle);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Object Class Handle", exception);
    throw;
  }
}

std::wstring UmbraRtiAmbassador::getObjectClassName(ObjectClassHandle const& objectClass) {
  auto instrumentationScope = beginRtiCall("getObjectClassName");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Object Class Name requires membership in a federation execution.");
  }

  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }

  auto const handle = objectClassHandleValue(objectClass);
  if (!handle) {
    throw InvalidObjectClassHandle(L"Get Object Class Name requires a valid ObjectClassHandle.");
  }
  auto const encodedName = registry.objectClassNameFor(*joinedFederationName_, *handle);
  if (!encodedName) {
    throw InvalidObjectClassHandle(
        L"The supplied ObjectClassHandle is not known in this federation execution.");
  }
  auto const objectClassName = umbra::detail::wideFromUtf8(*encodedName);
  if (!objectClassName) {
    throw RTIinternalError(
        L"The embedded federation stored an object class name that is not valid UTF-8 text.");
  }
  return *objectClassName;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Object Class Name", exception);
    throw;
  }
}

AttributeHandle UmbraRtiAmbassador::getAttributeHandle(
    ObjectClassHandle const& objectClass,
    std::wstring const& attributeName) {
  auto instrumentationScope = beginRtiCall("getAttributeHandle");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Attribute Handle requires membership in a federation execution.");
  }

  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }

  auto const objectClassHandle = objectClassHandleValue(objectClass);
  if (!objectClassHandle) {
    throw InvalidObjectClassHandle(
        L"Get Attribute Handle requires a valid ObjectClassHandle.");
  }
  auto const encodedObjectClassName = registry.objectClassNameFor(
      *joinedFederationName_,
      *objectClassHandle);
  if (!encodedObjectClassName) {
    throw InvalidObjectClassHandle(
        L"The supplied ObjectClassHandle is not known in this federation execution.");
  }

  auto const encodedAttributeName = umbra::detail::utf8FromWide(attributeName);
  if (!encodedAttributeName) {
    throw NameNotFound(L"The supplied attribute name is not valid UTF-8 text.");
  }
  auto const attributeHandle = registry.attributeHandleFor(
      *joinedFederationName_,
      *encodedObjectClassName,
      *encodedAttributeName);
  if (!attributeHandle) {
    throw NameNotFound(L"The supplied attribute name is not defined for this object class.");
  }
  return makeAttributeHandle(*attributeHandle);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Attribute Handle", exception);
    throw;
  }
}

std::wstring UmbraRtiAmbassador::getAttributeName(
    ObjectClassHandle const& objectClass,
    AttributeHandle const& attribute) {
  auto instrumentationScope = beginRtiCall("getAttributeName");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Attribute Name requires membership in a federation execution.");
  }

  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }

  auto const objectClassHandle = objectClassHandleValue(objectClass);
  if (!objectClassHandle) {
    throw InvalidObjectClassHandle(
        L"Get Attribute Name requires a valid ObjectClassHandle.");
  }
  auto const encodedObjectClassName = registry.objectClassNameFor(
      *joinedFederationName_,
      *objectClassHandle);
  if (!encodedObjectClassName) {
    throw InvalidObjectClassHandle(
        L"The supplied ObjectClassHandle is not known in this federation execution.");
  }

  auto const attributeHandle = attributeHandleValue(attribute);
  if (!attributeHandle) {
    throw InvalidAttributeHandle(L"Get Attribute Name requires a valid AttributeHandle.");
  }
  auto const encodedAttributeName = registry.attributeNameFor(
      *joinedFederationName_,
      *encodedObjectClassName,
      *attributeHandle);
  if (!encodedAttributeName) {
    throw AttributeNotDefined(
        L"The supplied AttributeHandle is not defined for this object class.");
  }
  auto const attributeName = umbra::detail::wideFromUtf8(*encodedAttributeName);
  if (!attributeName) {
    throw RTIinternalError(
        L"The embedded federation stored an attribute name that is not valid UTF-8 text.");
  }
  return *attributeName;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Attribute Name", exception);
    throw;
  }
}

double UmbraRtiAmbassador::getUpdateRateValue(
    std::wstring const& updateRateDesignator) {
  auto instrumentationScope = beginRtiCall("getUpdateRateValue");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Update Rate Value requires membership in a federation execution.");
  }

  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }

  auto const encodedDesignator = umbra::detail::utf8FromWide(updateRateDesignator);
  if (!encodedDesignator) {
    throw InvalidUpdateRateDesignator(
        L"The supplied update-rate designator is not valid UTF-8 text.");
  }
  auto const result = registry.updateRateValueForDesignator(
      *joinedFederationName_,
      *joinedFederateId_,
      *encodedDesignator);
  switch (result.status) {
    case umbra::detail::UpdateRateValueStatus::applied:
      return result.value;
    case umbra::detail::UpdateRateValueStatus::federation_does_not_exist:
    case umbra::detail::UpdateRateValueStatus::federate_not_member:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    case umbra::detail::UpdateRateValueStatus::invalid_update_rate_designator:
      throw InvalidUpdateRateDesignator(
          L"The supplied update-rate designator is not defined by the current FDD.");
    case umbra::detail::UpdateRateValueStatus::object_instance_not_known:
    case umbra::detail::UpdateRateValueStatus::attribute_not_defined:
      throw RTIinternalError(
          L"The embedded federation returned an invalid update-rate query outcome.");
    case umbra::detail::UpdateRateValueStatus::inconsistent_catalog:
      throw RTIinternalError(
          L"The embedded federation could not resolve the current FDD update-rate table.");
  }
  throw RTIinternalError(L"Umbra encountered an unknown update-rate query outcome.");
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Update Rate Value", exception);
    throw;
  }
}

double UmbraRtiAmbassador::getUpdateRateValueForAttribute(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandle const& attribute) {
  auto instrumentationScope = beginRtiCall("getUpdateRateValueForAttribute");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Update Rate Value For Attribute requires membership in a federation execution.");
  }

  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }

  auto const objectInstanceValue = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceValue) {
    throw ObjectInstanceNotKnown(
        L"Get Update Rate Value For Attribute requires a known ObjectInstanceHandle.");
  }
  auto const attributeValue = attributeHandleValue(attribute);
  if (!attributeValue) {
    throw AttributeNotDefined(
        L"Get Update Rate Value For Attribute requires a defined AttributeHandle.");
  }

  auto const result = registry.updateRateValueForAttribute(
      *joinedFederationName_,
      *joinedFederateId_,
      *objectInstanceValue,
      *attributeValue);
  switch (result.status) {
    case umbra::detail::UpdateRateValueStatus::applied:
      return result.value;
    case umbra::detail::UpdateRateValueStatus::federation_does_not_exist:
    case umbra::detail::UpdateRateValueStatus::federate_not_member:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    case umbra::detail::UpdateRateValueStatus::object_instance_not_known:
      throw ObjectInstanceNotKnown(
          L"The supplied ObjectInstanceHandle is not known to this federate.");
    case umbra::detail::UpdateRateValueStatus::attribute_not_defined:
      throw AttributeNotDefined(
          L"The supplied AttributeHandle is not defined for this known object instance.");
    case umbra::detail::UpdateRateValueStatus::invalid_update_rate_designator:
      throw RTIinternalError(
          L"The embedded federation returned an invalid update-rate designator outcome.");
    case umbra::detail::UpdateRateValueStatus::inconsistent_catalog:
      throw RTIinternalError(
          L"The embedded federation could not resolve the current FDD update-rate table.");
  }
  throw RTIinternalError(L"Umbra encountered an unknown attribute update-rate query outcome.");
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Update Rate Value For Attribute", exception);
    throw;
  }
}

void UmbraRtiAmbassador::publishObjectClassAttributes(
    ObjectClassHandle const& objectClass,
    AttributeHandleSet const& attributes) {
  auto instrumentationScope = beginRtiCall("publishObjectClassAttributes");
  try {
  std::wstring federationName;
  std::vector<umbra::detail::DeclarationAdvisory> declarationAdvisories;
  std::vector<umbra::detail::AttributeOwnershipAssumptionRecipient>
      newlyEligibleAssumptions;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Publish Object Class Attributes");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Publish Object Class Attributes requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const objectClassHandle = objectClassHandleValue(objectClass);
    if (!objectClassHandle) {
      throw ObjectClassNotDefined(
          L"Publish Object Class Attributes requires a defined ObjectClassHandle.");
    }
    auto const attributeHandles = attributeHandleValues(attributes);
    if (!attributeHandles) {
      throw AttributeNotDefined(
          L"Publish Object Class Attributes requires defined AttributeHandle values.");
    }
    auto const result = registry.setObjectClassAttributePublication(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectClassHandle,
        *attributeHandles,
        true);
    if (result != umbra::detail::ObjectClassAttributeDeclarationStatus::applied) {
      throwObjectClassAttributeDeclarationFailure(result);
    }
    // The accepted §5.2 publication transition is the service-report boundary.
    // Keep it before separately queued declaration advisories and ownership
    // assumptions, so the selected file records the successful void invocation
    // rather than any later callback work it can make eligible.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"PublishObjectClassAttributes",
        umbra::detail::MomServiceType::declaration_management,
        {{umbra::detail::MomArgumentType::object_class_handle,
          L"Object class designator",
          umbra::detail::formatMomObjectClassHandle(objectClass)},
         {umbra::detail::MomArgumentType::attribute_handle_set,
          L"Set of attribute designators",
          umbra::detail::formatMomAttributeHandleSet(attributes)}});
    federationName = *joinedFederationName_;
    declarationAdvisories = registry.planDeclarationAdvisories(federationName);
    newlyEligibleAssumptions = registry.planAttributeOwnershipAssumptionsForFederate(
        federationName,
        *joinedFederateId_);
  }
  queueDeclarationAdvisories(std::move(declarationAdvisories));
  queueAttributeOwnershipAssumptionRecipients(
      std::move(newlyEligibleAssumptions),
      federationName,
      VariableLengthData());
  } catch (Exception const& exception) {
    emitExceptionReport(L"Publish Object Class Attributes", exception);
    throw;
  }
}

void UmbraRtiAmbassador::unpublishObjectClass(
    ObjectClassHandle const& objectClass) {
  auto instrumentationScope = beginRtiCall("unpublishObjectClass");
  try {
  std::vector<umbra::detail::DeclarationAdvisory> declarationAdvisories;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Unpublish Object Class");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Unpublish Object Class requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const objectClassHandle = objectClassHandleValue(objectClass);
    if (!objectClassHandle) {
      throw ObjectClassNotDefined(
          L"Unpublish Object Class requires a defined ObjectClassHandle.");
    }

    // The registry exposes the complete current publication set, including the
    // implicit HLAprivilegeToDeleteObject publication for the active epoch.
    // Reuse the attribute-set service so its pending-acquisition and ownership
    // boundaries remain identical for the whole-class and subset forms.
    auto const publishedAttributes = registry.publishedObjectClassAttributeHandles(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectClassHandle);
    if (!publishedAttributes) {
      throw ObjectClassNotDefined(
          L"The supplied ObjectClassHandle is not defined in this federation execution.");
    }

    auto const result = registry.setObjectClassAttributePublication(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectClassHandle,
        *publishedAttributes,
        false);
    if (result != umbra::detail::ObjectClassAttributeDeclarationStatus::applied) {
      throwObjectClassAttributeDeclarationFailure(result);
    }
    declarationAdvisories = registry.planDeclarationAdvisories(*joinedFederationName_);
  }
  queueDeclarationAdvisories(std::move(declarationAdvisories));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Unpublish Object Class", exception);
    throw;
  }
}

void UmbraRtiAmbassador::unpublishObjectClassAttributes(
    ObjectClassHandle const& objectClass,
    AttributeHandleSet const& attributes) {
  auto instrumentationScope = beginRtiCall("unpublishObjectClassAttributes");
  try {
  std::vector<umbra::detail::DeclarationAdvisory> declarationAdvisories;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Unpublish Object Class Attributes");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Unpublish Object Class Attributes requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const objectClassHandle = objectClassHandleValue(objectClass);
    if (!objectClassHandle) {
      throw ObjectClassNotDefined(
          L"Unpublish Object Class Attributes requires a defined ObjectClassHandle.");
    }
    auto const attributeHandles = attributeHandleValues(attributes);
    if (!attributeHandles) {
      throw AttributeNotDefined(
          L"Unpublish Object Class Attributes requires defined AttributeHandle values.");
    }
    auto const result = registry.setObjectClassAttributePublication(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectClassHandle,
        *attributeHandles,
        false);
    if (result != umbra::detail::ObjectClassAttributeDeclarationStatus::applied) {
      throwObjectClassAttributeDeclarationFailure(result);
    }
    // The §5.3 unpublication and its synchronous ownership cleanup have
    // succeeded before this point. Keep the successful-void record ahead of
    // separately queued declaration advisories so the selected report file
    // reflects the invoking service, not downstream callback delivery.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"UnpublishObjectClassAttributes",
        umbra::detail::MomServiceType::declaration_management,
        {{umbra::detail::MomArgumentType::object_class_handle,
          L"Object class designator",
          umbra::detail::formatMomObjectClassHandle(objectClass)},
         {umbra::detail::MomArgumentType::attribute_handle_set,
          L"Optional set of attribute designators",
          umbra::detail::formatMomAttributeHandleSet(attributes)}});
    declarationAdvisories = registry.planDeclarationAdvisories(*joinedFederationName_);
  }
  queueDeclarationAdvisories(std::move(declarationAdvisories));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Unpublish Object Class Attributes", exception);
    throw;
  }
}

void UmbraRtiAmbassador::subscribeObjectClassAttributes(
    ObjectClassHandle const& objectClass,
    AttributeHandleSet const& attributes,
    bool active,
    std::wstring const& updateRateDesignator) {
  auto instrumentationScope = beginRtiCall("subscribeObjectClassAttributes");
  try {
  std::wstring federationName;
  std::vector<umbra::detail::DeclarationAdvisory> declarationAdvisories;
  std::vector<umbra::detail::ObjectInstanceDiscoveryRecipient> discoveries;
  std::vector<umbra::detail::ObjectInstanceDiscoveryRecipient> momDiscoveries;
  std::vector<umbra::detail::ObjectInstanceScopeChangeRecipient> changes;
  std::vector<umbra::detail::AttributeRelevanceAdvisoryRecipient>
      attributeRelevanceAdvisories;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Subscribe Object Class Attributes");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Subscribe Object Class Attributes requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const objectClassHandle = objectClassHandleValue(objectClass);
    if (!objectClassHandle) {
      throw ObjectClassNotDefined(
          L"Subscribe Object Class Attributes requires a defined ObjectClassHandle.");
    }
    auto const attributeHandles = attributeHandleValues(attributes);
    if (!attributeHandles) {
      throw AttributeNotDefined(
          L"Subscribe Object Class Attributes requires defined AttributeHandle values.");
    }
    auto const encodedUpdateRateDesignator =
        umbra::detail::utf8FromWide(updateRateDesignator);
    if (!encodedUpdateRateDesignator) {
      throw InvalidUpdateRateDesignator(
          L"Subscribe Object Class Attributes received an update-rate designator that is not valid UTF-8 text.");
    }
    auto result = registry.setObjectClassAttributeSubscriptionWithScopeChanges(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectClassHandle,
        *attributeHandles,
        active,
        *encodedUpdateRateDesignator);
    if (result.status != umbra::detail::ObjectClassAttributeDeclarationStatus::applied) {
      throwObjectClassAttributeDeclarationFailure(result.status);
    }

    // The §5.8 subscription has succeeded before this point.  The public C++
    // `active` argument has the inverse meaning of the service narrative's
    // Optional passive subscription indicator.  An empty update-rate
    // designator selects the default rate and therefore occupies its required
    // Table 5 argument position as Null rather than as an empty String.
    // Keep this successful-void record ahead of separately queued declaration,
    // scope, relevance, and discovery callbacks.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"SubscribeObjectClassAttributes",
        umbra::detail::MomServiceType::declaration_management,
        {{umbra::detail::MomArgumentType::object_class_handle,
          L"Object class designator",
          umbra::detail::formatMomObjectClassHandle(objectClass)},
         {umbra::detail::MomArgumentType::attribute_handle_set,
          L"Set of attribute designators",
          umbra::detail::formatMomAttributeHandleSet(attributes)},
         {umbra::detail::MomArgumentType::boolean,
          L"Optional passive subscription indicator",
          umbra::detail::formatMomBoolean(!active)},
         {updateRateDesignator.empty() ? umbra::detail::MomArgumentType::null_value
                                       : umbra::detail::MomArgumentType::string,
          L"Optional update rate designator",
          updateRateDesignator.empty() ? umbra::detail::formatMomNull()
                                       : umbra::detail::formatMomString(updateRateDesignator)}});

    federationName = *joinedFederationName_;
    changes = std::move(result.recipients);
    attributeRelevanceAdvisories = std::move(result.attributeRelevanceAdvisories);
    declarationAdvisories = registry.planDeclarationAdvisories(federationName);
    discoveries = registry.planObjectInstanceDiscoveriesForFederate(
        federationName,
        *joinedFederateId_);
    momDiscoveries = registry.planJoinedFederateMomObjectDiscoveriesForFederate(
        federationName,
        *joinedFederateId_);
  }
  queueDeclarationAdvisories(std::move(declarationAdvisories));
  queueObjectInstanceScopeChanges(std::move(changes), federationName);
  queueAttributeRelevanceAdvisories(std::move(attributeRelevanceAdvisories), federationName);
  queueObjectInstanceDiscoveries(std::move(discoveries), federationName);
  queueObjectInstanceDiscoveries(std::move(momDiscoveries), federationName);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Subscribe Object Class Attributes", exception);
    throw;
  }
}

void UmbraRtiAmbassador::unsubscribeObjectClass(
    ObjectClassHandle const& objectClass) {
  auto instrumentationScope = beginRtiCall("unsubscribeObjectClass");
  try {
  std::wstring federationName;
  std::vector<umbra::detail::DeclarationAdvisory> declarationAdvisories;
  std::vector<umbra::detail::ObjectInstanceScopeChangeRecipient> changes;
  std::vector<umbra::detail::AttributeRelevanceAdvisoryRecipient>
      attributeRelevanceAdvisories;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Unsubscribe Object Class");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Unsubscribe Object Class requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const objectClassHandle = objectClassHandleValue(objectClass);
    if (!objectClassHandle) {
      throw ObjectClassNotDefined(
          L"Unsubscribe Object Class requires a defined ObjectClassHandle.");
    }
    auto const declaration = registry.objectClassAttributeDeclarationFor(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectClassHandle);
    if (!declaration) {
      throw ObjectClassNotDefined(
          L"The supplied ObjectClassHandle is not defined in this federation execution.");
    }

    AttributeHandleSet ordinaryAttributes;
    for (auto const& [attributeHandle, active] : declaration->subscribedAttributes) {
      static_cast<void>(active);
      ordinaryAttributes.insert(makeAttributeHandle(attributeHandle));
    }
    auto const attributeHandles = attributeHandleValues(ordinaryAttributes);
    if (!attributeHandles) {
      throw AttributeNotDefined(
          L"Unsubscribe Object Class could not decode its subscribed attributes.");
    }
    auto result = registry.setObjectClassAttributeSubscriptionWithScopeChanges(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectClassHandle,
        *attributeHandles,
        std::nullopt,
        std::string{});
    if (result.status != umbra::detail::ObjectClassAttributeDeclarationStatus::applied) {
      throwObjectClassAttributeDeclarationFailure(result.status);
    }
    // The C++ whole-class overload represents §5.9 without its optional set
    // of attribute designators.  Table 5 keeps that supplied-argument slot,
    // and §11.5.1 requires an unused optional argument to be reported as
    // Null.  Write the successful-void record before separately queued
    // declaration, scope, and relevance callbacks.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"UnsubscribeObjectClassAttributes",
        umbra::detail::MomServiceType::declaration_management,
        {{umbra::detail::MomArgumentType::object_class_handle,
          L"Object class designator",
          umbra::detail::formatMomObjectClassHandle(objectClass)},
         {umbra::detail::MomArgumentType::null_value,
          L"Optional set of attribute designators",
          umbra::detail::formatMomNull()}});
    federationName = *joinedFederationName_;
    changes = std::move(result.recipients);
    attributeRelevanceAdvisories = std::move(result.attributeRelevanceAdvisories);
    declarationAdvisories = registry.planDeclarationAdvisories(federationName);
  }
  queueDeclarationAdvisories(std::move(declarationAdvisories));
  queueObjectInstanceScopeChanges(std::move(changes), federationName);
  queueAttributeRelevanceAdvisories(std::move(attributeRelevanceAdvisories), federationName);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Unsubscribe Object Class", exception);
    throw;
  }
}

void UmbraRtiAmbassador::unsubscribeObjectClassAttributes(
    ObjectClassHandle const& objectClass,
    AttributeHandleSet const& attributes) {
  auto instrumentationScope = beginRtiCall("unsubscribeObjectClassAttributes");
  try {
  std::wstring federationName;
  std::vector<umbra::detail::DeclarationAdvisory> declarationAdvisories;
  std::vector<umbra::detail::ObjectInstanceScopeChangeRecipient> changes;
  std::vector<umbra::detail::AttributeRelevanceAdvisoryRecipient>
      attributeRelevanceAdvisories;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Unsubscribe Object Class Attributes");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Unsubscribe Object Class Attributes requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const objectClassHandle = objectClassHandleValue(objectClass);
    if (!objectClassHandle) {
      throw ObjectClassNotDefined(
          L"Unsubscribe Object Class Attributes requires a defined ObjectClassHandle.");
    }
    auto const attributeHandles = attributeHandleValues(attributes);
    if (!attributeHandles) {
      throw AttributeNotDefined(
          L"Unsubscribe Object Class Attributes requires defined AttributeHandle values.");
    }
    auto result = registry.setObjectClassAttributeSubscriptionWithScopeChanges(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectClassHandle,
        *attributeHandles,
        std::nullopt,
        std::string{});
    if (result.status != umbra::detail::ObjectClassAttributeDeclarationStatus::applied) {
      throwObjectClassAttributeDeclarationFailure(result.status);
    }
    // The accepted §5.9 subset transition is the service-report boundary.
    // Unlike the whole-class overload, this entry point supplies the optional
    // set of attribute designators, including a supplied-empty set.  Keep the
    // successful-void record ahead of separately queued declaration, scope,
    // and relevance callbacks.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"UnsubscribeObjectClassAttributes",
        umbra::detail::MomServiceType::declaration_management,
        {{umbra::detail::MomArgumentType::object_class_handle,
          L"Object class designator",
          umbra::detail::formatMomObjectClassHandle(objectClass)},
         {umbra::detail::MomArgumentType::attribute_handle_set,
          L"Optional set of attribute designators",
          umbra::detail::formatMomAttributeHandleSet(attributes)}});
    federationName = *joinedFederationName_;
    changes = std::move(result.recipients);
    attributeRelevanceAdvisories = std::move(result.attributeRelevanceAdvisories);
    declarationAdvisories = registry.planDeclarationAdvisories(federationName);
  }
  queueDeclarationAdvisories(std::move(declarationAdvisories));
  queueObjectInstanceScopeChanges(std::move(changes), federationName);
  queueAttributeRelevanceAdvisories(std::move(attributeRelevanceAdvisories), federationName);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Unsubscribe Object Class Attributes", exception);
    throw;
  }
}

void UmbraRtiAmbassador::reserveObjectInstanceName(
    std::wstring const& objectInstanceName) {
  auto instrumentationScope = beginRtiCall("reserveObjectInstanceName");
  try {
  std::wstring federationName;
  std::uint64_t federateId = 0;
  umbra::detail::ObjectInstanceNameReservationResult result;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Reserve Object Instance Name");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Reserve Object Instance Name requires membership in a federation execution.");
    }
    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    federationName = *joinedFederationName_;
    federateId = *joinedFederateId_;
    result = registry.reserveObjectInstanceName(
        federationName,
        federateId,
        objectInstanceName);
    if (result.status !=
        umbra::detail::ObjectInstanceNameReservationStatus::applied) {
      throwObjectInstanceNameReservationFailure(
          result.status,
          L"Reserve Object Instance Name");
    }
    // §6.2 has no returned arguments.  The accepted reservation is the
    // service boundary; write its source-backed successful-void record before
    // the asynchronous Object Instance Name Reserved callback is queued.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"ReserveObjectInstanceName",
        umbra::detail::MomServiceType::object_management,
        {{umbra::detail::MomArgumentType::string,
          L"Name",
          umbra::detail::formatMomString(objectInstanceName)}});
  }
  queueObjectInstanceNameReservation(
      std::move(result.callbackRoute),
      std::move(federationName),
      federateId,
      result.succeeded,
      std::move(result.objectInstanceName));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Reserve Object Instance Name", exception);
    throw;
  }
}

void UmbraRtiAmbassador::releaseObjectInstanceName(
    std::wstring const& objectInstanceName) {
  auto instrumentationScope = beginRtiCall("releaseObjectInstanceName");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  requireFederationServiceOperationAvailable(L"Release Object Instance Name");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Release Object Instance Name requires membership in a federation execution.");
  }
  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  auto const status = registry.releaseObjectInstanceName(
      *joinedFederationName_,
      *joinedFederateId_,
      objectInstanceName);
  if (status != umbra::detail::ObjectInstanceNameReservationStatus::applied) {
    throwObjectInstanceNameReservationFailure(
        status,
        L"Release Object Instance Name");
  }
  // §6.4 is a successful-void service with one String/Name argument.  The
  // file record is durable before the public call returns; a rejected release
  // never reaches this boundary and therefore appends nothing.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"ReleaseObjectInstanceName",
      umbra::detail::MomServiceType::object_management,
      {{umbra::detail::MomArgumentType::string,
        L"Name",
        umbra::detail::formatMomString(objectInstanceName)}});
  } catch (Exception const& exception) {
    emitExceptionReport(L"Release Object Instance Name", exception);
    throw;
  }
}

void UmbraRtiAmbassador::reserveMultipleObjectInstanceNames(
    std::set<std::wstring> const& objectInstanceNames) {
  auto instrumentationScope = beginRtiCall("reserveMultipleObjectInstanceNames");
  try {
  std::wstring federationName;
  std::uint64_t federateId = 0;
  umbra::detail::MultipleObjectInstanceNameReservationResult result;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Reserve Multiple Object Instance Names");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Reserve Multiple Object Instance Names requires membership in a federation execution.");
    }
    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    federationName = *joinedFederationName_;
    federateId = *joinedFederateId_;
    result = registry.reserveMultipleObjectInstanceNames(
        federationName,
        federateId,
        objectInstanceNames);
    if (result.status !=
        umbra::detail::ObjectInstanceNameReservationStatus::applied) {
      throwObjectInstanceNameReservationFailure(
          result.status,
          L"Reserve Multiple Object Instance Names");
    }
    // §6.5 supplies one non-empty StringSet and returns None.  The accepted
    // request is the report boundary; its later per-name outcome callback is
    // a composite result and remains outside the file-report slice (RL-042).
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"ReserveMultipleObjectInstanceNames",
        umbra::detail::MomServiceType::object_management,
        {{umbra::detail::MomArgumentType::string_set,
          L"Name Set",
          umbra::detail::formatMomStringSet(objectInstanceNames)}});
  }

  if (!result.succeededNames.empty()) {
    queueMultipleObjectInstanceNameReservation(
        result.callbackRoute,
        federationName,
        federateId,
        true,
        result.succeededNames);
  }
  if (!result.failedNames.empty()) {
    queueMultipleObjectInstanceNameReservation(
        std::move(result.callbackRoute),
        std::move(federationName),
        federateId,
        false,
        std::move(result.failedNames));
  }
  } catch (Exception const& exception) {
    emitExceptionReport(L"Reserve Multiple Object Instance Names", exception);
    throw;
  }
}

void UmbraRtiAmbassador::releaseMultipleObjectInstanceNames(
    std::set<std::wstring> const& objectInstanceNames) {
  auto instrumentationScope = beginRtiCall("releaseMultipleObjectInstanceNames");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  requireFederationServiceOperationAvailable(L"Release Multiple Object Instance Names");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Release Multiple Object Instance Names requires membership in a federation execution.");
  }
  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  auto const status = registry.releaseMultipleObjectInstanceNames(
      *joinedFederationName_,
      *joinedFederateId_,
      objectInstanceNames);
  if (status != umbra::detail::ObjectInstanceNameReservationStatus::applied) {
    throwObjectInstanceNameReservationFailure(
        status,
        L"Release Multiple Object Instance Names");
  }
  // §6.7 supplies a StringSet and returns None.  Atomic failure is raised
  // above, so no file record is appended for an unreserved name.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"ReleaseMultipleObjectInstanceNames",
      umbra::detail::MomServiceType::object_management,
      {{umbra::detail::MomArgumentType::string_set,
        L"Name set",
        umbra::detail::formatMomStringSet(objectInstanceNames)}});
  } catch (Exception const& exception) {
    emitExceptionReport(L"Release Multiple Object Instance Names", exception);
    throw;
  }
}

ObjectInstanceHandle UmbraRtiAmbassador::registerObjectInstanceWithRegions(
    ObjectClassHandle const& objectClass,
    AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions) {
  auto instrumentationScope = beginRtiCall("registerObjectInstanceWithRegions");
  try {
  std::wstring federationName;
  std::vector<umbra::detail::ObjectInstanceDiscoveryRecipient> discoveries;
  std::uint64_t objectInstanceHandle = 0;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Register Object Instance With Regions");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Register Object Instance With Regions requires membership in a federation execution.");
    }
    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const objectClassHandle = objectClassHandleValue(objectClass);
    if (!objectClassHandle) {
      throw ObjectClassNotDefined(
          L"Register Object Instance With Regions requires a defined ObjectClassHandle.");
    }
    auto pairValues = attributeRegionPairValues(attributesAndRegions);
    if (pairValues.invalidAttributeHandle) {
      throw AttributeNotDefined(
          L"Register Object Instance With Regions requires defined AttributeHandle values.");
    }
    if (pairValues.invalidRegionHandle) {
      throw InvalidRegion(
          L"Register Object Instance With Regions requires defined RegionHandle values.");
    }
    auto const registration = registry.registerObjectInstance(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectClassHandle,
        &pairValues.values);
    if (registration.status != umbra::detail::ObjectInstanceRegistrationStatus::applied) {
      throwObjectInstanceRegistrationFailure(registration.status);
    }
    federationName = *joinedFederationName_;
    objectInstanceHandle = registration.objectInstanceHandle;
    discoveries = registry.planObjectInstanceDiscoveriesForInstance(
        federationName,
        objectInstanceHandle);
  }
  queueObjectInstanceDiscoveries(std::move(discoveries), federationName);
  return makeObjectInstanceHandle(objectInstanceHandle);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Register Object Instance With Regions", exception);
    throw;
  }
}

ObjectInstanceHandle UmbraRtiAmbassador::registerObjectInstanceWithRegions(
    ObjectClassHandle const& objectClass,
    AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions,
    std::wstring const& objectInstanceName) {
  auto instrumentationScope = beginRtiCall("registerObjectInstanceWithRegions");
  try {
  std::wstring federationName;
  std::vector<umbra::detail::ObjectInstanceDiscoveryRecipient> discoveries;
  std::uint64_t objectInstanceHandle = 0;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Register Object Instance With Regions");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Register Object Instance With Regions requires membership in a federation execution.");
    }
    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const objectClassHandle = objectClassHandleValue(objectClass);
    if (!objectClassHandle) {
      throw ObjectClassNotDefined(
          L"Register Object Instance With Regions requires a defined ObjectClassHandle.");
    }
    auto pairValues = attributeRegionPairValues(attributesAndRegions);
    if (pairValues.invalidAttributeHandle) {
      throw AttributeNotDefined(
          L"Register Object Instance With Regions requires defined AttributeHandle values.");
    }
    if (pairValues.invalidRegionHandle) {
      throw InvalidRegion(
          L"Register Object Instance With Regions requires defined RegionHandle values.");
    }
    auto const registration = registry.registerObjectInstance(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectClassHandle,
        &pairValues.values,
        &objectInstanceName);
    if (registration.status != umbra::detail::ObjectInstanceRegistrationStatus::applied) {
      throwObjectInstanceRegistrationFailure(registration.status);
    }
    federationName = *joinedFederationName_;
    objectInstanceHandle = registration.objectInstanceHandle;
    discoveries = registry.planObjectInstanceDiscoveriesForInstance(
        federationName,
        objectInstanceHandle);
  }
  queueObjectInstanceDiscoveries(std::move(discoveries), federationName);
  return makeObjectInstanceHandle(objectInstanceHandle);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Register Object Instance With Regions", exception);
    throw;
  }
}

void UmbraRtiAmbassador::associateRegionsForUpdates(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions) {
  auto instrumentationScope = beginRtiCall("associateRegionsForUpdates");
  try {
  std::wstring federationName;
  std::vector<umbra::detail::ObjectInstanceScopeChangeRecipient> changes;
  std::vector<umbra::detail::AttributeRelevanceAdvisoryRecipient>
      attributeRelevanceAdvisories;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Associate Regions For Updates");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Associate Regions For Updates requires membership in a federation execution.");
    }
    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const objectInstanceHandle = objectInstanceHandleValue(objectInstance);
    if (!objectInstanceHandle) {
      throw ObjectInstanceNotKnown(
          L"Associate Regions For Updates requires a known ObjectInstanceHandle.");
    }
    auto pairValues = attributeRegionPairValues(attributesAndRegions);
    if (pairValues.invalidAttributeHandle) {
      throw AttributeNotDefined(
          L"Associate Regions For Updates requires defined AttributeHandle values.");
    }
    if (pairValues.invalidRegionHandle) {
      throw InvalidRegion(
          L"Associate Regions For Updates requires defined RegionHandle values.");
    }
    auto result = registry.associateRegionsForUpdatesWithScopeChanges(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectInstanceHandle,
        pairValues.values);
    if (result.status != umbra::detail::ObjectInstanceRegionAssociationStatus::applied) {
      throwObjectInstanceRegionAssociationFailure(result.status);
    }
    // §9.6 returns None and supplies an ObjectInstanceHandle plus the
    // AttributeSetRegionSetPairList type-4 collection.  Record the accepted
    // transition before any scope/relevance callbacks are queued.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"AssociateRegionsForUpdates",
        umbra::detail::MomServiceType::data_distribution_management,
        {{umbra::detail::MomArgumentType::object_instance_handle,
          L"Object instance designator",
          umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
         {umbra::detail::MomArgumentType::attribute_set_region_set_pair_list,
          L"Collection of attribute designator set and region designator set pairs",
          umbra::detail::formatMomAttributeSetRegionSetPairList(attributesAndRegions)}});
    federationName = *joinedFederationName_;
    changes = std::move(result.recipients);
    attributeRelevanceAdvisories = std::move(result.attributeRelevanceAdvisories);
  }
  queueObjectInstanceScopeChanges(std::move(changes), federationName);
  queueAttributeRelevanceAdvisories(std::move(attributeRelevanceAdvisories), federationName);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Associate Regions For Updates", exception);
    throw;
  }
}

void UmbraRtiAmbassador::unassociateRegionsForUpdates(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions) {
  auto instrumentationScope = beginRtiCall("unassociateRegionsForUpdates");
  try {
  std::wstring federationName;
  std::vector<umbra::detail::ObjectInstanceScopeChangeRecipient> changes;
  std::vector<umbra::detail::AttributeRelevanceAdvisoryRecipient>
      attributeRelevanceAdvisories;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Unassociate Regions For Updates");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Unassociate Regions For Updates requires membership in a federation execution.");
    }
    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const objectInstanceHandle = objectInstanceHandleValue(objectInstance);
    if (!objectInstanceHandle) {
      throw ObjectInstanceNotKnown(
          L"Unassociate Regions For Updates requires a known ObjectInstanceHandle.");
    }
    auto pairValues = attributeRegionPairValues(attributesAndRegions);
    if (pairValues.invalidAttributeHandle) {
      throw AttributeNotDefined(
          L"Unassociate Regions For Updates requires defined AttributeHandle values.");
    }
    if (pairValues.invalidRegionHandle) {
      throw InvalidRegion(
          L"Unassociate Regions For Updates requires defined RegionHandle values.");
    }
    auto result = registry.unassociateRegionsForUpdatesWithScopeChanges(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectInstanceHandle,
        pairValues.values);
    if (result.status != umbra::detail::ObjectInstanceRegionAssociationStatus::applied) {
      throwObjectInstanceRegionAssociationFailure(result.status);
    }
    // §9.7 has the same supplied shape and successful-void return.  Atomic
    // validation failures stop above, so they cannot consume a report serial.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"UnassociateRegionsForUpdates",
        umbra::detail::MomServiceType::data_distribution_management,
        {{umbra::detail::MomArgumentType::object_instance_handle,
          L"Object instance designator",
          umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
         {umbra::detail::MomArgumentType::attribute_set_region_set_pair_list,
          L"Collection of attribute designator set and region designator set pairs",
          umbra::detail::formatMomAttributeSetRegionSetPairList(attributesAndRegions)}});
    federationName = *joinedFederationName_;
    changes = std::move(result.recipients);
    attributeRelevanceAdvisories = std::move(result.attributeRelevanceAdvisories);
  }
  queueObjectInstanceScopeChanges(std::move(changes), federationName);
  queueAttributeRelevanceAdvisories(std::move(attributeRelevanceAdvisories), federationName);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Unassociate Regions For Updates", exception);
    throw;
  }
}

void UmbraRtiAmbassador::subscribeObjectClassAttributesWithRegions(
    ObjectClassHandle const& objectClass,
    AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions,
    bool active,
    std::wstring const& updateRateDesignator) {
  auto instrumentationScope = beginRtiCall("subscribeObjectClassAttributesWithRegions");
  try {
  std::wstring federationName;
  std::vector<umbra::detail::ObjectInstanceDiscoveryRecipient> discoveries;
  std::vector<umbra::detail::ObjectInstanceDiscoveryRecipient> momDiscoveries;
  std::vector<umbra::detail::ObjectInstanceScopeChangeRecipient> changes;
  std::vector<umbra::detail::AttributeRelevanceAdvisoryRecipient>
      attributeRelevanceAdvisories;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Subscribe Object Class Attributes With Regions");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Subscribe Object Class Attributes With Regions requires membership in a federation execution.");
    }
    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const objectClassHandle = objectClassHandleValue(objectClass);
    if (!objectClassHandle) {
      throw ObjectClassNotDefined(
          L"Subscribe Object Class Attributes With Regions requires a defined ObjectClassHandle.");
    }
    auto pairValues = attributeRegionPairValues(attributesAndRegions);
    if (pairValues.invalidAttributeHandle) {
      throw AttributeNotDefined(
          L"Subscribe Object Class Attributes With Regions requires defined AttributeHandle values.");
    }
    if (pairValues.invalidRegionHandle) {
      throw InvalidRegion(
          L"Subscribe Object Class Attributes With Regions requires defined RegionHandle values.");
    }
    auto const encodedUpdateRateDesignator =
        umbra::detail::utf8FromWide(updateRateDesignator);
    if (!encodedUpdateRateDesignator) {
      throw InvalidUpdateRateDesignator(
          L"Subscribe Object Class Attributes With Regions received an update-rate designator that is not valid UTF-8 text.");
    }
    auto result = registry.setObjectClassAttributeRegionalSubscriptionWithScopeChanges(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectClassHandle,
        pairValues.values,
        active,
        *encodedUpdateRateDesignator);
    if (result.status !=
        umbra::detail::RegionalObjectClassAttributeDeclarationStatus::applied) {
      throwRegionalObjectClassAttributeDeclarationFailure(result.status);
    }
    // §9.8 is a successful-void DDM service.  Keep the report boundary after
    // the registry has accepted the regional subscription, but before any
    // separately queued scope, relevance, or discovery callbacks.  The C++
    // `active` argument is the inverse of the standard's optional passive
    // subscription indicator; an empty update-rate designator occupies the
    // required Table 5 argument position as Null.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"SubscribeObjectClassAttributesWithRegions",
        umbra::detail::MomServiceType::data_distribution_management,
        {{umbra::detail::MomArgumentType::object_class_handle,
          L"Object class designator",
          umbra::detail::formatMomObjectClassHandle(objectClass)},
         {umbra::detail::MomArgumentType::attribute_set_region_set_pair_list,
          L"Collection of attribute designator set and region designator set pairs",
          umbra::detail::formatMomAttributeSetRegionSetPairList(attributesAndRegions)},
         {umbra::detail::MomArgumentType::boolean,
          L"Optional passive subscription indicator",
          umbra::detail::formatMomBoolean(!active)},
         {updateRateDesignator.empty() ? umbra::detail::MomArgumentType::null_value
                                       : umbra::detail::MomArgumentType::string,
          L"Optional update rate designator",
          updateRateDesignator.empty() ? umbra::detail::formatMomNull()
                                       : umbra::detail::formatMomString(updateRateDesignator)}});
    federationName = *joinedFederationName_;
    changes = std::move(result.recipients);
    attributeRelevanceAdvisories = std::move(result.attributeRelevanceAdvisories);
    discoveries = registry.planObjectInstanceDiscoveriesForFederate(
        federationName,
        *joinedFederateId_);
    // Regional declarations are evaluated against the RTI-owned
    // HLAfederate point as well as ordinary object instances.  The registry
    // planner performs the callback-time recheck, so this reservation is
    // safe for both evoked and immediate callback models.
    momDiscoveries = registry.planJoinedFederateMomObjectDiscoveriesForFederate(
        federationName,
        *joinedFederateId_);
  }
  queueObjectInstanceScopeChanges(std::move(changes), federationName);
  queueAttributeRelevanceAdvisories(std::move(attributeRelevanceAdvisories), federationName);
  queueObjectInstanceDiscoveries(std::move(discoveries), federationName);
  queueObjectInstanceDiscoveries(std::move(momDiscoveries), federationName);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Subscribe Object Class Attributes With Regions", exception);
    throw;
  }
}

void UmbraRtiAmbassador::unsubscribeObjectClassAttributesWithRegions(
    ObjectClassHandle const& objectClass,
    AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions) {
  auto instrumentationScope = beginRtiCall("unsubscribeObjectClassAttributesWithRegions");
  try {
  std::wstring federationName;
  std::vector<umbra::detail::ObjectInstanceScopeChangeRecipient> changes;
  std::vector<umbra::detail::AttributeRelevanceAdvisoryRecipient>
      attributeRelevanceAdvisories;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Unsubscribe Object Class Attributes With Regions");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Unsubscribe Object Class Attributes With Regions requires membership in a federation execution.");
    }
    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const objectClassHandle = objectClassHandleValue(objectClass);
    if (!objectClassHandle) {
      throw ObjectClassNotDefined(
          L"Unsubscribe Object Class Attributes With Regions requires a defined ObjectClassHandle.");
    }
    auto pairValues = attributeRegionPairValues(attributesAndRegions);
    if (pairValues.invalidAttributeHandle) {
      throw AttributeNotDefined(
          L"Unsubscribe Object Class Attributes With Regions requires defined AttributeHandle values.");
    }
    if (pairValues.invalidRegionHandle) {
      throw InvalidRegion(
          L"Unsubscribe Object Class Attributes With Regions requires defined RegionHandle values.");
    }
    auto result = registry.removeObjectClassAttributeRegionalSubscriptionWithScopeChanges(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectClassHandle,
        pairValues.values);
    if (result.status !=
        umbra::detail::RegionalObjectClassAttributeDeclarationStatus::applied) {
      throwRegionalObjectClassAttributeDeclarationFailure(result.status);
    }
    // §9.9 has the object-class and AttributeRegionAssociationList supplied
    // shape and a successful-void return.  Validation failures stop above,
    // so they cannot consume a report serial; an accepted empty pair remains
    // a real invocation and is reported with the same type-4 representation.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"UnsubscribeObjectClassAttributesWithRegions",
        umbra::detail::MomServiceType::data_distribution_management,
        {{umbra::detail::MomArgumentType::object_class_handle,
          L"Object class designator",
          umbra::detail::formatMomObjectClassHandle(objectClass)},
         {umbra::detail::MomArgumentType::attribute_set_region_set_pair_list,
          L"Collection of attribute designator set and region designator set pairs",
          umbra::detail::formatMomAttributeSetRegionSetPairList(attributesAndRegions)}});
    federationName = *joinedFederationName_;
    changes = std::move(result.recipients);
    attributeRelevanceAdvisories = std::move(result.attributeRelevanceAdvisories);
  }
  queueObjectInstanceScopeChanges(std::move(changes), federationName);
  queueAttributeRelevanceAdvisories(std::move(attributeRelevanceAdvisories), federationName);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Unsubscribe Object Class Attributes With Regions", exception);
    throw;
  }
}

ObjectInstanceHandle UmbraRtiAmbassador::registerObjectInstance(
    ObjectClassHandle const& objectClass) {
  auto instrumentationScope = beginRtiCall("registerObjectInstance");
  try {
  std::wstring federationName;
  std::vector<umbra::detail::ObjectInstanceDiscoveryRecipient> discoveries;
  std::uint64_t objectInstanceHandle = 0;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Register Object Instance");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Register Object Instance requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const objectClassHandle = objectClassHandleValue(objectClass);
    if (!objectClassHandle) {
      throw ObjectClassNotDefined(
          L"Register Object Instance requires a defined ObjectClassHandle.");
    }

    auto const registration = registry.registerObjectInstance(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectClassHandle);
    if (registration.status != umbra::detail::ObjectInstanceRegistrationStatus::applied) {
      throwObjectInstanceRegistrationFailure(registration.status);
    }

    federationName = *joinedFederationName_;
    objectInstanceHandle = registration.objectInstanceHandle;
    discoveries = registry.planObjectInstanceDiscoveriesForInstance(
        federationName,
        objectInstanceHandle);
  }
  queueObjectInstanceDiscoveries(std::move(discoveries), federationName);
  return makeObjectInstanceHandle(objectInstanceHandle);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Register Object Instance", exception);
    throw;
  }
}

ObjectInstanceHandle UmbraRtiAmbassador::registerObjectInstance(
    ObjectClassHandle const& objectClass,
    std::wstring const& objectInstanceName) {
  auto instrumentationScope = beginRtiCall("registerObjectInstance");
  try {
  std::wstring federationName;
  std::vector<umbra::detail::ObjectInstanceDiscoveryRecipient> discoveries;
  std::uint64_t objectInstanceHandle = 0;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Register Object Instance");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Register Object Instance requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const objectClassHandle = objectClassHandleValue(objectClass);
    if (!objectClassHandle) {
      throw ObjectClassNotDefined(
          L"Register Object Instance requires a defined ObjectClassHandle.");
    }

    auto const registration = registry.registerObjectInstance(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectClassHandle,
        nullptr,
        &objectInstanceName);
    if (registration.status != umbra::detail::ObjectInstanceRegistrationStatus::applied) {
      throwObjectInstanceRegistrationFailure(registration.status);
    }

    federationName = *joinedFederationName_;
    objectInstanceHandle = registration.objectInstanceHandle;
    discoveries = registry.planObjectInstanceDiscoveriesForInstance(
        federationName,
        objectInstanceHandle);
  }
  queueObjectInstanceDiscoveries(std::move(discoveries), federationName);
  return makeObjectInstanceHandle(objectInstanceHandle);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Register Object Instance", exception);
    throw;
  }
}

void UmbraRtiAmbassador::deleteObjectInstance(
    ObjectInstanceHandle const& objectInstance,
    VariableLengthData const& userSuppliedTag) {
  auto instrumentationScope = beginRtiCall("deleteObjectInstance");
  try {
  std::wstring federationName;
  std::vector<umbra::detail::ObjectInstanceRemovalRecipient> removals;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Delete Object Instance");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Delete Object Instance requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const objectInstanceHandle = objectInstanceHandleValue(objectInstance);
    if (!objectInstanceHandle) {
      throw ObjectInstanceNotKnown(
          L"The supplied ObjectInstanceHandle is not known to this federate.");
    }
    auto deletion = registry.deleteObjectInstance(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectInstanceHandle);
    if (deletion.status != umbra::detail::ObjectInstanceDeletionStatus::applied) {
      throwObjectInstanceDeletionFailure(deletion.status);
    }

    // Section 6.16 has accepted the receive-order deletion and committed its
    // local object transition. Record the source-backed Table 5 invocation
    // before queueing any induced Remove Object Instance callbacks.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"DeleteObjectInstance",
        umbra::detail::MomServiceType::object_management,
        {{umbra::detail::MomArgumentType::object_instance_handle,
          L"Object instance designator",
          umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
         {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
          L"User-supplied tag",
          umbra::detail::formatMomUserSuppliedTag(userSuppliedTag)},
         {umbra::detail::MomArgumentType::null_value,
          L"Optional timestamp",
          umbra::detail::formatMomNull()}});
    federationName = *joinedFederationName_;
    removals = std::move(deletion.recipients);
  }
  queueObjectInstanceRemovals(std::move(removals), federationName, userSuppliedTag);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Delete Object Instance", exception);
    throw;
  }
}

MessageRetractionHandle UmbraRtiAmbassador::deleteObjectInstance(
    ObjectInstanceHandle const& objectInstance,
    VariableLengthData const& userSuppliedTag,
    LogicalTime const& time) {
  auto instrumentationScope = beginRtiCall("deleteObjectInstance");
  try {
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Timestamped Delete Object Instance");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Timestamped Delete Object Instance requires membership in a federation execution.");
    }
    if (!embeddedFederationManagement().registry().memberById(
            *joinedFederationName_,
            *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    timeState = federateTimeState_;
  }

  auto const objectInstanceHandle = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceHandle) {
    throw ObjectInstanceNotKnown(
        L"Timestamped Delete Object Instance requires a known ObjectInstanceHandle.");
  }

  auto timestamp = cloneReferenceLogicalTime(timeState->implementationName(), time);
  auto const timeSnapshot = timeState->snapshot();
  validateTsoTimestamp(timeSnapshot, *timestamp);
  VariableLengthData copiedTag(userSuppliedTag);

  std::wstring federationName;
  std::uint64_t producingFederateId = 0;
  std::vector<std::uint64_t> tsoRecipientIds;
  umbra::detail::FederationTsoObjectDeletionEnqueueResult enqueueResult;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ ||
        !federateTimeState_ || federateTimeState_ != timeState) {
      throw FederateNotExecutionMember(
          L"Timestamped Delete Object Instance requires an active joined federate.");
    }
    federationName = *joinedFederationName_;
    producingFederateId = *joinedFederateId_;
    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(federationName, producingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }

    auto const plan = registry.planTsoObjectInstanceDeletion(
        federationName,
        producingFederateId,
        *objectInstanceHandle);
    if (plan.status != umbra::detail::ObjectInstanceDeletionStatus::applied) {
      throwObjectInstanceDeletionFailure(plan.status);
    }

    std::set<std::uint64_t> timeConstrainedRecipients;
    if (timeSnapshot.timeRegulating) {
      auto const federationSnapshot = registry.timeSnapshotFor(federationName);
      if (!federationSnapshot) {
        throw RTIinternalError(
            L"The embedded federation no longer exposes a coherent time snapshot.");
      }
      for (auto const& federate : federationSnapshot->federates) {
        if (federate.time.timeConstrained) {
          timeConstrainedRecipients.insert(federate.membership.id);
        }
      }
    }
    for (auto const& recipient : plan.recipients) {
      if (timeSnapshot.timeRegulating &&
          timeConstrainedRecipients.contains(recipient.receivingFederateId)) {
        tsoRecipientIds.push_back(recipient.receivingFederateId);
      }
    }

    umbra::detail::TsoObjectDeletionMessage message;
    message.producingFederateId = producingFederateId;
    message.objectInstanceHandle = *objectInstanceHandle;
    message.userSuppliedTag = copiedTag;
    message.timestamp = std::shared_ptr<LogicalTime const>(timestamp);
    enqueueResult = registry.enqueueTsoObjectDeletion(
        federationName,
        producingFederateId,
        *objectInstanceHandle,
        std::move(message),
        tsoRecipientIds);
  }

  if (enqueueResult.status == umbra::detail::FederationTsoRegistryStatus::federation_does_not_exist ||
      enqueueResult.status == umbra::detail::FederationTsoRegistryStatus::federate_not_member) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  if (enqueueResult.deletionStatus !=
      umbra::detail::ObjectInstanceDeletionStatus::applied) {
    throwObjectInstanceDeletionFailure(enqueueResult.deletionStatus);
  }
  if (enqueueResult.status != umbra::detail::FederationTsoRegistryStatus::applied ||
      enqueueResult.queueStatus != umbra::detail::TsoMessageQueueStatus::applied ||
      enqueueResult.messageId == 0) {
    throw RTIinternalError(
        L"The embedded federation could not queue the timestamped Delete Object Instance service.");
  }

  std::set<std::uint64_t> queuedRecipients(
      tsoRecipientIds.begin(), tsoRecipientIds.end());
  for (auto const& recipient : enqueueResult.recipients) {
    if (queuedRecipients.contains(recipient.receivingFederateId)) {
      continue;
    }
    queueTimestampedObjectInstanceRemoval(
        recipient.callbackRoute,
        recipient.serviceReportRoute,
        federationName,
        recipient.receivingFederateId,
        recipient.objectInstanceHandle,
        enqueueResult.messageId,
        copiedTag,
        std::shared_ptr<LogicalTime const>(timestamp),
        timeSnapshot.timeRegulating);
  }

  if (!timeSnapshot.timeRegulating) {
    return MessageRetractionHandle();
  }
  return makeMessageRetractionHandle(enqueueResult.messageId);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Delete Object Instance", exception);
    throw;
  }
}

void UmbraRtiAmbassador::localDeleteObjectInstance(
    ObjectInstanceHandle const& objectInstance) {
  auto instrumentationScope = beginRtiCall("localDeleteObjectInstance");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  requireFederationServiceOperationAvailable(L"Local Delete Object Instance");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Local Delete Object Instance requires membership in a federation execution.");
  }

  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  auto const objectInstanceHandle = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceHandle) {
    throw ObjectInstanceNotKnown(
        L"Local Delete Object Instance requires a known ObjectInstanceHandle.");
  }

  auto const status = registry.localDeleteObjectInstance(
      *joinedFederationName_,
      *joinedFederateId_,
      *objectInstanceHandle);
  if (status != umbra::detail::LocalObjectInstanceDeletionStatus::applied) {
    throwLocalObjectInstanceDeletionFailure(status);
  }

  // Section 6.18 completes the invoking federate's local-forget transition at
  // successful invocation.  Record that accepted one-argument service after
  // the registry has committed it, rather than reporting any rejected
  // precondition path.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"LocalDeleteObjectInstance",
      umbra::detail::MomServiceType::object_management,
      {{umbra::detail::MomArgumentType::object_instance_handle,
        L"Object instance designator",
        umbra::detail::formatMomObjectInstanceHandle(objectInstance)}});
  } catch (Exception const& exception) {
    emitExceptionReport(L"Local Delete Object Instance", exception);
    throw;
  }
}

void UmbraRtiAmbassador::updateAttributeValues(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandleValueMap const& attributeValues,
    VariableLengthData const& userSuppliedTag) {
  auto instrumentationScope = beginRtiCall("updateAttributeValues");
  try {
  // Preserve the 2025 service's connection and membership preconditions ahead
  // of caller-supplied handle validation, matching the other public services.
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Update Attribute Values");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Update Attribute Values requires membership in a federation execution.");
    }
    if (!embeddedFederationManagement().registry().memberById(
            *joinedFederationName_,
            *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
  }

  auto const objectInstanceHandle = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceHandle) {
    throw ObjectInstanceNotKnown(
        L"Update Attribute Values requires a known ObjectInstanceHandle.");
  }
  auto const attributeHandles = attributeValueHandleValues(attributeValues);
  if (!attributeHandles) {
    throw AttributeNotDefined(
        L"Update Attribute Values requires defined AttributeHandle values.");
  }

  std::optional<std::wstring> federationName;
  std::optional<std::uint64_t> producingFederateId;
  auto planCurrentRequest = [&]() {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Update Attribute Values");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Update Attribute Values requires membership in a federation execution.");
    }
    if (!federationName) {
      federationName = *joinedFederationName_;
      producingFederateId = *joinedFederateId_;
    } else if (*joinedFederationName_ != *federationName ||
               *joinedFederateId_ != *producingFederateId) {
      throw FederateNotExecutionMember(
          L"The RTI ambassador's federation membership changed during Update Attribute Values.");
    }

    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(*federationName, *producingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto plan = registry.planReceiveOrderAttributeUpdate(
        *federationName,
        *producingFederateId,
        *objectInstanceHandle,
        *attributeHandles);
    if (plan.status != umbra::detail::ReceiveOrderAttributeUpdateStatus::applied) {
      throwReceiveOrderAttributeUpdateFailure(plan.status);
    }
    return plan;
  };

  // Validate membership, object knowledge, source ownership, attribute
  // definitions, and FOM transportation before copying caller-owned buffers.
  // The durable copies below are made without a runtime lock, then the plan is
  // rechecked before recipients are selected for delivery.
  static_cast<void>(planCurrentRequest());
  std::vector<AttributeValue> sentAttributes = copyAttributeValues(attributeValues);
  VariableLengthData copiedTag(userSuppliedTag);
  AttributeHandleValueMap reportAttributeValues;
  for (auto const& [attributeHandle, attributeValue] : sentAttributes) {
    reportAttributeValues.emplace(makeAttributeHandle(attributeHandle), attributeValue);
  }
  // Section 6.10.1 names four supplied values. The receive-order overload
  // leaves the optional timestamp unused, and Section 11.5.1 therefore
  // requires its corresponding supplied-argument element to be Null. The
  // accepted value map is rebuilt from durable copies before formatting so
  // caller-owned buffers cannot alter the report boundary.
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::object_instance_handle,
       L"Object instance designator",
       umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
      {umbra::detail::MomArgumentType::attribute_handle_value_map,
       L"Constrained set of attribute designator and value pairs",
       umbra::detail::formatMomAttributeHandleValueMap(reportAttributeValues)},
      {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       umbra::detail::formatMomUserSuppliedTag(copiedTag)},
      {umbra::detail::MomArgumentType::null_value,
       L"Optional timestamp",
       umbra::detail::formatMomNull()},
  };
  auto plan = planCurrentRequest();

  struct Delivery {
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute;
    std::uint64_t recipientId = 0;
    std::vector<std::uint64_t> sentAttributeHandles;
    TransportationTypeHandle transportationType;
    double maximumUpdateRate = 0.0;
    bool reliableTransportation = false;
    std::optional<std::set<std::uint64_t>> sentRegionHandles;
    bool defaultRegionUsed = false;
  };
  std::vector<Delivery> deliveries;
  for (auto const& passel : plan.passels) {
    auto const transportationName = umbra::detail::wideFromUtf8(passel.transportationName);
    if (!transportationName) {
      throw RTIinternalError(
          L"The composed FOM contains an invalid UTF-8 transportation type name.");
    }
    auto const transportationValue = standardTransportationTypeValue(*transportationName);
    if (!transportationValue) {
      // The 2025 binding must expose the mandatory standard transport types;
      // custom transport implementations have no delivery semantics in this
      // intentionally narrow embedded profile.
      throw RTIinternalError(
          L"The embedded profile cannot deliver an attribute update using this FOM transportation type.");
    }
    TransportationTypeHandle transportationType = makeTransportationTypeHandle(*transportationValue);
    for (auto const& recipient : passel.recipients) {
      if (!recipient.callbackRoute) {
        throw RTIinternalError(
            L"The embedded federation has an eligible attribute-update recipient without a callback route.");
      }
      deliveries.push_back({
          recipient.callbackRoute,
          recipient.federateId,
          passel.sentAttributeHandles,
          transportationType,
          recipient.maximumUpdateRate,
          passel.transportationName == "HLAreliable",
          passel.sentRegionHandles.empty()
              ? std::nullopt
              : std::optional<std::set<std::uint64_t>>(passel.sentRegionHandles),
          passel.defaultRegionUsed,
      });
    }
  }

  // Every synchronous pre-callback delivery check has now succeeded. Section
  // 6.10's accepted update is the report boundary, and the §11.5 file record
  // must precede any induced Reflect Attribute Values callback.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"UpdateAttributeValues",
      umbra::detail::MomServiceType::object_management,
      reportArguments);

  // The accepted Update Attribute Values invocation is the MOM counter's
  // source boundary. Record it after the service-report write has succeeded
  // and before any induced callback is exposed.
  {
    std::scoped_lock lock(federationManagementMutex());
    auto const status = embeddedFederationManagement().registry()
        .recordSuccessfulUpdateAttributeValues(
            *federationName,
            *producingFederateId,
            *objectInstanceHandle);
    if (status != umbra::detail::FederationRegistryStatus::applied) {
      throw RTIinternalError(
          L"The embedded federation lost the Update Attribute Values membership before its accepted boundary.");
    }
  }

  // Do not hold either sender lock while submitting a route: HLA_IMMEDIATE may
  // synchronously enter a different federate's Reflect Attribute Values callback.
  for (auto& delivery : deliveries) {
    queueReceiveOrderAttributeUpdate(
        std::move(delivery.callbackRoute),
        *federationName,
        *producingFederateId,
        delivery.recipientId,
        *objectInstanceHandle,
        std::move(delivery.sentAttributeHandles),
        sentAttributes,
        copiedTag,
        delivery.transportationType,
        delivery.maximumUpdateRate,
        delivery.reliableTransportation,
        std::move(delivery.sentRegionHandles),
        delivery.defaultRegionUsed);
  }
  } catch (Exception const& exception) {
    emitExceptionReport(L"Update Attribute Values", exception);
    throw;
  }
}

MessageRetractionHandle UmbraRtiAmbassador::updateAttributeValues(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandleValueMap const& attributeValues,
    VariableLengthData const& userSuppliedTag,
    LogicalTime const& time) {
  auto instrumentationScope = beginRtiCall("updateAttributeValues");
  try {
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Timestamped Update Attribute Values");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Timestamped Update Attribute Values requires membership in a federation execution.");
    }
    if (!embeddedFederationManagement().registry().memberById(
            *joinedFederationName_,
            *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    timeState = federateTimeState_;
  }

  auto const objectInstanceHandle = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceHandle) {
    throw ObjectInstanceNotKnown(
        L"Timestamped Update Attribute Values requires a known ObjectInstanceHandle.");
  }
  auto const attributeHandles = attributeValueHandleValues(attributeValues);
  if (!attributeHandles) {
    throw AttributeNotDefined(
        L"Timestamped Update Attribute Values requires defined AttributeHandle values.");
  }

  auto timestamp = cloneReferenceLogicalTime(timeState->implementationName(), time);
  auto const timeSnapshot = timeState->snapshot();
  validateTsoTimestamp(timeSnapshot, *timestamp);

  std::optional<std::wstring> federationName;
  std::optional<std::uint64_t> producingFederateId;
  auto planCurrentRequest = [&]() {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Timestamped Update Attribute Values");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ ||
        !federateTimeState_ || federateTimeState_ != timeState) {
      throw FederateNotExecutionMember(
          L"Timestamped Update Attribute Values requires an active joined federate.");
    }
    if (!federationName) {
      federationName = *joinedFederationName_;
      producingFederateId = *joinedFederateId_;
    } else if (*joinedFederationName_ != *federationName ||
               *joinedFederateId_ != *producingFederateId) {
      throw FederateNotExecutionMember(
          L"The RTI ambassador's federation membership changed during Timestamped Update Attribute Values.");
    }

    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(*federationName, *producingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto plan = registry.planReceiveOrderAttributeUpdate(
        *federationName,
        *producingFederateId,
        *objectInstanceHandle,
        *attributeHandles);
    if (plan.status != umbra::detail::ReceiveOrderAttributeUpdateStatus::applied) {
      throwReceiveOrderAttributeUpdateFailure(plan.status);
    }
    return plan;
  };

  // Validate the object, ownership, attribute definitions, and transport
  // passels before copying caller-owned buffers, then re-plan immediately
  // before accepting the timestamped message.
  static_cast<void>(planCurrentRequest());
  std::vector<AttributeValue> sentAttributes = copyAttributeValues(attributeValues);
  VariableLengthData copiedTag(userSuppliedTag);
  auto plan = planCurrentRequest();
  bool const hasTimestampOrderedAttribute = std::any_of(
      plan.passels.begin(),
      plan.passels.end(),
      [](umbra::detail::ReceiveOrderAttributeUpdatePassel const& passel) {
        return passel.preferredOrderType == TIMESTAMP;
      });

  struct Delivery {
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute;
    std::vector<umbra::detail::TsoAttributeUpdatePassel> receivePassels;
    std::vector<umbra::detail::TsoAttributeUpdatePassel> timestampPassels;
  };
  std::map<std::uint64_t, Delivery> deliveries;
  for (auto const& passel : plan.passels) {
    auto const transportationName = umbra::detail::wideFromUtf8(
        passel.transportationName);
    auto const transportationValue = transportationName
        ? standardTransportationTypeValue(*transportationName)
        : std::nullopt;
    if (!transportationValue) {
      throw RTIinternalError(
          L"The embedded profile cannot deliver a timestamped attribute update using this FOM transportation type.");
    }

    umbra::detail::TsoAttributeUpdatePassel payloadPassel;
    payloadPassel.transportationName = passel.transportationName;
    payloadPassel.preferredOrderType = passel.preferredOrderType;
    payloadPassel.sentAttributeHandles = passel.sentAttributeHandles;
    payloadPassel.sentRegionHandles = passel.sentRegionHandles;
    payloadPassel.defaultRegionUsed = passel.defaultRegionUsed;
    for (auto const& recipient : passel.recipients) {
      if (!recipient.callbackRoute) {
        throw RTIinternalError(
            L"The embedded federation has an eligible timestamped attribute recipient without a callback route.");
      }
      auto& delivery = deliveries[recipient.federateId];
      if (!delivery.callbackRoute) {
        delivery.callbackRoute = recipient.callbackRoute;
      }
      if (passel.preferredOrderType == TIMESTAMP) {
        delivery.timestampPassels.push_back(payloadPassel);
      } else {
        delivery.receivePassels.push_back(payloadPassel);
      }
    }
  }

  std::set<std::uint64_t> timeConstrainedRecipients;
  if (federationName && timeSnapshot.timeRegulating) {
    std::scoped_lock lock(federationManagementMutex());
    auto const snapshot = embeddedFederationManagement().registry().timeSnapshotFor(
        *federationName);
    if (!snapshot) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer exposes a coherent time snapshot.");
    }
    for (auto const& federate : snapshot->federates) {
      if (federate.time.timeConstrained) {
        timeConstrainedRecipients.insert(federate.membership.id);
      }
    }
  }

  std::uint64_t messageId = 0;
  if (timeSnapshot.timeRegulating && hasTimestampOrderedAttribute) {
    std::vector<std::uint64_t> queuedTsoRecipientIds;
    std::vector<std::uint64_t> allTimestampedRecipientIds;
    for (auto const& [recipientId, delivery] : deliveries) {
      if (delivery.timestampPassels.empty()) {
        continue;
      }
      allTimestampedRecipientIds.push_back(recipientId);
      if (timeConstrainedRecipients.contains(recipientId)) {
        queuedTsoRecipientIds.push_back(recipientId);
      }
    }

    umbra::detail::TsoAttributeUpdateMessage message;
    message.producingFederateId = *producingFederateId;
    message.objectInstanceHandle = *objectInstanceHandle;
    message.attributes = sentAttributes;
    message.userSuppliedTag = copiedTag;
    message.timestamp = std::shared_ptr<LogicalTime const>(timestamp);
    for (auto const& [recipientId, delivery] : deliveries) {
      if (!delivery.timestampPassels.empty()) {
        message.passelsByRecipient.emplace(recipientId, delivery.timestampPassels);
      }
    }

    std::scoped_lock lock(federationManagementMutex());
    auto const result = embeddedFederationManagement().registry()
        .enqueueTsoAttributeUpdate(
            *federationName,
            std::move(message),
            queuedTsoRecipientIds,
            allTimestampedRecipientIds);
    if (result.status != umbra::detail::FederationTsoRegistryStatus::applied ||
        result.queueStatus != umbra::detail::TsoMessageQueueStatus::applied ||
        result.messageId == 0) {
      throw RTIinternalError(
          L"The embedded federation could not queue the timestamped Update Attribute Values service.");
    }
    messageId = result.messageId;
  }

  // Count the successful timestamped invocation once queue admission and all
  // synchronous validation have completed. Timestamped payload delivery is a
  // later boundary and must not alter the MOM service-invocation count.
  {
    std::scoped_lock lock(federationManagementMutex());
    auto const status = embeddedFederationManagement().registry()
        .recordSuccessfulUpdateAttributeValues(
            *federationName,
            *producingFederateId,
            *objectInstanceHandle);
    if (status != umbra::detail::FederationRegistryStatus::applied) {
      throw RTIinternalError(
          L"The embedded federation lost the timestamped Update Attribute Values membership before its accepted boundary.");
    }
  }

  // Time-constrained recipients consume the typed payload at their grant.
  // Other recipients receive the timestamped callback immediately in the
  // bounded embedded profile; the callback still rechecks each accepted
  // passel immediately before entering user code.
  for (auto& [recipientId, delivery] : deliveries) {
    bool const queuedTimestamped =
        timeSnapshot.timeRegulating &&
        timeConstrainedRecipients.contains(recipientId) &&
        !delivery.timestampPassels.empty();
    if (!delivery.receivePassels.empty()) {
      queueTimestampedReflectAttributeUpdate(
          delivery.callbackRoute,
          *federationName,
          *producingFederateId,
          recipientId,
          *objectInstanceHandle,
          sentAttributes,
          std::move(delivery.receivePassels),
          copiedTag,
          std::shared_ptr<LogicalTime const>(timestamp),
          RECEIVE,
          RECEIVE,
          std::nullopt);
    }
    if (!queuedTimestamped && !delivery.timestampPassels.empty()) {
      queueTimestampedReflectAttributeUpdate(
          std::move(delivery.callbackRoute),
          *federationName,
          *producingFederateId,
          recipientId,
          *objectInstanceHandle,
          sentAttributes,
          std::move(delivery.timestampPassels),
          copiedTag,
          std::shared_ptr<LogicalTime const>(timestamp),
          timeSnapshot.timeRegulating ? TIMESTAMP : RECEIVE,
          RECEIVE,
          timeSnapshot.timeRegulating && messageId != 0
              ? std::optional<std::uint64_t>(messageId)
              : std::nullopt);
    }
  }

  if (!timeSnapshot.timeRegulating || messageId == 0) {
    return MessageRetractionHandle();
  }
  return makeMessageRetractionHandle(messageId);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Update Attribute Values", exception);
    throw;
  }
}

void UmbraRtiAmbassador::requestAttributeValueUpdate(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandleSet const& attributes,
    VariableLengthData const& userSuppliedTag) {
  auto instrumentationScope = beginRtiCall("requestAttributeValueUpdate");
  try {
  // Preserve the official connection and membership preconditions before
  // caller-supplied handle validation, as with the other object services.
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Request Attribute Value Update");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Request Attribute Value Update requires membership in a federation execution.");
    }
    if (!embeddedFederationManagement().registry().memberById(
            *joinedFederationName_,
            *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
  }

  auto const objectInstanceHandle = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceHandle) {
    throw ObjectInstanceNotKnown(
        L"Request Attribute Value Update requires a known ObjectInstanceHandle.");
  }
  auto const requestedAttributeHandles = attributeHandleValues(attributes);
  if (!requestedAttributeHandles) {
    throw AttributeNotDefined(
        L"Request Attribute Value Update requires defined AttributeHandle values.");
  }

  // A joined-federate MOM object is RTI-owned: Request Attribute Value Update
  // reflects the immutable MOM values directly and never induces a Provide
  // Attribute Value Update callback at a federate. Keep this branch separate
  // from the ordinary ownership planner so the private MOM ledger is not
  // mistaken for a federate-created object instance.
  std::optional<umbra::detail::ObjectInstanceCallbackRoute> momCallbackRoute;
  std::wstring momFederationName;
  std::uint64_t momFederateId = 0;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Request Attribute Value Update");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Request Attribute Value Update requires membership in a federation execution.");
    }
    auto& registry = embeddedFederationManagement().registry();
    auto momPlan = registry.planJoinedFederateMomAttributeValueUpdate(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectInstanceHandle,
        *requestedAttributeHandles);
    if (momPlan.rtiOwnedMomObject) {
      if (momPlan.status !=
          umbra::detail::JoinedFederateMomAttributeValueUpdateStatus::applied ||
          !momPlan.recipient) {
        throwJoinedFederateMomAttributeValueUpdateFailure(momPlan.status);
      }
      VariableLengthData copiedTag(userSuppliedTag);
      appendSuccessfulVoidServiceReportToFileIfSelected(
          L"RequestAttributeValueUpdate",
          umbra::detail::MomServiceType::object_management,
          {{umbra::detail::MomArgumentType::object_instance_handle,
            L"Object instance designator",
            umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
           {umbra::detail::MomArgumentType::attribute_handle_set,
            L"Set of attribute designators",
            umbra::detail::formatMomAttributeHandleSet(attributes)},
           {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
            L"User-supplied tag",
            umbra::detail::formatMomUserSuppliedTag(copiedTag)}});
      momFederationName = *joinedFederationName_;
      momFederateId = *joinedFederateId_;
      momCallbackRoute = std::move(momPlan.recipient->callbackRoute);
    }
  }
  if (momCallbackRoute) {
    queueJoinedFederateMomAttributeValueUpdate(
        std::move(*momCallbackRoute),
        std::move(momFederationName),
        momFederateId,
        *objectInstanceHandle,
        *requestedAttributeHandles);
    return;
  }

  std::optional<std::wstring> federationName;
  std::optional<std::uint64_t> requestingFederateId;
  std::optional<std::vector<umbra::detail::MomServiceArgument>> reportArguments;
  auto planCurrentRequest = [&](bool emitServiceReport) {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Request Attribute Value Update");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Request Attribute Value Update requires membership in a federation execution.");
    }
    if (!federationName) {
      federationName = *joinedFederationName_;
      requestingFederateId = *joinedFederateId_;
    } else if (*joinedFederationName_ != *federationName ||
               *joinedFederateId_ != *requestingFederateId) {
      throw FederateNotExecutionMember(
          L"The RTI ambassador's federation membership changed during Request Attribute Value Update.");
    }

    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(*federationName, *requestingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto plan = registry.planAttributeValueUpdateRequest(
        *federationName,
        *requestingFederateId,
        *objectInstanceHandle,
        *requestedAttributeHandles);
    if (plan.status != umbra::detail::AttributeValueUpdateRequestStatus::applied) {
      throwAttributeValueUpdateRequestFailure(plan.status);
    }
    if (emitServiceReport) {
      if (!reportArguments) {
        throw RTIinternalError(
            L"Umbra could not format the accepted Request Attribute Value Update report.");
      }
      // Section 6.21's accepted request is the reporting boundary.  Any
      // induced Provide Attribute Value Update callback remains separately
      // queued after this successful-void Table 5 record.
      appendSuccessfulVoidServiceReportToFileIfSelected(
          L"RequestAttributeValueUpdate",
          umbra::detail::MomServiceType::object_management,
          *reportArguments);
    }
    return plan;
  };

  // The request has no caller-owned value payload, but copy the tag only
  // after its current object and attribute boundary has been validated. Then
  // replan before routing so a concurrent resign or lifecycle change cannot
  // use a stale recipient snapshot.
  static_cast<void>(planCurrentRequest(false));
  VariableLengthData copiedTag(userSuppliedTag);
  // Section 6.21 names these supplied values "Object instance designator",
  // "Set of attribute designators", and "User-supplied tag".  Table 5 fixes
  // their type-37, type-1, and Binary Data value forms respectively.  Preserve
  // the copied tag in the file record so a caller-owned input buffer cannot
  // alter the accepted service's audit boundary.
  reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::object_instance_handle,
       L"Object instance designator",
       umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
      {umbra::detail::MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       umbra::detail::formatMomAttributeHandleSet(attributes)},
      {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       umbra::detail::formatMomUserSuppliedTag(copiedTag)},
  };
  auto plan = planCurrentRequest(true);

  struct Delivery {
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute;
    umbra::detail::FederateServiceReportRoute serviceReportRoute;
    std::uint64_t providingFederateId = 0;
    std::set<std::uint64_t> requestedAttributeHandles;
  };
  std::vector<Delivery> deliveries;
  deliveries.reserve(plan.recipients.size());
  for (auto const& recipient : plan.recipients) {
    if (!recipient.callbackRoute) {
      throw RTIinternalError(
          L"The embedded federation has an attribute-update provider without a callback route.");
    }
    deliveries.push_back({
        recipient.callbackRoute,
        recipient.serviceReportRoute,
        recipient.providingFederateId,
        recipient.requestedAttributeHandles,
    });
  }

  // Do not hold the requester lock while submitting a route: HLA_IMMEDIATE
  // may synchronously enter the owner's Provide Attribute Value Update callback.
  for (auto& delivery : deliveries) {
    queueAttributeValueUpdateProvide(
        std::move(delivery.callbackRoute),
        std::move(delivery.serviceReportRoute),
        *federationName,
        *requestingFederateId,
        delivery.providingFederateId,
        *objectInstanceHandle,
        std::move(delivery.requestedAttributeHandles),
        copiedTag);
  }
  } catch (Exception const& exception) {
    emitExceptionReport(L"Request Attribute Value Update", exception);
    throw;
  }
}

void UmbraRtiAmbassador::requestAttributeValueUpdate(
    ObjectClassHandle const& objectClass,
    AttributeHandleSet const& attributes,
    VariableLengthData const& userSuppliedTag) {
  auto instrumentationScope = beginRtiCall("requestAttributeValueUpdate");
  try {
  // Preserve the official connection and membership preconditions before
  // caller-supplied handle validation, as with the object-instance form.
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Request Attribute Value Update");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Request Attribute Value Update requires membership in a federation execution.");
    }
    if (!embeddedFederationManagement().registry().memberById(
            *joinedFederationName_,
            *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
  }

  auto const requestedObjectClassHandle = objectClassHandleValue(objectClass);
  if (!requestedObjectClassHandle) {
    throw ObjectClassNotDefined(
        L"Request Attribute Value Update requires a defined ObjectClassHandle.");
  }
  auto const requestedAttributeHandles = attributeHandleValues(attributes);
  if (!requestedAttributeHandles) {
    throw AttributeNotDefined(
        L"Request Attribute Value Update requires defined AttributeHandle values.");
  }

  std::vector<umbra::detail::JoinedFederateMomAttributeValueUpdateRecipient>
      momRecipients;
  std::wstring momFederationName;
  std::uint64_t momFederateId = 0;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Request Attribute Value Update");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Request Attribute Value Update requires membership in a federation execution.");
    }
    auto& registry = embeddedFederationManagement().registry();
    auto momPlan = registry.planJoinedFederateMomAttributeValueUpdateClass(
        *joinedFederationName_,
        *joinedFederateId_,
        *requestedObjectClassHandle,
        *requestedAttributeHandles);
    if (momPlan.rtiOwnedMomObject) {
      // Reuse the ordinary class planner's validation boundary for the public
      // class designator and requested attributes, then route only the
      // RTI-owned MOM values discovered by the dedicated plan.
      auto validation = registry.planAttributeValueUpdateClassRequest(
          *joinedFederationName_,
          *joinedFederateId_,
          *requestedObjectClassHandle,
          *requestedAttributeHandles);
      if (validation.status !=
          umbra::detail::AttributeValueUpdateClassRequestStatus::applied) {
        throwAttributeValueUpdateClassRequestFailure(validation.status);
      }
      VariableLengthData copiedTag(userSuppliedTag);
      appendSuccessfulVoidServiceReportToFileIfSelected(
          L"RequestAttributeValueUpdate",
          umbra::detail::MomServiceType::object_management,
          {{umbra::detail::MomArgumentType::object_class_handle,
            L"Object class designator",
            umbra::detail::formatMomObjectClassHandle(objectClass)},
           {umbra::detail::MomArgumentType::attribute_handle_set,
            L"Set of attribute designators",
            umbra::detail::formatMomAttributeHandleSet(attributes)},
           {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
            L"User-supplied tag",
            umbra::detail::formatMomUserSuppliedTag(copiedTag)}});
      momFederationName = *joinedFederationName_;
      momFederateId = *joinedFederateId_;
      momRecipients = std::move(momPlan.recipients);
    }
  }
  if (!momRecipients.empty() || !momFederationName.empty()) {
    for (auto& recipient : momRecipients) {
      queueJoinedFederateMomAttributeValueUpdate(
          std::move(recipient.callbackRoute),
          momFederationName,
          momFederateId,
          recipient.objectInstanceHandle,
          *requestedAttributeHandles);
    }
    return;
  }

  std::optional<std::wstring> federationName;
  std::optional<std::uint64_t> requestingFederateId;
  std::optional<std::vector<umbra::detail::MomServiceArgument>> reportArguments;
  auto planCurrentRequest = [&](bool emitServiceReport) {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Request Attribute Value Update");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Request Attribute Value Update requires membership in a federation execution.");
    }
    if (!federationName) {
      federationName = *joinedFederationName_;
      requestingFederateId = *joinedFederateId_;
    } else if (*joinedFederationName_ != *federationName ||
               *joinedFederateId_ != *requestingFederateId) {
      throw FederateNotExecutionMember(
          L"The RTI ambassador's federation membership changed during Request Attribute Value Update.");
    }

    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(*federationName, *requestingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto plan = registry.planAttributeValueUpdateClassRequest(
        *federationName,
        *requestingFederateId,
        *requestedObjectClassHandle,
        *requestedAttributeHandles);
    if (plan.status != umbra::detail::AttributeValueUpdateClassRequestStatus::applied) {
      throwAttributeValueUpdateClassRequestFailure(plan.status);
    }
    if (emitServiceReport) {
      if (!reportArguments) {
        throw RTIinternalError(
            L"Umbra could not format the accepted Request Attribute Value Update report.");
      }
      // As with the object-instance overload, write the accepted §6.21 record
      // before any later per-instance Provide Attribute Value Update callbacks.
      appendSuccessfulVoidServiceReportToFileIfSelected(
          L"RequestAttributeValueUpdate",
          umbra::detail::MomServiceType::object_management,
          *reportArguments);
    }
    return plan;
  };

  // Copy the tag only after the selected class and its attributes have been
  // validated. Replanning immediately before routing prevents a concurrent
  // resign or lifecycle transition from using a stale owner snapshot.
  static_cast<void>(planCurrentRequest(false));
  VariableLengthData copiedTag(userSuppliedTag);
  // The class overload changes only the first standards-facing argument; the
  // same attribute-set and copied Binary Data tag representation is required.
  reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::object_class_handle,
       L"Object class designator",
       umbra::detail::formatMomObjectClassHandle(objectClass)},
      {umbra::detail::MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       umbra::detail::formatMomAttributeHandleSet(attributes)},
      {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       umbra::detail::formatMomUserSuppliedTag(copiedTag)},
  };
  auto plan = planCurrentRequest(true);

  struct Delivery {
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute;
    umbra::detail::FederateServiceReportRoute serviceReportRoute;
    std::uint64_t objectInstanceHandle = 0;
    std::uint64_t providingFederateId = 0;
    std::set<std::uint64_t> requestedAttributeHandles;
  };
  std::vector<Delivery> deliveries;
  deliveries.reserve(plan.recipients.size());
  for (auto const& recipient : plan.recipients) {
    if (!recipient.callbackRoute) {
      throw RTIinternalError(
          L"The embedded federation has an attribute-update provider without a callback route.");
    }
    deliveries.push_back({
        recipient.callbackRoute,
        recipient.serviceReportRoute,
        recipient.objectInstanceHandle,
        recipient.providingFederateId,
        recipient.requestedAttributeHandles,
    });
  }

  // Do not hold the requester lock while submitting a route: HLA_IMMEDIATE
  // may synchronously enter the owner's Provide Attribute Value Update callback.
  for (auto& delivery : deliveries) {
    queueAttributeValueUpdateClassProvide(
        std::move(delivery.callbackRoute),
        std::move(delivery.serviceReportRoute),
        *federationName,
        *requestingFederateId,
        delivery.providingFederateId,
        delivery.objectInstanceHandle,
        *requestedObjectClassHandle,
        std::move(delivery.requestedAttributeHandles),
        copiedTag);
  }
  } catch (Exception const& exception) {
    emitExceptionReport(L"Request Attribute Value Update", exception);
    throw;
  }
}

void UmbraRtiAmbassador::requestAttributeValueUpdateWithRegions(
    ObjectClassHandle const& objectClass,
    AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions,
    VariableLengthData const& userSuppliedTag) {
  auto instrumentationScope = beginRtiCall("requestAttributeValueUpdateWithRegions");
  try {
  // Preserve the official connection and membership preconditions before
  // caller-supplied handle validation, matching the ordinary class request.
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Request Attribute Value Update With Regions");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Request Attribute Value Update With Regions requires membership in a federation execution.");
    }
    if (!embeddedFederationManagement().registry().memberById(
            *joinedFederationName_,
            *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
  }

  auto const requestedObjectClassHandle = objectClassHandleValue(objectClass);
  if (!requestedObjectClassHandle) {
    throw ObjectClassNotDefined(
        L"Request Attribute Value Update With Regions requires a defined ObjectClassHandle.");
  }
  auto pairValues = attributeRegionPairValues(attributesAndRegions);
  if (pairValues.invalidAttributeHandle) {
    throw AttributeNotDefined(
        L"Request Attribute Value Update With Regions requires defined AttributeHandle values.");
  }
  if (pairValues.invalidRegionHandle) {
    throw InvalidRegion(
        L"Request Attribute Value Update With Regions requires defined RegionHandle values.");
  }
  auto const requestedAttributeHandles = [&]() {
    std::set<std::uint64_t> result;
    for (auto const& [attributeHandle, regions] : pairValues.values) {
      static_cast<void>(regions);
      result.insert(attributeHandle);
    }
    return result;
  }();

  std::wstring federationName;
  std::uint64_t requestingFederateId = 0;
  umbra::detail::AttributeValueUpdateClassRequestPlan plan;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Request Attribute Value Update With Regions requires membership in a federation execution.");
    }
    federationName = *joinedFederationName_;
    requestingFederateId = *joinedFederateId_;
    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(federationName, requestingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    plan = registry.planAttributeValueUpdateClassRequest(
        federationName,
        requestingFederateId,
        *requestedObjectClassHandle,
        requestedAttributeHandles,
        &pairValues.values);
    if (plan.status != umbra::detail::AttributeValueUpdateClassRequestStatus::applied) {
      throwAttributeValueUpdateClassRequestFailure(plan.status);
    }
  }

  // The tag is copied only after all object-class, attribute, and region
  // arguments have been validated. Replanning after the copy closes the same
  // membership/ownership race as the ordinary class request.
  VariableLengthData copiedTag(userSuppliedTag);
  // §9.13 is a successful-void DDM service.  Keep the copied tag in the
  // supplied-argument record so caller-owned storage cannot change the audit
  // value after this accepted request boundary.
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::object_class_handle,
       L"Object class designator",
       umbra::detail::formatMomObjectClassHandle(objectClass)},
      {umbra::detail::MomArgumentType::attribute_set_region_set_pair_list,
       L"Collection of attribute designator set and region designator set pairs",
       umbra::detail::formatMomAttributeSetRegionSetPairList(attributesAndRegions)},
      {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       umbra::detail::formatMomUserSuppliedTag(copiedTag)},
  };
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ ||
        *joinedFederationName_ != federationName ||
        *joinedFederateId_ != requestingFederateId) {
      throw FederateNotExecutionMember(
          L"The RTI ambassador's federation membership changed during Request Attribute Value Update With Regions.");
    }
    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(federationName, requestingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    plan = registry.planAttributeValueUpdateClassRequest(
        federationName,
        requestingFederateId,
        *requestedObjectClassHandle,
        requestedAttributeHandles,
        &pairValues.values);
    if (plan.status != umbra::detail::AttributeValueUpdateClassRequestStatus::applied) {
      throwAttributeValueUpdateClassRequestFailure(plan.status);
    }
    // The requester-side §9.13 record precedes any separately queued
    // Provide Attribute Value Update callbacks at eligible owners.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"RequestAttributeValueUpdateWithRegions",
        umbra::detail::MomServiceType::data_distribution_management,
        reportArguments);
  }

  struct Delivery {
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute;
    umbra::detail::FederateServiceReportRoute serviceReportRoute;
    std::uint64_t objectInstanceHandle = 0;
    std::uint64_t providingFederateId = 0;
    std::set<std::uint64_t> requestedAttributeHandles;
  };
  std::vector<Delivery> deliveries;
  deliveries.reserve(plan.recipients.size());
  for (auto const& recipient : plan.recipients) {
    if (!recipient.callbackRoute) {
      throw RTIinternalError(
          L"The embedded federation has an attribute-update provider without a callback route.");
    }
    deliveries.push_back({
        recipient.callbackRoute,
        recipient.serviceReportRoute,
        recipient.objectInstanceHandle,
        recipient.providingFederateId,
        recipient.requestedAttributeHandles,
    });
  }

  // Do not hold the requester lock while submitting a route: HLA_IMMEDIATE
  // may synchronously enter the owner's Provide Attribute Value Update callback.
  for (auto& delivery : deliveries) {
    queueAttributeValueUpdateClassProvide(
        std::move(delivery.callbackRoute),
        std::move(delivery.serviceReportRoute),
        federationName,
        requestingFederateId,
        delivery.providingFederateId,
        delivery.objectInstanceHandle,
        *requestedObjectClassHandle,
        std::move(delivery.requestedAttributeHandles),
        copiedTag,
        pairValues.values);
  }
  } catch (Exception const& exception) {
    emitExceptionReport(L"Request Attribute Value Update With Regions", exception);
    throw;
  }
}

void UmbraRtiAmbassador::queryAttributeOwnership(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandleSet const& attributes) {
  auto instrumentationScope = beginRtiCall("queryAttributeOwnership");
  try {
  // Preserve the official connection and membership preconditions before
  // caller-supplied handle validation, as with the adjacent object services.
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Query Attribute Ownership");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Query Attribute Ownership requires membership in a federation execution.");
    }
    if (!embeddedFederationManagement().registry().memberById(
            *joinedFederationName_,
            *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
  }

  auto const objectInstanceHandle = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceHandle) {
    throw ObjectInstanceNotKnown(
        L"Query Attribute Ownership requires a known ObjectInstanceHandle.");
  }
  auto const requestedAttributeHandles = attributeHandleValues(attributes);
  if (!requestedAttributeHandles) {
    throw AttributeNotDefined(
        L"Query Attribute Ownership requires defined AttributeHandle values.");
  }
  // Section 7.17 names these supplied values "Object instance designator"
  // and "Set of attribute designators". Table 5 fixes their respective
  // type-37 and type-1 value forms as quoted handle.toString() text and a
  // bracketed array of quoted handle.toString() values.
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::object_instance_handle,
       L"Object instance designator",
       umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
      {umbra::detail::MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       umbra::detail::formatMomAttributeHandleSet(attributes)},
  };

  std::wstring federationName;
  std::uint64_t requestingFederateId = 0;
  umbra::detail::AttributeOwnershipQueryPlan plan;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Query Attribute Ownership");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Query Attribute Ownership requires membership in a federation execution.");
    }
    federationName = *joinedFederationName_;
    requestingFederateId = *joinedFederateId_;
    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(federationName, requestingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    plan = registry.planAttributeOwnershipQuery(
        federationName,
        requestingFederateId,
        *objectInstanceHandle,
        *requestedAttributeHandles);
    if (plan.status != umbra::detail::AttributeOwnershipQueryStatus::applied) {
      throwAttributeOwnershipQueryFailure(plan.status);
    }
    // The accepted request is the service-report boundary. Section 7.18
    // separately requires one or more later ownership-result callbacks.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"QueryAttributeOwnership",
        umbra::detail::MomServiceType::ownership_management,
        reportArguments);
  }

  struct Delivery {
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute;
    umbra::detail::AttributeOwnershipQueryReportKind reportKind =
        umbra::detail::AttributeOwnershipQueryReportKind::unowned;
    std::uint64_t owningFederateId = 0;
    std::set<std::uint64_t> requestedAttributeHandles;
  };
  std::vector<Delivery> deliveries;
  deliveries.reserve(plan.recipients.size());
  for (auto const& recipient : plan.recipients) {
    if (!recipient.callbackRoute) {
      throw RTIinternalError(
          L"The embedded federation has an ownership-query recipient without a callback route.");
    }
    deliveries.push_back({
        recipient.callbackRoute,
        recipient.reportKind,
        recipient.owningFederateId,
        recipient.attributeHandles,
    });
  }

  // Do not hold the requester lock while submitting a route: HLA_IMMEDIATE
  // may synchronously enter Inform Attribute Ownership user code.
  for (auto& delivery : deliveries) {
    queueAttributeOwnershipQueryReport(
        std::move(delivery.callbackRoute),
        federationName,
        requestingFederateId,
        *objectInstanceHandle,
        delivery.reportKind,
        delivery.owningFederateId,
        std::move(delivery.requestedAttributeHandles));
  }
  } catch (Exception const& exception) {
    emitExceptionReport(L"Query Attribute Ownership", exception);
    throw;
  }
}

bool UmbraRtiAmbassador::isAttributeOwnedByFederate(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandle const& attribute) {
  auto instrumentationScope = beginRtiCall("isAttributeOwnedByFederate");
  try {
  // This uses the same official connection, membership, known-instance, and
  // known-class boundaries as Query Attribute Ownership, but returns only the
  // invoking federate's boolean ownership status and has no callback effect.
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Is Attribute Owned By Federate");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Is Attribute Owned By Federate requires membership in a federation execution.");
    }
    if (!embeddedFederationManagement().registry().memberById(
            *joinedFederationName_,
            *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
  }

  auto const objectInstanceHandle = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceHandle) {
    throw ObjectInstanceNotKnown(
        L"Is Attribute Owned By Federate requires a known ObjectInstanceHandle.");
  }
  auto const attributeHandle = attributeHandleValue(attribute);
  if (!attributeHandle) {
    throw AttributeNotDefined(
        L"Is Attribute Owned By Federate requires a defined AttributeHandle.");
  }

  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  requireFederationServiceOperationAvailable(L"Is Attribute Owned By Federate");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Is Attribute Owned By Federate requires membership in a federation execution.");
  }
  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  auto const result = registry.attributeOwnedByFederate(
      *joinedFederationName_,
      *joinedFederateId_,
      *objectInstanceHandle,
      *attributeHandle);
  if (result.status != umbra::detail::AttributeOwnershipCheckStatus::applied) {
    throwAttributeOwnershipCheckFailure(result.status);
  }
  return result.ownedByRequestingFederate;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Is Attribute Owned By Federate", exception);
    throw;
  }
}

void UmbraRtiAmbassador::negotiatedAttributeOwnershipDivestiture(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandleSet const& attributes,
    VariableLengthData const& userSuppliedTag) {
  auto instrumentationScope = beginRtiCall("negotiatedAttributeOwnershipDivestiture");
  try {
  // Preserve the official connection and membership preconditions before
  // caller-supplied handle validation, matching the adjacent ownership
  // services. Save/restore has no implemented state in this profile.
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Negotiated Attribute Ownership Divestiture");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Negotiated Attribute Ownership Divestiture requires federation membership.");
    }
    if (!embeddedFederationManagement().registry().memberById(
            *joinedFederationName_,
            *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
  }

  auto const objectInstanceHandle = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceHandle) {
    throw ObjectInstanceNotKnown(
        L"Negotiated Attribute Ownership Divestiture requires a known ObjectInstanceHandle.");
  }
  auto const attributeHandles = attributeHandleValues(attributes);
  if (!attributeHandles) {
    throw AttributeNotDefined(
        L"Negotiated Attribute Ownership Divestiture requires defined AttributeHandle values.");
  }
  // Preserve the original tag with the private Waiting state even though the
  // current bounded slice only sends confirmation callbacks for an already
  // pending regular acquisition. A later owner-search extension must retain
  // this exact 2025 tag for Request Attribute Ownership Assumption.
  auto copiedTag = copyVariableLengthDataBytes(userSuppliedTag);
  // Section 7.3.1 supplies the object instance designator, set of attribute
  // designators, and user-supplied tag. Build the file-text forms before the
  // registry commits the private Waiting state, so formatting cannot leave an
  // accepted request without its selected service-report record. The Table 5
  // tag literal stays confined to private file reporting (RL-077), not a
  // future public MOM-interaction representation.
  VariableLengthData const reportTag(userSuppliedTag);
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::object_instance_handle,
       L"Object instance designator",
       umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
      {umbra::detail::MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       umbra::detail::formatMomAttributeHandleSet(attributes)},
      {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       umbra::detail::formatMomUserSuppliedTag(reportTag)},
  };

  std::wstring federationName;
  std::vector<umbra::detail::AttributeOwnershipAcquisitionWorkItem> workItems;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Negotiated Attribute Ownership Divestiture");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Negotiated Attribute Ownership Divestiture requires federation membership.");
    }
    federationName = *joinedFederationName_;
    auto const divestingFederateId = *joinedFederateId_;
    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(federationName, divestingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto plan = registry.planNegotiatedAttributeOwnershipDivestiture(
        federationName,
        divestingFederateId,
        *objectInstanceHandle,
        *attributeHandles,
        std::move(copiedTag));
    if (plan.status != umbra::detail::NegotiatedAttributeOwnershipDivestitureStatus::applied) {
      throwNegotiatedAttributeOwnershipDivestitureFailure(plan.status);
    }
    // The accepted Section 7.3 invocation is the service-report boundary.
    // Its Request Divestiture Confirmation work is separately queued after
    // this record, preserving the report before the later callback path.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"NegotiatedAttributeOwnershipDivestiture",
        umbra::detail::MomServiceType::ownership_management,
        reportArguments);
    workItems = std::move(plan.workItems);
  }

  queueAttributeOwnershipAcquisitionWorkItems(std::move(workItems), federationName);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Negotiated Attribute Ownership Divestiture", exception);
    throw;
  }
}

void UmbraRtiAmbassador::confirmDivestiture(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandleSet const& confirmedAttributes,
    VariableLengthData const& userSuppliedTag) {
  auto instrumentationScope = beginRtiCall("confirmDivestiture");
  try {
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Confirm Divestiture");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(L"Confirm Divestiture requires federation membership.");
    }
    if (!embeddedFederationManagement().registry().memberById(
            *joinedFederationName_,
            *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
  }

  auto const objectInstanceHandle = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceHandle) {
    throw ObjectInstanceNotKnown(L"Confirm Divestiture requires a known ObjectInstanceHandle.");
  }
  auto const attributeHandles = attributeHandleValues(confirmedAttributes);
  if (!attributeHandles) {
    throw AttributeNotDefined(L"Confirm Divestiture requires defined AttributeHandle values.");
  }
  auto copiedTag = copyVariableLengthDataBytes(userSuppliedTag);
  // Section 7.6.1 supplies the object instance designator, set of attribute
  // designators, and user-supplied tag. Keep the Table 5 type-63 form private
  // to the selected filesystem record (RL-077), rather than projecting it to
  // a future public MOM interaction.
  VariableLengthData const reportTag(userSuppliedTag);
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::object_instance_handle,
       L"Object instance designator",
       umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
      {umbra::detail::MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       umbra::detail::formatMomAttributeHandleSet(confirmedAttributes)},
      {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       umbra::detail::formatMomUserSuppliedTag(reportTag)},
  };

  std::wstring federationName;
  umbra::detail::ConfirmDivestiturePlan plan;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Confirm Divestiture");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(L"Confirm Divestiture requires federation membership.");
    }
    federationName = *joinedFederationName_;
    auto const divestingFederateId = *joinedFederateId_;
    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(federationName, divestingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    plan = registry.planConfirmDivestiture(
        federationName,
        divestingFederateId,
        *objectInstanceHandle,
        *attributeHandles,
        std::move(copiedTag));
    if (plan.status != umbra::detail::ConfirmDivestitureStatus::applied) {
      throwConfirmDivestitureFailure(plan.status);
    }
    // The successful Section 7.6 invocation is the service-report boundary.
    // Record it after ownership transfer commits but before separately queued
    // Attribute Ownership Acquisition Notification work begins.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"ConfirmDivestiture",
        umbra::detail::MomServiceType::ownership_management,
        reportArguments);
  }

  queueConfirmDivestitureNotifications(std::move(plan.notifications), federationName);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Confirm Divestiture", exception);
    throw;
  }
}

void UmbraRtiAmbassador::cancelNegotiatedAttributeOwnershipDivestiture(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandleSet const& attributes) {
  auto instrumentationScope = beginRtiCall("cancelNegotiatedAttributeOwnershipDivestiture");
  try {
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Cancel Negotiated Attribute Ownership Divestiture");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Cancel Negotiated Attribute Ownership Divestiture requires federation membership.");
    }
    if (!embeddedFederationManagement().registry().memberById(
            *joinedFederationName_,
            *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
  }

  auto const objectInstanceHandle = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceHandle) {
    throw ObjectInstanceNotKnown(
        L"Cancel Negotiated Attribute Ownership Divestiture requires a known ObjectInstanceHandle.");
  }
  auto const attributeHandles = attributeHandleValues(attributes);
  if (!attributeHandles) {
    throw AttributeNotDefined(
        L"Cancel Negotiated Attribute Ownership Divestiture requires defined AttributeHandle values.");
  }
  // Section 7.14.1 supplies the object instance designator and set of
  // attribute designators. Preserve both source forms before the registry
  // commits the cancellation, so formatting cannot leave an accepted
  // cancellation without its selected file record.
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::object_instance_handle,
       L"Object instance designator",
       umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
      {umbra::detail::MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       umbra::detail::formatMomAttributeHandleSet(attributes)},
  };

  std::wstring federationName;
  std::vector<umbra::detail::AttributeOwnershipAcquisitionWorkItem> followupWorkItems;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Cancel Negotiated Attribute Ownership Divestiture");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Cancel Negotiated Attribute Ownership Divestiture requires federation membership.");
    }
    federationName = *joinedFederationName_;
    auto const divestingFederateId = *joinedFederateId_;
    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(federationName, divestingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto plan = registry.planCancelNegotiatedAttributeOwnershipDivestiture(
        federationName,
        divestingFederateId,
        *objectInstanceHandle,
        *attributeHandles);
    if (plan.status !=
        umbra::detail::CancelNegotiatedAttributeOwnershipDivestitureStatus::applied) {
      throwCancelNegotiatedAttributeOwnershipDivestitureFailure(plan.status);
    }
    // The accepted Section 7.14 cancellation is the service-report boundary.
    // Write it before separately queued regular-acquisition follow-up work,
    // so the selected file records the successful invocation rather than any
    // later Request Attribute Ownership Release callback it can restore.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"CancelNegotiatedAttributeOwnershipDivestiture",
        umbra::detail::MomServiceType::ownership_management,
        reportArguments);
    followupWorkItems = std::move(plan.followupWorkItems);
  }

  queueAttributeOwnershipAcquisitionWorkItems(std::move(followupWorkItems), federationName);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Cancel Negotiated Attribute Ownership Divestiture", exception);
    throw;
  }
}

void UmbraRtiAmbassador::unconditionalAttributeOwnershipDivestiture(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandleSet const& attributes,
    VariableLengthData const& userSuppliedTag) {
  auto instrumentationScope = beginRtiCall("unconditionalAttributeOwnershipDivestiture");
  try {
  // Preserve the official connection and membership preconditions before
  // caller-supplied handle validation, matching the adjacent ownership
  // services. Save/restore has no implemented state in this profile.
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Unconditional Attribute Ownership Divestiture");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Unconditional Attribute Ownership Divestiture requires federation membership.");
    }
    if (!embeddedFederationManagement().registry().memberById(
            *joinedFederationName_,
            *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
  }

  auto const objectInstanceHandle = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceHandle) {
    throw ObjectInstanceNotKnown(
        L"Unconditional Attribute Ownership Divestiture requires a known ObjectInstanceHandle.");
  }
  auto const attributeHandles = attributeHandleValues(attributes);
  if (!attributeHandles) {
    throw AttributeNotDefined(
        L"Unconditional Attribute Ownership Divestiture requires defined AttributeHandle values.");
  }
  // Copy before the registry commits the immediate unowned state, so a failed
  // allocation never leaves an ownership-assumption callback without the
  // required divestiture tag.
  VariableLengthData copiedTag = userSuppliedTag;
  // Section 7.2.1 supplies the object instance designator, set of attribute
  // designators, and user-supplied tag. Table 5 makes the first two type-37
  // and type-1 forms and the tag Binary Data. The Table 5 UserSuppliedTag type
  // literal conflicts with the bundled MIM enum, so keep this formatter scoped
  // to file reporting (RL-077), not a future live MOM interaction path.
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::object_instance_handle,
       L"Object instance designator",
       umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
      {umbra::detail::MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       umbra::detail::formatMomAttributeHandleSet(attributes)},
      {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       umbra::detail::formatMomUserSuppliedTag(userSuppliedTag)},
  };

  std::wstring federationName;
  umbra::detail::UnconditionalAttributeOwnershipDivestiturePlan plan;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Unconditional Attribute Ownership Divestiture");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Unconditional Attribute Ownership Divestiture requires federation membership.");
    }
    federationName = *joinedFederationName_;
    auto const divestingFederateId = *joinedFederateId_;
    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(federationName, divestingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    plan = registry.planUnconditionalAttributeOwnershipDivestiture(
        federationName,
        divestingFederateId,
        *objectInstanceHandle,
        *attributeHandles);
    if (plan.status !=
        umbra::detail::UnconditionalAttributeOwnershipDivestitureStatus::applied) {
      throwUnconditionalAttributeOwnershipDivestitureFailure(plan.status);
    }
    // The accepted divestiture is the service-report boundary. Any established
    // acquisition work and the later Section 7.4 ownership-assumption offers
    // are separately queued after this record has been appended.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"UnconditionalAttributeOwnershipDivestiture",
        umbra::detail::MomServiceType::ownership_management,
        reportArguments);
  }

  // Established regular requests receive their normal acquisition work first.
  // Any HLA_IMMEDIATE acquisition can therefore make an attribute owned before
  // a later assumption offer is submitted; each offer independently rechecks
  // its standard unowned/published/pending preconditions at callback entry.
  queueAttributeOwnershipAcquisitionWorkItems(
      std::move(plan.acquisitionWorkItems),
      federationName);
  queueAttributeOwnershipAssumptionRecipients(
      std::move(plan.assumptionRecipients),
      federationName,
      copiedTag);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Unconditional Attribute Ownership Divestiture", exception);
    throw;
  }
}

void UmbraRtiAmbassador::attributeOwnershipAcquisition(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandleSet const& desiredAttributes,
    VariableLengthData const& userSuppliedTag) {
  auto instrumentationScope = beginRtiCall("attributeOwnershipAcquisition");
  try {
  // Preserve the official connection and membership preconditions before
  // caller-supplied handle validation, as with the adjacent ownership
  // services. Save/restore has no implemented state in this profile.
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Attribute Ownership Acquisition");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Attribute Ownership Acquisition requires federation membership.");
    }
    if (!embeddedFederationManagement().registry().memberById(
            *joinedFederationName_,
            *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
  }

  auto const objectInstanceHandle = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceHandle) {
    throw ObjectInstanceNotKnown(
        L"Attribute Ownership Acquisition requires a known ObjectInstanceHandle.");
  }
  auto const desiredAttributeHandles = attributeHandleValues(desiredAttributes);
  if (!desiredAttributeHandles) {
    throw AttributeNotDefined(
        L"Attribute Ownership Acquisition requires defined AttributeHandle values.");
  }
  // Copy before the registry accepts the request so a failed allocation never
  // leaves a private acquisition reservation without its callback tag.
  auto copiedTag = copyVariableLengthDataBytes(userSuppliedTag);
  VariableLengthData const reportTag(userSuppliedTag);
  // Section 7.8.1 supplies the object instance designator, set of attribute
  // designators, and user-supplied tag.  Table 5 makes the first two type-37
  // and type-1 forms and depicts its Binary Data tag as type 63.  Keep that
  // source-specific literal confined to the private file record (RL-077), not
  // a future live MOM interaction path.
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::object_instance_handle,
       L"Object instance designator",
       umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
      {umbra::detail::MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       umbra::detail::formatMomAttributeHandleSet(desiredAttributes)},
      {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       umbra::detail::formatMomUserSuppliedTag(reportTag)},
  };

  std::wstring federationName;
  std::vector<umbra::detail::AttributeOwnershipAcquisitionWorkItem> workItems;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Attribute Ownership Acquisition");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Attribute Ownership Acquisition requires federation membership.");
    }
    federationName = *joinedFederationName_;
    auto const requestingFederateId = *joinedFederateId_;
    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(federationName, requestingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto plan = registry.planAttributeOwnershipAcquisition(
        federationName,
        requestingFederateId,
        *objectInstanceHandle,
        *desiredAttributeHandles,
        std::move(copiedTag));
    if (plan.status != umbra::detail::AttributeOwnershipAcquisitionStatus::applied) {
      throwAttributeOwnershipAcquisitionFailure(plan.status);
    }
    // The accepted Section 7.8 request is the service-report boundary. Write
    // it before any separately queued release or acquisition work, so the
    // selected file records this successful invocation rather than a later
    // ownership callback.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"AttributeOwnershipAcquisition",
        umbra::detail::MomServiceType::ownership_management,
        reportArguments);
    workItems = std::move(plan.workItems);
  }

  queueAttributeOwnershipAcquisitionWorkItems(std::move(workItems), federationName);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Attribute Ownership Acquisition", exception);
    throw;
  }
}

void UmbraRtiAmbassador::attributeOwnershipAcquisitionIfAvailable(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandleSet const& desiredAttributes,
    VariableLengthData const& userSuppliedTag) {
  auto instrumentationScope = beginRtiCall("attributeOwnershipAcquisitionIfAvailable");
  try {
  // Preserve the official connection and membership preconditions before
  // caller-supplied handle validation, as with the adjacent ownership
  // services. Save/restore has no implemented state in this profile.
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Attribute Ownership Acquisition If Available");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Attribute Ownership Acquisition If Available requires federation membership.");
    }
    if (!embeddedFederationManagement().registry().memberById(
            *joinedFederationName_,
            *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
  }

  auto const objectInstanceHandle = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceHandle) {
    throw ObjectInstanceNotKnown(
        L"Attribute Ownership Acquisition If Available requires a known ObjectInstanceHandle.");
  }
  auto const desiredAttributeHandles = attributeHandleValues(desiredAttributes);
  if (!desiredAttributeHandles) {
    throw AttributeNotDefined(
        L"Attribute Ownership Acquisition If Available requires defined AttributeHandle values.");
  }
  // Copy before the registry accepts the request so a failed allocation never
  // leaves a private acquisition reservation without a callback payload.
  VariableLengthData copiedTag = userSuppliedTag;
  // Section 7.9.1 supplies the object instance designator, set of attribute
  // designators, and user-supplied tag. Table 5 makes the first two type-37
  // and type-1 forms and depicts its Binary Data tag as type 63. Keep that
  // source-specific literal confined to the private file record (RL-077), not
  // a future live MOM interaction path.
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::object_instance_handle,
       L"Object instance designator",
       umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
      {umbra::detail::MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       umbra::detail::formatMomAttributeHandleSet(desiredAttributes)},
      {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       umbra::detail::formatMomUserSuppliedTag(copiedTag)},
  };

  std::wstring federationName;
  std::uint64_t requestingFederateId = 0;
  umbra::detail::AttributeOwnershipAcquisitionIfAvailablePlan plan;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Attribute Ownership Acquisition If Available");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Attribute Ownership Acquisition If Available requires federation membership.");
    }
    federationName = *joinedFederationName_;
    requestingFederateId = *joinedFederateId_;
    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(federationName, requestingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    plan = registry.planAttributeOwnershipAcquisitionIfAvailable(
        federationName,
        requestingFederateId,
        *objectInstanceHandle,
        *desiredAttributeHandles);
    if (plan.status !=
        umbra::detail::AttributeOwnershipAcquisitionIfAvailableStatus::applied) {
      throwAttributeOwnershipAcquisitionIfAvailableFailure(plan.status);
    }
    // A missing callback route means this invocation cannot complete
    // successfully. Roll back the private reservation before it can reserve a
    // report serial or append a successful-void record.
    if (plan.requestId != 0 && !plan.callbackRoute) {
      registry.cancelAttributeOwnershipAcquisitionIfAvailable(
          federationName,
          requestingFederateId,
          *objectInstanceHandle,
          plan.requestId);
      throw RTIinternalError(
          L"The embedded federation has an ownership-acquisition requester without a callback route.");
    }
    // The accepted Section 7.9 request is the service-report boundary. Write
    // it before the supplied-empty no-callback return or any separately queued
    // unavailable/acquisition callback, so the file records the successful
    // invocation rather than later ownership delivery.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"AttributeOwnershipAcquisitionIfAvailable",
        umbra::detail::MomServiceType::ownership_management,
        reportArguments);
  }

  // The empty-attribute case has no ownership transition or callback, but it
  // remains a successful supplied-empty Section 7.9 invocation and was
  // recorded above.
  if (plan.requestId == 0) {
    return;
  }

  try {
    // Do not hold the requester lock while submitting a route: HLA_IMMEDIATE
    // may synchronously establish ownership and enter its standard callbacks.
    queueAttributeOwnershipAcquisitionIfAvailableReport(
        std::move(plan.callbackRoute),
        federationName,
        requestingFederateId,
        *objectInstanceHandle,
        plan.requestId,
        std::move(copiedTag));
  } catch (...) {
    std::scoped_lock lock(federationManagementMutex());
    embeddedFederationManagement().registry().cancelAttributeOwnershipAcquisitionIfAvailable(
        federationName,
        requestingFederateId,
        *objectInstanceHandle,
        plan.requestId);
    throw;
  }
  } catch (Exception const& exception) {
    emitExceptionReport(L"Attribute Ownership Acquisition If Available", exception);
    throw;
  }
}

void UmbraRtiAmbassador::attributeOwnershipReleaseDenied(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandleSet const& attributes,
    VariableLengthData const& userSuppliedTag) {
  auto instrumentationScope = beginRtiCall("attributeOwnershipReleaseDenied");
  try {
  // Preserve the official connection and membership preconditions before
  // caller-supplied handle validation, matching regular acquisition.
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Attribute Ownership Release Denied");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Attribute Ownership Release Denied requires federation membership.");
    }
    if (!embeddedFederationManagement().registry().memberById(
            *joinedFederationName_,
            *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
  }

  auto const objectInstanceHandle = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceHandle) {
    throw ObjectInstanceNotKnown(
        L"Attribute Ownership Release Denied requires a known ObjectInstanceHandle.");
  }
  auto const attributeHandles = attributeHandleValues(attributes);
  if (!attributeHandles) {
    throw AttributeNotDefined(
        L"Attribute Ownership Release Denied requires defined AttributeHandle values.");
  }
  // Copy before the registry terminates pending acquisitions so a failed
  // allocation cannot leave their required unavailable callbacks tagless.
  VariableLengthData copiedTag = userSuppliedTag;
  // Section 7.12.1 supplies the object instance designator, the set of
  // attribute designators for which the joined federate is unwilling to divest
  // ownership, and the user-supplied tag. Table 5 makes the first two type-37
  // and type-1 forms and depicts its Binary Data tag as type 63. Keep that
  // source-specific literal confined to the private file record (RL-077), not
  // a future live MOM interaction path.
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::object_instance_handle,
       L"Object instance designator",
       umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
      {umbra::detail::MomArgumentType::attribute_handle_set,
       L"Set of attribute designators for which the joined federate is unwilling to divest ownership",
       umbra::detail::formatMomAttributeHandleSet(attributes)},
      {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       umbra::detail::formatMomUserSuppliedTag(copiedTag)},
  };

  std::wstring federationName;
  std::vector<umbra::detail::AttributeOwnershipUnavailableRecipient> recipients;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Attribute Ownership Release Denied");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Attribute Ownership Release Denied requires federation membership.");
    }
    federationName = *joinedFederationName_;
    auto const owningFederateId = *joinedFederateId_;
    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(federationName, owningFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto plan = registry.planAttributeOwnershipReleaseDenied(
        federationName,
        owningFederateId,
        *objectInstanceHandle,
        *attributeHandles);
    if (plan.status != umbra::detail::AttributeOwnershipReleaseDeniedStatus::applied) {
      throwAttributeOwnershipReleaseDeniedFailure(plan.status);
    }
    // The accepted Section 7.12 denial is the service-report boundary. Write
    // it before any separately queued Attribute Ownership Unavailable
    // callbacks, so the selected file records the successful invocation rather
    // than downstream acquisition termination delivery.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"AttributeOwnershipReleaseDenied",
        umbra::detail::MomServiceType::ownership_management,
        reportArguments);
    recipients = std::move(plan.recipients);
  }

  queueAttributeOwnershipUnavailableRecipients(
      std::move(recipients),
      federationName,
      copiedTag);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Attribute Ownership Release Denied", exception);
    throw;
  }
}

void UmbraRtiAmbassador::attributeOwnershipDivestitureIfWanted(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandleSet const& attributes,
    VariableLengthData const& userSuppliedTag,
    AttributeHandleSet& divestedAttributes) {
  auto instrumentationScope = beginRtiCall("attributeOwnershipDivestitureIfWanted");
  try {
  // Preserve the official connection and membership preconditions before
  // caller-supplied handle validation, matching the adjacent ownership
  // services. Save/restore has no implemented state in this profile.
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Attribute Ownership Divestiture If Wanted");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Attribute Ownership Divestiture If Wanted requires federation membership.");
    }
    if (!embeddedFederationManagement().registry().memberById(
            *joinedFederationName_,
            *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
  }

  auto const objectInstanceHandle = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceHandle) {
    throw ObjectInstanceNotKnown(
        L"Attribute Ownership Divestiture If Wanted requires a known ObjectInstanceHandle.");
  }
  auto const attributeHandles = attributeHandleValues(attributes);
  if (!attributeHandles) {
    throw AttributeNotDefined(
        L"Attribute Ownership Divestiture If Wanted requires defined AttributeHandle values.");
  }
  // Copy before the registry commits the synchronous transfer, so an
  // allocation failure cannot leave a notification without its required
  // divestiture tag.
  auto copiedTag = copyVariableLengthDataBytes(userSuppliedTag);

  std::wstring federationName;
  umbra::detail::AttributeOwnershipDivestitureIfWantedPlan plan;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Attribute Ownership Divestiture If Wanted");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Attribute Ownership Divestiture If Wanted requires federation membership.");
    }
    federationName = *joinedFederationName_;
    auto const divestingFederateId = *joinedFederateId_;
    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(federationName, divestingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    plan = registry.planAttributeOwnershipDivestitureIfWanted(
        federationName,
        divestingFederateId,
        *objectInstanceHandle,
        *attributeHandles,
        std::move(copiedTag));
    if (plan.status != umbra::detail::AttributeOwnershipDivestitureIfWantedStatus::applied) {
      throwAttributeOwnershipDivestitureIfWantedFailure(plan.status);
    }
  }

  // The binding's out parameter reports exactly the subset that transferred
  // synchronously. It is assigned before routes are submitted because an
  // HLA_IMMEDIATE notification may enter user code before this service returns.
  AttributeHandleSet resolvedDivestedAttributes;
  for (std::uint64_t const attributeHandle : plan.divestedAttributeHandles) {
    resolvedDivestedAttributes.insert(makeAttributeHandle(attributeHandle));
  }
  divestedAttributes = std::move(resolvedDivestedAttributes);

  queueAttributeOwnershipDivestitureIfWantedNotifications(
      std::move(plan.notifications),
      federationName);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Attribute Ownership Divestiture If Wanted", exception);
    throw;
  }
}

void UmbraRtiAmbassador::cancelAttributeOwnershipAcquisition(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandleSet const& attributes) {
  auto instrumentationScope = beginRtiCall("cancelAttributeOwnershipAcquisition");
  try {
  // Preserve the official connection and membership preconditions before
  // caller-supplied handle validation, matching the regular acquisition path.
  // Save/restore has no implemented state in this profile.
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Cancel Attribute Ownership Acquisition");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Cancel Attribute Ownership Acquisition requires federation membership.");
    }
    if (!embeddedFederationManagement().registry().memberById(
            *joinedFederationName_,
            *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
  }

  auto const objectInstanceHandle = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceHandle) {
    throw ObjectInstanceNotKnown(
        L"Cancel Attribute Ownership Acquisition requires a known ObjectInstanceHandle.");
  }
  auto const attributeHandles = attributeHandleValues(attributes);
  if (!attributeHandles) {
    throw AttributeNotDefined(
        L"Cancel Attribute Ownership Acquisition requires defined AttributeHandle values.");
  }
  // Section 7.15.1 supplies the object instance designator and the set of
  // attribute designators.  Preserve both source forms before the registry
  // commits the cancellation, so formatting cannot leave an accepted
  // cancellation without its selected file record.
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::object_instance_handle,
       L"Object instance designator",
       umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
      {umbra::detail::MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       umbra::detail::formatMomAttributeHandleSet(attributes)},
  };

  std::wstring federationName;
  std::uint64_t requestingFederateId = 0;
  umbra::detail::AttributeOwnershipAcquisitionCancellationPlan plan;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Cancel Attribute Ownership Acquisition");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Cancel Attribute Ownership Acquisition requires federation membership.");
    }
    federationName = *joinedFederationName_;
    requestingFederateId = *joinedFederateId_;
    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(federationName, requestingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    plan = registry.planAttributeOwnershipAcquisitionCancellation(
        federationName,
        requestingFederateId,
        *objectInstanceHandle,
        *attributeHandles);
    if (plan.status !=
        umbra::detail::AttributeOwnershipAcquisitionCancellationStatus::applied) {
      throwAttributeOwnershipAcquisitionCancellationFailure(plan.status);
    }
    // The accepted Section 7.15 cancellation is the service-report boundary.
    // Write it before either the supplied-empty no-callback return or the
    // separately queued confirmation callback, so the report describes the
    // successful invocation rather than downstream notification delivery.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"CancelAttributeOwnershipAcquisition",
        umbra::detail::MomServiceType::ownership_management,
        reportArguments);
  }

  // An empty attribute set has no cancellation transition or callback, but it
  // remains a successful supplied-empty Section 7.15 invocation and was
  // recorded above.
  if (plan.cancellationId == 0) {
    return;
  }
  queueAttributeOwnershipAcquisitionCancellationConfirmation(
      std::move(plan.callbackRoute),
      federationName,
      requestingFederateId,
      *objectInstanceHandle,
      plan.cancellationId,
      std::move(plan.attributeHandles));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Cancel Attribute Ownership Acquisition", exception);
    throw;
  }
}

ObjectClassHandle UmbraRtiAmbassador::getKnownObjectClassHandle(
    ObjectInstanceHandle const& objectInstance) {
  auto instrumentationScope = beginRtiCall("getKnownObjectClassHandle");
  try {
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
  auto const objectInstanceHandle = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceHandle) {
    throw ObjectInstanceNotKnown(
        L"The supplied ObjectInstanceHandle is not known to this federate.");
  }
  auto const known = registry.knownObjectInstanceFor(
      *joinedFederationName_,
      *joinedFederateId_,
      *objectInstanceHandle);
  if (!known) {
    throw ObjectInstanceNotKnown(
        L"The supplied ObjectInstanceHandle is not known to this federate.");
  }
  return makeObjectClassHandle(known->knownObjectClassHandle);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Known Object Class Handle", exception);
    throw;
  }
}

ObjectInstanceHandle UmbraRtiAmbassador::getObjectInstanceHandle(
    std::wstring const& objectInstanceName) {
  auto instrumentationScope = beginRtiCall("getObjectInstanceHandle");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Object Instance Handle requires membership in a federation execution.");
  }

  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  auto const known = registry.knownObjectInstanceByNameFor(
      *joinedFederationName_,
      *joinedFederateId_,
      objectInstanceName);
  if (!known) {
    throw ObjectInstanceNotKnown(
        L"The supplied object instance name is not known to this federate.");
  }
  return makeObjectInstanceHandle(known->objectInstanceHandle);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Object Instance Handle", exception);
    throw;
  }
}

std::wstring UmbraRtiAmbassador::getObjectInstanceName(
    ObjectInstanceHandle const& objectInstance) {
  auto instrumentationScope = beginRtiCall("getObjectInstanceName");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Object Instance Name requires membership in a federation execution.");
  }

  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  auto const objectInstanceHandle = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceHandle) {
    throw ObjectInstanceNotKnown(
        L"The supplied ObjectInstanceHandle is not known to this federate.");
  }
  auto const known = registry.knownObjectInstanceFor(
      *joinedFederationName_,
      *joinedFederateId_,
      *objectInstanceHandle);
  if (!known) {
    throw ObjectInstanceNotKnown(
        L"The supplied ObjectInstanceHandle is not known to this federate.");
  }
  return known->objectInstanceName;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Object Instance Name", exception);
    throw;
  }
}

InteractionClassHandle UmbraRtiAmbassador::getInteractionClassHandle(
    std::wstring const& interactionClassName) {
  auto instrumentationScope = beginRtiCall("getInteractionClassHandle");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Interaction Class Handle requires membership in a federation execution.");
  }

  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }

  auto const encodedName = umbra::detail::utf8FromWide(interactionClassName);
  if (!encodedName) {
    throw NameNotFound(L"The supplied interaction class name is not valid UTF-8 text.");
  }
  auto const handle = registry.interactionClassHandleFor(*joinedFederationName_, *encodedName);
  if (!handle) {
    throw NameNotFound(
        L"The supplied interaction class name is not defined in this federation execution.");
  }
  return makeInteractionClassHandle(*handle);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Interaction Class Handle", exception);
    throw;
  }
}

std::wstring UmbraRtiAmbassador::getInteractionClassName(
    InteractionClassHandle const& interactionClass) {
  auto instrumentationScope = beginRtiCall("getInteractionClassName");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Interaction Class Name requires membership in a federation execution.");
  }

  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }

  auto const handle = interactionClassHandleValue(interactionClass);
  if (!handle) {
    throw InvalidInteractionClassHandle(
        L"Get Interaction Class Name requires a valid InteractionClassHandle.");
  }
  auto const encodedName = registry.interactionClassNameFor(*joinedFederationName_, *handle);
  if (!encodedName) {
    throw InvalidInteractionClassHandle(
        L"The supplied InteractionClassHandle is not known in this federation execution.");
  }
  auto const interactionClassName = umbra::detail::wideFromUtf8(*encodedName);
  if (!interactionClassName) {
    throw RTIinternalError(
        L"The embedded federation stored an interaction class name that is not valid UTF-8 text.");
  }
  return *interactionClassName;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Interaction Class Name", exception);
    throw;
  }
}

ParameterHandle UmbraRtiAmbassador::getParameterHandle(
    InteractionClassHandle const& interactionClass,
    std::wstring const& parameterName) {
  auto instrumentationScope = beginRtiCall("getParameterHandle");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Parameter Handle requires membership in a federation execution.");
  }

  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }

  auto const interactionClassHandle = interactionClassHandleValue(interactionClass);
  if (!interactionClassHandle) {
    throw InvalidInteractionClassHandle(
        L"Get Parameter Handle requires a valid InteractionClassHandle.");
  }
  auto const encodedInteractionClassName = registry.interactionClassNameFor(
      *joinedFederationName_,
      *interactionClassHandle);
  if (!encodedInteractionClassName) {
    throw InvalidInteractionClassHandle(
        L"The supplied InteractionClassHandle is not known in this federation execution.");
  }

  auto const encodedParameterName = umbra::detail::utf8FromWide(parameterName);
  if (!encodedParameterName) {
    throw NameNotFound(L"The supplied parameter name is not valid UTF-8 text.");
  }
  auto const parameterHandle = registry.parameterHandleFor(
      *joinedFederationName_,
      *encodedInteractionClassName,
      *encodedParameterName);
  if (!parameterHandle) {
    throw NameNotFound(L"The supplied parameter name is not defined for this interaction class.");
  }
  return makeParameterHandle(*parameterHandle);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Parameter Handle", exception);
    throw;
  }
}

std::wstring UmbraRtiAmbassador::getParameterName(
    InteractionClassHandle const& interactionClass,
    ParameterHandle const& parameter) {
  auto instrumentationScope = beginRtiCall("getParameterName");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Parameter Name requires membership in a federation execution.");
  }

  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }

  auto const interactionClassHandle = interactionClassHandleValue(interactionClass);
  if (!interactionClassHandle) {
    throw InvalidInteractionClassHandle(
        L"Get Parameter Name requires a valid InteractionClassHandle.");
  }
  auto const encodedInteractionClassName = registry.interactionClassNameFor(
      *joinedFederationName_,
      *interactionClassHandle);
  if (!encodedInteractionClassName) {
    throw InvalidInteractionClassHandle(
        L"The supplied InteractionClassHandle is not known in this federation execution.");
  }

  auto const parameterHandle = parameterHandleValue(parameter);
  if (!parameterHandle) {
    throw InvalidParameterHandle(L"Get Parameter Name requires a valid ParameterHandle.");
  }
  auto const encodedParameterName = registry.parameterNameFor(
      *joinedFederationName_,
      *encodedInteractionClassName,
      *parameterHandle);
  if (!encodedParameterName) {
    throw InteractionParameterNotDefined(
        L"The supplied ParameterHandle is not defined for this interaction class.");
  }
  auto const parameterName = umbra::detail::wideFromUtf8(*encodedParameterName);
  if (!parameterName) {
    throw RTIinternalError(
        L"The embedded federation stored a parameter name that is not valid UTF-8 text.");
  }
  return *parameterName;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Parameter Name", exception);
    throw;
  }
}

void UmbraRtiAmbassador::publishInteractionClass(
    InteractionClassHandle const& interactionClass) {
  auto instrumentationScope = beginRtiCall("publishInteractionClass");
  try {
  std::vector<umbra::detail::DeclarationAdvisory> declarationAdvisories;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Publish Interaction Class");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Publish Interaction Class requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const handle = interactionClassHandleValue(interactionClass);
    if (!handle) {
      throw InteractionClassNotDefined(
          L"Publish Interaction Class requires a defined InteractionClassHandle.");
    }
    auto const result = registry.setInteractionClassPublication(
        *joinedFederationName_,
        *joinedFederateId_,
        *handle,
        true);
    if (result != umbra::detail::InteractionClassDeclarationStatus::applied) {
      throwInteractionClassDeclarationFailure(result);
    }
    // The accepted §5.4 declaration transition is the service-report boundary.
    // Any declaration advisories remain separately queued only after its
    // successful-void record has been written.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"PublishInteractionClass",
        umbra::detail::MomServiceType::declaration_management,
        {{umbra::detail::MomArgumentType::interaction_class_handle,
          L"Interaction class designator",
          umbra::detail::formatMomInteractionClassHandle(interactionClass)}});
    declarationAdvisories = registry.planDeclarationAdvisories(*joinedFederationName_);
  }
  queueDeclarationAdvisories(std::move(declarationAdvisories));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Publish Interaction Class", exception);
    throw;
  }
}

void UmbraRtiAmbassador::unpublishInteractionClass(
    InteractionClassHandle const& interactionClass) {
  auto instrumentationScope = beginRtiCall("unpublishInteractionClass");
  try {
  std::vector<umbra::detail::DeclarationAdvisory> declarationAdvisories;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Unpublish Interaction Class");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Unpublish Interaction Class requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const handle = interactionClassHandleValue(interactionClass);
    if (!handle) {
      throw InteractionClassNotDefined(
          L"Unpublish Interaction Class requires a defined InteractionClassHandle.");
    }
    auto const result = registry.setInteractionClassPublication(
        *joinedFederationName_,
        *joinedFederateId_,
        *handle,
        false);
    if (result != umbra::detail::InteractionClassDeclarationStatus::applied) {
      throwInteractionClassDeclarationFailure(result);
    }
    // The accepted §5.5 declaration transition is the service-report boundary.
    // Any declaration advisories remain separately queued only after its
    // successful-void record has been written.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"UnpublishInteractionClass",
        umbra::detail::MomServiceType::declaration_management,
        {{umbra::detail::MomArgumentType::interaction_class_handle,
          L"Interaction class designator",
          umbra::detail::formatMomInteractionClassHandle(interactionClass)}});
    declarationAdvisories = registry.planDeclarationAdvisories(*joinedFederationName_);
  }
  queueDeclarationAdvisories(std::move(declarationAdvisories));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Unpublish Interaction Class", exception);
    throw;
  }
}

void UmbraRtiAmbassador::publishObjectClassDirectedInteractions(
    ObjectClassHandle const& objectClass,
    InteractionClassHandleSet const& interactionClasses) {
  auto instrumentationScope = beginRtiCall("publishObjectClassDirectedInteractions");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  requireFederationServiceOperationAvailable(
      L"Publish Object Class Directed Interactions");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Publish Object Class Directed Interactions requires membership in a federation execution.");
  }
  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  auto const objectClassHandleValueResult = objectClassHandleValue(objectClass);
  if (!objectClassHandleValueResult) {
    throw ObjectClassNotDefined(
        L"Publish Object Class Directed Interactions requires a defined ObjectClassHandle.");
  }
  auto const interactionClassHandleValuesResult =
      interactionClassHandleValues(interactionClasses);
  if (!interactionClassHandleValuesResult) {
    throw InteractionClassNotDefined(
        L"Publish Object Class Directed Interactions requires defined InteractionClassHandle values.");
  }
  auto const result = registry.publishObjectClassDirectedInteractions(
      *joinedFederationName_,
      *joinedFederateId_,
      *objectClassHandleValueResult,
      *interactionClassHandleValuesResult);
  if (result != umbra::detail::DirectedInteractionDeclarationStatus::applied) {
    throwDirectedInteractionDeclarationFailure(
        result,
        L"Publish Object Class Directed Interactions");
  }
  // The accepted §5.6 declaration transition is the service-report boundary.
  // An empty interaction-class set is still a successful invocation (and adds
  // no publications), so it must retain its supplied Array<InteractionClassHandle>
  // report argument rather than being treated as an absent argument.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"PublishObjectClassDirectedInteractions",
      umbra::detail::MomServiceType::declaration_management,
      {{umbra::detail::MomArgumentType::object_class_handle,
        L"Object class designator",
        umbra::detail::formatMomObjectClassHandle(objectClass)},
       {umbra::detail::MomArgumentType::interaction_class_handle_set,
        L"Set of interaction class designators",
        umbra::detail::formatMomInteractionClassHandleSet(interactionClasses)}});
  } catch (Exception const& exception) {
    emitExceptionReport(L"Publish Object Class Directed Interactions", exception);
    throw;
  }
}

void UmbraRtiAmbassador::unpublishObjectClassDirectedInteractions(
    ObjectClassHandle const& objectClass) {
  auto instrumentationScope = beginRtiCall("unpublishObjectClassDirectedInteractions");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  requireFederationServiceOperationAvailable(
      L"Unpublish Object Class Directed Interactions");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Unpublish Object Class Directed Interactions requires membership in a federation execution.");
  }
  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  auto const objectClassHandleValueResult = objectClassHandleValue(objectClass);
  if (!objectClassHandleValueResult) {
    throw ObjectClassNotDefined(
        L"Unpublish Object Class Directed Interactions requires a defined ObjectClassHandle.");
  }
  auto const result = registry.unpublishObjectClassDirectedInteractions(
      *joinedFederationName_,
      *joinedFederateId_,
      *objectClassHandleValueResult,
      std::nullopt);
  if (result != umbra::detail::DirectedInteractionDeclarationStatus::applied) {
    throwDirectedInteractionDeclarationFailure(
        result,
        L"Unpublish Object Class Directed Interactions");
  }
  // The C++ whole-class overload leaves the standards narrative's optional set
  // of interaction class designators unused. Preserve that required report
  // position as Null, rather than conflating it with a supplied-empty set.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"UnpublishObjectClassDirectedInteractions",
      umbra::detail::MomServiceType::declaration_management,
      {{umbra::detail::MomArgumentType::object_class_handle,
        L"Object class designator",
        umbra::detail::formatMomObjectClassHandle(objectClass)},
       {umbra::detail::MomArgumentType::null_value,
        L"Optional set of interaction class designators",
        umbra::detail::formatMomNull()}});
  } catch (Exception const& exception) {
    emitExceptionReport(L"Unpublish Object Class Directed Interactions", exception);
    throw;
  }
}

void UmbraRtiAmbassador::unpublishObjectClassDirectedInteractions(
    ObjectClassHandle const& objectClass,
    InteractionClassHandleSet const& interactionClasses) {
  auto instrumentationScope = beginRtiCall("unpublishObjectClassDirectedInteractions");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  requireFederationServiceOperationAvailable(
      L"Unpublish Object Class Directed Interactions");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Unpublish Object Class Directed Interactions requires membership in a federation execution.");
  }
  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  auto const objectClassHandleValueResult = objectClassHandleValue(objectClass);
  if (!objectClassHandleValueResult) {
    throw ObjectClassNotDefined(
        L"Unpublish Object Class Directed Interactions requires a defined ObjectClassHandle.");
  }
  auto const interactionClassHandleValuesResult =
      interactionClassHandleValues(interactionClasses);
  if (!interactionClassHandleValuesResult) {
    throw InteractionClassNotDefined(
        L"Unpublish Object Class Directed Interactions requires defined InteractionClassHandle values.");
  }
  auto const result = registry.unpublishObjectClassDirectedInteractions(
      *joinedFederationName_,
      *joinedFederateId_,
      *objectClassHandleValueResult,
      *interactionClassHandleValuesResult);
  if (result != umbra::detail::DirectedInteractionDeclarationStatus::applied) {
    throwDirectedInteractionDeclarationFailure(
        result,
        L"Unpublish Object Class Directed Interactions");
  }
  // Unlike the whole-class overload, this entry point supplies the optional
  // interaction-class set, including an explicitly supplied empty set. Write
  // the successful-void record only after the registry accepts the §5.7
  // declaration transition.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"UnpublishObjectClassDirectedInteractions",
      umbra::detail::MomServiceType::declaration_management,
      {{umbra::detail::MomArgumentType::object_class_handle,
        L"Object class designator",
        umbra::detail::formatMomObjectClassHandle(objectClass)},
       {umbra::detail::MomArgumentType::interaction_class_handle_set,
        L"Optional set of interaction class designators",
        umbra::detail::formatMomInteractionClassHandleSet(interactionClasses)}});
  } catch (Exception const& exception) {
    emitExceptionReport(L"Unpublish Object Class Directed Interactions", exception);
    throw;
  }
}

void UmbraRtiAmbassador::subscribeInteractionClass(
    InteractionClassHandle const& interactionClass,
    bool active) {
  auto instrumentationScope = beginRtiCall("subscribeInteractionClass");
  try {
  std::vector<umbra::detail::DeclarationAdvisory> declarationAdvisories;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Subscribe Interaction Class");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Subscribe Interaction Class requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const handle = interactionClassHandleValue(interactionClass);
    if (!handle) {
      throw InteractionClassNotDefined(
          L"Subscribe Interaction Class requires a defined InteractionClassHandle.");
    }
    auto const result = registry.setInteractionClassSubscription(
        *joinedFederationName_,
        *joinedFederateId_,
        *handle,
        active);
    if (result != umbra::detail::InteractionClassDeclarationStatus::applied) {
      throwInteractionClassDeclarationFailure(result);
    }
    // The accepted §5.10 subscription transition is the service-report
    // boundary. The C++ binding names its selector `active`, while the
    // standards-facing supplied argument is the optional *passive*
    // subscription indicator, so the reported Boolean is its inverse.
    // Declaration advisories remain separately queued only after the
    // successful-void record has been written.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"SubscribeInteractionClass",
        umbra::detail::MomServiceType::declaration_management,
        {{umbra::detail::MomArgumentType::interaction_class_handle,
          L"Interaction class designator",
          umbra::detail::formatMomInteractionClassHandle(interactionClass)},
         {umbra::detail::MomArgumentType::boolean,
          L"Optional passive subscription indicator",
          umbra::detail::formatMomBoolean(!active)}});
    declarationAdvisories = registry.planDeclarationAdvisories(*joinedFederationName_);
  }
  queueDeclarationAdvisories(std::move(declarationAdvisories));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Subscribe Interaction Class", exception);
    throw;
  }
}

void UmbraRtiAmbassador::unsubscribeInteractionClass(
    InteractionClassHandle const& interactionClass) {
  auto instrumentationScope = beginRtiCall("unsubscribeInteractionClass");
  try {
  std::vector<umbra::detail::DeclarationAdvisory> declarationAdvisories;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Unsubscribe Interaction Class");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Unsubscribe Interaction Class requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const handle = interactionClassHandleValue(interactionClass);
    if (!handle) {
      throw InteractionClassNotDefined(
          L"Unsubscribe Interaction Class requires a defined InteractionClassHandle.");
    }
    auto const result = registry.setInteractionClassSubscription(
        *joinedFederationName_,
        *joinedFederateId_,
        *handle,
        std::nullopt);
    if (result != umbra::detail::InteractionClassDeclarationStatus::applied) {
      throwInteractionClassDeclarationFailure(result);
    }
    // The accepted §5.11 unsubscription transition is the service-report
    // boundary. Any declaration advisories remain separately queued only after
    // its successful-void record has been written.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"UnsubscribeInteractionClass",
        umbra::detail::MomServiceType::declaration_management,
        {{umbra::detail::MomArgumentType::interaction_class_handle,
          L"Interaction class designator",
          umbra::detail::formatMomInteractionClassHandle(interactionClass)}});
    declarationAdvisories = registry.planDeclarationAdvisories(*joinedFederationName_);
  }
  queueDeclarationAdvisories(std::move(declarationAdvisories));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Unsubscribe Interaction Class", exception);
    throw;
  }
}

void UmbraRtiAmbassador::subscribeObjectClassDirectedInteractions(
    ObjectClassHandle const& objectClass,
    InteractionClassHandleSet const& interactionClasses,
    bool universally) {
  auto instrumentationScope = beginRtiCall("subscribeObjectClassDirectedInteractions");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  requireFederationServiceOperationAvailable(
      L"Subscribe Object Class Directed Interactions");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Subscribe Object Class Directed Interactions requires membership in a federation execution.");
  }
  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  auto const objectClassHandleValueResult = objectClassHandleValue(objectClass);
  if (!objectClassHandleValueResult) {
    throw ObjectClassNotDefined(
        L"Subscribe Object Class Directed Interactions requires a defined ObjectClassHandle.");
  }
  auto const interactionClassHandleValuesResult =
      interactionClassHandleValues(interactionClasses);
  if (!interactionClassHandleValuesResult) {
    throw InteractionClassNotDefined(
        L"Subscribe Object Class Directed Interactions requires defined InteractionClassHandle values.");
  }
  auto const result = registry.subscribeObjectClassDirectedInteractions(
      *joinedFederationName_,
      *joinedFederateId_,
      *objectClassHandleValueResult,
      *interactionClassHandleValuesResult,
      universally);
  if (result != umbra::detail::DirectedInteractionDeclarationStatus::applied) {
    throwDirectedInteractionDeclarationFailure(
        result,
        L"Subscribe Object Class Directed Interactions");
  }
  // The accepted §5.12 subscription transition is the service-report
  // boundary. The public C++ binding represents the optional universal
  // subscription indicator as its effective defaulted Boolean selector, so a
  // call that relies on the binding default records `false` (by ownership) and
  // an explicit universal subscription records `true`. A supplied empty set
  // remains a real type-28 argument; it is not an absent argument.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"SubscribeObjectClassDirectedInteractions",
      umbra::detail::MomServiceType::declaration_management,
      {{umbra::detail::MomArgumentType::object_class_handle,
        L"Object class designator",
        umbra::detail::formatMomObjectClassHandle(objectClass)},
       {umbra::detail::MomArgumentType::interaction_class_handle_set,
        L"Set of directed interaction designators",
        umbra::detail::formatMomInteractionClassHandleSet(interactionClasses)},
       {umbra::detail::MomArgumentType::boolean,
        L"Optional universal subscription indicator",
        umbra::detail::formatMomBoolean(universally)}});
  } catch (Exception const& exception) {
    emitExceptionReport(L"Subscribe Object Class Directed Interactions", exception);
    throw;
  }
}

void UmbraRtiAmbassador::unsubscribeObjectClassDirectedInteractions(
    ObjectClassHandle const& objectClass) {
  auto instrumentationScope = beginRtiCall("unsubscribeObjectClassDirectedInteractions");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  requireFederationServiceOperationAvailable(
      L"Unsubscribe Object Class Directed Interactions");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Unsubscribe Object Class Directed Interactions requires membership in a federation execution.");
  }
  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  auto const objectClassHandleValueResult = objectClassHandleValue(objectClass);
  if (!objectClassHandleValueResult) {
    throw ObjectClassNotDefined(
        L"Unsubscribe Object Class Directed Interactions requires a defined ObjectClassHandle.");
  }
  auto const result = registry.unsubscribeObjectClassDirectedInteractions(
      *joinedFederationName_,
      *joinedFederateId_,
      *objectClassHandleValueResult,
      std::nullopt);
  if (result != umbra::detail::DirectedInteractionDeclarationStatus::applied) {
    throwDirectedInteractionDeclarationFailure(
        result,
        L"Unsubscribe Object Class Directed Interactions");
  }
  // The official C++ whole-class overload omits the standards-facing optional
  // set. Preserve that required Table 5 slot as Null rather than conflating it
  // with the supplied-empty InteractionClassHandleSet form.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"UnsubscribeObjectClassDirectedInteractions",
      umbra::detail::MomServiceType::declaration_management,
      {{umbra::detail::MomArgumentType::object_class_handle,
        L"Object class designator",
        umbra::detail::formatMomObjectClassHandle(objectClass)},
       {umbra::detail::MomArgumentType::null_value,
        L"Optional set of directed interaction designators",
        umbra::detail::formatMomNull()}});
  } catch (Exception const& exception) {
    emitExceptionReport(L"Unsubscribe Object Class Directed Interactions", exception);
    throw;
  }
}

void UmbraRtiAmbassador::unsubscribeObjectClassDirectedInteractions(
    ObjectClassHandle const& objectClass,
    InteractionClassHandleSet const& interactionClasses) {
  auto instrumentationScope = beginRtiCall("unsubscribeObjectClassDirectedInteractions");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  requireFederationServiceOperationAvailable(
      L"Unsubscribe Object Class Directed Interactions");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Unsubscribe Object Class Directed Interactions requires membership in a federation execution.");
  }
  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  auto const objectClassHandleValueResult = objectClassHandleValue(objectClass);
  if (!objectClassHandleValueResult) {
    throw ObjectClassNotDefined(
        L"Unsubscribe Object Class Directed Interactions requires a defined ObjectClassHandle.");
  }
  auto const interactionClassHandleValuesResult =
      interactionClassHandleValues(interactionClasses);
  if (!interactionClassHandleValuesResult) {
    throw InteractionClassNotDefined(
        L"Unsubscribe Object Class Directed Interactions requires defined InteractionClassHandle values.");
  }
  auto const result = registry.unsubscribeObjectClassDirectedInteractions(
      *joinedFederationName_,
      *joinedFederateId_,
      *objectClassHandleValueResult,
      *interactionClassHandleValuesResult);
  if (result != umbra::detail::DirectedInteractionDeclarationStatus::applied) {
    throwDirectedInteractionDeclarationFailure(
        result,
        L"Unsubscribe Object Class Directed Interactions");
  }
  // A supplied empty set is distinct from the whole-class overload above: §5.13
  // defines it as a successful no-op, so it remains a type-28 [] report
  // argument after the registry accepts the invocation.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"UnsubscribeObjectClassDirectedInteractions",
      umbra::detail::MomServiceType::declaration_management,
      {{umbra::detail::MomArgumentType::object_class_handle,
        L"Object class designator",
        umbra::detail::formatMomObjectClassHandle(objectClass)},
       {umbra::detail::MomArgumentType::interaction_class_handle_set,
        L"Optional set of directed interaction designators",
        umbra::detail::formatMomInteractionClassHandleSet(interactionClasses)}});
  } catch (Exception const& exception) {
    emitExceptionReport(L"Unsubscribe Object Class Directed Interactions", exception);
    throw;
  }
}

void UmbraRtiAmbassador::subscribeInteractionClassWithRegions(
    InteractionClassHandle const& interactionClass,
    RegionHandleSet const& regions,
    bool active) {
  auto instrumentationScope = beginRtiCall("subscribeInteractionClassWithRegions");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  requireFederationServiceOperationAvailable(L"Subscribe Interaction Class With Regions");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Subscribe Interaction Class With Regions requires membership in a federation execution.");
  }

  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  auto const interactionClassValue = interactionClassHandleValue(interactionClass);
  if (!interactionClassValue) {
    throw InteractionClassNotDefined(
        L"Subscribe Interaction Class With Regions requires a defined InteractionClassHandle.");
  }
  std::set<std::uint64_t> regionValues;
  for (RegionHandle const& region : regions) {
    auto const value = regionHandleValue(region);
    if (!value) {
      throw InvalidRegion(
          L"Subscribe Interaction Class With Regions requires valid RegionHandle values.");
    }
    regionValues.insert(*value);
  }

  auto const result = registry.setInteractionClassRegionalSubscription(
      *joinedFederationName_,
      *joinedFederateId_,
      *interactionClassValue,
      regionValues,
      active);
  if (result != umbra::detail::RegionalInteractionClassDeclarationStatus::applied) {
    throwRegionalInteractionClassDeclarationFailure(result);
  }
  // §9.10 is a successful-void DDM service.  The C++ `active` selector is
  // the inverse of the standard's optional passive-subscription indicator;
  // preserve the supplied region set, including an accepted empty set, in
  // the private Table 5 report before any later declaration work is exposed.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"SubscribeInteractionClassWithRegions",
      umbra::detail::MomServiceType::data_distribution_management,
      {{umbra::detail::MomArgumentType::interaction_class_handle,
        L"Interaction class designator",
        umbra::detail::formatMomInteractionClassHandle(interactionClass)},
       {umbra::detail::MomArgumentType::region_handle_set,
        L"Set of region designators",
        umbra::detail::formatMomRegionHandleSet(regions)},
       {umbra::detail::MomArgumentType::boolean,
        L"Optional passive subscription indicator",
        umbra::detail::formatMomBoolean(!active)}});
  } catch (Exception const& exception) {
    emitExceptionReport(L"Subscribe Interaction Class With Regions", exception);
    throw;
  }
}

void UmbraRtiAmbassador::unsubscribeInteractionClassWithRegions(
    InteractionClassHandle const& interactionClass,
    RegionHandleSet const& regions) {
  auto instrumentationScope = beginRtiCall("unsubscribeInteractionClassWithRegions");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  requireFederationServiceOperationAvailable(L"Unsubscribe Interaction Class With Regions");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Unsubscribe Interaction Class With Regions requires membership in a federation execution.");
  }

  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  auto const interactionClassValue = interactionClassHandleValue(interactionClass);
  if (!interactionClassValue) {
    throw InteractionClassNotDefined(
        L"Unsubscribe Interaction Class With Regions requires a defined InteractionClassHandle.");
  }
  std::set<std::uint64_t> regionValues;
  for (RegionHandle const& region : regions) {
    auto const value = regionHandleValue(region);
    if (!value) {
      throw InvalidRegion(
          L"Unsubscribe Interaction Class With Regions requires valid RegionHandle values.");
    }
    regionValues.insert(*value);
  }

  auto const result = registry.removeInteractionClassRegionalSubscription(
      *joinedFederationName_,
      *joinedFederateId_,
      *interactionClassValue,
      regionValues);
  if (result != umbra::detail::RegionalInteractionClassDeclarationStatus::applied) {
    throwRegionalInteractionClassDeclarationFailure(result);
  }
  // §9.11 is the paired successful-void DDM removal.  An empty accepted set
  // remains a real invocation and is serialized as the supplied type-43
  // Array<RegionHandle>, rather than being conflated with a Null omission.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"UnsubscribeInteractionClassWithRegions",
      umbra::detail::MomServiceType::data_distribution_management,
      {{umbra::detail::MomArgumentType::interaction_class_handle,
        L"Interaction class designator",
        umbra::detail::formatMomInteractionClassHandle(interactionClass)},
       {umbra::detail::MomArgumentType::region_handle_set,
        L"Set of region designators",
        umbra::detail::formatMomRegionHandleSet(regions)}});
  } catch (Exception const& exception) {
    emitExceptionReport(L"Unsubscribe Interaction Class With Regions", exception);
    throw;
  }
}

void UmbraRtiAmbassador::sendInteraction(
    InteractionClassHandle const& interactionClass,
    ParameterHandleValueMap const& parameterValues,
    VariableLengthData const& userSuppliedTag) {
  auto instrumentationScope = beginRtiCall("sendInteraction");
  try {
  std::optional<std::wstring> federationName;
  std::optional<std::uint64_t> producingFederateId;
  // Preserve the 2025 service's connection and membership preconditions ahead
  // of caller-supplied handle validation, matching the other public services.
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Send Interaction");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Send Interaction requires membership in a federation execution.");
    }
    if (!embeddedFederationManagement().registry().memberById(
            *joinedFederationName_,
            *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    federationName = *joinedFederationName_;
    producingFederateId = *joinedFederateId_;
  }

  auto const interactionClassHandle = interactionClassHandleValue(interactionClass);
  if (!interactionClassHandle) {
    throw InteractionClassNotDefined(
        L"Send Interaction requires a defined InteractionClassHandle.");
  }
  auto const parameterHandles = interactionParameterHandleValues(parameterValues);
  if (!parameterHandles) {
    throw InteractionParameterNotDefined(
        L"Send Interaction requires defined ParameterHandle values.");
  }

  std::optional<JoinedFederateMomConditionalWork> momSwitchWork;
  std::optional<JoinedFederateMomConditionalWork> federationMomSwitchWork;
  auto const handleMomSwitchAdjustment = [&]() {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Send Interaction");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ ||
        *joinedFederationName_ != *federationName ||
        *joinedFederateId_ != *producingFederateId) {
      throw FederateNotExecutionMember(
          L"The RTI ambassador's federation membership changed during Send Interaction.");
    }
    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(*federationName, *producingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const interactionClassName = registry.interactionClassNameFor(
        *federationName,
        *interactionClassHandle);
    if (!interactionClassName) {
      return false;
    }
    auto const isFederateSetTiming = registry.interactionClassIsSameOrDescendantOf(
        *federationName,
        *interactionClassHandle,
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming");
    if (!isFederateSetTiming) {
      throw RTIinternalError(
          L"The embedded federation cannot resolve the HLAsetTiming MOM class hierarchy.");
    }
    if (*isFederateSetTiming) {
      std::optional<std::uint64_t> targetFederateId;
      std::optional<std::int32_t> reportPeriodSeconds;
      for (auto const& [parameterHandle, parameterValue] : parameterValues) {
        auto const suppliedParameterHandleValue = parameterHandleValue(parameterHandle);
        if (!suppliedParameterHandleValue) {
          throw InteractionParameterNotDefined(
              L"HLAsetTiming received an invalid parameter handle.");
        }
        auto const parameterName = registry.parameterNameFor(
            *federationName,
            *interactionClassName,
            *suppliedParameterHandleValue);
        if (!parameterName) {
          throw InteractionParameterNotDefined(
              L"HLAsetTiming received an unknown parameter for this interaction class.");
        }
        if (*parameterName == "HLAfederate") {
          try {
            auto const decodedHandle =
                ::rti1516_2025::umbra_binding_detail::decodeFederateHandle(parameterValue);
            targetFederateId =
                ::rti1516_2025::umbra_binding_detail::federateHandleValue(decodedHandle);
          } catch (Exception const&) {
            throw RTIinternalError(
                L"HLAsetTiming received an invalid HLAfederateReference value.");
          }
          if (!targetFederateId) {
            throw RTIinternalError(
                L"HLAsetTiming received an invalid HLAfederateReference value.");
          }
          continue;
        }
        if (*parameterName == "HLAreportPeriod") {
          rti1516_2025::HLAinteger32BE decodedPeriod;
          try {
            decodedPeriod.decode(parameterValue);
          } catch (Exception const&) {
            throw RTIinternalError(
                L"HLAsetTiming received an invalid HLAseconds HLAinteger32BE value.");
          }
          reportPeriodSeconds = decodedPeriod.get();
          continue;
        }
        // A compatible MOM extension may carry an additional parameter.  It
        // is received but does not alter the predefined HLAsetTiming state.
      }
      if (!targetFederateId || !reportPeriodSeconds) {
        throw InteractionParameterNotDefined(
            L"HLAsetTiming requires both HLAfederate and HLAreportPeriod parameters.");
      }
      auto const result = registry.setFederateMomReportPeriod(
          *federationName,
          *producingFederateId,
          *targetFederateId,
          *reportPeriodSeconds);
      switch (result) {
        case umbra::detail::FederateMOMTimingUpdateStatus::applied:
          return true;
        case umbra::detail::FederateMOMTimingUpdateStatus::federation_does_not_exist:
        case umbra::detail::FederateMOMTimingUpdateStatus::requesting_federate_not_member:
        case umbra::detail::FederateMOMTimingUpdateStatus::target_federate_not_member:
          throw FederateNotExecutionMember(
              L"HLAsetTiming requires both the requesting and target federates to be joined.");
        case umbra::detail::FederateMOMTimingUpdateStatus::invalid_report_period:
          throw RTIinternalError(
              L"HLAsetTiming requires a non-negative HLAreportPeriod value.");
      }
      throw RTIinternalError(
          L"The embedded federation rejected the HLAsetTiming adjustment.");
    }
    auto const isFederationSetSwitches = registry.interactionClassIsSameOrDescendantOf(
        *federationName,
        *interactionClassHandle,
        "HLAinteractionRoot.HLAmanager.HLAfederation.HLAadjust.HLAsetSwitches");
    auto const isFederateSetSwitches = registry.interactionClassIsSameOrDescendantOf(
        *federationName,
        *interactionClassHandle,
        "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches");
    if (!isFederationSetSwitches || !isFederateSetSwitches) {
      throw RTIinternalError(
          L"The embedded federation cannot resolve the HLAsetSwitches MOM class hierarchy.");
    }

    if (*isFederationSetSwitches) {
      if (parameterValues.empty()) {
        throw InteractionParameterNotDefined(
            L"The federation HLAsetSwitches interaction requires at least one declared parameter.");
      }
      std::optional<bool> autoProvideSwitchValue;
      for (auto const& [parameterHandle, parameterValue] : parameterValues) {
        auto const suppliedParameterHandleValue = parameterHandleValue(parameterHandle);
        if (!suppliedParameterHandleValue) {
          throw InteractionParameterNotDefined(
              L"HLAsetSwitches received an invalid parameter handle.");
        }
        auto const parameterName = registry.parameterNameFor(
            *federationName,
            *interactionClassName,
            *suppliedParameterHandleValue);
        if (!parameterName) {
          throw InteractionParameterNotDefined(
              L"HLAsetSwitches received an unknown parameter for this interaction class.");
        }
        if (*parameterName != "HLAautoProvide") {
          // A compatible MOM extension may carry a parameter on this class
          // or its subclass.  The RTI must receive it but process only the
          // predefined HLAautoProvide value.
          continue;
        }
        auto const switchValue = decodeHlaSwitch(parameterValue);
        if (!switchValue) {
          throw RTIinternalError(
              L"HLAsetSwitches received an invalid HLAswitch HLAinteger32BE value.");
        }
        autoProvideSwitchValue = *switchValue;
      }
      if (!autoProvideSwitchValue) {
        return true;
      }
      auto const previousAutoProvide = registry.autoProvideSwitchFor(
          *federationName,
          *producingFederateId);
      auto const result = registry.setAutoProvideSwitch(
          *federationName,
          *producingFederateId,
          *autoProvideSwitchValue);
      switch (result) {
        case umbra::detail::FederationRegistryStatus::applied:
          if (previousAutoProvide &&
              *previousAutoProvide != *autoProvideSwitchValue) {
            federationMomSwitchWork = federationMomConditionalWorkFor(
                registry,
                *federationName,
                {"HLAautoProvide"});
          }
          return true;
        case umbra::detail::FederationRegistryStatus::federation_does_not_exist:
        case umbra::detail::FederationRegistryStatus::federate_not_member:
          throw FederateNotExecutionMember(
              L"The embedded federation no longer records this RTI ambassador as a member.");
        default:
          throw RTIinternalError(
              L"The embedded federation rejected the HLAsetSwitches Auto Provide adjustment.");
      }
    }

    if (!*isFederateSetSwitches) {
      return false;
    }
    if (parameterValues.empty()) {
      throw InteractionParameterNotDefined(
          L"The joined-federate HLAsetSwitches interaction requires at least one declared parameter.");
    }

    umbra::detail::FederateMOMSwitchUpdate update;
    for (auto const& [parameterHandle, parameterValue] : parameterValues) {
      auto const suppliedParameterHandleValue = parameterHandleValue(parameterHandle);
      if (!suppliedParameterHandleValue) {
        throw InteractionParameterNotDefined(
            L"HLAsetSwitches received an invalid parameter handle.");
      }
      auto const parameterName = registry.parameterNameFor(
          *federationName,
          *interactionClassName,
          *suppliedParameterHandleValue);
      if (!parameterName) {
        throw InteractionParameterNotDefined(
            L"HLAsetSwitches received an unknown parameter for this interaction class.");
      }

      if (*parameterName == "HLAautomaticResignAction") {
        auto const resignAction = decodeHlaResignAction(parameterValue);
        if (!resignAction) {
          throw RTIinternalError(
              L"HLAsetSwitches received an invalid HLAresignAction HLAinteger32BE value.");
        }
        update.automaticResignAction = *resignAction;
        continue;
      }

      if (*parameterName != "HLAobjectClassRelevanceAdvisory" &&
          *parameterName != "HLAattributeRelevanceAdvisory" &&
          *parameterName != "HLAattributeScopeAdvisory" &&
          *parameterName != "HLAinteractionRelevanceAdvisory" &&
          *parameterName != "HLAconveyRegionDesignatorSets" &&
          *parameterName != "HLAserviceReporting" &&
          *parameterName != "HLAexceptionReporting" &&
          *parameterName != "HLAsendServiceReportsToFile") {
        // MOM extensions may add parameters to a predefined interaction. IEEE
        // 1516.1-2025 §11.4.1 requires the RTI to receive those values but
        // process only the predefined parameters, so deliberately leave an
        // extension value opaque instead of imposing a local wire type.
        continue;
      }
      auto const switchValue = decodeHlaSwitch(parameterValue);
      if (!switchValue) {
        throw RTIinternalError(
            L"HLAsetSwitches received an invalid HLAswitch HLAinteger32BE value.");
      }
      if (*parameterName == "HLAobjectClassRelevanceAdvisory") {
        update.objectClassRelevanceAdvisory = *switchValue;
      } else if (*parameterName == "HLAattributeRelevanceAdvisory") {
        update.attributeRelevanceAdvisory = *switchValue;
      } else if (*parameterName == "HLAattributeScopeAdvisory") {
        update.attributeScopeAdvisory = *switchValue;
      } else if (*parameterName == "HLAinteractionRelevanceAdvisory") {
        update.interactionRelevanceAdvisory = *switchValue;
      } else if (*parameterName == "HLAconveyRegionDesignatorSets") {
        update.conveyRegionDesignatorSets = *switchValue;
      } else if (*parameterName == "HLAserviceReporting") {
        update.serviceReporting = *switchValue;
      } else if (*parameterName == "HLAexceptionReporting") {
        update.exceptionReporting = *switchValue;
      } else if (*parameterName == "HLAsendServiceReportsToFile") {
        update.sendServiceReportsToFile = *switchValue;
      }
    }

    auto const result = registry.applyFederateMOMSwitchUpdate(
        *federationName,
        *producingFederateId,
        update);
    switch (result) {
      case umbra::detail::FederateMOMSwitchUpdateStatus::applied:
        {
          std::vector<std::string_view> changedAttributes;
          if (update.objectClassRelevanceAdvisory) {
            changedAttributes.emplace_back("HLAobjectClassRelevanceAdvisory");
          }
          if (update.attributeRelevanceAdvisory) {
            changedAttributes.emplace_back("HLAattributeRelevanceAdvisory");
          }
          if (update.attributeScopeAdvisory) {
            changedAttributes.emplace_back("HLAattributeScopeAdvisory");
          }
          if (update.interactionRelevanceAdvisory) {
            changedAttributes.emplace_back("HLAinteractionRelevanceAdvisory");
          }
          if (update.conveyRegionDesignatorSets) {
            changedAttributes.emplace_back("HLAconveyRegionDesignatorSets");
          }
          if (update.automaticResignAction) {
            changedAttributes.emplace_back("HLAautomaticResignAction");
          }
          if (update.serviceReporting) {
            changedAttributes.emplace_back("HLAserviceReporting");
          }
          if (update.exceptionReporting) {
            changedAttributes.emplace_back("HLAexceptionReporting");
          }
          if (update.sendServiceReportsToFile) {
            changedAttributes.emplace_back("HLAsendServiceReportsToFile");
          }
          if (!changedAttributes.empty()) {
            momSwitchWork = joinedFederateMomConditionalWorkFor(
                registry,
                *federationName,
                *producingFederateId,
                std::move(changedAttributes));
          }
        }
        return true;
      case umbra::detail::FederateMOMSwitchUpdateStatus::federation_does_not_exist:
      case umbra::detail::FederateMOMSwitchUpdateStatus::federate_not_member:
        throw FederateNotExecutionMember(
            L"The embedded federation no longer records this RTI ambassador as a member.");
      case umbra::detail::FederateMOMSwitchUpdateStatus::invalid_resign_action:
        throw RTIinternalError(
            L"The embedded federation rejected an invalid HLAresignAction HLAsetSwitches value.");
      case umbra::detail::FederateMOMSwitchUpdateStatus::
          report_service_invocations_are_subscribed:
        // §11.5.1 calls for an HLAsetSwitches-specific MOM
        // interaction-failure payload.  That distinct failure record is not
        // implemented yet, so retain the failure rather than silently
        // changing state and report the bounded embedded-profile limitation
        // through the public Send Interaction error channel.
        throw RTIinternalError(
            L"HLAsetSwitches cannot enable Service Reporting while report-service invocations are subscribed.");
      default:
        throw RTIinternalError(
            L"The embedded federation rejected the joined-federate HLAsetSwitches adjustment.");
    }
  };

  if (handleMomSwitchAdjustment()) {
    // HLAsetTiming/HLAsetSwitches are still successful Send Interaction
    // invocations. Count their accepted service boundary even though the
    // embedded profile consumes them as MOM adjustments without application
    // fan-out.
    {
      std::scoped_lock lock(federationManagementMutex());
      auto const status = embeddedFederationManagement().registry()
          .recordSuccessfulInteractionSend(
              *federationName, *producingFederateId, false);
      if (status != umbra::detail::FederationRegistryStatus::applied) {
        throw RTIinternalError(
            L"The embedded federation lost the Send Interaction membership before its accepted MOM adjustment boundary.");
      }
    }
    if (momSwitchWork) {
      queueJoinedFederateMomConditionalAttributeUpdate(
          momSwitchWork->federationName,
          momSwitchWork->objectInstanceHandle,
          std::move(momSwitchWork->attributeHandles));
    }
    if (federationMomSwitchWork) {
      queueJoinedFederateMomConditionalAttributeUpdate(
          federationMomSwitchWork->federationName,
          federationMomSwitchWork->objectInstanceHandle,
          std::move(federationMomSwitchWork->attributeHandles));
    }
    return;
  }

  auto planCurrentRequest = [&]() {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Send Interaction");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Send Interaction requires membership in a federation execution.");
    }
    if (!federationName) {
      federationName = *joinedFederationName_;
      producingFederateId = *joinedFederateId_;
    } else if (*joinedFederationName_ != *federationName ||
               *joinedFederateId_ != *producingFederateId) {
      throw FederateNotExecutionMember(
          L"The RTI ambassador's federation membership changed during Send Interaction.");
    }

    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(*federationName, *producingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto plan = registry.planReceiveOrderInteraction(
        *federationName,
        *producingFederateId,
        *interactionClassHandle,
        *parameterHandles);
    if (plan.status != umbra::detail::ReceiveOrderInteractionStatus::applied) {
      throwReceiveOrderInteractionFailure(plan.status);
    }
    return plan;
  };

  // Validate publication, membership, class, and parameter availability before
  // reading caller-owned VariableLengthData storage.  The durable copies below
  // are made without a runtime lock, then the plan is rechecked before routing.
  static_cast<void>(planCurrentRequest());
  std::vector<InteractionParameterValue> sentParameters =
      copyInteractionParameterValues(parameterValues);
  VariableLengthData copiedTag(userSuppliedTag);
  ParameterHandleValueMap reportParameterValues;
  for (auto const& [parameterHandle, parameterValue] : sentParameters) {
    reportParameterValues.emplace(makeParameterHandle(parameterHandle), parameterValue);
  }
  // Section 6.12.1 names four supplied values. The receive-order overload
  // leaves the optional timestamp unused, and Section 11.5.1 therefore
  // requires its corresponding supplied-argument element to be Null. The
  // accepted parameter map is rebuilt from durable copies before formatting so
  // caller-owned buffers cannot alter the report boundary.
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::interaction_class_handle,
       L"Interaction class designator",
       umbra::detail::formatMomInteractionClassHandle(interactionClass)},
      {umbra::detail::MomArgumentType::parameter_handle_value_map,
       L"Constrained set of interaction parameter designator and value pairs",
       umbra::detail::formatMomParameterHandleValueMap(reportParameterValues)},
      {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       umbra::detail::formatMomUserSuppliedTag(copiedTag)},
      {umbra::detail::MomArgumentType::null_value,
       L"Optional timestamp",
       umbra::detail::formatMomNull()},
  };
  auto plan = planCurrentRequest();

  auto const transportationName = umbra::detail::wideFromUtf8(plan.transportationName);
  if (!transportationName) {
    throw RTIinternalError(
        L"The composed FOM contains an invalid UTF-8 transportation type name.");
  }
  auto const transportationValue = standardTransportationTypeValue(*transportationName);
  if (!transportationValue) {
    throw RTIinternalError(
        L"The embedded profile cannot deliver an interaction using this FOM transportation type.");
  }
  TransportationTypeHandle const transportationType = makeTransportationTypeHandle(*transportationValue);

  struct Delivery {
    umbra::detail::InteractionCallbackRoute callbackRoute;
    std::uint64_t recipientId = 0;
  };
  std::vector<Delivery> deliveries;
  deliveries.reserve(plan.recipients.size());
  for (auto const& recipient : plan.recipients) {
    if (!recipient.callbackRoute) {
      throw RTIinternalError(
          L"The embedded federation has an eligible recipient without a callback route.");
    }
    deliveries.push_back({recipient.callbackRoute, recipient.federateId});
  }

  // Every synchronous pre-callback delivery check has now succeeded. Section
  // 6.12's accepted interaction is the report boundary, and the §11.5 file
  // record must precede any induced Receive Interaction callback.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"SendInteraction",
      umbra::detail::MomServiceType::object_management,
      reportArguments,
      true);

  // The accepted Send Interaction invocation is the MOM counter boundary;
  // recipient callback fan-out must not inflate HLAinteractionsSent.
  {
    std::scoped_lock lock(federationManagementMutex());
    auto const status = embeddedFederationManagement().registry()
        .recordSuccessfulInteractionSend(
            *federationName, *producingFederateId, false);
    if (status != umbra::detail::FederationRegistryStatus::applied) {
      throw RTIinternalError(
          L"The embedded federation lost the Send Interaction membership before its accepted boundary.");
    }
  }

  // Do not hold either sender lock while submitting a route: HLA_IMMEDIATE may
  // synchronously enter a different federate's Receive Interaction callback.
  for (auto& delivery : deliveries) {
    queueReceiveOrderInteraction(
        std::move(delivery.callbackRoute),
        *federationName,
        umbra::detail::InteractionProducer::joinedFederate(*producingFederateId),
        delivery.recipientId,
        *interactionClassHandle,
        sentParameters,
        copiedTag,
        transportationType,
        std::nullopt,
        plan.defaultRegionUsed);
  }
  } catch (Exception const& exception) {
    emitExceptionReport(L"Send Interaction", exception);
    throw;
  }
}

MessageRetractionHandle UmbraRtiAmbassador::sendInteraction(
    InteractionClassHandle const& interactionClass,
    ParameterHandleValueMap const& parameterValues,
    VariableLengthData const& userSuppliedTag,
    LogicalTime const& time) {
  auto instrumentationScope = beginRtiCall("sendInteraction");
  try {
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Timestamped Send Interaction");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Timestamped Send Interaction requires membership in a federation execution.");
    }
    if (!embeddedFederationManagement().registry().memberById(
            *joinedFederationName_,
            *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    timeState = federateTimeState_;
  }

  auto const interactionClassHandle = interactionClassHandleValue(interactionClass);
  if (!interactionClassHandle) {
    throw InteractionClassNotDefined(
        L"Timestamped Send Interaction requires a defined InteractionClassHandle.");
  }
  auto const parameterHandles = interactionParameterHandleValues(parameterValues);
  if (!parameterHandles) {
    throw InteractionParameterNotDefined(
        L"Timestamped Send Interaction requires defined ParameterHandle values.");
  }

  auto timestamp = cloneReferenceLogicalTime(timeState->implementationName(), time);
  auto const timeSnapshot = timeState->snapshot();
  validateTsoTimestamp(timeSnapshot, *timestamp);

  std::optional<std::wstring> federationName;
  std::optional<std::uint64_t> producingFederateId;
  auto planCurrentRequest = [&]() {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Timestamped Send Interaction");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ ||
        !federateTimeState_ || federateTimeState_ != timeState) {
      throw FederateNotExecutionMember(
          L"Timestamped Send Interaction requires an active joined federate.");
    }
    if (!federationName) {
      federationName = *joinedFederationName_;
      producingFederateId = *joinedFederateId_;
    } else if (*joinedFederationName_ != *federationName ||
               *joinedFederateId_ != *producingFederateId) {
      throw FederateNotExecutionMember(
          L"The RTI ambassador's federation membership changed during Timestamped Send Interaction.");
    }

    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(*federationName, *producingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto plan = registry.planReceiveOrderInteraction(
        *federationName,
        *producingFederateId,
        *interactionClassHandle,
        *parameterHandles);
    if (plan.status != umbra::detail::ReceiveOrderInteractionStatus::applied) {
      throwReceiveOrderInteractionFailure(plan.status);
    }
    return plan;
  };

  static_cast<void>(planCurrentRequest());
  std::vector<InteractionParameterValue> sentParameters =
      copyInteractionParameterValues(parameterValues);
  VariableLengthData copiedTag(userSuppliedTag);
  ParameterHandleValueMap reportParameterValues;
  for (auto const& [parameterHandle, parameterValue] : sentParameters) {
    reportParameterValues.emplace(makeParameterHandle(parameterHandle), parameterValue);
  }
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::interaction_class_handle,
       L"Interaction class designator",
       umbra::detail::formatMomInteractionClassHandle(interactionClass)},
      {umbra::detail::MomArgumentType::parameter_handle_value_map,
       L"Constrained set of interaction parameter designator and value pairs",
       umbra::detail::formatMomParameterHandleValueMap(reportParameterValues)},
      {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       umbra::detail::formatMomUserSuppliedTag(copiedTag)},
      {umbra::detail::MomArgumentType::logical_time,
       L"Optional timestamp",
       umbra::detail::formatMomLogicalTime(*timestamp)},
  };
  auto plan = planCurrentRequest();

  auto const transportationName = umbra::detail::wideFromUtf8(plan.transportationName);
  if (!transportationName) {
    throw RTIinternalError(
        L"The composed FOM contains an invalid UTF-8 transportation type name.");
  }
  auto const transportationValue = standardTransportationTypeValue(*transportationName);
  if (!transportationValue) {
    throw RTIinternalError(
        L"The embedded profile cannot deliver an interaction using this FOM transportation type.");
  }
  TransportationTypeHandle const transportationType =
      makeTransportationTypeHandle(*transportationValue);

  std::set<std::uint64_t> timeConstrainedRecipients;
  if (federationName && timeSnapshot.timeRegulating) {
    std::scoped_lock lock(federationManagementMutex());
    auto const snapshot = embeddedFederationManagement().registry().timeSnapshotFor(
        *federationName);
    if (!snapshot) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer exposes a coherent time snapshot.");
    }
    for (auto const& federate : snapshot->federates) {
      if (federate.time.timeConstrained) {
        timeConstrainedRecipients.insert(federate.membership.id);
      }
    }
  }

  std::uint64_t messageId = 0;
  if (timeSnapshot.timeRegulating && plan.preferredOrderType == TIMESTAMP) {
    std::vector<std::uint64_t> tsoRecipientIds;
    std::vector<std::uint64_t> allTimestampedRecipientIds;
    allTimestampedRecipientIds.reserve(plan.recipients.size());
    for (auto const& recipient : plan.recipients) {
      allTimestampedRecipientIds.push_back(recipient.federateId);
      if (timeConstrainedRecipients.contains(recipient.federateId)) {
        tsoRecipientIds.push_back(recipient.federateId);
      }
    }

    umbra::detail::TsoInteractionMessage message;
    message.producingFederateId = *producingFederateId;
    message.sentInteractionClassHandle = *interactionClassHandle;
    message.sentParameterHandles = *parameterHandles;
    message.parameters = sentParameters;
    message.userSuppliedTag = copiedTag;
    message.transportationName = plan.transportationName;
    message.defaultRegionUsed = plan.defaultRegionUsed;
    message.sentOrderType = TIMESTAMP;
    message.receivedOrderType = TIMESTAMP;
    std::shared_ptr<LogicalTime const> sharedTimestamp = timestamp;
    message.timestamp = std::move(sharedTimestamp);

    std::scoped_lock lock(federationManagementMutex());
    auto const result = embeddedFederationManagement().registry().enqueueTsoInteraction(
        *federationName,
        std::move(message),
        tsoRecipientIds,
        allTimestampedRecipientIds);
    if (result.status != umbra::detail::FederationTsoRegistryStatus::applied ||
        result.queueStatus != umbra::detail::TsoMessageQueueStatus::applied ||
        result.messageId == 0) {
      throw RTIinternalError(
          L"The embedded federation could not queue the timestamped Send Interaction.");
    }
    messageId = result.messageId;
  }

  umbra::detail::MomServiceArgument returnedArgument{
      umbra::detail::MomArgumentType::null_value,
      L"",
      umbra::detail::formatMomNull()};
  if (messageId != 0U) {
    returnedArgument = {
        umbra::detail::MomArgumentType::message_retraction_handle,
        L"Message retraction designator",
        umbra::detail::formatMomMessageRetractionHandle(messageId)};
  }
  // TSO admission is the accepted service boundary. Emit the RTI-originated
  // report only after the C++ registry has assigned the retraction identity,
  // but before any ordinary/timestamped receive callback is queued.
  static_cast<void>(emitSelectedMomServiceReportInteraction(
      L"SendInteraction",
      umbra::detail::MomServiceType::object_management,
      reportArguments,
      returnedArgument));

  // Timestamped queue admission (or successful no-recipient validation) is
  // the accepted Send Interaction boundary. Count the service once, not once
  // per TSO/receive-order recipient.
  {
    std::scoped_lock lock(federationManagementMutex());
    auto const status = embeddedFederationManagement().registry()
        .recordSuccessfulInteractionSend(
            *federationName, *producingFederateId, false);
    if (status != umbra::detail::FederationRegistryStatus::applied) {
      throw RTIinternalError(
          L"The embedded federation lost the timestamped Send Interaction membership before its accepted boundary.");
    }
  }

  // The first public slice queues TSO for time-constrained recipients. A
  // non-time-constrained recipient still receives the timestamped callback
  // immediately as Receive Order, with the sender's TSO designator when one
  // exists. The callback rechecks subscriptions at its own user-code boundary.
  for (auto const& recipient : plan.recipients) {
    if (timeSnapshot.timeRegulating && plan.preferredOrderType == TIMESTAMP &&
        timeConstrainedRecipients.contains(recipient.federateId)) {
      continue;
    }
    if (!recipient.callbackRoute) {
      throw RTIinternalError(
          L"The embedded federation has an eligible recipient without a callback route.");
    }
    queueTimestampedReceiveOrderInteraction(
        recipient.callbackRoute,
        *federationName,
        umbra::detail::InteractionProducer::joinedFederate(*producingFederateId),
        recipient.federateId,
        *interactionClassHandle,
        sentParameters,
        copiedTag,
        transportationType,
        std::shared_ptr<LogicalTime const>(timestamp),
        timeSnapshot.timeRegulating && plan.preferredOrderType == TIMESTAMP
            ? TIMESTAMP
            : RECEIVE,
        RECEIVE,
        timeSnapshot.timeRegulating && plan.preferredOrderType == TIMESTAMP && messageId != 0
            ? std::optional<std::uint64_t>(messageId)
            : std::nullopt,
        std::nullopt,
        plan.defaultRegionUsed);
  }

  if (!timeSnapshot.timeRegulating || messageId == 0) {
    return MessageRetractionHandle();
  }
  return makeMessageRetractionHandle(messageId);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Send Interaction", exception);
    throw;
  }
}

void UmbraRtiAmbassador::sendDirectedInteraction(
    InteractionClassHandle const& interactionClass,
    ObjectInstanceHandle const& objectInstance,
    ParameterHandleValueMap const& parameterValues,
    VariableLengthData const& userSuppliedTag) {
  auto instrumentationScope = beginRtiCall("sendDirectedInteraction");
  try {
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Send Directed Interaction");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Send Directed Interaction requires membership in a federation execution.");
    }
    if (!embeddedFederationManagement().registry().memberById(
            *joinedFederationName_,
            *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
  }

  auto const interactionClassHandle = interactionClassHandleValue(interactionClass);
  if (!interactionClassHandle) {
    throw InteractionClassNotDefined(
        L"Send Directed Interaction requires a defined InteractionClassHandle.");
  }
  auto const objectInstanceHandleValueResult = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceHandleValueResult) {
    throw ObjectInstanceNotKnown(
        L"Send Directed Interaction requires a valid target ObjectInstanceHandle.");
  }
  auto const parameterHandles = interactionParameterHandleValues(parameterValues);
  if (!parameterHandles) {
    throw InteractionParameterNotDefined(
        L"Send Directed Interaction requires defined ParameterHandle values.");
  }

  std::optional<std::wstring> federationName;
  std::optional<std::uint64_t> producingFederateId;
  auto planCurrentRequest = [&]() {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Send Directed Interaction");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Send Directed Interaction requires membership in a federation execution.");
    }
    if (!federationName) {
      federationName = *joinedFederationName_;
      producingFederateId = *joinedFederateId_;
    } else if (*joinedFederationName_ != *federationName ||
               *joinedFederateId_ != *producingFederateId) {
      throw FederateNotExecutionMember(
          L"The RTI ambassador's federation membership changed during Send Directed Interaction.");
    }

    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(*federationName, *producingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto plan = registry.planReceiveOrderDirectedInteraction(
        *federationName,
        *producingFederateId,
        *objectInstanceHandleValueResult,
        *interactionClassHandle,
        *parameterHandles);
    if (plan.status !=
        umbra::detail::ReceiveOrderDirectedInteractionStatus::applied) {
      throwReceiveOrderDirectedInteractionFailure(plan.status);
    }
    return plan;
  };

  // Validate publication, target knowledge, and parameter handles before
  // copying caller-owned values, then re-plan before routing.
  static_cast<void>(planCurrentRequest());
  std::vector<InteractionParameterValue> sentParameters =
      copyInteractionParameterValues(parameterValues);
  VariableLengthData copiedTag(userSuppliedTag);
  ParameterHandleValueMap reportParameterValues;
  for (auto const& [parameterHandle, parameterValue] : sentParameters) {
    reportParameterValues.emplace(makeParameterHandle(parameterHandle), parameterValue);
  }
  // Section 6.14.1 names five supplied values. The receive-order overload
  // leaves the optional timestamp unused, and Section 11.5.1 therefore
  // requires its corresponding supplied-argument element to be Null. The
  // accepted parameter map is rebuilt from durable copies before formatting so
  // caller-owned buffers cannot alter the report boundary.
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::interaction_class_handle,
       L"Interaction class designator",
       umbra::detail::formatMomInteractionClassHandle(interactionClass)},
      {umbra::detail::MomArgumentType::object_instance_handle,
       L"Object instance designator",
       umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
      {umbra::detail::MomArgumentType::parameter_handle_value_map,
       L"Constrained set of interaction parameter designator and value pairs",
       umbra::detail::formatMomParameterHandleValueMap(reportParameterValues)},
      {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       umbra::detail::formatMomUserSuppliedTag(copiedTag)},
      {umbra::detail::MomArgumentType::null_value,
       L"Optional timestamp",
       umbra::detail::formatMomNull()},
  };
  auto plan = planCurrentRequest();

  auto const transportationName = umbra::detail::wideFromUtf8(plan.transportationName);
  if (!transportationName) {
    throw RTIinternalError(
        L"The composed FOM contains an invalid UTF-8 directed-interaction transportation name.");
  }
  auto const transportationValue = standardTransportationTypeValue(*transportationName);
  if (!transportationValue) {
    throw RTIinternalError(
        L"The embedded profile cannot deliver a directed interaction using this FOM transportation type.");
  }
  TransportationTypeHandle const transportationType =
      makeTransportationTypeHandle(*transportationValue);

  struct Delivery {
    umbra::detail::InteractionCallbackRoute callbackRoute;
    std::uint64_t recipientId = 0;
  };
  std::vector<Delivery> deliveries;
  deliveries.reserve(plan.recipients.size());
  for (auto const& recipient : plan.recipients) {
    if (!recipient.callbackRoute) {
      throw RTIinternalError(
          L"The embedded federation has an eligible directed-interaction recipient without a callback route.");
    }
    deliveries.push_back({recipient.callbackRoute, recipient.federateId});
  }

  // Every synchronous pre-callback delivery check has now succeeded. Section
  // 6.14's accepted interaction is the report boundary, and the §11.5 file
  // record must precede any induced Receive Directed Interaction callback.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"SendDirectedInteraction",
      umbra::detail::MomServiceType::object_management,
      reportArguments);

  {
    std::scoped_lock lock(federationManagementMutex());
    auto const status = embeddedFederationManagement().registry()
        .recordSuccessfulInteractionSend(
            *federationName, *producingFederateId, true);
    if (status != umbra::detail::FederationRegistryStatus::applied) {
      throw RTIinternalError(
          L"The embedded federation lost the Send Directed Interaction membership before its accepted boundary.");
    }
  }

  // Submit only after releasing the sender/runtime locks: HLA_IMMEDIATE may
  // synchronously enter another federate's Receive Directed Interaction callback.
  for (auto& delivery : deliveries) {
    queueReceiveOrderDirectedInteraction(
        std::move(delivery.callbackRoute),
        *federationName,
        *producingFederateId,
        delivery.recipientId,
        *objectInstanceHandleValueResult,
        *interactionClassHandle,
        sentParameters,
        copiedTag,
        transportationType);
  }
  } catch (Exception const& exception) {
    emitExceptionReport(L"Send Directed Interaction", exception);
    throw;
  }
}

MessageRetractionHandle UmbraRtiAmbassador::sendDirectedInteraction(
    InteractionClassHandle const& interactionClass,
    ObjectInstanceHandle const& objectInstance,
    ParameterHandleValueMap const& parameterValues,
    VariableLengthData const& userSuppliedTag,
    LogicalTime const& time) {
  auto instrumentationScope = beginRtiCall("sendDirectedInteraction");
  try {
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Timestamped Send Directed Interaction");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Timestamped Send Directed Interaction requires membership in a federation execution.");
    }
    if (!embeddedFederationManagement().registry().memberById(
            *joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    timeState = federateTimeState_;
  }

  auto const interactionClassHandle = interactionClassHandleValue(interactionClass);
  if (!interactionClassHandle) {
    throw InteractionClassNotDefined(
        L"Timestamped Send Directed Interaction requires a defined InteractionClassHandle.");
  }
  auto const objectInstanceHandleValueResult = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceHandleValueResult) {
    throw ObjectInstanceNotKnown(
        L"Timestamped Send Directed Interaction requires a valid target ObjectInstanceHandle.");
  }
  auto const parameterHandles = interactionParameterHandleValues(parameterValues);
  if (!parameterHandles) {
    throw InteractionParameterNotDefined(
        L"Timestamped Send Directed Interaction requires defined ParameterHandle values.");
  }

  auto timestamp = cloneReferenceLogicalTime(timeState->implementationName(), time);
  auto const timeSnapshot = timeState->snapshot();
  validateTsoTimestamp(timeSnapshot, *timestamp);

  std::optional<std::wstring> federationName;
  std::optional<std::uint64_t> producingFederateId;
  auto planCurrentRequest = [&]() {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Timestamped Send Directed Interaction");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ ||
        !federateTimeState_ || federateTimeState_ != timeState) {
      throw FederateNotExecutionMember(
          L"Timestamped Send Directed Interaction requires an active joined federate.");
    }
    if (!federationName) {
      federationName = *joinedFederationName_;
      producingFederateId = *joinedFederateId_;
    } else if (*joinedFederationName_ != *federationName ||
               *joinedFederateId_ != *producingFederateId) {
      throw FederateNotExecutionMember(
          L"The RTI ambassador's federation membership changed during Timestamped Send Directed Interaction.");
    }

    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(*federationName, *producingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto plan = registry.planReceiveOrderDirectedInteraction(
        *federationName,
        *producingFederateId,
        *objectInstanceHandleValueResult,
        *interactionClassHandle,
        *parameterHandles);
    if (plan.status !=
        umbra::detail::ReceiveOrderDirectedInteractionStatus::applied) {
      throwReceiveOrderDirectedInteractionFailure(plan.status);
    }
    return plan;
  };

  static_cast<void>(planCurrentRequest());
  std::vector<InteractionParameterValue> sentParameters =
      copyInteractionParameterValues(parameterValues);
  VariableLengthData copiedTag(userSuppliedTag);
  auto plan = planCurrentRequest();

  auto const transportationName = umbra::detail::wideFromUtf8(plan.transportationName);
  if (!transportationName) {
    throw RTIinternalError(
        L"The composed FOM contains an invalid UTF-8 directed-interaction transportation name.");
  }
  auto const transportationValue = standardTransportationTypeValue(*transportationName);
  if (!transportationValue) {
    throw RTIinternalError(
        L"The embedded profile cannot deliver a directed interaction using this FOM transportation type.");
  }
  TransportationTypeHandle const transportationType =
      makeTransportationTypeHandle(*transportationValue);

  std::set<std::uint64_t> timeConstrainedRecipients;
  if (timeSnapshot.timeRegulating) {
    std::scoped_lock lock(federationManagementMutex());
    auto const snapshot = embeddedFederationManagement().registry().timeSnapshotFor(
        *federationName);
    if (!snapshot) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer exposes a coherent time snapshot.");
    }
    for (auto const& federate : snapshot->federates) {
      if (federate.time.timeConstrained) {
        timeConstrainedRecipients.insert(federate.membership.id);
      }
    }
  }

  std::vector<std::uint64_t> tsoRecipientIds;
  std::vector<umbra::detail::TsoDirectedInteractionRecipient> recipients;
  recipients.reserve(plan.recipients.size());
  for (auto const& candidate : plan.recipients) {
    if (!candidate.callbackRoute) {
      throw RTIinternalError(
          L"The embedded federation has an eligible directed-interaction recipient without a callback route.");
    }
    recipients.push_back({
        candidate.federateId,
        candidate.objectInstanceHandle,
        candidate.receivedInteractionClassHandle,
        candidate.receivedParameterHandles,
        candidate.callbackRoute});
    if (timeSnapshot.timeRegulating && plan.preferredOrderType == TIMESTAMP &&
        timeConstrainedRecipients.contains(candidate.federateId)) {
      tsoRecipientIds.push_back(candidate.federateId);
    }
  }

  std::uint64_t messageId = 0;
  if (timeSnapshot.timeRegulating && plan.preferredOrderType == TIMESTAMP &&
      !recipients.empty()) {
    umbra::detail::TsoDirectedInteractionMessage message;
    message.producingFederateId = *producingFederateId;
    message.objectInstanceHandle = *objectInstanceHandleValueResult;
    message.sentInteractionClassHandle = *interactionClassHandle;
    message.sentParameterHandles = *parameterHandles;
    message.parameters = sentParameters;
    message.userSuppliedTag = copiedTag;
    message.transportationName = plan.transportationName;
    message.sentOrderType = TIMESTAMP;
    message.receivedOrderType = TIMESTAMP;
    message.recipients = recipients;
    message.timestamp = timestamp;

    std::scoped_lock lock(federationManagementMutex());
    auto const result = embeddedFederationManagement().registry()
        .enqueueTsoDirectedInteraction(
            *federationName,
            std::move(message),
            tsoRecipientIds);
    if (result.status != umbra::detail::FederationTsoRegistryStatus::applied ||
        result.queueStatus != umbra::detail::TsoMessageQueueStatus::applied ||
        result.messageId == 0) {
      throw RTIinternalError(
          L"The embedded federation could not enqueue the timestamped directed interaction.");
    }
    messageId = result.messageId;
  }

  {
    std::scoped_lock lock(federationManagementMutex());
    auto const status = embeddedFederationManagement().registry()
        .recordSuccessfulInteractionSend(
            *federationName, *producingFederateId, true);
    if (status != umbra::detail::FederationRegistryStatus::applied) {
      throw RTIinternalError(
          L"The embedded federation lost the timestamped Send Directed Interaction membership before its accepted boundary.");
    }
  }

  for (auto const& recipient : recipients) {
    if (timeSnapshot.timeRegulating && plan.preferredOrderType == TIMESTAMP &&
        timeConstrainedRecipients.contains(recipient.receivingFederateId)) {
      continue;
    }
    queueTimestampedReceiveOrderDirectedInteraction(
        recipient.callbackRoute,
        *federationName,
        *producingFederateId,
        recipient.receivingFederateId,
        *objectInstanceHandleValueResult,
        *interactionClassHandle,
        sentParameters,
        copiedTag,
        transportationType,
        timestamp,
        timeSnapshot.timeRegulating && plan.preferredOrderType == TIMESTAMP
            ? TIMESTAMP
            : RECEIVE,
        RECEIVE,
        timeSnapshot.timeRegulating && plan.preferredOrderType == TIMESTAMP && messageId != 0
            ? std::optional<std::uint64_t>(messageId)
            : std::nullopt);
  }

  if (!timeSnapshot.timeRegulating || messageId == 0) {
    return MessageRetractionHandle();
  }
  return makeMessageRetractionHandle(messageId);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Timestamped Send Directed Interaction", exception);
    throw;
  }
}

MessageRetractionHandle UmbraRtiAmbassador::sendInteractionWithRegions(
    InteractionClassHandle const& interactionClass,
    ParameterHandleValueMap const& parameterValues,
    RegionHandleSet const& regions,
    VariableLengthData const& userSuppliedTag,
    LogicalTime const& time) {
  auto instrumentationScope = beginRtiCall("sendInteractionWithRegions");
  try {
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Timestamped Send Interaction With Regions");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Timestamped Send Interaction With Regions requires membership in a federation execution.");
    }
    if (!embeddedFederationManagement().registry().memberById(
            *joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    timeState = federateTimeState_;
  }

  auto const interactionClassHandle = interactionClassHandleValue(interactionClass);
  if (!interactionClassHandle) {
    throw InteractionClassNotDefined(
        L"Timestamped Send Interaction With Regions requires a defined InteractionClassHandle.");
  }
  auto const parameterHandles = interactionParameterHandleValues(parameterValues);
  if (!parameterHandles) {
    throw InteractionParameterNotDefined(
        L"Timestamped Send Interaction With Regions requires defined ParameterHandle values.");
  }
  std::set<std::uint64_t> regionValues;
  for (auto const& region : regions) {
    auto const value = regionHandleValue(region);
    if (!value) {
      throw InvalidRegion(
          L"Timestamped Send Interaction With Regions requires valid RegionHandle values.");
    }
    regionValues.insert(*value);
  }

  auto timestamp = cloneReferenceLogicalTime(timeState->implementationName(), time);
  auto const timeSnapshot = timeState->snapshot();
  validateTsoTimestamp(timeSnapshot, *timestamp);

  std::optional<std::wstring> federationName;
  std::optional<std::uint64_t> producingFederateId;
  auto planCurrentRequest = [&]() {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Timestamped Send Interaction With Regions");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ ||
        !federateTimeState_ || federateTimeState_ != timeState) {
      throw FederateNotExecutionMember(
          L"Timestamped Send Interaction With Regions requires an active joined federate.");
    }
    if (!federationName) {
      federationName = *joinedFederationName_;
      producingFederateId = *joinedFederateId_;
    } else if (*joinedFederationName_ != *federationName ||
               *joinedFederateId_ != *producingFederateId) {
      throw FederateNotExecutionMember(
          L"The RTI ambassador's federation membership changed during Timestamped Send Interaction With Regions.");
    }
    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(*federationName, *producingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto plan = registry.planReceiveOrderInteraction(
        *federationName,
        *producingFederateId,
        *interactionClassHandle,
        *parameterHandles,
        &regionValues);
    if (plan.status != umbra::detail::ReceiveOrderInteractionStatus::applied) {
      throwReceiveOrderInteractionFailure(plan.status);
    }
    return plan;
  };

  static_cast<void>(planCurrentRequest());
  std::vector<InteractionParameterValue> sentParameters =
      copyInteractionParameterValues(parameterValues);
  VariableLengthData copiedTag(userSuppliedTag);
  ParameterHandleValueMap reportParameterValues;
  for (auto const& [parameterHandle, parameterValue] : sentParameters) {
    reportParameterValues.emplace(makeParameterHandle(parameterHandle), parameterValue);
  }
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::interaction_class_handle,
       L"Interaction class designator",
       umbra::detail::formatMomInteractionClassHandle(interactionClass)},
      {umbra::detail::MomArgumentType::parameter_handle_value_map,
       L"Constrained set of interaction parameter designator and value pairs",
       umbra::detail::formatMomParameterHandleValueMap(reportParameterValues)},
      {umbra::detail::MomArgumentType::region_handle_set,
       L"Set of region designators",
       umbra::detail::formatMomRegionHandleSet(regions)},
      {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       umbra::detail::formatMomUserSuppliedTag(copiedTag)},
      {umbra::detail::MomArgumentType::logical_time,
       L"Optional timestamp",
       umbra::detail::formatMomLogicalTime(*timestamp)},
  };
  auto plan = planCurrentRequest();

  auto const transportationName = umbra::detail::wideFromUtf8(plan.transportationName);
  if (!transportationName) {
    throw RTIinternalError(
        L"The composed FOM contains an invalid UTF-8 regional interaction transportation name.");
  }
  auto const transportationValue = standardTransportationTypeValue(*transportationName);
  if (!transportationValue) {
    throw RTIinternalError(
        L"The embedded profile cannot deliver a regional interaction using this FOM transportation type.");
  }
  TransportationTypeHandle const transportationType =
      makeTransportationTypeHandle(*transportationValue);

  std::set<std::uint64_t> timeConstrainedRecipients;
  if (timeSnapshot.timeRegulating) {
    std::scoped_lock lock(federationManagementMutex());
    auto const snapshot = embeddedFederationManagement().registry().timeSnapshotFor(
        *federationName);
    if (!snapshot) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer exposes a coherent time snapshot.");
    }
    for (auto const& federate : snapshot->federates) {
      if (federate.time.timeConstrained) {
        timeConstrainedRecipients.insert(federate.membership.id);
      }
    }
  }

  std::uint64_t messageId = 0;
  if (timeSnapshot.timeRegulating && plan.preferredOrderType == TIMESTAMP) {
    std::vector<std::uint64_t> tsoRecipientIds;
    std::vector<std::uint64_t> allTimestampedRecipientIds;
    allTimestampedRecipientIds.reserve(plan.recipients.size());
    for (auto const& recipient : plan.recipients) {
      allTimestampedRecipientIds.push_back(recipient.federateId);
      if (timeConstrainedRecipients.contains(recipient.federateId)) {
        tsoRecipientIds.push_back(recipient.federateId);
      }
    }
    umbra::detail::TsoInteractionMessage message;
    message.producingFederateId = *producingFederateId;
    message.sentInteractionClassHandle = *interactionClassHandle;
    message.sentParameterHandles = *parameterHandles;
    message.parameters = sentParameters;
    message.userSuppliedTag = copiedTag;
    message.transportationName = plan.transportationName;
    message.sentOrderType = TIMESTAMP;
    message.receivedOrderType = TIMESTAMP;
    message.sentRegionHandles = regionValues;
    message.timestamp = timestamp;

    std::scoped_lock lock(federationManagementMutex());
    auto const result = embeddedFederationManagement().registry().enqueueTsoInteraction(
        *federationName,
        std::move(message),
        tsoRecipientIds,
        allTimestampedRecipientIds);
    if (result.status != umbra::detail::FederationTsoRegistryStatus::applied ||
        result.queueStatus != umbra::detail::TsoMessageQueueStatus::applied ||
        result.messageId == 0) {
      throw RTIinternalError(
          L"The embedded federation could not queue the timestamped regional interaction.");
    }
     messageId = result.messageId;
  }

  umbra::detail::MomServiceArgument returnedArgument{
      umbra::detail::MomArgumentType::null_value,
      L"",
      umbra::detail::formatMomNull()};
  if (messageId != 0U) {
    returnedArgument = {
        umbra::detail::MomArgumentType::message_retraction_handle,
        L"Message retraction designator",
        umbra::detail::formatMomMessageRetractionHandle(messageId)};
  }
  // The region-context TSO admission is the accepted service boundary. Keep
  // report routing on the same C++ reservation/Java callback path as the
  // nonregional overload, while preserving the supplied region set and the
  // retraction designator assigned by the federation registry.
  static_cast<void>(emitSelectedMomServiceReportInteraction(
      L"SendInteractionWithRegions",
      umbra::detail::MomServiceType::object_management,
      reportArguments,
      returnedArgument));

  {
    std::scoped_lock lock(federationManagementMutex());
    auto const status = embeddedFederationManagement().registry()
        .recordSuccessfulInteractionSend(
            *federationName, *producingFederateId, false);
    if (status != umbra::detail::FederationRegistryStatus::applied) {
      throw RTIinternalError(
          L"The embedded federation lost the timestamped regional Send Interaction membership before its accepted boundary.");
    }
  }

  for (auto const& recipient : plan.recipients) {
    if (timeSnapshot.timeRegulating && plan.preferredOrderType == TIMESTAMP &&
        timeConstrainedRecipients.contains(recipient.federateId)) {
      continue;
    }
    if (!recipient.callbackRoute) {
      throw RTIinternalError(
          L"The embedded federation has an eligible regional recipient without a callback route.");
    }
    queueTimestampedReceiveOrderInteraction(
        recipient.callbackRoute,
        *federationName,
        umbra::detail::InteractionProducer::joinedFederate(*producingFederateId),
        recipient.federateId,
        *interactionClassHandle,
        sentParameters,
        copiedTag,
        transportationType,
        timestamp,
        timeSnapshot.timeRegulating && plan.preferredOrderType == TIMESTAMP
            ? TIMESTAMP
            : RECEIVE,
        RECEIVE,
        timeSnapshot.timeRegulating && plan.preferredOrderType == TIMESTAMP && messageId != 0
            ? std::optional<std::uint64_t>(messageId)
            : std::nullopt,
        std::optional<std::set<std::uint64_t>>(regionValues));
  }

  if (!timeSnapshot.timeRegulating || messageId == 0) {
    return MessageRetractionHandle();
  }
  return makeMessageRetractionHandle(messageId);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Send Interaction With Regions", exception);
    throw;
  }
}

void UmbraRtiAmbassador::sendInteractionWithRegions(
    InteractionClassHandle const& interactionClass,
    ParameterHandleValueMap const& parameterValues,
    RegionHandleSet const& regions,
    VariableLengthData const& userSuppliedTag) {
  auto instrumentationScope = beginRtiCall("sendInteractionWithRegions");
  try {
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Send Interaction With Regions");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Send Interaction With Regions requires membership in a federation execution.");
    }
    if (!embeddedFederationManagement().registry().memberById(
            *joinedFederationName_,
            *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
  }

  auto const interactionClassValue = interactionClassHandleValue(interactionClass);
  if (!interactionClassValue) {
    throw InteractionClassNotDefined(
        L"Send Interaction With Regions requires a defined InteractionClassHandle.");
  }
  auto const parameterHandles = interactionParameterHandleValues(parameterValues);
  if (!parameterHandles) {
    throw InteractionParameterNotDefined(
        L"Send Interaction With Regions requires defined ParameterHandle values.");
  }
  std::set<std::uint64_t> regionValues;
  for (RegionHandle const& region : regions) {
    auto const value = regionHandleValue(region);
    if (!value) {
      throw InvalidRegion(
          L"Send Interaction With Regions requires valid RegionHandle values.");
    }
    regionValues.insert(*value);
  }

  std::optional<std::wstring> federationName;
  std::optional<std::uint64_t> producingFederateId;
  auto planCurrentRequest = [&]() {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Send Interaction With Regions");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Send Interaction With Regions requires membership in a federation execution.");
    }
    if (!federationName) {
      federationName = *joinedFederationName_;
      producingFederateId = *joinedFederateId_;
    } else if (*joinedFederationName_ != *federationName ||
               *joinedFederateId_ != *producingFederateId) {
      throw FederateNotExecutionMember(
          L"The RTI ambassador's federation membership changed during Send Interaction With Regions.");
    }

    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(*federationName, *producingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto plan = registry.planReceiveOrderInteraction(
        *federationName,
        *producingFederateId,
        *interactionClassValue,
        *parameterHandles,
        &regionValues);
    if (plan.status != umbra::detail::ReceiveOrderInteractionStatus::applied) {
      throwReceiveOrderInteractionFailure(plan.status);
    }
    return plan;
  };

  static_cast<void>(planCurrentRequest());
  std::vector<InteractionParameterValue> sentParameters =
      copyInteractionParameterValues(parameterValues);
  VariableLengthData copiedTag(userSuppliedTag);
  ParameterHandleValueMap reportParameterValues;
  for (auto const& [parameterHandle, parameterValue] : sentParameters) {
    reportParameterValues.emplace(makeParameterHandle(parameterHandle), parameterValue);
  }
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::interaction_class_handle,
       L"Interaction class designator",
       umbra::detail::formatMomInteractionClassHandle(interactionClass)},
      {umbra::detail::MomArgumentType::parameter_handle_value_map,
       L"Constrained set of interaction parameter designator and value pairs",
       umbra::detail::formatMomParameterHandleValueMap(reportParameterValues)},
      {umbra::detail::MomArgumentType::region_handle_set,
       L"Set of region designators",
       umbra::detail::formatMomRegionHandleSet(regions)},
      {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       umbra::detail::formatMomUserSuppliedTag(copiedTag)},
      {umbra::detail::MomArgumentType::null_value,
       L"Optional timestamp",
       umbra::detail::formatMomNull()},
  };
  auto plan = planCurrentRequest();

  auto const transportationName = umbra::detail::wideFromUtf8(plan.transportationName);
  if (!transportationName) {
    throw RTIinternalError(
        L"The composed FOM contains an invalid UTF-8 transportation type name.");
  }
  auto const transportationValue = standardTransportationTypeValue(*transportationName);
  if (!transportationValue) {
    throw RTIinternalError(
        L"The embedded profile cannot deliver an interaction using this FOM transportation type.");
  }
  TransportationTypeHandle const transportationType = makeTransportationTypeHandle(*transportationValue);

  struct Delivery {
    umbra::detail::InteractionCallbackRoute callbackRoute;
    std::uint64_t recipientId = 0;
  };
  std::vector<Delivery> deliveries;
  deliveries.reserve(plan.recipients.size());
  for (auto const& recipient : plan.recipients) {
    if (!recipient.callbackRoute) {
      throw RTIinternalError(
          L"The embedded federation has an eligible recipient without a callback route.");
    }
    deliveries.push_back({recipient.callbackRoute, recipient.federateId});
  }

  // The accepted region-context invocation is reported before its induced
  // receive callbacks, using the same callback-safe interaction sink as the
  // nonregional Send Interaction overload.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"SendInteractionWithRegions",
      umbra::detail::MomServiceType::object_management,
      reportArguments,
      true);

  {
    std::scoped_lock lock(federationManagementMutex());
    auto const status = embeddedFederationManagement().registry()
        .recordSuccessfulInteractionSend(
            *federationName, *producingFederateId, false);
    if (status != umbra::detail::FederationRegistryStatus::applied) {
      throw RTIinternalError(
          L"The embedded federation lost the regional Send Interaction membership before its accepted boundary.");
    }
  }

  for (auto& delivery : deliveries) {
    queueReceiveOrderInteraction(
        std::move(delivery.callbackRoute),
        *federationName,
        umbra::detail::InteractionProducer::joinedFederate(*producingFederateId),
        delivery.recipientId,
        *interactionClassValue,
        sentParameters,
        copiedTag,
        transportationType,
        std::optional<std::set<std::uint64_t>>(regionValues));
  }
  } catch (Exception const& exception) {
    emitExceptionReport(L"Send Interaction With Regions", exception);
    throw;
  }
}

void UmbraRtiAmbassador::changeAttributeOrderType(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandleSet const& attributes,
    OrderType orderType) {
  auto instrumentationScope = beginRtiCall("changeAttributeOrderType");
  try {
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Change Attribute Order Type");
  }
  auto const objectInstanceValue = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceValue) {
    throw ObjectInstanceNotKnown(
        L"Change Attribute Order Type requires a known ObjectInstanceHandle.");
  }
  auto const attributeValues = attributeHandleValues(attributes);
  if (!attributeValues) {
    throw AttributeNotDefined(
        L"Change Attribute Order Type requires defined AttributeHandle values.");
  }
  if (!standardOrderTypeName(orderType)) {
    throw RTIinternalError(
        L"The supplied OrderType is not supported by the embedded 2025 profile.");
  }
  // Section 8.24 names these supplied values "Object instance designator",
  // "Set of attribute designators", and "Order type". Table 5 fixes the
  // corresponding MIM types and value forms: the instance handle and order
  // type use their quoted forms, while AttributeHandleSet is an
  // Array<AttributeHandle>.
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::object_instance_handle,
       L"Object instance designator",
       umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
      {umbra::detail::MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       umbra::detail::formatMomAttributeHandleSet(attributes)},
      {umbra::detail::MomArgumentType::order_type,
       L"Order type",
       umbra::detail::formatMomOrderType(orderType)},
  };
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  requireFederationServiceOperationAvailable(L"Change Attribute Order Type");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Change Attribute Order Type requires membership in a federation execution.");
  }
  auto& registry = embeddedFederationManagement().registry();
  auto const status = registry.changeAttributeOrderType(
      *joinedFederationName_,
      *joinedFederateId_,
      *objectInstanceValue,
      *attributeValues,
      orderType);
  if (status != umbra::detail::AttributeOrderTypeChangeStatus::applied) {
    throwAttributeOrderTypeChangeFailure(status);
  }
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"ChangeAttributeOrderType",
      umbra::detail::MomServiceType::time_management,
      reportArguments);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Change Attribute Order Type", exception);
    throw;
  }
}

void UmbraRtiAmbassador::changeDefaultAttributeOrderType(
    ObjectClassHandle const& objectClass,
    AttributeHandleSet const& attributes,
    OrderType orderType) {
  auto instrumentationScope = beginRtiCall("changeDefaultAttributeOrderType");
  try {
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Change Default Attribute Order Type");
  }
  auto const objectClassValue = objectClassHandleValue(objectClass);
  if (!objectClassValue) {
    throw ObjectClassNotDefined(
        L"Change Default Attribute Order Type requires a defined ObjectClassHandle.");
  }
  auto const attributeValues = attributeHandleValues(attributes);
  if (!attributeValues) {
    throw AttributeNotDefined(
        L"Change Default Attribute Order Type requires defined AttributeHandle values.");
  }
  if (!standardOrderTypeName(orderType)) {
    throw RTIinternalError(
        L"The supplied OrderType is not supported by the embedded 2025 profile.");
  }
  // Section 8.25 names these supplied values "Object class designator",
  // "Set of attribute designators", and "Order type". Table 5 fixes the
  // corresponding MIM types and value forms: the class handle and order type
  // use their quoted forms, while AttributeHandleSet is an
  // Array<AttributeHandle>.
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::object_class_handle,
       L"Object class designator",
       umbra::detail::formatMomObjectClassHandle(objectClass)},
      {umbra::detail::MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       umbra::detail::formatMomAttributeHandleSet(attributes)},
      {umbra::detail::MomArgumentType::order_type,
       L"Order type",
       umbra::detail::formatMomOrderType(orderType)},
  };

  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  requireFederationServiceOperationAvailable(L"Change Default Attribute Order Type");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Change Default Attribute Order Type requires membership in a federation execution.");
  }
  auto& registry = embeddedFederationManagement().registry();
  auto const status = registry.changeDefaultAttributeOrderType(
      *joinedFederationName_,
      *joinedFederateId_,
      *objectClassValue,
      *attributeValues,
      orderType);
  if (status != umbra::detail::AttributeOrderTypeDefaultStatus::applied) {
    throwAttributeOrderTypeDefaultFailure(status);
  }
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"ChangeDefaultAttributeOrderType",
      umbra::detail::MomServiceType::time_management,
      reportArguments);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Change Default Attribute Order Type", exception);
    throw;
  }
}

void UmbraRtiAmbassador::changeInteractionOrderType(
    InteractionClassHandle const& interactionClass,
    OrderType orderType) {
  auto instrumentationScope = beginRtiCall("changeInteractionOrderType");
  try {
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Change Interaction Order Type");
  }
  auto const interactionClassValue = interactionClassHandleValue(interactionClass);
  if (!interactionClassValue) {
    throw InteractionClassNotDefined(
        L"Change Interaction Order Type requires a defined InteractionClassHandle.");
  }
  if (!standardOrderTypeName(orderType)) {
    throw RTIinternalError(
        L"The supplied OrderType is not supported by the embedded 2025 profile.");
  }
  // Section 8.26 names these supplied values "Interaction class designator"
  // and "Order type". Table 5 leaves HLAargumentName implementation-defined,
  // while fixing their type/value forms: type 27 String(handle.toString()) and
  // type 38 quoted RECEIVE/TIMESTAMP respectively.
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::interaction_class_handle,
       L"Interaction class designator",
       umbra::detail::formatMomInteractionClassHandle(interactionClass)},
      {umbra::detail::MomArgumentType::order_type,
       L"Order type",
       umbra::detail::formatMomOrderType(orderType)},
  };

  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  requireFederationServiceOperationAvailable(L"Change Interaction Order Type");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Change Interaction Order Type requires membership in a federation execution.");
  }
  auto& registry = embeddedFederationManagement().registry();
  auto const status = registry.changeInteractionOrderType(
      *joinedFederationName_,
      *joinedFederateId_,
      *interactionClassValue,
      orderType);
  if (status != umbra::detail::InteractionOrderTypeChangeStatus::applied) {
    throwInteractionOrderTypeChangeFailure(status);
  }
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"ChangeInteractionOrderType",
      umbra::detail::MomServiceType::time_management,
      reportArguments);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Change Interaction Order Type", exception);
    throw;
  }
}

void UmbraRtiAmbassador::requestAttributeTransportationTypeChange(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandleSet const& attributes,
    TransportationTypeHandle const& transportationType) {
  auto instrumentationScope = beginRtiCall("requestAttributeTransportationTypeChange");
  try {
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Request Attribute Transportation Type Change");
  }
  auto const objectInstanceValue = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceValue) {
    throw ObjectInstanceNotKnown(
        L"Request Attribute Transportation Type Change requires a known ObjectInstanceHandle.");
  }
  auto const attributeValues = attributeHandleValues(attributes);
  if (!attributeValues) {
    throw AttributeNotDefined(
        L"Request Attribute Transportation Type Change requires defined AttributeHandle values.");
  }
  auto const transportationName = embeddedTransportationNameFromHandle(transportationType);
  if (!transportationName) {
    throw InvalidTransportationTypeHandle(
        L"Request Attribute Transportation Type Change requires a supported TransportationTypeHandle.");
  }
  // Section 6.25 names these supplied values "Object instance designator",
  // "Set of attribute designators", and "Transportation type". Table 5
  // fixes the corresponding MIM types and value forms: the two handles use
  // quoted handle.toString(), while AttributeHandleSet is Array<AttributeHandle>.
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::object_instance_handle,
       L"Object instance designator",
       umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
      {umbra::detail::MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       umbra::detail::formatMomAttributeHandleSet(attributes)},
      {umbra::detail::MomArgumentType::transportation_type_handle,
       L"Transportation type",
       umbra::detail::formatMomTransportationTypeHandle(transportationType)},
  };

  std::wstring federationName;
  std::uint64_t requestingFederateId = 0;
  umbra::detail::AttributeTransportationTypeChangePlan plan;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Request Attribute Transportation Type Change");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Request Attribute Transportation Type Change requires membership in a federation execution.");
    }
    federationName = *joinedFederationName_;
    requestingFederateId = *joinedFederateId_;
    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(federationName, requestingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    plan = registry.planAttributeTransportationTypeChange(
        federationName,
        requestingFederateId,
        *objectInstanceValue,
        *attributeValues,
        *transportationName);
    if (plan.status !=
        umbra::detail::AttributeTransportationTypeChangeStatus::applied) {
      throwAttributeTransportationTypeChangeFailure(plan.status);
    }
    // The request succeeds once the registry accepts the pending change.
    // Section 6.25 separately makes the responding confirmation callback the
    // boundary at which the selected transportation takes effect.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"RequestAttributeTransportationTypeChange",
        umbra::detail::MomServiceType::object_management,
        reportArguments);
  }
  if (plan.requestId != 0) {
    if (!plan.callbackRoute) {
      throw RTIinternalError(
          L"The embedded federation has no attribute transportation confirmation callback route.");
    }
    queueConfirmAttributeTransportationTypeChange(
        std::move(plan.callbackRoute),
        std::move(federationName),
        requestingFederateId,
        plan.requestId);
  }
  } catch (Exception const& exception) {
    emitExceptionReport(L"Request Attribute Transportation Type Change", exception);
    throw;
  }
}

void UmbraRtiAmbassador::changeDefaultAttributeTransportationType(
    ObjectClassHandle const& objectClass,
    AttributeHandleSet const& attributes,
    TransportationTypeHandle const& transportationType) {
  auto instrumentationScope = beginRtiCall("changeDefaultAttributeTransportationType");
  try {
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Change Default Attribute Transportation Type");
  }
  auto const objectClassValue = objectClassHandleValue(objectClass);
  if (!objectClassValue) {
    throw ObjectClassNotDefined(
        L"Change Default Attribute Transportation Type requires a defined ObjectClassHandle.");
  }
  auto const attributeValues = attributeHandleValues(attributes);
  if (!attributeValues) {
    throw AttributeNotDefined(
        L"Change Default Attribute Transportation Type requires defined AttributeHandle values.");
  }
  auto const transportationName = embeddedTransportationNameFromHandle(transportationType);
  if (!transportationName) {
    throw InvalidTransportationTypeHandle(
        L"Change Default Attribute Transportation Type requires a supported TransportationTypeHandle.");
  }
  // Section 6.27 names these supplied values "Object class designator",
  // "Set of attribute designators", and "Transportation type". Table 5
  // fixes the corresponding MIM types and value forms: the class and
  // transportation handles use their quoted forms, while AttributeHandleSet
  // is an Array<AttributeHandle>.
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::object_class_handle,
       L"Object class designator",
       umbra::detail::formatMomObjectClassHandle(objectClass)},
      {umbra::detail::MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       umbra::detail::formatMomAttributeHandleSet(attributes)},
      {umbra::detail::MomArgumentType::transportation_type_handle,
       L"Transportation type",
       umbra::detail::formatMomTransportationTypeHandle(transportationType)},
  };

  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  requireFederationServiceOperationAvailable(
      L"Change Default Attribute Transportation Type");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Change Default Attribute Transportation Type requires membership in a federation execution.");
  }
  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  auto const status = registry.changeDefaultAttributeTransportationType(
      *joinedFederationName_,
      *joinedFederateId_,
      *objectClassValue,
      *attributeValues,
      *transportationName);
  if (status != umbra::detail::AttributeTransportationTypeDefaultStatus::applied) {
    throwAttributeTransportationTypeDefaultFailure(status);
  }
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"ChangeDefaultAttributeTransportationType",
      umbra::detail::MomServiceType::object_management,
      reportArguments);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Change Default Attribute Transportation Type", exception);
    throw;
  }
}

void UmbraRtiAmbassador::queryAttributeTransportationType(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandle const& attribute) {
  auto instrumentationScope = beginRtiCall("queryAttributeTransportationType");
  try {
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Query Attribute Transportation Type");
  }
  auto const objectInstanceValue = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceValue) {
    throw ObjectInstanceNotKnown(
        L"Query Attribute Transportation Type requires a known ObjectInstanceHandle.");
  }
  auto const attributeValue = attributeHandleValue(attribute);
  if (!attributeValue) {
    throw AttributeNotDefined(
        L"Query Attribute Transportation Type requires a defined AttributeHandle.");
  }
  // Section 6.28 names these supplied values "Object instance designator"
  // and "Attribute designator". Table 5 fixes their respective type-37 and
  // type-0 value forms as quoted handle.toString() text.
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::object_instance_handle,
       L"Object instance designator",
       umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
      {umbra::detail::MomArgumentType::attribute_handle,
       L"Attribute designator",
       umbra::detail::formatMomAttributeHandle(attribute)},
  };

  std::wstring federationName;
  std::uint64_t requestingFederateId = 0;
  umbra::detail::AttributeTransportationTypeQueryPlan plan;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Query Attribute Transportation Type");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Query Attribute Transportation Type requires membership in a federation execution.");
    }
    federationName = *joinedFederationName_;
    requestingFederateId = *joinedFederateId_;
    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(federationName, requestingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    plan = registry.planAttributeTransportationTypeQuery(
        federationName,
        requestingFederateId,
        *objectInstanceValue,
        *attributeValue);
    if (plan.status != umbra::detail::AttributeTransportationTypeQueryStatus::applied) {
      throwAttributeTransportationTypeQueryFailure(plan.status);
    }
    // The query is successfully invoked once the plan is accepted. Its
    // separate Report Attribute Transportation Type callback is not the
    // service-reporting boundary.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"QueryAttributeTransportationType",
        umbra::detail::MomServiceType::object_management,
        reportArguments);
  }
  if (!plan.callbackRoute) {
    throw RTIinternalError(
        L"The embedded federation has no attribute transportation report callback route.");
  }
  queueReportAttributeTransportationType(
      std::move(plan.callbackRoute),
      std::move(federationName),
      requestingFederateId,
      *objectInstanceValue,
      *attributeValue);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Query Attribute Transportation Type", exception);
    throw;
  }
}

void UmbraRtiAmbassador::requestInteractionTransportationTypeChange(
    InteractionClassHandle const& interactionClass,
    TransportationTypeHandle const& transportationType) {
  auto instrumentationScope = beginRtiCall("requestInteractionTransportationTypeChange");
  try {
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Request Interaction Transportation Type Change");
  }
  auto const interactionClassValue = interactionClassHandleValue(interactionClass);
  if (!interactionClassValue) {
    throw InteractionClassNotDefined(
        L"Request Interaction Transportation Type Change requires a defined InteractionClassHandle.");
  }
  auto const transportationName = embeddedTransportationNameFromHandle(transportationType);
  if (!transportationName) {
    throw InvalidTransportationTypeHandle(
        L"Request Interaction Transportation Type Change requires a supported TransportationTypeHandle.");
  }
  // Section 6.30 names these supplied values "Interaction class designator"
  // and "Transportation type". Table 5 leaves HLAargumentName
  // implementation-defined, while fixing the two quoted handle.toString()
  // representations as MIM argument types 27 and 59.
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::interaction_class_handle,
       L"Interaction class designator",
       umbra::detail::formatMomInteractionClassHandle(interactionClass)},
      {umbra::detail::MomArgumentType::transportation_type_handle,
       L"Transportation type",
       umbra::detail::formatMomTransportationTypeHandle(transportationType)},
  };

  std::wstring federationName;
  std::uint64_t requestingFederateId = 0;
  umbra::detail::InteractionTransportationTypeChangePlan plan;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Request Interaction Transportation Type Change");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Request Interaction Transportation Type Change requires membership in a federation execution.");
    }
    federationName = *joinedFederationName_;
    requestingFederateId = *joinedFederateId_;
    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(federationName, requestingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    plan = registry.planInteractionTransportationTypeChange(
        federationName,
        requestingFederateId,
        *interactionClassValue,
        *transportationName);
    if (plan.status != umbra::detail::InteractionTransportationTypeChangeStatus::applied) {
      throwInteractionTransportationTypeChangeFailure(plan.status);
    }
    // The request has succeeded once the registry has accepted the pending
    // change. Section 6.30 separately makes the later confirmation callback
    // the boundary at which the preferred transportation takes effect.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"RequestInteractionTransportationTypeChange",
        umbra::detail::MomServiceType::object_management,
        reportArguments);
  }
  if (!plan.callbackRoute) {
    throw RTIinternalError(
        L"The embedded federation has no interaction transportation confirmation callback route.");
  }
  queueConfirmInteractionTransportationTypeChange(
      std::move(plan.callbackRoute),
      std::move(federationName),
      requestingFederateId,
      *interactionClassValue);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Request Interaction Transportation Type Change", exception);
    throw;
  }
}

void UmbraRtiAmbassador::queryInteractionTransportationType(
    FederateHandle const& federate,
    InteractionClassHandle const& interactionClass) {
  auto instrumentationScope = beginRtiCall("queryInteractionTransportationType");
  try {
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Query Interaction Transportation Type");
  }
  auto const queriedFederateId = federateHandleValue(federate);
  if (!queriedFederateId) {
    throw FederateNotExecutionMember(
        L"Query Interaction Transportation Type requires a valid FederateHandle.");
  }
  auto const interactionClassValue = interactionClassHandleValue(interactionClass);
  if (!interactionClassValue) {
    throw InteractionClassNotDefined(
        L"Query Interaction Transportation Type requires a defined InteractionClassHandle.");
  }
  // Section 6.32 names these supplied values "Federate designator" and
  // "Interaction class designator". Table 5 fixes their respective type-15
  // and type-27 value forms as quoted handle.toString() text.
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::federate_handle,
       L"Federate designator",
       umbra::detail::formatMomFederateHandle(federate)},
      {umbra::detail::MomArgumentType::interaction_class_handle,
       L"Interaction class designator",
       umbra::detail::formatMomInteractionClassHandle(interactionClass)},
  };

  std::wstring federationName;
  std::uint64_t requestingFederateId = 0;
  umbra::detail::InteractionTransportationTypeQueryPlan plan;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Query Interaction Transportation Type");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Query Interaction Transportation Type requires membership in a federation execution.");
    }
    federationName = *joinedFederationName_;
    requestingFederateId = *joinedFederateId_;
    auto& registry = embeddedFederationManagement().registry();
    if (!registry.memberById(federationName, requestingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    plan = registry.planInteractionTransportationTypeQuery(
        federationName,
        requestingFederateId,
        *queriedFederateId,
        *interactionClassValue);
    if (plan.status != umbra::detail::InteractionTransportationTypeQueryStatus::applied) {
      throwInteractionTransportationTypeQueryFailure(plan.status);
    }
    // The query is successfully invoked once the plan is accepted. Its
    // separate Report Interaction Transportation Type callback is not the
    // service-reporting boundary.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"QueryInteractionTransportationType",
        umbra::detail::MomServiceType::object_management,
        reportArguments);
  }
  if (!plan.callbackRoute) {
    throw RTIinternalError(
        L"The embedded federation has no interaction transportation report callback route.");
  }
  queueReportInteractionTransportationType(
      std::move(plan.callbackRoute),
      std::move(federationName),
      requestingFederateId,
      *queriedFederateId,
      *interactionClassValue);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Query Interaction Transportation Type", exception);
    throw;
  }
}

TransportationTypeHandle UmbraRtiAmbassador::getTransportationTypeHandle(
    std::wstring const& transportationTypeName) {
  auto instrumentationScope = beginRtiCall("getTransportationTypeHandle");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Transportation Type Handle requires membership in a federation execution.");
  }

  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  auto const value = standardTransportationTypeValue(transportationTypeName);
  if (!value) {
    throw InvalidTransportationName(
        L"The embedded profile supports only HLAreliable and HLAbestEffort transportation types.");
  }
  return makeTransportationTypeHandle(*value);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Transportation Type Handle", exception);
    throw;
  }
}

std::wstring UmbraRtiAmbassador::getTransportationTypeName(
    TransportationTypeHandle const& transportationType) {
  auto instrumentationScope = beginRtiCall("getTransportationTypeName");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Transportation Type Name requires membership in a federation execution.");
  }

  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  auto const value = transportationTypeHandleValue(transportationType);
  auto const name = value ? standardTransportationTypeName(*value) : std::nullopt;
  if (!name) {
    throw InvalidTransportationTypeHandle(
        L"The supplied TransportationTypeHandle is not supported by this embedded profile.");
  }
  return std::wstring(*name);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Transportation Type Name", exception);
    throw;
  }
}

OrderType UmbraRtiAmbassador::getOrderType(std::wstring const& orderTypeName) {
  auto instrumentationScope = beginRtiCall("getOrderType");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Order Type requires membership in a federation execution.");
  }

  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  auto const value = standardOrderTypeValue(orderTypeName);
  if (!value) {
    throw InvalidOrderName(
        L"The embedded profile supports only the Receive and TimeStamp order names.");
  }
  return *value;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Order Type", exception);
    throw;
  }
}

std::wstring UmbraRtiAmbassador::getOrderName(OrderType orderType) {
  auto instrumentationScope = beginRtiCall("getOrderName");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Order Name requires membership in a federation execution.");
  }

  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  auto const name = standardOrderTypeName(orderType);
  if (!name) {
    throw InvalidOrderType(
        L"The supplied OrderType is not supported by this embedded profile.");
  }
  return *name;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Order Name", exception);
    throw;
  }
}

DimensionHandleSet UmbraRtiAmbassador::getAvailableDimensionsForObjectClass(
    ObjectClassHandle const& objectClass) {
  auto instrumentationScope = beginRtiCall("getAvailableDimensionsForObjectClass");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Available Dimensions for Object Class requires membership in a federation execution.");
  }

  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  auto const objectClassHandle = objectClassHandleValue(objectClass);
  if (!objectClassHandle) {
    throw InvalidObjectClassHandle(
        L"Get Available Dimensions for Object Class requires a valid ObjectClassHandle.");
  }
  auto const handles = registry.availableDimensionsForObjectClass(
      *joinedFederationName_, *objectClassHandle);
  if (!handles) {
    throw InvalidObjectClassHandle(
        L"The supplied ObjectClassHandle is not known in this federation execution.");
  }
  return makeDimensionHandleSet(*handles);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Available Dimensions For Object Class", exception);
    throw;
  }
}

DimensionHandleSet UmbraRtiAmbassador::getAvailableDimensionsForInteractionClass(
    InteractionClassHandle const& interactionClass) {
  auto instrumentationScope = beginRtiCall("getAvailableDimensionsForInteractionClass");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Available Dimensions for Interaction Class requires membership in a federation execution.");
  }

  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  auto const interactionClassHandle = interactionClassHandleValue(interactionClass);
  if (!interactionClassHandle) {
    throw InvalidInteractionClassHandle(
        L"Get Available Dimensions for Interaction Class requires a valid InteractionClassHandle.");
  }
  auto const handles = registry.availableDimensionsForInteractionClass(
      *joinedFederationName_, *interactionClassHandle);
  if (!handles) {
    throw InvalidInteractionClassHandle(
        L"The supplied InteractionClassHandle is not known in this federation execution.");
  }
  return makeDimensionHandleSet(*handles);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Available Dimensions For Interaction Class", exception);
    throw;
  }
}

DimensionHandle UmbraRtiAmbassador::getDimensionHandle(std::wstring const& dimensionName) {
  auto instrumentationScope = beginRtiCall("getDimensionHandle");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Dimension Handle requires membership in a federation execution.");
  }

  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  auto const encodedName = umbra::detail::utf8FromWide(dimensionName);
  if (!encodedName) {
    throw NameNotFound(L"The supplied dimension name is not valid UTF-8 text.");
  }
  auto const handle = registry.dimensionHandleFor(*joinedFederationName_, *encodedName);
  if (!handle) {
    throw NameNotFound(
        L"The supplied dimension name is not defined in this federation execution.");
  }
  return makeDimensionHandle(*handle);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Dimension Handle", exception);
    throw;
  }
}

std::wstring UmbraRtiAmbassador::getDimensionName(DimensionHandle const& dimension) {
  auto instrumentationScope = beginRtiCall("getDimensionName");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Dimension Name requires membership in a federation execution.");
  }

  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  auto const handle = dimensionHandleValue(dimension);
  if (!handle) {
    throw InvalidDimensionHandle(L"Get Dimension Name requires a valid DimensionHandle.");
  }
  auto const encodedName = registry.dimensionNameFor(*joinedFederationName_, *handle);
  if (!encodedName) {
    throw InvalidDimensionHandle(
        L"The supplied DimensionHandle is not known in this federation execution.");
  }
  auto const dimensionName = umbra::detail::wideFromUtf8(*encodedName);
  if (!dimensionName) {
    throw RTIinternalError(
        L"The embedded federation stored a dimension name that is not valid UTF-8 text.");
  }
  return *dimensionName;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Dimension Name", exception);
    throw;
  }
}

unsigned long UmbraRtiAmbassador::getDimensionUpperBound(DimensionHandle const& dimension) {
  auto instrumentationScope = beginRtiCall("getDimensionUpperBound");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Dimension Upper Bound requires membership in a federation execution.");
  }

  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  auto const handle = dimensionHandleValue(dimension);
  if (!handle) {
    throw InvalidDimensionHandle(
        L"Get Dimension Upper Bound requires a valid DimensionHandle.");
  }
  auto const upperBound = registry.dimensionUpperBoundFor(*joinedFederationName_, *handle);
  if (!upperBound) {
    throw InvalidDimensionHandle(
        L"The supplied DimensionHandle is not known in this federation execution.");
  }
  return *upperBound;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Dimension Upper Bound", exception);
    throw;
  }
}

RegionHandle UmbraRtiAmbassador::createRegion(DimensionHandleSet const& dimensions) {
  auto instrumentationScope = beginRtiCall("createRegion");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  requireFederationServiceOperationAvailable(L"Create Region");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Create Region requires membership in a federation execution.");
  }

  std::set<std::uint64_t> dimensionValues;
  for (DimensionHandle const& dimension : dimensions) {
    auto const value = dimensionHandleValue(dimension);
    if (!value) {
      throw InvalidDimensionHandle(L"Create Region requires valid DimensionHandle values.");
    }
    dimensionValues.insert(*value);
  }

  auto& registry = embeddedFederationManagement().registry();
  auto const result = registry.createRegion(
      *joinedFederationName_, *joinedFederateId_, dimensionValues);
  if (result.status != umbra::detail::RegionServiceStatus::applied) {
    throwRegionServiceFailure(result.status, L"Create Region");
  }
  return makeRegionHandle(result.regionHandle);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Create Region", exception);
    throw;
  }
}

void UmbraRtiAmbassador::commitRegionModifications(RegionHandleSet const& regions) {
  auto instrumentationScope = beginRtiCall("commitRegionModifications");
  try {
  std::wstring federationName;
  std::vector<umbra::detail::ObjectInstanceScopeChangeRecipient> changes;
  std::vector<umbra::detail::AttributeRelevanceAdvisoryRecipient>
      attributeRelevanceAdvisories;
  std::vector<umbra::detail::ObjectInstanceDiscoveryRecipient> momDiscoveries;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Commit Region Modifications");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Commit Region Modifications requires membership in a federation execution.");
    }

    std::set<std::uint64_t> regionValues;
    for (RegionHandle const& region : regions) {
      auto const value = regionHandleValue(region);
      if (!value) {
        throw InvalidRegion(L"Commit Region Modifications requires valid RegionHandle values.");
      }
      regionValues.insert(*value);
    }

    auto& registry = embeddedFederationManagement().registry();
    auto scopePlan = registry.commitRegionModificationsWithScopeChanges(
        *joinedFederationName_, *joinedFederateId_, regionValues);
    if (scopePlan.status != umbra::detail::RegionServiceStatus::applied) {
      throwRegionServiceFailure(scopePlan.status, L"Commit Region Modifications");
    }
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"CommitRegionModifications",
        umbra::detail::MomServiceType::data_distribution_management,
        {{umbra::detail::MomArgumentType::region_handle_set,
          L"Set of region designators",
          umbra::detail::formatMomRegionHandleSet(regions)}});
    federationName = *joinedFederationName_;
    changes = std::move(scopePlan.recipients);
    attributeRelevanceAdvisories = std::move(scopePlan.attributeRelevanceAdvisories);
    // A committed region mutation can make an existing RTI-owned MOM point
    // newly eligible without changing the subscription declaration. Reuse
    // the normal MOM planner so discovery is reserved exactly once and is
    // rechecked again at callback entry.
    momDiscoveries = registry.planJoinedFederateMomObjectDiscoveriesForFederate(
        federationName,
        *joinedFederateId_);
  }
  queueObjectInstanceScopeChanges(std::move(changes), federationName);
  queueAttributeRelevanceAdvisories(std::move(attributeRelevanceAdvisories), federationName);
  queueObjectInstanceDiscoveries(std::move(momDiscoveries), federationName);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Commit Region Modifications", exception);
    throw;
  }
}

void UmbraRtiAmbassador::deleteRegion(RegionHandle const& region) {
  auto instrumentationScope = beginRtiCall("deleteRegion");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  requireFederationServiceOperationAvailable(L"Delete Region");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Delete Region requires membership in a federation execution.");
  }
  auto const value = regionHandleValue(region);
  if (!value) {
    throw InvalidRegion(L"Delete Region requires a valid RegionHandle.");
  }

  auto& registry = embeddedFederationManagement().registry();
  auto const status = registry.deleteRegion(
      *joinedFederationName_, *joinedFederateId_, *value);
  if (status != umbra::detail::RegionServiceStatus::applied) {
    throwRegionServiceFailure(status, L"Delete Region");
  }
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"DeleteRegion",
      umbra::detail::MomServiceType::data_distribution_management,
      {{umbra::detail::MomArgumentType::region_handle,
        L"Region designator",
        umbra::detail::formatMomRegionHandle(region)}});
  } catch (Exception const& exception) {
    emitExceptionReport(L"Delete Region", exception);
    throw;
  }
}

DimensionHandleSet UmbraRtiAmbassador::getDimensionHandleSet(RegionHandle const& region) {
  auto instrumentationScope = beginRtiCall("getDimensionHandleSet");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  requireFederationServiceOperationAvailable(L"Get Dimension Handle Set");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Dimension Handle Set requires membership in a federation execution.");
  }
  auto const value = regionHandleValue(region);
  if (!value) {
    throw InvalidRegion(L"Get Dimension Handle Set requires a valid RegionHandle.");
  }

  auto& registry = embeddedFederationManagement().registry();
  auto const result = registry.dimensionHandleSetForRegion(
      *joinedFederationName_, *joinedFederateId_, *value);
  if (result.status != umbra::detail::RegionServiceStatus::applied) {
    throwRegionServiceFailure(result.status, L"Get Dimension Handle Set");
  }
  return makeDimensionHandleSet(result.dimensionHandles);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Dimension Handle Set", exception);
    throw;
  }
}

RangeBounds UmbraRtiAmbassador::getRangeBounds(
    RegionHandle const& region,
    DimensionHandle const& dimension) {
  auto instrumentationScope = beginRtiCall("getRangeBounds");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  requireFederationServiceOperationAvailable(L"Get Range Bounds");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Range Bounds requires membership in a federation execution.");
  }
  auto const regionValue = regionHandleValue(region);
  if (!regionValue) {
    throw InvalidRegion(L"Get Range Bounds requires a valid RegionHandle.");
  }
  auto const dimensionValue = dimensionHandleValue(dimension);
  if (!dimensionValue) {
    throw RegionDoesNotContainSpecifiedDimension(
        L"Get Range Bounds requires a dimension contained by the region.");
  }

  auto& registry = embeddedFederationManagement().registry();
  auto const result = registry.rangeBoundsForRegion(
      *joinedFederationName_, *joinedFederateId_, *regionValue, *dimensionValue);
  if (result.status != umbra::detail::RegionServiceStatus::applied) {
    throwRegionServiceFailure(result.status, L"Get Range Bounds");
  }
  return RangeBounds(result.range.lowerBound, result.range.upperBound);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Range Bounds", exception);
    throw;
  }
}

void UmbraRtiAmbassador::setRangeBounds(
    RegionHandle const& region,
    DimensionHandle const& dimension,
    RangeBounds const& rangeBounds) {
  auto instrumentationScope = beginRtiCall("setRangeBounds");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  requireFederationServiceOperationAvailable(L"Set Range Bounds");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Set Range Bounds requires membership in a federation execution.");
  }
  auto const regionValue = regionHandleValue(region);
  if (!regionValue) {
    throw InvalidRegion(L"Set Range Bounds requires a valid RegionHandle.");
  }
  auto const dimensionValue = dimensionHandleValue(dimension);
  if (!dimensionValue) {
    throw RegionDoesNotContainSpecifiedDimension(
        L"Set Range Bounds requires a dimension contained by the region.");
  }

  auto& registry = embeddedFederationManagement().registry();
  auto const status = registry.setRangeBounds(
      *joinedFederationName_,
      *joinedFederateId_,
      *regionValue,
      *dimensionValue,
      umbra::detail::RegionRangeBounds{
          rangeBounds.getLowerBound(),
          rangeBounds.getUpperBound(),
      });
  if (status != umbra::detail::RegionServiceStatus::applied) {
    throwRegionServiceFailure(status, L"Set Range Bounds");
  }
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"SetRangeBounds",
      umbra::detail::MomServiceType::data_distribution_management,
      {{umbra::detail::MomArgumentType::region_handle,
        L"Region handle",
        umbra::detail::formatMomRegionHandle(region)},
       {umbra::detail::MomArgumentType::dimension_handle,
        L"Dimension handle",
        umbra::detail::formatMomDimensionHandle(dimension)},
       {umbra::detail::MomArgumentType::number,
        L"Range lower bound",
        umbra::detail::formatMomNumber(std::to_wstring(rangeBounds.getLowerBound()))},
       {umbra::detail::MomArgumentType::number,
        L"Range upper bound",
        umbra::detail::formatMomNumber(std::to_wstring(rangeBounds.getUpperBound()))}});
  } catch (Exception const& exception) {
    emitExceptionReport(L"Set Range Bounds", exception);
    throw;
  }
}

unsigned long UmbraRtiAmbassador::normalizeServiceGroup(ServiceGroup serviceGroup) {
  auto instrumentationScope = beginRtiCall("normalizeServiceGroup");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Normalize Service Group requires membership in a federation execution.");
  }
  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }

  // HLAserviceGroup has the fixed standard upper bound of 7. The official
  // ServiceGroup indicator therefore already gives its point-range coordinate;
  // unlike execution-issued handles it must not be replaced with an arbitrary
  // per-execution value outside that dimension's domain.
  switch (serviceGroup) {
    case FEDERATION_MANAGEMENT:
    case DECLARATION_MANAGEMENT:
    case OBJECT_MANAGEMENT:
    case OWNERSHIP_MANAGEMENT:
    case TIME_MANAGEMENT:
    case DATA_DISTRIBUTION_MANAGEMENT:
    case SUPPORT_SERVICES:
      return static_cast<unsigned long>(serviceGroup);
  }
  throw InvalidServiceGroup(
      L"Normalize Service Group requires a supported ServiceGroup indicator.");
  } catch (Exception const& exception) {
    emitExceptionReport(L"Normalize Service Group", exception);
    throw;
  }
}

unsigned long UmbraRtiAmbassador::normalizeFederateHandle(
    FederateHandle const& federate) {
  auto instrumentationScope = beginRtiCall("normalizeFederateHandle");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Normalize Federate Handle requires membership in a federation execution.");
  }
  auto const federateValue = federateHandleValue(federate);
  if (!federateValue) {
    throw InvalidFederateHandle(
        L"Normalize Federate Handle requires a valid FederateHandle.");
  }

  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  auto const normalized = registry.normalizedFederateHandleValueFor(
      *joinedFederationName_, *federateValue);
  if (!normalized) {
    throw InvalidFederateHandle(
        L"The supplied FederateHandle is not valid in this federation execution.");
  }
  return *normalized;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Normalize Federate Handle", exception);
    throw;
  }
}

unsigned long UmbraRtiAmbassador::normalizeObjectClassHandle(
    ObjectClassHandle const& objectClass) {
  auto instrumentationScope = beginRtiCall("normalizeObjectClassHandle");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Normalize Object Class Handle requires membership in a federation execution.");
  }
  auto const objectClassValue = objectClassHandleValue(objectClass);
  if (!objectClassValue) {
    throw InvalidObjectClassHandle(
        L"Normalize Object Class Handle requires a valid ObjectClassHandle.");
  }

  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  auto const normalized = registry.normalizedObjectClassHandleValueFor(
      *joinedFederationName_, *objectClassValue);
  if (!normalized) {
    throw InvalidObjectClassHandle(
        L"The supplied ObjectClassHandle is not valid in this federation execution.");
  }
  return *normalized;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Normalize Object Class Handle", exception);
    throw;
  }
}

unsigned long UmbraRtiAmbassador::normalizeInteractionClassHandle(
    InteractionClassHandle const& interactionClass) {
  auto instrumentationScope = beginRtiCall("normalizeInteractionClassHandle");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Normalize Interaction Class Handle requires membership in a federation execution.");
  }
  auto const interactionClassValue = interactionClassHandleValue(interactionClass);
  if (!interactionClassValue) {
    throw InvalidInteractionClassHandle(
        L"Normalize Interaction Class Handle requires a valid InteractionClassHandle.");
  }

  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  auto const normalized = registry.normalizedInteractionClassHandleValueFor(
      *joinedFederationName_, *interactionClassValue);
  if (!normalized) {
    throw InvalidInteractionClassHandle(
        L"The supplied InteractionClassHandle is not valid in this federation execution.");
  }
  return *normalized;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Normalize Interaction Class Handle", exception);
    throw;
  }
}

unsigned long UmbraRtiAmbassador::normalizeObjectInstanceHandle(
    ObjectInstanceHandle const& objectInstance) {
  auto instrumentationScope = beginRtiCall("normalizeObjectInstanceHandle");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Normalize Object Instance Handle requires membership in a federation execution.");
  }
  auto const objectInstanceValue = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceValue) {
    throw InvalidObjectInstanceHandle(
        L"Normalize Object Instance Handle requires a valid ObjectInstanceHandle.");
  }

  auto& registry = embeddedFederationManagement().registry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  auto const normalized = registry.normalizedObjectInstanceHandleValueFor(
      *joinedFederationName_, *objectInstanceValue);
  if (!normalized) {
    throw InvalidObjectInstanceHandle(
        L"The supplied ObjectInstanceHandle is not valid in this federation execution.");
  }
  return *normalized;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Normalize Object Instance Handle", exception);
    throw;
  }
}

bool UmbraRtiAmbassador::getAttributeScopeAdvisorySwitch() const {
  auto instrumentationScope = beginRtiCall("getAttributeScopeAdvisorySwitch");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  requireFederationServiceOperationAvailable(L"Get Attribute Scope Advisory Switch");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Attribute Scope Advisory Switch requires membership in a federation execution.");
  }
  auto const switchValue = embeddedFederationManagement().registry()
      .attributeScopeAdvisorySwitchFor(*joinedFederationName_, *joinedFederateId_);
  if (!switchValue) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  return *switchValue;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Attribute Scope Advisory Switch", exception);
    throw;
  }
}

void UmbraRtiAmbassador::setAttributeScopeAdvisorySwitch(bool switchValue) {
  auto instrumentationScope = beginRtiCall("setAttributeScopeAdvisorySwitch");
  try {
  std::optional<JoinedFederateMomConditionalWork> momWork;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Set Attribute Scope Advisory Switch");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Set Attribute Scope Advisory Switch requires membership in a federation execution.");
    }
    auto const status = embeddedFederationManagement().registry().setAttributeScopeAdvisorySwitch(
        *joinedFederationName_, *joinedFederateId_, switchValue);
    if (status == umbra::detail::FederationRegistryStatus::applied) {
      appendSuccessfulVoidServiceReportToFileIfSelected(
          L"SetAttributeScopeAdvisorySwitch",
          umbra::detail::MomServiceType::support_services,
          {{umbra::detail::MomArgumentType::boolean,
            L"SwitchValue",
            umbra::detail::formatMomBoolean(switchValue)}});
      momWork = joinedFederateMomConditionalWorkFor(
          embeddedFederationManagement().registry(),
          *joinedFederationName_,
          *joinedFederateId_,
          {"HLAattributeScopeAdvisory"});
    } else if (status == umbra::detail::FederationRegistryStatus::federation_does_not_exist ||
               status == umbra::detail::FederationRegistryStatus::federate_not_member) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    } else {
      throw RTIinternalError(
          L"Set Attribute Scope Advisory Switch encountered an unknown outcome.");
    }
  }
  if (momWork) {
    queueJoinedFederateMomConditionalAttributeUpdate(
        momWork->federationName,
        momWork->objectInstanceHandle,
        std::move(momWork->attributeHandles));
  }
  } catch (Exception const& exception) {
    emitExceptionReport(L"Set Attribute Scope Advisory Switch", exception);
    throw;
  }
}

bool UmbraRtiAmbassador::getObjectClassRelevanceAdvisorySwitch() const {
  auto instrumentationScope = beginRtiCall("getObjectClassRelevanceAdvisorySwitch");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  requireFederationServiceOperationAvailable(
      L"Get Object Class Relevance Advisory Switch");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Object Class Relevance Advisory Switch requires membership in a federation execution.");
  }
  auto const switchValue = embeddedFederationManagement().registry()
      .objectClassRelevanceAdvisorySwitchFor(*joinedFederationName_, *joinedFederateId_);
  if (!switchValue) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  return *switchValue;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Object Class Relevance Advisory Switch", exception);
    throw;
  }
}

void UmbraRtiAmbassador::setObjectClassRelevanceAdvisorySwitch(bool switchValue) {
  auto instrumentationScope = beginRtiCall("setObjectClassRelevanceAdvisorySwitch");
  try {
  std::optional<JoinedFederateMomConditionalWork> momWork;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Set Object Class Relevance Advisory Switch");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Set Object Class Relevance Advisory Switch requires membership in a federation execution.");
    }
    auto const status = embeddedFederationManagement().registry()
        .setObjectClassRelevanceAdvisorySwitch(
            *joinedFederationName_, *joinedFederateId_, switchValue);
    if (status == umbra::detail::FederationRegistryStatus::applied) {
      appendSuccessfulVoidServiceReportToFileIfSelected(
          L"SetObjectClassRelevanceAdvisorySwitch",
          umbra::detail::MomServiceType::support_services,
          {{umbra::detail::MomArgumentType::boolean,
            L"SwitchValue",
            umbra::detail::formatMomBoolean(switchValue)}});
      momWork = joinedFederateMomConditionalWorkFor(
          embeddedFederationManagement().registry(),
          *joinedFederationName_,
          *joinedFederateId_,
          {"HLAobjectClassRelevanceAdvisory"});
    } else if (status == umbra::detail::FederationRegistryStatus::federation_does_not_exist ||
               status == umbra::detail::FederationRegistryStatus::federate_not_member) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    } else {
      throw RTIinternalError(
          L"Set Object Class Relevance Advisory Switch encountered an unknown outcome.");
    }
  }
  if (momWork) {
    queueJoinedFederateMomConditionalAttributeUpdate(
        momWork->federationName,
        momWork->objectInstanceHandle,
        std::move(momWork->attributeHandles));
  }
  } catch (Exception const& exception) {
    emitExceptionReport(L"Set Object Class Relevance Advisory Switch", exception);
    throw;
  }
}

bool UmbraRtiAmbassador::getAttributeRelevanceAdvisorySwitch() const {
  auto instrumentationScope = beginRtiCall("getAttributeRelevanceAdvisorySwitch");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  requireFederationServiceOperationAvailable(
      L"Get Attribute Relevance Advisory Switch");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Attribute Relevance Advisory Switch requires membership in a federation execution.");
  }
  auto const switchValue = embeddedFederationManagement().registry()
      .attributeRelevanceAdvisorySwitchFor(*joinedFederationName_, *joinedFederateId_);
  if (!switchValue) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  return *switchValue;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Attribute Relevance Advisory Switch", exception);
    throw;
  }
}

void UmbraRtiAmbassador::setAttributeRelevanceAdvisorySwitch(bool switchValue) {
  auto instrumentationScope = beginRtiCall("setAttributeRelevanceAdvisorySwitch");
  try {
  std::optional<JoinedFederateMomConditionalWork> momWork;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Set Attribute Relevance Advisory Switch");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Set Attribute Relevance Advisory Switch requires membership in a federation execution.");
    }
    auto const status = embeddedFederationManagement().registry()
        .setAttributeRelevanceAdvisorySwitch(
            *joinedFederationName_, *joinedFederateId_, switchValue);
    if (status == umbra::detail::FederationRegistryStatus::applied) {
      appendSuccessfulVoidServiceReportToFileIfSelected(
          L"SetAttributeRelevanceAdvisorySwitch",
          umbra::detail::MomServiceType::support_services,
          {{umbra::detail::MomArgumentType::boolean,
            L"SwitchValue",
            umbra::detail::formatMomBoolean(switchValue)}});
      momWork = joinedFederateMomConditionalWorkFor(
          embeddedFederationManagement().registry(),
          *joinedFederationName_,
          *joinedFederateId_,
          {"HLAattributeRelevanceAdvisory"});
    } else if (status == umbra::detail::FederationRegistryStatus::federation_does_not_exist ||
               status == umbra::detail::FederationRegistryStatus::federate_not_member) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    } else {
      throw RTIinternalError(
          L"Set Attribute Relevance Advisory Switch encountered an unknown outcome.");
    }
  }
  if (momWork) {
    queueJoinedFederateMomConditionalAttributeUpdate(
        momWork->federationName,
        momWork->objectInstanceHandle,
        std::move(momWork->attributeHandles));
  }
  } catch (Exception const& exception) {
    emitExceptionReport(L"Set Attribute Relevance Advisory Switch", exception);
    throw;
  }
}

bool UmbraRtiAmbassador::getInteractionRelevanceAdvisorySwitch() const {
  auto instrumentationScope = beginRtiCall("getInteractionRelevanceAdvisorySwitch");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  requireFederationServiceOperationAvailable(
      L"Get Interaction Relevance Advisory Switch");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Interaction Relevance Advisory Switch requires membership in a federation execution.");
  }
  auto const switchValue = embeddedFederationManagement().registry()
      .interactionRelevanceAdvisorySwitchFor(*joinedFederationName_, *joinedFederateId_);
  if (!switchValue) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  return *switchValue;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Interaction Relevance Advisory Switch", exception);
    throw;
  }
}

void UmbraRtiAmbassador::setInteractionRelevanceAdvisorySwitch(bool switchValue) {
  auto instrumentationScope = beginRtiCall("setInteractionRelevanceAdvisorySwitch");
  try {
  std::optional<JoinedFederateMomConditionalWork> momWork;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Set Interaction Relevance Advisory Switch");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Set Interaction Relevance Advisory Switch requires membership in a federation execution.");
    }
    auto const status = embeddedFederationManagement().registry()
        .setInteractionRelevanceAdvisorySwitch(
            *joinedFederationName_, *joinedFederateId_, switchValue);
    if (status == umbra::detail::FederationRegistryStatus::applied) {
      appendSuccessfulVoidServiceReportToFileIfSelected(
          L"SetInteractionRelevanceAdvisorySwitch",
          umbra::detail::MomServiceType::support_services,
          {{umbra::detail::MomArgumentType::boolean,
            L"SwitchValue",
            umbra::detail::formatMomBoolean(switchValue)}});
      momWork = joinedFederateMomConditionalWorkFor(
          embeddedFederationManagement().registry(),
          *joinedFederationName_,
          *joinedFederateId_,
          {"HLAinteractionRelevanceAdvisory"});
    } else if (status == umbra::detail::FederationRegistryStatus::federation_does_not_exist ||
               status == umbra::detail::FederationRegistryStatus::federate_not_member) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    } else {
      throw RTIinternalError(
          L"Set Interaction Relevance Advisory Switch encountered an unknown outcome.");
    }
  }
  if (momWork) {
    queueJoinedFederateMomConditionalAttributeUpdate(
        momWork->federationName,
        momWork->objectInstanceHandle,
        std::move(momWork->attributeHandles));
  }
  } catch (Exception const& exception) {
    emitExceptionReport(L"Set Interaction Relevance Advisory Switch", exception);
    throw;
  }
}

bool UmbraRtiAmbassador::getConveyRegionDesignatorSetsSwitch() const {
  auto instrumentationScope = beginRtiCall("getConveyRegionDesignatorSetsSwitch");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  requireFederationServiceOperationAvailable(
      L"Get Convey Region Designator Sets Switch");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Convey Region Designator Sets Switch requires membership in a federation execution.");
  }
  auto const switchValue = embeddedFederationManagement().registry()
      .conveyRegionDesignatorSetsSwitchFor(*joinedFederationName_, *joinedFederateId_);
  if (!switchValue) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  return *switchValue;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Convey Region Designator Sets Switch", exception);
    throw;
  }
}

void UmbraRtiAmbassador::setConveyRegionDesignatorSetsSwitch(bool switchValue) {
  auto instrumentationScope = beginRtiCall("setConveyRegionDesignatorSetsSwitch");
  try {
  std::optional<JoinedFederateMomConditionalWork> momWork;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Set Convey Region Designator Sets Switch");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Set Convey Region Designator Sets Switch requires membership in a federation execution.");
    }
    auto const status = embeddedFederationManagement().registry()
        .setConveyRegionDesignatorSetsSwitch(
            *joinedFederationName_, *joinedFederateId_, switchValue);
    if (status == umbra::detail::FederationRegistryStatus::applied) {
      appendSuccessfulVoidServiceReportToFileIfSelected(
          L"SetConveyRegionDesignatorSetsSwitch",
          umbra::detail::MomServiceType::support_services,
          {{umbra::detail::MomArgumentType::boolean,
            L"SwitchValue",
            umbra::detail::formatMomBoolean(switchValue)}});
      momWork = joinedFederateMomConditionalWorkFor(
          embeddedFederationManagement().registry(),
          *joinedFederationName_,
          *joinedFederateId_,
          {"HLAconveyRegionDesignatorSets"});
    } else if (status == umbra::detail::FederationRegistryStatus::federation_does_not_exist ||
               status == umbra::detail::FederationRegistryStatus::federate_not_member) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    } else {
      throw RTIinternalError(
          L"Set Convey Region Designator Sets Switch encountered an unknown outcome.");
    }
  }
  if (momWork) {
    queueJoinedFederateMomConditionalAttributeUpdate(
        momWork->federationName,
        momWork->objectInstanceHandle,
        std::move(momWork->attributeHandles));
  }
  } catch (Exception const& exception) {
    emitExceptionReport(L"Set Convey Region Designator Sets Switch", exception);
    throw;
  }
}

ResignAction UmbraRtiAmbassador::getAutomaticResignDirective() {
  auto instrumentationScope = beginRtiCall("getAutomaticResignDirective");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  requireFederationServiceOperationAvailable(L"Get Automatic Resign Directive");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Automatic Resign Directive requires membership in a federation execution.");
  }
  auto const action = embeddedFederationManagement().registry()
      .automaticResignActionFor(*joinedFederationName_, *joinedFederateId_);
  if (!action) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  return *action;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Automatic Resign Directive", exception);
    throw;
  }
}

void UmbraRtiAmbassador::setAutomaticResignDirective(ResignAction resignAction) {
  auto instrumentationScope = beginRtiCall("setAutomaticResignDirective");
  try {
  std::optional<JoinedFederateMomConditionalWork> momWork;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Set Automatic Resign Directive");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Set Automatic Resign Directive requires membership in a federation execution.");
    }
    requireValidResignAction(resignAction);
    auto const status = embeddedFederationManagement().registry().setAutomaticResignAction(
        *joinedFederationName_, *joinedFederateId_, resignAction);
    if (status == umbra::detail::FederationRegistryStatus::applied) {
      appendSuccessfulVoidServiceReportToFileIfSelected(
          L"SetAutomaticResignDirective",
          umbra::detail::MomServiceType::support_services,
          {{umbra::detail::MomArgumentType::resign_action,
            L"AutomaticResignDirective",
            umbra::detail::formatMomResignAction(resignAction)}});
      momWork = joinedFederateMomConditionalWorkFor(
          embeddedFederationManagement().registry(),
          *joinedFederationName_,
          *joinedFederateId_,
          {"HLAautomaticResignAction"});
    } else if (status == umbra::detail::FederationRegistryStatus::invalid_resign_action) {
      throw InvalidResignAction(
          L"The supplied resign action is not an IEEE 1516.1 ResignAction value.");
    } else if (status == umbra::detail::FederationRegistryStatus::federation_does_not_exist ||
               status == umbra::detail::FederationRegistryStatus::federate_not_member) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    } else {
      throw RTIinternalError(
          L"Set Automatic Resign Directive encountered an unknown outcome.");
    }
  }
  if (momWork) {
    queueJoinedFederateMomConditionalAttributeUpdate(
        momWork->federationName,
        momWork->objectInstanceHandle,
        std::move(momWork->attributeHandles));
  }
  } catch (Exception const& exception) {
    emitExceptionReport(L"Set Automatic Resign Directive", exception);
    throw;
  }
}

bool UmbraRtiAmbassador::getServiceReportingSwitch() const {
  auto instrumentationScope = beginRtiCall("getServiceReportingSwitch");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  requireFederationServiceOperationAvailable(L"Get Service Reporting Switch");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Service Reporting Switch requires membership in a federation execution.");
  }
  auto const switchValue = embeddedFederationManagement().registry()
      .serviceReportingSwitchFor(*joinedFederationName_, *joinedFederateId_);
  if (!switchValue) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  return *switchValue;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Service Reporting Switch", exception);
    throw;
  }
}

void UmbraRtiAmbassador::setServiceReportingSwitch(bool switchValue) {
  auto instrumentationScope = beginRtiCall("setServiceReportingSwitch");
  try {
  std::optional<JoinedFederateMomConditionalWork> momWork;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Set Service Reporting Switch");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Set Service Reporting Switch requires membership in a federation execution.");
    }
    auto const status = embeddedFederationManagement().registry().setServiceReportingSwitch(
        *joinedFederationName_, *joinedFederateId_, switchValue);
    if (status == umbra::detail::ServiceReportingSwitchStatus::applied) {
      momWork = joinedFederateMomConditionalWorkFor(
          embeddedFederationManagement().registry(),
          *joinedFederationName_,
          *joinedFederateId_,
          {"HLAserviceReporting"});
    } else if (status == umbra::detail::ServiceReportingSwitchStatus::
                   report_service_invocations_are_subscribed) {
      throw ReportServiceInvocationsAreSubscribed(
          L"The Service Reporting Switch cannot be enabled while the report-service-invocation interaction is subscribed.");
    } else if (status == umbra::detail::ServiceReportingSwitchStatus::
                   federation_does_not_exist ||
               status == umbra::detail::ServiceReportingSwitchStatus::federate_not_member) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    } else {
      throw RTIinternalError(L"Set Service Reporting Switch encountered an unknown outcome.");
    }
  }
  if (momWork) {
    queueJoinedFederateMomConditionalAttributeUpdate(
        momWork->federationName,
        momWork->objectInstanceHandle,
        std::move(momWork->attributeHandles));
  }
  } catch (Exception const& exception) {
    emitExceptionReport(L"Set Service Reporting Switch", exception);
    throw;
  }
}

bool UmbraRtiAmbassador::getExceptionReportingSwitch() const {
  auto instrumentationScope = beginRtiCall("getExceptionReportingSwitch");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  requireFederationServiceOperationAvailable(L"Get Exception Reporting Switch");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Exception Reporting Switch requires membership in a federation execution.");
  }
  auto const switchValue = embeddedFederationManagement().registry()
      .exceptionReportingSwitchFor(*joinedFederationName_, *joinedFederateId_);
  if (!switchValue) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  return *switchValue;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Exception Reporting Switch", exception);
    throw;
  }
}

void UmbraRtiAmbassador::setExceptionReportingSwitch(bool switchValue) {
  auto instrumentationScope = beginRtiCall("setExceptionReportingSwitch");
  try {
  std::optional<JoinedFederateMomConditionalWork> momWork;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Set Exception Reporting Switch");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Set Exception Reporting Switch requires membership in a federation execution.");
    }
    auto const status = embeddedFederationManagement().registry().setExceptionReportingSwitch(
        *joinedFederationName_, *joinedFederateId_, switchValue);
    if (status == umbra::detail::FederationRegistryStatus::applied) {
      appendSuccessfulVoidServiceReportToFileIfSelected(
          L"SetExceptionReportingSwitch",
          umbra::detail::MomServiceType::support_services,
          {{umbra::detail::MomArgumentType::boolean,
            L"SwitchValue",
            umbra::detail::formatMomBoolean(switchValue)}});
      momWork = joinedFederateMomConditionalWorkFor(
          embeddedFederationManagement().registry(),
          *joinedFederationName_,
          *joinedFederateId_,
          {"HLAexceptionReporting"});
    } else if (status == umbra::detail::FederationRegistryStatus::federation_does_not_exist ||
               status == umbra::detail::FederationRegistryStatus::federate_not_member) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    } else {
      throw RTIinternalError(
          L"Set Exception Reporting Switch encountered an unknown outcome.");
    }
  }
  if (momWork) {
    queueJoinedFederateMomConditionalAttributeUpdate(
        momWork->federationName,
        momWork->objectInstanceHandle,
        std::move(momWork->attributeHandles));
  }
  } catch (Exception const& exception) {
    emitExceptionReport(L"Set Exception Reporting Switch", exception);
    throw;
  }
}

bool UmbraRtiAmbassador::getSendServiceReportsToFileSwitch() const {
  auto instrumentationScope = beginRtiCall("getSendServiceReportsToFileSwitch");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  requireFederationServiceOperationAvailable(L"Get Send Service Reports To File Switch");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Send Service Reports To File Switch requires membership in a federation execution.");
  }
  auto const switchValue = embeddedFederationManagement().registry()
      .sendServiceReportsToFileSwitchFor(*joinedFederationName_, *joinedFederateId_);
  if (!switchValue) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  return *switchValue;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Send Service Reports To File Switch", exception);
    throw;
  }
}

void UmbraRtiAmbassador::setSendServiceReportsToFileSwitch(bool switchValue) {
  auto instrumentationScope = beginRtiCall("setSendServiceReportsToFileSwitch");
  try {
  std::optional<JoinedFederateMomConditionalWork> momWork;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Set Send Service Reports To File Switch");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Set Send Service Reports To File Switch requires membership in a federation execution.");
    }
    auto const status = embeddedFederationManagement().registry()
        .setSendServiceReportsToFileSwitch(
            *joinedFederationName_, *joinedFederateId_, switchValue);
    if (status == umbra::detail::FederationRegistryStatus::applied) {
      momWork = joinedFederateMomConditionalWorkFor(
          embeddedFederationManagement().registry(),
          *joinedFederationName_,
          *joinedFederateId_,
          {"HLAsendServiceReportsToFile"});
    } else if (status == umbra::detail::FederationRegistryStatus::federation_does_not_exist ||
               status == umbra::detail::FederationRegistryStatus::federate_not_member) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    } else {
      throw RTIinternalError(
          L"Set Send Service Reports To File Switch encountered an unknown outcome.");
    }
  }
  if (momWork) {
    queueJoinedFederateMomConditionalAttributeUpdate(
        momWork->federationName,
        momWork->objectInstanceHandle,
        std::move(momWork->attributeHandles));
  }
  } catch (Exception const& exception) {
    emitExceptionReport(L"Set Send Service Reports To File Switch", exception);
    throw;
  }
}

bool UmbraRtiAmbassador::getAutoProvideSwitch() const {
  auto instrumentationScope = beginRtiCall("getAutoProvideSwitch");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  requireFederationServiceOperationAvailable(L"Get Auto Provide Switch");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Auto Provide Switch requires membership in a federation execution.");
  }
  auto const switchValue = embeddedFederationManagement().registry().autoProvideSwitchFor(
      *joinedFederationName_,
      *joinedFederateId_);
  if (!switchValue) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  return *switchValue;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Auto Provide Switch", exception);
    throw;
  }
}

bool UmbraRtiAmbassador::getDelaySubscriptionEvaluationSwitch() const {
  auto instrumentationScope = beginRtiCall("getDelaySubscriptionEvaluationSwitch");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  requireFederationServiceOperationAvailable(
      L"Get Delay Subscription Evaluation Switch");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Delay Subscription Evaluation Switch requires membership in a federation execution.");
  }
  auto const switchValue = embeddedFederationManagement().registry()
      .delaySubscriptionEvaluationSwitchFor(*joinedFederationName_, *joinedFederateId_);
  if (!switchValue) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  return *switchValue;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Delay Subscription Evaluation Switch", exception);
    throw;
  }
}

bool UmbraRtiAmbassador::getAdvisoriesUseKnownClassSwitch() const {
  auto instrumentationScope = beginRtiCall("getAdvisoriesUseKnownClassSwitch");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  requireFederationServiceOperationAvailable(L"Get Advisories Use Known Class Switch");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Advisories Use Known Class Switch requires membership in a federation execution.");
  }
  auto const switchValue = embeddedFederationManagement().registry()
      .advisoriesUseKnownClassSwitchFor(*joinedFederationName_, *joinedFederateId_);
  if (!switchValue) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  return *switchValue;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Advisories Use Known Class Switch", exception);
    throw;
  }
}

bool UmbraRtiAmbassador::getAllowRelaxedDDMSwitch() const {
  auto instrumentationScope = beginRtiCall("getAllowRelaxedDDMSwitch");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  requireFederationServiceOperationAvailable(L"Get Allow Relaxed DDM Switch");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Allow Relaxed DDM Switch requires membership in a federation execution.");
  }
  auto const switchValue = embeddedFederationManagement().registry()
      .allowRelaxedDDMSwitchFor(*joinedFederationName_, *joinedFederateId_);
  if (!switchValue) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  return *switchValue;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Allow Relaxed DDM Switch", exception);
    throw;
  }
}

bool UmbraRtiAmbassador::getNonRegulatedGrantSwitch() const {
  auto instrumentationScope = beginRtiCall("getNonRegulatedGrantSwitch");
  try {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  requireFederationServiceOperationAvailable(L"Get Non Regulated Grant Switch");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Non Regulated Grant Switch requires membership in a federation execution.");
  }
  auto const switchValue = embeddedFederationManagement().registry()
      .nonRegulatedGrantSwitchFor(*joinedFederationName_, *joinedFederateId_);
  if (!switchValue) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  return *switchValue;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Non Regulated Grant Switch", exception);
    throw;
  }
}

FederateHandle UmbraRtiAmbassador::decodeFederateHandle(
    VariableLengthData const& encodedValue) const {
  auto instrumentationScope = beginRtiCall("decodeFederateHandle");
  try {
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Decode Federate Handle requires membership in a federation execution.");
  }
  return ::rti1516_2025::umbra_binding_detail::decodeFederateHandle(encodedValue);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Decode Federate Handle", exception);
    throw;
  }
}

ObjectClassHandle UmbraRtiAmbassador::decodeObjectClassHandle(
    VariableLengthData const& encodedValue) const {
  auto instrumentationScope = beginRtiCall("decodeObjectClassHandle");
  try {
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Decode Object Class Handle requires membership in a federation execution.");
  }
  return ::rti1516_2025::umbra_binding_detail::decodeObjectClassHandle(encodedValue);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Decode Object Class Handle", exception);
    throw;
  }
}

InteractionClassHandle UmbraRtiAmbassador::decodeInteractionClassHandle(
    VariableLengthData const& encodedValue) const {
  auto instrumentationScope = beginRtiCall("decodeInteractionClassHandle");
  try {
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Decode Interaction Class Handle requires membership in a federation execution.");
  }
  return ::rti1516_2025::umbra_binding_detail::decodeInteractionClassHandle(encodedValue);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Decode Interaction Class Handle", exception);
    throw;
  }
}

ObjectInstanceHandle UmbraRtiAmbassador::decodeObjectInstanceHandle(
    VariableLengthData const& encodedValue) const {
  auto instrumentationScope = beginRtiCall("decodeObjectInstanceHandle");
  try {
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Decode Object Instance Handle requires membership in a federation execution.");
  }
  return ::rti1516_2025::umbra_binding_detail::decodeObjectInstanceHandle(encodedValue);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Decode Object Instance Handle", exception);
    throw;
  }
}

AttributeHandle UmbraRtiAmbassador::decodeAttributeHandle(
    VariableLengthData const& encodedValue) const {
  auto instrumentationScope = beginRtiCall("decodeAttributeHandle");
  try {
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Decode Attribute Handle requires membership in a federation execution.");
  }
  return ::rti1516_2025::umbra_binding_detail::decodeAttributeHandle(encodedValue);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Decode Attribute Handle", exception);
    throw;
  }
}

ParameterHandle UmbraRtiAmbassador::decodeParameterHandle(
    VariableLengthData const& encodedValue) const {
  auto instrumentationScope = beginRtiCall("decodeParameterHandle");
  try {
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Decode Parameter Handle requires membership in a federation execution.");
  }
  return ::rti1516_2025::umbra_binding_detail::decodeParameterHandle(encodedValue);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Decode Parameter Handle", exception);
    throw;
  }
}

DimensionHandle UmbraRtiAmbassador::decodeDimensionHandle(
    VariableLengthData const& encodedValue) const {
  auto instrumentationScope = beginRtiCall("decodeDimensionHandle");
  try {
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Decode Dimension Handle requires membership in a federation execution.");
  }
  return ::rti1516_2025::umbra_binding_detail::decodeDimensionHandle(encodedValue);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Decode Dimension Handle", exception);
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

void UmbraRtiAmbassador::enableTimeRegulation(LogicalTimeInterval const& lookahead) {
  auto instrumentationScope = beginRtiCall("enableTimeRegulation");
  try {
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  std::shared_ptr<CallbackSession> callbackSession;
  std::wstring federationName;
  std::uint64_t federateId = 0;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Enable Time Regulation");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Enable Time Regulation requires a joined federate with initialized logical time.");
    }

    timeState = federateTimeState_;
    callbackSession = callbackSession_;
    federationName = *joinedFederationName_;
    federateId = *joinedFederateId_;
    if (!callbackSession) {
      throw RTIinternalError(
          L"The embedded connection has no federate ambassador callback recipient.");
    }
  }

  // LogicalTimeInterval is caller-provided polymorphic state. Decode a private
  // reference interval outside Umbra locks, then retain only that copy while
  // the regulation request awaits its official callback.
  auto requestedLookahead = cloneReferenceLogicalTimeInterval(
      timeState->implementationName(),
      lookahead);
  // Section 8.2.1 names the one supplied argument Lookahead, while Table 5
  // gives LogicalTimeInterval the quoted interval.toString() form. Format the
  // private reference copy before locking rather than invoking a caller-owned
  // polymorphic object while the federation state is locked.
  auto const reportLookaheadArgument = umbra::detail::MomServiceArgument{
      umbra::detail::MomArgumentType::logical_time_interval,
      L"Lookahead",
      umbra::detail::formatMomLogicalTimeInterval(*requestedLookahead),
  };
  umbra::detail::FederateTimeEnableResult result;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Enable Time Regulation");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_ ||
        *joinedFederationName_ != federationName || federateTimeState_ != timeState) {
      throw FederateNotExecutionMember(
          L"Enable Time Regulation requires an active joined federate with initialized logical time.");
    }
    result = timeState->requestTimeRegulation(std::move(requestedLookahead));
    if (result.status == umbra::detail::FederateTimeEnableStatus::applied) {
      // Acceptance makes the request reportable. The distinct Time Regulation
      // Enabled callback remains the transition that establishes the role.
      appendSuccessfulVoidServiceReportToFileIfSelected(
          L"EnableTimeRegulation",
          umbra::detail::MomServiceType::time_management,
          {reportLookaheadArgument});
    }
  }
  switch (result.status) {
    case umbra::detail::FederateTimeEnableStatus::applied:
      break;
    case umbra::detail::FederateTimeEnableStatus::time_advance_pending:
      throw InTimeAdvancingState(
          L"Enable Time Regulation cannot run while the joined federate has a time advance pending.");
    case umbra::detail::FederateTimeEnableStatus::request_pending:
      throw RequestForTimeRegulationPending(
          L"The joined federate already has an Enable Time Regulation request awaiting its callback.");
    case umbra::detail::FederateTimeEnableStatus::already_enabled:
      throw TimeRegulationAlreadyEnabled(
          L"Time regulation is already enabled for the joined federate.");
    case umbra::detail::FederateTimeEnableStatus::invalid_lookahead:
      throw InvalidLookahead(
          L"The requested lookahead is invalid for the joined federation's implementation.");
    case umbra::detail::FederateTimeEnableStatus::inactive:
      throw FederateNotExecutionMember(
          L"The joined federate's logical-time state is no longer active.");
    case umbra::detail::FederateTimeEnableStatus::generation_exhausted:
      throw RTIinternalError(
          L"Umbra exhausted the embedded temporal-request generation space.");
  }

  // This process-local development profile completes the callback at the
  // current logical time. It has bounded local TSO routing, but a
  // transport-bearing coordinator must calculate the complete regulation
  // boundary before this behavior can be reused outside this scope.
  callbacks_->submit([
      callbackSession = std::move(callbackSession),
      timeState = std::move(timeState),
      federationName = std::move(federationName),
      federateId,
      generation = result.generation] {
    std::vector<umbra::detail::FederationTimeGrantDispatch> newlyEligible;
    bool granted = false;
    callbackSession->invoke([timeState, federationName, generation, &newlyEligible, &granted](FederateAmbassador& recipient) {
      std::shared_ptr<LogicalTime const> enabledTime;
      {
        std::scoped_lock lock(federationManagementMutex());
        enabledTime = timeState->grantTimeRegulation(generation);
        if (!enabledTime) {
          return;
        }
        granted = true;
        auto scheduled = embeddedFederationManagement().registry().reevaluateTimeAdvanceGrants(
            federationName);
        if (scheduled.status == umbra::detail::FederationTimeGrantStatus::applied) {
          newlyEligible = std::move(scheduled.dispatches);
        }
      }
      recipient.timeRegulationEnabled(*enabledTime);
    });
    if (granted) {
      queueJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
          federationName,
          federateId,
          {"HLAtimeRegulating"});
    }
    submitTimeAdvanceGrantDispatches(std::move(newlyEligible));
  });
  } catch (Exception const& exception) {
    emitExceptionReport(L"Enable Time Regulation", exception);
    throw;
  }
}

void UmbraRtiAmbassador::disableTimeRegulation() {
  auto instrumentationScope = beginRtiCall("disableTimeRegulation");
  try {
  umbra::detail::FederateTimeDisableStatus result;
  std::vector<umbra::detail::FederationTimeGrantDispatch> newlyEligible;
  std::optional<JoinedFederateMomConditionalWork> momWork;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Disable Time Regulation");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Disable Time Regulation requires a joined federate with initialized logical time.");
    }
    result = federateTimeState_->disableTimeRegulation();
    if (result == umbra::detail::FederateTimeDisableStatus::applied) {
      appendSuccessfulVoidServiceReportToFileIfSelected(
          L"DisableTimeRegulation",
          umbra::detail::MomServiceType::time_management,
          {});
      auto scheduled = embeddedFederationManagement().registry().reevaluateTimeAdvanceGrants(
          *joinedFederationName_);
      if (scheduled.status == umbra::detail::FederationTimeGrantStatus::applied) {
        newlyEligible = std::move(scheduled.dispatches);
      }
      momWork = joinedFederateMomConditionalWorkFor(
          embeddedFederationManagement().registry(),
          *joinedFederationName_,
          *joinedFederateId_,
          {"HLAtimeRegulating"});
    }
  }

  if (momWork) {
    queueJoinedFederateMomConditionalAttributeUpdate(
        momWork->federationName,
        momWork->objectInstanceHandle,
        std::move(momWork->attributeHandles));
  }

  switch (result) {
    case umbra::detail::FederateTimeDisableStatus::applied:
      submitTimeAdvanceGrantDispatches(std::move(newlyEligible));
      return;
    case umbra::detail::FederateTimeDisableStatus::not_enabled:
      throw TimeRegulationIsNotEnabled(
          L"Time regulation is not enabled for the joined federate.");
    case umbra::detail::FederateTimeDisableStatus::inactive:
      throw FederateNotExecutionMember(
          L"The joined federate's logical-time state is no longer active.");
  }
  throw RTIinternalError(L"Umbra encountered an unknown time-regulation disable outcome.");
  } catch (Exception const& exception) {
    emitExceptionReport(L"Disable Time Regulation", exception);
    throw;
  }
}

void UmbraRtiAmbassador::enableTimeConstrained() {
  auto instrumentationScope = beginRtiCall("enableTimeConstrained");
  try {
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  std::shared_ptr<CallbackSession> callbackSession;
  std::wstring federationName;
  std::uint64_t federateId = 0;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Enable Time Constrained");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Enable Time Constrained requires a joined federate with initialized logical time.");
    }

    timeState = federateTimeState_;
    callbackSession = callbackSession_;
    federationName = *joinedFederationName_;
    federateId = *joinedFederateId_;
    if (!callbackSession) {
      throw RTIinternalError(
          L"The embedded connection has no federate ambassador callback recipient.");
    }
  }

  umbra::detail::FederateTimeEnableResult result;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Enable Time Constrained");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_ ||
        federateTimeState_ != timeState) {
      throw FederateNotExecutionMember(
          L"Enable Time Constrained requires an active joined federate with initialized logical time.");
    }
    result = timeState->requestTimeConstrained();
    if (result.status == umbra::detail::FederateTimeEnableStatus::applied) {
      // The service invocation succeeds when the request is accepted; the
      // distinct Time Constrained Enabled callback remains queued below.
      appendSuccessfulVoidServiceReportToFileIfSelected(
          L"EnableTimeConstrained",
          umbra::detail::MomServiceType::time_management,
          {});
    }
  }
  switch (result.status) {
    case umbra::detail::FederateTimeEnableStatus::applied:
      break;
    case umbra::detail::FederateTimeEnableStatus::time_advance_pending:
      throw InTimeAdvancingState(
          L"Enable Time Constrained cannot run while the joined federate has a time advance pending.");
    case umbra::detail::FederateTimeEnableStatus::request_pending:
      throw RequestForTimeConstrainedPending(
          L"The joined federate already has an Enable Time Constrained request awaiting its callback.");
    case umbra::detail::FederateTimeEnableStatus::already_enabled:
      throw TimeConstrainedAlreadyEnabled(
          L"Time constrained is already enabled for the joined federate.");
    case umbra::detail::FederateTimeEnableStatus::invalid_lookahead:
      throw RTIinternalError(
          L"The embedded time-constrained request returned an unexpected lookahead failure.");
    case umbra::detail::FederateTimeEnableStatus::inactive:
      throw FederateNotExecutionMember(
          L"The joined federate's logical-time state is no longer active.");
    case umbra::detail::FederateTimeEnableStatus::generation_exhausted:
      throw RTIinternalError(
          L"Umbra exhausted the embedded temporal-request generation space.");
  }

  callbacks_->submit([
      callbackSession = std::move(callbackSession),
      timeState = std::move(timeState),
      federationName = std::move(federationName),
      federateId,
      generation = result.generation] {
    bool granted = false;
    callbackSession->invoke([timeState, generation, &granted](FederateAmbassador& recipient) {
      auto enabledTime = timeState->grantTimeConstrained(generation);
      if (!enabledTime) {
        return;
      }
      granted = true;
      recipient.timeConstrainedEnabled(*enabledTime);
    });
    if (granted) {
      queueJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
          federationName,
          federateId,
          {"HLAtimeConstrained"});
    }
  });
  } catch (Exception const& exception) {
    emitExceptionReport(L"Enable Time Constrained", exception);
    throw;
  }
}

void UmbraRtiAmbassador::disableTimeConstrained() {
  auto instrumentationScope = beginRtiCall("disableTimeConstrained");
  try {
  umbra::detail::FederateTimeDisableStatus result;
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  std::vector<umbra::detail::FederationTimeGrantDispatch> newlyEligible;
  std::optional<JoinedFederateMomConditionalWork> momWork;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Disable Time Constrained");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Disable Time Constrained requires a joined federate with initialized logical time.");
    }
    timeState = federateTimeState_;
    result = timeState->disableTimeConstrained();
    if (result == umbra::detail::FederateTimeDisableStatus::applied) {
      appendSuccessfulVoidServiceReportToFileIfSelected(
          L"DisableTimeConstrained",
          umbra::detail::MomServiceType::time_management,
          {});
      auto scheduled = embeddedFederationManagement().registry().reevaluateTimeAdvanceGrants(
          *joinedFederationName_);
      if (scheduled.status == umbra::detail::FederationTimeGrantStatus::applied) {
        newlyEligible = std::move(scheduled.dispatches);
      }
      momWork = joinedFederateMomConditionalWorkFor(
          embeddedFederationManagement().registry(),
          *joinedFederationName_,
          *joinedFederateId_,
          {"HLAtimeConstrained"});
    }
  }

  if (momWork) {
    queueJoinedFederateMomConditionalAttributeUpdate(
        momWork->federationName,
        momWork->objectInstanceHandle,
        std::move(momWork->attributeHandles));
  }

  switch (result) {
    case umbra::detail::FederateTimeDisableStatus::applied:
      // A previously constrained TAR may no longer be GALT-bounded. Queue
      // any resulting grant only after the shared runtime locks are released.
      flushAsynchronousReceiveCallbacks(timeState);
      submitTimeAdvanceGrantDispatches(std::move(newlyEligible));
      return;
    case umbra::detail::FederateTimeDisableStatus::not_enabled:
      throw TimeConstrainedIsNotEnabled(
          L"Time constrained is not enabled for the joined federate.");
    case umbra::detail::FederateTimeDisableStatus::inactive:
      throw FederateNotExecutionMember(
          L"The joined federate's logical-time state is no longer active.");
  }
  throw RTIinternalError(L"Umbra encountered an unknown time-constrained disable outcome.");
  } catch (Exception const& exception) {
    emitExceptionReport(L"Disable Time Constrained", exception);
    throw;
  }
}

void UmbraRtiAmbassador::enableAsynchronousDelivery() {
  auto instrumentationScope = beginRtiCall("enableAsynchronousDelivery");
  try {
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  umbra::detail::FederateAsynchronousDeliveryStatus result;
  std::optional<JoinedFederateMomConditionalWork> momWork;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Enable Asynchronous Delivery");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Enable Asynchronous Delivery requires membership in a federation execution.");
    }
    timeState = federateTimeState_;
    result = timeState->enableAsynchronousDelivery();
    if (result == umbra::detail::FederateAsynchronousDeliveryStatus::applied) {
      appendSuccessfulVoidServiceReportToFileIfSelected(
          L"EnableAsynchronousDelivery",
          umbra::detail::MomServiceType::time_management,
          {});
      momWork = joinedFederateMomConditionalWorkFor(
          embeddedFederationManagement().registry(),
          *joinedFederationName_,
          *joinedFederateId_,
          {"HLAasynchronousDelivery"});
    }
  }

  switch (result) {
    case umbra::detail::FederateAsynchronousDeliveryStatus::applied:
      // Enabling the switch also makes receive-order callbacks that arrived
      // while the constrained federate was Time Granted immediately eligible.
      if (momWork) {
        queueJoinedFederateMomConditionalAttributeUpdate(
            momWork->federationName,
            momWork->objectInstanceHandle,
            std::move(momWork->attributeHandles));
      }
      flushAsynchronousReceiveCallbacks(timeState);
      return;
    case umbra::detail::FederateAsynchronousDeliveryStatus::already_enabled:
      throw AsynchronousDeliveryAlreadyEnabled(
          L"Asynchronous delivery is already enabled for the joined federate.");
    case umbra::detail::FederateAsynchronousDeliveryStatus::already_disabled:
      throw RTIinternalError(
          L"Umbra received an invalid asynchronous-delivery enable outcome.");
    case umbra::detail::FederateAsynchronousDeliveryStatus::inactive:
      throw FederateNotExecutionMember(
          L"The joined federate's logical-time state is no longer active.");
  }
  throw RTIinternalError(L"Umbra encountered an unknown asynchronous-delivery enable outcome.");
  } catch (Exception const& exception) {
    emitExceptionReport(L"Enable Asynchronous Delivery", exception);
    throw;
  }
}

void UmbraRtiAmbassador::disableAsynchronousDelivery() {
  auto instrumentationScope = beginRtiCall("disableAsynchronousDelivery");
  try {
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  umbra::detail::FederateAsynchronousDeliveryStatus result;
  std::optional<JoinedFederateMomConditionalWork> momWork;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Disable Asynchronous Delivery");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Disable Asynchronous Delivery requires membership in a federation execution.");
    }
    timeState = federateTimeState_;
    result = timeState->disableAsynchronousDelivery();
    if (result == umbra::detail::FederateAsynchronousDeliveryStatus::applied) {
      appendSuccessfulVoidServiceReportToFileIfSelected(
          L"DisableAsynchronousDelivery",
          umbra::detail::MomServiceType::time_management,
          {});
      momWork = joinedFederateMomConditionalWorkFor(
          embeddedFederationManagement().registry(),
          *joinedFederationName_,
          *joinedFederateId_,
          {"HLAasynchronousDelivery"});
    }
  }

  switch (result) {
    case umbra::detail::FederateAsynchronousDeliveryStatus::applied:
      if (momWork) {
        queueJoinedFederateMomConditionalAttributeUpdate(
            momWork->federationName,
            momWork->objectInstanceHandle,
            std::move(momWork->attributeHandles));
      }
      return;
    case umbra::detail::FederateAsynchronousDeliveryStatus::already_disabled:
      throw AsynchronousDeliveryAlreadyDisabled(
          L"Asynchronous delivery is already disabled for the joined federate.");
    case umbra::detail::FederateAsynchronousDeliveryStatus::already_enabled:
      throw RTIinternalError(
          L"Umbra received an invalid asynchronous-delivery disable outcome.");
    case umbra::detail::FederateAsynchronousDeliveryStatus::inactive:
      throw FederateNotExecutionMember(
          L"The joined federate's logical-time state is no longer active.");
  }
  throw RTIinternalError(L"Umbra encountered an unknown asynchronous-delivery disable outcome.");
  } catch (Exception const& exception) {
    emitExceptionReport(L"Disable Asynchronous Delivery", exception);
    throw;
  }
}

void UmbraRtiAmbassador::timeAdvanceRequest(LogicalTime const& time) {
  auto instrumentationScope = beginRtiCall("timeAdvanceRequest");
  try {
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  std::wstring federationName;
  std::uint64_t federateId = 0;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Time Advance Request");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Time Advance Request requires a joined federate with initialized logical time.");
    }

    timeState = federateTimeState_;
    federationName = *joinedFederationName_;
    federateId = *joinedFederateId_;
  }

  // LogicalTime is a caller-provided polymorphic object. Decode a reference
  // representation outside runtime locks, then commit only the private copy.
  auto requestedTime = cloneReferenceLogicalTime(timeState->implementationName(), time);
  // Section 8.8.1 names this supplied value Logical time. Table 5 gives
  // LogicalTime the quoted time.toString() form; obtain it from the private
  // reference copy outside federation locks.
  auto const reportTimeArgument = umbra::detail::MomServiceArgument{
      umbra::detail::MomArgumentType::logical_time,
      L"Logical time",
      umbra::detail::formatMomLogicalTime(*requestedTime),
  };
  umbra::detail::FederateTimeAdvanceResult result;
  umbra::detail::FederationTimeGrantDispatchResult scheduled;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Time Advance Request");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_ ||
        *joinedFederationName_ != federationName || *joinedFederateId_ != federateId ||
        federateTimeState_ != timeState) {
      throw FederateNotExecutionMember(
          L"Time Advance Request requires an active joined federate with initialized logical time.");
    }

    result = timeState->requestAdvance(std::move(requestedTime));
    if (result.status == umbra::detail::FederateTimeAdvanceStatus::applied) {
      retireTsoMessagePayloadsAtRetractionBoundary(
          federationName,
          federateId,
          *timeState);
      scheduled = embeddedFederationManagement().registry().requestTimeAdvanceGrant(
          federationName,
          federateId,
          result.generation);
      if (scheduled.status == umbra::detail::FederationTimeGrantStatus::applied) {
        // Acceptance puts the federate in Time Advancing state and is
        // reportable now. The distinct Time Advance Grant callback later
        // completes the logical-time transition.
        appendSuccessfulVoidServiceReportToFileIfSelected(
            L"TimeAdvanceRequest",
            umbra::detail::MomServiceType::time_management,
            {reportTimeArgument});
      }
    }
  }
  switch (result.status) {
    case umbra::detail::FederateTimeAdvanceStatus::applied:
      break;
    case umbra::detail::FederateTimeAdvanceStatus::logical_time_already_passed:
      throw LogicalTimeAlreadyPassed(
          L"Time Advance Request cannot move a joined federate backward in logical time.");
    case umbra::detail::FederateTimeAdvanceStatus::time_advance_pending:
      throw InTimeAdvancingState(
          L"The joined federate already has a Time Advance Request awaiting a grant.");
    case umbra::detail::FederateTimeAdvanceStatus::time_regulation_pending:
      throw RequestForTimeRegulationPending(
          L"The joined federate has an Enable Time Regulation request awaiting its callback.");
    case umbra::detail::FederateTimeAdvanceStatus::time_constrained_pending:
      throw RequestForTimeConstrainedPending(
          L"The joined federate has an Enable Time Constrained request awaiting its callback.");
    case umbra::detail::FederateTimeAdvanceStatus::invalid_logical_time:
      throw InvalidLogicalTime(
          L"The requested logical time is invalid for the joined federation's implementation.");
    case umbra::detail::FederateTimeAdvanceStatus::inactive:
      throw FederateNotExecutionMember(
          L"The joined federate's logical-time state is no longer active.");
    case umbra::detail::FederateTimeAdvanceStatus::generation_exhausted:
      throw RTIinternalError(
          L"Umbra exhausted the embedded time-advance request generation space.");
  }

  if (scheduled.status != umbra::detail::FederationTimeGrantStatus::applied) {
    throw RTIinternalError(
        L"Umbra could not register the accepted Time Advance Request with its federation scheduler.");
  }

  // The request has now entered Time Advancing.  Reflect the current MOM
  // HLAtimeManagerState from the federation-owned temporal state; the grant
  // callback will publish the return to Time Granted separately.
  queueJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
      federationName,
      federateId,
      {"HLAtimeManagerState"});

  // The registry admits nonconstrained requests immediately and holds a
  // constrained TAR until strict GALT/NRG policy allows delivery. Every action
  // is submitted only after the calling thread has released its runtime locks.
  flushAsynchronousReceiveCallbacks(timeState);
  submitTimeAdvanceGrantDispatches(std::move(scheduled.dispatches));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Time Advance Request", exception);
    throw;
  }
}

void UmbraRtiAmbassador::requestAvailableTimeAdvance(
    LogicalTime const& time,
    umbra::detail::FederateTimeAdvanceMode mode,
    bool selectNextQueuedMessage,
    std::wstring const& serviceName) {
  auto instrumentationScope = beginRtiCall("requestAvailableTimeAdvance");
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  std::wstring federationName;
  std::uint64_t federateId = 0;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(serviceName);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          serviceName + L" requires an active joined federate with initialized logical time.");
    }

    timeState = federateTimeState_;
    federationName = *joinedFederationName_;
    federateId = *joinedFederateId_;
  }

  auto requestedTime = cloneReferenceLogicalTime(timeState->implementationName(), time);
  // Sections 8.9.1 and 8.11.1 each name their supplied value Logical time.
  // Table 5 gives LogicalTime the quoted time.toString() form. The two
  // services share scheduling machinery but retain distinct report-service
  // identities.
  std::optional<umbra::detail::MomServiceArgument> reportTimeArgument;
  std::optional<std::wstring> reportService;
  if (mode == umbra::detail::FederateTimeAdvanceMode::time_advance_request_available) {
    reportService = L"TimeAdvanceRequestAvailable";
  } else if (mode == umbra::detail::FederateTimeAdvanceMode::next_message_request_available) {
    reportService = L"NextMessageRequestAvailable";
  }
  if (reportService) {
    reportTimeArgument = umbra::detail::MomServiceArgument{
        umbra::detail::MomArgumentType::logical_time,
        L"Logical time",
        umbra::detail::formatMomLogicalTime(*requestedTime),
    };
  }
  std::shared_ptr<LogicalTime> effectiveTime;
  std::shared_ptr<LogicalTime const> earliestQueuedTimestamp;
  if (selectNextQueuedMessage) {
    {
      std::scoped_lock lock(mutex_, federationManagementMutex());
      requireConnected(lifecycle_);
      requireFederationServiceOperationAvailable(serviceName);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_ ||
          *joinedFederationName_ != federationName || *joinedFederateId_ != federateId ||
          federateTimeState_ != timeState) {
        throw FederateNotExecutionMember(
            serviceName + L" requires an active joined federate with initialized logical time.");
      }

      auto const queued = embeddedFederationManagement().registry().earliestTsoTimestampFor(
          federationName,
          federateId);
      if (queued && *queued) {
        earliestQueuedTimestamp = *queued;
      }
    }

    if (earliestQueuedTimestamp) {
      auto candidate = cloneReferenceLogicalTime(
          timeState->implementationName(),
          *earliestQueuedTimestamp);
      auto const snapshot = timeState->snapshot();
      try {
        if (snapshot.currentTime && *candidate >= *snapshot.currentTime &&
            *candidate <= *requestedTime) {
          effectiveTime = std::move(candidate);
        }
      } catch (Exception const&) {
        throw InvalidLogicalTime(
            L"The next queued TSO timestamp cannot be compared with the requested logical time.");
      }
    }
  }

  umbra::detail::FederateTimeAdvanceResult result;
  umbra::detail::FederationTimeGrantDispatchResult scheduled;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(serviceName);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_ ||
        *joinedFederationName_ != federationName || *joinedFederateId_ != federateId ||
        federateTimeState_ != timeState) {
      throw FederateNotExecutionMember(
          serviceName + L" requires an active joined federate with initialized logical time.");
    }

    if (mode == umbra::detail::FederateTimeAdvanceMode::time_advance_request_available) {
      result = timeState->requestAdvanceAvailable(std::move(requestedTime));
    } else if (mode == umbra::detail::FederateTimeAdvanceMode::next_message_request_available) {
      result = timeState->requestNextMessageAvailableAdvance(
          std::move(requestedTime),
          std::move(effectiveTime));
    } else {
      throw RTIinternalError(L"Umbra received an unsupported available time-advance mode.");
    }

    if (result.status == umbra::detail::FederateTimeAdvanceStatus::applied) {
      retireTsoMessagePayloadsAtRetractionBoundary(
          federationName,
          federateId,
          *timeState);
      scheduled = embeddedFederationManagement().registry().requestTimeAdvanceGrant(
          federationName,
          federateId,
          result.generation);
      if (reportService && reportTimeArgument &&
          scheduled.status == umbra::detail::FederationTimeGrantStatus::applied) {
        // Acceptance is reportable now; the distinct Time Advance Grant
        // callback later completes the logical-time transition.
        appendSuccessfulVoidServiceReportToFileIfSelected(
            *reportService,
            umbra::detail::MomServiceType::time_management,
            {*reportTimeArgument});
      }
    }
  }

  switch (result.status) {
    case umbra::detail::FederateTimeAdvanceStatus::applied:
      break;
    case umbra::detail::FederateTimeAdvanceStatus::logical_time_already_passed:
      throw LogicalTimeAlreadyPassed(
          serviceName + L" cannot move a joined federate backward in logical time.");
    case umbra::detail::FederateTimeAdvanceStatus::time_advance_pending:
      throw InTimeAdvancingState(
          serviceName + L" cannot run while a time-advance request is awaiting a grant.");
    case umbra::detail::FederateTimeAdvanceStatus::time_regulation_pending:
      throw RequestForTimeRegulationPending(
          serviceName + L" cannot run while Enable Time Regulation is awaiting its callback.");
    case umbra::detail::FederateTimeAdvanceStatus::time_constrained_pending:
      throw RequestForTimeConstrainedPending(
          serviceName + L" cannot run while Enable Time Constrained is awaiting its callback.");
    case umbra::detail::FederateTimeAdvanceStatus::invalid_logical_time:
      throw InvalidLogicalTime(
          serviceName + L" received a logical time invalid for the joined federation.");
    case umbra::detail::FederateTimeAdvanceStatus::inactive:
      throw FederateNotExecutionMember(
          serviceName + L" found that the joined federate's logical-time state is inactive.");
    case umbra::detail::FederateTimeAdvanceStatus::generation_exhausted:
      throw RTIinternalError(
          L"Umbra exhausted the embedded time-advance request generation space.");
  }

  if (scheduled.status != umbra::detail::FederationTimeGrantStatus::applied) {
    throw RTIinternalError(
        serviceName + L" could not register the accepted request with its federation scheduler.");
  }

  queueJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
      federationName,
      federateId,
      {"HLAtimeManagerState"});

  // The shared grant dispatch delivers any selected queued TSO cohort before
  // the ordinary Time Advance Grant callback. With no selected message, the
  // effective target remains the caller's supplied request.
  flushAsynchronousReceiveCallbacks(timeState);
  submitTimeAdvanceGrantDispatches(std::move(scheduled.dispatches));
}

void UmbraRtiAmbassador::timeAdvanceRequestAvailable(LogicalTime const& time) {
  auto instrumentationScope = beginRtiCall("timeAdvanceRequestAvailable");
  try {
  requestAvailableTimeAdvance(
      time,
      umbra::detail::FederateTimeAdvanceMode::time_advance_request_available,
      false,
      L"Time Advance Request Available");
  } catch (Exception const& exception) {
    emitExceptionReport(L"Time Advance Request Available", exception);
    throw;
  }
}

void UmbraRtiAmbassador::nextMessageRequest(LogicalTime const& time) {
  auto instrumentationScope = beginRtiCall("nextMessageRequest");
  try {
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  std::wstring federationName;
  std::uint64_t federateId = 0;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Next Message Request");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Next Message Request requires an active joined federate with initialized logical time.");
    }

    timeState = federateTimeState_;
    federationName = *joinedFederationName_;
    federateId = *joinedFederateId_;
  }

  // Keep the caller's requested boundary independently of the effective grant
  // target. NMR may grant at the earliest currently queued TSO timestamp when
  // that timestamp is no greater than the supplied request.
  auto requestedTime = cloneReferenceLogicalTime(timeState->implementationName(), time);
  // Section 8.10.1 names the supplied value Logical time. Table 5 uses the
  // quoted time.toString() form. Preserve this caller-supplied boundary for
  // reporting even when a queued TSO message later determines an earlier
  // effective Time Advance Grant target.
  auto const reportTimeArgument = umbra::detail::MomServiceArgument{
      umbra::detail::MomArgumentType::logical_time,
      L"Logical time",
      umbra::detail::formatMomLogicalTime(*requestedTime),
  };
  std::shared_ptr<LogicalTime> effectiveTime;
  std::shared_ptr<LogicalTime const> earliestQueuedTimestamp;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Next Message Request");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_ ||
        *joinedFederationName_ != federationName || *joinedFederateId_ != federateId ||
        federateTimeState_ != timeState) {
      throw FederateNotExecutionMember(
          L"Next Message Request requires an active joined federate with initialized logical time.");
    }

    auto const queued = embeddedFederationManagement().registry().earliestTsoTimestampFor(
        federationName,
        federateId);
    if (queued && *queued) {
      earliestQueuedTimestamp = *queued;
    }
  }

  if (earliestQueuedTimestamp) {
    auto candidate = cloneReferenceLogicalTime(
        timeState->implementationName(),
        *earliestQueuedTimestamp);
    auto const snapshot = timeState->snapshot();
    try {
      if (snapshot.currentTime && *candidate >= *snapshot.currentTime &&
          *candidate <= *requestedTime) {
        effectiveTime = std::move(candidate);
      }
    } catch (Exception const&) {
      throw InvalidLogicalTime(
          L"The next queued TSO timestamp cannot be compared with the requested logical time.");
    }
  }

  umbra::detail::FederateTimeAdvanceResult result;
  umbra::detail::FederationTimeGrantDispatchResult scheduled;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Next Message Request");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_ ||
        *joinedFederationName_ != federationName || *joinedFederateId_ != federateId ||
        federateTimeState_ != timeState) {
      throw FederateNotExecutionMember(
          L"Next Message Request requires an active joined federate with initialized logical time.");
    }

    result = timeState->requestNextMessageAdvance(
        std::move(requestedTime),
        std::move(effectiveTime));
    if (result.status == umbra::detail::FederateTimeAdvanceStatus::applied) {
      retireTsoMessagePayloadsAtRetractionBoundary(
          federationName,
          federateId,
          *timeState);
      scheduled = embeddedFederationManagement().registry().requestTimeAdvanceGrant(
          federationName,
          federateId,
          result.generation);
      if (scheduled.status == umbra::detail::FederationTimeGrantStatus::applied) {
        // The accepted NMR records its supplied request now. The shared grant
        // dispatch later completes the request at either the selected queued
        // timestamp or the supplied boundary.
        appendSuccessfulVoidServiceReportToFileIfSelected(
            L"NextMessageRequest",
            umbra::detail::MomServiceType::time_management,
            {reportTimeArgument});
      }
    }
  }

  switch (result.status) {
    case umbra::detail::FederateTimeAdvanceStatus::applied:
      break;
    case umbra::detail::FederateTimeAdvanceStatus::logical_time_already_passed:
      throw LogicalTimeAlreadyPassed(
          L"Next Message Request cannot move a joined federate backward in logical time.");
    case umbra::detail::FederateTimeAdvanceStatus::time_advance_pending:
      throw InTimeAdvancingState(
          L"The joined federate already has a time-advance request awaiting a grant.");
    case umbra::detail::FederateTimeAdvanceStatus::time_regulation_pending:
      throw RequestForTimeRegulationPending(
          L"The joined federate has an Enable Time Regulation request awaiting its callback.");
    case umbra::detail::FederateTimeAdvanceStatus::time_constrained_pending:
      throw RequestForTimeConstrainedPending(
          L"The joined federate has an Enable Time Constrained request awaiting its callback.");
    case umbra::detail::FederateTimeAdvanceStatus::invalid_logical_time:
      throw InvalidLogicalTime(
          L"The requested logical time is invalid for the joined federation's implementation.");
    case umbra::detail::FederateTimeAdvanceStatus::inactive:
      throw FederateNotExecutionMember(
          L"The joined federate's logical-time state is no longer active.");
    case umbra::detail::FederateTimeAdvanceStatus::generation_exhausted:
      throw RTIinternalError(
          L"Umbra exhausted the embedded time-advance request generation space.");
  }

  if (scheduled.status != umbra::detail::FederationTimeGrantStatus::applied) {
    throw RTIinternalError(
        L"Umbra could not register the accepted Next Message Request with its federation scheduler.");
  }

  queueJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
      federationName,
      federateId,
      {"HLAtimeManagerState"});

  // The shared grant dispatch delivers the selected TSO timestamp cohort
  // before the ordinary Time Advance Grant callback. If no queued timestamp
  // is eligible, the state target remains the caller's requested time.
  flushAsynchronousReceiveCallbacks(timeState);
  submitTimeAdvanceGrantDispatches(std::move(scheduled.dispatches));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Next Message Request", exception);
    throw;
  }
}

void UmbraRtiAmbassador::nextMessageRequestAvailable(LogicalTime const& time) {
  auto instrumentationScope = beginRtiCall("nextMessageRequestAvailable");
  try {
  requestAvailableTimeAdvance(
      time,
      umbra::detail::FederateTimeAdvanceMode::next_message_request_available,
      true,
      L"Next Message Request Available");
  } catch (Exception const& exception) {
    emitExceptionReport(L"Next Message Request Available", exception);
    throw;
  }
}

void UmbraRtiAmbassador::flushQueueRequest(LogicalTime const& time) {
  auto instrumentationScope = beginRtiCall("flushQueueRequest");
  try {
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  std::wstring federationName;
  std::uint64_t federateId = 0;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Flush Queue Request");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Flush Queue Request requires an active joined federate with initialized logical time.");
    }

    timeState = federateTimeState_;
    federationName = *joinedFederationName_;
    federateId = *joinedFederateId_;
  }

  auto requestedTime = cloneReferenceLogicalTime(timeState->implementationName(), time);
  auto const reportTimeArgument = umbra::detail::MomServiceArgument{
      umbra::detail::MomArgumentType::logical_time,
      L"Logical time",
      umbra::detail::formatMomLogicalTime(*requestedTime),
  };
  umbra::detail::FederateTimeAdvanceResult result;
  umbra::detail::FederationTimeGrantDispatchResult scheduled;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Flush Queue Request");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_ ||
        *joinedFederationName_ != federationName || *joinedFederateId_ != federateId ||
        federateTimeState_ != timeState) {
      throw FederateNotExecutionMember(
          L"Flush Queue Request requires an active joined federate with initialized logical time.");
    }

    result = timeState->requestFlushQueueAdvance(std::move(requestedTime));
    if (result.status == umbra::detail::FederateTimeAdvanceStatus::applied) {
      retireTsoMessagePayloadsAtRetractionBoundary(
          federationName,
          federateId,
          *timeState);
      scheduled = embeddedFederationManagement().registry().requestTimeAdvanceGrant(
          federationName,
          federateId,
          result.generation);
    }
  }

  switch (result.status) {
    case umbra::detail::FederateTimeAdvanceStatus::applied:
      break;
    case umbra::detail::FederateTimeAdvanceStatus::logical_time_already_passed:
      throw LogicalTimeAlreadyPassed(
          L"Flush Queue Request cannot move a joined federate backward or below its optimistic logical time.");
    case umbra::detail::FederateTimeAdvanceStatus::time_advance_pending:
      throw InTimeAdvancingState(
          L"The joined federate already has a time-advance request awaiting a grant.");
    case umbra::detail::FederateTimeAdvanceStatus::time_regulation_pending:
      throw RequestForTimeRegulationPending(
          L"The joined federate has an Enable Time Regulation request awaiting its callback.");
    case umbra::detail::FederateTimeAdvanceStatus::time_constrained_pending:
      throw RequestForTimeConstrainedPending(
          L"The joined federate has an Enable Time Constrained request awaiting its callback.");
    case umbra::detail::FederateTimeAdvanceStatus::invalid_logical_time:
      throw InvalidLogicalTime(
          L"The requested logical time is invalid for the joined federation's implementation.");
    case umbra::detail::FederateTimeAdvanceStatus::inactive:
      throw FederateNotExecutionMember(
          L"The joined federate's logical-time state is no longer active.");
    case umbra::detail::FederateTimeAdvanceStatus::generation_exhausted:
      throw RTIinternalError(
          L"Umbra exhausted the embedded time-advance request generation space.");
  }

  if (scheduled.status != umbra::detail::FederationTimeGrantStatus::applied) {
    throw RTIinternalError(
        L"Umbra could not register the accepted Flush Queue Request with its federation scheduler.");
  }

  queueJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
      federationName,
      federateId,
      {"HLAtimeManagerState"});

  // The successful invocation records its supplied boundary. Flush Queue Grant
  // later reports the actual and optimistic logical times selected at callback
  // delivery, which are separate values under §8.12.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"FlushQueueRequest",
      umbra::detail::MomServiceType::time_management,
      {reportTimeArgument});

  // Flush Queue Grant dispatch is callback-gated just like Time Advance Grant,
  // but computes the actual and optimistic times at the delivery boundary.
  flushAsynchronousReceiveCallbacks(timeState);
  submitTimeAdvanceGrantDispatches(std::move(scheduled.dispatches));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Flush Queue Request", exception);
    throw;
  }
}

void UmbraRtiAmbassador::queryLogicalTime(LogicalTime& time) {
  auto instrumentationScope = beginRtiCall("queryLogicalTime");
  try {
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Query Logical Time");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Query Logical Time requires a joined federate with initialized logical time.");
    }
    timeState = federateTimeState_;
  }

  auto currentTime = timeState->currentTime();
  if (!currentTime) {
    throw FederateNotExecutionMember(
        L"The joined federate's logical-time state is no longer active.");
  }
  // Do not invoke the caller-provided LogicalTime assignment while holding an
  // Umbra state lock; a custom implementation may execute arbitrary code.
  copyQueriedLogicalTime(time, *currentTime);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Query Logical Time", exception);
    throw;
  }
}

bool UmbraRtiAmbassador::queryGALT(LogicalTime& time) {
  auto instrumentationScope = beginRtiCall("queryGALT");
  try {
  std::uint64_t federateId = 0;
  umbra::detail::FederationTimeExecutionSnapshot snapshot;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Query GALT");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Query GALT requires a joined federate with initialized logical time.");
    }
    auto currentSnapshot = embeddedFederationManagement().registry().timeSnapshotFor(
        *joinedFederationName_);
    if (!currentSnapshot) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador's time state.");
    }
    federateId = *joinedFederateId_;
    snapshot = std::move(*currentSnapshot);
  }

  auto const bounds = umbra::detail::FederationTimeBoundsCalculator{}.calculate(snapshot, federateId);
  switch (bounds.status) {
    case umbra::detail::FederationTimeBoundStatus::available:
      if (!bounds.galt) {
        throw RTIinternalError(L"Umbra computed an available GALT without a logical-time value.");
      }
      copyQueriedLogicalTime(time, *bounds.galt);
      return true;
    case umbra::detail::FederationTimeBoundStatus::undefined:
      return false;
    case umbra::detail::FederationTimeBoundStatus::requesting_federate_not_registered:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador's time state.");
    case umbra::detail::FederationTimeBoundStatus::factory_unavailable:
    case umbra::detail::FederationTimeBoundStatus::inconsistent_temporal_state:
      throw RTIinternalError(L"Umbra could not calculate a coherent federation GALT.");
  }
  throw RTIinternalError(L"Umbra encountered an unknown Query GALT outcome.");
  } catch (Exception const& exception) {
    emitExceptionReport(L"Query GALT", exception);
    throw;
  }
}

bool UmbraRtiAmbassador::queryLITS(LogicalTime& time) {
  auto instrumentationScope = beginRtiCall("queryLITS");
  try {
  std::uint64_t federateId = 0;
  umbra::detail::FederationTimeExecutionSnapshot snapshot;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Query LITS");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Query LITS requires a joined federate with initialized logical time.");
    }
    auto currentSnapshot = embeddedFederationManagement().registry().timeSnapshotFor(
        *joinedFederationName_);
    if (!currentSnapshot) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador's time state.");
    }
    federateId = *joinedFederateId_;
    snapshot = std::move(*currentSnapshot);
  }

  auto const bounds = umbra::detail::FederationTimeBoundsCalculator{}.calculate(snapshot, federateId);
  switch (bounds.status) {
    case umbra::detail::FederationTimeBoundStatus::available:
      if (!bounds.lits) {
        throw RTIinternalError(L"Umbra computed an available LITS without a logical-time value.");
      }
      copyQueriedLogicalTime(time, *bounds.lits);
      return true;
    case umbra::detail::FederationTimeBoundStatus::undefined:
      if (bounds.lits) {
        copyQueriedLogicalTime(time, *bounds.lits);
        return true;
      }
      return false;
    case umbra::detail::FederationTimeBoundStatus::requesting_federate_not_registered:
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador's time state.");
    case umbra::detail::FederationTimeBoundStatus::factory_unavailable:
    case umbra::detail::FederationTimeBoundStatus::inconsistent_temporal_state:
      throw RTIinternalError(L"Umbra could not calculate a coherent federation LITS.");
  }
  throw RTIinternalError(L"Umbra encountered an unknown Query LITS outcome.");
  } catch (Exception const& exception) {
    emitExceptionReport(L"Query LITS", exception);
    throw;
  }
}

void UmbraRtiAmbassador::queryLookahead(LogicalTimeInterval& interval) {
  auto instrumentationScope = beginRtiCall("queryLookahead");
  try {
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Query Lookahead");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Query Lookahead requires a joined federate with initialized logical time.");
    }
    timeState = federateTimeState_;
  }

  auto result = timeState->currentLookahead();
  switch (result.status) {
    case umbra::detail::FederateTimeLookaheadStatus::applied:
      if (!result.lookahead) {
        throw RTIinternalError(
            L"Umbra received a successful lookahead query without an interval value.");
      }
      copyQueriedLogicalTimeInterval(interval, *result.lookahead);
      return;
    case umbra::detail::FederateTimeLookaheadStatus::not_enabled:
      throw TimeRegulationIsNotEnabled(
          L"Time regulation is not enabled for the joined federate.");
    case umbra::detail::FederateTimeLookaheadStatus::inactive:
      throw FederateNotExecutionMember(
          L"The joined federate's logical-time state is no longer active.");
  }
  throw RTIinternalError(L"Umbra encountered an unknown Query Lookahead outcome.");
  } catch (Exception const& exception) {
    emitExceptionReport(L"Query Lookahead", exception);
    throw;
  }
}

void UmbraRtiAmbassador::modifyLookahead(LogicalTimeInterval const& lookahead) {
  auto instrumentationScope = beginRtiCall("modifyLookahead");
  try {
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  std::wstring federationName;
  std::uint64_t federateId = 0;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Modify Lookahead");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Modify Lookahead requires a joined federate with initialized logical time.");
    }
    timeState = federateTimeState_;
    federationName = *joinedFederationName_;
    federateId = *joinedFederateId_;
  }

 auto requestedLookahead = cloneReferenceLogicalTimeInterval(
     timeState->implementationName(), lookahead);
  // Section 8.20.1 identifies this supplied value as Requested lookahead.
  // Table 5 gives LogicalTimeInterval the quoted interval.toString() form;
  // obtain it from the private reference copy outside federation locks.
  auto const reportLookaheadArgument = umbra::detail::MomServiceArgument{
      umbra::detail::MomArgumentType::logical_time_interval,
      L"Requested lookahead",
      umbra::detail::formatMomLogicalTimeInterval(*requestedLookahead),
  };
 umbra::detail::FederateTimeModifyLookaheadStatus result;
  std::vector<umbra::detail::FederationTimeGrantDispatch> newlyEligible;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Modify Lookahead");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_ ||
        *joinedFederationName_ != federationName || *joinedFederateId_ != federateId ||
        federateTimeState_ != timeState) {
      throw FederateNotExecutionMember(
          L"Modify Lookahead requires an active joined federate with initialized logical time.");
   }
   result = timeState->modifyLookahead(std::move(requestedLookahead));
   if (result == umbra::detail::FederateTimeModifyLookaheadStatus::applied) {
      // A lower lookahead remains a pending state transition, but this
      // successful service invocation is reportable immediately.
      appendSuccessfulVoidServiceReportToFileIfSelected(
          L"ModifyLookahead",
          umbra::detail::MomServiceType::time_management,
          {reportLookaheadArgument});
     retireTsoMessagePayloadsAtRetractionBoundary(
         federationName,
          federateId,
          *timeState);
      auto scheduled = embeddedFederationManagement().registry().reevaluateTimeAdvanceGrants(
          federationName);
      if (scheduled.status == umbra::detail::FederationTimeGrantStatus::applied) {
        newlyEligible = std::move(scheduled.dispatches);
      }
    }
  }

  switch (result) {
    case umbra::detail::FederateTimeModifyLookaheadStatus::applied:
      submitTimeAdvanceGrantDispatches(std::move(newlyEligible));
      return;
    case umbra::detail::FederateTimeModifyLookaheadStatus::time_advance_pending:
      throw InTimeAdvancingState(
          L"Modify Lookahead cannot run while the joined federate has a time advance pending.");
    case umbra::detail::FederateTimeModifyLookaheadStatus::not_enabled:
      throw TimeRegulationIsNotEnabled(
          L"Modify Lookahead requires time regulation to be enabled for the joined federate.");
    case umbra::detail::FederateTimeModifyLookaheadStatus::inactive:
      throw FederateNotExecutionMember(
          L"The joined federate's logical-time state is no longer active.");
  }
  throw RTIinternalError(L"Umbra encountered an unknown Modify Lookahead outcome.");
  } catch (Exception const& exception) {
    emitExceptionReport(L"Modify Lookahead", exception);
    throw;
  }
}

void UmbraRtiAmbassador::retract(MessageRetractionHandle const& retraction) {
  auto instrumentationScope = beginRtiCall("retract");
  try {
  auto const messageId = messageRetractionHandleValue(retraction);
  if (!messageId) {
    throw InvalidMessageRetractionHandle(
        L"Retract requires a valid MessageRetractionHandle returned by a timestamped service.");
  }
  auto const reportRetractionArgument = umbra::detail::MomServiceArgument{
      umbra::detail::MomArgumentType::message_retraction_handle,
      L"MessageRetractionDesignator",
      umbra::detail::formatMomMessageRetractionHandle(*messageId),
  };

  std::wstring federationName;
  std::uint64_t producingFederateId = 0;
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  std::shared_ptr<LogicalTime const> retractionLowerBound;
  umbra::detail::FederationTsoRetractionResult result;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    requireFederationServiceOperationAvailable(L"Retract");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Retract requires membership in a federation execution.");
    }
    federationName = *joinedFederationName_;
    producingFederateId = *joinedFederateId_;
    timeState = federateTimeState_;
    auto const timeSnapshot = timeState->snapshot();
    if (!timeSnapshot.timeRegulating) {
      throw TimeRegulationIsNotEnabled(
          L"Retract requires time regulation to be enabled for the joined federate.");
    }
    retractionLowerBound = makeTsoRetractionLowerBound(timeSnapshot);
    result = embeddedFederationManagement().registry().retractTsoMessageForProducer(
        federationName,
        producingFederateId,
        *messageId,
        retractionLowerBound);
  }
  if (result.status == umbra::detail::FederationTsoRegistryStatus::federation_does_not_exist ||
      result.status == umbra::detail::FederationTsoRegistryStatus::federate_not_member) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  if (result.status != umbra::detail::FederationTsoRegistryStatus::applied) {
    throw InvalidMessageRetractionHandle(
        L"The MessageRetractionHandle is not owned by this joined federate.");
  }
  if (!result.timestampEligible) {
    throw MessageCanNoLongerBeRetracted(
        L"The timestamped message is not later than the producer's current or requested time plus lookahead.");
  }
  switch (result.queueResult.status) {
    case umbra::detail::TsoMessageQueueStatus::applied:
      // The accepted Retract invocation records its original designator. Any
      // Request Retraction callbacks are a separate consequence and remain
      // callback-model gated below.
      appendSuccessfulVoidServiceReportToFileIfSelected(
          L"Retract",
          umbra::detail::MomServiceType::time_management,
          {reportRetractionArgument});
      for (auto const& notification : result.requestRetractionNotifications) {
        queueRequestRetraction(
            notification.callbackRoute,
            federationName,
            notification.receivingFederateId,
            notification.messageId);
      }
      return;
    case umbra::detail::TsoMessageQueueStatus::message_already_retracted:
    case umbra::detail::TsoMessageQueueStatus::message_already_delivered:
    case umbra::detail::TsoMessageQueueStatus::message_not_found:
      throw MessageCanNoLongerBeRetracted(
          L"The timestamped message is no longer retractable.");
    case umbra::detail::TsoMessageQueueStatus::invalid_message_id:
    case umbra::detail::TsoMessageQueueStatus::invalid_recipient:
    case umbra::detail::TsoMessageQueueStatus::invalid_timestamp:
    case umbra::detail::TsoMessageQueueStatus::logical_time_implementation_mismatch:
      throw InvalidMessageRetractionHandle(
          L"The MessageRetractionHandle does not identify a valid pending message.");
  }
  throw RTIinternalError(L"Umbra encountered an unknown Retract outcome.");
  } catch (Exception const& exception) {
    emitExceptionReport(L"Retract", exception);
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
