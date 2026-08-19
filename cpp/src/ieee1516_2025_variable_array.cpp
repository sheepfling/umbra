#include <RTI/VariableLengthData.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/EncodingExceptions.h>
#include <RTI/encoding/HLAvariableArray.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

namespace {

[[noreturn]] void invalidEncoding(wchar_t const* message) {
  throw rti1516_2025::EncoderException(message);
}

[[nodiscard]] std::vector<rti1516_2025::Octet> toOctets(
    rti1516_2025::VariableLengthData const& input) {
  auto const* bytes = static_cast<rti1516_2025::Octet const*>(input.data());
  if (input.size() != 0U && bytes == nullptr) {
    invalidEncoding(L"The encoded HLAvariableArray buffer is invalid.");
  }
  return bytes == nullptr ? std::vector<rti1516_2025::Octet>{}
                          : std::vector<rti1516_2025::Octet>(bytes, bytes + input.size());
}

[[nodiscard]] std::size_t checkedAdd(std::size_t left, std::size_t right) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    invalidEncoding(L"The HLAvariableArray encoded length overflows size_t.");
  }
  return left + right;
}

[[nodiscard]] std::size_t paddingToBoundary(
    std::size_t encodedLength,
    unsigned int boundary) {
  if (boundary == 0U) {
    invalidEncoding(L"An HLAvariableArray component has an invalid octet boundary.");
  }
  auto const divisor = static_cast<std::size_t>(boundary);
  auto const remainder = encodedLength % divisor;
  return remainder == 0U ? 0U : divisor - remainder;
}

void appendZeroPadding(std::vector<rti1516_2025::Octet>& bytes, std::size_t count) {
  bytes.insert(bytes.end(), count, static_cast<rti1516_2025::Octet>(0));
}

void requireZeroPadding(
    std::vector<rti1516_2025::Octet> const& bytes,
    std::size_t index,
    std::size_t count) {
  if (index > bytes.size() || bytes.size() - index < count) {
    invalidEncoding(L"The HLAvariableArray encoding is truncated in its padding.");
  }
  for (std::size_t offset = 0U; offset < count; ++offset) {
    if (static_cast<std::uint8_t>(bytes[index + offset]) != 0U) {
      invalidEncoding(L"The HLAvariableArray encoding contains nonzero padding.");
    }
  }
}

}  // namespace

namespace rti1516_2025 {

class HLAvariableArrayImplementation {
 public:
  class ElementSlot {
   public:
    ElementSlot() = default;

    std::unique_ptr<DataElement> owned;
    DataElement* element = nullptr;
  };

