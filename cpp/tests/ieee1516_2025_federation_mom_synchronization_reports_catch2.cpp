#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded MOM federation synchronization reports list and status",
    "[integration][development-profile][federation-management][mom][mom-request-report]"
    "[federation-management][interaction-management]"
    "[rti.service.register-federation-synchronization-point][rti.service.synchronization-point-achieved]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.send-interaction][rti.service.evoke-callback]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador subjectReports;
  ReportingFederateAmbassador observerReports;
  auto subject = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(subject->connect(subjectReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      subject->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle subjectFederate;
  FederateHandle observerFederate;
  REQUIRE_NOTHROW(subjectFederate = subject->joinFederationExecution(
      L"mom-sync-subject", L"subject", federationName));
  REQUIRE_NOTHROW(observerFederate = observer->joinFederationExecution(
      L"mom-sync-observer", L"observer", federationName));

  auto const synchronizationLabel = std::wstring{L"mom-sync-report"};
  REQUIRE_NOTHROW(subject->registerFederationSynchronizationPoint(
      synchronizationLabel,
      VariableLengthData{}));
  while (subject->evokeCallback(0.0)) {
  }
  while (observer->evokeCallback(0.0)) {
  }

  auto const pointsRequestClass = subject->getInteractionClassHandle(
      standard_hla::mom::request_synchronization_points);
  auto const pointsReportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_synchronization_points);
  auto const pointsReportParameter = observer->getParameterHandle(
      pointsReportClass, standard_hla::mom::sync_points);
  auto const statusRequestClass = subject->getInteractionClassHandle(
      standard_hla::mom::request_synchronization_point_status);
  auto const statusRequestParameter = subject->getParameterHandle(
      statusRequestClass, standard_hla::mom::sync_point_name);
  auto const statusReportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_synchronization_point_status);
  auto const statusReportNameParameter = observer->getParameterHandle(
      statusReportClass, standard_hla::mom::sync_point_name);
  auto const statusReportFederatesParameter = observer->getParameterHandle(
      statusReportClass, standard_hla::mom::sync_point_federates);
  auto const fomRequestClass = subject->getInteractionClassHandle(
      standard_hla::mom::request_fom_module_data_federation);
  auto const fomRequestParameter = subject->getParameterHandle(
      fomRequestClass, standard_hla::mom::fom_module_indicator);
  REQUIRE(pointsRequestClass.isValid());
  REQUIRE(pointsReportClass.isValid());
  REQUIRE(pointsReportParameter.isValid());
  REQUIRE(statusRequestClass.isValid());
  REQUIRE(statusRequestParameter.isValid());
  REQUIRE(statusReportClass.isValid());
  REQUIRE(statusReportNameParameter.isValid());
  REQUIRE(statusReportFederatesParameter.isValid());
  REQUIRE(fomRequestParameter.isValid());

  REQUIRE_NOTHROW(observer->subscribeInteractionClass(pointsReportClass));
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(statusReportClass));
  while (observer->evokeCallback(0.0)) {
  }

  auto const decodeSynchronizationPoints = [](VariableLengthData const& encoded) {
    rti1516_2025::HLAunicodeString labelPrototype;
    rti1516_2025::HLAvariableArray labels{
        static_cast<rti1516_2025::DataElement const&>(labelPrototype)};
    REQUIRE_NOTHROW(labels.decode(encoded));
    std::vector<std::wstring> decoded;
    decoded.reserve(labels.size());
    for (std::size_t index = 0U; index < labels.size(); ++index) {
      decoded.push_back(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
          labels.get(index)).get());
    }
    return decoded;
  };
  auto const decodeSynchronizationStatuses = [&](VariableLengthData const& encoded) {
    rti1516_2025::HLAvariableArrayT<rti1516_2025::HLAoctet> federatePrototype;
    rti1516_2025::HLAfixedRecord recordPrototype;
    recordPrototype.appendElement(federatePrototype)
        .appendElement(rti1516_2025::HLAinteger32BE{});
    rti1516_2025::HLAvariableArray statuses{recordPrototype};
    REQUIRE_NOTHROW(statuses.decode(encoded));
    std::map<FederateHandle, rti1516_2025::Integer32> decoded;
    for (std::size_t index = 0U; index < statuses.size(); ++index) {
      auto const& record = dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
          statuses.get(index));
      auto const& federateReference = dynamic_cast<rti1516_2025::HLAvariableArray const&>(
          record.get(0U));
      auto const& status = dynamic_cast<rti1516_2025::HLAinteger32BE const&>(
          record.get(1U));
      auto const federate = observer->decodeFederateHandle(federateReference.encode());
      decoded.emplace(federate, status.get());
    }
    return decoded;
  };

  REQUIRE_NOTHROW(subject->sendInteraction(
      pointsRequestClass,
      ParameterHandleValueMap{},
      VariableLengthData{}));
  REQUIRE(observerReports.interactionReports.empty());
  while (observer->evokeCallback(0.0)) {
  }
  REQUIRE(observerReports.interactionReports.size() == 1U);
  auto const& pointsReport = observerReports.interactionReports.back();
  REQUIRE(pointsReport.interactionClass == pointsReportClass);
  REQUIRE(pointsReport.parameterValues.size() == 1U);
  REQUIRE(pointsReport.userSuppliedTag.size() == 0U);
  REQUIRE(pointsReport.transportationType == observer->getTransportationTypeHandle(
      standard_hla::mom::reliable));
  REQUIRE_FALSE(pointsReport.producingFederate.isValid());
  REQUIRE_FALSE(pointsReport.sentRegionsSupplied);
  auto const pointLabels = decodeSynchronizationPoints(
      pointsReport.parameterValues.at(pointsReportParameter));
  REQUIRE(pointLabels == std::vector<std::wstring>{synchronizationLabel});

  auto requestStatus = [&](std::wstring const& label) {
    observerReports.interactionReports.clear();
    REQUIRE_NOTHROW(subject->sendInteraction(
        statusRequestClass,
        ParameterHandleValueMap{{
            statusRequestParameter,
            rti1516_2025::HLAunicodeString{label}.encode(),
        }},
        VariableLengthData{}));
    REQUIRE(observerReports.interactionReports.empty());
    while (observer->evokeCallback(0.0)) {
    }
    REQUIRE(observerReports.interactionReports.size() == 1U);
    auto const& report = observerReports.interactionReports.back();
    REQUIRE(report.interactionClass == statusReportClass);
    REQUIRE(report.parameterValues.size() == 2U);
    REQUIRE(report.userSuppliedTag.size() == 0U);
    REQUIRE(report.transportationType == observer->getTransportationTypeHandle(
        standard_hla::mom::reliable));
    REQUIRE_FALSE(report.producingFederate.isValid());
    REQUIRE_FALSE(report.sentRegionsSupplied);
    rti1516_2025::HLAunicodeString decodedName;
    REQUIRE_NOTHROW(decodedName.decode(
        report.parameterValues.at(statusReportNameParameter)));
    REQUIRE(decodedName.get() == label);
    return decodeSynchronizationStatuses(
        report.parameterValues.at(statusReportFederatesParameter));
  };

  auto const beforeAchievement = requestStatus(synchronizationLabel);
  REQUIRE(beforeAchievement.size() == 2U);
  REQUIRE(beforeAchievement.at(subjectFederate) == 2);
  REQUIRE(beforeAchievement.at(observerFederate) == 2);

  REQUIRE_NOTHROW(subject->synchronizationPointAchieved(synchronizationLabel));
  while (subject->evokeCallback(0.0)) {
  }
  while (observer->evokeCallback(0.0)) {
  }
  auto const afterSubjectAchievement = requestStatus(synchronizationLabel);
  REQUIRE(afterSubjectAchievement.at(subjectFederate) == 3);
  REQUIRE(afterSubjectAchievement.at(observerFederate) == 2);

  auto const missingStatus = requestStatus(L"missing-sync-report");
  REQUIRE(missingStatus.empty());

  // The report callback is subscriber-gated at delivery time, including an
  // already accepted request whose callback has not yet been evoked.
  observerReports.interactionReports.clear();
  REQUIRE_NOTHROW(subject->sendInteraction(
      pointsRequestClass,
      ParameterHandleValueMap{},
      VariableLengthData{}));
  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(pointsReportClass));
  while (observer->evokeCallback(0.0)) {
  }
  REQUIRE(observerReports.interactionReports.empty());
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(pointsReportClass));
  while (observer->evokeCallback(0.0)) {
  }

  REQUIRE_THROWS_AS(
      subject->sendInteraction(
          pointsRequestClass,
          ParameterHandleValueMap{{
              statusRequestParameter,
              rti1516_2025::HLAunicodeString{synchronizationLabel}.encode(),
          }},
          VariableLengthData{}),
      rti1516_2025::InteractionParameterNotDefined);
  REQUIRE_THROWS_AS(
      subject->sendInteraction(
          statusRequestClass,
          ParameterHandleValueMap{},
          VariableLengthData{}),
      rti1516_2025::InteractionParameterNotDefined);
  REQUIRE_THROWS_AS(
      subject->sendInteraction(
          statusRequestClass,
          ParameterHandleValueMap{{
              fomRequestParameter,
              rti1516_2025::HLAinteger32BE{0}.encode(),
          }},
          VariableLengthData{}),
      rti1516_2025::InteractionParameterNotDefined);

  REQUIRE_NOTHROW(observer->synchronizationPointAchieved(synchronizationLabel));
  while (subject->evokeCallback(0.0)) {
  }
  while (observer->evokeCallback(0.0)) {
  }
  observerReports.interactionReports.clear();
  REQUIRE_NOTHROW(subject->sendInteraction(
      pointsRequestClass,
      ParameterHandleValueMap{},
      VariableLengthData{}));
  while (observer->evokeCallback(0.0)) {
  }
  REQUIRE(observerReports.interactionReports.size() == 1U);
  auto const afterCompletion = decodeSynchronizationPoints(
      observerReports.interactionReports.back().parameterValues.at(pointsReportParameter));
  REQUIRE(afterCompletion.empty());

  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(pointsReportClass));
  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(statusReportClass));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subject->disconnect());
  REQUIRE_NOTHROW(observer->disconnect());
}
}
