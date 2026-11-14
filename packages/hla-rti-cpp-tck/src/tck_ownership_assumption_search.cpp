#include "tck_main_scenarios.hpp"

#include <RTI/NullFederateAmbassador.h>

#include <chrono>
#include <atomic>
#include <condition_variable>
#include <exception>
#include <functional>
#include <mutex>
#include <thread>

namespace hla_rti_cpp_tck::detail {
namespace {

struct AssumptionEvent {
  rti::ObjectInstanceHandle object;
  rti::AttributeHandleSet attributes;
  std::vector<std::uint8_t> tag;
};

class AssumptionCallbacks final : public rti::NullFederateAmbassador {
 public:
  void discoverObjectInstance(
      rti::ObjectInstanceHandle const& object,
      rti::ObjectClassHandle const&,
      std::wstring const&,
      rti::FederateHandle const&) override {
    std::lock_guard<std::mutex> lock(mutex_);
    discovered_.push_back(object);
  }

  void requestAttributeOwnershipAssumption(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleSet const& attributes,
      rti::VariableLengthData const& tag) override {
    std::function<void()> hook;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      assumptions_.push_back(AssumptionEvent{object, attributes, copyBytes(tag)});
      hook = assumptionHook_;
    }
    if (hook) hook();
  }

  void confirmAttributeOwnershipAcquisitionCancellation(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleSet const& attributes) override {
    std::lock_guard<std::mutex> lock(mutex_);
    cancellations_.push_back(AssumptionEvent{object, attributes, {}});
  }

  void attributeOwnershipAcquisitionNotification(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleSet const& attributes,
      rti::VariableLengthData const& tag) override {
    std::lock_guard<std::mutex> lock(mutex_);
    acquisitions_.push_back(AssumptionEvent{object, attributes, copyBytes(tag)});
  }

  void requestDivestitureConfirmation(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleSet const& attributes,
      rti::VariableLengthData const& tag) override {
    std::function<void()> hook;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      divestitureRequests_.push_back(AssumptionEvent{object, attributes, copyBytes(tag)});
      hook = divestitureHook_;
    }
    if (hook) {
      try {
        hook();
      } catch (...) {
        std::lock_guard<std::mutex> lock(mutex_);
        divestitureFailure_ = std::current_exception();
      }
    }
  }

  void requestAttributeOwnershipRelease(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleSet const& attributes,
      rti::VariableLengthData const& tag) override {
    std::lock_guard<std::mutex> lock(mutex_);
    releaseRequests_.push_back(AssumptionEvent{object, attributes, copyBytes(tag)});
  }

  void attributeOwnershipUnavailable(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleSet const& attributes,
      rti::VariableLengthData const& tag) override {
    std::lock_guard<std::mutex> lock(mutex_);
    unavailable_.push_back(AssumptionEvent{object, attributes, copyBytes(tag)});
  }

  bool discovered(rti::ObjectInstanceHandle const& object) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return std::find(discovered_.begin(), discovered_.end(), object) != discovered_.end();
  }

  std::vector<AssumptionEvent> assumptions() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return assumptions_;
  }

  std::vector<AssumptionEvent> cancellations() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return cancellations_;
  }

  std::vector<AssumptionEvent> acquisitions() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return acquisitions_;
  }

  std::vector<AssumptionEvent> divestitureRequests() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return divestitureRequests_;
  }

  std::vector<AssumptionEvent> releaseRequests() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return releaseRequests_;
  }

  std::vector<AssumptionEvent> unavailable() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return unavailable_;
  }

  std::exception_ptr divestitureFailure() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return divestitureFailure_;
  }

  void setAssumptionHook(std::function<void()> hook) {
    std::lock_guard<std::mutex> lock(mutex_);
    assumptionHook_ = std::move(hook);
  }

  void setDivestitureHook(std::function<void()> hook) {
    std::lock_guard<std::mutex> lock(mutex_);
    divestitureHook_ = std::move(hook);
  }

 private:
  mutable std::mutex mutex_;
  std::vector<rti::ObjectInstanceHandle> discovered_;
  std::vector<AssumptionEvent> assumptions_;
  std::vector<AssumptionEvent> cancellations_;
  std::vector<AssumptionEvent> acquisitions_;
  std::vector<AssumptionEvent> divestitureRequests_;
  std::vector<AssumptionEvent> releaseRequests_;
  std::vector<AssumptionEvent> unavailable_;
  std::function<void()> assumptionHook_;
  std::function<void()> divestitureHook_;
  std::exception_ptr divestitureFailure_;
};

void connect(
    rti::RTIambassador& ambassador,
    AssumptionCallbacks& callbacks,
    Options const& options,
    rti::CallbackModel model,
    bool owner) {
  auto configurationName = options.configurationName;
  if (owner && !options.ownerConfigurationName.empty()) {
    configurationName = options.ownerConfigurationName;
  } else if (!owner && !options.memberConfigurationName.empty()) {
    configurationName = options.memberConfigurationName;
  }
  if (configurationName.empty() && options.rtiAddress.empty() &&
      options.additionalSettings.empty()) {
    static_cast<void>(ambassador.connect(callbacks, model));
    return;
  }
  auto configuration = rti::RtiConfiguration::createConfiguration();
  if (!configurationName.empty()) {
    configuration.withConfigurationName(toWide(configurationName));
  }
  if (!options.rtiAddress.empty()) {
    configuration.withRtiAddress(toWide(options.rtiAddress));
  }
  if (!options.additionalSettings.empty()) {
    configuration.withAdditionalSettings(toWide(options.additionalSettings));
  }
  static_cast<void>(ambassador.connect(callbacks, model, configuration));
}

template <typename Predicate>
void evokeUntil(
    rti::RTIambassador& ambassador,
    Predicate condition,
    Options const& options,
    rti::CallbackModel model,
    char const* description) {
  auto const deadline = std::chrono::steady_clock::now() +
                        std::chrono::milliseconds(options.timeoutMilliseconds);
  while (!condition() && std::chrono::steady_clock::now() < deadline) {
    if (model == rti::HLA_EVOKED) {
      static_cast<void>(ambassador.evokeCallback(0.0));
    } else {
      std::this_thread::yield();
    }
  }
  require(condition(), std::string("Timed out waiting for ") + description);
}

}  // namespace

