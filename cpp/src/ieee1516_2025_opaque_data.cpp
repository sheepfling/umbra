#include <RTI/VariableLengthData.h>
#include <RTI/encoding/EncodingExceptions.h>
#include <RTI/encoding/HLAopaqueData.h>

#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <vector>

namespace {

[[noreturn]] void invalidEncoding(wchar_t const* message) {
  throw rti1516_2025::EncoderException(message);
}

void appendUint32BE(std::vector<rti1516_2025::Octet>& output, std::uint32_t value) {
  output.push_back(static_cast<rti1516_2025::Octet>((value >> 24U) & 0xffU));
  output.push_back(static_cast<rti1516_2025::Octet>((value >> 16U) & 0xffU));
  output.push_back(static_cast<rti1516_2025::Octet>((value >> 8U) & 0xffU));
  output.push_back(static_cast<rti1516_2025::Octet>(value & 0xffU));
}

[[nodiscard]] std::uint32_t readUint32BE(
    std::vector<rti1516_2025::Octet> const& input,
    std::size_t index) {
  if (index > input.size() || input.size() - index < 4U) {
    invalidEncoding(L"The HLAopaqueData encoding is truncated before its element count.");
  }
  return (static_cast<std::uint32_t>(static_cast<std::uint8_t>(input[index])) << 24U) |
      (static_cast<std::uint32_t>(static_cast<std::uint8_t>(input[index + 1U])) << 16U) |
      (static_cast<std::uint32_t>(static_cast<std::uint8_t>(input[index + 2U])) << 8U) |
      static_cast<std::uint32_t>(static_cast<std::uint8_t>(input[index + 3U]));
}

[[nodiscard]] std::vector<rti1516_2025::Octet> toOctets(
    rti1516_2025::VariableLengthData const& input) {
  auto const* bytes = static_cast<rti1516_2025::Octet const*>(input.data());
  if (input.size() != 0U && bytes == nullptr) {
    invalidEncoding(L"The encoded HLAopaqueData buffer is invalid.");
  }
  return bytes == nullptr ? std::vector<rti1516_2025::Octet>{}
                          : std::vector<rti1516_2025::Octet>(bytes, bytes + input.size());
}

void validateElementCount(std::size_t dataLength) {
  if (dataLength > static_cast<std::size_t>(std::numeric_limits<rti1516_2025::Integer32>::max())) {
    invalidEncoding(L"The HLAopaqueData value is too large to encode.");
  }
}

}  // namespace

namespace rti1516_2025 {

class HLAopaqueDataImplementation {
 public:
  void setInternalData(Octet const* source, std::size_t sourceLength) {
    if (sourceLength != 0U && source == nullptr) {
      invalidEncoding(L"A nonempty HLAopaqueData value requires a data pointer.");
    }
    externalDataPointer = nullptr;
    externalBufferLength = 0U;
    externalDataLength = 0U;
    internalData.clear();
    if (sourceLength != 0U) {
      internalData.assign(source, source + sourceLength);
    }
  }

  void setExternalData(
      Octet** source,
      std::size_t bufferLength,
      std::size_t dataLength) {
    if (source == nullptr || *source == nullptr || bufferLength == 0U) {
      invalidEncoding(L"HLAopaqueData external memory must be non-null with a nonzero buffer length.");
    }
    if (dataLength > bufferLength) {
      invalidEncoding(L"The HLAopaqueData external data length exceeds its buffer length.");
    }
    internalData.clear();
    externalDataPointer = source;
    externalBufferLength = bufferLength;
    externalDataLength = dataLength;
  }

  void setData(Octet const* source, std::size_t sourceLength) {
    if (sourceLength != 0U && source == nullptr) {
      invalidEncoding(L"A nonempty HLAopaqueData value requires a data pointer.");
    }
    if (externalDataPointer == nullptr) {
      setInternalData(source, sourceLength);
      return;
    }
    if (*externalDataPointer == nullptr) {
      invalidEncoding(L"The HLAopaqueData external data pointer is no longer valid.");
    }
    if (sourceLength > externalBufferLength) {
      invalidEncoding(L"The HLAopaqueData value does not fit in the supplied external buffer.");
    }
    if (sourceLength != 0U) {
      std::memmove(*externalDataPointer, source, sourceLength);
    }
    externalDataLength = sourceLength;
  }

  [[nodiscard]] Octet const* data() const noexcept {
    if (externalDataPointer != nullptr) {
      return *externalDataPointer;
    }
    return internalData.empty() ? nullptr : internalData.data();
  }

