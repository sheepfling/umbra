#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "internal/handles/attribute_handle.hpp"
#include "internal/handles/object_class_handle.hpp"
#include "internal/handles/object_instance_handle.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAvariableArray.h>

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The joined-federate MOM object-information report tests require the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::DataElement;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::HLAoctet;
using rti1516_2025::HLAvariableArray;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

using rti1516_2025::umbra_binding_detail::decodeAttributeHandle;
using rti1516_2025::umbra_binding_detail::decodeObjectInstanceHandle;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"federation-joined-mom-object-instance-information-report-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path restaurantFom() {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / "examples" /
      "RestaurantFOMmodule-2025.xml";
}

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

void drainCallbacks(RTIambassador& rti) {
  for (int pass = 0; pass != 256; ++pass) {
    if (!rti.evokeCallback(0.0)) {
      break;
    }
  }
}

class ReportingFederateAmbassador final : public NullFederateAmbassador {
 public:
  struct InteractionReport final {
    InteractionClassHandle interactionClass;
    ParameterHandleValueMap parameterValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    bool sentRegionsSupplied = false;
  };

  struct ObjectDiscoveryReport final {
    ObjectInstanceHandle objectInstance;
    ObjectClassHandle objectClass;
    std::wstring objectInstanceName;
    FederateHandle producingFederate;
  };

  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      rti1516_2025::RegionHandleSet const* optionalSentRegions) override {
    interactionReports.push_back({
        interactionClass,
        parameterValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        optionalSentRegions != nullptr,
    });
  }

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const& objectClass,
      std::wstring const& objectInstanceName,
      FederateHandle const& producingFederate) override {
    objectDiscoveryReports.push_back({
        objectInstance,
        objectClass,
        objectInstanceName,
        producingFederate,
    });
  }

  std::vector<InteractionReport> interactionReports;
  std::vector<ObjectDiscoveryReport> objectDiscoveryReports;
};

std::vector<AttributeHandle> decodeAttributeHandleList(
    VariableLengthData const& encodedValue) {
  // HLAattributeHandleList is the official nested variable-array value:
  // each element is an encoded AttributeHandle byte array.
  HLAvariableArray attributeHandlePrototype{HLAoctet{}};
  HLAvariableArray handles{
      static_cast<DataElement const&>(attributeHandlePrototype)};
  handles.decode(encodedValue);
  std::vector<AttributeHandle> decoded;
  decoded.reserve(handles.size());
  for (std::size_t index = 0; index != handles.size(); ++index) {
    auto const& encodedAttribute =
        dynamic_cast<HLAvariableArray const&>(handles.get(index));
    decoded.push_back(decodeAttributeHandle(encodedAttribute.encode()));
  }
  return decoded;
}

}  // namespace

