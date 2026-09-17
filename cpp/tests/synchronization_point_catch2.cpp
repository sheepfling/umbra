#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The synchronization-point tests require the Umbra source directory."
#endif

namespace {

using rti1516_2025::CallbackModel;
using rti1516_2025::FederateHandle;
using rti1516_2025::FederateHandleSet;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::HLA_IMMEDIATE;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::SynchronizationPointFailureReason;
using rti1516_2025::VariableLengthData;

class ReportingFederateAmbassador final : public NullFederateAmbassador {
 public:
  struct RegistrationReport {
    std::wstring label;
    bool succeeded = false;
    SynchronizationPointFailureReason failureReason =
        rti1516_2025::SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE;
  };

  struct AnnouncementReport {
    std::wstring label;
    VariableLengthData userSuppliedTag;
  };

  struct SynchronizedReport {
    std::wstring label;
    FederateHandleSet failedToSyncSet;
  };

  void synchronizationPointRegistrationSucceeded(
      std::wstring const& label) override {
    registrationReports.push_back({label, true});
  }

  void synchronizationPointRegistrationFailed(
      std::wstring const& label,
      SynchronizationPointFailureReason reason) override {
    registrationReports.push_back({label, false, reason});
  }

  void announceSynchronizationPoint(
      std::wstring const& label,
      VariableLengthData const& userSuppliedTag) override {
    announcementReports.push_back({label, userSuppliedTag});
  }

  void federationSynchronized(
      std::wstring const& label,
      FederateHandleSet const& failedToSyncSet) override {
    synchronizedReports.push_back({label, failedToSyncSet});
  }

