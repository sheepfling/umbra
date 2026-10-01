#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded regional interaction subscriptions filter 2025 receive-order sends",
    "[integration][development-profile][interaction-management][ddm]"
    "[ddm-clause6-service-expansion]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.unsubscribe-interaction-class-with-regions]"
    "[rti.service.send-interaction-with-regions]"
    "[federate.callback.receive-interaction]"
    "[regional-interaction-subscription-service-report]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador subscriberReports;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, switchFom};
  auto reportDirectory = temporaryServiceReportDirectory();
  auto subscriberConfiguration = configurationForServiceReportDirectory(reportDirectory.path());
  subscriberConfiguration.withRtiAddress(L"in-process");
  unsigned char const parameterBytes[] = {0x2C, 0x01};
  unsigned char const tagBytes[] = {0x7A, 0x19};
  ParameterHandleValueMap parameterValues;
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberReports, HLA_EVOKED, subscriberConfiguration));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(
      publisherHandle = publisher->joinFederationExecution(
          L"regional-interaction-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"regional-interaction-subscriber", L"subscriber", federationName));
  REQUIRE_NOTHROW(subscriber->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(subscriber->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(subscriber->setConveyRegionDesignatorSetsSwitch(false));
  REQUIRE_FALSE(subscriber->getConveyRegionDesignatorSetsSwitch());
  auto const subscriberReportFiles = serviceReportFiles(reportDirectory.path());
  REQUIRE(subscriberReportFiles.size() == 1U);
  auto const subscriberReportFile = subscriberReportFiles.front();
  auto const subscriberInitialText = readTextFile(subscriberReportFile);

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const temperatureOk = publisher->getParameterHandle(interactionClass, fixture_hla::fixture::temperature_ok);
  auto const serverId = publisher->getDimensionHandle(fixture_hla::fixture::server_id);
  auto const barQuantity = publisher->getDimensionHandle(fixture_hla::fixture::bar_quantity);
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

  auto asAscii = [](std::wstring const& value) {
    std::string result;
    result.reserve(value.size());
    for (wchar_t const character : value) {
      REQUIRE(character >= L' ');
      REQUIRE(character <= L'~');
      result.push_back(static_cast<char>(character));
    }
    return result;
  };
  auto const interactionClassValue = asAscii(interactionClass.toString());
  auto const regionValue = asAscii(subscriberRegion.toString());

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

  // An explicitly supplied empty region set is a no-send operation.  It must
  // not fall back to the ordinary subscription that is still active here;
  // otherwise a regional invocation could leak through the invisible default
  // region and produce an induced callback.
  REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{},
      tag));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.interactionReports.size() == 1);

  // Remove only the non-region subscription. The committed regional
  // subscription then controls whether the send is eligible.
  REQUIRE_NOTHROW(subscriber->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(subscriber->setSendServiceReportsToFileSwitch(true));
  auto const regionalSubscribeRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"SubscribeInteractionClassWithRegions","HLAsuppliedArguments":[{"HLAargumentType":27,"HLAargumentName":"Interaction class designator","HLAargumentValue":")" +
      interactionClassValue +
      R"("},{"HLAargumentType":43,"HLAargumentName":"Set of region designators","HLAargumentValue":[")" +
      regionValue +
      R"("]},{"HLAargumentType":6,"HLAargumentName":"Optional passive subscription indicator","HLAargumentValue":false}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{subscriberRegion}));
  REQUIRE(readTextFile(subscriberReportFile) == subscriberInitialText + regionalSubscribeRecord);
  REQUIRE_NOTHROW(subscriber->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(subscriber->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{publisherRegion},
      tag));
  static_cast<void>(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.interactionReports.size() == 2);
  REQUIRE_FALSE(subscriberReports.interactionReports.back().sentRegionsSupplied);

  REQUIRE_NOTHROW(subscriber->setConveyRegionDesignatorSetsSwitch(true));
  REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{publisherRegion},
      tag));
  static_cast<void>(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.interactionReports.size() == 3);
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
  REQUIRE(subscriberReports.interactionReports.size() == 3);

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
  REQUIRE(subscriberReports.interactionReports.size() == 4);
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
  REQUIRE(subscriberReports.interactionReports.size() == 4);
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
  REQUIRE_NOTHROW(subscriber->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(subscriber->setSendServiceReportsToFileSwitch(true));
  auto const regionalUnsubscribeRecord = std::string{
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"UnsubscribeInteractionClassWithRegions","HLAsuppliedArguments":[{"HLAargumentType":27,"HLAargumentName":"Interaction class designator","HLAargumentValue":")" +
      interactionClassValue +
      R"("},{"HLAargumentType":43,"HLAargumentName":"Set of region designators","HLAargumentValue":[")" +
      regionValue +
      R"("]}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{subscriberRegion}));
  REQUIRE(readTextFile(subscriberReportFile) ==
          subscriberInitialText + regionalSubscribeRecord + regionalUnsubscribeRecord);
  REQUIRE_NOTHROW(subscriber->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(subscriber->setSendServiceReportsToFileSwitch(false));
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
