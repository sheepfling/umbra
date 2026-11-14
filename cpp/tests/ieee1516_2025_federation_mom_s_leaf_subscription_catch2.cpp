#include <array>
#include <cstring>
#include <set>
#include <string>
#include <string_view>

#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

constexpr auto parameterRequiredTable7SLeaves = std::to_array<std::wstring_view>({
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAmodifyAttributeState",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestDirectedInteractionsReceived",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestDirectedInteractionsSent",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestFOMmoduleData",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestInteractionsReceived",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestInteractionsSent",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstanceInformation",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstancesReflected",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstancesThatCanBeDeleted",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstancesUpdated",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestPublications",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestReflectionsReceived",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestSubscriptions",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestUpdatesSent",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAchangeAttributeOrderType",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAchangeInteractionOrderType",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAdeleteObjectInstance",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAdisableAsynchronousDelivery",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAdisableTimeConstrained",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAdisableTimeRegulation",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAenableAsynchronousDelivery",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAenableTimeConstrained",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAenableTimeRegulation",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAfederateRestoreComplete",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAfederateSaveComplete",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAflushQueueRequest",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAlocalDeleteObjectInstance",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAmodifyLookahead",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAnextMessageRequest",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAnextMessageRequestAvailable",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLApublishInteractionClass",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLApublishObjectClassAttributes",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLApublishObjectClassDirectedInteractions",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLArequestAttributeTransportationTypeChange",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLArequestInteractionTransportationTypeChange",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAresignFederationExecution",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAsubscribeInteractionClass",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAsubscribeObjectClassAttributes",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAsubscribeObjectClassDirectedInteractions",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAsynchronizationPointAchieved",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAtimeAdvanceRequest",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAtimeAdvanceRequestAvailable",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAunconditionalAttributeOwnershipDivestiture",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAunpublishInteractionClass",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAunpublishObjectClassAttributes",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAunpublishObjectClassDirectedInteractions",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAunsubscribeInteractionClass",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAunsubscribeObjectClassAttributes",
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAunsubscribeObjectClassDirectedInteractions",
    L"HLAinteractionRoot.HLAmanager.HLAfederation.HLAadjust.HLAsetSwitches",
    L"HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestFOMmoduleData",
    L"HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestSynchronizationPointStatus",
});

constexpr std::wstring_view requestMimDataLeaf =
    L"HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestMIMdata";
constexpr std::wstring_view requestSynchronizationPointsLeaf =
    L"HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestSynchronizationPoints";
static_assert(parameterRequiredTable7SLeaves.size() == 54U);

void drainCallbacks(rti1516_2025::RTIambassador& ambassador) {
  while (ambassador.evokeCallback(0.0)) {
  }
}

