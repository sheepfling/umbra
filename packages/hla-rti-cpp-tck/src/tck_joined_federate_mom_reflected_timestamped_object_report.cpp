#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAvariableArray.h>

#include "tck_harness.hpp"
#include "tck_logical_time.hpp"
#include "tck_main_scenarios.hpp"
#include "tck_joined_federate_mom_reflected_timestamped_object_report.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <stdexcept>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioJoinedFederateMomObjectInstancesReflectedTimestampedReport(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.objectClassName.empty() && !options.attributeName.empty(),
      "Joined-federate MOM timestamped reflected-object reporting requires adapter-supplied FOM, standard MIM, object class, and attribute configuration");

  Session requester(options, model, "mom-reflected-timestamped-target");
  Session subject(options, model, "mom-reflected-timestamped-subject");
  Session monitor(options, model, "mom-reflected-timestamped-requester");
  auto const federation = federationName(
      options,
      "joined-federate-mom-object-instances-reflected-timestamped-report");
  requester.connect();
  subject.connect();
  monitor.connect();
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

  auto const requesterName =
      options.memberFederateName + L"-mom-reflected-timestamped-target";
  auto const subjectName =
      options.ownerFederateName + L"-mom-reflected-timestamped-subject";
  auto const monitorName =
      options.memberFederateName + L"-mom-reflected-timestamped-requester";
  requester.join(requesterName, options.federateType, federation);
  subject.join(subjectName, options.federateType, federation);
  monitor.join(monitorName, options.federateType, federation);
  auto& requesterAmbassador = requester.rtiAmbassador();
  auto& subjectAmbassador = subject.rtiAmbassador();
  auto& monitorAmbassador = monitor.rtiAmbassador();

  auto const requestClass = monitorAmbassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstancesReflected");
  auto const requestFederateParameter =
      monitorAmbassador.getParameterHandle(requestClass, L"HLAfederate");
  auto const reportClass = monitorAmbassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectInstancesReflected");
  auto const reportCountsParameter = monitorAmbassador.getParameterHandle(
      reportClass,
      L"HLAobjectInstanceCounts");
  auto const targetFederate = requester.federateHandle();
  auto const reliable =
      monitorAmbassador.getTransportationTypeHandle(L"HLAreliable");
  auto const requesterObjectClass =
      requesterAmbassador.getObjectClassHandle(options.objectClassName);
  auto const requesterAttribute = requesterAmbassador.getAttributeHandle(
      requesterObjectClass,
      options.attributeName);
  auto const subjectObjectClass =
      subjectAmbassador.getObjectClassHandle(options.objectClassName);
  auto const subjectAttribute = subjectAmbassador.getAttributeHandle(
      subjectObjectClass,
      options.attributeName);
  auto const requesterReliable =
      requesterAmbassador.getTransportationTypeHandle(L"HLAreliable");
  require(
      requestClass.isValid() && requestFederateParameter.isValid() &&
          reportClass.isValid() && reportCountsParameter.isValid() &&
          targetFederate.isValid() && reliable.isValid() &&
          requesterReliable.isValid() && requesterObjectClass.isValid() &&
          requesterAttribute.isValid() && subjectObjectClass.isValid() &&
          subjectAttribute.isValid() &&
          requesterObjectClass == subjectObjectClass &&
          requesterAttribute == subjectAttribute,
      "Joined-federate MOM timestamped reflected-object request/report or adapter-supplied application lookup returned an invalid or inconsistent handle");

  requesterAmbassador.subscribeObjectClassAttributes(
      requesterObjectClass,
      rti::AttributeHandleSet{requesterAttribute},
      true);
  monitorAmbassador.subscribeInteractionClass(reportClass);
  subjectAmbassador.publishObjectClassAttributes(
      subjectObjectClass,
      rti::AttributeHandleSet{subjectAttribute});
  subjectAmbassador.changeDefaultAttributeOrderType(
      subjectObjectClass,
      rti::AttributeHandleSet{subjectAttribute},
      rti::TIMESTAMP);

  auto const discoveriesBefore = requester.recorder().discoveryCount();
  auto const firstObject =
      subjectAmbassador.registerObjectInstance(subjectObjectClass);
  auto const secondObject =
      subjectAmbassador.registerObjectInstance(subjectObjectClass);
  require(
      firstObject.isValid() && secondObject.isValid() &&
          firstObject != secondObject,
      "Joined-federate MOM timestamped reflected-object registration returned invalid or duplicate handles");
  waitFor(
      requester,
      [&] {
        return requester.recorder().hasDiscovery(firstObject) &&
            requester.recorder().hasDiscovery(secondObject);
      },
      options,
      "joined-federate MOM timestamped reflected-object discovery");
  auto const lastDiscovery = requester.recorder().discovery();
  require(
      requester.recorder().discoveryCount() == discoveriesBefore + 2U &&
          (lastDiscovery.object == firstObject ||
           lastDiscovery.object == secondObject) &&
          lastDiscovery.objectClass == requesterObjectClass &&
          lastDiscovery.producer == subject.federateHandle() &&
          requesterAmbassador.getKnownObjectClassHandle(firstObject) ==
              requesterObjectClass &&
          requesterAmbassador.getKnownObjectClassHandle(secondObject) ==
              requesterObjectClass,
      "Joined-federate MOM timestamped reflected-object test did not discover exactly both registered instances from the subject class");

  auto const subjectTime = makeTimeContext(subject);
  auto const requesterTime = makeTimeContext(requester);
  enableTimestampedRoles(
      subject,
      requester,
      subjectTime,
      requesterTime,
      options,
      "joined-federate MOM timestamped reflected-object report");
  auto const messageTime = timeAfter(
      *subjectTime.factory,
      *subjectTime.initial,
      *subjectTime.epsilon,
      3U);
  auto const requesterTarget = timeAfter(
      *requesterTime.factory,
      *requesterTime.initial,
      *requesterTime.epsilon,
      3U);
  auto const firstValue = rti::HLAbyte{static_cast<rti::Octet>(1U)};
  auto const repeatedValue = rti::HLAbyte{static_cast<rti::Octet>(2U)};
  auto const secondValue = rti::HLAbyte{static_cast<rti::Octet>(3U)};
  auto const firstValueBytes = copyBytes(firstValue.encode());
  auto const repeatedValueBytes = copyBytes(repeatedValue.encode());
  auto const secondValueBytes = copyBytes(secondValue.encode());
  std::vector<std::uint8_t> const firstTagBytes{0x31U, 0x32U};
  std::vector<std::uint8_t> const repeatedTagBytes{0x41U, 0x42U};
  std::vector<std::uint8_t> const secondTagBytes{0x51U, 0x52U};
  rti::VariableLengthData const firstTag(
      firstTagBytes.data(),
      firstTagBytes.size());
  rti::VariableLengthData const repeatedTag(
      repeatedTagBytes.data(),
      repeatedTagBytes.size());
  rti::VariableLengthData const secondTag(
      secondTagBytes.data(),
      secondTagBytes.size());
  rti::AttributeHandleValueMap const firstValues{{
      subjectAttribute,
      firstValue.encode()}};
  rti::AttributeHandleValueMap const repeatedValues{{
      subjectAttribute,
      repeatedValue.encode()}};
  rti::AttributeHandleValueMap const secondValues{{
      subjectAttribute,
      secondValue.encode()}};
  auto const firstRetraction = subjectAmbassador.updateAttributeValues(
      firstObject,
      firstValues,
      firstTag,
      *messageTime);
  auto const repeatedRetraction = subjectAmbassador.updateAttributeValues(
      firstObject,
      repeatedValues,
      repeatedTag,
      *messageTime);
  auto const secondRetraction = subjectAmbassador.updateAttributeValues(
      secondObject,
      secondValues,
      secondTag,
      *messageTime);
  require(
      firstRetraction.isValid() && repeatedRetraction.isValid() &&
          secondRetraction.isValid(),
      "Joined-federate MOM timestamped Update Attribute Values returned an invalid retraction handle");
  require(
      requester.recorder().timedReflections().empty(),
      "Joined-federate MOM timestamped reflection occurred before the constrained grant");

  requesterAmbassador.timeAdvanceRequest(*requesterTarget);
  subjectAmbassador.timeAdvanceRequest(*messageTime);
  try {
    waitFor(
        requester,
        [&] { return requester.recorder().timedReflections().size() >= 3U; },
        options,
        "joined-federate MOM timestamped reflect callbacks");
  } catch (std::exception const& error) {
    throw std::runtime_error(
        std::string(error.what()) + " (reflections=" +
        std::to_string(requester.recorder().timedReflections().size()) +
        ", target-grants=" +
        std::to_string(requester.recorder().timeAdvanceGrants().size()) +
        ", subject-grants=" +
        std::to_string(subject.recorder().timeAdvanceGrants().size()) + ")");
  }
  waitFor(
      subject,
      [&] { return subject.recorder().timeAdvanceGrants().size() >= 1U; },
      options,
      "joined-federate MOM timestamped subject grant");
  waitFor(
      requester,
      [&] { return requester.recorder().timeAdvanceGrants().size() >= 1U; },
      options,
      "joined-federate MOM timestamped target grant");

  auto const reflections = requester.recorder().timedReflections();
  require(
      reflections.size() == 3U,
      "Joined-federate MOM timestamped updates delivered a duplicate or missing reflection");
  auto findReflection = [&](rti::ObjectInstanceHandle const& object,
                            std::vector<std::uint8_t> const& tag) {
    return std::find_if(
        reflections.begin(),
        reflections.end(),
        [&](TimedReflectionRecord const& reflection) {
          return reflection.object == object && reflection.tag == tag;
        });
  };
  auto verifyReflection = [&](TimedReflectionRecord const& reflection,
                              rti::AttributeHandle const& expectedAttribute,
                              std::vector<std::uint8_t> const& expectedValue,
                              std::vector<std::uint8_t> const& expectedTag,
                              std::vector<std::uint8_t> const& expectedTime,
                              rti::MessageRetractionHandle const& expectedRetraction) {
    require(
        reflection.values.size() == 1U &&
            reflection.values.count(expectedAttribute) == 1U &&
            copyBytes(reflection.values.at(expectedAttribute)) == expectedValue &&
            reflection.tag == expectedTag && reflection.time == expectedTime &&
            !reflection.timeText.empty(),
        "Joined-federate MOM timestamped reflect callback changed its object value, tag, attribute, or logical-time metadata");
    require(
        reflection.producer == subject.federateHandle() &&
            reflection.sentOrder == rti::TIMESTAMP &&
            reflection.receivedOrder == rti::TIMESTAMP &&
            reflection.transportation == requesterReliable &&
            reflection.retractionPresent &&
            reflection.retraction == copyBytes(expectedRetraction.encode()) &&
            !reflection.regions.has_value(),
        "Joined-federate MOM timestamped reflect callback changed its standard delivery metadata");
  };
  auto const expectedTime = encodeTime(*messageTime);
  auto const firstReflection = findReflection(firstObject, firstTagBytes);
  auto const repeatedReflection = findReflection(firstObject, repeatedTagBytes);
  auto const secondReflection = findReflection(secondObject, secondTagBytes);
  require(
      firstReflection != reflections.end() &&
          repeatedReflection != reflections.end() &&
          secondReflection != reflections.end(),
      "Joined-federate MOM timestamped reflections did not cover both instances at all requested logical times");
  verifyReflection(
      *firstReflection,
      requesterAttribute,
      firstValueBytes,
      firstTagBytes,
      expectedTime,
      firstRetraction);
  verifyReflection(
      *repeatedReflection,
      requesterAttribute,
      repeatedValueBytes,
      repeatedTagBytes,
      expectedTime,
      repeatedRetraction);
  verifyReflection(
      *secondReflection,
      requesterAttribute,
      secondValueBytes,
      secondTagBytes,
      expectedTime,
      secondRetraction);
  auto const requesterGrants = requester.recorder().timeAdvanceGrants();
  auto const subjectGrants = subject.recorder().timeAdvanceGrants();
  require(
      requesterGrants.size() == 1U && subjectGrants.size() == 1U &&
          requesterGrants.front().encoded == encodeTime(*requesterTarget) &&
          subjectGrants.front().encoded == encodeTime(*messageTime),
      "Joined-federate MOM timestamped reflections returned the wrong standard time-advance grants");
  auto const callbackOrder = requester.recorder().callbackOrder();
  auto const firstGrant = std::find(callbackOrder.begin(), callbackOrder.end(), "grant");
  require(
      firstGrant != callbackOrder.end() &&
          std::count(callbackOrder.begin(), callbackOrder.end(), "reflect") == 3 &&
          std::all_of(
              callbackOrder.begin(),
              firstGrant,
              [](std::string const& callback) { return callback == "reflect"; }),
      "Joined-federate MOM timestamped reflect callbacks did not precede the observer's Time Advance Grant");

  auto const interactionsBeforeRequest =
      monitor.recorder().interactions().size();
  monitorAmbassador.sendInteraction(
      requestClass,
      rti::ParameterHandleValueMap{{
          requestFederateParameter,
          targetFederate.encode()}},
      rti::VariableLengthData{});
  waitFor(
      monitor,
      [&] {
        auto const interactions = monitor.recorder().interactions();
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
      "the joined-federate MOM timestamped reflected-object report");
  auto const interactions = monitor.recorder().interactions();
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
      "A timestamped reflected-object query did not deliver exactly one standard report");
  auto const& report = *reports.front();
  require(
      report.present && report.parameters.size() == 1U &&
          report.parameters.count(reportCountsParameter) == 1U &&
          report.transportation == reliable && report.tag.empty() &&
          !report.producer.isValid() && !report.regions.has_value(),
      "Joined-federate MOM timestamped reflected-object report did not use its standard reliable one-parameter response shape");

  struct ObjectClassCount final {
    rti::ObjectClassHandle objectClass;
    std::int32_t count;
  };
  rti::HLAvariableArrayT<rti::HLAoctet> objectClassHandlePrototype;
  rti::HLAfixedRecord recordPrototype;
  recordPrototype.appendElement(objectClassHandlePrototype)
      .appendElement(rti::HLAinteger32BE{});
  rti::HLAvariableArray counts{recordPrototype};
  counts.decode(report.parameters.at(reportCountsParameter));
  std::vector<ObjectClassCount> decodedCounts;
  decodedCounts.reserve(counts.size());
  for (std::size_t index = 0; index != counts.size(); ++index) {
    auto const& record =
        dynamic_cast<rti::HLAfixedRecord const&>(counts.get(index));
    auto const& encodedObjectClass =
        dynamic_cast<rti::HLAvariableArray const&>(record.get(0));
    decodedCounts.push_back({
        monitorAmbassador.decodeObjectClassHandle(encodedObjectClass.encode()),
        dynamic_cast<rti::HLAinteger32BE const&>(record.get(1)).get()});
  }
  require(
      decodedCounts.size() == 1U &&
          decodedCounts.front().objectClass == requesterObjectClass &&
          decodedCounts.front().count == 2,
      "Joined-federate MOM timestamped reflected-object report did not count two distinct reflected instances by registered class");

  monitorAmbassador.unsubscribeInteractionClass(reportClass);
  requesterAmbassador.unsubscribeObjectClassAttributes(
      requesterObjectClass,
      rti::AttributeHandleSet{requesterAttribute});
  subjectAmbassador.deleteObjectInstance(firstObject, rti::VariableLengthData{});
  subjectAmbassador.deleteObjectInstance(secondObject, rti::VariableLengthData{});
  subjectAmbassador.unpublishObjectClassAttributes(
      subjectObjectClass,
      rti::AttributeHandleSet{subjectAttribute});
  requester.resign(rti::NO_ACTION);
  subject.resign(rti::NO_ACTION);
  monitor.resign(rti::NO_ACTION);
  subjectAmbassador.destroyFederationExecution(federation);
  requester.disconnect();
  subject.disconnect();
  monitor.disconnect();
}

void scenarioJoinedFederateMomObjectInstancesReflectedTimestampedReportContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioJoinedFederateMomObjectInstancesReflectedTimestampedReport(
      options,
      model);
}

}  // namespace hla_rti_cpp_tck::detail
