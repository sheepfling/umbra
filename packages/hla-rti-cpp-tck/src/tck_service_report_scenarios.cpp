#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioServiceReportTimestampedAttributeUpdate(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Timestamped attribute-update service-report testing requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "Timestamped attribute-update service-report testing requires an adapter-supplied standard MIM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped attribute-update service-report testing requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "service-report-timestamped-attribute-publisher");
  Session receiver(options, model, "service-report-timestamped-attribute-receiver");
  Session observer(options, model, "service-report-timestamped-attribute-observer");
  auto const federation = federationName(options, "service-report-timestamped-attribute-update");
  publisher.connect();
  receiver.connect();
  observer.connect();
  publisher.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(
      options.ownerFederateName + L"-service-report-timestamped-attribute-publisher",
      options.federateType,
      federation);
  receiver.join(
      options.memberFederateName + L"-service-report-timestamped-attribute-receiver",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-timestamped-attribute-observer",
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
  require(
      publisherClass.isValid() && receiverClass.isValid() &&
          publisherAttribute.isValid() && receiverAttribute.isValid(),
      "Timestamped attribute-update service-report lookup returned an invalid handle");

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Timestamped attribute-update service-report lookup returned an invalid HLAreportServiceInvocation handle");
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
        "Timestamped attribute-update service-report parameter lookup returned an invalid handle");
    reportParameters.push_back(parameter);
  }
  auto const observerReliable =
      observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable");
  auto const receiverReliable =
      receiver.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable");
  require(
      observerReliable.isValid() && receiverReliable.isValid(),
      "Timestamped attribute-update service-report lookup returned an invalid reliable transportation handle");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const receiverAttributes{receiverAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes,
      true,
      L"");
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherClass,
      publisherAttributes,
      rti::TIMESTAMP);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      object.isValid(),
      "Timestamped attribute-update service-report registration returned an invalid object handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "timestamped attribute-update service-report object discovery");

  publisher.rtiAmbassador().setServiceReportingSwitch(false);
  publisher.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  receiver.rtiAmbassador().setServiceReportingSwitch(false);
  receiver.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.recorder().clearInteraction();
  receiver.recorder().clearTimedReflections();

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "Timestamped attribute-update service-report members selected different logical-time factories");
  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "timestamped attribute-update service-report time-constrained callback");
  auto const lookaheadTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto lookahead = publisherTime.factory->makeZero();
  require(
      lookahead != nullptr,
      "Timestamped attribute-update service-report could not allocate a lookahead interval");
  lookahead->setToDifference(*lookaheadTime, *publisherTime.initial);
  publisher.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "timestamped attribute-update service-report time-regulation callback");

  auto const timestamp = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      6U);
  std::vector<std::uint8_t> const valueBytes{0x01U, 0x02U};
  std::vector<std::uint8_t> const tagBytes{'t', 's', 'o'};
  rti::VariableLengthData const value(valueBytes.data(), valueBytes.size());
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  rti::AttributeHandleValueMap const values{{publisherAttribute, value}};
  publisher.rtiAmbassador().setServiceReportingSwitch(true);
  auto const retraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      values,
      tag,
      *timestamp);
  require(
      retraction.isValid(),
      "Timestamped attribute-update service-report update returned an invalid retraction handle");

  if (model == rti::HLA_IMMEDIATE) {
    require(
        observer.recorder().interactions().size() == 1U,
        "Immediate timestamped attribute-update service report was not delivered synchronously");
  } else {
    require(
        observer.recorder().interactions().empty(),
        "Evoked timestamped attribute-update service report was delivered before callback servicing");
    waitFor(
        observer,
        [&] { return observer.recorder().interactions().size() == 1U; },
        options,
        "timestamped attribute-update service-report callback");
  }
  require(
      receiver.recorder().timedReflections().empty() &&
          receiver.recorder().timeAdvanceGrants().empty(),
      "Timestamped attribute update was delivered before its time advance");

  auto const report = observer.recorder().interactions().at(0U);
  require(
      report.interaction == reportClass &&
          report.parameters.size() == reportParameters.size() &&
          report.tag.empty() && !report.producer.isValid() &&
          !report.regions.has_value() && report.transportation == observerReliable,
      "Timestamped attribute-update service report returned non-standard metadata");
  for (auto const& parameter : reportParameters) {
    require(
        report.parameters.count(parameter) == 1U,
        "Timestamped attribute-update service report omitted a standard parameter");
  }

  rti::HLAunicodeString decodedService;
  decodedService.decode(report.parameters.at(reportParameters.at(0U)));
  require(
      decodedService.get() == L"UpdateAttributeValues",
      "Timestamped attribute-update service report returned the wrong service name");
  rti::HLAinteger16BE decodedServiceType;
  decodedServiceType.decode(report.parameters.at(reportParameters.at(1U)));
  require(
      decodedServiceType.get() == 2,
      "Timestamped attribute-update service report returned the wrong service type");
  rti::HLAboolean decodedSuccess;
  decodedSuccess.decode(report.parameters.at(reportParameters.at(2U)));
  require(
      decodedSuccess.get(),
      "Timestamped attribute-update service report was not successful");

  auto const quoted = [](std::wstring const& valueText) {
    return std::wstring{L"\""} + valueText + L"\"";
  };
  rti::HLAfixedRecord argumentPrototype;
  argumentPrototype.appendElement(rti::HLAinteger32BE{})
      .appendElement(rti::HLAunicodeString{})
      .appendElement(rti::HLAunicodeString{});
  rti::HLAvariableArray suppliedArguments{argumentPrototype};
  suppliedArguments.decode(report.parameters.at(reportParameters.at(3U)));
  require(
      suppliedArguments.size() == 4U,
      "Timestamped attribute-update service report returned the wrong supplied-argument count");
  auto const verifyArgument = [&](std::size_t index,
                                  rti::Integer32 type,
                                  std::wstring const& name,
                                  std::wstring const& valueText) {
    auto const& argument = dynamic_cast<rti::HLAfixedRecord const&>(
        suppliedArguments.get(index));
    require(
        dynamic_cast<rti::HLAinteger32BE const&>(argument.get(0U)).get() == type &&
            dynamic_cast<rti::HLAunicodeString const&>(argument.get(1U)).get() == name &&
            dynamic_cast<rti::HLAunicodeString const&>(argument.get(2U)).get() == valueText,
        "Timestamped attribute-update service report returned a wrong supplied argument");
  };
  verifyArgument(
      0U,
      37,
      L"Object instance designator",
      quoted(object.toString()));
  verifyArgument(
      1U,
      2,
      L"Constrained set of attribute designator and value pairs",
      std::wstring{L"{"} + quoted(publisherAttribute.toString()) + L":\"AQI=\"}");
  verifyArgument(2U, 60, L"User-supplied tag", L"\"dHNv\"");
  verifyArgument(
      3U,
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
      "Timestamped attribute-update service report returned the wrong retraction argument");
  auto const retractionText = retraction.toString();
  auto const retractionOpen = retractionText.find(L'(');
  auto const retractionClose = retractionText.find(L')');
  require(
      retractionOpen != std::wstring::npos &&
          retractionClose > retractionOpen + 1U,
      "Timestamped attribute-update retraction had no standard text payload");
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
      "Timestamped attribute-update service report returned a mismatched retraction handle");
  rti::HLAunicodeString decodedException;
  decodedException.decode(report.parameters.at(reportParameters.at(5U)));
  require(
      decodedException.get().empty(),
      "Successful timestamped attribute-update service report carried an exception");
  rti::HLAinteger32BE decodedSerial;
  decodedSerial.decode(report.parameters.at(reportParameters.at(6U)));
  require(
      decodedSerial.get() == 0,
      "Timestamped attribute-update service report did not start at serial zero");

  receiver.recorder().clearCallbackOrder();
  publisher.recorder().clearCallbackOrder();
  publisher.rtiAmbassador().setServiceReportingSwitch(false);
  publisher.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  receiver.rtiAmbassador().timeAdvanceRequest(*timestamp);
  require(
      receiver.recorder().timedReflections().empty(),
      "Timestamped attribute update was delivered before the regulating federate advanced");
  auto const publisherAdvance = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherAdvance);
  waitFor(
      publisher,
      receiver,
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() == 1U &&
            receiver.recorder().timeAdvanceGrants().size() == 1U &&
            receiver.recorder().timedReflections().size() == 1U;
      },
      options,
      "timestamped attribute-update service-report delivery and grants");
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"reflect", "grant"},
      "Timestamped attribute-update service report route delivered after its grant");

  auto const reflection = receiver.recorder().timedReflections().at(0U);
  require(
      reflection.object == object &&
          reflection.values.size() == 1U &&
          reflection.values.count(receiverAttribute) == 1U &&
          copyBytes(reflection.values.at(receiverAttribute)) == valueBytes &&
          reflection.tag == tagBytes &&
          reflection.transportation == receiverReliable &&
          reflection.producer == publisher.federateHandle() &&
          reflection.time == encodeTime(*timestamp) &&
          reflection.timeText == timestamp->toString() &&
          reflection.sentOrder == rti::TIMESTAMP &&
          reflection.receivedOrder == rti::TIMESTAMP &&
          reflection.retractionPresent &&
          reflection.retraction == copyBytes(retraction.encode()) &&
          !reflection.regions.has_value(),
      "Timestamped attribute-update reflection returned non-standard data");

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

