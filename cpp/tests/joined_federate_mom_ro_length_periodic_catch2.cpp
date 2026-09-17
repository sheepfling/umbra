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
#error "The joined-federate MOM receive-order length tests require the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::ParameterHandle;
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
  return L"federation-joined-mom-ro-length-" +
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

  struct InteractionReport final {
    InteractionClassHandle interactionClass;
    ParameterHandleValueMap parameterValues;
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

  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      rti1516_2025::RegionHandleSet const* optionalSentRegions) override {
    static_cast<void>(optionalSentRegions);
    interactionReports.push_back({
        interactionClass,
        parameterValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
    });
  }

  std::vector<AttributeReflectionReport> attributeReflectionReports;
  std::vector<InteractionReport> interactionReports;
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
    "Embedded joined-federate MOM exposes receive-order queue length",
    "[integration][development-profile][federation-management][mom]"
    "[periodic-mom][interaction-management][callback-model]"
    "[request-attribute-value-update][send-interaction][receive-interaction]"
    "[reflect-attribute-values][joined-federate-mom-ro-length-periodic]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.subscribe-interaction-class]"
    "[rti.service.publish-interaction-class]"
    "[rti.service.request-attribute-value-update]"
    "[rti.service.send-interaction][rti.service.evoke-callback]"
    "[federate.callback.receive-interaction]"
    "[federate.callback.reflect-attribute-values]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("parameter-handle-provider-fom.xml").wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  auto const receiverFederate = receiver->joinFederationExecution(
      L"mom-ro-length-receiver", L"receiver", federationName);

  auto const momClass = receiver->getObjectClassHandle(
      standard_hla::mom::federate_object_class);
  auto const federateHandleAttribute = receiver->getAttributeHandle(
      momClass, standard_hla::mom::federate_handle);
  auto const roLengthAttribute = receiver->getAttributeHandle(
      momClass, L"HLAROlength");
  auto const reliable = receiver->getTransportationTypeHandle(
      standard_hla::mom::reliable);
  REQUIRE(momClass.isValid());
  REQUIRE(federateHandleAttribute.isValid());
  REQUIRE(roLengthAttribute.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(
      momClass,
      AttributeHandleSet{federateHandleAttribute, roLengthAttribute},
      true));

  auto const publisherFederate = publisher->joinFederationExecution(
      L"mom-ro-length-publisher", L"publisher", federationName);
  drainCallbacks(*receiver);
  auto const receiverMomObjectIterator = std::find_if(
      receiverReports.attributeReflectionReports.begin(),
      receiverReports.attributeReflectionReports.end(),
      [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
        auto const value = report.attributeValues.find(federateHandleAttribute);
        return value != report.attributeValues.end() &&
            variableLengthDataBytes(value->second) ==
                variableLengthDataBytes(receiverFederate.encode());
      });
  REQUIRE(receiverMomObjectIterator != receiverReports.attributeReflectionReports.end());
  auto const receiverMomObject = receiverMomObjectIterator->objectInstance;

  auto requestRoLength = [&] {
    auto const before = receiverReports.attributeReflectionReports.size();
    REQUIRE_NOTHROW(receiver->requestAttributeValueUpdate(
        receiverMomObject,
        AttributeHandleSet{roLengthAttribute},
        VariableLengthData{}));
    drainCallbacks(*receiver);
    auto const reflectionIterator = std::find_if(
        receiverReports.attributeReflectionReports.begin() +
            static_cast<std::ptrdiff_t>(before),
        receiverReports.attributeReflectionReports.end(),
        [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
          return report.objectInstance == receiverMomObject &&
              report.attributeValues.contains(roLengthAttribute);
        });
    REQUIRE(reflectionIterator != receiverReports.attributeReflectionReports.end());
    auto const& reflection = *reflectionIterator;
    REQUIRE(reflection.attributeValues.size() == 1U);
    REQUIRE(reflection.transportationType == reliable);
    REQUIRE_FALSE(reflection.producingFederate.isValid());
    REQUIRE(reflection.userSuppliedTag.size() == 0U);
    rti1516_2025::HLAinteger32BE decoded;
    REQUIRE_NOTHROW(decoded.decode(reflection.attributeValues.at(roLengthAttribute)));
    return decoded.get();
  };

  REQUIRE(requestRoLength() == 0);

  auto const interactionClass = publisher->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraParameterFixtureBase.UmbraParameterFixtureChild");
  auto const identifier = publisher->getParameterHandle(
      interactionClass, L"Identifier");
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));

  auto sendFixtureInteraction = [&](wchar_t const* value) {
    REQUIRE_NOTHROW(publisher->sendInteraction(
        interactionClass,
        ParameterHandleValueMap{
            {identifier, rti1516_2025::HLAunicodeString(value).encode()}},
        VariableLengthData{}));
  };

  sendFixtureInteraction(L"queued-once");
  REQUIRE(receiverReports.interactionReports.empty());
  REQUIRE(requestRoLength() == 1);
  REQUIRE(receiverReports.interactionReports.size() == 1U);
  REQUIRE(receiverReports.interactionReports.front().interactionClass == interactionClass);
  REQUIRE(receiverReports.interactionReports.front().parameterValues.size() == 1U);
  REQUIRE(receiverReports.interactionReports.front().transportationType == reliable);
  REQUIRE(receiverReports.interactionReports.front().producingFederate == publisherFederate);

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

  sendFixtureInteraction(L"queued-periodic");
  auto const beforePeriodic = receiverReports.attributeReflectionReports.size();
  std::this_thread::sleep_for(std::chrono::milliseconds(1100));
  drainCallbacks(*receiver);
  auto const periodicReflection = std::find_if(
      receiverReports.attributeReflectionReports.begin() +
          static_cast<std::ptrdiff_t>(beforePeriodic),
      receiverReports.attributeReflectionReports.end(),
      [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
        return report.objectInstance == receiverMomObject &&
            report.attributeValues.contains(roLengthAttribute);
      });
  REQUIRE(periodicReflection != receiverReports.attributeReflectionReports.end());
  REQUIRE(periodicReflection->attributeValues.size() == 1U);
  REQUIRE(periodicReflection->transportationType == reliable);
  REQUIRE_FALSE(periodicReflection->producingFederate.isValid());
  REQUIRE(periodicReflection->userSuppliedTag.size() == 0U);
  rti1516_2025::HLAinteger32BE periodicCount;
  REQUIRE_NOTHROW(periodicCount.decode(
      periodicReflection->attributeValues.at(roLengthAttribute)));
  REQUIRE(periodicCount.get() == 1);
  REQUIRE(receiverReports.interactionReports.size() == 2U);
  REQUIRE(requestRoLength() == 0);

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
