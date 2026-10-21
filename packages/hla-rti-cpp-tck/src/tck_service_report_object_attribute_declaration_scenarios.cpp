#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioServiceReportObjectAttributeDeclaration(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty(),
      "Object-attribute declaration service-report testing requires an adapter-supplied standard MIM");
  require(
      !options.multiAttributeFom.empty(),
      "Object-attribute publication service-report testing requires an adapter-supplied multi-attribute FOM");

  Session subject(options, model, "object-attribute-declaration-service-report-subject");
  Session observer(options, model, "object-attribute-declaration-service-report-observer");
  auto const federation = federationName(
      options,
      "service-report-object-attribute-declaration");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{
          options.fom.wstring(), options.multiAttributeFom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-object-attribute-declaration-service-report-subject",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-object-attribute-declaration-service-report-observer",
      options.federateType,
      federation);

  auto const objectClass = subject.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const attribute = subject.rtiAmbassador().getAttributeHandle(
      objectClass,
      options.attributeName);
  auto const publicationClass = subject.rtiAmbassador().getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const publicationAttribute = subject.rtiAmbassador().getAttributeHandle(
      publicationClass,
      options.multiAttributeFirstName);
  auto const secondPublicationAttribute = subject.rtiAmbassador().getAttributeHandle(
      publicationClass,
      options.multiAttributeSecondName);
  require(
      objectClass.isValid() && attribute.isValid(),
      "Object-attribute declaration service-report lookup returned an invalid object or attribute handle");
  require(
      publicationClass.isValid() && publicationAttribute.isValid() &&
          secondPublicationAttribute.isValid() &&
          publicationAttribute != secondPublicationAttribute,
      "Object-attribute publication service-report lookup returned an invalid or duplicate handle");

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Object-attribute declaration service-report lookup returned an invalid HLAreportServiceInvocation handle");
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
        "Object-attribute declaration service-report parameter lookup returned an invalid handle");
    reportParameters.push_back(parameter);
  }

  // Keep setup out of the asserted reports. The eight declaration calls below
  // are the only service invocations whose reporting switch is enabled.
  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);

  struct ExpectedArgument final {
    std::int32_t type;
    std::wstring name;
    std::wstring value;
  };
  auto const quoted = [](std::wstring const& value) {
    return std::wstring{L"\""} + value + L"\"";
  };
  auto const subscribeArguments = [&](std::wstring const& objectValue,
                                      std::wstring const& attributeSetValue,
                                      bool passive,
                                      std::wstring const& rate) {
    return std::vector<ExpectedArgument>{
        {36, L"Object class designator", quoted(objectValue)},
        {1, L"Set of attribute designators", attributeSetValue},
        {6, L"Optional passive subscription indicator", passive ? L"true" : L"false"},
        {53, L"Optional update rate designator", quoted(rate)}};
  };
  auto const unsubscribeArguments = [&](std::wstring const& objectValue,
                                        std::wstring const& attributeSetValue) {
    return std::vector<ExpectedArgument>{
        {36, L"Object class designator", quoted(objectValue)},
        {1, L"Optional set of attribute designators", attributeSetValue}};
  };
  auto const wholeUnsubscribeArguments = [&](std::wstring const& objectValue) {
    return std::vector<ExpectedArgument>{
        {36, L"Object class designator", quoted(objectValue)},
        {34, L"Optional set of attribute designators", L"null"}};
  };
  auto const attributeHandleSetValue = [&](rti::AttributeHandleSet const& handles) {
    std::wstring value{L"["};
    bool first = true;
    for (auto const handle : handles) {
      if (!first) {
        value += L",";
      }
      first = false;
      value += quoted(handle.toString());
    }
    value += L"]";
    return value;
  };

  auto verifyReport = [&](std::size_t index,
                          std::wstring const& expectedService,
                          std::vector<ExpectedArgument> const& expectedArguments,
                          bool expectedSuccess,
                          std::wstring const& expectedException) {
    waitFor(
        observer,
        [&] { return observer.recorder().interaction().present; },
        options,
        "object-attribute declaration HLAreportServiceInvocation callback");
    auto const report = observer.recorder().interaction();
    require(
        report.interaction == reportClass &&
            report.parameters.size() == reportParameters.size() &&
            !report.producer.isValid(),
        "Object-attribute declaration service-report callback returned the wrong identity or producer metadata");
    require(
        report.tag.empty() && !report.regions.has_value(),
        "Object-attribute declaration service-report callback returned user tag or region metadata");
    require(
        report.transportation ==
            observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable"),
        "Object-attribute declaration service-report callback returned the wrong transportation type");
    for (auto const parameterHandle : reportParameters) {
      require(
          report.parameters.count(parameterHandle) == 1U,
          "Object-attribute declaration service-report callback omitted a standard report parameter");
    }

    rti::HLAunicodeString service;
    service.decode(report.parameters.at(reportParameters[0]));
    require(
        service.get() == expectedService,
        "Object-attribute declaration service-report callback returned the wrong service name");
    rti::HLAinteger16BE serviceType;
    serviceType.decode(report.parameters.at(reportParameters[1]));
    require(
        serviceType.get() == 1,
        "Object-attribute declaration service-report callback returned the wrong service type");
    rti::HLAboolean success;
    success.decode(report.parameters.at(reportParameters[2]));
    require(
        success.get() == expectedSuccess,
        "Object-attribute declaration service-report callback returned the wrong success indicator");

    rti::HLAfixedRecord suppliedArgumentPrototype;
    suppliedArgumentPrototype.appendElement(rti::HLAinteger32BE{})
        .appendElement(rti::HLAunicodeString{})
        .appendElement(rti::HLAunicodeString{});
    rti::HLAvariableArray suppliedArguments{suppliedArgumentPrototype};
    suppliedArguments.decode(report.parameters.at(reportParameters[3]));
    require(
        suppliedArguments.size() == expectedArguments.size(),
        "Object-attribute declaration service-report callback returned the wrong supplied-argument count");
    for (std::size_t argumentIndex = 0U;
         argumentIndex != expectedArguments.size();
         ++argumentIndex) {
      auto const& supplied = dynamic_cast<rti::HLAfixedRecord const&>(
          suppliedArguments.get(argumentIndex));
      require(
          dynamic_cast<rti::HLAinteger32BE const&>(supplied.get(0U)).get() ==
                  expectedArguments[argumentIndex].type &&
              dynamic_cast<rti::HLAunicodeString const&>(supplied.get(1U)).get() ==
                  expectedArguments[argumentIndex].name &&
              dynamic_cast<rti::HLAunicodeString const&>(supplied.get(2U)).get() ==
                  expectedArguments[argumentIndex].value,
          "Object-attribute declaration service-report callback returned the wrong supplied-argument value");
    }

    rti::HLAfixedRecord returnedArgument;
    returnedArgument.appendElement(rti::HLAinteger32BE{})
        .appendElement(rti::HLAunicodeString{})
        .appendElement(rti::HLAunicodeString{});
    returnedArgument.decode(report.parameters.at(reportParameters[4]));
    require(
        dynamic_cast<rti::HLAinteger32BE const&>(returnedArgument.get(0U)).get() == 34 &&
            dynamic_cast<rti::HLAunicodeString const&>(returnedArgument.get(1U)).get().empty() &&
            dynamic_cast<rti::HLAunicodeString const&>(returnedArgument.get(2U)).get() == L"null",
        "Object-attribute declaration service-report callback returned the wrong Null return value");
    rti::HLAunicodeString exception;
    exception.decode(report.parameters.at(reportParameters[5]));
    if (expectedException.empty()) {
      require(
          exception.get().empty(),
          "Successful object-attribute declaration service report carried an exception");
    } else {
      require(
          exception.get().find(expectedException) != std::wstring::npos,
          "Object-attribute declaration service report returned the wrong exception name");
    }
    rti::HLAinteger32BE serial;
    serial.decode(report.parameters.at(reportParameters[6]));
    require(
        serial.get() == static_cast<std::int32_t>(index),
        "Object-attribute declaration service-report callback returned the wrong serial number");
    observer.recorder().clearInteraction();
  };

  auto const invalidObject = rti::ObjectClassHandle{}.toString();
  auto const validObject = objectClass.toString();
  auto const invalidAttribute = rti::AttributeHandle{}.toString();
  auto const validAttribute = attribute.toString();
  auto const invalidSet = L"[\"" + invalidAttribute + L"\"]";
  auto const validSet = L"[\"" + validAttribute + L"\"]";
  rti::AttributeHandleSet const attributes{attribute};

  subject.rtiAmbassador().setServiceReportingSwitch(true);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);

  requireException(
      [&] {
        subject.rtiAmbassador().subscribeObjectClassAttributes(
            rti::ObjectClassHandle{},
            rti::AttributeHandleSet{},
            false,
            L"High");
      },
      L"ObjectClassNotDefined",
      "subscribing an undefined object class with object-attribute declaration service reporting enabled");
  verifyReport(
      0U,
      L"SubscribeObjectClassAttributes",
      subscribeArguments(invalidObject, L"[]", true, L"High"),
      false,
      L"ObjectClassNotDefined");

  requireException(
      [&] {
        subject.rtiAmbassador().subscribeObjectClassAttributes(
            objectClass,
            rti::AttributeHandleSet{rti::AttributeHandle{}},
            false,
            L"High");
      },
      L"AttributeNotDefined",
      "subscribing an undefined attribute with object-attribute declaration service reporting enabled");
  verifyReport(
      1U,
      L"SubscribeObjectClassAttributes",
      subscribeArguments(validObject, invalidSet, true, L"High"),
      false,
      L"AttributeNotDefined");

  requireException(
      [&] {
        subject.rtiAmbassador().unsubscribeObjectClassAttributes(
            rti::ObjectClassHandle{},
            rti::AttributeHandleSet{});
      },
      L"ObjectClassNotDefined",
      "unsubscribing an undefined object class attribute set with object-attribute declaration service reporting enabled");
  verifyReport(
      2U,
      L"UnsubscribeObjectClassAttributes",
      unsubscribeArguments(invalidObject, L"[]"),
      false,
      L"ObjectClassNotDefined");

  requireException(
      [&] {
        subject.rtiAmbassador().unsubscribeObjectClassAttributes(
            objectClass,
            rti::AttributeHandleSet{rti::AttributeHandle{}});
      },
      L"AttributeNotDefined",
      "unsubscribing an undefined attribute with object-attribute declaration service reporting enabled");
  verifyReport(
      3U,
      L"UnsubscribeObjectClassAttributes",
      unsubscribeArguments(validObject, invalidSet),
      false,
      L"AttributeNotDefined");

  requireException(
      [&] { subject.rtiAmbassador().unsubscribeObjectClass(rti::ObjectClassHandle{}); },
      L"ObjectClassNotDefined",
      "unsubscribing an undefined object class with object-attribute declaration service reporting enabled");
  verifyReport(
      4U,
      L"UnsubscribeObjectClassAttributes",
      wholeUnsubscribeArguments(invalidObject),
      false,
      L"ObjectClassNotDefined");

  subject.rtiAmbassador().subscribeObjectClassAttributes(
      objectClass,
      attributes,
      false,
      L"High");
  verifyReport(
      5U,
      L"SubscribeObjectClassAttributes",
      subscribeArguments(validObject, validSet, true, L"High"),
      true,
      L"");
  subject.rtiAmbassador().unsubscribeObjectClassAttributes(objectClass, attributes);
  verifyReport(
      6U,
      L"UnsubscribeObjectClassAttributes",
      unsubscribeArguments(validObject, validSet),
      true,
      L"");
  subject.rtiAmbassador().unsubscribeObjectClass(objectClass);
  verifyReport(
      7U,
      L"UnsubscribeObjectClassAttributes",
      wholeUnsubscribeArguments(validObject),
      true,
      L"");

  rti::AttributeHandleSet const publicationAttributes{
      publicationAttribute, secondPublicationAttribute};
  require(
      publicationAttributes.size() == 2U,
      "Object-attribute publication service-report fixture did not provide two distinct attributes");
  subject.rtiAmbassador().publishObjectClassAttributes(
      publicationClass,
      publicationAttributes);
  verifyReport(
      8U,
      L"PublishObjectClassAttributes",
      {{36, L"Object class designator", quoted(publicationClass.toString())},
       {1,
        L"Set of attribute designators",
        attributeHandleSetValue(publicationAttributes)}},
      true,
      L"");

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  observer.resign(rti::NO_ACTION);
  subject.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

} // namespace hla_rti_cpp_tck::detail
