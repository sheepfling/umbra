#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <RTI/RTI1516.h>
#include <RTI/encoding/EncodingExceptions.h>
#include <RTI/encoding/HLAlogicalTime.h>
#include <RTI/encoding/HLAlogicalTimeInterval.h>
#include <RTI/time/HLAfloat64Interval.h>
#include <RTI/time/HLAfloat64Time.h>
#include <RTI/time/HLAfloat64TimeFactory.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>
#include <RTI/time/HLAinteger64TimeFactory.h>
#include <RTI/time/HLAlogicalTimeFactoryFactory.h>
#include <RTI/time/LogicalTime.h>
#include <RTI/time/LogicalTimeInterval.h>

#include "generated/rti_ambassador_shell.hpp"

namespace {

using rti1516_2025::CouldNotDecode;
using rti1516_2025::CouldNotEncode;
using rti1516_2025::EncoderException;
using rti1516_2025::HLAfloat64Interval;
using rti1516_2025::HLAfloat64Time;
using rti1516_2025::HLAfloat64TimeFactory;
using rti1516_2025::HLAinteger64Interval;
using rti1516_2025::HLAinteger64Time;
using rti1516_2025::HLAinteger64TimeFactory;
using rti1516_2025::HLAlogicalTime;
using rti1516_2025::HLAlogicalTimeFactoryFactory;
using rti1516_2025::HLAlogicalTimeInterval;
using rti1516_2025::IllegalTimeArithmetic;
using rti1516_2025::Integer64;
using rti1516_2025::InvalidLogicalTime;
using rti1516_2025::InvalidLogicalTimeInterval;
using rti1516_2025::LogicalTimeFactory;
using rti1516_2025::LogicalTimeFactoryFactory;
using rti1516_2025::LogicalTime;
using rti1516_2025::LogicalTimeInterval;
using rti1516_2025::Octet;
using rti1516_2025::VariableLengthData;

template <typename Left, typename Right>
concept SupportsAdditionAssignment = requires(Left& left, Right const& right) {
  left += right;
};

template <typename Left, typename Right>
concept SupportsSubtractionAssignment = requires(Left& left, Right const& right) {
  left -= right;
};

template <typename Left, typename Right>
concept SupportsAllComparisons = requires(Left const& left, Right const& right) {
  left < right;
  left <= right;
  left == right;
  left >= right;
  left > right;
  left != right;
};

template <typename Left, typename Right>
concept SupportsAdditionalBinaryArithmetic =
    requires(Left const& left, Right const& right) { left * right; } ||
    requires(Left const& left, Right const& right) { left / right; } ||
    requires(Left const& left, Right const& right) { left % right; };

template <typename Value>
concept SupportsAdditionalUnaryArithmetic =
    requires(Value const& value) { +value; } ||
    requires(Value const& value) { -value; };

template <typename Value>
concept SupportsIncrementOrDecrement =
    requires(Value& value) { ++value; } ||
    requires(Value& value) { value++; } ||
    requires(Value& value) { --value; } ||
    requires(Value& value) { value--; };

template <typename Left, typename Right>
concept SupportsAdditionalCompoundArithmetic =
    requires(Left& left, Right const& right) { left *= right; } ||
    requires(Left& left, Right const& right) { left /= right; } ||
    requires(Left& left, Right const& right) { left %= right; };

template <std::size_t Size>
bool encodedEquals(VariableLengthData const& actual, std::array<unsigned char, Size> const& expected) {
  return actual.size() == expected.size() &&
         std::memcmp(actual.data(), expected.data(), expected.size()) == 0;
}

class ReferenceTimeAmbassador final
    : public rti1516_2025::umbra_binding_detail::RtiAmbassadorShell {
 public:
  explicit ReferenceTimeAmbassador(std::wstring const& implementationName)
      : implementationName_(implementationName) {}

  std::unique_ptr<LogicalTimeFactory> getTimeFactory() const override {
    return HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(implementationName_);
  }

 private:
  std::wstring implementationName_;
};

template <typename Time, typename Interval>
void requireLogicalTimeIdentitySet() {
  Time initial;
  initial.setInitial();
  Time final;
  final.setFinal();
  Interval epsilon;
  epsilon.setEpsilon();
  Interval zero;
  zero.setZero();
  auto const timeLess = [](LogicalTime const& left, LogicalTime const& right) {
    return left.operator<(right);
  };
  auto const timeGreater = [](LogicalTime const& left, LogicalTime const& right) {
    return left.operator>(right);
  };
  auto const timeEqual = [](LogicalTime const& left, LogicalTime const& right) {
    return left.operator==(right);
  };
  auto const intervalEqual = [](
                                 LogicalTimeInterval const& left,
                                 LogicalTimeInterval const& right) {
    return left.operator==(right);
  };
  auto const intervalAtLeast = [](
                                   LogicalTimeInterval const& left,
                                   LogicalTimeInterval const& right) {
    return left.operator>=(right);
  };

  Time time(initial);
  time += epsilon;
  time += epsilon;
  Time advanced(time);
  advanced += epsilon;
  REQUIRE(timeLess(time, advanced));
  Time finalPlusInterval(final);
  finalPlusInterval += epsilon;
  REQUIRE(timeEqual(finalPlusInterval, final));
  Time retreated(time);
  retreated -= epsilon;
  REQUIRE(timeGreater(time, retreated));
  Time initialMinusInterval(initial);
  initialMinusInterval -= epsilon;
  REQUIRE(timeEqual(initialMinusInterval, initial));
  Time timePlusZero(time);
  timePlusZero += zero;
  REQUIRE(timeEqual(timePlusZero, time));
  Time timeMinusZero(time);
  timeMinusZero -= zero;
  REQUIRE(timeEqual(timeMinusZero, time));

  Interval differenceOfDistinctTimes;
  differenceOfDistinctTimes.setToDifference(advanced, time);
  REQUIRE(intervalAtLeast(differenceOfDistinctTimes, epsilon));
  Interval differenceOfEqualTimes;
  differenceOfEqualTimes.setToDifference(time, time);
  REQUIRE(intervalEqual(differenceOfEqualTimes, zero));

  Interval interval(epsilon);
  Interval intervalPlusZero(interval);
  intervalPlusZero += zero;
  REQUIRE(intervalEqual(intervalPlusZero, interval));
  Interval intervalMinusZero(interval);
  intervalMinusZero -= zero;
  REQUIRE(intervalEqual(intervalMinusZero, interval));
  Interval differenceOfEqualIntervals(interval);
  differenceOfEqualIntervals -= interval;
  REQUIRE(intervalEqual(differenceOfEqualIntervals, zero));
  Interval smallerMinusLarger(interval);
  Interval larger(interval);
  larger += epsilon;
  smallerMinusLarger -= larger;
  REQUIRE(intervalEqual(smallerMinusLarger, zero));
  Interval largerMinusSmaller(larger);
  largerMinusSmaller -= interval;
  REQUIRE(intervalAtLeast(largerMinusSmaller, epsilon));
  REQUIRE_FALSE(intervalEqual(epsilon, zero));
  REQUIRE(intervalAtLeast(epsilon, epsilon));
  REQUIRE(timeLess(retreated, time));
  REQUIRE(timeLess(initial, time));
  REQUIRE(timeLess(time, final));
}

}  // namespace

