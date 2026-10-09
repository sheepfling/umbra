#include "internal/runtime/umbra_rti_ambassador_service_failure_translation.hpp"

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)

#include <RTI/FederateAmbassador.h>
#include <RTI/RTIambassador.h>

namespace rti1516_2025::umbra_binding_detail::service_failure_translation {

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
    std::wstring const &operation) {
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
    std::wstring const &operation) {
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
    std::wstring const &operation) {
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
    case Status::attribute_owned_by_rti:
      throw RTIinternalError(
          L"Ownership management cannot modify an attribute owned by the RTI.");
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
    case Status::attribute_owned_by_rti:
      throw RTIinternalError(
          L"Ownership management cannot modify an attribute owned by the RTI.");
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
    case Status::attribute_owned_by_rti:
      throw RTIinternalError(
          L"Ownership management cannot modify an attribute owned by the RTI.");
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
    case Status::attribute_owned_by_rti:
      throw RTIinternalError(
          L"Ownership management cannot modify an attribute owned by the RTI.");
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
    case Status::attribute_owned_by_rti:
      throw RTIinternalError(
          L"Ownership management cannot modify an attribute owned by the RTI.");
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
    case Status::attribute_owned_by_rti:
      throw RTIinternalError(
          L"Ownership management cannot modify an attribute owned by the RTI.");
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
    case Status::attribute_owned_by_rti:
      throw RTIinternalError(
          L"Ownership management cannot modify an attribute owned by the RTI.");
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
          L"Confirm Divestiture requires a still-pending acquisition candidate.");
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
    case Status::attribute_owned_by_rti:
      throw RTIinternalError(
          L"Ownership management cannot modify an attribute owned by the RTI.");
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
    case Status::attribute_owned_by_rti:
      throw RTIinternalError(
          L"Ownership management cannot modify an attribute owned by the RTI.");
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
          L"The supplied TransportationTypeHandle is not declared in this federation execution.");
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
          L"The supplied TransportationTypeHandle is not declared in this federation execution.");
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
          L"The supplied TransportationTypeHandle is not declared in this federation execution.");
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
    umbra::detail::FederationPreparationResult const &preparation) {
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
          L"The supplied FOM/MIM modules or documented time representation cannot form one model: " +
          std::wstring(preparation.diagnostics.begin(), preparation.diagnostics.end()));
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

}  // namespace rti1516_2025::umbra_binding_detail::service_failure_translation

#endif
