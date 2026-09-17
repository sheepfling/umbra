#include <catch2/catch_test_macros.hpp>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/BasicDataElements.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <stdexcept>
#include <system_error>
#include <vector>

#include <umbra/embedded_profile_configuration.hpp>

namespace {

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::RtiConfiguration;
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
  TransportationTypeHandle transportationType;
  FederateHandle producingFederate;
};

class MomObserver final : public NullFederateAmbassador {
 public:
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
      VariableLengthData const&,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      rti1516_2025::RegionHandleSet const*) override {
    reflections.push_back({
        objectInstance,
        attributeValues,
        transportationType,
        producingFederate,
    });
  }

  std::vector<Discovery> discoveries;
  std::vector<Reflection> reflections;
};

std::filesystem::path reserveDirectory() {
  static std::atomic_uint64_t sequence{0U};
  auto const parent = std::filesystem::temp_directory_path();
  auto const tick = std::chrono::high_resolution_clock::now()
                        .time_since_epoch()
                        .count();
  for (std::size_t attempt = 0U; attempt != 128U; ++attempt) {
    auto const candidate =
        parent / ("umbra-joined-federate-mom-report-file-" +
                  std::to_string(tick) + "-" +
                  std::to_string(sequence.fetch_add(1U) + attempt));
    std::error_code error;
    if (std::filesystem::create_directory(candidate, error)) {
      return candidate;
    }
    if (error && error != std::errc::file_exists) {
      throw std::system_error(error, "Unable to reserve report directory");
    }
  }
  throw std::runtime_error("Unable to reserve a unique report directory");
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"joined-federate-mom-report-file-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

RtiConfiguration configurationFor(std::filesystem::path const& directory) {
  auto configuration = umbra::embedded::makeEmbeddedRtiConfiguration(
      umbra::embedded::ServiceReportConfiguration{directory});
  configuration.withRtiAddress(L"in-process");
  return configuration;
}

void drain(rti1516_2025::RTIambassador& ambassador) {
  while (ambassador.evokeCallback(0.0)) {
  }
}

}  // namespace

