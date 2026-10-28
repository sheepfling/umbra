#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

#include <RTI/libfedtime/LogicalTimeFactoryFactory.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAlogicalTimeFactoryFactory.h>
#include <RTI/time/HLAinteger64Time.h>
#include <RTI/time/HLAinteger64TimeFactory.h>

#include <cstdint>

namespace {
constexpr std::wstring_view customTimeName = L"UmbraTestInteger64Time";

class CustomInteger64Time final : public rti1516_2025::HLAinteger64Time {
 public:
  using HLAinteger64Time::HLAinteger64Time;

  [[nodiscard]] std::wstring implementationName() const override {
    return std::wstring(customTimeName);
  }
};

class CustomInteger64Interval final : public rti1516_2025::HLAinteger64Interval {
 public:
  using HLAinteger64Interval::HLAinteger64Interval;

  [[nodiscard]] std::wstring implementationName() const override {
    return std::wstring(customTimeName);
  }
};

class CustomInteger64TimeFactory final : public rti1516_2025::HLAinteger64TimeFactory {
 public:
  std::unique_ptr<rti1516_2025::HLAinteger64Time> makeLogicalTime(
      rti1516_2025::Integer64 value) override {
    return std::make_unique<CustomInteger64Time>(value);
  }

  std::unique_ptr<rti1516_2025::LogicalTime> makeInitial() override {
    auto time = std::make_unique<CustomInteger64Time>();
    time->setInitial();
    return time;
  }

  std::unique_ptr<rti1516_2025::LogicalTime> makeFinal() override {
    auto time = std::make_unique<CustomInteger64Time>();
    time->setFinal();
    return time;
  }

  std::unique_ptr<rti1516_2025::HLAinteger64Interval> makeLogicalTimeInterval(
      rti1516_2025::Integer64 value) override {
    return std::make_unique<CustomInteger64Interval>(value);
  }

  std::unique_ptr<rti1516_2025::LogicalTimeInterval> makeZero() override {
    auto interval = std::make_unique<CustomInteger64Interval>();
    interval->setZero();
    return interval;
  }

  std::unique_ptr<rti1516_2025::LogicalTimeInterval> makeEpsilon() override {
    auto interval = std::make_unique<CustomInteger64Interval>();
    interval->setEpsilon();
    return interval;
  }

  std::unique_ptr<rti1516_2025::LogicalTime> decodeLogicalTime(
      rti1516_2025::VariableLengthData const& encoded) override {
    auto time = std::make_unique<CustomInteger64Time>();
    time->decode(encoded);
    return time;
  }

  std::unique_ptr<rti1516_2025::LogicalTime> decodeLogicalTime(
      void const* buffer,
      std::size_t bufferSize) override {
    auto time = std::make_unique<CustomInteger64Time>();
    time->decode(buffer, bufferSize);
    return time;
  }

  std::unique_ptr<rti1516_2025::LogicalTimeInterval> decodeLogicalTimeInterval(
      rti1516_2025::VariableLengthData const& encoded) override {
    auto interval = std::make_unique<CustomInteger64Interval>();
    interval->decode(encoded);
    return interval;
  }

  std::unique_ptr<rti1516_2025::LogicalTimeInterval> decodeLogicalTimeInterval(
      void const* buffer,
      std::size_t bufferSize) override {
    auto interval = std::make_unique<CustomInteger64Interval>();
    interval->decode(buffer, bufferSize);
    return interval;
  }

