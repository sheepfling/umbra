#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "hla_test_names.hpp"

namespace {
TEST_CASE(
    "Embedded timestamped Send Interaction uses interaction FDD preferred order by default",
    "[integration][development-profile][interaction-management][time-management]"
    "[fdd-preferred-interaction-order][tso][receive-order][2025]"
    "[rti.service.send-interaction][rti.service.enable-time-regulation]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
                          "data" / "interaction-fdd-preferred-order-defaults-fom.xml")
                             .wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"fdd-preferred-order-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"fdd-preferred-order-receiver", L"receiver", federationName));

  auto const timestampedClass = publisher->getInteractionClassHandle(
      umbra::test::hla::wide::fom::fdd_preferred_timestamped_interaction);
  auto const receiveClass = publisher->getInteractionClassHandle(
      umbra::test::hla::wide::fom::fdd_preferred_receive_interaction);
  auto const timestampedReceiverClass = receiver->getInteractionClassHandle(
      umbra::test::hla::wide::fom::fdd_preferred_timestamped_interaction);
  auto const receiveReceiverClass = receiver->getInteractionClassHandle(
      umbra::test::hla::wide::fom::fdd_preferred_receive_interaction);
  auto const timestampedValue = publisher->getParameterHandle(timestampedClass, umbra::test::hla::wide::fixture::value);
  auto const receiveValue = publisher->getParameterHandle(receiveClass, umbra::test::hla::wide::fixture::value);
  REQUIRE(timestampedClass.isValid());
  REQUIRE(receiveClass.isValid());
  REQUIRE(timestampedValue.isValid());
  REQUIRE(receiveValue.isValid());
  REQUIRE_NOTHROW(publisher->publishInteractionClass(timestampedClass));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(receiveClass));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(timestampedReceiverClass));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(receiveReceiverClass));

  // Time regulation makes sender role incapable of masking the FDD default.
  // Neither class has its preferred order changed through the API.
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  for (unsigned attempt = 0U;
       attempt != 20U && publisherReports.timeRegulationEnabledReports.empty();
       ++attempt) {
    static_cast<void>(publisher->evokeCallback(0.1));
  }
  REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1U);

  unsigned char const payload[] = {'o', 'k'};
  unsigned char const tagBytes[] = {'f', 'd', 'd'};
  VariableLengthData const value(payload, sizeof(payload));
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  auto const timestampedRetraction = publisher->sendInteraction(
      timestampedClass,
      ParameterHandleValueMap{{timestampedValue, value}},
      tag,
      rti1516_2025::HLAinteger64Time(5));
  auto const receiveRetraction = publisher->sendInteraction(
      receiveClass,
      ParameterHandleValueMap{{receiveValue, value}},
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(timestampedRetraction.isValid());
  REQUIRE_FALSE(receiveRetraction.isValid());

  for (unsigned attempt = 0U;
       attempt != 20U && receiverReports.timestampedInteractionReports.size() < 2U;
       ++attempt) {
    static_cast<void>(receiver->evokeCallback(0.1));
  }
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 2U);
  auto const timestamped = std::find_if(
      receiverReports.timestampedInteractionReports.begin(),
      receiverReports.timestampedInteractionReports.end(),
      [&](auto const& report) { return report.interactionClass == timestampedReceiverClass; });
  auto const receive = std::find_if(
      receiverReports.timestampedInteractionReports.begin(),
      receiverReports.timestampedInteractionReports.end(),
      [&](auto const& report) { return report.interactionClass == receiveReceiverClass; });
  REQUIRE(timestamped != receiverReports.timestampedInteractionReports.end());
  REQUIRE(receive != receiverReports.timestampedInteractionReports.end());
  REQUIRE(timestamped->sentOrderType == TIMESTAMP);
  REQUIRE(timestamped->timeValue == L"5");
  REQUIRE(timestamped->retractionSupplied);
  REQUIRE(timestamped->retractionValid);
  REQUIRE(receive->sentOrderType == RECEIVE);
  REQUIRE(receive->timeValue == L"6");
  REQUIRE_FALSE(receive->retractionSupplied);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
  static_cast<void>(publisherHandle);
}
}  // namespace
