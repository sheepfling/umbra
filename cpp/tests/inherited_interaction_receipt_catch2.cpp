#include <catch2/catch_test_macros.hpp>

#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The inherited-interaction receipt test requires the Umbra source directory."
#endif

namespace {

using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::VariableLengthData;

class InteractionReceiver final : public NullFederateAmbassador {
 public:
  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const&,
      VariableLengthData const&,
      rti1516_2025::TransportationTypeHandle const&,
      FederateHandle const&,
      rti1516_2025::RegionHandleSet const*) override {
    receivedClasses.push_back(interactionClass);
  }

  std::vector<InteractionClassHandle> receivedClasses;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::filesystem::path interactionFom() {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / "inherited-interaction-receipt-fom.xml";
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"inherited-interaction-receipt-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

void drain(RTIambassador& ambassador) {
  for (int pass = 0; pass != 64; ++pass) {
    if (!ambassador.evokeCallback(0.0)) {
      break;
    }
  }
}

}  // namespace

TEST_CASE(
    "Embedded interaction receipt reports the subscribed candidate class",
    "[integration][development-profile][interaction-management]"
    "[interaction-receipt-inheritance][2025]"
    "[rti.service.publish-interaction-class][rti.service.subscribe-interaction-class]"
    "[rti.service.send-interaction][federate.callback.receive-interaction]"
    "[callback-evoked]") {
  NullFederateAmbassador publisherCallbacks;
  InteractionReceiver subscriberCallbacks;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = interactionFom().wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModule, L"HLAinteger64Time"));
  try {
    publisher->joinFederationExecution(
        L"inherited-interaction-publisher", L"publisher", federationName);
  } catch (rti1516_2025::Exception const& error) {
    auto const name = error.name();
    FAIL("Publisher Join Federation Execution failed with RTI exception "
         << std::string(name.begin(), name.end()));
  }
  try {
    subscriber->joinFederationExecution(
        L"inherited-interaction-subscriber", L"subscriber", federationName);
  } catch (rti1516_2025::Exception const& error) {
    auto const name = error.name();
    FAIL("Subscriber Join Federation Execution failed with RTI exception "
         << std::string(name.begin(), name.end()));
  }

  auto const publisherChild = publisher->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraInteractionBase.UmbraInteractionMiddle."
      L"UmbraInteractionChild");
  auto const subscriberBase = subscriber->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraInteractionBase");
  auto const subscriberMiddle = subscriber->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraInteractionBase.UmbraInteractionMiddle");
  auto const subscriberChild = subscriber->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraInteractionBase.UmbraInteractionMiddle."
      L"UmbraInteractionChild");
  REQUIRE(publisherChild.isValid());
  REQUIRE(subscriberBase.isValid());
  REQUIRE(subscriberMiddle.isValid());
  REQUIRE(subscriberChild.isValid());

  REQUIRE_NOTHROW(publisher->publishInteractionClass(publisherChild));
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClass(subscriberBase, true));
  REQUIRE_NOTHROW(publisher->sendInteraction(
      publisherChild, ParameterHandleValueMap{}, VariableLengthData{}));
  drain(*subscriber);

  REQUIRE(subscriberCallbacks.receivedClasses.size() == 1U);
  // The receive class is the closest subscribed ancestor that establishes
  // the candidate receipt for the sent descendant interaction.
  REQUIRE(subscriberCallbacks.receivedClasses.front() == subscriberBase);

  // With both ancestor subscriptions active, the closest matching superclass
  // is selected and only one receipt callback is produced.
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClass(subscriberMiddle, true));
  subscriberCallbacks.receivedClasses.clear();
  REQUIRE_NOTHROW(publisher->sendInteraction(
      publisherChild, ParameterHandleValueMap{}, VariableLengthData{}));
  drain(*subscriber);
  REQUIRE(subscriberCallbacks.receivedClasses.size() == 1U);
  REQUIRE(subscriberCallbacks.receivedClasses.front() == subscriberMiddle);

  // When the exact sent class is subscribed as well, it takes precedence
  // over either superclass and still produces one callback.
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClass(subscriberChild, true));
  subscriberCallbacks.receivedClasses.clear();
  REQUIRE_NOTHROW(publisher->sendInteraction(
      publisherChild, ParameterHandleValueMap{}, VariableLengthData{}));
  drain(*subscriber);
  REQUIRE(subscriberCallbacks.receivedClasses.size() == 1U);
  REQUIRE(subscriberCallbacks.receivedClasses.front() == subscriberChild);

  REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClass(subscriberBase));
  REQUIRE_NOTHROW(publisher->unpublishInteractionClass(publisherChild));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subscriber->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
