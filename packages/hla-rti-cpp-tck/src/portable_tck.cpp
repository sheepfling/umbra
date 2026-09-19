#define main hla_rti_cpp_tck_original_main
#include "main.cpp"
#undef main

namespace {

constexpr char automaticResignDirectiveDeleteObjectsId[] =
    "cpp-tck.automatic-resign-directive-delete-objects";
constexpr char automaticResignDirectiveDeleteObjectsContractId[] =
    "cpp-tck.automatic-resign-directive-delete-objects-contract";
constexpr char publicHandleDecodingId[] = "cpp-tck.public-handle-decoding";
constexpr char publicHandleDecodingContractId[] =
    "cpp-tck.public-handle-decoding-contract";
constexpr char customTransportationAttributeDeliveryId[] =
    "cpp-tck.custom-transportation-attribute-delivery";
constexpr char customTransportationAttributeDeliveryContractId[] =
    "cpp-tck.custom-transportation-attribute-delivery-contract";
constexpr char customTransportationTimestampedAttributeDeliveryId[] =
    "cpp-tck.custom-transportation-timestamped-attribute-delivery";
constexpr char customTransportationTimestampedAttributeDeliveryContractId[] =
    "cpp-tck.custom-transportation-timestamped-attribute-delivery-contract";
constexpr char customTransportationTimestampedAttributeAlternateAdvancesId[] =
    "cpp-tck.custom-transportation-timestamped-attribute-alternate-advances";
constexpr char customTransportationTimestampedAttributeAlternateAdvancesContractId[] =
    "cpp-tck.custom-transportation-timestamped-attribute-alternate-advances-contract";
constexpr char customTransportationTimestampedInteractionAlternateAdvancesId[] =
    "cpp-tck.custom-transportation-timestamped-interaction-alternate-advances";
constexpr char customTransportationTimestampedInteractionAlternateAdvancesContractId[] =
    "cpp-tck.custom-transportation-timestamped-interaction-alternate-advances-contract";
constexpr char customTransportationDirectedInteractionDeliveryId[] =
    "cpp-tck.custom-transportation-directed-interaction-delivery";
constexpr char customTransportationDirectedInteractionDeliveryContractId[] =
    "cpp-tck.custom-transportation-directed-interaction-delivery-contract";
constexpr char customTransportationTimestampedDirectedInteractionAlternateAdvancesId[] =
    "cpp-tck.custom-transportation-timestamped-directed-interaction-alternate-advances";
constexpr char customTransportationTimestampedDirectedInteractionAlternateAdvancesContractId[] =
    "cpp-tck.custom-transportation-timestamped-directed-interaction-alternate-advances-contract";
constexpr char customTransportationTimestampedRegionalInteractionDeliveryId[] =
    "cpp-tck.custom-transportation-timestamped-regional-interaction-delivery";
constexpr char customTransportationTimestampedRegionalInteractionDeliveryContractId[] =
    "cpp-tck.custom-transportation-timestamped-regional-interaction-delivery-contract";
constexpr char delaySubscriptionEvaluationInteractionScenario[] =
    "cpp-tck.delay-subscription-evaluation-interaction";
constexpr char delaySubscriptionEvaluationInteractionContractId[] =
    "cpp-tck.delay-subscription-evaluation-interaction-contract";
constexpr char delaySubscriptionEvaluationDirectedInteractionScenario[] =
    "cpp-tck.delay-subscription-evaluation-directed-interaction";
constexpr char delaySubscriptionEvaluationDirectedInteractionContractId[] =
    "cpp-tck.delay-subscription-evaluation-directed-interaction-contract";
constexpr char delaySubscriptionEvaluationAttributeUpdateScenario[] =
    "cpp-tck.delay-subscription-evaluation-attribute-update";
constexpr char delaySubscriptionEvaluationAttributeUpdateContractId[] =
    "cpp-tck.delay-subscription-evaluation-attribute-update-contract";
constexpr char delaySubscriptionEvaluationTimestampedInteractionScenario[] =
    "cpp-tck.delay-subscription-evaluation-timestamped-interaction";
constexpr char delaySubscriptionEvaluationTimestampedInteractionContractId[] =
    "cpp-tck.delay-subscription-evaluation-timestamped-interaction-contract";
constexpr char delaySubscriptionEvaluationTimestampedDirectedInteractionScenario[] =
    "cpp-tck.delay-subscription-evaluation-timestamped-directed-interaction";
constexpr char delaySubscriptionEvaluationTimestampedDirectedInteractionContractId[] =
    "cpp-tck.delay-subscription-evaluation-timestamped-directed-interaction-contract";
constexpr char delaySubscriptionEvaluationTimestampedAttributeUpdateScenario[] =
    "cpp-tck.delay-subscription-evaluation-timestamped-attribute-update";
constexpr char delaySubscriptionEvaluationTimestampedAttributeUpdateContractId[] =
    "cpp-tck.delay-subscription-evaluation-timestamped-attribute-update-contract";
constexpr char federationSaveRestoreInterlocksScenario[] =
    "cpp-tck.federation-save-restore-interlocks";
constexpr char federationSaveRestoreInterlocksContractId[] =
    "cpp-tck.federation-save-restore-interlocks-contract";
constexpr char timedFederationSaveRestoreScenario[] =
    "cpp-tck.timed-federation-save-restore";
constexpr char timedFederationSaveRestoreContractId[] =
    "cpp-tck.timed-federation-save-restore-contract";
constexpr char timedRegionalInteractionSaveRestoreScenario[] =
    "cpp-tck.timed-regional-interaction-save-restore";
constexpr char timedRegionalInteractionSaveRestoreContractId[] =
    "cpp-tck.timed-regional-interaction-save-restore-contract";
constexpr char timedDefaultRegionInteractionSaveRestoreScenario[] =
    "cpp-tck.timed-default-region-interaction-save-restore";
constexpr char timedDefaultRegionInteractionSaveRestoreContractId[] =
    "cpp-tck.timed-default-region-interaction-save-restore-contract";
constexpr char timedDefaultRegionAttributeSaveRestoreScenario[] =
    "cpp-tck.timed-default-region-attribute-save-restore";
constexpr char timedDefaultRegionAttributeSaveRestoreContractId[] =
    "cpp-tck.timed-default-region-attribute-save-restore-contract";
constexpr char allowRelaxedDdmScenario[] = "cpp-tck.allow-relaxed-ddm";
constexpr char allowRelaxedDdmContractId[] =
    "cpp-tck.allow-relaxed-ddm-contract";
constexpr char regionalMultiAttributeUpdateScenario[] =
    "cpp-tck.regional-multi-attribute-update";
constexpr char regionalMultiAttributeUpdateContractId[] =
    "cpp-tck.regional-multi-attribute-update-contract";
constexpr char regionalThreeDimensionalOverlapScenario[] =
    "cpp-tck.regional-three-dimensional-overlap";
constexpr char regionalThreeDimensionalOverlapContractId[] =
    "cpp-tck.regional-three-dimensional-overlap-contract";
constexpr char regionalAttributeUpdateCallbackDdmRecheckScenario[] =
    "cpp-tck.regional-attribute-update-callback-ddm-recheck";
constexpr char regionalAttributeUpdateCallbackDdmRecheckContractScenario[] =
    "cpp-tck.regional-attribute-update-callback-ddm-recheck-contract";
constexpr char regionalAttributeValueRequestFilteringScenario[] =
    "cpp-tck.regional-attribute-value-request-filtering";
constexpr char regionalAttributeValueRequestFilteringContractScenario[] =
    "cpp-tck.regional-attribute-value-request-filtering-contract";
constexpr char regionalAttributeValueUpdateResponseRecheckScenario[] =
    "cpp-tck.regional-attribute-value-update-response-recheck";
constexpr char regionalAttributeValueUpdateResponseRecheckContractScenario[] =
    "cpp-tck.regional-attribute-value-update-response-recheck-contract";
constexpr char regionalBoundariesScenario[] = "cpp-tck.regional-boundaries";
constexpr char regionalBoundariesContractId[] =
    "cpp-tck.regional-boundaries-contract";
constexpr char ownershipTransferRegionalUpdateScenario[] =
    "cpp-tck.ownership-transfer-regional-update";
constexpr char ownershipTransferRegionalUpdateContractId[] =
    "cpp-tck.ownership-transfer-regional-update-contract";
constexpr char momTransportationTypeChangeRequestScenario[] =
    "cpp-tck.mom-transportation-type-change-request";
constexpr char momTransportationTypeChangeRequestContractId[] =
    "cpp-tck.mom-transportation-type-change-request-contract";
constexpr char joinedFederateMomRegisteredObjectCountScenario[] =
    "cpp-tck.joined-federate-mom-registered-object-count";
constexpr char joinedFederateMomRegisteredObjectCountContractId[] =
    "cpp-tck.joined-federate-mom-registered-object-count-contract";
constexpr char joinedFederateMomDeletableObjectReportScenario[] =
    "cpp-tck.joined-federate-mom-object-instances-that-can-be-deleted-report";
constexpr char joinedFederateMomDeletableObjectReportContractId[] =
    "cpp-tck.joined-federate-mom-object-instances-that-can-be-deleted-report-contract";
constexpr char joinedFederateMomDeletableObjectCountScenario[] =
    "cpp-tck.joined-federate-mom-deletable-object-count";
constexpr char joinedFederateMomDeletableObjectCountContractId[] =
    "cpp-tck.joined-federate-mom-deletable-object-count-contract";
constexpr char receiveOrderAttributeUpdateScenario[] =
    "cpp-tck.receive-order-attribute-update";
constexpr char receiveOrderAttributeUpdateContractId[] =
    "cpp-tck.receive-order-attribute-update-contract";
constexpr char receiveOrderInteractionScenario[] =
    "cpp-tck.receive-order-interaction";
constexpr char receiveOrderInteractionContractId[] =
    "cpp-tck.receive-order-interaction-contract";
constexpr char receiveOrderObjectRemovalScenario[] =
    "cpp-tck.receive-order-object-removal";
constexpr char receiveOrderObjectRemovalContractId[] =
    "cpp-tck.receive-order-object-removal-contract";
constexpr char federationRestoreAbortScenario[] =
    "cpp-tck.federation-restore-abort";
constexpr char federationRestoreAbortContractId[] =
    "cpp-tck.federation-restore-abort-contract";
constexpr char federationRestoreOwnershipAssumptionScenario[] =
    "cpp-tck.federation-restore-work-item-ownership-assumption";
constexpr char federationRestoreOwnershipAssumptionContractId[] =
    "cpp-tck.federation-restore-work-item-ownership-assumption-contract";
constexpr char ownershipAcquisitionIfAvailableScenario[] =
    "cpp-tck.ownership-acquisition-if-available";
constexpr char ownershipAcquisitionIfAvailableContractId[] =
    "cpp-tck.ownership-acquisition-if-available-contract";
constexpr char autoProvideDisabledDiscoveryOnlyScenario[] =
    "cpp-tck.auto-provide-disabled-discovery-only";
constexpr char autoProvideDisabledDiscoveryOnlyContractId[] =
    "cpp-tck.auto-provide-disabled-discovery-only-contract";
constexpr char autoProvideDisabledExplicitRequestScenario[] =
    "cpp-tck.auto-provide-disabled-explicit-request";
constexpr char autoProvideDisabledExplicitRequestContractId[] =
    "cpp-tck.auto-provide-disabled-explicit-request-contract";
constexpr char objectRegistrationServiceBoundariesScenario[] =
    "cpp-tck.object-registration-service-boundaries";
constexpr char objectRegistrationServiceBoundariesContractId[] =
    "cpp-tck.object-registration-service-boundaries-contract";
constexpr char objectDeletionServiceBoundariesScenario[] =
    "cpp-tck.object-deletion-service-boundaries";
constexpr char objectDeletionServiceBoundariesContractId[] =
    "cpp-tck.object-deletion-service-boundaries-contract";
constexpr char attributeUpdateServiceBoundariesScenario[] =
    "cpp-tck.attribute-update-service-boundaries";
constexpr char attributeUpdateServiceBoundariesContractId[] =
    "cpp-tck.attribute-update-service-boundaries-contract";
constexpr char interactionServiceBoundariesScenario[] =
    "cpp-tck.interaction-service-boundaries";
constexpr char interactionServiceBoundariesContractId[] =
    "cpp-tck.interaction-service-boundaries-contract";
constexpr char attributeValueRequestServiceBoundariesScenario[] =
    "cpp-tck.attribute-value-request-service-boundaries";
constexpr char attributeValueRequestServiceBoundariesContractId[] =
    "cpp-tck.attribute-value-request-service-boundaries-contract";
constexpr char connectionServiceBoundariesScenario[] =
    "cpp-tck.connection-service-boundaries";
constexpr char connectionServiceBoundariesContractId[] =
    "cpp-tck.connection-service-boundaries-contract";

void scenarioAutoProvideDisabledDiscoveryOnly(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Disabled Auto Provide testing requires an adapter-supplied ordinary FOM");
  Session owner(options, model, "auto-provide-disabled-owner");
  Session requester(options, model, "auto-provide-disabled-requester");
  auto const federation = federationName(
      options,
      "auto-provide-disabled-discovery-only");
  connectAndJoin(owner, requester, options, federation, options.fom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  auto const requesterAttribute = requester.rtiAmbassador().getAttributeHandle(
      requesterClass,
      options.attributeName);
  require(
      ownerClass.isValid() && requesterClass.isValid() &&
          ownerAttribute.isValid() && requesterAttribute.isValid(),
      "Disabled Auto Provide lookup returned an invalid standard handle");
  require(
      !owner.rtiAmbassador().getAutoProvideSwitch() &&
          !requester.rtiAmbassador().getAutoProvideSwitch(),
      "ordinary adapter FOM unexpectedly enabled the standard Auto Provide switch");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const requesterAttributes{requesterAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass,
      requesterAttributes,
      true,
      L"");
  owner.recorder().clearProvidedUpdates();
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "Disabled Auto Provide registration returned an invalid object handle");

  waitFor(
      owner,
      requester,
      [&] { return requester.recorder().hasDiscovery(object); },
      options,
      "disabled Auto Provide discovery");
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(owner.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(owner.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  require(
      owner.recorder().providedUpdates().empty(),
      "disabled Auto Provide generated a provideAttributeValueUpdate callback");
  auto const discovery = requester.recorder().discovery();
  require(
      discovery.present && discovery.object == object &&
          discovery.objectClass == requesterClass,
      "disabled Auto Provide discovery returned the wrong standard object");

  requester.rtiAmbassador().unsubscribeObjectClassAttributes(
      requesterClass,
      requesterAttributes);
  owner.rtiAmbassador().unpublishObjectClassAttributes(ownerClass, ownerAttributes);
  requester.resign(rti::NO_ACTION);
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioAutoProvideDisabledDiscoveryOnlyContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAutoProvideDisabledDiscoveryOnly(options, model);
}

void scenarioAutoProvideDisabledExplicitRequest(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Disabled Auto Provide explicit-request testing requires an adapter-supplied ordinary FOM");
  Session owner(options, model, "auto-provide-disabled-request-owner");
  Session requester(options, model, "auto-provide-disabled-request-requester");
  auto const federation = federationName(
      options,
      "auto-provide-disabled-explicit-request");
  connectAndJoin(owner, requester, options, federation, options.fom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  auto const requesterAttribute = requester.rtiAmbassador().getAttributeHandle(
      requesterClass,
      options.attributeName);
  require(
      ownerClass.isValid() && requesterClass.isValid() &&
          ownerAttribute.isValid() && requesterAttribute.isValid(),
      "Disabled Auto Provide explicit-request lookup returned an invalid standard handle");
  require(
      !owner.rtiAmbassador().getAutoProvideSwitch() &&
          !requester.rtiAmbassador().getAutoProvideSwitch(),
      "ordinary adapter FOM unexpectedly enabled the standard Auto Provide switch");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const requesterAttributes{requesterAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass,
      requesterAttributes,
      true,
      L"");
  owner.recorder().clearProvidedUpdates();
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "Disabled Auto Provide explicit-request registration returned an invalid object handle");

  waitFor(
      owner,
      requester,
      [&] { return requester.recorder().hasDiscovery(object); },
      options,
      "disabled Auto Provide explicit-request discovery");
  require(
      owner.recorder().providedUpdates().empty(),
      "disabled Auto Provide emitted a solicitation before the explicit request");

  auto const discoveredObject = requester.recorder().discovery().object;
  std::vector<std::uint8_t> const requestTagBytes{0x51U, 0x52U, 0x54U};
  rti::VariableLengthData const requestTag(
      requestTagBytes.data(),
      requestTagBytes.size());
  requester.rtiAmbassador().requestAttributeValueUpdate(
      discoveredObject,
      requesterAttributes,
      requestTag);
  waitFor(
      owner,
      requester,
      [&] { return owner.recorder().providedUpdates().size() == 1U; },
      options,
      "disabled Auto Provide explicit provider request");

  auto const providedUpdates = owner.recorder().providedUpdates();
  require(
      providedUpdates.size() == 1U,
      "disabled Auto Provide explicit request produced an unexpected callback count");
  require(
      providedUpdates.front().object == object &&
          providedUpdates.front().attributes == ownerAttributes &&
          providedUpdates.front().tag == requestTagBytes,
      "disabled Auto Provide explicit request returned the wrong object, attributes, or tag");

  owner.recorder().clearProvidedUpdates();
  std::vector<std::uint8_t> const classRequestTagBytes{0x43U, 0x4CU, 0x53U};
  rti::VariableLengthData const classRequestTag(
      classRequestTagBytes.data(),
      classRequestTagBytes.size());
  requester.rtiAmbassador().requestAttributeValueUpdate(
      requesterClass,
      requesterAttributes,
      classRequestTag);
  waitFor(
      owner,
      requester,
      [&] { return owner.recorder().providedUpdates().size() == 1U; },
      options,
      "disabled Auto Provide explicit class request");
  auto const classProvidedUpdates = owner.recorder().providedUpdates();
  require(
      classProvidedUpdates.size() == 1U &&
          classProvidedUpdates.front().object == object &&
          classProvidedUpdates.front().attributes == ownerAttributes &&
          classProvidedUpdates.front().tag == classRequestTagBytes,
      "disabled Auto Provide explicit class request returned the wrong callback data");

  requester.rtiAmbassador().unsubscribeObjectClassAttributes(
      requesterClass,
      requesterAttributes);
  owner.rtiAmbassador().unpublishObjectClassAttributes(ownerClass, ownerAttributes);
  requester.resign(rti::NO_ACTION);
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioAutoProvideDisabledExplicitRequestContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAutoProvideDisabledExplicitRequest(options, model);
}

void scenarioObjectRegistrationServiceBoundaries(
    Options const& options,
    rti::CallbackModel model) {
  scenarioObjectRegistration(options, model);
}

void scenarioObjectRegistrationServiceBoundariesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioObjectRegistrationServiceBoundaries(options, model);
}

void scenarioObjectDeletionServiceBoundaries(
    Options const& options,
    rti::CallbackModel model) {
  scenarioObjectDeletion(options, model);
}

void scenarioObjectDeletionServiceBoundariesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioObjectDeletionServiceBoundaries(options, model);
}

void scenarioAttributeUpdateServiceBoundaries(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAttributeUpdate(options, model);
}

void scenarioAttributeUpdateServiceBoundariesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAttributeUpdateServiceBoundaries(options, model);
}

void scenarioInteractionServiceBoundaries(
    Options const& options,
    rti::CallbackModel model) {
  scenarioInteraction(options, model);
}

void scenarioInteractionServiceBoundariesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioInteractionServiceBoundaries(options, model);
}

void scenarioAttributeValueRequestServiceBoundaries(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAttributeValueRequest(options, model);
}

void scenarioAttributeValueRequestServiceBoundariesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAttributeValueRequestServiceBoundaries(options, model);
}

void scenarioConnectionServiceBoundaries(
    Options const& options,
    rti::CallbackModel model) {
  scenarioConnection(options, model);
}

void scenarioConnectionServiceBoundariesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioConnectionServiceBoundaries(options, model);
}

void scenarioJoinedFederateMomRegisteredObjectCount(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Joined-federate MOM registered-object testing requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "Joined-federate MOM registered-object testing requires an adapter-supplied standard MIM");

  Session owner(options, model, "mom-registered-owner");
  Session observer(options, model, "mom-registered-observer");
  auto const federation = federationName(
      options,
      "joined-federate-mom-registered-object-count");
  owner.connect();
  observer.connect();
  if (options.logicalTimeImplementationName.empty()) {
    owner.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring());
  } else {
    owner.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring(),
        options.logicalTimeImplementationName);
  }

  observer.join(
      options.memberFederateName + L"-mom-registered-observer",
      options.federateType,
      federation);
  auto const observerFederateClass = observer.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  require(
      observerFederateClass.isValid() &&
          observer.rtiAmbassador().getObjectClassName(observerFederateClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederate",
      "Joined-federate MOM registered-object lookup did not round-trip the standard class");

  auto const observerFederateHandleAttribute = observer.rtiAmbassador().getAttributeHandle(
      observerFederateClass,
      L"HLAfederateHandle");
  auto const observerRegisteredCountAttribute = observer.rtiAmbassador().getAttributeHandle(
      observerFederateClass,
      L"HLAobjectInstancesRegistered");
  auto const reliable = observer.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      observerFederateHandleAttribute.isValid() &&
          observerRegisteredCountAttribute.isValid() && reliable.isValid() &&
          observer.rtiAmbassador().getAttributeName(
              observerFederateClass,
              observerFederateHandleAttribute) == L"HLAfederateHandle" &&
          observer.rtiAmbassador().getAttributeName(
              observerFederateClass,
              observerRegisteredCountAttribute) == L"HLAobjectInstancesRegistered" &&
          observer.rtiAmbassador().getTransportationTypeName(reliable) == L"HLAreliable",
      "Joined-federate MOM registered-object lookup returned inconsistent standard handles");
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      observerFederateClass,
      rti::AttributeHandleSet{
          observerFederateHandleAttribute,
          observerRegisteredCountAttribute},
      true,
      L"");

  owner.join(
      options.ownerFederateName + L"-mom-registered-owner",
      options.federateType,
      federation);
  auto const ownerFederateClass = owner.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const ownerFederateHandleAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerFederateClass,
      L"HLAfederateHandle");
  auto const ownerRegisteredCountAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerFederateClass,
      L"HLAobjectInstancesRegistered");
  require(
      ownerFederateClass == observerFederateClass &&
          ownerFederateHandleAttribute == observerFederateHandleAttribute &&
          ownerRegisteredCountAttribute == observerRegisteredCountAttribute,
      "Joined-federate MOM registered-object lookup returned different handles to the joined federates");

  auto findOwnerMomReflection = [&]() -> std::optional<ReflectionRecord> {
    auto const reflections = observer.recorder().reflections();
    auto const expectedFederateHandle = copyBytes(owner.federateHandle().encode());
    auto const iterator = std::find_if(
        reflections.begin(),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          auto const value = reflection.values.find(observerFederateHandleAttribute);
          return reflection.present && value != reflection.values.end() &&
              copyBytes(value->second) == expectedFederateHandle;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return *iterator;
  };
  waitFor(
      observer,
      [&] { return findOwnerMomReflection().has_value(); },
      options,
      "joined-federate MOM registered-object owner object discovery");
  auto const ownerMomReflection = findOwnerMomReflection();
  require(
      ownerMomReflection.has_value() &&
          observer.rtiAmbassador().getKnownObjectClassHandle(
              ownerMomReflection->object) == observerFederateClass,
      "Joined-federate MOM registered-object discovery returned the wrong standard object");
  auto const ownerMomObject = ownerMomReflection->object;

  auto requestRegisteredCount = [&](std::int32_t expected,
                                    std::string const& description) {
    auto const before = observer.recorder().reflections().size();
    observer.rtiAmbassador().requestAttributeValueUpdate(
        ownerMomObject,
        rti::AttributeHandleSet{observerRegisteredCountAttribute},
        rti::VariableLengthData{});
    waitFor(
        observer,
        [&] {
          auto const reflections = observer.recorder().reflections();
          return reflections.size() > before &&
              std::any_of(
                  reflections.begin() + static_cast<std::ptrdiff_t>(before),
                  reflections.end(),
                  [&](ReflectionRecord const& reflection) {
                    return reflection.object == ownerMomObject &&
                        reflection.values.count(observerRegisteredCountAttribute) == 1U;
                  });
        },
        options,
        description);
    auto const reflections = observer.recorder().reflections();
    auto const iterator = std::find_if(
        reflections.begin() + static_cast<std::ptrdiff_t>(before),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          return reflection.object == ownerMomObject &&
              reflection.values.count(observerRegisteredCountAttribute) == 1U;
        });
    require(iterator != reflections.end(), description + " produced no registered-object value");
    auto const& reflection = *iterator;
    require(
        reflection.values.size() == 1U && reflection.transportation == reliable &&
            reflection.tag.empty() && !reflection.producer.isValid() &&
            !reflection.regions.has_value(),
        description + " returned non-standard MOM reflection metadata");
    rti::HLAinteger32BE decoded;
    decoded.decode(reflection.values.at(observerRegisteredCountAttribute));
    require(
        decoded.get() == expected,
        description + " returned the wrong registered-object count");
  };

  requestRegisteredCount(
      0,
      "joined-federate MOM registered-object initial value request");

  auto const ownerObjectClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerObjectClass,
      options.attributeName);
  require(
      ownerObjectClass.isValid() && ownerAttribute.isValid(),
      "Joined-federate MOM registered-object application lookup returned an invalid handle");
  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerObjectClass,
      ownerAttributes);
  auto const firstObject = owner.rtiAmbassador().registerObjectInstance(ownerObjectClass);
  require(
      firstObject.isValid(),
      "Joined-federate MOM registered-object first registration returned an invalid handle");
  requestRegisteredCount(
      1,
      "joined-federate MOM registered-object first value request");
  auto const secondObject = owner.rtiAmbassador().registerObjectInstance(ownerObjectClass);
  require(
      secondObject.isValid(),
      "Joined-federate MOM registered-object second registration returned an invalid handle");
  requestRegisteredCount(
      2,
      "joined-federate MOM registered-object second value request");

  observer.rtiAmbassador().unsubscribeObjectClassAttributes(
      observerFederateClass,
      rti::AttributeHandleSet{
          observerFederateHandleAttribute,
          observerRegisteredCountAttribute});
  owner.rtiAmbassador().unpublishObjectClassAttributes(
      ownerObjectClass,
      ownerAttributes);
  owner.resign(rti::DELETE_OBJECTS);
  observer.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  owner.disconnect();
}

