#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/BasicDataElements.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The joined-federate MOM removed-object-count periodic tests require the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::HLAinteger32BE;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
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
  return L"federation-joined-mom-removed-object-count-periodic-" +
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
  };

  struct ObjectDiscoveryReport final {
    ObjectInstanceHandle objectInstance;
    ObjectClassHandle objectClass;
    std::wstring objectInstanceName;
    FederateHandle producingFederate;
  };

  struct ObjectRemovalReport final {
    ObjectInstanceHandle objectInstance;
    VariableLengthData userSuppliedTag;
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
      rti1516_2025::RegionHandleSet const* optionalSentRegions) override {
    static_cast<void>(optionalSentRegions);
    attributeReflectionReports.push_back({
        objectInstance,
        attributeValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
    });
  }

  void removeObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      VariableLengthData const& userSuppliedTag,
      FederateHandle const& producingFederate) override {
    objectRemovalReports.push_back({
        objectInstance,
        userSuppliedTag,
        producingFederate,
    });
  }

  std::vector<AttributeReflectionReport> attributeReflectionReports;
  std::vector<ObjectDiscoveryReport> objectDiscoveryReports;
  std::vector<ObjectRemovalReport> objectRemovalReports;
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
    "Embedded joined-federate MOM HLAobjectInstancesRemoved counts committed callbacks",
    "[integration][development-profile][federation-management][mom]"
    "[periodic-mom][object-management][callback-model]"
    "[request-attribute-value-update][register-object-instance]"
    "[delete-object-instance][remove-object-instance][send-interaction]"
    "[reflect-attribute-values][joined-federate-mom-removed-object-count-periodic]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.request-attribute-value-update]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.register-object-instance][rti.service.delete-object-instance]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.send-interaction][rti.service.evoke-callback]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.remove-object-instance]"
    "[federate.callback.reflect-attribute-values]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador observerReports;
  auto owner = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("attribute-update-passel-fom.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  auto const observerFederate = observer->joinFederationExecution(
      L"mom-removed-object-count-periodic-observer", L"observer", federationName);

  auto const momClass = observer->getObjectClassHandle(
      standard_hla::mom::federate_object_class);
  auto const federateHandleAttribute = observer->getAttributeHandle(
      momClass, standard_hla::mom::federate_handle);
  auto const removedInstancesAttribute = observer->getAttributeHandle(
      momClass, standard_hla::mom::object_instances_removed);
  auto const reliable = observer->getTransportationTypeHandle(
      standard_hla::mom::reliable);
  REQUIRE(momClass.isValid());
  REQUIRE(federateHandleAttribute.isValid());
  REQUIRE(removedInstancesAttribute.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE_NOTHROW(observer->subscribeObjectClassAttributes(
      momClass,
      AttributeHandleSet{federateHandleAttribute, removedInstancesAttribute},
      true));

  auto const objectClass = observer->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableAttribute = observer->getAttributeHandle(
      objectClass, L"ReliableBaseA");
  REQUIRE(objectClass.isValid());
  REQUIRE(reliableAttribute.isValid());
  REQUIRE_NOTHROW(observer->subscribeObjectClassAttributes(
      objectClass, AttributeHandleSet{reliableAttribute}, true));

  auto const ownerFederate = owner->joinFederationExecution(
      L"mom-removed-object-count-periodic-owner", L"owner", federationName);
  auto const ownerObjectClass = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const ownerReliableAttribute = owner->getAttributeHandle(
      ownerObjectClass, L"ReliableBaseA");
  REQUIRE(ownerObjectClass.isValid());
  REQUIRE(ownerReliableAttribute.isValid());
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
      ownerObjectClass, AttributeHandleSet{ownerReliableAttribute}));
  drainCallbacks(*observer);

  auto const observerMomObjectIterator = std::find_if(
      observerReports.attributeReflectionReports.begin(),
      observerReports.attributeReflectionReports.end(),
      [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
        auto const value = report.attributeValues.find(federateHandleAttribute);
        return value != report.attributeValues.end() &&
            variableLengthDataBytes(value->second) ==
                variableLengthDataBytes(observerFederate.encode());
      });
  REQUIRE(observerMomObjectIterator != observerReports.attributeReflectionReports.end());
  auto const observerMomObject = observerMomObjectIterator->objectInstance;

  auto requestRemovedCount = [&] {
    auto const before = observerReports.attributeReflectionReports.size();
    REQUIRE_NOTHROW(observer->requestAttributeValueUpdate(
        observerMomObject,
        AttributeHandleSet{removedInstancesAttribute},
        VariableLengthData{}));
    drainCallbacks(*observer);
    auto const reflectionIterator = std::find_if(
        observerReports.attributeReflectionReports.begin() +
            static_cast<std::ptrdiff_t>(before),
        observerReports.attributeReflectionReports.end(),
        [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
          return report.objectInstance == observerMomObject &&
              report.attributeValues.contains(removedInstancesAttribute);
        });
    REQUIRE(reflectionIterator != observerReports.attributeReflectionReports.end());
    auto const& reflection = *reflectionIterator;
    REQUIRE(reflection.attributeValues.size() == 1U);
    REQUIRE(reflection.transportationType == reliable);
    REQUIRE_FALSE(reflection.producingFederate.isValid());
    REQUIRE(reflection.userSuppliedTag.size() == 0U);
    HLAinteger32BE decoded;
    REQUIRE_NOTHROW(decoded.decode(
        reflection.attributeValues.at(removedInstancesAttribute)));
    return decoded.get();
  };

  REQUIRE(requestRemovedCount() == 0);

  auto const firstObject = owner->registerObjectInstance(ownerObjectClass);
  auto const secondObject = owner->registerObjectInstance(ownerObjectClass);
  REQUIRE(firstObject.isValid());
  REQUIRE(secondObject.isValid());
  drainCallbacks(*observer);
  auto const applicationDiscoveryCount = std::count_if(
      observerReports.objectDiscoveryReports.begin(),
      observerReports.objectDiscoveryReports.end(),
      [&](ReportingFederateAmbassador::ObjectDiscoveryReport const& report) {
        return report.objectClass == objectClass &&
            report.producingFederate == ownerFederate;
      });
  REQUIRE(applicationDiscoveryCount == 2U);
  REQUIRE(std::find_if(
              observerReports.objectDiscoveryReports.begin(),
              observerReports.objectDiscoveryReports.end(),
              [&](ReportingFederateAmbassador::ObjectDiscoveryReport const& report) {
                return report.objectInstance == firstObject;
              }) != observerReports.objectDiscoveryReports.end());
  REQUIRE(std::find_if(
              observerReports.objectDiscoveryReports.begin(),
              observerReports.objectDiscoveryReports.end(),
              [&](ReportingFederateAmbassador::ObjectDiscoveryReport const& report) {
                return report.objectInstance == secondObject;
              }) != observerReports.objectDiscoveryReports.end());

  auto const setTiming = observer->getInteractionClassHandle(
      standard_hla::mom::set_timing);
  auto const federateParameter = observer->getParameterHandle(
      setTiming, standard_hla::mom::federate);
  auto const periodParameter = observer->getParameterHandle(
      setTiming, standard_hla::mom::report_period);
  REQUIRE(setTiming.isValid());
  REQUIRE(federateParameter.isValid());
  REQUIRE(periodParameter.isValid());
  REQUIRE_NOTHROW(observer->sendInteraction(
      setTiming,
      rti1516_2025::ParameterHandleValueMap{
          {federateParameter, observerFederate.encode()},
          {periodParameter, HLAinteger32BE{1}.encode()}},
      VariableLengthData{}));
  auto const beforePeriodic = observerReports.attributeReflectionReports.size();
  std::this_thread::sleep_for(std::chrono::milliseconds(1100));
  drainCallbacks(*observer);
  auto const periodicBeforeRemoval = std::find_if(
      observerReports.attributeReflectionReports.begin() +
          static_cast<std::ptrdiff_t>(beforePeriodic),
      observerReports.attributeReflectionReports.end(),
      [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
        return report.objectInstance == observerMomObject &&
            report.attributeValues.contains(removedInstancesAttribute);
      });
  REQUIRE(periodicBeforeRemoval != observerReports.attributeReflectionReports.end());
  REQUIRE(periodicBeforeRemoval->attributeValues.size() == 1U);
  REQUIRE(periodicBeforeRemoval->transportationType == reliable);
  REQUIRE_FALSE(periodicBeforeRemoval->producingFederate.isValid());
  REQUIRE(periodicBeforeRemoval->userSuppliedTag.size() == 0U);
  HLAinteger32BE periodicZero;
  REQUIRE_NOTHROW(periodicZero.decode(
      periodicBeforeRemoval->attributeValues.at(removedInstancesAttribute)));
  REQUIRE(periodicZero.get() == 0);
  REQUIRE_NOTHROW(observer->sendInteraction(
      setTiming,
      rti1516_2025::ParameterHandleValueMap{
          {federateParameter, observerFederate.encode()},
          {periodParameter, HLAinteger32BE{0}.encode()}},
      VariableLengthData{}));

  REQUIRE_NOTHROW(owner->deleteObjectInstance(firstObject, VariableLengthData{}));
  drainCallbacks(*observer);
  REQUIRE(observerReports.objectRemovalReports.size() == 1U);
  REQUIRE(observerReports.objectRemovalReports.front().objectInstance == firstObject);
  REQUIRE(observerReports.objectRemovalReports.front().producingFederate == ownerFederate);
  REQUIRE(requestRemovedCount() == 1);

  REQUIRE_NOTHROW(owner->deleteObjectInstance(secondObject, VariableLengthData{}));
  drainCallbacks(*observer);
  REQUIRE(observerReports.objectRemovalReports.size() == 2U);
  REQUIRE(observerReports.objectRemovalReports.back().objectInstance == secondObject);
  REQUIRE(observerReports.objectRemovalReports.back().producingFederate == ownerFederate);
  REQUIRE(requestRemovedCount() == 2);

  REQUIRE_NOTHROW(observer->sendInteraction(
      setTiming,
      rti1516_2025::ParameterHandleValueMap{
          {federateParameter, observerFederate.encode()},
          {periodParameter, HLAinteger32BE{1}.encode()}},
      VariableLengthData{}));
  auto const beforePostRemovalPeriodic = observerReports.attributeReflectionReports.size();
  std::this_thread::sleep_for(std::chrono::milliseconds(1100));
  drainCallbacks(*observer);
  auto const periodicAfterRemoval = std::find_if(
      observerReports.attributeReflectionReports.begin() +
          static_cast<std::ptrdiff_t>(beforePostRemovalPeriodic),
      observerReports.attributeReflectionReports.end(),
      [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
        return report.objectInstance == observerMomObject &&
            report.attributeValues.contains(removedInstancesAttribute);
      });
  REQUIRE(periodicAfterRemoval != observerReports.attributeReflectionReports.end());
  REQUIRE(periodicAfterRemoval->attributeValues.size() == 1U);
  REQUIRE(periodicAfterRemoval->transportationType == reliable);
  REQUIRE_FALSE(periodicAfterRemoval->producingFederate.isValid());
  REQUIRE(periodicAfterRemoval->userSuppliedTag.size() == 0U);
  HLAinteger32BE periodicTwo;
  REQUIRE_NOTHROW(periodicTwo.decode(
      periodicAfterRemoval->attributeValues.at(removedInstancesAttribute)));
  REQUIRE(periodicTwo.get() == 2);

  REQUIRE_NOTHROW(observer->sendInteraction(
      setTiming,
      rti1516_2025::ParameterHandleValueMap{
          {federateParameter, observerFederate.encode()},
          {periodParameter, HLAinteger32BE{0}.encode()}},
      VariableLengthData{}));
  REQUIRE_NOTHROW(observer->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(observer->disconnect());
}