void scenarioOwnershipAssumptionSearchAfterDecline(
    Options const& options,
    rti::CallbackModel model) {
  require(!options.fom.empty(),
          "Ownership-assumption search requires an adapter-supplied FOM");

  rti::RTIambassadorFactory factory;
  auto owner = factory.createRTIambassador();
  auto first = factory.createRTIambassador();
  auto second = factory.createRTIambassador();
  auto willing = factory.createRTIambassador();
  require(owner && first && second && willing,
          "The standard RTIambassador factory returned no ambassador");
  AssumptionCallbacks ownerCallbacks;
  AssumptionCallbacks firstCallbacks;
  AssumptionCallbacks secondCallbacks;
  AssumptionCallbacks willingCallbacks;
  connect(*owner, ownerCallbacks, options, model, true);
  connect(*first, firstCallbacks, options, model, false);
  connect(*second, secondCallbacks, options, model, false);
  connect(*willing, willingCallbacks, options, model, false);
  auto const federation = federationName(options, "assumption-search-after-decline");
  owner->createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  static_cast<void>(owner->joinFederationExecution(
      options.ownerFederateName + L"-assumption-search-owner",
      options.federateType, federation));
  static_cast<void>(first->joinFederationExecution(
      options.memberFederateName + L"-assumption-search-first",
      options.federateType, federation));
  static_cast<void>(second->joinFederationExecution(
      options.memberFederateName + L"-assumption-search-second",
      options.federateType, federation));
  static_cast<void>(willing->joinFederationExecution(
      options.memberFederateName + L"-assumption-search-willing",
      options.federateType, federation));

  auto const ownerClass = owner->getObjectClassHandle(options.objectClassName);
  auto const firstClass = first->getObjectClassHandle(options.objectClassName);
  auto const secondClass = second->getObjectClassHandle(options.objectClassName);
  auto const willingClass = willing->getObjectClassHandle(options.objectClassName);
  auto const ownerAttribute = owner->getAttributeHandle(ownerClass, options.attributeName);
  auto const firstAttribute = first->getAttributeHandle(firstClass, options.attributeName);
  auto const secondAttribute = second->getAttributeHandle(secondClass, options.attributeName);
  auto const willingAttribute = willing->getAttributeHandle(willingClass, options.attributeName);
  require(ownerClass.isValid() && firstClass.isValid() && secondClass.isValid() &&
              willingClass.isValid() &&
              ownerAttribute.isValid() && firstAttribute.isValid() &&
              secondAttribute.isValid() && willingAttribute.isValid(),
          "Ownership-assumption search lookup returned an invalid standard handle");
  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const firstAttributes{firstAttribute};
  rti::AttributeHandleSet const secondAttributes{secondAttribute};
  rti::AttributeHandleSet const willingAttributes{willingAttribute};
  owner->publishObjectClassAttributes(ownerClass, ownerAttributes);
  first->subscribeObjectClassAttributes(firstClass, firstAttributes, true, L"");
  second->subscribeObjectClassAttributes(secondClass, secondAttributes, true, L"");
  willing->subscribeObjectClassAttributes(willingClass, willingAttributes, true, L"");
  auto const object = owner->registerObjectInstance(ownerClass);
  require(object.isValid(), "Ownership-assumption search registration returned an invalid handle");
  evokeUntil(*first, [&] { return firstCallbacks.discovered(object); }, options, model,
             "first assumption candidate discovery");
  evokeUntil(*second, [&] { return secondCallbacks.discovered(object); }, options, model,
             "second assumption candidate discovery");
  evokeUntil(*willing, [&] { return willingCallbacks.discovered(object); }, options, model,
             "Willing-to-Acquire assumption candidate discovery");
  first->publishObjectClassAttributes(firstClass, firstAttributes);
  second->publishObjectClassAttributes(secondClass, secondAttributes);
  willing->publishObjectClassAttributes(willingClass, willingAttributes);

  std::vector<std::uint8_t> const acquisitionTagBytes{0xC1U, 0x25U};
  std::vector<std::uint8_t> const divestitureTagBytes{0xC2U, 0x25U};
  rti::VariableLengthData const acquisitionTag(
      acquisitionTagBytes.data(), acquisitionTagBytes.size());
  rti::VariableLengthData const divestitureTag(
      divestitureTagBytes.data(), divestitureTagBytes.size());
  if (model == rti::HLA_IMMEDIATE) {
    second->disableCallbacks();
  }
  second->attributeOwnershipAcquisition(object, secondAttributes, acquisitionTag);
  second->cancelAttributeOwnershipAcquisition(object, secondAttributes);
  // Hold the If-Available result pending across divestiture so this candidate
  // remains in the public API's Willing-to-Acquire state at the §4.12 search.
  if (model == rti::HLA_IMMEDIATE) {
    willing->disableCallbacks();
  }
  willing->attributeOwnershipAcquisitionIfAvailable(
      object, willingAttributes, acquisitionTag);

  std::mutex coordinationMutex;
  std::condition_variable coordination;
  bool firstEntered = false;
  bool cancellationDelivered = false;
  std::atomic<bool> firstHookFinished{false};
  std::atomic<bool> secondAssumptionBeforeFirstReturn{false};
  std::exception_ptr secondThreadException;
  firstCallbacks.setAssumptionHook([&] {
    {
      std::lock_guard<std::mutex> lock(coordinationMutex);
      firstEntered = true;
    }
    coordination.notify_all();
    std::unique_lock<std::mutex> lock(coordinationMutex);
    coordination.wait_for(lock, std::chrono::seconds(5),
                          [&] { return cancellationDelivered; });
    secondAssumptionBeforeFirstReturn.store(!secondCallbacks.assumptions().empty());
    firstHookFinished.store(true);
  });
  std::thread secondCallbackThread([&] {
    try {
      {
        std::unique_lock<std::mutex> lock(coordinationMutex);
        if (!coordination.wait_for(lock, std::chrono::seconds(5),
                                   [&] { return firstEntered; })) {
          throw std::runtime_error("First assumption callback did not enter");
        }
      }
      if (model == rti::HLA_IMMEDIATE) {
        second->enableCallbacks();
      }
      evokeUntil(*second, [&] { return !secondCallbacks.cancellations().empty(); },
                 options, model, "second candidate cancellation confirmation");
    } catch (...) {
      secondThreadException = std::current_exception();
    }
    {
      std::lock_guard<std::mutex> lock(coordinationMutex);
      cancellationDelivered = true;
    }
    coordination.notify_all();
  });
  std::exception_ptr firstCallbackException;
  try {
    owner->unconditionalAttributeOwnershipDivestiture(
        object, ownerAttributes, divestitureTag);
    if (model == rti::HLA_EVOKED) {
      require(secondCallbacks.assumptions().empty(),
              "Second candidate received an assumption before cancellation confirmation");
    }
    evokeUntil(*first, [&] { return firstHookFinished.load(); },
               options, model, "first ownership-assumption callback return");
  } catch (...) {
    firstCallbackException = std::current_exception();
  }
  secondCallbackThread.join();
  firstCallbacks.setAssumptionHook({});
  if (firstCallbackException) std::rethrow_exception(firstCallbackException);
  if (secondThreadException) std::rethrow_exception(secondThreadException);
  require(firstEntered && cancellationDelivered && firstHookFinished.load() &&
              !secondAssumptionBeforeFirstReturn.load() &&
              firstCallbacks.assumptions().size() == 1U &&
              secondCallbacks.cancellations().size() == 1U &&
              (model == rti::HLA_IMMEDIATE || secondCallbacks.assumptions().empty()),
          "Callback-boundary cancellation or first assumption occurred out of order");

  evokeUntil(*second, [&] { return !secondCallbacks.assumptions().empty(); },
             options, model, "continued ownership assumption after declined callback");
  auto const continued = secondCallbacks.assumptions();
  require(continued.size() == 1U && continued.front().object == object &&
              continued.front().attributes == secondAttributes &&
              continued.front().tag == divestitureTagBytes,
          "Continued assumption lost its object, attribute, or tag");

  if (model == rti::HLA_IMMEDIATE) {
    willing->enableCallbacks();
  }
  evokeUntil(*willing,
             [&] {
               return !willingCallbacks.assumptions().empty() ||
                   !willingCallbacks.unavailable().empty() ||
                   !willingCallbacks.acquisitions().empty();
             },
             options, model, "Willing-to-Acquire ownership result");
  if (model == rti::HLA_EVOKED) {
    while (willing->evokeCallback(0.0)) {
    }
  }
  require(willingCallbacks.assumptions().empty(),
          "RTI invoked Request Attribute Ownership Assumption at a federate already in the Willing-to-Acquire state");

  willing->unpublishObjectClass(willingClass);
  first->unpublishObjectClass(firstClass);
  second->unpublishObjectClass(secondClass);
  owner->resignFederationExecution(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  first->resignFederationExecution(rti::NO_ACTION);
  second->resignFederationExecution(rti::NO_ACTION);
  willing->resignFederationExecution(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  second->destroyFederationExecution(federation);
  willing->disconnect();
  second->disconnect();
  first->disconnect();
  owner->disconnect();
}

void scenarioOwnershipAssumptionSearchAfterDeclineContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioOwnershipAssumptionSearchAfterDecline(options, model);
}

void scenarioOwnershipAssumptionTransferEpoch(
    Options const& options,
    rti::CallbackModel model) {
  require(!options.fom.empty(), "Transfer-epoch search requires an adapter-supplied FOM");
  rti::RTIambassadorFactory factory;
  auto owner = factory.createRTIambassador();
  auto first = factory.createRTIambassador();
  auto second = factory.createRTIambassador();
  require(owner && first && second, "The standard factory returned no ambassador");
  AssumptionCallbacks ownerCallbacks;
  AssumptionCallbacks firstCallbacks;
  AssumptionCallbacks secondCallbacks;
  connect(*owner, ownerCallbacks, options, model, true);
  connect(*first, firstCallbacks, options, model, false);
  connect(*second, secondCallbacks, options, model, false);
  auto const federation = federationName(options, "assumption-transfer-epoch");
  owner->createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  static_cast<void>(owner->joinFederationExecution(
      options.ownerFederateName + L"-epoch-owner", options.federateType, federation));
  static_cast<void>(first->joinFederationExecution(
      options.memberFederateName + L"-epoch-first", options.federateType, federation));
  static_cast<void>(second->joinFederationExecution(
      options.memberFederateName + L"-epoch-second", options.federateType, federation));

  auto const ownerClass = owner->getObjectClassHandle(options.objectClassName);
  auto const firstClass = first->getObjectClassHandle(options.objectClassName);
  auto const secondClass = second->getObjectClassHandle(options.objectClassName);
  auto const ownerAttribute = owner->getAttributeHandle(ownerClass, options.attributeName);
  auto const firstAttribute = first->getAttributeHandle(firstClass, options.attributeName);
  auto const secondAttribute = second->getAttributeHandle(secondClass, options.attributeName);
  auto const privilege = owner->getAttributeHandle(
      ownerClass, L"HLAprivilegeToDeleteObject");
  require(ownerClass.isValid() && firstClass.isValid() && secondClass.isValid() &&
              ownerAttribute.isValid() && firstAttribute.isValid() &&
              secondAttribute.isValid() && privilege.isValid(),
          "Transfer-epoch lookup returned an invalid standard handle");
  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const firstAttributes{firstAttribute};
  rti::AttributeHandleSet const secondAttributes{secondAttribute};
  rti::AttributeHandleSet const transferred{ownerAttribute, privilege};
  owner->publishObjectClassAttributes(ownerClass, ownerAttributes);
  first->subscribeObjectClassAttributes(firstClass, firstAttributes, true, L"");
  second->subscribeObjectClassAttributes(secondClass, secondAttributes, true, L"");
  auto const object = owner->registerObjectInstance(ownerClass);
  require(object.isValid(), "Transfer-epoch registration returned an invalid handle");
  evokeUntil(*first, [&] { return firstCallbacks.discovered(object); }, options, model,
             "first candidate discovery");
  evokeUntil(*second, [&] { return secondCallbacks.discovered(object); }, options, model,
             "second candidate discovery");
  first->publishObjectClassAttributes(firstClass, firstAttributes);
  second->publishObjectClassAttributes(secondClass, secondAttributes);

  std::vector<std::uint8_t> const firstTagBytes{0xE1U, 0x25U};
  std::vector<std::uint8_t> const secondTagBytes{0xE2U, 0x25U};
  std::vector<std::uint8_t> const acquisitionTagBytes{0xE3U, 0x25U};
  rti::VariableLengthData const firstTag(firstTagBytes.data(), firstTagBytes.size());
  rti::VariableLengthData const secondTag(secondTagBytes.data(), secondTagBytes.size());
  rti::VariableLengthData const acquisitionTag(
      acquisitionTagBytes.data(), acquisitionTagBytes.size());
  owner->unconditionalAttributeOwnershipDivestiture(object, transferred, firstTag);
  require(
      !owner->isAttributeOwnedByFederate(object, ownerAttribute),
      "Unconditionally divested instance attribute remained owned by the divesting federate");
  evokeUntil(*first, [&] { return firstCallbacks.assumptions().size() == 1U; },
             options, model, "first transfer assumption");
  evokeUntil(*second, [&] { return secondCallbacks.assumptions().size() == 1U; },
             options, model, "second transfer assumption");
  auto const initialFirst = firstCallbacks.assumptions().front();
  auto const initialSecond = secondCallbacks.assumptions().front();
  require(initialFirst.object == object && initialSecond.object == object &&
              initialFirst.attributes == transferred &&
              initialSecond.attributes == transferred &&
              initialFirst.tag == firstTagBytes && initialSecond.tag == firstTagBytes,
          "Initial ownership search did not offer both candidates the original tag");
  require(
      !first->isAttributeOwnedByFederate(object, firstAttribute) &&
          !second->isAttributeOwnedByFederate(object, secondAttribute),
      "Ownership assumption offers transferred the attribute without an explicit acquisition service");

  first->attributeOwnershipAcquisition(object, transferred, acquisitionTag);
  evokeUntil(*first, [&] { return firstCallbacks.acquisitions().size() == 1U; },
             options, model, "first candidate acquisition notification");
  auto const acquisition = firstCallbacks.acquisitions().front();
  require(acquisition.object == object && acquisition.attributes == transferred &&
              first->isAttributeOwnedByFederate(object, firstAttribute),
          "First candidate did not acquire the transferred attribute set");

  if (model == rti::HLA_EVOKED) {
    while (second->evokeCallback(0.0)) {
    }
  }
  require(secondCallbacks.assumptions().size() == 1U &&
              first->isAttributeOwnedByFederate(object, firstAttribute),
          "RTI issued an additional assumption after the attribute became owned");

  second->unpublishObjectClass(secondClass);
  owner->resignFederationExecution(rti::NO_ACTION);
  first->unconditionalAttributeOwnershipDivestiture(object, transferred, secondTag);
  second->publishObjectClassAttributes(secondClass, secondAttributes);
  evokeUntil(*second, [&] { return secondCallbacks.assumptions().size() == 2U; },
             options, model, "fresh ownership assumption after transfer");
  auto const offers = secondCallbacks.assumptions();
  require(offers.size() == 2U && offers.back().object == object &&
              offers.back().attributes == transferred &&
              offers.back().tag == secondTagBytes,
          "Old assumption reservation suppressed the new transfer epoch or tag");

  second->unpublishObjectClass(secondClass);
  first->resignFederationExecution(rti::NO_ACTION);
  second->resignFederationExecution(rti::NO_ACTION);
  second->destroyFederationExecution(federation);
  second->disconnect();
  first->disconnect();
  owner->disconnect();
}

void scenarioOwnershipAssumptionTransferEpochContract(
    Options const& options, rti::CallbackModel model) {
  scenarioOwnershipAssumptionTransferEpoch(options, model);
}

void scenarioResignAssumptionLaterDiscovery(
    Options const& options,
    rti::CallbackModel model) {
  require(!options.fom.empty(),
          "Late-discovery assumption requires an adapter-supplied FOM");
  rti::RTIambassadorFactory factory;
  auto owner = factory.createRTIambassador();
  auto observer = factory.createRTIambassador();
  auto late = factory.createRTIambassador();
  require(owner && observer && late, "The standard factory returned no ambassador");
  AssumptionCallbacks ownerCallbacks;
  AssumptionCallbacks observerCallbacks;
  AssumptionCallbacks lateCallbacks;
  connect(*owner, ownerCallbacks, options, model, true);
  connect(*observer, observerCallbacks, options, model, false);
  connect(*late, lateCallbacks, options, model, false);
  auto const federation = federationName(options, "resign-assumption-later-discovery");
  owner->createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  static_cast<void>(owner->joinFederationExecution(
      options.ownerFederateName + L"-late-discovery-owner",
      options.federateType, federation));
  static_cast<void>(observer->joinFederationExecution(
      options.memberFederateName + L"-late-discovery-observer",
      options.federateType, federation));

  auto const ownerClass = owner->getObjectClassHandle(options.objectClassName);
  auto const ownerAttribute = owner->getAttributeHandle(ownerClass, options.attributeName);
  require(ownerClass.isValid() && ownerAttribute.isValid(),
          "Late-discovery owner FOM lookup returned an invalid handle");
  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  owner->publishObjectClassAttributes(ownerClass, ownerAttributes);
  auto const object = owner->registerObjectInstance(ownerClass);
  require(object.isValid(), "Late-discovery registration returned an invalid handle");

  // The observer keeps the execution alive; no assumption recipient is yet known.
  owner->resignFederationExecution(rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES);
  static_cast<void>(late->joinFederationExecution(
      options.memberFederateName + L"-late-discovery-candidate",
      options.federateType, federation));
  auto const lateClass = late->getObjectClassHandle(options.objectClassName);
  auto const lateAttribute = late->getAttributeHandle(lateClass, options.attributeName);
  auto const latePrivilege = late->getAttributeHandle(
      lateClass, L"HLAprivilegeToDeleteObject");
  require(lateClass.isValid() && lateAttribute.isValid() && latePrivilege.isValid(),
          "Late-discovery candidate FOM lookup returned an invalid handle");
  rti::AttributeHandleSet const lateAttributes{lateAttribute};
  late->subscribeObjectClassAttributes(lateClass, lateAttributes, true, L"");
  evokeUntil(*late, [&] { return lateCallbacks.discovered(object); }, options, model,
             "retained object discovery after joining");
  if (model == rti::HLA_EVOKED) {
    while (late->evokeMultipleCallbacks(0.0, 0.0)) {
    }
  }
  require(lateCallbacks.assumptions().empty(),
          "Nonpublishing late candidate received an ownership assumption");

  late->publishObjectClassAttributes(lateClass, lateAttributes);
  evokeUntil(*late, [&] { return lateCallbacks.assumptions().size() == 1U; },
             options, model, "late candidate ownership assumption");
  auto const offers = lateCallbacks.assumptions();
  rti::AttributeHandleSet const expected{lateAttribute, latePrivilege};
  require(offers.size() == 1U && offers.front().object == object &&
              offers.front().attributes == expected && offers.front().tag.empty(),
          "Late discovery/publication offered the wrong object, attributes, or tag");

  late->resignFederationExecution(rti::NO_ACTION);
  observer->resignFederationExecution(rti::NO_ACTION);
  observer->destroyFederationExecution(federation);
  late->disconnect();
  observer->disconnect();
  owner->disconnect();
}

void scenarioResignAssumptionLaterDiscoveryContract(
    Options const& options, rti::CallbackModel model) {
  scenarioResignAssumptionLaterDiscovery(options, model);
}

void scenarioNegotiatedDivestitureConcurrentAssumption(
    Options const& options, rti::CallbackModel model) {
  require(!options.fom.empty(),
          "Negotiated assumption requires an adapter-supplied FOM");
  rti::RTIambassadorFactory factory;
  auto owner = factory.createRTIambassador();
  auto requester = factory.createRTIambassador();
  auto candidate = factory.createRTIambassador();
  require(owner && requester && candidate,
          "The standard factory returned no ambassador");
  AssumptionCallbacks ownerCallbacks;
  AssumptionCallbacks requesterCallbacks;
  AssumptionCallbacks candidateCallbacks;
  connect(*owner, ownerCallbacks, options, model, true);
  connect(*requester, requesterCallbacks, options, model, false);
  connect(*candidate, candidateCallbacks, options, model, false);
  auto const federation = federationName(options, "negotiated-concurrent-assumption");
  owner->createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  static_cast<void>(owner->joinFederationExecution(
      options.ownerFederateName + L"-negotiated-owner",
      options.federateType, federation));
  static_cast<void>(requester->joinFederationExecution(
      options.memberFederateName + L"-negotiated-requester",
      options.federateType, federation));
  static_cast<void>(candidate->joinFederationExecution(
      options.memberFederateName + L"-negotiated-candidate",
      options.federateType, federation));

  auto const ownerClass = owner->getObjectClassHandle(options.objectClassName);
  auto const requesterClass = requester->getObjectClassHandle(options.objectClassName);
  auto const candidateClass = candidate->getObjectClassHandle(options.objectClassName);
  auto const ownerAttribute = owner->getAttributeHandle(ownerClass, options.attributeName);
  auto const requesterAttribute = requester->getAttributeHandle(
      requesterClass, options.attributeName);
  auto const candidateAttribute = candidate->getAttributeHandle(
      candidateClass, options.attributeName);
  require(ownerClass.isValid() && requesterClass.isValid() && candidateClass.isValid() &&
              ownerAttribute.isValid() && requesterAttribute.isValid() &&
              candidateAttribute.isValid(),
          "Negotiated assumption FOM lookup returned an invalid handle");
  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const requesterAttributes{requesterAttribute};
  rti::AttributeHandleSet const candidateAttributes{candidateAttribute};
  candidate->subscribeObjectClassAttributes(
      candidateClass, candidateAttributes, true, L"");
  candidate->publishObjectClassAttributes(candidateClass, candidateAttributes);
  requester->subscribeObjectClassAttributes(
      requesterClass, requesterAttributes, true, L"");
  owner->publishObjectClassAttributes(ownerClass, ownerAttributes);
  auto const object = owner->registerObjectInstance(ownerClass);
  require(object.isValid(), "Negotiated assumption registration returned an invalid handle");
  evokeUntil(*candidate, [&] { return candidateCallbacks.discovered(object); },
             options, model, "candidate discovery");
  evokeUntil(*requester, [&] { return requesterCallbacks.discovered(object); },
             options, model, "requester discovery");
  requester->publishObjectClassAttributes(requesterClass, requesterAttributes);

  std::vector<std::uint8_t> const offerTagBytes{0xD2U, 0x11U, 0x6DU, 0x09U};
  std::vector<std::uint8_t> const confirmationTagBytes{0xE2U, 0x6AU, 0xB4U, 0x2DU};
  rti::VariableLengthData const offerTag(offerTagBytes.data(), offerTagBytes.size());
  rti::VariableLengthData const confirmationTag(
      confirmationTagBytes.data(), confirmationTagBytes.size());
  requester->attributeOwnershipAcquisition(object, requesterAttributes, offerTag);
  owner->negotiatedAttributeOwnershipDivestiture(object, ownerAttributes, offerTag);
  evokeUntil(*owner, [&] { return ownerCallbacks.divestitureRequests().size() == 1U; },
             options, model, "owner divestiture confirmation request");
  evokeUntil(*candidate, [&] { return candidateCallbacks.assumptions().size() == 1U; },
             options, model, "third candidate assumption while confirmation is pending");
  auto const requests = ownerCallbacks.divestitureRequests();
  auto const assumptions = candidateCallbacks.assumptions();
  require(requests.size() == 1U && requests.front().object == object &&
              requests.front().attributes == ownerAttributes &&
              requests.front().tag == offerTagBytes &&
              assumptions.size() == 1U && assumptions.front().object == object &&
              assumptions.front().attributes == candidateAttributes &&
              assumptions.front().tag == offerTagBytes &&
              owner->isAttributeOwnedByFederate(object, ownerAttribute) &&
              !requester->isAttributeOwnedByFederate(object, requesterAttribute),
          "Pending confirmation lost the candidate assumption or transferred ownership early");

  owner->confirmDivestiture(object, ownerAttributes, confirmationTag);
  evokeUntil(*requester, [&] { return requesterCallbacks.acquisitions().size() == 1U; },
             options, model, "requester acquisition after confirmation");
  auto const acquisitions = requesterCallbacks.acquisitions();
  require(acquisitions.size() == 1U && acquisitions.front().object == object &&
              acquisitions.front().attributes == requesterAttributes &&
              acquisitions.front().tag == confirmationTagBytes &&
              !owner->isAttributeOwnedByFederate(object, ownerAttribute) &&
              requester->isAttributeOwnedByFederate(object, requesterAttribute),
          "Confirm Divestiture did not transfer only after owner confirmation");

  requester->resignFederationExecution(rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES);
  candidate->resignFederationExecution(rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES);
  owner->resignFederationExecution(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner->destroyFederationExecution(federation);
  candidate->disconnect();
  requester->disconnect();
  owner->disconnect();
}

void scenarioNegotiatedDivestitureConcurrentAssumptionContract(
    Options const& options, rti::CallbackModel model) {
  scenarioNegotiatedDivestitureConcurrentAssumption(options, model);
}

void scenarioNegotiatedDivestitureConfirmationCoalescing(
    Options const& options, rti::CallbackModel model) {
  require(!options.multiAttributeFom.empty(),
          "Negotiated confirmation coalescing requires a two-attribute FOM");
  rti::RTIambassadorFactory factory;
  auto owner = factory.createRTIambassador();
  auto requester = factory.createRTIambassador();
  require(owner && requester,
          "The standard RTIambassador factory returned no ambassador");
  AssumptionCallbacks ownerCallbacks;
  AssumptionCallbacks requesterCallbacks;
  connect(*owner, ownerCallbacks, options, model, true);
  connect(*requester, requesterCallbacks, options, model, false);

  auto const federation = federationName(
      options, "negotiated-divestiture-confirmation-coalescing");
  owner->createFederationExecution(
      federation, options.multiAttributeFom.wstring(),
      options.logicalTimeImplementationName);
  static_cast<void>(owner->joinFederationExecution(
      options.ownerFederateName + L"-confirmation-owner", options.federateType,
      federation));
  static_cast<void>(requester->joinFederationExecution(
      options.memberFederateName + L"-confirmation-requester", options.federateType,
      federation));

  auto const ownerClass = owner->getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const requesterClass = requester->getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const ownerFirst = owner->getAttributeHandle(
      ownerClass, options.multiAttributeFirstName);
  auto const ownerSecond = owner->getAttributeHandle(
      ownerClass, options.multiAttributeSecondName);
  auto const requesterFirst = requester->getAttributeHandle(
      requesterClass, options.multiAttributeFirstName);
  auto const requesterSecond = requester->getAttributeHandle(
      requesterClass, options.multiAttributeSecondName);
  require(ownerClass.isValid() && requesterClass.isValid() &&
              ownerFirst.isValid() && ownerSecond.isValid() &&
              requesterFirst.isValid() && requesterSecond.isValid() &&
              ownerFirst != ownerSecond && requesterFirst != requesterSecond,
          "Adapter FOM did not resolve two distinct ownership attributes");

  rti::AttributeHandleSet const ownerAttributes{ownerFirst, ownerSecond};
  rti::AttributeHandleSet const requesterAttributes{requesterFirst, requesterSecond};
  owner->publishObjectClassAttributes(ownerClass, ownerAttributes);
  requester->publishObjectClassAttributes(requesterClass, requesterAttributes);
  requester->subscribeObjectClassAttributes(
      requesterClass, requesterAttributes, true, L"");
  auto const object = owner->registerObjectInstance(ownerClass);
  require(object.isValid(),
          "Negotiated confirmation coalescing registration returned no object");
  evokeUntil(*requester, [&] { return requesterCallbacks.discovered(object); },
             options, model, "confirmation requester object discovery");

  std::vector<std::uint8_t> const offerTagBytes{0x41U, 0x7cU, 0x22U};
  std::vector<std::uint8_t> const acquisitionTagBytes{0x52U, 0x8dU, 0x33U};
  std::vector<std::uint8_t> const confirmationTagBytes{0x63U, 0x9eU, 0x44U};
  rti::VariableLengthData const offerTag(offerTagBytes.data(), offerTagBytes.size());
  rti::VariableLengthData const acquisitionTag(
      acquisitionTagBytes.data(), acquisitionTagBytes.size());
  rti::VariableLengthData const confirmationTag(
      confirmationTagBytes.data(), confirmationTagBytes.size());

  owner->negotiatedAttributeOwnershipDivestiture(
      object, ownerAttributes, offerTag);
  require(ownerCallbacks.divestitureRequests().empty() &&
              owner->isAttributeOwnedByFederate(object, ownerFirst) &&
              owner->isAttributeOwnedByFederate(object, ownerSecond),
          "Negotiated divestiture lost ownership before a new owner was found");

  requester->attributeOwnershipAcquisition(
      object, requesterAttributes, acquisitionTag);
  evokeUntil(*owner,
             [&] { return !ownerCallbacks.divestitureRequests().empty(); },
             options, model, "coalesced request divestiture confirmation");
  if (model == rti::HLA_EVOKED) {
    while (owner->evokeCallback(0.0)) {
    }
    while (requester->evokeCallback(0.0)) {
    }
  }

  auto const requests = ownerCallbacks.divestitureRequests();
  require(requests.size() == 1U && requests.front().object == object &&
              requests.front().attributes == ownerAttributes &&
              requests.front().tag == acquisitionTagBytes,
          "One acquisition call did not produce one full-set confirmation callback");
  require(owner->isAttributeOwnedByFederate(object, ownerFirst) &&
              owner->isAttributeOwnedByFederate(object, ownerSecond) &&
              !requester->isAttributeOwnedByFederate(object, requesterFirst) &&
              !requester->isAttributeOwnedByFederate(object, requesterSecond) &&
              requesterCallbacks.acquisitions().empty(),
          "Negotiated acquisition transferred attributes before owner confirmation");

  owner->confirmDivestiture(object, ownerAttributes, confirmationTag);
  evokeUntil(*requester,
             [&] { return !requesterCallbacks.acquisitions().empty(); },
             options, model, "acquisition notification after full-set confirmation");
  if (model == rti::HLA_EVOKED) {
    while (owner->evokeCallback(0.0)) {
    }
    while (requester->evokeCallback(0.0)) {
    }
  }
  auto const acquisitions = requesterCallbacks.acquisitions();
  require(acquisitions.size() == 1U && acquisitions.front().object == object &&
              acquisitions.front().attributes == requesterAttributes &&
              acquisitions.front().tag == confirmationTagBytes &&
              !owner->isAttributeOwnedByFederate(object, ownerFirst) &&
              !owner->isAttributeOwnedByFederate(object, ownerSecond) &&
              requester->isAttributeOwnedByFederate(object, requesterFirst) &&
              requester->isAttributeOwnedByFederate(object, requesterSecond),
          "Confirmed negotiated divestiture did not transfer the complete set");

  requester->unpublishObjectClassAttributes(requesterClass, requesterAttributes);
  owner->unpublishObjectClassAttributes(ownerClass, ownerAttributes);
  requester->resignFederationExecution(rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES);
  owner->resignFederationExecution(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner->destroyFederationExecution(federation);
  requester->disconnect();
  owner->disconnect();
}

void scenarioNegotiatedDivestitureConfirmationCoalescingContract(
    Options const& options, rti::CallbackModel model) {
  scenarioNegotiatedDivestitureConfirmationCoalescing(options, model);
}

void scenarioNegotiatedDivestitureReturnsToWaitingForNewOwner(
    Options const& options, rti::CallbackModel model) {
  require(!options.fom.empty(),
          "Negotiated-divestiture recovery requires an adapter-supplied FOM");
  rti::RTIambassadorFactory factory;
  auto owner = factory.createRTIambassador();
  auto requester = factory.createRTIambassador();
  require(owner && requester,
          "The standard RTIambassador factory returned no ambassador");
  AssumptionCallbacks ownerCallbacks;
  AssumptionCallbacks requesterCallbacks;
  connect(*owner, ownerCallbacks, options, model, true);
  connect(*requester, requesterCallbacks, options, model, false);

  auto const federation = federationName(
      options, "negotiated-divestiture-waiting-recovery");
  owner->createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  static_cast<void>(owner->joinFederationExecution(
      options.ownerFederateName + L"-waiting-recovery-owner",
      options.federateType, federation));
  static_cast<void>(requester->joinFederationExecution(
      options.memberFederateName + L"-waiting-recovery-requester",
      options.federateType, federation));

  auto const ownerClass = owner->getObjectClassHandle(options.objectClassName);
  auto const requesterClass = requester->getObjectClassHandle(options.objectClassName);
  auto const ownerAttribute = owner->getAttributeHandle(
      ownerClass, options.attributeName);
  auto const requesterAttribute = requester->getAttributeHandle(
      requesterClass, options.attributeName);
  require(ownerClass.isValid() && requesterClass.isValid() &&
              ownerAttribute.isValid() && requesterAttribute.isValid(),
          "Negotiated-divestiture recovery could not resolve the object attribute");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const requesterAttributes{requesterAttribute};
  owner->publishObjectClassAttributes(ownerClass, ownerAttributes);
  requester->publishObjectClassAttributes(requesterClass, requesterAttributes);
  requester->subscribeObjectClassAttributes(
      requesterClass, requesterAttributes, true, L"");
  auto const object = owner->registerObjectInstance(ownerClass);
  require(object.isValid(),
          "Negotiated-divestiture recovery registration returned no object");
  evokeUntil(*requester, [&] { return requesterCallbacks.discovered(object); },
             options, model, "waiting-recovery object discovery");

  std::vector<std::uint8_t> const offerTagBytes{0x37U, 0x61U};
  std::vector<std::uint8_t> const firstAcquisitionTagBytes{0x48U, 0x72U};
  std::vector<std::uint8_t> const secondAcquisitionTagBytes{0x59U, 0x83U};
  std::vector<std::uint8_t> const confirmationTagBytes{0x6AU, 0x94U};
  rti::VariableLengthData const offerTag(offerTagBytes.data(), offerTagBytes.size());
  rti::VariableLengthData const firstAcquisitionTag(
      firstAcquisitionTagBytes.data(), firstAcquisitionTagBytes.size());
  rti::VariableLengthData const secondAcquisitionTag(
      secondAcquisitionTagBytes.data(), secondAcquisitionTagBytes.size());
  rti::VariableLengthData const confirmationTag(
      confirmationTagBytes.data(), confirmationTagBytes.size());

  owner->negotiatedAttributeOwnershipDivestiture(
      object, ownerAttributes, offerTag);
  requester->attributeOwnershipAcquisition(
      object, requesterAttributes, firstAcquisitionTag);
  evokeUntil(*owner,
             [&] { return ownerCallbacks.divestitureRequests().size() == 1U; },
             options, model, "first negotiated-divestiture confirmation request");
  auto const firstRequests = ownerCallbacks.divestitureRequests();
  require(firstRequests.front().object == object &&
              firstRequests.front().attributes == ownerAttributes &&
              firstRequests.front().tag == firstAcquisitionTagBytes &&
              owner->isAttributeOwnedByFederate(object, ownerAttribute) &&
              !requester->isAttributeOwnedByFederate(object, requesterAttribute),
          "First negotiated confirmation did not preserve ownership and request metadata");

  requester->cancelAttributeOwnershipAcquisition(object, requesterAttributes);
  bool noAcquisitionPending = false;
  bool confirmationWasNoLongerRequested = false;
  if (model == rti::HLA_EVOKED) {
    try {
      owner->confirmDivestiture(object, ownerAttributes, confirmationTag);
    } catch (rti::NoAcquisitionPending const&) {
      noAcquisitionPending = true;
    }
  }
  evokeUntil(*requester,
             [&] { return requesterCallbacks.cancellations().size() == 1U; },
             options, model, "canceled acquisition confirmation");
  if (model == rti::HLA_IMMEDIATE) {
    try {
      owner->confirmDivestiture(object, ownerAttributes, confirmationTag);
    } catch (rti::AttributeDivestitureWasNotRequested const&) {
      confirmationWasNoLongerRequested = true;
    }
  }
  auto const cancellations = requesterCallbacks.cancellations();
  require(cancellations.front().object == object &&
              cancellations.front().attributes == requesterAttributes,
          "Canceled acquisition confirmation returned the wrong attribute set");
  require(
      (model == rti::HLA_EVOKED && noAcquisitionPending) ||
          (model == rti::HLA_IMMEDIATE && confirmationWasNoLongerRequested),
      "Confirm Divestiture did not report the exception matching the cancellation timing");
  require(owner->isAttributeOwnedByFederate(object, ownerAttribute) &&
              !requester->isAttributeOwnedByFederate(object, requesterAttribute) &&
              ownerCallbacks.divestitureRequests().size() == 1U &&
              requesterCallbacks.acquisitions().empty(),
          "Failed confirmation did not retain ownership while waiting for a new owner");

  requester->attributeOwnershipAcquisition(
      object, requesterAttributes, secondAcquisitionTag);
  evokeUntil(*owner,
             [&] { return ownerCallbacks.divestitureRequests().size() == 2U; },
             options, model, "new confirmation request after acquisition cancellation");
  auto const requests = ownerCallbacks.divestitureRequests();
  require(requests.size() == 2U && requests.back().object == object &&
              requests.back().attributes == ownerAttributes &&
              requests.back().tag == secondAcquisitionTagBytes &&
              owner->isAttributeOwnedByFederate(object, ownerAttribute),
          "Negotiated divestiture did not return to waiting for a later acquirer");

  owner->confirmDivestiture(object, ownerAttributes, confirmationTag);
  evokeUntil(*requester,
             [&] { return requesterCallbacks.acquisitions().size() == 1U; },
             options, model, "acquisition notification after recovered confirmation");
  auto const acquisitions = requesterCallbacks.acquisitions();
  require(acquisitions.size() == 1U && acquisitions.front().object == object &&
              acquisitions.front().attributes == requesterAttributes &&
              acquisitions.front().tag == confirmationTagBytes &&
              !owner->isAttributeOwnedByFederate(object, ownerAttribute) &&
              requester->isAttributeOwnedByFederate(object, requesterAttribute),
          "Recovered negotiated divestiture did not transfer after confirmation");

  requester->unpublishObjectClassAttributes(requesterClass, requesterAttributes);
  owner->unpublishObjectClassAttributes(ownerClass, ownerAttributes);
  requester->resignFederationExecution(rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES);
  owner->resignFederationExecution(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner->destroyFederationExecution(federation);
  requester->disconnect();
  owner->disconnect();
}

void scenarioNegotiatedDivestitureReturnsToWaitingForNewOwnerContract(
    Options const& options, rti::CallbackModel model) {
  scenarioNegotiatedDivestitureReturnsToWaitingForNewOwner(options, model);
}

void scenarioAttributeOwnershipReleaseCoalescing(
    Options const& options, rti::CallbackModel model) {
  require(!options.multiAttributeFom.empty(),
          "Ownership-release coalescing requires a two-attribute FOM");
  rti::RTIambassadorFactory factory;
  auto owner = factory.createRTIambassador();
  auto requester = factory.createRTIambassador();
  require(owner && requester,
          "The standard RTIambassador factory returned no ambassador");
  AssumptionCallbacks ownerCallbacks;
  AssumptionCallbacks requesterCallbacks;
  connect(*owner, ownerCallbacks, options, model, true);
  connect(*requester, requesterCallbacks, options, model, false);

  auto const federation = federationName(
      options, "attribute-ownership-release-coalescing");
  owner->createFederationExecution(
      federation, options.multiAttributeFom.wstring(),
      options.logicalTimeImplementationName);
  static_cast<void>(owner->joinFederationExecution(
      options.ownerFederateName + L"-release-coalescing-owner",
      options.federateType, federation));
  static_cast<void>(requester->joinFederationExecution(
      options.memberFederateName + L"-release-coalescing-requester",
      options.federateType, federation));

  auto const ownerClass = owner->getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const requesterClass = requester->getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const ownerFirst = owner->getAttributeHandle(
      ownerClass, options.multiAttributeFirstName);
  auto const ownerSecond = owner->getAttributeHandle(
      ownerClass, options.multiAttributeSecondName);
  auto const requesterFirst = requester->getAttributeHandle(
      requesterClass, options.multiAttributeFirstName);
  auto const requesterSecond = requester->getAttributeHandle(
      requesterClass, options.multiAttributeSecondName);
  require(ownerClass.isValid() && requesterClass.isValid() &&
              ownerFirst.isValid() && ownerSecond.isValid() &&
              requesterFirst.isValid() && requesterSecond.isValid() &&
              ownerFirst != ownerSecond && requesterFirst != requesterSecond,
          "Adapter FOM did not resolve two distinct ownership attributes");

  rti::AttributeHandleSet const ownerAttributes{ownerFirst, ownerSecond};
  rti::AttributeHandleSet const requesterAttributes{requesterFirst, requesterSecond};
  owner->publishObjectClassAttributes(ownerClass, ownerAttributes);
  requester->publishObjectClassAttributes(requesterClass, requesterAttributes);
  requester->subscribeObjectClassAttributes(
      requesterClass, requesterAttributes, true, L"");
  auto const object = owner->registerObjectInstance(ownerClass);
  require(object.isValid(),
          "Ownership-release coalescing registration returned no object");
  evokeUntil(*requester, [&] { return requesterCallbacks.discovered(object); },
             options, model, "ownership-release requester discovery");
  require(owner->isAttributeOwnedByFederate(object, ownerFirst) &&
              owner->isAttributeOwnedByFederate(object, ownerSecond),
          "Release-request owner did not initially own both attributes");

  std::vector<std::uint8_t> const acquisitionTagBytes{0x71U, 0x32U, 0x5AU};
  std::vector<std::uint8_t> const divestitureTagBytes{0x72U, 0x43U, 0x6BU};
  rti::VariableLengthData const acquisitionTag(
      acquisitionTagBytes.data(), acquisitionTagBytes.size());
  rti::VariableLengthData const divestitureTag(
      divestitureTagBytes.data(), divestitureTagBytes.size());
  requester->attributeOwnershipAcquisition(
      object, requesterAttributes, acquisitionTag);
  evokeUntil(*owner,
             [&] { return !ownerCallbacks.releaseRequests().empty(); },
             options, model, "coalesced ownership-release request");
  if (model == rti::HLA_EVOKED) {
    while (owner->evokeCallback(0.0)) {
    }
    while (requester->evokeCallback(0.0)) {
    }
  }
  auto const releases = ownerCallbacks.releaseRequests();
  require(releases.size() == 1U && releases.front().object == object &&
              releases.front().attributes == ownerAttributes &&
              releases.front().tag == acquisitionTagBytes,
          "One multi-attribute acquisition did not produce one complete release request");
  require(owner->isAttributeOwnedByFederate(object, ownerFirst) &&
              owner->isAttributeOwnedByFederate(object, ownerSecond) &&
              !requester->isAttributeOwnedByFederate(object, requesterFirst) &&
              !requester->isAttributeOwnedByFederate(object, requesterSecond),
          "Request Attribute Ownership Release changed ownership before divestiture");

  owner->unconditionalAttributeOwnershipDivestiture(
      object, ownerAttributes, divestitureTag);
  evokeUntil(*requester,
             [&] { return !requesterCallbacks.acquisitions().empty(); },
             options, model, "acquisition notification after release request");
  auto const acquisitions = requesterCallbacks.acquisitions();
  require(acquisitions.size() == 1U && acquisitions.front().object == object &&
              acquisitions.front().attributes == requesterAttributes &&
              acquisitions.front().tag == acquisitionTagBytes &&
              !owner->isAttributeOwnedByFederate(object, ownerFirst) &&
              !owner->isAttributeOwnedByFederate(object, ownerSecond) &&
              requester->isAttributeOwnedByFederate(object, requesterFirst) &&
              requester->isAttributeOwnedByFederate(object, requesterSecond),
          "Coalesced release request did not lead to one complete acquisition");

  requester->unpublishObjectClassAttributes(requesterClass, requesterAttributes);
  owner->unpublishObjectClassAttributes(ownerClass, ownerAttributes);
  requester->resignFederationExecution(rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES);
  owner->resignFederationExecution(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner->destroyFederationExecution(federation);
  requester->disconnect();
  owner->disconnect();
}

void scenarioAttributeOwnershipReleaseCoalescingContract(
    Options const& options, rti::CallbackModel model) {
  scenarioAttributeOwnershipReleaseCoalescing(options, model);
}

void scenarioAcquisitionWhileOwnerWaitsForNewOwner(
    Options const& options, rti::CallbackModel model) {
  require(!options.fom.empty(),
          "Waiting-for-new-owner acquisition requires an adapter-supplied FOM");
  rti::RTIambassadorFactory factory;
  auto owner = factory.createRTIambassador();
  auto requester = factory.createRTIambassador();
  require(owner && requester,
          "The standard factory returned no ownership ambassador");
  AssumptionCallbacks ownerCallbacks;
  AssumptionCallbacks requesterCallbacks;
  connect(*owner, ownerCallbacks, options, model, true);
  connect(*requester, requesterCallbacks, options, model, false);
  auto const federation = federationName(
      options, "ownership-acquisition-waiting-for-new-owner");
  owner->createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  static_cast<void>(owner->joinFederationExecution(
      options.ownerFederateName + L"-waiting-owner",
      options.federateType, federation));
  static_cast<void>(requester->joinFederationExecution(
      options.memberFederateName + L"-waiting-requester",
      options.federateType, federation));

  auto const ownerClass = owner->getObjectClassHandle(options.objectClassName);
  auto const requesterClass = requester->getObjectClassHandle(options.objectClassName);
  auto const ownerAttribute = owner->getAttributeHandle(ownerClass, options.attributeName);
  auto const requesterAttribute = requester->getAttributeHandle(
      requesterClass, options.attributeName);
  require(ownerClass.isValid() && requesterClass.isValid() &&
              ownerClass == requesterClass && ownerAttribute.isValid() &&
              requesterAttribute == ownerAttribute,
          "Waiting-for-new-owner FOM lookup returned inconsistent handles");
  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const requesterAttributes{requesterAttribute};
  owner->publishObjectClassAttributes(ownerClass, ownerAttributes);
  requester->subscribeObjectClassAttributes(
      requesterClass, requesterAttributes, true, L"");
  auto const object = owner->registerObjectInstance(ownerClass);
  require(object.isValid(),
          "Waiting-for-new-owner registration returned an invalid handle");
  evokeUntil(*requester, [&] { return requesterCallbacks.discovered(object); },
             options, model, "waiting-for-new-owner discovery");
  requester->publishObjectClassAttributes(requesterClass, requesterAttributes);

  std::vector<std::uint8_t> const divestitureTagBytes{0xD7U, 0x18U, 0x01U};
  std::vector<std::uint8_t> const acquisitionTagBytes{0xA7U, 0x18U, 0x02U};
  std::vector<std::uint8_t> const confirmationTagBytes{0xC7U, 0x18U, 0x03U};
  rti::VariableLengthData const divestitureTag(
      divestitureTagBytes.data(), divestitureTagBytes.size());
  rti::VariableLengthData const acquisitionTag(
      acquisitionTagBytes.data(), acquisitionTagBytes.size());
  rti::VariableLengthData const confirmationTag(
      confirmationTagBytes.data(), confirmationTagBytes.size());

  // With no pending acquirer, this puts the owner in Waiting for a New Owner
  // to be Found. A later acquisition must request confirmation, not release.
  owner->negotiatedAttributeOwnershipDivestiture(
      object, ownerAttributes, divestitureTag);
  require(ownerCallbacks.divestitureRequests().empty() &&
              ownerCallbacks.releaseRequests().empty() &&
              owner->isAttributeOwnedByFederate(object, ownerAttribute),
          "Negotiated divestiture did not leave the owner waiting with ownership intact");

  requester->attributeOwnershipAcquisition(
      object, requesterAttributes, acquisitionTag);
  evokeUntil(*owner,
             [&] { return ownerCallbacks.divestitureRequests().size() == 1U; },
             options, model, "request divestiture confirmation from waiting owner");
  if (model == rti::HLA_EVOKED) {
    for (int pass = 0; pass != 32; ++pass) {
      if (!owner->evokeCallback(0.0)) break;
    }
  }
  auto const requests = ownerCallbacks.divestitureRequests();
  auto const releaseRequests = ownerCallbacks.releaseRequests();
  require(requests.size() == 1U && requests.front().object == object &&
              requests.front().attributes == ownerAttributes &&
              requests.front().tag == acquisitionTagBytes,
          "Waiting-for-new-owner acquisition returned the wrong confirmation request");
  require(releaseRequests.empty(),
          "RTI invoked Request Attribute Ownership Release while the owner was waiting for a new owner");
  require(owner->isAttributeOwnedByFederate(object, ownerAttribute) &&
              !requester->isAttributeOwnedByFederate(object, requesterAttribute) &&
              requesterCallbacks.acquisitions().empty(),
          "Waiting-for-new-owner acquisition transferred before owner confirmation");

  owner->confirmDivestiture(object, ownerAttributes, confirmationTag);
  evokeUntil(*requester,
             [&] { return requesterCallbacks.acquisitions().size() == 1U; },
             options, model, "acquisition notification after owner confirmation");
  auto const acquisitions = requesterCallbacks.acquisitions();
  require(acquisitions.size() == 1U && acquisitions.front().object == object &&
              acquisitions.front().attributes == requesterAttributes &&
              acquisitions.front().tag == confirmationTagBytes &&
              !owner->isAttributeOwnedByFederate(object, ownerAttribute) &&
              requester->isAttributeOwnedByFederate(object, requesterAttribute),
          "Confirmed waiting-owner divestiture did not transfer the requested attribute");

  std::vector<std::uint8_t> const denialTagBytes{0xD9U, 0x18U, 0x04U};
  rti::VariableLengthData const denialTag(
      denialTagBytes.data(), denialTagBytes.size());
  owner->attributeOwnershipAcquisition(object, ownerAttributes, acquisitionTag);
  evokeUntil(*requester,
             [&] { return requesterCallbacks.releaseRequests().size() == 1U; },
             options, model, "release request before acquisition denial");
  auto const reacquisitionRelease = requesterCallbacks.releaseRequests();
  require(reacquisitionRelease.front().object == object &&
              reacquisitionRelease.front().attributes == requesterAttributes &&
              reacquisitionRelease.front().tag == acquisitionTagBytes,
          "Post-transfer reacquisition returned the wrong release request");
  requester->attributeOwnershipReleaseDenied(
      object, requesterAttributes, denialTag);
  evokeUntil(*owner,
             [&] { return ownerCallbacks.unavailable().size() == 1U; },
             options, model, "acquisition-unavailable callback after denial");
  auto const unavailable = ownerCallbacks.unavailable();
  require(unavailable.front().object == object &&
              unavailable.front().attributes == ownerAttributes &&
              unavailable.front().tag == denialTagBytes,
          "Denied reacquisition returned the wrong unavailable callback data");
  require(!owner->isAttributeOwnedByFederate(object, ownerAttribute) &&
              requester->isAttributeOwnedByFederate(object, requesterAttribute) &&
              ownerCallbacks.acquisitions().empty() &&
              requesterCallbacks.acquisitions().size() == 1U,
          "Denied reacquisition changed ownership or emitted an acquisition notification");

  owner->attributeOwnershipAcquisition(object, ownerAttributes, acquisitionTag);
  evokeUntil(*requester,
             [&] { return requesterCallbacks.releaseRequests().size() == 2U; },
             options, model, "release request before acquisition cancellation");
  owner->cancelAttributeOwnershipAcquisition(object, ownerAttributes);
  evokeUntil(*owner,
             [&] { return ownerCallbacks.cancellations().size() == 1U; },
             options, model, "acquisition-cancellation confirmation");
  auto const cancellations = ownerCallbacks.cancellations();
  require(cancellations.front().object == object &&
              cancellations.front().attributes == ownerAttributes &&
              ownerCallbacks.unavailable().size() == 1U &&
              ownerCallbacks.acquisitions().empty() &&
              requesterCallbacks.acquisitions().size() == 1U &&
              !owner->isAttributeOwnedByFederate(object, ownerAttribute) &&
              requester->isAttributeOwnedByFederate(object, requesterAttribute),
          "Canceled reacquisition did not remain pending until confirmed cancellation");

  requester->resignFederationExecution(rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES);
  owner->resignFederationExecution(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner->destroyFederationExecution(federation);
  requester->disconnect();
  owner->disconnect();
}

void scenarioAcquisitionWhileOwnerWaitsForNewOwnerContract(
    Options const& options, rti::CallbackModel model) {
  scenarioAcquisitionWhileOwnerWaitsForNewOwner(options, model);
}

void scenarioConfirmDivestitureFromCallback(
    Options const& options, rti::CallbackModel model) {
  require(!options.fom.empty(),
          "Callback re-entry requires an adapter-supplied FOM");
  rti::RTIambassadorFactory factory;
  auto owner = factory.createRTIambassador();
  auto requester = factory.createRTIambassador();
  require(owner && requester, "The standard factory returned no ambassador");
  AssumptionCallbacks ownerCallbacks;
  AssumptionCallbacks requesterCallbacks;
  connect(*owner, ownerCallbacks, options, model, true);
  connect(*requester, requesterCallbacks, options, model, false);
  auto const federation = federationName(options, "confirm-divestiture-callback");
  owner->createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  static_cast<void>(owner->joinFederationExecution(
      options.ownerFederateName + L"-confirm-callback-owner",
      options.federateType, federation));
  static_cast<void>(requester->joinFederationExecution(
      options.memberFederateName + L"-confirm-callback-requester",
      options.federateType, federation));

  auto const ownerClass = owner->getObjectClassHandle(options.objectClassName);
  auto const requesterClass = requester->getObjectClassHandle(options.objectClassName);
  auto const ownerAttribute = owner->getAttributeHandle(ownerClass, options.attributeName);
  auto const requesterAttribute = requester->getAttributeHandle(
      requesterClass, options.attributeName);
  require(ownerClass.isValid() && requesterClass.isValid() &&
              ownerAttribute.isValid() && requesterAttribute.isValid(),
          "Callback re-entry FOM lookup returned an invalid handle");
  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const requesterAttributes{requesterAttribute};
  requester->subscribeObjectClassAttributes(
      requesterClass, requesterAttributes, true, L"");
  owner->publishObjectClassAttributes(ownerClass, ownerAttributes);
  auto const object = owner->registerObjectInstance(ownerClass);
  require(object.isValid(), "Callback re-entry registration returned an invalid handle");
  evokeUntil(*requester, [&] { return requesterCallbacks.discovered(object); },
             options, model, "requester discovery before ownership negotiation");
  requester->publishObjectClassAttributes(requesterClass, requesterAttributes);

  std::vector<std::uint8_t> const negotiatedTagBytes{0xC2U, 0x11U, 0x6DU, 0x09U};
  std::vector<std::uint8_t> const confirmationTagBytes{0xE1U, 0x6AU, 0xB4U, 0x2DU};
  rti::VariableLengthData const negotiatedTag(
      negotiatedTagBytes.data(), negotiatedTagBytes.size());
  rti::VariableLengthData const confirmationTag(
      confirmationTagBytes.data(), confirmationTagBytes.size());
  requester->attributeOwnershipAcquisition(
      object, requesterAttributes, negotiatedTag);
  require(requesterCallbacks.acquisitions().empty() &&
              ownerCallbacks.divestitureRequests().empty() &&
              owner->isAttributeOwnedByFederate(object, ownerAttribute) &&
              !requester->isAttributeOwnedByFederate(object, requesterAttribute),
          "Acquisition transferred ownership before negotiated divestiture");

  std::atomic<bool> confirmed{false};
  ownerCallbacks.setDivestitureHook([&] {
    owner->confirmDivestiture(object, ownerAttributes, confirmationTag);
    confirmed.store(true);
  });
  owner->negotiatedAttributeOwnershipDivestiture(
      object, ownerAttributes, negotiatedTag);
  evokeUntil(*owner,
             [&] { return confirmed.load() || ownerCallbacks.divestitureFailure(); },
             options, model, "re-entrant Confirm Divestiture callback");
  if (auto failure = ownerCallbacks.divestitureFailure()) {
    std::rethrow_exception(failure);
  }
  ownerCallbacks.setDivestitureHook({});
  auto const requests = ownerCallbacks.divestitureRequests();
  require(confirmed.load() && requests.size() == 1U &&
              requests.front().object == object &&
              requests.front().attributes == ownerAttributes &&
              requests.front().tag == negotiatedTagBytes,
          "Re-entrant confirmation lost the request object, attributes, or tag");

  evokeUntil(*requester, [&] { return requesterCallbacks.acquisitions().size() == 1U; },
             options, model, "acquisition notification after callback confirmation");
  auto const acquisitions = requesterCallbacks.acquisitions();
  require(acquisitions.size() == 1U && acquisitions.front().object == object &&
              acquisitions.front().attributes == requesterAttributes &&
              acquisitions.front().tag == confirmationTagBytes &&
              !owner->isAttributeOwnedByFederate(object, ownerAttribute) &&
              requester->isAttributeOwnedByFederate(object, requesterAttribute),
          "Re-entrant confirmation did not transfer the attribute exactly once");

  requester->resignFederationExecution(rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES);
  owner->resignFederationExecution(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner->destroyFederationExecution(federation);
  requester->disconnect();
  owner->disconnect();
}

void scenarioConfirmDivestitureFromCallbackContract(
    Options const& options, rti::CallbackModel model) {
  scenarioConfirmDivestitureFromCallback(options, model);
}

void scenarioConfirmDivestitureMixedSetAtomicity(
    Options const& options, rti::CallbackModel model) {
  require(!options.multiAttributeFom.empty(),
          "Mixed Confirm Divestiture requires an adapter-supplied two-attribute FOM");
  rti::RTIambassadorFactory factory;
  auto owner = factory.createRTIambassador();
  auto requester = factory.createRTIambassador();
  require(owner && requester, "The standard factory returned no ambassador");
  AssumptionCallbacks ownerCallbacks;
  AssumptionCallbacks requesterCallbacks;
  connect(*owner, ownerCallbacks, options, model, true);
  connect(*requester, requesterCallbacks, options, model, false);
  auto const federation = federationName(options, "confirm-divestiture-mixed-set");
  owner->createFederationExecution(
      federation, options.multiAttributeFom.wstring(),
      options.logicalTimeImplementationName);
  static_cast<void>(owner->joinFederationExecution(
      options.ownerFederateName + L"-mixed-confirm-owner",
      options.federateType, federation));
  static_cast<void>(requester->joinFederationExecution(
      options.memberFederateName + L"-mixed-confirm-requester",
      options.federateType, federation));

  auto const ownerClass = owner->getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const requesterClass = requester->getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const ownerFirst = owner->getAttributeHandle(
      ownerClass, options.multiAttributeFirstName);
  auto const ownerSecond = owner->getAttributeHandle(
      ownerClass, options.multiAttributeSecondName);
  auto const requesterFirst = requester->getAttributeHandle(
      requesterClass, options.multiAttributeFirstName);
  require(ownerClass.isValid() && requesterClass.isValid() &&
              ownerFirst.isValid() && ownerSecond.isValid() &&
              requesterFirst.isValid() && ownerFirst != ownerSecond,
          "Adapter two-attribute FOM returned invalid or duplicate handles");
  rti::AttributeHandleSet const ownerBoth{ownerFirst, ownerSecond};
  rti::AttributeHandleSet const ownerRequested{ownerFirst};
  rti::AttributeHandleSet const requesterRequested{requesterFirst};
  owner->publishObjectClassAttributes(ownerClass, ownerBoth);
  requester->subscribeObjectClassAttributes(
      requesterClass, requesterRequested, true, L"");
  auto const object = owner->registerObjectInstance(ownerClass);
  require(object.isValid(), "Mixed confirmation registration returned an invalid handle");
  evokeUntil(*requester, [&] { return requesterCallbacks.discovered(object); },
             options, model, "requester discovery before mixed confirmation");
  requester->publishObjectClassAttributes(requesterClass, requesterRequested);

  std::vector<std::uint8_t> const negotiatedTagBytes{0xC3U, 0x11U, 0x6DU, 0x09U};
  std::vector<std::uint8_t> const confirmationTagBytes{0xE1U, 0x6AU, 0xB4U, 0x2DU};
  rti::VariableLengthData const negotiatedTag(
      negotiatedTagBytes.data(), negotiatedTagBytes.size());
  rti::VariableLengthData const confirmationTag(
      confirmationTagBytes.data(), confirmationTagBytes.size());
  requester->attributeOwnershipAcquisition(
      object, requesterRequested, negotiatedTag);
  std::atomic<bool> mixedAttempted{false};
  std::atomic<bool> mixedRejected{false};
  ownerCallbacks.setDivestitureHook([&] {
    try {
      owner->confirmDivestiture(object, ownerBoth, confirmationTag);
    } catch (rti::AttributeDivestitureWasNotRequested const&) {
      mixedRejected.store(true);
    }
    mixedAttempted.store(true);
  });
  owner->negotiatedAttributeOwnershipDivestiture(
      object, ownerRequested, negotiatedTag);
  evokeUntil(*owner,
             [&] { return mixedAttempted.load() || ownerCallbacks.divestitureFailure(); },
             options, model, "mixed-set Confirm Divestiture rejection");
  if (auto failure = ownerCallbacks.divestitureFailure()) {
    std::rethrow_exception(failure);
  }
  ownerCallbacks.setDivestitureHook({});
  auto const requests = ownerCallbacks.divestitureRequests();
  require(mixedRejected.load() && requests.size() == 1U &&
              requests.front().object == object &&
              requests.front().attributes == ownerRequested &&
              requests.front().tag == negotiatedTagBytes &&
              requesterCallbacks.acquisitions().empty() &&
              owner->isAttributeOwnedByFederate(object, ownerFirst) &&
              owner->isAttributeOwnedByFederate(object, ownerSecond) &&
              !requester->isAttributeOwnedByFederate(object, requesterFirst),
          "Mixed confirmation was not rejected atomically with ownership intact");

  owner->confirmDivestiture(object, ownerRequested, confirmationTag);
  evokeUntil(*requester, [&] { return requesterCallbacks.acquisitions().size() == 1U; },
             options, model, "valid confirmation after rejected mixed set");
  auto const acquisitions = requesterCallbacks.acquisitions();
  require(acquisitions.size() == 1U && acquisitions.front().object == object &&
              acquisitions.front().attributes == requesterRequested &&
              acquisitions.front().tag == confirmationTagBytes &&
              !owner->isAttributeOwnedByFederate(object, ownerFirst) &&
              owner->isAttributeOwnedByFederate(object, ownerSecond) &&
              requester->isAttributeOwnedByFederate(object, requesterFirst),
          "Rejected mixed set partially consumed the valid confirmation path");

  requester->resignFederationExecution(rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES);
  owner->resignFederationExecution(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner->destroyFederationExecution(federation);
  requester->disconnect();
  owner->disconnect();
}

void scenarioConfirmDivestitureMixedSetAtomicityContract(
    Options const& options, rti::CallbackModel model) {
  scenarioConfirmDivestitureMixedSetAtomicity(options, model);
}

void scenarioUnconditionalDivestitureMixedSetAtomicity(
    Options const& options, rti::CallbackModel model) {
  require(!options.multiAttributeFom.empty(),
          "Mixed unconditional divestiture requires an adapter-supplied two-attribute FOM");

  Session owner(options, model, "unconditional-divestiture-atomicity-owner");
  Session requester(options, model, "unconditional-divestiture-atomicity-requester");
  auto const federation = federationName(
      options, "unconditional-divestiture-mixed-set-atomicity");
  owner.connect();
  requester.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation, options.multiAttributeFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName + L"-unconditional-atomicity-owner",
             options.federateType, federation);
  requester.join(options.memberFederateName + L"-unconditional-atomicity-requester",
                 options.federateType, federation);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const ownerFirst = owner.rtiAmbassador().getAttributeHandle(
      ownerClass, options.multiAttributeFirstName);
  auto const ownerSecond = owner.rtiAmbassador().getAttributeHandle(
      ownerClass, options.multiAttributeSecondName);
  auto const requesterFirst = requester.rtiAmbassador().getAttributeHandle(
      requesterClass, options.multiAttributeFirstName);
  auto const requesterSecond = requester.rtiAmbassador().getAttributeHandle(
      requesterClass, options.multiAttributeSecondName);
  require(ownerClass.isValid() && requesterClass.isValid() &&
              ownerFirst.isValid() && ownerSecond.isValid() &&
              requesterFirst.isValid() && requesterSecond.isValid() &&
              ownerFirst != ownerSecond && requesterFirst != requesterSecond,
          "Adapter FOM did not resolve two distinct ordinary ownership attributes");

  rti::AttributeHandleSet const ownerBoth{ownerFirst, ownerSecond};
  rti::AttributeHandleSet const ownerFirstOnly{ownerFirst};
  rti::AttributeHandleSet const requesterBoth{requesterFirst, requesterSecond};
  rti::AttributeHandleSet const requesterFirstOnly{requesterFirst};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerBoth);
  requester.rtiAmbassador().publishObjectClassAttributes(
      requesterClass, requesterBoth);
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass, requesterBoth, true, L"");
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(object.isValid(),
          "Mixed unconditional divestiture registration returned an invalid handle");
  waitFor(requester,
          [&] { return requester.recorder().hasDiscovery(object); },
          options, "mixed unconditional divestiture object discovery");
  require(owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerFirst) &&
              owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerSecond),
          "Registrant did not initially own both ordinary attributes");

  std::vector<std::uint8_t> const acquisitionTagBytes{0x91U, 0x54U};
  std::vector<std::uint8_t> const transferTagBytes{0x92U, 0x54U};
  std::vector<std::uint8_t> const invalidSetTagBytes{0x93U, 0x54U};
  rti::VariableLengthData const acquisitionTag(
      acquisitionTagBytes.data(), acquisitionTagBytes.size());
  rti::VariableLengthData const transferTag(
      transferTagBytes.data(), transferTagBytes.size());
  rti::VariableLengthData const invalidSetTag(
      invalidSetTagBytes.data(), invalidSetTagBytes.size());
  requester.rtiAmbassador().attributeOwnershipAcquisition(
      object, requesterFirstOnly, acquisitionTag);
  rti::AttributeHandleSet divested;
  owner.rtiAmbassador().attributeOwnershipDivestitureIfWanted(
      object, ownerFirstOnly, transferTag, divested);
  require(divested == ownerFirstOnly &&
              !owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerFirst) &&
              owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerSecond) &&
              requester.rtiAmbassador().isAttributeOwnedByFederate(
                  object, requesterFirst),
          "The setup transfer did not leave one attribute owned by each federate");
  waitFor(requester,
          [&] { return requester.recorder().ownershipAcquisition().has_value(); },
          options, "setup ownership acquisition notification");
  auto const acquisition = requester.recorder().ownershipAcquisition();
  require(acquisition->object == object &&
              acquisition->attributes == requesterFirstOnly &&
              acquisition->tag == transferTagBytes,
          "Setup acquisition notification returned incorrect metadata");

  owner.recorder().clearOwnershipRecords();
  requester.recorder().clearOwnershipRecords();
  requireException(
      [&] {
        owner.rtiAmbassador().unconditionalAttributeOwnershipDivestiture(
            object, ownerBoth, invalidSetTag);
      },
      L"AttributeNotOwned",
      "unconditional divestiture with one attribute owned by another federate");
  for (int pass = 0; pass != 8; ++pass) {
    owner.pump();
    requester.pump();
  }
  require(!requester.recorder().ownershipAssumption().has_value() &&
              !requester.recorder().ownershipAcquisition().has_value() &&
              owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerSecond) &&
              !owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerFirst) &&
              requester.rtiAmbassador().isAttributeOwnedByFederate(
                  object, requesterFirst) &&
              !requester.rtiAmbassador().isAttributeOwnedByFederate(
                  object, requesterSecond),
          "Rejected mixed-set divestiture produced a callback or partially changed ownership");

  owner.rtiAmbassador().deleteObjectInstance(
      object, rti::VariableLengthData{});
  requester.resign(rti::NO_ACTION);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioUnconditionalDivestitureMixedSetAtomicityContract(
    Options const& options, rti::CallbackModel model) {
  scenarioUnconditionalDivestitureMixedSetAtomicity(options, model);
}