void scenarioJoinedFederateMomRegisteredObjectCountContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioJoinedFederateMomRegisteredObjectCount(options, model);
}

void scenarioJoinedFederateMomDeletableObjectReport(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Joined-federate MOM deletable-object reporting requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "Joined-federate MOM deletable-object reporting requires an adapter-supplied standard MIM");

  Session requester(options, model, "mom-deletable-requester");
  Session owner(options, model, "mom-deletable-owner");
  auto const federation = federationName(
      options,
      "joined-federate-mom-object-instances-that-can-be-deleted-report");
  requester.connect();
  owner.connect();
  if (options.logicalTimeImplementationName.empty()) {
    owner.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring());
  } else {
    owner.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring(),
        options.logicalTimeImplementationName);
  }

  requester.join(
      options.memberFederateName + L"-mom-deletable-requester",
      options.federateType,
      federation);
  auto const requestClass = requester.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstancesThatCanBeDeleted");
  auto const requestFederate = requester.rtiAmbassador().getParameterHandle(
      requestClass,
      L"HLAfederate");
  auto const reportClass = requester.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectInstancesThatCanBeDeleted");
  auto const reportCounts = requester.rtiAmbassador().getParameterHandle(
      reportClass,
      L"HLAobjectInstanceCounts");
  auto const reliable = requester.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      requestClass.isValid() && requestFederate.isValid() && reportClass.isValid() &&
          reportCounts.isValid() && reliable.isValid() &&
          requester.rtiAmbassador().getInteractionClassName(requestClass) ==
              L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstancesThatCanBeDeleted" &&
          requester.rtiAmbassador().getInteractionClassName(reportClass) ==
              L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectInstancesThatCanBeDeleted" &&
          requester.rtiAmbassador().getParameterName(requestClass, requestFederate) ==
              L"HLAfederate" &&
          requester.rtiAmbassador().getParameterName(reportClass, reportCounts) ==
              L"HLAobjectInstanceCounts" &&
          requester.rtiAmbassador().getTransportationTypeName(reliable) ==
              L"HLAreliable",
      "Joined-federate MOM deletable-object report lookup did not round-trip standard handles");
  requester.rtiAmbassador().subscribeInteractionClass(reportClass);

  owner.join(
      options.ownerFederateName + L"-mom-deletable-owner",
      options.federateType,
      federation);
  auto const ownerObjectClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerObjectClass,
      options.attributeName);
  require(
      ownerObjectClass.isValid() && ownerAttribute.isValid(),
      "Joined-federate MOM deletable-object application lookup returned an invalid handle");
  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerObjectClass,
      ownerAttributes);

  auto decodeObjectClassCount = [&](rti::VariableLengthData const& encoded)
      -> std::int32_t {
    rti::HLAvariableArrayT<rti::HLAoctet> objectClassHandlePrototype;
    rti::HLAfixedRecord recordPrototype;
    recordPrototype.appendElement(objectClassHandlePrototype)
        .appendElement(rti::HLAinteger32BE{});
    rti::HLAvariableArray counts{recordPrototype};
    counts.decode(encoded);
    for (std::size_t index = 0; index != counts.size(); ++index) {
      auto const& record = dynamic_cast<rti::HLAfixedRecord const&>(counts.get(index));
      auto const& encodedObjectClass =
          dynamic_cast<rti::HLAvariableArray const&>(record.get(0));
      auto const decodedObjectClass = requester.rtiAmbassador().decodeObjectClassHandle(
          encodedObjectClass.encode());
      auto const& count = dynamic_cast<rti::HLAinteger32BE const&>(record.get(1));
      if (decodedObjectClass == ownerObjectClass) {
        return count.get();
      }
    }
    return 0;
  };

  auto requestCount = [&](std::int32_t expected, std::string const& description) {
    auto const before = requester.recorder().interactions().size();
    requester.rtiAmbassador().sendInteraction(
        requestClass,
        rti::ParameterHandleValueMap{
            {requestFederate, owner.federateHandle().encode()}},
        rti::VariableLengthData{});
    waitFor(
        requester,
        [&] {
          auto const interactions = requester.recorder().interactions();
          return interactions.size() > before &&
              std::any_of(
                  interactions.begin() + static_cast<std::ptrdiff_t>(before),
                  interactions.end(),
                  [&](InteractionRecord const& interaction) {
                    return interaction.present &&
                        interaction.interaction == reportClass &&
                        interaction.parameters.count(reportCounts) == 1U;
                  });
        },
        options,
        description);
    auto const interactions = requester.recorder().interactions();
    auto const iterator = std::find_if(
        interactions.begin() + static_cast<std::ptrdiff_t>(before),
        interactions.end(),
        [&](InteractionRecord const& interaction) {
          return interaction.present && interaction.interaction == reportClass &&
              interaction.parameters.count(reportCounts) == 1U;
        });
    require(iterator != interactions.end(), description + " produced no report");
    require(
        iterator->parameters.size() == 1U && iterator->transportation == reliable &&
            iterator->tag.empty() && !iterator->producer.isValid() &&
            !iterator->regions.has_value(),
        description + " returned non-standard MOM report metadata");
    require(
        decodeObjectClassCount(iterator->parameters.at(reportCounts)) == expected,
        description + " returned the wrong deletable-object count");
  };

  auto const firstObject = owner.rtiAmbassador().registerObjectInstance(ownerObjectClass);
  auto const secondObject = owner.rtiAmbassador().registerObjectInstance(ownerObjectClass);
  require(
      firstObject.isValid() && secondObject.isValid(),
      "Joined-federate MOM deletable-object registration returned an invalid handle");
  requestCount(
      2,
      "joined-federate MOM deletable-object report with two live objects");
  owner.rtiAmbassador().deleteObjectInstance(
      firstObject,
      rti::VariableLengthData{});
  requestCount(
      1,
      "joined-federate MOM deletable-object report after one deletion");
  owner.rtiAmbassador().deleteObjectInstance(
      secondObject,
      rti::VariableLengthData{});
  requestCount(
      0,
      "joined-federate MOM deletable-object report after all deletions");

  requester.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  owner.rtiAmbassador().unpublishObjectClassAttributes(
      ownerObjectClass,
      ownerAttributes);
  requester.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioJoinedFederateMomDeletableObjectReportContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioJoinedFederateMomDeletableObjectReport(options, model);
}

void scenarioJoinedFederateMomDeletableObjectCount(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Joined-federate MOM deletable-object count testing requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "Joined-federate MOM deletable-object count testing requires an adapter-supplied standard MIM");

  Session owner(options, model, "mom-deletable-count-owner");
  Session observer(options, model, "mom-deletable-count-observer");
  auto const federation = federationName(
      options,
      "joined-federate-mom-deletable-object-count");
  owner.connect();
  observer.connect();
  if (options.logicalTimeImplementationName.empty()) {
    owner.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring());
  } else {
    owner.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring(),
        options.logicalTimeImplementationName);
  }

  observer.join(
      options.memberFederateName + L"-mom-deletable-count-observer",
      options.federateType,
      federation);
  auto const observerFederateClass = observer.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const observerFederateHandleAttribute = observer.rtiAmbassador().getAttributeHandle(
      observerFederateClass,
      L"HLAfederateHandle");
  auto const observerDeletableCountAttribute = observer.rtiAmbassador().getAttributeHandle(
      observerFederateClass,
      L"HLAobjectInstancesThatCanBeDeleted");
  auto const reliable = observer.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      observerFederateClass.isValid() &&
          observer.rtiAmbassador().getObjectClassName(observerFederateClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederate" &&
          observerFederateHandleAttribute.isValid() &&
          observerDeletableCountAttribute.isValid() && reliable.isValid() &&
          observer.rtiAmbassador().getAttributeName(
              observerFederateClass,
              observerFederateHandleAttribute) == L"HLAfederateHandle" &&
          observer.rtiAmbassador().getAttributeName(
              observerFederateClass,
              observerDeletableCountAttribute) ==
              L"HLAobjectInstancesThatCanBeDeleted" &&
          observer.rtiAmbassador().getTransportationTypeName(reliable) ==
              L"HLAreliable",
      "Joined-federate MOM deletable-object count lookup did not round-trip standard handles");
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      observerFederateClass,
      rti::AttributeHandleSet{
          observerFederateHandleAttribute,
          observerDeletableCountAttribute},
      true,
      L"");

  owner.join(
      options.ownerFederateName + L"-mom-deletable-count-owner",
      options.federateType,
      federation);
  auto const ownerFederateClass = owner.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const ownerFederateHandleAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerFederateClass,
      L"HLAfederateHandle");
  auto const ownerDeletableCountAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerFederateClass,
      L"HLAobjectInstancesThatCanBeDeleted");
  require(
      ownerFederateClass == observerFederateClass &&
          ownerFederateHandleAttribute == observerFederateHandleAttribute &&
          ownerDeletableCountAttribute == observerDeletableCountAttribute,
      "Joined-federate MOM deletable-object count returned different handles to the joined federates");

  auto findOwnerMomReflection = [&]() -> std::optional<ReflectionRecord> {
    auto const reflections = observer.recorder().reflections();
    auto const expectedFederateHandle = copyBytes(owner.federateHandle().encode());
    auto const iterator = std::find_if(
        reflections.begin(),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          auto const value = reflection.values.find(observerFederateHandleAttribute);
          return reflection.present && value != reflection.values.end() &&
              copyBytes(value->second) == expectedFederateHandle;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return *iterator;
  };
  waitFor(
      observer,
      [&] { return findOwnerMomReflection().has_value(); },
      options,
      "joined-federate MOM deletable-object owner discovery");
  auto const ownerMomReflection = findOwnerMomReflection();
  require(
      ownerMomReflection.has_value() &&
          observer.rtiAmbassador().getKnownObjectClassHandle(
              ownerMomReflection->object) == observerFederateClass,
      "Joined-federate MOM deletable-object discovery returned the wrong standard object");
  auto const ownerMomObject = ownerMomReflection->object;

  auto requestDeletableCount = [&](std::int32_t expected,
                                   std::string const& description) {
    auto const before = observer.recorder().reflections().size();
    observer.rtiAmbassador().requestAttributeValueUpdate(
        ownerMomObject,
        rti::AttributeHandleSet{observerDeletableCountAttribute},
        rti::VariableLengthData{});
    waitFor(
        observer,
        [&] {
          auto const reflections = observer.recorder().reflections();
          return reflections.size() > before &&
              std::any_of(
                  reflections.begin() + static_cast<std::ptrdiff_t>(before),
                  reflections.end(),
                  [&](ReflectionRecord const& reflection) {
                    return reflection.object == ownerMomObject &&
                        reflection.values.count(observerDeletableCountAttribute) == 1U;
                  });
        },
        options,
        description);
    auto const reflections = observer.recorder().reflections();
    auto const iterator = std::find_if(
        reflections.begin() + static_cast<std::ptrdiff_t>(before),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          return reflection.object == ownerMomObject &&
              reflection.values.count(observerDeletableCountAttribute) == 1U;
        });
    require(iterator != reflections.end(), description + " produced no value");
    auto const& reflection = *iterator;
    require(
        reflection.values.size() == 1U && reflection.transportation == reliable &&
            reflection.tag.empty() && !reflection.producer.isValid() &&
            !reflection.regions.has_value(),
        description + " returned non-standard MOM reflection metadata");
    rti::HLAinteger32BE decoded;
    decoded.decode(reflection.values.at(observerDeletableCountAttribute));
    require(
        decoded.get() == expected,
        description + " returned the wrong deletable-object count");
  };

  requestDeletableCount(
      0,
      "joined-federate MOM initial deletable-object value request");

  auto const ownerObjectClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerObjectClass,
      options.attributeName);
  require(
      ownerObjectClass.isValid() && ownerAttribute.isValid(),
      "Joined-federate MOM deletable-object application lookup returned an invalid handle");
  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerObjectClass,
      ownerAttributes);
  auto const firstObject = owner.rtiAmbassador().registerObjectInstance(ownerObjectClass);
  require(
      firstObject.isValid(),
      "Joined-federate MOM first deletable-object registration returned an invalid handle");
  requestDeletableCount(
      1,
      "joined-federate MOM one-object value request");
  auto const secondObject = owner.rtiAmbassador().registerObjectInstance(ownerObjectClass);
  require(
      secondObject.isValid(),
      "Joined-federate MOM second deletable-object registration returned an invalid handle");
  requestDeletableCount(
      2,
      "joined-federate MOM two-object value request");
  owner.rtiAmbassador().deleteObjectInstance(
      firstObject,
      rti::VariableLengthData{});
  requestDeletableCount(
      1,
      "joined-federate MOM value request after one deletion");
  owner.rtiAmbassador().deleteObjectInstance(
      secondObject,
      rti::VariableLengthData{});
  requestDeletableCount(
      0,
      "joined-federate MOM value request after all deletions");

  observer.rtiAmbassador().unsubscribeObjectClassAttributes(
      observerFederateClass,
      rti::AttributeHandleSet{
          observerFederateHandleAttribute,
          observerDeletableCountAttribute});
  owner.rtiAmbassador().unpublishObjectClassAttributes(
      ownerObjectClass,
      ownerAttributes);
  observer.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  owner.disconnect();
}

void scenarioJoinedFederateMomDeletableObjectCountContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioJoinedFederateMomDeletableObjectCount(options, model);
}

void scenarioReceiveOrderAttributeUpdate(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Receive-order attribute-update testing requires an adapter-supplied FOM");

  Session publisher(options, model, "receive-order-attribute-publisher");
  Session receiver(options, model, "receive-order-attribute-receiver");
  auto const federation = federationName(
      options,
      "receive-order-attribute-update");
  connectAndJoin(publisher, receiver, options, federation, options.fom);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const receiverClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  auto const receiverAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass,
      options.attributeName);
  require(
      publisherClass.isValid() && receiverClass.isValid() &&
          publisherAttribute.isValid() && receiverAttribute.isValid(),
      "Receive-order attribute-update lookup returned an invalid standard handle");
  require(
      publisher.rtiAmbassador().getObjectClassName(publisherClass) ==
              options.objectClassName &&
          receiver.rtiAmbassador().getObjectClassName(receiverClass) ==
              options.objectClassName &&
          publisher.rtiAmbassador().getAttributeName(
              publisherClass,
              publisherAttribute) == options.attributeName &&
          receiver.rtiAmbassador().getAttributeName(
              receiverClass,
              receiverAttribute) == options.attributeName,
      "Receive-order attribute-update lookup did not round-trip adapter names");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const receiverAttributes{receiverAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes,
      true,
      L"");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      object.isValid(),
      "Receive-order attribute-update registration returned an invalid object handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "receive-order attribute-update discovery");
  receiver.recorder().clearReflection();
  publisher.recorder().clearReflection();

  std::vector<std::uint8_t> const firstValue{0x52U, 0x31U};
  std::vector<std::uint8_t> const firstTag{0x54U, 0x31U};
  std::vector<std::uint8_t> const secondValue{0x52U, 0x32U};
  std::vector<std::uint8_t> const secondTag{0x54U, 0x32U};
  auto send = [&](std::vector<std::uint8_t> const& value,
                  std::vector<std::uint8_t> const& tagBytes) {
    rti::AttributeHandleValueMap values;
    values.emplace(
        publisherAttribute,
        rti::VariableLengthData(value.data(), value.size()));
    rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
    publisher.rtiAmbassador().updateAttributeValues(object, values, tag);
  };
  send(firstValue, firstTag);
  send(secondValue, secondTag);

  if (model == rti::HLA_EVOKED) {
    require(
        receiver.recorder().reflections().empty(),
        "Receive-order attribute-update delivered before callback servicing");
  }
  waitFor(
      receiver,
      [&] { return receiver.recorder().reflections().size() >= 2U; },
      options,
      "receive-order attribute-update reflections");

  auto const received = receiver.recorder().reflections();
  require(
      received.size() == 2U,
      "Receive-order attribute-update delivered an unexpected callback count");
  auto assertReflection = [&](ReflectionRecord const& reflection,
                              std::vector<std::uint8_t> const& expectedValue,
                              std::vector<std::uint8_t> const& expectedTag,
                              std::string const& description) {
    require(
        reflection.present && reflection.object == object &&
            reflection.values.size() == 1U &&
            reflection.values.count(receiverAttribute) == 1U &&
            copyBytes(reflection.values.at(receiverAttribute)) == expectedValue,
        description + " returned the wrong object, attribute, or value");
    require(
        reflection.tag == expectedTag &&
            reflection.producer == publisher.federateHandle(),
        description + " returned the wrong tag or producing federate");
    require(
        reflection.transportation.isValid() &&
            !receiver.rtiAmbassador().getTransportationTypeName(
                reflection.transportation).empty() &&
            !reflection.regions.has_value(),
        description + " returned invalid receive-order reflection metadata");
  };
  assertReflection(
      received.front(),
      firstValue,
      firstTag,
      "first receive-order attribute-update reflection");
  assertReflection(
      received.back(),
      secondValue,
      secondTag,
      "second receive-order attribute-update reflection");
  require(
      publisher.recorder().reflections().empty(),
      "Receive-order attribute-update looped a reflection back to its publisher");

  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioReceiveOrderAttributeUpdateContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioReceiveOrderAttributeUpdate(options, model);
}

void scenarioReceiveOrderInteraction(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Receive-order interaction testing requires an adapter-supplied FOM");

  Session publisher(options, model, "receive-order-interaction-publisher");
  Session receiver(options, model, "receive-order-interaction-receiver");
  auto const federation = federationName(
      options,
      "receive-order-interaction");
  connectAndJoin(publisher, receiver, options, federation, options.fom);

  auto const publisherInteraction =
      publisher.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const receiverInteraction =
      receiver.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.parameterName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction,
      options.parameterName);
  require(
      publisherInteraction.isValid() && receiverInteraction.isValid() &&
          publisherParameter.isValid() && receiverParameter.isValid(),
      "Receive-order interaction lookup returned an invalid standard handle");
  require(
      publisher.rtiAmbassador().getInteractionClassName(publisherInteraction) ==
              options.interactionClassName &&
          receiver.rtiAmbassador().getInteractionClassName(receiverInteraction) ==
              options.interactionClassName &&
          publisher.rtiAmbassador().getParameterName(
              publisherInteraction,
              publisherParameter) == options.parameterName &&
          receiver.rtiAmbassador().getParameterName(
              receiverInteraction,
              receiverParameter) == options.parameterName,
      "Receive-order interaction lookup did not round-trip adapter names");

  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);

  std::vector<std::uint8_t> const firstValue{0x50U, 0x31U};
  std::vector<std::uint8_t> const firstTag{0x49U, 0x31U};
  std::vector<std::uint8_t> const secondValue{0x50U, 0x32U};
  std::vector<std::uint8_t> const secondTag{0x49U, 0x32U};
  auto send = [&](std::vector<std::uint8_t> const& value,
                  std::vector<std::uint8_t> const& tagBytes) {
    rti::ParameterHandleValueMap parameters;
    parameters.emplace(
        publisherParameter,
        rti::VariableLengthData(value.data(), value.size()));
    rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
    publisher.rtiAmbassador().sendInteraction(
        publisherInteraction,
        parameters,
        tag);
  };
  send(firstValue, firstTag);
  send(secondValue, secondTag);

  if (model == rti::HLA_EVOKED) {
    require(
        receiver.recorder().interactions().empty(),
        "Receive-order interaction delivered before callback servicing");
  }
  waitFor(
      receiver,
      [&] { return receiver.recorder().interactions().size() >= 2U; },
      options,
      "receive-order interaction callbacks");

  auto const received = receiver.recorder().interactions();
  require(
      received.size() == 2U,
      "Receive-order interaction delivered an unexpected callback count");
  auto assertInteraction = [&](InteractionRecord const& interaction,
                               std::vector<std::uint8_t> const& expectedValue,
                               std::vector<std::uint8_t> const& expectedTag,
                               std::string const& description) {
    require(
        interaction.present &&
            interaction.interaction == receiverInteraction &&
            interaction.parameters.size() == 1U &&
            interaction.parameters.count(receiverParameter) == 1U &&
            copyBytes(interaction.parameters.at(receiverParameter)) ==
                expectedValue,
        description + " returned the wrong class, parameter, or value");
    require(
        interaction.tag == expectedTag &&
            interaction.producer == publisher.federateHandle(),
        description + " returned the wrong tag or producing federate");
    require(
        interaction.transportation.isValid() &&
            !receiver.rtiAmbassador().getTransportationTypeName(
                interaction.transportation).empty() &&
            !interaction.regions.has_value(),
        description + " returned invalid receive-order interaction metadata");
  };
  assertInteraction(
      received.front(),
      firstValue,
      firstTag,
      "first receive-order interaction");
  assertInteraction(
      received.back(),
      secondValue,
      secondTag,
      "second receive-order interaction");
  require(
      publisher.recorder().interactions().empty(),
      "Receive-order interaction looped a callback back to its publisher");

  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  publisher.rtiAmbassador().unpublishInteractionClass(publisherInteraction);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioReceiveOrderInteractionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioReceiveOrderInteraction(options, model);
}

