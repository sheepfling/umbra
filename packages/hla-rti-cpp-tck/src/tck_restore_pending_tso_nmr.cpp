#include "tck_restore_pending_tso_nmr.hpp"

#include <algorithm>
#include <array>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <string>
#include <thread>

namespace hla_rti_cpp_tck::detail {
namespace {

void runRestorePendingTso(
    Options const& options, rti::CallbackModel model, bool available) {
  require(!options.fom.empty(),
          "Pending-TSO NMR restore requires an adapter-supplied standard interaction FOM");
  require(!options.logicalTimeImplementationName.empty(),
          "Pending-TSO NMR restore requires an adapter-supplied logical-time implementation");
  require(model == rti::HLA_EVOKED || model == rti::HLA_IMMEDIATE,
          "Pending-TSO NMR restore requires a standard callback model");
  bool const immediate = model == rti::HLA_IMMEDIATE;

  Session publisher(options, model, "owner");
  Session regulator(options, model, "member");
  Session receiver(options, model, "member");
  auto const federation = federationName(
      options, available ? "restore-pending-tso-nmra" : "restore-pending-tso-nmr");
  std::wstring const saveLabel = available ? L"tck-pending-nmra-tso-replay"
                                            : L"tck-pending-nmr-tso-replay";
  std::string const payload = available ? "NMRA-TSO" : "NMR-TSO";
  std::string const tagBytes = available ? "NMRA-REPLAY" : "RESTORE";
  std::vector<std::uint8_t> const expectedPayload(payload.begin(), payload.end());
  std::vector<std::uint8_t> const expectedTag(tagBytes.begin(), tagBytes.end());
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());

  publisher.connect();
  regulator.connect();
  receiver.connect();
  publisher.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  publisher.join(options.ownerFederateName, options.federateType, federation);
  regulator.join(available ? L"tck-pending-nmra-independent-regulator"
                           : L"tck-pending-nmr-independent-regulator", options.federateType,
                 federation);
  receiver.join(options.memberFederateName, options.federateType, federation);

  auto const pubTime = makeTimeContext(publisher);
  auto const regTime = makeTimeContext(regulator);
  auto const recvTime = makeTimeContext(receiver);
  require(pubTime.factory->getName() == regTime.factory->getName() &&
              pubTime.factory->getName() == recvTime.factory->getName(),
          "Pending-TSO NMR members selected different time factories");
  auto const time4 = timeAfter(*pubTime.factory, *pubTime.initial, *pubTime.epsilon, 4U);
  auto const time5 = timeAfter(*pubTime.factory, *pubTime.initial, *pubTime.epsilon, 5U);
  auto const time9 = timeAfter(*pubTime.factory, *pubTime.initial, *pubTime.epsilon, 9U);
  auto const time10 = timeAfter(*pubTime.factory, *pubTime.initial, *pubTime.epsilon, 10U);
  auto const encoded4 = encodeTime(*time4);
  auto const encoded5 = encodeTime(*time5);
  auto const encoded9 = encodeTime(*time9);
  auto queryTime = [&](Session& member, rti::LogicalTimeFactory& factory) {
    auto result = factory.makeInitial();
    require(result != nullptr, "Could not allocate Query Logical Time result");
    member.rtiAmbassador().queryLogicalTime(*result);
    return result;
  };
  auto pumpAll = [&] {
    bool invoked = false;
    do {
      invoked = publisher.rtiAmbassador().evokeCallback(0.0);
      invoked = regulator.rtiAmbassador().evokeCallback(0.0) || invoked;
      invoked = receiver.rtiAmbassador().evokeCallback(0.0) || invoked;
    } while (invoked);
  };

