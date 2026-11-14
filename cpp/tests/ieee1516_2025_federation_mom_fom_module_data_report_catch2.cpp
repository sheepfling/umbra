#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
class RegionInspectingMomReportAmbassador final
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
    "Embedded MOM requestFOMmoduleData reports retained joined module content",
    "[integration][development-profile][federation-management][mom][mom-request-report]"
    "[mom-ddm-service-prohibition]"
    "[requirement-candidate-content-clauses-11-management-object-model-page-291-l77-25-ddm]"
    "[mom-table7-federate-fom-module-data]"
    "[federation-management][interaction-management]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.send-interaction][rti.service.evoke-callback]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador subjectReports;
  RegionInspectingMomReportAmbassador observerReports;
  auto subject = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(subject->connect(subjectReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      subject->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle subjectFederate;
  REQUIRE_NOTHROW(subjectFederate = subject->joinFederationExecution(
      L"mom-fom-module-subject",
      L"subject",
      federationName,
      std::vector<std::wstring>{fomModule}));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"mom-fom-module-observer", L"observer", federationName));
  observerReports.regionQueryAmbassador = observer.get();

  auto const requestClass = subject->getInteractionClassHandle(
      standard_hla::mom::request_fom_module_data_federate);
  auto const requestFederateParameter = subject->getParameterHandle(
      requestClass, standard_hla::mom::federate);
  auto const requestIndicatorParameter = subject->getParameterHandle(
      requestClass, standard_hla::mom::fom_module_indicator);
  auto const reportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_fom_module_data_federate);
  auto const reportIndicatorParameter = observer->getParameterHandle(
      reportClass, standard_hla::mom::fom_module_indicator);
  auto const reportDataParameter = observer->getParameterHandle(
      reportClass, standard_hla::mom::fom_module_data);
  REQUIRE(requestClass.isValid());
  REQUIRE(requestFederateParameter.isValid());
  REQUIRE(requestIndicatorParameter.isValid());
  REQUIRE(reportClass.isValid());
  REQUIRE(reportIndicatorParameter.isValid());
  REQUIRE(reportDataParameter.isValid());

  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(observer->setConveyRegionDesignatorSetsSwitch(true));
  auto const normalizedSubject = observer->normalizeFederateHandle(subjectFederate);
  while (observer->evokeCallback(0.0)) {
  }
  observerReports.interactionReports.clear();

  REQUIRE_NOTHROW(subject->sendInteraction(
      requestClass,
      ParameterHandleValueMap{
          {requestFederateParameter, subjectFederate.encode()},
          {requestIndicatorParameter, rti1516_2025::HLAinteger32BE{0}.encode()},
      },
      VariableLengthData{}));
  // The report is receive-order traffic and must remain behind HLA_EVOKED.
  REQUIRE(observerReports.interactionReports.empty());
  while (observer->evokeCallback(0.0)) {
  }
  REQUIRE(observerReports.interactionReports.size() == 1U);
  auto const& report = observerReports.interactionReports.front();
  REQUIRE(report.interactionClass == reportClass);
  REQUIRE(report.parameterValues.size() == 2U);
  REQUIRE(report.userSuppliedTag.size() == 0U);
  REQUIRE(report.transportationType == observer->getTransportationTypeHandle(
      standard_hla::mom::reliable));
  REQUIRE_FALSE(report.producingFederate.isValid());
  REQUIRE(report.sentRegionsSupplied);
  REQUIRE(report.sentRegions.size() == 1U);
  REQUIRE_FALSE(report.regionInspectionFailed);
  CHECK((report.sentRegionDimensionNames == std::set<std::wstring>{L"HLAfederate"}));
  REQUIRE(report.sentRegionBounds.size() == 1U);
  CHECK((report.sentRegionBounds.at(L"HLAfederate") ==
         std::pair<unsigned long, unsigned long>{
             normalizedSubject, normalizedSubject + 1UL}));

  rti1516_2025::HLAinteger32BE decodedIndicator;
  REQUIRE_NOTHROW(decodedIndicator.decode(
      report.parameterValues.at(reportIndicatorParameter)));
  REQUIRE(decodedIndicator.get() == 0);
  rti1516_2025::HLAunicodeString decodedModule;
  REQUIRE_NOTHROW(decodedModule.decode(
      report.parameterValues.at(reportDataParameter)));
  REQUIRE(decodedModule.get().find(L"<objectModel") != std::wstring::npos);
  REQUIRE(decodedModule.get().find(L"Restaurant FOM Module") != std::wstring::npos);

  // An unavailable index is rejected synchronously and does not enqueue a
  // stale report from the prior successful request.
  observerReports.interactionReports.clear();
  REQUIRE_THROWS_AS(
      subject->sendInteraction(
          requestClass,
          ParameterHandleValueMap{
              {requestFederateParameter, subjectFederate.encode()},
              {requestIndicatorParameter, rti1516_2025::HLAinteger32BE{99}.encode()},
          },
          VariableLengthData{}),
      rti1516_2025::RTIinternalError);
  while (observer->evokeCallback(0.0)) {
  }
  REQUIRE(observerReports.interactionReports.empty());

  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subject->disconnect());
  REQUIRE_NOTHROW(observer->disconnect());
}
} // namespace
