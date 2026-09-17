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
#error "The partial negotiated-divestiture test requires the Umbra source directory."
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
  return L"negotiated-divestiture-partial-acquisition-cancellation-" +
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

  std::vector<ObjectInstanceHandle> objectDiscoveryReports;
  std::vector<AttributeOwnershipAcquisitionReport>
      attributeOwnershipAcquisitionReports;
  std::vector<AttributeOwnershipAcquisitionCancellationReport>
      attributeOwnershipAcquisitionCancellationReports;
  std::vector<DivestitureConfirmationReport> divestitureConfirmationReports;
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
    "Embedded negotiated divestiture retains uncancelled attributes after a partial acquisition cancellation",
    "[integration][development-profile][federation-management][ownership-management][callbacks]"
    "[multi-attribute][ownership-acquisition-cancellation-race]"
    "[rti.service.attribute-ownership-acquisition]"
    "[rti.service.negotiated-attribute-ownership-divestiture]"
    "[rti.service.cancel-attribute-ownership-acquisition]"
    "[rti.service.cancel-negotiated-attribute-ownership-divestiture]"
    "[rti.service.confirm-divestiture]"
    "[federate.callback.request-divestiture-confirmation]"
    "[federate.callback.confirm-attribute-ownership-acquisition-cancellation]"
    "[federate.callback.attribute-ownership-acquisition-notification]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const acquisitionTagBytes[] = {0xC3, 0x25};
  unsigned char const divestitureTagBytes[] = {0xD3, 0x25};
  unsigned char const confirmationTagBytes[] = {0xE3, 0x25};
  VariableLengthData const acquisitionTag(acquisitionTagBytes, sizeof(acquisitionTagBytes));
  VariableLengthData const divestitureTag(divestitureTagBytes, sizeof(divestitureTagBytes));
  VariableLengthData const confirmationTag(confirmationTagBytes, sizeof(confirmationTagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"negotiated-partial-cancel-owner",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"negotiated-partial-cancel-requester",
      L"publisher",
      federationName));

  auto const child = owner->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const cancelledAttribute = owner->getAttributeHandle(
      child,
      fixture_hla::fixture::reliable_base_a);
  auto const retainedAttribute = owner->getAttributeHandle(
      child,
      fixture_hla::fixture::reliable_base_b);
  AttributeHandleSet const cancelledAttributes{cancelledAttribute};
  AttributeHandleSet const retainedAttributes{retainedAttribute};
  AttributeHandleSet const requestedAttributes{
      cancelledAttribute,
      retainedAttribute};
  REQUIRE(child.isValid());
  REQUIRE(cancelledAttribute.isValid());
  REQUIRE(retainedAttribute.isValid());
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
  drainCallbacks(*owner);

  // One ordinary acquisition covers both attributes. Negotiated divestiture
  // queues one confirmation reservation for the whole request.
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance,
      requestedAttributes,
      acquisitionTag));
  REQUIRE_NOTHROW(owner->negotiatedAttributeOwnershipDivestiture(
      objectInstance,
      requestedAttributes,
      divestitureTag));
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, cancelledAttribute));
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, retainedAttribute));

  // Cancel only one member before callback delivery. The cancellation
  // notification contains exactly that member and leaves the retained member
  // in the owner's confirmation reservation.
  REQUIRE_NOTHROW(requester->cancelAttributeOwnershipAcquisition(
      objectInstance,
      cancelledAttributes));
  drainCallbacks(*requester);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.size() == 1U);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.front().objectInstance ==
          objectInstance);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.front().attributes ==
          cancelledAttributes);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.empty());

  drainCallbacks(*owner);
  REQUIRE(ownerReports.divestitureConfirmationReports.size() == 1U);
  REQUIRE(ownerReports.divestitureConfirmationReports.front().objectInstance == objectInstance);
  REQUIRE(ownerReports.divestitureConfirmationReports.front().attributes == retainedAttributes);
  REQUIRE(variableLengthDataBytes(ownerReports.divestitureConfirmationReports.front().userSuppliedTag) ==
          std::vector<unsigned char>(
              acquisitionTagBytes,
              acquisitionTagBytes + sizeof(acquisitionTagBytes)));
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, cancelledAttribute));
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, retainedAttribute));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, cancelledAttribute));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, retainedAttribute));

  REQUIRE_NOTHROW(owner->confirmDivestiture(
      objectInstance,
      retainedAttributes,
      confirmationTag));
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, cancelledAttribute));
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, retainedAttribute));
  REQUIRE(requester->isAttributeOwnedByFederate(objectInstance, retainedAttribute));
  drainCallbacks(*requester);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.size() == 1U);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.front().objectInstance ==
          objectInstance);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.front().attributes ==
          retainedAttributes);
  REQUIRE(variableLengthDataBytes(
              requesterReports.attributeOwnershipAcquisitionReports.front().userSuppliedTag) ==
          std::vector<unsigned char>(
              confirmationTagBytes,
              confirmationTagBytes + sizeof(confirmationTagBytes)));

  // The cancelled member remains in private negotiated state until the owner
  // explicitly removes it; it must not be transferred with the retained set.
  REQUIRE_NOTHROW(owner->cancelNegotiatedAttributeOwnershipDivestiture(
      objectInstance,
      cancelledAttributes));
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, cancelledAttribute));
  REQUIRE_NOTHROW(requester->unpublishObjectClassAttributes(
      child,
      retainedAttributes));
  REQUIRE_NOTHROW(owner->unpublishObjectClassAttributes(
      child,
      requestedAttributes));

  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(requester->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}
