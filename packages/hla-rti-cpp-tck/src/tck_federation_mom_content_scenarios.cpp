#include "tck_federation_mom_content_scenarios.hpp"
#include "tck_main_scenarios.hpp"
#include <regex>

namespace hla_rti_cpp_tck::detail {

namespace {

bool disconnectServiceUsed(std::wstring const& fdd) {
  // HLAcurrentFDD is a standard FDD, but XML whitespace, quote style, attribute
  // order, and namespace-prefix choice are not part of this test's contract.
  std::wregex const element(
      LR"(<(?:[A-Za-z_][A-Za-z_0-9.-]*:)?disconnect\b[^>]*>)");
  std::wregex const attribute(
      LR"((section|isCallback|isUsed)\s*=\s*(['"])([^'"]*)\2)");
  bool found = false;
  bool used = false;
  for (std::wsregex_iterator tag(fdd.begin(), fdd.end(), element), end;
       tag != end; ++tag) {
    std::wstring const xml = tag->str();
    std::map<std::wstring, std::wstring> values;
    for (std::wsregex_iterator field(xml.begin(), xml.end(), attribute);
         field != end; ++field) {
      values[(*field)[1].str()] = (*field)[3].str();
    }
    if (values[L"section"] == L"4.3" && values[L"isCallback"] == L"false") {
      require(!found, "current FDD duplicated the disconnect service row");
      require(values.count(L"isUsed") == 1U,
              "current FDD omitted disconnect service usage");
      found = true;
      used = values[L"isUsed"] == L"true";
      require(used || values[L"isUsed"] == L"false",
              "current FDD returned a non-Boolean disconnect usage value");
    }
  }
  require(found, "current FDD omitted the adapter's disconnect service row");
  return used;
}

}  // namespace

void scenarioFomServiceUsageMerge(Options const& options,
                                  rti::CallbackModel model) {
  require(!options.mimFom.empty(),
          "service-usage merge requires an adapter-supplied standard MIM");
  auto const first = options.scenarioFoms.find("service-usage-first");
  auto const second = options.scenarioFoms.find("service-usage-second");
  require(first != options.scenarioFoms.end() &&
              second != options.scenarioFoms.end(),
          "service-usage merge requires two adapter-supplied FOM modules");

  Session creator(options, model, "service-usage-creator");
  Session observer(options, model, "service-usage-observer");
  auto const federation = federationName(options, "fom-service-usage-merge");
  creator.connect();
  observer.connect();
  auto const readUsage = [&](std::vector<std::wstring> const& modules,
                             bool expected) {
    creator.rtiAmbassador().createFederationExecutionWithMIM(
        federation, modules, options.mimFom.wstring(),
        options.logicalTimeImplementationName);
    observer.recorder().clearDiscovery();
    observer.recorder().clearReflection();
    observer.join(options.memberFederateName, options.federateType, federation);
    auto const federationClass = observer.rtiAmbassador().getObjectClassHandle(
        L"HLAobjectRoot.HLAmanager.HLAfederation");
    auto const currentFdd = observer.rtiAmbassador().getAttributeHandle(
        federationClass, L"HLAcurrentFDD");
    rti::AttributeHandleSet const attributes{currentFdd};
    observer.rtiAmbassador().subscribeObjectClassAttributes(
        federationClass, attributes, true, L"");
    waitFor(observer, [&] {
      auto const discovery = observer.recorder().discovery();
      return discovery.present && discovery.objectClass == federationClass &&
          discovery.name == L"HLAfederation";
    }, options, "the federation MOM object for service usage");
    auto const object = observer.recorder().discovery().object;
    observer.recorder().clearReflection();
    observer.rtiAmbassador().requestAttributeValueUpdate(
        object, attributes, rti::VariableLengthData{});
    waitFor(observer, [&] {
      auto const reflection = observer.recorder().reflection();
      return reflection.present && reflection.object == object &&
          reflection.values.count(currentFdd) == 1U;
    }, options, "the composed current FDD for service usage");
    rti::HLAunicodeString decoded;
    decoded.decode(observer.recorder().reflection().values.at(currentFdd));
    require(disconnectServiceUsed(decoded.get()) == expected,
            "current FDD did not logically OR the disconnect service usage");
    observer.resign(rti::NO_ACTION);
    creator.rtiAmbassador().destroyFederationExecution(federation);
  };
  readUsage({first->second.wstring()}, false);
  readUsage({second->second.wstring()}, true);
  readUsage({first->second.wstring(), second->second.wstring()}, true);
  readUsage({second->second.wstring(), first->second.wstring()}, true);
  observer.disconnect();
  creator.disconnect();
}

void scenarioFomServiceUsageMergeContract(Options const& options,
                                          rti::CallbackModel model) {
  scenarioFomServiceUsageMerge(options, model);
}

void scenarioFederationMomCurrentFdd(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty(),
      "Federation MOM current-FDD testing requires an adapter-supplied standard MIM");
  require(
      !options.additionalFomModules.empty(),
      "Federation MOM current-FDD testing requires an adapter-supplied extension FOM");

