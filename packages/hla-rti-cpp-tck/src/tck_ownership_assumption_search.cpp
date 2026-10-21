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
  require(owner && first && second,
          "The standard RTIambassador factory returned no ambassador");
  AssumptionCallbacks ownerCallbacks;
  AssumptionCallbacks firstCallbacks;
  AssumptionCallbacks secondCallbacks;
  connect(*owner, ownerCallbacks, options, model, true);
  connect(*first, firstCallbacks, options, model, false);
  connect(*second, secondCallbacks, options, model, false);
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

  auto const ownerClass = owner->getObjectClassHandle(options.objectClassName);
  auto const firstClass = first->getObjectClassHandle(options.objectClassName);
  auto const secondClass = second->getObjectClassHandle(options.objectClassName);
  auto const ownerAttribute = owner->getAttributeHandle(ownerClass, options.attributeName);
  auto const firstAttribute = first->getAttributeHandle(firstClass, options.attributeName);
  auto const secondAttribute = second->getAttributeHandle(secondClass, options.attributeName);
  require(ownerClass.isValid() && firstClass.isValid() && secondClass.isValid() &&
              ownerAttribute.isValid() && firstAttribute.isValid() &&
              secondAttribute.isValid(),
          "Ownership-assumption search lookup returned an invalid standard handle");
  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const firstAttributes{firstAttribute};
  rti::AttributeHandleSet const secondAttributes{secondAttribute};
  owner->publishObjectClassAttributes(ownerClass, ownerAttributes);
  first->subscribeObjectClassAttributes(firstClass, firstAttributes, true, L"");
  second->subscribeObjectClassAttributes(secondClass, secondAttributes, true, L"");
  auto const object = owner->registerObjectInstance(ownerClass);
  require(object.isValid(), "Ownership-assumption search registration returned an invalid handle");
  evokeUntil(*first, [&] { return firstCallbacks.discovered(object); }, options, model,
             "first assumption candidate discovery");
  evokeUntil(*second, [&] { return secondCallbacks.discovered(object); }, options, model,
             "second assumption candidate discovery");
  first->publishObjectClassAttributes(firstClass, firstAttributes);
  second->publishObjectClassAttributes(secondClass, secondAttributes);

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

  first->unpublishObjectClass(firstClass);
  second->unpublishObjectClass(secondClass);
  owner->resignFederationExecution(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  first->resignFederationExecution(rti::NO_ACTION);
  second->resignFederationExecution(rti::NO_ACTION);
  second->destroyFederationExecution(federation);
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

  first->attributeOwnershipAcquisition(object, transferred, acquisitionTag);
  evokeUntil(*first, [&] { return firstCallbacks.acquisitions().size() == 1U; },
             options, model, "first candidate acquisition notification");
  auto const acquisition = firstCallbacks.acquisitions().front();
  require(acquisition.object == object && acquisition.attributes == transferred &&
              first->isAttributeOwnedByFederate(object, firstAttribute),
          "First candidate did not acquire the transferred attribute set");

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
