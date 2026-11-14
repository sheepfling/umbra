#include "tck_logical_time.hpp"

namespace hla_rti_cpp_tck::detail {

namespace {

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

static_assert(SupportsAdditionAssignment<rti::LogicalTime, rti::LogicalTimeInterval>);
static_assert(SupportsSubtractionAssignment<rti::LogicalTime, rti::LogicalTimeInterval>);
static_assert(!SupportsAdditionAssignment<rti::LogicalTime, rti::LogicalTime>);
static_assert(!SupportsSubtractionAssignment<rti::LogicalTime, rti::LogicalTime>);
static_assert(
    SupportsAdditionAssignment<rti::LogicalTimeInterval, rti::LogicalTimeInterval>);
static_assert(
    SupportsSubtractionAssignment<rti::LogicalTimeInterval, rti::LogicalTimeInterval>);
static_assert(!SupportsAdditionAssignment<rti::LogicalTimeInterval, rti::LogicalTime>);
static_assert(!SupportsSubtractionAssignment<rti::LogicalTimeInterval, rti::LogicalTime>);
static_assert(SupportsAllComparisons<rti::LogicalTime, rti::LogicalTime>);
static_assert(
    SupportsAllComparisons<rti::LogicalTimeInterval, rti::LogicalTimeInterval>);
static_assert(!SupportsAllComparisons<rti::LogicalTime, rti::LogicalTimeInterval>);
static_assert(!SupportsAllComparisons<rti::LogicalTimeInterval, rti::LogicalTime>);
static_assert(!SupportsAdditionalBinaryArithmetic<rti::LogicalTime, rti::LogicalTime>);
static_assert(
    !SupportsAdditionalBinaryArithmetic<rti::LogicalTime, rti::LogicalTimeInterval>);
static_assert(
    !SupportsAdditionalBinaryArithmetic<rti::LogicalTimeInterval, rti::LogicalTime>);
static_assert(
    !SupportsAdditionalBinaryArithmetic<rti::LogicalTimeInterval, rti::LogicalTimeInterval>);
static_assert(!SupportsAdditionalUnaryArithmetic<rti::LogicalTime>);
static_assert(!SupportsAdditionalUnaryArithmetic<rti::LogicalTimeInterval>);
static_assert(!SupportsIncrementOrDecrement<rti::LogicalTime>);
static_assert(!SupportsIncrementOrDecrement<rti::LogicalTimeInterval>);
static_assert(!SupportsAdditionalCompoundArithmetic<rti::LogicalTime, rti::LogicalTimeInterval>);
static_assert(
    !SupportsAdditionalCompoundArithmetic<rti::LogicalTimeInterval, rti::LogicalTimeInterval>);

std::vector<rti::Octet> encodePortableLogicalTimeValue(std::int64_t value) {
  auto const bits = static_cast<std::uint64_t>(value);
  std::vector<rti::Octet> bytes(portableLogicalTimeEncodingSize);
  for (std::size_t index = 0U; index < bytes.size(); ++index) {
    auto const shift = static_cast<unsigned int>(
        (bytes.size() - index - 1U) * 8U);
    bytes[index] = static_cast<rti::Octet>((bits >> shift) & 0xffU);
  }
  return bytes;
}

std::int64_t decodePortableLogicalTimeValue(std::uint8_t const* bytes) {
  std::uint64_t bits = 0U;
  for (std::size_t index = 0U; index < portableLogicalTimeEncodingSize; ++index) {
    bits = (bits << 8U) | static_cast<std::uint64_t>(bytes[index]);
  }
  return static_cast<std::int64_t>(bits);
}

std::int64_t portableLogicalTimeValue(rti::LogicalTime const& value) {
  auto const* typed = dynamic_cast<PortableLogicalTime const*>(&value);
  if (typed == nullptr) {
    throw rti::InvalidLogicalTime(
        L"PortableLogicalTime received an incompatible value.");
  }
  return typed->value();
}

std::int64_t portableLogicalTimeIntervalValue(
    rti::LogicalTimeInterval const& value) {
  auto const* typed = dynamic_cast<PortableLogicalTimeInterval const*>(&value);
  if (typed == nullptr) {
    throw rti::InvalidLogicalTimeInterval(
        L"PortableLogicalTimeInterval received an incompatible value.");
  }
  return typed->value();
}

template <typename Time, typename Interval>
void verifyLogicalTimeIdentitySet(std::string const& representation) {
  Time initial;
  initial.setInitial();
  Time final;
  final.setFinal();
  Interval epsilon;
  epsilon.setEpsilon();
  Interval zero;
  zero.setZero();
  auto const timeLess = [](
                             rti::LogicalTime const& left,
                             rti::LogicalTime const& right) {
    return left.operator<(right);
  };
  auto const timeGreater = [](
                                rti::LogicalTime const& left,
                                rti::LogicalTime const& right) {
    return left.operator>(right);
  };
  auto const timeEqual = [](
                             rti::LogicalTime const& left,
                             rti::LogicalTime const& right) {
    return left.operator==(right);
  };
  auto const intervalEqual = [](
                                 rti::LogicalTimeInterval const& left,
                                 rti::LogicalTimeInterval const& right) {
    return left.operator==(right);
  };
  auto const intervalAtLeast = [](
                                   rti::LogicalTimeInterval const& left,
                                   rti::LogicalTimeInterval const& right) {
    return left.operator>=(right);
  };

  Time time(initial);
  time += epsilon;
  time += epsilon;
  Time advanced(time);
  advanced += epsilon;
  require(
      timeLess(time, advanced),
      representation + ": positive addition did not advance time");
  Time finalPlusInterval(final);
  finalPlusInterval += epsilon;
  require(
      timeEqual(finalPlusInterval, final),
      representation + ": final time changed on addition");
  Time retreated(time);
  retreated -= epsilon;
  require(
      timeGreater(time, retreated),
      representation + ": positive subtraction did not retreat time");
  Time initialMinusInterval(initial);
  initialMinusInterval -= epsilon;
  require(
      timeEqual(initialMinusInterval, initial),
      representation + ": initial time changed on subtraction");
  Time timePlusZero(time);
  timePlusZero += zero;
  require(timeEqual(timePlusZero, time), representation + ": adding zero changed time");
  Time timeMinusZero(time);
  timeMinusZero -= zero;
  require(timeEqual(timeMinusZero, time), representation + ": subtracting zero changed time");

  Interval distinctTimeDifference;
  distinctTimeDifference.setToDifference(advanced, time);
  require(
      intervalAtLeast(distinctTimeDifference, epsilon),
      representation + ": distinct time difference was below epsilon");
  Interval equalTimeDifference;
  equalTimeDifference.setToDifference(time, time);
  require(
      intervalEqual(equalTimeDifference, zero),
      representation + ": equal times had nonzero difference");

  Interval interval(epsilon);
  Interval intervalPlusZero(interval);
  intervalPlusZero += zero;
  require(
      intervalEqual(intervalPlusZero, interval),
      representation + ": adding zero changed an interval");
  Interval intervalMinusZero(interval);
  intervalMinusZero -= zero;
  require(
      intervalEqual(intervalMinusZero, interval),
      representation + ": subtracting zero changed an interval");
  Interval equalIntervalDifference(interval);
  equalIntervalDifference -= interval;
  require(
      intervalEqual(equalIntervalDifference, zero),
      representation + ": equal intervals had nonzero difference");
  Interval smallerMinusLarger(interval);
  Interval larger(interval);
  larger += epsilon;
  smallerMinusLarger -= larger;
  require(
      intervalEqual(smallerMinusLarger, zero),
      representation + ": negative interval difference was nonzero");
  Interval largerMinusSmaller(larger);
  largerMinusSmaller -= interval;
  require(
      intervalAtLeast(largerMinusSmaller, epsilon),
      representation + ": positive interval difference was below epsilon");
  require(!intervalEqual(epsilon, zero), representation + ": epsilon equaled zero");
  require(
      intervalAtLeast(epsilon, epsilon),
      representation + ": epsilon was below itself");
  require(
      timeLess(retreated, time),
      representation + ": reverse time comparison was inconsistent");
  require(timeLess(initial, time), representation + ": finite time was not above initial");
  require(timeLess(time, final), representation + ": finite time was not below final");
}

}  // namespace

PortableLogicalTime::PortableLogicalTime() = default;

PortableLogicalTime::PortableLogicalTime(std::int64_t value) : value_(value) {}

void PortableLogicalTime::setInitial() { value_ = 0; }
bool PortableLogicalTime::isInitial() const { return value_ == 0; }
void PortableLogicalTime::setFinal() {
  value_ = std::numeric_limits<std::int64_t>::max();
}
bool PortableLogicalTime::isFinal() const {
  return value_ == std::numeric_limits<std::int64_t>::max();
}

PortableLogicalTime& PortableLogicalTime::operator=(
    rti::LogicalTime const& value) {
  value_ = portableLogicalTimeValue(value);
  return *this;
}

PortableLogicalTime& PortableLogicalTime::operator+=(
    rti::LogicalTimeInterval const& addend) {
  value_ += portableLogicalTimeIntervalValue(addend);
  return *this;
}

PortableLogicalTime& PortableLogicalTime::operator-=(
    rti::LogicalTimeInterval const& subtrahend) {
  value_ -= portableLogicalTimeIntervalValue(subtrahend);
  return *this;
}

bool PortableLogicalTime::operator>(rti::LogicalTime const& value) const {
  return value_ > portableLogicalTimeValue(value);
}
bool PortableLogicalTime::operator<(rti::LogicalTime const& value) const {
  return value_ < portableLogicalTimeValue(value);
}
bool PortableLogicalTime::operator==(rti::LogicalTime const& value) const {
  return value_ == portableLogicalTimeValue(value);
}
bool PortableLogicalTime::operator>=(rti::LogicalTime const& value) const {
  return value_ >= portableLogicalTimeValue(value);
}
bool PortableLogicalTime::operator<=(rti::LogicalTime const& value) const {
  return value_ <= portableLogicalTimeValue(value);
}

rti::VariableLengthData PortableLogicalTime::encode() const {
  auto const bytes = encodePortableLogicalTimeValue(value_);
  return rti::VariableLengthData(bytes.data(), bytes.size());
}

std::size_t PortableLogicalTime::encode(
    void* buffer,
    std::size_t bufferSize) const {
  if (buffer == nullptr || bufferSize < portableLogicalTimeEncodingSize) {
    throw rti::CouldNotEncode(
        L"PortableLogicalTime output buffer is too small.");
  }
  auto const bytes = encodePortableLogicalTimeValue(value_);
  auto* output = static_cast<rti::Octet*>(buffer);
  std::copy(bytes.begin(), bytes.end(), output);
  return bytes.size();
}

std::size_t PortableLogicalTime::encodedLength() const {
  return portableLogicalTimeEncodingSize;
}

void PortableLogicalTime::decode(
    rti::VariableLengthData const& encodedLogicalTime) {
  if (encodedLogicalTime.size() != portableLogicalTimeEncodingSize) {
    throw rti::CouldNotDecode(
        L"PortableLogicalTime encoding has the wrong length.");
  }
  value_ = decodePortableLogicalTimeValue(
      static_cast<std::uint8_t const*>(encodedLogicalTime.data()));
}

void PortableLogicalTime::decode(void const* buffer, std::size_t bufferSize) {
  if (buffer == nullptr || bufferSize != portableLogicalTimeEncodingSize) {
    throw rti::CouldNotDecode(
        L"PortableLogicalTime input buffer has the wrong length.");
  }
  value_ = decodePortableLogicalTimeValue(
      static_cast<std::uint8_t const*>(buffer));
}

std::wstring PortableLogicalTime::toString() const {
  return std::to_wstring(value_);
}
std::wstring PortableLogicalTime::implementationName() const {
  return L"PortableLogicalTime";
}
std::int64_t PortableLogicalTime::value() const { return value_; }

PortableLogicalTimeInterval::PortableLogicalTimeInterval() = default;

PortableLogicalTimeInterval::PortableLogicalTimeInterval(std::int64_t value)
    : value_(value) {}

void PortableLogicalTimeInterval::setZero() { value_ = 0; }
bool PortableLogicalTimeInterval::isZero() const { return value_ == 0; }
void PortableLogicalTimeInterval::setEpsilon() { value_ = 1; }
bool PortableLogicalTimeInterval::isEpsilon() const { return value_ == 1; }

PortableLogicalTimeInterval& PortableLogicalTimeInterval::operator=(
    rti::LogicalTimeInterval const& value) {
  value_ = portableLogicalTimeIntervalValue(value);
  return *this;
}

PortableLogicalTimeInterval& PortableLogicalTimeInterval::operator+=(
    rti::LogicalTimeInterval const& addend) {
  value_ += portableLogicalTimeIntervalValue(addend);
  return *this;
}

PortableLogicalTimeInterval& PortableLogicalTimeInterval::operator-=(
    rti::LogicalTimeInterval const& subtrahend) {
  value_ -= portableLogicalTimeIntervalValue(subtrahend);
  return *this;
}

bool PortableLogicalTimeInterval::operator>(
    rti::LogicalTimeInterval const& value) const {
  return value_ > portableLogicalTimeIntervalValue(value);
}
bool PortableLogicalTimeInterval::operator<(
    rti::LogicalTimeInterval const& value) const {
  return value_ < portableLogicalTimeIntervalValue(value);
}
bool PortableLogicalTimeInterval::operator==(
    rti::LogicalTimeInterval const& value) const {
  return value_ == portableLogicalTimeIntervalValue(value);
}
bool PortableLogicalTimeInterval::operator>=(
    rti::LogicalTimeInterval const& value) const {
  return value_ >= portableLogicalTimeIntervalValue(value);
}
bool PortableLogicalTimeInterval::operator<=(
    rti::LogicalTimeInterval const& value) const {
  return value_ <= portableLogicalTimeIntervalValue(value);
}

void PortableLogicalTimeInterval::setToDifference(
    rti::LogicalTime const& minuend,
    rti::LogicalTime const& subtrahend) {
  value_ = portableLogicalTimeValue(minuend) -
           portableLogicalTimeValue(subtrahend);
}

rti::VariableLengthData PortableLogicalTimeInterval::encode() const {
  auto const bytes = encodePortableLogicalTimeValue(value_);
  return rti::VariableLengthData(bytes.data(), bytes.size());
}

std::size_t PortableLogicalTimeInterval::encode(
    void* buffer,
    std::size_t bufferSize) const {
  if (buffer == nullptr || bufferSize < portableLogicalTimeEncodingSize) {
    throw rti::CouldNotEncode(
        L"PortableLogicalTimeInterval output buffer is too small.");
  }
  auto const bytes = encodePortableLogicalTimeValue(value_);
  auto* output = static_cast<rti::Octet*>(buffer);
  std::copy(bytes.begin(), bytes.end(), output);
  return bytes.size();
}

std::size_t PortableLogicalTimeInterval::encodedLength() const {
  return portableLogicalTimeEncodingSize;
}

void PortableLogicalTimeInterval::decode(
    rti::VariableLengthData const& encodedValue) {
  if (encodedValue.size() != portableLogicalTimeEncodingSize) {
    throw rti::CouldNotDecode(
        L"PortableLogicalTimeInterval encoding has the wrong length.");
  }
  value_ = decodePortableLogicalTimeValue(
      static_cast<std::uint8_t const*>(encodedValue.data()));
}

void PortableLogicalTimeInterval::decode(
    void const* buffer,
    std::size_t bufferSize) {
  if (buffer == nullptr || bufferSize != portableLogicalTimeEncodingSize) {
    throw rti::CouldNotDecode(
        L"PortableLogicalTimeInterval input buffer has the wrong length.");
  }
  value_ = decodePortableLogicalTimeValue(
      static_cast<std::uint8_t const*>(buffer));
}

std::wstring PortableLogicalTimeInterval::toString() const {
  return std::to_wstring(value_);
}
std::wstring PortableLogicalTimeInterval::implementationName() const {
  return L"PortableLogicalTime";
}
std::int64_t PortableLogicalTimeInterval::value() const { return value_; }

std::unique_ptr<rti::LogicalTime> PortableLogicalTimeFactory::makeInitial() {
  return std::make_unique<PortableLogicalTime>();
}
std::unique_ptr<rti::LogicalTime> PortableLogicalTimeFactory::makeFinal() {
  auto result = std::make_unique<PortableLogicalTime>();
  result->setFinal();
  return result;
}
std::unique_ptr<rti::LogicalTimeInterval> PortableLogicalTimeFactory::makeZero() {
  return std::make_unique<PortableLogicalTimeInterval>();
}
std::unique_ptr<rti::LogicalTimeInterval> PortableLogicalTimeFactory::makeEpsilon() {
  auto result = std::make_unique<PortableLogicalTimeInterval>();
  result->setEpsilon();
  return result;
}
std::unique_ptr<rti::LogicalTime> PortableLogicalTimeFactory::decodeLogicalTime(
    rti::VariableLengthData const& encodedLogicalTime) {
  auto result = std::make_unique<PortableLogicalTime>();
  result->decode(encodedLogicalTime);
  return result;
}
std::unique_ptr<rti::LogicalTime> PortableLogicalTimeFactory::decodeLogicalTime(
    void const* buffer,
    std::size_t bufferSize) {
  auto result = std::make_unique<PortableLogicalTime>();
  result->decode(buffer, bufferSize);
  return result;
}
std::unique_ptr<rti::LogicalTimeInterval>
PortableLogicalTimeFactory::decodeLogicalTimeInterval(
    rti::VariableLengthData const& encodedValue) {
  auto result = std::make_unique<PortableLogicalTimeInterval>();
  result->decode(encodedValue);
  return result;
}
std::unique_ptr<rti::LogicalTimeInterval>
PortableLogicalTimeFactory::decodeLogicalTimeInterval(
    void const* buffer,
    std::size_t bufferSize) {
  auto result = std::make_unique<PortableLogicalTimeInterval>();
  result->decode(buffer, bufferSize);
  return result;
}
std::wstring PortableLogicalTimeFactory::getName() const {
  return L"PortableLogicalTime";
}

void verifyRequiredStandardLogicalTimeRepresentations() {
  rti::HLAinteger64Time requiredIntegerInitial;
  requiredIntegerInitial.setInitial();
  rti::HLAinteger64Time requiredIntegerFinal;
  requiredIntegerFinal.setFinal();
  rti::HLAfloat64Time requiredFloatInitial;
  requiredFloatInitial.setInitial();
  rti::HLAfloat64Time requiredFloatFinal;
  requiredFloatFinal.setFinal();
  rti::HLAinteger64TimeFactory requiredIntegerFactory;
  rti::HLAfloat64TimeFactory requiredFloatFactory;
  require(
      requiredIntegerInitial.isInitial() && requiredIntegerFinal.isFinal() &&
          requiredIntegerInitial < requiredIntegerFinal &&
          requiredIntegerFactory.getName() == L"HLAinteger64Time" &&
          requiredFloatInitial.isInitial() && requiredFloatFinal.isFinal() &&
          requiredFloatInitial < requiredFloatFinal &&
          requiredFloatFactory.getName() == L"HLAfloat64Time",
      "RTI did not provide both standard logical-time representations "
      "with initial/final points");
}

template <typename Value, typename FactoryDecoder>
void requireOffsetEncodingRoundTrip(
    Value const& value,
    FactoryDecoder&& factoryDecoder,
    std::string const& description) {
  auto const encodedLength = value.encodedLength();
  auto const sentinel = static_cast<rti::Octet>(0xa5U);
  std::vector<rti::Octet> buffer(encodedLength + 2U, sentinel);
  auto const encodedSize = value.encode(buffer.data() + 1U, encodedLength);
  require(
      encodedSize == encodedLength,
      description + " did not report its complete direct-buffer encoding");
  require(
      buffer.front() == sentinel && buffer.back() == sentinel,
      description + " wrote outside the selected buffer offset");

  Value decoded;
  decoded.decode(buffer.data() + 1U, encodedSize);
  require(
      decoded.operator==(value),
      description + " did not decode from a nonzero offset");

  auto factoryDecoded = factoryDecoder(buffer.data() + 1U, encodedSize);
  require(
      factoryDecoded != nullptr && factoryDecoded->operator==(value),
      description + " factory did not decode from a nonzero offset");
}

void verifyReferenceTimeTypes() {
  verifyRequiredStandardLogicalTimeRepresentations();
  rti::HLAinteger64Time integerTime(41);
  require(
      !integerTime.toString().empty(),
      "HLAinteger64Time returned no printable string representation");
  rti::HLAinteger64Interval integerEpsilon;
  integerEpsilon.setEpsilon();
  require(
      integerTime.getTime() == 41 &&
          integerTime.implementationName() == L"HLAinteger64Time" &&
          integerEpsilon.getInterval() == 1 && integerEpsilon.isEpsilon(),
      "HLAinteger64Time did not preserve its standard value and epsilon");
  integerTime += integerEpsilon;
  require(integerTime.getTime() == 42, "HLAinteger64Time addition was incorrect");
  integerTime -= integerEpsilon;
  require(integerTime.getTime() == 41, "HLAinteger64Time subtraction was incorrect");
  rti::HLAinteger64Time integerCopied(integerTime);
  rti::HLAinteger64Time integerAssigned;
  integerAssigned = integerTime;
  integerTime.setTime(43);
  require(
      integerCopied.getTime() == 41 && integerAssigned.getTime() == 41,
      "HLAinteger64Time copy or assignment was not independent");
  integerTime.setTime(41);

  rti::HLAinteger64Interval integerInterval(3);
  require(
      !integerInterval.toString().empty(),
      "HLAinteger64Interval returned no printable string representation");
  rti::HLAinteger64Interval integerIntervalCopied(integerInterval);
  rti::HLAinteger64Interval integerIntervalAssigned;
  integerIntervalAssigned = integerInterval;
  integerInterval.setInterval(4);
  require(
      integerIntervalCopied.getInterval() == 3 &&
          integerIntervalAssigned.getInterval() == 3,
      "HLAinteger64Interval copy or assignment was not independent");
  integerInterval.setZero();
  require(
      integerInterval.isZero() && integerInterval.getInterval() == 0,
      "HLAinteger64Interval zero mutator did not produce zero");
  integerInterval.setEpsilon();
  require(
      integerInterval.isEpsilon() && integerInterval.getInterval() == 1,
      "HLAinteger64Interval epsilon mutator did not produce epsilon");
  require(
      copyBytes(integerIntervalCopied.encode()) ==
          std::vector<std::uint8_t>{0U, 0U, 0U, 0U, 0U, 0U, 0U, 3U},
      "HLAinteger64Interval did not use HLAinteger64BE encoding");
  requireDirectTimeEncoding(
      integerIntervalCopied,
      "HLAinteger64Interval");

  rti::HLAinteger64Interval integerDifference;
  rti::HLAinteger64Time integerLater(44);
  integerDifference.setToDifference(integerLater, integerTime);
  require(
      integerDifference.getInterval() == 3,
      "HLAinteger64Interval difference was incorrect");
  require(
      copyBytes(integerTime.encode()) ==
          std::vector<std::uint8_t>{0U, 0U, 0U, 0U, 0U, 0U, 0U, 41U},
      "HLAinteger64Time did not use HLAinteger64BE encoding");
  requireDirectTimeEncoding(integerTime, "HLAinteger64Time");
  rti::HLAinteger64Time integerDecoded;
  integerDecoded.decode(integerTime.encode());
  require(
      integerDecoded.getTime() == integerTime.getTime(),
      "HLAinteger64Time did not decode its own encoding");

  rti::HLAinteger64Time integerFinal;
  integerFinal.setFinal();
  require(
      integerFinal.isFinal() &&
          integerFinal.getTime() == std::numeric_limits<rti::Integer64>::max(),
      "HLAinteger64Time final boundary was incorrect");
  rti::HLAinteger64Time integerInitial;
  require(integerInitial.isInitial(), "HLAinteger64Time initial boundary was incorrect");
  require(
      integerInitial < integerTime && integerTime < integerFinal,
      "HLAinteger64Time did not compare a finite value against both boundaries");
  integerInitial -= integerEpsilon;
  require(
      integerInitial.isInitial() && integerInitial.getTime() == 0,
      "HLAinteger64Time subtraction below initial did not clamp to initial");
  rti::HLAinteger64Interval integerZero;
  integerZero -= integerEpsilon;
  require(
      integerZero.isZero() && integerZero.getInterval() == 0,
      "HLAinteger64Interval subtraction below zero did not clamp to zero");
  integerFinal += integerEpsilon;
  require(
      integerFinal.isFinal() &&
          integerFinal.getTime() == std::numeric_limits<rti::Integer64>::max(),
      "HLAinteger64Time addition to final did not preserve final");
  integerFinal -= integerEpsilon;
  require(
      integerFinal.isFinal() &&
          integerFinal.getTime() == std::numeric_limits<rti::Integer64>::max(),
      "HLAinteger64Time subtraction from final did not preserve final");
  requireException(
      [&] { static_cast<void>(rti::HLAinteger64Time(-1)); },
      L"InvalidLogicalTime",
      "negative HLAinteger64Time construction");

  rti::HLAinteger64TimeFactory integerFactory;
  requireOffsetEncodingRoundTrip(
      integerTime,
      [&integerFactory](void const* buffer, std::size_t size) {
        return integerFactory.decodeLogicalTime(buffer, size);
      },
      "HLAinteger64Time");
  requireOffsetEncodingRoundTrip(
      integerIntervalCopied,
      [&integerFactory](void const* buffer, std::size_t size) {
        return integerFactory.decodeLogicalTimeInterval(buffer, size);
      },
      "HLAinteger64Interval");
  std::vector<rti::Octet> integerFactoryEncoding{
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(1U),
      static_cast<rti::Octet>(2U)};
  auto integerFactoryDecoded = integerFactory.decodeLogicalTime(
      integerFactoryEncoding.data(),
      integerFactoryEncoding.size());
  auto* integerConcrete = dynamic_cast<rti::HLAinteger64Time*>(
      integerFactoryDecoded.get());
  require(
      integerConcrete != nullptr && integerConcrete->getTime() == 258 &&
          integerFactory.getName() == L"HLAinteger64Time",
      "HLAinteger64TimeFactory did not decode its standard value");

  auto integerFactoryMade = integerFactory.makeLogicalTime(259);
  auto integerFactoryInitial = integerFactory.makeInitial();
  auto* integerFactoryInitialConcrete =
      dynamic_cast<rti::HLAinteger64Time*>(integerFactoryInitial.get());
  auto integerFactoryFinal = integerFactory.makeFinal();
  auto* integerFactoryFinalConcrete =
      dynamic_cast<rti::HLAinteger64Time*>(integerFactoryFinal.get());
  require(
      integerFactoryMade->getTime() == 259 &&
          integerFactoryInitialConcrete != nullptr &&
          integerFactoryInitialConcrete->isInitial() &&
          integerFactoryFinalConcrete != nullptr &&
          integerFactoryFinalConcrete->isFinal(),
      "HLAinteger64TimeFactory returned an unexpected logical-time type or value");

  auto integerFactoryInterval = integerFactory.makeLogicalTimeInterval(258);
  auto integerFactoryZero = integerFactory.makeZero();
  auto* integerFactoryZeroConcrete =
      dynamic_cast<rti::HLAinteger64Interval*>(integerFactoryZero.get());
  auto integerFactoryEpsilon = integerFactory.makeEpsilon();
  auto* integerFactoryEpsilonConcrete =
      dynamic_cast<rti::HLAinteger64Interval*>(integerFactoryEpsilon.get());
  auto integerFactoryDecodedInterval = integerFactory.decodeLogicalTimeInterval(
      integerFactoryEncoding.data(),
      integerFactoryEncoding.size());
  auto* integerFactoryDecodedIntervalConcrete =
      dynamic_cast<rti::HLAinteger64Interval*>(
          integerFactoryDecodedInterval.get());
  require(
      integerFactoryInterval != nullptr &&
          integerFactoryInterval->getInterval() == 258 &&
          integerFactoryZeroConcrete != nullptr &&
          integerFactoryZeroConcrete->isZero() &&
          integerFactoryEpsilonConcrete != nullptr &&
          integerFactoryEpsilonConcrete->isEpsilon() &&
          integerFactoryDecodedIntervalConcrete != nullptr &&
          integerFactoryDecodedIntervalConcrete->getInterval() == 258,
      "HLAinteger64TimeFactory returned an unexpected logical-time-interval type or value");

  rti::HLAfloat64Time floatInitial;
  rti::HLAfloat64Time floatFinal;
  rti::HLAfloat64Interval floatEpsilon;
  floatFinal.setFinal();
  floatEpsilon.setEpsilon();
  constexpr double floatFinalValue = std::numeric_limits<double>::max();
  constexpr double floatEpsilonValue = std::numeric_limits<double>::denorm_min();
  require(
      floatInitial.isInitial() && floatInitial.getTime() == 0.0 &&
          floatFinal.isFinal() &&
          floatFinal.getTime() == floatFinalValue && floatEpsilon.isEpsilon() &&
          floatEpsilon.getInterval() == floatEpsilonValue &&
          floatFinal.implementationName() == L"HLAfloat64Time",
      "HLAfloat64Time did not preserve its standard boundaries and epsilon");
  floatInitial -= floatEpsilon;
  require(
      floatInitial.isInitial() && floatInitial.getTime() == 0.0,
      "HLAfloat64Time subtraction below initial did not clamp to initial");
  rti::HLAfloat64Time floatEpsilonFromInitial;
  rti::LogicalTime& floatEpsilonFromInitialResult =
      floatEpsilonFromInitial.operator+=(floatEpsilon);
  require(
      &floatEpsilonFromInitialResult == &floatEpsilonFromInitial &&
          dynamic_cast<rti::HLAfloat64Time*>(&floatEpsilonFromInitialResult) ==
              &floatEpsilonFromInitial &&
          floatEpsilonFromInitialResult.implementationName() == L"HLAfloat64Time" &&
          floatEpsilonFromInitial.getTime() == floatEpsilonValue,
      "HLAfloat64Time epsilon addition from initial did not make the smallest guaranteed change");
  rti::HLAfloat64Interval floatZero;
  floatZero -= floatEpsilon;
  require(
      floatZero.isZero() && floatZero.getInterval() == 0.0,
      "HLAfloat64Interval subtraction below zero did not clamp to zero");
  floatFinal += floatEpsilon;
  require(floatFinal.isFinal() && floatFinal.getTime() == floatFinalValue,
          "HLAfloat64Time addition to final did not preserve final");
  floatFinal -= floatEpsilon;
  require(floatFinal.isFinal() && floatFinal.getTime() == floatFinalValue,
          "HLAfloat64Time subtraction from final did not preserve final");
  rti::HLAfloat64Time floatLarge(floatFinalValue / 2.0);
  auto const floatBefore = floatLarge.getTime();
  auto const floatNext = std::nextafter(floatBefore, floatFinalValue);
  auto const floatPrevious = std::nextafter(floatBefore, 0.0);
  require(
      floatEpsilonValue > 0.0 &&
          floatEpsilonValue < floatNext - floatBefore &&
          floatEpsilonValue < floatBefore - floatPrevious,
      "HLAfloat64Interval epsilon was not smaller than the local time spacing");
  floatLarge += floatEpsilon;
  require(
      floatLarge.getTime() == floatNext,
      "HLAfloat64Time epsilon did not advance by one representable step");
  floatLarge -= floatEpsilon;
  require(
      floatLarge.getTime() == floatBefore,
      "HLAfloat64Time epsilon subtraction did not restore the value");
  rti::HLAfloat64Time floatSubtracting(floatBefore);
  floatSubtracting -= floatEpsilon;
  require(
      floatSubtracting.getTime() == floatPrevious,
      "HLAfloat64Time sub-epsilon subtraction did not retreat by one representable step");

  rti::HLAfloat64Time floatValue(1.5);
  require(
      !floatValue.toString().empty(),
      "HLAfloat64Time returned no printable string representation");
  require(
      floatInitial < floatValue && floatValue < floatFinal,
      "HLAfloat64Time did not compare a finite value against both boundaries");
  require(
      copyBytes(floatValue.encode()) ==
          std::vector<std::uint8_t>{0x3FU, 0xF8U, 0U, 0U, 0U, 0U, 0U, 0U},
      "HLAfloat64Time did not use HLAfloat64BE encoding");
  rti::HLAfloat64Time floatWireValue(3.25);
  require(
      copyBytes(floatWireValue.encode()) ==
          std::vector<std::uint8_t>{0x40U, 0x0AU, 0U, 0U, 0U, 0U, 0U, 0U},
      "HLAfloat64Time did not preserve a second HLAfloat64BE golden vector");
  requireDirectTimeEncoding(floatValue, "HLAfloat64Time");
  rti::HLAfloat64Time floatDecoded;
  floatDecoded.decode(floatValue.encode());
  require(
      floatDecoded.getTime() == 1.5,
      "HLAfloat64Time did not decode its own encoding");
  rti::HLAfloat64Time floatCopied(floatValue);
  rti::HLAfloat64Time floatAssigned;
  rti::HLAfloat64Time& concreteAssignmentResult = (floatAssigned = floatValue);
  require(
      &concreteAssignmentResult == &floatAssigned &&
          concreteAssignmentResult.implementationName() == L"HLAfloat64Time",
      "HLAfloat64Time copy assignment did not return its concrete receiver");
  rti::LogicalTime const& abstractFloatValue = floatValue;
  rti::HLAfloat64Time floatAssignedThroughBase;
  rti::HLAfloat64Time& baseAssignmentResult =
      floatAssignedThroughBase.operator=(abstractFloatValue);
  require(
      &baseAssignmentResult == &floatAssignedThroughBase &&
          baseAssignmentResult.implementationName() == L"HLAfloat64Time",
      "HLAfloat64Time base-interface assignment did not return its concrete receiver");
  floatValue.setTime(2.5);
  require(
      floatCopied.getTime() == 1.5 && floatAssigned.getTime() == 1.5 &&
          floatAssignedThroughBase.getTime() == 1.5,
      "HLAfloat64Time copy or assignment was not independent");
  floatValue.setTime(1.5);

  rti::HLAfloat64Time floatArithmeticProbe(1.5);
  rti::HLAfloat64Interval floatArithmeticInterval(0.5);
  rti::LogicalTime& floatAdditionResult =
      floatArithmeticProbe.operator+=(floatArithmeticInterval);
  require(
      &floatAdditionResult == &floatArithmeticProbe &&
          dynamic_cast<rti::HLAfloat64Time*>(&floatAdditionResult) ==
              &floatArithmeticProbe &&
          floatAdditionResult.implementationName() == L"HLAfloat64Time" &&
          floatArithmeticProbe.getTime() == 2.0,
      "HLAfloat64Time addition did not return a reference to its concrete receiver");
  rti::LogicalTime& floatSubtractionResult =
      floatArithmeticProbe.operator-=(floatArithmeticInterval);
  require(
      &floatSubtractionResult == &floatArithmeticProbe &&
          dynamic_cast<rti::HLAfloat64Time*>(&floatSubtractionResult) ==
              &floatArithmeticProbe &&
          floatSubtractionResult.implementationName() == L"HLAfloat64Time" &&
          floatArithmeticProbe.getTime() == 1.5,
      "HLAfloat64Time subtraction did not return a reference to its concrete receiver");

  rti::HLAfloat64Interval floatInterval(2.5);
  require(
      !floatInterval.toString().empty(),
      "HLAfloat64Interval returned no printable string representation");
  require(
      copyBytes(floatInterval.encode()) ==
          std::vector<std::uint8_t>{0x40U, 0x04U, 0U, 0U, 0U, 0U, 0U, 0U},
      "HLAfloat64Interval did not use HLAfloat64BE encoding");
  rti::HLAfloat64Interval floatIntervalCopied(floatInterval);
  rti::HLAfloat64Interval floatIntervalAssigned;
  floatIntervalAssigned = floatInterval;
  floatInterval.setInterval(3.5);
  require(
      floatIntervalCopied.getInterval() == 2.5 &&
          floatIntervalAssigned.getInterval() == 2.5,
      "HLAfloat64Interval copy or assignment was not independent");
  floatInterval.setZero();
  require(
      floatInterval.isZero() && floatInterval.getInterval() == 0.0,
      "HLAfloat64Interval zero mutator did not produce zero");
  floatInterval.setEpsilon();
  require(
      floatInterval.isEpsilon() &&
          floatInterval.getInterval() == std::numeric_limits<double>::denorm_min(),
      "HLAfloat64Interval epsilon mutator did not produce epsilon");
  requireDirectTimeEncoding(
      floatIntervalCopied,
      "HLAfloat64Interval");
  rti::HLAfloat64Interval floatDifference;
  rti::HLAfloat64Time floatLater(4.5);
  floatDifference.setToDifference(floatLater, floatValue);
  require(
      floatDifference.getInterval() == 3.0,
      "HLAfloat64Interval difference was incorrect");
  requireException(
      [&] {
        static_cast<void>(
            rti::HLAfloat64Time(std::numeric_limits<double>::infinity()));
      },
      L"InvalidLogicalTime",
      "nonfinite HLAfloat64Time construction");

  std::vector<rti::Octet> positiveInfinity{
      static_cast<rti::Octet>(0x7FU),
      static_cast<rti::Octet>(0xF0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U)};
  requireException(
      [&] {
        rti::HLAfloat64Time malformed;
        malformed.decode(positiveInfinity.data(), positiveInfinity.size());
      },
      L"CouldNotDecode",
      "positive-infinity HLAfloat64Time encoding");
  rti::HLAfloat64TimeFactory floatFactory;
  requireOffsetEncodingRoundTrip(
      floatValue,
      [&floatFactory](void const* buffer, std::size_t size) {
        return floatFactory.decodeLogicalTime(buffer, size);
      },
      "HLAfloat64Time");
  requireOffsetEncodingRoundTrip(
      floatIntervalCopied,
      [&floatFactory](void const* buffer, std::size_t size) {
        return floatFactory.decodeLogicalTimeInterval(buffer, size);
      },
      "HLAfloat64Interval");
  std::vector<rti::Octet> floatFactoryEncoding{
      static_cast<rti::Octet>(0x40U),
      static_cast<rti::Octet>(0x0AU),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U)};
  auto floatFactoryDecoded = floatFactory.decodeLogicalTime(
      floatFactoryEncoding.data(),
      floatFactoryEncoding.size());
  auto* floatConcrete = dynamic_cast<rti::HLAfloat64Time*>(
      floatFactoryDecoded.get());
  require(
      floatConcrete != nullptr && floatConcrete->getTime() == 3.25 &&
          floatFactory.getName() == L"HLAfloat64Time",
      "HLAfloat64TimeFactory did not decode its standard value");

