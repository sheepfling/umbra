#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded MOM HLAsetTiming schedules multiple joined-federate targets independently",
    "[integration][development-profile][federation-management][mom][periodic-mom]"
    "[callback-model][immediate-callback][evoked-callback][multi-federate-callback-ordering]"
    "[hla-set-timing-multi-target-independence]"
    "[rti.service.evoke-callback]"
    "[rti.service.send-interaction][federate.callback.reflect-attribute-values]") {
  auto runScenario = [&](CallbackModel const callbackModel) {
    ImmediatePeriodicFederateAmbassador observerReports;
    TestFederateAmbassador firstTargetReports;
    TestFederateAmbassador secondTargetReports;
    auto observer = makeRti();
    auto firstTarget = makeRti();
    auto secondTarget = makeRti();
    auto const federationName = nextFederationName();
    auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

    REQUIRE_NOTHROW(observer->connect(observerReports, callbackModel));
    REQUIRE_NOTHROW(firstTarget->connect(firstTargetReports, HLA_EVOKED));
    REQUIRE_NOTHROW(secondTarget->connect(secondTargetReports, HLA_EVOKED));
    REQUIRE_NOTHROW(
        firstTarget->createFederationExecution(
            federationName,
            fomModule,
            standard_hla::mom::integer64_time));
    auto const observerFederate = observer->joinFederationExecution(
        L"mom-multi-target-observer", L"observer", federationName);
    auto const firstFederate = firstTarget->joinFederationExecution(
        L"mom-multi-target-first", L"first", federationName);
    auto const secondFederate = secondTarget->joinFederationExecution(
        L"mom-multi-target-second", L"second", federationName);

    auto waitForReflectionCount = [&](std::size_t const expected,
                                      std::chrono::milliseconds const timeout) {
      auto const deadline = std::chrono::steady_clock::now() + timeout;
      while (std::chrono::steady_clock::now() < deadline) {
        // HLA_EVOKED has no timer thread: the periodic planner is pumped at
        // the caller's Evoke boundary. HLA_IMMEDIATE uses the same planner
        // from its private scheduler, so this branch only observes callbacks.
        if (callbackModel == HLA_EVOKED) {
          static_cast<void>(observer->evokeMultipleCallbacks(0.0, 0.0));
        }
        if (observerReports.reflectionSnapshot().size() >= expected) {
          return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
      }
      return observerReports.reflectionSnapshot().size() >= expected;
    };

    auto const momClass = observer->getObjectClassHandle(
        standard_hla::mom::federate_object_class);
    auto const federateHandleAttribute = observer->getAttributeHandle(
        momClass, standard_hla::mom::federate_handle);
    auto const logicalTimeAttribute = observer->getAttributeHandle(
        momClass, standard_hla::mom::logical_time);
    auto const lookaheadAttribute = observer->getAttributeHandle(
        momClass, standard_hla::mom::lookahead);
    REQUIRE(momClass.isValid());
    REQUIRE(federateHandleAttribute.isValid());
    REQUIRE(logicalTimeAttribute.isValid());
    REQUIRE(lookaheadAttribute.isValid());
    REQUIRE_NOTHROW(observer->subscribeObjectClassAttributes(
        momClass,
        AttributeHandleSet{
            federateHandleAttribute,
            logicalTimeAttribute,
            lookaheadAttribute},
        true));

    // Discovery is driven through the ordinary MOM object path. The observer
    // sees its own object plus one object for each independently joined target.
    REQUIRE(waitForReflectionCount(3U, std::chrono::seconds(2)));
    auto objectForFederate = [&](FederateHandle const& expected)
        -> std::optional<ObjectInstanceHandle> {
      auto const reflections = observerReports.reflectionSnapshot();
      auto const found = std::find_if(
          reflections.begin(),
          reflections.end(),
          [&](ImmediatePeriodicFederateAmbassador::Reflection const& reflection) {
            auto const value = reflection.attributeValues.find(federateHandleAttribute);
            return value != reflection.attributeValues.end() &&
                observer->decodeFederateHandle(value->second) == expected;
          });
      if (found == reflections.end()) {
        return std::nullopt;
      }
      return found->objectInstance;
    };
    auto const observerObject = objectForFederate(observerFederate);
    auto const firstObject = objectForFederate(firstFederate);
    auto const secondObject = objectForFederate(secondFederate);
    REQUIRE(observerObject.has_value());
    REQUIRE(firstObject.has_value());
    REQUIRE(secondObject.has_value());
    REQUIRE(*firstObject != *secondObject);

    auto const setTiming = observer->getInteractionClassHandle(
        standard_hla::mom::set_timing);
    auto const federateParameter = observer->getParameterHandle(
        setTiming, standard_hla::mom::federate);
    auto const periodParameter = observer->getParameterHandle(
        setTiming, standard_hla::mom::report_period);
    REQUIRE(setTiming.isValid());
    REQUIRE(federateParameter.isValid());
    REQUIRE(periodParameter.isValid());

    auto sendTiming = [&](FederateHandle const& target, std::int32_t seconds) {
      REQUIRE_NOTHROW(observer->sendInteraction(
          setTiming,
          ParameterHandleValueMap{
              {federateParameter, target.encode()},
              {periodParameter, rti1516_2025::HLAinteger32BE{seconds}.encode()}},
          VariableLengthData{}));
    };

    auto const beforePeriodic = observerReports.reflectionSnapshot().size();
    // The first target is due after one second; the second is due after two.
    // Both callback models claim the same registry-owned target-local
    // deadlines, but HLA_IMMEDIATE uses its timer and HLA_EVOKED claims them
    // at the caller's Evoke boundary.
    sendTiming(firstFederate, 1);
    sendTiming(secondFederate, 2);
    REQUIRE(waitForReflectionCount(
        beforePeriodic + 1U,
        std::chrono::milliseconds(1500)));
    auto reflections = observerReports.reflectionSnapshot();
    auto periodicFor = [&](ObjectInstanceHandle const& object) {
      return std::count_if(
          reflections.begin() + static_cast<std::ptrdiff_t>(beforePeriodic),
          reflections.end(),
          [&](ImmediatePeriodicFederateAmbassador::Reflection const& reflection) {
            return reflection.objectInstance == object &&
                reflection.attributeValues.contains(logicalTimeAttribute) &&
                reflection.attributeValues.contains(lookaheadAttribute);
          });
    };
    REQUIRE(periodicFor(*firstObject) == 1);
    REQUIRE(periodicFor(*secondObject) == 0);

    // Disable only the first target. The second target must still receive its
    // independent deadline update, proving that one target's zero period does
    // not cancel or redirect another target's periodic state.
    sendTiming(firstFederate, 0);
    REQUIRE(waitForReflectionCount(
        beforePeriodic + 2U,
        std::chrono::milliseconds(1800)));
    reflections = observerReports.reflectionSnapshot();
    REQUIRE(periodicFor(*firstObject) == 1);
    REQUIRE(periodicFor(*secondObject) == 1);
    for (auto const& reflection : reflections) {
      if (reflection.objectInstance != *firstObject &&
          reflection.objectInstance != *secondObject) {
        continue;
      }
      if (reflection.attributeValues.contains(logicalTimeAttribute)) {
        REQUIRE(reflection.attributeValues.contains(lookaheadAttribute));
        REQUIRE(reflection.transportationType ==
                observer->getTransportationTypeHandle(standard_hla::mom::reliable));
        REQUIRE_FALSE(reflection.producingFederate.isValid());
        REQUIRE_FALSE(reflection.sentRegionsSupplied);
      }
    }

    REQUIRE_NOTHROW(firstTarget->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(secondTarget->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(observer->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(observer->disconnect());
    REQUIRE_NOTHROW(firstTarget->disconnect());
    REQUIRE_NOTHROW(secondTarget->disconnect());
  };

  SECTION("HLA_IMMEDIATE") {
    runScenario(rti1516_2025::HLA_IMMEDIATE);
  }
  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
}
}
