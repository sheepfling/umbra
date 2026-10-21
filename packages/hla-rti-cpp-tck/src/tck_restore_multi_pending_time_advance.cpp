#include "tck_restore_multi_pending_time_advance.hpp"

#include <chrono>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

namespace hla_rti_cpp_tck::detail {
namespace {

void runRestoreMultiPendingTimeAdvance(Options const& options, rti::CallbackModel model) {
  require(model == rti::HLA_EVOKED,
          "Multi-member pending-TAR restore has native evidence only for evoked callbacks");
  require(!options.fom.empty() && !options.logicalTimeImplementationName.empty(),
          "Multi-member pending-TAR restore requires adapter NRG-enabled FOM and time implementation");

  Session first(options, model, "member");
  Session second(options, model, "member");
  auto const federation = federationName(options, "restore-multi-pending-time-advance");
  std::wstring const saveLabel = L"tck-multi-pending-time-advance";
  first.connect();
  second.connect();
  first.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  first.join(options.ownerFederateName, options.federateType, federation);
  second.join(options.memberFederateName, options.federateType, federation);

  auto const firstTime = makeTimeContext(first);
  auto const secondTime = makeTimeContext(second);
  require(firstTime.factory->getName() == secondTime.factory->getName(),
          "Multi-member pending-TAR members selected different time factories");
  auto const firstFive = timeAfter(*firstTime.factory, *firstTime.initial,
                                   *firstTime.epsilon, 5U);
  auto const secondFive = timeAfter(*secondTime.factory, *secondTime.initial,
                                    *secondTime.epsilon, 5U);
  auto const firstNine = timeAfter(*firstTime.factory, *firstTime.initial,
                                   *firstTime.epsilon, 9U);
  auto const secondNine = timeAfter(*secondTime.factory, *secondTime.initial,
                                    *secondTime.epsilon, 9U);
  auto const firstFiveEncoded = encodeTime(*firstFive);
  auto const secondFiveEncoded = encodeTime(*secondFive);
  auto const firstNineEncoded = encodeTime(*firstNine);
  auto const secondNineEncoded = encodeTime(*secondNine);
  auto queryTime = [](Session& session, rti::LogicalTimeFactory& factory) {
    auto value = factory.makeInitial();
    require(value != nullptr, "Could not allocate queried logical time");
    session.rtiAmbassador().queryLogicalTime(*value);
    return value;
  };
  auto drain = [](Session& session) {
    while (session.rtiAmbassador().evokeCallback(0.0)) {
    }
  };

  first.rtiAmbassador().enableTimeConstrained();
  second.rtiAmbassador().enableTimeConstrained();
  waitForSessions({&first, &second}, [&] {
    return first.recorder().timeConstrainedEnabled().size() == 1U &&
           second.recorder().timeConstrainedEnabled().size() == 1U;
  }, options, "both pending-TAR members constrained");

  first.recorder().clearCallbackOrder();
  second.recorder().clearCallbackOrder();
  std::mutex boundaryMutex;
  std::condition_variable boundary;
  bool firstSaved = false;
  bool secondSaved = false;
  std::exception_ptr secondRouteError;
  first.recorder().setFederateSaveInitiateHook([&] {
    first.rtiAmbassador().federateSaveBegun();
    first.rtiAmbassador().federateSaveComplete();
    {
      std::scoped_lock lock(boundaryMutex);
      firstSaved = true;
    }
    boundary.notify_all();
    std::unique_lock lock(boundaryMutex);
    require(boundary.wait_for(lock, std::chrono::seconds(10),
                              [&] { return secondSaved || secondRouteError; }),
            "Second pending-TAR save callback did not complete at the boundary");
  });
  second.recorder().setFederateSaveInitiateHook([&] {
    second.rtiAmbassador().federateSaveBegun();
    second.rtiAmbassador().federateSaveComplete();
    {
      std::scoped_lock lock(boundaryMutex);
      secondSaved = true;
    }
    boundary.notify_all();
  });

  first.rtiAmbassador().requestFederationSave(saveLabel);
  require(first.federateSaveInitiations().empty() &&
              second.federateSaveInitiations().empty(),
          "Evoked multi-member save initiation arrived before servicing");
  first.rtiAmbassador().timeAdvanceRequest(*firstFive);
  second.rtiAmbassador().timeAdvanceRequest(*secondFive);
  std::thread secondRoute([&] {
    {
      std::unique_lock lock(boundaryMutex);
      if (!boundary.wait_for(lock, std::chrono::seconds(10),
                             [&] { return firstSaved; })) {
        secondRouteError = std::make_exception_ptr(
            std::runtime_error("First pending-TAR save callback did not reach boundary"));
        boundary.notify_all();
        return;
      }
    }
    try {
      static_cast<void>(second.rtiAmbassador().evokeCallback(0.0));
    } catch (...) {
      std::scoped_lock lock(boundaryMutex);
      secondRouteError = std::current_exception();
      boundary.notify_all();
    }
  });
  std::exception_ptr firstRouteError;
  try {
    static_cast<void>(first.rtiAmbassador().evokeCallback(0.0));
  } catch (...) {
    firstRouteError = std::current_exception();
    boundary.notify_all();
  }
  secondRoute.join();
  if (firstRouteError) std::rethrow_exception(firstRouteError);
  if (secondRouteError) std::rethrow_exception(secondRouteError);
  require(first.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
              second.federateSaveInitiations() == std::vector<std::wstring>{saveLabel},
          "Both independent save callback routes did not initiate");
  drain(first);
  drain(second);
  require(first.federationSavedCount() == 1U && second.federationSavedCount() == 1U &&
              first.federationNotSavedReasons().empty() &&
              second.federationNotSavedReasons().empty(),
          "Multi-member pending-TAR save did not complete for both members");
  require(first.recorder().timeAdvanceGrants().size() == 1U,
          "First initial pending-TAR grant was not delivered exactly once");
  require(second.recorder().timeAdvanceGrants().size() == 1U,
          "Second initial pending-TAR grant was not delivered exactly once");
  require(first.recorder().timeAdvanceGrants().back().encoded == firstFiveEncoded,
          "First initial pending-TAR grant reached a different target");
  require(second.recorder().timeAdvanceGrants().back().encoded == secondFiveEncoded,
          "Second initial pending-TAR grant reached a different target");

  first.recorder().setFederateSaveInitiateHook({});
  second.recorder().setFederateSaveInitiateHook({});
  first.rtiAmbassador().timeAdvanceRequest(*firstNine);
  second.rtiAmbassador().timeAdvanceRequest(*secondNine);
  waitForSessions({&first, &second}, [&] {
    return first.recorder().timeAdvanceGrants().size() == 2U &&
           second.recorder().timeAdvanceGrants().size() == 2U;
  }, options, "post-save grants on both routes");
  require(first.recorder().timeAdvanceGrants().back().encoded == firstNineEncoded &&
              second.recorder().timeAdvanceGrants().back().encoded == secondNineEncoded &&
              *queryTime(first, *firstTime.factory) == *firstNine &&
              *queryTime(second, *secondTime.factory) == *secondNine,
          "Post-save time advances did not reach both later targets");

  first.recorder().clearCallbackOrder();
  second.recorder().clearCallbackOrder();
  first.rtiAmbassador().requestFederationRestore(saveLabel);
  waitForSessions({&first, &second}, [&] {
    return first.federateRestoreInitiations().size() == 1U &&
           second.federateRestoreInitiations().size() == 1U;
  }, options, "multi-member pending-TAR restore initiation");
  require(first.federationRestoreRequestsSucceeded() ==
                  std::vector<std::wstring>{saveLabel} &&
              first.federateRestoreInitiations().front().label == saveLabel &&
              second.federateRestoreInitiations().front().label == saveLabel,
          "Multi-member pending-TAR restore used an unexpected label");
  first.recorder().clearCallbackOrder();
  second.recorder().clearCallbackOrder();
  first.rtiAmbassador().federateRestoreComplete();
  second.rtiAmbassador().federateRestoreComplete();

  drain(first);
  require(first.federationRestoredCount() == 1U &&
              first.recorder().timeAdvanceGrants().size() == 3U &&
              second.federationRestoredCount() == 0U &&
              second.recorder().timeAdvanceGrants().size() == 2U &&
              second.recorder().callbackOrder().empty(),
          "First restored pending TAR consumed or released the second route");
  drain(second);
  require(second.federationRestoredCount() == 1U &&
              second.recorder().timeAdvanceGrants().size() == 3U,
          "Second restored pending TAR did not complete independently");
  for (int pass = 0; pass != 3; ++pass) {
    drain(first);
    drain(second);
  }
  auto const firstOrder = first.recorder().callbackOrder();
  auto const secondOrder = second.recorder().callbackOrder();
  require(first.recorder().timeAdvanceGrants().back().encoded == firstFiveEncoded &&
              second.recorder().timeAdvanceGrants().back().encoded == secondFiveEncoded &&
              *queryTime(first, *firstTime.factory) == *firstFive &&
              *queryTime(second, *secondTime.factory) == *secondFive &&
              firstOrder.size() >= 2U && secondOrder.size() >= 2U &&
              std::vector<std::string>(firstOrder.end() - 2, firstOrder.end()) ==
                  std::vector<std::string>{"federation-restored", "grant"} &&
              std::vector<std::string>(secondOrder.end() - 2, secondOrder.end()) ==
                  std::vector<std::string>{"federation-restored", "grant"} &&
              first.federationNotRestoredReasons().empty() &&
              second.federationNotRestoredReasons().empty(),
          "Restored pending-TAR grants did not follow Federation Restored independently");

  second.resign(rti::NO_ACTION);
  first.resign(rti::NO_ACTION);
  first.rtiAmbassador().destroyFederationExecution(federation);
  second.disconnect();
  first.disconnect();
}

}  // namespace

void scenarioRestoreMultiPendingTimeAdvance(Options const& options, rti::CallbackModel model) {
  runRestoreMultiPendingTimeAdvance(options, model);
}

void scenarioRestoreMultiPendingTimeAdvanceContract(
    Options const& options, rti::CallbackModel model) {
  runRestoreMultiPendingTimeAdvance(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