  auto floatFactoryMade = floatFactory.makeLogicalTime(3.25);
  auto floatFactoryInitial = floatFactory.makeInitial();
  auto* floatFactoryInitialConcrete =
      dynamic_cast<rti::HLAfloat64Time*>(floatFactoryInitial.get());
  auto floatFactoryFinal = floatFactory.makeFinal();
  auto* floatFactoryFinalConcrete =
      dynamic_cast<rti::HLAfloat64Time*>(floatFactoryFinal.get());
  auto floatFactoryInterval = floatFactory.makeLogicalTimeInterval(3.25);
  auto* floatFactoryIntervalConcrete =
      dynamic_cast<rti::HLAfloat64Interval*>(floatFactoryInterval.get());
  auto floatFactoryZero = floatFactory.makeZero();
  auto* floatFactoryZeroConcrete =
      dynamic_cast<rti::HLAfloat64Interval*>(floatFactoryZero.get());
  auto floatFactoryEpsilon = floatFactory.makeEpsilon();
  auto* floatFactoryEpsilonConcrete =
      dynamic_cast<rti::HLAfloat64Interval*>(floatFactoryEpsilon.get());
  auto floatFactoryDecodedInterval = floatFactory.decodeLogicalTimeInterval(
      floatFactoryEncoding.data(),
      floatFactoryEncoding.size());
  auto* floatFactoryDecodedIntervalConcrete =
      dynamic_cast<rti::HLAfloat64Interval*>(
          floatFactoryDecodedInterval.get());
  require(
      floatFactoryMade != nullptr && floatFactoryMade->getTime() == 3.25 &&
          floatFactoryInitialConcrete != nullptr &&
          floatFactoryInitialConcrete->isInitial() &&
          floatFactoryFinalConcrete != nullptr &&
          floatFactoryFinalConcrete->isFinal() &&
          floatFactoryInterval != nullptr &&
          floatFactoryIntervalConcrete != nullptr &&
          floatFactoryInterval->getInterval() == 3.25 &&
          floatFactoryZeroConcrete != nullptr &&
          floatFactoryZeroConcrete->isZero() &&
          floatFactoryEpsilonConcrete != nullptr &&
          floatFactoryEpsilonConcrete->isEpsilon() &&
          floatFactoryDecodedIntervalConcrete != nullptr &&
          floatFactoryDecodedIntervalConcrete->getInterval() == 3.25,
      "HLAfloat64TimeFactory returned unexpected logical-time or interval values");

