#include "ieee1516_2025_federation_management_fixture_support.hpp"

#include <RTI/encoding/BasicDataElements.h>

namespace {

std::filesystem::path categoryCInteractionFom() {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "packages" /
      "hla-rti-cpp-tck" / "fom" / "mom-category-c-interaction-tck.xml";
}

}  // namespace

TEST_CASE(
    "Embedded RTI routes an uncategorized MOM interaction as ordinary federate traffic",
    "[integration][development-profile][federation-management][mom]"
    "[mom-category-c-ordinary-interaction][callback-evoked]"
    "[rti.service.create-federation-execution-with-mim]"
    "[rti.service.get-service-reporting-switch]"
    "[rti.service.send-interaction][federate.callback.receive-interaction]"
    "[2025]") {
  FederationEventFederateAmbassador publisherCallbacks;
  ReportingFederateAmbassador observerCallbacks;
  auto publisher = makeRti();
  auto observer = makeRti();
  auto const federation = nextFederationName();
  auto const restaurantFom = resourcePath(
      "examples/RestaurantFOMmodule-2025.xml");
  auto const categoryCFom = categoryCInteractionFom();
  auto const mimFom = resourcePath("mim/HLAstandardMIM-2025.xml");

  REQUIRE_NOTHROW(publisher->connect(publisherCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{restaurantFom.wstring(), categoryCFom.wstring()},
      mimFom.wstring(),
      standard_hla::mom::integer64_time));
  auto const publisherFederate = publisher->joinFederationExecution(
      L"mom-category-c-publisher", L"publisher", federation);
  auto const observerFederate = observer->joinFederationExecution(
      L"mom-category-c-observer", L"observer", federation);
  REQUIRE(publisherFederate.isValid());
  REQUIRE(observerFederate.isValid());

  auto const publisherClass = publisher->getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.TckCategoryCInteraction");
  auto const observerClass = observer->getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.TckCategoryCInteraction");
  auto const publisherParameter = publisher->getParameterHandle(
      publisherClass, L"HLAserviceReporting");
  auto const observerParameter = observer->getParameterHandle(
      observerClass, L"HLAserviceReporting");
  REQUIRE(publisherClass.isValid());
  REQUIRE(observerClass.isValid());
  REQUIRE(publisherParameter.isValid());
  REQUIRE(observerParameter.isValid());

  REQUIRE_FALSE(publisher->getServiceReportingSwitch());
  REQUIRE_NOTHROW(publisher->publishInteractionClass(publisherClass));
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(observerClass));
  auto const switchShapedValue = rti1516_2025::HLAinteger32BE{1}.encode();
  REQUIRE_NOTHROW(publisher->sendInteraction(
      publisherClass,
      rti1516_2025::ParameterHandleValueMap{
          {publisherParameter, switchShapedValue}},
      rti1516_2025::VariableLengthData{}));
  while (observer->evokeCallback(0.0)) {
  }

  REQUIRE(observerCallbacks.interactionReports.size() == 1U);
  auto const& received = observerCallbacks.interactionReports.front();
  REQUIRE(received.interactionClass == observerClass);
  REQUIRE(received.producingFederate == publisherFederate);
  REQUIRE(received.parameterValues.size() == 1U);
  REQUIRE(received.parameterValues.count(observerParameter) == 1U);
  rti1516_2025::HLAinteger32BE decodedSwitchShapedValue;
  decodedSwitchShapedValue.decode(
      received.parameterValues.at(observerParameter));
  REQUIRE(decodedSwitchShapedValue.get() == 1);

  // This FOM-added class is outside the predefined MOM control branches;
  // its switch-named parameter must not change the publisher's RTI switch.
  REQUIRE_FALSE(publisher->getServiceReportingSwitch());
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federation));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
