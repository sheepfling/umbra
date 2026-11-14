#include "tck_main_scenarios.hpp"
#include "tck_service_report_file_support.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioServiceReportOrderTransportationLookups(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty(),
      "Order/transportation service-report testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "service-report-order-transportation-subject");
  Session observer(options, model, "service-report-order-transportation-observer");
  auto const federation = federationName(options, "service-report-order-transportation-lookups");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-service-report-order-transportation-subject",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-order-transportation-observer",
      options.federateType,
      federation);

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Order/transportation service-report lookup returned an invalid report class handle");
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
      auto const parameter = observer.rtiAmbassador().getParameterHandle(reportClass, name);
      require(
          parameter.isValid(),
          "Order/transportation service-report lookup returned an invalid parameter handle");
      result.push_back(parameter);
    }
    return result;
  }();
  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);

  auto const reliable = subject.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable");
  require(reliable.isValid(), "Order/transportation lookup could not resolve HLAreliable");
  require(
      subject.rtiAmbassador().getOrderType(L"Receive") == rti::RECEIVE,
      "Order/transportation lookup did not resolve the standard Receive order");

  subject.rtiAmbassador().setServiceReportingSwitch(true);
  auto const timestampOrder = subject.rtiAmbassador().getOrderType(L"TimeStamp");
  auto const receiveOrderName = subject.rtiAmbassador().getOrderName(rti::RECEIVE);
  auto const reliableByName = subject.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable");
  auto const reliableName = subject.rtiAmbassador().getTransportationTypeName(reliable);
  auto const timeFactory = subject.rtiAmbassador().getTimeFactory();
  std::wstring const invalidOrderName = L"UnknownOrderName";
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getOrderType(invalidOrderName));
      },
      L"InvalidOrderName",
      "GetOrderType with an unknown order name");
  std::wstring const invalidTransportationName = L"UnknownTransportationName";
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getTransportationTypeHandle(
            invalidTransportationName));
      },
      L"InvalidTransportationName",
      "GetTransportationTypeHandle with an unknown transportation name");
  require(
      timestampOrder == rti::TIMESTAMP && receiveOrderName == L"Receive" &&
          reliableByName == reliable && reliableName == L"HLAreliable" &&
          timeFactory != nullptr &&
          timeFactory->getName() == options.logicalTimeImplementationName,
      "Order/transportation lookup returned the wrong standard value");
  waitFor(
      subject,
      observer,
      [&] { return observer.recorder().interactions().size() == 7U; },
      options,
      "order/transportation service-report callbacks");

  auto const quoted = [](std::wstring const& value) {
    return std::wstring{L"\""} + value + L"\"";
  };
  struct ExpectedArgument final {
    rti::Integer32 type;
    std::wstring name;
    std::wstring value;
  };
  auto const decodeArgument = [](rti::HLAfixedRecord const& record,
                                 ExpectedArgument const& expected) {
    require(
        dynamic_cast<rti::HLAinteger32BE const&>(record.get(0U)).get() == expected.type,
        "Order/transportation service report returned the wrong argument type");
    if (!expected.name.empty()) {
      require(
          dynamic_cast<rti::HLAunicodeString const&>(record.get(1U)).get() == expected.name,
          "Order/transportation service report returned the wrong argument name");
    }
    require(
        dynamic_cast<rti::HLAunicodeString const&>(record.get(2U)).get() == expected.value,
        "Order/transportation service report returned the wrong argument value");
  };
  auto const decodeReport = [&](InteractionRecord const& report,
                                std::wstring const& service,
                                std::vector<ExpectedArgument> const& expectedSupplied,
                                ExpectedArgument const& expectedReturned,
                                std::int32_t expectedSerial) {
    require(
        report.interaction == reportClass &&
            report.parameters.size() == reportParameters.size() && report.tag.empty() &&
            !report.producer.isValid() && !report.regions.has_value() &&
            report.transportation == observer.rtiAmbassador().getTransportationTypeHandle(
                L"HLAreliable"),
        "Order/transportation service report returned non-standard metadata");
    for (auto const& parameter : reportParameters) {
      require(
          report.parameters.count(parameter) == 1U,
          "Order/transportation service report omitted a standard parameter");
    }
    rti::HLAunicodeString decodedService;
    decodedService.decode(report.parameters.at(reportParameters.at(0U)));
    require(
        decodedService.get() == service,
        "Order/transportation service report returned the wrong service name");
    rti::HLAinteger16BE decodedServiceType;
    decodedServiceType.decode(report.parameters.at(reportParameters.at(1U)));
    require(
        decodedServiceType.get() == 6,
        "Order/transportation service report returned the wrong service type");
    rti::HLAboolean decodedSuccess;
    decodedSuccess.decode(report.parameters.at(reportParameters.at(2U)));
    require(
        decodedSuccess.get(),
        "Order/transportation service report was not marked successful");

    rti::HLAfixedRecord argumentPrototype;
    argumentPrototype.appendElement(rti::HLAinteger32BE{})
        .appendElement(rti::HLAunicodeString{})
        .appendElement(rti::HLAunicodeString{});
    rti::HLAvariableArray suppliedArguments{argumentPrototype};
    suppliedArguments.decode(report.parameters.at(reportParameters.at(3U)));
    require(
        suppliedArguments.size() == expectedSupplied.size(),
        "Order/transportation service report returned the wrong supplied-argument count");
    for (std::size_t index = 0U; index != expectedSupplied.size(); ++index) {
      auto const& supplied = dynamic_cast<rti::HLAfixedRecord const&>(
          suppliedArguments.get(index));
      decodeArgument(supplied, expectedSupplied.at(index));
    }

    rti::HLAfixedRecord returnedArgument;
    returnedArgument.appendElement(rti::HLAinteger32BE{})
        .appendElement(rti::HLAunicodeString{})
        .appendElement(rti::HLAunicodeString{});
    returnedArgument.decode(report.parameters.at(reportParameters.at(4U)));
    decodeArgument(returnedArgument, expectedReturned);
    rti::HLAunicodeString decodedException;
    decodedException.decode(report.parameters.at(reportParameters.at(5U)));
    require(
        decodedException.get().empty(),
        "Successful order/transportation service report carried an exception");
    rti::HLAinteger32BE decodedSerial;
    decodedSerial.decode(report.parameters.at(reportParameters.at(6U)));
    require(
        decodedSerial.get() == expectedSerial,
        "Order/transportation service report returned the wrong serial number");
  };

  auto const reports = observer.recorder().interactions();
  require(reports.size() == 7U, "Support-service lookup report count was unstable");
  decodeReport(
      reports.at(0U),
      L"GetOrderType",
      {{53, L"Order name", quoted(L"TimeStamp")}},
      {38, L"Order type", quoted(L"TIMESTAMP")},
      0);
  decodeReport(
      reports.at(1U),
      L"GetOrderName",
      {{38, L"Order type", quoted(L"RECEIVE")}},
      {53, L"Order name", quoted(receiveOrderName)},
      1);
  decodeReport(
      reports.at(2U),
      L"GetTransportationTypeHandle",
      {{53, L"Transportation type name", quoted(L"HLAreliable")}},
      {59, L"Transportation type handle", quoted(reliableByName.toString())},
      2);
  decodeReport(
      reports.at(3U),
      L"GetTransportationTypeName",
      {{59, L"Transportation type handle", quoted(reliable.toString())}},
      {53, L"Transportation type name", quoted(reliableName)},
      3);
  decodeReport(
      reports.at(4U),
      L"GetTimeFactory",
      {},
      {12, L"", quoted(L"LOGICAL_TIME_FACTORY")},
      4);
  requirePortableServiceReportFailure(
      observer,
      reports.at(5U),
      reportClass,
      reportParameters,
      L"GetOrderType",
      {{53, L"Order name", quoted(invalidOrderName)}},
      L"InvalidOrderName",
      5,
      "GetOrderType unknown-name service report");
  requirePortableServiceReportFailure(
      observer,
      reports.at(6U),
      reportClass,
      reportParameters,
      L"GetTransportationTypeHandle",
      {{53, L"Transportation type name", quoted(invalidTransportationName)}},
      L"InvalidTransportationName",
      6,
      "GetTransportationTypeHandle unknown-name service report");
  for (auto const& report : reports) {
    require(
        observer.rtiAmbassador().decodeFederateHandle(
            report.parameters.at(reportParameters.at(7U))) == subject.federateHandle(),
        "Order/transportation service report identified the wrong federate");
  }

  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  subject.rtiAmbassador().setServiceReportingSwitch(false);
  observer.resign(rti::NO_ACTION);
  subject.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioServiceReportOrderTransportationLookupsContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportOrderTransportationLookups(options, model);
}