void scenarioReceiveOrderObjectRemoval(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Receive-order object-removal testing requires an adapter-supplied FOM");

  Session publisher(options, model, "receive-order-object-removal-publisher");
  Session receiver(options, model, "receive-order-object-removal-receiver");
  auto const federation = federationName(
      options,
      "receive-order-object-removal");
  connectAndJoin(publisher, receiver, options, federation, options.fom);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const receiverClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  auto const receiverAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass,
      options.attributeName);
  require(
      publisherClass.isValid() && receiverClass.isValid() &&
          publisherAttribute.isValid() && receiverAttribute.isValid(),
      "Receive-order object-removal lookup returned an invalid standard handle");
  require(
      publisher.rtiAmbassador().getObjectClassName(publisherClass) ==
              options.objectClassName &&
          receiver.rtiAmbassador().getObjectClassName(receiverClass) ==
              options.objectClassName &&
          publisher.rtiAmbassador().getAttributeName(
              publisherClass,
              publisherAttribute) == options.attributeName &&
          receiver.rtiAmbassador().getAttributeName(
              receiverClass,
              receiverAttribute) == options.attributeName,
      "Receive-order object-removal lookup did not round-trip adapter names");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const receiverAttributes{receiverAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes,
      true,
      L"");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass);
  require(
      object.isValid(),
      "Receive-order object-removal registration returned an invalid object handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "receive-order object-removal discovery");

  std::vector<std::uint8_t> const removalTag{0x44U, 0x31U};
  rti::VariableLengthData tag(removalTag.data(), removalTag.size());
  publisher.rtiAmbassador().deleteObjectInstance(object, tag);

  if (model == rti::HLA_EVOKED) {
    require(
        receiver.recorder().removals().empty(),
        "Receive-order object-removal delivered before callback servicing");
  }
  waitFor(
      receiver,
      [&] { return receiver.recorder().removals().size() >= 1U; },
      options,
      "receive-order object-removal callback");

  auto const removals = receiver.recorder().removals();
  require(
      removals.size() == 1U,
      "Receive-order object-removal delivered an unexpected callback count");
  require(
      removals.front().object == object &&
          removals.front().tag == removalTag &&
          removals.front().producer == publisher.federateHandle(),
      "Receive-order object-removal returned the wrong object, tag, or producer");
  require(
      publisher.recorder().removals().empty(),
      "Receive-order object-removal looped a callback back to its publisher");

  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioReceiveOrderObjectRemovalContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioReceiveOrderObjectRemoval(options, model);
}

void scenarioFederationRestoreAbort(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Federation restore-abort testing requires an adapter-supplied FOM");

  Session owner(options, model, "federation-restore-abort-owner");
  Session peer(options, model, "federation-restore-abort-peer");
  auto const federation = federationName(
      options,
      "federation-restore-abort");
  connectAndJoin(owner, peer, options, federation, options.fom);

  std::wstring const saveLabel = L"tck-restore-abort-save";
  auto const ownerSaveBefore = owner.federateSaveInitiations().size();
  auto const peerSaveBefore = peer.federateSaveInitiations().size();
  owner.rtiAmbassador().requestFederationSave(saveLabel);
  waitForSessions(
      {&owner, &peer},
      [&] {
        return owner.federateSaveInitiations().size() > ownerSaveBefore &&
            peer.federateSaveInitiations().size() > peerSaveBefore;
      },
      options,
      "restore-abort save initiation callbacks");
  require(
      owner.federateSaveInitiations().back() == saveLabel &&
          peer.federateSaveInitiations().back() == saveLabel,
      "restore-abort save initiation returned the wrong label");

  owner.rtiAmbassador().federateSaveBegun();
  peer.rtiAmbassador().federateSaveBegun();
  auto const ownerSavedBefore = owner.federationSavedCount();
  auto const peerSavedBefore = peer.federationSavedCount();
  owner.rtiAmbassador().federateSaveComplete();
  peer.rtiAmbassador().federateSaveComplete();
  waitForSessions(
      {&owner, &peer},
      [&] {
        return owner.federationSavedCount() > ownerSavedBefore &&
            peer.federationSavedCount() > peerSavedBefore;
      },
      options,
      "restore-abort save completion callbacks");
  require(
      owner.federationNotSavedReasons().empty() &&
          peer.federationNotSavedReasons().empty(),
      "restore-abort setup reported a save failure");

  auto const ownerRestoreBefore = owner.federateRestoreInitiations().size();
  auto const peerRestoreBefore = peer.federateRestoreInitiations().size();
  auto const ownerRestoreBegunBefore = owner.federationRestoreBegunCount();
  auto const peerRestoreBegunBefore = peer.federationRestoreBegunCount();
  owner.rtiAmbassador().requestFederationRestore(saveLabel);
  waitForSessions(
      {&owner, &peer},
      [&] {
        return owner.federationRestoreRequestsSucceeded().size() >= 1U &&
            owner.federationRestoreBegunCount() > ownerRestoreBegunBefore &&
            peer.federationRestoreBegunCount() > peerRestoreBegunBefore &&
            owner.federateRestoreInitiations().size() > ownerRestoreBefore &&
            peer.federateRestoreInitiations().size() > peerRestoreBefore;
      },
      options,
      "restore-abort restore initiation callbacks");
  require(
      owner.federationRestoreRequestsSucceeded().back() == saveLabel &&
          owner.federationRestoreRequestsFailed().empty() &&
          peer.federationRestoreRequestsFailed().empty(),
      "restore-abort restore request returned an unexpected result");

  auto const ownerRestore = owner.federateRestoreInitiations().back();
  auto const peerRestore = peer.federateRestoreInitiations().back();
  require(
      ownerRestore.label == saveLabel && peerRestore.label == saveLabel &&
          ownerRestore.federateName == options.ownerFederateName &&
          peerRestore.federateName == options.memberFederateName &&
          ownerRestore.postRestoreFederateHandle.isValid() &&
          peerRestore.postRestoreFederateHandle.isValid(),
      "restore-abort initiation returned incomplete restore metadata");

  auto const ownerNotRestoredBefore = owner.federationNotRestoredReasons().size();
  auto const peerNotRestoredBefore = peer.federationNotRestoredReasons().size();
  auto const ownerRestoredBefore = owner.federationRestoredCount();
  auto const peerRestoredBefore = peer.federationRestoredCount();
  owner.rtiAmbassador().abortFederationRestore();
  waitForSessions(
      {&owner, &peer},
      [&] {
        return owner.federationNotRestoredReasons().size() > ownerNotRestoredBefore &&
            peer.federationNotRestoredReasons().size() > peerNotRestoredBefore;
      },
      options,
      "restore-abort failure callbacks");
  require(
      owner.federationNotRestoredReasons().back() == rti::RESTORE_ABORTED &&
          peer.federationNotRestoredReasons().back() == rti::RESTORE_ABORTED &&
          owner.federationRestoredCount() == ownerRestoredBefore &&
          peer.federationRestoredCount() == peerRestoredBefore,
      "abortFederationRestore returned the wrong terminal callback result");

  auto const statusBefore = owner.federationRestoreStatusResponses().size();
  owner.rtiAmbassador().queryFederationRestoreStatus();
  waitFor(
      owner,
      [&] {
        return owner.federationRestoreStatusResponses().size() > statusBefore;
      },
      options,
      "restore-abort terminal restore status");
  auto const status = owner.federationRestoreStatusResponses().back();
  require(
      status.size() == 2U,
      "restore-abort terminal status omitted a joined federate");
  for (auto const& entry : status) {
    require(
        entry.status == rti::NO_RESTORE_IN_PROGRESS,
        "restore-abort left a restore in progress");
  }

  peer.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  peer.disconnect();
  owner.disconnect();
}

void scenarioFederationRestoreAbortContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederationRestoreAbort(options, model);
}