  verifyLogicalTimeIdentitySet<rti::HLAinteger64Time, rti::HLAinteger64Interval>(
      "HLAinteger64Time");
  verifyLogicalTimeIdentitySet<rti::HLAfloat64Time, rti::HLAfloat64Interval>(
      "HLAfloat64Time");
  verifyStandardLogicalTimeFactoryFactoryContract();
}

void verifyLogicalTimeDataElements(Session& session, TimeContext const& time) {
  rti::HLAlogicalTime value(&session.rtiAmbassador());
  auto const initialEncoding = copyBytes(value.encode());
  require(
      initialEncoding == copyBytes(time.initial->encode()) &&
          value.getEncodedLength() == time.initial->encodedLength() &&
          value.getOctetBoundary() == 1U,
      "HLAlogicalTime did not expose the selected factory's initial encoding");
  rti::HLAlogicalTime sameType(&session.rtiAmbassador());
  require(
      value.isSameTypeAs(sameType),
      "HLAlogicalTime did not identify the selected implementation type");

  value.set(*time.initial);
  auto const valueEncoding = value.encode();
  rti::HLAlogicalTime decoded(&session.rtiAmbassador());
  decoded.decode(valueEncoding);
  auto recovered = time.factory->makeInitial();
  decoded.get(*recovered);
  require(
      *recovered == *time.initial,
      "HLAlogicalTime did not round-trip through its standard data-element wrapper");

  std::vector<rti::Octet> nested{static_cast<rti::Octet>(0xA4U)};
  value.encodeInto(nested);
  nested.push_back(static_cast<rti::Octet>(0x5AU));
  rti::HLAlogicalTime nestedDecoded(&session.rtiAmbassador());
  auto const nextIndex = nestedDecoded.decodeFrom(nested, 1U);
  require(
      nextIndex == 1U + value.getEncodedLength() &&
          nested.front() == static_cast<rti::Octet>(0xA4U) &&
          nested.back() == static_cast<rti::Octet>(0x5AU),
      "HLAlogicalTime did not preserve nested-buffer boundaries");
  auto nestedRecovered = time.factory->makeInitial();
  nestedDecoded.get(*nestedRecovered);
  require(
      *nestedRecovered == *time.initial,
      "HLAlogicalTime nested-buffer decoding returned the wrong value");

  rti::HLAlogicalTime copied(value);
  auto clone = value.clone();
  auto* cloned = dynamic_cast<rti::HLAlogicalTime*>(clone.get());
  require(cloned != nullptr, "HLAlogicalTime clone returned the wrong type");
  value.set(*time.finalTime);
  auto copiedValue = time.factory->makeInitial();
  auto clonedValue = time.factory->makeInitial();
  copied.get(*copiedValue);
  cloned->get(*clonedValue);
  require(
      *copiedValue == *time.initial && *clonedValue == *time.initial,
      "HLAlogicalTime copy and clone did not preserve independent storage");

  if (time.factory->getName() == L"HLAfloat64Time") {
    rti::HLAinteger64Time incompatible;
    requireException(
        [&] { static_cast<void>(value.set(incompatible)); },
        L"EncoderException",
        "HLAlogicalTime accepted an incompatible integer value");
    requireException(
        [&] { value.get(incompatible); },
        L"InvalidLogicalTime",
        "HLAlogicalTime wrote into an incompatible integer value");
  } else if (time.factory->getName() == L"HLAinteger64Time") {
    rti::HLAfloat64Time incompatible;
    requireException(
        [&] { static_cast<void>(value.set(incompatible)); },
        L"EncoderException",
        "HLAlogicalTime accepted an incompatible floating-point value");
    requireException(
        [&] { value.get(incompatible); },
        L"InvalidLogicalTime",
        "HLAlogicalTime wrote into an incompatible floating-point value");
  }

  auto truncatedBytes = copyBytes(valueEncoding);
  require(
      !truncatedBytes.empty(),
      "HLAlogicalTime produced an empty encoding");
  truncatedBytes.pop_back();
  std::vector<rti::Octet> truncated(truncatedBytes.begin(), truncatedBytes.end());
  rti::VariableLengthData malformed(truncated.data(), truncated.size());
  requireException(
      [&] { decoded.decode(malformed); },
      L"CouldNotDecode",
      "HLAlogicalTime accepted a truncated variable-length value");
  requireException(
      [&] { static_cast<void>(decoded.decodeFrom(truncated, 0U)); },
      L"EncoderException",
      "HLAlogicalTime accepted a truncated nested-buffer value");

  rti::HLAlogicalTimeInterval interval(&session.rtiAmbassador());
  auto const zeroEncoding = copyBytes(interval.encode());
  require(
      zeroEncoding == copyBytes(time.zero->encode()) &&
          interval.getEncodedLength() == time.zero->encodedLength() &&
          interval.getOctetBoundary() == 1U,
      "HLAlogicalTimeInterval did not expose the selected factory's zero encoding");
  rti::HLAlogicalTimeInterval sameIntervalType(&session.rtiAmbassador());
  require(
      interval.isSameTypeAs(sameIntervalType),
      "HLAlogicalTimeInterval did not identify the selected implementation type");

  interval.set(*time.epsilon);
  auto const intervalEncoding = interval.encode();
  rti::HLAlogicalTimeInterval decodedInterval(&session.rtiAmbassador());
  decodedInterval.decode(intervalEncoding);
  auto recoveredInterval = time.factory->makeZero();
  decodedInterval.get(*recoveredInterval);
  require(
      *recoveredInterval == *time.epsilon,
      "HLAlogicalTimeInterval did not round-trip through its standard data-element wrapper");

  std::vector<rti::Octet> nestedInterval{static_cast<rti::Octet>(0xA4U)};
  interval.encodeInto(nestedInterval);
  nestedInterval.push_back(static_cast<rti::Octet>(0x5AU));
  rti::HLAlogicalTimeInterval nestedIntervalDecoded(&session.rtiAmbassador());
  auto const intervalNextIndex = nestedIntervalDecoded.decodeFrom(nestedInterval, 1U);
  require(
      intervalNextIndex == 1U + interval.getEncodedLength() &&
          nestedInterval.front() == static_cast<rti::Octet>(0xA4U) &&
          nestedInterval.back() == static_cast<rti::Octet>(0x5AU),
      "HLAlogicalTimeInterval did not preserve nested-buffer boundaries");
  auto nestedIntervalRecovered = time.factory->makeZero();
  nestedIntervalDecoded.get(*nestedIntervalRecovered);
  require(
      *nestedIntervalRecovered == *time.epsilon,
      "HLAlogicalTimeInterval nested-buffer decoding returned the wrong value");

  rti::HLAlogicalTimeInterval copiedInterval(interval);
  auto intervalClone = interval.clone();
  auto* clonedInterval =
      dynamic_cast<rti::HLAlogicalTimeInterval*>(intervalClone.get());
  require(
      clonedInterval != nullptr,
      "HLAlogicalTimeInterval clone returned the wrong type");
  interval.set(*time.zero);
  auto copiedIntervalValue = time.factory->makeZero();
  auto clonedIntervalValue = time.factory->makeZero();
  copiedInterval.get(*copiedIntervalValue);
  clonedInterval->get(*clonedIntervalValue);
  require(
      *copiedIntervalValue == *time.epsilon &&
          *clonedIntervalValue == *time.epsilon,
      "HLAlogicalTimeInterval copy and clone did not preserve independent storage");

  if (time.factory->getName() == L"HLAfloat64Time") {
    rti::HLAinteger64Interval incompatible;
    requireException(
        [&] { static_cast<void>(interval.set(incompatible)); },
        L"EncoderException",
        "HLAlogicalTimeInterval accepted an incompatible integer interval");
    requireException(
        [&] { interval.get(incompatible); },
        L"InvalidLogicalTimeInterval",
        "HLAlogicalTimeInterval wrote into an incompatible integer interval");
  } else if (time.factory->getName() == L"HLAinteger64Time") {
    rti::HLAfloat64Interval incompatible;
    requireException(
        [&] { static_cast<void>(interval.set(incompatible)); },
        L"EncoderException",
        "HLAlogicalTimeInterval accepted an incompatible floating-point interval");
    requireException(
        [&] { interval.get(incompatible); },
        L"InvalidLogicalTimeInterval",
        "HLAlogicalTimeInterval wrote into an incompatible floating-point interval");
  }

  auto truncatedIntervalBytes = copyBytes(intervalEncoding);
  require(
      !truncatedIntervalBytes.empty(),
      "HLAlogicalTimeInterval produced an empty encoding");
  truncatedIntervalBytes.pop_back();
  std::vector<rti::Octet> truncatedInterval(
      truncatedIntervalBytes.begin(),
      truncatedIntervalBytes.end());
  rti::VariableLengthData malformedInterval(
      truncatedInterval.data(),
      truncatedInterval.size());
  requireException(
      [&] { decodedInterval.decode(malformedInterval); },
      L"CouldNotDecode",
      "HLAlogicalTimeInterval accepted a truncated variable-length value");
  requireException(
      [&] {
        static_cast<void>(decodedInterval.decodeFrom(truncatedInterval, 0U));
      },
      L"EncoderException",
      "HLAlogicalTimeInterval accepted a truncated nested-buffer value");
}


