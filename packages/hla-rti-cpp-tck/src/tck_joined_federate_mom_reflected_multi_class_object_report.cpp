#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAvariableArray.h>

#include "tck_harness.hpp"
#include "tck_main_scenarios.hpp"
#include "tck_joined_federate_mom_reflected_multi_class_object_report.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioJoinedFederateMomObjectInstancesReflectedMultiClassReport(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.multiAttributeFom.empty() &&
          !options.mimFom.empty() && !options.objectClassName.empty() &&
          !options.attributeName.empty() &&
          !options.multiAttributeObjectClassName.empty() &&
          !options.multiAttributeFirstName.empty(),
      "Joined-federate MOM multi-class reflected-object reporting requires adapter-supplied FOMs, standard MIM, and both object-class/attribute configurations");

  Session target(options, model, "mom-reflected-multi-target");
  Session subject(options, model, "mom-reflected-multi-subject");
  Session requester(options, model, "mom-reflected-multi-requester");
  auto const federation = federationName(
      options,
      "joined-federate-mom-object-instances-reflected-multi-class-report");
  target.connect();
  subject.connect();
  requester.connect();
  std::vector<std::wstring> foms{
      options.fom.wstring(),
      options.multiAttributeFom.wstring()};
  if (options.logicalTimeImplementationName.empty()) {
    subject.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        foms,
        options.mimFom.wstring());
  } else {
    subject.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        foms,
        options.mimFom.wstring(),
        options.logicalTimeImplementationName);
  }

  auto const targetName = options.memberFederateName + L"-mom-reflected-multi-target";
  auto const subjectName = options.ownerFederateName + L"-mom-reflected-multi-subject";
  auto const requesterName =
      options.memberFederateName + L"-mom-reflected-multi-requester";
  target.join(targetName, options.federateType, federation);
  subject.join(subjectName, options.federateType, federation);
  requester.join(requesterName, options.federateType, federation);
  auto& targetAmbassador = target.rtiAmbassador();
  auto& subjectAmbassador = subject.rtiAmbassador();
  auto& requesterAmbassador = requester.rtiAmbassador();

  auto const requestClass = requesterAmbassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstancesReflected");
  auto const requestFederateParameter =
      requesterAmbassador.getParameterHandle(requestClass, L"HLAfederate");
  auto const reportClass = requesterAmbassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectInstancesReflected");
  auto const reportCountsParameter = requesterAmbassador.getParameterHandle(
      reportClass,
      L"HLAobjectInstanceCounts");
  auto const targetFederate = target.federateHandle();
  auto const reliable =
      requesterAmbassador.getTransportationTypeHandle(L"HLAreliable");

  auto const targetClassA =
      targetAmbassador.getObjectClassHandle(options.objectClassName);
  auto const targetAttributeA = targetAmbassador.getAttributeHandle(
      targetClassA,
      options.attributeName);
  auto const subjectClassA =
      subjectAmbassador.getObjectClassHandle(options.objectClassName);
  auto const subjectAttributeA = subjectAmbassador.getAttributeHandle(
      subjectClassA,
      options.attributeName);
  auto const targetClassB =
      targetAmbassador.getObjectClassHandle(options.multiAttributeObjectClassName);
  auto const targetAttributeB = targetAmbassador.getAttributeHandle(
      targetClassB,
      options.multiAttributeFirstName);
  auto const subjectClassB = subjectAmbassador.getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const subjectAttributeB = subjectAmbassador.getAttributeHandle(
      subjectClassB,
      options.multiAttributeFirstName);
  require(
      requestClass.isValid() && requestFederateParameter.isValid() &&
          reportClass.isValid() && reportCountsParameter.isValid() &&
          targetFederate.isValid() && reliable.isValid() &&
          targetClassA.isValid() && targetAttributeA.isValid() &&
          subjectClassA.isValid() && subjectAttributeA.isValid() &&
          targetClassB.isValid() && targetAttributeB.isValid() &&
          subjectClassB.isValid() && subjectAttributeB.isValid() &&
          targetClassA == subjectClassA &&
          targetAttributeA == subjectAttributeA &&
          targetClassB == subjectClassB &&
          targetAttributeB == subjectAttributeB &&
          targetClassA != targetClassB,
      "Joined-federate MOM multi-class reflected-object lookup returned invalid or inconsistent standard handles");
  targetAmbassador.subscribeObjectClassAttributes(
      targetClassA,
      rti::AttributeHandleSet{targetAttributeA},
      true);
  targetAmbassador.subscribeObjectClassAttributes(
      targetClassB,
      rti::AttributeHandleSet{targetAttributeB},
      true);
  requesterAmbassador.subscribeInteractionClass(reportClass);
  subjectAmbassador.publishObjectClassAttributes(
      subjectClassA,
      rti::AttributeHandleSet{subjectAttributeA});
  subjectAmbassador.publishObjectClassAttributes(
      subjectClassB,
      rti::AttributeHandleSet{subjectAttributeB});

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

  auto const discoveriesBefore = target.recorder().discoveryCount();
  auto const firstA = subjectAmbassador.registerObjectInstance(subjectClassA);
  auto const secondA = subjectAmbassador.registerObjectInstance(subjectClassA);
  auto const firstB = subjectAmbassador.registerObjectInstance(subjectClassB);
  auto const secondB = subjectAmbassador.registerObjectInstance(subjectClassB);
  require(
      firstA.isValid() && secondA.isValid() && firstB.isValid() &&
          secondB.isValid() && firstA != secondA && firstA != firstB &&
          firstA != secondB && secondA != firstB && secondA != secondB &&
          firstB != secondB,
      "Joined-federate MOM multi-class registration returned invalid or duplicate object handles");
  waitFor(
      target,
      [&] {
        auto const& recorder = target.recorder();
        return recorder.hasDiscovery(firstA) && recorder.hasDiscovery(secondA) &&
            recorder.hasDiscovery(firstB) && recorder.hasDiscovery(secondB);
      },
      options,
      "joined-federate MOM multi-class reflected-object discovery");
  auto const lastDiscovery = target.recorder().discovery();
  require(
      target.recorder().discoveryCount() == discoveriesBefore + 4U &&
          (lastDiscovery.object == firstA || lastDiscovery.object == secondA ||
           lastDiscovery.object == firstB || lastDiscovery.object == secondB) &&
          lastDiscovery.producer == subject.federateHandle() &&
          targetAmbassador.getKnownObjectClassHandle(firstA) == targetClassA &&
          targetAmbassador.getKnownObjectClassHandle(secondA) == targetClassA &&
          targetAmbassador.getKnownObjectClassHandle(firstB) == targetClassB &&
          targetAmbassador.getKnownObjectClassHandle(secondB) == targetClassB,
      "Joined-federate MOM multi-class discovery did not identify all four subject instances by registered class");

  auto updateObject = [&](rti::ObjectInstanceHandle const& object,
                          rti::AttributeHandle const& attribute,
                          std::int32_t value) {
    rti::AttributeHandleValueMap const values{{
        attribute,
        rti::HLAbyte{static_cast<rti::Octet>(value)}.encode()}};
    subjectAmbassador.updateAttributeValues(
        object,
        values,
        rti::VariableLengthData{});
  };
  updateObject(firstA, subjectAttributeA, 11);
  updateObject(firstA, subjectAttributeA, 12);
  updateObject(secondA, subjectAttributeA, 13);
  updateObject(firstB, subjectAttributeB, 21);
  updateObject(firstB, subjectAttributeB, 22);
  updateObject(secondB, subjectAttributeB, 23);
  waitFor(
      target,
      [&] { return target.recorder().reflections().size() >= 6U; },
      options,
      "joined-federate MOM multi-class reflected-object updates");

  std::vector<std::int32_t> firstAValues;
  std::vector<std::int32_t> secondAValues;
  std::vector<std::int32_t> firstBValues;
  std::vector<std::int32_t> secondBValues;
  auto const reflections = target.recorder().reflections();
  for (auto const& reflection : reflections) {
    if (reflection.object != firstA && reflection.object != secondA &&
        reflection.object != firstB && reflection.object != secondB) {
      continue;
    }
    auto const attribute =
        reflection.object == firstA || reflection.object == secondA
        ? targetAttributeA
        : targetAttributeB;
    require(
        reflection.values.size() == 1U &&
            reflection.values.count(attribute) == 1U &&
            reflection.transportation == reliable && reflection.tag.empty() &&
            reflection.producer == subject.federateHandle() &&
            !reflection.regions.has_value(),
        "Joined-federate MOM multi-class reflect callback had unexpected value or transport metadata");
    rti::HLAbyte decoded;
    decoded.decode(reflection.values.at(attribute));
    auto const value = static_cast<std::int32_t>(decoded.get());
    if (reflection.object == firstA) {
      firstAValues.push_back(value);
    } else if (reflection.object == secondA) {
      secondAValues.push_back(value);
    } else if (reflection.object == firstB) {
      firstBValues.push_back(value);
    } else {
      secondBValues.push_back(value);
    }
  }
  require(
      firstAValues == std::vector<std::int32_t>{11, 12} &&
          secondAValues == std::vector<std::int32_t>{13} &&
          firstBValues == std::vector<std::int32_t>{21, 22} &&
          secondBValues == std::vector<std::int32_t>{23},
      "Joined-federate MOM multi-class callbacks did not preserve the expected instance values");

  auto const interactionsBeforeRequest =
      requester.recorder().interactions().size();
  rti::ParameterHandleValueMap const requestParameters{{
      requestFederateParameter,
      targetFederate.encode()}};
  requesterAmbassador.sendInteraction(
      requestClass,
      requestParameters,
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
      "the joined-federate MOM multi-class reflected-object report");
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
      "A multi-class reflected-object query did not deliver exactly one standard report");
  auto const& report = *reports.front();
  require(
      report.present && report.parameters.size() == 1U &&
          report.parameters.count(reportCountsParameter) == 1U &&
          report.transportation == reliable && report.tag.empty() &&
          !report.producer.isValid() && !report.regions.has_value(),
      "Joined-federate MOM multi-class report did not use its standard reliable response shape");
  auto const counts =
      decodeObjectClassCounts(report.parameters.at(reportCountsParameter));
  std::optional<std::int32_t> countA;
  std::optional<std::int32_t> countB;
  for (auto const& count : counts) {
    if (count.objectClass == targetClassA) {
      require(!countA.has_value(), "The MIM repeated its first class count record");
      countA = count.count;
    } else if (count.objectClass == targetClassB) {
      require(!countB.has_value(), "The MIM repeated its second class count record");
      countB = count.count;
    } else {
      require(
          false,
          "The MIM returned a reflected-instance count for an untested class");
    }
  }
  require(
      counts.size() == 2U && countA == 2 && countB == 2,
      "Joined-federate MOM reflected-object report did not preserve the two-class/two-instance count mapping");

  requesterAmbassador.unsubscribeInteractionClass(reportClass);
  targetAmbassador.unsubscribeObjectClassAttributes(
      targetClassA,
      rti::AttributeHandleSet{targetAttributeA});
  targetAmbassador.unsubscribeObjectClassAttributes(
      targetClassB,
      rti::AttributeHandleSet{targetAttributeB});
  subjectAmbassador.deleteObjectInstance(firstA, rti::VariableLengthData{});
  subjectAmbassador.deleteObjectInstance(secondA, rti::VariableLengthData{});
  subjectAmbassador.deleteObjectInstance(firstB, rti::VariableLengthData{});
  subjectAmbassador.deleteObjectInstance(secondB, rti::VariableLengthData{});
  subjectAmbassador.unpublishObjectClassAttributes(
      subjectClassA,
      rti::AttributeHandleSet{subjectAttributeA});
  subjectAmbassador.unpublishObjectClassAttributes(
      subjectClassB,
      rti::AttributeHandleSet{subjectAttributeB});
  target.resign(rti::NO_ACTION);
  subject.resign(rti::NO_ACTION);
  requester.resign(rti::NO_ACTION);
  subjectAmbassador.destroyFederationExecution(federation);
  target.disconnect();
  subject.disconnect();
  requester.disconnect();
}

void scenarioJoinedFederateMomObjectInstancesReflectedMultiClassReportContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioJoinedFederateMomObjectInstancesReflectedMultiClassReport(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
