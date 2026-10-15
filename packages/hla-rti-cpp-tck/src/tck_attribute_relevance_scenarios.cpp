#include "tck_attribute_relevance_scenarios.hpp"

#include <cstddef>
#include <filesystem>
#include <string>

namespace hla_rti_cpp_tck::detail {

void runKnownClassAttributeRelevance(
    Options const& options,
    rti::CallbackModel model,
    std::filesystem::path const& fom,
    bool knownClassPolicyEnabled) {
  require(
      !fom.empty(),
      "known-class attribute relevance testing requires an adapter-supplied FOM");

  Session owner(options, model, "owner");
  Session subscriber(options, model, "member");
  auto const federation = federationName(
      options,
      "attribute-relevance-known-class");

  owner.connect();
  subscriber.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation,
      fom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);
  subscriber.join(
      options.memberFederateName,
      options.federateType,
      federation);

  require(
      owner.rtiAmbassador().getAdvisoriesUseKnownClassSwitch() ==
              knownClassPolicyEnabled &&
          subscriber.rtiAmbassador().getAdvisoriesUseKnownClassSwitch() ==
              knownClassPolicyEnabled,
      "adapter known-class FOM did not expose the configured static policy");
  require(
      owner.rtiAmbassador().getAttributeRelevanceAdvisorySwitch() &&
          subscriber.rtiAmbassador().getAttributeRelevanceAdvisorySwitch(),
      "adapter known-class FOM did not enable attribute relevance advisories");

  auto const ownerBaseClass = owner.rtiAmbassador().getObjectClassHandle(
      options.knownClassObjectClassName);
  auto const ownerDerivedClass = owner.rtiAmbassador().getObjectClassHandle(
      options.knownClassDerivedObjectClassName);
  auto const subscriberBaseClass = subscriber.rtiAmbassador().getObjectClassHandle(
      options.knownClassObjectClassName);
  auto const subscriberDerivedClass =
      subscriber.rtiAmbassador().getObjectClassHandle(
          options.knownClassDerivedObjectClassName);
  require(
      ownerBaseClass.isValid() && ownerDerivedClass.isValid() &&
          subscriberBaseClass.isValid() && subscriberDerivedClass.isValid() &&
          ownerBaseClass == subscriberBaseClass &&
          ownerDerivedClass == subscriberDerivedClass,
      "known-class FOM object-class handles did not retain identity across members");

