#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAvariableArray.h>

#include "tck_harness.hpp"
#include "tck_main_scenarios.hpp"
#include "tck_joined_federate_mom_reflected_null_object_report.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioJoinedFederateMomObjectInstancesReflectedNullReport(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty(),
      "Joined-federate MOM zero-reflection reporting requires adapter-supplied FOM and standard MIM modules");

  Session subject(options, model, "mom-reflected-null-subject");
  Session requester(options, model, "mom-reflected-null-requester");
  auto const federation = federationName(
      options,
      "joined-federate-mom-object-instances-reflected-null-report");
  subject.connect();
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

  auto const subjectName = options.ownerFederateName + L"-mom-reflected-null-subject";
  subject.join(subjectName, options.federateType, federation);
  requester.join(
      options.memberFederateName + L"-mom-reflected-null-requester",
      options.federateType,
      federation);
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
  auto const subjectFederate = subject.federateHandle();
  auto const reliable =
      requesterAmbassador.getTransportationTypeHandle(L"HLAreliable");
  require(
      requestClass.isValid() && requestFederateParameter.isValid() &&
          reportClass.isValid() && reportCountsParameter.isValid() &&
          subjectFederate.isValid() && reliable.isValid(),
      "Joined-federate MOM zero-reflection request/report lookup returned an invalid standard handle");
  requesterAmbassador.subscribeInteractionClass(reportClass);

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
                      interaction.interaction == reportClass;
                });
      },
      options,
      "the standard NULL HLAreportObjectInstancesReflected response");

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
      "A joined subject with no reflect callbacks did not produce exactly one standard NULL report");
  auto const& report = *reports.front();
  require(
      report.present && report.parameters.size() == 1U &&
          report.parameters.count(reportCountsParameter) == 1U &&
          report.transportation == reliable && report.tag.empty() &&
          !report.producer.isValid() && !report.regions.has_value(),
      "The standard zero-reflection report did not preserve its reliable one-parameter NULL response shape");

  rti::HLAvariableArrayT<rti::HLAoctet> objectClassHandlePrototype;
  rti::HLAfixedRecord countRecordPrototype;
  countRecordPrototype.appendElement(objectClassHandlePrototype)
      .appendElement(rti::HLAinteger32BE{});
  rti::HLAvariableArray decodedCounts{countRecordPrototype};
  auto const& encodedCounts = report.parameters.at(reportCountsParameter);
  if (encodedCounts.size() != 0U) {
    decodedCounts.decode(encodedCounts);
  }
  require(
      decodedCounts.size() == 0U,
      "The standard NULL reflection report did not expose an undefined or empty HLAobjectClassBasedCounts value");

  requesterAmbassador.unsubscribeInteractionClass(reportClass);
  requester.resign(rti::NO_ACTION);
  subject.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  subject.disconnect();
}

void scenarioJoinedFederateMomObjectInstancesReflectedNullReportContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioJoinedFederateMomObjectInstancesReflectedNullReport(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