TEST_CASE(
    "HLAinteger64Time implements the IEEE reference values, arithmetic, and HLAinteger64BE encoding",
    "[baseline][time][unit][foundation][time-management]") {
  HLAinteger64Time time(41);
  HLAinteger64Interval epsilon;
  epsilon.setEpsilon();

  REQUIRE(time.getTime() == 41);
  REQUIRE(time.implementationName() == L"HLAinteger64Time");
  REQUIRE(epsilon.getInterval() == 1);
  REQUIRE(epsilon.isEpsilon());

  time += epsilon;
  REQUIRE(time.getTime() == 42);
  time -= epsilon;
  REQUIRE(time.getTime() == 41);

  HLAinteger64Interval difference;
  HLAinteger64Time later(44);
  difference.setToDifference(later, time);
  REQUIRE(difference.getInterval() == 3);

  std::array<unsigned char, 8> const expected{0, 0, 0, 0, 0, 0, 0, 41};
  auto encoded = time.encode();
  REQUIRE(encodedEquals(encoded, expected));

  HLAinteger64Time decoded;
  decoded.decode(encoded);
  REQUIRE(decoded.getTime() == time.getTime());

  HLAinteger64Time finalTime;
  finalTime.setFinal();
  REQUIRE(finalTime.isFinal());
  REQUIRE(finalTime.getTime() == std::numeric_limits<Integer64>::max());

  HLAinteger64Time initialTime;
  REQUIRE(initialTime.isInitial());
  REQUIRE_THROWS_AS(HLAinteger64Time(-1), InvalidLogicalTime);
}

