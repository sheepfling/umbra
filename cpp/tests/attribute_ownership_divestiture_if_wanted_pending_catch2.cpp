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
#error "The Divestiture If Wanted ownership test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using TestFederateAmbassador = rti1516_2025::NullFederateAmbassador;
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
  return L"attribute-ownership-divestiture-if-wanted-pending-" +
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
    "Embedded Attribute Ownership Divestiture If Wanted transfers only to 2025 pending acquirers",
    "[integration][development-profile][ownership-management]"
    "[rti.service.attribute-ownership-divestiture-if-wanted]"
    "[rti.service.attribute-ownership-acquisition]"
    "[rti.service.attribute-ownership-acquisition-if-available]"
    "[federate.callback.attribute-ownership-acquisition-notification]") {
  TestFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador regularRequesterReports;
  ReportingFederateAmbassador ifAvailableRequesterReports;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto regularRequester = makeRti();
  auto ifAvailableRequester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const regularAcquisitionTagBytes[] = {0xA1, 0x01, 0x25};
  unsigned char const ifAvailableAcquisitionTagBytes[] = {0xA2, 0x02, 0x25};
  unsigned char const divestitureTagBytes[] = {0xD1, 0x56, 0x25};
  VariableLengthData const regularAcquisitionTag(
      regularAcquisitionTagBytes,
      sizeof(regularAcquisitionTagBytes));
  VariableLengthData const ifAvailableAcquisitionTag(
      ifAvailableAcquisitionTagBytes,
      sizeof(ifAvailableAcquisitionTagBytes));
  VariableLengthData const divestitureTag(divestitureTagBytes, sizeof(divestitureTagBytes));
  ObjectInstanceHandle invalidObjectInstance;
  AttributeHandle invalidAttribute;
  AttributeHandleSet noAttributes;

  REQUIRE_THROWS_AS(
      unjoined->attributeOwnershipDivestitureIfWanted(
          invalidObjectInstance,
          noAttributes,
          divestitureTag,
          noAttributes),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->attributeOwnershipDivestitureIfWanted(
          invalidObjectInstance,
          noAttributes,
          divestitureTag,
          noAttributes),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regularRequester->connect(regularRequesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(ifAvailableRequester->connect(ifAvailableRequesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"divestiture-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(regularRequester->joinFederationExecution(
      L"divestiture-regular-requester",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(ifAvailableRequester->joinFederationExecution(
      L"divestiture-if-available-requester",
      L"publisher",
      federationName));

  auto const child = owner->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const reliableBaseA = owner->getAttributeHandle(child, fixture_hla::fixture::reliable_base_a);
  auto const reliableChild = owner->getAttributeHandle(child, fixture_hla::fixture::reliable_child);
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(reliableChild.isValid());
  AttributeHandleSet const regularAttributes{reliableBaseA};
  AttributeHandleSet const ifAvailableAttributes{reliableChild};
  AttributeHandleSet const ownedAttributes{reliableBaseA, reliableChild};

  REQUIRE_NOTHROW(regularRequester->subscribeObjectClassAttributes(child, regularAttributes));
  REQUIRE_NOTHROW(
      ifAvailableRequester->subscribeObjectClassAttributes(child, ifAvailableAttributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, ownedAttributes));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(regularRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(ifAvailableRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(regularRequesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE(ifAvailableRequesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE_NOTHROW(regularRequester->publishObjectClassAttributes(child, regularAttributes));
  REQUIRE_NOTHROW(
      ifAvailableRequester->publishObjectClassAttributes(child, ifAvailableAttributes));

  AttributeHandleSet divestedAttributes{reliableBaseA};
  REQUIRE_THROWS_AS(
      owner->attributeOwnershipDivestitureIfWanted(
          invalidObjectInstance,
          regularAttributes,
          divestitureTag,
          divestedAttributes),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      owner->attributeOwnershipDivestitureIfWanted(
          objectInstance,
          AttributeHandleSet{invalidAttribute},
          divestitureTag,
          divestedAttributes),
      rti1516_2025::AttributeNotDefined);
  REQUIRE_THROWS_AS(
      regularRequester->attributeOwnershipDivestitureIfWanted(
          objectInstance,
          regularAttributes,
          divestitureTag,
          divestedAttributes),
      rti1516_2025::AttributeNotOwned);

  // Without a pending acquirer, the service succeeds but returns an empty
  // result and retains the current owner. The output set is replaced rather
  // than appended to the caller's old contents.
  REQUIRE_NOTHROW(owner->attributeOwnershipDivestitureIfWanted(
      objectInstance,
      regularAttributes,
      divestitureTag,
      divestedAttributes));
  REQUIRE(divestedAttributes.empty());
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, reliableBaseA));

  REQUIRE_NOTHROW(regularRequester->attributeOwnershipAcquisition(
      objectInstance,
      regularAttributes,
      regularAcquisitionTag));
  REQUIRE_NOTHROW(ifAvailableRequester->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      ifAvailableAttributes,
      ifAvailableAcquisitionTag));

  // The owner accepts both forms as genuine pending acquirers. The regular
  // request's already queued owner-release callback is invalidated by the
  // synchronous ownership transfer.
  divestedAttributes = AttributeHandleSet{reliableBaseA};
  REQUIRE_NOTHROW(owner->attributeOwnershipDivestitureIfWanted(
      objectInstance,
      ownedAttributes,
      divestitureTag,
      divestedAttributes));
  REQUIRE(divestedAttributes == ownedAttributes);
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, reliableChild));
  REQUIRE(regularRequester->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE(ifAvailableRequester->isAttributeOwnedByFederate(objectInstance, reliableChild));
  REQUIRE_THROWS_AS(
      regularRequester->unpublishObjectClassAttributes(child, regularAttributes),
      rti1516_2025::OwnershipAcquisitionPending);
  REQUIRE_THROWS_AS(
      ifAvailableRequester->unpublishObjectClassAttributes(child, ifAvailableAttributes),
      rti1516_2025::OwnershipAcquisitionPending);
  REQUIRE_NOTHROW(owner->unpublishObjectClassAttributes(child, ownedAttributes));
  // The former owner's release request may still occupy the callback queue,
  // but it must be stale after the synchronous transfer and must not invoke
  // the release callback.
  drainCallbacks(*owner);
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());

  // The regular acquirer receives the divestiture tag, not the older regular
  // acquisition tag, and its publication guard ends exactly at callback entry.
  REQUIRE_FALSE(regularRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(regularRequesterReports.attributeOwnershipAcquisitionReports.size() == 1);
  auto const& regularNotification =
      regularRequesterReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(regularNotification.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
  REQUIRE(regularNotification.objectInstance == objectInstance);
  REQUIRE(regularNotification.attributes == regularAttributes);
  REQUIRE(variableLengthDataBytes(regularNotification.userSuppliedTag) ==
          std::vector<unsigned char>(
              divestitureTagBytes,
              divestitureTagBytes + sizeof(divestitureTagBytes)));
  REQUIRE_NOTHROW(regularRequester->unpublishObjectClassAttributes(child, regularAttributes));

  // The stale If Available report is consumed first; its direct divestiture
  // notification follows with the same mandatory divestiture tag.
  REQUIRE(ifAvailableRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ifAvailableRequesterReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE_FALSE(ifAvailableRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ifAvailableRequesterReports.attributeOwnershipAcquisitionReports.size() == 1);
  auto const& ifAvailableNotification =
      ifAvailableRequesterReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(ifAvailableNotification.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
  REQUIRE(ifAvailableNotification.objectInstance == objectInstance);
  REQUIRE(ifAvailableNotification.attributes == ifAvailableAttributes);
  REQUIRE(variableLengthDataBytes(ifAvailableNotification.userSuppliedTag) ==
          std::vector<unsigned char>(
              divestitureTagBytes,
              divestitureTagBytes + sizeof(divestitureTagBytes)));
  REQUIRE_NOTHROW(
      ifAvailableRequester->unpublishObjectClassAttributes(child, ifAvailableAttributes));
  REQUIRE_THROWS_AS(
      owner->attributeOwnershipDivestitureIfWanted(
          objectInstance,
          regularAttributes,
          divestitureTag,
          divestedAttributes),
      rti1516_2025::AttributeNotOwned);

  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(regularRequester->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(ifAvailableRequester->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(regularRequester->disconnect());
  REQUIRE_NOTHROW(ifAvailableRequester->disconnect());
}
