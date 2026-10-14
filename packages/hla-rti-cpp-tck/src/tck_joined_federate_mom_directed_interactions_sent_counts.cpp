#include "tck_joined_federate_mom_directed_interactions_sent_counts.hpp"
#include "tck_main_scenarios.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioJoinedFederateMomDirectedInteractionsSentCounts(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Joined-federate MOM directed-interactions-sent testing requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "Joined-federate MOM directed-interactions-sent testing requires an adapter-supplied standard MIM");

  Session sender(options, model, "owner");
  Session requester(options, model, "member");
  Session idle(options, model, "member");
  auto const federation =
      federationName(options, "joined-federate-mom-directed-interactions-sent-counts");
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
      options.ownerFederateName + L"-mom-directed-interactions-sent-sender",
      options.federateType,
      federation);
  requester.join(
      options.memberFederateName + L"-mom-directed-interactions-sent-requester",
      options.federateType,
      federation);
  idle.join(
      options.memberFederateName + L"-mom-directed-interactions-sent-idle",
      options.federateType,
      federation);

  auto const senderObjectClass = sender.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const senderAttribute = sender.rtiAmbassador().getAttributeHandle(
      senderObjectClass,
      options.attributeName);
  auto const requesterObjectClass = requester.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const requesterAttribute = requester.rtiAmbassador().getAttributeHandle(
      requesterObjectClass,
      options.attributeName);
  auto const senderInteraction = sender.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const senderParameter = sender.rtiAmbassador().getParameterHandle(
      senderInteraction,
      options.parameterName);
  require(
      senderObjectClass.isValid() && senderAttribute.isValid() &&
          sender.rtiAmbassador().getObjectClassName(senderObjectClass) ==
              options.objectClassName &&
          sender.rtiAmbassador().getAttributeName(
              senderObjectClass,
              senderAttribute) == options.attributeName &&
          senderInteraction.isValid() &&
          sender.rtiAmbassador().getInteractionClassName(senderInteraction) ==
              options.interactionClassName &&
          senderParameter.isValid() &&
          sender.rtiAmbassador().getParameterName(
              senderInteraction,
              senderParameter) == options.parameterName,
      "Joined-federate MOM directed-interactions-sent sender lookup did not round-trip");
  require(
      requesterObjectClass.isValid() && requesterAttribute.isValid() &&
          requester.rtiAmbassador().getObjectClassName(requesterObjectClass) ==
              options.objectClassName &&
          requester.rtiAmbassador().getAttributeName(
              requesterObjectClass,
              requesterAttribute) == options.attributeName,
      "Joined-federate MOM directed-interactions-sent target lookup did not round-trip");

  auto const reliable = sender.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable");
  auto const bestEffort = sender.rtiAmbassador().getTransportationTypeHandle(L"HLAbestEffort");
  require(
      reliable.isValid() && bestEffort.isValid() &&
          sender.rtiAmbassador().getTransportationTypeName(reliable) == L"HLAreliable" &&
          sender.rtiAmbassador().getTransportationTypeName(bestEffort) == L"HLAbestEffort",
      "Joined-federate MOM directed-interactions-sent lookup could not resolve standard transports");

  requester.rtiAmbassador().publishObjectClassAttributes(
      requesterObjectClass,
      rti::AttributeHandleSet{requesterAttribute});
  sender.rtiAmbassador().subscribeObjectClassAttributes(
      senderObjectClass,
      rti::AttributeHandleSet{senderAttribute},
      true,
      L"");
  sender.rtiAmbassador().publishInteractionClass(senderInteraction);
  rti::InteractionClassHandleSet const directedInteractions{senderInteraction};
  sender.rtiAmbassador().publishObjectClassDirectedInteractions(
      senderObjectClass,
      directedInteractions);

  auto const target = requester.rtiAmbassador().registerObjectInstance(requesterObjectClass);
  require(
      target.isValid(),
      "Joined-federate MOM directed-interactions-sent registration returned an invalid target");
  waitFor(
      sender,
      [&] { return sender.recorder().hasDiscovery(target); },
      options,
      "joined-federate MOM directed-interactions-sent target discovery");

  auto sendInteraction = [&](std::uint8_t value) {
    std::vector<std::uint8_t> bytes{value};
    sender.rtiAmbassador().sendInteraction(
        senderInteraction,
        rti::ParameterHandleValueMap{{
            senderParameter,
            rti::VariableLengthData(bytes.data(), bytes.size())}},
        rti::VariableLengthData{});
  };
  auto sendDirectedInteraction = [&](std::uint8_t value) {
    std::vector<std::uint8_t> bytes{value};
    sender.rtiAmbassador().sendDirectedInteraction(
        senderInteraction,
        target,
        rti::ParameterHandleValueMap{{
            senderParameter,
            rti::VariableLengthData(bytes.data(), bytes.size())}},
        rti::VariableLengthData{});
  };

  // Keep an ordinary send in the same reliable class ledger; the directed
  // report must count only the two target-qualified sends below.
  sendInteraction(0x81U);
  sendDirectedInteraction(0x82U);
  sendDirectedInteraction(0x83U);
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
      "joined-federate MOM directed-interactions-sent transportation change");
  auto const transportationConfirmations =
      sender.recorder().interactionTransportationConfirmations();
  require(
      transportationConfirmations.back().interaction == senderInteraction &&
          transportationConfirmations.back().transportation == bestEffort,
      "Joined-federate MOM directed-interactions-sent transportation change did not commit best effort");
  sendInteraction(0x84U);

  auto const reportClass = sender.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportDirectedInteractionsSent");
  auto const requestClass = sender.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestDirectedInteractionsSent");
  auto const requestFederate = sender.rtiAmbassador().getParameterHandle(
      requestClass,
      L"HLAfederate");
  auto const reportTransportation = sender.rtiAmbassador().getParameterHandle(
      reportClass,
      L"HLAtransportation");
  auto const reportCounts = sender.rtiAmbassador().getParameterHandle(
      reportClass,
      L"HLAinteractionCounts");
  require(
      reportClass.isValid() && requestClass.isValid() && requestFederate.isValid() &&
          reportTransportation.isValid() && reportCounts.isValid() &&
          sender.rtiAmbassador().getInteractionClassName(reportClass) ==
              L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportDirectedInteractionsSent" &&
          sender.rtiAmbassador().getInteractionClassName(requestClass) ==
              L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestDirectedInteractionsSent" &&
          sender.rtiAmbassador().getParameterName(requestClass, requestFederate) ==
              L"HLAfederate" &&
          sender.rtiAmbassador().getParameterName(reportClass, reportTransportation) ==
              L"HLAtransportation" &&
          sender.rtiAmbassador().getParameterName(reportClass, reportCounts) ==
              L"HLAinteractionCounts",
      "Joined-federate MOM directed-interactions-sent lookup did not round-trip the standard report surface");
  requester.rtiAmbassador().subscribeInteractionClass(reportClass, true);

  auto const expectedInteractionEncoding = copyBytes(senderInteraction.encode());
  auto const reliableEncoding = copyBytes(reliable.encode());
  auto const bestEffortEncoding = copyBytes(bestEffort.encode());
  auto decodePositiveCount = [&](rti::VariableLengthData const& encoded,
                                 std::int32_t expectedCount,
                                 std::string const& description) {
    rti::HLAfixedRecord countPrototype;
    countPrototype.appendElement(rti::HLAvariableArray{rti::HLAbyte{}})
        .appendElement(rti::HLAinteger32BE{});
    rti::HLAvariableArray counts{countPrototype};
    counts.decode(encoded);
    require(counts.size() == 1U, description + " returned the wrong count-list size");
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
    require(counts.size() == 0U, description + " returned a non-empty NULL count list");
  };
  auto verifyReports = [&](std::vector<InteractionRecord> const& reports,
                           std::string const& description,
                           bool expectDirected) {
    require(
        reports.size() == 2U,
        description + " did not return one report for each standard transport");
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
      auto const encodedTransportation =
          copyBytes(report.parameters.at(reportTransportation));
      if (encodedTransportation == reliableEncoding) {
        require(!sawReliable, description + " returned duplicate reliable reports");
        sawReliable = true;
        if (expectDirected) {
          decodePositiveCount(
              report.parameters.at(reportCounts),
              2,
              description + " reliable count");
        } else {
          verifyEmptyCount(
              report.parameters.at(reportCounts),
              description + " reliable NULL count");
        }
      } else if (encodedTransportation == bestEffortEncoding) {
        require(!sawBestEffort, description + " returned duplicate best-effort reports");
        sawBestEffort = true;
        verifyEmptyCount(
            report.parameters.at(reportCounts),
            description + " best-effort NULL count");
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
      "joined-federate MOM directed-interactions-sent sender reports");
  verifyReports(
      senderReports,
      "joined-federate MOM directed-interactions-sent sender",
      true);
  auto const idleReports = requestReports(
      idle.federateHandle(),
      "joined-federate MOM directed-interactions-sent empty reports");
  verifyReports(
      idleReports,
      "joined-federate MOM directed-interactions-sent empty",
      false);

  requester.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  sender.rtiAmbassador().unsubscribeObjectClassAttributes(
      senderObjectClass,
      rti::AttributeHandleSet{senderAttribute});
  requester.rtiAmbassador().unpublishObjectClassAttributes(
      requesterObjectClass,
      rti::AttributeHandleSet{requesterAttribute});
  sender.rtiAmbassador().unpublishObjectClassDirectedInteractions(
      senderObjectClass,
      directedInteractions);
  sender.rtiAmbassador().unpublishInteractionClass(senderInteraction);
  requester.resign(rti::DELETE_OBJECTS);
  idle.resign(rti::NO_ACTION);
  sender.resign(rti::NO_ACTION);
  sender.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  idle.disconnect();
  sender.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail

