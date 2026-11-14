#include <cstring>
#include <map>
#include <set>
#include <string>
#include <utility>

#include "internal/handles/transportation_type_handle.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
class RegionInspectingReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct InteractionReport final {
    rti1516_2025::InteractionClassHandle interactionClass;
    rti1516_2025::ParameterHandleValueMap parameterValues;
    rti1516_2025::VariableLengthData userSuppliedTag;
    rti1516_2025::TransportationTypeHandle transportationType;
    rti1516_2025::FederateHandle producingFederate;
    bool sentRegionsSupplied = false;
    rti1516_2025::RegionHandleSet sentRegions;
    bool regionInspectionFailed = false;
    std::set<std::wstring> sentRegionDimensionNames;
    std::map<std::wstring, std::pair<unsigned long, unsigned long>> sentRegionBounds;
  };

  void receiveInteraction(
      rti1516_2025::InteractionClassHandle const& interactionClass,
      rti1516_2025::ParameterHandleValueMap const& parameterValues,
      rti1516_2025::VariableLengthData const& userSuppliedTag,
      rti1516_2025::TransportationTypeHandle const& transportationType,
      rti1516_2025::FederateHandle const& producingFederate,
      rti1516_2025::RegionHandleSet const* optionalSentRegions) override {
    InteractionReport report{
        interactionClass,
        parameterValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        optionalSentRegions != nullptr,
    };
    if (optionalSentRegions != nullptr) {
      report.sentRegions = *optionalSentRegions;
      if (regionQueryAmbassador == nullptr) {
        report.regionInspectionFailed = true;
      } else {
        try {
          for (auto const& region : *optionalSentRegions) {
            auto const dimensions = regionQueryAmbassador->getDimensionHandleSet(region);
            for (auto const& dimension : dimensions) {
              auto const name = regionQueryAmbassador->getDimensionName(dimension);
              auto const bounds = regionQueryAmbassador->getRangeBounds(region, dimension);
              report.sentRegionDimensionNames.insert(name);
              report.sentRegionBounds.emplace(
                  name,
                  std::pair{bounds.getLowerBound(), bounds.getUpperBound()});
            }
          }
        } catch (...) {
          report.regionInspectionFailed = true;
        }
      }
    }
    interactionReports.push_back(std::move(report));
  }

  rti1516_2025::RTIambassador* regionQueryAmbassador = nullptr;
  std::vector<InteractionReport> interactionReports;
};

