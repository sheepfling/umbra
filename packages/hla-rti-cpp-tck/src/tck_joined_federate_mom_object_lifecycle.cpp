#include "tck_joined_federate_mom_object_lifecycle.hpp"
#include "tck_main_scenarios.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioJoinedFederateMomObjectLifecycle(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Joined-federate MOM object lifecycle requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "Joined-federate MOM object lifecycle requires an adapter-supplied standard MIM");
  require(
      !options.additionalFomModules.empty(),
      "Joined-federate MOM object lifecycle requires an adapter-supplied additional FOM module");

  Session moduleSubject(options, model, "owner");
  Session noModuleSubject(options, model, "member");
  Session observer(options, model, "member");
  auto const federation = federationName(options, "joined-federate-mom-object-lifecycle");
  moduleSubject.connect();
  noModuleSubject.connect();
  observer.connect();
  if (options.logicalTimeImplementationName.empty()) {
    moduleSubject.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring());
  } else {
    moduleSubject.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring(),
        options.logicalTimeImplementationName);
  }

  auto const observerName = options.memberFederateName + L"-mom-lifecycle-observer";
  auto const moduleSubjectName = options.ownerFederateName + L"-mom-lifecycle-module-subject";
  auto const noModuleSubjectName = options.memberFederateName + L"-mom-lifecycle-no-module";
  observer.join(observerName, options.federateType, federation);
  auto const momClass = observer.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateHandleAttribute = observer.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAfederateHandle");
  auto const federateNameAttribute = observer.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAfederateName");
  auto const federateTypeAttribute = observer.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAfederateType");
  auto const federateHostAttribute = observer.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAfederateHost");
  auto const fomModuleListAttribute = observer.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAFOMmoduleDesignatorList");
  auto const reliable = observer.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  rti::AttributeHandleSet const momAttributes{
      federateHandleAttribute,
      federateNameAttribute,
      federateTypeAttribute,
      federateHostAttribute,
      fomModuleListAttribute};
  require(
      momClass.isValid() && federateHandleAttribute.isValid() &&
          federateNameAttribute.isValid() && federateTypeAttribute.isValid() &&
          federateHostAttribute.isValid() &&
          fomModuleListAttribute.isValid() && reliable.isValid() &&
          observer.rtiAmbassador().getObjectClassName(momClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederate" &&
          observer.rtiAmbassador().getAttributeName(momClass, federateHandleAttribute) ==
              L"HLAfederateHandle" &&
          observer.rtiAmbassador().getAttributeName(momClass, federateNameAttribute) ==
              L"HLAfederateName" &&
          observer.rtiAmbassador().getAttributeName(momClass, federateTypeAttribute) ==
              L"HLAfederateType" &&
          observer.rtiAmbassador().getAttributeName(momClass, federateHostAttribute) ==
              L"HLAfederateHost" &&
          observer.rtiAmbassador().getAttributeName(momClass, fomModuleListAttribute) ==
              L"HLAFOMmoduleDesignatorList" &&
          observer.rtiAmbassador().getTransportationTypeName(reliable) == L"HLAreliable",
      "Joined-federate MOM object lifecycle did not resolve the standard MIM handles");
  observer.rtiAmbassador().subscribeObjectClassAttributes(momClass, momAttributes, true, L"");

  auto decodeUnicodeString = [](rti::VariableLengthData const& encoded) {
    rti::HLAunicodeString value;
    value.decode(encoded);
    return value.get();
  };
  auto decodeUnicodeStringList = [](rti::VariableLengthData const& encoded) {
    rti::HLAvariableArrayT<rti::HLAunicodeString> values;
    values.decode(encoded);
    std::vector<std::wstring> result;
    result.reserve(values.size());
    for (std::size_t index = 0; index != values.size(); ++index) {
      result.push_back(
          dynamic_cast<rti::HLAunicodeString const&>(values.get(index)).get());
    }
    return result;
  };
  auto findReflectionFor = [&](rti::FederateHandle const& federate)
      -> std::optional<ReflectionRecord> {
    auto const expectedHandle = copyBytes(federate.encode());
    auto const reflections = observer.recorder().reflections();
    auto const iterator = std::find_if(
        reflections.begin(),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          auto const value = reflection.values.find(federateHandleAttribute);
          return value != reflection.values.end() &&
              copyBytes(value->second) == expectedHandle;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return *iterator;
  };
  auto verifyReflection = [&](ReflectionRecord const& reflection,
                              rti::FederateHandle const& federate,
                              std::wstring const& federateName,
                              std::wstring const& federateType,
                              std::vector<std::wstring> const& expectedModules,
                              std::string const& description) {
    require(
        reflection.values.size() == momAttributes.size() &&
            reflection.values.count(federateHandleAttribute) == 1U &&
            reflection.values.count(federateNameAttribute) == 1U &&
            reflection.values.count(federateTypeAttribute) == 1U &&
            reflection.values.count(federateHostAttribute) == 1U &&
            reflection.values.count(fomModuleListAttribute) == 1U &&
            reflection.transportation == reliable,
        description + " returned incomplete MOM reflection data or the wrong MIM transportation");
    require(
        copyBytes(reflection.values.at(federateHandleAttribute)) ==
                copyBytes(federate.encode()) &&
            decodeUnicodeString(reflection.values.at(federateNameAttribute)) == federateName &&
            decodeUnicodeString(reflection.values.at(federateTypeAttribute)) == federateType &&
            decodeUnicodeStringList(reflection.values.at(fomModuleListAttribute)) ==
                expectedModules,
        description + " reflected a different federate identity or FOM designator list");
    require(
        !decodeUnicodeString(reflection.values.at(federateHostAttribute)).empty(),
        description + " omitted the required HLAfederateHost Unicode value");
  };
  auto requestKnownValues = [&](rti::ObjectInstanceHandle const& object,
                                rti::FederateHandle const& federate,
                                std::wstring const& federateName,
                                std::wstring const& federateType,
                                std::vector<std::wstring> const& expectedModules,
                                std::string const& description) {
    auto const before = observer.recorder().reflections().size();
    observer.rtiAmbassador().requestAttributeValueUpdate(
        object,
        momAttributes,
        rti::VariableLengthData{});
    waitFor(
        observer,
        [&] {
          auto const reflections = observer.recorder().reflections();
          return reflections.size() > before &&
              std::any_of(
                  reflections.begin() + static_cast<std::ptrdiff_t>(before),
                  reflections.end(),
                  [&](ReflectionRecord const& reflection) {
                    return reflection.object == object &&
                        reflection.values.count(federateHandleAttribute) == 1U;
                  });
        },
        options,
        description + " known-object request");
    auto const reflections = observer.recorder().reflections();
    auto const iterator = std::find_if(
        reflections.begin() + static_cast<std::ptrdiff_t>(before),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          return reflection.object == object &&
              reflection.values.count(federateHandleAttribute) == 1U;
        });
    require(iterator != reflections.end(), description + " produced no requested reflection");
    verifyReflection(
        *iterator,
        federate,
        federateName,
        federateType,
        expectedModules,
        description + " known-object request");
  };

  auto const expectedModules = fomModuleNames(options.additionalFomModules);
  moduleSubject.join(
      moduleSubjectName,
      options.federateType,
      federation,
      expectedModules);
  waitFor(
      observer,
      [&] {
        auto const reflection = findReflectionFor(moduleSubject.federateHandle());
        return reflection.has_value() && observer.recorder().hasDiscovery(reflection->object);
      },
      options,
      "joined-federate MOM module-subject discovery and initial reflection");
  auto const moduleSubjectReflection = findReflectionFor(moduleSubject.federateHandle());
  require(
      moduleSubjectReflection.has_value() &&
          observer.rtiAmbassador().getKnownObjectClassHandle(
              moduleSubjectReflection->object) == momClass,
      "Joined-federate MOM module-subject discovery returned the wrong standard object class");
  auto const moduleSubjectObject = moduleSubjectReflection->object;
  verifyReflection(
      *moduleSubjectReflection,
      moduleSubject.federateHandle(),
      moduleSubjectName,
      options.federateType,
      expectedModules,
      "joined-federate MOM module-subject initial reflection");
  auto const moduleSubjectObjectName =
      observer.rtiAmbassador().getObjectInstanceName(moduleSubjectObject);
  require(
      !moduleSubjectObjectName.empty() &&
          observer.rtiAmbassador().getObjectInstanceHandle(moduleSubjectObjectName) ==
              moduleSubjectObject,
      "Joined-federate MOM module-subject object-name lookup did not round-trip");
  requestKnownValues(
      moduleSubjectObject,
      moduleSubject.federateHandle(),
      moduleSubjectName,
      options.federateType,
      expectedModules,
      "joined-federate MOM module-subject");

  noModuleSubject.join(
      noModuleSubjectName,
      options.federateType,
      federation);
  waitFor(
      observer,
      [&] {
        auto const reflection = findReflectionFor(noModuleSubject.federateHandle());
        return reflection.has_value() && observer.recorder().hasDiscovery(reflection->object);
      },
      options,
      "joined-federate MOM no-module subject discovery and initial reflection");
  auto const noModuleSubjectReflection = findReflectionFor(noModuleSubject.federateHandle());
  require(
      noModuleSubjectReflection.has_value() &&
          noModuleSubjectReflection->object != moduleSubjectObject &&
          observer.rtiAmbassador().getKnownObjectClassHandle(
              noModuleSubjectReflection->object) == momClass,
      "Joined-federate MOM no-module subject did not receive a distinct standard object");
  auto const noModuleSubjectObject = noModuleSubjectReflection->object;
  verifyReflection(
      *noModuleSubjectReflection,
      noModuleSubject.federateHandle(),
      noModuleSubjectName,
      options.federateType,
      {},
      "joined-federate MOM no-module subject initial reflection");
  auto const noModuleSubjectObjectName =
      observer.rtiAmbassador().getObjectInstanceName(noModuleSubjectObject);
  require(
      !noModuleSubjectObjectName.empty() &&
          observer.rtiAmbassador().getObjectInstanceHandle(noModuleSubjectObjectName) ==
              noModuleSubjectObject,
      "Joined-federate MOM no-module object-name lookup did not round-trip");
  requestKnownValues(
      noModuleSubjectObject,
      noModuleSubject.federateHandle(),
      noModuleSubjectName,
      options.federateType,
      {},
      "joined-federate MOM no-module subject");

  moduleSubject.resign(rti::NO_ACTION);
  waitFor(
      observer,
      [&] {
        auto const removals = observer.recorder().removals();
        return std::any_of(
            removals.begin(),
            removals.end(),
            [&](RemovalRecord const& removal) {
              return removal.object == moduleSubjectObject;
            });
      },
      options,
      "joined-federate MOM module-subject removal");
  noModuleSubject.resign(rti::NO_ACTION);
  waitFor(
      observer,
      [&] {
        auto const removals = observer.recorder().removals();
        return std::any_of(
            removals.begin(),
            removals.end(),
            [&](RemovalRecord const& removal) {
              return removal.object == noModuleSubjectObject;
            });
      },
      options,
      "joined-federate MOM no-module subject removal");

  observer.rtiAmbassador().unsubscribeObjectClassAttributes(momClass, momAttributes);
  observer.resign(rti::NO_ACTION);
  moduleSubject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  noModuleSubject.disconnect();
  moduleSubject.disconnect();
}

void scenarioJoinedFederateMomObjectLifecycleContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioJoinedFederateMomObjectLifecycle(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
