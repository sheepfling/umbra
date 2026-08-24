#pragma once

#include "generated/rti_ambassador_shell.hpp"
#include "internal/callbacks/callback_dispatcher.hpp"
#include "internal/callbacks/callback_session.hpp"
#include "internal/observability/runtime_instrumentation.hpp"
#include "internal/observability/runtime_instrumentation_output.hpp"
#include "internal/observability/service_report_store.hpp"
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/federation/embedded_transport.hpp"
#include "internal/federation/federation_registry.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#endif
#include "internal/federation/federate_lifecycle.hpp"
#include "internal/time/federate_time_state.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <vector>
#include <thread>

namespace rti1516_2025::umbra_binding_detail {

// Connection fields are retained by every embedded-profile connection. In a
// federation-management build they later seed the joined-federate report file;
// the public configuration surface remains the official RtiConfiguration type.
// Umbra's typed embedded-profile directory helper only constructs that
// official type; it does not select a store.
struct ServiceReportConnectionSnapshot final {
  CallbackModel callbackModel = HLA_EVOKED;
  std::wstring configurationName;
  std::wstring rtiAddress;
  std::wstring additionalSettings;
};

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
// The joined-federate report writer is shared only with private recipient
// report routes.  A weak route cannot keep a resigned or disconnected
// federate's file writable, while the mutex keeps direct and RTI-initiated
// records serialized into that one lifetime file.
struct JoinedServiceReportEndpoint final {
  mutable std::mutex mutex;
  std::unique_ptr<umbra::detail::ServiceReportWriter> writer;
};

// One instance exists only during one joined-federate lifetime. Retaining
// both the path and endpoint here prevents a switch toggle from allocating a
// replacement report file later in that lifetime.
struct JoinedServiceReportState final {
  std::uint64_t federateId = 0;
  std::uint64_t joinIdentifier = 0;
  std::filesystem::path location;
  std::shared_ptr<JoinedServiceReportEndpoint> endpoint;
};
#endif

// Private adapter for the embedded connection slice. It deliberately exposes
// no Umbra-specific public surface: callers use only RTIambassador.
class UmbraRtiAmbassador final : public RtiAmbassadorShell {
 public:
  UmbraRtiAmbassador() = default;

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  // This tag and constructor live only in Umbra's non-installed internal
  // header. They let unit tests inject a deterministic store without making
  // the report backend a normal RTI configuration choice.
  struct ServiceReportStoreTestSeam final {};

  explicit UmbraRtiAmbassador(
      ServiceReportStoreTestSeam,
      std::unique_ptr<umbra::detail::ServiceReportStore> serviceReportStore);

  // Non-installed test inspection only. It exposes the private, unpublished
  // RTI-owned joined-federate MOM snapshot so integration tests can verify
  // lifecycle and exact initial encodings without manufacturing public MOM
  // discovery/reflection callbacks.
  [[nodiscard]] std::optional<umbra::detail::JoinedFederateMomObjectSnapshot>
  joinedFederateMomObjectSnapshotForTesting() const;
#endif

  ~UmbraRtiAmbassador() override;

  // Non-installed diagnostic inspection only. The public RTI API deliberately
  // has no statistics or tracing surface.
  [[nodiscard]] umbra::detail::RuntimeInstrumentationSnapshot
  runtimeInstrumentationSnapshotForTesting() const;

  // Non-installed diagnostic setup only. The provider includes the shared
  // embedded federation registry totals when that profile is enabled. The
  // caller must stop any sampler before destroying this ambassador.
  [[nodiscard]] umbra::detail::RuntimeInstrumentationSnapshotProvider
  runtimeInstrumentationSnapshotProviderForTesting() const;

  ConfigurationResult connect(
      FederateAmbassador& federateAmbassador,
      CallbackModel callbackModel) override;

  ConfigurationResult connect(
      FederateAmbassador& federateAmbassador,
      CallbackModel callbackModel,
      RtiConfiguration const& configuration) override;