TEST_CASE(
    "Public API reports Get Object Class Handle exceptions through HLAreportException",
    "[integration][development-profile][federation-management][mom]"
    "[mom-table7-hlareport-exception-public-api]"
    "[mom-other-report-single-federate-region]"
    "[mom-ddm-service-prohibition]"
    "[requirement-candidate-content-clauses-11-management-object-model-page-291-l77-25-ddm]"
    "[requirement-candidate-content-clauses-11-management-object-model-page-291-l47-15]"
    "[requirement-candidate-content-clauses-11-management-object-model-page-291-l53-17]"
    "[requirement-candidate-content-clauses-11-management-object-model-page-291-l56-18]"
    "[rti.service.get-object-class-handle]"
    "[rti.service.get-exception-reporting-switch]"
    "[rti.service.set-exception-reporting-switch]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.get-interaction-class-handle]"
    "[rti.service.get-parameter-handle]"
    "[rti.service.subscribe-interaction-class]"
    "[rti.service.evoke-callback]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador subjectReports;
  RegionInspectingReportingFederateAmbassador observerReports;
  std::uint64_t nextFomHandleValue = 0x1000U;
  auto const registry =
      std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
          nullptr,
          nullptr,
          [&nextFomHandleValue] {
            return std::optional<std::uint64_t>{nextFomHandleValue++};
          });
  ScopedEmbeddedFederationRegistry registryScope(registry);
  auto subject = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(subject->connect(subjectReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(subject->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  auto const subjectHandle = subject->joinFederationExecution(
      L"exception-report-subject", L"subject", federationName);
  REQUIRE(subjectHandle.isValid());
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"exception-report-observer", L"observer", federationName));
  observerReports.regionQueryAmbassador = observer.get();

  auto const reportClass = observer->getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportException");
  auto const federateParameter = observer->getParameterHandle(reportClass, L"HLAfederate");
  auto const serviceParameter = observer->getParameterHandle(reportClass, L"HLAservice");
  auto const exceptionParameter = observer->getParameterHandle(reportClass, L"HLAexception");
  auto const reliable = observer->getTransportationTypeHandle(L"HLAreliable");
  auto const reliableValue =
      rti1516_2025::umbra_binding_detail::transportationTypeHandleValue(reliable);
  REQUIRE(reportClass.isValid());
  REQUIRE(federateParameter.isValid());
  REQUIRE(serviceParameter.isValid());
  REQUIRE(exceptionParameter.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE(reliableValue.has_value());
  REQUIRE(*reliableValue != 1U);
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(observer->setConveyRegionDesignatorSetsSwitch(true));
  auto const normalizedSubject = observer->normalizeFederateHandle(subjectHandle);

  auto drainObserver = [&] {
    while (observer->evokeCallback(0.0)) {
    }
  };
  auto failLookup = [&] {
    std::wstring description;
    try {
      static_cast<void>(subject->getObjectClassHandle(
          L"HLAobjectRoot.UmbraMissingExceptionProbe"));
    } catch (rti1516_2025::NameNotFound const& exception) {
      description = exception.name();
      if (!exception.what().empty()) {
        description += L": " + exception.what();
      }
    }
    REQUIRE_FALSE(description.empty());
    return description;
  };

  REQUIRE_FALSE(subject->getExceptionReportingSwitch());
  static_cast<void>(failLookup());
  drainObserver();
  REQUIRE(observerReports.interactionReports.empty());

  REQUIRE_NOTHROW(subject->setExceptionReportingSwitch(true));
  REQUIRE(subject->getExceptionReportingSwitch());
  auto const expectedException = failLookup();
  drainObserver();
  REQUIRE(observerReports.interactionReports.size() == 1U);
  auto const& report = observerReports.interactionReports.front();
  REQUIRE(report.interactionClass == reportClass);
  REQUIRE(report.parameterValues.size() == 3U);
  REQUIRE(report.parameterValues.contains(federateParameter));
  REQUIRE(report.parameterValues.contains(serviceParameter));
  REQUIRE(report.parameterValues.contains(exceptionParameter));
  REQUIRE(report.userSuppliedTag.size() == 0U);
  REQUIRE(report.transportationType == reliable);
  REQUIRE_FALSE(report.producingFederate.isValid());
  REQUIRE(report.sentRegionsSupplied);
  REQUIRE(report.sentRegions.size() == 1U);
  REQUIRE_FALSE(report.regionInspectionFailed);
  CHECK((report.sentRegionDimensionNames == std::set<std::wstring>{L"HLAfederate"}));
  REQUIRE(report.sentRegionBounds.size() == 1U);
  CHECK((report.sentRegionBounds.at(L"HLAfederate") ==
         std::pair<unsigned long, unsigned long>{
             normalizedSubject, normalizedSubject + 1UL}));

  rti1516_2025::HLAunicodeString decodedService;
  REQUIRE_NOTHROW(decodedService.decode(report.parameterValues.at(serviceParameter)));
  REQUIRE(decodedService.get() == L"Get Object Class Handle");
  rti1516_2025::HLAunicodeString decodedException;
  REQUIRE_NOTHROW(decodedException.decode(report.parameterValues.at(exceptionParameter)));
  REQUIRE(decodedException.get() == expectedException);
  auto const handleEncoding = subjectHandle.encode();
  auto const& reportedHandle = report.parameterValues.at(federateParameter);
  REQUIRE(reportedHandle.size() == handleEncoding.size());
  REQUIRE(std::memcmp(
              reportedHandle.data(), handleEncoding.data(), handleEncoding.size()) == 0);

  REQUIRE_NOTHROW(subject->setExceptionReportingSwitch(false));
  REQUIRE_FALSE(subject->getExceptionReportingSwitch());
  static_cast<void>(failLookup());
  drainObserver();
  REQUIRE(observerReports.interactionReports.size() == 1U);

  REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subject->disconnect());
  REQUIRE_NOTHROW(observer->disconnect());
}