TEST_CASE(
    "The RTI reports missing arguments for parameter-required Table 7 S leaves",
    "[integration][development-profile][2025][federation-management][mom]"
    "[mom-table7-s-leaf-subscription]"
    "[mom-no-metadata-callbacks]"
    "[requirement-candidate-content-clauses-11-management-object-model-page-290-l156-51]"
    "[requirement-candidate-content-clauses-11-management-object-model-page-290-l165-54-no-metadata]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.send-interaction]"
    "[rti.service.evoke-callback][rti.service.register-federation-synchronization-point]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador subjectReports;
  ReportingFederateAmbassador observerReports;
  auto subject = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(subject->connect(subjectReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(subject->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  auto const subjectHandle = subject->joinFederationExecution(
      L"mom-s-leaf-subject", L"subject", federationName);
  REQUIRE(subjectHandle.isValid());
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"mom-s-leaf-observer", L"observer", federationName));

  auto const exceptionReportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_mom_exception);
  auto const exceptionFederateParameter = observer->getParameterHandle(
      exceptionReportClass, standard_hla::mom::federate);
  auto const exceptionServiceParameter = observer->getParameterHandle(
      exceptionReportClass, standard_hla::mom::service);
  auto const exceptionTextParameter = observer->getParameterHandle(
      exceptionReportClass, standard_hla::mom::exception);
  auto const parameterErrorParameter = observer->getParameterHandle(
      exceptionReportClass, standard_hla::mom::parameter_error);
  auto const reliable = observer->getTransportationTypeHandle(standard_hla::mom::reliable);
  REQUIRE(exceptionReportClass.isValid());
  REQUIRE(exceptionFederateParameter.isValid());
  REQUIRE(exceptionServiceParameter.isValid());
  REQUIRE(exceptionTextParameter.isValid());
  REQUIRE(parameterErrorParameter.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(exceptionReportClass));

  std::set<std::wstring> expectedServices;
  for (auto const leafName : parameterRequiredTable7SLeaves) {
    auto const interactionName = std::wstring{leafName};
    auto const leaf = subject->getInteractionClassHandle(interactionName);
    REQUIRE(leaf.isValid());
    expectedServices.emplace(interactionName);
    INFO("Table 7 S leaf: " << std::string(interactionName.begin(), interactionName.end()));
    REQUIRE_NOTHROW(subject->publishInteractionClass(leaf));
    bool rejected = false;
    try {
      subject->sendInteraction(
          leaf,
          rti1516_2025::ParameterHandleValueMap{},
          rti1516_2025::VariableLengthData{});
    } catch (...) {
      rejected = true;
    }
    CHECK(rejected);
    REQUIRE_NOTHROW(subject->unpublishInteractionClass(leaf));
  }
  drainCallbacks(*subject);
  drainCallbacks(*observer);

  REQUIRE(expectedServices.size() == 54U);
  CHECK(observerReports.interactionReports.size() == expectedServices.size());
  REQUIRE(observerReports.timestampedInteractionReports.empty());
  std::set<std::wstring> reportedServices;
  auto const subjectEncoding = subjectHandle.encode();
  for (auto const& report : observerReports.interactionReports) {
    REQUIRE(report.interactionClass == exceptionReportClass);
    REQUIRE(report.parameterValues.size() == 4U);
    REQUIRE(report.parameterValues.contains(exceptionFederateParameter));
    REQUIRE(report.parameterValues.contains(exceptionServiceParameter));
    REQUIRE(report.parameterValues.contains(exceptionTextParameter));
    REQUIRE(report.parameterValues.contains(parameterErrorParameter));
    REQUIRE(report.userSuppliedTag.size() == 0U);
    REQUIRE(report.transportationType == reliable);
    REQUIRE_FALSE(report.producingFederate.isValid());
    REQUIRE_FALSE(report.sentRegionsSupplied);

    rti1516_2025::HLAunicodeString serviceName;
    REQUIRE_NOTHROW(serviceName.decode(
        report.parameterValues.at(exceptionServiceParameter)));
    auto const serviceWide = serviceName.get();
    INFO("MOM exception service: " <<
         std::string(serviceWide.begin(), serviceWide.end()));
    REQUIRE(expectedServices.contains(serviceWide));
    REQUIRE(reportedServices.insert(serviceWide).second);

    rti1516_2025::HLAunicodeString exceptionText;
    REQUIRE_NOTHROW(exceptionText.decode(
        report.parameterValues.at(exceptionTextParameter)));
    REQUIRE_FALSE(exceptionText.get().empty());
    rti1516_2025::HLAboolean parameterError;
    REQUIRE_NOTHROW(parameterError.decode(
        report.parameterValues.at(parameterErrorParameter)));
    CHECK(parameterError.get());

    auto const& reportedFederate = report.parameterValues.at(exceptionFederateParameter);
    REQUIRE(reportedFederate.size() == subjectEncoding.size());
    REQUIRE(std::memcmp(
                reportedFederate.data(), subjectEncoding.data(), subjectEncoding.size()) == 0);
  }
  CHECK(reportedServices == expectedServices);

  // These parameterless federation S leaves are sent validly and must produce
  // their corresponding reports. HLAfederateSaveBegun is another parameterless
  // S leaf, covered separately while a save is in progress.
  observerReports.interactionReports.clear();
  auto const mimReportClass = observer->getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportMIMdata");
  REQUIRE(mimReportClass.isValid());
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(mimReportClass));
  auto const mimRequest = subject->getInteractionClassHandle(
      std::wstring{requestMimDataLeaf});
  REQUIRE(mimRequest.isValid());
  REQUIRE_NOTHROW(subject->publishInteractionClass(mimRequest));
  REQUIRE_NOTHROW(subject->sendInteraction(
      mimRequest,
      rti1516_2025::ParameterHandleValueMap{},
      rti1516_2025::VariableLengthData{}));
  REQUIRE_NOTHROW(subject->unpublishInteractionClass(mimRequest));
  drainCallbacks(*subject);
  drainCallbacks(*observer);
  REQUIRE(observerReports.interactionReports.size() == 1U);
  REQUIRE(observerReports.interactionReports.front().interactionClass == mimReportClass);
  REQUIRE(observerReports.interactionReports.front().userSuppliedTag.size() == 0U);
  REQUIRE(observerReports.timestampedInteractionReports.empty());

  observerReports.interactionReports.clear();
  auto const synchronizationReportClass = observer->getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportSynchronizationPoints");
  REQUIRE(synchronizationReportClass.isValid());
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(synchronizationReportClass));
  REQUIRE_NOTHROW(subject->registerFederationSynchronizationPoint(
      L"table7-s-leaf-subscription", rti1516_2025::VariableLengthData{}));
  drainCallbacks(*subject);
  drainCallbacks(*observer);
  auto const synchronizationRequest =
      subject->getInteractionClassHandle(
          std::wstring{requestSynchronizationPointsLeaf});
  REQUIRE(synchronizationRequest.isValid());
  REQUIRE_NOTHROW(subject->publishInteractionClass(synchronizationRequest));
  REQUIRE_NOTHROW(subject->sendInteraction(
      synchronizationRequest,
      rti1516_2025::ParameterHandleValueMap{},
      rti1516_2025::VariableLengthData{}));
  REQUIRE_NOTHROW(subject->unpublishInteractionClass(synchronizationRequest));
  drainCallbacks(*subject);
  drainCallbacks(*observer);
  REQUIRE(observerReports.interactionReports.size() == 1U);
  REQUIRE(observerReports.interactionReports.front().interactionClass ==
          synchronizationReportClass);
  REQUIRE(observerReports.interactionReports.front().userSuppliedTag.size() == 0U);
  REQUIRE(observerReports.timestampedInteractionReports.empty());

  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subject->disconnect());
  REQUIRE_NOTHROW(observer->disconnect());
}

