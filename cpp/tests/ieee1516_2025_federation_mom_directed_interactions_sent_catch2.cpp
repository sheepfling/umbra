#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded MOM requestDirectedInteractionsSent reports directed class and transportation counts",
    "[integration][development-profile][federation-management][mom][mom-request-report]"
    "[interaction-management][directed][transportation-management]"
    "[mom-directed-interactions-sent-report]"
    "[mom-directed-interactions-sent-report]"
    "[rti.service.get-interaction-class-handle][rti.service.get-object-class-handle]"
    "[rti.service.get-attribute-handle][rti.service.get-parameter-handle]"
    "[rti.service.publish-interaction-class][rti.service.publish-object-class-attributes]"
    "[rti.service.publish-object-class-directed-interactions]"
    "[rti.service.subscribe-interaction-class][rti.service.subscribe-object-class-attributes]"
    "[rti.service.subscribe-object-class-directed-interactions]"
    "[rti.service.register-object-instance][rti.service.send-directed-interaction]"
    "[rti.service.send-interaction][rti.service.evoke-callback]"
    "[federate.callback.receive-directed-interaction][federate.callback.receive-interaction]") {
  ReportingFederateAmbassador subjectReports;
  ReportingFederateAmbassador observerReports;
  auto subject = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(subject->connect(subjectReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      subject->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  FederateHandle subjectFederate;
  REQUIRE_NOTHROW(subjectFederate = subject->joinFederationExecution(
      L"mom-directed-interactions-sent-subject", L"subject", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"mom-directed-interactions-sent-observer", L"observer", federationName));

  // Keep the report lane focused: the service-report interaction is not part
  // of this assertion, while the directed interaction itself remains enabled.
  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(subject->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(observer->setServiceReportingSwitch(false));

  auto const takeOrder = subject->getInteractionClassHandle(
      L"HLAinteractionRoot.ServerAction.TakeOrder");
  auto const observerTakeOrder = observer->getInteractionClassHandle(
      L"HLAinteractionRoot.ServerAction.TakeOrder");
  auto const serverClass = subject->getObjectClassHandle(
      L"HLAobjectRoot.Employee.Server");
  auto const observerServerClass = observer->getObjectClassHandle(
      L"HLAobjectRoot.Employee.Server");
  auto const efficiency = subject->getAttributeHandle(serverClass, L"Efficiency");
  auto const observerEfficiency = observer->getAttributeHandle(
      observerServerClass, L"Efficiency");
  REQUIRE(takeOrder.isValid());
  REQUIRE(observerTakeOrder.isValid());
  REQUIRE(serverClass.isValid());
  REQUIRE(observerServerClass.isValid());
  REQUIRE(efficiency.isValid());
  REQUIRE(observerEfficiency.isValid());
  REQUIRE_NOTHROW(subject->publishInteractionClass(takeOrder));
  REQUIRE_NOTHROW(subject->publishObjectClassAttributes(
      serverClass, AttributeHandleSet{efficiency}));
  REQUIRE_NOTHROW(observer->subscribeObjectClassAttributes(
      observerServerClass, AttributeHandleSet{observerEfficiency}, true));
  REQUIRE_NOTHROW(subject->publishObjectClassDirectedInteractions(
      serverClass, InteractionClassHandleSet{takeOrder}));
  REQUIRE_NOTHROW(observer->subscribeObjectClassDirectedInteractions(
      observerServerClass, InteractionClassHandleSet{observerTakeOrder}, true));
  ObjectInstanceHandle target;
  REQUIRE_NOTHROW(target = subject->registerObjectInstance(serverClass));
  REQUIRE(target.isValid());
  while (observer->evokeCallback(0.0)) {
  }

  // Both accepted directed sends belong to one reliable transportation bucket
  // and one sent interaction class. The callback ledger is drained first so
  // the later report assertion cannot confuse application delivery with MOM.
  REQUIRE_NOTHROW(subject->sendDirectedInteraction(
      takeOrder, target, ParameterHandleValueMap{}, VariableLengthData{}));
  REQUIRE_NOTHROW(subject->sendDirectedInteraction(
      takeOrder, target, ParameterHandleValueMap{}, VariableLengthData{}));
  while (observer->evokeCallback(0.0)) {
  }
  REQUIRE(observerReports.directedInteractionReports.size() == 2U);
  observerReports.interactionReports.clear();
  observerReports.directedInteractionReports.clear();

  auto const requestClass = subject->getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestDirectedInteractionsSent");
  auto const requestFederateParameter = subject->getParameterHandle(
      requestClass, L"HLAfederate");
  auto const reportClass = observer->getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportDirectedInteractionsSent");
  auto const reportTransportationParameter = observer->getParameterHandle(
      reportClass, L"HLAtransportation");
  auto const reportCountsParameter = observer->getParameterHandle(
      reportClass, L"HLAinteractionCounts");
  REQUIRE(requestClass.isValid());
  REQUIRE(requestFederateParameter.isValid());
  REQUIRE(reportClass.isValid());
  REQUIRE(reportTransportationParameter.isValid());
  REQUIRE(reportCountsParameter.isValid());
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));
  while (observer->evokeCallback(0.0)) {
  }
  observerReports.interactionReports.clear();

  REQUIRE_NOTHROW(subject->sendInteraction(
      requestClass,
      ParameterHandleValueMap{{requestFederateParameter, subjectFederate.encode()}},
      VariableLengthData{}));
  REQUIRE(observerReports.interactionReports.empty());
  while (observer->evokeCallback(0.0)) {
  }
  REQUIRE(observerReports.interactionReports.size() == 2U);

  auto const reliableBytes = variableLengthDataBytes(
      observer->getTransportationTypeHandle(L"HLAreliable").encode());
  auto const bestEffortBytes = variableLengthDataBytes(
      observer->getTransportationTypeHandle(L"HLAbestEffort").encode());
  auto const takeOrderBytes = variableLengthDataBytes(observerTakeOrder.encode());
  std::map<std::vector<unsigned char>, std::size_t> decodedCounts;
  for (auto const& report : observerReports.interactionReports) {
    REQUIRE(report.interactionClass == reportClass);
    REQUIRE(report.parameterValues.size() == 2U);
    REQUIRE(report.parameterValues.contains(reportTransportationParameter));
    REQUIRE(report.parameterValues.contains(reportCountsParameter));
    REQUIRE(report.userSuppliedTag.size() == 0U);
    REQUIRE(report.transportationType == observer->getTransportationTypeHandle(L"HLAreliable"));
    REQUIRE_FALSE(report.producingFederate.isValid());
    REQUIRE_FALSE(report.sentRegionsSupplied);

    rti1516_2025::HLAvariableArrayT<rti1516_2025::HLAoctet> interactionClassPrototype;
    rti1516_2025::HLAfixedRecord recordPrototype;
    recordPrototype.appendElement(interactionClassPrototype)
        .appendElement(rti1516_2025::HLAinteger32BE{});
    rti1516_2025::HLAvariableArray counts{recordPrototype};
    REQUIRE_NOTHROW(counts.decode(report.parameterValues.at(reportCountsParameter)));
    auto const transportationBytes = variableLengthDataBytes(
        report.parameterValues.at(reportTransportationParameter));
    REQUIRE((transportationBytes == reliableBytes || transportationBytes == bestEffortBytes));
    decodedCounts[transportationBytes] = counts.size();
    if (transportationBytes == reliableBytes) {
      REQUIRE(counts.size() == 1U);
      auto const& record = dynamic_cast<rti1516_2025::HLAfixedRecord const&>(counts.get(0U));
      auto const& count = dynamic_cast<rti1516_2025::HLAinteger32BE const&>(record.get(1U));
      REQUIRE(variableLengthDataBytes(record.get(0U).encode()) == takeOrderBytes);
      REQUIRE(count.get() == 2);
    } else {
      REQUIRE(counts.size() == 0U);
    }
  }
  REQUIRE(decodedCounts.size() == 2U);
  REQUIRE(decodedCounts.at(reliableBytes) == 1U);
  REQUIRE(decodedCounts.at(bestEffortBytes) == 0U);

  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subject->disconnect());
  REQUIRE_NOTHROW(observer->disconnect());
}
} // namespace
