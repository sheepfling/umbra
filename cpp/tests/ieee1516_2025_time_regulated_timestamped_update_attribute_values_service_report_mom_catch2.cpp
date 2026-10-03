#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting delivers time-regulated timestamped Update Attribute Values through MOM interaction",
    "[integration][development-profile][federation-management][object-management]"
    "[time-management][tso][mom][service-reporting][service-report-interaction]"
    "[timestamped-attribute-update-time-regulated-service-report-interaction]"
    "[rti.service.timestamped-attribute-update-time-regulated-service-report-interaction]"
    "[rti.service.update-attribute-values][rti.service.change-default-attribute-order-type]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[federate.callback.reflect-attribute-values][federate.callback.receive-interaction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  ReportingFederateAmbassador observerReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const updateFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "attribute-update-passel-fom.xml")
          .wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{updateFom, switchFom};
  unsigned char const tagBytes[] = {'t', 's', 'o'};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  unsigned char const valueBytes[] = {0x01, 0x02};
  VariableLengthData const value(valueBytes, sizeof(valueBytes));
  rti1516_2025::HLAinteger64Time const timestamp(6);

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModules,
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-update-time-regulated-mom-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-update-time-regulated-mom-receiver", L"receiver", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"timestamped-update-time-regulated-mom-observer", L"observer", federationName));

  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(receiver->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(receiver->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(observer->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(publisher->setAttributeRelevanceAdvisorySwitch(true));

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
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));

  auto const objectClass = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const attribute = publisher->getAttributeHandle(objectClass, L"ReliableBaseA");
  auto const reliableTransportation = publisher->getTransportationTypeHandle(L"HLAreliable");
  REQUIRE(objectClass.isValid());
  REQUIRE(attribute.isValid());
  REQUIRE(reliableTransportation.isValid());
  AttributeHandleSet const attributes{attribute};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(objectClass, attributes, TIMESTAMP));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(objectClass));
  REQUIRE_FALSE(receiver->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
  // Discovery queues the initial attribute-relevance edge for the owner.
  // Observe its actual advisory before expecting the regulation callback;
  // with the switch disabled this work is suppressed only when dispatched.
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE(publisherReports.turnUpdatesOnForObjectInstanceReports.size() == 1U);
  REQUIRE(publisherReports.turnUpdatesOnForObjectInstanceReports.front().objectInstance ==
          objectInstance);
  REQUIRE(publisherReports.turnUpdatesOnForObjectInstanceReports.front().attributes ==
          attributes);
  REQUIRE(publisherReports.timeRegulationEnabledReports.empty());
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1U);

  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(true));
  REQUIRE_FALSE(publisher->getSendServiceReportsToFileSwitch());
  AttributeHandleValueMap const values{{attribute, value}};
  auto const retraction = publisher->updateAttributeValues(
      objectInstance,
      values,
      tag,
      timestamp);
  REQUIRE(retraction.isValid());
  REQUIRE(observerReports.interactionReports.size() == 1U);
  REQUIRE(receiverReports.attributeReflectionReports.empty());
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());

  auto const& report = observerReports.interactionReports.front();
  REQUIRE(report.interactionClass == reportClass);
  REQUIRE(report.parameterValues.size() == 8U);
  REQUIRE(report.userSuppliedTag.size() == 0U);
  REQUIRE(report.transportationType ==
          observer->getTransportationTypeHandle(standard_hla::mom::reliable));
  REQUIRE_FALSE(report.producingFederate.isValid());
  REQUIRE_FALSE(report.sentRegionsSupplied);

  rti1516_2025::HLAunicodeString decodedService;
  REQUIRE_NOTHROW(decodedService.decode(report.parameterValues.at(reportParameters[0])));
  REQUIRE(decodedService.get() == L"UpdateAttributeValues");
  rti1516_2025::HLAinteger16BE decodedServiceType;
  REQUIRE_NOTHROW(decodedServiceType.decode(report.parameterValues.at(reportParameters[1])));
  REQUIRE(decodedServiceType.get() == 2);
  rti1516_2025::HLAboolean decodedSuccess;
  REQUIRE_NOTHROW(decodedSuccess.decode(report.parameterValues.at(reportParameters[2])));
  REQUIRE(decodedSuccess.get());

  rti1516_2025::HLAfixedRecord argumentPrototype;
  argumentPrototype.appendElement(rti1516_2025::HLAinteger32BE{})
      .appendElement(rti1516_2025::HLAunicodeString{})
      .appendElement(rti1516_2025::HLAunicodeString{});
  rti1516_2025::HLAvariableArray suppliedArguments{argumentPrototype};
  REQUIRE_NOTHROW(
      suppliedArguments.decode(report.parameterValues.at(reportParameters[3])));
  REQUIRE(suppliedArguments.size() == 4U);
  auto const verifyArgument = [&](std::size_t argumentIndex,
                                  std::int32_t type,
                                  std::wstring const& name,
                                  std::wstring const& valueText) {
    auto const& supplied = dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
        suppliedArguments.get(argumentIndex));
    REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(supplied.get(0U)).get() == type);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(supplied.get(1U)).get() == name);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(supplied.get(2U)).get() == valueText);
  };
  verifyArgument(
      0U,
      37,
      L"Object instance designator",
      L"\"" + objectInstance.toString() + L"\"");
  verifyArgument(
      1U,
      2,
      L"Constrained set of attribute designator and value pairs",
      L"{\"" + attribute.toString() + L"\":\"AQI=\"}");
  verifyArgument(2U, 60, L"User-supplied tag", L"\"dHNv\"");
  verifyArgument(
      3U,
      31,
      L"Optional timestamp",
      L"\"" + timestamp.toString() + L"\"");

  rti1516_2025::HLAfixedRecord returnedArgument;
  returnedArgument.appendElement(rti1516_2025::HLAinteger32BE{})
      .appendElement(rti1516_2025::HLAunicodeString{})
      .appendElement(rti1516_2025::HLAunicodeString{});
  REQUIRE_NOTHROW(
      returnedArgument.decode(report.parameterValues.at(reportParameters[4])));
  REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(returnedArgument.get(0U)).get() ==
          33);
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(returnedArgument.get(1U)).get() ==
          L"Message retraction designator");
  auto const retractionText = retraction.toString();
  auto const retractionOpen = retractionText.find(L'(');
  auto const retractionClose = retractionText.find(L')');
  REQUIRE(retractionOpen != std::wstring::npos);
  REQUIRE(retractionClose > retractionOpen + 1U);
  auto const momRetraction =
      dynamic_cast<rti1516_2025::HLAunicodeString const&>(returnedArgument.get(2U)).get();
  constexpr std::wstring_view momRetractionPrefix = L"\"MessageRetractionHandle<";
  REQUIRE(momRetraction.rfind(std::wstring{momRetractionPrefix}, 0U) == 0U);
  REQUIRE(momRetraction.size() > momRetractionPrefix.size() + 1U);
  REQUIRE(momRetraction[momRetraction.size() - 2U] == L'>');
  REQUIRE(momRetraction.back() == L'\"');
  REQUIRE(
      momRetraction.substr(
          momRetractionPrefix.size(),
          momRetraction.size() - momRetractionPrefix.size() - 2U) ==
      retractionText.substr(retractionOpen + 1U, retractionClose - retractionOpen - 1U));
  rti1516_2025::HLAunicodeString decodedException;
  REQUIRE_NOTHROW(decodedException.decode(report.parameterValues.at(reportParameters[5])));
  REQUIRE(decodedException.get().empty());
  rti1516_2025::HLAinteger32BE decodedSerial;
  REQUIRE_NOTHROW(decodedSerial.decode(report.parameterValues.at(reportParameters[6])));
  REQUIRE(decodedSerial.get() == 0);

  // The immediate MOM report is observable before the constrained reflection;
  // the recipient then reaches timestamp six before the producer reaches two.
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(timestamp));
  REQUIRE(receiverReports.attributeReflectionReports.empty());
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(observerReports.interactionReports.size() == 1U);
  REQUIRE(receiverReports.attributeReflectionReports.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"reflect", "grant"});

  auto const& reflection = receiverReports.attributeReflectionReports.front();
  REQUIRE(reflection.objectInstance == objectInstance);
  REQUIRE(reflection.attributeValues.size() == 1U);
  REQUIRE(reflection.attributeValues.contains(attribute));
  REQUIRE(variableLengthDataBytes(reflection.attributeValues.at(attribute)) ==
          std::vector<unsigned char>(valueBytes, valueBytes + sizeof(valueBytes)));
  REQUIRE(variableLengthDataBytes(reflection.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(reflection.transportationType == reliableTransportation);
  REQUIRE(reflection.producingFederate == publisherHandle);
  REQUIRE_FALSE(reflection.sentRegionsSupplied);
  REQUIRE(reflection.sentRegions.empty());
  REQUIRE(reflection.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(reflection.timeValue == timestamp.toString());
  REQUIRE(reflection.sentOrderType == TIMESTAMP);
  REQUIRE(reflection.receivedOrderType == TIMESTAMP);
  REQUIRE(reflection.retractionSupplied);
  REQUIRE(reflection.retractionValid);

  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
} // namespace
