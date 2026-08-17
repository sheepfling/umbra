#include "internal/umbra_rti_ambassador.hpp"

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/attribute_handle.hpp"
#include "internal/dimension_handle.hpp"
#include "internal/federate_handle.hpp"
#include "internal/federate_time_state.hpp"
#include "internal/federation_management_coordinator.hpp"
#include "internal/federation_registry.hpp"
#include "internal/federation_time_bounds.hpp"
#include "internal/interaction_class_handle.hpp"
#include "internal/libxml2_fom_composer.hpp"
#include "internal/libxml2_fom_validator.hpp"
#include "internal/message_retraction_handle.hpp"
#include "internal/object_class_handle.hpp"
#include "internal/object_instance_handle.hpp"
#include "internal/parameter_handle.hpp"
#include "internal/region_handle.hpp"
#include "internal/reference_time_selection.hpp"
#include "internal/transportation_type_handle.hpp"
#include "internal/utf8_string.hpp"

#include <RTI/FederateAmbassador.h>
#include <RTI/time/HLAlogicalTimeFactoryFactory.h>
#endif

#include <chrono>
#include <cmath>
#include <cstddef>
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include <filesystem>
#endif
#include <limits>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {
namespace {

void validateCallbackModel(CallbackModel callbackModel) {
  switch (callbackModel) {
    case HLA_IMMEDIATE:
    case HLA_EVOKED:
      return;
  }
  throw UnsupportedCallbackModel(L"Umbra supports only HLA_IMMEDIATE and HLA_EVOKED callback models.");
}

ConfigurationResult ignoredConfigurationResult() {
  // The first runtime is an embedded connection backend. It does not yet
  // consume endpoint or additional-settings fields, so report that accurately
  // through the standard result object rather than claiming success for them.
  return ConfigurationResult(false, false, SETTINGS_IGNORED);
}

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
      timeSnapshot.timeAdvancePending && timeSnapshot.requestedTime
          ? *timeSnapshot.requestedTime
          : *timeSnapshot.currentTime);
  try {
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
  return [dispatcher, session](umbra::detail::FederateCallbackInvocation invocation) mutable {
    auto callbackDispatcher = dispatcher.lock();
    if (!callbackDispatcher) {
      return;
    }

    // Callers invoke this route only after releasing federation state locks.
    // An immediate dispatcher can enter user code synchronously here.
    callbackDispatcher->submit([session, invocation = std::move(invocation)]() mutable {
      auto callbackSession = session.lock();
      if (!callbackSession) {
        return;
      }
      callbackSession->invoke(std::move(invocation));
    });
  };
}

void queueReceiveOrderInteraction(
    umbra::detail::InteractionCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t producingFederateId,
    std::uint64_t receivingFederateId,
    std::uint64_t sentInteractionClassHandle,
    std::vector<InteractionParameterValue> sentParameters,
    VariableLengthData userSuppliedTag,
    TransportationTypeHandle transportationType,
    std::optional<std::set<std::uint64_t>> sentRegionHandles = std::nullopt) {
  std::vector<std::uint64_t> sentParameterHandles;
  sentParameterHandles.reserve(sentParameters.size());
  for (auto const& [parameterHandle, parameterValue] : sentParameters) {
    static_cast<void>(parameterValue);
    sentParameterHandles.push_back(parameterHandle);
  }

  callbackRoute([
      federationName = std::move(federationName),
      producingFederateId,
      receivingFederateId,
      sentInteractionClassHandle,
      sentParameterHandles = std::move(sentParameterHandles),
      sentParameters = std::move(sentParameters),
      userSuppliedTag = std::move(userSuppliedTag),
      transportationType = std::move(transportationType),
      sentRegionHandles = std::move(sentRegionHandles)](FederateAmbassador& recipient) mutable {
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
    if (sentRegionHandles) {
      optionalSentRegions.emplace();
      for (std::uint64_t const regionHandle : *sentRegionHandles) {
        optionalSentRegions->insert(makeRegionHandle(regionHandle));
      }
    }
    recipient.receiveInteraction(
        makeInteractionClassHandle(projection->receivedInteractionClassHandle),
        parameterValues,
        userSuppliedTag,
        transportationType,
        makeFederateHandle(producingFederateId),
        optionalSentRegions ? &*optionalSentRegions : nullptr);
  });
}

