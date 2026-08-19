#include <RTI/VariableLengthData.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/EncodingExceptions.h>
#include <RTI/encoding/HLAextendableVariantRecord.h>

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
    invalidEncoding(L"The encoded HLAextendableVariantRecord buffer is invalid.");
  }
  return bytes == nullptr ? std::vector<rti1516_2025::Octet>{}
                          : std::vector<rti1516_2025::Octet>(bytes, bytes + input.size());
}

[[nodiscard]] std::size_t checkedAdd(std::size_t left, std::size_t right) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    invalidEncoding(L"The HLAextendableVariantRecord encoded length overflows size_t.");
  }
  return left + right;
}

[[nodiscard]] std::size_t paddingToBoundary(std::size_t encodedLength, unsigned int boundary) {
  if (boundary == 0U) {
    invalidEncoding(L"An HLAextendableVariantRecord component has an invalid octet boundary.");
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
    invalidEncoding(L"The HLAextendableVariantRecord encoding is truncated in its padding.");
  }
  for (std::size_t offset = 0U; offset < count; ++offset) {
    if (static_cast<std::uint8_t>(bytes[index + offset]) != 0U) {
      invalidEncoding(L"The HLAextendableVariantRecord encoding contains nonzero padding.");
    }
  }
}

}  // namespace

