#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAvariableArray.h>

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_transport.hpp"
#include "internal/federation/process_transport_session.hpp"
#include "ieee1516_2025_connection_test_support.hpp"
#include "process_public_service_fixture.hpp"
#include "internal/fom/libxml2_fom_composer.hpp"
#include "internal/fom/libxml2_fom_validator.hpp"
#include "internal/handles/attribute_handle.hpp"
#include "internal/handles/object_instance_handle.hpp"
#endif

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The attribute-ownership query test requires the Umbra source directory."
#endif

namespace {

namespace fixture_hla = umbra::test::hla::wide;
namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::HLA_IMMEDIATE;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::ParameterHandle;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
using umbra::detail::EmbeddedFederationRegistry;
using umbra::detail::FederationDefinition;
using umbra::detail::FomCompositionStatus;
using umbra::detail::FomModuleKind;
using umbra::detail::FomValidationStatus;
using umbra::detail::LibXml2FomModuleComposer;
using umbra::detail::LibXml2FomValidator;
using umbra::detail::PrevalidatedFomModule;
using umbra::detail::ProcessFederationService;
using umbra::detail::ProcessTransportListener;
using umbra::detail::ProcessTransportServiceDispatcher;
using umbra::detail::ProcessTransportSession;
using rti1516_2025::umbra_binding_detail::makeAttributeHandle;
using rti1516_2025::umbra_binding_detail::makeObjectInstanceHandle;
#endif

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"attribute-ownership-query-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / relativePath;
}

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
std::filesystem::path processResourcePath(
    std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / relativePath;
}

PrevalidatedFomModule validatedProcessModule(
    std::filesystem::path const& source,
    FomModuleKind kind,
    std::wstring designator) {
  LibXml2FomValidator validator;
  auto result = validator.validate({
      source,
      processResourcePath("schemas/IEEE1516-DIF-2025.xsd"),
      kind,
      std::move(designator),
      L"IEEE1516-DIF-2025.xsd",
  });
  if (result.status != FomValidationStatus::valid || !result.module) {
    throw std::runtime_error("The process ownership FOM did not validate.");
  }
  return *result.module;
}

FederationDefinition composedProcessOwnershipDefinition() {
  std::vector<PrevalidatedFomModule> modules{
      validatedProcessModule(
          processResourcePath("mim/HLAstandardMIM-2025.xml"),
          FomModuleKind::mim,
          L"urn:umbra:test:process-ownership-mim"),
      validatedProcessModule(
          resourcePath("attribute-update-passel-fom.xml"),
          FomModuleKind::fom,
          L"urn:umbra:test:process-ownership-fom"),
  };
  LibXml2FomModuleComposer composer(
      processResourcePath("schemas/IEEE1516-FDD-2025.xsd"));
  auto result = composer.compose(modules);
  if (result.status != FomCompositionStatus::valid ||
      !result.catalog || !result.fdd) {
    throw std::runtime_error("The process ownership FOM did not compose.");
  }
  return {
      std::move(result.modules),
      L"HLAinteger64Time",
      std::move(result.catalog),
      std::move(result.fdd),
  };
}
#endif

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct ObjectDiscoveryReport final {
    ObjectInstanceHandle objectInstance;
    ObjectClassHandle objectClass;
  };

  struct ObjectRemovalReport final {
    ObjectInstanceHandle objectInstance;
  };

  struct AttributeReflectionReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleValueMap attributeValues;
  };

  struct AttributeOwnershipReport final {
    enum class Kind { federate, unowned, rti };

    Kind kind = Kind::unowned;
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    FederateHandle owner;
  };

  struct AcquisitionNotificationReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  struct AcquisitionUnavailableReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  struct OwnershipReleaseRequestReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
  };

  struct InteractionReport final {
    InteractionClassHandle interactionClass;
    ParameterHandleValueMap parameterValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const& objectClass,
      std::wstring const&,
      FederateHandle const&) override {
    objectDiscoveryReports.push_back({objectInstance, objectClass});
  }

  void removeObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      VariableLengthData const&,
      FederateHandle const&) override {
    objectRemovalReports.push_back({objectInstance});
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const&,
      TransportationTypeHandle const&,
      FederateHandle const&,
      RegionHandleSet const*) override {
    attributeReflectionReports.push_back({objectInstance, attributeValues});
  }

  void informAttributeOwnership(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      FederateHandle const& owner) override {
    attributeOwnershipReports.push_back({
        AttributeOwnershipReport::Kind::federate,
        objectInstance,
        attributes,
        owner,
    });
  }

  void attributeIsNotOwned(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes) override {
    attributeOwnershipReports.push_back({
        AttributeOwnershipReport::Kind::unowned,
        objectInstance,
        attributes,
        FederateHandle(),
    });
  }

  void attributeIsOwnedByRTI(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes) override {
    attributeOwnershipReports.push_back({
        AttributeOwnershipReport::Kind::rti,
        objectInstance,
        attributes,
        FederateHandle(),
    });
  }

  void attributeOwnershipAcquisitionNotification(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      VariableLengthData const& userSuppliedTag) override {
    acquisitionNotifications.push_back({
        objectInstance,
        attributes,
        userSuppliedTag,
    });
  }

  void attributeOwnershipUnavailable(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      VariableLengthData const& userSuppliedTag) override {
    acquisitionUnavailableReports.push_back({
        objectInstance,
        attributes,
        userSuppliedTag,
    });
  }

  void requestAttributeOwnershipRelease(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& candidateAttributes,
      VariableLengthData const&) override {
    ownershipReleaseRequests.push_back({objectInstance, candidateAttributes});
  }

  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      RegionHandleSet const*) override {
    interactionReports.push_back({
        interactionClass,
        parameterValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
    });
  }

  std::vector<ObjectDiscoveryReport> objectDiscoveryReports;
  std::vector<ObjectRemovalReport> objectRemovalReports;
  std::vector<AttributeReflectionReport> attributeReflectionReports;
  std::vector<AttributeOwnershipReport> attributeOwnershipReports;
  std::vector<AcquisitionNotificationReport> acquisitionNotifications;
  std::vector<AcquisitionUnavailableReport> acquisitionUnavailableReports;
  std::vector<OwnershipReleaseRequestReport> ownershipReleaseRequests;
  std::vector<InteractionReport> interactionReports;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Standalone Query Attribute Ownership groups 2025 owner and unowned results",
    "[integration][development-profile][ownership-management]"
    "[query-attribute-ownership][standalone]"
    "[rti.service.query-attribute-ownership]"
    "[federate.callback.inform-attribute-ownership]"
    "[federate.callback.attribute-is-not-owned]") {
  rti1516_2025::NullFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("attribute-update-passel-fom.xml").wstring();
  ObjectInstanceHandle invalidObjectInstance;
  AttributeHandleSet noAttributes;

  REQUIRE_THROWS_AS(
      unjoined->queryAttributeOwnership(invalidObjectInstance, noAttributes),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->queryAttributeOwnership(invalidObjectInstance, noAttributes),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName, fomModule, L"HLAinteger64Time"));
  FederateHandle ownerHandle;
  REQUIRE_NOTHROW(ownerHandle = owner->joinFederationExecution(
      L"query-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"query-requester", L"subscriber", federationName));

  auto const child = owner->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const reliableBaseA =
      owner->getAttributeHandle(child, fixture_hla::fixture::reliable_base_a);
  auto const reliableChild =
      owner->getAttributeHandle(child, fixture_hla::fixture::reliable_child);
  auto const unownedChild =
      owner->getAttributeHandle(child, fixture_hla::fixture::unowned_child);
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(reliableChild.isValid());
  REQUIRE(unownedChild.isValid());

  AttributeHandleSet const ownedAttributes{reliableBaseA, reliableChild};
  AttributeHandleSet const requesterSubscriptions{
      reliableBaseA,
      reliableChild,
      unownedChild,
  };
  AttributeHandleSet const queriedAttributes{reliableBaseA, unownedChild};
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(
      child, requesterSubscriptions));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, ownedAttributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE(requesterReports.objectDiscoveryReports.empty());
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(requester->getKnownObjectClassHandle(objectInstance) == child);

  AttributeHandle invalidAttribute;
  AttributeHandleSet const invalidAttributes{invalidAttribute};
  REQUIRE_THROWS_AS(
      requester->queryAttributeOwnership(invalidObjectInstance, queriedAttributes),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      requester->queryAttributeOwnership(objectInstance, invalidAttributes),
      rti1516_2025::AttributeNotDefined);

  REQUIRE_NOTHROW(
      requester->queryAttributeOwnership(objectInstance, queriedAttributes));
  REQUIRE(requesterReports.attributeOwnershipReports.empty());
  REQUIRE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipReports.size() == 2U);

  auto const federateReport = std::find_if(
      requesterReports.attributeOwnershipReports.begin(),
      requesterReports.attributeOwnershipReports.end(),
      [](ReportingFederateAmbassador::AttributeOwnershipReport const& report) {
        return report.kind ==
            ReportingFederateAmbassador::AttributeOwnershipReport::Kind::federate;
      });
  auto const unownedReport = std::find_if(
      requesterReports.attributeOwnershipReports.begin(),
      requesterReports.attributeOwnershipReports.end(),
      [](ReportingFederateAmbassador::AttributeOwnershipReport const& report) {
        return report.kind ==
            ReportingFederateAmbassador::AttributeOwnershipReport::Kind::unowned;
      });
  REQUIRE(federateReport != requesterReports.attributeOwnershipReports.end());
  REQUIRE(unownedReport != requesterReports.attributeOwnershipReports.end());
  REQUIRE(federateReport->objectInstance == objectInstance);
  REQUIRE(federateReport->attributes == AttributeHandleSet{reliableBaseA});
  REQUIRE(federateReport->owner == ownerHandle);
  REQUIRE(unownedReport->objectInstance == objectInstance);
  REQUIRE(unownedReport->attributes == AttributeHandleSet{unownedChild});
  REQUIRE(std::none_of(
      requesterReports.attributeOwnershipReports.begin(),
      requesterReports.attributeOwnershipReports.end(),
      [](ReportingFederateAmbassador::AttributeOwnershipReport const& report) {
        return report.kind ==
            ReportingFederateAmbassador::AttributeOwnershipReport::Kind::rti;
      }));

  // Remove Object Instance nullifies reports queued by an earlier query.
  VariableLengthData deletionTag;
  REQUIRE_NOTHROW(
      requester->queryAttributeOwnership(objectInstance, queriedAttributes));
  REQUIRE_NOTHROW(owner->deleteObjectInstance(objectInstance, deletionTag));
  REQUIRE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipReports.size() == 2U);
  REQUIRE(requesterReports.objectRemovalReports.size() == 1U);
  REQUIRE(requesterReports.objectRemovalReports.front().objectInstance == objectInstance);

  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(requester->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}

