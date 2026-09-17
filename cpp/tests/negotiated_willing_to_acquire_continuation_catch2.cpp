#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The negotiated ownership continuation test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
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
  return L"negotiated-willing-to-acquire-continuation-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path testDataPath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / relativePath;
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

  std::vector<ObjectInstanceHandle> objectDiscoveryReports;
  std::vector<AttributeOwnershipAcquisitionReport>
      attributeOwnershipAcquisitionReports;
  std::vector<AttributeOwnershipAcquisitionCancellationReport>
      attributeOwnershipAcquisitionCancellationReports;
  std::vector<DivestitureConfirmationReport> divestitureConfirmationReports;
  std::vector<AttributeOwnershipReleaseRequestReport>
      attributeOwnershipReleaseRequestReports;
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
    "Embedded Negotiated Attribute Ownership Divestiture advances after the first willing-to-acquire candidate cancels",
    "[integration][development-profile][ownership-management][callbacks]"
    "[multi-federate-callback-ordering]"
    "[negotiated-willing-to-acquire-continuation]"
    "[rti.service.negotiated-attribute-ownership-divestiture]"
    "[rti.service.attribute-ownership-acquisition-if-available]"
    "[rti.service.cancel-attribute-ownership-acquisition]"
    "[rti.service.confirm-divestiture]"
    "[federate.callback.request-divestiture-confirmation]"
    "[federate.callback.confirm-attribute-ownership-acquisition-cancellation]"
    "[federate.callback.attribute-ownership-acquisition-notification]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador firstCandidateReports;
  ReportingFederateAmbassador secondCandidateReports;
  auto owner = makeRti();
  auto firstCandidate = makeRti();
  auto secondCandidate = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = testDataPath(
      "attribute-update-passel-fom.xml").wstring();
  unsigned char const firstWillingTagBytes[] = {0x81, 0xA7, 0x25};
  unsigned char const firstRegularTagBytes[] = {0x85, 0xA7, 0x25};
  unsigned char const secondWillingTagBytes[] = {0x82, 0xA7, 0x25};
  unsigned char const divestitureTagBytes[] = {0x83, 0xA7, 0x25};
  unsigned char const confirmationTagBytes[] = {0x84, 0xA7, 0x25};
  VariableLengthData const firstWillingTag(
      firstWillingTagBytes,
      sizeof(firstWillingTagBytes));
  VariableLengthData const firstRegularTag(
      firstRegularTagBytes,
      sizeof(firstRegularTagBytes));
  VariableLengthData const secondWillingTag(
      secondWillingTagBytes,
      sizeof(secondWillingTagBytes));
  VariableLengthData const divestitureTag(
      divestitureTagBytes,
      sizeof(divestitureTagBytes));
  VariableLengthData const confirmationTag(
      confirmationTagBytes,
      sizeof(confirmationTagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(firstCandidate->connect(firstCandidateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(secondCandidate->connect(secondCandidateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"negotiated-continuation-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(firstCandidate->joinFederationExecution(
      L"negotiated-continuation-first", L"publisher", federationName));
  REQUIRE_NOTHROW(secondCandidate->joinFederationExecution(
      L"negotiated-continuation-second", L"publisher", federationName));

  auto const child = owner->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const transferredAttribute = owner->getAttributeHandle(
      child,
      fixture_hla::fixture::reliable_base_a);
  REQUIRE(child.isValid());
  REQUIRE(transferredAttribute.isValid());
  AttributeHandleSet const transferredAttributes{transferredAttribute};

  REQUIRE_NOTHROW(firstCandidate->subscribeObjectClassAttributes(
      child,
      transferredAttributes));
  REQUIRE_NOTHROW(secondCandidate->subscribeObjectClassAttributes(
      child,
      transferredAttributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
      child,
      transferredAttributes));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(firstCandidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(secondCandidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(firstCandidateReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(secondCandidateReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_NOTHROW(firstCandidate->publishObjectClassAttributes(
      child,
      transferredAttributes));
  REQUIRE_NOTHROW(secondCandidate->publishObjectClassAttributes(
      child,
      transferredAttributes));

  // Both requests enter Willing to Acquire while the owner still holds the
  // attribute. The serial profile retains the later candidate for a
  // continuation after the first federate cancels its superseding request.
  REQUIRE_NOTHROW(firstCandidate->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      transferredAttributes,
      firstWillingTag));
  REQUIRE_NOTHROW(secondCandidate->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      transferredAttributes,
      secondWillingTag));
  REQUIRE(firstCandidateReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE(secondCandidateReports.attributeOwnershipAcquisitionReports.empty());

  REQUIRE_NOTHROW(owner->negotiatedAttributeOwnershipDivestiture(
      objectInstance,
      transferredAttributes,
      divestitureTag));
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, transferredAttribute));
  REQUIRE_FALSE(firstCandidate->isAttributeOwnedByFederate(
      objectInstance,
      transferredAttribute));
  REQUIRE_FALSE(secondCandidate->isAttributeOwnedByFederate(
      objectInstance,
      transferredAttribute));
  REQUIRE(ownerReports.divestitureConfirmationReports.empty());

  // There is no separate WTA-cancellation service in the 2025 API. A regular
  // request supersedes the first reservation; Cancel Attribute Ownership
  // Acquisition then terminates it before the owner confirmation callback.
  REQUIRE_NOTHROW(firstCandidate->attributeOwnershipAcquisition(
      objectInstance,
      transferredAttributes,
      firstRegularTag));
  REQUIRE_NOTHROW(firstCandidate->cancelAttributeOwnershipAcquisition(
      objectInstance,
      transferredAttributes));
  drainCallbacks(*firstCandidate);
  REQUIRE(firstCandidateReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE(firstCandidateReports.attributeOwnershipAcquisitionCancellationReports.size() == 1U);
  REQUIRE(firstCandidateReports.attributeOwnershipAcquisitionCancellationReports.front().objectInstance ==
          objectInstance);
  REQUIRE(firstCandidateReports.attributeOwnershipAcquisitionCancellationReports.front().attributes ==
          transferredAttributes);
  REQUIRE(secondCandidateReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE(secondCandidateReports.attributeOwnershipAcquisitionCancellationReports.empty());

  // Older owner work is stale. The continuation confirmation for the second
  // candidate is the only owner-side confirmation and carries its WTA tag.
  drainCallbacks(*owner);
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());
  REQUIRE(ownerReports.divestitureConfirmationReports.size() == 1U);
  auto const& confirmation = ownerReports.divestitureConfirmationReports.front();
  REQUIRE(confirmation.objectInstance == objectInstance);
  REQUIRE(confirmation.attributes == transferredAttributes);
  REQUIRE(variableLengthDataBytes(confirmation.userSuppliedTag) ==
          std::vector<unsigned char>(
              secondWillingTagBytes,
              secondWillingTagBytes + sizeof(secondWillingTagBytes)));
  REQUIRE(secondCandidateReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE(secondCandidateReports.attributeOwnershipAcquisitionCancellationReports.empty());

  REQUIRE_NOTHROW(owner->confirmDivestiture(
      objectInstance,
      transferredAttributes,
      confirmationTag));
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, transferredAttribute));
  REQUIRE_FALSE(firstCandidate->isAttributeOwnedByFederate(
      objectInstance,
      transferredAttribute));
  REQUIRE(secondCandidate->isAttributeOwnedByFederate(
      objectInstance,
      transferredAttribute));
  REQUIRE(secondCandidateReports.attributeOwnershipAcquisitionReports.empty());

  // The second candidate's original If Available callback is stale; its
  // terminal notification is the only successful transfer report.
  drainCallbacks(*secondCandidate);
  REQUIRE(secondCandidateReports.attributeOwnershipAcquisitionReports.size() == 1U);
  auto const& acquisitionNotification =
      secondCandidateReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(acquisitionNotification.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
  REQUIRE(acquisitionNotification.objectInstance == objectInstance);
  REQUIRE(acquisitionNotification.attributes == transferredAttributes);
  REQUIRE(variableLengthDataBytes(acquisitionNotification.userSuppliedTag) ==
          std::vector<unsigned char>(
              confirmationTagBytes,
              confirmationTagBytes + sizeof(confirmationTagBytes)));

  REQUIRE_NOTHROW(firstCandidate->unpublishObjectClassAttributes(
      child,
      transferredAttributes));
  REQUIRE_NOTHROW(secondCandidate->unpublishObjectClassAttributes(
      child,
      transferredAttributes));
  REQUIRE_NOTHROW(owner->unpublishObjectClassAttributes(
      child,
      transferredAttributes));
  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(firstCandidate->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(secondCandidate->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(firstCandidate->disconnect());
  REQUIRE_NOTHROW(secondCandidate->disconnect());
}
