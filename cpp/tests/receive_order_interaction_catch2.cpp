#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The receive-order interaction test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using TestFederateAmbassador = rti1516_2025::NullFederateAmbassador;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::ParameterHandle;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

std::vector<unsigned char> variableLengthDataBytes(
    VariableLengthData const& value) {
  auto const* bytes = static_cast<unsigned char const*>(value.data());
  if (bytes == nullptr) {
    return {};
  }
  return {bytes, bytes + value.size()};
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"receive-order-interaction-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct InteractionReport final {
    InteractionClassHandle interactionClass;
    ParameterHandleValueMap parameterValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    bool sentRegionsSupplied = false;
  };

  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      rti1516_2025::RegionHandleSet const* optionalSentRegions) override {
    interactionReports.push_back({
        interactionClass,
        parameterValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        optionalSentRegions != nullptr,
    });
  }

  std::vector<InteractionReport> interactionReports;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

void drainCallbacks(RTIambassador& rti) {
  for (int pass = 0; pass != 128; ++pass) {
    if (!rti.evokeCallback(0.0)) {
      break;
    }
  }
}

}  // namespace

TEST_CASE(
    "Embedded receive-order Send Interaction honors 2025 promotion and callback lifecycle",
    "[integration][development-profile][interaction-management]"
    "[interaction-sent-class-preservation]"
    "[rti.service.subscribe-interaction-class][rti.service.send-interaction]"
    "[federate.callback.receive-interaction]") {
  TestFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador exactReports;
  ReportingFederateAmbassador promotedReports;
  ReportingFederateAmbassador dualSubscriptionReports;
  ReportingFederateAmbassador cancelledReports;
  ReportingFederateAmbassador immediateReports;
  auto unjoined = makeRti();
  auto publisher = makeRti();
  auto exact = makeRti();
  auto promoted = makeRti();
  auto dualSubscription = makeRti();
  auto cancelled = makeRti();
  auto immediate = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x71, 0x2A};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  ParameterHandleValueMap noParameters;
  InteractionClassHandle invalidInteractionClass;

  REQUIRE_THROWS_AS(
      unjoined->sendInteraction(invalidInteractionClass, noParameters, tag),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->sendInteraction(invalidInteractionClass, noParameters, tag),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(exact->connect(exactReports, HLA_EVOKED));
  REQUIRE_NOTHROW(promoted->connect(promotedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(dualSubscription->connect(dualSubscriptionReports, HLA_EVOKED));
  REQUIRE_NOTHROW(cancelled->connect(cancelledReports, HLA_EVOKED));
  REQUIRE_NOTHROW(immediate->connect(
      immediateReports,
      rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));

  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"interaction-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(exact->joinFederationExecution(
      L"interaction-exact",
      L"subscriber",
      federationName));
  REQUIRE_NOTHROW(promoted->joinFederationExecution(
      L"interaction-promoted",
      L"subscriber",
      federationName));
  REQUIRE_NOTHROW(dualSubscription->joinFederationExecution(
      L"interaction-dual",
      L"subscriber",
      federationName));
  REQUIRE_NOTHROW(cancelled->joinFederationExecution(
      L"interaction-cancelled",
      L"subscriber",
      federationName));
  REQUIRE_NOTHROW(immediate->joinFederationExecution(
      L"interaction-immediate",
      L"subscriber",
      federationName));

  auto const base = publisher->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_base);
  auto const child = publisher->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = publisher->getParameterHandle(
      child,
      fixture_hla::fixture::identifier);
  REQUIRE(base.isValid());
  REQUIRE(child.isValid());
  REQUIRE(identifier.isValid());
  unsigned char const identifierBytes[] = {0xC4, 0x19, 0x02};
  ParameterHandleValueMap parameterValues;
  parameterValues.emplace(
      identifier,
      VariableLengthData(identifierBytes, sizeof(identifierBytes)));

  REQUIRE_THROWS_AS(
      publisher->sendInteraction(child, parameterValues, tag),
      rti1516_2025::InteractionClassNotPublished);
  REQUIRE_NOTHROW(publisher->publishInteractionClass(child));
  REQUIRE_THROWS_AS(
      publisher->sendInteraction(invalidInteractionClass, parameterValues, tag),
      rti1516_2025::InteractionClassNotDefined);
  ParameterHandle invalidParameter;
  ParameterHandleValueMap invalidParameterValues;
  invalidParameterValues.emplace(invalidParameter, VariableLengthData());
  REQUIRE_THROWS_AS(
      publisher->sendInteraction(child, invalidParameterValues, tag),
      rti1516_2025::InteractionParameterNotDefined);

  // The child class passed to Send Interaction remains the sent class. A
  // passive base-only recipient still receives once another joined federate
  // has established relevance, with its callback class promoted to the base
  // per §6.1.9. A recipient subscribed to both base and child receives only
  // once, with the original parameter value preserved.
  REQUIRE_NOTHROW(exact->subscribeInteractionClass(child));
  REQUIRE_NOTHROW(promoted->subscribeInteractionClass(base, false));
  REQUIRE_NOTHROW(dualSubscription->subscribeInteractionClass(base, false));
  REQUIRE_NOTHROW(dualSubscription->subscribeInteractionClass(child));
  REQUIRE_NOTHROW(cancelled->subscribeInteractionClass(base));
  REQUIRE_NOTHROW(immediate->subscribeInteractionClass(child));

  REQUIRE_NOTHROW(publisher->sendInteraction(child, parameterValues, tag));
  REQUIRE(promotedReports.interactionReports.empty());
  REQUIRE(dualSubscriptionReports.interactionReports.empty());
  REQUIRE(cancelledReports.interactionReports.empty());
  REQUIRE(immediateReports.interactionReports.size() == 1U);

  // Removing the active subscription before callback delivery suppresses the
  // queued receive-order invocation.
  REQUIRE_NOTHROW(cancelled->unsubscribeInteractionClass(base));
  drainCallbacks(*exact);
  drainCallbacks(*promoted);
  drainCallbacks(*dualSubscription);
  drainCallbacks(*cancelled);

  auto requireDelivery = [&](ReportingFederateAmbassador::InteractionReport const& report,
                             InteractionClassHandle const& expectedClass) {
    REQUIRE(report.interactionClass == expectedClass);
    REQUIRE(report.parameterValues.size() == 1U);
    auto const value = report.parameterValues.find(identifier);
    REQUIRE(value != report.parameterValues.end());
    REQUIRE(variableLengthDataBytes(value->second) ==
            std::vector<unsigned char>(
                identifierBytes,
                identifierBytes + sizeof(identifierBytes)));
    REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
            std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
    REQUIRE(report.transportationType ==
            publisher->getTransportationTypeHandle(standard_hla::mom::reliable));
    REQUIRE(report.producingFederate == publisherHandle);
    REQUIRE_FALSE(report.sentRegionsSupplied);
  };

  REQUIRE(exactReports.interactionReports.size() == 1U);
  requireDelivery(exactReports.interactionReports.front(), child);
  REQUIRE(promotedReports.interactionReports.size() == 1U);
  requireDelivery(promotedReports.interactionReports.front(), base);
  REQUIRE(dualSubscriptionReports.interactionReports.size() == 1U);
  requireDelivery(dualSubscriptionReports.interactionReports.front(), child);
  REQUIRE(immediateReports.interactionReports.size() == 1U);
  requireDelivery(immediateReports.interactionReports.front(), child);
  REQUIRE(cancelledReports.interactionReports.empty());
  REQUIRE(publisherReports.interactionReports.empty());

  REQUIRE_NOTHROW(immediate->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(cancelled->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(dualSubscription->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(promoted->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(exact->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(exact->disconnect());
  REQUIRE_NOTHROW(promoted->disconnect());
  REQUIRE_NOTHROW(dualSubscription->disconnect());
  REQUIRE_NOTHROW(cancelled->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
}

TEST_CASE(
    "Embedded receive-order Send Interaction follows current publication state",
    "[integration][development-profile][2025][interaction-management]"
    "[interaction-publication-send-boundary]"
    "[rti.service.publish-interaction-class]"
    "[rti.service.unpublish-interaction-class]"
    "[rti.service.send-interaction]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" / "tests" / "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"interaction-publication-state-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"interaction-publication-state-receiver", L"subscriber", federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const parameter = publisher->getParameterHandle(
      interactionClass, fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(parameter.isValid());
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));

  unsigned char const firstPayload[] = {0x11, 0x25};
  unsigned char const firstTagBytes[] = {0xA1};
  ParameterHandleValueMap firstParameters;
  firstParameters.emplace(
      parameter, VariableLengthData(firstPayload, sizeof(firstPayload)));
  VariableLengthData const firstTag(firstTagBytes, sizeof(firstTagBytes));
  REQUIRE_NOTHROW(publisher->sendInteraction(
      interactionClass, firstParameters, firstTag));
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.interactionReports.size() == 1U);
  REQUIRE(receiverReports.interactionReports.front().interactionClass ==
          interactionClass);
  REQUIRE(variableLengthDataBytes(
              receiverReports.interactionReports.front().parameterValues.at(parameter)) ==
          std::vector<unsigned char>(
              firstPayload, firstPayload + sizeof(firstPayload)));
  REQUIRE(variableLengthDataBytes(
              receiverReports.interactionReports.front().userSuppliedTag) ==
          std::vector<unsigned char>(firstTagBytes, firstTagBytes + sizeof(firstTagBytes)));

  REQUIRE_NOTHROW(publisher->unpublishInteractionClass(interactionClass));
  REQUIRE_THROWS_AS(
      publisher->sendInteraction(interactionClass, firstParameters, firstTag),
      rti1516_2025::InteractionClassNotPublished);
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.interactionReports.size() == 1U);

  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  unsigned char const secondPayload[] = {0x22, 0x25};
  unsigned char const secondTagBytes[] = {0xA2};
  ParameterHandleValueMap secondParameters;
  secondParameters.emplace(
      parameter, VariableLengthData(secondPayload, sizeof(secondPayload)));
  VariableLengthData const secondTag(secondTagBytes, sizeof(secondTagBytes));
  REQUIRE_NOTHROW(publisher->sendInteraction(
      interactionClass, secondParameters, secondTag));
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.interactionReports.size() == 2U);
  auto const& secondReport = receiverReports.interactionReports.back();
  REQUIRE(secondReport.interactionClass == interactionClass);
  REQUIRE(variableLengthDataBytes(secondReport.parameterValues.at(parameter)) ==
          std::vector<unsigned char>(
              secondPayload, secondPayload + sizeof(secondPayload)));
  REQUIRE(variableLengthDataBytes(secondReport.userSuppliedTag) ==
          std::vector<unsigned char>(secondTagBytes, secondTagBytes + sizeof(secondTagBytes)));

  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