TEST_CASE(
    "Embedded service reporting delivers Query Attribute Ownership through MOM interaction",
    "[integration][development-profile][federation-management][ownership-management]"
    "[mom][service-reporting][service-report-interaction]"
    "[query-attribute-ownership]"
    "[rti.service.query-attribute-ownership]"
    "[rti.service.subscribe-interaction-class]"
    "[federate.callback.receive-interaction]"
    "[federate.callback.inform-attribute-ownership]"
    "[federate.callback.attribute-is-not-owned]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const ownershipFom = resourcePath("attribute-update-passel-fom.xml").wstring();
  auto const switchFom = resourcePath("switch-support-enabled-fom.xml").wstring();
  std::vector<std::wstring> const fomModules{ownershipFom, switchFom};

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_IMMEDIATE));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName, fomModules, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"query-ownership-mom-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"query-ownership-mom-requester", L"subscriber", federationName));

  auto const reportClass = owner->getInteractionClassHandle(
      standard_hla::mom::report_service_invocation);
  REQUIRE(reportClass.isValid());
  std::vector<ParameterHandle> reportParameters;
  for (auto const& name : {
           standard_hla::mom::service,
           standard_hla::mom::service_type,
           standard_hla::mom::success_indicator,
           standard_hla::mom::supplied_arguments,
           standard_hla::mom::returned_argument,
           standard_hla::mom::exception,
           standard_hla::mom::serial_number}) {
    auto const parameter = owner->getParameterHandle(reportClass, name);
    REQUIRE(parameter.isValid());
    reportParameters.push_back(parameter);
  }
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->subscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(requester->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(requester->setSendServiceReportsToFileSwitch(false));

  auto const child = owner->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const ownedAttribute = owner->getAttributeHandle(
      child, fixture_hla::fixture::reliable_base_a);
  auto const unownedAttribute = owner->getAttributeHandle(
      child, fixture_hla::fixture::unowned_child);
  REQUIRE(child.isValid());
  REQUIRE(ownedAttribute.isValid());
  REQUIRE(unownedAttribute.isValid());
  AttributeHandleSet const ownerAttributes{ownedAttribute};
  AttributeHandleSet const requesterSubscriptions{
      ownedAttribute,
      unownedAttribute,
  };
  AttributeHandleSet const queriedAttributes{
      ownedAttribute,
      unownedAttribute,
  };
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(
      child, requesterSubscriptions));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, ownerAttributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);

  // Enable only the requesting federate after setup. This makes the accepted
  // query the first service report and keeps its serial number at zero.
  REQUIRE_NOTHROW(requester->setServiceReportingSwitch(true));
  REQUIRE(requester->getServiceReportingSwitch());
  REQUIRE_FALSE(requester->getSendServiceReportsToFileSwitch());

  REQUIRE_NOTHROW(requester->queryAttributeOwnership(
      objectInstance, queriedAttributes));
  // The HLA_IMMEDIATE observer receives the MOM interaction at the service
  // boundary, before the HLA_EVOKED ownership-result callbacks are drained.
  REQUIRE(ownerReports.interactionReports.size() == 1U);
  REQUIRE(requesterReports.attributeOwnershipReports.empty());

  auto const& report = ownerReports.interactionReports.front();
  REQUIRE(report.interactionClass == reportClass);
  REQUIRE(report.parameterValues.size() == 8U);
  REQUIRE(report.userSuppliedTag.size() == 0U);
  REQUIRE_FALSE(report.producingFederate.isValid());

  rti1516_2025::HLAunicodeString service;
  REQUIRE_NOTHROW(service.decode(report.parameterValues.at(reportParameters[0])));
  REQUIRE(service.get() == L"QueryAttributeOwnership");
  rti1516_2025::HLAinteger16BE serviceType;
  REQUIRE_NOTHROW(serviceType.decode(report.parameterValues.at(reportParameters[1])));
  REQUIRE(serviceType.get() == 3);
  rti1516_2025::HLAboolean success;
  REQUIRE_NOTHROW(success.decode(report.parameterValues.at(reportParameters[2])));
  REQUIRE(success.get());

  rti1516_2025::HLAfixedRecord argumentPrototype;
  argumentPrototype.appendElement(rti1516_2025::HLAinteger32BE{})
      .appendElement(rti1516_2025::HLAunicodeString{})
      .appendElement(rti1516_2025::HLAunicodeString{});
  rti1516_2025::HLAvariableArray suppliedArguments{argumentPrototype};
  REQUIRE_NOTHROW(suppliedArguments.decode(
      report.parameterValues.at(reportParameters[3])));
  REQUIRE(suppliedArguments.size() == 2U);
  auto const& objectArgument = dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
      suppliedArguments.get(0U));
  REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(
              objectArgument.get(0U))
              .get() == 37);
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
              objectArgument.get(1U))
              .get() == L"Object instance designator");
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
              objectArgument.get(2U))
              .get() == L"\"" + objectInstance.toString() + L"\"");

  std::wstring attributeValues = L"[";
  for (auto iterator = queriedAttributes.begin();
       iterator != queriedAttributes.end();
       ++iterator) {
    if (iterator != queriedAttributes.begin()) {
      attributeValues.push_back(L',');
    }
    attributeValues += L"\"" + iterator->toString() + L"\"";
  }
  attributeValues.push_back(L']');
  auto const& setArgument = dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
      suppliedArguments.get(1U));
  REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(setArgument.get(0U))
              .get() == 1);
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(setArgument.get(1U))
              .get() == L"Set of attribute designators");
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(setArgument.get(2U))
              .get() == attributeValues);

  rti1516_2025::HLAfixedRecord returnedArgument;
  returnedArgument.appendElement(rti1516_2025::HLAinteger32BE{})
      .appendElement(rti1516_2025::HLAunicodeString{})
      .appendElement(rti1516_2025::HLAunicodeString{});
  REQUIRE_NOTHROW(returnedArgument.decode(
      report.parameterValues.at(reportParameters[4])));
  REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(
              returnedArgument.get(0U))
              .get() == 34);
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
              returnedArgument.get(1U))
              .get()
              .empty());
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
              returnedArgument.get(2U))
              .get() == L"null");

  rti1516_2025::HLAunicodeString exception;
  REQUIRE_NOTHROW(exception.decode(report.parameterValues.at(reportParameters[5])));
  REQUIRE(exception.get().empty());
  rti1516_2025::HLAinteger32BE serial;
  REQUIRE_NOTHROW(serial.decode(report.parameterValues.at(reportParameters[6])));
  REQUIRE(serial.get() == 0);

  REQUIRE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipReports.size() == 2U);
  REQUIRE(ownerReports.interactionReports.size() == 1U);

  REQUIRE_NOTHROW(owner->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(requester->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
TEST_CASE(
    "RTIambassador carries Is Attribute Owned By Federate through a configured process endpoint",
    "[integration][development-profile][federation-management][ownership-management]"
    "[transport][process-boundary][process-ownership-check][public-endpoint][is-attribute-owned-by-federate]"
    "[rti.service.is-attribute-owned-by-federate][2025]") {
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener != nullptr);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry,
          composedProcessOwnershipDefinition());
      auto connection = listener->accept(
          nullptr,
          {"process-ownership-check-server", 0xA701U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      auto serveExpected = [&](umbra::detail::TransportServiceOperation operation) {
        if (!umbra::test::servePrimaryProcessRequest(
                session, handler,
                [&](umbra::detail::TransportServiceMessage const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(
                        "The process ownership-check server received an unexpected operation.");
                  }
                  return handler(request);
                })) {
          throw std::runtime_error(
              "The process ownership-check server lost a request.");
        }
      };

      using umbra::detail::TransportServiceOperation;
      serveExpected(TransportServiceOperation::create_federation_execution);
      serveExpected(TransportServiceOperation::join_federation_execution);
      serveExpected(TransportServiceOperation::get_object_class_handle);
      serveExpected(TransportServiceOperation::get_attribute_handle);
      serveExpected(TransportServiceOperation::get_attribute_handle);
      serveExpected(TransportServiceOperation::publish_object_class_attributes);
      serveExpected(TransportServiceOperation::register_object_instance);
      serveExpected(TransportServiceOperation::get_known_object_class_handle);
      serveExpected(TransportServiceOperation::is_attribute_owned_by_federate);
      serveExpected(TransportServiceOperation::is_attribute_owned_by_federate);
      serveExpected(TransportServiceOperation::is_attribute_owned_by_federate);
      serveExpected(TransportServiceOperation::is_attribute_owned_by_federate);
      serveExpected(TransportServiceOperation::resign_federation_execution);
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  auto const federationName = nextFederationName() + L"-process";
  auto const fomModule = resourcePath("attribute-update-passel-fom.xml").wstring();
  rti1516_2025::NullFederateAmbassador reports;
  auto rti = makeRti();
  auto configuration = rti1516_2025::RtiConfiguration::createConfiguration()
                           .withConfigurationName(L"process-ownership-check-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(rti->createFederationExecution(
      federationName,
      fomModule));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"process-ownership-check-federate",
      L"publisher",
      federationName));

  auto const child = rti->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const ownedAttribute = rti->getAttributeHandle(
      child,
      fixture_hla::fixture::reliable_base_a);
  auto const unownedAttribute = rti->getAttributeHandle(
      child,
      fixture_hla::fixture::unowned_child);
  REQUIRE(child.isValid());
  REQUIRE(ownedAttribute.isValid());
  REQUIRE(unownedAttribute.isValid());
  REQUIRE_NOTHROW(rti->publishObjectClassAttributes(
      child,
      AttributeHandleSet{ownedAttribute}));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = rti->registerObjectInstance(child));
  REQUIRE(objectInstance.isValid());
  auto const knownObjectClass = rti->getKnownObjectClassHandle(objectInstance);
  if (knownObjectClass != child) {
    throw std::runtime_error(
        "The process known-object class lookup returned the wrong class.");
  }
  REQUIRE_THROWS_AS(
      rti->isAttributeOwnedByFederate(
          makeObjectInstanceHandle(0xDEAD0001U), ownedAttribute),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      rti->isAttributeOwnedByFederate(
          objectInstance, makeAttributeHandle(0xDEAD0002U)),
      rti1516_2025::AttributeNotDefined);
  REQUIRE(rti->isAttributeOwnedByFederate(objectInstance, ownedAttribute));
  REQUIRE_FALSE(rti->isAttributeOwnedByFederate(objectInstance, unownedAttribute));

  REQUIRE_NOTHROW(rti->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(rti->disconnect());
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  REQUIRE_FALSE(serverError);
}

