#include <RTI/VariableLengthData.h>
#include <RTI/encoding/EncodingExceptions.h>
#include <RTI/encoding/HLAvariantRecord.h>

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
    invalidEncoding(L"The encoded HLAvariantRecord buffer is invalid.");
  }
  return bytes == nullptr ? std::vector<rti1516_2025::Octet>{}
                          : std::vector<rti1516_2025::Octet>(bytes, bytes + input.size());
}

[[nodiscard]] std::size_t checkedAdd(std::size_t left, std::size_t right) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    invalidEncoding(L"The HLAvariantRecord encoded length overflows size_t.");
  }
  return left + right;
}

[[nodiscard]] std::size_t paddingToBoundary(std::size_t encodedLength, unsigned int boundary) {
  if (boundary == 0U) {
    invalidEncoding(L"An HLAvariantRecord component has an invalid octet boundary.");
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
    invalidEncoding(L"The HLAvariantRecord encoding is truncated in its padding.");
  }
  for (std::size_t offset = 0U; offset < count; ++offset) {
    if (static_cast<std::uint8_t>(bytes[index + offset]) != 0U) {
      invalidEncoding(L"The HLAvariantRecord encoding contains nonzero padding.");
    }
  }
}

}  // namespace

namespace rti1516_2025 {

class HLAvariantRecordImplementation {
 public:
  class VariantSlot {
   public:
    std::unique_ptr<DataElement> discriminant;
    Integer64 discriminantHash = 0;
    std::unique_ptr<DataElement> ownedValue;
    DataElement* value = nullptr;
  };

