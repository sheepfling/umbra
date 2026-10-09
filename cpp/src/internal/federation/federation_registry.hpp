#pragma once

#include "internal/handles/attribute_handle_directory.hpp"
#include "internal/handles/dimension_handle_directory.hpp"
#include "internal/time/federation_time_coordinator.hpp"
#include "internal/fom/fom_validation.hpp"
#include "internal/handles/interaction_class_handle_directory.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#include "internal/federation/federation_save_commit_store.hpp"
#include "internal/federation/federation_state_image.hpp"
#include "internal/handles/object_class_handle_directory.hpp"
#include "internal/handles/parameter_handle_directory.hpp"
#include "internal/handles/transportation_type_handle_directory.hpp"
#include "internal/observability/runtime_instrumentation.hpp"

#include <RTI/Enums.h>
#include <RTI/Typedefs.h>
#include <RTI/VariableLengthData.h>
#include <RTI/time/LogicalTime.h>

#include <cstddef>
#include <cstdint>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#include "internal/federation/federation_registry_types.hpp"

namespace umbra::detail {

class EmbeddedFederationRegistry final {
 public:
  explicit EmbeddedFederationRegistry(
      std::shared_ptr<RuntimeInstrumentation> instrumentation = {},
      std::shared_ptr<FederationSaveCommitStore> saveCommitStore = {});

  [[nodiscard]] RuntimeInstrumentationSnapshot
  runtimeInstrumentationSnapshotForTesting() const;

  FederationRegistryResult create(
      std::wstring const& federationName,
      FederationDefinition definition);

  FederationRegistryResult destroy(std::wstring const& federationName);

  FederationJoinResult join(
      std::wstring const& federationName,
      std::wstring const& federateType,
      std::optional<std::wstring> requestedFederateName = std::nullopt,
      InteractionCallbackRoute interactionCallbackRoute = {});

  // Runtime-backed joins register their FederateTimeState in the same private
  // membership transaction. The legacy join overload remains useful to unit
  // test the registry independently; it still registers the member as a TSO
  // recipient, but (without a time state) it is intentionally absent from the
  // coordinator's GALT/LITS input set.
  FederationJoinResult joinWithTimeState(
      std::wstring const& federationName,
      std::shared_ptr<FederateTimeState> timeState,
      std::wstring const& federateType,
      std::optional<std::wstring> requestedFederateName = std::nullopt,
      InteractionCallbackRoute interactionCallbackRoute = {},
      FederationTimeGrantDispatchFactory timeAdvanceGrantDispatchFactory = {},
      FederationTimeRoleEnableDispatchFactory timeRoleEnableDispatchFactory = {});

  // Commits a freshly prevalidated replacement definition and a membership as
  // one state transition. This is used only after an external coordinator has
  // proved additional FOM modules compatible with the current definition.
  FederationJoinResult joinWithDefinition(
      std::wstring const& federationName,
      FederationDefinition definition,
      std::wstring const& federateType,
      std::optional<std::wstring> requestedFederateName = std::nullopt);

  FederationJoinResult joinWithDefinitionAndTimeState(
      std::wstring const& federationName,
      FederationDefinition definition,
      std::shared_ptr<FederateTimeState> timeState,
      std::wstring const& federateType,
      std::optional<std::wstring> requestedFederateName = std::nullopt,
      InteractionCallbackRoute interactionCallbackRoute = {},
      FederationTimeGrantDispatchFactory timeAdvanceGrantDispatchFactory = {},
      FederationTimeRoleEnableDispatchFactory timeRoleEnableDispatchFactory = {});

  FederationRegistryResult resign(
      std::wstring const& federationName,
      std::uint64_t federateId,
      rti1516_2025::ResignAction resignAction = rti1516_2025::NO_ACTION);

  // This is the one explicit lifecycle exception to the ordinary
  // post-service report reservation path: Resign Federation Execution erases
  // the joined member before the ambassador can append its report.  The
  // known MIM service-group value is supplied by the adapter, and the returned
  // serial is present only for a selected report-to-file route.
  FederationRegistryResult resignWithFinalServiceReportReservation(
      std::wstring const& federationName,
      std::uint64_t federateId,
      rti1516_2025::ResignAction resignAction,
      std::uint16_t serviceGroup,
      bool reservePublicInteraction = false);

  // A joined ambassador attaches this private endpoint only after its
  // immutable report file has been created. The registry preserves it as a
  // live callback-side resource and removes it with the joined membership.
  [[nodiscard]] FederationRegistryStatus setServiceReportRoute(
      std::wstring const& federationName,
      std::uint64_t federateId,
      FederateServiceReportRoute serviceReportRoute);

  [[nodiscard]] FederationRegistryStatus setPublicServiceReportRoute(
      std::wstring const& federationName,
      std::uint64_t federateId,
      FederatePublicServiceReportRoute publicServiceReportRoute);

  // Establishes the registry-owned, as-yet-unpublished MIM object after the
  // real report-file writer has chosen its immutable location. This is kept
  // separate from joinImpl because a filesystem failure must roll membership
  // back before a successful Join becomes externally visible.
  [[nodiscard]] JoinedFederateMomObjectStatus establishJoinedFederateMomObject(
      std::wstring const& federationName,
      std::uint64_t federateId,
      JoinedFederateMomObjectDescriptor const& descriptor);

  // Establishes the single RTI-owned HLAmanager.HLAfederation object for an
  // execution.  This bounded foundation supplies only the MIM's static
  // federation-wide attributes; lifecycle-backed conditional values are
  // projected by their service-family update paths rather than as stale
  // initial values.
  [[nodiscard]] JoinedFederateMomObjectStatus establishFederationMomObject(
      std::wstring const& federationName,
      std::wstring const& rtiVersion,
      std::wstring const& mimDesignator);

  [[nodiscard]] std::optional<JoinedFederateMomObjectSnapshot>
  joinedFederateMomObjectFor(
      std::wstring const& federationName,
      std::uint64_t federateId) const;

  [[nodiscard]] std::optional<JoinedFederateMomObjectSnapshot>
  federationMomObjectFor(std::wstring const& federationName) const;

  // A transport fault applies the member's current Automatic Resign
  // Directive while forcing the membership transition even when a normal
  // caller-initiated resign would reject unresolved ownership work. The
  // returned callback records are for surviving federates; the lost
  // federate's own connectionLost callback is owned by the adapter session.
  FederationRegistryResult connectionLost(
      std::wstring const& federationName,
      std::uint64_t federateId);

  // Connection Lost is an RTI-invoked service. Like resignation, it erases
  // the member before the adapter can otherwise append the selected-file
  // service report. Reserve its final serial only for a report-to-file route,
  // after the forced-resignation processing has accepted the loss.
  FederationRegistryResult connectionLostWithFinalServiceReportReservation(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint16_t serviceGroup);

  [[nodiscard]] FederationSaveControlResult requestFederationSave(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::wstring label);

  [[nodiscard]] FederationSaveControlResult requestFederationSave(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::wstring label,
      std::shared_ptr<rti1516_2025::LogicalTime const> timestamp);

  // Admits one pending untimed save at a Time Advance Grant boundary. The
  // adapter calls this after the grant has passed federation scheduling but
  // before it mutates the recipient's FederateTimeState to Time Granted, so
  // the returned direct callback observes the IEEE 1516.1-2025-required Time
  // Advancing state. This is intentionally separate from the timed-save
  // scheduler, whose timestamp/TSO admission remains a different slice.
  [[nodiscard]] FederationSaveAdmission
  admitImmediateFederationSaveAtTimeAdvanceBoundary(
      std::wstring const& federationName,
      std::uint64_t federateId);

  // Admits one pending timestamped save after the current recipient has
  // received its required TSO payloads but before the matching ordinary Time
  // Advance Grant mutates its private state. The registry accepts TAR/NMR at
  // an inclusive save boundary and TARA/NMRA at an exclusive one. FQR uses
  // the separately calculated actual grant supplied through the dedicated
  // admission entry point below.
  [[nodiscard]] FederationSaveAdmission
  admitTimedFederationSaveAtTimeAdvanceBoundary(
      std::wstring const& federationName,
      std::uint64_t federateId);

  // Flush Queue Request calculates its actual grant at callback time. The
  // adapter supplies that calculated grant after draining the recipient's TSO
  // queue but before mutating FederateTimeState, so a qualifying strict save
  // boundary can still deliver Initiate Federate Save in Time Advancing.
  [[nodiscard]] FederationSaveAdmission
  admitTimedFederationSaveAtFlushQueueGrantBoundary(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::shared_ptr<rti1516_2025::LogicalTime const> flushQueueGrantedTime);

  // Re-evaluates a pending timestamped save after a time/TSO boundary.  The
  // result contains Initiate Federate Save callbacks only when every current
  // time-constrained member has crossed the requested timestamp and has no
  // undelivered TSO message at or below that boundary.
  [[nodiscard]] FederationSaveControlResult reevaluateTimedFederationSave(
      std::wstring const& federationName);

  [[nodiscard]] FederationSaveControlResult federateSaveBegun(
      std::wstring const& federationName,
      std::uint64_t federateId);

  [[nodiscard]] FederationSaveControlResult federateSaveComplete(
      std::wstring const& federationName,
      std::uint64_t federateId);

  [[nodiscard]] FederationSaveControlResult federateSaveNotComplete(
      std::wstring const& federationName,
      std::uint64_t federateId);

  [[nodiscard]] FederationSaveControlResult abortFederationSave(
      std::wstring const& federationName,
      std::uint64_t federateId);

  [[nodiscard]] FederationSaveControlResult queryFederationSaveStatus(
      std::wstring const& federationName,
      std::uint64_t federateId);

  [[nodiscard]] FederationServiceOperationStatus serviceOperationStatus(
      std::wstring const& federationName,
      std::uint64_t federateId) const;

  // Records the accepted boundary of one Update Attribute Values invocation
  // for one object instance. The public adapter calls this only after all
  // synchronous validation and timestamped queue admission have succeeded;
  // the invocation counter and distinct-object set are therefore RTI-owned
  // sources for HLAupdatesSent and HLAobjectInstancesUpdated rather than
  // callback- or payload-derived estimates.
  [[nodiscard]] FederationRegistryStatus
  recordSuccessfulUpdateAttributeValues(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t registeredObjectClassHandle,
      std::set<std::string> const& transportationNames,
      std::vector<std::pair<
          std::uint64_t,
          rti1516_2025::VariableLengthData>> const* acceptedAttributeValues = nullptr);

  // Records one accepted application-object Reflect Attribute Values callback
  // at the receiving federate's callback boundary. The retained handle set is
  // the RTI-owned source for HLAobjectInstancesReflected.
  [[nodiscard]] FederationRegistryStatus
  recordSuccessfulObjectInstanceReflection(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectInstanceHandle);

  // Records one accepted Send Interaction invocation. The public adapter
  // calls this only after synchronous validation and any timestamped queue
  // admission have succeeded, so MOM values are service counts rather than
  // callback or recipient counts.
  [[nodiscard]] FederationRegistryStatus
  recordSuccessfulInteractionSend(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t sentInteractionClassHandle,
      std::string const& transportationName,
      bool directed);

  [[nodiscard]] FederationRegistryStatus
  recordSuccessfulInteractionReceipt(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t receivedInteractionClassHandle,
      std::string const& transportationName,
      bool directed);

  [[nodiscard]] FederationRegistryStatus
  recordSuccessfulReflectionReceipt(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectInstanceHandle,
      std::string const& transportationName);

  [[nodiscard]] FederationRestoreControlResult requestFederationRestore(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::wstring label);

  [[nodiscard]] FederationRestoreControlResult federateRestoreComplete(
      std::wstring const& federationName,
      std::uint64_t federateId);

  [[nodiscard]] FederationRestoreControlResult federateRestoreNotComplete(
      std::wstring const& federationName,
      std::uint64_t federateId);

  [[nodiscard]] FederationRestoreControlResult abortFederationRestore(
      std::wstring const& federationName,
      std::uint64_t federateId);

  [[nodiscard]] FederationRestoreControlResult queryFederationRestoreStatus(
      std::wstring const& federationName,
      std::uint64_t federateId);

  // Federation synchronization points are retained as one-shot federation
  // state.  Registration and achievement return callback plans rather than
  // invoking user code while the registry is locked.
  [[nodiscard]] SynchronizationPointRegistrationPlan registerSynchronizationPoint(
      std::wstring const& federationName,
      std::uint64_t registeringFederateId,
      std::wstring label,
      std::vector<unsigned char> userSuppliedTag,
      std::set<std::uint64_t> const& synchronizationSet,
      bool synchronizationSetWasSupplied);

  [[nodiscard]] SynchronizationPointAnnouncementPlan
  announcePendingSynchronizationPoints(
      std::wstring const& federationName,
      std::uint64_t newlyJoinedFederateId);

  [[nodiscard]] SynchronizationPointAchievedPlan achieveSynchronizationPoint(
      std::wstring const& federationName,
      std::uint64_t achievingFederateId,
      std::wstring const& label,
      bool successfully);

  [[nodiscard]] bool contains(std::wstring const& federationName) const;
  [[nodiscard]] std::size_t memberCount(std::wstring const& federationName) const;
  [[nodiscard]] std::optional<FederationDefinition> definitionFor(
      std::wstring const& federationName) const;
  [[nodiscard]] std::optional<FederationTimeExecutionSnapshot> timeSnapshotFor(
      std::wstring const& federationName) const;

  // Live per-federate state used by the adapter's receive-order callback gate.
  // It is returned as a shared pointer so the registry lock can be released
  // before temporal state or callback-session code runs.
  [[nodiscard]] std::shared_ptr<FederateTimeState> timeStateFor(
      std::wstring const& federationName,
      std::uint64_t federateId) const;

  [[nodiscard]] std::optional<bool> attributeScopeAdvisorySwitchFor(
      std::wstring const& federationName,
      std::uint64_t federateId) const;

  [[nodiscard]] FederationRegistryStatus setAttributeScopeAdvisorySwitch(
      std::wstring const& federationName,
      std::uint64_t federateId,
      bool switchValue);

  [[nodiscard]] std::optional<bool> objectClassRelevanceAdvisorySwitchFor(
      std::wstring const& federationName,
      std::uint64_t federateId) const;

  [[nodiscard]] FederationRegistryStatus setObjectClassRelevanceAdvisorySwitch(
      std::wstring const& federationName,
      std::uint64_t federateId,
      bool switchValue);

  [[nodiscard]] std::optional<bool> attributeRelevanceAdvisorySwitchFor(
      std::wstring const& federationName,
      std::uint64_t federateId) const;

  [[nodiscard]] FederationRegistryStatus setAttributeRelevanceAdvisorySwitch(
      std::wstring const& federationName,
      std::uint64_t federateId,
      bool switchValue);

  [[nodiscard]] std::optional<bool> interactionRelevanceAdvisorySwitchFor(
      std::wstring const& federationName,
      std::uint64_t federateId) const;

  [[nodiscard]] std::optional<bool> advisoriesUseKnownClassSwitchFor(
      std::wstring const& federationName,
      std::uint64_t federateId) const;

  [[nodiscard]] std::optional<bool> autoProvideSwitchFor(
      std::wstring const& federationName,
      std::uint64_t federateId) const;

  // HLAmanager.HLAfederation.HLAadjust.HLAsetSwitches changes the
  // federation-wide Auto Provide value for all current members.  The public
  // MOM interaction path owns the payload validation; the registry only
  // applies the already-decoded value as one locked federation mutation.
  [[nodiscard]] FederationRegistryStatus setAutoProvideSwitch(
      std::wstring const& federationName,
      std::uint64_t federateId,
      bool switchValue);

  [[nodiscard]] std::optional<bool> nonRegulatedGrantSwitchFor(
      std::wstring const& federationName,
      std::uint64_t federateId) const;

  [[nodiscard]] std::optional<bool> conveyRegionDesignatorSetsSwitchFor(
      std::wstring const& federationName,
      std::uint64_t federateId) const;

  [[nodiscard]] FederationRegistryStatus setConveyRegionDesignatorSetsSwitch(
      std::wstring const& federationName,
      std::uint64_t federateId,
      bool switchValue);

  [[nodiscard]] std::optional<rti1516_2025::ResignAction> automaticResignActionFor(
      std::wstring const& federationName,
      std::uint64_t federateId) const;

  [[nodiscard]] FederationRegistryStatus setAutomaticResignAction(
      std::wstring const& federationName,
      std::uint64_t federateId,
      rti1516_2025::ResignAction resignAction);

  [[nodiscard]] std::optional<bool> serviceReportingSwitchFor(
      std::wstring const& federationName,
      std::uint64_t federateId) const;

  [[nodiscard]] ServiceReportingSwitchStatus setServiceReportingSwitch(
      std::wstring const& federationName,
      std::uint64_t federateId,
      bool switchValue);

