#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

#include <array>
#include <cstring>
#include <filesystem>

namespace {

std::filesystem::path sourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / relativePath;
}

class NegativeEncodedInteger64Interval final
    : public rti1516_2025::LogicalTimeInterval {
 public:
  void setZero() override { zero_.setZero(); }
  bool isZero() const override { return zero_.isZero(); }
  void setEpsilon() override { zero_.setEpsilon(); }
  bool isEpsilon() const override { return zero_.isEpsilon(); }
  rti1516_2025::LogicalTimeInterval& operator=(
      rti1516_2025::LogicalTimeInterval const& value) override {
    return zero_ = value;
  }
  rti1516_2025::LogicalTimeInterval& operator+=(
      rti1516_2025::LogicalTimeInterval const& value) override {
    return zero_ += value;
  }
  rti1516_2025::LogicalTimeInterval& operator-=(
      rti1516_2025::LogicalTimeInterval const& value) override {
    return zero_ -= value;
  }
  bool operator>(rti1516_2025::LogicalTimeInterval const& value) const override {
    return zero_ > value;
  }
  bool operator<(rti1516_2025::LogicalTimeInterval const& value) const override {
    return zero_ < value;
  }
  bool operator==(rti1516_2025::LogicalTimeInterval const& value) const override {
    return zero_ == value;
  }
  bool operator>=(rti1516_2025::LogicalTimeInterval const& value) const override {
    return zero_ >= value;
  }
  bool operator<=(rti1516_2025::LogicalTimeInterval const& value) const override {
    return zero_ <= value;
  }
  void setToDifference(
      rti1516_2025::LogicalTime const& minuend,
      rti1516_2025::LogicalTime const& subtrahend) override {
    zero_.setToDifference(minuend, subtrahend);
  }
  rti1516_2025::VariableLengthData encode() const override {
    auto const bytes = negativeOneEncoding();
    return rti1516_2025::VariableLengthData(bytes.data(), bytes.size());
  }
  std::size_t encode(void* buffer, std::size_t bufferSize) const override {
    auto const bytes = negativeOneEncoding();
    if (buffer == nullptr || bufferSize < bytes.size()) {
      throw rti1516_2025::CouldNotEncode(L"The test interval buffer is too small.");
    }
    std::memcpy(buffer, bytes.data(), bytes.size());
    return bytes.size();
  }
  std::size_t encodedLength() const override { return 8U; }
  void decode(rti1516_2025::VariableLengthData const& value) override {
    zero_.decode(value);
  }
  void decode(void const* buffer, std::size_t bufferSize) override {
    zero_.decode(buffer, bufferSize);
  }
  std::wstring toString() const override { return L"-1"; }
  std::wstring implementationName() const override {
    return zero_.implementationName();
  }

 private:
  static std::array<std::uint8_t, 8> negativeOneEncoding() {
    return {0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU};
  }

  rti1516_2025::HLAinteger64Interval zero_;
};

}  // namespace

