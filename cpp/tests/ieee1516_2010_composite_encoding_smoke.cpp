#include <RTI/VariableLengthData.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/EncodingExceptions.h>
#include <RTI/encoding/HLAfixedArray.h>
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAvariableArray.h>
#include <RTI/encoding/HLAvariantRecord.h>

#include "internal/encoding/composite_2010.hpp"
#include "internal/encoding/variable_length_data_2010.hpp"

#include <cassert>
#include <cstring>
#include <string>

namespace {

template <typename Element>
void assertBytes(Element const& element, std::string const& expected) {
  auto const encoded = element.encode();
  assert(encoded.size() == expected.size());
  assert(std::memcmp(encoded.data(), expected.data(), expected.size()) == 0);
}

}  // namespace

int main() {
  using namespace rti1516e;

  std::string const encoded_bytes("\x00\x10\x80\xff", 4);
  VariableLengthData encoded_value(encoded_bytes.data(), encoded_bytes.size());
  auto const copied_bytes =
      umbra::detail::variable_length_data_2010::copyEncodedBytes(
          encoded_value, L"The encoded 2010 buffer is invalid.");
  assert(copied_bytes.size() == encoded_bytes.size());
  assert(std::memcmp(copied_bytes.data(), encoded_bytes.data(), encoded_bytes.size()) == 0);
  assert(umbra::detail::variable_length_data_2010::copyEncodedBytes(
             VariableLengthData{}, L"The encoded 2010 buffer is invalid.")
             .empty());

  std::size_t checked_sum = 0U;
  assert(umbra::detail::composite2010::tryCheckedAdd(4U, 5U, checked_sum));
  assert(checked_sum == 9U);
  std::size_t padding = 0U;
  assert(umbra::detail::composite2010::tryPaddingToBoundary(5U, 4U, padding));
  assert(padding == 3U);
  assert(!umbra::detail::composite2010::tryPaddingToBoundary(5U, 0U, padding));
  std::vector<Octet> padding_bytes;
  umbra::detail::composite2010::appendZeroPadding(padding_bytes, 3U);
  assert(umbra::detail::composite2010::isZeroPadding(padding_bytes, 0U, 3U));
  assert(!umbra::detail::composite2010::hasRange(padding_bytes, 2U, 2U));

  HLAinteger32BE integer_one(1);
  HLAinteger32BE integer_two(2);

  HLAfixedArray fixed(integer_one, 2U);
  fixed.set(0U, integer_one);
  fixed.set(1U, integer_two);
  assert(fixed.size() == 2U);
  assertBytes(fixed, std::string("\x00\x00\x00\x01\x00\x00\x00\x02", 8));

  HLAfixedArray decoded_fixed(integer_one, 2U);
  decoded_fixed.decode(fixed.encode());
  assert(decoded_fixed.get(1U).encode().size() == 4U);

  HLAvariableArray variable(integer_one);
  variable.addElement(integer_one);
  variable.addElement(integer_two);
  assert(variable.size() == 2U);
  assertBytes(variable, std::string("\x00\x00\x00\x02\x00\x00\x00\x01\x00\x00\x00\x02", 12));

  HLAvariableArray decoded_variable(integer_one);
  decoded_variable.decode(variable.encode());
  assert(decoded_variable.size() == 2U);

  HLAoctet octet(0x7f);
  HLAfixedRecord record;
  record.appendElement(octet);
  record.appendElement(integer_one);
  assert(record.size() == 2U);
  assertBytes(record, std::string("\x7f\x00\x00\x00\x00\x00\x00\x01", 8));

  HLAvariantRecord variant(octet);
  variant.addVariant(octet, integer_one);
  assertBytes(variant, std::string("\x7f\x00\x00\x00\x00\x00\x00\x01", 8));
  variant.setDiscriminant(HLAoctet(0x01));
  assertBytes(variant, std::string("\x01", 1));

  bool rejected = false;
  try {
    auto malformed = std::string("\x7f\x01\x00\x00\x00\x00\x00\x01", 8);
    record.decode(VariableLengthData(malformed.data(), malformed.size()));
  } catch (EncoderException const&) {
    rejected = true;
  }
  assert(rejected);

  return 0;
}