  // Applies the standard HLAmanager.HLAfederate.HLAadjust.HLAsetTiming
  // parameter pair after the public adapter has decoded HLAfederateReference
  // and HLAseconds.  The requesting member may target any current joined
  // federate; a zero period disables automatic MOM updates.
  [[nodiscard]] FederateMOMTimingUpdateStatus setFederateMomReportPeriod(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t targetFederateId,
      std::int32_t reportPeriodSeconds);

  // Applies HLAmodifyAttributeState after the adapter has decoded the
  // inherited target federate, object/attribute handles, and HLAownership
  // enumerator.  Owned/Unowned changes are synchronous and callback-free;
  // RTI-owned MOM attributes are never accepted by this path.
  [[nodiscard]] FederateMOMAttributeStateUpdateStatus
  setFederateMomAttributeState(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t targetFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t attributeHandle,
      bool owned);

  // Claims each target whose HLAsetTiming deadline has elapsed once.  The
  // returned object/attribute pairs are immutable routing inputs; callback
  // eligibility is rechecked later by the ordinary MOM planner.
  [[nodiscard]] std::vector<JoinedFederateMomPeriodicUpdate>
  takeDueJoinedFederateMomPeriodicUpdates(
      std::wstring const& federationName,
      std::chrono::steady_clock::time_point now);

  // Applies the standard joined-federate HLAsetSwitches parameter subset as
  // one registry mutation.  This is intentionally private-runtime state: the
  // official public API exposes the individual switch services separately.
  [[nodiscard]] FederateMOMSwitchUpdateStatus applyFederateMOMSwitchUpdate(
      std::wstring const& federationName,
      std::uint64_t federateId,
      FederateMOMSwitchUpdate const& update);

  [[nodiscard]] std::optional<bool> exceptionReportingSwitchFor(
      std::wstring const& federationName,
      std::uint64_t federateId) const;

  [[nodiscard]] FederationRegistryStatus setExceptionReportingSwitch(
      std::wstring const& federationName,
      std::uint64_t federateId,
      bool switchValue);

  [[nodiscard]] std::optional<bool> sendServiceReportsToFileSwitchFor(
      std::wstring const& federationName,
      std::uint64_t federateId) const;

  [[nodiscard]] FederationRegistryStatus setSendServiceReportsToFileSwitch(
      std::wstring const& federationName,
      std::uint64_t federateId,
      bool switchValue);

  [[nodiscard]] std::optional<bool> delaySubscriptionEvaluationSwitchFor(
      std::wstring const& federationName,
      std::uint64_t federateId) const;

  [[nodiscard]] std::optional<bool> allowRelaxedDDMSwitchFor(
      std::wstring const& federationName,
      std::uint64_t federateId) const;

  [[nodiscard]] FederationRegistryStatus setInteractionRelevanceAdvisorySwitch(
      std::wstring const& federationName,
      std::uint64_t federateId,
      bool switchValue);

  // Re-evaluates a scheduled Turn Updates On/Off advisory immediately before
  // callback entry. A zero receivingFederateId evaluates the federation-wide
  // relevance edge; a nonzero value retains the older receiver-local query
  // seam. This prevents stale queued advisories after a second
  // subscription/region/ownership transition or after the owner disables the
  // Attribute Relevance Advisory Switch.
  [[nodiscard]] std::set<std::uint64_t> attributeRelevanceAdvisoryAttributes(
      std::wstring const& federationName,
      std::uint64_t providingFederateId,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& attributeHandles,
      bool expectedInScope) const;

  // Re-resolves the currently retained maximum active update-rate designator
  // for a queued Turn Updates On callback. A missing value means the official
  // no-rate overload must be used; an explicit HLAdefault remains distinct
  // and therefore selects the rate-bearing overload.
  [[nodiscard]] std::optional<std::string>
  attributeRelevanceAdvisoryUpdateRateDesignatorFor(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t attributeHandle) const;

  // Register a callback delivery for an already-accepted private advance using
  // the joined federate's live dispatch factory. Each production callback is
  // given a monotonically unique identity, so callback work queued before a
  // restore cannot be mistaken for a later request that reuses a saved time
  // generation.
  [[nodiscard]] FederationTimeGrantDispatchResult requestTimeAdvanceGrant(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t generation);

  // Low-level injected-dispatch seam used by registry-focused tests. Normal
  // embedded runtime calls must use the factory-backed overload above.
  [[nodiscard]] FederationTimeGrantDispatchResult requestTimeAdvanceGrant(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t generation,
      FederationTimeGrantDispatch dispatch);

  // Re-evaluate all pending TARs after a time-state change. Callers execute
  // the returned actions only after releasing all runtime locks.
  [[nodiscard]] FederationTimeGrantDispatchResult reevaluateTimeAdvanceGrants(
      std::wstring const& federationName);

  // Re-checks eligibility immediately before the queued callback changes the
  // federate's logical time. A stale or newly blocked callback becomes a
  // harmless no-op and is eligible for a future re-evaluation.
  [[nodiscard]] FederationTimeGrantStatus beginTimeAdvanceGrant(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t generation,
      std::uint64_t dispatchIdentity = 0);

  // Private temporal-coordinator hooks used by the bounded public timestamped
  // interaction, attribute-update, and object-deletion/removal slices. They
  // never invoke a FederateAmbassador; callers re-evaluate pending TAR
  // dispatches after each operation and own the official callback translation.
  [[nodiscard]] FederationTsoMessageIdResult allocateTsoMessageId(
      std::wstring const& federationName);

  [[nodiscard]] FederationTsoEnqueueResult enqueueTsoMessage(
      std::wstring const& federationName,
      std::uint64_t messageId,
      std::uint64_t recipientFederateId,
      std::shared_ptr<rti1516_2025::LogicalTime const> timestamp);

  // Read-only recipient-scoped view used by Next Message Request to select
  // the earliest currently queued TSO timestamp at or below the caller's
  // requested logical time.
  [[nodiscard]] std::optional<std::shared_ptr<rti1516_2025::LogicalTime const>>
  earliestTsoTimestampFor(
      std::wstring const& federationName,
      std::uint64_t recipientFederateId) const;

  // Atomically allocates one message-retraction designator, retains the
  // interaction payload while any recipient needs it, and records every
  // recipient of a timestamped Send Interaction.  The recipient set may be
  // empty: the public service still returns its designator, backed by a
  // lightweight ledger record. Only queuedRecipientFederateIds enter the
  // temporal queue; allTimestampedRecipientFederateIds also include
  // immediate/nonconstrained recipients so Retract can later distinguish
  // delivery from suppression.
  [[nodiscard]] FederationTsoInteractionEnqueueResult enqueueTsoInteraction(
      std::wstring const& federationName,
      TsoInteractionMessage message,
      std::vector<std::uint64_t> const& queuedRecipientFederateIds,
      std::vector<std::uint64_t> const& allTimestampedRecipientFederateIds);

  // Atomically allocates one message-retraction designator, retains the
  // attribute-update payload, and records every recipient of a timestamped
  // Update Attribute Values invocation. The recipient set may be empty: the
  // public service still returns its designator, backed by a lightweight
  // ledger record. Only queuedRecipientFederateIds enter the temporal queue;
  // allTimestampedRecipientFederateIds also include immediate/nonconstrained
  // recipients so Retract can later distinguish delivery from suppression.
  // The payload keeps recipient-specific passels so a changed subscription
  // can only suppress, never invent, a reflection at delivery.
  [[nodiscard]] FederationTsoAttributeUpdateEnqueueResult
  enqueueTsoAttributeUpdate(
      std::wstring const& federationName,
      TsoAttributeUpdateMessage message,
      std::vector<std::uint64_t> const& queuedRecipientFederateIds,
      std::vector<std::uint64_t> const& allTimestampedRecipientFederateIds);

  // Atomically allocates one message-retraction designator, retains a
  // timestamped object deletion payload, and reserves the known recipients'
  // pending-delete boundary.  The public adapter supplies only the selected
  // time-constrained recipient ids for queueing; non-constrained recipients
  // use the same accepted payload for immediate timestamped callbacks.
  [[nodiscard]] FederationTsoObjectDeletionEnqueueResult
  enqueueTsoObjectDeletion(
      std::wstring const& federationName,
      std::uint64_t producingFederateId,
      std::uint64_t objectInstanceHandle,
      TsoObjectDeletionMessage message,
      std::vector<std::uint64_t> const& recipientFederateIds);

  // Atomically allocates one message-retraction designator, retains the
  // directed-interaction payload, and records every recipient captured in that
  // payload. Only queuedRecipientFederateIds enter the temporal queue; the
  // retained recipient set also covers immediate/nonconstrained recipients so
  // Retract can distinguish delivery from suppression.
  [[nodiscard]] FederationTsoDirectedInteractionEnqueueResult
  enqueueTsoDirectedInteraction(
      std::wstring const& federationName,
      TsoDirectedInteractionMessage message,
      std::vector<std::uint64_t> const& queuedRecipientFederateIds);

  // Read-only validation and recipient snapshot for the timestamped delete
  // acceptance boundary.  The enqueue method repeats every check while the
  // federation-management mutex is held by the adapter.
  [[nodiscard]] ObjectInstanceDeletionPlan planTsoObjectInstanceDeletion(
      std::wstring const& federationName,
      std::uint64_t producingFederateId,
      std::uint64_t objectInstanceHandle) const;

  [[nodiscard]] FederationTsoRetractionResult retractTsoMessage(
      std::wstring const& federationName,
      std::uint64_t messageId);

  [[nodiscard]] FederationTsoRetractionResult retractTsoInteraction(
      std::wstring const& federationName,
      std::uint64_t producingFederateId,
      std::uint64_t messageId);

  [[nodiscard]] FederationTsoRetractionResult retractTsoMessageForProducer(
      std::wstring const& federationName,
      std::uint64_t producingFederateId,
      std::uint64_t messageId,
      std::shared_ptr<rti1516_2025::LogicalTime const> const& retractionLowerBound,
      bool enforceTimestampEligibility = true);

  // Retires heavyweight state for message designators whose producing
  // federate can no longer legally retract them at the supplied strict
  // Clause 8.22.3 boundary.  A lightweight producer-owned tombstone remains
  // so a later Retract is classified as MessageCanNoLongerBeRetracted rather
  // than as an invalid handle.  Pending deliveries keep their typed payload
  // until their own callback boundary has resolved.
  [[nodiscard]] std::size_t retireTsoMessagePayloadsAtOrBefore(
      std::wstring const& federationName,
      std::uint64_t producingFederateId,
      rti1516_2025::LogicalTime const& retractionLowerBound);

  // Claims the normal or directed timestamped interaction delivery boundary
  // immediately before the public callback enters user code. A concurrent
  // legal Retract can turn a still-pending recipient into retracted first,
  // causing this method to suppress the stale original callback.
  [[nodiscard]] bool beginTsoInteractionCallback(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::uint64_t messageId);

  // Claims one timestamped Reflect Attribute Values callback boundary. A
  // timestamped Update Attribute Values invocation may yield multiple
  // recipient passels under one retraction designator, so a recipient that
  // already began an earlier passel is allowed to continue unless a concurrent
  // legal Retract has changed it to retracted.
  [[nodiscard]] bool beginTsoAttributeUpdateCallback(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::uint64_t messageId);

  // Rechecks the recipient's active membership immediately before the
  // binding invokes Request Retraction on a route captured by a legal
  // Retract. The retraction record remains private and is never exposed as a
  // public service state object.
  [[nodiscard]] bool canDeliverTsoRequestRetraction(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::uint64_t messageId) const;

  [[nodiscard]] FederationTsoDeliveryRegistryResult beginTsoDelivery(
      std::wstring const& federationName,
      std::uint64_t recipientFederateId,
      rti1516_2025::LogicalTime const& boundary,
      bool inclusive);

  [[nodiscard]] FederationTsoInteractionDeliveryRegistryResult
  beginTsoInteractionDelivery(
      std::wstring const& federationName,
      std::uint64_t recipientFederateId,
      rti1516_2025::LogicalTime const& boundary,
      bool inclusive);

  // Begins one recipient boundary for all currently supported typed payloads
  // while preserving the queue's timestamp/sequence order across families.
  [[nodiscard]] FederationTsoPayloadDeliveryRegistryResult
  beginTsoPayloadDelivery(
      std::wstring const& federationName,
      std::uint64_t recipientFederateId,
      rti1516_2025::LogicalTime const& boundary,
      bool inclusive);

  // Begins one timestamped removal callback.  The execution-owned deletion
  // record remains reconstitutable after delivery as well: a legal Retract
  // either clears the pending marker before this boundary or restores the
  // invocation snapshot before Request Retraction reaches a delivered
  // recipient.
  [[nodiscard]] std::optional<RemovedObjectInstanceSnapshot>
  beginTsoObjectInstanceRemoval(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t messageId);

  [[nodiscard]] FederationTsoDeliveryRegistryResult completeTsoDelivery(
      std::wstring const& federationName,
      TsoQueuedMessage const& message);

  // Process-boundary acknowledgements carry only the execution-owned message
  // id and recipient identity.  Resolve the private queue sequence inside the
  // registry rather than exposing it in the transport protocol.
  [[nodiscard]] FederationTsoDeliveryRegistryResult completeTsoDeliveryFor(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::uint64_t messageId);

  // Completes a timestamped recipient boundary when its current declaration
  // suppresses every user callback. This is distinct from delivery and
  // retraction: no Request Retraction callback may be induced for a
  // suppressed recipient, but the original pending state must not retain
  // object-dependent connection-loss cleanup forever.
  [[nodiscard]] bool finishTsoRecipientCallbackSuppressed(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::uint64_t messageId);

  // Replans only forced connection-loss automatic-resign removals whose
  // queued callback reached the protected TSO boundary first. The returned
  // receive-order callbacks are submitted after the matching grant callback,
  // preserving both the required timestamped delivery and the ordinary
  // asynchronous-delivery gate.
  [[nodiscard]] std::vector<ObjectInstanceRemovalRecipient>
  releaseConnectionLossDeferredObjectInstanceRemovals(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId);

  [[nodiscard]] std::vector<FederationExecutionSummary> federationExecutions() const;
  [[nodiscard]] std::optional<std::vector<FederateMembership>> membersFor(
      std::wstring const& federationName) const;
  [[nodiscard]] std::optional<FederateMembership> memberByName(
      std::wstring const& federationName,
      std::wstring const& federateName) const;
  [[nodiscard]] std::optional<FederateMembership> memberById(
      std::wstring const& federationName,
      std::uint64_t federateId) const;
  // A returned FederateHandle remains a valid federate designator for the
  // execution after that federate leaves it. Keep this identity lookup
  // distinct from memberById(), which intentionally answers only whether a
  // federate is currently joined and is used as a callback-delivery fence.
  [[nodiscard]] std::optional<std::wstring> federateNameFor(
      std::wstring const& federationName,
      std::uint64_t federateId) const;
  // Normalized values are execution-scoped coordinates used by the standard
  // MIM dimensions. They deliberately do not expose the private sequential
  // handle directories and remain stable for equal designators throughout an
  // execution, including a restore of that execution.
  [[nodiscard]] std::optional<unsigned long> normalizedFederateHandleValueFor(
      std::wstring const& federationName,
      std::uint64_t federateId) const;
  [[nodiscard]] std::optional<std::uint64_t> objectClassHandleFor(
      std::wstring const& federationName,
      std::string const& objectClassName) const;
  [[nodiscard]] std::optional<std::string> objectClassNameFor(
      std::wstring const& federationName,
      std::uint64_t objectClassHandle) const;
  [[nodiscard]] std::optional<unsigned long> normalizedObjectClassHandleValueFor(
      std::wstring const& federationName,
      std::uint64_t objectClassHandle) const;
  [[nodiscard]] std::optional<std::uint64_t> attributeHandleFor(
      std::wstring const& federationName,
      std::string const& objectClassName,
      std::string const& attributeName) const;
  [[nodiscard]] std::optional<std::string> attributeNameFor(
      std::wstring const& federationName,
      std::string const& objectClassName,
      std::uint64_t attributeHandle) const;
  [[nodiscard]] std::optional<std::uint64_t> interactionClassHandleFor(
      std::wstring const& federationName,
      std::string const& interactionClassName) const;
  [[nodiscard]] std::optional<std::string> interactionClassNameFor(
      std::wstring const& federationName,
      std::uint64_t interactionClassHandle) const;
  [[nodiscard]] std::optional<unsigned long>
  normalizedInteractionClassHandleValueFor(
      std::wstring const& federationName,
      std::uint64_t interactionClassHandle) const;
  [[nodiscard]] std::optional<unsigned long> normalizedObjectInstanceHandleValueFor(
      std::wstring const& federationName,
      std::uint64_t objectInstanceHandle) const;
  // MOM administration must recognize compatible extension subclasses as a
  // promoted form of their predefined parent class. This lookup keeps that
  // hierarchy decision inside the federation-owned catalog/handle space.
  [[nodiscard]] std::optional<bool> interactionClassIsSameOrDescendantOf(
      std::wstring const& federationName,
      std::uint64_t interactionClassHandle,
      std::string const& ancestorInteractionClassName) const;
  [[nodiscard]] std::optional<std::uint64_t> parameterHandleFor(
      std::wstring const& federationName,
      std::string const& interactionClassName,
      std::string const& parameterName) const;
  [[nodiscard]] std::optional<std::string> parameterNameFor(
      std::wstring const& federationName,
      std::string const& interactionClassName,
      std::uint64_t parameterHandle) const;
  [[nodiscard]] UpdateRateValueResult updateRateValueForDesignator(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::string const& updateRateDesignator) const;
  [[nodiscard]] UpdateRateValueResult updateRateValueForAttribute(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t attributeHandle) const;
  [[nodiscard]] std::optional<std::uint64_t> dimensionHandleFor(
      std::wstring const& federationName,
      std::string const& dimensionName) const;
  [[nodiscard]] std::optional<std::string> dimensionNameFor(
      std::wstring const& federationName,
      std::uint64_t dimensionHandle) const;
  [[nodiscard]] std::optional<unsigned long> dimensionUpperBoundFor(
      std::wstring const& federationName,
      std::uint64_t dimensionHandle) const;
  [[nodiscard]] std::optional<std::uint64_t> transportationTypeHandleFor(
      std::wstring const& federationName,
      std::string const& transportationTypeName) const;
  [[nodiscard]] std::optional<std::string> transportationTypeNameFor(
      std::wstring const& federationName,
      std::uint64_t transportationTypeHandle) const;
  [[nodiscard]] std::optional<std::set<std::uint64_t>> availableDimensionsForObjectClass(
      std::wstring const& federationName,
      std::uint64_t objectClassHandle) const;
  [[nodiscard]] std::optional<std::set<std::uint64_t>> availableDimensionsForInteractionClass(
      std::wstring const& federationName,
      std::uint64_t interactionClassHandle) const;

