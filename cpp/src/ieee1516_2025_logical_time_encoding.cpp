#include <RTI/RTIambassador.h>
#include <RTI/VariableLengthData.h>
#include <RTI/encoding/EncodingExceptions.h>
#include <RTI/encoding/HLAlogicalTime.h>
#include <RTI/encoding/HLAlogicalTimeInterval.h>
#include <RTI/time/LogicalTime.h>
#include <RTI/time/LogicalTimeFactory.h>
#include <RTI/time/LogicalTimeInterval.h>

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

[[noreturn]] void invalidEncoding(wchar_t const* message) {
  throw rti1516_2025::EncoderException(message);
}

void requireValidEncodedData(rti1516_2025::VariableLengthData const& encoded) {
  if (encoded.size() != 0U && encoded.data() == nullptr) {
    invalidEncoding(L"The encoded logical-time value is invalid.");
  }
}

void appendEncodedData(
    std::vector<rti1516_2025::Octet>& buffer,
    rti1516_2025::VariableLengthData const& encoded) {
  requireValidEncodedData(encoded);
  if (encoded.size() == 0U) {
    return;
  }
  auto const* octets = static_cast<rti1516_2025::Octet const*>(encoded.data());
  buffer.insert(buffer.end(), octets, octets + encoded.size());
}

[[nodiscard]] rti1516_2025::VariableLengthData subrange(
    std::vector<rti1516_2025::Octet> const& buffer,
    std::size_t index,
    std::size_t size) {
  if (index > buffer.size() || buffer.size() - index < size) {
    invalidEncoding(L"The logical-time encoding is truncated.");
  }
  if (size == 0U) {
    return rti1516_2025::VariableLengthData{};
  }
  return rti1516_2025::VariableLengthData(buffer.data() + index, size);
}

}  // namespace

