#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded MOM HLAsetTiming drives joined-federate periodic values",
    "[integration][development-profile][federation-management][mom][periodic-mom]"
    "[rti.service.send-interaction]"
    "[federate.callback.reflect-attribute-values][hla-set-timing-periodic-values]") {
  ReportingFederateAmbassador subjectReports;
  ReportingFederateAmbassador observerReports;
  auto subject = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(subject->connect(subjectReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      subject->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"mom-periodic-observer", L"observer", federationName));

  auto const momClass = observer->getObjectClassHandle(
      standard_hla::mom::federate_object_class);
  auto const federateHandleAttribute = observer->getAttributeHandle(
      momClass, standard_hla::mom::federate_handle);
  auto const logicalTimeAttribute = observer->getAttributeHandle(
      momClass, standard_hla::mom::logical_time);
  auto const lookaheadAttribute = observer->getAttributeHandle(
      momClass, standard_hla::mom::lookahead);
  REQUIRE(momClass.isValid());
  REQUIRE(federateHandleAttribute.isValid());
  REQUIRE(logicalTimeAttribute.isValid());
  REQUIRE(lookaheadAttribute.isValid());
  REQUIRE_NOTHROW(observer->subscribeObjectClassAttributes(
      momClass,
      AttributeHandleSet{
          federateHandleAttribute,
          logicalTimeAttribute,
          lookaheadAttribute},
      true));

  auto const subjectFederate = subject->joinFederationExecution(
      L"mom-periodic-subject", L"subject", federationName);
  while (observer->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  auto const reflectedSubject = std::find_if(
      observerReports.attributeReflectionReports.begin(),
      observerReports.attributeReflectionReports.end(),
      [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
        auto const value = report.attributeValues.find(federateHandleAttribute);
        return value != report.attributeValues.end() &&
            variableLengthDataBytes(value->second) ==
                variableLengthDataBytes(subjectFederate.encode());
      });
  REQUIRE(reflectedSubject != observerReports.attributeReflectionReports.end());
  auto const subjectObjectInstance = reflectedSubject->objectInstance;
  auto const beforeTiming = observerReports.attributeReflectionReports.size();

  auto const setTiming = observer->getInteractionClassHandle(
      standard_hla::mom::set_timing);
  auto const federateParameter = observer->getParameterHandle(setTiming, standard_hla::mom::federate);
  auto const periodParameter = observer->getParameterHandle(setTiming, standard_hla::mom::report_period);
  REQUIRE(setTiming.isValid());
  REQUIRE(federateParameter.isValid());
  REQUIRE(periodParameter.isValid());

  auto const subjectReference = subjectFederate.encode();
  auto sendTiming = [&](std::int32_t const seconds) {
    REQUIRE_NOTHROW(observer->sendInteraction(
        setTiming,
        ParameterHandleValueMap{
            {federateParameter, subjectReference},
            {periodParameter, rti1516_2025::HLAinteger32BE{seconds}.encode()}},
        VariableLengthData{}));
  };

  // HLAseconds is the official signed HLAinteger32BE representation in the
  // 2025 MIM.  A negative period is invalid and must not arm the scheduler.
  REQUIRE_THROWS_AS(
      observer->sendInteraction(
          setTiming,
          ParameterHandleValueMap{
              {federateParameter, subjectReference},
              {periodParameter, rti1516_2025::HLAinteger32BE{-1}.encode()}},
          VariableLengthData{}),
      rti1516_2025::RTIinternalError);
  while (observer->evokeCallback(0.0)) {
  }
  REQUIRE(observerReports.interactionReports.empty());
  REQUIRE(observerReports.attributeReflectionReports.size() == beforeTiming);
  auto const rejectedPeriodDeadline =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(1100);
  while (observerReports.attributeReflectionReports.size() == beforeTiming &&
         std::chrono::steady_clock::now() < rejectedPeriodDeadline) {
    static_cast<void>(observer->evokeCallback(0.01));
  }
  REQUIRE(observerReports.attributeReflectionReports.size() == beforeTiming);

  sendTiming(1);
  auto const periodicDeadline =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(2500);
  while (observerReports.attributeReflectionReports.size() == beforeTiming &&
         std::chrono::steady_clock::now() < periodicDeadline) {
    static_cast<void>(observer->evokeCallback(0.01));
  }
  auto const periodicReflection = std::find_if(
      observerReports.attributeReflectionReports.begin() +
          static_cast<std::ptrdiff_t>(beforeTiming),
      observerReports.attributeReflectionReports.end(),
      [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
        return report.objectInstance == subjectObjectInstance &&
            report.attributeValues.contains(logicalTimeAttribute);
      });
  REQUIRE(periodicReflection != observerReports.attributeReflectionReports.end());
  REQUIRE(periodicReflection->attributeValues.size() == 2U);
  rti1516_2025::HLAinteger64Time periodicLogicalTime;
  REQUIRE_NOTHROW(periodicLogicalTime.decode(
      periodicReflection->attributeValues.at(logicalTimeAttribute)));
  REQUIRE(periodicLogicalTime.getTime() == 0);
  REQUIRE(periodicReflection->attributeValues.at(lookaheadAttribute).size() == 0U);
  REQUIRE(periodicReflection->transportationType ==
          observer->getTransportationTypeHandle(standard_hla::mom::reliable));
  REQUIRE_FALSE(periodicReflection->producingFederate.isValid());
  REQUIRE_FALSE(periodicReflection->sentRegionsSupplied);
  REQUIRE(periodicReflection->userSuppliedTag.size() == 0U);

  sendTiming(0);
  while (observer->evokeCallback(0.0)) {
  }

  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subject->disconnect());
  REQUIRE_NOTHROW(observer->disconnect());
}
}
