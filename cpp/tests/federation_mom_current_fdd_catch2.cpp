#include <catch2/catch_test_macros.hpp>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/BasicDataElements.h>

#include "internal/fom/hla_names.hpp"

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The federation MOM Current FDD tests require the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::OrderType;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::ParameterHandle;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RECEIVE;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

struct Discovery final {
  ObjectInstanceHandle objectInstance;
  ObjectClassHandle objectClass;
  std::wstring objectInstanceName;
  FederateHandle producingFederate;
};

struct Reflection final {
  ObjectInstanceHandle objectInstance;
  AttributeHandleValueMap attributeValues;
  VariableLengthData userSuppliedTag;
  TransportationTypeHandle transportationType;
  FederateHandle producingFederate;
  bool sentRegionsSupplied = false;
};

struct TransportationReport final {
  ObjectInstanceHandle objectInstance;
  AttributeHandle attribute;
  TransportationTypeHandle transportationType;
};

class MomObserver final : public NullFederateAmbassador {
 public:
  struct InteractionReport final {
    InteractionClassHandle interactionClass;
    ParameterHandleValueMap parameterValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    bool sentRegionsSupplied = false;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const& objectClass,
      std::wstring const& objectInstanceName,
      FederateHandle const& producingFederate) override {
    discoveries.push_back({
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
    reflections.push_back({
        objectInstance,
        attributeValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        optionalSentRegions != nullptr,
    });
  }

  void reportAttributeTransportationType(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandle const& attribute,
      TransportationTypeHandle const& transportationType) override {
    transportationReports.push_back({
        objectInstance,
        attribute,
        transportationType,
    });
  }

  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      rti1516_2025::RegionHandleSet const* optionalSentRegions) override {
    interactionReports.push_back({
        interactionClass,
        parameterValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        optionalSentRegions != nullptr,
    });
  }

  std::vector<Discovery> discoveries;
  std::vector<Reflection> reflections;
  std::vector<TransportationReport> transportationReports;
  std::vector<InteractionReport> interactionReports;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / relativePath;
}

std::filesystem::path testDataPath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / relativePath;
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"federation-mom-current-fdd-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

void drain(RTIambassador& rti) {
  for (int pass = 0; pass != 256; ++pass) {
    if (!rti.evokeCallback(0.0)) {
      break;
    }
  }
}

std::wstring decodeCurrentFdd(
    AttributeHandleValueMap const& values,
    AttributeHandle const& attribute) {
  auto const value = values.find(attribute);
  REQUIRE(value != values.end());
  rti1516_2025::HLAunicodeString currentFdd;
  REQUIRE_NOTHROW(currentFdd.decode(value->second));
  return currentFdd.get();
}

}  // namespace

