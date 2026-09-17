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
#error "The negotiated Willing-to-Acquire test requires the Umbra source directory."
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
  return L"negotiated-willing-to-acquire-candidate-" +
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

  struct DivestitureConfirmationReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  struct OwnershipAssumptionReport final {
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

  void requestAttributeOwnershipAssumption(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& offeredAttributes,
      VariableLengthData const& userSuppliedTag) override {
    ownershipAssumptionReports.push_back({
        objectInstance,
        offeredAttributes,
        userSuppliedTag,
    });
  }

  std::vector<ObjectInstanceHandle> objectDiscoveryReports;
  std::vector<AttributeOwnershipAcquisitionReport>
      attributeOwnershipAcquisitionReports;
  std::vector<DivestitureConfirmationReport> divestitureConfirmationReports;
  std::vector<OwnershipAssumptionReport> ownershipAssumptionReports;
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
    "Embedded Negotiated Attribute Ownership Divestiture selects a willing-to-acquire 2025 candidate",
    "[integration][development-profile][ownership-management][callbacks]"
    "[negotiated-willing-to-acquire]"
    "[rti.service.negotiated-attribute-ownership-divestiture]"
    "[rti.service.attribute-ownership-acquisition-if-available]"
    "[rti.service.confirm-divestiture]"
    "[federate.callback.request-divestiture-confirmation]"
    "[federate.callback.attribute-ownership-acquisition-notification]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador candidateReports;
  auto owner = makeRti();
  auto candidate = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const willingTagBytes[] = {0x71, 0xA7, 0x25};
  unsigned char const divestitureTagBytes[] = {0x72, 0xA7, 0x25};
  unsigned char const confirmationTagBytes[] = {0x73, 0xA7, 0x25};
  VariableLengthData const willingTag(willingTagBytes, sizeof(willingTagBytes));
  VariableLengthData const divestitureTag(divestitureTagBytes, sizeof(divestitureTagBytes));
  VariableLengthData const confirmationTag(confirmationTagBytes, sizeof(confirmationTagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(candidate->connect(candidateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"negotiated-wta-owner",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(candidate->joinFederationExecution(
      L"negotiated-wta-candidate",
      L"publisher",
      federationName));

  auto const child = owner->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const transferredAttribute = owner->getAttributeHandle(
      child,
      fixture_hla::fixture::reliable_base_a);
  REQUIRE(child.isValid());
  REQUIRE(transferredAttribute.isValid());
  AttributeHandleSet const transferredAttributes{transferredAttribute};
  REQUIRE_NOTHROW(candidate->subscribeObjectClassAttributes(
      child,
      transferredAttributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
      child,
      transferredAttributes));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(candidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(candidateReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_NOTHROW(candidate->publishObjectClassAttributes(
      child,
      transferredAttributes));
  drainCallbacks(*owner);

  // If Available enters Willing to Acquire without changing ownership. The
  // negotiated divestiture selects that private candidate and preserves the
  // acquisition tag in the owner-side confirmation callback.
  REQUIRE_NOTHROW(candidate->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      transferredAttributes,
      willingTag));
  REQUIRE(candidateReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE_NOTHROW(owner->negotiatedAttributeOwnershipDivestiture(
      objectInstance,
      transferredAttributes,
      divestitureTag));
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, transferredAttribute));
  REQUIRE_FALSE(candidate->isAttributeOwnedByFederate(objectInstance, transferredAttribute));
  drainCallbacks(*owner);
  REQUIRE(ownerReports.divestitureConfirmationReports.size() == 1U);
  auto const& confirmation = ownerReports.divestitureConfirmationReports.front();
  REQUIRE(confirmation.objectInstance == objectInstance);
  REQUIRE(confirmation.attributes == transferredAttributes);
  REQUIRE(variableLengthDataBytes(confirmation.userSuppliedTag) ==
          std::vector<unsigned char>(
              willingTagBytes,
              willingTagBytes + sizeof(willingTagBytes)));

  // Confirming consumes the WTA reservation before the ordinary If Available
  // callback is dispatched. The stale terminal report must not be surfaced as
  // unavailable; exactly one tagged acquisition notification is delivered.
  REQUIRE_NOTHROW(owner->confirmDivestiture(
      objectInstance,
      transferredAttributes,
      confirmationTag));
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, transferredAttribute));
  REQUIRE(candidate->isAttributeOwnedByFederate(objectInstance, transferredAttribute));
  REQUIRE(candidateReports.attributeOwnershipAcquisitionReports.empty());
  drainCallbacks(*candidate);
  REQUIRE(candidateReports.attributeOwnershipAcquisitionReports.size() == 1U);
  auto const& acquisitionNotification =
      candidateReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(acquisitionNotification.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
  REQUIRE(acquisitionNotification.objectInstance == objectInstance);
  REQUIRE(acquisitionNotification.attributes == transferredAttributes);
  REQUIRE(variableLengthDataBytes(acquisitionNotification.userSuppliedTag) ==
          std::vector<unsigned char>(
              confirmationTagBytes,
              confirmationTagBytes + sizeof(confirmationTagBytes)));
  REQUIRE_NOTHROW(candidate->unpublishObjectClassAttributes(
      child,
      transferredAttributes));

  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::DELETE_OBJECTS_THEN_DIVEST));
  drainCallbacks(*candidate);
  REQUIRE_NOTHROW(candidate->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(candidate->disconnect());
}

TEST_CASE(
    "Embedded Negotiated Attribute Ownership Divestiture forwards its tag to an assumption candidate",
    "[integration][development-profile][ownership-management][callbacks]"
    "[negotiated-willing-to-acquire][negotiated-assumption]"
    "[rti.service.negotiated-attribute-ownership-divestiture]"
    "[rti.service.attribute-ownership-acquisition]"
    "[rti.service.confirm-divestiture]"
    "[federate.callback.request-attribute-ownership-assumption]"
    "[federate.callback.request-divestiture-confirmation]"
    "[federate.callback.attribute-ownership-acquisition-notification]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador candidateReports;
  auto owner = makeRti();
  auto candidate = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const assumptionTagBytes[] = {0x61, 0xA7, 0x25};
  unsigned char const acquisitionTagBytes[] = {0x62, 0xA7, 0x25};
  unsigned char const confirmationTagBytes[] = {0x63, 0xA7, 0x25};
  VariableLengthData const assumptionTag(assumptionTagBytes, sizeof(assumptionTagBytes));
  VariableLengthData const acquisitionTag(acquisitionTagBytes, sizeof(acquisitionTagBytes));
  VariableLengthData const confirmationTag(confirmationTagBytes, sizeof(confirmationTagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(candidate->connect(candidateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"negotiated-assumption-owner",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(candidate->joinFederationExecution(
      L"negotiated-assumption-candidate",
      L"publisher",
      federationName));

  auto const child = owner->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const transferredAttribute = owner->getAttributeHandle(
      child,
      fixture_hla::fixture::reliable_base_a);
  AttributeHandleSet const transferredAttributes{transferredAttribute};
  REQUIRE_NOTHROW(candidate->subscribeObjectClassAttributes(
      child,
      transferredAttributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
      child,
      transferredAttributes));
  REQUIRE_NOTHROW(candidate->publishObjectClassAttributes(
      child,
      transferredAttributes));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  drainCallbacks(*candidate);
  REQUIRE(candidateReports.objectDiscoveryReports.size() == 1U);

  // Negotiated divestiture keeps the owner in place while it asks the next
  // eligible federate whether it wishes to assume ownership. The callback
  // must carry the divestiture tag, before any acquisition is requested.
  REQUIRE_NOTHROW(owner->negotiatedAttributeOwnershipDivestiture(
      objectInstance,
      transferredAttributes,
      assumptionTag));
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, transferredAttribute));
  REQUIRE_FALSE(candidate->isAttributeOwnedByFederate(objectInstance, transferredAttribute));
  drainCallbacks(*candidate);
  REQUIRE(candidateReports.ownershipAssumptionReports.size() == 1U);
  auto const& assumption = candidateReports.ownershipAssumptionReports.front();
  REQUIRE(assumption.objectInstance == objectInstance);
  REQUIRE(assumption.attributes == transferredAttributes);
  REQUIRE(variableLengthDataBytes(assumption.userSuppliedTag) ==
          std::vector<unsigned char>(
              assumptionTagBytes,
              assumptionTagBytes + sizeof(assumptionTagBytes)));

  // A positive assumption response is expressed through the standard regular
  // acquisition service. The owner receives the acquisition tag, then the
  // confirmation transfers ownership and carries its own terminal tag.
  REQUIRE_NOTHROW(candidate->attributeOwnershipAcquisition(
      objectInstance,
      transferredAttributes,
      acquisitionTag));
  drainCallbacks(*owner);
  REQUIRE(ownerReports.divestitureConfirmationReports.size() == 1U);
  auto const& confirmation = ownerReports.divestitureConfirmationReports.front();
  REQUIRE(variableLengthDataBytes(confirmation.userSuppliedTag) ==
          std::vector<unsigned char>(
              acquisitionTagBytes,
              acquisitionTagBytes + sizeof(acquisitionTagBytes)));
  REQUIRE_NOTHROW(owner->confirmDivestiture(
      objectInstance,
      transferredAttributes,
      confirmationTag));
  REQUIRE(candidate->isAttributeOwnedByFederate(objectInstance, transferredAttribute));
  drainCallbacks(*candidate);
  REQUIRE(candidateReports.attributeOwnershipAcquisitionReports.size() == 1U);
  auto const& notification = candidateReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(notification.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
  REQUIRE(variableLengthDataBytes(notification.userSuppliedTag) ==
          std::vector<unsigned char>(
              confirmationTagBytes,
              confirmationTagBytes + sizeof(confirmationTagBytes)));

  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::DELETE_OBJECTS_THEN_DIVEST));
  drainCallbacks(*candidate);
  REQUIRE_NOTHROW(candidate->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(candidate->disconnect());
}
