#include "tck_restore_pending_tso_nmra_delete.hpp"

#include <algorithm>
#include <array>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <string>
#include <thread>

namespace hla_rti_cpp_tck::detail {
namespace {

void runRestorePendingTsoNmraDelete(
    Options const& options, rti::CallbackModel model) {
  require(!options.fom.empty(),
          "Pending-TSO NMRA deletion restore requires an adapter-supplied standard object FOM");
  require(!options.logicalTimeImplementationName.empty(),
          "Pending-TSO NMRA deletion restore requires an adapter-supplied logical-time implementation");
  require(model == rti::HLA_EVOKED || model == rti::HLA_IMMEDIATE,
          "Pending-TSO NMRA deletion restore requires a standard callback model");
  bool const immediate = model == rti::HLA_IMMEDIATE;

  Session publisher(options, model, "owner");
  Session regulator(options, model, "member");
  Session receiver(options, model, "member");
  auto const federation = federationName(options, "restore-pending-tso-nmra-delete");
  std::wstring const saveLabel = L"tck-pending-nmra-delete-replay";
  std::string const tagBytes = "NMRA-DEL";
  std::vector<std::uint8_t> const expectedTag(tagBytes.begin(), tagBytes.end());
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());

  publisher.connect();
  regulator.connect();
  receiver.connect();
  publisher.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  publisher.join(options.ownerFederateName, options.federateType, federation);
  regulator.join(L"tck-pending-nmra-delete-regulator", options.federateType,
                 federation);
  receiver.join(options.memberFederateName, options.federateType, federation);

  auto const pubTime = makeTimeContext(publisher);
  auto const regTime = makeTimeContext(regulator);
  auto const recvTime = makeTimeContext(receiver);
  require(pubTime.factory->getName() == regTime.factory->getName() &&
              pubTime.factory->getName() == recvTime.factory->getName(),
          "Pending-TSO NMRA deletion members selected different time factories");
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