  auto const interaction =
      publisher.rtiAmbassador().getInteractionClassHandle(options.interactionClassName);
  auto const parameter =
      publisher.rtiAmbassador().getParameterHandle(interaction, options.parameterName);
  require(interaction.isValid() && parameter.isValid(),
          "Adapter FOM lacks the configured interaction or parameter");
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(parameter,
                     rti::VariableLengthData(payload.data(), payload.size()));
  publisher.rtiAmbassador().publishInteractionClass(interaction);
  publisher.rtiAmbassador().changeInteractionOrderType(interaction, rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeInteractionClass(interaction);
  publisher.rtiAmbassador().enableTimeConstrained();
  regulator.rtiAmbassador().enableTimeConstrained();
  receiver.rtiAmbassador().enableTimeConstrained();
  if (!immediate) {
    waitForSessions(
        {&publisher, &regulator, &receiver},
        [&] {
          return publisher.recorder().timeConstrainedEnabled().size() == 1U &&
              regulator.recorder().timeConstrainedEnabled().size() == 1U &&
              receiver.recorder().timeConstrainedEnabled().size() == 1U;
        }, options, "three pending-TSO NMR constrained enablements");
  }
  require(publisher.recorder().timeConstrainedEnabled().size() == 1U &&
              regulator.recorder().timeConstrainedEnabled().size() == 1U &&
              receiver.recorder().timeConstrainedEnabled().size() == 1U,
          "Pending-TSO NMR constrained enablement was incomplete");
  publisher.rtiAmbassador().enableTimeRegulation(*pubTime.epsilon);
  regulator.rtiAmbassador().enableTimeRegulation(*regTime.epsilon);
  if (!immediate) {
    waitForSessions(
        {&publisher, &regulator, &receiver},
        [&] {
          return publisher.recorder().timeRegulationEnabled().size() == 1U &&
              regulator.recorder().timeRegulationEnabled().size() == 1U;
        }, options, "two pending-TSO NMR regulation enablements");
  }
  require(publisher.recorder().timeRegulationEnabled().size() == 1U &&
              regulator.recorder().timeRegulationEnabled().size() == 1U,
          "Pending-TSO NMR regulation enablement was incomplete");

  if (available) {
    receiver.rtiAmbassador().nextMessageRequestAvailable(*time10);
  } else {
    receiver.rtiAmbassador().nextMessageRequest(*time10);
  }
  require(receiver.recorder().timedInteractions().empty(),
          "Receiver saw TSO before the queued send");
  auto const retraction = publisher.rtiAmbassador().sendInteraction(
      interaction, parameters, tag, *time5);
  require(retraction.isValid() && receiver.recorder().timedInteractions().empty() &&
              receiver.recorder().timeAdvanceGrants().empty(),
          "Timestamped interaction was not queued behind pending NMR");
  receiver.rtiAmbassador().requestFederationSave(saveLabel);
  require(publisher.federateSaveInitiations().empty() &&
              receiver.federateSaveInitiations().empty(),
          "Save initiated before the three-member advance boundary");

  std::mutex boundaryMutex;
  std::condition_variable boundary;
  std::size_t completedSaves = 0;
  std::array<std::exception_ptr, 3> threadFailures{};
  auto saveHook = [&](Session& member) {
    member.rtiAmbassador().federateSaveBegun();
    member.rtiAmbassador().federateSaveComplete();
    {
      std::scoped_lock lock(boundaryMutex);
      ++completedSaves;
    }
    boundary.notify_all();
    if (!immediate) {
      std::unique_lock lock(boundaryMutex);
      boundary.wait_for(lock, std::chrono::seconds(5), [&] {
        return completedSaves == 3U ||
            std::any_of(threadFailures.begin(), threadFailures.end(),
                        [](auto const& error) { return static_cast<bool>(error); });
      });
    }
  };
  publisher.recorder().setFederateSaveInitiateHook([&] { saveHook(publisher); });
  regulator.recorder().setFederateSaveInitiateHook([&] { saveHook(regulator); });
  receiver.recorder().setFederateSaveInitiateHook([&] { saveHook(receiver); });
  publisher.recorder().clearCallbackOrder();
  regulator.recorder().clearCallbackOrder();
  receiver.recorder().clearCallbackOrder();
  if (immediate) {
    publisher.rtiAmbassador().disableCallbacks();
    regulator.rtiAmbassador().disableCallbacks();
    receiver.rtiAmbassador().disableCallbacks();
  }
  publisher.rtiAmbassador().timeAdvanceRequest(available ? *time4 : *time5);
  regulator.rtiAmbassador().timeAdvanceRequest(available ? *time4 : *time5);
  if (immediate) {
    publisher.rtiAmbassador().enableCallbacks();
    regulator.rtiAmbassador().enableCallbacks();
    receiver.rtiAmbassador().enableCallbacks();
  } else {
    auto drainOnThread = [&](std::size_t index, Session& member) {
      try {
        while (member.rtiAmbassador().evokeCallback(0.0)) {
        }
      } catch (...) {
        {
          std::scoped_lock lock(boundaryMutex);
          threadFailures[index] = std::current_exception();
        }
        boundary.notify_all();
      }
    };
    std::thread first([&] { drainOnThread(0U, publisher); });
    std::thread second([&] { drainOnThread(1U, regulator); });
    std::thread third([&] { drainOnThread(2U, receiver); });
    first.join();
    second.join();
    third.join();
    for (auto const& error : threadFailures) {
      if (error) std::rethrow_exception(error);
    }
  }
  require(completedSaves == 3U &&
              publisher.federateSaveInitiations() ==
                  std::vector<std::wstring>{saveLabel} &&
              regulator.federateSaveInitiations() ==
                  std::vector<std::wstring>{saveLabel} &&
              receiver.federateSaveInitiations() ==
                  std::vector<std::wstring>{saveLabel} &&
              publisher.federationSavedCount() == 1U &&
              regulator.federationSavedCount() == 1U &&
              receiver.federationSavedCount() == 1U &&
              publisher.recorder().timeAdvanceGrants().size() == 1U &&
              regulator.recorder().timeAdvanceGrants().size() == 1U &&
              receiver.recorder().timeAdvanceGrants().size() == 1U &&
              publisher.recorder().timeAdvanceGrants().back().encoded ==
                  (available ? encoded4 : encoded5) &&
              regulator.recorder().timeAdvanceGrants().back().encoded ==
                  (available ? encoded4 : encoded5) &&
              receiver.recorder().timeAdvanceGrants().back().encoded == encoded5 &&
              receiver.recorder().timedInteractions().size() == 1U,
          "Pending-TSO NMR save boundary or first delivery was incomplete");
  auto const original = receiver.recorder().timedInteractions().front();
  require(original.interaction == interaction &&
              original.producer == publisher.federateHandle() &&
              original.time == encoded5 &&
              original.sentOrder == rti::TIMESTAMP &&
              original.receivedOrder == rti::TIMESTAMP &&
              original.retractionPresent && original.tag == expectedTag &&
              original.parameters.size() == 1U &&
              copyBytes(original.parameters.at(parameter)) == expectedPayload,
          "Original timestamped interaction did not preserve public callback fields");
  auto const originalOrder = receiver.recorder().callbackOrder();
  auto const saveInitiate =
      std::find(originalOrder.begin(), originalOrder.end(), "federate-save-initiate");
  auto const originalInteraction =
      std::find(originalOrder.begin(), originalOrder.end(), "interaction");
  auto const originalGrant =
      std::find(originalOrder.begin(), originalOrder.end(), "grant");
  require(saveInitiate != originalOrder.end() &&
              originalInteraction != originalOrder.end() &&
              originalGrant != originalOrder.end() &&
              saveInitiate < originalInteraction &&
              originalInteraction < originalGrant,
          "Original timestamped interaction did not precede NMR grant");

  publisher.recorder().setFederateSaveInitiateHook({});
  regulator.recorder().setFederateSaveInitiateHook({});
  receiver.recorder().setFederateSaveInitiateHook({});
  publisher.rtiAmbassador().timeAdvanceRequest(*time9);
  regulator.rtiAmbassador().timeAdvanceRequest(*time9);
  receiver.rtiAmbassador().timeAdvanceRequest(*time9);
  if (!immediate) {
    waitForSessions(
        {&publisher, &regulator, &receiver},
        [&] {
          return publisher.recorder().timeAdvanceGrants().size() == 2U &&
              regulator.recorder().timeAdvanceGrants().size() == 2U &&
              receiver.recorder().timeAdvanceGrants().size() == 2U;
        }, options, "post-save three-member time-9 grants");
  }
  require(publisher.recorder().timeAdvanceGrants().size() == 2U &&
              regulator.recorder().timeAdvanceGrants().size() == 2U &&
              receiver.recorder().timeAdvanceGrants().size() == 2U &&
              receiver.recorder().timeAdvanceGrants().back().encoded == encoded9 &&
              *queryTime(publisher, *pubTime.factory) == *time9 &&
              *queryTime(regulator, *regTime.factory) == *time9 &&
              *queryTime(receiver, *recvTime.factory) == *time9,
          "Post-save advances did not reach time 9 for all members");

  publisher.recorder().clearCallbackOrder();
  regulator.recorder().clearCallbackOrder();
  receiver.recorder().clearCallbackOrder();
  receiver.rtiAmbassador().requestFederationRestore(saveLabel);
  if (!immediate) {
    waitForSessions(
        {&publisher, &regulator, &receiver},
        [&] {
          return publisher.federateRestoreInitiations().size() == 1U &&
              regulator.federateRestoreInitiations().size() == 1U &&
              receiver.federateRestoreInitiations().size() == 1U;
        }, options, "three pending-TSO NMR restore initiations");
  }
  publisher.rtiAmbassador().federateRestoreComplete();
  regulator.rtiAmbassador().federateRestoreComplete();
  receiver.rtiAmbassador().federateRestoreComplete();
  if (!immediate) {
    waitForSessions(
        {&publisher, &regulator, &receiver},
        [&] {
          return publisher.federationRestoredCount() == 1U &&
              regulator.federationRestoredCount() == 1U &&
              receiver.federationRestoredCount() == 1U &&
              receiver.recorder().timedInteractions().size() == 2U &&
              receiver.recorder().timeAdvanceGrants().size() == 3U;
        }, options, "restored pending-TSO NMR delivery and grant");
  }
  require(publisher.federationRestoredCount() == 1U &&
              regulator.federationRestoredCount() == 1U &&
              receiver.federationRestoredCount() == 1U &&
              receiver.recorder().timedInteractions().size() == 2U &&
              receiver.recorder().timeAdvanceGrants().size() == 3U &&
              receiver.recorder().timeAdvanceGrants().back().encoded == encoded5,
          "Restore did not replay one interaction before the restored NMR grant");
  auto const replayed = receiver.recorder().timedInteractions().back();
  require(replayed.interaction == original.interaction &&
              replayed.producer == original.producer &&
              replayed.time == original.time &&
              replayed.sentOrder == original.sentOrder &&
              replayed.receivedOrder == original.receivedOrder &&
              replayed.retractionPresent == original.retractionPresent &&
              replayed.tag == expectedTag &&
              replayed.parameters.size() == 1U &&
              copyBytes(replayed.parameters.at(parameter)) == expectedPayload,
          "Replayed timestamped interaction changed public callback fields");
  auto const restoreOrder = receiver.recorder().callbackOrder();
  auto const restored =
      std::find(restoreOrder.begin(), restoreOrder.end(), "federation-restored");
  auto const delivered =
      std::find(restoreOrder.begin(), restoreOrder.end(), "interaction");
  auto const grant = std::find(restoreOrder.begin(), restoreOrder.end(), "grant");
  require(restored != restoreOrder.end() && delivered != restoreOrder.end() &&
              grant != restoreOrder.end() && restored < delivered &&
              delivered < grant &&
              *queryTime(publisher, *pubTime.factory) == (available ? *time4 : *time5) &&
              *queryTime(regulator, *regTime.factory) == (available ? *time4 : *time5) &&
              *queryTime(receiver, *recvTime.factory) == *time5,
          "Restored interaction was not delivered between restoration and grant");

  publisher.resign(rti::NO_ACTION);
  regulator.resign(rti::NO_ACTION);
  receiver.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  regulator.disconnect();
  publisher.disconnect();
}

}  // namespace

void scenarioRestorePendingTsoNmr(
    Options const& options, rti::CallbackModel model) {
  runRestorePendingTso(options, model, false);
}

void scenarioRestorePendingTsoNmrContract(
    Options const& options, rti::CallbackModel model) {
  runRestorePendingTso(options, model, false);
}

void scenarioRestorePendingTsoNmra(
    Options const& options, rti::CallbackModel model) {
  runRestorePendingTso(options, model, true);
}

void scenarioRestorePendingTsoNmraContract(
    Options const& options, rti::CallbackModel model) {
  runRestorePendingTso(options, model, true);
}

}  // namespace hla_rti_cpp_tck::detail
