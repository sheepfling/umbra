#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The joined-owner timestamped object-deletion retraction test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::LogicalTime;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::OrderType;
using rti1516_2025::RECEIVE;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TIMESTAMP;
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
  return L"timestamped-object-deletion-joined-owner-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path sourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / relativePath;
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct RemovalReport final {
    ObjectInstanceHandle objectInstance;
    VariableLengthData tag;
    FederateHandle producer;
    std::wstring timeImplementationName;
    std::wstring timeValue;
    OrderType sentOrderType = RECEIVE;
    OrderType receivedOrderType = RECEIVE;
    bool retractionSupplied = false;
    bool retractionValid = false;
  };

  struct RequestRetractionReport final {
    bool retractionValid = false;
    VariableLengthData encodedRetraction;
  };

  struct AttributeOwnershipReport final {
    enum class Kind { federate, unowned };

    Kind kind = Kind::unowned;
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    FederateHandle owner;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const&,
      std::wstring const&,
      FederateHandle const&) override {
    objectDiscoveryReports.push_back(objectInstance);
  }

  void removeObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      VariableLengthData const& tag,
      FederateHandle const& producer,
      LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
    objectRemovalReports.push_back({
        objectInstance,
        tag,
        producer,
        time.implementationName(),
        time.toString(),
        sentOrderType,
        receivedOrderType,
        optionalRetraction != nullptr,
        optionalRetraction != nullptr && optionalRetraction->isValid(),
    });
    callbackOrder.push_back("remove");
  }

  void requestRetraction(
      rti1516_2025::MessageRetractionHandle const& retraction) override {
    requestRetractionReports.push_back({retraction.isValid(), retraction.encode()});
    callbackOrder.push_back("request-retraction");
  }

  void informAttributeOwnership(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      FederateHandle const& owner) override {
    attributeOwnershipReports.push_back({
        AttributeOwnershipReport::Kind::federate,
        objectInstance,
        attributes,
        owner,
    });
  }

  void attributeIsNotOwned(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes) override {
    attributeOwnershipReports.push_back({
        AttributeOwnershipReport::Kind::unowned,
        objectInstance,
        attributes,
        FederateHandle(),
    });
  }

  std::vector<ObjectInstanceHandle> objectDiscoveryReports;
  std::vector<RemovalReport> objectRemovalReports;
  std::vector<RequestRetractionReport> requestRetractionReports;
  std::vector<AttributeOwnershipReport> attributeOwnershipReports;
  std::vector<std::string> callbackOrder;
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
    "Embedded Request Retraction reconstitutes timestamped deletion only for joined owners",
    "[integration][development-profile][tso-object-deletion-joined-owner-retraction]"
    "[object-management][ownership-management]"
    "[time-management][tso][retraction][multi-federate-callback-ordering]"
    "[rti.service.delete-object-instance][rti.service.retract]"
    "[rti.service.resign-federation-execution]"
    "[rti.service.query-attribute-ownership]"
    "[federate.callback.remove-object-instance]"
    "[federate.callback.request-retraction]"
    "[federate.callback.attribute-is-not-owned]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador departedOwnerReports;
  ReportingFederateAmbassador survivorReports;
  auto publisher = makeRti();
  auto departedOwner = makeRti();
  auto survivor = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = sourcePath(
      std::filesystem::path("cpp") / "tests" / "data" /
      "attribute-update-passel-fom.xml").wstring();
  unsigned char const deletionTagBytes[] = {0x4AU, 0x4FU, 0x49, 0x4E};
  unsigned char const divestitureTagBytes[] = {0x44U, 0x49, 0x56};
  VariableLengthData const deletionTag(
      deletionTagBytes, sizeof(deletionTagBytes));
  VariableLengthData const divestitureTag(
      divestitureTagBytes, sizeof(divestitureTagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(departedOwner->connect(departedOwnerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(survivor->connect(survivorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"joined-owner-retraction-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(departedOwner->joinFederationExecution(
      L"joined-owner-retraction-departed-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(survivor->joinFederationExecution(
      L"joined-owner-retraction-survivor", L"subscriber", federationName));

  auto const objectClass = publisher->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const reliable = publisher->getAttributeHandle(
      objectClass, fixture_hla::fixture::reliable_base_a);
  auto const bestEffort = publisher->getAttributeHandle(
      objectClass, fixture_hla::fixture::best_effort_base);
  REQUIRE(objectClass.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE(bestEffort.isValid());
  AttributeHandleSet const bothAttributes{reliable, bestEffort};
  AttributeHandleSet const departedOwnerAttributes{reliable};

  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      objectClass, bothAttributes));
  REQUIRE_NOTHROW(departedOwner->publishObjectClassAttributes(
      objectClass, departedOwnerAttributes));
  REQUIRE_NOTHROW(departedOwner->subscribeObjectClassAttributes(
      objectClass, bothAttributes));
  REQUIRE_NOTHROW(survivor->subscribeObjectClassAttributes(
      objectClass, bothAttributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(objectClass));
  drainCallbacks(*departedOwner);
  drainCallbacks(*survivor);
  REQUIRE(departedOwnerReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(survivorReports.objectDiscoveryReports.size() == 1U);
  auto const objectInstanceName = publisher->getObjectInstanceName(objectInstance);

  // Move one attribute to the soon-to-depart joined owner. The publisher keeps
  // HLAprivilegeToDeleteObject and the second attribute, so the deletion is
  // still issued by a different federate than the owner that will resign.
  VariableLengthData acquisitionTag;
  REQUIRE_NOTHROW(departedOwner->attributeOwnershipAcquisition(
      objectInstance, departedOwnerAttributes, acquisitionTag));
  AttributeHandleSet divestedAttributes;
  REQUIRE_NOTHROW(publisher->attributeOwnershipDivestitureIfWanted(
      objectInstance,
      departedOwnerAttributes,
      divestitureTag,
      divestedAttributes));
  REQUIRE(divestedAttributes == departedOwnerAttributes);
  drainCallbacks(*publisher);
  drainCallbacks(*departedOwner);
  REQUIRE_FALSE(publisher->isAttributeOwnedByFederate(objectInstance, reliable));
  REQUIRE(publisher->isAttributeOwnedByFederate(objectInstance, bestEffort));
  REQUIRE(departedOwner->isAttributeOwnedByFederate(objectInstance, reliable));
  REQUIRE_NOTHROW(publisher->unpublishObjectClassAttributes(
      objectClass, departedOwnerAttributes));

  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*publisher);
  auto const retraction = publisher->deleteObjectInstance(
      objectInstance,
      deletionTag,
      rti1516_2025::HLAinteger64Time(2));
  REQUIRE(retraction.isValid());

  // Both nonconstrained subscribers receive the deletion at their own callback
  // boundary. The departed owner is deliberately crossed first so its delivery
  // is recorded before it leaves the execution.
  REQUIRE_FALSE(departedOwner->evokeCallback(0.0));
  REQUIRE(departedOwnerReports.objectRemovalReports.size() == 1U);
  REQUIRE(departedOwnerReports.objectRemovalReports.front().objectInstance ==
          objectInstance);
  REQUIRE(departedOwnerReports.objectRemovalReports.front().producer ==
          publisherHandle);
  REQUIRE(departedOwnerReports.objectRemovalReports.front().sentOrderType == TIMESTAMP);
  REQUIRE(departedOwnerReports.objectRemovalReports.front().receivedOrderType == RECEIVE);
  REQUIRE(departedOwnerReports.objectRemovalReports.front().retractionSupplied);
  REQUIRE(departedOwnerReports.objectRemovalReports.front().retractionValid);

  REQUIRE_FALSE(survivor->evokeCallback(0.0));
  REQUIRE(survivorReports.objectRemovalReports.size() == 1U);
  REQUIRE(survivorReports.objectRemovalReports.front().objectInstance == objectInstance);

  // A departed recipient is removed from the active recipient set before the
  // legal Retract. It must not be reconstituted or receive Request Retraction.
  REQUIRE_NOTHROW(departedOwner->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE_FALSE(survivor->evokeCallback(0.0));
  REQUIRE(survivorReports.requestRetractionReports.size() == 1U);
  REQUIRE(survivorReports.requestRetractionReports.front().retractionValid);
  REQUIRE(variableLengthDataBytes(
              survivorReports.requestRetractionReports.front().encodedRetraction) ==
          variableLengthDataBytes(retraction.encode()));
  REQUIRE(survivorReports.callbackOrder ==
          std::vector<std::string>{"remove", "request-retraction"});
  REQUIRE(departedOwnerReports.requestRetractionReports.empty());

  // The invocation snapshot is restored only for members that are still in the
  // execution. The departed owner's former attribute remains unowned.
  REQUIRE(publisher->getObjectInstanceName(objectInstance) == objectInstanceName);
  REQUIRE(publisher->getObjectInstanceHandle(objectInstanceName) == objectInstance);
  REQUIRE(survivor->getObjectInstanceHandle(objectInstanceName) == objectInstance);
  REQUIRE(publisher->isAttributeOwnedByFederate(objectInstance, bestEffort));
  REQUIRE_FALSE(publisher->isAttributeOwnedByFederate(objectInstance, reliable));

  REQUIRE_NOTHROW(publisher->queryAttributeOwnership(
      objectInstance, bothAttributes));
  drainCallbacks(*publisher);
  REQUIRE(publisherReports.attributeOwnershipReports.size() == 2U);
  auto const ownerReport = std::find_if(
      publisherReports.attributeOwnershipReports.begin(),
      publisherReports.attributeOwnershipReports.end(),
      [](ReportingFederateAmbassador::AttributeOwnershipReport const& report) {
        return report.kind ==
            ReportingFederateAmbassador::AttributeOwnershipReport::Kind::federate;
      });
  auto const unownedReport = std::find_if(
      publisherReports.attributeOwnershipReports.begin(),
      publisherReports.attributeOwnershipReports.end(),
      [](ReportingFederateAmbassador::AttributeOwnershipReport const& report) {
        return report.kind ==
            ReportingFederateAmbassador::AttributeOwnershipReport::Kind::unowned;
      });
  REQUIRE(ownerReport != publisherReports.attributeOwnershipReports.end());
  REQUIRE(unownedReport != publisherReports.attributeOwnershipReports.end());
  REQUIRE(ownerReport->objectInstance == objectInstance);
  REQUIRE(ownerReport->attributes == AttributeHandleSet{bestEffort});
  REQUIRE(ownerReport->owner == publisherHandle);
  REQUIRE(unownedReport->objectInstance == objectInstance);
  REQUIRE(unownedReport->attributes == AttributeHandleSet{reliable});

  REQUIRE_NOTHROW(survivor->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(departedOwner->disconnect());
  REQUIRE_NOTHROW(survivor->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
