#include <catch2/catch_test_macros.hpp>

#include "hla_test_names.hpp"
#include "ieee1516_2025_attribute_ownership_test_support.hpp"

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
#endif

#include <array>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
namespace {

namespace fixture_hla = umbra::test::hla::wide;

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
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;
using umbra::detail::EmbeddedFederationRegistry;
using umbra::detail::ProcessFederationService;
using umbra::detail::ProcessFederationServiceOptions;
using umbra::detail::ProcessTransportListener;
using umbra::detail::ProcessTransportServiceDispatcher;
using umbra::detail::ProcessTransportSession;
using umbra::test::attribute_ownership_2025::ReportingFederateAmbassador;
using umbra::test::attribute_ownership_2025::composedProcessOwnershipDefinition;
using umbra::test::attribute_ownership_2025::makeRti;
using umbra::test::attribute_ownership_2025::nextFederationName;
using umbra::test::attribute_ownership_2025::resourcePath;
using umbra::test::attribute_ownership_2025::variableLengthDataBytes;

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
          TransportServiceOperation::get_attribute_handle,
          "The process negotiated-divestiture cancellation server lost requester second-attribute lookup.");
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
          TransportServiceOperation::get_attribute_handle,
          "The process negotiated-divestiture cancellation server lost owner second-attribute lookup.");
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
          TransportServiceOperation::is_attribute_owned_by_federate,
          "The process negotiated-divestiture cancellation server lost the owner-state query.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::is_attribute_owned_by_federate,
          "The process negotiated-divestiture cancellation server lost the owner's second-attribute state query.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::is_attribute_owned_by_federate,
          "The process negotiated-divestiture cancellation server lost the requester-state query.");
      serveExpected(
          requesterSession,
          requesterHandler,
          TransportServiceOperation::is_attribute_owned_by_federate,
          "The process negotiated-divestiture cancellation server lost the requester's second-attribute state query.");
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
    auto const requesterSecondAttribute = requester->getAttributeHandle(
        requesterClass,
        fixture_hla::fixture::reliable_base_b);
    REQUIRE(requesterClass.isValid());
    REQUIRE(requesterAttribute.isValid());
    REQUIRE(requesterSecondAttribute.isValid());
    REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(
        requesterClass,
        AttributeHandleSet{requesterAttribute, requesterSecondAttribute}));

    auto const ownerClass = owner->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const ownerAttribute = owner->getAttributeHandle(
        ownerClass,
        fixture_hla::fixture::reliable_base_a);
    auto const ownerSecondAttribute = owner->getAttributeHandle(
        ownerClass,
        fixture_hla::fixture::reliable_base_b);
    REQUIRE(ownerClass.isValid());
    REQUIRE(ownerAttribute.isValid());
    REQUIRE(ownerSecondAttribute.isValid());
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
        ownerClass,
        AttributeHandleSet{ownerAttribute, ownerSecondAttribute}));

    ObjectInstanceHandle objectInstance;
    REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(ownerClass));
    REQUIRE(objectInstance.isValid());
    REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
    REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(requesterReports.objectDiscoveryReports.front().objectInstance ==
            objectInstance);
    REQUIRE_NOTHROW(requester->publishObjectClassAttributes(
        requesterClass,
        AttributeHandleSet{requesterAttribute, requesterSecondAttribute}));

    unsigned char const tagBytes[] = {0xC2, 0x11, 0x6D, 0x09};
    VariableLengthData const tag(tagBytes, sizeof(tagBytes));
    REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
        objectInstance,
        AttributeHandleSet{requesterAttribute, requesterSecondAttribute},
        tag));
    REQUIRE(ownerReports.releaseRequests.empty());
    REQUIRE_NOTHROW(owner->negotiatedAttributeOwnershipDivestiture(
        objectInstance,
        AttributeHandleSet{ownerAttribute, ownerSecondAttribute},
        tag));
    REQUIRE_NOTHROW(owner->cancelNegotiatedAttributeOwnershipDivestiture(
        objectInstance,
        AttributeHandleSet{ownerAttribute, ownerSecondAttribute}));
    REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, ownerAttribute));
    REQUIRE(owner->isAttributeOwnedByFederate(
        objectInstance, ownerSecondAttribute));
    REQUIRE_FALSE(requester->isAttributeOwnedByFederate(
        objectInstance, requesterAttribute));
    REQUIRE_FALSE(requester->isAttributeOwnedByFederate(
        objectInstance, requesterSecondAttribute));
    REQUIRE(ownerReports.releaseRequests.empty());
    REQUIRE_NOTHROW(owner->evokeMultipleCallbacks(0.0, 0.0));
    REQUIRE(ownerReports.releaseRequests.size() == 1U);
    auto const& release = ownerReports.releaseRequests.front();
    REQUIRE(release.objectInstance == objectInstance);
    REQUIRE(release.attributes ==
            AttributeHandleSet{ownerAttribute, ownerSecondAttribute});
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


}  // namespace
#endif
