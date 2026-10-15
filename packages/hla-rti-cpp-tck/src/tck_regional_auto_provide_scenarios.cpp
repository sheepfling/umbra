#include "tck_regional_auto_provide_scenarios.hpp"
#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

class RegionalAutoProvideAmbassador final : public rti::NullFederateAmbassador {
 public:
  struct Discovery final {
    rti::ObjectInstanceHandle object;
    rti::ObjectClassHandle objectClass;
    rti::FederateHandle producer;
  };

  struct Request final {
    rti::ObjectInstanceHandle object;
    rti::AttributeHandleSet attributes;
    std::vector<std::uint8_t> tag;
  };

  struct OwnershipAssumption final {
    rti::ObjectInstanceHandle object;
    rti::AttributeHandleSet attributes;
    std::vector<std::uint8_t> tag;
  };

  struct OwnershipAcquisition final {
    rti::ObjectInstanceHandle object;
    rti::AttributeHandleSet attributes;
    std::vector<std::uint8_t> tag;
  };

  struct Reflection final {
    rti::ObjectInstanceHandle object;
    rti::AttributeHandleValueMap values;
    std::vector<std::uint8_t> tag;
    rti::TransportationTypeHandle transportation;
    rti::FederateHandle producer;
    std::optional<rti::RegionHandleSet> regions;
    std::wstring timeImplementation;
    std::wstring timeValue;
    rti::OrderType sentOrder = rti::RECEIVE;
    rti::OrderType receivedOrder = rti::RECEIVE;
    bool retractionSupplied = false;
    bool retractionValid = false;
  };

  void discoverObjectInstance(
      rti::ObjectInstanceHandle const& object,
      rti::ObjectClassHandle const& objectClass,
      std::wstring const&,
      rti::FederateHandle const& producer) override {
    std::function<void()> hook;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      discoveries_.push_back(Discovery{object, objectClass, producer});
      hook = discoveryHook_;
    }
    if (hook) {
      hook();
    }
  }

  void provideAttributeValueUpdate(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleSet const& attributes,
      rti::VariableLengthData const& tag) override {
    std::function<void(
        rti::ObjectInstanceHandle const&,
        rti::AttributeHandleSet const&,
        rti::VariableLengthData const&)> hook;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      requests_.push_back(Request{object, attributes, copyBytes(tag)});
      hook = requestHook_;
    }
    if (hook) {
      hook(object, attributes, tag);
    }
  }

  void requestAttributeOwnershipAssumption(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleSet const& attributes,
      rti::VariableLengthData const& tag) override {
    std::lock_guard<std::mutex> lock(mutex_);
    ownershipAssumptions_.push_back(
        OwnershipAssumption{object, attributes, copyBytes(tag)});
  }

  void attributeOwnershipAcquisitionNotification(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleSet const& attributes,
      rti::VariableLengthData const& tag) override {
    std::lock_guard<std::mutex> lock(mutex_);
    ownershipAcquisitions_.push_back(
        OwnershipAcquisition{object, attributes, copyBytes(tag)});
  }

  void reflectAttributeValues(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleValueMap const& values,
      rti::VariableLengthData const& tag,
      rti::TransportationTypeHandle const& transportation,
      rti::FederateHandle const& producer,
      rti::RegionHandleSet const* regions) override {
    std::function<void()> hook;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      reflections_.push_back(Reflection{
          object,
          values,
          copyBytes(tag),
          transportation,
          producer,
          regions == nullptr
              ? std::nullopt
              : std::optional<rti::RegionHandleSet>(*regions),
      });
      hook = reflectionHook_;
    }
    if (hook) {
      hook();
    }
  }

  void reflectAttributeValues(
      rti::ObjectInstanceHandle const& object,
      rti::AttributeHandleValueMap const& values,
      rti::VariableLengthData const& tag,
      rti::TransportationTypeHandle const& transportation,
      rti::FederateHandle const& producer,
      rti::RegionHandleSet const* regions,
      rti::LogicalTime const& time,
      rti::OrderType sentOrder,
      rti::OrderType receivedOrder,
      rti::MessageRetractionHandle const* retraction) override {
    std::function<void()> hook;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      reflections_.push_back(Reflection{
          object,
          values,
          copyBytes(tag),
          transportation,
          producer,
          regions == nullptr
              ? std::nullopt
              : std::optional<rti::RegionHandleSet>(*regions),
          time.implementationName(),
          time.toString(),
          sentOrder,
          receivedOrder,
          retraction != nullptr,
          retraction != nullptr && retraction->isValid(),
      });
      hook = reflectionHook_;
    }
    if (hook) {
      hook();
    }
  }

  void timeConstrainedEnabled(rti::LogicalTime const& time) override {
    std::lock_guard<std::mutex> lock(mutex_);
    timeConstrainedTimes_.push_back(time.toString());
  }

  void timeRegulationEnabled(rti::LogicalTime const& time) override {
    std::lock_guard<std::mutex> lock(mutex_);
    timeRegulationTimes_.push_back(time.toString());
  }

  void timeAdvanceGrant(rti::LogicalTime const& time) override {
    std::function<void()> hook;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      timeAdvanceGrantTimes_.push_back(time.toString());
      hook = timeAdvanceGrantHook_;
    }
    if (hook) {
      hook();
    }
  }

  std::vector<Discovery> discoveries() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return discoveries_;
  }

  std::vector<Request> requests() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return requests_;
  }

  std::vector<OwnershipAssumption> ownershipAssumptions() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return ownershipAssumptions_;
  }

  std::vector<OwnershipAcquisition> ownershipAcquisitions() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return ownershipAcquisitions_;
  }

  std::vector<Reflection> reflections() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return reflections_;
  }

  std::vector<std::wstring> timeConstrainedTimes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return timeConstrainedTimes_;
  }

  std::vector<std::wstring> timeRegulationTimes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return timeRegulationTimes_;
  }

  std::vector<std::wstring> timeAdvanceGrantTimes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return timeAdvanceGrantTimes_;
  }

  void setDiscoveryHook(std::function<void()> hook) {
    std::lock_guard<std::mutex> lock(mutex_);
    discoveryHook_ = std::move(hook);
  }

  void setReflectionHook(std::function<void()> hook) {
    std::lock_guard<std::mutex> lock(mutex_);
    reflectionHook_ = std::move(hook);
  }

  void setTimeAdvanceGrantHook(std::function<void()> hook) {
    std::lock_guard<std::mutex> lock(mutex_);
    timeAdvanceGrantHook_ = std::move(hook);
  }

  void setRequestHook(
      std::function<void(
          rti::ObjectInstanceHandle const&,
          rti::AttributeHandleSet const&,
          rti::VariableLengthData const&)> hook) {
    std::lock_guard<std::mutex> lock(mutex_);
    requestHook_ = std::move(hook);
  }

 private:
  mutable std::mutex mutex_;
  std::vector<Discovery> discoveries_;
  std::vector<Request> requests_;
  std::vector<OwnershipAssumption> ownershipAssumptions_;
  std::vector<OwnershipAcquisition> ownershipAcquisitions_;
  std::vector<Reflection> reflections_;
  std::vector<std::wstring> timeConstrainedTimes_;
  std::vector<std::wstring> timeRegulationTimes_;
  std::vector<std::wstring> timeAdvanceGrantTimes_;
  std::function<void()> discoveryHook_;
  std::function<void()> reflectionHook_;
  std::function<void()> timeAdvanceGrantHook_;
  std::function<void(
      rti::ObjectInstanceHandle const&,
      rti::AttributeHandleSet const&,
      rti::VariableLengthData const&)> requestHook_;
};

class RegionalAutoProvideSession final {
 public:
  RegionalAutoProvideSession(
      Options const& options,
      rti::CallbackModel callbackModel,
      std::string role,
      RegionalAutoProvideAmbassador& ambassador)
      : options_(options),
        callbackModel_(callbackModel),
        role_(std::move(role)),
        ambassador_(ambassador) {
    rti::RTIambassadorFactory factory;
    rti_ = factory.createRTIambassador();
    require(rti_ != nullptr, "regional Auto Provide factory returned no ambassador");
  }

  ~RegionalAutoProvideSession() {
    if (std::uncaught_exceptions() != 0) {
      return;
    }
    if (joined_) {
      try {
        rti_->resignFederationExecution(rti::NO_ACTION);
      } catch (...) {
      }
    }
    if (connected_) {
      try {
        rti_->disconnect();
      } catch (...) {
      }
    }
  }

