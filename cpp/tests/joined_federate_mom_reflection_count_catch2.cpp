#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The joined-federate MOM reflection-count tests require the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
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
using rti1516_2025::TIMESTAMP;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

std::vector<unsigned char> variableLengthDataBytes(
    VariableLengthData const& value) {
  auto const* bytes = static_cast<unsigned char const*>(value.data());
  if (bytes == nullptr) {
    return {};
  }
  return {bytes, bytes + value.size()};
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"federation-joined-mom-reflection-count-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / relativePath;
}

class ReportingFederateAmbassador final : public NullFederateAmbassador {
 public:
  struct AttributeReflectionReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleValueMap attributeValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    bool sentRegionsSupplied = false;
    RegionHandleSet sentRegions;
  };

  struct ObjectDiscoveryReport final {
    ObjectInstanceHandle objectInstance;
    ObjectClassHandle objectClass;
    std::wstring objectInstanceName;
    FederateHandle producingFederate;
  };

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

  std::vector<AttributeReflectionReport> attributeReflectionReports;
  std::vector<ObjectDiscoveryReport> objectDiscoveryReports;
};

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

}  // namespace

TEST_CASE(
    "Embedded joined-federate MOM separates reflection totals from distinct objects",
    "[integration][development-profile][federation-management][mom]"
    "[periodic-mom][object-management][time-management][callback-model]"
    "[reflection-counts][timestamped-attribute-update]"
    "[joined-federate-mom-reflection-counts]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.request-attribute-value-update]"
    "[rti.service.update-attribute-values][rti.service.register-object-instance]"
    "[rti.service.change-attribute-order-type]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.time-advance-request][rti.service.send-interaction]"
    "[rti.service.evoke-callback]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("attribute-update-passel-fom.xml").wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  auto const receiverFederate = receiver->joinFederationExecution(
      L"mom-reflection-count-receiver", L"receiver", federationName);

  auto const momClass = receiver->getObjectClassHandle(
      standard_hla::mom::federate_object_class);
  auto const federateHandleAttribute = receiver->getAttributeHandle(
      momClass, standard_hla::mom::federate_handle);
  auto const reflectedObjectsAttribute = receiver->getAttributeHandle(
      momClass, standard_hla::mom::object_instances_reflected);
  auto const reflectionsReceivedAttribute = receiver->getAttributeHandle(
      momClass, standard_hla::mom::reflections_received);
  auto const reliable = receiver->getTransportationTypeHandle(
      standard_hla::mom::reliable);
  REQUIRE(momClass.isValid());
  REQUIRE(federateHandleAttribute.isValid());
  REQUIRE(reflectedObjectsAttribute.isValid());
  REQUIRE(reflectionsReceivedAttribute.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(
      momClass,
      AttributeHandleSet{
          federateHandleAttribute,
          reflectedObjectsAttribute,
          reflectionsReceivedAttribute},
      true));

  auto const receiverFederateClass = receiver->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const receiverReliableAttribute = receiver->getAttributeHandle(
      receiverFederateClass, L"ReliableBaseA");
  REQUIRE(receiverFederateClass.isValid());
  REQUIRE(receiverReliableAttribute.isValid());
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(
      receiverFederateClass,
      AttributeHandleSet{receiverReliableAttribute}));

  auto const receiverMomObject = [&] {
    drainCallbacks(*receiver);
    auto const reflected = std::find_if(
        receiverReports.attributeReflectionReports.begin(),
        receiverReports.attributeReflectionReports.end(),
        [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
          auto const value = report.attributeValues.find(federateHandleAttribute);
          return value != report.attributeValues.end() &&
              variableLengthDataBytes(value->second) ==
                  variableLengthDataBytes(receiverFederate.encode());
        });
    REQUIRE(reflected != receiverReports.attributeReflectionReports.end());
    return reflected->objectInstance;
  }();

  auto requestCounts = [&] {
    auto const before = receiverReports.attributeReflectionReports.size();
    REQUIRE_NOTHROW(receiver->requestAttributeValueUpdate(
        receiverMomObject,
        AttributeHandleSet{
            reflectedObjectsAttribute,
            reflectionsReceivedAttribute},
        VariableLengthData{}));
    drainCallbacks(*receiver);
    auto const reflectionIterator = std::find_if(
        receiverReports.attributeReflectionReports.begin() +
            static_cast<std::ptrdiff_t>(before),
        receiverReports.attributeReflectionReports.end(),
        [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
          return report.objectInstance == receiverMomObject &&
              report.attributeValues.contains(reflectedObjectsAttribute) &&
              report.attributeValues.contains(reflectionsReceivedAttribute);
        });
    REQUIRE(reflectionIterator != receiverReports.attributeReflectionReports.end());
    auto const& reflection = *reflectionIterator;
    REQUIRE(reflection.attributeValues.size() == 2U);
    REQUIRE(reflection.transportationType == reliable);
    REQUIRE_FALSE(reflection.producingFederate.isValid());
    REQUIRE_FALSE(reflection.sentRegionsSupplied);
    REQUIRE(reflection.userSuppliedTag.size() == 0U);
    rti1516_2025::HLAinteger32BE reflectedObjects;
    rti1516_2025::HLAinteger32BE reflectionsReceived;
    REQUIRE_NOTHROW(reflectedObjects.decode(
        reflection.attributeValues.at(reflectedObjectsAttribute)));
    REQUIRE_NOTHROW(reflectionsReceived.decode(
        reflection.attributeValues.at(reflectionsReceivedAttribute)));
    return std::pair<std::int32_t, std::int32_t>{
        reflectedObjects.get(), reflectionsReceived.get()};
  };

  auto const initialCounts = requestCounts();
  REQUIRE(initialCounts.first == 0);
  REQUIRE(initialCounts.second == 0);

  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"mom-reflection-count-publisher", L"publisher", federationName));
  auto const publisherClass = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const publisherReliableAttribute = publisher->getAttributeHandle(
      publisherClass, L"ReliableBaseA");
  REQUIRE(publisherClass.isValid());
  REQUIRE(publisherReliableAttribute.isValid());
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      publisherClass,
      AttributeHandleSet{publisherReliableAttribute}));
  auto const objectOne = publisher->registerObjectInstance(publisherClass);
  auto const objectTwo = publisher->registerObjectInstance(publisherClass);
  drainCallbacks(*receiver);
  auto const discoveredOne = std::find_if(
      receiverReports.objectDiscoveryReports.begin(),
      receiverReports.objectDiscoveryReports.end(),
      [&](ReportingFederateAmbassador::ObjectDiscoveryReport const& report) {
        return report.objectInstance == objectOne;
      });
  auto const discoveredTwo = std::find_if(
      receiverReports.objectDiscoveryReports.begin(),
      receiverReports.objectDiscoveryReports.end(),
      [&](ReportingFederateAmbassador::ObjectDiscoveryReport const& report) {
        return report.objectInstance == objectTwo;
      });
  REQUIRE(discoveredOne != receiverReports.objectDiscoveryReports.end());
  REQUIRE(discoveredTwo != receiverReports.objectDiscoveryReports.end());

  auto sendReceiveOrderUpdate = [&](ObjectInstanceHandle const& object,
                                    wchar_t const* value) {
    REQUIRE_NOTHROW(publisher->updateAttributeValues(
        object,
        AttributeHandleValueMap{
            {publisherReliableAttribute,
             rti1516_2025::HLAunicodeString(value).encode()}},
        VariableLengthData{}));
    drainCallbacks(*receiver);
  };

  sendReceiveOrderUpdate(objectOne, L"one-first");
  auto const firstCounts = requestCounts();
  REQUIRE(firstCounts.first == 1);
  REQUIRE(firstCounts.second == 1);

  sendReceiveOrderUpdate(objectOne, L"one-second");
  auto const secondCounts = requestCounts();
  REQUIRE(secondCounts.first == 1);
  REQUIRE(secondCounts.second == 2);

  sendReceiveOrderUpdate(objectTwo, L"two-first");
  auto const thirdCounts = requestCounts();
  REQUIRE(thirdCounts.first == 2);
  REQUIRE(thirdCounts.second == 3);

    REQUIRE_NOTHROW(publisher->changeAttributeOrderType(
      objectTwo,
      AttributeHandleSet{publisherReliableAttribute},
      TIMESTAMP));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drainCallbacks(*receiver);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*publisher);
  auto const retraction = publisher->updateAttributeValues(
      objectTwo,
      AttributeHandleValueMap{
          {publisherReliableAttribute,
           rti1516_2025::HLAunicodeString(L"two-timestamped").encode()}},
      VariableLengthData{},
      rti1516_2025::HLAinteger64Time(2));
  REQUIRE(retraction.isValid());

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  drainCallbacks(*receiver);
  drainCallbacks(*publisher);
  auto const afterTimestampedCounts = requestCounts();
  REQUIRE(afterTimestampedCounts.first == 2);
  REQUIRE(afterTimestampedCounts.second == 4);

  auto const setTiming = receiver->getInteractionClassHandle(
      standard_hla::mom::set_timing);
  auto const federateParameter = receiver->getParameterHandle(
      setTiming, standard_hla::mom::federate);
  auto const periodParameter = receiver->getParameterHandle(
      setTiming, standard_hla::mom::report_period);
  REQUIRE(setTiming.isValid());
  REQUIRE(federateParameter.isValid());
  REQUIRE(periodParameter.isValid());
  REQUIRE_NOTHROW(receiver->sendInteraction(
      setTiming,
      ParameterHandleValueMap{
          {federateParameter, receiverFederate.encode()},
          {periodParameter, rti1516_2025::HLAinteger32BE{1}.encode()}},
      VariableLengthData{}));
  auto const beforePeriodic = receiverReports.attributeReflectionReports.size();
  std::this_thread::sleep_for(std::chrono::milliseconds(1100));
  drainCallbacks(*receiver);
  auto const periodicReflection = std::find_if(
      receiverReports.attributeReflectionReports.begin() +
          static_cast<std::ptrdiff_t>(beforePeriodic),
      receiverReports.attributeReflectionReports.end(),
      [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
        return report.objectInstance == receiverMomObject &&
            report.attributeValues.contains(reflectedObjectsAttribute) &&
            report.attributeValues.contains(reflectionsReceivedAttribute);
      });
  REQUIRE(periodicReflection != receiverReports.attributeReflectionReports.end());
  REQUIRE(periodicReflection->attributeValues.size() == 2U);
  REQUIRE(periodicReflection->transportationType == reliable);
  REQUIRE_FALSE(periodicReflection->producingFederate.isValid());
  REQUIRE_FALSE(periodicReflection->sentRegionsSupplied);
  REQUIRE(periodicReflection->userSuppliedTag.size() == 0U);
  rti1516_2025::HLAinteger32BE periodicReflectedObjects;
  rti1516_2025::HLAinteger32BE periodicReflectionsReceived;
  REQUIRE_NOTHROW(periodicReflectedObjects.decode(
      periodicReflection->attributeValues.at(reflectedObjectsAttribute)));
  REQUIRE_NOTHROW(periodicReflectionsReceived.decode(
      periodicReflection->attributeValues.at(reflectionsReceivedAttribute)));
  REQUIRE(periodicReflectedObjects.get() == 2);
  REQUIRE(periodicReflectionsReceived.get() == 4);

  REQUIRE_NOTHROW(receiver->sendInteraction(
      setTiming,
      ParameterHandleValueMap{
          {federateParameter, receiverFederate.encode()},
          {periodParameter, rti1516_2025::HLAinteger32BE{0}.encode()}},
      VariableLengthData{}));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