TEST_CASE(
    "Published HLAchangeAttributeOrderType with missing arguments reports a parameter MOM exception",
    "[integration][development-profile][2025][federation-management][mom]"
    "[mom-table7-s-leaf-subscription]"
    "[requirement-candidate-content-clauses-11-management-object-model-page-290-l156-51]"
    "[rti.service.get-interaction-class-handle][rti.service.publish-interaction-class]"
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
  REQUIRE_NOTHROW(subject->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  auto const subjectHandle = subject->joinFederationExecution(
      L"mom-change-attribute-order-subject", L"subject", federationName);
  REQUIRE(subjectHandle.isValid());
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"mom-change-attribute-order-observer", L"observer", federationName));

  auto const exceptionReportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_mom_exception);
  auto const exceptionFederateParameter = observer->getParameterHandle(
      exceptionReportClass, standard_hla::mom::federate);
  auto const exceptionServiceParameter = observer->getParameterHandle(
      exceptionReportClass, standard_hla::mom::service);
  auto const exceptionTextParameter = observer->getParameterHandle(
      exceptionReportClass, standard_hla::mom::exception);
  auto const parameterErrorParameter = observer->getParameterHandle(
      exceptionReportClass, standard_hla::mom::parameter_error);
  REQUIRE(exceptionReportClass.isValid());
  REQUIRE(exceptionFederateParameter.isValid());
  REQUIRE(exceptionServiceParameter.isValid());
  REQUIRE(exceptionTextParameter.isValid());
  REQUIRE(parameterErrorParameter.isValid());
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(exceptionReportClass));

  constexpr std::wstring_view changeAttributeOrderTypeName =
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAchangeAttributeOrderType";
  auto const interactionClass = subject->getInteractionClassHandle(
      std::wstring{changeAttributeOrderTypeName});
  REQUIRE(interactionClass.isValid());
  REQUIRE_NOTHROW(subject->publishInteractionClass(interactionClass));
  REQUIRE_THROWS_AS(
      subject->sendInteraction(
          interactionClass,
          rti1516_2025::ParameterHandleValueMap{},
          rti1516_2025::VariableLengthData{}),
      rti1516_2025::InteractionParameterNotDefined);

  drainCallbacks(*subject);
  drainCallbacks(*observer);
  REQUIRE(observerReports.interactionReports.size() == 1U);
  auto const& report = observerReports.interactionReports.front();
  REQUIRE(report.interactionClass == exceptionReportClass);
  REQUIRE(report.parameterValues.size() == 4U);
  REQUIRE(report.parameterValues.contains(exceptionFederateParameter));
  REQUIRE(report.parameterValues.contains(exceptionServiceParameter));
  REQUIRE(report.parameterValues.contains(exceptionTextParameter));
  REQUIRE(report.parameterValues.contains(parameterErrorParameter));
  REQUIRE(report.userSuppliedTag.size() == 0U);
  REQUIRE(observerReports.timestampedInteractionReports.empty());

  rti1516_2025::HLAunicodeString serviceName;
  REQUIRE_NOTHROW(serviceName.decode(
      report.parameterValues.at(exceptionServiceParameter)));
  REQUIRE(serviceName.get() == std::wstring{changeAttributeOrderTypeName});
  rti1516_2025::HLAunicodeString exceptionText;
  REQUIRE_NOTHROW(exceptionText.decode(
      report.parameterValues.at(exceptionTextParameter)));
  REQUIRE_FALSE(exceptionText.get().empty());
  rti1516_2025::HLAboolean parameterError;
  REQUIRE_NOTHROW(parameterError.decode(
      report.parameterValues.at(parameterErrorParameter)));
  REQUIRE(parameterError.get());

  auto const subjectEncoding = subjectHandle.encode();
  auto const& reportedFederate = report.parameterValues.at(exceptionFederateParameter);
  REQUIRE(reportedFederate.size() == subjectEncoding.size());
  REQUIRE(std::memcmp(
              reportedFederate.data(), subjectEncoding.data(), subjectEncoding.size()) == 0);

  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subject->disconnect());
  REQUIRE_NOTHROW(observer->disconnect());
}

