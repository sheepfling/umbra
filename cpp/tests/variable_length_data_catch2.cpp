#include <catch2/catch_test_macros.hpp>

#include <RTI/RTI1516.h>
#include <RTI/encoding/EncodingConfig.h>

#include <array>
#include <cstring>
#include <cstddef>

namespace {

using rti1516_2025::Octet;
using rti1516_2025::VariableLengthData;

std::size_t adoptedDataDeleteCount = 0;

void deleteAdoptedData(void* data) {
  ++adoptedDataDeleteCount;
  delete[] static_cast<Octet*>(data);
}

bool contains(VariableLengthData const& value, void const* expected, std::size_t expectedSize) {
  return value.size() == expectedSize &&
         (expectedSize == 0 || std::memcmp(value.data(), expected, expectedSize) == 0);
}

}  // namespace

TEST_CASE("VariableLengthData copies caller-owned data and preserves value copies", "[baseline][support][variable-length-data]") {
  std::array<Octet, 4> source{'H', 'L', 'A', '!'};
  std::array<Octet, 4> const expected{'H', 'L', 'A', '!'};

  VariableLengthData value(source.data(), source.size());
  source.front() = 'X';

  REQUIRE(contains(value, expected.data(), expected.size()));

  VariableLengthData copy(value);
  VariableLengthData assigned;
  assigned = value;

  std::array<Octet, 2> replacement{'O', 'K'};
  value.setData(replacement.data(), replacement.size());

  REQUIRE(contains(value, replacement.data(), replacement.size()));
  REQUIRE(contains(copy, expected.data(), expected.size()));
  REQUIRE(contains(assigned, expected.data(), expected.size()));
}

TEST_CASE("VariableLengthData copies borrowed storage before the caller mutates it", "[baseline][support][variable-length-data]") {
  std::array<Octet, 3> borrowedSource{'o', 'l', 'd'};
  std::array<Octet, 3> const expected{'o', 'l', 'd'};
  VariableLengthData alias;
  alias.setDataPointer(borrowedSource.data(), borrowedSource.size());

  VariableLengthData copy(alias);
  VariableLengthData assigned;
  assigned = alias;

  borrowedSource[0] = 'n';
  borrowedSource[1] = 'e';
  borrowedSource[2] = 'w';

  REQUIRE(contains(alias, borrowedSource.data(), borrowedSource.size()));
  REQUIRE(contains(copy, expected.data(), expected.size()));
  REQUIRE(contains(assigned, expected.data(), expected.size()));
}

TEST_CASE("VariableLengthData releases adopted storage once when it is replaced", "[baseline][support][variable-length-data]") {
  adoptedDataDeleteCount = 0;
  auto* adopted = new Octet[3]{'R', 'T', 'I'};
  std::array<Octet, 3> const expected{'R', 'T', 'I'};

  {
    VariableLengthData value;
    value.takeDataPointer(adopted, expected.size(), &deleteAdoptedData);
    REQUIRE(contains(value, expected.data(), expected.size()));

    VariableLengthData copy(value);
    REQUIRE(contains(copy, expected.data(), expected.size()));

    std::array<Octet, 1> replacement{'!'};
    value.setData(replacement.data(), replacement.size());
    REQUIRE(adoptedDataDeleteCount == 1);
    REQUIRE(contains(value, replacement.data(), replacement.size()));
  }

  REQUIRE(adoptedDataDeleteCount == 1);
}