  std::unique_ptr<DataElement> discriminantPrototype;
  std::unique_ptr<DataElement> currentDiscriminant;
  std::vector<VariantSlot> variants;
};

namespace {

[[nodiscard]] std::unique_ptr<DataElement> cloneElement(DataElement const& element) {
  auto clone = element.clone();
  if (!clone) {
    invalidEncoding(L"An HLAvariantRecord element clone is invalid.");
  }
  return clone;
}

[[nodiscard]] DataElement const& discriminantPrototypeOf(
    HLAvariantRecordImplementation const& implementation) {
  if (!implementation.discriminantPrototype) {
    invalidEncoding(L"The HLAvariantRecord discriminant prototype is invalid.");
  }
  return *implementation.discriminantPrototype;
}

[[nodiscard]] DataElement const& currentDiscriminantOf(
    HLAvariantRecordImplementation const& implementation) {
  if (!implementation.currentDiscriminant) {
    invalidEncoding(L"The HLAvariantRecord current discriminant is invalid.");
  }
  return *implementation.currentDiscriminant;
}

[[nodiscard]] DataElement const& valueOf(HLAvariantRecordImplementation::VariantSlot const& slot) {
  if (slot.value == nullptr) {
    invalidEncoding(L"An HLAvariantRecord variant is invalid.");
  }
  return *slot.value;
}

[[nodiscard]] DataElement& valueOf(HLAvariantRecordImplementation::VariantSlot& slot) {
  return const_cast<DataElement&>(valueOf(static_cast<HLAvariantRecordImplementation::VariantSlot const&>(slot)));
}

void assignOwnedValue(
    HLAvariantRecordImplementation::VariantSlot& slot,
    std::unique_ptr<DataElement> value) {
  if (!value) {
    invalidEncoding(L"An HLAvariantRecord variant clone is invalid.");
  }
  slot.ownedValue = std::move(value);
  slot.value = slot.ownedValue.get();
}

[[nodiscard]] std::vector<Octet> encodedOctets(DataElement const& element) {
  return toOctets(element.encode());
}

[[nodiscard]] bool hasSameDiscriminantValue(
    DataElement const& left,
    DataElement const& right) {
  // DataElement::hash is the binding's lookup hook for discriminants.  The
  // encoded bytes and type check remain the identity check so a hash collision
  // cannot select the wrong alternative.
  return left.isSameTypeAs(right) && left.hash() == right.hash() &&
         encodedOctets(left) == encodedOctets(right);
}

[[nodiscard]] HLAvariantRecordImplementation::VariantSlot const* findVariant(
    HLAvariantRecordImplementation const& implementation,
    DataElement const& discriminant) {
  auto const discriminantHash = discriminant.hash();
  for (auto const& slot : implementation.variants) {
    if (!slot.discriminant) {
      invalidEncoding(L"An HLAvariantRecord discriminant mapping is invalid.");
    }
    if (slot.discriminantHash == discriminantHash &&
        hasSameDiscriminantValue(*slot.discriminant, discriminant)) {
      return &slot;
    }
  }
  return nullptr;
}

[[nodiscard]] HLAvariantRecordImplementation::VariantSlot* findVariant(
    HLAvariantRecordImplementation& implementation,
    DataElement const& discriminant) {
  return const_cast<HLAvariantRecordImplementation::VariantSlot*>(
      findVariant(static_cast<HLAvariantRecordImplementation const&>(implementation), discriminant));
}

void requireMatchingDiscriminantType(
    HLAvariantRecordImplementation const& implementation,
    DataElement const& discriminant) {
  if (!discriminantPrototypeOf(implementation).isSameTypeAs(discriminant)) {
    invalidEncoding(L"The HLAvariantRecord discriminant type does not match its prototype.");
  }
}

void setCurrentDiscriminant(
    HLAvariantRecordImplementation& implementation,
    DataElement const& discriminant) {
  implementation.currentDiscriminant = cloneElement(discriminant);
}

[[nodiscard]] unsigned int discriminantBoundary(
    HLAvariantRecordImplementation const& implementation) {
  auto const boundary = discriminantPrototypeOf(implementation).getOctetBoundary();
  if (boundary == 0U) {
    invalidEncoding(L"An HLAvariantRecord discriminant has an invalid octet boundary.");
  }
  return boundary;
}

[[nodiscard]] unsigned int maximumAlternativeBoundary(
    HLAvariantRecordImplementation const& implementation) {
  unsigned int boundary = 1U;
  for (auto const& slot : implementation.variants) {
    auto const alternativeBoundary = valueOf(slot).getOctetBoundary();
    if (alternativeBoundary == 0U) {
      invalidEncoding(L"An HLAvariantRecord alternative has an invalid octet boundary.");
    }
    boundary = std::max(boundary, alternativeBoundary);
  }
  return boundary;
}

[[nodiscard]] unsigned int recordBoundary(HLAvariantRecordImplementation const& implementation) {
  return std::max(discriminantBoundary(implementation), maximumAlternativeBoundary(implementation));
}

void appendVariant(
    HLAvariantRecordImplementation& implementation,
    DataElement const& discriminant,
    std::unique_ptr<DataElement> ownedValue,
    DataElement* borrowedValue) {
  HLAvariantRecordImplementation::VariantSlot slot;
  slot.discriminant = cloneElement(discriminant);
  slot.discriminantHash = slot.discriminant->hash();
  if (ownedValue) {
    assignOwnedValue(slot, std::move(ownedValue));
  } else {
    if (borrowedValue == nullptr) {
      invalidEncoding(L"An HLAvariantRecord variant pointer must be non-null.");
    }
    slot.value = borrowedValue;
  }
  implementation.variants.emplace_back(std::move(slot));
}

}  // namespace

HLAvariantRecord::HLAvariantRecord(DataElement const& discriminantPrototype) : _impl(nullptr) {
  auto implementation = std::make_unique<HLAvariantRecordImplementation>();
  implementation->discriminantPrototype = cloneElement(discriminantPrototype);
  // Retain an immutable internal current discriminant from construction so an
  // otherwise unmapped discriminant can still use the standard's no-alternative
  // wire form.
  implementation->currentDiscriminant = cloneElement(discriminantPrototype);
  _impl = implementation.release();
}

HLAvariantRecord::HLAvariantRecord(HLAvariantRecord const& rhs) : _impl(nullptr) {
  auto implementation = std::make_unique<HLAvariantRecordImplementation>();
  implementation->discriminantPrototype = cloneElement(discriminantPrototypeOf(*rhs._impl));
  implementation->currentDiscriminant = cloneElement(currentDiscriminantOf(*rhs._impl));
  implementation->variants.reserve(rhs._impl->variants.size());
  for (auto const& sourceSlot : rhs._impl->variants) {
    if (!sourceSlot.discriminant) {
      invalidEncoding(L"An HLAvariantRecord discriminant mapping is invalid.");
    }
    appendVariant(
        *implementation,
        *sourceSlot.discriminant,
        cloneElement(valueOf(sourceSlot)),
        nullptr);
  }
  _impl = implementation.release();
}

HLAvariantRecord::~HLAvariantRecord() {
  delete _impl;
}

std::unique_ptr<DataElement> HLAvariantRecord::clone() const {
  return std::make_unique<HLAvariantRecord>(*this);
}

VariableLengthData HLAvariantRecord::encode() const {
  VariableLengthData value;
  encode(value);
  return value;
}

void HLAvariantRecord::encode(VariableLengthData& value) const {
  std::vector<Octet> bytes;
  encodeInto(bytes);
  value.setData(bytes.data(), bytes.size());
}

void HLAvariantRecord::encodeInto(std::vector<Octet>& bytes) const {
  auto const& discriminant = currentDiscriminantOf(*_impl);
  auto const discriminantStart = bytes.size();
  discriminant.encodeInto(bytes);
  if (bytes.size() < discriminantStart) {
    invalidEncoding(L"The HLAvariantRecord discriminant produced an invalid encoded length.");
  }

  auto const* variant = findVariant(*_impl, discriminant);
  if (variant == nullptr) {
    // IEEE 1516.2 §4.14.10.2 explicitly prohibits padding after an
    // unmapped discriminant.
    return;
  }

  auto const discriminantLength = bytes.size() - discriminantStart;
  appendZeroPadding(bytes, paddingToBoundary(discriminantLength, maximumAlternativeBoundary(*_impl)));
  auto const alternativeStart = bytes.size();
  valueOf(*variant).encodeInto(bytes);
  if (bytes.size() < alternativeStart) {
    invalidEncoding(L"The HLAvariantRecord alternative produced an invalid encoded length.");
  }
}

HLAvariantRecord& HLAvariantRecord::decode(VariableLengthData const& value) {
  auto const bytes = toOctets(value);
  auto const nextIndex = decodeFrom(bytes, 0U);
  if (nextIndex != bytes.size()) {
    invalidEncoding(L"The HLAvariantRecord encoding has trailing data.");
  }
  return *this;
}

std::size_t HLAvariantRecord::decodeFrom(
    std::vector<Octet> const& bytes,
    std::size_t index) {
  if (index > bytes.size()) {
    invalidEncoding(L"The HLAvariantRecord decode index is invalid.");
  }

  auto decodedDiscriminant = cloneElement(discriminantPrototypeOf(*_impl));
  auto cursor = decodedDiscriminant->decodeFrom(bytes, index);
  if (cursor < index || cursor > bytes.size()) {
    invalidEncoding(L"The HLAvariantRecord discriminant returned an invalid decoded length.");
  }

  auto* variant = findVariant(*_impl, *decodedDiscriminant);
  _impl->currentDiscriminant = std::move(decodedDiscriminant);
  if (variant == nullptr) {
    return cursor;
  }

  auto const padding = paddingToBoundary(cursor - index, maximumAlternativeBoundary(*_impl));
  requireZeroPadding(bytes, cursor, padding);
  cursor = checkedAdd(cursor, padding);

  auto const alternativeStart = cursor;
  cursor = valueOf(*variant).decodeFrom(bytes, cursor);
  if (cursor < alternativeStart || cursor > bytes.size()) {
    invalidEncoding(L"The HLAvariantRecord alternative returned an invalid decoded length.");
  }
  return cursor;
}

std::size_t HLAvariantRecord::getEncodedLength() const {
  auto const& discriminant = currentDiscriminantOf(*_impl);
  auto const discriminantLength = discriminant.getEncodedLength();
  auto const* variant = findVariant(*_impl, discriminant);
  if (variant == nullptr) {
    return discriminantLength;
  }
  auto const padding = paddingToBoundary(discriminantLength, maximumAlternativeBoundary(*_impl));
  return checkedAdd(checkedAdd(discriminantLength, padding), valueOf(*variant).getEncodedLength());
}

unsigned int HLAvariantRecord::getOctetBoundary() const {
  return recordBoundary(*_impl);
}

bool HLAvariantRecord::isSameTypeAs(DataElement const& inData) const {
  auto const* other = dynamic_cast<HLAvariantRecord const*>(&inData);
  if (other == nullptr ||
      !hasMatchingDiscriminantTypeAs(discriminantPrototypeOf(*other->_impl)) ||
      _impl->variants.size() != other->_impl->variants.size()) {
    return false;
  }

  for (auto const& slot : _impl->variants) {
    if (!slot.discriminant) {
      invalidEncoding(L"An HLAvariantRecord discriminant mapping is invalid.");
    }
    auto const* otherSlot = findVariant(*other->_impl, *slot.discriminant);
    if (otherSlot == nullptr || !valueOf(slot).isSameTypeAs(valueOf(*otherSlot))) {
      return false;
    }
  }
  return true;
}

bool HLAvariantRecord::isSameTypeAs(
    DataElement const& discriminant,
    DataElement const& inData) const {
  if (!hasMatchingDiscriminantTypeAs(discriminant)) {
    return false;
  }
  auto const* variant = findVariant(*_impl, discriminant);
  return variant != nullptr && valueOf(*variant).isSameTypeAs(inData);
}

bool HLAvariantRecord::hasMatchingDiscriminantTypeAs(DataElement const& dataElement) const {
  return discriminantPrototypeOf(*_impl).isSameTypeAs(dataElement);
}

HLAvariantRecord& HLAvariantRecord::addVariant(
    DataElement const& discriminant,
    DataElement const& valuePrototype) {
  requireMatchingDiscriminantType(*_impl, discriminant);
  if (findVariant(*_impl, discriminant) != nullptr) {
    invalidEncoding(L"The HLAvariantRecord discriminant is already mapped to a variant.");
  }
  appendVariant(*_impl, discriminant, cloneElement(valuePrototype), nullptr);
  setCurrentDiscriminant(*_impl, discriminant);
  return *this;
}

HLAvariantRecord& HLAvariantRecord::addVariantPointer(
    DataElement const& discriminant,
    DataElement* valuePtr) {
  if (valuePtr == nullptr) {
    invalidEncoding(L"An HLAvariantRecord variant pointer must be non-null.");
  }
  requireMatchingDiscriminantType(*_impl, discriminant);
  if (findVariant(*_impl, discriminant) != nullptr) {
    invalidEncoding(L"The HLAvariantRecord discriminant is already mapped to a variant.");
  }
  appendVariant(*_impl, discriminant, nullptr, valuePtr);
  setCurrentDiscriminant(*_impl, discriminant);
  return *this;
}

HLAvariantRecord& HLAvariantRecord::setDiscriminant(DataElement const& discriminant) {
  requireMatchingDiscriminantType(*_impl, discriminant);
  setCurrentDiscriminant(*_impl, discriminant);
  return *this;
}

HLAvariantRecord& HLAvariantRecord::setVariant(
    DataElement const& discriminant,
    DataElement const& value) {
  requireMatchingDiscriminantType(*_impl, discriminant);
  auto* variant = findVariant(*_impl, discriminant);
  if (variant == nullptr) {
    invalidEncoding(L"The HLAvariantRecord discriminant is not mapped to a variant.");
  }
  if (!valueOf(*variant).isSameTypeAs(value)) {
    invalidEncoding(L"The HLAvariantRecord replacement variant type does not match.");
  }
  // This deliberately decodes into the existing slot.  In particular, an
  // addVariantPointer slot remains caller-owned and is updated in place.
  valueOf(*variant).decode(value.encode());
  setCurrentDiscriminant(*_impl, discriminant);
  return *this;
}

HLAvariantRecord& HLAvariantRecord::setVariantPointer(
    DataElement const& discriminant,
    DataElement* valuePtr) {
  if (valuePtr == nullptr) {
    invalidEncoding(L"An HLAvariantRecord variant pointer must be non-null.");
  }
  requireMatchingDiscriminantType(*_impl, discriminant);
  auto* variant = findVariant(*_impl, discriminant);
  if (variant == nullptr) {
    invalidEncoding(L"The HLAvariantRecord discriminant is not mapped to a variant.");
  }
  if (!valueOf(*variant).isSameTypeAs(*valuePtr)) {
    invalidEncoding(L"The HLAvariantRecord replacement variant type does not match.");
  }
  if (variant->value != valuePtr) {
    variant->ownedValue.reset();
    variant->value = valuePtr;
  }
  setCurrentDiscriminant(*_impl, discriminant);
  return *this;
}

DataElement const& HLAvariantRecord::getDiscriminant() const {
  return currentDiscriminantOf(*_impl);
}

DataElement const& HLAvariantRecord::getVariant() const {
  auto const* variant = findVariant(*_impl, currentDiscriminantOf(*_impl));
  if (variant == nullptr) {
    invalidEncoding(L"The HLAvariantRecord current discriminant is not mapped to a variant.");
  }
  return valueOf(*variant);
}

}  // namespace rti1516_2025