  std::vector<RegistrationReport> registrationReports;
  std::vector<AnnouncementReport> announcementReports;
  std::vector<SynchronizedReport> synchronizedReports;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "third_party" / "ieee1516.2-2025" / "resources" / relativePath;
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"synchronization-point-scope-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::vector<unsigned char> variableLengthDataBytes(
    VariableLengthData const& value) {
  auto const* bytes = static_cast<unsigned char const*>(value.data());
  if (bytes == nullptr || value.size() == 0U) {
    return {};
  }
  return {bytes, bytes + value.size()};
}

void drainCallbacks(RTIambassador& rti, CallbackModel callbackModel) {
  if (callbackModel == HLA_EVOKED) {
    while (rti.evokeCallback(0.0)) {
    }
  }
}

void runSynchronizationScenario(CallbackModel callbackModel) {
  ReportingFederateAmbassador firstReports;
  ReportingFederateAmbassador secondReports;
  ReportingFederateAmbassador lateReports;
  ReportingFederateAmbassador newcomerReports;
  auto first = makeRti();
  auto second = makeRti();
  auto late = makeRti();
  auto newcomer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const scopedTagBytes[] = {0x53U, 0x43U, 0x4FU, 0x50U, 0x45U};
  unsigned char const defaultTagBytes[] = {0x47U, 0x4CU, 0x4FU, 0x42U, 0x41U, 0x4CU};
  VariableLengthData const scopedTag(scopedTagBytes, sizeof(scopedTagBytes));
  VariableLengthData const defaultTag(defaultTagBytes, sizeof(defaultTagBytes));

  REQUIRE_NOTHROW(first->connect(firstReports, callbackModel));
  REQUIRE_NOTHROW(second->connect(secondReports, callbackModel));
  REQUIRE_NOTHROW(late->connect(lateReports, callbackModel));
  REQUIRE_NOTHROW(newcomer->connect(newcomerReports, callbackModel));
  REQUIRE_NOTHROW(first->createFederationExecution(
      federationName, fomModule, L"HLAinteger64Time"));
  auto const firstHandle = first->joinFederationExecution(
      L"sync-first", L"sync", federationName);
  auto const secondHandle = second->joinFederationExecution(
      L"sync-second", L"sync", federationName);

  FederateHandleSet explicitSet;
  explicitSet.insert(firstHandle);
  explicitSet.insert(secondHandle);
  REQUIRE_NOTHROW(first->registerFederationSynchronizationPoint(
      L"explicit-scope", scopedTag, explicitSet));
  drainCallbacks(*first, callbackModel);
  drainCallbacks(*second, callbackModel);
  REQUIRE(firstReports.registrationReports.size() == 1U);
  REQUIRE(firstReports.registrationReports.front().succeeded);
  REQUIRE(firstReports.announcementReports.size() == 1U);
  REQUIRE(firstReports.announcementReports.front().label == L"explicit-scope");
  REQUIRE(variableLengthDataBytes(firstReports.announcementReports.front().userSuppliedTag) ==
      std::vector<unsigned char>(scopedTagBytes, scopedTagBytes + sizeof(scopedTagBytes)));
  REQUIRE(secondReports.announcementReports.size() == 1U);
  REQUIRE(secondReports.announcementReports.front().label == L"explicit-scope");

  // An explicitly scoped point must not be broadened to a late joiner.
  REQUIRE_NOTHROW(late->joinFederationExecution(
      L"sync-late", L"sync", federationName));
  drainCallbacks(*late, callbackModel);
  REQUIRE(lateReports.announcementReports.empty());

  // Duplicate labels are reported asynchronously and leave the original
  // point intact.
  REQUIRE_NOTHROW(second->registerFederationSynchronizationPoint(
      L"explicit-scope", scopedTag));
  drainCallbacks(*second, callbackModel);
  REQUIRE(secondReports.registrationReports.size() == 1U);
  REQUIRE_FALSE(secondReports.registrationReports.front().succeeded);
  REQUIRE(secondReports.registrationReports.front().failureReason ==
      rti1516_2025::SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE);

  REQUIRE_NOTHROW(first->synchronizationPointAchieved(L"explicit-scope"));
  REQUIRE_NOTHROW(second->synchronizationPointAchieved(
      L"explicit-scope", false));
  drainCallbacks(*first, callbackModel);
  drainCallbacks(*second, callbackModel);
  REQUIRE(firstReports.synchronizedReports.size() == 1U);
  REQUIRE(secondReports.synchronizedReports.size() == 1U);
  REQUIRE(firstReports.synchronizedReports.front().label == L"explicit-scope");
  REQUIRE(secondReports.synchronizedReports.front().label == L"explicit-scope");
  REQUIRE(firstReports.synchronizedReports.front().failedToSyncSet.contains(secondHandle));
  REQUIRE(secondReports.synchronizedReports.front().failedToSyncSet.contains(secondHandle));
  REQUIRE_THROWS_AS(
      first->synchronizationPointAchieved(L"explicit-scope"),
      rti1516_2025::SynchronizationPointLabelNotAnnounced);

  // The overload without a set includes all current members and expands to a
  // late joiner until the point completes.
  REQUIRE_NOTHROW(first->registerFederationSynchronizationPoint(
      L"default-scope", defaultTag));
  drainCallbacks(*first, callbackModel);
  drainCallbacks(*second, callbackModel);
  drainCallbacks(*late, callbackModel);
  REQUIRE(firstReports.registrationReports.size() == 2U);
  REQUIRE(firstReports.registrationReports.back().succeeded);
  REQUIRE(firstReports.announcementReports.size() == 2U);
  REQUIRE(secondReports.announcementReports.size() == 2U);
  REQUIRE(lateReports.announcementReports.size() == 1U);
  REQUIRE(lateReports.announcementReports.front().label == L"default-scope");
  REQUIRE(variableLengthDataBytes(lateReports.announcementReports.front().userSuppliedTag) ==
      std::vector<unsigned char>(defaultTagBytes, defaultTagBytes + sizeof(defaultTagBytes)));

  REQUIRE_NOTHROW(newcomer->joinFederationExecution(
      L"sync-newcomer", L"sync", federationName));
  drainCallbacks(*newcomer, callbackModel);
  REQUIRE(newcomerReports.announcementReports.size() == 1U);
  REQUIRE(newcomerReports.announcementReports.front().label == L"default-scope");

  REQUIRE_NOTHROW(first->synchronizationPointAchieved(L"default-scope"));
  REQUIRE_NOTHROW(second->synchronizationPointAchieved(L"default-scope"));
  REQUIRE_NOTHROW(late->synchronizationPointAchieved(L"default-scope"));
  REQUIRE_NOTHROW(newcomer->synchronizationPointAchieved(L"default-scope"));
  drainCallbacks(*first, callbackModel);
  drainCallbacks(*second, callbackModel);
  drainCallbacks(*late, callbackModel);
  drainCallbacks(*newcomer, callbackModel);
  REQUIRE(firstReports.synchronizedReports.size() == 2U);
  REQUIRE(secondReports.synchronizedReports.size() == 2U);
  REQUIRE(lateReports.synchronizedReports.size() == 1U);
  REQUIRE(newcomerReports.synchronizedReports.size() == 1U);
  REQUIRE(firstReports.synchronizedReports.back().label == L"default-scope");
  REQUIRE(secondReports.synchronizedReports.back().label == L"default-scope");
  REQUIRE(lateReports.synchronizedReports.back().label == L"default-scope");
  REQUIRE(newcomerReports.synchronizedReports.back().label == L"default-scope");
  REQUIRE(firstReports.synchronizedReports.back().failedToSyncSet.empty());
  REQUIRE(secondReports.synchronizedReports.back().failedToSyncSet.empty());
  REQUIRE(lateReports.synchronizedReports.back().failedToSyncSet.empty());
  REQUIRE(newcomerReports.synchronizedReports.back().failedToSyncSet.empty());
  REQUIRE_THROWS_AS(
      first->synchronizationPointAchieved(L"default-scope"),
      rti1516_2025::SynchronizationPointLabelNotAnnounced);

  REQUIRE_NOTHROW(newcomer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(late->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(second->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(first->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(first->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(newcomer->disconnect());
  REQUIRE_NOTHROW(late->disconnect());
  REQUIRE_NOTHROW(second->disconnect());
  REQUIRE_NOTHROW(first->disconnect());
}

}  // namespace

TEST_CASE(
    "Embedded synchronization points announce, track achievement, and complete",
    "[integration][development-profile][federation-management][synchronization]"
    "[callbacks][rti.service.register-federation-synchronization-point]"
    "[rti.service.synchronization-point-achieved]"
    "[federate.callback.synchronization-point-registration]"
    "[federate.callback.announce-synchronization-point]"
    "[federate.callback.federation-synchronized]") {
  runSynchronizationScenario(HLA_EVOKED);
}

TEST_CASE(
    "Embedded synchronization points dispatch callbacks directly in HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][synchronization]"
    "[callbacks][callback-immediate][rti.service.register-federation-synchronization-point]"
    "[rti.service.synchronization-point-achieved]"
    "[federate.callback.synchronization-point-registration]"
    "[federate.callback.announce-synchronization-point]"
    "[federate.callback.federation-synchronized]") {
  runSynchronizationScenario(HLA_IMMEDIATE);
}
