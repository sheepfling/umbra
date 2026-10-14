#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAvariableArray.h>

#include "tck_harness.hpp"
#include "tck_main_scenarios.hpp"
#include "tck_joined_federate_mom_updated_timestamped_object_report.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioJoinedFederateMomObjectInstancesUpdatedTimestampedReport(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.objectClassName.empty() && !options.attributeName.empty(),
      "Joined-federate MOM timestamped updated-object reporting requires adapter-supplied FOM, standard MIM, object class, and attribute configuration");

  Session subject(options, model, "mom-updated-timestamped-subject");
  Session observer(options, model, "mom-updated-timestamped-observer");
  Session requester(options, model, "mom-updated-timestamped-requester");
  auto const federation = federationName(
      options,
      "joined-federate-mom-object-instances-updated-timestamped-report");
  subject.connect();
  observer.connect();
  requester.connect();
  if (options.logicalTimeImplementationName.empty()) {
    subject.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring());
  } else {
    subject.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring(),
        options.logicalTimeImplementationName);
  }

  auto const subjectName =
      options.ownerFederateName + L"-mom-updated-timestamped-subject";
  auto const observerName =
      options.memberFederateName + L"-mom-updated-timestamped-observer";
  auto const requesterName =
      options.memberFederateName + L"-mom-updated-timestamped-requester";
  subject.join(subjectName, options.federateType, federation);
  observer.join(observerName, options.federateType, federation);
  requester.join(requesterName, options.federateType, federation);
  auto& subjectAmbassador = subject.rtiAmbassador();
  auto& observerAmbassador = observer.rtiAmbassador();
  auto& requesterAmbassador = requester.rtiAmbassador();

  auto const subjectObjectClass =
      subjectAmbassador.getObjectClassHandle(options.objectClassName);
  auto const subjectAttribute = subjectAmbassador.getAttributeHandle(
      subjectObjectClass,
      options.attributeName);
  auto const observerObjectClass =
      observerAmbassador.getObjectClassHandle(options.objectClassName);
  auto const observerAttribute = observerAmbassador.getAttributeHandle(
      observerObjectClass,
      options.attributeName);
  auto const requestClass = requesterAmbassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstancesUpdated");
  auto const requestFederateParameter =
      requesterAmbassador.getParameterHandle(requestClass, L"HLAfederate");
  auto const reportClass = requesterAmbassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectInstancesUpdated");
  auto const reportCountsParameter = requesterAmbassador.getParameterHandle(
      reportClass,
      L"HLAobjectInstanceCounts");
  auto const targetFederate = requesterAmbassador.getFederateHandle(subjectName);
  auto const reliable =
      requesterAmbassador.getTransportationTypeHandle(L"HLAreliable");
  auto const observerReliable =
      observerAmbassador.getTransportationTypeHandle(L"HLAreliable");
  auto const requesterObjectClass =
      requesterAmbassador.getObjectClassHandle(options.objectClassName);
  require(
      subjectObjectClass.isValid() && subjectAttribute.isValid() &&
          observerObjectClass.isValid() && observerAttribute.isValid() &&
          requestClass.isValid() && requestFederateParameter.isValid() &&
          reportClass.isValid() && reportCountsParameter.isValid() &&
          targetFederate.isValid() && reliable.isValid() &&
          observerReliable.isValid() && requesterObjectClass.isValid(),
      "Joined-federate MOM timestamped updated-object lookup returned an invalid standard handle");

  requesterAmbassador.subscribeInteractionClass(reportClass);
  subjectAmbassador.publishObjectClassAttributes(
      subjectObjectClass,
      rti::AttributeHandleSet{subjectAttribute});
  subjectAmbassador.changeDefaultAttributeOrderType(
      subjectObjectClass,
      rti::AttributeHandleSet{subjectAttribute},
      rti::TIMESTAMP);
  observerAmbassador.subscribeObjectClassAttributes(
      observerObjectClass,
      rti::AttributeHandleSet{observerAttribute});
  auto const firstObject = subjectAmbassador.registerObjectInstance(subjectObjectClass);
  auto const secondObject = subjectAmbassador.registerObjectInstance(subjectObjectClass);
  require(
      firstObject.isValid() && secondObject.isValid(),
      "Joined-federate MOM timestamped updated-object registration returned an invalid handle");
  waitFor(
      observer,
      [&] { return observer.recorder().discoveryCount() >= 2U; },
      options,
      "joined-federate MOM timestamped updated-object discovery");
  auto const lastDiscovery = observer.recorder().discovery();
  require(
      observer.recorder().discoveryCount() == 2U &&
          lastDiscovery.objectClass == observerObjectClass &&
          lastDiscovery.producer == subject.federateHandle() &&
          (lastDiscovery.object == firstObject ||
           lastDiscovery.object == secondObject),
      "Joined-federate MOM timestamped update fixture discovered unexpected object metadata");

  auto const subjectTime = makeTimeContext(subject);
  auto const observerTime = makeTimeContext(observer);
  enableTimestampedRoles(
      subject,
      observer,
      subjectTime,
      observerTime,
      options,
      "joined-federate MOM timestamped updated-object report");
  auto const messageTime = timeAfter(
      *subjectTime.factory,
      *subjectTime.initial,
      *subjectTime.epsilon,
      3U);
  auto const observerTarget = timeAfter(
      *observerTime.factory,
      *observerTime.initial,
      *observerTime.epsilon,
      3U);
  auto const firstValue = rti::HLAbyte{static_cast<rti::Octet>(0x41U)};
  auto const secondValue = rti::HLAbyte{static_cast<rti::Octet>(0x42U)};
  auto const firstValueBytes = copyBytes(firstValue.encode());
  auto const secondValueBytes = copyBytes(secondValue.encode());
  std::vector<std::uint8_t> const firstTagBytes{0x51U, 0x52U};
  std::vector<std::uint8_t> const secondTagBytes{0x61U, 0x62U};
  rti::VariableLengthData const firstTag(
      firstTagBytes.data(),
      firstTagBytes.size());
  rti::VariableLengthData const secondTag(
      secondTagBytes.data(),
      secondTagBytes.size());
  rti::AttributeHandleValueMap const firstValues{{
      subjectAttribute,
      firstValue.encode()}};
  rti::AttributeHandleValueMap const secondValues{{
      subjectAttribute,
      secondValue.encode()}};
  auto const firstRetraction = subjectAmbassador.updateAttributeValues(
      firstObject,
      firstValues,
      firstTag,
      *messageTime);
  auto const secondRetraction = subjectAmbassador.updateAttributeValues(
      secondObject,
      secondValues,
      secondTag,
      *messageTime);
  require(
      firstRetraction.isValid() && secondRetraction.isValid(),
      "Joined-federate MOM timestamped Update Attribute Values returned an invalid retraction handle");
  require(
      observer.recorder().timedReflections().empty(),
      "Joined-federate MOM timestamped update reflected before the constrained grant");

  observerAmbassador.timeAdvanceRequest(*observerTarget);
  subjectAmbassador.timeAdvanceRequest(*messageTime);
  waitForSessions(
      {&subject, &observer},
      [&] {
        return subject.recorder().timeAdvanceGrants().size() >= 1U &&
            observer.recorder().timeAdvanceGrants().size() >= 1U &&
            observer.recorder().timedReflections().size() >= 2U;
      },
      options,
      "joined-federate MOM timestamped update reflection and grants");

  auto const timedReflections = observer.recorder().timedReflections();
  require(
      timedReflections.size() == 2U,
      "Joined-federate MOM timestamped updates delivered a duplicate or missing reflection");
  auto findReflection = [&](rti::ObjectInstanceHandle const& object) {
    return std::find_if(
        timedReflections.begin(),
        timedReflections.end(),
        [&](TimedReflectionRecord const& reflection) {
          return reflection.object == object;
        });
  };
  auto const firstReflection = findReflection(firstObject);
  auto const secondReflection = findReflection(secondObject);
  require(
      firstReflection != timedReflections.end() &&
          secondReflection != timedReflections.end(),
      "Joined-federate MOM timestamped reflections did not cover both updated instances");
  auto verifyReflection = [&](TimedReflectionRecord const& reflection,
                              std::vector<std::uint8_t> const& expectedValue,
                              std::vector<std::uint8_t> const& expectedTag,
                              rti::MessageRetractionHandle const& expectedRetraction) {
    require(
        reflection.values.size() == 1U &&
            reflection.values.count(observerAttribute) == 1U &&
            copyBytes(reflection.values.at(observerAttribute)) == expectedValue &&
            reflection.tag == expectedTag &&
            reflection.time == encodeTime(*messageTime),
        "Joined-federate MOM timestamped reflection changed its object value, tag, or logical time");
    require(
        reflection.producer == subject.federateHandle() &&
            !reflection.timeText.empty() &&
            reflection.sentOrder == rti::TIMESTAMP &&
            reflection.receivedOrder == rti::TIMESTAMP &&
            reflection.transportation == observerReliable &&
            reflection.retractionPresent &&
            reflection.retraction == copyBytes(expectedRetraction.encode()),
        "Joined-federate MOM timestamped reflection changed its standard delivery metadata");
  };
  verifyReflection(
      *firstReflection,
      firstValueBytes,
      firstTagBytes,
      firstRetraction);
  verifyReflection(
      *secondReflection,
      secondValueBytes,
      secondTagBytes,
      secondRetraction);
  auto const subjectGrants = subject.recorder().timeAdvanceGrants();
  auto const observerGrants = observer.recorder().timeAdvanceGrants();
  require(
      subjectGrants.size() == 1U && observerGrants.size() == 1U &&
          subjectGrants.front().encoded == encodeTime(*messageTime) &&
          observerGrants.front().encoded == encodeTime(*observerTarget),
      "Joined-federate MOM timestamped update returned the wrong time-advance grants");

  auto const interactionsBeforeRequest =
      requester.recorder().interactions().size();
  requesterAmbassador.sendInteraction(
      requestClass,
      rti::ParameterHandleValueMap{{
          requestFederateParameter,
          targetFederate.encode()}},
      rti::VariableLengthData{});
  waitFor(
      requester,
      [&] {
        auto const interactions = requester.recorder().interactions();
        return interactions.size() > interactionsBeforeRequest &&
            std::any_of(
                interactions.begin() + static_cast<std::ptrdiff_t>(
                                           interactionsBeforeRequest),
                interactions.end(),
                [&](InteractionRecord const& interaction) {
                  return interaction.present &&
                      interaction.interaction == reportClass &&
                      interaction.parameters.count(reportCountsParameter) == 1U;
                });
      },
      options,
      "joined-federate MOM timestamped updated-object report");
  auto const interactions = requester.recorder().interactions();
  std::vector<InteractionRecord const*> reports;
  for (std::size_t index = interactionsBeforeRequest;
       index != interactions.size();
       ++index) {
    if (interactions[index].interaction == reportClass) {
      reports.push_back(&interactions[index]);
    }
  }
  require(
      reports.size() == 1U,
      "An updated-object timestamped publication query did not deliver exactly one standard report");
  auto const& report = *reports.front();
  require(
      report.present && report.parameters.size() == 1U &&
          report.parameters.count(reportCountsParameter) == 1U &&
          report.transportation == reliable && report.tag.empty() &&
          !report.producer.isValid() && !report.regions.has_value(),
      "Joined-federate MOM timestamped updated-object report did not use its standard reliable one-parameter response shape");

  struct ObjectClassCount final {
    rti::ObjectClassHandle objectClass;
    std::int32_t count;
  };
  auto decodeObjectClassCounts = [&](rti::VariableLengthData const& encoded) {
    rti::HLAvariableArrayT<rti::HLAoctet> objectClassHandlePrototype;
    rti::HLAfixedRecord recordPrototype;
    recordPrototype.appendElement(objectClassHandlePrototype)
        .appendElement(rti::HLAinteger32BE{});
    rti::HLAvariableArray counts{recordPrototype};
    counts.decode(encoded);

    std::vector<ObjectClassCount> decoded;
    decoded.reserve(counts.size());
    for (std::size_t index = 0; index != counts.size(); ++index) {
      auto const& record =
          dynamic_cast<rti::HLAfixedRecord const&>(counts.get(index));
      auto const& encodedObjectClass =
          dynamic_cast<rti::HLAvariableArray const&>(record.get(0));
      decoded.push_back({
          requesterAmbassador.decodeObjectClassHandle(
              encodedObjectClass.encode()),
          dynamic_cast<rti::HLAinteger32BE const&>(record.get(1)).get()});
    }
    return decoded;
  };
  auto const counts =
      decodeObjectClassCounts(report.parameters.at(reportCountsParameter));
  require(
      counts.size() == 1U &&
          counts.front().objectClass == requesterObjectClass &&
          counts.front().count == 2,
      "Joined-federate MOM report did not count both timestamp-updated instances by registered class");

  requesterAmbassador.unsubscribeInteractionClass(reportClass);
  observerAmbassador.unsubscribeObjectClassAttributes(
      observerObjectClass,
      rti::AttributeHandleSet{observerAttribute});
  subjectAmbassador.deleteObjectInstance(firstObject, rti::VariableLengthData{});
  subjectAmbassador.deleteObjectInstance(secondObject, rti::VariableLengthData{});
  subjectAmbassador.unpublishObjectClassAttributes(
      subjectObjectClass,
      rti::AttributeHandleSet{subjectAttribute});
  requester.resign(rti::NO_ACTION);
  observer.resign(rti::NO_ACTION);
  subject.resign(rti::NO_ACTION);
  subjectAmbassador.destroyFederationExecution(federation);
  requester.disconnect();
  observer.disconnect();
  subject.disconnect();
}

void scenarioJoinedFederateMomObjectInstancesUpdatedTimestampedReportContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioJoinedFederateMomObjectInstancesUpdatedTimestampedReport(
      options,
      model);
}

}  // namespace hla_rti_cpp_tck::detail
