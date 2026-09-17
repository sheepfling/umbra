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
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The federation MOM save-conditional tests require the Umbra source directory."
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
  return L"federation-mom-save-conditionals-" +
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
    std::wstring timeImplementationName;
    std::wstring timeValue;
    OrderType sentOrderType = RECEIVE;
    OrderType receivedOrderType = RECEIVE;
    bool retractionSupplied = false;
    bool retractionValid = false;
  };

  struct ObjectDiscoveryReport final {
    ObjectInstanceHandle objectInstance;
    ObjectClassHandle objectClass;
    std::wstring objectInstanceName;
    FederateHandle producingFederate;
  };

  struct TimeAdvanceGrantReport final {
    std::wstring implementationName;
    std::wstring value;
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

  void timeAdvanceGrant(LogicalTime const& time) override {
    timeAdvanceGrantReports.push_back({time.implementationName(), time.toString()});
    callbackOrder.push_back("grant");
  }

  void initiateFederateSave(std::wstring const& label) override {
    initiateFederateSaveReports.push_back(label);
    callbackOrder.push_back("save-initiate");
  }

  void initiateFederateSave(
      std::wstring const& label,
      LogicalTime const& time) override {
    initiateFederateSaveReports.push_back(label);
    timestampedSaveInitiationReports.push_back({
        label,
        time.implementationName(),
        time.toString(),
    });
    callbackOrder.push_back("save-initiate");
  }

  void federationSaved() override {
    ++federationSavedReportCount;
    callbackOrder.push_back("save-complete");
  }

  struct TimestampedSaveInitiationReport final {
    std::wstring label;
    std::wstring timeImplementationName;
    std::wstring timeValue;
  };

  std::vector<AttributeReflectionReport> attributeReflectionReports;
  std::vector<ObjectDiscoveryReport> objectDiscoveryReports;
  std::vector<TimeAdvanceGrantReport> timeAdvanceGrantReports;
  std::vector<std::wstring> initiateFederateSaveReports;
  std::vector<TimestampedSaveInitiationReport> timestampedSaveInitiationReports;
  std::size_t federationSavedReportCount = 0U;
  std::vector<std::string> callbackOrder;
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
    "Embedded federation MOM save conditionals follow pending admission and completion",
    "[integration][development-profile][federation-management][mom][save-restore]"
    "[time-management][timestamped-save]"
    "[rti.service.request-federation-save][rti.service.time-advance-request]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.request-attribute-value-update]"
    "[federate.callback.initiate-federate-save]"
    "[federate.callback.time-advance-grant][federate.callback.federation-saved]"
    "[federate.callback.reflect-attribute-values]"
    "[federation-mom-save-conditionals]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador observerReports;
  ReportingFederateAmbassador constrainedReports;
  auto owner = makeRti();
  auto observer = makeRti();
  auto constrained = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(constrained->connect(constrainedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"save-conditionals-owner", L"regulator", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"save-conditionals-observer", L"observer", federationName));

  auto const federationMomClass = observer->getObjectClassHandle(
      standard_hla::mom::federation_object_class);
  auto const nextSaveNameAttribute = observer->getAttributeHandle(
      federationMomClass,
      standard_hla::mom::next_save_name);
  auto const nextSaveTimeAttribute = observer->getAttributeHandle(
      federationMomClass,
      standard_hla::mom::next_save_time);
  auto const lastSaveNameAttribute = observer->getAttributeHandle(
      federationMomClass,
      standard_hla::mom::last_save_name);
  auto const lastSaveTimeAttribute = observer->getAttributeHandle(
      federationMomClass,
      standard_hla::mom::last_save_time);
  auto const reliable = observer->getTransportationTypeHandle(
      standard_hla::mom::reliable);
  REQUIRE(federationMomClass.isValid());
  REQUIRE(nextSaveNameAttribute.isValid());
  REQUIRE(nextSaveTimeAttribute.isValid());
  REQUIRE(lastSaveNameAttribute.isValid());
  REQUIRE(lastSaveTimeAttribute.isValid());
  REQUIRE(reliable.isValid());

  AttributeHandleSet const conditionalAttributes{
      nextSaveNameAttribute,
      nextSaveTimeAttribute,
      lastSaveNameAttribute,
      lastSaveTimeAttribute};
  REQUIRE_NOTHROW(observer->subscribeObjectClassAttributes(
      federationMomClass,
      conditionalAttributes,
      true));
  drainCallbacks(*observer);

  auto const discoveredFederation = std::find_if(
      observerReports.objectDiscoveryReports.begin(),
      observerReports.objectDiscoveryReports.end(),
      [&](ReportingFederateAmbassador::ObjectDiscoveryReport const& report) {
        return report.objectClass == federationMomClass &&
            report.objectInstanceName == standard_hla::mom::federation;
      });
  REQUIRE(discoveredFederation != observerReports.objectDiscoveryReports.end());
  auto const federationObjectInstance = discoveredFederation->objectInstance;

  struct SaveConditionals final {
    std::wstring nextName;
    std::size_t nextTimeBytes = 0U;
    std::wstring lastName;
    std::size_t lastTimeBytes = 0U;
    std::optional<rti1516_2025::HLAinteger64Time> nextTime;
    std::optional<rti1516_2025::HLAinteger64Time> lastTime;
  };

  auto decodeConditionals = [&](ReportingFederateAmbassador::AttributeReflectionReport const& reflection) {
    REQUIRE(reflection.objectInstance == federationObjectInstance);
    REQUIRE(reflection.transportationType == reliable);
    REQUIRE_FALSE(reflection.producingFederate.isValid());
    REQUIRE_FALSE(reflection.sentRegionsSupplied);
    REQUIRE(reflection.userSuppliedTag.size() == 0U);

    SaveConditionals result;
    if (reflection.attributeValues.contains(nextSaveNameAttribute)) {
      rti1516_2025::HLAunicodeString nextName;
      REQUIRE_NOTHROW(nextName.decode(
          reflection.attributeValues.at(nextSaveNameAttribute)));
      result.nextName = nextName.get();
      result.nextTimeBytes = reflection.attributeValues.at(nextSaveTimeAttribute).size();
      if (result.nextTimeBytes != 0U) {
        rti1516_2025::HLAinteger64Time value;
        REQUIRE_NOTHROW(value.decode(
            reflection.attributeValues.at(nextSaveTimeAttribute)));
        result.nextTime = value;
      }
    }
    if (reflection.attributeValues.contains(lastSaveNameAttribute)) {
      rti1516_2025::HLAunicodeString lastName;
      REQUIRE_NOTHROW(lastName.decode(
          reflection.attributeValues.at(lastSaveNameAttribute)));
      result.lastName = lastName.get();
      result.lastTimeBytes = reflection.attributeValues.at(lastSaveTimeAttribute).size();
      if (result.lastTimeBytes != 0U) {
        rti1516_2025::HLAinteger64Time value;
        REQUIRE_NOTHROW(value.decode(
            reflection.attributeValues.at(lastSaveTimeAttribute)));
        result.lastTime = value;
      }
    }
    return result;
  };

  auto latestReflectionWith = [&](AttributeHandle const& first,
                                  AttributeHandle const& second) {
    auto const reflectionIterator = std::find_if(
        observerReports.attributeReflectionReports.rbegin(),
        observerReports.attributeReflectionReports.rend(),
        [&](ReportingFederateAmbassador::AttributeReflectionReport const& candidate) {
          return candidate.objectInstance == federationObjectInstance &&
              candidate.attributeValues.contains(first) &&
              candidate.attributeValues.contains(second);
        });
    REQUIRE(reflectionIterator != observerReports.attributeReflectionReports.rend());
    return *reflectionIterator;
  };

  auto requestConditionals = [&]() {
    auto const before = observerReports.attributeReflectionReports.size();
    REQUIRE_NOTHROW(observer->requestAttributeValueUpdate(
        federationObjectInstance,
        conditionalAttributes,
        VariableLengthData{}));
    drainCallbacks(*observer);
    REQUIRE(observerReports.attributeReflectionReports.size() > before);
    auto const reflection = latestReflectionWith(
        nextSaveNameAttribute,
        lastSaveNameAttribute);
    REQUIRE(reflection.attributeValues.size() == 4U);
    return decodeConditionals(reflection);
  };

  auto readNextConditionals = [&]() {
    auto const reflection = latestReflectionWith(
        nextSaveNameAttribute,
        nextSaveTimeAttribute);
    REQUIRE(reflection.attributeValues.size() == 2U);
    return decodeConditionals(reflection);
  };

  auto readLastConditionals = [&]() {
    auto const reflection = latestReflectionWith(
        lastSaveNameAttribute,
        lastSaveTimeAttribute);
    REQUIRE(reflection.attributeValues.size() == 2U);
    return decodeConditionals(reflection);
  };

  auto const initial = requestConditionals();
  REQUIRE(initial.nextName.empty());
  REQUIRE(initial.nextTimeBytes == 0U);
  REQUIRE(initial.lastName.empty());
  REQUIRE(initial.lastTimeBytes == 0U);

  REQUIRE_NOTHROW(constrained->joinFederationExecution(
      L"save-conditionals-constrained", L"time-constrained", federationName));
  REQUIRE_NOTHROW(constrained->enableTimeConstrained());
  REQUIRE_FALSE(constrained->evokeCallback(0.0));
  REQUIRE_NOTHROW(owner->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));

  auto const saveLabel = std::wstring{L"save-conditionals-pending"};
  auto const saveTime = rti1516_2025::HLAinteger64Time(7);
  REQUIRE_NOTHROW(owner->requestFederationSave(saveLabel, saveTime));
  auto const pending = requestConditionals();
  REQUIRE(pending.nextName == saveLabel);
  REQUIRE(pending.nextTime.has_value());
  REQUIRE(pending.nextTime->getTime() == 7);
  REQUIRE(pending.lastName.empty());
  REQUIRE(pending.lastTimeBytes == 0U);

  REQUIRE_NOTHROW(constrained->timeAdvanceRequest(saveTime));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(saveTime));
  REQUIRE_FALSE(constrained->evokeCallback(0.0));
  static_cast<void>(owner->evokeCallback(0.0));
  drainCallbacks(*observer);
  auto const admitted = readNextConditionals();
  REQUIRE(admitted.nextName.empty());
  REQUIRE(admitted.nextTimeBytes == 0U);
  REQUIRE(admitted.lastName.empty());
  REQUIRE(admitted.lastTimeBytes == 0U);

  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(constrained->federateSaveBegun());
  REQUIRE_NOTHROW(observer->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(constrained->federateSaveComplete());
  REQUIRE_NOTHROW(observer->federateSaveComplete());
  drainCallbacks(*owner);
  drainCallbacks(*constrained);
  drainCallbacks(*observer);
  REQUIRE(ownerReports.federationSavedReportCount == 1U);
  REQUIRE(constrainedReports.federationSavedReportCount == 1U);
  REQUIRE(observerReports.federationSavedReportCount == 1U);
  auto const completed = readLastConditionals();
  REQUIRE(completed.lastName == saveLabel);
  REQUIRE(completed.lastTime.has_value());
  REQUIRE(completed.lastTime->getTime() == 7);

  REQUIRE_NOTHROW(constrained->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(observer->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(observer->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(constrained->disconnect());
}