namespace rti1516_2025 {

class HLAextendableVariantRecordImplementation {
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

constexpr unsigned int kEncodedLengthBoundary = 4U;
constexpr unsigned int kAlternativeBoundary = 8U;

[[nodiscard]] std::unique_ptr<DataElement> cloneElement(DataElement const& element) {
  auto clone = element.clone();
  if (!clone) {
    invalidEncoding(L"An HLAextendableVariantRecord element clone is invalid.");
  }
  return clone;
}

[[nodiscard]] DataElement const& discriminantPrototypeOf(
    HLAextendableVariantRecordImplementation const& implementation) {
  if (!implementation.discriminantPrototype) {
    invalidEncoding(L"The HLAextendableVariantRecord discriminant prototype is invalid.");
  }
  return *implementation.discriminantPrototype;
}

[[nodiscard]] DataElement const& currentDiscriminantOf(
    HLAextendableVariantRecordImplementation const& implementation) {
  if (!implementation.currentDiscriminant) {
    invalidEncoding(L"The HLAextendableVariantRecord current discriminant is invalid.");
  }
  return *implementation.currentDiscriminant;
}

[[nodiscard]] DataElement const& valueOf(
    HLAextendableVariantRecordImplementation::VariantSlot const& slot) {
  if (slot.value == nullptr) {
    invalidEncoding(L"An HLAextendableVariantRecord variant is invalid.");
  }
  return *slot.value;
}

[[nodiscard]] DataElement& valueOf(HLAextendableVariantRecordImplementation::VariantSlot& slot) {
  return const_cast<DataElement&>(
      valueOf(static_cast<HLAextendableVariantRecordImplementation::VariantSlot const&>(slot)));
}

void assignOwnedValue(
    HLAextendableVariantRecordImplementation::VariantSlot& slot,
    std::unique_ptr<DataElement> value) {
  if (!value) {
    invalidEncoding(L"An HLAextendableVariantRecord variant clone is invalid.");
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

[[nodiscard]] HLAextendableVariantRecordImplementation::VariantSlot const* findVariant(
    HLAextendableVariantRecordImplementation const& implementation,
    DataElement const& discriminant) {
  auto const discriminantHash = discriminant.hash();
  for (auto const& slot : implementation.variants) {
    if (!slot.discriminant) {
      invalidEncoding(L"An HLAextendableVariantRecord discriminant mapping is invalid.");
    }
    if (slot.discriminantHash == discriminantHash &&
        hasSameDiscriminantValue(*slot.discriminant, discriminant)) {
      return &slot;
    }
  }
  return nullptr;
}

[[nodiscard]] HLAextendableVariantRecordImplementation::VariantSlot* findVariant(
    HLAextendableVariantRecordImplementation& implementation,
    DataElement const& discriminant) {
  return const_cast<HLAextendableVariantRecordImplementation::VariantSlot*>(
      findVariant(static_cast<HLAextendableVariantRecordImplementation const&>(implementation), discriminant));
}

void requireMatchingDiscriminantType(
    HLAextendableVariantRecordImplementation const& implementation,
    DataElement const& discriminant) {
  if (!discriminantPrototypeOf(implementation).isSameTypeAs(discriminant)) {
    invalidEncoding(L"The HLAextendableVariantRecord discriminant type does not match its prototype.");
  }
}

void setCurrentDiscriminant(
    HLAextendableVariantRecordImplementation& implementation,
    DataElement const& discriminant) {
  implementation.currentDiscriminant = cloneElement(discriminant);
}

[[nodiscard]] unsigned int discriminantBoundary(
    HLAextendableVariantRecordImplementation const& implementation) {
  auto const boundary = discriminantPrototypeOf(implementation).getOctetBoundary();
  if (boundary == 0U) {
    invalidEncoding(L"An HLAextendableVariantRecord discriminant has an invalid octet boundary.");
  }
  return boundary;
}

void requireSupportedAlternative(DataElement const& value) {
  auto const boundary = value.getOctetBoundary();
  if (boundary == 0U || boundary > kAlternativeBoundary) {
    invalidEncoding(
        L"An HLAextendableVariantRecord alternative octet boundary must be in the range 1 through 8.");
  }
}

[[nodiscard]] unsigned int recordBoundary(
    HLAextendableVariantRecordImplementation const& implementation) {
  return std::max(discriminantBoundary(implementation), kAlternativeBoundary);
}

void requireEncodableAlternativeLength(std::size_t length) {
  if (length > static_cast<std::size_t>(std::numeric_limits<Integer32>::max())) {
    invalidEncoding(
        L"The HLAextendableVariantRecord alternative length exceeds HLAinteger32BE.");
  }
}

void appendVariant(
    HLAextendableVariantRecordImplementation& implementation,
    DataElement const& discriminant,
    std::unique_ptr<DataElement> ownedValue,
    DataElement* borrowedValue) {
  HLAextendableVariantRecordImplementation::VariantSlot slot;
  slot.discriminant = cloneElement(discriminant);
  slot.discriminantHash = slot.discriminant->hash();
  if (ownedValue) {
    assignOwnedValue(slot, std::move(ownedValue));
  } else {
    if (borrowedValue == nullptr) {
      invalidEncoding(L"An HLAextendableVariantRecord variant pointer must be non-null.");
    }
    slot.value = borrowedValue;
  }
  requireSupportedAlternative(valueOf(slot));
  implementation.variants.emplace_back(std::move(slot));
}

}  // namespace

HLAextendableVariantRecord::HLAextendableVariantRecord(
    DataElement const& discriminantPrototype)
    : _impl(nullptr) {
  auto implementation = std::make_unique<HLAextendableVariantRecordImplementation>();
  implementation->discriminantPrototype = cloneElement(discriminantPrototype);
  implementation->currentDiscriminant = cloneElement(discriminantPrototype);
  _impl = implementation.release();
}

HLAextendableVariantRecord::HLAextendableVariantRecord(
    HLAextendableVariantRecord const& rhs)
    : _impl(nullptr) {
  auto implementation = std::make_unique<HLAextendableVariantRecordImplementation>();
  implementation->discriminantPrototype = cloneElement(discriminantPrototypeOf(*rhs._impl));
  implementation->currentDiscriminant = cloneElement(currentDiscriminantOf(*rhs._impl));
  implementation->variants.reserve(rhs._impl->variants.size());
  for (auto const& sourceSlot : rhs._impl->variants) {
    if (!sourceSlot.discriminant) {
      invalidEncoding(L"An HLAextendableVariantRecord discriminant mapping is invalid.");
    }
    appendVariant(
        *implementation,
        *sourceSlot.discriminant,
        cloneElement(valueOf(sourceSlot)),
        nullptr);
  }
  _impl = implementation.release();
}

HLAextendableVariantRecord::~HLAextendableVariantRecord() {
  delete _impl;
}

std::unique_ptr<DataElement> HLAextendableVariantRecord::clone() const {
  return std::make_unique<HLAextendableVariantRecord>(*this);
}

VariableLengthData HLAextendableVariantRecord::encode() const {
  VariableLengthData value;
  encode(value);
  return value;
}

void HLAextendableVariantRecord::encode(VariableLengthData& value) const {
  std::vector<Octet> bytes;
  encodeInto(bytes);
  value.setData(bytes.data(), bytes.size());
}

void HLAextendableVariantRecord::encodeInto(std::vector<Octet>& bytes) const {
  auto const& discriminant = currentDiscriminantOf(*_impl);
  auto const recordStart = bytes.size();
  discriminant.encodeInto(bytes);
  if (bytes.size() < recordStart) {
    invalidEncoding(L"The HLAextendableVariantRecord discriminant produced an invalid encoded length.");
  }

  auto const discriminantLength = bytes.size() - recordStart;
  appendZeroPadding(bytes, paddingToBoundary(discriminantLength, kEncodedLengthBoundary));

  auto const* variant = findVariant(*_impl, discriminant);
  std::vector<Octet> alternativeBytes;
  if (variant != nullptr) {
    valueOf(*variant).encodeInto(alternativeBytes);
    requireEncodableAlternativeLength(alternativeBytes.size());
  }

  HLAinteger32BE encodedLength{static_cast<Integer32>(alternativeBytes.size())};
  encodedLength.encodeInto(bytes);

  // The predefined alternative boundary is eight even if the selected
  // alternative has a smaller boundary.  This pad is structurally present
  // before the (possibly zero-length) alternative, so an unknown alternative
  // can be skipped using its encoded_length field.
  auto const prefixLength = bytes.size() - recordStart;
  appendZeroPadding(bytes, paddingToBoundary(prefixLength, kAlternativeBoundary));
  bytes.insert(bytes.end(), alternativeBytes.begin(), alternativeBytes.end());
}

HLAextendableVariantRecord& HLAextendableVariantRecord::decode(
    VariableLengthData const& value) {
  auto const bytes = toOctets(value);
  auto const nextIndex = decodeFrom(bytes, 0U);
  if (nextIndex != bytes.size()) {
    invalidEncoding(L"The HLAextendableVariantRecord encoding has trailing data.");
  }
  return *this;
}

std::size_t HLAextendableVariantRecord::decodeFrom(
    std::vector<Octet> const& bytes,
    std::size_t index) {
  if (index > bytes.size()) {
    invalidEncoding(L"The HLAextendableVariantRecord decode index is invalid.");
  }

  auto decodedDiscriminant = cloneElement(discriminantPrototypeOf(*_impl));
  auto cursor = decodedDiscriminant->decodeFrom(bytes, index);
  if (cursor < index || cursor > bytes.size()) {
    invalidEncoding(
        L"The HLAextendableVariantRecord discriminant returned an invalid decoded length.");
  }

  auto const discriminantLength = cursor - index;
  auto const discriminantPadding = paddingToBoundary(discriminantLength, kEncodedLengthBoundary);
  requireZeroPadding(bytes, cursor, discriminantPadding);
  cursor = checkedAdd(cursor, discriminantPadding);

  HLAinteger32BE encodedLength;
  cursor = encodedLength.decodeFrom(bytes, cursor);
  if (encodedLength.get() < 0) {
    invalidEncoding(L"The HLAextendableVariantRecord alternative length is negative.");
  }
  auto const alternativeLength = static_cast<std::size_t>(encodedLength.get());

  auto const prefixLength = cursor - index;
  auto const alternativePadding = paddingToBoundary(prefixLength, kAlternativeBoundary);
  requireZeroPadding(bytes, cursor, alternativePadding);
  cursor = checkedAdd(cursor, alternativePadding);

  auto const alternativeEnd = checkedAdd(cursor, alternativeLength);
  if (alternativeEnd > bytes.size()) {
    invalidEncoding(L"The HLAextendableVariantRecord alternative is truncated.");
  }

  auto* variant = findVariant(*_impl, *decodedDiscriminant);
  _impl->currentDiscriminant = std::move(decodedDiscriminant);
  if (variant == nullptr) {
    // The length prefix makes this successful skip possible for future or
    // otherwise unknown alternatives.
    return alternativeEnd;
  }

  std::vector<Octet> alternativeBytes(
      bytes.begin() + static_cast<std::ptrdiff_t>(cursor),
      bytes.begin() + static_cast<std::ptrdiff_t>(alternativeEnd));
  auto const consumed = valueOf(*variant).decodeFrom(alternativeBytes, 0U);
  if (consumed != alternativeBytes.size()) {
    invalidEncoding(
        L"The HLAextendableVariantRecord alternative does not match its encoded length.");
  }
  return alternativeEnd;
}

std::size_t HLAextendableVariantRecord::getEncodedLength() const {
  auto const& discriminant = currentDiscriminantOf(*_impl);
  auto const discriminantLength = discriminant.getEncodedLength();
  auto length = checkedAdd(
      discriminantLength,
      paddingToBoundary(discriminantLength, kEncodedLengthBoundary));
  length = checkedAdd(length, kEncodedLengthBoundary);
  length = checkedAdd(length, paddingToBoundary(length, kAlternativeBoundary));

  auto const* variant = findVariant(*_impl, discriminant);
  if (variant == nullptr) {
    return length;
  }
  auto const alternativeLength = valueOf(*variant).getEncodedLength();
  requireEncodableAlternativeLength(alternativeLength);
  return checkedAdd(length, alternativeLength);
}

unsigned int HLAextendableVariantRecord::getOctetBoundary() const {
  return recordBoundary(*_impl);
}

bool HLAextendableVariantRecord::isSameTypeAs(DataElement const& inData) const {
  auto const* other = dynamic_cast<HLAextendableVariantRecord const*>(&inData);
  if (other == nullptr ||
      !hasMatchingDiscriminantTypeAs(discriminantPrototypeOf(*other->_impl)) ||
      _impl->variants.size() != other->_impl->variants.size()) {
    return false;
  }

  for (auto const& slot : _impl->variants) {
    if (!slot.discriminant) {
      invalidEncoding(L"An HLAextendableVariantRecord discriminant mapping is invalid.");
    }
    auto const* otherSlot = findVariant(*other->_impl, *slot.discriminant);
    if (otherSlot == nullptr || !valueOf(slot).isSameTypeAs(valueOf(*otherSlot))) {
      return false;
    }
  }
  return true;
}

bool HLAextendableVariantRecord::isSameTypeAs(
    DataElement const& discriminant,
    DataElement const& inData) const {
  if (!hasMatchingDiscriminantTypeAs(discriminant)) {
    return false;
  }
  auto const* variant = findVariant(*_impl, discriminant);
  return variant != nullptr && valueOf(*variant).isSameTypeAs(inData);
}

bool HLAextendableVariantRecord::hasMatchingDiscriminantTypeAs(
    DataElement const& dataElement) const {
  return discriminantPrototypeOf(*_impl).isSameTypeAs(dataElement);
}

HLAextendableVariantRecord& HLAextendableVariantRecord::addVariant(
    DataElement const& discriminant,
    DataElement const& valuePrototype) {
  requireMatchingDiscriminantType(*_impl, discriminant);
  if (findVariant(*_impl, discriminant) != nullptr) {
    invalidEncoding(L"The HLAextendableVariantRecord discriminant is already mapped to a variant.");
  }
  appendVariant(*_impl, discriminant, cloneElement(valuePrototype), nullptr);
  setCurrentDiscriminant(*_impl, discriminant);
  return *this;
}

HLAextendableVariantRecord& HLAextendableVariantRecord::addVariantPointer(
    DataElement const& discriminant,
    DataElement* valuePtr) {
  if (valuePtr == nullptr) {
    invalidEncoding(L"An HLAextendableVariantRecord variant pointer must be non-null.");
  }
  requireMatchingDiscriminantType(*_impl, discriminant);
  if (findVariant(*_impl, discriminant) != nullptr) {
    invalidEncoding(L"The HLAextendableVariantRecord discriminant is already mapped to a variant.");
  }
  appendVariant(*_impl, discriminant, nullptr, valuePtr);
  setCurrentDiscriminant(*_impl, discriminant);
  return *this;
}

HLAextendableVariantRecord& HLAextendableVariantRecord::setDiscriminant(
    DataElement const& discriminant) {
  requireMatchingDiscriminantType(*_impl, discriminant);
  setCurrentDiscriminant(*_impl, discriminant);
  return *this;
}

HLAextendableVariantRecord& HLAextendableVariantRecord::setVariant(
    DataElement const& discriminant,
    DataElement const& value) {
  requireMatchingDiscriminantType(*_impl, discriminant);
  auto* variant = findVariant(*_impl, discriminant);
  if (variant == nullptr) {
    invalidEncoding(L"The HLAextendableVariantRecord discriminant is not mapped to a variant.");
  }
  if (!valueOf(*variant).isSameTypeAs(value)) {
    invalidEncoding(L"The HLAextendableVariantRecord replacement variant type does not match.");
  }
  requireSupportedAlternative(value);
  // This deliberately decodes into the existing slot.  In particular, an
  // addVariantPointer slot remains caller-owned and is updated in place.
  valueOf(*variant).decode(value.encode());
  setCurrentDiscriminant(*_impl, discriminant);
  return *this;
}

HLAextendableVariantRecord& HLAextendableVariantRecord::setVariantPointer(
    DataElement const& discriminant,
    DataElement* valuePtr) {
  if (valuePtr == nullptr) {
    invalidEncoding(L"An HLAextendableVariantRecord variant pointer must be non-null.");
  }
  requireMatchingDiscriminantType(*_impl, discriminant);
  auto* variant = findVariant(*_impl, discriminant);
  if (variant == nullptr) {
    invalidEncoding(L"The HLAextendableVariantRecord discriminant is not mapped to a variant.");
  }
  if (!valueOf(*variant).isSameTypeAs(*valuePtr)) {
    invalidEncoding(L"The HLAextendableVariantRecord replacement variant type does not match.");
  }
  requireSupportedAlternative(*valuePtr);
  if (variant->value != valuePtr) {
    variant->ownedValue.reset();
    variant->value = valuePtr;
  }
  setCurrentDiscriminant(*_impl, discriminant);
  return *this;
}

DataElement const& HLAextendableVariantRecord::getDiscriminant() const {
  return currentDiscriminantOf(*_impl);
}

DataElement const& HLAextendableVariantRecord::getVariant() const {
  auto const* variant = findVariant(*_impl, currentDiscriminantOf(*_impl));
  if (variant == nullptr) {
    invalidEncoding(L"The HLAextendableVariantRecord current discriminant is not mapped to a variant.");
  }
  return valueOf(*variant);
}

}  // namespace rti1516_2025