  auto const ownerKnownAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerDerivedClass,
      options.knownClassKnownAttributeName);
  auto const ownerDerivedAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerDerivedClass,
      options.knownClassDerivedAttributeName);
  auto const subscriberKnownAttribute = subscriber.rtiAmbassador().getAttributeHandle(
      subscriberBaseClass,
      options.knownClassKnownAttributeName);
  auto const subscriberDerivedAttribute =
      subscriber.rtiAmbassador().getAttributeHandle(
          subscriberDerivedClass,
          options.knownClassDerivedAttributeName);
  require(
      ownerKnownAttribute.isValid() && ownerDerivedAttribute.isValid() &&
          subscriberKnownAttribute.isValid() && subscriberDerivedAttribute.isValid() &&
          ownerKnownAttribute == subscriberKnownAttribute &&
          ownerDerivedAttribute == subscriberDerivedAttribute,
      "known-class FOM attribute handles did not retain identity across members");

  rti::AttributeHandleSet const ownerKnownAttributes{ownerKnownAttribute};
  rti::AttributeHandleSet const ownerDerivedAttributes{ownerDerivedAttribute};
  rti::AttributeHandleSet const ownerPublishedAttributes{
      ownerKnownAttribute,
      ownerDerivedAttribute};
  rti::AttributeHandleSet const subscriberKnownAttributes{subscriberKnownAttribute};
  rti::AttributeHandleSet const subscriberDerivedAttributes{
      subscriberDerivedAttribute};

  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberBaseClass,
      subscriberKnownAttributes,
      true,
      L"");
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerDerivedClass,
      ownerPublishedAttributes);
  owner.recorder().clearAdvisories();

  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerDerivedClass);
  require(
      object.isValid(),
      "known-class attribute relevance registration returned an invalid object handle");
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().hasDiscovery(object); },
      options,
      "known-class attribute relevance discovery");
  auto const discovery = subscriber.recorder().discovery();
  require(
      discovery.object == object && discovery.objectClass == subscriberBaseClass,
      "known-class policy did not project discovery to the receiver's known class");

  waitFor(
      owner,
      [&] { return owner.recorder().updatesOn().size() >= 1U; },
      options,
      "known-class attribute relevance turn-updates-on advisory");
  auto const initialUpdatesOn = owner.recorder().updatesOn();
  require(
      initialUpdatesOn.size() == 1U &&
          initialUpdatesOn.front().object == object &&
          initialUpdatesOn.front().attributes == ownerKnownAttributes &&
          !initialUpdatesOn.front().updateRateDesignator.has_value(),
      "known-class policy did not limit the initial advisory to the known attribute");
  owner.recorder().clearAdvisories();

  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberDerivedClass,
      subscriberDerivedAttributes,
      true,
      L"");
  if (knownClassPolicyEnabled) {
    for (int pass = 0; pass != 8; ++pass) {
      owner.pump();
    }
    require(
        owner.recorder().updatesOn().empty() &&
            owner.recorder().updatesOff().empty(),
        "known-class policy exposed a derived-only advisory for the known instance");

    subscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
        subscriberDerivedClass,
        subscriberDerivedAttributes);
    for (int pass = 0; pass != 8; ++pass) {
      owner.pump();
    }
    require(
        owner.recorder().updatesOn().empty() &&
            owner.recorder().updatesOff().empty(),
        "removing a derived-only declaration changed known-class advisory state");
  } else {
    waitFor(
        owner,
        [&] { return owner.recorder().updatesOn().size() >= 1U; },
        options,
        "known-class-disabled derived-only turn-updates-on advisory");
    auto const derivedUpdatesOn = owner.recorder().updatesOn();
    require(
        derivedUpdatesOn.size() == 1U &&
            derivedUpdatesOn.front().object == object &&
            derivedUpdatesOn.front().attributes == ownerDerivedAttributes &&
            !derivedUpdatesOn.front().updateRateDesignator.has_value(),
        "disabled known-class policy did not expose the derived-only attribute");
    owner.recorder().clearAdvisories();

    subscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
        subscriberDerivedClass,
        subscriberDerivedAttributes);
    waitFor(
        owner,
        [&] { return owner.recorder().updatesOff().size() >= 1U; },
        options,
        "known-class-disabled derived-only turn-updates-off advisory");
    auto const derivedUpdatesOff = owner.recorder().updatesOff();
    require(
        derivedUpdatesOff.size() == 1U &&
            derivedUpdatesOff.front().object == object &&
            derivedUpdatesOff.front().attributes == ownerDerivedAttributes,
        "disabled known-class policy did not withdraw the derived-only attribute");
    owner.recorder().clearAdvisories();
  }

  subscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      subscriberBaseClass,
      subscriberKnownAttributes);
  waitFor(
      owner,
      [&] { return owner.recorder().updatesOff().size() >= 1U; },
      options,
      "known-class attribute relevance turn-updates-off advisory");
  auto const updatesOff = owner.recorder().updatesOff();
  require(
      updatesOff.size() == 1U &&
      updatesOff.front().object == object &&
          updatesOff.front().attributes == ownerKnownAttributes,
      "known-class policy did not preserve the base attribute advisory");

  subscriber.rtiAmbassador().unsubscribeObjectClass(subscriberBaseClass);
  owner.rtiAmbassador().unpublishObjectClassAttributes(
      ownerDerivedClass,
      ownerPublishedAttributes);
  owner.rtiAmbassador().unpublishObjectClass(ownerDerivedClass);
  owner.resign(rti::DELETE_OBJECTS);
  subscriber.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  subscriber.disconnect();
  owner.disconnect();
}

void scenarioKnownClassAttributeRelevance(
    Options const& options,
    rti::CallbackModel model) {
  runKnownClassAttributeRelevance(
      options,
      model,
      options.knownClassFom,
      true);
}

void scenarioKnownClassAttributeRelevanceDisabled(
    Options const& options,
    rti::CallbackModel model) {
  runKnownClassAttributeRelevance(
      options,
      model,
      options.knownClassDisabledFom,
      false);
}