void scenarioServiceReportDimensionUpperBound(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Dimension upper-bound service-report testing requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "Dimension upper-bound service-report testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "service-report-dimension-upper-bound-subject");
  Session observer(options, model, "service-report-dimension-upper-bound-observer");
  auto const federation =
      federationName(options, "service-report-dimension-upper-bound");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.modelFom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-service-report-dimension-upper-bound-subject",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-dimension-upper-bound-observer",
      options.federateType,
      federation);

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Dimension upper-bound service-report lookup returned an invalid report class handle");
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
      auto const parameter = observer.rtiAmbassador().getParameterHandle(
          reportClass,
          name);
      require(
          parameter.isValid(),
          "Dimension upper-bound service-report lookup returned an invalid parameter handle");
      result.push_back(parameter);
    }
    return result;
  }();

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);

  auto const dimension = subject.rtiAmbassador().getDimensionHandle(
      options.fomDimensionName);
  require(
      dimension.isValid(),
      "Dimension upper-bound service-report lookup returned an invalid dimension handle");
  require(
      !subject.rtiAmbassador().getSendServiceReportsToFileSwitch(),
      "Dimension upper-bound service-report test could not select MOM delivery");
  subject.rtiAmbassador().setServiceReportingSwitch(true);
  auto const upperBound = subject.rtiAmbassador().getDimensionUpperBound(dimension);
  require(
      upperBound == 64UL,
      "Dimension upper-bound lookup did not return the adapter FOM's bound of 64");
  auto const quoted = [](std::wstring const& value) {
    return std::wstring{L"\""} + value + L"\"";
  };
  rti::DimensionHandle const invalidDimension;
  requireException(
      [&] {
        static_cast<void>(
            subject.rtiAmbassador().getDimensionUpperBound(invalidDimension));
      },
      L"InvalidDimensionHandle",
      "GetDimensionUpperBound with an invalid dimension handle");

  waitFor(
      subject,
      observer,
      [&] { return observer.recorder().interactions().size() >= 2U; },
      options,
      "GetDimensionUpperBound success and failure service-report callbacks");
  auto const reports = observer.recorder().interactions();
  require(
      reports.size() == 2U,
      "GetDimensionUpperBound delivered an unexpected number of service reports");
  rti::HLAfixedRecord reportedUpperBoundArgument;
  reportedUpperBoundArgument
      .appendElement(rti::HLAinteger32BE{})
      .appendElement(rti::HLAunicodeString{})
      .appendElement(rti::HLAunicodeString{});
  reportedUpperBoundArgument.decode(
      reports.front().parameters.at(reportParameters.at(4U)));
  auto const reportedUpperBoundText = dynamic_cast<rti::HLAunicodeString const&>(
      reportedUpperBoundArgument.get(2U)).get();
  std::string reportedUpperBoundAscii;
  for (wchar_t const character : reportedUpperBoundText) {
    reportedUpperBoundAscii.push_back(
        character >= L' ' && character <= L'~'
            ? static_cast<char>(character)
            : '?');
  }
  require(
      reportedUpperBoundText == std::to_wstring(upperBound),
      "GetDimensionUpperBound service report encoded return value `" +
          reportedUpperBoundAscii + "` instead of `" +
          std::to_string(upperBound) + "`");
  requirePortableServiceReport(
      observer,
      reports.front(),
      reportClass,
      reportParameters,
      L"GetDimensionUpperBound",
      {{10,
        L"Dimension handle",
        quoted(dimension.toString())}},
      {35, L"Dimension upper bound", std::to_wstring(upperBound)},
      0,
      "GetDimensionUpperBound service report");
  require(
      observer.rtiAmbassador().decodeFederateHandle(
          reports.front().parameters.at(reportParameters.at(7U))) ==
          subject.federateHandle(),
      "GetDimensionUpperBound service report identified the wrong federate");
  requirePortableServiceReportFailure(
      observer,
      reports.at(1U),
      reportClass,
      reportParameters,
      L"GetDimensionUpperBound",
      {{10, L"Dimension handle", quoted(invalidDimension.toString())}},
      L"InvalidDimensionHandle",
      1,
      "GetDimensionUpperBound invalid-handle service report");
  require(
      observer.rtiAmbassador().decodeFederateHandle(
          reports.at(1U).parameters.at(reportParameters.at(7U))) ==
          subject.federateHandle(),
      "GetDimensionUpperBound invalid-handle report identified the wrong federate");

  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.resign(rti::NO_ACTION);
  observer.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioServiceReportDimensionUpperBoundContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportDimensionUpperBound(options, model);
}

void scenarioServiceReportDimensionName(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Dimension-name service-report testing requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "Dimension-name service-report testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "service-report-dimension-name-subject");
  Session observer(options, model, "service-report-dimension-name-observer");
  auto const federation = federationName(options, "service-report-dimension-name");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.modelFom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-service-report-dimension-name-subject",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-dimension-name-observer",
      options.federateType,
      federation);

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Dimension-name service-report lookup returned an invalid report class handle");
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
      auto const parameter = observer.rtiAmbassador().getParameterHandle(
          reportClass,
          name);
      require(
          parameter.isValid(),
          "Dimension-name service-report lookup returned an invalid parameter handle");
      result.push_back(parameter);
    }
    return result;
  }();

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);

  auto const dimension = subject.rtiAmbassador().getDimensionHandle(
      options.fomDimensionName);
  require(
      dimension.isValid(),
      "Dimension-name service-report lookup returned an invalid dimension handle");
  auto const quoted = [](std::wstring const& value) {
    return std::wstring{L"\""} + value + L"\"";
  };
  auto const dimensionName = subject.rtiAmbassador().getDimensionName(dimension);
  require(
      dimensionName == options.fomDimensionName,
      "Dimension-name lookup did not return the adapter FOM's dimension name");
  require(
      !subject.rtiAmbassador().getSendServiceReportsToFileSwitch(),
      "Dimension-name service-report test could not select MOM delivery");
  subject.rtiAmbassador().setServiceReportingSwitch(true);
  require(
      subject.rtiAmbassador().getDimensionName(dimension) == dimensionName,
      "Reported dimension-name lookup returned a different name");

  waitFor(
      subject,
      observer,
      [&] { return observer.recorder().interactions().size() >= 1U; },
      options,
      "successful GetDimensionName service-report callback");
  auto const reports = observer.recorder().interactions();
  require(
      reports.size() == 1U,
      "GetDimensionName delivered an unexpected number of service reports");
  requirePortableServiceReport(
      observer,
      reports.front(),
      reportClass,
      reportParameters,
      L"GetDimensionName",
      {{10,
        L"Dimension handle",
        quoted(dimension.toString())}},
      {53,
       L"Dimension name",
       quoted(dimensionName)},
      0,
      "GetDimensionName service report");
  require(
      observer.rtiAmbassador().decodeFederateHandle(
          reports.front().parameters.at(reportParameters.at(7U))) ==
          subject.federateHandle(),
      "GetDimensionName service report identified the wrong federate");

  rti::DimensionHandle const invalidDimension;
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getDimensionName(invalidDimension));
      },
      L"InvalidDimensionHandle",
      "GetDimensionName with an invalid dimension handle");
  waitFor(
      subject,
      observer,
      [&] { return observer.recorder().interactions().size() >= 2U; },
      options,
      "failed GetDimensionName service-report callback");
  auto const reportsAfterFailure = observer.recorder().interactions();
  require(
      reportsAfterFailure.size() == 2U,
      "GetDimensionName delivered an unexpected number of success and failure reports");
  requirePortableServiceReportFailure(
      observer,
      reportsAfterFailure.at(1U),
      reportClass,
      reportParameters,
      L"GetDimensionName",
      {{10, L"Dimension handle", quoted(invalidDimension.toString())}},
      L"InvalidDimensionHandle",
      1,
      "GetDimensionName invalid-handle service report");
  require(
      observer.rtiAmbassador().decodeFederateHandle(
          reportsAfterFailure.at(1U).parameters.at(reportParameters.at(7U))) ==
          subject.federateHandle(),
      "GetDimensionName invalid-handle service report identified the wrong federate");

  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.resign(rti::NO_ACTION);
  observer.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioServiceReportDimensionNameContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportDimensionName(options, model);
}

void scenarioServiceReportDimensionHandle(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Dimension-handle service-report testing requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "Dimension-handle service-report testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "service-report-dimension-handle-subject");
  Session observer(options, model, "service-report-dimension-handle-observer");
  auto const federation = federationName(options, "service-report-dimension-handle");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.modelFom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-service-report-dimension-handle-subject",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-dimension-handle-observer",
      options.federateType,
      federation);

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Dimension-handle service-report lookup returned an invalid report class handle");
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
      auto const parameter = observer.rtiAmbassador().getParameterHandle(
          reportClass,
          name);
      require(
          parameter.isValid(),
          "Dimension-handle service-report lookup returned an invalid parameter handle");
      result.push_back(parameter);
    }
    return result;
  }();

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);
  require(
      !subject.rtiAmbassador().getSendServiceReportsToFileSwitch(),
      "Dimension-handle service-report test could not select MOM delivery");
  subject.rtiAmbassador().setServiceReportingSwitch(true);
  auto const quoted = [](std::wstring const& value) {
    return std::wstring{L"\""} + value + L"\"";
  };

  auto const dimension = subject.rtiAmbassador().getDimensionHandle(
      options.fomDimensionName);
  require(
      dimension.isValid(),
      "Adapter FOM dimension-name lookup returned an invalid dimension handle");
  waitFor(
      subject,
      observer,
      [&] { return observer.recorder().interactions().size() >= 1U; },
      options,
      "successful GetDimensionHandle service-report callback");
  auto const reports = observer.recorder().interactions();
  require(
      reports.size() == 1U,
      "GetDimensionHandle delivered an unexpected number of service reports");
  requirePortableServiceReport(
      observer,
      reports.front(),
      reportClass,
      reportParameters,
      L"GetDimensionHandle",
      {{53,
        L"Dimension name",
        quoted(options.fomDimensionName)}},
      {10,
       L"Dimension handle",
       quoted(dimension.toString())},
      0,
      "GetDimensionHandle service report");
  require(
      observer.rtiAmbassador().decodeFederateHandle(
          reports.front().parameters.at(reportParameters.at(7U))) ==
          subject.federateHandle(),
      "GetDimensionHandle service report identified the wrong federate");

  std::wstring const missingDimensionName = L"TckAbsentDimension_2025";
  requireException(
      [&] {
        static_cast<void>(
            subject.rtiAmbassador().getDimensionHandle(missingDimensionName));
      },
      L"NameNotFound",
      "GetDimensionHandle with an undefined dimension name");
  waitFor(
      subject,
      observer,
      [&] { return observer.recorder().interactions().size() >= 2U; },
      options,
      "failed GetDimensionHandle service-report callback");
  auto const reportsAfterFailure = observer.recorder().interactions();
  require(
      reportsAfterFailure.size() == 2U,
      "GetDimensionHandle delivered an unexpected number of success and failure reports");
  requirePortableServiceReportFailure(
      observer,
      reportsAfterFailure.at(1U),
      reportClass,
      reportParameters,
      L"GetDimensionHandle",
      {{53, L"Dimension name", quoted(missingDimensionName)}},
      L"NameNotFound",
      1,
      "GetDimensionHandle NameNotFound service report");
  require(
      observer.rtiAmbassador().decodeFederateHandle(
          reportsAfterFailure.at(1U).parameters.at(reportParameters.at(7U))) ==
          subject.federateHandle(),
      "GetDimensionHandle NameNotFound service report identified the wrong federate");

  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.resign(rti::NO_ACTION);
  observer.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioServiceReportDimensionHandleContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportDimensionHandle(options, model);
}

