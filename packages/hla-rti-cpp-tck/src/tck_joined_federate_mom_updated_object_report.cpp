#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAvariableArray.h>

#include "tck_harness.hpp"
#include "tck_main_scenarios.hpp"
#include "tck_joined_federate_mom_updated_object_report.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioJoinedFederateMomObjectInstancesUpdatedReport(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.objectClassName.empty() && !options.attributeName.empty(),
      "Joined-federate MOM updated-object reporting requires adapter-supplied FOM, standard MIM, object class, and attribute configuration");

  Session requester(options, model, "mom-updated-requester");
  Session subject(options, model, "mom-updated-subject");
  auto const federation =
      federationName(options, "joined-federate-mom-object-instances-updated-report");
  requester.connect();
  subject.connect();
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
      options.ownerFederateName + L"-mom-updated-subject";
  auto const requesterName =
      options.memberFederateName + L"-mom-updated-requester";
  subject.join(subjectName, options.federateType, federation);
  requester.join(requesterName, options.federateType, federation);
  auto& requesterAmbassador = requester.rtiAmbassador();
  auto& subjectAmbassador = subject.rtiAmbassador();

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
  auto const subjectObjectClass =
      subjectAmbassador.getObjectClassHandle(options.objectClassName);
  auto const subjectAttribute = subjectAmbassador.getAttributeHandle(
      subjectObjectClass,
      options.attributeName);
  auto const requesterObjectClass =
      requesterAmbassador.getObjectClassHandle(options.objectClassName);
  require(
      requestClass.isValid() && requestFederateParameter.isValid() &&
          reportClass.isValid() && reportCountsParameter.isValid() &&
          targetFederate.isValid() && reliable.isValid() &&
          subjectObjectClass.isValid() && subjectAttribute.isValid() &&
          requesterObjectClass.isValid(),
      "Joined-federate MOM updated-object request/report or adapter-supplied application lookup returned an invalid handle");
  requesterAmbassador.subscribeInteractionClass(reportClass);
  subjectAmbassador.publishObjectClassAttributes(
      subjectObjectClass,
      rti::AttributeHandleSet{subjectAttribute});

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

  auto const firstObject = subjectAmbassador.registerObjectInstance(subjectObjectClass);
  auto const secondObject = subjectAmbassador.registerObjectInstance(subjectObjectClass);
  require(
      firstObject.isValid() && secondObject.isValid(),
      "Joined-federate MOM updated-object registration returned an invalid handle");
  auto updateObject = [&](rti::ObjectInstanceHandle const& object,
                          std::int32_t value) {
    rti::AttributeHandleValueMap const values{{
        subjectAttribute,
        rti::HLAbyte{static_cast<rti::Octet>(value)}.encode()}};
    subjectAmbassador.updateAttributeValues(
        object,
        values,
        rti::VariableLengthData{});
  };
  // Repeated updates to one instance must not inflate the class-based count.
  updateObject(firstObject, 1);
  updateObject(firstObject, 2);
  updateObject(secondObject, 3);

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
      "the joined-federate MOM updated-object report");

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
      "An updated-object publication query did not deliver exactly one standard report");
  auto const& report = *reports.front();
  require(
      report.present && report.parameters.size() == 1U &&
          report.parameters.count(reportCountsParameter) == 1U &&
          report.transportation == reliable && report.tag.empty() &&
          !report.producer.isValid() && !report.regions.has_value(),
      "Joined-federate MOM updated-object report did not use its standard reliable one-parameter response shape");
  auto const counts =
      decodeObjectClassCounts(report.parameters.at(reportCountsParameter));
  require(
      counts.size() == 1U && counts.front().objectClass == requesterObjectClass &&
          counts.front().count == 2,
      "Joined-federate MOM updated-object report did not count the two updated instances once by registered class");

  requesterAmbassador.unsubscribeInteractionClass(reportClass);
  subjectAmbassador.deleteObjectInstance(firstObject, rti::VariableLengthData{});
  subjectAmbassador.deleteObjectInstance(secondObject, rti::VariableLengthData{});
  subjectAmbassador.unpublishObjectClassAttributes(
      subjectObjectClass,
      rti::AttributeHandleSet{subjectAttribute});
  requester.resign(rti::NO_ACTION);
  subject.resign(rti::NO_ACTION);
  subjectAmbassador.destroyFederationExecution(federation);
  requester.disconnect();
  subject.disconnect();
}

void scenarioJoinedFederateMomObjectInstancesUpdatedReportContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioJoinedFederateMomObjectInstancesUpdatedReport(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