TEST_CASE(
    "RTIambassador carries Query Attribute Ownership through a configured process endpoint",
    "[integration][development-profile][federation-management][ownership-management]"
    "[transport][process-boundary][process-ownership-query][public-endpoint]"
    "[query-attribute-ownership][federate.callback.inform-attribute-ownership]"
    "[federate.callback.attribute-is-not-owned][2025]") {
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener != nullptr);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry,
          composedProcessOwnershipDefinition());
      auto connection = listener->accept(
          nullptr,
          {"process-ownership-query-server", 0xA702U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      auto serveExpected = [&](umbra::detail::TransportServiceOperation operation) {
        if (!umbra::test::servePrimaryProcessRequest(
                session, handler,
                [&](umbra::detail::TransportServiceMessage const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(
                        "The process ownership-query server received an unexpected operation.");
                  }
                  return handler(request);
                })) {
          throw std::runtime_error(
              "The process ownership-query server lost a request.");
        }
      };

      using umbra::detail::TransportServiceOperation;
      serveExpected(TransportServiceOperation::create_federation_execution);
      serveExpected(TransportServiceOperation::join_federation_execution);
      serveExpected(TransportServiceOperation::get_object_class_handle);
      serveExpected(TransportServiceOperation::get_attribute_handle);
      serveExpected(TransportServiceOperation::get_attribute_handle);
      serveExpected(TransportServiceOperation::publish_object_class_attributes);
      serveExpected(TransportServiceOperation::register_object_instance);
      serveExpected(TransportServiceOperation::query_attribute_ownership);
      serveExpected(
          TransportServiceOperation::report_successful_void_service_invocation);
      serveExpected(TransportServiceOperation::receive_interaction);
      serveExpected(TransportServiceOperation::receive_interaction);
      serveExpected(TransportServiceOperation::receive_interaction);
      serveExpected(TransportServiceOperation::resign_federation_execution);
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  auto const federationName = nextFederationName() + L"-process-query";
  auto const fomModule = resourcePath("attribute-update-passel-fom.xml").wstring();
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto configuration = rti1516_2025::RtiConfiguration::createConfiguration()
                           .withConfigurationName(L"process-ownership-query-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(rti->createFederationExecution(
      federationName,
      fomModule));
  FederateHandle federateHandle;
  REQUIRE_NOTHROW(federateHandle = rti->joinFederationExecution(
      L"process-ownership-query-federate",
      L"publisher",
      federationName));

  auto const child = rti->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const ownedAttribute = rti->getAttributeHandle(
      child,
      fixture_hla::fixture::reliable_base_a);
  auto const unownedAttribute = rti->getAttributeHandle(
      child,
      fixture_hla::fixture::unowned_child);
  REQUIRE(child.isValid());
  REQUIRE(ownedAttribute.isValid());
  REQUIRE(unownedAttribute.isValid());
  REQUIRE_NOTHROW(rti->publishObjectClassAttributes(
      child,
      AttributeHandleSet{ownedAttribute}));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = rti->registerObjectInstance(child));
  REQUIRE(objectInstance.isValid());
  REQUIRE_NOTHROW(rti->queryAttributeOwnership(
      objectInstance,
      AttributeHandleSet{ownedAttribute, unownedAttribute}));
  REQUIRE(reports.attributeOwnershipReports.empty());
  REQUIRE_NOTHROW(rti->evokeMultipleCallbacks(0.25, 0.25));
  REQUIRE(reports.attributeOwnershipReports.size() == 2U);

  auto const federateReport = std::find_if(
      reports.attributeOwnershipReports.begin(),
      reports.attributeOwnershipReports.end(),
      [](ReportingFederateAmbassador::AttributeOwnershipReport const& report) {
        return report.kind ==
            ReportingFederateAmbassador::AttributeOwnershipReport::Kind::federate;
      });
  auto const unownedReport = std::find_if(
      reports.attributeOwnershipReports.begin(),
      reports.attributeOwnershipReports.end(),
      [](ReportingFederateAmbassador::AttributeOwnershipReport const& report) {
        return report.kind ==
            ReportingFederateAmbassador::AttributeOwnershipReport::Kind::unowned;
      });
  REQUIRE(federateReport != reports.attributeOwnershipReports.end());
  REQUIRE(unownedReport != reports.attributeOwnershipReports.end());
  REQUIRE(federateReport->objectInstance == objectInstance);
  REQUIRE(federateReport->attributes == AttributeHandleSet{ownedAttribute});
  REQUIRE(federateReport->owner == federateHandle);
  REQUIRE(unownedReport->objectInstance == objectInstance);
  REQUIRE(unownedReport->attributes == AttributeHandleSet{unownedAttribute});

  REQUIRE_NOTHROW(rti->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(rti->disconnect());
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  REQUIRE_FALSE(serverError);
}