void scenarioFederationRestoreOwnershipAssumption(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Federation restore ownership-assumption testing requires an adapter-supplied FOM");

  Session owner(options, model, "owner");
  Session candidate(options, model, "member");
  auto const federation = federationName(
      options,
      "federation-restore-work-item-ownership-assumption");
  connectAndJoin(owner, candidate, options, federation, options.fom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const candidateClass = candidate.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  auto const candidateAttribute = candidate.rtiAmbassador().getAttributeHandle(
      candidateClass,
      options.attributeName);
  require(
      ownerClass.isValid() && candidateClass.isValid() &&
          ownerAttribute.isValid() && candidateAttribute.isValid(),
      "restore ownership-assumption lookup returned an invalid standard handle");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const candidateAttributes{candidateAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  candidate.rtiAmbassador().subscribeObjectClassAttributes(
      candidateClass,
      candidateAttributes,
      true,
      L"");
  candidate.rtiAmbassador().publishObjectClassAttributes(
      candidateClass,
      candidateAttributes);

  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "restore ownership-assumption registration returned an invalid object handle");
  if (model == rti::HLA_IMMEDIATE) {
    waitFor(
        candidate,
        [&] { return candidate.recorder().hasDiscovery(object); },
        options,
        "restore ownership-assumption object discovery");
  }

  std::vector<std::uint8_t> const assumptionTagBytes{
      0xD4U, 0x31U, 0x7BU, 0x0AU};
  rti::VariableLengthData const assumptionTag(
      assumptionTagBytes.data(),
      assumptionTagBytes.size());
  owner.recorder().clearOwnershipRecords();
  candidate.recorder().clearOwnershipRecords();
  owner.recorder().clearCallbackOrder();
  candidate.recorder().clearCallbackOrder();

  if (model == rti::HLA_IMMEDIATE) {
    owner.rtiAmbassador().disableCallbacks();
    candidate.rtiAmbassador().disableCallbacks();
  }
  owner.rtiAmbassador().unconditionalAttributeOwnershipDivestiture(
      object,
      ownerAttributes,
      assumptionTag);
  if (model == rti::HLA_EVOKED) {
    // A standard public service call admits pending pushed frames to the
    // HLA_EVOKED callback queue without invoking them.  Keep that queue
    // untouched through save and restore so the test proves preservation of
    // an unconsumed discovery/ownership-assumption work item.
    static_cast<void>(candidate.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
    require(
        !candidate.recorder().hasDiscovery(object) &&
            !candidate.recorder().ownershipAssumption().has_value(),
        "restore ownership-assumption work was delivered before callback servicing");
  }
  require(
      !candidate.recorder().ownershipAssumption().has_value(),
      "restore ownership-assumption work was delivered before the save");

  std::wstring const saveLabel =
      L"tck-federation-restore-work-item-ownership-assumption-save";
  auto serviceCallbacks = [&] {
    if (model == rti::HLA_EVOKED) {
      owner.pump();
      candidate.pump();
      return;
    }
    static_cast<void>(owner.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
    static_cast<void>(candidate.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  };
  auto waitForCallbacks = [&](auto predicate, std::string const& description) {
    auto const deadline = Clock::now() +
        std::chrono::milliseconds(options.timeoutMilliseconds);
    while (Clock::now() < deadline) {
      serviceCallbacks();
      if (predicate()) {
        return;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    throw std::runtime_error("Timed out waiting for " + description);
  };
  auto waitForOwnerCallbacks = [&](auto predicate, std::string const& description) {
    auto const deadline = Clock::now() +
        std::chrono::milliseconds(options.timeoutMilliseconds);
    while (Clock::now() < deadline) {
      owner.pump();
      if (predicate()) {
        return;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    throw std::runtime_error("Timed out waiting for " + description);
  };

  if (model == rti::HLA_EVOKED) {
    owner.rtiAmbassador().requestFederationSave(saveLabel);
    static_cast<void>(owner.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
    static_cast<void>(candidate.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
    waitForOwnerCallbacks(
        [&] {
          return owner.federateSaveInitiations().size() >= 1U;
        },
        "restore ownership-assumption owner save initiation callback");
    require(
        !candidate.recorder().hasDiscovery(object) &&
            !candidate.recorder().ownershipAssumption().has_value() &&
            candidate.federateSaveInitiations().empty(),
        "restore ownership-assumption candidate callback queue was serviced during save");
    owner.rtiAmbassador().federateSaveBegun();
    candidate.rtiAmbassador().federateSaveBegun();
    owner.rtiAmbassador().federateSaveComplete();
    candidate.rtiAmbassador().federateSaveComplete();
    waitForOwnerCallbacks(
        [&] {
          return owner.federationSavedCount() >= 1U;
        },
        "restore ownership-assumption owner save completion callback");
    require(
        !candidate.recorder().hasDiscovery(object) &&
            !candidate.recorder().ownershipAssumption().has_value() &&
            candidate.federateSaveInitiations().empty() &&
            candidate.federationSavedCount() == 0U,
        "restore ownership-assumption candidate callback queue was serviced before restore");

    owner.rtiAmbassador().requestFederationRestore(saveLabel);
    static_cast<void>(owner.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
    static_cast<void>(candidate.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
    owner.rtiAmbassador().federateRestoreComplete();
    candidate.rtiAmbassador().federateRestoreComplete();
    static_cast<void>(owner.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
    static_cast<void>(candidate.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
    require(
        !candidate.recorder().hasDiscovery(object) &&
            !candidate.recorder().ownershipAssumption().has_value() &&
            candidate.federateSaveInitiations().empty() &&
            candidate.federationSavedCount() == 0U &&
            candidate.federationRestoreBegunCount() == 0U &&
            candidate.federateRestoreInitiations().empty() &&
            candidate.federationRestoredCount() == 0U,
        "restore ownership-assumption callback queue was serviced before federation restore completion");
    waitForOwnerCallbacks(
        [&] {
          return owner.federationRestoreRequestsSucceeded().size() >= 1U &&
              owner.federationRestoreBegunCount() >= 1U &&
              owner.federateRestoreInitiations().size() >= 1U &&
              owner.federationRestoredCount() >= 1U;
        },
        "restore ownership-assumption owner restore callbacks");

    auto waitForCandidateCallbacks = [&](auto predicate,
                                         std::string const& description) {
      auto const deadline = Clock::now() +
          std::chrono::milliseconds(options.timeoutMilliseconds);
      while (Clock::now() < deadline) {
        candidate.pump();
        if (predicate()) {
          return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
      throw std::runtime_error("Timed out waiting for " + description);
    };
    waitForCandidateCallbacks(
        [&] { return candidate.recorder().hasDiscovery(object); },
        "restore ownership-assumption discovery callback");
    waitForCandidateCallbacks(
        [&] { return candidate.recorder().ownershipAssumption().has_value(); },
        "restore ownership-assumption callback");
    waitForCandidateCallbacks(
        [&] { return candidate.federateSaveInitiations().size() >= 1U; },
        "restore ownership-assumption save initiation callback");
    waitForCandidateCallbacks(
        [&] { return candidate.federationSavedCount() >= 1U; },
        "restore ownership-assumption save completion callback");
    waitForCandidateCallbacks(
        [&] { return candidate.federationRestoreBegunCount() >= 1U; },
        "restore ownership-assumption restore-begun callback");
    waitForCandidateCallbacks(
        [&] { return candidate.federateRestoreInitiations().size() >= 1U; },
        "restore ownership-assumption restore initiation callback");
    waitForCandidateCallbacks(
        [&] { return candidate.federationRestoredCount() >= 1U; },
        "restore ownership-assumption federation-restored callback");
  } else {
    owner.rtiAmbassador().requestFederationSave(saveLabel);
    owner.rtiAmbassador().federateSaveBegun();
    candidate.rtiAmbassador().federateSaveBegun();
    owner.rtiAmbassador().federateSaveComplete();
    candidate.rtiAmbassador().federateSaveComplete();
    require(
        owner.federationSavedCount() == 0U &&
            candidate.federationSavedCount() == 0U,
        "disabled callbacks exposed save completion");

    owner.rtiAmbassador().requestFederationRestore(saveLabel);
    static_cast<void>(owner.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
    require(
        owner.federationRestoredCount() == 0U &&
            candidate.federationRestoredCount() == 0U &&
            !candidate.recorder().ownershipAssumption().has_value(),
        "disabled callbacks exposed restore completion or ownership work");
    owner.rtiAmbassador().federateRestoreComplete();
    candidate.rtiAmbassador().federateRestoreComplete();
    owner.rtiAmbassador().enableCallbacks();
    candidate.rtiAmbassador().enableCallbacks();
    waitForCallbacks(
        [&] {
          return owner.federationRestoredCount() >= 1U &&
              candidate.federationRestoredCount() >= 1U &&
              candidate.recorder().ownershipAssumption().has_value();
        },
        "restored ownership-assumption immediate callbacks");
  }
  require(
      owner.federationRestoreRequestsSucceeded().size() >= 1U &&
          owner.federationRestoreRequestsSucceeded().back() == saveLabel &&
          owner.federationRestoreRequestsFailed().empty() &&
          candidate.federationRestoreRequestsFailed().empty(),
      "restore ownership-assumption request returned an unexpected result");
  auto const ownerRestore = owner.federateRestoreInitiations().back();
  auto const candidateRestore = candidate.federateRestoreInitiations().back();
  require(
      ownerRestore.label == saveLabel && candidateRestore.label == saveLabel &&
          ownerRestore.federateName == options.ownerFederateName &&
          candidateRestore.federateName == options.memberFederateName &&
          ownerRestore.postRestoreFederateHandle.isValid() &&
          candidateRestore.postRestoreFederateHandle.isValid(),
      "restore ownership-assumption initiation returned incomplete metadata");
  require(
      owner.federationNotRestoredReasons().empty() &&
          candidate.federationNotRestoredReasons().empty(),
      "restore ownership-assumption restore reported a failure");

  auto const assumption = candidate.recorder().ownershipAssumption();
  require(
      assumption->object == object &&
          assumption->attributes == candidateAttributes &&
          assumption->tag == assumptionTagBytes,
      "restored ownership-assumption callback returned the wrong metadata");
  require(
      !owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
          !candidate.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              candidateAttribute),
      "restored ownership-assumption work transferred ownership prematurely");
  require(
      owner.federationRestoredCount() >= 1U &&
          candidate.federationRestoredCount() >= 1U,
      "restored ownership-assumption work did not complete federation restore first");

  candidate.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  candidate.disconnect();
  owner.disconnect();
}

void scenarioFederationRestoreOwnershipAssumptionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederationRestoreOwnershipAssumption(options, model);
}

void scenarioOwnershipAcquisitionIfAvailable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "If Available ownership-acquisition testing requires an adapter-supplied FOM");

  Session owner(options, model, "owner");
  Session requester(options, model, "member");
  auto const federation = federationName(
      options,
      "ownership-acquisition-if-available");
  connectAndJoin(owner, requester, options, federation, options.fom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  auto const requesterAttribute = requester.rtiAmbassador().getAttributeHandle(
      requesterClass,
      options.attributeName);
  require(
      ownerClass.isValid() && requesterClass.isValid() &&
          ownerAttribute.isValid() && requesterAttribute.isValid(),
      "If Available ownership-acquisition lookup returned an invalid standard handle");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const requesterAttributes{requesterAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerClass,
      ownerAttributes);
  requester.rtiAmbassador().publishObjectClassAttributes(
      requesterClass,
      requesterAttributes);
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass,
      requesterAttributes,
      true,
      L"");

  auto const unavailableObject = owner.rtiAmbassador().registerObjectInstance(
      ownerClass);
  auto const availableObject = owner.rtiAmbassador().registerObjectInstance(
      ownerClass);
  require(
      unavailableObject.isValid() && availableObject.isValid(),
      "If Available ownership-acquisition registration returned an invalid object handle");
  waitFor(
      requester,
      [&] {
        return requester.recorder().hasDiscovery(unavailableObject) &&
            requester.recorder().hasDiscovery(availableObject);
      },
      options,
      "If Available ownership-acquisition object discovery");

  require(
      owner.rtiAmbassador().isAttributeOwnedByFederate(
          unavailableObject,
          ownerAttribute) &&
          owner.rtiAmbassador().isAttributeOwnedByFederate(
              availableObject,
              ownerAttribute) &&
          !requester.rtiAmbassador().isAttributeOwnedByFederate(
              unavailableObject,
              requesterAttribute) &&
          !requester.rtiAmbassador().isAttributeOwnedByFederate(
              availableObject,
              requesterAttribute),
      "If Available ownership-acquisition did not establish the standard initial ownership state");

  std::vector<std::uint8_t> const unavailableTagBytes{
      0x51U,
      0xA7U,
      0x0CU};
  rti::VariableLengthData const unavailableTag(
      unavailableTagBytes.data(),
      unavailableTagBytes.size());
  requester.recorder().clearOwnershipRecords();
  requester.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
      unavailableObject,
      requesterAttributes,
      unavailableTag);
  waitFor(
      requester,
      [&] { return requester.recorder().ownershipUnavailable().has_value(); },
      options,
      "If Available ownership-acquisition unavailable callback");
  auto const unavailable = requester.recorder().ownershipUnavailable();
  require(
      unavailable->object == unavailableObject &&
          unavailable->attributes == requesterAttributes &&
          unavailable->tag == unavailableTagBytes,
      "If Available ownership-acquisition unavailable callback returned the wrong metadata");
  require(
      !requester.recorder().ownershipAcquisition().has_value() &&
          owner.rtiAmbassador().isAttributeOwnedByFederate(
              unavailableObject,
              ownerAttribute) &&
          !requester.rtiAmbassador().isAttributeOwnedByFederate(
              unavailableObject,
              requesterAttribute),
      "If Available ownership-acquisition unavailable changed ownership");

  std::vector<std::uint8_t> const acquisitionTagBytes{
      0x3CU,
      0xA1U,
      0x7EU};
  rti::VariableLengthData const acquisitionTag(
      acquisitionTagBytes.data(),
      acquisitionTagBytes.size());
  requester.recorder().clearOwnershipRecords();
  owner.rtiAmbassador().unconditionalAttributeOwnershipDivestiture(
      availableObject,
      ownerAttributes,
      rti::VariableLengthData{});
  require(
      !owner.rtiAmbassador().isAttributeOwnedByFederate(
          availableObject,
          ownerAttribute) &&
          !requester.rtiAmbassador().isAttributeOwnedByFederate(
              availableObject,
              requesterAttribute),
      "If Available ownership-acquisition divestiture transferred ownership before the request");

  requester.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
      availableObject,
      requesterAttributes,
      acquisitionTag);
  waitFor(
      requester,
      [&] { return requester.recorder().ownershipAcquisition().has_value(); },
      options,
      "If Available ownership-acquisition notification");
  auto const acquisition = requester.recorder().ownershipAcquisition();
  require(
      acquisition->object == availableObject &&
          acquisition->attributes == requesterAttributes &&
          acquisition->tag == acquisitionTagBytes,
      "If Available ownership-acquisition notification returned the wrong metadata");
  require(
      !requester.recorder().ownershipUnavailable().has_value() &&
          !owner.rtiAmbassador().isAttributeOwnedByFederate(
              availableObject,
              ownerAttribute) &&
          requester.rtiAmbassador().isAttributeOwnedByFederate(
              availableObject,
              requesterAttribute),
      "If Available ownership-acquisition notification did not establish ownership");
  requireException(
      [&] {
        requester.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
            availableObject,
            requesterAttributes,
            acquisitionTag);
      },
      L"FederateOwnsAttributes",
      "repeating an If Available ownership-acquisition request after ownership transfer");

  requester.rtiAmbassador().unconditionalAttributeOwnershipDivestiture(
      availableObject,
      requesterAttributes,
      rti::VariableLengthData{});
  require(
      !requester.rtiAmbassador().isAttributeOwnedByFederate(
          availableObject,
          requesterAttribute),
      "If Available ownership-acquisition cleanup did not release the transferred attribute");
  requester.resign(rti::DELETE_OBJECTS);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioOwnershipAcquisitionIfAvailableContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioOwnershipAcquisitionIfAvailable(options, model);
}

void scenarioAutomaticResignDirectiveDeleteObjects(
    Options const& options,
    rti::CallbackModel model) {
  require(
      options.connectionLossServerManaged,
      "Automatic resignation on connection loss requires an adapter-managed fault fixture");
  Session survivor(options, model, "owner");
  Session lost(options, model, "member");
  auto const federation = federationName(
      options,
      "automatic-resign-directive-delete-objects");
  connectAndJoin(survivor, lost, options, federation, options.fom);

  auto const lostClass = lost.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const survivorClass = survivor.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const lostAttribute = lost.rtiAmbassador().getAttributeHandle(
      lostClass,
      options.attributeName);
  auto const survivorAttribute = survivor.rtiAmbassador().getAttributeHandle(
      survivorClass,
      options.attributeName);
  require(
      survivorClass.isValid() && lostClass.isValid() &&
          survivorAttribute.isValid() && lostAttribute.isValid(),
      "automatic resign directive lookup returned an invalid standard handle");

  rti::AttributeHandleSet const survivorAttributes{survivorAttribute};
  rti::AttributeHandleSet const lostAttributes{lostAttribute};
  lost.rtiAmbassador().publishObjectClassAttributes(
      lostClass,
      lostAttributes);
  survivor.rtiAmbassador().subscribeObjectClassAttributes(
      survivorClass,
      survivorAttributes,
      true,
      L"");

  auto const object = lost.rtiAmbassador().registerObjectInstance(lostClass);
  require(
      object.isValid(),
      "automatic resign directive registration returned an invalid object handle");
  auto const objectName = lost.rtiAmbassador().getObjectInstanceName(object);
  require(
      !objectName.empty(),
      "automatic resign directive registration returned an empty object name");
  waitFor(
      survivor,
      [&] { return survivor.recorder().hasDiscovery(object); },
      options,
      "automatic resign directive object discovery");
  lost.rtiAmbassador().setAutomaticResignDirective(rti::DELETE_OBJECTS);
  require(
      lost.rtiAmbassador().getAutomaticResignDirective() == rti::DELETE_OBJECTS,
      "automatic resign directive did not round-trip before connection loss");
  require(
      survivor.rtiAmbassador().getObjectInstanceHandle(objectName) == object,
      "automatic resign directive discovery did not preserve object-name lookup");

  survivor.recorder().clearRemovals();
  waitForConnectionLossAndSignal(
      lost,
      options,
      "automatic resign directive delete-object cleanup");
  waitFor(
      survivor,
      [&] {
        return survivor.recorder().hasRemoval(
            object,
            {},
            lost.federateHandle());
      },
      options,
      "automatic resign directive delete-object removal");

  auto const removals = survivor.recorder().removals();
  require(
      removals.size() == 1U,
      "automatic resign directive delivered a duplicate object removal callback");
  require(
      removals.front().object == object &&
          removals.front().tag.empty() &&
          removals.front().producer == lost.federateHandle(),
      "automatic resign directive returned the wrong removal metadata");
  requireException(
      [&] {
        static_cast<void>(survivor.rtiAmbassador().getObjectInstanceHandle(objectName));
      },
      L"ObjectInstanceNotKnown",
      "looking up an automatically deleted object");

  survivor.resign(rti::NO_ACTION);
  survivor.disconnect();
}

void scenarioAutomaticResignDirectiveDeleteObjectsContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAutomaticResignDirectiveDeleteObjects(options, model);
}

void scenarioPublicHandleDecoding(Options const& options, rti::CallbackModel model) {
  scenarioHandleLookups(options, model);
  scenarioHandleWireFormats(options, model);
}

void scenarioPublicHandleDecodingContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioPublicHandleDecoding(options, model);
}

void scenarioCustomTransportationAttributeDelivery(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Custom transportation attribute testing requires an adapter-supplied rich FOM");

  auto ordinary = options;
  ordinary.objectClassName = ordinary.customRegionalObjectClassName;
  ordinary.attributeName = ordinary.customRegionalAttributeName;
  ordinary.requireCustomTransportation = true;

  Session publisher(ordinary, model, "custom-transportation-attribute-publisher");
  Session subscriber(ordinary, model, "custom-transportation-attribute-subscriber");
  auto const federation = federationName(
      ordinary,
      "custom-transportation-attribute-delivery");
  connectAndJoin(publisher, subscriber, ordinary, federation, ordinary.modelFom);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      ordinary.objectClassName);
  auto const subscriberClass = subscriber.rtiAmbassador().getObjectClassHandle(
      ordinary.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      ordinary.attributeName);
  auto const subscriberAttribute = subscriber.rtiAmbassador().getAttributeHandle(
      subscriberClass,
      ordinary.attributeName);
  require(
      publisherClass.isValid() && subscriberClass.isValid() &&
          publisherAttribute.isValid() && subscriberAttribute.isValid(),
      "custom transportation ordinary attribute lookup returned an invalid handle");
  require(
      publisher.rtiAmbassador().getObjectClassName(publisherClass) ==
              ordinary.objectClassName &&
          subscriber.rtiAmbassador().getObjectClassName(subscriberClass) ==
              ordinary.objectClassName &&
          publisher.rtiAmbassador().getAttributeName(
              publisherClass,
              publisherAttribute) == ordinary.attributeName &&
          subscriber.rtiAmbassador().getAttributeName(
              subscriberClass,
              subscriberAttribute) == ordinary.attributeName,
      "custom transportation ordinary attribute lookup did not round-trip names");

  auto const publisherTransportation = publisher.rtiAmbassador().getTransportationTypeHandle(
      ordinary.fomCustomTransportationName);
  auto const subscriberTransportation = subscriber.rtiAmbassador().getTransportationTypeHandle(
      ordinary.fomCustomTransportationName);
  require(
      publisherTransportation.isValid() && subscriberTransportation.isValid() &&
          publisher.rtiAmbassador().getTransportationTypeName(publisherTransportation) ==
              ordinary.fomCustomTransportationName &&
          subscriber.rtiAmbassador().getTransportationTypeName(subscriberTransportation) ==
              ordinary.fomCustomTransportationName,
      "custom transportation ordinary attribute lookup did not round-trip the transport name");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const subscriberAttributes{subscriberAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass,
      subscriberAttributes,
      true,
      L"");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      object.isValid(),
      "custom transportation ordinary attribute registration returned an invalid handle");
  auto const objectName = publisher.rtiAmbassador().getObjectInstanceName(object);
  require(
      !objectName.empty() &&
          publisher.rtiAmbassador().getObjectInstanceHandle(objectName) == object,
      "custom transportation ordinary attribute registration did not preserve object naming");
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().hasDiscovery(object); },
      ordinary,
      "custom transportation ordinary attribute discovery");
  auto const discovery = subscriber.recorder().discovery();
  require(
      discovery.object == object &&
          discovery.objectClass == subscriberClass &&
          discovery.producer == publisher.federateHandle() &&
          !discovery.name.empty() &&
          subscriber.rtiAmbassador().getObjectInstanceHandle(discovery.name) == object &&
          subscriber.rtiAmbassador().getKnownObjectClassHandle(object) == subscriberClass,
      "custom transportation ordinary attribute discovery changed standard object metadata");

  std::vector<std::uint8_t> const valueBytes{0xA1U, 0xB2U, 0xC3U};
  std::vector<std::uint8_t> const tagBytes{0x54U, 0x41U, 0x47U};
  rti::AttributeHandleValueMap values;
  values.emplace(
      publisherAttribute,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  publisher.rtiAmbassador().updateAttributeValues(object, values, tag);
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().reflection().present; },
      ordinary,
      "custom transportation ordinary attribute reflection");

  auto const reflection = subscriber.recorder().reflection();
  require(
      reflection.object == object &&
          reflection.values.size() == 1U &&
          reflection.values.count(subscriberAttribute) == 1U &&
          copyBytes(reflection.values.at(subscriberAttribute)) == valueBytes &&
          reflection.tag == tagBytes &&
          reflection.producer == publisher.federateHandle(),
      "custom transportation ordinary attribute reflection changed value or metadata");
  require(
      reflection.transportation.isValid() &&
          subscriber.rtiAmbassador().getTransportationTypeName(
              reflection.transportation) == ordinary.fomCustomTransportationName,
      "custom transportation ordinary attribute reflection changed transportation identity");

  subscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      subscriberClass,
      subscriberAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  publisher.resign(rti::DELETE_OBJECTS);
  subscriber.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  subscriber.disconnect();
  publisher.disconnect();
}

void scenarioCustomTransportationAttributeDeliveryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCustomTransportationAttributeDelivery(options, model);
}

void scenarioCustomTransportationTimestampedAttributeDelivery(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Custom transportation timestamped attribute testing requires an adapter-supplied rich FOM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Custom transportation timestamped attribute testing requires an adapter-supplied logical-time implementation");

  auto timestamped = options;
  timestamped.objectClassName = timestamped.customRegionalObjectClassName;
  timestamped.attributeName = timestamped.customRegionalAttributeName;
  timestamped.requireCustomTransportation = true;

  Session publisher(
      timestamped,
      model,
      "custom-transportation-timestamped-attribute-publisher");
  Session immediate(
      timestamped,
      model,
      "custom-transportation-timestamped-attribute-immediate");
  Session constrained(
      timestamped,
      model,
      "custom-transportation-timestamped-attribute-constrained");
  auto const federation = federationName(
      timestamped,
      "custom-transportation-timestamped-attribute-delivery");
  connectAndJoin(
      publisher,
      immediate,
      timestamped,
      federation,
      timestamped.modelFom);
  constrained.connect();
  constrained.join(
      timestamped.memberFederateName + L"-custom-transportation-timestamped-attribute-constrained",
      timestamped.federateType,
      federation);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      timestamped.objectClassName);
  auto const immediateClass = immediate.rtiAmbassador().getObjectClassHandle(
      timestamped.objectClassName);
  auto const constrainedClass = constrained.rtiAmbassador().getObjectClassHandle(
      timestamped.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      timestamped.attributeName);
  auto const immediateAttribute = immediate.rtiAmbassador().getAttributeHandle(
      immediateClass,
      timestamped.attributeName);
  auto const constrainedAttribute = constrained.rtiAmbassador().getAttributeHandle(
      constrainedClass,
      timestamped.attributeName);
  require(
      publisherClass.isValid() && immediateClass.isValid() &&
          constrainedClass.isValid() && publisherAttribute.isValid() &&
          immediateAttribute.isValid() && constrainedAttribute.isValid(),
      "custom transportation timestamped attribute lookup returned an invalid handle");
  require(
      publisher.rtiAmbassador().getObjectClassName(publisherClass) ==
              timestamped.objectClassName &&
          immediate.rtiAmbassador().getObjectClassName(immediateClass) ==
              timestamped.objectClassName &&
          constrained.rtiAmbassador().getObjectClassName(constrainedClass) ==
              timestamped.objectClassName &&
          publisher.rtiAmbassador().getAttributeName(
              publisherClass,
              publisherAttribute) == timestamped.attributeName &&
          immediate.rtiAmbassador().getAttributeName(
              immediateClass,
              immediateAttribute) == timestamped.attributeName &&
          constrained.rtiAmbassador().getAttributeName(
              constrainedClass,
              constrainedAttribute) == timestamped.attributeName,
      "custom transportation timestamped attribute lookup did not round-trip names");

  auto const publisherTransportation = publisher.rtiAmbassador().getTransportationTypeHandle(
      timestamped.fomCustomTransportationName);
  auto const immediateTransportation = immediate.rtiAmbassador().getTransportationTypeHandle(
      timestamped.fomCustomTransportationName);
  auto const constrainedTransportation = constrained.rtiAmbassador().getTransportationTypeHandle(
      timestamped.fomCustomTransportationName);
  require(
      publisherTransportation.isValid() && immediateTransportation.isValid() &&
          constrainedTransportation.isValid() &&
          publisher.rtiAmbassador().getTransportationTypeName(publisherTransportation) ==
              timestamped.fomCustomTransportationName &&
          immediate.rtiAmbassador().getTransportationTypeName(immediateTransportation) ==
              timestamped.fomCustomTransportationName &&
          constrained.rtiAmbassador().getTransportationTypeName(constrainedTransportation) ==
              timestamped.fomCustomTransportationName,
      "custom transportation timestamped attribute lookup did not round-trip the transport name");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const immediateAttributes{immediateAttribute};
  rti::AttributeHandleSet const constrainedAttributes{constrainedAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherClass,
      publisherAttributes,
      rti::TIMESTAMP);
  immediate.rtiAmbassador().subscribeObjectClassAttributes(
      immediateClass,
      immediateAttributes,
      true,
      L"");
  constrained.rtiAmbassador().subscribeObjectClassAttributes(
      constrainedClass,
      constrainedAttributes,
      true,
      L"");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      object.isValid(),
      "custom transportation timestamped attribute registration returned an invalid handle");
  waitForSessions(
      {&publisher, &immediate, &constrained},
      [&] {
        return immediate.recorder().hasDiscovery(object) &&
            constrained.recorder().hasDiscovery(object);
      },
      timestamped,
      "custom transportation timestamped attribute discovery");
  require(
      immediate.recorder().discovery().object == object &&
          constrained.recorder().discovery().object == object &&
          immediate.recorder().discovery().objectClass == immediateClass &&
          constrained.recorder().discovery().objectClass == constrainedClass,
      "custom transportation timestamped attribute discovery changed object metadata");

  auto publisherTime = makeTimeContext(publisher);
  auto constrainedTime = makeTimeContext(constrained);
  enableTimestampedRoles(
      publisher,
      constrained,
      publisherTime,
      constrainedTime,
      timestamped,
      "custom transportation timestamped attribute");

  auto assertReflection = [&](TimedReflectionRecord const& record,
                              rti::AttributeHandle const& expectedAttribute,
                              std::vector<std::uint8_t> const& expectedValue,
                              std::vector<std::uint8_t> const& expectedTag,
                              std::vector<std::uint8_t> const& expectedTime,
                              rti::OrderType expectedReceivedOrder,
                              Session& inspector) {
    require(
        record.object == object && record.values.size() == 1U &&
            record.values.count(expectedAttribute) == 1U,
        "custom transportation timestamped attribute reflection returned the wrong object or attribute set");
    require(
        copyBytes(record.values.at(expectedAttribute)) == expectedValue &&
            record.tag == expectedTag && record.time == expectedTime,
        "custom transportation timestamped attribute reflection changed value, tag, or time");
    require(
        record.producer == publisher.federateHandle() && !record.timeText.empty(),
        "custom transportation timestamped attribute reflection changed producer or time metadata");
    require(
        record.sentOrder == rti::TIMESTAMP &&
            record.receivedOrder == expectedReceivedOrder,
        "custom transportation timestamped attribute reflection changed order metadata");
    require(
        record.transportation.isValid() &&
            inspector.rtiAmbassador().getTransportationTypeName(record.transportation) ==
                timestamped.fomCustomTransportationName,
        "custom transportation timestamped attribute reflection changed transportation identity");
  };

  auto const firstTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      3U);
  auto const firstConstrainedTime = timeAfter(
      *constrainedTime.factory,
      *constrainedTime.initial,
      *constrainedTime.epsilon,
      3U);
  std::vector<std::uint8_t> const firstValueBytes{0x41U, 0x42U, 0x43U};
  std::vector<std::uint8_t> const firstTagBytes{0x51U, 0x52U};
  rti::AttributeHandleValueMap firstValues;
  firstValues.emplace(
      publisherAttribute,
      rti::VariableLengthData(firstValueBytes.data(), firstValueBytes.size()));
  rti::VariableLengthData const firstTag(firstTagBytes.data(), firstTagBytes.size());
  auto const firstRetraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      firstValues,
      firstTag,
      *firstTime);
  require(
      firstRetraction.isValid(),
      "custom transportation timestamped attribute update returned an invalid retraction handle");
  require(
      constrained.recorder().timedReflections().empty(),
      "custom transportation timestamped attribute delivered before the constrained grant");
  waitFor(
      immediate,
      [&] { return immediate.recorder().timedReflections().size() >= 1U; },
      timestamped,
      "custom transportation timestamped attribute immediate reflection");
  auto const firstImmediate = immediate.recorder().timedReflections();
  require(
      firstImmediate.size() == 1U,
      "custom transportation timestamped attribute delivered a duplicate immediate reflection");
  assertReflection(
      firstImmediate.front(),
      immediateAttribute,
      firstValueBytes,
      firstTagBytes,
      encodeTime(*firstTime),
      rti::RECEIVE,
      immediate);
  require(
      firstImmediate.front().retractionPresent &&
          firstImmediate.front().retraction == copyBytes(firstRetraction.encode()),
      "custom transportation timestamped attribute reflection returned the wrong retraction handle");

  publisher.rtiAmbassador().retract(firstRetraction);
  waitFor(
      immediate,
      [&] { return immediate.recorder().retractions().size() >= 1U; },
      timestamped,
      "custom transportation timestamped attribute retraction callback");
  auto const firstRetractionCallbacks = immediate.recorder().retractions();
  require(
      firstRetractionCallbacks.size() == 1U &&
          firstRetractionCallbacks.front().valid &&
          firstRetractionCallbacks.front().encoded == copyBytes(firstRetraction.encode()),
      "custom transportation timestamped attribute retraction callback returned the wrong handle");
  require(
      constrained.recorder().timedReflections().empty() &&
          constrained.recorder().retractions().empty(),
      "custom transportation timestamped attribute retraction leaked to the pending receiver");

  constrained.rtiAmbassador().timeAdvanceRequest(*firstConstrainedTime);
  publisher.rtiAmbassador().timeAdvanceRequest(*firstTime);
  waitForSessions(
      {&publisher, &constrained},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            constrained.recorder().timeAdvanceGrants().size() >= 1U;
      },
      timestamped,
      "custom transportation timestamped attribute first time-advance grants");
  for (int pass = 0; pass != 8; ++pass) {
    constrained.pump();
  }
  require(
      constrained.recorder().timedReflections().empty(),
      "custom transportation timestamped attribute delivered a retracted update at its grant");

  auto const secondTime = timeAfter(
      *publisherTime.factory,
      *firstTime,
      *publisherTime.epsilon,
      3U);
  auto const secondConstrainedTime = timeAfter(
      *constrainedTime.factory,
      *firstConstrainedTime,
      *constrainedTime.epsilon,
      3U);
  std::vector<std::uint8_t> const secondValueBytes{0x61U, 0x62U, 0x63U};
  std::vector<std::uint8_t> const secondTagBytes{0x71U, 0x72U};
  rti::AttributeHandleValueMap secondValues;
  secondValues.emplace(
      publisherAttribute,
      rti::VariableLengthData(secondValueBytes.data(), secondValueBytes.size()));
  rti::VariableLengthData const secondTag(secondTagBytes.data(), secondTagBytes.size());
  auto const secondRetraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      secondValues,
      secondTag,
      *secondTime);
  require(
      secondRetraction.isValid(),
      "custom transportation timestamped attribute second update returned an invalid retraction handle");
  waitFor(
      immediate,
      [&] { return immediate.recorder().timedReflections().size() >= 2U; },
      timestamped,
      "custom transportation timestamped attribute second immediate reflection");
  auto const secondImmediate = immediate.recorder().timedReflections();
  require(
      secondImmediate.size() == 2U,
      "custom transportation timestamped attribute delivered a duplicate second reflection");
  assertReflection(
      secondImmediate.at(1),
      immediateAttribute,
      secondValueBytes,
      secondTagBytes,
      encodeTime(*secondTime),
      rti::RECEIVE,
      immediate);
  require(
      secondImmediate.at(1).retractionPresent &&
          secondImmediate.at(1).retraction == copyBytes(secondRetraction.encode()),
      "custom transportation timestamped attribute second reflection returned the wrong retraction handle");

  constrained.rtiAmbassador().timeAdvanceRequest(*secondConstrainedTime);
  publisher.rtiAmbassador().timeAdvanceRequest(*secondTime);
  waitForSessions(
      {&publisher, &constrained},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 2U &&
            constrained.recorder().timeAdvanceGrants().size() >= 2U &&
            constrained.recorder().timedReflections().size() >= 1U;
      },
      timestamped,
      "custom transportation timestamped attribute second delivery and grants");
  auto const constrainedReflections = constrained.recorder().timedReflections();
  require(
      constrainedReflections.size() == 1U,
      "custom transportation timestamped attribute delivered the wrong number of constrained reflections");
  assertReflection(
      constrainedReflections.front(),
      constrainedAttribute,
      secondValueBytes,
      secondTagBytes,
      encodeTime(*secondTime),
      rti::TIMESTAMP,
      constrained);
  require(
      constrainedReflections.front().retractionPresent &&
          constrainedReflections.front().retraction == copyBytes(secondRetraction.encode()),
      "custom transportation timestamped attribute constrained reflection returned the wrong retraction handle");
  requireException(
      [&] { publisher.rtiAmbassador().retract(secondRetraction); },
      L"MessageCanNoLongerBeRetracted",
      "retracting a delivered custom transportation timestamped attribute update");

  constrained.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  immediate.rtiAmbassador().unsubscribeObjectClassAttributes(
      immediateClass,
      immediateAttributes);
  constrained.rtiAmbassador().unsubscribeObjectClassAttributes(
      constrainedClass,
      constrainedAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  constrained.resign(rti::NO_ACTION);
  immediate.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  constrained.disconnect();
  immediate.disconnect();
  publisher.disconnect();
}

void scenarioCustomTransportationTimestampedAttributeDeliveryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCustomTransportationTimestampedAttributeDelivery(options, model);
}