TEST_CASE(
    "Embedded MOM requestObjectInstanceInformation reports known and NULL object state",
    "[integration][development-profile][federation-management][mom]"
    "[mom-request-report][object-management][interaction-management]"
    "[callback-model][joined-federate-mom-object-instance-information-report]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.register-object-instance][rti.service.local-delete-object-instance]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.subscribe-interaction-class][rti.service.send-interaction]"
    "[rti.service.evoke-callback][federate.callback.discover-object-instance]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador requesterReports;
  ReportingFederateAmbassador ownerReports;
  auto requester = makeRti();
  auto owner = makeRti();
  auto const federationName = nextFederationName();

  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName, restaurantFom().wstring(), standard_hla::mom::integer64_time));
  auto const requesterFederate = requester->joinFederationExecution(
      L"mom-object-instance-information-requester", L"requester", federationName);
  auto const ownerFederate = owner->joinFederationExecution(
      L"mom-object-instance-information-owner", L"owner", federationName);

  auto const requesterServer = requester->getObjectClassHandle(
      L"HLAobjectRoot.Employee.Server");
  auto const requesterEfficiency = requester->getAttributeHandle(
      requesterServer, L"Efficiency");
  auto const requesterPrivilege = requester->getAttributeHandle(
      requesterServer, L"HLAprivilegeToDeleteObject");
  auto const ownerServer = owner->getObjectClassHandle(
      L"HLAobjectRoot.Employee.Server");
  auto const ownerEfficiency = owner->getAttributeHandle(ownerServer, L"Efficiency");
  REQUIRE(requesterServer.isValid());
  REQUIRE(requesterEfficiency.isValid());
  REQUIRE(requesterPrivilege.isValid());
  REQUIRE(ownerServer.isValid());
  REQUIRE(ownerEfficiency.isValid());
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(
      requesterServer, AttributeHandleSet{requesterEfficiency}));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
      ownerServer, AttributeHandleSet{ownerEfficiency}));
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(
      requesterServer, AttributeHandleSet{requesterEfficiency}, true));

  auto const reportClass = requester->getInteractionClassHandle(
      standard_hla::mom::report_object_instance_information);
  auto const reportObject = requester->getParameterHandle(
      reportClass, standard_hla::mom::object_instance);
  auto const reportOwnedAttributes = requester->getParameterHandle(
      reportClass, standard_hla::mom::owned_instance_attribute_list);
  auto const reportRegisteredClass = requester->getParameterHandle(
      reportClass, standard_hla::mom::registered_class);
  auto const reportKnownClass = requester->getParameterHandle(
      reportClass, standard_hla::mom::known_class);
  auto const requestClass = requester->getInteractionClassHandle(
      standard_hla::mom::request_object_instance_information);
  auto const requestObject = requester->getParameterHandle(
      requestClass, standard_hla::mom::object_instance);
  REQUIRE(reportClass.isValid());
  REQUIRE(reportObject.isValid());
  REQUIRE(reportOwnedAttributes.isValid());
  REQUIRE(reportRegisteredClass.isValid());
  REQUIRE(reportKnownClass.isValid());
  REQUIRE(requestClass.isValid());
  REQUIRE(requestObject.isValid());
  REQUIRE_NOTHROW(requester->subscribeInteractionClass(reportClass));

  // The registering federate's own object carries Efficiency and the
  // implicitly published HLAprivilegeToDeleteObject attribute.
  auto const ownedObject = requester->registerObjectInstance(requesterServer);
  REQUIRE(ownedObject.isValid());
  REQUIRE_NOTHROW(requester->sendInteraction(
      requestClass,
      ParameterHandleValueMap{{requestObject, ownedObject.encode()}},
      VariableLengthData{}));
  REQUIRE(requesterReports.interactionReports.empty());
  drainCallbacks(*requester);
  REQUIRE(requesterReports.interactionReports.size() == 1U);
  auto const& ownedReport = requesterReports.interactionReports.back();
  REQUIRE(ownedReport.interactionClass == reportClass);
  REQUIRE(ownedReport.parameterValues.size() == 4U);
  REQUIRE(ownedReport.parameterValues.contains(reportObject));
  REQUIRE(ownedReport.parameterValues.contains(reportOwnedAttributes));
  REQUIRE(ownedReport.parameterValues.contains(reportRegisteredClass));
  REQUIRE(ownedReport.parameterValues.contains(reportKnownClass));
  REQUIRE(decodeObjectInstanceHandle(
              ownedReport.parameterValues.at(reportObject)) == ownedObject);
  auto const ownedAttributes = decodeAttributeHandleList(
      ownedReport.parameterValues.at(reportOwnedAttributes));
  REQUIRE(ownedAttributes.size() == 2U);
  REQUIRE(std::find(ownedAttributes.begin(), ownedAttributes.end(), requesterEfficiency) !=
          ownedAttributes.end());
  REQUIRE(std::find(ownedAttributes.begin(), ownedAttributes.end(), requesterPrivilege) !=
          ownedAttributes.end());
  REQUIRE(rti1516_2025::umbra_binding_detail::decodeObjectClassHandle(
              ownedReport.parameterValues.at(reportRegisteredClass)) == requesterServer);
  REQUIRE(rti1516_2025::umbra_binding_detail::decodeObjectClassHandle(
              ownedReport.parameterValues.at(reportKnownClass)) == requesterServer);
  REQUIRE(ownedReport.userSuppliedTag.size() == 0U);
  REQUIRE(ownedReport.producingFederate.isValid() == false);
  REQUIRE(ownedReport.sentRegionsSupplied == false);
  requesterReports.interactionReports.clear();

  // A separate owner-created object establishes requester-local knowledge
  // without requester-owned attributes, making Local Delete Object Instance
  // a valid way to transition the requester's report to the MIM NULL shape.
  auto const observedObject = owner->registerObjectInstance(ownerServer);
  REQUIRE(observedObject.isValid());
  drainCallbacks(*requester);
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(requesterReports.objectDiscoveryReports.front().objectInstance == observedObject);
  REQUIRE(requesterReports.objectDiscoveryReports.front().objectClass == requesterServer);
  REQUIRE(requesterReports.objectDiscoveryReports.front().producingFederate == ownerFederate);

  REQUIRE_NOTHROW(requester->sendInteraction(
      requestClass,
      ParameterHandleValueMap{{requestObject, observedObject.encode()}},
      VariableLengthData{}));
  drainCallbacks(*requester);
  REQUIRE(requesterReports.interactionReports.size() == 1U);
  auto const& knownReport = requesterReports.interactionReports.back();
  REQUIRE(knownReport.parameterValues.size() == 4U);
  REQUIRE(decodeObjectInstanceHandle(
              knownReport.parameterValues.at(reportObject)) == observedObject);
  REQUIRE(decodeAttributeHandleList(
              knownReport.parameterValues.at(reportOwnedAttributes)).empty());
  REQUIRE(rti1516_2025::umbra_binding_detail::decodeObjectClassHandle(
              knownReport.parameterValues.at(reportRegisteredClass)) == ownerServer);
  REQUIRE(rti1516_2025::umbra_binding_detail::decodeObjectClassHandle(
              knownReport.parameterValues.at(reportKnownClass)) == requesterServer);
  requesterReports.interactionReports.clear();

  REQUIRE_NOTHROW(requester->localDeleteObjectInstance(observedObject));
  REQUIRE_THROWS_AS(
      requester->getKnownObjectClassHandle(observedObject),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_NOTHROW(requester->sendInteraction(
      requestClass,
      ParameterHandleValueMap{{requestObject, observedObject.encode()}},
      VariableLengthData{}));
  REQUIRE(requesterReports.interactionReports.empty());
  drainCallbacks(*requester);
  REQUIRE(requesterReports.interactionReports.size() == 1U);
  auto const& nullReport = requesterReports.interactionReports.back();
  REQUIRE(nullReport.interactionClass == reportClass);
  REQUIRE(nullReport.parameterValues.size() == 2U);
  REQUIRE(nullReport.parameterValues.contains(reportObject));
  REQUIRE(nullReport.parameterValues.contains(reportOwnedAttributes));
  REQUIRE_FALSE(nullReport.parameterValues.contains(reportRegisteredClass));
  REQUIRE_FALSE(nullReport.parameterValues.contains(reportKnownClass));
  REQUIRE(decodeObjectInstanceHandle(
              nullReport.parameterValues.at(reportObject)) == observedObject);
  REQUIRE(decodeAttributeHandleList(
              nullReport.parameterValues.at(reportOwnedAttributes)).empty());
  REQUIRE(nullReport.userSuppliedTag.size() == 0U);
  REQUIRE(nullReport.producingFederate.isValid() == false);
  REQUIRE(nullReport.sentRegionsSupplied == false);

  REQUIRE_NOTHROW(requester->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