TEST_CASE(
    "RTIambassador reports RTI ownership of a joined-federate MOM attribute through a process endpoint",
    "[integration][development-profile][federation-management][mom]"
    "[ownership-management][rti-owned-state][query-attribute-ownership]"
    "[transport][process-boundary][process-rti-owned-mom-ownership-query]"
    "[public-endpoint][2025][rti.service.query-attribute-ownership]"
    "[federate.callback.attribute-is-owned-by-rti]") {
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener != nullptr);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);
  auto const reportDirectory =
      umbra::test::connection_support_2025::temporaryServiceReportDirectory();

  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      umbra::detail::ProcessFederationServiceOptions options;
      options.serviceReportDirectory = reportDirectory;
      ProcessFederationService service(
          registry, composedProcessOwnershipDefinition(), std::move(options));
      auto connection = listener->accept(
          nullptr,
          {"process-rti-owned-query-server", 0xA703U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      bool resigned = false;
      std::size_t requestCount = 0U;
      while (!resigned && requestCount != 96U &&
             umbra::test::servePrimaryProcessRequest(
                 session, handler,
                 [&](umbra::detail::TransportServiceMessage const& request) {
                   using umbra::detail::TransportServiceOperation;
                   switch (request.operation) {
                     case TransportServiceOperation::create_federation_execution:
                     case TransportServiceOperation::join_federation_execution:
                     case TransportServiceOperation::get_object_class_handle:
                     case TransportServiceOperation::get_attribute_handle:
                     case TransportServiceOperation::subscribe_object_class_attributes:
                     case TransportServiceOperation::receive_interaction:
                     case TransportServiceOperation::query_attribute_ownership:
                     case TransportServiceOperation::report_successful_void_service_invocation:
                       break;
                     case TransportServiceOperation::resign_federation_execution:
                       resigned = true;
                       break;
                     default:
                       throw std::runtime_error(
                           "The process RTI-owned query server received an unexpected operation.");
                   }
                   return handler(request);
                 })) {
        ++requestCount;
      }
      if (!resigned) {
        throw std::runtime_error(
            "The process RTI-owned query server did not receive Resign.");
      }
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  auto const federationName = nextFederationName() + L"-process-rti-owned";
  auto const fomModule = resourcePath("attribute-update-passel-fom.xml").wstring();
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto configuration = rti1516_2025::RtiConfiguration::createConfiguration()
                           .withConfigurationName(L"process-rti-owned-query-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(rti->createFederationExecution(federationName, fomModule));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"process-rti-owned-query-federate", L"observer", federationName));

  auto const momClass = rti->getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const reportFileAttribute = rti->getAttributeHandle(
      momClass, L"HLAreportServiceFile");
  REQUIRE(momClass.isValid());
  REQUIRE(reportFileAttribute.isValid());
  REQUIRE_NOTHROW(rti->subscribeObjectClassAttributes(
      momClass, AttributeHandleSet{reportFileAttribute}));
  for (std::size_t pass = 0U;
       pass != 16U &&
       (reports.objectDiscoveryReports.empty() ||
        reports.attributeReflectionReports.empty());
       ++pass) {
    static_cast<void>(rti->evokeCallback(0.25));
  }
  REQUIRE(reports.objectDiscoveryReports.size() == 1U);
  REQUIRE(reports.objectDiscoveryReports.front().objectClass == momClass);
  REQUIRE(reports.attributeReflectionReports.size() == 1U);
  auto const momObject = reports.objectDiscoveryReports.front().objectInstance;
  auto const& reflected = reports.attributeReflectionReports.front();
  REQUIRE(reflected.objectInstance == momObject);
  REQUIRE(reflected.attributeValues.size() == 1U);
  rti1516_2025::HLAunicodeString advertisedPath;
  REQUIRE_NOTHROW(advertisedPath.decode(
      reflected.attributeValues.at(reportFileAttribute)));
  auto const reportPath = std::filesystem::path(advertisedPath.get());
  REQUIRE(reportPath.is_absolute());
  REQUIRE(reportPath.parent_path() ==
          std::filesystem::absolute(reportDirectory).lexically_normal());
  REQUIRE(std::filesystem::exists(reportPath));

  REQUIRE_NOTHROW(rti->queryAttributeOwnership(
      momObject, AttributeHandleSet{reportFileAttribute}));
  REQUIRE(reports.attributeOwnershipReports.empty());
  for (std::size_t pass = 0U;
       pass != 16U && reports.attributeOwnershipReports.empty();
       ++pass) {
    static_cast<void>(rti->evokeCallback(0.25));
  }
  REQUIRE(reports.attributeOwnershipReports.size() == 1U);
  auto const& ownership = reports.attributeOwnershipReports.front();
  REQUIRE(ownership.kind ==
          ReportingFederateAmbassador::AttributeOwnershipReport::Kind::rti);
  REQUIRE(ownership.objectInstance == momObject);
  REQUIRE(ownership.attributes == AttributeHandleSet{reportFileAttribute});
  REQUIRE_FALSE(ownership.owner.isValid());

  REQUIRE_NOTHROW(rti->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(rti->disconnect());
  listener.reset();
  server.join();
  std::error_code cleanupError;
  std::filesystem::remove_all(reportDirectory, cleanupError);
  REQUIRE_FALSE(cleanupError);
  REQUIRE_FALSE(serverError);
}

TEST_CASE(
    "RTIambassador reports a process ownership transition after unconditional divestiture",
    "[integration][development-profile][federation-management][ownership-management]"
    "[transport][process-boundary][process-ownership-transition-query]"
    "[public-endpoint][query-attribute-ownership]"
    "[unconditional-attribute-ownership-divestiture]"
    "[federate.callback.inform-attribute-ownership]"
    "[federate.callback.attribute-is-not-owned][2025]") {
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener != nullptr);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry, composedProcessOwnershipDefinition());
      auto connection = listener->accept(
          nullptr,
          {"process-ownership-transition-server", 0xA705U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      bool resigned = false;
      std::size_t requestCount = 0U;
      while (!resigned && requestCount != 96U &&
             umbra::test::servePrimaryProcessRequest(
                 session, handler,
                 [&](umbra::detail::TransportServiceMessage const& request) {
                   using umbra::detail::TransportServiceOperation;
                   switch (request.operation) {
                     case TransportServiceOperation::create_federation_execution:
                     case TransportServiceOperation::join_federation_execution:
                     case TransportServiceOperation::get_object_class_handle:
                     case TransportServiceOperation::get_attribute_handle:
                     case TransportServiceOperation::publish_object_class_attributes:
                     case TransportServiceOperation::register_object_instance:
                     case TransportServiceOperation::query_attribute_ownership:
                     case TransportServiceOperation::unconditional_attribute_ownership_divestiture:
                     case TransportServiceOperation::report_successful_void_service_invocation:
                     case TransportServiceOperation::receive_interaction:
                       break;
                     case TransportServiceOperation::resign_federation_execution:
                       resigned = true;
                       break;
                     default:
                       throw std::runtime_error(
                           "The process ownership-transition server received an unexpected operation.");
                   }
                   return handler(request);
                 })) {
        ++requestCount;
      }
      if (!resigned) {
        throw std::runtime_error(
            "The process ownership-transition server did not receive Resign.");
      }
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  auto const federationName = nextFederationName() + L"-process-transition";
  auto const fomModule = resourcePath("attribute-update-passel-fom.xml").wstring();
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto configuration = rti1516_2025::RtiConfiguration::createConfiguration()
                           .withConfigurationName(L"process-ownership-transition-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(rti->createFederationExecution(federationName, fomModule));
  FederateHandle owner;
  REQUIRE_NOTHROW(owner = rti->joinFederationExecution(
      L"process-ownership-transition-federate", L"publisher", federationName));
  REQUIRE(owner.isValid());

  auto const child = rti->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const attribute = rti->getAttributeHandle(
      child, fixture_hla::fixture::reliable_base_a);
  REQUIRE(child.isValid());
  REQUIRE(attribute.isValid());
  REQUIRE_NOTHROW(rti->publishObjectClassAttributes(
      child, AttributeHandleSet{attribute}));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = rti->registerObjectInstance(child));
  REQUIRE(objectInstance.isValid());

  auto queryAndEvoke = [&] {
    REQUIRE_NOTHROW(rti->queryAttributeOwnership(
        objectInstance, AttributeHandleSet{attribute}));
    REQUIRE(reports.attributeOwnershipReports.empty());
    for (std::size_t pass = 0U;
         pass != 16U && reports.attributeOwnershipReports.empty(); ++pass) {
      static_cast<void>(rti->evokeCallback(0.25));
    }
    REQUIRE(reports.attributeOwnershipReports.size() == 1U);
    auto const report = reports.attributeOwnershipReports.front();
    reports.attributeOwnershipReports.clear();
    return report;
  };
  auto const before = queryAndEvoke();
  REQUIRE(before.kind ==
          ReportingFederateAmbassador::AttributeOwnershipReport::Kind::federate);
  REQUIRE(before.objectInstance == objectInstance);
  REQUIRE(before.attributes == AttributeHandleSet{attribute});
  REQUIRE(before.owner == owner);

  unsigned char const tagBytes[] = {0x54, 0xA7};
  REQUIRE_NOTHROW(rti->unconditionalAttributeOwnershipDivestiture(
      objectInstance, AttributeHandleSet{attribute},
      VariableLengthData(tagBytes, sizeof(tagBytes))));
  auto const after = queryAndEvoke();
  REQUIRE(after.kind ==
          ReportingFederateAmbassador::AttributeOwnershipReport::Kind::unowned);
  REQUIRE(after.objectInstance == objectInstance);
  REQUIRE(after.attributes == AttributeHandleSet{attribute});
  REQUIRE_FALSE(after.owner.isValid());

  REQUIRE_NOTHROW(rti->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(rti->disconnect());
  listener.reset();
  server.join();
  REQUIRE_FALSE(serverError);
}

TEST_CASE(
    "RTIambassador reports a process owner after another federate acquires a divested attribute",
    "[integration][development-profile][federation-management][ownership-management]"
    "[transport][process-boundary][process-ownership-acquisition-query]"
    "[public-endpoint][query-attribute-ownership]"
    "[unconditional-attribute-ownership-divestiture]"
    "[attribute-ownership-acquisition]"
    "[federate.callback.attribute-ownership-acquisition-notification]"
    "[federate.callback.inform-attribute-ownership]"
    "[federate.callback.attribute-is-not-owned][2025]") {
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener != nullptr);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry, composedProcessOwnershipDefinition());
      auto ownerConnection = listener->accept(
          nullptr,
          {"process-ownership-acquisition-query-server", 0xA70BU},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession ownerSession(ownerConnection);
      auto ownerHandler = service.handlerFor(ownerSession);
      auto serveExpected = [&](ProcessTransportSession& session,
                               auto const& handler,
                               umbra::detail::TransportServiceOperation expected,
                               char const* description) {
        bool matched = false;
        std::size_t requestCount = 0U;
        while (!matched && requestCount != 256U) {
          if (!ProcessTransportServiceDispatcher::serveOne(
                  session,
                  [&](umbra::detail::TransportServiceMessage const& request) {
                    using umbra::detail::TransportServiceOperation;
                    if (request.operation == expected) {
                      matched = true;
                    } else if (
                        request.operation !=
                            TransportServiceOperation::receive_interaction &&
                        request.operation !=
                            TransportServiceOperation::report_successful_void_service_invocation) {
                      throw std::runtime_error(description);
                    }
                    return handler(request);
                  })) {
            throw std::runtime_error(description);
          }
          ++requestCount;
        }
        if (!matched) {
          throw std::runtime_error(description);
        }
      };

      using umbra::detail::TransportServiceOperation;
      serveExpected(
          ownerSession, ownerHandler,
          TransportServiceOperation::create_federation_execution,
          "The process acquisition-query server lost Create.");
      serveExpected(
          ownerSession, ownerHandler,
          TransportServiceOperation::join_federation_execution,
          "The process acquisition-query server lost owner Join.");

      auto acquirerConnection = listener->accept(
          nullptr,
          {"process-ownership-acquisition-query-server", 0xA70CU},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession acquirerSession(acquirerConnection);
      auto acquirerHandler = service.handlerFor(acquirerSession);
      serveExpected(
          acquirerSession, acquirerHandler,
          TransportServiceOperation::join_federation_execution,
          "The process acquisition-query server lost acquirer Join.");
      serveExpected(
          acquirerSession, acquirerHandler,
          TransportServiceOperation::get_object_class_handle,
          "The process acquisition-query server lost acquirer class lookup.");
      serveExpected(
          acquirerSession, acquirerHandler,
          TransportServiceOperation::get_attribute_handle,
          "The process acquisition-query server lost acquirer attribute lookup.");
      serveExpected(
          acquirerSession, acquirerHandler,
          TransportServiceOperation::subscribe_object_class_attributes,
          "The process acquisition-query server lost acquirer Subscribe.");

      serveExpected(
          ownerSession, ownerHandler,
          TransportServiceOperation::get_object_class_handle,
          "The process acquisition-query server lost owner class lookup.");
      serveExpected(
          ownerSession, ownerHandler,
          TransportServiceOperation::get_attribute_handle,
          "The process acquisition-query server lost owner attribute lookup.");
      serveExpected(
          ownerSession, ownerHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The process acquisition-query server lost owner Publish.");
      serveExpected(
          ownerSession, ownerHandler,
          TransportServiceOperation::register_object_instance,
          "The process acquisition-query server lost owner Register.");
      serveExpected(
          acquirerSession, acquirerHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The process acquisition-query server lost acquirer Publish.");
      serveExpected(
          ownerSession, ownerHandler,
          TransportServiceOperation::query_attribute_ownership,
          "The process acquisition-query server lost the initial owner query.");
      serveExpected(
          ownerSession, ownerHandler,
          TransportServiceOperation::unconditional_attribute_ownership_divestiture,
          "The process acquisition-query server lost Divestiture.");
      serveExpected(
          ownerSession, ownerHandler,
          TransportServiceOperation::query_attribute_ownership,
          "The process acquisition-query server lost the unowned query.");
      serveExpected(
          ownerSession, ownerHandler,
          TransportServiceOperation::get_object_class_handle,
          "The process acquisition-query server lost the unowned-query fence.");
      serveExpected(
          acquirerSession, acquirerHandler,
          TransportServiceOperation::attribute_ownership_acquisition,
          "The process acquisition-query server lost Acquisition.");
      serveExpected(
          acquirerSession, acquirerHandler,
          TransportServiceOperation::get_object_class_handle,
          "The process acquisition-query server lost the notification fence.");
      serveExpected(
          ownerSession, ownerHandler,
          TransportServiceOperation::query_attribute_ownership,
          "The process acquisition-query server lost the new-owner query.");
      serveExpected(
          ownerSession, ownerHandler,
          TransportServiceOperation::get_object_class_handle,
          "The process acquisition-query server lost the new-owner query fence.");
      serveExpected(
          acquirerSession, acquirerHandler,
          TransportServiceOperation::resign_federation_execution,
          "The process acquisition-query server lost acquirer Resign.");
      serveExpected(
          ownerSession, ownerHandler,
          TransportServiceOperation::resign_federation_execution,
          "The process acquisition-query server lost owner Resign.");
      service.detach(acquirerSession);
      service.detach(ownerSession);
      acquirerConnection->close();
      ownerConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  auto const federationName = nextFederationName() + L"-process-acquisition-query";
  auto const fomModule = resourcePath("attribute-update-passel-fom.xml").wstring();
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador acquirerReports;
  auto owner = makeRti();
  auto acquirer = makeRti();
  auto ownerConfiguration = rti1516_2025::RtiConfiguration::createConfiguration()
                                .withConfigurationName(L"process-acquisition-query-owner")
                                .withRtiAddress(
                                    L"tcp://127.0.0.1:" + std::to_wstring(port));
  auto acquirerConfiguration = rti1516_2025::RtiConfiguration::createConfiguration()
                                   .withConfigurationName(L"process-acquisition-query-acquirer")
                                   .withRtiAddress(
                                       L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool ownerJoined = false;
  bool acquirerJoined = false;
  try {
    REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED, ownerConfiguration));
    REQUIRE_NOTHROW(owner->createFederationExecution(federationName, fomModule));
    FederateHandle ownerHandle;
    REQUIRE_NOTHROW(ownerHandle = owner->joinFederationExecution(
        L"process-acquisition-query-owner", L"publisher", federationName));
    ownerJoined = true;
    REQUIRE(ownerHandle.isValid());

    REQUIRE_NOTHROW(acquirer->connect(
        acquirerReports, HLA_EVOKED, acquirerConfiguration));
    FederateHandle acquirerHandle;
    REQUIRE_NOTHROW(acquirerHandle = acquirer->joinFederationExecution(
        L"process-acquisition-query-acquirer", L"publisher", federationName));
    acquirerJoined = true;
    REQUIRE(acquirerHandle.isValid());
    REQUIRE(acquirerHandle != ownerHandle);

    auto const acquirerClass = acquirer->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const acquirerAttribute = acquirer->getAttributeHandle(
        acquirerClass, fixture_hla::fixture::reliable_base_a);
    REQUIRE(acquirerClass.isValid());
    REQUIRE(acquirerAttribute.isValid());
    REQUIRE_NOTHROW(acquirer->subscribeObjectClassAttributes(
        acquirerClass, AttributeHandleSet{acquirerAttribute}));

    auto const ownerClass = owner->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const ownerAttribute = owner->getAttributeHandle(
        ownerClass, fixture_hla::fixture::reliable_base_a);
    REQUIRE(ownerClass == acquirerClass);
    REQUIRE(ownerAttribute == acquirerAttribute);
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
        ownerClass, AttributeHandleSet{ownerAttribute}));
    ObjectInstanceHandle objectInstance;
    REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(ownerClass));
    REQUIRE(objectInstance.isValid());
    for (std::size_t pass = 0U;
         pass != 32U && acquirerReports.objectDiscoveryReports.empty(); ++pass) {
      static_cast<void>(acquirer->evokeMultipleCallbacks(0.01, 0.02));
    }
    REQUIRE(acquirerReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(acquirerReports.objectDiscoveryReports.front().objectInstance ==
            objectInstance);
    REQUIRE_NOTHROW(acquirer->publishObjectClassAttributes(
        acquirerClass, AttributeHandleSet{acquirerAttribute}));

    auto queryAndEvoke = [&] {
      REQUIRE_NOTHROW(owner->queryAttributeOwnership(
          objectInstance, AttributeHandleSet{ownerAttribute}));
      REQUIRE(ownerReports.attributeOwnershipReports.empty());
      for (std::size_t pass = 0U;
           pass != 32U && ownerReports.attributeOwnershipReports.empty();
           ++pass) {
        static_cast<void>(owner->evokeMultipleCallbacks(0.01, 0.02));
      }
      REQUIRE(ownerReports.attributeOwnershipReports.size() == 1U);
      auto const report = ownerReports.attributeOwnershipReports.front();
      ownerReports.attributeOwnershipReports.clear();
      REQUIRE(report.objectInstance == objectInstance);
      REQUIRE(report.attributes == AttributeHandleSet{ownerAttribute});
      return report;
    };
    auto const before = queryAndEvoke();
    REQUIRE(before.kind ==
            ReportingFederateAmbassador::AttributeOwnershipReport::Kind::federate);
    REQUIRE(before.owner == ownerHandle);

    unsigned char const divestitureTag[] = {0x54, 0xA7};
    REQUIRE_NOTHROW(owner->unconditionalAttributeOwnershipDivestiture(
        objectInstance, AttributeHandleSet{ownerAttribute},
        VariableLengthData(divestitureTag, sizeof(divestitureTag))));
    auto const unowned = queryAndEvoke();
    REQUIRE(unowned.kind ==
            ReportingFederateAmbassador::AttributeOwnershipReport::Kind::unowned);
    REQUIRE_FALSE(unowned.owner.isValid());
    REQUIRE(owner->getObjectClassHandle(
                fixture_hla::fom::attribute_fixture_child) == ownerClass);

    unsigned char const acquisitionTag[] = {0xA7, 0x0C};
    REQUIRE_NOTHROW(acquirer->attributeOwnershipAcquisition(
        objectInstance, AttributeHandleSet{acquirerAttribute},
        VariableLengthData(acquisitionTag, sizeof(acquisitionTag))));
    REQUIRE(acquirerReports.acquisitionNotifications.empty());
    for (std::size_t pass = 0U;
         pass != 32U && acquirerReports.acquisitionNotifications.empty();
         ++pass) {
      static_cast<void>(acquirer->evokeMultipleCallbacks(0.01, 0.02));
    }
    REQUIRE(acquirerReports.acquisitionNotifications.size() == 1U);
    auto const& notification = acquirerReports.acquisitionNotifications.front();
    REQUIRE(notification.objectInstance == objectInstance);
    REQUIRE(notification.attributes == AttributeHandleSet{acquirerAttribute});
    REQUIRE(notification.userSuppliedTag.size() == sizeof(acquisitionTag));
    REQUIRE(acquirer->getObjectClassHandle(
                fixture_hla::fom::attribute_fixture_child) == acquirerClass);

    auto const acquired = queryAndEvoke();
    REQUIRE(acquired.kind ==
            ReportingFederateAmbassador::AttributeOwnershipReport::Kind::federate);
    REQUIRE(acquired.owner == acquirerHandle);
    REQUIRE(owner->getObjectClassHandle(
                fixture_hla::fom::attribute_fixture_child) == ownerClass);

    REQUIRE_NOTHROW(acquirer->resignFederationExecution(
        rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES));
    acquirerJoined = false;
    REQUIRE_NOTHROW(owner->resignFederationExecution(rti1516_2025::NO_ACTION));
    ownerJoined = false;
    REQUIRE_NOTHROW(acquirer->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
  } catch (...) {
    clientError = std::current_exception();
    if (acquirerJoined) {
      try {
        acquirer->resignFederationExecution(
            rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES);
      } catch (...) {
      }
    }
    if (ownerJoined) {
      try {
        owner->resignFederationExecution(rti1516_2025::NO_ACTION);
      } catch (...) {
      }
    }
    try {
      acquirer->disconnect();
    } catch (...) {
    }
    try {
      owner->disconnect();
    } catch (...) {
    }
  }
  listener.reset();
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
}

TEST_CASE(
    "RTIambassador resolves competing process If Available acquisitions to one queried owner",
    "[integration][development-profile][federation-management][ownership-management]"
    "[transport][process-boundary][process-ownership-competing-if-available-query]"
    "[public-endpoint][query-attribute-ownership]"
    "[unconditional-attribute-ownership-divestiture]"
    "[attribute-ownership-acquisition-if-available]"
    "[federate.callback.attribute-ownership-acquisition-notification]"
    "[federate.callback.attribute-ownership-unavailable]"
    "[federate.callback.inform-attribute-ownership][2025]") {
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener != nullptr);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry, composedProcessOwnershipDefinition());
      auto ownerConnection = listener->accept(
          nullptr, {"process-competing-ownership-server", 0xA70DU},
          [](std::wstring) {}, [](std::wstring) { return false; });
      auto firstConnection = listener->accept(
          nullptr, {"process-competing-ownership-server", 0xA70EU},
          [](std::wstring) {}, [](std::wstring) { return false; });
      auto secondConnection = listener->accept(
          nullptr, {"process-competing-ownership-server", 0xA70FU},
          [](std::wstring) {}, [](std::wstring) { return false; });
      ProcessTransportSession ownerSession(ownerConnection);
      ProcessTransportSession firstSession(firstConnection);
      ProcessTransportSession secondSession(secondConnection);
      auto ownerHandler = service.handlerFor(ownerSession);
      auto firstHandler = service.handlerFor(firstSession);
      auto secondHandler = service.handlerFor(secondSession);

      auto serveJoinedFederate = [&](auto& session, auto const& handler,
                                     auto const& connection,
                                     std::exception_ptr& workerError) {
        try {
          bool resigned = false;
          std::size_t requestCount = 0U;
          while (!resigned && requestCount != 512U &&
                 umbra::test::servePrimaryProcessRequest(
                     session, handler,
                     [&](umbra::detail::TransportServiceMessage const& request) {
                       using umbra::detail::TransportServiceOperation;
                       switch (request.operation) {
                         case TransportServiceOperation::create_federation_execution:
                         case TransportServiceOperation::join_federation_execution:
                         case TransportServiceOperation::get_object_class_handle:
                         case TransportServiceOperation::get_attribute_handle:
                         case TransportServiceOperation::subscribe_object_class_attributes:
                         case TransportServiceOperation::publish_object_class_attributes:
                         case TransportServiceOperation::register_object_instance:
                         case TransportServiceOperation::query_attribute_ownership:
                         case TransportServiceOperation::unconditional_attribute_ownership_divestiture:
                         case TransportServiceOperation::attribute_ownership_acquisition_if_available:
                         case TransportServiceOperation::receive_interaction:
                         case TransportServiceOperation::report_successful_void_service_invocation:
                           break;
                         case TransportServiceOperation::resign_federation_execution:
                           resigned = true;
                           break;
                         default:
                           throw std::runtime_error(
                               "The competing-acquisition server received an unexpected operation.");
                       }
                       return handler(request);
                     })) {
            ++requestCount;
          }
          if (!resigned) {
            throw std::runtime_error(
                "The competing-acquisition server did not receive Resign.");
          }
        } catch (...) {
          workerError = std::current_exception();
        }
        try {
          service.detach(session);
        } catch (...) {
          if (!workerError) workerError = std::current_exception();
        }
        connection->close();
      };

      std::exception_ptr ownerError;
      std::exception_ptr firstError;
      std::exception_ptr secondError;
      std::jthread ownerWorker([&] {
        serveJoinedFederate(
            ownerSession, ownerHandler, ownerConnection, ownerError);
      });
      std::jthread firstWorker([&] {
        serveJoinedFederate(
            firstSession, firstHandler, firstConnection, firstError);
      });
      std::jthread secondWorker([&] {
        serveJoinedFederate(
            secondSession, secondHandler, secondConnection, secondError);
      });
      ownerWorker.join();
      firstWorker.join();
      secondWorker.join();
      if (ownerError) std::rethrow_exception(ownerError);
      if (firstError) std::rethrow_exception(firstError);
      if (secondError) std::rethrow_exception(secondError);
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  auto const federationName = nextFederationName() + L"-process-competing";
  auto const fomModule = resourcePath("attribute-update-passel-fom.xml").wstring();
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador firstReports;
  ReportingFederateAmbassador secondReports;
  auto owner = makeRti();
  auto first = makeRti();
  auto second = makeRti();
  auto configurationFor = [port](std::wstring name) {
    return rti1516_2025::RtiConfiguration::createConfiguration()
        .withConfigurationName(std::move(name))
        .withRtiAddress(L"tcp://127.0.0.1:" + std::to_wstring(port));
  };
  bool ownerJoined = false;
  bool firstJoined = false;
  bool secondJoined = false;
  std::exception_ptr clientError;
  try {
    // Connect all three endpoints before the service workers begin. Neither
    // request order nor federate handle is used as a winner assertion.
    REQUIRE_NOTHROW(owner->connect(
        ownerReports, HLA_EVOKED, configurationFor(L"competing-owner")));
    REQUIRE_NOTHROW(first->connect(
        firstReports, HLA_EVOKED, configurationFor(L"competing-first")));
    REQUIRE_NOTHROW(second->connect(
        secondReports, HLA_EVOKED, configurationFor(L"competing-second")));
    REQUIRE_NOTHROW(owner->createFederationExecution(federationName, fomModule));
    FederateHandle ownerHandle;
    FederateHandle firstHandle;
    FederateHandle secondHandle;
    REQUIRE_NOTHROW(ownerHandle = owner->joinFederationExecution(
        L"competing-owner", L"publisher", federationName));
    ownerJoined = true;
    REQUIRE_NOTHROW(firstHandle = first->joinFederationExecution(
        L"competing-first", L"publisher", federationName));
    firstJoined = true;
    REQUIRE_NOTHROW(secondHandle = second->joinFederationExecution(
        L"competing-second", L"publisher", federationName));
    secondJoined = true;
    REQUIRE(ownerHandle.isValid());
    REQUIRE(firstHandle.isValid());
    REQUIRE(secondHandle.isValid());
    REQUIRE(ownerHandle != firstHandle);
    REQUIRE(ownerHandle != secondHandle);
    REQUIRE(firstHandle != secondHandle);

    auto const ownerClass = owner->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const attribute = owner->getAttributeHandle(
        ownerClass, fixture_hla::fixture::reliable_base_a);
    REQUIRE(ownerClass.isValid());
    REQUIRE(attribute.isValid());
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
        ownerClass, AttributeHandleSet{attribute}));
    auto prepareCandidate = [&](auto& candidate) {
      auto const candidateClass = candidate.getObjectClassHandle(
          fixture_hla::fom::attribute_fixture_child);
      auto const candidateAttribute = candidate.getAttributeHandle(
          candidateClass, fixture_hla::fixture::reliable_base_a);
      REQUIRE(candidateClass == ownerClass);
      REQUIRE(candidateAttribute == attribute);
      REQUIRE_NOTHROW(candidate.subscribeObjectClassAttributes(
          candidateClass, AttributeHandleSet{candidateAttribute}));
    };
    prepareCandidate(*first);
    prepareCandidate(*second);

    ObjectInstanceHandle objectInstance;
    REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(ownerClass));
    REQUIRE(objectInstance.isValid());
    auto awaitDiscovery = [&](auto& candidate, auto& reports) {
      auto const deadline =
          std::chrono::steady_clock::now() + std::chrono::seconds(2);
      while (reports.objectDiscoveryReports.empty() &&
             std::chrono::steady_clock::now() < deadline) {
        static_cast<void>(candidate.evokeMultipleCallbacks(0.01, 0.02));
      }
      REQUIRE(reports.objectDiscoveryReports.size() == 1U);
      REQUIRE(reports.objectDiscoveryReports.front().objectInstance ==
              objectInstance);
      REQUIRE_NOTHROW(candidate.publishObjectClassAttributes(
          ownerClass, AttributeHandleSet{attribute}));
    };
    awaitDiscovery(*first, firstReports);
    awaitDiscovery(*second, secondReports);

    unsigned char const divestitureTag[] = {0x54, 0xA7};
    REQUIRE_NOTHROW(owner->unconditionalAttributeOwnershipDivestiture(
        objectInstance, AttributeHandleSet{attribute},
        VariableLengthData(divestitureTag, sizeof(divestitureTag))));
    auto queryOwner = [&] {
      REQUIRE_NOTHROW(owner->queryAttributeOwnership(
          objectInstance, AttributeHandleSet{attribute}));
      auto const deadline =
          std::chrono::steady_clock::now() + std::chrono::seconds(2);
      while (ownerReports.attributeOwnershipReports.empty() &&
             std::chrono::steady_clock::now() < deadline) {
        static_cast<void>(owner->evokeMultipleCallbacks(0.01, 0.02));
      }
      REQUIRE(ownerReports.attributeOwnershipReports.size() == 1U);
      auto result = ownerReports.attributeOwnershipReports.front();
      ownerReports.attributeOwnershipReports.clear();
      REQUIRE(result.objectInstance == objectInstance);
      REQUIRE(result.attributes == AttributeHandleSet{attribute});
      return result;
    };
    auto const unowned = queryOwner();
    REQUIRE(unowned.kind ==
            ReportingFederateAmbassador::AttributeOwnershipReport::Kind::unowned);

    unsigned char const firstTag[] = {0xA7, 0x0E};
    unsigned char const secondTag[] = {0xA7, 0x0F};
    REQUIRE_NOTHROW(first->attributeOwnershipAcquisitionIfAvailable(
        objectInstance, AttributeHandleSet{attribute},
        VariableLengthData(firstTag, sizeof(firstTag))));
    REQUIRE_NOTHROW(second->attributeOwnershipAcquisitionIfAvailable(
        objectInstance, AttributeHandleSet{attribute},
        VariableLengthData(secondTag, sizeof(secondTag))));
    REQUIRE(firstReports.acquisitionNotifications.empty());
    REQUIRE(firstReports.acquisitionUnavailableReports.empty());
    REQUIRE(secondReports.acquisitionNotifications.empty());
    REQUIRE(secondReports.acquisitionUnavailableReports.empty());

    auto awaitTerminal = [&](auto& candidate, auto& reports) {
      auto const deadline =
          std::chrono::steady_clock::now() + std::chrono::seconds(2);
      while (reports.acquisitionNotifications.empty() &&
             reports.acquisitionUnavailableReports.empty() &&
             std::chrono::steady_clock::now() < deadline) {
        static_cast<void>(candidate.evokeMultipleCallbacks(0.01, 0.02));
      }
      REQUIRE(reports.acquisitionNotifications.size() +
                  reports.acquisitionUnavailableReports.size() == 1U);
      if (!reports.acquisitionNotifications.empty()) {
        REQUIRE(reports.acquisitionNotifications.front().objectInstance ==
                objectInstance);
        REQUIRE(reports.acquisitionNotifications.front().attributes ==
                AttributeHandleSet{attribute});
      } else {
        REQUIRE(reports.acquisitionUnavailableReports.front().objectInstance ==
                objectInstance);
        REQUIRE(reports.acquisitionUnavailableReports.front().attributes ==
                AttributeHandleSet{attribute});
      }
    };
    // Evoking the second candidate first exercises callback-time arbitration.
    // The test observes the winner instead of requiring a particular one.
    awaitTerminal(*second, secondReports);
    awaitTerminal(*first, firstReports);
    REQUIRE(firstReports.acquisitionNotifications.size() +
                secondReports.acquisitionNotifications.size() == 1U);
    REQUIRE(firstReports.acquisitionUnavailableReports.size() +
                secondReports.acquisitionUnavailableReports.size() == 1U);
    auto const winner = firstReports.acquisitionNotifications.empty()
                            ? secondHandle
                            : firstHandle;
    auto const acquired = queryOwner();
    REQUIRE(acquired.kind ==
            ReportingFederateAmbassador::AttributeOwnershipReport::Kind::federate);
    REQUIRE(acquired.owner == winner);
    REQUIRE(acquired.owner != ownerHandle);

    REQUIRE_NOTHROW(first->resignFederationExecution(
        rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES));
    firstJoined = false;
    REQUIRE_NOTHROW(second->resignFederationExecution(
        rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES));
    secondJoined = false;
    REQUIRE_NOTHROW(owner->resignFederationExecution(rti1516_2025::NO_ACTION));
    ownerJoined = false;
    REQUIRE_NOTHROW(first->disconnect());
    REQUIRE_NOTHROW(second->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
  } catch (...) {
    clientError = std::current_exception();
    if (secondJoined) {
      try {
        second->resignFederationExecution(
            rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST);
      } catch (...) {
      }
    }
    if (firstJoined) {
      try {
        first->resignFederationExecution(
            rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST);
      } catch (...) {
      }
    }
    if (ownerJoined) {
      try {
        owner->resignFederationExecution(
            rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST);
      } catch (...) {
      }
    }
    for (auto* client : {second.get(), first.get(), owner.get()}) {
      try {
        client->disconnect();
      } catch (...) {
      }
    }
  }
  listener.reset();
  if (server.joinable()) server.join();
  if (clientError) std::rethrow_exception(clientError);
  REQUIRE_FALSE(serverError);
}

