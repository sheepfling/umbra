#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

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

  // The promoted receiver uses an active superclass subscription so it is
  // eligible for both discovery and the projected base-attribute reflections.
  // The cancelled receiver starts eligible so that its later unsubscribe
  // exercises callback-time suppression.
  REQUIRE_NOTHROW(exact->subscribeObjectClassAttributes(child, allOwned));
  REQUIRE_NOTHROW(promoted->subscribeObjectClassAttributes(base, baseAttributes, true));
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

  // Unpublishing the whole class removes the producer's ownership of every
  // corresponding instance attribute, so a later update is rejected at the
  // official AttributeNotOwned boundary rather than using stale state.
  REQUIRE_NOTHROW(publisher->unpublishObjectClass(child));
  REQUIRE_THROWS_AS(
      publisher->updateAttributeValues(objectInstance, attributeValues, tag),
      rti1516_2025::AttributeNotOwned);

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
    "Embedded partial unpublication revokes only the affected attribute update",
    "[integration][development-profile][declaration-management][object-management]"
    "[publication-ownership-update-boundary][2025]"
    "[rti.service.unpublish-object-class-attributes]"
    "[rti.service.is-attribute-owned-by-federate]"
    "[rti.service.update-attribute-values]"
    "[federate.callback.reflect-attribute-values]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador subscriberReports;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" / "tests" / "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const firstBytes[] = {0xA1};
  unsigned char const secondBytes[] = {0xB2};
  VariableLengthData const tag;

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"partial-unpublish-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"partial-unpublish-subscriber", L"subscriber", federationName));

  auto const child = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const first = publisher->getAttributeHandle(child, L"ReliableBaseA");
  auto const second = publisher->getAttributeHandle(child, L"ReliableBaseB");
  REQUIRE(child.isValid());
  REQUIRE(first.isValid());
  REQUIRE(second.isValid());
  REQUIRE(first != second);
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      child, AttributeHandleSet{first, second}));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(
      child, AttributeHandleSet{first, second}));
  auto const objectInstance = publisher->registerObjectInstance(child);
  REQUIRE(objectInstance.isValid());
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.objectDiscoveryReports.size() == 1U);

  AttributeHandleValueMap firstValue;
  firstValue.emplace(first, VariableLengthData(firstBytes, sizeof(firstBytes)));
  AttributeHandleValueMap secondValue;
  secondValue.emplace(
      second, VariableLengthData(secondBytes, sizeof(secondBytes)));
  REQUIRE(publisher->isAttributeOwnedByFederate(objectInstance, first));
  REQUIRE(publisher->isAttributeOwnedByFederate(objectInstance, second));
  REQUIRE_NOTHROW(
      publisher->updateAttributeValues(objectInstance, firstValue, tag));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.attributeReflectionReports.size() == 1U);
  REQUIRE(subscriberReports.attributeReflectionReports.front()
              .attributeValues.contains(first));

  // 2025 §5.3.3 allows unpublishing one class attribute, with ownership of
  // its corresponding instance attribute lost. The still-published attribute
  // is a positive control for both ownership and reflection.
  REQUIRE_NOTHROW(publisher->unpublishObjectClassAttributes(
      child, AttributeHandleSet{first}));
  REQUIRE_FALSE(publisher->isAttributeOwnedByFederate(objectInstance, first));
  REQUIRE(publisher->isAttributeOwnedByFederate(objectInstance, second));
  REQUIRE_THROWS_AS(
      publisher->updateAttributeValues(objectInstance, firstValue, tag),
      rti1516_2025::AttributeNotOwned);
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.attributeReflectionReports.size() == 1U);
  REQUIRE_NOTHROW(
      publisher->updateAttributeValues(objectInstance, secondValue, tag));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.attributeReflectionReports.size() == 2U);
  auto const& retainedReflection =
      subscriberReports.attributeReflectionReports.back();
  REQUIRE(retainedReflection.objectInstance == objectInstance);
  REQUIRE(retainedReflection.attributeValues.size() == 1U);
  REQUIRE_FALSE(retainedReflection.attributeValues.contains(first));
  REQUIRE(retainedReflection.attributeValues.contains(second));
  REQUIRE(variableLengthDataBytes(retainedReflection.attributeValues.at(second)) ==
          std::vector<unsigned char>(
              secondBytes, secondBytes + sizeof(secondBytes)));

  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subscriber->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded receive-order attribute update reflects only subscribed class attributes",
    "[integration][development-profile][declaration-management][object-management]"
    "[subscription-attribute-reflection-boundary][2025]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.update-attribute-values]"
    "[federate.callback.reflect-attribute-values]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador firstReports;
  ReportingFederateAmbassador secondReports;
  auto publisher = makeRti();
  auto firstReceiver = makeRti();
  auto secondReceiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" / "tests" / "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const firstBytes[] = {0xA1, 0xA2};
  unsigned char const secondBytes[] = {0xB1, 0xB2};
  VariableLengthData const tag;

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(firstReceiver->connect(firstReports, HLA_EVOKED));
  REQUIRE_NOTHROW(secondReceiver->connect(secondReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"attribute-filter-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(firstReceiver->joinFederationExecution(
      L"attribute-filter-first", L"subscriber", federationName));
  REQUIRE_NOTHROW(secondReceiver->joinFederationExecution(
      L"attribute-filter-second", L"subscriber", federationName));

  auto const child = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const first = publisher->getAttributeHandle(child, L"ReliableBaseA");
  auto const second = publisher->getAttributeHandle(child, L"ReliableBaseB");
  REQUIRE(child.isValid());
  REQUIRE(first.isValid());
  REQUIRE(second.isValid());
  REQUIRE(first != second);
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      child, AttributeHandleSet{first, second}));
  REQUIRE_NOTHROW(firstReceiver->subscribeObjectClassAttributes(
      child, AttributeHandleSet{first}));
  REQUIRE_NOTHROW(secondReceiver->subscribeObjectClassAttributes(
      child, AttributeHandleSet{second}));
  auto const objectInstance = publisher->registerObjectInstance(child);
  REQUIRE(objectInstance.isValid());
  REQUIRE_FALSE(firstReceiver->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(secondReceiver->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(firstReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(secondReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(firstReceiver->getKnownObjectClassHandle(objectInstance) == child);
  REQUIRE(secondReceiver->getKnownObjectClassHandle(objectInstance) == child);
  REQUIRE(firstReports.attributeReflectionReports.empty());
  REQUIRE(secondReports.attributeReflectionReports.empty());
  REQUIRE(publisher->isAttributeOwnedByFederate(objectInstance, first));
  REQUIRE(publisher->isAttributeOwnedByFederate(objectInstance, second));

  AttributeHandleValueMap values;
  values.emplace(first, VariableLengthData(firstBytes, sizeof(firstBytes)));
  values.emplace(second, VariableLengthData(secondBytes, sizeof(secondBytes)));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(objectInstance, values, tag));
  REQUIRE_FALSE(firstReceiver->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(secondReceiver->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(firstReports.attributeReflectionReports.size() == 1U);
  REQUIRE(secondReports.attributeReflectionReports.size() == 1U);
  auto const& firstReflection = firstReports.attributeReflectionReports.front();
  auto const& secondReflection = secondReports.attributeReflectionReports.front();
  REQUIRE(firstReflection.objectInstance == objectInstance);
  REQUIRE(secondReflection.objectInstance == objectInstance);
  REQUIRE(firstReflection.attributeValues.size() == 1U);
  REQUIRE(secondReflection.attributeValues.size() == 1U);
  REQUIRE(firstReflection.attributeValues.contains(first));
  REQUIRE_FALSE(firstReflection.attributeValues.contains(second));
  REQUIRE(secondReflection.attributeValues.contains(second));
  REQUIRE_FALSE(secondReflection.attributeValues.contains(first));
  REQUIRE(variableLengthDataBytes(firstReflection.attributeValues.at(first)) ==
          std::vector<unsigned char>(
              firstBytes, firstBytes + sizeof(firstBytes)));
  REQUIRE(variableLengthDataBytes(secondReflection.attributeValues.at(second)) ==
          std::vector<unsigned char>(
              secondBytes, secondBytes + sizeof(secondBytes)));
  REQUIRE(publisherReports.attributeReflectionReports.empty());

  REQUIRE_NOTHROW(secondReceiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(firstReceiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(secondReceiver->disconnect());
  REQUIRE_NOTHROW(firstReceiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

}  // namespace
