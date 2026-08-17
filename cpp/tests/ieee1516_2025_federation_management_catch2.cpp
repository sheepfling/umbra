#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <memory>
#include <set>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAfloat64Interval.h>
#include <RTI/time/HLAfloat64Time.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The embedded federation-management tests require the Umbra source directory."
#endif

namespace {

using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::InteractionClassHandleSet;
using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleSetRegionHandleSetPairVector;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::DimensionHandle;
using rti1516_2025::DimensionHandleSet;
using rti1516_2025::ParameterHandle;
using rti1516_2025::OrderType;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RegionHandle;
using rti1516_2025::RangeBounds;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::ResignAction;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

using TestFederateAmbassador = NullFederateAmbassador;

class ReportingFederateAmbassador final : public NullFederateAmbassador {
 public:
  struct MemberReport {
    std::wstring federationName;
    rti1516_2025::FederationExecutionMemberInformationVector members;
  };

  struct TimeAdvanceGrantReport {
    std::wstring implementationName;
    std::wstring value;
  };

  struct InteractionReport {
    InteractionClassHandle interactionClass;
    ParameterHandleValueMap parameterValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    bool sentRegionsSupplied = false;
  };

  struct TimestampedInteractionReport {
    InteractionClassHandle interactionClass;
    ParameterHandleValueMap parameterValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    std::wstring timeImplementationName;
    std::wstring timeValue;
    OrderType sentOrderType = rti1516_2025::RECEIVE;
    OrderType receivedOrderType = rti1516_2025::RECEIVE;
    bool retractionSupplied = false;
    bool retractionValid = false;
  };

  struct DirectedInteractionReport {
    InteractionClassHandle interactionClass;
    ObjectInstanceHandle objectInstance;
    ParameterHandleValueMap parameterValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
  };

  struct ObjectDiscoveryReport {
    ObjectInstanceHandle objectInstance;
    ObjectClassHandle objectClass;
    std::wstring objectInstanceName;
    FederateHandle producingFederate;
  };

  struct ObjectRemovalReport {
    ObjectInstanceHandle objectInstance;
    VariableLengthData userSuppliedTag;
    FederateHandle producingFederate;
    std::wstring timeImplementationName;
    std::wstring timeValue;
    OrderType sentOrderType = rti1516_2025::RECEIVE;
    OrderType receivedOrderType = rti1516_2025::RECEIVE;
    bool retractionSupplied = false;
    bool retractionValid = false;
  };

  struct AttributeReflectionReport {
    ObjectInstanceHandle objectInstance;
    AttributeHandleValueMap attributeValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    bool sentRegionsSupplied = false;
    RegionHandleSet sentRegions;
    std::wstring timeImplementationName;
    std::wstring timeValue;
    OrderType sentOrderType = rti1516_2025::RECEIVE;
    OrderType receivedOrderType = rti1516_2025::RECEIVE;
    bool retractionSupplied = false;
    bool retractionValid = false;
  };

  struct AttributeScopeReport {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
  };

  struct AttributeValueUpdateRequestReport {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  struct AttributeOwnershipReport {
    enum class Kind {
      federate,
      unowned,
      rti,
    };

    Kind kind = Kind::unowned;
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    FederateHandle owner;
  };

  struct AttributeOwnershipAcquisitionReport {
    enum class Kind {
      notification,
      unavailable,
    };

    Kind kind = Kind::unavailable;
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  struct AttributeOwnershipReleaseRequestReport {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  struct AttributeOwnershipAcquisitionCancellationReport {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
  };

  struct AttributeOwnershipAssumptionReport {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  struct DivestitureConfirmationReport {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  struct ObjectInstanceNameReservationReport {
    std::wstring objectInstanceName;
  };

  struct MultipleObjectInstanceNameReservationReport {
    std::set<std::wstring> objectInstanceNames;
  };

  void reportFederationExecutions(
      rti1516_2025::FederationExecutionInformationVector const& report) override {
    federationExecutionReports.push_back(report);
  }

  void reportFederationExecutionMembers(
      std::wstring const& federationName,
      rti1516_2025::FederationExecutionMemberInformationVector const& report) override {
    federationExecutionMemberReports.push_back({federationName, report});
  }

  void reportFederationExecutionDoesNotExist(std::wstring const& federationName) override {
    missingFederationReports.push_back(federationName);
  }

  void timeAdvanceGrant(rti1516_2025::LogicalTime const& time) override {
    timeAdvanceGrantReports.push_back({time.implementationName(), time.toString()});
    callbackOrder.push_back("grant");
  }

  void timeRegulationEnabled(rti1516_2025::LogicalTime const& time) override {
    timeRegulationEnabledReports.push_back({time.implementationName(), time.toString()});
  }

  void timeConstrainedEnabled(rti1516_2025::LogicalTime const& time) override {
    timeConstrainedEnabledReports.push_back({time.implementationName(), time.toString()});
  }

  void objectInstanceNameReservationSucceeded(
      std::wstring const& objectInstanceName) override {
    objectInstanceNameReservationSucceededReports.push_back({objectInstanceName});
  }

  void objectInstanceNameReservationFailed(
      std::wstring const& objectInstanceName) override {
    objectInstanceNameReservationFailedReports.push_back({objectInstanceName});
  }

  void multipleObjectInstanceNameReservationSucceeded(
      std::set<std::wstring> const& objectInstanceNames) override {
    multipleObjectInstanceNameReservationSucceededReports.push_back({objectInstanceNames});
  }

  void multipleObjectInstanceNameReservationFailed(
      std::set<std::wstring> const& objectInstanceNames) override {
    multipleObjectInstanceNameReservationFailedReports.push_back({objectInstanceNames});
  }

  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      RegionHandleSet const* optionalSentRegions) override {
    interactionReports.push_back({
        interactionClass,
        parameterValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        optionalSentRegions != nullptr,
    });
  }

  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      RegionHandleSet const* optionalSentRegions,
      rti1516_2025::LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
    static_cast<void>(optionalSentRegions);
    timestampedInteractionReports.push_back({
        interactionClass,
        parameterValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        time.implementationName(),
        time.toString(),
        sentOrderType,
        receivedOrderType,
        optionalRetraction != nullptr,
        optionalRetraction != nullptr && optionalRetraction->isValid(),
    });
    callbackOrder.push_back("interaction");
  }

  void receiveDirectedInteraction(
      InteractionClassHandle const& interactionClass,
      ObjectInstanceHandle const& objectInstance,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate) override {
    directedInteractionReports.push_back({
        interactionClass,
        objectInstance,
        parameterValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
    });
  }

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const& objectClass,
      std::wstring const& objectInstanceName,
      FederateHandle const& producingFederate) override {
    objectDiscoveryReports.push_back({
        objectInstance,
        objectClass,
        objectInstanceName,
        producingFederate,
    });
  }

  void removeObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      VariableLengthData const& userSuppliedTag,
      FederateHandle const& producingFederate) override {
    objectRemovalReports.push_back({
        objectInstance,
        userSuppliedTag,
        producingFederate,
    });
  }

  void removeObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      VariableLengthData const& userSuppliedTag,
      FederateHandle const& producingFederate,
      rti1516_2025::LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
    objectRemovalReports.push_back({
        objectInstance,
        userSuppliedTag,
        producingFederate,
        time.implementationName(),
        time.toString(),
        sentOrderType,
        receivedOrderType,
        optionalRetraction != nullptr,
        optionalRetraction != nullptr && optionalRetraction->isValid(),
    });
    callbackOrder.push_back("remove");
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      RegionHandleSet const* optionalSentRegions) override {
    attributeReflectionReports.push_back({
        objectInstance,
        attributeValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        optionalSentRegions != nullptr,
        optionalSentRegions == nullptr ? RegionHandleSet{} : *optionalSentRegions,
    });
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      RegionHandleSet const* optionalSentRegions,
      rti1516_2025::LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
    attributeReflectionReports.push_back({
        objectInstance,
        attributeValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        optionalSentRegions != nullptr,
        optionalSentRegions == nullptr ? RegionHandleSet{} : *optionalSentRegions,
        time.implementationName(),
        time.toString(),
        sentOrderType,
        receivedOrderType,
        optionalRetraction != nullptr,
        optionalRetraction != nullptr && optionalRetraction->isValid(),
    });
    callbackOrder.push_back("reflect");
  }

  void attributesInScope(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes) override {
    attributesInScopeReports.push_back({objectInstance, attributes});
  }

  void attributesOutOfScope(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes) override {
    attributesOutOfScopeReports.push_back({objectInstance, attributes});
  }

  void provideAttributeValueUpdate(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      VariableLengthData const& userSuppliedTag) override {
    attributeValueUpdateRequestReports.push_back({
        objectInstance,
        attributes,
        userSuppliedTag,
    });
    if (provideAttributeValueUpdateHandler) {
      provideAttributeValueUpdateHandler(objectInstance, attributes, userSuppliedTag);
    }
  }

  void informAttributeOwnership(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      FederateHandle const& owner) override {
    attributeOwnershipReports.push_back({
        AttributeOwnershipReport::Kind::federate,
        objectInstance,
        attributes,
        owner,
    });
  }

  void attributeIsNotOwned(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes) override {
    attributeOwnershipReports.push_back({
        AttributeOwnershipReport::Kind::unowned,
        objectInstance,
        attributes,
        FederateHandle(),
    });
  }

  void attributeIsOwnedByRTI(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes) override {
    attributeOwnershipReports.push_back({
        AttributeOwnershipReport::Kind::rti,
        objectInstance,
        attributes,
        FederateHandle(),
    });
  }

  void attributeOwnershipAcquisitionNotification(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& securedAttributes,
      VariableLengthData const& userSuppliedTag) override {
    attributeOwnershipAcquisitionReports.push_back({
        AttributeOwnershipAcquisitionReport::Kind::notification,
        objectInstance,
        securedAttributes,
        userSuppliedTag,
    });
  }

  void attributeOwnershipUnavailable(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      VariableLengthData const& userSuppliedTag) override {
    attributeOwnershipAcquisitionReports.push_back({
        AttributeOwnershipAcquisitionReport::Kind::unavailable,
        objectInstance,
        attributes,
        userSuppliedTag,
    });
  }

  void requestAttributeOwnershipRelease(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& candidateAttributes,
      VariableLengthData const& userSuppliedTag) override {
    attributeOwnershipReleaseRequestReports.push_back({
        objectInstance,
        candidateAttributes,
        userSuppliedTag,
    });
  }

  void confirmAttributeOwnershipAcquisitionCancellation(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes) override {
    attributeOwnershipAcquisitionCancellationReports.push_back({
        objectInstance,
        attributes,
    });
  }

  void requestAttributeOwnershipAssumption(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& offeredAttributes,
      VariableLengthData const& userSuppliedTag) override {
    attributeOwnershipAssumptionReports.push_back({
        objectInstance,
        offeredAttributes,
        userSuppliedTag,
    });
  }

  void requestDivestitureConfirmation(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& releasedAttributes,
      VariableLengthData const& userSuppliedTag) override {
    divestitureConfirmationReports.push_back({
        objectInstance,
        releasedAttributes,
        userSuppliedTag,
    });
  }

  std::vector<rti1516_2025::FederationExecutionInformationVector> federationExecutionReports;
  std::vector<MemberReport> federationExecutionMemberReports;
  std::vector<std::wstring> missingFederationReports;
  std::vector<TimeAdvanceGrantReport> timeAdvanceGrantReports;
  std::vector<TimeAdvanceGrantReport> timeRegulationEnabledReports;
  std::vector<TimeAdvanceGrantReport> timeConstrainedEnabledReports;
  std::vector<std::string> callbackOrder;
  std::vector<ObjectInstanceNameReservationReport>
      objectInstanceNameReservationSucceededReports;
  std::vector<ObjectInstanceNameReservationReport>
      objectInstanceNameReservationFailedReports;
  std::vector<MultipleObjectInstanceNameReservationReport>
      multipleObjectInstanceNameReservationSucceededReports;
  std::vector<MultipleObjectInstanceNameReservationReport>
      multipleObjectInstanceNameReservationFailedReports;
  std::vector<InteractionReport> interactionReports;
  std::vector<TimestampedInteractionReport> timestampedInteractionReports;
  std::vector<DirectedInteractionReport> directedInteractionReports;
  std::vector<ObjectDiscoveryReport> objectDiscoveryReports;
  std::vector<ObjectRemovalReport> objectRemovalReports;
  std::vector<AttributeReflectionReport> attributeReflectionReports;
  std::vector<AttributeScopeReport> attributesInScopeReports;
  std::vector<AttributeScopeReport> attributesOutOfScopeReports;
  std::vector<AttributeValueUpdateRequestReport> attributeValueUpdateRequestReports;
  std::function<void(
      ObjectInstanceHandle const&,
      AttributeHandleSet const&,
      VariableLengthData const&)>
      provideAttributeValueUpdateHandler;
  std::vector<AttributeOwnershipReport> attributeOwnershipReports;
  std::vector<AttributeOwnershipAcquisitionReport> attributeOwnershipAcquisitionReports;
  std::vector<AttributeOwnershipReleaseRequestReport> attributeOwnershipReleaseRequestReports;
  std::vector<AttributeOwnershipAcquisitionCancellationReport>
      attributeOwnershipAcquisitionCancellationReports;
  std::vector<AttributeOwnershipAssumptionReport> attributeOwnershipAssumptionReports;
  std::vector<DivestitureConfirmationReport> divestitureConfirmationReports;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
         "third_party" /
         "ieee1516.2-2025" /
         "resources" /
         relativePath;
}

class ScopedTemporaryFile final {
 public:
  explicit ScopedTemporaryFile(std::filesystem::path path) : path_(std::move(path)) {}

  ScopedTemporaryFile(ScopedTemporaryFile const&) = delete;
  ScopedTemporaryFile& operator=(ScopedTemporaryFile const&) = delete;

  ~ScopedTemporaryFile() {
    std::error_code ignored;
    std::filesystem::remove(path_, ignored);
  }

  [[nodiscard]] std::filesystem::path const& path() const noexcept {
    return path_;
  }

 private:
  std::filesystem::path path_;
};

ScopedTemporaryFile nrgEnabledRestaurantModule() {
  static std::atomic_uint64_t counter{0};
  std::ifstream input(resourcePath("examples/RestaurantFOMmodule-2025.xml"), std::ios::binary);
  REQUIRE(input.good());
  std::string fomText{
      std::istreambuf_iterator<char>(input),
      std::istreambuf_iterator<char>()};
  std::string const marker = "</switches>";
  auto const switchesPosition = fomText.find(marker);
  REQUIRE(switchesPosition != std::string::npos);
  fomText.insert(
      switchesPosition,
      "        <nonRegulatedGrant isEnabled=\"true\"/>\n    ");

  auto const path = std::filesystem::temp_directory_path() /
      ("umbra-nrg-scheduler-" + std::to_string(++counter) + ".xml");
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  REQUIRE(output.good());
  output << fomText;
  REQUIRE(output.good());
  return ScopedTemporaryFile(path);
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t counter{0};
  return L"umbra-catch2-federation-" + std::to_wstring(++counter);
}

std::vector<unsigned char> variableLengthDataBytes(VariableLengthData const& value) {
  auto const* bytes = static_cast<unsigned char const*>(value.data());
  if (bytes == nullptr) {
    return {};
  }
  return {bytes, bytes + value.size()};
}

}  // namespace

TEST_CASE(
    "Embedded federation-management services require an RTI connection",
    "[integration][federation-management][connection]") {
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_THROWS_AS(
      rti->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->destroyFederationExecution(federationName),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->joinFederationExecution(L"observer", federationName),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      rti->resignFederationExecution(NO_ACTION),
      rti1516_2025::NotConnected);
}