TEST_CASE(
    "IEEE 1516.1-2025 logical time intervals reject negative values",
    "[2025][time][unit][time-management][logical-time-interval-nonnegative]") {
  REQUIRE_THROWS(HLAinteger64Interval(-1));
  HLAinteger64Interval integerInterval;
  REQUIRE_THROWS(integerInterval.setInterval(-1));

  REQUIRE_THROWS(HLAfloat64Interval(-1.0));
  HLAfloat64Interval floatInterval;
  REQUIRE_THROWS(floatInterval.setInterval(-1.0));
}

TEST_CASE(
    "Logical time arithmetic preserves the final value when adding or subtracting intervals",
    "[baseline][time][unit][foundation][time-management][final-time-arithmetic]") {
  HLAinteger64Interval integerEpsilon;
  integerEpsilon.setEpsilon();
  HLAinteger64Time integerFinal;
  integerFinal.setFinal();
  integerFinal += integerEpsilon;
  REQUIRE(integerFinal.isFinal());
  REQUIRE(integerFinal.getTime() == std::numeric_limits<Integer64>::max());
  integerFinal -= integerEpsilon;
  REQUIRE(integerFinal.isFinal());
  REQUIRE(integerFinal.getTime() == std::numeric_limits<Integer64>::max());

  HLAfloat64Interval floatEpsilon;
  floatEpsilon.setEpsilon();
  HLAfloat64Time floatFinal;
  floatFinal.setFinal();
  floatFinal += floatEpsilon;
  REQUIRE(floatFinal.isFinal());
  REQUIRE(floatFinal.getTime() == std::numeric_limits<double>::max());
  floatFinal -= floatEpsilon;
  REQUIRE(floatFinal.isFinal());
  REQUIRE(floatFinal.getTime() == std::numeric_limits<double>::max());
}

TEST_CASE("HLAinteger64Time reports malformed encodings and factory-decodes valid values", "[baseline][time][unit][foundation][time-management]") {
  HLAinteger64Time value;
  std::array<unsigned char, 7> const tooShort{};
  std::array<unsigned char, 8> const negative{0x80, 0, 0, 0, 0, 0, 0, 0};
  std::array<unsigned char, 7> output{};

  REQUIRE_THROWS_AS(value.decode(tooShort.data(), tooShort.size()), CouldNotDecode);
  REQUIRE_THROWS_AS(value.decode(negative.data(), negative.size()), CouldNotDecode);
  REQUIRE_THROWS_AS(value.encode(output.data(), output.size()), CouldNotEncode);

  HLAinteger64TimeFactory factory;
  std::array<unsigned char, 8> const encoded{0, 0, 0, 0, 0, 0, 1, 2};
  auto decoded = factory.decodeLogicalTime(encoded.data(), encoded.size());
  auto* concrete = dynamic_cast<HLAinteger64Time*>(decoded.get());
  REQUIRE(concrete != nullptr);
  REQUIRE(concrete->getTime() == 258);
  REQUIRE(factory.getName() == L"HLAinteger64Time");
}

