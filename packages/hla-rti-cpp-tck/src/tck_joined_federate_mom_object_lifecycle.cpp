#include "tck_joined_federate_mom_object_lifecycle.hpp"
#include "tck_main_scenarios.hpp"

#include <RTI/encoding/BasicDataElements.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <mutex>
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
  auto const federateSubclassFom = options.scenarioFoms.find(
      "mom-federate-subclass-instance");
  require(
      federateSubclassFom != options.scenarioFoms.end() &&
          !federateSubclassFom->second.empty(),
      "Joined-federate MOM object lifecycle requires the HLAfederate subclass FOM");
  std::vector<std::wstring> const federationFomModules{
      options.fom.wstring(),
      federateSubclassFom->second.wstring()};

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
        federationFomModules,
        options.mimFom.wstring());
  } else {
    moduleSubject.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        federationFomModules,
        options.mimFom.wstring(),
        options.logicalTimeImplementationName);
  }

  auto const observerName = options.memberFederateName + L"-mom-lifecycle-observer";
  auto const moduleSubjectName = options.ownerFederateName + L"-mom-lifecycle-module-subject";
  auto const noModuleSubjectName = options.memberFederateName + L"-mom-lifecycle-no-module";
  observer.join(observerName, options.federateType, federation);
  auto const momClass = observer.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateSubclass = observer.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate.TckFederateExtension");
  auto const federateSubclassValueAttribute =
      observer.rtiAmbassador().getAttributeHandle(
          federateSubclass,
          L"TckExtensionValue");
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
  auto const federateDimension = observer.rtiAmbassador().getDimensionHandle(
      L"HLAfederate");
  rti::AttributeHandleSet const momAttributes{
      federateHandleAttribute,
      federateNameAttribute,
      federateTypeAttribute,
      federateHostAttribute,
      fomModuleListAttribute};
  std::vector<std::wstring> const table8PublishedNames{
      L"HLAfederateHandle",
      L"HLAfederateName",
      L"HLAfederateType",
      L"HLAfederateHost",
      L"HLARTIversion",
      L"HLAFOMmoduleDesignatorList",
      L"HLAtimeConstrained",
      L"HLAtimeRegulating",
      L"HLAasynchronousDelivery",
      L"HLAfederateState",
      L"HLAtimeManagerState",
      L"HLAlogicalTime",
      L"HLAlookahead",
      L"HLAGALT",
      L"HLALITS",
      L"HLAROlength",
      L"HLATSOlength",
      L"HLAreflectionsReceived",
      L"HLAupdatesSent",
      L"HLAinteractionsReceived",
      L"HLAinteractionsSent",
      L"HLAdirectedInteractionsReceived",
      L"HLAdirectedInteractionsSent",
      L"HLAobjectInstancesThatCanBeDeleted",
      L"HLAobjectInstancesUpdated",
      L"HLAobjectInstancesReflected",
      L"HLAobjectInstancesDeleted",
      L"HLAobjectInstancesRemoved",
      L"HLAobjectInstancesRegistered",
      L"HLAobjectInstancesDiscovered",
      L"HLAtimeGrantedTime",
      L"HLAtimeAdvancingTime",
      L"HLAobjectClassRelevanceAdvisory",
      L"HLAattributeRelevanceAdvisory",
      L"HLAattributeScopeAdvisory",
      L"HLAinteractionRelevanceAdvisory",
      L"HLAconveyRegionDesignatorSets",
      L"HLAautomaticResignAction",
      L"HLAserviceReporting",
      L"HLAexceptionReporting",
      L"HLAsendServiceReportsToFile",
      L"HLAreportServiceFile",
  };
  rti::AttributeHandleSet table8PublishedAttributes;
  for (auto const& name : table8PublishedNames) {
    auto const attribute = observer.rtiAmbassador().getAttributeHandle(
        momClass,
        name);
    require(
        attribute.isValid() &&
            observer.rtiAmbassador().getAttributeName(momClass, attribute) ==
                name,
        "Joined-federate Table 8 attribute lookup did not round-trip: " +
            std::string(name.begin(), name.end()));
    table8PublishedAttributes.insert(attribute);
  }
  require(
      momClass.isValid() && federateSubclass.isValid() &&
          federateSubclassValueAttribute.isValid() &&
          federateHandleAttribute.isValid() &&
          federateNameAttribute.isValid() && federateTypeAttribute.isValid() &&
          federateHostAttribute.isValid() &&
          fomModuleListAttribute.isValid() && reliable.isValid() &&
          federateDimension.isValid() &&
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
          observer.rtiAmbassador().getObjectClassName(federateSubclass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederate.TckFederateExtension" &&
          observer.rtiAmbassador().getAttributeName(
              federateSubclass,
              federateSubclassValueAttribute) == L"TckExtensionValue" &&
          observer.rtiAmbassador().getTransportationTypeName(reliable) == L"HLAreliable" &&
          observer.rtiAmbassador().getDimensionName(federateDimension) == L"HLAfederate",
      "Joined-federate MOM object lifecycle did not resolve the standard MIM handles");
  require(
      table8PublishedAttributes.size() == table8PublishedNames.size(),
      "Joined-federate Table 8 did not resolve 42 distinct P-designated attributes");
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      momClass,
      table8PublishedAttributes,
      true,
      L"");
  rti::AttributeHandleSet federateSubclassAttributes = table8PublishedAttributes;
  federateSubclassAttributes.insert(federateSubclassValueAttribute);
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      federateSubclass,
      federateSubclassAttributes,
      true,
      L"");
  observer.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  require(
      observer.rtiAmbassador().getConveyRegionDesignatorSetsSwitch(),
      "Joined-federate MOM test did not enable region-designator callback metadata");
  struct Table8RegionPointObservation final {
    rti::ObjectInstanceHandle object;
    unsigned long lowerBound = 0;
    unsigned long upperBound = 0;
  };
  std::vector<Table8RegionPointObservation> table8RegionPoints;
  std::mutex table8RegionPointsMutex;
  observer.recorder().setDetailedReflectionHook(
      [&](rti::ObjectInstanceHandle const& object,
          rti::AttributeHandleValueMap const&,
          rti::RegionHandleSet const* regions) {
        require(
            regions != nullptr && regions->size() == 1U,
            "Joined-federate MOM reflection omitted its single update region");
        auto const dimensionHandles = observer.rtiAmbassador().getDimensionHandleSet(
            *regions->begin());
        require(
            dimensionHandles == rti::DimensionHandleSet{federateDimension},
            "Joined-federate MOM update region specified dimensions other than HLAfederate");
        auto const bounds = observer.rtiAmbassador().getRangeBounds(
            *regions->begin(),
            federateDimension);
        require(
            bounds.getUpperBound() > bounds.getLowerBound(),
            "Joined-federate MOM HLAfederate update region was empty");
        std::lock_guard<std::mutex> lock(table8RegionPointsMutex);
        table8RegionPoints.push_back(Table8RegionPointObservation{
            object,
            bounds.getLowerBound(),
            bounds.getUpperBound()});
      });

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
  auto objectInstancesFor = [&](rti::FederateHandle const& federate) {
    auto const expectedHandle = copyBytes(federate.encode());
    std::vector<rti::ObjectInstanceHandle> objects;
    for (auto const& reflection : observer.recorder().reflections()) {
      auto const value = reflection.values.find(federateHandleAttribute);
      if (value == reflection.values.end() ||
          copyBytes(value->second) != expectedHandle) {
        continue;
      }
      if (std::find(objects.begin(), objects.end(), reflection.object) ==
          objects.end()) {
        objects.push_back(reflection.object);
      }
    }
    return objects;
  };
  auto verifyOneMomObjectPerFederate = [&](rti::FederateHandle const& federate,
                                            rti::ObjectInstanceHandle const& expectedObject,
                                            std::string const& description) {
    auto const objects = objectInstancesFor(federate);
    require(
        objects.size() == 1U && objects.front() == expectedObject,
        description +
            " did not have exactly one distinct RTI-registered HLAfederate MOM object");
    require(
        observer.rtiAmbassador().getKnownObjectClassHandle(objects.front()) == momClass,
        description + " MOM object did not use the standard HLAfederate class");
  };
  auto verifyReflection = [&](ReflectionRecord const& reflection,
                              rti::FederateHandle const& federate,
                              std::wstring const& federateName,
                              std::wstring const& federateType,
                              std::vector<std::wstring> const& expectedModules,
                              std::string const& description) {
    std::string missingMimAttributes;
    for (auto const& attribute : momAttributes) {
      if (reflection.values.count(attribute) == 0U) {
        auto const name = observer.rtiAmbassador().getAttributeName(momClass, attribute);
        if (!missingMimAttributes.empty()) missingMimAttributes += ", ";
        missingMimAttributes += std::string(name.begin(), name.end());
      }
    }
    require(
        missingMimAttributes.empty(),
        description + " omitted MIM attributes: " + missingMimAttributes);
    require(
        reflection.transportation == reliable,
        description +
            " returned a transportation handle other than the federation's HLAreliable handle");
    require(
        reflection.tag.empty(),
        description + " supplied a user tag with an instance attribute update");
    require(
        reflection.regions.has_value() && reflection.regions->size() == 1U,
        description +
            " did not report exactly one region for the joined-federate MOM update");
    require(
        copyBytes(reflection.values.at(federateHandleAttribute)) ==
                copyBytes(federate.encode()) &&
            decodeUnicodeString(reflection.values.at(federateNameAttribute)) == federateName &&
            decodeUnicodeString(reflection.values.at(federateTypeAttribute)) == federateType,
        description + " reflected a different federate handle, name, or type");
    auto const reflectedModules =
        decodeUnicodeStringList(reflection.values.at(fomModuleListAttribute));
    if (reflectedModules != expectedModules) {
      auto formatModuleNames = [](std::vector<std::wstring> const& names) {
        std::string result;
        for (auto const& name : names) {
          if (!result.empty()) {
            result += ", ";
          }
          result += std::string(name.begin(), name.end());
        }
        return result;
      };
      require(
          false,
          description + " reflected different FOM designators (expected [" +
              formatModuleNames(expectedModules) + "] received [" +
              formatModuleNames(reflectedModules) + "])");
    }
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
    auto const timedBefore = observer.recorder().timedReflections().size();
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
    require(
        observer.recorder().timedReflections().size() == timedBefore,
        description + " delivered an instance attribute update through a timestamped callback");
    verifyReflection(
        *iterator,
        federate,
        federateName,
        federateType,
        expectedModules,
        description + " known-object request");
  };
  auto requestTable8Values = [&](rti::ObjectInstanceHandle const& object,
                                 std::string const& description) {
    auto const before = observer.recorder().reflections().size();
    auto const timedBefore = observer.recorder().timedReflections().size();
    observer.rtiAmbassador().requestAttributeValueUpdate(
        object,
        table8PublishedAttributes,
        rti::VariableLengthData{});
    auto hasCompletePublishedSet = [&](ReflectionRecord const& reflection) {
      return reflection.object == object &&
          std::all_of(
              table8PublishedAttributes.begin(),
              table8PublishedAttributes.end(),
              [&](rti::AttributeHandle const& attribute) {
                return reflection.values.count(attribute) == 1U;
              });
    };
    waitFor(
        observer,
        [&] {
          auto const reflections = observer.recorder().reflections();
          return std::any_of(
              reflections.begin() + static_cast<std::ptrdiff_t>(before),
              reflections.end(),
              hasCompletePublishedSet);
        },
        options,
        description + " complete Table 8 P-attribute request");
    auto const reflections = observer.recorder().reflections();
    auto const iterator = std::find_if(
        reflections.begin() + static_cast<std::ptrdiff_t>(before),
        reflections.end(),
        hasCompletePublishedSet);
    require(
        iterator != reflections.end() &&
            iterator->values.size() == table8PublishedAttributes.size() &&
            iterator->transportation == reliable && iterator->tag.empty() &&
            !iterator->producer.isValid() && iterator->regions.has_value() &&
            iterator->regions->size() == 1U &&
            observer.recorder().timedReflections().size() == timedBefore,
        description +
            " Table 8 request omitted a P value, reported timestamp/tag metadata, or did not report exactly one update region");
  };

  auto const expectedModules = fomModuleNames(options.additionalFomModules);
  auto const timedModuleJoinReflections = observer.recorder().timedReflections().size();
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
  require(
      observer.recorder().timedReflections().size() == timedModuleJoinReflections,
      "Joined-federate MOM module-subject initial attribute update used a timestamped callback");
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
  requestTable8Values(
      moduleSubjectObject,
      "joined-federate MOM module-subject");

  auto const table8ClassRequestReflectionStart =
      observer.recorder().reflections().size();
  auto const table8ClassRequestTimedReflectionStart =
      observer.recorder().timedReflections().size();
  observer.rtiAmbassador().requestAttributeValueUpdate(
      momClass,
      table8PublishedAttributes,
      rti::VariableLengthData{});
  auto isCompleteModuleSubjectTable8Reflection =
      [&](ReflectionRecord const& reflection) {
        return reflection.object == moduleSubjectObject &&
            reflection.values.size() == table8PublishedAttributes.size() &&
            std::all_of(
                table8PublishedAttributes.begin(),
                table8PublishedAttributes.end(),
                [&](rti::AttributeHandle const& attribute) {
                  return reflection.values.count(attribute) == 1U;
                });
      };
  waitFor(
      observer,
      [&] {
        auto const reflections = observer.recorder().reflections();
        return std::any_of(
            reflections.begin() + static_cast<std::ptrdiff_t>(
                                     table8ClassRequestReflectionStart),
            reflections.end(),
            isCompleteModuleSubjectTable8Reflection);
      },
      options,
      "joined-federate MOM class-wide Table 8 request reflection");
  auto const table8ClassRequestReflections = observer.recorder().reflections();
  auto const table8ClassRequestReflection = std::find_if(
      table8ClassRequestReflections.begin() + static_cast<std::ptrdiff_t>(
                                                  table8ClassRequestReflectionStart),
      table8ClassRequestReflections.end(),
      isCompleteModuleSubjectTable8Reflection);
  require(
      table8ClassRequestReflection != table8ClassRequestReflections.end() &&
          table8ClassRequestReflection->transportation == reliable &&
          table8ClassRequestReflection->tag.empty() &&
          !table8ClassRequestReflection->producer.isValid() &&
          table8ClassRequestReflection->regions.has_value() &&
          table8ClassRequestReflection->regions->size() == 1U &&
          observer.recorder().timedReflections().size() ==
              table8ClassRequestTimedReflectionStart,
      "Joined-federate MOM class-wide request did not return all Table 8 P values through one ordinary untimed reflection");

  auto const moduleSubjectFederateSubclass =
      moduleSubject.rtiAmbassador().getObjectClassHandle(
          L"HLAobjectRoot.HLAmanager.HLAfederate.TckFederateExtension");
  rti::AttributeHandleSet moduleSubjectFederateSubclassAttributes;
  for (auto const& name : table8PublishedNames) {
    auto const attribute = moduleSubject.rtiAmbassador().getAttributeHandle(
        moduleSubjectFederateSubclass,
        name);
    require(
        attribute.isValid(),
        "Joined federate could not resolve an inherited Table 8 attribute on its HLAfederate subclass");
    moduleSubjectFederateSubclassAttributes.insert(attribute);
  }
  auto const moduleSubjectFederateSubclassAttribute =
      moduleSubject.rtiAmbassador().getAttributeHandle(
          moduleSubjectFederateSubclass,
          L"TckExtensionValue");
  require(
      moduleSubjectFederateSubclass.isValid() &&
          moduleSubjectFederateSubclassAttribute.isValid(),
      "Joined federate could not resolve the published HLAfederate subclass or its test attribute");
  moduleSubjectFederateSubclassAttributes.insert(
      moduleSubjectFederateSubclassAttribute);
  moduleSubject.rtiAmbassador().publishObjectClassAttributes(
      moduleSubjectFederateSubclass,
      moduleSubjectFederateSubclassAttributes);
  auto const extensionInheritedReflectionStart =
      observer.recorder().reflections().size();
  auto const moduleSubjectExtensionObject =
      moduleSubject.rtiAmbassador().registerObjectInstance(
          federateSubclass);
  require(
      moduleSubjectExtensionObject.isValid() &&
          moduleSubjectExtensionObject != moduleSubjectObject,
      "Joined federate's subclass instance reused the RTI-registered HLAfederate MOM object handle");
  waitFor(
      observer,
      [&] { return observer.recorder().hasDiscovery(moduleSubjectExtensionObject); },
      options,
      "joined-federate HLAfederate subclass instance discovery");
  auto const moduleSubjectExtensionObjectName =
      observer.rtiAmbassador().getObjectInstanceName(moduleSubjectExtensionObject);
  require(
      observer.rtiAmbassador().getKnownObjectClassHandle(moduleSubjectObject) == momClass &&
          observer.rtiAmbassador().getKnownObjectClassHandle(
              moduleSubjectExtensionObject) == federateSubclass &&
          moduleSubjectExtensionObject != moduleSubjectObject &&
          !moduleSubjectExtensionObjectName.empty() &&
          observer.rtiAmbassador().getObjectInstanceHandle(
              moduleSubjectExtensionObjectName) == moduleSubjectExtensionObject,
      "FOM-added HLAfederate subclass did not remain a separately discoverable object instance from the RTI's MOM object");

  auto const timedNoModuleJoinReflections = observer.recorder().timedReflections().size();
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
  require(
      observer.recorder().timedReflections().size() == timedNoModuleJoinReflections,
      "Joined-federate MOM no-module initial attribute update used a timestamped callback");
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
  requestTable8Values(
      noModuleSubjectObject,
      "joined-federate MOM no-module subject");
  if (model == rti::HLA_EVOKED) {
    for (std::size_t attempt = 0U; attempt != 64U; ++attempt) {
      if (!observer.rtiAmbassador().evokeMultipleCallbacks(0.0, 0.0)) {
        break;
      }
    }
  }
  auto const subclassUpdateReflections = observer.recorder().reflections();
  auto const inheritedSubclassUpdate = std::find_if(
      subclassUpdateReflections.begin() + static_cast<std::ptrdiff_t>(
                                              extensionInheritedReflectionStart),
      subclassUpdateReflections.end(),
      [&](ReflectionRecord const& reflection) {
        if (reflection.object != moduleSubjectExtensionObject) {
          return false;
        }
        return std::any_of(
            table8PublishedAttributes.begin(),
            table8PublishedAttributes.end(),
            [&](rti::AttributeHandle const& attribute) {
              return reflection.values.count(attribute) != 0U;
            });
      });
  require(
      inheritedSubclassUpdate == subclassUpdateReflections.end(),
      "RTI updated inherited predefined attributes on a federate-registered HLAfederate subclass instance");

  std::vector<Table8RegionPointObservation> regionPointSnapshot;
  {
    std::lock_guard<std::mutex> lock(table8RegionPointsMutex);
    regionPointSnapshot = table8RegionPoints;
  }
  auto const moduleSubjectNormalized = observer.rtiAmbassador().normalizeFederateHandle(
      moduleSubject.federateHandle());
  auto const noModuleSubjectNormalized = observer.rtiAmbassador().normalizeFederateHandle(
      noModuleSubject.federateHandle());
  auto const observerSubjectNormalized = observer.rtiAmbassador().normalizeFederateHandle(
      observer.federateHandle());
  require(
      moduleSubjectNormalized != std::numeric_limits<unsigned long>::max() &&
          noModuleSubjectNormalized != std::numeric_limits<unsigned long>::max() &&
          observerSubjectNormalized != std::numeric_limits<unsigned long>::max(),
      "Normalize Federate Handle returned a value with no representable point-range upper bound");
  waitFor(
      observer,
      [&] {
        auto const reflection = findReflectionFor(observer.federateHandle());
        return reflection.has_value() &&
            observer.recorder().hasDiscovery(reflection->object);
      },
      options,
      "joined-federate MOM observer instance discovery and initial reflection");
  auto const observerSubjectReflection = findReflectionFor(observer.federateHandle());
  require(
      observerSubjectReflection.has_value(),
      "Joined-federate MOM observer did not receive its RTI-registered object");
  verifyOneMomObjectPerFederate(
      moduleSubject.federateHandle(),
      moduleSubjectObject,
      "Joined-federate MOM module subject");
  verifyOneMomObjectPerFederate(
      noModuleSubject.federateHandle(),
      noModuleSubjectObject,
      "Joined-federate MOM no-module subject");
  verifyOneMomObjectPerFederate(
      observer.federateHandle(),
      observerSubjectReflection->object,
      "Joined-federate MOM observer");
  auto matchesNormalizedPoint = [&](Table8RegionPointObservation const& observation,
                                   rti::ObjectInstanceHandle const& object,
                                   unsigned long normalizedHandle) {
    return normalizedHandle != std::numeric_limits<unsigned long>::max() &&
        observation.object == object &&
        observation.lowerBound == normalizedHandle &&
        observation.upperBound == normalizedHandle + 1UL;
  };
  std::string regionPointEvidence;
  for (auto const& observation : regionPointSnapshot) {
    regionPointEvidence += " range=[" + std::to_string(observation.lowerBound) + "," +
        std::to_string(observation.upperBound) + ");";
  }
  auto const normalizedPointRangesMatch = !regionPointSnapshot.empty() &&
      std::all_of(
          regionPointSnapshot.begin(),
          regionPointSnapshot.end(),
          [&](Table8RegionPointObservation const& observation) {
            return matchesNormalizedPoint(
                       observation,
                       moduleSubjectObject,
                       moduleSubjectNormalized) ||
                matchesNormalizedPoint(
                    observation,
                    noModuleSubjectObject,
                        noModuleSubjectNormalized) ||
                    (observerSubjectReflection.has_value() &&
                     matchesNormalizedPoint(
                         observation,
                         observerSubjectReflection->object,
                         observerSubjectNormalized));
          }) &&
      std::any_of(
          regionPointSnapshot.begin(),
          regionPointSnapshot.end(),
          [&](Table8RegionPointObservation const& observation) {
            return matchesNormalizedPoint(
                observation,
                moduleSubjectObject,
                moduleSubjectNormalized);
          }) &&
      std::any_of(
          regionPointSnapshot.begin(),
          regionPointSnapshot.end(),
          [&](Table8RegionPointObservation const& observation) {
            return matchesNormalizedPoint(
                observation,
                noModuleSubjectObject,
                noModuleSubjectNormalized);
          });
  require(
      normalizedPointRangesMatch,
      "Joined-federate MOM update regions were not half-open unit point ranges [normalized HLAfederateHandle, normalized HLAfederateHandle + 1):" +
          regionPointEvidence + " expected=[" +
          std::to_string(moduleSubjectNormalized) + "," +
          std::to_string(moduleSubjectNormalized + 1UL) + ") and [" +
          std::to_string(noModuleSubjectNormalized) + "," +
          std::to_string(noModuleSubjectNormalized + 1UL) + ")");

  auto const timedModuleResignRemovals = observer.recorder().timedRemovals().size();
  moduleSubject.resign(rti::DELETE_OBJECTS);
  waitFor(
      observer,
      [&] {
        auto const removals = observer.recorder().removals();
        auto const timedRemovals = observer.recorder().timedRemovals();
        auto hasRemoval = [&](rti::ObjectInstanceHandle const& object) {
          return std::any_of(
                     removals.begin(),
                     removals.end(),
                     [&](RemovalRecord const& removal) {
                       return removal.object == object;
                     }) ||
              std::any_of(
                     timedRemovals.begin(),
                     timedRemovals.end(),
                     [&](TimedRemovalRecord const& removal) {
                       return removal.object == object;
                     });
        };
        return hasRemoval(moduleSubjectObject) &&
            hasRemoval(moduleSubjectExtensionObject);
      },
      options,
      "joined-federate MOM module-subject and subclass-object removal");
  auto const moduleSubjectRemovals = observer.recorder().removals();
  auto const moduleSubjectRemoval = std::find_if(
      moduleSubjectRemovals.begin(),
      moduleSubjectRemovals.end(),
      [&](RemovalRecord const& removal) {
        return removal.object == moduleSubjectObject;
      });
  auto const moduleSubjectExtensionRemoval = std::find_if(
      moduleSubjectRemovals.begin(),
      moduleSubjectRemovals.end(),
      [&](RemovalRecord const& removal) {
        return removal.object == moduleSubjectExtensionObject;
      });
  require(
      moduleSubjectRemoval != moduleSubjectRemovals.end() &&
          moduleSubjectExtensionRemoval != moduleSubjectRemovals.end() &&
          moduleSubjectRemoval->tag.empty() &&
          moduleSubjectExtensionRemoval->tag.empty() &&
          observer.recorder().timedRemovals().size() == timedModuleResignRemovals,
      "Joined-federate MOM or subclass-object deletion supplied a tag or timestamp");
  auto const timedNoModuleResignRemovals = observer.recorder().timedRemovals().size();
  noModuleSubject.resign(rti::NO_ACTION);
  waitFor(
      observer,
      [&] {
        auto const removals = observer.recorder().removals();
        auto const timedRemovals = observer.recorder().timedRemovals();
        return std::any_of(
            removals.begin(),
            removals.end(),
            [&](RemovalRecord const& removal) {
              return removal.object == noModuleSubjectObject;
            }) || std::any_of(
            timedRemovals.begin(),
            timedRemovals.end(),
            [&](TimedRemovalRecord const& removal) {
              return removal.object == noModuleSubjectObject;
            });
      },
      options,
      "joined-federate MOM no-module subject removal");
  auto const noModuleSubjectRemovals = observer.recorder().removals();
  auto const noModuleSubjectRemoval = std::find_if(
      noModuleSubjectRemovals.begin(),
      noModuleSubjectRemovals.end(),
      [&](RemovalRecord const& removal) {
        return removal.object == noModuleSubjectObject;
      });
  require(
      noModuleSubjectRemoval != noModuleSubjectRemovals.end() &&
          noModuleSubjectRemoval->tag.empty() &&
          observer.recorder().timedRemovals().size() == timedNoModuleResignRemovals,
      "Joined-federate MOM no-module deletion supplied a tag or timestamp");
  require(
      observer.recorder().timedReflections().empty() &&
          observer.recorder().timedRemovals().empty(),
      "Joined-federate MOM lifecycle used timestamped attribute-update or deletion callbacks");

  observer.rtiAmbassador().unsubscribeObjectClassAttributes(
      momClass,
      table8PublishedAttributes);
  observer.rtiAmbassador().unsubscribeObjectClassAttributes(
      federateSubclass,
      federateSubclassAttributes);
  observer.resign(rti::NO_ACTION);
  moduleSubject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  noModuleSubject.disconnect();
  moduleSubject.disconnect();

  // Pair the joined-federate object checks with the standard federation MOM
  // object's published Table 8 values in the same TCK scenario.
  verifyFederationMomTable8PublishedAttributes(options, model);
}