TEST_CASE(
    "Published HLAchangeInteractionOrderType with missing arguments reports a parameter MOM exception",
    "[integration][development-profile][2025][federation-management][mom]"
    "[mom-table7-s-leaf-subscription]"
    "[requirement-candidate-content-clauses-11-management-object-model-page-290-l156-51]"
    "[rti.service.get-interaction-class-handle][rti.service.publish-interaction-class]"
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
  REQUIRE_NOTHROW(subject->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  auto const subjectHandle = subject->joinFederationExecution(
      L"mom-change-interaction-order-subject", L"subject", federationName);
  REQUIRE(subjectHandle.isValid());
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"mom-change-interaction-order-observer", L"observer", federationName));

  auto const exceptionReportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_mom_exception);
  auto const exceptionFederateParameter = observer->getParameterHandle(
      exceptionReportClass, standard_hla::mom::federate);
  auto const exceptionServiceParameter = observer->getParameterHandle(
      exceptionReportClass, standard_hla::mom::service);
  auto const exceptionTextParameter = observer->getParameterHandle(
      exceptionReportClass, standard_hla::mom::exception);
  auto const parameterErrorParameter = observer->getParameterHandle(
      exceptionReportClass, standard_hla::mom::parameter_error);
  REQUIRE(exceptionReportClass.isValid());
  REQUIRE(exceptionFederateParameter.isValid());
  REQUIRE(exceptionServiceParameter.isValid());
  REQUIRE(exceptionTextParameter.isValid());
  REQUIRE(parameterErrorParameter.isValid());
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(exceptionReportClass));

  constexpr std::wstring_view changeInteractionOrderTypeName =
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAchangeInteractionOrderType";
  auto const interactionClass = subject->getInteractionClassHandle(
      std::wstring{changeInteractionOrderTypeName});
  REQUIRE(interactionClass.isValid());
  REQUIRE_NOTHROW(subject->publishInteractionClass(interactionClass));
  REQUIRE_THROWS_AS(
      subject->sendInteraction(
          interactionClass,
          rti1516_2025::ParameterHandleValueMap{},
          rti1516_2025::VariableLengthData{}),
      rti1516_2025::InteractionParameterNotDefined);

  drainCallbacks(*subject);
  drainCallbacks(*observer);
  REQUIRE(observerReports.interactionReports.size() == 1U);
  auto const& report = observerReports.interactionReports.front();
  REQUIRE(report.interactionClass == exceptionReportClass);
  REQUIRE(report.parameterValues.size() == 4U);
  REQUIRE(report.parameterValues.contains(exceptionFederateParameter));
  REQUIRE(report.parameterValues.contains(exceptionServiceParameter));
  REQUIRE(report.parameterValues.contains(exceptionTextParameter));
  REQUIRE(report.parameterValues.contains(parameterErrorParameter));
  REQUIRE(report.userSuppliedTag.size() == 0U);
  REQUIRE(observerReports.timestampedInteractionReports.empty());

  rti1516_2025::HLAunicodeString serviceName;
  REQUIRE_NOTHROW(serviceName.decode(
      report.parameterValues.at(exceptionServiceParameter)));
  REQUIRE(serviceName.get() == std::wstring{changeInteractionOrderTypeName});
  rti1516_2025::HLAunicodeString exceptionText;
  REQUIRE_NOTHROW(exceptionText.decode(
      report.parameterValues.at(exceptionTextParameter)));
  REQUIRE_FALSE(exceptionText.get().empty());
  rti1516_2025::HLAboolean parameterError;
  REQUIRE_NOTHROW(parameterError.decode(
      report.parameterValues.at(parameterErrorParameter)));
  REQUIRE(parameterError.get());

  auto const subjectEncoding = subjectHandle.encode();
  auto const& reportedFederate = report.parameterValues.at(exceptionFederateParameter);
  REQUIRE(reportedFederate.size() == subjectEncoding.size());
  REQUIRE(std::memcmp(
              reportedFederate.data(), subjectEncoding.data(), subjectEncoding.size()) == 0);

  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subject->disconnect());
  REQUIRE_NOTHROW(observer->disconnect());
}