void queueTimestampedReceiveOrderInteraction(
    umbra::detail::InteractionCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t producingFederateId,
    std::uint64_t receivingFederateId,
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

  callbackRoute([
      federationName = std::move(federationName),
      producingFederateId,
      receivingFederateId,
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
    std::optional<umbra::detail::ReceiveOrderInteractionRecipient> projection;
    {
      std::scoped_lock lock(federationManagementMutex());
      projection = embeddedFederationManagement().registry().receiveOrderInteractionRecipientFor(
          federationName,
          producingFederateId,
          receivingFederateId,
          sentInteractionClassHandle,
          sentParameterHandles);
    }
    if (!projection) {
      return;
    }

    ParameterHandleValueMap parameterValues = projectInteractionParameterValues(
        sentParameters,
        projection->receivedParameterHandles);
    std::optional<MessageRetractionHandle> retraction;
    if (retractionMessageId) {
      retraction.emplace(makeMessageRetractionHandle(*retractionMessageId));
    }
    recipient.receiveInteraction(
        makeInteractionClassHandle(projection->receivedInteractionClassHandle),
        parameterValues,
        userSuppliedTag,
        transportationType,
        makeFederateHandle(producingFederateId),
        nullptr,
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
  callbackRoute([
      federationName = std::move(federationName),
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

    for (auto const& passel : passels) {
      std::optional<umbra::detail::ReceiveOrderAttributeUpdateRecipient> projection;
      {
        std::scoped_lock lock(federationManagementMutex());
        projection = embeddedFederationManagement().registry()
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
      std::optional<RegionHandleSet> optionalSentRegions;
      if (!passel.sentRegionHandles.empty()) {
        optionalSentRegions.emplace();
        for (std::uint64_t const regionHandle : passel.sentRegionHandles) {
          optionalSentRegions->insert(makeRegionHandle(regionHandle));
        }
      }

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

  callbackRoute([
      federationName = std::move(federationName),
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
    recipient.receiveDirectedInteraction(
        makeInteractionClassHandle(projection->receivedInteractionClassHandle),
        makeObjectInstanceHandle(projection->objectInstanceHandle),
        parameterValues,
        userSuppliedTag,
        transportationType,
        makeFederateHandle(producingFederateId));
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
    std::optional<std::set<std::uint64_t>> sentRegionHandles = std::nullopt) {
  callbackRoute([
      federationName = std::move(federationName),
      producingFederateId,
      receivingFederateId,
      objectInstanceHandle,
      sentAttributeHandles = std::move(sentAttributeHandles),
      sentAttributes = std::move(sentAttributes),
      userSuppliedTag = std::move(userSuppliedTag),
      transportationType = std::move(transportationType),
      sentRegionHandles = std::move(sentRegionHandles)](FederateAmbassador& recipient) mutable {
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
    std::optional<RegionHandleSet> optionalSentRegions;
    if (sentRegionHandles) {
      optionalSentRegions.emplace();
      for (std::uint64_t const regionHandle : *sentRegionHandles) {
        optionalSentRegions->insert(makeRegionHandle(regionHandle));
      }
    }
    recipient.reflectAttributeValues(
        makeObjectInstanceHandle(objectInstanceHandle),
        attributeValues,
        userSuppliedTag,
        transportationType,
        makeFederateHandle(producingFederateId),
        optionalSentRegions ? &*optionalSentRegions : nullptr);
  });
}

void queueAttributeValueUpdateProvide(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
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
      userSuppliedTag = std::move(userSuppliedTag)](FederateAmbassador& provider) mutable {
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
    provider.provideAttributeValueUpdate(
        makeObjectInstanceHandle(objectInstanceHandle),
        attributes,
        userSuppliedTag);
  });
}

void queueAttributeValueUpdateClassProvide(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
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

void queueObjectInstanceDiscovery(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle) {
  callbackRoute([
      federationName = std::move(federationName),
      receivingFederateId,
      objectInstanceHandle](FederateAmbassador& recipient) {
    std::optional<umbra::detail::KnownObjectInstanceSnapshot> discovery;
    {
      std::scoped_lock lock(federationManagementMutex());
      discovery = embeddedFederationManagement().registry().beginObjectInstanceDiscovery(
          federationName,
          receivingFederateId,
          objectInstanceHandle);
    }
    if (!discovery) {
      return;
    }

    // The registry commits the recipient's known-instance state before this
    // callback, so a FederateAmbassador may safely use the matching 2025
    // support services from within Discover Object Instance.
    recipient.discoverObjectInstance(
        makeObjectInstanceHandle(discovery->objectInstanceHandle),
        makeObjectClassHandle(discovery->knownObjectClassHandle),
        discovery->objectInstanceName,
        makeFederateHandle(discovery->producingFederateId));
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
          federationName,
          discovery.receivingFederateId,
          discovery.objectInstanceHandle);
    } catch (...) {
      // An immediate callback can have committed known-instance state before
      // propagating a user exception.  The registry clears only still-pending
      // reservations, preserving that committed state while allowing all
      // undelivered recipients to be reconsidered later.
      std::scoped_lock lock(federationManagementMutex());
      auto& registry = embeddedFederationManagement().registry();
      for (std::size_t pending = index; pending < discoveries.size(); ++pending) {
        registry.cancelObjectInstanceDiscovery(
            federationName,
            discoveries[pending].receivingFederateId,
            discoveries[pending].objectInstanceHandle);
      }
      throw;
    }
  }
}

void queueObjectInstanceScopeChanges(
    std::vector<umbra::detail::ObjectInstanceScopeChangeRecipient> changes,
    std::wstring const& federationName) {
  for (auto& change : changes) {
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

void queueObjectInstanceRemoval(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    VariableLengthData userSuppliedTag) {
  callbackRoute([
      federationName = std::move(federationName),
      receivingFederateId,
      objectInstanceHandle,
      userSuppliedTag = std::move(userSuppliedTag)](FederateAmbassador& recipient) mutable {
    std::optional<umbra::detail::RemovedObjectInstanceSnapshot> removal;
    {
      std::scoped_lock lock(federationManagementMutex());
      removal = embeddedFederationManagement().registry().beginObjectInstanceRemoval(
          federationName,
          receivingFederateId,
          objectInstanceHandle);
    }
    if (!removal) {
      return;
    }

    // Removal commits the recipient's transition to unknown before its
    // callback. That mirrors the lifecycle boundary rather than preserving a
    // stale instance through user code after Remove Object Instance begins.
    recipient.removeObjectInstance(
        makeObjectInstanceHandle(removal->objectInstanceHandle),
        userSuppliedTag,
        makeFederateHandle(removal->producingFederateId));
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
          federationName,
          removal.receivingFederateId,
          removal.objectInstanceHandle,
          userSuppliedTag);
    } catch (...) {
      // A synchronous callback may already have removed the current
      // recipient's known state. Clear only still-pending reservations so the
      // remaining known recipients are not stranded in an in-flight state.
      std::scoped_lock lock(federationManagementMutex());
      auto& registry = embeddedFederationManagement().registry();
      for (std::size_t pending = index; pending < removals.size(); ++pending) {
        registry.cancelObjectInstanceRemoval(
            federationName,
            removals[pending].receivingFederateId,
            removals[pending].objectInstanceHandle);
      }
      throw;
    }
  }
}

void queueTimestampedObjectInstanceRemoval(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
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
  callbackRoute([
      federationName = std::move(federationName),
      receivingFederateId,
      objectInstanceHandle,
      messageId,
      userSuppliedTag = std::move(userSuppliedTag),
      timestamp = std::move(timestamp),
      provideRetraction](FederateAmbassador& recipient) mutable {
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
    std::uint64_t generation) {
  std::weak_ptr<umbra::detail::CallbackDispatcher> const dispatcher = callbackDispatcher;
  std::weak_ptr<CallbackSession> const session = callbackSession;
  std::weak_ptr<umbra::detail::FederateTimeState> const federateTimeState = timeState;

  return [dispatcher, session, federateTimeState, federationName = std::move(federationName), federateId, generation] {
    auto callbackDispatcher = dispatcher.lock();
    if (!callbackDispatcher) {
      return;
    }

    callbackDispatcher->submit([
        session,
        federateTimeState,
        federationName,
        federateId,
        generation] {
      auto callbackSession = session.lock();
      auto timeState = federateTimeState.lock();
      if (!callbackSession || !timeState) {
        return;
      }

      std::vector<umbra::detail::FederationTimeGrantDispatch> newlyEligible;
      callbackSession->invoke([
          timeState = std::move(timeState),
          federationName,
          federateId,
          generation,
          &newlyEligible](FederateAmbassador& recipient) {
        std::shared_ptr<LogicalTime const> grantedTime;
        std::vector<umbra::detail::TsoPayloadDelivery> tsoDeliveries;
        {
          std::scoped_lock lock(federationManagementMutex());
          auto& registry = embeddedFederationManagement().registry();
          if (registry.beginTimeAdvanceGrant(federationName, federateId, generation) !=
              umbra::detail::FederationTimeGrantStatus::applied) {
            return;
          }
          grantedTime = timeState->grant(generation);
          if (!grantedTime) {
            return;
          }
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
          auto scheduled = registry.reevaluateTimeAdvanceGrants(federationName);
          if (scheduled.status == umbra::detail::FederationTimeGrantStatus::applied) {
            newlyEligible = std::move(scheduled.dispatches);
          }
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
                    {
                      std::scoped_lock lock(federationManagementMutex());
                      projection = embeddedFederationManagement().registry()
                          .receiveOrderInteractionRecipientFor(
                              federationName,
                              message.producingFederateId,
                              federateId,
                              message.sentInteractionClassHandle,
                              message.sentParameterHandles);
                    }
                    if (!projection) {
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
                    auto retraction = makeMessageRetractionHandle(message.messageId);
                    recipient.receiveInteraction(
                        makeInteractionClassHandle(projection->receivedInteractionClassHandle),
                        parameterValues,
                        message.userSuppliedTag,
                        makeTransportationTypeHandle(*transportationValue),
                        makeFederateHandle(message.producingFederateId),
                        nullptr,
                        *message.timestamp,
                        TIMESTAMP,
                        TIMESTAMP,
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
                    for (auto const& passel : passels->second) {
                      std::optional<umbra::detail::ReceiveOrderAttributeUpdateRecipient>
                          projection;
                      {
                        std::scoped_lock lock(federationManagementMutex());
                        projection = embeddedFederationManagement().registry()
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
                      std::optional<RegionHandleSet> optionalSentRegions;
                      if (!passel.sentRegionHandles.empty()) {
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
                          TIMESTAMP,
                          TIMESTAMP,
                          &retraction);
                    }
                  } else {
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

                    auto retraction = makeMessageRetractionHandle(message.messageId);
                    recipient.removeObjectInstance(
                        makeObjectInstanceHandle(removal->objectInstanceHandle),
                        message.userSuppliedTag,
                        makeFederateHandle(removal->producingFederateId),
                        *message.timestamp,
                        TIMESTAMP,
                        TIMESTAMP,
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
          auto const completed = embeddedFederationManagement().registry().completeTsoDelivery(
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
        }

        // The state mutation happens immediately before its matching standard
        // callback, after the registry has rechecked the current shared bound.
        recipient.timeAdvanceGrant(*grantedTime);
      });

      submitTimeAdvanceGrantDispatches(std::move(newlyEligible));
    });
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

ConfigurationResult UmbraRtiAmbassador::connect(
    FederateAmbassador& federateAmbassador,
    CallbackModel callbackModel) {
  return connectImpl(federateAmbassador, callbackModel, nullptr);
}

ConfigurationResult UmbraRtiAmbassador::connect(
    FederateAmbassador& federateAmbassador,
    CallbackModel callbackModel,
    RtiConfiguration const& configuration) {
  return connectImpl(federateAmbassador, callbackModel, &configuration);
}

ConfigurationResult UmbraRtiAmbassador::connect(
    FederateAmbassador& federateAmbassador,
    CallbackModel callbackModel,
    Credentials const& credentials) {
  static_cast<void>(credentials);
  return connectImpl(federateAmbassador, callbackModel, nullptr);
}

ConfigurationResult UmbraRtiAmbassador::connect(
    FederateAmbassador& federateAmbassador,
    CallbackModel callbackModel,
    RtiConfiguration const& configuration,
    Credentials const& credentials) {
  static_cast<void>(credentials);
  return connectImpl(federateAmbassador, callbackModel, &configuration);
}

ConfigurationResult UmbraRtiAmbassador::connectImpl(
    FederateAmbassador& federateAmbassador,
    CallbackModel callbackModel,
    RtiConfiguration const* configuration) {
  validateCallbackModel(callbackModel);
  static_cast<void>(configuration);

  // Construct the truthful result before mutating lifecycle state so a failed
  // allocation cannot leave a partially established connection behind.
  ConfigurationResult result = ignoredConfigurationResult();
  auto callbackSession = std::make_shared<CallbackSession>(federateAmbassador);

  std::scoped_lock lock(mutex_);
  if (lifecycle_.apply(umbra::detail::FederateLifecycleEvent::connect) !=
      umbra::detail::FederateLifecycleResult::applied) {
    throw AlreadyConnected(L"The RTI ambassador already has an active connection.");
  }

  callbackSession_ = std::move(callbackSession);
  callbackModel_ = callbackModel;
  callbacks_->configure(toDispatchModel(callbackModel));
  return result;
}

void UmbraRtiAmbassador::disconnect() {
  std::shared_ptr<CallbackSession> callbackSession;
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
    callbacks_->reset();
  }

  // Do not hold the ambassador lock while an in-flight callback drains: the
  // recipient may make a re-entrant RTI call before it returns.
  if (callbackSession) {
    callbackSession->close();
  }
}

bool UmbraRtiAmbassador::evokeCallback(double approximateMinimumTimeInSeconds) {
  if (callbacks_->isExecutingCallback()) {
    throw CallNotAllowedFromWithinCallback(
        L"Evoke Callback cannot be called from within a federate callback.");
  }
  return callbacks_->evokeOne(callbackWaitDuration(approximateMinimumTimeInSeconds));
}

bool UmbraRtiAmbassador::evokeMultipleCallbacks(
    double approximateMinimumTimeInSeconds,
    double approximateMaximumTimeInSeconds) {
  if (callbacks_->isExecutingCallback()) {
    throw CallNotAllowedFromWithinCallback(
        L"Evoke Multiple Callbacks cannot be called from within a federate callback.");
  }
  return callbacks_->evokeMultiple(
      callbackWaitDuration(approximateMinimumTimeInSeconds),
      callbackWaitDuration(approximateMaximumTimeInSeconds));
}

void UmbraRtiAmbassador::enableCallbacks() {
  callbacks_->setEnabled(true);
}

void UmbraRtiAmbassador::disableCallbacks() {
  callbacks_->setEnabled(false);
}

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)

void UmbraRtiAmbassador::createFederationExecution(
    std::wstring const& federationName,
    std::wstring const& fomModule,
    std::wstring const& logicalTimeImplementationName) {
  createFederationExecution(
      federationName,
      std::vector<std::wstring>{fomModule},
      logicalTimeImplementationName);
}

void UmbraRtiAmbassador::createFederationExecution(
    std::wstring const& federationName,
    std::vector<std::wstring> const& fomModules,
    std::wstring const& logicalTimeImplementationName) {
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
}

void UmbraRtiAmbassador::createFederationExecutionWithMIM(
    std::wstring const& federationName,
    std::vector<std::wstring> const& fomModules,
    std::wstring const& mimModule,
    std::wstring const& logicalTimeImplementationName) {
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
}

void UmbraRtiAmbassador::destroyFederationExecution(std::wstring const& federationName) {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);

  auto destroyed = embeddedFederationManagement().registry().destroy(federationName);
  switch (destroyed.status) {
    case umbra::detail::FederationRegistryStatus::applied:
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
}

void UmbraRtiAmbassador::listFederationExecutions() {
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
  if (callbacks_->isExecutingCallback()) {
    throw CallNotAllowedFromWithinCallback(
        L"Join Federation Execution cannot be called from within a federate callback.");
  }

  FederateHandle result;
  std::vector<umbra::detail::FederationTimeGrantDispatch> newlyEligible;
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
    if (additionalFomModules.empty()) {
      // Allocate and validate the initial logical-time state before the registry
      // commits membership, so a factory/allocation failure cannot leave a
      // joined federate without the required initial time position.
      timeState = makeFederateTimeState(management, *currentDefinition);
      joined = management.registry().joinWithTimeState(
          federationName,
          timeState,
          federateType,
          std::move(requestedFederateName),
          std::move(callbackRoute));
    } else {
      auto preparation = management.coordinator().prepareAdditionalModules(
          *currentDefinition,
          additionalFomModules);
      if (!preparation.accepted()) {
        throwPreparationFailure(preparation);
      }
      timeState = makeFederateTimeState(management, *preparation.definition);
      joined = management.registry().joinWithDefinitionAndTimeState(
          federationName,
          std::move(*preparation.definition),
          timeState,
          federateType,
          std::move(requestedFederateName),
          std::move(callbackRoute));
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
    if (!joined.membership ||
        lifecycle_.apply(umbra::detail::FederateLifecycleEvent::join) !=
            umbra::detail::FederateLifecycleResult::applied) {
      throw RTIinternalError(L"Umbra could not complete the Join Federation Execution transition.");
    }

    joinedFederationName_ = federationName;
    joinedFederateId_ = joined.membership->id;
    federateTimeState_ = std::move(timeState);
    result = makeFederateHandle(joined.membership->id);

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
  return result;
}

void UmbraRtiAmbassador::resignFederationExecution(ResignAction resignAction) {
  if (callbacks_->isExecutingCallback()) {
    throw CallNotAllowedFromWithinCallback(
        L"Resign Federation Execution cannot be called from within a federate callback.");
  }

  std::vector<umbra::detail::FederationTimeGrantDispatch> newlyEligible;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"This RTI ambassador is not a member of a federation execution.");
    }
    requireValidResignAction(resignAction);

    std::wstring const federationName = *joinedFederationName_;
    // The limited object-registration/discovery profile preserves its private
    // instance records across resignation only so queued callbacks can safely
    // recheck membership. It does not implement the distinct 2025 resign
    // action dispositions or claim object-lifecycle/ownership behavior.
    auto resigned = embeddedFederationManagement().registry().resign(
        federationName,
        *joinedFederateId_);
    if (resigned.status == umbra::detail::FederationRegistryStatus::federate_not_member ||
        resigned.status == umbra::detail::FederationRegistryStatus::federation_does_not_exist) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    if (resigned.status != umbra::detail::FederationRegistryStatus::applied ||
        lifecycle_.apply(umbra::detail::FederateLifecycleEvent::resign) !=
            umbra::detail::FederateLifecycleResult::applied) {
      throw RTIinternalError(L"Umbra could not complete the Resign Federation Execution transition.");
    }

    if (federateTimeState_) {
      federateTimeState_->deactivate();
    }
    federateTimeState_.reset();
    joinedFederationName_.reset();
    joinedFederateId_.reset();

    auto scheduled = embeddedFederationManagement().registry().reevaluateTimeAdvanceGrants(
        federationName);
    if (scheduled.status == umbra::detail::FederationTimeGrantStatus::applied) {
      newlyEligible = std::move(scheduled.dispatches);
    }
  }
  submitTimeAdvanceGrantDispatches(std::move(newlyEligible));
}

std::unique_ptr<LogicalTimeFactory> UmbraRtiAmbassador::getTimeFactory() const {
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
}

std::wstring UmbraRtiAmbassador::getFederateName(FederateHandle const& federate) {
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

  auto membership = registry.memberById(*joinedFederationName_, *federateId);
  if (!membership) {
    throw FederateHandleNotKnown(
        L"The supplied FederateHandle is not active in this federation execution.");
  }
  return membership->name;
}

ObjectClassHandle UmbraRtiAmbassador::getObjectClassHandle(
    std::wstring const& objectClassName) {
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
}

std::wstring UmbraRtiAmbassador::getObjectClassName(ObjectClassHandle const& objectClass) {
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
}

AttributeHandle UmbraRtiAmbassador::getAttributeHandle(
    ObjectClassHandle const& objectClass,
    std::wstring const& attributeName) {
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
}

std::wstring UmbraRtiAmbassador::getAttributeName(
    ObjectClassHandle const& objectClass,
    AttributeHandle const& attribute) {
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
}

void UmbraRtiAmbassador::publishObjectClassAttributes(
    ObjectClassHandle const& objectClass,
    AttributeHandleSet const& attributes) {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
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
}

void UmbraRtiAmbassador::unpublishObjectClassAttributes(
    ObjectClassHandle const& objectClass,
    AttributeHandleSet const& attributes) {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
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
}

void UmbraRtiAmbassador::subscribeObjectClassAttributes(
    ObjectClassHandle const& objectClass,
    AttributeHandleSet const& attributes,
    bool active,
    std::wstring const& updateRateDesignator) {
  // This profile's receive-order update/reflection path has no update-rate
  // reduction, so an update-rate designator has no runtime effect yet.
  // Accepting it preserves the exact 2025 declaration-management API boundary.
  static_cast<void>(updateRateDesignator);

  std::wstring federationName;
  std::vector<umbra::detail::ObjectInstanceDiscoveryRecipient> discoveries;
  std::vector<umbra::detail::ObjectInstanceScopeChangeRecipient> changes;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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
    auto result = registry.setObjectClassAttributeSubscriptionWithScopeChanges(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectClassHandle,
        *attributeHandles,
        active);
    if (result.status != umbra::detail::ObjectClassAttributeDeclarationStatus::applied) {
      throwObjectClassAttributeDeclarationFailure(result.status);
    }

    federationName = *joinedFederationName_;
    changes = std::move(result.recipients);
    discoveries = registry.planObjectInstanceDiscoveriesForFederate(
        federationName,
        *joinedFederateId_);
  }
  queueObjectInstanceScopeChanges(std::move(changes), federationName);
  queueObjectInstanceDiscoveries(std::move(discoveries), federationName);
}

void UmbraRtiAmbassador::unsubscribeObjectClassAttributes(
    ObjectClassHandle const& objectClass,
    AttributeHandleSet const& attributes) {
  std::wstring federationName;
  std::vector<umbra::detail::ObjectInstanceScopeChangeRecipient> changes;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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
        std::nullopt);
    if (result.status != umbra::detail::ObjectClassAttributeDeclarationStatus::applied) {
      throwObjectClassAttributeDeclarationFailure(result.status);
    }
    federationName = *joinedFederationName_;
    changes = std::move(result.recipients);
  }
  queueObjectInstanceScopeChanges(std::move(changes), federationName);
}

void UmbraRtiAmbassador::reserveObjectInstanceName(
    std::wstring const& objectInstanceName) {
  std::wstring federationName;
  std::uint64_t federateId = 0;
  umbra::detail::ObjectInstanceNameReservationResult result;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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
  }
  queueObjectInstanceNameReservation(
      std::move(result.callbackRoute),
      std::move(federationName),
      federateId,
      result.succeeded,
      std::move(result.objectInstanceName));
}

void UmbraRtiAmbassador::releaseObjectInstanceName(
    std::wstring const& objectInstanceName) {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
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
}

void UmbraRtiAmbassador::reserveMultipleObjectInstanceNames(
    std::set<std::wstring> const& objectInstanceNames) {
  std::wstring federationName;
  std::uint64_t federateId = 0;
  umbra::detail::MultipleObjectInstanceNameReservationResult result;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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
}

void UmbraRtiAmbassador::releaseMultipleObjectInstanceNames(
    std::set<std::wstring> const& objectInstanceNames) {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
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
}

ObjectInstanceHandle UmbraRtiAmbassador::registerObjectInstanceWithRegions(
    ObjectClassHandle const& objectClass,
    AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions) {
  std::wstring federationName;
  std::vector<umbra::detail::ObjectInstanceDiscoveryRecipient> discoveries;
  std::uint64_t objectInstanceHandle = 0;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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
}

ObjectInstanceHandle UmbraRtiAmbassador::registerObjectInstanceWithRegions(
    ObjectClassHandle const& objectClass,
    AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions,
    std::wstring const& objectInstanceName) {
  std::wstring federationName;
  std::vector<umbra::detail::ObjectInstanceDiscoveryRecipient> discoveries;
  std::uint64_t objectInstanceHandle = 0;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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
}

void UmbraRtiAmbassador::associateRegionsForUpdates(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions) {
  std::wstring federationName;
  std::vector<umbra::detail::ObjectInstanceScopeChangeRecipient> changes;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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
    federationName = *joinedFederationName_;
    changes = std::move(result.recipients);
  }
  queueObjectInstanceScopeChanges(std::move(changes), federationName);
}

void UmbraRtiAmbassador::unassociateRegionsForUpdates(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions) {
  std::wstring federationName;
  std::vector<umbra::detail::ObjectInstanceScopeChangeRecipient> changes;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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
    federationName = *joinedFederationName_;
    changes = std::move(result.recipients);
  }
  queueObjectInstanceScopeChanges(std::move(changes), federationName);
}

void UmbraRtiAmbassador::subscribeObjectClassAttributesWithRegions(
    ObjectClassHandle const& objectClass,
    AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions,
    bool active,
    std::wstring const& updateRateDesignator) {
  static_cast<void>(updateRateDesignator);
  std::wstring federationName;
  std::vector<umbra::detail::ObjectInstanceDiscoveryRecipient> discoveries;
  std::vector<umbra::detail::ObjectInstanceScopeChangeRecipient> changes;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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
    auto result = registry.setObjectClassAttributeRegionalSubscriptionWithScopeChanges(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectClassHandle,
        pairValues.values,
        active);
    if (result.status !=
        umbra::detail::RegionalObjectClassAttributeDeclarationStatus::applied) {
      throwRegionalObjectClassAttributeDeclarationFailure(result.status);
    }
    federationName = *joinedFederationName_;
    changes = std::move(result.recipients);
    discoveries = registry.planObjectInstanceDiscoveriesForFederate(
        federationName,
        *joinedFederateId_);
  }
  queueObjectInstanceScopeChanges(std::move(changes), federationName);
  queueObjectInstanceDiscoveries(std::move(discoveries), federationName);
}

void UmbraRtiAmbassador::unsubscribeObjectClassAttributesWithRegions(
    ObjectClassHandle const& objectClass,
    AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions) {
  std::wstring federationName;
  std::vector<umbra::detail::ObjectInstanceScopeChangeRecipient> changes;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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
    federationName = *joinedFederationName_;
    changes = std::move(result.recipients);
  }
  queueObjectInstanceScopeChanges(std::move(changes), federationName);
}

ObjectInstanceHandle UmbraRtiAmbassador::registerObjectInstance(
    ObjectClassHandle const& objectClass) {
  std::wstring federationName;
  std::vector<umbra::detail::ObjectInstanceDiscoveryRecipient> discoveries;
  std::uint64_t objectInstanceHandle = 0;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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
}

ObjectInstanceHandle UmbraRtiAmbassador::registerObjectInstance(
    ObjectClassHandle const& objectClass,
    std::wstring const& objectInstanceName) {
  std::wstring federationName;
  std::vector<umbra::detail::ObjectInstanceDiscoveryRecipient> discoveries;
  std::uint64_t objectInstanceHandle = 0;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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
}

void UmbraRtiAmbassador::deleteObjectInstance(
    ObjectInstanceHandle const& objectInstance,
    VariableLengthData const& userSuppliedTag) {
  std::wstring federationName;
  std::vector<umbra::detail::ObjectInstanceRemovalRecipient> removals;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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

    federationName = *joinedFederationName_;
    removals = std::move(deletion.recipients);
  }
  queueObjectInstanceRemovals(std::move(removals), federationName, userSuppliedTag);
}

MessageRetractionHandle UmbraRtiAmbassador::deleteObjectInstance(
    ObjectInstanceHandle const& objectInstance,
    VariableLengthData const& userSuppliedTag,
    LogicalTime const& time) {
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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
}

void UmbraRtiAmbassador::localDeleteObjectInstance(
    ObjectInstanceHandle const& objectInstance) {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
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
}

void UmbraRtiAmbassador::updateAttributeValues(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandleValueMap const& attributeValues,
    VariableLengthData const& userSuppliedTag) {
  // Preserve the 2025 service's connection and membership preconditions ahead
  // of caller-supplied handle validation, matching the other public services.
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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
  auto plan = planCurrentRequest();

  struct Delivery {
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute;
    std::uint64_t recipientId = 0;
    std::vector<std::uint64_t> sentAttributeHandles;
    TransportationTypeHandle transportationType;
    std::optional<std::set<std::uint64_t>> sentRegionHandles;
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
          passel.sentRegionHandles.empty()
              ? std::nullopt
              : std::optional<std::set<std::uint64_t>>(passel.sentRegionHandles),
      });
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
        std::move(delivery.sentRegionHandles));
  }
}

MessageRetractionHandle UmbraRtiAmbassador::updateAttributeValues(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandleValueMap const& attributeValues,
    VariableLengthData const& userSuppliedTag,
    LogicalTime const& time) {
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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

  struct Delivery {
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute;
    std::vector<umbra::detail::TsoAttributeUpdatePassel> passels;
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
    payloadPassel.sentAttributeHandles = passel.sentAttributeHandles;
    payloadPassel.sentRegionHandles = passel.sentRegionHandles;
    for (auto const& recipient : passel.recipients) {
      if (!recipient.callbackRoute) {
        throw RTIinternalError(
            L"The embedded federation has an eligible timestamped attribute recipient without a callback route.");
      }
      auto& delivery = deliveries[recipient.federateId];
      if (!delivery.callbackRoute) {
        delivery.callbackRoute = recipient.callbackRoute;
      }
      delivery.passels.push_back(payloadPassel);
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
  if (timeSnapshot.timeRegulating) {
    std::vector<std::uint64_t> tsoRecipientIds;
    for (auto const& [recipientId, delivery] : deliveries) {
      static_cast<void>(delivery);
      if (timeConstrainedRecipients.contains(recipientId)) {
        tsoRecipientIds.push_back(recipientId);
      }
    }

    if (!tsoRecipientIds.empty()) {
      umbra::detail::TsoAttributeUpdateMessage message;
      message.producingFederateId = *producingFederateId;
      message.objectInstanceHandle = *objectInstanceHandle;
      message.attributes = sentAttributes;
      message.userSuppliedTag = copiedTag;
      message.timestamp = std::shared_ptr<LogicalTime const>(timestamp);
      for (auto const& [recipientId, delivery] : deliveries) {
        message.passelsByRecipient.emplace(recipientId, delivery.passels);
      }

      std::scoped_lock lock(federationManagementMutex());
      auto const result = embeddedFederationManagement().registry()
          .enqueueTsoAttributeUpdate(
              *federationName,
              std::move(message),
              tsoRecipientIds);
      if (result.status != umbra::detail::FederationTsoRegistryStatus::applied ||
          result.queueStatus != umbra::detail::TsoMessageQueueStatus::applied ||
          result.messageId == 0) {
        throw RTIinternalError(
            L"The embedded federation could not queue the timestamped Update Attribute Values service.");
      }
      messageId = result.messageId;
    } else if (!deliveries.empty()) {
      std::scoped_lock lock(federationManagementMutex());
      auto const result = embeddedFederationManagement().registry().allocateTsoMessageId(
          *federationName);
      if (result.status != umbra::detail::FederationTsoRegistryStatus::applied ||
          result.messageId == 0) {
        throw RTIinternalError(
            L"The embedded federation could not allocate a timestamped message designator.");
      }
      messageId = result.messageId;
    }
  }

  // Time-constrained recipients consume the typed payload at their grant.
  // Other recipients receive the timestamped callback immediately in the
  // bounded embedded profile; the callback still rechecks each accepted
  // passel immediately before entering user code.
  for (auto& [recipientId, delivery] : deliveries) {
    if (timeSnapshot.timeRegulating &&
        timeConstrainedRecipients.contains(recipientId)) {
      continue;
    }
    queueTimestampedReflectAttributeUpdate(
        std::move(delivery.callbackRoute),
        *federationName,
        *producingFederateId,
        recipientId,
        *objectInstanceHandle,
        sentAttributes,
        std::move(delivery.passels),
        copiedTag,
        std::shared_ptr<LogicalTime const>(timestamp),
        timeSnapshot.timeRegulating ? TIMESTAMP : RECEIVE,
        RECEIVE,
        timeSnapshot.timeRegulating && messageId != 0
            ? std::optional<std::uint64_t>(messageId)
            : std::nullopt);
  }

  if (!timeSnapshot.timeRegulating || messageId == 0) {
    return MessageRetractionHandle();
  }
  return makeMessageRetractionHandle(messageId);
}

void UmbraRtiAmbassador::requestAttributeValueUpdate(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandleSet const& attributes,
    VariableLengthData const& userSuppliedTag) {
  // Preserve the official connection and membership preconditions before
  // caller-supplied handle validation, as with the other object services.
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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

  std::optional<std::wstring> federationName;
  std::optional<std::uint64_t> requestingFederateId;
  auto planCurrentRequest = [&]() {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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
    return plan;
  };

  // The request has no caller-owned value payload, but copy the tag only
  // after its current object and attribute boundary has been validated. Then
  // replan before routing so a concurrent resign or lifecycle change cannot
  // use a stale recipient snapshot.
  static_cast<void>(planCurrentRequest());
  VariableLengthData copiedTag(userSuppliedTag);
  auto plan = planCurrentRequest();

  struct Delivery {
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute;
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
        recipient.providingFederateId,
        recipient.requestedAttributeHandles,
    });
  }

  // Do not hold the requester lock while submitting a route: HLA_IMMEDIATE
  // may synchronously enter the owner's Provide Attribute Value Update callback.
  for (auto& delivery : deliveries) {
    queueAttributeValueUpdateProvide(
        std::move(delivery.callbackRoute),
        *federationName,
        *requestingFederateId,
        delivery.providingFederateId,
        *objectInstanceHandle,
        std::move(delivery.requestedAttributeHandles),
        copiedTag);
  }
}

void UmbraRtiAmbassador::requestAttributeValueUpdate(
    ObjectClassHandle const& objectClass,
    AttributeHandleSet const& attributes,
    VariableLengthData const& userSuppliedTag) {
  // Preserve the official connection and membership preconditions before
  // caller-supplied handle validation, as with the object-instance form.
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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

  std::optional<std::wstring> federationName;
  std::optional<std::uint64_t> requestingFederateId;
  auto planCurrentRequest = [&]() {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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
    return plan;
  };

  // Copy the tag only after the selected class and its attributes have been
  // validated. Replanning immediately before routing prevents a concurrent
  // resign or lifecycle transition from using a stale owner snapshot.
  static_cast<void>(planCurrentRequest());
  VariableLengthData copiedTag(userSuppliedTag);
  auto plan = planCurrentRequest();

  struct Delivery {
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute;
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
        *federationName,
        *requestingFederateId,
        delivery.providingFederateId,
        delivery.objectInstanceHandle,
        *requestedObjectClassHandle,
        std::move(delivery.requestedAttributeHandles),
        copiedTag);
  }
}