  Session owner(options, model, "federation-mom-current-fdd-owner");
  Session observer(options, model, "federation-mom-current-fdd-observer");
  Session extension(options, model, "federation-mom-current-fdd-extension");
  auto const federation = federationName(options, "federation-mom-current-fdd");
  owner.connect();
  observer.connect();
  extension.connect();
  owner.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);
  observer.join(options.memberFederateName, options.federateType, federation);

  auto const federationClass = observer.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederation");
  auto const currentFddAttribute = observer.rtiAmbassador().getAttributeHandle(
      federationClass,
      L"HLAcurrentFDD");
  auto const reliable = observer.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      federationClass.isValid() && currentFddAttribute.isValid() && reliable.isValid(),
      "Federation MOM current-FDD lookup returned an invalid standard handle");

  rti::AttributeHandleSet const currentFddAttributes{currentFddAttribute};
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      federationClass,
      currentFddAttributes,
      true,
      L"");
  waitFor(
      observer,
      [&] {
        auto const discovery = observer.recorder().discovery();
        return discovery.present && discovery.objectClass == federationClass &&
            discovery.name == L"HLAfederation";
      },
      options,
      "the standard federation MOM object discovery");

  auto const federationObject = observer.recorder().discovery().object;
  require(
      observer.rtiAmbassador().getKnownObjectClassHandle(federationObject) ==
          federationClass,
      "Federation MOM current-FDD discovery returned the wrong known class");
  require(
      observer.rtiAmbassador().getObjectInstanceName(federationObject) ==
          L"HLAfederation",
      "Federation MOM current-FDD discovery returned the wrong object name");
  require(
      observer.rtiAmbassador().getObjectInstanceHandle(L"HLAfederation") ==
          federationObject,
      "Federation MOM current-FDD object identity did not round-trip");

  observer.recorder().clearTransportationRecords();
  observer.rtiAmbassador().queryAttributeTransportationType(
      federationObject,
      currentFddAttribute);
  waitFor(
      observer,
      [&] { return !observer.recorder().attributeTransportationReports().empty(); },
      options,
      "the federation MOM current-FDD transportation report");
  auto const transportationReports = observer.recorder().attributeTransportationReports();
  auto const& transportation = transportationReports.back();
  require(
      transportation.object == federationObject &&
          transportation.attribute == currentFddAttribute &&
          transportation.transportation == reliable,
      "Federation MOM current-FDD reported the wrong transportation type");

  auto requestCurrentFdd = [&](std::string const& description) {
    observer.recorder().clearReflection();
    observer.rtiAmbassador().requestAttributeValueUpdate(
        federationObject,
        currentFddAttributes,
        rti::VariableLengthData{});
    waitFor(
        observer,
        [&] {
          auto const reflection = observer.recorder().reflection();
          return reflection.present && reflection.object == federationObject &&
              reflection.values.count(currentFddAttribute) == 1U;
        },
        options,
        description);
    auto const reflection = observer.recorder().reflection();
    require(
        reflection.transportation == reliable && reflection.tag.empty() &&
            !reflection.producer.isValid() && !reflection.regions.has_value(),
        description + " returned non-standard MOM callback metadata");
    rti::HLAunicodeString currentFdd;
    currentFdd.decode(reflection.values.at(currentFddAttribute));
    require(!currentFdd.get().empty(), description + " returned an empty HLAcurrentFDD");
    return currentFdd.get();
  };

  auto const baseFdd = requestCurrentFdd("the base current-FDD request");
  require(
      baseFdd.find(L"TckObject") != std::wstring::npos,
      "the base current-FDD value omitted the adapter FOM declaration");
  require(
      baseFdd.find(L"TckExtensionObject") == std::wstring::npos,
      "the base current-FDD value already contained the extension declaration");

  observer.recorder().clearReflection();
  extension.join(
      options.memberFederateName + L"-current-fdd-extension",
      options.federateType,
      federation,
      fomModuleNames(options.additionalFomModules));
  waitFor(
      observer,
      [&] {
        auto const reflection = observer.recorder().reflection();
        if (!reflection.present || reflection.object != federationObject) {
          return false;
        }
        auto const value = reflection.values.find(currentFddAttribute);
        if (value == reflection.values.end()) {
          return false;
        }
        rti::HLAunicodeString currentFdd;
        currentFdd.decode(value->second);
        return currentFdd.get().find(L"TckExtensionObject") != std::wstring::npos;
      },
      options,
      "the refreshed current-FDD reflection after an additional-FOM join");
  auto const refreshedReflection = observer.recorder().reflection();
  rti::HLAunicodeString refreshedFdd;
  refreshedFdd.decode(refreshedReflection.values.at(currentFddAttribute));
  auto const refreshedValue = refreshedFdd.get();
  require(
      refreshedValue.find(L"TckObject") != std::wstring::npos &&
          refreshedValue.find(L"TckExtensionObject") != std::wstring::npos &&
          refreshedValue.size() > baseFdd.size(),
      "the refreshed current-FDD value omitted the composed extension declaration");

  auto const directlyRequestedFdd = requestCurrentFdd(
      "the post-composition current-FDD request");
  require(
      directlyRequestedFdd == refreshedValue,
      "the direct current-FDD request disagreed with the join-triggered refresh");

  extension.resign(rti::NO_ACTION);
  observer.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  extension.disconnect();
  observer.disconnect();
  owner.disconnect();
}

