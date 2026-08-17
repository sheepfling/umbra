#include <RTI/RTI1516.h>
#include <RTI/time/HLAfloat64Interval.h>
#include <RTI/time/HLAfloat64Time.h>
#include <RTI/time/HLAfloat64TimeFactory.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>
#include <RTI/time/HLAinteger64TimeFactory.h>

#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <limits>
#include <memory>
#include <new>
#include <ostream>
#include <sstream>
#include <utility>

namespace rti1516_2025 {

class HLAinteger64TimeImpl {
 public:
  Integer64 value = 0;
};

class HLAinteger64IntervalImpl {
 public:
  Integer64 value = 0;
};

class HLAfloat64TimeImpl {
 public:
  double value = 0.0;
};

class HLAfloat64IntervalImpl {
 public:
  double value = 0.0;
};

namespace {

constexpr std::size_t kEncodedLength = sizeof(std::uint64_t);
constexpr Integer64 kInteger64Final = std::numeric_limits<Integer64>::max();
constexpr double kFloat64Final = std::numeric_limits<double>::max();
constexpr double kFloat64Epsilon = std::numeric_limits<double>::denorm_min();

static_assert(sizeof(double) == sizeof(std::uint64_t));
static_assert(std::numeric_limits<double>::is_iec559);
static_assert(std::numeric_limits<double>::has_denorm != std::denorm_absent);
static_assert(kFloat64Epsilon > 0.0);

[[noreturn]] void invalidLogicalTime(wchar_t const* message) {
  throw InvalidLogicalTime(message);
}

[[noreturn]] void invalidLogicalTimeInterval(wchar_t const* message) {
  throw InvalidLogicalTimeInterval(message);
}

[[noreturn]] void illegalTimeArithmetic(wchar_t const* message) {
  throw IllegalTimeArithmetic(message);
}

[[noreturn]] void couldNotDecode(wchar_t const* message) {
  throw CouldNotDecode(message);
}

[[noreturn]] void couldNotEncode(wchar_t const* message) {
  throw CouldNotEncode(message);
}

template <typename T, typename... Args>
T* allocateImplementation(Args&&... args) {
  try {
    return new T{std::forward<Args>(args)...};
  } catch (std::bad_alloc const&) {
    throw InternalError(L"Unable to allocate a reference logical-time value");
  }
}

template <typename T, typename... Args>
std::unique_ptr<T> makeTimeValue(Args&&... args) {
  try {
    return std::make_unique<T>(std::forward<Args>(args)...);
  } catch (std::bad_alloc const&) {
    throw InternalError(L"Unable to allocate a reference logical-time value");
  }
}

bool isValidIntegerValue(Integer64 value) noexcept {
  return value >= 0;
}

double canonicalizeFloatZero(double value) noexcept {
  return value == 0.0 ? 0.0 : value;
}

bool isValidFloatValue(double value) noexcept {
  return std::isfinite(value) && value >= 0.0;
}

std::array<unsigned char, kEncodedLength> integer64BigEndian(Integer64 value) noexcept {
  std::array<unsigned char, kEncodedLength> encoded{};
  auto remaining = static_cast<std::uint64_t>(value);
  for (std::size_t index = kEncodedLength; index > 0; --index) {
    encoded[index - 1] = static_cast<unsigned char>(remaining & 0xFFU);
    remaining >>= 8U;
  }
  return encoded;
}

std::array<unsigned char, kEncodedLength> float64BigEndian(double value) noexcept {
  std::array<unsigned char, kEncodedLength> encoded{};
  auto remaining = std::bit_cast<std::uint64_t>(value);
  for (std::size_t index = kEncodedLength; index > 0; --index) {
    encoded[index - 1] = static_cast<unsigned char>(remaining & 0xFFU);
    remaining >>= 8U;
  }
  return encoded;
}

std::uint64_t readBigEndian(void const* buffer, std::size_t bufferSize) {
  if (buffer == nullptr || bufferSize != kEncodedLength) {
    couldNotDecode(L"A reference logical-time encoding must contain exactly eight bytes");
  }
  auto const* encoded = static_cast<unsigned char const*>(buffer);
  std::uint64_t value = 0;
  for (std::size_t index = 0; index < kEncodedLength; ++index) {
    value = (value << 8U) | encoded[index];
  }
  return value;
}

std::size_t writeEncoded(
    std::array<unsigned char, kEncodedLength> const& encoded,
    void* buffer,
    std::size_t bufferSize) {
  if (buffer == nullptr || bufferSize < encoded.size()) {
    couldNotEncode(L"The output buffer is too small for a reference logical-time encoding");
  }
  std::memcpy(buffer, encoded.data(), encoded.size());
  return encoded.size();
}

VariableLengthData variableLengthData(
    std::array<unsigned char, kEncodedLength> const& encoded) {
  return VariableLengthData(encoded.data(), encoded.size());
}

std::wstring integerString(Integer64 value) {
  return std::to_wstring(value);
}

std::wstring floatString(double value) {
  std::wostringstream stream;
  stream << std::setprecision(std::numeric_limits<double>::max_digits10) << value;
  return stream.str();
}

Integer64 checkedIntegerTime( LogicalTime const& value) {
  auto const* concrete = dynamic_cast<HLAinteger64Time const*>(&value);
  if (concrete == nullptr) {
    invalidLogicalTime(L"Expected an HLAinteger64Time value");
  }
  return concrete->getTime();
}

Integer64 checkedIntegerInterval(LogicalTimeInterval const& value) {
  auto const* concrete = dynamic_cast<HLAinteger64Interval const*>(&value);
  if (concrete == nullptr) {
    invalidLogicalTimeInterval(L"Expected an HLAinteger64Interval value");
  }
  return concrete->getInterval();
}

double checkedFloatTime(LogicalTime const& value) {
  auto const* concrete = dynamic_cast<HLAfloat64Time const*>(&value);
  if (concrete == nullptr) {
    invalidLogicalTime(L"Expected an HLAfloat64Time value");
  }
  return concrete->getTime();
}

double checkedFloatInterval(LogicalTimeInterval const& value) {
  auto const* concrete = dynamic_cast<HLAfloat64Interval const*>(&value);
  if (concrete == nullptr) {
    invalidLogicalTimeInterval(L"Expected an HLAfloat64Interval value");
  }
  return concrete->getInterval();
}

double addFloatValue(double value, double addend) {
  if (value == kFloat64Final && addend == 0.0) {
    return value;
  }
  if (value >= kFloat64Final || addend > kFloat64Final - value) {
    illegalTimeArithmetic(L"Reference logical-time addition exceeds the final value");
  }
  auto result = value + addend;
  if (addend == kFloat64Epsilon && result == value) {
    result = std::nextafter(value, kFloat64Final);
  }
  if (!isValidFloatValue(result)) {
    illegalTimeArithmetic(L"Reference logical-time addition produced an invalid value");
  }
  return canonicalizeFloatZero(result);
}

double subtractFloatValue(double value, double subtrahend) {
  if (subtrahend > value) {
    illegalTimeArithmetic(L"Reference logical-time subtraction precedes the initial value");
  }
  auto result = value - subtrahend;
  if (subtrahend == kFloat64Epsilon && result == value && value > 0.0) {
    result = std::nextafter(value, 0.0);
  }
  if (!isValidFloatValue(result)) {
    illegalTimeArithmetic(L"Reference logical-time subtraction produced an invalid value");
  }
  return canonicalizeFloatZero(result);
}

}  // namespace

HLAinteger64Time::HLAinteger64Time() : _impl(allocateImplementation<HLAinteger64TimeImpl>()) {}

HLAinteger64Time::HLAinteger64Time(Integer64 value)
    : _impl(allocateImplementation<HLAinteger64TimeImpl>()) {
  setTime(value);
}

HLAinteger64Time::HLAinteger64Time(LogicalTime const& value)
    : _impl(allocateImplementation<HLAinteger64TimeImpl>()) {
  setTime(checkedIntegerTime(value));
}

HLAinteger64Time::HLAinteger64Time(HLAinteger64Time const& value)
    : _impl(allocateImplementation<HLAinteger64TimeImpl>()) {
  _impl->value = value._impl->value;
}

HLAinteger64Time::~HLAinteger64Time() noexcept {
  delete _impl;
}

void HLAinteger64Time::setInitial() {
  _impl->value = 0;
}

bool HLAinteger64Time::isInitial() const {
  return _impl->value == 0;
}

void HLAinteger64Time::setFinal() {
  _impl->value = kInteger64Final;
}

bool HLAinteger64Time::isFinal() const {
  return _impl->value == kInteger64Final;
}

HLAinteger64Time& HLAinteger64Time::operator=(LogicalTime const& value) {
  return operator=(HLAinteger64Time(checkedIntegerTime(value)));
}

LogicalTime& HLAinteger64Time::operator+=(LogicalTimeInterval const& addend) {
  auto value = checkedIntegerInterval(addend);
  if (value > kInteger64Final - _impl->value) {
    illegalTimeArithmetic(L"HLAinteger64Time addition exceeds the final value");
  }
  _impl->value += value;
  return *this;
}

LogicalTime& HLAinteger64Time::operator-=(LogicalTimeInterval const& subtrahend) {
  auto value = checkedIntegerInterval(subtrahend);
  if (value > _impl->value) {
    illegalTimeArithmetic(L"HLAinteger64Time subtraction precedes the initial value");
  }
  _impl->value -= value;
  return *this;
}

bool HLAinteger64Time::operator>(LogicalTime const& value) const {
  return _impl->value > checkedIntegerTime(value);
}

bool HLAinteger64Time::operator<(LogicalTime const& value) const {
  return _impl->value < checkedIntegerTime(value);
}

bool HLAinteger64Time::operator==(LogicalTime const& value) const {
  return _impl->value == checkedIntegerTime(value);
}

bool HLAinteger64Time::operator>=(LogicalTime const& value) const {
  return _impl->value >= checkedIntegerTime(value);
}

bool HLAinteger64Time::operator<=(LogicalTime const& value) const {
  return _impl->value <= checkedIntegerTime(value);
}

VariableLengthData HLAinteger64Time::encode() const {
  return variableLengthData(integer64BigEndian(_impl->value));
}

std::size_t HLAinteger64Time::encode(void* buffer, std::size_t bufferSize) const {
  return writeEncoded(integer64BigEndian(_impl->value), buffer, bufferSize);
}

std::size_t HLAinteger64Time::encodedLength() const {
  return kEncodedLength;
}

void HLAinteger64Time::decode(VariableLengthData const& encodedValue) {
  decode(encodedValue.data(), encodedValue.size());
}

void HLAinteger64Time::decode(void const* buffer, std::size_t bufferSize) {
  auto bits = readBigEndian(buffer, bufferSize);
  if ((bits & (std::uint64_t{1} << 63U)) != 0U) {
    couldNotDecode(L"HLAinteger64Time cannot decode a negative value");
  }
  _impl->value = static_cast<Integer64>(bits);
}

std::wstring HLAinteger64Time::toString() const {
  return integerString(_impl->value);
}

std::wstring HLAinteger64Time::implementationName() const {
  return HLAinteger64TimeName;
}

Integer64 HLAinteger64Time::getTime() const {
  return _impl->value;
}

void HLAinteger64Time::setTime(Integer64 value) {
  if (!isValidIntegerValue(value)) {
    invalidLogicalTime(L"HLAinteger64Time must be nonnegative");
  }
  _impl->value = value;
}

HLAinteger64Time& HLAinteger64Time::operator=(HLAinteger64Time const& value) {
  if (this != &value) {
    _impl->value = value._impl->value;
  }
  return *this;
}

HLAinteger64Time::operator Integer64() const {
  return _impl->value;
}

HLAinteger64Interval::HLAinteger64Interval()
    : _impl(allocateImplementation<HLAinteger64IntervalImpl>()) {}

HLAinteger64Interval::HLAinteger64Interval(HLAinteger64Interval const& value)
    : _impl(allocateImplementation<HLAinteger64IntervalImpl>()) {
  _impl->value = value._impl->value;
}

HLAinteger64Interval::HLAinteger64Interval(LogicalTimeInterval const& value)
    : _impl(allocateImplementation<HLAinteger64IntervalImpl>()) {
  setInterval(checkedIntegerInterval(value));
}

HLAinteger64Interval::HLAinteger64Interval(Integer64 value)
    : _impl(allocateImplementation<HLAinteger64IntervalImpl>()) {
  setInterval(value);
}

HLAinteger64Interval::~HLAinteger64Interval() noexcept {
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

HLAinteger64Interval& HLAinteger64Interval::operator=(LogicalTimeInterval const& value) {
  return operator=(HLAinteger64Interval(checkedIntegerInterval(value)));
}

LogicalTimeInterval& HLAinteger64Interval::operator+=(LogicalTimeInterval const& addend) {
  auto value = checkedIntegerInterval(addend);
  if (value > kInteger64Final - _impl->value) {
    illegalTimeArithmetic(L"HLAinteger64Interval addition exceeds its representable range");
  }
  _impl->value += value;
  return *this;
}

LogicalTimeInterval& HLAinteger64Interval::operator-=(LogicalTimeInterval const& subtrahend) {
  auto value = checkedIntegerInterval(subtrahend);
  if (value > _impl->value) {
    illegalTimeArithmetic(L"HLAinteger64Interval subtraction precedes zero");
  }
  _impl->value -= value;
  return *this;
}

bool HLAinteger64Interval::operator>(LogicalTimeInterval const& value) const {
  return _impl->value > checkedIntegerInterval(value);
}

bool HLAinteger64Interval::operator<(LogicalTimeInterval const& value) const {
  return _impl->value < checkedIntegerInterval(value);
}

bool HLAinteger64Interval::operator==(LogicalTimeInterval const& value) const {
  return _impl->value == checkedIntegerInterval(value);
}

bool HLAinteger64Interval::operator>=(LogicalTimeInterval const& value) const {
  return _impl->value >= checkedIntegerInterval(value);
}

bool HLAinteger64Interval::operator<=(LogicalTimeInterval const& value) const {
  return _impl->value <= checkedIntegerInterval(value);
}

void HLAinteger64Interval::setToDifference(
    LogicalTime const& minuend,
    LogicalTime const& subtrahend) {
  auto left = checkedIntegerTime(minuend);
  auto right = checkedIntegerTime(subtrahend);
  if (right > left) {
    illegalTimeArithmetic(L"HLAinteger64Time difference would be negative");
  }
  _impl->value = left - right;
}

VariableLengthData HLAinteger64Interval::encode() const {
  return variableLengthData(integer64BigEndian(_impl->value));
}

std::size_t HLAinteger64Interval::encode(void* buffer, std::size_t bufferSize) const {
  return writeEncoded(integer64BigEndian(_impl->value), buffer, bufferSize);
}

std::size_t HLAinteger64Interval::encodedLength() const {
  return kEncodedLength;
}

void HLAinteger64Interval::decode(VariableLengthData const& encodedValue) {
  decode(encodedValue.data(), encodedValue.size());
}

void HLAinteger64Interval::decode(void const* buffer, std::size_t bufferSize) {
  auto bits = readBigEndian(buffer, bufferSize);
  if ((bits & (std::uint64_t{1} << 63U)) != 0U) {
    couldNotDecode(L"HLAinteger64Interval cannot decode a negative value");
  }
  _impl->value = static_cast<Integer64>(bits);
}

std::wstring HLAinteger64Interval::toString() const {
  return integerString(_impl->value);
}

std::wstring HLAinteger64Interval::implementationName() const {
  return HLAinteger64TimeName;
}

Integer64 HLAinteger64Interval::getInterval() const {
  return _impl->value;
}

void HLAinteger64Interval::setInterval(Integer64 value) {
  if (!isValidIntegerValue(value)) {
    invalidLogicalTimeInterval(L"HLAinteger64Interval must be nonnegative");
  }
  _impl->value = value;
}

HLAinteger64Interval& HLAinteger64Interval::operator=(HLAinteger64Interval const& value) {
  if (this != &value) {
    _impl->value = value._impl->value;
  }
  return *this;
}

HLAinteger64Interval::operator Integer64() const {
  return _impl->value;
}

HLAfloat64Time::HLAfloat64Time() : _impl(allocateImplementation<HLAfloat64TimeImpl>()) {}

HLAfloat64Time::HLAfloat64Time(double const& value)
    : _impl(allocateImplementation<HLAfloat64TimeImpl>()) {
  setTime(value);
}

HLAfloat64Time::HLAfloat64Time(LogicalTime const& value)
    : _impl(allocateImplementation<HLAfloat64TimeImpl>()) {
  setTime(checkedFloatTime(value));
}

HLAfloat64Time::HLAfloat64Time(HLAfloat64Time const& value)
    : _impl(allocateImplementation<HLAfloat64TimeImpl>()) {
  _impl->value = value._impl->value;
}

HLAfloat64Time::~HLAfloat64Time() noexcept {
  delete _impl;
}

void HLAfloat64Time::setInitial() {
  _impl->value = 0.0;
}

bool HLAfloat64Time::isInitial() const {
  return _impl->value == 0.0;
}

void HLAfloat64Time::setFinal() {
  _impl->value = kFloat64Final;
}

bool HLAfloat64Time::isFinal() const {
  return _impl->value == kFloat64Final;
}

HLAfloat64Time& HLAfloat64Time::operator=(LogicalTime const& value) {
  return operator=(HLAfloat64Time(checkedFloatTime(value)));
}

LogicalTime& HLAfloat64Time::operator+=(LogicalTimeInterval const& addend) {
  _impl->value = addFloatValue(_impl->value, checkedFloatInterval(addend));
  return *this;
}

LogicalTime& HLAfloat64Time::operator-=(LogicalTimeInterval const& subtrahend) {
  _impl->value = subtractFloatValue(_impl->value, checkedFloatInterval(subtrahend));
  return *this;
}

bool HLAfloat64Time::operator>(LogicalTime const& value) const {
  return _impl->value > checkedFloatTime(value);
}

bool HLAfloat64Time::operator<(LogicalTime const& value) const {
  return _impl->value < checkedFloatTime(value);
}

bool HLAfloat64Time::operator==(LogicalTime const& value) const {
  return _impl->value == checkedFloatTime(value);
}

bool HLAfloat64Time::operator>=(LogicalTime const& value) const {
  return _impl->value >= checkedFloatTime(value);
}

bool HLAfloat64Time::operator<=(LogicalTime const& value) const {
  return _impl->value <= checkedFloatTime(value);
}

VariableLengthData HLAfloat64Time::encode() const {
  return variableLengthData(float64BigEndian(_impl->value));
}

std::size_t HLAfloat64Time::encode(void* buffer, std::size_t bufferSize) const {
  return writeEncoded(float64BigEndian(_impl->value), buffer, bufferSize);
}

std::size_t HLAfloat64Time::encodedLength() const {
  return kEncodedLength;
}

void HLAfloat64Time::decode(VariableLengthData const& encodedValue) {
  decode(encodedValue.data(), encodedValue.size());
}

void HLAfloat64Time::decode(void const* buffer, std::size_t bufferSize) {
  auto value = std::bit_cast<double>(readBigEndian(buffer, bufferSize));
  if (!isValidFloatValue(value)) {
    couldNotDecode(L"HLAfloat64Time must decode to a finite nonnegative value");
  }
  _impl->value = canonicalizeFloatZero(value);
}

std::wstring HLAfloat64Time::toString() const {
  return floatString(_impl->value);
}

std::wstring HLAfloat64Time::implementationName() const {
  return HLAfloat64TimeName;
}

double HLAfloat64Time::getTime() const {
  return _impl->value;
}

void HLAfloat64Time::setTime(double value) {
  if (!isValidFloatValue(value)) {
    invalidLogicalTime(L"HLAfloat64Time must be finite and nonnegative");
  }
  _impl->value = canonicalizeFloatZero(value);
}

HLAfloat64Time& HLAfloat64Time::operator=(HLAfloat64Time const& value) {
  if (this != &value) {
    _impl->value = value._impl->value;
  }
  return *this;
}

HLAfloat64Time::operator double() const {
  return _impl->value;
}

HLAfloat64Interval::HLAfloat64Interval()
    : _impl(allocateImplementation<HLAfloat64IntervalImpl>()) {}

HLAfloat64Interval::HLAfloat64Interval(double value)
    : _impl(allocateImplementation<HLAfloat64IntervalImpl>()) {
  setInterval(value);
}

HLAfloat64Interval::HLAfloat64Interval(LogicalTimeInterval const& value)
    : _impl(allocateImplementation<HLAfloat64IntervalImpl>()) {
  setInterval(checkedFloatInterval(value));
}

HLAfloat64Interval::HLAfloat64Interval(HLAfloat64Interval const& value)
    : _impl(allocateImplementation<HLAfloat64IntervalImpl>()) {
  _impl->value = value._impl->value;
}

HLAfloat64Interval::~HLAfloat64Interval() noexcept {
  delete _impl;
}

void HLAfloat64Interval::setZero() {
  _impl->value = 0.0;
}

bool HLAfloat64Interval::isZero() const {
  return _impl->value == 0.0;
}

void HLAfloat64Interval::setEpsilon() {
  _impl->value = kFloat64Epsilon;
}

bool HLAfloat64Interval::isEpsilon() const {
  return _impl->value == kFloat64Epsilon;
}

HLAfloat64Interval& HLAfloat64Interval::operator=(LogicalTimeInterval const& value) {
  return operator=(HLAfloat64Interval(checkedFloatInterval(value)));
}

LogicalTimeInterval& HLAfloat64Interval::operator+=(LogicalTimeInterval const& addend) {
  _impl->value = addFloatValue(_impl->value, checkedFloatInterval(addend));
  return *this;
}

LogicalTimeInterval& HLAfloat64Interval::operator-=(LogicalTimeInterval const& subtrahend) {
  _impl->value = subtractFloatValue(_impl->value, checkedFloatInterval(subtrahend));
  return *this;
}

bool HLAfloat64Interval::operator>(LogicalTimeInterval const& value) const {
  return _impl->value > checkedFloatInterval(value);
}

bool HLAfloat64Interval::operator<(LogicalTimeInterval const& value) const {
  return _impl->value < checkedFloatInterval(value);
}

bool HLAfloat64Interval::operator==(LogicalTimeInterval const& value) const {
  return _impl->value == checkedFloatInterval(value);
}

bool HLAfloat64Interval::operator>=(LogicalTimeInterval const& value) const {
  return _impl->value >= checkedFloatInterval(value);
}

bool HLAfloat64Interval::operator<=(LogicalTimeInterval const& value) const {
  return _impl->value <= checkedFloatInterval(value);
}

void HLAfloat64Interval::setToDifference(
    LogicalTime const& minuend,
    LogicalTime const& subtrahend) {
  auto left = checkedFloatTime(minuend);
  auto right = checkedFloatTime(subtrahend);
  if (right > left) {
    illegalTimeArithmetic(L"HLAfloat64Time difference would be negative");
  }
  _impl->value = canonicalizeFloatZero(left - right);
}

VariableLengthData HLAfloat64Interval::encode() const {
  return variableLengthData(float64BigEndian(_impl->value));
}

std::size_t HLAfloat64Interval::encode(void* buffer, std::size_t bufferSize) const {
  return writeEncoded(float64BigEndian(_impl->value), buffer, bufferSize);
}

std::size_t HLAfloat64Interval::encodedLength() const {
  return kEncodedLength;
}

void HLAfloat64Interval::decode(VariableLengthData const& encodedValue) {
  decode(encodedValue.data(), encodedValue.size());
}

void HLAfloat64Interval::decode(void const* buffer, std::size_t bufferSize) {
  auto value = std::bit_cast<double>(readBigEndian(buffer, bufferSize));
  if (!isValidFloatValue(value)) {
    couldNotDecode(L"HLAfloat64Interval must decode to a finite nonnegative value");
  }
  _impl->value = canonicalizeFloatZero(value);
}

std::wstring HLAfloat64Interval::toString() const {
  return floatString(_impl->value);
}

std::wstring HLAfloat64Interval::implementationName() const {
  return HLAfloat64TimeName;
}

double HLAfloat64Interval::getInterval() const {
  return _impl->value;
}

void HLAfloat64Interval::setInterval(double value) {
  if (!isValidFloatValue(value)) {
    invalidLogicalTimeInterval(L"HLAfloat64Interval must be finite and nonnegative");
  }
  _impl->value = canonicalizeFloatZero(value);
}

HLAfloat64Interval& HLAfloat64Interval::operator=(HLAfloat64Interval const& value) {
  if (this != &value) {
    _impl->value = value._impl->value;
  }
  return *this;
}

HLAfloat64Interval::operator double() const {
  return _impl->value;
}

HLAinteger64TimeFactory::HLAinteger64TimeFactory() = default;
HLAinteger64TimeFactory::~HLAinteger64TimeFactory() noexcept = default;

std::unique_ptr<HLAinteger64Time> HLAinteger64TimeFactory::makeLogicalTime(Integer64 value) {
  return makeTimeValue<HLAinteger64Time>(value);
}

std::unique_ptr<LogicalTime> HLAinteger64TimeFactory::makeInitial() {
  return makeTimeValue<HLAinteger64Time>();
}

std::unique_ptr<LogicalTime> HLAinteger64TimeFactory::makeFinal() {
  auto time = makeTimeValue<HLAinteger64Time>();
  time->setFinal();
  return time;
}

std::unique_ptr<HLAinteger64Interval> HLAinteger64TimeFactory::makeLogicalTimeInterval(
    Integer64 value) {
  return makeTimeValue<HLAinteger64Interval>(value);
}

std::unique_ptr<LogicalTimeInterval> HLAinteger64TimeFactory::makeZero() {
  return makeTimeValue<HLAinteger64Interval>();
}

std::unique_ptr<LogicalTimeInterval> HLAinteger64TimeFactory::makeEpsilon() {
  auto interval = makeTimeValue<HLAinteger64Interval>();
  interval->setEpsilon();
  return interval;
}

std::unique_ptr<LogicalTime> HLAinteger64TimeFactory::decodeLogicalTime(
    VariableLengthData const& encodedLogicalTime) {
  auto time = makeTimeValue<HLAinteger64Time>();
  time->decode(encodedLogicalTime);
  return time;
}

std::unique_ptr<LogicalTime> HLAinteger64TimeFactory::decodeLogicalTime(
    void const* buffer,
    std::size_t bufferSize) {
  auto time = makeTimeValue<HLAinteger64Time>();
  time->decode(buffer, bufferSize);
  return time;
}

std::unique_ptr<LogicalTimeInterval> HLAinteger64TimeFactory::decodeLogicalTimeInterval(
    VariableLengthData const& encodedValue) {
  auto interval = makeTimeValue<HLAinteger64Interval>();
  interval->decode(encodedValue);
  return interval;
}

std::unique_ptr<LogicalTimeInterval> HLAinteger64TimeFactory::decodeLogicalTimeInterval(
    void const* buffer,
    std::size_t bufferSize) {
  auto interval = makeTimeValue<HLAinteger64Interval>();
  interval->decode(buffer, bufferSize);
  return interval;
}

std::wstring HLAinteger64TimeFactory::getName() const {
  return HLAinteger64TimeName;
}

HLAfloat64TimeFactory::HLAfloat64TimeFactory() = default;
HLAfloat64TimeFactory::~HLAfloat64TimeFactory() noexcept = default;

std::unique_ptr<HLAfloat64Time> HLAfloat64TimeFactory::makeLogicalTime(double value) {
  return makeTimeValue<HLAfloat64Time>(value);
}

std::unique_ptr<LogicalTime> HLAfloat64TimeFactory::makeInitial() {
  return makeTimeValue<HLAfloat64Time>();
}

std::unique_ptr<LogicalTime> HLAfloat64TimeFactory::makeFinal() {
  auto time = makeTimeValue<HLAfloat64Time>();
  time->setFinal();
  return time;
}

std::unique_ptr<HLAfloat64Interval> HLAfloat64TimeFactory::makeLogicalTimeInterval(
    double value) {
  return makeTimeValue<HLAfloat64Interval>(value);
}

std::unique_ptr<LogicalTimeInterval> HLAfloat64TimeFactory::makeZero() {
  return makeTimeValue<HLAfloat64Interval>();
}

std::unique_ptr<LogicalTimeInterval> HLAfloat64TimeFactory::makeEpsilon() {
  auto interval = makeTimeValue<HLAfloat64Interval>();
  interval->setEpsilon();
  return interval;
}

std::unique_ptr<LogicalTime> HLAfloat64TimeFactory::decodeLogicalTime(
    VariableLengthData const& encodedLogicalTime) {
  auto time = makeTimeValue<HLAfloat64Time>();
  time->decode(encodedLogicalTime);
  return time;
}

std::unique_ptr<LogicalTime> HLAfloat64TimeFactory::decodeLogicalTime(
    void const* buffer,
    std::size_t bufferSize) {
  auto time = makeTimeValue<HLAfloat64Time>();
  time->decode(buffer, bufferSize);
  return time;
}

std::unique_ptr<LogicalTimeInterval> HLAfloat64TimeFactory::decodeLogicalTimeInterval(
    VariableLengthData const& encodedValue) {
  auto interval = makeTimeValue<HLAfloat64Interval>();
  interval->decode(encodedValue);
  return interval;
}

std::unique_ptr<LogicalTimeInterval> HLAfloat64TimeFactory::decodeLogicalTimeInterval(
    void const* buffer,
    std::size_t bufferSize) {
  auto interval = makeTimeValue<HLAfloat64Interval>();
  interval->decode(buffer, bufferSize);
  return interval;
}

std::wstring HLAfloat64TimeFactory::getName() const {
  return HLAfloat64TimeName;
}

LogicalTimeFactory::~LogicalTimeFactory() noexcept = default;

std::unique_ptr<LogicalTimeFactory> HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
    std::wstring const& implementationName) {
  if (implementationName.empty() || implementationName == HLAfloat64TimeName) {
    return makeTimeValue<HLAfloat64TimeFactory>();
  }
  if (implementationName == HLAinteger64TimeName) {
    return makeTimeValue<HLAinteger64TimeFactory>();
  }
  return nullptr;
}

std::wostream& operator<<(std::wostream& stream, LogicalTime const& value) {
  stream << value.toString();
  return stream;
}

std::wostream& operator<<(std::wostream& stream, LogicalTimeInterval const& value) {
  stream << value.toString();
  return stream;
}

}  // namespace rti1516_2025
