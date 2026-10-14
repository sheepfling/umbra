#include "tck_joined_federate_mom_interactions_sent_counts.hpp"
#include "tck_main_scenarios.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioJoinedFederateMomInteractionsSentCounts(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Joined-federate MOM interactions-sent testing requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "Joined-federate MOM interactions-sent testing requires an adapter-supplied standard MIM");

  Session sender(options, model, "owner");
  Session requester(options, model, "member");
  Session idle(options, model, "member");
  auto const federation =
      federationName(options, "joined-federate-mom-interactions-sent-counts");
  sender.connect();
  requester.connect();
  idle.connect();
  if (options.logicalTimeImplementationName.empty()) {
    sender.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring());
  } else {
    sender.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring(),
        options.logicalTimeImplementationName);
  }
  sender.join(
      options.ownerFederateName + L"-mom-interactions-sent-sender",
      options.federateType,
      federation);
  requester.join(
      options.memberFederateName + L"-mom-interactions-sent-requester",
      options.federateType,
      federation);
  idle.join(
      options.memberFederateName + L"-mom-interactions-sent-idle",
      options.federateType,
      federation);

  auto const senderInteraction = sender.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const senderParameter = sender.rtiAmbassador().getParameterHandle(
      senderInteraction,
      options.parameterName);
  require(
      senderInteraction.isValid() &&
          sender.rtiAmbassador().getInteractionClassName(senderInteraction) ==
              options.interactionClassName,
      "Joined-federate MOM interactions-sent interaction lookup did not round-trip");
  require(
      senderParameter.isValid() &&
          sender.rtiAmbassador().getParameterName(
              senderInteraction,
              senderParameter) == options.parameterName,
      "Joined-federate MOM interactions-sent parameter lookup did not round-trip");

  auto const reliable = sender.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable");
  auto const bestEffort = sender.rtiAmbassador().getTransportationTypeHandle(L"HLAbestEffort");
  require(
      reliable.isValid() && bestEffort.isValid() &&
          sender.rtiAmbassador().getTransportationTypeName(reliable) == L"HLAreliable" &&
          sender.rtiAmbassador().getTransportationTypeName(bestEffort) == L"HLAbestEffort",
      "Joined-federate MOM interactions-sent lookup could not resolve standard transports");
  sender.rtiAmbassador().publishInteractionClass(senderInteraction);

  auto sendInteraction = [&](std::uint8_t value) {
    std::vector<std::uint8_t> bytes{value};
    sender.rtiAmbassador().sendInteraction(
        senderInteraction,
        rti::ParameterHandleValueMap{{
            senderParameter,
            rti::VariableLengthData(bytes.data(), bytes.size())}},
        rti::VariableLengthData{});
  };
  sendInteraction(0x81U);
  sender.rtiAmbassador().requestInteractionTransportationTypeChange(
      senderInteraction,
      bestEffort);
  waitFor(
      sender,
      [&] {
        return !sender.recorder().interactionTransportationConfirmations().empty();
      },
      options,
      "joined-federate MOM interactions-sent transportation change");
  auto const transportationConfirmations =
      sender.recorder().interactionTransportationConfirmations();
  require(
      transportationConfirmations.back().interaction == senderInteraction &&
          transportationConfirmations.back().transportation == bestEffort,
      "Joined-federate MOM interactions-sent transportation change did not commit best effort");
  sendInteraction(0x82U);
  sendInteraction(0x83U);

  auto const reportClass = requester.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionsSent");
  auto const requestClass = requester.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestInteractionsSent");
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
              L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionsSent" &&
          requester.rtiAmbassador().getInteractionClassName(requestClass) ==
              L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestInteractionsSent" &&
          requester.rtiAmbassador().getParameterName(requestClass, requestFederate) ==
              L"HLAfederate" &&
          requester.rtiAmbassador().getParameterName(reportClass, reportTransportation) ==
              L"HLAtransportation" &&
          requester.rtiAmbassador().getParameterName(reportClass, reportCounts) ==
              L"HLAinteractionCounts",
      "Joined-federate MOM interactions-sent lookup did not round-trip the standard report surface");
  requester.rtiAmbassador().subscribeInteractionClass(reportClass, true);

  auto const expectedInteractionEncoding = copyBytes(senderInteraction.encode());
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
    require(
        copyBytes(encodedInteraction.encode()) == expectedInteractionEncoding,
        description + " returned the wrong interaction class");
    require(
        dynamic_cast<rti::HLAinteger32BE const&>(count.get(1U)).get() == expectedCount,
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
          report.present && report.interaction == reportClass &&
              report.parameters.size() == 2U &&
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

  auto const senderReports = requestReports(
      sender.federateHandle(),
      "joined-federate MOM interactions-sent sender reports");
  verifyReports(senderReports, "joined-federate MOM interactions-sent sender", true);
  auto const idleReports = requestReports(
      idle.federateHandle(),
      "joined-federate MOM interactions-sent empty reports");
  verifyReports(idleReports, "joined-federate MOM interactions-sent empty", false);

  requester.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  sender.rtiAmbassador().unpublishInteractionClass(senderInteraction);
  requester.resign(rti::NO_ACTION);
  idle.resign(rti::NO_ACTION);
  sender.resign(rti::NO_ACTION);
  sender.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  idle.disconnect();
  sender.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