void scenarioCustomTransportationTimestampedAttributeAlternateAdvances(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Custom transportation timestamped attribute alternate-advance testing requires an adapter-supplied rich FOM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Custom transportation timestamped attribute alternate-advance testing requires an adapter-supplied logical-time implementation");

  auto alternate = options;
  alternate.fom = alternate.modelFom;
  alternate.objectClassName = alternate.customRegionalObjectClassName;
  alternate.attributeName = alternate.customRegionalAttributeName;
  alternate.requireCustomTransportation = true;

  // The established standard oracle covers the three alternate advance
  // services, grant ordering, query boundaries, and retraction terminalization.
  scenarioTimestampedAttributeUpdateAlternateAdvances(alternate, model);

  Session publisher(
      alternate,
      model,
      "custom-transportation-timestamped-attribute-alternate-publisher");
  Session flushQueue(
      alternate,
      model,
      "custom-transportation-timestamped-attribute-alternate-flush-queue");
  Session timeAdvanceAvailable(
      alternate,
      model,
      "custom-transportation-timestamped-attribute-alternate-tara");
  Session nextMessageAvailable(
      alternate,
      model,
      "custom-transportation-timestamped-attribute-alternate-nmra");
  auto const federation = federationName(
      alternate,
      "custom-transportation-timestamped-attribute-alternate-advances");

  publisher.connect();
  flushQueue.connect();
  timeAdvanceAvailable.connect();
  nextMessageAvailable.connect();
  publisher.rtiAmbassador().createFederationExecution(
      federation,
      alternate.modelFom.wstring(),
      alternate.logicalTimeImplementationName);
  publisher.join(alternate.ownerFederateName, alternate.federateType, federation);
  flushQueue.join(
      alternate.memberFederateName + L"-custom-transportation-flush-queue",
      alternate.federateType,
      federation);
  timeAdvanceAvailable.join(
      alternate.memberFederateName + L"-custom-transportation-tara",
      alternate.federateType,
      federation);
  nextMessageAvailable.join(
      alternate.memberFederateName + L"-custom-transportation-nmra",
      alternate.federateType,
      federation);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      alternate.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      alternate.attributeName);
  require(
      publisherClass.isValid() && publisherAttribute.isValid() &&
          publisher.rtiAmbassador().getObjectClassName(publisherClass) ==
              alternate.objectClassName &&
          publisher.rtiAmbassador().getAttributeName(
              publisherClass,
              publisherAttribute) == alternate.attributeName,
      "custom transportation timestamped attribute alternate-advance publisher lookup did not round-trip");

  auto const publisherTransportation =
      publisher.rtiAmbassador().getTransportationTypeHandle(
          alternate.fomCustomTransportationName);
  require(
      publisherTransportation.isValid() &&
          publisher.rtiAmbassador().getTransportationTypeName(
              publisherTransportation) == alternate.fomCustomTransportationName,
      "custom transportation timestamped attribute alternate-advance publisher transport lookup did not round-trip");

  auto const members = std::vector<Session*>{
      &flushQueue,
      &timeAdvanceAvailable,
      &nextMessageAvailable,
  };
  std::vector<rti::ObjectClassHandle> memberClasses;
  std::vector<rti::AttributeHandle> memberAttributes;
  memberClasses.reserve(members.size());
  memberAttributes.reserve(members.size());
  for (auto* member : members) {
    auto const objectClass = member->rtiAmbassador().getObjectClassHandle(
        alternate.objectClassName);
    auto const attribute = member->rtiAmbassador().getAttributeHandle(
        objectClass,
        alternate.attributeName);
    auto const transportation = member->rtiAmbassador().getTransportationTypeHandle(
        alternate.fomCustomTransportationName);
    require(
        objectClass.isValid() && attribute.isValid() && transportation.isValid() &&
            member->rtiAmbassador().getObjectClassName(objectClass) ==
                alternate.objectClassName &&
            member->rtiAmbassador().getAttributeName(objectClass, attribute) ==
                alternate.attributeName &&
            member->rtiAmbassador().getTransportationTypeName(transportation) ==
                alternate.fomCustomTransportationName,
        "custom transportation timestamped attribute alternate-advance member lookup did not round-trip");
    memberClasses.push_back(objectClass);
    memberAttributes.push_back(attribute);
  }

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherClass,
      publisherAttributes,
      rti::TIMESTAMP);
  for (std::size_t index = 0U; index != members.size(); ++index) {
    members.at(index)->rtiAmbassador().subscribeObjectClassAttributes(
        memberClasses.at(index),
        rti::AttributeHandleSet{memberAttributes.at(index)},
        true,
        L"");
  }

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      object.isValid(),
      "custom transportation timestamped attribute alternate-advance registration returned an invalid object handle");
  waitForSessions(
      {&publisher, &flushQueue, &timeAdvanceAvailable, &nextMessageAvailable},
      [&] {
        return flushQueue.recorder().hasDiscovery(object) &&
            timeAdvanceAvailable.recorder().hasDiscovery(object) &&
            nextMessageAvailable.recorder().hasDiscovery(object);
      },
      alternate,
      "custom transportation timestamped attribute alternate-advance discovery");

  auto publisherTime = makeTimeContext(publisher);
  auto flushQueueTime = makeTimeContext(flushQueue);
  auto timeAdvanceAvailableTime = makeTimeContext(timeAdvanceAvailable);
  auto nextMessageAvailableTime = makeTimeContext(nextMessageAvailable);
  require(
      publisherTime.factory->getName() == flushQueueTime.factory->getName() &&
          publisherTime.factory->getName() == timeAdvanceAvailableTime.factory->getName() &&
          publisherTime.factory->getName() == nextMessageAvailableTime.factory->getName(),
      "custom transportation timestamped attribute alternate-advance members selected different logical-time factories");

  for (auto* member : members) {
    member->rtiAmbassador().enableTimeConstrained();
    waitFor(
        *member,
        [&] { return member->recorder().timeConstrainedEnabled().size() >= 1U; },
        alternate,
        "custom transportation timestamped attribute alternate-advance time-constrained callback");
  }
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      alternate,
      "custom transportation timestamped attribute alternate-advance time-regulation callback");

  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      4U);
  auto const requestBoundary = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      10U);
  std::vector<std::uint8_t> const valueBytes{0x43U, 0x55U, 0x53U, 0x54U};
  std::vector<std::uint8_t> const tagBytes{0x41U, 0x4CU, 0x54U};
  rti::AttributeHandleValueMap values;
  values.emplace(
      publisherAttribute,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      values,
      tag,
      *messageTime);
  require(
      retraction.isValid(),
      "custom transportation timestamped attribute alternate-advance update returned an invalid retraction handle");
  for (auto* member : members) {
    require(
        member->recorder().timedReflections().empty(),
        "custom transportation timestamped attribute alternate-advance delivered before its request");
  }

  flushQueue.rtiAmbassador().flushQueueRequest(*requestBoundary);
  timeAdvanceAvailable.rtiAmbassador().timeAdvanceRequestAvailable(*messageTime);
  nextMessageAvailable.rtiAmbassador().nextMessageRequestAvailable(*requestBoundary);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  waitForSessions(
      {&publisher, &flushQueue, &timeAdvanceAvailable, &nextMessageAvailable},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            flushQueue.recorder().timedReflections().size() >= 1U &&
            flushQueue.recorder().flushQueueGrants().size() >= 1U &&
            timeAdvanceAvailable.recorder().timedReflections().size() >= 1U &&
            timeAdvanceAvailable.recorder().timeAdvanceGrants().size() >= 1U &&
            nextMessageAvailable.recorder().timedReflections().size() >= 1U &&
            nextMessageAvailable.recorder().timeAdvanceGrants().size() >= 1U;
      },
      alternate,
      "custom transportation timestamped attribute alternate-advance delivery and grants");

  auto assertReflection = [&](TimedReflectionRecord const& record,
                              rti::AttributeHandle const& expectedAttribute,
                              Session& inspector) {
    require(
        record.object == object && record.values.size() == 1U &&
            record.values.count(expectedAttribute) == 1U &&
            copyBytes(record.values.at(expectedAttribute)) == valueBytes &&
            record.tag == tagBytes && record.time == encodeTime(*messageTime),
        "custom transportation timestamped attribute alternate-advance callback changed the payload");
    require(
        record.producer == publisher.federateHandle() && !record.timeText.empty() &&
            record.sentOrder == rti::TIMESTAMP &&
            record.receivedOrder == rti::TIMESTAMP,
        "custom transportation timestamped attribute alternate-advance callback changed time or order metadata");
    require(
        record.transportation.isValid() &&
            inspector.rtiAmbassador().getTransportationTypeName(
                record.transportation) == alternate.fomCustomTransportationName,
        "custom transportation timestamped attribute alternate-advance callback changed transportation identity");
    require(
        record.retractionPresent &&
            record.retraction == copyBytes(retraction.encode()),
        "custom transportation timestamped attribute alternate-advance callback returned the wrong retraction handle");
  };
  require(
      flushQueue.recorder().timedReflections().size() == 1U &&
          timeAdvanceAvailable.recorder().timedReflections().size() == 1U &&
          nextMessageAvailable.recorder().timedReflections().size() == 1U,
      "custom transportation timestamped attribute alternate-advance delivered an unexpected callback count");
  assertReflection(
      flushQueue.recorder().timedReflections().front(),
      memberAttributes.at(0),
      flushQueue);
  assertReflection(
      timeAdvanceAvailable.recorder().timedReflections().front(),
      memberAttributes.at(1),
      timeAdvanceAvailable);
  assertReflection(
      nextMessageAvailable.recorder().timedReflections().front(),
      memberAttributes.at(2),
      nextMessageAvailable);
  requireException(
      [&] { publisher.rtiAmbassador().retract(retraction); },
      L"MessageCanNoLongerBeRetracted",
      "retracting a delivered custom transportation timestamped attribute alternate-advance update");

  for (auto* member : members) {
    member->rtiAmbassador().disableTimeConstrained();
  }
  publisher.rtiAmbassador().disableTimeRegulation();
  for (std::size_t index = 0U; index != members.size(); ++index) {
    members.at(index)->rtiAmbassador().unsubscribeObjectClassAttributes(
        memberClasses.at(index),
        rti::AttributeHandleSet{memberAttributes.at(index)});
  }
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  nextMessageAvailable.resign(rti::NO_ACTION);
  timeAdvanceAvailable.resign(rti::NO_ACTION);
  flushQueue.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  nextMessageAvailable.disconnect();
  timeAdvanceAvailable.disconnect();
  flushQueue.disconnect();
  publisher.disconnect();
}

void scenarioCustomTransportationTimestampedAttributeAlternateAdvancesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCustomTransportationTimestampedAttributeAlternateAdvances(options, model);
}

void scenarioCustomTransportationTimestampedInteractionAlternateAdvances(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Custom transportation timestamped interaction alternate-advance testing requires an adapter-supplied rich FOM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Custom transportation timestamped interaction alternate-advance testing requires an adapter-supplied logical-time implementation");

  auto alternate = options;
  alternate.fom = alternate.modelFom;
  alternate.interactionClassName = alternate.customRegionalInteractionClassName;
  alternate.parameterName = alternate.customRegionalParameterName;
  alternate.requireCustomTransportation = true;

  // The established standard oracle covers the three alternate advance
  // services, grant ordering, query boundaries, and retraction terminalization.
  scenarioTimestampedInteractionMixedAdvances(alternate, model);

  Session publisher(
      alternate,
      model,
      "custom-transportation-timestamped-interaction-alternate-publisher");
  Session flushQueue(
      alternate,
      model,
      "custom-transportation-timestamped-interaction-alternate-flush-queue");
  Session timeAdvanceAvailable(
      alternate,
      model,
      "custom-transportation-timestamped-interaction-alternate-tara");
  Session nextMessageAvailable(
      alternate,
      model,
      "custom-transportation-timestamped-interaction-alternate-nmra");
  auto const federation = federationName(
      alternate,
      "custom-transportation-timestamped-interaction-alternate-advances");

  publisher.connect();
  flushQueue.connect();
  timeAdvanceAvailable.connect();
  nextMessageAvailable.connect();
  publisher.rtiAmbassador().createFederationExecution(
      federation,
      alternate.modelFom.wstring(),
      alternate.logicalTimeImplementationName);
  publisher.join(alternate.ownerFederateName, alternate.federateType, federation);
  flushQueue.join(
      alternate.memberFederateName + L"-custom-transportation-interaction-flush-queue",
      alternate.federateType,
      federation);
  timeAdvanceAvailable.join(
      alternate.memberFederateName + L"-custom-transportation-interaction-tara",
      alternate.federateType,
      federation);
  nextMessageAvailable.join(
      alternate.memberFederateName + L"-custom-transportation-interaction-nmra",
      alternate.federateType,
      federation);

  auto const publisherInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      alternate.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      alternate.parameterName);
  require(
      publisherInteraction.isValid() && publisherParameter.isValid() &&
          publisher.rtiAmbassador().getInteractionClassName(publisherInteraction) ==
              alternate.interactionClassName &&
          publisher.rtiAmbassador().getParameterName(
              publisherInteraction,
              publisherParameter) == alternate.parameterName,
      "custom transportation timestamped interaction alternate-advance publisher lookup did not round-trip");

  auto const publisherTransportation =
      publisher.rtiAmbassador().getTransportationTypeHandle(
          alternate.fomCustomTransportationName);
  require(
      publisherTransportation.isValid() &&
          publisher.rtiAmbassador().getTransportationTypeName(
              publisherTransportation) == alternate.fomCustomTransportationName,
      "custom transportation timestamped interaction alternate-advance publisher transport lookup did not round-trip");

  auto const members = std::vector<Session*>{
      &flushQueue,
      &timeAdvanceAvailable,
      &nextMessageAvailable,
  };
  std::vector<rti::InteractionClassHandle> memberInteractions;
  std::vector<rti::ParameterHandle> memberParameters;
  memberInteractions.reserve(members.size());
  memberParameters.reserve(members.size());
  for (auto* member : members) {
    auto const interaction = member->rtiAmbassador().getInteractionClassHandle(
        alternate.interactionClassName);
    auto const parameter = member->rtiAmbassador().getParameterHandle(
        interaction,
        alternate.parameterName);
    auto const transportation = member->rtiAmbassador().getTransportationTypeHandle(
        alternate.fomCustomTransportationName);
    require(
        interaction.isValid() && parameter.isValid() && transportation.isValid() &&
            member->rtiAmbassador().getInteractionClassName(interaction) ==
                alternate.interactionClassName &&
            member->rtiAmbassador().getParameterName(interaction, parameter) ==
                alternate.parameterName &&
            member->rtiAmbassador().getTransportationTypeName(transportation) ==
                alternate.fomCustomTransportationName,
        "custom transportation timestamped interaction alternate-advance member lookup did not round-trip");
    memberInteractions.push_back(interaction);
    memberParameters.push_back(parameter);
  }

  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::TIMESTAMP);
  for (auto const& interaction : memberInteractions) {
    auto const index = static_cast<std::size_t>(
        &interaction - memberInteractions.data());
    members.at(index)->rtiAmbassador().subscribeInteractionClass(interaction, true);
  }

  auto publisherTime = makeTimeContext(publisher);
  auto flushQueueTime = makeTimeContext(flushQueue);
  auto timeAdvanceAvailableTime = makeTimeContext(timeAdvanceAvailable);
  auto nextMessageAvailableTime = makeTimeContext(nextMessageAvailable);
  require(
      publisherTime.factory->getName() == flushQueueTime.factory->getName() &&
          publisherTime.factory->getName() == timeAdvanceAvailableTime.factory->getName() &&
          publisherTime.factory->getName() == nextMessageAvailableTime.factory->getName(),
      "custom transportation timestamped interaction alternate-advance members selected different logical-time factories");

  for (auto* member : members) {
    member->rtiAmbassador().enableTimeConstrained();
    waitFor(
        *member,
        [&] { return member->recorder().timeConstrainedEnabled().size() >= 1U; },
        alternate,
        "custom transportation timestamped interaction alternate-advance time-constrained callback");
  }
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      alternate,
      "custom transportation timestamped interaction alternate-advance time-regulation callback");

  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      4U);
  auto const requestBoundary = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      10U);
  std::vector<std::uint8_t> const parameterBytes{0x49U, 0x41U, 0x4CU, 0x54U};
  std::vector<std::uint8_t> const tagBytes{0x54U, 0x41U, 0x4CU};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherParameter,
      rti::VariableLengthData(parameterBytes.data(), parameterBytes.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      parameters,
      tag,
      *messageTime);
  require(
      retraction.isValid(),
      "custom transportation timestamped interaction alternate-advance send returned an invalid retraction handle");
  for (auto* member : members) {
    require(
        member->recorder().timedInteractions().empty(),
        "custom transportation timestamped interaction alternate-advance delivered before its request");
  }

  flushQueue.rtiAmbassador().flushQueueRequest(*requestBoundary);
  timeAdvanceAvailable.rtiAmbassador().timeAdvanceRequestAvailable(*messageTime);
  nextMessageAvailable.rtiAmbassador().nextMessageRequestAvailable(*requestBoundary);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  waitForSessions(
      {&publisher, &flushQueue, &timeAdvanceAvailable, &nextMessageAvailable},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            flushQueue.recorder().timedInteractions().size() >= 1U &&
            flushQueue.recorder().flushQueueGrants().size() >= 1U &&
            timeAdvanceAvailable.recorder().timedInteractions().size() >= 1U &&
            timeAdvanceAvailable.recorder().timeAdvanceGrants().size() >= 1U &&
            nextMessageAvailable.recorder().timedInteractions().size() >= 1U &&
            nextMessageAvailable.recorder().timeAdvanceGrants().size() >= 1U;
      },
      alternate,
      "custom transportation timestamped interaction alternate-advance delivery and grants");

  auto assertInteraction = [&](TimedInteractionRecord const& record,
                               rti::InteractionClassHandle const& expectedInteraction,
                               rti::ParameterHandle const& expectedParameter,
                               Session& inspector) {
    require(
        record.interaction == expectedInteraction && record.parameters.size() == 1U &&
            record.parameters.count(expectedParameter) == 1U &&
            copyBytes(record.parameters.at(expectedParameter)) == parameterBytes &&
            record.tag == tagBytes && record.time == encodeTime(*messageTime),
        "custom transportation timestamped interaction alternate-advance callback changed the payload");
    require(
        record.producer == publisher.federateHandle() && !record.timeText.empty() &&
            record.sentOrder == rti::TIMESTAMP &&
            record.receivedOrder == rti::TIMESTAMP,
        "custom transportation timestamped interaction alternate-advance callback changed time or order metadata");
    require(
        !record.regions.has_value() && record.transportation.isValid() &&
            inspector.rtiAmbassador().getTransportationTypeName(
                record.transportation) == alternate.fomCustomTransportationName,
        "custom transportation timestamped interaction alternate-advance callback changed transportation identity or regions");
    require(
        record.retractionPresent &&
            record.retraction == copyBytes(retraction.encode()),
        "custom transportation timestamped interaction alternate-advance callback returned the wrong retraction handle");
  };
  require(
      flushQueue.recorder().timedInteractions().size() == 1U &&
          timeAdvanceAvailable.recorder().timedInteractions().size() == 1U &&
          nextMessageAvailable.recorder().timedInteractions().size() == 1U,
      "custom transportation timestamped interaction alternate-advance delivered an unexpected callback count");
  assertInteraction(
      flushQueue.recorder().timedInteractions().front(),
      memberInteractions.at(0),
      memberParameters.at(0),
      flushQueue);
  assertInteraction(
      timeAdvanceAvailable.recorder().timedInteractions().front(),
      memberInteractions.at(1),
      memberParameters.at(1),
      timeAdvanceAvailable);
  assertInteraction(
      nextMessageAvailable.recorder().timedInteractions().front(),
      memberInteractions.at(2),
      memberParameters.at(2),
      nextMessageAvailable);
  requireException(
      [&] { publisher.rtiAmbassador().retract(retraction); },
      L"MessageCanNoLongerBeRetracted",
      "retracting a delivered custom transportation timestamped interaction alternate-advance message");

  for (auto* member : members) {
    member->rtiAmbassador().disableTimeConstrained();
  }
  publisher.rtiAmbassador().disableTimeRegulation();
  for (std::size_t index = 0U; index != members.size(); ++index) {
    members.at(index)->rtiAmbassador().unsubscribeInteractionClass(
        memberInteractions.at(index));
  }
  publisher.rtiAmbassador().unpublishInteractionClass(publisherInteraction);
  nextMessageAvailable.resign(rti::NO_ACTION);
  timeAdvanceAvailable.resign(rti::NO_ACTION);
  flushQueue.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  nextMessageAvailable.disconnect();
  timeAdvanceAvailable.disconnect();
  flushQueue.disconnect();
  publisher.disconnect();
}

void scenarioCustomTransportationTimestampedInteractionAlternateAdvancesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCustomTransportationTimestampedInteractionAlternateAdvances(options, model);
}

void scenarioCustomTransportationTimestampedDirectedDeliveryPortable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Custom transportation timestamped directed delivery requires an adapter-supplied rich FOM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Custom transportation timestamped directed delivery requires an adapter-supplied logical-time implementation");

  // Keep the established standard directed-delivery oracle behind the portable
  // adapter boundary. It uses only the official API; the adapter supplies the
  // provider, FOM, endpoint, logical-time, and callback configuration.
  scenarioCustomTransportationTimestampedDirectedDelivery(options, model);
}

void scenarioCustomTransportationTimestampedDirectedDeliveryPortableContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCustomTransportationTimestampedDirectedDeliveryPortable(options, model);
}

void scenarioCustomTransportationInteractionDeliveryPortable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Custom transportation interaction delivery requires an adapter-supplied rich FOM");
  scenarioCustomTransportationInteractionDelivery(options, model);
}

void scenarioCustomTransportationInteractionDeliveryPortableContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCustomTransportationInteractionDeliveryPortable(options, model);
}

void scenarioCustomTransportationTimestampedDeliveryPortable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Custom transportation timestamped interaction delivery requires an adapter-supplied rich FOM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Custom transportation timestamped interaction delivery requires an adapter-supplied logical-time implementation");
  scenarioCustomTransportationTimestampedDelivery(options, model);
}

void scenarioCustomTransportationTimestampedDeliveryPortableContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCustomTransportationTimestampedDeliveryPortable(options, model);
}

void scenarioCustomTransportationDirectedInteractionDelivery(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Custom transportation directed interaction testing requires an adapter-supplied rich FOM");

  auto directed = options;
  directed.fom = directed.modelFom;
  directed.objectClassName = directed.typedObjectClassName;
  directed.attributeName = directed.typedIntegerAttributeName;
  directed.interactionClassName = directed.typedInteractionClassName;
  directed.parameterName = directed.typedIntegerParameterName;
  directed.requireCustomTransportation = true;

  Session publisher(
      directed,
      model,
      "custom-transportation-directed-interaction-publisher");
  Session targeted(
      directed,
      model,
      "custom-transportation-directed-interaction-targeted");
  Session universal(
      directed,
      rti::HLA_IMMEDIATE,
      "custom-transportation-directed-interaction-universal");
  auto const federation = federationName(
      directed,
      "custom-transportation-directed-interaction-delivery");
  connectAndJoin(
      publisher,
      targeted,
      directed,
      federation,
      directed.modelFom);
  universal.connect();
  universal.join(
      directed.memberFederateName + L"-custom-transportation-directed-interaction-universal",
      directed.federateType,
      federation);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      directed.objectClassName);
  auto const targetedClass = targeted.rtiAmbassador().getObjectClassHandle(
      directed.objectClassName);
  auto const universalClass = universal.rtiAmbassador().getObjectClassHandle(
      directed.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      directed.attributeName);
  auto const targetedAttribute = targeted.rtiAmbassador().getAttributeHandle(
      targetedClass,
      directed.attributeName);
  auto const universalAttribute = universal.rtiAmbassador().getAttributeHandle(
      universalClass,
      directed.attributeName);
  auto const publisherInteraction =
      publisher.rtiAmbassador().getInteractionClassHandle(
          directed.interactionClassName);
  auto const targetedInteraction =
      targeted.rtiAmbassador().getInteractionClassHandle(
          directed.interactionClassName);
  auto const universalInteraction =
      universal.rtiAmbassador().getInteractionClassHandle(
          directed.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      directed.parameterName);
  auto const targetedParameter = targeted.rtiAmbassador().getParameterHandle(
      targetedInteraction,
      directed.parameterName);
  auto const universalParameter = universal.rtiAmbassador().getParameterHandle(
      universalInteraction,
      directed.parameterName);
  auto const publisherTransportation =
      publisher.rtiAmbassador().getTransportationTypeHandle(
          directed.fomCustomTransportationName);
  auto const targetedTransportation =
      targeted.rtiAmbassador().getTransportationTypeHandle(
          directed.fomCustomTransportationName);
  auto const universalTransportation =
      universal.rtiAmbassador().getTransportationTypeHandle(
          directed.fomCustomTransportationName);
  require(
      publisherClass.isValid() && targetedClass.isValid() &&
          universalClass.isValid() && publisherAttribute.isValid() &&
          targetedAttribute.isValid() && universalAttribute.isValid() &&
          publisherInteraction.isValid() && targetedInteraction.isValid() &&
          universalInteraction.isValid() && publisherParameter.isValid() &&
          targetedParameter.isValid() && universalParameter.isValid() &&
          publisherTransportation.isValid() && targetedTransportation.isValid() &&
          universalTransportation.isValid(),
      "custom transportation directed interaction lookup returned an invalid handle");
  require(
      publisher.rtiAmbassador().getObjectClassName(publisherClass) ==
              directed.objectClassName &&
          targeted.rtiAmbassador().getObjectClassName(targetedClass) ==
              directed.objectClassName &&
          universal.rtiAmbassador().getObjectClassName(universalClass) ==
              directed.objectClassName &&
          publisher.rtiAmbassador().getAttributeName(
              publisherClass,
              publisherAttribute) == directed.attributeName &&
          targeted.rtiAmbassador().getAttributeName(
              targetedClass,
              targetedAttribute) == directed.attributeName &&
          universal.rtiAmbassador().getAttributeName(
              universalClass,
              universalAttribute) == directed.attributeName &&
          publisher.rtiAmbassador().getInteractionClassName(
              publisherInteraction) == directed.interactionClassName &&
          targeted.rtiAmbassador().getInteractionClassName(
              targetedInteraction) == directed.interactionClassName &&
          universal.rtiAmbassador().getInteractionClassName(
              universalInteraction) == directed.interactionClassName &&
          publisher.rtiAmbassador().getParameterName(
              publisherInteraction,
              publisherParameter) == directed.parameterName &&
          targeted.rtiAmbassador().getParameterName(
              targetedInteraction,
              targetedParameter) == directed.parameterName &&
          universal.rtiAmbassador().getParameterName(
              universalInteraction,
              universalParameter) == directed.parameterName &&
          publisher.rtiAmbassador().getTransportationTypeName(
              publisherTransportation) == directed.fomCustomTransportationName &&
          targeted.rtiAmbassador().getTransportationTypeName(
              targetedTransportation) == directed.fomCustomTransportationName &&
          universal.rtiAmbassador().getTransportationTypeName(
              universalTransportation) == directed.fomCustomTransportationName,
      "custom transportation directed interaction lookup did not round-trip names");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const targetedAttributes{targetedAttribute};
  rti::AttributeHandleSet const universalAttributes{universalAttribute};
  publisher.rtiAmbassador().subscribeObjectClassAttributes(
      publisherClass,
      publisherAttributes,
      true,
      L"");
  targeted.rtiAmbassador().publishObjectClassAttributes(
      targetedClass,
      targetedAttributes);
  universal.rtiAmbassador().subscribeObjectClassAttributes(
      universalClass,
      universalAttributes,
      true,
      L"");
  targeted.rtiAmbassador().subscribeObjectClassDirectedInteractions(
      targetedClass,
      rti::InteractionClassHandleSet{targetedInteraction},
      false);
  universal.rtiAmbassador().subscribeObjectClassDirectedInteractions(
      universalClass,
      rti::InteractionClassHandleSet{universalInteraction},
      true);
  publisher.rtiAmbassador().publishObjectClassDirectedInteractions(
      publisherClass,
      rti::InteractionClassHandleSet{publisherInteraction});

  auto const target = targeted.rtiAmbassador().registerObjectInstance(
      targetedClass);
  require(
      target.isValid(),
      "custom transportation directed interaction registration returned an invalid target");
  auto const targetName = targeted.rtiAmbassador().getObjectInstanceName(target);
  require(
      !targetName.empty() &&
          targeted.rtiAmbassador().getObjectInstanceHandle(targetName) == target,
      "custom transportation directed interaction registration did not preserve object naming");
  waitForSessions(
      {&publisher, &universal},
      [&] {
        return publisher.recorder().hasDiscovery(target) &&
            universal.recorder().hasDiscovery(target);
      },
      directed,
      "custom transportation directed interaction target discovery");

  std::vector<std::uint8_t> const parameterBytes{
      0x43U,
      0x55U,
      0x53U,
      0x54U,
  };
  std::vector<std::uint8_t> const tagBytes{
      0x44U,
      0x49U,
      0x52U,
      0x2DU,
      0x43U,
  };
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherParameter,
      rti::VariableLengthData(parameterBytes.data(), parameterBytes.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  publisher.rtiAmbassador().sendDirectedInteraction(
      publisherInteraction,
      target,
      parameters,
      tag);
  waitFor(
      targeted,
      [&] { return targeted.recorder().directedInteractions().size() >= 1U; },
      directed,
      "custom transportation directed interaction targeted delivery");
  require(
      universal.recorder().directedInteractions().size() == 1U,
      "custom transportation directed interaction universal delivery returned the wrong count");
  require(
      publisher.recorder().directedInteractions().empty(),
      "custom transportation directed interaction looped back to its publisher");

  auto assertDelivery = [&](DirectedInteractionRecord const& record,
                            rti::InteractionClassHandle const& expectedInteraction,
                            rti::ParameterHandle const& expectedParameter,
                            Session& inspector,
                            std::string const& description) {
    require(
        record.interaction == expectedInteraction && record.object == target &&
            record.parameters.size() == 1U &&
            record.parameters.count(expectedParameter) == 1U &&
            copyBytes(record.parameters.at(expectedParameter)) == parameterBytes,
        description + " returned the wrong target or parameter value");
    require(
        record.tag == tagBytes && record.producer == publisher.federateHandle(),
        description + " changed the standard interaction metadata");
    require(
        record.transportation.isValid() &&
            inspector.rtiAmbassador().getTransportationTypeName(
                record.transportation) == directed.fomCustomTransportationName,
        description + " changed the custom transportation identity");
  };
  assertDelivery(
      targeted.recorder().directedInteractions().front(),
      targetedInteraction,
      targetedParameter,
      targeted,
      "custom transportation directed interaction targeted callback");
  assertDelivery(
      universal.recorder().directedInteractions().front(),
      universalInteraction,
      universalParameter,
      universal,
      "custom transportation directed interaction universal callback");

  targeted.rtiAmbassador().unsubscribeObjectClassDirectedInteractions(
      targetedClass,
      rti::InteractionClassHandleSet{targetedInteraction});
  universal.rtiAmbassador().unsubscribeObjectClassDirectedInteractions(
      universalClass,
      rti::InteractionClassHandleSet{universalInteraction});
  publisher.rtiAmbassador().unpublishObjectClassDirectedInteractions(
      publisherClass,
      rti::InteractionClassHandleSet{publisherInteraction});
  publisher.rtiAmbassador().unsubscribeObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  universal.rtiAmbassador().unsubscribeObjectClassAttributes(
      universalClass,
      universalAttributes);
  targeted.rtiAmbassador().unpublishObjectClassAttributes(
      targetedClass,
      targetedAttributes);
  universal.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  targeted.resign(rti::DELETE_OBJECTS);
  targeted.rtiAmbassador().destroyFederationExecution(federation);
  universal.disconnect();
  publisher.disconnect();
  targeted.disconnect();
}

