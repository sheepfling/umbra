#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/BasicDataElements.h>

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The joined-federate HLAsetSwitches test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::ParameterHandle;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::VariableLengthData;

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"mom-federate-set-switches-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / relativePath;
}

VariableLengthData encodeSwitch(bool const enabled) {
  return rti1516_2025::HLAinteger32BE{enabled ? 1 : 0}.encode();
}

VariableLengthData encodeResignAction(std::int32_t const value) {
  return rti1516_2025::HLAinteger32BE{value}.encode();
}

VariableLengthData opaqueBytes(std::initializer_list<unsigned char> values) {
  auto const bytes = std::vector<unsigned char>(values);
  return VariableLengthData(bytes.data(), bytes.size());
}

}  // namespace

TEST_CASE(
    "Embedded MOM HLAsetSwitches updates the sending federate's switch subset",
    "[integration][development-profile][federation-management][mom][switches]"
    "[mom-federate-set-switches]"
    "[rti.service.send-interaction]"
    "[rti.service.subscribe-interaction-class]"
    "[rti.service.unsubscribe-interaction-class]"
    "[rti.service.get-object-class-relevance-advisory-switch]"
    "[rti.service.get-attribute-relevance-advisory-switch]"
    "[rti.service.get-attribute-scope-advisory-switch]"
    "[rti.service.get-interaction-relevance-advisory-switch]"
    "[rti.service.get-convey-region-designator-sets-switch]"
    "[rti.service.get-automatic-resign-directive]"
    "[rti.service.get-service-reporting-switch]"
    "[rti.service.get-exception-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]") {
  rti1516_2025::NullFederateAmbassador subjectReports;
  rti1516_2025::NullFederateAmbassador peerReports;
  auto subject = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
       "ieee1516.2-2025" / "resources" / "examples" /
       "RestaurantFOMmodule-2025.xml")
          .wstring();
  auto const extensionFom = resourcePath("mom-set-switches-extension-fom.xml").wstring();

  REQUIRE_NOTHROW(subject->connect(subjectReports, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(subject->createFederationExecution(
      federationName,
      std::vector<std::wstring>{restaurantFom, extensionFom},
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(subject->joinFederationExecution(
      L"set-switches-subject", L"subject", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"set-switches-peer", L"peer", federationName));

  auto const setSwitches = subject->getInteractionClassHandle(
      standard_hla::mom::set_switches_federate);
  auto const extendedSetSwitches = subject->getInteractionClassHandle(
      fixture_hla::fom::extended_set_switches);
  REQUIRE(setSwitches.isValid());
  REQUIRE(extendedSetSwitches.isValid());

  auto const serviceReporting = subject->getParameterHandle(
      setSwitches, standard_hla::mom::service_reporting);
  auto const automaticResign = subject->getParameterHandle(
      setSwitches, standard_hla::mom::automatic_resign_action);
  auto const extensionPayload = subject->getParameterHandle(
      setSwitches, fixture_hla::fixture::umbra_extension_switch_payload);
  auto const subclassPayload = subject->getParameterHandle(
      extendedSetSwitches, fixture_hla::fixture::umbra_extended_switch_payload);
  auto const reportServiceInvocation = subject->getInteractionClassHandle(
      standard_hla::mom::report_service_invocation);
  REQUIRE(serviceReporting.isValid());
  REQUIRE(automaticResign.isValid());
  REQUIRE(extensionPayload.isValid());
  REQUIRE(subclassPayload.isValid());
  REQUIRE(reportServiceInvocation.isValid());

  auto const subjectObjectClassRelevance =
      subject->getObjectClassRelevanceAdvisorySwitch();
  auto const subjectAttributeRelevance =
      subject->getAttributeRelevanceAdvisorySwitch();
  auto const subjectAttributeScope = subject->getAttributeScopeAdvisorySwitch();
  auto const subjectInteractionRelevance =
      subject->getInteractionRelevanceAdvisorySwitch();
  auto const subjectConveyRegionDesignators =
      subject->getConveyRegionDesignatorSetsSwitch();
  auto const subjectExceptionReporting = subject->getExceptionReportingSwitch();
  auto const subjectSendServiceReportsToFile =
      subject->getSendServiceReportsToFileSwitch();
  auto const peerObjectClassRelevance =
      peer->getObjectClassRelevanceAdvisorySwitch();
  auto const peerAttributeRelevance = peer->getAttributeRelevanceAdvisorySwitch();
  auto const peerAttributeScope = peer->getAttributeScopeAdvisorySwitch();
  auto const peerInteractionRelevance =
      peer->getInteractionRelevanceAdvisorySwitch();
  auto const peerConveyRegionDesignators =
      peer->getConveyRegionDesignatorSetsSwitch();
  auto const peerServiceReporting = peer->getServiceReportingSwitch();
  auto const peerAutomaticResign = peer->getAutomaticResignDirective();
  auto const peerExceptionReporting = peer->getExceptionReportingSwitch();
  auto const peerSendServiceReportsToFile =
      peer->getSendServiceReportsToFileSwitch();
  REQUIRE_NOTHROW(subject->sendInteraction(
      setSwitches,
      ParameterHandleValueMap{
          {serviceReporting, encodeSwitch(false)},
          {automaticResign, encodeResignAction(5)},
          {extensionPayload, opaqueBytes({0x01, 0x02, 0x03})},
      },
      VariableLengthData{}));
  REQUIRE_FALSE(subject->getServiceReportingSwitch());
  REQUIRE(subject->getAutomaticResignDirective() == rti1516_2025::NO_ACTION);
  REQUIRE(subject->getObjectClassRelevanceAdvisorySwitch() ==
          subjectObjectClassRelevance);
  REQUIRE(subject->getAttributeRelevanceAdvisorySwitch() ==
          subjectAttributeRelevance);
  REQUIRE(subject->getAttributeScopeAdvisorySwitch() == subjectAttributeScope);
  REQUIRE(subject->getInteractionRelevanceAdvisorySwitch() ==
          subjectInteractionRelevance);
  REQUIRE(subject->getConveyRegionDesignatorSetsSwitch() ==
          subjectConveyRegionDesignators);
  REQUIRE(subject->getExceptionReportingSwitch() == subjectExceptionReporting);
  REQUIRE(subject->getSendServiceReportsToFileSwitch() ==
          subjectSendServiceReportsToFile);
  REQUIRE(peer->getObjectClassRelevanceAdvisorySwitch() == peerObjectClassRelevance);
  REQUIRE(peer->getAttributeRelevanceAdvisorySwitch() == peerAttributeRelevance);
  REQUIRE(peer->getAttributeScopeAdvisorySwitch() == peerAttributeScope);
  REQUIRE(peer->getInteractionRelevanceAdvisorySwitch() ==
          peerInteractionRelevance);
  REQUIRE(peer->getConveyRegionDesignatorSetsSwitch() ==
          peerConveyRegionDesignators);
  REQUIRE(peer->getServiceReportingSwitch() == peerServiceReporting);
  REQUIRE(peer->getAutomaticResignDirective() == peerAutomaticResign);
  REQUIRE(peer->getExceptionReportingSwitch() == peerExceptionReporting);
  REQUIRE(peer->getSendServiceReportsToFileSwitch() ==
          peerSendServiceReportsToFile);

  // A predefined interaction subclass is promoted to the standard behavior;
  // its extension-only parameter is received but does not mutate state.
  REQUIRE_NOTHROW(subject->sendInteraction(
      extendedSetSwitches,
      ParameterHandleValueMap{{subclassPayload, opaqueBytes({0x04, 0x05})}},
      VariableLengthData{}));
  REQUIRE_FALSE(subject->getServiceReportingSwitch());
  REQUIRE(subject->getAutomaticResignDirective() == rti1516_2025::NO_ACTION);

  REQUIRE_THROWS_AS(
      subject->sendInteraction(
          setSwitches,
          ParameterHandleValueMap{{automaticResign, encodeResignAction(99)}},
          VariableLengthData{}),
      rti1516_2025::RTIinternalError);
  REQUIRE(subject->getAutomaticResignDirective() == rti1516_2025::NO_ACTION);
  REQUIRE_NOTHROW(subject->subscribeInteractionClass(reportServiceInvocation));
  REQUIRE_THROWS_AS(
      subject->sendInteraction(
          setSwitches,
          ParameterHandleValueMap{{serviceReporting, encodeSwitch(true)}},
          VariableLengthData{}),
      rti1516_2025::RTIinternalError);
  REQUIRE_NOTHROW(subject->unsubscribeInteractionClass(reportServiceInvocation));

  REQUIRE_THROWS_AS(
      subject->sendInteraction(
          setSwitches,
          ParameterHandleValueMap{},
          VariableLengthData{}),
      rti1516_2025::InteractionParameterNotDefined);
  REQUIRE_NOTHROW(subject->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(peer->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subject->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}
