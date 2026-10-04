#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded MOM requestSubscriptions reports active passive and directed subscription state",
    "[integration][development-profile][federation-management][mom][mom-request-report]"
    "[object-management][interaction-management][directed][interaction-subscription-mode-exclusivity]"
    "[rti.service.get-federate-handle][rti.service.get-object-class-handle]"
    "[rti.service.get-attribute-handle][rti.service.get-interaction-class-handle]"
    "[rti.service.get-parameter-handle]"
    "[rti.service.subscribe-object-class-attributes][rti.service.unsubscribe-object-class-attributes]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.subscribe-object-class-directed-interactions]"
    "[rti.service.unsubscribe-object-class-directed-interactions]"
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
  REQUIRE_NOTHROW(subjectFederate = subject->joinFederationExecution(
      L"mom-subscriptions-subject", L"subject", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"mom-subscriptions-observer", L"observer", federationName));

  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(subject->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(observer->setServiceReportingSwitch(false));

  auto const serverClass = subject->getObjectClassHandle(
      fixture_hla::fom::employee_server);
  auto const observerServerClass = observer->getObjectClassHandle(
      fixture_hla::fom::employee_server);
  auto const efficiency = subject->getAttributeHandle(serverClass, fixture_hla::fixture::efficiency);
  auto const privilegeToDelete = subject->getAttributeHandle(
      serverClass, standard_hla::mom::privilege_to_delete_object);
  auto const takeOrder = subject->getInteractionClassHandle(
      fixture_hla::fom::server_take_order);
  auto const customerSeated = subject->getInteractionClassHandle(
      fixture_hla::fom::customer_seated);
  REQUIRE(serverClass.isValid());
  REQUIRE(observerServerClass.isValid());
  REQUIRE(efficiency.isValid());
  REQUIRE(privilegeToDelete.isValid());
  REQUIRE(takeOrder.isValid());
  REQUIRE(customerSeated.isValid());

  // The object-class report groups by (class, active/passive), and reports the
  // highest subscribed update rate in each group. The C++ active selector is
  // already represented as the MIM HLAactive value in the registry ledger.
  REQUIRE_NOTHROW(subject->subscribeObjectClassAttributes(
      serverClass, AttributeHandleSet{efficiency}, true, L"Low"));
  REQUIRE_NOTHROW(subject->subscribeObjectClassAttributes(
      serverClass, AttributeHandleSet{privilegeToDelete}, false, L"High"));
  // Switching a class between active and passive replaces its mode; the MOM
  // report below must contain exactly one (class, mode) entry for each class.
  REQUIRE_NOTHROW(subject->subscribeInteractionClass(takeOrder, true));
  REQUIRE_NOTHROW(subject->subscribeInteractionClass(takeOrder, false));
  REQUIRE_NOTHROW(subject->subscribeInteractionClass(customerSeated, false));
  REQUIRE_NOTHROW(subject->subscribeInteractionClass(customerSeated, true));
  REQUIRE_NOTHROW(subject->subscribeObjectClassDirectedInteractions(
      serverClass, InteractionClassHandleSet{takeOrder}, true));

  auto const requestClass = subject->getInteractionClassHandle(
      standard_hla::mom::request_subscriptions);
  auto const requestFederateParameter = subject->getParameterHandle(
      requestClass, standard_hla::mom::federate);
  auto const objectReportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_object_class_subscription);
  auto const objectReportNumberOfClasses = observer->getParameterHandle(
      objectReportClass, standard_hla::mom::number_of_classes);
  auto const objectReportObjectClass = observer->getParameterHandle(
      objectReportClass, standard_hla::mom::object_class);
  auto const objectReportActive = observer->getParameterHandle(
      objectReportClass, standard_hla::mom::active);
  auto const objectReportMaxUpdateRate = observer->getParameterHandle(
      objectReportClass, standard_hla::mom::max_update_rate);
  auto const objectReportAttributeList = observer->getParameterHandle(
      objectReportClass, standard_hla::mom::attribute_list);
  auto const interactionReportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_interaction_subscription);
  auto const interactionReportClassList = observer->getParameterHandle(
      interactionReportClass, standard_hla::mom::interaction_class_list);
  auto const directedReportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_directed_interaction_subscription);
  auto const directedReportNumberOfClasses = observer->getParameterHandle(
      directedReportClass, standard_hla::mom::number_of_classes);
  auto const directedReportObjectClass = observer->getParameterHandle(
      directedReportClass, standard_hla::mom::object_class);
  auto const directedReportClassList = observer->getParameterHandle(
      directedReportClass, standard_hla::mom::interaction_class_list);
  REQUIRE(requestClass.isValid());
  REQUIRE(requestFederateParameter.isValid());
  REQUIRE(objectReportClass.isValid());
  REQUIRE(objectReportNumberOfClasses.isValid());
  REQUIRE(objectReportObjectClass.isValid());
  REQUIRE(objectReportActive.isValid());
  REQUIRE(objectReportMaxUpdateRate.isValid());
  REQUIRE(objectReportAttributeList.isValid());
  REQUIRE(interactionReportClass.isValid());
  REQUIRE(interactionReportClassList.isValid());
  REQUIRE(directedReportClass.isValid());
  REQUIRE(directedReportNumberOfClasses.isValid());
  REQUIRE(directedReportObjectClass.isValid());
  REQUIRE(directedReportClassList.isValid());
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(objectReportClass));
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(interactionReportClass));
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(directedReportClass));
  while (observer->evokeCallback(0.0)) {
  }

  auto const decodeHandleList = [](VariableLengthData const& encoded) {
    rti1516_2025::HLAvariableArrayT<rti1516_2025::HLAoctet> handlePrototype;
    rti1516_2025::HLAvariableArray handles{
        static_cast<rti1516_2025::DataElement const&>(handlePrototype)};
    REQUIRE_NOTHROW(handles.decode(encoded));
    std::set<std::vector<unsigned char>> values;
    for (std::size_t index = 0U; index < handles.size(); ++index) {
      values.insert(variableLengthDataBytes(handles.get(index).encode()));
    }
    return values;
  };
  auto const decodeInteractionSubscriptions = [](VariableLengthData const& encoded) {
    rti1516_2025::HLAvariableArrayT<rti1516_2025::HLAoctet> interactionClassPrototype;
    rti1516_2025::HLAfixedRecord recordPrototype;
    recordPrototype.appendElement(interactionClassPrototype)
        .appendElement(rti1516_2025::HLAboolean{});
    rti1516_2025::HLAvariableArray subscriptions{recordPrototype};
    REQUIRE_NOTHROW(subscriptions.decode(encoded));
    std::vector<std::pair<std::vector<unsigned char>, bool>> values;
    for (std::size_t index = 0U; index < subscriptions.size(); ++index) {
      auto const& record = dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
          subscriptions.get(index));
      auto const& interactionClass = dynamic_cast<rti1516_2025::HLAvariableArray const&>(
          record.get(0U));
      auto const& active = dynamic_cast<rti1516_2025::HLAboolean const&>(record.get(1U));
      values.emplace_back(variableLengthDataBytes(interactionClass.encode()), active.get());
    }
    return values;
  };

  auto const requestSubscriptions = [&](std::size_t expectedCount) {
    observerReports.interactionReports.clear();
    REQUIRE_NOTHROW(subject->sendInteraction(
        requestClass,
        ParameterHandleValueMap{{requestFederateParameter, subjectFederate.encode()}},
        VariableLengthData{}));
    REQUIRE(observerReports.interactionReports.empty());
    while (observer->evokeCallback(0.0)) {
    }
    REQUIRE(observerReports.interactionReports.size() == expectedCount);
  };

  requestSubscriptions(4U);
  bool sawActiveObjectSubscription = false;
  bool sawPassiveObjectSubscription = false;
  bool sawInteractionSubscription = false;
  bool sawDirectedSubscription = false;
  for (auto const& report : observerReports.interactionReports) {
    REQUIRE(report.transportationType == observer->getTransportationTypeHandle(
        standard_hla::mom::reliable));
    REQUIRE(report.userSuppliedTag.size() == 0U);
    REQUIRE_FALSE(report.producingFederate.isValid());
    REQUIRE_FALSE(report.sentRegionsSupplied);
    if (report.interactionClass == objectReportClass) {
      REQUIRE(report.parameterValues.size() == 5U);
      rti1516_2025::HLAinteger32BE classCount;
      REQUIRE_NOTHROW(classCount.decode(
          report.parameterValues.at(objectReportNumberOfClasses)));
      REQUIRE(classCount.get() == 2);
      rti1516_2025::HLAboolean active;
      REQUIRE_NOTHROW(active.decode(report.parameterValues.at(objectReportActive)));
      rti1516_2025::HLAunicodeString maxRate;
      REQUIRE_NOTHROW(maxRate.decode(
          report.parameterValues.at(objectReportMaxUpdateRate)));
      auto const attributes = decodeHandleList(
          report.parameterValues.at(objectReportAttributeList));
      REQUIRE(variableLengthDataBytes(
                  report.parameterValues.at(objectReportObjectClass)) ==
              variableLengthDataBytes(observerServerClass.encode()));
      if (active.get()) {
        sawActiveObjectSubscription = true;
        REQUIRE(maxRate.get() == L"Low");
        REQUIRE(attributes.size() == 1U);
        REQUIRE(attributes.contains(variableLengthDataBytes(
            observer->getAttributeHandle(observerServerClass, fixture_hla::fixture::efficiency).encode())));
      } else {
        sawPassiveObjectSubscription = true;
        REQUIRE(maxRate.get() == L"High");
        REQUIRE(attributes.size() == 1U);
        REQUIRE(attributes.contains(variableLengthDataBytes(
            observer->getAttributeHandle(
                observerServerClass, standard_hla::mom::privilege_to_delete_object).encode())));
      }
    } else if (report.interactionClass == interactionReportClass) {
      sawInteractionSubscription = true;
      REQUIRE(report.parameterValues.size() == 1U);
      auto const subscriptions = decodeInteractionSubscriptions(
          report.parameterValues.at(interactionReportClassList));
      REQUIRE(subscriptions.size() == 2U);
      REQUIRE(std::find(subscriptions.begin(), subscriptions.end(),
                        std::make_pair(variableLengthDataBytes(observer->getInteractionClassHandle(
                            fixture_hla::fom::server_take_order).encode()), false)) !=
              subscriptions.end());
      REQUIRE(std::find(subscriptions.begin(), subscriptions.end(),
                        std::make_pair(variableLengthDataBytes(observer->getInteractionClassHandle(
                            fixture_hla::fom::customer_seated).encode()), true)) !=
              subscriptions.end());
    } else {
      REQUIRE(report.interactionClass == directedReportClass);
      sawDirectedSubscription = true;
      REQUIRE(report.parameterValues.size() == 3U);
      rti1516_2025::HLAinteger32BE classCount;
      REQUIRE_NOTHROW(classCount.decode(
          report.parameterValues.at(directedReportNumberOfClasses)));
      REQUIRE(classCount.get() == 1);
      REQUIRE(variableLengthDataBytes(
                  report.parameterValues.at(directedReportObjectClass)) ==
              variableLengthDataBytes(observerServerClass.encode()));
      auto const interactions = decodeHandleList(
          report.parameterValues.at(directedReportClassList));
      REQUIRE(interactions.size() == 1U);
      REQUIRE(interactions.contains(variableLengthDataBytes(
          observer->getInteractionClassHandle(
              fixture_hla::fom::server_take_order).encode())));
    }
  }
  REQUIRE(sawActiveObjectSubscription);
  REQUIRE(sawPassiveObjectSubscription);
  REQUIRE(sawInteractionSubscription);
  REQUIRE(sawDirectedSubscription);

  REQUIRE_NOTHROW(subject->unsubscribeObjectClassAttributes(
      serverClass, AttributeHandleSet{efficiency, privilegeToDelete}));
  REQUIRE_NOTHROW(subject->unsubscribeInteractionClass(takeOrder));
  REQUIRE_NOTHROW(subject->unsubscribeInteractionClass(customerSeated));
  REQUIRE_NOTHROW(subject->unsubscribeObjectClassDirectedInteractions(serverClass));
  requestSubscriptions(3U);
  sawActiveObjectSubscription = false;
  sawPassiveObjectSubscription = false;
  sawInteractionSubscription = false;
  sawDirectedSubscription = false;
  for (auto const& report : observerReports.interactionReports) {
    if (report.interactionClass == objectReportClass) {
      sawActiveObjectSubscription = true;
      REQUIRE(report.parameterValues.size() == 1U);
      rti1516_2025::HLAinteger32BE classCount;
      REQUIRE_NOTHROW(classCount.decode(
          report.parameterValues.at(objectReportNumberOfClasses)));
      REQUIRE(classCount.get() == 0);
      REQUIRE_FALSE(report.parameterValues.contains(objectReportObjectClass));
      REQUIRE_FALSE(report.parameterValues.contains(objectReportActive));
      REQUIRE_FALSE(report.parameterValues.contains(objectReportMaxUpdateRate));
      REQUIRE_FALSE(report.parameterValues.contains(objectReportAttributeList));
    } else if (report.interactionClass == interactionReportClass) {
      sawInteractionSubscription = true;
      REQUIRE(report.parameterValues.size() == 1U);
      REQUIRE(decodeInteractionSubscriptions(
                  report.parameterValues.at(interactionReportClassList)).empty());
    } else {
      REQUIRE(report.interactionClass == directedReportClass);
      sawDirectedSubscription = true;
      REQUIRE(report.parameterValues.size() == 2U);
      rti1516_2025::HLAinteger32BE classCount;
      REQUIRE_NOTHROW(classCount.decode(
          report.parameterValues.at(directedReportNumberOfClasses)));
      REQUIRE(classCount.get() == 0);
      REQUIRE_FALSE(report.parameterValues.contains(directedReportObjectClass));
      REQUIRE(decodeHandleList(
                  report.parameterValues.at(directedReportClassList)).empty());
    }
  }
  REQUIRE(sawActiveObjectSubscription);
  REQUIRE(sawInteractionSubscription);
  REQUIRE(sawDirectedSubscription);

  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(objectReportClass));
  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(interactionReportClass));
  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(directedReportClass));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subject->disconnect());
  REQUIRE_NOTHROW(observer->disconnect());
}
}
