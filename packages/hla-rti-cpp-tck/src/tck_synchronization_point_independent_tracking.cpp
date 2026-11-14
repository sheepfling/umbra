#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

void requireTwoAnnouncements(
    Session& session,
    std::wstring const& firstLabel,
    std::vector<std::uint8_t> const& firstTag,
    std::wstring const& secondLabel,
    std::vector<std::uint8_t> const& secondTag,
    std::string const& description) {
  auto const announcements = session.synchronizationPointAnnouncements();
  require(
      announcements.size() == 2U,
      description + " did not receive exactly two synchronization announcements");
  bool firstObserved = false;
  bool secondObserved = false;
  for (auto const& announcement : announcements) {
    if (announcement.label == firstLabel && announcement.tag == firstTag) {
      firstObserved = true;
    } else if (announcement.label == secondLabel && announcement.tag == secondTag) {
      secondObserved = true;
    }
  }
  require(
      firstObserved && secondObserved,
      description + " did not preserve both synchronization labels and tags independently");
}

void requireTwoIndependentCompletions(
    Session& session,
    std::wstring const& firstLabel,
    std::wstring const& secondLabel,
    std::string const& description) {
  auto const completions = session.federationSynchronized();
  require(
      completions.size() == 2U,
      description + " did not receive exactly two synchronization completions");
  bool firstCompleted = false;
  bool secondCompleted = false;
  for (auto const& completion : completions) {
    require(
        completion.failedToSyncSet.empty(),
        description + " reported a failed federate for a fully achieved point");
    if (completion.label == firstLabel) {
      require(!firstCompleted, description + " duplicated the first point completion");
      firstCompleted = true;
    } else if (completion.label == secondLabel) {
      require(!secondCompleted, description + " duplicated the second point completion");
      secondCompleted = true;
    } else {
      require(false, description + " reported an unknown synchronization label");
    }
  }
  require(
      firstCompleted && secondCompleted,
      description + " did not complete both labels as distinct synchronization points");
}

}  // namespace

void scenarioSynchronizationPointIndependentTracking(
    Options const& options,
    rti::CallbackModel model) {
  Session registrar(options, model, "sync-independent-registrar");
  Session participant(options, model, "sync-independent-participant");
  Session lateJoiner(options, model, "sync-independent-late-joiner");
  auto const federation =
      federationName(options, "synchronization-points-independent-tracking");
  connectAndJoin(registrar, participant, options, federation, options.fom);

  std::wstring const firstLabel = L"tck-sync-independent-alpha";
  std::wstring const secondLabel = L"tck-sync-independent-beta";
  std::vector<std::uint8_t> const firstTagBytes{0x41U, 0x31U};
  std::vector<std::uint8_t> const secondTagBytes{0x42U, 0x32U};
  rti::VariableLengthData const firstTag(
      firstTagBytes.data(), firstTagBytes.size());
  rti::VariableLengthData const secondTag(
      secondTagBytes.data(), secondTagBytes.size());

  registrar.rtiAmbassador().registerFederationSynchronizationPoint(firstLabel, firstTag);
  registrar.rtiAmbassador().registerFederationSynchronizationPoint(secondLabel, secondTag);
  waitForSessions(
      {&registrar, &participant},
      [&] {
        return registrar.synchronizationPointRegistrations().size() >= 2U &&
            registrar.synchronizationPointAnnouncements().size() >= 2U &&
            participant.synchronizationPointAnnouncements().size() >= 2U;
      },
      options,
      "independent synchronization-point registration and announcement");

  auto const registrations = registrar.synchronizationPointRegistrations();
  require(
      registrations.size() == 2U,
      "registrar did not receive exactly two synchronization registration results");
  bool firstRegistered = false;
  bool secondRegistered = false;
  for (auto const& registration : registrations) {
    require(registration.succeeded, "a synchronization point registration failed");
    if (registration.label == firstLabel) {
      firstRegistered = true;
    } else if (registration.label == secondLabel) {
      secondRegistered = true;
    }
  }
  require(
      firstRegistered && secondRegistered &&
          participant.synchronizationPointRegistrations().empty(),
      "registration results were not associated with the two independent labels");
  requireTwoAnnouncements(
      registrar,
      firstLabel,
      firstTagBytes,
      secondLabel,
      secondTagBytes,
      "registrar");
  requireTwoAnnouncements(
      participant,
      firstLabel,
      firstTagBytes,
      secondLabel,
      secondTagBytes,
      "participant");

  registrar.rtiAmbassador().synchronizationPointAchieved(firstLabel);
  auto const waitingForMemberDeadline =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(250);
  while (std::chrono::steady_clock::now() < waitingForMemberDeadline) {
    registrar.pump();
    participant.pump();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  require(
      registrar.federationSynchronized().empty() &&
          participant.federationSynchronized().empty(),
      "Federation Synchronized was invoked before every synchronization-set member achieved the point");
  participant.rtiAmbassador().synchronizationPointAchieved(firstLabel);
  waitForSessions(
      {&registrar, &participant},
      [&] {
        return registrar.federationSynchronized().size() >= 1U &&
            participant.federationSynchronized().size() >= 1U;
      },
      options,
      "first synchronization point completion while second remains pending");
  auto const firstRegistrarCompletion = registrar.federationSynchronized();
  auto const firstParticipantCompletion = participant.federationSynchronized();
  require(
      firstRegistrarCompletion.size() == 1U &&
          firstRegistrarCompletion.front().label == firstLabel &&
          firstParticipantCompletion.size() == 1U &&
          firstParticipantCompletion.front().label == firstLabel,
      "completing the first label also completed or corrupted the second point");
  require(
      firstRegistrarCompletion.front().failedToSyncSet.empty() &&
          firstParticipantCompletion.front().failedToSyncSet.empty(),
      "Synchronization Point Achieved without a success indicator was not treated as successful");

  auto const quietDeadline =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(250);
  while (std::chrono::steady_clock::now() < quietDeadline) {
    registrar.pump();
    participant.pump();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  require(
      registrar.federationSynchronized().size() == 1U &&
          participant.federationSynchronized().size() == 1U,
      "the unachieved second synchronization point completed with the first");

  registrar.rtiAmbassador().synchronizationPointAchieved(secondLabel);
  participant.rtiAmbassador().synchronizationPointAchieved(secondLabel);
  waitForSessions(
      {&registrar, &participant},
      [&] {
        return registrar.federationSynchronized().size() >= 2U &&
            participant.federationSynchronized().size() >= 2U;
      },
      options,
      "second synchronization point completion after independent achievement");
  requireTwoIndependentCompletions(
      registrar, firstLabel, secondLabel, "registrar");
  requireTwoIndependentCompletions(
      participant, firstLabel, secondLabel, "participant");

  lateJoiner.connect();
  lateJoiner.join(
      L"tck-sync-independent-late-joiner", options.federateType, federation);
  auto const lateJoinQuietDeadline =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(250);
  while (std::chrono::steady_clock::now() < lateJoinQuietDeadline) {
    lateJoiner.pump();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  require(
      lateJoiner.synchronizationPointAnnouncements().empty(),
      "A federate joining after synchronization-point completion received an announcement");

  registrar.rtiAmbassador().registerFederationSynchronizationPoint(firstLabel, firstTag);
  waitForSessions(
      {&registrar, &participant, &lateJoiner},
      [&] {
        return registrar.synchronizationPointRegistrations().size() >= 3U &&
            registrar.synchronizationPointAnnouncements().size() >= 3U &&
            participant.synchronizationPointAnnouncements().size() >= 3U &&
            lateJoiner.synchronizationPointAnnouncements().size() >= 1U;
      },
      options,
      "registration of a completed synchronization label as a new point");
  auto const registrationsAfterReuse = registrar.synchronizationPointRegistrations();
  require(
      registrationsAfterReuse.size() == 3U &&
          registrationsAfterReuse.back().succeeded &&
          registrationsAfterReuse.back().label == firstLabel,
      "a completed synchronization label remained registered instead of being reusable");
  for (auto* session : {&registrar, &participant, &lateJoiner}) {
    auto const announcements = session->synchronizationPointAnnouncements();
    std::size_t const expectedCount = session == &lateJoiner ? 1U : 3U;
    require(
        announcements.size() == expectedCount &&
            announcements.back().label == firstLabel &&
            announcements.back().tag == firstTagBytes,
        "re-registering a completed label did not announce a fresh point to every current member");
  }

  registrar.rtiAmbassador().synchronizationPointAchieved(firstLabel);
  participant.rtiAmbassador().synchronizationPointAchieved(firstLabel);
  lateJoiner.rtiAmbassador().synchronizationPointAchieved(firstLabel);
  waitForSessions(
      {&registrar, &participant, &lateJoiner},
      [&] {
        return registrar.federationSynchronized().size() >= 3U &&
            participant.federationSynchronized().size() >= 3U &&
            lateJoiner.federationSynchronized().size() >= 1U;
      },
      options,
      "completion of the newly registered synchronization point");
  for (auto* session : {&registrar, &participant, &lateJoiner}) {
    auto const completions = session->federationSynchronized();
    std::size_t const expectedCount = session == &lateJoiner ? 1U : 3U;
    require(
        completions.size() == expectedCount &&
            completions.back().label == firstLabel &&
            completions.back().failedToSyncSet.empty(),
        "the newly registered synchronization point did not complete once for every current member");
  }

  lateJoiner.resign(rti::NO_ACTION);
  participant.resign(rti::NO_ACTION);
  registrar.resign(rti::NO_ACTION);
  registrar.rtiAmbassador().destroyFederationExecution(federation);
  lateJoiner.disconnect();
  participant.disconnect();
  registrar.disconnect();
}

void scenarioSynchronizationPointIndependentTrackingContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioSynchronizationPointIndependentTracking(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