TEST_CASE(
    "HLAfloat64Time uses finite IEEE boundaries and epsilon advances by a representable step",
    "[baseline][time][unit][foundation][time-management]") {
  constexpr double finalValue = std::numeric_limits<double>::max();
  constexpr double epsilonValue = std::numeric_limits<double>::denorm_min();

  HLAfloat64Time initial;
  HLAfloat64Time finalTime;
  HLAfloat64Interval epsilon;
  finalTime.setFinal();
  epsilon.setEpsilon();

  REQUIRE(initial.isInitial());
  REQUIRE(finalTime.isFinal());
  REQUIRE(finalTime.getTime() == finalValue);
  REQUIRE(epsilon.isEpsilon());
  REQUIRE(epsilon.getInterval() == epsilonValue);
  REQUIRE(finalTime.implementationName() == L"HLAfloat64Time");

  HLAfloat64Time large(finalValue / 2.0);
  auto const before = large.getTime();
  large += epsilon;
  REQUIRE(large.getTime() == std::nextafter(before, finalValue));
  large -= epsilon;
  REQUIRE(large.getTime() == before);

  HLAfloat64Time encodedValue(1.5);
  std::array<unsigned char, 8> const expected{0x3F, 0xF8, 0, 0, 0, 0, 0, 0};
  REQUIRE(encodedEquals(encodedValue.encode(), expected));

  HLAfloat64Time decoded;
  decoded.decode(encodedValue.encode());
  REQUIRE(decoded.getTime() == 1.5);

  finalTime += epsilon;
  REQUIRE(finalTime.isFinal());
  REQUIRE(finalTime.getTime() == finalValue);
  REQUIRE_THROWS_AS(HLAfloat64Time(std::numeric_limits<double>::infinity()), InvalidLogicalTime);
}

TEST_CASE(
    "IEEE 1516.1-2025 float64 time uses local epsilon for smaller nonzero intervals",
    "[2025][time][unit][time-management][logical-time-epsilon-step]") {
  constexpr double finalValue = std::numeric_limits<double>::max();
  constexpr double smallerInterval = std::numeric_limits<double>::denorm_min();

  HLAfloat64Interval interval(smallerInterval);
  HLAfloat64Time const startingTime(finalValue / 2.0);
  auto const startingValue = startingTime.getTime();
  auto const nextValue = std::nextafter(startingValue, finalValue);
  auto const previousValue = std::nextafter(startingValue, 0.0);
  REQUIRE(interval.getInterval() > 0.0);
  REQUIRE(interval.getInterval() < nextValue - startingValue);
  REQUIRE(interval.getInterval() < startingValue - previousValue);

  HLAfloat64Time adding(startingValue);
  adding += interval;
  REQUIRE(adding.getTime() == nextValue);

  HLAfloat64Time subtracting(startingValue);
  subtracting -= interval;
  REQUIRE(subtracting.getTime() == previousValue);
}

TEST_CASE(
    "IEEE 1516.1-2025 reference time values satisfy the logical-time identity set",
    "[2025][time][unit][time-management][logical-time-standard-identities]") {
  requireLogicalTimeIdentitySet<HLAinteger64Time, HLAinteger64Interval>();
  requireLogicalTimeIdentitySet<HLAfloat64Time, HLAfloat64Interval>();
}

TEST_CASE("HLAfloat64Time rejects nonfinite encodings and factory-decodes HLAfloat64BE", "[baseline][time][unit][foundation][time-management]") {
  HLAfloat64Time value;
  std::array<unsigned char, 8> const positiveInfinity{0x7F, 0xF0, 0, 0, 0, 0, 0, 0};

  REQUIRE_THROWS_AS(value.decode(positiveInfinity.data(), positiveInfinity.size()), CouldNotDecode);

  HLAfloat64TimeFactory factory;
  std::array<unsigned char, 8> const encoded{0x40, 0x0A, 0, 0, 0, 0, 0, 0};
  auto decoded = factory.decodeLogicalTime(encoded.data(), encoded.size());
  auto* concrete = dynamic_cast<HLAfloat64Time*>(decoded.get());
  REQUIRE(concrete != nullptr);
  REQUIRE(concrete->getTime() == 3.25);
  REQUIRE(factory.getName() == L"HLAfloat64Time");
}