TEST_CASE(
    "Embedded MOM rejects malformed interactions through HLAreportMOMexception",
    "[integration][development-profile][federation-management][mom][mom-request-report][mom-exception]"
    "[mom-ddm-service-prohibition]"
    "[requirement-candidate-content-clauses-11-management-object-model-page-291-l77-25-ddm]"
    "[interaction-management][rti.service.get-interaction-class-handle]"
    "[rti.service.get-parameter-handle][rti.service.subscribe-interaction-class]"
    "[rti.service.unsubscribe-interaction-class][rti.service.send-interaction]"
    "[rti.service.evoke-callback][federate.callback.receive-interaction]") {
  ReportingFederateAmbassador subjectReports;
  RegionInspectingReportingFederateAmbassador observerReports;
  std::uint64_t nextFomHandleValue = 0x1000U;
  auto const registry =
      std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
          nullptr,
          nullptr,
          [&nextFomHandleValue] {
            return std::optional<std::uint64_t>{nextFomHandleValue++};
          });
  ScopedEmbeddedFederationRegistry registryScope(registry);
  auto subject = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(subject->connect(subjectReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      subject->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  auto const subjectHandle = subject->joinFederationExecution(
      L"mom-exception-subject", L"subject", federationName);
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"mom-exception-observer", L"observer", federationName));
  observerReports.regionQueryAmbassador = observer.get();

  auto const reportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_mom_exception);
  auto const federateParameter = observer->getParameterHandle(
      reportClass, standard_hla::mom::federate);
  auto const serviceParameter = observer->getParameterHandle(reportClass, standard_hla::mom::service);
  auto const exceptionParameter = observer->getParameterHandle(reportClass, standard_hla::mom::exception);
  auto const parameterErrorParameter = observer->getParameterHandle(
      reportClass, standard_hla::mom::parameter_error);
  auto const reliableTransportation = observer->getTransportationTypeHandle(
      standard_hla::mom::reliable);
  auto const setSwitchesClass = subject->getInteractionClassHandle(
      standard_hla::mom::set_switches_federate);
  auto const serviceReportingParameter = subject->getParameterHandle(
      setSwitchesClass, standard_hla::mom::service_reporting);
  auto const objectInstancesUpdatedRequestClass = subject->getInteractionClassHandle(
      standard_hla::mom::request_object_instances_updated);
  auto const serviceInvocationClass = subject->getInteractionClassHandle(
      standard_hla::mom::report_service_invocation);
  REQUIRE(reportClass.isValid());
  REQUIRE(federateParameter.isValid());
  REQUIRE(serviceParameter.isValid());
  REQUIRE(exceptionParameter.isValid());
  REQUIRE(parameterErrorParameter.isValid());
  REQUIRE(reliableTransportation.isValid());
  auto const reliableValue =
      rti1516_2025::umbra_binding_detail::transportationTypeHandleValue(
          reliableTransportation);
  REQUIRE(reliableValue.has_value());
  REQUIRE(*reliableValue != 1U);
  REQUIRE(setSwitchesClass.isValid());
  REQUIRE(serviceReportingParameter.isValid());
  REQUIRE(objectInstancesUpdatedRequestClass.isValid());
  REQUIRE(serviceInvocationClass.isValid());

  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));
  while (observer->evokeCallback(0.0)) {
  }
  observerReports.interactionReports.clear();

  // HLAsetSwitches is a MOM interaction with an explicit at-least-one-
  // parameter rule. The malformed request still reaches the caller as the
  // standard C++ exception, while the RTI emits HLAreportMOMexception with
  // HLAfederate, HLAservice, HLAexception, and HLAparameterError.
  REQUIRE_THROWS_AS(
      subject->sendInteraction(
          setSwitchesClass,
          ParameterHandleValueMap{},
          VariableLengthData{}),
      rti1516_2025::InteractionParameterNotDefined);
  REQUIRE(observerReports.interactionReports.empty());
  while (observer->evokeCallback(0.0)) {
  }
  REQUIRE(observerReports.interactionReports.size() == 1U);
  auto const& report = observerReports.interactionReports.front();
  REQUIRE(report.interactionClass == reportClass);
  REQUIRE(report.parameterValues.size() == 4U);
  REQUIRE(report.parameterValues.contains(federateParameter));
  REQUIRE(report.userSuppliedTag.size() == 0U);
  REQUIRE(report.transportationType == reliableTransportation);
  REQUIRE_FALSE(report.producingFederate.isValid());
  REQUIRE_FALSE(report.sentRegionsSupplied);

  rti1516_2025::HLAunicodeString decodedService;
  REQUIRE_NOTHROW(decodedService.decode(report.parameterValues.at(serviceParameter)));
  REQUIRE(decodedService.get() ==
          standard_hla::mom::set_switches_federate);
  rti1516_2025::HLAunicodeString decodedException;
  REQUIRE_NOTHROW(decodedException.decode(report.parameterValues.at(exceptionParameter)));
  REQUIRE(decodedException.get().find(L"InteractionParameterNotDefined") !=
          std::wstring::npos);
  rti1516_2025::HLAboolean decodedParameterError;
  REQUIRE_NOTHROW(
      decodedParameterError.decode(report.parameterValues.at(parameterErrorParameter)));
  REQUIRE(decodedParameterError.get());

  // HLArequestObjectInstancesUpdated is also a Subscribe-only MOM
  // interaction. Supplying a parameter that belongs to HLAsetSwitches is
  // globally well-formed but not defined for this request class; the caller
  // receives the typed exception and eligible subscribers receive the same
  // distinct HLAreportMOMexception projection.
  REQUIRE_NOTHROW(observer->setConveyRegionDesignatorSetsSwitch(true));
  auto const normalizedSubject = observer->normalizeFederateHandle(subjectHandle);
  observerReports.interactionReports.clear();
  REQUIRE_THROWS_AS(
      subject->sendInteraction(
          objectInstancesUpdatedRequestClass,
          ParameterHandleValueMap{{
              serviceReportingParameter,
              rti1516_2025::HLAboolean{true}.encode(),
          }},
          VariableLengthData{}),
      rti1516_2025::InteractionParameterNotDefined);
  while (observer->evokeCallback(0.0)) {
  }
  REQUIRE(observerReports.interactionReports.size() == 1U);
  auto const& malformedObjectRequestReport = observerReports.interactionReports.front();
  REQUIRE(malformedObjectRequestReport.transportationType == reliableTransportation);
  REQUIRE(malformedObjectRequestReport.sentRegionsSupplied);
  REQUIRE(malformedObjectRequestReport.sentRegions.size() == 1U);
  REQUIRE_FALSE(malformedObjectRequestReport.regionInspectionFailed);
  CHECK((malformedObjectRequestReport.sentRegionDimensionNames ==
         std::set<std::wstring>{L"HLAfederate"}));
  REQUIRE(malformedObjectRequestReport.sentRegionBounds.size() == 1U);
  CHECK((malformedObjectRequestReport.sentRegionBounds.at(L"HLAfederate") ==
         std::pair<unsigned long, unsigned long>{
             normalizedSubject, normalizedSubject + 1UL}));
  rti1516_2025::HLAunicodeString malformedObjectRequestService;
  REQUIRE_NOTHROW(malformedObjectRequestService.decode(
      malformedObjectRequestReport.parameterValues.at(serviceParameter)));
  REQUIRE(malformedObjectRequestService.get() ==
          standard_hla::mom::request_object_instances_updated);
  rti1516_2025::HLAunicodeString malformedObjectRequestException;
  REQUIRE_NOTHROW(malformedObjectRequestException.decode(
      malformedObjectRequestReport.parameterValues.at(exceptionParameter)));
  REQUIRE(malformedObjectRequestException.get().find(
              L"InteractionParameterNotDefined") != std::wstring::npos);
  rti1516_2025::HLAboolean malformedObjectRequestParameterError;
  REQUIRE_NOTHROW(malformedObjectRequestParameterError.decode(
      malformedObjectRequestReport.parameterValues.at(parameterErrorParameter)));
  REQUIRE(malformedObjectRequestParameterError.get());

  // A well-formed MOM adjustment can still fail a service precondition. In
  // that case HLAparameterError is false while the caller retains the RTI
  // exception and the observer receives the same distinct MOM report class.
  REQUIRE_NOTHROW(subject->subscribeInteractionClass(serviceInvocationClass));
  observerReports.interactionReports.clear();
  REQUIRE_THROWS_AS(
      subject->sendInteraction(
          setSwitchesClass,
          ParameterHandleValueMap{{
              serviceReportingParameter,
              rti1516_2025::HLAboolean{true}.encode(),
          }},
          VariableLengthData{}),
      rti1516_2025::RTIinternalError);
  while (observer->evokeCallback(0.0)) {
  }
  REQUIRE(observerReports.interactionReports.size() == 1U);
  auto const& preconditionReport = observerReports.interactionReports.front();
  REQUIRE(preconditionReport.transportationType == reliableTransportation);
  rti1516_2025::HLAunicodeString preconditionException;
  REQUIRE_NOTHROW(preconditionException.decode(
      preconditionReport.parameterValues.at(exceptionParameter)));
  REQUIRE(preconditionException.get().find(L"RTIinternalError") !=
          std::wstring::npos);
  rti1516_2025::HLAboolean preconditionParameterError;
  REQUIRE_NOTHROW(preconditionParameterError.decode(
      preconditionReport.parameterValues.at(parameterErrorParameter)));
  REQUIRE_FALSE(preconditionParameterError.get());
  REQUIRE_NOTHROW(subject->unsubscribeInteractionClass(serviceInvocationClass));

  // Timestamped MOM invocations must use the same HLAreportMOMexception
  // projection as receive-order invocations.  The timestamp is valid for the
  // subject's integer64 time state, while HLAsetSwitches remains a
  // Subscribe-only MOM interaction and therefore fails the normal publication
  // precondition during timestamped planning.
  REQUIRE_NOTHROW(subject->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  while (subject->evokeCallback(0.0)) {
  }
  observerReports.interactionReports.clear();
  REQUIRE_THROWS_AS(
      subject->sendInteraction(
          setSwitchesClass,
          ParameterHandleValueMap{},
          VariableLengthData{},
          rti1516_2025::HLAinteger64Time(1)),
      rti1516_2025::InteractionClassNotPublished);
  REQUIRE(observerReports.interactionReports.empty());
  while (observer->evokeCallback(0.0)) {
  }
  REQUIRE(observerReports.interactionReports.size() == 1U);
  auto const& timestampedMomExceptionReport = observerReports.interactionReports.front();
  REQUIRE(timestampedMomExceptionReport.interactionClass == reportClass);
  REQUIRE(timestampedMomExceptionReport.transportationType == reliableTransportation);
  rti1516_2025::HLAunicodeString timestampedMomService;
  REQUIRE_NOTHROW(timestampedMomService.decode(
      timestampedMomExceptionReport.parameterValues.at(serviceParameter)));
  REQUIRE(timestampedMomService.get() == standard_hla::mom::set_switches_federate);
  rti1516_2025::HLAunicodeString timestampedMomException;
  REQUIRE_NOTHROW(timestampedMomException.decode(
      timestampedMomExceptionReport.parameterValues.at(exceptionParameter)));
  REQUIRE(timestampedMomException.get().find(L"InteractionClassNotPublished") !=
          std::wstring::npos);
  rti1516_2025::HLAboolean timestampedMomParameterError;
  REQUIRE_NOTHROW(timestampedMomParameterError.decode(
      timestampedMomExceptionReport.parameterValues.at(parameterErrorParameter)));
  REQUIRE_FALSE(timestampedMomParameterError.get());

  // The queued report is subject to callback-time subscription revalidation;
  // withdrawing the report subscription before Evoke suppresses delivery.
  observerReports.interactionReports.clear();
  REQUIRE_THROWS_AS(
      subject->sendInteraction(
          setSwitchesClass,
          ParameterHandleValueMap{},
          VariableLengthData{}),
      rti1516_2025::InteractionParameterNotDefined);
  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  while (observer->evokeCallback(0.0)) {
  }
  REQUIRE(observerReports.interactionReports.empty());

  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subject->disconnect());
  REQUIRE_NOTHROW(observer->disconnect());
}