  void connect() {
    auto configurationName = options_.configurationName;
    if (role_ == "owner" && !options_.ownerConfigurationName.empty()) {
      configurationName = options_.ownerConfigurationName;
    } else if (role_ != "owner" && !options_.memberConfigurationName.empty()) {
      configurationName = options_.memberConfigurationName;
    }
    if (configurationName.empty() && options_.rtiAddress.empty() &&
        options_.additionalSettings.empty()) {
      static_cast<void>(rti_->connect(ambassador_, callbackModel_));
    } else {
      auto configuration = rti::RtiConfiguration::createConfiguration();
      if (!configurationName.empty()) {
        configuration.withConfigurationName(toWide(configurationName));
      }
      if (!options_.rtiAddress.empty()) {
        configuration.withRtiAddress(toWide(options_.rtiAddress));
      }
      if (!options_.additionalSettings.empty()) {
        configuration.withAdditionalSettings(toWide(options_.additionalSettings));
      }
      static_cast<void>(rti_->connect(ambassador_, callbackModel_, configuration));
    }
    connected_ = true;
  }

  void join(
      std::wstring const& federateName,
      std::wstring const& federateType,
      std::wstring const& federation) {
    federateHandle_ = rti_->joinFederationExecution(
        federateName,
        federateType,
        federation);
    require(
        federateHandle_.isValid(),
        "regional Auto Provide join returned an invalid federate handle");
    joined_ = true;
  }

  void resign(rti::ResignAction action) {
    if (!joined_) {
      return;
    }
    rti_->resignFederationExecution(action);
    joined_ = false;
  }

  void disconnect() {
    if (!connected_) {
      return;
    }
    rti_->disconnect();
    connected_ = false;
  }

  void pump() {
    if (connected_ && callbackModel_ == rti::HLA_EVOKED) {
      static_cast<void>(rti_->evokeCallback(0.0));
    }
  }

  rti::RTIambassador& rtiAmbassador() { return *rti_; }
  rti::FederateHandle const& federateHandle() const { return federateHandle_; }

 private:
  Options const& options_;
  rti::CallbackModel callbackModel_;
  std::string role_;
  RegionalAutoProvideAmbassador& ambassador_;
  std::unique_ptr<rti::RTIambassador> rti_;
  rti::FederateHandle federateHandle_;
  bool connected_ = false;
  bool joined_ = false;
};

void scenarioRegionalAutoProvideResponse(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.autoProvideFom.empty(),
      "regional Auto Provide testing requires an adapter-supplied Auto Provide FOM");
  require(
      !options.ddmDimensionNames.empty(),
      "regional Auto Provide testing requires an adapter-supplied dimension name");

  RegionalAutoProvideAmbassador ownerReports;
  RegionalAutoProvideAmbassador requesterReports;
  RegionalAutoProvideSession owner(options, model, "owner", ownerReports);
  RegionalAutoProvideSession requester(
      options,
      model,
      "requester",
      requesterReports);
  owner.connect();
  requester.connect();
  auto const federation = federationName(options, "regional-auto-provide-response");
  owner.rtiAmbassador().createFederationExecution(
      federation,
      options.autoProvideFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);
  requester.join(options.memberFederateName, options.federateType, federation);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.autoProvideObjectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.autoProvideFirstAttributeName);
  auto const ownerDimension = owner.rtiAmbassador().getDimensionHandle(
      options.ddmDimensionNames.front());
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.autoProvideObjectClassName);
  auto const requesterAttribute = requester.rtiAmbassador().getAttributeHandle(
      requesterClass,
      options.autoProvideFirstAttributeName);
  auto const requesterDimension = requester.rtiAmbassador().getDimensionHandle(
      options.ddmDimensionNames.front());
  require(
      ownerClass.isValid() && ownerAttribute.isValid() && ownerDimension.isValid() &&
          requesterClass.isValid() && requesterAttribute.isValid() &&
          requesterDimension.isValid(),
      "regional Auto Provide lookup returned an invalid standard handle");
  require(
      owner.rtiAmbassador().getAutoProvideSwitch() &&
          requester.rtiAmbassador().getAutoProvideSwitch(),
      "regional Auto Provide FOM did not enable the standard Auto Provide switch");
  require(
      owner.rtiAmbassador().getAvailableDimensionsForObjectClass(ownerClass) ==
              rti::DimensionHandleSet{ownerDimension} &&
          requester.rtiAmbassador().getAvailableDimensionsForObjectClass(requesterClass) ==
              rti::DimensionHandleSet{requesterDimension},
      "regional Auto Provide object class did not expose its configured dimension");
  require(
      owner.rtiAmbassador().getDimensionUpperBound(ownerDimension) >= 3U &&
          requester.rtiAmbassador().getDimensionUpperBound(requesterDimension) >= 3U,
      "regional Auto Provide dimension has insufficient range for the overlap case");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const requesterAttributes{requesterAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  auto const ownerRegion = owner.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{ownerDimension});
  auto const requesterRegion = requester.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{requesterDimension});
  require(
      ownerRegion.isValid() && requesterRegion.isValid(),
      "regional Auto Provide region creation returned an invalid handle");
  owner.rtiAmbassador().setRangeBounds(
      ownerRegion,
      ownerDimension,
      rti::RangeBounds(0UL, 2UL));
  requester.rtiAmbassador().setRangeBounds(
      requesterRegion,
      requesterDimension,
      rti::RangeBounds(1UL, 3UL));
  owner.rtiAmbassador().commitRegionModifications(rti::RegionHandleSet{ownerRegion});
  requester.rtiAmbassador().commitRegionModifications(
      rti::RegionHandleSet{requesterRegion});
  rti::AttributeHandleSetRegionHandleSetPairVector const ownerPair{{
      ownerAttributes,
      rti::RegionHandleSet{ownerRegion},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const requesterPair{{
      requesterAttributes,
      rti::RegionHandleSet{requesterRegion},
  }};
  requester.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  require(
      requester.rtiAmbassador().getConveyRegionDesignatorSetsSwitch(),
      "regional Auto Provide requester did not enable region designator callbacks");

  std::vector<std::string> callbackOrder;
  std::vector<std::uint8_t> observedRequestTag;
  std::vector<std::uint8_t> const responseValue{0x52U, 0x45U, 0x53U};
  std::vector<std::uint8_t> const responseTag{0x41U, 0x50U, 0x52U};
  ownerReports.setRequestHook(
      [&](rti::ObjectInstanceHandle const& object,
          rti::AttributeHandleSet const& attributes,
          rti::VariableLengthData const& tag) {
        callbackOrder.push_back("provide");
        observedRequestTag = copyBytes(tag);
        rti::AttributeHandleValueMap values;
        for (auto const& attribute : attributes) {
          require(
              attribute == ownerAttribute,
              "regional Auto Provide requested an unexpected attribute");
          values.emplace(
              attribute,
              rti::VariableLengthData(responseValue.data(), responseValue.size()));
        }
        rti::VariableLengthData updateTag(responseTag.data(), responseTag.size());
        owner.rtiAmbassador().updateAttributeValues(object, values, updateTag);
      });
  requesterReports.setDiscoveryHook([&] { callbackOrder.push_back("discover"); });
  requesterReports.setReflectionHook([&] { callbackOrder.push_back("reflect"); });

  auto const object = owner.rtiAmbassador().registerObjectInstanceWithRegions(
      ownerClass,
      ownerPair);
  require(object.isValid(), "regional Auto Provide registration returned an invalid handle");
  requester.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      requesterClass,
      requesterPair);

  auto const deadline = Clock::now() +
      std::chrono::milliseconds(options.timeoutMilliseconds);
  while (Clock::now() < deadline) {
    requester.pump();
    owner.pump();
    if (requesterReports.discoveries().size() == 1U &&
        ownerReports.requests().size() == 1U &&
        requesterReports.reflections().size() == 1U) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  auto const discoveries = requesterReports.discoveries();
  auto const requests = ownerReports.requests();
  auto const reflections = requesterReports.reflections();
  require(
      discoveries.size() == 1U && requests.size() == 1U && reflections.size() == 1U,
      "regional Auto Provide did not complete one discovery, request, and reflection");
  require(
      callbackOrder == std::vector<std::string>{"discover", "provide", "reflect"},
      "regional Auto Provide callback order was not discovery, provide, reflect");
  require(
      discoveries.front().object == object &&
          discoveries.front().objectClass == requesterClass &&
          discoveries.front().producer == owner.federateHandle(),
      "regional Auto Provide discovery returned the wrong object metadata");
  require(
      requests.front().object == object &&
          requests.front().attributes == ownerAttributes &&
          observedRequestTag.empty(),
      "regional Auto Provide request returned the wrong object, attributes, or tag");
  auto const& reflection = reflections.front();
  require(
      reflection.object == object && reflection.values.size() == 1U &&
          reflection.values.count(requesterAttribute) == 1U &&
          copyBytes(reflection.values.at(requesterAttribute)) == responseValue &&
          reflection.tag == responseTag && reflection.transportation ==
              owner.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable") &&
          reflection.producer == owner.federateHandle() && reflection.regions.has_value() &&
          reflection.regions.value() == rti::RegionHandleSet{ownerRegion},
      "regional Auto Provide reflection did not preserve scoped value metadata");

  requester.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      requesterClass,
      requesterPair);
  owner.rtiAmbassador().unassociateRegionsForUpdates(object, ownerPair);
  requester.rtiAmbassador().deleteRegion(requesterRegion);
  owner.rtiAmbassador().deleteRegion(ownerRegion);
  requester.resign(rti::NO_ACTION);
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  owner.disconnect();
  requester.disconnect();
}

void scenarioRegionalAutoProvideResponseContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioRegionalAutoProvideResponse(options, model);
}

void scenarioRegionalAutoProvideTimestampedResponse(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.autoProvideFom.empty(),
      "timestamped regional Auto Provide testing requires an adapter-supplied Auto Provide FOM");
  require(
      !options.ddmDimensionNames.empty(),
      "timestamped regional Auto Provide testing requires an adapter-supplied dimension name");
  require(
      !options.logicalTimeImplementationName.empty(),
      "timestamped regional Auto Provide testing requires an adapter-supplied logical-time implementation");

  RegionalAutoProvideAmbassador ownerReports;
  RegionalAutoProvideAmbassador requesterReports;
  RegionalAutoProvideSession owner(options, model, "owner", ownerReports);
  RegionalAutoProvideSession requester(
      options,
      model,
      "requester",
      requesterReports);
  owner.connect();
  requester.connect();
  auto const federation = federationName(
      options,
      "regional-auto-provide-timestamped-response");
  owner.rtiAmbassador().createFederationExecution(
      federation,
      options.autoProvideFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);
  requester.join(options.memberFederateName, options.federateType, federation);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.autoProvideObjectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.autoProvideFirstAttributeName);
  auto const ownerDimension = owner.rtiAmbassador().getDimensionHandle(
      options.ddmDimensionNames.front());
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.autoProvideObjectClassName);
  auto const requesterAttribute = requester.rtiAmbassador().getAttributeHandle(
      requesterClass,
      options.autoProvideFirstAttributeName);
  auto const requesterDimension = requester.rtiAmbassador().getDimensionHandle(
      options.ddmDimensionNames.front());
  require(
      ownerClass.isValid() && ownerAttribute.isValid() && ownerDimension.isValid() &&
          requesterClass.isValid() && requesterAttribute.isValid() &&
          requesterDimension.isValid(),
      "timestamped regional Auto Provide lookup returned an invalid standard handle");
  require(
      owner.rtiAmbassador().getAutoProvideSwitch() &&
          requester.rtiAmbassador().getAutoProvideSwitch(),
      "timestamped regional Auto Provide FOM did not enable the standard Auto Provide switch");
  require(
      owner.rtiAmbassador().getAvailableDimensionsForObjectClass(ownerClass) ==
              rti::DimensionHandleSet{ownerDimension} &&
          requester.rtiAmbassador().getAvailableDimensionsForObjectClass(requesterClass) ==
              rti::DimensionHandleSet{requesterDimension},
      "timestamped regional Auto Provide object class did not expose its configured dimension");
  require(
      owner.rtiAmbassador().getDimensionUpperBound(ownerDimension) >= 3U &&
          requester.rtiAmbassador().getDimensionUpperBound(requesterDimension) >= 3U,
      "timestamped regional Auto Provide dimension has insufficient range for the overlap case");

  auto ownerTimeFactory = owner.rtiAmbassador().getTimeFactory();
  auto requesterTimeFactory = requester.rtiAmbassador().getTimeFactory();
  require(
      ownerTimeFactory != nullptr && requesterTimeFactory != nullptr &&
          ownerTimeFactory->getName() == options.logicalTimeImplementationName &&
          requesterTimeFactory->getName() == options.logicalTimeImplementationName,
      "timestamped regional Auto Provide selected the wrong logical-time factory");
  auto ownerInitial = ownerTimeFactory->makeInitial();
  auto ownerEpsilon = ownerTimeFactory->makeEpsilon();
  auto requesterInitial = requesterTimeFactory->makeInitial();
  auto requesterEpsilon = requesterTimeFactory->makeEpsilon();
  require(
      ownerInitial != nullptr && ownerEpsilon != nullptr &&
          requesterInitial != nullptr && requesterEpsilon != nullptr,
      "timestamped regional Auto Provide logical-time factory returned incomplete boundaries");
  auto responseTime = timeAfter(
      *ownerTimeFactory,
      *ownerInitial,
      *ownerEpsilon,
      2U);
  auto requesterTarget = timeAfter(
      *requesterTimeFactory,
      *requesterInitial,
      *requesterEpsilon,
      2U);

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const requesterAttributes{requesterAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  owner.rtiAmbassador().changeDefaultAttributeOrderType(
      ownerClass,
      ownerAttributes,
      rti::TIMESTAMP);
  auto const ownerRegion = owner.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{ownerDimension});
  auto const requesterRegion = requester.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{requesterDimension});
  require(
      ownerRegion.isValid() && requesterRegion.isValid(),
      "timestamped regional Auto Provide region creation returned an invalid handle");
  owner.rtiAmbassador().setRangeBounds(
      ownerRegion,
      ownerDimension,
      rti::RangeBounds(0UL, 2UL));
  requester.rtiAmbassador().setRangeBounds(
      requesterRegion,
      requesterDimension,
      rti::RangeBounds(1UL, 3UL));
  owner.rtiAmbassador().commitRegionModifications(
      rti::RegionHandleSet{ownerRegion});
  requester.rtiAmbassador().commitRegionModifications(
      rti::RegionHandleSet{requesterRegion});
  rti::AttributeHandleSetRegionHandleSetPairVector const ownerPair{{
      ownerAttributes,
      rti::RegionHandleSet{ownerRegion},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const requesterPair{{
      requesterAttributes,
      rti::RegionHandleSet{requesterRegion},
  }};
  requester.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  require(
      requester.rtiAmbassador().getConveyRegionDesignatorSetsSwitch(),
      "timestamped regional Auto Provide requester did not enable region designator callbacks");

  requester.rtiAmbassador().enableTimeConstrained();
  owner.rtiAmbassador().enableTimeRegulation(*ownerEpsilon);
  auto const timeCallbackDeadline = Clock::now() +
      std::chrono::milliseconds(options.timeoutMilliseconds);
  while (Clock::now() < timeCallbackDeadline &&
         (requesterReports.timeConstrainedTimes().empty() ||
          ownerReports.timeRegulationTimes().empty())) {
    requester.pump();
    owner.pump();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  require(
      requesterReports.timeConstrainedTimes().size() == 1U &&
          ownerReports.timeRegulationTimes().size() == 1U,
      "timestamped regional Auto Provide did not enable both time-management roles");

  std::vector<std::string> callbackOrder;
  std::vector<std::string> requesterCallbackOrder;
  std::vector<std::uint8_t> observedRequestTag;
  std::vector<std::uint8_t> const responseValue{0x52U, 0x45U, 0x47U};
  std::vector<std::uint8_t> const responseTag{0x52U, 0x41U, 0x54U, 0x53U};
  rti::MessageRetractionHandle responseRetraction;
  ownerReports.setRequestHook(
      [&](rti::ObjectInstanceHandle const& object,
          rti::AttributeHandleSet const& attributes,
          rti::VariableLengthData const& tag) {
        callbackOrder.push_back("provide");
        observedRequestTag = copyBytes(tag);
        rti::AttributeHandleValueMap values;
        for (auto const& attribute : attributes) {
          require(
              attribute == ownerAttribute,
              "timestamped regional Auto Provide requested an unexpected attribute");
          values.emplace(
              attribute,
              rti::VariableLengthData(responseValue.data(), responseValue.size()));
        }
        rti::VariableLengthData updateTag(responseTag.data(), responseTag.size());
        responseRetraction = owner.rtiAmbassador().updateAttributeValues(
            object,
            values,
            updateTag,
            *responseTime);
      });
  requesterReports.setDiscoveryHook([&] { callbackOrder.push_back("discover"); });
  requesterReports.setReflectionHook([&] {
    callbackOrder.push_back("reflect");
    requesterCallbackOrder.push_back("reflect");
  });
  requesterReports.setTimeAdvanceGrantHook(
      [&] { requesterCallbackOrder.push_back("grant"); });

  auto const object = owner.rtiAmbassador().registerObjectInstanceWithRegions(
      ownerClass,
      ownerPair);
  require(
      object.isValid(),
      "timestamped regional Auto Provide registration returned an invalid handle");
  requester.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      requesterClass,
      requesterPair);

  auto const solicitationDeadline = Clock::now() +
      std::chrono::milliseconds(options.timeoutMilliseconds);
  while (Clock::now() < solicitationDeadline &&
         (requesterReports.discoveries().size() != 1U ||
          ownerReports.requests().size() != 1U)) {
    requester.pump();
    owner.pump();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  auto const discoveries = requesterReports.discoveries();
  auto const requests = ownerReports.requests();
  require(
      discoveries.size() == 1U && requests.size() == 1U &&
          requesterReports.reflections().empty(),
      "timestamped regional Auto Provide delivered an unexpected pre-grant reflection");
  require(
      callbackOrder == std::vector<std::string>{"discover", "provide"},
      "timestamped regional Auto Provide callback order was not discovery, provide");
  require(responseRetraction.isValid(), "timestamped regional Auto Provide returned no retraction handle");
  require(
      discoveries.front().object == object &&
          discoveries.front().objectClass == requesterClass &&
          discoveries.front().producer == owner.federateHandle(),
      "timestamped regional Auto Provide discovery returned the wrong object metadata");
  require(
      requests.front().object == object &&
          requests.front().attributes == ownerAttributes &&
          observedRequestTag.empty(),
      "timestamped regional Auto Provide request returned the wrong object, attributes, or tag");

  requester.rtiAmbassador().timeAdvanceRequest(*requesterTarget);
  owner.rtiAmbassador().timeAdvanceRequest(*responseTime);
  auto const deliveryDeadline = Clock::now() +
      std::chrono::milliseconds(options.timeoutMilliseconds);
  while (Clock::now() < deliveryDeadline &&
         (requesterReports.reflections().size() != 1U ||
          requesterReports.timeAdvanceGrantTimes().size() != 1U ||
          ownerReports.timeAdvanceGrantTimes().size() != 1U)) {
    owner.pump();
    requester.pump();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  auto const reflections = requesterReports.reflections();
  require(
      reflections.size() == 1U &&
          requesterReports.timeAdvanceGrantTimes().size() == 1U &&
          ownerReports.timeAdvanceGrantTimes().size() == 1U,
      "timestamped regional Auto Provide did not complete one reflection and two grants");
  require(
      callbackOrder == std::vector<std::string>{"discover", "provide", "reflect"} &&
          requesterCallbackOrder == std::vector<std::string>{"reflect", "grant"},
      "timestamped regional Auto Provide callback order was not reflect before grant");
  auto const& reflection = reflections.front();
  require(
      reflection.object == object && reflection.values.size() == 1U &&
          reflection.values.count(requesterAttribute) == 1U &&
          copyBytes(reflection.values.at(requesterAttribute)) == responseValue &&
          reflection.tag == responseTag &&
          reflection.transportation ==
              owner.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable") &&
          reflection.producer == owner.federateHandle() && reflection.regions.has_value() &&
          reflection.regions.value() == rti::RegionHandleSet{ownerRegion} &&
          reflection.timeImplementation == ownerTimeFactory->getName() &&
          reflection.timeValue == responseTime->toString() &&
          reflection.sentOrder == rti::TIMESTAMP &&
          reflection.receivedOrder == rti::TIMESTAMP &&
          reflection.retractionSupplied && reflection.retractionValid,
      "timestamped regional Auto Provide reflection did not preserve standard time metadata");
  requireException(
      [&] { owner.rtiAmbassador().retract(responseRetraction); },
      L"MessageCanNoLongerBeRetracted",
      "retracting a delivered timestamped regional Auto Provide response");

  requester.rtiAmbassador().disableTimeConstrained();
  owner.rtiAmbassador().disableTimeRegulation();
  requester.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      requesterClass,
      requesterPair);
  owner.rtiAmbassador().unassociateRegionsForUpdates(object, ownerPair);
  requester.rtiAmbassador().deleteRegion(requesterRegion);
  owner.rtiAmbassador().deleteRegion(ownerRegion);
  requester.resign(rti::NO_ACTION);
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  owner.disconnect();
  requester.disconnect();
}

void scenarioRegionalAutoProvideTimestampedResponseContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioRegionalAutoProvideTimestampedResponse(options, model);
}

void scenarioRegionalAutoProvideOverlap(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.autoProvideFom.empty(),
      "regional Auto Provide overlap testing requires an adapter-supplied Auto Provide FOM");
  require(
      !options.ddmDimensionNames.empty(),
      "regional Auto Provide overlap testing requires an adapter-supplied dimension name");

  RegionalAutoProvideAmbassador ownerReports;
  RegionalAutoProvideAmbassador requesterReports;
  RegionalAutoProvideSession owner(options, model, "owner", ownerReports);
  RegionalAutoProvideSession requester(
      options,
      model,
      "requester",
      requesterReports);
  owner.connect();
  requester.connect();
  auto const federation = federationName(
      options,
      "regional-auto-provide-overlap");
  owner.rtiAmbassador().createFederationExecution(
      federation,
      options.autoProvideFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);
  requester.join(options.memberFederateName, options.federateType, federation);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.autoProvideObjectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.autoProvideFirstAttributeName);
  auto const ownerDimension = owner.rtiAmbassador().getDimensionHandle(
      options.ddmDimensionNames.front());
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.autoProvideObjectClassName);
  auto const requesterAttribute = requester.rtiAmbassador().getAttributeHandle(
      requesterClass,
      options.autoProvideFirstAttributeName);
  auto const requesterDimension = requester.rtiAmbassador().getDimensionHandle(
      options.ddmDimensionNames.front());
  require(
      ownerClass.isValid() && ownerAttribute.isValid() && ownerDimension.isValid() &&
          requesterClass.isValid() && requesterAttribute.isValid() &&
          requesterDimension.isValid(),
      "regional Auto Provide overlap lookup returned an invalid standard handle");
  require(
      owner.rtiAmbassador().getAutoProvideSwitch() &&
          requester.rtiAmbassador().getAutoProvideSwitch(),
      "regional Auto Provide overlap FOM did not enable the standard Auto Provide switch");
  require(
      owner.rtiAmbassador().getAvailableDimensionsForObjectClass(ownerClass) ==
              rti::DimensionHandleSet{ownerDimension} &&
          requester.rtiAmbassador().getAvailableDimensionsForObjectClass(requesterClass) ==
              rti::DimensionHandleSet{requesterDimension},
      "regional Auto Provide overlap object class did not expose its configured dimension");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const requesterAttributes{requesterAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  auto const ownerRegion = owner.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{ownerDimension});
  auto const requesterRegion = requester.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{requesterDimension});
  require(
      ownerRegion.isValid() && requesterRegion.isValid(),
      "regional Auto Provide overlap region creation returned an invalid handle");
  owner.rtiAmbassador().setRangeBounds(
      ownerRegion,
      ownerDimension,
      rti::RangeBounds(0UL, 2UL));
  requester.rtiAmbassador().setRangeBounds(
      requesterRegion,
      requesterDimension,
      rti::RangeBounds(1UL, 3UL));
  owner.rtiAmbassador().commitRegionModifications(
      rti::RegionHandleSet{ownerRegion});
  requester.rtiAmbassador().commitRegionModifications(
      rti::RegionHandleSet{requesterRegion});
  rti::AttributeHandleSetRegionHandleSetPairVector const ownerPair{{
      ownerAttributes,
      rti::RegionHandleSet{ownerRegion},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const requesterPair{{
      requesterAttributes,
      rti::RegionHandleSet{requesterRegion},
  }};
  requester.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  require(
      requester.rtiAmbassador().getConveyRegionDesignatorSetsSwitch(),
      "regional Auto Provide overlap requester did not enable region designator callbacks");

  std::vector<std::string> callbackOrder;
  bool moveDiscoveryOutOfScope = false;
  ownerReports.setRequestHook(
      [&](rti::ObjectInstanceHandle const&,
          rti::AttributeHandleSet const&,
          rti::VariableLengthData const&) {
        callbackOrder.push_back("provide");
      });
  requesterReports.setDiscoveryHook([&] {
    callbackOrder.push_back("discover");
    if (moveDiscoveryOutOfScope) {
      requester.rtiAmbassador().setRangeBounds(
          requesterRegion,
          requesterDimension,
          rti::RangeBounds(4UL, 5UL));
      requester.rtiAmbassador().commitRegionModifications(
          rti::RegionHandleSet{requesterRegion});
    }
  });

  auto pumpUntil = [&](std::function<bool()> const& condition,
                       std::string const& description) {
    auto const deadline = Clock::now() +
        std::chrono::milliseconds(options.timeoutMilliseconds);
    while (Clock::now() < deadline && !condition()) {
      requester.pump();
      owner.pump();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    require(condition(), description);
  };

  auto const firstObject = owner.rtiAmbassador().registerObjectInstanceWithRegions(
      ownerClass,
      ownerPair);
  require(
      firstObject.isValid(),
      "regional Auto Provide overlap first registration returned an invalid handle");
  requester.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      requesterClass,
      requesterPair);
  pumpUntil(
      [&] {
        return requesterReports.discoveries().size() == 1U &&
            ownerReports.requests().size() == 1U;
      },
      "regional Auto Provide overlap initial discovery and solicitation");
  auto const firstDiscoveries = requesterReports.discoveries();
  auto const firstRequests = ownerReports.requests();
  require(
      firstDiscoveries.size() == 1U && firstRequests.size() == 1U &&
          firstDiscoveries.front().object == firstObject &&
          firstDiscoveries.front().objectClass == requesterClass &&
          firstDiscoveries.front().producer == owner.federateHandle(),
      "regional Auto Provide overlap initial discovery returned the wrong metadata");
  require(
      firstRequests.front().object == firstObject &&
          firstRequests.front().attributes == ownerAttributes &&
          firstRequests.front().tag.empty(),
      "regional Auto Provide overlap initial solicitation returned the wrong metadata");

  moveDiscoveryOutOfScope = true;
  auto const secondObject = owner.rtiAmbassador().registerObjectInstanceWithRegions(
      ownerClass,
      ownerPair);
  require(
      secondObject.isValid(),
      "regional Auto Provide overlap second registration returned an invalid handle");
  pumpUntil(
      [&] { return requesterReports.discoveries().size() == 2U; },
      "regional Auto Provide overlap second discovery");
  auto const secondDiscoveries = requesterReports.discoveries();
  auto const secondRequests = ownerReports.requests();
  require(
      secondDiscoveries.size() == 2U &&
          secondDiscoveries.back().object == secondObject &&
          secondRequests.size() == 1U,
      "regional Auto Provide overlap issued stale solicitation after the subscription moved out of scope");
  require(
      std::count(callbackOrder.begin(), callbackOrder.end(), "discover") == 2 &&
          std::count(callbackOrder.begin(), callbackOrder.end(), "provide") == 1,
      "regional Auto Provide overlap delivered an unexpected callback count");

  requester.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      requesterClass,
      requesterPair);
  owner.rtiAmbassador().unassociateRegionsForUpdates(firstObject, ownerPair);
  owner.rtiAmbassador().unassociateRegionsForUpdates(secondObject, ownerPair);
  requester.rtiAmbassador().deleteRegion(requesterRegion);
  owner.rtiAmbassador().deleteRegion(ownerRegion);
  requester.resign(rti::NO_ACTION);
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  owner.disconnect();
  requester.disconnect();
}

void scenarioRegionalAutoProvideOverlapContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioRegionalAutoProvideOverlap(options, model);
}

void scenarioRegionalAutoProvideMultiSource(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.autoProvideFom.empty(),
      "regional Auto Provide multi-source testing requires an adapter-supplied Auto Provide FOM");
  require(
      !options.ddmDimensionNames.empty(),
      "regional Auto Provide multi-source testing requires an adapter-supplied dimension name");
  require(
      !options.autoProvideSecondAttributeName.empty(),
      "regional Auto Provide multi-source testing requires two adapter-supplied attributes");

  RegionalAutoProvideAmbassador ownerReports;
  RegionalAutoProvideAmbassador flavorReports;
  RegionalAutoProvideAmbassador organicReports;
  RegionalAutoProvideSession owner(options, model, "owner", ownerReports);
  RegionalAutoProvideSession flavor(options, model, "flavor", flavorReports);
  RegionalAutoProvideSession organic(options, model, "organic", organicReports);
  owner.connect();
  flavor.connect();
  organic.connect();
  auto const federation = federationName(
      options,
      "regional-auto-provide-multi-source");
  owner.rtiAmbassador().createFederationExecution(
      federation,
      options.autoProvideFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);
  flavor.join(
      options.memberFederateName + L"-flavor",
      options.federateType,
      federation);
  organic.join(
      options.memberFederateName + L"-organic",
      options.federateType,
      federation);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.autoProvideObjectClassName);
  auto const ownerAAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.autoProvideFirstAttributeName);
  auto const ownerBAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.autoProvideSecondAttributeName);
  auto const ownerDimension = owner.rtiAmbassador().getDimensionHandle(
      options.ddmDimensionNames.front());
  auto const flavorClass = flavor.rtiAmbassador().getObjectClassHandle(
      options.autoProvideObjectClassName);
  auto const flavorAttribute = flavor.rtiAmbassador().getAttributeHandle(
      flavorClass,
      options.autoProvideFirstAttributeName);
  auto const flavorDimension = flavor.rtiAmbassador().getDimensionHandle(
      options.ddmDimensionNames.front());
  auto const organicClass = organic.rtiAmbassador().getObjectClassHandle(
      options.autoProvideObjectClassName);
  auto const organicAttribute = organic.rtiAmbassador().getAttributeHandle(
      organicClass,
      options.autoProvideSecondAttributeName);
  auto const organicDimension = organic.rtiAmbassador().getDimensionHandle(
      options.ddmDimensionNames.front());
  require(
      ownerClass.isValid() && ownerAAttribute.isValid() &&
          ownerBAttribute.isValid() && ownerDimension.isValid() &&
          flavorClass.isValid() && flavorAttribute.isValid() &&
          flavorDimension.isValid() && organicClass.isValid() &&
          organicAttribute.isValid() && organicDimension.isValid(),
      "regional Auto Provide multi-source lookup returned an invalid standard handle");
  require(
      owner.rtiAmbassador().getAutoProvideSwitch() &&
          flavor.rtiAmbassador().getAutoProvideSwitch() &&
          organic.rtiAmbassador().getAutoProvideSwitch(),
      "regional Auto Provide multi-source FOM did not enable the standard Auto Provide switch");
  require(
      owner.rtiAmbassador().getAvailableDimensionsForObjectClass(ownerClass) ==
              rti::DimensionHandleSet{ownerDimension} &&
          flavor.rtiAmbassador().getAvailableDimensionsForObjectClass(flavorClass) ==
              rti::DimensionHandleSet{flavorDimension} &&
          organic.rtiAmbassador().getAvailableDimensionsForObjectClass(organicClass) ==
              rti::DimensionHandleSet{organicDimension},
      "regional Auto Provide multi-source object class did not expose its configured dimension");

  rti::AttributeHandleSet const ownerAttributes{ownerAAttribute, ownerBAttribute};
  rti::AttributeHandleSet const flavorAttributes{flavorAttribute};
  rti::AttributeHandleSet const organicAttributes{organicAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  auto const ownerFlavorRegion = owner.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{ownerDimension});
  auto const ownerOrganicRegion = owner.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{ownerDimension});
  auto const flavorRegion = flavor.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{flavorDimension});
  auto const organicRegion = organic.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{organicDimension});
  require(
      ownerFlavorRegion.isValid() && ownerOrganicRegion.isValid() &&
          flavorRegion.isValid() && organicRegion.isValid(),
      "regional Auto Provide multi-source region creation returned an invalid handle");
  owner.rtiAmbassador().setRangeBounds(
      ownerFlavorRegion,
      ownerDimension,
      rti::RangeBounds(0UL, 2UL));
  owner.rtiAmbassador().setRangeBounds(
      ownerOrganicRegion,
      ownerDimension,
      rti::RangeBounds(5UL, 7UL));
  flavor.rtiAmbassador().setRangeBounds(
      flavorRegion,
      flavorDimension,
      rti::RangeBounds(1UL, 3UL));
  organic.rtiAmbassador().setRangeBounds(
      organicRegion,
      organicDimension,
      rti::RangeBounds(6UL, 8UL));
  owner.rtiAmbassador().commitRegionModifications(
      rti::RegionHandleSet{ownerFlavorRegion, ownerOrganicRegion});
  flavor.rtiAmbassador().commitRegionModifications(
      rti::RegionHandleSet{flavorRegion});
  organic.rtiAmbassador().commitRegionModifications(
      rti::RegionHandleSet{organicRegion});
  rti::AttributeHandleSetRegionHandleSetPairVector const ownerPairs{
      {rti::AttributeHandleSet{ownerAAttribute},
       rti::RegionHandleSet{ownerFlavorRegion}},
      {rti::AttributeHandleSet{ownerBAttribute},
       rti::RegionHandleSet{ownerOrganicRegion}}};
  rti::AttributeHandleSetRegionHandleSetPairVector const flavorPair{{
      flavorAttributes,
      rti::RegionHandleSet{flavorRegion},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const organicPair{{
      organicAttributes,
      rti::RegionHandleSet{organicRegion},
  }};
  flavor.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  organic.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  require(
      flavor.rtiAmbassador().getConveyRegionDesignatorSetsSwitch() &&
          organic.rtiAmbassador().getConveyRegionDesignatorSetsSwitch(),
      "regional Auto Provide multi-source subscribers did not enable region designator callbacks");

  std::vector<std::uint8_t> const flavorValue{0x4DU, 0x53U, 0x41U};
  std::vector<std::uint8_t> const organicValue{0x4DU, 0x53U, 0x42U};
  std::vector<std::uint8_t> const flavorTag{0x54U, 0x41U};
  std::vector<std::uint8_t> const organicTag{0x54U, 0x42U};
  ownerReports.setRequestHook(
      [&](rti::ObjectInstanceHandle const& object,
          rti::AttributeHandleSet const& attributes,
          rti::VariableLengthData const&) {
        rti::AttributeHandleValueMap values;
        rti::VariableLengthData responseTag;
        if (attributes.count(ownerAAttribute) != 0U) {
          values.emplace(
              ownerAAttribute,
              rti::VariableLengthData(flavorValue.data(), flavorValue.size()));
          responseTag = rti::VariableLengthData(
              flavorTag.data(),
              flavorTag.size());
        }
        if (attributes.count(ownerBAttribute) != 0U) {
          values.emplace(
              ownerBAttribute,
              rti::VariableLengthData(organicValue.data(), organicValue.size()));
          responseTag = rti::VariableLengthData(
              organicTag.data(),
              organicTag.size());
        }
        if (!values.empty()) {
          owner.rtiAmbassador().updateAttributeValues(
              object,
              values,
              responseTag);
        }
      });

  auto const object = owner.rtiAmbassador().registerObjectInstanceWithRegions(
      ownerClass,
      ownerPairs);
  require(
      object.isValid(),
      "regional Auto Provide multi-source registration returned an invalid handle");
  flavor.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      flavorClass,
      flavorPair);
  organic.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      organicClass,
      organicPair);

  auto const deadline = Clock::now() +
      std::chrono::milliseconds(options.timeoutMilliseconds);
  while (Clock::now() < deadline &&
         (flavorReports.discoveries().size() != 1U ||
          organicReports.discoveries().size() != 1U ||
          ownerReports.requests().size() != 2U ||
          flavorReports.reflections().size() != 1U ||
          organicReports.reflections().size() != 1U)) {
    flavor.pump();
    organic.pump();
    owner.pump();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  auto const flavorDiscoveries = flavorReports.discoveries();
  auto const organicDiscoveries = organicReports.discoveries();
  auto const requests = ownerReports.requests();
  auto const flavorReflections = flavorReports.reflections();
  auto const organicReflections = organicReports.reflections();
  require(
      flavorDiscoveries.size() == 1U && organicDiscoveries.size() == 1U &&
          requests.size() == 2U && flavorReflections.size() == 1U &&
          organicReflections.size() == 1U,
      "regional Auto Provide multi-source did not complete both independent deliveries");
  require(
      flavorDiscoveries.front().object == object &&
          flavorDiscoveries.front().objectClass == flavorClass &&
          flavorDiscoveries.front().producer == owner.federateHandle() &&
          organicDiscoveries.front().object == object &&
          organicDiscoveries.front().objectClass == organicClass &&
          organicDiscoveries.front().producer == owner.federateHandle(),
      "regional Auto Provide multi-source discovery returned the wrong metadata");

  bool sawARequest = false;
  bool sawBRequest = false;
  for (auto const& request : requests) {
    require(
        request.object == object && request.tag.empty(),
        "regional Auto Provide multi-source solicitation returned the wrong metadata");
    sawARequest = sawARequest || request.attributes ==
        rti::AttributeHandleSet{ownerAAttribute};
    sawBRequest = sawBRequest || request.attributes ==
        rti::AttributeHandleSet{ownerBAttribute};
  }
  require(
      sawARequest && sawBRequest,
      "regional Auto Provide multi-source did not issue one request per source attribute");

  auto const reliable = owner.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      reliable.isValid(),
      "regional Auto Provide multi-source reliable transportation lookup failed");
  auto const& flavorReflection = flavorReflections.front();
  require(
      flavorReflection.object == object && flavorReflection.values.size() == 1U &&
          flavorReflection.values.count(flavorAttribute) == 1U &&
          copyBytes(flavorReflection.values.at(flavorAttribute)) == flavorValue &&
          flavorReflection.tag == flavorTag &&
          flavorReflection.transportation == reliable &&
          flavorReflection.producer == owner.federateHandle() &&
          flavorReflection.regions.has_value() &&
          flavorReflection.regions.value() == rti::RegionHandleSet{ownerFlavorRegion},
      "regional Auto Provide multi-source flavor delivery changed scoped value metadata");
  auto const& organicReflection = organicReflections.front();
  require(
      organicReflection.object == object && organicReflection.values.size() == 1U &&
          organicReflection.values.count(organicAttribute) == 1U &&
          copyBytes(organicReflection.values.at(organicAttribute)) == organicValue &&
          organicReflection.tag == organicTag &&
          organicReflection.transportation == reliable &&
          organicReflection.producer == owner.federateHandle() &&
          organicReflection.regions.has_value() &&
          organicReflection.regions.value() == rti::RegionHandleSet{ownerOrganicRegion},
      "regional Auto Provide multi-source organic delivery changed scoped value metadata");

  flavor.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      flavorClass,
      flavorPair);
  organic.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      organicClass,
      organicPair);
  owner.rtiAmbassador().unassociateRegionsForUpdates(object, ownerPairs);
  owner.rtiAmbassador().deleteRegion(ownerFlavorRegion);
  owner.rtiAmbassador().deleteRegion(ownerOrganicRegion);
  flavor.rtiAmbassador().deleteRegion(flavorRegion);
  organic.rtiAmbassador().deleteRegion(organicRegion);
  flavor.resign(rti::NO_ACTION);
  organic.resign(rti::NO_ACTION);
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  owner.disconnect();
  flavor.disconnect();
  organic.disconnect();
}

