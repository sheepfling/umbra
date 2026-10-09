#include <catch2/catch_test_macros.hpp>

#include "internal/fom/fdd_document.hpp"
#include "internal/time/federate_time_state.hpp"
#include "internal/fom/fom_catalog.hpp"
#include "internal/federation/federation_management_coordinator.hpp"
#include "internal/federation/federation_registry.hpp"
#include "internal/handles/handle_variable_array_encoding.hpp"
#include "internal/fom/libxml2_fom_composer.hpp"
#include "internal/fom/libxml2_fom_validator.hpp"
#include "internal/time/reference_time_selection.hpp"

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <vector>

#include <RTI/time/HLAinteger64Time.h>
#include <RTI/encoding/BasicDataElements.h>

#include "fom_composer_test_support.hpp"

#ifndef UMBRA_SOURCE_DIRECTORY
#error "UMBRA_SOURCE_DIRECTORY must be defined by CMake for FOM resource tests."
#endif

namespace {

using umbra::detail::FomCompositionStatus;
using umbra::detail::FomModuleKind;
using umbra::detail::FomValidationRequest;
using umbra::detail::FomValidationStatus;
using umbra::detail::FederationManagementCoordinator;
using umbra::detail::FederationManagementResources;
using umbra::detail::FederationPreparationStatus;
using umbra::detail::FederationDefinition;
using umbra::detail::FederationRegistryStatus;
using umbra::detail::FederateTimeState;
using umbra::detail::InteractionClassDeclarationStatus;
using umbra::detail::InteractionProducer;
using umbra::detail::MomServiceReportDisposition;
using umbra::detail::ObjectClassAttributeDeclarationStatus;
using umbra::detail::ObjectInstanceRegistrationStatus;
using umbra::detail::LibXml2FomModuleComposer;
using umbra::detail::LibXml2FomValidator;
using umbra::detail::PrevalidatedFomModule;
using umbra::detail::ReceiveOrderInteractionStatus;
using umbra::detail::ReferenceLogicalTimeSelectionStatus;
using umbra::detail::ReferenceLogicalTimeSelector;

}  // namespace

TEST_CASE(
    "The registry builds a private joined-federate MOM snapshot from canonical Join FOM modules",
    "[unit][kernel][federation-registry][fom][mom][mom-object-foundation]") {
  umbra::detail::EmbeddedFederationRegistry registry;
  auto definition = composedRestaurantDefinition();
  REQUIRE(definition.fomModules.size() >= 2U);
  auto firstJoinModule = definition.fomModules.back();
  REQUIRE(firstJoinModule.kind == FomModuleKind::fom);
  auto repeatedJoinModule = firstJoinModule;
  repeatedJoinModule.designator = L"urn:umbra:test:duplicate-designator";

  REQUIRE(registry.create(L"mom-object-foundation", std::move(definition)).status ==
          FederationRegistryStatus::applied);
  auto const joined = registry.join(
      L"mom-object-foundation", L"observer", L"mom-object-subject");
  REQUIRE(joined.membership);
  REQUIRE(
      registry.establishJoinedFederateMomObject(
          L"mom-object-foundation",
          joined.membership->id,
          {
              L"umbra-embedded",
              L"Umbra test",
              {firstJoinModule, repeatedJoinModule},
              L"C:\\umbra-test-service-reports\\subject.log",
          }) == umbra::detail::JoinedFederateMomObjectStatus::applied);

  auto const snapshot = registry.joinedFederateMomObjectFor(
      L"mom-object-foundation", joined.membership->id);
  REQUIRE(snapshot);
  REQUIRE(snapshot->objectInstanceHandle != 0U);
  REQUIRE(snapshot->immutableFederatePoint.specificationCommitted);
  REQUIRE(snapshot->immutableFederatePoint.dimensionHandles.size() == 1U);
  REQUIRE(snapshot->effectiveAttributeHandles.size() > snapshot->initialAttributeValues.size());

  auto initialAttribute = [&](char const* attributeName)
      -> rti1516_2025::VariableLengthData const& {
    auto const attributeHandle = registry.attributeHandleFor(
        L"mom-object-foundation",
        "HLAobjectRoot.HLAmanager.HLAfederate",
        attributeName);
    REQUIRE(attributeHandle);
    auto const encoded = snapshot->initialAttributeValues.find(*attributeHandle);
    REQUIRE(encoded != snapshot->initialAttributeValues.end());
    return encoded->second;
  };
  auto decodeUnicodeAttribute = [&](char const* attributeName) {
    auto const& encoded = initialAttribute(attributeName);
    auto const* raw = static_cast<rti1516_2025::Octet const*>(encoded.data());
    REQUIRE(raw != nullptr);
    std::vector<rti1516_2025::Octet> bytes(raw, raw + encoded.size());
    rti1516_2025::HLAunicodeString decoded;
    REQUIRE(decoded.decodeFrom(bytes, 0U) == bytes.size());
    return std::wstring{decoded.get()};
  };

  // The seven direct Table 8 values must use the types retained from the
  // official MIM rather than a parallel homegrown MOM schema. In particular,
  // the federate handle is the standard HLAvariableArray<HLAbyte> value while
  // the report-file location is the exact Unicode string selected at Join.
  auto const encodedFederateHandle = initialAttribute("HLAfederateHandle");
  auto const decodedFederateHandle =
      rti1516_2025::umbra_binding_detail::decodeUmbraHandleVariableArray(
          encodedFederateHandle);
  REQUIRE(decodedFederateHandle);
  REQUIRE(*decodedFederateHandle == joined.membership->id);
  REQUIRE(decodeUnicodeAttribute("HLAfederateName") == L"mom-object-subject");
  REQUIRE(decodeUnicodeAttribute("HLAfederateType") == L"observer");
  REQUIRE(decodeUnicodeAttribute("HLAfederateHost") == L"umbra-embedded");
  REQUIRE(decodeUnicodeAttribute("HLARTIversion") == L"Umbra test");
  REQUIRE(decodeUnicodeAttribute("HLAreportServiceFile") ==
          L"C:\\umbra-test-service-reports\\subject.log");

  auto const moduleListAttribute = registry.attributeHandleFor(
      L"mom-object-foundation",
      "HLAobjectRoot.HLAmanager.HLAfederate",
      "HLAFOMmoduleDesignatorList");
  auto const deletePrivilegeAttribute = registry.attributeHandleFor(
      L"mom-object-foundation",
      "HLAobjectRoot.HLAmanager.HLAfederate",
      "HLAprivilegeToDeleteObject");
  REQUIRE(moduleListAttribute);
  REQUIRE(deletePrivilegeAttribute);
  REQUIRE(snapshot->effectiveAttributeHandles.contains(*deletePrivilegeAttribute));
  auto const encodedModules = snapshot->initialAttributeValues.find(*moduleListAttribute);
  REQUIRE(encodedModules != snapshot->initialAttributeValues.end());

  auto const* raw = static_cast<rti1516_2025::Octet const*>(encodedModules->second.data());
  REQUIRE(raw != nullptr);
  std::vector<rti1516_2025::Octet> bytes(
      raw,
      raw + encodedModules->second.size());
  rti1516_2025::HLAinteger32BE count;
  auto index = count.decodeFrom(bytes, 0U);
  REQUIRE(count.get() == 1);
  rti1516_2025::HLAunicodeString retainedDesignator;
  index = retainedDesignator.decodeFrom(bytes, index);
  REQUIRE(index == bytes.size());
  REQUIRE(retainedDesignator.get() == firstJoinModule.designator);

  REQUIRE(registry.resign(L"mom-object-foundation", joined.membership->id).status ==
          FederationRegistryStatus::applied);
  REQUIRE_FALSE(registry.joinedFederateMomObjectFor(
      L"mom-object-foundation", joined.membership->id));
}