void scenarioServiceReportInteractionDimensions(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Interaction-dimensions service-report testing requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "Interaction-dimensions service-report testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "service-report-interaction-dimensions-subject");
  Session observer(options, model, "service-report-interaction-dimensions-observer");
  auto const federation = federationName(options, "service-report-interaction-dimensions");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.modelFom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-service-report-interaction-dimensions-subject",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-interaction-dimensions-observer",
      options.federateType,
      federation);

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Interaction-dimensions service-report lookup returned an invalid report class handle");
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
      auto const parameter = observer.rtiAmbassador().getParameterHandle(
          reportClass,
          name);
      require(
          parameter.isValid(),
          "Interaction-dimensions service-report lookup returned an invalid parameter handle");
      result.push_back(parameter);
    }
    return result;
  }();

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);

  auto const dimension = subject.rtiAmbassador().getDimensionHandle(
      options.fomDimensionName);
  auto const interactionClass = subject.rtiAmbassador().getInteractionClassHandle(
      options.typedInteractionClassName);
  auto const rootInteractionClass =
      subject.rtiAmbassador().getInteractionClassHandle(L"HLAinteractionRoot");
  require(
      dimension.isValid() && interactionClass.isValid() &&
          rootInteractionClass.isValid(),
      "Adapter FOM interaction-dimensions setup returned an invalid handle");
  require(
      !subject.rtiAmbassador().getSendServiceReportsToFileSwitch(),
      "Interaction-dimensions service-report test could not select MOM delivery");
  subject.rtiAmbassador().setServiceReportingSwitch(true);
  auto const quoted = [](std::wstring const& value) {
    return std::wstring{L"\""} + value + L"\"";
  };
  auto const dimensions =
      subject.rtiAmbassador().getAvailableDimensionsForInteractionClass(interactionClass);
  require(
      dimensions == rti::DimensionHandleSet{dimension},
      "Adapter FOM interaction did not expose exactly its configured dimension");

  waitFor(
      subject,
      observer,
      [&] { return observer.recorder().interactions().size() >= 1U; },
      options,
      "successful GetAvailableDimensionsForInteractionClass service-report callback");
  auto const reports = observer.recorder().interactions();
  require(
      reports.size() == 1U,
      "GetAvailableDimensionsForInteractionClass delivered an unexpected number of service reports");
  requirePortableServiceReport(
      observer,
      reports.front(),
      reportClass,
      reportParameters,
      L"GetAvailableDimensionsForInteractionClass",
      {{27,
        L"Interaction class handle",
        quoted(interactionClass.toString())}},
      {11,
       L"A set of dimension handles",
       std::wstring{L"[\""} + dimension.toString() + L"\"]"},
      0,
      "GetAvailableDimensionsForInteractionClass service report");
  require(
      observer.rtiAmbassador().decodeFederateHandle(
          reports.front().parameters.at(reportParameters.at(7U))) ==
          subject.federateHandle(),
      "GetAvailableDimensionsForInteractionClass report identified the wrong federate");

  auto const rootDimensions =
      subject.rtiAmbassador().getAvailableDimensionsForInteractionClass(
          rootInteractionClass);
  require(
      rootDimensions.empty(),
      "HLAinteractionRoot unexpectedly has available dimensions");
  waitFor(
      subject,
      observer,
      [&] { return observer.recorder().interactions().size() >= 2U; },
      options,
      "empty GetAvailableDimensionsForInteractionClass service-report callback");
  auto const reportsIncludingEmptySet = observer.recorder().interactions();
  require(
      reportsIncludingEmptySet.size() == 2U,
      "GetAvailableDimensionsForInteractionClass delivered an unexpected number of reports");
  requirePortableServiceReport(
      observer,
      reportsIncludingEmptySet.at(1U),
      reportClass,
      reportParameters,
      L"GetAvailableDimensionsForInteractionClass",
      {{27,
        L"Interaction class handle",
        quoted(rootInteractionClass.toString())}},
      {11, L"A set of dimension handles", L"[]"},
      1,
      "GetAvailableDimensionsForInteractionClass empty-set service report");
  require(
      observer.rtiAmbassador().decodeFederateHandle(
          reportsIncludingEmptySet.at(1U).parameters.at(reportParameters.at(7U))) ==
          subject.federateHandle(),
      "GetAvailableDimensionsForInteractionClass empty-set report identified the wrong federate");

  rti::InteractionClassHandle const invalidInteractionClass;
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getAvailableDimensionsForInteractionClass(
            invalidInteractionClass));
      },
      L"InvalidInteractionClassHandle",
      "GetAvailableDimensionsForInteractionClass with an invalid interaction-class handle");
  waitFor(
      subject,
      observer,
      [&] { return observer.recorder().interactions().size() >= 3U; },
      options,
      "failed GetAvailableDimensionsForInteractionClass service-report callback");
  auto const reportsAfterInvalidHandle = observer.recorder().interactions();
  require(
      reportsAfterInvalidHandle.size() == 3U,
      "GetAvailableDimensionsForInteractionClass delivered an unexpected failure-report count");
  requirePortableServiceReportFailure(
      observer,
      reportsAfterInvalidHandle.at(2U),
      reportClass,
      reportParameters,
      L"GetAvailableDimensionsForInteractionClass",
      {{27,
        L"Interaction class handle",
        quoted(invalidInteractionClass.toString())}},
      L"InvalidInteractionClassHandle",
      2,
      "GetAvailableDimensionsForInteractionClass invalid-handle service report");
  require(
      observer.rtiAmbassador().decodeFederateHandle(
          reportsAfterInvalidHandle.at(2U).parameters.at(reportParameters.at(7U))) ==
          subject.federateHandle(),
      "GetAvailableDimensionsForInteractionClass invalid-handle report identified the wrong federate");

  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.resign(rti::NO_ACTION);
  observer.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioServiceReportInteractionDimensionsContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportInteractionDimensions(options, model);
}

void scenarioServiceReportObjectDimensions(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Object-dimensions service-report testing requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "Object-dimensions service-report testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "service-report-object-dimensions-subject");
  Session observer(options, model, "service-report-object-dimensions-observer");
  auto const federation = federationName(options, "service-report-object-dimensions");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.modelFom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-service-report-object-dimensions-subject",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-object-dimensions-observer",
      options.federateType,
      federation);

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Object-dimensions service-report lookup returned an invalid report class handle");
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
      auto const parameter = observer.rtiAmbassador().getParameterHandle(
          reportClass,
          name);
      require(
          parameter.isValid(),
          "Object-dimensions service-report lookup returned an invalid parameter handle");
      result.push_back(parameter);
    }
    return result;
  }();

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);

  auto const dimension = subject.rtiAmbassador().getDimensionHandle(
      options.fomDimensionName);
  auto const objectClass = subject.rtiAmbassador().getObjectClassHandle(
      options.typedObjectClassName);
  auto const rootObjectClass =
      subject.rtiAmbassador().getObjectClassHandle(L"HLAobjectRoot");
  require(
      dimension.isValid() && objectClass.isValid() && rootObjectClass.isValid(),
      "Adapter FOM object-dimensions setup returned an invalid handle");
  require(
      !subject.rtiAmbassador().getSendServiceReportsToFileSwitch(),
      "Object-dimensions service-report test could not select MOM delivery");
  subject.rtiAmbassador().setServiceReportingSwitch(true);
  auto const quoted = [](std::wstring const& value) {
    return std::wstring{L"\""} + value + L"\"";
  };
  auto const dimensions =
      subject.rtiAmbassador().getAvailableDimensionsForObjectClass(objectClass);
  require(
      dimensions == rti::DimensionHandleSet{dimension},
      "Adapter FOM object class did not expose exactly its configured dimension");

  waitFor(
      subject,
      observer,
      [&] { return observer.recorder().interactions().size() >= 1U; },
      options,
      "successful GetAvailableDimensionsForObjectClass service-report callback");
  auto const reports = observer.recorder().interactions();
  require(
      reports.size() == 1U,
      "GetAvailableDimensionsForObjectClass delivered an unexpected number of service reports");
  requirePortableServiceReport(
      observer,
      reports.front(),
      reportClass,
      reportParameters,
      L"GetAvailableDimensionsForObjectClass",
      {{36,
        L"Object class handle",
        quoted(objectClass.toString())}},
      {11,
       L"A set of dimension handles",
       std::wstring{L"[\""} + dimension.toString() + L"\"]"},
      0,
      "GetAvailableDimensionsForObjectClass service report");
  require(
      observer.rtiAmbassador().decodeFederateHandle(
          reports.front().parameters.at(reportParameters.at(7U))) ==
          subject.federateHandle(),
      "GetAvailableDimensionsForObjectClass report identified the wrong federate");

  auto const rootDimensions =
      subject.rtiAmbassador().getAvailableDimensionsForObjectClass(rootObjectClass);
  require(
      rootDimensions.empty(),
      "HLAobjectRoot unexpectedly has available dimensions");
  waitFor(
      subject,
      observer,
      [&] { return observer.recorder().interactions().size() >= 2U; },
      options,
      "empty GetAvailableDimensionsForObjectClass service-report callback");
  auto const reportsIncludingEmptySet = observer.recorder().interactions();
  require(
      reportsIncludingEmptySet.size() == 2U,
      "GetAvailableDimensionsForObjectClass delivered an unexpected number of reports");
  requirePortableServiceReport(
      observer,
      reportsIncludingEmptySet.at(1U),
      reportClass,
      reportParameters,
      L"GetAvailableDimensionsForObjectClass",
      {{36,
        L"Object class handle",
        quoted(rootObjectClass.toString())}},
      {11, L"A set of dimension handles", L"[]"},
      1,
      "GetAvailableDimensionsForObjectClass empty-set service report");
  require(
      observer.rtiAmbassador().decodeFederateHandle(
          reportsIncludingEmptySet.at(1U).parameters.at(reportParameters.at(7U))) ==
          subject.federateHandle(),
      "GetAvailableDimensionsForObjectClass empty-set report identified the wrong federate");

  rti::ObjectClassHandle const invalidObjectClass;
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getAvailableDimensionsForObjectClass(
            invalidObjectClass));
      },
      L"InvalidObjectClassHandle",
      "GetAvailableDimensionsForObjectClass with an invalid object-class handle");
  waitFor(
      subject,
      observer,
      [&] { return observer.recorder().interactions().size() >= 3U; },
      options,
      "failed GetAvailableDimensionsForObjectClass service-report callback");
  auto const reportsAfterInvalidHandle = observer.recorder().interactions();
  require(
      reportsAfterInvalidHandle.size() == 3U,
      "GetAvailableDimensionsForObjectClass delivered an unexpected failure-report count");
  requirePortableServiceReportFailure(
      observer,
      reportsAfterInvalidHandle.at(2U),
      reportClass,
      reportParameters,
      L"GetAvailableDimensionsForObjectClass",
      {{36,
        L"Object class handle",
        quoted(invalidObjectClass.toString())}},
      L"InvalidObjectClassHandle",
      2,
      "GetAvailableDimensionsForObjectClass invalid-handle service report");
  require(
      observer.rtiAmbassador().decodeFederateHandle(
          reportsAfterInvalidHandle.at(2U).parameters.at(reportParameters.at(7U))) ==
          subject.federateHandle(),
      "GetAvailableDimensionsForObjectClass invalid-handle report identified the wrong federate");

  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.resign(rti::NO_ACTION);
  observer.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioServiceReportObjectDimensionsContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportObjectDimensions(options, model);
}

