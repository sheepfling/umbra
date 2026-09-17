#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

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
#error "The joined-federate MOM GALT/LITS tests require the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::LogicalTime;
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
  return L"federation-joined-mom-galt-lits-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / relativePath;
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct AttributeReflectionReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleValueMap attributeValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    bool sentRegionsSupplied = false;
    RegionHandleSet sentRegions;
    OrderType sentOrderType = RECEIVE;
    OrderType receivedOrderType = RECEIVE;
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
    "Embedded joined-federate MOM exposes federation GALT and LITS",
    "[integration][development-profile][federation-management][mom]"
    "[periodic-mom][time-management][galt][lits][galt-lits-periodic]"
    "[rti.service.query-galt][rti.service.query-lits]"
    "[rti.service.request-attribute-value-update][rti.service.send-interaction]"
    "[federate.callback.reflect-attribute-values]") {
  ReportingFederateAmbassador subjectReports;
  ReportingFederateAmbassador regulatorReports;
  ReportingFederateAmbassador observerReports;
  auto subject = makeRti();
  auto regulator = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(subject->connect(subjectReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regulator->connect(regulatorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(subject->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"mom-galt-lits-observer", L"observer", federationName));

  auto const momClass = observer->getObjectClassHandle(
      standard_hla::mom::federate_object_class);
  auto const federateHandleAttribute = observer->getAttributeHandle(
      momClass, standard_hla::mom::federate_handle);
  auto const galtAttribute = observer->getAttributeHandle(
      momClass, standard_hla::mom::galt);
  auto const litsAttribute = observer->getAttributeHandle(
      momClass, standard_hla::mom::lits);
  auto const reliable = observer->getTransportationTypeHandle(
      standard_hla::mom::reliable);
  REQUIRE(momClass.isValid());
  REQUIRE(federateHandleAttribute.isValid());
  REQUIRE(galtAttribute.isValid());
  REQUIRE(litsAttribute.isValid());
  REQUIRE(reliable.isValid());
  AttributeHandleSet const requestedAttributes{
      galtAttribute,
      litsAttribute};
  REQUIRE_NOTHROW(observer->subscribeObjectClassAttributes(
      momClass,
      AttributeHandleSet{
          federateHandleAttribute,
          galtAttribute,
          litsAttribute},
      true));
  drainCallbacks(*observer);

  REQUIRE_NOTHROW(regulator->joinFederationExecution(
      L"mom-galt-lits-regulator", L"regulator", federationName));
  REQUIRE_NOTHROW(regulator->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(2)));
  REQUIRE_FALSE(regulator->evokeCallback(0.0));
  auto const subjectFederate = subject->joinFederationExecution(
      L"mom-galt-lits-subject", L"subject", federationName);
  drainCallbacks(*observer);

  auto const reflectedSubject = std::find_if(
      observerReports.attributeReflectionReports.begin(),
      observerReports.attributeReflectionReports.end(),
      [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
        auto const value = report.attributeValues.find(federateHandleAttribute);
        return value != report.attributeValues.end() &&
            variableLengthDataBytes(value->second) ==
                variableLengthDataBytes(subjectFederate.encode());
      });
  REQUIRE(reflectedSubject != observerReports.attributeReflectionReports.end());
  auto const subjectObjectInstance = reflectedSubject->objectInstance;

  auto decodeBounds = [&](AttributeHandleValueMap const& values) {
    REQUIRE(values.size() == 2U);
    rti1516_2025::HLAinteger64Time galt;
    rti1516_2025::HLAinteger64Time lits;
    REQUIRE_NOTHROW(galt.decode(values.at(galtAttribute)));
    REQUIRE_NOTHROW(lits.decode(values.at(litsAttribute)));
    REQUIRE(galt.getTime() == 2);
    REQUIRE(lits.getTime() == 2);
  };

  auto requestBounds = [&] {
    auto const before = observerReports.attributeReflectionReports.size();
    REQUIRE_NOTHROW(observer->requestAttributeValueUpdate(
        subjectObjectInstance,
        requestedAttributes,
        VariableLengthData{}));
    drainCallbacks(*observer);
    auto const reflectionIterator = std::find_if(
        observerReports.attributeReflectionReports.begin() +
            static_cast<std::ptrdiff_t>(before),
        observerReports.attributeReflectionReports.end(),
        [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
          return report.objectInstance == subjectObjectInstance &&
              report.attributeValues.contains(galtAttribute) &&
              report.attributeValues.contains(litsAttribute);
        });
    REQUIRE(reflectionIterator != observerReports.attributeReflectionReports.end());
    auto const& reflection = *reflectionIterator;
    REQUIRE(reflection.transportationType == reliable);
    REQUIRE_FALSE(reflection.producingFederate.isValid());
    REQUIRE_FALSE(reflection.sentRegionsSupplied);
    REQUIRE(reflection.userSuppliedTag.size() == 0U);
    decodeBounds(reflection.attributeValues);
  };

  rti1516_2025::HLAinteger64Time queriedGalt;
  rti1516_2025::HLAinteger64Time queriedLits;
  REQUIRE(observer->queryGALT(queriedGalt));
  REQUIRE(observer->queryLITS(queriedLits));
  REQUIRE(queriedGalt.getTime() == 2);
  REQUIRE(queriedLits.getTime() == 2);
  requestBounds();

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
      ParameterHandleValueMap{
          {federateParameter, subjectFederate.encode()},
          {periodParameter, rti1516_2025::HLAinteger32BE{1}.encode()}},
      VariableLengthData{}));
  auto const beforePeriodic = observerReports.attributeReflectionReports.size();
  std::this_thread::sleep_for(std::chrono::milliseconds(1100));
  drainCallbacks(*observer);
  auto const periodicReflection = std::find_if(
      observerReports.attributeReflectionReports.begin() +
          static_cast<std::ptrdiff_t>(beforePeriodic),
      observerReports.attributeReflectionReports.end(),
      [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
        return report.objectInstance == subjectObjectInstance &&
            report.attributeValues.contains(galtAttribute) &&
            report.attributeValues.contains(litsAttribute);
      });
  REQUIRE(periodicReflection != observerReports.attributeReflectionReports.end());
  REQUIRE(periodicReflection->transportationType == reliable);
  REQUIRE_FALSE(periodicReflection->producingFederate.isValid());
  REQUIRE_FALSE(periodicReflection->sentRegionsSupplied);
  decodeBounds(periodicReflection->attributeValues);

  REQUIRE_NOTHROW(observer->sendInteraction(
      setTiming,
      ParameterHandleValueMap{
          {federateParameter, subjectFederate.encode()},
          {periodParameter, rti1516_2025::HLAinteger32BE{0}.encode()}},
      VariableLengthData{}));
  REQUIRE_NOTHROW(regulator->disableTimeRegulation());
  drainCallbacks(*regulator);
  REQUIRE_FALSE(observer->queryGALT(queriedGalt));
  REQUIRE_FALSE(observer->queryLITS(queriedLits));
  auto const beforeUndefined = observerReports.attributeReflectionReports.size();
  REQUIRE_NOTHROW(observer->requestAttributeValueUpdate(
      subjectObjectInstance,
      requestedAttributes,
      VariableLengthData{}));
  drainCallbacks(*observer);
  auto const undefinedReflection = std::find_if(
      observerReports.attributeReflectionReports.begin() +
          static_cast<std::ptrdiff_t>(beforeUndefined),
      observerReports.attributeReflectionReports.end(),
      [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
        return report.objectInstance == subjectObjectInstance &&
            report.attributeValues.contains(galtAttribute) &&
            report.attributeValues.contains(litsAttribute);
      });
  REQUIRE(undefinedReflection != observerReports.attributeReflectionReports.end());
  REQUIRE(undefinedReflection->attributeValues.at(galtAttribute).size() == 0U);
  REQUIRE(undefinedReflection->attributeValues.at(litsAttribute).size() == 0U);

  REQUIRE_NOTHROW(subject->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(observer->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(regulator->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subject->disconnect());
  REQUIRE_NOTHROW(regulator->disconnect());
  REQUIRE_NOTHROW(observer->disconnect());
}
