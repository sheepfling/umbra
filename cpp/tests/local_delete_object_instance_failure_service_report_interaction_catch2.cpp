#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAvariableArray.h>

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The failed Local Delete Object Instance MOM interaction test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::AttributeHandleSet;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::HLA_IMMEDIATE;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::ParameterHandle;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"local-delete-object-instance-mom-failure-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / relativePath;
}

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct InteractionReport final {
    InteractionClassHandle interactionClass;
    ParameterHandleValueMap parameterValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
  };

  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      RegionHandleSet const*) override {
    interactionReports.push_back({
        interactionClass,
        parameterValues,
        userSuppliedTag,
        transportationType,
        producingFederate});
  }

  std::vector<InteractionReport> interactionReports;
};

}  // namespace

TEST_CASE(
    "Embedded service reporting delivers failed local-delete object-instance invocations through MOM interaction",
    "[integration][development-profile][federation-management][object-management]"
    "[mom][service-reporting][service-report-interaction][service-failure]"
    "[local-delete-failure][rti.service.local-delete-failure-matrix-interaction]") {
  rti1516_2025::NullFederateAmbassador ownerReports;
  rti1516_2025::NullFederateAmbassador requesterReports;
  ReportingFederateAmbassador observerReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath(
      "examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, HLA_IMMEDIATE));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      std::vector<std::wstring>{restaurantFom, switchFom},
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"local-delete-interaction-owner", L"owner", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"local-delete-interaction-requester", L"requester", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"local-delete-interaction-observer", L"observer", federationName));

  auto const reportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_service_invocation);
  REQUIRE(reportClass.isValid());
  std::vector<ParameterHandle> reportParameters;
  for (auto const& name : {
           standard_hla::mom::service,
           standard_hla::mom::service_type,
           standard_hla::mom::success_indicator,
           standard_hla::mom::supplied_arguments,
           standard_hla::mom::returned_argument,
           standard_hla::mom::exception,
           standard_hla::mom::serial_number}) {
    auto const parameter = observer->getParameterHandle(reportClass, name);
    REQUIRE(parameter.isValid());
    reportParameters.push_back(parameter);
  }
  REQUIRE_NOTHROW(observer->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(requester->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(requester->setSendServiceReportsToFileSwitch(false));

  auto const server = owner->getObjectClassHandle(
      L"HLAobjectRoot.Employee.Server");
  auto const efficiency = owner->getAttributeHandle(server, L"Efficiency");
  REQUIRE(server.isValid());
  REQUIRE(efficiency.isValid());
  AttributeHandleSet const attributes{efficiency};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, attributes));
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(server, attributes));
  auto const objectInstance = owner->registerObjectInstance(server);
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_NOTHROW(requester->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(requester->setSendServiceReportsToFileSwitch(false));

  auto const verifyReport = [&](std::size_t index,
                                std::wstring const& objectValue,
                                bool success,
                                std::wstring const& serviceException) {
    auto const& report = observerReports.interactionReports.at(index);
    REQUIRE(report.interactionClass == reportClass);
    REQUIRE(report.parameterValues.size() == 7U);
    REQUIRE(report.userSuppliedTag.size() == 0U);
    REQUIRE_FALSE(report.producingFederate.isValid());

    rti1516_2025::HLAunicodeString service;
    REQUIRE_NOTHROW(service.decode(report.parameterValues.at(reportParameters[0])));
    REQUIRE(service.get() == L"LocalDeleteObjectInstance");
    rti1516_2025::HLAinteger16BE serviceType;
    REQUIRE_NOTHROW(serviceType.decode(report.parameterValues.at(reportParameters[1])));
    REQUIRE(serviceType.get() == 2);
    rti1516_2025::HLAboolean successValue;
    REQUIRE_NOTHROW(successValue.decode(report.parameterValues.at(reportParameters[2])));
    REQUIRE(successValue.get() == success);

    rti1516_2025::HLAfixedRecord argumentPrototype;
    argumentPrototype.appendElement(rti1516_2025::HLAinteger32BE{})
        .appendElement(rti1516_2025::HLAunicodeString{})
        .appendElement(rti1516_2025::HLAunicodeString{});
    rti1516_2025::HLAvariableArray suppliedArguments{argumentPrototype};
    REQUIRE_NOTHROW(suppliedArguments.decode(report.parameterValues.at(reportParameters[3])));
    REQUIRE(suppliedArguments.size() == 1U);
    auto const& supplied = dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
        suppliedArguments.get(0U));
    REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(supplied.get(0U)).get() == 37);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(supplied.get(1U)).get() ==
            L"Object instance designator");
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(supplied.get(2U)).get() ==
            L"\"" + objectValue + L"\"");

    rti1516_2025::HLAfixedRecord nullReturned;
    nullReturned.appendElement(rti1516_2025::HLAinteger32BE{})
        .appendElement(rti1516_2025::HLAunicodeString{})
        .appendElement(rti1516_2025::HLAunicodeString{});
    REQUIRE_NOTHROW(nullReturned.decode(report.parameterValues.at(reportParameters[4])));
    REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(nullReturned.get(0U)).get() == 34);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(nullReturned.get(1U)).get().empty());
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(nullReturned.get(2U)).get() == L"null");
    rti1516_2025::HLAunicodeString decodedException;
    REQUIRE_NOTHROW(decodedException.decode(report.parameterValues.at(reportParameters[5])));
    REQUIRE(decodedException.get() == serviceException);
    rti1516_2025::HLAinteger32BE decodedSerial;
    REQUIRE_NOTHROW(decodedSerial.decode(report.parameterValues.at(reportParameters[6])));
    REQUIRE(decodedSerial.get() == static_cast<rti1516_2025::Integer32>(index));
  };

  auto const invalidObjectValue = ObjectInstanceHandle{}.toString();
  auto const validObjectValue = objectInstance.toString();
  REQUIRE_THROWS_AS(
      requester->localDeleteObjectInstance(ObjectInstanceHandle{}),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE(observerReports.interactionReports.size() == 1U);
  verifyReport(
      0U,
      invalidObjectValue,
      false,
      L"ObjectInstanceNotKnown: Local Delete Object Instance requires a known ObjectInstanceHandle.");
  REQUIRE_NOTHROW(requester->localDeleteObjectInstance(objectInstance));
  REQUIRE(observerReports.interactionReports.size() == 2U);
  verifyReport(1U, validObjectValue, true, L"");
  REQUIRE_THROWS_AS(
      requester->localDeleteObjectInstance(objectInstance),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE(observerReports.interactionReports.size() == 3U);
  verifyReport(
      2U,
      validObjectValue,
      false,
      L"ObjectInstanceNotKnown: The supplied ObjectInstanceHandle is not known to this federate.");

  REQUIRE_NOTHROW(requester->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(observer->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(requester->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