TEST_CASE(
    "Embedded standard MOM interaction missing required parameter reports HLAreportMOMexception",
    "[integration][development-profile][2025][federation-management][mom]"
    "[joined-federate-mom-required-parameter-exception]"
    "[requirement-candidate-content-clauses-11-management-object-model-page-291-l101-33]"
    "[rti.service.get-interaction-class-handle]"
    "[rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class]"
    "[rti.service.send-interaction]"
    "[rti.service.evoke-callback]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador subjectReports;
  RegionInspectingReportingFederateAmbassador observerReports;
  std::uint64_t nextFomHandleValue = 0x1000U;
  auto const registry =
      std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
          nullptr,
          nullptr,
          [&nextFomHandleValue] {
            return std::optional<std::uint64_t>{nextFomHandleValue++};
          });
  ScopedEmbeddedFederationRegistry registryScope(registry);
  auto subject = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(subject->connect(subjectReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(subject->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  auto const subjectHandle = subject->joinFederationExecution(
      L"mom-required-parameter-subject", L"subject", federationName);
  REQUIRE(subjectHandle.isValid());
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"mom-required-parameter-observer", L"observer", federationName));

  auto const reportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_mom_exception);
  auto const federateParameter = observer->getParameterHandle(
      reportClass, standard_hla::mom::federate);
  auto const serviceParameter = observer->getParameterHandle(
      reportClass, standard_hla::mom::service);
  auto const exceptionParameter = observer->getParameterHandle(
      reportClass, standard_hla::mom::exception);
  auto const parameterErrorParameter = observer->getParameterHandle(
      reportClass, standard_hla::mom::parameter_error);
  auto const reliableTransportation = observer->getTransportationTypeHandle(
      standard_hla::mom::reliable);
  auto const requestClass = subject->getInteractionClassHandle(
      standard_hla::mom::request_synchronization_point_status);
  auto const statusReportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_synchronization_point_status);
  REQUIRE(reportClass.isValid());
  REQUIRE(federateParameter.isValid());
  REQUIRE(serviceParameter.isValid());
  REQUIRE(exceptionParameter.isValid());
  REQUIRE(parameterErrorParameter.isValid());
  REQUIRE(reliableTransportation.isValid());
  REQUIRE(requestClass.isValid());
  REQUIRE(statusReportClass.isValid());

  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(statusReportClass));
  while (observer->evokeCallback(0.0)) {
  }
  observerReports.interactionReports.clear();

  // HLArequestSynchronizationPointStatus requires HLAsyncPointName. An empty
  // parameter map must be rejected and reported, not treated as a status query.
  REQUIRE_THROWS_AS(
      subject->sendInteraction(
          requestClass,
          ParameterHandleValueMap{},
          VariableLengthData{}),
      rti1516_2025::InteractionParameterNotDefined);
  REQUIRE(observerReports.interactionReports.empty());
  while (observer->evokeCallback(0.0)) {
  }

  REQUIRE(observerReports.interactionReports.size() == 1U);
  auto const& report = observerReports.interactionReports.front();
  REQUIRE(report.interactionClass == reportClass);
  REQUIRE(report.parameterValues.size() == 4U);
  REQUIRE(report.parameterValues.contains(federateParameter));
  REQUIRE(report.parameterValues.contains(serviceParameter));
  REQUIRE(report.parameterValues.contains(exceptionParameter));
  REQUIRE(report.parameterValues.contains(parameterErrorParameter));
  REQUIRE(report.transportationType == reliableTransportation);
  CHECK(report.userSuppliedTag.size() == 0U);
  CHECK_FALSE(report.producingFederate.isValid());

  rti1516_2025::HLAunicodeString decodedService;
  REQUIRE_NOTHROW(decodedService.decode(report.parameterValues.at(serviceParameter)));
  CHECK(decodedService.get() == standard_hla::mom::request_synchronization_point_status);
  rti1516_2025::HLAunicodeString decodedException;
  REQUIRE_NOTHROW(decodedException.decode(report.parameterValues.at(exceptionParameter)));
  CHECK_FALSE(decodedException.get().empty());
  rti1516_2025::HLAboolean decodedParameterError;
  REQUIRE_NOTHROW(
      decodedParameterError.decode(report.parameterValues.at(parameterErrorParameter)));
  CHECK(decodedParameterError.get());
  auto const handleEncoding = subjectHandle.encode();
  auto const& reportedHandle = report.parameterValues.at(federateParameter);
  REQUIRE(reportedHandle.size() == handleEncoding.size());
  CHECK(std::memcmp(
            reportedHandle.data(), handleEncoding.data(), handleEncoding.size()) == 0);

  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(statusReportClass));
  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subject->disconnect());
  REQUIRE_NOTHROW(observer->disconnect());
}
}
