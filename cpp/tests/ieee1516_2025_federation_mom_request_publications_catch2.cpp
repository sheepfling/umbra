#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded MOM requestPublications reports publication state and NULL responses",
    "[integration][development-profile][federation-management][mom][mom-request-report]"
    "[object-management][interaction-management][directed]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.publish-object-class-attributes][rti.service.unpublish-object-class-attributes]"
    "[rti.service.publish-interaction-class][rti.service.unpublish-interaction-class]"
    "[rti.service.publish-object-class-directed-interactions]"
    "[rti.service.unpublish-object-class-directed-interactions]"
    "[rti.service.subscribe-interaction-class][rti.service.send-interaction]"
    "[rti.service.evoke-callback]"
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
      L"mom-publications-subject", L"subject", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"mom-publications-observer", L"observer", federationName));

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
  auto const observerTakeOrder = observer->getInteractionClassHandle(
      fixture_hla::fom::server_take_order);
  REQUIRE(serverClass.isValid());
  REQUIRE(observerServerClass.isValid());
  REQUIRE(efficiency.isValid());
  REQUIRE(privilegeToDelete.isValid());
  REQUIRE(takeOrder.isValid());
  REQUIRE(observerTakeOrder.isValid());
  REQUIRE_NOTHROW(subject->publishObjectClassAttributes(
      serverClass, AttributeHandleSet{efficiency}));
  REQUIRE_NOTHROW(subject->publishInteractionClass(takeOrder));
  REQUIRE_NOTHROW(subject->publishObjectClassDirectedInteractions(
      serverClass, InteractionClassHandleSet{takeOrder}));

  auto const requestClass = subject->getInteractionClassHandle(
      standard_hla::mom::request_publications);
  auto const requestFederateParameter = subject->getParameterHandle(
      requestClass, standard_hla::mom::federate);
  auto const objectReportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_object_class_publication);
  auto const objectReportNumberOfClasses = observer->getParameterHandle(
      objectReportClass, standard_hla::mom::number_of_classes);
  auto const objectReportObjectClass = observer->getParameterHandle(
      objectReportClass, standard_hla::mom::object_class);
  auto const objectReportAttributeList = observer->getParameterHandle(
      objectReportClass, standard_hla::mom::attribute_list);
  auto const interactionReportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_interaction_publication);
  auto const interactionReportClassList = observer->getParameterHandle(
      interactionReportClass, standard_hla::mom::interaction_class_list);
  auto const directedReportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_directed_interaction_publication);
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
  observerReports.interactionReports.clear();

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

  auto const requestPublications = [&] {
    observerReports.interactionReports.clear();
    REQUIRE_NOTHROW(subject->sendInteraction(
        requestClass,
        ParameterHandleValueMap{{requestFederateParameter, subjectFederate.encode()}},
        VariableLengthData{}));
    REQUIRE(observerReports.interactionReports.empty());
    while (observer->evokeCallback(0.0)) {
    }
    REQUIRE(observerReports.interactionReports.size() == 3U);
  };

  requestPublications();
  bool sawObjectPublication = false;
  bool sawInteractionPublication = false;
  bool sawDirectedPublication = false;
  for (auto const& report : observerReports.interactionReports) {
    REQUIRE(report.transportationType == observer->getTransportationTypeHandle(
        standard_hla::mom::reliable));
    REQUIRE(report.userSuppliedTag.size() == 0U);
    REQUIRE_FALSE(report.producingFederate.isValid());
    REQUIRE_FALSE(report.sentRegionsSupplied);
    if (report.interactionClass == objectReportClass) {
      sawObjectPublication = true;
      REQUIRE(report.parameterValues.size() == 3U);
      rti1516_2025::HLAinteger32BE classCount;
      REQUIRE_NOTHROW(classCount.decode(
          report.parameterValues.at(objectReportNumberOfClasses)));
      REQUIRE(classCount.get() == 1);
      REQUIRE(variableLengthDataBytes(
                  report.parameterValues.at(objectReportObjectClass)) ==
              variableLengthDataBytes(observerServerClass.encode()));
      auto const attributes = decodeHandleList(
          report.parameterValues.at(objectReportAttributeList));
      REQUIRE(attributes.size() == 2U);
      REQUIRE(attributes.contains(variableLengthDataBytes(efficiency.encode())));
      REQUIRE(attributes.contains(variableLengthDataBytes(privilegeToDelete.encode())));
    } else if (report.interactionClass == interactionReportClass) {
      sawInteractionPublication = true;
      REQUIRE(report.parameterValues.size() == 1U);
      auto const interactions = decodeHandleList(
          report.parameterValues.at(interactionReportClassList));
      REQUIRE(interactions.size() == 1U);
      REQUIRE(interactions.contains(variableLengthDataBytes(observerTakeOrder.encode())));
    } else {
      REQUIRE(report.interactionClass == directedReportClass);
      sawDirectedPublication = true;
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
      REQUIRE(interactions.contains(variableLengthDataBytes(observerTakeOrder.encode())));
    }
  }
  REQUIRE(sawObjectPublication);
  REQUIRE(sawInteractionPublication);
  REQUIRE(sawDirectedPublication);

  REQUIRE_NOTHROW(subject->unpublishObjectClassDirectedInteractions(serverClass));
  REQUIRE_NOTHROW(subject->unpublishInteractionClass(takeOrder));
  REQUIRE_NOTHROW(subject->unpublishObjectClassAttributes(
      serverClass, AttributeHandleSet{efficiency}));
  requestPublications();
  sawObjectPublication = false;
  sawInteractionPublication = false;
  sawDirectedPublication = false;
  for (auto const& report : observerReports.interactionReports) {
    if (report.interactionClass == objectReportClass) {
      sawObjectPublication = true;
      REQUIRE(report.parameterValues.size() == 1U);
      rti1516_2025::HLAinteger32BE classCount;
      REQUIRE_NOTHROW(classCount.decode(
          report.parameterValues.at(objectReportNumberOfClasses)));
      REQUIRE(classCount.get() == 0);
      REQUIRE_FALSE(report.parameterValues.contains(objectReportObjectClass));
      REQUIRE_FALSE(report.parameterValues.contains(objectReportAttributeList));
    } else if (report.interactionClass == interactionReportClass) {
      sawInteractionPublication = true;
      REQUIRE(report.parameterValues.size() == 1U);
      REQUIRE(decodeHandleList(
                  report.parameterValues.at(interactionReportClassList))
                  .empty());
    } else {
      REQUIRE(report.interactionClass == directedReportClass);
      sawDirectedPublication = true;
      REQUIRE(report.parameterValues.size() == 2U);
      rti1516_2025::HLAinteger32BE classCount;
      REQUIRE_NOTHROW(classCount.decode(
          report.parameterValues.at(directedReportNumberOfClasses)));
      REQUIRE(classCount.get() == 0);
      REQUIRE_FALSE(report.parameterValues.contains(directedReportObjectClass));
      REQUIRE(decodeHandleList(
                  report.parameterValues.at(directedReportClassList))
                  .empty());
    }
  }
  REQUIRE(sawObjectPublication);
  REQUIRE(sawInteractionPublication);
  REQUIRE(sawDirectedPublication);

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