void scenarioAttributeRelevanceRateReissue(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.rateFom.empty(),
      "Attribute relevance rate reissue testing requires an adapter-supplied rate advisory FOM");
  require(
      options.fomUpdateRateName != options.rateDesignatorName,
      "Attribute relevance rate reissue testing requires distinct fast and slow rate names");

  Session owner(options, model, "attribute-relevance-rate-owner");
  Session fastSubscriber(options, model, "attribute-relevance-rate-fast");
  Session slowSubscriber(options, model, "attribute-relevance-rate-slow");
  auto const federation = federationName(options, "attribute-relevance-rate-reissue");

  owner.connect();
  fastSubscriber.connect();
  slowSubscriber.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation,
      options.rateFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(
      options.ownerFederateName + L"-attribute-relevance-rate-owner",
      options.federateType,
      federation);
  fastSubscriber.join(
      options.memberFederateName + L"-attribute-relevance-rate-fast",
      options.federateType,
      federation);
  slowSubscriber.join(
      options.memberFederateName + L"-attribute-relevance-rate-slow",
      options.federateType,
      federation);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.rateObjectClassName);
  auto const fastClass = fastSubscriber.rtiAmbassador().getObjectClassHandle(
      options.rateObjectClassName);
  auto const slowClass = slowSubscriber.rtiAmbassador().getObjectClassHandle(
      options.rateObjectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.rateBestEffortAttributeName);
  auto const fastAttribute = fastSubscriber.rtiAmbassador().getAttributeHandle(
      fastClass,
      options.rateBestEffortAttributeName);
  auto const slowAttribute = slowSubscriber.rtiAmbassador().getAttributeHandle(
      slowClass,
      options.rateBestEffortAttributeName);
  require(
      ownerClass.isValid() && fastClass.isValid() && slowClass.isValid() &&
          ownerAttribute.isValid() && fastAttribute.isValid() &&
          slowAttribute.isValid() && ownerClass == fastClass &&
          ownerClass == slowClass && ownerAttribute == fastAttribute &&
          ownerAttribute == slowAttribute,
      "Attribute relevance rate reissue handles did not retain identity across members");

  auto const fastRate = fastSubscriber.rtiAmbassador().getUpdateRateValue(
      options.fomUpdateRateName);
  auto const slowRate = slowSubscriber.rtiAmbassador().getUpdateRateValue(
      options.rateDesignatorName);
  require(
      fastRate > slowRate && slowRate > 0.0,
      "Attribute relevance rate reissue adapter rates were not ordered and positive");
  require(
      owner.rtiAmbassador().getAttributeRelevanceAdvisorySwitch() &&
          fastSubscriber.rtiAmbassador().getAttributeRelevanceAdvisorySwitch() &&
          slowSubscriber.rtiAmbassador().getAttributeRelevanceAdvisorySwitch(),
      "Attribute relevance rate reissue adapter did not enable attribute advisories");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const fastAttributes{fastAttribute};
  rti::AttributeHandleSet const slowAttributes{slowAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "Attribute relevance rate reissue registration returned an invalid object handle");

  auto requireRateOn = [&](std::size_t expectedCount,
                           std::wstring const& expectedRate,
                           std::string const& description) {
    auto const updatesOn = owner.recorder().updatesOn();
    require(
        updatesOn.size() == expectedCount,
        description + " delivered an unexpected turn-updates-on count");
    require(
        updatesOn.back().object == object &&
            updatesOn.back().attributes == ownerAttributes &&
            updatesOn.back().updateRateDesignator.has_value() &&
            updatesOn.back().updateRateDesignator.value() == expectedRate,
        description + " delivered the wrong object, attributes, or rate designator");
    require(
        owner.recorder().updatesOff().empty(),
        description + " unexpectedly delivered turn-updates-off");
  };

  fastSubscriber.rtiAmbassador().subscribeObjectClassAttributes(
      fastClass,
      fastAttributes,
      true,
      options.fomUpdateRateName);
  waitFor(
      fastSubscriber,
      [&] { return fastSubscriber.recorder().hasDiscovery(object); },
      options,
      "Attribute relevance rate reissue fast subscriber discovery");
  waitFor(
      owner,
      [&] { return owner.recorder().updatesOn().size() >= 1U; },
      options,
      "Attribute relevance rate reissue initial advisory");
  requireRateOn(
      1U,
      options.fomUpdateRateName,
      "Attribute relevance rate reissue initial advisory");
  owner.recorder().clearAdvisories();

  slowSubscriber.rtiAmbassador().subscribeObjectClassAttributes(
      slowClass,
      slowAttributes,
      true,
      options.rateDesignatorName);
  waitFor(
      slowSubscriber,
      [&] { return slowSubscriber.recorder().hasDiscovery(object); },
      options,
      "Attribute relevance rate reissue slow subscriber discovery");
  for (int pass = 0; pass != 8; ++pass) {
    owner.pump();
  }
  require(
      owner.recorder().updatesOn().empty() && owner.recorder().updatesOff().empty(),
      "Adding a lower-rate subscriber unexpectedly reissued an advisory");

  fastSubscriber.rtiAmbassador().subscribeObjectClassAttributes(
      fastClass,
      fastAttributes,
      true,
      options.rateDesignatorName);
  waitFor(
      owner,
      [&] { return owner.recorder().updatesOn().size() >= 1U; },
      options,
      "Attribute relevance rate reissue lower-maximum advisory");
  requireRateOn(
      1U,
      options.rateDesignatorName,
      "Attribute relevance rate reissue lower-maximum advisory");
  owner.recorder().clearAdvisories();

  fastSubscriber.rtiAmbassador().subscribeObjectClassAttributes(
      fastClass,
      fastAttributes,
      true,
      options.fomUpdateRateName);
  waitFor(
      owner,
      [&] { return owner.recorder().updatesOn().size() >= 1U; },
      options,
      "Attribute relevance rate reissue restored-maximum advisory");
  requireRateOn(
      1U,
      options.fomUpdateRateName,
      "Attribute relevance rate reissue restored-maximum advisory");
  owner.recorder().clearAdvisories();

  fastSubscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      fastClass,
      fastAttributes);
  waitFor(
      owner,
      [&] { return owner.recorder().updatesOn().size() >= 1U; },
      options,
      "Attribute relevance rate reissue peer-removal advisory");
  requireRateOn(
      1U,
      options.rateDesignatorName,
      "Attribute relevance rate reissue peer-removal advisory");
  owner.recorder().clearAdvisories();

  slowSubscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      slowClass,
      slowAttributes);
  waitFor(
      owner,
      [&] { return owner.recorder().updatesOff().size() >= 1U; },
      options,
      "Attribute relevance rate reissue final advisory");
  auto const updatesOff = owner.recorder().updatesOff();
  require(
      updatesOff.size() == 1U && updatesOff.front().object == object &&
          updatesOff.front().attributes == ownerAttributes &&
          owner.recorder().updatesOn().empty(),
      "Attribute relevance rate reissue final turn-updates-off lost standard data");

  owner.rtiAmbassador().unpublishObjectClassAttributes(ownerClass, ownerAttributes);
  fastSubscriber.resign(rti::NO_ACTION);
  slowSubscriber.resign(rti::NO_ACTION);
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  fastSubscriber.disconnect();
  slowSubscriber.disconnect();
  owner.disconnect();
}

void scenarioAttributeRelevanceRateReissueContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAttributeRelevanceRateReissue(options, model);
}

void scenarioRelevanceAdvisories(Options const& options, rti::CallbackModel model) {
  Session switchLifecycle(options, model, "support-switch-lifecycle");
  auto checkBooleanSwitchLifecycle = [&](auto getter,
                                         auto setter,
                                         std::wstring const& expected,
                                         std::string const& phase,
                                         std::string const& description) {
    requireException(
        [&] { static_cast<void>(getter()); },
        expected,
        "getting the " + description + " " + phase);
    requireException(
        [&] { setter(false); },
        expected,
        "setting the " + description + " " + phase);
  };
  auto checkReadOnlySwitchLifecycle = [&](auto getter,
                                          std::wstring const& expected,
                                          std::string const& phase,
                                          std::string const& description) {
    requireException(
        [&] { static_cast<void>(getter()); },
        expected,
        "getting the " + description + " " + phase);
  };
  auto checkSwitchLifecycle = [&](std::wstring const& expected,
                                  std::string const& phase) {
    checkBooleanSwitchLifecycle(
        [&] { return switchLifecycle.rtiAmbassador().getObjectClassRelevanceAdvisorySwitch(); },
        [&](bool value) {
          switchLifecycle.rtiAmbassador().setObjectClassRelevanceAdvisorySwitch(value);
        },
        expected,
        phase,
        "object-class relevance advisory switch");
    checkBooleanSwitchLifecycle(
        [&] { return switchLifecycle.rtiAmbassador().getAttributeRelevanceAdvisorySwitch(); },
        [&](bool value) {
          switchLifecycle.rtiAmbassador().setAttributeRelevanceAdvisorySwitch(value);
        },
        expected,
        phase,
        "attribute relevance advisory switch");
    checkBooleanSwitchLifecycle(
        [&] { return switchLifecycle.rtiAmbassador().getAttributeScopeAdvisorySwitch(); },
        [&](bool value) {
          switchLifecycle.rtiAmbassador().setAttributeScopeAdvisorySwitch(value);
        },
        expected,
        phase,
        "attribute scope advisory switch");
    checkBooleanSwitchLifecycle(
        [&] { return switchLifecycle.rtiAmbassador().getInteractionRelevanceAdvisorySwitch(); },
        [&](bool value) {
          switchLifecycle.rtiAmbassador().setInteractionRelevanceAdvisorySwitch(value);
        },
        expected,
        phase,
        "interaction relevance advisory switch");
    checkBooleanSwitchLifecycle(
        [&] { return switchLifecycle.rtiAmbassador().getConveyRegionDesignatorSetsSwitch(); },
        [&](bool value) {
          switchLifecycle.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(value);
        },
        expected,
        phase,
        "convey-region-designator-sets switch");
    requireException(
        [&] {
          static_cast<void>(switchLifecycle.rtiAmbassador().getAutomaticResignDirective());
        },
        expected,
        "getting the automatic resign directive " + phase);
    requireException(
        [&] {
          switchLifecycle.rtiAmbassador().setAutomaticResignDirective(rti::NO_ACTION);
        },
        expected,
        "setting the automatic resign directive " + phase);
    checkBooleanSwitchLifecycle(
        [&] { return switchLifecycle.rtiAmbassador().getServiceReportingSwitch(); },
        [&](bool value) { switchLifecycle.rtiAmbassador().setServiceReportingSwitch(value); },
        expected,
        phase,
        "service-reporting switch");
    checkBooleanSwitchLifecycle(
        [&] { return switchLifecycle.rtiAmbassador().getExceptionReportingSwitch(); },
        [&](bool value) { switchLifecycle.rtiAmbassador().setExceptionReportingSwitch(value); },
        expected,
        phase,
        "exception-reporting switch");
    checkBooleanSwitchLifecycle(
        [&] { return switchLifecycle.rtiAmbassador().getSendServiceReportsToFileSwitch(); },
        [&](bool value) {
          switchLifecycle.rtiAmbassador().setSendServiceReportsToFileSwitch(value);
        },
        expected,
        phase,
        "send-service-reports-to-file switch");
    checkReadOnlySwitchLifecycle(
        [&] { return switchLifecycle.rtiAmbassador().getAutoProvideSwitch(); },
        expected,
        phase,
        "auto-provide switch");
    checkReadOnlySwitchLifecycle(
        [&] { return switchLifecycle.rtiAmbassador().getDelaySubscriptionEvaluationSwitch(); },
        expected,
        phase,
        "delay-subscription-evaluation switch");
    checkReadOnlySwitchLifecycle(
        [&] { return switchLifecycle.rtiAmbassador().getAdvisoriesUseKnownClassSwitch(); },
        expected,
        phase,
        "advisories-use-known-class switch");
    checkReadOnlySwitchLifecycle(
        [&] { return switchLifecycle.rtiAmbassador().getAllowRelaxedDDMSwitch(); },
        expected,
        phase,
        "allow-relaxed-DDM switch");
    checkReadOnlySwitchLifecycle(
        [&] { return switchLifecycle.rtiAmbassador().getNonRegulatedGrantSwitch(); },
        expected,
        phase,
        "non-regulated-grant switch");
  };
  checkSwitchLifecycle(L"NotConnected", "before connect");
  switchLifecycle.connect();
  checkSwitchLifecycle(
      L"FederateNotExecutionMember",
      "after connect before joining an execution");
  switchLifecycle.disconnect();

  Session publisher(options, model, "owner");
  Session subscriber(options, model, "member");
  auto const federation = federationName(options, "relevance-advisories");
  connectAndJoin(publisher, subscriber, options, federation, options.fom);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const subscriberClass = subscriber.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherName = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass, L"Name");
  auto const publisherPayRate = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass, L"PayRate");
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass, options.attributeName);
  auto const subscriberName = subscriber.rtiAmbassador().getAttributeHandle(
      subscriberClass, L"Name");
  auto const subscriberPayRate = subscriber.rtiAmbassador().getAttributeHandle(
      subscriberClass, L"PayRate");
  auto const subscriberAttribute = subscriber.rtiAmbassador().getAttributeHandle(
      subscriberClass, options.attributeName);
  require(publisherClass.isValid() && subscriberClass.isValid() &&
              publisherName.isValid() && publisherPayRate.isValid() &&
              publisherAttribute.isValid() && subscriberName.isValid() &&
              subscriberPayRate.isValid() && subscriberAttribute.isValid(),
          "Relevance advisory attribute lookup returned an invalid handle");

  rti::AttributeHandleSet publisherNameSet;
  publisherNameSet.insert(publisherName);
  rti::AttributeHandleSet publisherAllSet;
  publisherAllSet.insert(publisherName);
  publisherAllSet.insert(publisherPayRate);
  publisherAllSet.insert(publisherAttribute);
  rti::AttributeHandleSet subscriberNameSet;
  subscriberNameSet.insert(subscriberName);
  rti::AttributeHandleSet subscriberPayRateSet;
  subscriberPayRateSet.insert(subscriberPayRate);
  rti::AttributeHandleSet subscriberAttributeSet;
  subscriberAttributeSet.insert(subscriberAttribute);

  auto const publisherInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const subscriberInteraction = subscriber.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  require(publisherInteraction.isValid() && subscriberInteraction.isValid(),
          "Relevance advisory interaction lookup returned an invalid handle");

  auto roundTripBooleanSwitch = [&](auto getter, auto setter,
                                    std::string const& description) {
    auto const original = getter();
    setter(false);
    require(!getter(), description + " did not turn off");
    setter(true);
    require(getter(), description + " did not turn on");
    setter(original);
    require(getter() == original, description + " did not restore its state");
  };
  roundTripBooleanSwitch(
      [&] { return publisher.rtiAmbassador().getObjectClassRelevanceAdvisorySwitch(); },
      [&](bool value) {
        publisher.rtiAmbassador().setObjectClassRelevanceAdvisorySwitch(value);
      },
      "object-class relevance advisory switch");
  roundTripBooleanSwitch(
      [&] { return publisher.rtiAmbassador().getAttributeRelevanceAdvisorySwitch(); },
      [&](bool value) {
        publisher.rtiAmbassador().setAttributeRelevanceAdvisorySwitch(value);
      },
      "attribute relevance advisory switch");
  roundTripBooleanSwitch(
      [&] { return publisher.rtiAmbassador().getAttributeScopeAdvisorySwitch(); },
      [&](bool value) {
        publisher.rtiAmbassador().setAttributeScopeAdvisorySwitch(value);
      },
      "attribute scope advisory switch");
  roundTripBooleanSwitch(
      [&] { return publisher.rtiAmbassador().getInteractionRelevanceAdvisorySwitch(); },
      [&](bool value) {
        publisher.rtiAmbassador().setInteractionRelevanceAdvisorySwitch(value);
      },
      "interaction relevance advisory switch");
  roundTripBooleanSwitch(
      [&] { return publisher.rtiAmbassador().getConveyRegionDesignatorSetsSwitch(); },
      [&](bool value) {
        publisher.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(value);
      },
      "convey-region-designator-sets switch");
  roundTripBooleanSwitch(
      [&] { return publisher.rtiAmbassador().getServiceReportingSwitch(); },
      [&](bool value) { publisher.rtiAmbassador().setServiceReportingSwitch(value); },
      "service-reporting switch");
  roundTripBooleanSwitch(
      [&] { return publisher.rtiAmbassador().getExceptionReportingSwitch(); },
      [&](bool value) { publisher.rtiAmbassador().setExceptionReportingSwitch(value); },
      "exception-reporting switch");
  roundTripBooleanSwitch(
      [&] { return publisher.rtiAmbassador().getSendServiceReportsToFileSwitch(); },
      [&](bool value) {
        publisher.rtiAmbassador().setSendServiceReportsToFileSwitch(value);
      },
      "send-service-reports-to-file switch");
  auto checkReadOnlySwitch = [&](auto getter, std::string const& description) {
    auto const first = getter();
    require(getter() == first, description + " was not stable across queries");
  };
  checkReadOnlySwitch(
      [&] { return publisher.rtiAmbassador().getAutoProvideSwitch(); },
      "auto-provide switch");
  checkReadOnlySwitch(
      [&] { return publisher.rtiAmbassador().getDelaySubscriptionEvaluationSwitch(); },
      "delay-subscription-evaluation switch");
  checkReadOnlySwitch(
      [&] { return publisher.rtiAmbassador().getAdvisoriesUseKnownClassSwitch(); },
      "advisories-use-known-class switch");
  checkReadOnlySwitch(
      [&] { return publisher.rtiAmbassador().getAllowRelaxedDDMSwitch(); },
      "allow-relaxed-DDM switch");
  checkReadOnlySwitch(
      [&] { return publisher.rtiAmbassador().getNonRegulatedGrantSwitch(); },
      "non-regulated-grant switch");

  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass, subscriberNameSet, true, L"");
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass, publisherAllSet);
  waitFor(publisher,
          [&] { return publisher.recorder().registrationStarts().size() >= 1U; },
          options,
          "active object registration relevance advisory");
  auto const registrationStarts = publisher.recorder().registrationStarts();
  require(registrationStarts.size() == 1U && registrationStarts.at(0) == publisherClass,
          "active object subscription did not produce one start-registration advisory");
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass, publisherAllSet);
  for (int pass = 0; pass != 8; ++pass) {
    publisher.pump();
  }
  require(publisher.recorder().registrationStarts().size() == 1U,
          "repeated object publication repeated start-registration advisory");

  subscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      subscriberClass, subscriberNameSet);
  waitFor(publisher,
          [&] { return publisher.recorder().registrationStops().size() >= 1U; },
          options,
          "object registration relevance stop advisory");
  require(publisher.recorder().registrationStops().size() == 1U &&
              publisher.recorder().registrationStops().at(0) == publisherClass,
          "removing active object subscription did not produce one stop-registration advisory");

  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass, subscriberNameSet, false, L"");
  for (int pass = 0; pass != 8; ++pass) {
    publisher.pump();
  }
  require(publisher.recorder().registrationStarts().size() == 1U &&
              publisher.recorder().registrationStops().size() == 1U,
          "passive object subscription changed registration relevance");
  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass, subscriberNameSet, true, L"");
  waitFor(publisher,
          [&] { return publisher.recorder().registrationStarts().size() >= 2U; },
          options,
          "active object subscription promotion");
  subscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      subscriberClass, subscriberNameSet);
  waitFor(publisher,
          [&] { return publisher.recorder().registrationStops().size() >= 2U; },
          options,
          "active object subscription demotion");
  subscriber.rtiAmbassador().unsubscribeObjectClass(subscriberClass);

  subscriber.rtiAmbassador().subscribeInteractionClass(subscriberInteraction, true);
  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  waitFor(publisher,
          [&] { return publisher.recorder().interactionsOn().size() >= 1U; },
          options,
          "active interaction relevance advisory");
  require(publisher.recorder().interactionsOn().size() == 1U &&
              publisher.recorder().interactionsOn().at(0) == publisherInteraction,
          "active interaction subscription did not produce one turn-on advisory");
  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  for (int pass = 0; pass != 8; ++pass) {
    publisher.pump();
  }
  require(publisher.recorder().interactionsOn().size() == 1U,
          "repeated interaction publication repeated turn-on advisory");
  subscriber.rtiAmbassador().unsubscribeInteractionClass(subscriberInteraction);
  waitFor(publisher,
          [&] { return publisher.recorder().interactionsOff().size() >= 1U; },
          options,
          "interaction relevance stop advisory");
  require(publisher.recorder().interactionsOff().size() == 1U &&
              publisher.recorder().interactionsOff().at(0) == publisherInteraction,
          "removing active interaction subscription did not produce one turn-off advisory");
  subscriber.rtiAmbassador().subscribeInteractionClass(subscriberInteraction, false);
  for (int pass = 0; pass != 8; ++pass) {
    publisher.pump();
  }
  require(publisher.recorder().interactionsOn().size() == 1U,
          "passive interaction subscription changed interaction relevance");
  subscriber.rtiAmbassador().subscribeInteractionClass(subscriberInteraction, true);
  waitFor(publisher,
          [&] { return publisher.recorder().interactionsOn().size() >= 2U; },
          options,
          "active interaction subscription promotion");
  subscriber.rtiAmbassador().unsubscribeInteractionClass(subscriberInteraction);
  waitFor(publisher,
          [&] { return publisher.recorder().interactionsOff().size() >= 2U; },
          options,
          "active interaction subscription demotion");
  subscriber.rtiAmbassador().unsubscribeInteractionClass(subscriberInteraction);

  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass, subscriberNameSet, true, L"");
  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(object.isValid(), "Relevance advisory registration returned an invalid object handle");
  waitFor(subscriber,
          [&] { return subscriber.recorder().hasDiscovery(object); },
          options,
          "active object subscriber discovery");
  for (int pass = 0; pass != 8; ++pass) {
    publisher.pump();
  }
  publisher.recorder().clearAdvisories();

  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass, subscriberPayRateSet, true, L"");
  waitFor(publisher,
          [&] { return publisher.recorder().updatesOn().size() >= 1U; },
          options,
          "unnamed attribute turn-updates-on advisory");
  auto const unnamedUpdatesOn = publisher.recorder().updatesOn();
  require(unnamedUpdatesOn.size() == 1U && unnamedUpdatesOn.at(0).object == object &&
              unnamedUpdatesOn.at(0).attributes.count(publisherPayRate) == 1U &&
              !unnamedUpdatesOn.at(0).updateRateDesignator.has_value(),
          "unnamed turn-updates-on advisory did not preserve instance and attributes");

  subscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      subscriberClass, subscriberPayRateSet);
  waitFor(publisher,
          [&] { return publisher.recorder().updatesOff().size() >= 1U; },
          options,
          "attribute turn-updates-off advisory");
  auto const unnamedUpdatesOff = publisher.recorder().updatesOff();
  require(unnamedUpdatesOff.size() == 1U && unnamedUpdatesOff.at(0).object == object &&
              unnamedUpdatesOff.at(0).attributes.count(publisherPayRate) == 1U,
          "turn-updates-off advisory did not preserve instance and attributes");

  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass, subscriberAttributeSet, true, L"High");
  waitFor(publisher,
          [&] { return publisher.recorder().updatesOn().size() >= 2U; },
          options,
          "named attribute turn-updates-on advisory");
  auto const namedUpdatesOn = publisher.recorder().updatesOn();
  require(namedUpdatesOn.size() == 2U && namedUpdatesOn.at(1).object == object &&
              namedUpdatesOn.at(1).attributes.count(publisherAttribute) == 1U &&
              namedUpdatesOn.at(1).updateRateDesignator.has_value() &&
              namedUpdatesOn.at(1).updateRateDesignator.value() == L"High",
          "named turn-updates-on advisory did not preserve its update rate");
  require(
      subscriber.rtiAmbassador().getUpdateRateValue(L"High") == 1.0,
      "named update-rate lookup did not return the configured FOM value");
  require(
      subscriber.rtiAmbassador().getUpdateRateValueForAttribute(
          object, subscriberAttribute) == 1.0,
      "attribute update-rate lookup did not return the active subscription rate");
  subscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      subscriberClass, subscriberAttributeSet);
  waitFor(publisher,
          [&] { return publisher.recorder().updatesOff().size() >= 2U; },
          options,
          "named attribute turn-updates-off advisory");

  subscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      subscriberClass, subscriberNameSet);
  subscriber.rtiAmbassador().unsubscribeObjectClass(subscriberClass);
  publisher.rtiAmbassador().unpublishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(publisherClass, publisherAllSet);
  publisher.rtiAmbassador().unpublishObjectClass(publisherClass);
  publisher.resign(rti::DELETE_OBJECTS);
  subscriber.resign(rti::NO_ACTION);
}

}  // namespace hla_rti_cpp_tck::detail