  ConfigurationResult connect(
      FederateAmbassador& federateAmbassador,
      CallbackModel callbackModel,
      Credentials const& credentials) override;

  ConfigurationResult connect(
      FederateAmbassador& federateAmbassador,
      CallbackModel callbackModel,
      RtiConfiguration const& configuration,
      Credentials const& credentials) override;

  void disconnect() override;

  bool evokeCallback(double approximateMinimumTimeInSeconds) override;

  bool evokeMultipleCallbacks(
      double approximateMinimumTimeInSeconds,
      double approximateMaximumTimeInSeconds) override;

  void enableCallbacks() override;
  void disableCallbacks() override;

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  void createFederationExecution(
      std::wstring const& federationName,
      std::wstring const& fomModule,
      std::wstring const& logicalTimeImplementationName = L"") override;

  void createFederationExecution(
      std::wstring const& federationName,
      std::vector<std::wstring> const& fomModules,
      std::wstring const& logicalTimeImplementationName = L"") override;

  void createFederationExecutionWithMIM(
      std::wstring const& federationName,
      std::vector<std::wstring> const& fomModules,
      std::wstring const& mimModule,
      std::wstring const& logicalTimeImplementationName = L"") override;

  void destroyFederationExecution(std::wstring const& federationName) override;

  void listFederationExecutions() override;

  void listFederationExecutionMembers(std::wstring const& federationName) override;

  FederateHandle joinFederationExecution(
      std::wstring const& federateType,
      std::wstring const& federationName,
      std::vector<std::wstring> const& additionalFomModules = std::vector<std::wstring>()) override;

  FederateHandle joinFederationExecution(
      std::wstring const& federateName,
      std::wstring const& federateType,
      std::wstring const& federationName,
      std::vector<std::wstring> const& additionalFomModules = std::vector<std::wstring>()) override;

  void resignFederationExecution(ResignAction resignAction) override;

  void registerFederationSynchronizationPoint(
      std::wstring const& synchronizationPointLabel,
      VariableLengthData const& userSuppliedTag) override;

  void registerFederationSynchronizationPoint(
      std::wstring const& synchronizationPointLabel,
      VariableLengthData const& userSuppliedTag,
      FederateHandleSet const& synchronizationSet) override;

  void synchronizationPointAchieved(
      std::wstring const& synchronizationPointLabel,
      bool successfully = true) override;

  void requestFederationSave(std::wstring const& label) override;

  void requestFederationSave(
      std::wstring const& label,
      LogicalTime const& time) override;

  void federateSaveBegun() override;

  void federateSaveComplete() override;

  void federateSaveNotComplete() override;

  void abortFederationSave() override;

  void queryFederationSaveStatus() override;

  void requestFederationRestore(std::wstring const& label) override;

  void federateRestoreComplete() override;

  void federateRestoreNotComplete() override;

  void abortFederationRestore() override;

  void queryFederationRestoreStatus() override;

  std::unique_ptr<LogicalTimeFactory> getTimeFactory() const override;


  FederateHandle getFederateHandle(std::wstring const& federateName) override;

  std::wstring getFederateName(FederateHandle const& federate) override;

  ObjectClassHandle getObjectClassHandle(std::wstring const& objectClassName) override;

  std::wstring getObjectClassName(ObjectClassHandle const& objectClass) override;

  AttributeHandle getAttributeHandle(
      ObjectClassHandle const& objectClass,
      std::wstring const& attributeName) override;

  std::wstring getAttributeName(
      ObjectClassHandle const& objectClass,
      AttributeHandle const& attribute) override;

  double getUpdateRateValue(
      std::wstring const& updateRateDesignator) override;

  double getUpdateRateValueForAttribute(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandle const& attribute) override;

  void publishObjectClassAttributes(
      ObjectClassHandle const& objectClass,
      AttributeHandleSet const& attributes) override;

  void unpublishObjectClass(ObjectClassHandle const& objectClass) override;

  void unpublishObjectClassAttributes(
      ObjectClassHandle const& objectClass,
      AttributeHandleSet const& attributes) override;

