#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAvariableArray.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The timestamped Delete Object Instance MOM failure test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::AttributeHandleSet;
using rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
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
  return L"timestamped-delete-mom-failure-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
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
        producingFederate,
    });
  }

  std::vector<InteractionReport> interactionReports;
};

}  // namespace

TEST_CASE(
    "Embedded service reporting delivers failed timestamped Delete Object Instance invocations through MOM interaction",
    "[integration][development-profile][federation-management][object-management][time-management]"
    "[tso][mom][service-reporting][service-report-interaction][service-failure]"
    "[timestamped-delete-failure][timestamped-delete-object-instance-failure-mom-interaction]"
    "[rti.service.delete-object-instance][rti.service.enable-time-regulation]"
    "[rti.service.set-service-reporting-switch][rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.subscribe-interaction-class][federate.callback.receive-interaction]") {
  rti1516_2025::NullFederateAmbassador publisherCallbacks;
  ReportingFederateAmbassador observerCallbacks;
  auto publisher = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
       "ieee1516.2-2025" / "resources" / "examples" /
       "RestaurantFOMmodule-2025.xml")
          .wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  unsigned char const tagBytes[] = {'t', 's', 'o'};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      std::vector<std::wstring>{restaurantFom, switchFom},
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"timestamped-delete-mom-failure-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(observer->connect(observerCallbacks, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"timestamped-delete-mom-failure-observer", L"observer", federationName));

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
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));

  auto const server = publisher->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const efficiency = publisher->getAttributeHandle(server, L"Efficiency");
  REQUIRE(server.isValid());
  REQUIRE(efficiency.isValid());
  AttributeHandleSet const attributes{efficiency};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(server, attributes));
  ObjectInstanceHandle const objectInstance = publisher->registerObjectInstance(server);

  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(true));
  REQUIRE_FALSE(publisher->getSendServiceReportsToFileSwitch());

  auto const verifyFailure = [&](std::size_t index,
                                 std::wstring const& objectValue,
                                 std::wstring const& timestampValue,
                                 std::wstring const& serviceException) {
    auto const& report = observerCallbacks.interactionReports.at(index);
    REQUIRE(report.interactionClass == reportClass);
    REQUIRE(report.parameterValues.size() == 7U);
    REQUIRE(report.userSuppliedTag.size() == 0U);
    REQUIRE_FALSE(report.producingFederate.isValid());

    rti1516_2025::HLAunicodeString service;
    REQUIRE_NOTHROW(service.decode(report.parameterValues.at(reportParameters[0])));
    REQUIRE(service.get() == L"DeleteObjectInstance");
    rti1516_2025::HLAinteger16BE serviceType;
    REQUIRE_NOTHROW(serviceType.decode(report.parameterValues.at(reportParameters[1])));
    REQUIRE(serviceType.get() == 2);
    rti1516_2025::HLAboolean success;
    REQUIRE_NOTHROW(success.decode(report.parameterValues.at(reportParameters[2])));
    REQUIRE_FALSE(success.get());

    rti1516_2025::HLAfixedRecord argumentPrototype;
    argumentPrototype.appendElement(rti1516_2025::HLAinteger32BE{})
        .appendElement(rti1516_2025::HLAunicodeString{})
        .appendElement(rti1516_2025::HLAunicodeString{});
    rti1516_2025::HLAvariableArray suppliedArguments{argumentPrototype};
    REQUIRE_NOTHROW(suppliedArguments.decode(report.parameterValues.at(reportParameters[3])));
    REQUIRE(suppliedArguments.size() == 3U);
    auto const verifyArgument = [&](std::size_t argumentIndex,
                                    std::int32_t type,
                                    std::wstring const& name,
                                    std::wstring const& value) {
      auto const& supplied = dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
          suppliedArguments.get(argumentIndex));
      REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(supplied.get(0U)).get() == type);
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(supplied.get(1U)).get() == name);
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(supplied.get(2U)).get() == value);
    };
    verifyArgument(
        0U,
        37,
        L"Object instance designator",
        L"\"" + objectValue + L"\"");
    verifyArgument(1U, 63, L"User-supplied tag", L"\"dHNv\"");
    verifyArgument(2U, 31, L"Optional timestamp", L"\"" + timestampValue + L"\"");

    rti1516_2025::HLAfixedRecord returned;
    returned.appendElement(rti1516_2025::HLAinteger32BE{})
        .appendElement(rti1516_2025::HLAunicodeString{})
        .appendElement(rti1516_2025::HLAunicodeString{});
    REQUIRE_NOTHROW(returned.decode(report.parameterValues.at(reportParameters[4])));
    REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(returned.get(0U)).get() == 34);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(returned.get(1U)).get().empty());
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(returned.get(2U)).get() ==
            L"null");
    rti1516_2025::HLAunicodeString exception;
    REQUIRE_NOTHROW(exception.decode(report.parameterValues.at(reportParameters[5])));
    REQUIRE(exception.get() == serviceException);
    rti1516_2025::HLAinteger32BE serial;
    REQUIRE_NOTHROW(serial.decode(report.parameterValues.at(reportParameters[6])));
    REQUIRE(serial.get() == static_cast<rti1516_2025::Integer32>(index));
  };

  auto const invalidObject = ObjectInstanceHandle{}.toString();
  REQUIRE_THROWS_AS(
      publisher->deleteObjectInstance(
          ObjectInstanceHandle{},
          tag,
          rti1516_2025::HLAinteger64Time(6)),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE(observerCallbacks.interactionReports.size() == 1U);
  verifyFailure(
      0U,
      invalidObject,
      L"6",
      L"ObjectInstanceNotKnown: Timestamped Delete Object Instance requires a known ObjectInstanceHandle.");

  auto const objectValue = objectInstance.toString();
  REQUIRE_THROWS_AS(
      publisher->deleteObjectInstance(
          objectInstance,
          tag,
          rti1516_2025::HLAinteger64Time(4)),
      rti1516_2025::InvalidLogicalTime);
  REQUIRE(observerCallbacks.interactionReports.size() == 2U);
  verifyFailure(
      1U,
      objectValue,
      L"4",
      L"InvalidLogicalTime: A timestamped service is earlier than the sender's current logical time plus lookahead.");

  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(observer->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