  [[nodiscard]] std::wstring getName() const override {
    return std::wstring(customTimeName);
  }
};

constexpr std::wstring_view taggedTimeName = L"UmbraTestTaggedTime";

enum class TaggedTimeType : std::uint8_t {
  logical = 0U,
  wallClock = 1U,
};

constexpr std::uint64_t taggedTimeTypeField = std::uint64_t{1} << 62U;
constexpr std::uint64_t taggedTimeValueMask = taggedTimeTypeField - 1U;

// Keep HLAinteger64BE's eight-octet shape and nonnegative signed range: bit
// 62 is the TRADT time-type discriminator and bits 0-61 hold the value.
class TaggedCustomInteger64Time final
    : public rti1516_2025::HLAinteger64Time {
 public:
  TaggedCustomInteger64Time() = default;

  explicit TaggedCustomInteger64Time(rti1516_2025::Integer64 logicalValue)
      : TaggedCustomInteger64Time(TaggedTimeType::logical, logicalValue) {}

  TaggedCustomInteger64Time(TaggedTimeType type,
                            rti1516_2025::Integer64 logicalValue)
      : rti1516_2025::HLAinteger64Time(encodeValue(type, logicalValue)) {}

  [[nodiscard]] TaggedTimeType timeType() const {
    return (static_cast<std::uint64_t>(getTime()) & taggedTimeTypeField) == 0U
        ? TaggedTimeType::logical
        : TaggedTimeType::wallClock;
  }

  void setInitial() override {
    setTime(encodePayload(timeType(), 0U));
  }

  [[nodiscard]] bool isInitial() const override {
    return logicalValue() == 0;
  }

  void setFinal() override {
    setTime(encodePayload(timeType(), taggedTimeValueMask));
  }

  [[nodiscard]] bool isFinal() const override {
    return logicalValue() == static_cast<rti1516_2025::Integer64>(
                                 taggedTimeValueMask);
  }

  [[nodiscard]] rti1516_2025::Integer64 logicalValue() const {
    return static_cast<rti1516_2025::Integer64>(
        static_cast<std::uint64_t>(getTime()) & taggedTimeValueMask);
  }

  [[nodiscard]] std::wstring implementationName() const override {
    return std::wstring(taggedTimeName);
  }

 private:
  [[nodiscard]] static rti1516_2025::Integer64 encodeValue(
      TaggedTimeType type,
      rti1516_2025::Integer64 logicalValue) {
    if (logicalValue < 0 ||
        static_cast<std::uint64_t>(logicalValue) >= taggedTimeValueMask) {
      throw rti1516_2025::InvalidLogicalTime(
          L"Tagged TRADT values must fit below the reserved final-time marker.");
    }
    return encodePayload(type, static_cast<std::uint64_t>(logicalValue));
  }

  [[nodiscard]] static rti1516_2025::Integer64 encodePayload(
      TaggedTimeType type,
      std::uint64_t payload) {
    auto encoded = payload;
    if (type == TaggedTimeType::wallClock) {
      encoded |= taggedTimeTypeField;
    }
    return static_cast<rti1516_2025::Integer64>(encoded);
  }
};

class TaggedCustomInteger64Interval final
    : public rti1516_2025::HLAinteger64Interval {
 public:
  using HLAinteger64Interval::HLAinteger64Interval;

  [[nodiscard]] std::wstring implementationName() const override {
    return std::wstring(taggedTimeName);
  }
};

class TaggedCustomInteger64TimeFactory final
    : public rti1516_2025::HLAinteger64TimeFactory {
 public:
  std::unique_ptr<rti1516_2025::HLAinteger64Time> makeLogicalTime(
      rti1516_2025::Integer64 value) override {
    return std::make_unique<TaggedCustomInteger64Time>(value);
  }

  std::unique_ptr<rti1516_2025::LogicalTime> makeInitial() override {
    auto time = std::make_unique<TaggedCustomInteger64Time>();
    time->setInitial();
    return time;
  }

  std::unique_ptr<rti1516_2025::LogicalTime> makeFinal() override {
    auto time = std::make_unique<TaggedCustomInteger64Time>();
    time->setFinal();
    return time;
  }

  std::unique_ptr<rti1516_2025::HLAinteger64Interval>
  makeLogicalTimeInterval(rti1516_2025::Integer64 value) override {
    return std::make_unique<TaggedCustomInteger64Interval>(value);
  }

  std::unique_ptr<rti1516_2025::LogicalTimeInterval> makeZero() override {
    auto interval = std::make_unique<TaggedCustomInteger64Interval>();
    interval->setZero();
    return interval;
  }

  std::unique_ptr<rti1516_2025::LogicalTimeInterval> makeEpsilon() override {
    auto interval = std::make_unique<TaggedCustomInteger64Interval>();
    interval->setEpsilon();
    return interval;
  }

  std::unique_ptr<rti1516_2025::LogicalTime> decodeLogicalTime(
      rti1516_2025::VariableLengthData const& encoded) override {
    auto time = std::make_unique<TaggedCustomInteger64Time>();
    time->decode(encoded);
    return time;
  }

  std::unique_ptr<rti1516_2025::LogicalTime> decodeLogicalTime(
      void const* buffer,
      std::size_t bufferSize) override {
    auto time = std::make_unique<TaggedCustomInteger64Time>();
    time->decode(buffer, bufferSize);
    return time;
  }

  std::unique_ptr<rti1516_2025::LogicalTimeInterval>
  decodeLogicalTimeInterval(
      rti1516_2025::VariableLengthData const& encoded) override {
    auto interval = std::make_unique<TaggedCustomInteger64Interval>();
    interval->decode(encoded);
    return interval;
  }

  std::unique_ptr<rti1516_2025::LogicalTimeInterval>
  decodeLogicalTimeInterval(void const* buffer,
                            std::size_t bufferSize) override {
    auto interval = std::make_unique<TaggedCustomInteger64Interval>();
    interval->decode(buffer, bufferSize);
    return interval;
  }

  [[nodiscard]] std::wstring getName() const override {
    return std::wstring(taggedTimeName);
  }
};
}  // namespace