  void subscribeObjectClassAttributes(
      ObjectClassHandle const& objectClass,
      AttributeHandleSet const& attributes,
      bool active = true,
      std::wstring const& updateRateDesignator = L"") override;

  void unsubscribeObjectClass(ObjectClassHandle const& objectClass) override;

  void unsubscribeObjectClassAttributes(
      ObjectClassHandle const& objectClass,
      AttributeHandleSet const& attributes) override;

  void reserveObjectInstanceName(
      std::wstring const& objectInstanceName) override;

  void releaseObjectInstanceName(
      std::wstring const& objectInstanceName) override;

  void reserveMultipleObjectInstanceNames(
      std::set<std::wstring> const& objectInstanceNames) override;

  void releaseMultipleObjectInstanceNames(
      std::set<std::wstring> const& objectInstanceNames) override;

  ObjectInstanceHandle registerObjectInstanceWithRegions(
      ObjectClassHandle const& objectClass,
      AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions) override;

  void associateRegionsForUpdates(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions) override;

  void unassociateRegionsForUpdates(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions) override;

  void subscribeObjectClassAttributesWithRegions(
      ObjectClassHandle const& objectClass,
      AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions,
      bool active = true,
      std::wstring const& updateRateDesignator = L"") override;

  void unsubscribeObjectClassAttributesWithRegions(
      ObjectClassHandle const& objectClass,
      AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions) override;

  ObjectInstanceHandle registerObjectInstance(
      ObjectClassHandle const& objectClass) override;

  ObjectInstanceHandle registerObjectInstance(
      ObjectClassHandle const& objectClass,
      std::wstring const& objectInstanceName) override;

  ObjectInstanceHandle registerObjectInstanceWithRegions(
      ObjectClassHandle const& objectClass,
      AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions,
      std::wstring const& objectInstanceName) override;

  void deleteObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      VariableLengthData const& userSuppliedTag) override;