void scenarioRegionalAutoProvideMultiSourceContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioRegionalAutoProvideMultiSource(options, model);
}

void scenarioRegionalAutoProvideMultiProvider(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.autoProvideFom.empty(),
      "regional Auto Provide multi-provider testing requires an adapter-supplied Auto Provide FOM");
  require(
      !options.ddmDimensionNames.empty(),
      "regional Auto Provide multi-provider testing requires an adapter-supplied dimension name");
  require(
      !options.autoProvideSecondAttributeName.empty(),
      "regional Auto Provide multi-provider testing requires two adapter-supplied attributes");

  RegionalAutoProvideAmbassador ownerReports;
  RegionalAutoProvideAmbassador secondProviderReports;
  RegionalAutoProvideAmbassador requesterReports;
  RegionalAutoProvideSession owner(options, model, "owner", ownerReports);
  RegionalAutoProvideSession secondProvider(
      options,
      model,
      "second-provider",
      secondProviderReports);
  RegionalAutoProvideSession requester(
      options,
      model,
      "requester",
      requesterReports);
  owner.connect();
  secondProvider.connect();
  requester.connect();
  auto const federation = federationName(
      options,
      "regional-auto-provide-multi-provider");
  owner.rtiAmbassador().createFederationExecution(
      federation,
      options.autoProvideFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);
  secondProvider.join(
      options.memberFederateName + L"-provider",
      options.federateType,
      federation);
  requester.join(
      options.memberFederateName + L"-requester",
      options.federateType,
      federation);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.autoProvideObjectClassName);
  auto const ownerAAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.autoProvideFirstAttributeName);
  auto const ownerBAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.autoProvideSecondAttributeName);
  auto const ownerDimension = owner.rtiAmbassador().getDimensionHandle(
      options.ddmDimensionNames.front());
  auto const secondProviderClass =
      secondProvider.rtiAmbassador().getObjectClassHandle(
          options.autoProvideObjectClassName);
  auto const secondProviderBAttribute =
      secondProvider.rtiAmbassador().getAttributeHandle(
          secondProviderClass,
          options.autoProvideSecondAttributeName);
  auto const secondProviderDimension =
      secondProvider.rtiAmbassador().getDimensionHandle(
          options.ddmDimensionNames.front());
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.autoProvideObjectClassName);
  auto const requesterAAttribute = requester.rtiAmbassador().getAttributeHandle(
      requesterClass,
      options.autoProvideFirstAttributeName);
  auto const requesterBAttribute = requester.rtiAmbassador().getAttributeHandle(
      requesterClass,
      options.autoProvideSecondAttributeName);
  auto const requesterDimension = requester.rtiAmbassador().getDimensionHandle(
      options.ddmDimensionNames.front());
  require(
      ownerClass.isValid() && ownerAAttribute.isValid() &&
          ownerBAttribute.isValid() && ownerDimension.isValid() &&
          secondProviderClass.isValid() && secondProviderBAttribute.isValid() &&
          secondProviderDimension.isValid() && requesterClass.isValid() &&
          requesterAAttribute.isValid() && requesterBAttribute.isValid() &&
          requesterDimension.isValid(),
      "regional Auto Provide multi-provider lookup returned an invalid standard handle");
  require(
      owner.rtiAmbassador().getAutoProvideSwitch() &&
          secondProvider.rtiAmbassador().getAutoProvideSwitch() &&
          requester.rtiAmbassador().getAutoProvideSwitch(),
      "regional Auto Provide multi-provider FOM did not enable the standard switch");

  rti::AttributeHandleSet const ownerAttributes{
      ownerAAttribute,
      ownerBAttribute};
  rti::AttributeHandleSet const secondProviderAttributes{
      secondProviderBAttribute};
  rti::AttributeHandleSet const requesterAttributes{
      requesterAAttribute,
      requesterBAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerClass,
      ownerAttributes);
  secondProvider.rtiAmbassador().publishObjectClassAttributes(
      secondProviderClass,
      secondProviderAttributes);

  auto const ownerRegion = owner.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{ownerDimension});
  auto const secondProviderRegion = secondProvider.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{secondProviderDimension});
  auto const requesterRegion = requester.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{requesterDimension});
  require(
      ownerRegion.isValid() && secondProviderRegion.isValid() &&
          requesterRegion.isValid(),
      "regional Auto Provide multi-provider region creation returned an invalid handle");
  owner.rtiAmbassador().setRangeBounds(
      ownerRegion,
      ownerDimension,
      rti::RangeBounds(0UL, 2UL));
  secondProvider.rtiAmbassador().setRangeBounds(
      secondProviderRegion,
      secondProviderDimension,
      rti::RangeBounds(0UL, 10UL));
  requester.rtiAmbassador().setRangeBounds(
      requesterRegion,
      requesterDimension,
      rti::RangeBounds(1UL, 3UL));
  owner.rtiAmbassador().commitRegionModifications(
      rti::RegionHandleSet{ownerRegion});
  secondProvider.rtiAmbassador().commitRegionModifications(
      rti::RegionHandleSet{secondProviderRegion});
  requester.rtiAmbassador().commitRegionModifications(
      rti::RegionHandleSet{requesterRegion});

  rti::AttributeHandleSetRegionHandleSetPairVector const ownerPair{{
      ownerAttributes,
      rti::RegionHandleSet{ownerRegion},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const secondProviderPair{{
      secondProviderAttributes,
      rti::RegionHandleSet{secondProviderRegion},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const requesterPair{{
      requesterAttributes,
      rti::RegionHandleSet{requesterRegion},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const ownerARegionPair{{
      rti::AttributeHandleSet{ownerAAttribute},
      rti::RegionHandleSet{ownerRegion},
  }};
  secondProvider.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      secondProviderClass,
      secondProviderPair);
  requester.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  require(
      requester.rtiAmbassador().getConveyRegionDesignatorSetsSwitch(),
      "regional Auto Provide multi-provider requester did not enable region designators");

  auto pumpUntil = [&](std::function<bool()> const& condition,
                       std::string const& description) {
    auto const deadline = Clock::now() +
        std::chrono::milliseconds(options.timeoutMilliseconds);
    while (Clock::now() < deadline && !condition()) {
      requester.pump();
      secondProvider.pump();
      owner.pump();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    require(condition(), description);
  };

  auto const object = owner.rtiAmbassador().registerObjectInstanceWithRegions(
      ownerClass,
      ownerPair);
  require(
      object.isValid(),
      "regional Auto Provide multi-provider registration returned an invalid handle");
  pumpUntil(
      [&] {
        return secondProviderReports.discoveries().size() == 1U &&
            ownerReports.requests().size() == 1U;
      },
      "regional Auto Provide multi-provider initial discovery and solicitation");
  auto const providerDiscoveries = secondProviderReports.discoveries();
  auto const initialRequests = ownerReports.requests();
  require(
      providerDiscoveries.size() == 1U &&
          providerDiscoveries.front().object == object &&
          providerDiscoveries.front().objectClass == secondProviderClass &&
          providerDiscoveries.front().producer == owner.federateHandle(),
      "regional Auto Provide multi-provider initial discovery returned the wrong metadata");
  require(
      initialRequests.size() == 1U &&
          initialRequests.front().object == object &&
          initialRequests.front().attributes ==
              rti::AttributeHandleSet{ownerBAttribute} &&
          initialRequests.front().tag.empty(),
      "regional Auto Provide multi-provider initial solicitation did not target the published second attribute");

  std::vector<std::uint8_t> const divestitureTag{0xD1U, 0x4DU, 0x25U};
  std::vector<std::uint8_t> const acquisitionTag{0xA1U, 0x4DU, 0x25U};
  rti::VariableLengthData const divestitureTagData(
      divestitureTag.data(),
      divestitureTag.size());
  rti::VariableLengthData const acquisitionTagData(
      acquisitionTag.data(),
      acquisitionTag.size());
  owner.rtiAmbassador().unconditionalAttributeOwnershipDivestiture(
      object,
      rti::AttributeHandleSet{ownerBAttribute},
      divestitureTagData);
  require(
      owner.rtiAmbassador().isAttributeOwnedByFederate(
          object,
          ownerAAttribute) &&
          !owner.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              ownerBAttribute),
      "regional Auto Provide multi-provider divestiture changed the wrong ownership state");
  pumpUntil(
      [&] { return secondProviderReports.ownershipAssumptions().size() == 1U; },
      "regional Auto Provide multi-provider ownership assumption callback");
  auto const ownershipAssumptions =
      secondProviderReports.ownershipAssumptions();
  require(
      ownershipAssumptions.size() == 1U &&
          ownershipAssumptions.front().object == object &&
          ownershipAssumptions.front().attributes == secondProviderAttributes &&
          ownershipAssumptions.front().tag == divestitureTag,
      "regional Auto Provide multi-provider ownership offer changed its attributes or tag");

  secondProvider.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
      object,
      secondProviderAttributes,
      acquisitionTagData);
  pumpUntil(
      [&] { return secondProviderReports.ownershipAcquisitions().size() == 1U; },
      "regional Auto Provide multi-provider ownership acquisition notification");
  auto const ownershipAcquisitions =
      secondProviderReports.ownershipAcquisitions();
  require(
      ownershipAcquisitions.size() == 1U &&
          ownershipAcquisitions.front().object == object &&
          ownershipAcquisitions.front().attributes == secondProviderAttributes &&
          ownershipAcquisitions.front().tag == acquisitionTag &&
          secondProvider.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              secondProviderBAttribute),
      "regional Auto Provide multi-provider acquisition notification changed state or tag");

  std::vector<std::uint8_t> const ownerValue{
      0x41U, 0x2DU, 0x52U, 0x45U, 0x47U};
  std::vector<std::uint8_t> const secondProviderValue{
      0x42U, 0x2DU, 0x52U, 0x45U, 0x47U};
  std::vector<std::uint8_t> const ownerTag{0x54U, 0x41U};
  std::vector<std::uint8_t> const secondProviderTag{0x54U, 0x42U};
  rti::VariableLengthData const ownerTagData(ownerTag.data(), ownerTag.size());
  rti::VariableLengthData const secondProviderTagData(
      secondProviderTag.data(),
      secondProviderTag.size());
  ownerReports.setRequestHook(
      [&](rti::ObjectInstanceHandle const& requestedObject,
          rti::AttributeHandleSet const& attributes,
          rti::VariableLengthData const& requestTag) {
        require(
            requestedObject == object &&
                attributes == rti::AttributeHandleSet{ownerAAttribute} &&
                copyBytes(requestTag).empty(),
            "regional Auto Provide multi-provider owner received the wrong request");
        rti::AttributeHandleValueMap values;
        values.emplace(
            ownerAAttribute,
            rti::VariableLengthData(ownerValue.data(), ownerValue.size()));
        owner.rtiAmbassador().updateAttributeValues(
            requestedObject,
            values,
            ownerTagData);
      });
  secondProviderReports.setRequestHook(
      [&](rti::ObjectInstanceHandle const& requestedObject,
          rti::AttributeHandleSet const& attributes,
          rti::VariableLengthData const& requestTag) {
        require(
            requestedObject == object &&
                attributes == secondProviderAttributes &&
                copyBytes(requestTag).empty(),
            "regional Auto Provide multi-provider second provider received the wrong request");
        rti::AttributeHandleValueMap values;
        values.emplace(
            secondProviderBAttribute,
            rti::VariableLengthData(
                secondProviderValue.data(),
                secondProviderValue.size()));
        secondProvider.rtiAmbassador().updateAttributeValues(
            requestedObject,
            values,
            secondProviderTagData);
      });

  requester.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      requesterClass,
      requesterPair);
  pumpUntil(
      [&] {
        return requesterReports.discoveries().size() == 1U &&
            ownerReports.requests().size() == 2U &&
            secondProviderReports.requests().size() == 1U &&
            requesterReports.reflections().size() == 2U;
      },
      "regional Auto Provide multi-provider fan-out and reflections");
  auto const requesterDiscoveries = requesterReports.discoveries();
  auto const ownerRequests = ownerReports.requests();
  auto const secondProviderRequests = secondProviderReports.requests();
  auto const reflections = requesterReports.reflections();
  require(
      requesterDiscoveries.size() == 1U &&
          requesterDiscoveries.front().object == object &&
          requesterDiscoveries.front().objectClass == requesterClass &&
          requesterDiscoveries.front().producer == owner.federateHandle(),
      "regional Auto Provide multi-provider requester discovery returned the wrong metadata");
  require(
      ownerRequests.size() == 2U &&
          ownerRequests[0].attributes == rti::AttributeHandleSet{ownerBAttribute} &&
          ownerRequests[1].attributes == rti::AttributeHandleSet{ownerAAttribute} &&
          ownerRequests[1].object == object && ownerRequests[1].tag.empty() &&
          secondProviderRequests.size() == 1U &&
          secondProviderRequests.front().object == object &&
          secondProviderRequests.front().attributes == secondProviderAttributes &&
          secondProviderRequests.front().tag.empty(),
      "regional Auto Provide multi-provider fan-out did not request each owned attribute from its owner");

  auto const reliable = owner.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      reliable.isValid(),
      "regional Auto Provide multi-provider reliable transportation lookup failed");
  bool sawOwnerReflection = false;
  bool sawSecondProviderReflection = false;
  for (auto const& reflection : reflections) {
    require(
        reflection.object == object && reflection.values.size() == 1U &&
            reflection.transportation == reliable &&
            reflection.regions.has_value(),
        "regional Auto Provide multi-provider reflection changed common metadata");
    if (reflection.producer == owner.federateHandle()) {
      require(
          !sawOwnerReflection &&
              reflection.values.count(requesterAAttribute) == 1U &&
              copyBytes(reflection.values.at(requesterAAttribute)) == ownerValue &&
              reflection.tag == ownerTag &&
              reflection.regions.value() == rti::RegionHandleSet{ownerRegion},
          "regional Auto Provide multi-provider owner reflection changed its value or source region");
      sawOwnerReflection = true;
    } else if (reflection.producer == secondProvider.federateHandle()) {
      require(
          !sawSecondProviderReflection &&
              reflection.values.count(requesterBAttribute) == 1U &&
              copyBytes(reflection.values.at(requesterBAttribute)) ==
                  secondProviderValue &&
              reflection.tag == secondProviderTag &&
              reflection.regions.value().empty(),
          "regional Auto Provide multi-provider second-provider reflection changed its value or default region designator");
      sawSecondProviderReflection = true;
    } else {
      require(
          false,
          "regional Auto Provide multi-provider reflection came from an unexpected federate");
    }
  }
  require(
      sawOwnerReflection && sawSecondProviderReflection,
      "regional Auto Provide multi-provider did not reflect once from each owner");

  requester.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      requesterClass,
      requesterPair);
  secondProvider.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      secondProviderClass,
      secondProviderPair);
  owner.rtiAmbassador().unassociateRegionsForUpdates(object, ownerARegionPair);
  requester.rtiAmbassador().deleteRegion(requesterRegion);
  secondProvider.rtiAmbassador().deleteRegion(secondProviderRegion);
  owner.rtiAmbassador().deleteRegion(ownerRegion);
  requester.resign(rti::NO_ACTION);
  secondProvider.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  owner.disconnect();
  secondProvider.disconnect();
  requester.disconnect();
}

