#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAvariableArray.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The negotiated ownership test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using TestFederateAmbassador = rti1516_2025::NullFederateAmbassador;
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

std::vector<unsigned char> variableLengthDataBytes(
    VariableLengthData const& value) {
  auto const* bytes = static_cast<unsigned char const*>(value.data());
  if (bytes == nullptr) {
    return {};
  }
  return {bytes, bytes + value.size()};
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"negotiated-attribute-ownership-divestiture-pending-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct AttributeOwnershipAcquisitionReport final {
    enum class Kind {
      notification,
      unavailable,
    };

    Kind kind = Kind::unavailable;
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  struct AttributeOwnershipAcquisitionCancellationReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
  };

  struct DivestitureConfirmationReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  struct AttributeOwnershipReleaseRequestReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
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
      ObjectClassHandle const&,
      std::wstring const&,
      FederateHandle const&) override {
    objectDiscoveryReports.push_back(objectInstance);
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

  void confirmAttributeOwnershipAcquisitionCancellation(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes) override {
    attributeOwnershipAcquisitionCancellationReports.push_back({
        objectInstance,
        attributes,
    });
  }

  void requestDivestitureConfirmation(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& releasedAttributes,
      VariableLengthData const& userSuppliedTag) override {
    divestitureConfirmationReports.push_back({
        objectInstance,
        releasedAttributes,
        userSuppliedTag,
    });
  }

  void requestAttributeOwnershipRelease(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& candidateAttributes,
      VariableLengthData const& userSuppliedTag) override {
    attributeOwnershipReleaseRequestReports.push_back({
        objectInstance,
        candidateAttributes,
        userSuppliedTag,
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

  std::vector<ObjectInstanceHandle> objectDiscoveryReports;
  std::vector<AttributeOwnershipAcquisitionReport>
      attributeOwnershipAcquisitionReports;
  std::vector<AttributeOwnershipAcquisitionCancellationReport>
      attributeOwnershipAcquisitionCancellationReports;
  std::vector<DivestitureConfirmationReport> divestitureConfirmationReports;
  std::vector<AttributeOwnershipReleaseRequestReport>
      attributeOwnershipReleaseRequestReports;
  std::vector<InteractionReport> interactionReports;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

void drainCallbacks(RTIambassador& rti) {
  for (int pass = 0; pass != 128; ++pass) {
    if (!rti.evokeCallback(0.0)) {
      break;
    }
  }
}

}  // namespace

TEST_CASE(
    "Embedded Negotiated Attribute Ownership Divestiture confirms a pending 2025 acquirer",
    "[integration][development-profile][ownership-management][callbacks]"
    "[rti.service.negotiated-attribute-ownership-divestiture]"
    "[rti.service.confirm-divestiture]"
    "[rti.service.cancel-negotiated-attribute-ownership-divestiture]"
    "[rti.service.attribute-ownership-acquisition]"
    "[federate.callback.request-divestiture-confirmation]"
    "[federate.callback.attribute-ownership-acquisition-notification]") {
  TestFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const acquisitionTagBytes[] = {0x49, 0xA7, 0x11};
  unsigned char const divestitureTagBytes[] = {0x52, 0xA7, 0x11};
  unsigned char const confirmationTagBytes[] = {0x63, 0xA7, 0x11};
  VariableLengthData const acquisitionTag(acquisitionTagBytes, sizeof(acquisitionTagBytes));
  VariableLengthData const divestitureTag(divestitureTagBytes, sizeof(divestitureTagBytes));
  VariableLengthData const confirmationTag(confirmationTagBytes, sizeof(confirmationTagBytes));
  ObjectInstanceHandle invalidObjectInstance;
  AttributeHandle invalidAttribute;
  AttributeHandleSet noAttributes;

  REQUIRE_THROWS_AS(
      unjoined->negotiatedAttributeOwnershipDivestiture(
          invalidObjectInstance,
          noAttributes,
          divestitureTag),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->confirmDivestiture(invalidObjectInstance, noAttributes, confirmationTag),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->cancelNegotiatedAttributeOwnershipDivestiture(
          invalidObjectInstance,
          noAttributes),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->negotiatedAttributeOwnershipDivestiture(
          invalidObjectInstance,
          noAttributes,
          divestitureTag),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->confirmDivestiture(invalidObjectInstance, noAttributes, confirmationTag),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->cancelNegotiatedAttributeOwnershipDivestiture(
          invalidObjectInstance,
          noAttributes),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"negotiated-divestiture-owner",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"negotiated-divestiture-requester",
      L"publisher",
      federationName));

  auto const child = owner->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const transferredAttribute = owner->getAttributeHandle(
      child,
      fixture_hla::fixture::reliable_base_a);
  auto const cancelledAttribute = owner->getAttributeHandle(
      child,
      fixture_hla::fixture::reliable_base_b);
  auto const laterAttribute = owner->getAttributeHandle(
      child,
      fixture_hla::fixture::best_effort_base);
  auto const beforeDeliveryAttribute = owner->getAttributeHandle(
      child,
      fixture_hla::fixture::unowned_child);
  auto const noAcquirerAttribute = owner->getAttributeHandle(
      child,
      fixture_hla::fixture::reliable_child);
  REQUIRE(child.isValid());
  REQUIRE(transferredAttribute.isValid());
  REQUIRE(cancelledAttribute.isValid());
  REQUIRE(laterAttribute.isValid());
  REQUIRE(beforeDeliveryAttribute.isValid());
  REQUIRE(noAcquirerAttribute.isValid());
  AttributeHandleSet const transferredAttributes{transferredAttribute};
  AttributeHandleSet const cancelledAttributes{cancelledAttribute};
  AttributeHandleSet const laterAttributes{laterAttribute};
  AttributeHandleSet const beforeDeliveryAttributes{beforeDeliveryAttribute};
  AttributeHandleSet const noAcquirerAttributes{noAcquirerAttribute};
  AttributeHandleSet const ownedAttributes{
      transferredAttribute,
      cancelledAttribute,
      laterAttribute,
      beforeDeliveryAttribute,
      noAcquirerAttribute,
  };

  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(child, ownedAttributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, ownedAttributes));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(child, ownedAttributes));
  drainCallbacks(*owner);

  REQUIRE_THROWS_AS(
      owner->negotiatedAttributeOwnershipDivestiture(
          invalidObjectInstance,
          transferredAttributes,
          divestitureTag),
      rti1516_2025::ObjectInstanceNotKnown);
  AttributeHandleSet const invalidAttributes{invalidAttribute};
  REQUIRE_THROWS_AS(
      owner->negotiatedAttributeOwnershipDivestiture(
          objectInstance,
          invalidAttributes,
          divestitureTag),
      rti1516_2025::AttributeNotDefined);
  REQUIRE_THROWS_AS(
      requester->negotiatedAttributeOwnershipDivestiture(
          objectInstance,
          transferredAttributes,
          divestitureTag),
      rti1516_2025::AttributeNotOwned);
  REQUIRE_THROWS_AS(
      owner->cancelNegotiatedAttributeOwnershipDivestiture(
          objectInstance,
          transferredAttributes),
      rti1516_2025::AttributeDivestitureWasNotRequested);
  REQUIRE_THROWS_AS(
      owner->confirmDivestiture(objectInstance, transferredAttributes, confirmationTag),
      rti1516_2025::AttributeDivestitureWasNotRequested);

  // A regular acquisition first queues ordinary release work. Starting
  // negotiated divestiture supersedes that work and leaves the owner in
  // Waiting until the explicit confirmation service succeeds.
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance,
      transferredAttributes,
      acquisitionTag));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE_NOTHROW(owner->negotiatedAttributeOwnershipDivestiture(
      objectInstance,
      transferredAttributes,
      divestitureTag));
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, transferredAttribute));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, transferredAttribute));
  REQUIRE_THROWS_AS(
      owner->negotiatedAttributeOwnershipDivestiture(
          objectInstance,
          transferredAttributes,
          divestitureTag),
      rti1516_2025::AttributeAlreadyBeingDivested);
  REQUIRE_THROWS_AS(
      owner->confirmDivestiture(objectInstance, transferredAttributes, confirmationTag),
      rti1516_2025::AttributeDivestitureWasNotRequested);

  drainCallbacks(*owner);
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());
  REQUIRE(ownerReports.divestitureConfirmationReports.size() == 1U);
  auto const& firstConfirmation = ownerReports.divestitureConfirmationReports.front();
  REQUIRE(firstConfirmation.objectInstance == objectInstance);
  REQUIRE(firstConfirmation.attributes == transferredAttributes);
  REQUIRE(variableLengthDataBytes(firstConfirmation.userSuppliedTag) ==
          std::vector<unsigned char>(
              acquisitionTagBytes,
              acquisitionTagBytes + sizeof(acquisitionTagBytes)));
  auto const confirmationCount = ownerReports.divestitureConfirmationReports.size();
  drainCallbacks(*owner);
  REQUIRE(ownerReports.divestitureConfirmationReports.size() == confirmationCount);

  REQUIRE_NOTHROW(owner->confirmDivestiture(
      objectInstance,
      transferredAttributes,
      confirmationTag));
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, transferredAttribute));
  REQUIRE(requester->isAttributeOwnedByFederate(objectInstance, transferredAttribute));
  REQUIRE_THROWS_AS(
      requester->unpublishObjectClassAttributes(child, transferredAttributes),
      rti1516_2025::OwnershipAcquisitionPending);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.empty());
  drainCallbacks(*requester);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.size() == 1U);
  auto const& transferNotification =
      requesterReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(transferNotification.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
  REQUIRE(transferNotification.objectInstance == objectInstance);
  REQUIRE(transferNotification.attributes == transferredAttributes);
  REQUIRE(variableLengthDataBytes(transferNotification.userSuppliedTag) ==
          std::vector<unsigned char>(
              confirmationTagBytes,
              confirmationTagBytes + sizeof(confirmationTagBytes)));
  auto const acquisitionReportCount = requesterReports.attributeOwnershipAcquisitionReports.size();
  drainCallbacks(*requester);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.size() == acquisitionReportCount);
  REQUIRE_NOTHROW(requester->unpublishObjectClassAttributes(child, transferredAttributes));

  // A request arriving after the owner is Waiting still uses the negotiated
  // confirmation route; cancelling that negotiated state restores one normal
  // release request without transferring ownership.
  REQUIRE_NOTHROW(owner->negotiatedAttributeOwnershipDivestiture(
      objectInstance,
      laterAttributes,
      divestitureTag));
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, laterAttribute));
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance,
      laterAttributes,
      acquisitionTag));
  drainCallbacks(*owner);
  REQUIRE(ownerReports.divestitureConfirmationReports.size() == 2U);
  REQUIRE(ownerReports.divestitureConfirmationReports.back().attributes == laterAttributes);
  REQUIRE_NOTHROW(owner->cancelNegotiatedAttributeOwnershipDivestiture(
      objectInstance,
      laterAttributes));
  drainCallbacks(*owner);
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.size() == 1U);
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.back().attributes == laterAttributes);
  REQUIRE(variableLengthDataBytes(
              ownerReports.attributeOwnershipReleaseRequestReports.back().userSuppliedTag) ==
          std::vector<unsigned char>(
              acquisitionTagBytes,
              acquisitionTagBytes + sizeof(acquisitionTagBytes)));
  REQUIRE_NOTHROW(requester->cancelAttributeOwnershipAcquisition(
      objectInstance,
      laterAttributes));
  drainCallbacks(*requester);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.size() == 1U);
  REQUIRE_NOTHROW(requester->unpublishObjectClassAttributes(child, laterAttributes));

  // Cancelling before the confirmation callback is delivered suppresses that
  // callback and preserves the queued ordinary release request exactly once.
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance,
      beforeDeliveryAttributes,
      acquisitionTag));
  REQUIRE_NOTHROW(owner->negotiatedAttributeOwnershipDivestiture(
      objectInstance,
      beforeDeliveryAttributes,
      divestitureTag));
  REQUIRE_NOTHROW(owner->cancelNegotiatedAttributeOwnershipDivestiture(
      objectInstance,
      beforeDeliveryAttributes));
  drainCallbacks(*owner);
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.size() == 2U);
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.back().attributes ==
          beforeDeliveryAttributes);
  REQUIRE(ownerReports.divestitureConfirmationReports.size() == 2U);
  REQUIRE_NOTHROW(requester->cancelAttributeOwnershipAcquisition(
      objectInstance,
      beforeDeliveryAttributes));
  drainCallbacks(*requester);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.size() == 2U);
  REQUIRE_NOTHROW(requester->unpublishObjectClassAttributes(
      child,
      beforeDeliveryAttributes));

  // Once a real confirmation has been delivered, cancellation still leaves
  // ownership with the current owner and returns to the ordinary release path.
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance,
      cancelledAttributes,
      acquisitionTag));
  REQUIRE_NOTHROW(owner->negotiatedAttributeOwnershipDivestiture(
      objectInstance,
      cancelledAttributes,
      divestitureTag));
  drainCallbacks(*owner);
  REQUIRE(ownerReports.divestitureConfirmationReports.size() == 3U);
  REQUIRE_NOTHROW(owner->cancelNegotiatedAttributeOwnershipDivestiture(
      objectInstance,
      cancelledAttributes));
  drainCallbacks(*owner);
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.size() == 3U);
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.back().attributes ==
          cancelledAttributes);
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, cancelledAttribute));
  REQUIRE_THROWS_AS(
      owner->confirmDivestiture(objectInstance, cancelledAttributes, confirmationTag),
      rti1516_2025::AttributeDivestitureWasNotRequested);
  REQUIRE_NOTHROW(requester->cancelAttributeOwnershipAcquisition(
      objectInstance,
      cancelledAttributes));
  drainCallbacks(*requester);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.size() == 3U);
  REQUIRE_NOTHROW(requester->unpublishObjectClassAttributes(child, cancelledAttributes));

  // A selected requester can cancel after the one-shot confirmation. The
  // official NoAcquisitionPending result leaves ownership intact and allows
  // the owner to remove the negotiated state explicitly.
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance,
      noAcquirerAttributes,
      acquisitionTag));
  REQUIRE_NOTHROW(owner->negotiatedAttributeOwnershipDivestiture(
      objectInstance,
      noAcquirerAttributes,
      divestitureTag));
  drainCallbacks(*owner);
  REQUIRE(ownerReports.divestitureConfirmationReports.size() == 4U);
  REQUIRE_NOTHROW(requester->cancelAttributeOwnershipAcquisition(
      objectInstance,
      noAcquirerAttributes));
  REQUIRE_THROWS_AS(
      owner->confirmDivestiture(objectInstance, noAcquirerAttributes, confirmationTag),
      rti1516_2025::NoAcquisitionPending);
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, noAcquirerAttribute));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, noAcquirerAttribute));
  REQUIRE_NOTHROW(owner->cancelNegotiatedAttributeOwnershipDivestiture(
      objectInstance,
      noAcquirerAttributes));
  drainCallbacks(*requester);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.size() == 4U);
  REQUIRE_NOTHROW(requester->unpublishObjectClassAttributes(child, noAcquirerAttributes));

  REQUIRE_NOTHROW(owner->unpublishObjectClassAttributes(child, ownedAttributes));
  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(requester->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}

