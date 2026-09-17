#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/BasicDataElements.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The joined-federate MOM updates-sent periodic tests require the Umbra source directory."
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
using rti1516_2025::ParameterHandleValueMap;
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
  return L"federation-joined-mom-updates-sent-periodic-" +
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
    "Embedded joined-federate MOM exposes HLAupdatesSent count",
    "[integration][development-profile][federation-management][mom]"
    "[periodic-mom][object-management][callback-model]"
    "[request-attribute-value-update][update-attribute-values][send-interaction]"
    "[reflect-attribute-values][joined-federate-mom-updates-sent-periodic]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.request-attribute-value-update]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.register-object-instance][rti.service.update-attribute-values]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.send-interaction][rti.service.evoke-callback]"
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
      L"mom-updates-sent-periodic-observer", L"observer", federationName);

  auto const momClass = observer->getObjectClassHandle(
      standard_hla::mom::federate_object_class);
  auto const federateHandleAttribute = observer->getAttributeHandle(
      momClass, standard_hla::mom::federate_handle);
  auto const updatesSentAttribute = observer->getAttributeHandle(
      momClass, L"HLAupdatesSent");
  auto const reliable = observer->getTransportationTypeHandle(
      standard_hla::mom::reliable);
  REQUIRE(momClass.isValid());
  REQUIRE(federateHandleAttribute.isValid());
  REQUIRE(updatesSentAttribute.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE_NOTHROW(observer->subscribeObjectClassAttributes(
      momClass,
      AttributeHandleSet{federateHandleAttribute, updatesSentAttribute},
      true));

  auto const ownerFederate = owner->joinFederationExecution(
      L"mom-updates-sent-periodic-owner", L"owner", federationName);
  drainCallbacks(*observer);
  auto const ownerMomObjectIterator = std::find_if(
      observerReports.attributeReflectionReports.begin(),
      observerReports.attributeReflectionReports.end(),
      [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
        auto const value = report.attributeValues.find(federateHandleAttribute);
        return value != report.attributeValues.end() &&
            variableLengthDataBytes(value->second) ==
                variableLengthDataBytes(ownerFederate.encode());
      });
  REQUIRE(ownerMomObjectIterator != observerReports.attributeReflectionReports.end());
  auto const ownerMomObject = ownerMomObjectIterator->objectInstance;

  auto requestUpdatesSent = [&] {
    auto const before = observerReports.attributeReflectionReports.size();
    REQUIRE_NOTHROW(observer->requestAttributeValueUpdate(
        ownerMomObject,
        AttributeHandleSet{updatesSentAttribute},
        VariableLengthData{}));
    drainCallbacks(*observer);
    auto const reflectionIterator = std::find_if(
        observerReports.attributeReflectionReports.begin() +
            static_cast<std::ptrdiff_t>(before),
        observerReports.attributeReflectionReports.end(),
        [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
          return report.objectInstance == ownerMomObject &&
              report.attributeValues.contains(updatesSentAttribute);
        });
    REQUIRE(reflectionIterator != observerReports.attributeReflectionReports.end());
    auto const& reflection = *reflectionIterator;
    REQUIRE(reflection.attributeValues.size() == 1U);
    REQUIRE(reflection.transportationType == reliable);
    REQUIRE_FALSE(reflection.producingFederate.isValid());
    REQUIRE(reflection.userSuppliedTag.size() == 0U);
    HLAinteger32BE decoded;
    REQUIRE_NOTHROW(decoded.decode(
        reflection.attributeValues.at(updatesSentAttribute)));
    return decoded.get();
  };

  REQUIRE(requestUpdatesSent() == 0);

  auto const objectClass = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableAttribute = owner->getAttributeHandle(
      objectClass, L"ReliableBaseA");
  REQUIRE(objectClass.isValid());
  REQUIRE(reliableAttribute.isValid());
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
      objectClass, AttributeHandleSet{reliableAttribute}));
  auto const objectInstance = owner->registerObjectInstance(objectClass);
  REQUIRE(objectInstance.isValid());

  REQUIRE_NOTHROW(owner->updateAttributeValues(
      objectInstance,
      AttributeHandleValueMap{{reliableAttribute, HLAinteger32BE{1}.encode()}},
      VariableLengthData{}));
  REQUIRE(requestUpdatesSent() == 1);
  REQUIRE_NOTHROW(owner->updateAttributeValues(
      objectInstance,
      AttributeHandleValueMap{{reliableAttribute, HLAinteger32BE{2}.encode()}},
      VariableLengthData{}));
  REQUIRE(requestUpdatesSent() == 2);

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
          {federateParameter, ownerFederate.encode()},
          {periodParameter, HLAinteger32BE{1}.encode()}},
      VariableLengthData{}));
  auto const beforePeriodic = observerReports.attributeReflectionReports.size();
  std::this_thread::sleep_for(std::chrono::milliseconds(1100));
  drainCallbacks(*observer);
  auto const periodicReflection = std::find_if(
      observerReports.attributeReflectionReports.begin() +
          static_cast<std::ptrdiff_t>(beforePeriodic),
      observerReports.attributeReflectionReports.end(),
      [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
        return report.objectInstance == ownerMomObject &&
            report.attributeValues.contains(updatesSentAttribute);
      });
  REQUIRE(periodicReflection != observerReports.attributeReflectionReports.end());
  REQUIRE(periodicReflection->attributeValues.size() == 1U);
  REQUIRE(periodicReflection->transportationType == reliable);
  REQUIRE_FALSE(periodicReflection->producingFederate.isValid());
  REQUIRE(periodicReflection->userSuppliedTag.size() == 0U);
  HLAinteger32BE periodicCount;
  REQUIRE_NOTHROW(periodicCount.decode(
      periodicReflection->attributeValues.at(updatesSentAttribute)));
  REQUIRE(periodicCount.get() == 2);

  REQUIRE_NOTHROW(observer->sendInteraction(
      setTiming,
      ParameterHandleValueMap{
          {federateParameter, ownerFederate.encode()},
          {periodParameter, HLAinteger32BE{0}.encode()}},
      VariableLengthData{}));
  REQUIRE_NOTHROW(observer->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(observer->disconnect());
}
