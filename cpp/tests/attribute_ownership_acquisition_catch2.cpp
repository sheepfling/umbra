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
#endif

#include <atomic>
#include <exception>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The regular ownership-acquisition test requires the Umbra source directory."
#endif

namespace {

namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::HLA_IMMEDIATE;
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
using umbra::detail::ProcessFederationServiceOptions;
using umbra::detail::ProcessTransportListener;
using umbra::detail::ProcessTransportServiceDispatcher;
using umbra::detail::ProcessTransportSession;
#endif

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"attribute-ownership-acquisition-" +
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
          L"urn:umbra:test:process-ownership-acquisition-mim"),
      validatedProcessModule(
          resourcePath("attribute-update-passel-fom.xml"),
          FomModuleKind::fom,
          L"urn:umbra:test:process-ownership-acquisition-fom"),
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

  struct AcquisitionReport final {
    enum class Kind { notification, unavailable };

    Kind kind = Kind::unavailable;
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  struct ReleaseRequestReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  struct AcquisitionCancellationReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
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
    acquisitionReports.push_back({
        AcquisitionReport::Kind::notification,
        objectInstance,
        securedAttributes,
        userSuppliedTag,
    });
  }

  void attributeOwnershipUnavailable(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      VariableLengthData const& userSuppliedTag) override {
    acquisitionReports.push_back({
        AcquisitionReport::Kind::unavailable,
        objectInstance,
        attributes,
        userSuppliedTag,
    });
  }

  void requestAttributeOwnershipRelease(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& candidateAttributes,
      VariableLengthData const& userSuppliedTag) override {
    releaseRequests.push_back({
        objectInstance,
        candidateAttributes,
        userSuppliedTag,
    });
  }

  void confirmAttributeOwnershipAcquisitionCancellation(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes) override {
    acquisitionCancellationReports.push_back({objectInstance, attributes});
  }

  std::vector<ObjectDiscoveryReport> objectDiscoveryReports;
  std::vector<ObjectRemovalReport> objectRemovalReports;
  std::vector<AcquisitionReport> acquisitionReports;
  std::vector<ReleaseRequestReport> releaseRequests;
  std::vector<AcquisitionCancellationReport> acquisitionCancellationReports;
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
    "Standalone Attribute Ownership Acquisition honors 2025 release and denial callbacks",
    "[integration][development-profile][ownership-management]"
    "[attribute-ownership-acquisition][standalone]"
    "[rti.service.attribute-ownership-acquisition]"
    "[federate.callback.request-attribute-ownership-release]"
    "[rti.service.attribute-ownership-release-denied]"
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
  unsigned char const willingToAcquireTagBytes[] = {0x19, 0x53};
  unsigned char const acquisitionTagBytes[] = {0xA5, 0x70, 0xE1};
  unsigned char const denialTagBytes[] = {0xD3, 0x1A, 0x1E, 0xD0};
  VariableLengthData const willingToAcquireTag(
      willingToAcquireTagBytes, sizeof(willingToAcquireTagBytes));
  VariableLengthData const acquisitionTag(acquisitionTagBytes,
                                          sizeof(acquisitionTagBytes));
  VariableLengthData const denialTag(denialTagBytes, sizeof(denialTagBytes));
  ObjectInstanceHandle invalidObjectInstance;
  AttributeHandle invalidAttribute;

  REQUIRE_THROWS_AS(
      unjoined->attributeOwnershipAcquisition(
          invalidObjectInstance, AttributeHandleSet{}, acquisitionTag),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->attributeOwnershipAcquisition(
          invalidObjectInstance, AttributeHandleSet{}, acquisitionTag),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"regular-acquisition-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"regular-acquisition-requester", L"publisher", federationName));

  auto const child = owner->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const reliableBaseA = owner->getAttributeHandle(
      child, fixture_hla::fixture::reliable_base_a);
  auto const unownedChild = owner->getAttributeHandle(
      child, fixture_hla::fixture::unowned_child);
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(unownedChild.isValid());

  AttributeHandleSet const requesterAttributes{reliableBaseA, unownedChild};
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(
      child, requesterAttributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
      child, AttributeHandleSet{reliableBaseA}));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(requester->getKnownObjectClassHandle(objectInstance) == child);

  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisition(
          objectInstance, AttributeHandleSet{unownedChild}, acquisitionTag),
      rti1516_2025::ObjectClassNotPublished);
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(
      child, requesterAttributes));
  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisition(
          invalidObjectInstance, AttributeHandleSet{unownedChild}, acquisitionTag),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisition(
          objectInstance, AttributeHandleSet{invalidAttribute}, acquisitionTag),
      rti1516_2025::AttributeNotDefined);
  REQUIRE_THROWS_AS(
      owner->attributeOwnershipReleaseDenied(
          objectInstance, AttributeHandleSet{unownedChild}, denialTag),
      rti1516_2025::AttributeNotOwned);

  // A regular request supersedes the same attribute's pending If Available
  // reservation, but the regular request still asks the owner for release.
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisitionIfAvailable(
      objectInstance, AttributeHandleSet{reliableBaseA}, willingToAcquireTag));
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance, requesterAttributes, acquisitionTag));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, unownedChild));
  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisitionIfAvailable(
          objectInstance, AttributeHandleSet{reliableBaseA}, willingToAcquireTag),
      rti1516_2025::AttributeAlreadyBeingAcquired);
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance, AttributeHandleSet{reliableBaseA}, acquisitionTag));
  REQUIRE_THROWS_AS(
      requester->unpublishObjectClassAttributes(
          child, AttributeHandleSet{unownedChild}),
      rti1516_2025::OwnershipAcquisitionPending);
  REQUIRE_THROWS_AS(
      requester->unpublishObjectClassAttributes(
          child, AttributeHandleSet{reliableBaseA}),
      rti1516_2025::OwnershipAcquisitionPending);

  REQUIRE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.releaseRequests.size() == 1U);
  auto const& releaseRequest = ownerReports.releaseRequests.front();
  REQUIRE(releaseRequest.objectInstance == objectInstance);
  REQUIRE(releaseRequest.attributes == AttributeHandleSet{reliableBaseA});
  REQUIRE(variableLengthDataBytes(releaseRequest.userSuppliedTag) ==
          std::vector<unsigned char>(
              acquisitionTagBytes,
              acquisitionTagBytes + sizeof(acquisitionTagBytes)));

  REQUIRE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.acquisitionReports.empty());
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.acquisitionReports.size() == 1U);
  auto const& notification = requesterReports.acquisitionReports.front();
  REQUIRE(notification.kind ==
          ReportingFederateAmbassador::AcquisitionReport::Kind::notification);
  REQUIRE(notification.objectInstance == objectInstance);
  REQUIRE(notification.attributes == AttributeHandleSet{unownedChild});
  REQUIRE(variableLengthDataBytes(notification.userSuppliedTag) ==
          std::vector<unsigned char>(
              acquisitionTagBytes,
              acquisitionTagBytes + sizeof(acquisitionTagBytes)));
  REQUIRE(requester->isAttributeOwnedByFederate(objectInstance, unownedChild));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_NOTHROW(requester->unpublishObjectClassAttributes(
      child, AttributeHandleSet{unownedChild}));
  REQUIRE_THROWS_AS(
      requester->unpublishObjectClassAttributes(
          child, AttributeHandleSet{reliableBaseA}),
      rti1516_2025::OwnershipAcquisitionPending);

  REQUIRE_NOTHROW(owner->attributeOwnershipReleaseDenied(
      objectInstance, AttributeHandleSet{reliableBaseA}, denialTag));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.acquisitionReports.size() == 2U);
  auto const& unavailable = requesterReports.acquisitionReports.back();
  REQUIRE(unavailable.kind ==
          ReportingFederateAmbassador::AcquisitionReport::Kind::unavailable);
  REQUIRE(unavailable.objectInstance == objectInstance);
  REQUIRE(unavailable.attributes == AttributeHandleSet{reliableBaseA});
  REQUIRE(variableLengthDataBytes(unavailable.userSuppliedTag) ==
          std::vector<unsigned char>(denialTagBytes,
                                     denialTagBytes + sizeof(denialTagBytes)));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_NOTHROW(requester->unpublishObjectClassAttributes(
      child, AttributeHandleSet{reliableBaseA}));
  REQUIRE_NOTHROW(owner->attributeOwnershipReleaseDenied(
      objectInstance, AttributeHandleSet{reliableBaseA}, denialTag));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));

  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(
      child, AttributeHandleSet{reliableBaseA}));
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance, AttributeHandleSet{reliableBaseA}, acquisitionTag));
  VariableLengthData deletionTag;
  REQUIRE_NOTHROW(owner->deleteObjectInstance(objectInstance, deletionTag));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.releaseRequests.size() == 1U);
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.acquisitionReports.size() == 2U);
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

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
TEST_CASE(
    "RTIambassador carries Attribute Ownership Acquisition through a configured process endpoint",
    "[integration][development-profile][federation-management][ownership-management]"
    "[transport][process-boundary][process-ownership-acquisition][public-endpoint]"
    "[attribute-ownership-acquisition]"
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
          {"process-ownership-acquisition-server", 0xA704U},
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
      serveExpected(TransportServiceOperation::attribute_ownership_acquisition);
      serveExpected(TransportServiceOperation::receive_interaction);
      serveExpected(TransportServiceOperation::receive_interaction);
      serveExpected(TransportServiceOperation::resign_federation_execution);
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  auto const federationName = nextFederationName() + L"-process-acquisition";
  auto const fomModule = resourcePath("attribute-update-passel-fom.xml").wstring();
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto configuration = rti1516_2025::RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"process-ownership-acquisition-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(rti->createFederationExecution(
      federationName,
      fomModule));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"process-ownership-acquisition-federate",
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
  REQUIRE_NOTHROW(rti->attributeOwnershipAcquisition(
      objectInstance,
      AttributeHandleSet{acquirableAttribute},
      tag));
  REQUIRE(reports.acquisitionReports.empty());
  static_cast<void>(rti->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(reports.acquisitionReports.size() == 1U);
  auto const& report = reports.acquisitionReports.front();
  REQUIRE(report.kind ==
          ReportingFederateAmbassador::AcquisitionReport::Kind::notification);
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

TEST_CASE(
    "RTIambassadors deliver the Attribute Ownership Acquisition release request through a configured process endpoint",
    "[integration][development-profile][federation-management][ownership-management]"
    "[transport][process-boundary][process-ownership-acquisition-release][public-endpoint]"
    "[attribute-ownership-acquisition]"
    "[federate.callback.request-attribute-ownership-release][2025]") {
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
      auto ownerConnection = listener->accept(
          nullptr,
          {"process-ownership-acquisition-release-server", 0xA705U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession ownerSession(ownerConnection);
      auto ownerHandler = service.handlerFor(ownerSession);
      auto serveExpected = [&](ProcessTransportSession& session,
                               auto const& handler,
                               umbra::detail::TransportServiceOperation operation,
                               char const* description) {
        if (!ProcessTransportServiceDispatcher::serveOne(
                session,
                [&](umbra::detail::TransportServiceMessage const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(description);
                  }
                  return handler(request);
                })) {
          throw std::runtime_error(description);
        }
      };

      using umbra::detail::TransportServiceOperation;
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::create_federation_execution,
          "The process ownership-acquisition release server lost Create.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::join_federation_execution,
          "The process ownership-acquisition release server lost owner Join.");

      auto requesterConnection = listener->accept(
          nullptr,
          {"process-ownership-acquisition-release-server", 0xA706U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession requesterSession(requesterConnection);
      auto requesterHandler = service.handlerFor(requesterSession);
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::join_federation_execution,
          "The process ownership-acquisition release server lost requester Join.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::get_object_class_handle,
          "The process ownership-acquisition release server lost requester class lookup.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::get_attribute_handle,
          "The process ownership-acquisition release server lost requester attribute lookup.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::subscribe_object_class_attributes,
          "The process ownership-acquisition release server lost requester Subscribe.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::get_object_class_handle,
          "The process ownership-acquisition release server lost owner class lookup.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::get_attribute_handle,
          "The process ownership-acquisition release server lost owner attribute lookup.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The process ownership-acquisition release server lost owner Publish.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::register_object_instance,
          "The process ownership-acquisition release server lost owner Register.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::receive_interaction,
          "The process ownership-acquisition release server lost requester discovery poll.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::receive_interaction,
          "The process ownership-acquisition release server lost requester discovery drain.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The process ownership-acquisition release server lost requester Publish.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::attribute_ownership_acquisition,
          "The process ownership-acquisition release server lost Acquisition.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::receive_interaction,
          "The process ownership-acquisition release server lost owner release-request poll.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::receive_interaction,
          "The process ownership-acquisition release server lost owner release-request drain.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::resign_federation_execution,
          "The process ownership-acquisition release server lost requester Resign.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::resign_federation_execution,
          "The process ownership-acquisition release server lost owner Resign.");
      service.detach(ownerSession);
      service.detach(requesterSession);
      ownerConnection->close();
      requesterConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  auto const federationName = nextFederationName() + L"-process-acquisition-release";
  auto const fomModule = resourcePath("attribute-update-passel-fom.xml").wstring();
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto ownerConfiguration = rti1516_2025::RtiConfiguration::createConfiguration()
                                .withConfigurationName(
                                    L"process-ownership-acquisition-release-owner")
                                .withRtiAddress(
                                    L"tcp://127.0.0.1:" + std::to_wstring(port));
  auto requesterConfiguration = rti1516_2025::RtiConfiguration::createConfiguration()
                                    .withConfigurationName(
                                        L"process-ownership-acquisition-release-requester")
                                    .withRtiAddress(
                                        L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool ownerJoined = false;
  bool requesterJoined = false;
  try {
    REQUIRE_NOTHROW(owner->connect(
        ownerReports, HLA_EVOKED, ownerConfiguration));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName,
        fomModule));
    REQUIRE_NOTHROW(owner->joinFederationExecution(
        L"process-ownership-acquisition-release-owner",
        L"publisher",
        federationName));
    ownerJoined = true;

    REQUIRE_NOTHROW(requester->connect(
        requesterReports, HLA_EVOKED, requesterConfiguration));
    REQUIRE_NOTHROW(requester->joinFederationExecution(
        L"process-ownership-acquisition-release-requester",
        L"publisher",
        federationName));
    requesterJoined = true;

    auto const requesterClass = requester->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const requesterAttribute = requester->getAttributeHandle(
        requesterClass,
        fixture_hla::fixture::reliable_base_a);
    REQUIRE(requesterClass.isValid());
    REQUIRE(requesterAttribute.isValid());
    REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(
        requesterClass,
        AttributeHandleSet{requesterAttribute}));

    auto const ownerClass = owner->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const ownerAttribute = owner->getAttributeHandle(
        ownerClass,
        fixture_hla::fixture::reliable_base_a);
    REQUIRE(ownerClass.isValid());
    REQUIRE(ownerAttribute.isValid());
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
        ownerClass,
        AttributeHandleSet{ownerAttribute}));

    ObjectInstanceHandle objectInstance;
    REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(ownerClass));
    REQUIRE(objectInstance.isValid());
    REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
    REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(requesterReports.objectDiscoveryReports.front().objectInstance ==
            objectInstance);
    REQUIRE_NOTHROW(requester->publishObjectClassAttributes(
        requesterClass,
        AttributeHandleSet{requesterAttribute}));

    unsigned char const tagBytes[] = {0xC2, 0x11, 0x6D, 0x09};
    VariableLengthData const tag(tagBytes, sizeof(tagBytes));
    REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
        objectInstance,
        AttributeHandleSet{requesterAttribute},
        tag));
    REQUIRE(ownerReports.releaseRequests.empty());
    static_cast<void>(owner->evokeMultipleCallbacks(0.0, 0.0));
    REQUIRE(ownerReports.releaseRequests.size() == 1U);
    auto const& releaseRequest = ownerReports.releaseRequests.front();
    REQUIRE(releaseRequest.objectInstance == objectInstance);
    REQUIRE(releaseRequest.attributes == AttributeHandleSet{ownerAttribute});
    REQUIRE(variableLengthDataBytes(releaseRequest.userSuppliedTag) ==
            std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));

    REQUIRE_NOTHROW(requester->resignFederationExecution(
        rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES));
    requesterJoined = false;
    REQUIRE_NOTHROW(owner->resignFederationExecution(
        rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
    ownerJoined = false;
    REQUIRE_NOTHROW(owner->disconnect());
    REQUIRE_NOTHROW(requester->disconnect());
  } catch (...) {
    clientError = std::current_exception();
    if (requesterJoined) {
      try {
        requester->resignFederationExecution(
            rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES);
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
    try {
      requester->disconnect();
    } catch (...) {
    }
    try {
      owner->disconnect();
    } catch (...) {
    }
  }
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
}

TEST_CASE(
    "RTIambassadors deliver Attribute Ownership Unavailable after a denied process acquisition",
    "[integration][development-profile][federation-management][ownership-management]"
    "[transport][process-boundary][process-ownership-acquisition-release-denied][public-endpoint]"
    "[attribute-ownership-acquisition][federate.callback.attribute-ownership-unavailable][2025]") {
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
      auto ownerConnection = listener->accept(
          nullptr,
          {"process-ownership-acquisition-release-denied-server", 0xA707U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession ownerSession(ownerConnection);
      auto ownerHandler = service.handlerFor(ownerSession);
      auto serveExpected = [&](ProcessTransportSession& session,
                               auto const& handler,
                               umbra::detail::TransportServiceOperation operation,
                               char const* description) {
        if (!ProcessTransportServiceDispatcher::serveOne(
                session,
                [&](umbra::detail::TransportServiceMessage const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(description);
                  }
                  return handler(request);
                })) {
          throw std::runtime_error(description);
        }
      };

      using umbra::detail::TransportServiceOperation;
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::create_federation_execution,
          "The process ownership-acquisition release-denied server lost Create.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::join_federation_execution,
          "The process ownership-acquisition release-denied server lost owner Join.");

      auto requesterConnection = listener->accept(
          nullptr,
          {"process-ownership-acquisition-release-denied-server", 0xA708U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession requesterSession(requesterConnection);
      auto requesterHandler = service.handlerFor(requesterSession);
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::join_federation_execution,
          "The process ownership-acquisition release-denied server lost requester Join.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::get_object_class_handle,
          "The process ownership-acquisition release-denied server lost requester class lookup.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::get_attribute_handle,
          "The process ownership-acquisition release-denied server lost requester attribute lookup.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::subscribe_object_class_attributes,
          "The process ownership-acquisition release-denied server lost requester Subscribe.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::get_object_class_handle,
          "The process ownership-acquisition release-denied server lost owner class lookup.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::get_attribute_handle,
          "The process ownership-acquisition release-denied server lost owner attribute lookup.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The process ownership-acquisition release-denied server lost owner Publish.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::register_object_instance,
          "The process ownership-acquisition release-denied server lost owner Register.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::receive_interaction,
          "The process ownership-acquisition release-denied server lost requester discovery poll.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::receive_interaction,
          "The process ownership-acquisition release-denied server lost requester discovery drain.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The process ownership-acquisition release-denied server lost requester Publish.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::attribute_ownership_acquisition,
          "The process ownership-acquisition release-denied server lost Acquisition.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::receive_interaction,
          "The process ownership-acquisition release-denied server lost owner release-request poll.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::receive_interaction,
          "The process ownership-acquisition release-denied server lost owner release-request drain.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::attribute_ownership_release_denied,
          "The process ownership-acquisition release-denied server lost Release Denied.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::receive_interaction,
          "The process ownership-acquisition release-denied server lost requester unavailable poll.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::receive_interaction,
          "The process ownership-acquisition release-denied server lost requester unavailable drain.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::resign_federation_execution,
          "The process ownership-acquisition release-denied server lost requester Resign.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::resign_federation_execution,
          "The process ownership-acquisition release-denied server lost owner Resign.");
      service.detach(ownerSession);
      service.detach(requesterSession);
      ownerConnection->close();
      requesterConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  auto const federationName = nextFederationName() + L"-process-acquisition-release-denied";
  auto const fomModule = resourcePath("attribute-update-passel-fom.xml").wstring();
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto ownerConfiguration = rti1516_2025::RtiConfiguration::createConfiguration()
                                .withConfigurationName(
                                    L"process-ownership-acquisition-release-denied-owner")
                                .withRtiAddress(
                                    L"tcp://127.0.0.1:" + std::to_wstring(port));
  auto requesterConfiguration = rti1516_2025::RtiConfiguration::createConfiguration()
                                    .withConfigurationName(
                                        L"process-ownership-acquisition-release-denied-requester")
                                    .withRtiAddress(
                                        L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool ownerJoined = false;
  bool requesterJoined = false;
  try {
    REQUIRE_NOTHROW(owner->connect(
        ownerReports, HLA_EVOKED, ownerConfiguration));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName,
        fomModule));
    REQUIRE_NOTHROW(owner->joinFederationExecution(
        L"process-ownership-acquisition-release-denied-owner",
        L"publisher",
        federationName));
    ownerJoined = true;

    REQUIRE_NOTHROW(requester->connect(
        requesterReports, HLA_EVOKED, requesterConfiguration));
    REQUIRE_NOTHROW(requester->joinFederationExecution(
        L"process-ownership-acquisition-release-denied-requester",
        L"publisher",
        federationName));
    requesterJoined = true;

    auto const requesterClass = requester->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const requesterAttribute = requester->getAttributeHandle(
        requesterClass,
        fixture_hla::fixture::reliable_base_a);
    REQUIRE(requesterClass.isValid());
    REQUIRE(requesterAttribute.isValid());
    REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(
        requesterClass,
        AttributeHandleSet{requesterAttribute}));

    auto const ownerClass = owner->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const ownerAttribute = owner->getAttributeHandle(
        ownerClass,
        fixture_hla::fixture::reliable_base_a);
    REQUIRE(ownerClass.isValid());
    REQUIRE(ownerAttribute.isValid());
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
        ownerClass,
        AttributeHandleSet{ownerAttribute}));

    ObjectInstanceHandle objectInstance;
    REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(ownerClass));
    REQUIRE(objectInstance.isValid());
    REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
    REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(requesterReports.objectDiscoveryReports.front().objectInstance ==
            objectInstance);

    REQUIRE_NOTHROW(requester->publishObjectClassAttributes(
        requesterClass,
        AttributeHandleSet{requesterAttribute}));

    unsigned char const acquisitionTagBytes[] = {0xC2, 0x11, 0x6D, 0x09};
    VariableLengthData const acquisitionTag(
        acquisitionTagBytes, sizeof(acquisitionTagBytes));
    REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
        objectInstance,
        AttributeHandleSet{requesterAttribute},
        acquisitionTag));
    REQUIRE(ownerReports.releaseRequests.empty());
    static_cast<void>(owner->evokeMultipleCallbacks(0.0, 0.0));
    REQUIRE(ownerReports.releaseRequests.size() == 1U);
    auto const& releaseRequest = ownerReports.releaseRequests.front();
    REQUIRE(releaseRequest.objectInstance == objectInstance);
    REQUIRE(releaseRequest.attributes == AttributeHandleSet{ownerAttribute});

    unsigned char const denialTagBytes[] = {0xDA, 0x7A, 0x01};
    VariableLengthData const denialTag(denialTagBytes, sizeof(denialTagBytes));
    REQUIRE_NOTHROW(owner->attributeOwnershipReleaseDenied(
        objectInstance,
        AttributeHandleSet{ownerAttribute},
        denialTag));
    REQUIRE(requesterReports.acquisitionReports.empty());
    static_cast<void>(requester->evokeMultipleCallbacks(0.0, 0.0));
    REQUIRE(requesterReports.acquisitionReports.size() == 1U);
    auto const& unavailable = requesterReports.acquisitionReports.front();
    REQUIRE(unavailable.kind ==
            ReportingFederateAmbassador::AcquisitionReport::Kind::unavailable);
    REQUIRE(unavailable.objectInstance == objectInstance);
    REQUIRE(unavailable.attributes == AttributeHandleSet{requesterAttribute});
    REQUIRE(variableLengthDataBytes(unavailable.userSuppliedTag) ==
            std::vector<unsigned char>(
                denialTagBytes, denialTagBytes + sizeof(denialTagBytes)));

    REQUIRE_NOTHROW(requester->resignFederationExecution(
        rti1516_2025::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS));
    requesterJoined = false;
    REQUIRE_NOTHROW(owner->resignFederationExecution(
        rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
    ownerJoined = false;
    REQUIRE_NOTHROW(owner->disconnect());
    REQUIRE_NOTHROW(requester->disconnect());
  } catch (...) {
    clientError = std::current_exception();
    if (requesterJoined) {
      try {
        requester->resignFederationExecution(
            rti1516_2025::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS);
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
    try {
      requester->disconnect();
    } catch (...) {
    }
    try {
      owner->disconnect();
    } catch (...) {
    }
  }
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
}

TEST_CASE(
    "RTIambassadors deliver Attribute Ownership Acquisition Cancellation through a configured process endpoint",
    "[integration][development-profile][federation-management][ownership-management]"
    "[transport][process-boundary][process-ownership-acquisition-cancellation][public-endpoint]"
    "[attribute-ownership-acquisition][rti.service.cancel-attribute-ownership-acquisition]"
    "[federate.callback.confirm-attribute-ownership-acquisition-cancellation][2025]") {
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
      auto ownerConnection = listener->accept(
          nullptr,
          {"process-ownership-acquisition-cancellation-server", 0xA709U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession ownerSession(ownerConnection);
      auto ownerHandler = service.handlerFor(ownerSession);
      auto serveExpected = [&](ProcessTransportSession& processSession,
                               auto const& handler,
                               umbra::detail::TransportServiceOperation operation,
                               char const* description) {
        if (!ProcessTransportServiceDispatcher::serveOne(
                processSession,
                [&](umbra::detail::TransportServiceMessage const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(description);
                  }
                  return handler(request);
                })) {
          throw std::runtime_error(description);
        }
      };

      using umbra::detail::TransportServiceOperation;
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::create_federation_execution,
          "The process ownership-acquisition cancellation server lost Create.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::join_federation_execution,
          "The process ownership-acquisition cancellation server lost owner Join.");

      auto requesterConnection = listener->accept(
          nullptr,
          {"process-ownership-acquisition-cancellation-server", 0xA70AU},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession requesterSession(requesterConnection);
      auto requesterHandler = service.handlerFor(requesterSession);
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::join_federation_execution,
          "The process ownership-acquisition cancellation server lost requester Join.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::get_object_class_handle,
          "The process ownership-acquisition cancellation server lost requester class lookup.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::get_attribute_handle,
          "The process ownership-acquisition cancellation server lost requester attribute lookup.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::subscribe_object_class_attributes,
          "The process ownership-acquisition cancellation server lost requester Subscribe.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::get_object_class_handle,
          "The process ownership-acquisition cancellation server lost owner class lookup.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::get_attribute_handle,
          "The process ownership-acquisition cancellation server lost owner attribute lookup.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The process ownership-acquisition cancellation server lost owner Publish.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::register_object_instance,
          "The process ownership-acquisition cancellation server lost owner Register.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::receive_interaction,
          "The process ownership-acquisition cancellation server lost requester discovery poll.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::receive_interaction,
          "The process ownership-acquisition cancellation server lost requester discovery drain.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The process ownership-acquisition cancellation server lost requester Publish.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::attribute_ownership_acquisition,
          "The process ownership-acquisition cancellation server lost Acquisition.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::cancel_attribute_ownership_acquisition,
          "The process ownership-acquisition cancellation server lost Cancellation.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::receive_interaction,
          "The process ownership-acquisition cancellation server lost requester confirmation poll.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::receive_interaction,
          "The process ownership-acquisition cancellation server lost requester confirmation drain.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::resign_federation_execution,
          "The process ownership-acquisition cancellation server lost requester Resign.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::resign_federation_execution,
          "The process ownership-acquisition cancellation server lost owner Resign.");
      service.detach(ownerSession);
      service.detach(requesterSession);
      ownerConnection->close();
      requesterConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  auto const federationName =
      nextFederationName() + L"-process-acquisition-cancellation";
  auto const fomModule = resourcePath("attribute-update-passel-fom.xml").wstring();
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto ownerConfiguration = rti1516_2025::RtiConfiguration::createConfiguration()
                                .withConfigurationName(
                                    L"process-ownership-acquisition-cancellation-owner")
                                .withRtiAddress(
                                    L"tcp://127.0.0.1:" + std::to_wstring(port));
  auto requesterConfiguration = rti1516_2025::RtiConfiguration::createConfiguration()
                                    .withConfigurationName(
                                        L"process-ownership-acquisition-cancellation-requester")
                                    .withRtiAddress(
                                        L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool ownerJoined = false;
  bool requesterJoined = false;
  try {
    REQUIRE_NOTHROW(owner->connect(
        ownerReports, HLA_EVOKED, ownerConfiguration));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName,
        fomModule));
    REQUIRE_NOTHROW(owner->joinFederationExecution(
        L"process-ownership-acquisition-cancellation-owner",
        L"publisher",
        federationName));
    ownerJoined = true;

    REQUIRE_NOTHROW(requester->connect(
        requesterReports, HLA_EVOKED, requesterConfiguration));
    REQUIRE_NOTHROW(requester->joinFederationExecution(
        L"process-ownership-acquisition-cancellation-requester",
        L"publisher",
        federationName));
    requesterJoined = true;

    auto const requesterClass = requester->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const requesterAttribute = requester->getAttributeHandle(
        requesterClass,
        fixture_hla::fixture::reliable_base_a);
    REQUIRE(requesterClass.isValid());
    REQUIRE(requesterAttribute.isValid());
    REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(
        requesterClass,
        AttributeHandleSet{requesterAttribute}));

    auto const ownerClass = owner->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const ownerAttribute = owner->getAttributeHandle(
        ownerClass,
        fixture_hla::fixture::reliable_base_a);
    REQUIRE(ownerClass.isValid());
    REQUIRE(ownerAttribute.isValid());
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
        ownerClass,
        AttributeHandleSet{ownerAttribute}));

    ObjectInstanceHandle objectInstance;
    REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(ownerClass));
    REQUIRE(objectInstance.isValid());
    REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
    REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(requesterReports.objectDiscoveryReports.front().objectInstance ==
            objectInstance);
    REQUIRE_NOTHROW(requester->publishObjectClassAttributes(
        requesterClass,
        AttributeHandleSet{requesterAttribute}));

    unsigned char const tagBytes[] = {0xC2, 0x11, 0x6D, 0x09};
    VariableLengthData const tag(tagBytes, sizeof(tagBytes));
    REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
        objectInstance,
        AttributeHandleSet{requesterAttribute},
        tag));
    REQUIRE(ownerReports.releaseRequests.empty());
    REQUIRE_NOTHROW(requester->cancelAttributeOwnershipAcquisition(
        objectInstance,
        AttributeHandleSet{requesterAttribute}));
    REQUIRE(ownerReports.releaseRequests.empty());
    REQUIRE(requesterReports.acquisitionCancellationReports.empty());
    REQUIRE_NOTHROW(requester->evokeMultipleCallbacks(0.0, 0.0));
    REQUIRE(requesterReports.acquisitionCancellationReports.size() == 1U);
    auto const& cancellation =
        requesterReports.acquisitionCancellationReports.front();
    REQUIRE(cancellation.objectInstance == objectInstance);
    REQUIRE(cancellation.attributes == AttributeHandleSet{requesterAttribute});
    REQUIRE(requesterReports.acquisitionReports.empty());

    REQUIRE_NOTHROW(requester->resignFederationExecution(
        rti1516_2025::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS));
    requesterJoined = false;
    REQUIRE_NOTHROW(owner->resignFederationExecution(
        rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
    ownerJoined = false;
    REQUIRE_NOTHROW(owner->disconnect());
    REQUIRE_NOTHROW(requester->disconnect());
  } catch (...) {
    clientError = std::current_exception();
    if (requesterJoined) {
      try {
        requester->resignFederationExecution(
            rti1516_2025::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS);
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
    try {
      requester->disconnect();
    } catch (...) {
    }
    try {
      owner->disconnect();
    } catch (...) {
    }
  }
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
}

TEST_CASE(
    "RTIambassadors cancel Negotiated Attribute Ownership Divestiture through a configured process endpoint",
    "[integration][development-profile][federation-management][ownership-management]"
    "[transport][process-boundary][process-negotiated-divestiture-cancellation][public-endpoint]"
    "[negotiated-attribute-ownership-divestiture][rti.service.cancel-negotiated-attribute-ownership-divestiture]"
    "[federate.callback.request-attribute-ownership-release][2025]") {
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
      auto ownerConnection = listener->accept(
          nullptr,
          {"process-negotiated-divestiture-cancellation-server", 0xA70BU},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession ownerSession(ownerConnection);
      auto ownerHandler = service.handlerFor(ownerSession);
       auto serveExpected = [&](ProcessTransportSession& processSession,
                               auto const& handler,
                               umbra::detail::TransportServiceOperation operation,
                               char const* description)
          -> umbra::detail::TransportServiceMessage {
        umbra::detail::TransportServiceMessage response;
        if (!ProcessTransportServiceDispatcher::serveOne(
                processSession,
                [&](umbra::detail::TransportServiceMessage const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(description);
                  }
                   response = handler(request);
                  return response;
                })) {
          throw std::runtime_error(description);
        }
        return response;
      };

      using umbra::detail::TransportServiceOperation;
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::create_federation_execution,
          "The process negotiated-divestiture cancellation server lost Create.");
       serveExpected(
           ownerSession,
           ownerHandler,
           TransportServiceOperation::join_federation_execution,
           "The process negotiated-divestiture cancellation server lost owner Join.");

      auto requesterConnection = listener->accept(
          nullptr,
          {"process-negotiated-divestiture-cancellation-server", 0xA70CU},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession requesterSession(requesterConnection);
      auto requesterHandler = service.handlerFor(requesterSession);
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::join_federation_execution,
          "The process negotiated-divestiture cancellation server lost requester Join.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::get_object_class_handle,
          "The process negotiated-divestiture cancellation server lost requester class lookup.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::get_attribute_handle,
          "The process negotiated-divestiture cancellation server lost requester attribute lookup.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::subscribe_object_class_attributes,
          "The process negotiated-divestiture cancellation server lost requester Subscribe.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::get_object_class_handle,
          "The process negotiated-divestiture cancellation server lost owner class lookup.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::get_attribute_handle,
          "The process negotiated-divestiture cancellation server lost owner attribute lookup.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The process negotiated-divestiture cancellation server lost owner Publish.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::register_object_instance,
          "The process negotiated-divestiture cancellation server lost owner Register.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::receive_interaction,
          "The process negotiated-divestiture cancellation server lost requester discovery poll.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::receive_interaction,
          "The process negotiated-divestiture cancellation server lost requester discovery drain.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The process negotiated-divestiture cancellation server lost requester Publish.");
       serveExpected(
           requesterSession,
           requesterHandler,
           TransportServiceOperation::attribute_ownership_acquisition,
           "The process negotiated-divestiture cancellation server lost Acquisition.");
       serveExpected(
           ownerSession,
           ownerHandler,
           TransportServiceOperation::negotiated_attribute_ownership_divestiture,
           "The process negotiated-divestiture cancellation server lost Negotiated Divestiture.");

       serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::
              cancel_negotiated_attribute_ownership_divestiture,
          "The process negotiated-divestiture cancellation server lost Cancellation.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::receive_interaction,
          "The process negotiated-divestiture cancellation server lost owner release poll.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::receive_interaction,
          "The process negotiated-divestiture cancellation server lost owner release drain.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::resign_federation_execution,
          "The process negotiated-divestiture cancellation server lost requester Resign.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::resign_federation_execution,
          "The process negotiated-divestiture cancellation server lost owner Resign.");
      service.detach(ownerSession);
      service.detach(requesterSession);
      ownerConnection->close();
      requesterConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  auto const federationName =
      nextFederationName() + L"-process-negotiated-divestiture-cancellation";
  auto const fomModule = resourcePath("attribute-update-passel-fom.xml").wstring();
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto ownerConfiguration = rti1516_2025::RtiConfiguration::createConfiguration()
                                .withConfigurationName(
                                    L"process-negotiated-divestiture-cancellation-owner")
                                .withRtiAddress(
                                    L"tcp://127.0.0.1:" + std::to_wstring(port));
  auto requesterConfiguration = rti1516_2025::RtiConfiguration::createConfiguration()
                                    .withConfigurationName(
                                        L"process-negotiated-divestiture-cancellation-requester")
                                    .withRtiAddress(
                                        L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool ownerJoined = false;
  bool requesterJoined = false;
  try {
    REQUIRE_NOTHROW(owner->connect(
        ownerReports, HLA_EVOKED, ownerConfiguration));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName,
        fomModule));
    REQUIRE_NOTHROW(owner->joinFederationExecution(
        L"process-negotiated-divestiture-cancellation-owner",
        L"publisher",
        federationName));
    ownerJoined = true;

    REQUIRE_NOTHROW(requester->connect(
        requesterReports, HLA_EVOKED, requesterConfiguration));
    REQUIRE_NOTHROW(requester->joinFederationExecution(
        L"process-negotiated-divestiture-cancellation-requester",
        L"publisher",
        federationName));
    requesterJoined = true;

    auto const requesterClass = requester->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const requesterAttribute = requester->getAttributeHandle(
        requesterClass,
        fixture_hla::fixture::reliable_base_a);
    REQUIRE(requesterClass.isValid());
    REQUIRE(requesterAttribute.isValid());
    REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(
        requesterClass,
        AttributeHandleSet{requesterAttribute}));

    auto const ownerClass = owner->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const ownerAttribute = owner->getAttributeHandle(
        ownerClass,
        fixture_hla::fixture::reliable_base_a);
    REQUIRE(ownerClass.isValid());
    REQUIRE(ownerAttribute.isValid());
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
        ownerClass,
        AttributeHandleSet{ownerAttribute}));

    ObjectInstanceHandle objectInstance;
    REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(ownerClass));
    REQUIRE(objectInstance.isValid());
    REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
    REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(requesterReports.objectDiscoveryReports.front().objectInstance ==
            objectInstance);
    REQUIRE_NOTHROW(requester->publishObjectClassAttributes(
        requesterClass,
        AttributeHandleSet{requesterAttribute}));

    unsigned char const tagBytes[] = {0xC2, 0x11, 0x6D, 0x09};
    VariableLengthData const tag(tagBytes, sizeof(tagBytes));
    REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
        objectInstance,
        AttributeHandleSet{requesterAttribute},
        tag));
    REQUIRE(ownerReports.releaseRequests.empty());
    REQUIRE_NOTHROW(owner->negotiatedAttributeOwnershipDivestiture(
        objectInstance,
        AttributeHandleSet{ownerAttribute},
        tag));
    REQUIRE_NOTHROW(owner->cancelNegotiatedAttributeOwnershipDivestiture(
        objectInstance,
        AttributeHandleSet{ownerAttribute}));
    REQUIRE(ownerReports.releaseRequests.empty());
    REQUIRE_NOTHROW(owner->evokeMultipleCallbacks(0.0, 0.0));
    REQUIRE(ownerReports.releaseRequests.size() == 1U);
    auto const& release = ownerReports.releaseRequests.front();
    REQUIRE(release.objectInstance == objectInstance);
    REQUIRE(release.attributes == AttributeHandleSet{ownerAttribute});
    REQUIRE(variableLengthDataBytes(release.userSuppliedTag) ==
            std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));

    REQUIRE_NOTHROW(requester->resignFederationExecution(
        rti1516_2025::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS));
    requesterJoined = false;
    REQUIRE_NOTHROW(owner->resignFederationExecution(
        rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
    ownerJoined = false;
    REQUIRE_NOTHROW(owner->disconnect());
    REQUIRE_NOTHROW(requester->disconnect());
  } catch (...) {
    clientError = std::current_exception();
    if (requesterJoined) {
      try {
        requester->resignFederationExecution(
            rti1516_2025::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS);
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
    try {
      requester->disconnect();
    } catch (...) {
    }
    try {
      owner->disconnect();
    } catch (...) {
    }
  }
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
}

TEST_CASE(
    "RTIambassadors deliver Confirm Divestiture through a configured process endpoint",
    "[integration][development-profile][federation-management][ownership-management]"
    "[transport][process-boundary][process-confirm-divestiture][public-endpoint]"
    "[negotiated-attribute-ownership-divestiture][confirm-divestiture]"
    "[rti.service.negotiated-attribute-ownership-divestiture]"
    "[rti.service.confirm-divestiture]"
    "[federate.callback.request-divestiture-confirmation]"
    "[federate.callback.attribute-ownership-acquisition-notification][2025]") {
  class ConfirmingFederateAmbassador final
      : public rti1516_2025::NullFederateAmbassador {
   public:
    struct RequestReport final {
      ObjectInstanceHandle objectInstance;
      AttributeHandleSet attributes;
      VariableLengthData userSuppliedTag;
    };

    struct AcquisitionReport final {
      ObjectInstanceHandle objectInstance;
      AttributeHandleSet attributes;
      VariableLengthData userSuppliedTag;
    };

    void requestDivestitureConfirmation(
        ObjectInstanceHandle const& objectInstance,
        AttributeHandleSet const& attributes,
        VariableLengthData const& userSuppliedTag) override {
      requests.push_back({objectInstance, attributes, userSuppliedTag});
      if (rti != nullptr) {
        unsigned char const confirmationTagBytes[] = {0xE1, 0x6A, 0xB4, 0x2D};
        VariableLengthData const confirmationTag(
            confirmationTagBytes, sizeof(confirmationTagBytes));
        rti->confirmDivestiture(
            objectInstance, attributes, confirmationTag);
        confirmed = true;
      }
    }

    void attributeOwnershipAcquisitionNotification(
        ObjectInstanceHandle const& objectInstance,
        AttributeHandleSet const& attributes,
        VariableLengthData const& userSuppliedTag) override {
      acquisitions.push_back({objectInstance, attributes, userSuppliedTag});
    }

    RTIambassador* rti = nullptr;
    bool confirmed = false;
    std::vector<RequestReport> requests;
    std::vector<AcquisitionReport> acquisitions;
  } ownerReports;
  ReportingFederateAmbassador requesterReports;

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
      auto ownerConnection = listener->accept(
          nullptr,
          {"process-confirm-divestiture-server", 0xA70DU},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession ownerSession(ownerConnection);
      auto ownerHandler = service.handlerFor(ownerSession);
      auto serveExpected = [&](ProcessTransportSession& processSession,
                               auto const& handler,
                               umbra::detail::TransportServiceOperation operation,
                               char const* description)
          -> umbra::detail::TransportServiceMessage {
        umbra::detail::TransportServiceMessage response;
        if (!ProcessTransportServiceDispatcher::serveOne(
                processSession,
                [&](umbra::detail::TransportServiceMessage const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(description);
                  }
                  response = handler(request);
                  return response;
                })) {
          throw std::runtime_error(description);
        }
        return response;
      };

      using umbra::detail::TransportServiceOperation;
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::create_federation_execution,
          "The process Confirm Divestiture server lost Create.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::join_federation_execution,
          "The process Confirm Divestiture server lost owner Join.");

      auto requesterConnection = listener->accept(
          nullptr,
          {"process-confirm-divestiture-server", 0xA70EU},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession requesterSession(requesterConnection);
      auto requesterHandler = service.handlerFor(requesterSession);
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::join_federation_execution,
          "The process Confirm Divestiture server lost requester Join.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::get_object_class_handle,
          "The process Confirm Divestiture server lost requester class lookup.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::get_attribute_handle,
          "The process Confirm Divestiture server lost requester attribute lookup.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::subscribe_object_class_attributes,
          "The process Confirm Divestiture server lost requester Subscribe.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::get_object_class_handle,
          "The process Confirm Divestiture server lost owner class lookup.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::get_attribute_handle,
          "The process Confirm Divestiture server lost owner attribute lookup.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The process Confirm Divestiture server lost owner Publish.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::register_object_instance,
          "The process Confirm Divestiture server lost owner Register.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::receive_interaction,
          "The process Confirm Divestiture server lost requester discovery poll.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::receive_interaction,
          "The process Confirm Divestiture server lost requester discovery drain.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The process Confirm Divestiture server lost requester Publish.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::attribute_ownership_acquisition,
          "The process Confirm Divestiture server lost Acquisition.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::negotiated_attribute_ownership_divestiture,
          "The process Confirm Divestiture server lost Negotiated Divestiture.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::receive_interaction,
          "The process Confirm Divestiture server lost owner confirmation poll.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::receive_interaction,
          "The process Confirm Divestiture server lost owner confirmation drain.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::confirm_divestiture,
          "The process Confirm Divestiture server lost Confirm Divestiture.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::receive_interaction,
          "The process Confirm Divestiture server lost requester acquisition poll.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::receive_interaction,
          "The process Confirm Divestiture server lost requester acquisition drain.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::resign_federation_execution,
          "The process Confirm Divestiture server lost requester Resign.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::resign_federation_execution,
          "The process Confirm Divestiture server lost owner Resign.");
      service.detach(ownerSession);
      service.detach(requesterSession);
      ownerConnection->close();
      requesterConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  auto const federationName =
      nextFederationName() + L"-process-confirm-divestiture";
  auto const fomModule = resourcePath("attribute-update-passel-fom.xml").wstring();
  auto owner = makeRti();
  auto requester = makeRti();
  auto ownerConfiguration = rti1516_2025::RtiConfiguration::createConfiguration()
                                .withConfigurationName(
                                    L"process-confirm-divestiture-owner")
                                .withRtiAddress(
                                    L"tcp://127.0.0.1:" + std::to_wstring(port));
  auto requesterConfiguration = rti1516_2025::RtiConfiguration::createConfiguration()
                                    .withConfigurationName(
                                        L"process-confirm-divestiture-requester")
                                    .withRtiAddress(
                                        L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool ownerJoined = false;
  bool requesterJoined = false;
  try {
    REQUIRE_NOTHROW(owner->connect(
        ownerReports, HLA_EVOKED, ownerConfiguration));
    ownerReports.rti = owner.get();
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName,
        fomModule));
    REQUIRE_NOTHROW(owner->joinFederationExecution(
        L"process-confirm-divestiture-owner",
        L"publisher",
        federationName));
    ownerJoined = true;

    REQUIRE_NOTHROW(requester->connect(
        requesterReports, HLA_EVOKED, requesterConfiguration));
    REQUIRE_NOTHROW(requester->joinFederationExecution(
        L"process-confirm-divestiture-requester",
        L"publisher",
        federationName));
    requesterJoined = true;

    auto const requesterClass = requester->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const requesterAttribute = requester->getAttributeHandle(
        requesterClass,
        fixture_hla::fixture::reliable_base_a);
    REQUIRE(requesterClass.isValid());
    REQUIRE(requesterAttribute.isValid());
    REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(
        requesterClass,
        AttributeHandleSet{requesterAttribute}));

    auto const ownerClass = owner->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const ownerAttribute = owner->getAttributeHandle(
        ownerClass,
        fixture_hla::fixture::reliable_base_a);
    REQUIRE(ownerClass.isValid());
    REQUIRE(ownerAttribute.isValid());
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
        ownerClass,
        AttributeHandleSet{ownerAttribute}));

    ObjectInstanceHandle objectInstance;
    REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(ownerClass));
    REQUIRE(objectInstance.isValid());
    REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
    REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(requesterReports.objectDiscoveryReports.front().objectInstance ==
            objectInstance);
    REQUIRE_NOTHROW(requester->publishObjectClassAttributes(
        requesterClass,
        AttributeHandleSet{requesterAttribute}));

    unsigned char const negotiatedTagBytes[] = {0xC2, 0x11, 0x6D, 0x09};
    VariableLengthData const negotiatedTag(
        negotiatedTagBytes, sizeof(negotiatedTagBytes));
    REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
        objectInstance,
        AttributeHandleSet{requesterAttribute},
        negotiatedTag));
    REQUIRE(ownerReports.requests.empty());
    REQUIRE_NOTHROW(owner->negotiatedAttributeOwnershipDivestiture(
        objectInstance,
        AttributeHandleSet{ownerAttribute},
        negotiatedTag));
    REQUIRE(ownerReports.requests.empty());
    REQUIRE_NOTHROW(owner->evokeMultipleCallbacks(0.0, 0.0));
    REQUIRE(ownerReports.requests.size() == 1U);
    REQUIRE(ownerReports.requests.front().objectInstance == objectInstance);
    REQUIRE(ownerReports.requests.front().attributes ==
            AttributeHandleSet{ownerAttribute});
    REQUIRE(variableLengthDataBytes(
                ownerReports.requests.front().userSuppliedTag) ==
            std::vector<unsigned char>(
                negotiatedTagBytes,
                negotiatedTagBytes + sizeof(negotiatedTagBytes)));
    REQUIRE(ownerReports.confirmed);
    REQUIRE(requesterReports.acquisitionReports.empty());

    REQUIRE_NOTHROW(requester->evokeMultipleCallbacks(0.0, 0.0));
    REQUIRE(requesterReports.acquisitionReports.size() == 1U);
    REQUIRE(requesterReports.acquisitionReports.front().kind ==
            ReportingFederateAmbassador::AcquisitionReport::Kind::notification);
    REQUIRE(requesterReports.acquisitionReports.front().objectInstance ==
            objectInstance);
    REQUIRE(requesterReports.acquisitionReports.front().attributes ==
            AttributeHandleSet{requesterAttribute});
    unsigned char const confirmationTagBytes[] = {0xE1, 0x6A, 0xB4, 0x2D};
    REQUIRE(variableLengthDataBytes(
                requesterReports.acquisitionReports.front().userSuppliedTag) ==
            std::vector<unsigned char>(
                confirmationTagBytes,
                confirmationTagBytes + sizeof(confirmationTagBytes)));

    REQUIRE_NOTHROW(requester->resignFederationExecution(
        rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES));
    requesterJoined = false;
    REQUIRE_NOTHROW(owner->resignFederationExecution(
        rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
    ownerJoined = false;
    REQUIRE_NOTHROW(owner->disconnect());
    REQUIRE_NOTHROW(requester->disconnect());
  } catch (...) {
    clientError = std::current_exception();
    if (requesterJoined) {
      try {
        requester->resignFederationExecution(
            rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES);
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
    try {
      requester->disconnect();
    } catch (...) {
    }
    try {
      owner->disconnect();
    } catch (...) {
    }
  }
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
}

TEST_CASE(
    "RTIambassadors deliver Confirm Divestiture through a configured process endpoint in push receive mode",
    "[integration][development-profile][federation-management][ownership-management]"
    "[transport][process-boundary][process-confirm-divestiture-push][public-endpoint]"
    "[negotiated-attribute-ownership-divestiture][confirm-divestiture]"
    "[rti.service.negotiated-attribute-ownership-divestiture]"
    "[rti.service.confirm-divestiture]"
    "[federate.callback.request-divestiture-confirmation]"
    "[federate.callback.attribute-ownership-acquisition-notification][2025]") {
  class ConfirmingFederateAmbassador final
      : public rti1516_2025::NullFederateAmbassador {
   public:
    struct RequestReport final {
      ObjectInstanceHandle objectInstance;
      AttributeHandleSet attributes;
      VariableLengthData userSuppliedTag;
    };

    void requestDivestitureConfirmation(
        ObjectInstanceHandle const& objectInstance,
        AttributeHandleSet const& attributes,
        VariableLengthData const& userSuppliedTag) override {
      requests.push_back({objectInstance, attributes, userSuppliedTag});
      if (rti != nullptr) {
        unsigned char const confirmationTagBytes[] = {0xE1, 0x6A, 0xB4, 0x2D};
        VariableLengthData const confirmationTag(
            confirmationTagBytes, sizeof(confirmationTagBytes));
        rti->confirmDivestiture(
            objectInstance, attributes, confirmationTag);
        confirmed = true;
      }
    }

    RTIambassador* rti = nullptr;
    bool confirmed = false;
    std::vector<RequestReport> requests;
  } ownerReports;
  ReportingFederateAmbassador requesterReports;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener != nullptr);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationServiceOptions serviceOptions;
      serviceOptions.pushReceiveOrderEvents = true;
      ProcessFederationService service(
          registry, composedProcessOwnershipDefinition(), serviceOptions);

      auto ownerConnection = listener->accept(
          nullptr,
          {"process-confirm-divestiture-push-server", 0xA71DU},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession ownerSession(ownerConnection);
      auto ownerHandler = service.handlerFor(ownerSession);
      auto serveExpected = [&](ProcessTransportSession& processSession,
                               auto const& handler,
                               umbra::detail::TransportServiceOperation operation,
                               char const* description) {
        umbra::detail::TransportServiceMessage response;
        if (!ProcessTransportServiceDispatcher::serveOne(
                processSession,
                [&](umbra::detail::TransportServiceMessage const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(description);
                  }
                  response = handler(request);
                  return response;
                })) {
          throw std::runtime_error(description);
        }
        if (response.status != umbra::detail::TransportServiceStatus::ok) {
          throw std::runtime_error(description);
        }
      };

      using umbra::detail::TransportServiceOperation;
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::create_federation_execution,
          "The push Confirm Divestiture server lost Create.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::join_federation_execution,
          "The push Confirm Divestiture server lost owner Join.");

      auto requesterConnection = listener->accept(
          nullptr,
          {"process-confirm-divestiture-push-server", 0xA71EU},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession requesterSession(requesterConnection);
      auto requesterHandler = service.handlerFor(requesterSession);
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::join_federation_execution,
          "The push Confirm Divestiture server lost requester Join.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::get_object_class_handle,
          "The push Confirm Divestiture server lost requester class lookup.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::get_attribute_handle,
          "The push Confirm Divestiture server lost requester attribute lookup.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::subscribe_object_class_attributes,
          "The push Confirm Divestiture server lost requester Subscribe.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::get_object_class_handle,
          "The push Confirm Divestiture server lost owner class lookup.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::get_attribute_handle,
          "The push Confirm Divestiture server lost owner attribute lookup.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The push Confirm Divestiture server lost owner Publish.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::register_object_instance,
          "The push Confirm Divestiture server lost owner Register.");
      // In push mode the discovery event is already on the requester socket;
      // this ordinary lookup is the HLA_IMMEDIATE polling fence that receives
      // and dispatches it before returning its handle.
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::get_object_class_handle,
          "The push Confirm Divestiture server lost requester discovery fence.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The push Confirm Divestiture server lost requester Publish.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::attribute_ownership_acquisition,
          "The push Confirm Divestiture server lost Acquisition.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::negotiated_attribute_ownership_divestiture,
          "The push Confirm Divestiture server lost Negotiated Divestiture.");
      // The confirmation request is pushed before the negotiated response is
      // returned.  The client dispatches it after that response, then the
      // re-entrant public callback issues this request on the same session.
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::confirm_divestiture,
          "The push Confirm Divestiture server lost Confirm Divestiture.");
      // Confirm emits the acquisition notification directly to the requester;
      // its next ordinary lookup is the second HLA_IMMEDIATE receive fence.
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::get_object_class_handle,
          "The push Confirm Divestiture server lost requester acquisition fence.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::resign_federation_execution,
          "The push Confirm Divestiture server lost requester Resign.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::resign_federation_execution,
          "The push Confirm Divestiture server lost owner Resign.");
      service.detach(ownerSession);
      service.detach(requesterSession);
      ownerConnection->close();
      requesterConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  auto owner = makeRti();
  auto requester = makeRti();
  auto ownerConfiguration = rti1516_2025::RtiConfiguration::createConfiguration()
                                .withConfigurationName(
                                    L"process-confirm-divestiture-push-owner")
                                .withRtiAddress(
                                    L"tcp://127.0.0.1:" + std::to_wstring(port));
  auto requesterConfiguration = rti1516_2025::RtiConfiguration::createConfiguration()
                                    .withConfigurationName(
                                        L"process-confirm-divestiture-push-requester")
                                    .withRtiAddress(
                                        L"tcp://127.0.0.1:" + std::to_wstring(port));
  auto const federationName =
      nextFederationName() + L"-process-confirm-divestiture-push";
  auto const fomModule = resourcePath("attribute-update-passel-fom.xml").wstring();
  std::exception_ptr clientError;
  bool ownerJoined = false;
  bool requesterJoined = false;
  try {
    REQUIRE_NOTHROW(owner->connect(
        ownerReports, HLA_IMMEDIATE, ownerConfiguration));
    ownerReports.rti = owner.get();
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName, fomModule));
    REQUIRE_NOTHROW(owner->joinFederationExecution(
        L"process-confirm-divestiture-push-owner",
        L"publisher",
        federationName));
    ownerJoined = true;

    REQUIRE_NOTHROW(requester->connect(
        requesterReports, HLA_IMMEDIATE, requesterConfiguration));
    REQUIRE_NOTHROW(requester->joinFederationExecution(
        L"process-confirm-divestiture-push-requester",
        L"publisher",
        federationName));
    requesterJoined = true;

    auto const requesterClass = requester->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const requesterAttribute = requester->getAttributeHandle(
        requesterClass,
        fixture_hla::fixture::reliable_base_a);
    REQUIRE(requesterClass.isValid());
    REQUIRE(requesterAttribute.isValid());
    REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(
        requesterClass,
        AttributeHandleSet{requesterAttribute}));

    auto const ownerClass = owner->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const ownerAttribute = owner->getAttributeHandle(
        ownerClass,
        fixture_hla::fixture::reliable_base_a);
    REQUIRE(ownerClass.isValid());
    REQUIRE(ownerAttribute.isValid());
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
        ownerClass,
        AttributeHandleSet{ownerAttribute}));

    ObjectInstanceHandle objectInstance;
    REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(ownerClass));
    REQUIRE(objectInstance.isValid());
    REQUIRE(requesterReports.objectDiscoveryReports.empty());
    // The push discovery frame is consumed while this lookup response is in
    // flight, so HLA_IMMEDIATE dispatches it before the call returns.
    REQUIRE_NOTHROW(static_cast<void>(requester->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child)));
    REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(requesterReports.objectDiscoveryReports.front().objectInstance ==
            objectInstance);
    REQUIRE(requesterReports.objectDiscoveryReports.front().objectClass ==
            requesterClass);

    REQUIRE_NOTHROW(requester->publishObjectClassAttributes(
        requesterClass,
        AttributeHandleSet{requesterAttribute}));
    unsigned char const negotiatedTagBytes[] = {0xC2, 0x11, 0x6D, 0x09};
    VariableLengthData const negotiatedTag(
        negotiatedTagBytes, sizeof(negotiatedTagBytes));
    REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
        objectInstance,
        AttributeHandleSet{requesterAttribute},
        negotiatedTag));
    REQUIRE(ownerReports.requests.empty());
    REQUIRE_NOTHROW(owner->negotiatedAttributeOwnershipDivestiture(
        objectInstance,
        AttributeHandleSet{ownerAttribute},
        negotiatedTag));
    REQUIRE(ownerReports.requests.size() == 1U);
    REQUIRE(ownerReports.requests.front().objectInstance == objectInstance);
    REQUIRE(ownerReports.requests.front().attributes ==
            AttributeHandleSet{ownerAttribute});
    REQUIRE(variableLengthDataBytes(
                ownerReports.requests.front().userSuppliedTag) ==
            std::vector<unsigned char>(
                negotiatedTagBytes,
                negotiatedTagBytes + sizeof(negotiatedTagBytes)));
    REQUIRE(ownerReports.confirmed);
    REQUIRE(requesterReports.acquisitionReports.empty());

    // Confirm was pushed to the requester by the owner's callback-side
    // confirmDivestiture request.  The ordinary lookup is the push fence and
    // dispatches the official acquisition notification synchronously.
    REQUIRE_NOTHROW(static_cast<void>(requester->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child)));
    REQUIRE(requesterReports.acquisitionReports.size() == 1U);
    REQUIRE(requesterReports.acquisitionReports.front().kind ==
            ReportingFederateAmbassador::AcquisitionReport::Kind::notification);
    REQUIRE(requesterReports.acquisitionReports.front().objectInstance ==
            objectInstance);
    REQUIRE(requesterReports.acquisitionReports.front().attributes ==
            AttributeHandleSet{requesterAttribute});
    unsigned char const confirmationTagBytes[] = {0xE1, 0x6A, 0xB4, 0x2D};
    REQUIRE(variableLengthDataBytes(
                requesterReports.acquisitionReports.front().userSuppliedTag) ==
            std::vector<unsigned char>(
                confirmationTagBytes,
                confirmationTagBytes + sizeof(confirmationTagBytes)));

    REQUIRE_NOTHROW(requester->resignFederationExecution(
        rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES));
    requesterJoined = false;
    REQUIRE_NOTHROW(owner->resignFederationExecution(
        rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
    ownerJoined = false;
    REQUIRE_NOTHROW(owner->disconnect());
    REQUIRE_NOTHROW(requester->disconnect());
  } catch (...) {
    clientError = std::current_exception();
    if (requesterJoined) {
      try {
        requester->resignFederationExecution(
            rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES);
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
    try {
      requester->disconnect();
    } catch (...) {
    }
    try {
      owner->disconnect();
    } catch (...) {
    }
  }
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
}

TEST_CASE(
    "RTIambassadors continue Confirm Divestiture with a pushed assumption candidate",
    "[integration][development-profile][federation-management][ownership-management]"
    "[transport][process-boundary][process-confirm-divestiture-assumption][public-endpoint]"
    "[negotiated-attribute-ownership-divestiture][attribute-ownership-acquisition][confirm-divestiture]"
    "[rti.service.negotiated-attribute-ownership-divestiture]"
    "[rti.service.attribute-ownership-acquisition]"
    "[rti.service.confirm-divestiture]"
    "[federate.callback.request-attribute-ownership-assumption]"
    "[federate.callback.request-divestiture-confirmation]"
    "[federate.callback.attribute-ownership-acquisition-notification][2025]") {
  class ConfirmingFederateAmbassador final
      : public rti1516_2025::NullFederateAmbassador {
   public:
    struct RequestReport final {
      ObjectInstanceHandle objectInstance;
      AttributeHandleSet attributes;
      VariableLengthData userSuppliedTag;
    };

    void requestDivestitureConfirmation(
        ObjectInstanceHandle const& objectInstance,
        AttributeHandleSet const& attributes,
        VariableLengthData const& userSuppliedTag) override {
      requests.push_back({objectInstance, attributes, userSuppliedTag});
    }

    std::vector<RequestReport> requests;
  } ownerReports;

  class AssumingFederateAmbassador final
      : public rti1516_2025::NullFederateAmbassador {
   public:
    struct DiscoveryReport final {
      ObjectInstanceHandle objectInstance;
      ObjectClassHandle objectClass;
    };

    struct AssumptionReport final {
      ObjectInstanceHandle objectInstance;
      AttributeHandleSet attributes;
      VariableLengthData userSuppliedTag;
    };

    void discoverObjectInstance(
        ObjectInstanceHandle const& objectInstance,
        ObjectClassHandle const& objectClass,
        std::wstring const&,
        FederateHandle const&) override {
      discoveries.push_back({objectInstance, objectClass});
    }

    void requestAttributeOwnershipAssumption(
        ObjectInstanceHandle const& objectInstance,
        AttributeHandleSet const& offeredAttributes,
        VariableLengthData const& userSuppliedTag) override {
      assumptions.push_back({objectInstance, offeredAttributes, userSuppliedTag});
    }

    std::vector<DiscoveryReport> discoveries;
    std::vector<AssumptionReport> assumptions;
  } candidateReports;
  ReportingFederateAmbassador requesterReports;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener != nullptr);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationServiceOptions serviceOptions;
      serviceOptions.pushReceiveOrderEvents = true;
      ProcessFederationService service(
          registry, composedProcessOwnershipDefinition(), serviceOptions);

      auto ownerConnection = listener->accept(
          nullptr,
          {"process-confirm-divestiture-assumption-server", 0xA72DU},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession ownerSession(ownerConnection);
      auto ownerHandler = service.handlerFor(ownerSession);
      auto serveExpected = [&](ProcessTransportSession& processSession,
                               auto const& handler,
                               umbra::detail::TransportServiceOperation operation,
                               char const* description) {
        umbra::detail::TransportServiceMessage response;
        if (!ProcessTransportServiceDispatcher::serveOne(
                processSession,
                [&](umbra::detail::TransportServiceMessage const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(description);
                  }
                  response = handler(request);
                  return response;
                })) {
          throw std::runtime_error(description);
        }
        if (response.status != umbra::detail::TransportServiceStatus::ok) {
          throw std::runtime_error(description);
        }
      };

      using umbra::detail::TransportServiceOperation;
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::create_federation_execution,
          "The process Confirm Divestiture assumption server lost Create.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::join_federation_execution,
          "The process Confirm Divestiture assumption server lost owner Join.");

      auto requesterConnection = listener->accept(
          nullptr,
          {"process-confirm-divestiture-assumption-server", 0xA72EU},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession requesterSession(requesterConnection);
      auto requesterHandler = service.handlerFor(requesterSession);
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::join_federation_execution,
          "The process Confirm Divestiture assumption server lost requester Join.");

      auto candidateConnection = listener->accept(
          nullptr,
          {"process-confirm-divestiture-assumption-server", 0xA72FU},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession candidateSession(candidateConnection);
      auto candidateHandler = service.handlerFor(candidateSession);
      serveExpected(
          candidateSession,
          candidateHandler,
          TransportServiceOperation::join_federation_execution,
          "The process Confirm Divestiture assumption server lost candidate Join.");
      serveExpected(
          candidateSession,
          candidateHandler,
          TransportServiceOperation::get_object_class_handle,
          "The process Confirm Divestiture assumption server lost candidate class lookup.");
      serveExpected(
          candidateSession,
          candidateHandler,
          TransportServiceOperation::get_attribute_handle,
          "The process Confirm Divestiture assumption server lost candidate attribute lookup.");
      serveExpected(
          candidateSession,
          candidateHandler,
          TransportServiceOperation::subscribe_object_class_attributes,
          "The process Confirm Divestiture assumption server lost candidate Subscribe.");
      serveExpected(
          candidateSession,
          candidateHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The process Confirm Divestiture assumption server lost candidate Publish.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::get_object_class_handle,
          "The process Confirm Divestiture assumption server lost requester class lookup.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::get_attribute_handle,
          "The process Confirm Divestiture assumption server lost requester attribute lookup.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::subscribe_object_class_attributes,
          "The process Confirm Divestiture assumption server lost requester Subscribe.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::get_object_class_handle,
          "The process Confirm Divestiture assumption server lost owner class lookup.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::get_attribute_handle,
          "The process Confirm Divestiture assumption server lost owner attribute lookup.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The process Confirm Divestiture assumption server lost owner Publish.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::register_object_instance,
          "The process Confirm Divestiture assumption server lost owner Register.");
      serveExpected(
          candidateSession,
          candidateHandler,
          TransportServiceOperation::get_object_class_handle,
          "The process Confirm Divestiture assumption server lost candidate discovery fence.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::get_object_class_handle,
          "The process Confirm Divestiture assumption server lost requester discovery fence.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The process Confirm Divestiture assumption server lost requester Publish.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::attribute_ownership_acquisition,
          "The process Confirm Divestiture assumption server lost Acquisition.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::negotiated_attribute_ownership_divestiture,
          "The process Confirm Divestiture assumption server lost Negotiated Divestiture.");
      serveExpected(
          candidateSession,
          candidateHandler,
          TransportServiceOperation::get_object_class_handle,
          "The process Confirm Divestiture assumption server lost candidate assumption fence.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::confirm_divestiture,
          "The process Confirm Divestiture assumption server lost Confirm Divestiture.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::get_object_class_handle,
          "The process Confirm Divestiture assumption server lost requester acquisition fence.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::resign_federation_execution,
          "The process Confirm Divestiture assumption server lost requester Resign.");
      serveExpected(
          candidateSession,
          candidateHandler,
          TransportServiceOperation::resign_federation_execution,
          "The process Confirm Divestiture assumption server lost candidate Resign.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::resign_federation_execution,
          "The process Confirm Divestiture assumption server lost owner Resign.");
      service.detach(ownerSession);
      service.detach(requesterSession);
      service.detach(candidateSession);
      ownerConnection->close();
      requesterConnection->close();
      candidateConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  auto owner = makeRti();
  auto requester = makeRti();
  auto candidate = makeRti();
  auto ownerConfiguration = rti1516_2025::RtiConfiguration::createConfiguration()
                                .withConfigurationName(
                                    L"process-confirm-divestiture-assumption-owner")
                                .withRtiAddress(
                                    L"tcp://127.0.0.1:" + std::to_wstring(port));
  auto requesterConfiguration = rti1516_2025::RtiConfiguration::createConfiguration()
                                    .withConfigurationName(
                                        L"process-confirm-divestiture-assumption-requester")
                                    .withRtiAddress(
                                        L"tcp://127.0.0.1:" + std::to_wstring(port));
  auto candidateConfiguration = rti1516_2025::RtiConfiguration::createConfiguration()
                                    .withConfigurationName(
                                        L"process-confirm-divestiture-assumption-candidate")
                                    .withRtiAddress(
                                        L"tcp://127.0.0.1:" + std::to_wstring(port));
  auto const federationName =
      nextFederationName() + L"-process-confirm-divestiture-assumption";
  auto const fomModule = resourcePath("attribute-update-passel-fom.xml").wstring();
  std::exception_ptr clientError;
  bool ownerJoined = false;
  bool requesterJoined = false;
  bool candidateJoined = false;
  try {
    REQUIRE_NOTHROW(owner->connect(
        ownerReports, HLA_IMMEDIATE, ownerConfiguration));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName, fomModule));
    REQUIRE_NOTHROW(owner->joinFederationExecution(
        L"process-confirm-divestiture-assumption-owner",
        L"publisher",
        federationName));
    ownerJoined = true;

    REQUIRE_NOTHROW(requester->connect(
        requesterReports, HLA_IMMEDIATE, requesterConfiguration));
    REQUIRE_NOTHROW(requester->joinFederationExecution(
        L"process-confirm-divestiture-assumption-requester",
        L"publisher",
        federationName));
    requesterJoined = true;

    REQUIRE_NOTHROW(candidate->connect(
        candidateReports, HLA_IMMEDIATE, candidateConfiguration));
    REQUIRE_NOTHROW(candidate->joinFederationExecution(
        L"process-confirm-divestiture-assumption-candidate",
        L"publisher",
        federationName));
    candidateJoined = true;

    auto const candidateClass = candidate->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const candidateAttribute = candidate->getAttributeHandle(
        candidateClass,
        fixture_hla::fixture::reliable_base_a);
    REQUIRE(candidateClass.isValid());
    REQUIRE(candidateAttribute.isValid());
    REQUIRE_NOTHROW(candidate->subscribeObjectClassAttributes(
        candidateClass,
        AttributeHandleSet{candidateAttribute}));
    REQUIRE_NOTHROW(candidate->publishObjectClassAttributes(
        candidateClass,
        AttributeHandleSet{candidateAttribute}));

    auto const requesterClass = requester->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const requesterAttribute = requester->getAttributeHandle(
        requesterClass,
        fixture_hla::fixture::reliable_base_a);
    REQUIRE(requesterClass.isValid());
    REQUIRE(requesterAttribute.isValid());
    REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(
        requesterClass,
        AttributeHandleSet{requesterAttribute}));

    auto const ownerClass = owner->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const ownerAttribute = owner->getAttributeHandle(
        ownerClass,
        fixture_hla::fixture::reliable_base_a);
    REQUIRE(ownerClass.isValid());
    REQUIRE(ownerAttribute.isValid());
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
        ownerClass,
        AttributeHandleSet{ownerAttribute}));

    ObjectInstanceHandle objectInstance;
    REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(ownerClass));
    REQUIRE(objectInstance.isValid());
    REQUIRE(candidateReports.discoveries.empty());
    REQUIRE_NOTHROW(static_cast<void>(candidate->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child)));
    REQUIRE(candidateReports.discoveries.size() == 1U);
    REQUIRE(candidateReports.discoveries.front().objectInstance == objectInstance);
    REQUIRE(candidateReports.discoveries.front().objectClass == candidateClass);
    REQUIRE(requesterReports.objectDiscoveryReports.empty());
    REQUIRE_NOTHROW(static_cast<void>(requester->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child)));
    REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(requesterReports.objectDiscoveryReports.front().objectInstance == objectInstance);
    REQUIRE(requesterReports.objectDiscoveryReports.front().objectClass == requesterClass);

    REQUIRE_NOTHROW(requester->publishObjectClassAttributes(
        requesterClass,
        AttributeHandleSet{requesterAttribute}));
    unsigned char const negotiatedTagBytes[] = {0xD2, 0x11, 0x6D, 0x09};
    VariableLengthData const negotiatedTag(
        negotiatedTagBytes, sizeof(negotiatedTagBytes));
    REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
        objectInstance,
        AttributeHandleSet{requesterAttribute},
        negotiatedTag));
    REQUIRE(ownerReports.requests.empty());
    REQUIRE_NOTHROW(owner->negotiatedAttributeOwnershipDivestiture(
        objectInstance,
        AttributeHandleSet{ownerAttribute},
        negotiatedTag));
    REQUIRE(ownerReports.requests.size() == 1U);
    REQUIRE(ownerReports.requests.front().objectInstance == objectInstance);
    REQUIRE(ownerReports.requests.front().attributes ==
            AttributeHandleSet{ownerAttribute});
    REQUIRE(variableLengthDataBytes(
                ownerReports.requests.front().userSuppliedTag) ==
            std::vector<unsigned char>(
                negotiatedTagBytes,
                negotiatedTagBytes + sizeof(negotiatedTagBytes)));
    REQUIRE(candidateReports.assumptions.empty());

    // The candidate's pushed assumption frame is fenced independently while
    // the owner confirmation remains pending. This proves the assumption
    // callback is not lost when a regular acquisition is also waiting.
    REQUIRE_NOTHROW(static_cast<void>(candidate->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child)));
    REQUIRE(candidateReports.assumptions.size() == 1U);
    REQUIRE(candidateReports.assumptions.front().objectInstance == objectInstance);
    REQUIRE(candidateReports.assumptions.front().attributes ==
            AttributeHandleSet{candidateAttribute});
    REQUIRE(variableLengthDataBytes(
                candidateReports.assumptions.front().userSuppliedTag) ==
            std::vector<unsigned char>(
                negotiatedTagBytes,
                negotiatedTagBytes + sizeof(negotiatedTagBytes)));

    unsigned char const confirmationTagBytes[] = {0xE2, 0x6A, 0xB4, 0x2D};
    VariableLengthData const confirmationTag(
        confirmationTagBytes, sizeof(confirmationTagBytes));
    REQUIRE_NOTHROW(owner->confirmDivestiture(
        objectInstance,
        AttributeHandleSet{ownerAttribute},
        confirmationTag));
    REQUIRE(requesterReports.acquisitionReports.empty());
    REQUIRE_NOTHROW(static_cast<void>(requester->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child)));
    REQUIRE(requesterReports.acquisitionReports.size() == 1U);
    REQUIRE(requesterReports.acquisitionReports.front().kind ==
            ReportingFederateAmbassador::AcquisitionReport::Kind::notification);
    REQUIRE(requesterReports.acquisitionReports.front().objectInstance == objectInstance);
    REQUIRE(requesterReports.acquisitionReports.front().attributes ==
            AttributeHandleSet{requesterAttribute});
    REQUIRE(variableLengthDataBytes(
                requesterReports.acquisitionReports.front().userSuppliedTag) ==
            std::vector<unsigned char>(
                confirmationTagBytes,
                confirmationTagBytes + sizeof(confirmationTagBytes)));

    REQUIRE_NOTHROW(requester->resignFederationExecution(
        rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES));
    requesterJoined = false;
    REQUIRE_NOTHROW(candidate->resignFederationExecution(
        rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES));
    candidateJoined = false;
    REQUIRE_NOTHROW(owner->resignFederationExecution(
        rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
    ownerJoined = false;
    REQUIRE_NOTHROW(owner->disconnect());
    REQUIRE_NOTHROW(requester->disconnect());
    REQUIRE_NOTHROW(candidate->disconnect());
  } catch (...) {
    clientError = std::current_exception();
    if (candidateJoined) {
      try {
        candidate->resignFederationExecution(
            rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES);
      } catch (...) {
      }
    }
    if (requesterJoined) {
      try {
        requester->resignFederationExecution(
            rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES);
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
    try {
      owner->disconnect();
    } catch (...) {
    }
    try {
      requester->disconnect();
    } catch (...) {
    }
    try {
      candidate->disconnect();
    } catch (...) {
    }
  }
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
}
#endif
