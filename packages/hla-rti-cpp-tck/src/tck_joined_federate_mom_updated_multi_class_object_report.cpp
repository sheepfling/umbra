#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAvariableArray.h>

#include "tck_harness.hpp"
#include "tck_main_scenarios.hpp"
#include "tck_joined_federate_mom_updated_multi_class_object_report.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioJoinedFederateMomObjectInstancesUpdatedMultiClassReport(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.multiAttributeFom.empty() &&
          !options.mimFom.empty() && !options.objectClassName.empty() &&
          !options.attributeName.empty() &&
          !options.multiAttributeObjectClassName.empty() &&
          !options.multiAttributeFirstName.empty(),
      "Joined-federate MOM multi-class updated-object reporting requires adapter-supplied FOMs, standard MIM, and both object-class/attribute configurations");

  Session subject(options, model, "mom-updated-multi-subject");
  Session observer(options, model, "mom-updated-multi-observer");
  Session requester(options, model, "mom-updated-multi-requester");
  auto const federation = federationName(
      options,
      "joined-federate-mom-object-instances-updated-multi-class-report");
  subject.connect();
  observer.connect();
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

  auto const subjectName = options.ownerFederateName + L"-mom-updated-multi-subject";
  auto const observerName = options.memberFederateName + L"-mom-updated-multi-observer";
  subject.join(subjectName, options.federateType, federation);
  observer.join(observerName, options.federateType, federation);
  requester.join(
      options.memberFederateName + L"-mom-updated-multi-requester",
      options.federateType,
      federation);
  auto& subjectAmbassador = subject.rtiAmbassador();
  auto& observerAmbassador = observer.rtiAmbassador();
  auto& requesterAmbassador = requester.rtiAmbassador();

  auto const requestClass = requesterAmbassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstancesUpdated");
  auto const requestFederateParameter =
      requesterAmbassador.getParameterHandle(requestClass, L"HLAfederate");
  auto const reportClass = requesterAmbassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectInstancesUpdated");
  auto const reportCountsParameter = requesterAmbassador.getParameterHandle(
      reportClass,
      L"HLAobjectInstanceCounts");
  auto const subjectFederate = subject.federateHandle();
  auto const reliable =
      requesterAmbassador.getTransportationTypeHandle(L"HLAreliable");

  auto const subjectClassA =
      subjectAmbassador.getObjectClassHandle(options.objectClassName);
  auto const subjectAttributeA = subjectAmbassador.getAttributeHandle(
      subjectClassA,
      options.attributeName);
  auto const observerClassA =
      observerAmbassador.getObjectClassHandle(options.objectClassName);
  auto const observerAttributeA = observerAmbassador.getAttributeHandle(
      observerClassA,
      options.attributeName);
  auto const subjectClassB =
      subjectAmbassador.getObjectClassHandle(options.multiAttributeObjectClassName);
  auto const subjectAttributeB = subjectAmbassador.getAttributeHandle(
      subjectClassB,
      options.multiAttributeFirstName);
  auto const observerClassB =
      observerAmbassador.getObjectClassHandle(options.multiAttributeObjectClassName);
  auto const observerAttributeB = observerAmbassador.getAttributeHandle(
      observerClassB,
      options.multiAttributeFirstName);
  require(
      requestClass.isValid() && requestFederateParameter.isValid() &&
          reportClass.isValid() && reportCountsParameter.isValid() &&
          subjectFederate.isValid() && reliable.isValid() &&
          subjectClassA.isValid() && subjectAttributeA.isValid() &&
          observerClassA.isValid() && observerAttributeA.isValid() &&
          subjectClassB.isValid() && subjectAttributeB.isValid() &&
          observerClassB.isValid() && observerAttributeB.isValid() &&
          subjectClassA == observerClassA &&
          subjectAttributeA == observerAttributeA &&
          subjectClassB == observerClassB &&
          subjectAttributeB == observerAttributeB &&
          subjectClassA != subjectClassB,
      "Joined-federate MOM multi-class updated-object lookup returned invalid or inconsistent standard handles");
  observerAmbassador.subscribeObjectClassAttributes(
      observerClassA,
      rti::AttributeHandleSet{observerAttributeA},
      true);
  observerAmbassador.subscribeObjectClassAttributes(
      observerClassB,
      rti::AttributeHandleSet{observerAttributeB},
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

  auto const discoveriesBefore = observer.recorder().discoveryCount();
  auto const firstA = subjectAmbassador.registerObjectInstance(subjectClassA);
  auto const secondA = subjectAmbassador.registerObjectInstance(subjectClassA);
  auto const firstB = subjectAmbassador.registerObjectInstance(subjectClassB);
  auto const secondB = subjectAmbassador.registerObjectInstance(subjectClassB);
  require(
      firstA.isValid() && secondA.isValid() && firstB.isValid() &&
          secondB.isValid() && firstA != secondA && firstA != firstB &&
          firstA != secondB && secondA != firstB && secondA != secondB &&
          firstB != secondB,
      "Joined-federate MOM multi-class updated-object registration returned invalid or duplicate object handles");
  waitFor(
      observer,
      [&] {
        auto const& recorder = observer.recorder();
        return recorder.hasDiscovery(firstA) && recorder.hasDiscovery(secondA) &&
            recorder.hasDiscovery(firstB) && recorder.hasDiscovery(secondB);
      },
      options,
      "joined-federate MOM multi-class updated-object discovery");
  auto const lastDiscovery = observer.recorder().discovery();
  require(
      observer.recorder().discoveryCount() == discoveriesBefore + 4U &&
          (lastDiscovery.object == firstA || lastDiscovery.object == secondA ||
           lastDiscovery.object == firstB || lastDiscovery.object == secondB) &&
          lastDiscovery.producer == subject.federateHandle() &&
          observerAmbassador.getKnownObjectClassHandle(firstA) == observerClassA &&
          observerAmbassador.getKnownObjectClassHandle(secondA) == observerClassA &&
          observerAmbassador.getKnownObjectClassHandle(firstB) == observerClassB &&
          observerAmbassador.getKnownObjectClassHandle(secondB) == observerClassB,
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
  auto const reflectionsBefore = observer.recorder().reflections().size();
  // Repeated updates in each class must still count a distinct instance once.
  updateObject(firstA, subjectAttributeA, 11);
  updateObject(firstA, subjectAttributeA, 12);
  updateObject(secondA, subjectAttributeA, 13);
  updateObject(firstB, subjectAttributeB, 21);
  updateObject(firstB, subjectAttributeB, 22);
  updateObject(secondB, subjectAttributeB, 23);
  waitFor(
      observer,
      [&] {
        return observer.recorder().reflections().size() >= reflectionsBefore + 6U;
      },
      options,
      "joined-federate MOM multi-class receive-order reflections");

  std::vector<std::int32_t> firstAValues;
  std::vector<std::int32_t> secondAValues;
  std::vector<std::int32_t> firstBValues;
  std::vector<std::int32_t> secondBValues;
  auto const reflections = observer.recorder().reflections();
  for (auto const& reflection : reflections) {
    if (reflection.object != firstA && reflection.object != secondA &&
        reflection.object != firstB && reflection.object != secondB) {
      continue;
    }
    auto const attribute =
        reflection.object == firstA || reflection.object == secondA
        ? observerAttributeA
        : observerAttributeB;
    require(
        reflection.values.size() == 1U &&
            reflection.values.count(attribute) == 1U &&
            reflection.transportation == reliable && reflection.tag.empty() &&
            reflection.producer == subject.federateHandle() &&
            !reflection.regions.has_value(),
        "Joined-federate MOM multi-class update callback had unexpected attribute or transport metadata");
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
      "Joined-federate MOM multi-class callbacks did not preserve all six updated values");

  auto const interactionsBeforeRequest =
      requester.recorder().interactions().size();
  rti::ParameterHandleValueMap const requestParameters{{
      requestFederateParameter,
      subjectFederate.encode()}};
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
      "the joined-federate MOM multi-class updated-object report");
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
      "A multi-class updated-object query did not deliver exactly one standard report");
  auto const& report = *reports.front();
  require(
      report.present && report.parameters.size() == 1U &&
          report.parameters.count(reportCountsParameter) == 1U &&
          report.transportation == reliable && report.tag.empty() &&
          !report.producer.isValid() && !report.regions.has_value(),
      "Joined-federate MOM multi-class updated-object report did not use its standard reliable response shape");
  auto const counts =
      decodeObjectClassCounts(report.parameters.at(reportCountsParameter));
  std::optional<std::int32_t> countA;
  std::optional<std::int32_t> countB;
  for (auto const& count : counts) {
    if (count.objectClass == subjectClassA) {
      require(!countA.has_value(), "The MIM repeated its first updated-class count record");
      countA = count.count;
    } else if (count.objectClass == subjectClassB) {
      require(!countB.has_value(), "The MIM repeated its second updated-class count record");
      countB = count.count;
    } else {
      require(
          false,
          "The MIM returned an updated-instance count for an untested class");
    }
  }
  require(
      counts.size() == 2U && countA == 2 && countB == 2,
      "Joined-federate MOM updated-object report did not map two distinct updated instances to each registered class");

  requesterAmbassador.unsubscribeInteractionClass(reportClass);
  observerAmbassador.unsubscribeObjectClassAttributes(
      observerClassA,
      rti::AttributeHandleSet{observerAttributeA});
  observerAmbassador.unsubscribeObjectClassAttributes(
      observerClassB,
      rti::AttributeHandleSet{observerAttributeB});
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
  observer.resign(rti::NO_ACTION);
  subject.resign(rti::NO_ACTION);
  requester.resign(rti::NO_ACTION);
  subjectAmbassador.destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
  requester.disconnect();
}

void scenarioJoinedFederateMomObjectInstancesUpdatedMultiClassReportContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioJoinedFederateMomObjectInstancesUpdatedMultiClassReport(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
