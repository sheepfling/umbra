#include <RTI/VariableLengthData.h>
#include <RTI/encoding/EncodingExceptions.h>
#include <RTI/encoding/HLAfixedRecord.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

namespace {

[[noreturn]] void invalidEncoding(wchar_t const* message) {
  throw rti1516e::EncoderException(message);
}

[[nodiscard]] std::vector<rti1516e::Octet> toOctets(
    rti1516e::VariableLengthData const& input) {
  auto const* bytes = static_cast<rti1516e::Octet const*>(input.data());
  if (input.size() != 0U && bytes == nullptr) {
    invalidEncoding(L"The encoded HLAfixedRecord buffer is invalid.");
  }
  return bytes == nullptr ? std::vector<rti1516e::Octet>{}
                          : std::vector<rti1516e::Octet>(bytes, bytes + input.size());
}

[[nodiscard]] std::size_t checkedAdd(std::size_t left, std::size_t right) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    invalidEncoding(L"The HLAfixedRecord encoded length overflows size_t.");
  }
  return left + right;
}

[[nodiscard]] std::size_t paddingAfter(
    std::size_t componentOffset,
    std::size_t componentLength,
    unsigned int nextBoundary) {
  if (nextBoundary == 0U) {
    invalidEncoding(L"An HLAfixedRecord component has an invalid octet boundary.");
  }
  auto const nextOffset = checkedAdd(componentOffset, componentLength);
  auto const remainder = nextOffset % static_cast<std::size_t>(nextBoundary);
  return remainder == 0U ? 0U : static_cast<std::size_t>(nextBoundary) - remainder;
}

void appendZeroPadding(std::vector<rti1516e::Octet>& bytes, std::size_t count) {
  bytes.insert(bytes.end(), count, static_cast<rti1516e::Octet>(0));
}

void requireZeroPadding(
    std::vector<rti1516e::Octet> const& bytes,
    std::size_t index,
    std::size_t count) {
  if (index > bytes.size() || bytes.size() - index < count) {
    invalidEncoding(L"The HLAfixedRecord encoding is truncated in its padding.");
  }
  for (std::size_t offset = 0U; offset < count; ++offset) {
    if (static_cast<std::uint8_t>(bytes[index + offset]) != 0U) {
      invalidEncoding(L"The HLAfixedRecord encoding contains nonzero padding.");
    }
  }
}

}  // namespace

namespace rti1516e {

class HLAfixedRecordImplementation {
 public:
  class ElementSlot {
   public:
    explicit ElementSlot(std::unique_ptr<DataElement> ownedElement)
        : owned(std::move(ownedElement)), element(owned.get()) {}

    explicit ElementSlot(DataElement* borrowedElement) : element(borrowedElement) {}

    std::unique_ptr<DataElement> owned;
    DataElement* element = nullptr;
  };

