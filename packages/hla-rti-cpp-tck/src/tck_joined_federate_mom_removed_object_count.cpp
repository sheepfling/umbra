#include "tck_joined_federate_mom_removed_object_count.hpp"
#include "tck_main_scenarios.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioJoinedFederateMomRemovedObjectCount(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty(),
      "Joined-federate MOM removed-object testing requires an adapter-supplied standard MIM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Joined-federate MOM removed-object testing requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "mom-removed-publisher");
  Session observer(options, model, "mom-removed-observer");
  auto const federation = federationName(options, "joined-federate-mom-removed-object-count");
  publisher.connect();
  observer.connect();
  publisher.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(
      options.ownerFederateName + L"-mom-removed-publisher",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-mom-removed-observer",
      options.federateType,
      federation);

  auto const publisherFederateClass = publisher.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const observerFederateClass = observer.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  require(
      publisherFederateClass.isValid() && observerFederateClass.isValid() &&
          publisherFederateClass == observerFederateClass &&
          observer.rtiAmbassador().getObjectClassName(observerFederateClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederate",
      "Joined-federate MOM removed-object lookup did not round-trip the standard class");

  auto const observerFederateHandleAttribute = observer.rtiAmbassador().getAttributeHandle(
      observerFederateClass,
      L"HLAfederateHandle");
  auto const observerRemovedCountAttribute = observer.rtiAmbassador().getAttributeHandle(
      observerFederateClass,
      L"HLAobjectInstancesRemoved");
  auto const publisherFederateHandleAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherFederateClass,
      L"HLAfederateHandle");
  auto const publisherRemovedCountAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherFederateClass,
      L"HLAobjectInstancesRemoved");
  require(
      observerFederateHandleAttribute.isValid() && observerRemovedCountAttribute.isValid() &&
          publisherFederateHandleAttribute == observerFederateHandleAttribute &&
          publisherRemovedCountAttribute == observerRemovedCountAttribute &&
          observer.rtiAmbassador().getAttributeName(
              observerFederateClass,
              observerRemovedCountAttribute) == L"HLAobjectInstancesRemoved",
      "Joined-federate MOM removed-object lookup returned inconsistent standard attributes");

  auto const reliable = observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable");
  require(
      reliable.isValid() &&
          observer.rtiAmbassador().getTransportationTypeName(reliable) == L"HLAreliable",
      "Joined-federate MOM removed-object lookup could not resolve HLAreliable");
  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Joined-federate MOM removed-object service-report lookup returned an invalid HLAreportServiceInvocation handle");
  std::vector<rti::ParameterHandle> reportParameters;
  for (auto const& name : {
           L"HLAservice",
           L"HLAserviceType",
           L"HLAsuccessIndicator",
           L"HLAsuppliedArguments",
           L"HLAreturnedArgument",
           L"HLAexception",
           L"HLAserialNumber"}) {
    auto const parameter = observer.rtiAmbassador().getParameterHandle(reportClass, name);
    require(
        parameter.isValid(),
        "Joined-federate MOM removed-object service-report parameter lookup returned an invalid handle");
    reportParameters.push_back(parameter);
  }
  auto const publisherReportClass = publisher.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      publisherReportClass.isValid() && publisherReportClass == reportClass,
      "Joined-federate MOM removed-object publisher report lookup did not match the standard HLAreportServiceInvocation handle");
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  publisher.rtiAmbassador().subscribeInteractionClass(publisherReportClass);
  rti::AttributeHandleSet const momAttributes{
      observerFederateHandleAttribute,
      observerRemovedCountAttribute};
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      observerFederateClass,
      momAttributes,
      true,
      L"");

  auto findObserverMomReflection = [&]() -> std::optional<ReflectionRecord> {
    auto const reflections = observer.recorder().reflections();
    auto const expectedFederateHandle = copyBytes(observer.federateHandle().encode());
    auto const iterator = std::find_if(
        reflections.begin(),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          auto const value = reflection.values.find(observerFederateHandleAttribute);
          return reflection.present && value != reflection.values.end() &&
              copyBytes(value->second) == expectedFederateHandle;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return *iterator;
  };
  waitFor(
      observer,
      [&] { return findObserverMomReflection().has_value(); },
      options,
      "joined-federate MOM removed-object observer object discovery");
  auto const observerMomReflection = findObserverMomReflection();
  require(
      observerMomReflection.has_value() &&
          observer.rtiAmbassador().getKnownObjectClassHandle(
              observerMomReflection->object) == observerFederateClass,
      "Joined-federate MOM removed-object discovery returned the wrong standard object");
  auto const observerMomObject = observerMomReflection->object;

  // A joined-federate MOM object is RTI-owned. Query one of its standard
  // attributes to exercise the distinct IEEE callback rather than treating
  // RTI ownership as an unowned or synthetic-federate result.
  observer.recorder().clearOwnershipRecords();
  observer.rtiAmbassador().queryAttributeOwnership(
      observerMomObject,
      rti::AttributeHandleSet{observerFederateHandleAttribute});
  waitFor(
      observer,
      [&] { return observer.recorder().ownershipOwnedByRti().has_value(); },
      options,
      "joined-federate MOM RTI-owned attribute query");
  auto const rtiOwned = observer.recorder().ownershipOwnedByRti();
  require(
      rtiOwned->object == observerMomObject &&
          rtiOwned->attributes ==
              rti::AttributeHandleSet{observerFederateHandleAttribute},
      "Joined-federate MOM ownership query did not report RTI ownership");

  auto requestRemovedCount = [&](std::int32_t expected, std::string const& description) {
    auto const before = observer.recorder().reflections().size();
    observer.rtiAmbassador().requestAttributeValueUpdate(
        observerMomObject,
        rti::AttributeHandleSet{observerRemovedCountAttribute},
        rti::VariableLengthData{});
    waitFor(
        observer,
        [&] {
          auto const reflections = observer.recorder().reflections();
          return reflections.size() > before &&
              std::any_of(
                  reflections.begin() + static_cast<std::vector<ReflectionRecord>::difference_type>(before),
                  reflections.end(),
                  [&](ReflectionRecord const& reflection) {
                    return reflection.object == observerMomObject &&
                        reflection.values.count(observerRemovedCountAttribute) == 1U;
                  });
        },
        options,
        description);
    auto const reflections = observer.recorder().reflections();
    auto const iterator = std::find_if(
        reflections.begin() + static_cast<std::vector<ReflectionRecord>::difference_type>(before),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          return reflection.object == observerMomObject &&
              reflection.values.count(observerRemovedCountAttribute) == 1U;
        });
    require(iterator != reflections.end(), description + " produced no removed-object value");
    auto const& reflection = *iterator;
    require(
        reflection.values.size() == 1U && reflection.transportation == reliable &&
            reflection.tag.empty() && !reflection.producer.isValid() &&
            !reflection.regions.has_value(),
        description + " returned non-standard MOM reflection metadata");
    rti::HLAinteger32BE decoded;
    decoded.decode(reflection.values.at(observerRemovedCountAttribute));
    require(decoded.get() == expected, description + " returned the wrong removed-object count");
  };

  requestRemovedCount(0, "joined-federate MOM removed-object initial value request");

  auto const publisherObjectClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const observerObjectClass = observer.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherObjectClass,
      options.attributeName);
  auto const observerAttribute = observer.rtiAmbassador().getAttributeHandle(
      observerObjectClass,
      options.attributeName);
  require(
      publisherObjectClass.isValid() && observerObjectClass.isValid() &&
          publisherAttribute.isValid() && observerAttribute.isValid(),
      "Joined-federate MOM removed-object application lookup returned an invalid handle");
  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const observerAttributes{observerAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherObjectClass,
      publisherAttributes);
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      observerObjectClass,
      observerAttributes,
      true,
      L"");
  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherObjectClass);
  require(object.isValid(), "Joined-federate MOM removed-object registration returned an invalid handle");
  waitFor(
      observer,
      [&] { return observer.recorder().hasDiscovery(object); },
      options,
      "joined-federate MOM removed-object application discovery");
  auto const objectName = publisher.rtiAmbassador().getObjectInstanceName(object);
  observer.recorder().clearRemovals();
  std::vector<std::uint8_t> const deletionTagBytes{0x4DU, 0x4FU, 0x4DU};
  rti::VariableLengthData const deletionTag(
      deletionTagBytes.data(),
      deletionTagBytes.size());
  publisher.rtiAmbassador().deleteObjectInstance(object, deletionTag);
  waitFor(
      observer,
      [&] {
        return observer.recorder().hasRemoval(
            object,
            deletionTagBytes,
            publisher.federateHandle());
      },
      options,
      "joined-federate MOM removed-object application removal");
  auto const removals = observer.recorder().removals();
  require(
      removals.size() == 1U && removals.front().object == object &&
          removals.front().tag == deletionTagBytes &&
          removals.front().producer == publisher.federateHandle(),
      "Joined-federate MOM removed-object callback returned the wrong removal metadata");
  requestRemovedCount(1, "joined-federate MOM removed-object post-removal value request");
  requireException(
      [&] { static_cast<void>(observer.rtiAmbassador().getObjectInstanceHandle(objectName)); },
      L"ObjectInstanceNotKnown",
      "looking up an application object after the MOM-counted removal");

  // The RTI-owned MOM object is also a standard Request Attribute Value Update
  // target. Exercise its public service-report route after the ordinary MOM
  // reflection path so the report and the callback remain independently
  // observable under both callback models.
  publisher.recorder().clearInteraction();
  observer.rtiAmbassador().setServiceReportingSwitch(true);
  requestRemovedCount(
      1,
      "joined-federate MOM removed-object service-reported value request");
  waitFor(
      publisher,
      [&] { return publisher.recorder().interaction().present; },
      options,
      "joined-federate MOM RequestAttributeValueUpdate service report");
  auto const momReport = publisher.recorder().interaction();
  require(
      momReport.interaction == reportClass,
      "Joined-federate MOM RequestAttributeValueUpdate report returned the wrong interaction handle");
  require(
      !momReport.producer.isValid(),
      "Joined-federate MOM RequestAttributeValueUpdate report identified an unexpected producing federate");
  require(
      momReport.tag.empty() && !momReport.regions.has_value() &&
          momReport.transportation == reliable,
      "Joined-federate MOM RequestAttributeValueUpdate report returned non-standard metadata");
  for (auto const parameterHandle : reportParameters) {
    require(
        momReport.parameters.count(parameterHandle) == 1U,
          "Joined-federate MOM RequestAttributeValueUpdate report omitted standard parameter " +
              toNarrow(observer.rtiAmbassador().getParameterName(reportClass, parameterHandle)));
  }
  rti::HLAunicodeString momService;
  momService.decode(momReport.parameters.at(reportParameters[0]));
  require(
      momService.get() == L"RequestAttributeValueUpdate",
      "Joined-federate MOM service report returned the wrong service name");
  rti::HLAinteger16BE momServiceType;
  momServiceType.decode(momReport.parameters.at(reportParameters[1]));
  require(
      momServiceType.get() == 2,
      "Joined-federate MOM service report returned the wrong service type");
  rti::HLAboolean momSuccess;
  momSuccess.decode(momReport.parameters.at(reportParameters[2]));
  require(momSuccess.get(), "Joined-federate MOM service report did not report success");
  rti::HLAunicodeString momException;
  momException.decode(momReport.parameters.at(reportParameters[5]));
  require(
      momException.get().empty(),
      "Successful joined-federate MOM service report carried an exception");
  rti::HLAinteger32BE momSerial;
  momSerial.decode(momReport.parameters.at(reportParameters[6]));
  require(
      momSerial.get() == 0,
      "First joined-federate MOM service report did not use serial number zero");
  observer.rtiAmbassador().setServiceReportingSwitch(false);

  auto const setTiming = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming");
  auto const timingFederate = observer.rtiAmbassador().getParameterHandle(
      setTiming,
      L"HLAfederate");
  auto const reportPeriod = observer.rtiAmbassador().getParameterHandle(
      setTiming,
      L"HLAreportPeriod");
  require(
      setTiming.isValid() && timingFederate.isValid() && reportPeriod.isValid() &&
          observer.rtiAmbassador().getInteractionClassName(setTiming) ==
              L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming" &&
          observer.rtiAmbassador().getParameterName(setTiming, timingFederate) ==
              L"HLAfederate" &&
          observer.rtiAmbassador().getParameterName(setTiming, reportPeriod) ==
              L"HLAreportPeriod",
      "Joined-federate MOM removed-object HLAsetTiming lookup did not round-trip");
  auto const beforePeriodic = observer.recorder().reflections().size();
  observer.rtiAmbassador().sendInteraction(
      setTiming,
      rti::ParameterHandleValueMap{
          {timingFederate, observer.federateHandle().encode()},
          {reportPeriod, rti::HLAinteger32BE{1}.encode()}},
      rti::VariableLengthData{});
  waitFor(
      observer,
      [&] {
        auto const reflections = observer.recorder().reflections();
        auto const begin = reflections.begin() +
            static_cast<std::vector<ReflectionRecord>::difference_type>(std::min(beforePeriodic, reflections.size()));
        return std::any_of(
            begin,
            reflections.end(),
            [&](ReflectionRecord const& reflection) {
              return reflection.present && reflection.object == observerMomObject &&
                  reflection.values.count(observerRemovedCountAttribute) == 1U;
            });
      },
      options,
      "joined-federate MOM removed-object periodic reflection");
  auto const periodicReflections = observer.recorder().reflections();
  auto const periodicBegin = periodicReflections.begin() +
      static_cast<std::vector<ReflectionRecord>::difference_type>(std::min(beforePeriodic, periodicReflections.size()));
  auto const periodicReflection = std::find_if(
      periodicBegin,
      periodicReflections.end(),
      [&](ReflectionRecord const& reflection) {
        return reflection.present && reflection.object == observerMomObject &&
            reflection.values.count(observerRemovedCountAttribute) == 1U;
      });
  require(
      periodicReflection != periodicReflections.end() &&
          periodicReflection->values.size() == 1U &&
          periodicReflection->transportation == reliable && periodicReflection->tag.empty() &&
          !periodicReflection->producer.isValid() &&
          !periodicReflection->regions.has_value(),
      "Joined-federate MOM removed-object periodic reflection metadata was incorrect");
  rti::HLAinteger32BE periodicRemovedCount;
  periodicRemovedCount.decode(
      periodicReflection->values.at(observerRemovedCountAttribute));
  require(
      periodicRemovedCount.get() == 1,
      "Joined-federate MOM removed-object periodic report returned the wrong count");
  observer.rtiAmbassador().sendInteraction(
      setTiming,
      rti::ParameterHandleValueMap{
          {timingFederate, observer.federateHandle().encode()},
          {reportPeriod, rti::HLAinteger32BE{0}.encode()}},
      rti::VariableLengthData{});

  observer.rtiAmbassador().unsubscribeObjectClassAttributes(
      observerFederateClass,
      momAttributes);
  publisher.rtiAmbassador().unsubscribeInteractionClass(publisherReportClass);
  observer.rtiAmbassador().unsubscribeObjectClassAttributes(
      observerObjectClass,
      observerAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherObjectClass,
      publisherAttributes);
  observer.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  publisher.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
