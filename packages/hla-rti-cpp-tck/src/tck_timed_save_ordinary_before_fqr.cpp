#include "tck_timed_save_ordinary_before_fqr.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

void runTimedSaveOrdinaryBeforeFqr(
    Options const& options, rti::CallbackModel model) {
  require(model == rti::HLA_EVOKED,
          "Ordinary-before-FQR timed save requires evoked callbacks");
  require(!options.fom.empty(),
          "Ordinary-before-FQR timed save requires an adapter-supplied FOM");
  require(!options.logicalTimeImplementationName.empty(),
          "Ordinary-before-FQR timed save requires an adapter-supplied logical-time implementation");

  Session owner(options, model, "owner");
  Session ordinary(options, model, "member");
  Session flushQueue(options, model, "member");
  auto const federation = federationName(options, "timed-save-ordinary-before-fqr");
  std::wstring const saveLabel = L"tck-timed-save-ordinary-before-fqr";
  owner.connect();
  ordinary.connect();
  flushQueue.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);
  ordinary.join(options.memberFederateName + L"-ordinary", options.federateType,
                federation);
  flushQueue.join(options.memberFederateName + L"-fqr", options.federateType,
                  federation);

  auto const time = makeTimeContext(owner);
  require(time.factory->getName() == makeTimeContext(ordinary).factory->getName() &&
              time.factory->getName() == makeTimeContext(flushQueue).factory->getName(),
          "Ordinary-before-FQR members selected different logical-time factories");
  auto const saveTime = timeAfter(*time.factory, *time.initial, *time.epsilon, 5U);
  auto const flushTime = timeAfter(*time.factory, *time.initial, *time.epsilon, 6U);

  ordinary.rtiAmbassador().enableTimeConstrained();
  waitFor(ordinary,
          [&] { return ordinary.recorder().timeConstrainedEnabled().size() == 1U; },
          options, "ordinary member constrained-role enablement");
  flushQueue.rtiAmbassador().enableTimeConstrained();
  waitFor(flushQueue,
          [&] { return flushQueue.recorder().timeConstrainedEnabled().size() == 1U; },
          options, "FQR member constrained-role enablement");
  owner.rtiAmbassador().enableTimeRegulation(*time.epsilon);
  waitFor(owner,
          [&] { return owner.recorder().timeRegulationEnabled().size() == 1U; },
          options, "ordinary-before-FQR regulator enablement");
  owner.recorder().clearCallbackOrder();
  ordinary.recorder().clearCallbackOrder();
  flushQueue.recorder().clearCallbackOrder();

  owner.rtiAmbassador().requestFederationSave(saveLabel, *saveTime);
  flushQueue.rtiAmbassador().flushQueueRequest(*flushTime);
  ordinary.rtiAmbassador().timeAdvanceRequest(*saveTime);
  owner.rtiAmbassador().timeAdvanceRequest(*saveTime);

  waitFor(ordinary,
          [&] {
            return ordinary.federateSaveInitiations().size() == 1U &&
                ordinary.recorder().timeAdvanceGrants().size() == 1U;
          }, options, "ordinary boundary save initiation and grant");
  require(ordinary.federateSaveInitiations() ==
              std::vector<std::wstring>{saveLabel} &&
              ordinary.federateSaveTimes().size() == 1U &&
              ordinary.federateSaveTimes().front().encoded == encodeTime(*saveTime) &&
              ordinary.recorder().timeAdvanceGrants().front().encoded ==
                  encodeTime(*saveTime) &&
              ordinary.recorder().callbackOrder() ==
                  std::vector<std::string>{"federate-save-initiate", "grant"},
          "Ordinary TAR member did not initiate save before grant 5");
  require(flushQueue.federateSaveInitiations().empty() &&
              flushQueue.recorder().callbackOrder().empty() &&
              owner.federateSaveInitiations().empty(),
          "FQR or owner received save initiation without callback service");

  waitFor(flushQueue,
          [&] {
            return flushQueue.federateSaveInitiations().size() == 1U &&
                flushQueue.recorder().flushQueueGrants().size() == 1U;
          }, options, "FQR save initiation and grant after ordinary boundary");
  require(flushQueue.federateSaveInitiations() ==
              std::vector<std::wstring>{saveLabel} &&
              flushQueue.federateSaveTimes().size() == 1U &&
              flushQueue.federateSaveTimes().front().encoded == encodeTime(*saveTime) &&
              flushQueue.recorder().flushQueueGrants().front().time ==
                  encodeTime(*flushTime) &&
              flushQueue.recorder().callbackOrder() ==
                  std::vector<std::string>{"federate-save-initiate", "flush-grant"} &&
              ordinary.recorder().callbackOrder() ==
                  std::vector<std::string>{"federate-save-initiate", "grant"} &&
              owner.federateSaveInitiations().empty(),
          "FQR member did not initiate save before actual grant 6");

  waitFor(owner,
          [&] { return owner.federateSaveInitiations().size() == 1U; },
          options, "owner save initiation after ordinary and FQR members");
  require(owner.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
              owner.federateSaveTimes().size() == 1U &&
              owner.federateSaveTimes().front().encoded == encodeTime(*saveTime),
          "Owner did not receive matching timed save initiation");

  for (auto* member : {&owner, &ordinary, &flushQueue}) {
    member->rtiAmbassador().federateSaveBegun();
  }
  for (auto* member : {&owner, &ordinary, &flushQueue}) {
    member->rtiAmbassador().federateSaveComplete();
  }
  waitForSessions(
      {&owner, &ordinary, &flushQueue},
      [&] {
        return owner.federationSavedCount() == 1U &&
            ordinary.federationSavedCount() == 1U &&
            flushQueue.federationSavedCount() == 1U;
      }, options, "three-member ordinary-before-FQR save completion");
  for (auto* member : {&owner, &ordinary, &flushQueue}) {
    require(member->federationNotSavedReasons().empty(),
            "Ordinary-before-FQR member received Federation Not Saved");
  }

  flushQueue.rtiAmbassador().disableTimeConstrained();
  ordinary.rtiAmbassador().disableTimeConstrained();
  owner.rtiAmbassador().disableTimeRegulation();
  flushQueue.resign(rti::NO_ACTION);
  ordinary.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  flushQueue.disconnect();
  ordinary.disconnect();
  owner.disconnect();
}

}  // namespace

void scenarioTimedSaveOrdinaryBeforeFqr(
    Options const& options, rti::CallbackModel model) {
  runTimedSaveOrdinaryBeforeFqr(options, model);
}

void scenarioTimedSaveOrdinaryBeforeFqrContract(
    Options const& options, rti::CallbackModel model) {
  runTimedSaveOrdinaryBeforeFqr(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