TEST_CASE(
    "Embedded service reporting delivers Cancel Negotiated Attribute Ownership Divestiture through MOM interaction",
    "[integration][development-profile][federation-management][ownership-management]"
    "[mom][service-reporting][service-report-interaction]"
    "[cancel-negotiated-attribute-ownership-divestiture-service-report-interaction]"
    "[cancel-negotiated-attribute-ownership-divestiture]"
    "[rti.service.negotiated-attribute-ownership-divestiture]"
    "[rti.service.attribute-ownership-acquisition]"
    "[rti.service.cancel-negotiated-attribute-ownership-divestiture]"
    "[rti.service.subscribe-interaction-class]"
    "[federate.callback.request-attribute-ownership-release]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  ReportingFederateAmbassador observerReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const ownershipFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "attribute-update-passel-fom.xml")
          .wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{ownershipFom, switchFom};
  unsigned char const acquisitionTagBytes[] = {0x49, 0xA7, 0x11};
  unsigned char const divestitureTagBytes[] = {0x52, 0xA7, 0x11};
  VariableLengthData const acquisitionTag(
      acquisitionTagBytes,
      sizeof(acquisitionTagBytes));
  VariableLengthData const divestitureTag(
      divestitureTagBytes,
      sizeof(divestitureTagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, HLA_IMMEDIATE));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModules,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"negotiated-cancellation-mom-owner",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"negotiated-cancellation-mom-requester",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"negotiated-cancellation-mom-observer",
      L"observer",
      federationName));

  auto const reportClass = observer->getInteractionClassHandle(
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
    auto const parameter = observer->getParameterHandle(reportClass, name);
    REQUIRE(parameter.isValid());
    reportParameters.push_back(parameter);
  }
  REQUIRE_NOTHROW(observer->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(requester->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(requester->setSendServiceReportsToFileSwitch(false));

  auto const child = owner->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const ownedAttribute = owner->getAttributeHandle(
      child,
      fixture_hla::fixture::reliable_base_a);
  REQUIRE(child.isValid());
  REQUIRE(ownedAttribute.isValid());
  AttributeHandleSet const requestedAttributes{ownedAttribute};
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(
      child,
      requestedAttributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
      child,
      requestedAttributes));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(
      child,
      requestedAttributes));

  // The regular acquisition creates the pending owner-release path that a
  // negotiated divestiture supersedes. Keep both service-report switches off
  // until the accepted cancellation is the only reported service.
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance,
      requestedAttributes,
      acquisitionTag));
  REQUIRE(observerReports.interactionReports.empty());
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());
  REQUIRE_NOTHROW(owner->negotiatedAttributeOwnershipDivestiture(
      objectInstance,
      requestedAttributes,
      divestitureTag));
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, ownedAttribute));
  REQUIRE(observerReports.interactionReports.empty());
  REQUIRE(ownerReports.divestitureConfirmationReports.empty());

  // Cancel has no user tag (the supplied report records are only the object
  // and attribute designators). Enable interaction reporting immediately
  // before the accepted call so the observer sees one synchronous report,
  // while the ordinary release callback remains evoked and separately queued.
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE(owner->getServiceReportingSwitch());
  REQUIRE_FALSE(owner->getSendServiceReportsToFileSwitch());
  REQUIRE_NOTHROW(owner->cancelNegotiatedAttributeOwnershipDivestiture(
      objectInstance,
      requestedAttributes));
  REQUIRE(observerReports.interactionReports.size() == 1U);
  REQUIRE(ownerReports.divestitureConfirmationReports.empty());

  auto const& report = observerReports.interactionReports.front();
  REQUIRE(report.interactionClass == reportClass);
  REQUIRE(report.parameterValues.size() == 7U);
  REQUIRE(report.userSuppliedTag.size() == 0U);
  REQUIRE(report.transportationType ==
          observer->getTransportationTypeHandle(standard_hla::mom::reliable));
  REQUIRE_FALSE(report.producingFederate.isValid());

  rti1516_2025::HLAunicodeString service;
  REQUIRE_NOTHROW(service.decode(report.parameterValues.at(reportParameters[0])));
  REQUIRE(service.get() == L"CancelNegotiatedAttributeOwnershipDivestiture");
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

  std::wstring attributeValues = L"[\"" + ownedAttribute.toString() + L"\"]";
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

  // Cancellation invalidates the pending negotiated confirmation and queues
  // exactly one ordinary release request. Cancelling the acquisition from the
  // requester then clears that follow-up state without another MOM report.
  drainCallbacks(*owner);
  REQUIRE(ownerReports.divestitureConfirmationReports.empty());
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.size() == 1U);
  auto const& releaseRequest =
      ownerReports.attributeOwnershipReleaseRequestReports.front();
  REQUIRE(releaseRequest.objectInstance == objectInstance);
  REQUIRE(releaseRequest.attributes == requestedAttributes);
  REQUIRE(variableLengthDataBytes(releaseRequest.userSuppliedTag) ==
          std::vector<unsigned char>(
              acquisitionTagBytes,
              acquisitionTagBytes + sizeof(acquisitionTagBytes)));
  REQUIRE_NOTHROW(requester->cancelAttributeOwnershipAcquisition(
      objectInstance,
      requestedAttributes));
  drainCallbacks(*requester);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.size() ==
          1U);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE_NOTHROW(requester->unpublishObjectClassAttributes(
      child,
      requestedAttributes));
  REQUIRE_NOTHROW(owner->unpublishObjectClassAttributes(
      child,
      requestedAttributes));

  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(requester->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(observer->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
