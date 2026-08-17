#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <sstream>

#include <RTI/RTI1516.h>
#include <RTI/time/HLAfloat64Interval.h>
#include <RTI/time/HLAfloat64Time.h>
#include <RTI/time/HLAfloat64TimeFactory.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>
#include <RTI/time/HLAinteger64TimeFactory.h>

namespace {

using rti1516_2025::CouldNotDecode;
using rti1516_2025::CouldNotEncode;
using rti1516_2025::HLAfloat64Interval;
using rti1516_2025::HLAfloat64Time;
using rti1516_2025::HLAfloat64TimeFactory;
using rti1516_2025::HLAinteger64Interval;
using rti1516_2025::HLAinteger64Time;
using rti1516_2025::HLAinteger64TimeFactory;
using rti1516_2025::IllegalTimeArithmetic;
using rti1516_2025::Integer64;
using rti1516_2025::InvalidLogicalTime;
using rti1516_2025::LogicalTimeFactory;
using rti1516_2025::LogicalTimeFactoryFactory;
using rti1516_2025::VariableLengthData;

template <std::size_t Size>
bool encodedEquals(VariableLengthData const& actual, std::array<unsigned char, Size> const& expected) {
  return actual.size() == expected.size() &&
         std::memcmp(actual.data(), expected.data(), expected.size()) == 0;
}

}  // namespace

TEST_CASE(
    "HLAinteger64Time implements the IEEE reference values, arithmetic, and HLAinteger64BE encoding",
    "[baseline][time]") {
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
  REQUIRE_THROWS_AS(initialTime -= epsilon, IllegalTimeArithmetic);
  REQUIRE_THROWS_AS(finalTime += epsilon, IllegalTimeArithmetic);
  REQUIRE_THROWS_AS(HLAinteger64Time(-1), InvalidLogicalTime);
}

TEST_CASE("HLAinteger64Time reports malformed encodings and factory-decodes valid values", "[baseline][time]") {
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
    "[baseline][time]") {
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

  REQUIRE_THROWS_AS(finalTime += epsilon, IllegalTimeArithmetic);
  REQUIRE_THROWS_AS(HLAfloat64Time(std::numeric_limits<double>::infinity()), InvalidLogicalTime);
}

TEST_CASE("HLAfloat64Time rejects nonfinite encodings and factory-decodes HLAfloat64BE", "[baseline][time]") {
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

TEST_CASE("IEEE reference factory selection returns both mandated names and defaults to HLAfloat64Time", "[baseline][time]") {
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