  [[nodiscard]] std::size_t bufferLength() const noexcept {
    return externalDataPointer == nullptr ? internalData.size() : externalBufferLength;
  }

  [[nodiscard]] std::size_t dataLength() const noexcept {
    return externalDataPointer == nullptr ? internalData.size() : externalDataLength;
  }

 private:
  std::vector<Octet> internalData;
  Octet** externalDataPointer = nullptr;
  std::size_t externalBufferLength = 0U;
  std::size_t externalDataLength = 0U;
};

HLAopaqueData::HLAopaqueData() : _impl(new HLAopaqueDataImplementation()) {}

HLAopaqueData::HLAopaqueData(Octet const* inData, std::size_t dataSize)
    : _impl(new HLAopaqueDataImplementation()) {
  _impl->setInternalData(inData, dataSize);
}

HLAopaqueData::HLAopaqueData(Octet** inData, std::size_t bufferSize, std::size_t dataSize)
    : _impl(new HLAopaqueDataImplementation()) {
  _impl->setExternalData(inData, bufferSize, dataSize);
}

HLAopaqueData::HLAopaqueData(HLAopaqueData const& rhs)
    : _impl(new HLAopaqueDataImplementation()) {
  _impl->setInternalData(rhs.get(), rhs.dataLength());
}

HLAopaqueData::~HLAopaqueData() {
  delete _impl;
}

std::unique_ptr<DataElement> HLAopaqueData::clone() const {
  return std::make_unique<HLAopaqueData>(*this);
}

VariableLengthData HLAopaqueData::encode() const {
  VariableLengthData value;
  encode(value);
  return value;
}

void HLAopaqueData::encode(VariableLengthData& value) const {
  std::vector<Octet> bytes;
  encodeInto(bytes);
  value.setData(bytes.data(), bytes.size());
}

void HLAopaqueData::encodeInto(std::vector<Octet>& bytes) const {
  auto const length = _impl->dataLength();
  validateElementCount(length);
  appendUint32BE(bytes, static_cast<std::uint32_t>(length));
  if (length == 0U) {
    return;
  }
  auto const* data = _impl->data();
  if (data == nullptr) {
    invalidEncoding(L"The HLAopaqueData data pointer is invalid.");
  }
  bytes.insert(bytes.end(), data, data + length);
}

HLAopaqueData& HLAopaqueData::decode(VariableLengthData const& value) {
  auto const bytes = toOctets(value);
  auto const nextIndex = decodeFrom(bytes, 0U);
  if (nextIndex != bytes.size()) {
    invalidEncoding(L"The HLAopaqueData encoding has trailing data.");
  }
  return *this;
}

std::size_t HLAopaqueData::decodeFrom(
    std::vector<Octet> const& bytes,
    std::size_t index) {
  auto const elementCount = readUint32BE(bytes, index);
  if (elementCount > static_cast<std::uint32_t>(std::numeric_limits<Integer32>::max())) {
    invalidEncoding(L"The HLAopaqueData encoding has an invalid element count.");
  }
  auto const payloadOffset = index + 4U;
  auto const payloadLength = static_cast<std::size_t>(elementCount);
  if (payloadOffset > bytes.size() || bytes.size() - payloadOffset < payloadLength) {
    invalidEncoding(L"The HLAopaqueData encoding is truncated.");
  }
  _impl->setData(payloadLength == 0U ? nullptr : bytes.data() + payloadOffset, payloadLength);
  return payloadOffset + payloadLength;
}

std::size_t HLAopaqueData::getEncodedLength() const {
  auto const length = _impl->dataLength();
  validateElementCount(length);
  return 4U + length;
}

unsigned int HLAopaqueData::getOctetBoundary() const {
  return 4U;
}

std::size_t HLAopaqueData::bufferLength() const {
  return _impl->bufferLength();
}

std::size_t HLAopaqueData::dataLength() const {
  return _impl->dataLength();
}

HLAopaqueData& HLAopaqueData::setDataPointer(
    Octet** inData,
    std::size_t bufferSize,
    std::size_t dataSize) {
  _impl->setExternalData(inData, bufferSize, dataSize);
  return *this;
}

HLAopaqueData& HLAopaqueData::set(Octet const* inData, std::size_t dataSize) {
  _impl->setData(inData, dataSize);
  return *this;
}

Octet const* HLAopaqueData::get() const {
  return _impl->data();
}

}  // namespace rti1516_2025