TEST_CASE("IEEE reference factory selection returns both mandated names and defaults to HLAfloat64Time", "[baseline][time][unit][foundation][time-management]") {
  auto defaultFactory = rti1516_2025::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(L"");
  REQUIRE(defaultFactory);
  REQUIRE(defaultFactory->getName() == L"HLAfloat64Time");
  REQUIRE(dynamic_cast<HLAfloat64TimeFactory*>(defaultFactory.get()) != nullptr);

  auto integerFactory =
      rti1516_2025::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(L"HLAinteger64Time");
  REQUIRE(integerFactory);
  REQUIRE(integerFactory->getName() == L"HLAinteger64Time");
  REQUIRE(dynamic_cast<HLAinteger64TimeFactory*>(integerFactory.get()) != nullptr);

  auto fedtimeDefault = LogicalTimeFactoryFactory::makeLogicalTimeFactory(L"");
  REQUIRE(fedtimeDefault);
  REQUIRE(fedtimeDefault->getName() == L"HLAfloat64Time");
  REQUIRE_FALSE(rti1516_2025::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(L"missing"));
  REQUIRE_FALSE(LogicalTimeFactoryFactory::makeLogicalTimeFactory(L"missing"));

  std::wostringstream stream;
  auto time = defaultFactory->makeInitial();
  stream << *time;
  REQUIRE(stream.str() == L"0");
}

TEST_CASE(
    "HLAlogicalTime delegates opaque bytes to the selected reference factory",
    "[baseline][time][logical-time-encoding][unit][foundation][time-management]"
    "[logical-time-opaque-factory-encoding]") {
  ReferenceTimeAmbassador floatAmbassador(L"HLAfloat64Time");
  ReferenceTimeAmbassador integerAmbassador(L"HLAinteger64Time");
  HLAlogicalTime value(&floatAmbassador);
  HLAlogicalTime integerValue(&integerAmbassador);

  std::array<unsigned char, 8> const initialBytes{};
  REQUIRE(encodedEquals(value.encode(), initialBytes));
  REQUIRE(value.getEncodedLength() == initialBytes.size());
  REQUIRE(value.getOctetBoundary() == 1U);
  REQUIRE_FALSE(value.isSameTypeAs(integerValue));
  REQUIRE_THROWS_AS(HLAlogicalTime(nullptr), EncoderException);

  HLAfloat64Time source(1.5);
  value.set(source);
  std::array<unsigned char, 8> const expected{0x3F, 0xF8, 0, 0, 0, 0, 0, 0};
  REQUIRE(encodedEquals(value.encode(), expected));

  std::vector<Octet> nested{static_cast<Octet>(0xA4)};
  value.encodeInto(nested);
  nested.push_back(0x5A);
  HLAlogicalTime decoded(&floatAmbassador);
  REQUIRE(decoded.decodeFrom(nested, 1U) == 9U);
  REQUIRE(static_cast<unsigned char>(nested[0]) == 0xA4);
  REQUIRE(nested[9] == 0x5A);
  HLAfloat64Time recovered;
  decoded.get(recovered);
  REQUIRE(recovered.getTime() == 1.5);

  HLAlogicalTime copied(value);
  auto clone = value.clone();
  auto* cloned = dynamic_cast<HLAlogicalTime*>(clone.get());
  REQUIRE(cloned != nullptr);
  value.set(HLAfloat64Time(2.0));
  HLAfloat64Time copiedValue;
  HLAfloat64Time clonedValue;
  copied.get(copiedValue);
  cloned->get(clonedValue);
  REQUIRE(copiedValue.getTime() == 1.5);
  REQUIRE(clonedValue.getTime() == 1.5);

  HLAinteger64Time incompatibleTime;
  REQUIRE_THROWS_AS(value.set(incompatibleTime), EncoderException);
  REQUIRE_THROWS_AS(value.get(incompatibleTime), InvalidLogicalTime);

  std::array<unsigned char, 7> const truncated{};
  VariableLengthData malformed(truncated.data(), truncated.size());
  REQUIRE_THROWS_AS(decoded.decode(malformed), CouldNotDecode);
  std::vector<Octet> const tooShort(truncated.begin(), truncated.end());
  REQUIRE_THROWS_AS(decoded.decodeFrom(tooShort, 0U), EncoderException);
}