TEST_CASE(
    "Embedded federation-management services commit only a prevalidated federation definition",
    "[integration][development-profile][federation-management][rti.service.create-federation-execution]"
    "[rti.service.destroy-federation-execution][rti.service.join-federation-execution]"
    "[rti.service.resign-federation-execution][m16.transition.federate-resign]") {
  TestFederateAmbassador creatorFederate;
  TestFederateAmbassador joiningFederate;
  TestFederateAmbassador duplicateNameFederate;
  auto creator = makeRti();
  auto joiner = makeRti();
  auto duplicateName = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(creator->connect(creatorFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(joiner->connect(joiningFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(duplicateName->connect(duplicateNameFederate, HLA_EVOKED));

  SECTION("Create rejects unresolved logical-time or standard-MIM designators") {
    REQUIRE_THROWS_AS(
        creator->createFederationExecution(federationName, fomModule),
        rti1516_2025::InconsistentFOM);
    REQUIRE_THROWS_AS(
        creator->createFederationExecution(federationName, fomModule, L"ExampleCustomTime"),
        rti1516_2025::CouldNotCreateLogicalTimeFactory);
    REQUIRE_THROWS_AS(
        creator->createFederationExecutionWithMIM(
            federationName,
            std::vector<std::wstring>{fomModule},
            L"HLAstandardMIM",
            L"HLAinteger64Time"),
        rti1516_2025::DesignatorIsHLAstandardMIM);
  }

  REQUIRE_NOTHROW(
      creator->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_THROWS_AS(
      creator->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"),
      rti1516_2025::FederationExecutionAlreadyExists);

  FederateHandle creatorHandle;
  REQUIRE_NOTHROW(
      creatorHandle = creator->joinFederationExecution(L"creator", L"owner", federationName));
  REQUIRE(creatorHandle.isValid());
  REQUIRE_THROWS_AS(
      creator->joinFederationExecution(L"creator", federationName),
      rti1516_2025::FederateAlreadyExecutionMember);
  REQUIRE_THROWS_AS(creator->disconnect(), rti1516_2025::FederateIsExecutionMember);

  FederateHandle joinerHandle;
  REQUIRE_NOTHROW(
      joinerHandle = joiner->joinFederationExecution(
          L"joiner",
          L"observer",
          federationName,
          std::vector<std::wstring>{fomModule}));
  REQUIRE(joinerHandle.isValid());
  REQUIRE(creatorHandle != joinerHandle);
  REQUIRE_THROWS_AS(
      duplicateName->joinFederationExecution(L"creator", L"observer", federationName),
      rti1516_2025::FederateNameAlreadyInUse);
  REQUIRE_THROWS_AS(
      creator->destroyFederationExecution(federationName),
      rti1516_2025::FederatesCurrentlyJoined);

  REQUIRE_THROWS_AS(
      creator->resignFederationExecution(static_cast<ResignAction>(-1)),
      rti1516_2025::InvalidResignAction);
  REQUIRE_NOTHROW(creator->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(joiner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(creator->destroyFederationExecution(federationName));

  REQUIRE_NOTHROW(creator->disconnect());
  REQUIRE_NOTHROW(joiner->disconnect());
  REQUIRE_NOTHROW(duplicateName->disconnect());
}

TEST_CASE(
    "Embedded federation-management preparation failures leave shared federation state unchanged",
    "[integration][development-profile][federation-management][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.destroy-federation-execution]") {
  TestFederateAmbassador ownerFederate;
  TestFederateAmbassador applicantFederate;
  auto owner = makeRti();
  auto applicant = makeRti();
  auto const federationName = nextFederationName();
  auto const baseFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const extensionFom = resourcePath("examples/RestaurantExtensionFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(applicant->connect(applicantFederate, HLA_EVOKED));

  // The base module documents HLAinteger64Time, so the no-name Create default
  // must fail without reserving the federation name.
  REQUIRE_THROWS_AS(
      owner->createFederationExecution(federationName, baseFom),
      rti1516_2025::InconsistentFOM);
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, baseFom, L"HLAinteger64Time"));

  // The supplied extension cannot be materialized by the official FDD XSD.
  // Its failed addition must leave the valid base definition joinable.
  REQUIRE_THROWS_AS(
      applicant->joinFederationExecution(
          L"applicant",
          L"observer",
          federationName,
          std::vector<std::wstring>{extensionFom}),
      rti1516_2025::InconsistentFOM);
  FederateHandle applicantHandle;
  REQUIRE_NOTHROW(
      applicantHandle = applicant->joinFederationExecution(L"applicant", L"observer", federationName));
  REQUIRE(applicantHandle.isValid());

  REQUIRE_NOTHROW(applicant->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(applicant->disconnect());
}

TEST_CASE(
    "Embedded getTimeFactory returns the joined federation's selected time factory",
    "[integration][development-profile][federation-management][rti.service.get-time-factory]") {
  TestFederateAmbassador federate;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_THROWS_AS(rti->getTimeFactory(), rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(rti->connect(federate, HLA_EVOKED));
  REQUIRE_THROWS_AS(rti->getTimeFactory(), rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(rti->joinFederationExecution(L"time-client", L"observer", federationName));

  auto factory = rti->getTimeFactory();
  REQUIRE(factory);
  REQUIRE(factory->getName() == L"HLAinteger64Time");
  auto initial = factory->makeInitial();
  auto* integerInitial = dynamic_cast<rti1516_2025::HLAinteger64Time*>(initial.get());
  REQUIRE(integerInitial != nullptr);
  REQUIRE(integerInitial->isInitial());

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_THROWS_AS(rti->getTimeFactory(), rti1516_2025::FederateNotExecutionMember);
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded federate lookup services resolve active identities within the joined federation",
    "[integration][development-profile][federation-management][rti.service.get-federate-handle]"
    "[rti.service.get-federate-name]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador ownerFederate;
  TestFederateAmbassador peerFederate;
  TestFederateAmbassador foreignCreatorFederate;
  TestFederateAmbassador foreignMemberFederate;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto peer = makeRti();
  auto foreignCreator = makeRti();
  auto foreignMember = makeRti();
  auto const federationName = nextFederationName();
  auto const foreignFederationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  FederateHandle invalid;

  REQUIRE_THROWS_AS(
      unjoined->getFederateHandle(L"lookup-owner"),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(unjoined->getFederateName(invalid), rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->getFederateHandle(L"lookup-owner"),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->getFederateName(invalid),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(foreignCreator->connect(foreignCreatorFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(foreignMember->connect(foreignMemberFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(foreignCreator->createFederationExecution(
      foreignFederationName,
      fomModule,
      L"HLAinteger64Time"));

  FederateHandle ownerHandle;
  FederateHandle peerHandle;
  FederateHandle foreignHandle;
  REQUIRE_NOTHROW(
      ownerHandle = owner->joinFederationExecution(L"lookup-owner", L"owner", federationName));
  REQUIRE_NOTHROW(
      peerHandle = peer->joinFederationExecution(L"lookup-peer", L"observer", federationName));
  REQUIRE_NOTHROW(foreignHandle = foreignMember->joinFederationExecution(
      L"lookup-foreign",
      L"observer",
      foreignFederationName));

  REQUIRE(owner->getFederateHandle(L"lookup-owner") == ownerHandle);
  REQUIRE(owner->getFederateHandle(L"lookup-peer") == peerHandle);
  REQUIRE(peer->getFederateName(ownerHandle) == L"lookup-owner");
  REQUIRE(peer->getFederateName(peerHandle) == L"lookup-peer");
  REQUIRE_THROWS_AS(owner->getFederateHandle(L"missing"), rti1516_2025::NameNotFound);
  REQUIRE_THROWS_AS(owner->getFederateName(invalid), rti1516_2025::InvalidFederateHandle);
  REQUIRE_THROWS_AS(
      owner->getFederateName(foreignHandle),
      rti1516_2025::FederateHandleNotKnown);

  REQUIRE_NOTHROW(peer->resignFederationExecution(NO_ACTION));
  REQUIRE_THROWS_AS(owner->getFederateHandle(L"lookup-peer"), rti1516_2025::NameNotFound);
  REQUIRE_THROWS_AS(
      owner->getFederateName(peerHandle),
      rti1516_2025::FederateHandleNotKnown);

  REQUIRE_NOTHROW(foreignMember->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(foreignCreator->destroyFederationExecution(foreignFederationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
  REQUIRE_NOTHROW(foreignCreator->disconnect());
  REQUIRE_NOTHROW(foreignMember->disconnect());
}

TEST_CASE(
    "Embedded object-class lookup services use stable handles from the joined federation FOM",
    "[integration][development-profile][federation-management][rti.service.get-object-class-handle]"
    "[rti.service.get-object-class-name]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador ownerFederate;
  TestFederateAmbassador extensionFederate;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto extensionJoiner = makeRti();
  auto const federationName = nextFederationName();
  auto const baseFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const extensionFom = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "cpp" / "tests" / "data" / "reference-data-class-provider-fom.xml";
  ObjectClassHandle invalid;

  REQUIRE_THROWS_AS(
      unjoined->getObjectClassHandle(L"HLAobjectRoot.Employee"),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(unjoined->getObjectClassName(invalid), rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->getObjectClassHandle(L"HLAobjectRoot.Employee"),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->getObjectClassName(invalid),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(extensionJoiner->connect(extensionFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, baseFom, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(L"lookup-owner", L"owner", federationName));

  auto const employee = owner->getObjectClassHandle(L"HLAobjectRoot.Employee");
  REQUIRE(employee.isValid());
  REQUIRE(owner->getObjectClassHandle(L"HLAobjectRoot.Employee") == employee);
  REQUIRE(owner->getObjectClassName(employee) == L"HLAobjectRoot.Employee");
  REQUIRE_THROWS_AS(
      owner->getObjectClassHandle(L"HLAobjectRoot.Missing"),
      rti1516_2025::NameNotFound);
  REQUIRE_THROWS_AS(
      owner->getObjectClassName(invalid),
      rti1516_2025::InvalidObjectClassHandle);

  // A compatible additional-FOM join extends the catalog. The previously
  // returned class handle must remain stable while the new class becomes
  // discoverable from the same joined federation.
  REQUIRE_NOTHROW(extensionJoiner->joinFederationExecution(
      L"lookup-extension",
      L"observer",
      federationName,
      std::vector<std::wstring>{extensionFom.wstring()}));
  REQUIRE(owner->getObjectClassHandle(L"HLAobjectRoot.Employee") == employee);
  auto const extension = owner->getObjectClassHandle(L"HLAobjectRoot.UmbraReferenceFixtureClass");
  REQUIRE(extension.isValid());
  REQUIRE(extension != employee);
  REQUIRE(owner->getObjectClassName(extension) == L"HLAobjectRoot.UmbraReferenceFixtureClass");

  REQUIRE_NOTHROW(extensionJoiner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(extensionJoiner->disconnect());
}

TEST_CASE(
    "Embedded attribute lookup resolves inherited definitions in the joined federation FOM",
    "[integration][development-profile][federation-management][rti.service.get-attribute-handle]"
    "[rti.service.get-attribute-name]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador ownerFederate;
  TestFederateAmbassador extensionFederate;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto extensionJoiner = makeRti();
  auto const federationName = nextFederationName();
  auto const baseFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const extensionFom = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "cpp" / "tests" / "data" / "reference-data-attribute-provider-fom.xml";
  ObjectClassHandle invalidObjectClass;
  AttributeHandle invalidAttribute;

  REQUIRE_THROWS_AS(
      unjoined->getAttributeHandle(invalidObjectClass, L"Name"),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->getAttributeName(invalidObjectClass, invalidAttribute),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->getAttributeHandle(invalidObjectClass, L"Name"),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->getAttributeName(invalidObjectClass, invalidAttribute),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(extensionJoiner->connect(extensionFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, baseFom, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(L"attribute-owner", L"owner", federationName));

  auto const employee = owner->getObjectClassHandle(L"HLAobjectRoot.Employee");
  auto const server = owner->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const customer = owner->getObjectClassHandle(L"HLAobjectRoot.Customer");
  auto const employeeName = owner->getAttributeHandle(employee, L"Name");
  REQUIRE(employeeName.isValid());
  REQUIRE(owner->getAttributeHandle(employee, L"Name") == employeeName);
  REQUIRE(owner->getAttributeHandle(server, L"Name") == employeeName);
  REQUIRE(owner->getAttributeName(employee, employeeName) == L"Name");
  REQUIRE(owner->getAttributeName(server, employeeName) == L"Name");
  REQUIRE_THROWS_AS(
      owner->getAttributeHandle(server, L"Missing"),
      rti1516_2025::NameNotFound);
  REQUIRE_THROWS_AS(
      owner->getAttributeHandle(invalidObjectClass, L"Name"),
      rti1516_2025::InvalidObjectClassHandle);
  REQUIRE_THROWS_AS(
      owner->getAttributeName(invalidObjectClass, employeeName),
      rti1516_2025::InvalidObjectClassHandle);
  REQUIRE_THROWS_AS(
      owner->getAttributeName(employee, invalidAttribute),
      rti1516_2025::InvalidAttributeHandle);
  REQUIRE_THROWS_AS(
      owner->getAttributeName(customer, employeeName),
      rti1516_2025::AttributeNotDefined);

  // A compatible additional-FOM join adds a new inheritance chain. The
  // original Employee::Name handle stays stable, while the extension's
  // defining attribute has the same handle through its child class.
  REQUIRE_NOTHROW(extensionJoiner->joinFederationExecution(
      L"attribute-extension",
      L"observer",
      federationName,
      std::vector<std::wstring>{extensionFom.wstring()}));
  REQUIRE(owner->getAttributeHandle(employee, L"Name") == employeeName);
  auto const extensionBase = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase");
  auto const extensionChild = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureClass");
  auto const identifier = owner->getAttributeHandle(extensionBase, L"Identifier");
  REQUIRE(identifier.isValid());
  REQUIRE(owner->getAttributeHandle(extensionChild, L"Identifier") == identifier);
  REQUIRE(owner->getAttributeName(extensionChild, identifier) == L"Identifier");

  REQUIRE_NOTHROW(extensionJoiner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(extensionJoiner->disconnect());
}

TEST_CASE(
    "Embedded interaction-class lookup services use stable handles from the joined federation FOM",
    "[integration][development-profile][federation-management][rti.service.get-interaction-class-handle]"
    "[rti.service.get-interaction-class-name]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador ownerFederate;
  TestFederateAmbassador extensionFederate;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto extensionJoiner = makeRti();
  auto const federationName = nextFederationName();
  auto const baseFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const extensionFom = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "cpp" / "tests" / "data" / "directed-interaction-interaction-provider-fom.xml";
  InteractionClassHandle invalid;

  REQUIRE_THROWS_AS(
      unjoined->getInteractionClassHandle(L"HLAinteractionRoot.ServerAction.TakeOrder"),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->getInteractionClassName(invalid),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->getInteractionClassHandle(L"HLAinteractionRoot.ServerAction.TakeOrder"),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->getInteractionClassName(invalid),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(extensionJoiner->connect(extensionFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, baseFom, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(L"lookup-owner", L"owner", federationName));

  auto const takeOrder = owner->getInteractionClassHandle(
      L"HLAinteractionRoot.ServerAction.TakeOrder");
  REQUIRE(takeOrder.isValid());
  REQUIRE(
      owner->getInteractionClassHandle(L"HLAinteractionRoot.ServerAction.TakeOrder") == takeOrder);
  REQUIRE(owner->getInteractionClassName(takeOrder) == L"HLAinteractionRoot.ServerAction.TakeOrder");
  REQUIRE_THROWS_AS(
      owner->getInteractionClassHandle(L"HLAinteractionRoot.Missing"),
      rti1516_2025::NameNotFound);
  REQUIRE_THROWS_AS(
      owner->getInteractionClassName(invalid),
      rti1516_2025::InvalidInteractionClassHandle);

  // A compatible additional-FOM join extends the catalog. The previously
  // returned interaction handle must remain stable while the new interaction
  // becomes discoverable from the same joined federation.
  REQUIRE_NOTHROW(extensionJoiner->joinFederationExecution(
      L"lookup-extension",
      L"observer",
      federationName,
      std::vector<std::wstring>{extensionFom.wstring()}));
  REQUIRE(
      owner->getInteractionClassHandle(L"HLAinteractionRoot.ServerAction.TakeOrder") == takeOrder);
  auto const extension = owner->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraDirectedFixtureInteraction");
  REQUIRE(extension.isValid());
  REQUIRE(extension != takeOrder);
  REQUIRE(
      owner->getInteractionClassName(extension) ==
      L"HLAinteractionRoot.UmbraDirectedFixtureInteraction");

  REQUIRE_NOTHROW(extensionJoiner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(extensionJoiner->disconnect());
}

TEST_CASE(
    "Embedded parameter lookup resolves inherited definitions in the joined federation FOM",
    "[integration][development-profile][federation-management][rti.service.get-parameter-handle]"
    "[rti.service.get-parameter-name]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador ownerFederate;
  TestFederateAmbassador extensionFederate;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto extensionJoiner = makeRti();
  auto const federationName = nextFederationName();
  auto const baseFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const extensionFom = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "cpp" / "tests" / "data" / "parameter-handle-provider-fom.xml";
  InteractionClassHandle invalidInteractionClass;
  ParameterHandle invalidParameter;

  REQUIRE_THROWS_AS(
      unjoined->getParameterHandle(invalidInteractionClass, L"TemperatureOk"),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->getParameterName(invalidInteractionClass, invalidParameter),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->getParameterHandle(invalidInteractionClass, L"TemperatureOk"),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->getParameterName(invalidInteractionClass, invalidParameter),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(extensionJoiner->connect(extensionFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, baseFom, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(L"parameter-owner", L"owner", federationName));

  auto const mainCourseServed = owner->getInteractionClassHandle(
      L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
  auto const takeOrder = owner->getInteractionClassHandle(
      L"HLAinteractionRoot.ServerAction.TakeOrder");
  auto const temperatureOk = owner->getParameterHandle(mainCourseServed, L"TemperatureOk");
  REQUIRE(temperatureOk.isValid());
  REQUIRE(owner->getParameterHandle(mainCourseServed, L"TemperatureOk") == temperatureOk);
  REQUIRE(owner->getParameterName(mainCourseServed, temperatureOk) == L"TemperatureOk");
  REQUIRE_THROWS_AS(
      owner->getParameterHandle(mainCourseServed, L"Missing"),
      rti1516_2025::NameNotFound);
  REQUIRE_THROWS_AS(
      owner->getParameterHandle(invalidInteractionClass, L"TemperatureOk"),
      rti1516_2025::InvalidInteractionClassHandle);
  REQUIRE_THROWS_AS(
      owner->getParameterName(invalidInteractionClass, temperatureOk),
      rti1516_2025::InvalidInteractionClassHandle);
  REQUIRE_THROWS_AS(
      owner->getParameterName(mainCourseServed, invalidParameter),
      rti1516_2025::InvalidParameterHandle);
  REQUIRE_THROWS_AS(
      owner->getParameterName(takeOrder, temperatureOk),
      rti1516_2025::InteractionParameterNotDefined);

  // A compatible additional-FOM join adds a new interaction inheritance
  // chain. The original Restaurant parameter handle remains stable, while the
  // extension's defining parameter has the same handle through its child.
  REQUIRE_NOTHROW(extensionJoiner->joinFederationExecution(
      L"parameter-extension",
      L"observer",
      federationName,
      std::vector<std::wstring>{extensionFom.wstring()}));
  REQUIRE(owner->getParameterHandle(mainCourseServed, L"TemperatureOk") == temperatureOk);
  auto const extensionBase = owner->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraParameterFixtureBase");
  auto const extensionChild = owner->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraParameterFixtureBase.UmbraParameterFixtureChild");
  auto const identifier = owner->getParameterHandle(extensionBase, L"Identifier");
  REQUIRE(identifier.isValid());
  REQUIRE(owner->getParameterHandle(extensionChild, L"Identifier") == identifier);
  REQUIRE(owner->getParameterName(extensionChild, identifier) == L"Identifier");

  REQUIRE_NOTHROW(extensionJoiner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(extensionJoiner->disconnect());
}

TEST_CASE(
    "Embedded 2025 dimension lookup follows FOM hierarchy and upper bounds",
    "[integration][development-profile][federation-management]"
    "[rti.service.get-available-dimensions-for-object-class]"
    "[rti.service.get-available-dimensions-for-interaction-class]"
    "[rti.service.get-dimension-handle][rti.service.get-dimension-name]"
    "[rti.service.get-dimension-upper-bound]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador ownerFederate;
  TestFederateAmbassador extensionFederate;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto extensionJoiner = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const dimensionConsumerFom = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "cpp" / "tests" / "data" / "dimension-reference-consumer-fom.xml";
  auto const dimensionProviderFom = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "cpp" / "tests" / "data" / "dimension-reference-provider-fom.xml";
  ObjectClassHandle invalidObjectClass;
  InteractionClassHandle invalidInteractionClass;
  DimensionHandle invalidDimension;

  REQUIRE_THROWS_AS(
      unjoined->getDimensionHandle(L"SodaFlavor"),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->getDimensionName(invalidDimension),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->getAvailableDimensionsForObjectClass(invalidObjectClass),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->getAvailableDimensionsForInteractionClass(invalidInteractionClass),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(extensionJoiner->connect(extensionFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(L"dimension-owner", L"owner", federationName));

  auto const barQuantity = owner->getDimensionHandle(L"BarQuantity");
  auto const sodaFlavor = owner->getDimensionHandle(L"SodaFlavor");
  auto const sweetener = owner->getDimensionHandle(L"Sweetener");
  auto const serverId = owner->getDimensionHandle(L"ServerId");
  REQUIRE(barQuantity.isValid());
  REQUIRE(sodaFlavor.isValid());
  REQUIRE(sweetener.isValid());
  REQUIRE(serverId.isValid());
  REQUIRE(owner->getDimensionHandle(L"SodaFlavor") == sodaFlavor);
  REQUIRE(owner->getDimensionName(sodaFlavor) == L"SodaFlavor");
  REQUIRE(owner->getDimensionUpperBound(barQuantity) == 25UL);
  REQUIRE(owner->getDimensionUpperBound(sodaFlavor) == 4UL);
  REQUIRE(owner->getDimensionUpperBound(sweetener) == 3UL);
  REQUIRE(owner->getDimensionUpperBound(serverId) == 20UL);

  auto const drink = owner->getObjectClassHandle(L"HLAobjectRoot.Food.Drink");
  auto const soda = owner->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const light = owner->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda.Light");
  DimensionHandleSet drinkDimensions = owner->getAvailableDimensionsForObjectClass(drink);
  DimensionHandleSet sodaDimensions = owner->getAvailableDimensionsForObjectClass(soda);
  DimensionHandleSet lightDimensions = owner->getAvailableDimensionsForObjectClass(light);
  REQUIRE(drinkDimensions == DimensionHandleSet{barQuantity});
  REQUIRE(sodaDimensions == DimensionHandleSet{barQuantity, sodaFlavor});
  REQUIRE(lightDimensions == DimensionHandleSet{barQuantity, sodaFlavor, sweetener});

  auto const foodServed = owner->getInteractionClassHandle(
      L"HLAinteractionRoot.CustomerTransactions.FoodServed");
  auto const mainCourseServed = owner->getInteractionClassHandle(
      L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
  REQUIRE(owner->getAvailableDimensionsForInteractionClass(foodServed).empty());
  REQUIRE(
      owner->getAvailableDimensionsForInteractionClass(mainCourseServed) ==
      DimensionHandleSet{serverId});

  // A later 2025 FOM join contributes a class, interaction, and dimension
  // from separate modules. Existing Restaurant handles stay stable while the
  // newly composed dimension is immediately available to lookup services.
  REQUIRE_NOTHROW(extensionJoiner->joinFederationExecution(
      L"dimension-extension",
      L"observer",
      federationName,
      std::vector<std::wstring>{dimensionConsumerFom.wstring(), dimensionProviderFom.wstring()}));
  REQUIRE(owner->getDimensionHandle(L"SodaFlavor") == sodaFlavor);
  auto const fixtureDimension = owner->getDimensionHandle(L"UmbraDimensionFixture");
  REQUIRE(fixtureDimension.isValid());
  REQUIRE(owner->getDimensionUpperBound(fixtureDimension) == 100UL);
  auto const fixtureObjectClass = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraDimensionFixtureObject");
  REQUIRE(
      owner->getAvailableDimensionsForObjectClass(fixtureObjectClass) ==
      DimensionHandleSet{fixtureDimension});
  auto const fixtureInteractionClass = owner->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraDimensionFixtureInteraction");
  REQUIRE(
      owner->getAvailableDimensionsForInteractionClass(fixtureInteractionClass) ==
      DimensionHandleSet{fixtureDimension});

  REQUIRE_THROWS_AS(
      owner->getDimensionHandle(L"MissingDimension"),
      rti1516_2025::NameNotFound);
  REQUIRE_THROWS_AS(
      owner->getDimensionName(invalidDimension),
      rti1516_2025::InvalidDimensionHandle);
  REQUIRE_THROWS_AS(
      owner->getDimensionUpperBound(invalidDimension),
      rti1516_2025::InvalidDimensionHandle);
  REQUIRE_THROWS_AS(
      owner->getAvailableDimensionsForObjectClass(invalidObjectClass),
      rti1516_2025::InvalidObjectClassHandle);
  REQUIRE_THROWS_AS(
      owner->getAvailableDimensionsForInteractionClass(invalidInteractionClass),
      rti1516_2025::InvalidInteractionClassHandle);

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(extensionJoiner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(extensionJoiner->disconnect());
}

TEST_CASE(
    "Embedded 2025 region templates preserve pending and committed range state",
    "[integration][development-profile][federation-management][ddm][region-lifecycle]"
    "[rti.service.create-region][rti.service.commit-region-modifications]"
    "[rti.service.delete-region][rti.service.get-dimension-handle-set]"
    "[rti.service.get-range-bounds][rti.service.set-range-bounds]"
    "[rti.service.decode-region-handle]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador ownerFederate;
  TestFederateAmbassador foreignFederate;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto foreign = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  RegionHandle invalidRegion;
  DimensionHandle invalidDimension;

  REQUIRE_THROWS_AS(
      unjoined->createRegion(DimensionHandleSet{}),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->decodeRegionHandle(VariableLengthData{}),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->createRegion(DimensionHandleSet{}),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->decodeRegionHandle(VariableLengthData{}),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->getDimensionHandleSet(invalidRegion),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(foreign->connect(foreignFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(L"region-owner", L"owner", federationName));
  REQUIRE_NOTHROW(foreign->joinFederationExecution(L"region-foreign", L"foreign", federationName));

  auto const barQuantity = owner->getDimensionHandle(L"BarQuantity");
  auto const sodaFlavor = owner->getDimensionHandle(L"SodaFlavor");
  REQUIRE(barQuantity.isValid());
  REQUIRE(sodaFlavor.isValid());

  auto const region = owner->createRegion(DimensionHandleSet{barQuantity, sodaFlavor});
  REQUIRE(region.isValid());
  REQUIRE(owner->getDimensionHandleSet(region) == DimensionHandleSet{barQuantity, sodaFlavor});
  REQUIRE(owner->decodeRegionHandle(region.encode()) == region);
  REQUIRE_THROWS_AS(
      owner->decodeRegionHandle(VariableLengthData{}),
      rti1516_2025::CouldNotDecode);

  REQUIRE_THROWS_AS(
      owner->getRangeBounds(region, barQuantity),
      rti1516_2025::InvalidRegion);
  REQUIRE_THROWS_AS(
      owner->commitRegionModifications(RegionHandleSet{region}),
      rti1516_2025::InvalidRegion);
  REQUIRE_THROWS_AS(
      owner->setRangeBounds(region, invalidDimension, RangeBounds(0UL, 10UL)),
      rti1516_2025::RegionDoesNotContainSpecifiedDimension);
  REQUIRE_THROWS_AS(
      owner->setRangeBounds(region, barQuantity, RangeBounds(10UL, 10UL)),
      rti1516_2025::InvalidRangeBound);
  REQUIRE_THROWS_AS(
      owner->setRangeBounds(region, barQuantity, RangeBounds(0UL, 26UL)),
      rti1516_2025::InvalidRangeBound);

  REQUIRE_NOTHROW(owner->setRangeBounds(region, barQuantity, RangeBounds(0UL, 10UL)));
  auto const pendingBar = owner->getRangeBounds(region, barQuantity);
  REQUIRE(pendingBar.getLowerBound() == 0UL);
  REQUIRE(pendingBar.getUpperBound() == 10UL);
  REQUIRE_THROWS_AS(
      owner->commitRegionModifications(RegionHandleSet{region}),
      rti1516_2025::InvalidRegion);
  REQUIRE(owner->getRangeBounds(region, barQuantity).getUpperBound() == 10UL);
  REQUIRE_THROWS_AS(
      owner->getRangeBounds(region, sodaFlavor),
      rti1516_2025::InvalidRegion);

  REQUIRE_THROWS_AS(
      foreign->commitRegionModifications(RegionHandleSet{region}),
      rti1516_2025::RegionNotCreatedByThisFederate);
  REQUIRE_THROWS_AS(
      foreign->deleteRegion(region),
      rti1516_2025::RegionNotCreatedByThisFederate);
  REQUIRE_THROWS_AS(
      foreign->getDimensionHandleSet(region),
      rti1516_2025::InvalidRegion);
  REQUIRE_THROWS_AS(
      foreign->getRangeBounds(region, barQuantity),
      rti1516_2025::InvalidRegion);
  REQUIRE_THROWS_AS(
      foreign->setRangeBounds(region, barQuantity, RangeBounds(0UL, 5UL)),
      rti1516_2025::RegionNotCreatedByThisFederate);

  REQUIRE_NOTHROW(owner->setRangeBounds(region, sodaFlavor, RangeBounds(1UL, 3UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{region}));
  auto const committedBar = owner->getRangeBounds(region, barQuantity);
  auto const committedSoda = owner->getRangeBounds(region, sodaFlavor);
  REQUIRE(committedBar.getLowerBound() == 0UL);
  REQUIRE(committedBar.getUpperBound() == 10UL);
  REQUIRE(committedSoda.getLowerBound() == 1UL);
  REQUIRE(committedSoda.getUpperBound() == 3UL);

  REQUIRE_NOTHROW(owner->setRangeBounds(region, barQuantity, RangeBounds(5UL, 15UL)));
  auto const replacement = owner->getRangeBounds(region, barQuantity);
  REQUIRE(replacement.getLowerBound() == 5UL);
  REQUIRE(replacement.getUpperBound() == 15UL);
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{region}));
  auto const recommitted = owner->getRangeBounds(region, barQuantity);
  REQUIRE(recommitted.getLowerBound() == 5UL);
  REQUIRE(recommitted.getUpperBound() == 15UL);

  REQUIRE_NOTHROW(owner->deleteRegion(region));
  REQUIRE_THROWS_AS(
      owner->getDimensionHandleSet(region),
      rti1516_2025::InvalidRegion);
  REQUIRE_THROWS_AS(
      owner->deleteRegion(region),
      rti1516_2025::InvalidRegion);

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(foreign->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(foreign->disconnect());
}

TEST_CASE(
    "Embedded interaction declaration services retain valid 2025 lifecycle and FOM boundaries",
    "[integration][development-profile][federation-management]"
    "[rti.service.publish-interaction-class][rti.service.unpublish-interaction-class]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador publisherFederate;
  TestFederateAmbassador subscriberFederate;
  auto unjoined = makeRti();
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  InteractionClassHandle invalid;

  REQUIRE_THROWS_AS(
      unjoined->publishInteractionClass(invalid),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->subscribeInteractionClass(invalid),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->unpublishInteractionClass(invalid),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->unsubscribeInteractionClass(invalid),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(publisher->connect(publisherFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      publisher->joinFederationExecution(L"declaration-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(
      subscriber->joinFederationExecution(L"declaration-subscriber", L"subscriber", federationName));

  REQUIRE_THROWS_AS(
      publisher->publishInteractionClass(invalid),
      rti1516_2025::InteractionClassNotDefined);
  REQUIRE_THROWS_AS(
      publisher->unpublishInteractionClass(invalid),
      rti1516_2025::InteractionClassNotDefined);
  REQUIRE_THROWS_AS(
      subscriber->subscribeInteractionClass(invalid),
      rti1516_2025::InteractionClassNotDefined);
  REQUIRE_THROWS_AS(
      subscriber->unsubscribeInteractionClass(invalid),
      rti1516_2025::InteractionClassNotDefined);

  auto const takeOrder = publisher->getInteractionClassHandle(
      L"HLAinteractionRoot.ServerAction.TakeOrder");
  REQUIRE(takeOrder.isValid());

  // Declaration state remains independent and idempotent per federate.  The
  // dedicated Send Interaction test below consumes this same state for
  // hierarchy-aware callback routing.
  REQUIRE_NOTHROW(publisher->publishInteractionClass(takeOrder));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(takeOrder));
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClass(takeOrder, false));
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClass(takeOrder, true));
  REQUIRE_NOTHROW(publisher->unpublishInteractionClass(takeOrder));
  REQUIRE_NOTHROW(publisher->unpublishInteractionClass(takeOrder));
  REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClass(takeOrder));
  REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClass(takeOrder));

  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(subscriber->disconnect());
}

TEST_CASE(
    "Embedded transportation type lookup exposes the mandatory 2025 support pair",
    "[integration][development-profile][federation-management]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.get-transportation-type-name]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador memberFederate;
  auto unjoined = makeRti();
  auto member = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  TransportationTypeHandle invalid;

  REQUIRE_THROWS_AS(
      unjoined->getTransportationTypeHandle(L"HLAreliable"),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->getTransportationTypeName(invalid),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->getTransportationTypeHandle(L"HLAreliable"),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->getTransportationTypeName(invalid),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(member->connect(memberFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      member->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      member->joinFederationExecution(L"transport-member", L"member", federationName));

  auto const reliable = member->getTransportationTypeHandle(L"HLAreliable");
  auto const bestEffort = member->getTransportationTypeHandle(L"HLAbestEffort");
  REQUIRE(reliable.isValid());
  REQUIRE(bestEffort.isValid());
  REQUIRE(reliable != bestEffort);
  REQUIRE(member->getTransportationTypeHandle(L"HLAreliable") == reliable);
  REQUIRE(member->getTransportationTypeName(reliable) == L"HLAreliable");
  REQUIRE(member->getTransportationTypeName(bestEffort) == L"HLAbestEffort");
  REQUIRE_THROWS_AS(
      member->getTransportationTypeHandle(L"UmbraCustomTransport"),
      rti1516_2025::InvalidTransportationName);
  REQUIRE_THROWS_AS(
      member->getTransportationTypeName(invalid),
      rti1516_2025::InvalidTransportationTypeHandle);

  REQUIRE_NOTHROW(member->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(member->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(member->disconnect());
}

TEST_CASE(
    "Embedded object class attribute declarations retain 2025 FOM and lifecycle boundaries",
    "[integration][development-profile][federation-management][declaration-management]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.unpublish-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.unsubscribe-object-class-attributes]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador publisherFederate;
  TestFederateAmbassador subscriberFederate;
  auto unjoined = makeRti();
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  ObjectClassHandle invalidObjectClass;
  AttributeHandle invalidAttribute;
  AttributeHandleSet const noAttributes;

  REQUIRE_THROWS_AS(
      publisher->publishObjectClassAttributes(invalidObjectClass, noAttributes),
      rti1516_2025::NotConnected);

  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->connect(publisherFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->subscribeObjectClassAttributes(invalidObjectClass, noAttributes),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(L"subscriber", federationName));

  auto const employee = publisher->getObjectClassHandle(L"HLAobjectRoot.Employee");
  auto const server = publisher->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const name = publisher->getAttributeHandle(employee, L"Name");
  auto const inheritedName = publisher->getAttributeHandle(server, L"Name");
  auto const efficiency = publisher->getAttributeHandle(server, L"Efficiency");
  REQUIRE(name == inheritedName);

  AttributeHandleSet const nameOnly{name};
  AttributeHandleSet const efficiencyOnly{efficiency};
  AttributeHandleSet const invalidOnly{invalidAttribute};
  REQUIRE_THROWS_AS(
      publisher->publishObjectClassAttributes(invalidObjectClass, nameOnly),
      rti1516_2025::ObjectClassNotDefined);
  REQUIRE_THROWS_AS(
      publisher->publishObjectClassAttributes(employee, invalidOnly),
      rti1516_2025::AttributeNotDefined);
  REQUIRE_THROWS_AS(
      publisher->publishObjectClassAttributes(employee, efficiencyOnly),
      rti1516_2025::AttributeNotDefined);

  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(employee, nameOnly));
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(server, nameOnly, false));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(server, efficiencyOnly, true));
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributes(server, nameOnly));
  REQUIRE_NOTHROW(publisher->unpublishObjectClassAttributes(employee, nameOnly));

  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(subscriber->disconnect());
}

TEST_CASE(
    "Embedded object-instance registration/discovery honors 2025 publication, promotion, and callback lifecycle",
    "[integration][development-profile][object-management]"
    "[rti.service.register-object-instance]"
    "[rti.service.get-known-object-class-handle]"
    "[rti.service.get-object-instance-handle]"
    "[rti.service.get-object-instance-name]"
    "[federate.callback.discover-object-instance]") {
  TestFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador exactReports;
  ReportingFederateAmbassador promotedReports;
  ReportingFederateAmbassador cancelledReports;
  ReportingFederateAmbassador implicitPrivilegeReports;
  ReportingFederateAmbassador lateReports;
  ReportingFederateAmbassador immediateReports;
  auto unjoined = makeRti();
  auto publisher = makeRti();
  auto exact = makeRti();
  auto promoted = makeRti();
  auto cancelled = makeRti();
  auto implicitPrivilege = makeRti();
  auto late = makeRti();
  auto immediate = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  ObjectClassHandle invalidObjectClass;
  ObjectInstanceHandle objectInstance;

  // The 2025 service preserves connection and membership preconditions ahead
  // of caller-supplied handle validation.
  REQUIRE_THROWS_AS(
      unjoined->registerObjectInstance(invalidObjectClass),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->registerObjectInstance(invalidObjectClass),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(exact->connect(exactReports, HLA_EVOKED));
  REQUIRE_NOTHROW(promoted->connect(promotedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(cancelled->connect(cancelledReports, HLA_EVOKED));
  REQUIRE_NOTHROW(implicitPrivilege->connect(implicitPrivilegeReports, HLA_EVOKED));
  REQUIRE_NOTHROW(late->connect(lateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(immediate->connect(immediateReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));

  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(
      publisherHandle = publisher->joinFederationExecution(
          L"object-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(exact->joinFederationExecution(
      L"object-exact", L"subscriber", federationName));
  REQUIRE_NOTHROW(promoted->joinFederationExecution(
      L"object-promoted", L"subscriber", federationName));
  REQUIRE_NOTHROW(cancelled->joinFederationExecution(
      L"object-cancelled", L"subscriber", federationName));
  REQUIRE_NOTHROW(implicitPrivilege->joinFederationExecution(
      L"object-implicit-privilege", L"subscriber", federationName));
  REQUIRE_NOTHROW(late->joinFederationExecution(
      L"object-late", L"subscriber", federationName));
  REQUIRE_NOTHROW(immediate->joinFederationExecution(
      L"object-immediate", L"subscriber", federationName));

  auto const employee = publisher->getObjectClassHandle(L"HLAobjectRoot.Employee");
  auto const server = publisher->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const name = publisher->getAttributeHandle(employee, L"Name");
  auto const efficiency = publisher->getAttributeHandle(server, L"Efficiency");
  auto const privilegeToDelete = publisher->getAttributeHandle(
      server,
      L"HLAprivilegeToDeleteObject");
  AttributeHandleSet const nameOnly{name};
  AttributeHandleSet const efficiencyOnly{efficiency};
  AttributeHandleSet const privilegeToDeleteOnly{privilegeToDelete};
  AttributeHandleSet const publishedAttributes{name, efficiency};

  REQUIRE_THROWS_AS(
      publisher->registerObjectInstance(invalidObjectClass),
      rti1516_2025::ObjectClassNotDefined);
  REQUIRE_THROWS_AS(
      publisher->registerObjectInstance(server),
      rti1516_2025::ObjectClassNotPublished);

  // A registered Server is discoverable as Server to its closest exact
  // subscribers and as Employee to a subscriber only at that superclass.
  // Passive/active only affects advisory behavior outside this narrow slice,
  // so both forms remain eligible for discovery.
  REQUIRE_NOTHROW(exact->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(promoted->subscribeObjectClassAttributes(employee, nameOnly, false));
  REQUIRE_NOTHROW(cancelled->subscribeObjectClassAttributes(server, efficiencyOnly));
  // Publishing an ordinary attribute establishes implicit publication of the
  // standard HLAprivilegeToDeleteObject attribute for this class epoch. Its
  // subscriber therefore proves that registration snapshots the complete
  // currently published set, rather than only the explicit arguments.
  REQUIRE_NOTHROW(
      implicitPrivilege->subscribeObjectClassAttributes(server, privilegeToDeleteOnly));
  REQUIRE_NOTHROW(immediate->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(server, publishedAttributes));

  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(server));
  REQUIRE(objectInstance.isValid());
  auto const objectInstanceName = publisher->getObjectInstanceName(objectInstance);
  REQUIRE_FALSE(objectInstanceName.empty());
  REQUIRE(publisher->getObjectInstanceHandle(objectInstanceName) == objectInstance);
  REQUIRE(publisher->getKnownObjectClassHandle(objectInstance) == server);

  // HLA_IMMEDIATE gets its induced discovery callback during registration;
  // HLA_EVOKED recipients remain unknown until their callback is evoked.
  REQUIRE(immediateReports.objectDiscoveryReports.size() == 1);
  REQUIRE(exactReports.objectDiscoveryReports.empty());
  REQUIRE(promotedReports.objectDiscoveryReports.empty());
  REQUIRE(cancelledReports.objectDiscoveryReports.empty());
  REQUIRE(implicitPrivilegeReports.objectDiscoveryReports.empty());
  REQUIRE(lateReports.objectDiscoveryReports.empty());
  REQUIRE_THROWS_AS(
      exact->getKnownObjectClassHandle(objectInstance),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      late->getObjectInstanceHandle(objectInstanceName),
      rti1516_2025::ObjectInstanceNotKnown);

  // A queued callback must re-evaluate eligibility after an unsubscribe. A
  // later subscription to an already-registered instance must independently
  // plan the new discovery callback.
  REQUIRE_NOTHROW(cancelled->unsubscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(late->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_FALSE(exact->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(promoted->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(cancelled->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(implicitPrivilege->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(late->evokeMultipleCallbacks(0.0, 0.0));

  auto requireDiscovery = [&](ReportingFederateAmbassador::ObjectDiscoveryReport const& report,
                              ObjectClassHandle const& expectedClass) {
    REQUIRE(report.objectInstance == objectInstance);
    REQUIRE(report.objectClass == expectedClass);
    REQUIRE(report.objectInstanceName == objectInstanceName);
    REQUIRE(report.producingFederate == publisherHandle);
  };

  REQUIRE(exactReports.objectDiscoveryReports.size() == 1);
  requireDiscovery(exactReports.objectDiscoveryReports.front(), server);
  REQUIRE(promotedReports.objectDiscoveryReports.size() == 1);
  requireDiscovery(promotedReports.objectDiscoveryReports.front(), employee);
  REQUIRE(implicitPrivilegeReports.objectDiscoveryReports.size() == 1);
  requireDiscovery(implicitPrivilegeReports.objectDiscoveryReports.front(), server);
  REQUIRE(lateReports.objectDiscoveryReports.size() == 1);
  requireDiscovery(lateReports.objectDiscoveryReports.front(), server);
  REQUIRE(immediateReports.objectDiscoveryReports.size() == 1);
  requireDiscovery(immediateReports.objectDiscoveryReports.front(), server);
  REQUIRE(cancelledReports.objectDiscoveryReports.empty());
  REQUIRE(publisherReports.objectDiscoveryReports.empty());

  REQUIRE(exact->getKnownObjectClassHandle(objectInstance) == server);
  REQUIRE(promoted->getKnownObjectClassHandle(objectInstance) == employee);
  REQUIRE(implicitPrivilege->getKnownObjectClassHandle(objectInstance) == server);
  REQUIRE(late->getKnownObjectClassHandle(objectInstance) == server);
  REQUIRE(immediate->getKnownObjectClassHandle(objectInstance) == server);
  REQUIRE(exact->getObjectInstanceName(objectInstance) == objectInstanceName);
  REQUIRE(promoted->getObjectInstanceHandle(objectInstanceName) == objectInstance);
  REQUIRE_THROWS_AS(
      cancelled->getObjectInstanceName(objectInstance),
      rti1516_2025::ObjectInstanceNotKnown);

  // Known-instance state is stable, so repeating a declaration does not
  // generate a duplicate discovery callback.
  REQUIRE_NOTHROW(exact->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_FALSE(exact->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(exactReports.objectDiscoveryReports.size() == 1);

  REQUIRE_NOTHROW(immediate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(late->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(implicitPrivilege->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(cancelled->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(promoted->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(exact->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(exact->disconnect());
  REQUIRE_NOTHROW(promoted->disconnect());
  REQUIRE_NOTHROW(cancelled->disconnect());
  REQUIRE_NOTHROW(implicitPrivilege->disconnect());
  REQUIRE_NOTHROW(late->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
}

TEST_CASE(
    "Embedded 2025 named object-instance registration consumes reservations",
    "[integration][development-profile][object-management]"
    "[rti.service.register-object-instance-named]"
    "[rti.service.register-object-instance-with-regions-named]"
    "[rti.service.reserve-object-instance-name]"
    "[rti.service.release-object-instance-name]"
    "[federate.callback.discover-object-instance]") {
  TestFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador peerReports;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  ObjectClassHandle invalidObjectClass;

  REQUIRE_THROWS_AS(
      unjoined->registerObjectInstance(invalidObjectClass, L"Named-Not-Connected"),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->registerObjectInstance(invalidObjectClass, L"Named-Not-Joined"),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"named-owner", L"owner", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"named-peer", L"peer", federationName));

  auto const employee = owner->getObjectClassHandle(L"HLAobjectRoot.Employee");
  auto const server = owner->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const name = owner->getAttributeHandle(employee, L"Name");
  auto const efficiency = owner->getAttributeHandle(server, L"Efficiency");
  AttributeHandleSet const nameOnly{name};
  AttributeHandleSet const efficiencyOnly{efficiency};
  AttributeHandleSet const publishedAttributes{name, efficiency};

  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, publishedAttributes));
  REQUIRE_NOTHROW(peer->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(peer->subscribeObjectClassAttributes(server, nameOnly));

  std::wstring const firstName = L"UmbraNamedEmployee-1";
  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(firstName));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.objectInstanceNameReservationSucceededReports.size() == 1);
  REQUIRE(ownerReports.objectInstanceNameReservationSucceededReports.front().objectInstanceName ==
          firstName);

  // Reservation ownership is enforced at registration; the peer has
  // published the class but cannot consume the owner's reservation.
  REQUIRE_THROWS_AS(
      peer->registerObjectInstance(server, firstName),
      rti1516_2025::ObjectInstanceNameNotReserved);

  ObjectInstanceHandle firstInstance;
  REQUIRE_NOTHROW(firstInstance = owner->registerObjectInstance(server, firstName));
  REQUIRE(firstInstance.isValid());
  REQUIRE(owner->getObjectInstanceName(firstInstance) == firstName);
  REQUIRE(owner->getObjectInstanceHandle(firstName) == firstInstance);
  REQUIRE_THROWS_AS(
      owner->releaseObjectInstanceName(firstName),
      rti1516_2025::ObjectInstanceNameNotReserved);
  REQUIRE_THROWS_AS(
      peer->registerObjectInstance(server, firstName),
      rti1516_2025::ObjectInstanceNameInUse);

  REQUIRE(peerReports.objectDiscoveryReports.empty());
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.objectDiscoveryReports.size() == 1);
  REQUIRE(peerReports.objectDiscoveryReports.front().objectInstance == firstInstance);
  REQUIRE(peerReports.objectDiscoveryReports.front().objectClass == server);
  REQUIRE(peerReports.objectDiscoveryReports.front().objectInstanceName == firstName);

  // A failed registration does not consume a valid reservation. Once the
  // class is published, the same name can be used successfully.
  std::wstring const secondName = L"UmbraNamedEmployee-2";
  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(secondName));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_THROWS_AS(
      owner->registerObjectInstance(employee, secondName),
      rti1516_2025::ObjectClassNotPublished);
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(employee, nameOnly));
  REQUIRE_NOTHROW(peer->subscribeObjectClassAttributes(employee, nameOnly));

  ObjectInstanceHandle secondInstance;
  REQUIRE_NOTHROW(secondInstance = owner->registerObjectInstance(employee, secondName));
  REQUIRE(secondInstance.isValid());
  REQUIRE(owner->getObjectInstanceName(secondInstance) == secondName);
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.objectDiscoveryReports.size() == 2);
  REQUIRE(peerReports.objectDiscoveryReports.back().objectInstance == secondInstance);
  REQUIRE(peerReports.objectDiscoveryReports.back().objectClass == employee);
  REQUIRE(peerReports.objectDiscoveryReports.back().objectInstanceName == secondName);

  // A regional named registration consumes the same reservation state after
  // the existing 2025 regional validation path has accepted its association.
  auto const soda = owner->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = owner->getAttributeHandle(soda, L"Flavor");
  auto const sodaFlavor = owner->getDimensionHandle(L"SodaFlavor");
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(soda, flavorOnly));
  auto const ownerRegion = owner->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(owner->setRangeBounds(
      ownerRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{ownerRegion}));
  AttributeHandleSetRegionHandleSetPairVector const regionalPair{{
      flavorOnly,
      RegionHandleSet{ownerRegion},
  }};
  REQUIRE_THROWS_AS(
      owner->registerObjectInstanceWithRegions(
          soda,
          regionalPair,
          L"UmbraRegionalUnreserved"),
      rti1516_2025::ObjectInstanceNameNotReserved);

  std::wstring const regionalName = L"UmbraRegional-1";
  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(regionalName));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  ObjectInstanceHandle regionalInstance;
  REQUIRE_NOTHROW(regionalInstance = owner->registerObjectInstanceWithRegions(
      soda,
      regionalPair,
      regionalName));
  REQUIRE(regionalInstance.isValid());
  REQUIRE(owner->getObjectInstanceName(regionalInstance) == regionalName);
  REQUIRE(owner->getObjectInstanceHandle(regionalName) == regionalInstance);
  REQUIRE_THROWS_AS(
      owner->releaseObjectInstanceName(regionalName),
      rti1516_2025::ObjectInstanceNameNotReserved);

  // Release before registration returns the name to the federation-wide
  // pool, allowing a different joined federate to reserve and consume it.
  std::wstring const reusableName = L"UmbraNamedReusable";
  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(reusableName));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_NOTHROW(owner->releaseObjectInstanceName(reusableName));
  REQUIRE_NOTHROW(peer->reserveObjectInstanceName(reusableName));
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  ObjectInstanceHandle peerInstance;
  REQUIRE_NOTHROW(peerInstance = peer->registerObjectInstance(server, reusableName));
  REQUIRE(peerInstance.isValid());
  REQUIRE(peer->getObjectInstanceName(peerInstance) == reusableName);
  REQUIRE_THROWS_AS(
      peer->releaseObjectInstanceName(reusableName),
      rti1516_2025::ObjectInstanceNameNotReserved);

  REQUIRE_NOTHROW(peer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}

TEST_CASE(
    "Embedded receive-order Delete Object Instance honors 2025 removal lifecycle",
    "[integration][development-profile][object-management]"
    "[rti.service.delete-object-instance]"
    "[federate.callback.remove-object-instance]") {
  TestFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador evokedReports;
  ReportingFederateAmbassador immediateReports;
  auto unjoined = makeRti();
  auto publisher = makeRti();
  auto evoked = makeRti();
  auto immediate = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  ObjectInstanceHandle invalidObjectInstance;

  // The 2025 service preserves connection and membership preconditions ahead
  // of caller-supplied handle validation.
  REQUIRE_THROWS_AS(
      publisher->deleteObjectInstance(invalidObjectInstance, VariableLengthData()),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->deleteObjectInstance(invalidObjectInstance, VariableLengthData()),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(evoked->connect(evokedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(immediate->connect(immediateReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));

  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(
      publisherHandle = publisher->joinFederationExecution(
          L"delete-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(evoked->joinFederationExecution(
      L"delete-evoked", L"subscriber", federationName));
  REQUIRE_NOTHROW(immediate->joinFederationExecution(
      L"delete-immediate", L"subscriber", federationName));

  auto const server = publisher->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const efficiency = publisher->getAttributeHandle(server, L"Efficiency");
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(evoked->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(immediate->subscribeObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(server));
  auto const objectInstanceName = publisher->getObjectInstanceName(objectInstance);
  REQUIRE(immediateReports.objectDiscoveryReports.size() == 1);
  REQUIRE(evokedReports.objectDiscoveryReports.empty());
  REQUIRE_FALSE(evoked->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(evokedReports.objectDiscoveryReports.size() == 1);
  REQUIRE(evoked->getKnownObjectClassHandle(objectInstance) == server);

  unsigned char const deleteTagBytes[] = {0xD3, 0x1E, 0x7E};
  VariableLengthData const deleteTag(deleteTagBytes, sizeof(deleteTagBytes));
  REQUIRE_THROWS_AS(
      publisher->deleteObjectInstance(invalidObjectInstance, deleteTag),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      evoked->deleteObjectInstance(objectInstance, deleteTag),
      rti1516_2025::DeletePrivilegeNotHeld);

  // The deleting producer is immediately unknown and never receives its own
  // induced removal. An evoked recipient remains known until that callback
  // begins; an immediate recipient observes removal during the service call.
  REQUIRE_NOTHROW(publisher->deleteObjectInstance(objectInstance, deleteTag));
  REQUIRE(publisherReports.objectRemovalReports.empty());
  REQUIRE(immediateReports.objectRemovalReports.size() == 1);
  REQUIRE(evokedReports.objectRemovalReports.empty());
  REQUIRE_THROWS_AS(
      publisher->getObjectInstanceName(objectInstance),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE(evoked->getObjectInstanceHandle(objectInstanceName) == objectInstance);
  REQUIRE_THROWS_AS(
      immediate->getKnownObjectClassHandle(objectInstance),
      rti1516_2025::ObjectInstanceNotKnown);

  auto requireRemoval = [&](ReportingFederateAmbassador::ObjectRemovalReport const& report) {
    REQUIRE(report.objectInstance == objectInstance);
    REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
            std::vector<unsigned char>{deleteTagBytes[0], deleteTagBytes[1], deleteTagBytes[2]});
    REQUIRE(report.producingFederate == publisherHandle);
  };
  requireRemoval(immediateReports.objectRemovalReports.front());

  REQUIRE_FALSE(evoked->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(evokedReports.objectRemovalReports.size() == 1);
  requireRemoval(evokedReports.objectRemovalReports.front());
  REQUIRE_THROWS_AS(
      evoked->getObjectInstanceHandle(objectInstanceName),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      publisher->deleteObjectInstance(objectInstance, deleteTag),
      rti1516_2025::ObjectInstanceNotKnown);

  REQUIRE_NOTHROW(immediate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(evoked->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(evoked->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
}

TEST_CASE(
    "Embedded Local Delete Object Instance preserves 2025 federation state",
    "[integration][development-profile][object-management]"
    "[rti.service.local-delete-object-instance]") {
  TestFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  ObjectInstanceHandle invalidObjectInstance;

  REQUIRE_THROWS_AS(
      owner->localDeleteObjectInstance(invalidObjectInstance),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->localDeleteObjectInstance(invalidObjectInstance),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"local-delete-owner", L"owner", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"local-delete-requester", L"requester", federationName));

  auto const server = owner->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const efficiency = owner->getAttributeHandle(server, L"Efficiency");
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(server));
  REQUIRE(requesterReports.objectDiscoveryReports.empty());
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE(requester->getKnownObjectClassHandle(objectInstance) == server);

  // The producer owns the registration-established attribute and therefore
  // cannot use Local Delete Object Instance until ownership has moved away.
  REQUIRE_THROWS_AS(
      owner->localDeleteObjectInstance(objectInstance),
      rti1516_2025::FederateOwnsAttributes);

  unsigned char const tagBytes[] = {0x51, 0x18};
  VariableLengthData const acquisitionTag(tagBytes, sizeof(tagBytes));
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance,
      efficiencyOnly,
      acquisitionTag));
  REQUIRE_THROWS_AS(
      requester->localDeleteObjectInstance(objectInstance),
      rti1516_2025::OwnershipAcquisitionPending);
  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));

  // Rejoin a fresh requester so the local-delete transition itself is tested
  // without carrying an acquisition reservation across the teardown boundary.
  requester = makeRti();
  requesterReports = ReportingFederateAmbassador{};
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"local-delete-requester-2", L"requester", federationName));
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE_NOTHROW(requester->localDeleteObjectInstance(objectInstance));
  REQUIRE_THROWS_AS(
      requester->getKnownObjectClassHandle(objectInstance),
      rti1516_2025::ObjectInstanceNotKnown);

  // Local deletion does not remove the federation-wide object. Repeating the
  // eligible subscription permits a new discovery, and the owner remains able
  // to update the object for the rediscovered requester.
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 2);
  REQUIRE(requester->getKnownObjectClassHandle(objectInstance) == server);

  unsigned char const valueBytes[] = {0x04, 0x20};
  unsigned char const updateTagBytes[] = {0x7A};
  AttributeHandleValueMap values;
  values.emplace(efficiency, VariableLengthData(valueBytes, sizeof(valueBytes)));
  VariableLengthData const updateTag(updateTagBytes, sizeof(updateTagBytes));
  REQUIRE_NOTHROW(owner->updateAttributeValues(objectInstance, values, updateTag));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeReflectionReports.size() == 1);
  REQUIRE(requesterReports.attributeReflectionReports.front().objectInstance == objectInstance);

  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}

TEST_CASE(
    "Embedded receive-order Update Attribute Values honors 2025 passel and callback lifecycle",
    "[integration][development-profile][object-management]"
    "[rti.service.update-attribute-values][federate.callback.reflect-attribute-values]") {
  TestFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador exactReports;
  ReportingFederateAmbassador promotedReports;
  ReportingFederateAmbassador cancelledReports;
  ReportingFederateAmbassador immediateReports;
  auto unjoined = makeRti();
  auto publisher = makeRti();
  auto exact = makeRti();
  auto promoted = makeRti();
  auto cancelled = makeRti();
  auto immediate = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0xC0, 0x25, 0xA4};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  ObjectInstanceHandle invalidObjectInstance;
  AttributeHandleValueMap noAttributeValues;

  // The service retains its connection and membership preconditions ahead of
  // validation of its object handle and value map.
  REQUIRE_THROWS_AS(
      unjoined->updateAttributeValues(invalidObjectInstance, noAttributeValues, tag),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->updateAttributeValues(invalidObjectInstance, noAttributeValues, tag),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(exact->connect(exactReports, HLA_EVOKED));
  REQUIRE_NOTHROW(promoted->connect(promotedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(cancelled->connect(cancelledReports, HLA_EVOKED));
  REQUIRE_NOTHROW(immediate->connect(immediateReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));

  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(
      publisherHandle = publisher->joinFederationExecution(
          L"attribute-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(exact->joinFederationExecution(
      L"attribute-exact", L"subscriber", federationName));
  REQUIRE_NOTHROW(promoted->joinFederationExecution(
      L"attribute-promoted", L"subscriber", federationName));
  REQUIRE_NOTHROW(cancelled->joinFederationExecution(
      L"attribute-cancelled", L"subscriber", federationName));
  REQUIRE_NOTHROW(immediate->joinFederationExecution(
      L"attribute-immediate", L"subscriber", federationName));

  auto const base = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase");
  auto const child = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableBaseA = publisher->getAttributeHandle(child, L"ReliableBaseA");
  auto const reliableBaseB = publisher->getAttributeHandle(child, L"ReliableBaseB");
  auto const bestEffortBase = publisher->getAttributeHandle(child, L"BestEffortBase");
  auto const reliableChild = publisher->getAttributeHandle(child, L"ReliableChild");
  auto const unownedChild = publisher->getAttributeHandle(child, L"UnownedChild");
  REQUIRE(base.isValid());
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(reliableBaseB.isValid());
  REQUIRE(bestEffortBase.isValid());
  REQUIRE(reliableChild.isValid());
  REQUIRE(unownedChild.isValid());

  AttributeHandleSet const allOwned{
      reliableBaseA,
      reliableBaseB,
      bestEffortBase,
      reliableChild,
  };
  AttributeHandleSet const baseAttributes{
      reliableBaseA,
      reliableBaseB,
      bestEffortBase,
  };
  AttributeHandleSet const reliableChildOnly{reliableChild};

  // The promoted receiver uses a passive superclass subscription. Passive
  // changes registration/advisory behavior, not attribute reflection
  // eligibility. The cancelled receiver starts eligible so that its later
  // unsubscribe exercises callback-time suppression.
  REQUIRE_NOTHROW(exact->subscribeObjectClassAttributes(child, allOwned));
  REQUIRE_NOTHROW(promoted->subscribeObjectClassAttributes(base, baseAttributes, false));
  REQUIRE_NOTHROW(cancelled->subscribeObjectClassAttributes(child, reliableChildOnly));
  REQUIRE_NOTHROW(immediate->subscribeObjectClassAttributes(child, allOwned));
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(child, allOwned));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(child));
  REQUIRE(objectInstance.isValid());
  REQUIRE(immediateReports.objectDiscoveryReports.size() == 1);
  REQUIRE(exactReports.objectDiscoveryReports.empty());
  REQUIRE(promotedReports.objectDiscoveryReports.empty());
  REQUIRE(cancelledReports.objectDiscoveryReports.empty());
  REQUIRE_FALSE(exact->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(promoted->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(cancelled->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(exactReports.objectDiscoveryReports.size() == 1);
  REQUIRE(promotedReports.objectDiscoveryReports.size() == 1);
  REQUIRE(cancelledReports.objectDiscoveryReports.size() == 1);
  REQUIRE(exact->getKnownObjectClassHandle(objectInstance) == child);
  REQUIRE(promoted->getKnownObjectClassHandle(objectInstance) == base);
  REQUIRE(cancelled->getKnownObjectClassHandle(objectInstance) == child);

  unsigned char const reliableBaseABytes[] = {0x01, 0x02};
  unsigned char const reliableBaseBBytes[] = {0x03, 0x04};
  unsigned char const bestEffortBaseBytes[] = {0x05, 0x06};
  unsigned char const reliableChildBytes[] = {0x07, 0x08};
  AttributeHandleValueMap attributeValues;
  attributeValues.emplace(
      reliableBaseA,
      VariableLengthData(reliableBaseABytes, sizeof(reliableBaseABytes)));
  attributeValues.emplace(
      reliableBaseB,
      VariableLengthData(reliableBaseBBytes, sizeof(reliableBaseBBytes)));
  attributeValues.emplace(
      bestEffortBase,
      VariableLengthData(bestEffortBaseBytes, sizeof(bestEffortBaseBytes)));
  attributeValues.emplace(
      reliableChild,
      VariableLengthData(reliableChildBytes, sizeof(reliableChildBytes)));

  AttributeHandleValueMap unownedValues;
  unownedValues.emplace(unownedChild, VariableLengthData(reliableChildBytes, sizeof(reliableChildBytes)));
  REQUIRE_THROWS_AS(
      publisher->updateAttributeValues(invalidObjectInstance, attributeValues, tag),
      rti1516_2025::ObjectInstanceNotKnown);
  AttributeHandle invalidAttribute;
  AttributeHandleValueMap invalidAttributeValues;
  invalidAttributeValues.emplace(invalidAttribute, VariableLengthData());
  REQUIRE_THROWS_AS(
      publisher->updateAttributeValues(objectInstance, invalidAttributeValues, tag),
      rti1516_2025::AttributeNotDefined);
  REQUIRE_THROWS_AS(
      publisher->updateAttributeValues(objectInstance, unownedValues, tag),
      rti1516_2025::AttributeNotOwned);
  AttributeHandleValueMap exactValues;
  exactValues.emplace(reliableChild, VariableLengthData(reliableChildBytes, sizeof(reliableChildBytes)));
  REQUIRE_THROWS_AS(
      exact->updateAttributeValues(objectInstance, exactValues, tag),
      rti1516_2025::AttributeNotOwned);

  // One no-time request contains two immutable passels: the three reliable
  // values stay together, and the best-effort value remains separate. The
  // child-only attribute is deliberately absent from the promoted receiver's
  // known superclass projection.
  REQUIRE_NOTHROW(publisher->updateAttributeValues(objectInstance, attributeValues, tag));
  REQUIRE(immediateReports.attributeReflectionReports.size() == 2);
  REQUIRE(exactReports.attributeReflectionReports.empty());
  REQUIRE(promotedReports.attributeReflectionReports.empty());
  REQUIRE(cancelledReports.attributeReflectionReports.empty());

  REQUIRE_NOTHROW(cancelled->unsubscribeObjectClassAttributes(child, reliableChildOnly));
  // With a zero maximum interval, the callback model processes one queued
  // callback per invocation and reports whether another passel remains.
  REQUIRE(exact->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(exact->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(promoted->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(promoted->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(cancelled->evokeMultipleCallbacks(0.0, 0.0));

  auto const reliableTransportation = publisher->getTransportationTypeHandle(L"HLAreliable");
  auto const bestEffortTransportation = publisher->getTransportationTypeHandle(L"HLAbestEffort");
  auto reflectionFor = [](
                           std::vector<ReportingFederateAmbassador::AttributeReflectionReport> const& reports,
                           TransportationTypeHandle const& transportationType) {
    auto const found = std::find_if(
        reports.begin(),
        reports.end(),
        [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
          return report.transportationType == transportationType;
        });
    REQUIRE(found != reports.end());
    return &*found;
  };
  auto requireReflection = [&](ReportingFederateAmbassador::AttributeReflectionReport const& report,
                               TransportationTypeHandle const& transportationType,
                               std::vector<AttributeHandle> const& expectedAttributes) {
    REQUIRE(report.objectInstance == objectInstance);
    REQUIRE(report.attributeValues.size() == expectedAttributes.size());
    for (AttributeHandle const& attribute : expectedAttributes) {
      auto const expected = attributeValues.find(attribute);
      REQUIRE(expected != attributeValues.end());
      auto const received = report.attributeValues.find(attribute);
      REQUIRE(received != report.attributeValues.end());
      REQUIRE(variableLengthDataBytes(received->second) == variableLengthDataBytes(expected->second));
    }
    REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
            std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
    REQUIRE(report.transportationType == transportationType);
    REQUIRE(report.producingFederate == publisherHandle);
    REQUIRE_FALSE(report.sentRegionsSupplied);
  };

  REQUIRE(exactReports.attributeReflectionReports.size() == 2);
  requireReflection(
      *reflectionFor(exactReports.attributeReflectionReports, reliableTransportation),
      reliableTransportation,
      {reliableBaseA, reliableBaseB, reliableChild});
  requireReflection(
      *reflectionFor(exactReports.attributeReflectionReports, bestEffortTransportation),
      bestEffortTransportation,
      {bestEffortBase});
  REQUIRE(promotedReports.attributeReflectionReports.size() == 2);
  requireReflection(
      *reflectionFor(promotedReports.attributeReflectionReports, reliableTransportation),
      reliableTransportation,
      {reliableBaseA, reliableBaseB});
  requireReflection(
      *reflectionFor(promotedReports.attributeReflectionReports, bestEffortTransportation),
      bestEffortTransportation,
      {bestEffortBase});
  REQUIRE(immediateReports.attributeReflectionReports.size() == 2);
  requireReflection(
      *reflectionFor(immediateReports.attributeReflectionReports, reliableTransportation),
      reliableTransportation,
      {reliableBaseA, reliableBaseB, reliableChild});
  requireReflection(
      *reflectionFor(immediateReports.attributeReflectionReports, bestEffortTransportation),
      bestEffortTransportation,
      {bestEffortBase});
  REQUIRE(cancelledReports.attributeReflectionReports.empty());
  REQUIRE(publisherReports.attributeReflectionReports.empty());

  REQUIRE_NOTHROW(immediate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(cancelled->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(promoted->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(exact->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(exact->disconnect());
  REQUIRE_NOTHROW(promoted->disconnect());
  REQUIRE_NOTHROW(cancelled->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
}

TEST_CASE(
    "Embedded regional object attributes filter 2025 no-time updates by overlap",
    "[integration][development-profile][federation-management][ddm]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.associate-regions-for-updates]"
    "[rti.service.unassociate-regions-for-updates]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.unsubscribe-object-class-attributes-with-regions]"
    "[rti.service.update-attribute-values]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador subscriberReports;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"regional-object-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"regional-object-subscriber", L"subscriber", federationName));

  auto const soda = publisher->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = publisher->getAttributeHandle(soda, L"Flavor");
  auto const sodaFlavor = publisher->getDimensionHandle(L"SodaFlavor");
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));

  auto const publisherRegion = publisher->createRegion(DimensionHandleSet{sodaFlavor});
  auto const subscriberRegion = subscriber->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      subscriberRegion,
      sodaFlavor,
      RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(RegionHandleSet{subscriberRegion}));

  AttributeHandleSetRegionHandleSetPairVector const regionalPair{{
      flavorOnly,
      RegionHandleSet{publisherRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const subscriberPair{{
      flavorOnly,
      RegionHandleSet{subscriberRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const emptyRegionPair{{
      flavorOnly,
      RegionHandleSet{},
  }};
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      soda,
      regionalPair));
  REQUIRE(objectInstance.isValid());
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      soda,
      subscriberPair));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      soda,
      emptyRegionPair));
  REQUIRE_NOTHROW(publisher->associateRegionsForUpdates(
      objectInstance,
      emptyRegionPair));
  REQUIRE(subscriberReports.objectDiscoveryReports.empty());

  unsigned char const firstValueBytes[] = {0x10, 0x25};
  AttributeHandleValueMap firstValue;
  firstValue.emplace(
      flavor,
      VariableLengthData(firstValueBytes, sizeof(firstValueBytes)));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      objectInstance,
      firstValue,
      VariableLengthData()));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.attributeReflectionReports.empty());

  // Association is additive and idempotent. The disjoint subscriber remains
  // undiscoverable even after the producer repeats the association explicitly.
  REQUIRE_NOTHROW(publisher->associateRegionsForUpdates(objectInstance, regionalPair));
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      subscriberRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(RegionHandleSet{subscriberRegion}));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      soda,
      subscriberPair));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.objectDiscoveryReports.size() == 1);
  REQUIRE(subscriber->getKnownObjectClassHandle(objectInstance) == soda);

  unsigned char const secondValueBytes[] = {0x20, 0x25};
  AttributeHandleValueMap secondValue;
  secondValue.emplace(
      flavor,
      VariableLengthData(secondValueBytes, sizeof(secondValueBytes)));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      objectInstance,
      secondValue,
      VariableLengthData()));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.attributeReflectionReports.size() == 1);
  REQUIRE(subscriberReports.attributeReflectionReports.front().sentRegionsSupplied);
  REQUIRE(subscriberReports.attributeReflectionReports.front().sentRegions.contains(publisherRegion));
  REQUIRE(
      subscriberReports.attributeReflectionReports.front().attributeValues.contains(flavor));

  // Changing the committed subscriber region to a disjoint range removes the
  // regional reflection route without changing ordinary known-instance state.
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      subscriberRegion,
      sodaFlavor,
      RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(RegionHandleSet{subscriberRegion}));
  unsigned char const disjointValueBytes[] = {0x30, 0x25};
  AttributeHandleValueMap disjointValue;
  disjointValue.emplace(
      flavor,
      VariableLengthData(disjointValueBytes, sizeof(disjointValueBytes)));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      objectInstance,
      disjointValue,
      VariableLengthData()));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.attributeReflectionReports.size() == 1);

  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, regionalPair));
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, emptyRegionPair));
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(
      soda,
      subscriberPair));
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(
      soda,
      emptyRegionPair));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(subscriber->deleteRegion(subscriberRegion));

  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(subscriber->disconnect());
}

TEST_CASE(
    "Embedded regional object scope callbacks follow 2025 region, association, and subscription changes",
    "[integration][development-profile][federation-management][ddm]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.associate-regions-for-updates]"
    "[rti.service.unassociate-regions-for-updates]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.unsubscribe-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.unsubscribe-object-class-attributes-with-regions]"
    "[federate.callback.attributes-in-scope]"
    "[federate.callback.attributes-out-of-scope]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador evokedReports;
  ReportingFederateAmbassador immediateReports;
  auto owner = makeRti();
  auto evoked = makeRti();
  auto immediate = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(evoked->connect(evokedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(immediate->connect(immediateReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"scope-owner", L"scope-owner", federationName));
  REQUIRE_NOTHROW(evoked->joinFederationExecution(
      L"scope-evoked", L"scope-evoked", federationName));
  REQUIRE_NOTHROW(immediate->joinFederationExecution(
      L"scope-immediate", L"scope-immediate", federationName));
  REQUIRE_FALSE(evoked->getAttributeScopeAdvisorySwitch());
  REQUIRE_FALSE(immediate->getAttributeScopeAdvisorySwitch());
  REQUIRE_NOTHROW(evoked->setAttributeScopeAdvisorySwitch(true));
  REQUIRE_NOTHROW(immediate->setAttributeScopeAdvisorySwitch(true));
  REQUIRE(evoked->getAttributeScopeAdvisorySwitch());
  REQUIRE(immediate->getAttributeScopeAdvisorySwitch());

  auto const soda = owner->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = owner->getAttributeHandle(soda, L"Flavor");
  auto const sodaFlavor = owner->getDimensionHandle(L"SodaFlavor");
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(soda, flavorOnly));

  auto const ownerRegion = owner->createRegion(DimensionHandleSet{sodaFlavor});
  auto const evokedRegion = evoked->createRegion(DimensionHandleSet{sodaFlavor});
  auto const immediateRegion = immediate->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(owner->setRangeBounds(ownerRegion, sodaFlavor, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{ownerRegion}));
  REQUIRE_NOTHROW(evoked->setRangeBounds(evokedRegion, sodaFlavor, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(evoked->commitRegionModifications(RegionHandleSet{evokedRegion}));
  REQUIRE_NOTHROW(
      immediate->setRangeBounds(immediateRegion, sodaFlavor, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(immediate->commitRegionModifications(RegionHandleSet{immediateRegion}));

  AttributeHandleSetRegionHandleSetPairVector const ownerPair{{
      flavorOnly,
      RegionHandleSet{ownerRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const evokedPair{{
      flavorOnly,
      RegionHandleSet{evokedRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const immediatePair{{
      flavorOnly,
      RegionHandleSet{immediateRegion},
  }};
  REQUIRE_NOTHROW(evoked->subscribeObjectClassAttributesWithRegions(soda, evokedPair));
  REQUIRE_NOTHROW(immediate->subscribeObjectClassAttributesWithRegions(soda, immediatePair));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstanceWithRegions(soda, ownerPair));
  REQUIRE(immediateReports.objectDiscoveryReports.size() == 1);
  REQUIRE(evokedReports.objectDiscoveryReports.empty());
  REQUIRE_FALSE(evoked->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(evokedReports.objectDiscoveryReports.size() == 1);
  REQUIRE(evoked->getKnownObjectClassHandle(objectInstance) == soda);
  REQUIRE(immediate->getKnownObjectClassHandle(objectInstance) == soda);
  REQUIRE(evokedReports.attributesInScopeReports.empty());
  REQUIRE(evokedReports.attributesOutOfScopeReports.empty());
  REQUIRE(immediateReports.attributesInScopeReports.empty());
  REQUIRE(immediateReports.attributesOutOfScopeReports.empty());

  // Association changes are also scope transitions for an already-known
  // object. Immediate delivery is synchronous; evoked delivery waits for
  // callback dispatch, just as it does for committed region changes.
  evokedReports.attributesInScopeReports.clear();
  evokedReports.attributesOutOfScopeReports.clear();
  immediateReports.attributesInScopeReports.clear();
  immediateReports.attributesOutOfScopeReports.clear();
  REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(objectInstance, ownerPair));
  REQUIRE(evokedReports.attributesOutOfScopeReports.empty());
  REQUIRE(immediateReports.attributesOutOfScopeReports.size() == 1);
  REQUIRE_FALSE(evoked->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(evokedReports.attributesOutOfScopeReports.size() == 1);
  REQUIRE(evokedReports.attributesOutOfScopeReports.front().objectInstance == objectInstance);
  REQUIRE(evokedReports.attributesOutOfScopeReports.front().attributes == flavorOnly);

  REQUIRE_NOTHROW(owner->associateRegionsForUpdates(objectInstance, ownerPair));
  REQUIRE(immediateReports.attributesInScopeReports.size() == 1);
  REQUIRE_FALSE(evoked->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(evokedReports.attributesInScopeReports.size() == 1);
  REQUIRE(evokedReports.attributesInScopeReports.front().objectInstance == objectInstance);
  REQUIRE(evokedReports.attributesInScopeReports.front().attributes == flavorOnly);
  evokedReports.attributesInScopeReports.clear();
  evokedReports.attributesOutOfScopeReports.clear();
  immediateReports.attributesInScopeReports.clear();
  immediateReports.attributesOutOfScopeReports.clear();

  // A queued association transition is rechecked against the current
  // association map. Removing and restoring the association before evocation
  // suppresses stale out-of-scope work and retains the current in-scope work.
  REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(objectInstance, ownerPair));
  REQUIRE(immediateReports.attributesOutOfScopeReports.size() == 1);
  REQUIRE_NOTHROW(owner->associateRegionsForUpdates(objectInstance, ownerPair));
  REQUIRE(immediateReports.attributesInScopeReports.size() == 1);
  REQUIRE(evoked->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(evoked->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(evokedReports.attributesOutOfScopeReports.empty());
  REQUIRE(evokedReports.attributesInScopeReports.size() == 1);
  REQUIRE(evokedReports.attributesInScopeReports.front().objectInstance == objectInstance);
  REQUIRE(evokedReports.attributesInScopeReports.front().attributes == flavorOnly);
  evokedReports.attributesInScopeReports.clear();
  evokedReports.attributesOutOfScopeReports.clear();
  immediateReports.attributesInScopeReports.clear();
  immediateReports.attributesOutOfScopeReports.clear();

  // Subscription changes are scope transitions for an already-known object.
  // Regional and ordinary declarations remain independent, but each source
  // drives the same official in/out callbacks and callback-time recheck.
  REQUIRE_NOTHROW(evoked->unsubscribeObjectClassAttributesWithRegions(soda, evokedPair));
  REQUIRE(evokedReports.attributesOutOfScopeReports.empty());
  REQUIRE_NOTHROW(immediate->unsubscribeObjectClassAttributesWithRegions(soda, immediatePair));
  REQUIRE(immediateReports.attributesOutOfScopeReports.size() == 1);
  REQUIRE_FALSE(evoked->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(evokedReports.attributesOutOfScopeReports.size() == 1);
  REQUIRE(evokedReports.attributesOutOfScopeReports.front().objectInstance == objectInstance);
  REQUIRE(evokedReports.attributesOutOfScopeReports.front().attributes == flavorOnly);
  evokedReports.attributesInScopeReports.clear();
  evokedReports.attributesOutOfScopeReports.clear();
  immediateReports.attributesInScopeReports.clear();
  immediateReports.attributesOutOfScopeReports.clear();

  REQUIRE_NOTHROW(evoked->subscribeObjectClassAttributesWithRegions(soda, evokedPair));
  REQUIRE_NOTHROW(immediate->subscribeObjectClassAttributesWithRegions(soda, immediatePair));
  REQUIRE(immediateReports.attributesInScopeReports.size() == 1);
  REQUIRE_FALSE(evoked->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(evokedReports.attributesInScopeReports.size() == 1);
  REQUIRE(evokedReports.attributesInScopeReports.front().objectInstance == objectInstance);
  REQUIRE(evokedReports.attributesInScopeReports.front().attributes == flavorOnly);
  evokedReports.attributesInScopeReports.clear();
  evokedReports.attributesOutOfScopeReports.clear();
  immediateReports.attributesInScopeReports.clear();
  immediateReports.attributesOutOfScopeReports.clear();

  // Subscription work is stale-checked too: an evoked out-of-scope
  // transition is suppressed when the same regional subscription is restored
  // before callback dispatch.
  REQUIRE_NOTHROW(evoked->unsubscribeObjectClassAttributesWithRegions(soda, evokedPair));
  REQUIRE_NOTHROW(evoked->subscribeObjectClassAttributesWithRegions(soda, evokedPair));
  REQUIRE(evoked->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(evoked->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(evokedReports.attributesOutOfScopeReports.empty());
  REQUIRE(evokedReports.attributesInScopeReports.size() == 1);
  evokedReports.attributesInScopeReports.clear();
  evokedReports.attributesOutOfScopeReports.clear();

  // Ordinary subscription state is independent from the regional state. The
  // evoked federate crosses out of regional scope, into ordinary scope, back
  // out, and finally into regional scope again.
  REQUIRE_NOTHROW(evoked->unsubscribeObjectClassAttributesWithRegions(soda, evokedPair));
  REQUIRE_FALSE(evoked->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(evokedReports.attributesOutOfScopeReports.size() == 1);
  evokedReports.attributesOutOfScopeReports.clear();
  REQUIRE_NOTHROW(evoked->subscribeObjectClassAttributes(soda, flavorOnly, true, L""));
  REQUIRE_FALSE(evoked->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(evokedReports.attributesInScopeReports.size() == 1);
  evokedReports.attributesInScopeReports.clear();
  REQUIRE_NOTHROW(evoked->unsubscribeObjectClassAttributes(soda, flavorOnly));
  REQUIRE_FALSE(evoked->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(evokedReports.attributesOutOfScopeReports.size() == 1);
  evokedReports.attributesOutOfScopeReports.clear();
  REQUIRE_NOTHROW(evoked->subscribeObjectClassAttributesWithRegions(soda, evokedPair));
  REQUIRE_FALSE(evoked->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(evokedReports.attributesInScopeReports.size() == 1);
  evokedReports.attributesInScopeReports.clear();
  immediateReports.attributesInScopeReports.clear();
  immediateReports.attributesOutOfScopeReports.clear();

  // A committed disjoint subscriber region produces one evoked out-of-scope
  // callback, while the immediate subscriber is entered synchronously.
  REQUIRE_NOTHROW(evoked->setRangeBounds(evokedRegion, sodaFlavor, RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(evoked->commitRegionModifications(RegionHandleSet{evokedRegion}));
  REQUIRE(evokedReports.attributesOutOfScopeReports.empty());
  REQUIRE_FALSE(evoked->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(evokedReports.attributesOutOfScopeReports.size() == 1);
  REQUIRE(evokedReports.attributesOutOfScopeReports.front().objectInstance == objectInstance);
  REQUIRE(evokedReports.attributesOutOfScopeReports.front().attributes == flavorOnly);

  REQUIRE_NOTHROW(
      immediate->setRangeBounds(immediateRegion, sodaFlavor, RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(immediate->commitRegionModifications(RegionHandleSet{immediateRegion}));
  REQUIRE(immediateReports.attributesOutOfScopeReports.size() == 1);
  REQUIRE(immediateReports.attributesOutOfScopeReports.front().objectInstance == objectInstance);
  REQUIRE(immediateReports.attributesOutOfScopeReports.front().attributes == flavorOnly);

  REQUIRE_NOTHROW(evoked->setRangeBounds(evokedRegion, sodaFlavor, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(evoked->commitRegionModifications(RegionHandleSet{evokedRegion}));
  REQUIRE_FALSE(evoked->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(evokedReports.attributesInScopeReports.size() == 1);
  REQUIRE(evokedReports.attributesInScopeReports.front().objectInstance == objectInstance);
  REQUIRE(evokedReports.attributesInScopeReports.front().attributes == flavorOnly);

  REQUIRE_NOTHROW(
      immediate->setRangeBounds(immediateRegion, sodaFlavor, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(immediate->commitRegionModifications(RegionHandleSet{immediateRegion}));
  REQUIRE(immediateReports.attributesInScopeReports.size() == 1);
  REQUIRE(immediateReports.attributesInScopeReports.front().objectInstance == objectInstance);
  REQUIRE(immediateReports.attributesInScopeReports.front().attributes == flavorOnly);

  // Two queued transitions collapse at callback time: the stale out-of-scope
  // work is suppressed after the region has returned to its committed overlap.
  evokedReports.attributesInScopeReports.clear();
  evokedReports.attributesOutOfScopeReports.clear();
  REQUIRE_NOTHROW(evoked->setRangeBounds(evokedRegion, sodaFlavor, RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(evoked->commitRegionModifications(RegionHandleSet{evokedRegion}));
  REQUIRE_NOTHROW(evoked->setRangeBounds(evokedRegion, sodaFlavor, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(evoked->commitRegionModifications(RegionHandleSet{evokedRegion}));
  REQUIRE(evoked->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(evoked->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(evokedReports.attributesOutOfScopeReports.empty());
  REQUIRE(evokedReports.attributesInScopeReports.size() == 1);
  REQUIRE(evokedReports.attributesInScopeReports.front().attributes == flavorOnly);

  // The advisory switch gates notification generation. Re-enabling it does
  // not manufacture a callback until a subsequent committed transition.
  immediateReports.attributesInScopeReports.clear();
  immediateReports.attributesOutOfScopeReports.clear();
  REQUIRE_NOTHROW(immediate->setAttributeScopeAdvisorySwitch(false));
  REQUIRE_NOTHROW(
      immediate->setRangeBounds(immediateRegion, sodaFlavor, RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(immediate->commitRegionModifications(RegionHandleSet{immediateRegion}));
  REQUIRE(immediateReports.attributesOutOfScopeReports.empty());
  REQUIRE_NOTHROW(immediate->setAttributeScopeAdvisorySwitch(true));
  REQUIRE_NOTHROW(
      immediate->setRangeBounds(immediateRegion, sodaFlavor, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(immediate->commitRegionModifications(RegionHandleSet{immediateRegion}));
  REQUIRE(immediateReports.attributesInScopeReports.size() == 1);
  REQUIRE(immediateReports.attributesInScopeReports.front().attributes == flavorOnly);

  REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(objectInstance, ownerPair));
  REQUIRE_NOTHROW(evoked->unsubscribeObjectClassAttributesWithRegions(soda, evokedPair));
  REQUIRE_NOTHROW(immediate->unsubscribeObjectClassAttributesWithRegions(soda, immediatePair));
  REQUIRE_NOTHROW(owner->deleteRegion(ownerRegion));
  REQUIRE_NOTHROW(evoked->deleteRegion(evokedRegion));
  REQUIRE_NOTHROW(immediate->deleteRegion(immediateRegion));
  REQUIRE_NOTHROW(evoked->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(immediate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(evoked->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
}

TEST_CASE(
    "Embedded object-instance Request Attribute Value Update solicits 2025 owners",
    "[integration][development-profile][object-management]"
    "[rti.service.request-attribute-value-update][federate.callback.provide-attribute-value-update]") {
  TestFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0xD7, 0x4E};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  ObjectInstanceHandle invalidObjectInstance;
  AttributeHandleSet noAttributes;

  REQUIRE_THROWS_AS(
      unjoined->requestAttributeValueUpdate(invalidObjectInstance, noAttributes, tag),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->requestAttributeValueUpdate(invalidObjectInstance, noAttributes, tag),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"provide-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(
      requester->joinFederationExecution(L"provide-requester", L"subscriber", federationName));

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableBaseA = owner->getAttributeHandle(child, L"ReliableBaseA");
  auto const reliableChild = owner->getAttributeHandle(child, L"ReliableChild");
  auto const unownedChild = owner->getAttributeHandle(child, L"UnownedChild");
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(reliableChild.isValid());
  REQUIRE(unownedChild.isValid());

  AttributeHandleSet const ownedAttributes{reliableBaseA, reliableChild};
  AttributeHandleSet const requesterSubscriptions{
      reliableBaseA,
      reliableChild,
      unownedChild,
  };
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(child, requesterSubscriptions));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, ownedAttributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE(requesterReports.objectDiscoveryReports.empty());
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE(requester->getKnownObjectClassHandle(objectInstance) == child);

  REQUIRE_THROWS_AS(
      requester->requestAttributeValueUpdate(invalidObjectInstance, ownedAttributes, tag),
      rti1516_2025::ObjectInstanceNotKnown);
  AttributeHandle invalidAttribute;
  AttributeHandleSet const invalidAttributes{invalidAttribute};
  REQUIRE_THROWS_AS(
      requester->requestAttributeValueUpdate(objectInstance, invalidAttributes, tag),
      rti1516_2025::AttributeNotDefined);

  // Defined-but-unowned attributes do not solicit a Provide callback. The
  // requester knows this instance as Child, so the no-callback outcome is not
  // a class-availability failure.
  AttributeHandleSet const unownedAttributes{unownedChild};
  REQUIRE_NOTHROW(
      requester->requestAttributeValueUpdate(objectInstance, unownedAttributes, tag));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.empty());

  // One request with two attributes owned by the same federate induces at
  // most one Provide Attribute Value Update callback at that owner. The
  // requester itself never receives a corresponding callback.
  REQUIRE_NOTHROW(
      requester->requestAttributeValueUpdate(objectInstance, ownedAttributes, tag));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.empty());
  REQUIRE(requesterReports.attributeValueUpdateRequestReports.empty());
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 1);
  auto const& provide = ownerReports.attributeValueUpdateRequestReports.front();
  REQUIRE(provide.objectInstance == objectInstance);
  REQUIRE(provide.attributes == ownedAttributes);
  REQUIRE(variableLengthDataBytes(provide.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(requesterReports.attributeValueUpdateRequestReports.empty());

  // Requesting an attribute already owned by the requester is an implicit
  // local provide and must not call the public provider callback on itself.
  REQUIRE_NOTHROW(owner->requestAttributeValueUpdate(objectInstance, ownedAttributes, tag));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 1);

  // A queued owner callback is rechecked at delivery. Resignation removes the
  // provider's federation route, so stale work cannot invoke user code after
  // the membership transition.
  REQUIRE_NOTHROW(
      requester->requestAttributeValueUpdate(objectInstance, ownedAttributes, tag));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 1);
  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}

TEST_CASE(
    "Embedded object-class Request Attribute Value Update solicits 2025 subclass owners",
    "[integration][development-profile][object-management]"
    "[rti.service.request-attribute-value-update][federate.callback.provide-attribute-value-update]") {
  TestFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0xC1, 0x25};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  ObjectClassHandle invalidObjectClass;
  AttributeHandleSet noAttributes;

  REQUIRE_THROWS_AS(
      unjoined->requestAttributeValueUpdate(invalidObjectClass, noAttributes, tag),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->requestAttributeValueUpdate(invalidObjectClass, noAttributes, tag),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"class-provide-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(
      requester->joinFederationExecution(L"class-provide-requester", L"subscriber", federationName));

  auto const base = owner->getObjectClassHandle(L"HLAobjectRoot.UmbraAttributeFixtureBase");
  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableBaseA = owner->getAttributeHandle(child, L"ReliableBaseA");
  auto const reliableBaseB = owner->getAttributeHandle(child, L"ReliableBaseB");
  auto const reliableChild = owner->getAttributeHandle(child, L"ReliableChild");
  auto const unownedChild = owner->getAttributeHandle(child, L"UnownedChild");
  REQUIRE(base.isValid());
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(reliableBaseB.isValid());
  REQUIRE(reliableChild.isValid());
  REQUIRE(unownedChild.isValid());

  AttributeHandleSet const providerAttributes{reliableBaseA, reliableBaseB, reliableChild};
  AttributeHandleSet const baseAttributes{reliableBaseA, reliableBaseB};
  AttributeHandleSet const childOnlyAttributes{reliableChild};
  AttributeHandleSet const unownedChildAttributes{unownedChild};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, providerAttributes));

  ObjectInstanceHandle objectInstance;
  ObjectInstanceHandle secondObjectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_NOTHROW(secondObjectInstance = owner->registerObjectInstance(child));
  // The class-designator form has no requester-known-instance precondition.
  // This requester has not subscribed, so discovery has not made the child
  // instance known locally before it asks for the base-class attribute.
  REQUIRE(requesterReports.objectDiscoveryReports.empty());
  REQUIRE_THROWS_AS(
      requester->getKnownObjectClassHandle(objectInstance),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      requester->getKnownObjectClassHandle(secondObjectInstance),
      rti1516_2025::ObjectInstanceNotKnown);

  REQUIRE_THROWS_AS(
      requester->requestAttributeValueUpdate(invalidObjectClass, baseAttributes, tag),
      rti1516_2025::ObjectClassNotDefined);
  REQUIRE_THROWS_AS(
      requester->requestAttributeValueUpdate(base, childOnlyAttributes, tag),
      rti1516_2025::AttributeNotDefined);
  REQUIRE_NOTHROW(
      requester->requestAttributeValueUpdate(child, unownedChildAttributes, tag));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.empty());

  // The base-class request expands over every registered Child instance and
  // invokes one owner callback for each particular object instance. Both base
  // attributes stay together in that one callback for each owner/instance.
  REQUIRE_NOTHROW(requester->requestAttributeValueUpdate(base, baseAttributes, tag));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.empty());
  REQUIRE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 1);
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 2);
  auto const& firstProvide = ownerReports.attributeValueUpdateRequestReports[0];
  auto const& secondProvide = ownerReports.attributeValueUpdateRequestReports[1];
  REQUIRE(firstProvide.objectInstance == objectInstance);
  REQUIRE(secondProvide.objectInstance == secondObjectInstance);
  REQUIRE(firstProvide.attributes == baseAttributes);
  REQUIRE(secondProvide.attributes == baseAttributes);
  REQUIRE(variableLengthDataBytes(firstProvide.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(variableLengthDataBytes(secondProvide.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));

  // Requester-owned attributes are an implicit local Provide, including when
  // the request expands from a class to its registered subclass instance.
  REQUIRE_NOTHROW(owner->requestAttributeValueUpdate(base, baseAttributes, tag));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 2);

  // Class expansion is rechecked at delivery just like the instance form.
  // A queued owner callback cannot outlive the provider's resignation.
  REQUIRE_NOTHROW(requester->requestAttributeValueUpdate(base, baseAttributes, tag));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 2);
  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}

TEST_CASE(
    "Embedded regional Request Attribute Value Update filters 2025 owner solicitations",
    "[integration][development-profile][federation-management][ddm]"
    "[rti.service.request-attribute-value-update-with-regions]"
    "[federate.callback.provide-attribute-value-update]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const tagBytes[] = {0x91, 0x25};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"regional-request-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"regional-requester", L"requester", federationName));

  auto const soda = owner->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = owner->getAttributeHandle(soda, L"Flavor");
  auto const sodaFlavor = owner->getDimensionHandle(L"SodaFlavor");
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(soda, flavorOnly));

  auto const ownerRegion = owner->createRegion(DimensionHandleSet{sodaFlavor});
  auto const requestRegion = requester->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(owner->setRangeBounds(ownerRegion, sodaFlavor, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{ownerRegion}));
  REQUIRE_NOTHROW(requester->setRangeBounds(requestRegion, sodaFlavor, RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(requester->commitRegionModifications(RegionHandleSet{requestRegion}));

  AttributeHandleSetRegionHandleSetPairVector const ownerPair{{
      flavorOnly,
      RegionHandleSet{ownerRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const disjointRequestPair{{
      flavorOnly,
      RegionHandleSet{requestRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const emptyRequestPair{{
      flavorOnly,
      RegionHandleSet{},
  }};
  auto const serverId = requester->getDimensionHandle(L"ServerId");
  auto const wrongContextRegion = requester->createRegion(DimensionHandleSet{serverId});
  auto const uncommittedRegion = requester->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(requester->setRangeBounds(
      wrongContextRegion,
      serverId,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(requester->commitRegionModifications(RegionHandleSet{wrongContextRegion}));
  AttributeHandleSetRegionHandleSetPairVector const foreignRequestPair{{
      flavorOnly,
      RegionHandleSet{ownerRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const wrongContextRequestPair{{
      flavorOnly,
      RegionHandleSet{wrongContextRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const uncommittedRequestPair{{
      flavorOnly,
      RegionHandleSet{uncommittedRegion},
  }};

  ObjectInstanceHandle regionalObject;
  ObjectInstanceHandle defaultObject;
  REQUIRE_NOTHROW(regionalObject = owner->registerObjectInstanceWithRegions(soda, ownerPair));
  REQUIRE_NOTHROW(defaultObject = owner->registerObjectInstance(soda));
  REQUIRE(regionalObject.isValid());
  REQUIRE(defaultObject.isValid());

  REQUIRE_THROWS_AS(
      requester->requestAttributeValueUpdateWithRegions(soda, foreignRequestPair, tag),
      rti1516_2025::RegionNotCreatedByThisFederate);
  REQUIRE_THROWS_AS(
      requester->requestAttributeValueUpdateWithRegions(soda, wrongContextRequestPair, tag),
      rti1516_2025::InvalidRegionContext);
  REQUIRE_THROWS_AS(
      requester->requestAttributeValueUpdateWithRegions(soda, uncommittedRequestPair, tag),
      rti1516_2025::InvalidRegion);

  // An empty request-region set is a no-op for that class attribute.
  REQUIRE_NOTHROW(requester->requestAttributeValueUpdateWithRegions(
      soda,
      emptyRequestPair,
      tag));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.empty());

  // The explicit [0,1) update association is disjoint from [2,3), while the
  // ordinary/default-region object remains eligible for a regional request.
  REQUIRE_NOTHROW(requester->requestAttributeValueUpdateWithRegions(
      soda,
      disjointRequestPair,
      tag));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 1);
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.front().objectInstance == defaultObject);
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.front().attributes == flavorOnly);
  REQUIRE(variableLengthDataBytes(
              ownerReports.attributeValueUpdateRequestReports.front().userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));

  // Recommitting the requester region into [0,1) makes both the explicit and
  // default-region instances eligible.
  REQUIRE_NOTHROW(requester->setRangeBounds(requestRegion, sodaFlavor, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(requester->commitRegionModifications(RegionHandleSet{requestRegion}));
  REQUIRE_NOTHROW(requester->requestAttributeValueUpdateWithRegions(
      soda,
      disjointRequestPair,
      tag));
  REQUIRE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 3);

  // Queue the same overlapping request, then move the committed request
  // region away before delivery. The explicit provider callback is suppressed
  // at callback entry while the default-region callback remains eligible.
  REQUIRE_NOTHROW(requester->requestAttributeValueUpdateWithRegions(
      soda,
      disjointRequestPair,
      tag));
  REQUIRE_NOTHROW(requester->setRangeBounds(requestRegion, sodaFlavor, RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(requester->commitRegionModifications(RegionHandleSet{requestRegion}));
  REQUIRE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 4);
  std::set<ObjectInstanceHandle> reportedObjects;
  for (auto const& report : ownerReports.attributeValueUpdateRequestReports) {
    REQUIRE(report.attributes == flavorOnly);
    reportedObjects.insert(report.objectInstance);
  }
  REQUIRE(reportedObjects == std::set<ObjectInstanceHandle>{regionalObject, defaultObject});

  REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(regionalObject, ownerPair));
  REQUIRE_NOTHROW(owner->deleteRegion(ownerRegion));
  REQUIRE_NOTHROW(requester->deleteRegion(wrongContextRegion));
  REQUIRE_NOTHROW(requester->deleteRegion(uncommittedRegion));
  REQUIRE_NOTHROW(requester->deleteRegion(requestRegion));
  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}

TEST_CASE(
    "Embedded Request Attribute Value Update supports a 2025 provider response",
    "[integration][development-profile][object-management]"
    "[rti.service.request-attribute-value-update]"
    "[rti.service.update-attribute-values]"
    "[federate.callback.provide-attribute-value-update]"
    "[federate.callback.reflect-attribute-values]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const requestTagBytes[] = {0xD7, 0x4E};
  unsigned char const responseTagBytes[] = {0x5A, 0x25};
  unsigned char const responseValueBytes[] = {0x42, 0x24, 0x07};
  VariableLengthData const requestTag(requestTagBytes, sizeof(requestTagBytes));
  ObjectInstanceHandle invalidObjectInstance;
  AttributeHandleSet noAttributes;

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"response-provider", L"publisher", federationName));
  REQUIRE_NOTHROW(
      requester->joinFederationExecution(L"response-requester", L"subscriber", federationName));

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableChild = owner->getAttributeHandle(child, L"ReliableChild");
  REQUIRE(child.isValid());
  REQUIRE(reliableChild.isValid());

  AttributeHandleSet const requestedAttributes{reliableChild};
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(child, requestedAttributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, requestedAttributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE(requester->getKnownObjectClassHandle(objectInstance) == child);

  auto const ownerHandle = owner->getFederateHandle(L"response-provider");
  std::vector<unsigned char> observedRequestTag;
  std::vector<unsigned char> responseValue(
      responseValueBytes,
      responseValueBytes + sizeof(responseValueBytes));
  std::vector<unsigned char> responseTag(
      responseTagBytes,
      responseTagBytes + sizeof(responseTagBytes));
  ownerReports.provideAttributeValueUpdateHandler = [
      &owner,
      &observedRequestTag,
      responseValue,
      responseTag](
      ObjectInstanceHandle const& callbackObject,
      AttributeHandleSet const& callbackAttributes,
      VariableLengthData const& callbackTag) {
    observedRequestTag = variableLengthDataBytes(callbackTag);
    AttributeHandleValueMap values;
    for (AttributeHandle const& attribute : callbackAttributes) {
      values.emplace(
          attribute,
          VariableLengthData(responseValue.data(), responseValue.size()));
    }
    VariableLengthData responseUserTag(responseTag.data(), responseTag.size());
    owner->updateAttributeValues(callbackObject, values, responseUserTag);
  };

  REQUIRE_NOTHROW(
      requester->requestAttributeValueUpdate(objectInstance, requestedAttributes, requestTag));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.empty());
  REQUIRE(requesterReports.attributeReflectionReports.empty());

  // The provider callback is the response point: its update is submitted
  // while the official Provide Attribute Value Update callback is executing.
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 1);
  REQUIRE(observedRequestTag ==
          std::vector<unsigned char>(
              requestTagBytes,
              requestTagBytes + sizeof(requestTagBytes)));
  REQUIRE(requesterReports.attributeReflectionReports.empty());

  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeReflectionReports.size() == 1);
  auto const& reflection = requesterReports.attributeReflectionReports.front();
  REQUIRE(reflection.objectInstance == objectInstance);
  REQUIRE(reflection.attributeValues.size() == 1);
  auto const reflected = reflection.attributeValues.find(reliableChild);
  REQUIRE(reflected != reflection.attributeValues.end());
  REQUIRE(variableLengthDataBytes(reflected->second) == responseValue);
  REQUIRE(variableLengthDataBytes(reflection.userSuppliedTag) == responseTag);
  REQUIRE(
      reflection.transportationType == owner->getTransportationTypeHandle(L"HLAreliable"));
  REQUIRE(reflection.producingFederate == ownerHandle);
  REQUIRE_FALSE(reflection.sentRegionsSupplied);

  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}

TEST_CASE(
    "Embedded Query Attribute Ownership reports 2025 federate and unowned attributes",
    "[integration][development-profile][ownership-management]"
    "[rti.service.query-attribute-ownership]"
    "[federate.callback.inform-attribute-ownership]"
    "[federate.callback.attribute-is-not-owned]") {
  TestFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  ObjectInstanceHandle invalidObjectInstance;
  AttributeHandleSet noAttributes;

  REQUIRE_THROWS_AS(
      unjoined->queryAttributeOwnership(invalidObjectInstance, noAttributes),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->queryAttributeOwnership(invalidObjectInstance, noAttributes),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"query-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(
      requester->joinFederationExecution(L"query-requester", L"subscriber", federationName));

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableBaseA = owner->getAttributeHandle(child, L"ReliableBaseA");
  auto const reliableChild = owner->getAttributeHandle(child, L"ReliableChild");
  auto const unownedChild = owner->getAttributeHandle(child, L"UnownedChild");
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(reliableChild.isValid());
  REQUIRE(unownedChild.isValid());

  AttributeHandleSet const ownedAttributes{reliableBaseA, reliableChild};
  AttributeHandleSet const requesterSubscriptions{
      reliableBaseA,
      reliableChild,
      unownedChild,
  };
  AttributeHandleSet const queriedAttributes{reliableBaseA, unownedChild};
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(child, requesterSubscriptions));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, ownedAttributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE(requesterReports.objectDiscoveryReports.empty());
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE(requester->getKnownObjectClassHandle(objectInstance) == child);

  AttributeHandle invalidAttribute;
  AttributeHandleSet const invalidAttributes{invalidAttribute};
  REQUIRE_THROWS_AS(
      requester->queryAttributeOwnership(invalidObjectInstance, queriedAttributes),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      requester->queryAttributeOwnership(objectInstance, invalidAttributes),
      rti1516_2025::AttributeNotDefined);

  // One successful query groups attributes by the standard ownership result:
  // a concrete joined federate is identified through Inform Attribute
  // Ownership, while an available attribute reaches Attribute Is Not Owned.
  REQUIRE_NOTHROW(requester->queryAttributeOwnership(objectInstance, queriedAttributes));
  REQUIRE(requesterReports.attributeOwnershipReports.empty());
  REQUIRE(ownerReports.attributeOwnershipReports.empty());
  REQUIRE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipReports.size() == 2);

  auto const federateReport = std::find_if(
      requesterReports.attributeOwnershipReports.begin(),
      requesterReports.attributeOwnershipReports.end(),
      [](ReportingFederateAmbassador::AttributeOwnershipReport const& report) {
        return report.kind == ReportingFederateAmbassador::AttributeOwnershipReport::Kind::federate;
      });
  auto const unownedReport = std::find_if(
      requesterReports.attributeOwnershipReports.begin(),
      requesterReports.attributeOwnershipReports.end(),
      [](ReportingFederateAmbassador::AttributeOwnershipReport const& report) {
        return report.kind == ReportingFederateAmbassador::AttributeOwnershipReport::Kind::unowned;
      });
  REQUIRE(federateReport != requesterReports.attributeOwnershipReports.end());
  REQUIRE(unownedReport != requesterReports.attributeOwnershipReports.end());
  REQUIRE(federateReport->objectInstance == objectInstance);
  REQUIRE(federateReport->attributes == AttributeHandleSet{reliableBaseA});
  REQUIRE(federateReport->owner == owner->getFederateHandle(L"query-owner"));
  REQUIRE(unownedReport->objectInstance == objectInstance);
  REQUIRE(unownedReport->attributes == AttributeHandleSet{unownedChild});
  REQUIRE(std::none_of(
      requesterReports.attributeOwnershipReports.begin(),
      requesterReports.attributeOwnershipReports.end(),
      [](ReportingFederateAmbassador::AttributeOwnershipReport const& report) {
        return report.kind == ReportingFederateAmbassador::AttributeOwnershipReport::Kind::rti;
      }));

  // Remove Object Instance nullifies reports that were queued by an earlier
  // query. The queued removal still reaches the requester, but no stale
  // ownership result may enter its FederateAmbassador afterward.
  VariableLengthData deletionTag;
  REQUIRE_NOTHROW(requester->queryAttributeOwnership(objectInstance, queriedAttributes));
  REQUIRE_NOTHROW(owner->deleteObjectInstance(objectInstance, deletionTag));
  REQUIRE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipReports.size() == 2);
  REQUIRE(requesterReports.objectRemovalReports.size() == 1);
  REQUIRE(requesterReports.objectRemovalReports.front().objectInstance == objectInstance);

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}

TEST_CASE(
    "Embedded Is Attribute Owned By Federate reads 2025 ownership state",
    "[integration][development-profile][ownership-management]"
    "[rti.service.is-attribute-owned-by-federate]") {
  TestFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  ObjectInstanceHandle invalidObjectInstance;
  AttributeHandle invalidAttribute;

  REQUIRE_THROWS_AS(
      unjoined->isAttributeOwnedByFederate(invalidObjectInstance, invalidAttribute),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->isAttributeOwnedByFederate(invalidObjectInstance, invalidAttribute),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"ownership-check-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(
      requester->joinFederationExecution(L"ownership-check-requester", L"subscriber", federationName));

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableBaseA = owner->getAttributeHandle(child, L"ReliableBaseA");
  auto const unownedChild = owner->getAttributeHandle(child, L"UnownedChild");
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(unownedChild.isValid());

  AttributeHandleSet const ownedAttributes{reliableBaseA};
  AttributeHandleSet const requesterSubscriptions{reliableBaseA, unownedChild};
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(child, requesterSubscriptions));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, ownedAttributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE(requester->getKnownObjectClassHandle(objectInstance) == child);

  // The same shared 2025 ownership snapshot returns true only to its current
  // joined owner. A known remote owner and a defined-but-unowned attribute are
  // both false for the requester without triggering a callback.
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, unownedChild));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, unownedChild));

  REQUIRE_THROWS_AS(
      requester->isAttributeOwnedByFederate(invalidObjectInstance, reliableBaseA),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      requester->isAttributeOwnedByFederate(objectInstance, invalidAttribute),
      rti1516_2025::AttributeNotDefined);

  // Once receive-order removal starts, the instance is no longer queryable
  // through this read-only ownership service before the removal callback runs.
  VariableLengthData deletionTag;
  REQUIRE_NOTHROW(owner->deleteObjectInstance(objectInstance, deletionTag));
  REQUIRE_THROWS_AS(
      requester->isAttributeOwnedByFederate(objectInstance, reliableBaseA),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectRemovalReports.size() == 1);

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}

TEST_CASE(
    "Embedded Attribute Ownership Acquisition If Available resolves 2025 ownership callbacks",
    "[integration][development-profile][ownership-management]"
    "[rti.service.attribute-ownership-acquisition-if-available]"
    "[federate.callback.attribute-ownership-acquisition-notification]"
    "[federate.callback.attribute-ownership-unavailable]") {
  TestFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x51, 0xA7, 0x0C};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  ObjectInstanceHandle invalidObjectInstance;
  AttributeHandle invalidAttribute;
  AttributeHandleSet noAttributes;

  REQUIRE_THROWS_AS(
      unjoined->attributeOwnershipAcquisitionIfAvailable(
          invalidObjectInstance,
          noAttributes,
          tag),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->attributeOwnershipAcquisitionIfAvailable(
          invalidObjectInstance,
          noAttributes,
          tag),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"acquisition-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(
      requester->joinFederationExecution(L"acquisition-requester", L"publisher", federationName));

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableBaseA = owner->getAttributeHandle(child, L"ReliableBaseA");
  auto const reliableChild = owner->getAttributeHandle(child, L"ReliableChild");
  auto const unownedChild = owner->getAttributeHandle(child, L"UnownedChild");
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(reliableChild.isValid());
  REQUIRE(unownedChild.isValid());

  AttributeHandleSet const ownerPublishedAttributes{reliableBaseA};
  AttributeHandleSet const requesterSubscriptions{
      reliableBaseA,
      reliableChild,
      unownedChild,
  };
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(child, requesterSubscriptions));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, ownerPublishedAttributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE(requester->getKnownObjectClassHandle(objectInstance) == child);

  // The requester must publish at its known class before it can enter the
  // 2025 Willing to Acquire state. Publishing only one requested attribute
  // distinguishes class publication from per-attribute publication.
  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisitionIfAvailable(
          objectInstance,
          AttributeHandleSet{unownedChild},
          tag),
      rti1516_2025::ObjectClassNotPublished);
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(
      child,
      AttributeHandleSet{unownedChild}));
  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisitionIfAvailable(
          objectInstance,
          AttributeHandleSet{reliableBaseA},
          tag),
      rti1516_2025::AttributeNotPublished);
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(
      child,
      AttributeHandleSet{reliableBaseA, reliableChild}));

  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisitionIfAvailable(
          invalidObjectInstance,
          AttributeHandleSet{unownedChild},
          tag),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisitionIfAvailable(
          objectInstance,
          AttributeHandleSet{invalidAttribute},
          tag),
      rti1516_2025::AttributeNotDefined);

  AttributeHandleSet const mixedAvailabilityAttributes{unownedChild, reliableBaseA};
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      mixedAvailabilityAttributes,
      tag));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.empty());
  // Ownership transfers only when the matching callback begins. Until then,
  // 2025 requires a repeated If Available request for the same WTA attributes
  // to leave them unchanged, not fail or queue another terminal callback.
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, unownedChild));
  REQUIRE_NOTHROW(
      requester->attributeOwnershipAcquisitionIfAvailable(
          objectInstance,
          AttributeHandleSet{unownedChild},
          tag));
  // A mixed repeat preserves the already-pending attribute and independently
  // admits the new eligible attribute into Willing to Acquire.
  REQUIRE_NOTHROW(
      requester->attributeOwnershipAcquisitionIfAvailable(
          objectInstance,
          AttributeHandleSet{unownedChild, reliableChild},
          tag));
  // The coupled declaration-management precondition prevents the requester
  // from withdrawing a publication that the pending 7.9 request still needs.
  REQUIRE_THROWS_AS(
      requester->unpublishObjectClassAttributes(child, AttributeHandleSet{unownedChild}),
      rti1516_2025::OwnershipAcquisitionPending);

  REQUIRE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.size() == 3);
  auto const notification = std::find_if(
      requesterReports.attributeOwnershipAcquisitionReports.begin(),
      requesterReports.attributeOwnershipAcquisitionReports.end(),
      [](ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport const& report) {
        return report.kind ==
            ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification;
      });
  auto const unavailable = std::find_if(
      requesterReports.attributeOwnershipAcquisitionReports.begin(),
      requesterReports.attributeOwnershipAcquisitionReports.end(),
      [](ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport const& report) {
        return report.kind ==
            ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::unavailable;
      });
  REQUIRE(notification != requesterReports.attributeOwnershipAcquisitionReports.end());
  REQUIRE(unavailable != requesterReports.attributeOwnershipAcquisitionReports.end());
  REQUIRE(notification->objectInstance == objectInstance);
  REQUIRE(notification->attributes == AttributeHandleSet{unownedChild});
  REQUIRE(variableLengthDataBytes(notification->userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(unavailable->objectInstance == objectInstance);
  REQUIRE(unavailable->attributes == AttributeHandleSet{reliableBaseA});
  REQUIRE(variableLengthDataBytes(unavailable->userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  auto const additionalNotification = std::find_if(
      requesterReports.attributeOwnershipAcquisitionReports.begin(),
      requesterReports.attributeOwnershipAcquisitionReports.end(),
      [&reliableChild](
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport const& report) {
        return report.kind ==
                   ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification &&
               report.attributes == AttributeHandleSet{reliableChild};
      });
  REQUIRE(additionalNotification != requesterReports.attributeOwnershipAcquisitionReports.end());
  REQUIRE(additionalNotification->objectInstance == objectInstance);
  REQUIRE(requester->isAttributeOwnedByFederate(objectInstance, unownedChild));
  REQUIRE(requester->isAttributeOwnedByFederate(objectInstance, reliableChild));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisitionIfAvailable(
          objectInstance,
          AttributeHandleSet{unownedChild},
          tag),
      rti1516_2025::FederateOwnsAttributes);
  REQUIRE_NOTHROW(
      requester->unpublishObjectClassAttributes(child, AttributeHandleSet{unownedChild}));

  // Remove Object Instance nullifies a queued If Available callback before it
  // can establish ownership of a deleted instance. Its queued removal still
  // reaches the requester afterward.
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      AttributeHandleSet{reliableBaseA},
      tag));
  VariableLengthData deletionTag;
  REQUIRE_NOTHROW(owner->deleteObjectInstance(objectInstance, deletionTag));
  REQUIRE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.size() == 3);
  REQUIRE(requesterReports.objectRemovalReports.size() == 1);
  REQUIRE(requesterReports.objectRemovalReports.front().objectInstance == objectInstance);
  REQUIRE(ownerReports.attributeOwnershipAcquisitionReports.empty());

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}

TEST_CASE(
    "Embedded Attribute Ownership Acquisition honors 2025 release and denial callbacks",
    "[integration][development-profile][ownership-management]"
    "[rti.service.attribute-ownership-acquisition]"
    "[federate.callback.request-attribute-ownership-release]"
    "[rti.service.attribute-ownership-release-denied]"
    "[federate.callback.attribute-ownership-acquisition-notification]"
    "[federate.callback.attribute-ownership-unavailable]") {
  TestFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const willingToAcquireTagBytes[] = {0x19, 0x53};
  unsigned char const acquisitionTagBytes[] = {0xA5, 0x70, 0xE1};
  unsigned char const denialTagBytes[] = {0xD3, 0x1A, 0x1E, 0xD0};
  VariableLengthData const willingToAcquireTag(
      willingToAcquireTagBytes,
      sizeof(willingToAcquireTagBytes));
  VariableLengthData const acquisitionTag(acquisitionTagBytes, sizeof(acquisitionTagBytes));
  VariableLengthData const denialTag(denialTagBytes, sizeof(denialTagBytes));
  ObjectInstanceHandle invalidObjectInstance;
  AttributeHandle invalidAttribute;
  AttributeHandleSet noAttributes;

  REQUIRE_THROWS_AS(
      unjoined->attributeOwnershipAcquisition(
          invalidObjectInstance,
          noAttributes,
          acquisitionTag),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->attributeOwnershipAcquisition(
          invalidObjectInstance,
          noAttributes,
          acquisitionTag),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"regular-acquisition-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"regular-acquisition-requester",
      L"publisher",
      federationName));

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableBaseA = owner->getAttributeHandle(child, L"ReliableBaseA");
  auto const unownedChild = owner->getAttributeHandle(child, L"UnownedChild");
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(unownedChild.isValid());

  AttributeHandleSet const requesterAttributes{reliableBaseA, unownedChild};
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(child, requesterAttributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, AttributeHandleSet{reliableBaseA}));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE(requester->getKnownObjectClassHandle(objectInstance) == child);

  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisition(
          objectInstance,
          AttributeHandleSet{unownedChild},
          acquisitionTag),
      rti1516_2025::ObjectClassNotPublished);
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(child, requesterAttributes));
  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisition(
          invalidObjectInstance,
          AttributeHandleSet{unownedChild},
          acquisitionTag),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisition(
          objectInstance,
          AttributeHandleSet{invalidAttribute},
          acquisitionTag),
      rti1516_2025::AttributeNotDefined);
  REQUIRE_THROWS_AS(
      owner->attributeOwnershipReleaseDenied(
          objectInstance,
          AttributeHandleSet{unownedChild},
          denialTag),
      rti1516_2025::AttributeNotOwned);

  // The regular service overrides this requester's still-pending WTA state.
  // The earlier queued If Available work therefore becomes a no-delivery
  // callback rather than reporting the remote-owned attribute unavailable.
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      AttributeHandleSet{reliableBaseA},
      willingToAcquireTag));
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance,
      requesterAttributes,
      acquisitionTag));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, unownedChild));
  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisitionIfAvailable(
          objectInstance,
          AttributeHandleSet{reliableBaseA},
          willingToAcquireTag),
      rti1516_2025::AttributeAlreadyBeingAcquired);
  // A repeated regular request preserves the Acquisition Pending state and
  // must not cause a duplicate Request Attribute Ownership Release callback.
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance,
      AttributeHandleSet{reliableBaseA},
      acquisitionTag));
  REQUIRE_THROWS_AS(
      requester->unpublishObjectClassAttributes(child, AttributeHandleSet{unownedChild}),
      rti1516_2025::OwnershipAcquisitionPending);
  REQUIRE_THROWS_AS(
      requester->unpublishObjectClassAttributes(child, AttributeHandleSet{reliableBaseA}),
      rti1516_2025::OwnershipAcquisitionPending);

  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.size() == 1);
  auto const& releaseRequest = ownerReports.attributeOwnershipReleaseRequestReports.front();
  REQUIRE(releaseRequest.objectInstance == objectInstance);
  REQUIRE(releaseRequest.attributes == AttributeHandleSet{reliableBaseA});
  REQUIRE(variableLengthDataBytes(releaseRequest.userSuppliedTag) ==
          std::vector<unsigned char>(acquisitionTagBytes,
                                     acquisitionTagBytes + sizeof(acquisitionTagBytes)));

  // The WTA work was queued first and is now stale; the second callback is
  // the regular unowned-acquisition notification.
  REQUIRE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.size() == 1);
  auto const& notification = requesterReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(notification.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
  REQUIRE(notification.objectInstance == objectInstance);
  REQUIRE(notification.attributes == AttributeHandleSet{unownedChild});
  REQUIRE(variableLengthDataBytes(notification.userSuppliedTag) ==
          std::vector<unsigned char>(acquisitionTagBytes,
                                     acquisitionTagBytes + sizeof(acquisitionTagBytes)));
  REQUIRE(requester->isAttributeOwnedByFederate(objectInstance, unownedChild));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_NOTHROW(requester->unpublishObjectClassAttributes(
      child,
      AttributeHandleSet{unownedChild}));
  REQUIRE_THROWS_AS(
      requester->unpublishObjectClassAttributes(child, AttributeHandleSet{reliableBaseA}),
      rti1516_2025::OwnershipAcquisitionPending);

  // Release Denied preserves the owner's attribute ownership, ends every
  // matching regular acquisition, and supplies its own tag to Unavailable.
  REQUIRE_NOTHROW(owner->attributeOwnershipReleaseDenied(
      objectInstance,
      AttributeHandleSet{reliableBaseA},
      denialTag));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.size() == 2);
  auto const& unavailable = requesterReports.attributeOwnershipAcquisitionReports.back();
  REQUIRE(unavailable.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::unavailable);
  REQUIRE(unavailable.objectInstance == objectInstance);
  REQUIRE(unavailable.attributes == AttributeHandleSet{reliableBaseA});
  REQUIRE(variableLengthDataBytes(unavailable.userSuppliedTag) ==
          std::vector<unsigned char>(denialTagBytes, denialTagBytes + sizeof(denialTagBytes)));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_NOTHROW(requester->unpublishObjectClassAttributes(
      child,
      AttributeHandleSet{reliableBaseA}));
  REQUIRE_NOTHROW(owner->attributeOwnershipReleaseDenied(
      objectInstance,
      AttributeHandleSet{reliableBaseA},
      denialTag));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));

  // Receive-order removal invalidates a queued regular owner-release
  // callback before it can enter user code or create a terminal report.
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(
      child,
      AttributeHandleSet{reliableBaseA}));
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance,
      AttributeHandleSet{reliableBaseA},
      acquisitionTag));
  VariableLengthData deletionTag;
  REQUIRE_NOTHROW(owner->deleteObjectInstance(objectInstance, deletionTag));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.size() == 1);
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.size() == 2);
  REQUIRE(requesterReports.objectRemovalReports.size() == 1);

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}

TEST_CASE(
    "Embedded Attribute Ownership Release Denied reaches all 2025 regular acquirers",
    "[integration][development-profile][ownership-management]"
    "[rti.service.attribute-ownership-acquisition]"
    "[federate.callback.request-attribute-ownership-release]"
    "[rti.service.attribute-ownership-release-denied]"
    "[federate.callback.attribute-ownership-unavailable]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador firstRequesterReports;
  ReportingFederateAmbassador secondRequesterReports;
  auto owner = makeRti();
  auto firstRequester = makeRti();
  auto secondRequester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const firstAcquisitionTagBytes[] = {0x01, 0x02, 0x03};
  unsigned char const secondAcquisitionTagBytes[] = {0x04, 0x05, 0x06};
  unsigned char const denialTagBytes[] = {0xD3, 0x1E, 0xD0};
  VariableLengthData const firstAcquisitionTag(
      firstAcquisitionTagBytes,
      sizeof(firstAcquisitionTagBytes));
  VariableLengthData const secondAcquisitionTag(
      secondAcquisitionTagBytes,
      sizeof(secondAcquisitionTagBytes));
  VariableLengthData const denialTag(denialTagBytes, sizeof(denialTagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(firstRequester->connect(firstRequesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(secondRequester->connect(secondRequesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"denial-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(firstRequester->joinFederationExecution(
      L"denial-first-requester",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(secondRequester->joinFederationExecution(
      L"denial-second-requester",
      L"publisher",
      federationName));

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableBaseA = owner->getAttributeHandle(child, L"ReliableBaseA");
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  AttributeHandleSet const ownedAttributes{reliableBaseA};
  REQUIRE_NOTHROW(firstRequester->subscribeObjectClassAttributes(child, ownedAttributes));
  REQUIRE_NOTHROW(secondRequester->subscribeObjectClassAttributes(child, ownedAttributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, ownedAttributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(firstRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(secondRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(firstRequesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE(secondRequesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE_NOTHROW(firstRequester->publishObjectClassAttributes(child, ownedAttributes));
  REQUIRE_NOTHROW(secondRequester->publishObjectClassAttributes(child, ownedAttributes));

  REQUIRE_NOTHROW(firstRequester->attributeOwnershipAcquisition(
      objectInstance,
      ownedAttributes,
      firstAcquisitionTag));
  REQUIRE_NOTHROW(secondRequester->attributeOwnershipAcquisition(
      objectInstance,
      ownedAttributes,
      secondAcquisitionTag));
  REQUIRE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.size() == 2);
  auto const firstRelease = std::find_if(
      ownerReports.attributeOwnershipReleaseRequestReports.begin(),
      ownerReports.attributeOwnershipReleaseRequestReports.end(),
      [&firstAcquisitionTagBytes](
          ReportingFederateAmbassador::AttributeOwnershipReleaseRequestReport const& report) {
        return variableLengthDataBytes(report.userSuppliedTag) ==
            std::vector<unsigned char>(
                firstAcquisitionTagBytes,
                firstAcquisitionTagBytes + sizeof(firstAcquisitionTagBytes));
      });
  auto const secondRelease = std::find_if(
      ownerReports.attributeOwnershipReleaseRequestReports.begin(),
      ownerReports.attributeOwnershipReleaseRequestReports.end(),
      [&secondAcquisitionTagBytes](
          ReportingFederateAmbassador::AttributeOwnershipReleaseRequestReport const& report) {
        return variableLengthDataBytes(report.userSuppliedTag) ==
            std::vector<unsigned char>(
                secondAcquisitionTagBytes,
                secondAcquisitionTagBytes + sizeof(secondAcquisitionTagBytes));
      });
  REQUIRE(firstRelease != ownerReports.attributeOwnershipReleaseRequestReports.end());
  REQUIRE(secondRelease != ownerReports.attributeOwnershipReleaseRequestReports.end());
  REQUIRE(firstRelease->objectInstance == objectInstance);
  REQUIRE(secondRelease->objectInstance == objectInstance);
  REQUIRE(firstRelease->attributes == ownedAttributes);
  REQUIRE(secondRelease->attributes == ownedAttributes);

  REQUIRE_NOTHROW(owner->attributeOwnershipReleaseDenied(
      objectInstance,
      ownedAttributes,
      denialTag));
  REQUIRE_FALSE(firstRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(secondRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(firstRequesterReports.attributeOwnershipAcquisitionReports.size() == 1);
  REQUIRE(secondRequesterReports.attributeOwnershipAcquisitionReports.size() == 1);
  auto const& firstUnavailable = firstRequesterReports.attributeOwnershipAcquisitionReports.front();
  auto const& secondUnavailable = secondRequesterReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(firstUnavailable.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::unavailable);
  REQUIRE(secondUnavailable.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::unavailable);
  REQUIRE(firstUnavailable.attributes == ownedAttributes);
  REQUIRE(secondUnavailable.attributes == ownedAttributes);
  REQUIRE(variableLengthDataBytes(firstUnavailable.userSuppliedTag) ==
          std::vector<unsigned char>(denialTagBytes, denialTagBytes + sizeof(denialTagBytes)));
  REQUIRE(variableLengthDataBytes(secondUnavailable.userSuppliedTag) ==
          std::vector<unsigned char>(denialTagBytes, denialTagBytes + sizeof(denialTagBytes)));
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_FALSE(firstRequester->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_FALSE(secondRequester->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_NOTHROW(firstRequester->unpublishObjectClassAttributes(child, ownedAttributes));
  REQUIRE_NOTHROW(secondRequester->unpublishObjectClassAttributes(child, ownedAttributes));

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(firstRequester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(secondRequester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(firstRequester->disconnect());
  REQUIRE_NOTHROW(secondRequester->disconnect());
}

TEST_CASE(
    "Embedded Unconditional Attribute Ownership Divestiture offers eligible 2025 federates",
    "[integration][development-profile][ownership-management]"
    "[rti.service.unconditional-attribute-ownership-divestiture]"
    "[rti.service.attribute-ownership-acquisition]"
    "[rti.service.attribute-ownership-acquisition-if-available]"
    "[federate.callback.request-attribute-ownership-assumption]"
    "[federate.callback.attribute-ownership-acquisition-notification]") {
  TestFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador regularRequesterReports;
  ReportingFederateAmbassador ifAvailableRequesterReports;
  ReportingFederateAmbassador invitedCandidateReports;
  ReportingFederateAmbassador staleCandidateReports;
  ReportingFederateAmbassador unpublishedCandidateReports;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto regularRequester = makeRti();
  auto ifAvailableRequester = makeRti();
  auto invitedCandidate = makeRti();
  auto staleCandidate = makeRti();
  auto unpublishedCandidate = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const regularAcquisitionTagBytes[] = {0xA2, 0x07, 0x20};
  unsigned char const pendingIfAvailableTagBytes[] = {0xB2, 0x07, 0x20};
  unsigned char const assumptionTagBytes[] = {0xD2, 0x07, 0x20};
  unsigned char const ifAvailableAcquisitionTagBytes[] = {0xC2, 0x07, 0x20};
  VariableLengthData const regularAcquisitionTag(
      regularAcquisitionTagBytes,
      sizeof(regularAcquisitionTagBytes));
  VariableLengthData const pendingIfAvailableTag(
      pendingIfAvailableTagBytes,
      sizeof(pendingIfAvailableTagBytes));
  VariableLengthData const assumptionTag(assumptionTagBytes, sizeof(assumptionTagBytes));
  VariableLengthData const ifAvailableAcquisitionTag(
      ifAvailableAcquisitionTagBytes,
      sizeof(ifAvailableAcquisitionTagBytes));
  ObjectInstanceHandle invalidObjectInstance;
  AttributeHandle invalidAttribute;
  AttributeHandleSet noAttributes;

  REQUIRE_THROWS_AS(
      unjoined->unconditionalAttributeOwnershipDivestiture(
          invalidObjectInstance,
          noAttributes,
          assumptionTag),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->unconditionalAttributeOwnershipDivestiture(
          invalidObjectInstance,
          noAttributes,
          assumptionTag),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regularRequester->connect(regularRequesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(ifAvailableRequester->connect(ifAvailableRequesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(invitedCandidate->connect(invitedCandidateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(staleCandidate->connect(staleCandidateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(unpublishedCandidate->connect(unpublishedCandidateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"unconditional-divestiture-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(regularRequester->joinFederationExecution(
      L"unconditional-divestiture-regular-requester",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(ifAvailableRequester->joinFederationExecution(
      L"unconditional-divestiture-if-available-requester",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(invitedCandidate->joinFederationExecution(
      L"unconditional-divestiture-invited-candidate",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(staleCandidate->joinFederationExecution(
      L"unconditional-divestiture-stale-candidate",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(unpublishedCandidate->joinFederationExecution(
      L"unconditional-divestiture-unpublished-candidate",
      L"publisher",
      federationName));

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableBaseA = owner->getAttributeHandle(child, L"ReliableBaseA");
  auto const reliableBaseB = owner->getAttributeHandle(child, L"ReliableBaseB");
  auto const reliableChild = owner->getAttributeHandle(child, L"ReliableChild");
  auto const unownedChild = owner->getAttributeHandle(child, L"UnownedChild");
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(reliableBaseB.isValid());
  REQUIRE(reliableChild.isValid());
  REQUIRE(unownedChild.isValid());
  AttributeHandleSet const regularAttributes{reliableBaseA};
  AttributeHandleSet const ifAvailableAttributes{reliableBaseB};
  AttributeHandleSet const candidateAttributes{reliableChild, unownedChild};
  AttributeHandleSet const ownedAttributes{
      reliableBaseA,
      reliableBaseB,
      reliableChild,
      unownedChild,
  };

  // Discovery gives every eventual candidate a known class. Only the two
  // selected candidates publish the offered attributes; a known but
  // unpublished federate must not receive the §7.4 callback.
  REQUIRE_NOTHROW(regularRequester->subscribeObjectClassAttributes(child, regularAttributes));
  REQUIRE_NOTHROW(
      ifAvailableRequester->subscribeObjectClassAttributes(child, ifAvailableAttributes));
  REQUIRE_NOTHROW(invitedCandidate->subscribeObjectClassAttributes(child, candidateAttributes));
  REQUIRE_NOTHROW(staleCandidate->subscribeObjectClassAttributes(child, candidateAttributes));
  REQUIRE_NOTHROW(
      unpublishedCandidate->subscribeObjectClassAttributes(child, candidateAttributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, ownedAttributes));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(regularRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(ifAvailableRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(invitedCandidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(staleCandidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(unpublishedCandidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(regularRequesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE(ifAvailableRequesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE(invitedCandidateReports.objectDiscoveryReports.size() == 1);
  REQUIRE(staleCandidateReports.objectDiscoveryReports.size() == 1);
  REQUIRE(unpublishedCandidateReports.objectDiscoveryReports.size() == 1);
  REQUIRE_NOTHROW(regularRequester->publishObjectClassAttributes(child, regularAttributes));
  REQUIRE_NOTHROW(
      ifAvailableRequester->publishObjectClassAttributes(child, ifAvailableAttributes));
  REQUIRE_NOTHROW(invitedCandidate->publishObjectClassAttributes(child, candidateAttributes));
  REQUIRE_NOTHROW(staleCandidate->publishObjectClassAttributes(child, candidateAttributes));

  REQUIRE_THROWS_AS(
      owner->unconditionalAttributeOwnershipDivestiture(
          invalidObjectInstance,
          ownedAttributes,
          assumptionTag),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      owner->unconditionalAttributeOwnershipDivestiture(
          objectInstance,
          AttributeHandleSet{invalidAttribute},
          assumptionTag),
      rti1516_2025::AttributeNotDefined);
  REQUIRE_THROWS_AS(
      owner->unconditionalAttributeOwnershipDivestiture(
          objectInstance,
          AttributeHandleSet{reliableBaseA, invalidAttribute},
          assumptionTag),
      rti1516_2025::AttributeNotDefined);
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_THROWS_AS(
      regularRequester->unconditionalAttributeOwnershipDivestiture(
          objectInstance,
          regularAttributes,
          assumptionTag),
      rti1516_2025::AttributeNotOwned);

  // A regular acquirer is not offered the same attribute. Its existing
  // Acquisition Pending request instead becomes regular notification work
  // when unconditional divestiture makes the attribute unowned.
  REQUIRE_NOTHROW(regularRequester->attributeOwnershipAcquisition(
      objectInstance,
      regularAttributes,
      regularAcquisitionTag));
  REQUIRE_NOTHROW(ifAvailableRequester->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      ifAvailableAttributes,
      pendingIfAvailableTag));
  REQUIRE_NOTHROW(owner->unconditionalAttributeOwnershipDivestiture(
      objectInstance,
      ownedAttributes,
      assumptionTag));
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, reliableBaseB));
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, reliableChild));
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, unownedChild));
  REQUIRE_FALSE(regularRequester->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_FALSE(ifAvailableRequester->isAttributeOwnedByFederate(objectInstance, reliableBaseB));
  REQUIRE_FALSE(invitedCandidate->isAttributeOwnedByFederate(objectInstance, reliableChild));

  // The old owner-side release work is stale. A candidate that stops
  // publishing before its queued callback also receives no stale offer.
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());
  REQUIRE_NOTHROW(staleCandidate->unpublishObjectClassAttributes(child, candidateAttributes));
  REQUIRE_FALSE(staleCandidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(staleCandidateReports.attributeOwnershipAssumptionReports.empty());
  REQUIRE_FALSE(unpublishedCandidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(unpublishedCandidateReports.attributeOwnershipAssumptionReports.empty());

  // A pre-existing If Available request keeps its own terminal callback and
  // is not duplicated as an assumption offer. It establishes ownership only
  // when that original callback begins after the unconditional transition.
  REQUIRE_FALSE(ifAvailableRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ifAvailableRequesterReports.attributeOwnershipAssumptionReports.empty());
  REQUIRE(ifAvailableRequesterReports.attributeOwnershipAcquisitionReports.size() == 1);
  auto const& ifAvailableNotification =
      ifAvailableRequesterReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(ifAvailableNotification.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
  REQUIRE(ifAvailableNotification.objectInstance == objectInstance);
  REQUIRE(ifAvailableNotification.attributes == ifAvailableAttributes);
  REQUIRE(variableLengthDataBytes(ifAvailableNotification.userSuppliedTag) ==
          std::vector<unsigned char>(
              pendingIfAvailableTagBytes,
              pendingIfAvailableTagBytes + sizeof(pendingIfAvailableTagBytes)));
  REQUIRE(ifAvailableRequester->isAttributeOwnedByFederate(objectInstance, reliableBaseB));
  REQUIRE_NOTHROW(
      ifAvailableRequester->unpublishObjectClassAttributes(child, ifAvailableAttributes));

  REQUIRE_FALSE(regularRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(regularRequesterReports.attributeOwnershipAcquisitionReports.size() == 1);
  auto const& regularNotification =
      regularRequesterReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(regularNotification.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
  REQUIRE(regularNotification.objectInstance == objectInstance);
  REQUIRE(regularNotification.attributes == regularAttributes);
  REQUIRE(variableLengthDataBytes(regularNotification.userSuppliedTag) ==
          std::vector<unsigned char>(
              regularAcquisitionTagBytes,
              regularAcquisitionTagBytes + sizeof(regularAcquisitionTagBytes)));
  REQUIRE(regularRequester->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_NOTHROW(
      regularRequester->unpublishObjectClassAttributes(child, regularAttributes));

  // The still-eligible, non-pending candidate receives a single grouped offer
  // with the unconditional-divestiture tag. The offer changes no ownership;
  // only its later acquisition request can do so.
  REQUIRE_FALSE(invitedCandidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(invitedCandidateReports.attributeOwnershipAssumptionReports.size() == 1);
  auto const& assumption = invitedCandidateReports.attributeOwnershipAssumptionReports.front();
  REQUIRE(assumption.objectInstance == objectInstance);
  REQUIRE(assumption.attributes == candidateAttributes);
  REQUIRE(variableLengthDataBytes(assumption.userSuppliedTag) ==
          std::vector<unsigned char>(
              assumptionTagBytes,
              assumptionTagBytes + sizeof(assumptionTagBytes)));
  REQUIRE_FALSE(invitedCandidate->isAttributeOwnedByFederate(objectInstance, reliableChild));
  REQUIRE_FALSE(invitedCandidate->isAttributeOwnedByFederate(objectInstance, unownedChild));

  REQUIRE_NOTHROW(invitedCandidate->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      candidateAttributes,
      ifAvailableAcquisitionTag));
  REQUIRE_FALSE(invitedCandidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(invitedCandidateReports.attributeOwnershipAcquisitionReports.size() == 1);
  auto const& candidateNotification =
      invitedCandidateReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(candidateNotification.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
  REQUIRE(candidateNotification.objectInstance == objectInstance);
  REQUIRE(candidateNotification.attributes == candidateAttributes);
  REQUIRE(variableLengthDataBytes(candidateNotification.userSuppliedTag) ==
          std::vector<unsigned char>(
              ifAvailableAcquisitionTagBytes,
              ifAvailableAcquisitionTagBytes + sizeof(ifAvailableAcquisitionTagBytes)));
  REQUIRE(invitedCandidate->isAttributeOwnedByFederate(objectInstance, reliableChild));
  REQUIRE(invitedCandidate->isAttributeOwnedByFederate(objectInstance, unownedChild));

  REQUIRE_NOTHROW(owner->unpublishObjectClassAttributes(child, ownedAttributes));
  REQUIRE_NOTHROW(invitedCandidate->unpublishObjectClassAttributes(child, candidateAttributes));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(regularRequester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(ifAvailableRequester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(invitedCandidate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(staleCandidate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(unpublishedCandidate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(regularRequester->disconnect());
  REQUIRE_NOTHROW(ifAvailableRequester->disconnect());
  REQUIRE_NOTHROW(invitedCandidate->disconnect());
  REQUIRE_NOTHROW(staleCandidate->disconnect());
  REQUIRE_NOTHROW(unpublishedCandidate->disconnect());
}

TEST_CASE(
    "Embedded Attribute Ownership Divestiture If Wanted transfers only to 2025 pending acquirers",
    "[integration][development-profile][ownership-management]"
    "[rti.service.attribute-ownership-divestiture-if-wanted]"
    "[rti.service.attribute-ownership-acquisition]"
    "[rti.service.attribute-ownership-acquisition-if-available]"
    "[federate.callback.attribute-ownership-acquisition-notification]") {
  TestFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador regularRequesterReports;
  ReportingFederateAmbassador ifAvailableRequesterReports;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto regularRequester = makeRti();
  auto ifAvailableRequester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const regularAcquisitionTagBytes[] = {0xA1, 0x01, 0x25};
  unsigned char const ifAvailableAcquisitionTagBytes[] = {0xA2, 0x02, 0x25};
  unsigned char const divestitureTagBytes[] = {0xD1, 0x56, 0x25};
  VariableLengthData const regularAcquisitionTag(
      regularAcquisitionTagBytes,
      sizeof(regularAcquisitionTagBytes));
  VariableLengthData const ifAvailableAcquisitionTag(
      ifAvailableAcquisitionTagBytes,
      sizeof(ifAvailableAcquisitionTagBytes));
  VariableLengthData const divestitureTag(divestitureTagBytes, sizeof(divestitureTagBytes));
  ObjectInstanceHandle invalidObjectInstance;
  AttributeHandle invalidAttribute;
  AttributeHandleSet noAttributes;

  REQUIRE_THROWS_AS(
      unjoined->attributeOwnershipDivestitureIfWanted(
          invalidObjectInstance,
          noAttributes,
          divestitureTag,
          noAttributes),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->attributeOwnershipDivestitureIfWanted(
          invalidObjectInstance,
          noAttributes,
          divestitureTag,
          noAttributes),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regularRequester->connect(regularRequesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(ifAvailableRequester->connect(ifAvailableRequesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"divestiture-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(regularRequester->joinFederationExecution(
      L"divestiture-regular-requester",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(ifAvailableRequester->joinFederationExecution(
      L"divestiture-if-available-requester",
      L"publisher",
      federationName));

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableBaseA = owner->getAttributeHandle(child, L"ReliableBaseA");
  auto const reliableChild = owner->getAttributeHandle(child, L"ReliableChild");
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(reliableChild.isValid());
  AttributeHandleSet const regularAttributes{reliableBaseA};
  AttributeHandleSet const ifAvailableAttributes{reliableChild};
  AttributeHandleSet const ownedAttributes{reliableBaseA, reliableChild};

  REQUIRE_NOTHROW(regularRequester->subscribeObjectClassAttributes(child, regularAttributes));
  REQUIRE_NOTHROW(
      ifAvailableRequester->subscribeObjectClassAttributes(child, ifAvailableAttributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, ownedAttributes));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(regularRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(ifAvailableRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(regularRequesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE(ifAvailableRequesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE_NOTHROW(regularRequester->publishObjectClassAttributes(child, regularAttributes));
  REQUIRE_NOTHROW(
      ifAvailableRequester->publishObjectClassAttributes(child, ifAvailableAttributes));

  AttributeHandleSet divestedAttributes{reliableBaseA};
  REQUIRE_THROWS_AS(
      owner->attributeOwnershipDivestitureIfWanted(
          invalidObjectInstance,
          regularAttributes,
          divestitureTag,
          divestedAttributes),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      owner->attributeOwnershipDivestitureIfWanted(
          objectInstance,
          AttributeHandleSet{invalidAttribute},
          divestitureTag,
          divestedAttributes),
      rti1516_2025::AttributeNotDefined);
  REQUIRE_THROWS_AS(
      regularRequester->attributeOwnershipDivestitureIfWanted(
          objectInstance,
          regularAttributes,
          divestitureTag,
          divestedAttributes),
      rti1516_2025::AttributeNotOwned);

  // Without a pending acquirer, the service succeeds but returns an empty
  // result and retains the current owner. The output set is replaced rather
  // than appended to the caller's old contents.
  REQUIRE_NOTHROW(owner->attributeOwnershipDivestitureIfWanted(
      objectInstance,
      regularAttributes,
      divestitureTag,
      divestedAttributes));
  REQUIRE(divestedAttributes.empty());
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, reliableBaseA));

  REQUIRE_NOTHROW(regularRequester->attributeOwnershipAcquisition(
      objectInstance,
      regularAttributes,
      regularAcquisitionTag));
  REQUIRE_NOTHROW(ifAvailableRequester->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      ifAvailableAttributes,
      ifAvailableAcquisitionTag));

  // The owner accepts both forms as genuine pending acquirers. The regular
  // request's already queued owner-release callback is invalidated by the
  // synchronous ownership transfer.
  divestedAttributes = AttributeHandleSet{reliableBaseA};
  REQUIRE_NOTHROW(owner->attributeOwnershipDivestitureIfWanted(
      objectInstance,
      ownedAttributes,
      divestitureTag,
      divestedAttributes));
  REQUIRE(divestedAttributes == ownedAttributes);
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, reliableChild));
  REQUIRE(regularRequester->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE(ifAvailableRequester->isAttributeOwnedByFederate(objectInstance, reliableChild));
  REQUIRE_THROWS_AS(
      regularRequester->unpublishObjectClassAttributes(child, regularAttributes),
      rti1516_2025::OwnershipAcquisitionPending);
  REQUIRE_THROWS_AS(
      ifAvailableRequester->unpublishObjectClassAttributes(child, ifAvailableAttributes),
      rti1516_2025::OwnershipAcquisitionPending);
  REQUIRE_NOTHROW(owner->unpublishObjectClassAttributes(child, ownedAttributes));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());

  // The regular acquirer receives the divestiture tag, not the older regular
  // acquisition tag, and its publication guard ends exactly at callback entry.
  REQUIRE_FALSE(regularRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(regularRequesterReports.attributeOwnershipAcquisitionReports.size() == 1);
  auto const& regularNotification =
      regularRequesterReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(regularNotification.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
  REQUIRE(regularNotification.objectInstance == objectInstance);
  REQUIRE(regularNotification.attributes == regularAttributes);
  REQUIRE(variableLengthDataBytes(regularNotification.userSuppliedTag) ==
          std::vector<unsigned char>(
              divestitureTagBytes,
              divestitureTagBytes + sizeof(divestitureTagBytes)));
  REQUIRE_NOTHROW(regularRequester->unpublishObjectClassAttributes(child, regularAttributes));

  // The stale If Available report is consumed first; its direct divestiture
  // notification follows with the same mandatory divestiture tag.
  REQUIRE(ifAvailableRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ifAvailableRequesterReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE_FALSE(ifAvailableRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ifAvailableRequesterReports.attributeOwnershipAcquisitionReports.size() == 1);
  auto const& ifAvailableNotification =
      ifAvailableRequesterReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(ifAvailableNotification.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
  REQUIRE(ifAvailableNotification.objectInstance == objectInstance);
  REQUIRE(ifAvailableNotification.attributes == ifAvailableAttributes);
  REQUIRE(variableLengthDataBytes(ifAvailableNotification.userSuppliedTag) ==
          std::vector<unsigned char>(
              divestitureTagBytes,
              divestitureTagBytes + sizeof(divestitureTagBytes)));
  REQUIRE_NOTHROW(
      ifAvailableRequester->unpublishObjectClassAttributes(child, ifAvailableAttributes));
  REQUIRE_THROWS_AS(
      owner->attributeOwnershipDivestitureIfWanted(
          objectInstance,
          regularAttributes,
          divestitureTag,
          divestedAttributes),
      rti1516_2025::AttributeNotOwned);

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(regularRequester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(ifAvailableRequester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(regularRequester->disconnect());
  REQUIRE_NOTHROW(ifAvailableRequester->disconnect());
}

TEST_CASE(
    "Embedded Attribute Ownership Divestiture If Wanted orders mixed 2025 acquirers by request",
    "[integration][development-profile][ownership-management]"
    "[rti.service.attribute-ownership-divestiture-if-wanted]"
    "[rti.service.attribute-ownership-acquisition]"
    "[rti.service.attribute-ownership-acquisition-if-available]"
    "[federate.callback.attribute-ownership-acquisition-notification]"
    "[federate.callback.request-attribute-ownership-release]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador firstIfAvailableReports;
  ReportingFederateAmbassador laterRegularReports;
  auto owner = makeRti();
  auto firstIfAvailable = makeRti();
  auto laterRegular = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const ifAvailableTagBytes[] = {0x11, 0x25};
  unsigned char const regularTagBytes[] = {0x22, 0x25};
  unsigned char const divestitureTagBytes[] = {0xD2, 0x25};
  unsigned char const denialTagBytes[] = {0xD3, 0x25};
  VariableLengthData const ifAvailableTag(ifAvailableTagBytes, sizeof(ifAvailableTagBytes));
  VariableLengthData const regularTag(regularTagBytes, sizeof(regularTagBytes));
  VariableLengthData const divestitureTag(divestitureTagBytes, sizeof(divestitureTagBytes));
  VariableLengthData const denialTag(denialTagBytes, sizeof(denialTagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(firstIfAvailable->connect(firstIfAvailableReports, HLA_EVOKED));
  REQUIRE_NOTHROW(laterRegular->connect(laterRegularReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"mixed-divestiture-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(firstIfAvailable->joinFederationExecution(
      L"mixed-divestiture-first-if-available", L"publisher", federationName));
  REQUIRE_NOTHROW(laterRegular->joinFederationExecution(
      L"mixed-divestiture-later-regular", L"publisher", federationName));

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableBaseA = owner->getAttributeHandle(child, L"ReliableBaseA");
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  AttributeHandleSet const attributes{reliableBaseA};
  REQUIRE_NOTHROW(firstIfAvailable->subscribeObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(laterRegular->subscribeObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, attributes));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(firstIfAvailable->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(laterRegular->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(firstIfAvailableReports.objectDiscoveryReports.size() == 1);
  REQUIRE(laterRegularReports.objectDiscoveryReports.size() == 1);
  REQUIRE_NOTHROW(firstIfAvailable->publishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(laterRegular->publishObjectClassAttributes(child, attributes));

  // The standard leaves multi-acquirer arbitration to the RTI. The bounded
  // serial profile makes its policy explicit: select the earliest accepted
  // request across regular and If Available forms, then retain later regular
  // requests for the selected owner to answer.
  REQUIRE_NOTHROW(firstIfAvailable->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      attributes,
      ifAvailableTag));
  REQUIRE_NOTHROW(laterRegular->attributeOwnershipAcquisition(
      objectInstance,
      attributes,
      regularTag));
  AttributeHandleSet divestedAttributes;
  REQUIRE_NOTHROW(owner->attributeOwnershipDivestitureIfWanted(
      objectInstance,
      attributes,
      divestitureTag,
      divestedAttributes));
  REQUIRE(divestedAttributes == attributes);
  REQUIRE(firstIfAvailable->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_FALSE(laterRegular->isAttributeOwnedByFederate(objectInstance, reliableBaseA));

  // The old owner's release request is stale. The first queued If Available
  // terminal report is stale too, followed by the direct notification and a
  // follow-up regular release request addressed to the new owner.
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());
  REQUIRE(firstIfAvailable->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(firstIfAvailableReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE(firstIfAvailable->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(firstIfAvailableReports.attributeOwnershipAcquisitionReports.size() == 1);
  auto const& divestitureNotification =
      firstIfAvailableReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(divestitureNotification.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
  REQUIRE(divestitureNotification.attributes == attributes);
  REQUIRE(variableLengthDataBytes(divestitureNotification.userSuppliedTag) ==
          std::vector<unsigned char>(
              divestitureTagBytes,
              divestitureTagBytes + sizeof(divestitureTagBytes)));
  REQUIRE_FALSE(firstIfAvailable->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(firstIfAvailableReports.attributeOwnershipReleaseRequestReports.size() == 1);
  auto const& laterRelease =
      firstIfAvailableReports.attributeOwnershipReleaseRequestReports.front();
  REQUIRE(laterRelease.objectInstance == objectInstance);
  REQUIRE(laterRelease.attributes == attributes);
  REQUIRE(variableLengthDataBytes(laterRelease.userSuppliedTag) ==
          std::vector<unsigned char>(regularTagBytes, regularTagBytes + sizeof(regularTagBytes)));

  REQUIRE_NOTHROW(firstIfAvailable->attributeOwnershipReleaseDenied(
      objectInstance,
      attributes,
      denialTag));
  REQUIRE_FALSE(laterRegular->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(laterRegularReports.attributeOwnershipAcquisitionReports.size() == 1);
  auto const& laterUnavailable = laterRegularReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(laterUnavailable.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::unavailable);
  REQUIRE(laterUnavailable.attributes == attributes);
  REQUIRE(variableLengthDataBytes(laterUnavailable.userSuppliedTag) ==
          std::vector<unsigned char>(denialTagBytes, denialTagBytes + sizeof(denialTagBytes)));
  REQUIRE_NOTHROW(owner->unpublishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(firstIfAvailable->unpublishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(laterRegular->unpublishObjectClassAttributes(child, attributes));

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(firstIfAvailable->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(laterRegular->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(firstIfAvailable->disconnect());
  REQUIRE_NOTHROW(laterRegular->disconnect());
}

TEST_CASE(
    "Embedded Cancel Attribute Ownership Acquisition honors the 2025 confirmation boundary",
    "[integration][development-profile][ownership-management]"
    "[rti.service.cancel-attribute-ownership-acquisition]"
    "[federate.callback.confirm-attribute-ownership-acquisition]") {
  TestFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const acquisitionTagBytes[] = {0xC0, 0xDE, 0x25};
  VariableLengthData const acquisitionTag(acquisitionTagBytes, sizeof(acquisitionTagBytes));
  ObjectInstanceHandle invalidObjectInstance;
  AttributeHandle invalidAttribute;
  AttributeHandleSet noAttributes;

  REQUIRE_THROWS_AS(
      unjoined->cancelAttributeOwnershipAcquisition(invalidObjectInstance, noAttributes),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->cancelAttributeOwnershipAcquisition(invalidObjectInstance, noAttributes),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"cancellation-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"cancellation-requester",
      L"publisher",
      federationName));

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableBaseA = owner->getAttributeHandle(child, L"ReliableBaseA");
  auto const unownedChild = owner->getAttributeHandle(child, L"UnownedChild");
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(unownedChild.isValid());
  AttributeHandleSet const requestedAttributes{reliableBaseA, unownedChild};

  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(child, requestedAttributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, AttributeHandleSet{reliableBaseA}));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(child, requestedAttributes));

  REQUIRE_THROWS_AS(
      requester->cancelAttributeOwnershipAcquisition(
          invalidObjectInstance,
          AttributeHandleSet{unownedChild}),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      requester->cancelAttributeOwnershipAcquisition(
          objectInstance,
          AttributeHandleSet{invalidAttribute}),
      rti1516_2025::AttributeNotDefined);
  REQUIRE_THROWS_AS(
      requester->cancelAttributeOwnershipAcquisition(
          objectInstance,
          AttributeHandleSet{unownedChild}),
      rti1516_2025::AttributeAcquisitionWasNotRequested);
  REQUIRE_THROWS_AS(
      owner->cancelAttributeOwnershipAcquisition(
          objectInstance,
          AttributeHandleSet{reliableBaseA}),
      rti1516_2025::AttributeAlreadyOwned);

  // An If Available request is not a cancelable regular acquisition. The
  // following regular request overrides it, preserving the official state
  // boundary without synthesizing an explicit WTA cancellation callback.
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      AttributeHandleSet{unownedChild},
      acquisitionTag));
  REQUIRE_THROWS_AS(
      requester->cancelAttributeOwnershipAcquisition(
          objectInstance,
          AttributeHandleSet{unownedChild}),
      rti1516_2025::AttributeAcquisitionWasNotRequested);
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance,
      requestedAttributes,
      acquisitionTag));
  REQUIRE_NOTHROW(requester->cancelAttributeOwnershipAcquisition(
      objectInstance,
      requestedAttributes));

  // Cancellation is terminal only when confirmation begins. Until then it
  // retains the same publication fence and blocks another acquisition mode.
  REQUIRE_THROWS_AS(
      requester->unpublishObjectClassAttributes(child, AttributeHandleSet{reliableBaseA}),
      rti1516_2025::OwnershipAcquisitionPending);
  REQUIRE_THROWS_AS(
      requester->unpublishObjectClassAttributes(child, AttributeHandleSet{unownedChild}),
      rti1516_2025::OwnershipAcquisitionPending);
  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisitionIfAvailable(
          objectInstance,
          AttributeHandleSet{unownedChild},
          acquisitionTag),
      rti1516_2025::AttributeAlreadyBeingAcquired);
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance,
      requestedAttributes,
      acquisitionTag));

  // The queued owner-release request is now stale, and neither it nor the
  // stale notification may reach user code after successful cancellation.
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());
  REQUIRE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.size() == 1);
  auto const& confirmation =
      requesterReports.attributeOwnershipAcquisitionCancellationReports.front();
  REQUIRE(confirmation.objectInstance == objectInstance);
  REQUIRE(confirmation.attributes == requestedAttributes);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, unownedChild));
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_NOTHROW(requester->unpublishObjectClassAttributes(child, requestedAttributes));
  REQUIRE_THROWS_AS(
      requester->cancelAttributeOwnershipAcquisition(objectInstance, requestedAttributes),
      rti1516_2025::AttributeAcquisitionWasNotRequested);

  // Receive-order removal invalidates a queued cancellation confirmation just
  // as it invalidates other ownership callbacks.
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(
      child,
      AttributeHandleSet{reliableBaseA}));
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance,
      AttributeHandleSet{reliableBaseA},
      acquisitionTag));
  REQUIRE_NOTHROW(requester->cancelAttributeOwnershipAcquisition(
      objectInstance,
      AttributeHandleSet{reliableBaseA}));
  VariableLengthData deletionTag;
  REQUIRE_NOTHROW(owner->deleteObjectInstance(objectInstance, deletionTag));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());
  REQUIRE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.size() == 1);
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectRemovalReports.size() == 1);

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}

TEST_CASE(
    "Embedded Negotiated Attribute Ownership Divestiture confirms a pending 2025 acquirer",
    "[integration][development-profile][ownership-management][callbacks]"
    "[rti.service.negotiated-attribute-ownership-divestiture]"
    "[rti.service.confirm-divestiture]"
    "[rti.service.cancel-negotiated-attribute-ownership-divestiture]"
    "[rti.service.attribute-ownership-acquisition]"
    "[federate.callback.request-divestiture-confirmation]"
    "[federate.callback.attribute-ownership-acquisition-notification]") {
  TestFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const acquisitionTagBytes[] = {0x49, 0xA7, 0x11};
  unsigned char const divestitureTagBytes[] = {0x52, 0xA7, 0x11};
  unsigned char const confirmationTagBytes[] = {0x63, 0xA7, 0x11};
  VariableLengthData const acquisitionTag(acquisitionTagBytes, sizeof(acquisitionTagBytes));
  VariableLengthData const divestitureTag(divestitureTagBytes, sizeof(divestitureTagBytes));
  VariableLengthData const confirmationTag(confirmationTagBytes, sizeof(confirmationTagBytes));
  ObjectInstanceHandle invalidObjectInstance;
  AttributeHandle invalidAttribute;
  AttributeHandleSet noAttributes;

  REQUIRE_THROWS_AS(
      unjoined->negotiatedAttributeOwnershipDivestiture(
          invalidObjectInstance,
          noAttributes,
          divestitureTag),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->confirmDivestiture(invalidObjectInstance, noAttributes, confirmationTag),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->cancelNegotiatedAttributeOwnershipDivestiture(
          invalidObjectInstance,
          noAttributes),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->negotiatedAttributeOwnershipDivestiture(
          invalidObjectInstance,
          noAttributes,
          divestitureTag),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->confirmDivestiture(invalidObjectInstance, noAttributes, confirmationTag),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->cancelNegotiatedAttributeOwnershipDivestiture(
          invalidObjectInstance,
          noAttributes),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"negotiated-divestiture-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"negotiated-divestiture-requester", L"publisher", federationName));

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const transferredAttribute = owner->getAttributeHandle(child, L"ReliableBaseA");
  auto const cancelledAttribute = owner->getAttributeHandle(child, L"ReliableBaseB");
  auto const laterAcquirerAttribute = owner->getAttributeHandle(child, L"BestEffortBase");
  auto const noAcquirerAttribute = owner->getAttributeHandle(child, L"ReliableChild");
  auto const cancellationBeforeDeliveryAttribute = owner->getAttributeHandle(child, L"UnownedChild");
  REQUIRE(child.isValid());
  REQUIRE(transferredAttribute.isValid());
  REQUIRE(cancelledAttribute.isValid());
  REQUIRE(laterAcquirerAttribute.isValid());
  REQUIRE(noAcquirerAttribute.isValid());
  REQUIRE(cancellationBeforeDeliveryAttribute.isValid());
  AttributeHandleSet const transferredAttributes{transferredAttribute};
  AttributeHandleSet const cancelledAttributes{cancelledAttribute};
  AttributeHandleSet const laterAcquirerAttributes{laterAcquirerAttribute};
  AttributeHandleSet const noAcquirerAttributes{noAcquirerAttribute};
  AttributeHandleSet const cancellationBeforeDeliveryAttributes{
      cancellationBeforeDeliveryAttribute};
  AttributeHandleSet const ownedAttributes{
      transferredAttribute,
      cancelledAttribute,
      laterAcquirerAttribute,
      noAcquirerAttribute,
      cancellationBeforeDeliveryAttribute,
  };

  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(child, ownedAttributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, ownedAttributes));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(child, ownedAttributes));

  REQUIRE_THROWS_AS(
      owner->negotiatedAttributeOwnershipDivestiture(
          invalidObjectInstance,
          transferredAttributes,
          divestitureTag),
      rti1516_2025::ObjectInstanceNotKnown);
  AttributeHandleSet const invalidAttributes{invalidAttribute};
  REQUIRE_THROWS_AS(
      owner->negotiatedAttributeOwnershipDivestiture(
          objectInstance,
          invalidAttributes,
          divestitureTag),
      rti1516_2025::AttributeNotDefined);
  REQUIRE_THROWS_AS(
      requester->negotiatedAttributeOwnershipDivestiture(
          objectInstance,
          transferredAttributes,
          divestitureTag),
      rti1516_2025::AttributeNotOwned);
  REQUIRE_THROWS_AS(
      owner->cancelNegotiatedAttributeOwnershipDivestiture(objectInstance, transferredAttributes),
      rti1516_2025::AttributeDivestitureWasNotRequested);
  REQUIRE_THROWS_AS(
      owner->confirmDivestiture(objectInstance, transferredAttributes, confirmationTag),
      rti1516_2025::AttributeDivestitureWasNotRequested);

  // An ordinary regular acquisition initially queues Request Attribute
  // Ownership Release. Starting negotiated divestiture before its callback
  // boundary makes that work stale and replaces it with the §7.5 confirmation
  // callback carrying the original acquisition tag.
  REQUIRE_NOTHROW(
      requester->attributeOwnershipAcquisition(objectInstance, transferredAttributes, acquisitionTag));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE_NOTHROW(
      owner->negotiatedAttributeOwnershipDivestiture(
          objectInstance,
          transferredAttributes,
          divestitureTag));
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, transferredAttribute));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, transferredAttribute));
  REQUIRE_THROWS_AS(
      owner->negotiatedAttributeOwnershipDivestiture(
          objectInstance,
          transferredAttributes,
          divestitureTag),
      rti1516_2025::AttributeAlreadyBeingDivested);
  REQUIRE_THROWS_AS(
      owner->confirmDivestiture(objectInstance, transferredAttributes, confirmationTag),
      rti1516_2025::AttributeDivestitureWasNotRequested);
  // The formerly queued ordinary release work is consumed without calling the
  // federate while the negotiated state is active; the real §7.5 callback is
  // then delivered on the next callback pass.
  REQUIRE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());
  REQUIRE(ownerReports.divestitureConfirmationReports.size() == 1);
  auto const& firstConfirmation = ownerReports.divestitureConfirmationReports.front();
  REQUIRE(firstConfirmation.objectInstance == objectInstance);
  REQUIRE(firstConfirmation.attributes == transferredAttributes);
  REQUIRE(variableLengthDataBytes(firstConfirmation.userSuppliedTag) ==
          std::vector<unsigned char>(
              acquisitionTagBytes,
              acquisitionTagBytes + sizeof(acquisitionTagBytes)));
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, transferredAttribute));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, transferredAttribute));

  REQUIRE_NOTHROW(owner->confirmDivestiture(
      objectInstance,
      transferredAttributes,
      confirmationTag));
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, transferredAttribute));
  REQUIRE(requester->isAttributeOwnedByFederate(objectInstance, transferredAttribute));
  REQUIRE_THROWS_AS(
      requester->unpublishObjectClassAttributes(child, transferredAttributes),
      rti1516_2025::OwnershipAcquisitionPending);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.size() == 1);
  auto const& transferNotification = requesterReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(transferNotification.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
  REQUIRE(transferNotification.objectInstance == objectInstance);
  REQUIRE(transferNotification.attributes == transferredAttributes);
  REQUIRE(variableLengthDataBytes(transferNotification.userSuppliedTag) ==
          std::vector<unsigned char>(
              confirmationTagBytes,
              confirmationTagBytes + sizeof(confirmationTagBytes)));
  REQUIRE_NOTHROW(requester->unpublishObjectClassAttributes(child, transferredAttributes));

  // A regular acquisition that arrives after the owner has entered Waiting
  // still takes the §7.5 confirmation route, rather than the ordinary release
  // route. Cancelling after that callback re-plans exactly one normal release.
  REQUIRE_NOTHROW(owner->negotiatedAttributeOwnershipDivestiture(
      objectInstance,
      laterAcquirerAttributes,
      divestitureTag));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.divestitureConfirmationReports.size() == 1);
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance,
      laterAcquirerAttributes,
      acquisitionTag));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());
  REQUIRE(ownerReports.divestitureConfirmationReports.size() == 2);
  REQUIRE(ownerReports.divestitureConfirmationReports.back().attributes == laterAcquirerAttributes);
  REQUIRE_NOTHROW(owner->cancelNegotiatedAttributeOwnershipDivestiture(
      objectInstance,
      laterAcquirerAttributes));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.size() == 1);
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.back().attributes ==
          laterAcquirerAttributes);
  REQUIRE_NOTHROW(requester->cancelAttributeOwnershipAcquisition(
      objectInstance,
      laterAcquirerAttributes));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.size() == 1);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.back().attributes ==
          laterAcquirerAttributes);
  REQUIRE_NOTHROW(requester->unpublishObjectClassAttributes(child, laterAcquirerAttributes));

  // If cancellation happens before the older ordinary release work reaches
  // the owner, the preserved queued reservation lets that original release
  // callback run once. The queued confirmation work is stale and suppressed,
  // so cancellation does not create a duplicate owner callback.
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance,
      cancellationBeforeDeliveryAttributes,
      acquisitionTag));
  REQUIRE_NOTHROW(owner->negotiatedAttributeOwnershipDivestiture(
      objectInstance,
      cancellationBeforeDeliveryAttributes,
      divestitureTag));
  REQUIRE_NOTHROW(owner->cancelNegotiatedAttributeOwnershipDivestiture(
      objectInstance,
      cancellationBeforeDeliveryAttributes));
  REQUIRE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.size() == 2);
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.back().attributes ==
          cancellationBeforeDeliveryAttributes);
  REQUIRE(ownerReports.divestitureConfirmationReports.size() == 2);
  REQUIRE_NOTHROW(requester->cancelAttributeOwnershipAcquisition(
      objectInstance,
      cancellationBeforeDeliveryAttributes));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.size() == 2);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.back().attributes ==
          cancellationBeforeDeliveryAttributes);
  REQUIRE_NOTHROW(requester->unpublishObjectClassAttributes(
      child,
      cancellationBeforeDeliveryAttributes));

  // Cancellation after a real §7.5 callback restores the ordinary release
  // route without changing ownership or allowing a stale confirmation to
  // complete the divestiture.
  REQUIRE_NOTHROW(
      requester->attributeOwnershipAcquisition(objectInstance, cancelledAttributes, acquisitionTag));
  REQUIRE_NOTHROW(
      owner->negotiatedAttributeOwnershipDivestiture(
          objectInstance,
          cancelledAttributes,
          divestitureTag));
  REQUIRE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.divestitureConfirmationReports.size() == 3);
  REQUIRE(ownerReports.divestitureConfirmationReports.back().attributes == cancelledAttributes);
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, cancelledAttribute));
  REQUIRE_NOTHROW(
      owner->cancelNegotiatedAttributeOwnershipDivestiture(objectInstance, cancelledAttributes));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.size() == 3);
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.back().attributes == cancelledAttributes);
  REQUIRE(variableLengthDataBytes(
              ownerReports.attributeOwnershipReleaseRequestReports.back().userSuppliedTag) ==
          std::vector<unsigned char>(
              acquisitionTagBytes,
              acquisitionTagBytes + sizeof(acquisitionTagBytes)));
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, cancelledAttribute));
  REQUIRE_THROWS_AS(
      owner->confirmDivestiture(objectInstance, cancelledAttributes, confirmationTag),
      rti1516_2025::AttributeDivestitureWasNotRequested);
  REQUIRE_NOTHROW(requester->cancelAttributeOwnershipAcquisition(objectInstance, cancelledAttributes));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.size() == 3);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.back().attributes ==
          cancelledAttributes);
  REQUIRE_NOTHROW(requester->unpublishObjectClassAttributes(child, cancelledAttributes));

  // If the selected regular acquirer cancels after the confirmation request,
  // Confirm Divestiture returns the official NoAcquisitionPending exception,
  // leaves ownership intact, and returns the private state to Waiting so the
  // owner can explicitly cancel the negotiated request.
  REQUIRE_NOTHROW(
      requester->attributeOwnershipAcquisition(objectInstance, noAcquirerAttributes, acquisitionTag));
  REQUIRE_NOTHROW(
      owner->negotiatedAttributeOwnershipDivestiture(
          objectInstance,
          noAcquirerAttributes,
          divestitureTag));
  REQUIRE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.divestitureConfirmationReports.size() == 4);
  REQUIRE(ownerReports.divestitureConfirmationReports.back().attributes == noAcquirerAttributes);
  REQUIRE_NOTHROW(requester->cancelAttributeOwnershipAcquisition(
      objectInstance,
      noAcquirerAttributes));
  REQUIRE_THROWS_AS(
      owner->confirmDivestiture(objectInstance, noAcquirerAttributes, confirmationTag),
      rti1516_2025::NoAcquisitionPending);
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, noAcquirerAttribute));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, noAcquirerAttribute));
  REQUIRE_NOTHROW(owner->cancelNegotiatedAttributeOwnershipDivestiture(
      objectInstance,
      noAcquirerAttributes));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.size() == 4);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.back().attributes ==
          noAcquirerAttributes);
  REQUIRE_NOTHROW(requester->unpublishObjectClassAttributes(child, noAcquirerAttributes));

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}

TEST_CASE(
    "Embedded receive-order Send Interaction honors 2025 promotion and callback lifecycle",
    "[integration][development-profile][interaction-management]"
    "[rti.service.send-interaction][federate.callback.receive-interaction]") {
  TestFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador exactReports;
  ReportingFederateAmbassador promotedReports;
  ReportingFederateAmbassador dualSubscriptionReports;
  ReportingFederateAmbassador cancelledReports;
  ReportingFederateAmbassador unsubscribedReports;
  ReportingFederateAmbassador immediateReports;
  auto unjoined = makeRti();
  auto publisher = makeRti();
  auto exact = makeRti();
  auto promoted = makeRti();
  auto dualSubscription = makeRti();
  auto cancelled = makeRti();
  auto unsubscribed = makeRti();
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
  REQUIRE_NOTHROW(unsubscribed->connect(unsubscribedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(immediate->connect(immediateReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));

  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(
      publisherHandle = publisher->joinFederationExecution(
          L"interaction-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(exact->joinFederationExecution(
      L"interaction-exact", L"subscriber", federationName));
  REQUIRE_NOTHROW(promoted->joinFederationExecution(
      L"interaction-promoted", L"subscriber", federationName));
  REQUIRE_NOTHROW(dualSubscription->joinFederationExecution(
      L"interaction-dual", L"subscriber", federationName));
  REQUIRE_NOTHROW(cancelled->joinFederationExecution(
      L"interaction-cancelled", L"subscriber", federationName));
  REQUIRE_NOTHROW(unsubscribed->joinFederationExecution(
      L"interaction-unsubscribed", L"subscriber", federationName));
  REQUIRE_NOTHROW(immediate->joinFederationExecution(
      L"interaction-immediate", L"subscriber", federationName));

  auto const base = publisher->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraParameterFixtureBase");
  auto const child = publisher->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraParameterFixtureBase.UmbraParameterFixtureChild");
  auto const identifier = publisher->getParameterHandle(child, L"Identifier");
  REQUIRE(base.isValid());
  REQUIRE(child.isValid());
  REQUIRE(identifier.isValid());
  unsigned char const identifierBytes[] = {0xC4, 0x19, 0x02};
  ParameterHandleValueMap parameterValues;
  parameterValues.emplace(identifier, VariableLengthData(identifierBytes, sizeof(identifierBytes)));

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

  // A passive superclass subscription remains eligible for Receive
  // Interaction.  Promotion selects the closest subscribed superclass, while
  // a child-and-parent subscription results in exactly one child callback.
  REQUIRE_NOTHROW(publisher->subscribeInteractionClass(child));
  REQUIRE_NOTHROW(exact->subscribeInteractionClass(child));
  REQUIRE_NOTHROW(promoted->subscribeInteractionClass(base, false));
  REQUIRE_NOTHROW(dualSubscription->subscribeInteractionClass(base, false));
  REQUIRE_NOTHROW(dualSubscription->subscribeInteractionClass(child));
  REQUIRE_NOTHROW(cancelled->subscribeInteractionClass(base));
  REQUIRE_NOTHROW(immediate->subscribeInteractionClass(child));

  REQUIRE_NOTHROW(publisher->sendInteraction(child, parameterValues, tag));
  REQUIRE(exactReports.interactionReports.empty());
  REQUIRE(promotedReports.interactionReports.empty());
  REQUIRE(dualSubscriptionReports.interactionReports.empty());
  REQUIRE(cancelledReports.interactionReports.empty());
  REQUIRE(immediateReports.interactionReports.size() == 1);

  // The standard suppresses delivery if a receiver unsubscribes after Send
  // Interaction and before its induced callback is invoked.
  REQUIRE_NOTHROW(cancelled->unsubscribeInteractionClass(base));
  REQUIRE_FALSE(exact->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(promoted->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(dualSubscription->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(cancelled->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(unsubscribed->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));

  auto requireDelivery = [&](ReportingFederateAmbassador::InteractionReport const& report,
                             InteractionClassHandle const& expectedClass) {
    REQUIRE(report.interactionClass == expectedClass);
    REQUIRE(report.parameterValues.size() == 1);
    auto const value = report.parameterValues.find(identifier);
    REQUIRE(value != report.parameterValues.end());
    REQUIRE(variableLengthDataBytes(value->second) ==
            std::vector<unsigned char>(identifierBytes, identifierBytes + sizeof(identifierBytes)));
    REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
            std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
    REQUIRE(report.transportationType == publisher->getTransportationTypeHandle(L"HLAreliable"));
    REQUIRE(report.producingFederate == publisherHandle);
    REQUIRE_FALSE(report.sentRegionsSupplied);
  };

  REQUIRE(exactReports.interactionReports.size() == 1);
  requireDelivery(exactReports.interactionReports.front(), child);
  REQUIRE(promotedReports.interactionReports.size() == 1);
  requireDelivery(promotedReports.interactionReports.front(), base);
  REQUIRE(dualSubscriptionReports.interactionReports.size() == 1);
  requireDelivery(dualSubscriptionReports.interactionReports.front(), child);
  REQUIRE(immediateReports.interactionReports.size() == 1);
  requireDelivery(immediateReports.interactionReports.front(), child);
  REQUIRE(cancelledReports.interactionReports.empty());
  REQUIRE(unsubscribedReports.interactionReports.empty());
  REQUIRE(publisherReports.interactionReports.empty());

  REQUIRE_NOTHROW(immediate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(unsubscribed->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(cancelled->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(dualSubscription->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(promoted->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(exact->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(exact->disconnect());
  REQUIRE_NOTHROW(promoted->disconnect());
  REQUIRE_NOTHROW(dualSubscription->disconnect());
  REQUIRE_NOTHROW(cancelled->disconnect());
  REQUIRE_NOTHROW(unsubscribed->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
}

TEST_CASE(
    "Embedded timestamped Send Interaction queues TSO before the grant and supports retraction",
    "[integration][development-profile][interaction-management][time-management]"
    "[rti.service.send-interaction][rti.service.retract]"
    "[rti.service.time-advance-request][federate.callback.receive-interaction]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x31, 0x42};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"timestamped-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-receiver", L"subscriber", federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraParameterFixtureBase.UmbraParameterFixtureChild");
  auto const identifier = publisher->getParameterHandle(interactionClass, L"Identifier");
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  ParameterHandleValueMap parameterValues;
  unsigned char const identifierBytes[] = {0xA1, 0xB2};
  parameterValues.emplace(
      identifier,
      VariableLengthData(identifierBytes, sizeof(identifierBytes)));

  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  // The sender's lower TSO bound is current time 0 plus lookahead 5.
  REQUIRE_THROWS_AS(
      publisher->sendInteraction(
          interactionClass,
          parameterValues,
          tag,
          rti1516_2025::HLAinteger64Time(4)),
      rti1516_2025::InvalidLogicalTime);

  auto const firstHandle = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(firstHandle.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  // Retracting before the receiver's grant removes the pending TSO entry.
  // Advancing the regulator beyond its lookahead bound then releases the
  // receiver's TAR without invoking a stale interaction callback.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->retract(firstHandle));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  // The next message is delivered at its exact TSO boundary. The callback is
  // entered before the matching Time Advance Grant and carries its handle.
  auto const secondHandle = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(secondHandle.isValid());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2);
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1);
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{"grant", "interaction", "grant"});
  auto const& secondReport = receiverReports.timestampedInteractionReports.front();
  REQUIRE(secondReport.timeImplementationName == L"HLAinteger64Time");
  REQUIRE(secondReport.timeValue == L"7");
  REQUIRE(secondReport.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(secondReport.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(secondReport.retractionSupplied);
  REQUIRE(secondReport.retractionValid);
  REQUIRE(secondReport.parameterValues.size() == 1);
  REQUIRE_THROWS_AS(
      publisher->retract(secondHandle),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded timestamped Update Attribute Values queues passels before the grant and supports retraction",
    "[integration][development-profile][object-management][time-management]"
    "[rti.service.update-attribute-values][rti.service.retract]"
    "[rti.service.time-advance-request][federate.callback.reflect-attribute-values]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0xA4, 0x15};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-attribute-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-attribute-receiver", L"subscriber", federationName));

  auto const child = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliable = publisher->getAttributeHandle(child, L"ReliableBaseA");
  auto const bestEffort = publisher->getAttributeHandle(child, L"BestEffortBase");
  REQUIRE(child.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE(bestEffort.isValid());
  AttributeHandleSet const attributes{reliable, bestEffort};
  AttributeHandleValueMap attributeValues;
  unsigned char const reliableBytes[] = {0x11, 0x22};
  unsigned char const bestEffortBytes[] = {0x33, 0x44, 0x55};
  attributeValues.emplace(
      reliable,
      VariableLengthData(reliableBytes, sizeof(reliableBytes)));
  attributeValues.emplace(
      bestEffort,
      VariableLengthData(bestEffortBytes, sizeof(bestEffortBytes)));

  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(child, attributes));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(child));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1);
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  REQUIRE_THROWS_AS(
      publisher->updateAttributeValues(
          objectInstance,
          attributeValues,
          tag,
          rti1516_2025::HLAinteger64Time(4)),
      rti1516_2025::InvalidLogicalTime);

  auto const firstHandle = publisher->updateAttributeValues(
      objectInstance,
      attributeValues,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(firstHandle.isValid());
  REQUIRE(receiverReports.attributeReflectionReports.empty());

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->retract(firstHandle));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.attributeReflectionReports.empty());

  auto const secondHandle = publisher->updateAttributeValues(
      objectInstance,
      attributeValues,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(secondHandle.isValid());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2);
  REQUIRE(receiverReports.attributeReflectionReports.size() == 2);
  REQUIRE(
      receiverReports.callbackOrder ==
      std::vector<std::string>{"grant", "reflect", "reflect", "grant"});

  bool reliableReported = false;
  bool bestEffortReported = false;
  for (auto const& report : receiverReports.attributeReflectionReports) {
    REQUIRE(report.objectInstance == objectInstance);
    REQUIRE(report.producingFederate == publisherHandle);
    REQUIRE(report.timeImplementationName == L"HLAinteger64Time");
    REQUIRE(report.timeValue == L"7");
    REQUIRE(report.sentOrderType == rti1516_2025::TIMESTAMP);
    REQUIRE(report.receivedOrderType == rti1516_2025::TIMESTAMP);
    REQUIRE(report.retractionSupplied);
    REQUIRE(report.retractionValid);
    REQUIRE_FALSE(report.sentRegionsSupplied);
    REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
            std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
    REQUIRE(report.attributeValues.size() == 1);
    if (report.transportationType == publisher->getTransportationTypeHandle(L"HLAreliable")) {
      reliableReported = report.attributeValues.contains(reliable);
      REQUIRE(variableLengthDataBytes(report.attributeValues.at(reliable)) ==
              std::vector<unsigned char>(
                  reliableBytes,
                  reliableBytes + sizeof(reliableBytes)));
    } else if (
        report.transportationType == publisher->getTransportationTypeHandle(L"HLAbestEffort")) {
      bestEffortReported = report.attributeValues.contains(bestEffort);
      REQUIRE(variableLengthDataBytes(report.attributeValues.at(bestEffort)) ==
              std::vector<unsigned char>(
                  bestEffortBytes,
                  bestEffortBytes + sizeof(bestEffortBytes)));
    } else {
      FAIL("Unexpected timestamped attribute transportation type");
    }
  }
  REQUIRE(reliableReported);
  REQUIRE(bestEffortReported);
  REQUIRE_THROWS_AS(
      publisher->retract(secondHandle),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded timestamped Delete Object Instance reconstitutes on retraction and removes before grant",
    "[integration][development-profile][object-management][time-management]"
    "[rti.service.delete-object-instance][rti.service.retract]"
    "[rti.service.time-advance-request][federate.callback.remove-object-instance]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0xD4, 0x16, 0x2A};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"timestamped-delete-publisher", L"publisher", federationName));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->getFederateHandle(L"timestamped-delete-publisher"));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-delete-receiver", L"subscriber", federationName));

  auto const child = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliable = publisher->getAttributeHandle(child, L"ReliableBaseA");
  auto const bestEffort = publisher->getAttributeHandle(child, L"BestEffortBase");
  AttributeHandleSet const attributes{reliable, bestEffort};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(child, attributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(child));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1);
  auto const objectInstanceName = publisher->getObjectInstanceName(objectInstance);

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  REQUIRE_THROWS_AS(
      publisher->deleteObjectInstance(
          objectInstance,
          tag,
          rti1516_2025::HLAinteger64Time(4)),
      rti1516_2025::InvalidLogicalTime);

  auto const firstHandle = publisher->deleteObjectInstance(
      objectInstance,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(firstHandle.isValid());
  REQUIRE(receiverReports.objectRemovalReports.empty());
  REQUIRE(receiver->getObjectInstanceHandle(objectInstanceName) == objectInstance);

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->retract(firstHandle));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.objectRemovalReports.empty());
  REQUIRE(receiver->getObjectInstanceHandle(objectInstanceName) == objectInstance);

  auto const secondHandle = publisher->deleteObjectInstance(
      objectInstance,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(secondHandle.isValid());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2);
  REQUIRE(receiverReports.objectRemovalReports.size() == 1);
  REQUIRE(
      receiverReports.callbackOrder ==
      std::vector<std::string>{"grant", "remove", "grant"});

  auto const& removal = receiverReports.objectRemovalReports.front();
  REQUIRE(removal.objectInstance == objectInstance);
  REQUIRE(removal.producingFederate == publisherHandle);
  REQUIRE(variableLengthDataBytes(removal.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(removal.timeImplementationName == L"HLAinteger64Time");
  REQUIRE(removal.timeValue == L"7");
  REQUIRE(removal.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(removal.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(removal.retractionSupplied);
  REQUIRE(removal.retractionValid);
  REQUIRE_THROWS_AS(
      receiver->getObjectInstanceHandle(objectInstanceName),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      publisher->getObjectInstanceName(objectInstance),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      publisher->retract(secondHandle),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded directed interactions route to known object-class subscribers",
    "[integration][development-profile][interaction-management][directed]"
    "[rti.service.publish-object-class-directed-interactions]"
    "[rti.service.unpublish-object-class-directed-interactions]"
    "[rti.service.subscribe-object-class-directed-interactions]"
    "[rti.service.unsubscribe-object-class-directed-interactions]"
    "[rti.service.send-directed-interaction]"
    "[federate.callback.receive-directed-interaction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador subscriberReports;
  ReportingFederateAmbassador immediateReports;
  ReportingFederateAmbassador unsubscribedReports;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto immediate = makeRti();
  auto unsubscribed = makeRti();
  auto const federationName = nextFederationName();
  auto const objectConsumer = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                               "cpp" /
                               "tests" /
                               "data" /
                               "directed-interaction-object-consumer-fom.xml")
                                  .wstring();
  auto const interactionProvider = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                                    "cpp" /
                                    "tests" /
                                    "data" /
                                    "directed-interaction-interaction-provider-fom.xml")
                                       .wstring();
  unsigned char const tagBytes[] = {0x43, 0x11, 0x9A};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  InteractionClassHandleSet directedClasses;

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberReports, HLA_EVOKED));
  REQUIRE_NOTHROW(immediate->connect(immediateReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(unsubscribed->connect(unsubscribedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      std::vector<std::wstring>{objectConsumer, interactionProvider},
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"directed-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"directed-subscriber", L"subscriber", federationName));
  REQUIRE_NOTHROW(immediate->joinFederationExecution(
      L"directed-immediate", L"subscriber", federationName));
  REQUIRE_NOTHROW(unsubscribed->joinFederationExecution(
      L"directed-unsubscribed", L"subscriber", federationName));

  auto const objectClass = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraDirectedFixtureObject");
  auto const marker = publisher->getAttributeHandle(objectClass, L"DirectedTargetMarker");
  auto const interactionClass = publisher->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraDirectedFixtureInteraction");
  REQUIRE(objectClass.isValid());
  REQUIRE(marker.isValid());
  REQUIRE(interactionClass.isValid());
  directedClasses.insert(interactionClass);

  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(immediate->subscribeObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(unsubscribed->subscribeObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(publisher->publishObjectClassDirectedInteractions(objectClass, directedClasses));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassDirectedInteractions(objectClass, directedClasses));
  REQUIRE_NOTHROW(immediate->subscribeObjectClassDirectedInteractions(objectClass, directedClasses));

  ObjectInstanceHandle target;
  REQUIRE_NOTHROW(target = publisher->registerObjectInstance(objectClass));
  REQUIRE(target.isValid());
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(unsubscribed->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.objectDiscoveryReports.size() == 1);
  REQUIRE(immediateReports.objectDiscoveryReports.size() == 1);
  REQUIRE(unsubscribedReports.objectDiscoveryReports.size() == 1);

  // The target object and interaction are both valid, but only the two
  // declared directed subscribers receive the receive-order callback. The
  // sender is not an induced recipient.
  REQUIRE_NOTHROW(publisher->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag));
  REQUIRE(subscriberReports.directedInteractionReports.empty());
  REQUIRE(immediateReports.directedInteractionReports.size() == 1);
  REQUIRE(unsubscribedReports.directedInteractionReports.empty());
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.directedInteractionReports.size() == 1);
  auto const& first = subscriberReports.directedInteractionReports.front();
  REQUIRE(first.interactionClass == interactionClass);
  REQUIRE(first.objectInstance == target);
  REQUIRE(first.parameterValues.empty());
  REQUIRE(variableLengthDataBytes(first.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(first.transportationType == publisher->getTransportationTypeHandle(L"HLAreliable"));

  // A queued callback is stale when the recipient unsubscribes before
  // evocation. Re-subscribing plans a fresh callback rather than reviving the
  // old one.
  subscriberReports.directedInteractionReports.clear();
  REQUIRE_NOTHROW(publisher->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag));
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassDirectedInteractions(
      objectClass,
      directedClasses));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.directedInteractionReports.empty());
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassDirectedInteractions(
      objectClass,
      directedClasses));

  // The source publication is another callback-time fence. Unpublishing the
  // pair before evocation suppresses the already accepted send.
  REQUIRE_NOTHROW(publisher->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag));
  REQUIRE_NOTHROW(publisher->unpublishObjectClassDirectedInteractions(
      objectClass,
      directedClasses));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.directedInteractionReports.empty());
  REQUIRE_THROWS_AS(
      publisher->sendDirectedInteraction(
          interactionClass,
          target,
          ParameterHandleValueMap{},
          tag),
      rti1516_2025::InteractionClassNotPublished);

  // Re-publication restores the route for a new send.
  REQUIRE_NOTHROW(publisher->publishObjectClassDirectedInteractions(objectClass, directedClasses));
  REQUIRE_NOTHROW(publisher->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.directedInteractionReports.size() == 1);

  REQUIRE_NOTHROW(immediate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(unsubscribed->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(subscriber->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
  REQUIRE_NOTHROW(unsubscribed->disconnect());
}

TEST_CASE(
    "Embedded regional interaction subscriptions filter 2025 receive-order sends",
    "[integration][development-profile][interaction-management][ddm]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.unsubscribe-interaction-class-with-regions]"
    "[rti.service.send-interaction-with-regions]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador subscriberReports;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const parameterBytes[] = {0x2C, 0x01};
  unsigned char const tagBytes[] = {0x7A, 0x19};
  ParameterHandleValueMap parameterValues;
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(
      publisherHandle = publisher->joinFederationExecution(
          L"regional-interaction-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"regional-interaction-subscriber", L"subscriber", federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
  auto const temperatureOk = publisher->getParameterHandle(interactionClass, L"TemperatureOk");
  auto const serverId = publisher->getDimensionHandle(L"ServerId");
  auto const barQuantity = publisher->getDimensionHandle(L"BarQuantity");
  REQUIRE(interactionClass.isValid());
  REQUIRE(temperatureOk.isValid());
  REQUIRE(serverId.isValid());
  REQUIRE(barQuantity.isValid());
  parameterValues.emplace(
      temperatureOk,
      VariableLengthData(parameterBytes, sizeof(parameterBytes)));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));

  auto const publisherRegion = publisher->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion,
      serverId,
      RangeBounds(0UL, 10UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));

  auto const subscriberRegion = subscriber->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      subscriberRegion,
      serverId,
      RangeBounds(5UL, 15UL)));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(RegionHandleSet{subscriberRegion}));

  // Regional declarations are independent from the ordinary subscription;
  // an empty region set does not create or remove a default subscription.
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClassWithRegions(interactionClass, {}));
  REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClassWithRegions(interactionClass, {}));
  REQUIRE_NOTHROW(publisher->sendInteraction(interactionClass, parameterValues, tag));
  static_cast<void>(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.interactionReports.size() == 1);
  REQUIRE_FALSE(subscriberReports.interactionReports.back().sentRegionsSupplied);
  REQUIRE(subscriberReports.interactionReports.back().interactionClass == interactionClass);
  REQUIRE(subscriberReports.interactionReports.back().producingFederate == publisherHandle);

  // Remove only the non-region subscription. The committed regional
  // subscription then controls whether the send is eligible.
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{subscriberRegion}));
  REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{publisherRegion},
      tag));
  static_cast<void>(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.interactionReports.size() == 2);
  REQUIRE(subscriberReports.interactionReports.back().sentRegionsSupplied);

  // A queued regional send is rechecked at callback entry. Changing the
  // subscription region to a disjoint committed range suppresses the stale
  // delivery without changing the accepted send.
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      subscriberRegion,
      serverId,
      RangeBounds(15UL, 20UL)));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(RegionHandleSet{subscriberRegion}));
  REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{publisherRegion},
      tag));
  static_cast<void>(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.interactionReports.size() == 2);

  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      subscriberRegion,
      serverId,
      RangeBounds(5UL, 15UL)));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(RegionHandleSet{subscriberRegion}));
  REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{publisherRegion},
      tag));
  static_cast<void>(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.interactionReports.size() == 3);
  REQUIRE(subscriberReports.interactionReports.back().sentRegionsSupplied);
  REQUIRE(variableLengthDataBytes(subscriberReports.interactionReports.back().userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));

  // An empty regional send is a no-send operation, while foreign, invalid,
  // uncommitted, and incompatible region specifications fail at the service
  // boundary with the 2025 exception vocabulary.
  REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{},
      tag));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.interactionReports.size() == 3);
  REQUIRE_THROWS_AS(
      publisher->sendInteractionWithRegions(
          interactionClass,
          parameterValues,
          RegionHandleSet{subscriberRegion},
          tag),
      rti1516_2025::RegionNotCreatedByThisFederate);
  REQUIRE_THROWS_AS(
      subscriber->subscribeInteractionClassWithRegions(
          interactionClass,
          RegionHandleSet{publisherRegion}),
      rti1516_2025::RegionNotCreatedByThisFederate);

  auto const uncommittedRegion = subscriber->createRegion(DimensionHandleSet{serverId});
  REQUIRE_THROWS_AS(
      subscriber->subscribeInteractionClassWithRegions(
          interactionClass,
          RegionHandleSet{uncommittedRegion}),
      rti1516_2025::InvalidRegion);
  auto const wrongContextRegion = subscriber->createRegion(DimensionHandleSet{barQuantity});
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      wrongContextRegion,
      barQuantity,
      RangeBounds(0UL, 10UL)));
  REQUIRE_NOTHROW(
      subscriber->commitRegionModifications(RegionHandleSet{wrongContextRegion}));
  REQUIRE_THROWS_AS(
      subscriber->subscribeInteractionClassWithRegions(
          interactionClass,
          RegionHandleSet{wrongContextRegion}),
      rti1516_2025::InvalidRegionContext);
  auto const publisherWrongContextRegion = publisher->createRegion(DimensionHandleSet{barQuantity});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherWrongContextRegion,
      barQuantity,
      RangeBounds(0UL, 10UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(
      RegionHandleSet{publisherWrongContextRegion}));
  REQUIRE_THROWS_AS(
      publisher->sendInteractionWithRegions(
          interactionClass,
          parameterValues,
          RegionHandleSet{publisherWrongContextRegion},
          tag),
      rti1516_2025::InvalidRegionContext);

  REQUIRE_THROWS_AS(
      subscriber->deleteRegion(subscriberRegion),
      rti1516_2025::RegionInUseForUpdateOrSubscription);
  REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{subscriberRegion}));
  REQUIRE_NOTHROW(subscriber->deleteRegion(subscriberRegion));
  REQUIRE_NOTHROW(subscriber->deleteRegion(uncommittedRegion));
  REQUIRE_NOTHROW(subscriber->deleteRegion(wrongContextRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherWrongContextRegion));

  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(subscriber->disconnect());
}

TEST_CASE(
    "Embedded federation-list services dispatch standards reports in both callback models",
    "[integration][development-profile][federation-management][callbacks]"
    "[rti.service.list-federation-executions][rti.service.list-federation-execution-members]") {
  ReportingFederateAmbassador evokedReports;
  ReportingFederateAmbassador immediateReports;
  ReportingFederateAmbassador disconnectedReports;
  TestFederateAmbassador creatorFederate;
  TestFederateAmbassador memberFederate;
  auto evoked = makeRti();
  auto immediate = makeRti();
  auto disconnected = makeRti();
  auto creator = makeRti();
  auto member = makeRti();
  auto const firstFederationName = nextFederationName();
  auto const secondFederationName = nextFederationName();
  auto const missingFederationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_THROWS_AS(evoked->listFederationExecutions(), rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      evoked->listFederationExecutionMembers(firstFederationName),
      rti1516_2025::NotConnected);

  REQUIRE_NOTHROW(evoked->connect(evokedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(immediate->connect(immediateReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(disconnected->connect(disconnectedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(creator->connect(creatorFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(member->connect(memberFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      creator->createFederationExecution(firstFederationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      creator->createFederationExecution(secondFederationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(member->joinFederationExecution(L"listed-member", L"observer", firstFederationName));

  REQUIRE_NOTHROW(evoked->listFederationExecutions());
  REQUIRE(evokedReports.federationExecutionReports.empty());
  REQUIRE_FALSE(evoked->evokeCallback(0.0));
  REQUIRE(evokedReports.federationExecutionReports.size() == 1);
  auto const& listedFederations = evokedReports.federationExecutionReports.front();
  REQUIRE(listedFederations.size() == 2);
  auto const first = std::find_if(
      listedFederations.begin(),
      listedFederations.end(),
      [&firstFederationName](auto const& federation) {
        return federation.federationExecutionName == firstFederationName;
      });
  REQUIRE(first != listedFederations.end());
  REQUIRE(first->logicalTimeImplementationName == L"HLAinteger64Time");

  REQUIRE_NOTHROW(evoked->listFederationExecutionMembers(firstFederationName));
  REQUIRE_FALSE(evoked->evokeCallback(0.0));
  REQUIRE(evokedReports.federationExecutionMemberReports.size() == 1);
  auto const& memberReport = evokedReports.federationExecutionMemberReports.front();
  REQUIRE(memberReport.federationName == firstFederationName);
  REQUIRE(memberReport.members.size() == 1);
  REQUIRE(memberReport.members.front().federateName == L"listed-member");
  REQUIRE(memberReport.members.front().federateType == L"observer");

  REQUIRE_NOTHROW(evoked->listFederationExecutionMembers(missingFederationName));
  REQUIRE_FALSE(evoked->evokeCallback(0.0));
  REQUIRE(evokedReports.missingFederationReports == std::vector<std::wstring>{missingFederationName});

  REQUIRE_NOTHROW(immediate->listFederationExecutions());
  REQUIRE(immediateReports.federationExecutionReports.size() == 1);
  REQUIRE(immediateReports.federationExecutionReports.front().size() == 2);

  // Disconnect must discard a report that was queued against the prior
  // callback session; a later Evoke cannot dereference that stale recipient.
  REQUIRE_NOTHROW(disconnected->listFederationExecutions());
  REQUIRE_NOTHROW(disconnected->disconnect());
  REQUIRE_FALSE(disconnected->evokeCallback(0.0));
  REQUIRE(disconnectedReports.federationExecutionReports.empty());

  REQUIRE_NOTHROW(member->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(creator->destroyFederationExecution(firstFederationName));
  REQUIRE_NOTHROW(creator->destroyFederationExecution(secondFederationName));
  REQUIRE_NOTHROW(evoked->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
  REQUIRE_NOTHROW(creator->disconnect());
  REQUIRE_NOTHROW(member->disconnect());
}

TEST_CASE(
    "Embedded Time Advance Request changes logical time only at Time Advance Grant dispatch",
    "[integration][development-profile][time-management][callbacks]"
    "[rti.service.time-advance-request][rti.service.query-logical-time]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador evokedReports;
  ReportingFederateAmbassador immediateReports;
  auto evoked = makeRti();
  auto immediate = makeRti();
  auto const evokedFederationName = nextFederationName();
  auto const immediateFederationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  rti1516_2025::HLAinteger64Time queriedTime;
  REQUIRE_THROWS_AS(evoked->queryLogicalTime(queriedTime), rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      evoked->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)),
      rti1516_2025::NotConnected);

  REQUIRE_NOTHROW(evoked->connect(evokedReports, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      evoked->queryLogicalTime(queriedTime),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      evoked->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(
      evoked->createFederationExecution(
          evokedFederationName,
          fomModule,
          L"HLAinteger64Time"));
  REQUIRE_NOTHROW(evoked->joinFederationExecution(L"evoked-time-client", evokedFederationName));
  REQUIRE_NOTHROW(evoked->queryLogicalTime(queriedTime));
  REQUIRE(queriedTime.isInitial());
  rti1516_2025::HLAfloat64Time mismatchedQueryTime;
  REQUIRE_THROWS_AS(
      evoked->queryLogicalTime(mismatchedQueryTime),
      rti1516_2025::RTIinternalError);

  REQUIRE_THROWS_AS(
      evoked->timeAdvanceRequest(rti1516_2025::HLAfloat64Time(7.0)),
      rti1516_2025::InvalidLogicalTime);
  REQUIRE_NOTHROW(evoked->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(evoked->queryLogicalTime(queriedTime));
  REQUIRE(queriedTime.isInitial());
  REQUIRE(evokedReports.timeAdvanceGrantReports.empty());
  REQUIRE_THROWS_AS(
      evoked->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(8)),
      rti1516_2025::InTimeAdvancingState);

  REQUIRE_FALSE(evoked->evokeCallback(0.0));
  REQUIRE(evokedReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(evokedReports.timeAdvanceGrantReports.front().implementationName == L"HLAinteger64Time");
  REQUIRE(evokedReports.timeAdvanceGrantReports.front().value == L"7");
  REQUIRE_NOTHROW(evoked->queryLogicalTime(queriedTime));
  REQUIRE(queriedTime.getTime() == 7);
  REQUIRE_THROWS_AS(
      evoked->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)),
      rti1516_2025::LogicalTimeAlreadyPassed);

  // Resignation deactivates the per-federate state. Its already-queued grant
  // is harmlessly consumed without advancing state or invoking a callback.
  REQUIRE_NOTHROW(evoked->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_NOTHROW(evoked->resignFederationExecution(NO_ACTION));
  REQUIRE_FALSE(evoked->evokeCallback(0.0));
  REQUIRE(evokedReports.timeAdvanceGrantReports.size() == 1);

  REQUIRE_NOTHROW(immediate->connect(immediateReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      immediate->createFederationExecution(
          immediateFederationName,
          fomModule,
          L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      immediate->joinFederationExecution(L"immediate-time-client", immediateFederationName));
  REQUIRE_NOTHROW(immediate->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE(immediateReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(immediateReports.timeAdvanceGrantReports.front().value == L"5");
  rti1516_2025::HLAinteger64Time immediateTime;
  REQUIRE_NOTHROW(immediate->queryLogicalTime(immediateTime));
  REQUIRE(immediateTime.getTime() == 5);

  REQUIRE_NOTHROW(evoked->destroyFederationExecution(evokedFederationName));
  REQUIRE_NOTHROW(immediate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(immediate->destroyFederationExecution(immediateFederationName));
  REQUIRE_NOTHROW(evoked->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
}

TEST_CASE(
    "Embedded time-role services keep enable requests callback-gated before TSO support",
    "[integration][development-profile][time-management][callbacks]"
    "[rti.service.enable-time-regulation][rti.service.disable-time-regulation]"
    "[rti.service.enable-time-constrained][rti.service.disable-time-constrained]"
    "[rti.service.query-lookahead]"
    "[federate.callback.time-regulation-enabled]"
    "[federate.callback.time-constrained-enabled]") {
  ReportingFederateAmbassador evokedReports;
  ReportingFederateAmbassador immediateReports;
  auto evoked = makeRti();
  auto immediate = makeRti();
  auto const evokedFederationName = nextFederationName();
  auto const immediateFederationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  rti1516_2025::HLAinteger64Interval queriedLookahead;
  REQUIRE_THROWS_AS(
      evoked->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(2)),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(evoked->disableTimeRegulation(), rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(evoked->enableTimeConstrained(), rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(evoked->disableTimeConstrained(), rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(evoked->queryLookahead(queriedLookahead), rti1516_2025::NotConnected);

  REQUIRE_NOTHROW(evoked->connect(evokedReports, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      evoked->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(2)),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(evoked->enableTimeConstrained(), rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      evoked->queryLookahead(queriedLookahead),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(
      evoked->createFederationExecution(
          evokedFederationName,
          fomModule,
          L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      evoked->joinFederationExecution(L"evoked-time-role-client", evokedFederationName));
  REQUIRE_THROWS_AS(
      evoked->queryLookahead(queriedLookahead),
      rti1516_2025::TimeRegulationIsNotEnabled);
  REQUIRE_THROWS_AS(
      evoked->enableTimeRegulation(rti1516_2025::HLAfloat64Interval(2.0)),
      rti1516_2025::InvalidLookahead);
  // The official reference interval constructors reject a negative value
  // before a caller can invoke the service; the mismatched reference type
  // above exercises Umbra's service-boundary InvalidLookahead mapping.

  REQUIRE_NOTHROW(evoked->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(2)));
  REQUIRE(evokedReports.timeRegulationEnabledReports.empty());
  REQUIRE_THROWS_AS(
      evoked->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)),
      rti1516_2025::RequestForTimeRegulationPending);
  REQUIRE_THROWS_AS(
      evoked->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(3)),
      rti1516_2025::RequestForTimeRegulationPending);
  REQUIRE_FALSE(evoked->evokeCallback(0.0));
  REQUIRE(evokedReports.timeRegulationEnabledReports.size() == 1);
  REQUIRE(evokedReports.timeRegulationEnabledReports.front().implementationName == L"HLAinteger64Time");
  REQUIRE(evokedReports.timeRegulationEnabledReports.front().value == L"0");
  REQUIRE_NOTHROW(evoked->queryLookahead(queriedLookahead));
  REQUIRE(queriedLookahead.getInterval() == 2);
  rti1516_2025::HLAfloat64Interval mismatchedQueryLookahead;
  REQUIRE_THROWS_AS(
      evoked->queryLookahead(mismatchedQueryLookahead),
      rti1516_2025::RTIinternalError);
  REQUIRE_NOTHROW(evoked->disableTimeRegulation());
  REQUIRE_THROWS_AS(
      evoked->queryLookahead(queriedLookahead),
      rti1516_2025::TimeRegulationIsNotEnabled);
  REQUIRE_THROWS_AS(
      evoked->disableTimeRegulation(),
      rti1516_2025::TimeRegulationIsNotEnabled);

  REQUIRE_NOTHROW(evoked->enableTimeConstrained());
  REQUIRE(evokedReports.timeConstrainedEnabledReports.empty());
  REQUIRE_THROWS_AS(
      evoked->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)),
      rti1516_2025::RequestForTimeConstrainedPending);
  REQUIRE_THROWS_AS(
      evoked->enableTimeConstrained(),
      rti1516_2025::RequestForTimeConstrainedPending);
  REQUIRE_FALSE(evoked->evokeCallback(0.0));
  REQUIRE(evokedReports.timeConstrainedEnabledReports.size() == 1);
  REQUIRE(evokedReports.timeConstrainedEnabledReports.front().value == L"0");
  REQUIRE_NOTHROW(evoked->disableTimeConstrained());
  REQUIRE_THROWS_AS(
      evoked->disableTimeConstrained(),
      rti1516_2025::TimeConstrainedIsNotEnabled);

  REQUIRE_NOTHROW(immediate->connect(immediateReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      immediate->createFederationExecution(
          immediateFederationName,
          fomModule,
          L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      immediate->joinFederationExecution(L"immediate-time-role-client", immediateFederationName));
  REQUIRE_NOTHROW(immediate->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE(immediateReports.timeRegulationEnabledReports.size() == 1);
  rti1516_2025::HLAinteger64Interval immediateLookahead;
  REQUIRE_NOTHROW(immediate->queryLookahead(immediateLookahead));
  REQUIRE(immediateLookahead.getInterval() == 5);
  REQUIRE_NOTHROW(immediate->enableTimeConstrained());
  REQUIRE(immediateReports.timeConstrainedEnabledReports.size() == 1);

  REQUIRE_NOTHROW(evoked->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(evoked->destroyFederationExecution(evokedFederationName));
  REQUIRE_NOTHROW(immediate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(immediate->destroyFederationExecution(immediateFederationName));
  REQUIRE_NOTHROW(evoked->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
}

TEST_CASE(
    "Embedded Query GALT and Query LITS observe other regulator time and pending advances",
    "[integration][development-profile][time-management][galt][lits]"
    "[rti.service.query-galt][rti.service.query-lits]") {
  ReportingFederateAmbassador receiverReports;
  ReportingFederateAmbassador regulatorReports;
  auto receiver = makeRti();
  auto regulator = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  rti1516_2025::HLAinteger64Time galt;
  rti1516_2025::HLAinteger64Time lits;

  REQUIRE_THROWS_AS(receiver->queryGALT(galt), rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(receiver->queryLITS(lits), rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_THROWS_AS(receiver->queryGALT(galt), rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(receiver->queryLITS(lits), rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(
      receiver->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(L"receiver", federationName));
  REQUIRE_NOTHROW(regulator->connect(regulatorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regulator->joinFederationExecution(L"regulator", federationName));

  // A regulator is not active until the Time Regulation Enabled callback; no
  // other regulator means both no-TSO bounds are correctly undefined.
  REQUIRE_FALSE(receiver->queryGALT(galt));
  REQUIRE_FALSE(receiver->queryLITS(lits));
  REQUIRE_NOTHROW(regulator->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(2)));
  REQUIRE_FALSE(receiver->queryGALT(galt));
  REQUIRE_FALSE(regulator->evokeCallback(0.0));

  REQUIRE(receiver->queryGALT(galt));
  REQUIRE(galt.getTime() == 2);
  REQUIRE(receiver->queryLITS(lits));
  REQUIRE(lits.getTime() == 2);
  rti1516_2025::HLAfloat64Time mismatchedOutput;
  REQUIRE_THROWS_AS(receiver->queryGALT(mismatchedOutput), rti1516_2025::RTIinternalError);

  // While the regulator is Time Advancing, its requested time rather than its
  // prior granted time constrains its earliest possible future TSO timestamp.
  REQUIRE_NOTHROW(regulator->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE(receiver->queryGALT(galt));
  REQUIRE(galt.getTime() == 7);
  REQUIRE(receiver->queryLITS(lits));
  REQUIRE(lits.getTime() == 7);
  REQUIRE_FALSE(regulator->evokeCallback(0.0));
  REQUIRE(receiver->queryGALT(galt));
  REQUIRE(galt.getTime() == 7);

  REQUIRE_NOTHROW(regulator->disableTimeRegulation());
  REQUIRE_FALSE(receiver->queryGALT(galt));
  REQUIRE_FALSE(receiver->queryLITS(lits));

  REQUIRE_NOTHROW(regulator->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(regulator->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());
}

TEST_CASE(
    "Embedded three-federate GALT tracks the minimum regulator and resignation",
    "[integration][development-profile][time-management][galt][lits][callbacks]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request][rti.service.query-galt]"
    "[rti.service.query-lits][rti.service.resign-federation-execution]") {
  ReportingFederateAmbassador observerReports;
  ReportingFederateAmbassador leftRegulatorReports;
  ReportingFederateAmbassador rightRegulatorReports;
  auto observer = makeRti();
  auto leftRegulator = makeRti();
  auto rightRegulator = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  rti1516_2025::HLAinteger64Time galt;
  rti1516_2025::HLAinteger64Time lits;
  rti1516_2025::HLAinteger64Time leftTime;

  REQUIRE_NOTHROW(observer->connect(observerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      observer->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(observer->joinFederationExecution(L"observer", federationName));
  REQUIRE_NOTHROW(leftRegulator->connect(leftRegulatorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(leftRegulator->joinFederationExecution(L"left-regulator", federationName));
  REQUIRE_NOTHROW(rightRegulator->connect(rightRegulatorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(rightRegulator->joinFederationExecution(L"right-regulator", federationName));

  REQUIRE_NOTHROW(observer->enableTimeConstrained());
  REQUIRE_FALSE(observer->evokeCallback(0.0));
  REQUIRE(observerReports.timeConstrainedEnabledReports.size() == 1);
  REQUIRE_NOTHROW(leftRegulator->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(leftRegulator->evokeCallback(0.0));
  REQUIRE(leftRegulatorReports.timeRegulationEnabledReports.size() == 1);
  REQUIRE_NOTHROW(rightRegulator->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(3)));
  REQUIRE_FALSE(rightRegulator->evokeCallback(0.0));
  REQUIRE(rightRegulatorReports.timeRegulationEnabledReports.size() == 1);

  // At the initial logical time, the left regulator (0 + 1) supplies the
  // minimum GALT/LITS candidate rather than the right regulator (0 + 3).
  REQUIRE(observer->queryGALT(galt));
  REQUIRE(galt.getTime() == 1);
  REQUIRE(observer->queryLITS(lits));
  REQUIRE(lits.getTime() == 1);

  // A pending and then granted TAR changes the left regulator's candidate to
  // 5 + 1.  The active right regulator continues to set the minimum at 3.
  REQUIRE_NOTHROW(leftRegulator->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE(observer->queryGALT(galt));
  REQUIRE(galt.getTime() == 3);
  REQUIRE_FALSE(leftRegulator->evokeCallback(0.0));
  REQUIRE(leftRegulatorReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE_NOTHROW(leftRegulator->queryLogicalTime(leftTime));
  REQUIRE(leftTime.getTime() == 5);
  REQUIRE(observer->queryGALT(galt));
  REQUIRE(galt.getTime() == 3);

  // Resigning the regulator that supplied the minimum removes it from the
  // federation-owned snapshot, leaving the left regulator's 5 + 1 candidate.
  REQUIRE_NOTHROW(rightRegulator->resignFederationExecution(NO_ACTION));
  REQUIRE(observer->queryGALT(galt));
  REQUIRE(galt.getTime() == 6);
  REQUIRE(observer->queryLITS(lits));
  REQUIRE(lits.getTime() == 6);

  REQUIRE_NOTHROW(leftRegulator->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(observer->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rightRegulator->disconnect());
  REQUIRE_NOTHROW(leftRegulator->disconnect());
  REQUIRE_NOTHROW(observer->disconnect());
}

TEST_CASE(
    "Embedded constrained TAR waits for GALT and is released by a regulator advance",
    "[integration][development-profile][time-management][galt][callbacks]"
    "[rti.service.time-advance-request][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador receiverReports;
  ReportingFederateAmbassador regulatorReports;
  auto receiver = makeRti();
  auto regulator = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      receiver->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(L"receiver", federationName));
  REQUIRE_NOTHROW(regulator->connect(regulatorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regulator->joinFederationExecution(L"regulator", federationName));

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1);
  REQUIRE_NOTHROW(regulator->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(2)));
  REQUIRE_FALSE(regulator->evokeCallback(0.0));
  REQUIRE(regulatorReports.timeRegulationEnabledReports.size() == 1);

  // GALT is 2, so TAR is strict and TAR(2) must remain pending.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());
  rti1516_2025::HLAinteger64Time receiverTime;
  REQUIRE_NOTHROW(receiver->queryLogicalTime(receiverTime));
  REQUIRE(receiverTime.isInitial());

  // While the regulator is Time Advancing to 1, its pending time plus
  // lookahead raises GALT to 3 and releases the receiver's TAR(2), before the
  // regulator receives its own grant.
  REQUIRE_NOTHROW(regulator->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.timeAdvanceGrantReports.front().value == L"2");
  REQUIRE_NOTHROW(receiver->queryLogicalTime(receiverTime));
  REQUIRE(receiverTime.getTime() == 2);

  REQUIRE_FALSE(regulator->evokeCallback(0.0));
  REQUIRE(regulatorReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(regulatorReports.timeAdvanceGrantReports.front().value == L"1");

  REQUIRE_NOTHROW(regulator->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(regulator->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());
}

TEST_CASE(
    "Embedded NRG-disabled constrained TAR waits until a regulator becomes active",
    "[integration][development-profile][time-management][galt][callbacks]"
    "[rti.service.enable-time-regulation][rti.service.time-advance-request]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador receiverReports;
  ReportingFederateAmbassador regulatorReports;
  auto receiver = makeRti();
  auto regulator = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      receiver->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(L"receiver", federationName));
  REQUIRE_NOTHROW(regulator->connect(regulatorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regulator->joinFederationExecution(L"regulator", federationName));

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());

  // The supplied FOM omits NRG, so its standard default is Disabled. Once a
  // regulator becomes active at time 0 with lookahead 2, TAR(1) is below the
  // newly defined GALT and the role callback wakes the deferred receiver.
  REQUIRE_NOTHROW(regulator->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(2)));
  REQUIRE_FALSE(regulator->evokeCallback(0.0));
  REQUIRE(regulatorReports.timeRegulationEnabledReports.size() == 1);
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.timeAdvanceGrantReports.front().value == L"1");

  REQUIRE_NOTHROW(regulator->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(regulator->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());
}

TEST_CASE(
    "Embedded constrained TAR is released when time-constrained mode is disabled",
    "[integration][development-profile][time-management][galt][callbacks]"
    "[rti.service.disable-time-constrained][rti.service.time-advance-request]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador receiverReports;
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      receiver->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(L"receiver", federationName));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));

  // No other regulator means GALT is undefined, and the supplied FOM's NRG
  // switch defaults to Disabled. The constrained TAR therefore remains
  // pending until the role transition removes its GALT restriction.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());
  REQUIRE_NOTHROW(receiver->disableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.timeAdvanceGrantReports.front().value == L"1");

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
}

TEST_CASE(
    "Embedded NRG-enabled constrained TAR is released when its only regulator disables",
    "[integration][development-profile][time-management][galt][non-regulated-grant][callbacks]"
    "[rti.service.disable-time-regulation][rti.service.time-advance-request]"
    "[federate.callback.time-advance-grant]") {
  auto const nrgFom = nrgEnabledRestaurantModule();
  ReportingFederateAmbassador receiverReports;
  ReportingFederateAmbassador regulatorReports;
  auto receiver = makeRti();
  auto regulator = makeRti();
  auto const federationName = nextFederationName();

  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->createFederationExecution(
      federationName,
      nrgFom.path().wstring(),
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(L"receiver", federationName));
  REQUIRE_NOTHROW(regulator->connect(regulatorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regulator->joinFederationExecution(L"regulator", federationName));

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(regulator->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(2)));
  REQUIRE_FALSE(regulator->evokeCallback(0.0));

  // Defined GALT is 2, so the strict TAR(2) waits while the regulator is
  // active. Disabling the sole regulator makes GALT undefined; the FDD's
  // enabled NRG switch then lets the receiver advance without waiting.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());
  REQUIRE_NOTHROW(regulator->disableTimeRegulation());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.timeAdvanceGrantReports.front().value == L"2");

  REQUIRE_NOTHROW(regulator->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(regulator->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());
}

TEST_CASE(
    "Embedded NRG-enabled constrained TAR is released when its only regulator resigns",
    "[integration][development-profile][time-management][galt][non-regulated-grant][callbacks]"
    "[rti.service.resign-federation-execution][rti.service.time-advance-request]"
    "[federate.callback.time-advance-grant]") {
  auto const nrgFom = nrgEnabledRestaurantModule();
  ReportingFederateAmbassador receiverReports;
  ReportingFederateAmbassador regulatorReports;
  auto receiver = makeRti();
  auto regulator = makeRti();
  auto const federationName = nextFederationName();

  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->createFederationExecution(
      federationName,
      nrgFom.path().wstring(),
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(L"receiver", federationName));
  REQUIRE_NOTHROW(regulator->connect(regulatorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regulator->joinFederationExecution(L"regulator", federationName));

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(regulator->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(2)));
  REQUIRE_FALSE(regulator->evokeCallback(0.0));

  // The defined GALT is 2, so strict TAR(2) waits. Resigning the sole
  // regulator removes the bound; enabled NRG then releases the pending TAR.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());
  REQUIRE_NOTHROW(regulator->resignFederationExecution(NO_ACTION));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.timeAdvanceGrantReports.front().value == L"2");

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(regulator->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());
}

TEST_CASE(
    "Embedded additional FOM NRG metadata re-evaluates a pending constrained TAR",
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
      receiver->createFederationExecution(federationName, baseFom, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(L"receiver", federationName));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));

  // The official Restaurant FOM omits NRG, which means Disabled. With no
  // regulator, the constrained TAR must remain pending before the additional
  // module becomes part of the composed federation definition.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());

  REQUIRE_NOTHROW(applicant->connect(applicantReports, HLA_EVOKED));
  REQUIRE_NOTHROW(applicant->joinFederationExecution(
      L"metadata-applicant",
      L"observer",
      federationName,
      std::vector<std::wstring>{nrgFom.path().wstring()}));

  // The successful MIM-first replacement definition enables NRG. The existing
  // TAR is re-evaluated only after that definition and membership commit.
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.timeAdvanceGrantReports.front().value == L"1");

  REQUIRE_NOTHROW(applicant->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(applicant->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());
}

TEST_CASE(
    "Embedded 2025 object-instance name reservation commits and reports asynchronously",
    "[integration][development-profile][federation-management]"
    "[rti.service.reserve-object-instance-name][rti.service.release-object-instance-name]"
    "[rti.service.reserve-multiple-object-instance-names]"
    "[rti.service.release-multiple-object-instance-names]"
    "[federate.callback.object-instance-name-reservation-succeeded]"
    "[federate.callback.object-instance-name-reservation-failed]"
    "[federate.callback.multiple-object-instance-name-reservation-succeeded]"
    "[federate.callback.multiple-object-instance-name-reservation-failed]") {
  ReportingFederateAmbassador unjoinedReports;
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador peerReports;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_THROWS_AS(
      unjoined->reserveObjectInstanceName(L"Umbra.NotConnected"),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->reserveMultipleObjectInstanceNames({L"Umbra.NotConnected"}),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedReports, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->reserveObjectInstanceName(L"Umbra.NotJoined"),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(L"reservation-owner", L"owner", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(L"reservation-peer", L"peer", federationName));

  REQUIRE_THROWS_AS(
      owner->reserveObjectInstanceName(L""),
      rti1516_2025::IllegalName);
  REQUIRE_THROWS_AS(
      owner->reserveObjectInstanceName(L"HLA.ReservedByTheRTI"),
      rti1516_2025::IllegalName);
  REQUIRE_THROWS_AS(
      owner->reserveMultipleObjectInstanceNames({}),
      rti1516_2025::NameSetWasEmpty);
  REQUIRE_THROWS_AS(
      owner->reserveMultipleObjectInstanceNames({L"Umbra.Valid", L"HLA.Invalid"}),
      rti1516_2025::IllegalName);

  auto const singleName = std::wstring{L"Umbra.SingleReservation"};
  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(singleName));
  REQUIRE(ownerReports.objectInstanceNameReservationSucceededReports.empty());
  REQUIRE(ownerReports.objectInstanceNameReservationFailedReports.empty());
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.objectInstanceNameReservationSucceededReports.size() == 1);
  REQUIRE(ownerReports.objectInstanceNameReservationSucceededReports.front().objectInstanceName ==
          singleName);

  // Contention is an asynchronous failure, not ObjectInstanceNameInUse at
  // the service call boundary.
  REQUIRE_NOTHROW(peer->reserveObjectInstanceName(singleName));
  REQUIRE(peerReports.objectInstanceNameReservationFailedReports.empty());
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.objectInstanceNameReservationFailedReports.size() == 1);
  REQUIRE(peerReports.objectInstanceNameReservationFailedReports.front().objectInstanceName ==
          singleName);

  REQUIRE_NOTHROW(owner->releaseObjectInstanceName(singleName));
  REQUIRE_THROWS_AS(
      owner->releaseObjectInstanceName(singleName),
      rti1516_2025::ObjectInstanceNameNotReserved);
  REQUIRE_NOTHROW(peer->reserveObjectInstanceName(singleName));
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.objectInstanceNameReservationSucceededReports.size() == 1);
  REQUIRE(peerReports.objectInstanceNameReservationSucceededReports.front().objectInstanceName ==
          singleName);
  REQUIRE_NOTHROW(peer->releaseObjectInstanceName(singleName));

  auto const multipleA = std::wstring{L"Umbra.MultipleA"};
  auto const multipleB = std::wstring{L"Umbra.MultipleB"};
  auto const multipleC = std::wstring{L"Umbra.MultipleC"};
  REQUIRE_NOTHROW(owner->reserveMultipleObjectInstanceNames({multipleA, multipleB}));
  REQUIRE(ownerReports.multipleObjectInstanceNameReservationSucceededReports.empty());
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.multipleObjectInstanceNameReservationSucceededReports.size() == 1);
  REQUIRE(
      ownerReports.multipleObjectInstanceNameReservationSucceededReports.front().objectInstanceNames ==
      std::set<std::wstring>{multipleA, multipleB});

  REQUIRE_NOTHROW(peer->reserveMultipleObjectInstanceNames({multipleB, multipleC}));
  REQUIRE(peerReports.multipleObjectInstanceNameReservationSucceededReports.empty());
  REQUIRE(peerReports.multipleObjectInstanceNameReservationFailedReports.empty());
  REQUIRE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.multipleObjectInstanceNameReservationSucceededReports.size() == 1);
  REQUIRE(peerReports.multipleObjectInstanceNameReservationFailedReports.size() == 1);
  REQUIRE(
      peerReports.multipleObjectInstanceNameReservationSucceededReports.front().objectInstanceNames ==
      std::set<std::wstring>{multipleC});
  REQUIRE(
      peerReports.multipleObjectInstanceNameReservationFailedReports.front().objectInstanceNames ==
      std::set<std::wstring>{multipleB});

  // Multiple release validates the entire set before mutating any name.
  REQUIRE_THROWS_AS(
      owner->releaseMultipleObjectInstanceNames({multipleA, multipleC}),
      rti1516_2025::ObjectInstanceNameNotReserved);
  REQUIRE_NOTHROW(peer->releaseObjectInstanceName(multipleC));
  REQUIRE_NOTHROW(owner->releaseMultipleObjectInstanceNames({multipleA, multipleB}));
  REQUIRE_NOTHROW(owner->releaseMultipleObjectInstanceNames({}));

  // A reserved RTI-generated spelling cannot shadow an unnamed registration.
  // The handle sequence advances with the selected generated name.
  auto const generatedName = std::wstring{L"UmbraObjectInstance-1"};
  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(generatedName));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  auto const soda = owner->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = owner->getAttributeHandle(soda, L"Flavor");
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(soda, AttributeHandleSet{flavor}));
  auto const generatedObject = owner->registerObjectInstance(soda);
  REQUIRE(owner->getObjectInstanceName(generatedObject) == L"UmbraObjectInstance-2");
  REQUIRE_NOTHROW(owner->releaseObjectInstanceName(generatedName));

  // Resignation returns reservations to the federation-wide pool.
  auto const resignedName = std::wstring{L"Umbra.ReleasedOnResign"};
  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(resignedName));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(peer->reserveObjectInstanceName(resignedName));
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.objectInstanceNameReservationSucceededReports.size() == 2);
  REQUIRE(peerReports.objectInstanceNameReservationSucceededReports.back().objectInstanceName ==
          resignedName);
  REQUIRE_NOTHROW(peer->releaseObjectInstanceName(resignedName));

  REQUIRE_NOTHROW(peer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(peer->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}
