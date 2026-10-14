#include "tck_joined_federate_mom_deletable_object_report.hpp"
#include "tck_main_scenarios.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioJoinedFederateMomDeletableObjectReport(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Joined-federate MOM deletable-object reporting requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "Joined-federate MOM deletable-object reporting requires an adapter-supplied standard MIM");

  Session requester(options, model, "mom-deletable-requester");
  Session owner(options, model, "mom-deletable-owner");
  auto const federation = federationName(
      options,
      "joined-federate-mom-object-instances-that-can-be-deleted-report");
  requester.connect();
  owner.connect();
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

  requester.join(
      options.memberFederateName + L"-mom-deletable-requester",
      options.federateType,
      federation);
  auto const requestClass = requester.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstancesThatCanBeDeleted");
  auto const requestFederate = requester.rtiAmbassador().getParameterHandle(
      requestClass,
      L"HLAfederate");
  auto const reportClass = requester.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectInstancesThatCanBeDeleted");
  auto const reportCounts = requester.rtiAmbassador().getParameterHandle(
      reportClass,
      L"HLAobjectInstanceCounts");
  auto const reliable = requester.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      requestClass.isValid() && requestFederate.isValid() && reportClass.isValid() &&
          reportCounts.isValid() && reliable.isValid() &&
          requester.rtiAmbassador().getInteractionClassName(requestClass) ==
              L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstancesThatCanBeDeleted" &&
          requester.rtiAmbassador().getInteractionClassName(reportClass) ==
              L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectInstancesThatCanBeDeleted" &&
          requester.rtiAmbassador().getParameterName(requestClass, requestFederate) ==
              L"HLAfederate" &&
          requester.rtiAmbassador().getParameterName(reportClass, reportCounts) ==
              L"HLAobjectInstanceCounts" &&
          requester.rtiAmbassador().getTransportationTypeName(reliable) ==
              L"HLAreliable",
      "Joined-federate MOM deletable-object report lookup did not round-trip standard handles");
  requester.rtiAmbassador().subscribeInteractionClass(reportClass);

  owner.join(
      options.ownerFederateName + L"-mom-deletable-owner",
      options.federateType,
      federation);
  auto const ownerObjectClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerObjectClass,
      options.attributeName);
  require(
      ownerObjectClass.isValid() && ownerAttribute.isValid(),
      "Joined-federate MOM deletable-object application lookup returned an invalid handle");
  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerObjectClass,
      ownerAttributes);

  auto decodeObjectClassCount = [&](rti::VariableLengthData const& encoded)
      -> std::int32_t {
    rti::HLAvariableArrayT<rti::HLAoctet> objectClassHandlePrototype;
    rti::HLAfixedRecord recordPrototype;
    recordPrototype.appendElement(objectClassHandlePrototype)
        .appendElement(rti::HLAinteger32BE{});
    rti::HLAvariableArray counts{recordPrototype};
    counts.decode(encoded);
    for (std::size_t index = 0; index != counts.size(); ++index) {
      auto const& record = dynamic_cast<rti::HLAfixedRecord const&>(counts.get(index));
      auto const& encodedObjectClass =
          dynamic_cast<rti::HLAvariableArray const&>(record.get(0));
      auto const decodedObjectClass = requester.rtiAmbassador().decodeObjectClassHandle(
          encodedObjectClass.encode());
      auto const& count = dynamic_cast<rti::HLAinteger32BE const&>(record.get(1));
      if (decodedObjectClass == ownerObjectClass) {
        return count.get();
      }
    }
    return 0;
  };

  auto requestCount = [&](std::int32_t expected, std::string const& description) {
    auto const before = requester.recorder().interactions().size();
    requester.rtiAmbassador().sendInteraction(
        requestClass,
        rti::ParameterHandleValueMap{
            {requestFederate, owner.federateHandle().encode()}},
        rti::VariableLengthData{});
    waitFor(
        requester,
        [&] {
          auto const interactions = requester.recorder().interactions();
          return interactions.size() > before &&
              std::any_of(
                  interactions.begin() + static_cast<std::ptrdiff_t>(before),
                  interactions.end(),
                  [&](InteractionRecord const& interaction) {
                    return interaction.present &&
                        interaction.interaction == reportClass &&
                        interaction.parameters.count(reportCounts) == 1U;
                  });
        },
        options,
        description);
    auto const interactions = requester.recorder().interactions();
    auto const iterator = std::find_if(
        interactions.begin() + static_cast<std::ptrdiff_t>(before),
        interactions.end(),
        [&](InteractionRecord const& interaction) {
          return interaction.present && interaction.interaction == reportClass &&
              interaction.parameters.count(reportCounts) == 1U;
        });
    require(iterator != interactions.end(), description + " produced no report");
    require(
        iterator->parameters.size() == 1U && iterator->transportation == reliable &&
            iterator->tag.empty() && !iterator->producer.isValid() &&
            !iterator->regions.has_value(),
        description + " returned non-standard MOM report metadata");
    require(
        decodeObjectClassCount(iterator->parameters.at(reportCounts)) == expected,
        description + " returned the wrong deletable-object count");
  };

  auto const firstObject = owner.rtiAmbassador().registerObjectInstance(ownerObjectClass);
  auto const secondObject = owner.rtiAmbassador().registerObjectInstance(ownerObjectClass);
  require(
      firstObject.isValid() && secondObject.isValid(),
      "Joined-federate MOM deletable-object registration returned an invalid handle");
  requestCount(
      2,
      "joined-federate MOM deletable-object report with two live objects");
  owner.rtiAmbassador().deleteObjectInstance(
      firstObject,
      rti::VariableLengthData{});
  requestCount(
      1,
      "joined-federate MOM deletable-object report after one deletion");
  owner.rtiAmbassador().deleteObjectInstance(
      secondObject,
      rti::VariableLengthData{});
  requestCount(
      0,
      "joined-federate MOM deletable-object report after all deletions");

  requester.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  owner.rtiAmbassador().unpublishObjectClassAttributes(
      ownerObjectClass,
      ownerAttributes);
  requester.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioJoinedFederateMomDeletableObjectReportContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioJoinedFederateMomDeletableObjectReport(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
