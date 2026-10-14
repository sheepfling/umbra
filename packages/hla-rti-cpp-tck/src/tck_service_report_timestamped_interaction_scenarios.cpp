#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioServiceReportTimestampedInteraction(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty(),
      "Timestamped service-report interaction testing requires an adapter-supplied standard MIM");

  Session publisher(options, model, "service-report-timestamped-publisher");
  Session receiver(options, model, "service-report-timestamped-receiver");
  Session observer(options, model, "service-report-timestamped-observer");
  auto const federation = federationName(options, "service-report-timestamped-interaction");
  publisher.connect();
  receiver.connect();
  observer.connect();
  publisher.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(
      options.ownerFederateName + L"-service-report-timestamped-publisher",
      options.federateType,
      federation);
  receiver.join(
      options.memberFederateName + L"-service-report-timestamped-receiver",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-timestamped-observer",
      options.federateType,
      federation);

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Timestamped service-report lookup returned an invalid HLAreportServiceInvocation handle");
  std::vector<rti::ParameterHandle> reportParameters;
  for (auto const& name : {
           L"HLAservice",
           L"HLAserviceType",
           L"HLAsuccessIndicator",
           L"HLAsuppliedArguments",
           L"HLAreturnedArgument",
           L"HLAexception",
           L"HLAserialNumber",
           L"HLAfederate"}) {
    auto const parameter = observer.rtiAmbassador().getParameterHandle(reportClass, name);
    require(
        parameter.isValid(),
        "Timestamped service-report parameter lookup returned an invalid handle");
    reportParameters.push_back(parameter);
  }

  auto const publisherInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.parameterName);
  auto const receiverInteraction = receiver.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction,
      options.parameterName);
  require(
      publisherInteraction.isValid() && publisherParameter.isValid() &&
          receiverInteraction.isValid() && receiverParameter.isValid(),
      "Timestamped service-report source interaction lookup returned an invalid handle");

  // Keep setup services out of the asserted report. The timestamped source
  // invocation below is the only service whose report is enabled.
  publisher.rtiAmbassador().setServiceReportingSwitch(false);
  publisher.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  receiver.rtiAmbassador().setServiceReportingSwitch(false);
  receiver.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);
  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);

  auto const publisherTime = makeTimeContext(publisher);
  auto const receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "timestamped service-report interaction members selected different logical-time factories");
  auto const lookaheadTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto lookahead = publisherTime.factory->makeZero();
  require(
      lookahead != nullptr,
      "timestamped service-report interaction could not allocate a lookahead interval");
  lookahead->setToDifference(*lookaheadTime, *publisherTime.initial);
  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "timestamped service-report interaction time-constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "timestamped service-report interaction time-regulation callback");
  observer.recorder().clearInteraction();

  publisher.rtiAmbassador().setServiceReportingSwitch(true);
  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      6U);
  std::vector<std::uint8_t> const valueBytes{0x54U, 0x53U, 0x4FU};
  std::vector<std::uint8_t> const tagBytes{0x4DU, 0x4FU, 0x4DU};
  rti::ParameterHandleValueMap values;
  values.emplace(
      publisherParameter,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      values,
      tag,
      *messageTime);
  require(
      retraction.isValid(),
      "Timestamped service-report interaction returned an invalid retraction handle");

  waitFor(
      observer,
      [&] { return observer.recorder().interaction().present; },
      options,
      "the timestamped HLAreportServiceInvocation callback");
  auto const report = observer.recorder().interaction();
  require(
      report.interaction == reportClass &&
          report.parameters.size() == reportParameters.size() &&
          !report.producer.isValid(),
      "Timestamped service-report callback returned the wrong interaction identity or producer metadata");
  require(
      report.tag.empty() && !report.regions.has_value(),
      "Timestamped service-report callback returned user tag or region metadata");
  require(
      report.transportation ==
          observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable"),
      "Timestamped service-report callback returned the wrong transportation type");
  for (auto const parameterHandle : reportParameters) {
    require(
        report.parameters.count(parameterHandle) == 1U,
        "Timestamped service-report callback omitted a standard report parameter");
  }

  rti::HLAunicodeString service;
  service.decode(report.parameters.at(reportParameters[0]));
  require(
      service.get() == L"SendInteraction",
      "Timestamped service-report callback returned the wrong service name");
  rti::HLAinteger16BE serviceType;
  serviceType.decode(report.parameters.at(reportParameters[1]));
  require(
      serviceType.get() == 2,
      "Timestamped service-report callback returned the wrong interaction service type");
  rti::HLAboolean success;
  success.decode(report.parameters.at(reportParameters[2]));
  require(success.get(), "Timestamped service-report callback did not report success");
  rti::HLAunicodeString exception;
  exception.decode(report.parameters.at(reportParameters[5]));
  require(exception.get().empty(), "Successful timestamped service report carried an exception");
  rti::HLAinteger32BE serial;
  serial.decode(report.parameters.at(reportParameters[6]));
  require(serial.get() == 0, "First timestamped service report did not use serial number zero");

  require(
      receiver.recorder().timedInteractions().empty(),
      "Timestamped service-report source interaction bypassed time-constrained delivery");
  auto const receiverTarget = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      6U);
  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      1U);
  receiver.rtiAmbassador().timeAdvanceRequest(*receiverTarget);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  waitFor(
      publisher,
      receiver,
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timedInteractions().size() >= 1U;
      },
      options,
      "timestamped service-report source interaction delivery");
  auto const interactions = receiver.recorder().timedInteractions();
  require(
      interactions.size() == 1U,
      "Timestamped service-report source interaction delivered duplicate callbacks");
  auto const& interaction = interactions.front();
  require(
      interaction.interaction == receiverInteraction &&
          interaction.parameters.size() == 1U &&
          interaction.parameters.count(receiverParameter) == 1U,
      "Timestamped service-report source interaction returned the wrong class or parameters");
  require(
      copyBytes(interaction.parameters.at(receiverParameter)) == valueBytes &&
          interaction.tag == tagBytes && interaction.time == encodeTime(*messageTime),
      "Timestamped service-report source interaction did not preserve payload, tag, or time");
  require(
      interaction.producer == publisher.federateHandle() &&
          interaction.sentOrder == rti::TIMESTAMP &&
          interaction.receivedOrder == rti::TIMESTAMP &&
          !interaction.timeText.empty(),
      "Timestamped service-report source interaction returned the wrong producer or order metadata");
  require(
      interaction.transportation ==
          receiver.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable"),
      "Timestamped service-report source interaction returned the wrong transportation type");
  require(
      interaction.retractionPresent &&
          interaction.retraction == copyBytes(retraction.encode()),
      "Timestamped service-report source interaction omitted its retraction handle");

  receiver.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  publisher.rtiAmbassador().setServiceReportingSwitch(false);
  receiver.resign(rti::NO_ACTION);
  observer.resign(rti::NO_ACTION);
  publisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  receiver.disconnect();
  publisher.disconnect();
}


} // namespace hla_rti_cpp_tck::detail
