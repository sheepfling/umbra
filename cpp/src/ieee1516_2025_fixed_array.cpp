#include <RTI/VariableLengthData.h>
#include <RTI/encoding/EncodingExceptions.h>
#include <RTI/encoding/HLAfixedArray.h>

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
    invalidEncoding(L"The encoded HLAfixedArray buffer is invalid.");
  }
  return bytes == nullptr ? std::vector<rti1516_2025::Octet>{}
                          : std::vector<rti1516_2025::Octet>(bytes, bytes + input.size());
}

[[nodiscard]] std::size_t checkedAdd(std::size_t left, std::size_t right) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    invalidEncoding(L"The HLAfixedArray encoded length overflows size_t.");
  }
  return left + right;
}

[[nodiscard]] std::size_t paddingAfterElement(
    std::size_t elementLength,
    unsigned int elementBoundary) {
  if (elementBoundary == 0U) {
    invalidEncoding(L"An HLAfixedArray element has an invalid octet boundary.");
  }
  auto const boundary = static_cast<std::size_t>(elementBoundary);
  auto const remainder = elementLength % boundary;
  return remainder == 0U ? 0U : boundary - remainder;
}

void appendZeroPadding(std::vector<rti1516_2025::Octet>& bytes, std::size_t count) {
  bytes.insert(bytes.end(), count, static_cast<rti1516_2025::Octet>(0));
}

void requireZeroPadding(
    std::vector<rti1516_2025::Octet> const& bytes,
    std::size_t index,
    std::size_t count) {
  if (index > bytes.size() || bytes.size() - index < count) {
    invalidEncoding(L"The HLAfixedArray encoding is truncated in its padding.");
  }
  for (std::size_t offset = 0U; offset < count; ++offset) {
    if (static_cast<std::uint8_t>(bytes[index + offset]) != 0U) {
      invalidEncoding(L"The HLAfixedArray encoding contains nonzero padding.");
    }
  }
}

}  // namespace

namespace rti1516_2025 {

class HLAfixedArrayImplementation {
 public:
  class ElementSlot {
   public:
    ElementSlot() = default;

    std::unique_ptr<DataElement> owned;
    DataElement* element = nullptr;
  };