  MessageRetractionHandle deleteObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      VariableLengthData const& userSuppliedTag,
      LogicalTime const& time) override;

  void localDeleteObjectInstance(
      ObjectInstanceHandle const& objectInstance) override;

  void updateAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const& userSuppliedTag) override;

  MessageRetractionHandle updateAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const& userSuppliedTag,
      LogicalTime const& time) override;

  void requestAttributeValueUpdate(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      VariableLengthData const& userSuppliedTag) override;

  void requestAttributeValueUpdate(
      ObjectClassHandle const& objectClass,
      AttributeHandleSet const& attributes,
      VariableLengthData const& userSuppliedTag) override;

  void requestAttributeValueUpdateWithRegions(
      ObjectClassHandle const& objectClass,
      AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions,
      VariableLengthData const& userSuppliedTag) override;

  void queryAttributeOwnership(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes) override;

  bool isAttributeOwnedByFederate(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandle const& attribute) override;

  void negotiatedAttributeOwnershipDivestiture(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      VariableLengthData const& userSuppliedTag) override;

  void confirmDivestiture(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& confirmedAttributes,
      VariableLengthData const& userSuppliedTag) override;

  void cancelNegotiatedAttributeOwnershipDivestiture(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes) override;

  void unconditionalAttributeOwnershipDivestiture(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      VariableLengthData const& userSuppliedTag) override;

  void attributeOwnershipAcquisition(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& desiredAttributes,
      VariableLengthData const& userSuppliedTag) override;

  void attributeOwnershipAcquisitionIfAvailable(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& desiredAttributes,
      VariableLengthData const& userSuppliedTag) override;

  void attributeOwnershipReleaseDenied(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      VariableLengthData const& userSuppliedTag) override;

  void attributeOwnershipDivestitureIfWanted(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      VariableLengthData const& userSuppliedTag,
      AttributeHandleSet& divestedAttributes) override;

  void cancelAttributeOwnershipAcquisition(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes) override;

  ObjectClassHandle getKnownObjectClassHandle(
      ObjectInstanceHandle const& objectInstance) override;

  ObjectInstanceHandle getObjectInstanceHandle(
      std::wstring const& objectInstanceName) override;

  std::wstring getObjectInstanceName(
      ObjectInstanceHandle const& objectInstance) override;

  InteractionClassHandle getInteractionClassHandle(
      std::wstring const& interactionClassName) override;

  std::wstring getInteractionClassName(
      InteractionClassHandle const& interactionClass) override;

  ParameterHandle getParameterHandle(
      InteractionClassHandle const& interactionClass,
      std::wstring const& parameterName) override;

  std::wstring getParameterName(
      InteractionClassHandle const& interactionClass,
      ParameterHandle const& parameter) override;

  void publishInteractionClass(InteractionClassHandle const& interactionClass) override;

  void unpublishInteractionClass(InteractionClassHandle const& interactionClass) override;

  void publishObjectClassDirectedInteractions(
      ObjectClassHandle const& objectClass,
      InteractionClassHandleSet const& interactionClasses) override;

  void unpublishObjectClassDirectedInteractions(
      ObjectClassHandle const& objectClass) override;

  void unpublishObjectClassDirectedInteractions(
      ObjectClassHandle const& objectClass,
      InteractionClassHandleSet const& interactionClasses) override;

  void subscribeInteractionClass(
      InteractionClassHandle const& interactionClass,
      bool active = true) override;

  void unsubscribeInteractionClass(InteractionClassHandle const& interactionClass) override;

  void subscribeObjectClassDirectedInteractions(
      ObjectClassHandle const& objectClass,
      InteractionClassHandleSet const& interactionClasses,
      bool universally = false) override;

  void unsubscribeObjectClassDirectedInteractions(
      ObjectClassHandle const& objectClass) override;

  void unsubscribeObjectClassDirectedInteractions(
      ObjectClassHandle const& objectClass,
      InteractionClassHandleSet const& interactionClasses) override;

  void subscribeInteractionClassWithRegions(
      InteractionClassHandle const& interactionClass,
      RegionHandleSet const& regions,
      bool active = true) override;

  void unsubscribeInteractionClassWithRegions(
      InteractionClassHandle const& interactionClass,
      RegionHandleSet const& regions) override;

  void sendInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag) override;

  MessageRetractionHandle sendInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      LogicalTime const& time) override;

  void sendDirectedInteraction(
      InteractionClassHandle const& interactionClass,
      ObjectInstanceHandle const& objectInstance,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag) override;

  MessageRetractionHandle sendDirectedInteraction(
      InteractionClassHandle const& interactionClass,
      ObjectInstanceHandle const& objectInstance,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      LogicalTime const& time) override;

  void sendInteractionWithRegions(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameterValues,
      RegionHandleSet const& regions,
      VariableLengthData const& userSuppliedTag) override;

  MessageRetractionHandle sendInteractionWithRegions(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameterValues,
      RegionHandleSet const& regions,
      VariableLengthData const& userSuppliedTag,
      LogicalTime const& time) override;

  void changeAttributeOrderType(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      OrderType orderType) override;

  void changeDefaultAttributeOrderType(
      ObjectClassHandle const& objectClass,
      AttributeHandleSet const& attributes,
      OrderType orderType) override;

  void changeInteractionOrderType(
      InteractionClassHandle const& interactionClass,
      OrderType orderType) override;

  void requestAttributeTransportationTypeChange(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      TransportationTypeHandle const& transportationType) override;

  void changeDefaultAttributeTransportationType(
      ObjectClassHandle const& objectClass,
      AttributeHandleSet const& attributes,
      TransportationTypeHandle const& transportationType) override;

  void queryAttributeTransportationType(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandle const& attribute) override;

  void requestInteractionTransportationTypeChange(
      InteractionClassHandle const& interactionClass,
      TransportationTypeHandle const& transportationType) override;

  void queryInteractionTransportationType(
      FederateHandle const& federate,
      InteractionClassHandle const& interactionClass) override;

  TransportationTypeHandle getTransportationTypeHandle(
      std::wstring const& transportationTypeName) override;

  std::wstring getTransportationTypeName(
      TransportationTypeHandle const& transportationType) override;

  OrderType getOrderType(std::wstring const& orderTypeName) override;

  std::wstring getOrderName(OrderType orderType) override;

  DimensionHandleSet getAvailableDimensionsForObjectClass(
      ObjectClassHandle const& objectClass) override;

  DimensionHandleSet getAvailableDimensionsForInteractionClass(
      InteractionClassHandle const& interactionClass) override;

  DimensionHandle getDimensionHandle(
      std::wstring const& dimensionName) override;

  std::wstring getDimensionName(
      DimensionHandle const& dimension) override;

  unsigned long getDimensionUpperBound(
      DimensionHandle const& dimension) override;

  RegionHandle createRegion(
      DimensionHandleSet const& dimensions) override;

  void commitRegionModifications(
      RegionHandleSet const& regions) override;

  void deleteRegion(
      RegionHandle const& region) override;

  DimensionHandleSet getDimensionHandleSet(
      RegionHandle const& region) override;

  RangeBounds getRangeBounds(
      RegionHandle const& region,
      DimensionHandle const& dimension) override;

  void setRangeBounds(
      RegionHandle const& region,
      DimensionHandle const& dimension,
      RangeBounds const& rangeBounds) override;

  unsigned long normalizeServiceGroup(ServiceGroup serviceGroup) override;

  unsigned long normalizeFederateHandle(FederateHandle const& federate) override;

  unsigned long normalizeObjectClassHandle(ObjectClassHandle const& objectClass) override;

  unsigned long normalizeInteractionClassHandle(
      InteractionClassHandle const& interactionClass) override;

  unsigned long normalizeObjectInstanceHandle(
      ObjectInstanceHandle const& objectInstance) override;

  bool getAttributeScopeAdvisorySwitch() const override;

  void setAttributeScopeAdvisorySwitch(bool switchValue) override;

  bool getObjectClassRelevanceAdvisorySwitch() const override;

  void setObjectClassRelevanceAdvisorySwitch(bool switchValue) override;

  bool getAttributeRelevanceAdvisorySwitch() const override;

  void setAttributeRelevanceAdvisorySwitch(bool switchValue) override;

  bool getInteractionRelevanceAdvisorySwitch() const override;

  void setInteractionRelevanceAdvisorySwitch(bool switchValue) override;

  bool getConveyRegionDesignatorSetsSwitch() const override;

  void setConveyRegionDesignatorSetsSwitch(bool switchValue) override;

  ResignAction getAutomaticResignDirective() override;

  void setAutomaticResignDirective(ResignAction resignAction) override;

  bool getServiceReportingSwitch() const override;

  void setServiceReportingSwitch(bool switchValue) override;

  bool getExceptionReportingSwitch() const override;

  void setExceptionReportingSwitch(bool switchValue) override;

  bool getSendServiceReportsToFileSwitch() const override;

  void setSendServiceReportsToFileSwitch(bool switchValue) override;

  bool getAutoProvideSwitch() const override;

  bool getDelaySubscriptionEvaluationSwitch() const override;

  bool getAdvisoriesUseKnownClassSwitch() const override;

  bool getAllowRelaxedDDMSwitch() const override;

  bool getNonRegulatedGrantSwitch() const override;

  void enableTimeRegulation(LogicalTimeInterval const& lookahead) override;

  void disableTimeRegulation() override;

  void enableTimeConstrained() override;

  void disableTimeConstrained() override;

  void enableAsynchronousDelivery() override;

  void disableAsynchronousDelivery() override;

  void timeAdvanceRequest(LogicalTime const& time) override;

  void timeAdvanceRequestAvailable(LogicalTime const& time) override;

  void nextMessageRequest(LogicalTime const& time) override;

  void nextMessageRequestAvailable(LogicalTime const& time) override;

  void flushQueueRequest(LogicalTime const& time) override;

  bool queryGALT(LogicalTime& time) override;

  void queryLogicalTime(LogicalTime& time) override;

  bool queryLITS(LogicalTime& time) override;

  void queryLookahead(LogicalTimeInterval& interval) override;

  void modifyLookahead(LogicalTimeInterval const& lookahead) override;

  void retract(MessageRetractionHandle const& retraction) override;

  FederateHandle decodeFederateHandle(
      VariableLengthData const& encodedValue) const override;

  ObjectClassHandle decodeObjectClassHandle(
      VariableLengthData const& encodedValue) const override;

  InteractionClassHandle decodeInteractionClassHandle(
      VariableLengthData const& encodedValue) const override;

  ObjectInstanceHandle decodeObjectInstanceHandle(
      VariableLengthData const& encodedValue) const override;

  AttributeHandle decodeAttributeHandle(
      VariableLengthData const& encodedValue) const override;

  ParameterHandle decodeParameterHandle(
      VariableLengthData const& encodedValue) const override;

  DimensionHandle decodeDimensionHandle(
      VariableLengthData const& encodedValue) const override;

  RegionHandle decodeRegionHandle(
      VariableLengthData const& encodedValue) const override;

  MessageRetractionHandle decodeMessageRetractionHandle(
      VariableLengthData const& encodedValue) const override;
