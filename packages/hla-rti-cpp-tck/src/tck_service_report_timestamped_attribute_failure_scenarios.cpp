#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioServiceReportTimestampedAttributeUpdateFailure(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty(),
      "Timestamped service-report attribute-update failure testing requires an adapter-supplied standard MIM");

  Session publisher(options, model, "service-report-timestamped-attribute-failure-publisher");
  Session receiver(options, model, "service-report-timestamped-attribute-failure-receiver");
  Session observer(options, model, "service-report-timestamped-attribute-failure-observer");
  auto const federation = federationName(
      options,
      "service-report-timestamped-attribute-update-failure");
  publisher.connect();
  receiver.connect();
  observer.connect();
  publisher.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(
      options.ownerFederateName +
          L"-service-report-timestamped-attribute-failure-publisher",
      options.federateType,
      federation);
  receiver.join(
      options.memberFederateName +
          L"-service-report-timestamped-attribute-failure-receiver",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName +
          L"-service-report-timestamped-attribute-failure-observer",
      options.federateType,
      federation);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  auto const receiverClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const receiverAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass,
      options.attributeName);
  require(
      publisherClass.isValid() && publisherAttribute.isValid() && receiverClass.isValid() &&
          receiverAttribute.isValid(),
      "Timestamped service-report attribute-update failure lookup returned an invalid object or attribute handle");

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Timestamped service-report attribute-update failure lookup returned an invalid HLAreportServiceInvocation handle");
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
        "Timestamped service-report attribute-update failure parameter lookup returned an invalid handle");
    reportParameters.push_back(parameter);
  }

  // Keep declarations and registration out of the asserted reports. The
  // three failed timestamped updates below are the only reported services.
  publisher.rtiAmbassador().setServiceReportingSwitch(false);
  publisher.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  receiver.rtiAmbassador().setServiceReportingSwitch(false);
  receiver.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);
  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const receiverAttributes{receiverAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherClass,
      publisherAttributes,
      rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes,
      true,
      L"");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      object.isValid(),
      "Timestamped service-report attribute-update failure registration returned an invalid object");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "timestamped service-report attribute-update failure object discovery");
  auto const reflectionsBeforeFailures = receiver.recorder().reflections().size();

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "timestamped service-report attribute-update failure members selected different logical-time factories");
  auto const lookaheadTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto lookahead = publisherTime.factory->makeZero();
  require(
      lookahead != nullptr,
      "timestamped service-report attribute-update failure could not allocate a lookahead interval");
  lookahead->setToDifference(*lookaheadTime, *publisherTime.initial);
  publisher.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "timestamped service-report attribute-update failure time-regulation callback");

  publisher.rtiAmbassador().setServiceReportingSwitch(true);
  publisher.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  std::vector<std::uint8_t> const valueBytes{0x01U, 0x02U};
  std::vector<std::uint8_t> const tagBytes{0x74U, 0x73U, 0x6FU};
  rti::VariableLengthData const value(valueBytes.data(), valueBytes.size());
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  rti::AttributeHandleValueMap validValues;
  validValues.emplace(publisherAttribute, value);
  rti::AttributeHandleValueMap invalidValues;
  invalidValues.emplace(rti::AttributeHandle{}, value);
  auto const validTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      6U);
  auto const invalidTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      4U);

  auto const validMapValue = L"{\"" + publisherAttribute.toString() + L"\":\"AQI=\"}";
  auto const invalidMapValue =
      L"{\"" + rti::AttributeHandle{}.toString() + L"\":\"AQI=\"}";
  auto const verifyFailureReport = [&](std::int32_t expectedSerial,
                                       std::wstring const& objectValue,
                                       std::wstring const& mapValue,
                                       rti::LogicalTime const& timestamp,
                                       std::wstring const& expectedException,
                                       std::string const& description) {
    waitFor(
        observer,
        [&] { return observer.recorder().interaction().present; },
        options,
        description + " HLAreportServiceInvocation callback");
    auto const report = observer.recorder().interaction();
    require(
        report.interaction == reportClass &&
            report.parameters.size() == reportParameters.size() &&
            !report.producer.isValid(),
        description + " report returned the wrong interaction identity or producer metadata");
    require(
        report.tag.empty() && !report.regions.has_value(),
        description + " report returned user tag or region metadata");
    require(
        report.transportation ==
            observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable"),
        description + " report returned the wrong transportation type");
    for (auto const parameterHandle : reportParameters) {
      require(
          report.parameters.count(parameterHandle) == 1U,
          description + " report omitted a standard report parameter");
    }

    rti::HLAunicodeString service;
    service.decode(report.parameters.at(reportParameters[0]));
    require(
        service.get() == L"UpdateAttributeValues",
        description + " report returned the wrong service name");
    rti::HLAinteger16BE serviceType;
    serviceType.decode(report.parameters.at(reportParameters[1]));
    require(
        serviceType.get() == 2,
        description + " report returned the wrong object-management service type");
    rti::HLAboolean success;
    success.decode(report.parameters.at(reportParameters[2]));
    require(!success.get(), description + " report incorrectly reported success");

    rti::HLAfixedRecord suppliedArgumentPrototype;
    suppliedArgumentPrototype.appendElement(rti::HLAinteger32BE{})
        .appendElement(rti::HLAunicodeString{})
        .appendElement(rti::HLAunicodeString{});
    rti::HLAvariableArray suppliedArguments{suppliedArgumentPrototype};
    suppliedArguments.decode(report.parameters.at(reportParameters[3]));
    require(
        suppliedArguments.size() == 4U,
        description + " report omitted standard supplied arguments");
    auto const verifyArgument = [&](std::size_t index,
                                    std::int32_t type,
                                    std::wstring const& name,
                                    std::wstring const& expectedValue) {
      auto const& supplied = dynamic_cast<rti::HLAfixedRecord const&>(
          suppliedArguments.get(index));
      require(
          dynamic_cast<rti::HLAinteger32BE const&>(supplied.get(0U)).get() == type &&
              dynamic_cast<rti::HLAunicodeString const&>(supplied.get(1U)).get() == name &&
              dynamic_cast<rti::HLAunicodeString const&>(supplied.get(2U)).get() ==
                  expectedValue,
          description + " report returned the wrong supplied-argument shape");
    };
    verifyArgument(
        0U,
        37,
        L"Object instance designator",
        L"\"" + objectValue + L"\"");
    verifyArgument(
        1U,
        2,
        L"Constrained set of attribute designator and value pairs",
        mapValue);
    verifyArgument(2U, 60, L"User-supplied tag", L"\"dHNv\"");
    verifyArgument(
        3U,
        31,
        L"Optional timestamp",
        L"\"" + timestamp.toString() + L"\"");

    rti::HLAfixedRecord returnedArgument;
    returnedArgument.appendElement(rti::HLAinteger32BE{})
        .appendElement(rti::HLAunicodeString{})
        .appendElement(rti::HLAunicodeString{});
    returnedArgument.decode(report.parameters.at(reportParameters[4]));
    require(
        dynamic_cast<rti::HLAinteger32BE const&>(returnedArgument.get(0U)).get() == 34 &&
            dynamic_cast<rti::HLAunicodeString const&>(returnedArgument.get(1U)).get().empty() &&
            dynamic_cast<rti::HLAunicodeString const&>(returnedArgument.get(2U)).get() ==
                L"null",
        description + " report returned the wrong Null return value");
    rti::HLAunicodeString exception;
    exception.decode(report.parameters.at(reportParameters[5]));
    require(
        exception.get().find(expectedException) != std::wstring::npos,
        description + " report returned the wrong exception name");
    rti::HLAinteger32BE serial;
    serial.decode(report.parameters.at(reportParameters[6]));
    require(
        serial.get() == expectedSerial,
        description + " report returned the wrong serial number");
    observer.recorder().clearInteraction();
  };

  rti::ObjectInstanceHandle const invalidObject;
  requireException(
      [&] {
        publisher.rtiAmbassador().updateAttributeValues(
            invalidObject,
            validValues,
            tag,
            *validTime);
      },
      L"ObjectInstanceNotKnown",
      "timestamped update with an unknown object and service reporting enabled");
  verifyFailureReport(
      0,
      invalidObject.toString(),
      validMapValue,
      *validTime,
      L"ObjectInstanceNotKnown",
      "unknown-object timestamped attribute update");

  requireException(
      [&] {
        publisher.rtiAmbassador().updateAttributeValues(
            object,
            invalidValues,
            tag,
            *validTime);
      },
      L"AttributeNotDefined",
      "timestamped update with an undefined attribute and service reporting enabled");
  verifyFailureReport(
      1,
      object.toString(),
      invalidMapValue,
      *validTime,
      L"AttributeNotDefined",
      "undefined-attribute timestamped update");

  requireException(
      [&] {
        publisher.rtiAmbassador().updateAttributeValues(
            object,
            validValues,
            tag,
            *invalidTime);
      },
      L"InvalidLogicalTime",
      "timestamped update earlier than current time plus lookahead");
  verifyFailureReport(
      2,
      object.toString(),
      validMapValue,
      *invalidTime,
      L"InvalidLogicalTime",
      "invalid-time timestamped attribute update");

  for (int pass = 0; pass != 8; ++pass) {
    receiver.pump();
  }
  require(
      receiver.recorder().reflections().size() == reflectionsBeforeFailures,
      "failed timestamped attribute updates delivered an ordinary reflection callback");

  publisher.rtiAmbassador().setServiceReportingSwitch(false);
  publisher.rtiAmbassador().disableTimeRegulation();
  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  observer.resign(rti::NO_ACTION);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioServiceReportTimestampedInteractionFailure(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty(),
      "Timestamped service-report interaction failure testing requires an adapter-supplied standard MIM");

  Session publisher(options, model, "service-report-timestamped-interaction-failure-publisher");
  Session receiver(options, model, "service-report-timestamped-interaction-failure-receiver");
  Session observer(options, model, "service-report-timestamped-interaction-failure-observer");
  auto const federation = federationName(
      options,
      "service-report-timestamped-interaction-failure");
  publisher.connect();
  receiver.connect();
  observer.connect();
  publisher.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(
      options.ownerFederateName +
          L"-service-report-timestamped-interaction-failure-publisher",
      options.federateType,
      federation);
  receiver.join(
      options.memberFederateName +
          L"-service-report-timestamped-interaction-failure-receiver",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName +
          L"-service-report-timestamped-interaction-failure-observer",
      options.federateType,
      federation);

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
      "Timestamped service-report interaction failure source lookup returned an invalid handle");

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Timestamped service-report interaction failure lookup returned an invalid HLAreportServiceInvocation handle");
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
        "Timestamped service-report interaction failure parameter lookup returned an invalid handle");
    reportParameters.push_back(parameter);
  }

  // Keep declarations and time-role setup out of the asserted reports. The
  // three failed timestamped sends below are the only reported services.
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

  auto publisherTime = makeTimeContext(publisher);
  auto const lookaheadTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto lookahead = publisherTime.factory->makeZero();
  require(
      lookahead != nullptr,
      "timestamped service-report interaction failure could not allocate a lookahead interval");
  lookahead->setToDifference(*lookaheadTime, *publisherTime.initial);
  publisher.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "timestamped service-report interaction failure time-regulation callback");

  publisher.rtiAmbassador().setServiceReportingSwitch(true);
  publisher.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  std::vector<std::uint8_t> const valueBytes{0x01U};
  std::vector<std::uint8_t> const tagBytes{0x74U, 0x73U, 0x6FU};
  rti::VariableLengthData const value(valueBytes.data(), valueBytes.size());
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  rti::ParameterHandleValueMap validValues;
  validValues.emplace(publisherParameter, value);
  rti::ParameterHandleValueMap invalidValues;
  invalidValues.emplace(rti::ParameterHandle{}, value);
  auto const validTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      6U);
  auto const invalidTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      4U);
  auto const validMapValue =
      L"{\"" + publisherParameter.toString() + L"\":\"AQ==\"}";
  auto const invalidMapValue =
      L"{\"" + rti::ParameterHandle{}.toString() + L"\":\"AQ==\"}";

  auto const verifyFailureReport = [&](std::int32_t expectedSerial,
                                       std::wstring const& interactionValue,
                                       std::wstring const& mapValue,
                                       rti::LogicalTime const& timestamp,
                                       std::wstring const& expectedException,
                                       std::string const& description) {
    waitFor(
        observer,
        [&] { return observer.recorder().interaction().present; },
        options,
        description + " HLAreportServiceInvocation callback");
    auto const report = observer.recorder().interaction();
    require(
        report.interaction == reportClass &&
            report.parameters.size() == reportParameters.size() &&
            !report.producer.isValid(),
        description + " report returned the wrong interaction identity or producer metadata");
    require(
        report.tag.empty() && !report.regions.has_value(),
        description + " report returned user tag or region metadata");
    require(
        report.transportation ==
            observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable"),
        description + " report returned the wrong transportation type");
    for (auto const parameterHandle : reportParameters) {
      require(
          report.parameters.count(parameterHandle) == 1U,
          description + " report omitted a standard report parameter");
    }

    rti::HLAunicodeString service;
    service.decode(report.parameters.at(reportParameters[0]));
    require(
        service.get() == L"SendInteraction",
        description + " report returned the wrong service name");
    rti::HLAinteger16BE serviceType;
    serviceType.decode(report.parameters.at(reportParameters[1]));
    require(
        serviceType.get() == 2,
        description + " report returned the wrong interaction service type");
    rti::HLAboolean success;
    success.decode(report.parameters.at(reportParameters[2]));
    require(!success.get(), description + " report incorrectly reported success");

    rti::HLAfixedRecord suppliedArgumentPrototype;
    suppliedArgumentPrototype.appendElement(rti::HLAinteger32BE{})
        .appendElement(rti::HLAunicodeString{})
        .appendElement(rti::HLAunicodeString{});
    rti::HLAvariableArray suppliedArguments{suppliedArgumentPrototype};
    suppliedArguments.decode(report.parameters.at(reportParameters[3]));
    require(
        suppliedArguments.size() == 4U,
        description + " report omitted standard supplied arguments");
    auto const verifyArgument = [&](std::size_t index,
                                    std::int32_t type,
                                    std::wstring const& name,
                                    std::wstring const& expectedValue) {
      auto const& supplied = dynamic_cast<rti::HLAfixedRecord const&>(
          suppliedArguments.get(index));
      require(
          dynamic_cast<rti::HLAinteger32BE const&>(supplied.get(0U)).get() == type &&
              dynamic_cast<rti::HLAunicodeString const&>(supplied.get(1U)).get() == name &&
              dynamic_cast<rti::HLAunicodeString const&>(supplied.get(2U)).get() ==
                  expectedValue,
          description + " report returned the wrong supplied-argument shape");
    };
    verifyArgument(
        0U,
        27,
        L"Interaction class designator",
        L"\"" + interactionValue + L"\"");
    verifyArgument(
        1U,
        40,
        L"Constrained set of interaction parameter designator and value pairs",
        mapValue);
    verifyArgument(2U, 60, L"User-supplied tag", L"\"dHNv\"");
    verifyArgument(
        3U,
        31,
        L"Optional timestamp",
        L"\"" + timestamp.toString() + L"\"");

    rti::HLAfixedRecord returnedArgument;
    returnedArgument.appendElement(rti::HLAinteger32BE{})
        .appendElement(rti::HLAunicodeString{})
        .appendElement(rti::HLAunicodeString{});
    returnedArgument.decode(report.parameters.at(reportParameters[4]));
    require(
        dynamic_cast<rti::HLAinteger32BE const&>(returnedArgument.get(0U)).get() == 34 &&
            dynamic_cast<rti::HLAunicodeString const&>(returnedArgument.get(1U)).get().empty() &&
            dynamic_cast<rti::HLAunicodeString const&>(returnedArgument.get(2U)).get() ==
                L"null",
        description + " report returned the wrong Null return value");
    rti::HLAunicodeString exception;
    exception.decode(report.parameters.at(reportParameters[5]));
    require(
        exception.get().find(expectedException) != std::wstring::npos,
        description + " report returned the wrong exception name");
    rti::HLAinteger32BE serial;
    serial.decode(report.parameters.at(reportParameters[6]));
    require(
        serial.get() == expectedSerial,
        description + " report returned the wrong serial number");
    observer.recorder().clearInteraction();
  };

  auto const pumpReceiverWithoutApplicationCallback = [&](std::string const& description) {
    receiver.recorder().clearInteraction();
    for (int pass = 0; pass != 8; ++pass) {
      receiver.pump();
    }
    require(
        !receiver.recorder().interaction().present,
        description + " delivered an application callback");
  };

  rti::InteractionClassHandle const invalidInteraction;
  requireException(
      [&] {
        publisher.rtiAmbassador().sendInteraction(
            invalidInteraction,
            validValues,
            tag,
            *validTime);
      },
      L"InteractionClassNotDefined",
      "timestamped send with an invalid interaction class and service reporting enabled");
  verifyFailureReport(
      0,
      invalidInteraction.toString(),
      validMapValue,
      *validTime,
      L"InteractionClassNotDefined",
      "invalid interaction-class timestamped send");
  pumpReceiverWithoutApplicationCallback("invalid interaction-class timestamped send");

  requireException(
      [&] {
        publisher.rtiAmbassador().sendInteraction(
            publisherInteraction,
            invalidValues,
            tag,
            *validTime);
      },
      L"InteractionParameterNotDefined",
      "timestamped send with an invalid parameter and service reporting enabled");
  verifyFailureReport(
      1,
      publisherInteraction.toString(),
      invalidMapValue,
      *validTime,
      L"InteractionParameterNotDefined",
      "invalid-parameter timestamped send");
  pumpReceiverWithoutApplicationCallback("invalid-parameter timestamped send");

  requireException(
      [&] {
        publisher.rtiAmbassador().sendInteraction(
            publisherInteraction,
            validValues,
            tag,
            *invalidTime);
      },
      L"InvalidLogicalTime",
      "timestamped send earlier than current time plus lookahead");
  verifyFailureReport(
      2,
      publisherInteraction.toString(),
      validMapValue,
      *invalidTime,
      L"InvalidLogicalTime",
      "invalid-time timestamped send");
  pumpReceiverWithoutApplicationCallback("invalid-time timestamped send");

  publisher.rtiAmbassador().setServiceReportingSwitch(false);
  publisher.rtiAmbassador().disableTimeRegulation();
  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  publisher.rtiAmbassador().unpublishInteractionClass(publisherInteraction);
  observer.resign(rti::NO_ACTION);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  receiver.disconnect();
  publisher.disconnect();
}


} // namespace hla_rti_cpp_tck::detail
