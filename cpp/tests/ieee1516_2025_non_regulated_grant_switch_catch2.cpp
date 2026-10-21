#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
void drainCallbacks(RTIambassador& rti) {
  for (int pass = 0; pass != 128; ++pass) {
    if (!rti.evokeCallback(0.0)) {
      break;
    }
  }
}

void verifyFlushQueueRequestWithUndefinedGalt(
    bool nrgEnabled,
    bool queueTimestampedInteraction) {
  auto const nrgFom = nrgEnabledRestaurantModule();
  ReportingFederateAmbassador requesterReports;
  ReportingFederateAmbassador producerReports;
  auto requester = makeRti();
  auto producer = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom =
      resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const interactionFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "parameter-handle-provider-fom.xml")
          .wstring();
  std::vector<std::wstring> fomModules{
      nrgEnabled ? nrgFom.path().wstring() : restaurantFom,
      interactionFom};

  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(producer->connect(producerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->createFederationExecution(
      federationName,
      fomModules,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"undefined-galt-fqr-requester", L"requester", federationName));
  REQUIRE_NOTHROW(producer->joinFederationExecution(
      L"undefined-galt-fqr-producer", L"producer", federationName));
  REQUIRE(requester->getNonRegulatedGrantSwitch() == nrgEnabled);
  REQUIRE_NOTHROW(requester->enableTimeConstrained());
  drainCallbacks(*requester);

  if (queueTimestampedInteraction) {
    auto const interactionClass = producer->getInteractionClassHandle(
        fixture_hla::fom::parameter_fixture_child_interaction);
    auto const identifier = producer->getParameterHandle(
        interactionClass, fixture_hla::fixture::identifier);
    REQUIRE(interactionClass.isValid());
    REQUIRE(identifier.isValid());
    REQUIRE_NOTHROW(producer->publishInteractionClass(interactionClass));
    REQUIRE_NOTHROW(producer->changeInteractionOrderType(
        interactionClass, TIMESTAMP));
    REQUIRE_NOTHROW(requester->subscribeInteractionClass(interactionClass));
    REQUIRE_NOTHROW(producer->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    drainCallbacks(*producer);

    unsigned char const valueBytes[] = {0x4EU, 0x52U, 0x47U};
    unsigned char const tagBytes[] = {0x46U, 0x51U, 0x52U};
    ParameterHandleValueMap values;
    values.emplace(
        identifier,
        VariableLengthData(valueBytes, sizeof(valueBytes)));
    VariableLengthData const tag(tagBytes, sizeof(tagBytes));
    REQUIRE(producer->sendInteraction(
                interactionClass,
                values,
                tag,
                rti1516_2025::HLAinteger64Time(5))
                .isValid());
    REQUIRE_NOTHROW(producer->disableTimeRegulation());
    drainCallbacks(*producer);
  }

  rti1516_2025::HLAinteger64Time galt;
  REQUIRE_FALSE(requester->queryGALT(galt));
  REQUIRE_NOTHROW(requester->flushQueueRequest(
      rti1516_2025::HLAinteger64Time(10)));
  drainCallbacks(*requester);

  REQUIRE(requesterReports.flushQueueGrantReports.size() == 1U);
  auto const& grant = requesterReports.flushQueueGrantReports.front();
  REQUIRE(grant.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(grant.value ==
          (nrgEnabled
               ? queueTimestampedInteraction ? L"5" : L"10"
               : L"0"));
  REQUIRE(grant.optimisticValue ==
          (queueTimestampedInteraction ? L"5" : L"10"));
  REQUIRE(requesterReports.timestampedInteractionReports.size() ==
          (queueTimestampedInteraction ? 1U : 0U));
  REQUIRE(requesterReports.callbackOrder ==
          (queueTimestampedInteraction
               ? std::vector<std::string>{"interaction", "flush-grant"}
               : std::vector<std::string>{"flush-grant"}));
  REQUIRE_FALSE(requester->queryGALT(galt));

  rti1516_2025::HLAinteger64Time logicalTime;
  REQUIRE_NOTHROW(requester->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == (nrgEnabled
                                        ? queueTimestampedInteraction ? 5 : 10
                                        : 0));

  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(producer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(requester->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(producer->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}

TEST_CASE(
    "Embedded additional FOM NRG metadata cannot change the static switch or TAR",
    "[integration][development-profile][time-management][fom][non-regulated-grant][callbacks]"
    "[rti.service.join-federation-execution][rti.service.time-advance-request]"
    "[federate.callback.time-advance-grant]") {
  auto const nrgFom = nrgEnabledRestaurantModule();
  ReportingFederateAmbassador receiverReports;
  ReportingFederateAmbassador applicantReports;
  auto receiver = makeRti();
  auto applicant = makeRti();
  auto const federationName = nextFederationName();
  auto const baseFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      receiver->createFederationExecution(federationName, baseFom, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(L"receiver", federationName));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));

  // The official Restaurant FOM omits NRG, which means Disabled. With no
  // regulator, the constrained TAR must remain pending before the additional
  // module joins.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());

  REQUIRE_NOTHROW(applicant->connect(applicantReports, HLA_EVOKED));
  REQUIRE_NOTHROW(applicant->joinFederationExecution(
      L"metadata-applicant",
      L"observer",
      federationName,
      std::vector<std::wstring>{nrgFom.path().wstring()}));

  // NRG is a static federation-wide switch established at creation. The
  // additional module cannot change it or release the existing TAR.
  REQUIRE_FALSE(receiver->getNonRegulatedGrantSwitch());
  REQUIRE_FALSE(applicant->getNonRegulatedGrantSwitch());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());

  REQUIRE_NOTHROW(applicant->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(applicant->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());
}

TEST_CASE(
    "Embedded FQR applies NRG when GALT is undefined",
    "[integration][development-profile][2025][time-management]"
    "[flush-queue-request][non-regulated-grant][undefined-galt]"
    "[rti.service.flush-queue-request][federate.callback.flush-queue-grant]"
    "[federate.callback.receive-interaction]") {
  SECTION("disabled NRG with no queued TSO stays at current time") {
    verifyFlushQueueRequestWithUndefinedGalt(false, false);
  }
  SECTION("enabled NRG with no queued TSO permits the request time") {
    verifyFlushQueueRequestWithUndefinedGalt(true, false);
  }
  SECTION("disabled NRG with queued TSO keeps actual grant at current time") {
    verifyFlushQueueRequestWithUndefinedGalt(false, true);
  }
  SECTION("enabled NRG with queued TSO grants at its delivery frontier") {
    verifyFlushQueueRequestWithUndefinedGalt(true, true);
  }
}
}  // namespace