void scenarioServiceReportOrderTransportationLookupFailures(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty(),
      "Order/transportation failure service-report testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "service-report-order-transportation-failure-subject");
  Session observer(options, model, "service-report-order-transportation-failure-observer");
  auto const federation = federationName(options, "service-report-order-transportation-failures");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-service-report-order-transportation-failure-subject",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-order-transportation-failure-observer",
      options.federateType,
      federation);

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Order/transportation failure report lookup returned an invalid report class handle");
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
      auto const parameter = observer.rtiAmbassador().getParameterHandle(reportClass, name);
      require(
          parameter.isValid(),
          "Order/transportation failure report lookup returned an invalid parameter handle");
      result.push_back(parameter);
    }
    return result;
  }();
  subject.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);
  auto const reliable = subject.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable");
  require(reliable.isValid(), "Order/transportation failure lookup could not resolve HLAreliable");
  subject.rtiAmbassador().setServiceReportingSwitch(true);

  auto const quoted = [](std::wstring const& value) {
    return std::wstring{L"\""} + value + L"\"";
  };
  struct ExpectedArgument final {
    rti::Integer32 type;
    std::wstring name;
    std::wstring value;
  };
  auto const decodeArgument = [](rti::HLAfixedRecord const& record,
                                 ExpectedArgument const& expected) {
    require(
        dynamic_cast<rti::HLAinteger32BE const&>(record.get(0U)).get() == expected.type,
        "Order/transportation failure report returned the wrong argument type");
    require(
        dynamic_cast<rti::HLAunicodeString const&>(record.get(1U)).get() == expected.name,
        "Order/transportation failure report returned the wrong argument name");
    require(
        dynamic_cast<rti::HLAunicodeString const&>(record.get(2U)).get() == expected.value,
        "Order/transportation failure report returned the wrong argument value");
  };
  auto const verifyReport = [&](InteractionRecord const& report,
                                std::wstring const& service,
                                ExpectedArgument const& expectedSupplied,
                                ExpectedArgument const& expectedReturned,
                                bool expectedSuccess,
                                std::wstring const& expectedException,
                                std::int32_t expectedSerial) {
    require(
        report.interaction == reportClass &&
            report.parameters.size() == reportParameters.size() && report.tag.empty() &&
            !report.producer.isValid() && !report.regions.has_value() &&
            report.transportation == reliable,
        "Order/transportation failure report returned non-standard metadata");
    for (auto const& parameter : reportParameters) {
      require(
          report.parameters.count(parameter) == 1U,
          "Order/transportation failure report omitted a standard parameter");
    }
    rti::HLAunicodeString decodedService;
    decodedService.decode(report.parameters.at(reportParameters.at(0U)));
    require(
        decodedService.get() == service,
        "Order/transportation failure report returned the wrong service name");
    rti::HLAinteger16BE decodedServiceType;
    decodedServiceType.decode(report.parameters.at(reportParameters.at(1U)));
    require(
        decodedServiceType.get() == 6,
        "Order/transportation failure report returned the wrong service type");
    rti::HLAboolean decodedSuccess;
    decodedSuccess.decode(report.parameters.at(reportParameters.at(2U)));
    require(
        decodedSuccess.get() == expectedSuccess,
        "Order/transportation failure report returned the wrong success indicator");

    rti::HLAfixedRecord argumentPrototype;
    argumentPrototype.appendElement(rti::HLAinteger32BE{})
        .appendElement(rti::HLAunicodeString{})
        .appendElement(rti::HLAunicodeString{});
    rti::HLAvariableArray suppliedArguments{argumentPrototype};
    suppliedArguments.decode(report.parameters.at(reportParameters.at(3U)));
    require(
        suppliedArguments.size() == 1U,
        "Order/transportation failure report returned the wrong supplied-argument count");
    decodeArgument(
        dynamic_cast<rti::HLAfixedRecord const&>(suppliedArguments.get(0U)),
        expectedSupplied);

    rti::HLAfixedRecord returnedArgument;
    returnedArgument.appendElement(rti::HLAinteger32BE{})
        .appendElement(rti::HLAunicodeString{})
        .appendElement(rti::HLAunicodeString{});
    returnedArgument.decode(report.parameters.at(reportParameters.at(4U)));
    decodeArgument(returnedArgument, expectedReturned);
    rti::HLAunicodeString decodedException;
    decodedException.decode(report.parameters.at(reportParameters.at(5U)));
    if (expectedSuccess) {
      require(
          decodedException.get().empty(),
          "Successful order/transportation lookup report carried an exception");
    } else {
      require(
          decodedException.get().find(expectedException) != std::wstring::npos,
          "Failed order/transportation lookup report omitted its standard exception class");
    }
    rti::HLAinteger32BE decodedSerial;
    decodedSerial.decode(report.parameters.at(reportParameters.at(6U)));
    require(
        decodedSerial.get() == expectedSerial,
        "Order/transportation failure report returned the wrong serial number");
  };

  std::wstring const unsupportedOrder = L"TckUnsupportedOrder";
  std::wstring const unsupportedTransportation = L"TckUnsupportedTransportation";
  requireException(
      [&] { subject.rtiAmbassador().getOrderType(unsupportedOrder); },
      L"InvalidOrderName",
      "lookup of an unsupported order name with service reporting enabled");
  requireException(
      [&] {
        subject.rtiAmbassador().getOrderName(
            static_cast<rti::OrderType>(0x7f));
      },
      L"InvalidOrderType",
      "lookup of an unsupported order type with service reporting enabled");
  requireException(
      [&] {
        subject.rtiAmbassador().getTransportationTypeHandle(unsupportedTransportation);
      },
      L"InvalidTransportationName",
      "lookup of an unsupported transportation name with service reporting enabled");
  rti::TransportationTypeHandle const invalidTransportation;
  requireException(
      [&] { subject.rtiAmbassador().getTransportationTypeName(invalidTransportation); },
      L"InvalidTransportationTypeHandle",
      "lookup of an invalid transportation handle with service reporting enabled");
  waitFor(
      subject,
      observer,
      [&] { return observer.recorder().interactions().size() == 4U; },
      options,
      "order/transportation failure service-report callbacks");

  auto const failureReports = observer.recorder().interactions();
  require(
      failureReports.size() == 4U,
      "Order/transportation failure service report count was unstable");
  verifyReport(
      failureReports.at(0U),
      L"GetOrderType",
      {53, L"Order name", quoted(unsupportedOrder)},
      {34, L"", L"null"},
      false,
      L"InvalidOrderName",
      0);
  verifyReport(
      failureReports.at(1U),
      L"GetOrderName",
      {38, L"Order type", quoted(L"UNSUPPORTED")},
      {34, L"", L"null"},
      false,
      L"InvalidOrderType",
      1);
  verifyReport(
      failureReports.at(2U),
      L"GetTransportationTypeHandle",
      {53, L"Transportation type name", quoted(unsupportedTransportation)},
      {34, L"", L"null"},
      false,
      L"InvalidTransportationName",
      2);
  verifyReport(
      failureReports.at(3U),
      L"GetTransportationTypeName",
      {59, L"Transportation type handle", quoted(invalidTransportation.toString())},
      {34, L"", L"null"},
      false,
      L"InvalidTransportationTypeHandle",
      3);

  require(
      subject.rtiAmbassador().getOrderType(L"Receive") == rti::RECEIVE,
      "Order/transportation lookup failed after reported failures");
  waitFor(
      subject,
      observer,
      [&] { return observer.recorder().interactions().size() == 5U; },
      options,
      "successful order lookup after failure service reports");
  auto const reports = observer.recorder().interactions();
  verifyReport(
      reports.at(4U),
      L"GetOrderType",
      {53, L"Order name", quoted(L"Receive")},
      {38, L"Order type", quoted(L"RECEIVE")},
      true,
      L"",
      4);

  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  subject.rtiAmbassador().setServiceReportingSwitch(false);
  observer.resign(rti::NO_ACTION);
  subject.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioServiceReportOrderTransportationLookupFailuresContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportOrderTransportationLookupFailures(options, model);
}