TEST_CASE(
    "RTIambassador resolves mixed process If Available attributes per owner",
    "[integration][development-profile][federation-management][ownership-management]"
    "[transport][process-boundary][process-ownership-mixed-if-available-query]"
    "[public-endpoint][query-attribute-ownership]"
    "[unconditional-attribute-ownership-divestiture]"
    "[attribute-ownership-acquisition-if-available]"
    "[federate.callback.attribute-ownership-acquisition-notification]"
    "[federate.callback.attribute-ownership-unavailable]"
    "[federate.callback.inform-attribute-ownership][2025]") {
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener != nullptr);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry, composedProcessOwnershipDefinition());
      auto ownerConnection = listener->accept(
          nullptr, {"process-mixed-ownership-server", 0xA710U},
          [](std::wstring) {}, [](std::wstring) { return false; });
      auto requesterConnection = listener->accept(
          nullptr, {"process-mixed-ownership-server", 0xA711U},
          [](std::wstring) {}, [](std::wstring) { return false; });
      ProcessTransportSession ownerSession(ownerConnection);
      ProcessTransportSession requesterSession(requesterConnection);
      auto ownerHandler = service.handlerFor(ownerSession);
      auto requesterHandler = service.handlerFor(requesterSession);

      auto serveJoinedFederate = [&](auto& session, auto const& handler,
                                     auto const& connection,
                                     std::exception_ptr& workerError) {
        try {
          bool resigned = false;
          std::size_t requestCount = 0U;
          while (!resigned && requestCount != 512U &&
                 umbra::test::servePrimaryProcessRequest(
                     session, handler,
                     [&](umbra::detail::TransportServiceMessage const& request) {
                       using umbra::detail::TransportServiceOperation;
                       switch (request.operation) {
                         case TransportServiceOperation::create_federation_execution:
                         case TransportServiceOperation::join_federation_execution:
                         case TransportServiceOperation::get_object_class_handle:
                         case TransportServiceOperation::get_attribute_handle:
                         case TransportServiceOperation::subscribe_object_class_attributes:
                         case TransportServiceOperation::publish_object_class_attributes:
                         case TransportServiceOperation::register_object_instance:
                         case TransportServiceOperation::query_attribute_ownership:
                         case TransportServiceOperation::unconditional_attribute_ownership_divestiture:
                         case TransportServiceOperation::attribute_ownership_acquisition_if_available:
                         case TransportServiceOperation::receive_interaction:
                         case TransportServiceOperation::report_successful_void_service_invocation:
                           break;
                         case TransportServiceOperation::resign_federation_execution:
                           resigned = true;
                           break;
                         default:
                           throw std::runtime_error(
                               "The mixed-attribute server received an unexpected operation.");
                       }
                       return handler(request);
                     })) {
            ++requestCount;
          }
          if (!resigned) {
            throw std::runtime_error(
                "The mixed-attribute server did not receive Resign.");
          }
        } catch (...) {
          workerError = std::current_exception();
        }
        try {
          service.detach(session);
        } catch (...) {
          if (!workerError) workerError = std::current_exception();
        }
        connection->close();
      };

      std::exception_ptr ownerError;
      std::exception_ptr requesterError;
      std::jthread ownerWorker([&] {
        serveJoinedFederate(
            ownerSession, ownerHandler, ownerConnection, ownerError);
      });
      std::jthread requesterWorker([&] {
        serveJoinedFederate(
            requesterSession, requesterHandler, requesterConnection,
            requesterError);
      });
      ownerWorker.join();
      requesterWorker.join();
      if (ownerError) std::rethrow_exception(ownerError);
      if (requesterError) std::rethrow_exception(requesterError);
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  auto const federationName = nextFederationName() + L"-process-mixed";
  auto const fomModule = resourcePath("attribute-update-passel-fom.xml").wstring();
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto configurationFor = [port](std::wstring name) {
    return rti1516_2025::RtiConfiguration::createConfiguration()
        .withConfigurationName(std::move(name))
        .withRtiAddress(L"tcp://127.0.0.1:" + std::to_wstring(port));
  };
  bool ownerJoined = false;
  bool requesterJoined = false;
  std::exception_ptr clientError;
  try {
    REQUIRE_NOTHROW(owner->connect(
        ownerReports, HLA_EVOKED, configurationFor(L"mixed-owner")));
    REQUIRE_NOTHROW(requester->connect(
        requesterReports, HLA_EVOKED, configurationFor(L"mixed-requester")));
    REQUIRE_NOTHROW(owner->createFederationExecution(federationName, fomModule));
    FederateHandle ownerHandle;
    FederateHandle requesterHandle;
    REQUIRE_NOTHROW(ownerHandle = owner->joinFederationExecution(
        L"mixed-owner", L"publisher", federationName));
    ownerJoined = true;
    REQUIRE_NOTHROW(requesterHandle = requester->joinFederationExecution(
        L"mixed-requester", L"publisher", federationName));
    requesterJoined = true;
    REQUIRE(ownerHandle.isValid());
    REQUIRE(requesterHandle.isValid());
    REQUIRE(ownerHandle != requesterHandle);

    auto const ownerClass = owner->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const freeAttribute = owner->getAttributeHandle(
        ownerClass, fixture_hla::fixture::reliable_base_a);
    auto const heldAttribute = owner->getAttributeHandle(
        ownerClass, fixture_hla::fixture::reliable_base_b);
    REQUIRE(ownerClass.isValid());
    REQUIRE(freeAttribute.isValid());
    REQUIRE(heldAttribute.isValid());
    REQUIRE(freeAttribute != heldAttribute);

    auto const requesterClass = requester->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const requesterFree = requester->getAttributeHandle(
        requesterClass, fixture_hla::fixture::reliable_base_a);
    auto const requesterHeld = requester->getAttributeHandle(
        requesterClass, fixture_hla::fixture::reliable_base_b);
    REQUIRE(requesterClass == ownerClass);
    REQUIRE(requesterFree == freeAttribute);
    REQUIRE(requesterHeld == heldAttribute);
    AttributeHandleSet const both{freeAttribute, heldAttribute};
    REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(
        requesterClass, both));
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(ownerClass, both));
    ObjectInstanceHandle objectInstance;
    REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(ownerClass));
    REQUIRE(objectInstance.isValid());
    auto const discoveryDeadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (requesterReports.objectDiscoveryReports.empty() &&
           std::chrono::steady_clock::now() < discoveryDeadline) {
      static_cast<void>(requester->evokeMultipleCallbacks(0.01, 0.02));
    }
    REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(requesterReports.objectDiscoveryReports.front().objectInstance ==
            objectInstance);
    REQUIRE_NOTHROW(requester->publishObjectClassAttributes(
        requesterClass, both));

    auto queryOne = [&](AttributeHandle const& attribute) {
      REQUIRE_NOTHROW(owner->queryAttributeOwnership(
          objectInstance, AttributeHandleSet{attribute}));
      auto const deadline =
          std::chrono::steady_clock::now() + std::chrono::seconds(2);
      while (ownerReports.attributeOwnershipReports.empty() &&
             std::chrono::steady_clock::now() < deadline) {
        static_cast<void>(owner->evokeMultipleCallbacks(0.01, 0.02));
      }
      REQUIRE(ownerReports.attributeOwnershipReports.size() == 1U);
      auto const report = ownerReports.attributeOwnershipReports.front();
      ownerReports.attributeOwnershipReports.clear();
      REQUIRE(report.objectInstance == objectInstance);
      REQUIRE(report.attributes == AttributeHandleSet{attribute});
      return report;
    };
    auto const initialFree = queryOne(freeAttribute);
    auto const initialHeld = queryOne(heldAttribute);
    REQUIRE(initialFree.kind ==
            ReportingFederateAmbassador::AttributeOwnershipReport::Kind::federate);
    REQUIRE(initialHeld.kind ==
            ReportingFederateAmbassador::AttributeOwnershipReport::Kind::federate);
    REQUIRE(initialFree.owner == ownerHandle);
    REQUIRE(initialHeld.owner == ownerHandle);

    unsigned char const divestitureTag[] = {0x54, 0x10};
    REQUIRE_NOTHROW(owner->unconditionalAttributeOwnershipDivestiture(
        objectInstance, AttributeHandleSet{freeAttribute},
        VariableLengthData(divestitureTag, sizeof(divestitureTag))));
    auto const unowned = queryOne(freeAttribute);
    auto const stillHeld = queryOne(heldAttribute);
    REQUIRE(unowned.kind ==
            ReportingFederateAmbassador::AttributeOwnershipReport::Kind::unowned);
    REQUIRE(stillHeld.kind ==
            ReportingFederateAmbassador::AttributeOwnershipReport::Kind::federate);
    REQUIRE(stillHeld.owner == ownerHandle);

    unsigned char const requestTag[] = {0xA7, 0x10, 0x11};
    REQUIRE_NOTHROW(requester->attributeOwnershipAcquisitionIfAvailable(
        objectInstance, both,
        VariableLengthData(requestTag, sizeof(requestTag))));
    REQUIRE(requesterReports.acquisitionNotifications.empty());
    REQUIRE(requesterReports.acquisitionUnavailableReports.empty());
    auto const terminalDeadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while ((requesterReports.acquisitionNotifications.empty() ||
            requesterReports.acquisitionUnavailableReports.empty()) &&
           std::chrono::steady_clock::now() < terminalDeadline) {
      static_cast<void>(requester->evokeMultipleCallbacks(0.01, 0.02));
    }
    // The standard permits distinct callbacks for the same request; assert
    // per-attribute terminal outcomes without imposing callback order.
    REQUIRE(requesterReports.acquisitionNotifications.size() == 1U);
    REQUIRE(requesterReports.acquisitionUnavailableReports.size() == 1U);
    auto const& acquired = requesterReports.acquisitionNotifications.front();
    auto const& unavailable =
        requesterReports.acquisitionUnavailableReports.front();
    REQUIRE(acquired.objectInstance == objectInstance);
    REQUIRE(unavailable.objectInstance == objectInstance);
    REQUIRE(acquired.attributes == AttributeHandleSet{freeAttribute});
    REQUIRE(unavailable.attributes == AttributeHandleSet{heldAttribute});
    REQUIRE(acquired.userSuppliedTag.size() == sizeof(requestTag));
    REQUIRE(unavailable.userSuppliedTag.size() == sizeof(requestTag));
    REQUIRE(std::equal(
        requestTag, requestTag + sizeof(requestTag),
        static_cast<unsigned char const*>(acquired.userSuppliedTag.data())));
    REQUIRE(std::equal(
        requestTag, requestTag + sizeof(requestTag),
        static_cast<unsigned char const*>(unavailable.userSuppliedTag.data())));

    auto const newOwner = queryOne(freeAttribute);
    auto const unchangedOwner = queryOne(heldAttribute);
    REQUIRE(newOwner.kind ==
            ReportingFederateAmbassador::AttributeOwnershipReport::Kind::federate);
    REQUIRE(unchangedOwner.kind ==
            ReportingFederateAmbassador::AttributeOwnershipReport::Kind::federate);
    REQUIRE(newOwner.owner == requesterHandle);
    REQUIRE(unchangedOwner.owner == ownerHandle);
    for (std::size_t pass = 0U; pass != 3U; ++pass) {
      static_cast<void>(owner->evokeMultipleCallbacks(0.01, 0.02));
    }
    REQUIRE(ownerReports.ownershipReleaseRequests.empty());

    REQUIRE_NOTHROW(requester->resignFederationExecution(
        rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES));
    requesterJoined = false;
    REQUIRE_NOTHROW(owner->resignFederationExecution(
        rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES));
    ownerJoined = false;
    REQUIRE_NOTHROW(requester->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
  } catch (...) {
    clientError = std::current_exception();
    if (requesterJoined) {
      try {
        requester->resignFederationExecution(
            rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST);
      } catch (...) {
      }
    }
    if (ownerJoined) {
      try {
        owner->resignFederationExecution(
            rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST);
      } catch (...) {
      }
    }
    for (auto* client : {requester.get(), owner.get()}) {
      try {
        client->disconnect();
      } catch (...) {
      }
    }
  }
  listener.reset();
  if (server.joinable()) server.join();
  if (clientError) std::rethrow_exception(clientError);
  REQUIRE_FALSE(serverError);
}

#endif
