#include "tck_restore_pending_tso_nmra_attribute.hpp"

#include <algorithm>
#include <array>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <string>
#include <thread>

namespace hla_rti_cpp_tck::detail {
namespace {

void runRestorePendingTsoNmraAttribute(
    Options const& options, rti::CallbackModel model) {
  require(!options.fom.empty(),
          "Pending-TSO NMRA attribute restore requires an adapter-supplied standard object FOM");
  require(!options.logicalTimeImplementationName.empty(),
          "Pending-TSO NMRA attribute restore requires an adapter-supplied logical-time implementation");
  require(model == rti::HLA_EVOKED || model == rti::HLA_IMMEDIATE,
          "Pending-TSO NMRA attribute restore requires a standard callback model");
  bool const immediate = model == rti::HLA_IMMEDIATE;

  Session publisher(options, model, "owner");
  Session regulator(options, model, "member");
  Session receiver(options, model, "member");
  auto const federation = federationName(options, "restore-pending-tso-nmra-attribute");
  std::wstring const saveLabel = L"tck-pending-nmra-attribute-replay";
  std::array<std::uint8_t, 3> const payload{0x51, 0x22, 0x73};
  std::string const tagBytes = "NMRA-UPD";
  std::vector<std::uint8_t> const expectedPayload(payload.begin(), payload.end());
  std::vector<std::uint8_t> const expectedTag(tagBytes.begin(), tagBytes.end());
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());

  publisher.connect();
  regulator.connect();
  receiver.connect();
  publisher.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  publisher.join(options.ownerFederateName, options.federateType, federation);
  regulator.join(L"tck-pending-nmra-attribute-regulator", options.federateType,
                 federation);
  receiver.join(options.memberFederateName, options.federateType, federation);

