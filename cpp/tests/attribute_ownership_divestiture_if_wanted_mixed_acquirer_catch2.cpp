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
#error "The mixed If Wanted ownership test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleValueMap;
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
  return L"attribute-ownership-divestiture-if-wanted-mixed-acquirer-" +
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
    "Embedded Attribute Ownership Divestiture If Wanted orders mixed 2025 acquirers by request",
    "[integration][development-profile][ownership-management]"
    "[multi-federate-callback-ordering]"
    "[rti.service.attribute-ownership-divestiture-if-wanted]"
    "[rti.service.attribute-ownership-acquisition]"
    "[rti.service.attribute-ownership-acquisition-if-available]"
    "[federate.callback.attribute-ownership-acquisition-notification]"
    "[federate.callback.request-attribute-ownership-release]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador firstIfAvailableReports;
  ReportingFederateAmbassador laterRegularReports;
  auto owner = makeRti();
  auto firstIfAvailable = makeRti();
  auto laterRegular = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = testDataPath(
      "attribute-update-passel-fom.xml").wstring();
  unsigned char const ifAvailableTagBytes[] = {0x11, 0x25};
  unsigned char const regularTagBytes[] = {0x22, 0x25};
  unsigned char const divestitureTagBytes[] = {0xD2, 0x25};
  unsigned char const denialTagBytes[] = {0xD3, 0x25};
  VariableLengthData const ifAvailableTag(
      ifAvailableTagBytes,
      sizeof(ifAvailableTagBytes));
  VariableLengthData const regularTag(regularTagBytes, sizeof(regularTagBytes));
  VariableLengthData const divestitureTag(
      divestitureTagBytes,
      sizeof(divestitureTagBytes));
  VariableLengthData const denialTag(denialTagBytes, sizeof(denialTagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(firstIfAvailable->connect(firstIfAvailableReports, HLA_EVOKED));
  REQUIRE_NOTHROW(laterRegular->connect(laterRegularReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"mixed-divestiture-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(firstIfAvailable->joinFederationExecution(
      L"mixed-divestiture-first-if-available", L"publisher", federationName));
  REQUIRE_NOTHROW(laterRegular->joinFederationExecution(
      L"mixed-divestiture-later-regular", L"publisher", federationName));

  auto const child = owner->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const reliableBaseA = owner->getAttributeHandle(
      child,
      fixture_hla::fixture::reliable_base_a);
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  AttributeHandleSet const attributes{reliableBaseA};
  REQUIRE_NOTHROW(firstIfAvailable->subscribeObjectClassAttributes(
      child,
      attributes));
  REQUIRE_NOTHROW(laterRegular->subscribeObjectClassAttributes(
      child,
      attributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, attributes));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(firstIfAvailable->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(laterRegular->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(firstIfAvailableReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(laterRegularReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_NOTHROW(firstIfAvailable->publishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(laterRegular->publishObjectClassAttributes(child, attributes));

  // The serial embedded policy chooses the earliest accepted request across
  // regular and If Available forms, then retains later regular work for the
  // selected owner to answer.
  REQUIRE_NOTHROW(firstIfAvailable->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      attributes,
      ifAvailableTag));
  REQUIRE_NOTHROW(laterRegular->attributeOwnershipAcquisition(
      objectInstance,
      attributes,
      regularTag));
  AttributeHandleSet divestedAttributes;
  REQUIRE_NOTHROW(owner->attributeOwnershipDivestitureIfWanted(
      objectInstance,
      attributes,
      divestitureTag,
      divestedAttributes));
  REQUIRE(divestedAttributes == attributes);
  REQUIRE(firstIfAvailable->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_FALSE(laterRegular->isAttributeOwnedByFederate(objectInstance, reliableBaseA));

  // The old owner's release request and the first queued If Available
  // terminal report are stale. The direct notification is followed by the
  // retained regular release request on the selected owner.
  REQUIRE_NOTHROW(drainCallbacks(*owner));
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());
  REQUIRE(laterRegularReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE(laterRegularReports.attributeOwnershipReleaseRequestReports.empty());
  REQUIRE(firstIfAvailable->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(firstIfAvailableReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE(laterRegularReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE(laterRegularReports.attributeOwnershipReleaseRequestReports.empty());
  REQUIRE(firstIfAvailable->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(firstIfAvailableReports.attributeOwnershipAcquisitionReports.size() == 1U);
  auto const& divestitureNotification =
      firstIfAvailableReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(divestitureNotification.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
  REQUIRE(divestitureNotification.attributes == attributes);
  REQUIRE(variableLengthDataBytes(divestitureNotification.userSuppliedTag) ==
          std::vector<unsigned char>(
              divestitureTagBytes,
              divestitureTagBytes + sizeof(divestitureTagBytes)));
  REQUIRE(laterRegularReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE(laterRegularReports.attributeOwnershipReleaseRequestReports.empty());
  REQUIRE_FALSE(firstIfAvailable->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(firstIfAvailableReports.attributeOwnershipReleaseRequestReports.size() == 1U);
  auto const& laterRelease =
      firstIfAvailableReports.attributeOwnershipReleaseRequestReports.front();
  REQUIRE(laterRelease.objectInstance == objectInstance);
  REQUIRE(laterRelease.attributes == attributes);
  REQUIRE(variableLengthDataBytes(laterRelease.userSuppliedTag) ==
          std::vector<unsigned char>(regularTagBytes, regularTagBytes + sizeof(regularTagBytes)));
  REQUIRE(laterRegularReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE(laterRegularReports.attributeOwnershipReleaseRequestReports.empty());

  REQUIRE_NOTHROW(firstIfAvailable->attributeOwnershipReleaseDenied(
      objectInstance,
      attributes,
      denialTag));
  REQUIRE_FALSE(laterRegular->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(laterRegularReports.attributeOwnershipAcquisitionReports.size() == 1U);
  auto const& laterUnavailable =
      laterRegularReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(laterUnavailable.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::unavailable);
  REQUIRE(laterUnavailable.attributes == attributes);
  REQUIRE(variableLengthDataBytes(laterUnavailable.userSuppliedTag) ==
          std::vector<unsigned char>(denialTagBytes, denialTagBytes + sizeof(denialTagBytes)));

  REQUIRE_NOTHROW(owner->unpublishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(firstIfAvailable->unpublishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(laterRegular->unpublishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(firstIfAvailable->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(laterRegular->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(firstIfAvailable->disconnect());
  REQUIRE_NOTHROW(laterRegular->disconnect());
}