TEST_CASE(
    "The registry plans 2025 MOM reports with an RTI-owned normalized endpoint",
    "[unit][fom][composition][mom][ddm][service-reporting]") {

  umbra::detail::EmbeddedFederationRegistry registry;
  auto definition = composedRestaurantDefinition();
  REQUIRE(registry.create(L"mom-report-route", std::move(definition)).status ==
          FederationRegistryStatus::applied);

  auto callbackRoute = [](umbra::detail::FederateCallbackInvocation) {};
  auto const subject = registry.join(
      L"mom-report-route", L"subject", L"subject", callbackRoute);
  auto const observer = registry.join(
      L"mom-report-route", L"observer", L"observer", callbackRoute);
  REQUIRE(subject.membership);
  REQUIRE(observer.membership);
  REQUIRE(registry.setServiceReportingSwitch(
              L"mom-report-route", subject.membership->id, true) ==
          umbra::detail::ServiceReportingSwitchStatus::applied);
  REQUIRE(registry.setServiceReportingSwitch(
              L"mom-report-route", observer.membership->id, false) ==
          umbra::detail::ServiceReportingSwitchStatus::applied);

  auto const reportClass = registry.interactionClassHandleFor(
      L"mom-report-route",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  auto const federateDimension = registry.dimensionHandleFor(L"mom-report-route", "HLAfederate");
  auto const serviceGroupDimension = registry.dimensionHandleFor(
      L"mom-report-route", "HLAserviceGroup");
  REQUIRE(reportClass);
  REQUIRE(federateDimension);
  REQUIRE(serviceGroupDimension);
  auto const normalizedSubject = registry.normalizedFederateHandleValueFor(
      L"mom-report-route", subject.membership->id);
  REQUIRE(normalizedSubject);
  auto const observerRegion = registry.createRegion(
      L"mom-report-route",
      observer.membership->id,
      {*federateDimension, *serviceGroupDimension});
  REQUIRE(observerRegion.status == umbra::detail::RegionServiceStatus::applied);
  REQUIRE(registry.setRangeBounds(
              L"mom-report-route",
              observer.membership->id,
              observerRegion.regionHandle,
              *federateDimension,
              {*normalizedSubject, *normalizedSubject + 1U}) ==
          umbra::detail::RegionServiceStatus::applied);
  REQUIRE(registry.setRangeBounds(
              L"mom-report-route",
              observer.membership->id,
              observerRegion.regionHandle,
              *serviceGroupDimension,
              {6U, 7U}) == umbra::detail::RegionServiceStatus::applied);
  REQUIRE(registry.commitRegionModifications(
              L"mom-report-route", observer.membership->id, {observerRegion.regionHandle}) ==
          umbra::detail::RegionServiceStatus::applied);
  REQUIRE(registry.setInteractionClassRegionalSubscription(
              L"mom-report-route",
              observer.membership->id,
              *reportClass,
              {observerRegion.regionHandle},
              true) == umbra::detail::RegionalInteractionClassDeclarationStatus::applied);

  auto const plan = registry.planMomServiceReport(
      L"mom-report-route", subject.membership->id, 6U);
  REQUIRE(plan.disposition == MomServiceReportDisposition::interaction);
  REQUIRE(plan.producer.kind() == InteractionProducer::Kind::rti);
  REQUIRE_FALSE(plan.producer.joinedFederateId());
  REQUIRE(plan.interactionClassHandle == *reportClass);
  REQUIRE(plan.endpointRegionHandle != 0U);
  REQUIRE(plan.endpointRegion.dimensionHandles ==
          std::set<std::uint64_t>{*federateDimension, *serviceGroupDimension});
  REQUIRE(plan.endpointRegion.committedRangeBounds.at(*federateDimension).upperBound ==
          plan.endpointRegion.committedRangeBounds.at(*federateDimension).lowerBound + 1U);
  REQUIRE(plan.endpointRegion.committedRangeBounds.at(*serviceGroupDimension).lowerBound == 6U);
  REQUIRE(plan.endpointRegion.committedRangeBounds.at(*serviceGroupDimension).upperBound == 7U);
  REQUIRE(plan.recipients.size() == 1U);
  REQUIRE(plan.recipients.front().federateId == observer.membership->id);
  REQUIRE(registry.momServiceReportRecipientFor(
              L"mom-report-route",
              subject.membership->id,
              observer.membership->id,
              6U));

  // This direct recipient query is the callback-time subscription predicate
  // that a future public MOM delivery will use.  It must not retain a
  // planning-time recipient after the observer withdraws its exact regional
  // declaration, and the endpoint's two-dimensional range must remain
  // selective when the subscription is restored.
  REQUIRE(registry.setInteractionClassRegionalSubscription(
              L"mom-report-route",
              observer.membership->id,
              *reportClass,
              {observerRegion.regionHandle},
              false) == umbra::detail::RegionalInteractionClassDeclarationStatus::applied);
  REQUIRE_FALSE(registry.momServiceReportRecipientFor(
      L"mom-report-route", subject.membership->id, observer.membership->id, 6U));
  REQUIRE(registry.setInteractionClassRegionalSubscription(
              L"mom-report-route",
              observer.membership->id,
              *reportClass,
              {observerRegion.regionHandle},
              true) == umbra::detail::RegionalInteractionClassDeclarationStatus::applied);
  REQUIRE_FALSE(registry.momServiceReportRecipientFor(
      L"mom-report-route", subject.membership->id, observer.membership->id, 5U));
  REQUIRE(registry.momServiceReportRecipientFor(
              L"mom-report-route",
              subject.membership->id,
              observer.membership->id,
              6U));

  auto const firstReserved = registry.reserveMomServiceReport(
      L"mom-report-route", subject.membership->id, 6U);
  auto const secondReserved = registry.reserveMomServiceReport(
      L"mom-report-route", subject.membership->id, 6U);
  REQUIRE(firstReserved.acceptedForEmission);
  REQUIRE(firstReserved.serialNumber == 0U);
  REQUIRE(secondReserved.acceptedForEmission);
  REQUIRE(secondReserved.serialNumber == 1U);

  // Invalid/suppressed report requests are not report emissions and therefore
  // cannot consume the per-joined-federate serial sequence.
  auto const rejectedReserved = registry.reserveMomServiceReport(
      L"mom-report-route", subject.membership->id, 7U);
  REQUIRE_FALSE(rejectedReserved.acceptedForEmission);
  REQUIRE(rejectedReserved.routing.disposition == MomServiceReportDisposition::invalid_service_group);

  REQUIRE(registry.setSendServiceReportsToFileSwitch(
              L"mom-report-route", subject.membership->id, true) ==
          FederationRegistryStatus::applied);
  auto const filePlan = registry.planMomServiceReport(
      L"mom-report-route", subject.membership->id, 6U);
  REQUIRE(filePlan.disposition == MomServiceReportDisposition::report_to_file);
  REQUIRE(filePlan.recipients.empty());
  auto const fileReserved = registry.reserveMomServiceReport(
      L"mom-report-route", subject.membership->id, 6U);
  REQUIRE(fileReserved.acceptedForEmission);
  REQUIRE(fileReserved.serialNumber == 2U);
  REQUIRE(fileReserved.routing.disposition == MomServiceReportDisposition::report_to_file);
}

TEST_CASE(
    "The registry plans HLAreportFederateLost before a fault-driven resignation",
    "[unit][kernel][federation-registry][mom][connection-lost][ddm]") {

  umbra::detail::EmbeddedFederationRegistry registry;
  std::wstring const federationName = L"federate-lost-report-route";
  REQUIRE(
      registry.create(federationName, composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);

  auto lostTimeState = std::make_shared<FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  auto observerTimeState = std::make_shared<FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  auto unsubscriberTimeState = std::make_shared<FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  auto const lost = registry.joinWithTimeState(
      federationName,
      std::move(lostTimeState),
      L"lost",
      L"lost",
      [](umbra::detail::FederateCallbackInvocation) {});
  auto const observer = registry.joinWithTimeState(
      federationName,
      std::move(observerTimeState),
      L"observer",
      L"observer",
      [](umbra::detail::FederateCallbackInvocation) {});
  auto const unsubscriber = registry.joinWithTimeState(
      federationName,
      std::move(unsubscriberTimeState),
      L"observer",
      L"unsubscribed",
      [](umbra::detail::FederateCallbackInvocation) {});
  REQUIRE(lost.membership);
  REQUIRE(observer.membership);
  REQUIRE(unsubscriber.membership);

  std::string const reportClassName =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportFederateLost";
  auto const reportClass = registry.interactionClassHandleFor(federationName, reportClassName);
  auto const federateDimension = registry.dimensionHandleFor(federationName, "HLAfederate");
  auto const federateParameter =
      registry.parameterHandleFor(federationName, reportClassName, "HLAfederate");
  auto const federateNameParameter =
      registry.parameterHandleFor(federationName, reportClassName, "HLAfederateName");
  auto const timestampParameter =
      registry.parameterHandleFor(federationName, reportClassName, "HLAtimeStamp");
  auto const faultDescriptionParameter =
      registry.parameterHandleFor(federationName, reportClassName, "HLAfaultDescription");
  auto const normalizedLost = registry.normalizedFederateHandleValueFor(
      federationName,
      lost.membership->id);
  REQUIRE(reportClass);
  REQUIRE(federateDimension);
  REQUIRE(federateParameter);
  REQUIRE(federateNameParameter);
  REQUIRE(timestampParameter);
  REQUIRE(faultDescriptionParameter);
  REQUIRE(normalizedLost);
  REQUIRE(
      registry.setInteractionClassSubscription(
          federationName,
          observer.membership->id,
          *reportClass,
          true) == InteractionClassDeclarationStatus::applied);

  auto const plan = registry.planFederateLostReport(federationName, lost.membership->id);
  REQUIRE(plan.status == umbra::detail::FederateLostReportStatus::applied);
  REQUIRE(plan.producer.kind() == InteractionProducer::Kind::rti);
  REQUIRE_FALSE(plan.producer.joinedFederateId());
  REQUIRE(plan.reportedFederateId == lost.membership->id);
  REQUIRE(plan.reportedFederateName == L"lost");
  REQUIRE_FALSE(plan.reportedFederateWasTimeRegulating);
  REQUIRE(plan.lastKnownTime);
  REQUIRE(plan.lastKnownTime->implementationName() == L"HLAinteger64Time");
  auto const* lastKnownTime = dynamic_cast<rti1516_2025::HLAinteger64Time const*>(
      plan.lastKnownTime.get());
  REQUIRE(lastKnownTime != nullptr);
  REQUIRE(lastKnownTime->getTime() == 0);
  REQUIRE(plan.routing.interactionClassHandle == *reportClass);
  REQUIRE(plan.routing.federateParameterHandle == *federateParameter);
  REQUIRE(plan.routing.federateNameParameterHandle == *federateNameParameter);
  REQUIRE(plan.routing.timestampParameterHandle == *timestampParameter);
  REQUIRE(plan.routing.faultDescriptionParameterHandle == *faultDescriptionParameter);
  REQUIRE(plan.routing.endpointRegionHandle != 0U);
  REQUIRE(plan.routing.endpointRegion.dimensionHandles ==
          std::set<std::uint64_t>{*federateDimension});
  REQUIRE(plan.routing.endpointRegion.committedRangeBounds.at(*federateDimension).lowerBound ==
          *normalizedLost);
  REQUIRE(plan.routing.endpointRegion.committedRangeBounds.at(*federateDimension).upperBound ==
          *normalizedLost + 1U);
  REQUIRE(plan.recipients.size() == 1U);
  REQUIRE(plan.recipients.front().federateId == observer.membership->id);

  // The callback-time predicate deliberately survives the reported
  // federate's departure but does not retain a subscriber that later removes
  // its declaration. This is the boundary used by the adapter's queued report.
  REQUIRE(
      registry.connectionLost(federationName, lost.membership->id).status ==
      FederationRegistryStatus::applied);
  REQUIRE(registry.federateLostReportRecipientFor(
      federationName,
      lost.membership->id,
      observer.membership->id));
  REQUIRE_FALSE(registry.federateLostReportRecipientFor(
      federationName,
      lost.membership->id,
      unsubscriber.membership->id));
  REQUIRE(
      registry.setInteractionClassSubscription(
          federationName,
          observer.membership->id,
          *reportClass,
          false) == InteractionClassDeclarationStatus::applied);
  REQUIRE_FALSE(registry.federateLostReportRecipientFor(
      federationName,
      lost.membership->id,
      observer.membership->id));
}

TEST_CASE(
    "The federation registry retains independent interaction declarations until removal or resign",
    "[unit][kernel][federation-registry][declaration-management]") {

  umbra::detail::EmbeddedFederationRegistry registry;
  REQUIRE(
      registry.create(L"declaration-exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);

  auto publisher = registry.join(L"declaration-exercise", L"publisher", L"publisher");
  auto subscriber = registry.join(L"declaration-exercise", L"subscriber", L"subscriber");
  REQUIRE(publisher.membership);
  REQUIRE(subscriber.membership);
  auto const takeOrder = registry.interactionClassHandleFor(
      L"declaration-exercise",
      "HLAinteractionRoot.ServerAction.TakeOrder");
  REQUIRE(takeOrder);

  auto publisherState = registry.interactionClassDeclarationFor(
      L"declaration-exercise",
      publisher.membership->id,
      *takeOrder);
  REQUIRE(publisherState);
  REQUIRE_FALSE(publisherState->published);
  REQUIRE_FALSE(publisherState->subscriptionActive.has_value());

  REQUIRE(
      registry.setInteractionClassPublication(
          L"missing",
          publisher.membership->id,
          *takeOrder,
          true) == InteractionClassDeclarationStatus::federation_does_not_exist);
  REQUIRE(
      registry.setInteractionClassSubscription(
          L"declaration-exercise",
          9999,
          *takeOrder,
          true) == InteractionClassDeclarationStatus::federate_not_member);
  REQUIRE(
      registry.setInteractionClassPublication(
          L"declaration-exercise",
          publisher.membership->id,
          0,
          true) == InteractionClassDeclarationStatus::interaction_class_not_defined);

  REQUIRE(
      registry.setInteractionClassPublication(
          L"declaration-exercise",
          publisher.membership->id,
          *takeOrder,
          true) == InteractionClassDeclarationStatus::applied);
  publisherState = registry.interactionClassDeclarationFor(
      L"declaration-exercise",
      publisher.membership->id,
      *takeOrder);
  REQUIRE(publisherState);
  REQUIRE(publisherState->published);
  REQUIRE_FALSE(publisherState->subscriptionActive.has_value());

  auto subscriberState = registry.interactionClassDeclarationFor(
      L"declaration-exercise",
      subscriber.membership->id,
      *takeOrder);
  REQUIRE(subscriberState);
  REQUIRE_FALSE(subscriberState->published);
  REQUIRE_FALSE(subscriberState->subscriptionActive.has_value());

  REQUIRE(
      registry.setInteractionClassSubscription(
          L"declaration-exercise",
          subscriber.membership->id,
          *takeOrder,
          false) == InteractionClassDeclarationStatus::applied);
  subscriberState = registry.interactionClassDeclarationFor(
      L"declaration-exercise",
      subscriber.membership->id,
      *takeOrder);
  REQUIRE(subscriberState);
  REQUIRE_FALSE(subscriberState->published);
  REQUIRE(subscriberState->subscriptionActive == std::optional<bool>{false});

  REQUIRE(
      registry.setInteractionClassSubscription(
          L"declaration-exercise",
          subscriber.membership->id,
          *takeOrder,
          true) == InteractionClassDeclarationStatus::applied);
  subscriberState = registry.interactionClassDeclarationFor(
      L"declaration-exercise",
      subscriber.membership->id,
      *takeOrder);
  REQUIRE(subscriberState);
  REQUIRE(subscriberState->subscriptionActive == std::optional<bool>{true});

  REQUIRE(
      registry.setInteractionClassPublication(
          L"declaration-exercise",
          publisher.membership->id,
          *takeOrder,
          false) == InteractionClassDeclarationStatus::applied);
  publisherState = registry.interactionClassDeclarationFor(
      L"declaration-exercise",
      publisher.membership->id,
      *takeOrder);
  REQUIRE(publisherState);
  REQUIRE_FALSE(publisherState->published);
  REQUIRE_FALSE(publisherState->subscriptionActive.has_value());

  REQUIRE(
      registry.setInteractionClassSubscription(
          L"declaration-exercise",
          subscriber.membership->id,
          *takeOrder,
          std::nullopt) == InteractionClassDeclarationStatus::applied);
  subscriberState = registry.interactionClassDeclarationFor(
      L"declaration-exercise",
      subscriber.membership->id,
      *takeOrder);
  REQUIRE(subscriberState);
  REQUIRE_FALSE(subscriberState->published);
  REQUIRE_FALSE(subscriberState->subscriptionActive.has_value());

  REQUIRE(
      registry.resign(L"declaration-exercise", publisher.membership->id).status ==
      FederationRegistryStatus::applied);
  REQUIRE_FALSE(
      registry.interactionClassDeclarationFor(
          L"declaration-exercise",
          publisher.membership->id,
          *takeOrder)
          .has_value());
}

TEST_CASE(
    "The federation registry retains 2025 object class attribute declarations independently",
    "[unit][kernel][federation-registry][declaration-management][object-management]") {

  umbra::detail::EmbeddedFederationRegistry registry;
  std::wstring const federationName = L"object-attribute-declaration-exercise";
  REQUIRE(
      registry.create(federationName, composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);

  auto publisher = registry.join(federationName, L"publisher", L"publisher");
  auto subscriber = registry.join(federationName, L"subscriber", L"subscriber");
  REQUIRE(publisher.membership);
  REQUIRE(subscriber.membership);

  std::string const employee = "HLAobjectRoot.Employee";
  std::string const server = "HLAobjectRoot.Employee.Server";
  auto const employeeHandle = registry.objectClassHandleFor(federationName, employee);
  auto const serverHandle = registry.objectClassHandleFor(federationName, server);
  auto const employeeName = registry.attributeHandleFor(federationName, employee, "Name");
  auto const serverName = registry.attributeHandleFor(federationName, server, "Name");
  auto const efficiency = registry.attributeHandleFor(federationName, server, "Efficiency");
  REQUIRE(employeeHandle);
  REQUIRE(serverHandle);
  REQUIRE(employeeName);
  REQUIRE(serverName);
  REQUIRE(efficiency);
  REQUIRE(*serverName == *employeeName);

  auto publisherEmployee = registry.objectClassAttributeDeclarationFor(
      federationName,
      publisher.membership->id,
      *employeeHandle);
  REQUIRE(publisherEmployee);
  REQUIRE(publisherEmployee->explicitlyPublishedAttributes.empty());
  REQUIRE(publisherEmployee->subscribedAttributes.empty());

  REQUIRE(
      registry.setObjectClassAttributePublication(
          L"missing",
          publisher.membership->id,
          *employeeHandle,
          {*employeeName},
          true) == ObjectClassAttributeDeclarationStatus::federation_does_not_exist);
  REQUIRE(
      registry.setObjectClassAttributeSubscription(
          federationName,
          9999,
          *employeeHandle,
          {*employeeName},
          true) == ObjectClassAttributeDeclarationStatus::federate_not_member);
  REQUIRE(
      registry.setObjectClassAttributePublication(
          federationName,
          publisher.membership->id,
          0,
          {*employeeName},
          true) == ObjectClassAttributeDeclarationStatus::object_class_not_defined);
  REQUIRE(
      registry.setObjectClassAttributePublication(
          federationName,
          publisher.membership->id,
          *employeeHandle,
          {*employeeName, *efficiency},
          true) == ObjectClassAttributeDeclarationStatus::attribute_not_defined);

  publisherEmployee = registry.objectClassAttributeDeclarationFor(
      federationName,
      publisher.membership->id,
      *employeeHandle);
  REQUIRE(publisherEmployee);
  REQUIRE(publisherEmployee->explicitlyPublishedAttributes.empty());

  REQUIRE(
      registry.setObjectClassAttributePublication(
          federationName,
          publisher.membership->id,
          *employeeHandle,
          {*employeeName},
          true) == ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(
      registry.setObjectClassAttributePublication(
          federationName,
          publisher.membership->id,
          *serverHandle,
          {*efficiency},
          true) == ObjectClassAttributeDeclarationStatus::applied);

  publisherEmployee = registry.objectClassAttributeDeclarationFor(
      federationName,
      publisher.membership->id,
      *employeeHandle);
  REQUIRE(publisherEmployee);
  REQUIRE(publisherEmployee->explicitlyPublishedAttributes == std::set<std::uint64_t>{*employeeName});
  auto publisherServer = registry.objectClassAttributeDeclarationFor(
      federationName,
      publisher.membership->id,
      *serverHandle);
  REQUIRE(publisherServer);
  REQUIRE(publisherServer->explicitlyPublishedAttributes == std::set<std::uint64_t>{*efficiency});

  REQUIRE(
      registry.setObjectClassAttributeSubscription(
          federationName,
          subscriber.membership->id,
          *serverHandle,
          {*serverName},
          false) == ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(
      registry.setObjectClassAttributeSubscription(
          federationName,
          subscriber.membership->id,
          *serverHandle,
          {*efficiency},
          true) == ObjectClassAttributeDeclarationStatus::applied);
  auto subscriberServer = registry.objectClassAttributeDeclarationFor(
      federationName,
      subscriber.membership->id,
      *serverHandle);
  REQUIRE(subscriberServer);
  REQUIRE(subscriberServer->explicitlyPublishedAttributes.empty());
  REQUIRE(subscriberServer->subscribedAttributes.size() == 2);
  REQUIRE(subscriberServer->subscribedAttributes.at(*serverName) == false);
  REQUIRE(subscriberServer->subscribedAttributes.at(*efficiency) == true);
  auto subscriberEmployee = registry.objectClassAttributeDeclarationFor(
      federationName,
      subscriber.membership->id,
      *employeeHandle);
  REQUIRE(subscriberEmployee);
  REQUIRE(subscriberEmployee->subscribedAttributes.empty());

  REQUIRE(
      registry.setObjectClassAttributeSubscription(
          federationName,
          subscriber.membership->id,
          *serverHandle,
          {*serverName},
          std::nullopt) == ObjectClassAttributeDeclarationStatus::applied);
  subscriberServer = registry.objectClassAttributeDeclarationFor(
      federationName,
      subscriber.membership->id,
      *serverHandle);
  REQUIRE(subscriberServer);
  REQUIRE(subscriberServer->subscribedAttributes.size() == 1);
  REQUIRE(subscriberServer->subscribedAttributes.contains(*efficiency));

  REQUIRE(
      registry.setObjectClassAttributePublication(
          federationName,
          publisher.membership->id,
          *employeeHandle,
          {*employeeName},
          false) == ObjectClassAttributeDeclarationStatus::applied);
  publisherEmployee = registry.objectClassAttributeDeclarationFor(
      federationName,
      publisher.membership->id,
      *employeeHandle);
  REQUIRE(publisherEmployee);
  REQUIRE(publisherEmployee->explicitlyPublishedAttributes.empty());
  publisherServer = registry.objectClassAttributeDeclarationFor(
      federationName,
      publisher.membership->id,
      *serverHandle);
  REQUIRE(publisherServer);
  REQUIRE(publisherServer->explicitlyPublishedAttributes == std::set<std::uint64_t>{*efficiency});

  REQUIRE(
      registry.resign(federationName, publisher.membership->id).status ==
      FederationRegistryStatus::applied);
  REQUIRE_FALSE(
      registry.objectClassAttributeDeclarationFor(
          federationName,
          publisher.membership->id,
          *employeeHandle)
          .has_value());
}

TEST_CASE(
    "The federation registry rejects resignation route gaps atomically",
    "[unit][kernel][federation-registry][federation-management][resign][ownership][atomicity]") {
  umbra::detail::EmbeddedFederationRegistry registry;
  std::wstring const federationName = L"resign-route-gap-atomicity-exercise";
  REQUIRE(
      registry.create(federationName, composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);

  auto source = registry.join(federationName, L"source", L"source");
  auto routeTime = std::make_shared<FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  auto const callback = [](umbra::detail::FederateCallbackInvocation) {};
  auto routedRecipient = registry.joinWithTimeState(
      federationName,
      std::move(routeTime),
      L"routed-recipient",
      L"routed-recipient",
      callback);
  // The legacy registry join intentionally has no callback route.  It lets
  // this private-kernel regression exercise the same impossible delivery
  // precondition that a failed adapter route would expose.
  auto unroutedRecipient = registry.join(
      federationName,
      L"unrouted-recipient",
      L"unrouted-recipient");
  REQUIRE(source.membership);
  REQUIRE(routedRecipient.membership);
  REQUIRE(unroutedRecipient.membership);

  auto const serverHandle = registry.objectClassHandleFor(
      federationName,
      "HLAobjectRoot.Employee.Server");
  auto const nameHandle = registry.attributeHandleFor(
      federationName,
      "HLAobjectRoot.Employee.Server",
      "Name");
  auto const efficiencyHandle = registry.attributeHandleFor(
      federationName,
      "HLAobjectRoot.Employee.Server",
      "Efficiency");
  auto const privilegeHandle = registry.attributeHandleFor(
      federationName,
      "HLAobjectRoot.Employee.Server",
      "HLAprivilegeToDeleteObject");
  REQUIRE(serverHandle);
  REQUIRE(nameHandle);
  REQUIRE(efficiencyHandle);
  REQUIRE(privilegeHandle);

  // Make the lower-handle attribute eligible only for the routed member and
  // the higher-handle attribute eligible only for the member without a route.
  // The production implementation must discover the missing route before it
  // clears the lower attribute's ownership.
  auto const lowerAttribute = std::min(*nameHandle, *efficiencyHandle);
  auto const higherAttribute = std::max(*nameHandle, *efficiencyHandle);
  std::set<std::uint64_t> const sourceAttributes{lowerAttribute, higherAttribute};
  REQUIRE(
      registry.setObjectClassAttributePublication(
          federationName,
          source.membership->id,
          *serverHandle,
          sourceAttributes,
          true) == ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(
      registry.setObjectClassAttributePublication(
          federationName,
          source.membership->id,
          *serverHandle,
          {*privilegeHandle},
          false) == ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(
      registry.setObjectClassAttributePublication(
          federationName,
          routedRecipient.membership->id,
          *serverHandle,
          {lowerAttribute},
          true) == ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(
      registry.setObjectClassAttributePublication(
          federationName,
          routedRecipient.membership->id,
          *serverHandle,
          {*privilegeHandle},
          false) == ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(
      registry.setObjectClassAttributePublication(
          federationName,
          unroutedRecipient.membership->id,
          *serverHandle,
          {higherAttribute},
          true) == ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(
      registry.setObjectClassAttributePublication(
          federationName,
          unroutedRecipient.membership->id,
          *serverHandle,
          {*privilegeHandle},
          false) == ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(
      registry.setObjectClassAttributeSubscription(
          federationName,
          routedRecipient.membership->id,
          *serverHandle,
          {lowerAttribute},
          true) == ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(
      registry.setObjectClassAttributeSubscription(
          federationName,
          unroutedRecipient.membership->id,
          *serverHandle,
          {higherAttribute},
          true) == ObjectClassAttributeDeclarationStatus::applied);

  auto const registration = registry.registerObjectInstance(
      federationName,
      source.membership->id,
      *serverHandle);
  REQUIRE(registration.status == ObjectInstanceRegistrationStatus::applied);
  REQUIRE(
      registry.beginObjectInstanceDiscovery(
          federationName,
          routedRecipient.membership->id,
          registration.objectInstanceHandle));
  REQUIRE(
      registry.beginObjectInstanceDiscovery(
          federationName,
          unroutedRecipient.membership->id,
          registration.objectInstanceHandle));

  auto const lowerBefore = registry.attributeOwnedByFederate(
      federationName,
      source.membership->id,
      registration.objectInstanceHandle,
      lowerAttribute);
  auto const higherBefore = registry.attributeOwnedByFederate(
      federationName,
      source.membership->id,
      registration.objectInstanceHandle,
      higherAttribute);
  REQUIRE(
      lowerBefore.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE(
      higherBefore.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE(lowerBefore.ownedByRequestingFederate);
  REQUIRE(higherBefore.ownedByRequestingFederate);

  auto const result = registry.resign(
      federationName,
      source.membership->id,
      rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES);
  REQUIRE(result.status == FederationRegistryStatus::invalid_request);

  // No part of the source's ownership or membership transaction may have
  // committed when the later eligible recipient lacked a delivery route.
  auto const lowerAfter = registry.attributeOwnedByFederate(
      federationName,
      source.membership->id,
      registration.objectInstanceHandle,
      lowerAttribute);
  auto const higherAfter = registry.attributeOwnedByFederate(
      federationName,
      source.membership->id,
      registration.objectInstanceHandle,
      higherAttribute);
  REQUIRE(
      lowerAfter.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE(
      higherAfter.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE(lowerAfter.ownedByRequestingFederate);
  REQUIRE(higherAfter.ownedByRequestingFederate);
  REQUIRE(
      registry.knownObjectInstanceFor(
          federationName,
          source.membership->id,
          registration.objectInstanceHandle));
}

TEST_CASE(
    "The federation registry restores subscription-generation allocation with declaration state",
    "[unit][kernel][federation-registry][declaration-management][save-restore]"
    "[object-class-declaration-state]") {
  umbra::detail::EmbeddedFederationRegistry registry;
  std::wstring const federationName = L"subscription-generation-restore-exercise";
  REQUIRE(
      registry.create(federationName, composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);

  auto publisherTime = std::make_shared<FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  auto subscriberTime = std::make_shared<FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  auto const callback = [](umbra::detail::FederateCallbackInvocation) {};
  auto publisher = registry.joinWithTimeState(
      federationName,
      std::move(publisherTime),
      L"publisher",
      L"publisher",
      callback);
  auto subscriber = registry.joinWithTimeState(
      federationName,
      std::move(subscriberTime),
      L"subscriber",
      L"subscriber",
      callback);
  REQUIRE(publisher.membership);
  REQUIRE(subscriber.membership);

  auto const server = registry.objectClassHandleFor(
      federationName,
      "HLAobjectRoot.Employee.Server");
  auto const efficiency = registry.attributeHandleFor(
      federationName,
      "HLAobjectRoot.Employee.Server",
      "Efficiency");
  REQUIRE(server);
  REQUIRE(efficiency);
  REQUIRE(
      registry.setObjectClassAttributeSubscription(
          federationName,
          subscriber.membership->id,
          *server,
          {*efficiency},
          true) == ObjectClassAttributeDeclarationStatus::applied);

  auto savedDeclaration = registry.objectClassAttributeDeclarationFor(
      federationName,
      subscriber.membership->id,
      *server);
  REQUIRE(savedDeclaration);
  REQUIRE(savedDeclaration->subscriptionGeneration != 0U);
  auto const savedGeneration = savedDeclaration->subscriptionGeneration;

  REQUIRE(
      registry.requestFederationSave(
          federationName,
          publisher.membership->id,
          L"subscription-generation-baseline")
          .status == umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(
      registry.federateSaveBegun(federationName, publisher.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(
      registry.federateSaveBegun(federationName, subscriber.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(
      registry.federateSaveComplete(federationName, publisher.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  auto const saveComplete = registry.federateSaveComplete(
      federationName,
      subscriber.membership->id);
  REQUIRE(saveComplete.status == umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(saveComplete.saveCompletedSuccessfully);

  // This post-save mutation advances the live allocator beyond the saved
  // boundary. Restore must rewind both the declaration and its next-id state.
  REQUIRE(
      registry.setObjectClassAttributeSubscription(
          federationName,
          subscriber.membership->id,
          *server,
          {*efficiency},
          false) == ObjectClassAttributeDeclarationStatus::applied);

  REQUIRE(
      registry.requestFederationRestore(
          federationName,
          publisher.membership->id,
          L"subscription-generation-baseline")
          .status == umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(
      registry.federateRestoreComplete(federationName, publisher.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto const restoreComplete = registry.federateRestoreComplete(
      federationName,
      subscriber.membership->id);
  REQUIRE(restoreComplete.status == umbra::detail::FederationRestoreControlStatus::applied);

  auto restoredDeclaration = registry.objectClassAttributeDeclarationFor(
      federationName,
      subscriber.membership->id,
      *server);
  REQUIRE(restoredDeclaration);
  REQUIRE(restoredDeclaration->subscribedAttributes.at(*efficiency));
  REQUIRE(restoredDeclaration->subscriptionGeneration == savedGeneration);

  // Reapplying the same declaration is a real mutation and must consume the
  // next saved generation, not the post-save generation that was discarded.
  REQUIRE(
      registry.setObjectClassAttributeSubscription(
          federationName,
          subscriber.membership->id,
          *server,
          {*efficiency},
          true) == ObjectClassAttributeDeclarationStatus::applied);
  auto afterRestoreMutation = registry.objectClassAttributeDeclarationFor(
      federationName,
      subscriber.membership->id,
      *server);
  REQUIRE(afterRestoreMutation);
  REQUIRE(afterRestoreMutation->subscriptionGeneration == savedGeneration + 1U);
}

TEST_CASE(
    "The federation registry allocates 2025 object identities only from current publication",
    "[unit][kernel][federation-registry][object-management]") {
  umbra::detail::EmbeddedFederationRegistry registry;
  std::wstring const federationName = L"object-instance-registration-exercise";
  REQUIRE(
      registry.create(federationName, composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);

  auto publisher = registry.join(federationName, L"publisher", L"publisher");
  auto observer = registry.join(federationName, L"observer", L"observer");
  REQUIRE(publisher.membership);
  REQUIRE(observer.membership);

  std::string const server = "HLAobjectRoot.Employee.Server";
  auto const serverHandle = registry.objectClassHandleFor(federationName, server);
  auto const efficiency = registry.attributeHandleFor(federationName, server, "Efficiency");
  REQUIRE(serverHandle);
  REQUIRE(efficiency);

  auto undefined = registry.registerObjectInstance(
      federationName,
      publisher.membership->id,
      0);
  REQUIRE(undefined.status == ObjectInstanceRegistrationStatus::object_class_not_defined);
  auto unpublished = registry.registerObjectInstance(
      federationName,
      publisher.membership->id,
      *serverHandle);
  REQUIRE(unpublished.status == ObjectInstanceRegistrationStatus::object_class_not_published);

  REQUIRE(
      registry.setObjectClassAttributePublication(
          federationName,
          publisher.membership->id,
          *serverHandle,
          {*efficiency},
          true) == ObjectClassAttributeDeclarationStatus::applied);

  auto first = registry.registerObjectInstance(
      federationName,
      publisher.membership->id,
      *serverHandle);
  auto second = registry.registerObjectInstance(
      federationName,
      publisher.membership->id,
      *serverHandle);
  REQUIRE(first.status == ObjectInstanceRegistrationStatus::applied);
  REQUIRE(second.status == ObjectInstanceRegistrationStatus::applied);
  REQUIRE(first.objectInstanceHandle != 0);
  REQUIRE(second.objectInstanceHandle != 0);
  REQUIRE(first.objectInstanceHandle != second.objectInstanceHandle);
  REQUIRE_FALSE(first.objectInstanceName.empty());
  REQUIRE_FALSE(second.objectInstanceName.empty());
  REQUIRE(first.objectInstanceName != second.objectInstanceName);

  auto publisherKnown = registry.knownObjectInstanceFor(
      federationName,
      publisher.membership->id,
      first.objectInstanceHandle);
  REQUIRE(publisherKnown);
  REQUIRE(publisherKnown->knownObjectClassHandle == *serverHandle);
  REQUIRE(publisherKnown->objectInstanceName == first.objectInstanceName);
  REQUIRE(publisherKnown->producingFederateId == publisher.membership->id);
  auto publisherKnownByName = registry.knownObjectInstanceByNameFor(
      federationName,
      publisher.membership->id,
      first.objectInstanceName);
  REQUIRE(publisherKnownByName);
  REQUIRE(publisherKnownByName->objectInstanceHandle == first.objectInstanceHandle);
  REQUIRE_FALSE(
      registry.knownObjectInstanceFor(
          federationName,
          observer.membership->id,
          first.objectInstanceHandle)
          .has_value());
}

TEST_CASE(
    "The federation registry releases undelivered 2025 discovery reservations",
    "[unit][kernel][federation-registry][object-management][callbacks]") {
  umbra::detail::EmbeddedFederationRegistry registry;
  std::wstring const federationName = L"object-instance-discovery-reservation-exercise";
  REQUIRE(
      registry.create(federationName, composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);

  auto publisher = registry.join(federationName, L"publisher", L"publisher");
  auto observerTimeState = std::make_shared<FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  auto observer = registry.joinWithTimeState(
      federationName,
      observerTimeState,
      L"observer",
      L"observer",
      [](umbra::detail::FederateCallbackInvocation) {});
  REQUIRE(publisher.membership);
  REQUIRE(observer.membership);

  std::string const server = "HLAobjectRoot.Employee.Server";
  auto const serverHandle = registry.objectClassHandleFor(federationName, server);
  auto const efficiency = registry.attributeHandleFor(federationName, server, "Efficiency");
  REQUIRE(serverHandle);
  REQUIRE(efficiency);
  REQUIRE(
      registry.setObjectClassAttributePublication(
          federationName,
          publisher.membership->id,
          *serverHandle,
          {*efficiency},
          true) == ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(
      registry.setObjectClassAttributeSubscription(
          federationName,
          observer.membership->id,
          *serverHandle,
          {*efficiency},
          true) == ObjectClassAttributeDeclarationStatus::applied);

  auto const registration = registry.registerObjectInstance(
      federationName,
      publisher.membership->id,
      *serverHandle);
  REQUIRE(registration.status == ObjectInstanceRegistrationStatus::applied);

  auto firstPlan = registry.planObjectInstanceDiscoveriesForInstance(
      federationName,
      registration.objectInstanceHandle);
  REQUIRE(firstPlan.size() == 1);
  REQUIRE(firstPlan.front().receivingFederateId == observer.membership->id);

  registry.cancelObjectInstanceDiscovery(
      federationName,
      observer.membership->id,
      registration.objectInstanceHandle);
  auto replacementPlan = registry.planObjectInstanceDiscoveriesForInstance(
      federationName,
      registration.objectInstanceHandle);
  REQUIRE(replacementPlan.size() == 1);
  REQUIRE(replacementPlan.front().receivingFederateId == observer.membership->id);

  auto const discovery = registry.beginObjectInstanceDiscovery(
      federationName,
      observer.membership->id,
      registration.objectInstanceHandle);
  REQUIRE(discovery);
  REQUIRE(discovery->knownObjectClassHandle == *serverHandle);
  REQUIRE(
      registry.planObjectInstanceDiscoveriesForInstance(
          federationName,
          registration.objectInstanceHandle)
          .empty());
}

TEST_CASE(
    "The federation registry plans 2025 interaction promotion and parameter projection",
    "[unit][kernel][federation-registry][interaction-routing][interaction-management]") {

  umbra::detail::EmbeddedFederationRegistry registry;
  std::wstring const federationName = L"interaction-routing-exercise";
  REQUIRE(
      registry.create(federationName, composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);

  auto callbackRoute = [](umbra::detail::FederateCallbackInvocation) {};
  auto publisher = registry.join(
      federationName,
      L"publisher",
      L"publisher",
      callbackRoute);
  auto exactSubscriber = registry.join(
      federationName,
      L"subscriber",
      L"exact",
      callbackRoute);
  auto promotedSubscriber = registry.join(
      federationName,
      L"subscriber",
      L"promoted",
      callbackRoute);
  auto ancestorSubscriber = registry.join(
      federationName,
      L"subscriber",
      L"ancestor",
      callbackRoute);
  auto unrelatedSubscriber = registry.join(federationName, L"subscriber", L"unrelated");
  auto routeLessSubscriber = registry.join(federationName, L"subscriber", L"route-less");
  REQUIRE(publisher.membership);
  REQUIRE(exactSubscriber.membership);
  REQUIRE(promotedSubscriber.membership);
  REQUIRE(ancestorSubscriber.membership);
  REQUIRE(unrelatedSubscriber.membership);
  REQUIRE(routeLessSubscriber.membership);

  std::string const mainCourse =
      "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed";
  std::string const foodServed =
      "HLAinteractionRoot.CustomerTransactions.FoodServed";
  std::string const customerTransactions = "HLAinteractionRoot.CustomerTransactions";
  std::string const unrelated = "HLAinteractionRoot.ServerAction.TakeOrder";
  auto const mainCourseHandle = registry.interactionClassHandleFor(federationName, mainCourse);
  auto const foodServedHandle = registry.interactionClassHandleFor(federationName, foodServed);
  auto const customerTransactionsHandle = registry.interactionClassHandleFor(
      federationName,
      customerTransactions);
  auto const unrelatedHandle = registry.interactionClassHandleFor(federationName, unrelated);
  auto const temperatureOk = registry.parameterHandleFor(
      federationName,
      mainCourse,
      "TemperatureOk");
  auto const accuracyOk = registry.parameterHandleFor(federationName, mainCourse, "AccuracyOk");
  auto const timelinessOk = registry.parameterHandleFor(federationName, mainCourse, "TimelinessOk");
  REQUIRE(mainCourseHandle);
  REQUIRE(foodServedHandle);
  REQUIRE(customerTransactionsHandle);
  REQUIRE(unrelatedHandle);
  REQUIRE(temperatureOk);
  REQUIRE(accuracyOk);
  REQUIRE(timelinessOk);

  std::vector<std::uint64_t> const sentParameters{
      *temperatureOk,
      *accuracyOk,
      *timelinessOk,
  };

  auto unpublished = registry.planReceiveOrderInteraction(
      federationName,
      publisher.membership->id,
      *mainCourseHandle,
      sentParameters);
  REQUIRE(unpublished.status == ReceiveOrderInteractionStatus::interaction_class_not_published);

  REQUIRE(
      registry.setInteractionClassPublication(
          federationName,
          publisher.membership->id,
          *mainCourseHandle,
          true) == InteractionClassDeclarationStatus::applied);
  REQUIRE(
      registry.setInteractionClassSubscription(
          federationName,
          publisher.membership->id,
          *mainCourseHandle,
          true) == InteractionClassDeclarationStatus::applied);
  REQUIRE(
      registry.setInteractionClassSubscription(
          federationName,
          exactSubscriber.membership->id,
          *mainCourseHandle,
          true) == InteractionClassDeclarationStatus::applied);
  // Promotion is the subject here, so the superclass declaration is active.
  // Passive-delivery suppression is covered by the dedicated public regression.
  REQUIRE(
      registry.setInteractionClassSubscription(
          federationName,
          promotedSubscriber.membership->id,
          *foodServedHandle,
          true) == InteractionClassDeclarationStatus::applied);
  REQUIRE(
      registry.setInteractionClassSubscription(
          federationName,
          ancestorSubscriber.membership->id,
          *customerTransactionsHandle,
          true) == InteractionClassDeclarationStatus::applied);
  REQUIRE(
      registry.setInteractionClassSubscription(
          federationName,
          unrelatedSubscriber.membership->id,
          *unrelatedHandle,
          true) == InteractionClassDeclarationStatus::applied);
  REQUIRE(
      registry.setInteractionClassSubscription(
          federationName,
          routeLessSubscriber.membership->id,
          *mainCourseHandle,
          true) == InteractionClassDeclarationStatus::applied);

  auto invalidParameter = registry.planReceiveOrderInteraction(
      federationName,
      publisher.membership->id,
      *mainCourseHandle,
      {*temperatureOk, 9999});
  REQUIRE(invalidParameter.status == ReceiveOrderInteractionStatus::interaction_parameter_not_defined);

  auto plan = registry.planReceiveOrderInteraction(
      federationName,
      publisher.membership->id,
      *mainCourseHandle,
      sentParameters);
  REQUIRE(plan.status == ReceiveOrderInteractionStatus::applied);
  REQUIRE(plan.transportationName == "HLAreliable");
  REQUIRE(plan.recipients.size() == 3);

  std::set<std::uint64_t> const exactParameters{
      *temperatureOk,
      *accuracyOk,
      *timelinessOk,
  };
  bool sawExact = false;
  bool sawPromoted = false;
  bool sawAncestor = false;
  for (auto const& recipient : plan.recipients) {
    REQUIRE(recipient.federateId != publisher.membership->id);
    if (recipient.federateId == exactSubscriber.membership->id) {
      sawExact = true;
      REQUIRE(recipient.receivedInteractionClassHandle == *mainCourseHandle);
      REQUIRE(recipient.receivedParameterHandles == exactParameters);
    } else if (recipient.federateId == promotedSubscriber.membership->id) {
      sawPromoted = true;
      REQUIRE(recipient.receivedInteractionClassHandle == *foodServedHandle);
      REQUIRE(recipient.receivedParameterHandles.empty());
    } else if (recipient.federateId == ancestorSubscriber.membership->id) {
      sawAncestor = true;
      REQUIRE(recipient.receivedInteractionClassHandle == *customerTransactionsHandle);
      REQUIRE(recipient.receivedParameterHandles.empty());
    } else {
      FAIL("The routing plan selected an unrelated interaction subscription.");
    }
  }
  REQUIRE(sawExact);
  REQUIRE(sawPromoted);
  REQUIRE(sawAncestor);
  REQUIRE_FALSE(
      registry.receiveOrderInteractionRecipientFor(
          federationName,
          publisher.membership->id,
          routeLessSubscriber.membership->id,
          *mainCourseHandle,
          sentParameters)
          .has_value());

  auto deliveryProjection = registry.receiveOrderInteractionRecipientFor(
      federationName,
      publisher.membership->id,
      promotedSubscriber.membership->id,
      *mainCourseHandle,
      sentParameters);
  REQUIRE(deliveryProjection);
  REQUIRE(deliveryProjection->receivedInteractionClassHandle == *foodServedHandle);
  REQUIRE(deliveryProjection->receivedParameterHandles.empty());

  REQUIRE(
      registry.setInteractionClassSubscription(
          federationName,
          promotedSubscriber.membership->id,
          *foodServedHandle,
          std::nullopt) == InteractionClassDeclarationStatus::applied);
  REQUIRE_FALSE(
      registry.receiveOrderInteractionRecipientFor(
          federationName,
          publisher.membership->id,
          promotedSubscriber.membership->id,
          *mainCourseHandle,
          sentParameters)
          .has_value());
}