void scenarioLogicalTimeFactoryPortable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "logical-time factory scenario requires an adapter-supplied time representation");
  Session session(options, model, "time");
  requireException(
      [&] { static_cast<void>(session.rtiAmbassador().getTimeFactory()); },
      L"NotConnected",
      "getting the logical-time factory before connect");
  session.connect();
  requireException(
      [&] { static_cast<void>(session.rtiAmbassador().getTimeFactory()); },
      L"FederateNotExecutionMember",
      "getting the logical-time factory before joining");
  auto const federation = federationName(options, "logical-time-factory");
  session.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  session.join(options.ownerFederateName, options.federateType, federation);

  auto time = makeTimeContext(session);
  verifyReferenceTimeTypes();
  verifyLogicalTimeDataElements(session, time);
  require(
      time.factory->getName() == options.logicalTimeImplementationName,
      "RTI logical-time factory did not match the adapter-specified time representation");
  require(
      time.factory->getName() == time.initial->implementationName(),
      "logical-time factory name did not match the returned logical-time implementation");

  auto const initialEncoding = encodeTime(*time.initial);
  auto const initialEncodedValue = time.initial->encode();
  auto decodedInitial = time.factory->decodeLogicalTime(time.initial->encode());
  require(decodedInitial != nullptr, "logical-time factory returned no decoded initial time");
  requireTimeEquals(
      *decodedInitial,
      *time.initial,
      "logical-time initial encode/decode");
  auto decodedInitialFromBuffer = time.factory->decodeLogicalTime(
      initialEncodedValue.data(),
      time.initial->encodedLength());
  require(decodedInitialFromBuffer != nullptr,
          "logical-time factory returned no buffer-decoded initial time");
  requireTimeEquals(
      *decodedInitialFromBuffer,
      *time.initial,
      "logical-time initial buffer decode");
  require(
      initialEncoding.size() == time.initial->encodedLength(),
      "logical-time encoded length did not match the encoded value");
  requireDirectTimeEncoding(
      *time.initial,
      "logical-time initial value");
  requireDirectTimeEncoding(
      *time.finalTime,
      "logical-time final value");

  auto directDecodedInitial = time.factory->makeFinal();
  directDecodedInitial->decode(
      initialEncoding.data(),
      initialEncoding.size());
  requireTimeEquals(
      *directDecodedInitial,
      *time.initial,
      "logical-time direct buffer decode");
  if (initialEncoding.size() > 1U) {
    std::vector<rti::Octet> truncatedInitial(
        initialEncoding.begin(),
        initialEncoding.end() - 1);
    requireException(
        [&] {
          static_cast<void>(
              time.factory->decodeLogicalTime(
                  truncatedInitial.data(),
                  truncatedInitial.size()));
        },
        L"CouldNotDecode",
        "truncated logical-time buffer");
  }

  auto decodedZero = time.factory->decodeLogicalTimeInterval(time.zero->encode());
  require(decodedZero != nullptr, "logical-time factory returned no decoded zero interval");
  require(*decodedZero == *time.zero, "logical-time zero interval did not round-trip");
  auto const epsilonEncodedValue = time.epsilon->encode();
  auto decodedEpsilon = time.factory->decodeLogicalTimeInterval(
      epsilonEncodedValue.data(),
      time.epsilon->encodedLength());
  require(decodedEpsilon != nullptr,
          "logical-time factory returned no buffer-decoded epsilon interval");
  require(*decodedEpsilon == *time.epsilon,
          "logical-time epsilon interval did not round-trip");

  requireDirectTimeEncoding(
      *time.zero,
      "logical-time zero interval");
  requireDirectTimeEncoding(
      *time.epsilon,
      "logical-time epsilon interval");

  auto zeroEncoding = copyBytes(time.zero->encode());
  auto directDecodedZero = time.factory->makeEpsilon();
  directDecodedZero->decode(zeroEncoding.data(), zeroEncoding.size());
  require(
      *directDecodedZero == *time.zero,
      "logical-time interval direct buffer decode");
  if (zeroEncoding.size() > 1U) {
    std::vector<rti::Octet> truncatedZero(
        zeroEncoding.begin(),
        zeroEncoding.end() - 1);
    requireException(
        [&] {
          static_cast<void>(
              time.factory->decodeLogicalTimeInterval(
                  truncatedZero.data(),
                  truncatedZero.size()));
        },
        L"CouldNotDecode",
        "truncated logical-time interval buffer");
  }

  auto boundary = time.factory->makeInitial();
  boundary->setFinal();
  require(boundary->isFinal(), "logical-time setFinal did not create final time");
  boundary->setInitial();
  require(boundary->isInitial(), "logical-time setInitial did not restore initial time");
  auto intervalArithmetic = time.factory->makeZero();
  *intervalArithmetic += *time.epsilon;
  require(
      *intervalArithmetic == *time.epsilon,
      "logical-time interval addition did not advance from zero");
  *intervalArithmetic -= *time.epsilon;
  require(
      *intervalArithmetic == *time.zero,
      "logical-time interval subtraction did not return to zero");
  require(
      *time.epsilon > *time.zero && *time.zero < *time.epsilon,
      "logical-time interval ordering did not place epsilon after zero");

  auto first = timeAfter(*time.factory, *time.initial, *time.epsilon, 1U);
  auto second = timeAfter(*time.factory, *time.initial, *time.epsilon, 2U);
  require(*first > *time.initial, "epsilon did not advance logical time");
  require(*second > *first, "logical-time values did not preserve ordering");
  require(*first <= *second && *second >= *first,
          "logical-time comparison operators were inconsistent");
  auto difference = time.factory->makeZero();
  difference->setToDifference(*second, *first);
  require(*difference == *time.epsilon,
          "logical-time difference did not recover one epsilon interval");
  auto copied = time.factory->makeInitial();
  *copied = *second;
  require(*copied == *second, "logical-time assignment did not preserve the value");
  auto underflow = time.factory->makeInitial();
  *underflow -= *time.epsilon;
  require(
      *underflow == *time.initial,
      "logical-time subtraction below initial did not preserve the initial value");
  auto finalBoundary = time.factory->makeFinal();
  *finalBoundary += *time.epsilon;
  require(finalBoundary->isFinal(),
          "logical-time addition to final did not preserve final");
  *finalBoundary -= *time.epsilon;
  require(finalBoundary->isFinal(),
          "logical-time subtraction from final did not preserve final");

  requireException(
      [&] { static_cast<void>(time.factory->decodeLogicalTime(rti::VariableLengthData{})); },
      L"CouldNotDecode",
      "empty logical-time encoding");
  requireException(
      [&] {
        static_cast<void>(
            time.factory->decodeLogicalTimeInterval(rti::VariableLengthData{}));
      },
      L"CouldNotDecode",
      "empty logical-time interval encoding");

  session.resign(rti::NO_ACTION);
  session.rtiAmbassador().destroyFederationExecution(federation);
  session.disconnect();
}