void UmbraRtiAmbassador::requestAttributeValueUpdateWithRegions(
    ObjectClassHandle const& objectClass,
    AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions,
    VariableLengthData const& userSuppliedTag) {
  // Preserve the official connection and membership preconditions before
  // caller-supplied handle validation, matching the ordinary class request.
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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
  }

  struct Delivery {
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute;
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
        federationName,
        requestingFederateId,
        delivery.providingFederateId,
        delivery.objectInstanceHandle,
        *requestedObjectClassHandle,
        std::move(delivery.requestedAttributeHandles),
        copiedTag,
        pairValues.values);
  }
}

void UmbraRtiAmbassador::queryAttributeOwnership(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandleSet const& attributes) {
  // Preserve the official connection and membership preconditions before
  // caller-supplied handle validation, as with the adjacent object services.
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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

  std::wstring federationName;
  std::uint64_t requestingFederateId = 0;
  umbra::detail::AttributeOwnershipQueryPlan plan;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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
}

bool UmbraRtiAmbassador::isAttributeOwnedByFederate(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandle const& attribute) {
  // This uses the same official connection, membership, known-instance, and
  // known-class boundaries as Query Attribute Ownership, but returns only the
  // invoking federate's boolean ownership status and has no callback effect.
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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
}

void UmbraRtiAmbassador::negotiatedAttributeOwnershipDivestiture(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandleSet const& attributes,
    VariableLengthData const& userSuppliedTag) {
  // Preserve the official connection and membership preconditions before
  // caller-supplied handle validation, matching the adjacent ownership
  // services. Save/restore has no implemented state in this profile.
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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

  std::wstring federationName;
  std::vector<umbra::detail::AttributeOwnershipAcquisitionWorkItem> workItems;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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
    workItems = std::move(plan.workItems);
  }

  queueAttributeOwnershipAcquisitionWorkItems(std::move(workItems), federationName);
}