TEST_CASE(
    "Embedded federation MOM exposes and refreshes HLAcurrentFDD",
    "[integration][development-profile][federation-management][mom][fom]"
    "[object-management][fom-module-management][2025]"
    "[federation-mom-current-fdd]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.request-attribute-value-update]"
    "[rti.service.get-known-object-class-handle]"
    "[rti.service.get-object-instance-handle]"
    "[rti.service.get-object-instance-name]"
    "[rti.service.query-attribute-transportation-type]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values]"
    "[federate.callback.report-attribute-transportation-type]") {
  NullFederateAmbassador creatorReports;
  MomObserver observerReports;
  NullFederateAmbassador extensionReports;
  auto creator = makeRti();
  auto observer = makeRti();
  auto extensionJoiner = makeRti();
  auto const federationName = nextFederationName();
  auto const baseFom = resourcePath("examples/RestaurantFOMmodule-2025.xml");
  auto const extensionFom = testDataPath("reference-data-class-provider-fom.xml");

  REQUIRE_NOTHROW(creator->connect(creatorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(extensionJoiner->connect(extensionReports, HLA_EVOKED));
  REQUIRE_NOTHROW(creator->createFederationExecution(
      federationName,
      baseFom.wstring(),
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(creator->joinFederationExecution(
      L"current-fdd-creator", L"creator", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"current-fdd-observer", L"observer", federationName));

  auto const federationMomClass = observer->getObjectClassHandle(
      standard_hla::mom::federation_object_class);
  auto const currentFddAttribute = observer->getAttributeHandle(
      federationMomClass,
      standard_hla::mom::current_fdd);
  auto const reliable = observer->getTransportationTypeHandle(
      standard_hla::mom::reliable);
  REQUIRE(federationMomClass.isValid());
  REQUIRE(currentFddAttribute.isValid());
  REQUIRE(reliable.isValid());

  REQUIRE_NOTHROW(observer->subscribeObjectClassAttributes(
      federationMomClass,
      AttributeHandleSet{currentFddAttribute},
      true));
  drain(*observer);

  auto const discoveredFederation = std::find_if(
      observerReports.discoveries.begin(),
      observerReports.discoveries.end(),
      [&](Discovery const& discovery) {
        return discovery.objectClass == federationMomClass &&
            discovery.objectInstanceName == standard_hla::mom::federation;
      });
  REQUIRE(discoveredFederation != observerReports.discoveries.end());
  auto const federationObjectInstance = discoveredFederation->objectInstance;
  REQUIRE(observer->getKnownObjectClassHandle(federationObjectInstance) ==
          federationMomClass);
  REQUIRE(observer->getObjectInstanceName(federationObjectInstance) ==
          standard_hla::mom::federation);
  REQUIRE(observer->getObjectInstanceHandle(standard_hla::mom::federation) ==
          federationObjectInstance);

  auto const beforeTransportationQuery = observerReports.transportationReports.size();
  REQUIRE_NOTHROW(observer->queryAttributeTransportationType(
      federationObjectInstance,
      currentFddAttribute));
  drain(*observer);
  REQUIRE(observerReports.transportationReports.size() ==
          beforeTransportationQuery + 1U);
  auto const& transportation = observerReports.transportationReports.back();
  REQUIRE(transportation.objectInstance == federationObjectInstance);
  REQUIRE(transportation.attribute == currentFddAttribute);
  REQUIRE(transportation.transportationType == reliable);

  auto requestCurrentFdd = [&]() {
    auto const before = observerReports.reflections.size();
    REQUIRE_NOTHROW(observer->requestAttributeValueUpdate(
        federationObjectInstance,
        AttributeHandleSet{currentFddAttribute},
        VariableLengthData{}));
    drain(*observer);
    REQUIRE(observerReports.reflections.size() == before + 1U);
    auto const& reflection = observerReports.reflections.back();
    REQUIRE(reflection.objectInstance == federationObjectInstance);
    REQUIRE(reflection.attributeValues.size() == 1U);
    REQUIRE(reflection.transportationType == reliable);
    REQUIRE_FALSE(reflection.producingFederate.isValid());
    REQUIRE(reflection.userSuppliedTag.size() == 0U);
    REQUIRE_FALSE(reflection.sentRegionsSupplied);
    return decodeCurrentFdd(reflection.attributeValues, currentFddAttribute);
  };

  auto const baseCurrentFdd = requestCurrentFdd();
  REQUIRE_FALSE(baseCurrentFdd.empty());
  REQUIRE(baseCurrentFdd.find(L"objectModel") != std::wstring::npos);
  REQUIRE(baseCurrentFdd.find(L"UmbraReferenceFixtureClass") == std::wstring::npos);

  REQUIRE_NOTHROW(extensionJoiner->joinFederationExecution(
      L"current-fdd-extension",
      L"extension",
      federationName,
      std::vector<std::wstring>{extensionFom.wstring()}));
  drain(*observer);

  auto const refreshed = std::find_if(
      observerReports.reflections.begin(),
      observerReports.reflections.end(),
      [&](Reflection const& reflection) {
        return reflection.objectInstance == federationObjectInstance &&
            reflection.attributeValues.contains(currentFddAttribute) &&
            decodeCurrentFdd(reflection.attributeValues, currentFddAttribute) !=
                baseCurrentFdd;
      });
  REQUIRE(refreshed != observerReports.reflections.end());
  auto const refreshedCurrentFdd = decodeCurrentFdd(
      refreshed->attributeValues,
      currentFddAttribute);
  REQUIRE(refreshedCurrentFdd.find(L"UmbraReferenceFixtureClass") !=
          std::wstring::npos);
  REQUIRE(refreshedCurrentFdd.size() > baseCurrentFdd.size());

  auto const directlyRequestedCurrentFdd = requestCurrentFdd();
  REQUIRE(directlyRequestedCurrentFdd == refreshedCurrentFdd);

  REQUIRE_NOTHROW(extensionJoiner->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(observer->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(creator->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(creator->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(creator->disconnect());
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(extensionJoiner->disconnect());
}

TEST_CASE(
    "Embedded federation MOM content reports FOM module and MIM data through a focused lane",
    "[integration][development-profile][federation-management][mom]"
    "[mom-request-report][fom-module-management][interaction-management]"
    "[federation-mom-content-reports][2025]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.send-interaction][rti.service.evoke-callback]"
    "[federate.callback.receive-interaction]") {
  MomObserver subjectReports;
  MomObserver observerReports;
  auto subject = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(subject->connect(subjectReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(subject->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(subject->joinFederationExecution(
      L"mom-content-subject", L"subject", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"mom-content-observer", L"observer", federationName));

  auto const fomRequestClass = observer->getInteractionClassHandle(
      standard_hla::mom::request_fom_module_data_federation);
  auto const fomRequestIndicator = observer->getParameterHandle(
      fomRequestClass,
      standard_hla::mom::fom_module_indicator);
  auto const fomReportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_fom_module_data_federation);
  auto const fomReportIndicator = observer->getParameterHandle(
      fomReportClass,
      standard_hla::mom::fom_module_indicator);
  auto const fomReportData = observer->getParameterHandle(
      fomReportClass,
      standard_hla::mom::fom_module_data);
  auto const mimRequestClass = observer->getInteractionClassHandle(
      standard_hla::mom::request_mim_data);
  auto const mimReportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_mim_data);
  auto const mimReportData = observer->getParameterHandle(
      mimReportClass,
      standard_hla::mom::mim_data);
  auto const reliable = observer->getTransportationTypeHandle(
      standard_hla::mom::reliable);

  REQUIRE(fomRequestClass.isValid());
  REQUIRE(fomRequestIndicator.isValid());
  REQUIRE(fomReportClass.isValid());
  REQUIRE(fomReportIndicator.isValid());
  REQUIRE(fomReportData.isValid());
  REQUIRE(mimRequestClass.isValid());
  REQUIRE(mimReportClass.isValid());
  REQUIRE(mimReportData.isValid());
  REQUIRE(reliable.isValid());

  REQUIRE_NOTHROW(observer->subscribeInteractionClass(fomReportClass));
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(mimReportClass));
  drain(*observer);
  observerReports.interactionReports.clear();

  REQUIRE_NOTHROW(subject->sendInteraction(
      fomRequestClass,
      ParameterHandleValueMap{
          {fomRequestIndicator, rti1516_2025::HLAinteger32BE{0}.encode()},
      },
      VariableLengthData{}));
  REQUIRE(observerReports.interactionReports.empty());
  drain(*observer);
  REQUIRE(observerReports.interactionReports.size() == 1U);
  auto const& fomReport = observerReports.interactionReports.front();
  REQUIRE(fomReport.interactionClass == fomReportClass);
  REQUIRE(fomReport.parameterValues.size() == 2U);
  REQUIRE(fomReport.userSuppliedTag.size() == 0U);
  REQUIRE(fomReport.transportationType == reliable);
  REQUIRE_FALSE(fomReport.producingFederate.isValid());
  REQUIRE_FALSE(fomReport.sentRegionsSupplied);
  rti1516_2025::HLAinteger32BE decodedIndicator;
  REQUIRE_NOTHROW(decodedIndicator.decode(
      fomReport.parameterValues.at(fomReportIndicator)));
  REQUIRE(decodedIndicator.get() == 0);
  rti1516_2025::HLAunicodeString decodedFom;
  REQUIRE_NOTHROW(decodedFom.decode(fomReport.parameterValues.at(fomReportData)));
  REQUIRE(decodedFom.get().find(L"<objectModel") != std::wstring::npos);
  REQUIRE(decodedFom.get().find(L"Restaurant FOM Module") != std::wstring::npos);

  // A report planned before Evoke is still subscriber-gated at callback
  // entry.  Removing the subscription must suppress that queued callback.
  observerReports.interactionReports.clear();
  REQUIRE_NOTHROW(subject->sendInteraction(
      fomRequestClass,
      ParameterHandleValueMap{
          {fomRequestIndicator, rti1516_2025::HLAinteger32BE{0}.encode()},
      },
      VariableLengthData{}));
  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(fomReportClass));
  drain(*observer);
  REQUIRE(observerReports.interactionReports.empty());
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(fomReportClass));
  drain(*observer);

  observerReports.interactionReports.clear();
  REQUIRE_NOTHROW(subject->sendInteraction(
      mimRequestClass,
      ParameterHandleValueMap{},
      VariableLengthData{}));
  drain(*observer);
  REQUIRE(observerReports.interactionReports.size() == 1U);
  auto const& mimReport = observerReports.interactionReports.front();
  REQUIRE(mimReport.interactionClass == mimReportClass);
  REQUIRE(mimReport.parameterValues.size() == 1U);
  REQUIRE(mimReport.userSuppliedTag.size() == 0U);
  REQUIRE(mimReport.transportationType == reliable);
  REQUIRE_FALSE(mimReport.producingFederate.isValid());
  REQUIRE_FALSE(mimReport.sentRegionsSupplied);
  rti1516_2025::HLAunicodeString decodedMim;
  REQUIRE_NOTHROW(decodedMim.decode(mimReport.parameterValues.at(mimReportData)));
  REQUIRE(decodedMim.get().find(standard_hla::mom::request_mim_data_name) !=
          std::wstring::npos);
  REQUIRE(decodedMim.get().find(L"Standard MOM") != std::wstring::npos);

  observerReports.interactionReports.clear();
  REQUIRE_THROWS_AS(
      subject->sendInteraction(
          fomRequestClass,
          ParameterHandleValueMap{
              {fomRequestIndicator, rti1516_2025::HLAinteger32BE{99}.encode()},
          },
          VariableLengthData{}),
      rti1516_2025::RTIinternalError);
  REQUIRE_THROWS_AS(
      subject->sendInteraction(
          mimRequestClass,
          ParameterHandleValueMap{
              {fomRequestIndicator, rti1516_2025::HLAinteger32BE{0}.encode()},
          },
          VariableLengthData{}),
      rti1516_2025::InteractionParameterNotDefined);
  drain(*observer);
  REQUIRE(observerReports.interactionReports.empty());

  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(fomReportClass));
  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(mimReportClass));
  REQUIRE_NOTHROW(observer->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(subject->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subject->disconnect());
  REQUIRE_NOTHROW(observer->disconnect());
}