void scenarioCustomTransportationDirectedInteractionDeliveryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCustomTransportationDirectedInteractionDelivery(options, model);
}

void scenarioCustomTransportationTimestampedDirectedInteractionAlternateAdvances(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Custom transportation timestamped directed interaction alternate-advance testing requires an adapter-supplied rich FOM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Custom transportation timestamped directed interaction alternate-advance testing requires an adapter-supplied logical-time implementation");

  auto alternate = options;
  alternate.fom = alternate.modelFom;
  alternate.objectClassName = alternate.typedObjectClassName;
  alternate.attributeName = alternate.typedIntegerAttributeName;
  alternate.interactionClassName = alternate.typedInteractionClassName;
  alternate.parameterName = alternate.typedIntegerParameterName;
  alternate.requireCustomTransportation = true;

  // The established standard oracle covers the three alternate advance
  // services, grant ordering, query boundaries, and retraction terminalization.
  scenarioTimestampedDirectedAlternateAdvances(alternate, model);

  Session publisher(
      alternate,
      model,
      "custom-transportation-timestamped-directed-interaction-alternate-publisher");
  Session flushQueue(
      alternate,
      model,
      "custom-transportation-timestamped-directed-interaction-alternate-flush-queue");
  Session timeAdvanceAvailable(
      alternate,
      model,
      "custom-transportation-timestamped-directed-interaction-alternate-tara");
  Session nextMessageAvailable(
      alternate,
      model,
      "custom-transportation-timestamped-directed-interaction-alternate-nmra");
  auto const federation = federationName(
      alternate,
      "custom-transportation-timestamped-directed-interaction-alternate-advances");

  publisher.connect();
  flushQueue.connect();
  timeAdvanceAvailable.connect();
  nextMessageAvailable.connect();
  publisher.rtiAmbassador().createFederationExecution(
      federation,
      alternate.modelFom.wstring(),
      alternate.logicalTimeImplementationName);
  publisher.join(alternate.ownerFederateName, alternate.federateType, federation);
  flushQueue.join(
      alternate.memberFederateName + L"-custom-transportation-directed-flush-queue",
      alternate.federateType,
      federation);
  timeAdvanceAvailable.join(
      alternate.memberFederateName + L"-custom-transportation-directed-tara",
      alternate.federateType,
      federation);
  nextMessageAvailable.join(
      alternate.memberFederateName + L"-custom-transportation-directed-nmra",
      alternate.federateType,
      federation);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      alternate.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      alternate.attributeName);
  auto const publisherInteraction =
      publisher.rtiAmbassador().getInteractionClassHandle(
          alternate.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      alternate.parameterName);
  auto const publisherTransportation =
      publisher.rtiAmbassador().getTransportationTypeHandle(
          alternate.fomCustomTransportationName);
  require(
      publisherClass.isValid() && publisherAttribute.isValid() &&
          publisherInteraction.isValid() && publisherParameter.isValid() &&
          publisherTransportation.isValid() &&
          publisher.rtiAmbassador().getObjectClassName(publisherClass) ==
              alternate.objectClassName &&
          publisher.rtiAmbassador().getAttributeName(
              publisherClass,
              publisherAttribute) == alternate.attributeName &&
          publisher.rtiAmbassador().getInteractionClassName(
              publisherInteraction) == alternate.interactionClassName &&
          publisher.rtiAmbassador().getParameterName(
              publisherInteraction,
              publisherParameter) == alternate.parameterName &&
          publisher.rtiAmbassador().getTransportationTypeName(
              publisherTransportation) == alternate.fomCustomTransportationName,
      "custom transportation timestamped directed interaction alternate-advance publisher lookup did not round-trip");

  auto const members = std::vector<Session*>{
      &flushQueue,
      &timeAdvanceAvailable,
      &nextMessageAvailable,
  };
  std::vector<rti::ObjectClassHandle> memberClasses;
  std::vector<rti::AttributeHandle> memberAttributes;
  std::vector<rti::InteractionClassHandle> memberInteractions;
  std::vector<rti::ParameterHandle> memberParameters;
  memberClasses.reserve(members.size());
  memberAttributes.reserve(members.size());
  memberInteractions.reserve(members.size());
  memberParameters.reserve(members.size());
  for (auto* member : members) {
    auto const objectClass = member->rtiAmbassador().getObjectClassHandle(
        alternate.objectClassName);
    auto const attribute = member->rtiAmbassador().getAttributeHandle(
        objectClass,
        alternate.attributeName);
    auto const interaction = member->rtiAmbassador().getInteractionClassHandle(
        alternate.interactionClassName);
    auto const parameter = member->rtiAmbassador().getParameterHandle(
        interaction,
        alternate.parameterName);
    auto const transportation = member->rtiAmbassador().getTransportationTypeHandle(
        alternate.fomCustomTransportationName);
    require(
        objectClass.isValid() && attribute.isValid() && interaction.isValid() &&
            parameter.isValid() && transportation.isValid() &&
            member->rtiAmbassador().getObjectClassName(objectClass) ==
                alternate.objectClassName &&
            member->rtiAmbassador().getAttributeName(objectClass, attribute) ==
                alternate.attributeName &&
            member->rtiAmbassador().getInteractionClassName(interaction) ==
                alternate.interactionClassName &&
            member->rtiAmbassador().getParameterName(interaction, parameter) ==
                alternate.parameterName &&
            member->rtiAmbassador().getTransportationTypeName(transportation) ==
                alternate.fomCustomTransportationName,
        "custom transportation timestamped directed interaction alternate-advance member lookup did not round-trip");
    memberClasses.push_back(objectClass);
    memberAttributes.push_back(attribute);
    memberInteractions.push_back(interaction);
    memberParameters.push_back(parameter);
  }

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::InteractionClassHandleSet const publisherDirected{publisherInteraction};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  publisher.rtiAmbassador().publishObjectClassDirectedInteractions(
      publisherClass,
      publisherDirected);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::TIMESTAMP);
  for (std::size_t index = 0U; index != members.size(); ++index) {
    members.at(index)->rtiAmbassador().subscribeObjectClassAttributes(
        memberClasses.at(index),
        rti::AttributeHandleSet{memberAttributes.at(index)},
        true,
        L"");
    members.at(index)->rtiAmbassador().subscribeObjectClassDirectedInteractions(
        memberClasses.at(index),
        rti::InteractionClassHandleSet{memberInteractions.at(index)},
        true);
  }

  auto const target = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      target.isValid(),
      "custom transportation timestamped directed interaction alternate-advance registration returned an invalid handle");
  auto const targetName = publisher.rtiAmbassador().getObjectInstanceName(target);
  require(
      !targetName.empty() &&
          publisher.rtiAmbassador().getObjectInstanceHandle(targetName) == target,
      "custom transportation timestamped directed interaction alternate-advance registration did not round-trip its name");
  waitForSessions(
      {&publisher, &flushQueue, &timeAdvanceAvailable, &nextMessageAvailable},
      [&] {
        return flushQueue.recorder().hasDiscovery(target) &&
            timeAdvanceAvailable.recorder().hasDiscovery(target) &&
            nextMessageAvailable.recorder().hasDiscovery(target);
      },
      alternate,
      "custom transportation timestamped directed interaction alternate-advance target discovery");

  auto publisherTime = makeTimeContext(publisher);
  auto flushQueueTime = makeTimeContext(flushQueue);
  auto timeAdvanceAvailableTime = makeTimeContext(timeAdvanceAvailable);
  auto nextMessageAvailableTime = makeTimeContext(nextMessageAvailable);
  require(
      publisherTime.factory->getName() == flushQueueTime.factory->getName() &&
          publisherTime.factory->getName() == timeAdvanceAvailableTime.factory->getName() &&
          publisherTime.factory->getName() == nextMessageAvailableTime.factory->getName(),
      "custom transportation timestamped directed interaction alternate-advance members selected different logical-time factories");

  for (auto* member : members) {
    member->rtiAmbassador().enableTimeConstrained();
    waitFor(
        *member,
        [&] { return member->recorder().timeConstrainedEnabled().size() >= 1U; },
        alternate,
        "custom transportation timestamped directed interaction alternate-advance time-constrained callback");
  }
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      alternate,
      "custom transportation timestamped directed interaction alternate-advance time-regulation callback");

  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      4U);
  auto const requestBoundary = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      10U);
  auto const parameterBytes = copyBytes(rti::HLAinteger32BE{42}.encode());
  std::vector<std::uint8_t> const tagBytes{
      0x44U,
      0x49U,
      0x52U,
      0x2DU,
      0x41U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherParameter,
      rti::VariableLengthData(parameterBytes.data(), parameterBytes.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().sendDirectedInteraction(
      publisherInteraction,
      target,
      parameters,
      tag,
      *messageTime);
  require(
      retraction.isValid(),
      "custom transportation timestamped directed interaction alternate-advance send returned an invalid retraction handle");
  for (auto* member : members) {
    require(
        member->recorder().timedDirectedInteractions().empty(),
        "custom transportation timestamped directed interaction alternate-advance delivered before its request");
    member->recorder().clearCallbackOrder();
  }
  publisher.recorder().clearCallbackOrder();

  flushQueue.rtiAmbassador().flushQueueRequest(*requestBoundary);
  timeAdvanceAvailable.rtiAmbassador().timeAdvanceRequestAvailable(*messageTime);
  nextMessageAvailable.rtiAmbassador().nextMessageRequestAvailable(*requestBoundary);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  waitForSessions(
      {&publisher, &flushQueue, &timeAdvanceAvailable, &nextMessageAvailable},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            flushQueue.recorder().timedDirectedInteractions().size() >= 1U &&
            flushQueue.recorder().flushQueueGrants().size() >= 1U &&
            timeAdvanceAvailable.recorder().timedDirectedInteractions().size() >= 1U &&
            timeAdvanceAvailable.recorder().timeAdvanceGrants().size() >= 1U &&
            nextMessageAvailable.recorder().timedDirectedInteractions().size() >= 1U &&
            nextMessageAvailable.recorder().timeAdvanceGrants().size() >= 1U;
      },
      alternate,
      "custom transportation timestamped directed interaction alternate-advance delivery and grants");

  require(
      flushQueue.recorder().timedDirectedInteractions().size() == 1U &&
          flushQueue.recorder().flushQueueGrants().size() == 1U &&
          flushQueue.recorder().timeAdvanceGrants().empty(),
      "Flush Queue Request did not produce exactly one custom directed interaction and flush grant");
  require(
      timeAdvanceAvailable.recorder().timedDirectedInteractions().size() == 1U &&
          timeAdvanceAvailable.recorder().timeAdvanceGrants().size() == 1U &&
          timeAdvanceAvailable.recorder().flushQueueGrants().empty(),
      "Time Advance Request Available did not produce exactly one custom directed interaction and grant");
  require(
      nextMessageAvailable.recorder().timedDirectedInteractions().size() == 1U &&
          nextMessageAvailable.recorder().timeAdvanceGrants().size() == 1U &&
          nextMessageAvailable.recorder().flushQueueGrants().empty(),
      "Next Message Request Available did not produce exactly one custom directed interaction and grant");
  require(
      flushQueue.recorder().callbackOrder() ==
          std::vector<std::string>{"directed", "flush-grant"},
      "Flush Queue Request delivered its custom directed interaction after the flush grant");
  require(
      timeAdvanceAvailable.recorder().callbackOrder() ==
          std::vector<std::string>{"directed", "grant"},
      "Time Advance Request Available delivered its custom directed interaction after the grant");
  require(
      nextMessageAvailable.recorder().callbackOrder() ==
          std::vector<std::string>{"directed", "grant"},
      "Next Message Request Available delivered its custom directed interaction after the grant");

  auto assertDirected = [&](TimedDirectedInteractionRecord const& record,
                            rti::InteractionClassHandle const& expectedInteraction,
                            rti::ParameterHandle const& expectedParameter,
                            Session& inspector) {
    require(
        record.interaction == expectedInteraction && record.object == target &&
            record.parameters.size() == 1U &&
            record.parameters.count(expectedParameter) == 1U &&
            copyBytes(record.parameters.at(expectedParameter)) == parameterBytes,
        "custom transportation timestamped directed interaction alternate-advance callback changed the target or payload");
    require(
        record.tag == tagBytes && record.time == encodeTime(*messageTime) &&
            !record.timeText.empty() &&
            record.producer == publisher.federateHandle(),
        "custom transportation timestamped directed interaction alternate-advance callback changed standard metadata");
    require(
        record.sentOrder == rti::TIMESTAMP &&
            record.receivedOrder == rti::TIMESTAMP && record.transportation.isValid() &&
            inspector.rtiAmbassador().getTransportationTypeName(record.transportation) ==
                alternate.fomCustomTransportationName,
        "custom transportation timestamped directed interaction alternate-advance callback changed order or transport identity");
    require(
        record.retractionPresent &&
            record.retraction == copyBytes(retraction.encode()),
        "custom transportation timestamped directed interaction alternate-advance callback returned the wrong retraction handle");
  };
  assertDirected(
      flushQueue.recorder().timedDirectedInteractions().front(),
      memberInteractions.at(0),
      memberParameters.at(0),
      flushQueue);
  assertDirected(
      timeAdvanceAvailable.recorder().timedDirectedInteractions().front(),
      memberInteractions.at(1),
      memberParameters.at(1),
      timeAdvanceAvailable);
  assertDirected(
      nextMessageAvailable.recorder().timedDirectedInteractions().front(),
      memberInteractions.at(2),
      memberParameters.at(2),
      nextMessageAvailable);
  requireException(
      [&] { publisher.rtiAmbassador().retract(retraction); },
      L"MessageCanNoLongerBeRetracted",
      "retracting a delivered custom transportation timestamped directed interaction alternate-advance message");

  for (auto* member : members) {
    member->rtiAmbassador().disableTimeConstrained();
  }
  publisher.rtiAmbassador().disableTimeRegulation();
  for (std::size_t index = 0U; index != members.size(); ++index) {
    members.at(index)->rtiAmbassador().unsubscribeObjectClassDirectedInteractions(
        memberClasses.at(index),
        rti::InteractionClassHandleSet{memberInteractions.at(index)});
    members.at(index)->rtiAmbassador().unsubscribeObjectClassAttributes(
        memberClasses.at(index),
        rti::AttributeHandleSet{memberAttributes.at(index)});
  }
  publisher.rtiAmbassador().unpublishObjectClassDirectedInteractions(
      publisherClass,
      publisherDirected);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  nextMessageAvailable.resign(rti::NO_ACTION);
  timeAdvanceAvailable.resign(rti::NO_ACTION);
  flushQueue.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  nextMessageAvailable.disconnect();
  timeAdvanceAvailable.disconnect();
  flushQueue.disconnect();
  publisher.disconnect();
}

void scenarioCustomTransportationTimestampedDirectedInteractionAlternateAdvancesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCustomTransportationTimestampedDirectedInteractionAlternateAdvances(options, model);
}

void scenarioCustomTransportationTimestampedRegionalInteractionDelivery(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Custom transportation timestamped regional interaction testing requires an adapter-supplied rich FOM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Custom transportation timestamped regional interaction testing requires an adapter-supplied logical-time implementation");
  require(
      options.ddmDimensionNames.size() >= 2U,
      "Custom transportation timestamped regional interaction testing requires two adapter-supplied dimensions");

  auto regional = options;
  regional.ddmFom = regional.modelFom;
  regional.objectClassName = regional.customRegionalObjectClassName;
  regional.attributeName = regional.customRegionalAttributeName;
  regional.interactionClassName = regional.customRegionalInteractionClassName;
  regional.parameterName = regional.customRegionalParameterName;
  regional.requireCustomTransportation = true;

  // Keep the complete standard timestamped regional interaction oracle as the
  // first half of this extension. The focused probe below then verifies that
  // the same send-with-regions operation retains the adapter-declared custom
  // transportation identity.
  scenarioTimestampedRegionalInteraction(regional, model);

  Session producer(regional, model, "custom-transportation-timestamped-regional-producer");
  Session receiver(regional, model, "custom-transportation-timestamped-regional-receiver");
  auto const federation = federationName(
      regional,
      "custom-transportation-timestamped-regional-interaction-delivery");
  connectAndJoin(producer, receiver, regional, federation, regional.ddmFom);

  auto const producerHandles = ddmHandles(producer, regional);
  auto const receiverHandles = ddmHandles(receiver, regional);
  verifyDdmClassDimensions(
      producer,
      regional,
      producerHandles,
      "custom transportation timestamped regional interaction");
  auto const producerTransportation = producer.rtiAmbassador().getTransportationTypeHandle(
      regional.fomCustomTransportationName);
  auto const receiverTransportation = receiver.rtiAmbassador().getTransportationTypeHandle(
      regional.fomCustomTransportationName);
  require(
      producerTransportation.isValid() && receiverTransportation.isValid(),
      "custom transportation timestamped regional interaction lookup returned an invalid handle");
  require(
      producer.rtiAmbassador().getTransportationTypeName(producerTransportation) ==
              regional.fomCustomTransportationName &&
          receiver.rtiAmbassador().getTransportationTypeName(receiverTransportation) ==
              regional.fomCustomTransportationName,
      "custom transportation timestamped regional interaction name lookup did not round-trip");

  producer.rtiAmbassador().publishInteractionClass(producerHandles.interactionClass);
  producer.rtiAmbassador().changeInteractionOrderType(
      producerHandles.interactionClass,
      rti::TIMESTAMP);
  auto const sourceRegion = createDdmRegion(
      producer,
      producerHandles,
      1UL,
      5UL,
      1UL,
      5UL);
  auto const receiverRegion = createDdmRegion(
      receiver,
      receiverHandles,
      2UL,
      6UL,
      2UL,
      6UL);
  auto const sourceRegionSet = rti::RegionHandleSet{sourceRegion};
  auto const receiverRegionSet = rti::RegionHandleSet{receiverRegion};
  receiver.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  require(
      receiver.rtiAmbassador().getConveyRegionDesignatorSetsSwitch(),
      "custom transportation timestamped regional interaction did not enable region designator callbacks");
  receiver.rtiAmbassador().subscribeInteractionClassWithRegions(
      receiverHandles.interactionClass,
      receiverRegionSet,
      true);

  auto producerTime = makeTimeContext(producer);
  auto receiverTime = makeTimeContext(receiver);
  enableTimestampedRoles(
      producer,
      receiver,
      producerTime,
      receiverTime,
      regional,
      "custom transportation timestamped regional interaction");
  auto const messageTime = timeAfter(
      *producerTime.factory,
      *producerTime.initial,
      *producerTime.epsilon,
      3U);
  auto const receiverTarget = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      3U);
  std::vector<std::uint8_t> const parameterBytes{0xC5U, 0x52U, 0x49U};
  std::vector<std::uint8_t> const tagBytes{0x54U, 0x52U, 0x49U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      producerHandles.parameter,
      rti::VariableLengthData(parameterBytes.data(), parameterBytes.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  auto const retraction = producer.rtiAmbassador().sendInteractionWithRegions(
      producerHandles.interactionClass,
      parameters,
      sourceRegionSet,
      tag,
      *messageTime);
  require(
      retraction.isValid(),
      "custom transportation timestamped regional interaction returned an invalid retraction handle");
  require(
      receiver.recorder().timedInteractions().empty(),
      "custom transportation timestamped regional interaction delivered before its grant");

  receiver.rtiAmbassador().timeAdvanceRequest(*receiverTarget);
  producer.rtiAmbassador().timeAdvanceRequest(*messageTime);
  waitForSessions(
      {&producer, &receiver},
      [&] {
        return producer.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timedInteractions().size() >= 1U;
      },
      regional,
      "custom transportation timestamped regional interaction delivery and grants");

  auto const deliveries = receiver.recorder().timedInteractions();
  require(
      deliveries.size() == 1U,
      "custom transportation timestamped regional interaction produced duplicate callbacks");
  auto const& delivery = deliveries.front();
  require(
      delivery.interaction == receiverHandles.interactionClass &&
          delivery.parameters.size() == 1U &&
          delivery.parameters.count(receiverHandles.parameter) == 1U &&
          copyBytes(delivery.parameters.at(receiverHandles.parameter)) == parameterBytes,
      "custom transportation timestamped regional interaction changed the payload");
  require(
      delivery.tag == tagBytes &&
          delivery.producer == producer.federateHandle() &&
          delivery.time == encodeTime(*messageTime) &&
          !delivery.timeText.empty() &&
          delivery.sentOrder == rti::TIMESTAMP &&
          delivery.receivedOrder == rti::TIMESTAMP &&
          delivery.retractionPresent &&
          delivery.retraction == copyBytes(retraction.encode()),
      "custom transportation timestamped regional interaction changed standard callback metadata");
  require(
      delivery.transportation.isValid() &&
          receiver.rtiAmbassador().getTransportationTypeName(delivery.transportation) ==
              regional.fomCustomTransportationName,
      "custom transportation timestamped regional interaction changed transportation identity");
  require(
      delivery.regions.has_value() &&
          delivery.regions->size() == 1U &&
          delivery.regions->count(sourceRegion) == 1U,
      "custom transportation timestamped regional interaction omitted the source region designator set");

  producer.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().disableTimeConstrained();
  receiver.rtiAmbassador().unsubscribeInteractionClassWithRegions(
      receiverHandles.interactionClass,
      receiverRegionSet);
  producer.rtiAmbassador().unpublishInteractionClass(producerHandles.interactionClass);
  producer.rtiAmbassador().deleteRegion(sourceRegion);
  receiver.rtiAmbassador().deleteRegion(receiverRegion);
  receiver.resign(rti::NO_ACTION);
  producer.resign(rti::NO_ACTION);
  producer.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  producer.disconnect();
}

void scenarioCustomTransportationTimestampedRegionalInteractionDeliveryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCustomTransportationTimestampedRegionalInteractionDelivery(options, model);
}