void scenarioRegionalAutoProvideMultiProviderContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioRegionalAutoProvideMultiProvider(options, model);
}

void scenarioRegionalAutoProvideCallbackControl(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.autoProvideFom.empty(),
      "regional Auto Provide callback-control testing requires an adapter-supplied Auto Provide FOM");
  require(
      !options.ddmDimensionNames.empty(),
      "regional Auto Provide callback-control testing requires an adapter-supplied dimension name");

  RegionalAutoProvideAmbassador ownerReports;
  RegionalAutoProvideAmbassador requesterReports;
  RegionalAutoProvideSession owner(options, model, "owner", ownerReports);
  RegionalAutoProvideSession requester(
      options,
      model,
      "requester",
      requesterReports);
  owner.connect();
  requester.connect();
  auto const federation = federationName(
      options,
      "regional-auto-provide-callback-control");
  owner.rtiAmbassador().createFederationExecution(
      federation,
      options.autoProvideFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);
  requester.join(
      options.memberFederateName + L"-callback-control",
      options.federateType,
      federation);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.autoProvideObjectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.autoProvideFirstAttributeName);
  auto const ownerDimension = owner.rtiAmbassador().getDimensionHandle(
      options.ddmDimensionNames.front());
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.autoProvideObjectClassName);
  auto const requesterAttribute = requester.rtiAmbassador().getAttributeHandle(
      requesterClass,
      options.autoProvideFirstAttributeName);
  auto const requesterDimension = requester.rtiAmbassador().getDimensionHandle(
      options.ddmDimensionNames.front());
  require(
      ownerClass.isValid() && ownerAttribute.isValid() &&
          ownerDimension.isValid() && requesterClass.isValid() &&
          requesterAttribute.isValid() && requesterDimension.isValid(),
      "regional Auto Provide callback-control lookup returned an invalid standard handle");
  require(
      owner.rtiAmbassador().getAutoProvideSwitch() &&
          requester.rtiAmbassador().getAutoProvideSwitch(),
      "regional Auto Provide callback-control FOM did not enable the standard switch");
  require(
      owner.rtiAmbassador().getDimensionUpperBound(ownerDimension) >= 3U &&
          requester.rtiAmbassador().getDimensionUpperBound(requesterDimension) >= 3U,
      "regional Auto Provide callback-control dimension has insufficient range");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const requesterAttributes{requesterAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerClass,
      ownerAttributes);
  auto const ownerRegion = owner.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{ownerDimension});
  auto const requesterRegion = requester.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{requesterDimension});
  require(
      ownerRegion.isValid() && requesterRegion.isValid(),
      "regional Auto Provide callback-control region creation returned an invalid handle");
  owner.rtiAmbassador().setRangeBounds(
      ownerRegion,
      ownerDimension,
      rti::RangeBounds(0UL, 2UL));
  requester.rtiAmbassador().setRangeBounds(
      requesterRegion,
      requesterDimension,
      rti::RangeBounds(1UL, 3UL));
  owner.rtiAmbassador().commitRegionModifications(
      rti::RegionHandleSet{ownerRegion});
  requester.rtiAmbassador().commitRegionModifications(
      rti::RegionHandleSet{requesterRegion});
  rti::AttributeHandleSetRegionHandleSetPairVector const ownerPair{{
      ownerAttributes,
      rti::RegionHandleSet{ownerRegion},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const requesterPair{{
      requesterAttributes,
      rti::RegionHandleSet{requesterRegion},
  }};
  requester.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  require(
      requester.rtiAmbassador().getConveyRegionDesignatorSetsSwitch(),
      "regional Auto Provide callback-control requester did not enable region designators");

  std::vector<std::string> callbackOrder;
  requesterReports.setDiscoveryHook([&] {
    callbackOrder.push_back("discover");
  });
  ownerReports.setRequestHook(
      [&](rti::ObjectInstanceHandle const&,
          rti::AttributeHandleSet const&,
          rti::VariableLengthData const&) {
        callbackOrder.push_back("provide");
      });

  requester.rtiAmbassador().disableCallbacks();
  auto const object = owner.rtiAmbassador().registerObjectInstanceWithRegions(
      ownerClass,
      ownerPair);
  require(
      object.isValid(),
      "regional Auto Provide callback-control registration returned an invalid handle");
  requester.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      requesterClass,
      requesterPair);
  requester.pump();
  owner.pump();
  requester.pump();
  owner.pump();
  require(
      requesterReports.discoveries().empty() && ownerReports.requests().empty() &&
          requesterReports.reflections().empty() && callbackOrder.empty(),
      "regional Auto Provide delivered discovery or provider callbacks while the requester had callbacks disabled");

  requester.rtiAmbassador().enableCallbacks();
  auto const deadline = Clock::now() +
      std::chrono::milliseconds(options.timeoutMilliseconds);
  while (Clock::now() < deadline &&
         (requesterReports.discoveries().size() != 1U ||
          ownerReports.requests().size() != 1U)) {
    requester.pump();
    owner.pump();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  auto const discoveries = requesterReports.discoveries();
  auto const requests = ownerReports.requests();
  require(
      discoveries.size() == 1U && requests.size() == 1U &&
          requesterReports.reflections().empty(),
      "regional Auto Provide callback-control did not resume one discovery and one provider request");
  require(
      callbackOrder == std::vector<std::string>{"discover", "provide"},
      "regional Auto Provide callback-control did not deliver discovery before provider request");
  require(
      discoveries.front().object == object &&
          discoveries.front().objectClass == requesterClass &&
          discoveries.front().producer == owner.federateHandle(),
      "regional Auto Provide callback-control discovery returned the wrong metadata");
  require(
      requests.front().object == object &&
          requests.front().attributes == ownerAttributes &&
          requests.front().tag.empty(),
      "regional Auto Provide callback-control request returned the wrong metadata");

  requester.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      requesterClass,
      requesterPair);
  owner.rtiAmbassador().unassociateRegionsForUpdates(object, ownerPair);
  requester.rtiAmbassador().deleteRegion(requesterRegion);
  owner.rtiAmbassador().deleteRegion(ownerRegion);
  requester.resign(rti::NO_ACTION);
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  owner.disconnect();
  requester.disconnect();
}

void scenarioRegionalAutoProvideCallbackControlContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioRegionalAutoProvideCallbackControl(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
