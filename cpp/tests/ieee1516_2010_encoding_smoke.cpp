#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/EncodingExceptions.h>
#include <RTI/encoding/HLAopaqueData.h>
#include <RTI/VariableLengthData.h>

#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>

namespace {

template <typename Element>
void assertBytes(Element const& element, std::string const& expected, char const* label) {
  auto const encoded = element.encode();
  (void)label;
  assert(encoded.size() == expected.size());
  assert(std::memcmp(encoded.data(), expected.data(), expected.size()) == 0);
}

}  // namespace

int main() {
  using namespace rti1516e;

  HLAinteger32BE integer(0x01020304);
  assertBytes(integer, "\x01\x02\x03\x04", "integer");
  integer.decode(VariableLengthData("\x04\x03\x02\x01", 4));
  assert(integer.get() == 0x04030201);

  HLAinteger32LE little(0x01020304);
  assertBytes(little, "\x04\x03\x02\x01", "little");
  little.decode(VariableLengthData("\x01\x02\x03\x04", 4));
  assert(little.get() == 0x04030201);

  HLAfloat64BE floating(1.25);
  assertBytes(floating, std::string("\x3f\xf4\x00\x00\x00\x00\x00\x00", 8), "floating");

  HLAboolean truth(true);
  assertBytes(truth, std::string("\x00\x00\x00\x01", 4), "truth");
  truth.decode(VariableLengthData("\x00\x00\x00\x00", 4));
  assert(!truth.get());

  HLAASCIIstring ascii("hello");
  assertBytes(ascii, std::string("\x00\x00\x00\x05hello", 9), "ascii");
  HLAunicodeString unicode(L"A");
  // Keep the final ASCII character outside the preceding \x00 escape: C++
  // hexadecimal escapes consume all following hexadecimal digits.
  assertBytes(unicode, std::string("\x00\x00\x00\x01\x00", 5) + "A", "unicode");

  HLAoctetPairBE pair_be(OctetPair(0x12, 0x34));
  HLAoctetPairLE pair_le(OctetPair(0x12, 0x34));
  assertBytes(pair_be, "\x12\x34", "pair_be");
  assertBytes(pair_le, "\x34\x12", "pair_le");

  HLAopaqueData opaque;
  const Octet opaque_bytes[] = {
      static_cast<Octet>(0x01U), static_cast<Octet>(0x02U), static_cast<Octet>(0xffU)};
  opaque.set(opaque_bytes, sizeof(opaque_bytes));
  assertBytes(opaque, std::string("\x00\x00\x00\x03\x01\x02\xff", 7), "opaque");
  assert(opaque.dataLength() == 3U);
  HLAopaqueData decoded_opaque;
  decoded_opaque.decode(opaque.encode());
  assert(decoded_opaque.dataLength() == 3U);
  assert(std::memcmp(decoded_opaque.get(), opaque_bytes, sizeof(opaque_bytes)) == 0);

  bool rejected = false;
  try {
    integer.decode(VariableLengthData("\x00", 1));
  } catch (EncoderException const&) {
    rejected = true;
  }
  assert(rejected);

  rejected = false;
  try {
    truth.decode(VariableLengthData("\x00\x00\x00\x02", 4));
  } catch (EncoderException const&) {
    rejected = true;
  }
  assert(rejected);

  rejected = false;
  try {
    decoded_opaque.decode(VariableLengthData("\x00\x00\x00\x04\x01", 5));
  } catch (EncoderException const&) {
    rejected = true;
  }
  assert(rejected);

  return 0;
}