void UmbraRtiAmbassador::confirmDivestiture(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandleSet const& confirmedAttributes,
    VariableLengthData const& userSuppliedTag) {
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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

  std::wstring federationName;
  umbra::detail::ConfirmDivestiturePlan plan;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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
  }

  queueConfirmDivestitureNotifications(std::move(plan.notifications), federationName);
}

void UmbraRtiAmbassador::cancelNegotiatedAttributeOwnershipDivestiture(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandleSet const& attributes) {
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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

  std::wstring federationName;
  std::vector<umbra::detail::AttributeOwnershipAcquisitionWorkItem> followupWorkItems;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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
    followupWorkItems = std::move(plan.followupWorkItems);
  }

  queueAttributeOwnershipAcquisitionWorkItems(std::move(followupWorkItems), federationName);
}

void UmbraRtiAmbassador::unconditionalAttributeOwnershipDivestiture(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandleSet const& attributes,
    VariableLengthData const& userSuppliedTag) {
  // Preserve the official connection and membership preconditions before
  // caller-supplied handle validation, matching the adjacent ownership
  // services. Save/restore has no implemented state in this profile.
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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

  std::wstring federationName;
  umbra::detail::UnconditionalAttributeOwnershipDivestiturePlan plan;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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
}

void UmbraRtiAmbassador::attributeOwnershipAcquisition(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandleSet const& desiredAttributes,
    VariableLengthData const& userSuppliedTag) {
  // Preserve the official connection and membership preconditions before
  // caller-supplied handle validation, as with the adjacent ownership
  // services. Save/restore has no implemented state in this profile.
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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

  std::wstring federationName;
  std::vector<umbra::detail::AttributeOwnershipAcquisitionWorkItem> workItems;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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
    workItems = std::move(plan.workItems);
  }

  queueAttributeOwnershipAcquisitionWorkItems(std::move(workItems), federationName);
}