void scenarioServiceReportFederateObjectClassLookups(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Federate/object-class service-report testing requires an adapter-supplied model FOM");
  require(
      !options.mimFom.empty(),
      "Federate/object-class service-report testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "service-report-federate-object-class-subject");
  Session observer(options, model, "service-report-federate-object-class-observer");
  auto const federation = federationName(
      options,
      "service-report-federate-object-class-lookups");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.modelFom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  auto const subjectName =
      options.ownerFederateName + L"-service-report-federate-object-class-subject";
  subject.join(
      subjectName,
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-federate-object-class-observer",
      options.federateType,
      federation);

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Federate/object-class service-report lookup returned an invalid report class handle");
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
      auto const parameter = observer.rtiAmbassador().getParameterHandle(reportClass, name);
      require(
          parameter.isValid(),
          "Federate/object-class service-report lookup returned an invalid parameter handle");
      result.push_back(parameter);
    }
    return result;
  }();

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);

  auto const objectClassName = options.typedObjectClassName;
  auto const objectClass = subject.rtiAmbassador().getObjectClassHandle(objectClassName);
  require(
      objectClass.isValid(),
      "Federate/object-class service-report setup returned an invalid object-class handle");
  auto const subjectFederate = subject.federateHandle();
  require(
      subjectFederate.isValid(),
      "Federate/object-class service-report setup returned an invalid federate handle");

  subject.rtiAmbassador().setServiceReportingSwitch(true);
  auto const federateByName =
      subject.rtiAmbassador().getFederateHandle(subjectName);
  auto const federateName =
      subject.rtiAmbassador().getFederateName(subjectFederate);
  auto const objectClassByName =
      subject.rtiAmbassador().getObjectClassHandle(objectClassName);
  auto const objectClassResolvedName =
      subject.rtiAmbassador().getObjectClassName(objectClass);
  require(
      federateByName == subjectFederate &&
          federateName == subjectName &&
          objectClassByName == objectClass &&
          objectClassResolvedName == objectClassName,
      "Federate/object-class lookup did not round-trip its standard values");
  rti::ObjectClassHandle const invalidObjectClass;
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getObjectClassName(invalidObjectClass));
      },
      L"InvalidObjectClassHandle",
      "GetObjectClassName with an invalid object-class handle");
  auto const unknownObjectClassName = objectClassName + L"-missing";
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getObjectClassHandle(
            unknownObjectClassName));
      },
      L"NameNotFound",
      "GetObjectClassHandle with an unknown object-class name");
  auto const unknownFederateName = subjectName + L"-missing";
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getFederateHandle(
            unknownFederateName));
      },
      L"NameNotFound",
      "GetFederateHandle with an unknown federate name");
  rti::FederateHandle const invalidFederate;
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getFederateName(
            invalidFederate));
      },
      L"InvalidFederateHandle",
      "GetFederateName with an invalid federate handle");
  waitFor(
      subject,
      observer,
      [&] { return observer.recorder().interactions().size() >= 8U; },
      options,
      "federate/object-class service-report callbacks");

  auto const quoted = [](std::wstring const& value) {
    return std::wstring{L"\""} + value + L"\"";
  };
  auto const reports = observer.recorder().interactions();
  require(
      reports.size() == 8U,
      "Federate/object-class service-report count was unstable");
  requirePortableServiceReport(
      observer,
      reports.at(0U),
      reportClass,
      reportParameters,
      L"GetFederateHandle",
      {{53, L"Federate name", quoted(subjectName)}},
      {15, L"Federate handle", quoted(federateByName.toString())},
      0,
      "GetFederateHandle service report");
  requirePortableServiceReport(
      observer,
      reports.at(1U),
      reportClass,
      reportParameters,
      L"GetFederateName",
      {{15, L"Federate handle", quoted(subjectFederate.toString())}},
      {53, L"Federate name", quoted(subjectName)},
      1,
      "GetFederateName service report");
  requirePortableServiceReport(
      observer,
      reports.at(2U),
      reportClass,
      reportParameters,
      L"GetObjectClassHandle",
      {{53, L"Object class name", quoted(objectClassName)}},
      {36, L"Object class handle", quoted(objectClassByName.toString())},
      2,
      "GetObjectClassHandle service report");
  requirePortableServiceReport(
      observer,
      reports.at(3U),
      reportClass,
      reportParameters,
      L"GetObjectClassName",
      {{36, L"Object class handle", quoted(objectClass.toString())}},
      {53, L"Object class name", quoted(objectClassResolvedName)},
      3,
      "GetObjectClassName service report");
  requirePortableServiceReportFailure(
      observer,
      reports.at(4U),
      reportClass,
      reportParameters,
      L"GetObjectClassName",
      {{36,
        L"Object class handle",
        quoted(invalidObjectClass.toString())}},
      L"InvalidObjectClassHandle",
      4,
      "GetObjectClassName invalid-handle service report");
  require(
      observer.rtiAmbassador().decodeFederateHandle(
          reports.at(4U).parameters.at(reportParameters.at(7U))) == subjectFederate,
      "GetObjectClassName invalid-handle report identified the wrong federate");
  requirePortableServiceReportFailure(
      observer,
      reports.at(5U),
      reportClass,
      reportParameters,
      L"GetObjectClassHandle",
      {{53, L"Object class name", quoted(unknownObjectClassName)}},
      L"NameNotFound",
      5,
      "GetObjectClassHandle unknown-name service report");
  require(
      observer.rtiAmbassador().decodeFederateHandle(
          reports.at(5U).parameters.at(reportParameters.at(7U))) == subjectFederate,
      "GetObjectClassHandle unknown-name report identified the wrong federate");
  requirePortableServiceReportFailure(
      observer,
      reports.at(6U),
      reportClass,
      reportParameters,
      L"GetFederateHandle",
      {{53, L"Federate name", quoted(unknownFederateName)}},
      L"NameNotFound",
      6,
      "GetFederateHandle unknown-name service report");
  require(
      observer.rtiAmbassador().decodeFederateHandle(
          reports.at(6U).parameters.at(reportParameters.at(7U))) == subjectFederate,
      "GetFederateHandle unknown-name report identified the wrong federate");
  requirePortableServiceReportFailure(
      observer,
      reports.at(7U),
      reportClass,
      reportParameters,
      L"GetFederateName",
      {{15, L"Federate handle", quoted(invalidFederate.toString())}},
      L"InvalidFederateHandle",
      7,
      "GetFederateName invalid-handle service report");
  require(
      observer.rtiAmbassador().decodeFederateHandle(
          reports.at(7U).parameters.at(reportParameters.at(7U))) == subjectFederate,
      "GetFederateName invalid-handle report identified the wrong federate");

  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  subject.rtiAmbassador().setServiceReportingSwitch(false);
  observer.resign(rti::NO_ACTION);
  subject.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioServiceReportInteractionParameterLookups(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Interaction/parameter service-report testing requires an adapter-supplied model FOM");
  require(
      !options.mimFom.empty(),
      "Interaction/parameter service-report testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "service-report-interaction-parameter-subject");
  Session observer(options, model, "service-report-interaction-parameter-observer");
  auto const federation = federationName(
      options,
      "service-report-interaction-parameter-lookups");
  service_report_file_support::connectWithServiceReportDirectory(subject, options);
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.modelFom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  auto const subjectName =
      options.ownerFederateName + L"-service-report-interaction-parameter-subject";
  subject.join(
      subjectName,
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-interaction-parameter-observer",
      options.federateType,
      federation);

  auto const federateMomClass = observer.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateNameAttribute = observer.rtiAmbassador().getAttributeHandle(
      federateMomClass,
      L"HLAfederateName");
  auto const reportFileAttribute = observer.rtiAmbassador().getAttributeHandle(
      federateMomClass,
      L"HLAreportServiceFile");
  require(
      federateMomClass.isValid() && federateNameAttribute.isValid() &&
          reportFileAttribute.isValid(),
      "Standard Federate MOM did not expose HLAreportServiceFile");
  rti::AttributeHandleSet const federateMomAttributes{
      federateNameAttribute,
      reportFileAttribute};
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      federateMomClass,
      federateMomAttributes,
      true,
      L"");
  std::optional<std::filesystem::path> subjectReportFile;
  waitFor(
      observer,
      subject,
      [&] {
        subjectReportFile = service_report_file_support::advertisedServiceReportFile(
            observer.recorder(),
            federateNameAttribute,
            reportFileAttribute,
            subjectName);
        return subjectReportFile.has_value();
      },
      options,
      "subject HLAreportServiceFile MOM reflection");
  require(
      std::filesystem::is_regular_file(*subjectReportFile) &&
          !service_report_file_support::readServiceReportFile(*subjectReportFile).empty(),
      "Subject HLAreportServiceFile did not identify a file with its initial record");

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Interaction/parameter service-report lookup returned an invalid report class handle");
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
      auto const parameter = observer.rtiAmbassador().getParameterHandle(reportClass, name);
      require(
          parameter.isValid(),
          "Interaction/parameter service-report lookup returned an invalid parameter handle");
      result.push_back(parameter);
    }
    return result;
  }();

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);

  auto const interactionName = options.typedInteractionClassName;
  auto const parameterName = options.typedIntegerParameterName;
  auto const interaction =
      subject.rtiAmbassador().getInteractionClassHandle(interactionName);
  auto const parameter =
      subject.rtiAmbassador().getParameterHandle(interaction, parameterName);
  auto const unrelatedInteraction = subject.rtiAmbassador().getInteractionClassHandle(
      options.customRegionalInteractionClassName);
  auto const unrelatedParameter = subject.rtiAmbassador().getParameterHandle(
      unrelatedInteraction,
      options.customRegionalParameterName);
  require(
      interaction.isValid() && parameter.isValid() &&
          unrelatedInteraction.isValid() && unrelatedParameter.isValid() &&
          unrelatedInteraction != interaction && unrelatedParameter != parameter,
      "Interaction/parameter service-report setup returned invalid or overlapping handles");

  subject.rtiAmbassador().setServiceReportingSwitch(true);
  auto const interactionByName =
      subject.rtiAmbassador().getInteractionClassHandle(interactionName);
  auto const interactionResolvedName =
      subject.rtiAmbassador().getInteractionClassName(interaction);
  auto const parameterByName =
      subject.rtiAmbassador().getParameterHandle(interaction, parameterName);
  auto const parameterResolvedName =
      subject.rtiAmbassador().getParameterName(interaction, parameter);
  require(
      interactionByName == interaction &&
          interactionResolvedName == interactionName &&
          parameterByName == parameter &&
          parameterResolvedName == parameterName,
      "Interaction/parameter lookup did not round-trip its standard values");
  rti::InteractionClassHandle const invalidInteraction;
  rti::ParameterHandle const invalidParameter;
  auto const missingInteractionName = interactionName + L"-missing";
  auto const missingParameterName = parameterName + L"-missing";
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getParameterName(
            interaction,
            invalidParameter));
      },
      L"InvalidParameterHandle",
      "GetParameterName with an invalid parameter handle");
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getParameterHandle(
            interaction,
            missingParameterName));
      },
      L"NameNotFound",
      "GetParameterHandle with an unknown parameter name");
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getParameterHandle(
            invalidInteraction,
            parameterName));
      },
      L"InvalidInteractionClassHandle",
      "GetParameterHandle with an invalid interaction-class handle");
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getParameterName(
            invalidInteraction,
            parameter));
      },
      L"InvalidInteractionClassHandle",
      "GetParameterName with an invalid interaction-class handle");
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getInteractionClassName(
            invalidInteraction));
      },
      L"InvalidInteractionClassHandle",
      "GetInteractionClassName with an invalid interaction-class handle");
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getInteractionClassHandle(
            missingInteractionName));
      },
      L"NameNotFound",
      "GetInteractionClassHandle with an unknown interaction-class name");
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getParameterName(
            interaction,
            unrelatedParameter));
      },
      L"InteractionParameterNotDefined",
      "GetParameterName with a parameter defined only for another interaction class");
  waitFor(
      subject,
      observer,
      [&] { return observer.recorder().interactions().size() >= 11U; },
      options,
      "interaction/parameter service-report callbacks");

  auto const quoted = [](std::wstring const& value) {
    return std::wstring{L"\""} + value + L"\"";
  };
  auto const reports = observer.recorder().interactions();
  require(
      reports.size() == 11U,
      "Interaction/parameter service-report count was unstable");
  requirePortableServiceReport(
      observer,
      reports.at(0U),
      reportClass,
      reportParameters,
      L"GetInteractionClassHandle",
      {{53, L"Interaction class name", quoted(interactionName)}},
      {27, L"Interaction class handle", quoted(interactionByName.toString())},
      0,
      "GetInteractionClassHandle service report");
  requirePortableServiceReport(
      observer,
      reports.at(1U),
      reportClass,
      reportParameters,
      L"GetInteractionClassName",
      {{27, L"Interaction class handle", quoted(interaction.toString())}},
      {53, L"Interaction class name", quoted(interactionResolvedName)},
      1,
      "GetInteractionClassName service report");
  requirePortableServiceReport(
      observer,
      reports.at(2U),
      reportClass,
      reportParameters,
      L"GetParameterHandle",
      {{27, L"Interaction class handle", quoted(interaction.toString())},
       {53, L"Parameter name", quoted(parameterName)}},
      {39, L"Parameter handle", quoted(parameterByName.toString())},
      2,
      "GetParameterHandle service report");
  requirePortableServiceReport(
      observer,
      reports.at(3U),
      reportClass,
      reportParameters,
      L"GetParameterName",
      {{27, L"Interaction class handle", quoted(interaction.toString())},
       {39, L"Parameter handle", quoted(parameter.toString())}},
      {53, L"Parameter name", quoted(parameterResolvedName)},
      3,
      "GetParameterName service report");
  requirePortableServiceReportFailure(
      observer,
      reports.at(4U),
      reportClass,
      reportParameters,
      L"GetParameterName",
      {{27, L"Interaction class handle", quoted(interaction.toString())},
       {39, L"Parameter handle", quoted(invalidParameter.toString())}},
      L"InvalidParameterHandle",
      4,
      "GetParameterName invalid-handle service report");
  require(
      observer.rtiAmbassador().decodeFederateHandle(
          reports.at(4U).parameters.at(reportParameters.at(7U))) ==
          subject.federateHandle(),
      "GetParameterName invalid-handle report identified the wrong federate");
  requirePortableServiceReportFailure(
      observer,
      reports.at(5U),
      reportClass,
      reportParameters,
      L"GetParameterHandle",
      {{27, L"Interaction class handle", quoted(interaction.toString())},
       {53, L"Parameter name", quoted(missingParameterName)}},
      L"NameNotFound",
      5,
      "GetParameterHandle unknown-name service report");
  require(
      observer.rtiAmbassador().decodeFederateHandle(
          reports.at(5U).parameters.at(reportParameters.at(7U))) ==
          subject.federateHandle(),
      "GetParameterHandle unknown-name report identified the wrong federate");
  requirePortableServiceReportFailure(
      observer,
      reports.at(6U),
      reportClass,
      reportParameters,
      L"GetParameterHandle",
      {{27, L"Interaction class handle", quoted(invalidInteraction.toString())},
       {53, L"Parameter name", quoted(parameterName)}},
      L"InvalidInteractionClassHandle",
      6,
      "GetParameterHandle invalid-class service report");
  require(
      observer.rtiAmbassador().decodeFederateHandle(
          reports.at(6U).parameters.at(reportParameters.at(7U))) ==
          subject.federateHandle(),
      "GetParameterHandle invalid-class report identified the wrong federate");
  requirePortableServiceReportFailure(
      observer,
      reports.at(7U),
      reportClass,
      reportParameters,
      L"GetParameterName",
      {{27, L"Interaction class handle", quoted(invalidInteraction.toString())},
       {39, L"Parameter handle", quoted(parameter.toString())}},
      L"InvalidInteractionClassHandle",
      7,
      "GetParameterName invalid-class service report");
  require(
      observer.rtiAmbassador().decodeFederateHandle(
          reports.at(7U).parameters.at(reportParameters.at(7U))) ==
          subject.federateHandle(),
      "GetParameterName invalid-class report identified the wrong federate");
  requirePortableServiceReportFailure(
      observer,
      reports.at(8U),
      reportClass,
      reportParameters,
      L"GetInteractionClassName",
      {{27, L"Interaction class handle", quoted(invalidInteraction.toString())}},
      L"InvalidInteractionClassHandle",
      8,
      "GetInteractionClassName invalid-class service report");
  require(
      observer.rtiAmbassador().decodeFederateHandle(
          reports.at(8U).parameters.at(reportParameters.at(7U))) ==
          subject.federateHandle(),
      "GetInteractionClassName invalid-class report identified the wrong federate");
  requirePortableServiceReportFailure(
      observer,
      reports.at(9U),
      reportClass,
      reportParameters,
      L"GetInteractionClassHandle",
      {{53, L"Interaction class name", quoted(missingInteractionName)}},
      L"NameNotFound",
      9,
      "GetInteractionClassHandle unknown-name service report");
  require(
      observer.rtiAmbassador().decodeFederateHandle(
          reports.at(9U).parameters.at(reportParameters.at(7U))) ==
          subject.federateHandle(),
      "GetInteractionClassHandle unknown-name report identified the wrong federate");
  requirePortableServiceReportFailure(
      observer,
      reports.at(10U),
      reportClass,
      reportParameters,
      L"GetParameterName",
      {{27, L"Interaction class handle", quoted(interaction.toString())},
       {39, L"Parameter handle", quoted(unrelatedParameter.toString())}},
      L"InteractionParameterNotDefined",
      10,
      "GetParameterName undefined-parameter-on-class service report");
  require(
      observer.rtiAmbassador().decodeFederateHandle(
          reports.at(10U).parameters.at(reportParameters.at(7U))) ==
          subject.federateHandle(),
      "GetParameterName undefined-parameter-on-class report identified the wrong federate");

  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(true);
  subject.rtiAmbassador().setServiceReportingSwitch(true);
  auto const beforeHandleFileReport =
      service_report_file_support::readServiceReportFile(*subjectReportFile);
  auto const fileReportedInteraction =
      subject.rtiAmbassador().getInteractionClassHandle(interactionName);
  auto const afterHandleFileReport =
      service_report_file_support::readServiceReportFile(*subjectReportFile);
  require(
      fileReportedInteraction == interactionByName,
      "File-backed interaction-class lookup returned a different handle");
  auto const servicePosition = afterHandleFileReport.find(
      "\"HLAservice\":\"GetInteractionClassHandle\"",
      beforeHandleFileReport.size());
  require(
      servicePosition != std::string::npos,
      "Public HLAreportServiceFile did not append GetInteractionClassHandle");
  auto const recordLineBreak = afterHandleFileReport.rfind('\n', servicePosition);
  auto const recordStart = recordLineBreak == std::string::npos
      ? 0U
      : recordLineBreak + 1U;
  auto const recordEnd = afterHandleFileReport.find('\n', servicePosition);
  auto const record = std::string_view(afterHandleFileReport).substr(
      recordStart,
      (recordEnd == std::string::npos ? afterHandleFileReport.size() : recordEnd) -
          recordStart);
  std::string handleText;
  for (auto const character : fileReportedInteraction.toString()) {
    require(character >= 0 && character <= 0x7f, "InteractionClassHandle was not ASCII");
    handleText.push_back(static_cast<char>(character));
  }
  auto const expectedHandleArgument =
      "\"HLAargumentType\":27,\"HLAargumentValue\":\"" + handleText + "\"";
  require(
      record.find(expectedHandleArgument) != std::string_view::npos,
      "GetInteractionClassHandle file report did not use the Table 5 type-27 value");

  observer.rtiAmbassador().unsubscribeObjectClassAttributes(
      federateMomClass,
      federateMomAttributes);
  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.resign(rti::NO_ACTION);
  subject.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}



