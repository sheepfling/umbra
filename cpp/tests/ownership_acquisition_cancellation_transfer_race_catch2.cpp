#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <exception>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The ownership acquisition cancellation transfer-race test requires the Umbra source directory."
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
  return L"ownership-acquisition-cancellation-transfer-race-" +
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
    if (onRequestAttributeOwnershipRelease) {
      onRequestAttributeOwnershipRelease();
    }
    attributeOwnershipReleaseRequestReports.push_back({
        objectInstance,
        candidateAttributes,
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

  struct AttributeOwnershipReleaseRequestReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  struct AttributeOwnershipAcquisitionCancellationReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
  };

  std::vector<ObjectInstanceHandle> objectDiscoveryReports;
  std::vector<AttributeOwnershipAcquisitionReport>
      attributeOwnershipAcquisitionReports;
  std::vector<AttributeOwnershipReleaseRequestReport>
      attributeOwnershipReleaseRequestReports;
  std::vector<AttributeOwnershipAcquisitionCancellationReport>
      attributeOwnershipAcquisitionCancellationReports;
  std::function<void()> onRequestAttributeOwnershipRelease;
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
    "Embedded cancellation race returns Attribute Ownership Acquisition Notification after transfer",
    "[integration][development-profile][ownership-management]"
    "[rti.service.cancel-attribute-ownership-acquisition]"
    "[rti.service.attribute-ownership-acquisition]"
    "[rti.service.attribute-ownership-divestiture-if-wanted]"
    "[federate.callback.request-attribute-ownership-release]"
    "[federate.callback.confirm-attribute-ownership-acquisition-cancellation]"
    "[federate.callback.attribute-ownership-acquisition-notification]"
    "[ownership-acquisition-cancellation-race]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = testDataPath(
      "attribute-update-passel-fom.xml").wstring();
  unsigned char const acquisitionTagBytes[] = {0xC1, 0x25};
  unsigned char const divestitureTagBytes[] = {0xD1, 0x25};
  VariableLengthData const acquisitionTag(
      acquisitionTagBytes,
      sizeof(acquisitionTagBytes));
  VariableLengthData const divestitureTag(
      divestitureTagBytes,
      sizeof(divestitureTagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"cancellation-transfer-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"cancellation-transfer-requester", L"publisher", federationName));

  auto const child = owner->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const reliableBaseA = owner->getAttributeHandle(
      child,
      fixture_hla::fixture::reliable_base_a);
  AttributeHandleSet const attributes{reliableBaseA};
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, attributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  drainCallbacks(*requester);
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(child, attributes));
  drainCallbacks(*owner);
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance,
      attributes,
      acquisitionTag));

  std::mutex coordinationMutex;
  std::condition_variable coordination;
  bool releaseCallbackEntered = false;
  bool cancellationCompleted = false;
  std::exception_ptr cancellationFailure;
  std::exception_ptr divestitureFailure;
  ownerReports.onRequestAttributeOwnershipRelease = [&] {
    {
      std::lock_guard lock(coordinationMutex);
      releaseCallbackEntered = true;
    }
    coordination.notify_all();

    std::unique_lock lock(coordinationMutex);
    bool const observedCancellation = coordination.wait_for(
        lock,
        std::chrono::seconds(5),
        [&] { return cancellationCompleted; });
    lock.unlock();
    if (!observedCancellation) {
      return;
    }

    // Divestiture If Wanted may complete a transfer after cancellation has
    // been accepted. That transfer wins the terminal race, so the queued
    // cancellation confirmation must become stale and the acquisition
    // notification must be the only reply.
    try {
      AttributeHandleSet divestedAttributes;
      owner->attributeOwnershipDivestitureIfWanted(
          objectInstance,
          attributes,
          divestitureTag,
          divestedAttributes);
      if (divestedAttributes != attributes) {
        divestitureFailure = std::make_exception_ptr(
            std::runtime_error(
                "Divestiture If Wanted did not transfer the raced attribute."));
      }
    } catch (...) {
      divestitureFailure = std::current_exception();
    }
  };

  std::thread canceller([&] {
    {
      std::unique_lock lock(coordinationMutex);
      coordination.wait_for(
          lock,
          std::chrono::seconds(5),
          [&] { return releaseCallbackEntered; });
    }
    try {
      requester->cancelAttributeOwnershipAcquisition(objectInstance, attributes);
    } catch (...) {
      cancellationFailure = std::current_exception();
    }
    {
      std::lock_guard lock(coordinationMutex);
      cancellationCompleted = true;
    }
    coordination.notify_all();
  });

  REQUIRE_NOTHROW(owner->evokeCallback(0.0));
  canceller.join();
  REQUIRE_FALSE(cancellationFailure);
  REQUIRE_FALSE(divestitureFailure);
  ownerReports.onRequestAttributeOwnershipRelease = {};

  drainCallbacks(*requester);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.empty());
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.size() == 1U);
  auto const& notification = requesterReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(notification.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
  REQUIRE(notification.objectInstance == objectInstance);
  REQUIRE(notification.attributes == attributes);
  REQUIRE(variableLengthDataBytes(notification.userSuppliedTag) ==
          std::vector<unsigned char>(
              divestitureTagBytes,
              divestitureTagBytes + sizeof(divestitureTagBytes)));
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE(requester->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_NOTHROW(requester->unpublishObjectClassAttributes(child, attributes));

  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(requester->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}
