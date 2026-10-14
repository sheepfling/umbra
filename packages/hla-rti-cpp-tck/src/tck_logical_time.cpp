#include "tck_logical_time.hpp"

namespace hla_rti_cpp_tck::detail {

namespace {

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

void verifyReferenceTimeTypes() {
  rti::HLAinteger64Time integerTime(41);
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
  requireException(
      [&] { integerInitial -= integerEpsilon; },
      L"IllegalTimeArithmetic",
      "HLAinteger64Time subtraction before initial");
  requireException(
      [&] { integerFinal += integerEpsilon; },
      L"IllegalTimeArithmetic",
      "HLAinteger64Time addition after final");
  requireException(
      [&] { static_cast<void>(rti::HLAinteger64Time(-1)); },
      L"InvalidLogicalTime",
      "negative HLAinteger64Time construction");

  rti::HLAinteger64TimeFactory integerFactory;
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

  rti::HLAfloat64Time floatInitial;
  rti::HLAfloat64Time floatFinal;
  rti::HLAfloat64Interval floatEpsilon;
  floatFinal.setFinal();
  floatEpsilon.setEpsilon();
  constexpr double floatFinalValue = std::numeric_limits<double>::max();
  constexpr double floatEpsilonValue = std::numeric_limits<double>::denorm_min();
  require(
      floatInitial.isInitial() && floatFinal.isFinal() &&
          floatFinal.getTime() == floatFinalValue && floatEpsilon.isEpsilon() &&
          floatEpsilon.getInterval() == floatEpsilonValue &&
          floatFinal.implementationName() == L"HLAfloat64Time",
      "HLAfloat64Time did not preserve its standard boundaries and epsilon");
  rti::HLAfloat64Time floatLarge(floatFinalValue / 2.0);
  auto const floatBefore = floatLarge.getTime();
  floatLarge += floatEpsilon;
  require(
      floatLarge.getTime() == std::nextafter(floatBefore, floatFinalValue),
      "HLAfloat64Time epsilon did not advance by one representable step");
  floatLarge -= floatEpsilon;
  require(
      floatLarge.getTime() == floatBefore,
      "HLAfloat64Time epsilon subtraction did not restore the value");

  rti::HLAfloat64Time floatValue(1.5);
  require(
      copyBytes(floatValue.encode()) ==
          std::vector<std::uint8_t>{0x3FU, 0xF8U, 0U, 0U, 0U, 0U, 0U, 0U},
      "HLAfloat64Time did not use HLAfloat64BE encoding");
  requireDirectTimeEncoding(floatValue, "HLAfloat64Time");
  rti::HLAfloat64Time floatDecoded;
  floatDecoded.decode(floatValue.encode());
  require(
      floatDecoded.getTime() == 1.5,
      "HLAfloat64Time did not decode its own encoding");
  rti::HLAfloat64Time floatCopied(floatValue);
  rti::HLAfloat64Time floatAssigned;
  floatAssigned = floatValue;
  floatValue.setTime(2.5);
  require(
      floatCopied.getTime() == 1.5 && floatAssigned.getTime() == 1.5,
      "HLAfloat64Time copy or assignment was not independent");
  floatValue.setTime(1.5);

  rti::HLAfloat64Interval floatInterval(2.5);
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
      [&] { floatFinal += floatEpsilon; },
      L"IllegalTimeArithmetic",
      "HLAfloat64Time addition after final");
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
  requireException(
      [&] { *underflow -= *time.epsilon; },
      L"IllegalTimeArithmetic",
      "logical-time subtraction before initial");
  auto overflow = time.factory->makeFinal();
  requireException(
      [&] { *overflow += *time.epsilon; },
      L"IllegalTimeArithmetic",
      "logical-time addition after final");

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

void scenarioLogicalTimeFactoryFactoryPortable(
    Options const&,
    rti::CallbackModel) {
  verifyStandardLogicalTimeFactoryFactoryContract();
}


}  // namespace hla_rti_cpp_tck::detail
