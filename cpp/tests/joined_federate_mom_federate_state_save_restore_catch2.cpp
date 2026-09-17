#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/time/HLAinteger64Time.h>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The joined-federate MOM save/restore tests require the Umbra source directory."
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
  return L"joined-federate-mom-federate-state-save-restore-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "third_party" / "ieee1516.2-2025" / "resources" / relativePath;
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

  void reflectAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      RegionHandleSet const* optionalSentRegions,
      LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
    attributeReflectionReports.push_back({
        objectInstance,
        attributeValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        optionalSentRegions != nullptr,
        optionalSentRegions == nullptr ? RegionHandleSet{} : *optionalSentRegions,
        time.implementationName(),
        time.toString(),
        sentOrderType,
        receivedOrderType,
        optionalRetraction != nullptr,
        optionalRetraction != nullptr && optionalRetraction->isValid(),
    });
  }

  std::vector<AttributeReflectionReport> attributeReflectionReports;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Embedded joined-federate MOM HLAfederateState follows save and restore callbacks",
    "[integration][development-profile][federation-management][mom][save-restore]"
    "[rti.service.join-federation-execution]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.confirm-federation-restoration-request]"
    "[rti.service.federate-restore-complete]"
    "[federate.callback.initiate-federate-save]"
    "[federate.callback.federation-saved]"
    "[federate.callback.federation-restore-begun]"
    "[federate.callback.initiate-federate-restore]"
    "[federate.callback.federation-restored]"
    "[joined-federate-mom-federate-state]") {
  auto runScenario = [](auto const callbackModel) {
    ReportingFederateAmbassador subjectReports;
    ReportingFederateAmbassador observerReports;
    auto subject = makeRti();
    auto observer = makeRti();
    auto const federationName = nextFederationName();
    auto const fomModule =
        resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

    REQUIRE_NOTHROW(subject->connect(subjectReports, callbackModel));
    REQUIRE_NOTHROW(observer->connect(observerReports, callbackModel));
    REQUIRE_NOTHROW(subject->createFederationExecution(
        federationName,
        fomModule,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(observer->joinFederationExecution(
        L"mom-save-restore-observer",
        L"observer",
        federationName));

    auto const momClass = observer->getObjectClassHandle(
        standard_hla::mom::federate_object_class);
    auto const federateHandleAttribute = observer->getAttributeHandle(
        momClass,
        standard_hla::mom::federate_handle);
    auto const federateStateAttribute = observer->getAttributeHandle(
        momClass,
        standard_hla::mom::federate_state);
    REQUIRE(momClass.isValid());
    REQUIRE(federateHandleAttribute.isValid());
    REQUIRE(federateStateAttribute.isValid());
    REQUIRE_NOTHROW(observer->subscribeObjectClassAttributes(
        momClass,
        AttributeHandleSet{federateHandleAttribute, federateStateAttribute},
        true));

    // Keep the observer subscribed before the subject joins so the
    // Join-Federation-Execution conditional boundary is exercised for the
    // RTI-owned HLAfederate object and its later state transitions.
    auto const subjectFederate = subject->joinFederationExecution(
        L"mom-save-restore-subject",
        L"subject",
        federationName);

    // The saving federate also observes its own MOM state, but the state
    // transition reflection is suppressed while that federate is saving.
    auto const subjectMomClass = subject->getObjectClassHandle(
        standard_hla::mom::federate_object_class);
    auto const subjectFederateHandleAttribute = subject->getAttributeHandle(
        subjectMomClass,
        standard_hla::mom::federate_handle);
    auto const subjectFederateStateAttribute = subject->getAttributeHandle(
        subjectMomClass,
        standard_hla::mom::federate_state);
    REQUIRE(subjectMomClass.isValid());
    REQUIRE(subjectFederateHandleAttribute.isValid());
    REQUIRE(subjectFederateStateAttribute.isValid());
    REQUIRE_NOTHROW(subject->subscribeObjectClassAttributes(
        subjectMomClass,
        AttributeHandleSet{
            subjectFederateHandleAttribute,
            subjectFederateStateAttribute},
        true));

    auto drainCallbacks = [&] {
      while (subject->evokeCallback(0.0)) {
      }
      while (observer->evokeCallback(0.0)) {
      }
    };
    drainCallbacks();

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
    REQUIRE(reflectedSubject->attributeValues.find(federateStateAttribute) ==
            reflectedSubject->attributeValues.end());

    auto const reflectedSubjectToSelf = std::find_if(
        subjectReports.attributeReflectionReports.begin(),
        subjectReports.attributeReflectionReports.end(),
        [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
          auto const value = report.attributeValues.find(
              subjectFederateHandleAttribute);
          return value != report.attributeValues.end() &&
              variableLengthDataBytes(value->second) ==
                  variableLengthDataBytes(subjectFederate.encode());
        });
    REQUIRE(reflectedSubjectToSelf != subjectReports.attributeReflectionReports.end());
    auto const subjectSelfObjectInstance = reflectedSubjectToSelf->objectInstance;
    REQUIRE(reflectedSubjectToSelf->attributeValues.find(
                subjectFederateStateAttribute) ==
            reflectedSubjectToSelf->attributeValues.end());

    auto decodeState = [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
      rti1516_2025::HLAinteger32BE state;
      REQUIRE_NOTHROW(state.decode(report.attributeValues.at(federateStateAttribute)));
      return state.get();
    };
    auto requireStateReflection = [&](std::size_t const before,
                                      std::int32_t const expected) {
      bool found = false;
      for (auto iterator = observerReports.attributeReflectionReports.begin() +
                                static_cast<std::ptrdiff_t>(before);
           iterator != observerReports.attributeReflectionReports.end();
           ++iterator) {
        if (iterator->objectInstance != subjectObjectInstance ||
            !iterator->attributeValues.contains(federateStateAttribute)) {
          continue;
        }
        REQUIRE(iterator->attributeValues.size() == 1U);
        REQUIRE(iterator->transportationType ==
                observer->getTransportationTypeHandle(standard_hla::mom::reliable));
        REQUIRE_FALSE(iterator->producingFederate.isValid());
        REQUIRE_FALSE(iterator->sentRegionsSupplied);
        if (decodeState(*iterator) == expected) {
          found = true;
        }
      }
      REQUIRE(found);
    };

    auto requireNoSubjectSaveStateReflection = [&](std::size_t const before) {
      std::size_t stateReflections = 0U;
      for (auto iterator = subjectReports.attributeReflectionReports.begin() +
                                static_cast<std::ptrdiff_t>(before);
           iterator != subjectReports.attributeReflectionReports.end();
           ++iterator) {
        if (iterator->objectInstance == subjectSelfObjectInstance &&
            iterator->attributeValues.contains(subjectFederateStateAttribute)) {
          ++stateReflections;
        }
      }
      REQUIRE(stateReflections == 0U);
    };

    auto const beforeInitialQuery = observerReports.attributeReflectionReports.size();
    REQUIRE_NOTHROW(observer->requestAttributeValueUpdate(
        subjectObjectInstance,
        AttributeHandleSet{federateStateAttribute},
        VariableLengthData{}));
    auto const beforeSubjectInitialQuery = subjectReports.attributeReflectionReports.size();
    REQUIRE_NOTHROW(subject->requestAttributeValueUpdate(
        subjectSelfObjectInstance,
        AttributeHandleSet{subjectFederateStateAttribute},
        VariableLengthData{}));
    drainCallbacks();
    REQUIRE(observerReports.attributeReflectionReports.size() == beforeInitialQuery + 1U);
    REQUIRE(subjectReports.attributeReflectionReports.size() ==
            beforeSubjectInitialQuery + 1U);
    auto const& initialState = observerReports.attributeReflectionReports.back();
    REQUIRE(initialState.objectInstance == subjectObjectInstance);
    REQUIRE(initialState.attributeValues.size() == 1U);
    REQUIRE(decodeState(initialState) == 1);
    auto const& subjectInitialState = subjectReports.attributeReflectionReports.back();
    REQUIRE(subjectInitialState.objectInstance == subjectSelfObjectInstance);
    REQUIRE(subjectInitialState.attributeValues.size() == 1U);
    rti1516_2025::HLAinteger32BE subjectState;
    REQUIRE_NOTHROW(subjectState.decode(
        subjectInitialState.attributeValues.at(subjectFederateStateAttribute)));
    REQUIRE(subjectState.get() == 1);

    auto const beforeSave = observerReports.attributeReflectionReports.size();
    auto const beforeSubjectSave = subjectReports.attributeReflectionReports.size();
    REQUIRE_NOTHROW(subject->requestFederationSave(L"mom-state-save"));
    drainCallbacks();
    requireStateReflection(beforeSave, 3);
    requireNoSubjectSaveStateReflection(beforeSubjectSave);

    REQUIRE_NOTHROW(subject->federateSaveBegun());
    REQUIRE_NOTHROW(observer->federateSaveBegun());
    REQUIRE_NOTHROW(subject->federateSaveComplete());
    auto const beforeSaveCompletion = observerReports.attributeReflectionReports.size();
    REQUIRE_NOTHROW(observer->federateSaveComplete());
    drainCallbacks();
    requireStateReflection(beforeSaveCompletion, 1);

    auto const beforeRestore = observerReports.attributeReflectionReports.size();
    REQUIRE_NOTHROW(subject->requestFederationRestore(L"mom-state-save"));
    drainCallbacks();
    requireStateReflection(beforeRestore, 5);

    REQUIRE_NOTHROW(subject->federateRestoreComplete());
    auto const beforeRestoreCompletion = observerReports.attributeReflectionReports.size();
    REQUIRE_NOTHROW(observer->federateRestoreComplete());
    drainCallbacks();
    requireStateReflection(beforeRestoreCompletion, 1);

    REQUIRE_NOTHROW(subject->resignFederationExecution(
        rti1516_2025::NO_ACTION));
    drainCallbacks();
    REQUIRE_NOTHROW(observer->resignFederationExecution(
        rti1516_2025::NO_ACTION));
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