  // DDM region-template/specification lifecycle.  Create allocates a region
  // with the requested dimensions; SetRangeBounds records pending values;
  // Commit promotes a complete pending map to the current specification; and
  // Delete removes an unused region.  These methods intentionally do not
  // create region realizations or route messages.
  [[nodiscard]] RegionCreateResult createRegion(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::set<std::uint64_t> const& dimensionHandles);
  [[nodiscard]] RegionServiceStatus commitRegionModifications(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::set<std::uint64_t> const& regionHandles);
  [[nodiscard]] RegionScopeChangePlan commitRegionModificationsWithScopeChanges(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::set<std::uint64_t> const& regionHandles);
  [[nodiscard]] RegionServiceStatus deleteRegion(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t regionHandle);
  [[nodiscard]] RegionDimensionSetResult dimensionHandleSetForRegion(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t regionHandle) const;
  [[nodiscard]] RegionRangeBoundsResult rangeBoundsForRegion(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t regionHandle,
      std::uint64_t dimensionHandle) const;
  [[nodiscard]] RegionServiceStatus setRangeBounds(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t regionHandle,
      std::uint64_t dimensionHandle,
      RegionRangeBounds range);

  InteractionClassDeclarationStatus setInteractionClassPublication(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t interactionClassHandle,
      bool published);
  [[nodiscard]] DirectedInteractionDeclarationStatus
  publishObjectClassDirectedInteractions(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::set<std::uint64_t> const& interactionClassHandles);
  [[nodiscard]] DirectedInteractionDeclarationStatus
  unpublishObjectClassDirectedInteractions(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::optional<std::set<std::uint64_t>> const& interactionClassHandles);
  [[nodiscard]] DirectedInteractionDeclarationStatus
  subscribeObjectClassDirectedInteractions(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::set<std::uint64_t> const& interactionClassHandles,
      bool universally);
  [[nodiscard]] DirectedInteractionDeclarationStatus
  unsubscribeObjectClassDirectedInteractions(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::optional<std::set<std::uint64_t>> const& interactionClassHandles);
  InteractionClassDeclarationStatus setInteractionClassSubscription(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t interactionClassHandle,
      std::optional<bool> active);
  RegionalInteractionClassDeclarationStatus
  setInteractionClassRegionalSubscription(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t interactionClassHandle,
      std::set<std::uint64_t> const& regionHandles,
      bool active);
  RegionalInteractionClassDeclarationStatus
  removeInteractionClassRegionalSubscription(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t interactionClassHandle,
      std::set<std::uint64_t> const& regionHandles);
  [[nodiscard]] std::optional<InteractionClassDeclarationSnapshot>
  interactionClassDeclarationFor(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t interactionClassHandle) const;

  // Computes ordinary declaration-management relevance transitions for the
  // current publication/subscription state. Regional declarations are kept
  // out of this planner because the 2025 regional services must not trigger
  // the ordinary Start/Stop or Turn On/Off advisories.
  [[nodiscard]] std::vector<DeclarationAdvisory> planDeclarationAdvisories(
      std::wstring const& federationName);

  ObjectClassAttributeDeclarationStatus setObjectClassAttributePublication(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::set<std::uint64_t> const& attributeHandles,
      bool published);
  ObjectClassAttributeDeclarationStatus setObjectClassAttributeSubscription(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::set<std::uint64_t> const& attributeHandles,
      std::optional<bool> active,
      std::string const& updateRateDesignator = "HLAdefault");
  [[nodiscard]] ObjectClassAttributeSubscriptionScopePlan
  setObjectClassAttributeSubscriptionWithScopeChanges(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::set<std::uint64_t> const& attributeHandles,
      std::optional<bool> active,
      std::string const& updateRateDesignator = "HLAdefault");
  RegionalObjectClassAttributeDeclarationStatus
  setObjectClassAttributeRegionalSubscription(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::map<std::uint64_t, std::set<std::uint64_t>> const& attributesAndRegions,
      bool active,
      std::string const& updateRateDesignator = "HLAdefault");
  [[nodiscard]] RegionalObjectClassAttributeSubscriptionScopePlan
  setObjectClassAttributeRegionalSubscriptionWithScopeChanges(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::map<std::uint64_t, std::set<std::uint64_t>> const& attributesAndRegions,
      bool active,
      std::string const& updateRateDesignator = "HLAdefault");
  RegionalObjectClassAttributeDeclarationStatus
  removeObjectClassAttributeRegionalSubscription(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::map<std::uint64_t, std::set<std::uint64_t>> const& attributesAndRegions);
  [[nodiscard]] RegionalObjectClassAttributeSubscriptionScopePlan
  removeObjectClassAttributeRegionalSubscriptionWithScopeChanges(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::map<std::uint64_t, std::set<std::uint64_t>> const& attributesAndRegions);
  [[nodiscard]] std::optional<ObjectClassAttributeDeclarationSnapshot>
  objectClassAttributeDeclarationFor(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle) const;
  [[nodiscard]] std::optional<std::set<std::uint64_t>>
  publishedObjectClassAttributeHandles(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle) const;

  [[nodiscard]] ObjectInstanceNameReservationResult reserveObjectInstanceName(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::wstring const& objectInstanceName);
  [[nodiscard]] ObjectInstanceNameReservationStatus releaseObjectInstanceName(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::wstring const& objectInstanceName);
  [[nodiscard]] MultipleObjectInstanceNameReservationResult
  reserveMultipleObjectInstanceNames(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::set<std::wstring> const& objectInstanceNames);
  [[nodiscard]] ObjectInstanceNameReservationStatus
  releaseMultipleObjectInstanceNames(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::set<std::wstring> const& objectInstanceNames);

  [[nodiscard]] ObjectInstanceRegistrationResult registerObjectInstance(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::map<std::uint64_t, std::set<std::uint64_t>> const* updateRegionsByAttribute = nullptr,
      std::wstring const* requestedObjectInstanceName = nullptr);

  [[nodiscard]] ObjectInstanceRegionAssociationStatus associateRegionsForUpdates(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectInstanceHandle,
      std::map<std::uint64_t, std::set<std::uint64_t>> const& attributesAndRegions);
  [[nodiscard]] ObjectInstanceRegionAssociationScopePlan
  associateRegionsForUpdatesWithScopeChanges(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectInstanceHandle,
      std::map<std::uint64_t, std::set<std::uint64_t>> const& attributesAndRegions);
  [[nodiscard]] ObjectInstanceRegionAssociationStatus unassociateRegionsForUpdates(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectInstanceHandle,
      std::map<std::uint64_t, std::set<std::uint64_t>> const& attributesAndRegions);
  [[nodiscard]] ObjectInstanceRegionAssociationScopePlan
  unassociateRegionsForUpdatesWithScopeChanges(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectInstanceHandle,
      std::map<std::uint64_t, std::set<std::uint64_t>> const& attributesAndRegions);

  // Accepts the limited no-time deletion request and reserves one removal
  // callback per other known recipient. The deleting federate becomes unknown
  // before this method returns; other recipients remain known until their
  // queued Remove Object Instance callback starts.
  [[nodiscard]] ObjectInstanceDeletionPlan deleteObjectInstance(
      std::wstring const& federationName,
      std::uint64_t deletingFederateId,
      std::uint64_t objectInstanceHandle);

  // Removes only the invoking federate's known-instance state. The object is
  // retained in the federation so a later eligible subscription can discover
  // it again, potentially at a different known class.
  [[nodiscard]] LocalObjectInstanceDeletionStatus localDeleteObjectInstance(
      std::wstring const& federationName,
      std::uint64_t deletingFederateId,
      std::uint64_t objectInstanceHandle);

  // Plans the not-yet-known recipients of one newly registered instance. The
  // registry reserves each returned discovery before releasing its state lock,
  // preventing duplicate queued callbacks while a recipient uses HLA_EVOKED.
  [[nodiscard]] std::vector<ObjectInstanceDiscoveryRecipient>
  planObjectInstanceDiscoveriesForInstance(
      std::wstring const& federationName,
      std::uint64_t objectInstanceHandle);

  // A later subscription can make existing instances discoverable. This
  // mirrors the same private planning boundary as registration without
  // treating a declaration mutation as an object-delivery adapter.
  [[nodiscard]] std::vector<ObjectInstanceDiscoveryRecipient>
  planObjectInstanceDiscoveriesForFederate(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId);

  // After an ordinary discovery establishes the receiving federate's known
  // instance, return the current values for its subscribed attributes,
  // grouped by the callback transportation type. The current declaration and
  // DDM boundary are re-evaluated at callback time.
  [[nodiscard]] std::vector<ObjectInstanceInitialAttributeReflection>
  planInitialObjectInstanceAttributeReflectionsForDiscovery(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle) const;

  // Once a discovery commits the receiver's known-instance state, plan the
  // initial owner-directed Attribute Relevance Advisory for attributes that
  // are already relevant through the receiver's active declaration. The
  // planner compares against an empty prior declaration/association state so
  // this boundary is distinct from later subscription and region transitions.
  [[nodiscard]] std::vector<AttributeRelevanceAdvisoryRecipient>
  planInitialAttributeRelevanceAdvisoriesForDiscovery(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle);

  // RTI-owned joined-federate MOM objects are planned separately from
  // federate-created instances. Their eligibility still follows the active
  // ordinary subscription declaration, while regional MOM realization and
  // dynamic update scheduling remain explicit follow-on slices.
  [[nodiscard]] std::vector<ObjectInstanceDiscoveryRecipient>
  planJoinedFederateMomObjectDiscoveriesForInstance(
      std::wstring const& federationName,
      std::uint64_t objectInstanceHandle);
  [[nodiscard]] std::vector<ObjectInstanceDiscoveryRecipient>
  planJoinedFederateMomObjectDiscoveriesForFederate(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId);

  // Releases a reservation whose callback route could not accept delivery.
  // This deliberately preserves an already-known instance, so it is safe to
  // use after a synchronous HLA_IMMEDIATE callback has entered user code.
  void cancelObjectInstanceDiscovery(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle);

  void cancelJoinedFederateMomObjectDiscovery(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle);

  // Rechecks a planned Attribute In/Out Of Scope transition immediately
  // before callback entry, suppressing stale work after another region,
  // association, subscription, removal, or resignation change.
  [[nodiscard]] std::set<std::uint64_t> objectInstanceScopeAttributes(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& attributeHandles,
      bool expectedInScope) const;

  // Rechecks and commits a pending no-time removal immediately before user
  // callback delivery. Unlike discovery, it does not re-evaluate declarations:
  // a recipient already known to the deleted instance must be informed. A
  // forced connection-loss cleanup retains its reservation instead when a
  // marked timestamped object-dependent payload still needs that recipient's
  // known-instance state; releaseConnectionLossDeferredObjectInstanceRemovals
  // replans it after the protected TSO boundary completes.
  [[nodiscard]] std::optional<RemovedObjectInstanceSnapshot> beginObjectInstanceRemoval(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle);

  [[nodiscard]] std::optional<RemovedObjectInstanceSnapshot>
  beginJoinedFederateMomObjectRemoval(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle);

  // Releases only an undelivered removal reservation. It preserves the
  // recipient's known-instance state so a future recovery path can replan it.
  void cancelObjectInstanceRemoval(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle);

  void cancelJoinedFederateMomObjectRemoval(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle);

  // Rechecks the current discovery predicate immediately before the callback
  // executes, then atomically establishes the recipient's known class. A
  // changed subscription, resignation, or stale queued callback becomes an
  // ordinary no-delivery outcome.
  [[nodiscard]] std::optional<KnownObjectInstanceSnapshot> beginObjectInstanceDiscovery(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle);

  [[nodiscard]] std::optional<KnownObjectInstanceSnapshot>
  beginJoinedFederateMomObjectDiscovery(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle);

  [[nodiscard]] std::optional<KnownObjectInstanceSnapshot> knownObjectInstanceFor(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectInstanceHandle) const;
  [[nodiscard]] std::optional<KnownObjectInstanceSnapshot> knownObjectInstanceByNameFor(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::wstring const& objectInstanceName) const;

  // Plans the RTI-invoked Provide Attribute Value Update callbacks required
  // by the federation-wide Auto Provide switch after a recipient has become
  // newly aware of an object. The supplied tag is intentionally not part of
  // this private plan: the public adapter invokes the callback with an empty
  // tag for this RTI-originated service.
  [[nodiscard]] AttributeValueUpdateRequestPlan planAutoProvideForDiscovery(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle) const;

  // Validates an Update Attribute Values request and forms one passel for
  // every submitted FOM transportation type and explicit association region
  // set. The common plan feeds receive-order and timestamped traffic; relaxed
  // DDM remains outside this boundary while the adapter applies each
  // recipient's explicit update-rate reduction at delivery time.
  [[nodiscard]] ReceiveOrderAttributeUpdatePlan planReceiveOrderAttributeUpdate(
      std::wstring const& federationName,
      std::uint64_t producingFederateId,
       std::uint64_t objectInstanceHandle,
       std::vector<std::uint64_t> const& sentAttributeHandles) const;

  // Recomputes one originally planned passel immediately before callback
  // delivery. A resigning recipient, changed known-instance state, or changed
  // subscription is an ordinary no-delivery outcome; the returned subset is
  // never combined with another passel.
  [[nodiscard]] std::optional<ReceiveOrderAttributeUpdateRecipient>
  receiveOrderAttributeUpdateRecipientFor(
      std::wstring const& federationName,
      std::uint64_t producingFederateId,
      std::uint64_t receivingFederateId,
       std::uint64_t objectInstanceHandle,
       std::vector<std::uint64_t> const& sentAttributeHandles,
       std::set<std::uint64_t> const* sentRegionHandles = nullptr,
       std::map<std::uint64_t, RegionSpecificationSnapshot> const* regionOverrides = nullptr) const;

  // Validates an object-instance Request Attribute Value Update request at
  // the requester's known class, then groups currently owned requested
  // attributes by their providing federate. Attributes owned by the requester
  // are an implicit local provide and therefore have no callback recipient.
  [[nodiscard]] AttributeValueUpdateRequestPlan planAttributeValueUpdateRequest(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& requestedAttributeHandles) const;

  [[nodiscard]] JoinedFederateMomAttributeValueUpdatePlan
  planJoinedFederateMomAttributeValueUpdate(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& requestedAttributeHandles,
      bool requireActiveSubscription = false,
      std::map<std::uint64_t, std::set<std::uint64_t>> const*
          requestRegionsByAttribute = nullptr) const;

  [[nodiscard]] JoinedFederateMomAttributeValueUpdateClassPlan
  planJoinedFederateMomAttributeValueUpdateClass(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectClassHandle,
      std::set<std::uint64_t> const& requestedAttributeHandles,
      bool requireActiveSubscription = false,
      std::map<std::uint64_t, std::set<std::uint64_t>> const*
          requestRegionsByAttribute = nullptr) const;

  // Plans an RTI-originated conditional update for one joined-federate MOM
  // object. Automatic updates set requireActiveSubscription so only current
  // ordinary/regional subscribers receive them; public Request Attribute
  // Value Update keeps the instance/class forms above without that policy.
  [[nodiscard]] JoinedFederateMomAttributeValueUpdateClassPlan
  planJoinedFederateMomAttributeValueUpdateForObject(
      std::wstring const& federationName,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& requestedAttributeHandles,
      bool requireActiveSubscription = true,
      std::optional<std::uint64_t> excludedReceivingFederateId = std::nullopt) const;

