#include <RTI/LogicalTime.h>
#include <RTI/LogicalTimeInterval.h>
#include <RTI/VariableLengthData.h>
#include <RTI/time/HLAfloat64Interval.h>
#include <RTI/time/HLAfloat64Time.h>
#include <RTI/time/HLAfloat64TimeFactory.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>
#include <RTI/time/HLAinteger64TimeFactory.h>

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>

namespace {

constexpr std::size_t kEncodedLength = 8U;

void requireSameEncoding(
    rti1516e::VariableLengthData const& expected,
    rti1516e::VariableLengthData const& actual) {
  assert(expected.size() == actual.size());
  assert(std::memcmp(expected.data(), actual.data(), expected.size()) == 0);
}

void checkIntegerTime() {
  using namespace rti1516e;
  HLAinteger64TimeFactory factory;
  std::array<Integer64, 4> values = {{0, 1, 123456789, (std::numeric_limits<Integer64>::max)()}};
  for (Integer64 value : values) {
    auto source = factory.makeLogicalTime(value);
    auto encoded = source->encode();
    assert(source->encodedLength() == kEncodedLength);
    auto decoded = factory.decodeLogicalTime(encoded);
    auto* concrete = dynamic_cast<HLAinteger64Time*>(decoded.get());
    assert(concrete != nullptr);
    assert(concrete->getTime() == value);
    requireSameEncoding(encoded, decoded->encode());

    unsigned char output[kEncodedLength + 1U] = {};
    assert(source->encode(output, sizeof(output)) == kEncodedLength);
    assert(std::memcmp(output, encoded.data(), kEncodedLength) == 0);
  }

  std::array<Integer64, 4> intervals = {{0, 1, 1234, (std::numeric_limits<Integer64>::max)()}};
  for (Integer64 value : intervals) {
    auto source = factory.makeLogicalTimeInterval(value);
    auto encoded = source->encode();
    auto decoded = factory.decodeLogicalTimeInterval(encoded);
    auto* concrete = dynamic_cast<HLAinteger64Interval*>(decoded.get());
    assert(concrete != nullptr);
    assert(concrete->getInterval() == value);
    requireSameEncoding(encoded, decoded->encode());
  }

  unsigned char negative[kEncodedLength] = {0x80, 0, 0, 0, 0, 0, 0, 0};
  bool rejected = false;
  try {
    factory.decodeLogicalTime(VariableLengthData(negative, sizeof(negative)));
  } catch (CouldNotDecode const&) {
    rejected = true;
  }
  assert(rejected);
  rejected = false;
  try {
    factory.decodeLogicalTimeInterval(VariableLengthData(negative, sizeof(negative)));
  } catch (CouldNotDecode const&) {
    rejected = true;
  }
  assert(rejected);
}

void checkFloatTime() {
  using namespace rti1516e;
  HLAfloat64TimeFactory factory;
  std::array<double, 4> values = {{0.0, 1.25, 12.5, (std::numeric_limits<double>::max)()}};
  for (double value : values) {
    auto source = factory.makeLogicalTime(value);
    auto encoded = source->encode();
    assert(source->encodedLength() == kEncodedLength);
    auto decoded = factory.decodeLogicalTime(encoded);
    auto* concrete = dynamic_cast<HLAfloat64Time*>(decoded.get());
    assert(concrete != nullptr);
    assert(concrete->getTime() == value);
    requireSameEncoding(encoded, decoded->encode());
  }

  std::array<double, 4> intervals = {{0.0, 0.25, 2.5, (std::numeric_limits<double>::max)()}};
  for (double value : intervals) {
    auto source = factory.makeLogicalTimeInterval(value);
    auto encoded = source->encode();
    auto decoded = factory.decodeLogicalTimeInterval(encoded);
    auto* concrete = dynamic_cast<HLAfloat64Interval*>(decoded.get());
    assert(concrete != nullptr);
    assert(concrete->getInterval() == value);
    requireSameEncoding(encoded, decoded->encode());
  }

  unsigned char negative[kEncodedLength] = {0xbf, 0xf0, 0, 0, 0, 0, 0, 0};
  unsigned char nan[kEncodedLength] = {0x7f, 0xf8, 0, 0, 0, 0, 0, 0};
  for (auto const& value : {negative, nan}) {
    bool rejected = false;
    try {
      factory.decodeLogicalTime(VariableLengthData(value, kEncodedLength));
    } catch (CouldNotDecode const&) {
      rejected = true;
    }
    assert(rejected);
    rejected = false;
    try {
      factory.decodeLogicalTimeInterval(VariableLengthData(value, kEncodedLength));
    } catch (CouldNotDecode const&) {
      rejected = true;
    }
    assert(rejected);
  }
}

}  // namespace

int main() {
  checkIntegerTime();
  checkFloatTime();
  return 0;
}
