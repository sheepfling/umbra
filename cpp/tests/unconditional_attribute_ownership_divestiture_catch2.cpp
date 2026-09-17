#include <catch2/catch_test_macros.hpp>

#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <memory>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The unconditional-divestiture test requires the Umbra source directory."
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

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"unconditional-attribute-ownership-divestiture-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / relativePath;
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct ObjectDiscoveryReport final {
    ObjectInstanceHandle objectInstance;
    ObjectClassHandle objectClass;
  };

  struct OwnershipAssumptionReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  struct AcquisitionReport final {
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

  void attributeOwnershipAcquisitionNotification(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& securedAttributes,
      VariableLengthData const& userSuppliedTag) override {
    acquisitionReports.push_back({
        objectInstance,
        securedAttributes,
        userSuppliedTag,
    });
  }

  std::vector<ObjectDiscoveryReport> objectDiscoveryReports;
  std::vector<OwnershipAssumptionReport> ownershipAssumptionReports;
  std::vector<AcquisitionReport> acquisitionReports;
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
    "Standalone Unconditional Attribute Ownership Divestiture offers eligible 2025 federates",
    "[integration][development-profile][ownership-management]"
    "[unconditional-attribute-ownership-divestiture][standalone]"
    "[rti.service.unconditional-attribute-ownership-divestiture]"
    "[rti.service.attribute-ownership-acquisition]"
    "[rti.service.attribute-ownership-acquisition-if-available]"
    "[federate.callback.request-attribute-ownership-assumption]"
    "[federate.callback.attribute-ownership-acquisition-notification]") {
  rti1516_2025::NullFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador regularRequesterReports;
  ReportingFederateAmbassador ifAvailableRequesterReports;
  ReportingFederateAmbassador invitedCandidateReports;
  ReportingFederateAmbassador staleCandidateReports;
  ReportingFederateAmbassador unpublishedCandidateReports;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto regularRequester = makeRti();
  auto ifAvailableRequester = makeRti();
  auto invitedCandidate = makeRti();
  auto staleCandidate = makeRti();
  auto unpublishedCandidate = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("attribute-update-passel-fom.xml").wstring();
  unsigned char const regularAcquisitionTagBytes[] = {0xA2, 0x07, 0x20};
  unsigned char const pendingIfAvailableTagBytes[] = {0xB2, 0x07, 0x20};
  unsigned char const assumptionTagBytes[] = {0xD2, 0x07, 0x20};
  unsigned char const ifAvailableAcquisitionTagBytes[] = {0xC2, 0x07, 0x20};
  VariableLengthData const regularAcquisitionTag(
      regularAcquisitionTagBytes, sizeof(regularAcquisitionTagBytes));
  VariableLengthData const pendingIfAvailableTag(
      pendingIfAvailableTagBytes, sizeof(pendingIfAvailableTagBytes));
  VariableLengthData const assumptionTag(assumptionTagBytes,
                                         sizeof(assumptionTagBytes));
  VariableLengthData const ifAvailableAcquisitionTag(
      ifAvailableAcquisitionTagBytes, sizeof(ifAvailableAcquisitionTagBytes));
  ObjectInstanceHandle invalidObjectInstance;
  AttributeHandle invalidAttribute;

  REQUIRE_THROWS_AS(
      unjoined->unconditionalAttributeOwnershipDivestiture(
          invalidObjectInstance, AttributeHandleSet{}, assumptionTag),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->unconditionalAttributeOwnershipDivestiture(
          invalidObjectInstance, AttributeHandleSet{}, assumptionTag),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regularRequester->connect(regularRequesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      ifAvailableRequester->connect(ifAvailableRequesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(invitedCandidate->connect(invitedCandidateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(staleCandidate->connect(staleCandidateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      unpublishedCandidate->connect(unpublishedCandidateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"unconditional-divestiture-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(regularRequester->joinFederationExecution(
      L"unconditional-divestiture-regular-requester",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(ifAvailableRequester->joinFederationExecution(
      L"unconditional-divestiture-if-available-requester",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(invitedCandidate->joinFederationExecution(
      L"unconditional-divestiture-invited-candidate",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(staleCandidate->joinFederationExecution(
      L"unconditional-divestiture-stale-candidate",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(unpublishedCandidate->joinFederationExecution(
      L"unconditional-divestiture-unpublished-candidate",
      L"publisher",
      federationName));

  auto const child = owner->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const reliableBaseA = owner->getAttributeHandle(
      child, fixture_hla::fixture::reliable_base_a);
  auto const reliableBaseB = owner->getAttributeHandle(
      child, fixture_hla::fixture::reliable_base_b);
  auto const reliableChild = owner->getAttributeHandle(
      child, fixture_hla::fixture::reliable_child);
  auto const unownedChild = owner->getAttributeHandle(
      child, fixture_hla::fixture::unowned_child);
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(reliableBaseB.isValid());
  REQUIRE(reliableChild.isValid());
  REQUIRE(unownedChild.isValid());

  AttributeHandleSet const regularAttributes{reliableBaseA};
  AttributeHandleSet const ifAvailableAttributes{reliableBaseB};
  AttributeHandleSet const candidateAttributes{reliableChild, unownedChild};
  AttributeHandleSet const ownedAttributes{
      reliableBaseA, reliableBaseB, reliableChild, unownedChild};
  REQUIRE_NOTHROW(regularRequester->subscribeObjectClassAttributes(
      child, regularAttributes));
  REQUIRE_NOTHROW(ifAvailableRequester->subscribeObjectClassAttributes(
      child, ifAvailableAttributes));
  REQUIRE_NOTHROW(invitedCandidate->subscribeObjectClassAttributes(
      child, candidateAttributes));
  REQUIRE_NOTHROW(staleCandidate->subscribeObjectClassAttributes(
      child, candidateAttributes));
  REQUIRE_NOTHROW(unpublishedCandidate->subscribeObjectClassAttributes(
      child, candidateAttributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, ownedAttributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(regularRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(ifAvailableRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(invitedCandidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(staleCandidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(unpublishedCandidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(regularRequesterReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(ifAvailableRequesterReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(invitedCandidateReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(staleCandidateReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(unpublishedCandidateReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_NOTHROW(regularRequester->publishObjectClassAttributes(
      child, regularAttributes));
  REQUIRE_NOTHROW(ifAvailableRequester->publishObjectClassAttributes(
      child, ifAvailableAttributes));
  REQUIRE_NOTHROW(invitedCandidate->publishObjectClassAttributes(
      child, candidateAttributes));
  REQUIRE_NOTHROW(staleCandidate->publishObjectClassAttributes(
      child, candidateAttributes));

  REQUIRE_THROWS_AS(
      owner->unconditionalAttributeOwnershipDivestiture(
          invalidObjectInstance, ownedAttributes, assumptionTag),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      owner->unconditionalAttributeOwnershipDivestiture(
          objectInstance, AttributeHandleSet{invalidAttribute}, assumptionTag),
      rti1516_2025::AttributeNotDefined);
  REQUIRE_THROWS_AS(
      owner->unconditionalAttributeOwnershipDivestiture(
          objectInstance,
          AttributeHandleSet{reliableBaseA, invalidAttribute},
          assumptionTag),
      rti1516_2025::AttributeNotDefined);
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_THROWS_AS(
      regularRequester->unconditionalAttributeOwnershipDivestiture(
          objectInstance, regularAttributes, assumptionTag),
      rti1516_2025::AttributeNotOwned);

  REQUIRE_NOTHROW(regularRequester->attributeOwnershipAcquisition(
      objectInstance, regularAttributes, regularAcquisitionTag));
  REQUIRE_NOTHROW(ifAvailableRequester->attributeOwnershipAcquisitionIfAvailable(
      objectInstance, ifAvailableAttributes, pendingIfAvailableTag));
  REQUIRE_NOTHROW(owner->unconditionalAttributeOwnershipDivestiture(
      objectInstance, ownedAttributes, assumptionTag));
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, reliableBaseB));
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, reliableChild));
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, unownedChild));
  REQUIRE_FALSE(regularRequester->isAttributeOwnedByFederate(
      objectInstance, reliableBaseA));
  REQUIRE_FALSE(ifAvailableRequester->isAttributeOwnedByFederate(
      objectInstance, reliableBaseB));
  REQUIRE_FALSE(invitedCandidate->isAttributeOwnedByFederate(
      objectInstance, reliableChild));

  REQUIRE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_NOTHROW(staleCandidate->unpublishObjectClassAttributes(
      child, candidateAttributes));
  REQUIRE_FALSE(staleCandidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(staleCandidateReports.ownershipAssumptionReports.empty());
  REQUIRE_NOTHROW(staleCandidate->publishObjectClassAttributes(
      child, candidateAttributes));
  REQUIRE_FALSE(staleCandidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(staleCandidateReports.ownershipAssumptionReports.size() == 1U);
  auto const& reofferedAssumption =
      staleCandidateReports.ownershipAssumptionReports.front();
  REQUIRE(reofferedAssumption.objectInstance == objectInstance);
  REQUIRE(reofferedAssumption.attributes == candidateAttributes);
  REQUIRE(variableLengthDataBytes(reofferedAssumption.userSuppliedTag) ==
          std::vector<unsigned char>(
              assumptionTagBytes, assumptionTagBytes + sizeof(assumptionTagBytes)));
  REQUIRE_FALSE(unpublishedCandidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(unpublishedCandidateReports.ownershipAssumptionReports.empty());

  REQUIRE_FALSE(ifAvailableRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ifAvailableRequesterReports.ownershipAssumptionReports.empty());
  REQUIRE(ifAvailableRequesterReports.acquisitionReports.size() == 1U);
  auto const& ifAvailableNotification =
      ifAvailableRequesterReports.acquisitionReports.front();
  REQUIRE(ifAvailableNotification.objectInstance == objectInstance);
  REQUIRE(ifAvailableNotification.attributes == ifAvailableAttributes);
  REQUIRE(variableLengthDataBytes(ifAvailableNotification.userSuppliedTag) ==
          std::vector<unsigned char>(pendingIfAvailableTagBytes,
                                     pendingIfAvailableTagBytes +
                                         sizeof(pendingIfAvailableTagBytes)));
  REQUIRE(ifAvailableRequester->isAttributeOwnedByFederate(
      objectInstance, reliableBaseB));
  REQUIRE_NOTHROW(ifAvailableRequester->unpublishObjectClassAttributes(
      child, ifAvailableAttributes));

  REQUIRE_FALSE(regularRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(regularRequesterReports.acquisitionReports.size() == 1U);
  auto const& regularNotification =
      regularRequesterReports.acquisitionReports.front();
  REQUIRE(regularNotification.objectInstance == objectInstance);
  REQUIRE(regularNotification.attributes == regularAttributes);
  REQUIRE(variableLengthDataBytes(regularNotification.userSuppliedTag) ==
          std::vector<unsigned char>(
              regularAcquisitionTagBytes,
              regularAcquisitionTagBytes + sizeof(regularAcquisitionTagBytes)));
  REQUIRE(regularRequester->isAttributeOwnedByFederate(
      objectInstance, reliableBaseA));
  REQUIRE_NOTHROW(regularRequester->unpublishObjectClassAttributes(
      child, regularAttributes));

  REQUIRE_FALSE(invitedCandidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(invitedCandidateReports.ownershipAssumptionReports.size() == 1U);
  auto const& assumption = invitedCandidateReports.ownershipAssumptionReports.front();
  REQUIRE(assumption.objectInstance == objectInstance);
  REQUIRE(assumption.attributes == candidateAttributes);
  REQUIRE(variableLengthDataBytes(assumption.userSuppliedTag) ==
          std::vector<unsigned char>(
              assumptionTagBytes, assumptionTagBytes + sizeof(assumptionTagBytes)));
  REQUIRE_FALSE(invitedCandidate->isAttributeOwnedByFederate(
      objectInstance, reliableChild));
  REQUIRE_FALSE(invitedCandidate->isAttributeOwnedByFederate(
      objectInstance, unownedChild));
  REQUIRE_NOTHROW(invitedCandidate->attributeOwnershipAcquisitionIfAvailable(
      objectInstance, candidateAttributes, ifAvailableAcquisitionTag));
  REQUIRE_FALSE(invitedCandidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(invitedCandidateReports.acquisitionReports.size() == 1U);
  auto const& candidateNotification = invitedCandidateReports.acquisitionReports.front();
  REQUIRE(candidateNotification.objectInstance == objectInstance);
  REQUIRE(candidateNotification.attributes == candidateAttributes);
  REQUIRE(variableLengthDataBytes(candidateNotification.userSuppliedTag) ==
          std::vector<unsigned char>(
              ifAvailableAcquisitionTagBytes,
              ifAvailableAcquisitionTagBytes + sizeof(ifAvailableAcquisitionTagBytes)));
  REQUIRE(invitedCandidate->isAttributeOwnedByFederate(
      objectInstance, reliableChild));
  REQUIRE(invitedCandidate->isAttributeOwnedByFederate(
      objectInstance, unownedChild));

  REQUIRE_NOTHROW(owner->unpublishObjectClassAttributes(child, ownedAttributes));
  REQUIRE_NOTHROW(invitedCandidate->unpublishObjectClassAttributes(
      child, candidateAttributes));
  REQUIRE_NOTHROW(staleCandidate->unpublishObjectClassAttributes(
      child, candidateAttributes));
  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(regularRequester->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(ifAvailableRequester->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(invitedCandidate->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(staleCandidate->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(unpublishedCandidate->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(regularRequester->disconnect());
  REQUIRE_NOTHROW(ifAvailableRequester->disconnect());
  REQUIRE_NOTHROW(invitedCandidate->disconnect());
  REQUIRE_NOTHROW(staleCandidate->disconnect());
  REQUIRE_NOTHROW(unpublishedCandidate->disconnect());
}