  std::unique_ptr<DataElement> prototype;
  mutable std::vector<ElementSlot> elements;
};

namespace {

[[nodiscard]] std::unique_ptr<DataElement> cloneElement(DataElement const& element) {
  auto clone = element.clone();
  if (!clone) {
    invalidEncoding(L"An HLAfixedArray element clone is invalid.");
  }
  return clone;
}

[[nodiscard]] DataElement const& prototypeOf(HLAfixedArrayImplementation const& implementation) {
  if (!implementation.prototype) {
    invalidEncoding(L"The HLAfixedArray prototype is invalid.");
  }
  return *implementation.prototype;
}

[[nodiscard]] unsigned int prototypeBoundary(HLAfixedArrayImplementation const& implementation) {
  auto const boundary = prototypeOf(implementation).getOctetBoundary();
  if (boundary == 0U) {
    invalidEncoding(L"An HLAfixedArray element has an invalid octet boundary.");
  }
  return boundary;
}

[[nodiscard]] HLAfixedArrayImplementation::ElementSlot& slotAt(
    HLAfixedArrayImplementation const& implementation,
    std::size_t index) {
  if (index >= implementation.elements.size()) {
    invalidEncoding(L"The HLAfixedArray element index is invalid.");
  }
  return implementation.elements[index];
}

void assignOwned(
    HLAfixedArrayImplementation::ElementSlot& slot,
    std::unique_ptr<DataElement> element) {
  slot.owned = std::move(element);
  slot.element = slot.owned.get();
}

[[nodiscard]] DataElement& materializeElement(
    HLAfixedArrayImplementation const& implementation,
    std::size_t index) {
  auto& slot = slotAt(implementation, index);
  if (slot.element == nullptr) {
    assignOwned(slot, cloneElement(prototypeOf(implementation)));
  }
  return *slot.element;
}

void requirePrototypeType(
    HLAfixedArrayImplementation const& implementation,
    DataElement const& element) {
  if (!prototypeOf(implementation).isSameTypeAs(element)) {
    invalidEncoding(L"The HLAfixedArray element type does not match its prototype.");
  }
}

}  // namespace

HLAfixedArray::HLAfixedArray(DataElement const& prototype, std::size_t length) : _impl(nullptr) {
  auto implementation = std::make_unique<HLAfixedArrayImplementation>();
  implementation->prototype = cloneElement(prototype);
  implementation->elements.resize(length);
  _impl = implementation.release();
}

HLAfixedArray::HLAfixedArray(HLAfixedArray const& rhs) : _impl(nullptr) {
  auto implementation = std::make_unique<HLAfixedArrayImplementation>();
  implementation->prototype = cloneElement(prototypeOf(*rhs._impl));
  implementation->elements.resize(rhs._impl->elements.size());
  for (std::size_t index = 0U; index < rhs._impl->elements.size(); ++index) {
    auto const* source = rhs._impl->elements[index].element;
    if (source != nullptr) {
      assignOwned(implementation->elements[index], cloneElement(*source));
    }
  }
  _impl = implementation.release();
}

HLAfixedArray::~HLAfixedArray() {
  delete _impl;
}

std::unique_ptr<DataElement> HLAfixedArray::clone() const {
  return std::make_unique<HLAfixedArray>(*this);
}

VariableLengthData HLAfixedArray::encode() const {
  VariableLengthData value;
  encode(value);
  return value;
}

void HLAfixedArray::encode(VariableLengthData& value) const {
  std::vector<Octet> bytes;
  encodeInto(bytes);
  value.setData(bytes.data(), bytes.size());
}

void HLAfixedArray::encodeInto(std::vector<Octet>& bytes) const {
  auto const boundary = prototypeBoundary(*_impl);
  for (std::size_t index = 0U; index < _impl->elements.size(); ++index) {
    auto const start = bytes.size();
    materializeElement(*_impl, index).encodeInto(bytes);
    if (bytes.size() < start) {
      invalidEncoding(L"An HLAfixedArray element produced an invalid encoded length.");
    }
    if (index + 1U == _impl->elements.size()) {
      continue;
    }
    appendZeroPadding(bytes, paddingAfterElement(bytes.size() - start, boundary));
  }
}

HLAfixedArray& HLAfixedArray::decode(VariableLengthData const& value) {
  auto const bytes = toOctets(value);
  auto const nextIndex = decodeFrom(bytes, 0U);
  if (nextIndex != bytes.size()) {
    invalidEncoding(L"The HLAfixedArray encoding has trailing data.");
  }
  return *this;
}

std::size_t HLAfixedArray::decodeFrom(
    std::vector<Octet> const& bytes,
    std::size_t index) {
  if (index > bytes.size()) {
    invalidEncoding(L"The HLAfixedArray decode index is invalid.");
  }
  auto const boundary = prototypeBoundary(*_impl);
  auto cursor = index;
  for (std::size_t elementIndex = 0U; elementIndex < _impl->elements.size(); ++elementIndex) {
    auto const elementStart = cursor;
    cursor = materializeElement(*_impl, elementIndex).decodeFrom(bytes, cursor);
    if (cursor < elementStart || cursor > bytes.size()) {
      invalidEncoding(L"An HLAfixedArray element returned an invalid decoded length.");
    }
    if (elementIndex + 1U == _impl->elements.size()) {
      continue;
    }
    auto const padding = paddingAfterElement(cursor - elementStart, boundary);
    requireZeroPadding(bytes, cursor, padding);
    cursor = checkedAdd(cursor, padding);
  }
  return cursor;
}

std::size_t HLAfixedArray::getEncodedLength() const {
  auto const boundary = prototypeBoundary(*_impl);
  std::size_t length = 0U;
  for (std::size_t index = 0U; index < _impl->elements.size(); ++index) {
    auto const elementLength = materializeElement(*_impl, index).getEncodedLength();
    length = checkedAdd(length, elementLength);
    if (index + 1U != _impl->elements.size()) {
      length = checkedAdd(length, paddingAfterElement(elementLength, boundary));
    }
  }
  return length;
}

unsigned int HLAfixedArray::getOctetBoundary() const {
  return prototypeBoundary(*_impl);
}

bool HLAfixedArray::isSameTypeAs(DataElement const& inData) const {
  auto const* other = dynamic_cast<HLAfixedArray const*>(&inData);
  return other != nullptr && size() == other->size() &&
         hasPrototypeSameTypeAs(prototypeOf(*other->_impl));
}

bool HLAfixedArray::hasPrototypeSameTypeAs(DataElement const& dataElement) const {
  return prototypeOf(*_impl).isSameTypeAs(dataElement);
}

std::size_t HLAfixedArray::size() const {
  return _impl->elements.size();
}

HLAfixedArray& HLAfixedArray::set(std::size_t index, DataElement const& dataElement) {
  auto& slot = slotAt(*_impl, index);
  requirePrototypeType(*_impl, dataElement);
  if (slot.element == nullptr) {
    assignOwned(slot, cloneElement(dataElement));
    return *this;
  }
  slot.element->decode(dataElement.encode());
  return *this;
}

HLAfixedArray& HLAfixedArray::setElementPointer(std::size_t index, DataElement* dataElement) {
  if (dataElement == nullptr) {
    invalidEncoding(L"An HLAfixedArray element pointer must be non-null.");
  }
  auto& slot = slotAt(*_impl, index);
  requirePrototypeType(*_impl, *dataElement);
  if (slot.element != dataElement) {
    // The official header assigns element-pointer lifetime responsibility to
    // the caller, so this slot deliberately borrows rather than deletes it.
    slot.owned.reset();
    slot.element = dataElement;
  }
  return *this;
}

DataElement const& HLAfixedArray::get(std::size_t index) const {
  return materializeElement(*_impl, index);
}

DataElement const& HLAfixedArray::operator[](std::size_t index) const {
  return get(index);
}

}  // namespace rti1516_2025