TEST_CASE(
    "Embedded Modify Lookahead applies increases immediately and decreases gradually",
    "[integration][development-profile][time-management][lookahead][time-role][modify-lookahead]"
    "[rti.service.modify-lookahead][rti.service.query-lookahead]"
    "[rti.service.time-advance-request][federate.callback.time-advance-grant]"
    "[modify-lookahead-increase-and-gradual-decrease][2025]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  rti1516_2025::HLAinteger64Interval lookahead;

  REQUIRE_THROWS_AS(
      rti->modifyLookahead(rti1516_2025::HLAinteger64Interval(2)),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      rti->modifyLookahead(rti1516_2025::HLAinteger64Interval(2)),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(L"lookahead-client", federationName));
  REQUIRE_THROWS_AS(
      rti->modifyLookahead(rti1516_2025::HLAinteger64Interval(2)),
      rti1516_2025::TimeRegulationIsNotEnabled);
  auto const interactionClass = rti->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  REQUIRE(interactionClass.isValid());
  REQUIRE_NOTHROW(rti->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(rti->changeInteractionOrderType(interactionClass, TIMESTAMP));
  ParameterHandleValueMap const values;
  VariableLengthData const tag;

  REQUIRE_NOTHROW(rti->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(2)));
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 2);

  REQUIRE_NOTHROW(rti->modifyLookahead(rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 5);
  REQUIRE_THROWS_AS(
      rti->sendInteraction(
          interactionClass,
          values,
          tag,
          rti1516_2025::HLAinteger64Time(4)),
      rti1516_2025::InvalidLogicalTime);
  REQUIRE(rti->sendInteraction(
              interactionClass,
              values,
              tag,
              rti1516_2025::HLAinteger64Time(5))
              .isValid());

  // A decrease is announced immediately but the actual lookahead remains at
  // five until logical time advances.
  REQUIRE_NOTHROW(rti->modifyLookahead(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 5);
  REQUIRE_THROWS_AS(
      rti->sendInteraction(
          interactionClass,
          values,
          tag,
          rti1516_2025::HLAinteger64Time(4)),
      rti1516_2025::InvalidLogicalTime);
  REQUIRE(rti->sendInteraction(
              interactionClass,
              values,
              tag,
              rti1516_2025::HLAinteger64Time(5))
              .isValid());

  REQUIRE_NOTHROW(rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
  REQUIRE_THROWS_AS(
      rti->modifyLookahead(rti1516_2025::HLAinteger64Interval(2)),
      rti1516_2025::InTimeAdvancingState);
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 2);
  REQUIRE(rti->sendInteraction(
              interactionClass,
              values,
              tag,
              rti1516_2025::HLAinteger64Time(5))
              .isValid());

  REQUIRE_NOTHROW(rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 1);
  REQUIRE_THROWS_AS(
      rti->sendInteraction(
          interactionClass,
          values,
          tag,
          rti1516_2025::HLAinteger64Time(5)),
      rti1516_2025::InvalidLogicalTime);
  REQUIRE(rti->sendInteraction(
              interactionClass,
              values,
              tag,
              rti1516_2025::HLAinteger64Time(6))
              .isValid());

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded Modify Lookahead applies its send boundary to timestamped attribute updates",
    "[integration][development-profile][object-management][time-management]"
    "[lookahead][time-role][modify-lookahead][timestamped-attribute-update]"
    "[modify-lookahead-attribute-update-boundary]"
    "[rti.service.enable-time-regulation][rti.service.modify-lookahead]"
    "[rti.service.query-lookahead][rti.service.update-attribute-values]"
    "[rti.service.time-advance-request][rti.service.evoke-callback]"
    "[federate.callback.time-regulation-enabled]"
    "[federate.callback.time-advance-grant]"
    "[requirement:hla-1516.1-2025:clause-8.20.4]"
    "[requirement:hla-1516.1-2025:clause-8.15.3][2025]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = sourcePath(
      std::filesystem::path("cpp") / "tests" / "data" / "attribute-update-passel-fom.xml")
                               .wstring();
  unsigned char const attributeBytes[] = {0xE1U, 0x52U};
  rti1516_2025::HLAinteger64Interval lookahead;

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(rti->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"modify-lookahead-attribute-update", L"publisher", federationName));

  auto const objectClass = rti->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const attribute = rti->getAttributeHandle(
      objectClass, fixture_hla::fixture::reliable_base_a);
  REQUIRE(objectClass.isValid());
  REQUIRE(attribute.isValid());
  AttributeHandleSet const attributes{attribute};
  REQUIRE_NOTHROW(rti->publishObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(rti->changeDefaultAttributeOrderType(
      objectClass, attributes, TIMESTAMP));
  auto const objectInstance = rti->registerObjectInstance(objectClass);
  REQUIRE(objectInstance.isValid());

  AttributeHandleValueMap values;
  values.emplace(
      attribute,
      VariableLengthData(attributeBytes, sizeof(attributeBytes)));
  VariableLengthData const tag;
  REQUIRE_NOTHROW(rti->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(2)));
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.timeRegulationEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 2);

  REQUIRE_NOTHROW(rti->modifyLookahead(rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 5);
  REQUIRE_THROWS_AS(
      rti->updateAttributeValues(
          objectInstance,
          values,
          tag,
          rti1516_2025::HLAinteger64Time(4)),
      rti1516_2025::InvalidLogicalTime);
  REQUIRE(rti->updateAttributeValues(
              objectInstance,
              values,
              tag,
              rti1516_2025::HLAinteger64Time(5))
              .isValid());

  REQUIRE_NOTHROW(rti->modifyLookahead(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 5);
  REQUIRE_THROWS_AS(
      rti->updateAttributeValues(
          objectInstance,
          values,
          tag,
          rti1516_2025::HLAinteger64Time(4)),
      rti1516_2025::InvalidLogicalTime);
  REQUIRE(rti->updateAttributeValues(
              objectInstance,
              values,
              tag,
              rti1516_2025::HLAinteger64Time(5))
              .isValid());

  REQUIRE_NOTHROW(rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(reports.timeAdvanceGrantReports.front().value == L"3");
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 2);
  REQUIRE(rti->updateAttributeValues(
              objectInstance,
              values,
              tag,
              rti1516_2025::HLAinteger64Time(5))
              .isValid());

  REQUIRE_NOTHROW(rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.timeAdvanceGrantReports.size() == 2U);
  REQUIRE(reports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 1);
  REQUIRE_THROWS_AS(
      rti->updateAttributeValues(
          objectInstance,
          values,
          tag,
          rti1516_2025::HLAinteger64Time(5)),
      rti1516_2025::InvalidLogicalTime);
  REQUIRE(rti->updateAttributeValues(
              objectInstance,
              values,
              tag,
              rti1516_2025::HLAinteger64Time(6))
              .isValid());

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded Modify Lookahead applies its send boundary to timestamped object deletion",
    "[integration][development-profile][object-management][time-management]"
    "[lookahead][time-role][modify-lookahead][timestamped-object-deletion]"
    "[modify-lookahead-delete-object-boundary]"
    "[rti.service.enable-time-regulation][rti.service.modify-lookahead]"
    "[rti.service.query-lookahead][rti.service.delete-object-instance]"
    "[rti.service.time-advance-request][rti.service.evoke-callback]"
    "[federate.callback.time-regulation-enabled]"
    "[federate.callback.time-advance-grant]"
    "[requirement:hla-1516.1-2025:clause-8.20.4]"
    "[requirement:hla-1516.1-2025:clause-8.15.3][2025]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = sourcePath(
      std::filesystem::path("cpp") / "tests" / "data" / "attribute-update-passel-fom.xml")
                               .wstring();
  VariableLengthData const tag;
  rti1516_2025::HLAinteger64Interval lookahead;

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(rti->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"modify-lookahead-object-delete", L"publisher", federationName));

  auto const objectClass = rti->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const reliable = rti->getAttributeHandle(
      objectClass, fixture_hla::fixture::reliable_base_a);
  auto const privilegeToDelete = rti->getAttributeHandle(
      objectClass, L"HLAprivilegeToDeleteObject");
  REQUIRE(objectClass.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE(privilegeToDelete.isValid());
  REQUIRE_NOTHROW(rti->publishObjectClassAttributes(
      objectClass, AttributeHandleSet{reliable}));

  // The per-instance preferred order for HLAprivilegeToDeleteObject selects
  // the timestamp-ordered Delete Object Instance route exercised below.
  auto const increasingObject = rti->registerObjectInstance(objectClass);
  auto const deferredObject = rti->registerObjectInstance(objectClass);
  auto const decreasedObject = rti->registerObjectInstance(objectClass);
  auto const finalObject = rti->registerObjectInstance(objectClass);
  REQUIRE(increasingObject.isValid());
  REQUIRE(deferredObject.isValid());
  REQUIRE(decreasedObject.isValid());
  REQUIRE(finalObject.isValid());
  AttributeHandleSet const privilegeAttributeSet{privilegeToDelete};
  REQUIRE_NOTHROW(rti->changeAttributeOrderType(
      increasingObject, privilegeAttributeSet, TIMESTAMP));
  REQUIRE_NOTHROW(rti->changeAttributeOrderType(
      deferredObject, privilegeAttributeSet, TIMESTAMP));
  REQUIRE_NOTHROW(rti->changeAttributeOrderType(
      decreasedObject, privilegeAttributeSet, TIMESTAMP));
  REQUIRE_NOTHROW(rti->changeAttributeOrderType(
      finalObject, privilegeAttributeSet, TIMESTAMP));

  REQUIRE_NOTHROW(rti->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(2)));
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.timeRegulationEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 2);

  REQUIRE_NOTHROW(rti->modifyLookahead(rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 5);
  REQUIRE_THROWS_AS(
      rti->deleteObjectInstance(
          increasingObject,
          tag,
          rti1516_2025::HLAinteger64Time(4)),
      rti1516_2025::InvalidLogicalTime);
  REQUIRE(rti->deleteObjectInstance(
              increasingObject,
              tag,
              rti1516_2025::HLAinteger64Time(5))
              .isValid());

  REQUIRE_NOTHROW(rti->modifyLookahead(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 5);
  REQUIRE_THROWS_AS(
      rti->deleteObjectInstance(
          deferredObject,
          tag,
          rti1516_2025::HLAinteger64Time(4)),
      rti1516_2025::InvalidLogicalTime);
  REQUIRE(rti->deleteObjectInstance(
              deferredObject,
              tag,
              rti1516_2025::HLAinteger64Time(5))
              .isValid());

  REQUIRE_NOTHROW(rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(reports.timeAdvanceGrantReports.front().value == L"3");
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 2);
  REQUIRE(rti->deleteObjectInstance(
              decreasedObject,
              tag,
              rti1516_2025::HLAinteger64Time(5))
              .isValid());

  REQUIRE_NOTHROW(rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.timeAdvanceGrantReports.size() == 2U);
  REQUIRE(reports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 1);
  REQUIRE_THROWS_AS(
      rti->deleteObjectInstance(
          finalObject,
          tag,
          rti1516_2025::HLAinteger64Time(5)),
      rti1516_2025::InvalidLogicalTime);
  REQUIRE(rti->deleteObjectInstance(
              finalObject,
              tag,
              rti1516_2025::HLAinteger64Time(6))
              .isValid());

  REQUIRE_NOTHROW(rti->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded Modify Lookahead rejects negative values without changing actual lookahead",
    "[integration][development-profile][time-management][lookahead][time-role][modify-lookahead]"
    "[rti.service.modify-lookahead][rti.service.query-lookahead]"
    "[modify-lookahead-negative-interval][2025]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  rti1516_2025::HLAinteger64Interval actualLookahead;

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(rti->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(L"negative-lookahead-client", federationName));
  REQUIRE_NOTHROW(
      rti->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(2)));
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE_NOTHROW(rti->queryLookahead(actualLookahead));
  REQUIRE(actualLookahead.getInterval() == 2);

  REQUIRE_THROWS_AS(
      rti->modifyLookahead(NegativeEncodedInteger64Interval{}),
      rti1516_2025::InvalidLookahead);
  REQUIRE_NOTHROW(rti->queryLookahead(actualLookahead));
  REQUIRE(actualLookahead.getInterval() == 2);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}
