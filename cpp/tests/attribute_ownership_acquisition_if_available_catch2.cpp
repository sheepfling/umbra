#include <catch2/catch_test_macros.hpp>

#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

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
#include <filesystem>
#include <memory>
#include <exception>
#include <stdexcept>
#include <thread>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The If Available ownership test requires the Umbra source directory."
#endif

namespace {

namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
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
  return L"attribute-ownership-if-available-" +
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
          L"urn:umbra:test:process-ownership-if-available-mim"),
      validatedProcessModule(
          resourcePath("attribute-update-passel-fom.xml"),
          FomModuleKind::fom,
          L"urn:umbra:test:process-ownership-if-available-fom"),
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

  struct AttributeOwnershipAcquisitionReport final {
    enum class Kind { notification, unavailable };

    Kind kind = Kind::unavailable;
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
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

  void attributeOwnershipAcquisitionNotification(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& securedAttributes,
      VariableLengthData const& userSuppliedTag) override {
    attributeOwnershipAcquisitionReports.push_back({
        AttributeOwnershipAcquisitionReport::Kind::notification,
        objectInstance,
        securedAttributes,
        userSuppliedTag,
    });
  }

  void attributeOwnershipUnavailable(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      VariableLengthData const& userSuppliedTag) override {
    attributeOwnershipAcquisitionReports.push_back({
        AttributeOwnershipAcquisitionReport::Kind::unavailable,
        objectInstance,
        attributes,
        userSuppliedTag,
    });
  }

  std::vector<ObjectDiscoveryReport> objectDiscoveryReports;
  std::vector<ObjectRemovalReport> objectRemovalReports;
  std::vector<AttributeOwnershipAcquisitionReport>
      attributeOwnershipAcquisitionReports;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::vector<unsigned char> variableLengthDataBytes(
    VariableLengthData const& value) {
  auto const* bytes = static_cast<unsigned char const*>(value.data());
  if (bytes == nullptr || value.size() == 0U) {
    return {};
  }
  return {bytes, bytes + value.size()};
}

}  // namespace

