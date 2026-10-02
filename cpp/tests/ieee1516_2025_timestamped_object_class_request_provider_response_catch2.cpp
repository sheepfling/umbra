#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded object-class Request Attribute Value Update supports a timestamped provider response under HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][object-management][time-management]"
    "[object-class-request-provider-response][tso][callback-immediate]"
    "[rti.service.request-attribute-value-update]"
    "[rti.service.update-attribute-values]"
    "[rti.service.retract]"
    "[rti.service.change-default-attribute-order-type]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request]"
    "[federate.callback.provide-attribute-value-update]"
    "[federate.callback.reflect-attribute-values]"
    "[federate.callback.time-constrained-enabled]"
    "[federate.callback.time-advance-grant]") {
  auto runScenario = [](auto const callbackModel) {
    ReportingFederateAmbassador ownerReports;
    ReportingFederateAmbassador requesterReports;
    auto owner = makeRti();
    auto requester = makeRti();
    auto const federationName = nextFederationName();
    auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
    bool const immediate = callbackModel == rti1516_2025::HLA_IMMEDIATE;
    std::vector<unsigned char> const requestTagBytes{0x43U, 0x4CU, 0x53U};
    std::vector<unsigned char> const responseTagBytes{
        0x43U, 0x4C, 0x53U, 0x2DU, 0x52U};
    std::vector<unsigned char> const responseValueBytes{0x6AU, 0x25U, 0x02U};
    VariableLengthData const requestTag(
        requestTagBytes.data(), requestTagBytes.size());
    VariableLengthData const responseTag(
        responseTagBytes.data(), responseTagBytes.size());
    VariableLengthData const responseValue(
        responseValueBytes.data(), responseValueBytes.size());
    rti1516_2025::MessageRetractionHandle responseRetraction;

    auto const drain = [](RTIambassador& ambassador) {
      for (int pass = 0; pass != 32; ++pass) {
        static_cast<void>(ambassador.evokeCallback(0.0));
      }
    };

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
    REQUIRE_NOTHROW(requester->connect(requesterReports, callbackModel));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName, fomModule, standard_hla::mom::integer64_time));
    FederateHandle ownerHandle;
    REQUIRE_NOTHROW(ownerHandle = owner->joinFederationExecution(
        L"class-response-owner", L"publisher", federationName));
    REQUIRE_NOTHROW(requester->joinFederationExecution(
        L"class-response-requester", L"subscriber", federationName));

    auto const soda = owner->getObjectClassHandle(
        fixture_hla::fom::food_drink_soda);
    auto const flavor = owner->getAttributeHandle(
        soda, fixture_hla::fixture::flavor);
    REQUIRE(soda.isValid());
    REQUIRE(flavor.isValid());
    AttributeHandleSet const flavorOnly{flavor};
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(soda, flavorOnly));
    REQUIRE_NOTHROW(owner->changeDefaultAttributeOrderType(
        soda, flavorOnly, TIMESTAMP));
    REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(
        soda, flavorOnly));

    ObjectInstanceHandle objectInstance;
    REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(soda));
    drain(*requester);
    REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(requester->getKnownObjectClassHandle(objectInstance) == soda);

    // RestaurantFOMmodule-2025 enables automatic provision. Drain that setup
    // callback before installing the explicit class-request response handler.
    drain(*owner);
    auto const initialProvideReportCount =
        ownerReports.attributeValueUpdateRequestReports.size();
    std::vector<unsigned char> observedRequestTag;
    ownerReports.provideAttributeValueUpdateHandler =
        [&owner,
         &observedRequestTag,
         &responseRetraction,
         responseValue,
         responseTag](
            ObjectInstanceHandle const& callbackObject,
            AttributeHandleSet const& callbackAttributes,
            VariableLengthData const& callbackTag) {
          observedRequestTag = variableLengthDataBytes(callbackTag);
          AttributeHandleValueMap values;
          for (AttributeHandle const& attribute : callbackAttributes) {
            values.emplace(attribute, responseValue);
          }
          responseRetraction = owner->updateAttributeValues(
              callbackObject,
              values,
              responseTag,
              rti1516_2025::HLAinteger64Time(2));
        };

    REQUIRE_NOTHROW(requester->enableTimeConstrained());
    if (!immediate) {
      drain(*requester);
    }
    REQUIRE(requesterReports.timeConstrainedEnabledReports.size() == 1U);
    REQUIRE_NOTHROW(owner->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    if (!immediate) {
      drain(*owner);
    }
    REQUIRE(ownerReports.timeRegulationEnabledReports.size() == 1U);

    REQUIRE_NOTHROW(requester->requestAttributeValueUpdate(
        soda, flavorOnly, requestTag));
    if (!immediate) {
      // HLA_EVOKED exposes the provider callback through Evoke; the request
      // itself must not synchronously run the callback.
      REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() ==
              initialProvideReportCount);
      drain(*owner);
    }
    REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() ==
            initialProvideReportCount + 1U);
    REQUIRE(responseRetraction.isValid());
    REQUIRE(observedRequestTag == requestTagBytes);
    REQUIRE(requesterReports.attributeReflectionReports.empty());

    // The timestamped provider response remains queued until the constrained
    // requester reaches the matching grant. The class request itself expands
    // over the registered subclass instance and preserves its default order.
    REQUIRE_NOTHROW(requester->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(2)));
    REQUIRE_NOTHROW(owner->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(2)));
    drain(*owner);
    drain(*requester);
    REQUIRE(requesterReports.attributeReflectionReports.size() == 1U);
    REQUIRE(requesterReports.timeAdvanceGrantReports.size() == 1U);
    REQUIRE(requesterReports.callbackOrder ==
            std::vector<std::string>{"reflect", "grant"});

    auto const& reflection = requesterReports.attributeReflectionReports.front();
    REQUIRE(reflection.objectInstance == objectInstance);
    REQUIRE(reflection.attributeValues.size() == 1U);
    REQUIRE(reflection.attributeValues.contains(flavor));
    REQUIRE(variableLengthDataBytes(
                reflection.attributeValues.at(flavor)) == responseValueBytes);
    REQUIRE(variableLengthDataBytes(reflection.userSuppliedTag) ==
            responseTagBytes);
    REQUIRE(reflection.transportationType ==
            owner->getTransportationTypeHandle(L"HLAreliable"));
    REQUIRE(reflection.producingFederate == ownerHandle);
    REQUIRE_FALSE(reflection.sentRegionsSupplied);
    REQUIRE(reflection.timeValue == L"2");
    REQUIRE(reflection.sentOrderType == TIMESTAMP);
    REQUIRE(reflection.receivedOrderType == TIMESTAMP);
    REQUIRE(reflection.retractionSupplied);
    REQUIRE(reflection.retractionValid);
    REQUIRE_THROWS_AS(
        owner->retract(responseRetraction),
        rti1516_2025::MessageCanNoLongerBeRetracted);

    REQUIRE_NOTHROW(requester->unsubscribeObjectClassAttributes(
        soda, flavorOnly));
    REQUIRE_NOTHROW(owner->unpublishObjectClassAttributes(
        soda, flavorOnly));
    REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(owner->disconnect());
    REQUIRE_NOTHROW(requester->disconnect());
  };

  SECTION("HLA_EVOKED") {
    runScenario(rti1516_2025::HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(rti1516_2025::HLA_IMMEDIATE);
  }
}

}
