#pragma once

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)

#include "internal/federation/federation_management_coordinator.hpp"
#include "internal/runtime/umbra_rti_ambassador.hpp"

#include <string>
#endif

namespace rti1516_2025::umbra_binding_detail::service_failure_translation {

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
[[noreturn]] void throwInteractionClassDeclarationFailure(
    umbra::detail::InteractionClassDeclarationStatus status);

[[noreturn]] void throwDirectedInteractionDeclarationFailure(
    umbra::detail::DirectedInteractionDeclarationStatus status,
    std::wstring const &operation);

[[noreturn]] void throwRegionalInteractionClassDeclarationFailure(
    umbra::detail::RegionalInteractionClassDeclarationStatus status);

[[noreturn]] void throwObjectClassAttributeDeclarationFailure(
    umbra::detail::ObjectClassAttributeDeclarationStatus status);

[[noreturn]] void throwRegionServiceFailure(
    umbra::detail::RegionServiceStatus status,
    std::wstring const &operation);

[[noreturn]] void throwObjectInstanceRegistrationFailure(
    umbra::detail::ObjectInstanceRegistrationStatus status);

[[noreturn]] void throwObjectInstanceNameReservationFailure(
    umbra::detail::ObjectInstanceNameReservationStatus status,
    std::wstring const &operation);

[[noreturn]] void throwRegionalObjectClassAttributeDeclarationFailure(
    umbra::detail::RegionalObjectClassAttributeDeclarationStatus status);

[[noreturn]] void throwObjectInstanceRegionAssociationFailure(
    umbra::detail::ObjectInstanceRegionAssociationStatus status);

[[noreturn]] void throwObjectInstanceDeletionFailure(
    umbra::detail::ObjectInstanceDeletionStatus status);

[[noreturn]] void throwLocalObjectInstanceDeletionFailure(
    umbra::detail::LocalObjectInstanceDeletionStatus status);

[[noreturn]] void throwReceiveOrderInteractionFailure(
    umbra::detail::ReceiveOrderInteractionStatus status);

[[noreturn]] void throwReceiveOrderDirectedInteractionFailure(
    umbra::detail::ReceiveOrderDirectedInteractionStatus status);

[[noreturn]] void throwReceiveOrderAttributeUpdateFailure(
    umbra::detail::ReceiveOrderAttributeUpdateStatus status);

[[noreturn]] void throwAttributeValueUpdateRequestFailure(
    umbra::detail::AttributeValueUpdateRequestStatus status);

[[noreturn]] void throwJoinedFederateMomAttributeValueUpdateFailure(
    umbra::detail::JoinedFederateMomAttributeValueUpdateStatus status);

[[noreturn]] void throwAttributeValueUpdateClassRequestFailure(
    umbra::detail::AttributeValueUpdateClassRequestStatus status);

[[noreturn]] void throwAttributeOwnershipQueryFailure(
    umbra::detail::AttributeOwnershipQueryStatus status);

[[noreturn]] void throwAttributeOwnershipCheckFailure(
    umbra::detail::AttributeOwnershipCheckStatus status);

[[noreturn]] void throwAttributeOwnershipAcquisitionIfAvailableFailure(
    umbra::detail::AttributeOwnershipAcquisitionIfAvailableStatus status);

[[noreturn]] void throwAttributeOwnershipAcquisitionFailure(
    umbra::detail::AttributeOwnershipAcquisitionStatus status);

[[noreturn]] void throwAttributeOwnershipReleaseDeniedFailure(
    umbra::detail::AttributeOwnershipReleaseDeniedStatus status);

[[noreturn]] void throwAttributeOwnershipDivestitureIfWantedFailure(
    umbra::detail::AttributeOwnershipDivestitureIfWantedStatus status);

[[noreturn]] void throwUnconditionalAttributeOwnershipDivestitureFailure(
    umbra::detail::UnconditionalAttributeOwnershipDivestitureStatus status);

[[noreturn]] void throwNegotiatedAttributeOwnershipDivestitureFailure(
    umbra::detail::NegotiatedAttributeOwnershipDivestitureStatus status);

[[noreturn]] void throwConfirmDivestitureFailure(
    umbra::detail::ConfirmDivestitureStatus status);

[[noreturn]] void throwCancelNegotiatedAttributeOwnershipDivestitureFailure(
    umbra::detail::CancelNegotiatedAttributeOwnershipDivestitureStatus status);

[[noreturn]] void throwAttributeOwnershipAcquisitionCancellationFailure(
    umbra::detail::AttributeOwnershipAcquisitionCancellationStatus status);

[[noreturn]] void throwAttributeTransportationTypeChangeFailure(
    umbra::detail::AttributeTransportationTypeChangeStatus status);

[[noreturn]] void throwAttributeTransportationTypeDefaultFailure(
    umbra::detail::AttributeTransportationTypeDefaultStatus status);

[[noreturn]] void throwAttributeTransportationTypeQueryFailure(
    umbra::detail::AttributeTransportationTypeQueryStatus status);

[[noreturn]] void throwInteractionTransportationTypeChangeFailure(
    umbra::detail::InteractionTransportationTypeChangeStatus status);

[[noreturn]] void throwInteractionTransportationTypeQueryFailure(
    umbra::detail::InteractionTransportationTypeQueryStatus status);

[[noreturn]] void throwAttributeOrderTypeChangeFailure(
    umbra::detail::AttributeOrderTypeChangeStatus status);

[[noreturn]] void throwAttributeOrderTypeDefaultFailure(
    umbra::detail::AttributeOrderTypeDefaultStatus status);

[[noreturn]] void throwInteractionOrderTypeChangeFailure(
    umbra::detail::InteractionOrderTypeChangeStatus status);

[[noreturn]] void throwPreparationFailure(
    umbra::detail::FederationPreparationResult const &preparation);

#endif

}  // namespace rti1516_2025::umbra_binding_detail::service_failure_translation