  auto const pubTime = makeTimeContext(publisher);
  auto const regTime = makeTimeContext(regulator);
  auto const recvTime = makeTimeContext(receiver);
  require(pubTime.factory->getName() == regTime.factory->getName() &&
              pubTime.factory->getName() == recvTime.factory->getName(),
          "Pending-TSO NMRA attribute members selected different time factories");
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
  auto const attribute =
      publisher.rtiAmbassador().getAttributeHandle(objectClass, options.attributeName);
  require(objectClass.isValid() && attribute.isValid(),
          "Adapter FOM lacks the configured object class or attribute");
  rti::AttributeHandleSet const attributes{attribute};
  rti::AttributeHandleValueMap values;
  values.emplace(attribute,
                 rti::VariableLengthData(payload.data(), payload.size()));
  publisher.rtiAmbassador().publishObjectClassAttributes(objectClass, attributes);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(objectClass, attributes);
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      objectClass, attributes, rti::TIMESTAMP);
  auto const object = publisher.rtiAmbassador().registerObjectInstance(objectClass);
  if (!immediate) {
    waitForSessions({&publisher, &regulator, &receiver},
        [&] { return receiver.recorder().hasDiscovery(object); },
        options, "pending-NMRA attribute object discovery");
  }
  require(publisher.recorder().discoveryCount() == 0U &&
              receiver.recorder().discoveryCount() == 1U &&
              receiver.recorder().discovery().object == object &&
              receiver.recorder().discovery().objectClass == objectClass,
          "Pending-NMRA attribute object was not discovered exactly once");

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
        }, options, "three pending-TSO NMRA attribute constrained enablements");
  }
  require(publisher.recorder().timeConstrainedEnabled().size() == 1U &&
              regulator.recorder().timeConstrainedEnabled().size() == 1U &&
              receiver.recorder().timeConstrainedEnabled().size() == 1U,
          "Pending-TSO NMRA attribute constrained enablement was incomplete");
  publisher.rtiAmbassador().enableTimeRegulation(*pubTime.epsilon);
  regulator.rtiAmbassador().enableTimeRegulation(*regTime.epsilon);
  if (!immediate) {
    waitForSessions(
        {&publisher, &regulator, &receiver},
        [&] {
          return publisher.recorder().timeRegulationEnabled().size() == 1U &&
              regulator.recorder().timeRegulationEnabled().size() == 1U;
        }, options, "two pending-TSO NMRA attribute regulation enablements");
  }
  require(publisher.recorder().timeRegulationEnabled().size() == 1U &&
              regulator.recorder().timeRegulationEnabled().size() == 1U,
          "Pending-TSO NMRA attribute regulation enablement was incomplete");

  receiver.rtiAmbassador().nextMessageRequestAvailable(*time10);
  require(receiver.recorder().timedReflections().empty(),
          "Receiver saw TSO before the queued send");
  auto const retraction = publisher.rtiAmbassador().updateAttributeValues(
      object, values, tag, *time5);
  require(retraction.isValid() && receiver.recorder().timedReflections().empty() &&
              receiver.recorder().timeAdvanceGrants().empty(),
          "Timestamped attribute update was not queued behind pending NMRA");
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
              receiver.recorder().timedReflections().size() == 1U,
          "Pending-TSO NMRA attribute save boundary or first delivery was incomplete");
  auto const original = receiver.recorder().timedReflections().front();
  require(original.object == object &&
              original.producer == publisher.federateHandle() &&
              original.time == encoded5 &&
              original.sentOrder == rti::TIMESTAMP &&
              original.receivedOrder == rti::TIMESTAMP &&
              original.retractionPresent && original.tag == expectedTag &&
              original.values.size() == 1U &&
              copyBytes(original.values.at(attribute)) == expectedPayload,
          "Original timestamped attribute reflection changed public callback fields");
  auto const originalOrder = receiver.recorder().callbackOrder();
  auto const saveInitiate =
      std::find(originalOrder.begin(), originalOrder.end(), "federate-save-initiate");
  auto const originalReflection =
      std::find(originalOrder.begin(), originalOrder.end(), "reflect");
  auto const originalGrant =
      std::find(originalOrder.begin(), originalOrder.end(), "grant");
  require(saveInitiate != originalOrder.end() &&
              originalReflection != originalOrder.end() &&
              originalGrant != originalOrder.end() &&
              saveInitiate < originalReflection &&
              originalReflection < originalGrant,
          "Original timestamped attribute reflection did not precede NMRA grant");

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
        }, options, "three pending-TSO NMRA attribute restore initiations");
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
              receiver.recorder().timedReflections().size() == 2U &&
              receiver.recorder().timeAdvanceGrants().size() == 3U;
        }, options, "restored pending-TSO NMRA attribute delivery and grant");
  }
  require(publisher.federationRestoredCount() == 1U &&
              regulator.federationRestoredCount() == 1U &&
              receiver.federationRestoredCount() == 1U &&
              receiver.recorder().timedReflections().size() == 2U &&
              receiver.recorder().timeAdvanceGrants().size() == 3U &&
              receiver.recorder().timeAdvanceGrants().back().encoded == encoded5,
          "Restore did not replay one attribute reflection before the restored NMRA grant");
  auto const replayed = receiver.recorder().timedReflections().back();
  require(replayed.object == original.object &&
              replayed.producer == original.producer &&
              replayed.time == original.time &&
              replayed.sentOrder == original.sentOrder &&
              replayed.receivedOrder == original.receivedOrder &&
              replayed.retractionPresent == original.retractionPresent &&
              replayed.tag == expectedTag &&
              replayed.values.size() == 1U &&
              copyBytes(replayed.values.at(attribute)) == expectedPayload,
          "Replayed timestamped attribute reflection changed public callback fields");
  auto const restoreOrder = receiver.recorder().callbackOrder();
  auto const restored =
      std::find(restoreOrder.begin(), restoreOrder.end(), "federation-restored");
  auto const delivered =
      std::find(restoreOrder.begin(), restoreOrder.end(), "reflect");
  auto const grant = std::find(restoreOrder.begin(), restoreOrder.end(), "grant");
  require(restored != restoreOrder.end() && delivered != restoreOrder.end() &&
              grant != restoreOrder.end() && restored < delivered &&
              delivered < grant &&
              *queryTime(publisher, *pubTime.factory) == *time4 &&
              *queryTime(regulator, *regTime.factory) == *time4 &&
              *queryTime(receiver, *recvTime.factory) == *time5,
          "Restored reflection was not delivered between restoration and grant");

  publisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  regulator.resign(rti::NO_ACTION);
  receiver.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  regulator.disconnect();
  publisher.disconnect();
}

}  // namespace

void scenarioRestorePendingTsoNmraAttribute(
    Options const& options, rti::CallbackModel model) {
  runRestorePendingTsoNmraAttribute(options, model);
}

void scenarioRestorePendingTsoNmraAttributeContract(
    Options const& options, rti::CallbackModel model) {
  runRestorePendingTsoNmraAttribute(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
