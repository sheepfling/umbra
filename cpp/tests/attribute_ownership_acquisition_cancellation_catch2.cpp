#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAvariableArray.h>

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The attribute-ownership acquisition cancellation test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::HLA_IMMEDIATE;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::ParameterHandle;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"attribute-ownership-acquisition-cancellation-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path testDataPath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / relativePath;
}

std::vector<unsigned char> variableLengthDataBytes(
    VariableLengthData const& value) {
  auto const* bytes = static_cast<unsigned char const*>(value.data());
  if (bytes == nullptr) {
    return {};
  }
  return {bytes, bytes + value.size()};
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct AttributeOwnershipAcquisitionReport final {
    enum class Kind {
      notification,
      unavailable,
    };

    Kind kind = Kind::unavailable;
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  struct AttributeOwnershipAcquisitionCancellationReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
  };

  struct AttributeOwnershipReleaseRequestReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  struct InteractionReport final {
    InteractionClassHandle interactionClass;
    ParameterHandleValueMap parameterValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const&,
      std::wstring const&,
      FederateHandle const&) override {
    objectDiscoveryReports.push_back(objectInstance);
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

  void confirmAttributeOwnershipAcquisitionCancellation(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes) override {
    attributeOwnershipAcquisitionCancellationReports.push_back({
        objectInstance,
        attributes,
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

  void removeObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      VariableLengthData const&,
      FederateHandle const&) override {
    objectRemovalReports.push_back(objectInstance);
  }

  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      RegionHandleSet const*) override {
    interactionReports.push_back({
        interactionClass,
        parameterValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
    });
  }

  std::vector<ObjectInstanceHandle> objectDiscoveryReports;
  std::vector<AttributeOwnershipAcquisitionReport>
      attributeOwnershipAcquisitionReports;
  std::vector<AttributeOwnershipAcquisitionCancellationReport>
      attributeOwnershipAcquisitionCancellationReports;
  std::vector<AttributeOwnershipReleaseRequestReport>
      attributeOwnershipReleaseRequestReports;
  std::vector<ObjectInstanceHandle> objectRemovalReports;
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
    "Embedded Cancel Attribute Ownership Acquisition honors the 2025 confirmation boundary",
    "[integration][development-profile][ownership-management]"
    "[rti.service.cancel-attribute-ownership-acquisition]"
    "[federate.callback.confirm-attribute-ownership-acquisition]") {
  rti1516_2025::NullFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = testDataPath(
      "attribute-update-passel-fom.xml").wstring();
  unsigned char const acquisitionTagBytes[] = {0xC0, 0xDE, 0x25};
  VariableLengthData const acquisitionTag(
      acquisitionTagBytes,
      sizeof(acquisitionTagBytes));
  ObjectInstanceHandle invalidObjectInstance;
  AttributeHandle invalidAttribute;
  AttributeHandleSet noAttributes;

  REQUIRE_THROWS_AS(
      unjoined->cancelAttributeOwnershipAcquisition(
          invalidObjectInstance,
          noAttributes),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->cancelAttributeOwnershipAcquisition(
          invalidObjectInstance,
          noAttributes),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"cancellation-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"cancellation-requester", L"publisher", federationName));

  auto const child = owner->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const reliableBaseA = owner->getAttributeHandle(
      child,
      fixture_hla::fixture::reliable_base_a);
  auto const unownedChild = owner->getAttributeHandle(
      child,
      fixture_hla::fixture::unowned_child);
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(unownedChild.isValid());
  AttributeHandleSet const requestedAttributes{reliableBaseA, unownedChild};

  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(
      child,
      requestedAttributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
      child,
      AttributeHandleSet{reliableBaseA}));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(
      child,
      requestedAttributes));

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
      requester->unpublishObjectClassAttributes(
          child,
          AttributeHandleSet{reliableBaseA}),
      rti1516_2025::OwnershipAcquisitionPending);
  REQUIRE_THROWS_AS(
      requester->unpublishObjectClassAttributes(
          child,
          AttributeHandleSet{unownedChild}),
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
  drainCallbacks(*owner);
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());
  drainCallbacks(*requester);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.size() == 1U);
  auto const& confirmation =
      requesterReports.attributeOwnershipAcquisitionCancellationReports.front();
  REQUIRE(confirmation.objectInstance == objectInstance);
  REQUIRE(confirmation.attributes == requestedAttributes);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(
      objectInstance,
      unownedChild));
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_NOTHROW(requester->unpublishObjectClassAttributes(
      child,
      requestedAttributes));
  REQUIRE_THROWS_AS(
      requester->cancelAttributeOwnershipAcquisition(
          objectInstance,
          requestedAttributes),
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
  drainCallbacks(*owner);
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());
  drainCallbacks(*requester);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.size() == 1U);
  REQUIRE(requesterReports.objectRemovalReports.size() == 1U);

  REQUIRE_NOTHROW(owner->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(requester->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}

TEST_CASE(
    "Embedded service reporting delivers Cancel Attribute Ownership Acquisition through MOM",
    "[integration][development-profile][federation-management][ownership-management]"
    "[mom][service-reporting][service-report-interaction]"
    "[cancel-attribute-ownership-acquisition-service-report-interaction]"
    "[rti.service.attribute-ownership-acquisition]"
    "[rti.service.cancel-attribute-ownership-acquisition]"
    "[rti.service.subscribe-interaction-class]"
    "[federate.callback.receive-interaction]"
    "[federate.callback.confirm-attribute-ownership-acquisition-cancellation]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  ReportingFederateAmbassador observerReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const ownershipFom = testDataPath("attribute-update-passel-fom.xml").wstring();
  auto const switchFom = testDataPath("switch-support-enabled-fom.xml").wstring();
  std::vector<std::wstring> const fomModules{ownershipFom, switchFom};
  unsigned char const acquisitionTagBytes[] = {0xC0, 0xDE, 0x25};
  VariableLengthData const acquisitionTag(
      acquisitionTagBytes,
      sizeof(acquisitionTagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, HLA_IMMEDIATE));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModules,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"cancellation-mom-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"cancellation-mom-requester", L"publisher", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"cancellation-mom-observer", L"observer", federationName));

  auto const reportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_service_invocation);
  REQUIRE(reportClass.isValid());
  std::vector<ParameterHandle> reportParameters;
  for (auto const& name : {
           standard_hla::mom::service,
           standard_hla::mom::service_type,
           standard_hla::mom::success_indicator,
           standard_hla::mom::supplied_arguments,
           standard_hla::mom::returned_argument,
           standard_hla::mom::exception,
           standard_hla::mom::serial_number}) {
    auto const parameter = observer->getParameterHandle(reportClass, name);
    REQUIRE(parameter.isValid());
    reportParameters.push_back(parameter);
  }
  REQUIRE_NOTHROW(observer->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(requester->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(requester->setSendServiceReportsToFileSwitch(false));

  auto const child = owner->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const ownedAttribute = owner->getAttributeHandle(
      child,
      fixture_hla::fixture::reliable_base_a);
  REQUIRE(child.isValid());
  REQUIRE(ownedAttribute.isValid());
  AttributeHandleSet const requestedAttributes{ownedAttribute};
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(
      child,
      requestedAttributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
      child,
      requestedAttributes));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(
      child,
      requestedAttributes));

  // The acquisition itself is accepted while service reporting is disabled,
  // leaving the owner's release request pending for cancellation.
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance,
      requestedAttributes,
      acquisitionTag));
  REQUIRE(observerReports.interactionReports.empty());
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());

  // Re-enable only the requesting federate. Cancellation is therefore the
  // first report (serial zero), while the observer receives it synchronously
  // and the confirmation remains queued on the requester.
  REQUIRE_NOTHROW(requester->setServiceReportingSwitch(true));
  REQUIRE(requester->getServiceReportingSwitch());
  REQUIRE_FALSE(requester->getSendServiceReportsToFileSwitch());
  REQUIRE_NOTHROW(requester->cancelAttributeOwnershipAcquisition(
      objectInstance,
      requestedAttributes));
  REQUIRE(observerReports.interactionReports.size() == 1U);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.empty());

  auto const& report = observerReports.interactionReports.front();
  REQUIRE(report.interactionClass == reportClass);
  REQUIRE(report.parameterValues.size() == 7U);
  REQUIRE(report.userSuppliedTag.size() == 0U);
  REQUIRE(report.transportationType ==
          observer->getTransportationTypeHandle(standard_hla::mom::reliable));
  REQUIRE_FALSE(report.producingFederate.isValid());

  rti1516_2025::HLAunicodeString service;
  REQUIRE_NOTHROW(service.decode(report.parameterValues.at(reportParameters[0])));
  REQUIRE(service.get() == L"CancelAttributeOwnershipAcquisition");
  rti1516_2025::HLAinteger16BE serviceType;
  REQUIRE_NOTHROW(serviceType.decode(report.parameterValues.at(reportParameters[1])));
  REQUIRE(serviceType.get() == 3);
  rti1516_2025::HLAboolean success;
  REQUIRE_NOTHROW(success.decode(report.parameterValues.at(reportParameters[2])));
  REQUIRE(success.get());

  rti1516_2025::HLAfixedRecord argumentPrototype;
  argumentPrototype.appendElement(rti1516_2025::HLAinteger32BE{})
      .appendElement(rti1516_2025::HLAunicodeString{})
      .appendElement(rti1516_2025::HLAunicodeString{});
  rti1516_2025::HLAvariableArray suppliedArguments{argumentPrototype};
  REQUIRE_NOTHROW(suppliedArguments.decode(
      report.parameterValues.at(reportParameters[3])));
  REQUIRE(suppliedArguments.size() == 2U);
  auto const& objectArgument = dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
      suppliedArguments.get(0U));
  REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(
              objectArgument.get(0U))
              .get() == 37);
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
              objectArgument.get(1U))
              .get() == L"Object instance designator");
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
              objectArgument.get(2U))
              .get() == L"\"" + objectInstance.toString() + L"\"");

  std::wstring attributeValues = L"[\"" + ownedAttribute.toString() + L"\"]";
  auto const& setArgument = dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
      suppliedArguments.get(1U));
  REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(setArgument.get(0U))
              .get() == 1);
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(setArgument.get(1U))
              .get() == L"Set of attribute designators");
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(setArgument.get(2U))
              .get() == attributeValues);

  rti1516_2025::HLAfixedRecord returnedArgument;
  returnedArgument.appendElement(rti1516_2025::HLAinteger32BE{})
      .appendElement(rti1516_2025::HLAunicodeString{})
      .appendElement(rti1516_2025::HLAunicodeString{});
  REQUIRE_NOTHROW(returnedArgument.decode(
      report.parameterValues.at(reportParameters[4])));
  REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(
              returnedArgument.get(0U))
              .get() == 34);
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
              returnedArgument.get(1U))
              .get()
              .empty());
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(
              returnedArgument.get(2U))
              .get() == L"null");

  rti1516_2025::HLAunicodeString exception;
  REQUIRE_NOTHROW(exception.decode(report.parameterValues.at(reportParameters[5])));
  REQUIRE(exception.get().empty());
  rti1516_2025::HLAinteger32BE serial;
  REQUIRE_NOTHROW(serial.decode(report.parameterValues.at(reportParameters[6])));
  REQUIRE(serial.get() == 0);

  // The owner-side release work item is cancelled at its callback boundary;
  // draining that queue completes the cancellation confirmation transaction.
  drainCallbacks(*owner);
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());
  drainCallbacks(*requester);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.size() == 1U);
  auto const& confirmation =
      requesterReports.attributeOwnershipAcquisitionCancellationReports.front();
  REQUIRE(confirmation.objectInstance == objectInstance);
  REQUIRE(confirmation.attributes == requestedAttributes);

  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(requester->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(observer->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
