#include "tck_joined_federate_mom_interactions_received_counts.hpp"
#include "tck_main_scenarios.hpp"

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioJoinedFederateMomInteractionsReceivedCounts(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Joined-federate MOM interactions-received testing requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "Joined-federate MOM interactions-received testing requires an adapter-supplied standard MIM");

  Session receiver(options, model, "owner");
  Session sender(options, model, "member");
  Session requester(options, model, "member");
  auto const federation =
      federationName(options, "joined-federate-mom-interactions-received-counts");
  receiver.connect();
  sender.connect();
  requester.connect();
  if (options.logicalTimeImplementationName.empty()) {
    receiver.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring());
  } else {
    receiver.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring(),
        options.logicalTimeImplementationName);
  }
  receiver.join(
      options.ownerFederateName + L"-mom-interactions-received-receiver",
      options.federateType,
      federation);
  sender.join(
      options.memberFederateName + L"-mom-interactions-received-sender",
      options.federateType,
      federation);
  requester.join(
      options.memberFederateName + L"-mom-interactions-received-requester",
      options.federateType,
      federation);

  auto const receiverInteraction = receiver.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction,
      options.parameterName);
  require(
      receiverInteraction.isValid() &&
          receiver.rtiAmbassador().getInteractionClassName(receiverInteraction) ==
              options.interactionClassName,
      "Joined-federate MOM interactions-received interaction lookup did not round-trip");
  require(
      receiverParameter.isValid() &&
          receiver.rtiAmbassador().getParameterName(
              receiverInteraction,
              receiverParameter) == options.parameterName,
      "Joined-federate MOM interactions-received parameter lookup did not round-trip");

  auto const senderInteraction = sender.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const senderParameter = sender.rtiAmbassador().getParameterHandle(
      senderInteraction,
      options.parameterName);
  require(
      senderInteraction.isValid() && senderParameter.isValid(),
      "Joined-federate MOM interactions-received sender lookup returned invalid handles");
  auto const reliable = sender.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable");
  auto const bestEffort = sender.rtiAmbassador().getTransportationTypeHandle(L"HLAbestEffort");
  require(
      reliable.isValid() && bestEffort.isValid() &&
          sender.rtiAmbassador().getTransportationTypeName(reliable) == L"HLAreliable" &&
          sender.rtiAmbassador().getTransportationTypeName(bestEffort) == L"HLAbestEffort",
      "Joined-federate MOM interactions-received lookup could not resolve standard transports");

  sender.rtiAmbassador().publishInteractionClass(senderInteraction);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);

  auto sendInteraction = [&](std::uint8_t value) {
    std::vector<std::uint8_t> bytes{value};
    sender.rtiAmbassador().sendInteraction(
        senderInteraction,
        rti::ParameterHandleValueMap{{
            senderParameter,
            rti::VariableLengthData(bytes.data(), bytes.size())}},
        rti::VariableLengthData{});
  };
  sendInteraction(0x61U);
  waitFor(
      receiver,
      [&] { return receiver.recorder().interactions().size() >= 1U; },
      options,
      "joined-federate MOM interactions-received reliable delivery");
  auto const confirmationBefore =
      sender.recorder().interactionTransportationConfirmations().size();
  sender.rtiAmbassador().requestInteractionTransportationTypeChange(
      senderInteraction,
      bestEffort);
  waitFor(
      sender,
      [&] {
        return sender.recorder().interactionTransportationConfirmations().size() >
            confirmationBefore;
      },
      options,
      "joined-federate MOM interactions-received transportation change");
  auto const transportationConfirmations =
      sender.recorder().interactionTransportationConfirmations();
  require(
      transportationConfirmations.back().interaction == senderInteraction &&
          transportationConfirmations.back().transportation == bestEffort,
      "Joined-federate MOM interactions-received transportation change did not commit best effort");
  sendInteraction(0x62U);
  sendInteraction(0x63U);
  waitFor(
      receiver,
      [&] { return receiver.recorder().interactions().size() >= 3U; },
      options,
      "joined-federate MOM interactions-received best-effort delivery");

  auto const received = receiver.recorder().interactions();
  require(
      received.size() == 3U,
      "Joined-federate MOM interactions-received receiver observed the wrong delivery count");
  std::size_t reliableReceived = 0U;
  std::size_t bestEffortReceived = 0U;
  for (auto const& interaction : received) {
    require(
        interaction.present && interaction.interaction == receiverInteraction &&
            interaction.parameters.size() == 1U &&
            interaction.parameters.count(receiverParameter) == 1U &&
            interaction.tag.empty() && interaction.producer == sender.federateHandle() &&
            !interaction.regions.has_value(),
        "Joined-federate MOM interactions-received delivery returned non-standard metadata");
    auto const value = copyBytes(interaction.parameters.at(receiverParameter));
    require(
        value.size() == 1U &&
            (value.front() == 0x61U || value.front() == 0x62U || value.front() == 0x63U),
        "Joined-federate MOM interactions-received delivery returned the wrong payload");
    if (interaction.transportation == reliable) {
      ++reliableReceived;
    } else if (interaction.transportation == bestEffort) {
      ++bestEffortReceived;
    } else {
      require(
          false,
          "Joined-federate MOM interactions-received delivery returned an unknown transport");
    }
  }
  require(
      reliableReceived == 1U && bestEffortReceived == 2U,
      "Joined-federate MOM interactions-received delivery returned wrong transport counts");

  auto const reportClass = requester.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionsReceived");
  auto const requestClass = requester.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestInteractionsReceived");
  auto const requestFederate = requester.rtiAmbassador().getParameterHandle(
      requestClass,
      L"HLAfederate");
  auto const reportTransportation = requester.rtiAmbassador().getParameterHandle(
      reportClass,
      L"HLAtransportation");
  auto const reportCounts = requester.rtiAmbassador().getParameterHandle(
      reportClass,
      L"HLAinteractionCounts");
  require(
      reportClass.isValid() && requestClass.isValid() && requestFederate.isValid() &&
          reportTransportation.isValid() && reportCounts.isValid() &&
          requester.rtiAmbassador().getInteractionClassName(reportClass) ==
              L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionsReceived" &&
          requester.rtiAmbassador().getInteractionClassName(requestClass) ==
              L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestInteractionsReceived" &&
          requester.rtiAmbassador().getParameterName(requestClass, requestFederate) ==
              L"HLAfederate" &&
          requester.rtiAmbassador().getParameterName(reportClass, reportTransportation) ==
              L"HLAtransportation" &&
          requester.rtiAmbassador().getParameterName(reportClass, reportCounts) ==
              L"HLAinteractionCounts",
      "Joined-federate MOM interactions-received lookup did not round-trip the standard report surface");
  requester.rtiAmbassador().subscribeInteractionClass(reportClass, true);

  auto const expectedInteractionEncoding = copyBytes(receiverInteraction.encode());
  auto formatBytes = [](std::vector<std::uint8_t> const& bytes) {
    std::ostringstream stream;
    stream << std::hex;
    for (auto const byte : bytes) {
      if (byte < 0x10U) {
        stream << '0';
      }
      stream << static_cast<unsigned int>(byte);
    }
    return stream.str();
  };
  auto decodePositiveCount = [&](rti::VariableLengthData const& encoded,
                                 std::int32_t expectedCount,
                                 std::string const& description) {
    rti::HLAfixedRecord countPrototype;
    countPrototype.appendElement(rti::HLAvariableArray{rti::HLAbyte{}})
        .appendElement(rti::HLAinteger32BE{});
    rti::HLAvariableArray counts{countPrototype};
    counts.decode(encoded);
    require(
        counts.size() == 1U,
        description + " returned the wrong number of positive interaction counts");
    auto const& count = dynamic_cast<rti::HLAfixedRecord const&>(counts.get(0U));
    auto const& encodedInteraction =
        dynamic_cast<rti::HLAvariableArray const&>(count.get(0U));
    auto const actualInteractionEncoding = copyBytes(encodedInteraction.encode());
    require(
        actualInteractionEncoding == expectedInteractionEncoding,
        description + " returned the wrong interaction class (expected " +
            formatBytes(expectedInteractionEncoding) + ", got " +
            formatBytes(actualInteractionEncoding) + ")");
    auto const countValue = dynamic_cast<rti::HLAinteger32BE const&>(count.get(1U)).get();
    require(
        countValue == expectedCount,
        description + " returned the wrong interaction count");
  };
  auto verifyEmptyCount = [&](rti::VariableLengthData const& encoded,
                              std::string const& description) {
    if (encoded.size() == 0U) {
      return;
    }
    rti::HLAfixedRecord countPrototype;
    countPrototype.appendElement(rti::HLAvariableArray{rti::HLAbyte{}})
        .appendElement(rti::HLAinteger32BE{});
    rti::HLAvariableArray counts{countPrototype};
    counts.decode(encoded);
    require(
        counts.size() == 0U,
        description + " encoded a non-empty interaction count list for a NULL response");
  };

  auto verifyReports = [&](std::vector<InteractionRecord> const& reports,
                           std::string const& description,
                           bool expectInteractions) {
    require(
        reports.size() == 2U,
        description + " did not return exactly one report for each standard transport");
    bool sawReliable = false;
    bool sawBestEffort = false;
    for (auto const& report : reports) {
      require(
          report.present && report.interaction == reportClass && report.parameters.size() == 2U &&
              report.parameters.count(reportTransportation) == 1U &&
              report.parameters.count(reportCounts) == 1U && report.tag.empty() &&
              report.transportation == reliable && !report.producer.isValid() &&
              !report.regions.has_value(),
          description + " returned non-standard report metadata");
      auto const transportBytes = copyBytes(report.parameters.at(reportTransportation));
      if (transportBytes == copyBytes(reliable.encode())) {
        require(!sawReliable, description + " returned duplicate reliable reports");
        sawReliable = true;
        if (expectInteractions) {
          decodePositiveCount(
              report.parameters.at(reportCounts),
              1,
              description + " reliable count");
        } else {
          verifyEmptyCount(
              report.parameters.at(reportCounts),
              description + " reliable NULL count");
        }
      } else if (transportBytes == copyBytes(bestEffort.encode())) {
        require(!sawBestEffort, description + " returned duplicate best-effort reports");
        sawBestEffort = true;
        if (expectInteractions) {
          decodePositiveCount(
              report.parameters.at(reportCounts),
              2,
              description + " best-effort count");
        } else {
          verifyEmptyCount(
              report.parameters.at(reportCounts),
              description + " best-effort NULL count");
        }
      } else {
        require(false, description + " returned an unknown transportation value");
      }
    }
    require(
        sawReliable && sawBestEffort,
        description + " omitted a standard transportation bucket");
  };

  auto requestReports = [&](rti::FederateHandle const& subject,
                            std::string const& description) {
    requester.recorder().clearInteraction();
    requester.rtiAmbassador().sendInteraction(
        requestClass,
        rti::ParameterHandleValueMap{{requestFederate, subject.encode()}},
        rti::VariableLengthData{});
    waitFor(
        requester,
        [&] { return requester.recorder().interactions().size() >= 2U; },
        options,
        description);
    return requester.recorder().interactions();
  };

  auto const receiverReports = requestReports(
      receiver.federateHandle(),
      "joined-federate MOM interactions-received receiver reports");
  verifyReports(receiverReports, "joined-federate MOM interactions-received receiver", true);
  auto const requesterReports = requestReports(
      requester.federateHandle(),
      "joined-federate MOM interactions-received empty reports");
  verifyReports(requesterReports, "joined-federate MOM interactions-received empty", false);

  requester.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  sender.rtiAmbassador().unpublishInteractionClass(senderInteraction);
  requester.resign(rti::NO_ACTION);
  sender.resign(rti::NO_ACTION);
  receiver.resign(rti::NO_ACTION);
  receiver.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  sender.disconnect();
  receiver.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
