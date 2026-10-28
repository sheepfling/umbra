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
#error "The object-instance deletion test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::NO_ACTION;
using rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"object-instance-deletion-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / relativePath;
}

std::vector<unsigned char> bytes(VariableLengthData const& value) {
  auto const* data = static_cast<unsigned char const*>(value.data());
  if (data == nullptr || value.size() == 0U) {
    return {};
  }
  return {data, data + value.size()};
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct ObjectDiscoveryReport final {
    ObjectInstanceHandle objectInstance;
    ObjectClassHandle objectClass;
    std::wstring objectInstanceName;
    FederateHandle producingFederate;
  };

  struct ObjectRemovalReport final {
    ObjectInstanceHandle objectInstance;
    VariableLengthData userSuppliedTag;
    FederateHandle producingFederate;
  };

  struct OwnershipAcquisitionReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
  };

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

  void attributeOwnershipAcquisitionNotification(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      VariableLengthData const&) override {
    ownershipAcquisitionReports.push_back({objectInstance, attributes});
  }

  std::vector<ObjectDiscoveryReport> objectDiscoveryReports;
  std::vector<ObjectRemovalReport> objectRemovalReports;
  std::vector<OwnershipAcquisitionReport> ownershipAcquisitionReports;
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
    "Embedded receive-order Delete Object Instance honors 2025 removal lifecycle",
    "[integration][development-profile][object-management][callbacks]"
    "[object-instance-deletion]"
    "[rti.service.delete-object-instance]"
    "[federate.callback.remove-object-instance]") {
  rti1516_2025::NullFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto unjoined = makeRti();
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  ObjectInstanceHandle invalidObjectInstance;

  REQUIRE_THROWS_AS(
      publisher->deleteObjectInstance(invalidObjectInstance, VariableLengthData()),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->deleteObjectInstance(invalidObjectInstance, VariableLengthData()),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"object-deletion-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"object-deletion-receiver", L"receiver", federationName));

  auto const server = publisher->getObjectClassHandle(
      fixture_hla::fom::employee_server);
  auto const efficiency = publisher->getAttributeHandle(
      server,
      fixture_hla::fixture::efficiency);
  auto const privilegeToDelete = publisher->getAttributeHandle(
      server, umbra::test::hla::wide::standard::privilege_to_delete_object);
  REQUIRE(server.isValid());
  REQUIRE(efficiency.isValid());
  REQUIRE(privilegeToDelete.isValid());
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle const objectInstance = publisher->registerObjectInstance(server);
  REQUIRE(receiverReports.objectDiscoveryReports.empty());
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(receiverReports.objectDiscoveryReports.front().objectInstance == objectInstance);
  REQUIRE(receiver->getKnownObjectClassHandle(objectInstance) == server);
  REQUIRE(publisher->getKnownObjectClassHandle(objectInstance) == server);

  unsigned char const tagBytes[] = {'d', 'e', 'l'};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  ObjectInstanceHandle const unknownObject;
  REQUIRE_THROWS_AS(
      publisher->deleteObjectInstance(unknownObject, tag),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE(publisher->isAttributeOwnedByFederate(
      objectInstance, privilegeToDelete));
  REQUIRE_FALSE(receiver->isAttributeOwnedByFederate(
      objectInstance, privilegeToDelete));
  REQUIRE_THROWS_AS(
      receiver->deleteObjectInstance(objectInstance, tag),
      rti1516_2025::DeletePrivilegeNotHeld);

  REQUIRE(receiverReports.objectRemovalReports.empty());
  REQUIRE(publisherReports.objectRemovalReports.empty());
  REQUIRE_NOTHROW(publisher->deleteObjectInstance(objectInstance, tag));

  // The invoker becomes unknown before the public call returns; recipients
  // remain known until their queued Remove Object Instance callback starts.
  REQUIRE_THROWS_AS(
      publisher->getKnownObjectClassHandle(objectInstance),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE(receiver->getKnownObjectClassHandle(objectInstance) == server);
  REQUIRE(receiverReports.objectRemovalReports.empty());
  REQUIRE(publisherReports.objectRemovalReports.empty());

  drainCallbacks(*receiver);
  REQUIRE(receiverReports.objectRemovalReports.size() == 1U);
  auto const& removal = receiverReports.objectRemovalReports.front();
  REQUIRE(removal.objectInstance == objectInstance);
  REQUIRE(bytes(removal.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(removal.producingFederate == publisherHandle);
  REQUIRE_THROWS_AS(
      receiver->getKnownObjectClassHandle(objectInstance),
      rti1516_2025::ObjectInstanceNotKnown);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded HLAprivilegeToDeleteObject transfers the right to delete",
    "[integration][development-profile][ownership-management]"
    "[object-management][privilege-to-delete-transfer]"
    "[rti.service.attribute-ownership-acquisition]"
    "[rti.service.attribute-ownership-divestiture-if-wanted]"
    "[ownership.hla-privilege-delete-ordinary-ownership-services]"
    "[clause-7.1.3]"
    "[rti.service.delete-object-instance]"
    "[federate.callback.attribute-ownership-acquisition-notification]"
    "[federate.callback.remove-object-instance]") {
  ReportingFederateAmbassador oldOwnerReports;
  ReportingFederateAmbassador newOwnerReports;
  ReportingFederateAmbassador observerReports;
  auto oldOwner = makeRti();
  auto newOwner = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(oldOwner->connect(oldOwnerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(newOwner->connect(newOwnerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(oldOwner->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle oldOwnerHandle;
  REQUIRE_NOTHROW(oldOwnerHandle = oldOwner->joinFederationExecution(
      L"delete-privilege-old-owner", L"publisher", federationName));
  FederateHandle newOwnerHandle;
  REQUIRE_NOTHROW(newOwnerHandle = newOwner->joinFederationExecution(
      L"delete-privilege-new-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"delete-privilege-observer", L"subscriber", federationName));
  REQUIRE(oldOwnerHandle.isValid());
  REQUIRE(newOwnerHandle.isValid());

  auto const server = oldOwner->getObjectClassHandle(
      fixture_hla::fom::employee_server);
  auto const efficiency = oldOwner->getAttributeHandle(
      server, fixture_hla::fixture::efficiency);
  auto const privilegeToDelete = oldOwner->getAttributeHandle(
      server, umbra::test::hla::wide::standard::privilege_to_delete_object);
  REQUIRE(server.isValid());
  REQUIRE(efficiency.isValid());
  REQUIRE(privilegeToDelete.isValid());
  AttributeHandleSet const observedAttributes{efficiency};
  AttributeHandleSet const transferredAttributes{
      efficiency, privilegeToDelete};
  REQUIRE_NOTHROW(oldOwner->publishObjectClassAttributes(
      server, observedAttributes));
  REQUIRE_NOTHROW(newOwner->subscribeObjectClassAttributes(
      server, observedAttributes));
  REQUIRE_NOTHROW(observer->subscribeObjectClassAttributes(
      server, observedAttributes));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = oldOwner->registerObjectInstance(server));
  drainCallbacks(*newOwner);
  drainCallbacks(*observer);
  REQUIRE(newOwnerReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(newOwnerReports.objectDiscoveryReports.front().objectInstance ==
          objectInstance);
  REQUIRE(observerReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(observerReports.objectDiscoveryReports.front().objectInstance ==
          objectInstance);
  REQUIRE(oldOwner->isAttributeOwnedByFederate(
      objectInstance, privilegeToDelete));
  REQUIRE_FALSE(newOwner->isAttributeOwnedByFederate(
      objectInstance, privilegeToDelete));

  // The acquirer must publish the standard class attribute at its known class.
  REQUIRE_NOTHROW(newOwner->publishObjectClassAttributes(
      server, transferredAttributes));
  unsigned char const acquisitionTagBytes[] = {0xA5, 0x25};
  unsigned char const divestitureTagBytes[] = {0xD1, 0x25};
  VariableLengthData const acquisitionTag(
      acquisitionTagBytes, sizeof(acquisitionTagBytes));
  VariableLengthData const divestitureTag(
      divestitureTagBytes, sizeof(divestitureTagBytes));
  REQUIRE_NOTHROW(newOwner->attributeOwnershipAcquisition(
      objectInstance, transferredAttributes, acquisitionTag));
  AttributeHandleSet divestedAttributes;
  REQUIRE_NOTHROW(oldOwner->attributeOwnershipDivestitureIfWanted(
      objectInstance, transferredAttributes, divestitureTag,
      divestedAttributes));
  REQUIRE(divestedAttributes == transferredAttributes);
  drainCallbacks(*newOwner);
  REQUIRE(newOwnerReports.ownershipAcquisitionReports.size() == 1U);
  REQUIRE(newOwnerReports.ownershipAcquisitionReports.front().objectInstance ==
          objectInstance);
  REQUIRE(newOwnerReports.ownershipAcquisitionReports.front().attributes ==
          transferredAttributes);
  REQUIRE_FALSE(oldOwner->isAttributeOwnedByFederate(
      objectInstance, efficiency));
  REQUIRE(newOwner->isAttributeOwnedByFederate(objectInstance, efficiency));
  REQUIRE_FALSE(oldOwner->isAttributeOwnedByFederate(
      objectInstance, privilegeToDelete));
  REQUIRE(newOwner->isAttributeOwnedByFederate(
      objectInstance, privilegeToDelete));

  // The ordinary ownership service path transfers both an application
  // attribute and HLAprivilegeToDeleteObject. Only the new owner may now
  // provide values for the ordinary attribute.
  unsigned char const efficiencyValueBytes[] = {0x25, 0x07};
  AttributeHandleValueMap efficiencyValue;
  efficiencyValue.emplace(
      efficiency,
      VariableLengthData(efficiencyValueBytes, sizeof(efficiencyValueBytes)));
  REQUIRE_THROWS_AS(
      oldOwner->updateAttributeValues(
          objectInstance, efficiencyValue, acquisitionTag),
      rti1516_2025::AttributeNotOwned);
  REQUIRE_NOTHROW(newOwner->updateAttributeValues(
      objectInstance, efficiencyValue, acquisitionTag));

  unsigned char const deletionTagBytes[] = {0xD3, 0x1E, 0x25};
  VariableLengthData const deletionTag(
      deletionTagBytes, sizeof(deletionTagBytes));
  REQUIRE_THROWS_AS(
      oldOwner->deleteObjectInstance(objectInstance, deletionTag),
      rti1516_2025::DeletePrivilegeNotHeld);
  REQUIRE(observerReports.objectRemovalReports.empty());
  REQUIRE_NOTHROW(newOwner->deleteObjectInstance(objectInstance, deletionTag));
  drainCallbacks(*observer);
  REQUIRE(observerReports.objectRemovalReports.size() == 1U);
  auto const& removal = observerReports.objectRemovalReports.front();
  REQUIRE(removal.objectInstance == objectInstance);
  // Section 6.17 identifies the registrant, not the later deleting federate.
  REQUIRE(removal.producingFederate == oldOwnerHandle);
  REQUIRE_FALSE(removal.producingFederate == newOwnerHandle);
  REQUIRE(bytes(removal.userSuppliedTag) ==
          std::vector<unsigned char>(
              deletionTagBytes, deletionTagBytes + sizeof(deletionTagBytes)));

  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(newOwner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(oldOwner->resignFederationExecution(
      CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(oldOwner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(newOwner->disconnect());
  REQUIRE_NOTHROW(oldOwner->disconnect());
}

TEST_CASE(
    "Embedded delete privilege acquisition requires publication at the known class",
    "[integration][development-profile][ownership-management]"
    "[privilege-to-delete-publication-eligibility]"
    "[rti.service.attribute-ownership-acquisition]"
    "[rti.service.attribute-ownership-divestiture-if-wanted]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador acquirerReports;
  auto owner = makeRti();
  auto acquirer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(acquirer->connect(acquirerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"delete-privilege-eligibility-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(acquirer->joinFederationExecution(
      L"delete-privilege-eligibility-acquirer", L"publisher", federationName));

  auto const server = owner->getObjectClassHandle(
      fixture_hla::fom::employee_server);
  auto const efficiency = owner->getAttributeHandle(
      server, fixture_hla::fixture::efficiency);
  auto const privilegeToDelete = owner->getAttributeHandle(
      server, umbra::test::hla::wide::standard::privilege_to_delete_object);
  REQUIRE(server.isValid());
  REQUIRE(efficiency.isValid());
  REQUIRE(privilegeToDelete.isValid());
  AttributeHandleSet const efficiencyOnly{efficiency};
  AttributeHandleSet const privilegeOnly{privilegeToDelete};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(acquirer->subscribeObjectClassAttributes(server, efficiencyOnly));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(server));
  drainCallbacks(*acquirer);
  REQUIRE(acquirerReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(acquirerReports.objectDiscoveryReports.front().objectInstance ==
          objectInstance);
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, privilegeToDelete));
  REQUIRE_FALSE(acquirer->isAttributeOwnedByFederate(
      objectInstance, privilegeToDelete));

  // Publishing the class through another attribute implicitly publishes the
  // delete privilege in 2025. Explicitly unpublish that privilege while the
  // class stays published to exercise the actual eligibility boundary.
  REQUIRE_NOTHROW(acquirer->publishObjectClassAttributes(
      server, efficiencyOnly));
  REQUIRE_NOTHROW(acquirer->unpublishObjectClassAttributes(
      server, privilegeOnly));
  unsigned char const acquisitionTagBytes[] = {0xE1, 0x25};
  VariableLengthData const acquisitionTag(
      acquisitionTagBytes, sizeof(acquisitionTagBytes));
  REQUIRE_THROWS_AS(
      acquirer->attributeOwnershipAcquisition(
          objectInstance, privilegeOnly, acquisitionTag),
      rti1516_2025::AttributeNotPublished);
  drainCallbacks(*acquirer);
  REQUIRE(acquirerReports.ownershipAcquisitionReports.empty());
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, privilegeToDelete));
  REQUIRE_FALSE(acquirer->isAttributeOwnedByFederate(
      objectInstance, privilegeToDelete));

  // The same joined federate becomes eligible when it publishes this class
  // attribute at the object's known class; a real ownership transfer follows.
  REQUIRE_NOTHROW(acquirer->publishObjectClassAttributes(
      server, privilegeOnly));
  REQUIRE_NOTHROW(acquirer->attributeOwnershipAcquisition(
      objectInstance, privilegeOnly, acquisitionTag));
  AttributeHandleSet divestedAttributes;
  VariableLengthData const divestitureTag;
  REQUIRE_NOTHROW(owner->attributeOwnershipDivestitureIfWanted(
      objectInstance, privilegeOnly, divestitureTag, divestedAttributes));
  REQUIRE(divestedAttributes == privilegeOnly);
  drainCallbacks(*acquirer);
  REQUIRE(acquirerReports.ownershipAcquisitionReports.size() == 1U);
  REQUIRE(acquirerReports.ownershipAcquisitionReports.front().objectInstance ==
          objectInstance);
  REQUIRE(acquirerReports.ownershipAcquisitionReports.front().attributes ==
          privilegeOnly);
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(
      objectInstance, privilegeToDelete));
  REQUIRE(acquirer->isAttributeOwnedByFederate(
      objectInstance, privilegeToDelete));

  VariableLengthData const deletionTag;
  REQUIRE_NOTHROW(acquirer->deleteObjectInstance(objectInstance, deletionTag));
  REQUIRE_NOTHROW(acquirer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(
      CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(acquirer->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