  // Rechecks one original owner group immediately before Provide Attribute
  // Value Update delivery. A removed instance, resigned owner/requester, or
  // changed ownership becomes an ordinary no-delivery outcome.
  [[nodiscard]] std::optional<AttributeValueUpdateProvideRecipient>
  attributeValueUpdateProvideRecipientFor(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t providingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& requestedAttributeHandles,
      bool requireCurrentScope = false) const;

  // Persists one accepted object-instance Request Attribute Value Update in
  // the route-free federation state. The live provider callback is queued by
  // the ambassador only after the request has been recorded here.
  [[nodiscard]] std::optional<std::uint64_t>
  registerAttributeValueUpdateRequest(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t providingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& requestedAttributeHandles,
      std::vector<unsigned char> userSuppliedTag);

  // Atomically consumes a persisted request and rechecks its live provider
  // eligibility. A suppressed or stale request is consumed without a
  // callback, matching the existing one-shot provider-delivery boundary.
  [[nodiscard]] std::optional<AttributeValueUpdateProvideRecipient>
  beginAttributeValueUpdateProvideRecipientFor(
      std::wstring const& federationName,
      std::uint64_t requestId,
      std::uint64_t requestingFederateId,
      std::uint64_t providingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& requestedAttributeHandles);

  // Validates an object-class Request Attribute Value Update request, then
  // expands it over all current instances registered at the requested class
  // and its subclasses. Unlike the object-instance form, the standard has no
  // requester-known-instance precondition for this form.
  [[nodiscard]] AttributeValueUpdateClassRequestPlan planAttributeValueUpdateClassRequest(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectClassHandle,
      std::set<std::uint64_t> const& requestedAttributeHandles,
      std::map<std::uint64_t, std::set<std::uint64_t>> const* requestRegionsByAttribute =
          nullptr) const;

  // Rechecks one original object-class request owner group immediately before
  // Provide Attribute Value Update delivery. A removed instance, resigned
  // owner/requester, changed class hierarchy, or changed ownership becomes an
  // ordinary no-delivery outcome.
  [[nodiscard]] std::optional<AttributeValueUpdateProvideRecipient>
  attributeValueUpdateClassProvideRecipientFor(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t providingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t requestedObjectClassHandle,
      std::set<std::uint64_t> const& requestedAttributeHandles,
      std::map<std::uint64_t, std::set<std::uint64_t>> const* requestRegionsByAttribute =
          nullptr) const;

  // Persists one accepted non-regional object-class Request Attribute Value
  // Update provider delivery.  The class request is expanded by the planner,
  // so each object/provider group receives its own durable identity.
  [[nodiscard]] std::optional<std::uint64_t>
  registerAttributeValueUpdateClassRequest(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t providingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t requestedObjectClassHandle,
      std::set<std::uint64_t> const& requestedAttributeHandles,
      std::vector<unsigned char> userSuppliedTag);

  // Atomically consumes one persisted class-form delivery and rechecks its
  // current hierarchy/ownership eligibility. Regional class requests remain
  // on the existing non-durable path until their own typed ledger is added.
  [[nodiscard]] std::optional<AttributeValueUpdateProvideRecipient>
  beginAttributeValueUpdateClassProvideRecipientFor(
      std::wstring const& federationName,
      std::uint64_t requestId,
      std::uint64_t requestingFederateId,
      std::uint64_t providingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t requestedObjectClassHandle,
      std::set<std::uint64_t> const& requestedAttributeHandles);

  // Persists one accepted regional object-class Request Attribute Value
  // Update provider delivery, including the requester's per-attribute region
  // designators. This is a distinct ledger from the non-regional class form.
  [[nodiscard]] std::optional<std::uint64_t>
  registerAttributeValueUpdateRegionalRequest(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t providingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t requestedObjectClassHandle,
      std::set<std::uint64_t> const& requestedAttributeHandles,
      std::map<std::uint64_t, std::set<std::uint64_t>> const&
          requestRegionsByAttribute,
      std::vector<unsigned char> userSuppliedTag);

  [[nodiscard]] std::optional<AttributeValueUpdateProvideRecipient>
  beginAttributeValueUpdateRegionalProvideRecipientFor(
      std::wstring const& federationName,
      std::uint64_t requestId,
      std::uint64_t requestingFederateId,
      std::uint64_t providingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t requestedObjectClassHandle,
      std::set<std::uint64_t> const& requestedAttributeHandles,
      std::map<std::uint64_t, std::set<std::uint64_t>> const&
          requestRegionsByAttribute);

  // Validates a Query Attribute Ownership request at the requester's known
  // class, then groups requested attributes into their standard C++ owner
  // reports. Federate-created instances use federate/unowned reports;
  // discovered RTI-owned joined-federate MOM instances use the explicit RTI
  // report kind.
  [[nodiscard]] AttributeOwnershipQueryPlan planAttributeOwnershipQuery(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& requestedAttributeHandles);

  // Rechecks one pending ownership report immediately before callback
  // delivery. A removed instance, resigned requester, changed known-class
  // boundary, or changed owner state becomes an ordinary no-delivery outcome.
  // In particular, deletion nullifies pending Inform Attribute Ownership
  // callbacks before they can enter user code.
  [[nodiscard]] std::optional<AttributeOwnershipQueryRecipient>
  attributeOwnershipQueryRecipientFor(
      std::wstring const& federationName,
      std::uint64_t requestId,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      AttributeOwnershipQueryReportKind reportKind,
      std::uint64_t owningFederateId,
      std::set<std::uint64_t> const& requestedAttributeHandles);

  // Determines whether one valid attribute of a requester's known instance is
  // owned by that same joined federate. It has no callback or transfer effect.
  [[nodiscard]] AttributeOwnershipCheckResult attributeOwnedByFederate(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t attributeHandle) const;

  // Validates and records one Attribute Ownership Acquisition If Available
  // request. For a nonempty successful request, the returned request ID owns
  // the private Willing to Acquire reservation until callback delivery or
  // an internal invalidating lifecycle transition. The adapter retains the
  // user tag and queues the callback only after releasing its locks. A
  // repeated request made by the same federate for attributes already in that state leaves those attributes
  // unchanged and returns an applied plan without a duplicate callback route.
  [[nodiscard]] AttributeOwnershipAcquisitionIfAvailablePlan
  planAttributeOwnershipAcquisitionIfAvailable(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& desiredAttributeHandles,
      std::vector<unsigned char> userSuppliedTag);

  // Resolves one still-pending request immediately before a federate callback
  // executes. A deleted instance, resigned requester, stale request, or
  // inconsistent ownership state becomes a no-delivery outcome. Attributes
  // that remain unowned are atomically assigned to the requester immediately
  // before Attribute Ownership Acquisition Notification is invoked.
  [[nodiscard]] std::optional<AttributeOwnershipAcquisitionIfAvailableDelivery>
  beginAttributeOwnershipAcquisitionIfAvailable(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t requestId);

  // Releases an accepted request only when its callback route could not be
  // submitted. It is harmless after immediate callback delivery or another
  // invalidating lifecycle transition has already consumed the reservation.
  void cancelAttributeOwnershipAcquisitionIfAvailable(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t requestId);

  // Records a Negotiated Attribute Ownership Divestiture without changing
  // ownership. Existing and later regular Acquisition Pending or Willing to
  // Acquire requests can cause the owning federate to receive the standard
  // Request Divestiture Confirmation callback. The bounded profile does not
  // yet run the complete owner-search lifecycle or negotiated acquisition.
  [[nodiscard]] NegotiatedAttributeOwnershipDivestiturePlan
  planNegotiatedAttributeOwnershipDivestiture(
      std::wstring const& federationName,
      std::uint64_t divestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& attributeHandles,
      std::vector<unsigned char> userSuppliedTag);

  // Rechecks a queued Request Divestiture Confirmation immediately before
  // callback entry. Only attributes that still belong to the divesting owner
  // and still have the selected regular or If Available acquisition pending
  // are exposed.
  [[nodiscard]] std::optional<RequestDivestitureConfirmationDelivery>
  beginRequestDivestitureConfirmation(
      std::wstring const& federationName,
      std::uint64_t divestingFederateId,
      std::uint64_t acquiringFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t acquisitionRequestId,
      bool candidateIsIfAvailable,
      std::set<std::uint64_t> const& scheduledAttributeHandles);

  // Completes the confirmed negotiated transfer atomically. Ownership is
  // visible before the paired Acquisition Notification callback begins, while
  // a private reservation retains the new owner's publication boundary.
  [[nodiscard]] ConfirmDivestiturePlan planConfirmDivestiture(
      std::wstring const& federationName,
      std::uint64_t divestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& attributeHandles,
      std::vector<unsigned char> userSuppliedTag);

  // Consumes one post-confirmation notification reservation immediately
  // before the official Attribute Ownership Acquisition Notification enters
  // user code, then plans any later regular-acquisition work.
  [[nodiscard]] std::optional<ConfirmDivestitureNotificationDelivery>
  beginConfirmDivestitureNotification(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t notificationId,
      std::set<std::uint64_t> const& scheduledAttributeHandles);

  // Cancels a still-pending negotiated divestiture and restores ordinary
  // regular-acquisition release planning. Any previously queued confirmation
  // callback becomes stale before it can enter user code.
  [[nodiscard]] CancelNegotiatedAttributeOwnershipDivestiturePlan
  planCancelNegotiatedAttributeOwnershipDivestiture(
      std::wstring const& federationName,
      std::uint64_t divestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& attributeHandles);

  // Validates and records a regular Attribute Ownership Acquisition request.
  // Repeated attributes already pending for the same federate remain in that
  // state and do not produce a duplicate owner-side release callback.  A
  // regular request overrides any still-pending If Available reservation for
  // the same federate and attribute, as required by IEEE 1516.1-2025 7.1.2.2.
  [[nodiscard]] AttributeOwnershipAcquisitionPlan planAttributeOwnershipAcquisition(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& desiredAttributeHandles,
      std::vector<unsigned char> userSuppliedTag);

  // Resolves one queued regular-acquisition notification immediately before
  // user code runs.  It commits ownership only for still-unowned attributes,
  // then returns any next-step work that must be queued after that callback.
  [[nodiscard]] std::optional<AttributeOwnershipAcquisitionNotificationDelivery>
  beginAttributeOwnershipAcquisitionNotification(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t requestId,
      std::set<std::uint64_t> const& scheduledAttributeHandles);

  // Rechecks a queued owner-side Request Attribute Ownership Release callback
  // before delivery.  A stale request, removed instance, resigned federate,
  // or changed owner is an ordinary no-delivery outcome.
  [[nodiscard]] std::optional<AttributeOwnershipAcquisitionReleaseDelivery>
  beginAttributeOwnershipAcquisitionRelease(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t owningFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t requestId,
      std::set<std::uint64_t> const& scheduledAttributeHandles);

  // Validates an owner-side release denial, retains current ownership, and
  // terminates matching regular pending acquisitions.  The adapter supplies
  // the release-denial tag to each resulting unavailable callback.
  [[nodiscard]] AttributeOwnershipReleaseDeniedPlan
  planAttributeOwnershipReleaseDenied(
      std::wstring const& federationName,
      std::uint64_t owningFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& attributeHandles);

  // Validates and performs a synchronous Attribute Ownership Divestiture If
  // Wanted transfer. The returned set contains only attributes for which a
  // still-pending regular or If Available acquirer was selected. The callback
  // work uses the divestiture tag, not an older acquisition-request tag.
  [[nodiscard]] AttributeOwnershipDivestitureIfWantedPlan
  planAttributeOwnershipDivestitureIfWanted(
      std::wstring const& federationName,
      std::uint64_t divestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& attributeHandles,
      std::vector<unsigned char> userSuppliedTag);

  // Consumes the private post-transfer notification reservation immediately
  // before the standard Acquisition Notification callback enters user code.
  // This is the terminal boundary for the selected acquirer's publication
  // guard and can make other regular acquisition work eligible afterward.
  [[nodiscard]] std::optional<AttributeOwnershipDivestitureIfWantedDelivery>
  beginAttributeOwnershipDivestitureIfWantedNotification(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t notificationId,
      std::set<std::uint64_t> const& scheduledAttributeHandles);

  // Validates and performs an Unconditional Attribute Ownership Divestiture.
  // Every supplied attribute is made unowned before this method returns.  It
  // produces follow-up regular-acquisition work plus one grouped Request
  // Attribute Ownership Assumption callback candidate for each currently
  // eligible non-pending joined federate.  The adapter owns the original
  // divestiture tag and queues all returned work after releasing its locks.
  [[nodiscard]] UnconditionalAttributeOwnershipDivestiturePlan
  planUnconditionalAttributeOwnershipDivestiture(
      std::wstring const& federationName,
      std::uint64_t divestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& attributeHandles,
      std::vector<unsigned char> userSuppliedTag);

  // Rechecks one queued Request Attribute Ownership Assumption callback at
  // its delivery boundary.  It emits only the original offered attributes
  // that remain unowned and currently eligible at this recipient.  A stale
  // callback is suppressed before it can enter FederateAmbassador code, and
  // its candidate reservation is released so a later publication/discovery
  // event can continue the assumption search.
  [[nodiscard]] std::optional<AttributeOwnershipAssumptionDelivery>
  attributeOwnershipAssumptionDeliveryFor(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& scheduledAttributeHandles);

  // Continues a prior unconditional-divestiture or resign-action assumption
  // search for one federate whose eligibility may have changed.  The method
  // reserves each newly offered (object, attribute, federate) tuple before
  // returning callback records, so repeated publication/discovery events do
  // not duplicate an outstanding assumption callback.  Ownership itself is
  // still established only by a standard acquisition service.
  [[nodiscard]] std::vector<AttributeOwnershipAssumptionRecipient>
  planAttributeOwnershipAssumptionsForFederate(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::optional<std::uint64_t> objectInstanceFilter = std::nullopt,
      std::set<std::uint64_t> const* attributeFilter = nullptr);

  // Validates and accepts a regular-acquisition cancellation. The original
  // acquisition work is made stale immediately, but the separate cancellation
  // reservation keeps the caller's required publication active until the
  // confirmation callback begins.
  [[nodiscard]] AttributeOwnershipAcquisitionCancellationPlan
  planAttributeOwnershipAcquisitionCancellation(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& attributeHandles);

  // Rechecks and consumes one queued Confirm Attribute Ownership Acquisition
  // Cancellation callback immediately before it enters user code. Removing
  // the reservation at that boundary releases the paired publication guard
  // and can make later regular acquisition work eligible.
  [[nodiscard]] std::optional<AttributeOwnershipAcquisitionCancellationDelivery>
  beginAttributeOwnershipAcquisitionCancellation(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t cancellationId,
      std::set<std::uint64_t> const& scheduledAttributeHandles);

  // Rechecks a queued unavailable callback after release denial.  Object
  // removal, requester resignation, or a changed known-instance boundary
  // suppresses the callback before user code is entered.
  [[nodiscard]] std::optional<AttributeOwnershipUnavailableRecipient>
  attributeOwnershipUnavailableRecipientFor(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& attributeHandles);

  // Validates a receive-order Send Interaction request and captures one
  // recipient projection per eligible joined federate.  A passive
  // subscription still receives interactions; its active flag only controls
  // declaration-management advisory callbacks that are outside this slice.
  [[nodiscard]] ReceiveOrderInteractionPlan planReceiveOrderInteraction(
      std::wstring const& federationName,
      std::uint64_t producingFederateId,
      std::uint64_t sentInteractionClassHandle,
      std::vector<std::uint64_t> const& sentParameterHandles,
      std::set<std::uint64_t> const* sentRegionHandles = nullptr) const;

  // Recomputes one recipient projection immediately before callback delivery.
  // A missing recipient, changed subscription, or removed federation is an
  // ordinary no-delivery outcome rather than a callback-time public error.
  [[nodiscard]] std::optional<ReceiveOrderInteractionRecipient>
  receiveOrderInteractionRecipientFor(
      std::wstring const& federationName,
      std::uint64_t producingFederateId,
      std::uint64_t receivingFederateId,
      std::uint64_t sentInteractionClassHandle,
      std::vector<std::uint64_t> const& sentParameterHandles,
      std::set<std::uint64_t> const* sentRegionHandles = nullptr,
      std::map<std::uint64_t, RegionSpecificationSnapshot> const* regionOverrides = nullptr) const;

  // Plans the private RTI report endpoint mandated by §11.5.1.  The exact
  // endpoint contains HLAfederate and HLAserviceGroup point ranges only;
  // report-to-file selects a distinct sink and therefore never also delivers
  // to interaction subscribers.
  [[nodiscard]] MomServiceReportRoutingPlan planMomServiceReport(
      std::wstring const& federationName,
      std::uint64_t reportedFederateId,
      std::uint16_t serviceGroup) const;

  // Atomically obtains the routing decision and serial value for one report
  // accepted for an interaction or report-file sink. Suppressed, invalid, and
  // recipientless-interaction requests leave the sequence untouched.
  [[nodiscard]] ReservedMomServiceReport reserveMomServiceReport(
      std::wstring const& federationName,
      std::uint64_t reportedFederateId,
      std::uint16_t serviceGroup);