TEST_CASE(
    "HLAlogicalTimeInterval delegates opaque bytes to the selected reference factory",
    "[baseline][time][logical-time-encoding][unit][foundation][time-management]"
    "[logical-time-opaque-factory-encoding]") {
  ReferenceTimeAmbassador integerAmbassador(L"HLAinteger64Time");
  ReferenceTimeAmbassador floatAmbassador(L"HLAfloat64Time");
  HLAlogicalTimeInterval value(&integerAmbassador);
  HLAlogicalTimeInterval floatValue(&floatAmbassador);

  std::array<unsigned char, 8> const initialBytes{};
  REQUIRE(encodedEquals(value.encode(), initialBytes));
  REQUIRE(value.getEncodedLength() == initialBytes.size());
  REQUIRE(value.getOctetBoundary() == 1U);
  REQUIRE_FALSE(value.isSameTypeAs(floatValue));
  REQUIRE_THROWS_AS(HLAlogicalTimeInterval(nullptr), EncoderException);

  HLAinteger64Interval source(9);
  value.set(source);
  std::array<unsigned char, 8> const expected{0, 0, 0, 0, 0, 0, 0, 9};
  REQUIRE(encodedEquals(value.encode(), expected));

  std::vector<Octet> nested{static_cast<Octet>(0xA4)};
  value.encodeInto(nested);
  nested.push_back(0x5A);
  HLAlogicalTimeInterval decoded(&integerAmbassador);
  REQUIRE(decoded.decodeFrom(nested, 1U) == 9U);
  REQUIRE(static_cast<unsigned char>(nested[0]) == 0xA4);
  REQUIRE(nested[9] == 0x5A);
  HLAinteger64Interval recovered;
  decoded.get(recovered);
  REQUIRE(recovered.getInterval() == 9);

  HLAlogicalTimeInterval copied(value);
  auto clone = value.clone();
  auto* cloned = dynamic_cast<HLAlogicalTimeInterval*>(clone.get());
  REQUIRE(cloned != nullptr);
  value.set(HLAinteger64Interval(12));
  HLAinteger64Interval copiedValue;
  HLAinteger64Interval clonedValue;
  copied.get(copiedValue);
  cloned->get(clonedValue);
  REQUIRE(copiedValue.getInterval() == 9);
  REQUIRE(clonedValue.getInterval() == 9);

  HLAfloat64Interval incompatibleInterval;
  REQUIRE_THROWS_AS(value.set(incompatibleInterval), EncoderException);
  REQUIRE_THROWS_AS(value.get(incompatibleInterval), InvalidLogicalTimeInterval);

  std::array<unsigned char, 7> const truncated{};
  VariableLengthData malformed(truncated.data(), truncated.size());
  REQUIRE_THROWS_AS(decoded.decode(malformed), CouldNotDecode);
  std::vector<Octet> const tooShort(truncated.begin(), truncated.end());
  REQUIRE_THROWS_AS(decoded.decodeFrom(tooShort, 0U), EncoderException);
}

TEST_CASE(
    "IEEE 1516.1-2025 logical-time intervals test zero alongside interval arithmetic",
    "[2025][time][unit][time-management][logical-time-interval-zero]") {
  HLAinteger64Interval integerZero;
  HLAinteger64Interval integerEpsilon;
  integerZero.setZero();
  integerEpsilon.setEpsilon();
  REQUIRE(integerZero.isZero());
  REQUIRE(integerZero.getInterval() == 0);
  integerZero += integerEpsilon;
  REQUIRE(integerZero.getInterval() == 1);
  integerZero -= integerEpsilon;
  REQUIRE(integerZero.isZero());

  HLAfloat64Interval floatZero;
  HLAfloat64Interval floatEpsilon;
  floatZero.setZero();
  floatEpsilon.setEpsilon();
  REQUIRE(floatZero.isZero());
  REQUIRE(floatZero.getInterval() == 0.0);
  floatZero += floatEpsilon;
  REQUIRE(floatZero.getInterval() == std::numeric_limits<double>::denorm_min());
  floatZero -= floatEpsilon;
  REQUIRE(floatZero.isZero());
}

