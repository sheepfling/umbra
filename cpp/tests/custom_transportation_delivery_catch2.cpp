#include <catch2/catch_test_macros.hpp>

#include "hla_test_names.hpp"
#include "internal/fom/hla_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/BasicDataElements.h>

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The custom transportation delivery test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"custom-transportation-delivery-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / relativePath;
}

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

void drainCallbacks(RTIambassador& rti) {
  for (int pass = 0; pass != 128; ++pass) {
    if (!rti.evokeCallback(0.0)) {
      break;
    }
  }
}

bool hasBytes(VariableLengthData const& value,
              std::vector<unsigned char> const& expected) {
  if (value.size() != expected.size()) {
    return false;
  }
  auto const* bytes = static_cast<unsigned char const*>(value.data());
  return bytes != nullptr && std::equal(expected.begin(), expected.end(), bytes);
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

  struct AttributeReflectionReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleValueMap attributeValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    bool sentRegionsSupplied = false;
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
        optionalSentRegions != nullptr});
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
        optionalSentRegions != nullptr});
  }

  std::vector<ObjectInstanceHandle> objectDiscoveryReports;
  std::vector<InteractionReport> interactionReports;
  std::vector<AttributeReflectionReport> attributeReflectionReports;
};

}  // namespace

TEST_CASE(
    "Embedded ordinary delivery accepts a declared custom FOM transportation",
    "[integration][development-profile][federation-management]"
    "[custom-transportation][custom-transportation-delivery]"
    "[fom][transportation-type-lookup][transportation]"
    "[interaction-management][object-management]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.publish-interaction-class][rti.service.subscribe-interaction-class]"
    "[rti.service.send-interaction]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.register-object-instance][rti.service.update-attribute-values]"
    "[rti.service.evoke-callback]"
    "[federate.callback.receive-interaction]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values]") {
  ReportingFederateAmbassador producerReports;
  ReportingFederateAmbassador consumerReports;
  auto producer = makeRti();
  auto consumer = makeRti();
  auto const federationName = nextFederationName();
  std::vector<std::wstring> const fomModules{
      resourcePath("transportation-reference-consumer-fom.xml").wstring(),
      resourcePath("transportation-reference-provider-fom.xml").wstring(),
  };

  REQUIRE_NOTHROW(producer->connect(producerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(consumer->connect(consumerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(producer->createFederationExecution(
      federationName,
      fomModules,
      standard_hla::mom::integer64_time));
  FederateHandle producerHandle;
  REQUIRE_NOTHROW(producerHandle = producer->joinFederationExecution(
      L"ordinary-transport-producer",
      L"producer",
      federationName));
  REQUIRE_NOTHROW(consumer->joinFederationExecution(
      L"ordinary-transport-consumer",
      L"consumer",
      federationName));

  auto const custom = producer->getTransportationTypeHandle(
      fixture_hla::fixture::umbra_transportation_fixture);
  REQUIRE(custom.isValid());

  auto const interaction = producer->getInteractionClassHandle(
      fixture_hla::fom::transportation_fixture_interaction);
  auto const consumerInteraction = consumer->getInteractionClassHandle(
      fixture_hla::fom::transportation_fixture_interaction);
  REQUIRE(interaction.isValid());
  REQUIRE(consumerInteraction.isValid());
  REQUIRE_NOTHROW(producer->publishInteractionClass(interaction));
  REQUIRE_NOTHROW(consumer->subscribeInteractionClass(consumerInteraction));

  auto const objectClass = producer->getObjectClassHandle(
      fixture_hla::fom::transportation_fixture_object);
  auto const consumerObjectClass = consumer->getObjectClassHandle(
      fixture_hla::fom::transportation_fixture_object);
  auto const attribute = producer->getAttributeHandle(
      objectClass,
      fixture_hla::fixture::umbra_transportation_fixture_attribute);
  auto const consumerAttribute = consumer->getAttributeHandle(
      consumerObjectClass,
      fixture_hla::fixture::umbra_transportation_fixture_attribute);
  REQUIRE(objectClass.isValid());
  REQUIRE(consumerObjectClass.isValid());
  REQUIRE(attribute.isValid());
  REQUIRE(consumerAttribute.isValid());
  AttributeHandleSet const producerAttributes{attribute};
  REQUIRE_NOTHROW(producer->publishObjectClassAttributes(
      objectClass,
      producerAttributes));
  REQUIRE_NOTHROW(consumer->subscribeObjectClassAttributes(
      consumerObjectClass,
      AttributeHandleSet{consumerAttribute}));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = producer->registerObjectInstance(objectClass));
  REQUIRE(objectInstance.isValid());
  drainCallbacks(*consumer);
  REQUIRE(consumerReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(consumerReports.objectDiscoveryReports.front() == objectInstance);

  unsigned char const tagBytes[] = {'O', 'R', 'D'};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  REQUIRE_NOTHROW(producer->sendInteraction(
      interaction,
      ParameterHandleValueMap{},
      tag));
  rti1516_2025::HLAunicodeString value{L"ordinary-custom-transportation-value"};
  AttributeHandleValueMap values{{attribute, value.encode()}};
  REQUIRE_NOTHROW(producer->updateAttributeValues(
      objectInstance,
      values,
      tag));
  drainCallbacks(*consumer);

  REQUIRE(consumerReports.interactionReports.size() == 1U);
  auto const& interactionReport = consumerReports.interactionReports.front();
  REQUIRE(interactionReport.interactionClass == consumerInteraction);
  REQUIRE(interactionReport.parameterValues.empty());
  REQUIRE(interactionReport.transportationType == custom);
  REQUIRE(interactionReport.producingFederate == producerHandle);
  REQUIRE_FALSE(interactionReport.sentRegionsSupplied);
  REQUIRE(hasBytes(
      interactionReport.userSuppliedTag,
      std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes))));

  REQUIRE(consumerReports.attributeReflectionReports.size() == 1U);
  auto const& reflection = consumerReports.attributeReflectionReports.front();
  REQUIRE(reflection.objectInstance == objectInstance);
  REQUIRE(reflection.attributeValues.size() == 1U);
  REQUIRE(reflection.attributeValues.contains(consumerAttribute));
  REQUIRE(reflection.transportationType == custom);
  REQUIRE(reflection.producingFederate == producerHandle);
  REQUIRE_FALSE(reflection.sentRegionsSupplied);
  REQUIRE(hasBytes(
      reflection.userSuppliedTag,
      std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes))));

  REQUIRE_NOTHROW(consumer->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(producer->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(producer->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(consumer->disconnect());
  REQUIRE_NOTHROW(producer->disconnect());
}
