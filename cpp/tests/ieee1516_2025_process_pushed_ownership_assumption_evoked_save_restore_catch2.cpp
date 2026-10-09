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
    "RTIambassadors preserve an unconsumed pushed ownership-assumption callback across HLA_EVOKED save and restore",
    "[integration][development-profile][federation-management][save-restore][ownership-management]"
    "[transport][process-boundary][process-local-restore][push-mode][public-endpoint][ownership-assumption]"
    "[callback-model][evoked][unconsumed-callback][durable-pending-callback]"
    "[process-pushed-ownership-assumption-evoked-save-restore]"
    "[rti.service.unconditional-attribute-ownership-divestiture]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.request-federation-restore][rti.service.federate-restore-complete]"
    "[federate.callback.federation-restored][federate.callback.request-attribute-ownership-assumption][2025]") {
  class PendingRestoreFederateAmbassador final
      : public rti1516_2025::NullFederateAmbassador {
   public:
    void discoverObjectInstance(
        ObjectInstanceHandle const& objectInstance,
        ObjectClassHandle const& objectClass,
        std::wstring const&,
        FederateHandle const&) override {
      ++discoveryCount;
      discoveredObject = objectInstance;
      discoveredClass = objectClass;
    }

    void requestAttributeOwnershipAssumption(
        ObjectInstanceHandle const& objectInstance,
        AttributeHandleSet const& attributes,
        VariableLengthData const& tag) override {
      ++assumptionCount;
      assumptionObject = objectInstance;
      assumptionAttributes = attributes;
      assumptionTag = tag;
    }

    void initiateFederateSave(std::wstring const& label) override {
      ++saveInitiateCount;
      saveLabel = label;
    }

    void federationSaved() override { ++saveCompleteCount; }

    void federationRestoreBegun() override { ++restoreBegunCount; }

    void initiateFederateRestore(
        std::wstring const& label,
        std::wstring const& federateName,
        FederateHandle const& postRestoreFederateHandle) override {
      ++restoreInitiateCount;
      restoreLabel = label;
      restoreFederateName = federateName;
      restorePostFederateHandle = postRestoreFederateHandle;
    }

    void federationRestored() override { ++restoreCompleteCount; }

    std::size_t discoveryCount = 0U;
    ObjectInstanceHandle discoveredObject;
    ObjectClassHandle discoveredClass;
    std::size_t assumptionCount = 0U;
    ObjectInstanceHandle assumptionObject;
    AttributeHandleSet assumptionAttributes;
    VariableLengthData assumptionTag;
    std::size_t saveInitiateCount = 0U;
    std::size_t saveCompleteCount = 0U;
    std::size_t restoreBegunCount = 0U;
    std::size_t restoreInitiateCount = 0U;
    std::size_t restoreCompleteCount = 0U;
    std::wstring saveLabel;
    std::wstring restoreLabel;
    std::wstring restoreFederateName;
    FederateHandle restorePostFederateHandle;
  } ownerReports, candidateReports;

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
          {"process-evoked-pending-ownership-assumption-server", 0xA760U},
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
          "The pending ownership-assumption server lost Create.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::join_federation_execution,
          "The pending ownership-assumption server lost owner Join.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::get_object_class_handle,
          "The pending ownership-assumption server lost owner class lookup.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::get_attribute_handle,
          "The pending ownership-assumption server lost owner attribute lookup.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The pending ownership-assumption server lost owner Publish.");

      auto candidateConnection = listener->accept(
          nullptr,
          {"process-evoked-pending-ownership-assumption-server", 0xA761U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession candidateSession(candidateConnection);
      auto candidateHandler = service.handlerFor(candidateSession);
      serveExpected(
          candidateSession,
          candidateHandler,
          TransportServiceOperation::join_federation_execution,
          "The pending ownership-assumption server lost candidate Join.");
      serveExpected(
          candidateSession,
          candidateHandler,
          TransportServiceOperation::get_object_class_handle,
          "The pending ownership-assumption server lost candidate class lookup.");
      serveExpected(
          candidateSession,
          candidateHandler,
          TransportServiceOperation::get_attribute_handle,
          "The pending ownership-assumption server lost candidate attribute lookup.");
      serveExpected(
          candidateSession,
          candidateHandler,
          TransportServiceOperation::subscribe_object_class_attributes,
          "The pending ownership-assumption server lost candidate Subscribe.");
      serveExpected(
          candidateSession,
          candidateHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The pending ownership-assumption server lost candidate Publish.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::register_object_instance,
          "The pending ownership-assumption server lost owner Register.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::unconditional_attribute_ownership_divestiture,
          "The pending ownership-assumption server lost Unconditional Divestiture.");
      serveExpected(
          candidateSession,
          candidateHandler,
          TransportServiceOperation::get_object_class_handle,
          "The pending ownership-assumption server lost pushed callback fence.");

      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::request_federation_save,
          "The pending ownership-assumption server lost Request Federation Save.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::get_object_class_handle,
          "The pending ownership-assumption server lost owner save fence.");
      serveExpected(
          candidateSession,
          candidateHandler,
          TransportServiceOperation::get_object_class_handle,
          "The pending ownership-assumption server lost candidate save fence.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::federate_save_begun,
          "The pending ownership-assumption server lost owner Save Begun.");
      serveExpected(
          candidateSession,
          candidateHandler,
          TransportServiceOperation::federate_save_begun,
          "The pending ownership-assumption server lost candidate Save Begun.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::federate_save_complete,
          "The pending ownership-assumption server lost owner Save Complete.");
      serveExpected(
          candidateSession,
          candidateHandler,
          TransportServiceOperation::federate_save_complete,
          "The pending ownership-assumption server lost candidate Save Complete.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::get_object_class_handle,
          "The pending ownership-assumption server lost owner save-complete fence.");
      serveExpected(
          candidateSession,
          candidateHandler,
          TransportServiceOperation::get_object_class_handle,
          "The pending ownership-assumption server lost candidate save-complete fence.");

      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::request_federation_restore,
          "The pending ownership-assumption server lost Request Federation Restore.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::get_object_class_handle,
          "The pending ownership-assumption server lost owner restore fence.");
      serveExpected(
          candidateSession,
          candidateHandler,
          TransportServiceOperation::get_object_class_handle,
          "The pending ownership-assumption server lost candidate restore fence.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::federate_restore_complete,
          "The pending ownership-assumption server lost owner Restore Complete.");
      serveExpected(
          candidateSession,
          candidateHandler,
          TransportServiceOperation::federate_restore_complete,
          "The pending ownership-assumption server lost candidate Restore Complete.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::get_object_class_handle,
          "The pending ownership-assumption server lost owner restored fence.");
      serveExpected(
          candidateSession,
          candidateHandler,
          TransportServiceOperation::get_object_class_handle,
          "The pending ownership-assumption server lost candidate restored fence.");
      serveExpected(
          candidateSession,
          candidateHandler,
          TransportServiceOperation::resign_federation_execution,
          "The pending ownership-assumption server lost candidate Resign.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::resign_federation_execution,
          "The pending ownership-assumption server lost owner Resign.");
      service.detach(candidateSession);
      service.detach(ownerSession);
      candidateConnection->close();
      ownerConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  auto owner = makeRti();
  auto candidate = makeRti();
  auto ownerConfiguration = rti1516_2025::RtiConfiguration::createConfiguration()
                                .withConfigurationName(
                                    L"process-evoked-pending-ownership-assumption-owner")
                                .withRtiAddress(
                                    L"tcp://127.0.0.1:" + std::to_wstring(port));
  auto candidateConfiguration =
      rti1516_2025::RtiConfiguration::createConfiguration()
          .withConfigurationName(
              L"process-evoked-pending-ownership-assumption-candidate")
          .withRtiAddress(
              L"tcp://127.0.0.1:" + std::to_wstring(port));
  auto const federationName =
      nextFederationName() + L"-process-evoked-pending-ownership-assumption";
  auto const fomModule = resourcePath("attribute-update-passel-fom.xml").wstring();
  unsigned char const assumptionTagBytes[] = {0xC4, 0x70, 0x2A, 0xD8};
  VariableLengthData const assumptionTag(
      assumptionTagBytes, sizeof(assumptionTagBytes));
  constexpr wchar_t const* saveLabel =
      L"process-evoked-pending-ownership-assumption-save";
  std::exception_ptr clientError;
  bool ownerJoined = false;
  bool candidateJoined = false;
  try {
    REQUIRE_NOTHROW(owner->connect(
        ownerReports, HLA_EVOKED, ownerConfiguration));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName, fomModule));
    REQUIRE_NOTHROW(owner->joinFederationExecution(
        L"process-evoked-pending-ownership-assumption-owner",
        L"publisher",
        federationName));
    ownerJoined = true;
    auto const ownerClass = owner->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const ownerAttribute = owner->getAttributeHandle(
        ownerClass, fixture_hla::fixture::unowned_child);
    REQUIRE(ownerClass.isValid());
    REQUIRE(ownerAttribute.isValid());
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
        ownerClass, AttributeHandleSet{ownerAttribute}));

    REQUIRE_NOTHROW(candidate->connect(
        candidateReports, HLA_EVOKED, candidateConfiguration));
    REQUIRE_NOTHROW(candidate->joinFederationExecution(
        L"process-evoked-pending-ownership-assumption-candidate",
        L"publisher",
        federationName));
    candidateJoined = true;
    auto const candidateClass = candidate->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const candidateAttribute = candidate->getAttributeHandle(
        candidateClass, fixture_hla::fixture::unowned_child);
    REQUIRE(candidateClass.isValid());
    REQUIRE(candidateAttribute.isValid());
    REQUIRE_NOTHROW(candidate->subscribeObjectClassAttributes(
        candidateClass, AttributeHandleSet{candidateAttribute}));
    REQUIRE_NOTHROW(candidate->publishObjectClassAttributes(
        candidateClass, AttributeHandleSet{candidateAttribute}));

    ObjectInstanceHandle objectInstance;
    REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(ownerClass));
    REQUIRE(objectInstance.isValid());

    REQUIRE_NOTHROW(owner->unconditionalAttributeOwnershipDivestiture(
        objectInstance, AttributeHandleSet{ownerAttribute}, assumptionTag));
    // This public fence admits the pushed discovery and ownership frames into
    // the HLA_EVOKED callback queue, but deliberately does not invoke either.
    REQUIRE_NOTHROW(static_cast<void>(candidate->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child)));
    REQUIRE(candidateReports.discoveryCount == 0U);
    REQUIRE(candidateReports.assumptionCount == 0U);

    REQUIRE_NOTHROW(owner->requestFederationSave(saveLabel));
    REQUIRE_NOTHROW(static_cast<void>(owner->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child)));
    REQUIRE_NOTHROW(static_cast<void>(candidate->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child)));
    REQUIRE(ownerReports.saveInitiateCount == 0U);
    REQUIRE(candidateReports.saveInitiateCount == 0U);
    REQUIRE(candidateReports.assumptionCount == 0U);
    REQUIRE_NOTHROW(owner->federateSaveBegun());
    REQUIRE_NOTHROW(candidate->federateSaveBegun());
    REQUIRE_NOTHROW(owner->federateSaveComplete());
    REQUIRE_NOTHROW(candidate->federateSaveComplete());
    REQUIRE_NOTHROW(static_cast<void>(owner->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child)));
    REQUIRE_NOTHROW(static_cast<void>(candidate->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child)));
    REQUIRE(ownerReports.saveInitiateCount == 0U);
    REQUIRE(candidateReports.saveInitiateCount == 0U);
    REQUIRE(candidateReports.assumptionCount == 0U);

    REQUIRE_NOTHROW(owner->requestFederationRestore(saveLabel));
    REQUIRE_NOTHROW(static_cast<void>(owner->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child)));
    REQUIRE_NOTHROW(static_cast<void>(candidate->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child)));
    REQUIRE_NOTHROW(owner->federateRestoreComplete());
    REQUIRE_NOTHROW(candidate->federateRestoreComplete());
    REQUIRE_NOTHROW(static_cast<void>(owner->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child)));
    REQUIRE_NOTHROW(static_cast<void>(candidate->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child)));
    REQUIRE(ownerReports.discoveryCount == 0U);
    REQUIRE(ownerReports.assumptionCount == 0U);
    REQUIRE(ownerReports.saveInitiateCount == 0U);
    REQUIRE(candidateReports.discoveryCount == 0U);
    REQUIRE(candidateReports.assumptionCount == 0U);
    REQUIRE(candidateReports.saveInitiateCount == 0U);
    REQUIRE(candidateReports.restoreCompleteCount == 0U);

    static_cast<void>(candidate->evokeCallback(0.0));
    REQUIRE(candidateReports.discoveryCount == 1U);
    REQUIRE(candidateReports.discoveredObject == objectInstance);
    REQUIRE(candidateReports.discoveredClass == candidateClass);
    REQUIRE(candidateReports.assumptionCount == 0U);
    static_cast<void>(candidate->evokeCallback(0.0));
    REQUIRE(candidateReports.assumptionCount == 1U);
    REQUIRE(candidateReports.assumptionObject == objectInstance);
    REQUIRE(candidateReports.assumptionAttributes ==
            AttributeHandleSet{candidateAttribute});
    REQUIRE(variableLengthDataBytes(candidateReports.assumptionTag) ==
            std::vector<unsigned char>(
                assumptionTagBytes,
                assumptionTagBytes + sizeof(assumptionTagBytes)));
    static_cast<void>(candidate->evokeCallback(0.0));
    REQUIRE(candidateReports.saveInitiateCount == 1U);
    REQUIRE(candidateReports.saveLabel == saveLabel);
    static_cast<void>(candidate->evokeCallback(0.0));
    REQUIRE(candidateReports.saveCompleteCount == 1U);
    static_cast<void>(candidate->evokeCallback(0.0));
    REQUIRE(candidateReports.restoreBegunCount == 1U);
    static_cast<void>(candidate->evokeCallback(0.0));
    REQUIRE(candidateReports.restoreInitiateCount == 1U);
    static_cast<void>(candidate->evokeCallback(0.0));
    REQUIRE(candidateReports.restoreCompleteCount == 1U);

    static_cast<void>(owner->evokeCallback(0.0));
    REQUIRE(ownerReports.saveInitiateCount == 1U);
    static_cast<void>(owner->evokeCallback(0.0));
    REQUIRE(ownerReports.saveCompleteCount == 1U);
    // The requester also receives the successful Confirm Federation
    // Restoration Request callback before the federation-wide restore
    // lifecycle callbacks.  The ambassador intentionally leaves that
    // callback unrecorded here; the following Evoke is its boundary.
    static_cast<void>(owner->evokeCallback(0.0));
    static_cast<void>(owner->evokeCallback(0.0));
    REQUIRE(ownerReports.restoreBegunCount == 1U);
    static_cast<void>(owner->evokeCallback(0.0));
    REQUIRE(ownerReports.restoreInitiateCount == 1U);
    static_cast<void>(owner->evokeCallback(0.0));
    REQUIRE(ownerReports.restoreCompleteCount == 1U);

    REQUIRE_NOTHROW(candidate->resignFederationExecution(
        rti1516_2025::NO_ACTION));
    candidateJoined = false;
    REQUIRE_NOTHROW(owner->resignFederationExecution(
        rti1516_2025::NO_ACTION));
    ownerJoined = false;
    REQUIRE_NOTHROW(owner->disconnect());
    REQUIRE_NOTHROW(candidate->disconnect());
  } catch (...) {
    clientError = std::current_exception();
    if (candidateJoined) {
      try {
        candidate->resignFederationExecution(rti1516_2025::NO_ACTION);
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
      owner->disconnect();
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
