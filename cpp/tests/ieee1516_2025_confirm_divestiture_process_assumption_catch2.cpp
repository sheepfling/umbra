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
        bool expectedOperationServed = false;
        while (!expectedOperationServed) {
          umbra::detail::TransportServiceMessage response;
          if (!ProcessTransportServiceDispatcher::serveOne(
                  processSession,
                  [&](umbra::detail::TransportServiceMessage const& request) {
                    if (request.operation ==
                            umbra::detail::TransportServiceOperation::receive_interaction &&
                        operation !=
                            umbra::detail::TransportServiceOperation::receive_interaction) {
                      response = handler(request);
                      return response;
                    }
                    if (request.operation != operation) {
                      throw std::runtime_error(description);
                    }
                    expectedOperationServed = true;
                    response = handler(request);
                    return response;
                  })) {
            throw std::runtime_error(description);
          }
          if (response.status != umbra::detail::TransportServiceStatus::ok) {
            throw std::runtime_error(description);
          }
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
      serveExpected(ownerSession, ownerHandler,
          TransportServiceOperation::confirm_divestiture,
          "The process Confirm Divestiture assumption server lost Confirm Divestiture.");
      serveExpected(ownerSession, ownerHandler,
          TransportServiceOperation::report_successful_void_service_invocation,
          "The process Confirm Divestiture assumption server lost its service-report append.");
      serveExpected(
          requesterSession, requesterHandler,
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


}
#endif
