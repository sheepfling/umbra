#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The timestamped object-deletion source-resignation test requires the Umbra source directory."
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
  return L"timestamped-object-deletion-source-resignation-fanout-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path sourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / relativePath;
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct TimeReport final {
    std::wstring implementationName;
    std::wstring value;
  };

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

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const&,
      std::wstring const&,
      FederateHandle const&) override {
    objectDiscoveryReports.push_back(objectInstance);
  }

  void timeAdvanceGrant(LogicalTime const& time) override {
    timeAdvanceGrantReports.push_back({time.implementationName(), time.toString()});
    callbackOrder.push_back("grant");
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

  std::vector<ObjectInstanceHandle> objectDiscoveryReports;
  std::vector<RemovalReport> objectRemovalReports;
  std::vector<TimeReport> timeAdvanceGrantReports;
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
    "Embedded queued timestamped object deletion survives source resignation for each recipient",
    "[integration][development-profile][federation-management][object-management][time-management]"
    "[timestamped-object-deletion][tso][resignation][multi-federate-callback-ordering]"
    "[rti.service.delete-object-instance][rti.service.resign-federation-execution]"
    "[rti.service.unconditional-attribute-ownership-divestiture]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.time-advance-request][rti.service.next-message-request]"
    "[federate.callback.remove-object-instance][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador firstReports;
  ReportingFederateAmbassador secondReports;
  ReportingFederateAmbassador clockReports;
  auto owner = makeRti();
  auto first = makeRti();
  auto second = makeRti();
  auto clock = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = sourcePath(std::filesystem::path("cpp") / "tests" / "data" /
                                    "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const deletionTagBytes[] = {
      0x52, 0x45, 0x53, 0x49, 0x47, 0x2D, 0x44};
  VariableLengthData const deletionTag(deletionTagBytes, sizeof(deletionTagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(first->connect(firstReports, HLA_EVOKED));
  REQUIRE_NOTHROW(second->connect(secondReports, HLA_EVOKED));
  REQUIRE_NOTHROW(clock->connect(clockReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle ownerHandle;
  REQUIRE_NOTHROW(ownerHandle = owner->joinFederationExecution(
      L"tso-resigning-delete-owner",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(first->joinFederationExecution(
      L"tso-resigning-delete-first",
      L"subscriber",
      federationName));
  REQUIRE_NOTHROW(second->joinFederationExecution(
      L"tso-resigning-delete-second",
      L"subscriber",
      federationName));
  REQUIRE_NOTHROW(clock->joinFederationExecution(
      L"tso-resigning-delete-clock",
      L"publisher",
      federationName));

  auto const ownerClass = owner->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const ownerReliable = owner->getAttributeHandle(
      ownerClass,
      fixture_hla::fixture::reliable_base_a);
  auto const ownerPrivilege = owner->getAttributeHandle(
      ownerClass,
      standard_hla::mom::privilege_to_delete_object);
  auto const firstClass = first->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const firstReliable = first->getAttributeHandle(
      firstClass,
      fixture_hla::fixture::reliable_base_a);
  auto const secondClass = second->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const secondReliable = second->getAttributeHandle(
      secondClass,
      fixture_hla::fixture::reliable_base_a);
  REQUIRE(ownerClass.isValid());
  REQUIRE(ownerReliable.isValid());
  REQUIRE(ownerPrivilege.isValid());
  REQUIRE(firstReliable.isValid());
  REQUIRE(secondReliable.isValid());

  AttributeHandleSet const ownerPublishedAttributes{ownerReliable};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(ownerClass, ownerPublishedAttributes));
  REQUIRE_NOTHROW(first->subscribeObjectClassAttributes(
      firstClass,
      AttributeHandleSet{firstReliable}));
  REQUIRE_NOTHROW(second->subscribeObjectClassAttributes(
      secondClass,
      AttributeHandleSet{secondReliable}));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(ownerClass));
  drainCallbacks(*first);
  drainCallbacks(*second);
  REQUIRE(firstReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(secondReports.objectDiscoveryReports.size() == 1U);

  REQUIRE_NOTHROW(first->enableTimeConstrained());
  REQUIRE_NOTHROW(second->enableTimeConstrained());
  drainCallbacks(*first);
  drainCallbacks(*second);
  REQUIRE_NOTHROW(owner->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drainCallbacks(*owner);
  REQUIRE_NOTHROW(clock->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drainCallbacks(*clock);

  auto const retraction = owner->deleteObjectInstance(
      objectInstance,
      deletionTag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(retraction.isValid());
  REQUIRE(firstReports.objectRemovalReports.empty());
  REQUIRE(secondReports.objectRemovalReports.empty());

  // Admit each recipient's request while the source regulator is still
  // present; both requests must remain blocked at the accepted deletion's
  // timestamp until an independent regulator advances the frontier.
  REQUIRE_NOTHROW(first->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(second->nextMessageRequest(rti1516_2025::HLAinteger64Time(10)));
  REQUIRE(firstReports.objectRemovalReports.empty());
  REQUIRE(secondReports.objectRemovalReports.empty());

  // The accepted removal belongs to each recipient's temporal queue. A
  // voluntary source departure must not erase those recipient-local copies.
  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES));
  REQUIRE(firstReports.objectRemovalReports.empty());
  REQUIRE(secondReports.objectRemovalReports.empty());

  // Establish an independent temporal frontier after the source leaves. The
  // regulator remains in the execution and can release both queued requests.
  REQUIRE_NOTHROW(clock->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  drainCallbacks(*clock);
  REQUIRE(clockReports.timeAdvanceGrantReports.size() == 1U);
  firstReports.callbackOrder.clear();
  drainCallbacks(*first);
  REQUIRE(firstReports.objectRemovalReports.size() == 1U);
  REQUIRE(firstReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(firstReports.callbackOrder == std::vector<std::string>{"remove", "grant"});
  REQUIRE(secondReports.objectRemovalReports.empty());
  REQUIRE(secondReports.timeAdvanceGrantReports.empty());

  secondReports.callbackOrder.clear();
  drainCallbacks(*second);
  REQUIRE(secondReports.objectRemovalReports.size() == 1U);
  REQUIRE(secondReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(secondReports.callbackOrder == std::vector<std::string>{"remove", "grant"});

  auto requireRemoval = [&](auto const& report) {
    REQUIRE(report.objectInstance == objectInstance);
    // Remove Object Instance identifies the federate that produced the
    // object, not the separate federate that held HLAprivilegeToDeleteObject
    // and invoked the deletion service.
    REQUIRE(report.producer == ownerHandle);
    REQUIRE(variableLengthDataBytes(report.tag) ==
            std::vector<unsigned char>(
                deletionTagBytes,
                deletionTagBytes + sizeof(deletionTagBytes)));
    REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
    REQUIRE(report.timeValue == L"7");
    REQUIRE(report.sentOrderType == TIMESTAMP);
    REQUIRE(report.receivedOrderType == TIMESTAMP);
    REQUIRE(report.retractionSupplied);
    REQUIRE(report.retractionValid);
  };
  requireRemoval(firstReports.objectRemovalReports.front());
  requireRemoval(secondReports.objectRemovalReports.front());
  REQUIRE(firstReports.timeAdvanceGrantReports.front().value == L"7");
  REQUIRE(secondReports.timeAdvanceGrantReports.front().value == L"7");

  REQUIRE_NOTHROW(first->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(second->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(clock->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(first->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(first->disconnect());
  REQUIRE_NOTHROW(second->disconnect());
  REQUIRE_NOTHROW(clock->disconnect());
}