void scenarioUnconditionalDivestitureAssumptionCoalescing(
    Options const& options, rti::CallbackModel model) {
  require(!options.multiAttributeFom.empty(),
          "Unconditional divestiture assumption requires a two-attribute FOM");

  rti::RTIambassadorFactory factory;
  auto owner = factory.createRTIambassador();
  auto candidate = factory.createRTIambassador();
  auto observer = factory.createRTIambassador();
  require(owner && candidate && observer,
          "The standard RTIambassador factory returned no ambassador");
  AssumptionCallbacks ownerCallbacks;
  AssumptionCallbacks candidateCallbacks;
  AssumptionCallbacks observerCallbacks;
  connect(*owner, ownerCallbacks, options, model, true);
  connect(*candidate, candidateCallbacks, options, model, false);
  connect(*observer, observerCallbacks, options, model, false);

  auto const federation = federationName(
      options, "unconditional-divestiture-assumption-coalescing");
  owner->createFederationExecution(
      federation, options.multiAttributeFom.wstring(),
      options.logicalTimeImplementationName);
  static_cast<void>(owner->joinFederationExecution(
      options.ownerFederateName + L"-assumption-owner", options.federateType,
      federation));
  static_cast<void>(candidate->joinFederationExecution(
      options.memberFederateName + L"-assumption-candidate", options.federateType,
      federation));
  static_cast<void>(observer->joinFederationExecution(
      options.memberFederateName + L"-assumption-observer", options.federateType,
      federation));

  auto const ownerClass = owner->getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const candidateClass = candidate->getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const observerClass = observer->getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const ownerFirst = owner->getAttributeHandle(
      ownerClass, options.multiAttributeFirstName);
  auto const ownerSecond = owner->getAttributeHandle(
      ownerClass, options.multiAttributeSecondName);
  auto const candidateFirst = candidate->getAttributeHandle(
      candidateClass, options.multiAttributeFirstName);
  auto const candidateSecond = candidate->getAttributeHandle(
      candidateClass, options.multiAttributeSecondName);
  auto const observerFirst = observer->getAttributeHandle(
      observerClass, options.multiAttributeFirstName);
  auto const observerSecond = observer->getAttributeHandle(
      observerClass, options.multiAttributeSecondName);
  require(ownerClass.isValid() && candidateClass.isValid() &&
              observerClass.isValid() && ownerFirst.isValid() &&
              ownerSecond.isValid() && candidateFirst.isValid() &&
              candidateSecond.isValid() && observerFirst.isValid() &&
              observerSecond.isValid() && ownerFirst != ownerSecond &&
              candidateFirst != candidateSecond && observerFirst != observerSecond,
          "Adapter FOM did not resolve two distinct ownership attributes");

  rti::AttributeHandleSet const ownerAttributes{ownerFirst, ownerSecond};
  rti::AttributeHandleSet const candidateAttributes{candidateFirst, candidateSecond};
  rti::AttributeHandleSet const observerAttributes{observerFirst, observerSecond};
  owner->publishObjectClassAttributes(ownerClass, ownerAttributes);
  candidate->publishObjectClassAttributes(candidateClass, candidateAttributes);
  candidate->subscribeObjectClassAttributes(
      candidateClass, candidateAttributes, true, L"");
  observer->subscribeObjectClassAttributes(
      observerClass, observerAttributes, true, L"");

  auto const object = owner->registerObjectInstance(ownerClass);
  require(object.isValid(),
          "Unconditional-divestiture assumption registration returned no object");
  evokeUntil(*candidate, [&] { return candidateCallbacks.discovered(object); },
             options, model, "eligible assumption candidate discovery");
  evokeUntil(*observer, [&] { return observerCallbacks.discovered(object); },
             options, model, "non-publishing observer discovery");

  std::vector<std::uint8_t> const tagBytes{0x74U, 0x43U, 0x4fU};
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  owner->unconditionalAttributeOwnershipDivestiture(object, ownerAttributes, tag);
  evokeUntil(*candidate,
             [&] { return !candidateCallbacks.assumptions().empty(); },
             options, model, "unconditional-divestiture assumption callback");
  if (model == rti::HLA_EVOKED) {
    while (owner->evokeCallback(0.0)) {
    }
    while (candidate->evokeCallback(0.0)) {
    }
    while (observer->evokeCallback(0.0)) {
    }
  }

  auto const offers = candidateCallbacks.assumptions();
  require(offers.size() == 1U && offers.front().object == object &&
              offers.front().attributes == candidateAttributes &&
              offers.front().tag == tagBytes,
          "Eligible federate did not receive one coalesced assumption for the full set");
  require(ownerCallbacks.assumptions().empty(),
          "Divesting federate was incorrectly asked to assume its own attributes");
  require(observerCallbacks.assumptions().empty(),
          "A non-publishing subscriber was incorrectly offered ownership");

  candidate->resignFederationExecution(rti::NO_ACTION);
  observer->resignFederationExecution(rti::NO_ACTION);
  owner->resignFederationExecution(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner->destroyFederationExecution(federation);
  observer->disconnect();
  candidate->disconnect();
  owner->disconnect();
}

void scenarioUnconditionalDivestitureAssumptionCoalescingContract(
    Options const& options, rti::CallbackModel model) {
  scenarioUnconditionalDivestitureAssumptionCoalescing(options, model);
}

void scenarioResignMultiSurvivorAssumptionFanout(
    Options const& options, rti::CallbackModel model) {
  require(!options.fom.empty(),
          "Multi-survivor resignation requires an adapter-supplied FOM");
  require(model == rti::HLA_EVOKED,
          "Independent callback-queue observation requires evoked callbacks");
  rti::RTIambassadorFactory factory;
  auto owner = factory.createRTIambassador();
  auto first = factory.createRTIambassador();
  auto second = factory.createRTIambassador();
  require(owner && first && second,
          "The standard RTIambassador factory returned no ambassador");
  AssumptionCallbacks ownerCallbacks;
  AssumptionCallbacks firstCallbacks;
  AssumptionCallbacks secondCallbacks;
  connect(*owner, ownerCallbacks, options, model, true);
  connect(*first, firstCallbacks, options, model, false);
  connect(*second, secondCallbacks, options, model, false);
  auto const federation = federationName(options, "resign-assumption-fanout");
  owner->createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  static_cast<void>(owner->joinFederationExecution(
      options.ownerFederateName + L"-fanout-owner", options.federateType, federation));
  static_cast<void>(first->joinFederationExecution(
      options.memberFederateName + L"-fanout-first", options.federateType, federation));
  static_cast<void>(second->joinFederationExecution(
      options.memberFederateName + L"-fanout-second", options.federateType, federation));

  auto const ownerClass = owner->getObjectClassHandle(options.objectClassName);
  auto const firstClass = first->getObjectClassHandle(options.objectClassName);
  auto const secondClass = second->getObjectClassHandle(options.objectClassName);
  auto const ownerAttribute = owner->getAttributeHandle(ownerClass, options.attributeName);
  auto const firstAttribute = first->getAttributeHandle(firstClass, options.attributeName);
  auto const secondAttribute = second->getAttributeHandle(secondClass, options.attributeName);
  auto const firstPrivilege = first->getAttributeHandle(
      firstClass, L"HLAprivilegeToDeleteObject");
  auto const secondPrivilege = second->getAttributeHandle(
      secondClass, L"HLAprivilegeToDeleteObject");
  require(ownerClass.isValid() && firstClass.isValid() && secondClass.isValid() &&
              ownerAttribute.isValid() && firstAttribute.isValid() &&
              secondAttribute.isValid() && firstPrivilege.isValid() &&
              secondPrivilege.isValid(),
          "Adapter FOM did not resolve the object attribute and standard delete privilege");
  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const firstAttributes{firstAttribute};
  rti::AttributeHandleSet const secondAttributes{secondAttribute};
  owner->publishObjectClassAttributes(ownerClass, ownerAttributes);
  first->subscribeObjectClassAttributes(firstClass, firstAttributes, true, L"");
  second->subscribeObjectClassAttributes(secondClass, secondAttributes, true, L"");
  auto const object = owner->registerObjectInstance(ownerClass);
  require(object.isValid(), "Multi-survivor registration returned no object");
  evokeUntil(*first, [&] { return firstCallbacks.discovered(object); },
             options, model, "first surviving federate discovery");
  evokeUntil(*second, [&] { return secondCallbacks.discovered(object); },
             options, model, "second surviving federate discovery");
  first->publishObjectClassAttributes(firstClass, firstAttributes);
  second->publishObjectClassAttributes(secondClass, secondAttributes);

  owner->resignFederationExecution(rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES);
  require(firstCallbacks.assumptions().empty() &&
              secondCallbacks.assumptions().empty(),
          "Evoked assumption callback arrived before either survivor was serviced");
  evokeUntil(*first, [&] { return firstCallbacks.assumptions().size() == 1U; },
             options, model, "first resignation assumption");
  auto const firstOffers = firstCallbacks.assumptions();
  require(firstOffers.size() == 1U && firstOffers.front().object == object &&
              firstOffers.front().attributes ==
                  rti::AttributeHandleSet{firstAttribute, firstPrivilege} &&
              firstOffers.front().tag.empty() &&
              secondCallbacks.assumptions().empty(),
          "First survivor did not receive its independent standard assumption offer");
  evokeUntil(*second, [&] { return secondCallbacks.assumptions().size() == 1U; },
             options, model, "second resignation assumption");
  auto const secondOffers = secondCallbacks.assumptions();
  require(secondOffers.size() == 1U && secondOffers.front().object == object &&
              secondOffers.front().attributes ==
                  rti::AttributeHandleSet{secondAttribute, secondPrivilege} &&
              secondOffers.front().tag.empty() &&
              firstCallbacks.assumptions().size() == 1U,
          "Second survivor did not receive its independent standard assumption offer");

  first->resignFederationExecution(rti::NO_ACTION);
  second->resignFederationExecution(rti::NO_ACTION);
  second->destroyFederationExecution(federation);
  second->disconnect();
  first->disconnect();
  owner->disconnect();
}

void scenarioResignMultiSurvivorAssumptionFanoutContract(
    Options const& options, rti::CallbackModel model) {
  scenarioResignMultiSurvivorAssumptionFanout(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