void scenarioServiceReportTimestampedAttributeUpdateContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportTimestampedAttributeUpdate(options, model);
}

void scenarioServiceReportTimestampedDeleteObjectInstance(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Timestamped delete-object service-report testing requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "Timestamped delete-object service-report testing requires an adapter-supplied standard MIM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped delete-object service-report testing requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "service-report-timestamped-delete-publisher");
  Session receiver(options, model, "service-report-timestamped-delete-receiver");
  Session observer(options, model, "service-report-timestamped-delete-observer");
  auto const federation = federationName(options, "service-report-timestamped-delete-object-instance");
  publisher.connect();
  receiver.connect();
  observer.connect();
  publisher.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(
      options.ownerFederateName + L"-service-report-timestamped-delete-publisher",
      options.federateType,
      federation);
  receiver.join(
      options.memberFederateName + L"-service-report-timestamped-delete-receiver",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-timestamped-delete-observer",
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
  require(
      publisherClass.isValid() && receiverClass.isValid() &&
          publisherAttribute.isValid() && receiverAttribute.isValid(),
      "Timestamped delete-object service-report lookup returned an invalid object or attribute handle");

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Timestamped delete-object service-report lookup returned an invalid HLAreportServiceInvocation handle");
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
        "Timestamped delete-object service-report parameter lookup returned an invalid handle");
    reportParameters.push_back(parameter);
  }
  auto const observerReliable =
      observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable");
  require(
      observerReliable.isValid(),
      "Timestamped delete-object service-report lookup returned an invalid reliable transportation handle");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const receiverAttributes{receiverAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes,
      true,
      L"");
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      object.isValid(),
      "Timestamped delete-object service-report registration returned an invalid object handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "timestamped delete-object service-report object discovery");

  publisher.rtiAmbassador().setServiceReportingSwitch(false);
  publisher.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  receiver.rtiAmbassador().setServiceReportingSwitch(false);
  receiver.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.recorder().clearInteraction();
  receiver.recorder().clearTimedRemovals();

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "Timestamped delete-object service-report members selected different logical-time factories");
  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "timestamped delete-object service-report time-constrained callback");
  auto const lookaheadTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto lookahead = publisherTime.factory->makeZero();
  require(
      lookahead != nullptr,
      "Timestamped delete-object service-report could not allocate a lookahead interval");
  lookahead->setToDifference(*lookaheadTime, *publisherTime.initial);
  publisher.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "timestamped delete-object service-report time-regulation callback");

  auto const timestamp = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      6U);
  std::vector<std::uint8_t> const tagBytes{'t', 's', 'o'};
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  publisher.rtiAmbassador().setServiceReportingSwitch(true);
  auto const retraction = publisher.rtiAmbassador().deleteObjectInstance(
      object,
      tag,
      *timestamp);
  require(
      retraction.isValid(),
      "Timestamped delete-object service-report delete returned an invalid retraction handle");

  if (model == rti::HLA_IMMEDIATE) {
    require(
        observer.recorder().interactions().size() == 1U,
        "Immediate timestamped delete-object service report was not delivered synchronously");
  } else {
    require(
        observer.recorder().interactions().empty(),
        "Evoked timestamped delete-object service report was delivered before callback servicing");
    waitFor(
        observer,
        [&] { return observer.recorder().interactions().size() == 1U; },
        options,
        "timestamped delete-object service-report callback");
  }
  require(
      receiver.recorder().timedRemovals().empty() &&
          receiver.recorder().timeAdvanceGrants().empty(),
      "Timestamped object deletion was delivered before its time advance");

  auto const report = observer.recorder().interactions().at(0U);
  require(
      report.interaction == reportClass &&
          report.parameters.size() == reportParameters.size() &&
          report.tag.empty() && !report.producer.isValid() &&
          !report.regions.has_value() && report.transportation == observerReliable,
      "Timestamped delete-object service report returned non-standard metadata");
  for (auto const& parameter : reportParameters) {
    require(
        report.parameters.count(parameter) == 1U,
        "Timestamped delete-object service report omitted a standard parameter");
  }

  rti::HLAunicodeString decodedService;
  decodedService.decode(report.parameters.at(reportParameters.at(0U)));
  require(
      decodedService.get() == L"DeleteObjectInstance",
      "Timestamped delete-object service report returned the wrong service name");
  rti::HLAinteger16BE decodedServiceType;
  decodedServiceType.decode(report.parameters.at(reportParameters.at(1U)));
  require(
      decodedServiceType.get() == 2,
      "Timestamped delete-object service report returned the wrong service type");
  rti::HLAboolean decodedSuccess;
  decodedSuccess.decode(report.parameters.at(reportParameters.at(2U)));
  require(
      decodedSuccess.get(),
      "Timestamped delete-object service report was not successful");

  auto const quoted = [](std::wstring const& valueText) {
    return std::wstring{L"\""} + valueText + L"\"";
  };
  rti::HLAfixedRecord argumentPrototype;
  argumentPrototype.appendElement(rti::HLAinteger32BE{})
      .appendElement(rti::HLAunicodeString{})
      .appendElement(rti::HLAunicodeString{});
  rti::HLAvariableArray suppliedArguments{argumentPrototype};
  suppliedArguments.decode(report.parameters.at(reportParameters.at(3U)));
  require(
      suppliedArguments.size() == 3U,
      "Timestamped delete-object service report returned the wrong supplied-argument count");
  auto const verifyArgument = [&](std::size_t index,
                                  rti::Integer32 type,
                                  std::wstring const& name,
                                  std::wstring const& valueText) {
    auto const& argument = dynamic_cast<rti::HLAfixedRecord const&>(
        suppliedArguments.get(index));
    require(
        dynamic_cast<rti::HLAinteger32BE const&>(argument.get(0U)).get() == type &&
            dynamic_cast<rti::HLAunicodeString const&>(argument.get(1U)).get() == name &&
            dynamic_cast<rti::HLAunicodeString const&>(argument.get(2U)).get() == valueText,
        "Timestamped delete-object service report returned a wrong supplied argument");
  };
  verifyArgument(
      0U,
      37,
      L"Object instance designator",
      quoted(object.toString()));
  verifyArgument(1U, 60, L"User-supplied tag", L"\"dHNv\"");
  verifyArgument(
      2U,
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
      "Timestamped delete-object service report returned the wrong retraction argument");
  auto const retractionText = retraction.toString();
  auto const retractionOpen = retractionText.find(L'(');
  auto const retractionClose = retractionText.find(L')');
  require(
      retractionOpen != std::wstring::npos &&
          retractionClose > retractionOpen + 1U,
      "Timestamped delete-object retraction had no standard text payload");
  auto const momRetraction = dynamic_cast<rti::HLAunicodeString const&>(
                                 returnedArgument.get(2U))
                                 .get();
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
      "Timestamped delete-object service report returned a mismatched retraction handle");
  rti::HLAunicodeString decodedException;
  decodedException.decode(report.parameters.at(reportParameters.at(5U)));
  require(
      decodedException.get().empty(),
      "Successful timestamped delete-object service report carried an exception");
  rti::HLAinteger32BE decodedSerial;
  decodedSerial.decode(report.parameters.at(reportParameters.at(6U)));
  require(
      decodedSerial.get() == 0,
      "Timestamped delete-object service report did not start at serial zero");

  receiver.recorder().clearCallbackOrder();
  publisher.recorder().clearCallbackOrder();
  publisher.rtiAmbassador().setServiceReportingSwitch(false);
  publisher.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  receiver.rtiAmbassador().timeAdvanceRequest(*timestamp);
  require(
      receiver.recorder().timedRemovals().empty(),
      "Timestamped object deletion was delivered before the regulating federate advanced");
  auto const publisherAdvance = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherAdvance);
  waitFor(
      publisher,
      receiver,
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() == 1U &&
            receiver.recorder().timeAdvanceGrants().size() == 1U &&
            receiver.recorder().timedRemovals().size() == 1U;
      },
      options,
      "timestamped delete-object service-report delivery and grants");
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"remove", "grant"},
      "Timestamped delete-object service report route delivered after its grant");

  auto const removal = receiver.recorder().timedRemovals().at(0U);
  require(
      removal.object == object &&
          removal.tag == tagBytes &&
          removal.producer == publisher.federateHandle() &&
          removal.time == encodeTime(*timestamp) &&
          removal.timeText == timestamp->toString() &&
          removal.sentOrder == rti::TIMESTAMP &&
          removal.receivedOrder == rti::TIMESTAMP &&
          removal.retractionPresent &&
          removal.retraction == copyBytes(retraction.encode()),
      "Timestamped delete-object removal returned non-standard metadata");

  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes);
  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  receiver.resign(rti::NO_ACTION);
  observer.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioServiceReportTimestampedDeleteObjectInstanceContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportTimestampedDeleteObjectInstance(options, model);
}

