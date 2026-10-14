#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioServiceReportTimestampedDeleteObjectInstanceFailure(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty() && !options.logicalTimeImplementationName.empty(),
      "Timestamped delete-object failure service-report testing requires adapter-supplied standard MIM and logical-time configuration");

  Session publisher(
      options,
      model,
      "service-report-timestamped-delete-failure-publisher");
  Session receiver(
      options,
      model,
      "service-report-timestamped-delete-failure-receiver");
  Session observer(
      options,
      model,
      "service-report-timestamped-delete-failure-observer");
  auto const federation = federationName(
      options,
      "service-report-timestamped-delete-object-instance-failure");
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
          L"-service-report-timestamped-delete-failure-publisher",
      options.federateType,
      federation);
  receiver.join(
      options.memberFederateName +
          L"-service-report-timestamped-delete-failure-receiver",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName +
          L"-service-report-timestamped-delete-failure-observer",
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
      publisherClass.isValid() && publisherAttribute.isValid() &&
          receiverClass.isValid() && receiverAttribute.isValid(),
      "Timestamped delete-object failure service-report lookup returned an invalid object or attribute handle");

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Timestamped delete-object failure service-report lookup returned an invalid HLAreportServiceInvocation handle");
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
    auto const parameter = observer.rtiAmbassador().getParameterHandle(
        reportClass,
        name);
    require(
        parameter.isValid(),
        "Timestamped delete-object failure service-report parameter lookup returned an invalid handle");
    reportParameters.push_back(parameter);
  }

  // Keep declarations, registration, and time-role setup out of the asserted
  // reports. The two failed timestamped deletions below are the only reported
  // service invocations.
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
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes,
      true,
      L"");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      object.isValid(),
      "Timestamped delete-object failure service-report registration returned an invalid object");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "timestamped delete-object failure service-report object discovery");
  receiver.recorder().clearRemovals();

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "Timestamped delete-object failure service-report members selected different logical-time factories");
  auto const lookaheadTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto lookahead = publisherTime.factory->makeZero();
  require(
      lookahead != nullptr,
      "Timestamped delete-object failure service-report could not allocate a lookahead interval");
  lookahead->setToDifference(*lookaheadTime, *publisherTime.initial);
  publisher.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "timestamped delete-object failure service-report time-regulation callback");

  publisher.rtiAmbassador().setServiceReportingSwitch(true);
  publisher.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  std::vector<std::uint8_t> const tagBytes{0x74U, 0x73U, 0x6FU};
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
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

  auto const verifyFailureReport = [&](std::int32_t expectedSerial,
                                       std::wstring const& objectValue,
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
        service.get() == L"DeleteObjectInstance",
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
        suppliedArguments.size() == 3U,
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
    verifyArgument(1U, 60, L"User-supplied tag", L"\"dHNv\"");
    verifyArgument(
        2U,
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

  auto const requireNoApplicationRemoval = [&](std::string const& description) {
    for (int pass = 0; pass != 8; ++pass) {
      receiver.pump();
    }
    require(
        receiver.recorder().removals().empty() &&
            receiver.recorder().timedRemovals().empty(),
        description + " delivered an application removal callback");
  };

  rti::ObjectInstanceHandle const invalidObject;
  requireException(
      [&] {
        publisher.rtiAmbassador().deleteObjectInstance(
            invalidObject,
            tag,
            *validTime);
      },
      L"ObjectInstanceNotKnown",
      "timestamped DeleteObjectInstance with an unknown object");
  verifyFailureReport(
      0,
      invalidObject.toString(),
      *validTime,
      L"ObjectInstanceNotKnown",
      "unknown-object timestamped deletion");
  requireNoApplicationRemoval("unknown-object timestamped deletion");

  requireException(
      [&] {
        publisher.rtiAmbassador().deleteObjectInstance(
            object,
            tag,
            *invalidTime);
      },
      L"InvalidLogicalTime",
      "timestamped DeleteObjectInstance before current time plus lookahead");
  verifyFailureReport(
      1,
      object.toString(),
      *invalidTime,
      L"InvalidLogicalTime",
      "invalid-logical-time timestamped deletion");
  requireNoApplicationRemoval("invalid-logical-time timestamped deletion");

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


} // namespace hla_rti_cpp_tck::detail