void UmbraRtiAmbassador::attributeOwnershipAcquisitionIfAvailable(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandleSet const& desiredAttributes,
    VariableLengthData const& userSuppliedTag) {
  // Preserve the official connection and membership preconditions before
  // caller-supplied handle validation, as with the adjacent ownership
  // services. Save/restore has no implemented state in this profile.
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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

  std::wstring federationName;
  std::uint64_t requestingFederateId = 0;
  umbra::detail::AttributeOwnershipAcquisitionIfAvailablePlan plan;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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
  }

  // The empty-attribute case has no ownership transition or callback.
  if (plan.requestId == 0) {
    return;
  }
  if (!plan.callbackRoute) {
    std::scoped_lock lock(federationManagementMutex());
    embeddedFederationManagement().registry().cancelAttributeOwnershipAcquisitionIfAvailable(
        federationName,
        requestingFederateId,
        *objectInstanceHandle,
        plan.requestId);
    throw RTIinternalError(
        L"The embedded federation has an ownership-acquisition requester without a callback route.");
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
}

void UmbraRtiAmbassador::attributeOwnershipReleaseDenied(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandleSet const& attributes,
    VariableLengthData const& userSuppliedTag) {
  // Preserve the official connection and membership preconditions before
  // caller-supplied handle validation, matching regular acquisition.
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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

  std::wstring federationName;
  std::vector<umbra::detail::AttributeOwnershipUnavailableRecipient> recipients;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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
    recipients = std::move(plan.recipients);
  }

  queueAttributeOwnershipUnavailableRecipients(
      std::move(recipients),
      federationName,
      copiedTag);
}