TEST_CASE(
    "Published HLAdeleteObjectInstance with missing arguments reports a parameter MOM exception",
    "[integration][development-profile][2025][federation-management][mom]"
    "[mom-table7-s-leaf-subscription]"
    "[requirement-candidate-content-clauses-11-management-object-model-page-290-l156-51]"
    "[rti.service.get-interaction-class-handle][rti.service.publish-interaction-class]"
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
  REQUIRE_NOTHROW(subject->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  auto const subjectHandle = subject->joinFederationExecution(
      L"mom-delete-object-instance-subject", L"subject", federationName);
  REQUIRE(subjectHandle.isValid());
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"mom-delete-object-instance-observer", L"observer", federationName));

  auto const exceptionReportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_mom_exception);
  auto const exceptionFederateParameter = observer->getParameterHandle(
      exceptionReportClass, standard_hla::mom::federate);
  auto const exceptionServiceParameter = observer->getParameterHandle(
      exceptionReportClass, standard_hla::mom::service);
  auto const exceptionTextParameter = observer->getParameterHandle(
      exceptionReportClass, standard_hla::mom::exception);
  auto const parameterErrorParameter = observer->getParameterHandle(
      exceptionReportClass, standard_hla::mom::parameter_error);
  REQUIRE(exceptionReportClass.isValid());
  REQUIRE(exceptionFederateParameter.isValid());
  REQUIRE(exceptionServiceParameter.isValid());
  REQUIRE(exceptionTextParameter.isValid());
  REQUIRE(parameterErrorParameter.isValid());
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(exceptionReportClass));

  constexpr std::wstring_view deleteObjectInstanceName =
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAdeleteObjectInstance";
  auto const interactionClass = subject->getInteractionClassHandle(
      std::wstring{deleteObjectInstanceName});
  REQUIRE(interactionClass.isValid());
  REQUIRE_NOTHROW(subject->publishInteractionClass(interactionClass));
  REQUIRE_THROWS_AS(
      subject->sendInteraction(
          interactionClass,
          rti1516_2025::ParameterHandleValueMap{},
          rti1516_2025::VariableLengthData{}),
      rti1516_2025::InteractionParameterNotDefined);

  drainCallbacks(*subject);
  drainCallbacks(*observer);
  REQUIRE(observerReports.interactionReports.size() == 1U);
  auto const& report = observerReports.interactionReports.front();
  REQUIRE(report.interactionClass == exceptionReportClass);
  REQUIRE(report.parameterValues.size() == 4U);
  REQUIRE(report.parameterValues.contains(exceptionFederateParameter));
  REQUIRE(report.parameterValues.contains(exceptionServiceParameter));
  REQUIRE(report.parameterValues.contains(exceptionTextParameter));
  REQUIRE(report.parameterValues.contains(parameterErrorParameter));
  REQUIRE(report.userSuppliedTag.size() == 0U);
  REQUIRE(observerReports.timestampedInteractionReports.empty());

  rti1516_2025::HLAunicodeString serviceName;
  REQUIRE_NOTHROW(serviceName.decode(
      report.parameterValues.at(exceptionServiceParameter)));
  REQUIRE(serviceName.get() == std::wstring{deleteObjectInstanceName});
  rti1516_2025::HLAunicodeString exceptionText;
  REQUIRE_NOTHROW(exceptionText.decode(
      report.parameterValues.at(exceptionTextParameter)));
  REQUIRE_FALSE(exceptionText.get().empty());
  rti1516_2025::HLAboolean parameterError;
  REQUIRE_NOTHROW(parameterError.decode(
      report.parameterValues.at(parameterErrorParameter)));
  REQUIRE(parameterError.get());

  auto const subjectEncoding = subjectHandle.encode();
  auto const& reportedFederate = report.parameterValues.at(exceptionFederateParameter);
  REQUIRE(reportedFederate.size() == subjectEncoding.size());
  REQUIRE(std::memcmp(
              reportedFederate.data(), subjectEncoding.data(), subjectEncoding.size()) == 0);

  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subject->disconnect());
  REQUIRE_NOTHROW(observer->disconnect());
}

}  // namespace
