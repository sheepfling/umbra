#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded asynchronous delivery gates regional receive-order interactions",
    "[integration][development-profile][interaction-management][ddm][time-management]"
    "[asynchronous-delivery][regional-interaction][callbacks]"
    "[rti.service.enable-asynchronous-delivery][rti.service.disable-asynchronous-delivery]"
    "[rti.service.send-interaction-with-regions]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.enable-time-constrained]"
    "[federate.callback.receive-interaction][ddm-regional-asynchronous-receive-order][2025]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const parameterBytes[] = {0xA5, 0x53};
  unsigned char const firstTagBytes[] = {0x41, 0x53, 0x59, 0x31};
  unsigned char const secondTagBytes[] = {0x41, 0x53, 0x59, 0x32};
  ParameterHandleValueMap parameters;
  VariableLengthData const firstTag(firstTagBytes, sizeof(firstTagBytes));
  VariableLengthData const secondTag(secondTagBytes, sizeof(secondTagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"asynchronous-regional-interaction-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"asynchronous-regional-interaction-receiver", L"subscriber", federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const temperatureOk = publisher->getParameterHandle(interactionClass, fixture_hla::fixture::temperature_ok);
  auto const serverId = publisher->getDimensionHandle(fixture_hla::fixture::server_id);
  REQUIRE(interactionClass.isValid());
  REQUIRE(temperatureOk.isValid());
  REQUIRE(serverId.isValid());
  parameters.emplace(
      temperatureOk,
      VariableLengthData(parameterBytes, sizeof(parameterBytes)));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));

  auto const sourceRegion = publisher->createRegion(DimensionHandleSet{serverId});
  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      sourceRegion,
      serverId,
      RangeBounds(8UL, 9UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{sourceRegion}));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      serverId,
      RangeBounds(8UL, 9UL)));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));

  // A time-constrained federate starts with asynchronous delivery disabled.
  // The overlap-qualified receive-order callback must therefore remain behind
  // the federate's time-advance boundary even though the source send itself is
  // immediate and the region metadata is already committed.
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1);

  REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
      interactionClass,
      parameters,
      RegionHandleSet{sourceRegion},
      firstTag));
  REQUIRE(receiverReports.interactionReports.empty());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));

  // Enabling the switch makes the deferred receive-order callback eligible;
  // HLA_EVOKED still requires one explicit callback pump at the public seam.
  REQUIRE_NOTHROW(receiver->enableAsynchronousDelivery());
  REQUIRE(receiverReports.interactionReports.empty());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.interactionReports.size() == 1);
  auto const& firstReport = receiverReports.interactionReports.front();
  REQUIRE(firstReport.interactionClass == interactionClass);
  REQUIRE(firstReport.parameterValues.size() == 1);
  REQUIRE(firstReport.parameterValues.contains(temperatureOk));
  REQUIRE(firstReport.producingFederate == publisherHandle);
  REQUIRE(firstReport.sentRegionsSupplied);
  REQUIRE(firstReport.sentRegions == RegionHandleSet{sourceRegion});
  REQUIRE(variableLengthDataBytes(firstReport.userSuppliedTag) ==
          std::vector<unsigned char>(firstTagBytes, firstTagBytes + sizeof(firstTagBytes)));

  // Disabling returns the same recipient to the time-advance gate. Re-enable
  // must release the second regional callback without changing its source
  // region, target, or user tag.
  REQUIRE_NOTHROW(receiver->disableAsynchronousDelivery());
  REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
      interactionClass,
      parameters,
      RegionHandleSet{sourceRegion},
      secondTag));
  REQUIRE(receiverReports.interactionReports.size() == 1);
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(receiver->enableAsynchronousDelivery());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.interactionReports.size() == 2);
  auto const& secondReport = receiverReports.interactionReports.back();
  REQUIRE(secondReport.producingFederate == publisherHandle);
  REQUIRE(secondReport.sentRegionsSupplied);
  REQUIRE(secondReport.sentRegions == RegionHandleSet{sourceRegion});
  REQUIRE(variableLengthDataBytes(secondReport.userSuppliedTag) ==
          std::vector<unsigned char>(secondTagBytes, secondTagBytes + sizeof(secondTagBytes)));

  REQUIRE_NOTHROW(receiver->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