void scenarioLogicalTimePortable(Options const&, rti::CallbackModel) {
  verifyReferenceTimeTypes();
}

void scenarioLogicalTimeIntervalNonnegativeContract(
    Options const&,
    rti::CallbackModel) {
  requireException(
      [] { static_cast<void>(rti::HLAinteger64Interval(-1)); },
      L"InvalidLogicalTimeInterval",
      "negative integer64 logical-time interval construction");
  rti::HLAinteger64Interval integerInterval;
  requireException(
      [&] { integerInterval.setInterval(-1); },
      L"InvalidLogicalTimeInterval",
      "negative integer64 logical-time interval mutation");

  requireException(
      [] { static_cast<void>(rti::HLAfloat64Interval(-1.0)); },
      L"InvalidLogicalTimeInterval",
      "negative float64 logical-time interval construction");
  rti::HLAfloat64Interval floatInterval;
  requireException(
      [&] { floatInterval.setInterval(-1.0); },
      L"InvalidLogicalTimeInterval",
      "negative float64 logical-time interval mutation");
}

void scenarioLogicalTimeFactoryFactoryPortable(
    Options const&,
    rti::CallbackModel) {
  verifyStandardLogicalTimeFactoryFactoryContract();

  auto forwardedIntegerFactory =
      rti::LogicalTimeFactoryFactory::makeLogicalTimeFactory(L"HLAinteger64Time");
  require(
      forwardedIntegerFactory != nullptr &&
          forwardedIntegerFactory->getName() == L"HLAinteger64Time" &&
          dynamic_cast<rti::HLAinteger64TimeFactory*>(
              forwardedIntegerFactory.get()) != nullptr,
      "LogicalTimeFactoryFactory did not forward HLAinteger64Time to its standard factory");

  auto forwardedFloatFactory =
      rti::LogicalTimeFactoryFactory::makeLogicalTimeFactory(L"HLAfloat64Time");
  require(
      forwardedFloatFactory != nullptr &&
          forwardedFloatFactory->getName() == L"HLAfloat64Time" &&
          dynamic_cast<rti::HLAfloat64TimeFactory*>(
              forwardedFloatFactory.get()) != nullptr,
      "LogicalTimeFactoryFactory did not forward HLAfloat64Time to its standard factory");
}


}  // namespace hla_rti_cpp_tck::detail