void scenarioDelaySubscriptionEvaluationInteractionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioDelaySubscriptionEvaluationInteraction(options, model);
}

void scenarioDelaySubscriptionEvaluationDirectedInteractionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioDelaySubscriptionEvaluationDirectedInteraction(options, model);
}

void scenarioDelaySubscriptionEvaluationAttributeUpdateContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioDelaySubscriptionEvaluationAttributeUpdate(options, model);
}

void scenarioDelaySubscriptionEvaluationTimestampedInteractionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioDelaySubscriptionEvaluationTimestampedInteraction(options, model);
}

void scenarioDelaySubscriptionEvaluationTimestampedDirectedInteractionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioDelaySubscriptionEvaluationTimestampedDirectedInteraction(options, model);
}

void scenarioDelaySubscriptionEvaluationTimestampedAttributeUpdateContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioDelaySubscriptionEvaluationTimestampedAttributeUpdate(options, model);
}

void scenarioFederationSaveRestoreInterlocksContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederationSaveRestoreInterlocks(options, model);
}

void scenarioTimedFederationSaveRestoreContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimedFederationSaveRestore(options, model);
}

void scenarioTimedRegionalInteractionSaveRestoreContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimedRegionalInteractionSaveRestore(options, model);
}

void scenarioTimedDefaultRegionInteractionSaveRestoreContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimedDefaultRegionInteractionSaveRestore(options, model);
}

void scenarioTimedDefaultRegionAttributeSaveRestoreContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimedDefaultRegionAttributeSaveRestore(options, model);
}

void scenarioRegionalMultiAttributeUpdateContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioRegionalMultiAttributeUpdate(options, model);
}

void scenarioRegionalThreeDimensionalOverlapContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioRegionalThreeDimensionalOverlap(options, model);
}

void scenarioAllowRelaxedDdmContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAllowRelaxedDdm(options, model);
}

void scenarioRegionalBoundariesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioRegionalBoundaries(options, model);
}

void scenarioOwnershipTransferRegionalUpdateContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioOwnershipTransferRegionalUpdate(options, model);
}

void scenarioMomTransportationTypeChangeRequestContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioMomTransportationTypeChangeRequest(options, model);
}

using PortableScenario = void (*)(Options const&, rti::CallbackModel);

int runPortableScenarioPair(
    int argc,
    char** argv,
    char const* scenarioId,
    char const* contractId,
    PortableScenario scenario,
    PortableScenario contract) {
  auto const options = parseOptions(argc, argv);
  std::vector<ScenarioResult> results;
  for (auto const& callback : callbackModels(options)) {
    for (auto const& selected : options.scenarios) {
      if (selected != scenarioId && selected != contractId) {
        continue;
      }
      auto const started = Clock::now();
      ScenarioResult result{selected, callback.first, "passed", "", 0};
      try {
        if (selected == scenarioId) {
          scenario(options, callback.second);
        } else {
          contract(options, callback.second);
        }
      } catch (rti::Exception const& error) {
        result.status = "failed";
        result.message = toNarrow(error.name()) + ": " + toNarrow(error.what());
      } catch (std::exception const& error) {
        result.status = "failed";
        result.message = error.what();
      } catch (...) {
        result.status = "failed";
        result.message = "unknown non-standard exception";
      }
      result.durationMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
          Clock::now() - started).count();
      results.push_back(result);
      std::cout << result.status << " " << result.id << " ["
                << result.callbackModel << "]";
      if (!result.message.empty()) {
        std::cout << ": " << result.message;
      }
      std::cout << '\n';
    }
  }
  if (!options.results.empty()) {
    writeResults(options.results, options, results);
  }
  if (!options.junit.empty()) {
    writeJUnit(options.junit, results);
  }
  auto const failures = std::count_if(
      results.begin(),
      results.end(),
      [](auto const& result) { return result.status == "failed"; });
  auto const skipped = std::count_if(
      results.begin(),
      results.end(),
      [](auto const& result) { return result.status == "skipped"; });
  std::cout << "summary passed=" << results.size() - failures - skipped
            << " skipped=" << skipped << " failed=" << failures << '\n';
  return failures == 0 ? 0 : 1;
}

int runMomTransportationTypeChangeRequestScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      momTransportationTypeChangeRequestScenario,
      momTransportationTypeChangeRequestContractId,
      scenarioMomTransportationTypeChangeRequest,
      scenarioMomTransportationTypeChangeRequestContract);
}

int runJoinedFederateMomRegisteredObjectCountScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      joinedFederateMomRegisteredObjectCountScenario,
      joinedFederateMomRegisteredObjectCountContractId,
      scenarioJoinedFederateMomRegisteredObjectCount,
      scenarioJoinedFederateMomRegisteredObjectCountContract);
}

int runJoinedFederateMomDeletableObjectReportScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      joinedFederateMomDeletableObjectReportScenario,
      joinedFederateMomDeletableObjectReportContractId,
      scenarioJoinedFederateMomDeletableObjectReport,
      scenarioJoinedFederateMomDeletableObjectReportContract);
}

int runJoinedFederateMomDeletableObjectCountScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      joinedFederateMomDeletableObjectCountScenario,
      joinedFederateMomDeletableObjectCountContractId,
      scenarioJoinedFederateMomDeletableObjectCount,
      scenarioJoinedFederateMomDeletableObjectCountContract);
}

int runReceiveOrderAttributeUpdateScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      receiveOrderAttributeUpdateScenario,
      receiveOrderAttributeUpdateContractId,
      scenarioReceiveOrderAttributeUpdate,
      scenarioReceiveOrderAttributeUpdateContract);
}

int runReceiveOrderInteractionScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      receiveOrderInteractionScenario,
      receiveOrderInteractionContractId,
      scenarioReceiveOrderInteraction,
      scenarioReceiveOrderInteractionContract);
}

int runReceiveOrderObjectRemovalScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      receiveOrderObjectRemovalScenario,
      receiveOrderObjectRemovalContractId,
      scenarioReceiveOrderObjectRemoval,
      scenarioReceiveOrderObjectRemovalContract);
}

int runFederationRestoreAbortScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federationRestoreAbortScenario,
      federationRestoreAbortContractId,
      scenarioFederationRestoreAbort,
      scenarioFederationRestoreAbortContract);
}

int runFederationRestoreOwnershipAssumptionScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federationRestoreOwnershipAssumptionScenario,
      federationRestoreOwnershipAssumptionContractId,
      scenarioFederationRestoreOwnershipAssumption,
      scenarioFederationRestoreOwnershipAssumptionContract);
}

int runOwnershipAcquisitionIfAvailableScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      ownershipAcquisitionIfAvailableScenario,
      ownershipAcquisitionIfAvailableContractId,
      scenarioOwnershipAcquisitionIfAvailable,
      scenarioOwnershipAcquisitionIfAvailableContract);
}

int runAutoProvideDisabledDiscoveryOnlyScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      autoProvideDisabledDiscoveryOnlyScenario,
      autoProvideDisabledDiscoveryOnlyContractId,
      scenarioAutoProvideDisabledDiscoveryOnly,
      scenarioAutoProvideDisabledDiscoveryOnlyContract);
}

int runAutoProvideDisabledExplicitRequestScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      autoProvideDisabledExplicitRequestScenario,
      autoProvideDisabledExplicitRequestContractId,
      scenarioAutoProvideDisabledExplicitRequest,
      scenarioAutoProvideDisabledExplicitRequestContract);
}

int runObjectRegistrationServiceBoundariesScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      objectRegistrationServiceBoundariesScenario,
      objectRegistrationServiceBoundariesContractId,
      scenarioObjectRegistrationServiceBoundaries,
      scenarioObjectRegistrationServiceBoundariesContract);
}

int runObjectDeletionServiceBoundariesScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      objectDeletionServiceBoundariesScenario,
      objectDeletionServiceBoundariesContractId,
      scenarioObjectDeletionServiceBoundaries,
      scenarioObjectDeletionServiceBoundariesContract);
}

int runAttributeUpdateServiceBoundariesScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      attributeUpdateServiceBoundariesScenario,
      attributeUpdateServiceBoundariesContractId,
      scenarioAttributeUpdateServiceBoundaries,
      scenarioAttributeUpdateServiceBoundariesContract);
}

int runInteractionServiceBoundariesScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      interactionServiceBoundariesScenario,
      interactionServiceBoundariesContractId,
      scenarioInteractionServiceBoundaries,
      scenarioInteractionServiceBoundariesContract);
}

int runAttributeValueRequestServiceBoundariesScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      attributeValueRequestServiceBoundariesScenario,
      attributeValueRequestServiceBoundariesContractId,
      scenarioAttributeValueRequestServiceBoundaries,
      scenarioAttributeValueRequestServiceBoundariesContract);
}

int runConnectionServiceBoundariesScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      connectionServiceBoundariesScenario,
      connectionServiceBoundariesContractId,
      scenarioConnectionServiceBoundaries,
      scenarioConnectionServiceBoundariesContract);
}

PortableScenario delaySubscriptionEvaluationScenario(std::string const& id) {
  if (id == delaySubscriptionEvaluationInteractionScenario) {
    return scenarioDelaySubscriptionEvaluationInteraction;
  }
  if (id == delaySubscriptionEvaluationInteractionContractId) {
    return scenarioDelaySubscriptionEvaluationInteractionContract;
  }
  if (id == delaySubscriptionEvaluationDirectedInteractionScenario) {
    return scenarioDelaySubscriptionEvaluationDirectedInteraction;
  }
  if (id == delaySubscriptionEvaluationDirectedInteractionContractId) {
    return scenarioDelaySubscriptionEvaluationDirectedInteractionContract;
  }
  if (id == delaySubscriptionEvaluationAttributeUpdateScenario) {
    return scenarioDelaySubscriptionEvaluationAttributeUpdate;
  }
  if (id == delaySubscriptionEvaluationAttributeUpdateContractId) {
    return scenarioDelaySubscriptionEvaluationAttributeUpdateContract;
  }
  if (id == delaySubscriptionEvaluationTimestampedInteractionScenario) {
    return scenarioDelaySubscriptionEvaluationTimestampedInteraction;
  }
  if (id == delaySubscriptionEvaluationTimestampedInteractionContractId) {
    return scenarioDelaySubscriptionEvaluationTimestampedInteractionContract;
  }
  if (id == delaySubscriptionEvaluationTimestampedDirectedInteractionScenario) {
    return scenarioDelaySubscriptionEvaluationTimestampedDirectedInteraction;
  }
  if (id == delaySubscriptionEvaluationTimestampedDirectedInteractionContractId) {
    return scenarioDelaySubscriptionEvaluationTimestampedDirectedInteractionContract;
  }
  if (id == delaySubscriptionEvaluationTimestampedAttributeUpdateScenario) {
    return scenarioDelaySubscriptionEvaluationTimestampedAttributeUpdate;
  }
  if (id == delaySubscriptionEvaluationTimestampedAttributeUpdateContractId) {
    return scenarioDelaySubscriptionEvaluationTimestampedAttributeUpdateContract;
  }
  return nullptr;
}

bool isDelaySubscriptionEvaluationTimestamped(std::string const& id) {
  return id == delaySubscriptionEvaluationTimestampedInteractionScenario ||
      id == delaySubscriptionEvaluationTimestampedInteractionContractId ||
      id == delaySubscriptionEvaluationTimestampedDirectedInteractionScenario ||
      id == delaySubscriptionEvaluationTimestampedDirectedInteractionContractId ||
      id == delaySubscriptionEvaluationTimestampedAttributeUpdateScenario ||
      id == delaySubscriptionEvaluationTimestampedAttributeUpdateContractId;
}

int runDelaySubscriptionEvaluationScenarios(int argc, char** argv) {
  auto const options = parseOptions(argc, argv);
  std::vector<ScenarioResult> results;
  for (auto const& callback : callbackModels(options)) {
    for (auto const& selected : options.scenarios) {
      auto const scenario = delaySubscriptionEvaluationScenario(selected);
      if (scenario == nullptr) {
        continue;
      }
      auto const started = Clock::now();
      ScenarioResult result{selected, callback.first, "passed", "", 0};
      if (options.switchesFom.empty()) {
        result.status = "skipped";
        result.message = "requires an adapter-supplied switch-declaration FOM";
      } else if (isDelaySubscriptionEvaluationTimestamped(selected) &&
                 options.logicalTimeImplementationName.empty()) {
        result.status = "skipped";
        result.message = "requires an adapter-supplied logical-time implementation";
      } else {
        try {
          scenario(options, callback.second);
        } catch (rti::Exception const& error) {
          result.status = "failed";
          result.message = toNarrow(error.name()) + ": " + toNarrow(error.what());
        } catch (std::exception const& error) {
          result.status = "failed";
          result.message = error.what();
        } catch (...) {
          result.status = "failed";
          result.message = "unknown non-standard exception";
        }
      }
      result.durationMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
          Clock::now() - started).count();
      results.push_back(result);
      std::cout << result.status << " " << result.id << " ["
                << result.callbackModel << "]";
      if (!result.message.empty()) {
        std::cout << ": " << result.message;
      }
      std::cout << '\n';
    }
  }
  if (!options.results.empty()) {
    writeResults(options.results, options, results);
  }
  if (!options.junit.empty()) {
    writeJUnit(options.junit, results);
  }
  auto const failures = std::count_if(
      results.begin(),
      results.end(),
      [](auto const& result) { return result.status == "failed"; });
  auto const skipped = std::count_if(
      results.begin(),
      results.end(),
      [](auto const& result) { return result.status == "skipped"; });
  std::cout << "summary passed=" << results.size() - failures - skipped
            << " skipped=" << skipped << " failed=" << failures << '\n';
  return failures == 0 ? 0 : 1;
}

int runFederationSaveRestoreInterlocksScenarios(int argc, char** argv) {
  auto const options = parseOptions(argc, argv);
  std::vector<ScenarioResult> results;
  for (auto const& callback : callbackModels(options)) {
    for (auto const& selected : options.scenarios) {
      if (selected != federationSaveRestoreInterlocksScenario &&
          selected != federationSaveRestoreInterlocksContractId) {
        continue;
      }
      auto const started = Clock::now();
      ScenarioResult result{selected, callback.first, "passed", "", 0};
      if (options.ddmFom.empty()) {
        result.status = "skipped";
        result.message = "requires an adapter-supplied dimensional FOM";
      } else if (options.logicalTimeImplementationName.empty()) {
        result.status = "skipped";
        result.message = "requires an adapter-supplied logical-time implementation";
      } else {
        try {
          if (selected == federationSaveRestoreInterlocksScenario) {
            scenarioFederationSaveRestoreInterlocks(options, callback.second);
          } else {
            scenarioFederationSaveRestoreInterlocksContract(
                options,
                callback.second);
          }
        } catch (rti::Exception const& error) {
          result.status = "failed";
          result.message = toNarrow(error.name()) + ": " + toNarrow(error.what());
        } catch (std::exception const& error) {
          result.status = "failed";
          result.message = error.what();
        } catch (...) {
          result.status = "failed";
          result.message = "unknown non-standard exception";
        }
      }
      result.durationMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
          Clock::now() - started).count();
      results.push_back(result);
      std::cout << result.status << " " << result.id << " ["
                << result.callbackModel << "]";
      if (!result.message.empty()) {
        std::cout << ": " << result.message;
      }
      std::cout << '\n';
    }
  }
  if (!options.results.empty()) {
    writeResults(options.results, options, results);
  }
  if (!options.junit.empty()) {
    writeJUnit(options.junit, results);
  }
  auto const failures = std::count_if(
      results.begin(),
      results.end(),
      [](auto const& result) { return result.status == "failed"; });
  auto const skipped = std::count_if(
      results.begin(),
      results.end(),
      [](auto const& result) { return result.status == "skipped"; });
  std::cout << "summary passed=" << results.size() - failures - skipped
            << " skipped=" << skipped << " failed=" << failures << '\n';
  return failures == 0 ? 0 : 1;
}

PortableScenario timedSaveRestoreScenario(std::string const& id) {
  if (id == timedFederationSaveRestoreScenario) {
    return scenarioTimedFederationSaveRestore;
  }
  if (id == timedFederationSaveRestoreContractId) {
    return scenarioTimedFederationSaveRestoreContract;
  }
  if (id == timedRegionalInteractionSaveRestoreScenario) {
    return scenarioTimedRegionalInteractionSaveRestore;
  }
  if (id == timedRegionalInteractionSaveRestoreContractId) {
    return scenarioTimedRegionalInteractionSaveRestoreContract;
  }
  if (id == timedDefaultRegionInteractionSaveRestoreScenario) {
    return scenarioTimedDefaultRegionInteractionSaveRestore;
  }
  if (id == timedDefaultRegionInteractionSaveRestoreContractId) {
    return scenarioTimedDefaultRegionInteractionSaveRestoreContract;
  }
  if (id == timedDefaultRegionAttributeSaveRestoreScenario) {
    return scenarioTimedDefaultRegionAttributeSaveRestore;
  }
  if (id == timedDefaultRegionAttributeSaveRestoreContractId) {
    return scenarioTimedDefaultRegionAttributeSaveRestoreContract;
  }
  return nullptr;
}

bool timedSaveRestoreRequiresDdm(std::string const& id) {
  return id == timedRegionalInteractionSaveRestoreScenario ||
      id == timedRegionalInteractionSaveRestoreContractId ||
      id == timedDefaultRegionInteractionSaveRestoreScenario ||
      id == timedDefaultRegionInteractionSaveRestoreContractId ||
      id == timedDefaultRegionAttributeSaveRestoreScenario ||
      id == timedDefaultRegionAttributeSaveRestoreContractId;
}

int runTimedSaveRestoreScenarios(int argc, char** argv) {
  auto const options = parseOptions(argc, argv);
  std::vector<ScenarioResult> results;
  for (auto const& callback : callbackModels(options)) {
    for (auto const& selected : options.scenarios) {
      auto const scenario = timedSaveRestoreScenario(selected);
      if (scenario == nullptr) {
        continue;
      }
      auto const started = Clock::now();
      ScenarioResult result{selected, callback.first, "passed", "", 0};
      if (options.logicalTimeImplementationName.empty()) {
        result.status = "skipped";
        result.message = "requires an adapter-supplied logical-time implementation";
      } else if (timedSaveRestoreRequiresDdm(selected) && options.ddmFom.empty()) {
        result.status = "skipped";
        result.message = "requires an adapter-supplied dimensional FOM";
      } else {
        try {
          scenario(options, callback.second);
        } catch (rti::Exception const& error) {
          result.status = "failed";
          result.message = toNarrow(error.name()) + ": " + toNarrow(error.what());
        } catch (std::exception const& error) {
          result.status = "failed";
          result.message = error.what();
        } catch (...) {
          result.status = "failed";
          result.message = "unknown non-standard exception";
        }
      }
      result.durationMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
          Clock::now() - started).count();
      results.push_back(result);
      std::cout << result.status << " " << result.id << " ["
                << result.callbackModel << "]";
      if (!result.message.empty()) {
        std::cout << ": " << result.message;
      }
      std::cout << '\n';
    }
  }
  if (!options.results.empty()) {
    writeResults(options.results, options, results);
  }
  if (!options.junit.empty()) {
    writeJUnit(options.junit, results);
  }
  auto const failures = std::count_if(
      results.begin(),
      results.end(),
      [](auto const& result) { return result.status == "failed"; });
  auto const skipped = std::count_if(
      results.begin(),
      results.end(),
      [](auto const& result) { return result.status == "skipped"; });
  std::cout << "summary passed=" << results.size() - failures - skipped
            << " skipped=" << skipped << " failed=" << failures << '\n';
  return failures == 0 ? 0 : 1;
}

PortableScenario regionalDdmScenario(std::string const& id) {
  if (id == regionalMultiAttributeUpdateScenario) {
    return scenarioRegionalMultiAttributeUpdate;
  }
  if (id == regionalMultiAttributeUpdateContractId) {
    return scenarioRegionalMultiAttributeUpdateContract;
  }
  if (id == regionalThreeDimensionalOverlapScenario) {
    return scenarioRegionalThreeDimensionalOverlap;
  }
  if (id == regionalThreeDimensionalOverlapContractId) {
    return scenarioRegionalThreeDimensionalOverlapContract;
  }
  if (id == regionalAttributeUpdateCallbackDdmRecheckScenario) {
    return scenarioRegionalAttributeUpdateCallbackDdmRecheck;
  }
  if (id == regionalAttributeUpdateCallbackDdmRecheckContractScenario) {
    return scenarioRegionalAttributeUpdateCallbackDdmRecheckContract;
  }
  if (id == regionalAttributeValueRequestFilteringScenario) {
    return scenarioRegionalAttributeValueRequestFiltering;
  }
  if (id == regionalAttributeValueRequestFilteringContractScenario) {
    return scenarioRegionalAttributeValueRequestFilteringContract;
  }
  if (id == regionalAttributeValueUpdateResponseRecheckScenario) {
    return scenarioRegionalAttributeValueUpdateResponseRecheck;
  }
  if (id == regionalAttributeValueUpdateResponseRecheckContractScenario) {
    return scenarioRegionalAttributeValueUpdateResponseRecheckContract;
  }
  if (id == allowRelaxedDdmScenario) {
    return scenarioAllowRelaxedDdm;
  }
  if (id == allowRelaxedDdmContractId) {
    return scenarioAllowRelaxedDdmContract;
  }
  if (id == regionalBoundariesScenario) {
    return scenarioRegionalBoundaries;
  }
  if (id == regionalBoundariesContractId) {
    return scenarioRegionalBoundariesContract;
  }
  if (id == ownershipTransferRegionalUpdateScenario) {
    return scenarioOwnershipTransferRegionalUpdate;
  }
  if (id == ownershipTransferRegionalUpdateContractId) {
    return scenarioOwnershipTransferRegionalUpdateContract;
  }
  return nullptr;
}