  std::vector<ElementSlot> elements;
};

namespace {

[[nodiscard]] DataElement& elementAt(HLAfixedRecordImplementation& implementation, std::size_t index) {
  if (index >= implementation.elements.size() || implementation.elements[index].element == nullptr) {
    invalidEncoding(L"The HLAfixedRecord element index is invalid.");
  }
  return *implementation.elements[index].element;
}

[[nodiscard]] DataElement const& elementAt(
    HLAfixedRecordImplementation const& implementation,
    std::size_t index) {
  if (index >= implementation.elements.size() || implementation.elements[index].element == nullptr) {
    invalidEncoding(L"The HLAfixedRecord element index is invalid.");
  }
  return *implementation.elements[index].element;
}

void appendClone(HLAfixedRecordImplementation& implementation, DataElement const& element) {
  auto clone = element.clone();
  if (clone.get() == nullptr) {
    invalidEncoding(L"An HLAfixedRecord element clone is invalid.");
  }
  implementation.elements.emplace_back(
      std::unique_ptr<DataElement>(clone.release()));
}

}  // namespace

HLAfixedRecord::HLAfixedRecord() : _impl(new HLAfixedRecordImplementation()) {}

HLAfixedRecord::HLAfixedRecord(HLAfixedRecord const& rhs)
    : _impl(new HLAfixedRecordImplementation()) {
  for (std::size_t index = 0U; index < rhs.size(); ++index) {
    appendClone(*_impl, rhs.get(index));
  }
}

HLAfixedRecord::~HLAfixedRecord() {
  delete _impl;
}

std::auto_ptr<DataElement> HLAfixedRecord::clone() const {
  return std::auto_ptr<DataElement>(new HLAfixedRecord(*this));
}

VariableLengthData HLAfixedRecord::encode() const {
  VariableLengthData value;
  encode(value);
  return value;
}

void HLAfixedRecord::encode(VariableLengthData& value) const {
  std::vector<Octet> bytes;
  encodeInto(bytes);
  value.setData(bytes.data(), bytes.size());
}

void HLAfixedRecord::encodeInto(std::vector<Octet>& bytes) const {
  std::size_t recordOffset = 0U;
  for (std::size_t index = 0U; index < _impl->elements.size(); ++index) {
    auto const start = bytes.size();
    elementAt(*_impl, index).encodeInto(bytes);
    auto const componentLength = bytes.size() - start;
    if (index + 1U == _impl->elements.size()) {
      continue;
    }
    auto const padding = paddingAfter(
        recordOffset,
        componentLength,
        elementAt(*_impl, index + 1U).getOctetBoundary());
    appendZeroPadding(bytes, padding);
    recordOffset = checkedAdd(recordOffset, checkedAdd(componentLength, padding));
  }
}

void HLAfixedRecord::decode(VariableLengthData const& value) {
  auto const bytes = toOctets(value);
  auto const nextIndex = decodeFrom(bytes, 0U);
  if (nextIndex != bytes.size()) {
    invalidEncoding(L"The HLAfixedRecord encoding has trailing data.");
  }
  return;
}

std::size_t HLAfixedRecord::decodeFrom(
    std::vector<Octet> const& bytes,
    std::size_t index) {
  if (index > bytes.size()) {
    invalidEncoding(L"The HLAfixedRecord decode index is invalid.");
  }
  auto cursor = index;
  std::size_t recordOffset = 0U;
  for (std::size_t componentIndex = 0U; componentIndex < _impl->elements.size(); ++componentIndex) {
    auto const componentStart = cursor;
    cursor = elementAt(*_impl, componentIndex).decodeFrom(bytes, cursor);
    if (cursor < componentStart || cursor > bytes.size()) {
      invalidEncoding(L"An HLAfixedRecord component returned an invalid decoded length.");
    }
    auto const componentLength = cursor - componentStart;
    if (componentIndex + 1U == _impl->elements.size()) {
      continue;
    }
    auto const padding = paddingAfter(
        recordOffset,
        componentLength,
        elementAt(*_impl, componentIndex + 1U).getOctetBoundary());
    requireZeroPadding(bytes, cursor, padding);
    cursor = checkedAdd(cursor, padding);
    recordOffset = checkedAdd(recordOffset, checkedAdd(componentLength, padding));
  }
  return cursor;
}

std::size_t HLAfixedRecord::getEncodedLength() const {
  std::size_t recordLength = 0U;
  for (std::size_t index = 0U; index < _impl->elements.size(); ++index) {
    auto const componentLength = elementAt(*_impl, index).getEncodedLength();
    if (index + 1U == _impl->elements.size()) {
      recordLength = checkedAdd(recordLength, componentLength);
      continue;
    }
    auto const padding = paddingAfter(
        recordLength,
        componentLength,
        elementAt(*_impl, index + 1U).getOctetBoundary());
    recordLength = checkedAdd(recordLength, checkedAdd(componentLength, padding));
  }
  return recordLength;
}

unsigned int HLAfixedRecord::getOctetBoundary() const {
  unsigned int boundary = 1U;
  for (auto const& slot : _impl->elements) {
    if (slot.element == nullptr) {
      invalidEncoding(L"An HLAfixedRecord element is invalid.");
    }
    auto const elementBoundary = slot.element->getOctetBoundary();
    if (elementBoundary == 0U) {
      invalidEncoding(L"An HLAfixedRecord component has an invalid octet boundary.");
    }
    boundary = std::max(boundary, elementBoundary);
  }
  return boundary;
}

bool HLAfixedRecord::isSameTypeAs(DataElement const& inData) const {
  auto const* other = dynamic_cast<HLAfixedRecord const*>(&inData);
  if (other == nullptr || size() != other->size()) {
    return false;
  }
  for (std::size_t index = 0U; index < size(); ++index) {
    if (!hasElementSameTypeAs(index, other->get(index))) {
      return false;
    }
  }
  return true;
}

bool HLAfixedRecord::hasElementSameTypeAs(
    std::size_t index,
    DataElement const& inData) const {
  if (index >= _impl->elements.size() || _impl->elements[index].element == nullptr) {
    return false;
  }
  return _impl->elements[index].element->isSameTypeAs(inData);
}

std::size_t HLAfixedRecord::size() const {
  return _impl->elements.size();
}

void HLAfixedRecord::appendElement(DataElement const& dataElement) {
  appendClone(*_impl, dataElement);
  return;
}

void HLAfixedRecord::appendElementPointer(DataElement* dataElement) {
  if (dataElement == nullptr) {
    invalidEncoding(L"An HLAfixedRecord element pointer must be non-null.");
  }
  _impl->elements.emplace_back(dataElement);
  return;
}

void HLAfixedRecord::set(std::size_t index, DataElement const& dataElement) {
  auto& existing = elementAt(*_impl, index);
  if (!existing.isSameTypeAs(dataElement)) {
    invalidEncoding(L"The replacement HLAfixedRecord element type does not match.");
  }
  existing.decode(dataElement.encode());
  return;
}

void HLAfixedRecord::setElementPointer(std::size_t index, DataElement* dataElement) {
  if (dataElement == nullptr) {
    invalidEncoding(L"An HLAfixedRecord element pointer must be non-null.");
  }
  if (index >= _impl->elements.size() || _impl->elements[index].element == nullptr) {
    invalidEncoding(L"The HLAfixedRecord element index is invalid.");
  }
  auto& slot = _impl->elements[index];
  if (!slot.element->isSameTypeAs(*dataElement)) {
    invalidEncoding(L"The replacement HLAfixedRecord element type does not match.");
  }
  if (slot.element != dataElement) {
    slot.owned.reset();
    slot.element = dataElement;
  }
  return;
}

DataElement const& HLAfixedRecord::get(std::size_t index) const {
  return elementAt(*_impl, index);
}

DataElement const& HLAfixedRecord::operator[](std::size_t index) const {
  return get(index);
}

}  // namespace rti1516e