void scenarioJoinedFederateMomObjectLifecycleContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioJoinedFederateMomObjectLifecycle(options, model);
}

void scenarioJoinedFederateMomAddedAttributeSameRtiInstance(
    Options const& options,
    rti::CallbackModel model) {
  auto const addedAttributeFom = options.scenarioFoms.find(
      "mom-federate-added-attribute-same-instance");
  require(
      addedAttributeFom != options.scenarioFoms.end() &&
          !addedAttributeFom->second.empty(),
      "Joined-federate MOM added-attribute scenario requires its FOM module");
  require(
      !options.mimFom.empty(),
      "Joined-federate MOM added-attribute scenario requires the standard MIM");

  auto const federation = federationName(
      options,
      "joined-federate-mom-added-attribute-same-rti-instance");
  auto const targetName =
      options.ownerFederateName + L"-mom-added-attribute-target";
  auto const updaterName =
      options.memberFederateName + L"-mom-added-attribute-updater";
  auto const watcherName =
      options.memberFederateName + L"-mom-added-attribute-watcher";
  Session target(options, model, "owner");
  Session updater(options, model, "member");
  Session watcher(options, model, "member");
  target.connect();
  updater.connect();
  watcher.connect();

  std::vector<std::wstring> const federationFomModules{
      addedAttributeFom->second.wstring()};
  if (options.logicalTimeImplementationName.empty()) {
    target.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        federationFomModules,
        options.mimFom.wstring());
  } else {
    target.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        federationFomModules,
        options.mimFom.wstring(),
        options.logicalTimeImplementationName);
  }

  updater.join(updaterName, options.federateType, federation);
  watcher.join(watcherName, options.federateType, federation);
  auto const updaterMomClass = updater.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const updaterAddedAttribute = updater.rtiAmbassador().getAttributeHandle(
      updaterMomClass,
      L"TckFederateAddedValue");
  auto const updaterFederateHandleAttribute =
      updater.rtiAmbassador().getAttributeHandle(
          updaterMomClass,
          L"HLAfederateHandle");
  auto const watcherMomClass = watcher.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const watcherAddedAttribute = watcher.rtiAmbassador().getAttributeHandle(
      watcherMomClass,
      L"TckFederateAddedValue");
  auto const watcherFederateHandleAttribute =
      watcher.rtiAmbassador().getAttributeHandle(
          watcherMomClass,
          L"HLAfederateHandle");
  require(
      updaterMomClass.isValid() && updaterAddedAttribute.isValid() &&
          updaterFederateHandleAttribute.isValid() && watcherMomClass.isValid() &&
          watcherAddedAttribute.isValid() &&
          watcherFederateHandleAttribute.isValid() &&
          updater.rtiAmbassador().getAttributeName(
              updaterMomClass,
              updaterAddedAttribute) == L"TckFederateAddedValue" &&
          watcher.rtiAmbassador().getAttributeName(
              watcherMomClass,
              watcherAddedAttribute) == L"TckFederateAddedValue",
      "FOM-added HLAfederate attribute handles did not resolve through the public API");

  rti::AttributeHandleSet const updaterSubscribedAttributes{
      updaterAddedAttribute,
      updaterFederateHandleAttribute};
  rti::AttributeHandleSet const watcherSubscribedAttributes{
      watcherAddedAttribute,
      watcherFederateHandleAttribute};
  updater.rtiAmbassador().publishObjectClassAttributes(
      updaterMomClass,
      rti::AttributeHandleSet{updaterAddedAttribute});
  updater.rtiAmbassador().subscribeObjectClassAttributes(
      updaterMomClass,
      updaterSubscribedAttributes,
      true,
      L"");
  watcher.rtiAmbassador().subscribeObjectClassAttributes(
      watcherMomClass,
      watcherSubscribedAttributes,
      true,
      L"");

  target.join(targetName, options.federateType, federation);
  auto const targetFederateHandleBytes = copyBytes(target.federateHandle().encode());
  auto findTargetMomReflection = [&](Session& participant,
                                    rti::AttributeHandle const& handleAttribute)
      -> std::optional<ReflectionRecord> {
    auto const reflections = participant.recorder().reflections();
    auto const iterator = std::find_if(
        reflections.begin(),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          auto const value = reflection.values.find(handleAttribute);
          return value != reflection.values.end() &&
              copyBytes(value->second) == targetFederateHandleBytes;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return *iterator;
  };
  waitFor(
      updater,
      [&] {
        auto const reflection = findTargetMomReflection(
            updater,
            updaterFederateHandleAttribute);
        return reflection.has_value() &&
            updater.recorder().hasDiscovery(reflection->object);
      },
      options,
      "FOM-added attribute updater discovery of the RTI-registered target MOM object");
  waitFor(
      watcher,
      [&] {
        auto const reflection = findTargetMomReflection(
            watcher,
            watcherFederateHandleAttribute);
        return reflection.has_value() &&
            watcher.recorder().hasDiscovery(reflection->object);
      },
      options,
      "FOM-added attribute watcher discovery of the RTI-registered target MOM object");
  auto const updaterTargetReflection = findTargetMomReflection(
      updater,
      updaterFederateHandleAttribute);
  auto const watcherTargetReflection = findTargetMomReflection(
      watcher,
      watcherFederateHandleAttribute);
  require(
      updaterTargetReflection.has_value() && watcherTargetReflection.has_value() &&
          updater.rtiAmbassador().getKnownObjectClassHandle(
              updaterTargetReflection->object) == updaterMomClass &&
          watcher.rtiAmbassador().getKnownObjectClassHandle(
              watcherTargetReflection->object) == watcherMomClass,
      "The target federate's discovered object was not its RTI-registered HLAfederate MOM instance");
  auto const updaterTargetName = updater.rtiAmbassador().getObjectInstanceName(
      updaterTargetReflection->object);
  auto const watcherTargetName = watcher.rtiAmbassador().getObjectInstanceName(
      watcherTargetReflection->object);
  require(
      !updaterTargetName.empty() && updaterTargetName == watcherTargetName &&
          updater.rtiAmbassador().getObjectInstanceHandle(updaterTargetName) ==
              updaterTargetReflection->object &&
          watcher.rtiAmbassador().getObjectInstanceHandle(watcherTargetName) ==
              watcherTargetReflection->object,
      "Federates did not identify the same RTI-registered MOM instance through its object name");

  auto const ownershipNotOwnedStart =
      updater.recorder().ownershipNotOwnedEvents().size();
  auto const ownershipOwnedByRtiStart =
      updater.recorder().ownershipOwnedByRtiEvents().size();
  updater.rtiAmbassador().queryAttributeOwnership(
      updaterTargetReflection->object,
      rti::AttributeHandleSet{updaterAddedAttribute});
  waitFor(
      updater,
      [&] {
        return updater.recorder().ownershipNotOwnedEvents().size() >
                ownershipNotOwnedStart ||
            updater.recorder().ownershipOwnedByRtiEvents().size() >
                ownershipOwnedByRtiStart;
      },
      options,
      "ownership query for the unacquired FOM-added MOM attribute");
  auto const notOwnedBeforeAcquisition =
      updater.recorder().ownershipNotOwnedEvents();
  auto const ownedByRtiBeforeAcquisition =
      updater.recorder().ownershipOwnedByRtiEvents();
  require(
      notOwnedBeforeAcquisition.size() == ownershipNotOwnedStart + 1U &&
          notOwnedBeforeAcquisition.back().object == updaterTargetReflection->object &&
          notOwnedBeforeAcquisition.back().attributes ==
              rti::AttributeHandleSet{updaterAddedAttribute} &&
          ownedByRtiBeforeAcquisition.size() == ownershipOwnedByRtiStart,
      "RTI reported ownership of a FOM-added MOM class attribute before a federate acquired it");

  std::vector<std::uint8_t> const acquisitionTagBytes{0x11U, 0x22U, 0x33U};
  rti::VariableLengthData const acquisitionTag(
      acquisitionTagBytes.data(), acquisitionTagBytes.size());
  auto const acquisitionStart =
      updater.recorder().ownershipAcquisitionEvents().size();
  updater.rtiAmbassador().attributeOwnershipAcquisition(
      updaterTargetReflection->object,
      rti::AttributeHandleSet{updaterAddedAttribute},
      acquisitionTag);
  waitFor(
      updater,
      [&] {
        return updater.recorder().ownershipAcquisitionEvents().size() >
            acquisitionStart;
      },
      options,
      "acquisition of the FOM-added HLAfederate MOM class attribute");
  auto const acquisitions = updater.recorder().ownershipAcquisitionEvents();
  require(
      acquisitions.size() > acquisitionStart &&
          acquisitions[acquisitionStart].object == updaterTargetReflection->object &&
          acquisitions[acquisitionStart].attributes ==
              rti::AttributeHandleSet{updaterAddedAttribute} &&
          acquisitions[acquisitionStart].tag == acquisitionTagBytes &&
          updater.rtiAmbassador().isAttributeOwnedByFederate(
              updaterTargetReflection->object,
              updaterAddedAttribute),
      "FOM-added MOM attribute acquisition was not granted for the discovered RTI object");

  constexpr std::int32_t expectedAddedValue = 15162025;
  rti::AttributeHandleValueMap const updateValues{
      {updaterAddedAttribute,
       rti::HLAinteger32BE{expectedAddedValue}.encode()}};
  std::vector<std::uint8_t> const updateTagBytes{0x44U, 0x55U, 0x66U};
  rti::VariableLengthData const updateTag(
      updateTagBytes.data(), updateTagBytes.size());
  auto const watcherReflectionStart = watcher.recorder().reflections().size();
  updater.rtiAmbassador().updateAttributeValues(
      updaterTargetReflection->object,
      updateValues,
      updateTag);
  waitFor(
      watcher,
      [&] {
        auto const reflections = watcher.recorder().reflections();
        return std::any_of(
            reflections.begin() + static_cast<std::ptrdiff_t>(
                                      watcherReflectionStart),
            reflections.end(),
            [&](ReflectionRecord const& reflection) {
              return reflection.object == watcherTargetReflection->object &&
                  reflection.values.count(watcherAddedAttribute) == 1U;
            });
      },
      options,
      "FOM-added attribute reflection on the RTI-registered target MOM object");
  auto const watcherReflections = watcher.recorder().reflections();
  auto const addedAttributeReflection = std::find_if(
      watcherReflections.begin() + static_cast<std::ptrdiff_t>(
                                       watcherReflectionStart),
      watcherReflections.end(),
      [&](ReflectionRecord const& reflection) {
        return reflection.object == watcherTargetReflection->object &&
            reflection.values.count(watcherAddedAttribute) == 1U;
      });
  require(
      addedAttributeReflection != watcherReflections.end() &&
          watcher.rtiAmbassador().getKnownObjectClassHandle(
              addedAttributeReflection->object) == watcherMomClass &&
          watcher.rtiAmbassador().getObjectInstanceName(
              addedAttributeReflection->object) == watcherTargetName &&
          addedAttributeReflection->tag == updateTagBytes,
      "FOM-added attribute update was not observed on the target's RTI-registered HLAfederate instance");
  rti::HLAinteger32BE decodedAddedValue;
  decodedAddedValue.decode(
      addedAttributeReflection->values.at(watcherAddedAttribute));
  require(
      decodedAddedValue.get() == expectedAddedValue,
      "FOM-added HLAfederate attribute update carried the wrong value");

  updater.rtiAmbassador().unsubscribeObjectClassAttributes(
      updaterMomClass,
      updaterSubscribedAttributes);
  watcher.rtiAmbassador().unsubscribeObjectClassAttributes(
      watcherMomClass,
      watcherSubscribedAttributes);
  updater.resign(rti::NO_ACTION);
  watcher.resign(rti::NO_ACTION);
  target.resign(rti::NO_ACTION);
  target.rtiAmbassador().destroyFederationExecution(federation);
  updater.disconnect();
  watcher.disconnect();
  target.disconnect();
}

void scenarioJoinedFederateMomAddedAttributeSameRtiInstanceContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioJoinedFederateMomAddedAttributeSameRtiInstance(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