void scenarioServiceReportInterlock(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty(),
      "Service-report interlock testing requires an adapter-supplied standard MIM");

  Session session(options, model, "service-report-interlock");
  auto const federation = federationName(options, "service-report-interlock");
  session.connect();
  session.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  session.join(
      options.ownerFederateName + L"-service-report-interlock",
      options.federateType,
      federation);

  auto const reportClass = session.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Service-report interlock lookup returned an invalid HLAreportServiceInvocation handle");
  require(
      !session.rtiAmbassador().getServiceReportingSwitch(),
      "Service-report interlock did not start with reporting disabled");

  session.rtiAmbassador().setServiceReportingSwitch(true);
  require(
      session.rtiAmbassador().getServiceReportingSwitch(),
      "Service-report interlock did not enable service reporting");
  requireException(
      [&] { session.rtiAmbassador().subscribeInteractionClass(reportClass); },
      L"FederateServiceInvocationsAreBeingReportedViaMOM",
      "active HLAreportServiceInvocation subscription while reporting is enabled");
  requireException(
      [&] { session.rtiAmbassador().subscribeInteractionClass(reportClass, false); },
      L"FederateServiceInvocationsAreBeingReportedViaMOM",
      "passive HLAreportServiceInvocation subscription while reporting is enabled");
  require(
      session.rtiAmbassador().getServiceReportingSwitch(),
      "Rejected HLAreportServiceInvocation subscriptions changed the reporting switch");

  session.rtiAmbassador().setServiceReportingSwitch(false);
  require(
      !session.rtiAmbassador().getServiceReportingSwitch(),
      "Service-report interlock did not disable service reporting");
  session.rtiAmbassador().subscribeInteractionClass(reportClass);
  requireException(
      [&] { session.rtiAmbassador().setServiceReportingSwitch(true); },
      L"ReportServiceInvocationsAreSubscribed",
      "enabling reporting with an active HLAreportServiceInvocation subscription");
  require(
      !session.rtiAmbassador().getServiceReportingSwitch(),
      "Rejected active HLAreportServiceInvocation subscription changed the reporting switch");
  session.rtiAmbassador().unsubscribeInteractionClass(reportClass);

  session.rtiAmbassador().subscribeInteractionClass(reportClass, false);
  requireException(
      [&] { session.rtiAmbassador().setServiceReportingSwitch(true); },
      L"ReportServiceInvocationsAreSubscribed",
      "enabling reporting with a passive HLAreportServiceInvocation subscription");
  require(
      !session.rtiAmbassador().getServiceReportingSwitch(),
      "Rejected passive HLAreportServiceInvocation subscription changed the reporting switch");
  session.rtiAmbassador().unsubscribeInteractionClass(reportClass);

  session.rtiAmbassador().setServiceReportingSwitch(true);
  require(
      session.rtiAmbassador().getServiceReportingSwitch(),
      "Service-report interlock did not re-enable reporting after unsubscribe");
  session.rtiAmbassador().setServiceReportingSwitch(false);
  session.resign(rti::NO_ACTION);
  session.rtiAmbassador().destroyFederationExecution(federation);
  session.disconnect();
}

void scenarioServiceReportInterlockContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportInterlock(options, model);
}

void scenarioServiceReportSynchronization(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty(),
      "Synchronization service-report testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "service-report-synchronization-subject");
  Session observer(options, model, "service-report-synchronization-observer");
  Session reportSink(options, model, "service-report-synchronization-report-sink");
  auto const federation = federationName(options, "service-report-synchronization");
  subject.connect();
  observer.connect();
  reportSink.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-service-report-synchronization-subject",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-synchronization-observer",
      options.federateType,
      federation);
  reportSink.join(
      options.memberFederateName + L"-service-report-synchronization-report-sink",
      options.federateType,
      federation);

  auto const reportClass = reportSink.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Synchronization service-report lookup returned an invalid HLAreportServiceInvocation handle");
  std::vector<rti::ParameterHandle> const reportParameters = [&] {
    std::vector<rti::ParameterHandle> result;
    for (auto const& name : {
             L"HLAservice",
             L"HLAserviceType",
             L"HLAsuccessIndicator",
             L"HLAsuppliedArguments",
             L"HLAreturnedArgument",
             L"HLAexception",
             L"HLAserialNumber",
             L"HLAfederate"}) {
      auto const parameter = reportSink.rtiAmbassador().getParameterHandle(reportClass, name);
      require(
          parameter.isValid(),
          "Synchronization service-report lookup returned an invalid report parameter handle");
      result.push_back(parameter);
    }
    return result;
  }();
  auto const reliable = reportSink.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable");

  // Service-report routing is enabled only after all setup declarations have
  // completed, so the asserted serial stream starts with the registration
  // service itself.  MOM serial numbers are scoped to the reported federate;
  // leave the observer switch disabled so this no-region subscription exposes
  // one complete, unambiguous subject stream.
  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  reportSink.rtiAmbassador().setServiceReportingSwitch(false);
  reportSink.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  reportSink.rtiAmbassador().subscribeInteractionClass(reportClass);
  subject.rtiAmbassador().setServiceReportingSwitch(true);

  auto const reportService = [&](InteractionRecord const& report) {
    rti::HLAunicodeString service;
    service.decode(report.parameters.at(reportParameters.at(0U)));
    return service.get();
  };
  auto const reportCount = [&](std::wstring const& expectedService) {
    std::size_t count = 0U;
    for (auto const& report : reportSink.recorder().interactions()) {
      if (report.interaction == reportClass && reportService(report) == expectedService) {
        ++count;
      }
    }
    return count;
  };
  auto const latestReport = [&](std::wstring const& expectedService) {
    auto const reports = reportSink.recorder().interactions();
    for (auto iterator = reports.rbegin(); iterator != reports.rend(); ++iterator) {
      if (iterator->interaction == reportClass && reportService(*iterator) == expectedService) {
        return *iterator;
      }
    }
    throw std::runtime_error("missing synchronization service report");
  };
  auto const verifyReport = [&](InteractionRecord const& report,
                                std::wstring const& expectedService,
                                std::vector<std::int32_t> const& expectedTypes,
                                std::vector<std::wstring> const& expectedNames,
                                std::int32_t expectedSerial,
                                std::optional<std::wstring> expectedLastArgumentValue) {
    require(
        report.interaction == reportClass &&
            report.parameters.size() == reportParameters.size() &&
            !report.producer.isValid() && report.tag.empty() &&
            !report.regions.has_value() && report.transportation == reliable,
        "Synchronization service report returned non-standard identity or metadata");
    require(
        reportService(report) == expectedService,
        "Synchronization service report returned the wrong service name");

    rti::HLAinteger16BE serviceType;
    serviceType.decode(report.parameters.at(reportParameters.at(1U)));
    require(
        serviceType.get() == 0,
        "Synchronization service report returned the wrong Federation Management type");
    rti::HLAboolean success;
    success.decode(report.parameters.at(reportParameters.at(2U)));
    require(success.get(), "Successful synchronization service report was not marked successful");

    rti::HLAfixedRecord suppliedArgumentPrototype;
    suppliedArgumentPrototype.appendElement(rti::HLAinteger32BE{})
        .appendElement(rti::HLAunicodeString{})
        .appendElement(rti::HLAunicodeString{});
    rti::HLAvariableArray suppliedArguments{suppliedArgumentPrototype};
    suppliedArguments.decode(report.parameters.at(reportParameters.at(3U)));
    require(
        suppliedArguments.size() == expectedTypes.size() &&
            expectedTypes.size() == expectedNames.size(),
        "Synchronization service report returned the wrong supplied-argument count");
    for (std::size_t index = 0U; index != expectedTypes.size(); ++index) {
      auto const& supplied = dynamic_cast<rti::HLAfixedRecord const&>(
          suppliedArguments.get(index));
      auto const actualType =
          dynamic_cast<rti::HLAinteger32BE const&>(supplied.get(0U)).get();
      auto const actualName =
          dynamic_cast<rti::HLAunicodeString const&>(supplied.get(1U)).get();
      auto const actualValue =
          dynamic_cast<rti::HLAunicodeString const&>(supplied.get(2U)).get();
      require(
          actualType == expectedTypes.at(index) &&
              actualName == expectedNames.at(index),
          "Synchronization service report returned the wrong supplied-argument shape");
      if (expectedLastArgumentValue.has_value() && index == 2U) {
        require(
            actualValue == expectedLastArgumentValue.value(),
            "Synchronization service report returned the wrong failure reason value");
      }
    }

    rti::HLAfixedRecord returnedArgument;
    returnedArgument.appendElement(rti::HLAinteger32BE{})
        .appendElement(rti::HLAunicodeString{})
        .appendElement(rti::HLAunicodeString{});
    returnedArgument.decode(report.parameters.at(reportParameters.at(4U)));
    require(
        dynamic_cast<rti::HLAinteger32BE const&>(returnedArgument.get(0U)).get() == 34 &&
            dynamic_cast<rti::HLAunicodeString const&>(returnedArgument.get(1U)).get().empty() &&
            dynamic_cast<rti::HLAunicodeString const&>(returnedArgument.get(2U)).get() ==
                L"null",
        "Synchronization service report returned a non-Null return argument");
    rti::HLAunicodeString exception;
    exception.decode(report.parameters.at(reportParameters.at(5U)));
    require(
        exception.get().empty(),
        "Successful synchronization service report carried an exception");
    rti::HLAinteger32BE serial;
    serial.decode(report.parameters.at(reportParameters.at(6U)));
    require(
        serial.get() == expectedSerial,
        "Synchronization service report returned the wrong serial number (expected " +
            std::to_string(expectedSerial) + ", got " + std::to_string(serial.get()) + ")");
  };

  std::vector<std::uint8_t> const tagBytes{0x53U, 0x59U, 0x4EU, 0x43U};
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  std::wstring const label = L"service-report-synchronization-point";
  rti::FederateHandleSet const synchronizationSet{
      subject.federateHandle(),
      observer.federateHandle()};
  subject.rtiAmbassador().registerFederationSynchronizationPoint(
      label,
      tag,
      synchronizationSet);
  waitForSessions(
      {&subject, &observer, &reportSink},
      [&] { return subject.synchronizationPointRegistrations().size() == 1U; },
      options,
      "synchronization registration callback");
  waitForSessions(
      {&subject, &observer, &reportSink},
      [&] { return observer.synchronizationPointAnnouncements().size() == 1U; },
      options,
      "synchronization announcement callback");
  waitForSessions(
      {&subject, &observer, &reportSink},
      [&] { return reportCount(L"RegisterFederationSynchronizationPoint") == 1U; },
      options,
      "RegisterFederationSynchronizationPoint service report");
  waitForSessions(
      {&subject, &observer, &reportSink},
      [&] { return reportCount(L"ConfirmSynchronizationPointRegistration") == 1U; },
      options,
      "ConfirmSynchronizationPointRegistration service report");
  waitForSessions(
      {&subject, &observer, &reportSink},
      [&] { return reportCount(L"AnnounceSynchronizationPoint") == 1U; },
      options,
      "AnnounceSynchronizationPoint service report");
  verifyReport(
      latestReport(L"RegisterFederationSynchronizationPoint"),
      L"RegisterFederationSynchronizationPoint",
      {53, 60, 18},
      {L"Synchronization point label",
       L"User-supplied tag",
       L"Optional set of joined federate designators"},
      0,
      std::nullopt);
  verifyReport(
      latestReport(L"ConfirmSynchronizationPointRegistration"),
      L"ConfirmSynchronizationPointRegistration",
      {53, 6, 34},
      {L"Synchronization point label",
       L"Registration-success indicator",
       L"Optional failure reason"},
      1,
      std::nullopt);
  verifyReport(
      latestReport(L"AnnounceSynchronizationPoint"),
      L"AnnounceSynchronizationPoint",
      {53, 60},
      {L"Synchronization point label", L"User-supplied tag"},
      2,
      std::nullopt);
  auto const registration = subject.synchronizationPointRegistrations().front();
  require(
      registration.succeeded && registration.label == label,
      "Synchronization service-report registration callback returned the wrong result");
  auto const announcement = observer.synchronizationPointAnnouncements().front();
  require(
      announcement.label == label && announcement.tag == tagBytes,
      "Synchronization service-report announcement returned the wrong label or tag");

  // A duplicate label is accepted as a service invocation and reported as a
  // failed confirmation callback, matching the standard two-stage API.
  subject.rtiAmbassador().registerFederationSynchronizationPoint(
      label,
      tag,
      synchronizationSet);
  waitForSessions(
      {&subject, &observer, &reportSink},
      [&] {
        return subject.synchronizationPointRegistrations().size() == 2U &&
            reportCount(L"RegisterFederationSynchronizationPoint") == 2U &&
            reportCount(L"ConfirmSynchronizationPointRegistration") == 2U;
      },
      options,
      "duplicate synchronization registration service reports");
  verifyReport(
      latestReport(L"RegisterFederationSynchronizationPoint"),
      L"RegisterFederationSynchronizationPoint",
      {53, 60, 18},
      {L"Synchronization point label",
       L"User-supplied tag",
       L"Optional set of joined federate designators"},
      3,
      std::nullopt);
  verifyReport(
      latestReport(L"ConfirmSynchronizationPointRegistration"),
      L"ConfirmSynchronizationPointRegistration",
       {53, 6, 56},
      {L"Synchronization point label",
       L"Registration-success indicator",
       L"Optional failure reason"},
      4,
      L"\"SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE\"");
  auto const duplicate = subject.synchronizationPointRegistrations().back();
  require(
      !duplicate.succeeded && duplicate.label == label &&
          duplicate.failureReason.has_value() &&
          duplicate.failureReason.value() == rti::SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE,
      "Duplicate synchronization registration returned the wrong callback result");

  subject.rtiAmbassador().synchronizationPointAchieved(label, true);
  observer.rtiAmbassador().synchronizationPointAchieved(label, false);
  waitForSessions(
      {&subject, &observer, &reportSink},
      [&] {
        return subject.federationSynchronized().size() == 1U &&
            observer.federationSynchronized().size() == 1U;
      },
      options,
      "synchronization completion callbacks");
  waitForSessions(
      {&subject, &observer, &reportSink},
      [&] { return reportCount(L"SynchronizationPointAchieved") == 1U; },
      options,
      "SynchronizationPointAchieved service report");
  waitForSessions(
      {&subject, &observer, &reportSink},
      [&] { return reportCount(L"FederationSynchronized") == 1U; },
      options,
      "FederationSynchronized service report");
  verifyReport(
      latestReport(L"SynchronizationPointAchieved"),
      L"SynchronizationPointAchieved",
      {53, 6},
      {L"Synchronization point label", L"Optional synchronization-success indicator"},
      5,
      std::nullopt);
  verifyReport(
      latestReport(L"FederationSynchronized"),
      L"FederationSynchronized",
      {53, 18},
      {L"Synchronization point label", L"Set of joined federate designators"},
      6,
      std::nullopt);
  for (auto* session : {&subject, &observer}) {
    auto const completions = session->federationSynchronized();
    require(
        completions.size() == 1U && completions.front().label == label &&
            completions.front().failedToSyncSet.count(observer.federateHandle()) == 1U,
        "Synchronization service-report completion returned the wrong failure set");
  }
  require(
      reportSink.recorder().interactions().size() == 7U,
      "Synchronization service-report sink received an unexpected report count");

  reportSink.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  subject.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.resign(rti::NO_ACTION);
  subject.resign(rti::NO_ACTION);
  reportSink.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
  reportSink.disconnect();
}

void scenarioServiceReportSynchronizationContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportSynchronization(options, model);
}

void scenarioServiceReportQueryGalt(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty(),
      "Query GALT service-report testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "query-galt-service-report-subject");
  Session regulator(options, model, "query-galt-service-report-regulator");
  Session observer(options, model, "query-galt-service-report-observer");
  auto const federation = federationName(options, "service-report-query-galt");
  subject.connect();
  regulator.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-query-galt-service-report-subject",
      options.federateType,
      federation);
  regulator.join(
      options.memberFederateName + L"-query-galt-service-report-regulator",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-query-galt-service-report-observer",
      options.federateType,
      federation);

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Query GALT report lookup returned an invalid HLAreportServiceInvocation handle");
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
        "Query GALT report lookup returned an invalid standard parameter handle");
    reportParameters.push_back(parameter);
  }

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);

  auto const time = makeTimeContext(regulator);
  auto const expectedGalt = timeAfter(
      *time.factory,
      *time.initial,
      *time.epsilon,
      3U);
  auto lookahead = time.factory->makeZero();
  require(lookahead != nullptr, "Query GALT service-report factory returned no lookahead");
  lookahead->setToDifference(*expectedGalt, *time.initial);
  regulator.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "Query GALT service-report time-regulation callback");

  observer.recorder().clearInteraction();
  subject.rtiAmbassador().setServiceReportingSwitch(true);
  auto actualGalt = time.factory->makeInitial();
  require(actualGalt != nullptr, "Query GALT service-report factory returned no result time");
  require(
      subject.rtiAmbassador().queryGALT(*actualGalt),
      "Query GALT did not report a defined bound for the active regulator");
  require(
      *actualGalt == *expectedGalt,
      "Query GALT returned a value different from the active regulator's current time plus lookahead");

  waitFor(
      observer,
      [&] {
        for (auto const& candidate : observer.recorder().interactions()) {
          auto const service = candidate.parameters.find(reportParameters[0]);
          if (service == candidate.parameters.end()) {
            continue;
          }
          rti::HLAunicodeString name;
          name.decode(service->second);
          if (name.get() == L"QueryGALT") {
            return true;
          }
        }
        return false;
      },
      options,
      "QueryGALT HLAreportServiceInvocation callback");
  auto const reports = observer.recorder().interactions();
  auto const reportIterator = std::find_if(
      reports.begin(),
      reports.end(),
      [&](InteractionRecord const& candidate) {
        auto const service = candidate.parameters.find(reportParameters[0]);
        if (service == candidate.parameters.end()) {
          return false;
        }
        rti::HLAunicodeString name;
        name.decode(service->second);
        return name.get() == L"QueryGALT";
      });
  require(reportIterator != reports.end(), "QueryGALT service report was not recorded");
  auto const& report = *reportIterator;
  require(
      report.interaction == reportClass &&
          report.parameters.size() == reportParameters.size() &&
          report.tag.empty() && !report.regions.has_value() && !report.producer.isValid() &&
          report.transportation ==
              observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable"),
      "QueryGALT service report returned invalid interaction metadata");
  for (auto const parameter : reportParameters) {
    require(
        report.parameters.count(parameter) == 1U,
        "QueryGALT service report omitted a standard parameter");
  }

  rti::HLAinteger16BE serviceType;
  serviceType.decode(report.parameters.at(reportParameters[1]));
  rti::HLAboolean success;
  success.decode(report.parameters.at(reportParameters[2]));
  require(
      serviceType.get() == 4 && success.get(),
      "QueryGALT service report returned the wrong service type or success indicator");
  rti::HLAvariableArray suppliedArguments{rti::HLAfixedRecord{}};
  suppliedArguments.decode(report.parameters.at(reportParameters[3]));
  require(
      suppliedArguments.size() == 0U,
      "QueryGALT service report returned unexpected supplied arguments");

  rti::HLAfixedRecord returnedArgument;
  returnedArgument.appendElement(rti::HLAinteger32BE{})
      .appendElement(rti::HLAunicodeString{})
      .appendElement(rti::HLAunicodeString{});
  returnedArgument.decode(report.parameters.at(reportParameters[4]));
  auto const reportedGalt =
      dynamic_cast<rti::HLAunicodeString const&>(returnedArgument.get(2U)).get();
  auto const expectedText = L"\"" + expectedGalt->toString() + L"\"";
  require(
      dynamic_cast<rti::HLAinteger32BE const&>(returnedArgument.get(0U)).get() == 31 &&
          dynamic_cast<rti::HLAunicodeString const&>(returnedArgument.get(1U)).get() == L"GALT" &&
          reportedGalt == expectedText,
      "QueryGALT service report did not preserve its standard logical-time return argument");
  rti::HLAunicodeString exception;
  exception.decode(report.parameters.at(reportParameters[5]));
  rti::HLAinteger32BE serial;
  serial.decode(report.parameters.at(reportParameters[6]));
  require(
      exception.get().empty() && serial.get() == 0 &&
          observer.rtiAmbassador().decodeFederateHandle(
              report.parameters.at(reportParameters[7])) == subject.federateHandle(),
      "QueryGALT service report returned an exception, wrong serial, or wrong federate");

  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  subject.rtiAmbassador().setServiceReportingSwitch(false);
  regulator.rtiAmbassador().disableTimeRegulation();
  subject.resign(rti::NO_ACTION);
  observer.resign(rti::NO_ACTION);
  regulator.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  regulator.disconnect();
  subject.disconnect();
}


} // namespace hla_rti_cpp_tck::detail