void scenarioServiceReportObjectAttributeUpdateRateLookups(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Object/attribute/update-rate service-report testing requires an adapter-supplied model FOM");
  require(
      !options.mimFom.empty(),
      "Object/attribute/update-rate service-report testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "service-report-object-attribute-rate-subject");
  Session observer(options, model, "service-report-object-attribute-rate-observer");
  auto const federation = federationName(
      options,
      "service-report-object-attribute-update-rate-lookups");
  auto const subjectName =
      options.ownerFederateName + L"-service-report-object-attribute-rate-subject";
  service_report_file_support::connectWithServiceReportDirectory(subject, options);
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.modelFom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(subjectName, options.federateType, federation);
  observer.join(
      options.memberFederateName + L"-service-report-object-attribute-rate-observer",
      options.federateType,
      federation);

  auto const federateMomClass = observer.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateNameAttribute = observer.rtiAmbassador().getAttributeHandle(
      federateMomClass,
      L"HLAfederateName");
  auto const reportFileAttribute = observer.rtiAmbassador().getAttributeHandle(
      federateMomClass,
      L"HLAreportServiceFile");
  require(
      federateMomClass.isValid() && federateNameAttribute.isValid() &&
          reportFileAttribute.isValid(),
      "Standard Federate MOM did not expose HLAreportServiceFile");
  rti::AttributeHandleSet const federateMomAttributes{
      federateNameAttribute,
      reportFileAttribute};
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      federateMomClass,
      federateMomAttributes,
      true,
      L"");
  std::optional<std::filesystem::path> subjectReportFile;
  waitFor(
      observer,
      subject,
      [&] {
        subjectReportFile = service_report_file_support::advertisedServiceReportFile(
            observer.recorder(),
            federateNameAttribute,
            reportFileAttribute,
            subjectName);
        return subjectReportFile.has_value();
      },
      options,
      "object/attribute/update-rate subject HLAreportServiceFile reflection");
  require(
      std::filesystem::is_regular_file(*subjectReportFile) &&
          !service_report_file_support::readServiceReportFile(*subjectReportFile).empty(),
      "Subject HLAreportServiceFile did not identify a file with its initial record");

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Object/attribute/update-rate service-report lookup returned an invalid report class handle");
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
      auto const parameter = observer.rtiAmbassador().getParameterHandle(reportClass, name);
      require(
          parameter.isValid(),
          "Object/attribute/update-rate service-report lookup returned an invalid parameter handle");
      result.push_back(parameter);
    }
    return result;
  }();

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);

  auto const objectClass = subject.rtiAmbassador().getObjectClassHandle(
      options.rateObjectClassName);
  auto const attribute = subject.rtiAmbassador().getAttributeHandle(
      objectClass,
      options.rateBestEffortAttributeName);
  require(
      objectClass.isValid() && attribute.isValid(),
      "Object/attribute/update-rate service-report setup returned an invalid handle");
  subject.rtiAmbassador().publishObjectClassAttributes(
      objectClass,
      rti::AttributeHandleSet{attribute});
  auto const object = subject.rtiAmbassador().registerObjectInstance(objectClass);
  require(
      object.isValid(),
      "Object/attribute/update-rate service-report registration returned an invalid object handle");
  auto const objectName = subject.rtiAmbassador().getObjectInstanceName(object);
  require(
      !objectName.empty(),
      "Object/attribute/update-rate service-report registration returned no object name");

  subject.rtiAmbassador().setServiceReportingSwitch(true);
  auto const knownObjectClass =
      subject.rtiAmbassador().getKnownObjectClassHandle(object);
  auto const objectByName =
      subject.rtiAmbassador().getObjectInstanceHandle(objectName);
  auto const objectResolvedName =
      subject.rtiAmbassador().getObjectInstanceName(object);
  auto const attributeByClass = subject.rtiAmbassador().getAttributeHandle(
      objectClass,
      options.rateBestEffortAttributeName);
  auto const attributeResolvedName = subject.rtiAmbassador().getAttributeName(
      objectClass,
      attribute);
  auto const maximumRate =
      subject.rtiAmbassador().getUpdateRateValue(options.rateDesignatorName);
  auto const attributeMaximumRate =
      subject.rtiAmbassador().getUpdateRateValueForAttribute(object, attribute);
  require(
      knownObjectClass == objectClass &&
          objectByName == object &&
          objectResolvedName == objectName &&
          attributeByClass == attribute &&
          attributeResolvedName == options.rateBestEffortAttributeName &&
          maximumRate > 0.0 &&
          attributeMaximumRate == 0.0,
      "Object/attribute/update-rate lookup did not round-trip its standard values");
  rti::AttributeHandle const invalidAttribute;
  auto const missingAttributeName =
      options.rateBestEffortAttributeName + L"-missing";
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getAttributeName(
            objectClass,
            invalidAttribute));
      },
      L"InvalidAttributeHandle",
      "GetAttributeName with an invalid attribute handle");
  auto const invalidRateDesignator = options.rateDesignatorName + L"-missing";
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getUpdateRateValue(
            invalidRateDesignator));
      },
      L"InvalidUpdateRateDesignator",
      "GetUpdateRateValue with an unknown update-rate designator");
  rti::ObjectInstanceHandle const unknownObject;
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getUpdateRateValueForAttribute(
            unknownObject,
            attribute));
      },
      L"ObjectInstanceNotKnown",
      "GetUpdateRateValueForAttribute with an unknown object instance");
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getUpdateRateValueForAttribute(
            object,
            invalidAttribute));
      },
      L"AttributeNotDefined",
      "GetUpdateRateValueForAttribute with an undefined attribute");
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getAttributeHandle(
            objectClass,
            missingAttributeName));
      },
      L"NameNotFound",
      "GetAttributeHandle with an unknown attribute name");
  rti::ObjectClassHandle const invalidObjectClass;
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getAttributeHandle(
            invalidObjectClass,
            options.rateBestEffortAttributeName));
      },
      L"InvalidObjectClassHandle",
      "GetAttributeHandle with an invalid object class handle");
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getObjectInstanceName(
            unknownObject));
      },
      L"ObjectInstanceNotKnown",
      "GetObjectInstanceName with an unknown object instance");
  auto const unknownObjectName = objectName + L"-missing";
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getObjectInstanceHandle(
            unknownObjectName));
      },
      L"ObjectInstanceNotKnown",
      "GetObjectInstanceHandle with an unknown object name");
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getKnownObjectClassHandle(
            unknownObject));
      },
      L"ObjectInstanceNotKnown",
      "GetKnownObjectClassHandle with an unknown object instance");
  waitFor(
      subject,
      observer,
      [&] { return observer.recorder().interactions().size() >= 16U; },
      options,
      "object/attribute/update-rate service-report callbacks");

  auto const quoted = [](std::wstring const& value) {
    return std::wstring{L"\""} + value + L"\"";
  };
  auto const reports = observer.recorder().interactions();
  require(
      reports.size() == 16U,
      "Object/attribute/update-rate service-report count was unstable; observed " +
          std::to_string(reports.size()));
  requirePortableServiceReport(
      observer,
      reports.at(0U),
      reportClass,
      reportParameters,
      L"GetKnownObjectClassHandle",
      {{37, L"Object instance handle", quoted(object.toString())}},
      {36, L"Object class handle", quoted(knownObjectClass.toString())},
      0,
      "GetKnownObjectClassHandle service report");
  requirePortableServiceReport(
      observer,
      reports.at(1U),
      reportClass,
      reportParameters,
      L"GetObjectInstanceHandle",
      {{53, L"Object instance name", quoted(objectName)}},
      {37, L"Object instance handle", quoted(objectByName.toString())},
      1,
      "GetObjectInstanceHandle service report");
  requirePortableServiceReport(
      observer,
      reports.at(2U),
      reportClass,
      reportParameters,
      L"GetObjectInstanceName",
      {{37, L"Object instance handle", quoted(object.toString())}},
      {53, L"Object instance name", quoted(objectResolvedName)},
      2,
      "GetObjectInstanceName service report");
  requirePortableServiceReport(
      observer,
      reports.at(3U),
      reportClass,
      reportParameters,
      L"GetAttributeHandle",
      {{36, L"Object class handle", quoted(objectClass.toString())},
       {53, L"Class attribute name", quoted(options.rateBestEffortAttributeName)}},
      {0, L"Class attribute handle", quoted(attributeByClass.toString())},
      3,
      "GetAttributeHandle service report");
  requirePortableServiceReport(
      observer,
      reports.at(4U),
      reportClass,
      reportParameters,
      L"GetAttributeName",
      {{36, L"Object class handle", quoted(objectClass.toString())},
       {0, L"Class attribute handle", quoted(attribute.toString())}},
      {53, L"Class attribute name", quoted(attributeResolvedName)},
      4,
      "GetAttributeName service report");
  requirePortableServiceReport(
      observer,
      reports.at(5U),
      reportClass,
      reportParameters,
      L"GetUpdateRateValue",
      {{53, L"Update rate name", quoted(options.rateDesignatorName)}},
      {35, L"Maximum update rate value", std::to_wstring(maximumRate)},
      5,
      "GetUpdateRateValue service report");
  requirePortableServiceReport(
      observer,
      reports.at(6U),
      reportClass,
      reportParameters,
      L"GetUpdateRateValueForAttribute",
      {{37, L"Object instance handle", quoted(object.toString())},
       {0, L"Attribute handle", quoted(attribute.toString())}},
      {35, L"Maximum update rate value", std::to_wstring(attributeMaximumRate)},
      6,
      "GetUpdateRateValueForAttribute service report");
  requirePortableServiceReportFailure(
      observer,
      reports.at(7U),
      reportClass,
      reportParameters,
      L"GetAttributeName",
      {{36, L"Object class handle", quoted(objectClass.toString())},
       {0, L"Class attribute handle", quoted(invalidAttribute.toString())}},
      L"InvalidAttributeHandle",
      7,
      "GetAttributeName invalid-handle service report");
  require(
      observer.rtiAmbassador().decodeFederateHandle(
          reports.at(7U).parameters.at(reportParameters.at(7U))) ==
          subject.federateHandle(),
      "GetAttributeName invalid-handle report identified the wrong federate");
  requirePortableServiceReportFailure(
      observer,
      reports.at(8U),
      reportClass,
      reportParameters,
      L"GetUpdateRateValue",
      {{53,
        L"Update rate name",
        quoted(invalidRateDesignator)}},
      L"InvalidUpdateRateDesignator",
      8,
      "GetUpdateRateValue invalid-designator service report");
  require(
      observer.rtiAmbassador().decodeFederateHandle(
          reports.at(8U).parameters.at(reportParameters.at(7U))) ==
          subject.federateHandle(),
      "GetUpdateRateValue invalid-designator report identified the wrong federate");
  requirePortableServiceReportFailure(
      observer,
      reports.at(9U),
      reportClass,
      reportParameters,
      L"GetUpdateRateValueForAttribute",
      {{37, L"Object instance handle", quoted(unknownObject.toString())},
       {0, L"Attribute handle", quoted(attribute.toString())}},
      L"ObjectInstanceNotKnown",
      9,
      "GetUpdateRateValueForAttribute unknown-object service report");
  require(
      observer.rtiAmbassador().decodeFederateHandle(
          reports.at(9U).parameters.at(reportParameters.at(7U))) ==
          subject.federateHandle(),
      "GetUpdateRateValueForAttribute unknown-object report identified the wrong federate");
  requirePortableServiceReportFailure(
      observer,
      reports.at(10U),
      reportClass,
      reportParameters,
      L"GetUpdateRateValueForAttribute",
      {{37, L"Object instance handle", quoted(object.toString())},
       {0, L"Attribute handle", quoted(invalidAttribute.toString())}},
      L"AttributeNotDefined",
      10,
      "GetUpdateRateValueForAttribute undefined-attribute service report");
  require(
      observer.rtiAmbassador().decodeFederateHandle(
          reports.at(10U).parameters.at(reportParameters.at(7U))) ==
          subject.federateHandle(),
      "GetUpdateRateValueForAttribute undefined-attribute report identified the wrong federate");
  requirePortableServiceReportFailure(
      observer,
      reports.at(11U),
      reportClass,
      reportParameters,
      L"GetAttributeHandle",
      {{36, L"Object class handle", quoted(objectClass.toString())},
       {53, L"Class attribute name", quoted(missingAttributeName)}},
      L"NameNotFound",
      11,
      "GetAttributeHandle unknown-name service report");
  require(
      observer.rtiAmbassador().decodeFederateHandle(
          reports.at(11U).parameters.at(reportParameters.at(7U))) ==
          subject.federateHandle(),
      "GetAttributeHandle unknown-name report identified the wrong federate");
  requirePortableServiceReportFailure(
      observer,
      reports.at(12U),
      reportClass,
      reportParameters,
      L"GetAttributeHandle",
      {{36, L"Object class handle", quoted(invalidObjectClass.toString())},
       {53, L"Class attribute name", quoted(options.rateBestEffortAttributeName)}},
      L"InvalidObjectClassHandle",
      12,
      "GetAttributeHandle invalid-class service report");
  require(
      observer.rtiAmbassador().decodeFederateHandle(
          reports.at(12U).parameters.at(reportParameters.at(7U))) ==
          subject.federateHandle(),
      "GetAttributeHandle invalid-class report identified the wrong federate");
  requirePortableServiceReportFailure(
      observer,
      reports.at(13U),
      reportClass,
      reportParameters,
      L"GetObjectInstanceName",
      {{37, L"Object instance handle", quoted(unknownObject.toString())}},
      L"ObjectInstanceNotKnown",
      13,
      "GetObjectInstanceName unknown-object service report");
  require(
      observer.rtiAmbassador().decodeFederateHandle(
          reports.at(13U).parameters.at(reportParameters.at(7U))) ==
          subject.federateHandle(),
      "GetObjectInstanceName unknown-object report identified the wrong federate");
  requirePortableServiceReportFailure(
      observer,
      reports.at(14U),
      reportClass,
      reportParameters,
      L"GetObjectInstanceHandle",
      {{53, L"Object instance name", quoted(unknownObjectName)}},
      L"ObjectInstanceNotKnown",
      14,
      "GetObjectInstanceHandle unknown-name service report");
  require(
      observer.rtiAmbassador().decodeFederateHandle(
          reports.at(14U).parameters.at(reportParameters.at(7U))) ==
          subject.federateHandle(),
      "GetObjectInstanceHandle unknown-name report identified the wrong federate");
  requirePortableServiceReportFailure(
      observer,
      reports.at(15U),
      reportClass,
      reportParameters,
      L"GetKnownObjectClassHandle",
      {{37, L"Object instance handle", quoted(unknownObject.toString())}},
      L"ObjectInstanceNotKnown",
      15,
      "GetKnownObjectClassHandle unknown-object service report");
  require(
      observer.rtiAmbassador().decodeFederateHandle(
          reports.at(15U).parameters.at(reportParameters.at(7U))) ==
          subject.federateHandle(),
      "GetKnownObjectClassHandle unknown-object report identified the wrong federate");

  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  auto const beforeRateFileReport =
      service_report_file_support::readServiceReportFile(*subjectReportFile);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(true);
  auto const fileMaximumRate =
      subject.rtiAmbassador().getUpdateRateValue(options.rateDesignatorName);
  require(
      fileMaximumRate == maximumRate,
      "GetUpdateRateValue returned a different result during the file-report check");
  auto const reportText =
      service_report_file_support::readServiceReportFile(*subjectReportFile);
  auto const rateServicePosition = reportText.find(
      "\"HLAservice\":\"GetUpdateRateValue\"",
      beforeRateFileReport.size());
  require(
      rateServicePosition != std::string::npos,
      "Subject HLAreportServiceFile omitted GetUpdateRateValue");
  auto const rateLineBreak = reportText.rfind('\n', rateServicePosition);
  auto const rateRecordStart = rateLineBreak == std::string::npos
      ? 0U
      : rateLineBreak + 1U;
  auto const rateRecordEnd = reportText.find('\n', rateServicePosition);
  auto const rateRecord = std::string_view(reportText).substr(
      rateRecordStart,
      (rateRecordEnd == std::string::npos ? reportText.size() : rateRecordEnd) -
          rateRecordStart);
  constexpr std::string_view numberArgumentPrefix =
      "\"HLAargumentType\":35,\"HLAargumentValue\":";
  auto const numberArgumentPosition = rateRecord.find(numberArgumentPrefix);
  require(
      numberArgumentPosition != std::string_view::npos &&
          rateRecord.find("\"HLAsuccessIndicator\":true") != std::string_view::npos,
      "GetUpdateRateValue file record did not return a successful type-35 Number");
  auto const numberValueStart = numberArgumentPosition + numberArgumentPrefix.size();
  auto const numberValueEnd = rateRecord.find_first_of(",}", numberValueStart);
  require(
      numberValueEnd != std::string_view::npos,
      "GetUpdateRateValue file record omitted its Number value");
  auto const numberValue = rateRecord.substr(
      numberValueStart,
      numberValueEnd - numberValueStart);
  std::string expectedNumberValue;
  for (auto const character : std::to_wstring(fileMaximumRate)) {
    require(
        character >= 0 && character <= 0x7f,
        "GetUpdateRateValue Number result was not ASCII");
    expectedNumberValue.push_back(static_cast<char>(character));
  }
  require(
      numberValue == expectedNumberValue,
      "GetUpdateRateValue file report did not encode its API result as an unquoted type-35 Number");

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().unsubscribeObjectClassAttributes(
      federateMomClass,
      federateMomAttributes);
  observer.resign(rti::NO_ACTION);
  subject.rtiAmbassador().unpublishObjectClassAttributes(
      objectClass,
      rti::AttributeHandleSet{attribute});
  subject.resign(rti::DELETE_OBJECTS);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

} // namespace hla_rti_cpp_tck::detail