int runRegionalDdmScenarios(int argc, char** argv) {
  auto const options = parseOptions(argc, argv);
  std::vector<ScenarioResult> results;
  for (auto const& callback : callbackModels(options)) {
    for (auto const& selected : options.scenarios) {
      auto const scenario = regionalDdmScenario(selected);
      if (scenario == nullptr) {
        continue;
      }
      auto const started = Clock::now();
      ScenarioResult result{selected, callback.first, "passed", "", 0};
      if ((selected == allowRelaxedDdmScenario ||
           selected == allowRelaxedDdmContractId) &&
          (options.ddmFom.empty() || options.switchesFom.empty())) {
        result.status = "skipped";
        result.message =
            "requires adapter-supplied dimensional and switch-declaration FOMs";
      } else if ((selected == regionalBoundariesScenario ||
                  selected == regionalBoundariesContractId ||
                  selected == ownershipTransferRegionalUpdateScenario ||
                  selected == ownershipTransferRegionalUpdateContractId) &&
                 options.ddmFom.empty()) {
        result.status = "skipped";
        result.message = "requires an adapter-supplied dimensional FOM";
      } else if ((selected == regionalMultiAttributeUpdateScenario ||
                  selected == regionalMultiAttributeUpdateContractId) &&
                 options.multiAttributeFom.empty()) {
        result.status = "skipped";
        result.message =
            "requires an adapter-supplied multi-attribute dimensional FOM";
      } else if ((selected == regionalThreeDimensionalOverlapScenario ||
                  selected == regionalThreeDimensionalOverlapContractId) &&
                 options.threeDimensionalFom.empty()) {
        result.status = "skipped";
        result.message =
            "requires an adapter-supplied three-dimensional DDM FOM";
      } else if ((selected == regionalAttributeUpdateCallbackDdmRecheckScenario ||
                  selected == regionalAttributeUpdateCallbackDdmRecheckContractScenario ||
                  selected == regionalAttributeValueRequestFilteringScenario ||
                  selected == regionalAttributeValueRequestFilteringContractScenario ||
                  selected == regionalAttributeValueUpdateResponseRecheckScenario ||
                  selected == regionalAttributeValueUpdateResponseRecheckContractScenario) &&
                 options.ddmFom.empty()) {
        result.status = "skipped";
        result.message = "requires an adapter-supplied dimensional FOM";
      }
      if (result.status == "passed") {
        try {
          scenario(options, callback.second);
        } catch (rti::Exception const& error) {
          result.status = "failed";
          result.message = toNarrow(error.name()) + ": " + toNarrow(error.what());
        } catch (std::exception const& error) {
          result.status = "failed";
          result.message = error.what();
        } catch (...) {
          result.status = "failed";
          result.message = "unknown non-standard exception";
        }
      }
      result.durationMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
          Clock::now() - started).count();
      results.push_back(result);
      std::cout << result.status << " " << result.id << " ["
                << result.callbackModel << "]";
      if (!result.message.empty()) {
        std::cout << ": " << result.message;
      }
      std::cout << '\n';
    }
  }
  if (!options.results.empty()) {
    writeResults(options.results, options, results);
  }
  if (!options.junit.empty()) {
    writeJUnit(options.junit, results);
  }
  auto const failures = std::count_if(
      results.begin(),
      results.end(),
      [](auto const& result) { return result.status == "failed"; });
  auto const skipped = std::count_if(
      results.begin(),
      results.end(),
      [](auto const& result) { return result.status == "skipped"; });
  std::cout << "summary passed=" << results.size() - failures - skipped
            << " skipped=" << skipped << " failed=" << failures << '\n';
  return failures == 0 ? 0 : 1;
}

int runCustomTransportationTimestampedRegionalInteractionDeliveryScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      customTransportationTimestampedRegionalInteractionDeliveryId,
      customTransportationTimestampedRegionalInteractionDeliveryContractId,
      scenarioCustomTransportationTimestampedRegionalInteractionDelivery,
      scenarioCustomTransportationTimestampedRegionalInteractionDeliveryContract);
}

int runCustomTransportationAttributeDeliveryScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      customTransportationAttributeDeliveryId,
      customTransportationAttributeDeliveryContractId,
      scenarioCustomTransportationAttributeDelivery,
      scenarioCustomTransportationAttributeDeliveryContract);
}

int runCustomTransportationTimestampedAttributeDeliveryScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      customTransportationTimestampedAttributeDeliveryId,
      customTransportationTimestampedAttributeDeliveryContractId,
      scenarioCustomTransportationTimestampedAttributeDelivery,
      scenarioCustomTransportationTimestampedAttributeDeliveryContract);
}

int runCustomTransportationTimestampedAttributeAlternateAdvancesScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      customTransportationTimestampedAttributeAlternateAdvancesId,
      customTransportationTimestampedAttributeAlternateAdvancesContractId,
      scenarioCustomTransportationTimestampedAttributeAlternateAdvances,
      scenarioCustomTransportationTimestampedAttributeAlternateAdvancesContract);
}

int runCustomTransportationTimestampedInteractionAlternateAdvancesScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      customTransportationTimestampedInteractionAlternateAdvancesId,
      customTransportationTimestampedInteractionAlternateAdvancesContractId,
      scenarioCustomTransportationTimestampedInteractionAlternateAdvances,
      scenarioCustomTransportationTimestampedInteractionAlternateAdvancesContract);
}

int runCustomTransportationTimestampedDirectedDeliveryPortableScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      "cpp-tck.custom-transportation-timestamped-directed-delivery",
      "cpp-tck.custom-transportation-timestamped-directed-delivery-contract",
      scenarioCustomTransportationTimestampedDirectedDeliveryPortable,
      scenarioCustomTransportationTimestampedDirectedDeliveryPortableContract);
}

int runCustomTransportationInteractionDeliveryPortableScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      "cpp-tck.custom-transportation-interaction-delivery",
      "cpp-tck.custom-transportation-interaction-delivery-contract",
      scenarioCustomTransportationInteractionDeliveryPortable,
      scenarioCustomTransportationInteractionDeliveryPortableContract);
}

int runCustomTransportationTimestampedDeliveryPortableScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      "cpp-tck.custom-transportation-timestamped-delivery",
      "cpp-tck.custom-transportation-timestamped-delivery-contract",
      scenarioCustomTransportationTimestampedDeliveryPortable,
      scenarioCustomTransportationTimestampedDeliveryPortableContract);
}

int runCustomTransportationDirectedInteractionDeliveryScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      customTransportationDirectedInteractionDeliveryId,
      customTransportationDirectedInteractionDeliveryContractId,
      scenarioCustomTransportationDirectedInteractionDelivery,
      scenarioCustomTransportationDirectedInteractionDeliveryContract);
}

int runCustomTransportationTimestampedDirectedInteractionAlternateAdvancesScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      customTransportationTimestampedDirectedInteractionAlternateAdvancesId,
      customTransportationTimestampedDirectedInteractionAlternateAdvancesContractId,
      scenarioCustomTransportationTimestampedDirectedInteractionAlternateAdvances,
      scenarioCustomTransportationTimestampedDirectedInteractionAlternateAdvancesContract);
}

int runAutomaticResignDirectiveScenarios(int argc, char** argv) {
  auto const options = parseOptions(argc, argv);
  std::vector<ScenarioResult> results;
  for (auto const& callback : callbackModels(options)) {
    for (auto const& scenario : options.scenarios) {
      if (scenario != automaticResignDirectiveDeleteObjectsId &&
          scenario != automaticResignDirectiveDeleteObjectsContractId) {
        continue;
      }
      auto const started = Clock::now();
      ScenarioResult result{scenario, callback.first, "passed", "", 0};
      if (!options.connectionLossServerManaged) {
        result.status = "skipped";
        result.message =
            "requires an adapter-managed connection-loss fixture";
      } else {
        try {
          if (scenario == automaticResignDirectiveDeleteObjectsId) {
            scenarioAutomaticResignDirectiveDeleteObjects(options, callback.second);
          } else {
            scenarioAutomaticResignDirectiveDeleteObjectsContract(
                options,
                callback.second);
          }
        } catch (rti::Exception const& error) {
          result.status = "failed";
          result.message = toNarrow(error.name()) + ": " + toNarrow(error.what());
        } catch (std::exception const& error) {
          result.status = "failed";
          result.message = error.what();
        } catch (...) {
          result.status = "failed";
          result.message = "unknown non-standard exception";
        }
      }
      result.durationMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
          Clock::now() - started).count();
      results.push_back(result);
      std::cout << result.status << " " << result.id << " ["
                << result.callbackModel << "]";
      if (!result.message.empty()) {
        std::cout << ": " << result.message;
      }
      std::cout << '\n';
    }
  }
  if (!options.results.empty()) {
    writeResults(options.results, options, results);
  }
  if (!options.junit.empty()) {
    writeJUnit(options.junit, results);
  }
  auto const failures = std::count_if(
      results.begin(),
      results.end(),
      [](auto const& result) { return result.status == "failed"; });
  auto const skipped = std::count_if(
      results.begin(),
      results.end(),
      [](auto const& result) { return result.status == "skipped"; });
  std::cout << "summary passed=" << results.size() - failures - skipped
            << " skipped=" << skipped << " failed=" << failures << '\n';
  return failures == 0 ? 0 : 1;
}

bool hasAutomaticResignDirectiveScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == automaticResignDirectiveDeleteObjectsId ||
        scenario == automaticResignDirectiveDeleteObjectsContractId) {
      return true;
    }
  }
  return false;
}

int runPublicHandleDecodingScenarios(int argc, char** argv) {
  auto const options = parseOptions(argc, argv);
  std::vector<ScenarioResult> results;
  for (auto const& callback : callbackModels(options)) {
    for (auto const& scenario : options.scenarios) {
      if (scenario != publicHandleDecodingId &&
          scenario != publicHandleDecodingContractId) {
        continue;
      }
      auto const started = Clock::now();
      ScenarioResult result{scenario, callback.first, "passed", "", 0};
      try {
        if (scenario == publicHandleDecodingId) {
          scenarioPublicHandleDecoding(options, callback.second);
        } else {
          scenarioPublicHandleDecodingContract(options, callback.second);
        }
      } catch (rti::Exception const& error) {
        result.status = "failed";
        result.message = toNarrow(error.name()) + ": " + toNarrow(error.what());
      } catch (std::exception const& error) {
        result.status = "failed";
        result.message = error.what();
      } catch (...) {
        result.status = "failed";
        result.message = "unknown non-standard exception";
      }
      result.durationMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
          Clock::now() - started).count();
      results.push_back(result);
      std::cout << result.status << " " << result.id << " ["
                << result.callbackModel << "]";
      if (!result.message.empty()) {
        std::cout << ": " << result.message;
      }
      std::cout << '\n';
    }
  }
  if (!options.results.empty()) {
    writeResults(options.results, options, results);
  }
  if (!options.junit.empty()) {
    writeJUnit(options.junit, results);
  }
  auto const failures = std::count_if(
      results.begin(),
      results.end(),
      [](auto const& result) { return result.status == "failed"; });
  auto const skipped = std::count_if(
      results.begin(),
      results.end(),
      [](auto const& result) { return result.status == "skipped"; });
  std::cout << "summary passed=" << results.size() - failures - skipped
            << " skipped=" << skipped << " failed=" << failures << '\n';
  return failures == 0 ? 0 : 1;
}

bool hasPublicHandleDecodingScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == publicHandleDecodingId ||
        scenario == publicHandleDecodingContractId) {
      return true;
    }
  }
  return false;
}

bool hasMomTransportationTypeChangeRequestScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == momTransportationTypeChangeRequestScenario ||
        scenario == momTransportationTypeChangeRequestContractId) {
      return true;
    }
  }
  return false;
}

bool hasJoinedFederateMomRegisteredObjectCountScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == joinedFederateMomRegisteredObjectCountScenario ||
        scenario == joinedFederateMomRegisteredObjectCountContractId) {
      return true;
    }
  }
  return false;
}

bool hasJoinedFederateMomDeletableObjectReportScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == joinedFederateMomDeletableObjectReportScenario ||
        scenario == joinedFederateMomDeletableObjectReportContractId) {
      return true;
    }
  }
  return false;
}

bool hasJoinedFederateMomDeletableObjectCountScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == joinedFederateMomDeletableObjectCountScenario ||
        scenario == joinedFederateMomDeletableObjectCountContractId) {
      return true;
    }
  }
  return false;
}

bool hasReceiveOrderAttributeUpdateScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == receiveOrderAttributeUpdateScenario ||
        scenario == receiveOrderAttributeUpdateContractId) {
      return true;
    }
  }
  return false;
}

bool hasReceiveOrderInteractionScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == receiveOrderInteractionScenario ||
        scenario == receiveOrderInteractionContractId) {
      return true;
    }
  }
  return false;
}

bool hasReceiveOrderObjectRemovalScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == receiveOrderObjectRemovalScenario ||
        scenario == receiveOrderObjectRemovalContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederationRestoreAbortScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federationRestoreAbortScenario ||
        scenario == federationRestoreAbortContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederationRestoreOwnershipAssumptionScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federationRestoreOwnershipAssumptionScenario ||
        scenario == federationRestoreOwnershipAssumptionContractId) {
      return true;
    }
  }
  return false;
}

bool hasOwnershipAcquisitionIfAvailableScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == ownershipAcquisitionIfAvailableScenario ||
        scenario == ownershipAcquisitionIfAvailableContractId) {
      return true;
    }
  }
  return false;
}

bool hasAutoProvideDisabledDiscoveryOnlyScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == autoProvideDisabledDiscoveryOnlyScenario ||
        scenario == autoProvideDisabledDiscoveryOnlyContractId) {
      return true;
    }
  }
  return false;
}

bool hasAutoProvideDisabledExplicitRequestScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == autoProvideDisabledExplicitRequestScenario ||
        scenario == autoProvideDisabledExplicitRequestContractId) {
      return true;
    }
  }
  return false;
}

bool hasObjectRegistrationServiceBoundariesScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == objectRegistrationServiceBoundariesScenario ||
        scenario == objectRegistrationServiceBoundariesContractId) {
      return true;
    }
  }
  return false;
}

bool hasObjectDeletionServiceBoundariesScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == objectDeletionServiceBoundariesScenario ||
        scenario == objectDeletionServiceBoundariesContractId) {
      return true;
    }
  }
  return false;
}

bool hasAttributeUpdateServiceBoundariesScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == attributeUpdateServiceBoundariesScenario ||
        scenario == attributeUpdateServiceBoundariesContractId) {
      return true;
    }
  }
  return false;
}

bool hasInteractionServiceBoundariesScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == interactionServiceBoundariesScenario ||
        scenario == interactionServiceBoundariesContractId) {
      return true;
    }
  }
  return false;
}

bool hasAttributeValueRequestServiceBoundariesScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == attributeValueRequestServiceBoundariesScenario ||
        scenario == attributeValueRequestServiceBoundariesContractId) {
      return true;
    }
  }
  return false;
}

bool hasConnectionServiceBoundariesScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == connectionServiceBoundariesScenario ||
        scenario == connectionServiceBoundariesContractId) {
      return true;
    }
  }
  return false;
}

bool hasCustomTransportationTimestampedRegionalInteractionDeliveryScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == customTransportationTimestampedRegionalInteractionDeliveryId ||
        scenario == customTransportationTimestampedRegionalInteractionDeliveryContractId) {
      return true;
    }
  }
  return false;
}

bool hasCustomTransportationAttributeDeliveryScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == customTransportationAttributeDeliveryId ||
        scenario == customTransportationAttributeDeliveryContractId) {
      return true;
    }
  }
  return false;
}

bool hasCustomTransportationTimestampedAttributeDeliveryScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == customTransportationTimestampedAttributeDeliveryId ||
        scenario == customTransportationTimestampedAttributeDeliveryContractId) {
      return true;
    }
  }
  return false;
}

bool hasCustomTransportationTimestampedAttributeAlternateAdvancesScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == customTransportationTimestampedAttributeAlternateAdvancesId ||
        scenario == customTransportationTimestampedAttributeAlternateAdvancesContractId) {
      return true;
    }
  }
  return false;
}

bool hasCustomTransportationTimestampedInteractionAlternateAdvancesScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == customTransportationTimestampedInteractionAlternateAdvancesId ||
        scenario == customTransportationTimestampedInteractionAlternateAdvancesContractId) {
      return true;
    }
  }
  return false;
}

bool hasCustomTransportationTimestampedDirectedDeliveryPortableScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == "cpp-tck.custom-transportation-timestamped-directed-delivery" ||
        scenario == "cpp-tck.custom-transportation-timestamped-directed-delivery-contract") {
      return true;
    }
  }
  return false;
}

bool hasCustomTransportationInteractionDeliveryPortableScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == "cpp-tck.custom-transportation-interaction-delivery" ||
        scenario == "cpp-tck.custom-transportation-interaction-delivery-contract") {
      return true;
    }
  }
  return false;
}

bool hasCustomTransportationTimestampedDeliveryPortableScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == "cpp-tck.custom-transportation-timestamped-delivery" ||
        scenario == "cpp-tck.custom-transportation-timestamped-delivery-contract") {
      return true;
    }
  }
  return false;
}

bool hasCustomTransportationDirectedInteractionDeliveryScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == customTransportationDirectedInteractionDeliveryId ||
        scenario == customTransportationDirectedInteractionDeliveryContractId) {
      return true;
    }
  }
  return false;
}

bool hasCustomTransportationTimestampedDirectedInteractionAlternateAdvancesScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == customTransportationTimestampedDirectedInteractionAlternateAdvancesId ||
        scenario == customTransportationTimestampedDirectedInteractionAlternateAdvancesContractId) {
      return true;
    }
  }
  return false;
}

bool hasDelaySubscriptionEvaluationScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    if (delaySubscriptionEvaluationScenario(std::string(argv[index + 1])) != nullptr) {
      return true;
    }
  }
  return false;
}

bool hasFederationSaveRestoreInterlocksScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federationSaveRestoreInterlocksScenario ||
        scenario == federationSaveRestoreInterlocksContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimedSaveRestoreScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    if (timedSaveRestoreScenario(std::string(argv[index + 1])) != nullptr) {
      return true;
    }
  }
  return false;
}

bool hasRegionalDdmScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    if (regionalDdmScenario(std::string(argv[index + 1])) != nullptr) {
      return true;
    }
  }
  return false;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (hasAutomaticResignDirectiveScenario(argc, argv)) {
      return runAutomaticResignDirectiveScenarios(argc, argv);
    }
    if (hasPublicHandleDecodingScenario(argc, argv)) {
      return runPublicHandleDecodingScenarios(argc, argv);
    }
    if (hasMomTransportationTypeChangeRequestScenario(argc, argv)) {
      return runMomTransportationTypeChangeRequestScenarios(argc, argv);
    }
    if (hasJoinedFederateMomRegisteredObjectCountScenario(argc, argv)) {
      return runJoinedFederateMomRegisteredObjectCountScenarios(argc, argv);
    }
    if (hasJoinedFederateMomDeletableObjectReportScenario(argc, argv)) {
      return runJoinedFederateMomDeletableObjectReportScenarios(argc, argv);
    }
    if (hasJoinedFederateMomDeletableObjectCountScenario(argc, argv)) {
      return runJoinedFederateMomDeletableObjectCountScenarios(argc, argv);
    }
    if (hasReceiveOrderAttributeUpdateScenario(argc, argv)) {
      return runReceiveOrderAttributeUpdateScenarios(argc, argv);
    }
    if (hasReceiveOrderInteractionScenario(argc, argv)) {
      return runReceiveOrderInteractionScenarios(argc, argv);
    }
    if (hasReceiveOrderObjectRemovalScenario(argc, argv)) {
      return runReceiveOrderObjectRemovalScenarios(argc, argv);
    }
    if (hasFederationRestoreAbortScenario(argc, argv)) {
      return runFederationRestoreAbortScenarios(argc, argv);
    }
    if (hasFederationRestoreOwnershipAssumptionScenario(argc, argv)) {
      return runFederationRestoreOwnershipAssumptionScenarios(argc, argv);
    }
    if (hasOwnershipAcquisitionIfAvailableScenario(argc, argv)) {
      return runOwnershipAcquisitionIfAvailableScenarios(argc, argv);
    }
    if (hasAutoProvideDisabledDiscoveryOnlyScenario(argc, argv)) {
      return runAutoProvideDisabledDiscoveryOnlyScenarios(argc, argv);
    }
    if (hasAutoProvideDisabledExplicitRequestScenario(argc, argv)) {
      return runAutoProvideDisabledExplicitRequestScenarios(argc, argv);
    }
    if (hasObjectRegistrationServiceBoundariesScenario(argc, argv)) {
      return runObjectRegistrationServiceBoundariesScenarios(argc, argv);
    }
    if (hasObjectDeletionServiceBoundariesScenario(argc, argv)) {
      return runObjectDeletionServiceBoundariesScenarios(argc, argv);
    }
    if (hasAttributeUpdateServiceBoundariesScenario(argc, argv)) {
      return runAttributeUpdateServiceBoundariesScenarios(argc, argv);
    }
    if (hasInteractionServiceBoundariesScenario(argc, argv)) {
      return runInteractionServiceBoundariesScenarios(argc, argv);
    }
    if (hasAttributeValueRequestServiceBoundariesScenario(argc, argv)) {
      return runAttributeValueRequestServiceBoundariesScenarios(argc, argv);
    }
    if (hasConnectionServiceBoundariesScenario(argc, argv)) {
      return runConnectionServiceBoundariesScenarios(argc, argv);
    }
    if (hasCustomTransportationAttributeDeliveryScenario(argc, argv)) {
      return runCustomTransportationAttributeDeliveryScenarios(argc, argv);
    }
    if (hasCustomTransportationTimestampedAttributeDeliveryScenario(argc, argv)) {
      return runCustomTransportationTimestampedAttributeDeliveryScenarios(argc, argv);
    }
    if (hasCustomTransportationTimestampedAttributeAlternateAdvancesScenario(argc, argv)) {
      return runCustomTransportationTimestampedAttributeAlternateAdvancesScenarios(argc, argv);
    }
    if (hasCustomTransportationTimestampedInteractionAlternateAdvancesScenario(argc, argv)) {
      return runCustomTransportationTimestampedInteractionAlternateAdvancesScenarios(argc, argv);
    }
    if (hasCustomTransportationTimestampedDirectedDeliveryPortableScenario(argc, argv)) {
      return runCustomTransportationTimestampedDirectedDeliveryPortableScenarios(argc, argv);
    }
    if (hasCustomTransportationInteractionDeliveryPortableScenario(argc, argv)) {
      return runCustomTransportationInteractionDeliveryPortableScenarios(argc, argv);
    }
    if (hasCustomTransportationTimestampedDeliveryPortableScenario(argc, argv)) {
      return runCustomTransportationTimestampedDeliveryPortableScenarios(argc, argv);
    }
    if (hasCustomTransportationDirectedInteractionDeliveryScenario(argc, argv)) {
      return runCustomTransportationDirectedInteractionDeliveryScenarios(argc, argv);
    }
    if (hasCustomTransportationTimestampedDirectedInteractionAlternateAdvancesScenario(argc, argv)) {
      return runCustomTransportationTimestampedDirectedInteractionAlternateAdvancesScenarios(argc, argv);
    }
    if (hasDelaySubscriptionEvaluationScenario(argc, argv)) {
      return runDelaySubscriptionEvaluationScenarios(argc, argv);
    }
    if (hasFederationSaveRestoreInterlocksScenario(argc, argv)) {
      return runFederationSaveRestoreInterlocksScenarios(argc, argv);
    }
    if (hasTimedSaveRestoreScenario(argc, argv)) {
      return runTimedSaveRestoreScenarios(argc, argv);
    }
    if (hasRegionalDdmScenario(argc, argv)) {
      return runRegionalDdmScenarios(argc, argv);
    }
    if (hasCustomTransportationTimestampedRegionalInteractionDeliveryScenario(argc, argv)) {
      return runCustomTransportationTimestampedRegionalInteractionDeliveryScenarios(argc, argv);
    }
    return hla_rti_cpp_tck_original_main(argc, argv);
  } catch (std::exception const& error) {
    std::cerr << "error: " << error.what() << '\n';
    return 2;
  }
}