void scenarioFederationMomFomModuleDesignatorList(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.additionalFomModules.empty() &&
          !options.mimFom.empty(),
      "Federation MOM FOM-module designator testing requires adapter-supplied base and additional FOM modules plus the standard MIM");
  auto const additionalModules = fomModuleNames(options.additionalFomModules);
  require(
      std::find(additionalModules.begin(), additionalModules.end(),
                options.fom.wstring()) == additionalModules.end(),
      "Federation MOM FOM-module designator testing requires a distinct additional module");

  Session owner(options, model, "federation-mom-fom-designators-owner");
  Session observer(options, model, "federation-mom-fom-designators-observer");
  Session extension(options, model, "federation-mom-fom-designators-extension");
  auto const federation = federationName(options, "federation-mom-fom-designators");
  owner.connect();
  observer.connect();
  extension.connect();
  if (options.logicalTimeImplementationName.empty()) {
    owner.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring());
  } else {
    owner.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring(),
        options.logicalTimeImplementationName);
  }
  auto const ownerName = options.ownerFederateName + L"-fom-designators-owner";
  auto const observerName = options.memberFederateName + L"-fom-designators-observer";
  auto const extensionName = options.memberFederateName + L"-fom-designators-extension";
  owner.join(ownerName, options.federateType, federation);
  observer.join(observerName, options.federateType, federation);

  auto const federationClass = observer.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederation");
  auto const designatorAttribute = observer.rtiAmbassador().getAttributeHandle(
      federationClass,
      L"HLAFOMmoduleDesignatorList");
  auto const reliable = observer.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      federationClass.isValid() && designatorAttribute.isValid() && reliable.isValid() &&
          observer.rtiAmbassador().getObjectClassName(federationClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederation" &&
          observer.rtiAmbassador().getAttributeName(federationClass, designatorAttribute) ==
              L"HLAFOMmoduleDesignatorList" &&
          observer.rtiAmbassador().getTransportationTypeName(reliable) == L"HLAreliable",
      "Federation MOM FOM-module designator lookup did not resolve standard MIM handles");
  rti::AttributeHandleSet const designatorAttributes{designatorAttribute};
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      federationClass,
      designatorAttributes,
      true,
      L"");
  waitFor(
      observer,
      [&] {
        auto const discovery = observer.recorder().discovery();
        return discovery.present && discovery.objectClass == federationClass &&
            discovery.name == L"HLAfederation";
      },
      options,
      "standard federation MOM discovery for the FOM-module designator list");
  auto const federationObject = observer.recorder().discovery().object;
  require(
      observer.rtiAmbassador().getKnownObjectClassHandle(federationObject) ==
              federationClass &&
          observer.rtiAmbassador().getObjectInstanceName(federationObject) ==
              L"HLAfederation" &&
          observer.rtiAmbassador().getObjectInstanceHandle(L"HLAfederation") ==
              federationObject,
      "Federation MOM FOM-module designator discovery did not preserve standard object identity");

  auto decodeDesignators = [](rti::VariableLengthData const& encoded) {
    rti::HLAvariableArrayT<rti::HLAunicodeString> values;
    values.decode(encoded);
    std::vector<std::wstring> result;
    result.reserve(values.size());
    for (std::size_t index = 0; index < values.size(); ++index) {
      result.push_back(
          dynamic_cast<rti::HLAunicodeString const&>(values.get(index)).get());
    }
    return result;
  };
  auto canonicalizeDesignators = [](std::vector<std::wstring> values) {
    std::sort(values.begin(), values.end());
    require(
        std::adjacent_find(values.begin(), values.end()) == values.end(),
        "HLAFOMmoduleDesignatorList contained a duplicate module designator");
    return values;
  };
  auto const initialDesignators = canonicalizeDesignators(
      std::vector<std::wstring>{options.fom.wstring()});
  auto const allDesignators = [&] {
    auto result = initialDesignators;
    result.insert(result.end(), additionalModules.begin(), additionalModules.end());
    return canonicalizeDesignators(std::move(result));
  }();

  auto verifyDesignatorReflection = [&](
      std::vector<std::wstring> const& expected,
      std::string const& description) {
    observer.recorder().clearReflection();
    observer.rtiAmbassador().requestAttributeValueUpdate(
        federationObject,
        designatorAttributes,
        rti::VariableLengthData{});
    waitFor(
        observer,
        [&] {
          auto const reflection = observer.recorder().reflection();
          if (!reflection.present || reflection.object != federationObject ||
              reflection.values.count(designatorAttribute) != 1U) {
            return false;
          }
          return canonicalizeDesignators(
                     decodeDesignators(reflection.values.at(designatorAttribute))) ==
              expected;
        },
        options,
        description);
    auto const reflection = observer.recorder().reflection();
    require(
        reflection.values.size() == 1U && reflection.transportation == reliable &&
            reflection.tag.empty() && !reflection.producer.isValid() &&
            !reflection.regions.has_value(),
        description + " returned incomplete data or non-standard MOM callback metadata");
    auto const actual = canonicalizeDesignators(
        decodeDesignators(reflection.values.at(designatorAttribute)));
    require(actual == expected, description + " returned the wrong FOM-module designator set");
    return actual;
  };

  auto const initialValue = verifyDesignatorReflection(
      initialDesignators,
      "the requested initial federation FOM-module designator list");
  require(
      initialValue.size() == 1U,
      "the created federation FOM-module designator list had an unexpected size");

  observer.recorder().clearReflection();
  extension.join(
      extensionName,
      options.federateType,
      federation,
      additionalModules);
  waitFor(
      observer,
      [&] {
        auto const reflection = observer.recorder().reflection();
        if (!reflection.present || reflection.object != federationObject ||
            reflection.values.count(designatorAttribute) != 1U) {
          return false;
        }
        return canonicalizeDesignators(
                   decodeDesignators(reflection.values.at(designatorAttribute))) ==
            allDesignators;
      },
      options,
      "the conditional federation FOM-module designator reflection after Join Federation Execution");
  auto const joinedReflection = observer.recorder().reflection();
  require(
      joinedReflection.values.size() == 1U &&
          joinedReflection.transportation == reliable && joinedReflection.tag.empty() &&
          !joinedReflection.producer.isValid() && !joinedReflection.regions.has_value(),
      "the join-triggered federation FOM-module designator reflection returned non-standard metadata");
  auto const joinedValue = canonicalizeDesignators(
      decodeDesignators(joinedReflection.values.at(designatorAttribute)));
  require(
      joinedValue == allDesignators && joinedValue.size() == 1U + additionalModules.size(),
      "the join-triggered federation FOM-module designator list omitted or duplicated modules");

  auto const requestedAfterJoin = verifyDesignatorReflection(
      allDesignators,
      "the requested federation FOM-module designator list after composition");
  require(
      requestedAfterJoin == joinedValue,
      "the requested federation designator list disagreed with the join-triggered update");

  observer.rtiAmbassador().unsubscribeObjectClassAttributes(
      federationClass,
      designatorAttributes);
  extension.resign(rti::NO_ACTION);
  observer.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  extension.disconnect();
  observer.disconnect();
  owner.disconnect();
}