  [[nodiscard]] std::optional<ReceiveOrderInteractionRecipient>
  momServiceReportRecipientFor(
      std::wstring const& federationName,
      std::uint64_t reportedFederateId,
      std::uint64_t receivingFederateId,
      std::uint16_t serviceGroup) const;

  // Captures the mandatory HLAreportFederateLost traffic before a fault-driven
  // resignation removes the affected membership and its time state. The
  // selected recipients are current surviving subscribers; an empty list is
  // a valid outcome when no surviving federate subscribed.
  [[nodiscard]] FederateLostReportPlan planFederateLostReport(
      std::wstring const& federationName,
      std::uint64_t reportedFederateId) const;

  // Rechecks one queued report after the lost membership was removed. Unlike
  // ordinary send interaction routing, the reported federate is deliberately
  // allowed to be departed; its lifetime designator remains in
  // federateNamesById while the receiving federate must still be joined.
  [[nodiscard]] std::optional<ReceiveOrderInteractionRecipient>
  federateLostReportRecipientFor(
      std::wstring const& federationName,
      std::uint64_t reportedFederateId,
      std::uint64_t receivingFederateId) const;

  // Captures one HLAreportException route while the failing federate remains
  // joined. The callback-time recipient query repeats the same switch,
  // catalog, and subscription checks after the service exception has crossed
  // the public call boundary.
  [[nodiscard]] ExceptionReportPlan planExceptionReport(
      std::wstring const& federationName,
      std::uint64_t reportedFederateId) const;

  [[nodiscard]] std::optional<ReceiveOrderInteractionRecipient>
  exceptionReportRecipientFor(
      std::wstring const& federationName,
      std::uint64_t reportedFederateId,
      std::uint64_t receivingFederateId) const;

  // Captures one HLAreportMOMexception route for a rejected MOM interaction.
  // This report family is subscription-selected rather than controlled by
  // HLAexceptionReporting, and it uses the same private HLAfederate endpoint
  // shape as HLAreportException.
  [[nodiscard]] MomExceptionReportPlan planMomExceptionReport(
      std::wstring const& federationName,
      std::uint64_t reportedFederateId) const;

  [[nodiscard]] std::optional<ReceiveOrderInteractionRecipient>
  momExceptionReportRecipientFor(
      std::wstring const& federationName,
      std::uint64_t reportedFederateId,
      std::uint64_t receivingFederateId) const;

  // Plans and rechecks the bounded HLArequestObjectInstancesUpdated /
  // HLAreportObjectInstancesUpdated MOM interaction pair. Counts are
  // captured at request acceptance and grouped by registered object class.
  [[nodiscard]] MomObjectInstanceCountsReportPlan
  planMomObjectInstancesUpdatedReport(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t reportedFederateId) const;

  [[nodiscard]] std::optional<ReceiveOrderInteractionRecipient>
  momObjectInstancesUpdatedReportRecipientFor(
      std::wstring const& federationName,
      std::uint64_t reportedFederateId,
      std::uint64_t receivingFederateId) const;

  // Plans and rechecks the bounded HLArequestObjectInstancesThatCanBeDeleted /
  // HLAreportObjectInstancesThatCanBeDeleted MOM interaction pair. Counts
  // are derived from the live HLAprivilegeToDeleteObject ownership ledger and
  // grouped by registered object class.
  [[nodiscard]] MomObjectInstanceCountsReportPlan
  planMomObjectInstancesThatCanBeDeletedReport(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t reportedFederateId) const;

  [[nodiscard]] std::optional<ReceiveOrderInteractionRecipient>
  momObjectInstancesThatCanBeDeletedReportRecipientFor(
      std::wstring const& federationName,
      std::uint64_t reportedFederateId,
      std::uint64_t receivingFederateId) const;

  // Plans and rechecks the bounded HLArequestObjectInstancesReflected /
  // HLAreportObjectInstancesReflected MOM interaction pair. Counts are
  // grouped by registered object class and retained for the joined-federate
  // lifetime at the accepted reflection callback boundary.
  [[nodiscard]] MomObjectInstanceCountsReportPlan
  planMomObjectInstancesReflectedReport(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t reportedFederateId) const;

  [[nodiscard]] std::optional<ReceiveOrderInteractionRecipient>
  momObjectInstancesReflectedReportRecipientFor(
      std::wstring const& federationName,
      std::uint64_t reportedFederateId,
      std::uint64_t receivingFederateId) const;

  // Plans and rechecks the bounded HLArequestObjectInstanceInformation /
  // HLAreportObjectInstanceInformation MOM interaction pair. The request is
  // scoped to its sender; the report payload is a request-time snapshot and
  // retains the same private HLAfederate endpoint through callback delivery.
  [[nodiscard]] MomObjectInstanceInformationReportPlan
  planMomObjectInstanceInformationReport(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle) const;

  [[nodiscard]] std::optional<ReceiveOrderInteractionRecipient>
  momObjectInstanceInformationReportRecipientFor(
      std::wstring const& federationName,
      std::uint64_t reportedFederateId,
      std::uint64_t receivingFederateId) const;

  // Plans and rechecks the federate-scoped HLArequestFOMmoduleData /
  // HLAreportFOMmoduleData MOM interaction pair. Module indices address the
  // first-designator FOM sequence supplied at Join, and the report retains
  // the represented federate's private HLAfederate endpoint.
  [[nodiscard]] MomFomModuleDataReportPlan
  planMomFomModuleDataReport(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t reportedFederateId,
      std::uint32_t moduleIndex) const;

  [[nodiscard]] std::optional<ReceiveOrderInteractionRecipient>
  momFomModuleDataReportRecipientFor(
      std::wstring const& federationName,
      std::uint64_t reportedFederateId,
      std::uint64_t receivingFederateId) const;

  // Plans and rechecks the federation-scoped FOM-module data report. The
  // report is dimensionless in the official MIM and therefore reaches every
  // current subscriber rather than a private HLAfederate point.
  [[nodiscard]] MomFederationFomModuleDataReportPlan
  planMomFederationFomModuleDataReport(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint32_t moduleIndex) const;

  [[nodiscard]] std::optional<ReceiveOrderInteractionRecipient>
  momFederationFomModuleDataReportRecipientFor(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId) const;

  // Plans and rechecks the federation-scoped MIM-data report. The validated
  // MIM serialization is retained in the federation definition rather than
  // being reread from its source path when a request arrives.
  [[nodiscard]] MomFederationMimDataReportPlan
  planMomFederationMimDataReport(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId) const;

  [[nodiscard]] std::optional<ReceiveOrderInteractionRecipient>
  momFederationMimDataReportRecipientFor(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId) const;

  // Plans and rechecks the federation-scoped synchronization-point list
  // report. The report contains the active labels in registry order and uses
  // the MIM-defined empty variable-array value when no point is active.
  [[nodiscard]] MomFederationSynchronizationPointsReportPlan
  planMomFederationSynchronizationPointsReport(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId) const;

  [[nodiscard]] std::optional<ReceiveOrderInteractionRecipient>
  momFederationSynchronizationPointsReportRecipientFor(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId) const;

  // Plans and rechecks the federation-scoped synchronization-point status
  // report. Unknown labels intentionally produce a valid NULL response with
  // an empty HLAsyncPointFederates array, matching the MIM semantics.
  [[nodiscard]] MomFederationSynchronizationPointStatusReportPlan
  planMomFederationSynchronizationPointStatusReport(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::wstring synchronizationPointName) const;

  [[nodiscard]] std::optional<ReceiveOrderInteractionRecipient>
  momFederationSynchronizationPointStatusReportRecipientFor(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId) const;

  // Plans and rechecks the bounded HLArequestPublications family.  The
  // request-time snapshot includes implicit HLAprivilegeToDeleteObject
  // publication, ordinary interaction publications, and object-class
  // directed-interaction publication.  Each report retains the target's
  // private HLAfederate endpoint through callback delivery.
  [[nodiscard]] MomPublicationsReportPlan planMomPublicationsReport(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId) const;

  [[nodiscard]] std::optional<ReceiveOrderInteractionRecipient>
  momObjectClassPublicationReportRecipientFor(
      std::wstring const& federationName,
      std::uint64_t reportedFederateId,
      std::uint64_t receivingFederateId) const;

  [[nodiscard]] std::optional<ReceiveOrderInteractionRecipient>
  momInteractionPublicationReportRecipientFor(
      std::wstring const& federationName,
      std::uint64_t reportedFederateId,
      std::uint64_t receivingFederateId) const;

  [[nodiscard]] std::optional<ReceiveOrderInteractionRecipient>
  momDirectedInteractionPublicationReportRecipientFor(
      std::wstring const& federationName,
      std::uint64_t reportedFederateId,
      std::uint64_t receivingFederateId) const;

  // Plans and rechecks the bounded HLArequestSubscriptions family. The
  // request-time snapshot preserves active/passive object-class groups,
  // maximum update-rate designators, ordinary interaction subscription
  // pairs, and directed object-class subscription groups. Each report keeps
  // the target's private HLAfederate endpoint through callback delivery.
  [[nodiscard]] MomSubscriptionsReportPlan planMomSubscriptionsReport(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId) const;

  [[nodiscard]] std::optional<ReceiveOrderInteractionRecipient>
  momObjectClassSubscriptionReportRecipientFor(
      std::wstring const& federationName,
      std::uint64_t reportedFederateId,
      std::uint64_t receivingFederateId) const;

  [[nodiscard]] std::optional<ReceiveOrderInteractionRecipient>
  momInteractionSubscriptionReportRecipientFor(
      std::wstring const& federationName,
      std::uint64_t reportedFederateId,
      std::uint64_t receivingFederateId) const;

  [[nodiscard]] std::optional<ReceiveOrderInteractionRecipient>
  momDirectedInteractionSubscriptionReportRecipientFor(
      std::wstring const& federationName,
      std::uint64_t reportedFederateId,
      std::uint64_t receivingFederateId) const;

  // Plans and rechecks the bounded HLArequestUpdatesSent /
  // HLAreportUpdatesSent MOM interaction pair. One report payload is created
  // for each transportation type present in the accepted update ledger.
  [[nodiscard]] MomUpdatesSentReportPlan planMomUpdatesSentReport(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t reportedFederateId) const;

  [[nodiscard]] std::optional<ReceiveOrderInteractionRecipient>
  momUpdatesSentReportRecipientFor(
      std::wstring const& federationName,
      std::uint64_t reportedFederateId,
      std::uint64_t receivingFederateId) const;

  // Plans and rechecks the bounded HLArequestInteractionsSent /
  // HLAreportInteractionsSent MOM interaction pair. One report payload is
  // created for each transportation type present in the accepted sender
  // ledger and groups positive counts by sent interaction class.
  [[nodiscard]] MomInteractionsSentReportPlan
  planMomInteractionsSentReport(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t reportedFederateId) const;

  [[nodiscard]] std::optional<ReceiveOrderInteractionRecipient>
  momInteractionsSentReportRecipientFor(
      std::wstring const& federationName,
      std::uint64_t reportedFederateId,
      std::uint64_t receivingFederateId) const;

  // Plans and rechecks the bounded HLArequestDirectedInteractionsSent /
  // HLAreportDirectedInteractionsSent MOM interaction pair. One report
  // payload is created for each transportation type present in the accepted
  // directed sender ledger and groups positive counts by sent class.
  [[nodiscard]] MomDirectedInteractionsSentReportPlan
  planMomDirectedInteractionsSentReport(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t reportedFederateId) const;

  [[nodiscard]] std::optional<ReceiveOrderInteractionRecipient>
  momDirectedInteractionsSentReportRecipientFor(
      std::wstring const& federationName,
      std::uint64_t reportedFederateId,
      std::uint64_t receivingFederateId) const;

  // Plans and rechecks the bounded HLArequestInteractionsReceived /
  // HLAreportInteractionsReceived MOM interaction pair. One report payload is
  // created for each supported transportation type, including an empty NULL
  // response bucket, and groups positive counts by sent interaction class.
  [[nodiscard]] MomInteractionsReceivedReportPlan
  planMomInteractionsReceivedReport(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t reportedFederateId) const;

  [[nodiscard]] std::optional<ReceiveOrderInteractionRecipient>
  momInteractionsReceivedReportRecipientFor(
      std::wstring const& federationName,
      std::uint64_t reportedFederateId,
      std::uint64_t receivingFederateId) const;

  // Plans and rechecks the HLArequestDirectedInteractionsReceived /
  // HLAreportDirectedInteractionsReceived MOM interaction pair. The two
  // standard transportation types are represented even when a bucket is
  // empty, which is the MIM-defined NULL response shape.
  [[nodiscard]] MomDirectedInteractionsReceivedReportPlan
  planMomDirectedInteractionsReceivedReport(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t reportedFederateId) const;

  [[nodiscard]] std::optional<ReceiveOrderInteractionRecipient>
  momDirectedInteractionsReceivedReportRecipientFor(
      std::wstring const& federationName,
      std::uint64_t reportedFederateId,
      std::uint64_t receivingFederateId) const;

  // Plans and rechecks the HLArequestReflectionsReceived /
  // HLAreportReflectionsReceived MOM interaction pair. One report payload is
  // created for each supported transportation type, including an empty NULL
  // response bucket, and groups positive reflection counts by registered
  // object class.
  [[nodiscard]] MomReflectionsReceivedReportPlan
  planMomReflectionsReceivedReport(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t reportedFederateId) const;

  [[nodiscard]] std::optional<ReceiveOrderInteractionRecipient>
  momReflectionsReceivedReportRecipientFor(
      std::wstring const& federationName,
      std::uint64_t reportedFederateId,
      std::uint64_t receivingFederateId) const;

  [[nodiscard]] AttributeTransportationTypeChangePlan
  planAttributeTransportationTypeChange(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& attributeHandles,
      std::string transportationName);

  [[nodiscard]] AttributeOrderTypeChangeStatus changeAttributeOrderType(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& attributeHandles,
      rti1516_2025::OrderType orderType);

  [[nodiscard]] std::optional<AttributeTransportationTypeChangeDelivery>
  beginAttributeTransportationTypeChange(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t requestId);

  void cancelAttributeTransportationTypeChange(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t requestId);

  [[nodiscard]] AttributeTransportationTypeDefaultStatus
  changeDefaultAttributeTransportationType(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectClassHandle,
      std::set<std::uint64_t> const& attributeHandles,
      std::string transportationName);

  [[nodiscard]] AttributeOrderTypeDefaultStatus changeDefaultAttributeOrderType(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectClassHandle,
      std::set<std::uint64_t> const& attributeHandles,
      rti1516_2025::OrderType orderType);

  [[nodiscard]] AttributeTransportationTypeQueryPlan
  planAttributeTransportationTypeQuery(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t attributeHandle) const;

  [[nodiscard]] std::optional<AttributeTransportationTypeQueryPlan>
  attributeTransportationTypeQueryFor(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t attributeHandle) const;

  [[nodiscard]] InteractionTransportationTypeChangePlan
  planInteractionTransportationTypeChange(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t interactionClassHandle,
      std::string transportationName);

  [[nodiscard]] InteractionOrderTypeChangeStatus changeInteractionOrderType(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t interactionClassHandle,
      rti1516_2025::OrderType orderType);

  [[nodiscard]] std::optional<std::string>
  beginInteractionTransportationTypeChange(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t interactionClassHandle);

  void cancelInteractionTransportationTypeChange(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t interactionClassHandle);

  [[nodiscard]] InteractionTransportationTypeQueryPlan
  planInteractionTransportationTypeQuery(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t queriedFederateId,
      std::uint64_t interactionClassHandle) const;

  [[nodiscard]] std::optional<InteractionTransportationTypeQueryPlan>
  interactionTransportationTypeQueryFor(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t queriedFederateId,
      std::uint64_t interactionClassHandle) const;

  // Validates and plans the bounded receive-order Send Directed Interaction
  // overload. The producer must know the target object and publish the
  // directed interaction for its registered object class; each recipient
  // must know that object and subscribe to the class-directed pair.
  [[nodiscard]] ReceiveOrderDirectedInteractionPlan
  planReceiveOrderDirectedInteraction(
      std::wstring const& federationName,
      std::uint64_t producingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t sentInteractionClassHandle,
      std::vector<std::uint64_t> const& sentParameterHandles) const;

  [[nodiscard]] std::optional<ReceiveOrderDirectedInteractionRecipient>
 receiveOrderDirectedInteractionRecipientFor(
      std::wstring const& federationName,
      std::uint64_t producingFederateId,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t sentInteractionClassHandle,
      std::vector<std::uint64_t> const& sentParameterHandles) const;