void UmbraRtiAmbassador::attributeOwnershipDivestitureIfWanted(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandleSet const& attributes,
    VariableLengthData const& userSuppliedTag,
    AttributeHandleSet& divestedAttributes) {
  // Preserve the official connection and membership preconditions before
  // caller-supplied handle validation, matching the adjacent ownership
  // services. Save/restore has no implemented state in this profile.
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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
}

void UmbraRtiAmbassador::cancelAttributeOwnershipAcquisition(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandleSet const& attributes) {
  // Preserve the official connection and membership preconditions before
  // caller-supplied handle validation, matching the regular acquisition path.
  // Save/restore has no implemented state in this profile.
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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

  std::wstring federationName;
  std::uint64_t requestingFederateId = 0;
  umbra::detail::AttributeOwnershipAcquisitionCancellationPlan plan;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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
  }

  // An empty attribute set has no cancellation transition or callback.
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
}

ObjectClassHandle UmbraRtiAmbassador::getKnownObjectClassHandle(
    ObjectInstanceHandle const& objectInstance) {
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
}

ObjectInstanceHandle UmbraRtiAmbassador::getObjectInstanceHandle(
    std::wstring const& objectInstanceName) {
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
}

std::wstring UmbraRtiAmbassador::getObjectInstanceName(
    ObjectInstanceHandle const& objectInstance) {
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
}

InteractionClassHandle UmbraRtiAmbassador::getInteractionClassHandle(
    std::wstring const& interactionClassName) {
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
}

std::wstring UmbraRtiAmbassador::getInteractionClassName(
    InteractionClassHandle const& interactionClass) {
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
}

ParameterHandle UmbraRtiAmbassador::getParameterHandle(
    InteractionClassHandle const& interactionClass,
    std::wstring const& parameterName) {
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
}

std::wstring UmbraRtiAmbassador::getParameterName(
    InteractionClassHandle const& interactionClass,
    ParameterHandle const& parameter) {
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
}

void UmbraRtiAmbassador::publishInteractionClass(
    InteractionClassHandle const& interactionClass) {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
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
}

void UmbraRtiAmbassador::unpublishInteractionClass(
    InteractionClassHandle const& interactionClass) {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
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
}

void UmbraRtiAmbassador::publishObjectClassDirectedInteractions(
    ObjectClassHandle const& objectClass,
    InteractionClassHandleSet const& interactionClasses) {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
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
}

void UmbraRtiAmbassador::unpublishObjectClassDirectedInteractions(
    ObjectClassHandle const& objectClass) {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
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
}

void UmbraRtiAmbassador::unpublishObjectClassDirectedInteractions(
    ObjectClassHandle const& objectClass,
    InteractionClassHandleSet const& interactionClasses) {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
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
}

void UmbraRtiAmbassador::subscribeInteractionClass(
    InteractionClassHandle const& interactionClass,
    bool active) {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
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
}

void UmbraRtiAmbassador::unsubscribeInteractionClass(
    InteractionClassHandle const& interactionClass) {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
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
}

void UmbraRtiAmbassador::subscribeObjectClassDirectedInteractions(
    ObjectClassHandle const& objectClass,
    InteractionClassHandleSet const& interactionClasses,
    bool universally) {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
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
}

void UmbraRtiAmbassador::unsubscribeObjectClassDirectedInteractions(
    ObjectClassHandle const& objectClass) {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
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
}

void UmbraRtiAmbassador::unsubscribeObjectClassDirectedInteractions(
    ObjectClassHandle const& objectClass,
    InteractionClassHandleSet const& interactionClasses) {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
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
}

void UmbraRtiAmbassador::subscribeInteractionClassWithRegions(
    InteractionClassHandle const& interactionClass,
    RegionHandleSet const& regions,
    bool active) {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
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
}

void UmbraRtiAmbassador::unsubscribeInteractionClassWithRegions(
    InteractionClassHandle const& interactionClass,
    RegionHandleSet const& regions) {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
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
}