  std::unique_ptr<DataElement> prototype;
  std::vector<ElementSlot> elements;
};

namespace {

[[nodiscard]] std::unique_ptr<DataElement> cloneElement(DataElement const& element) {
  auto clone = element.clone();
  if (!clone) {
    invalidEncoding(L"An HLAvariableArray element clone is invalid.");
  }
  return clone;
}

[[nodiscard]] DataElement const& prototypeOf(HLAvariableArrayImplementation const& implementation) {
  if (!implementation.prototype) {
    invalidEncoding(L"The HLAvariableArray prototype is invalid.");
  }
  return *implementation.prototype;
}

[[nodiscard]] unsigned int prototypeBoundary(HLAvariableArrayImplementation const& implementation) {
  auto const boundary = prototypeOf(implementation).getOctetBoundary();
  if (boundary == 0U) {
    invalidEncoding(L"An HLAvariableArray element has an invalid octet boundary.");
  }
  return boundary;
}

[[nodiscard]] unsigned int arrayBoundary(HLAvariableArrayImplementation const& implementation) {
  return std::max(4U, prototypeBoundary(implementation));
}

[[nodiscard]] HLAvariableArrayImplementation::ElementSlot& slotAt(
    HLAvariableArrayImplementation& implementation,
    std::size_t index) {
  if (index >= implementation.elements.size()) {
    invalidEncoding(L"The HLAvariableArray element index is invalid.");
  }
  return implementation.elements[index];
}

[[nodiscard]] HLAvariableArrayImplementation::ElementSlot const& slotAt(
    HLAvariableArrayImplementation const& implementation,
    std::size_t index) {
  if (index >= implementation.elements.size() || implementation.elements[index].element == nullptr) {
    invalidEncoding(L"The HLAvariableArray element index is invalid.");
  }
  return implementation.elements[index];
}

void assignOwned(
    HLAvariableArrayImplementation::ElementSlot& slot,
    std::unique_ptr<DataElement> element) {
  slot.owned = std::move(element);
  slot.element = slot.owned.get();
}

[[nodiscard]] DataElement& elementAt(
    HLAvariableArrayImplementation& implementation,
    std::size_t index) {
  auto& slot = slotAt(implementation, index);
  if (slot.element == nullptr) {
    assignOwned(slot, cloneElement(prototypeOf(implementation)));
  }
  return *slot.element;
}

[[nodiscard]] DataElement const& elementAt(
    HLAvariableArrayImplementation const& implementation,
    std::size_t index) {
  return *slotAt(implementation, index).element;
}

void requirePrototypeType(
    HLAvariableArrayImplementation const& implementation,
    DataElement const& element) {
  if (!prototypeOf(implementation).isSameTypeAs(element)) {
    invalidEncoding(L"The HLAvariableArray element type does not match its prototype.");
  }
}

void requireEncodableCount(std::size_t count) {
  if (count > static_cast<std::size_t>(std::numeric_limits<Integer32>::max())) {
    invalidEncoding(L"The HLAvariableArray element count exceeds HLAinteger32BE.");
  }
}

void resizeForDecodedCount(HLAvariableArrayImplementation& implementation, std::size_t count) {
  if (count > implementation.elements.max_size()) {
    invalidEncoding(L"The decoded HLAvariableArray element count is too large.");
  }
  implementation.elements.resize(count);
  for (std::size_t index = 0U; index < count; ++index) {
    static_cast<void>(elementAt(implementation, index));
  }
}

}  // namespace

HLAvariableArray::HLAvariableArray(DataElement const& prototype) : _impl(nullptr) {
  auto implementation = std::make_unique<HLAvariableArrayImplementation>();
  implementation->prototype = cloneElement(prototype);
  _impl = implementation.release();
}

HLAvariableArray::HLAvariableArray(HLAvariableArray const& rhs) : _impl(nullptr) {
  auto implementation = std::make_unique<HLAvariableArrayImplementation>();
  implementation->prototype = cloneElement(prototypeOf(*rhs._impl));
  implementation->elements.resize(rhs._impl->elements.size());
  for (std::size_t index = 0U; index < rhs._impl->elements.size(); ++index) {
    assignOwned(implementation->elements[index], cloneElement(elementAt(*rhs._impl, index)));
  }
  _impl = implementation.release();
}

HLAvariableArray::~HLAvariableArray() {
  delete _impl;
}

std::unique_ptr<DataElement> HLAvariableArray::clone() const {
  return std::make_unique<HLAvariableArray>(*this);
}

VariableLengthData HLAvariableArray::encode() const {
  VariableLengthData value;
  encode(value);
  return value;
}

void HLAvariableArray::encode(VariableLengthData& value) const {
  std::vector<Octet> bytes;
  encodeInto(bytes);
  value.setData(bytes.data(), bytes.size());
}

void HLAvariableArray::encodeInto(std::vector<Octet>& bytes) const {
  requireEncodableCount(_impl->elements.size());
  HLAinteger32BE count{static_cast<Integer32>(_impl->elements.size())};
  count.encodeInto(bytes);
  if (_impl->elements.empty()) {
    return;
  }

  appendZeroPadding(bytes, paddingToBoundary(4U, arrayBoundary(*_impl)));
  auto const elementBoundary = prototypeBoundary(*_impl);
  for (std::size_t index = 0U; index < _impl->elements.size(); ++index) {
    auto const start = bytes.size();
    elementAt(*_impl, index).encodeInto(bytes);
    if (bytes.size() < start) {
      invalidEncoding(L"An HLAvariableArray element produced an invalid encoded length.");
    }
    if (index + 1U != _impl->elements.size()) {
      appendZeroPadding(bytes, paddingToBoundary(bytes.size() - start, elementBoundary));
    }
  }
}

HLAvariableArray& HLAvariableArray::decode(VariableLengthData const& value) {
  auto const bytes = toOctets(value);
  auto const nextIndex = decodeFrom(bytes, 0U);
  if (nextIndex != bytes.size()) {
    invalidEncoding(L"The HLAvariableArray encoding has trailing data.");
  }
  return *this;
}

std::size_t HLAvariableArray::decodeFrom(
    std::vector<Octet> const& bytes,
    std::size_t index) {
  HLAinteger32BE count;
  auto cursor = count.decodeFrom(bytes, index);
  if (count.get() < 0) {
    invalidEncoding(L"The HLAvariableArray element count is negative.");
  }
  auto const decodedCount = static_cast<std::size_t>(count.get());
  resizeForDecodedCount(*_impl, decodedCount);
  if (decodedCount == 0U) {
    return cursor;
  }

  auto const leadingPadding = paddingToBoundary(4U, arrayBoundary(*_impl));
  requireZeroPadding(bytes, cursor, leadingPadding);
  cursor = checkedAdd(cursor, leadingPadding);

  auto const elementBoundary = prototypeBoundary(*_impl);
  for (std::size_t elementIndex = 0U; elementIndex < decodedCount; ++elementIndex) {
    auto const elementStart = cursor;
    cursor = elementAt(*_impl, elementIndex).decodeFrom(bytes, cursor);
    if (cursor < elementStart || cursor > bytes.size()) {
      invalidEncoding(L"An HLAvariableArray element returned an invalid decoded length.");
    }
    if (elementIndex + 1U == decodedCount) {
      continue;
    }
    auto const padding = paddingToBoundary(cursor - elementStart, elementBoundary);
    requireZeroPadding(bytes, cursor, padding);
    cursor = checkedAdd(cursor, padding);
  }
  return cursor;
}

std::size_t HLAvariableArray::getEncodedLength() const {
  requireEncodableCount(_impl->elements.size());
  std::size_t length = 4U;
  if (_impl->elements.empty()) {
    return length;
  }

  length = checkedAdd(length, paddingToBoundary(4U, arrayBoundary(*_impl)));
  auto const elementBoundary = prototypeBoundary(*_impl);
  for (std::size_t index = 0U; index < _impl->elements.size(); ++index) {
    auto const elementLength = elementAt(*_impl, index).getEncodedLength();
    length = checkedAdd(length, elementLength);
    if (index + 1U != _impl->elements.size()) {
      length = checkedAdd(length, paddingToBoundary(elementLength, elementBoundary));
    }
  }
  return length;
}

unsigned int HLAvariableArray::getOctetBoundary() const {
  return arrayBoundary(*_impl);
}

std::size_t HLAvariableArray::size() const {
  return _impl->elements.size();
}

bool HLAvariableArray::isSameTypeAs(DataElement const& inData) const {
  auto const* other = dynamic_cast<HLAvariableArray const*>(&inData);
  return other != nullptr && hasPrototypeSameTypeAs(prototypeOf(*other->_impl));
}

bool HLAvariableArray::hasPrototypeSameTypeAs(DataElement const& dataElement) const {
  return prototypeOf(*_impl).isSameTypeAs(dataElement);
}

HLAvariableArray& HLAvariableArray::addElement(DataElement const& dataElement) {
  requirePrototypeType(*_impl, dataElement);
  _impl->elements.emplace_back();
  assignOwned(_impl->elements.back(), cloneElement(dataElement));
  return *this;
}

HLAvariableArray& HLAvariableArray::addElementPointer(DataElement* dataElement) {
  if (dataElement == nullptr) {
    invalidEncoding(L"An HLAvariableArray element pointer must be non-null.");
  }
  requirePrototypeType(*_impl, *dataElement);
  _impl->elements.emplace_back();
  // The official header assigns element-pointer lifetime responsibility to
  // the caller, so this slot deliberately borrows rather than deletes it.
  _impl->elements.back().element = dataElement;
  return *this;
}

HLAvariableArray& HLAvariableArray::set(std::size_t index, DataElement const& dataElement) {
  auto& slot = slotAt(*_impl, index);
  requirePrototypeType(*_impl, dataElement);
  if (slot.element == nullptr) {
    assignOwned(slot, cloneElement(dataElement));
    return *this;
  }
  slot.element->decode(dataElement.encode());
  return *this;
}

HLAvariableArray& HLAvariableArray::setElementPointer(
    std::size_t index,
    DataElement* dataElement) {
  if (dataElement == nullptr) {
    invalidEncoding(L"An HLAvariableArray element pointer must be non-null.");
  }
  auto& slot = slotAt(*_impl, index);
  requirePrototypeType(*_impl, *dataElement);
  if (slot.element != dataElement) {
    slot.owned.reset();
    slot.element = dataElement;
  }
  return *this;
}

DataElement const& HLAvariableArray::get(std::size_t index) const {
  return elementAt(*_impl, index);
}

DataElement const& HLAvariableArray::operator[](std::size_t index) const {
  return get(index);
}

}  // namespace rti1516_2025
