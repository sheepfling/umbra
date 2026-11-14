#include "tck_federate_mom_object_instance_information.hpp"

#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAvariableArray.h>

#include "tck_main_scenarios.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioFederateMomObjectInstanceInformation(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty() &&
          !options.objectClassName.empty() && !options.attributeName.empty(),
      "Federate MOM object-information reporting requires adapter-supplied FOM, standard MIM, object class, attribute, and logical-time configuration");

  Session requester(options, model, "member");
  Session owner(options, model, "owner");
  auto const federation = federationName(
      options,
      "federate-mom-object-instance-information");
  requester.connect();
  owner.connect();
  owner.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  auto const requesterName =
      options.memberFederateName + L"-mom-object-information-requester";
  auto const ownerName = options.ownerFederateName + L"-mom-object-information-owner";
  requester.join(requesterName, options.federateType, federation);
  owner.join(ownerName, options.federateType, federation);

  auto& ambassador = requester.rtiAmbassador();
  auto const requesterClass =
      ambassador.getObjectClassHandle(options.objectClassName);
  auto const requesterAttribute =
      ambassador.getAttributeHandle(requesterClass, options.attributeName);
  auto const requesterFederate = ambassador.getFederateHandle(requesterName);
  auto const ownerClass =
      owner.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  require(
      requesterClass.isValid() && requesterAttribute.isValid() &&
          requesterFederate.isValid() && ownerClass.isValid() &&
          ownerAttribute.isValid(),
      "Adapter-supplied object class or attribute lookup returned an invalid handle");
  ambassador.publishObjectClassAttributes(
      requesterClass,
      rti::AttributeHandleSet{requesterAttribute});
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerClass,
      rti::AttributeHandleSet{ownerAttribute});
  ambassador.subscribeObjectClassAttributes(
      requesterClass,
      rti::AttributeHandleSet{requesterAttribute},
      true);

  auto const reportClass = ambassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectInstanceInformation");
  auto const reportObjectParameter =
      ambassador.getParameterHandle(reportClass, L"HLAobjectInstance");
  auto const reportOwnedAttributesParameter =
      ambassador.getParameterHandle(reportClass, L"HLAownedInstanceAttributeList");
  auto const reportRegisteredClassParameter =
      ambassador.getParameterHandle(reportClass, L"HLAregisteredClass");
  auto const reportKnownClassParameter =
      ambassador.getParameterHandle(reportClass, L"HLAknownClass");
  auto const reportFederateParameter =
      ambassador.getParameterHandle(reportClass, L"HLAfederate");
  auto const requestClass = ambassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstanceInformation");
  auto const requestObjectParameter =
      ambassador.getParameterHandle(requestClass, L"HLAobjectInstance");
  auto const reliable = ambassador.getTransportationTypeHandle(L"HLAreliable");
  require(
      reportClass.isValid() && reportObjectParameter.isValid() &&
          reportOwnedAttributesParameter.isValid() &&
          reportRegisteredClassParameter.isValid() &&
          reportKnownClassParameter.isValid() &&
          reportFederateParameter.isValid() && requestClass.isValid() &&
          requestObjectParameter.isValid() && reliable.isValid(),
      "Standard MIM object-information request/report lookup returned an invalid handle");
  ambassador.subscribeInteractionClass(reportClass);

  auto decodeOwnedAttributes = [&](rti::VariableLengthData const& encoded) {
    // HLAattributeHandleList is a variable array whose elements are encoded
    // attribute-handle byte arrays, as declared by the standard MIM.
    rti::HLAvariableArray const attributeHandlePrototype{rti::HLAoctet{}};
    rti::HLAvariableArray handles{
        static_cast<rti::DataElement const&>(attributeHandlePrototype)};
    handles.decode(encoded);
    std::vector<rti::AttributeHandle> decoded;
    decoded.reserve(handles.size());
    for (std::size_t index = 0; index != handles.size(); ++index) {
      auto const& encodedAttribute =
          dynamic_cast<rti::HLAvariableArray const&>(handles.get(index));
      decoded.push_back(
          ambassador.decodeAttributeHandle(encodedAttribute.encode()));
    }
    return decoded;
  };
  auto requestInformation = [&](rti::ObjectInstanceHandle const& object,
                                std::string const& description) {
    requester.recorder().clearInteraction();
    rti::ParameterHandleValueMap const parameters{
        {requestObjectParameter, object.encode()}};
    ambassador.sendInteraction(
        requestClass,
        parameters,
        rti::VariableLengthData{});
    waitFor(
        requester,
        [&] { return !requester.recorder().interactions().empty(); },
        options,
        description);
    auto const reports = requester.recorder().interactions();
    require(
        reports.size() == 1U,
        description + " did not deliver exactly one object-information report");
    return reports.front();
  };
  auto requireReportBase = [&](InteractionRecord const& report,
                               rti::ObjectInstanceHandle const& object) {
    require(
        report.present && report.interaction == reportClass &&
            !report.producer.isValid() && report.transportation == reliable &&
            report.parameters.find(reportObjectParameter) != report.parameters.end() &&
            report.parameters.find(reportOwnedAttributesParameter) !=
                report.parameters.end(),
        "Object-information report was not RTI-originated or omitted its standard identity/list fields or reliable delivery");
    auto const inheritedFederateParameterCount =
        report.parameters.count(reportFederateParameter);
    require(
        report.parameters.size() == 2U +
                (report.parameters.count(reportRegisteredClassParameter) != 0U ? 1U : 0U) +
                (report.parameters.count(reportKnownClassParameter) != 0U ? 1U : 0U) +
                inheritedFederateParameterCount,
        "Object-information report included parameters outside its MIM-defined response shape");
    require(
        ambassador.decodeObjectInstanceHandle(
            report.parameters.at(reportObjectParameter)) == object,
        "Object-information report identified a different object instance");
    if (inheritedFederateParameterCount == 1U) {
      require(
          copyBytes(report.parameters.at(reportFederateParameter)) ==
              copyBytes(requesterFederate.encode()),
          "Object-information report identified a different joined federate");
    }
  };

  auto const ownedObject = ambassador.registerObjectInstance(requesterClass);
  require(ownedObject.isValid(), "Object-information test registration returned an invalid handle");
  auto const ownedReport = requestInformation(
      ownedObject,
      "the requesting federate's owned object-information report");
  requireReportBase(ownedReport, ownedObject);
  auto const ownedAttributes = decodeOwnedAttributes(
      ownedReport.parameters.at(reportOwnedAttributesParameter));
  require(
      std::find(ownedAttributes.begin(), ownedAttributes.end(), requesterAttribute) !=
          ownedAttributes.end(),
      "Object-information report omitted the requesting federate's owned application attribute");
  require(
      ownedReport.parameters.count(reportRegisteredClassParameter) == 1U &&
          ownedReport.parameters.count(reportKnownClassParameter) == 1U &&
          ambassador.decodeObjectClassHandle(
              ownedReport.parameters.at(reportRegisteredClassParameter)) == requesterClass &&
          ambassador.decodeObjectClassHandle(
              ownedReport.parameters.at(reportKnownClassParameter)) == requesterClass,
      "Owned object-information report returned the wrong registered or known class");

  auto const observedObject = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(observedObject.isValid(), "Remote object-information registration returned an invalid handle");
  waitFor(
      requester,
      [&] { return requester.recorder().hasDiscovery(observedObject); },
      options,
      "discovery of the object-information query target");
  require(
      ambassador.getKnownObjectClassHandle(observedObject) == requesterClass,
      "Object-information query target was discovered with a different class");

  auto const knownReport = requestInformation(
      observedObject,
      "the known object-information report");
  requireReportBase(knownReport, observedObject);
  require(
      decodeOwnedAttributes(
          knownReport.parameters.at(reportOwnedAttributesParameter)).empty(),
      "Known-object information report included attributes not owned by the requesting federate");
  require(
      knownReport.parameters.count(reportRegisteredClassParameter) == 1U &&
          knownReport.parameters.count(reportKnownClassParameter) == 1U &&
          ambassador.decodeObjectClassHandle(
              knownReport.parameters.at(reportRegisteredClassParameter)) == requesterClass &&
          ambassador.decodeObjectClassHandle(
              knownReport.parameters.at(reportKnownClassParameter)) == requesterClass,
      "Known-object information report returned the wrong registered or known class");

  ambassador.localDeleteObjectInstance(observedObject);
  auto const nullReport = requestInformation(
      observedObject,
      "the NULL object-information report after local deletion");
  requireReportBase(nullReport, observedObject);
  require(
      decodeOwnedAttributes(
          nullReport.parameters.at(reportOwnedAttributesParameter)).empty() &&
          nullReport.parameters.count(reportRegisteredClassParameter) == 0U &&
          nullReport.parameters.count(reportKnownClassParameter) == 0U,
      "NULL object-information report did not use the standard empty-list/class-omission shape");

  ambassador.unsubscribeInteractionClass(reportClass);
  ambassador.unsubscribeObjectClassAttributes(
      requesterClass,
      rti::AttributeHandleSet{requesterAttribute});
  requester.resign(rti::DELETE_OBJECTS);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioFederateMomObjectInstanceInformationContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomObjectInstanceInformation(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
