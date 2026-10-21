#include "tck_timed_save_mixed_fqr_tar.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

void runTimedSaveMixedFqrTar(Options const& options, rti::CallbackModel model) {
  require(model == rti::HLA_EVOKED,
          "Mixed FQR/TAR timed save requires evoked callbacks");
  require(!options.fom.empty(),
          "Mixed FQR/TAR timed save requires an adapter-supplied FOM");
  require(!options.logicalTimeImplementationName.empty(),
          "Mixed FQR/TAR timed save requires an adapter-supplied logical-time implementation");

  Session owner(options, model, "owner");
  Session first(options, model, "member");
  Session second(options, model, "member");
  Session third(options, model, "member");
  auto const federation = federationName(options, "timed-save-mixed-fqr-tar");
  std::wstring const saveLabel = L"tck-timed-save-mixed-fqr-tar";
  owner.connect();
  first.connect();
  second.connect();
  third.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);
  first.join(options.memberFederateName + L"-first", options.federateType, federation);
  second.join(options.memberFederateName + L"-second", options.federateType, federation);
  third.join(options.memberFederateName + L"-third", options.federateType, federation);

  auto const time = makeTimeContext(owner);
  require(time.factory->getName() == makeTimeContext(first).factory->getName() &&
              time.factory->getName() == makeTimeContext(second).factory->getName() &&
              time.factory->getName() == makeTimeContext(third).factory->getName(),
          "Mixed FQR/TAR members selected different logical-time factories");
  auto const saveTime = timeAfter(*time.factory, *time.initial, *time.epsilon, 5U);
  auto const flushTime = timeAfter(*time.factory, *time.initial, *time.epsilon, 6U);
  auto const laterFlushTime = timeAfter(*time.factory, *time.initial, *time.epsilon, 7U);

  for (auto* member : {&first, &second, &third}) {
    member->rtiAmbassador().enableTimeConstrained();
    waitFor(*member,
            [&] { return member->recorder().timeConstrainedEnabled().size() == 1U; },
            options, "mixed FQR/TAR constrained-role enablement");
    member->recorder().clearCallbackOrder();
  }
  owner.rtiAmbassador().enableTimeRegulation(*time.epsilon);
  waitFor(owner,
          [&] { return owner.recorder().timeRegulationEnabled().size() == 1U; },
          options, "mixed FQR/TAR regulator enablement");
  owner.recorder().clearCallbackOrder();

  owner.rtiAmbassador().requestFederationSave(saveLabel, *saveTime);
  first.rtiAmbassador().flushQueueRequest(*flushTime);
  second.rtiAmbassador().flushQueueRequest(*laterFlushTime);
  third.rtiAmbassador().timeAdvanceRequest(*saveTime);
  owner.rtiAmbassador().timeAdvanceRequest(*saveTime);

  waitFor(first,
          [&] {
            return first.federateSaveInitiations().size() == 1U &&
                first.recorder().flushQueueGrants().size() == 1U;
          }, options, "first mixed FQR save initiation and grant");
  require(first.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
              first.federateSaveTimes().size() == 1U &&
              first.federateSaveTimes().front().encoded == encodeTime(*saveTime) &&
              first.recorder().flushQueueGrants().front().time == encodeTime(*flushTime) &&
              first.recorder().callbackOrder() ==
                  std::vector<std::string>{"federate-save-initiate", "flush-grant"},
          "First mixed FQR member did not initiate save before grant 6");
  require(second.federateSaveInitiations().empty() &&
              third.federateSaveInitiations().empty() &&
              owner.federateSaveInitiations().empty(),
          "Other mixed FQR/TAR members received save initiation without callback service");

  waitFor(second,
          [&] {
            return second.federateSaveInitiations().size() == 1U &&
                second.recorder().flushQueueGrants().size() == 1U;
          }, options, "second mixed FQR save initiation and grant");
  require(second.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
              second.federateSaveTimes().size() == 1U &&
              second.federateSaveTimes().front().encoded == encodeTime(*saveTime) &&
              second.recorder().flushQueueGrants().front().time == encodeTime(*flushTime) &&
              second.recorder().callbackOrder() ==
                  std::vector<std::string>{"federate-save-initiate", "flush-grant"},
          "Second mixed FQR member did not initiate save before actual grant 6");
  require(third.federateSaveInitiations().empty() &&
              owner.federateSaveInitiations().empty(),
          "TAR or owner member received save initiation without callback service");

  waitFor(third,
          [&] {
            return third.federateSaveInitiations().size() == 1U &&
                third.recorder().timeAdvanceGrants().size() == 1U;
          }, options, "ordinary TAR mixed save initiation and grant");
  require(third.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
              third.federateSaveTimes().size() == 1U &&
              third.federateSaveTimes().front().encoded == encodeTime(*saveTime) &&
              third.recorder().timeAdvanceGrants().front().encoded == encodeTime(*saveTime) &&
              third.recorder().callbackOrder() ==
                  std::vector<std::string>{"federate-save-initiate", "grant"},
          "Ordinary TAR member did not initiate save before its boundary grant");
  require(owner.federateSaveInitiations().empty(),
          "Owner received save initiation without callback service");

  waitFor(owner,
          [&] { return owner.federateSaveInitiations().size() == 1U; },
          options, "mixed FQR/TAR owner save initiation");
  require(owner.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
              owner.federateSaveTimes().size() == 1U &&
              owner.federateSaveTimes().front().encoded == encodeTime(*saveTime),
          "Owner did not receive matching timed save initiation");

  for (auto* member : {&owner, &first, &second, &third}) {
    member->rtiAmbassador().federateSaveBegun();
  }
  for (auto* member : {&owner, &first, &second, &third}) {
    member->rtiAmbassador().federateSaveComplete();
  }
  waitForSessions(
      {&owner, &first, &second, &third},
      [&] {
        return owner.federationSavedCount() == 1U &&
            first.federationSavedCount() == 1U &&
            second.federationSavedCount() == 1U &&
            third.federationSavedCount() == 1U;
      }, options, "four-member mixed FQR/TAR save completion");
  for (auto* member : {&owner, &first, &second, &third}) {
    require(member->federationNotSavedReasons().empty(),
            "Mixed FQR/TAR member received Federation Not Saved");
  }

  for (auto* member : {&first, &second, &third}) {
    member->rtiAmbassador().disableTimeConstrained();
    member->resign(rti::NO_ACTION);
  }
  owner.rtiAmbassador().disableTimeRegulation();
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  third.disconnect();
  second.disconnect();
  first.disconnect();
  owner.disconnect();
}

}  // namespace

void scenarioTimedSaveMixedFqrTar(
    Options const& options, rti::CallbackModel model) {
  runTimedSaveMixedFqrTar(options, model);
}

void scenarioTimedSaveMixedFqrTarContract(
    Options const& options, rti::CallbackModel model) {
  runTimedSaveMixedFqrTar(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