void UmbraRtiAmbassador::sendInteraction(
    InteractionClassHandle const& interactionClass,
    ParameterHandleValueMap const& parameterValues,
    VariableLengthData const& userSuppliedTag) {
  // Preserve the 2025 service's connection and membership preconditions ahead
  // of caller-supplied handle validation, matching the other public services.
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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

  std::optional<std::wstring> federationName;
  std::optional<std::uint64_t> producingFederateId;
  auto planCurrentRequest = [&]() {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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

  // Do not hold either sender lock while submitting a route: HLA_IMMEDIATE may
  // synchronously enter a different federate's Receive Interaction callback.
  for (auto& delivery : deliveries) {
    queueReceiveOrderInteraction(
        std::move(delivery.callbackRoute),
        *federationName,
        *producingFederateId,
        delivery.recipientId,
        *interactionClassHandle,
        sentParameters,
        copiedTag,
        transportationType);
  }
}

MessageRetractionHandle UmbraRtiAmbassador::sendInteraction(
    InteractionClassHandle const& interactionClass,
    ParameterHandleValueMap const& parameterValues,
    VariableLengthData const& userSuppliedTag,
    LogicalTime const& time) {
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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
  if (timeSnapshot.timeRegulating) {
    std::vector<std::uint64_t> tsoRecipientIds;
    for (auto const& recipient : plan.recipients) {
      if (timeConstrainedRecipients.contains(recipient.federateId)) {
        tsoRecipientIds.push_back(recipient.federateId);
      }
    }

    if (!tsoRecipientIds.empty()) {
      umbra::detail::TsoInteractionMessage message;
      message.producingFederateId = *producingFederateId;
      message.sentInteractionClassHandle = *interactionClassHandle;
      message.sentParameterHandles = *parameterHandles;
      message.parameters = sentParameters;
      message.userSuppliedTag = copiedTag;
      message.transportationName = plan.transportationName;
      std::shared_ptr<LogicalTime const> sharedTimestamp = timestamp;
      message.timestamp = std::move(sharedTimestamp);

      std::scoped_lock lock(federationManagementMutex());
      auto const result = embeddedFederationManagement().registry().enqueueTsoInteraction(
          *federationName,
          std::move(message),
          tsoRecipientIds);
      if (result.status != umbra::detail::FederationTsoRegistryStatus::applied ||
          result.queueStatus != umbra::detail::TsoMessageQueueStatus::applied ||
          result.messageId == 0) {
        throw RTIinternalError(
            L"The embedded federation could not queue the timestamped Send Interaction.");
      }
      messageId = result.messageId;
    } else if (!plan.recipients.empty()) {
      std::scoped_lock lock(federationManagementMutex());
      auto const result = embeddedFederationManagement().registry().allocateTsoMessageId(
          *federationName);
      if (result.status != umbra::detail::FederationTsoRegistryStatus::applied ||
          result.messageId == 0) {
        throw RTIinternalError(
            L"The embedded federation could not allocate a timestamped message designator.");
      }
      messageId = result.messageId;
    }
  }

  // The first public slice queues TSO for time-constrained recipients. A
  // non-time-constrained recipient still receives the timestamped callback
  // immediately as Receive Order, with the sender's TSO designator when one
  // exists. The callback rechecks subscriptions at its own user-code boundary.
  for (auto const& recipient : plan.recipients) {
    if (timeSnapshot.timeRegulating &&
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
        *producingFederateId,
        recipient.federateId,
        *interactionClassHandle,
        sentParameters,
        copiedTag,
        transportationType,
        std::shared_ptr<LogicalTime const>(timestamp),
        timeSnapshot.timeRegulating ? TIMESTAMP : RECEIVE,
        RECEIVE,
        timeSnapshot.timeRegulating && messageId != 0
            ? std::optional<std::uint64_t>(messageId)
            : std::nullopt);
  }

  if (!timeSnapshot.timeRegulating || messageId == 0) {
    return MessageRetractionHandle();
  }
  return makeMessageRetractionHandle(messageId);
}

void UmbraRtiAmbassador::sendDirectedInteraction(
    InteractionClassHandle const& interactionClass,
    ObjectInstanceHandle const& objectInstance,
    ParameterHandleValueMap const& parameterValues,
    VariableLengthData const& userSuppliedTag) {
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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
}

void UmbraRtiAmbassador::sendInteractionWithRegions(
    InteractionClassHandle const& interactionClass,
    ParameterHandleValueMap const& parameterValues,
    RegionHandleSet const& regions,
    VariableLengthData const& userSuppliedTag) {
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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

  for (auto& delivery : deliveries) {
    queueReceiveOrderInteraction(
        std::move(delivery.callbackRoute),
        *federationName,
        *producingFederateId,
        delivery.recipientId,
        *interactionClassValue,
        sentParameters,
        copiedTag,
        transportationType,
        std::optional<std::set<std::uint64_t>>(regionValues));
  }
}

TransportationTypeHandle UmbraRtiAmbassador::getTransportationTypeHandle(
    std::wstring const& transportationTypeName) {
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
}

std::wstring UmbraRtiAmbassador::getTransportationTypeName(
    TransportationTypeHandle const& transportationType) {
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
}

DimensionHandleSet UmbraRtiAmbassador::getAvailableDimensionsForObjectClass(
    ObjectClassHandle const& objectClass) {
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
}

DimensionHandleSet UmbraRtiAmbassador::getAvailableDimensionsForInteractionClass(
    InteractionClassHandle const& interactionClass) {
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
}

DimensionHandle UmbraRtiAmbassador::getDimensionHandle(std::wstring const& dimensionName) {
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
}

std::wstring UmbraRtiAmbassador::getDimensionName(DimensionHandle const& dimension) {
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
}

unsigned long UmbraRtiAmbassador::getDimensionUpperBound(DimensionHandle const& dimension) {
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
}

RegionHandle UmbraRtiAmbassador::createRegion(DimensionHandleSet const& dimensions) {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
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
}

void UmbraRtiAmbassador::commitRegionModifications(RegionHandleSet const& regions) {
  std::wstring federationName;
  std::vector<umbra::detail::ObjectInstanceScopeChangeRecipient> changes;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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
    federationName = *joinedFederationName_;
    changes = std::move(scopePlan.recipients);
  }
  queueObjectInstanceScopeChanges(std::move(changes), federationName);
}

void UmbraRtiAmbassador::deleteRegion(RegionHandle const& region) {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
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
}

DimensionHandleSet UmbraRtiAmbassador::getDimensionHandleSet(RegionHandle const& region) {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
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
}

RangeBounds UmbraRtiAmbassador::getRangeBounds(
    RegionHandle const& region,
    DimensionHandle const& dimension) {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
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
}

void UmbraRtiAmbassador::setRangeBounds(
    RegionHandle const& region,
    DimensionHandle const& dimension,
    RangeBounds const& rangeBounds) {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
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
}

bool UmbraRtiAmbassador::getAttributeScopeAdvisorySwitch() const {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
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
}

void UmbraRtiAmbassador::setAttributeScopeAdvisorySwitch(bool switchValue) {
  std::scoped_lock lock(mutex_, federationManagementMutex());
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Set Attribute Scope Advisory Switch requires membership in a federation execution.");
  }
  auto const status = embeddedFederationManagement().registry().setAttributeScopeAdvisorySwitch(
      *joinedFederationName_, *joinedFederateId_, switchValue);
  if (status == umbra::detail::FederationRegistryStatus::applied) {
    return;
  }
  if (status == umbra::detail::FederationRegistryStatus::federation_does_not_exist ||
      status == umbra::detail::FederationRegistryStatus::federate_not_member) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  throw RTIinternalError(L"Set Attribute Scope Advisory Switch encountered an unknown outcome.");
}

RegionHandle UmbraRtiAmbassador::decodeRegionHandle(
    VariableLengthData const& encodedValue) const {
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Decode Region Handle requires membership in a federation execution.");
  }
  return ::rti1516_2025::umbra_binding_detail::decodeRegionHandle(encodedValue);
}

void UmbraRtiAmbassador::enableTimeRegulation(LogicalTimeInterval const& lookahead) {
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  std::shared_ptr<CallbackSession> callbackSession;
  std::wstring federationName;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Enable Time Regulation requires a joined federate with initialized logical time.");
    }

    timeState = federateTimeState_;
    callbackSession = callbackSession_;
    federationName = *joinedFederationName_;
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
  umbra::detail::FederateTimeEnableResult result;
  {
    std::scoped_lock lock(federationManagementMutex());
    result = timeState->requestTimeRegulation(std::move(requestedLookahead));
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

  // This development profile has no TSO send/receive services or timestamped
  // queues. Its closed-world constraints therefore permit the callback at the
  // current logical time. A transport-bearing coordinator must calculate GALT
  // before this behavior can be reused outside that scope.
  callbacks_->submit([
      callbackSession = std::move(callbackSession),
      timeState = std::move(timeState),
      federationName = std::move(federationName),
      generation = result.generation] {
    std::vector<umbra::detail::FederationTimeGrantDispatch> newlyEligible;
    callbackSession->invoke([timeState, federationName, generation, &newlyEligible](FederateAmbassador& recipient) {
      std::shared_ptr<LogicalTime const> enabledTime;
      {
        std::scoped_lock lock(federationManagementMutex());
        enabledTime = timeState->grantTimeRegulation(generation);
        if (!enabledTime) {
          return;
        }
        auto scheduled = embeddedFederationManagement().registry().reevaluateTimeAdvanceGrants(
            federationName);
        if (scheduled.status == umbra::detail::FederationTimeGrantStatus::applied) {
          newlyEligible = std::move(scheduled.dispatches);
        }
      }
      recipient.timeRegulationEnabled(*enabledTime);
    });
    submitTimeAdvanceGrantDispatches(std::move(newlyEligible));
  });
}

