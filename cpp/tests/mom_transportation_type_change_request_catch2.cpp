#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/BasicDataElements.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The MOM transportation-type request test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::FederateHandle;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::LogicalTime;
using rti1516_2025::MessageRetractionHandle;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::OrderType;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RECEIVE;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"mom-transportation-type-change-request-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / relativePath;
}

std::vector<unsigned char> variableLengthDataBytes(
    VariableLengthData const& value) {
  auto const* bytes = static_cast<unsigned char const*>(value.data());
  if (bytes == nullptr || value.size() == 0U) {
    return {};
  }
  return {bytes, bytes + value.size()};
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
    RegionHandleSet sentRegions;
  };

  struct AttributeReflectionReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleValueMap attributeValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    bool sentRegionsSupplied = false;
    RegionHandleSet sentRegions;
  };

  struct AttributeTransportationTypeChangeReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    TransportationTypeHandle transportationType;
  };

  struct InteractionTransportationTypeChangeReport final {
    InteractionClassHandle interactionClass;
    TransportationTypeHandle transportationType;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const&,
      std::wstring const&,
      FederateHandle const&) override {
    objectDiscoveryReports.push_back(objectInstance);
  }

  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      RegionHandleSet const* optionalSentRegions) override {
    interactionReports.push_back({
        interactionClass,
        parameterValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        optionalSentRegions != nullptr,
        optionalSentRegions == nullptr ? RegionHandleSet{} : *optionalSentRegions,
    });
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      RegionHandleSet const* optionalSentRegions) override {
    attributeReflectionReports.push_back({
        objectInstance,
        attributeValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        optionalSentRegions != nullptr,
        optionalSentRegions == nullptr ? RegionHandleSet{} : *optionalSentRegions,
    });
  }

  void confirmAttributeTransportationTypeChange(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      TransportationTypeHandle const& transportationType) override {
    attributeTransportationTypeChangeReports.push_back({
        objectInstance,
        attributes,
        transportationType,
    });
  }

  void confirmInteractionTransportationTypeChange(
      InteractionClassHandle const& interactionClass,
      TransportationTypeHandle const& transportationType) override {
    interactionTransportationTypeChangeReports.push_back({
        interactionClass,
        transportationType,
    });
  }

  std::vector<ObjectInstanceHandle> objectDiscoveryReports;
  std::vector<InteractionReport> interactionReports;
  std::vector<AttributeReflectionReport> attributeReflectionReports;
  std::vector<AttributeTransportationTypeChangeReport>
      attributeTransportationTypeChangeReports;
  std::vector<InteractionTransportationTypeChangeReport>
      interactionTransportationTypeChangeReports;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

void pump(RTIambassador& rti, rti1516_2025::CallbackModel callbackModel) {
  if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
    return;
  }
  for (int pass = 0; pass != 128; ++pass) {
    if (!rti.evokeCallback(0.0)) {
      break;
    }
  }
}

}  // namespace