  // Re-evaluates a queued timestamped directed interaction at its callback
  // boundary.  A source that has just been removed by Connection Lost is
  // normally no longer a valid live directed-interaction producer, but 4.4
  // preserves delivery for a marked at-or-before-cutoff payload.  The narrow
  // exception retains the source acceptance snapshot while still enforcing
  // the target's and recipient's current delivery predicates.
  [[nodiscard]] std::optional<ReceiveOrderDirectedInteractionRecipient>
  timestampedDirectedInteractionRecipientFor(
      std::wstring const& federationName,
      std::uint64_t producingFederateId,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t sentInteractionClassHandle,
      std::vector<std::uint64_t> const& sentParameterHandles,
      std::uint64_t messageId) const;

 private:
  [[nodiscard]] RuntimeInstrumentation::Scope beginInstrumentation(
      std::string_view operation) const;

  [[nodiscard]] FederationSaveAdmission admitTimedFederationSaveAtGrantBoundary(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::shared_ptr<rti1516_2025::LogicalTime const> flushQueueGrantedTime);

  struct Federation {
    struct PendingTimeAdvanceGrant {
      std::uint64_t generation = 0;
      // Ordinary requests use zero. A restored request receives a nonzero
      // identity so a stale pre-restore callback cannot consume it.
      std::uint64_t dispatchIdentity = 0;
      FederationTimeGrantDispatch dispatch;
      bool dispatchQueued = false;
    };

    enum class TsoRecipientDeliveryState {
      pending,
      delivered,
      // The queue crossed the recipient's callback boundary, but a current
      // declaration/lifetime check suppressed user-code invocation. This is
      // terminal for payload retention and distinct from a user-visible
      // delivery, so a later Retract cannot request retraction from it.
      suppressed,
      retracted,
    };

    // The queue tracks only temporal fanout. This companion record tracks the
    // original recipients of the supported timestamped message families
    // through the distinct delivery/retraction states required by 8.22/8.23.
    struct TsoRequestRetractionRecord {
      std::uint64_t producingFederateId = 0;
      std::shared_ptr<rti1516_2025::LogicalTime const> timestamp;
      std::map<std::uint64_t, TsoRecipientDeliveryState>
          recipientStates;
      // An execution-owned retraction designator is terminal after the first
      // successful Retract invocation, even when the original fanout was
      // empty (which is possible for a timestamped Delete Object Instance).
      bool retractionApplied = false;
      // A terminal record is intentionally lightweight: the producer id and
      // recipient states survive for public exception/callback classification,
      // while its timestamp and every reclaimable payload may be released.
      bool terminal = false;
      // A voluntary Resign Federation Execution removes the producer before
      // an already accepted directed TSO payload reaches its recipient. Keep
      // this separate from the connection-loss cutoff marker below: voluntary
      // resignation retains every pending accepted recipient, while forced
      // connection loss retains only messages at or before the lost regulator
      // boundary.
      bool producerResigned = false;
      // IEEE 1516.1-2025 connection-loss handling marks a message that was
      // sent at or before a lost time regulator's last-known time. The marker
      // is consumed only to release existing queued TSO delivery after that
      // regulator has been removed; it does not alter ordinary resign or
      // no-regulated-grant policy.
      bool deliveryRequiredAfterConnectionLoss = false;
    };

    struct FederateInteractionDeclarations {
      std::set<std::uint64_t> publishedInteractionClasses;
      std::map<std::uint64_t, bool> subscribedInteractionClasses;
      std::map<std::uint64_t, std::map<std::uint64_t, bool>>
          regionalSubscribedInteractionClasses;
      std::map<std::uint64_t, std::set<std::uint64_t>>
          publishedObjectClassDirectedInteractions;
      // Each directed interaction class has its own subscription kind: false
      // means by ownership, true means universal. This matches the 2025
      // service's per-class optional universal-subscription indicator.
      std::map<std::uint64_t, std::map<std::uint64_t, bool>>
          subscribedObjectClassDirectedInteractions;
      // Per-federate interaction transport overrides affect future ordinary
      // and regional Send Interaction calls. A pending change is committed
      // only at the matching confirmation callback boundary.
      std::map<std::uint64_t, std::string> interactionTransportationTypes;
      // Per-federate interaction order overrides affect future sends by this
      // publisher and do not rewrite another federate's preferred order.
      std::map<std::uint64_t, rti1516_2025::OrderType> interactionOrderTypes;
      std::map<std::uint64_t, std::string>
          pendingInteractionTransportationTypeChanges;
    };

    struct SynchronizationPoint {
      std::vector<unsigned char> userSuppliedTag;
      std::set<std::uint64_t> synchronizationSet;
      std::set<std::uint64_t> announcedFederates;
      std::map<std::uint64_t, bool> achievedFederates;
      // Only a registration without the optional set expands to a late
      // joining federate.  Preserve this distinction through save/restore;
      // an explicitly scoped point must not silently broaden its membership.
      bool lateJoinExpansionAllowed = true;
    };

    struct SaveOperation {
      std::wstring label;
      std::map<std::uint64_t, rti1516_2025::SaveStatus> statuses;
      // Untimed saves may reach constrained members at separate Time Advance
      // Grant callback boundaries. Until each direct callback is delivered,
      // it remains outside the save operation's participant set; the standard
      // permits non-constrained members to be instructed only after every
      // constrained member is eligible.
      std::set<std::uint64_t> pendingTimeConstrainedInitiations;
      bool directTimeConstrainedInitiation = false;
      // A non-null value identifies a timestamped direct-admission operation.
      // Each constrained recipient must have received all queued/in-transit
      // TSO payloads through this time before it can receive its direct save
      // callback.
      std::shared_ptr<rti1516_2025::LogicalTime const> scheduledSaveTime;
    };

    struct PendingImmediateSave {
      std::wstring label;
    };

    struct PendingTimedSave {
      std::wstring label;
      std::shared_ptr<rti1516_2025::LogicalTime const> timestamp;
    };

    struct RestoreOperation {
      std::wstring label;
      std::map<std::uint64_t, rti1516_2025::RestoreStatus> statuses;
      // The canonical durable image is captured at restore admission so the
      // completion boundary rehydrates from the same validated bytes rather
      // than re-reading a mutable store.
      std::optional<FederationStateImage> stateImage;
      // True when admission also found the process-local snapshot. A false
      // value means completion must materialize the route-free image against
      // the current live federation instead.
      bool processLocalSnapshot = false;
    };

    struct ObjectClassAttributeDeclarations {
      struct PerObjectClass {
        std::set<std::uint64_t> explicitlyPublishedAttributes;
        std::map<std::uint64_t, bool> subscribedAttributes;
        std::map<std::uint64_t, std::string> subscribedUpdateRateDesignators;
        std::map<std::uint64_t, std::map<std::uint64_t, bool>>
            regionalSubscribedAttributes;
        std::map<std::uint64_t, std::map<std::uint64_t, std::string>>
            regionalSubscribedUpdateRateDesignators;
        // A class default is scoped to one federate's future instance
        // attributes. Existing object instances retain their captured value.
        std::map<std::uint64_t, std::string> defaultTransportationTypes;
        std::map<std::uint64_t, rti1516_2025::OrderType> defaultOrderTypes;
        bool privilegeToDeleteExplicitlyUnpublished = false;
      };

      std::map<std::uint64_t, PerObjectClass> byObjectClass;
      std::uint64_t subscriptionGeneration = 0;
    };

    struct Region {
      std::uint64_t ownerFederateId = 0;
      std::set<std::uint64_t> dimensionHandles;
      std::map<std::uint64_t, RegionRangeBounds> pendingRangeBounds;
      std::map<std::uint64_t, RegionRangeBounds> committedRangeBounds;
      bool specificationCommitted = false;
      bool inUse = false;
    };

      struct ObjectInstance {
      struct PendingAttributeOwnershipAcquisitionIfAvailable {
        std::uint64_t requestingFederateId = 0;
        std::uint64_t requestSequence = 0;
        std::set<std::uint64_t> desiredAttributeHandles;
        std::vector<unsigned char> userSuppliedTag;
      };

      struct PendingAttributeOwnershipAcquisition {
        std::uint64_t requestingFederateId = 0;
        std::uint64_t requestSequence = 0;
        std::set<std::uint64_t> desiredAttributeHandles;
        std::set<std::uint64_t> notificationQueuedAttributeHandles;
        // A release-denied response is terminal for the acquisition, but its
        // official Attribute Ownership Unavailable callback may still be
        // queued. Keep the attribute in the request until that callback's
        // delivery boundary so a concurrent cancellation can win the race.
        std::set<std::uint64_t> unavailableQueuedAttributeHandles;
        std::map<std::uint64_t, std::set<std::uint64_t>>
            releaseCallbacksQueuedByOwningFederate;
        std::vector<unsigned char> userSuppliedTag;
      };

      struct PendingAttributeOwnershipAcquisitionCancellation {
        std::uint64_t requestingFederateId = 0;
        std::set<std::uint64_t> attributeHandles;
      };

      struct PendingAttributeOwnershipDivestitureIfWantedNotification {
        std::uint64_t receivingFederateId = 0;
        std::set<std::uint64_t> attributeHandles;
        std::vector<unsigned char> userSuppliedTag;
      };

      // A negotiated divestiture leaves the owner in the private Waiting for
      // a New Owner to be Found state. The selected regular or If Available
      // acquisition is retained until Confirm Divestiture, so a stale
      // confirmation callback never transfers ownership by itself.
      struct PendingNegotiatedAttributeOwnershipDivestiture {
        std::uint64_t divestingFederateId = 0;
        std::uint64_t acquiringFederateId = 0;
        std::uint64_t acquisitionRequestId = 0;
        bool acquiringFederateIsIfAvailable = false;
        bool confirmationQueued = false;
        bool confirmationDelivered = false;
        std::vector<unsigned char> userSuppliedTag;
      };

      struct PendingConfirmDivestitureNotification {
        std::uint64_t receivingFederateId = 0;
        std::set<std::uint64_t> attributeHandles;
        std::vector<unsigned char> userSuppliedTag;
      };

      struct PendingAttributeTransportationTypeChange {
        std::uint64_t requestingFederateId = 0;
        std::set<std::uint64_t> attributeHandles;
        std::string transportationName;
      };

      std::uint64_t handle = 0;
      std::wstring name;
      std::uint64_t registeredObjectClassHandle = 0;
      std::uint64_t producingFederateId = 0;
      // Registration seeds every currently published attribute with the
      // producing federate as owner. Keeping the owner per attribute avoids a
      // second state model as the initial 2025 ownership slices are added.
      std::map<std::uint64_t, std::uint64_t> attributeOwnersByHandle;
      // The effective type is captured for every owned attribute at
      // registration/ownership-transfer time, so later class-default changes
      // cannot rewrite existing instances.
      std::map<std::uint64_t, std::string> attributeTransportationTypes;
      // Preferred order is likewise captured per instance attribute. An
      // ownership transfer resets it from the acquiring federate's class
      // default before the ownership notification is delivered.
      std::map<std::uint64_t, rti1516_2025::OrderType> attributeOrderTypes;
      // Explicit region associations used by the no-time Update Attribute
      // Values path for the current owner. Missing entries retain the ordinary
      // no-region behavior. Associations made by a federate that does not
      // currently own an attribute are retained separately until that
      // federate acquires ownership.
      std::map<std::uint64_t, std::set<std::uint64_t>> updateRegionsByAttribute;
      std::map<
          std::uint64_t,
          std::map<std::uint64_t, std::set<std::uint64_t>>>
          deferredUpdateRegionsByFederate;
      // Latest accepted application values. Receive-order updates commit at
      // admission; timestamped updates commit at their callback/reclaim
      // boundary so a retracted payload never becomes current state.
      std::map<std::uint64_t, rti1516_2025::VariableLengthData> attributeValues;
      struct PendingAttributeValueUpdateRequest {
        std::uint64_t requestingFederateId = 0;
        std::uint64_t providingFederateId = 0;
        std::set<std::uint64_t> requestedAttributeHandles;
        std::vector<unsigned char> userSuppliedTag;
      };
      struct PendingAttributeValueUpdateClassRequest {
        std::uint64_t requestingFederateId = 0;
        std::uint64_t providingFederateId = 0;
        std::uint64_t requestedObjectClassHandle = 0;
        std::set<std::uint64_t> requestedAttributeHandles;
        std::vector<unsigned char> userSuppliedTag;
      };
      struct PendingAttributeValueUpdateRegionalRequest {
        std::uint64_t requestingFederateId = 0;
        std::uint64_t providingFederateId = 0;
        std::uint64_t requestedObjectClassHandle = 0;
        std::set<std::uint64_t> requestedAttributeHandles;
        std::map<std::uint64_t, std::set<std::uint64_t>>
            requestRegionsByAttribute;
        std::vector<unsigned char> userSuppliedTag;
      };
      // Accepted object-instance Request Attribute Value Update callbacks
      // remain here until their provider callback reaches its delivery
      // boundary. The callback route itself is live-only and rebound on a
      // fresh-registry restore.
      std::map<std::uint64_t, PendingAttributeValueUpdateRequest>
          pendingAttributeValueUpdateRequests;
      // Accepted object-class Request Attribute Value Update callbacks are
      // retained per expanded object/provider delivery until their provider
      // callback reaches its one-shot begin boundary.
      std::map<std::uint64_t, PendingAttributeValueUpdateClassRequest>
          pendingAttributeValueUpdateClassRequests;
      std::map<std::uint64_t, PendingAttributeValueUpdateRegionalRequest>
          pendingAttributeValueUpdateRegionalRequests;
      std::map<std::uint64_t, PendingAttributeOwnershipAcquisitionIfAvailable>
          pendingAttributeOwnershipAcquisitionIfAvailableRequests;
      std::map<std::uint64_t, PendingAttributeOwnershipAcquisition>
          pendingAttributeOwnershipAcquisitionRequests;
      std::map<std::uint64_t, PendingAttributeOwnershipAcquisitionCancellation>
          pendingAttributeOwnershipAcquisitionCancellations;
      std::map<std::uint64_t,
               PendingAttributeOwnershipDivestitureIfWantedNotification>
          pendingAttributeOwnershipDivestitureIfWantedNotifications;
      // For each attribute made unowned by an unconditional divestiture or
      // resign action, retain the federates that have already received an
      // assumption offer.  A later join, discovery, or publication can then
      // continue the search without duplicating the same callback.
      std::map<std::uint64_t, std::set<std::uint64_t>>
          ownershipAssumptionRecipientsByAttribute;
      // Preserve the source tag for each still-unowned assumption search.
      // Resign-action searches use an empty value; unconditional divestiture
      // stores the accepted service tag for later continuation callbacks.
      std::map<std::uint64_t, std::vector<unsigned char>>
          ownershipAssumptionUserSuppliedTagsByAttribute;
      // Callback routes are process-local. Retain only queued assumption
      // callback identities so a fresh-registry restore can rebind them to
      // the current federate ambassadors without replaying callbacks that
      // already crossed their begin boundary.
      std::vector<PendingAttributeOwnershipAssumptionCallback>
          pendingAttributeOwnershipAssumptionCallbacks;
      std::map<std::uint64_t, PendingNegotiatedAttributeOwnershipDivestiture>
          pendingNegotiatedAttributeOwnershipDivestitures;
      std::map<std::uint64_t, PendingConfirmDivestitureNotification>
          pendingConfirmDivestitureNotifications;
      std::map<std::uint64_t, PendingAttributeTransportationTypeChange>
          pendingAttributeTransportationTypeChanges;
      std::map<std::uint64_t, std::uint64_t> knownObjectClassHandlesByFederate;
      std::set<std::uint64_t> pendingDiscoveryFederates;
      bool deleteAccepted = false;
      std::set<std::uint64_t> pendingRemovalFederates;
      // These sets classify only no-time Remove Object Instance work created
      // by a forced connection-loss automatic-resign action. The first records
      // its source; the second records a callback that reached its delivery
      // boundary while an at-or-before-cutoff object-dependent TSO payload
      // still required the recipient's known-instance state.
      std::set<std::uint64_t> connectionLossAutomaticRemovalFederates;
      std::set<std::uint64_t> deferredConnectionLossTsoRemovalFederates;
      std::optional<std::uint64_t> pendingTimestampedDeletionMessageId;
      std::set<std::uint64_t> pendingTimestampedRemovalFederates;
    };

    // A timestamped Delete Object Instance is unusual among the currently
    // supported retractable message families: the standard requires a legal
    // post-delivery Retract to reconstitute the object and reassume its
    // invocation-time ownership.  Keep that state execution-owned, rather
    // than trying to reconstruct it from recipient callbacks after the
    // original object has been deleted.
    struct TsoObjectDeletionReconstitutionRecord {
      ObjectInstance objectInstanceAtInvocation;
    };

