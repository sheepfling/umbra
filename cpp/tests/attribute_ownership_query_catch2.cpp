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
#include "internal/fom/libxml2_fom_composer.hpp"
#include "internal/fom/libxml2_fom_validator.hpp"
#include "internal/handles/attribute_handle.hpp"
#include "internal/handles/object_instance_handle.hpp"
#endif

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The attribute-ownership query test requires the Umbra source directory."
#endif

namespace {

namespace fixture_hla = umbra::test::hla::wide;
namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
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

  struct AttributeOwnershipReport final {
    enum class Kind { federate, unowned, rti };

    Kind kind = Kind::unowned;
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    FederateHandle owner;
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
  std::vector<AttributeOwnershipReport> attributeOwnershipReports;
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
  REQUIRE(report.parameterValues.size() == 7U);
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
        if (!ProcessTransportServiceDispatcher::serveOne(
                session,
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
        if (!ProcessTransportServiceDispatcher::serveOne(
                session,
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
  REQUIRE(rti->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(rti->evokeMultipleCallbacks(0.0, 0.0));
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
#endif
