#include "ieee1516_2025_federation_management_fixture_support.hpp"

#include <chrono>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <thread>

namespace {
TEST_CASE(
    "Embedded ownership assumption search advances after a declined callback",
    "[integration][development-profile][ownership-management][federation-management]"
    "[rti.service.unconditional-attribute-ownership-divestiture]"
    "[rti.service.attribute-ownership-acquisition]"
    "[rti.service.cancel-attribute-ownership-acquisition]"
    "[federate.callback.request-attribute-ownership-assumption]"
    "[federate.callback.confirm-attribute-ownership-acquisition-cancellation]"
    "[ownership-assumption-search-continuation]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador firstCandidateReports;
  ReportingFederateAmbassador secondCandidateReports;
  auto owner = makeRti();
  auto firstCandidate = makeRti();
  auto secondCandidate = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(firstCandidate->connect(firstCandidateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(secondCandidate->connect(secondCandidateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"ownership-continuation-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(firstCandidate->joinFederationExecution(
      L"ownership-continuation-first", L"candidate", federationName));
  REQUIRE_NOTHROW(secondCandidate->joinFederationExecution(
      L"ownership-continuation-second", L"candidate", federationName));

  auto const server = owner->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = owner->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  AttributeHandleSet const candidateAttributes{efficiency};
  AttributeHandleSet const divestedAttributes{efficiency};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, candidateAttributes));
  REQUIRE_NOTHROW(firstCandidate->subscribeObjectClassAttributes(server, candidateAttributes));
  REQUIRE_NOTHROW(secondCandidate->subscribeObjectClassAttributes(server, candidateAttributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(server));
  while (firstCandidate->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  while (secondCandidate->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(firstCandidateReports.objectDiscoveryReports.size() == 1);
  REQUIRE(secondCandidateReports.objectDiscoveryReports.size() == 1);
  REQUIRE_NOTHROW(firstCandidate->publishObjectClassAttributes(server, candidateAttributes));
  REQUIRE_NOTHROW(secondCandidate->publishObjectClassAttributes(server, candidateAttributes));

  // Put the second candidate in the cancellation-confirmation boundary. It
  // is therefore not an eligible assumption recipient at divestiture time,
  // but becomes eligible when its already-queued cancellation confirmation is
  // delivered from the first candidate's assumption callback.
  unsigned char const acquisitionTagBytes[] = {0xC1, 0x25};
  unsigned char const divestitureTagBytes[] = {0xC2, 0x25};
  VariableLengthData const acquisitionTag(acquisitionTagBytes, sizeof(acquisitionTagBytes));
  VariableLengthData const divestitureTag(divestitureTagBytes, sizeof(divestitureTagBytes));
  REQUIRE_NOTHROW(secondCandidate->attributeOwnershipAcquisition(
      objectInstance,
      candidateAttributes,
      acquisitionTag));
  REQUIRE_NOTHROW(secondCandidate->cancelAttributeOwnershipAcquisition(
      objectInstance,
      candidateAttributes));

  REQUIRE_NOTHROW(owner->unconditionalAttributeOwnershipDivestiture(
      objectInstance,
      divestedAttributes,
      divestitureTag));
  REQUIRE(secondCandidateReports.attributeOwnershipAssumptionReports.empty());

  std::mutex callbackCoordinationMutex;
  std::condition_variable callbackCoordination;
  bool firstAssumptionCallbackEntered = false;
  bool secondCancellationCallbackRan = false;
  std::exception_ptr secondCancellationThreadException;
  secondCandidateReports.onConfirmAttributeOwnershipAcquisitionCancellation = [&] {
    std::scoped_lock lock(callbackCoordinationMutex);
    secondCancellationCallbackRan = true;
    callbackCoordination.notify_all();
  };
  firstCandidateReports.onRequestAttributeOwnershipAssumption = [&] {
    // Complete the second candidate's pending cancellation while the first
    // candidate's assumption callback is in user code. No declaration event
    // follows this transition; the callback return itself must advance the
    // original unowned search.
    {
      std::scoped_lock lock(callbackCoordinationMutex);
      firstAssumptionCallbackEntered = true;
    }
    callbackCoordination.notify_all();
    std::unique_lock lock(callbackCoordinationMutex);
    callbackCoordination.wait_for(
        lock,
        std::chrono::seconds(5),
        [&] { return secondCancellationCallbackRan; });
  };
  std::thread secondCancellationThread([&] {
    {
      std::unique_lock lock(callbackCoordinationMutex);
      callbackCoordination.wait_for(
          lock,
          std::chrono::seconds(5),
          [&] { return firstAssumptionCallbackEntered; });
    }
    try {
      for (;;) {
        bool const evokeResult = secondCandidate->evokeCallback(0.0);
        std::scoped_lock lock(callbackCoordinationMutex);
        if (secondCancellationCallbackRan || !evokeResult) {
          callbackCoordination.notify_all();
          break;
        }
      }
    } catch (...) {
      std::scoped_lock lock(callbackCoordinationMutex);
      secondCancellationThreadException = std::current_exception();
      secondCancellationCallbackRan = true;
      callbackCoordination.notify_all();
    }
  });
  std::exception_ptr firstCandidateCallbackException;
  try {
    while (firstCandidate->evokeMultipleCallbacks(0.0, 0.0)) {
    }
  } catch (...) {
    firstCandidateCallbackException = std::current_exception();
  }
  secondCancellationThread.join();
  if (firstCandidateCallbackException) {
    std::rethrow_exception(firstCandidateCallbackException);
  }
  if (secondCancellationThreadException) {
    std::rethrow_exception(secondCancellationThreadException);
  }

  REQUIRE(firstCandidateReports.attributeOwnershipAssumptionReports.size() == 1);
  REQUIRE(secondCandidateReports.attributeOwnershipAcquisitionCancellationReports.size() == 1);
  REQUIRE(secondCancellationCallbackRan);
  REQUIRE(secondCandidateReports.attributeOwnershipAssumptionReports.empty());

  while (secondCandidate->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(secondCandidateReports.attributeOwnershipAssumptionReports.size() == 1);
  auto const& continuedAssumption =
      secondCandidateReports.attributeOwnershipAssumptionReports.front();
  REQUIRE(continuedAssumption.objectInstance == objectInstance);
  REQUIRE(continuedAssumption.attributes == candidateAttributes);
  REQUIRE(variableLengthDataBytes(continuedAssumption.userSuppliedTag) ==
          std::vector<unsigned char>(
              divestitureTagBytes,
              divestitureTagBytes + sizeof(divestitureTagBytes)));

  REQUIRE_NOTHROW(firstCandidate->unpublishObjectClass(server));
  REQUIRE_NOTHROW(secondCandidate->unpublishObjectClass(server));
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  while (firstCandidate->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  while (secondCandidate->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE_NOTHROW(firstCandidate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(secondCandidate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(secondCandidate->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(firstCandidate->disconnect());
  REQUIRE_NOTHROW(secondCandidate->disconnect());
}

} // namespace
