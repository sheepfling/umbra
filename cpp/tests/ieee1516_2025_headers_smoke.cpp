#include <type_traits>

#include <RTI/RTI1516.h>

static_assert(HLA_API_MAJOR_VERSION == 2);
static_assert(HLA_API_MINOR_VERSION == 0);
static_assert(std::has_virtual_destructor_v<rti1516_2025::RTIambassador>);
static_assert(std::is_abstract_v<rti1516_2025::RTIambassador>);

int main() {
  return 0;
}
