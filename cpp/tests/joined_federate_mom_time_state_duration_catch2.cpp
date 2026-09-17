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
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The joined-federate MOM time-state duration tests require the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::HLA_IMMEDIATE;
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
  return L"federation-joined-mom-time-state-duration-" +
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
    "Embedded joined-federate MOM exposes time-state durations directly and periodically",
    "[integration][development-profile][federation-management][mom]"
    "[periodic-mom][time-management][callback-model][time-state-duration]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.request-attribute-value-update][rti.service.send-interaction]"
    "[rti.service.evoke-callback]"
    "[federate.callback.reflect-attribute-values]") {
  auto runScenario = [](auto const callbackModel) {
    ReportingFederateAmbassador targetReports;
    ReportingFederateAmbassador regulatorReports;
    ReportingFederateAmbassador observerReports;
    auto target = makeRti();
    auto regulator = makeRti();
    auto observer = makeRti();
    auto const federationName = nextFederationName();
    auto const fomModule =
        resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

    REQUIRE_NOTHROW(target->connect(targetReports, callbackModel));
    REQUIRE_NOTHROW(regulator->connect(regulatorReports, callbackModel));
    REQUIRE_NOTHROW(observer->connect(observerReports, callbackModel));
    REQUIRE_NOTHROW(target->createFederationExecution(
        federationName,
        fomModule,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(observer->joinFederationExecution(
        L"mom-time-state-duration-observer",
        L"observer",
        federationName));

    auto const momClass = observer->getObjectClassHandle(
        standard_hla::mom::federate_object_class);
    auto const federateHandleAttribute = observer->getAttributeHandle(
        momClass,
        standard_hla::mom::federate_handle);
    auto const grantedDurationAttribute = observer->getAttributeHandle(
        momClass,
        standard_hla::mom::time_granted_time);
    auto const advancingDurationAttribute = observer->getAttributeHandle(
        momClass,
        standard_hla::mom::time_advancing_time);
    auto const reliable = observer->getTransportationTypeHandle(
        standard_hla::mom::reliable);
    REQUIRE(momClass.isValid());
    REQUIRE(federateHandleAttribute.isValid());
    REQUIRE(grantedDurationAttribute.isValid());
    REQUIRE(advancingDurationAttribute.isValid());
    REQUIRE(reliable.isValid());
    REQUIRE_NOTHROW(observer->subscribeObjectClassAttributes(
        momClass,
        AttributeHandleSet{
            federateHandleAttribute,
            grantedDurationAttribute,
            advancingDurationAttribute},
        true));

    auto const targetFederate = target->joinFederationExecution(
        L"mom-time-state-duration-target",
        L"target",
        federationName);
    REQUIRE_NOTHROW(regulator->joinFederationExecution(
        L"mom-time-state-duration-regulator",
        L"regulator",
        federationName));
    drainCallbacks(*observer);

    auto const reflectedTarget = std::find_if(
        observerReports.attributeReflectionReports.begin(),
        observerReports.attributeReflectionReports.end(),
        [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
          auto const value = report.attributeValues.find(federateHandleAttribute);
          return value != report.attributeValues.end() &&
              variableLengthDataBytes(value->second) ==
                  variableLengthDataBytes(targetFederate.encode());
        });
    REQUIRE(reflectedTarget != observerReports.attributeReflectionReports.end());
    auto const targetObjectInstance = reflectedTarget->objectInstance;

    using DurationPair = std::pair<std::int32_t, std::int32_t>;
    auto requestDurations = [&] {
      auto const before = observerReports.attributeReflectionReports.size();
      REQUIRE_NOTHROW(observer->requestAttributeValueUpdate(
          targetObjectInstance,
          AttributeHandleSet{
              grantedDurationAttribute,
              advancingDurationAttribute},
          VariableLengthData{}));
      drainCallbacks(*observer);
      auto const reflectionIterator = std::find_if(
          observerReports.attributeReflectionReports.begin() +
              static_cast<std::ptrdiff_t>(before),
          observerReports.attributeReflectionReports.end(),
          [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
            return report.objectInstance == targetObjectInstance &&
                report.attributeValues.contains(grantedDurationAttribute) &&
                report.attributeValues.contains(advancingDurationAttribute);
          });
      REQUIRE(reflectionIterator != observerReports.attributeReflectionReports.end());
      auto const& reflection = *reflectionIterator;
      REQUIRE(reflection.attributeValues.size() == 2U);
      REQUIRE(reflection.transportationType == reliable);
      REQUIRE_FALSE(reflection.producingFederate.isValid());
      REQUIRE_FALSE(reflection.sentRegionsSupplied);
      REQUIRE(reflection.userSuppliedTag.size() == 0U);
      rti1516_2025::HLAinteger32BE granted;
      rti1516_2025::HLAinteger32BE advancing;
      REQUIRE_NOTHROW(granted.decode(
          reflection.attributeValues.at(grantedDurationAttribute)));
      REQUIRE_NOTHROW(advancing.decode(
          reflection.attributeValues.at(advancingDurationAttribute)));
      return DurationPair{granted.get(), advancing.get()};
    };

    auto const initial = requestDurations();
    REQUIRE(initial.first >= 0);
    REQUIRE(initial.second >= 0);

    REQUIRE_NOTHROW(target->enableTimeConstrained());
    drainCallbacks(*target);
    REQUIRE_NOTHROW(regulator->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    drainCallbacks(*regulator);
    REQUIRE_NOTHROW(target->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(2)));
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    auto const whileAdvancing = requestDurations();
    REQUIRE(whileAdvancing.first >= initial.first);
    REQUIRE(whileAdvancing.second >= initial.second);
    REQUIRE(whileAdvancing.second > 0);

    REQUIRE_NOTHROW(regulator->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(2)));
    drainCallbacks(*target);
    drainCallbacks(*regulator);
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    auto const afterGrant = requestDurations();
    REQUIRE(afterGrant.first >= initial.first);
    REQUIRE(afterGrant.first > 0);
    REQUIRE(afterGrant.second >= whileAdvancing.second);

    auto const setTiming = observer->getInteractionClassHandle(
        standard_hla::mom::set_timing);
    auto const federateParameter = observer->getParameterHandle(
        setTiming,
        standard_hla::mom::federate);
    auto const periodParameter = observer->getParameterHandle(
        setTiming,
        standard_hla::mom::report_period);
    REQUIRE(setTiming.isValid());
    REQUIRE(federateParameter.isValid());
    REQUIRE(periodParameter.isValid());
    REQUIRE_NOTHROW(observer->sendInteraction(
        setTiming,
        ParameterHandleValueMap{
            {federateParameter, targetFederate.encode()},
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
          return report.objectInstance == targetObjectInstance &&
              report.attributeValues.contains(grantedDurationAttribute) &&
              report.attributeValues.contains(advancingDurationAttribute);
        });
    REQUIRE(periodicReflection != observerReports.attributeReflectionReports.end());
    REQUIRE(periodicReflection->attributeValues.size() == 2U);
    REQUIRE(periodicReflection->transportationType == reliable);
    REQUIRE_FALSE(periodicReflection->producingFederate.isValid());
    REQUIRE_FALSE(periodicReflection->sentRegionsSupplied);
    REQUIRE(periodicReflection->userSuppliedTag.size() == 0U);
    rti1516_2025::HLAinteger32BE periodicGranted;
    rti1516_2025::HLAinteger32BE periodicAdvancing;
    REQUIRE_NOTHROW(periodicGranted.decode(
        periodicReflection->attributeValues.at(grantedDurationAttribute)));
    REQUIRE_NOTHROW(periodicAdvancing.decode(
        periodicReflection->attributeValues.at(advancingDurationAttribute)));
    REQUIRE(periodicGranted.get() >= afterGrant.first);
    REQUIRE(periodicAdvancing.get() >= afterGrant.second);

    // Direct AVU is non-consuming; the periodic registry path is the single
    // consumer of the accumulated interval.  A direct read immediately after
    // the periodic reflection therefore observes a fresh, smaller interval.
    auto const afterPeriodic = requestDurations();
    REQUIRE(afterPeriodic.first < periodicGranted.get());
    REQUIRE(afterPeriodic.second <= periodicAdvancing.get());
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    auto const secondAfterPeriodic = requestDurations();
    REQUIRE(secondAfterPeriodic.first >= afterPeriodic.first);
    REQUIRE(secondAfterPeriodic.second >= afterPeriodic.second);

    REQUIRE_NOTHROW(observer->sendInteraction(
        setTiming,
        ParameterHandleValueMap{
            {federateParameter, targetFederate.encode()},
            {periodParameter, rti1516_2025::HLAinteger32BE{0}.encode()}},
        VariableLengthData{}));
    REQUIRE_NOTHROW(target->resignFederationExecution(rti1516_2025::NO_ACTION));
    REQUIRE_NOTHROW(regulator->resignFederationExecution(rti1516_2025::NO_ACTION));
    REQUIRE_NOTHROW(observer->resignFederationExecution(rti1516_2025::NO_ACTION));
    REQUIRE_NOTHROW(target->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(target->disconnect());
    REQUIRE_NOTHROW(regulator->disconnect());
    REQUIRE_NOTHROW(observer->disconnect());
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(HLA_IMMEDIATE);
  }
}
