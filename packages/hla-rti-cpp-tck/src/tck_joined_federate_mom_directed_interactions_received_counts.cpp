#include "tck_joined_federate_mom_directed_interactions_received_counts.hpp"
#include "tck_main_scenarios.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioJoinedFederateMomDirectedInteractionsReceivedCounts(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Joined-federate MOM directed-interactions-received testing requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "Joined-federate MOM directed-interactions-received testing requires an adapter-supplied standard MIM");

  Session receiver(options, model, "owner");
  Session sender(options, model, "member");
  Session requester(options, model, "member");
  auto const federation =
      federationName(options, "joined-federate-mom-directed-interactions-received-counts");
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
      options.ownerFederateName + L"-mom-directed-interactions-received-receiver",
      options.federateType,
      federation);
  sender.join(
      options.memberFederateName + L"-mom-directed-interactions-received-sender",
      options.federateType,
      federation);
  requester.join(
      options.memberFederateName + L"-mom-directed-interactions-received-requester",
      options.federateType,
      federation);

  auto const receiverObjectClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const receiverAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverObjectClass,
      options.attributeName);
  auto const senderObjectClass = sender.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const senderAttribute = sender.rtiAmbassador().getAttributeHandle(
      senderObjectClass,
      options.attributeName);
  auto const receiverInteraction = receiver.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction,
      options.parameterName);
  auto const senderInteraction = sender.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const senderParameter = sender.rtiAmbassador().getParameterHandle(
      senderInteraction,
      options.parameterName);
  require(
      receiverObjectClass.isValid() && receiverAttribute.isValid() &&
          receiverInteraction.isValid() && receiverParameter.isValid() &&
          receiver.rtiAmbassador().getObjectClassName(receiverObjectClass) ==
              options.objectClassName &&
          receiver.rtiAmbassador().getAttributeName(
              receiverObjectClass,
              receiverAttribute) == options.attributeName &&
          receiver.rtiAmbassador().getInteractionClassName(receiverInteraction) ==
              options.interactionClassName &&
          receiver.rtiAmbassador().getParameterName(
              receiverInteraction,
              receiverParameter) == options.parameterName,
      "Joined-federate MOM directed-interactions-received receiver lookups did not round-trip");
  require(
      senderObjectClass.isValid() && senderAttribute.isValid() &&
          senderInteraction.isValid() && senderParameter.isValid(),
      "Joined-federate MOM directed-interactions-received sender lookup returned invalid handles");

  auto const reliable = sender.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable");
  auto const bestEffort = sender.rtiAmbassador().getTransportationTypeHandle(L"HLAbestEffort");
  require(
      reliable.isValid() && bestEffort.isValid() &&
          sender.rtiAmbassador().getTransportationTypeName(reliable) == L"HLAreliable" &&
          sender.rtiAmbassador().getTransportationTypeName(bestEffort) == L"HLAbestEffort",
      "Joined-federate MOM directed-interactions-received lookup could not resolve standard transports");

  receiver.rtiAmbassador().publishObjectClassAttributes(
      receiverObjectClass,
      rti::AttributeHandleSet{receiverAttribute});
  sender.rtiAmbassador().subscribeObjectClassAttributes(
      senderObjectClass,
      rti::AttributeHandleSet{senderAttribute},
      true,
      L"");
  sender.rtiAmbassador().publishInteractionClass(senderInteraction);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);
  rti::InteractionClassHandleSet const senderDirectedInteractions{senderInteraction};
  rti::InteractionClassHandleSet const receiverDirectedInteractions{receiverInteraction};
  sender.rtiAmbassador().publishObjectClassDirectedInteractions(
      senderObjectClass,
      senderDirectedInteractions);
  receiver.rtiAmbassador().subscribeObjectClassDirectedInteractions(
      receiverObjectClass,
      receiverDirectedInteractions,
      true);

  auto const target = receiver.rtiAmbassador().registerObjectInstance(receiverObjectClass);
  require(target.isValid(), "Joined-federate MOM directed-interactions-received registration returned an invalid target");
  waitFor(
      sender,
      [&] { return sender.recorder().hasDiscovery(target); },
      options,
      "joined-federate MOM directed-interactions-received target discovery");

  std::vector<std::uint8_t> const parameterBytes{0x61U};
  rti::ParameterHandleValueMap const parameters{{
      senderParameter,
      rti::VariableLengthData(parameterBytes.data(), parameterBytes.size())}};
  sender.rtiAmbassador().sendInteraction(
      senderInteraction,
      parameters,
      rti::VariableLengthData{});
  waitFor(
      receiver,
      [&] { return receiver.recorder().interactions().size() >= 1U; },
      options,
      "joined-federate MOM directed-interactions-received ordinary delivery");
  auto const ordinaryReceives = receiver.recorder().interactions();
  require(
      ordinaryReceives.size() == 1U &&
          ordinaryReceives.front().interaction == receiverInteraction &&
          ordinaryReceives.front().parameters.size() == 1U &&
          ordinaryReceives.front().parameters.count(receiverParameter) == 1U &&
          copyBytes(ordinaryReceives.front().parameters.at(receiverParameter)) ==
              parameterBytes &&
          ordinaryReceives.front().tag.empty() &&
          ordinaryReceives.front().transportation == reliable &&
          ordinaryReceives.front().producer == sender.federateHandle() &&
          !ordinaryReceives.front().regions.has_value(),
      "Joined-federate MOM directed-interactions-received ordinary delivery returned non-standard metadata");

  sender.rtiAmbassador().sendDirectedInteraction(
      senderInteraction,
      target,
      parameters,
      rti::VariableLengthData{});
  waitFor(
      receiver,
      [&] { return receiver.recorder().directedInteractions().size() >= 1U; },
      options,
      "joined-federate MOM directed-interactions-received directed delivery");
  auto const directedReceives = receiver.recorder().directedInteractions();
  require(
      directedReceives.size() == 1U &&
          directedReceives.front().interaction == receiverInteraction &&
          directedReceives.front().object == target &&
          directedReceives.front().parameters.size() == 1U &&
          directedReceives.front().parameters.count(receiverParameter) == 1U &&
          copyBytes(directedReceives.front().parameters.at(receiverParameter)) ==
              parameterBytes &&
          directedReceives.front().tag.empty() &&
          directedReceives.front().transportation == reliable &&
          directedReceives.front().producer == sender.federateHandle(),
      "Joined-federate MOM directed-interactions-received directed delivery returned non-standard metadata");

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
      "joined-federate MOM directed-interactions-received transportation change");
  auto const transportationConfirmations =
      sender.recorder().interactionTransportationConfirmations();
  require(
      transportationConfirmations.back().interaction == senderInteraction &&
          transportationConfirmations.back().transportation == bestEffort,
      "Joined-federate MOM directed-interactions-received transportation change did not commit best effort");

  auto const reportClass = requester.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportDirectedInteractionsReceived");
  auto const requestClass = requester.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestDirectedInteractionsReceived");
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
              L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportDirectedInteractionsReceived" &&
          requester.rtiAmbassador().getInteractionClassName(requestClass) ==
              L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestDirectedInteractionsReceived" &&
          requester.rtiAmbassador().getParameterName(requestClass, requestFederate) ==
              L"HLAfederate" &&
          requester.rtiAmbassador().getParameterName(reportClass, reportTransportation) ==
              L"HLAtransportation" &&
          requester.rtiAmbassador().getParameterName(reportClass, reportCounts) ==
              L"HLAinteractionCounts",
      "Joined-federate MOM directed-interactions-received lookup did not round-trip the standard report surface");
  requester.rtiAmbassador().subscribeInteractionClass(reportClass, true);

  auto const expectedInteractionEncoding = copyBytes(receiverInteraction.encode());
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
              1,
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
            description + " best-effort count");
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
      "joined-federate MOM directed-interactions-received receiver reports");
  verifyReports(
      receiverReports,
      "joined-federate MOM directed-interactions-received receiver",
      true);
  auto const requesterReports = requestReports(
      requester.federateHandle(),
      "joined-federate MOM directed-interactions-received empty reports");
  verifyReports(
      requesterReports,
      "joined-federate MOM directed-interactions-received empty",
      false);

  requester.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  receiver.rtiAmbassador().unsubscribeObjectClassDirectedInteractions(
      receiverObjectClass,
      receiverDirectedInteractions);
  sender.rtiAmbassador().unpublishObjectClassDirectedInteractions(
      senderObjectClass,
      senderDirectedInteractions);
  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  sender.rtiAmbassador().unpublishInteractionClass(senderInteraction);
  sender.rtiAmbassador().unsubscribeObjectClassAttributes(
      senderObjectClass,
      rti::AttributeHandleSet{senderAttribute});
  receiver.rtiAmbassador().unpublishObjectClassAttributes(
      receiverObjectClass,
      rti::AttributeHandleSet{receiverAttribute});
  requester.resign(rti::NO_ACTION);
  sender.resign(rti::NO_ACTION);
  receiver.resign(rti::DELETE_OBJECTS);
  receiver.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  sender.disconnect();
  receiver.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
