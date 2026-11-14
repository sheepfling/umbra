#include "tck_main_scenarios.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace hla_rti_cpp_tck::detail {
namespace {

std::optional<std::size_t> transportConnectionCount(
    std::filesystem::path const& path) {
  std::ifstream input(path, std::ios::binary);
  std::size_t count = 0U;
  if (!(input >> count)) {
    return std::nullopt;
  }
  return count;
}

struct TransportSessionProtocolObservation final {
  std::string recordType;
  std::size_t connectionIndex = 0U;
  std::string direction;
  std::uint32_t sequenceNumber = 0U;
  std::uint32_t lastReceivedMessageNumber = 0U;
  std::uint32_t messageType = 0U;
  std::string sessionId;
  std::uint32_t reason = 0U;
  std::string probeOutcome;
};

std::optional<std::vector<TransportSessionProtocolObservation>>
transportSessionProtocolObservations(std::filesystem::path const& path) {
  std::ifstream input(path, std::ios::binary);
  std::vector<TransportSessionProtocolObservation> observations;
  std::string kind;
  while (input >> kind) {
    TransportSessionProtocolObservation observation;
    observation.recordType = kind;
    if (kind == "frame") {
      if (!(input >> observation.connectionIndex >> observation.direction >>
            observation.sequenceNumber >>
            observation.lastReceivedMessageNumber >> observation.messageType >>
            observation.sessionId >> observation.reason)) {
        return std::nullopt;
      }
    } else if (kind == "closed") {
      if (!(input >> observation.connectionIndex >> observation.direction)) {
        return std::nullopt;
      }
    } else if (kind == "stale-resume-probe") {
      if (!(input >> observation.sessionId >> observation.probeOutcome)) {
        return std::nullopt;
      }
    } else {
      return std::nullopt;
    }
    observations.push_back(observation);
  }
  if (!input.eof() || input.bad() || observations.empty()) {
    return std::nullopt;
  }
  return observations;
}

void writeFixtureMarker(
    std::filesystem::path const& path,
    std::string_view value) {
  std::ofstream marker(path, std::ios::binary | std::ios::trunc);
  marker << value << '\n';
  require(static_cast<bool>(marker), "Could not write a process-fixture marker");
}

}  // namespace