namespace rti1516_2025 {
std::unique_ptr<LogicalTimeFactory> LogicalTimeFactoryFactory::makeLogicalTimeFactory(
    std::wstring const& implementationName) {
  if (implementationName.empty()) {
    return HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(implementationName);
  }
  if (implementationName == customTimeName) {
    return std::make_unique<CustomInteger64TimeFactory>();
  }
  if (implementationName == taggedTimeName) {
    return std::make_unique<TaggedCustomInteger64TimeFactory>();
  }
  return {};
}
}  // namespace rti1516_2025

TEST_CASE(
    "Embedded RTI uses the federation-supplied custom logical-time factory",
    "[integration][development-profile][custom-time-factory][time-management]"
    "[rti.service.create-federation-execution][rti.service.get-time-factory]"
    "[rti.service.query-logical-time][rti.service.query-lookahead]"
    "[rti.service.enable-time-regulation]"
    "[rti.service.time-advance-request][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "time-representation-custom-integer64-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  try {
    rti->createFederationExecution(federationName, fomModule, std::wstring(customTimeName));
  } catch (rti1516_2025::Exception const& error) {
    auto const name = error.name();
    auto const message = error.what();
    FAIL(std::string(name.begin(), name.end()) + ": " +
         std::string(message.begin(), message.end()));
  }
  REQUIRE_NOTHROW(rti->joinFederationExecution(L"custom-time-client", federationName));

  auto factory = rti->getTimeFactory();
  REQUIRE(factory);
  REQUIRE(factory->getName() == customTimeName);
  auto initial = factory->makeInitial();
  auto* customInitial = dynamic_cast<CustomInteger64Time*>(initial.get());
  REQUIRE(customInitial != nullptr);
  REQUIRE(customInitial->isInitial());
  REQUIRE(customInitial->implementationName() == customTimeName);

  CustomInteger64Time currentTime;
  REQUIRE_NOTHROW(rti->queryLogicalTime(currentTime));
  REQUIRE(currentTime.getTime() == 0);
  REQUIRE(currentTime.implementationName() == customTimeName);

  REQUIRE_NOTHROW(rti->enableTimeRegulation(CustomInteger64Interval(3)));
  REQUIRE(reports.timeRegulationEnabledReports.empty());
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.timeRegulationEnabledReports.size() == 1U);
  REQUIRE(reports.timeRegulationEnabledReports.front().implementationName == customTimeName);
  CustomInteger64Interval currentLookahead;
  REQUIRE_NOTHROW(rti->queryLookahead(currentLookahead));
  REQUIRE(currentLookahead.getInterval() == 3);
  REQUIRE(currentLookahead.implementationName() == customTimeName);

  REQUIRE_NOTHROW(rti->timeAdvanceRequest(CustomInteger64Time(12)));
  REQUIRE_NOTHROW(rti->queryLogicalTime(currentTime));
  REQUIRE(currentTime.getTime() == 0);
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(reports.timeAdvanceGrantReports.front().implementationName == customTimeName);
  REQUIRE(reports.timeAdvanceGrantReports.front().value == L"12");
  REQUIRE_NOTHROW(rti->queryLogicalTime(currentTime));
  REQUIRE(currentTime.getTime() == 12);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "A federation TRADT discriminates logical and wall-clock time values",
    "[integration][development-profile][custom-time-factory]"
    "[time-representation][tradt-discriminator]"
    "[rti.service.create-federation-execution][rti.service.get-time-factory]"
    "[rti.service.query-logical-time]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "time-representation-tagged-multi-type-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  try {
    rti->createFederationExecution(
        federationName, fomModule, std::wstring(taggedTimeName));
  } catch (rti1516_2025::Exception const& error) {
    auto const name = error.name();
    auto const message = error.what();
    FAIL(std::string(name.begin(), name.end()) + ": " +
         std::string(message.begin(), message.end()));
  }
  REQUIRE_NOTHROW(rti->joinFederationExecution(L"tagged-time-client", federationName));

  auto factory = rti->getTimeFactory();
  REQUIRE(factory);
  REQUIRE(factory->getName() == taggedTimeName);
  auto currentTime = factory->makeInitial();
  REQUIRE(currentTime);
  REQUIRE_NOTHROW(rti->queryLogicalTime(*currentTime));
  auto const* taggedCurrent =
      dynamic_cast<TaggedCustomInteger64Time const*>(currentTime.get());
  REQUIRE(taggedCurrent != nullptr);
  REQUIRE(taggedCurrent->isInitial());
  REQUIRE(taggedCurrent->timeType() == TaggedTimeType::logical);

  auto finalTime = factory->makeFinal();
  auto const* taggedFinal =
      dynamic_cast<TaggedCustomInteger64Time const*>(finalTime.get());
  REQUIRE(taggedFinal != nullptr);
  REQUIRE(taggedFinal->isFinal());
  REQUIRE(taggedFinal->timeType() == TaggedTimeType::logical);

  TaggedCustomInteger64Time logicalTime{TaggedTimeType::logical, 41};
  TaggedCustomInteger64Time wallClockTime{TaggedTimeType::wallClock, 41};
  auto const logicalEncoding = logicalTime.encode();
  auto const wallClockEncoding = wallClockTime.encode();
  REQUIRE(logicalEncoding.size() == sizeof(rti1516_2025::Integer64));
  REQUIRE(wallClockEncoding.size() == logicalEncoding.size());
  auto const equalPayloadsCompareUnequal =
      logicalTime.operator==(wallClockTime);
  REQUIRE_FALSE(equalPayloadsCompareUnequal);

  auto decodedLogical = factory->decodeLogicalTime(logicalEncoding);
  auto const* taggedLogical =
      dynamic_cast<TaggedCustomInteger64Time const*>(decodedLogical.get());
  REQUIRE(taggedLogical != nullptr);
  REQUIRE(taggedLogical->timeType() == TaggedTimeType::logical);
  REQUIRE(taggedLogical->logicalValue() == 41);
  REQUIRE(taggedLogical->implementationName() == taggedTimeName);

  auto decodedWallClock = factory->decodeLogicalTime(
      wallClockEncoding.data(), wallClockEncoding.size());
  auto const* taggedWallClock =
      dynamic_cast<TaggedCustomInteger64Time const*>(decodedWallClock.get());
  REQUIRE(taggedWallClock != nullptr);
  REQUIRE(taggedWallClock->timeType() == TaggedTimeType::wallClock);
  REQUIRE(taggedWallClock->logicalValue() == 41);
  REQUIRE(taggedWallClock->implementationName() == taggedTimeName);
  auto const decodedTimeKindsCompareUnequal =
      decodedLogical->operator==(*decodedWallClock);
  REQUIRE_FALSE(decodedTimeKindsCompareUnequal);

  wallClockTime.setFinal();
  REQUIRE(wallClockTime.isFinal());
  REQUIRE(wallClockTime.timeType() == TaggedTimeType::wallClock);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}