    FederationDefinition definition;
    // This is deliberately independent from every private handle directory.
    // IEEE 1516.1 normalized values must be RTI-originated rather than a
    // promise that their values follow the implementation's handle sequence.
    std::uint64_t normalizationSeed = 0;
    // Auto Provide is a federation-wide dynamic switch. The current private
    // runtime seeds it from the creation FDD; MOM switch adjustment is a
    // separate service path and is not silently inferred from an additional
    // FOM join.
    bool autoProvideSwitch = false;
    // Advisories Use Known Class is a static, federation-wide switch. It is
    // captured at federation creation and survives additional-FOM joins and
    // save/restore as one value shared by every current member.
    bool advisoriesUseKnownClassSwitch = false;
    // Non-Regulated Grant has the same static federation-wide lifetime as
    // Advisories Use Known Class and is consumed by the time-bound calculator.
    bool nonRegulatedGrantSwitch = false;
    // Static FDD switches retained for the lifetime of the execution.
    bool delaySubscriptionEvaluationSwitch = false;
    bool allowRelaxedDDMSwitch = false;
    std::shared_ptr<ObjectClassHandleDirectory const> objectClassHandles;
    std::shared_ptr<AttributeHandleDirectory const> attributeHandles;
    std::shared_ptr<InteractionClassHandleDirectory const> interactionClassHandles;
    std::shared_ptr<ParameterHandleDirectory const> parameterHandles;
    std::shared_ptr<DimensionHandleDirectory const> dimensionHandles;
    std::shared_ptr<TransportationTypeHandleDirectory const> transportationTypeHandles;
    std::map<std::uint64_t, FederateMembership> members;
    std::map<std::wstring, std::uint64_t> memberIdsByName;
    // Unlike memberIdsByName, this lifetime index is not removed at resign.
    // IEEE 1516.1 defines a valid federate designator as one returned by Join
    // during this execution, even if the federate is no longer joined.
    std::map<std::uint64_t, std::wstring> federateNamesById;
    std::map<std::uint64_t, FederateInteractionDeclarations> interactionDeclarations;
    std::map<std::wstring, SynchronizationPoint> synchronizationPoints;
    std::optional<SaveOperation> saveOperation;
    std::optional<PendingImmediateSave> pendingImmediateSave;
    std::optional<PendingTimedSave> pendingTimedSave;
    // Federation-scoped save history is part of the execution MOM, not a
    // callback-derived adapter cache. An absent time means the corresponding
    // save was untimed (or has not occurred yet).
    std::wstring lastSaveName;
    std::shared_ptr<rti1516_2025::LogicalTime const> lastSaveTime;
    std::wstring nextSaveName;
    std::shared_ptr<rti1516_2025::LogicalTime const> nextSaveTime;
    std::optional<RestoreOperation> restoreOperation;
    std::map<std::uint64_t, ObjectClassAttributeDeclarations> objectClassAttributeDeclarations;
    std::map<std::uint64_t, InteractionCallbackRoute> interactionCallbackRoutes;
    std::map<std::uint64_t, FederateServiceReportRoute> serviceReportRoutes;
    std::map<std::uint64_t, FederatePublicServiceReportRoute>
        publicServiceReportRoutes;
    struct PendingAttributeOwnershipQuery {
      std::uint64_t requestingFederateId = 0;
      std::uint64_t objectInstanceHandle = 0;
      AttributeOwnershipQueryReportKind reportKind =
          AttributeOwnershipQueryReportKind::unowned;
      std::uint64_t owningFederateId = 0;
      std::set<std::uint64_t> requestedAttributeHandles;
    };
    // Accepted Query Attribute Ownership callbacks remain here until their
    // one-shot callback boundary. The route itself is live-only and rebound
    // on a fresh-registry restore.
    std::map<std::uint64_t, PendingAttributeOwnershipQuery>
        pendingAttributeOwnershipQueries;
    // Like callback routes, grant factories are live binding endpoints and
    // are preserved from the current ambassadors when state is restored.
    std::map<std::uint64_t, FederationTimeGrantDispatchFactory>
        timeAdvanceGrantDispatchFactories;
    std::map<std::uint64_t, FederationTimeRoleEnableDispatchFactory>
        timeRoleEnableDispatchFactories;
    // This monotonically increases for every production grant dispatch and
    // never rolls back with a snapshot, invalidating pre-restore callback
    // work even when the saved time-generation value is later reused.
    std::uint64_t nextTimeAdvanceGrantDispatchIdentity = 1;
    std::set<std::pair<std::uint64_t, std::uint64_t>>
        objectClassRegistrationRelevance;
    std::set<std::pair<std::uint64_t, std::uint64_t>> interactionRelevance;
    FederationTimeCoordinator timeCoordinator;
    std::map<std::uint64_t, TsoInteractionMessage> tsoInteractionMessages;
    std::map<std::uint64_t, TsoRequestRetractionRecord>
        tsoRequestRetractionRecords;
    std::map<std::uint64_t, TsoAttributeUpdateMessage>
        tsoAttributeUpdateMessages;
    std::map<std::uint64_t, TsoObjectDeletionMessage>
        tsoObjectDeletionMessages;
    std::map<std::uint64_t, TsoObjectDeletionReconstitutionRecord>
        tsoObjectDeletionReconstitutionRecords;
    std::map<std::uint64_t, TsoDirectedInteractionMessage>
        tsoDirectedInteractionMessages;
    std::map<std::uint64_t, PendingTimeAdvanceGrant> pendingTimeAdvanceGrants;
    std::map<std::uint64_t, Region> regions;
    // RTI-created joined-federate MOM objects reserve common object-instance
    // handle values but are not inserted into objectInstances. The ordinary
    // object-management paths therefore cannot accidentally treat them as
    // federate-produced, transfer-capable instances before the RTI-owned
    // callback protocol is implemented.
    std::map<std::uint64_t, JoinedFederateMomObjectSnapshot>
        rtiOwnedJoinedFederateMomObjects;
    std::map<std::uint64_t, ObjectInstance> objectInstances;
    std::map<std::wstring, std::uint64_t> objectInstanceHandlesByName;
    std::map<std::wstring, std::uint64_t> reservedObjectInstanceNamesByFederate;
    std::uint64_t nextRegionHandle = 1;
    std::uint64_t nextSubscriptionGeneration = 1;
    std::uint64_t nextObjectInstanceHandle = 1;
    std::uint64_t nextAttributeOwnershipAcquisitionIfAvailableRequestId = 1;
    std::uint64_t nextAttributeOwnershipAcquisitionRequestId = 1;
    std::uint64_t nextAttributeOwnershipAcquisitionRequestSequence = 1;
    std::uint64_t nextAttributeOwnershipAcquisitionCancellationId = 1;
    std::uint64_t nextAttributeOwnershipDivestitureIfWantedNotificationId = 1;
    std::uint64_t nextConfirmDivestitureNotificationId = 1;
    std::uint64_t nextAttributeTransportationTypeChangeRequestId = 1;
    std::uint64_t nextAttributeValueUpdateRequestId = 1;
    std::uint64_t nextAttributeOwnershipQueryRequestId = 1;
  };

  [[nodiscard]] static bool isFirstPendingAttributeOwnershipAcquisition(
      Federation::ObjectInstance const& instance,
      std::uint64_t requestId,
      std::uint64_t attributeHandle);
  // An ownership-assumption search is scoped to one divestiture offer. Once an
  // attribute becomes owned, a later divestiture starts a fresh search and
  // must not inherit the prior search's recipient reservations or tag. A
  // negotiated offer is the one exception to the usual unowned interval: its
  // owner remains owner while the assumption callbacks are outstanding.
  static void clearOwnershipAssumptionSearch(
      Federation::ObjectInstance& instance,
      std::uint64_t attributeHandle);
  [[nodiscard]] static std::vector<AttributeOwnershipAssumptionRecipient>
  planAttributeOwnershipAssumptionsForFederateLocked(
      Federation& federation,
      std::uint64_t receivingFederateId,
      std::optional<std::uint64_t> objectInstanceFilter = std::nullopt,
      std::set<std::uint64_t> const* attributeFilter = nullptr);
  [[nodiscard]] static std::vector<AttributeOwnershipAcquisitionWorkItem>
  planPendingAttributeOwnershipAcquisitionWork(
      Federation& federation,
      Federation::ObjectInstance& instance);
  static void clearPendingAttributeOwnershipQueries(
      Federation& federation,
      std::uint64_t objectInstanceHandle);
  [[nodiscard]] static std::vector<AttributeOwnershipAcquisitionWorkItem>
  planPendingNegotiatedAttributeOwnershipDivestitureConfirmations(
      Federation& federation,
      Federation::ObjectInstance& instance);

  [[nodiscard]] static std::optional<ReceiveOrderInteractionRecipient>
  candidateReceiveOrderInteractionRecipient(
      Federation const& federation,
      InteractionProducer const& producingSource,
      std::uint64_t receivingFederateId,
      std::uint64_t sentInteractionClassHandle,
      std::vector<std::uint64_t> const& sentParameterHandles,
      std::set<std::uint64_t> const* sentRegionHandles = nullptr,
      std::map<std::uint64_t, RegionSpecificationSnapshot> const* regionOverrides = nullptr);
  [[nodiscard]] static MomServiceReportRoutingPlan momServiceReportRoutingPlanFor(
      Federation const& federation,
      std::uint64_t reportedFederateId,
      std::uint16_t serviceGroup);
  [[nodiscard]] static std::optional<FederateLostReportRouting>
  federateLostReportRoutingFor(
      Federation const& federation,
      std::uint64_t reportedFederateId);
  [[nodiscard]] static std::optional<ExceptionReportRouting>
  exceptionReportRoutingFor(
      Federation const& federation,
      std::uint64_t reportedFederateId);
  [[nodiscard]] static bool validDirectedInteractionForObjectClass(
      Federation const& federation,
      std::uint64_t objectClassHandle,
      std::uint64_t interactionClassHandle);
  [[nodiscard]] static bool directedInteractionDeclarationApplies(
      Federation const& federation,
      std::uint64_t federateId,
      std::uint64_t registeredObjectClassHandle,
      std::uint64_t interactionClassHandle,
      bool publication);
  [[nodiscard]] static std::optional<bool>
  directedInteractionSubscriptionIsUniversal(
      Federation const& federation,
      std::uint64_t federateId,
      std::uint64_t registeredObjectClassHandle,
      std::uint64_t interactionClassHandle);
  [[nodiscard]] static std::optional<ReceiveOrderDirectedInteractionRecipient>
  candidateReceiveOrderDirectedInteractionRecipient(
      Federation const& federation,
      std::uint64_t producingFederateId,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t sentInteractionClassHandle,
      std::vector<std::uint64_t> const& sentParameterHandles,
      bool allowMissingProducingFederate = false);
  [[nodiscard]] static std::optional<ReceiveOrderAttributeUpdateRecipient>
  candidateReceiveOrderAttributeUpdateRecipient(
      Federation const& federation,
      std::uint64_t producingFederateId,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle,
      std::vector<std::uint64_t> const& sentAttributeHandles,
      std::set<std::uint64_t> const* sentRegionHandles,
      std::map<std::uint64_t, RegionSpecificationSnapshot> const* regionOverrides = nullptr);
  [[nodiscard]] static std::optional<AttributeValueUpdateProvideRecipient>
  candidateAttributeValueUpdateProvideRecipient(
      Federation const& federation,
      std::uint64_t requestingFederateId,
      std::uint64_t providingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& requestedAttributeHandles);
  [[nodiscard]] static std::optional<AttributeValueUpdateProvideRecipient>
  candidateAttributeValueUpdateClassProvideRecipient(
      Federation const& federation,
      std::uint64_t requestingFederateId,
      std::uint64_t providingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t requestedObjectClassHandle,
      std::set<std::uint64_t> const& requestedAttributeHandles,
      std::map<std::uint64_t, std::set<std::uint64_t>> const* requestRegionsByAttribute =
          nullptr);
  [[nodiscard]] static std::optional<AttributeOwnershipQueryRecipient>
  candidateAttributeOwnershipQueryRecipient(
      Federation const& federation,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      AttributeOwnershipQueryReportKind reportKind,
      std::uint64_t owningFederateId,
      std::set<std::uint64_t> const& requestedAttributeHandles);
  [[nodiscard]] static bool objectInstanceRegisteredAtOrBelowClass(
      Federation const& federation,
      std::uint64_t registeredObjectClassHandle,
      std::uint64_t requestedObjectClassHandle);

  [[nodiscard]] static bool validDefinition(
      std::wstring const& federationName,
      FederationDefinition const& definition);
  [[nodiscard]] static unsigned long normalizedHandleValue(
      std::uint64_t normalizationSeed,
      std::uint64_t handleValue,
      std::uint64_t handleKind) noexcept;

  FederationJoinResult joinImpl(
      std::wstring const& federationName,
      FederationDefinition* replacementDefinition,
      std::shared_ptr<FederateTimeState> timeState,
      std::wstring const& federateType,
      std::optional<std::wstring> requestedFederateName,
      InteractionCallbackRoute interactionCallbackRoute,
      FederationTimeGrantDispatchFactory timeAdvanceGrantDispatchFactory,
      FederationTimeRoleEnableDispatchFactory timeRoleEnableDispatchFactory);

  [[nodiscard]] static std::optional<FederationTimeExecutionSnapshot> makeTimeSnapshot(
      Federation const& federation);

  [[nodiscard]] static bool validInteractionClass(
      Federation const& federation,
      std::uint64_t interactionClassHandle);
  [[nodiscard]] static bool isReportServiceInvocationInteractionClass(
      Federation const& federation,
      std::uint64_t interactionClassHandle);
  [[nodiscard]] static bool hasReportServiceInvocationSubscription(
      Federation const& federation,
      std::uint64_t federateId);
  [[nodiscard]] static std::optional<std::set<std::uint64_t>>
  availableInteractionDimensions(
      Federation const& federation,
      std::uint64_t interactionClassHandle);
  [[nodiscard]] static std::optional<std::set<std::uint64_t>>
  availableObjectClassDimensions(
      Federation const& federation,
      std::uint64_t objectClassHandle);
  [[nodiscard]] static bool regionsOverlap(
      Federation const& federation,
      std::uint64_t firstRegionHandle,
      std::uint64_t secondRegionHandle);
  // Applies strict IEEE 1516.1-2025 range overlap and Umbra's documented
  // Allow Relaxed DDM policy. With the static federation switch enabled,
  // exactly touching committed ranges qualify; a strict overlap always
  // qualifies and a nonzero gap never does.
  [[nodiscard]] static bool regionsOverlap(
      Federation const& federation,
      std::uint64_t firstRegionHandle,
      std::uint64_t secondRegionHandle,
      std::map<std::uint64_t, RegionSpecificationSnapshot> const* overrides);
  [[nodiscard]] static bool regionSnapshotsOverlap(
      Federation const& federation,
      RegionSpecificationSnapshot const& first,
      RegionSpecificationSnapshot const& second);
  // The RTI-owned HLAfederate object has an immutable point region rather
  // than a public RegionHandle.  Regional MOM discovery compares a
  // subscriber's committed region with that snapshot using the same strict
  // overlap and Allow Relaxed DDM policy as ordinary object routing.
  [[nodiscard]] static bool regionOverlapsSnapshot(
      Federation const& federation,
      std::uint64_t regionHandle,
      RegionSpecificationSnapshot const& snapshot);
  // The standard default region spans every FDD dimension and cannot be
  // referenced by a federate.  This predicate preserves that invisible
  // relationship without allocating a synthetic public RegionHandle.  An
  // explicit realization with no dimensions still cannot overlap it.
  [[nodiscard]] static bool regionOverlapsDefault(
      Federation const& federation,
      std::uint64_t regionHandle,
      std::map<std::uint64_t, RegionSpecificationSnapshot> const* overrides = nullptr);
  // An explicit update-region association belongs to the current owner of an
  // instance attribute.  IEEE 1516.1-2025 removes that association whenever
  // ownership is divested, transferred, or otherwise lost; it is not carried
  // forward to a later owner.
  static void clearUpdateRegionAssociation(
      Federation::ObjectInstance& objectInstance,
      std::uint64_t attributeHandle) noexcept;
  static void promoteDeferredUpdateRegionAssociation(
      Federation::ObjectInstance& objectInstance,
      std::uint64_t federateId,
      std::uint64_t attributeHandle) noexcept;
  static void refreshRegionUsage(Federation& federation);