#endif

 private:
  [[nodiscard]] umbra::detail::RuntimeInstrumentation::Scope beginRtiCall(
      std::string_view operation) const;

  ConfigurationResult connectImpl(
      FederateAmbassador& federateAmbassador,
      CallbackModel callbackModel,
      RtiConfiguration const* configuration);

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  enum class EmbeddedMembershipLossKind {
    connection_lost,
    rti_resigned,
  };

  FederateHandle joinFederationExecutionImpl(
      std::optional<std::wstring> requestedFederateName,
      std::wstring const& federateType,
      std::wstring const& federationName,
      std::vector<std::wstring> const& additionalFomModules);

  void registerFederationSynchronizationPointImpl(
      std::wstring const& synchronizationPointLabel,
      VariableLengthData const& userSuppliedTag,
      FederateHandleSet const& synchronizationSet,
      bool synchronizationSetWasSupplied);

  void requestAvailableTimeAdvance(
      LogicalTime const& time,
      umbra::detail::FederateTimeAdvanceMode mode,
      bool selectNextQueuedMessage,
      std::wstring const& serviceName);

  void requireFederationServiceOperationAvailable(
      std::wstring const& operation) const;

  // Called only after a service wrapper has completed successfully.  The
  // The optional interaction path is enabled for interaction service wrappers
  // whose callers have released their planning locks; the remaining
  // successful-void wrappers retain the file/suppressed path until their
  // callback scheduling is lifted out of their service locks.
  void appendSuccessfulVoidServiceReportToFileIfSelected(
      std::wstring const& service,
      umbra::detail::MomServiceType serviceType,
      std::vector<umbra::detail::MomServiceArgument> const& suppliedArguments,
      bool emitInteraction = false) const;

  // Non-void services use the rendered Table 5 ReturnArgument form for the
  // filesystem sink and the MIM HLAargument form for the public interaction
  // sink. Call this after releasing the service's state locks when
  // emitInteraction is true; an immediate observer callback is otherwise
  // allowed to re-enter the ambassador.
  void appendSuccessfulServiceReportToFileIfSelected(
      std::wstring const& service,
      umbra::detail::MomServiceType serviceType,
      std::vector<umbra::detail::MomServiceArgument> const& suppliedArguments,
      umbra::detail::MomServiceArgument const& returnedArgument,
      bool emitInteraction = true) const;

  // Failed service invocations use a Null returned argument and carry the
  // public exception description in HLAexception. Keep this on the same
  // selected file/interaction seam as successful invocations so the per-
  // joined-federate serial stream remains one ordered sequence.
  void appendFailedServiceReportToFileIfSelected(
      std::wstring const& service,
      umbra::detail::MomServiceType serviceType,
      std::vector<umbra::detail::MomServiceArgument> const& suppliedArguments,
      std::wstring const& exception,
      bool emitInteraction = true) const;

  // Emit one standard MIM interaction when the subject has selected the
  // interaction sink.  This is deliberately independent of the file writer:
  // timestamped services can reserve/encode their returned argument after
  // C++ has admitted the TSO message but before any callback route is queued.
  [[nodiscard]] bool emitSelectedMomServiceReportInteraction(
      std::wstring const& service,
      umbra::detail::MomServiceType serviceType,
      std::vector<umbra::detail::MomServiceArgument> const& suppliedArguments,
      umbra::detail::MomServiceArgument const& returnedArgument,
      bool success = true,
      std::wstring const& exception = L"") const;

  // Emit the source-backed HLAreportException interaction while preserving
  // the original service exception at the public API boundary. This helper
  // is deliberately best-effort: a reporting route must never replace the
  // C++ exception that the invoking federate is required to receive.
  void emitExceptionReport(
      std::wstring const& service,
      rti1516_2025::Exception const& exception) const noexcept;

  // Emit the source-backed HLAreportMOMexception interaction for a rejected
  // MOM Send Interaction.  This route is subscription-selected and therefore
  // independent of the reported federate's Exception Reporting Switch.
  void emitMomExceptionReport(
      std::wstring const& service,
      rti1516_2025::Exception const& exception,
      bool parameterError) const noexcept;

  // Resign Federation Execution needs a serial that was atomically reserved
  // before its registry transaction erased the joined member.  Keep the
  // writer-only half separate so the ordinary helper remains membership-based.
  void appendReservedSuccessfulVoidServiceReportToFile(
      std::uint32_t serialNumber,
      std::wstring const& service,
      std::vector<umbra::detail::MomServiceArgument> const& suppliedArguments) const;

  void handleEmbeddedTransportFailure(std::wstring faultDescription);
  [[nodiscard]] bool handleEmbeddedFederateResignation(
      std::wstring reasonForResign);
  [[nodiscard]] bool handleEmbeddedMembershipLoss(
      EmbeddedMembershipLossKind kind,
      std::wstring reason);

  // HLA_IMMEDIATE has no caller-side Evoke boundary. The scheduler claims
  // due RTI-owned periodic MOM work on a private thread, while the ordinary
  // callback dispatcher still controls callback enable/disable and lifetime.
  void startPeriodicMomScheduler();
  void stopPeriodicMomScheduler() noexcept;
  void periodicMomSchedulerLoop(std::stop_token stopToken);
