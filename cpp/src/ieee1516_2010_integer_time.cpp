#include <RTI/VariableLengthData.h>
#include <RTI/LogicalTime.h>
#include <RTI/LogicalTimeFactory.h>
#include <RTI/LogicalTimeInterval.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>
#include <RTI/time/HLAinteger64TimeFactory.h>
#include <RTI/time/HLAfloat64TimeFactory.h>

#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <string>

namespace rti1516e {

namespace {

constexpr std::size_t kEncodedLength = 8U;
constexpr Integer64 kFinalValue = std::numeric_limits<Integer64>::max();

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

bool valid(Integer64 value) {
  return value >= 0;
}

void appendBigEndian(std::uint64_t value, unsigned char* output) {
  for (std::size_t index = kEncodedLength; index > 0U; --index) {
    output[index - 1U] = static_cast<unsigned char>(value & 0xffU);
    value >>= 8U;
  }
}

std::uint64_t readBigEndian(void const* buffer, std::size_t size) {
  if (buffer == nullptr || size != kEncodedLength) {
    couldNotDecode(L"An HLAinteger64Time encoding must contain exactly eight octets.");
  }
  auto const* bytes = static_cast<unsigned char const*>(buffer);
  std::uint64_t value = 0U;
  for (std::size_t index = 0U; index < kEncodedLength; ++index) {
    value = (value << 8U) | bytes[index];
  }
  if ((value & (std::uint64_t{1} << 63U)) != 0U) {
    couldNotDecode(L"An HLAinteger64Time encoding contains a negative value.");
  }
  return value;
}

VariableLengthData encoded(Integer64 value) {
  unsigned char bytes[kEncodedLength] = {};
  appendBigEndian(static_cast<std::uint64_t>(value), bytes);
  return VariableLengthData(bytes, kEncodedLength);
}

std::size_t writeEncoded(Integer64 value, void* buffer, std::size_t size) {
  if (buffer == nullptr || size < kEncodedLength) {
    couldNotEncode(L"The output buffer is too small for an HLAinteger64Time encoding.");
  }
  unsigned char bytes[kEncodedLength] = {};
  appendBigEndian(static_cast<std::uint64_t>(value), bytes);
  std::memcpy(buffer, bytes, kEncodedLength);
  return kEncodedLength;
}

HLAinteger64Time const& asTime(LogicalTime const& value) {
  auto const* concrete = dynamic_cast<HLAinteger64Time const*>(&value);
  if (concrete == nullptr) {
    invalidTime(L"The logical-time value is not HLAinteger64Time.");
  }
  return *concrete;
}

HLAinteger64Interval const& asInterval(LogicalTimeInterval const& value) {
  auto const* concrete = dynamic_cast<HLAinteger64Interval const*>(&value);
  if (concrete == nullptr) {
    invalidInterval(L"The logical-time interval is not HLAinteger64Interval.");
  }
  return *concrete;
}

}  // namespace

class HLAinteger64TimeImpl {
 public:
  Integer64 value = 0;
};

class HLAinteger64IntervalImpl {
 public:
  Integer64 value = 0;
};

HLAinteger64Time::HLAinteger64Time()
    : _impl(new HLAinteger64TimeImpl()) {}

HLAinteger64Time::HLAinteger64Time(Integer64 value)
    : _impl(new HLAinteger64TimeImpl()) {
  setTime(value);
}

HLAinteger64Time::HLAinteger64Time(LogicalTime const& value)
    : _impl(new HLAinteger64TimeImpl()) {
  setTime(asTime(value).getTime());
}

HLAinteger64Time::HLAinteger64Time(HLAinteger64Time const& value)
    : _impl(new HLAinteger64TimeImpl()) {
  _impl->value = value._impl->value;
}

HLAinteger64Time::~HLAinteger64Time() throw() {
  delete _impl;
}

void HLAinteger64Time::setInitial() {
  _impl->value = 0;
}

bool HLAinteger64Time::isInitial() const {
  return _impl->value == 0;
}

void HLAinteger64Time::setFinal() {
  _impl->value = kFinalValue;
}

bool HLAinteger64Time::isFinal() const {
  return _impl->value == kFinalValue;
}

LogicalTime& HLAinteger64Time::operator=(LogicalTime const& value) {
  _impl->value = asTime(value).getTime();
  return *this;
}

LogicalTime& HLAinteger64Time::operator+=(LogicalTimeInterval const& addend) {
  auto const increment = asInterval(addend).getInterval();
  if (increment > kFinalValue - _impl->value) {
    illegalArithmetic(L"HLAinteger64Time addition exceeds the final value.");
  }
  _impl->value += increment;
  return *this;
}

LogicalTime& HLAinteger64Time::operator-=(LogicalTimeInterval const& subtrahend) {
  auto const decrement = asInterval(subtrahend).getInterval();
  if (decrement > _impl->value) {
    illegalArithmetic(L"HLAinteger64Time subtraction precedes the initial value.");
  }
  _impl->value -= decrement;
  return *this;
}

bool HLAinteger64Time::operator>(LogicalTime const& value) const {
  return _impl->value > asTime(value).getTime();
}

bool HLAinteger64Time::operator<(LogicalTime const& value) const {
  return _impl->value < asTime(value).getTime();
}

bool HLAinteger64Time::operator==(LogicalTime const& value) const {
  return _impl->value == asTime(value).getTime();
}

bool HLAinteger64Time::operator>=(LogicalTime const& value) const {
  return _impl->value >= asTime(value).getTime();
}

bool HLAinteger64Time::operator<=(LogicalTime const& value) const {
  return _impl->value <= asTime(value).getTime();
}

VariableLengthData HLAinteger64Time::encode() const {
  return encoded(_impl->value);
}

size_t HLAinteger64Time::encode(void* buffer, size_t bufferSize) const {
  return writeEncoded(_impl->value, buffer, bufferSize);
}

size_t HLAinteger64Time::encodedLength() const {
  return kEncodedLength;
}

void HLAinteger64Time::decode(VariableLengthData const& value) {
  decode(const_cast<void*>(value.data()), value.size());
}

void HLAinteger64Time::decode(void* buffer, size_t bufferSize) {
  _impl->value = static_cast<Integer64>(readBigEndian(buffer, bufferSize));
}

std::wstring HLAinteger64Time::toString() const {
  return std::to_wstring(_impl->value);
}

std::wstring HLAinteger64Time::implementationName() const {
  return HLAinteger64TimeName;
}

Integer64 HLAinteger64Time::getTime() const {
  return _impl->value;
}

void HLAinteger64Time::setTime(Integer64 value) {
  if (!valid(value)) {
    invalidTime(L"HLAinteger64Time must be nonnegative.");
  }
  _impl->value = value;
}

HLAinteger64Time& HLAinteger64Time::operator=(HLAinteger64Time const& value) throw(InvalidLogicalTime) {
  if (this != &value) {
    _impl->value = value._impl->value;
  }
  return *this;
}

HLAinteger64Time::operator Integer64() const {
  return _impl->value;
}

HLAinteger64Interval::HLAinteger64Interval()
    : _impl(new HLAinteger64IntervalImpl()) {}

HLAinteger64Interval::HLAinteger64Interval(HLAinteger64Interval const& value)
    : _impl(new HLAinteger64IntervalImpl()) {
  _impl->value = value._impl->value;
}

HLAinteger64Interval::HLAinteger64Interval(LogicalTimeInterval const& value)
    : _impl(new HLAinteger64IntervalImpl()) {
  setInterval(asInterval(value).getInterval());
}

HLAinteger64Interval::HLAinteger64Interval(Integer64 value)
    : _impl(new HLAinteger64IntervalImpl()) {
  setInterval(value);
}

HLAinteger64Interval::~HLAinteger64Interval() throw() {
  delete _impl;
}

void HLAinteger64Interval::setZero() {
  _impl->value = 0;
}

bool HLAinteger64Interval::isZero() const {
  return _impl->value == 0;
}

void HLAinteger64Interval::setEpsilon() {
  _impl->value = 1;
}

bool HLAinteger64Interval::isEpsilon() const {
  return _impl->value == 1;
}

LogicalTimeInterval& HLAinteger64Interval::operator=(LogicalTimeInterval const& value) {
  _impl->value = asInterval(value).getInterval();
  return *this;
}

LogicalTimeInterval& HLAinteger64Interval::operator+=(LogicalTimeInterval const& addend) {
  auto const increment = asInterval(addend).getInterval();
  if (increment > kFinalValue - _impl->value) {
    illegalArithmetic(L"HLAinteger64Interval addition exceeds its range.");
  }
  _impl->value += increment;
  return *this;
}

LogicalTimeInterval& HLAinteger64Interval::operator-=(LogicalTimeInterval const& subtrahend) {
  auto const decrement = asInterval(subtrahend).getInterval();
  if (decrement > _impl->value) {
    illegalArithmetic(L"HLAinteger64Interval subtraction precedes zero.");
  }
  _impl->value -= decrement;
  return *this;
}

bool HLAinteger64Interval::operator>(LogicalTimeInterval const& value) const {
  return _impl->value > asInterval(value).getInterval();
}

bool HLAinteger64Interval::operator<(LogicalTimeInterval const& value) const {
  return _impl->value < asInterval(value).getInterval();
}

bool HLAinteger64Interval::operator==(LogicalTimeInterval const& value) const {
  return _impl->value == asInterval(value).getInterval();
}

bool HLAinteger64Interval::operator>=(LogicalTimeInterval const& value) const {
  return _impl->value >= asInterval(value).getInterval();
}

bool HLAinteger64Interval::operator<=(LogicalTimeInterval const& value) const {
  return _impl->value <= asInterval(value).getInterval();
}

void HLAinteger64Interval::setToDifference(
    LogicalTime const& minuend,
    LogicalTime const& subtrahend) {
  auto const left = asTime(minuend).getTime();
  auto const right = asTime(subtrahend).getTime();
  if (right > left) {
    illegalArithmetic(L"HLAinteger64Time difference would be negative.");
  }
  _impl->value = left - right;
}

VariableLengthData HLAinteger64Interval::encode() const {
  return encoded(_impl->value);
}

size_t HLAinteger64Interval::encode(void* buffer, size_t bufferSize) const {
  return writeEncoded(_impl->value, buffer, bufferSize);
}

size_t HLAinteger64Interval::encodedLength() const {
  return kEncodedLength;
}

void HLAinteger64Interval::decode(VariableLengthData const& value) {
  decode(const_cast<void*>(value.data()), value.size());
}

void HLAinteger64Interval::decode(void* buffer, size_t bufferSize) {
  _impl->value = static_cast<Integer64>(readBigEndian(buffer, bufferSize));
}

std::wstring HLAinteger64Interval::toString() const {
  return std::to_wstring(_impl->value);
}

std::wstring HLAinteger64Interval::implementationName() const {
  return HLAinteger64TimeName;
}

Integer64 HLAinteger64Interval::getInterval() const {
  return _impl->value;
}

void HLAinteger64Interval::setInterval(Integer64 value) {
  if (!valid(value)) {
    invalidInterval(L"HLAinteger64Interval must be nonnegative.");
  }
  _impl->value = value;
}

HLAinteger64Interval& HLAinteger64Interval::operator=(HLAinteger64Interval const& value) throw(InvalidLogicalTimeInterval) {
  if (this != &value) {
    _impl->value = value._impl->value;
  }
  return *this;
}

HLAinteger64Interval::operator Integer64() const {
  return _impl->value;
}

HLAinteger64TimeFactory::HLAinteger64TimeFactory() = default;
HLAinteger64TimeFactory::~HLAinteger64TimeFactory() throw() = default;

std::auto_ptr<LogicalTime> HLAinteger64TimeFactory::makeInitial() {
  return std::auto_ptr<LogicalTime>(new HLAinteger64Time(0));
}

std::auto_ptr<LogicalTime> HLAinteger64TimeFactory::makeFinal() {
  return std::auto_ptr<LogicalTime>(new HLAinteger64Time(kFinalValue));
}

std::auto_ptr<LogicalTimeInterval> HLAinteger64TimeFactory::makeZero() {
  return std::auto_ptr<LogicalTimeInterval>(new HLAinteger64Interval(0));
}

std::auto_ptr<LogicalTimeInterval> HLAinteger64TimeFactory::makeEpsilon() {
  return std::auto_ptr<LogicalTimeInterval>(new HLAinteger64Interval(1));
}

std::auto_ptr<HLAinteger64Time> HLAinteger64TimeFactory::makeLogicalTime(Integer64 value) {
  return std::auto_ptr<HLAinteger64Time>(new HLAinteger64Time(value));
}

std::auto_ptr<HLAinteger64Interval> HLAinteger64TimeFactory::makeLogicalTimeInterval(Integer64 value) {
  return std::auto_ptr<HLAinteger64Interval>(new HLAinteger64Interval(value));
}

std::auto_ptr<LogicalTime> HLAinteger64TimeFactory::decodeLogicalTime(
    VariableLengthData const& value) {
  auto result = std::auto_ptr<HLAinteger64Time>(new HLAinteger64Time());
  result->decode(value);
  return std::auto_ptr<LogicalTime>(result.release());
}

std::auto_ptr<LogicalTime> HLAinteger64TimeFactory::decodeLogicalTime(
    void* buffer, size_t bufferSize) {
  auto result = std::auto_ptr<HLAinteger64Time>(new HLAinteger64Time());
  result->decode(buffer, bufferSize);
  return std::auto_ptr<LogicalTime>(result.release());
}

std::auto_ptr<LogicalTimeInterval> HLAinteger64TimeFactory::decodeLogicalTimeInterval(
    VariableLengthData const& value) {
  auto result = std::auto_ptr<HLAinteger64Interval>(new HLAinteger64Interval());
  result->decode(value);
  return std::auto_ptr<LogicalTimeInterval>(result.release());
}

std::auto_ptr<LogicalTimeInterval> HLAinteger64TimeFactory::decodeLogicalTimeInterval(
    void* buffer, size_t bufferSize) {
  auto result = std::auto_ptr<HLAinteger64Interval>(new HLAinteger64Interval());
  result->decode(buffer, bufferSize);
  return std::auto_ptr<LogicalTimeInterval>(result.release());
}

std::wstring HLAinteger64TimeFactory::getName() const {
  return HLAinteger64TimeName;
}

std::auto_ptr<LogicalTimeFactory> HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
    std::wstring const& implementationName) {
  if (implementationName.empty() || implementationName == HLAinteger64TimeName) {
    return std::auto_ptr<LogicalTimeFactory>(new HLAinteger64TimeFactory());
  }
  if (implementationName == HLAfloat64TimeName) {
    return std::auto_ptr<LogicalTimeFactory>(new HLAfloat64TimeFactory());
  }
  return std::auto_ptr<LogicalTimeFactory>();
}

std::auto_ptr<LogicalTimeFactory> LogicalTimeFactoryFactory::makeLogicalTimeFactory(
    std::wstring const& implementationName) {
  return HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(implementationName);
}

}  // namespace rti1516e