  [[nodiscard]] static bool validObjectClass(
      Federation const& federation,
      std::uint64_t objectClassHandle);
  [[nodiscard]] static bool validObjectClassAttributes(
      Federation const& federation,
      std::uint64_t objectClassHandle,
      std::set<std::uint64_t> const& attributeHandles);
  [[nodiscard]] static std::optional<std::string> attributeTransportationName(
      Federation const& federation,
      std::uint64_t objectClassHandle,
      std::uint64_t attributeHandle);
  [[nodiscard]] static std::optional<rti1516_2025::OrderType>
  orderTypeFromName(std::string const& orderName);
  [[nodiscard]] static std::optional<rti1516_2025::OrderType>
  attributeDefaultOrderType(
      Federation const& federation,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::uint64_t attributeHandle);
  [[nodiscard]] static std::optional<rti1516_2025::OrderType>
  effectiveAttributeOrderType(
      Federation const& federation,
      Federation::ObjectInstance const& objectInstance,
      std::uint64_t attributeHandle);
  [[nodiscard]] static std::optional<rti1516_2025::OrderType>
  interactionOrderType(
      Federation const& federation,
      std::uint64_t federateId,
      std::uint64_t interactionClassHandle);
  [[nodiscard]] static std::optional<std::string>
  attributeDefaultTransportationName(
      Federation const& federation,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::uint64_t attributeHandle);
  [[nodiscard]] static std::optional<std::string>
  effectiveAttributeTransportationName(
      Federation const& federation,
      Federation::ObjectInstance const& objectInstance,
      std::uint64_t attributeHandle);
  [[nodiscard]] static std::optional<std::string>
  effectiveInteractionTransportationName(
      Federation const& federation,
      std::uint64_t federateId,
      std::uint64_t interactionClassHandle);
  [[nodiscard]] static std::optional<std::set<std::uint64_t>> publishedObjectClassAttributes(
      Federation const& federation,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle);
  [[nodiscard]] static std::optional<std::uint64_t> candidateObjectInstanceDiscoveryClass(
      Federation const& federation,
      Federation::ObjectInstance const& objectInstance,
      std::uint64_t receivingFederateId,
      std::map<std::uint64_t, RegionSpecificationSnapshot> const* regionOverrides = nullptr,
      std::map<std::uint64_t, std::set<std::uint64_t>> const* updateRegionOverrides = nullptr);
  [[nodiscard]] static std::optional<std::uint64_t>
  candidateJoinedFederateMomObjectDiscoveryClass(
      Federation const& federation,
      JoinedFederateMomObjectSnapshot const& object,
      std::uint64_t receivingFederateId);
  [[nodiscard]] static std::optional<std::map<std::uint64_t,
                                               rti1516_2025::VariableLengthData>>
  joinedFederateMomObjectAttributeValues(
      Federation const& federation,
      JoinedFederateMomObjectSnapshot const& object,
      std::uint64_t receivingFederateId,
      std::set<std::uint64_t> const& requestedAttributeHandles,
      bool requireActiveSubscription = false,
      std::map<std::uint64_t, std::set<std::uint64_t>> const*
          requestRegionsByAttribute = nullptr);
  [[nodiscard]] static std::optional<rti1516_2025::VariableLengthData>
  joinedFederateMomObjectAttributeValue(
      Federation const& federation,
      JoinedFederateMomObjectSnapshot const& object,
      std::uint64_t attributeHandle);
  [[nodiscard]] static bool objectAttributeInScope(
      Federation const& federation,
      Federation::ObjectInstance const& objectInstance,
      std::uint64_t receivingFederateId,
      std::uint64_t attributeHandle,
      std::map<std::uint64_t, RegionSpecificationSnapshot> const* overrides = nullptr,
      std::map<std::uint64_t, std::set<std::uint64_t>> const* associationOverrides = nullptr,
      Federation::ObjectClassAttributeDeclarations const* declarationOverrides = nullptr);
  // Attribute relevance advisories have one deliberate fork in the 2025
  // model.  With Advisories Use Known Class enabled they follow the actual
  // known-class scope calculation above.  With it disabled they are based on
  // the subscribing federate's retained subscriptions from the registered
  // class lineage, even when the known class cannot reflect the attribute.
  [[nodiscard]] static bool objectAttributeRelevantForAdvisory(
      Federation const& federation,
      Federation::ObjectInstance const& objectInstance,
      std::uint64_t receivingFederateId,
      std::uint64_t attributeHandle,
      std::map<std::uint64_t, RegionSpecificationSnapshot> const* overrides = nullptr,
      std::map<std::uint64_t, std::set<std::uint64_t>> const* associationOverrides = nullptr,
      Federation::ObjectClassAttributeDeclarations const* declarationOverrides = nullptr);
  struct AttributeRelevanceRateSnapshot {
    bool relevant = false;
    std::optional<std::string> updateRateDesignator;
  };
  // Evaluate one object's attribute across every known non-owner receiver.
  // `declarationOverrideFederateId` identifies the receiver whose prior
  // declaration snapshot is supplied for a subscription mutation; a zero
  // value means that all receivers use their live declarations.  Region and
  // association overrides describe the state being evaluated.
  [[nodiscard]] static AttributeRelevanceRateSnapshot
  attributeRelevanceRateSnapshotForState(
      Federation const& federation,
      Federation::ObjectInstance const& objectInstance,
      std::uint64_t declarationOverrideFederateId,
      std::uint64_t attributeHandle,
      std::map<std::uint64_t, RegionSpecificationSnapshot> const* regionOverrides = nullptr,
      std::map<std::uint64_t, std::set<std::uint64_t>> const* associationOverrides = nullptr,
      Federation::ObjectClassAttributeDeclarations const* declarationOverrides = nullptr);
  [[nodiscard]] static std::optional<std::string>
  subscribedUpdateRateDesignatorForAttribute(
      Federation const& federation,
      Federation::ObjectInstance const& objectInstance,
      std::uint64_t receivingFederateId,
      std::uint64_t attributeHandle,
      std::map<std::uint64_t, RegionSpecificationSnapshot> const* regionOverrides = nullptr,
      std::map<std::uint64_t, std::set<std::uint64_t>> const* associationOverrides = nullptr,
      Federation::ObjectClassAttributeDeclarations const* declarationOverrides = nullptr);
  [[nodiscard]] static std::vector<ObjectInstanceScopeChangeRecipient>
  objectInstanceScopeChangesForAssociation(
      Federation const& federation,
      Federation::ObjectInstance const& objectInstance,
      std::set<std::uint64_t> const& attributeHandles,
      std::map<std::uint64_t, std::set<std::uint64_t>> const& associationOverrides);
  [[nodiscard]] static std::vector<ObjectInstanceScopeChangeRecipient>
  objectInstanceScopeChangesForSubscription(
      Federation const& federation,
      std::uint64_t receivingFederateId,
      std::set<std::uint64_t> const& attributeHandles,
      Federation::ObjectClassAttributeDeclarations const& previousDeclarations);
  [[nodiscard]] static std::vector<AttributeRelevanceAdvisoryRecipient>
  attributeRelevanceAdvisoriesForScopeChanges(
      Federation const& federation,
      std::vector<ObjectInstanceScopeChangeRecipient> const& scopeChanges);
  // When the static known-class switch is disabled, actual scope changes are
  // not a sufficient trigger: a subscription may become relevant for an
  // already-known instance even though the known class cannot reflect it.
  // Compare the pre/post state for one mutation family and build the same
  // owner-directed advisory records without contaminating Attribute Scope
  // callbacks with the advisory-only transition.
  // The comparison always evaluates every known non-owner receiver so one
  // owner-directed callback represents the federation-wide relevance edge.
  // A nonzero receivingFederateId identifies the receiver whose prior
  // declaration snapshot is supplied for a subscription mutation; zero means
  // all receivers use their live declarations. Association transitions may
  // supply both the pre-mutation and staged post-mutation association maps;
  // other mutation families leave the latter null and use live state.
  [[nodiscard]] static std::vector<AttributeRelevanceAdvisoryRecipient>
  attributeRelevanceAdvisoriesForTransitions(
      Federation const& federation,
      std::uint64_t receivingFederateId,
      std::set<std::uint64_t> const& attributeHandles,
      std::map<std::uint64_t, RegionSpecificationSnapshot> const* previousRegions = nullptr,
      std::map<std::uint64_t, std::set<std::uint64_t>> const* previousAssociations = nullptr,
      std::map<std::uint64_t, std::set<std::uint64_t>> const* currentAssociations = nullptr,
      Federation::ObjectClassAttributeDeclarations const* previousDeclarations = nullptr,
      std::optional<std::uint64_t> objectInstanceHandle = std::nullopt);
  [[nodiscard]] static std::optional<KnownObjectInstanceSnapshot> knownObjectInstanceSnapshot(
      Federation const& federation,
      Federation::ObjectInstance const& objectInstance,
      std::uint64_t federateId);
  [[nodiscard]] static bool canPurgeDeletedObjectInstance(
      Federation::ObjectInstance const& objectInstance) noexcept;
  [[nodiscard]] static bool hasPendingConnectionLossTsoObjectDelivery(
      Federation const& federation,
      std::uint64_t objectInstanceHandle,
      std::uint64_t receivingFederateId) noexcept;

  [[nodiscard]] static bool hasPendingTsoRecipient(
      Federation const& federation,
      Federation::TsoRequestRetractionRecord const& record) noexcept;
  [[nodiscard]] static std::optional<std::shared_ptr<rti1516_2025::LogicalTime const>>
  earliestConnectionLossTsoDeliveryBoundary(
      Federation const& federation,
      std::uint64_t recipientFederateId);
  [[nodiscard]] static bool connectionLossTsoDeliveryMayGrant(
      Federation const& federation,
      std::uint64_t recipientFederateId,
      FederateTimeSnapshot const& requester,
      FederationTimeBounds const& bounds);
  static void reclaimTsoMessagePayload(
      Federation& federation,
      std::uint64_t messageId);
  static void reclaimTsoMessagePayloads(Federation& federation);
  static void applyTsoAttributeUpdateValues(
      Federation& federation,
      std::uint64_t messageId);

  [[nodiscard]] static std::vector<FederationTimeGrantDispatch> scheduleEligibleTimeAdvanceGrants(
      Federation& federation);

  // Rebind callback-gated role-enable requests after the route-free temporal
  // image has been applied. Factories capture the target state's fresh
  // callback epoch when these dispatches are created.
  [[nodiscard]] static std::vector<FederationTimeGrantDispatch>
  scheduleRestoredTimeRoleEnableDispatches(Federation& federation);

  // Starts a federation-wide save after all admission/readiness checks have
  // passed.  The caller owns mutex_ and supplies a result whose callbacks are
  // submitted only after that lock is released.
  [[nodiscard]] static bool startFederationSave(
      Federation& federation,
      std::wstring label,
      FederationSaveControlResult& result,
      std::shared_ptr<rti1516_2025::LogicalTime const> timestamp = nullptr);

  static void appendSaveCompletionNotifications(
      Federation& federation,
      bool successful,
      rti1516_2025::SaveFailureReason failureReason,
      std::vector<FederationSaveNotification>& notifications,
      std::optional<std::uint64_t> excludedFederateId = std::nullopt);

  static void appendRestoreCompletionNotifications(
      Federation& federation,
      bool successful,
      rti1516_2025::RestoreFailureReason failureReason,
      std::vector<FederationRestoreNotification>& notifications,
      std::optional<std::uint64_t> excludedFederateId = std::nullopt);

  // Builds the route-free, versioned payload that accompanies a durable save
  // commit.  Live callback endpoints remain outside the image and are
  // rebound by restoreFederationFromSnapshot from the current ambassadors.
  [[nodiscard]] static FederationStateImage stateImageFor(
      Federation const& federation,
      std::wstring const& federationName);

  // Rebuild the accepted Update Attribute Values telemetry ledger from the
  // admission-validated image. Callback routes are unrelated to these
  // joined-federate lifetime values and remain outside the image.
  static void restoreMemberUpdateTelemetryFromStateImage(
      Federation& federation,
      FederationStateImage const& image,
      Federation const& liveFederation);

  // Rebuild the accepted application reflection callback count from the
  // admission-validated durable image. The count is a joined-federate
  // lifetime statistic and does not depend on callback routing.
  static void restoreMemberReflectionTelemetryFromStateImage(
      Federation& federation,
      FederationStateImage const& image,
      Federation const& liveFederation);

  // Rebuild the joined-federate object-lifecycle MOM counters from the
  // admission-validated durable image. These are lifetime statistics, not a
  // reconstruction of the current object map.
  static void restoreMemberObjectLifecycleTelemetryFromStateImage(
      Federation& federation,
      FederationStateImage const& image,
      Federation const& liveFederation);

  // Rebuild the accepted sender-side interaction statistics from the
  // admission-validated durable image. Directed sends are retained as a
  // subset while both ledgers remain scoped to the joined-federate lifetime.
  static void restoreMemberInteractionSendTelemetryFromStateImage(
      Federation& federation,
      FederationStateImage const& image,
      Federation const& liveFederation);

  // Rebuild accepted application Receive Interaction callback statistics from
  // the admission-validated durable image. Directed receipts remain a typed
  // subset of the total receiver-side ledger.
  static void restoreMemberInteractionReceiptTelemetryFromStateImage(
      Federation& federation,
      FederationStateImage const& image,
      Federation const& liveFederation);

  // Rebuild the scalar federation/member controls and route-free temporal
  // state when a durable image is being admitted without a process-local
  // save snapshot.  Callback routes, MOM objects, and service writers remain
  // owned by the current joined ambassadors.
  static void restoreControlAndTimeFromStateImage(
      Federation& federation,
      FederationStateImage const& image,
      Federation const& liveFederation);

  // Rebuild the accepted object-instance-name reservations from the
  // admission-validated durable image. Reservations are federation-scoped
  // ownership of exact names and remain valid until release, registration, or
  // resignation.
  static void restoreObjectInstanceNameReservationsFromStateImage(
      Federation& federation,
      FederationStateImage const& image,
      Federation const& liveFederation);

  // Rebuild federation-owned region specifications from the
  // admission-validated durable image. Pending and committed range maps are
  // restored before regional declaration ledgers so their region references
  // can be validated against the restored DDM catalog.
  static void restoreRegionsFromStateImage(
      Federation& federation,
      FederationStateImage const& image,
      Federation const& liveFederation);

  // Rebuild pending synchronization-point state from the admission-validated
  // durable image. Callback routes remain live-only and are rebound by the
  // normal restore notification path.
  static void restoreSynchronizationPointsFromStateImage(
      Federation& federation,
      FederationStateImage const& image,
      Federation const& liveFederation);

  // Rebuild the per-federate object-class publication/subscription/default
  // declaration ledgers from the admission-validated durable image. These
  // declarations are federation state, while callback routes remain live-only
  // and are rebound from the current joined federates during restore.
  static void restoreObjectClassAttributeDeclarationsFromStateImage(
      Federation& federation,
      FederationStateImage const& image,
      Federation const& liveFederation);

  // Rebuild the bounded route-free interaction declaration slice used by a
  // fresh-registry restore. The current tranche admits one or two published
  // classes (the one-class form may carry one same-class subscription and/or
  // one pending transportation-type change), one standalone subscription, or
  // one standalone regional subscription; callback routes remain live-only
  // and are rebound after the snapshot is applied.
  static void restoreInteractionDeclarationsFromStateImage(
      Federation& federation,
      FederationStateImage const& image,
      Federation const& liveFederation);

  // Recomputes the declaration-relevance baseline while the registry mutex
  // is already held. Restore uses this to seed the post-image state without
  // exposing synthetic Start/Turn-On callbacks to application code.
  [[nodiscard]] std::vector<DeclarationAdvisory>
  planDeclarationAdvisoriesLocked(std::wstring const& federationName);

  static void restoreTsoQueueFromStateImage(
      Federation& federation,
      FederationStateImage const& image);

  // Rebuild object identity, ownership, and latest application-value state
  // from the admission-validated durable image. If Available and regular
  // reservations are restored with callback-free identity and attribute sets;
  // any remaining opaque pending-operation count is preserved only when a
  // process-local snapshot supplies its live-only callback bookkeeping. A
  // restarted registry may materialize only the bounded, route-free object
  // application-value slice; broader object/application ledgers remain gated.
  static void restoreObjectOwnershipLedgersFromStateImage(
      Federation& federation,
      FederationStateImage const& image,
      Federation const& liveFederation,
      bool allowProcessRestartApplicationValues = false);

  // Rebuild the route-free timestamped application payloads and Request
  // Retraction ledger from the admission-validated durable image. Directed
  // and object-deletion recipients are rebound to the current live
  // callback/report routes; no callback endpoint is copied from the
  // process-local save snapshot. The deletion invocation snapshot is
  // restored with its bounded object/ownership basis.
  static void restoreTsoPayloadsFromStateImage(
      Federation& federation,
      FederationStateImage const& image,
      Federation const& liveFederation);

  static void restoreFederationFromSnapshot(
      Federation& target,
      Federation const& snapshot);

  [[nodiscard]] FederationRegistryResult resignLocked(
      std::wstring const& federationName,
      std::uint64_t federateId,
      rti1516_2025::ResignAction resignAction,
      bool forcedConnectionLoss,
      std::optional<std::uint16_t> finalServiceReportGroup,
      bool reservePublicInteraction = false);

  mutable std::mutex mutex_;
  std::shared_ptr<RuntimeInstrumentation> instrumentation_;
  std::map<std::wstring, Federation> federations_;
  // One or more completed labels may be restored while the federation
  // remains alive. These snapshots are intentionally process-local and
  // immutable after save completion. The save-commit store receives the
  // canonical route-free state image before Federation Saved callbacks;
  // bounded queue, payload, Request Retraction-ledger, and typed ownership-
  // acquisition-reservation rehydration consume that image during restore,
  // while the remaining typed ledgers are a later versioned step.
  std::map<std::wstring, std::map<std::wstring, Federation>> saveSnapshots_;
  std::shared_ptr<FederationSaveCommitStore> saveCommitStore_;
  std::uint64_t nextFederateId_ = 1;
};

}  // namespace umbra::detail