void UmbraRtiAmbassador::disableTimeRegulation() {
  umbra::detail::FederateTimeDisableStatus result;
  std::vector<umbra::detail::FederationTimeGrantDispatch> newlyEligible;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Disable Time Regulation requires a joined federate with initialized logical time.");
    }
    result = federateTimeState_->disableTimeRegulation();
    if (result == umbra::detail::FederateTimeDisableStatus::applied) {
      auto scheduled = embeddedFederationManagement().registry().reevaluateTimeAdvanceGrants(
          *joinedFederationName_);
      if (scheduled.status == umbra::detail::FederationTimeGrantStatus::applied) {
        newlyEligible = std::move(scheduled.dispatches);
      }
    }
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
}

void UmbraRtiAmbassador::enableTimeConstrained() {
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  std::shared_ptr<CallbackSession> callbackSession;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Enable Time Constrained requires a joined federate with initialized logical time.");
    }

    timeState = federateTimeState_;
    callbackSession = callbackSession_;
    if (!callbackSession) {
      throw RTIinternalError(
          L"The embedded connection has no federate ambassador callback recipient.");
    }
  }

  auto result = timeState->requestTimeConstrained();
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
      generation = result.generation] {
    callbackSession->invoke([timeState, generation](FederateAmbassador& recipient) {
      auto enabledTime = timeState->grantTimeConstrained(generation);
      if (!enabledTime) {
        return;
      }
      recipient.timeConstrainedEnabled(*enabledTime);
    });
  });
}

void UmbraRtiAmbassador::disableTimeConstrained() {
  umbra::detail::FederateTimeDisableStatus result;
  std::vector<umbra::detail::FederationTimeGrantDispatch> newlyEligible;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Disable Time Constrained requires a joined federate with initialized logical time.");
    }
    result = federateTimeState_->disableTimeConstrained();
    if (result == umbra::detail::FederateTimeDisableStatus::applied) {
      auto scheduled = embeddedFederationManagement().registry().reevaluateTimeAdvanceGrants(
          *joinedFederationName_);
      if (scheduled.status == umbra::detail::FederationTimeGrantStatus::applied) {
        newlyEligible = std::move(scheduled.dispatches);
      }
    }
  }

  switch (result) {
    case umbra::detail::FederateTimeDisableStatus::applied:
      // A previously constrained TAR may no longer be GALT-bounded. Queue
      // any resulting grant only after the shared runtime locks are released.
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
}

void UmbraRtiAmbassador::timeAdvanceRequest(LogicalTime const& time) {
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  std::shared_ptr<CallbackSession> callbackSession;
  std::shared_ptr<umbra::detail::CallbackDispatcher> callbackDispatcher;
  std::wstring federationName;
  std::uint64_t federateId = 0;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Time Advance Request requires a joined federate with initialized logical time.");
    }

    timeState = federateTimeState_;
    callbackSession = callbackSession_;
    callbackDispatcher = callbacks_;
    federationName = *joinedFederationName_;
    federateId = *joinedFederateId_;
    if (!callbackSession || !callbackDispatcher) {
      throw RTIinternalError(
        L"The embedded connection has no federate ambassador callback recipient.");
    }
  }

  // LogicalTime is a caller-provided polymorphic object. Decode a reference
  // representation outside runtime locks, then commit only the private copy.
  auto requestedTime = cloneReferenceLogicalTime(timeState->implementationName(), time);
  umbra::detail::FederateTimeAdvanceResult result;
  umbra::detail::FederationTimeGrantDispatchResult scheduled;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_ ||
        *joinedFederationName_ != federationName || *joinedFederateId_ != federateId ||
        federateTimeState_ != timeState) {
      throw FederateNotExecutionMember(
          L"Time Advance Request requires an active joined federate with initialized logical time.");
    }

    result = timeState->requestAdvance(std::move(requestedTime));
    if (result.status == umbra::detail::FederateTimeAdvanceStatus::applied) {
      scheduled = embeddedFederationManagement().registry().requestTimeAdvanceGrant(
          federationName,
          federateId,
          result.generation,
          makeTimeAdvanceGrantDispatch(
              callbackDispatcher,
              callbackSession,
              timeState,
              federationName,
              federateId,
              result.generation));
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

  // The registry admits nonconstrained requests immediately and holds a
  // constrained TAR until strict GALT/NRG policy allows delivery. Every action
  // is submitted only after the calling thread has released its runtime locks.
  submitTimeAdvanceGrantDispatches(std::move(scheduled.dispatches));
}

void UmbraRtiAmbassador::queryLogicalTime(LogicalTime& time) {
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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
}

bool UmbraRtiAmbassador::queryGALT(LogicalTime& time) {
  std::uint64_t federateId = 0;
  umbra::detail::FederationTimeExecutionSnapshot snapshot;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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
}

bool UmbraRtiAmbassador::queryLITS(LogicalTime& time) {
  std::uint64_t federateId = 0;
  umbra::detail::FederationTimeExecutionSnapshot snapshot;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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
}

void UmbraRtiAmbassador::queryLookahead(LogicalTimeInterval& interval) {
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
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
}

void UmbraRtiAmbassador::retract(MessageRetractionHandle const& retraction) {
  auto const messageId = messageRetractionHandleValue(retraction);
  if (!messageId) {
    throw InvalidMessageRetractionHandle(
        L"Retract requires a valid MessageRetractionHandle returned by a timestamped service.");
  }

  std::wstring federationName;
  std::uint64_t producingFederateId = 0;
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  {
    std::scoped_lock lock(mutex_, federationManagementMutex());
    requireConnected(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Retract requires membership in a federation execution.");
    }
    federationName = *joinedFederationName_;
    producingFederateId = *joinedFederateId_;
    timeState = federateTimeState_;
  }
  if (!timeState->snapshot().timeRegulating) {
    throw TimeRegulationIsNotEnabled(
        L"Retract requires time regulation to be enabled for the joined federate.");
  }

  umbra::detail::FederationTsoRetractionResult result;
  {
    std::scoped_lock lock(federationManagementMutex());
    result = embeddedFederationManagement().registry().retractTsoMessageForProducer(
        federationName,
        producingFederateId,
        *messageId);
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
  switch (result.queueResult.status) {
    case umbra::detail::TsoMessageQueueStatus::applied:
      return;
    case umbra::detail::TsoMessageQueueStatus::message_already_retracted:
    case umbra::detail::TsoMessageQueueStatus::message_already_delivered:
    case umbra::detail::TsoMessageQueueStatus::message_not_found:
      throw MessageCanNoLongerBeRetracted(
          L"The timestamped message has already been delivered or is no longer pending.");
    case umbra::detail::TsoMessageQueueStatus::invalid_message_id:
    case umbra::detail::TsoMessageQueueStatus::invalid_recipient:
    case umbra::detail::TsoMessageQueueStatus::invalid_timestamp:
    case umbra::detail::TsoMessageQueueStatus::logical_time_implementation_mismatch:
      throw InvalidMessageRetractionHandle(
          L"The MessageRetractionHandle does not identify a valid pending message.");
  }
  throw RTIinternalError(L"Umbra encountered an unknown Retract outcome.");
}

MessageRetractionHandle UmbraRtiAmbassador::decodeMessageRetractionHandle(
    VariableLengthData const& encodedValue) const {
  requireConnected(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Decode Message Retraction Handle requires membership in a federation execution.");
  }
  return ::rti1516_2025::umbra_binding_detail::decodeMessageRetractionHandle(encodedValue);
}

#endif

}  // namespace rti1516_2025::umbra_binding_detail