#endif

  mutable std::mutex mutex_;
  umbra::detail::FederateLifecycle lifecycle_;
  std::shared_ptr<umbra::detail::RuntimeInstrumentation> instrumentation_ =
      std::make_shared<umbra::detail::RuntimeInstrumentation>();
  std::shared_ptr<umbra::detail::CallbackDispatcher> callbacks_ =
      std::make_shared<umbra::detail::CallbackDispatcher>(
          umbra::detail::CallbackDispatchModel::evoked,
          instrumentation_);
  std::shared_ptr<CallbackSession> callbackSession_;
  CallbackModel callbackModel_ = HLA_EVOKED;
  // A factory-created ambassador always retains a filesystem store selected
  // from its connection configuration. The federation-management profile
  // later uses it to allocate a per-joined-federate writer.
  std::unique_ptr<umbra::detail::ServiceReportStore> serviceReportStore_;
  ServiceReportConnectionSnapshot serviceReportConnection_;
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  std::shared_ptr<umbra::detail::EmbeddedTransportConnection>
      transportConnection_;
  std::unique_ptr<umbra::detail::ServiceReportStore>
      injectedServiceReportStoreForTesting_;
  bool activeServiceReportStoreIsTestOnly_ = false;
  std::optional<JoinedServiceReportState> joinedServiceReport_;
  std::optional<std::wstring> joinedFederationName_;
  std::optional<std::uint64_t> joinedFederateId_;
  std::shared_ptr<umbra::detail::FederateTimeState> federateTimeState_;
  std::jthread periodicMomScheduler_;
#endif
};

}  // namespace rti1516_2025::umbra_binding_detail