void scenarioFederationMomContentReports(
    Options const& options,
    rti::CallbackModel model) {
  Session subject(options, model, "federation-mom-content-subject");
  Session observer(options, model, "federation-mom-content-observer");
  auto const federation = federationName(options, "federation-mom-content-reports");
  connectAndJoin(subject, observer, options, federation, options.fom);

  auto const fomRequestClass = subject.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestFOMmoduleData");
  auto const fomRequestIndicator = subject.rtiAmbassador().getParameterHandle(
      fomRequestClass,
      L"HLAFOMmoduleIndicator");
  auto const fomReportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportFOMmoduleData");
  auto const fomReportIndicator = observer.rtiAmbassador().getParameterHandle(
      fomReportClass,
      L"HLAFOMmoduleIndicator");
  auto const fomReportData = observer.rtiAmbassador().getParameterHandle(
      fomReportClass,
      L"HLAFOMmoduleData");
  auto const mimRequestClass = subject.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestMIMdata");
  auto const mimReportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportMIMdata");
  auto const mimReportData = observer.rtiAmbassador().getParameterHandle(
      mimReportClass,
      L"HLAMIMdata");
  auto const reliable = observer.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      fomRequestClass.isValid() && fomRequestIndicator.isValid() &&
          fomReportClass.isValid() && fomReportIndicator.isValid() &&
          fomReportData.isValid() && mimRequestClass.isValid() &&
          mimReportClass.isValid() && mimReportData.isValid() && reliable.isValid(),
      "Federation MOM content-report lookup returned an invalid standard handle");

  observer.rtiAmbassador().subscribeInteractionClass(fomReportClass);
  observer.rtiAmbassador().subscribeInteractionClass(mimReportClass);

  auto const hasInteraction = [&](rti::InteractionClassHandle const& expected) {
    auto const record = observer.recorder().interaction();
    return record.present && record.interaction == expected;
  };
  auto const sendFomRequest = [&] {
    rti::ParameterHandleValueMap parameters;
    parameters.emplace(fomRequestIndicator, rti::HLAinteger32BE{0}.encode());
    subject.rtiAmbassador().sendInteraction(
        fomRequestClass,
        parameters,
        rti::VariableLengthData{});
  };

  // In evoked mode this first request exercises subscription revalidation at
  // callback time. In immediate mode the callback boundary has already been
  // crossed, so it establishes the corresponding immediate-delivery contract.
  observer.recorder().clearInteraction();
  sendFomRequest();
  if (model == rti::HLA_EVOKED) {
    observer.rtiAmbassador().unsubscribeInteractionClass(fomReportClass);
    for (int pass = 0; pass != 8; ++pass) {
      observer.pump();
    }
    require(
        observer.recorder().interactions().empty(),
        "evoked MOM FOM-module report survived callback-time unsubscription");
    observer.rtiAmbassador().subscribeInteractionClass(fomReportClass);
  } else {
    waitFor(
        observer,
        [&] { return hasInteraction(fomReportClass); },
        options,
        "the immediate federation MOM FOM-module content report");
    require(
        observer.recorder().interactions().size() == 1U,
        "immediate MOM FOM-module request delivered more than one report");
    observer.rtiAmbassador().unsubscribeInteractionClass(fomReportClass);
    observer.rtiAmbassador().subscribeInteractionClass(fomReportClass);
  }

  observer.recorder().clearInteraction();
  sendFomRequest();
  waitFor(
      observer,
      [&] { return hasInteraction(fomReportClass); },
      options,
      "the federation MOM FOM-module content report");
  auto const fomReport = observer.recorder().interaction();
  require(
      fomReport.parameters.size() == 2U &&
          fomReport.parameters.count(fomReportIndicator) == 1U &&
          fomReport.parameters.count(fomReportData) == 1U,
      "federation MOM FOM-module report returned the wrong parameter set");
  require(
      fomReport.tag.empty() && fomReport.transportation == reliable &&
          !fomReport.producer.isValid() && !fomReport.regions.has_value(),
      "federation MOM FOM-module report returned non-standard callback metadata");
  rti::HLAinteger32BE decodedIndicator;
  decodedIndicator.decode(fomReport.parameters.at(fomReportIndicator));
  require(
      decodedIndicator.get() == 0,
      "federation MOM FOM-module report returned the wrong module indicator");
  rti::HLAunicodeString decodedFom;
  decodedFom.decode(fomReport.parameters.at(fomReportData));
  require(
      !decodedFom.get().empty() &&
          decodedFom.get().find(L"objectModel") != std::wstring::npos,
      "federation MOM FOM-module report did not carry standard FOM XML");

  observer.recorder().clearInteraction();
  subject.rtiAmbassador().sendInteraction(
      mimRequestClass,
      rti::ParameterHandleValueMap{},
      rti::VariableLengthData{});
  waitFor(
      observer,
      [&] { return hasInteraction(mimReportClass); },
      options,
      "the federation MOM MIM content report");
  auto const mimReport = observer.recorder().interaction();
  require(
      mimReport.parameters.size() == 1U &&
          mimReport.parameters.count(mimReportData) == 1U,
      "federation MOM MIM report returned the wrong parameter set");
  require(
      mimReport.tag.empty() && mimReport.transportation == reliable &&
          !mimReport.producer.isValid() && !mimReport.regions.has_value(),
      "federation MOM MIM report returned non-standard callback metadata");
  rti::HLAunicodeString decodedMim;
  decodedMim.decode(mimReport.parameters.at(mimReportData));
  require(
      !decodedMim.get().empty() &&
          decodedMim.get().find(L"HLArequestMIMdata") != std::wstring::npos &&
          decodedMim.get().find(L"HLAreportMIMdata") != std::wstring::npos,
      "federation MOM MIM report did not carry standard MIM XML");

  // The request class has no parameters. Supplying a parameter from the
  // FOM-module request must therefore fail with the standard public exception
  // and must not synthesize an application-level report.
  observer.recorder().clearInteraction();
  rti::ParameterHandleValueMap invalidMimParameters;
  invalidMimParameters.emplace(fomRequestIndicator, rti::HLAinteger32BE{0}.encode());
  requireException(
      [&] {
        subject.rtiAmbassador().sendInteraction(
            mimRequestClass,
            invalidMimParameters,
            rti::VariableLengthData{});
      },
      L"InteractionParameterNotDefined",
      "MOM MIM request with an undefined parameter");
  for (int pass = 0; pass != 8; ++pass) {
    observer.pump();
  }
  require(
      observer.recorder().interactions().empty(),
      "malformed MOM MIM request produced an application-level report");

  observer.rtiAmbassador().unsubscribeInteractionClass(fomReportClass);
  observer.rtiAmbassador().unsubscribeInteractionClass(mimReportClass);
  observer.resign(rti::NO_ACTION);
  subject.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioFederationMomCurrentFddContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederationMomCurrentFdd(options, model);
}

void scenarioFederationMomFomModuleDesignatorListContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederationMomFomModuleDesignatorList(options, model);
}

void scenarioFederationMomContentReportsContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederationMomContentReports(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