TEST_CASE(
    "Embedded MOM transportation-type request interactions invoke public changes",
    "[integration][development-profile][federation-management][mom]"
    "[mom-transportation-type-change-request]"
    "[transportation-management][object-management][interaction-management]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.publish-object-class-attributes][rti.service.register-object-instance]"
    "[rti.service.publish-interaction-class][rti.service.subscribe-object-class-attributes]"
    "[rti.service.subscribe-interaction-class][rti.service.send-interaction]"
    "[rti.service.request-attribute-transportation-type-change]"
    "[rti.service.request-interaction-transportation-type-change]"
    "[rti.service.evoke-callback]"
    "[federate.callback.confirm-attribute-transportation-type-change]"
    "[federate.callback.confirm-interaction-transportation-type-change]"
    "[federate.callback.reflect-attribute-values][federate.callback.receive-interaction]") {
  auto runScenario = [](auto const callbackModel) {
    ReportingFederateAmbassador subjectReports;
    ReportingFederateAmbassador observerReports;
    auto subject = makeRti();
    auto observer = makeRti();
    bool const immediate = callbackModel == rti1516_2025::HLA_IMMEDIATE;
    auto const federationName = nextFederationName();
    auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

    REQUIRE_NOTHROW(subject->connect(subjectReports, callbackModel));
    REQUIRE_NOTHROW(observer->connect(observerReports, callbackModel));
    REQUIRE_NOTHROW(subject->createFederationExecution(
        federationName,
        fomModule,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(subject->joinFederationExecution(
        L"mom-transport-request-subject",
        L"subject",
        federationName));
    REQUIRE_NOTHROW(observer->joinFederationExecution(
        L"mom-transport-request-observer",
        L"observer",
        federationName));

    // Keep this focused on the MIM control path; service-report interactions
    // are enabled only for the final explicit report assertion.
    REQUIRE_NOTHROW(subject->setServiceReportingSwitch(false));
    REQUIRE_NOTHROW(subject->setSendServiceReportsToFileSwitch(false));
    REQUIRE_NOTHROW(observer->setServiceReportingSwitch(false));
    REQUIRE_NOTHROW(observer->setSendServiceReportsToFileSwitch(false));

    auto const serverClass = subject->getObjectClassHandle(
        fixture_hla::fom::employee_server);
    auto const serverAttribute = subject->getAttributeHandle(
        serverClass,
        fixture_hla::fixture::efficiency);
    auto const observerServerClass = observer->getObjectClassHandle(
        fixture_hla::fom::employee_server);
    auto const observerServerAttribute = observer->getAttributeHandle(
        observerServerClass,
        fixture_hla::fixture::efficiency);
    auto const takeOrder = subject->getInteractionClassHandle(
        fixture_hla::fom::server_take_order);
    auto const observerTakeOrder = observer->getInteractionClassHandle(
        fixture_hla::fom::server_take_order);
    auto const reliable = subject->getTransportationTypeHandle(
        standard_hla::mom::reliable);
    auto const bestEffort = subject->getTransportationTypeHandle(
        standard_hla::mom::best_effort);
    REQUIRE(serverClass.isValid());
    REQUIRE(serverAttribute.isValid());
    REQUIRE(observerServerClass.isValid());
    REQUIRE(observerServerAttribute.isValid());
    REQUIRE(takeOrder.isValid());
    REQUIRE(observerTakeOrder.isValid());
    REQUIRE(reliable.isValid());
    REQUIRE(bestEffort.isValid());
    REQUIRE_NOTHROW(subject->publishObjectClassAttributes(
        serverClass,
        AttributeHandleSet{serverAttribute}));
    REQUIRE_NOTHROW(observer->subscribeObjectClassAttributes(
        observerServerClass,
        AttributeHandleSet{observerServerAttribute}));
    REQUIRE_NOTHROW(subject->publishInteractionClass(takeOrder));
    REQUIRE_NOTHROW(observer->subscribeInteractionClass(observerTakeOrder));
    // Keep a second publisher active so the MIM change proves its invoker
    // scope. The observer retains its FOM-selected reliable default.
    REQUIRE_NOTHROW(subject->subscribeInteractionClass(takeOrder));
    REQUIRE_NOTHROW(observer->publishInteractionClass(observerTakeOrder));

    ObjectInstanceHandle objectInstance;
    REQUIRE_NOTHROW(objectInstance = subject->registerObjectInstance(serverClass));
    pump(*observer, callbackModel);
    REQUIRE(observerReports.objectDiscoveryReports.size() == 1U);
    observerReports.attributeReflectionReports.clear();

    // Establish the second publisher's baseline before the subject requests a
    // transportation change. The subject is the recipient because the sender
    // is excluded from ordinary interaction delivery.
    REQUIRE_NOTHROW(observer->sendInteraction(
        observerTakeOrder,
        ParameterHandleValueMap{},
        VariableLengthData{}));
    pump(*subject, callbackModel);
    REQUIRE(subjectReports.interactionReports.size() == 1U);
    REQUIRE(subjectReports.interactionReports.back().interactionClass == takeOrder);
    REQUIRE(subjectReports.interactionReports.back().transportationType == reliable);
    subjectReports.interactionReports.clear();

    // HLAattributeHandleList is an official HLAvariableArray<HLAoctet> payload:
    // four octets of element count followed by the encoded handle.
    VariableLengthData encodedAttribute;
    serverAttribute.encode(encodedAttribute);
    auto const encodedAttributeBytes = variableLengthDataBytes(encodedAttribute);
    std::vector<rti1516_2025::Octet> encodedAttributeListBytes{
        0x00U, 0x00U, 0x00U, 0x01U};
    encodedAttributeListBytes.insert(
        encodedAttributeListBytes.end(),
        encodedAttributeBytes.begin(),
        encodedAttributeBytes.end());
    VariableLengthData attributeList(
        encodedAttributeListBytes.data(),
        encodedAttributeListBytes.size());

    auto const requestAttributeClass = subject->getInteractionClassHandle(
        standard_hla::mom::request_attribute_transportation_type_change);
    auto const requestAttributeObject = subject->getParameterHandle(
        requestAttributeClass,
        standard_hla::mom::object_instance);
    auto const requestAttributeList = subject->getParameterHandle(
        requestAttributeClass,
        standard_hla::mom::attribute_list);
    auto const requestAttributeTransportation = subject->getParameterHandle(
        requestAttributeClass,
        standard_hla::mom::transportation);
    REQUIRE(requestAttributeClass.isValid());
    REQUIRE(requestAttributeObject.isValid());
    REQUIRE(requestAttributeList.isValid());
    REQUIRE(requestAttributeTransportation.isValid());

    REQUIRE_NOTHROW(subject->sendInteraction(
        requestAttributeClass,
        ParameterHandleValueMap{
            {requestAttributeObject, objectInstance.encode()},
            {requestAttributeList, attributeList},
            {requestAttributeTransportation, bestEffort.encode()}},
        VariableLengthData{}));
    if (!immediate) {
      REQUIRE(subjectReports.attributeTransportationTypeChangeReports.empty());
    }
    pump(*subject, callbackModel);
    REQUIRE(subjectReports.attributeTransportationTypeChangeReports.size() == 1U);
    REQUIRE(subjectReports.attributeTransportationTypeChangeReports.front().objectInstance ==
            objectInstance);
    REQUIRE(subjectReports.attributeTransportationTypeChangeReports.front().attributes ==
            AttributeHandleSet{serverAttribute});
    REQUIRE(subjectReports.attributeTransportationTypeChangeReports.front().transportationType ==
            bestEffort);

    // Missing and malformed official MIM payloads fail at the MOM boundary
    // without enqueueing another confirmation or mutating the pending plan.
    REQUIRE_THROWS_AS(
        subject->sendInteraction(
            requestAttributeClass,
            ParameterHandleValueMap{
                {requestAttributeObject, objectInstance.encode()},
                {requestAttributeTransportation, bestEffort.encode()}},
            VariableLengthData{}),
        rti1516_2025::InteractionParameterNotDefined);
    REQUIRE(subjectReports.attributeTransportationTypeChangeReports.size() == 1U);
    std::vector<rti1516_2025::Octet> malformedAttributeListBytes{
        0x00U, 0x00U, 0x00U, 0x01U, 0x00U};
    VariableLengthData malformedAttributeList(
        malformedAttributeListBytes.data(),
        malformedAttributeListBytes.size());
    REQUIRE_THROWS_AS(
        subject->sendInteraction(
            requestAttributeClass,
            ParameterHandleValueMap{
                {requestAttributeObject, objectInstance.encode()},
                {requestAttributeList, malformedAttributeList},
                {requestAttributeTransportation, bestEffort.encode()}},
            VariableLengthData{}),
        rti1516_2025::RTIinternalError);
    auto unknownTransportationBytes = variableLengthDataBytes(bestEffort.encode());
    REQUIRE_FALSE(unknownTransportationBytes.empty());
    unknownTransportationBytes.back() = 0x7FU;
    VariableLengthData unknownTransportation(
        unknownTransportationBytes.data(),
        unknownTransportationBytes.size());
    REQUIRE_THROWS_AS(
        subject->sendInteraction(
            requestAttributeClass,
            ParameterHandleValueMap{
                {requestAttributeObject, objectInstance.encode()},
                {requestAttributeList, attributeList},
                {requestAttributeTransportation, unknownTransportation}},
            VariableLengthData{}),
        rti1516_2025::InvalidTransportationTypeHandle);
    REQUIRE(subjectReports.attributeTransportationTypeChangeReports.size() == 1U);

    unsigned char const valueBytes[] = {0xA1U};
    REQUIRE_NOTHROW(subject->updateAttributeValues(
        objectInstance,
        AttributeHandleValueMap{{
            serverAttribute,
            VariableLengthData(valueBytes, sizeof(valueBytes))}},
        VariableLengthData{}));
    pump(*observer, callbackModel);
    REQUIRE(observerReports.attributeReflectionReports.size() == 1U);
    REQUIRE(observerReports.attributeReflectionReports.back().transportationType == bestEffort);

    auto const requestInteractionClass = subject->getInteractionClassHandle(
        standard_hla::mom::request_interaction_transportation_type_change);
    auto const requestInteractionTarget = subject->getParameterHandle(
        requestInteractionClass,
        standard_hla::mom::interaction_class);
    auto const requestInteractionTransportation = subject->getParameterHandle(
        requestInteractionClass,
        standard_hla::mom::transportation);
    REQUIRE(requestInteractionClass.isValid());
    REQUIRE(requestInteractionTarget.isValid());
    REQUIRE(requestInteractionTransportation.isValid());
    REQUIRE_NOTHROW(subject->sendInteraction(
        requestInteractionClass,
        ParameterHandleValueMap{
            {requestInteractionTarget, takeOrder.encode()},
            {requestInteractionTransportation, bestEffort.encode()}},
        VariableLengthData{}));
    if (!immediate) {
      REQUIRE(subjectReports.interactionTransportationTypeChangeReports.empty());
    }
    pump(*subject, callbackModel);
    REQUIRE(subjectReports.interactionTransportationTypeChangeReports.size() == 1U);
    REQUIRE(subjectReports.interactionTransportationTypeChangeReports.front().interactionClass ==
            takeOrder);
    REQUIRE(subjectReports.interactionTransportationTypeChangeReports.front().transportationType ==
            bestEffort);
    REQUIRE_THROWS_AS(
        subject->sendInteraction(
            requestInteractionClass,
            ParameterHandleValueMap{{requestInteractionTarget, takeOrder.encode()}},
            VariableLengthData{}),
        rti1516_2025::InteractionParameterNotDefined);
    REQUIRE(subjectReports.interactionTransportationTypeChangeReports.size() == 1U);
    std::vector<rti1516_2025::Octet> malformedInteractionClassBytes{0x00U};
    VariableLengthData malformedInteractionClass(
        malformedInteractionClassBytes.data(),
        malformedInteractionClassBytes.size());
    REQUIRE_THROWS_AS(
        subject->sendInteraction(
            requestInteractionClass,
            ParameterHandleValueMap{
                {requestInteractionTarget, malformedInteractionClass},
                {requestInteractionTransportation, bestEffort.encode()}},
            VariableLengthData{}),
        rti1516_2025::RTIinternalError);
    REQUIRE_THROWS_AS(
        subject->sendInteraction(
            requestInteractionClass,
            ParameterHandleValueMap{
                {requestInteractionTarget, takeOrder.encode()},
                {requestInteractionTransportation, unknownTransportation}},
            VariableLengthData{}),
        rti1516_2025::InvalidTransportationTypeHandle);
    REQUIRE(subjectReports.interactionTransportationTypeChangeReports.size() == 1U);

    // The observer's independent publisher remains reliable after the
    // subject's best-effort request, while subject sends use best effort.
    REQUIRE_NOTHROW(observer->sendInteraction(
        observerTakeOrder,
        ParameterHandleValueMap{},
        VariableLengthData{}));
    pump(*subject, callbackModel);
    REQUIRE(subjectReports.interactionReports.size() == 1U);
    REQUIRE(subjectReports.interactionReports.back().interactionClass == takeOrder);
    REQUIRE(subjectReports.interactionReports.back().transportationType == reliable);
    subjectReports.interactionReports.clear();
    REQUIRE_NOTHROW(subject->sendInteraction(
        takeOrder,
        ParameterHandleValueMap{},
        VariableLengthData{}));
    pump(*observer, callbackModel);
    REQUIRE(observerReports.interactionReports.size() == 1U);
    REQUIRE(observerReports.interactionReports.back().transportationType == bestEffort);

    // Finally prove the accepted MIM request remains an ordinary public send
    // and is reportable with the RTI-originated producer policy.
    auto const reportClass = observer->getInteractionClassHandle(
        standard_hla::mom::report_service_invocation);
    auto const reportService = observer->getParameterHandle(
        reportClass,
        standard_hla::mom::service);
    REQUIRE(reportClass.isValid());
    REQUIRE(reportService.isValid());
    REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));
    REQUIRE_NOTHROW(subject->setServiceReportingSwitch(true));
    observerReports.interactionReports.clear();
    REQUIRE_NOTHROW(subject->sendInteraction(
        requestInteractionClass,
        ParameterHandleValueMap{
            {requestInteractionTarget, takeOrder.encode()},
            {requestInteractionTransportation, reliable.encode()}},
        VariableLengthData{}));
    pump(*observer, callbackModel);
    REQUIRE(observerReports.interactionReports.size() == 1U);
    auto const& serviceReport = observerReports.interactionReports.front();
    REQUIRE(serviceReport.interactionClass == reportClass);
    REQUIRE(serviceReport.transportationType == reliable);
    REQUIRE_FALSE(serviceReport.producingFederate.isValid());
    REQUIRE_FALSE(serviceReport.sentRegionsSupplied);
    rti1516_2025::HLAunicodeString decodedService;
    REQUIRE_NOTHROW(decodedService.decode(serviceReport.parameterValues.at(reportService)));
    REQUIRE(decodedService.get() == L"SendInteraction");

    REQUIRE_NOTHROW(observer->resignFederationExecution(rti1516_2025::NO_ACTION));
    REQUIRE_NOTHROW(subject->resignFederationExecution(
        rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(subject->disconnect());
    REQUIRE_NOTHROW(observer->disconnect());
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(rti1516_2025::HLA_IMMEDIATE);
  }
}