TEST_CASE(
    "Standalone Attribute Ownership Acquisition If Available resolves 2025 callbacks",
    "[integration][development-profile][ownership-management]"
    "[attribute-ownership-acquisition-if-available][standalone]"
    "[rti.service.attribute-ownership-acquisition-if-available]"
    "[federate.callback.attribute-ownership-acquisition-notification]"
    "[federate.callback.attribute-ownership-unavailable]") {
  rti1516_2025::NullFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("attribute-update-passel-fom.xml").wstring();
  unsigned char const tagBytes[] = {0x51, 0xA7, 0x0C};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  ObjectInstanceHandle invalidObjectInstance;
  AttributeHandle invalidAttribute;

  REQUIRE_THROWS_AS(
      unjoined->attributeOwnershipAcquisitionIfAvailable(
          invalidObjectInstance, AttributeHandleSet{}, tag),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->attributeOwnershipAcquisitionIfAvailable(
          invalidObjectInstance, AttributeHandleSet{}, tag),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"if-available-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"if-available-requester", L"publisher", federationName));

  auto const child = owner->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const reliableBaseA = owner->getAttributeHandle(
      child, fixture_hla::fixture::reliable_base_a);
  auto const reliableChild = owner->getAttributeHandle(
      child, fixture_hla::fixture::reliable_child);
  auto const unownedChild = owner->getAttributeHandle(
      child, fixture_hla::fixture::unowned_child);
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(reliableChild.isValid());
  REQUIRE(unownedChild.isValid());

  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(
      child, AttributeHandleSet{reliableBaseA, reliableChild, unownedChild}));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
      child, AttributeHandleSet{reliableBaseA}));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(requester->getKnownObjectClassHandle(objectInstance) == child);

  // The service requires publication at the known class for every requested
  // attribute before a Willing to Acquire reservation can be accepted.
  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisitionIfAvailable(
          objectInstance, AttributeHandleSet{unownedChild}, tag),
      rti1516_2025::ObjectClassNotPublished);
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(
      child, AttributeHandleSet{unownedChild}));
  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisitionIfAvailable(
          objectInstance, AttributeHandleSet{reliableBaseA}, tag),
      rti1516_2025::AttributeNotPublished);
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(
      child, AttributeHandleSet{reliableBaseA, reliableChild}));
  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisitionIfAvailable(
          invalidObjectInstance, AttributeHandleSet{unownedChild}, tag),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisitionIfAvailable(
          objectInstance, AttributeHandleSet{invalidAttribute}, tag),
      rti1516_2025::AttributeNotDefined);

  AttributeHandleSet const mixedAvailabilityAttributes{unownedChild, reliableBaseA};
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisitionIfAvailable(
      objectInstance, mixedAvailabilityAttributes, tag));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, unownedChild));
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisitionIfAvailable(
      objectInstance, AttributeHandleSet{unownedChild}, tag));
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisitionIfAvailable(
      objectInstance, AttributeHandleSet{unownedChild, reliableChild}, tag));
  REQUIRE_THROWS_AS(
      requester->unpublishObjectClassAttributes(
          child, AttributeHandleSet{unownedChild}),
      rti1516_2025::OwnershipAcquisitionPending);

  REQUIRE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.size() == 3U);
  auto const notification = std::find_if(
      requesterReports.attributeOwnershipAcquisitionReports.begin(),
      requesterReports.attributeOwnershipAcquisitionReports.end(),
      [](auto const& report) {
        return report.kind ==
            ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification &&
            report.attributes.size() == 1U;
      });
  auto const unavailable = std::find_if(
      requesterReports.attributeOwnershipAcquisitionReports.begin(),
      requesterReports.attributeOwnershipAcquisitionReports.end(),
      [&reliableBaseA](auto const& report) {
        return report.kind ==
                   ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::unavailable &&
               report.attributes == AttributeHandleSet{reliableBaseA};
      });
  REQUIRE(notification != requesterReports.attributeOwnershipAcquisitionReports.end());
  REQUIRE(unavailable != requesterReports.attributeOwnershipAcquisitionReports.end());
  REQUIRE(notification->objectInstance == objectInstance);
  REQUIRE(variableLengthDataBytes(notification->userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(unavailable->objectInstance == objectInstance);
  REQUIRE(variableLengthDataBytes(unavailable->userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(std::any_of(
      requesterReports.attributeOwnershipAcquisitionReports.begin(),
      requesterReports.attributeOwnershipAcquisitionReports.end(),
      [&reliableChild](auto const& report) {
        return report.kind ==
                   ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification &&
               report.attributes == AttributeHandleSet{reliableChild};
      }));
  REQUIRE(requester->isAttributeOwnedByFederate(objectInstance, unownedChild));
  REQUIRE(requester->isAttributeOwnedByFederate(objectInstance, reliableChild));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisitionIfAvailable(
          objectInstance, AttributeHandleSet{unownedChild}, tag),
      rti1516_2025::FederateOwnsAttributes);
  REQUIRE_NOTHROW(requester->unpublishObjectClassAttributes(
      child, AttributeHandleSet{unownedChild}));

  // Deletion suppresses a pending terminal callback but still delivers the
  // receive-order removal callback.
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisitionIfAvailable(
      objectInstance, AttributeHandleSet{reliableBaseA}, tag));
  VariableLengthData deletionTag;
  REQUIRE_NOTHROW(owner->deleteObjectInstance(objectInstance, deletionTag));
  REQUIRE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.size() == 3U);
  REQUIRE(requesterReports.objectRemovalReports.size() == 1U);
  REQUIRE(requesterReports.objectRemovalReports.front().objectInstance == objectInstance);
  REQUIRE(ownerReports.attributeOwnershipAcquisitionReports.empty());

  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(requester->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
TEST_CASE(
    "RTIambassador carries Attribute Ownership Acquisition If Available through a configured process endpoint",
    "[integration][development-profile][federation-management][ownership-management]"
    "[transport][process-boundary][process-ownership-acquisition-if-available][public-endpoint]"
    "[attribute-ownership-acquisition-if-available]"
    "[federate.callback.attribute-ownership-acquisition-notification][2025]") {
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
          {"process-ownership-if-available-server", 0xA703U},
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
                        "The process ownership-acquisition server received an unexpected operation.");
                  }
                  return handler(request);
                })) {
          throw std::runtime_error(
              "The process ownership-acquisition server lost a request.");
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
      serveExpected(TransportServiceOperation::publish_object_class_attributes);
      serveExpected(
          TransportServiceOperation::attribute_ownership_acquisition_if_available);
      serveExpected(TransportServiceOperation::receive_interaction);
      serveExpected(TransportServiceOperation::receive_interaction);
      serveExpected(TransportServiceOperation::resign_federation_execution);
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  auto const federationName = nextFederationName() + L"-process-if-available";
  auto const fomModule = resourcePath("attribute-update-passel-fom.xml").wstring();
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto configuration = rti1516_2025::RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"process-ownership-if-available-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(rti->createFederationExecution(
      federationName,
      fomModule));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"process-ownership-if-available-federate",
      L"publisher",
      federationName));

  auto const child = rti->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const ownedAttribute = rti->getAttributeHandle(
      child,
      fixture_hla::fixture::reliable_base_a);
  auto const acquirableAttribute = rti->getAttributeHandle(
      child,
      fixture_hla::fixture::unowned_child);
  REQUIRE(child.isValid());
  REQUIRE(ownedAttribute.isValid());
  REQUIRE(acquirableAttribute.isValid());
  REQUIRE_NOTHROW(rti->publishObjectClassAttributes(
      child,
      AttributeHandleSet{ownedAttribute}));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = rti->registerObjectInstance(child));
  REQUIRE(objectInstance.isValid());
  REQUIRE_NOTHROW(rti->publishObjectClassAttributes(
      child,
      AttributeHandleSet{acquirableAttribute}));

  unsigned char const tagBytes[] = {0x3C, 0xA1, 0x7E};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  REQUIRE_NOTHROW(rti->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      AttributeHandleSet{acquirableAttribute},
      tag));
  REQUIRE(reports.attributeOwnershipAcquisitionReports.empty());
  static_cast<void>(rti->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(reports.attributeOwnershipAcquisitionReports.size() == 1U);
  auto const& report = reports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(report.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
  REQUIRE(report.objectInstance == objectInstance);
  REQUIRE(report.attributes == AttributeHandleSet{acquirableAttribute});
  REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));

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