TEST_CASE(
    "Embedded HLAreportServiceFile is published as a stable joined-federate MOM attribute",
    "[integration][development-profile][federation-management][mom]"
    "[service-report-file][service-reporting][joined-federate-mom][2025]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.request-attribute-value-update]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values]") {
  using rti1516_2025::NO_ACTION;

  auto const subjectDirectory = reserveDirectory();
  auto const observerDirectory = reserveDirectory();
  auto const federationName = nextFederationName();
  auto const fomModule =
      std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / "switch-nrg-disabled-fom.xml";

  RTIambassadorFactory factory;
  auto subject = factory.createRTIambassador();
  auto observer = factory.createRTIambassador();
  rti1516_2025::NullFederateAmbassador subjectReports;
  MomObserver observerReports;

  REQUIRE_NOTHROW(subject->connect(
      subjectReports, HLA_EVOKED, configurationFor(subjectDirectory)));
  REQUIRE_NOTHROW(observer->connect(
      observerReports, HLA_EVOKED, configurationFor(observerDirectory)));
  REQUIRE_NOTHROW(subject->createFederationExecution(
      federationName, fomModule.wstring(), L"HLAinteger64Time"));
  auto const subjectFederate = subject->joinFederationExecution(
      L"joined-federate-mom-report-subject", L"subject", federationName);
  REQUIRE(subjectFederate.isValid());
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"joined-federate-mom-report-observer", L"observer", federationName));

  auto const momClass =
      observer->getObjectClassHandle(L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const reportFileAttribute =
      observer->getAttributeHandle(momClass, L"HLAreportServiceFile");
  auto const federateNameAttribute =
      observer->getAttributeHandle(momClass, L"HLAfederateName");
  REQUIRE(momClass.isValid());
  REQUIRE(reportFileAttribute.isValid());
  REQUIRE(federateNameAttribute.isValid());

  AttributeHandleSet const requestedAttributes{
      reportFileAttribute,
      federateNameAttribute,
  };
  REQUIRE_NOTHROW(observer->subscribeObjectClassAttributes(
      momClass, requestedAttributes, true));
  drain(*observer);

  // The observer itself is also a joined federate, so an active MOM
  // subscription receives one RTI-owned object per joined lifetime. Locate
  // the subject by its reflected federate name rather than relying on callback
  // ordering.
  REQUIRE(observerReports.discoveries.size() == 2U);
  REQUIRE(observerReports.reflections.size() == 2U);
  auto const subjectReflection = std::find_if(
      observerReports.reflections.begin(),
      observerReports.reflections.end(),
      [&](Reflection const& reflection) {
        auto const value = reflection.attributeValues.find(federateNameAttribute);
        if (value == reflection.attributeValues.end()) {
          return false;
        }
        rti1516_2025::HLAunicodeString name;
        try {
          name.decode(value->second);
        } catch (...) {
          return false;
        }
        return name.get() == L"joined-federate-mom-report-subject";
  });
  REQUIRE(subjectReflection != observerReports.reflections.end());
  // Keep a value copy because the requested-value callback below appends to
  // the same vector and may reallocate it.
  auto const initialReflection = *subjectReflection;
  auto const subjectDiscovery = std::find_if(
      observerReports.discoveries.begin(),
      observerReports.discoveries.end(),
      [&](Discovery const& discovery) {
        return discovery.objectInstance == initialReflection.objectInstance;
      });
  REQUIRE(subjectDiscovery != observerReports.discoveries.end());
  REQUIRE(subjectDiscovery->objectClass == momClass);
  REQUIRE(subjectDiscovery->objectInstanceName.size() > 0U);
  REQUIRE_FALSE(subjectDiscovery->producingFederate.isValid());
  REQUIRE(initialReflection.attributeValues.size() == requestedAttributes.size());
  REQUIRE_FALSE(initialReflection.producingFederate.isValid());

  rti1516_2025::HLAunicodeString advertisedPath;
  REQUIRE_NOTHROW(advertisedPath.decode(
      initialReflection.attributeValues.at(reportFileAttribute)));
  auto const reportPath = std::filesystem::path(advertisedPath.get());
  REQUIRE(reportPath.is_absolute());
  REQUIRE(reportPath.lexically_normal() == reportPath);
  REQUIRE(reportPath.parent_path() ==
          std::filesystem::absolute(subjectDirectory).lexically_normal());
  REQUIRE(std::filesystem::exists(reportPath));

  rti1516_2025::HLAunicodeString reflectedFederateName;
  REQUIRE_NOTHROW(reflectedFederateName.decode(
      initialReflection.attributeValues.at(federateNameAttribute)));
  REQUIRE(reflectedFederateName.get() ==
          L"joined-federate-mom-report-subject");

  REQUIRE_NOTHROW(observer->requestAttributeValueUpdate(
      initialReflection.objectInstance,
      AttributeHandleSet{reportFileAttribute},
      VariableLengthData{}));
  drain(*observer);
  REQUIRE(observerReports.reflections.size() == 3U);
  auto const& requestedReflection = observerReports.reflections.back();
  REQUIRE(requestedReflection.objectInstance == initialReflection.objectInstance);
  rti1516_2025::HLAunicodeString requestedPath;
  REQUIRE_NOTHROW(requestedPath.decode(
      requestedReflection.attributeValues.at(reportFileAttribute)));
  REQUIRE(requestedPath.get() == advertisedPath.get());

  REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subject->disconnect());
  REQUIRE_NOTHROW(observer->disconnect());

  std::error_code ignored;
  std::filesystem::remove_all(subjectDirectory, ignored);
  std::filesystem::remove_all(observerDirectory, ignored);
}
