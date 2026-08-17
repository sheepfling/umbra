#include <RTI/RTI1516.h>

namespace rti1516_2025 {

std::unique_ptr<LogicalTimeFactory> LogicalTimeFactoryFactory::makeLogicalTimeFactory(
    std::wstring const& implementationName) {
  return HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(implementationName);
}

}  // namespace rti1516_2025