namespace rti1516_2025 {

namespace {

constexpr unsigned int kOpaqueOctetBoundary = 1U;

struct LogicalTimeFactoryState final {
  std::shared_ptr<LogicalTimeFactory> factory;
  std::wstring implementationName;
};

[[nodiscard]] LogicalTimeFactoryState factoryStateFor(RTIambassador const* ambassador) {
  if (ambassador == nullptr) {
    invalidEncoding(L"An HLA logical-time encoding helper requires an RTI ambassador.");
  }

  auto uniqueFactory = ambassador->getTimeFactory();
  if (!uniqueFactory) {
    invalidEncoding(L"The RTI ambassador did not provide a logical-time factory.");
  }

  LogicalTimeFactoryState state;
  state.factory = std::shared_ptr<LogicalTimeFactory>(std::move(uniqueFactory));
  state.implementationName = state.factory->getName();
  if (state.implementationName.empty()) {
    invalidEncoding(L"The RTI ambassador provided an unnamed logical-time factory.");
  }
  return state;
}

void requireCompatible(
    LogicalTimeFactoryState const& state,
    LogicalTime const& value) {
  if (value.implementationName() != state.implementationName) {
    invalidEncoding(L"The LogicalTime value does not use this helper's selected time implementation.");
  }
}

void requireCompatible(
    LogicalTimeFactoryState const& state,
    LogicalTimeInterval const& value) {
  if (value.implementationName() != state.implementationName) {
    invalidEncoding(
        L"The LogicalTimeInterval value does not use this helper's selected time implementation.");
  }
}

[[nodiscard]] std::unique_ptr<LogicalTime> requireTime(
    LogicalTimeFactoryState const& state,
    std::unique_ptr<LogicalTime> value) {
  if (!value) {
    invalidEncoding(L"The logical-time factory returned no LogicalTime value.");
  }
  requireCompatible(state, *value);
  return value;
}

[[nodiscard]] std::unique_ptr<LogicalTimeInterval> requireInterval(
    LogicalTimeFactoryState const& state,
    std::unique_ptr<LogicalTimeInterval> value) {
  if (!value) {
    invalidEncoding(L"The logical-time factory returned no LogicalTimeInterval value.");
  }
  requireCompatible(state, *value);
  return value;
}

[[nodiscard]] std::unique_ptr<LogicalTime> initialTime(LogicalTimeFactoryState const& state) {
  return requireTime(state, state.factory->makeInitial());
}

[[nodiscard]] std::unique_ptr<LogicalTimeInterval> zeroInterval(
    LogicalTimeFactoryState const& state) {
  return requireInterval(state, state.factory->makeZero());
}

[[nodiscard]] std::unique_ptr<LogicalTime> copyTime(
    LogicalTimeFactoryState const& state,
    LogicalTime const& source) {
  requireCompatible(state, source);
  auto encoded = source.encode();
  requireValidEncodedData(encoded);
  return requireTime(state, state.factory->decodeLogicalTime(encoded));
}

[[nodiscard]] std::unique_ptr<LogicalTimeInterval> copyInterval(
    LogicalTimeFactoryState const& state,
    LogicalTimeInterval const& source) {
  requireCompatible(state, source);
  auto encoded = source.encode();
  requireValidEncodedData(encoded);
  return requireInterval(state, state.factory->decodeLogicalTimeInterval(encoded));
}

[[nodiscard]] std::size_t encodedLength(LogicalTime const& value) {
  auto const length = value.encodedLength();
  auto encoded = value.encode();
  requireValidEncodedData(encoded);
  if (encoded.size() != length) {
    invalidEncoding(L"The LogicalTime implementation reported an inconsistent encoded length.");
  }
  return length;
}

[[nodiscard]] std::size_t encodedLength(LogicalTimeInterval const& value) {
  auto const length = value.encodedLength();
  auto encoded = value.encode();
  requireValidEncodedData(encoded);
  if (encoded.size() != length) {
    invalidEncoding(L"The LogicalTimeInterval implementation reported an inconsistent encoded length.");
  }
  return length;
}

}  // namespace

class HLAlogicalTimeImplementation {
 public:
  LogicalTimeFactoryState state;
  std::unique_ptr<LogicalTime> value;
};

class HLAlogicalTimeIntervalImplementation {
 public:
  LogicalTimeFactoryState state;
  std::unique_ptr<LogicalTimeInterval> value;
};

HLAlogicalTime::HLAlogicalTime(RTIambassador const* ambassador)
    : _impl(nullptr) {
  auto implementation = std::make_unique<HLAlogicalTimeImplementation>();
  implementation->state = factoryStateFor(ambassador);
  implementation->value = initialTime(implementation->state);
  _impl = implementation.release();
}

HLAlogicalTime::HLAlogicalTime(
    RTIambassador const* ambassador,
    LogicalTime const& logicalTime)
    : _impl(nullptr) {
  auto implementation = std::make_unique<HLAlogicalTimeImplementation>();
  implementation->state = factoryStateFor(ambassador);
  implementation->value = copyTime(implementation->state, logicalTime);
  _impl = implementation.release();
}

HLAlogicalTime::HLAlogicalTime(HLAlogicalTime const& rhs)
    : _impl(nullptr) {
  auto implementation = std::make_unique<HLAlogicalTimeImplementation>();
  implementation->state = rhs._impl->state;
  implementation->value = copyTime(implementation->state, *rhs._impl->value);
  _impl = implementation.release();
}

HLAlogicalTime::~HLAlogicalTime() {
  delete _impl;
}

std::unique_ptr<DataElement> HLAlogicalTime::clone() const {
  return std::make_unique<HLAlogicalTime>(*this);
}

VariableLengthData HLAlogicalTime::encode() const {
  auto encoded = _impl->value->encode();
  requireValidEncodedData(encoded);
  return encoded;
}

void HLAlogicalTime::encode(VariableLengthData& inData) const {
  inData = encode();
}

void HLAlogicalTime::encodeInto(std::vector<Octet>& buffer) const {
  appendEncodedData(buffer, encode());
}

HLAlogicalTime& HLAlogicalTime::decode(VariableLengthData const& inData) {
  requireValidEncodedData(inData);
  _impl->value = requireTime(
      _impl->state,
      _impl->state.factory->decodeLogicalTime(inData));
  return *this;
}

std::size_t HLAlogicalTime::decodeFrom(
    std::vector<Octet> const& buffer,
    std::size_t index) {
  auto const length = encodedLength(*_impl->value);
  auto encoded = subrange(buffer, index, length);
  decode(encoded);
  return index + length;
}

std::size_t HLAlogicalTime::getEncodedLength() const {
  return encodedLength(*_impl->value);
}

unsigned int HLAlogicalTime::getOctetBoundary() const {
  return kOpaqueOctetBoundary;
}

bool HLAlogicalTime::isSameTypeAs(DataElement const& inData) const {
  auto const* other = dynamic_cast<HLAlogicalTime const*>(&inData);
  return other != nullptr &&
         _impl->state.implementationName == other->_impl->state.implementationName;
}

HLAlogicalTime& HLAlogicalTime::set(LogicalTime const& value) {
  _impl->value = copyTime(_impl->state, value);
  return *this;
}

void HLAlogicalTime::get(LogicalTime& time) {
  time = *_impl->value;
}

HLAlogicalTimeInterval::HLAlogicalTimeInterval(RTIambassador const* ambassador)
    : _impl(nullptr) {
  auto implementation = std::make_unique<HLAlogicalTimeIntervalImplementation>();
  implementation->state = factoryStateFor(ambassador);
  implementation->value = zeroInterval(implementation->state);
  _impl = implementation.release();
}

HLAlogicalTimeInterval::HLAlogicalTimeInterval(
    RTIambassador const* ambassador,
    LogicalTimeInterval const& logicalTimeInterval)
    : _impl(nullptr) {
  auto implementation = std::make_unique<HLAlogicalTimeIntervalImplementation>();
  implementation->state = factoryStateFor(ambassador);
  implementation->value = copyInterval(implementation->state, logicalTimeInterval);
  _impl = implementation.release();
}

HLAlogicalTimeInterval::HLAlogicalTimeInterval(HLAlogicalTimeInterval const& rhs)
    : _impl(nullptr) {
  auto implementation = std::make_unique<HLAlogicalTimeIntervalImplementation>();
  implementation->state = rhs._impl->state;
  implementation->value = copyInterval(implementation->state, *rhs._impl->value);
  _impl = implementation.release();
}

HLAlogicalTimeInterval::~HLAlogicalTimeInterval() {
  delete _impl;
}

std::unique_ptr<DataElement> HLAlogicalTimeInterval::clone() const {
  return std::make_unique<HLAlogicalTimeInterval>(*this);
}

VariableLengthData HLAlogicalTimeInterval::encode() const {
  auto encoded = _impl->value->encode();
  requireValidEncodedData(encoded);
  return encoded;
}

void HLAlogicalTimeInterval::encode(VariableLengthData& inData) const {
  inData = encode();
}

void HLAlogicalTimeInterval::encodeInto(std::vector<Octet>& buffer) const {
  appendEncodedData(buffer, encode());
}

HLAlogicalTimeInterval& HLAlogicalTimeInterval::decode(VariableLengthData const& inData) {
  requireValidEncodedData(inData);
  _impl->value = requireInterval(
      _impl->state,
      _impl->state.factory->decodeLogicalTimeInterval(inData));
  return *this;
}

std::size_t HLAlogicalTimeInterval::decodeFrom(
    std::vector<Octet> const& buffer,
    std::size_t index) {
  auto const length = encodedLength(*_impl->value);
  auto encoded = subrange(buffer, index, length);
  decode(encoded);
  return index + length;
}

std::size_t HLAlogicalTimeInterval::getEncodedLength() const {
  return encodedLength(*_impl->value);
}

unsigned int HLAlogicalTimeInterval::getOctetBoundary() const {
  return kOpaqueOctetBoundary;
}

bool HLAlogicalTimeInterval::isSameTypeAs(DataElement const& inData) const {
  auto const* other = dynamic_cast<HLAlogicalTimeInterval const*>(&inData);
  return other != nullptr &&
         _impl->state.implementationName == other->_impl->state.implementationName;
}

HLAlogicalTimeInterval& HLAlogicalTimeInterval::set(LogicalTimeInterval const& value) {
  _impl->value = copyInterval(_impl->state, value);
  return *this;
}

void HLAlogicalTimeInterval::get(LogicalTimeInterval& interval) {
  interval = *_impl->value;
}

}  // namespace rti1516_2025
