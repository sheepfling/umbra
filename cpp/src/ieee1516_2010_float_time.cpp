#include <RTI/VariableLengthData.h>
#include <RTI/LogicalTime.h>
#include <RTI/LogicalTimeInterval.h>
#include <RTI/time/HLAfloat64Interval.h>
#include <RTI/time/HLAfloat64Time.h>
#include <RTI/time/HLAfloat64TimeFactory.h>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <limits>
#include <memory>
#include <sstream>
#include <string>

namespace rti1516e {

namespace {

constexpr std::size_t kEncodedLength = 8U;
constexpr double kFinalValue = (std::numeric_limits<double>::max)();
constexpr double kEpsilon = (std::numeric_limits<double>::denorm_min)();

[[noreturn]] void invalidTime(wchar_t const* message) {
  throw InvalidLogicalTime(message);
}

[[noreturn]] void invalidInterval(wchar_t const* message) {
  throw InvalidLogicalTimeInterval(message);
}

[[noreturn]] void illegalArithmetic(wchar_t const* message) {
  throw IllegalTimeArithmetic(message);
}

[[noreturn]] void couldNotDecode(wchar_t const* message) {
  throw CouldNotDecode(message);
}

[[noreturn]] void couldNotEncode(wchar_t const* message) {
  throw CouldNotEncode(message);
}

bool valid(double value) {
  return std::isfinite(value) && value >= 0.0;
}

double canonicalZero(double value) {
  return value == 0.0 ? 0.0 : value;
}

std::uint64_t bitsOf(double value) {
  std::uint64_t bits = 0U;
  std::memcpy(&bits, &value, sizeof(bits));
  return bits;
}

double doubleOf(std::uint64_t bits) {
  double value = 0.0;
  std::memcpy(&value, &bits, sizeof(value));
  return value;
}

void appendBigEndian(std::uint64_t value, unsigned char* output) {
  for (std::size_t index = kEncodedLength; index > 0U; --index) {
    output[index - 1U] = static_cast<unsigned char>(value & 0xffU);
    value >>= 8U;
  }
}

std::uint64_t readBigEndian(void const* buffer, std::size_t size) {
  if (buffer == nullptr || size != kEncodedLength) {
    couldNotDecode(L"An HLAfloat64Time encoding must contain exactly eight octets.");
  }
  auto const* bytes = static_cast<unsigned char const*>(buffer);
  std::uint64_t value = 0U;
  for (std::size_t index = 0U; index < kEncodedLength; ++index) {
    value = (value << 8U) | bytes[index];
  }
  return value;
}

VariableLengthData encoded(double value) {
  unsigned char bytes[kEncodedLength] = {};
  appendBigEndian(bitsOf(value), bytes);
  return VariableLengthData(bytes, kEncodedLength);
}

std::size_t writeEncoded(double value, void* buffer, std::size_t size) {
  if (buffer == nullptr || size < kEncodedLength) {
    couldNotEncode(L"The output buffer is too small for an HLAfloat64Time encoding.");
  }
  unsigned char bytes[kEncodedLength] = {};
  appendBigEndian(bitsOf(value), bytes);
  std::memcpy(buffer, bytes, kEncodedLength);
  return kEncodedLength;
}

double asTime(LogicalTime const& value) {
  auto const* concrete = dynamic_cast<HLAfloat64Time const*>(&value);
  if (concrete == nullptr) {
    invalidTime(L"The logical-time value is not HLAfloat64Time.");
  }
  return concrete->getTime();
}

double asInterval(LogicalTimeInterval const& value) {
  auto const* concrete = dynamic_cast<HLAfloat64Interval const*>(&value);
  if (concrete == nullptr) {
    invalidInterval(L"The logical-time interval is not HLAfloat64Interval.");
  }
  return concrete->getInterval();
}

double addValue(double value, double increment) {
  if (value == kFinalValue && increment == 0.0) {
    return value;
  }
  if (value > kFinalValue - increment) {
    illegalArithmetic(L"HLAfloat64Time addition exceeds the final value.");
  }
  auto result = value + increment;
  if (!valid(result)) {
    illegalArithmetic(L"HLAfloat64Time addition produced an invalid value.");
  }
  if (increment == kEpsilon && result == value) {
    result = std::nextafter(value, kFinalValue);
  }
  return canonicalZero(result);
}

double subtractValue(double value, double decrement) {
  if (decrement > value) {
    illegalArithmetic(L"HLAfloat64Time subtraction precedes the initial value.");
  }
  return canonicalZero(value - decrement);
}

std::wstring stringValue(double value) {
  std::wostringstream stream;
  stream << std::setprecision((std::numeric_limits<double>::max_digits10)) << value;
  return stream.str();
}

}  // namespace

class HLAfloat64TimeImpl {
 public:
  double value = 0.0;
};

class HLAfloat64IntervalImpl {
 public:
  double value = 0.0;
};

HLAfloat64Time::HLAfloat64Time() : _impl(new HLAfloat64TimeImpl()) {}

HLAfloat64Time::HLAfloat64Time(double const& value)
    : _impl(new HLAfloat64TimeImpl()) {
  setTime(value);
}

HLAfloat64Time::HLAfloat64Time(LogicalTime const& value)
    : _impl(new HLAfloat64TimeImpl()) {
  setTime(asTime(value));
}

HLAfloat64Time::HLAfloat64Time(HLAfloat64Time const& value)
    : _impl(new HLAfloat64TimeImpl()) {
  _impl->value = value._impl->value;
}

HLAfloat64Time::~HLAfloat64Time() throw() { delete _impl; }

void HLAfloat64Time::setInitial() { _impl->value = 0.0; }
bool HLAfloat64Time::isInitial() const { return _impl->value == 0.0; }
void HLAfloat64Time::setFinal() { _impl->value = kFinalValue; }
bool HLAfloat64Time::isFinal() const { return _impl->value == kFinalValue; }

LogicalTime& HLAfloat64Time::operator=(LogicalTime const& value) {
  _impl->value = asTime(value);
  return *this;
}

LogicalTime& HLAfloat64Time::operator+=(LogicalTimeInterval const& addend) {
  _impl->value = addValue(_impl->value, asInterval(addend));
  return *this;
}

LogicalTime& HLAfloat64Time::operator-=(LogicalTimeInterval const& subtrahend) {
  _impl->value = subtractValue(_impl->value, asInterval(subtrahend));
  return *this;
}

bool HLAfloat64Time::operator>(LogicalTime const& value) const { return _impl->value > asTime(value); }
bool HLAfloat64Time::operator<(LogicalTime const& value) const { return _impl->value < asTime(value); }
bool HLAfloat64Time::operator==(LogicalTime const& value) const { return _impl->value == asTime(value); }
bool HLAfloat64Time::operator>=(LogicalTime const& value) const { return _impl->value >= asTime(value); }
bool HLAfloat64Time::operator<=(LogicalTime const& value) const { return _impl->value <= asTime(value); }

VariableLengthData HLAfloat64Time::encode() const { return encoded(_impl->value); }
size_t HLAfloat64Time::encode(void* buffer, size_t bufferSize) const {
  return writeEncoded(_impl->value, buffer, bufferSize);
}
size_t HLAfloat64Time::encodedLength() const { return kEncodedLength; }

void HLAfloat64Time::decode(VariableLengthData const& value) {
  decode(const_cast<void*>(value.data()), value.size());
}

void HLAfloat64Time::decode(void* buffer, size_t bufferSize) {
  auto const value = doubleOf(readBigEndian(buffer, bufferSize));
  if (!valid(value)) {
    couldNotDecode(L"HLAfloat64Time must decode to a finite nonnegative value.");
  }
  _impl->value = canonicalZero(value);
}

std::wstring HLAfloat64Time::toString() const { return stringValue(_impl->value); }
std::wstring HLAfloat64Time::implementationName() const { return HLAfloat64TimeName; }
double HLAfloat64Time::getTime() const { return _impl->value; }
void HLAfloat64Time::setTime(double value) {
  if (!valid(value)) {
    invalidTime(L"HLAfloat64Time must be finite and nonnegative.");
  }
  _impl->value = canonicalZero(value);
}
HLAfloat64Time& HLAfloat64Time::operator=(HLAfloat64Time const& value) {
  if (this != &value) _impl->value = value._impl->value;
  return *this;
}
HLAfloat64Time::operator double() const { return _impl->value; }

HLAfloat64Interval::HLAfloat64Interval() : _impl(new HLAfloat64IntervalImpl()) {}
HLAfloat64Interval::HLAfloat64Interval(double value)
    : _impl(new HLAfloat64IntervalImpl()) {
  setInterval(value);
}
HLAfloat64Interval::HLAfloat64Interval(LogicalTimeInterval const& value)
    : _impl(new HLAfloat64IntervalImpl()) {
  setInterval(asInterval(value));
}
HLAfloat64Interval::HLAfloat64Interval(HLAfloat64Interval const& value)
    : _impl(new HLAfloat64IntervalImpl()) {
  _impl->value = value._impl->value;
}
HLAfloat64Interval::~HLAfloat64Interval() throw() { delete _impl; }
void HLAfloat64Interval::setZero() { _impl->value = 0.0; }
bool HLAfloat64Interval::isZero() const { return _impl->value == 0.0; }
void HLAfloat64Interval::setEpsilon() { _impl->value = kEpsilon; }
bool HLAfloat64Interval::isEpsilon() const { return _impl->value == kEpsilon; }

LogicalTimeInterval& HLAfloat64Interval::operator=(LogicalTimeInterval const& value) {
  _impl->value = asInterval(value);
  return *this;
}
LogicalTimeInterval& HLAfloat64Interval::operator+=(LogicalTimeInterval const& addend) {
  _impl->value = addValue(_impl->value, asInterval(addend));
  return *this;
}
LogicalTimeInterval& HLAfloat64Interval::operator-=(LogicalTimeInterval const& subtrahend) {
  _impl->value = subtractValue(_impl->value, asInterval(subtrahend));
  return *this;
}
bool HLAfloat64Interval::operator>(LogicalTimeInterval const& value) const { return _impl->value > asInterval(value); }
bool HLAfloat64Interval::operator<(LogicalTimeInterval const& value) const { return _impl->value < asInterval(value); }
bool HLAfloat64Interval::operator==(LogicalTimeInterval const& value) const { return _impl->value == asInterval(value); }
bool HLAfloat64Interval::operator>=(LogicalTimeInterval const& value) const { return _impl->value >= asInterval(value); }
bool HLAfloat64Interval::operator<=(LogicalTimeInterval const& value) const { return _impl->value <= asInterval(value); }

void HLAfloat64Interval::setToDifference(
    LogicalTime const& minuend, LogicalTime const& subtrahend) {
  auto const left = asTime(minuend);
  auto const right = asTime(subtrahend);
  if (right > left) {
    illegalArithmetic(L"HLAfloat64Time difference would be negative.");
  }
  _impl->value = canonicalZero(left - right);
}

VariableLengthData HLAfloat64Interval::encode() const { return encoded(_impl->value); }
size_t HLAfloat64Interval::encode(void* buffer, size_t bufferSize) const {
  return writeEncoded(_impl->value, buffer, bufferSize);
}
size_t HLAfloat64Interval::encodedLength() const { return kEncodedLength; }
void HLAfloat64Interval::decode(VariableLengthData const& value) {
  decode(const_cast<void*>(value.data()), value.size());
}
void HLAfloat64Interval::decode(void* buffer, size_t bufferSize) {
  auto const value = doubleOf(readBigEndian(buffer, bufferSize));
  if (!valid(value)) {
    couldNotDecode(L"HLAfloat64Interval must decode to a finite nonnegative value.");
  }
  _impl->value = canonicalZero(value);
}
std::wstring HLAfloat64Interval::toString() const { return stringValue(_impl->value); }
std::wstring HLAfloat64Interval::implementationName() const { return HLAfloat64TimeName; }
double HLAfloat64Interval::getInterval() const { return _impl->value; }
void HLAfloat64Interval::setInterval(double value) {
  if (!valid(value)) {
    invalidInterval(L"HLAfloat64Interval must be finite and nonnegative.");
  }
  _impl->value = canonicalZero(value);
}
HLAfloat64Interval& HLAfloat64Interval::operator=(HLAfloat64Interval const& value) {
  if (this != &value) _impl->value = value._impl->value;
  return *this;
}
HLAfloat64Interval::operator double() const { return _impl->value; }

HLAfloat64TimeFactory::HLAfloat64TimeFactory() = default;
HLAfloat64TimeFactory::~HLAfloat64TimeFactory() throw() = default;

std::auto_ptr<HLAfloat64Time> HLAfloat64TimeFactory::makeLogicalTime(double value) {
  return std::auto_ptr<HLAfloat64Time>(new HLAfloat64Time(value));
}
std::auto_ptr<LogicalTime> HLAfloat64TimeFactory::makeInitial() {
  return std::auto_ptr<LogicalTime>(new HLAfloat64Time(0.0));
}
std::auto_ptr<LogicalTime> HLAfloat64TimeFactory::makeFinal() {
  return std::auto_ptr<LogicalTime>(new HLAfloat64Time(kFinalValue));
}
std::auto_ptr<HLAfloat64Interval> HLAfloat64TimeFactory::makeLogicalTimeInterval(double value) {
  return std::auto_ptr<HLAfloat64Interval>(new HLAfloat64Interval(value));
}
std::auto_ptr<LogicalTimeInterval> HLAfloat64TimeFactory::makeZero() {
  return std::auto_ptr<LogicalTimeInterval>(new HLAfloat64Interval(0.0));
}
std::auto_ptr<LogicalTimeInterval> HLAfloat64TimeFactory::makeEpsilon() {
  return std::auto_ptr<LogicalTimeInterval>(new HLAfloat64Interval(kEpsilon));
}
std::auto_ptr<LogicalTime> HLAfloat64TimeFactory::decodeLogicalTime(VariableLengthData const& value) {
  auto result = std::auto_ptr<HLAfloat64Time>(new HLAfloat64Time());
  result->decode(value);
  return std::auto_ptr<LogicalTime>(result.release());
}
std::auto_ptr<LogicalTime> HLAfloat64TimeFactory::decodeLogicalTime(void* buffer, size_t bufferSize) {
  auto result = std::auto_ptr<HLAfloat64Time>(new HLAfloat64Time());
  result->decode(buffer, bufferSize);
  return std::auto_ptr<LogicalTime>(result.release());
}
std::auto_ptr<LogicalTimeInterval> HLAfloat64TimeFactory::decodeLogicalTimeInterval(VariableLengthData const& value) {
  auto result = std::auto_ptr<HLAfloat64Interval>(new HLAfloat64Interval());
  result->decode(value);
  return std::auto_ptr<LogicalTimeInterval>(result.release());
}
std::auto_ptr<LogicalTimeInterval> HLAfloat64TimeFactory::decodeLogicalTimeInterval(void* buffer, size_t bufferSize) {
  auto result = std::auto_ptr<HLAfloat64Interval>(new HLAfloat64Interval());
  result->decode(buffer, bufferSize);
  return std::auto_ptr<LogicalTimeInterval>(result.release());
}
std::wstring HLAfloat64TimeFactory::getName() const { return HLAfloat64TimeName; }

}  // namespace rti1516e
