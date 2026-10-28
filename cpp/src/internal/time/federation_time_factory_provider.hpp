#pragma once

#include <RTI/libfedtime/LogicalTimeFactoryFactory.h>

#if defined(UMBRA_BUILD_SHARED_LIBRARIES)
#include <RTI/time/HLAlogicalTimeFactoryFactory.h>
#endif

namespace umbra::detail {

// In the static profile, the federation supplies the implementation of the
// standard fedtime factory-factory hook. Shared RTI builds currently retain
// the built-in reference factory until their runtime provider-loading seam is
// available.
[[nodiscard]] inline std::unique_ptr<rti1516_2025::LogicalTimeFactory>
makeFederationLogicalTimeFactory(std::wstring const& implementationName) {
#if defined(UMBRA_BUILD_SHARED_LIBRARIES)
  return rti1516_2025::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(implementationName);
#else
  return rti1516_2025::LogicalTimeFactoryFactory::makeLogicalTimeFactory(implementationName);
#endif
}

}  // namespace umbra::detail
