#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioServiceReportTimestampedDirectedInteraction(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Timestamped directed-interaction service-report testing requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "Timestamped directed-interaction service-report testing requires an adapter-supplied standard MIM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped directed-interaction service-report testing requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "service-report-timestamped-directed-publisher");
  Session receiver(options, model, "service-report-timestamped-directed-receiver");
  Session observer(options, model, "service-report-timestamped-directed-observer");
  auto const federation =
      federationName(options, "service-report-timestamped-directed-interaction");
  publisher.connect();
  receiver.connect();
  observer.connect();
  publisher.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(
      options.ownerFederateName + L"-service-report-timestamped-directed-publisher",
      options.federateType,
      federation);
  receiver.join(
      options.memberFederateName + L"-service-report-timestamped-directed-receiver",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-timestamped-directed-observer",
      options.federateType,
      federation);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const receiverClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  auto const receiverAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass,
      options.attributeName);
  auto const publisherInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const receiverInteraction = receiver.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  require(
      publisherClass.isValid() && receiverClass.isValid() &&
          publisherAttribute.isValid() && receiverAttribute.isValid() &&
          publisherInteraction.isValid() && receiverInteraction.isValid(),
      "Timestamped directed-interaction service-report lookup returned an invalid handle");

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Timestamped directed-interaction service-report lookup returned an invalid HLAreportServiceInvocation handle");
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
        "Timestamped directed-interaction service-report parameter lookup returned an invalid handle");
    reportParameters.push_back(parameter);
  }
  auto const observerReliable =
      observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable");
  auto const receiverReliable =
      receiver.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable");
  require(
      observerReliable.isValid() && receiverReliable.isValid(),
      "Timestamped directed-interaction service-report lookup returned an invalid reliable transportation handle");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const receiverAttributes{receiverAttribute};
  rti::InteractionClassHandleSet const publisherDirected{publisherInteraction};
  rti::InteractionClassHandleSet const receiverDirected{receiverInteraction};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes,
      true,
      L"");
  publisher.rtiAmbassador().publishObjectClassDirectedInteractions(
      publisherClass,
      publisherDirected);
  receiver.rtiAmbassador().subscribeObjectClassDirectedInteractions(
      receiverClass,
      receiverDirected,
      true);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::TIMESTAMP);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);

  auto const target = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      target.isValid(),
      "Timestamped directed-interaction service-report registration returned an invalid handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(target); },
      options,
      "timestamped directed-interaction service-report target discovery");

  publisher.rtiAmbassador().setServiceReportingSwitch(false);
  publisher.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  receiver.rtiAmbassador().setServiceReportingSwitch(false);
  receiver.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.recorder().clearInteraction();
  receiver.recorder().clearTimedDirectedInteractions();

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "Timestamped directed-interaction service-report members selected different logical-time factories");
  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "timestamped directed-interaction service-report time-constrained callback");
  auto const lookaheadTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto lookahead = publisherTime.factory->makeZero();
  require(
      lookahead != nullptr,
      "Timestamped directed-interaction service-report could not allocate a lookahead interval");
  lookahead->setToDifference(*lookaheadTime, *publisherTime.initial);
  publisher.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "timestamped directed-interaction service-report time-regulation callback");

  auto const timestamp = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      6U);
  std::vector<std::uint8_t> const tagBytes{'t', 's', 'o'};
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  publisher.rtiAmbassador().setServiceReportingSwitch(true);
  auto const retraction = publisher.rtiAmbassador().sendDirectedInteraction(
      publisherInteraction,
      target,
      rti::ParameterHandleValueMap{},
      tag,
      *timestamp);
  require(
      retraction.isValid(),
      "Timestamped directed-interaction service-report send returned an invalid retraction handle");

  if (model == rti::HLA_IMMEDIATE) {
    require(
        observer.recorder().interactions().size() == 1U,
        "Immediate timestamped directed-interaction service report was not delivered synchronously");
  } else {
    require(
        observer.recorder().interactions().empty(),
        "Evoked timestamped directed-interaction service report was delivered before callback servicing");
    waitFor(
        observer,
        [&] { return observer.recorder().interactions().size() == 1U; },
        options,
        "timestamped directed-interaction service-report callback");
  }
  require(
      receiver.recorder().timedDirectedInteractions().empty() &&
          receiver.recorder().timeAdvanceGrants().empty(),
      "Timestamped directed interaction was delivered before its time advance");

  auto const report = observer.recorder().interactions().at(0U);
  require(
      report.interaction == reportClass &&
          report.parameters.size() == reportParameters.size() &&
          report.tag.empty() && !report.producer.isValid() &&
          !report.regions.has_value() && report.transportation == observerReliable,
      "Timestamped directed-interaction service report returned non-standard metadata");
  for (auto const& parameter : reportParameters) {
    require(
        report.parameters.count(parameter) == 1U,
        "Timestamped directed-interaction service report omitted a standard parameter");
  }
  rti::HLAunicodeString decodedService;
  decodedService.decode(report.parameters.at(reportParameters.at(0U)));
  require(
      decodedService.get() == L"SendDirectedInteraction",
      "Timestamped directed-interaction service report returned the wrong service name");
  rti::HLAinteger16BE decodedServiceType;
  decodedServiceType.decode(report.parameters.at(reportParameters.at(1U)));
  require(
      decodedServiceType.get() == 2,
      "Timestamped directed-interaction service report returned the wrong service type");
  rti::HLAboolean decodedSuccess;
  decodedSuccess.decode(report.parameters.at(reportParameters.at(2U)));
  require(
      decodedSuccess.get(),
      "Timestamped directed-interaction service report was not successful");

  auto const quoted = [](std::wstring const& value) {
    return std::wstring{L"\""} + value + L"\"";
  };
  rti::HLAfixedRecord argumentPrototype;
  argumentPrototype.appendElement(rti::HLAinteger32BE{})
      .appendElement(rti::HLAunicodeString{})
      .appendElement(rti::HLAunicodeString{});
  rti::HLAvariableArray suppliedArguments{argumentPrototype};
  suppliedArguments.decode(report.parameters.at(reportParameters.at(3U)));
  require(
      suppliedArguments.size() == 5U,
      "Timestamped directed-interaction service report returned the wrong supplied-argument count");
  auto const verifyArgument = [&](std::size_t index,
                                  rti::Integer32 type,
                                  std::wstring const& name,
                                  std::wstring const& value) {
    auto const& argument = dynamic_cast<rti::HLAfixedRecord const&>(
        suppliedArguments.get(index));
    require(
        dynamic_cast<rti::HLAinteger32BE const&>(argument.get(0U)).get() == type &&
            dynamic_cast<rti::HLAunicodeString const&>(argument.get(1U)).get() == name &&
            dynamic_cast<rti::HLAunicodeString const&>(argument.get(2U)).get() == value,
        "Timestamped directed-interaction service report returned a wrong supplied argument");
  };
  verifyArgument(
      0U,
      27,
      L"Interaction class designator",
      quoted(publisherInteraction.toString()));
  verifyArgument(
      1U,
      37,
      L"Object instance designator",
      quoted(target.toString()));
  verifyArgument(
      2U,
      40,
      L"Constrained set of interaction parameter designator and value pairs",
      L"{}");
  verifyArgument(3U, 60, L"User-supplied tag", L"\"dHNv\"");
  verifyArgument(
      4U,
      31,
      L"Optional timestamp",
      quoted(timestamp->toString()));

  rti::HLAfixedRecord returnedArgument;
  returnedArgument.appendElement(rti::HLAinteger32BE{})
      .appendElement(rti::HLAunicodeString{})
      .appendElement(rti::HLAunicodeString{});
  returnedArgument.decode(report.parameters.at(reportParameters.at(4U)));
  require(
      dynamic_cast<rti::HLAinteger32BE const&>(returnedArgument.get(0U)).get() == 33 &&
          dynamic_cast<rti::HLAunicodeString const&>(returnedArgument.get(1U)).get() ==
              L"Message retraction designator",
      "Timestamped directed-interaction service report returned the wrong retraction argument");
  auto const retractionText = retraction.toString();
  auto const retractionOpen = retractionText.find(L'(');
  auto const retractionClose = retractionText.find(L')');
  require(
      retractionOpen != std::wstring::npos &&
          retractionClose > retractionOpen + 1U,
      "Timestamped directed-interaction retraction had no standard text payload");
  auto const momRetraction = dynamic_cast<rti::HLAunicodeString const&>(
      returnedArgument.get(2U)).get();
  constexpr std::wstring_view momRetractionPrefix = L"\"MessageRetractionHandle<";
  require(
      momRetraction.rfind(std::wstring{momRetractionPrefix}, 0U) == 0U &&
          momRetraction.size() > momRetractionPrefix.size() + 1U &&
          momRetraction[momRetraction.size() - 2U] == L'>' &&
          momRetraction.back() == L'\"' &&
          momRetraction.substr(
              momRetractionPrefix.size(),
              momRetraction.size() - momRetractionPrefix.size() - 2U) ==
              retractionText.substr(
                  retractionOpen + 1U,
                  retractionClose - retractionOpen - 1U),
      "Timestamped directed-interaction service report returned a mismatched retraction handle");
  rti::HLAunicodeString decodedException;
  decodedException.decode(report.parameters.at(reportParameters.at(5U)));
  require(
      decodedException.get().empty(),
      "Successful timestamped directed-interaction service report carried an exception");
  rti::HLAinteger32BE decodedSerial;
  decodedSerial.decode(report.parameters.at(reportParameters.at(6U)));
  require(
      decodedSerial.get() == 0,
      "Timestamped directed-interaction service report did not start at serial zero");

  receiver.recorder().clearCallbackOrder();
  publisher.recorder().clearCallbackOrder();
  publisher.rtiAmbassador().setServiceReportingSwitch(false);
  publisher.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  receiver.rtiAmbassador().timeAdvanceRequest(*timestamp);
  require(
      receiver.recorder().timedDirectedInteractions().empty(),
      "Timestamped directed interaction was delivered before the regulating federate advanced");
  auto const publisherAdvance = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      1U);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherAdvance);
  auto const deadline = Clock::now() +
      std::chrono::milliseconds(options.timeoutMilliseconds);
  while (Clock::now() < deadline) {
    publisher.pump();
    receiver.pump();
    if (publisher.recorder().timeAdvanceGrants().size() == 1U &&
        receiver.recorder().timeAdvanceGrants().size() == 1U &&
        receiver.recorder().timedDirectedInteractions().size() == 1U) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  require(
      publisher.recorder().timeAdvanceGrants().size() == 1U &&
          receiver.recorder().timeAdvanceGrants().size() == 1U &&
          receiver.recorder().timedDirectedInteractions().size() == 1U,
      "Timestamped directed-interaction service-report delivery and grants timed out (publisher grants=" +
          std::to_string(publisher.recorder().timeAdvanceGrants().size()) +
          ", receiver grants=" +
          std::to_string(receiver.recorder().timeAdvanceGrants().size()) +
          ", receiver directed=" +
          std::to_string(receiver.recorder().timedDirectedInteractions().size()) +
          ")");
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"directed", "grant"},
      "Timestamped directed-interaction service report route delivered after its grant");

  auto const directed = receiver.recorder().timedDirectedInteractions().at(0U);
  require(
      directed.interaction == receiverInteraction && directed.object == target &&
          directed.parameters.empty() && directed.tag == tagBytes &&
          directed.transportation == receiverReliable &&
          directed.producer == publisher.federateHandle() &&
          directed.time == encodeTime(*timestamp) &&
          directed.timeText == timestamp->toString() &&
          directed.sentOrder == rti::TIMESTAMP &&
          directed.receivedOrder == rti::TIMESTAMP &&
          directed.retractionPresent &&
          directed.retraction == copyBytes(retraction.encode()),
      "Timestamped directed-interaction callback returned non-standard data");
  require(
      receiver.recorder().retractions().empty(),
      "Timestamped directed-interaction service report route produced an unexpected retraction callback");

  receiver.rtiAmbassador().unsubscribeObjectClassDirectedInteractions(
      receiverClass,
      receiverDirected);
  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes);
  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  receiver.resign(rti::NO_ACTION);
  observer.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  receiver.disconnect();
  publisher.disconnect();
}

} // namespace hla_rti_cpp_tck::detail
