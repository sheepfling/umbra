#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

#include <array>
#include <cstring>

namespace {

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

  REQUIRE_NOTHROW(rti->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(2)));
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 2);

  REQUIRE_NOTHROW(rti->modifyLookahead(rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 5);

  // A decrease is announced immediately but the actual lookahead remains at
  // five until logical time advances.
  REQUIRE_NOTHROW(rti->modifyLookahead(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 5);

  REQUIRE_NOTHROW(rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
  REQUIRE_THROWS_AS(
      rti->modifyLookahead(rti1516_2025::HLAinteger64Interval(2)),
      rti1516_2025::InTimeAdvancingState);
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 2);

  REQUIRE_NOTHROW(rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 1);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
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