TEST_CASE(
    "IEEE 1516.1-2025 time and interval subtraction clamp at their lower bounds",
    "[2025][time][unit][time-management][logical-time-lower-bound-clamp]") {
  HLAinteger64Time integerInitial;
  HLAinteger64Interval integerEpsilon;
  integerEpsilon.setEpsilon();
  integerInitial -= integerEpsilon;
  REQUIRE(integerInitial.isInitial());
  REQUIRE(integerInitial.getTime() == 0);

  HLAfloat64Time floatInitial;
  HLAfloat64Interval floatEpsilon;
  floatEpsilon.setEpsilon();
  floatInitial -= floatEpsilon;
  REQUIRE(floatInitial.isInitial());
  REQUIRE(floatInitial.getTime() == 0.0);

  HLAinteger64Interval integerZero;
  integerZero -= integerEpsilon;
  REQUIRE(integerZero.isZero());
  REQUIRE(integerZero.getInterval() == 0);

  HLAfloat64Interval floatZero;
  floatZero -= floatEpsilon;
  REQUIRE(floatZero.isZero());
  REQUIRE(floatZero.getInterval() == 0.0);
}

TEST_CASE(
    "IEEE 1516.1-2025 logical-time arithmetic and comparisons enforce operand types",
    "[2025][time][unit][time-management][logical-time-operand-types]") {
  STATIC_REQUIRE(SupportsAdditionAssignment<LogicalTime, LogicalTimeInterval>);
  STATIC_REQUIRE(SupportsSubtractionAssignment<LogicalTime, LogicalTimeInterval>);
  STATIC_REQUIRE_FALSE(SupportsAdditionAssignment<LogicalTime, LogicalTime>);
  STATIC_REQUIRE_FALSE(SupportsSubtractionAssignment<LogicalTime, LogicalTime>);
  STATIC_REQUIRE(SupportsAdditionAssignment<LogicalTimeInterval, LogicalTimeInterval>);
  STATIC_REQUIRE(SupportsSubtractionAssignment<LogicalTimeInterval, LogicalTimeInterval>);
  STATIC_REQUIRE_FALSE(SupportsAdditionAssignment<LogicalTimeInterval, LogicalTime>);
  STATIC_REQUIRE_FALSE(SupportsSubtractionAssignment<LogicalTimeInterval, LogicalTime>);
  STATIC_REQUIRE(SupportsAllComparisons<LogicalTime, LogicalTime>);
  STATIC_REQUIRE(SupportsAllComparisons<LogicalTimeInterval, LogicalTimeInterval>);
  STATIC_REQUIRE_FALSE(SupportsAllComparisons<LogicalTime, LogicalTimeInterval>);
  STATIC_REQUIRE_FALSE(SupportsAllComparisons<LogicalTimeInterval, LogicalTime>);
}

TEST_CASE(
    "IEEE 1516.1-2025 logical-time interfaces expose no additional arithmetic operators",
    "[2025][time][unit][time-management][logical-time-no-other-arithmetic]") {
  STATIC_REQUIRE_FALSE(SupportsAdditionalBinaryArithmetic<LogicalTime, LogicalTime>);
  STATIC_REQUIRE_FALSE(SupportsAdditionalBinaryArithmetic<LogicalTime, LogicalTimeInterval>);
  STATIC_REQUIRE_FALSE(SupportsAdditionalBinaryArithmetic<LogicalTimeInterval, LogicalTime>);
  STATIC_REQUIRE_FALSE(
      SupportsAdditionalBinaryArithmetic<LogicalTimeInterval, LogicalTimeInterval>);
  STATIC_REQUIRE_FALSE(SupportsAdditionalUnaryArithmetic<LogicalTime>);
  STATIC_REQUIRE_FALSE(SupportsAdditionalUnaryArithmetic<LogicalTimeInterval>);
  STATIC_REQUIRE_FALSE(SupportsIncrementOrDecrement<LogicalTime>);
  STATIC_REQUIRE_FALSE(SupportsIncrementOrDecrement<LogicalTimeInterval>);
  STATIC_REQUIRE_FALSE(
      SupportsAdditionalCompoundArithmetic<LogicalTime, LogicalTimeInterval>);
  STATIC_REQUIRE_FALSE(
      SupportsAdditionalCompoundArithmetic<LogicalTimeInterval, LogicalTimeInterval>);
}