  auto const objectClass =
      publisher.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const reliableAttribute =
      publisher.rtiAmbassador().getAttributeHandle(objectClass, options.attributeName);
  auto const bestEffortAttribute = publisher.rtiAmbassador().getAttributeHandle(
      objectClass, options.secondaryAttributeName);
  require(objectClass.isValid() && reliableAttribute.isValid() &&
              bestEffortAttribute.isValid() && reliableAttribute != bestEffortAttribute,
          "Adapter FOM lacks the configured object class or two distinct attributes");
  rti::AttributeHandleSet const attributes{reliableAttribute, bestEffortAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(objectClass, attributes);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(objectClass, attributes);
  auto const object = publisher.rtiAmbassador().registerObjectInstance(objectClass);
  if (!immediate) {
    waitForSessions({&publisher, &regulator, &receiver},
        [&] { return receiver.recorder().hasDiscovery(object); },
        options, "pending-NMRA deletion object discovery");
  }
  require(publisher.recorder().discoveryCount() == 0U &&
              receiver.recorder().discoveryCount() == 1U &&
              receiver.recorder().discovery().object == object &&
              receiver.recorder().discovery().objectClass == objectClass,
          "Pending-NMRA deletion object was not discovered exactly once");

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
        }, options, "three pending-TSO NMRA deletion constrained enablements");
  }
  require(publisher.recorder().timeConstrainedEnabled().size() == 1U &&
              regulator.recorder().timeConstrainedEnabled().size() == 1U &&
              receiver.recorder().timeConstrainedEnabled().size() == 1U,
          "Pending-TSO NMRA deletion constrained enablement was incomplete");
  publisher.rtiAmbassador().enableTimeRegulation(*pubTime.epsilon);
  regulator.rtiAmbassador().enableTimeRegulation(*regTime.epsilon);
  if (!immediate) {
    waitForSessions(
        {&publisher, &regulator, &receiver},
        [&] {
          return publisher.recorder().timeRegulationEnabled().size() == 1U &&
              regulator.recorder().timeRegulationEnabled().size() == 1U;
        }, options, "two pending-TSO NMRA deletion regulation enablements");
  }
  require(publisher.recorder().timeRegulationEnabled().size() == 1U &&
              regulator.recorder().timeRegulationEnabled().size() == 1U,
          "Pending-TSO NMRA deletion regulation enablement was incomplete");

  receiver.rtiAmbassador().nextMessageRequestAvailable(*time10);
  require(receiver.recorder().timedRemovals().empty(),
          "Receiver saw TSO before the queued send");
  auto const retraction = publisher.rtiAmbassador().deleteObjectInstance(
      object, tag, *time5);
  require(retraction.isValid() && receiver.recorder().timedRemovals().empty() &&
              receiver.recorder().timeAdvanceGrants().empty(),
          "Timestamped object deletion was not queued behind pending NMRA");
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
  publisher.rtiAmbassador().timeAdvanceRequest(*time4);
  regulator.rtiAmbassador().timeAdvanceRequest(*time4);
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
                  encoded4 &&
              regulator.recorder().timeAdvanceGrants().back().encoded ==
                  encoded4 &&
              receiver.recorder().timeAdvanceGrants().back().encoded == encoded5 &&
              receiver.recorder().timedRemovals().size() == 1U,
          "Pending-TSO NMRA deletion save boundary or first delivery was incomplete");
  auto const original = receiver.recorder().timedRemovals().front();
  require(original.object == object &&
              original.producer == publisher.federateHandle() &&
              original.time == encoded5 &&
              original.sentOrder == rti::TIMESTAMP &&
              original.receivedOrder == rti::TIMESTAMP &&
              original.retractionPresent && original.tag == expectedTag,
          "Original timestamped object removal changed public callback fields");
  auto const originalOrder = receiver.recorder().callbackOrder();
  auto const saveInitiate =
      std::find(originalOrder.begin(), originalOrder.end(), "federate-save-initiate");
  auto const originalRemoval =
      std::find(originalOrder.begin(), originalOrder.end(), "remove");
  auto const originalGrant =
      std::find(originalOrder.begin(), originalOrder.end(), "grant");
  require(saveInitiate != originalOrder.end() &&
              originalRemoval != originalOrder.end() &&
              originalGrant != originalOrder.end() &&
              saveInitiate < originalRemoval &&
              originalRemoval < originalGrant,
          "Original timestamped object removal did not precede NMRA grant");

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
        }, options, "three pending-TSO NMRA deletion restore initiations");
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
              receiver.recorder().timedRemovals().size() == 2U &&
              receiver.recorder().timeAdvanceGrants().size() == 3U;
        }, options, "restored pending-TSO NMRA deletion delivery and grant");
  }
  require(publisher.federationRestoredCount() == 1U &&
              regulator.federationRestoredCount() == 1U &&
              receiver.federationRestoredCount() == 1U &&
              receiver.recorder().timedRemovals().size() == 2U &&
              receiver.recorder().timeAdvanceGrants().size() == 3U &&
              receiver.recorder().timeAdvanceGrants().back().encoded == encoded5,
          "Restore did not replay one object removal before the restored NMRA grant");
  auto const replayed = receiver.recorder().timedRemovals().back();
  require(replayed.object == original.object &&
              replayed.producer == original.producer &&
              replayed.time == original.time &&
              replayed.sentOrder == original.sentOrder &&
              replayed.receivedOrder == original.receivedOrder &&
              replayed.retractionPresent == original.retractionPresent &&
              replayed.tag == expectedTag,
          "Replayed timestamped object removal changed public callback fields");
  auto const restoreOrder = receiver.recorder().callbackOrder();
  auto const restored =
      std::find(restoreOrder.begin(), restoreOrder.end(), "federation-restored");
  auto const delivered =
      std::find(restoreOrder.begin(), restoreOrder.end(), "remove");
  auto const grant = std::find(restoreOrder.begin(), restoreOrder.end(), "grant");
  require(restored != restoreOrder.end() && delivered != restoreOrder.end() &&
              grant != restoreOrder.end() && restored < delivered &&
              delivered < grant &&
              *queryTime(publisher, *pubTime.factory) == *time4 &&
              *queryTime(regulator, *regTime.factory) == *time4 &&
              *queryTime(receiver, *recvTime.factory) == *time5,
          "Restored removal was not delivered between restoration and grant");

  publisher.resign(rti::NO_ACTION);
  regulator.resign(rti::NO_ACTION);
  receiver.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  regulator.disconnect();
  publisher.disconnect();
}

}  // namespace

void scenarioRestorePendingTsoNmraDelete(
    Options const& options, rti::CallbackModel model) {
  runRestorePendingTsoNmraDelete(options, model);
}

void scenarioRestorePendingTsoNmraDeleteContract(
    Options const& options, rti::CallbackModel model) {
  runRestorePendingTsoNmraDelete(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