void scenarioOneTransportConnectionPerFederateSession(
    Options const& options,
    rti::CallbackModel model) {
  require(
      options.rtiAddress.rfind("tcp://", 0U) == 0U,
      "Transport-session cardinality requires an adapter-supplied TCP endpoint");
  require(
      !options.results.empty() && !options.fom.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "Transport-session cardinality requires result, FOM, and time configuration");
  require(
      options.additionalSettings ==
          "FED_INT_HEART=0.1;FED_TIMEOUT_HEART=2;FED_TIMEOUT_RECONNECT=0.25",
      "The TCP process fixture did not provide its bounded heartbeat settings");

  Session owner(options, model, "owner");
  Session member(options, model, "member");
  auto const ownerConnection = owner.connect();
  require(
      ownerConnection.addressUsed,
      "The owner federate session must use the adapter-supplied TCP endpoint");

  auto const markerDirectory = std::filesystem::path(options.results).parent_path();
  auto const connectionCountPath =
      markerDirectory / "transport-connection-count.txt";
  waitFor(
      owner,
      [&] {
        auto const count = transportConnectionCount(connectionCountPath);
        return count.has_value() && *count >= 1U;
      },
      options,
      "the adapter to observe the owner TCP transport connection");
  auto const ownerConnectionCount =
      transportConnectionCount(connectionCountPath);
  require(
      ownerConnectionCount == 1U,
      "One successful Connect call did not use exactly one TCP transport connection");

  auto const memberConnection = member.connect();
  require(
      memberConnection.addressUsed,
      "The member federate session must use the adapter-supplied TCP endpoint");
  waitFor(
      owner,
      member,
      [&] {
        auto const count = transportConnectionCount(connectionCountPath);
        return count.has_value() && *count >= 2U;
      },
      options,
      "the adapter to observe the member TCP transport connection");
  require(
      transportConnectionCount(connectionCountPath) == 2U,
      "Two successful Connect calls did not use exactly two TCP transport connections");

  auto const protocolObservationPath =
      markerDirectory / "transport-session-protocol.txt";
  waitFor(
      owner,
      member,
      [&] {
        auto const observations =
            transportSessionProtocolObservations(protocolObservationPath);
        if (!observations.has_value()) {
          return false;
        }
        std::size_t newSessionCount = 0U;
        std::size_t successfulStatusCount = 0U;
        for (auto const& observation : *observations) {
          newSessionCount += observation.messageType == 1U ? 1U : 0U;
          successfulStatusCount +=
              observation.messageType == 2U &&
                      observation.reason == 0U
                  ? 1U
                  : 0U;
        }
        return newSessionCount == 2U && successfulStatusCount == 2U;
      },
      options,
      "the TCP adapter to observe both session handshakes");

  auto const protocolObservations =
      transportSessionProtocolObservations(protocolObservationPath);
  require(
      protocolObservations.has_value(),
      "The TCP adapter did not publish parseable session protocol observations");
  auto const noSessionId = std::string("0000000000000000");
  std::set<std::size_t> newSessionConnections;
  std::set<std::size_t> successfulStatusConnections;
  std::set<std::string> assignedSessionIds;
  std::size_t newSessionCount = 0U;
  std::size_t successfulStatusCount = 0U;
  for (auto const& observation : *protocolObservations) {
    if (observation.recordType != "frame") {
      continue;
    }
    if (observation.messageType == 1U) {
      ++newSessionCount;
      require(
          observation.sessionId == noSessionId,
          "CTRL_NEW_SESSION must use the all-zero Session ID for no session");
      newSessionConnections.insert(observation.connectionIndex);
      continue;
    }
    if (observation.messageType != 2U) {
      continue;
    }
    require(
        observation.reason == 0U,
        "A successful Connect call received an unsuccessful CTRL_NEW_SESSION_STATUS");
    ++successfulStatusCount;
    require(
        observation.sessionId != noSessionId,
        "A successful CTRL_NEW_SESSION_STATUS must assign a nonzero Session ID");
    require(
        newSessionConnections.find(observation.connectionIndex) !=
            newSessionConnections.end(),
        "A successful session status did not use the connection that requested it");
    successfulStatusConnections.insert(observation.connectionIndex);
    assignedSessionIds.insert(observation.sessionId);
  }
  require(
      newSessionCount == 2U && successfulStatusCount == 2U &&
          newSessionConnections.size() == 2U &&
          successfulStatusConnections == newSessionConnections &&
          assignedSessionIds.size() == 2U,
      "The RTI did not assign a distinct nonzero Session ID to each successful federate session");

  writeFixtureMarker(
      markerDirectory / "federate-idle-heartbeat-ready.ok", "ready");
  auto const federation = federationName(options, "transport-session-cardinality");
  owner.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);
  member.join(options.memberFederateName, options.federateType, federation);
  require(
      owner.federateHandle() != member.federateHandle(),
      "Two RTIambassador sessions joined as the same federate");

  owner.rtiAmbassador().listFederationExecutions();
  member.rtiAmbassador().listFederationExecutions();
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(owner.rtiAmbassador().evokeCallback(0.25));
    static_cast<void>(member.rtiAmbassador().evokeCallback(0.25));
  }
  require(
      owner.recorder().listsFederation(
          federation,
          options.logicalTimeImplementationName) &&
          member.recorder().listsFederation(
              federation,
              options.logicalTimeImplementationName),
      "Both federate sessions did not complete a service on their transport connections");

  auto const countAfterServices = transportConnectionCount(connectionCountPath);
  require(
      countAfterServices == 2U,
      "The two federate sessions did not remain on exactly two TCP transport connections");

  auto const completeProtocolObservations =
      transportSessionProtocolObservations(protocolObservationPath);
  require(
      completeProtocolObservations.has_value(),
      "The TCP adapter did not publish complete parseable protocol observations");
  std::map<std::pair<std::size_t, std::string>, std::uint32_t>
      finalSequenceByConnectionAndDirection;
  std::size_t callRequests = 0U;
  std::size_t callResponses = 0U;
  std::map<std::size_t, std::vector<std::uint32_t>>
      callRequestSequencesByConnection;
  std::map<std::size_t, std::vector<std::uint32_t>>
      callResponseAcknowledgedSequencesByConnection;
  for (auto const& observation : *completeProtocolObservations) {
    if (observation.recordType != "frame") {
      continue;
    }
    if (observation.messageType == 1U &&
        observation.sequenceNumber == 0x80000000U) {
      continue;
    }
    require(
        observation.sequenceNumber <= 0x7fffffffU,
        "Transport sequence number " +
            std::to_string(observation.sequenceNumber) + " for message type " +
            std::to_string(observation.messageType) + " on connection " +
            std::to_string(observation.connectionIndex) + " in direction " +
            observation.direction + " has its reserved MSB set");
    auto const sequenceKey =
        std::make_pair(observation.connectionIndex, observation.direction);
    auto const previous = finalSequenceByConnectionAndDirection.find(sequenceKey);
    if (previous != finalSequenceByConnectionAndDirection.end()) {
      require(
          observation.sequenceNumber > previous->second ||
              (previous->second == 0x7fffffffU &&
               observation.sequenceNumber == 0U),
          "Transport message sequence numbers did not increase or wrap from "
          "0x7fffffff to zero");
    }
    finalSequenceByConnectionAndDirection[sequenceKey] =
        observation.sequenceNumber;

    if (observation.messageType == 20U) {
      require(
          observation.direction == "federate-to-rti",
          "HLA_CALL_REQUEST must travel from the federate to the RTI");
      ++callRequests;
      callRequestSequencesByConnection[observation.connectionIndex].push_back(
          observation.sequenceNumber);
    } else if (observation.messageType == 21U) {
      require(
          observation.direction == "rti-to-federate",
          "HLA_CALL_RESPONSE must travel from the RTI to the federate");
      ++callResponses;
      callResponseAcknowledgedSequencesByConnection[
          observation.connectionIndex]
          .push_back(observation.lastReceivedMessageNumber);
    } else if (observation.messageType == 22U) {
      require(
          observation.direction == "rti-to-federate",
          "HLA_CALLBACK_REQUEST must travel from the RTI to the federate");
    } else if (observation.messageType == 23U) {
      require(
          observation.direction == "federate-to-rti",
          "HLA_CALLBACK_RESPONSE must travel from the federate to the RTI");
    }
  }
  require(
      callRequests > 0U && callResponses > 0U,
      "The session exercise did not observe both HLA_CALL_REQUEST and "
      "HLA_CALL_RESPONSE service traffic");
  require(
      callRequestSequencesByConnection.size() ==
          callResponseAcknowledgedSequencesByConnection.size(),
      "The observed HLA_CALL_REQUEST and HLA_CALL_RESPONSE connection sets differ");
  for (auto const& [connectionIndex, requestSequences] :
       callRequestSequencesByConnection) {
    auto const responses = callResponseAcknowledgedSequencesByConnection.find(
        connectionIndex);
    require(
        responses != callResponseAcknowledgedSequencesByConnection.end(),
        "An HLA_CALL_REQUEST connection has no HLA_CALL_RESPONSE messages");
    auto orderedRequests = requestSequences;
    auto orderedAcknowledgements = responses->second;
    std::sort(orderedRequests.begin(), orderedRequests.end());
    std::sort(orderedAcknowledgements.begin(), orderedAcknowledgements.end());
    require(
        orderedAcknowledgements == orderedRequests,
        "HLA_CALL_RESPONSE did not include the sequence numbers of the "
        "HLA_CALL_REQUEST messages received on its connection");
  }

  member.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
  owner.disconnect();

  std::optional<std::vector<TransportSessionProtocolObservation>>
      terminatedProtocolObservations;
  auto const terminationDeadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(3);
  while (std::chrono::steady_clock::now() < terminationDeadline) {
    terminatedProtocolObservations =
        transportSessionProtocolObservations(protocolObservationPath);
    if (terminatedProtocolObservations.has_value()) {
      std::size_t federateCloseCount = 0U;
      std::size_t staleResumeProbeCount = 0U;
      for (auto const& observation : *terminatedProtocolObservations) {
        federateCloseCount +=
            observation.recordType == "closed" &&
                    observation.direction == "federate-to-rti"
                ? 1U
                : 0U;
        staleResumeProbeCount +=
            observation.recordType == "stale-resume-probe" ? 1U : 0U;
      }
      if (federateCloseCount == 2U && staleResumeProbeCount == 2U) {
        break;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  require(
      terminatedProtocolObservations.has_value(),
      "The TCP adapter did not publish parseable termination observations");

  std::map<std::size_t, std::size_t> terminateSessionIndexByConnection;
  std::map<std::size_t, std::size_t> sessionTerminatedIndexByConnection;
  std::map<std::size_t, std::size_t> federateCloseIndexByConnection;
  std::map<std::size_t, std::string> terminateSessionIdByConnection;
  std::map<std::size_t, std::string> sessionTerminatedIdByConnection;
  std::map<std::size_t, std::uint32_t> terminateSessionSequenceByConnection;
  std::map<std::size_t, std::uint32_t>
      sessionTerminatedAcknowledgementByConnection;
  std::map<std::string, std::string> staleResumeProbeOutcomeBySession;
  for (std::size_t index = 0U;
       index < terminatedProtocolObservations->size();
       ++index) {
    auto const& observation = (*terminatedProtocolObservations)[index];
    if (observation.recordType == "stale-resume-probe") {
      require(
          staleResumeProbeOutcomeBySession.emplace(
              observation.sessionId, observation.probeOutcome).second,
          "A terminated session received more than one stale Resume Request probe");
      continue;
    }
    if (observation.recordType == "closed") {
      if (observation.direction == "federate-to-rti") {
        federateCloseIndexByConnection[observation.connectionIndex] = index;
      }
      continue;
    }
    if (observation.messageType == 5U) {
      require(
          observation.direction == "federate-to-rti",
          "CTRL_CLOSE must be sent from the federate to the RTI");
      require(
          terminateSessionIndexByConnection.emplace(
              observation.connectionIndex, index).second,
          "A federate session sent more than one CTRL_CLOSE");
      terminateSessionIdByConnection[observation.connectionIndex] =
          observation.sessionId;
      terminateSessionSequenceByConnection[observation.connectionIndex] =
          observation.sequenceNumber;
    } else if (observation.messageType == 6U) {
      require(
          observation.direction == "rti-to-federate",
          "CTRL_CLOSE_ACK must be sent from the RTI to the federate");
      require(
          sessionTerminatedIndexByConnection.emplace(
              observation.connectionIndex, index).second,
          "A federate session received more than one CTRL_CLOSE_ACK");
      sessionTerminatedIdByConnection[observation.connectionIndex] =
          observation.sessionId;
      sessionTerminatedAcknowledgementByConnection[
          observation.connectionIndex] =
          observation.lastReceivedMessageNumber;
    }
  }
  require(
      terminateSessionIndexByConnection.size() == 2U &&
          sessionTerminatedIndexByConnection.size() == 2U &&
          federateCloseIndexByConnection.size() == 2U &&
          staleResumeProbeOutcomeBySession.size() == 2U,
      "Disconnect did not exchange one termination request and response on each federate session");
  std::set<std::string> terminatedSessionIds;
  for (auto const& [connectionIndex, sessionId] :
       terminateSessionIdByConnection) {
    static_cast<void>(connectionIndex);
    terminatedSessionIds.emplace(sessionId);
  }
  std::set<std::string> probedSessionIds;
  for (auto const& [sessionId, outcome] : staleResumeProbeOutcomeBySession) {
    require(
        outcome == "closed",
        "The RTI did not close a stale Resume Request using a terminated Session ID");
    probedSessionIds.emplace(sessionId);
  }
  require(
      probedSessionIds == terminatedSessionIds,
      "The adapter did not probe each terminated Session ID exactly once");
  for (auto const& [connectionIndex, requestIndex] :
       terminateSessionIndexByConnection) {
    auto const responseIndex =
        sessionTerminatedIndexByConnection.find(connectionIndex);
    auto const closeIndex = federateCloseIndexByConnection.find(connectionIndex);
    require(
        responseIndex != sessionTerminatedIndexByConnection.end() &&
            closeIndex != federateCloseIndexByConnection.end(),
        "CTRL_CLOSE did not receive both a matching response and a federate close event");
    require(
        sessionTerminatedIdByConnection.at(connectionIndex) ==
            terminateSessionIdByConnection.at(connectionIndex),
        "CTRL_CLOSE_ACK did not use the terminating federate session ID");
    require(
        sessionTerminatedAcknowledgementByConnection.at(connectionIndex) ==
            terminateSessionSequenceByConnection.at(connectionIndex),
        "CTRL_CLOSE_ACK did not acknowledge its corresponding CTRL_CLOSE sequence");
    require(
        requestIndex < responseIndex->second,
        "CTRL_CLOSE_ACK was observed before its CTRL_CLOSE request");
    require(
        responseIndex->second < closeIndex->second,
        "The federate closed its transport before receiving CTRL_CLOSE_ACK");
  }
}

void scenarioRejectsInvalidTransportSessionMetadata(
    Options const& options,
    rti::CallbackModel model) {
  require(
      options.rtiAddress.rfind("tcp://", 0U) == 0U,
      "Invalid transport-session metadata requires an adapter-supplied TCP endpoint");

  auto expectConnectionFailure = [&](std::string role) {
    Session session(options, model, std::move(role));
    bool rejected = false;
    try {
      static_cast<void>(session.connect());
    } catch (rti::ConnectionFailed const&) {
      rejected = true;
    }
    require(
        rejected,
        "RTIambassador.connect must report ConnectionFailed for invalid "
        "New Session Status metadata");
  };

  expectConnectionFailure("invalid-sequence-status");
  expectConnectionFailure("invalid-session-id-status");
}

}  // namespace hla_rti_cpp_tck::detail
